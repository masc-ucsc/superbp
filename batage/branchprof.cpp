#include "utils.hpp"
#include "branchprof.hpp"
#include "dromajo.h"
//#include "emulator.hpp"
#include "predictor.hpp"
#include <iostream>

#include <map>

#include "ftq.hpp"
#include "huq.hpp"

// key = fetchPC and value = num_mispredictions
std::map<uint64_t, uint64_t> fetchPC_Map;
uint64_t temp;

//#define DEBUG_GSHARE
//#define DEBUG_DETAIL
//#define T_TRACE
//PREDICTOR bp;
/*
bool predDir, last_predDir, resolveDir, last_resolveDir;
uint64_t branchTarget;
*/

uint8_t partial_pop;
uint32_t update_gshare_index, update_gshare_tag;

#ifdef GSHARE
// gshare allocate
gshare_prediction gshare_pred_inst, last_gshare_pred_inst;
bool gshare_pos1_correct, gshare_pos0_correct, gshare_prediction_correct;
uint8_t highconf_ctr = 0;
vector <uint64_t> gshare_PCs;
vector <uint8_t> gshare_poses;
uint64_t num_gshare_allocations, num_gshare_predictions, num_gshare_correct_predictions, gshare_batage_1st_pred_mismatch, gshare_batage_2nd_pred_mismatch,  num_gshare_jump_correct_predictions, num_gshare_jump_mispredictions, num_gshare_tag_match; 
	int gshare_index;
	int gshare_tag;	
#endif

#if (defined (GSHARE) || defined (Ideal_2T))
	bool gshare_tracking = false;
	bool highconfT_in_packet = false;
#endif // GSHARE || Ideal_2T

#ifdef Ideal_2T
	uint8_t pos_0;
#endif // Ideal_2T


FILE *pc_trace;

#ifdef DEBUG_GSHARE
FILE *debug_gshare;
#endif
#ifdef T_TRACE
FILE *t_trace;
#endif
#ifdef GSHARE_TRACE
FILE *gshare_trace;
#endif
#ifdef DEBUG_DETAIL
FILE *details;
#endif

#ifdef T_TRACE
uint32_t i_count;
uint64_t PC1 = 0, PC2 = 0;
bool PC1_branch = false, PC2_branch = false; // true means PC1 is a branch and PC2 may be a jump
#endif

#define EN_BB_FB_COUNT
#ifdef EN_BB_FB_COUNT
#define MAX_BB_SIZE 50
#define MAX_FB_SIZE 250
uint64_t bb_size[MAX_BB_SIZE], fb_size[MAX_FB_SIZE];
uint64_t sum_bb_size, sum_fb_size;
uint32_t bb_count, fb_count;
uint8_t running_bb_size, running_fb_size;
#endif // EN_BB_FB_COUNT

#ifdef SUPERSCALAR
#ifdef FTQ
#include "ftq.hpp"
#include "huq.hpp"
ftq_entry ftq_data; // Only 1 instance - assuming the updates for superscalar
                    // will be done 1 by 1, so they may reuse the same instance
huq_entry huq_data;

#endif // FTQ
#endif // SUPERSCALAR

//extern uint64_t instruction_count;    // total # of instructions - including skipped and benchmark instructions
/*
uint64_t correct_prediction_count, misprediction_count;

uint64_t benchmark_instruction_count; // total # of benchmarked instructions
                                      // only - excluding skip instructions
                                      
uint64_t num_fetches;
                                      
uint64_t fetch_pc, last_fetch_pc;
*/

/*
* missPredict per branch
* missPredict per Control Flow
* missControl per MPKI (predicted taken when it was not a control)
* BB size
* FetchBlock Size
counters:
total number of instructions
number of branches
number of control flow instructions
number of mispredicts - separate for branches and jumps
number of taken branches
number of taken control flow instructions
*/
uint64_t branch_count, branch_mispredict_count, jump_count, cti_count,
    misscontrol_count, taken_branch_count;

/* 
index_from_aligned_fetch_pc is based on the aligned PC, and not on whether the pc is the target of a branch
so for a branch into a packet to index != 0, index_into_packet is != 0
*/
//static uint64_t aligned_fetch_pc, last_aligned_fetch_pc;
//static uint16_t index_from_aligned_fetch_pc, last_index_from_aligned_fetch_pc; // starts from pc - (alignedPC) after every redirect
/*
static uint64_t last_pc;
static uint32_t last_insn_raw;
static insn_t last_insn, insn;
static bool last_misprediction, misprediction;
bool i0_done = false;
*/
prediction batage_prediction;

#ifdef EN_BB_FB_COUNT
static uint8_t bb_over = 0, fb_over = 0;
#endif // EN_BB_FB_COUNT
#ifdef SUPERSCALAR
//static int16_t inst_index_in_fetch = 0, last_inst_index_in_fetch; // starts from 0 after every redirect
#endif

extern uint64_t maxinsns, skip_insns;

 branchprof::branchprof(ftq* f, huq* h, batage* b, PREDICTOR* bp_ptr)
{
	ftq_p = f;
	huq_p = h;
	pred = b;
	bp = bp_ptr;
	//FETCH_WIDTH = b->FETCHWIDTH;
	//fprintf(stderr, "Received BATAGE:FETCHWIDTH = %d\n", "FETCHWIDTH\n");
	//fprintf(stderr, "FETCH_WIDTH = %d\n", "FETCH_WIDTH\n");
}

