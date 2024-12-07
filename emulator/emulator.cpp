
// FIXME: Bootstrap dromajo with superbp:gold and batage
#include "emulator.hpp"
#include "dromajo.h"
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <inttypes.h>
#include <net/if.h>
#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#ifdef BRANCHPROF
//#include "branchprof.hpp"
#include "predictor.hpp"
PREDICTOR bp;
uint64_t maxinsns = 0;
uint64_t skip_insns = 0;
char* bp_logfile = NULL;
#endif

uint64_t instruction_count; // total # of instructions - including skipped nd
                            // benchmark instructions

int iterate_core(RISCVMachine *m, int hartid) {

  /* Succeed after N instructions without failure.*/
#ifdef BRANCHPROF
  if (instruction_count >= skip_insns) {
    // if (m->common.maxinsns-- <= 0)
    if (maxinsns-- == 0)
      return 0;
  }
#else
  if (m->common.maxinsns-- <= 0)
    return 0;
#endif

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
  if (instruction_count >= skip_insns) {
    for (int i = 0; i < m->ncpus; ++i) {
      bp.handle_insn(pc, insn_raw);
    }
  }
#endif // BRANCHPROF

  int n_cycles = 1; // Check - this should be taken from dromajo
  int keep_going = virt_machine_run(m, hartid, n_cycles);
  if (pc == virt_machine_get_pc(m, hartid))
    return 0;

  if (m->common.trace) {
    --m->common.trace;
    return keep_going;
  }

  fprintf(dromajo_stderr, "%d %d 0x%016" PRIx64 " (0x%08x)", hartid, priv, pc,
          (insn_raw & 3) == 3 ? insn_raw : (uint16_t)insn_raw);

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

#define HACKERY
#ifdef HACKERY
// Handles just one command line argument in emulator.cpp, removes it from the argv and sends remaining to dromajo
/*
int c;
while (1) {
        int this_option_optind = optind ? optind : 1;
        int option_index = 0;
        static struct option long_options[] = {
            {"bp_logfile",     required_argument, 0,  0 }
        };

       c = getopt_long(argc, argv, "f:0:",
                 long_options, &option_index);
        if (c == -1)
            break;

       switch (c) {
        case 0:
            printf("option %s", long_options[option_index].name);
            if (optarg)
                printf(" with arg %s", optarg);
            printf("\n");
            bp_logfile = strdup( optarg);
            printf("bp_logfile with value '%s'\n", bp_logfile);
            break;

       case 'f':
            bp_logfile = strdup(optarg);
            printf("bp_logfile with value '%s'\n", bp_logfile);
            break;

       case '?':
            break;

       default:
            printf("?? getopt returned character code 0%o ??\n", c);
        }
    }*/

std::vector<std::string> arguments;
std::string argumentToRemove = "--bp_logfile";
bool skipNext = false; // Flag to skip the next optarg
 
for (int i = 0; i < argc; ++i) {
        std::string currentArg = argv[i];

        // Check if the current argument matches the one to be removed
        if (currentArg == argumentToRemove) {
            // If it matches, set the skipNext flag and continue to the next iteration
            skipNext = true;
            continue;
        }

        // If skipNext is set, skip this argument (optarg) and reset the flag
        if (skipNext) {
            bp_logfile = new char[currentArg.size() + 1];
            strcpy(bp_logfile, currentArg.c_str());
            fprintf (stderr, "bplogfile = %s\n", bp_logfile);
            skipNext = false;
            continue;
        }

        // If it doesn't match and skipNext is not set, add it to the modified arguments
        arguments.push_back(currentArg);    
    }
    
    
          // Create a new char* array to hold the modified arguments
         char** newArgv = new char*[arguments.size() + 1];
         
         // Copy the modified arguments from the vector to the new char* array
    	for (size_t i = 0; i < arguments.size(); ++i) {
        	newArgv[i] = new char[arguments[i].size() + 1];
        	strcpy(newArgv[i], arguments[i].c_str());
   	}
   	
   	// Set the last element of the new argv to nullptr (required by convention)
    	newArgv[arguments.size()] = nullptr;
    	
    	 // Update argc to reflect the number of modified arguments
    	int newArgc = static_cast<int>(arguments.size());
    	
    	 fprintf (stderr, "New args\n");
   for (int i = 0; i < newArgc; ++i) {
        fprintf ( stderr, "%s \n", newArgv[i]);
    }
	fprintf (stderr, "New args over\n");

    // Update argv and argc to point to the new array
    //argv = newArgv;
    //argc = newArgc;

#endif // HACKERY


#ifdef BRANCHPROF
  //bp.branchprof_inst.branchprof_init(bp_logfile);
  fprintf(stderr, "%s\n", "Reading Env variables\n");
	bp.pred.read_env_variables();
	bp.pred.populate_dependent_globals();
	bp.pred.batage_resize();
	fprintf(stderr, "%s\n", "Finished Resize\n");
	bp.hist.get_predictor_vars(&(bp.pred));
  bp.init_branchprof(bp_logfile);
#endif // BRANCHPROF

  //bp = new(....)
  RISCVMachine *m = virt_machine_main(newArgc, newArgv);
  if (!m)
    return 1;
    
#ifdef BRANCHPROF
  maxinsns = m->common.maxinsns;
  skip_insns = m->common.skip_insns;
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
  //bp.branchprof_inst.branchprof_exit();
  bp.exit_branchprof();
//branchprof_exit(bp_logfile);
#endif // BRANCHPROF

  return 0;
}
