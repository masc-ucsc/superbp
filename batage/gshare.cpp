#include "gshare.hpp"

gshare_entry::gshare_entry() {
  ctr = 0;
  tag = 0;
#ifdef DEBUG_UTILIZATION
  allocated = false;
#endif
  // TODO - Update from 2
  hist_patch.resize(2);  // NUM_TAKEN_BRANCHES
  PCs.resize(2);         // NUM_TAKEN_BRANCHES
  poses.resize(2);       // NUM_TAKEN_BRANCHES
}

gshare::gshare() {
  table.resize(NUM_GSHARE_ENTRIES);
  decay_ctr = 0;
// start_log();
#ifdef DEBUG_UTILIZATION
  num_unallocated         = NUM_GSHARE_ENTRIES;
  num_collisions_blocked  = 0;
  num_collisions_replaced = 0;
#endif
}

uint16_t gshare::calc_tag(uint64_t PC) {
  // TODO Update
  return ((uint16_t)(PC ^ (PC << 2) ^ (PC >> 16) ^ (PC >> 32)));
}

uint16_t gshare::calc_index(uint64_t PC) {
  // TODO Update
  return (PC % NUM_GSHARE_ENTRIES);
}

void gshare::decay() {
  int ctr;
  for (int i = 0; i < NUM_GSHARE_ENTRIES; i++) {
    ctr = table[i].ctr;
    if (ctr > 0) {
      table[i].ctr = ctr - 1;
    }
  }
}

bool gshare::is_hit(uint64_t PC, uint16_t index, uint16_t tag) {
  // uint16_t index = calc_index(PC);
  // uint16_t tag = calc_tag(PC);
  uint8_t ctr = table[index].ctr;

  if ((tag == table[index].tag) && (tag != 0)) {
    prediction.tag_match = true;

#ifdef DEBUG_PREDICT
    if (ctr < CTR_THRESHOLD) {
      fprintf(gshare_log,
              "gshare tag matched miss for fetchPC = %#llx at index = %u, with tag = %#x, since ctr = %u < CTR_THRESHOLD = %u\n",
              PC,
              index,
              tag,
              ctr,
              CTR_THRESHOLD);
    }
#endif
  }

  return (prediction.tag_match && (ctr >= CTR_THRESHOLD));
}

gshare_prediction& gshare::predict(uint64_t PC, uint16_t index, uint16_t tag) {
  index %= NUM_GSHARE_ENTRIES;
  prediction.index     = index;
  prediction.hit       = false;
  prediction.tag_match = false;
#ifdef DEBUG_PC
  if ((PC == 0xffffffff8010a4f2) || (PC == 0xffffffff8010a130)) {
    for (int i = 0; i < NUM_GSHARE_ENTRIES; i++) {
      if (PC == table[i].PC) {
        printf("PC present at index = %d, ctr = %d\n", i, table[i].ctr);
        break;
      }
    }
  }
#endif
  // if ()
  {
    // uint16_t index = calc_index(PC);
    prediction.hit   = is_hit(PC, index, tag);
    prediction.index = index;
    prediction.info  = table[index];
#ifdef DEBUG_PREDICT
    if (prediction.hit) {
      fprintf(
          gshare_log,
          "gshare hit index = %u for fetchPC = %#llx, sent tag = %#x, pos[0] = %u, PC[0] = %#llx, pos[1] = %u, PC[1] = %#llx \n",
          index,
          PC,
          tag,
          prediction.info.poses[0],
          prediction.info.PCs[0],
          prediction.info.poses[1],
          prediction.info.PCs[1]);
    }
#endif
  }

  return prediction;
}

void gshare::allocate(vector<uint64_t>& PCs, vector<uint8_t>& poses, uint16_t update_gshare_index, uint16_t update_gshare_tag) {
  uint16_t index   = update_gshare_index % NUM_GSHARE_ENTRIES;  // calc_index (PCs[0]);
  uint16_t tag     = update_gshare_tag;                         // calc_tag(PCs[0]);
  uint16_t old_tag = table[index].tag;

  // TODO Check - If same tag - return, do we increase ctr ?
  if (old_tag == tag) {
    return;
  }

  uint8_t ctr = table[index].ctr;

  // TODO Check
  if (ctr > GSHARE_T_HIGHCONF) {
#ifdef DEBUG_ALLOC
    fprintf(gshare_log,
            "Alloc called for tag = %#x, old_tag = %#x, but ctr = %u is more than GSHARE_T_HIGHCONF = %u \n",
            tag,
            old_tag,
            ctr,
            GSHARE_T_HIGHCONF);
#endif  // DEBUG_ALLOC

    table[index].ctr = ctr - 1;

#ifdef DEBUG_UTILIZATION
    num_collisions_blocked++;
#endif

    return;
  } else {
    table[index].ctr = CTR_ALLOC_VAL;
    // TODO - Update
    for (int i = 0; i < 2; i++)  // NUM_TAKEN_BRANCHES
    {
      table[index].tag      = tag;
      table[index].PCs[i]   = PCs[i + 1];
      table[index].poses[i] = poses[i];
    }

    decay_ctr++;
    if (decay_ctr == (NUM_GSHARE_ENTRIES >> 1)) {
      decay();
      decay_ctr = 0;
    }

#ifdef DEBUG_ALLOC
    fprintf(gshare_log,
            "Alloc done at index = %u for sent index update_gshare_index = %u, intended for fetch_pc = %#llx, saved branch to "
            "%#llx at pos = %u and  branch to %#llx at pos = %u, saving tag = %#x over old_tag = %#x\n",
            index,
            update_gshare_index,
            PCs[0],
            PCs[1],
            poses[0],
            PCs[2],
            poses[1],
            tag,
            old_tag);
#endif  // DEBUG_ALLOC

#ifdef DEBUG_UTILIZATION
    if (!table[index].allocated) {
      num_unallocated--;
      table[index].allocated = true;
    } else {
      num_collisions_replaced++;
    }
#endif
  }
}

void gshare::update(gshare_prediction& prediction, bool prediction_correct) {
  uint16_t index     = prediction.index;
  bool     tag_match = prediction.tag_match;
  uint8_t  ctr       = table[index].ctr;
  if (tag_match) {
    if (prediction_correct) {
      table[index].ctr = (ctr < GSHARE_CTRMAX) ? (ctr + 1) : GSHARE_CTRMAX;
#ifdef DEBUG_UPDATE
      fprintf(gshare_log,
              "Update on correct prediction for PC = %#llx, updated index = %d's counter to %u\n",
              prediction.info.PCs[0],
              index,
              table[index].ctr);
#endif
    } else {
      table[index].ctr = (ctr > 0) ? (ctr - 1) : 0;
#ifdef DEBUG_UPDATE
      fprintf(gshare_log,
              "Update on misprediction for PC = %#llx, updated index = %d's counter to %u\n",
              prediction.info.PCs[0],
              index,
              table[index].ctr);
#endif
    }
  }
}

#ifdef DEBUG_UTILIZATION
void gshare::get_gshare_utilization(uint64_t* buffer) {
  buffer[0] = num_unallocated;
  buffer[1] = num_collisions_blocked;
  buffer[2] = num_collisions_replaced;
  return;
}
#endif
