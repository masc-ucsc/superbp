#ifndef _PREDICTOR_H_
#define _PREDICTOR_H_

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <inttypes.h>
#include <math.h>

#include <cstdint>

//#include "utils.h"

#include "batage.hpp"


class PREDICTOR {

 private:

  batage pred;
  histories hist;

 public:

  PREDICTOR(void);
  bool GetPrediction(uint64_t PC);  
  void UpdatePredictor(uint64_t PC, bool resolveDir, bool predDir, uint64_t branchTarget);
  void TrackOtherInst(uint64_t PC, bool branchDir, uint64_t branchTarget);

};



#endif

