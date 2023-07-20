#include <iostream>
#include "dromajo.h"
#include "emulator.hpp"
#include "predictor.hpp"
#include "branchprof.hpp"

//#define DELETE

PREDICTOR bp;
bool predDir, last_predDir, resolveDir, last_resolveDir;
uint64_t branchTarget;

FILE* pc_trace;

#ifdef EN_BB_FB_COUNT
#define MAX_BB_SIZE 50
#define MAX_FB_SIZE 250
uint64_t bb_size[MAX_BB_SIZE], fb_size[MAX_FB_SIZE];
uint32_t sum_bb_size, sum_fb_size, bb_count, fb_count;
uint8_t running_bb_size, running_fb_size;
#endif // EN_BB_FB_COUNT

#ifdef SUPERSCALAR
#ifdef FTQ
#include "ftq.hpp"
ftq_entry ftq_data; // Only 1 instance - assuming the updates for superscalar will be done 1 by 1, so they may reuse the same instance
#endif // FTQ
#endif // SUPERSCALAR

uint64_t correct_prediction_count, misprediction_count;
extern uint64_t instruction_count; // total # of instructions - including skipped and benchmark instructions
uint64_t benchmark_instruction_count; // total # of benchmarked instructions only - excluding skip instructions

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
uint64_t branch_count, branch_mispredict_count, jump_count, cti_count, misscontrol_count, taken_branch_count;

static uint64_t last_pc;
static uint32_t last_insn_raw;
static insn_t last_insn, insn;
static bool last_misprediction, misprediction;
bool i0_done = false;
#ifdef EN_BB_FB_COUNT
static uint8_t bb_over = 0, fb_over = 0;
#endif // EN_BB_FB_COUNT
#ifdef SUPERSCALAR
static int16_t inst_index_in_fetch = 0;
#endif

void branchprof_init()
{
	// PREDICTOR bp;
  	pc_trace = fopen("pc_trace.txt", "a");
  	if (pc_trace == nullptr) {
    	fprintf(dromajo_stderr,
            "\nerror: could not open pc_trace.txt for dumping trace\n");
    	exit(-3);
  	} else {
    	fprintf(dromajo_stderr, "\nOpened dromajo_simpoint.bb for dumping trace\n");
#ifdef PC_TRACE
    	fprintf(pc_trace, "%20s\t\t|%20s\t|%32s\n", "PC", "Instruction", "Instructiontype");
#endif  // PC_TRACE
  }
#ifdef FTQ
	std::cout << "NUM_FTQ_ENTRIES = " << NUM_FTQ_ENTRIES << "\n\n";
#endif // FTQ
	return;
}

void branchprof_exit()
{
	fprintf (pc_trace, "branch_count = %lu\njump_count = %lu\ncti_count = %lu\nbenchmark_instruction_count = %lu\nInstruction Count = %lu\nCorrect prediciton Count = %lu\nmispredction count = %lu\nbranch_mispredict_count=%lu\nmisscontrol_count=%lu\nmisprediction rate = %lf\nMPKI = %lf\n", branch_count, jump_count, cti_count, benchmark_instruction_count, instruction_count, correct_prediction_count, misprediction_count, branch_mispredict_count, misscontrol_count, (double)misprediction_count/(double)(correct_prediction_count + misprediction_count) *100,  (double)misprediction_count/(double)benchmark_instruction_count *1000 );
  
#ifdef EN_BB_FB_COUNT
	fprintf(pc_trace, "bb_size = \n");
  	for (int i = 0; i < MAX_BB_SIZE; i++)
  	{    	
    	fprintf(pc_trace, "%lu, ", bb_size[i]);
  	}
  	fprintf(pc_trace, "\n");
  	fprintf(pc_trace, "avg_bb_size = %lf \n", (double)sum_bb_size/bb_count);
  	
  	fprintf(pc_trace, "fb_size = \n");
  	for (int i = 0; i < MAX_FB_SIZE; i++)
  	{    	
    	fprintf(pc_trace, "%lu, ", fb_size[i]);
  	}
  	fprintf(pc_trace, "\n");
  	fprintf(pc_trace, "avg_fb_size = %lf \n", (double)sum_fb_size/fb_count);
#endif // EN_BB_FB_COUNT

  	fclose(pc_trace);
	return;
}

