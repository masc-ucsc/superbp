#include "batage.hpp"
#include <stdbool.h>

#define NUM_GSHARE_ENTRIES 64
#define NUM_GSHARE_TAGBITS 12
#define NUM_GSHARE_CTRBITS 3
#define GSHARE_CTRMAX ((1 << NUM_GSHARE_CTRBITS) -1)
#define GSHARE_T_HIGHCONF 5

//TODO - Confirm these values
#define CTR_THRESHOLD 5
#define CTR_ALLOC_VAL 4

class gshare_prediction {
public :
	bool hit;
	// TODO - update - we do not want to return "everything" including tag, bad design, but ok for now
	gshare_entry info;
	uint16_t index;

	// Default constructor for uninitialized instance
	gshare_prediction() = default; 
}

class gshare_entry {
public :
uint8_t ctr;
uint16_t tag;
vector <uint8_t> hist_patch;
vector <uint64_t> PCs;
vector <uint8_t> poses;

public :

// constructor
void gshare_entry ()
{
	ctr 		= 3;
	tag		= 0;
	// TODO - Update from 2
	hist_patch.reserve(2); // NUM_TAKEN_BRANCHES
	PCs.reserve(2); // NUM_TAKEN_BRANCHES
	poses.reserve(2); // NUM_TAKEN_BRANCHES
}

// Copy assignment
gshare_entry& operator=(const gshare_entry& rhs)
{
	ctr = rhs.ctr;
	tag - rhs.tag;
	hist_patch = rhs.hist_patch;
	PCs = rhs.PCs;
	poses = rhs.poses;
	return *this;
}
} 

class gshare {
public :

vector <gshare_entry> table;

public :

// constructor
void gshare ()
{
	table.reserve(NUM_GSHARE_ENTRIES);
}

uint16_t calc_tag (uint64_t PC)
{
	// TODO Update  
	return ( (uint16_t)(PC ^ (PC << 2) ^ (PC >> 16) ^ (PC >> 32)) ); 
}

uint16_t calc_index (uint64_t PC)
{
	// TODO Update  
	return ( PC % NUM_GSHARE_ENTRIES); 
}

bool is_hit (uint64_t PC)
{
	uint16_t index = calc_index(PC);
	uint16_t tag = calc_tag(PC)
	return ( ( tag == table[index].tag) && (table[index].ctr > CTR_THRESHOLD) );
} 

// TODO Check - Ques - When is gshare predict called - is it once for a SS packet - then how do we get pos ?
// Is it once per PC in packet ?
// Also how to infer the prediction - does it return taken, not taken ?
gshare_prediction predict (uint64_t PC)
{
	gshare_prediction prediction;
	
	if (is_hit (PC))
	{
		prediction.hit = true;
		prediction.index = index;
		prediction.info = table[index];
	}
}

// TODO Check - Ques - Who calls this ? batage has conf info but not insn info  - for it all except taken branches are nt
// branchprof ?? - it does not have conf info - will need batage update
void allocate (vector <uint64_t>& PCs, vector <uint64_t>& poses)
{
	uint16_t index = calc_index (PCs[0]);
	uint8_t ctr = table[index].ctr;
	// TODO Check
	if (ctr > GSHARE_T_HIGHCONF)
	{
		table[index].ctr = ctr - 1;
		return;
	}
	else
	{
		table[index].tag = calc_tag(PC[0]);
		table[index].ctr = CTR_ALLOC_VAL;
		// TODO - Update
		for (int i = 0; i < 2; i++) // NUM_TAKEN_BRANCHES
		{
			table[index].PCs[i] = PCs[i+1];
			table[index].poses[i] = poses[i+1];
		}
	}
}


void update (bool prediction_correct, gshare_prediction& prediction)
{
	uint16_t index = prediction.index;
	uint8_t ctr = table[index].ctr;
	if (prediction_correct)
	{
		table[index].ctr = (ctr < GSHARE_CTRMAX) ? (ctr +1) : GSHARE_CTRMAX;
	}
	else
	{
		table[index].ctr = (ctr > 0) ? (ctr - 1) : 0;
	}
}
};
