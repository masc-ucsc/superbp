#pragma once

#include "utils.hpp"
#include <inttypes.h>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <vector>

#include "../batage/predictor.hpp"
#include "emulator.hpp"
#ifdef BATAGE
#include "../batage/batage.hpp"
#endif

//#define DEBUG_FTQ
//#define DEBUG_ALLOC

/*#ifdef DEBUG_FTQ
FILE* fp = fopen ("ftq_log.txt", w+);
#endif*/

//#define INFO_PER_ENTRY 1
#define NUM_FTQ_ENTRIES (FETCH_WIDTH * INFO_PER_ENTRY * 2)

class ftq_entry {

#ifdef SUPERBP
public:
  AddrType branch_PC;
  AddrType branch_target; // Check
  bool pred_taken;
  bool tage_pred;
  bool LongestMatchPred;
  bool alttaken;
  int HitBank;
  int AltBank;
  long long IMLIcount;
  long long phist;
  long long GHIST;

  Folded_history *ch_i;
  Folded_history *ch_t_0;
  Folded_history *ch_t_1;
#elif defined BATAGE
public:
  bool predDir;			// 1 per instruction
  bool resolveDir;		// 1 per instruction
  uint64_t pc;			/* 1 per instruction (option is to do 1 per fetch_pc + index for each instruction, will save space) */
  insn_t insn;			// 1 per instruction
  uint64_t branchTarget;// 1 per instruction - TODO Check if required

/* Changes for SS predictor
hit - vector of banks that hit, stays the same
s - vector of counters from hitting banks -> need correct vector for each pc
bp - Dual counter procviding the final prediction, need correct for each 
bi, bi2 - Stays the same
gi - stays the same 
b_bi, b2_bi2 - Must return a vector - one element for each subentry/ offset ???
*/
  vector<int32_t> hit;   // size = NUMG - 1 per fetch_pc
  vector<vector<dualcounter>> s; // size = NUMG - 1 per instruction
  int meta;
  vector<int> bp;
  // Check if required -> cd, cat , predictor.pred.<tagged_enry>.dualc,
  // predictor.pred.getg(hit[i]).dualc.n

  int bi;				// 1 per fetch packet
  int bi2;				// 1 per fetch packet
  // int b[1<<LOGB];
  // int b2[1<<LOGB2];
  vector<int> b_bi;				// 1 per instruction
  vector<int> b2_bi2;			// 1 per instruction
  std::vector<int> gi;	// 1 per fetch packet

  // ptr, fold

  // histories * hist_ptr;

public:
  // Default Constructor - for uninitialized - ftq_data and ftq[]
  ftq_entry() = default;

  // Constructor - to allocate entry
  ftq_entry(const bool &predDir1, const bool &resolveDir1, const uint64_t &pc1,
            const insn_t &insn1, const uint64_t &branchTarget1,
            const PREDICTOR &predictor)
      /*: predDir {predDir1}, resolveDir {resolveDir1}, pc {pc1}, branchTarget
         {branchTarget1}, hit {predictor.pred.hit}, s {predictor.pred.s}, meta
         {predictor.pred.meta}, bp {predictor.pred.bp}, cat
         {predictor.pred.cat},  bi {predictor.pred.bi}, bi2
         {predictor.pred.bi2}, b {std::vector<int>(std::begin(predictor.pred.b),
         std::end(predictor.pred.b))}, b2
         {std::vector<int>(std::begin(predictor.pred.b2),
         std::end(predictor.pred.b2))}*/
      : predDir{predDir1}, resolveDir{resolveDir1}, pc{pc1}, insn{insn1},
        branchTarget{branchTarget1}, hit{predictor.pred.hit},
        s{predictor.pred.s}, meta{predictor.pred.meta}, bp{predictor.pred.bp},
        bi{predictor.pred.bi}, bi2{predictor.pred.bi2},
        b_bi{predictor.pred.b_bi}, b2_bi2{predictor.pred.b2_bi2},
        gi{ptr2vec(predictor.pred.gi, NUMG)} {}

  // Move assignment since queue must contain entire ftq_entry (not just
  // pointer), since allocated entry will get "destructed"
  ftq_entry &operator=(ftq_entry &&src) {
    predDir = std::move(src.predDir);
    resolveDir = std::move(src.resolveDir);
    pc = std::move(src.pc);
    insn = std::move(src.insn);
    branchTarget = std::move(src.branchTarget);

    hit = std::move(src.hit);
    s = std::move(src.s);
    meta = std::move(src.meta);
    bp = std::move(src.bp);

    b_bi = std::move(src.b_bi);
    b2_bi2 = std::move(src.b2_bi2);
    bi = std::move(src.bi);
    bi2 = std::move(src.bi2);

    gi = std::move(src.gi);

    return *this;
  }

  /*Move constructor - Not required yet
  ftq_entry (ftq_entry&& src)
  : predDir {std::move(src.predDir)}, resolveDir {std::move(src.resolveDir)}, pc
  {std::move(src.pc)}, branchTarget {std::move(src.branchTarget)}
  {
  }*/

  // Destructor - must check if any updates necessary
  ~ftq_entry() = default;
#endif // BATAGE
};     // class ftq_entry

using ftq_entry_ptr = ftq_entry *;

bool is_ftq_full(void);
bool is_ftq_empty(void);
uint16_t get_num_free_ftq_entries (void);

#ifdef SUPERBP
void allocate_ftq_entry(AddrType branch_PC, AddrType branch_target,
                        IMLI &IMLI_inst);
void get_ftq_data(IMLI &IMLI_inst);
#elif defined BATAGE
void allocate_ftq_entry(const bool &predDir, const bool &resolveDir,
                        const uint64_t &pc, const insn_t &insn,
                        const uint64_t &branchTarget,
                        const PREDICTOR &predictor); // , histories* hist_ptr);
void set_ftq_index (uint16_t index);
bool get_predDir_from_ftq (uint16_t index);
void ftq_update_resolvedinfo (uint16_t index, insn_t insn, bool resolveDir, uint64_t branchTarget); 
void get_ftq_data(ftq_entry *ftq_data_ptr);
void deallocate_ftq_entry(void);
#endif
void nuke_ftq();
