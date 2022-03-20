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
