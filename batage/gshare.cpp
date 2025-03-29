#include "gshare.hpp"
#include "time.h"

  	void gshare::read_env_variables()
  	{
  		char *temp; 
  		
  		temp     = getenv("NUM_GSHARE_ENTRIES_SHIFT");
  NUM_GSHARE_ENTRIES_SHIFT = atoi(temp);
  fprintf(stderr, "NUM_GSHARE_ENTRIES_SHIFT = %d\n", NUM_GSHARE_ENTRIES_SHIFT);
  
  temp     = getenv("NUM_PAGES_PER_GROUP");
  NUM_PAGES_PER_GROUP = atoi(temp);
  fprintf(stderr, "NUM_PAGES_PER_GROUP = %d\n", NUM_PAGES_PER_GROUP);
  
  temp     = getenv("PAGE_OFFSET_SIZE");
  PAGE_OFFSET_SIZE = atoi(temp);
  fprintf(stderr, "PAGE_OFFSET_SIZE = %d\n", PAGE_OFFSET_SIZE);
  
    temp     = getenv("PAGE_TABLE_INDEX_SIZE");
  PAGE_TABLE_INDEX_SIZE = atoi(temp);
  fprintf(stderr, "PAGE_TABLE_INDEX_SIZE = %d\n", PAGE_TABLE_INDEX_SIZE);
  	}
  	
	void gshare::populate_dependent_globals()
	{
		NUM_GSHARE_ENTRIES = (1 << NUM_GSHARE_ENTRIES_SHIFT);
		PAGE_SIZE = (1 << PAGE_OFFSET_SIZE);
		NUM_PAGE_TABLE_ENTRIES = (1 << PAGE_TABLE_INDEX_SIZE);
	}
	
gshare_entry::gshare_entry() {
  ctr = 0;
  tag = 0;
#ifdef DEBUG_UTILIZATION
  allocated = false;
#endif
  // TODO - Update from 2
  hist_patch.resize(2);  // NUM_TAKEN_BRANCHES
  poses.resize(2);       // NUM_TAKEN_BRANCHES
  
  //   PCs.resize(2);         // NUM_TAKEN_BRANCHES 			
  page_table_index.resize(2); 		
  page_offset.resize(2); 
}

uint64_t gshare_entry::size(uint8_t PAGE_TABLE_INDEX_SIZE, uint8_t PAGE_OFFSET_SIZE)
{
	// ctr + tag + 2 * (poses + valid + page_table_index + page_offset)
	return ( NUM_GSHARE_CTRBITS + NUM_GSHARE_TAGBITS + 1 + 2 * (4 + PAGE_TABLE_INDEX_SIZE + PAGE_OFFSET_SIZE));
}

gshare_entry_formed::gshare_entry_formed() {
  ctr = 0;
  tag = 0;
#ifdef DEBUG_UTILIZATION
  allocated = false;
#endif
  // TODO - Update from 2
  hist_patch.resize(2);  // NUM_TAKEN_BRANCHES
  poses.resize(2);       // NUM_TAKEN_BRANCHES
  PCs.resize(2);         // NUM_TAKEN_BRANCHES
}

gshare_entry_formed::gshare_entry_formed(const uint64_t PC, const gshare_entry& rhs, vector<uint64_t>&  pages, uint8_t PAGE_OFFSET_SIZE) {
    ctr        = rhs.ctr;
    tag        = rhs.tag;
    hist_patch = rhs.hist_patch;
    poses      = rhs.poses;

	uint64_t PC_page_num = (PC >> PAGE_OFFSET_SIZE);
	vector<uint64_t> targets(2);
	for (int i = 0; i < 2; i++)
    {
    	uint64_t page_num 	= rhs.page_table_index[i] ? pages[rhs.page_table_index[i]] : PC_page_num;
    	uint16_t page_offset 	= rhs.page_offset[i];
    	targets[i] 			= (page_num << PAGE_OFFSET_SIZE) | page_offset;
	}
	PCs         					= targets; 
  }
  
gshare_entry_formed& gshare_entry_formed::operator=(const gshare_entry_formed& rhs) {
    ctr        = rhs.ctr;
    tag        = rhs.tag;
    hist_patch = rhs.hist_patch;
    poses      = rhs.poses;

    PCs        = rhs.PCs; 
    return *this;
  }
 
 void gshare::gshare_resize()
 {
 	  pages.resize(NUM_PAGE_TABLE_ENTRIES);
  page_valid.resize(NUM_PAGE_TABLE_ENTRIES);
  //page_repl_ctr.resize(NUM_PAGE_TABLE_ENTRIES/NUM_PAGES_PER_GROUP);
  fifo.resize(NUM_PAGE_TABLE_ENTRIES/NUM_PAGES_PER_GROUP);
  table.resize(NUM_GSHARE_ENTRIES);
  
  size();
 }

