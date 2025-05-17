// Author: Pierre Michaud
// Release 1, may 2018

#ifndef BATAGE_H
#define BATAGE_H

#include <math.h>
#include <stdint.h>

#include <vector>

// ARM instructions are 4-byte aligned ==> shift PC by 2 bits
// TODO - Check if the SBP_PC_SHIFT should be based on alignment or on instruction size, since we use RV32I
#define SBP_PC_SHIFT 1  // 1 for RISCV

// SBP_NUMG: number of tagged banks (also, number of non-null global history
// lengths) SBP_MAXHIST: greatest global history length SBP_MINHIST: smallest non-null
// global history length SBP_MAXPATH: path history length SBP_PATHBITS: number of target
// bits pushed into the path history per branch SBP_LOGB: log2 of the number of
// bimodal prediction bits SBP_LOGB2: log2 of the number of bimodal hysteresis
// entries SBP_LOGG: log2 of the number of entries in one tagged bank SBP_TAGBITS: tag
// size SBP_BHYSTBITS: number of bimodal hysteresis bits
#define SBP_MAXHIST   700
#define SBP_MINHIST   4
#define SBP_MAXPATH   30
#define SBP_PATHBITS  6
#define SBP_LOGB      12
#define SBP_LOGB2     10
#define SBP_TAGBITS   12
#define SBP_BHYSTBITS 2

// extern uint32_t SBP_NUMG;

#define NUM_ENTRIES (14016)  // (14016)

// #define SBP_LOGG (11)
// #define ORIG_ENTRIES_PER_TABLE(i)  ((NUM_ENTRIES/SBP_NUMG)/INFO_PER_ENTRY(i))
#ifndef POS
#define NUM_ENTRIES (14016)  // (14016)
// #define ORIG_ENTRIES_PER_TABLE(i)  ( (i > 9) ? 3488 : ( (i > 3) ? 960 : 320 ) )
#else  // POS
#define NUM_ENTRIES (11712)
// #define ORIG_ENTRIES_PER_TABLE(i)  ( (i > 9) ? (312*12) : ( (i > 3) ? (48 * 12) : (16 * 12) ) )
#endif  // POS

/*
#ifdef SINGLE_TAG
#define NEW_BITS_PER_TABLE(i) (SBP_TAGBITS * (INFO_PER_ENTRY(i)-1) * (SS_ENTRIES_PER_TABLE(i)))
#define NEW_ENTRY_SIZE(i) (SBP_TAGBITS + (INFO_PER_ENTRY(i) * (POSBITS + dualcounter::size()) ) )
#define NEW_ENTRIES_PER_TABLE(i) (NEW_BITS_PER_TABLE(i)/NEW_ENTRY_SIZE(i))
//#define LOG2_NEW_ENTRIES_PER_TABLE ((int)log2(NEW_ENTRIES_PER_TABLE))

//#define SBP_LOGGE  ((int)ceil(log2(ORIG_ENTRIES_PER_TABLE + NEW_ENTRIES_PER_TABLE)))
#define SBP_LOGGE(i)  ((int)ceil(log2(SS_ENTRIES_PER_TABLE(i)+ NEW_ENTRIES_PER_TABLE(i))))


//#define LOST_ENTRIES_PER_TABLE ((1<< LOGGE_ORIG) + NEW_ENTRIES_PER_TABLE - (1 << SBP_LOGGE))
//#define LOST_ENTRIES_TOTAL LOST_ENTRIES_PER_TABLE * SBP_NUMG


#else // SINGLE_TAG
#endif // SINGLE_TAG
*/

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
#define CATMAX   ((CATR_NUM << 15) - 1)
#define MINAP    11

// controlled decay, for a tiny accuracy boost (not in the BATAGE paper)
// CDR = CDR_NUM / CDR_DEN
// CDR must be greater than CATR
// CDR, CDMAX and MINDP must be re-tuned if you change SBP_NUMG or SBP_LOGG
// in particular, if you decrease CATR, you should probably decrease CDR too
#define USE_CD
#ifdef USE_CD
#define CDR_NUM 5
#define CDR_DEN 4
#define CDMAX   ((CDR_NUM << 10) - 1)
#define MINDP   5
#endif

