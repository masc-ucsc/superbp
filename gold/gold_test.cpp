
#include "imli.hpp"

#include <string>
#include "fmt/format.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"
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

TEST_F(Gold_test, Trivial_Global_entry_test1) {
   // Dummy - to be done
  int blogb  = 24, log2fetchwidth = 3, bwidth = 3, nhist = 6;
  bool sc = true;
  IMLI IMLI_inst(blogb, log2fetchwidth, bwidth, nhist, sc);
  //fmt::print("hello world\n");

  //EXPECT_NE(10,100) << "Test failure user message";
  int predictorsize = IMLI_inst.predictorsize();
  EXPECT_NE (0, predictorsize ) << "PredictorSize = " << predictorsize;

  AddrType PC = 0x0123456789abcdef;   // random value - unaligned, but ok

  int bindex = IMLI_inst.bindex(PC);
  EXPECT_EQ(bindex, ((PC) & ((1 << (blogb)) - 1)) );

  int lindex = IMLI_inst.lindex(PC);
  EXPECT_EQ(lindex, ((PC & ((1 << (LOGL - 2)) - 1)) << 2));
  bool isloop = IMLI_inst.getloop(PC);
  EXPECT_NE (isloop, true);
}

TEST_F(Gold_test, Trivial_Bimodal_test1) {

  // Dummy - to be done
  int blogb  = 24, log2fetchwidth = 3, bwidth = 3, nhist = 6;
  bool sc = true;
  IMLI IMLI_inst(blogb, log2fetchwidth, bwidth, nhist, sc);
  //fmt::print("hello world\n");

  //EXPECT_NE(10,100) << "Test failure user message";
  int predictorsize = IMLI_inst.predictorsize();
  EXPECT_NE (0, predictorsize ) << "PredictorSize = " << predictorsize;

  AddrType PC = 0x0123456789abcdef;   // random value - unaligned, but ok

  int bindex = IMLI_inst.bindex(PC);
  EXPECT_EQ(bindex, ((PC) & ((1 << (blogb)) - 1)) );

  int lindex = IMLI_inst.lindex(PC);
  EXPECT_EQ(lindex, ((PC & ((1 << (LOGL - 2)) - 1)) << 2));
  bool isloop = IMLI_inst.getloop(PC);
  EXPECT_NE (isloop, true);
}


TEST_F(Gold_test, Trivial_imli_test1) {

  // instantiate IMLI, some configuration and test the basic API so that it it
  // learns. It is also a way to showcase the API
     /*IMLI(int _blogb, int _log2fetchwidth, int _bwidth, int _nhist, bool _sc)
      : bimodal(_blogb, _log2fetchwidth, _bwidth)
      , blogb(_blogb)
      , log2fetchwidth(_log2fetchwidth)
      , bwidth(_bwidth)
      , nhist(_nhist>=MAXHIST?MAXHIST:_nhist)
      , sc(_sc)*/
  int blogb  = 24, log2fetchwidth = 3, bwidth = 3, nhist = 6;
  bool sc = true;
  IMLI IMLI_inst(blogb, log2fetchwidth, bwidth, nhist, sc);
  //fmt::print("hello world\n");

  //EXPECT_NE(10,100) << "Test failure user message";
  int predictorsize = IMLI_inst.predictorsize();
  EXPECT_NE (0, predictorsize ) << "PredictorSize = " << predictorsize;

  AddrType PC = 0x0123456789abcdef;   // random value - unaligned, but ok

  int bindex = IMLI_inst.bindex(PC);
  EXPECT_EQ(bindex, ((PC) & ((1 << (blogb)) - 1)) );

  int lindex = IMLI_inst.lindex(PC);
  EXPECT_EQ(lindex, ((PC & ((1 << (LOGL - 2)) - 1)) << 2));
  bool isloop = IMLI_inst.getloop(PC);
  EXPECT_NE (isloop, true);
}


} // namespace
