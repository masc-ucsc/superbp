#pragma once

// only enable when the counts are required.
#define EN_BB_BR_COUNT

#define SUPERSCALAR
#ifdef SUPERSCALAR
#define FETCH_WIDTH 4
#define FTQ
#endif // SUPERSCALAR

// Change this to change predictor - may later want to make this a command line parameter - argument to main
#define BATAGE