#ifdef FTQ
static inline void copy_ftq_data_to_predictor(ftq_entry* ftq_data_ptr)
{
    bp.pred.hit 		= ftq_data_ptr->hit;
    bp.pred.s 			= ftq_data_ptr->s;
    bp.pred.meta 		= ftq_data_ptr->meta;
    bp.pred.bp 			= ftq_data_ptr->bp;
    bp.pred.bi 			= ftq_data_ptr->bi;
    bp.pred.bi2 		= ftq_data_ptr->bi2;
	bp.pred.b_bi		= ftq_data_ptr->b_bi;
	bp.pred.b2_bi2		= ftq_data_ptr->b2_bi2;
	memcpy(bp.pred.gi, &((ftq_data_ptr->gi)[0]), sizeof(int) * NUMG);
}
#endif // FTQ

static inline void update_counters_pc_minus_1_branch()
{
		if (last_predDir == last_resolveDir) // branch is resolved in next CC, so match last_resolveDir with last_predDir
    	{
    		last_misprediction = false;
    		correct_prediction_count++;
    	}
    else 
    	{
    		last_misprediction = true;
    		misprediction_count++;		// Both mispredicted T and NT
    		branch_mispredict_count++; 	// Both mispredicted T and NT
    	}
}

#ifdef FTQ
static inline void read_ftq_update_predictor ()
{
	uint64_t update_pc, update_branchTarget;
	bool update_predDir, update_resolveDir;
	
	// TODO Check misprediction and resolveDir use correct value
	uint8_t partial_pop = (last_misprediction || last_resolveDir);
	
	if ( (inst_index_in_fetch == FETCH_WIDTH) || partial_pop )
    {
		
#ifdef DEBUG_FTQ
    	std::cout << "Deallocating - inst_index_in_fetch = " << inst_index_in_fetch << " last_misprediction = " << last_misprediction << " last_resolveDir = " << last_resolveDir << "\n"; 
#endif

    	for (int i = 0; i < (partial_pop ? inst_index_in_fetch : FETCH_WIDTH); i++)
    	{
    		if (!is_ftq_empty()) 
    		{
    			get_ftq_data(&ftq_data);
    			
    			// TODO Check resolveDir, predDir, branchTarget
    			update_pc = ftq_data.pc;
    			update_resolveDir = ftq_data.resolveDir;
    			update_predDir = ftq_data.predDir;
    			update_branchTarget = ftq_data.branchTarget;
    			
    			// Store the read predictor fields into the predictor
    			// TODO Check if predictor contents need to be saved before doing this.
    			copy_ftq_data_to_predictor(&ftq_data);

    			bp.UpdatePredictor(update_pc, update_resolveDir, update_predDir, update_branchTarget);
    			inst_index_in_fetch--;
    		}
    		else
    		{
    	 		fprintf(stderr, "Pop on empty ftq, inst_index_in_fetch = %d, misprediction = %d, resolveDir = %d \n", inst_index_in_fetch, misprediction, resolveDir);
    		}
    	}
    	last_pc = update_pc;
    	last_resolveDir = update_resolveDir;
    	last_predDir = update_predDir;
    	//branchTarget = update_branchTarget;
#ifdef DEBUG_FTQ
    	{std::cout << "After deallocations over - inst_index_in_fetch = " << inst_index_in_fetch << "\n";}
#endif // DEBUG_FTQ
    }
}
#endif // FTQ

static inline void resolve_pc_minus_1_branch (uint64_t pc)
{
	if (pc - last_pc == 4) {
		last_resolveDir = false;
#ifdef PC_TRACE
		fprintf(pc_trace, "%32s\n", "Not Taken Branch");
#endif
	} else {
		last_resolveDir = true;
		taken_branch_count++;
#ifdef EN_BB_FB_COUNT
		fb_over = 1;
#endif // EN_BB_FB_COUNT
#ifdef PC_TRACE
		fprintf(pc_trace, "%32s\n", "Taken Branch");
#endif
	}
	
    update_counters_pc_minus_1_branch();
	return;
}

