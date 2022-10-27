#include "predictor.h"
#include "batage.cc"

#define VERBOSE


PREDICTOR::PREDICTOR(void)
{
#ifdef VERBOSE
  hist.printconfig();
  printf("total bits = %d\n",pred.size()+hist.size());
#endif
}


bool
PREDICTOR::GetPrediction(UINT64 PC)
{
  return pred.predict(PC,hist);
}


void
PREDICTOR::UpdatePredictor(UINT64 PC, OpType opType, bool resolveDir, bool predDir, UINT64 branchTarget)
{
  pred.update(PC,resolveDir,hist);
  hist.update(branchTarget,resolveDir);
}


void
PREDICTOR::TrackOtherInst(UINT64 PC, OpType opType, bool branchDir, UINT64 branchTarget)
{
  // also update the global history with unconditional branches
  hist.update(branchTarget,true);
  return;
}
