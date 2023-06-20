#pragma once

#define BRANCHPROF

// Change this to change predictor - may later want to make this a command line parameter - argument to main
#define BATAGE

#define SUPERSCALAR
#ifdef SUPERSCALAR
#define FETCH_WIDTH 4
#define FTQ
#endif // SUPERSCALAR

// only enable when the counts are required.
#define EN_BB_BR_COUNT

enum class insn_t
{
	non_cti,
	jump,
	branch
};

