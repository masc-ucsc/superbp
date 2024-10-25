/*
Structure - 
Target - 57 bits

Offset - Stored with entry
delta - bit, 0 = page and region from Branch PC, 1 = page and region from BTBM - opposite from paper, so 0 is defult and more frequent
BTBM - indexed by branch pc.
Region btb pointer with entry indexed using corresponding region bits of branch PC
Page btb pointer with entry indexed using corresponding page bits of branch PC

Meta data bits - 1 bit PID, 2 bits SRRIP, 1 delta, 2 confidence bits.
 
Allocation - Only store taken branches
Allocations in BTBM only if allocations in Page and Region BTBs succeed.
Allocations in region and page btb are srrip guided
 
Prediction - 1CC BTB lookup for same page branches that do not need to read reagion/ page ;2 CC for different page branches
Tag matching to get to btb entry.
Page and region pointers are direct memory addresses, no tag matching. 

Update -

Replacement in btbm and region and page btbs is independent.
Size - 

*/

// Must support SS
// TODO Check all
#define BTB_ENTRIES_PER_WAY 	(1024)
#define BTB_ASSOCIATIVITY 		4
#define NUM_PAGE_ENTRIES 		64
#define NUM_REGION_ENTRIES 		32

#define NUM_REGION_BITS			22
#define NUM_PAGE_BITS 			23
#define NUM_OFFSET_BITS			12
#define NUM_TAG_BITS			12


class pdede {
private :
unsigned int 			tag [BTB_ASSOCIATIVITY][BTB_ENTRIES_PER_WAY];
unsigned long long int 	offset [BTB_ASSOCIATIVITY][BTB_ENTRIES_PER_WAY];
unsigned int 			page [NUM_PAGE_ENTRIES];
unsigned int 			region [NUM_REGION_ENTRIES];
bool					delta [BTB_ASSOCIATIVITY][BTB_ENTRIES_PER_WAY];
signed char 			confidence [BTB_ASSOCIATIVITY][BTB_ENTRIES_PER_WAY];


public :

// constructor
void pdede ()
{
	for int (i = 0; i < BTB_ASSOCIATIVITY, i++)
	{
		for (int j = 0; j < BTB_ENTRIES_PER_WAY; j++)
		{
			tag[i][j] 			= 0;
			offset[i][j]		= 0;
			delta[i][j] 		= false;
			confidence[i][j]	= 0;
		}
	}
	for int (i = 0; i < NUM_REGION_ENTRIES, i++)
	{
		region[i] = 0;
	}
	for int (i = 0; i < NUM_PAGE_ENTRIES, i++)
	{
		page[i] = 0;
	}
} 

// size in bits
unsigned long long int size ()
{
	return ((BTB_ASSOCIATIVITY * BTB_ENTRIES_PER_WAY * (NUM_TAG_BITS + NUM_OFFSET_BITS + 1 + 2)) + (NUM_REGION_ENTRIES * NUM_REGION_BITS) + (NUM_PAGE_ENTRIES * NUM_PAGE_BITS));
}

// predict - must support SS

// Update/ Allocate - must support SS

}
