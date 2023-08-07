// Author: Pierre Michaud
// Release 1, may 2018

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <algorithm>

#include "batage.hpp"

#define SIMFASTER

#ifdef SIMFASTER
#define ASSERT(cond)
#else
#define ASSERT(cond) if (!(cond)) {fprintf(stderr,"file %s assert line %d\n",__FILE__,__LINE__); abort();}
#endif


uint32_t
rando()
{
  // Marsaglia's xorshift
  static uint32_t x = 2463534242;
  x ^= x << 13;
  x ^= x >> 17;
  x ^= x << 5;
  return x;
}


int
gcd(int a, int b)
{
  if (a == b) {
    return a;
  } else if (a > b) {
    return gcd(b,a-b);
  } else{
    return gcd(a,b-a);
  }
}


int
lcm(int a, int b)
{
  int g = gcd(a,b);
  ASSERT(g>0);
  return (a*b)/g;
}


uint32_t
reverse(uint32_t x, int nbits)
{
  // reverse the 16 rightmost bit (Strachey's method)
  ASSERT(nbits <= 16);
  x &= 0xFFFF;
  x |= (x & 0x000000FF) << 16;
  x = (x & 0xF0F0F0F0) | ((x & 0x0F0F0F0F) << 8);
  x = (x & 0xCCCCCCCC) | ((x & 0x33333333) << 4);
  x = (x & 0xAAAAAAAA) | ((x & 0x55555555) << 2);
  return x >> (31-nbits);
}


dualcounter::dualcounter(int b1, int b2)
{
  ASSERT(BHYSTBITS>=1);
  n[b1^1] = (1<<(BHYSTBITS-1)) - 1;
  n[b1] = n[b1^1] + (1<<BHYSTBITS) - b2;
}


dualcounter::dualcounter()
{
  reset();
}


void
dualcounter::reset()
{
  n[0] = n[1] = 0;
}


int
dualcounter::pred()
{
  return (n[1] > n[0]);
}


bool
dualcounter::mediumconf()
{
  return (n[1] == (2*n[0]+1)) || (n[0] == (2*n[1]+1));
}


bool
dualcounter::lowconf()
{
  return (n[1] < (2*n[0]+1)) && (n[0] < (2*n[1]+1));
}


bool
dualcounter::highconf()
{
  return ! mediumconf() && ! lowconf();
}


bool
dualcounter::veryhighconf()
{
  ASSERT(nmax==7);
  // the formula below works with nmax=7 (see the BATAGE paper)
  return ((n[0]==0) && (n[1]>=4)) || ((n[1]==0) && (n[0]>=4));
}


int
dualcounter::sum()
{
  return n[1] + n[0];
}


int
dualcounter::diff()
{
  return abs(n[1]-n[0]);
}


bool
dualcounter::saturated()
{
  return (n[0] == nmax) || (n[1] == nmax);
}


int
dualcounter::conflevel(int meta)
{
  if (n[1]==n[0]) return 3;
  bool low = lowconf();
  bool promotemed = (meta>=0) && (sum()==1);
  bool med = ! promotemed && mediumconf();
  return (low<<1) | med;
}


void
dualcounter::decay()
{
  if (n[1] > n[0]) n[1]--;
  if (n[0] > n[1]) n[0]--;
}


void
dualcounter::update(bool taken)
{
  ASSERT((n[0]>=0) && (n[0]<=nmax));
  ASSERT((n[1]>=0) && (n[1]<=nmax));
  int d = (taken)? 1:0;
  if (n[d] < nmax) {
    n[d]++;
  } else if (n[d^1] > 0) {
    n[d^1]--;
  }
}


int
dualcounter::size()
{
  unsigned x = nmax;
  int nbits = 0;
  while (x) {
    nbits++;
    x>>=1;
  }
  return 2*nbits;
}


void
path_history::init(int hlen)
{
// Just allocates a history vector, initializes to 0 and sets ptr = 0
  hlength = hlen;
  h = new unsigned [hlen];
  for (int i=0; i<hlength; i++) {
    h[i] = 0;
  }
  ptr = 0;
}


void 
path_history::insert(unsigned val)
{
// Path history update
  ptr--;
  if (ptr == (-1)) {
    ptr = hlength-1;
  }
  h[ptr] = val;
}


unsigned & 
path_history::operator [] (int n)
{
// Reads history h[(ptr+n)%hlength]
  ASSERT((n>=0) && (n<hlength));
  int k = ptr + n;
  if (k >= hlength) {
    k -= hlength;
  }
  ASSERT((k>=0) && (k<hlength));
  return h[k];
}


