#ifndef _PREDICTOR_H_
#define _PREDICTOR_H_

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <inttypes.h>
#include <math.h>
#include "utils.h"

#include "batage.h"



class PREDICTOR {

 private:

  batage pred;
  histories hist;

 public:

  PREDICTOR(void);
  bool GetPrediction(UINT64 PC);  
  void UpdatePredictor(UINT64 PC, OpType opType, bool resolveDir, bool predDir, UINT64 branchTarget);
  void TrackOtherInst(UINT64 PC, OpType opType, bool branchDir, UINT64 branchTarget);

};



#endif

