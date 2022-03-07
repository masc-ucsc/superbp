
#include "imli.hpp"

#include <string>
#include "fmt/format.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "inst_opcode.hpp"

//#define DEBUG_PRINTS

#define MISPREDICTIONRATE_STATS_AFTER_TRAINING
#ifdef MISPREDICTIONRATE_STATS_AFTER_TRAINING
#define TRAINING_LENGTH 512
#endif

// Tests
//#define FOR_IN_FOR_ROUGH
// Bad example
//#define FOR_IN_FOR_PROPER
#define IF_ELSE_IN_IF_ELSE

namespace {

class Gold_test : public ::testing::Test {
protected:
  void SetUp() override {
}

  void TearDown() override {
    // Graph_library::sync_all();
  }	

  //IMLI IMLI_inst(24, 3, 3, 6, true);
   //IMLI IMLI_inst;

};

/*
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

TEST_F(Gold_test, Trivial_Loop_entry_test1) {

  Loop_entry Loop_entry_inst;
  EXPECT_EQ (Loop_entry_inst.confid, 0 ) ;
  EXPECT_EQ(Loop_entry_inst.CurrentIter, 0);
  EXPECT_EQ(Loop_entry_inst.NbIter, 0);
  EXPECT_EQ (Loop_entry_inst.TAG, 0 );
  EXPECT_EQ(Loop_entry_inst.age, 0 );
  EXPECT_EQ (Loop_entry_inst.dir, false);

}
*/

/*
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
*/


/*
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
*/

TEST_F(Gold_test, Trivial_IMLI_test) {

  int blogb = 10, log2fetchwidth = 3, bwidth = 3, nhist = 6;
  bool sc = false;
  IMLI IMLI_inst( blogb, log2fetchwidth, bwidth, nhist, sc);
  
#ifdef FOR_IN_FOR_ROUGH
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
#endif

#ifdef FOR_IN_FOR_PROPER
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
#endif

#ifdef IF_ELSE_IN_IF_ELSE
/*
  if (i > 4096)            	// b1 - pc = 0x05050a0a, taken target = 0x05050b0b - 2048 occurences
  	{
      if (i > 6144)       	// b2 - pc = 0x05050a0c, taken target = 0x05050ae0 - 2/3 *2048 occurences
      ....
      else             	// pc = 0x05050ae0,
      .... 
    }
  else                 	// pc = 0x05050b0b,
    {
      if (i > 2048)       	// b3 - pc = 0x05050b0f, taken target = 0x05050bf0 - 1/3 *2048 occurences
      ....
      else if (i > 1024)	// b4 - pc = 0x05050bf0, taken target = 0x05050c0b - 1/5 * 1/3 *2048 occurences
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
  bool no_alloc = true; // Check ??
  bool b1_resolveDir, b2_resolveDir, b3_resolveDir, b4_resolveDir;
  bool b1_predDir, b2_predDir, b3_predDir, b4_predDir;
  uint32_t b1_correct_predicitons = 0, b1_mispredictions = 0;
  uint32_t b2_correct_predicitons = 0, b2_mispredictions = 0;
  uint32_t b3_correct_predicitons = 0, b3_mispredictions = 0;
  uint32_t b4_correct_predicitons = 0, b4_mispredictions = 0;
  
  
  for (int i = 0; i < 8192; i++)
  {
  	b1_predDir = IMLI_inst.getPrediction(b1_PC, bias,  sign);
  	#ifdef DEBUG_PRINTS
  	printf ("b1 Prediction = \t\t%32s \n", b1_predDir ? "predicted taken" : "predicted not taken");
  	#endif
  	if (i > 4096)
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
  		b2_predDir = IMLI_inst.getPrediction(b2_PC, bias,  sign);
  		#ifdef DEBUG_PRINTS
  		printf ("b2 Prediction = \t\t%32s \n", b2_predDir ? "predicted taken" : "predicted not taken");
  		#endif
  		if (i > 6144)
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
  		b3_predDir = IMLI_inst.getPrediction(b3_PC, bias,  sign);
  		#ifdef DEBUG_PRINTS
  		printf ("b3 Prediction = \t\t%32s \n", b3_predDir ? "predicted taken" : "predicted not taken");
  		#endif
  		if (i > 2048)
  			{
  				b3_resolveDir = false;
  			}
  		else
  			{
  				b3_resolveDir = true;  
  				
  				b4_predDir = IMLI_inst.getPrediction(b4_PC, bias,  sign);
  				if (i > 1024)
  					b4_resolveDir = false;
  				else
  					b4_resolveDir = true; 
  					
  				printf ("b4 Prediction = \t\t%32s \n", b4_predDir ? "predicted taken" : "predicted not taken");
  				printf ("b4 Actual outcome = \t%32s \n", b4_resolveDir ? "taken" : "not taken");
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
  		#ifdef DEBUG_PRINTS
  		printf ("b3 Actual outcome = \t%32s \n", b3_resolveDir ? "taken" : "not taken");
  		#endif
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
  	}
  } // end of for

#ifdef DEBUG_PRINTS
  printf ("b1_correct_predicitons = %u, b1_mispredictions = %u, b1_misprediction_rate = %f\n", b1_correct_predicitons, b1_mispredictions, (float)b1_mispredictions/(b1_correct_predicitons + b1_mispredictions));
  printf ("b2_correct_predicitons = %u, b2_mispredictions = %u, b2_misprediction_rate = %f\n", b2_correct_predicitons, b2_mispredictions, (float)b2_mispredictions/(b2_correct_predicitons + b2_mispredictions));
  printf ("b3_correct_predicitons = %u, b3_mispredictions = %u, b3_misprediction_rate = %f\n", b3_correct_predicitons, b3_mispredictions, (float)b3_mispredictions/(b3_correct_predicitons + b3_mispredictions));
#endif // DEBUG_PRINTS
  printf ("b4_correct_predicitons = %u, b4_mispredictions = %u, b4_misprediction_rate = %f\n", b4_correct_predicitons, b4_mispredictions, (float)b4_mispredictions/(b4_correct_predicitons + b4_mispredictions));
#endif  // IF_ELSE_IN_IF_ELSE

} // TEST_F

} // namespace
