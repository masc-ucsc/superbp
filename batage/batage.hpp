// Author: Pierre Michaud
// Release 1, may 2018

#ifndef BATAGE_H
#define BATAGE_H

#include <stdint.h>
#include <vector>
#include <math.h>

// ARM instructions are 4-byte aligned ==> shift PC by 2 bits
// TODO - Check if the SBP_PC_SHIFT should be based on alignment or on instruction size, since we use RV32I
#define SBP_PC_SHIFT 1 // 1 for RISCV

// SBP_NUMG: number of tagged banks (also, number of non-null global history
// lengths) SBP_MAXHIST: greatest global history length SBP_MINHIST: smallest non-null
// global history length SBP_MAXPATH: path history length SBP_PATHBITS: number of target
// bits pushed into the path history per branch SBP_LOGB: log2 of the number of
// bimodal prediction bits SBP_LOGB2: log2 of the number of bimodal hysteresis
// entries SBP_LOGG: log2 of the number of entries in one tagged bank SBP_TAGBITS: tag
// size SBP_BHYSTBITS: number of bimodal hysteresis bits
#define SBP_NUMG 12
#define SBP_MAXHIST 700
#define SBP_MINHIST 4
#define SBP_MAXPATH 30
#define SBP_PATHBITS 6
#define SBP_LOGB 12
#define SBP_LOGB2 10
#define SBP_TAGBITS 12
#define SBP_BHYSTBITS 2

// Check - change it to use FETCH_WIDTH
#define LOG2FETCHWIDTH (4)
#define FETCHWIDTH (1 << LOG2FETCHWIDTH)
#define NUM_TAKEN_BRANCHES (1)

//#define SINGLE_TAG
#ifdef SINGLE_TAG
#define POS
#ifdef POS
#define POSBITS 4
#else // POS
#define POSBITS 0
#endif // POS
#else // SINGLE_TAG
#define XIANGSHAN
#ifdef XIANGSHAN
//#define CONT_MAP
#define MT_PLUS
#endif // XIANGSHAN
#endif //SINGLE_TAG

#if (defined (POS) || defined (MT_PLUS))
// Exactly one of these must be defined
#define CONFLEVEL
//#define DEFAULT_MAP
//#define RANDOM_ALLOCS
//#define NOT_MRU
#endif //  (defined (POS) || defined (MT_PLUS))

#if (defined XIANGSHAN) || (defined SINGLE_TAG)
//#define INFO_PER_ENTRY(i)  ( (FETCHWIDTH >= 2) ? ((FETCHWIDTH * NUM_TAKEN_BRANCHES)>>2) :  (FETCHWIDTH * NUM_TAKEN_BRANCHES) )
#define INFO_PER_ENTRY(i) (2)
#else
//#define INFO_PER_ENTRY(i) ( (i >= 9) ? (FETCHWIDTH * NUM_TAKEN_BRANCHES) : ( (i >= 6) ? ((FETCHWIDTH * NUM_TAKEN_BRANCHES)>>1) : ( (i >= 3) ? ((FETCHWIDTH * NUM_TAKEN_BRANCHES)>>2) : ((FETCHWIDTH * NUM_TAKEN_BRANCHES)>>3) ) ) ) 
//#define INFO_PER_ENTRY(i) ( (i > 6) ? (FETCHWIDTH * NUM_TAKEN_BRANCHES) : ((FETCHWIDTH * NUM_TAKEN_BRANCHES)>>1) ) 
//#define INFO_PER_ENTRY(i) ( (i > 9) ? 2 : ( (i > 3) ? 1 : 1 ) ) 
#define INFO_PER_ENTRY(i) (FETCHWIDTH)
#endif // XIANGSHAN

#define SBP_LOGE(i) ((int)log2(INFO_PER_ENTRY(i)))

#define LOGFE  ((int)log2(FETCHWIDTH))
#define SBP_LOGBE (SBP_LOGB - LOGFE)
#define SBP_LOGB2E (SBP_LOGB2 - LOGFE)

#define NUM_ENTRIES (14016) // (14016)

//#define SBP_LOGG (11)
//#define ORIG_ENTRIES_PER_TABLE(i)  ((NUM_ENTRIES/SBP_NUMG)/INFO_PER_ENTRY(i)) 
#ifndef POS
#define NUM_ENTRIES (14016) // (14016)

