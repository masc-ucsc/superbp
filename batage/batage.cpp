// Author: Pierre Michaud
// Release 1, may 2018

#include <algorithm>
#include <math.h>
#include <stdio.h>
#include <iostream>
#include <stdlib.h>

#include "batage.hpp"

#define SIMFASTER

#ifdef SIMFASTER
#define ASSERT(cond)
#else
#define ASSERT(cond)                                                           \
  if (!(cond)) {                                                               \
    fprintf(stderr, "file %s assert line %d\n", __FILE__, __LINE__);           \
    abort();                                                                   \
  }
#endif

uint32_t rando() {
  // Marsaglia's xorshift
  static uint32_t x = 2463534242;
  x ^= x << 13;
  x ^= x >> 17;
  x ^= x << 5;
  return x;
}

int gcd(int a, int b) {
  if (a == b) {
    return a;
  } else if (a > b) {
    return gcd(b, a - b);
  } else {
    return gcd(a, b - a);
  }
}

int lcm(int a, int b) {
  int g = gcd(a, b);
  ASSERT(g > 0);
  return (a * b) / g;
}

uint32_t reverse(uint32_t x, int nbits) {
  // reverse the 16 rightmost bit (Strachey's method)
  ASSERT(nbits <= 16);
  x &= 0xFFFF;
  x |= (x & 0x000000FF) << 16;
  x = (x & 0xF0F0F0F0) | ((x & 0x0F0F0F0F) << 8);
  x = (x & 0xCCCCCCCC) | ((x & 0x33333333) << 4);
  x = (x & 0xAAAAAAAA) | ((x & 0x55555555) << 2);
  return x >> (31 - nbits);
}

dualcounter::dualcounter(int b1, int b2) {
  ASSERT(BHYSTBITS >= 1);
  n[b1 ^ 1] = (1 << (BHYSTBITS - 1)) - 1;
  n[b1] = n[b1 ^ 1] + (1 << BHYSTBITS) - b2;
}

dualcounter::dualcounter() { reset(); }

void dualcounter::reset() { n[0] = n[1] = 0; }

int dualcounter::pred() { return (n[1] > n[0]); }

bool dualcounter::mediumconf() {
  return (n[1] == (2 * n[0] + 1)) || (n[0] == (2 * n[1] + 1));
}

bool dualcounter::lowconf() {
  return (n[1] < (2 * n[0] + 1)) && (n[0] < (2 * n[1] + 1));
}

bool dualcounter::highconf() { return !mediumconf() && !lowconf(); }

bool dualcounter::veryhighconf() {
  ASSERT(nmax == 7);
  // the formula below works with nmax=7 (see the BATAGE paper)
  return ((n[0] == 0) && (n[1] >= 4)) || ((n[1] == 0) && (n[0] >= 4));
}

int dualcounter::sum() { return n[1] + n[0]; }

int dualcounter::diff() { return abs(n[1] - n[0]); }

bool dualcounter::saturated() { return (n[0] == nmax) || (n[1] == nmax); }

int dualcounter::conflevel(int meta) {
  if (n[1] == n[0])
    return 3;
  bool low = lowconf();
  bool promotemed = (meta >= 0) && (sum() == 1);
  bool med = !promotemed && mediumconf();
  return (low << 1) | med;
}

void dualcounter::decay() {
  if (n[1] > n[0])
    n[1]--;
  if (n[0] > n[1])
    n[0]--;
}

void dualcounter::update(bool taken) {
  ASSERT((n[0] >= 0) && (n[0] <= nmax));
  ASSERT((n[1] >= 0) && (n[1] <= nmax));
  int d = (taken) ? 1 : 0;
  if (n[d] < nmax) {
    n[d]++;
  } else if (n[d ^ 1] > 0) {
    n[d ^ 1]--;
  }
}

int dualcounter::size() {
  unsigned x = nmax;
  int nbits = 0;
  while (x) {
    nbits++;
    x >>= 1;
  }
  return 2 * nbits;
}

void path_history::init(int hlen) {
  // Just allocates a history vector, initializes to 0 and sets ptr = 0
  hlength = hlen;
  h = new unsigned[hlen];
  for (int i = 0; i < hlength; i++) {
    h[i] = 0;
  }
  ptr = 0;
}

void path_history::insert(unsigned val) {
  // Path history update
  ptr--;
  if (ptr == (-1)) {
    ptr = hlength - 1;
  }
  h[ptr] = val;
}

