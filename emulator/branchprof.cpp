#include "branchprof.hpp"
#include "dromajo.h"
#include "emulator.hpp"
#include "predictor.hpp"
#include <iostream>

#define DEBUG_GSHARE

PREDICTOR bp;
bool predDir, last_predDir, resolveDir, last_resolveDir;
uint64_t branchTarget;

// gshare allocate
gshare_prediction gshare_pred_inst;
bool gshare_pos1_correct, gshare_pos0_correct, gshare_prediction_correct;
uint8_t highconf_ctr = 0;
bool gshare_tracking = false, highconfT_in_packet = false;
vector <uint64_t> gshare_PCs;
vector <uint8_t> gshare_poses;
uint64_t num_gshare_predicitons, num_gshare_correct_predicitons; 
	int gshare_index;
	int gshare_tag;

FILE *pc_trace;

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

uint64_t correct_prediction_count, misprediction_count;
extern uint64_t instruction_count;    // total # of instructions - including
                                      // skipped and benchmark instructions
uint64_t benchmark_instruction_count; // total # of benchmarked instructions
                                      // only - excluding skip instructions
                                      
uint64_t fetch_pc;

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
static uint64_t aligned_fetch_pc, last_aligned_fetch_pc;
static uint16_t index_from_aligned_fetch_pc, last_index_from_aligned_fetch_pc; // starts from pc - (alignedPC) after every redirect

static uint64_t last_pc;
static uint32_t last_insn_raw;
static insn_t last_insn, insn;
static bool last_misprediction, misprediction;
bool i0_done = false;
#ifdef EN_BB_FB_COUNT
static uint8_t bb_over = 0, fb_over = 0;
#endif // EN_BB_FB_COUNT
#ifdef SUPERSCALAR
static int16_t inst_index_in_fetch = 0, last_inst_index_in_fetch; // starts from 0 after every redirect
#endif

extern uint64_t maxinsns, skip_insns;

void branchprof_init(char* bp_logfile) {
  // PREDICTOR bp;
  pc_trace = fopen(bp_logfile, "w");
  if (pc_trace == nullptr) {	
    fprintf(stderr,
            "\nerror: could not open %s for dumping trace\n", bp_logfile);
    //exit(-3);
  } else {
    fprintf(stderr, "\nOpened dromajo_simpoint.bb for dumping trace\n");
    fprintf(pc_trace,
            "FETCH_WIDTH = %u, SKIP_COUNT = %lu, benchmark_count = %lu \n",
            FETCH_WIDTH, skip_insns, maxinsns);
#ifdef PC_TRACE
    fprintf(pc_trace, "%20s\t\t|%20s\t|%32s\n", "PC", "Instruction",
            "Instructiontype");
#endif // PC_TRACE
  }
#ifdef FTQ
  std::cerr << "NUM_FTQ_ENTRIES = " << NUM_FTQ_ENTRIES << "\n\n";
#endif // FTQ
  return;
}

