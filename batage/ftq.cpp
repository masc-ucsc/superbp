#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>

#include <vector>

#include "../batage/predictor.hpp"
// #include "emulator.hpp"
#include "ftq.hpp"

#ifdef SUPERBP
ftq_entry_ptr ftq[NUM_FTQ_ENTRIES];
#elif defined BATAGE
// ftq_entry ftq[NUM_FTQ_ENTRIES];
#endif

ftq_entry::ftq_entry(const bool &predDir1, const bool &highconf1, const bool &resolveDir1, const uint64_t &pc1, const insn_t &insn1,
                     const uint64_t &branchTarget1, const uint64_t &fetch_pc, const uint8_t &inst_offset_from_fpc, const batage *bp)
    /*: predDir {predDir1}, resolveDir {resolveDir1}, pc {pc1}, branchTarget
       {branchTarget1}, hit {predictor.pred.hit}, s {predictor.pred.s}, meta
       {predictor.pred.meta}, bp {predictor.pred.bp}, cat
       {predictor.pred.cat},  bi {predictor.pred.bi}, bi2
       {predictor.pred.bi2}, b {std::vector<int>(std::begin(predictor.pred.b),
       std::end(predictor.pred.b))}, b2
       {std::vector<int>(std::begin(predictor.pred.b2),
       std::end(predictor.pred.b2))}*/
    : predDir{predDir1}
    , highconf{highconf1}
    , resolveDir{resolveDir1}
    , pc{pc1}
    , insn{insn1}
    , branchTarget{branchTarget1}
    , fetch_pc{fetch_pc}
    , inst_offset_from_fpc{inst_offset_from_fpc}
    , hit{bp->hit}
    , tags{bp->tags}
    , s{bp->s}
    , poses{bp->poses}
    , meta{bp->meta}
    , bp{bp->bp}
    , bi{bp->bi}
    , bi2{bp->bi2}
    , b_bi{bp->b_bi}
    , b2_bi2{bp->b2_bi2}
    , gi{ptr2vec(bp->gi, bp->SBP_NUMG)} {}
    //, gi{bp->gi} {}

ftq::ftq(batage *p) {
  bp                  = p;
  next_allocate_index = 0;  // To be written/ allocated next
  next_free_index     = 0;  // To be read/ freed next
  filled_ftq_entries  = 0;
  // NUM_FTQ_ENTRIES = (p->FETCHWIDTH * 2);
  // NUM_FTQ_ENTRIES = 64;
  // mem.reserve(NUM_FTQ_ENTRIES);
  mem.resize(NUM_FTQ_ENTRIES);
}

bool ftq::is_ftq_full(void) { return (filled_ftq_entries == NUM_FTQ_ENTRIES); }

bool ftq::is_ftq_empty(void) { return (filled_ftq_entries == 0); }

uint16_t ftq::get_num_free_ftq_entries(void) { return (NUM_FTQ_ENTRIES - filled_ftq_entries); }

#ifdef SUPERBP
void allocate_ftq_entry(AddrType branch_PC, AddrType branch_target, IMLI &IMLI_inst)
#elif defined BATAGE
void ftq::allocate_ftq_entry(const bool &predDir, const bool &highconf, const bool &resolveDir, const uint64_t &pc,
                             const insn_t &insn, const uint64_t &branchTarget, const uint64_t &fetch_pc,
                             const uint8_t &inst_offset_from_fpc)  // , histories* hist_ptr)
#else
void allocate_ftq_entry(void)
#endif
{
  // next_entry_index is updated in the end of the function, so still called
  // "next" when it is already being updated in this function

#ifdef SUPERBP
  ftq_entry_ptr ptr  = (ftq_entry_ptr)malloc(sizeof(ftq_entry));
  ptr->branch_PC     = branch_PC;
  ptr->branch_target = branch_target;  // Actually, after Execute

  ptr->pred_taken       = IMLI_inst.pred_taken;
  ptr->tage_pred        = IMLI_inst.tage_pred;
  ptr->LongestMatchPred = IMLI_inst.LongestMatchPred;
  ptr->alttaken         = IMLI_inst.alttaken;
  ptr->HitBank          = IMLI_inst.HitBank;
  ptr->AltBank          = IMLI_inst.AltBank;
  ptr->phist            = IMLI_inst.phist;
  ptr->GHIST            = IMLI_inst.GHIST;

  int nhist = IMLI_inst.nhist;
  ptr->ch_i = new Folded_history[nhist + 1];
  memcpy(ptr->ch_i, IMLI_inst.ch_i, ((nhist + 1) * sizeof(Folded_history)));
  ptr->ch_t_0 = new Folded_history[nhist + 1];
  memcpy(ptr->ch_t_0, IMLI_inst.ch_t[0], ((nhist + 1) * sizeof(Folded_history)));
  ptr->ch_t_1 = new Folded_history[nhist + 1];
  memcpy(ptr->ch_t_1, IMLI_inst.ch_t[1], ((nhist + 1) * sizeof(Folded_history)));

#elif defined BATAGE
  // ftq_entry f {predDir, resolveDir, pc, branchTarget, predictor.pred.hit,
  // predictor.pred.s, predictor.pred.meta};
  ftq_entry f{predDir, highconf, resolveDir, pc, insn, branchTarget, fetch_pc, inst_offset_from_fpc, bp};
  // Move assignment
  mem[next_allocate_index] = std::move(f);
#endif

#ifdef DEBUG_FTQ
#ifdef BATAGE
  std::cerr << "Entry # " << next_allocate_index << " allocated to PC = " << std::hex << pc << std::dec << "\n";
#endif  // BATAGE
#endif  // DEBUG_FTQ

  next_allocate_index = (next_allocate_index + 1) % NUM_FTQ_ENTRIES;
  filled_ftq_entries++;
#ifdef DEBUG_FTQ
  std::cerr << "After allocation - filled_ftq_entries = " << filled_ftq_entries << ", next_allocate_index = " << next_allocate_index
            << "\n";
#endif  // DEBUG_FTQ
  return;
}

