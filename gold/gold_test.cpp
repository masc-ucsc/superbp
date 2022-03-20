
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

// Submodule tests
//#define FOLDED_HISTORY_TEST
//#define LOOP_ENTRY_TEST
//#define BIMODAL_TEST
//#define GLOBAL_ENTRY_TEST
#define IMLI_TEST

// IMLI Tests
//#define FOR_IN_FOR_ROUGH
#define IF_ELSE_IN_IF_ELSE
//#define TWO_TAKEN_BRANCHES_IN_ONE_PACKET
// Bad example
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
  
#ifdef FOR_IN_FOR_ROUGH
#include "test_imli_for_in_for_rough.hpp"
#endif // FOR_IN_FOR_ROUGH

#ifdef FOR_IN_FOR_PROPER
#include "test_imli_for_in_for_proper.hpp"
#endif // FOR_IN_FOR_PROPER

#ifdef IF_ELSE_IN_IF_ELSE
#include "test_imli_if_else_in_if_else.hpp"
#endif  // IF_ELSE_IN_IF_ELSE

#ifdef TWO_TAKEN_BRANCHES_IN_ONE_PACKET
/*
TODO
if b1 is taken, b2 is as well
b1 - pc = 0x05050a0a, taken target = 0x05050b0b 
b2 - pc = 0x05050b0c, taken target = 0x05050ce0
*/
  AddrType b1_PC = 0x05050a0a;      
  AddrType b1_branchTarget = 0x05050b0b; 
  AddrType b2_PC = 0x05050b0c;        
  AddrType b2_branchTarget = 0x05050ce0; 
  
  bool bias;
  uint32_t sign;
  bool no_alloc = true; // Check ??
  bool b1_resolveDir, b2_resolveDir;
  bool b1_predDir, b2_predDir;
  uint32_t b1_correct_predicitons = 0, b1_mispredictions = 0;
  uint32_t b2_correct_predicitons = 0, b2_mispredictions = 0;

  int counter = 0;
  
  for (int i = 0; i < 5000; i++)
  {
  
  }  
#endif // TWO_BRANCHES_IN_ONE_PACKET

} // TEST_F
#endif // IMLI_TEST

} // namespace
