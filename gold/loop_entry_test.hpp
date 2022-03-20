TEST_F(Gold_test, Trivial_Loop_entry_test1) {

  Loop_entry Loop_entry_inst;
  EXPECT_EQ (Loop_entry_inst.confid, 0 ) ;
  EXPECT_EQ(Loop_entry_inst.CurrentIter, 0);
  EXPECT_EQ(Loop_entry_inst.NbIter, 0);
  EXPECT_EQ (Loop_entry_inst.TAG, 0 );
  EXPECT_EQ(Loop_entry_inst.age, 0 );
  EXPECT_EQ (Loop_entry_inst.dir, false);

}
