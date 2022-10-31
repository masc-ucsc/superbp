/*

8 - 1 byte instructions per packet => 8 bytes per packet => With byte addressable - packets are 0x8 aligned or 3 LSBs of packet address should be 0

fetchPC = 0x05050a08
8 instructions - 
nop, conditional branch (b1), nop, nop, conditional branch (b2), unconditional jump (j1), nop, nop
Note  - update_predictor has no optype argument - only call for conditional


  if ( (!(i % 3)) || (!(i % 5)) )       // b1 - pc = 0x05050a09, taken target = 0x05050b0b
  	{
      if ( !(i % 4) || !(i % 6))       	// b2 - pc = 0x05050a0c, taken target = 0x05050ce0
      {
      jump/all/ret                      // j1 - pc = 0x05050a0d, taken target = 0x05050cf0
      }
      else
      .... 
    }

Prediction should be returned for all PCs that might have a branch or a loop - so return a list of fetch_width predictions 
For update - update all PCs till the first taken branch

Order - 
1. Make all predictions.
2. Update in order - on a misprediction - //nuke AFTER the update for that branch  

b1 is taken - correctly predicted - update b1 - deallocate b2 from ftq w/o any updates - no nuke
b1 is taken - mispredicted - update b1 and nuke

b1 is not taken - correctly predicted
	
b1 is not taken - mispredicted

*/
#define DEBUG_B1_MISPREDICTS
#define DEBUG_B2_MISPREDICTS	
//#define DEBUG_NUKE

#include "branch_superbp_scratch.hpp"
  
bool bias;
uint32_t sign;
bool no_alloc = false; // Check ??
std::vector<bool> resolveDir;
bool b1_resolveDir, b2_resolveDir, j1_resolveDir;
std::vector<bool> predDir;
bool b1_predDir, b2_predDir, j1_predDir;
std::vector<uint32_t> correct_predictions, mispredictions;
uint32_t b1_correct_predictions = 0, b1_mispredictions = 0;
uint32_t b2_correct_predictions = 0, b2_mispredictions = 0;

