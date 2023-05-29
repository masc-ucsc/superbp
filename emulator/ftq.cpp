#include <stdlib.h>
#include <stdio.h>
#include <vector>
#include <inttypes.h>

#include "ftq.hpp"
#include "../batage/predictor.hpp"

ftq_entry_ptr ftq[NUM_FTQ_ENTRIES];

uint8_t next_allocate_index; // To be written/ allocated next
uint8_t next_free_index; // To be read/ freed next
uint8_t filled_ftq_entries;

bool is_ftq_full(void)
{
	return (filled_ftq_entries == NUM_FTQ_ENTRIES);
}

#ifdef SUPERBP
void allocate_ftq_entry(AddrType branch_PC, AddrType branch_target, IMLI& IMLI_inst)
#elif defined BATAGE
void allocate_ftq_entry (bool predDir, bool resolveDir, uint64_t pc, uint64_t branchTarget, PREDICTOR* predictor_ptr) // , histories* hist_ptr)
#else
void allocate_ftq_entry (void)
#endif
{
	// next_entry_index is updated in the end of the function, so still called "next" when it is already being updated in this function 

	ftq_entry_ptr ptr 		= (ftq_entry_ptr) malloc (sizeof(ftq_entry) );
	
#ifdef SUPERBP	
	ptr->branch_PC 			= branch_PC;
	ptr->branch_target 		= branch_target; // Actually, after Execute
	
	ptr->pred_taken 		= IMLI_inst.pred_taken;
	ptr->tage_pred 			= IMLI_inst.tage_pred;
	ptr->LongestMatchPred 	= IMLI_inst.LongestMatchPred;
	ptr->alttaken 			= IMLI_inst.alttaken;
	ptr->HitBank 			= IMLI_inst.HitBank;
	ptr->AltBank 			= IMLI_inst.AltBank;
	ptr->phist 				= IMLI_inst.phist;
	ptr->GHIST 				= IMLI_inst.GHIST;
	
	int nhist 				= IMLI_inst.nhist;
	ptr->ch_i   			= new Folded_history[nhist + 1];
	memcpy(ptr->ch_i, IMLI_inst.ch_i, ((nhist + 1)*sizeof(Folded_history)));
    ptr->ch_t_0 			= new Folded_history[nhist + 1];
    memcpy(ptr->ch_t_0, IMLI_inst.ch_t[0], ((nhist + 1)*sizeof(Folded_history)));
    ptr->ch_t_1 			= new Folded_history[nhist + 1];
    memcpy(ptr->ch_t_1, IMLI_inst.ch_t[1], ((nhist + 1)*sizeof(Folded_history)));
#elif defined BATAGE
	ptr->predDir = predDir;
	ptr->resolveDir = resolveDir;
	ptr->pc = pc;
	ptr->branchTarget = branchTarget;
	
	//ptr->hit = (predictor_ptr->pred).hit;
	// a.insert(std::end(a), std::begin(b), std::end(b));
	(ptr->hit).clear();
	//(ptr->hit).insert(std::end(ptr->hit), std::begin((predictor_ptr->pred).hit), std::end((predictor_ptr->pred).hit));
#endif

	ftq[next_allocate_index] = ptr;
	next_allocate_index = (next_allocate_index+1) % NUM_FTQ_ENTRIES;
	filled_ftq_entries++;
	return;
}

#ifdef SUPERBP
void get_ftq_data(IMLI& IMLI_inst)
#elif defined BATAGE
void get_ftq_data (ftq_entry_ptr ftq_data_ptr, PREDICTOR* predictor_ptr)
#else 
void get_ftq_data()
#endif
{
	ftq_entry_ptr ptr = ftq[next_free_index];
	
#ifdef SUPERBP
//	PC							= ptr->branch_PC;
//	branchTarget    			= ptr->branch_target; // Actually, after Execute
	IMLI_inst.pred_taken 		= ptr->pred_taken;
	IMLI_inst.tage_pred 		= ptr->tage_pred;
	IMLI_inst.LongestMatchPred 	= ptr->LongestMatchPred;
	IMLI_inst.alttaken 			= ptr->alttaken;
	IMLI_inst.HitBank 			= ptr->HitBank;
	IMLI_inst.AltBank 			= ptr->AltBank;
	IMLI_inst.phist				= ptr->phist;
	IMLI_inst.GHIST				= ptr->GHIST;
	
	int nhist = IMLI_inst.nhist;
	memcpy(IMLI_inst.ch_i, ptr->ch_i, ((nhist + 1)*sizeof(Folded_history)));
	delete(ptr->ch_i);
	memcpy(IMLI_inst.ch_t[0], ptr->ch_t_0, ((nhist + 1)*sizeof(Folded_history)));
	delete(ptr->ch_t_0);
	memcpy(IMLI_inst.ch_t[1], ptr->ch_t_1, ((nhist + 1)*sizeof(Folded_history)));
	delete(ptr->ch_t_1);
#elif defined BATAGE
	ftq_data_ptr->pc 			= ptr->pc;
	ftq_data_ptr->predDir 		= ptr->predDir;
	ftq_data_ptr->resolveDir 	= ptr->resolveDir;
	ftq_data_ptr->branchTarget 	= ptr->branchTarget;
	
	(predictor_ptr->pred).hit 	= ptr->hit;
#endif
	
	free(ptr);
	ftq[next_free_index] = NULL;
	next_free_index = (next_free_index+1) % NUM_FTQ_ENTRIES;
	filled_ftq_entries--;
	return;
} // get_ftq_data() over

void deallocate_ftq_entry(void)
{
	ftq_entry_ptr ptr = ftq[next_free_index];

	free(ptr);
	ftq[next_free_index] = NULL;
	next_free_index = (next_free_index+1) % NUM_FTQ_ENTRIES;
	
	filled_ftq_entries--;
	return;
} // deallocate_ftq_entry() over

void nuke()
{
	ftq_entry_ptr ptr;
	
	while ( filled_ftq_entries != 0 )
	{
		ptr = ftq[next_free_index];

#ifdef SUPERBP			
		delete(ptr->ch_i);
		delete(ptr->ch_t_0);
		delete(ptr->ch_t_1);
#elif defined BATAGE

#endif
		free(ptr);
		ftq[next_free_index] = NULL;
		next_free_index = (next_free_index+1) % NUM_FTQ_ENTRIES;
		filled_ftq_entries--;
	}
	next_allocate_index = 0;
	next_free_index = 0;
}