void branchprof_exit() {
  fprintf(pc_trace,
          "branch_count = %lu\njump_count = %lu\ncti_count = "
          "%lu\nbenchmark_instruction_count = %lu\nInstruction Count = "
          "%lu\nCorrect prediciton Count = %lu\nmispredction count = "
          "%lu\nbranch_mispredict_count=%lu\nmisscontrol_count=%"
          "lu\nmisprediction rate = %lf\nMPKI = %lf\n",
          branch_count, jump_count, cti_count, benchmark_instruction_count,
          instruction_count, correct_prediction_count, misprediction_count,
          branch_mispredict_count, misscontrol_count,
          (double)(branch_mispredict_count + misscontrol_count) /
              (double)(correct_prediction_count + misprediction_count - jump_count) * 100,
          (double)(branch_mispredict_count + misscontrol_count) / (double)(benchmark_instruction_count - jump_count) *
              1000);
		#ifdef GSHARE
              fprintf (pc_trace, "gshare local rates - \ngshare_num_predictions = %llu\ngshare_num_correct_predictions = %llu\ngshare_misprediction_rate = %lf%\n", num_gshare_predicitons, num_gshare_correct_predicitons, ((double)(num_gshare_predicitons-num_gshare_correct_predicitons)*100/num_gshare_predicitons) );
		#endif // GSHARE              
for (int i = 0; i < SBP_NUMG; i++)
              {
              	fprintf(pc_trace, "Allocations on Table %d = %u\n", i, bp.get_allocs(i));
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
  return;
}

#ifdef FTQ
static inline void copy_ftq_data_to_predictor(ftq_entry *ftq_data_ptr) {
  bp.pred.hit = ftq_data_ptr->hit;
  bp.pred.s = ftq_data_ptr->s;
  bp.pred.poses = ftq_data_ptr->poses;
  bp.pred.meta = ftq_data_ptr->meta;
  bp.pred.bp = ftq_data_ptr->bp;
  bp.pred.bi = ftq_data_ptr->bi;
  bp.pred.bi2 = ftq_data_ptr->bi2;
  bp.pred.b_bi = ftq_data_ptr->b_bi;
  bp.pred.b2_bi2 = ftq_data_ptr->b2_bi2;
  memcpy(bp.pred.gi, &((ftq_data_ptr->gi)[0]), sizeof(int) * SBP_NUMG);
}
#endif // FTQ

static inline void update_counters_pc_minus_1_branch() {
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

void update_gshare(int i, uint64_t target)
{
        if (gshare_pred_inst.tag_match)
        {
        	if (i == gshare_pred_inst.info.poses[0]) 
        	{
        		//if  ( gshare_pred_inst.info.PCs[0] == target)
        		{gshare_pos0_correct = true;}
        	}
        	else if ( (gshare_pred_inst.info.poses[0] + i) == gshare_pred_inst.info.poses[1]) 
        	{
        		//if ( gshare_pred_inst.info.PCs[1] == target)
        		{gshare_pos1_correct = true;}
        	}
        }
}

#ifdef FTQ
static inline void read_ftq_update_predictor() {
  uint64_t update_pc, update_fetch_pc, update_branchTarget;
  uint16_t update_gshare_index, update_gshare_tag;
  insn_t update_insn;
  bool update_predDir, update_resolveDir;
  bool update_highconf;

  uint8_t partial_pop = (last_misprediction || last_resolveDir);

	highconfT_in_packet = false;

	// For multiple taken predictions, changed from FETCH_WIDTH to INFO_PER_ENTRY 
  if ((inst_index_in_fetch == FETCH_WIDTH) || partial_pop) {
  
  #ifdef DEBUG_FTQ
  fprintf (stderr, "\nread_ftq_update_predictor : inst_index_in_fetch = %u, partial_pop = %d\n", inst_index_in_fetch, partial_pop);
  #endif

#ifdef DEBUG_GSHARE
if (gshare_pred_inst.hit)
{
printf ("gshare hit prediction - pos[0] = %u, PC[0] = %#llx, pos[1] = %u, PC[1] = %llx\n", gshare_pred_inst.info.poses[0], gshare_pred_inst.info.PCs[0], gshare_pred_inst.info.poses[1], gshare_pred_inst.info.PCs[1]);
}
#endif

    for (int i = 0; i < (partial_pop ? inst_index_in_fetch : FETCH_WIDTH);
         i++) {
      if (!is_ftq_empty()) {
        get_ftq_data(&ftq_data);

        update_pc = ftq_data.pc;
        update_insn = ftq_data.insn;
        update_resolveDir = ftq_data.resolveDir;
        update_predDir = ftq_data.predDir;
        update_branchTarget = ftq_data.branchTarget;
        update_fetch_pc = ftq_data.fetch_pc;

	#ifdef GSHARE
	// update gshare
	#ifdef DEBUG_GSHARE
	if (gshare_pred_inst.hit)
        {
        	if (i == gshare_pred_inst.info.poses[0]) 
        	{
        		printf ("gshare resolution for i = %u, resolveDir = %u, target = %#llx \n", i, update_resolveDir, update_branchTarget);
        	}
        	else if ( (gshare_pred_inst.info.poses[0] + i) == gshare_pred_inst.info.poses[1]) 
        	{
        		printf ("gshare resolution for i = %u, resolveDir = %u, target = %#llx \n", gshare_pred_inst.info.poses[0] + i, update_resolveDir, update_branchTarget);
        	}
        }
		
	#endif // DEBUG_GSHARE
       	update_gshare(i, update_branchTarget);
	#endif
        
        // allocate_gshare
        update_highconf = ftq_data.highconf;
        update_gshare_index = gshare_pred_inst.index; // ftq_data.gi[1];

	// allocate on a branch or if second is a jump
	if (!gshare_pred_inst.hit && update_resolveDir && (((update_insn == insn_t::branch) && update_highconf) || (update_insn == insn_t::jump)) )
	{
		highconfT_in_packet = true;
		if (!gshare_tracking) // The first must be a branch
		{
			if (update_insn == insn_t::branch)
			{
				gshare_PCs.push_back(update_fetch_pc);
				gshare_poses.push_back( i ); // (update_pc - update_fetch_pc) >> 1
				gshare_PCs.push_back(update_branchTarget);
				gshare_tracking = true;
				update_gshare_tag = ftq_data.gi[ftq_data.bp[i]];
			}
		}
		else
		{
			gshare_poses.push_back( (gshare_poses[0] + i )); // (update_pc - update_fetch_pc) >> 1)
			gshare_PCs.push_back(update_branchTarget);
		}
		
		// TODO Change to NUM_TAKEN_BRANCHES
		if (gshare_poses.size() == 2) 
		{
			bp.fast_pred.allocate(gshare_PCs, gshare_poses, update_gshare_index, update_gshare_tag);
			// TODO - do we handle if there are 3 high conf T in 3 consecutive packets 
			gshare_tracking = false;
			gshare_PCs.clear();
			gshare_poses.clear();
		}
	}
	// allocate gshare done
	
        // Store the read predictor fields into the predictor
        copy_ftq_data_to_predictor(&ftq_data);
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
		bp.Updatetables(update_pc, update_fetch_pc, i, update_resolveDir);
	}
  }
  #ifdef DEBUG_FTQ
  fprintf (stderr, "Update predictor tables done for pc = %llx \n", update_pc);
  #endif
        if (update_insn != insn_t::non_cti) {
			  #ifdef DEBUG_HUQ
  			fprintf (stderr, "Allocate huq entry for pc = %llx, target = %llx, resolvedir = %d \n", update_pc, update_branchTarget, update_resolveDir);
  			#endif
          allocate_huq_entry(/*update_pc,*/ update_branchTarget,
                             update_resolveDir);
        }
        /*else if(update_insn == insn_t::jump)
        {
                allocate_huq_entry(update_branchTarget, true);
                //bp.TrackOtherInst(update_pc, bool branchDir,
        update_branchTarget);
        }*/

        //inst_index_in_fetch--; // i is being incremented, DO NOT decrement this
      } else {
        fprintf(stderr,
                "Pop on empty ftq, inst_index_in_fetch = %d, misprediction = "
                "%d, resolveDir = %d \n",
                inst_index_in_fetch, misprediction, resolveDir);
      }
    }
    
    if (!highconfT_in_packet && gshare_tracking)
    {
    	gshare_tracking = false;
    	gshare_PCs.clear();
	gshare_poses.clear();
    }

	#ifdef GSHARE
	gshare_prediction_correct = gshare_pos1_correct && gshare_pos0_correct;
	
        if (gshare_pred_inst.hit)
        {
        	if (gshare_prediction_correct)
        	{
        		num_gshare_correct_predicitons++;
        	}
        }
	//if (gshare_pred_inst.hit)
	{
    		bp.fast_pred.update(gshare_pred_inst, gshare_prediction_correct);
    	}
	#endif
    
    inst_index_in_fetch = 0;
    #ifdef DEBUG
  	fprintf (stderr, "||||||||||||||||||||||||||| NUKING FTQ |||||||||||||||||||||||||| \n");
  	#endif
  	// TODO - Check if nuke necessary/ correct
    nuke_ftq();
    #ifdef DEBUG
  	fprintf (stderr, "Popping huq, Updating histories \n");
  	#endif
    while (!is_huq_empty()) {
      get_huq_data(&huq_data);
      	#ifdef DEBUG
  			fprintf (stderr, "Updating history for target = %llx, resolvedir = %d \n", huq_data.branchTarget, huq_data.resolveDir);
  			#endif
      bp.Updatehistory(huq_data.resolveDir, huq_data.branchTarget);
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
  }
}
#endif // FTQ

static inline void resolve_pc_minus_1_branch(uint64_t pc) {

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
#ifdef DEBUG
    fprintf(pc_trace, "%32s\n", "Taken Branch");
    fprintf(stderr, "%llx is %32s\n", last_pc, "Taken Branch");
#endif
  }

	// TODO - Temporarily disabled print
  //fprintf(stderr, "target pc=%llx last_pc (branch pc) =%llx diff=%d pred:%d resolv:%d\n", pc, last_pc, pc - last_pc, last_predDir, last_resolveDir);
          
  update_counters_pc_minus_1_branch();
  return;
}

