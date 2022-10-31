
// FIXME: Bootstrap dromajo with superbp:gold and batage
#include "dromajo.h"

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

#define BRANCHPROF
#ifdef BRANCHPROF

#endif

FILE* pc_trace;

void print_branch_info (uint64_t last_pc, uint32_t insn_raw)
{
        static uint64_t last_last_pc;
	static uint8_t branch_flag = 0;
	//for (int i = 0; i < m->ncpus; ++i)
	{

 		if (branch_flag)
 		{
 			if (last_pc-last_last_pc == 4)
 				fprintf (pc_trace, "%32s\n", "Not Taken Branch");
 			else
 				fprintf (pc_trace, "%32s\n", "Taken Branch");
 			branch_flag = 0;
 		}

 		fprintf (pc_trace, "%20lx\t|%20x\t", last_pc, insn_raw);
 		if (insn_raw < 0x100)
 		{
 			fprintf (pc_trace, "\t|");
 		}
 		else
 		{
 			fprintf (pc_trace, "|");
 		}

 		if ( ((insn_raw & 0x7fff) == 0x73))
 		{
 			if (( ((insn_raw & 0xffffff80) == 0x0)) ) 				// ECall
 			{
 				fprintf (pc_trace, "%32s\n", "ECALL type");
 			}
 			else if ( (insn_raw == 0x100073) || (insn_raw == 0x200073) || (insn_raw == 0x30200073) || (insn_raw == 0x7b200073))		//EReturn
 			{
 				fprintf (pc_trace, "%32s\n", "ERET type");
 			}
 		}

 		else if (((insn_raw & 0x70) == 0x60))
 		{
 			if (((insn_raw & 0xf) == 0x3))
 			{
 				branch_flag = 1;
 			}
 			else // Jump
 			{
 				if((insn_raw & 0xf) == 0x7)
 				{
 					if  (((insn_raw & 0xf80)>>7) == 0x0)
 					{
 						fprintf (pc_trace, "%32s\n", "Return");
 					}
 					else
 					{
 						fprintf (pc_trace, "%32s\n", "Reg based Fxn Call");
 					}
 				}
 				else
 				{
 					fprintf (pc_trace, "%32s\n", "PC relative Fxn Call");
 				}
 			}
 		}
 		else // Non CTI
 		{
 			fprintf (pc_trace, "%32s\n", "Non - CTI");
 		}

 		//fprintf (pc_trace, "\n");
 		last_last_pc = last_pc;
 	}
}


int iterate_core(RISCVMachine *m, int hartid) {
    if (m->common.maxinsns-- <= 0)
        /* Succeed after N instructions without failure. */
        return 0;

    RISCVCPUState *cpu = m->cpu_state[hartid];

    /* Instruction that raises exceptions should be marked as such in
     * the trace of retired instructions.
     */
    uint64_t last_pc  = virt_machine_get_pc(m, hartid);
    int      priv     = riscv_get_priv_level(cpu);
    uint32_t insn_raw = -1;
    (void)riscv_read_insn(cpu, &insn_raw, last_pc);

#ifdef BRANCHPROF
	for (int i = 0; i < m->ncpus; ++i)
	{
		print_branch_info (last_pc, insn_raw);
	}
#endif // BRANCHPROF

    int keep_going = virt_machine_run(m, hartid);
    if (last_pc == virt_machine_get_pc(m, hartid))
        return 0;

    if (m->common.trace) {
        --m->common.trace;
        return keep_going;
    }

    fprintf(dromajo_stderr,
            "%d %d 0x%016" PRIx64 " (0x%08x)",
            hartid,
            priv,
            last_pc,
            (insn_raw & 3) == 3 ? insn_raw : (uint16_t)insn_raw);

    int iregno = riscv_get_most_recently_written_reg(cpu);
    int fregno = riscv_get_most_recently_written_fp_reg(cpu);

    if (cpu->pending_exception != -1)
        fprintf(dromajo_stderr,
                " exception %d, tval %016" PRIx64,
                cpu->pending_exception,
                riscv_get_priv_level(cpu) == PRV_M ? cpu->mtval : cpu->stval);
    else if (iregno > 0)
        fprintf(dromajo_stderr, " x%2d 0x%016" PRIx64, iregno, virt_machine_get_reg(m, hartid, iregno));
    else if (fregno >= 0)
        fprintf(dromajo_stderr, " f%2d 0x%016" PRIx64, fregno, virt_machine_get_fpreg(m, hartid, fregno));
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
    const char *port_name = NULL;
    int         port_num = 0;
    for (;;) {
        int option_index = 0;
        // clang-format off
        static struct option long_options[] = {
            {"gdbinit",                     required_argument, 0,  'G' } // CFG
        };
        // clang-format on

        int c = getopt_long(argc, argv, "", long_options, &option_index);
        if (c == -1) {
            break;
        }
        switch (c) {
            case 'G':
                port_name = strdup(optarg);
                port_num  = atoi(port_name);
                break;
            default: break;
        }
    };

    RISCVMachine *m = virt_machine_main(argc, argv);
    if (!m)
        return 1;

    if (port_num)
      gdb_stub(m, port_num);

#ifdef BRANCHPROF
        pc_trace = fopen("pc_trace.txt", "w+");
        if (pc_trace == nullptr)
        {
            	fprintf(dromajo_stderr, "\nerror: could not open pc_trace.txt for dumping trace\n");
            	exit(-3);
        }
        else
        {
        	fprintf(dromajo_stderr, "\nOpened dromajo_simpoint.bb for dumping trace\n");
        	fprintf (pc_trace, "%20s\t\t|%20s\t|%32s\n", "PC", "Instruction", "Instructiontype");
        }

#endif

    execution_start_ts = get_current_time_in_seconds();
    execution_progress_meassure = &m->cpu_state[0]->minstret;
    signal(SIGINT, sigintr_handler);

    int keep_going;
    do {
      keep_going = 0;
      for (int i = 0; i < m->ncpus; ++i) keep_going |= iterate_core(m, i);
    } while (keep_going);

    double t = get_current_time_in_seconds();

    for (int i = 0; i < m->ncpus; ++i) {
      int benchmark_exit_code = riscv_benchmark_exit_code(m->cpu_state[i]);
      if (benchmark_exit_code != 0) {
        fprintf(dromajo_stderr, "\nBenchmark exited with code: %i \n", benchmark_exit_code);
        return 1;
      }
    }

    fprintf(dromajo_stderr, "Simulation speed: %5.2f MIPS (single-core)\n",
            1e-6 * *execution_progress_meassure / (t - execution_start_ts));

    fprintf(dromajo_stderr, "\nPower off.\n");

    virt_machine_end(m);
#ifdef BRANCHPROF
    fclose (pc_trace);
#endif

return 0;
}

