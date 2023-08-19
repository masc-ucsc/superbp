
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
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#ifdef BRANCHPROF
#include "branchprof.hpp"
#include "predictor.hpp"
uint64_t maxinsns;
uint64_t skip_insns;
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
      handle_branch(pc, insn_raw);
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

  RISCVMachine *m = virt_machine_main(argc, argv);
  if (!m)
    return 1;

#ifdef BRANCHPROF
  branchprof_init();
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
  branchprof_exit();
#endif // BRANCHPROF

  return 0;
}
