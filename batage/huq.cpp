#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <vector>

#include "../batage/predictor.hpp"
//#include "emulator.hpp"
#include "huq.hpp"

#ifdef SUPERBP
huq_entry_ptr huq[NUM_HUQ_ENTRIES];
#elif defined BATAGE
//huq_entry huq[NUM_HUQ_ENTRIES];
#endif

  huq_entry::huq_entry(const uint64_t &branchTarget1, const bool &resolveDir1)
      : branchTarget{branchTarget1}, resolveDir{resolveDir1} {}
                                                                
 huq::huq(batage * p)
	{
		//NUM_HUQ_ENTRIES = ((p->FETCHWIDTH) * 2);
		//NUM_HUQ_ENTRIES = 64;
		mem.resize(NUM_HUQ_ENTRIES);
		next_allocate_index = 0;
		next_free_index = 0;
		filled_huq_entries = 0;
	}

bool huq::is_huq_full(void) { return (filled_huq_entries == NUM_HUQ_ENTRIES); }

bool huq::is_huq_empty(void) { return (filled_huq_entries == 0); }

#ifdef SUPERBP
void allocate_huq_entry(AddrType branch_PC, AddrType branch_target,
                        IMLI &IMLI_inst)
#elif defined BATAGE
void huq::allocate_huq_entry(/*const uint64_t& pc,*/ const uint64_t &branchTarget,
                        const bool &resolveDir)
#else
void allocate_huq_entry(void)
#endif
{
  // next_entry_index is updated in the end of the function, so still called
  // "next" when it is already being updated in this function

#ifdef SUPERBP
  huq_entry_ptr ptr = (huq_entry_ptr)malloc(sizeof(huq_entry));
  ptr->branch_PC = branch_PC;
  ptr->branch_target = branch_target; // Actually, after Execute

  ptr->pred_taken = IMLI_inst.pred_taken;
  ptr->tage_pred = IMLI_inst.tage_pred;
  ptr->LongestMatchPred = IMLI_inst.LongestMatchPred;
  ptr->alttaken = IMLI_inst.alttaken;
  ptr->HitBank = IMLI_inst.HitBank;
  ptr->AltBank = IMLI_inst.AltBank;
  ptr->phist = IMLI_inst.phist;
  ptr->GHIST = IMLI_inst.GHIST;

  int nhist = IMLI_inst.nhist;
  ptr->ch_i = new Folded_history[nhist + 1];
  memcpy(ptr->ch_i, IMLI_inst.ch_i, ((nhist + 1) * sizeof(Folded_history)));
  ptr->ch_t_0 = new Folded_history[nhist + 1];
  memcpy(ptr->ch_t_0, IMLI_inst.ch_t[0],
         ((nhist + 1) * sizeof(Folded_history)));
  ptr->ch_t_1 = new Folded_history[nhist + 1];
  memcpy(ptr->ch_t_1, IMLI_inst.ch_t[1],
         ((nhist + 1) * sizeof(Folded_history)));

#elif defined BATAGE
  // huq_entry f {predDir, resolveDir, pc, branchTarget, predictor.pred.hit,
  // predictor.pred.s, predictor.pred.meta};
  huq_entry h{/*pc,*/ branchTarget, resolveDir};
  // Move assignment
  mem[next_allocate_index] = std::move(h);
#endif

#ifdef DEBUG_HUQ
#ifdef BATAGE
  std::cout << "HUQ Entry # " << next_allocate_index
            << " allocated to target = " << std::hex << branchTarget << std::dec << "\n";
#endif // BATAGE
#endif // DEBUG_HUQ

  next_allocate_index = (next_allocate_index + 1) % NUM_HUQ_ENTRIES;
  filled_huq_entries++;
#ifdef DEBUG_HUQ
  std::cout << "After allocation - filled_huq_entries = " << filled_huq_entries
            << ", next_allocate_index = " << next_allocate_index << "\n";
#endif // DEBUG_HUQ
  return;
}

