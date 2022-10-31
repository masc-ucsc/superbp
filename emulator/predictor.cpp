
#include "fmt/format.h"

#include "predictor.hpp"
#include "batage.hpp"

#define VERBOSE


PREDICTOR::PREDICTOR(void)
{
#ifdef VERBOSE
  hist.printconfig();
  fmt::print("total bits = {}\n",pred.size()+hist.size());
#endif
}


bool
PREDICTOR::GetPrediction(uint64_t PC)
{
  return pred.predict(PC,hist);
}


void
PREDICTOR::UpdatePredictor(uint64_t PC, bool resolveDir, bool predDir, uint64_t branchTarget)
{
  pred.update(static_cast<uint32_t>(PC),resolveDir,hist, false);
  hist.update(static_cast<uint32_t>(branchTarget),resolveDir);
}


void
PREDICTOR::TrackOtherInst(uint64_t PC, bool branchDir, uint64_t branchTarget)
{
  // also update the global history with unconditional branches
  hist.update(branchTarget,true);
  return;
}