//#define ORIG_ENTRIES_PER_TABLE(i)  ( (i < 4) ? 1168 : ( (i < 8) ? 1168 : 1168 ) ) 
#define ORIG_ENTRIES_PER_TABLE(i)  ( (i > 9) ? 3488 : ( (i > 3) ? 960 : 320 ) ) 
#else // POS
#define NUM_ENTRIES (11712)
#define ORIG_ENTRIES_PER_TABLE(i)  ( (i > 9) ? (312*12) : ( (i > 3) ? (48 * 12) : (16 * 12) ) ) 
#endif // POS
//#define ORIG_ENTRIES_PER_TABLE(i)  ( (i > 6) ? 1488 : 848 ) 
#define SBP_LOGG(i)  (int)ceil(log2(ORIG_ENTRIES_PER_TABLE(i)))

#define SS_ENTRIES_PER_TABLE(i) (ORIG_ENTRIES_PER_TABLE(i) / INFO_PER_ENTRY(i))
#define LOGGE_ORIG(i)  (SBP_LOGG(i) - SBP_LOGE(i))

#ifdef SINGLE_TAG
#define NEW_BITS_PER_TABLE(i) (SBP_TAGBITS * (INFO_PER_ENTRY(i)-1) * (SS_ENTRIES_PER_TABLE(i)))
#define NEW_ENTRY_SIZE(i) (SBP_TAGBITS + (INFO_PER_ENTRY(i) * (POSBITS + dualcounter::size()) ) )
#define NEW_ENTRIES_PER_TABLE(i) (NEW_BITS_PER_TABLE(i)/NEW_ENTRY_SIZE(i))
//#define LOG2_NEW_ENTRIES_PER_TABLE ((int)log2(NEW_ENTRIES_PER_TABLE))

//#define SBP_LOGGE  ((int)ceil(log2(ORIG_ENTRIES_PER_TABLE + NEW_ENTRIES_PER_TABLE)))
#define SBP_LOGGE(i)  ((int)ceil(log2(SS_ENTRIES_PER_TABLE(i)+ NEW_ENTRIES_PER_TABLE(i))))

/*
#define LOST_ENTRIES_PER_TABLE ((1<< LOGGE_ORIG) + NEW_ENTRIES_PER_TABLE - (1 << SBP_LOGGE))
#define LOST_ENTRIES_TOTAL LOST_ENTRIES_PER_TABLE * SBP_NUMG
*/

#else // SINGLE_TAG
#define NEW_ENTRIES_PER_TABLE(i) (0)
#define SBP_LOGGE(i)  (LOGGE_ORIG(i)) 
#endif // SINGLE_TAG

#define ENTRIES_PER_TABLE(i) (SS_ENTRIES_PER_TABLE(i) + NEW_ENTRIES_PER_TABLE(i))

// SKIPMAX: maximum number of banks skipped on allocation
// if you change SBP_NUMG, you must re-tune SKIPMAX
#define SKIPMAX 2

// meta predictor, for a small accuracy boost (not in the BATAGE paper)
// inspired from the meta predictor in TAGE, but used differently
#define USE_META

// controlled allocation throttling (CAT)
// CATR = CATR_NUM / CATR_DEN
// CATR, CATMAX and MINAP must be re-tuned if you change SBP_NUMG or SBP_LOGG
// for SBP_NUMG<~20, a smaller SBP_NUMG needs a smaller CATR
// a larger predictor may need a larger CATMAX
#define CATR_NUM 3
#define CATR_DEN 4
#define CATMAX ((CATR_NUM << 15) - 1)
#define MINAP 11

// controlled decay, for a tiny accuracy boost (not in the BATAGE paper)
// CDR = CDR_NUM / CDR_DEN
// CDR must be greater than CATR
// CDR, CDMAX and MINDP must be re-tuned if you change SBP_NUMG or SBP_LOGG
// in particular, if you decrease CATR, you should probably decrease CDR too
#define USE_CD
#ifdef USE_CD
#define CDR_NUM 5
#define CDR_DEN 4
#define CDMAX ((CDR_NUM << 10) - 1)
#define MINDP 5
#endif

// bank interleaving, inspired from Seznec's TAGE-SC-L (CBP 2016)
//#define BANK_INTERLEAVING
#ifdef BANK_INTERLEAVING
// taking MIDBANK=(SBP_NUMG-1)*0.4 is probably close to optimal
// take GHGBITS=1 for SBP_NUMG<~10
#define MIDBANK 5
#define GHGBITS 1
#endif

//#define DEBUG
//#define DEBUG_GINDEX



using namespace std;

class dualcounter {
public:
  static const int nmax = 7;
  int n[2];
  dualcounter();
  dualcounter(int b1, int b2);
  void reset();
  bool is_counter_reset ();
  int pred();
  bool mediumconf();
  bool lowconf();
  bool highconf();
  bool veryhighconf();
  int sum();
  int diff();
  bool saturated();
  int conflevel(int meta);
  void decay();
  void update(bool taken);
  static int size();
};

