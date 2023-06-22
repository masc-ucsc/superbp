
#include "emulator.hpp"
#ifdef FTQ
#include "ftq.hpp"
#endif //FTQ


void branchprof_init();
void branchprof_exit();
#ifdef FTQ
void copy_ftq_data_to_predictor(ftq_entry* ftq_data_ptr);
#endif // FTQ
void handle_branch(uint64_t pc, uint32_t insn_raw);
