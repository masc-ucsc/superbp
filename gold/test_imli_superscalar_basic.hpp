/*
TODO
if b1 is taken, b2 is not reached
b1 - pc = 0x05050a08, taken target = 0x05050b0b 
b2 - pc = 0x05050a0c, taken target = 0x05050ce0

8 - 1 byte instructions per packet => 8 bytes per packet => With byte addressable - packets are 0x8 aligned or 3 LSBs of packet address should be 0

  if (i < 4096)            	// b1 - pc = 0x05050a08, taken target = 0x05050b0b
  	{
      if (i < 2048)       	// b2 - pc = 0x05050a0c, taken target = 0x05050ce0
      ....
      else             	
      .... 
    }

Prediction should be returned for all PCs that might have a branch or a loop - so return a list of fetch_width predictions 
For update - update all PCs till the first taken branch
*/
  AddrType packet_PC = 0x05050a00;
  AddrType b1_PC = 0x05050a02; // 0x05050a08;      
  AddrType b1_branchTarget = 0x05050b0b; 
  AddrType b2_PC = 0x05050a06; // 0x05050a0c;        
  AddrType b2_branchTarget = 0x05050ce0; 
  
  bool bias;
  uint32_t sign;
  bool no_alloc = true; // Check ??
  bool b1_resolveDir, b2_resolveDir;
  bool b1_predDir, b2_predDir;
  uint32_t b1_correct_predictions = 0, b1_mispredictions = 0;
  uint32_t b2_correct_predictions = 0, b2_mispredictions = 0;
  
#define ACTUAL_CODE
//#define TRIAL_DEBUG_DELETE   
#ifdef ACTUAL_CODE
 for (int i = 0; i < 8192; i++)
  {
  	IMLI_inst.fetchBoundaryBegin(packet_PC);
  	
  	if (is_ftq_full()) {printf("FTQ full, stall \n");}
  	b1_predDir = IMLI_inst.getPrediction(b1_PC, bias,  sign);
  	allocate_ftq_entry(b1_PC, b1_branchTarget, IMLI_inst);
  	
  	if (i < 4096)
  	{
  		b1_resolveDir = false;
  		#ifdef MISPREDICTIONRATE_STATS_AFTER_TRAINING
  		if (i > TRAINING_LENGTH-1)
  		#endif
  		{
  			if (b1_predDir == b1_resolveDir)
  				b1_correct_predictions++;
  			else
  				b1_mispredictions++;
  		}
  		
  		if (is_ftq_full()) {printf("FTQ full, stall \n");}
  		b2_predDir = IMLI_inst.getPrediction(b2_PC, bias,  sign);
  		allocate_ftq_entry(b1_PC, b1_branchTarget, IMLI_inst);
  		if (i < 2048) // b2 - NT
  			b2_resolveDir = false;
  		else // b2 - T
  			b2_resolveDir = true;  
  		
  		#ifdef MISPREDICTIONRATE_STATS_AFTER_TRAINING
  		if (i > TRAINING_LENGTH-1)
  		#endif
  		{
  			if (b2_predDir == b2_resolveDir)
  				b2_correct_predictions++;
  			else
  				b2_mispredictions++;
  		}
  		
  		get_ftq_data(IMLI_inst);
  		IMLI_inst.updatePredictor(b1_PC, b1_resolveDir, b1_predDir, b1_branchTarget, no_alloc);
  		get_ftq_data(IMLI_inst);
  		IMLI_inst.updatePredictor(b2_PC, b2_resolveDir, b2_predDir, b2_branchTarget, no_alloc);
  	} // b1 - NT
  	else // b1 - T
  	{
  		b1_resolveDir = true;
  		#ifdef MISPREDICTIONRATE_STATS_AFTER_TRAINING
  		if (i >= TRAINING_LENGTH-1)
  		#endif
  		{
  			if (b1_predDir == b1_resolveDir)
  				b1_correct_predictions++;
  			else
  				b1_mispredictions++;
  		}
  
  		IMLI_inst.updatePredictor(b1_PC, b1_resolveDir, b1_predDir, b1_branchTarget, no_alloc);
  	} // b1 - T
  	
  	IMLI_inst.fetchBoundaryEnd();
  } // for (int i = 0; i < 8192; i++)
  
  printf ("\n====================================================================================\n");
  printf ("b1_correct_predictions = %u, b1_mispredictions = %u, b1_misprediction_rate = %f\n", b1_correct_predictions, b1_mispredictions, (float)b1_mispredictions/(b1_correct_predictions + b1_mispredictions));
  printf ("b2_correct_predictions = %u, b2_mispredictions = %u, b2_misprediction_rate = %f\n", b2_correct_predictions, b2_mispredictions, (float)b2_mispredictions/(b2_correct_predictions + b2_mispredictions));
  printf ("====================================================================================\n");
  