class path_history {
public:
  int ptr;     // pointer into the path history
  int hlength; // history length
  unsigned *h; // history data - array of unsigned ints
  void init(int hlen);
  void insert(unsigned val);
  unsigned &operator[](int n);
};

// folded global history, for speeding up hash computations
// I introduced it in the 2001 tech report "A Comprehensive Study of Dynamic
// Global History Branch Prediction" (https://hal.inria.fr/inria-00072400) I
// advertised it with the PPM-like predictor (CBP 2004) see also the paper by
// Schlais and Lipasti at ICCD 2016

class folded_history {
public:
  uint32_t fold;  // updated in update
  int clength;    // constant after init
  int olength;    // constant after init
  int outpoint;   // constant after init
  uint32_t mask1; // constant after init
  uint32_t mask2; // constant after init
  void init(int original_length, int compressed_length, int injected_bits);
  uint32_t rotateleft(uint32_t x, int m);
  void update(path_history &ph);
};

class histories {
public:
  path_history bh;      // global history of branch directions
  path_history ph;      // path history (target address bits)
  folded_history *chg;  // compressed length = SBP_LOGGE
  folded_history *chgg; // compressed length = SBP_LOGGE-hashparam
  folded_history *cht;  // compressed length = SBP_TAGBITS
  folded_history *chtt; // compressed length = SBP_TAGBITS-1
  histories();
  void update(uint32_t targetpc, bool taken);
  int gindex(uint32_t pc, int i) const;
  int gtag(uint32_t pc, int i) const;
#ifdef BANK_INTERLEAVING
  int phybank(int i) const;
  int ghg(int i) const;
#endif
  void printconfig();
  int size();
};

// tagged entry
class tagged_entry {
public:
  int tag;
  #ifdef POS
  int pos;
  #endif // POS
  #ifdef NOT_MRU
  int mru;
  #endif
  dualcounter dualc;
  tagged_entry();
};

class prediction
{
	public :
	std::vector<bool> prediction_vector;
	std::vector<bool> highconf; // i = high, 0 = low
	int gshare_index;
	int gshare_tag;
	
	prediction() = default;
};

class batage {

public:
  int b[1 << SBP_LOGBE][FETCHWIDTH];   // bimodal predictions
  int b2[1 << SBP_LOGB2E][FETCHWIDTH]; // bimodal hystereses
  tagged_entry ***g;                   // tagged entries
  int bi;                              // hash for the bimodal prediction
  int bi2;                             // hash for the bimodal hysteresis
  vector<int> b_bi;
  vector<int> b2_bi2;
  int *gi;               // Hashes for/Indices into the tagged banks
  
  vector<vector<int>> hit;       // tell which banks have a hit - stays same for single tag SS - a vector each for multitag SS
  
  vector<vector<dualcounter>> s; // dual-counters for the hitting banks - vector of vectors for SS
  
  vector<vector<int>> poses;  // contains the offset within entry for the hitting subentry
  
  vector<int> bp; // dual-counter providing the final BATAGE prediction - index within s - vector for SS
  prediction pred_out; // (INFO_PER_ENTRY);
  int cat;  // CAT counter
  int meta; // for a small accuracy boost
#ifdef USE_CD
  int cd; // for a small accuracy boost
#endif
#ifdef BANK_INTERLEAVING
  int *bank;
  bool *check;
#endif
vector<uint32_t> allocs;
#if (defined (POS) || defined (MT_PLUS))
int random;
#endif
uint32_t get_allocs(int table);
  batage();
  tagged_entry &getgb(int i);
  tagged_entry &getgp(int i, uint32_t offset_within_packet);
  tagged_entry &getge(int i, uint32_t offset_within_entry) ;
  uint32_t get_offset_within_entry (uint32_t offset_within_packet, int table);
  prediction& predict_vec(uint32_t fetch_pc, const histories &p);
  void update_bimodal(bool taken, uint32_t offset_within_packet);
  void update_entry_p(int i, uint32_t offset_within_packet, bool taken);
  void update_entry_e(int i, uint32_t offset_within_packet, uint32_t offset_within_entry, bool taken);
  void update(uint32_t pc, uint32_t fetch_pc, uint32_t offset_within_packet, bool taken, const histories &p, bool noalloc);
  int size();
#ifdef BANK_INTERLEAVING
  void check_bank_conflicts();
#endif

#ifdef DEBUG
  FILE *predict_pcs, *update_pcs;
#endif // DEBUG
};

#endif
