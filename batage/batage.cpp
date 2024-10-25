// Author: Pierre Michaud
// Release 1, may 2018

#include <algorithm>
#include <math.h>
#include <stdio.h>
#include <iostream>
#include <stdlib.h>
#include <cassert>
#include "string.h"

#include "batage.hpp"
#include "gshare.hpp"

//#define SIMFASTER

#ifdef SIMFASTER
#define ASSERT(cond)
#else
#define ASSERT(cond)                                                           \
  if (!(cond)) {                                                               \
    fprintf(stderr, "file %s assert line %d\n", __FILE__, __LINE__);           \
    abort();                                                                   \
  }
#endif

/*
// static std::vector<uint8_t> SBP_LOGGE; 
std::vector<uint8_t> SBP_LOGGE;

uint32_t SBP_NUMG;
uint8_t LOG2FETCHWIDTH;
uint8_t NUM_TAKEN_BRANCHES;
std::vector<uint32_t> ORIG_ENTRIES_PER_TABLE;
std::vector<uint32_t> INFO_PER_ENTRY;
uint8_t FETCHWIDTH;
std::vector<uint8_t> SBP_LOGE;
uint8_t LOGFE;  
uint8_t SBP_LOGBE; 
uint8_t SBP_LOGB2E; 
std::vector<uint8_t>  SBP_LOGG;
std::vector<uint32_t> SS_ENTRIES_PER_TABLE; 
std::vector<uint8_t>  LOGGE_ORIG;

std::vector<uint32_t> NEW_ENTRIES_PER_TABLE;
std::vector<uint32_t> ENTRIES_PER_TABLE;
*/

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
  ASSERT(SBP_BHYSTBITS >= 1);
  n[b1 ^ 1] = (1 << (SBP_BHYSTBITS - 1)) - 1;
  n[b1] = n[b1 ^ 1] + (1 << SBP_BHYSTBITS) - b2;
}

dualcounter::dualcounter() { reset(); }

bool dualcounter::is_counter_reset ()
{
	return (  (n[0] == 0) && (n[1] == 0) );
}
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

