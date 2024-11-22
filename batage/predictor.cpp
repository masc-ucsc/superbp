
#include "fmt/format.h"

#include "batage.hpp"
#include "predictor.hpp"
#include "ftq.hpp"
#include "huq.hpp"
#include "branchprof.hpp"
#include "gshare.hpp"

#define VERBOSE

PREDICTOR::PREDICTOR(void)  : pred(), hist(&pred), ftq_inst(&pred), huq_inst(&pred), branchprof_inst(&ftq_inst, &huq_inst, &pred, this), fast_pred() {
#ifdef VERBOSE
  hist.printconfig();
  fmt::print("total bits = {}\n", pred.size() + hist.size());
#endif

//ftq_inst = new ftq(&pred);
//huq_inst = new huq(&pred);

}

void PREDICTOR::fetchBoundaryBegin(uint64_t PC)
{
	branchprof_inst.fetchBoundaryBegin(PC);
}

void PREDICTOR::fetchBoundaryEnd()
{
	branchprof_inst.fetchBoundaryEnd();
}

 void PREDICTOR::handle_insn(uint64_t pc, uint32_t insn_raw)
{
	branchprof_inst.handle_insn(pc, insn_raw);
}

bool PREDICTOR::handle_insn_t(uint64_t pc, uint8_t insn_type)
{
	return branchprof_inst.handle_insn_t(pc, insn_type);
}

gshare_prediction& PREDICTOR::GetFastPrediction(uint64_t PC, int index, int tag) 
  {return (fast_pred.predict(PC, index, tag));}

prediction& PREDICTOR::GetPrediction(uint64_t PC) { return pred.predict_vec(PC, hist); }

// Update only for branches
void PREDICTOR::Updatetables(uint64_t PC, uint64_t fetch_pc, uint32_t offset_within_entry, bool resolveDir) {
  pred.update(static_cast<uint32_t>(PC), static_cast<uint32_t>(fetch_pc), offset_within_entry, resolveDir, hist, false);
}

// TODO Check if this needs correct offset
void PREDICTOR::Updatehistory(bool resolveDir, uint64_t branchTarget) {
  hist.update(static_cast<uint32_t>(branchTarget), resolveDir);
}

// Update for unconditional cti-s
void PREDICTOR::TrackOtherInst(uint64_t PC, bool branchDir,
                               uint64_t branchTarget) {
  // also update the global history with unconditional branches
  hist.update(branchTarget, true);
  return;
}

uint32_t PREDICTOR::get_allocs(int table)
{
	return (pred.get_allocs(table));
}

 void PREDICTOR::init_branchprof(char* logfile)
 {
 	branchprof_inst.branchprof_init(logfile);
 }
 void PREDICTOR::exit_branchprof()
 {
 	branchprof_inst.branchprof_exit(this);
 }


