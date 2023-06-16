
// FIXME: Bootstrap dromajo with superbp:gold and batage
#include "dromajo.h"

#include "predictor.hpp"
#include "emulator.hpp"
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <inttypes.h>
#include <net/if.h>
#include <signal.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>
#include <stdbool.h>

#define BRANCHPROF
#ifdef BRANCHPROF


//#define SKIP_COUNT 2500000000 // for x264
//#define SKIP_COUNT 1000000000 //  for perlbench_splitmail
#define SKIP_COUNT 4000000000 // for all other benchmarks
/*#define RUN_COUNT 1000000000 - desired
#define RUN_COUNT 1000000 - actually used - sent as part of maxinsns to dromajo*/

PREDICTOR bp;
bool predDir, resolveDir;
uint64_t branchTarget;
bool taken_flag;
//#define PC_TRACE

FILE* pc_trace;

#ifdef EN_BB_BR_COUNT
uint64_t bb_count[50], br_count[250];
uint8_t running_bb_count, running_br_count;
#endif // EN_BB_BR_COUNT

#ifdef SUPERSCALAR
#ifdef FTQ
#include "ftq.hpp"
ftq_entry ftq_data; // Only 1 instance - assuming the updates for superscalar will be done 1 by 1, so they may reuse the same instance
#endif // FTQ
#endif // SUPERSCALAR

uint64_t correct_prediction_count, misprediction_count, instruction_count;

void copy_ftq_data_to_predictor(ftq_entry* ftq_data_ptr)
{
    bp.pred.hit 		= ftq_data_ptr->hit;
    bp.pred.s 			= ftq_data_ptr->s;
    bp.pred.meta 		= ftq_data_ptr->meta;
    bp.pred.bp 			= ftq_data_ptr->bp;
    bp.pred.bi 			= ftq_data_ptr->bi;
    bp.pred.bi2 		= ftq_data_ptr->bi2;
	memcpy(bp.pred.b, &((ftq_data_ptr->b)[0]), sizeof(int) * (ftq_data_ptr->b).size());
	memcpy(bp.pred.b2, &((ftq_data_ptr->b2)[0]), sizeof(int) * (ftq_data_ptr->b2).size());
	memcpy(bp.pred.gi, &((ftq_data_ptr->gi)[0]), sizeof(int) * NUMG);
}

/*
For getting a prediction and updating predictor, like hardware, there will be a race in software also, I think in any CC, I should read the predictor first with whatever history is there and update it after the read. That is what will happen in hardware.
*/