void SBP_folded_history::init(int original_length, int compressed_length,
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

uint32_t SBP_folded_history::rotateleft(uint32_t x, int m) {
  ASSERT(m < clength);
  ASSERT((x >> clength) == 0);
  uint32_t y = x >> (clength - m);
  x = (x << m) | y;
  return x & mask1;
}

void SBP_folded_history::update(path_history &ph) {
  fold = rotateleft(fold, 1);
  unsigned inbits = ph[0] & mask2;
  unsigned outbits = ph[olength] & mask2;
  outbits = rotateleft(outbits, outpoint);
  fold ^= inbits ^ outbits;
}

void histories::get_predictor_vars(const batage* bp)
{
	SBP_NUMG = bp->SBP_NUMG;
	SBP_LOGGE = bp->SBP_LOGGE;
	
	 int *hist = new int[SBP_NUMG];
  int prevh = 0;
  
   for (int i = 0; i < SBP_NUMG; i++) {
    // geometric history lengths
    int h = SBP_MINHIST * pow((double)SBP_MAXHIST / SBP_MINHIST, (double)i / (SBP_NUMG - 1));
    h = max(prevh + 1, h);
    hist[SBP_NUMG - 1 - i] = h; // hist[0] = longest history
    prevh = h;
  }
  
  bh.init(SBP_MAXHIST + 1);
  ph.init(SBP_MAXPATH + 1);
  chg = new SBP_folded_history[SBP_NUMG];
  chgg = new SBP_folded_history[SBP_NUMG];
  cht = new SBP_folded_history[SBP_NUMG];
  chtt = new SBP_folded_history[SBP_NUMG];
  
  for (int i = 0; i < SBP_NUMG; i++) {
    chg[i].init(hist[i], SBP_LOGGE[i], 1);
    cht[i].init(hist[i], SBP_TAGBITS, 1);
    int hashparam = 1;
    if (SBP_LOGGE[i] == SBP_TAGBITS) {
      hashparam = (lcm(SBP_LOGGE[i], SBP_LOGGE[i] - 3) > lcm(SBP_LOGGE[i], SBP_LOGGE[i] - 2)) ? 3 : 2;
    }
    if (hist[i] <= SBP_MAXPATH) {
      chgg[i].init(hist[i], SBP_LOGGE[i] - hashparam, SBP_PATHBITS);
      chtt[i].init(hist[i], SBP_TAGBITS - 1, SBP_PATHBITS);
    } else {
      chgg[i].init(hist[i], SBP_LOGGE[i] - hashparam, 1);
      chtt[i].init(hist[i], SBP_TAGBITS - 1, 1);
    }
  }
  
}

histories::histories(const batage* bp) : SBP_NUMG{ bp->SBP_NUMG }, SBP_LOGGE{ bp->SBP_LOGGE }
{

	/*int SBP_NUMG = bp->SBP_NUMG;
	const auto &SBP_LOGGE = bp->SBP_LOGGE;*/
/*
  int *hist = new int[SBP_NUMG];
  int prevh = 0;
  for (int i = 0; i < SBP_NUMG; i++) {
    // geometric history lengths
    int h = SBP_MINHIST * pow((double)SBP_MAXHIST / SBP_MINHIST, (double)i / (SBP_NUMG - 1));
    h = max(prevh + 1, h);
    hist[SBP_NUMG - 1 - i] = h; // hist[0] = longest history
    prevh = h;
  }
  bh.init(SBP_MAXHIST + 1);
  ph.init(SBP_MAXPATH + 1);
  chg = new SBP_folded_history[SBP_NUMG];
  chgg = new SBP_folded_history[SBP_NUMG];
  cht = new SBP_folded_history[SBP_NUMG];
  chtt = new SBP_folded_history[SBP_NUMG];
  for (int i = 0; i < SBP_NUMG; i++) {
    chg[i].init(hist[i], SBP_LOGGE[i], 1);
    cht[i].init(hist[i], SBP_TAGBITS, 1);
    int hashparam = 1;
    if (SBP_LOGGE[i] == SBP_TAGBITS) {
      hashparam = (lcm(SBP_LOGGE[i], SBP_LOGGE[i] - 3) > lcm(SBP_LOGGE[i], SBP_LOGGE[i] - 2)) ? 3 : 2;
    }
    if (hist[i] <= SBP_MAXPATH) {
      chgg[i].init(hist[i], SBP_LOGGE[i] - hashparam, SBP_PATHBITS);
      chtt[i].init(hist[i], SBP_TAGBITS - 1, SBP_PATHBITS);
    } else {
      chgg[i].init(hist[i], SBP_LOGGE[i] - hashparam, 1);
      chtt[i].init(hist[i], SBP_TAGBITS - 1, 1);
    }
  }*/
}

//void histories::update(uint32_t targetpc, bool taken, const batage* bp) {
void histories::update(uint32_t targetpc, bool taken) {
//int SBP_NUMG = bp->SBP_NUMG;

#ifdef SBP_PC_SHIFT
  // targetpc ^= targetpc << 5;
  targetpc ^= targetpc >> SBP_PC_SHIFT;
#endif
  bh.insert(taken);
  ph.insert(targetpc);
  for (int i = 0; i < SBP_NUMG; i++) {
    chg[i].update(bh);
    cht[i].update(bh);
    if (chgg[i].olength <= SBP_MAXPATH) {
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

int histories::gindex(uint32_t pc, int i, const batage* bp) const {

	//int SBP_NUMG = bp->SBP_NUMG;
	//const auto &SBP_LOGGE = bp->SBP_LOGGE;
	const auto &ENTRIES_PER_TABLE = bp->ENTRIES_PER_TABLE;
	
  ASSERT((i >= 0) && (i < SBP_NUMG));
  ASSERT (SBP_LOGGE.size());
#ifdef DEBUG_GINDEX
//fprintf (stderr, "gindex, %lx, %d \n", pc, i);
#endif
  uint32_t hash = pc ^ i ^ chg[i].fold ^ (chgg[i].fold << (chg[i].clength - chgg[i].clength));
   int raw_index = hash & ((1 << (SBP_LOGGE[i])) - 1);
   int rolled_index = (raw_index % ENTRIES_PER_TABLE[i]);
#ifdef DEBUG_GINDEX
fprintf (stderr, "ENTRIES_PER_TABLE = %d, rolled_index = %d \n", ENTRIES_PER_TABLE[i], rolled_index);
#endif
   return rolled_index;
}

//#define DEBUG_GSHARE_INDEX
int histories::gshare_index(uint32_t pc, int i, const batage* bp) const {

	//const auto &ENTRIES_PER_TABLE = bp->ENTRIES_PER_TABLE;
	
  ASSERT((i >= 0) && (i < SBP_NUMG));
  //ASSERT (SBP_LOGGE.size());
	int j = i >> 2;
  uint32_t hash = pc ^  (pc << j) ^ (pc >> j) ^ chg[i].fold ^ (chgg[i].fold << (chg[i].clength - chgg[i].clength));
   int raw_index = hash & ((1 << ((int)log2(NUM_GSHARE_ENTRIES))) - 1);
     #ifdef DEBUG_GSHARE_INDEX
fprintf (stderr, "hash = %x and raw_index = %x \n", hash, raw_index);
#endif

   return raw_index;
}

//int histories::gtag(uint32_t pc, int i, const batage* bp) const {
int histories::gtag(uint32_t pc, int i) const {

	//int SBP_NUMG = bp->SBP_NUMG;
	
#ifdef DEBUG 
//fprintf (stderr, "gtag, %d, %d \n", pc, i);
#endif

  ASSERT((i >= 0) && (i < SBP_NUMG));
  uint32_t hash = (pc + i) ^ reverse(cht[i].fold, cht[i].clength) ^
                  (chtt[i].fold << (cht[i].clength - chtt[i].clength));
#ifdef GHGBITS
  // when bank interleaving is enabled, introducing 1 or 2 bits in the tag
  // for identifying the path length generally reduces tag aliasing when SBP_NUMG is
  // large
  hash = (hash << GHGBITS) | ghg(i);
#endif
  return hash & ((1 << SBP_TAGBITS) - 1);
}

#ifdef BANK_INTERLEAVING
// inspired from Seznec's TAGE-SC-L (CBP 2016), but different:
// interleaving is global, unlike in TAGE-SC-L ==> unique tag size
//int histories::phybank(int i, const batage* bp) const {
int histories::phybank(int i) const {

	//int SBP_NUMG = bp->SBP_NUMG;
	
  ASSERT((i >= 0) && (i < SBP_NUMG));
  unsigned pos;
  if (i >= (SBP_NUMG - MIDBANK)) {
    pos = i;
  } else {
    // on some workloads, the shortest non-null global history length does not
    // generate enough entropy, which may lead to uneven utilization of banks,
    // hence MIDBANK
    pos = (chgg[SBP_NUMG - MIDBANK - 1].fold + i) % (SBP_NUMG - MIDBANK);
  }
  return (chgg[SBP_NUMG - 1].fold + pos) % SBP_NUMG;
}

#ifdef GHGBITS
//int histories::ghg(int i, const batage* bp) const 
int histories::ghg(int i) const 
{ 	//int SBP_NUMG = bp->SBP_NUMG; 
return ((SBP_NUMG - 1 - i) << GHGBITS) / SBP_NUMG; }
#endif

#endif // BANK_INTERLEAVING

//void histories::printconfig(const batage* bp) {
void histories::printconfig() {

	//int SBP_NUMG = bp->SBP_NUMG; 
	
  printf("history lengths: ");
  for (int i = SBP_NUMG - 1; i >= 0; i--) {
    printf("%d ", chg[i].olength);
  }
  printf("\n");
}

int histories::size() {
  return SBP_MAXHIST + SBP_MAXPATH * SBP_PATHBITS; // number of bits
  // The storage for folded histories is ignored here
  // If folded histories are implemented in hardware, they must be checkpointed
  // (cf. Schlais & Lipasti, ICCD 2016) An implementation without folded
  // histories is possible (cf. Seznec's GEHL, ISCA 2005)
}

tagged_entry::tagged_entry() {
  tag = 0;
  #ifdef POS
  pos = -1;
  #endif // POS
  #ifdef NOT_MRU
  mru = -1;
  #endif
  dualc.reset();
}

/*
prediction::prediction()
{
	prediction_vector.reserve();
	highconf.reserve();
}
*/

//uint32_t abcd, xyz;


void batage::read_env_variables()
{
	char *temp; // [32]; 
	
	/*temp = getenv("temp");
	//strcpy(temp, getenv("temp"));
	abcd = atoi(temp);
	fprintf(stderr, "abcd = %u\n", abcd);
	temp = getenv("temp1");
	//strcpy(temp, getenv("temp"));
	xyz = atoi(temp);
	fprintf(stderr, "xyz = %u\n", xyz);*/
	
	temp = getenv("SBP_NUMG");
	SBP_NUMG = atoi(temp);
	fprintf(stderr, "SBP_NUMG = %d\n", SBP_NUMG);
	
	temp = getenv("LOG2FETCHWIDTH");
	LOG2FETCHWIDTH = atoi(temp);
	fprintf(stderr, "LOG2FETCHWIDTH = %d\n", LOG2FETCHWIDTH);
	
	temp = getenv("NUM_TAKEN_BRANCHES");
	NUM_TAKEN_BRANCHES = atoi(temp);
	
	ORIG_ENTRIES_PER_TABLE.resize(SBP_NUMG);
	INFO_PER_ENTRY.resize(SBP_NUMG);
	
	temp = getenv("ORIG_ENTRIES_PER_TABLE_00");
	ORIG_ENTRIES_PER_TABLE[0] = atoi(temp);
	temp = getenv("ORIG_ENTRIES_PER_TABLE_01");
	ORIG_ENTRIES_PER_TABLE[1] = atoi(temp);
	temp = getenv("ORIG_ENTRIES_PER_TABLE_02");
	ORIG_ENTRIES_PER_TABLE[2] = atoi(temp);
	temp = getenv("ORIG_ENTRIES_PER_TABLE_03");
	ORIG_ENTRIES_PER_TABLE[3] = atoi(temp);
	temp = getenv("ORIG_ENTRIES_PER_TABLE_04");
	ORIG_ENTRIES_PER_TABLE[4] = atoi(temp);
	temp = getenv("ORIG_ENTRIES_PER_TABLE_05");
	ORIG_ENTRIES_PER_TABLE[5] = atoi(temp);
	temp = getenv("ORIG_ENTRIES_PER_TABLE_06");
	ORIG_ENTRIES_PER_TABLE[6] = atoi(temp);
	temp = getenv("ORIG_ENTRIES_PER_TABLE_07");
	ORIG_ENTRIES_PER_TABLE[7] = atoi(temp);
	temp = getenv("ORIG_ENTRIES_PER_TABLE_08");
	ORIG_ENTRIES_PER_TABLE[8] = atoi(temp);
	temp = getenv("ORIG_ENTRIES_PER_TABLE_09");
	ORIG_ENTRIES_PER_TABLE[9] = atoi(temp);
	temp = getenv("ORIG_ENTRIES_PER_TABLE_10");
	ORIG_ENTRIES_PER_TABLE[10] = atoi(temp);
	temp = getenv("ORIG_ENTRIES_PER_TABLE_11");
	ORIG_ENTRIES_PER_TABLE[11] = atoi(temp);
		
	temp = getenv("INFO_PER_ENTRY_00");
	INFO_PER_ENTRY[0] = atoi(temp);
	temp = getenv("INFO_PER_ENTRY_01");
	INFO_PER_ENTRY[1] = atoi(temp);
	temp = getenv("INFO_PER_ENTRY_02");
	INFO_PER_ENTRY[2] = atoi(temp);
	temp = getenv("INFO_PER_ENTRY_03");
	INFO_PER_ENTRY[3] = atoi(temp);
	temp = getenv("INFO_PER_ENTRY_04");
	INFO_PER_ENTRY[4] = atoi(temp);
	temp = getenv("INFO_PER_ENTRY_05");
	INFO_PER_ENTRY[5] = atoi(temp);
	temp = getenv("INFO_PER_ENTRY_06");
	INFO_PER_ENTRY[6] = atoi(temp);
	temp = getenv("INFO_PER_ENTRY_07");
	INFO_PER_ENTRY[7] = atoi(temp);
	temp = getenv("INFO_PER_ENTRY_08");
	INFO_PER_ENTRY[8] = atoi(temp);
	temp = getenv("INFO_PER_ENTRY_09");
	INFO_PER_ENTRY[9] = atoi(temp);
	temp = getenv("INFO_PER_ENTRY_10");
	INFO_PER_ENTRY[10] = atoi(temp);
	temp = getenv("INFO_PER_ENTRY_11");
	INFO_PER_ENTRY[11] = atoi(temp);
}



void batage::populate_dependent_globals()
{
	FETCHWIDTH = (1 << LOG2FETCHWIDTH);
	fprintf(stderr, "LOG2FETCHWIDTH = %d\n", LOG2FETCHWIDTH);
	fprintf(stderr, "BATAGE:FETCHWIDTH = %d\n", FETCHWIDTH);
	
	SBP_LOGE.resize(SBP_NUMG);
	SBP_LOGG.resize(SBP_NUMG);
	LOGGE_ORIG.resize(SBP_NUMG);
	SS_ENTRIES_PER_TABLE.resize(SBP_NUMG);
	NEW_ENTRIES_PER_TABLE.resize(SBP_NUMG);
	SBP_LOGGE.resize(SBP_NUMG);
	ENTRIES_PER_TABLE.resize(SBP_NUMG);
	for (int i = 0; i < SBP_NUMG; i++)
	{
		SBP_LOGE[i] = ((int)log2(INFO_PER_ENTRY[i]));
		SBP_LOGG[i] = (int)ceil(log2(ORIG_ENTRIES_PER_TABLE[i]));
		LOGGE_ORIG[i] = (SBP_LOGG[i] - SBP_LOGE[i]);
		SS_ENTRIES_PER_TABLE[i] = (ORIG_ENTRIES_PER_TABLE[i] / INFO_PER_ENTRY[i]);
		#ifndef SINGLE_TAG
		NEW_ENTRIES_PER_TABLE[i] = 0;
		SBP_LOGGE[i] =  (LOGGE_ORIG[i]) ;
		ENTRIES_PER_TABLE[i] = (SS_ENTRIES_PER_TABLE[i] + NEW_ENTRIES_PER_TABLE[i]);
		#endif // SINGLE_TAG
	}
	
	LOGFE = ((int)log2(FETCHWIDTH));
	SBP_LOGBE = (SBP_LOGB - LOGFE);
	SBP_LOGB2E = (SBP_LOGB2 - LOGFE);
}

void batage::batage_resize()
{
g = new tagged_entry **[SBP_NUMG];
  
  for (int i = 0; i < SBP_NUMG; i++) {
#ifdef DEBUG_BATAGE
fprintf (stderr, "ENTRIES_PER_TABLE(%d) = %d and INFO_PER_ENTRY(%d) = %d \n", i, ENTRIES_PER_TABLE[i], i, INFO_PER_ENTRY[i]);
#endif // DEBUG_BATAGE
    g[i] = new tagged_entry *[ENTRIES_PER_TABLE[i]];
    for (int j = 0; j < (ENTRIES_PER_TABLE[i]); j++) {
      g[i][j] = new tagged_entry[INFO_PER_ENTRY[i]];
    }
  }
  
  gi = new int[SBP_NUMG];

  b.resize(1 << SBP_LOGBE);
  for (int i = 0; i < (1 << SBP_LOGBE); i++) {
  b[i].resize(FETCHWIDTH);
    for (int j = 0; j < FETCHWIDTH; j++) {
      b[i][j] = 0; // not-taken prediction
    }
  }
  
    b2.resize(1 << SBP_LOGB2E);
  for (int i = 0; i < (1 << SBP_LOGB2E); i++) {
  b2[i].resize(FETCHWIDTH);
    for (int j = 0; j < FETCHWIDTH; j++) {
      b2[i][j] = (1 << SBP_BHYSTBITS) - 1; // weak state
    }
  }
  
  #ifdef BANK_INTERLEAVING
  bank = new int[SBP_NUMG];
  check = new bool[SBP_NUMG];
#endif

  hit.resize(FETCHWIDTH);
s.resize(FETCHWIDTH);
allocs.resize(SBP_NUMG);

tags.resize(FETCHWIDTH);
#if (defined (POS) || defined (MT_PLUS))
poses.resize(FETCHWIDTH);
random = 0x05af5a0f ^ 0x5f0aa05f;
#endif // POS
  
}

batage::batage() {

	//read_env_variables();
	//populate_dependent_globals();
	fprintf(stderr, "%s\n", "BATAGE Constructed\n");

  cat = 0;
  meta = -1;
#ifdef USE_META
  meta = 0;
#endif
#ifdef USE_CD
  cd = 0;
#endif

#ifdef DEBUG
  predict_pcs = fopen("predict_pcs.txt", "w+");
  update_pcs = fopen("update_pcs.txt", "w+");
#endif // DEBUG

}

#ifdef BANK_INTERLEAVING
void batage::check_bank_conflicts() {
  for (int i = 0; i < SBP_NUMG; i++) {
    check[i] = false;
  }
  for (int i = 0; i < SBP_NUMG; i++) {
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
  ASSERT((i >= 0) && (i < SBP_NUMG));
#ifdef BANK_INTERLEAVING
  ASSERT((bank[i] >= 0) && (bank[i] < SBP_NUMG));
  return g[bank[i]][gi[bank[i] ]][0];
#else
  return g[i][gi[i]][0];
#endif
}

// i is the bank number - not the index within bank
// Index is taken from gi, must be saved from prediction to update
tagged_entry &batage::getgp(int i, uint32_t offset_within_packet) {

#ifdef DEBUG 
//fprintf (stderr, "getgo, %d, %d \n", i, offset_within_entry);
#endif

  ASSERT((i >= 0) && (i < SBP_NUMG));
#ifdef BANK_INTERLEAVING
  ASSERT((bank[i] >= 0) && (bank[i] < SBP_NUMG));
uint32_t offset_within_entry = get_offset_within_entry (offset_within_packet, bank[i]);
assert( ((offset_within_entry >= 0) && (offset_within_entry< INFO_PER_ENTRY(bank[i]))));
  return g[bank[i]][gi[bank[i]]][offset_within_entry];
#else
uint32_t offset_within_entry = get_offset_within_entry (offset_within_packet, i);
 assert( ((offset_within_entry >= 0) && (offset_within_entry< INFO_PER_ENTRY[i])));
  return g[i][gi[i]][offset_within_entry];
#endif
}


void batage::fetchBoundaryBegin(uint64_t PC)
{
	return;
}

void batage::fetchBoundaryEnd()
{
	return;
}

// i is the bank number - not the index within bank
// Index is taken from gi, must be saved from prediction to update
tagged_entry &batage::getge(int i, uint32_t offset_within_entry) {

#ifdef DEBUG 
//fprintf (stderr, "getgo, %d, %d \n", i, offset_within_entry);
#endif

  ASSERT((i >= 0) && (i < SBP_NUMG));
/*#ifdef BANK_INTERLEAVING
  ASSERT((bank[i] >= 0) && (bank[i] < SBP_NUMG));
  assert( ((offset_within_entry_banki >= 0) && (offset_within_entry_banki< INFO_PER_ENTRY(bank[i]))));
  return g[bank[i]][gi[bank[i] ]][offset_within_entry_banki];
#else
*/
// Works with BANK_INTERLEAVING, this particulaar function already sends both the bank and index based on bank interleaving, so no need to do bank[i]
 assert( ((offset_within_entry >= 0) && (offset_within_entry< INFO_PER_ENTRY[i])));
  return g[i][gi[i]][offset_within_entry];
//#endif
}

uint32_t batage::get_offset_within_entry (uint32_t offset_within_packet, int table)
{
	/*#ifdef MT_PLUS
	fprintf (stderr, "This should not be happening\n");
	return 0;
	*/
	// TODO Check if this should be used for POS also
	#ifdef XIANGSHAN
		return ( offset_within_packet % INFO_PER_ENTRY[table]  );
	#else
		return ( offset_within_packet / (FETCHWIDTH / INFO_PER_ENTRY[table]) );
	#endif
}

/*
To add POS - with single tag - pos to offset_within_entry mapping is not constantand may chnage, so each subentry needs to be checked
for prediction, match pos in addition to tag, if matched, then consider it a match and proceed as usual
TODO - Check storing POS in FTQ is a good design choice - else rematch pos everytime
*/
prediction& batage::predict_vec(uint32_t fetch_pc, const histories &p) {

#ifdef DEBUG
  //fprintf(predict_pcs, "%lu \n", fetch_pc);
  fprintf (stderr, "predict for fetch_pc = %llx\n", fetch_pc);
#endif // DEBUG

#ifdef SBP_PC_SHIFT
uint32_t hash_fetch_pc = fetch_pc ^ (fetch_pc >> SBP_PC_SHIFT);
#ifndef SINGLE_TAG
    vector <uint32_t> hash_pc;
  uint32_t pc;
  for ( int i = 0;  i < FETCHWIDTH;  i++)
  {
  	pc = fetch_pc + (i<<1);
  	hash_pc.push_back(pc ^ (pc >> SBP_PC_SHIFT));
  }
#else
  uint32_t hash_pc = fetch_pc ^ (fetch_pc >> SBP_PC_SHIFT);
#endif // SINGLE_TAG
#endif // SBP_PC_SHIFT

  uint32_t offset_within_entry, offset_within_packet; // = hash_pc % INFO_PER_ENTRY;
  
#ifdef DEBUG
  std::cerr << "00000" << "\n";
#endif
  
  for (offset_within_packet = 0; offset_within_packet < FETCHWIDTH; offset_within_packet++)
  {
  	hit[offset_within_packet].clear();
  	s[offset_within_packet].clear();
  	tags[offset_within_packet].clear();
#if (defined (POS) || defined (MT_PLUS))
  	poses[offset_within_packet].clear();
#endif
  }
  
for (offset_within_packet = 0; offset_within_packet < FETCHWIDTH; offset_within_packet++)
   {
     	for (int i = 0; i < SBP_NUMG; i++) {
     	  	
		#ifdef BANK_INTERLEAVING
    		bank[i] = p.phybank(i);
		#ifdef DEBUG
		//fprintf (stderr, "For predict, bank[%d] = %d \n ", i, bank[i]);
		#endif // DEBUG
		#endif
    		gi[i] = p.gindex(hash_fetch_pc, i, this);
    		
		//TODO - Try different i for gshare index
    		/*if ( (offset_within_packet == 0) && (i == 3) )
    		{
    			pred_out.gshare_index = gi[i]; 
    		}*/
		#ifdef DEBUG
		std::cerr << "11111" << "\n";
		//fprintf (stderr, "For predict, gi[%d] = %d \n ", i, gi[i]);
		#endif // DEBUG
   
   		int tag;
#ifdef MT_PLUS
    		int j = 0;
    		for (j = 0; j < INFO_PER_ENTRY[i]; j++)
    		{
			tag = getge(i, j).tag;
    			if (tag == p.gtag ( hash_pc[offset_within_packet], i))
    			{
    				#ifdef DEBUG
				fprintf (stderr, "For offset_within_packet = %d, bank = %d hit at subentry = %d\n", offset_within_packet, i, j);
				#endif //DEBUG
     		 		hit[offset_within_packet].push_back(i);
				s[offset_within_packet].push_back(getge(i, j).dualc);
				poses[offset_within_packet].push_back(j);
				tags[offset_within_packet].push_back(tag);
				break;
    			}
    		}
 #else // MT_PLUS
     #ifndef SINGLE_TAG
     		tag = getgp(i, offset_within_packet).tag;
   		if ( tag == p.gtag(hash_pc[offset_within_packet], i))
    #else
    		tag = getgb(i).tag;
    	        if ( tag == p.gtag(hash_fetch_pc, i)) 
    #endif
    		{
			#ifndef POS
			#ifdef DEBUG
			fprintf (stderr, "bank %d hit for offset %d\n", i, offset_within_packet);
			#endif //DEBUG
     		 	hit[offset_within_packet].push_back(i);
			s[offset_within_packet].push_back(getgp(i, offset_within_packet).dualc);
			tags[offset_within_packet].push_back(tag);
			#else // POS
				int j;
				for (j =0; j < INFO_PER_ENTRY[i]; j++)
				{
					if (getge(i, j).pos == offset_within_packet)
					{
						#ifdef DEBUG
						fprintf (stderr, "bank %d hit for offset %d\n", i, offset_within_packet);
						#endif //DEBUG
						
						hit[offset_within_packet].push_back(i);
						s[offset_within_packet].push_back(getge(i, j).dualc);
						poses[offset_within_packet].push_back(j);
						tags[offset_within_packet].push_back(tag);
						break;
					}
				}
			#endif // POS	
      		}
#endif // MT_PLUS
      		
     #ifdef DEBUG
     std::cerr << "22222" << "\n";	
     #endif // DEBUG
      	}
    }
    
     #ifdef DEBUG_POS
     for (int i = 0; i < FETCHWIDTH; i++)
     {
     
     	if (poses[i].size() == 0)
	{
		std::cerr << "After prediction, poses [" << i << "] = empty \n";
	}
	else
	{
  		std::cerr << "After prediction, poses [" << i << "] = \n";
      		for (int j = 0; j < poses[i].size(); j++)
      		{
      			std::cerr << poses[i][j];
      		}
      		std::cerr << "\n";
  	}
     }	
     #endif // DEBUG_POS
    
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

  bi = hash_fetch_pc & ((1 << SBP_LOGBE) - 1);
  bi2 = bi & ((1 << SBP_LOGB2E) - 1);
  b_bi.clear();
  b2_bi2.clear();

  for (offset_within_packet = 0; offset_within_packet < FETCHWIDTH; offset_within_packet++)
  {
  		b_bi.push_back (b[bi][offset_within_packet]);
  		b2_bi2.push_back(b2[bi2][offset_within_packet]);
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
  pred_out.prediction_vector.clear();
  pred_out.highconf.clear();
  bool gshare_tag_saved = false;
  bool i_pred = false;
  bool i_highconf = false;
  
  for (offset_within_packet = 0; offset_within_packet < FETCHWIDTH; offset_within_packet++)
  {
     	//predict[offset_within_entry] = s[offset_within_entry][bp].pred();
   	i_pred = s[offset_within_packet][bp[offset_within_packet]].pred();
   	i_highconf = (s[offset_within_packet][bp[offset_within_packet]].veryhighconf()) /* || (s[offset_within_packet][bp[offset_within_packet]].highconf()) */;
   	pred_out.prediction_vector.push_back(i_pred);
   	pred_out.highconf.push_back(i_highconf);   
   	if ( (i_pred) && (i_highconf) && !gshare_tag_saved )
   	{
   		// TODO - Must be accounting for Bimodal, but that hits number of gshare predictions
   		if (hit.empty()) // Bimodal
   		{pred_out.gshare_tag = hash_fetch_pc;}
   		else
   		{pred_out.gshare_tag = tags[offset_within_packet][bp[offset_within_packet]]; } // gi[bp[offset_within_packet]];
   		gshare_tag_saved = true;
   	}
}
//pred_out.gshare_index = pred_out.gshare_tag;
pred_out.gshare_index = p.gshare_index(hash_fetch_pc, 7, this);

  #ifdef DEBUG
	std::cerr << "66666" << "\n";
	#endif // DEBUG
	// TODO Temporarily Diabled
  	//fprintf(stderr, "ba.predict pc=%llx predict:%d\n", pc, predict);

  return pred_out;
}

void batage::update_bimodal(bool taken, uint32_t offset_within_packet) {
  // see Loh et al., "Exploiting bias in the hysteresis bit of 2-bit saturating
  // counters in branch predictors", Journal of ILP, 2003
  if (b[bi][offset_within_packet] == taken) {
    if (b2[bi2][offset_within_packet] > 0)
      b2[bi2][offset_within_packet]--;
  } else {
    if (b2[bi2][offset_within_packet] < ((1 << SBP_BHYSTBITS) - 1)) {
      b2[bi2][offset_within_packet]++;
    } else {
      b[bi][offset_within_packet] = taken;
    }
  }
}

// i - index within s
void batage::update_entry_p(int i, uint32_t offset_within_packet, bool taken) {
  
  ASSERT(i < s[offset_within_packet].size());
     
  if (i < (int)hit[offset_within_packet].size()) {
    getgp(hit[offset_within_packet][i], offset_within_packet).dualc.update(taken);
  } else {
    update_bimodal(taken, offset_within_packet);
  }
}

// i - index within s
void batage::update_entry_e(int i, uint32_t offset_within_packet, uint32_t offset_within_entry, bool taken) {
  
  ASSERT(i < s[offset_within_packet].size());

  if (i < (int)hit[offset_within_packet].size()) {
    getge(hit[offset_within_packet][i], offset_within_entry).dualc.update(taken);
#ifdef NOT_MRU
if ( getge(hit[offset_within_packet][i], offset_within_entry).dualc.pred() == taken )
{
	getgb(hit[offset_within_packet][i]).mru = offset_within_entry;
}
#endif
  } else {
    update_bimodal(taken, offset_within_packet);
  }
}

uint32_t batage::get_allocs(int table)
{
	return (allocs[table]);
}

/*For superscalar - anything that is non constant & is used before/ without
 assigning a value must be saved in ftq*/
 /*
To add POS - with single tag - pos to offset_within_entry mapping is not constantand may change, so each subentry needs to be checked
For update, pos must either be matched again or taken from FTQ, if matched, then consider it a match and proceed as usual for update
For allocation - pos must be populated with offset_within_packet
TODO - Check storing POS in FTQ is a good design choice - else rematch pos everytime
 */
void batage::update(uint32_t pc, uint32_t fetch_pc, uint32_t offset_within_packet, bool taken, const histories &p,
                    bool noalloc = false) {

#ifdef DEBUG
  //fprintf(update_pcs, "%lu \n", pc);
    fprintf (stderr, "update for pc = %llx, fetch_pc = %llx\n", pc, fetch_pc);
#endif // DEBUG
     #ifdef DEBUG_POS
      	std::cerr << "Before update, poses [" << offset_within_packet << "] = \n";
      	for (int j = 0; j < poses[offset_within_packet].size(); j++)
      	{
      		std::cerr << poses[offset_within_packet][j];
      	}
      	std::cerr << "\n";
#endif // DEBUG_POS

#ifdef SBP_PC_SHIFT
  uint32_t hash_fetch_pc = fetch_pc ^ (fetch_pc >> SBP_PC_SHIFT);
  uint32_t hash_pc = pc ^ (pc >> SBP_PC_SHIFT);
#endif

  #ifdef DEBUG
  //fprintf (stderr, "offset = %d, bp = %d, b_bi = %d, b2_bi2 = %d\n", offset_within_entry, bp[offset_within_entry], b_bi[offset_within_entry], b2_bi2[offset_within_entry]);
  #endif // DEBUG
  
  // TODO - Check, might/ should not be necessary
  b[bi][offset_within_packet] = b_bi[offset_within_packet];
  b2[bi2][offset_within_packet] = b2_bi2[offset_within_packet];

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
  /*for (int i = 0; i < SBP_NUMG; i++) {
fprintf (stderr, "For update, bank[%d] = %d \n ", i, bank[i]);
}*/
#endif // DEBUG
#endif
#ifdef DEBUG
/*  for (int i = 0; i < SBP_NUMG; i++) {
fprintf (stderr, "For update, gi[%d] = %d \n ", i, gi[i]);
}*/
#endif // DEBUG
uint32_t offset_within_entry ;

/*
For update - for index i into the hit[offset_within_packet] vector
table # = hit[offset_within_packet][i]
offset_within_entry = poses[offset_within_packet][i]
*/
  // update from 0 to bp-1
  #if (defined (POS) || defined (MT_PLUS))
	for (int i = 0; i < bp[offset_within_packet]; i++) {

		if ((meta >= 0) || s[offset_within_packet][i].lowconf() || 		(s[offset_within_packet][i].pred() != s[offset_within_packet][bp[offset_within_packet]].pred()) ||
        ((rando() % 8) == 0)) {
        offset_within_entry = poses[offset_within_packet][i];  
      getge(hit[offset_within_packet][i], offset_within_entry).dualc.update(taken);
    		}
    		
#ifdef NOT_MRU
		//if (s[offset_within_packet][i].pred() == s[offset_within_packet][bp[offset_within_packet]].pred())
		if (s[offset_within_packet][i].pred() == taken)
		{
			offset_within_entry = poses[offset_within_packet][i];  
			getgb(hit[offset_within_packet][i]).mru = offset_within_entry;
		}
#endif // NOT_MRU
 	 }
#else // POS
  for (int i = 0; i < bp[offset_within_packet]; i++) {
  //offset_within_entry = get_offset_within_entry (offset_within_packet, i);
    if ((meta >= 0) || s[offset_within_packet][i].lowconf() || (s[offset_within_packet][i].pred() != s[offset_within_packet][bp[offset_within_packet]].pred()) ||
        ((rando() % 8) == 0)) {
      getgp(hit[offset_within_packet][i], offset_within_packet).dualc.update(taken);
    }
  }
#endif
  
  // update at bp
#if (defined (POS) || defined (MT_PLUS))

        #ifdef DEBUG_POS
      	std::cerr << "00000 \n";
        #endif // DEBUG_POS
    if ((bp[offset_within_packet] < (int)hit[offset_within_packet].size()) && s[offset_within_packet][bp[offset_within_packet]].highconf() && s[offset_within_packet][bp[offset_within_packet] + 1].highconf() &&
      (s[offset_within_packet][bp[offset_within_packet] + 1].pred() == taken) &&
      ((s[offset_within_packet][bp[offset_within_packet]].pred() == taken) || (cat >= (CATMAX / 2)))) {
    if (!s[offset_within_packet][bp[offset_within_packet]].saturated() || ((meta < 0) && ((rando() % 8) == 0))) {
         #ifdef DEBUG_POS
      	std::cerr << "11111 \n";
        #endif // DEBUG_POS
        offset_within_entry = poses[offset_within_packet][bp[offset_within_packet]];
      getge(hit[offset_within_packet][bp[offset_within_packet]], offset_within_entry).dualc.decay();
    }
  } else {
           #ifdef DEBUG_POS
      	std::cerr << "22222 \n";
        #endif // DEBUG_POS
// TODO Check
if  ((int)hit[offset_within_packet].size() > 0 ) 
    {
    offset_within_entry = poses[offset_within_packet][bp[offset_within_packet]];
    }
   else // no hit, bimodal, this is not used since bimodal uses offset_within_packet directly in update_entry_e
   {
   	offset_within_entry = offset_within_packet;
   }
    update_entry_e(bp[offset_within_packet], offset_within_packet, offset_within_entry, taken);
               #ifdef DEBUG_POS
      	std::cerr << "33333 \n";
        #endif // DEBUG_POS
  }
#else  // POS
  // offset_within_entry = get_offset_within_entry ( offset_within_packet,  bp[offset_within_packet]); 
  if ((bp[offset_within_packet] < (int)hit[offset_within_packet].size()) && s[offset_within_packet][bp[offset_within_packet]].highconf() && s[offset_within_packet][bp[offset_within_packet] + 1].highconf() &&
      (s[offset_within_packet][bp[offset_within_packet] + 1].pred() == taken) &&
      ((s[offset_within_packet][bp[offset_within_packet]].pred() == taken) || (cat >= (CATMAX / 2)))) {
    if (!s[offset_within_packet][bp[offset_within_packet]].saturated() || ((meta < 0) && ((rando() % 8) == 0))) {
      getgp(hit[offset_within_packet][bp[offset_within_packet]], offset_within_packet).dualc.decay();
    }
  } else {
    update_entry_p(bp[offset_within_packet], offset_within_packet, taken);
  }
  #endif //POS
  
  // update at bp+1
#if (defined (POS) || defined (MT_PLUS))

        #ifdef DEBUG_POS
      	std::cerr << "44444 \n";
        #endif // DEBUG_POS

  if (!s[offset_within_packet][bp[offset_within_packet]].highconf() && (bp[offset_within_packet] < (int)hit[offset_within_packet].size())) {
 if  ((int)hit[offset_within_packet].size() > (bp[offset_within_packet]+1) ) 
    {
    offset_within_entry = poses[offset_within_packet][bp[offset_within_packet]+1];
    }
   else // bimodal, this is not used since bimodal uses offset_within_packet directly in update_entry_e
   {
   	offset_within_entry = offset_within_packet;
   }
  	update_entry_e(bp[offset_within_packet] + 1, offset_within_packet, offset_within_entry, taken);
  }

        #ifdef DEBUG_POS
      	std::cerr << "55555 \n";
        #endif // DEBUG_POS
#else // POS
  if (!s[offset_within_packet][bp[offset_within_packet]].highconf() && (bp[offset_within_packet] < (int)hit[offset_within_packet].size())) {
    update_entry_p(bp[offset_within_packet] + 1, offset_within_packet, taken);
  }
#endif // POS

  // ALLOCATE
  // TODO Check what to do for these getg (for allocation)
  bool allocate = !noalloc && (s[offset_within_packet][bp[offset_within_packet]].pred() != taken);

  #ifdef DEBUG
  fprintf (stderr, "For allocation - s[%d][%d] = %d, %d\n", offset_within_packet, bp[offset_within_packet], \
  s[offset_within_packet][bp[offset_within_packet]].n[0], s[offset_within_packet][bp[offset_within_packet]].n[1]);
  #endif // DEBUG

  if (allocate && ((int)(rando() % MINAP) >= ((cat * MINAP) / (CATMAX + 1)))) {
    int i = (hit[offset_within_packet].size() > 0) ? hit[offset_within_packet][0] : SBP_NUMG;
    i -= rando() % (1 + s[offset_within_packet][0].diff() * SKIPMAX / dualcounter::nmax);
    int mhc = 0;
/*
If nothing hit, i = SBP_NUMG and then something less than that, but no bank hit
If any bank had hit, i starts = hit[offset_within_packet][0], leftmost hitting table (smallest table number), then we subtract randomly a positive number to possibly move to left, so smaller i
and then we decrement i in while loop, so inside the while loop, none of the tables ever hit.
Hence, this will update ALL (but none of thenm hit) till it can ALLOCATE ON ONE,
after allocation, it exits the loop with break, so no more updates/ allocations after first allocation
*/
    while (--i >= 0) {

#if (defined (POS) || defined (MT_PLUS))
	int max_conf = -1;
	offset_within_entry = 0;
	bool free_subentry_avail = false;
	
/* TODO - If any subentry is free, allocate that, else allocate according to Xiangshan rather than conflevel
 Also check decaying all subentries rather than just one if no subentry is free 
 Also LRU and random */
 
#if ( defined (RANDOM_ALLOCS) || defined (NOT_MRU) )
#ifdef NOT_MRU
do {
#endif // NOT_MRU
offset_within_entry = random % INFO_PER_ENTRY[i];
random = (random ^ (random >> 5) ) ;
#ifdef NOT_MRU
} while(offset_within_entry == getgb(i).mru);
#endif // NOT_MRU
#else // RANDOM_ALLOCS_or_NOT_MRU

	random = (random ^ (random >> 5) ) ;
	int r = random %2;
	for (int j = r ? 0 : INFO_PER_ENTRY[i]-1; r ?  (j < INFO_PER_ENTRY[i]) : (j >= 0); r ? (j++) : (j--) )
	{
#ifdef MT_PLUS
		if ( (getge(i, j).dualc.is_counter_reset()) && (getge(i, j).tag == 0) )
		{
			offset_within_entry = j;
			free_subentry_avail = true;
			break;
		}
#else //MT_PLUS
		if  ((getge(i, j).pos == -1) && (getge(i, j).tag == 0) )
		{
			offset_within_entry = j;
			free_subentry_avail = true;
			break;
		}
#endif //MT_PLUS

#ifdef CONFLEVEL
// TODO Check behavior of conflevel, may require <
		if (getge(i, j).dualc.conflevel(meta) > max_conf)
		{
			max_conf = getge(i, j).dualc.conflevel(meta);
			offset_within_entry = j;
		}
#endif // CONFLEVEL
	}
	
	if (free_subentry_avail == true)
	{
		goto JUST_ALLOCATE;
	}
#ifdef DEFAULT_MAP
	else
	{
		offset_within_entry = get_offset_within_entry(offset_within_packet, i);
	}
#endif // DEFAULT_MAP
#endif // RANDOM_ALLOCS
	
  	 if  (getge(i, offset_within_entry).dualc.highconf())  {
#ifdef USE_CD
        if ((int)(rando() % MINDP) >= ((cd * MINDP) / (CDMAX + 1)))
          getge(i, offset_within_entry).dualc.decay();
#else
        if ((rando() % 4) == 0)
          getge(i, offset_within_entry).dualc.decay();
#endif
        if (!getge(i, offset_within_entry).dualc.veryhighconf())
          mhc++;
      } else {
      
        // TODO Check what to do for these three - allocation should be same for SINGLE_TAG and MULTI_TAG
JUST_ALLOCATE :
	#ifdef POS
        getgb(i).tag = p.gtag(hash_fetch_pc, i);
	getge(i, offset_within_entry).pos = offset_within_packet;
	allocs[i]++;
	#endif
	#ifdef MT_PLUS
        getge(i, offset_within_entry).tag = p.gtag(hash_pc , i);
	allocs[i]++;
	#endif
#ifdef NOT_MRU
        getgb(i).mru = offset_within_entry;
#endif //NOT_MRU

  #ifdef DEBUG_ALLOC
  fprintf (stderr, "pc = %llx Allocated entry in bank %d\n", pc, i);
  #endif // DEBUG_ALLOC
  
  #ifdef MT_PLUS
    	getge(i, offset_within_entry).dualc.reset();
  	getge(i, offset_within_entry).dualc.update(taken);
  #else // MT_PLUS
  #ifdef BANK_INTERLEAVING
  	uint32_t offset_within_entry_bank_i = offset_within_packet / (FETCHWIDTH / INFO_PER_ENTRY(bank[i]));
  	for (uint32_t offset = 0; offset < INFO_PER_ENTRY(bank[i]); offset++) {
  	  getge(bank[i], offset).dualc.reset();
          if (offset == offset_within_entry_bank_i) {
            getge(bank[i], offset).dualc.update(taken);
          }
        }
  #else
  	for (uint32_t offset = 0; offset < INFO_PER_ENTRY[i]; offset++) {
  	  getge(i, offset).dualc.reset();
          if (offset == offset_within_entry) {
            getge(i, offset).dualc.update(taken);
          }
        }
  #endif // BANK_INTERLEAVING
  #endif // MT_PLUS
        cat += CATR_NUM - mhc * CATR_DEN;
        cat = min(CATMAX, max(0, cat));
#ifdef USE_CD
        cd += CDR_NUM - mhc * CDR_DEN;
        cd = min(CDMAX, max(0, cd));
#endif
        break;
      }
      
      
#else // POS

  offset_within_entry = get_offset_within_entry (offset_within_packet, i); 
    
      if (getgp(i, offset_within_packet).dualc.highconf()) {
#ifdef USE_CD
        if ((int)(rando() % MINDP) >= ((cd * MINDP) / (CDMAX + 1)))
          getgp(i, offset_within_packet).dualc.decay();
#else
        if ((rando() % 4) == 0)
          getgp(i, offset_within_packet).dualc.decay();
#endif
        if (!getgp(i, offset_within_packet).dualc.veryhighconf())
          mhc++;
      } else {
      
        // TODO Check what to do for these three - allocation should be same for SINGLE_TAG and MULTI_TAG

        #ifndef SINGLE_TAG
        getgp(i, offset_within_packet).tag = p.gtag(hash_pc , i);
        #else // SINGLE_TAG
        getgb(i).tag = p.gtag(hash_fetch_pc, i);
        #endif // SINGLE_TAG
        
#ifdef BANK_INTERLEAVING
 	allocs[bank[i]]++;
#else
	allocs[i]++;
#endif // BANK_INTERLEAVING

  #ifdef DEBUG
  fprintf (stderr, "pc = %llx Allocated entry in bank %d\n", pc, i);
  #endif // DEBUG
  
  #ifndef SINGLE_TAG
  	getgp(i, offset_within_packet).dualc.reset();
  	getgp(i, offset_within_packet).dualc.update(taken);
  #else // SINGLE_TAG
  #ifdef BANK_INTERLEAVING
  	uint32_t offset_within_entry_bank_i = get_offset_within_entry (offset_within_packet, bank[i]);
  	for (uint32_t offset = 0; offset < INFO_PER_ENTRY(bank[i]); offset++) {
  	  getge(bank[i], offset).dualc.reset();
          if (offset == offset_within_entry_bank_i) {
            getge(bank[i], offset).dualc.update(taken);
          }
        }
  #else
  	for (uint32_t offset = 0; offset < INFO_PER_ENTRY[i]; offset++) {
  	  getge(i, offset).dualc.reset();
          if (offset == offset_within_entry) {
            getge(i, offset).dualc.update(taken);
          }
        }
  #endif // BANK_INTERLEAVING

    #endif // SINGLE_TAG
        
        cat += CATR_NUM - mhc * CATR_DEN;
        cat = min(CATMAX, max(0, cat));
#ifdef USE_CD
        cd += CDR_NUM - mhc * CDR_DEN;
        cd = min(CDMAX, max(0, cd));
#endif
        break;
      }
#endif //POS

  #ifdef DEBUG_ALLOC
  std::cerr << "Done allocate \n";
  #endif // DEBUG_ALLOC
    }
  } // allocate over
  
}

int batage::size() {
  int bimodal_size = 0, table_size = 0, totsize = 0;
  
  bimodal_size = (1 << SBP_LOGB) + (SBP_BHYSTBITS << SBP_LOGB2);
  fprintf (stderr, "Bimodal size = %u bits\n", bimodal_size);
  
  for (int i = 0; i < SBP_NUMG; i++)
  {
  
    fprintf (stderr, "table %d, ORIG_ENTRIES_PER_TABLE = %d, SBP_LOGG = %d, SS_ENTRIES_PER_TABLE = %d, LOGGE_ORIG = %d, ENTRIES_PER_TABLE = %d, SBP_LOGGE = %d\n", i, ORIG_ENTRIES_PER_TABLE[i], SBP_LOGG[i], SS_ENTRIES_PER_TABLE[i], LOGGE_ORIG[i], ENTRIES_PER_TABLE[i], SBP_LOGGE[i]);
    
  #ifndef SINGLE_TAG
   table_size =  (((dualcounter::size()+ SBP_TAGBITS) *INFO_PER_ENTRY[i]) * ENTRIES_PER_TABLE[i]);
  #else // SINGLE_TAG
  table_size =  ((((dualcounter::size() + POSBITS) * INFO_PER_ENTRY[i]) + SBP_TAGBITS) * ENTRIES_PER_TABLE[i]);
  #endif // SINGLE_TAG
  totsize += table_size;
  }
  
  fprintf (stderr, "Total size = %u bits\n", totsize);
  
  #ifdef SINGLE_TAG
    //fprintf (stderr, " NEW_ENTRIES_PER_TABLE = %u \n", NEW_ENTRIES_PER_TABLE[i]);
        /*fprintf (stderr, " LOST_ENTRIES_PER_TABLE = %u, LOST_ENTRIES_TOTAL = %u \n", LOST_ENTRIES_PER_TABLE, LOST_ENTRIES_TOTAL);*/
   #endif // SINGLE_TAG
   
    return totsize; // number of bits
  // the storage for counters 'cat', 'meta' and 'cd' is neglected here
}