void 
folded_history::init(int original_length, int compressed_length, int injected_bits)
{
  olength = original_length;
  clength = compressed_length;
  outpoint = olength % clength;
  ASSERT(clength < 32);
  injected_bits = min(injected_bits,clength);
  mask1 = (1<<clength)-1;
  mask2 = (1<<injected_bits)-1;
  fold = 0; // must be consistent with path_history::init()
}


uint32_t
folded_history::rotateleft(uint32_t x, int m)
{
  ASSERT(m < clength);
  ASSERT((x>>clength) == 0);
  uint32_t y = x >> (clength-m);
  x = (x << m) | y;
  return x & mask1;
}


void 
folded_history::update(path_history & ph)
{
  fold = rotateleft(fold,1);
  unsigned inbits = ph[0] & mask2;
  unsigned outbits = ph[olength] & mask2;
  outbits = rotateleft(outbits,outpoint);
  fold ^= inbits ^ outbits;
}


histories::histories()
{
  int * hist = new int [NUMG];
  int prevh = 0;
  for (int i=0; i<NUMG; i++) {
    // geometric history lengths
    int h = MINHIST * pow((double)MAXHIST/MINHIST,(double)i/(NUMG-1));
    h = max(prevh+1,h);
    hist[NUMG-1-i] = h; // hist[0] = longest history
    prevh = h;
  }
  bh.init(MAXHIST+1);
  ph.init(MAXPATH+1);
  chg = new folded_history [NUMG];
  chgg = new folded_history [NUMG];
  cht = new folded_history [NUMG];
  chtt = new folded_history [NUMG];
  for (int i=0; i<NUMG; i++) {
    chg[i].init(hist[i],LOGGE,1);
    cht[i].init(hist[i],TAGBITS,1);
    int hashparam = 1;
    if (LOGGE == TAGBITS) {
      hashparam = (lcm(LOGGE,LOGGE-3) > lcm(LOGGE,LOGGE-2))? 3 : 2;
    }
    if (hist[i] <= MAXPATH) {
      chgg[i].init(hist[i],LOGGE-hashparam,PATHBITS);
      chtt[i].init(hist[i],TAGBITS-1,PATHBITS);
    } else {
      chgg[i].init(hist[i],LOGGE-hashparam,1);
      chtt[i].init(hist[i],TAGBITS-1,1);
    }
  }
}


void 
histories::update(uint32_t targetpc, bool taken)
{
#ifdef PC_SHIFT
  //targetpc ^= targetpc << 5;
  targetpc ^= targetpc >> PC_SHIFT;
#endif
  bh.insert(taken);
  ph.insert(targetpc);
  for (int i=0; i<NUMG; i++) {
    chg[i].update(bh);
    cht[i].update(bh);
    if (chgg[i].olength <= MAXPATH) {
      chgg[i].update(ph);
      chtt[i].update(ph);
    } else {
      chgg[i].update(bh);
      chtt[i].update(bh);
    }      
  }
}

// the hash functions below are somewhat more complex than what would be implemented in real processors

int
histories::gindex(uint32_t pc, int i) const
{
  ASSERT((i>=0) && (i<NUMG));
  uint32_t hash = pc ^ i ^ chg[i].fold ^ (chgg[i].fold << (chg[i].clength-chgg[i].clength));
  return hash & ((1<<LOGGE)-1);
}


int
histories::gtag(uint32_t pc, int i) const
{
  ASSERT((i>=0) && (i<NUMG));
  uint32_t hash = (pc+i) ^ reverse(cht[i].fold,cht[i].clength) ^ (chtt[i].fold << (cht[i].clength-chtt[i].clength));
#ifdef GHGBITS
  // when bank interleaving is enabled, introducing 1 or 2 bits in the tag
  // for identifying the path length generally reduces tag aliasing when NUMG is large
  hash = (hash << GHGBITS) | ghg(i);
#endif
  return hash & ((1<<TAGBITS)-1);
}


#ifdef BANK_INTERLEAVING
// inspired from Seznec's TAGE-SC-L (CBP 2016), but different:
// interleaving is global, unlike in TAGE-SC-L ==> unique tag size
int
histories::phybank(int i) const
{
  ASSERT((i>=0) && (i<NUMG));
  unsigned pos;
  if (i >= (NUMG-MIDBANK)) {
    pos = i;
  } else {
    // on some workloads, the shortest non-null global history length does not generate
    // enough entropy, which may lead to uneven utilization of banks, hence MIDBANK
    pos = (chgg[NUMG-MIDBANK-1].fold + i) % (NUMG-MIDBANK);
  }
  return (chgg[NUMG-1].fold + pos) % NUMG;
}


#ifdef GHGBITS
int
histories::ghg(int i) const
{
  return ((NUMG-1-i) << GHGBITS) / NUMG;
}
#endif

#endif // BANK_INTERLEAVING


