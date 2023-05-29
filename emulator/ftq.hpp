#include <stdlib.h>
#include <stdio.h>
#include <vector>
#include <inttypes.h>

#define NUM_FTQ_ENTRIES 32

//#define SUPERBP
#include "../batage/predictor.hpp"
#define BATAGE
#ifdef BATAGE
#include "../batage/batage.hpp"
#endif

typedef struct {
#ifdef SUPERBP
	AddrType branch_PC;
	AddrType branch_target; // Check
	bool pred_taken; 
	bool tage_pred; 
	bool LongestMatchPred; 
	bool alttaken; 
	int HitBank;
	int AltBank;
	long long IMLIcount;
	long long phist;
	long long GHIST;
	
	Folded_history *ch_i;
	Folded_history *ch_t_0;
	Folded_history *ch_t_1;
#elif defined BATAGE
	bool predDir;
	bool resolveDir;	
	uint64_t pc;
	uint64_t branchTarget;
	
	vector<int> hit;
	//vector<dualcounter> s;
	//histories * hist_ptr;
	
#endif 
}ftq_entry;

typedef ftq_entry *ftq_entry_ptr;

bool is_ftq_full (void);
#ifdef SUPERBP
void allocate_ftq_entry (AddrType branch_PC, AddrType branch_target, IMLI& IMLI_inst);
void get_ftq_data (IMLI& IMLI_inst);
#elif defined BATAGE
void allocate_ftq_entry (bool predDir, bool resolveDir, uint64_t pc, uint64_t branchTarget, PREDICTOR* predictor_ptr); // , histories* hist_ptr);
void get_ftq_data (ftq_entry_ptr ftq_data_ptr, PREDICTOR* predictor_ptr);
#endif
void nuke();