for (int i = 0; i < 4096; i++)
{
  	/* For simplicity and portability of C code - resolve branches here */
  	//#define BASIC
  	#define COMPLEX
  	#ifdef BASIC
  	if (i < 4096) 
  	{
  		b1_resolveDir = false;
  		if (i < 2048) // b2 - NT
  			b2_resolveDir = false;
  		else // b2 - T
  			b2_resolveDir = true;
  	}
  	else
  	{
  		b1_resolveDir = true;
  	}
  	#elif defined COMPLEX
  	if ( (!(i%3)) || (!(i%5)) )    // Not taken if i is a multiple of 3
  	{
  		b1_resolveDir = false;
  		if ((!(i%7)) || (!(i%11))) // Not taken if i is a multiple of 4
  		{
  			b2_resolveDir = false;
  			j1_resolveDir = true;
  		}
  		else
  		{
  			b2_resolveDir = true;
  		} 
  	}
  	else
  	{
  		b1_resolveDir = true;
  	}
  	#endif 
  	
  	/* Begin fetch boundry -  Make both predictions - Allocate ftq entries for both - Call Fetch boundry end 
  	ftq entry allocation must be done immediately after prediction - before next prediction ****************/
  	// Begin fetch boundry
  	IMLI_inst.fetchBoundaryBegin(packet_PC);
  	
  	// Make both predictions 

  	if (is_ftq_full()) {printf("FTQ full, stall \n"); return;}
  	AddrType* targets = (AddrType*) malloc(fetch_width * sizeof(AddrType));
  	*(targets+1) = b1_Target;
  	*(targets+4) = b2_Target;
  	IMLI_inst.fetchPredict(packet_PC, fetch_width, predDir, bias, sign, targets, IMLI_inst);
  	b1_predDir = predDir[1];
  	b2_predDir = predDir[4];
  	j1_predDir = predDir[5];
  	
  	// Allocate ftq entries for both
  	//allocate_ftq_entry(packet_PC, 0, IMLI_inst);
  	//allocate_ftq_entry(packet_PC+1, b1_Target, IMLI_inst);
  	/*allocate_ftq_entry(packet_PC+2, 0, IMLI_inst);
  	allocate_ftq_entry(packet_PC+3, 0, IMLI_inst);*/
  	//allocate_ftq_entry(packet_PC+4, b2_Target, IMLI_inst);
  	//allocate_ftq_entry(packet_PC+5, j1_Target, IMLI_inst);
  	/*allocate_ftq_entry(packet_PC+6, 0, IMLI_inst);
  	allocate_ftq_entry(packet_PC+7, 0, IMLI_inst);*/
  	
  	// Call Fetch boundry end
  	IMLI_inst.fetchBoundaryEnd();  	
  	/* Begin fetch boundry -  Make all predictions - Allocate ftq entries for all - Call Fetch boundry end  - All done*/
  	
  	
  	/******************************** b1 not taken ***************************************/
  	if (b1_resolveDir == false) 
  	{
  		
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
  		// update b1 stats
  		if (b1_predDir == b1_resolveDir)
  		{
  			#ifdef MISPREDICTIONRATE_STATS_AFTER_TRAINING
  			if (i > TRAINING_LENGTH-1)
  			#endif
  			b1_correct_predictions++;
  			#ifdef DEBUG_B1_MISPREDICTS
  			printf ("b1 correctly predicted as %s for i = %d\n", b1_predDir ? "taken" : "not taken", i);
  			printf ("******************************************************************************\n");
  			#endif
  		}
  		else
  		{
  			#ifdef MISPREDICTIONRATE_STATS_AFTER_TRAINING
  			if (i > TRAINING_LENGTH-1)
  			#endif
  			b1_mispredictions++;
  			#ifdef DEBUG_B1_MISPREDICTS
  			printf ("b1 mispredicted as %s for i = %d\n", b1_predDir ? "taken" : "not taken", i);
  			printf ("******************************************************************************\n");
  			#endif
  		}
  		//update b1 predictor
  		IMLI_inst.updatePredictor(b1_PC, b1_resolveDir, b1_predDir, b1_Target, no_alloc);

  		/******** Read/deallocate ftq entry, update b1 predictor, update b1 stats done *******/	
  		
  		/* Read/deallocate ftq entry, update b2 predictor, update b2 stats*/
  		// Read/deallocate ftq entry
  		#ifdef CPP
  		ftq_inst.get_ftq_data(IMLI_inst);
  		#else
  		get_ftq_data(IMLI_inst);
  		#endif
  		#ifdef DEBUG_FTQ_ENTRY_ALLOC_DEALLOC_NUKE
  		printf ("ftq entry deallocated \n");
  		#endif
  		// update b2 stats
  		if (b2_predDir == b2_resolveDir)
  		{
  			#ifdef MISPREDICTIONRATE_STATS_AFTER_TRAINING
  			if (i > TRAINING_LENGTH-1)
  			#endif
  			b2_correct_predictions++;
  			#ifdef DEBUG_B2_MISPREDICTS
  			printf ("b2 correctly predicted as %s for i = %d\n", b2_predDir ? "taken" : "not taken", i);
  			printf ("******************************************************************************\n");
  			#endif
  		}
  		else
  		{
  			#ifdef MISPREDICTIONRATE_STATS_AFTER_TRAINING
  			if (i > TRAINING_LENGTH-1)
  			#endif
  			b2_mispredictions++;
  			#ifdef DEBUG_B2_MISPREDICTS
  			printf ("b2 mispredicted as %s for i = %d\n", b2_predDir ? "taken" : "not taken", i);
  			printf ("******************************************************************************\n");
  			#endif
  		}
  		// update b2 predictor
  		IMLI_inst.updatePredictor(b2_PC, b2_resolveDir, b2_predDir, b2_Target, no_alloc);

  		/* Resolve b2, Read/deallocate ftq entry, update b2 predictor, update b2 stats*/	
  	}
  	/******************************** b1 not taken over ***************************************/
  	
  	/******************************** b1 taken ***************************************/
  	else // b1_resolveDir == true;
  	{
  		
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
 		// update b1 stats
  		if (b1_predDir == b1_resolveDir)
  		{
  			#ifdef MISPREDICTIONRATE_STATS_AFTER_TRAINING
  			if (i > TRAINING_LENGTH-1)
  			#endif
  			b1_correct_predictions++;
  			#ifdef DEBUG_B1_MISPREDICTS
  			printf ("b1 correctly predicted as %s for i = %d\n", b1_predDir ? "taken" : "not taken", i);
  			printf ("******************************************************************************\n");
  			#endif
  		}
  		else
  		{
  			#ifdef MISPREDICTIONRATE_STATS_AFTER_TRAINING
  			if (i > TRAINING_LENGTH-1)
  			#endif
  			b1_mispredictions++;
  			#ifdef DEBUG_B1_MISPREDICTS
  			printf ("b1 mispredicted as %s for i = %d\n", b1_predDir ? "taken" : "not taken", i);
  			printf ("******************************************************************************\n");
  			#endif
  		}
  		//update b1 predictor
  		IMLI_inst.updatePredictor(b1_PC, b1_resolveDir, b1_predDir, b1_Target, no_alloc);
 
  		/******** Read/deallocate ftq entry, update b1 predictor, update b1 stats done *******/	
  		nuke();
  		#ifdef DEBUG_NUKE
  		printf ("nuked \n");
  		printf ("******************************************************************************\n");
  		#endif
  	}
  	/******************************** b1 taken over ***************************************/
  		
} // for (int i = 0; i < 8192; i++) over
  	
printf ("\n====================================================================================\n");
  printf ("b1_correct_predictions = %u, b1_mispredictions = %u, b1_misprediction_rate = %f\n", b1_correct_predictions, b1_mispredictions, (float)b1_mispredictions/(b1_correct_predictions + b1_mispredictions));
  printf ("b2_correct_predictions = %u, b2_mispredictions = %u, b2_misprediction_rate = %f\n", b2_correct_predictions, b2_mispredictions, (float)b2_mispredictions/(b2_correct_predictions + b2_mispredictions));
  printf ("====================================================================================\n");

