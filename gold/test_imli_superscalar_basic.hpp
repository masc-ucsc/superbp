//#include "imli.hpp"

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

Order - 
1. Make all (both in this case) predictions.
2. Update in order - on a misprediction - //nuke AFTER the update for that branch  

b1 is taken - correctly predicted - update b1 - deallocate b2 from ftq w/o any updates - no nuke
b1 is taken - mispredicted - update b1 and nuke

b1 is not taken - correctly predicted
	
b1 is not taken - mispredicted
*/

//#define DEBUG_B1_MISPREDICTS
//#define DEBUG_FTQ_ENTRY_ALLOC_DEALLOC_NUKE

AddrType packet_PC = 0x05050a00;
AddrType b1_PC = 0x05050a02; // 0x05050a08;      
AddrType b1_branchTarget = 0x05050b0b; 
AddrType b2_PC = 0x05050a06; // 0x05050a0c;        
AddrType b2_branchTarget = 0x05050ce0; 
  
bool bias;
uint32_t sign;
bool no_alloc = false; // Check ??
bool b1_resolveDir, b2_resolveDir;
bool b1_predDir, b2_predDir;
uint32_t b1_correct_predictions = 0, b1_mispredictions = 0;
uint32_t b2_correct_predictions = 0, b2_mispredictions = 0;
 