gshare::gshare() {


  srand (time(NULL));		
  random = 0x05af5a0f ^ 0x5f0aa05f;
  
  
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


bool gshare::is_tag_match(uint64_t PC, uint16_t index, uint16_t tag) const {
  return ((tag == table[index].tag) && (tag != 0));
}

bool gshare::is_hit(bool tag_match, uint64_t PC, uint16_t index, uint16_t tag) const {
  // uint16_t index = calc_index(PC);
  // uint16_t tag = calc_tag(PC);
  uint8_t ctr = table[index].ctr;

  if ((tag == table[index].tag) && (tag != 0)) {
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

  return (tag_match && (ctr >= CTR_THRESHOLD));
}

gshare_prediction gshare::predict(uint64_t PC, uint16_t index, uint16_t tag) {
  index %= NUM_GSHARE_ENTRIES;
  gshare_prediction pred;
  pred.index     = index;
  pred.hit       = false;
  pred.tag_match = false;
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
    pred.tag_match = is_tag_match(PC, index, tag);
    pred.hit   = is_hit(pred.tag_match, PC, index, tag);
    pred.index = index;
    pred.info  = gshare_entry_formed(PC, table[index], pages, PAGE_OFFSET_SIZE);
#ifdef DEBUG_PREDICT
    if (pred.hit) {
      fprintf(
          gshare_log,
          "gshare hit index = %u for fetchPC = %#llx, sent tag = %#x, pos[0] = %u, PC[0] = %#llx, pos[1] = %u, PC[1] = %#llx \n",
          index,
          PC,
          tag,
          pred.info.poses[0],
          pred.info.PCs[0],
          pred.info.poses[1],
          pred.info.PCs[1]);
    }
#endif
  }

  return pred;
}

uint8_t gshare::get_group()
{
	// return LRU
	uint8_t num_groups = (NUM_PAGE_TABLE_ENTRIES/NUM_PAGES_PER_GROUP);
	return fifo[num_groups-1];
}

uint8_t gshare::get_entry_in_group(uint8_t group)
{
	// If any entry has page_valid = 0, then use it, but skip group 0, index 0, that is reserved for same page
	for (int i = group ? 1 : 0; i < NUM_PAGES_PER_GROUP; i++)
	{
		if (page_valid[(group * NUM_PAGES_PER_GROUP)+i]== 0)
		return i;
	}
	// If no entry has page_valid = 0, choose random
	//return (random ^ (random >> 5) ^ (random << 5)) % NUM_PAGES_PER_GROUP;
	// TODO Check to handle index 0 - that is not index in the group, it is index in 
	return group ?  (rand() % NUM_PAGES_PER_GROUP) : ((rand() % (NUM_PAGES_PER_GROUP-1))+1) ;
}

uint64_t gshare::get_a_page_index()
{
	// TODO
	uint8_t group = get_group();
	return get_entry_in_group(group);
}

void gshare::update_page_repl_ctr(uint8_t page_index)
{
	uint8_t num_groups = (NUM_PAGE_TABLE_ENTRIES/NUM_PAGES_PER_GROUP);
	uint8_t group = page_index % NUM_PAGES_PER_GROUP;
	uint8_t sel_index;
	for (int i = 0; i < num_groups; i++ )
	{
		if (i == group)
			{ sel_index = i; }
	}
	for (int i = sel_index; i >0; i-- )
	{
		fifo[i] = fifo[i-1];
	}
	fifo[0] = group;
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
    
    table[index].tag      = tag;
    uint64_t PCs0_page_num = (PCs[0] >> PAGE_OFFSET_SIZE);
    for (int i = 0; i < 2; i++)  // NUM_TAKEN_BRANCHES
    {
      table[index].poses[i] = poses[i];
      //       table[index].PCs[i]   = PCs[i + 1];
    uint64_t target_page_num = (PCs[i+1] >> PAGE_OFFSET_SIZE);
    uint16_t target_page_offset = (PCs[i+1] << (64-PAGE_OFFSET_SIZE)) >> (64-PAGE_OFFSET_SIZE);
	// TODO - Check page_table_index and saving the page number and comparison
	uint64_t page_index = get_a_page_index();
	pages[page_index] = target_page_num;
	page_valid[page_index] = true;
	// TODO Check update page_repl_ctr for this and others
	update_page_repl_ctr(page_index);
	table[index].page_table_index[i] = (PCs0_page_num == target_page_num) ? 0 : page_index;
	table[index].page_offset[i] = target_page_offset;
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

void gshare::update(gshare_prediction& upd_pred, bool prediction_correct) {
  uint16_t index     = upd_pred.index;
  bool     tag_match = upd_pred.tag_match;
  uint8_t  ctr       = table[index].ctr;
  if (tag_match) {
    if (prediction_correct) {
      table[index].ctr = (ctr < GSHARE_CTRMAX) ? (ctr + 1) : GSHARE_CTRMAX;
      
      uint8_t page_index = table[index].page_table_index[1];
      if (page_index != 0)
      {
      	update_page_repl_ctr(page_index);
      }
#ifdef DEBUG_UPDATE
      fprintf(gshare_log,
              "Update on correct prediction for PC = %#llx, updated index = %d's counter to %u\n",
              upd_pred.info.PCs[0],
              index,
              table[index].ctr);
#endif
    } else {
    // TODO Check if 
      table[index].ctr = (ctr > 0) ? (ctr - 1) : 0;
#ifdef DEBUG_UPDATE
      fprintf(gshare_log,
              "Update on misprediction for PC = %#llx, updated index = %d's counter to %u\n",
              upd_pred.info.PCs[0],
              index,
              table[index].ctr);
#endif
    }
  }
}

void gshare::size ()
{
	uint64_t pages_size = NUM_PAGE_TABLE_ENTRIES * ( log2(NUM_PAGE_TABLE_ENTRIES/NUM_PAGES_PER_GROUP) + (40 - PAGE_OFFSET_SIZE));
	uint64_t table_size = NUM_GSHARE_ENTRIES * gshare_entry::size(PAGE_TABLE_INDEX_SIZE, PAGE_OFFSET_SIZE);
	printf ("pages size in bits = %lu, table size = %lu, gshare size = %lu \n", pages_size, table_size, pages_size + table_size);
}

#ifdef DEBUG_UTILIZATION
void gshare::get_gshare_utilization (uint64_t* buffer) {
  buffer[0] = num_unallocated;
  buffer[1] = num_collisions_blocked;
  buffer[2] = num_collisions_replaced;
  return;
}
#endif
