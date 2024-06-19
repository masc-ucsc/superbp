#ifndef _PREDICTOR_H_
#define _PREDICTOR_H_

#include <assert.h>
#include <inttypes.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include <cstdint>

//#include "utils.h"

#include "batage.hpp"
#include "gshare.hpp"

class PREDICTOR {

  /*private:

   batage pred;
   histories hist;*/

public:
  batage pred;
  gshare fast_pred;
  histories hist;

  PREDICTOR(void);

  gshare_prediction& GetFastPrediction(uint64_t PC, int index, int tag) ;
  prediction& GetPrediction(uint64_t PC) ;
  
  void Updatetables(uint64_t PC, uint64_t fetch_pc, uint32_t offset_within_entry, bool resolveDir);
  void Updatehistory(bool resolveDir, uint64_t branchTarget);
  void TrackOtherInst(uint64_t PC, bool branchDir, uint64_t branchTarget);
  uint32_t get_allocs(int table);

};

#endif
