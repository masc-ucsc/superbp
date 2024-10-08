
#include "fmt/format.h"

#include "batage.hpp"
#include "predictor.hpp"

#define VERBOSE

PREDICTOR::PREDICTOR(void)  : pred(), hist(&pred){
#ifdef VERBOSE
  hist.printconfig();
  fmt::print("total bits = {}\n", pred.size() + hist.size());
#endif
}

void PREDICTOR::fetchBoundaryBegin(uint64_t PC)
{
	pred.fetchBoundaryBegin(PC);
}

void PREDICTOR::fetchBoundaryEnd()
{
	pred.fetchBoundaryEnd();
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
