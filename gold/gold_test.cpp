
#include "imli.hpp"

#include <string>
#include "fmt/format.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "inst_opcode.hpp"

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

/*
TEST_F(Gold_test, Trivial_Folded_history_test1) {

  Folded_history Folded_history_inst;
  
  int      compressed_length = 32;
  int      original_length = 200;
  Folded_history_inst.init(original_length, compressed_length);
  EXPECT_EQ (Folded_history_inst.OUTPOINT, (original_length % compressed_length)) << "OLENGTH init failed \n";
  EXPECT_EQ (Folded_history_inst.comp, 0) << "comp init failed \n";

  unsigned c = 0xa5a5;
  Folded_history_inst.set(c);
  EXPECT_EQ (Folded_history_inst.comp, (c) & ((1 << compressed_length) - 1)) << "set failed \n";

  int old_comp = Folded_history_inst.comp;
  Folded_history_inst.mix(c);
  EXPECT_EQ (Folded_history_inst.comp, old_comp ^ ((c) & ((1 << compressed_length) - 1))) << "mix failed \n";

}

TEST_F(Gold_test, Trivial_Loop_entry_test1) {

  Loop_entry Loop_entry_inst;
  EXPECT_EQ (Loop_entry_inst.confid, 0 ) ;
  EXPECT_EQ(Loop_entry_inst.CurrentIter, 0);
  EXPECT_EQ(Loop_entry_inst.NbIter, 0);
  EXPECT_EQ (Loop_entry_inst.TAG, 0 );
  EXPECT_EQ(Loop_entry_inst.age, 0 );
  EXPECT_EQ (Loop_entry_inst.dir, false);

}
*/

/*
TEST_F(Gold_test, Trivial_Bimodal_test1) {

  // Dummy - to be done
  int ls  = 10, lfw = 3, bw = 2;
  // 1024 entries, each entry has 8 sub entries for superscalar, each subentry = 2 bits
  // Index gets 13 lsbs of pc - ignores alignment ???
  Bimodal Bimodal_inst(ls, lfw, bw);
  printf ("bm_size = %d \n", Bimodal_inst.getsize());

  Bimodal_inst.dump();
  printf("\n");
   
  uint32_t fetchpc = 0xabcd1234;
  //printf ("Index for pc = %x = %x \n", fetchpc, Bimodal_inst.checkIndex(fetchpc));
  //printf ("Index for pc = %x = %x\n", fetchpc+4, Bimodal_inst.checkIndex(fetchpc+4));
  Bimodal_inst.select(fetchpc);
  for ( int i = 0; i <= 7; i++)
  {
	printf ("outer loop - %d\n", i);
	for ( int j = 0; j <= 4; j++)
  	{
    printf ("inner loop - %d\n", j);
  	printf ("Prediction = %s, confidence = %s, actual = T \n", Bimodal_inst.predict() ? "T": "NT", Bimodal_inst.highconf() ? "high" : "low");
  	Bimodal_inst.update(true);
  	Bimodal_inst.dump();
  	printf("\n"); 
  	}
  	printf ("Prediction = %s, confidence = %s, actual = NT \n", Bimodal_inst.predict() ? "T": "NT", Bimodal_inst.highconf() ? "high" : "low");
    Bimodal_inst.update(false);
  	Bimodal_inst.dump();
  	printf("\n"); 
  }
}
*/



TEST_F(Gold_test, Trivial_Global_entry_test1) {


  Global_entry Global_entry_inst;
  //fmt::print("hello world\n");

  Global_entry_inst.allocate(8);
  int tableid = 4; // table id
  uint32_t tag = 0x05050a0a; // tag
  bool taken = true; 
  Global_entry_inst.reset(tableid, tag, taken);
  printf("************************* Initial value ********************* \n");
  Global_entry_inst.dump();
  
  AddrType t = 0x05050a0a;
  int b = 4;
  
  for (int i = 0; i < 8; i++)
  {
 	Global_entry_inst.select(t,b);
 	bool prediction = Global_entry_inst.ctr_isTaken();
 	printf ("prediction = %s \n", prediction ? "taken":"not taken");
 	Global_entry_inst.ctr_update(tableid, !(taken));
 	printf("\n******************** Value after branch = %d ********************* \n", i+1);
 	Global_entry_inst.dump();
  }
}


} // namespace