// Not being used
static inline void close_pc_minus_1_branch(uint64_t pc) {
#ifdef FTQ
  read_ftq_update_predictor();
#endif
  return;
}

static inline void close_pc_jump(uint64_t pc, uint32_t insn_raw) {
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

static inline void start_pc_branch() {
  branch_count++;
  cti_count++;
#ifdef EN_BB_FB_COUNT
  bb_over = 1;
#endif // EN_BB_FB_COUNT
}

static inline void close_pc_non_cti() {
  resolveDir = false;
#ifdef PC_TRACE
  fprintf(pc_trace, "%32s\n", "Non - CTI");
  fprintf(stderr, "%32s\n", "Non - CTI");
#endif
}

void print_pc_insn(uint64_t pc, uint32_t insn_raw) {
  fprintf(pc_trace, "%20lx\t|%20x\t", pc, insn_raw);
  fprintf(stderr, "Fetched %d|\t%20lx\t|%20x\t", inst_index_in_fetch, pc, insn_raw);
  if (insn_raw < 0x100) {
    fprintf(pc_trace, "\t|");
  } else {
    fprintf(pc_trace, "|");
  }
  return;
}

static inline void update_bb_fb() {
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

static inline void handle_nb() {
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
TODO
Changes to return multiple predictions per FetchPC
1. Dromajo will still send actual instruction PCs. 
2. Calculate FetchPC and index in packet from PC.
3. Check if it is sequentially next PC
	If instruction == first in packet (either index == 0 or coming in from a branch/jump etc), then get prediction(s) using FetchPC- should return (fetch_width * num_subentries) predictions and related info. All must be saved into FTQ.
	On subsequent calls, check if PC is within fetch_packet and if it is sequential, if both, then 
		If PC is not sequential i.e. taken branch, then  
*/

/*
Changes to "get" multiple predictions per FetchPC
The predictor will still return 1 prediction per PC, but branchprof will call get_prediction multiple times for each FetchPC using different indices.
get_prediction must be called together for each PC
Hence allocate must also be done at this time immediately after get_prediction, so allocate to last_insn must be removed, Predict + Allocate to insn and then Resolve + update later at resolution time.

TODO - Check all cases
In order to allocate_ftq_entry
If PC == FetchPC aligned, then call get_prediction multiple times to get all the predictions, save each in FTQ.
If PC != FetchPC - 
	1. If 
 	2.
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
/*
void check_gshare (void)
{
	if ( (gshare_pred_inst.hit) && (gshare_pred_inst.info.PCs[0] == fetch_pc) )
	{
		// TODO Change to NUM_TAKEN_BRANCHES
		if (((last_pc - fetch_pc) >> 1) == gshare_pred_inst.info.poses[0])
		{
			if (last_resolveDir) // direction taken
			{
				gshare_pos0_correct = true;
			}
		}
		// TODO - Check
		if (((last_pc - fetch_pc) >> 1) == (gshare_pred_inst.info.poses[1] - gshare_pred_inst.info.poses[0] ))
		{
			if (last_resolveDir) // direction taken
			{
				gshare_pos1_correct = true;
			}
		}
	}
}
*/

void get_gshare_prediction(uint64_t temp_pc, int index, int tag)
{
   	gshare_pred_inst = bp.GetFastPrediction(temp_pc, index, tag);
    	gshare_pos0_correct = false;
    	gshare_pos1_correct = false;
    	gshare_prediction_correct = false;
    	if (gshare_pred_inst.hit)
    	{
    		num_gshare_predicitons++;
    	}
    	
    	
}

void handle_branch(uint64_t pc, uint32_t insn_raw) {

	// TODO - Written for 16 bit instructions, actual are mostly 32, so wrong aligned fetch_pc and offset
	aligned_fetch_pc = (pc >> (LOG2_FETCH_WIDTH + 1)) << (LOG2_FETCH_WIDTH + 1);
	index_from_aligned_fetch_pc = (pc - aligned_fetch_pc)>>1; 
	
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
		ftq_update_resolvedinfo (last_inst_index_in_fetch, last_pc, last_insn, last_resolveDir, branchTarget); 
	}

#ifdef SUPERSCALAR
#ifdef FTQ
    read_ftq_update_predictor();
#endif // FTQ
#else  // #ifndef SUPERSCALAR
    bp.UpdatePredictor(last_pc, last_resolveDir, last_predDir, branchTarget);
#endif // SUPERSCALAR
  }

  // At this point pc-1 handling is complete

  // Now start handling insn at pc

#ifdef PC_TRACE
fprintf (stderr, "\n");
  print_pc_insn(pc, insn_raw);
#endif

      if (((insn_raw & 0x7fff) == 0x73)) {
    // ECall and EBrk
    insn = insn_t::jump;
    close_pc_jump(pc, insn_raw);
  }
  else if (((insn_raw & 0x70) == 0x60)) {
    if (((insn_raw & 0xf) == 0x3)) {
      insn = insn_t::branch;
      start_pc_branch();
    } 
    else if (((insn_raw & 0xf) == 0xf)) {
      insn = insn_t::jump;
      close_pc_jump(pc, insn_raw);
    }
    else // Return
    {
      insn = insn_t::ret;
      close_pc_jump(pc, insn_raw);
    }
  }
  else // Non CTI
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

/*
Present - For PC with index = 0, get fetch_width predictions using expected/ sequential pcs.
Two options - 
1. Probably harder - Call get_prediciton multiple times, "all calls to sub entries in same entry must match the same tag". No change to ftq.
2. Probably easier - Call get_prediction only once, it must return all (info per entry) predictions. FTQ must save the following info "correct and separate" for each of the pcs in every cc - 
bp + Check counters "s", bi, bi2, gi, b_bi, b2_bi2 
*/

  // Get predDir - TODO new code - so Check
  if (inst_index_in_fetch == 0)      // (pc == aligned_fetch_pc)
  {
  	fetch_pc = pc;
  	uint64_t temp_pc = pc;
  	set_ftq_index (inst_index_in_fetch);
  	
  	// temp_predDir = bp.GetPrediction(temp_pc);
  	prediction batage_prediction;
  	std::vector<bool> vec_predDir(FETCH_WIDTH, false);
  	std::vector<bool> vec_highconf(FETCH_WIDTH, false);
  	
  	#ifdef DEBUG
    {
      std::cerr << "getting predictions" << "\n";
    }
    #endif // DEBUG
    

    
	//vec_predDir = std::move(bp.GetPrediction(temp_pc));
	batage_prediction = bp.GetPrediction(temp_pc);
	vec_predDir =  batage_prediction.prediction_vector;
	vec_highconf = batage_prediction.highconf;	

	gshare_index = batage_prediction.gshare_index;
	gshare_tag = batage_prediction.gshare_tag;
	#ifdef GSHARE
	#ifdef DEBUG_GSHARE
	//printf ("gshare_index = %u and gshare_tag = %x\n", gshare_index, gshare_tag);
	#endif // DEBUG_GSHARE
 	get_gshare_prediction(temp_pc, gshare_index, gshare_tag);
	#endif  // GSHARE

	#ifdef DEBUG
    	{
      		std::cerr << "got predictions" << "\n";
    	}
	#endif // DEBUG
	
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
  		if (is_ftq_full() == false) {
  			// Always allocate with default info - i.e. a non_cti 
  			// At this point if 1st instruction for which correct info is avail is non-cti it is already updated
  			// If it is cti - then branchtarget is avail in next CC and hence should only be updated condirionally then
    		allocate_ftq_entry(vec_predDir[i], vec_highconf[i], false, temp_pc, insn_t::non_cti, temp_pc+2, fetch_pc, bp);
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
  }
  	
  	predDir = false;
#ifdef GSHARE 	
  if (gshare_pred_inst.hit)
  	{
  		if (inst_index_in_fetch == gshare_pred_inst.info.poses[0])
  			{predDir = true;}
  		else if (inst_index_in_fetch == gshare_pred_inst.info.poses[1])
  			{predDir = true;}
  }
else
#endif
  //Get predDir for the instruction
  {predDir = get_predDir_from_ftq (inst_index_in_fetch);}

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

  	last_pc = pc;
  	last_insn_raw = insn_raw;
  	last_insn = insn;
  	last_predDir = predDir;
  	last_aligned_fetch_pc = aligned_fetch_pc;
	last_index_from_aligned_fetch_pc = index_from_aligned_fetch_pc;
  
  if (insn != insn_t::branch) {
    last_resolveDir = resolveDir;
    last_misprediction = misprediction;
  }
  if (i0_done == false) {
    i0_done = true;
  }
}