#ifdef BATAGE
void huq::huq_update_resolvedinfo (uint16_t index, bool resolveDir)
{
	mem[index].resolveDir = resolveDir;
}
#endif // BATAGE

#ifdef SUPERBP
void get_huq_data(IMLI &IMLI_inst)
#elif defined BATAGE
void huq::get_huq_data(huq_entry *huq_data_ptr)
#else
void get_huq_data()
#endif
{

#ifdef SUPERBP
  huq_entry_ptr ptr = huq[next_free_index];
  //	PC							=
  //ptr->branch_PC; 	branchTarget    			= ptr->branch_target; //
  //Actually, after Execute
  IMLI_inst.pred_taken = ptr->pred_taken;
  IMLI_inst.tage_pred = ptr->tage_pred;
  IMLI_inst.LongestMatchPred = ptr->LongestMatchPred;
  IMLI_inst.alttaken = ptr->alttaken;
  IMLI_inst.HitBank = ptr->HitBank;
  IMLI_inst.AltBank = ptr->AltBank;
  IMLI_inst.phist = ptr->phist;
  IMLI_inst.GHIST = ptr->GHIST;

  int nhist = IMLI_inst.nhist;
  memcpy(IMLI_inst.ch_i, ptr->ch_i, ((nhist + 1) * sizeof(Folded_history)));
  delete (ptr->ch_i);
  memcpy(IMLI_inst.ch_t[0], ptr->ch_t_0,
         ((nhist + 1) * sizeof(Folded_history)));
  delete (ptr->ch_t_0);
  memcpy(IMLI_inst.ch_t[1], ptr->ch_t_1,
         ((nhist + 1) * sizeof(Folded_history)));
  delete (ptr->ch_t_1);
  free(ptr);
  huq[next_free_index] = NULL;
#elif defined BATAGE

  *huq_data_ptr = std::move(mem[next_free_index]);
#endif

#ifdef DEBUG_HUQ
#ifdef BATAGE
  std::cout << "HUQ Entry # " << next_free_index
            << " deallocated with branchTarget = " << std::hex << huq_data_ptr->branchTarget
            << std::dec << "\n";
#endif // BATAGE
#endif // DEBUG_HUQ
  next_free_index = (next_free_index + 1) % NUM_HUQ_ENTRIES;
  filled_huq_entries--;
#ifdef DEBUG_HUQ
  std::cout << "After deallocation - filled_huq_entries = "
            << filled_huq_entries << ", next_free_index = " << next_free_index
            << "\n";
#endif // DEBUG_HUQ
  return;
} // get_huq_data() over

void huq::deallocate_huq_entry(void) {
#ifdef SUPERBP
  huq_entry_ptr ptr = huq[next_free_index];

  free(ptr);
  huq[next_free_index] = NULL;
#elif defined BATAGE
  mem[next_free_index].~huq_entry();
#endif // BATAGE
  next_free_index = (next_free_index + 1) % NUM_HUQ_ENTRIES;

  filled_huq_entries--;
  return;
} // deallocate_huq_entry() over

void huq::nuke_huq() {
#ifdef SUPERBP
  huq_entry_ptr ptr;

  while (filled_huq_entries != 0) {
    ptr = huq[next_free_index];

    delete (ptr->ch_i);
    delete (ptr->ch_t_0);
    delete (ptr->ch_t_1);

    free(ptr);
    huq[next_free_index] = NULL;

    next_free_index = (next_free_index + 1) % NUM_HUQ_ENTRIES;
    filled_huq_entries--;
  }
#elif defined BATAGE
  while (filled_huq_entries != 0) {
    mem[next_free_index].~huq_entry();
    next_free_index = (next_free_index + 1) % NUM_HUQ_ENTRIES;
    filled_huq_entries--;
  }
#endif

  next_allocate_index = 0;
  next_free_index = 0;
}