// Not being used
static inline void close_pc_minus_1_branch (uint64_t pc)
{
#ifdef FTQ
    read_ftq_update_predictor();
#endif
	return;
}

static inline void close_pc_jump(uint32_t insn_raw)
{
	jump_count++;
	cti_count++;
	resolveDir = true;
      
#ifdef EN_BB_FB_COUNT
	bb_over = 1; fb_over = 1;
#endif // EN_BB_FB_COUNT

#ifdef PC_TRACE
	if ((((insn_raw & 0xffffff80) == 0x0))) // ECall
	{
		fprintf(pc_trace, "%32s\n", "ECALL type");
	} else if ((insn_raw == 0x100073) || (insn_raw == 0x200073) ||
                 (insn_raw == 0x30200073) ||
                 (insn_raw == 0x7b200073)) // EReturn
	{      
		fprintf(pc_trace, "%32s\n", "ERET type");
	}
	
	// Jump
	else if ((insn_raw & 0xf) == 0x7) {
    	if (((insn_raw & 0xf80) >> 7) == 0x0) {
        	fprintf(pc_trace, "%32s\n", "Return");
      	} else {
			fprintf(pc_trace, "%32s\n", "Reg based Fxn Call");
		}
	} else {
		fprintf(pc_trace, "%32s\n", "PC relative Fxn Call");
	}
#endif
}

static inline void start_pc_branch()
{
	branch_count++;
	cti_count++;
#ifdef EN_BB_FB_COUNT
	bb_over = 1;
#endif // EN_BB_FB_COUNT
}

static inline void close_pc_non_cti()
{
	resolveDir = false;
#ifdef PC_TRACE
	fprintf(pc_trace, "%32s\n", "Non - CTI");
#endif
}

void print_pc_insn (uint64_t pc, uint32_t insn_raw)
{
	fprintf(pc_trace, "%20lx\t|%20x\t", pc, insn_raw);
    if (insn_raw < 0x100) {
      fprintf(pc_trace, "\t|");
    } else {
      fprintf(pc_trace, "|");
    }
	return;
}

static inline void update_bb_br()
{
	if (bb_over == 1)
	{
		bb_size[running_bb_size]++;
		sum_bb_size+=running_bb_size;
		bb_count++;
		running_bb_size = 0;
	}
	else
	{
		running_bb_size++;
	}
	if (fb_over == 1)
	{
		fb_size[running_fb_size]++;
		sum_fb_size+=running_fb_size;
		fb_count++;
		if (bb_over == 1)
			running_fb_size = 0;
		else
			running_fb_size = 1;
	}
	else
	{
		running_fb_size++;
	}
}

static inline void handle_nb()
{
	if (predDir == resolveDir)
    	{
    		misprediction = false;
    		correct_prediction_count++;
    	}
    else 
    	{
    		misprediction = true;
    		misprediction_count++;
    	}
    if ( (insn == insn_t::non_cti) && (predDir == true) )
    	{
    		misscontrol_count++; 
    	}
}