#endif // ACTUAL_CODE
  
  // trial - debug - delete everything below this

#ifdef TRIAL_DEBUG_DELETE   
  for (int i = 0; i < 8192; i++)
  {
  	IMLI_inst.fetchBoundaryBegin(packet_PC);
  	
  	b1_predDir = IMLI_inst.getPrediction(b1_PC, bias,  sign);
  	
  	if (i < 4096)
  	{
  		b1_resolveDir = false;
  		#ifdef MISPREDICTIONRATE_STATS_AFTER_TRAINING
  		if (i > TRAINING_LENGTH-1)
  		#endif
  		{
  			if (b1_predDir == b1_resolveDir)
  				b1_correct_predictions++;
  			else
  				b1_mispredictions++;
  		}
  		IMLI_inst.updatePredictor(b1_PC, b1_resolveDir, b1_predDir, b1_branchTarget, no_alloc);
  		b2_predDir = IMLI_inst.getPrediction(b2_PC, bias,  sign);
  		if (i < 2048) // b2 - NT
  			b2_resolveDir = false;
  		else // b2 - T
  			b2_resolveDir = true;  
  		
  		#ifdef MISPREDICTIONRATE_STATS_AFTER_TRAINING
  		if (i > TRAINING_LENGTH-1)
  		#endif
  		{
  			if (b2_predDir == b2_resolveDir)
  				b2_correct_predictions++;
  			else
  				b2_mispredictions++;
  		}
  		
  		
  		IMLI_inst.updatePredictor(b2_PC, b2_resolveDir, b2_predDir, b2_branchTarget, no_alloc);
  	} // b1 - NT
  	else // b1 - T
  	{
  		b1_resolveDir = true;
  		#ifdef MISPREDICTIONRATE_STATS_AFTER_TRAINING
  		if (i >= TRAINING_LENGTH-1)
  		#endif
  		{
  			if (b1_predDir == b1_resolveDir)
  				b1_correct_predictions++;
  			else
  				b1_mispredictions++;
  		}
  
  		IMLI_inst.updatePredictor(b1_PC, b1_resolveDir, b1_predDir, b1_branchTarget, no_alloc);
  	} // b1 - T
  	
  	IMLI_inst.fetchBoundaryEnd();
  } // for (int i = 0; i < 8192; i++)
  
  printf ("\n====================================================================================\n");
  printf ("b1_correct_predictions = %u, b1_mispredictions = %u, b1_misprediction_rate = %f\n", b1_correct_predictions, b1_mispredictions, (float)b1_mispredictions/(b1_correct_predictions + b1_mispredictions));
  printf ("b2_correct_predictions = %u, b2_mispredictions = %u, b2_misprediction_rate = %f\n", b2_correct_predictions, b2_mispredictions, (float)b2_mispredictions/(b2_correct_predictions + b2_mispredictions));
  printf ("====================================================================================\n");
 
#endif // TRIAL_DEBUG_DELETE 
