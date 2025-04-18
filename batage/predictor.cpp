
#include "predictor.hpp"

#include "batage.hpp"
#include "branchprof.hpp"
#include "fmt/format.h"
#include "ftq.hpp"
#include "gshare.hpp"
#include "huq.hpp"

// #define VERBOSE

PREDICTOR::PREDICTOR(void)
    : pred(), hist(&pred), ftq_inst(&pred), huq_inst(&pred), branchprof_inst(&ftq_inst, &huq_inst, &pred, this), fast_pred() {
#ifdef VERBOSE
  hist.printconfig();
  fmt::print("total bits = {}\n", pred.size() + hist.size());
#endif
  // ftq_inst = new ftq(&pred);
  // huq_inst = new huq(&pred);
}

PREDICTOR::~PREDICTOR(void) {
  // exit_branchprof();
}

PREDICTOR::PREDICTOR(int& SBP_NUMG, int& LOG2FETCHWIDTH, int& NUM_TAKEN_BRANCHES, std::vector<uint32_t>& ORIG_ENTRIES_PER_TABLE,
                     std::vector<uint32_t>& INFO_PER_ENTRY, uint32_t& NUM_GSHARE_ENTRIES_SHIFT, uint8_t& NUM_PAGES_PER_GROUP,
                     uint8_t& PAGE_OFFSET_SIZE, uint8_t& PAGE_TABLE_INDEX_SIZE)
    : pred(), hist(&pred), ftq_inst(&pred), huq_inst(&pred), branchprof_inst(&ftq_inst, &huq_inst, &pred, this), fast_pred() {
#ifdef VERBOSE
  hist.printconfig();
  fmt::print("total bits = {}\n", pred.size() + hist.size());
#endif
  pred.SBP_NUMG           = SBP_NUMG;
  pred.LOG2FETCHWIDTH     = LOG2FETCHWIDTH;
  pred.NUM_TAKEN_BRANCHES = NUM_TAKEN_BRANCHES;

  pred.ORIG_ENTRIES_PER_TABLE = ORIG_ENTRIES_PER_TABLE;
  pred.INFO_PER_ENTRY         = INFO_PER_ENTRY;

  pred.populate_dependent_globals();
  pred.batage_resize();

  fast_pred.NUM_GSHARE_ENTRIES_SHIFT = NUM_GSHARE_ENTRIES_SHIFT;
  fast_pred.NUM_PAGES_PER_GROUP      = NUM_PAGES_PER_GROUP;
  fast_pred.PAGE_OFFSET_SIZE         = PAGE_OFFSET_SIZE;
  fast_pred.PAGE_TABLE_INDEX_SIZE    = PAGE_TABLE_INDEX_SIZE;

  fast_pred.populate_dependent_globals();
  fast_pred.gshare_resize();

  fprintf(stderr, "%s\n", "Finished Resize\n");
  hist.get_predictor_vars(&pred, &fast_pred);
  char bp_logfile[] = "log.txt";
  init_branchprof(bp_logfile);
}

void PREDICTOR::fetchBoundaryBegin(uint64_t PC) { branchprof_inst.fetchBoundaryBegin(PC); }

void PREDICTOR::fetchBoundaryEnd() { branchprof_inst.fetchBoundaryEnd(); }

void PREDICTOR::handle_insn(uint64_t pc, uint32_t insn_raw) { branchprof_inst.handle_insn(pc, insn_raw); }

void PREDICTOR::handle_insn_desesc(uint64_t pc, uint64_t branchTarget, uint8_t insn_type, bool taken,  bool* p_batage_pred, bool* p_batage_conf, bool* p_gshare_use) {
  branchprof_inst.handle_insn_desesc(pc, branchTarget, insn_type, taken, p_batage_pred, p_batage_conf, p_gshare_use);
  return;
}

gshare_prediction PREDICTOR::GetFastPrediction(uint64_t PC, int index, int tag) { return (fast_pred.predict(PC, index, tag)); }

prediction& PREDICTOR::GetPrediction(uint64_t PC) { return pred.predict_vec(PC, hist); }

// Update only for branches
void PREDICTOR::Updatetables(uint64_t PC, uint64_t fetch_pc, uint32_t offset_within_packet, bool resolveDir) {
  pred.update(PC, fetch_pc, offset_within_packet, resolveDir, hist, false);
}

// TODO Check if this needs correct offset
void PREDICTOR::Updatehistory(bool resolveDir, uint64_t branchTarget) {
  hist.update(static_cast<uint32_t>(branchTarget), resolveDir);
}

// Update for unconditional cti-s
void PREDICTOR::TrackOtherInst(uint64_t PC, bool branchDir, uint64_t branchTarget) {
  // also update the global history with unconditional branches
  hist.update(branchTarget, true);
  return;
}

uint32_t PREDICTOR::get_allocs(int table) { return (pred.get_allocs(table)); }

void PREDICTOR::init_branchprof(char* logfile) { branchprof_inst.branchprof_init(logfile); }
void PREDICTOR::exit_branchprof() { branchprof_inst.branchprof_exit(this); }