// bank interleaving, inspired from Seznec's TAGE-SC-L (CBP 2016)
// #define BANK_INTERLEAVING
#ifdef BANK_INTERLEAVING
// taking MIDBANK=(SBP_NUMG-1)*0.4 is probably close to optimal
// take GHGBITS=1 for SBP_NUMG<~10
#define MIDBANK 5
#define GHGBITS 1
#endif

// #define DEBUG
// #define DEBUG_GINDEX

using namespace std;

class dualcounter {
public:
  static const int nmax = 7;
  int              n[2];
  dualcounter();
  dualcounter(int b1, int b2);
  void       reset();
  bool       is_counter_reset();
  int        pred();
  bool       mediumconf();
  bool       lowconf();
  bool       highconf();
  bool       veryhighconf();
  bool       ultrahighconf();
  int        sum();
  int        diff();
  bool       saturated();
  int        conflevel(int meta);
  void       decay();
  void       update(bool taken);
  static int size();
};

class path_history {
public:
  int       ptr;      // pointer into the path history
  int       hlength;  // history length
  unsigned *h;        // history data - array of unsigned ints
  void      init(int hlen);
  void      insert(unsigned val);
  unsigned &operator[](int n);
};

// folded global history, for speeding up hash computations
// I introduced it in the 2001 tech report "A Comprehensive Study of Dynamic
// Global History Branch Prediction" (https://hal.inria.fr/inria-00072400) I
// advertised it with the PPM-like predictor (CBP 2004) see also the paper by
// Schlais and Lipasti at ICCD 2016

class SBP_folded_history {
public:
  uint32_t fold;      // updated in update
  int      clength;   // constant after init
  int      olength;   // constant after init
  int      outpoint;  // constant after init
  uint32_t mask1;     // constant after init
  uint32_t mask2;     // constant after init
  void     init(int original_length, int compressed_length, int injected_bits);
  uint32_t rotateleft(uint32_t x, int m);
  void     update(path_history &ph);
};

class batage;
class gshare;

class histories {
public:
  std::vector<uint8_t> SBP_LOGGE;
  uint32_t             SBP_NUMG;
  uint32_t             NUM_GSHARE_ENTRIES;

  path_history        bh;    // global history of branch directions
  path_history        ph;    // path history (target address bits)
  SBP_folded_history *chg;   // compressed length = SBP_LOGGE
  SBP_folded_history *chgg;  // compressed length = SBP_LOGGE-hashparam
  SBP_folded_history *cht;   // compressed length = SBP_TAGBITS
  SBP_folded_history *chtt;  // compressed length = SBP_TAGBITS-1

  void get_predictor_vars(const batage *bp, const gshare *fp);
  histories() = default;

  histories(const batage *bp);
  void update(uint32_t targetpc, bool taken);
  int  gindex(uint32_t pc, int i, const batage *bp) const;
  int  gshare_index(uint32_t pc, int i, const batage *bp) const;
  int  gtag(uint32_t pc, int i) const;
#ifdef BANK_INTERLEAVING
  int phybank(int i) const;
  int ghg(int i) const;
#endif
  void printconfig();
  int  size();
};

// tagged entry
class tagged_entry {
public:
  int tag;

  int pos;

  uint32_t mru;

  dualcounter dualc;
  tagged_entry();
};

class prediction {
public:
  std::vector<bool> prediction_vector;
  std::vector<bool> highconf;  // i = high, 0 = low
  int               gshare_index;
  int               gshare_tag;

  prediction() = default;
};