unsigned &path_history::operator[](int n) {
  // Reads history h[(ptr+n)%hlength]
  ASSERT((n >= 0) && (n < hlength));
  int k = ptr + n;
  if (k >= hlength) {
    k -= hlength;
  }
  ASSERT((k >= 0) && (k < hlength));
  return h[k];
}

void folded_history::init(int original_length, int compressed_length,
                          int injected_bits) {
  olength = original_length;
  clength = compressed_length;
  outpoint = olength % clength;
  ASSERT(clength < 32);
  injected_bits = min(injected_bits, clength);
  mask1 = (1 << clength) - 1;
  mask2 = (1 << injected_bits) - 1;
  fold = 0; // must be consistent with path_history::init()
}

uint32_t folded_history::rotateleft(uint32_t x, int m) {
  ASSERT(m < clength);
  ASSERT((x >> clength) == 0);
  uint32_t y = x >> (clength - m);
  x = (x << m) | y;
  return x & mask1;
}

void folded_history::update(path_history &ph) {
  fold = rotateleft(fold, 1);
  unsigned inbits = ph[0] & mask2;
  unsigned outbits = ph[olength] & mask2;
  outbits = rotateleft(outbits, outpoint);
  fold ^= inbits ^ outbits;
}

histories::histories() {
  int *hist = new int[NUMG];
  int prevh = 0;
  for (int i = 0; i < NUMG; i++) {
    // geometric history lengths
    int h = MINHIST * pow((double)MAXHIST / MINHIST, (double)i / (NUMG - 1));
    h = max(prevh + 1, h);
    hist[NUMG - 1 - i] = h; // hist[0] = longest history
    prevh = h;
  }
  bh.init(MAXHIST + 1);
  ph.init(MAXPATH + 1);
  chg = new folded_history[NUMG];
  chgg = new folded_history[NUMG];
  cht = new folded_history[NUMG];
  chtt = new folded_history[NUMG];
  for (int i = 0; i < NUMG; i++) {
    chg[i].init(hist[i], LOGGE(i), 1);
    cht[i].init(hist[i], TAGBITS, 1);
    int hashparam = 1;
    if (LOGGE(i) == TAGBITS) {
      hashparam = (lcm(LOGGE(i), LOGGE(i) - 3) > lcm(LOGGE(i), LOGGE(i) - 2)) ? 3 : 2;
    }
    if (hist[i] <= MAXPATH) {
      chgg[i].init(hist[i], LOGGE(i) - hashparam, PATHBITS);
      chtt[i].init(hist[i], TAGBITS - 1, PATHBITS);
    } else {
      chgg[i].init(hist[i], LOGGE(i) - hashparam, 1);
      chtt[i].init(hist[i], TAGBITS - 1, 1);
    }
  }
}