void
histories::printconfig()
{
  printf("history lengths: ");
  for (int i=NUMG-1; i>=0; i--) {
    printf("%d ",chg[i].olength);
  }
  printf("\n");
}


int
histories::size()
{
  return MAXHIST + MAXPATH * PATHBITS; // number of bits
  // The storage for folded histories is ignored here
  // If folded histories are implemented in hardware, they must be checkpointed (cf. Schlais & Lipasti, ICCD 2016)
  // An implementation without folded histories is possible (cf. Seznec's GEHL, ISCA 2005)
}


tagged_entry::tagged_entry()
{
  tag = 0;
  dualc.reset();
}


batage::batage()
{
  g = new tagged_entry** [NUMG];
  for (int i=0; i<NUMG; i++) {
    g[i] = new tagged_entry* [1<<LOGGE];
    for (int j = 0; j < (1<<LOGGE); j++)
    {
    	g[i][j] = new tagged_entry [INFO_PER_ENTRY];
    }
  }
  gi = new int [NUMG];
  
  for (int i=0; i<(1<<LOGBE); i++) 
  {
  	for (int j = 0; j < INFO_PER_ENTRY; j++)
  	{
      	b[i][j] = 0; // not-taken prediction
    }
  }
  for (int i=0; i<(1<<LOGB2E); i++) 
  {
  	for (int j = 0; j < INFO_PER_ENTRY; j++)
  	{
    	b2[i][j] = (1<<BHYSTBITS)-1; // weak state
    }
  }
  cat = 0;
  meta = -1;
#ifdef USE_META
  meta = 0;
#endif
#ifdef USE_CD
  cd = 0;
#endif
#ifdef BANK_INTERLEAVING
  bank = new int [NUMG];
  check = new bool [NUMG]; 
#endif   

#ifdef DEBUG
	predict_pcs = fopen("predict_pcs.txt", "w+");
	update_pcs = fopen("update_pcs.txt", "w+");
#endif // DEBUG

}


#ifdef BANK_INTERLEAVING
void
batage::check_bank_conflicts()
{
  for (int i=0; i<NUMG; i++) {
    check[i] = false;
  }
  for (int i=0; i<NUMG; i++) {
    if (check[bank[i]]) {
      fprintf(stderr,"BANK CONFLICT\n");
      ASSERT(0);
    }
    check[bank[i]] = true;
  }
}
#endif 

// i is the bank number - not the index within bank
tagged_entry &
batage::getgb(int i)
{
  ASSERT((i>=0) && (i<NUMG));
#ifdef BANK_INTERLEAVING
  ASSERT((bank[i]>=0) && (bank[i]<NUMG));
  return g[bank[i]][gi[i]][0];
#else
  return g[i][gi[i]][0];
#endif
}

// i is the bank number - not the index within bank
tagged_entry &
batage::getgo(int i, uint32_t offset_within_entry)
{
  ASSERT((i>=0) && (i<NUMG));
#ifdef BANK_INTERLEAVING
  ASSERT((bank[i]>=0) && (bank[i]<NUMG));
  return g[bank[i]][gi[i]][offset_within_entry];
#else
  return g[i][gi[i]][offset_within_entry];
#endif
}

bool 
batage::predict(uint32_t pc, const histories & p)
{

#ifdef DEBUG
	fprintf(predict_pcs, "%lu \n", pc);
#endif // DEBUG

#ifdef PC_SHIFT
  //pc ^= pc << 5;
  pc ^= pc >> PC_SHIFT;
#endif
  
  uint32_t offset_within_entry = pc % INFO_PER_ENTRY;
  
  hit.clear();
  s.clear();
  for (int i=0; i<NUMG; i++) {
#ifdef BANK_INTERLEAVING
    bank[i] = p.phybank(i);
#endif
    gi[i] = p.gindex(pc,i);
    if (getgb(i).tag == p.gtag(pc,i)) {
      hit.push_back(i);
      s.push_back(getgo(i, offset_within_entry).dualc);
    }
  }
  
#ifdef BANK_INTERLEAVING
#ifndef SIMFASTER
  check_bank_conflicts();
#endif
#endif

  bi = pc & ((1<<LOGBE)-1);
  bi2 = bi & ((1<<LOGB2E)-1);
    b_bi = b[bi][offset_within_entry];
	b2_bi2 = b2[bi2][offset_within_entry];
  s.push_back(dualcounter(b_bi,b2_bi2));

	// bp = index within s
  bp = 0;
  for (int i=1; i<(int)s.size(); i++) {
    if (s[i].conflevel(meta) < s[bp].conflevel(meta)) {
      bp = i;
    }
  }
  
  // For superscalar - save s, p, hit 
  return s[bp].pred();
}


