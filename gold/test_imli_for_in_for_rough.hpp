AddrType PC = 0x05050a0a;    
  AddrType branchTarget = 0x05050aa0;
  bool bias;
  uint32_t sign;
  bool no_alloc = false; // Check ??- Allocate on a mispredicttion
  bool resolveDir;
  uint32_t correct_predicitons = 0, mispredictions = 0;
  
  for (int i = 0; i < 1050; i++)
  {
  bool predDir = IMLI_inst.getPrediction(PC, bias,  sign);
  printf ("Prediction = \t\t%32s \n", predDir ? "predicted taken" : "predicted not taken");
  if (i % 3)
  	resolveDir = false;
  else
  	resolveDir = true;
  	
  printf ("Actual outcome = \t%32s \n", resolveDir ? "taken" : "not taken");
  
  #ifdef MISPREDICTIONRATE_STATS_AFTER_TRAINING
  if (i >= TRAINING_LENGTH-1)
#endif
  {
  	if (predDir == resolveDir)
  		correct_predicitons++;
  	else
  		mispredictions++;
  }
  
  IMLI_inst.updatePredictor(PC, resolveDir, predDir, branchTarget, no_alloc); // Not good prediction performance - Check if TAGE is getting updated.
  } 
  
  printf ("correct_predicitons = %u, mispredictions = %u, misprediction_rate = %f\n", correct_predicitons, mispredictions, (float)mispredictions/(correct_predicitons+mispredictions));
  