class batage {
public:
  friend class histories;

/*
// #define SINGLE_TAG
#ifdef SINGLE_TAG
#define POS
#ifdef POS
#define POSBITS 4
#else  // POS
#define POSBITS 0
#endif  // POS
#else   // SINGLE_TAG
#define XIANGSHAN
#ifdef XIANGSHAN
// #define CONT_MAP
#define MT_PLUS
#endif  // XIANGSHAN
#endif  // SINGLE_TAG

#if (defined(POS) || defined(MT_PLUS))
// Exactly one of these must be defined
#define CONFLEVEL
// #define DEFAULT_MAP
// #define RANDOM_ALLOCS
// #define NOT_MRU
#endif  //  (defined (POS) || defined (MT_PLUS))
*/

//default
bool SINGLE_TAG = false;
bool POS = false;
uint8_t POSBITS = 0;

bool XIANGSHAN = true;
bool BOOM = false;
bool MT_PLUS = true;
bool CONFLEVEL = true;
bool DEFAULT_MAP = false;
bool RANDOM_ALLOCS = false;
bool NOT_MRU = false;

  // static std::vector<uint8_t> SBP_LOGGE;
  std::vector<uint8_t> SBP_LOGGE;

  uint32_t              SBP_NUMG;
  uint8_t               LOG2FETCHWIDTH;
  uint8_t               NUM_TAKEN_BRANCHES;
  std::vector<uint32_t> ORIG_ENTRIES_PER_TABLE;
  std::vector<uint32_t> INFO_PER_ENTRY;
  uint8_t               FETCHWIDTH;
  std::vector<uint8_t>  SBP_LOGE;
  uint8_t               LOGFE;
  uint8_t               SBP_LOGBE;
  uint8_t               SBP_LOGB2E;
  std::vector<uint8_t>  SBP_LOGG;
  std::vector<uint32_t> SS_ENTRIES_PER_TABLE;
  std::vector<uint8_t>  LOGGE_ORIG;

  std::vector<uint32_t> NEW_ENTRIES_PER_TABLE;
  std::vector<uint32_t> ENTRIES_PER_TABLE;

  // int b[1 << SBP_LOGBE][FETCHWIDTH];   // bimodal predictions
  vector<vector<int>> b;
  // int b2[1 << SBP_LOGB2E][FETCHWIDTH]; // bimodal hystereses
  vector<vector<int>> b2;

  tagged_entry ***g;    // tagged entries
  int             bi;   // hash for the bimodal prediction
  int             bi2;  // hash for the bimodal hysteresis
  vector<int>     b_bi;
  vector<int>     b2_bi2;
  int            *gi;  // Hashes for/Indices into the tagged banks

  vector<vector<int>> hit;  // tell which banks have a hit - stays same for single tag SS - a vector each for multitag SS

  vector<vector<dualcounter>> s;  // dual-counters for the hitting banks - vector of vectors for SS

  vector<vector<int>> poses;  // contains the offset within entry for the hitting subentry

  vector<vector<int>> tags;      // tags for each table
  vector<int>         bp;        // dual-counter providing the final BATAGE prediction - index within s - vector for SS
  prediction          pred_out;  // (INFO_PER_ENTRY);
  int                 cat;       // CAT counter
  int                 meta;      // for a small accuracy boost
#ifdef USE_CD
  int cd;  // for a small accuracy boost
#endif
#ifdef BANK_INTERLEAVING
  int  *bank;
  bool *check;
#endif
  vector<uint32_t> allocs;

  int random;

  uint32_t get_allocs(int table);
  void     read_env_variables();
  void     populate_dependent_globals();
  void     batage_resize();
  batage();
  tagged_entry &getgb(int i);
  tagged_entry &getgp(int i, uint32_t offset_within_packet);
  tagged_entry &getge(int i, uint32_t offset_within_entry);
  uint32_t      get_offset_within_entry(uint32_t offset_within_packet, int table);
  prediction   &predict_vec(uint64_t fetch_pc, const histories &p);
  void          update_bimodal(bool taken, uint32_t offset_within_packet);
  void          update_entry_p(int i, uint32_t offset_within_packet, bool taken);
  void          update_entry_e(int i, uint32_t offset_within_packet, uint32_t offset_within_entry, bool taken);
  void          update(uint64_t pc, uint64_t fetch_pc, uint32_t offset_within_packet, bool taken, const histories &p, bool noalloc);
  int           size();
#ifdef BANK_INTERLEAVING
  void check_bank_conflicts();
#endif

#ifdef DEBUG
  FILE *predict_pcs, *update_pcs;
#endif  // DEBUG
};

#endif
