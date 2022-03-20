
 AddrType outer_PC = 0x0505b0b0;        // end address
  AddrType outer_branchTarget = 0x05050a0a;  // start address
  AddrType inner_PC = 0x0505a0a0;        // end address
  AddrType inner_branchTarget = 0x05050b0b;  // start address
  bool bias;
  uint32_t sign;
  bool no_alloc = true; // Check ??
  bool outer_resolveDir, inner_resolveDir;
  uint32_t outer_correct_predicitons = 0, outer_mispredictions = 0, inner_correct_predicitons = 0, inner_mispredictions = 0;
  
  for (int i = 0; i < 64; i++)
  {
  bool outer_predDir = IMLI_inst.getPrediction(outer_PC, bias,  sign);
  printf ("outer Prediction = \t\t%32s \n", outer_predDir ? "predicted taken" : "predicted not taken");
  
  for (int j = 0; j < 64; j++) 
  {
  		bool inner_predDir = IMLI_inst.getPrediction(inner_PC, bias,  sign);
  		printf ("inner Prediction = \t\t%32s \n", inner_predDir ? "predicted taken" : "predicted not taken");
  		
  		if (i == 63)
  			inner_resolveDir = false;
  		else
  			inner_resolveDir = true;
  	
  printf ("Inner Actual outcome = \t%32s \n", inner_resolveDir ? "taken" : "not taken");
  
  #ifdef MISPREDICTIONRATE_STATS_AFTER_TRAINING
  if (i >= TRAINING_LENGTH-1)
#endif
  {
  	if (inner_predDir == inner_resolveDir)
  		inner_correct_predicitons++;
  	else
  		inner_mispredictions++;
  }
  
  IMLI_inst.updatePredictor(inner_PC, inner_resolveDir, inner_predDir, inner_branchTarget, no_alloc); // Not good prediction performance - Check if TAGE is getting updated.
  }
  if (i == 63)
  	outer_resolveDir = false;
  else
  	outer_resolveDir = true;
  	
  printf ("Outer Actual outcome = \t%32s \n", outer_resolveDir ? "taken" : "not taken");
  
#ifdef MISPREDICTIONRATE_STATS_AFTER_TRAINING
  if (i >= TRAINING_LENGTH-1)
#endif
  {
  	if (outer_predDir == outer_resolveDir)
  		outer_correct_predicitons++;
  	else
  		outer_mispredictions++;
  }
  
  IMLI_inst.updatePredictor(outer_PC, outer_resolveDir, outer_predDir, outer_branchTarget, no_alloc);
  } 
  
  printf ("inner_correct_predicitons = %u, inner_mispredictions = %u, inner_misprediction_rate = %f\n", inner_correct_predicitons, inner_mispredictions, (float)inner_mispredictions/(inner_correct_predicitons + inner_mispredictions));
  printf ("outer_correct_predicitons = %u, outer_mispredictions = %u, outer_misprediction_rate = %f\n", outer_correct_predicitons, outer_mispredictions, (float)outer_mispredictions/(outer_correct_predicitons + outer_mispredictions));
  