void histories::update(uint32_t targetpc, bool taken) {
#ifdef PC_SHIFT
  // targetpc ^= targetpc << 5;
  targetpc ^= targetpc >> PC_SHIFT;
#endif
  bh.insert(taken);
  ph.insert(targetpc);
  for (int i = 0; i < NUMG; i++) {
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

// the hash functions below are somewhat more complex than what would be
// implemented in real processors

int histories::gindex(uint32_t pc, int i) const {
  ASSERT((i >= 0) && (i < NUMG));
#ifdef DEBUG_GINDEX
//fprintf (stderr, "gindex, %lx, %d \n", pc, i);
#endif
  uint32_t hash = pc ^ i ^ chg[i].fold ^
                  (chgg[i].fold << (chg[i].clength - chgg[i].clength));
   int raw_index = hash & ((1 << LOGGE(i)) - 1);
   int rolled_index = (raw_index % ENTRIES_PER_TABLE(i));
#ifdef DEBUG_GINDEX
fprintf (stderr, "ENTRIES_PER_TABLE = %d, rolled_index = %d \n", ENTRIES_PER_TABLE(i), rolled_index);
#endif
   return rolled_index;
}

int histories::gtag(uint32_t pc, int i) const {

#ifdef DEBUG 
//fprintf (stderr, "gtag, %d, %d \n", pc, i);
#endif

  ASSERT((i >= 0) && (i < NUMG));
  uint32_t hash = (pc + i) ^ reverse(cht[i].fold, cht[i].clength) ^
                  (chtt[i].fold << (cht[i].clength - chtt[i].clength));
#ifdef GHGBITS
  // when bank interleaving is enabled, introducing 1 or 2 bits in the tag
  // for identifying the path length generally reduces tag aliasing when NUMG is
  // large
  hash = (hash << GHGBITS) | ghg(i);
#endif
  return hash & ((1 << TAGBITS) - 1);
}

#ifdef BANK_INTERLEAVING
// inspired from Seznec's TAGE-SC-L (CBP 2016), but different:
// interleaving is global, unlike in TAGE-SC-L ==> unique tag size
int histories::phybank(int i) const {
  ASSERT((i >= 0) && (i < NUMG));
  unsigned pos;
  if (i >= (NUMG - MIDBANK)) {
    pos = i;
  } else {
    // on some workloads, the shortest non-null global history length does not
    // generate enough entropy, which may lead to uneven utilization of banks,
    // hence MIDBANK
    pos = (chgg[NUMG - MIDBANK - 1].fold + i) % (NUMG - MIDBANK);
  }
  return (chgg[NUMG - 1].fold + pos) % NUMG;
}

#ifdef GHGBITS
int histories::ghg(int i) const { return ((NUMG - 1 - i) << GHGBITS) / NUMG; }
#endif

#endif // BANK_INTERLEAVING

void histories::printconfig() {
  printf("history lengths: ");
  for (int i = NUMG - 1; i >= 0; i--) {
    printf("%d ", chg[i].olength);
  }
  printf("\n");
}

int histories::size() {
  return MAXHIST + MAXPATH * PATHBITS; // number of bits
  // The storage for folded histories is ignored here
  // If folded histories are implemented in hardware, they must be checkpointed
  // (cf. Schlais & Lipasti, ICCD 2016) An implementation without folded
  // histories is possible (cf. Seznec's GEHL, ISCA 2005)
}

tagged_entry::tagged_entry() {
  tag = 0;
  dualc.reset();
}

batage::batage() {

  g = new tagged_entry **[NUMG];
  
  for (int i = 0; i < NUMG; i++) {
    g[i] = new tagged_entry *[ENTRIES_PER_TABLE(i)];
    for (int j = 0; j < (ENTRIES_PER_TABLE(i)); j++) {
      g[i][j] = new tagged_entry[INFO_PER_ENTRY];
    }
  }
  
  gi = new int[NUMG];

  for (int i = 0; i < (1 << LOGBE); i++) {
    for (int j = 0; j < INFO_PER_ENTRY; j++) {
      b[i][j] = 0; // not-taken prediction
    }
  }
  
  for (int i = 0; i < (1 << LOGB2E); i++) {
    for (int j = 0; j < INFO_PER_ENTRY; j++) {
      b2[i][j] = (1 << BHYSTBITS) - 1; // weak state
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
  bank = new int[NUMG];
  check = new bool[NUMG];
#endif

#ifdef DEBUG
  predict_pcs = fopen("predict_pcs.txt", "w+");
  update_pcs = fopen("update_pcs.txt", "w+");
#endif // DEBUG

s.reserve(FETCHWIDTH);
hit.reserve(FETCHWIDTH);

}

#ifdef BANK_INTERLEAVING
void batage::check_bank_conflicts() {
  for (int i = 0; i < NUMG; i++) {
    check[i] = false;
  }
  for (int i = 0; i < NUMG; i++) {
    if (check[bank[i]]) {
      fprintf(stderr, "BANK CONFLICT\n");
      ASSERT(0);
    }
    check[bank[i]] = true;
  }
}
#endif

// i is the bank number - not the index within bank
// Index is taken from gi, must be saved from prediction to update
tagged_entry &batage::getgb(int i) {
  ASSERT((i >= 0) && (i < NUMG));
#ifdef BANK_INTERLEAVING
  ASSERT((bank[i] >= 0) && (bank[i] < NUMG));
  return g[bank[i]][gi[i]][0];
#else
  return g[i][gi[i]][0];
#endif
}

// i is the bank number - not the index within bank
// Index is taken from gi, must be saved from prediction to update
tagged_entry &batage::getgo(int i, uint32_t offset_within_entry) {

#ifdef DEBUG 
//fprintf (stderr, "getgo, %d, %d \n", i, offset_within_entry);
#endif

  ASSERT((i >= 0) && (i < NUMG));
#ifdef BANK_INTERLEAVING
  ASSERT((bank[i] >= 0) && (bank[i] < NUMG));
  return g[bank[i]][gi[i]][offset_within_entry];
#else
  return g[i][gi[i]][offset_within_entry];
#endif
}

std::vector<bool>& batage::predict_vec(uint32_t fetch_pc, const histories &p) {

#ifdef DEBUG
  //fprintf(predict_pcs, "%lu \n", fetch_pc);
  fprintf (stderr, "predict for fetch_pc = %llx\n", fetch_pc);
#endif // DEBUG

#ifdef PC_SHIFT
uint32_t hash_fetch_pc = fetch_pc ^ (fetch_pc >> PC_SHIFT);
#ifndef SINGLE_TAG
    vector <uint32_t> hash_pc;
  uint32_t pc;
  for ( int i = 0;  i < FETCHWIDTH;  i++)
  {
  	pc = fetch_pc + (i<<1);
  	hash_pc.push_back(pc ^ (pc >> PC_SHIFT));
  }
#else
  uint32_t hash_pc = fetch_pc ^ (fetch_pc >> PC_SHIFT);
#endif // SINGLE_TAG
#endif // PC_SHIFT

  uint32_t offset_within_entry, offset_within_packet; // = hash_pc % INFO_PER_ENTRY;
  
#ifdef DEBUG
  std::cerr << "00000" << "\n";
#endif
  
  for (offset_within_packet = 0; offset_within_packet < FETCHWIDTH; offset_within_packet++)
  {
  	hit[offset_within_packet].clear();
  	s[offset_within_packet].clear();
  }
  
for (offset_within_packet = 0; offset_within_packet < FETCHWIDTH; offset_within_packet++)
   {
   
   	 #ifdef XIANGSHAN
   		offset_within_entry = offset_within_packet / (FETCHWIDTH / INFO_PER_ENTRY);
   	#else
   		offset_within_entry = offset_within_packet;
   	#endif
   
     	for (int i = 0; i < NUMG; i++) {
		#ifdef BANK_INTERLEAVING
    		bank[i] = p.phybank(i);
		#ifdef DEBUG
		//fprintf (stderr, "For predict, bank[%d] = %d \n ", i, bank[i]);
		#endif // DEBUG
		#endif
    		gi[i] = p.gindex(hash_fetch_pc, i);
    		
		#ifdef DEBUG
		std::cerr << "11111" << "\n";
		//fprintf (stderr, "For predict, gi[%d] = %d \n ", i, gi[i]);
		#endif // DEBUG
   
     #ifndef SINGLE_TAG
   		if (getgo(i, offset_within_entry).tag == p.gtag(hash_pc[offset_within_packet], i))
    #else
    	        if (getgb(i).tag == p.gtag(hash_fetch_pc, i)) 
    #endif
    		{
			#ifdef DEBUG
			fprintf (stderr, "bank %d hit for offset %d\n", i, offset_within_packet);
			#endif //DEBUG
     		 	hit[offset_within_packet].push_back(i);
			s[offset_within_packet].push_back(getgo(i, offset_within_entry).dualc);
      		}
      		#ifdef DEBUG
     std::cerr << "22222" << "\n";	
     #endif // DEBUG
      	}
    }
    
  /*
  for (int i = 0; i < NUMG; i++) {
#ifdef BANK_INTERLEAVING
    bank[i] = p.phybank(i);
#ifdef DEBUG
//fprintf (stderr, "For predict, bank[%d] = %d \n ", i, bank[i]);
#endif // DEBUG
#endif
    gi[i] = p.gindex(hash_fetch_pc, i);
#ifdef DEBUG
//fprintf (stderr, "For predict, gi[%d] = %d \n ", i, gi[i]);
#endif // DEBUG

    if (getgb(i).tag == p.gtag(hash_pc, i)) {
//#define CHECK_SS
#ifdef DEBUG
	fprintf (stderr, "bank %d hit\n", i);
#endif //DEBUG
      hit.push_back(i);
      for (offset_within_entry = 0; offset_within_entry < INFO_PER_ENTRY; offset_within_entry++)
  		{
      		s[offset_within_entry].push_back(getgo(i, offset_within_entry).dualc);
      	}
    }
}*/


  #ifdef DEBUG
  if (hit.empty())
  {
	fprintf (stderr, "No bank hit\n");
	}
#endif //DEBUG

#ifdef BANK_INTERLEAVING
#ifndef SIMFASTER
  check_bank_conflicts();
#endif
#endif

#ifdef DEBUG
std::cerr << "33333" << "\n";
#endif // DEBUG

  bi = hash_fetch_pc & ((1 << LOGBE) - 1);
  bi2 = bi & ((1 << LOGB2E) - 1);
  b_bi.clear();
  b2_bi2.clear();

  for (offset_within_packet = 0; offset_within_packet < FETCHWIDTH; offset_within_packet++)
  {
  
     	 #ifdef XIANGSHAN
   		offset_within_entry = offset_within_packet / (FETCHWIDTH / INFO_PER_ENTRY);
   	#else
   		offset_within_entry = offset_within_packet;
   	#endif
   	
  		b_bi.push_back (b[bi][offset_within_entry]);
  		b2_bi2.push_back(b2[bi2][offset_within_entry]);
  		s[offset_within_packet].push_back(dualcounter(b_bi[offset_within_packet], b2_bi2[offset_within_packet]));
  }
  
    #ifdef DEBUG
  /*for (offset_within_entry = 0; offset_within_entry < INFO_PER_ENTRY; offset_within_entry++)
  {
  	for (int j=0; j < s[offset_within_entry].size(); j++)
  	{
  fprintf (stderr, "For prediction - s[%d][%d] = %d, %d\n", offset_within_packet, j, \
  s[offset_within_packet][j].n[0], s[offset_within_packet][j].n[1]);
  	}
  }*/
  std::cerr << "44444" << "\n";
  #endif // DEBUG
	
  // bp = index within s
  bp.clear();
  for (offset_within_packet = 0; offset_within_packet < FETCHWIDTH; offset_within_packet++)
  {
  int t = 0;
  	for (int i = 1; i < (int)s[offset_within_packet].size(); i++) {
    	if (s[offset_within_packet][i].conflevel(meta) < s[offset_within_packet][t].conflevel(meta)) {
      	t = i;
    	}
  	}
  	bp.push_back(t);
  	#ifdef DEBUG
  		//fprintf (stderr, "Offset = %d, bp = %d, b_bi = %d, b2_bi2 = %d\n", offset_within_packet, bp[offset_within_packet], b_bi[offset_within_packet], b2_bi2[offset_within_packet]);
	#endif // DEBUG
  }
 
#ifdef DEBUG
	std::cerr << "55555" << "\n";
	#endif // DEBUG
	
  // For superscalar - save s, p, hit
  predict.clear();
  for (offset_within_packet = 0; offset_within_packet < FETCHWIDTH; offset_within_packet++)
  {
   	//predict[offset_within_entry] = s[offset_within_entry][bp].pred();
   	predict.push_back(s[offset_within_packet][bp[offset_within_packet]].pred());
  }
  #ifdef DEBUG
	std::cerr << "66666" << "\n";
	#endif // DEBUG
	// TODO Temporarily Diabled
  //fprintf(stderr, "ba.predict pc=%llx predict:%d\n", pc, predict);

  return predict;
}

void batage::update_bimodal(bool taken, uint32_t offset_within_entry) {
  // see Loh et al., "Exploiting bias in the hysteresis bit of 2-bit saturating
  // counters in branch predictors", Journal of ILP, 2003
  if (b[bi][offset_within_entry] == taken) {
    if (b2[bi2][offset_within_entry] > 0)
      b2[bi2][offset_within_entry]--;
  } else {
    if (b2[bi2][offset_within_entry] < ((1 << BHYSTBITS) - 1)) {
      b2[bi2][offset_within_entry]++;
    } else {
      b[bi][offset_within_entry] = taken;
    }
  }
}

// i - index within s
void batage::update_entry(int i, uint32_t offset_within_packet, bool taken) {
  
  ASSERT(i < s[offset_within_packet].size());
  
#ifdef XIANGSHAN
  uint32_t offset_within_entry = offset_within_packet / (FETCHWIDTH / INFO_PER_ENTRY);
#else
  uint32_t offset_within_entry = offset_within_packet;
#endif
  
  if (i < (int)hit[offset_within_packet].size()) {
    getgo(hit[offset_within_packet][i], offset_within_entry).dualc.update(taken);
  } else {
    update_bimodal(taken, offset_within_entry);
  }
}

/*For superscalar - anything that is non constant & is used before/ without
 * assigning a value must be saved in ftq*/
void batage::update(uint32_t pc, uint32_t fetch_pc, uint32_t offset_within_packet, bool taken, const histories &p,
                    bool noalloc = false) {

#ifdef DEBUG
  //fprintf(update_pcs, "%lu \n", pc);
    fprintf (stderr, "update for pc = %llx, fetch_pc = %llx\n", pc, fetch_pc);
#endif // DEBUG

// Hash was using "actual pc", since this is used only for new entry allocation, NOT for counter update
// Changed to use fetch_pc
#ifdef PC_SHIFT
  // pc ^= pc << 5;
  uint32_t hash_fetch_pc = fetch_pc ^ (fetch_pc >> PC_SHIFT);
  uint32_t hash_pc = pc ^ (pc >> PC_SHIFT);
#endif

#ifdef XIANGSHAN
  uint32_t offset_within_entry = offset_within_packet / (FETCHWIDTH / INFO_PER_ENTRY);
#else
  uint32_t offset_within_entry = offset_within_packet;
#endif
  
  #ifdef DEBUG
  //fprintf (stderr, "offset = %d, bp = %d, b_bi = %d, b2_bi2 = %d\n", offset_within_entry, bp[offset_within_entry], b_bi[offset_within_entry], b2_bi2[offset_within_entry]);
  #endif // DEBUG
  
  // TODO - Check, might/ should not be necessary
  b[bi][offset_within_entry] = b_bi[offset_within_packet];
  b2[bi2][offset_within_entry] = b2_bi2[offset_within_packet];

#ifdef USE_META
  if ((s[offset_within_packet].size() > 1) && (s[offset_within_packet][0].sum() == 1) && s[offset_within_packet][1].highconf() &&
      (s[offset_within_packet][0].pred() != s[offset_within_packet][1].pred())) {
    if (s[offset_within_packet][0].pred() == taken) {
      if (meta < 15)
        meta++;
    } else {
      if (meta > (-16))
        meta--;
    }
  }
#endif

#ifdef BANK_INTERLEAVING
#ifdef DEBUG
  /*for (int i = 0; i < NUMG; i++) {
fprintf (stderr, "For update, bank[%d] = %d \n ", i, bank[i]);
}*/
#endif // DEBUG
#endif
#ifdef DEBUG
/*  for (int i = 0; i < NUMG; i++) {
fprintf (stderr, "For update, gi[%d] = %d \n ", i, gi[i]);
}*/
#endif // DEBUG

  // update from 0 to bp-1
  for (int i = 0; i < bp[offset_within_packet]; i++) {
    if ((meta >= 0) || s[offset_within_packet][i].lowconf() || (s[offset_within_packet][i].pred() != s[offset_within_packet][bp[offset_within_packet]].pred()) ||
        ((rando() % 8) == 0)) {
      getgo(hit[offset_within_packet][i], offset_within_entry).dualc.update(taken);
    }
  }
  // update at bp
  if ((bp[offset_within_packet] < (int)hit[offset_within_packet].size()) && s[offset_within_packet][bp[offset_within_packet]].highconf() && s[offset_within_packet][bp[offset_within_packet] + 1].highconf() &&
      (s[offset_within_packet][bp[offset_within_packet] + 1].pred() == taken) &&
      ((s[offset_within_packet][bp[offset_within_packet]].pred() == taken) || (cat >= (CATMAX / 2)))) {
    if (!s[offset_within_packet][bp[offset_within_packet]].saturated() || ((meta < 0) && ((rando() % 8) == 0))) {
      getgo(hit[offset_within_packet][bp[offset_within_packet]], offset_within_entry).dualc.decay();
    }
  } else {
    update_entry(bp[offset_within_packet], offset_within_packet, taken);
  }
  // update at bp+1
  if (!s[offset_within_packet][bp[offset_within_packet]].highconf() && (bp[offset_within_packet] < (int)hit[offset_within_packet].size())) {
    update_entry(bp[offset_within_packet] + 1, offset_within_packet, taken);
  }

  // ALLOCATE
  // TODO Check what to do for these getg (for allocation)
  bool allocate = !noalloc && (s[offset_within_packet][bp[offset_within_packet]].pred() != taken);

  #ifdef DEBUG
  fprintf (stderr, "For allocation - s[%d][%d] = %d, %d\n", offset_within_packet, bp[offset_within_packet], \
  s[offset_within_packet][bp[offset_within_packet]].n[0], s[offset_within_packet][bp[offset_within_packet]].n[1]);
  #endif // DEBUG

  if (allocate && ((int)(rando() % MINAP) >= ((cat * MINAP) / (CATMAX + 1)))) {
    int i = (hit[offset_within_packet].size() > 0) ? hit[offset_within_packet][0] : NUMG;
    i -= rando() % (1 + s[offset_within_packet][0].diff() * SKIPMAX / dualcounter::nmax);
    int mhc = 0;
    while (--i >= 0) {
      if (getgo(i, offset_within_entry).dualc.highconf()) {
#ifdef USE_CD
        if ((int)(rando() % MINDP) >= ((cd * MINDP) / (CDMAX + 1)))
          getgo(i, offset_within_entry).dualc.decay();
#else
        if ((rando() % 4) == 0)
          getgo(i, offset_within_entry).dualc.decay();
#endif
        if (!getgo(i, offset_within_entry).dualc.veryhighconf())
          mhc++;
      } else {
      
        // TODO Check what to do for these three - allocation should be same for SINGLE_TAG and MULTI_TAG

        #ifndef SINGLE_TAG
        getgo(i, offset_within_entry).tag = p.gtag(hash_pc , i);
        #else // SINGLE_TAG
        getgb(i).tag = p.gtag(hash_fetch_pc, i);
        #endif // SINGLE_TAG
        
  #ifdef DEBUG
  fprintf (stderr, "pc = %llx Allocated entry in bank %d\n", pc, i);
  #endif // DEBUG
  
  #ifndef SINGLE_TAG
  	getgo(i, offset_within_entry).dualc.reset();
  	getgo(i, offset_within_entry).dualc.update(taken);
  #else // SINGLE_TAG
        for (uint32_t offset = 0; offset < INFO_PER_ENTRY; offset++) {
          getgo(i, offset).dualc.reset();
          if (offset == offset_within_entry) {
            getgo(i, offset).dualc.update(taken);
          }
        }
    #endif // SINGLE_TAG
        
        cat += CATR_NUM - mhc * CATR_DEN;
        cat = min(CATMAX, max(0, cat));
#ifdef USE_CD
        cd += CDR_NUM - mhc * CDR_DEN;
        cd = min(CDMAX, max(0, cd));
#endif
        break;
      }
    }
  } // allocate over
  
}

int batage::size() {
  int bimodal_size, table_size, totsize;
  
  bimodal_size = (1 << LOGB) + (BHYSTBITS << LOGB2);
  fprintf (stderr, "Bimodal size = %u bits\n", bimodal_size);
  
  for (int i = 0; i < NUMG; i++)
  {
  
    fprintf (stderr, "table %d, ORIG_ENTRIES_PER_TABLE = %d, LOGG = %d, SS_ENTRIES_PER_TABLE = %d, LOGGE_ORIG = %d, ENTRIES_PER_TABLE = %d, LOGGE = %d\n", i, ORIG_ENTRIES_PER_TABLE(i), LOGG(i), SS_ENTRIES_PER_TABLE(i), LOGGE_ORIG(i), ENTRIES_PER_TABLE(i), LOGGE(i));
    
  #ifndef SINGLE_TAG
   table_size =  (((dualcounter::size()+ TAGBITS) *INFO_PER_ENTRY) * ENTRIES_PER_TABLE(i));
  #else // SINGLE_TAG
  table_size =  (((dualcounter::size()*INFO_PER_ENTRY) + TAGBITS) * ENTRIES_PER_TABLE(i));
  #endif // SINGLE_TAG
  totsize += table_size;
  }
  
  fprintf (stderr, "Total size = %u bits\n", totsize);
  
  #ifdef SINGLE_TAG
    //fprintf (stderr, " NEW_ENTRIES_PER_TABLE = %u \n", NEW_ENTRIES_PER_TABLE(i));
        /*fprintf (stderr, " LOST_ENTRIES_PER_TABLE = %u, LOST_ENTRIES_TOTAL = %u \n", LOST_ENTRIES_PER_TABLE, LOST_ENTRIES_TOTAL);*/
   #endif // SINGLE_TAG
   
    return totsize; // number of bits
  // the storage for counters 'cat', 'meta' and 'cd' is neglected here
}
