// Author: Pierre Michaud
// Release 1, may 2018

#ifndef BATAGE_H
#define BATAGE_H

#include <stdint.h>
#include <vector>

// ARM instructions are 4-byte aligned ==> shift PC by 2 bits
// TODO - Check if the PC_SHIFT should be based on alignment or on instruction size, since we use RV32I
#define PC_SHIFT 1 // 1 for RISCV

// NUMG: number of tagged banks (also, number of non-null global history
// lengths) MAXHIST: greatest global history length MINHIST: smallest non-null
// global history length MAXPATH: path history length PATHBITS: number of target
// bits pushed into the path history per branch LOGB: log2 of the number of
// bimodal prediction bits LOGB2: log2 of the number of bimodal hysteresis
// entries LOGG: log2 of the number of entries in one tagged bank TAGBITS: tag
// size BHYSTBITS: number of bimodal hysteresis bits
#define NUMG 26
#define MAXHIST 700
#define MINHIST 4
#define MAXPATH 30
#define PATHBITS 6
#define LOGB 12
#define LOGB2 10
#define LOGG 7
#define TAGBITS 12
#define BHYSTBITS 2

// Check - change it to use FETCH_WIDTH
#define LOG2FETCHWIDTH 4
#define FETCHWIDTH (1 << LOG2FETCHWIDTH)
#define NUM_TAKEN_BRANCHES 1
#define INFO_PER_ENTRY (FETCHWIDTH * NUM_TAKEN_BRANCHES)
#define LOGE 0 // log2(INFO_PER_ENTRY)
#define LOGBE (LOGB - LOGE)
#define LOGB2E (LOGB2 - LOGE)
#define LOGGE (LOGG - LOGE)

// SKIPMAX: maximum number of banks skipped on allocation
// if you change NUMG, you must re-tune SKIPMAX
#define SKIPMAX 5

// meta predictor, for a small accuracy boost (not in the BATAGE paper)
// inspired from the meta predictor in TAGE, but used differently
#define USE_META

// controlled allocation throttling (CAT)
// CATR = CATR_NUM / CATR_DEN
// CATR, CATMAX and MINAP must be re-tuned if you change NUMG or LOGG
// for NUMG<~20, a smaller NUMG needs a smaller CATR
// a larger predictor may need a larger CATMAX
#define CATR_NUM 2
#define CATR_DEN 3
#define CATMAX ((CATR_NUM << 15) - 1)
#define MINAP 16

// controlled decay, for a tiny accuracy boost (not in the BATAGE paper)
// CDR = CDR_NUM / CDR_DEN
// CDR must be greater than CATR
// CDR, CDMAX and MINDP must be re-tuned if you change NUMG or LOGG
// in particular, if you decrease CATR, you should probably decrease CDR too
#define USE_CD
#ifdef USE_CD
#define CDR_NUM 6
#define CDR_DEN 5
#define CDMAX ((CDR_NUM << 10) - 1)
#define MINDP 7
#endif

// bank interleaving, inspired from Seznec's TAGE-SC-L (CBP 2016)
#define BANK_INTERLEAVING
#ifdef BANK_INTERLEAVING
// taking MIDBANK=(NUMG-1)*0.4 is probably close to optimal
// take GHGBITS=1 for NUMG<~10
#define MIDBANK 10
#define GHGBITS 2
#endif

//#define DEBUG

using namespace std;

class dualcounter {
public:
  static const int nmax = 7;
  int n[2];
  dualcounter();
  dualcounter(int b1, int b2);
  void reset();
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
  folded_history *chg;  // compressed length = LOGGE
  folded_history *chgg; // compressed length = LOGGE-hashparam
  folded_history *cht;  // compressed length = TAGBITS
  folded_history *chtt; // compressed length = TAGBITS-1
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
  dualcounter dualc;
  tagged_entry();
};

class batage {
public:
  int b[1 << LOGBE][INFO_PER_ENTRY];   // bimodal predictions
  int b2[1 << LOGB2E][INFO_PER_ENTRY]; // bimodal hystereses
  tagged_entry ***g;                   // tagged entries
  int bi;                              // hash for the bimodal prediction
  int bi2;                             // hash for the bimodal hysteresis
  vector<int> b_bi;
  vector<int> b2_bi2;
  int *gi;               // Hashes for/Indices into the tagged banks
  vector<int> hit;       // tell which banks have a hit - stays same for SS
  vector<vector<dualcounter>> s; // dual-counters for the hitting banks - vector of vectors for SS
  vector<int> bp; // dual-counter providing the final BATAGE prediction - index within s - vector for SS
  vector<bool> predict; // (INFO_PER_ENTRY);
  int cat;  // CAT counter
  int meta; // for a small accuracy boost
#ifdef USE_CD
  int cd; // for a small accuracy boost
#endif
#ifdef BANK_INTERLEAVING
  int *bank;
  bool *check;
#endif
  batage();
  tagged_entry &getgb(int i);
  tagged_entry &getgo(int i, uint32_t offset_within_entry);
  std::vector<bool>& predict_vec(uint32_t pc, const histories &p);
  void update_bimodal(bool taken, uint32_t offset_within_entry);
  void update_entry(int i, uint32_t offset_within_entry, bool taken);
  void update(uint32_t pc, uint32_t fetch_pc, uint32_t offset_within_entry, bool taken, const histories &p, bool noalloc);
  int size();
#ifdef BANK_INTERLEAVING
  void check_bank_conflicts();
#endif

#ifdef DEBUG
  FILE *predict_pcs, *update_pcs;
#endif // DEBUG
};

#endif
