// bitboard.c

#include "defs.h"

U64 SetMask[64];	// Bit masks for each square, with one corresponding bit set
U64 ClrMask[64];	// Bit masks for each square, with all but the corresponding bit set

U64 FileMask[8];	// Bit masks indexed by file, with all bits of the corresponding file set
U64 RankMask[8];	// Bit masks indexed by rank, with all bits of the corresponding rank set

U64 IsoMask[64];	// Bit masks for each square, with the file to the right and left of it set
// A set of bit masks indexed by color and square, with all bits set that represent squares
// that determine if a pawn is a passed pawn,
// e.g. for a white pawn on e4 the mask would look like this:
// 0 0 0 1 1 1 0 0
// 0 0 0 1 1 1 0 0
// 0 0 0 1 1 1 0 0
// 0 0 0 1 1 1 0 0
// 0 0 0 0 0 0 0 0
// 0 0 0 0 0 0 0 0
// 0 0 0 0 0 0 0 0
// 0 0 0 0 0 0 0 0
U64 PassedMask[2][64];

// 0 0 0 1 0 1 0 0
// 0 0 0 1 0 1 0 0
// 0 0 0 1 0 1 0 0
// 0 0 0 1 0 1 0 0
// 0 0 0 0 0 0 0 0
// 0 0 0 0 0 0 0 0
// 0 0 0 0 0 0 0 0
// 0 0 0 0 0 0 0 0
U64 OutpostMask[2][64];


// this function removes the lowest set bit in 'bb' and returns its location
inline int PopBit(U64 *bb) {
	register int loc =  LOCATEBIT(*bb);
	*bb &= (*bb-1);
	return loc;
}
// returns the number of set bits in 'b'
int CountBits(U64 b){
	int r;
	for(r = 0; b; r++, b &= b-1);
	return r;
}

// prints a chess board style representation of a bit board where a '-' is a 0 and a 'X' is a 1
// (mainly a debugging function)
void PrintBits(U64 b){
	int f, r;
	printf("    A B C D E F G H\n");
	for(r = 7; r >= 0; r--){
		printf("\n%-4d", 1+r);
		for(f = 0; f < 8; f++){
			if(( b>>((r<<3)+f) )&1) printf("X ");
			else printf("- ");
		}
	}
}

// returns the type of piece occupying square 'sq'
int GetPiece(U64 *bb, int sq)
{
	if(TESTBIT(bb[Pawn],sq)) return Pawn;
	if(TESTBIT(bb[Knight],sq)) return Knight;
	if(TESTBIT(bb[Bishop],sq)) return Bishop;
	if(TESTBIT(bb[Rook],sq)) return Rook;
	if(TESTBIT(bb[Queen],sq)) return Queen;
	if(TESTBIT(bb[King],sq)) return King;
	return Empty;
}