void
batage::update_bimodal(bool taken, uint32_t offset_within_entry)
{
  // see Loh et al., "Exploiting bias in the hysteresis bit of 2-bit saturating counters in branch predictors", Journal of ILP, 2003   
  if (b[bi][offset_within_entry] == taken) {
    if (b2[bi2][offset_within_entry]>0) b2[bi2][offset_within_entry]--;
  } else {
    if (b2[bi2][offset_within_entry] < ((1<<BHYSTBITS)-1)) {
      b2[bi2][offset_within_entry]++;
    } else {
      b[bi][offset_within_entry] = taken;
    }
  }  
}

// i - index within s
void
batage::update_entry(int i, uint32_t offset_within_entry, bool taken)
{
  ASSERT(i<s.size());
  if (i<(int)hit.size()) {
    getgo(hit[i], offset_within_entry).dualc.update(taken);
  } else {
    update_bimodal(taken, offset_within_entry);
  }
}

/*For superscalar - anything that is non constant & is used before/ without assigning a value must be saved in ftq*/
void
batage::update(uint32_t pc, bool taken, const histories & p, bool noalloc = false)
{

#ifdef DEBUG
	fprintf(update_pcs, "%lu \n", pc);
#endif // DEBUG

#ifdef PC_SHIFT
  //pc ^= pc << 5;
  pc ^= pc >> PC_SHIFT;
#endif

uint32_t offset_within_entry = pc % INFO_PER_ENTRY;
b[bi][offset_within_entry] = b_bi;
b2[bi2][offset_within_entry] = b2_bi2;

#ifdef USE_META
  if ((s.size()>1) && (s[0].sum()==1) && s[1].highconf() && (s[0].pred() != s[1].pred())) {
    if (s[0].pred() == taken) {
      if (meta<15) meta++;
    } else {
      if (meta>(-16)) meta--;
    }
  }
#endif
  
  // update from 0 to bp-1
  for (int i=0; i<bp; i++) {
    if ((meta>=0) || s[i].lowconf() || (s[i].pred() != s[bp].pred()) || ((rando() % 8)==0)) {
      getgo(hit[i], offset_within_entry).dualc.update(taken);
    }
  }
  // update at bp
  if ((bp<(int)hit.size()) && s[bp].highconf() && s[bp+1].highconf() && (s[bp+1].pred()==taken) && ((s[bp].pred()==taken) || (cat>=(CATMAX/2)))) {
    if (! s[bp].saturated() || ((meta<0) && ((rando() % 8)==0))) { 
      getgo(hit[bp], offset_within_entry).dualc.decay();
    }
  } else {
    update_entry(bp, offset_within_entry, taken);
  }
  // update at bp+1
  if (! s[bp].highconf() && (bp<(int)hit.size())) {
    update_entry(bp+1, offset_within_entry, taken);
  }
 
  // ALLOCATE
	// TODO Check what to do for these getg (for allocation)
  bool allocate = ! noalloc && (s[bp].pred() != taken);

  if (allocate && ((int)(rando() % MINAP) >= ((cat * MINAP) / (CATMAX+1)))) {
    int i = (hit.size()>0)? hit[0] : NUMG;
    i -= rando() % (1+s[0].diff()*SKIPMAX/dualcounter::nmax);
    int mhc = 0;
    while (--i >= 0) {
      if (getgo(i, offset_within_entry).dualc.highconf()) {
#ifdef USE_CD
        if ((int)(rando() % MINDP) >= ((cd*MINDP)/(CDMAX+1))) getgo(i, offset_within_entry).dualc.decay();
#else
        if ((rando() % 4)==0) getgo(i, offset_within_entry).dualc.decay();
#endif
        if (! getgo(i, offset_within_entry).dualc.veryhighconf()) mhc++;
      } else { 
      // TODO Check what to do for these three
        getgb(i).tag = p.gtag(pc,i);
        for (uint32_t offset = 0; offset < INFO_PER_ENTRY; offset++)
        {
        	getgo(i, offset).dualc.reset();
        	if (offset == offset_within_entry)
        	{
        		getgo(i, offset).dualc.update(taken);
        	}
        }
        cat += CATR_NUM - mhc * CATR_DEN;
        cat = min(CATMAX,max(0,cat));
#ifdef USE_CD
        cd += CDR_NUM - mhc * CDR_DEN;
        cd = min(CDMAX,max(0,cd));
#endif
        break;
      }
    }
  }

}


int
batage::size()
{
  int totsize = (1 << LOGB) + (BHYSTBITS << LOGB2);
  totsize += NUMG * ((dualcounter::size() + TAGBITS) << LOGG);
  return totsize; // number of bits
  // the storage for counters 'cat', 'meta' and 'cd' is neglected here
}