void branchprof::branchprof_init(char* bp_logfile) {
  // PREDICTOR bp;
  FETCH_WIDTH = pred->FETCHWIDTH;
  fprintf(stderr, "Received BATAGE:FETCHWIDTH = %d\n", pred->FETCHWIDTH);
  fprintf(stderr, "FETCH_WIDTH = %d\n", FETCH_WIDTH);
  
  pc_trace = fopen(bp_logfile, "w");
  #ifdef DEBUG_GSHARE
debug_gshare = fopen("debug_gshare.txt", "w");
#endif
  #ifdef T_TRACE
  t_trace = fopen("t_trace.txt", "w");
  #endif
  #ifdef GSHARE_TRACE
  gshare_trace = fopen("gshare_logfile.txt", "w");
  #endif
  #ifdef DEBUG_DETAIL
details = fopen("details.txt", "w");
#endif
  if (pc_trace == nullptr) {	
    fprintf(stderr,
            "\nerror: could not open %s for dumping trace\n", bp_logfile);
    //exit(-3);
  } else {
    fprintf(stderr, "\nOpened dromajo_simpoint.bb for dumping trace\n");
    /*fprintf(pc_trace,
            "FETCH_WIDTH = %u, SKIP_COUNT = %lu, benchmark_count = %lu \n",
            FETCH_WIDTH, skip_insns, maxinsns);*/
#ifdef PC_TRACE
    fprintf(pc_trace, "%20s\t\t|%20s\t|%32s\n", "PC", "Instruction",
            "Instructiontype");
#endif // PC_TRACE
  }
#ifdef FTQ
  //std::cerr << "NUM_FTQ_ENTRIES = " << NUM_FTQ_ENTRIES << "\n\n";
#endif // FTQ
	//bp->pred.read_env_variables();
	//bp->pred.populate_dependent_globals();
	//bp->pred.batage_resize();
	//bp->hist.get_predictor_vars(&(bp->pred));
  return;
}

void branchprof::branchprof_exit(PREDICTOR* bp) {
//void branchprof_exit(char* bp_logfile) {
 // pc_trace = fopen(bp_logfile, "w");
  fprintf(pc_trace,
          "branch_count = %lu\njump_count = %lu\ncti_count = "
          "%lu\nbenchmark_instruction_count = %lu\n/*Instruction Count = "
          "%lu\n*/Correct prediciton Count = %lu\nmisprediction count = "
          "%lu\nbranch_mispredict_count=%lu\nmisscontrol_count=%"
          "lu\nbranch misprediction rate = %lf\nMPKI = %lf\nAverage Fetch Width = %lf\n",
          branch_count, jump_count, cti_count, benchmark_instruction_count,
          /*instruction_count,*/ correct_prediction_count, misprediction_count,
          branch_mispredict_count, misscontrol_count,
          (double)(branch_mispredict_count) /(double)(branch_count) * 100,
          (double)(branch_mispredict_count) / (double)(benchmark_instruction_count) *
              1000, ((double)benchmark_instruction_count/num_fetches) );
		#ifdef GSHARE
              fprintf (pc_trace, "gshare local rates - \ngshare_num_allocations = %llu\nnum_gshare_tag_match = %llu\ngshare_num_predictions = %llu\ngshare_num_correct_predictions = %llu\ngshare_num_jump_correct_predictions = %llu\ngshare_num_jump_mispredictions = %llu\ngshare_misprediction_rate = %lf%\ngshare_batage_1st_pred_mismatch = %llu\ngshare_batage_2nd_pred_mismatch = %llu\n",num_gshare_allocations, num_gshare_tag_match, num_gshare_predictions, num_gshare_correct_predictions, num_gshare_jump_correct_predictions, num_gshare_jump_mispredictions, ((double)(num_gshare_predictions-num_gshare_correct_predictions)*100/num_gshare_predictions), gshare_batage_1st_pred_mismatch, gshare_batage_2nd_pred_mismatch );
              
              auto firstEntry = *fetchPC_Map.begin();
              fprintf (pc_trace, "max mispredictions for fetchpc = %#llx are %llu\n", firstEntry.first, firstEntry.second);
    #ifdef DEBUG_GSHARE_UTILIZATION
		uint64_t gshare_util_info[3];
		bp->fast_pred.get_gshare_utilization(gshare_util_info);
		fprintf (pc_trace, "gshare num_unallocated = %u\ngshare_num_collisions_blocked = %lu\ngshare_num_collisions_replaced = %lu\n", gshare_util_info[0] , gshare_util_info[1], gshare_util_info[2]);
	#endif
		#endif // GSHARE              
for (int i = 0; i < bp->pred.SBP_NUMG; i++)
              {
              	fprintf(pc_trace, "Allocations on Table %d = %u\n", i, bp->get_allocs(i));
              }
#ifdef EN_BB_FB_COUNT
  fprintf(pc_trace, "bb_size = \n");
  for (int i = 0; i < MAX_BB_SIZE; i++) {
    fprintf(pc_trace, "%lu, ", bb_size[i]);
  }
  fprintf(pc_trace, "\n");
  fprintf(pc_trace, "avg_bb_size = %lf \n", (double)sum_bb_size / bb_count);

  fprintf(pc_trace, "fb_size = \n");
  for (int i = 0; i < MAX_FB_SIZE; i++) {
    fprintf(pc_trace, "%lu, ", fb_size[i]);
  }
  fprintf(pc_trace, "\n");
  fprintf(pc_trace, "avg_fb_size = %lf \n", (double)sum_fb_size / fb_count);
#endif // EN_BB_FB_COUNT

  fclose(pc_trace);
    #ifdef DEBUG_GSHARE
	fclose (debug_gshare);
	#endif
  #ifdef T_TRACE
  fclose (t_trace);
  #endif
  #ifdef GSHARE_TRACE
  fclose(gshare_trace);
  #endif
    #ifdef DEBUG_DETAIL
fclose(details);
#endif
  return;
}

