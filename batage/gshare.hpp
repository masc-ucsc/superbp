#pragma once

#include <stdbool.h>
#include <stdio.h>

#include <vector>

#include "batage.hpp"

#define NUM_GSHARE_ENTRIES (1 << 10)

#define PAGE_OFFSET_SIZE (12)
#define PAGE_SIZE (1 << PAGE_OFFSET_SIZE)

#define PAGE_TABLE_INDEX_SIZE (10)
#define NUM_PAGE_TABLE_ENTRIES (1 << PAGE_TABLE_INDEX_SIZE)

#define NUM_GSHARE_TAGBITS 12
#define NUM_GSHARE_CTRBITS 3
#define GSHARE_CTRMAX      ((1 << NUM_GSHARE_CTRBITS) - 1)

// blocked if counter > HIGH_CONF
#define GSHARE_T_HIGHCONF 4

// TODO - Confirm these values
//  Hit if >= Threshold
#define CTR_THRESHOLD 4
#define CTR_ALLOC_VAL 4

// #define DEBUG_PREDICT
// #define DEBUG_ALLOC
// #define DEBUG_UPDATE
#define DEBUG_UTILIZATION

/*
class page_table {
public:
    // Constructor
  //page_table();
};
*/

class gshare_entry {
public:
#ifdef DEBUG_PC
  uint64_t PC;
#endif
#ifdef DEBUG_UTILIZATION
  bool allocated;
#endif
  uint8_t          ctr;
  uint16_t         tag;
  vector<uint8_t>  hist_patch;
  vector<uint8_t>  poses;
  
  //vector<uint64_t> PCs;
  vector<bool> same_page;					// 1 bit
  vector<uint64_t> page_table_index;		// actual size = PAGE_TABLE_INDEX_SIZE
  vector<uint16_t> page_offset;			// actual size = PAGE_OFFSET_SIZE

public:
  // constructor
  gshare_entry();

  // Copy assignment
  gshare_entry& operator=(const gshare_entry& rhs) {
    ctr        = rhs.ctr;
    tag        = rhs.tag;
    hist_patch = rhs.hist_patch;
    poses      = rhs.poses;

    //PCs        = rhs.PCs;
    same_page = rhs.same_page;   
    page_table_index = rhs.page_table_index;
    page_offset = rhs.page_offset;
 
    return *this;
  }
  
  static uint64_t size();
};

class gshare_entry_formed {
public:
#ifdef DEBUG_PC
  uint64_t PC;
#endif
#ifdef DEBUG_UTILIZATION
  bool allocated;
#endif
  uint8_t          ctr;
  uint16_t         tag;
  vector<uint8_t>  hist_patch;
  vector<uint8_t>  poses;
  
  vector<uint64_t> PCs;
public:
  // constructor
  gshare_entry_formed();
  gshare_entry_formed(const uint64_t PC, const gshare_entry& rhs, vector<uint64_t>&  pages);
  // Copy assignment
  gshare_entry_formed& operator=(const gshare_entry_formed& rhs) ;

};

class gshare_prediction {
public:
  bool hit;
  bool tag_match;
  // TODO - update - we do not want to return "everything" including tag, bad design, but ok for now
  gshare_entry_formed info;
  uint16_t     index;

  // Default constructor for uninitialized instance
  gshare_prediction() = default;
  /*	gshare_prediction()
          {
                  hit = false;
                  index = 0;
          }

  gshare_prediction& operator=(const gshare_prediction& rhs)
  {
          hit = (rhs.hit);
          info = (rhs.info);
          return *this;
  }*/
};

class gshare {
public:

	friend class gshare_entry_formed;
	vector<uint64_t>  pages;	// only contains page numbers, size = 64 - PAGE_OFFSET_SIZE

	// table of predictions - not of pages
  vector<gshare_entry> table;
  gshare_prediction    prediction;
  // FILE *gshare_log;
  uint32_t decay_ctr;

#ifdef DEBUG_UTILIZATION
  uint32_t num_unallocated;
  uint64_t num_collisions_blocked;
  uint64_t num_collisions_replaced;
#endif

public:
  // constructor
  gshare();

  void start_log() {
    // gshare_log = fopen("gshare_trace.txt", "w");
  }

  uint16_t calc_tag(uint64_t PC);

  uint16_t calc_index(uint64_t PC);

  void decay();

  bool is_hit(uint64_t PC, uint16_t index, uint16_t tag);

  // TODO Check - Ques - When is gshare predict called - is it once for a SS packet - then how do we get pos ?
  // Is it once per PC in packet ?
  // Also how to infer the prediction - does it return taken, not taken ?
  gshare_prediction& predict(uint64_t PC, uint16_t index, uint16_t tag);

  uint64_t get_a_page_index();
  void allocate(vector<uint64_t>& PCs, vector<uint8_t>& poses, uint16_t update_gshare_index, uint16_t update_gshare_tag);

  void update(gshare_prediction& prediction, bool prediction_correct);
  
  static void size ();

#ifdef DEBUG_UTILIZATION
  void get_gshare_utilization(uint64_t* buffer);
#endif
};