#ifdef BATAGE
void ftq::set_ftq_index(uint16_t index) {
  next_allocate_index = index;
  next_free_index     = index;
}

bool ftq::get_predDir_from_ftq(uint8_t index) { return mem[index].predDir; }
bool ftq::get_conf_from_ftq(uint8_t index) { return mem[index].highconf; }

void ftq::ftq_update_resolvedinfo(uint8_t index, uint64_t branch_pc, insn_t insn, bool resolveDir, uint64_t branchTarget,
                                  const uint8_t inst_offset_from_fpc) {
  // ftq[index].pc = branch_pc;
  mem[index].insn                 = insn;
  mem[index].resolveDir           = resolveDir;
  mem[index].branchTarget         = branchTarget;
  mem[index].inst_offset_from_fpc = inst_offset_from_fpc;
  return;
}
#endif  // BATAGE

#ifdef SUPERBP
void get_ftq_data(IMLI &IMLI_inst)
#elif defined BATAGE
void ftq::get_ftq_data(ftq_entry *ftq_data_ptr)
#else
void get_ftq_data()
#endif
{

#ifdef SUPERBP
  ftq_entry_ptr ptr = ftq[next_free_index];
  //	PC							=
  // ptr->branch_PC; 	branchTarget    			= ptr->branch_target; //
  // Actually, after Execute
  IMLI_inst.pred_taken       = ptr->pred_taken;
  IMLI_inst.tage_pred        = ptr->tage_pred;
  IMLI_inst.LongestMatchPred = ptr->LongestMatchPred;
  IMLI_inst.alttaken         = ptr->alttaken;
  IMLI_inst.HitBank          = ptr->HitBank;
  IMLI_inst.AltBank          = ptr->AltBank;
  IMLI_inst.phist            = ptr->phist;
  IMLI_inst.GHIST            = ptr->GHIST;

  int nhist = IMLI_inst.nhist;
  memcpy(IMLI_inst.ch_i, ptr->ch_i, ((nhist + 1) * sizeof(Folded_history)));
  delete (ptr->ch_i);
  memcpy(IMLI_inst.ch_t[0], ptr->ch_t_0, ((nhist + 1) * sizeof(Folded_history)));
  delete (ptr->ch_t_0);
  memcpy(IMLI_inst.ch_t[1], ptr->ch_t_1, ((nhist + 1) * sizeof(Folded_history)));
  delete (ptr->ch_t_1);
  free(ptr);
  ftq[next_free_index] = NULL;
#elif defined BATAGE

  *ftq_data_ptr = std::move(mem[next_free_index]);
#endif

#ifdef DEBUG_FTQ
#ifdef BATAGE
  std::cerr << "Entry # " << next_free_index << " deallocated with PC = " << std::hex << ftq_data_ptr->pc << std::dec << "\n";
#endif  // BATAGE
#endif  // DEBUG_FTQ
  next_free_index = (next_free_index + 1) % NUM_FTQ_ENTRIES;
  filled_ftq_entries--;
#ifdef DEBUG_FTQ
  std::cerr << "After deallocation - filled_ftq_entries = " << filled_ftq_entries << ", next_free_index = " << next_free_index
            << "\n";
#endif  // DEBUG_FTQ
  return;
}  // get_ftq_data() over

void ftq::deallocate_ftq_entry(void) {
#ifdef SUPERBP
  ftq_entry_ptr ptr = ftq[next_free_index];

  free(ptr);
  ftq[next_free_index] = NULL;
#elif defined BATAGE
  mem[next_free_index].~ftq_entry();
#endif  // BATAGE
  next_free_index = (next_free_index + 1) % NUM_FTQ_ENTRIES;

  filled_ftq_entries--;
  return;
}  // deallocate_ftq_entry() over

void ftq::nuke_ftq() {
#ifdef SUPERBP
  ftq_entry_ptr ptr;

  while (filled_ftq_entries != 0) {
    ptr = mem[next_free_index];

    delete (ptr->ch_i);
    delete (ptr->ch_t_0);
    delete (ptr->ch_t_1);

    free(ptr);
    mem[next_free_index] = NULL;

    next_free_index = (next_free_index + 1) % NUM_FTQ_ENTRIES;
    filled_ftq_entries--;
  }
#elif defined BATAGE
#ifdef DEBUG_FTQ
  fprintf(stderr, "nuke called with filled_ftq_entries = %u\n", filled_ftq_entries);
#endif  // DEBUG_FTQ
        // ftq_entry* temp;
  while (filled_ftq_entries != 0) {
    //*temp = std::move(ftq[next_free_index]);
    next_free_index = (next_free_index + 1) % NUM_FTQ_ENTRIES;
    filled_ftq_entries--;
  }
#endif

  next_allocate_index = 0;
  next_free_index     = 0;
}
