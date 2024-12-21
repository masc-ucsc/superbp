#ifndef _PREDICTOR_H_
#define _PREDICTOR_H_

#include <assert.h>
#include <inttypes.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include <cstdint>

#define BRANCHPROF
#define BATAGE
#define SUPERSCALAR
#define FTQ
#define GSHARE

#include "batage.hpp"
#include "branchprof.hpp"
#include "ftq.hpp"
#include "gshare.hpp"
#include "huq.hpp"
#include "utils.hpp"

// class ftq;
// class huq;

class PREDICTOR {
  /*private:

   batage pred;
   histories hist;*/

public:
  batage     pred;
  gshare     fast_pred;
  histories  hist;
  ftq        ftq_inst;
  huq        huq_inst;
  branchprof branchprof_inst;

  PREDICTOR(void);
  ~PREDICTOR(void);
  PREDICTOR (int SBP_NUMG, int LOG2FETCHWIDTH, int NUM_TAKEN_BRANCHES);

  void fetchBoundaryBegin(uint64_t PC);
  void fetchBoundaryEnd();
  void handle_insn(uint64_t pc, uint32_t insn_raw);
  bool handle_insn_desesc(uint64_t pc, uint8_t insn_type, bool taken);

  gshare_prediction& GetFastPrediction(uint64_t PC, int index, int tag);
  prediction&        GetPrediction(uint64_t PC);

  void     Updatetables(uint64_t PC, uint64_t fetch_pc, uint32_t offset_within_entry, bool resolveDir);
  void     Updatehistory(bool resolveDir, uint64_t branchTarget);
  void     TrackOtherInst(uint64_t PC, bool branchDir, uint64_t branchTarget);
  uint32_t get_allocs(int table);

  void init_branchprof(char* logfile);
  void exit_branchprof();
};

#endif
