// pv.c

#include "defs.h"

// The amount of memory that should be allocated for the transposition table
int pvSize = 0x2000000; // 32 MB

void ClrPv(pvtable_t *pv){
	memset(pv->pTable, 0, pv->len * sizeof(hashentry_t));
}

// Allocates and initialises the memory for the transposition table
void InitPv(pvtable_t *pv)
{
	pv->len = pvSize / sizeof(hashentry_t);
	pv->pTable = (hashentry_t *) malloc(pv->len * sizeof(hashentry_t));
	pv->len -= 2;
	ClrPv(pv);
}

// stores information in the transposition table
void StorePvMove(board_t *brd, int move, int depth, int score, int flags)
{
	register int i = brd->hash % brd->pv.len;
	brd->pv.pTable[i].hash = brd->hash;
	brd->pv.pTable[i].move = move;
	brd->pv.pTable[i].depth = depth;
	brd->pv.pTable[i].score = score;
	brd->pv.pTable[i].flags = flags;
}

// checks if there is a move entry in the transposition table for the current position
// if so, it returns the move stored (used to retrieve the pv)
int TestBrdPv(board_t *brd)
{
	int i = brd->hash % brd->pv.len;
	if(brd->pv.pTable[i].hash == brd->hash && brd->pv.pTable[i].flags == HFEXACT)
		return brd->pv.pTable[i].move;
	return NO_MOVE;
}

// checks if there is an entry in the transposition table for the current position
// and if that entry can be used to skip the the search for it
int TestHashTable(board_t *brd, int *move, int *score, int alpha, int beta, int depth)
{
	int i = brd->hash % brd->pv.len;
	if(brd->pv.pTable[i].hash == brd->hash){
		*move = brd->pv.pTable[i].move;			// retrieve the move stored
		if(depth <= brd->pv.pTable[i].depth){	// if the data is accurate enough for our depth
			*score = brd->pv.pTable[i].score;	// then use the saved score
			switch(brd->pv.pTable[i].flags){
			case HFEXACT:
				return true;			// if it was an exact value simply use it
				break;
			case HFALPHA:
				if(*score <= alpha){	// if we couldn't beat alpha
					*score = alpha;		// then we couldn't improve it
					return true;
				}
				break;
			case HFBETA:
				if(*score >= beta){		// if we had a beta cutoff which beats this beta
					*score = beta;		// then we can safely return this beta
					return true;
				}
				break;
			default:
				ASSERT(false);	// should not get here
			}
		}
	}
	return false;
}

// Fills 'pvLine' with the best succession of moves through the pv up to length 'depth'
// its return value is the length of the retrieved pv
int GetPvLine(board_t *brd, int depth) 
{
	int len = 0;
	int move = TestBrdPv(brd);
	
	// as long as there is a best move stored for the current position
	while(move != NO_MOVE && len < depth){
		if(!MakeMove(brd, move)) break;	// make the move
		brd->pvLine[len++] = move; 			// and store it
		move = TestBrdPv(brd);
	}
	while(brd->ply > 0) TakeBack(brd);		// undo all the moves made in the process

	return len;
}