void branchprof::fetchBoundaryBegin(uint64_t pc)
{
	num_fetches++;
  	#ifdef GSHARE
  	last_gshare_pred_inst = gshare_pred_inst;
  	#endif // GSHARE
    last_fetch_pc = fetch_pc;
	
  	fetch_pc = pc;
  	bp->ftq_inst.set_ftq_index (inst_index_in_fetch);
  	
  	  	uint64_t temp_pc = fetch_pc;
  	
  	// temp_predDir = bp->GetPrediction(temp_pc);
  	std::vector<bool> vec_predDir(FETCH_WIDTH, false);
  	std::vector<bool> vec_highconf(FETCH_WIDTH, false);
  	
  	#ifdef DEBUG
    {
      std::cerr << "getting predictions" << "\n";
    }
    #endif // DEBUG
    
	//vec_predDir = std::move(bp->GetPrediction(temp_pc));
	batage_prediction = bp->GetPrediction(fetch_pc);
	vec_predDir =  batage_prediction.prediction_vector;
	vec_highconf = batage_prediction.highconf;	

    	#ifdef DEBUG_DETAIL
    	for (int i =0; i < FETCH_WIDTH; i++)
    	{
			if ( vec_predDir[i] && vec_highconf[i]) 
			{
				//fprintf (details, "fetchpc = %llx has a high conf branch at pos = %u \n", fetch_pc, i);
			}
		}
    	#endif
    	
    #ifdef GSHARE
	gshare_index = batage_prediction.gshare_index;
	gshare_tag = batage_prediction.gshare_tag;
	
	#ifdef DEBUG_GSHARE2
	//printf ("gshare_index = %u and gshare_tag = %x\n", gshare_index, gshare_tag);
	#endif // DEBUG_GSHARE
 	get_gshare_prediction (fetch_pc, gshare_index, gshare_tag);
 	
   	if (gshare_pred_inst.hit)
 	{
 		int pos0 = gshare_pred_inst.info.poses[0];
 		if (!vec_predDir[pos0])
 		{
 			/*gshare_pred_inst.tag_match = false;
 			gshare_pred_inst.hit = false;*/
 			gshare_batage_1st_pred_mismatch++;
 		}
 	}
 		
 		 if (gshare_pred_inst.tag_match)
   		{
   			num_gshare_tag_match++;
   			if (gshare_pred_inst.hit)
    		{
    			num_gshare_predictions++;
    		}
   		}
   		
 			#ifdef GSHARE_TRACE
			fprintf (gshare_trace, "fetchPC = %llx predicted from index = %u with tag = %x and hit = %u\n", fetch_pc, gshare_index, gshare_tag, gshare_pred_inst.hit);
			#endif
	#endif  // GSHARE

	#ifdef DEBUG
    	{
      		std::cerr << "got predictions" << "\n";
    	}
	#endif // DEBUG
	
	//fprintf(stderr, "FETCH_WIDTH = %d\n", FETCH_WIDTH);
  	for (int i = 0; i < FETCH_WIDTH; i++)
  	{
  		/*
  		Not required with dromajo, will be needed with desesc
  		if (i == 0)
  		{
  			if ( gftq.is_ftq_full() == false )
  			{
  				gftq.allocate_ftq_entry(gshare_pred_inst);
  			}
  		}
  		*/
  		// Allocate
  		//fprintf(stderr, "%s\n", "Checking FTQ full\n");
  		if (bp->ftq_inst.is_ftq_full() == false) {
  			// Always allocate with default info - i.e. a non_cti 
  			// At this point if 1st instruction for which correct info is avail is non-cti it is already updated
  			// If it is cti - then branchtarget is avail in next CC and hence should only be updated condirionally then
  			 //fprintf(stderr, "%s\n", "Requesting FTQ entry allocation\n");
    		bp->ftq_inst.allocate_ftq_entry(vec_predDir[i], vec_highconf[i], false, temp_pc, insn_t::non_cti, temp_pc+2, fetch_pc);
    	} else {
      fprintf(stderr, "%s\n", "FTQ full");
    	}
  		// Repeat for next sequential instruction
  		temp_pc+=2;
  	}
#ifdef DEBUG
    {
      std::cerr << "FTQ allocarions done" << "\n";
    }
#endif // DEBUG  
  	return;
}

