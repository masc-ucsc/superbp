#pragma once

#include "batage.hpp"
#include <stdbool.h>
#include <stdio.h>
#include <vector>

#define NUM_GSHARE_ENTRIES (1 << 12)
#define NUM_GSHARE_TAGBITS 12
#define NUM_GSHARE_CTRBITS 3
#define GSHARE_CTRMAX ((1 << NUM_GSHARE_CTRBITS) -1)
#define GSHARE_T_HIGHCONF 4

//TODO - Confirm these values
#define CTR_THRESHOLD 4
#define CTR_ALLOC_VAL 4

//#define DEBUG_PREDICT
//#define DEBUG_ALLOC
//#define DEBUG_UPDATE


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
	hist_patch.resize(2);  // NUM_TAKEN_BRANCHES
	PCs.resize(2);  // NUM_TAKEN_BRANCHES
	poses.resize(2);  // NUM_TAKEN_BRANCHES
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
	bool tag_match;
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
//FILE *gshare_log;

public :

// constructor
gshare ()
{
	table.resize(NUM_GSHARE_ENTRIES);
	//start_log();
}

void start_log()
{
	//gshare_log = fopen("gshare_trace.txt", "w");
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

bool is_hit (uint64_t PC,uint16_t index, uint16_t tag)
{
	//uint16_t index = calc_index(PC);
	//uint16_t tag = calc_tag(PC);
	uint8_t ctr = table[index].ctr;
	
	if ((tag == table[index].tag))
	{ 
		prediction.tag_match = true;
	
	#ifdef DEBUG_PREDICT
	if  (ctr < CTR_THRESHOLD)
	{fprintf( gshare_log, "gshare tag matched miss for fetchPC = %#llx at index = %u, with tag = %#x, since ctr = %u < CTR_THRESHOLD = %u\n", PC, index, tag, ctr, CTR_THRESHOLD); }
	#endif
	}
	
	return ( prediction.tag_match && (ctr >= CTR_THRESHOLD) );
} 

// TODO Check - Ques - When is gshare predict called - is it once for a SS packet - then how do we get pos ?
// Is it once per PC in packet ?
// Also how to infer the prediction - does it return taken, not taken ?
gshare_prediction& predict (uint64_t PC, uint16_t index, uint16_t tag)
{	
	index %= NUM_GSHARE_ENTRIES;
	prediction.index = index;
	prediction.hit = false;
	prediction.tag_match = false;
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
	//if ()
	{
		//uint16_t index = calc_index(PC);
		prediction.hit = is_hit (PC, index, tag);
		prediction.index = index;
		prediction.info = table[index];
		#ifdef DEBUG_PREDICT
		if (prediction.hit)
			{fprintf( gshare_log, "gshare hit index = %u for fetchPC = %#llx, sent tag = %#x, pos[0] = %u, PC[0] = %#llx, pos[1] = %u, PC[1] = %#llx \n", index, PC, tag, prediction.info.poses[0], prediction.info.PCs[0], prediction.info.poses[1], prediction.info.PCs[1]);}
		#endif
	}
	return prediction;
}

void allocate (vector <uint64_t>& PCs, vector <uint8_t>& poses, uint16_t update_gshare_index, uint16_t update_gshare_tag)
{
	uint16_t index = update_gshare_index % NUM_GSHARE_ENTRIES; // calc_index (PCs[0]);
	uint16_t tag = update_gshare_tag; //calc_tag(PCs[0]);
	uint16_t old_tag = table[index].tag;
	
	// TODO Check - If same tag - return, do we increase ctr ?
	if (old_tag == tag)
	{return;}
	
	uint8_t ctr = table[index].ctr;
	// TODO Check
	if (ctr > GSHARE_T_HIGHCONF)
	{
		#ifdef DEBUG_ALLOC
		fprintf( gshare_log, "Alloc called for tag = %#x, old_tag = %#x, but ctr = %u is more than GSHARE_T_HIGHCONF = %u \n", tag, old_tag, ctr, GSHARE_T_HIGHCONF);
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
		}
		#ifdef DEBUG_ALLOC
		fprintf( gshare_log, "Alloc done at index = %u for sent index update_gshare_index = %u, intended for fetch_pc = %#llx, saved branch to %#llx at pos = %u and  branch to %#llx at pos = %u, saving tag = %#x over old_tag = %#x\n", index, update_gshare_index, PCs[0], PCs[1], poses[0],  PCs[2], poses[1], tag, old_tag);
		#endif // DEBUG_ALLOC
	}
}


void update (gshare_prediction& prediction, bool prediction_correct)
{
	uint16_t index = prediction.index;
	bool tag_match = prediction.tag_match;
	uint8_t ctr = table[index].ctr;
	if (tag_match)
	{
		if (prediction_correct)
		{
			table[index].ctr = (ctr < GSHARE_CTRMAX) ? (ctr +1) : GSHARE_CTRMAX;
			#ifdef DEBUG_ALLOC
				fprintf( gshare_log, "Update on correct prediction for PC = %#llx, updated index = %d's counter to %u\n", prediction.info.PCs[0], index, table[index].ctr);
			#endif
		}
		else
		{
			table[index].ctr = (ctr > 0) ? (ctr - 1) : 0;
			#ifdef DEBUG_ALLOC
				fprintf( gshare_log, "Update on misprediction for PC = %#llx, updated index = %d's counter to %u\n", prediction.info.PCs[0], index, table[index].ctr);
			#endif
		}
	}
}
};
