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
