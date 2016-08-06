// hash.c

#include "defs.h"

// constant variables for everything that influences the hash key
// they hold random numbers that can be hashed in and out via XOR
U64 pceHash[2][7][64];
U64 epHash[65];
U64 caHash[16];
U64 sideHash;

// generates and returns a hash key for the position 'brd'
U64 GenHash(const board_t *brd)
{
	int pce, color;
	U64 bb = 0ULL;
	U64 outHash = 0ULL;
	
	for(color = White; color <= Black; color++){
		for(pce = Pawn; pce <= King; pce++){
			bb = brd->bb[color][pce];
			while(bb) outHash ^= pceHash[color][pce][PopBit(&bb)];
		}
	}
	
	outHash ^= epHash[brd->enPas];
	
	outHash ^= caHash[brd->castle];
	if(brd->side == White) outHash ^= sideHash;
	
	return outHash;
}


