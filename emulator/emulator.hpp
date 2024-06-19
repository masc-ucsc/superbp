#pragma once

#define BRANCHPROF

// Change this to change predictor - may later want to make this a command line
// parameter - argument to main
#define BATAGE

#define SUPERSCALAR
#ifdef SUPERSCALAR
#define LOG2_FETCH_WIDTH (4)
#define FETCH_WIDTH (1 << LOG2_FETCH_WIDTH)
#define FTQ
#else // SUPERSCALAR
#define FETCH_WIDTH (1)
#endif // SUPERSCALAR

// only enable when the counts are required.
#define EN_BB_FB_COUNT

//#define PC_TRACE
#define GSHARE
#define DEBUG_GSHARE_UTILIZATION

enum class insn_t { non_cti, jump, branch, ret };