void branchprof::fetchBoundaryEnd()
{
	uint64_t update_pc, update_fetch_pc, update_branchTarget;
  insn_t update_insn;
  bool update_predDir, update_resolveDir;
  bool update_highconf;
  
  #ifdef DEBUG_FTQ
  fprintf (stderr, "\nread_ftq_update_predictor : inst_index_in_fetch = %u, partial_pop = %d\n", inst_index_in_fetch, partial_pop);
  #endif

#if (defined (GSHARE) || defined (Ideal_2T))
	highconfT_in_packet = false;
#endif // GSHARE 

    for (int i = 0; i < (partial_pop ? inst_index_in_fetch : FETCH_WIDTH); i++) 
    {
      if (!(bp->ftq_inst.is_ftq_empty()))
       {
        bp->ftq_inst.get_ftq_data(&ftq_data);

        update_pc = ftq_data.pc;
        update_insn = ftq_data.insn;
        update_resolveDir = ftq_data.resolveDir;
        update_predDir = ftq_data.predDir;
        update_branchTarget = ftq_data.branchTarget;
        update_fetch_pc = ftq_data.fetch_pc;

#ifdef Ideal_2T
if ( ( update_resolveDir && update_predDir ) ||  (update_insn == insn_t::jump) /*||  (update_insn == insn_t::ret)*/)
{	
	highconfT_in_packet = true;
	num_fetches--;

	if (!gshare_tracking && (update_insn == insn_t::branch)) // The first must be a branch
	{
		pos_0  = i;
		gshare_tracking = true;
	}
	else
	{
		if (pos_0 + i + 1 < FETCH_WIDTH)
		{
			//num_fetches--;
		}
		gshare_tracking = false;
		pos_0 = 100;
	}
}
#endif // Ideal_2T

	#ifdef GSHARE
       	resolve_gshare(i, update_predDir, update_resolveDir, update_branchTarget);
       	
       	if (last_gshare_pred_inst.hit && (i == last_gshare_pred_inst.info.poses[1]) && (update_insn == insn_t::jump))
       	{
       		if (gshare_pos1_correct)
       			{num_gshare_jump_correct_predictions++;}
       		else
       			{num_gshare_jump_mispredictions++;}
       	}
       	
        // allocate_gshare
        update_highconf = ftq_data.highconf;
        // TODO Check
        //update_gshare_index = last_gshare_pred_inst.index; // ftq_data.gi[1];

	// allocate on a branch or if second is a jump
	// TODO Check
	if ( ((update_insn == insn_t::branch) && update_resolveDir && update_predDir && update_highconf) || (update_insn == insn_t::jump) /* || (update_insn == insn_t::ret) */)
	{
		highconfT_in_packet = true;
		if (!gshare_tracking) // The first must be a branch
		{
			if (!last_gshare_pred_inst.tag_match && !gshare_pred_inst.tag_match && (update_insn == insn_t::branch)) // The first must be a branch
			{
				gshare_PCs.push_back(update_fetch_pc);
				gshare_poses.push_back( i ); // (update_pc - update_fetch_pc) >> 1
				gshare_PCs.push_back(update_branchTarget);
				update_gshare_tag = batage_prediction.gshare_tag; //ftq_data.tags[i][ftq_data.bp[i]];
				update_gshare_index = batage_prediction.gshare_index;
				gshare_tracking = true;
			}
		}
		else
		{
			if (gshare_poses[0] + i + 1 < FETCH_WIDTH)
			{
				gshare_poses.push_back( ( i )); // (update_pc - update_fetch_pc) >> 1)
				gshare_PCs.push_back(update_branchTarget);
			
				bp->fast_pred.allocate(gshare_PCs, gshare_poses, update_gshare_index, update_gshare_tag);
				#ifdef GSHARE_TRACE
				fprintf (gshare_trace, "fetchPC = %llx allocated at index = %u with tag = %x \n", gshare_PCs[0], update_gshare_index, update_gshare_tag);
				#endif
				num_gshare_allocations++;
			}
			// TODO - do we handle if there are 3 high conf T in 3 consecutive packets 
			gshare_tracking = false;
			gshare_PCs.clear();
			gshare_poses.clear();
		}
	}
	// allocate gshare done
	#endif
	
        // Store the read predictor fields into the predictor
        copy_ftq_data_to_predictor(bp, &ftq_data);
		
		#ifdef DEBUG_FTQ
  		fprintf (stderr, "Updating predictor tables for pc = %llx with resolvdir = %d, i = %d \n", update_pc, update_resolveDir, i);
  		#endif
  		
  if ((update_insn != insn_t::jump)  && (update_insn != insn_t::ret) )
  {
	#ifdef GSHARE
	// TODO Check if number of updates is correct
	if (!gshare_prediction_correct)
	#endif
        {
		bp->Updatetables(update_pc, update_fetch_pc, i, update_resolveDir);
		}
  }
  #ifdef DEBUG_FTQ
  fprintf (stderr, "Update predictor tables done for pc = %llx \n", update_pc);
  #endif
        if (update_insn != insn_t::non_cti) {
			  #ifdef DEBUG_HUQ
  			fprintf (stderr, "Allocate huq entry for pc = %llx, target = %llx, resolvedir = %d \n", update_pc, update_branchTarget, update_resolveDir);
  			#endif
          bp->huq_inst.allocate_huq_entry(/*update_pc,*/ update_branchTarget,
                             update_resolveDir);
        }
        /*else if(update_insn == insn_t::jump)
        {
                allocate_huq_entry(update_branchTarget, true);
                //bp->TrackOtherInst(update_pc, bool branchDir,
        update_branchTarget);
        }*/

        //inst_index_in_fetch--; // i is being incremented, DO NOT decrement this
      } else {
        fprintf(stderr,
                "Pop on empty ftq, inst_index_in_fetch = %d, misprediction = %d, resolveDir = %d \n", inst_index_in_fetch, misprediction, resolveDir);
      }
    }
    
#ifdef Ideal_2T
if (!highconfT_in_packet)
{
	gshare_tracking = false;
	pos_0 = 100;
}
#endif // Ideal_2T
    
 	#ifdef GSHARE   
    if (!highconfT_in_packet)
    {
    	gshare_tracking = false;
    	gshare_PCs.clear();
	    gshare_poses.clear();
    }
	    	
    	if (last_gshare_pred_inst.hit)
        {
        	if (gshare_prediction_correct)
        	{
        		num_gshare_correct_predictions++;
        		num_fetches--;
        	}
        	else
        	{
        		(fetchPC_Map[last_fetch_pc])++;
        		#ifdef DEBUG_GSHARE
        		fprintf (debug_gshare, " *** gshare mispredicted for fetchPC = %llx *** \n", last_fetch_pc);
        		#endif // DEBUG_GSHARE
        	}
        }

        // TODO Check if consecutive tag_matches are handled correctly 
        if (last_gshare_pred_inst.tag_match)
        {
        	//if (gshare_pos0_correct)
        	{bp->fast_pred.update(last_gshare_pred_inst, gshare_prediction_correct);}
    		gshare_pos1_correct = false;
    		gshare_prediction_correct = false;
        }
        if (!gshare_pred_inst.tag_match)
        {
			gshare_pos0_correct = false;
		}
	#endif
    
    inst_index_in_fetch = 0;
    #ifdef DEBUG
  	fprintf (stderr, "||||||||||||||||||||||||||| NUKING FTQ |||||||||||||||||||||||||| \n");
  	#endif
  	// TODO - Check if nuke necessary/ correct
    bp->ftq_inst.nuke_ftq();
    #ifdef DEBUG
  	fprintf (stderr, "Popping huq, Updating histories \n");
  	#endif
    while (!(bp->huq_inst.is_huq_empty())) {
      bp->huq_inst.get_huq_data(&huq_data);
      	#ifdef DEBUG
  			fprintf (stderr, "Updating history for target = %llx, resolvedir = %d \n", huq_data.branchTarget, huq_data.resolveDir);
  			#endif
      bp->Updatehistory(huq_data.resolveDir, huq_data.branchTarget);
    }
    #ifdef DEBUG
  	fprintf (stderr, "Update histories done \n");
  	#endif
    /*last_pc = update_pc;
    last_resolveDir = update_resolveDir;
    last_predDir = update_predDir;*/
    // branchTarget = update_branchTarget;
#ifdef DEBUG
    {
      std::cerr << "After deallocations over - inst_index_in_fetch = "
                << inst_index_in_fetch << "\n";
    }
#endif // DEBUG_FTQ
return;
}

