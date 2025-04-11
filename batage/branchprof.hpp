#pragma once

#define FTQ
#define BATAGE
#define GSHARE
// #include "emulator.hpp"
// #include "predictor.hpp"
#include "batage.hpp"
#include "gshare.hpp"
#include "utils.hpp"
#include "ftq.hpp"
#include "huq.hpp"

class PREDICTOR;
/*class ftq_entry;
class ftq;
class huq;
class huq_entry;
class gshare;*/

class branchprof {
public:
  ftq*       ftq_p;
  huq*       huq_p;
  batage*    pred;
  PREDICTOR* bp;

  uint8_t  FETCH_WIDTH;
  uint64_t last_pc;
  uint32_t last_insn_raw;
  insn_t   last_insn, insn;
  bool     last_misprediction, misprediction;
  bool     i0_done = false;
  uint64_t correct_prediction_count, misprediction_count;
  uint64_t benchmark_instruction_count;  // total # of benchmarked instructions only - excluding skip instructions
  bool     predDir, last_predDir, resolveDir, last_resolveDir;
  uint64_t branchTarget;

  uint64_t num_fetches;

  uint64_t fetch_pc, last_fetch_pc;

  #ifdef GSHARE
// gshare allocate
gshare_prediction gshare_pred_inst, last_gshare_pred_inst;

bool              gshare_pos1_correct, gshare_pos0_correct, gshare_prediction_correct;
uint8_t           highconf_ctr = 0;
vector<uint64_t>  gshare_PCs;
vector<uint8_t>   gshare_poses;
uint64_t          num_gshare_allocations, num_gshare_predictions, num_gshare_correct_predictions, gshare_batage_1st_pred_mismatch,
    gshare_batage_2nd_pred_mismatch, num_gshare_jump_correct_predictions, num_gshare_jump_mispredictions, num_gshare_tag_match;
int gshare_index;
int gshare_tag;
#endif
  
#ifdef FTQ
#include "ftq.hpp"
#include "huq.hpp"

ftq_entry ftq_data;  // Only 1 instance - assuming the updates for superscalar
                     // will be done 1 by 1, so they may reuse the same instance
huq_entry huq_data;

#endif  // FTQ
  
  #if (defined(GSHARE) || defined(Ideal_2T))
bool gshare_tracking     = false;
bool highconfT_in_packet = false;
#endif  // GSHARE || Ideal_2T

#ifdef Ideal_2T
uint8_t pos_0;
#endif  // Ideal_2T

  uint8_t  partial_pop;
uint32_t update_gshare_index, update_gshare_tag;

prediction batage_prediction;

// index into ftq, used to interact with ftq, increases by 1 for each instruction for every instruction pushed to ftq till
// fetchboundaryend
uint8_t inst_index_in_fetch, last_inst_index_in_fetch;  // starts from 0 after every redirect
// offset from fetch_pc, used for pos
uint8_t inst_offset_from_fpc, last_inst_offset_from_fpc;  // starts from 0 after every redirect

  branchprof(ftq* f, huq* h, batage* b, PREDICTOR* bp_ptr);
  /*{
          ftq_p = f;
          huq_p = h;
  }*/

  void branchprof_init(char* logfile);
  void print_pc_insn(uint64_t pc, uint32_t insn_raw);
  void update_bb_fb();
  void handle_nb();
  void close_pc_non_cti();
  void start_pc_branch();
  void resolve_pc_minus_1_branch(uint64_t pc);
  void close_pc_jump(uint64_t pc, uint32_t insn_raw);
  void close_pc_minus_1_branch(uint64_t pc);
  void read_ftq_update_predictor();
  void copy_ftq_data_to_predictor(PREDICTOR* bp, ftq_entry* ftq_data_ptr);
  void update_counters_pc_minus_1_branch();
  void get_gshare_prediction(uint64_t temp_pc, int index, int tag);
  void resolve_gshare(uint8_t i, bool update_predDir, bool update_resolveDir, uint64_t target);
  void branchprof_exit(PREDICTOR*);
// void branchprof_exit(char* logfile);
#ifdef FTQ
// static inline void copy_ftq_data_to_predictor(ftq_entry *ftq_data_ptr);
#endif  // FTQ
  void handle_insn(uint64_t pc, uint32_t insn_raw);

  void resolve_pc_minus_1_branch_desesc(uint64_t pc);
  void close_pc_jump_desesc(uint64_t pc);
  void update_counters_desesc();
  void handle_insn_desesc(uint64_t pc, uint64_t branchTarget, uint8_t insn_type, bool taken,  bool* p_batage_pred, bool* p_batage_conf, bool* p_gshare_use) ;

  void fetchBoundaryBegin(uint64_t PC);
  void fetchBoundaryEnd();
  void fetchBoundaryEnd_dromajo();
};
