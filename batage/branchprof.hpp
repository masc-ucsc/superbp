#pragma once

//#include "emulator.hpp"
//#include "predictor.hpp"
#include "batage.hpp"
#include "utils.hpp"
//#include "ftq.hpp"
//#include "huq.hpp"
#define FTQ

class PREDICTOR;
class ftq_entry;
class ftq;
class huq;
class gshare;

class branchprof {
public :
ftq* ftq_p;
huq* huq_p;
batage* pred;
PREDICTOR* bp;

uint8_t FETCH_WIDTH;
int16_t inst_index_in_fetch = 0, last_inst_index_in_fetch; // starts from 0 after every redirect
uint64_t last_pc;
uint32_t last_insn_raw;
insn_t last_insn, insn;
bool last_misprediction, misprediction;
bool i0_done = false;
uint64_t correct_prediction_count, misprediction_count;
uint64_t benchmark_instruction_count; // total # of benchmarked instructions only - excluding skip instructions
bool predDir, last_predDir, resolveDir, last_resolveDir;
uint64_t branchTarget;
                                      
uint64_t num_fetches;
                                      
uint64_t fetch_pc, last_fetch_pc;

 branchprof(ftq* f, huq* h, batage* b, PREDICTOR* bp_ptr);
/*{
	ftq_p = f;
	huq_p = h;
}*/

void branchprof_init(char* logfile);
void print_pc_insn(uint64_t pc, uint32_t insn_raw) ;
void update_bb_fb() ;
void handle_nb() ;
void close_pc_non_cti();
void start_pc_branch();
void resolve_pc_minus_1_branch(uint64_t pc);
void close_pc_jump(uint64_t pc, uint32_t insn_raw);
void close_pc_minus_1_branch(uint64_t pc);
void read_ftq_update_predictor();
void copy_ftq_data_to_predictor(PREDICTOR* bp, ftq_entry *ftq_data_ptr);
void update_counters_pc_minus_1_branch();
void get_gshare_prediction(uint64_t temp_pc, int index, int tag);
void resolve_gshare(int i, bool update_predDir, bool update_resolveDir, uint64_t target);
void branchprof_exit(PREDICTOR*);
//void branchprof_exit(char* logfile);
#ifdef FTQ
//static inline void copy_ftq_data_to_predictor(ftq_entry *ftq_data_ptr);
#endif // FTQ
void handle_insn(uint64_t pc, uint32_t insn_raw);

void resolve_pc_minus_1_branch_desesc(uint64_t pc);
void close_pc_jump_desesc(uint64_t pc);
void update_counters_desesc();
bool handle_insn_desesc(uint64_t pc, uint8_t insn_type, bool taken);

void fetchBoundaryBegin(uint64_t PC);
void fetchBoundaryEnd();

};
