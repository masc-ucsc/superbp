#pragma once

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

//#define DEBUG_HUQ

#define NUM_HUQ_ENTRIES (FETCH_WIDTH * INFO_PER_ENTRY)

class huq_entry {

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
  // uint64_t pc;
  uint64_t branchTarget;
  bool resolveDir;

public:
  // Default Constructor - for uninitialized - huq_data and huq[]
  huq_entry() = default;

  // Constructor - to allocate entry
  huq_entry(/*const uint64_t& pc1,*/ const uint64_t &branchTarget1,
            const bool &resolveDir1)
      : /*predDir {pc {pc1},*/ branchTarget{branchTarget1}, resolveDir{
                                                                resolveDir1} {}

  // Move assignment since queue must contain entire huq_entry (not just
  // pointer), since allocated entry will get "destructed"
  huq_entry &operator=(huq_entry &&src) {
    // pc = std::move(src.pc);
    branchTarget = std::move(src.branchTarget);
    resolveDir = std::move(src.resolveDir);
    return *this;
  }

  /*Move constructor - Not required yet
  huq_entry (huq_entry&& src)
  : pc {std::move(src.pc)}, branchTarget {std::move(src.branchTarget)}
  {
  }*/

  // Destructor - must check if any updates necessary
  ~huq_entry() = default;
#endif // BATAGE
};     // class huq_entry

using huq_entry_ptr = huq_entry *;

bool is_huq_full(void);
bool is_huq_empty(void);
#ifdef SUPERBP
void allocate_huq_entry(AddrType branch_PC, AddrType branch_target,
                        IMLI &IMLI_inst);
void get_huq_data(IMLI &IMLI_inst);
#elif defined BATAGE
void allocate_huq_entry(/*const uint64_t& pc,*/ const uint64_t &branchTarget,
                        const bool &resolveDir);
void huq_update_resolvedinfo (uint16_t index, bool resolveDir);
void get_huq_data(huq_entry *huq_data_ptr);
#endif
void nuke_huq();