for (int i = 0; i < 8192; i++)
{
  
  	/* Begin fetch boundry -  Make both predictions - Allocate ftq entries for both - Call Fetch boundry end 
  	ftq entry allocation must be done immediately after prediction - before next prediction ****************/
  	// Begin fetch boundry
  	IMLI_inst.fetchBoundaryBegin(packet_PC);
  	
  	// Make both predictions 
  	#ifdef CPP
  	if (ftq_inst.is_ftq_full()) {printf("FTQ full, stall \n"); return;}
  	#else
  	if (is_ftq_full()) {printf("FTQ full, stall \n"); return;}
  	#endif
  	b1_predDir = IMLI_inst.getPrediction(b1_PC, bias,  sign);
  	// Allocate ftq entries for both
  	#ifdef CPP
  	ftq_inst.allocate_ftq_entry(b1_PC, b1_branchTarget, IMLI_inst);
  	#else
  	allocate_ftq_entry(b1_PC, b1_branchTarget, IMLI_inst);
  	#endif
  	
  	#ifdef CPP
  	if (ftq_inst.is_ftq_full()) {printf("FTQ full, stall \n"); return;}
  	#else
  	if (is_ftq_full()) {printf("FTQ full, stall \n"); return;}
  	#endif
  	b2_predDir = IMLI_inst.getPrediction(b2_PC, bias,  sign);
  	#ifdef CPP
  	ftq_inst.allocate_ftq_entry(b2_PC, b2_branchTarget, IMLI_inst);
  	#else
  	allocate_ftq_entry(b2_PC, b2_branchTarget, IMLI_inst);
  	#endif
  	
  	#ifdef DEBUG_FTQ_ENTRY_ALLOC_DEALLOC_NUKE
  	printf ("2 ftq entries allocated \n");
  	#endif
  	// Call Fetch boundry end
  	IMLI_inst.fetchBoundaryEnd();  	
  	/* Begin fetch boundry -  Make both predictions - Allocate ftq entries for both - Call Fetch boundry end  - ALl done*/
  	
  	/******************************** b1 not taken ***************************************/
  	if (i < 4096) 
  	{
  		b1_resolveDir = false;
  		
  		/******** Read/deallocate ftq entry, update b1 predictor, update b1 stats  *******/	
  		// Read/deallocate ftq entry	
  		#ifdef CPP
  		ftq_inst.get_ftq_data(IMLI_inst);
  		#else
  		get_ftq_data(IMLI_inst);
  		#endif
  		#ifdef DEBUG_FTQ_ENTRY_ALLOC_DEALLOC_NUKE
  		printf ("ftq entry deallocated \n"); 
  		#endif 		
  		//update b1 predictor
  		IMLI_inst.updatePredictor(b1_PC, b1_resolveDir, b1_predDir, b1_branchTarget, no_alloc);
  		// update b1 stats
  		if (b1_predDir == b1_resolveDir)
  		{
  			#ifdef MISPREDICTIONRATE_STATS_AFTER_TRAINING
  			if (i > TRAINING_LENGTH-1)
  			#endif
  			b1_correct_predictions++;
  			#ifdef DEBUG_B1_MISPREDICTS
  			printf ("b1 correctly predicted \n");
  			#endif
  		}
  		else
  		{
  			#ifdef MISPREDICTIONRATE_STATS_AFTER_TRAINING
  			if (i > TRAINING_LENGTH-1)
  			#endif
  			b1_mispredictions++;
  			#ifdef DEBUG_B1_MISPREDICTS
  			printf ("b1 mispredicted \n");
  			#endif
  		}
  		/******** Read/deallocate ftq entry, update b1 predictor, update b1 stats done *******/	
  		
  		/* Resolve b2, Read/deallocate ftq entry, update b2 predictor, update b2 stats*/
  		// Resolve b2
  		if (i < 2048) // b2 - NT
  			b2_resolveDir = false;
  		else // b2 - T
  			b2_resolveDir = true;
  		// Read/deallocate ftq entry
  		#ifdef CPP
  		ftq_inst.get_ftq_data(IMLI_inst);
  		#else
  		get_ftq_data(IMLI_inst);
  		#endif
  		#ifdef DEBUG_FTQ_ENTRY_ALLOC_DEALLOC_NUKE
  		printf ("ftq entry deallocated \n");
  		#endif
  		// update b2 predictor
  		IMLI_inst.updatePredictor(b2_PC, b2_resolveDir, b2_predDir, b2_branchTarget, no_alloc);
  		// update b2 stats
  		if (b2_predDir == b2_resolveDir)
  		{
  			#ifdef MISPREDICTIONRATE_STATS_AFTER_TRAINING
  			if (i > TRAINING_LENGTH-1)
  			#endif
  			b2_correct_predictions++;
  		}
  		else
  		{
  			#ifdef MISPREDICTIONRATE_STATS_AFTER_TRAINING
  			if (i > TRAINING_LENGTH-1)
  			#endif
  			b2_mispredictions++;
  		}
  		/* Resolve b2, Read/deallocate ftq entry, update b2 predictor, update b2 stats*/	
  	}
  	/******************************** b1 not taken over ***************************************/
  	/******************************** b1 taken ***************************************/
  	else
  	{
  		b1_resolveDir = true;
  		
  		/******** Read/deallocate ftq entry, update b1 predictor, update b1 stats  *******/	
  		// Read/deallocate ftq entry	
  		#ifdef CPP
  		ftq_inst.get_ftq_data(IMLI_inst);
  		#else
  		get_ftq_data(IMLI_inst);
  		#endif
  		#ifdef DEBUG_FTQ_ENTRY_ALLOC_DEALLOC_NUKE
  		printf ("ftq entry deallocated \n"); 
  		#endif 		
  		//update b1 predictor
  		IMLI_inst.updatePredictor(b1_PC, b1_resolveDir, b1_predDir, b1_branchTarget, no_alloc);
  		// update b1 stats
  		if (b1_predDir == b1_resolveDir)
  		{
  			#ifdef MISPREDICTIONRATE_STATS_AFTER_TRAINING
  			if (i > TRAINING_LENGTH-1)
  			#endif
  			b1_correct_predictions++;
  			#ifdef DEBUG_B1_MISPREDICTS
  			printf ("b1 correctly predicted \n");
  			#endif
  		}
  		else
  		{
  			#ifdef MISPREDICTIONRATE_STATS_AFTER_TRAINING
  			if (i > TRAINING_LENGTH-1)
  			#endif
  			b1_mispredictions++;
  			#ifdef DEBUG_B1_MISPREDICTS
  			printf ("b1 mispredicted \n");
  			#endif
  		}
  		/******** Read/deallocate ftq entry, update b1 predictor, update b1 stats done *******/	
  		nuke();
  		#ifdef DEBUG_FTQ_ENTRY_ALLOC_DEALLOC_NUKE
  		printf ("nuked \n");
  		#endif
  	}
  	/******************************** b1 taken over ***************************************/
  		
 } // for (int i = 0; i < 8192; i++)		
  		
 printf ("\n====================================================================================\n");
  printf ("b1_correct_predictions = %u, b1_mispredictions = %u, b1_misprediction_rate = %f\n", b1_correct_predictions, b1_mispredictions, (float)b1_mispredictions/(b1_correct_predictions + b1_mispredictions));
  printf ("b2_correct_predictions = %u, b2_mispredictions = %u, b2_misprediction_rate = %f\n", b2_correct_predictions, b2_mispredictions, (float)b2_mispredictions/(b2_correct_predictions + b2_mispredictions));
  printf ("====================================================================================\n");
  
  

