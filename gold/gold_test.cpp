
#include "imli.hpp"

#include <string>
#include "fmt/format.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "inst_opcode.hpp"

#define DEBUG_PRINTS

#define MISPREDICTIONRATE_STATS_AFTER_TRAINING
#ifdef MISPREDICTIONRATE_STATS_AFTER_TRAINING
#define TRAINING_LENGTH 512
#endif

#ifdef CPP
#define NUM_FTQ_ENTRIES 32
#endif // CPP

// Submodule tests
//#define FOLDED_HISTORY_TEST
//#define LOOP_ENTRY_TEST
//#define BIMODAL_TEST
//#define GLOBAL_ENTRY_TEST
#define IMLI_TEST

// IMLI Tests
//#define FOR_IN_FOR_ROUGH
//#define IF_ELSE_IN_IF_ELSE
#define SUPERSCALAR_SOTA
// Bad example - incomplete
//#define FOR_IN_FOR_PROPER


namespace {

class Gold_test : public ::testing::Test {
protected:
  void SetUp() override {
}

  void TearDown() override {
    // Graph_library::sync_all();
  }	

  //IMLI IMLI_inst(24, 3, 3, 6, true);
   //IMLI IMLI_inst;

};

#ifdef FOLDED_HISTORY_TEST
#include "folded_history_test.hpp"
#endif

#ifdef LOOP_ENTRY_TEST
#include "loop_entry_test.hpp"
#endif

#ifdef BIMODAL_TEST
#include "bimodal_test.hpp"
#endif

#ifdef GLOBAL_ENTRY_TEST
#include "global_entry_test.hpp"
#endif

#ifdef IMLI_TEST

TEST_F(Gold_test, Trivial_IMLI_test) {

  int blogb = 10, log2fetchwidth = 3, bwidth = 3, nhist = 6;
  bool sc = false;
  IMLI IMLI_inst( blogb, log2fetchwidth, bwidth, nhist, sc);
  
  #ifdef CPP
  ftq ftq_inst(NUM_FTQ_ENTRIES);
  #endif // CPP
  
#ifdef FOR_IN_FOR_ROUGH
#include "test_imli_for_in_for_rough.hpp"
#endif // FOR_IN_FOR_ROUGH

#ifdef FOR_IN_FOR_PROPER
#include "test_imli_for_in_for_proper.hpp"
#endif // FOR_IN_FOR_PROPER

#ifdef IF_ELSE_IN_IF_ELSE
#include "test_imli_if_else_in_if_else.hpp"
#endif  // IF_ELSE_IN_IF_ELSE

#ifdef SUPERSCALAR_SOTA
#include "test_imli_superscalar_sota.hpp"  
#endif // SUPERSCALAR_BASIC_SOTA

} // TEST_F
#endif // IMLI_TEST

} // namespace