//#ifdef FTQ
void branchprof::copy_ftq_data_to_predictor(PREDICTOR* bp, ftq_entry *ftq_data_ptr) {
  bp->pred.hit = ftq_data_ptr->hit;
  bp->pred.tags = ftq_data_ptr->tags;
  bp->pred.s = ftq_data_ptr->s;
  bp->pred.poses = ftq_data_ptr->poses;
  bp->pred.meta = ftq_data_ptr->meta;
  bp->pred.bp = ftq_data_ptr->bp;
  bp->pred.bi = ftq_data_ptr->bi;
  bp->pred.bi2 = ftq_data_ptr->bi2;
  bp->pred.b_bi = ftq_data_ptr->b_bi;
  bp->pred.b2_bi2 = ftq_data_ptr->b2_bi2;
  memcpy(bp->pred.gi, &((ftq_data_ptr->gi)[0]), sizeof(int) * bp->pred.SBP_NUMG);
}
//#endif // FTQ

void branchprof::update_counters_pc_minus_1_branch() {
  // branch is resolved in next CC, so match last_resolveDir with last_predDir
  if (last_predDir == last_resolveDir) {
    last_misprediction = false;
    correct_prediction_count++;
    #ifdef DEBUG
  	fprintf (stderr, "Branch correctly predicted \n");
  	#endif
  } else {
    last_misprediction = true;
    misprediction_count++;     // Both mispredicted T and NT
    branch_mispredict_count++; // Both mispredicted T and NT
    #ifdef DEBUG
  	fprintf (stderr, "Branch Mispredicted \n");
  	#endif
  }
}

/*
gshare allocation -
if no gshare_tracking is in progress - start gshare_tracking - push to PCs[0]
If gshare_tracking is in progress - 
	1. if found a highconf taken branch - 
	push to gshare PCs and poses
	if the ctr reaches desired value - allocate on gshare and then clear the local buffer
	if the ctr is not at desired value - do nothing
	2. If not a high conf taken - deallocate and clear local gshare_tracking buffer
*/

#ifdef GSHARE
void branchprof::resolve_gshare(int i, bool update_predDir, bool update_resolveDir, uint64_t target)
{
	if (last_gshare_pred_inst.tag_match)
        {
        	if (i == 0)
        		{gshare_pos1_correct = false;}
             if (  i == last_gshare_pred_inst.info.poses[1]) 
        	{
				if ( last_gshare_pred_inst.info.PCs[1] == target)
        		{gshare_pos1_correct = true;}     	
			}
			gshare_prediction_correct =  gshare_pos1_correct; 
        }

        else if (gshare_pred_inst.tag_match)
        {
        	if (i == 0) 
        		{gshare_pos0_correct = false;}
        		
        	if (update_resolveDir == update_predDir) 
        	{
        		if ( update_predDir )
        		{
        			if ( gshare_pred_inst.info.PCs[0] == target)
        			{
        				gshare_pos0_correct = true;
        				//num_fetches--;
        			}
        			else
        			{
        				gshare_pos0_correct = false;
        				gshare_pred_inst.tag_match = false;
        				if (gshare_pred_inst.hit)
        				{
        					gshare_pred_inst.hit = false;
        					num_gshare_predictions--;
        				}
        			}
        		}
        	}
        	else 
        	{
        		gshare_pos0_correct = false;
        		gshare_pred_inst.tag_match = false;
        		if (gshare_pred_inst.hit)
        		{
        			gshare_pred_inst.hit = false;
        			num_gshare_predictions--;
        		}
        	}	
        }
}
#endif // GSHARE

