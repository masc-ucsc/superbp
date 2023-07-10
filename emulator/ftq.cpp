#include <stdlib.h>
#include <stdio.h>
#include <vector>
#include <inttypes.h>

#include "ftq.hpp"
#include "../batage/predictor.hpp"
#include "emulator.hpp"

#ifdef SUPERBP
ftq_entry_ptr ftq[NUM_FTQ_ENTRIES];
#elif defined BATAGE
ftq_entry ftq[NUM_FTQ_ENTRIES];
#endif

uint16_t next_allocate_index; // To be written/ allocated next
uint16_t next_free_index; // To be read/ freed next
int16_t filled_ftq_entries;

bool is_ftq_full(void)
{
	return (filled_ftq_entries == NUM_FTQ_ENTRIES);
}

bool is_ftq_empty(void)
{
	return (filled_ftq_entries == 0);
}

#ifdef SUPERBP
void allocate_ftq_entry(AddrType branch_PC, AddrType branch_target, IMLI& IMLI_inst)
#elif defined BATAGE
void allocate_ftq_entry (const bool& predDir, const bool& resolveDir, const uint64_t& pc, const uint64_t& branchTarget, const PREDICTOR& predictor) // , histories* hist_ptr)
#else
void allocate_ftq_entry (void)
#endif
{
	// next_entry_index is updated in the end of the function, so still called "next" when it is already being updated in this function 
	
#ifdef SUPERBP	
	ftq_entry_ptr ptr 		= (ftq_entry_ptr) malloc (sizeof(ftq_entry) );
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
	//ftq_entry f {predDir, resolveDir, pc, branchTarget, predictor.pred.hit, predictor.pred.s, predictor.pred.meta};
	ftq_entry f {predDir, resolveDir, pc, branchTarget, predictor};
	// Move assignment
	ftq[next_allocate_index] = std::move(f);
#endif

#ifdef DEBUG_FTQ
#ifdef BATAGE
	std::cout << "Entry # " << next_allocate_index << " allocated to PC = " << std::hex << pc << std::dec << "\n";
#endif // BATAGE
#endif // DEBUG_FTQ

	next_allocate_index = (next_allocate_index+1) % NUM_FTQ_ENTRIES;
	filled_ftq_entries++;
#ifdef DEBUG_FTQ	
	std::cout << "After allocation - filled_ftq_entries = " << filled_ftq_entries << ", next_allocate_index = " << next_allocate_index << "\n";
#endif // DEBUG_FTQ	
	return;
}

#ifdef SUPERBP
void get_ftq_data(IMLI& IMLI_inst)
#elif defined BATAGE
void get_ftq_data (ftq_entry* ftq_data_ptr)
#else 
void get_ftq_data()
#endif
{
		
#ifdef SUPERBP
	ftq_entry_ptr ptr = ftq[next_free_index];
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
	free(ptr);
	ftq[next_free_index] = NULL;
#elif defined BATAGE

	*ftq_data_ptr = std::move(ftq[next_free_index]);
#endif
	
#ifdef DEBUG_FTQ
#ifdef BATAGE
	std::cout << "Entry # " << next_free_index << " deallocated with PC = " << std::hex << ftq_data_ptr->pc << std::dec << "\n";
#endif // BATAGE
#endif // DEBUG_FTQ
	next_free_index = (next_free_index+1) % NUM_FTQ_ENTRIES;
	filled_ftq_entries--;
#ifdef DEBUG_FTQ	
	std::cout << "After deallocation - filled_ftq_entries = " << filled_ftq_entries << ", next_free_index = " << next_free_index << "\n";
#endif // DEBUG_FTQ
	return;
} // get_ftq_data() over

void deallocate_ftq_entry(void)
{
#ifdef SUPERBP
	ftq_entry_ptr ptr = ftq[next_free_index];

	free(ptr);
	ftq[next_free_index] = NULL;
#elif defined BATAGE
	ftq[next_free_index].~ftq_entry();
#endif // BATAGE
	next_free_index = (next_free_index+1) % NUM_FTQ_ENTRIES;
	
	filled_ftq_entries--;
	return;
} // deallocate_ftq_entry() over

void nuke()
{
#ifdef SUPERBP	
	ftq_entry_ptr ptr;
	
	while ( filled_ftq_entries != 0 )
	{
		ptr = ftq[next_free_index];

		delete(ptr->ch_i);
		delete(ptr->ch_t_0);
		delete(ptr->ch_t_1);

		free(ptr);
		ftq[next_free_index] = NULL;

		next_free_index = (next_free_index+1) % NUM_FTQ_ENTRIES;
		filled_ftq_entries--;
	}
#elif defined BATAGE
	while ( filled_ftq_entries != 0 )
	{
		ftq[next_free_index].~ftq_entry();
		next_free_index = (next_free_index+1) % NUM_FTQ_ENTRIES;
		filled_ftq_entries--;
	}
#endif

	next_allocate_index = 0;
	next_free_index = 0;
}