void print_branch_info(uint64_t pc, uint32_t insn_raw) {
  static uint64_t last_pc;
  static uint8_t branch_flag = 0;
  bool misprediction;
  
  #ifdef SUPERSCALAR
  static uint8_t inst_index_in_fetch;
  #endif

  #ifdef EN_BB_BR_COUNT
  uint8_t bb_over = 0, br_over = 0;
  #endif // EN_BB_BR_COUNT

  branchTarget = pc;

  // for (int i = 0; i < m->ncpus; ++i)
  {

    if (branch_flag) {
      if (pc - last_pc == 4) {
	      #ifdef PC_TRACE
        fprintf(pc_trace, "%32s\n", "Not Taken Branch");
 				#endif
        resolveDir = false;
      } else {
	      #ifdef PC_TRACE
        fprintf(pc_trace, "%32s\n", "Taken Branch");
 				#endif
        resolveDir = true;
        #ifdef EN_BB_BR_COUNT
        br_over = 1;
        #endif // EN_BB_BR_COUNT
      }
      branch_flag = 0;
    }
    
 		#ifdef PC_TRACE
    fprintf(pc_trace, "%20lx\t|%20x\t", pc, insn_raw);
    if (insn_raw < 0x100) {
      fprintf(pc_trace, "\t|");
    } else {
      fprintf(pc_trace, "|");
    }
 		#endif
    
    if (((insn_raw & 0x7fff) == 0x73)) {
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
 			#endif
      resolveDir = true;
      #ifdef EN_BB_BR_COUNT
      bb_over = 1; br_over = 1;
      #endif // EN_BB_BR_COUNT
    }

    else if (((insn_raw & 0x70) == 0x60)) {
      if (((insn_raw & 0xf) == 0x3)) {
        branch_flag = 1;
        #ifdef EN_BB_BR_COUNT
        bb_over = 1;
        #endif // EN_BB_BR_COUNT
      } else // Jump
      {
        #ifdef PC_TRACE
        if ((insn_raw & 0xf) == 0x7) {
          if (((insn_raw & 0xf80) >> 7) == 0x0) {
            fprintf(pc_trace, "%32s\n", "Return");
          } else {
            fprintf(pc_trace, "%32s\n", "Reg based Fxn Call");
          }
        } else {
          fprintf(pc_trace, "%32s\n", "PC relative Fxn Call");
        }
 				#endif
        resolveDir = true;
        #ifdef EN_BB_BR_COUNT
        bb_over = 1; br_over = 1;
        #endif // EN_BB_BR_COUNT
      }
    } else // Non CTI
    {
 			#ifdef PC_TRACE
      fprintf(pc_trace, "%32s\n", "Non - CTI");
 			#endif
      resolveDir = false;
    }

    #ifdef EN_BB_BR_COUNT
    if (bb_over == 1)
      {
        bb_count[running_bb_count]++;
        running_bb_count = 0;
      }
      else
      {
        running_bb_count++;
      }
      if (br_over == 1)
      {
        br_count[running_br_count]++;
        if (bb_over == 1)
          running_br_count = 0;
        else
          running_br_count = 1;
      }
      else
      {
        running_br_count++;
      }
      #endif // EN_BB_BR_COUNT

    predDir = bp.GetPrediction(last_pc);
    
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
    //instruction_count++;

    #ifdef SUPERSCALAR
	#ifdef FTQ
	if (is_ftq_full() == false)
    	allocate_ftq_entry(predDir, resolveDir, last_pc, branchTarget, bp);
    else
    	fprintf(stderr, "%s\n", "FTQ full");
    
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
    inst_index_in_fetch++;
    if ( (inst_index_in_fetch == FETCH_WIDTH) || misprediction )
    {

    	for (int i = 0; misprediction ? (i < inst_index_in_fetch) : (i < FETCH_WIDTH); i++)
    	{
    		if (!is_ftq_empty()) 
    		{
    			get_ftq_data(&ftq_data);
    			
    			last_pc = ftq_data.pc;
    			resolveDir = ftq_data.resolveDir;
    			predDir = ftq_data.predDir;
    			branchTarget = ftq_data.branchTarget;
    			
    			// Store the read predictor fields into the predictor
    			copy_ftq_data_to_predictor(&ftq_data);

    			// Update - Check history update
    			bp.UpdatePredictor(last_pc, resolveDir, predDir, branchTarget);
    		}
    		else
    		{
    	 		fprintf(stderr, "%s\n", "Pop on empty ftq");
    		}
    	}
    	inst_index_in_fetch = 0;
    }
    #endif // FTQ
    #else // SUPERSCALAR
    	bp.UpdatePredictor(last_pc, resolveDir, predDir, branchTarget);
    #endif // SUPERSCALAR

    last_pc = pc;
  }

}
#endif