#ifdef FTQ
 void branchprof::read_ftq_update_predictor() {
  partial_pop = (last_misprediction || last_resolveDir);
	// For multiple taken predictions, changed from FETCH_WIDTH to INFO_PER_ENTRY 
  if ((inst_index_in_fetch == FETCH_WIDTH) || partial_pop) 																																																																																									{
  fetchBoundaryEnd();  
  }
}
#endif // FTQ

void branchprof::resolve_pc_minus_1_branch(uint64_t pc) {
  if (pc - last_pc == 4) {
    last_resolveDir = false;
#ifdef DEBUG
    fprintf(pc_trace, "%32s\n", "Not Taken Branch");
    fprintf(stderr, "%llx is %32s\n", last_pc, "Not Taken Branch");
#endif
  } else {
    last_resolveDir = true;
    taken_branch_count++;
#ifdef EN_BB_FB_COUNT
    fb_over = 1;
#endif // EN_BB_FB_COUNT
#ifdef T_TRACE
PC2 = last_pc;
PC2_branch = true;
fprintf (t_trace, "PC1 = %llx, PC2 = %llx, i_count = %u\n", PC1, PC2, i_count);
 i_count = 0;
 PC1 = PC2;
 PC1_branch = PC2_branch;
#endif
#ifdef DEBUG
    fprintf(pc_trace, "%32s\n", "Taken Branch");
    fprintf(stderr, "%llx is %32s\n", last_pc, "Taken Branch");
#endif
  }

  //fprintf(stderr, "target pc=%llx last_pc (branch pc) =%llx diff=%d pred:%d resolv:%d\n", pc, last_pc, pc - last_pc, last_predDir, last_resolveDir);
          
  update_counters_pc_minus_1_branch();
  return;
}

// Not being used
void branchprof::close_pc_minus_1_branch(uint64_t pc) {
#ifdef FTQ
  //read_ftq_update_predictor();
#endif
  return;
}

void branchprof::close_pc_jump(uint64_t pc, uint32_t insn_raw) {
  jump_count++;
  cti_count++;
  resolveDir = true;

#ifdef EN_BB_FB_COUNT
  bb_over = 1;
  fb_over = 1;
#endif // EN_BB_FB_COUNT

#ifdef DEBUG
fprintf(stderr, "%llx is %32s\n", pc, "Jump");
#endif

#ifdef PC_TRACE
  if ((((insn_raw & 0xffffff80) == 0x0))) // ECall
  {
    fprintf(pc_trace, "%32s\n", "ECALL type");
    //fprintf(stderr, "%llx is %32s\n", "ECALL type");
  } else if ((insn_raw == 0x100073) || (insn_raw == 0x200073) ||
             (insn_raw == 0x30200073) || (insn_raw == 0x7b200073)) // EReturn
  {
    fprintf(pc_trace, "%32s\n", "ERET type");
    //fprintf(stderr, "%llx is %32s\n", "ERET type");
  }

  // Jump
  else if ((insn_raw & 0xf) == 0x7) {
    if (((insn_raw & 0xf80) >> 7) == 0x0) {
      fprintf(pc_trace, "%32s\n", "Return");
      //fprintf(stderr, "%llx is %32s\n", "Return");
    } else {
      fprintf(pc_trace, "%32s\n", "Reg based Fxn Call");
      //fprintf(stderr, "%llx is %32s\n", "Reg based Fxn Call");
    }
  } else {
    fprintf(pc_trace, "%32s\n", "PC relative Fxn Call");
    //fprintf(stderr, "%llx is %32s\n", "PC relative Fxn Call");
  }
#endif
}

void branchprof::start_pc_branch() {
  branch_count++;
  cti_count++;
#ifdef EN_BB_FB_COUNT
  bb_over = 1;
#endif // EN_BB_FB_COUNT
}

void branchprof::close_pc_non_cti() {
  resolveDir = false;
#ifdef PC_TRACE
  fprintf(pc_trace, "%32s\n", "Non - CTI");
  fprintf(stderr, "%32s\n", "Non - CTI");
#endif
																																																																																																																																																																																																									}

																																																																																																																																																																																																									void branchprof::print_pc_insn(uint64_t pc, uint32_t insn_raw) {
  fprintf(pc_trace, "%20lx\t|%20x\t", pc, insn_raw);
  fprintf(stderr, "Fetched %d|\t%20lx\t|%20x\t", inst_index_in_fetch, pc, insn_raw);
  if (insn_raw < 0x100) {
    fprintf(pc_trace, "\t|");
  } else {
    fprintf(pc_trace, "|");
  }
  return;
}

