
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

// Update only for branches
void
PREDICTOR::Updatetables(uint64_t PC, bool resolveDir)
{
  pred.update(static_cast<uint32_t>(PC),resolveDir,hist, false);
}

void
PREDICTOR::Updatehistory(bool resolveDir, uint64_t branchTarget)
{
  hist.update(static_cast<uint32_t>(branchTarget),resolveDir);
}

// Update for unconditional cti-s
void
PREDICTOR::TrackOtherInst(uint64_t PC, bool branchDir, uint64_t branchTarget)
{
  // also update the global history with unconditional branches
  hist.update(branchTarget,true);
  return;
}
