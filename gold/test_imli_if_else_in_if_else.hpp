

/*
  if (i < 4096)            	// b1 - pc = 0x05050a0a, taken target = 0x05050b0b - 2048 occurences
  	{
      if (i < 2048)       	// b2 - pc = 0x05050a0c, taken target = 0x05050ae0 - 2/3 *2048 occurences
      ....
      else             	// pc = 0x05050ae0,
      .... 
    }
  else                 	// pc = 0x05050b0b,
    {
      if (i < 6144)       	// b3 - pc = 0x05050b0f, taken target = 0x05050bf0 - 1/3 *2048 occurences
      ....
      else if (i < 7000)	// b4 - pc = 0x05050bf0, taken target = 0x05050c0b - 1/5 * 1/3 *2048 occurences
      .... 
    }
                        // pc = 0x05050c0b
*/
  AddrType b1_PC = 0x05050a0a;      
  AddrType b1_branchTarget = 0x05050b0b; 
  AddrType b2_PC = 0x05050a0c;        
  AddrType b2_branchTarget = 0x05050ae0; 
  AddrType b3_PC = 0x05050b0f;      
  AddrType b3_branchTarget = 0x05050bf0; 
  AddrType b4_PC = 0x05060bf0;        
  AddrType b4_branchTarget = 0x05060c0b; 
  
  bool bias;
  uint32_t sign;
  bool no_alloc = false; // Check ??
  bool b1_resolveDir, b2_resolveDir, b3_resolveDir, b4_resolveDir;
  bool b1_predDir, b2_predDir, b3_predDir, b4_predDir;
  uint32_t b1_correct_predicitons = 0, b1_mispredictions = 0;
  uint32_t b2_correct_predicitons = 0, b2_mispredictions = 0;
  uint32_t b3_correct_predicitons = 0, b3_mispredictions = 0;
  uint32_t b4_correct_predicitons = 0, b4_mispredictions = 0;
  
  
  for (int i = 0; i < 8192; i++)
  {
    IMLI_inst.fetchBoundaryBegin(b1_PC);
  	b1_predDir = IMLI_inst.getPrediction(b1_PC, bias,  sign);
  	#ifdef DEBUG_PRINTS
  	printf ("b1 Prediction = \t\t%32s \n", b1_predDir ? "predicted taken" : "predicted not taken");
  	#endif
  	if (i < 4096)
  		b1_resolveDir = false;
  	else
  		b1_resolveDir = true;  	
  	#ifdef DEBUG_PRINTS
  	printf ("b1 Actual outcome = \t%32s \n", b1_resolveDir ? "taken" : "not taken");
  	#endif
  	printf ("===============================================================================================\n");
  	
  #ifdef MISPREDICTIONRATE_STATS_AFTER_TRAINING
  if (i >= TRAINING_LENGTH-1)
  #endif
  {
  	if (b1_predDir == b1_resolveDir)
  		b1_correct_predicitons++;
  	else
  		b1_mispredictions++;
  }
  
  IMLI_inst.updatePredictor(b1_PC, b1_resolveDir, b1_predDir, b1_branchTarget, no_alloc);
  	
  	if (!b1_resolveDir)
  	{
  	    //IMLI_inst.fetchBoundaryBegin(b2_PC);
  		b2_predDir = IMLI_inst.getPrediction(b2_PC, bias,  sign);
  		#ifdef DEBUG_PRINTS
  		printf ("b2 Prediction = \t\t%32s \n", b2_predDir ? "predicted taken" : "predicted not taken");
  		#endif
  		if (i < 2048)
  			b2_resolveDir = false;
  		else
  			b2_resolveDir = true;  	
  		#ifdef DEBUG_PRINTS
  		printf ("b2 Actual outcome = \t%32s \n", b2_resolveDir ? "taken" : "not taken");
  		#endif
  		printf ("===============================================================================================\n");
  		
  		#ifdef MISPREDICTIONRATE_STATS_AFTER_TRAINING
  		if (i >= TRAINING_LENGTH-1)
  		#endif
  		{
  			if (b2_predDir == b2_resolveDir)
  				b2_correct_predicitons++;
  			else
  				b2_mispredictions++;
  		}
  
  		IMLI_inst.updatePredictor(b2_PC, b2_resolveDir, b2_predDir, b2_branchTarget, no_alloc);
  	}
  	else
  	{
  	    IMLI_inst.fetchBoundaryBegin(b3_PC);
  		b3_predDir = IMLI_inst.getPrediction(b3_PC, bias,  sign);
  		//#ifdef DEBUG_PRINTS
  		printf ("b3 Prediction = \t\t%32s \n", b3_predDir ? "predicted taken" : "predicted not taken");
  		//#endif
  		if (i < 6144)
  			{
  				b3_resolveDir = false;
  			}
  		else
  			{
  				b3_resolveDir = true;  
			}
  		//#ifdef DEBUG_PRINTS
  		printf ("b3 Actual outcome = \t%32s \n", b3_resolveDir ? "taken" : "not taken");
  		//#endif
  		printf ("===============================================================================================\n");
  		
  		#ifdef MISPREDICTIONRATE_STATS_AFTER_TRAINING
  		if (i >= TRAINING_LENGTH-1)
  		#endif
  		{
  			if (b3_predDir == b3_resolveDir)
  				b3_correct_predicitons++;
  			else
  				b3_mispredictions++;
  		}
  
  		IMLI_inst.updatePredictor(b3_PC, b3_resolveDir, b3_predDir, b3_branchTarget, no_alloc);
  		
  		if (b3_resolveDir)
  		{
  		  		IMLI_inst.fetchBoundaryBegin(b4_PC);
  				b4_predDir = IMLI_inst.getPrediction(b4_PC, bias,  sign);
  				if (i < 7000)
  					b4_resolveDir = false;
  				else
  					b4_resolveDir = true; 
  				
  				#ifdef DEBUG_PRINTS	
  				printf ("b4 Prediction = \t\t%32s \n", b4_predDir ? "predicted taken" : "predicted not taken");
  				printf ("b4 Actual outcome = \t%32s \n", b4_resolveDir ? "taken" : "not taken");
  				#endif
  				printf ("===============================================================================================\n");
  		
  				#ifdef MISPREDICTIONRATE_STATS_AFTER_TRAINING
  				if (i >= TRAINING_LENGTH-1)
  				#endif
  				{
  					if (b4_predDir == b4_resolveDir)
  						b4_correct_predicitons++;
  					else
  						b4_mispredictions++;
  				}
  
  				IMLI_inst.updatePredictor(b4_PC, b4_resolveDir, b4_predDir, b4_branchTarget, no_alloc); 
  		}
  	}
  } // end of for

#ifdef DEBUG_PRINTS
  printf ("b1_correct_predicitons = %u, b1_mispredictions = %u, b1_misprediction_rate = %f\n", b1_correct_predicitons, b1_mispredictions, (float)b1_mispredictions/(b1_correct_predicitons + b1_mispredictions));
  printf ("b2_correct_predicitons = %u, b2_mispredictions = %u, b2_misprediction_rate = %f\n", b2_correct_predicitons, b2_mispredictions, (float)b2_mispredictions/(b2_correct_predicitons + b2_mispredictions));
    printf ("b4_correct_predicitons = %u, b4_mispredictions = %u, b4_misprediction_rate = %f\n", b4_correct_predicitons, b4_mispredictions, (float)b4_mispredictions/(b4_correct_predicitons + b4_mispredictions));
  #endif // DEBUG_PRINTS
  printf ("b3_correct_predicitons = %u, b3_mispredictions = %u, b3_misprediction_rate = %f\n", b3_correct_predicitons, b3_mispredictions, (float)b3_mispredictions/(b3_correct_predicitons + b3_mispredictions));


