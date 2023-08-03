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

 /*private:

  batage pred;
  histories hist;*/

 public:
 
   batage pred;
   histories hist;

  PREDICTOR(void);
  bool GetPrediction(uint64_t PC);  
  void Updatetables(uint64_t PC, bool resolveDir);
  void Updatehistory(bool resolveDir, uint64_t branchTarget);
  void TrackOtherInst(uint64_t PC, bool branchDir, uint64_t branchTarget);

};



#endif

