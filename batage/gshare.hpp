#include "batage.hpp"
#include <stdbool.h>
#include <stdio.h>
#include <vector>

#define NUM_GSHARE_ENTRIES 64
#define NUM_GSHARE_TAGBITS 12
#define NUM_GSHARE_CTRBITS 3
#define GSHARE_CTRMAX ((1 << NUM_GSHARE_CTRBITS) -1)
#define GSHARE_T_HIGHCONF 5

//TODO - Confirm these values
#define CTR_THRESHOLD 4
#define CTR_ALLOC_VAL 4

//#define DEBUG_ALLOC

class gshare_entry {
public :
#ifdef DEBUG_PC
uint64_t PC;
#endif
uint8_t ctr;
uint16_t tag;
vector <uint8_t> hist_patch;
vector <uint64_t> PCs;
vector <uint8_t> poses;

public :

// constructor
gshare_entry ()
{
	ctr 		= 0;
	tag		= 0;
	// TODO - Update from 2
	hist_patch.resize(2); // NUM_TAKEN_BRANCHES
	PCs.resize(2); // NUM_TAKEN_BRANCHES
	poses.resize(2); // NUM_TAKEN_BRANCHES
}

// Copy assignment
gshare_entry& operator=(const gshare_entry& rhs)
{
	ctr = rhs.ctr;
	tag = rhs.tag;
	hist_patch = rhs.hist_patch;
	PCs = rhs.PCs;
	poses = rhs.poses;
	return *this;
}
} ;

class gshare_prediction {
public :
	bool hit;
	// TODO - update - we do not want to return "everything" including tag, bad design, but ok for now
	gshare_entry info;
	uint16_t index;

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
public :

vector <gshare_entry> table;
gshare_prediction prediction;

public :

// constructor
gshare ()
{
	table.resize(NUM_GSHARE_ENTRIES);
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
	uint16_t tag = calc_tag(PC);
	return ( ( tag == table[index].tag) && (table[index].ctr >= CTR_THRESHOLD) );
} 

// TODO Check - Ques - When is gshare predict called - is it once for a SS packet - then how do we get pos ?
// Is it once per PC in packet ?
// Also how to infer the prediction - does it return taken, not taken ?
gshare_prediction& predict (uint64_t PC)
{	
	prediction.hit = false;
		#ifdef DEBUG_PREDICT
			//fprintf(stderr, "Predict called for fetch pc = %llx\n", PC);
		#endif
		#ifdef DEBUG_PC
		if ( (PC == 0xffffffff8010a4f2) || (PC == 0xffffffff8010a130) )
		{
			for (int i = 0; i < NUM_GSHARE_ENTRIES; i++)
			{
				if (PC == table[i].PC)
				{
					printf ("PC present at index = %d, ctr = %d\n", i, table[i].ctr);
					break;
				}
			}
		}
		#endif
	if (is_hit (PC))
	{
		uint16_t index = calc_index(PC);
		prediction.hit = true;
		prediction.index = index;
		prediction.info = table[index];
		#ifdef DEBUG_PREDICT
			fprintf(stderr, "*********************************** gshare hit index = %u ****************************************** \n", index);
		#endif
	}
	return prediction;
}

// TODO Check - Ques - Who calls this ? batage has conf info but not insn info  - for it all except taken branches are nt
// branchprof ?? - it does not have conf info - will need batage update
void allocate (vector <uint64_t>& PCs, vector <uint8_t>& poses)
{
	uint16_t index = calc_index (PCs[0]);
	uint16_t tag = calc_tag(PCs[0]);
	
	// TODO Check - If same tag - return, do we increase ctr ?
	if (table[index].tag == tag)
	{return;}
	
	uint8_t ctr = table[index].ctr;
	// TODO Check
	if (ctr > GSHARE_T_HIGHCONF)
	{
		#ifdef DEBUG_ALLOC
		printf ("Alloc called but ctr = %u is more than GSHARE_T_HIGHCONF = %u \n", ctr, GSHARE_T_HIGHCONF);
		#endif // DEBUG_ALLOC
		table[index].ctr = ctr - 1;
		return;
	}
	else
	{
		table[index].ctr = CTR_ALLOC_VAL;
		// TODO - Update
		for (int i = 0; i < 2; i++) // NUM_TAKEN_BRANCHES
		{
			table[index].tag = tag;
			table[index].PCs[i] = PCs[i+1];
			table[index].poses[i] = poses[i];
			#ifdef DEBUG_PC
			table[index].PC = PCs[0];
			if (table[index].PC == 0xffffffff8010a4f2)
			printf ("check tag \n");
			#endif
		}
		#ifdef DEBUG_ALLOC
		printf ("Alloc done at index = %u, for fetch_pc = %llx, saved branch to %llx at pos = %u and  branch to %llx at pos = %u\n", index, PCs[0], PCs[1], poses[0],  PCs[2], poses[1]);
		#endif // DEBUG_ALLOC
	}
}


void update (gshare_prediction& prediction, bool prediction_correct)
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