void branchprof::update_bb_fb() {
  if (bb_over == 1) {
    if (running_bb_size >= MAX_BB_SIZE) {
      bb_size[MAX_BB_SIZE - 1]++;
    } else {
      bb_size[running_bb_size]++;
    }
    sum_bb_size += running_bb_size;
    bb_count++;
    running_bb_size = 0;
  } else {
    running_bb_size++;
  }
  if (fb_over == 1) {
    if (running_fb_size >= MAX_FB_SIZE) {
      fb_size[MAX_FB_SIZE - 1]++;
    } else {
      fb_size[running_fb_size]++;
    }
    sum_fb_size += running_fb_size;
    fb_count++;
    if (bb_over == 1)
      running_fb_size = 0;
    else
      running_fb_size = 1;
  } else {
    running_fb_size++;
  }
}

void branchprof::handle_nb() {
  if (predDir == resolveDir) {
    misprediction = false;
    correct_prediction_count++;
    #ifdef DEBUG
  	//fprintf (stderr, "Non branch correctly predicted \n");
  	#endif
  } else {
    misprediction = true;
    misprediction_count++;
    #ifdef DEBUG
  	fprintf (stderr, "Non branch Mispredicted \n");
  	#endif
  }
  if ((insn == insn_t::non_cti) && (predDir == true)) {
    misscontrol_count++;
    #ifdef DEBUG
  	fprintf (stderr, " that is also a Non_CTI \n");
  	#endif
  }
}

  /******************************************************************
  As given, the simulator (dromajo) always fetches correctly,
  i.e. the FETCH_WIDTH instructions being received by the CPU are the actual
  dynamic code execution stream, there is no wrong fetch, only wrong prediction
  hence, all entries in FTQ are for correct instructions
  hence -> ??? NO NEED TO NUKE UNLESS ITS REAL SUPERSCALAR RETURNING SEQUENTIAL
  INSTRUCTIONS FROM STATIC BINARY ??? Hence, for now -> If branch is correctly
  predicted -> update MPKI + update predictor - but pr, pr, up, up If branch is
  mispredicted -> update MPKI + update predictor - pr, up, pr, up i.e. if
  misprediction, immediately update rather than after FETCH_WIDTH instructions
  and restart counter to collect FETCH_WIDTH instructions for next update

  Also, update after FETCH_WIDTH instructions means update after 1 CC in
  Superscalar Check if this needs to be delayed + split, i.e. global history to
  be updated after a few CC, but actual predictor table to be
  updated after commit
  *******************************************************************/
  
/*
For getting a prediction and updating predictor, like hardware, there will be a
race in software also, I think in any CC, I should read the predictor first with
whatever history is there and update it after the read. That is what will happen
in hardware.
*/

/*
Changes to return multiple predictions per FetchPC
1. Dromajo will still send actual instruction PCs. 
2. Calculate FetchPC and index in packet from PC.
3. Check if it is sequentially next PC
	If instruction == first in packet (either index == 0 or coming in from a branch/jump etc), then get prediction(s) using FetchPC- should return (fetch_width * num_subentries) predictions and related info. All must be saved into FTQ.
	On subsequent calls, check if PC is within fetch_packet and if it is sequential, if both, then 
		If PC is not sequential i.e. taken branch, then  
*/

/*
TODO Check
Handling multiple Taken
1. 1st taken is in fetchpacket0 and 2nd taken is in fetchpacket1
pos[0] = index in fetchpacket0, pos[1] = pos[0] + index in fetchpacket1
Must check over 2 packets for both allocate and resolve/ update   !!!!!!!!!!!!!!!!!!!!

Resolve - 
If prediction is a hit - resolving = true
else 
	if resolving = 
	
	
	
Update - 
*/
/*
gshare update - 
the pc, pos and target info is available int he prediction_packet, in dromajo, we have only one such packet ata a time, in desesc, we will have multiple in ftq, but only 1 active - dequeued
At each instruction resolution - compare pc and pos to see if there was a gshare prediction, if yes - 
 1, right or wrong - update the flag
when the packet is over and update for the entire packet needs to be done, gshare nust be updated based on the various resolved flags

1. just taken and pos - update gshare prediciton counters and only local rates
2. update prediciton selection and global rates
3. add target to update check
*/

#ifdef GSHARE
void branchprof::get_gshare_prediction(uint64_t temp_pc, int index, int tag)
{
   	gshare_pred_inst = bp->GetFastPrediction(temp_pc, index, tag);
}
#endif // GSHARE