int iterate_core(RISCVMachine *m, int hartid) {
  if (m->common.maxinsns-- <= 0)
    /* Succeed after N instructions without failure. */
    return 0;

  RISCVCPUState *cpu = m->cpu_state[hartid];

	
  	/* Instruction that raises exceptions should be marked as such in
   	* the trace of retired instructions.
   	*/
  	uint64_t pc = virt_machine_get_pc(m, hartid);
  	int priv = riscv_get_priv_level(cpu);
  	uint32_t insn_raw = -1;
  	(void)riscv_read_insn(cpu, &insn_raw, pc);
  	instruction_count++;
#ifdef BRANCHPROF
  	if (instruction_count >= SKIP_COUNT)
	{
  		for (int i = 0; i < m->ncpus; ++i) {
    	print_branch_info(pc, insn_raw);
  		}
	}
#endif // BRANCHPROF

  int keep_going = virt_machine_run(m, hartid);
  if (pc == virt_machine_get_pc(m, hartid))
    return 0;

  if (m->common.trace) {
    --m->common.trace;
    return keep_going;
  }

  fprintf(dromajo_stderr, "%d %d 0x%016" PRIx64 " (0x%08x)", hartid, priv,
          pc, (insn_raw & 3) == 3 ? insn_raw : (uint16_t)insn_raw);

  int iregno = riscv_get_most_recently_written_reg(cpu);
  int fregno = riscv_get_most_recently_written_fp_reg(cpu);

  if (cpu->pending_exception != -1)
    fprintf(dromajo_stderr, " exception %d, tval %016" PRIx64,
            cpu->pending_exception,
            riscv_get_priv_level(cpu) == PRV_M ? cpu->mtval : cpu->stval);
  else if (iregno > 0)
    fprintf(dromajo_stderr, " x%2d 0x%016" PRIx64, iregno,
            virt_machine_get_reg(m, hartid, iregno));
  else if (fregno >= 0)
    fprintf(dromajo_stderr, " f%2d 0x%016" PRIx64, fregno,
            virt_machine_get_fpreg(m, hartid, fregno));
  else
    for (int i = 31; i >= 0; i--)
      if (cpu->most_recently_written_vregs[i]) {
        fprintf(dromajo_stderr, " v%2d 0x", i);
        for (int j = VLEN / 8 - 1; j >= 0; j--) {
          fprintf(dromajo_stderr, "%02" PRIx8, cpu->v_reg[i][j]);
        }
      }

  putc('\n', dromajo_stderr);

  return keep_going;
}

static double execution_start_ts;
static uint64_t *execution_progress_meassure;

static void sigintr_handler(int dummy) {
  double t = get_current_time_in_seconds();
  fprintf(dromajo_stderr, "Simulation speed: %5.2f MIPS (single-core)\n",
          1e-6 * *execution_progress_meassure / (t - execution_start_ts));
  exit(1);
}

int main(int argc, char **argv) {

  
  RISCVMachine *m = virt_machine_main(argc, argv);
  if (!m)
    return 1;

#ifdef BRANCHPROF
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
  
#endif // BRANCHPROF

  execution_start_ts = get_current_time_in_seconds();
  execution_progress_meassure = &m->cpu_state[0]->minstret;
  signal(SIGINT, sigintr_handler);

  int keep_going;
  do {
    keep_going = 0;
    for (int i = 0; i < m->ncpus; ++i)
      keep_going |= iterate_core(m, i);
  } while (keep_going);

  double t = get_current_time_in_seconds();

  for (int i = 0; i < m->ncpus; ++i) {
    int benchmark_exit_code = riscv_benchmark_exit_code(m->cpu_state[i]);
    if (benchmark_exit_code != 0) {
      fprintf(dromajo_stderr, "\nBenchmark exited with code: %i \n",
              benchmark_exit_code);
      return 1;
    }
  }

  fprintf(dromajo_stderr, "Simulation speed: %5.2f MIPS (single-core)\n",
          1e-6 * *execution_progress_meassure / (t - execution_start_ts));

  fprintf(dromajo_stderr, "\nPower off.\n");

  virt_machine_end(m);
#ifdef BRANCHPROF
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
  fprintf (pc_trace, "Instruction Count = %lu, Correct prediciton Count = %lu, mispredction count = %lu and misprediction rate = %lf and MPKI = %lf \n", instruction_count, correct_prediction_count, misprediction_count, (double)misprediction_count/(double)(correct_prediction_count + misprediction_count) *100,  (double)misprediction_count/(double)instruction_count *1000 );
  
  #ifdef EN_BB_BR_COUNT
  fprintf(pc_trace, "bb_count = \n");
  for (int i = 0; i < 5; i++)
  {
    for (int j = 0; j < 10; j++)
    {
      fprintf(pc_trace, "%lu, ", bb_count[i*10 + j]);
    }
    fprintf(pc_trace, "\n");
  }
  fprintf(pc_trace, "br_count = \n");
  for (int i = 0; i < 25; i++)
  {
    for (int j = 0; j < 10; j++)
    {
      fprintf(pc_trace, "%lu, ", br_count[i*10 + j]);
    }
    fprintf(pc_trace, "\n");
  }
  #endif // EN_BB_BR_COUNT

  fclose(pc_trace);
#endif // BRANCHPROF

  return 0;
}
