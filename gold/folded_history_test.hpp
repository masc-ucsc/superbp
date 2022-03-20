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