/*
For getting a prediction and updating predictor, like hardware, there will be a race in software also, I think in any CC, I should read the predictor first with whatever history is there and update it after the read. That is what will happen in hardware.
*/
void handle_branch (uint64_t pc, uint32_t insn_raw) {

#ifdef DELETE1
				//if ( pc == 0x665512)
	    		printf ("pc = %lx\n", pc);
#endif // DELETE1

  	branchTarget = pc;
  	bb_over = 0, fb_over = 0;
  	misprediction = false;

  // for (int i = 0; i < m->ncpus; ++i)
  if (i0_done == true)
  {
	// If previous instruction was a branch. resolve that first
    if (last_insn == insn_t::branch) {
    	resolve_pc_minus_1_branch(pc);
    }
    
   	// Allocation - all info for last_insn
	// For branches - resolveDir and branchTarget are not available for insn,  
	// branchtarget is available only in next CC even for jumps, so unavail for insn 
	// Best - allocate for last_insn, in the beginning just after resolving a branch
#ifdef SUPERSCALAR
#ifdef FTQ    
	if (is_ftq_full() == false)
    {
    		allocate_ftq_entry(last_predDir, last_resolveDir, last_pc, branchTarget, bp); 
    		inst_index_in_fetch++;
    }
    else
    	{fprintf(stderr, "%s\n", "FTQ full"); }
    	
    // TODO Check - Read FTQ + Update predictor based on all info about last_insn
	read_ftq_update_predictor();
#else   // #ifndef SUPERSCALAR
    // TODO Check last_pc, resolveDir, predDir, branchTarget
    	bp.UpdatePredictor(last_pc, last_resolveDir, last_predDir, branchTarget);
#endif // FTQ
#endif // SUPERSCALAR
	}
    
    // At this point pc-1 handling is complete
    
    // Now start handling insn at pc
    
#ifdef PC_TRACE
	print_pc_insn (pc, insn_raw)
#endif
    
    if (((insn_raw & 0x7fff) == 0x73)) {
    // ECall and ERet
      insn = insn_t::jump;
      close_pc_jump(insn_raw);
    }
    else if (((insn_raw & 0x70) == 0x60)) {
      if (((insn_raw & 0xf) == 0x3)) {
      	insn = insn_t::branch;
      	start_pc_branch();
      } else // Jump
      {
      	insn = insn_t::jump;
      	close_pc_jump(insn_raw);
      }
    } else // Non CTI
    {
    	insn = insn_t::non_cti;
    	close_pc_non_cti();
    }
    
    // At this point - if present instruction - insn is not a branch - resolveDir contains info about pc, else pc - 1 

	// Update BB - BR Based on present instruction (exception - Branch)
#ifdef EN_BB_FB_COUNT
	update_bb_br();
#endif // EN_BB_FB_COUNT

	// Get predDir for pc
    predDir = bp.GetPrediction(pc);
    
    // Update counters - 
    // If this instruction is not a branch - straight case - predDir and resolveDir in sync
    // If this instruction is a branch - it will be resolved in next CC - hence, 
    
    if ( insn != insn_t::branch )
    {
    	handle_nb();
    }
    
        	#ifdef DELETE1
        	    if ( pc == 0x665512)
    	{
    		printf ("pc = %lx, insn_raw = %x, insn = %d, last_pc = %lx, last_insn_raw = %x, last_insn = %d, predDir = %d, resolveDir = %d\n", pc, insn_raw, insn, last_pc, last_insn_raw, last_insn, predDir, resolveDir);
    		}
    	#endif // DELETE1
    
    benchmark_instruction_count++;
    
    /******************************************************************
    As given, the simulator (dromajo) always fetches correctly, 
    i.e. the FETCH_WIDTH instructions being received by the CPU are the actual dynamic code execution stream,
    there is no wrong fetch, only wrong prediction
    hence, all entries in FTQ are for correct instructions
    hence -> ??? NO NEED TO NUKE UNLESS ITS REAL SUPERSCALAR RETURNING SEQUENTIAL INSTRUCTIONS FROM STATIC BINARY ???
    Hence, for now ->
    If branch is correctly predicted -> update MPKI + update predictor - but pr, pr, up, up
    If branch is mispredicted -> update MPKI + update predictor - pr, up, pr, up i.e. if misprediction, immediately update rather than after FETCH_WIDTH instructions and restart counter to collect FETCH_WIDTH instructions for next update
    
    Also, update after FETCH_WIDTH instructions means update after 1 CC in Superscalar
    Check if this needs to be delayed + split, i.e. global history to be updated after a few CC, but actual predictor table to be 		updated after commit
    *******************************************************************/	

    last_pc = pc;
    last_insn_raw = insn_raw;
    last_insn = insn;
    last_predDir = predDir;
    last_resolveDir = resolveDir;
    last_misprediction = misprediction;
    if (i0_done == false)
    {
    	i0_done = true;
    }
}