void branchprof:: handle_insn (uint64_t pc, uint32_t insn_raw) {

	// TODO - Written for 16 bit instructions, actual are mostly 32, so wrong aligned fetch_pc and offset
	//aligned_fetch_pc = (pc >> (LOG2_FETCH_WIDTH + 1)) << (LOG2_FETCH_WIDTH + 1);
	//index_from_aligned_fetch_pc = (pc - aligned_fetch_pc)>>1; 
	
	#ifdef CHECK_SS
	//fprintf (stderr, "aligned_fetch_pc = %llu, index = %llu and ", aligned_fetch_pc, index_from_aligned_fetch_pc);
	#endif // CHECK_SS

	branchTarget = pc;
  	bb_over = 0, fb_over = 0;
  	misprediction = false;

  // for (int i = 0; i < m->ncpus; ++i)
  if (i0_done == true) {
    // If previous instruction was a branch. resolve that first
    if (last_insn == insn_t::branch) {
      resolve_pc_minus_1_branch(pc);
    }

	// Update ftq for all insns, even if last_insn == non_cti
	//if (last_insn != insn_t::non_cti)
	{
		bp->ftq_inst.ftq_update_resolvedinfo (last_inst_index_in_fetch, last_pc, last_insn, last_resolveDir, branchTarget); 
	}

#ifdef SUPERSCALAR
#ifdef FTQ
    read_ftq_update_predictor();
#endif // FTQ
#else  // #ifndef SUPERSCALAR
    bp->UpdatePredictor(last_pc, last_resolveDir, last_predDir, branchTarget);
#endif // SUPERSCALAR
  }

  // At this point pc-1 handling is complete

  // Now start handling insn at pc

#ifdef PC_TRACE
fprintf (stderr, "\n");
  print_pc_insn(pc, insn_raw);
#endif


  if (opcode(insn_raw) == 0x73) // ECall, EBreak
  {
    	insn = insn_t::jump;
    	close_pc_jump(pc, insn_raw);
  }
  else if (opcode(insn_raw) == 0x63) // Branch
  	{
  		insn = insn_t::branch;
   		start_pc_branch();
  	}
  	else if (opcode(insn_raw) == 0x6f) // jal
  	{
  				if ( rd(insn_raw) == 0x1) // call
  				{
    				insn = insn_t::jump;
    				close_pc_jump(pc, insn_raw);
  				}
  				else // jump
  				{
    				insn = insn_t::jump;
    				close_pc_jump(pc, insn_raw);	
  				}
  	}
  	else if (opcode(insn_raw) == 0x67) // jalr
  	{
  				if ( rd(insn_raw) == 0x1) // call
  				{
    				insn = insn_t::jump;
    				close_pc_jump(pc, insn_raw);
  				}
  				else if ( (rd(insn_raw) == 0x0) && (rs1(insn_raw) == 0x1) && (imm_itype(insn_raw) == 0x0)) // return
  				{
  					insn = insn_t::ret;
      				close_pc_jump(pc, insn_raw);
  				}
  				else // jump
  				{
    				insn = insn_t::jump;
    				close_pc_jump(pc, insn_raw);	
  				}
  	}
  	else // non_cti
  	{
  			    insn = insn_t::non_cti;
    			close_pc_non_cti();
  	}

  // At this point - if present instruction - insn is not a branch - resolveDir
  // contains info about pc, else pc - 1

  // Update BB - BR Based on present instruction (exception - Branch)
#ifdef EN_BB_FB_COUNT
  update_bb_fb();
#endif // EN_BB_FB_COUNT

 #ifdef T_TRACE
 if ( (PC1_branch == true) && (insn == insn_t::jump) ) 
 {
 	PC2 = pc;
	PC2_branch = false;
	fprintf (t_trace, "PC1 = %llx, PC2 = %llx, i_count = %u\n", PC1, PC2, i_count);
 	i_count = 0;
 	PC1 = PC2;
 	PC1_branch = PC2_branch;
}
#endif
/*
Present - For PC with index = 0, get fetch_width predictions using expected/ sequential pcs.
Two options - 
1. Probably harder - Call get_prediciton multiple times, "all calls to sub entries in same entry must match the same tag". No change to ftq.
2. Probably easier - Call get_prediction only once, it must return all (info per entry) predictions. FTQ must save the following info "correct and separate" for each of the pcs in every cc - 
bp + Check counters "s", bi, bi2, gi, b_bi, b2_bi2 
*/

  if (inst_index_in_fetch == 0)      
  {
  	fetchBoundaryBegin(pc);	
  }
  	
  	predDir = false;
#ifdef GSHARE 	
   	/*if (gshare_pred_inst.hit && (inst_index_in_fetch == gshare_pred_inst.info.poses[0]))
  	{
  		if (!get_predDir_from_ftq (inst_index_in_fetch))
  		{
  			gshare_batage_1st_pred_mismatch++;
  		}
  	}*/
 
  if ( /*(gshare_pred_inst.hit && (inst_index_in_fetch == gshare_pred_inst.info.poses[0])) ||*/ (  last_gshare_pred_inst.hit && (inst_index_in_fetch == last_gshare_pred_inst.info.poses[1])) )
  {
  	predDir = true;
  	if (!(bp->ftq_inst.get_predDir_from_ftq (inst_index_in_fetch)))
  	{
  		gshare_batage_2nd_pred_mismatch++;
  	}
  }
else
#endif
  //Get predDir for the instruction
  {predDir = bp->ftq_inst.get_predDir_from_ftq (inst_index_in_fetch);}

#ifdef DEBUG
    {
      std::cerr << "Prediction gotten from ftq" << "\n";
    }
#endif // DEBUG  
  // Update counters -
  // If this instruction is not a branch - straight case - predDir and
  // resolveDir in sync If this instruction is a branch - it will be resolved in
  // next CC - hence,

  if (insn != insn_t::branch) {
    handle_nb();
  }

  	benchmark_instruction_count++;
  	last_inst_index_in_fetch = inst_index_in_fetch;
	inst_index_in_fetch++;
	#ifdef T_TRACE
	i_count++;
	#endif

  	last_pc = pc;
  	last_insn_raw = insn_raw;
  	last_insn = insn;
  	last_predDir = predDir;
  	//last_aligned_fetch_pc = aligned_fetch_pc;
	//last_index_from_aligned_fetch_pc = index_from_aligned_fetch_pc;
    	
  if (insn != insn_t::branch) {
    last_resolveDir = resolveDir;
    last_misprediction = misprediction;
  }
  if (i0_done == false) {
    i0_done = true;
  }
}

void handle_insn_t(uint64_t pc, uint8_t insn_type)
{
	// TBDone
	return;
}
