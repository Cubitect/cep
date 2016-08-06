// eval.c

#include "defs.h"

const int pceMat[7] = {0,100,300,320,500,915,0};	// Material value of the pieces

const int endMaterial = 1500;		// Material boundary when the game is considered an end game
const int noQendMaterial = 2000;	// Material boundary for end game when there are no queens

// bonus for passed pawns depending on their advance
const int pawnPassed[8] = { 0, 5, 10, 20, 35, 60, 100, 200 };

const int isoPawn = -15;			// Isolated pawn penalty
const int openRook = 20;			// Rook on open file bonus
const int semiOpenRook = 8;		// Rook on semi-open file bonus
const int openQueen = 4;			// Queen on open file bonus
const int semiOpenQueen = 2;		// queen on semi-open file bonus
const int kingSafe = 3;			// middle game bonus if king is behind pieces
const int bishopPair = 15;			// bonus for having both bishops
// adjustments of material value for closed games
const int closedKnight = 10;
const int closedBishop = -10;
const int closedRook = -10;

const int uselessPiece = -12;		// penalty for a piece that has nowhere to go
const int semiUseless = -6;
const int sideOutpost = 4;
const int outpost = 12;

// The values in the tables are added to the piece according to the position of the piece
// eg. a pawn on e4 has a score of +20 centipawns so the engine will try to control the center

const int PawnTable[64] = {
    0,   0,   0,   0,   0,   0,   0,   0,
   10,  10,   0, -10, -10,   0,  10,  10,
    5,   0,   0,   5,   5,   0,   0,   5,
    0,   0,  10,  20,  20,  10,   0,   0,
    5,   5,   5,  10,  10,   5,   5,   5,
   10,  10,  10,  20,  20,  10,  10,  10,
   20,  20,  20,  30,  30,  20,  20,  20,
    0,   0,   0,   0,   0,   0,   0,   0
};

// The king and pawns have different score tables for the end game
// as their behaviour has to change drastically
const int PawnEndTable[64] = {
    0,   0,   0,   0,   0,   0,   0,   0,
    7,   7,   0, -10, -10,   0,   7,   7,
    5,   2,   0,   4,   4,   0,   2,   5,
    4,   3,  10,  10,  10,  10,   3,   4,
    8,   8,  12,  14,  14,  12,   8,   8,
   15,  18,  20,  24,  24,  15,  15,  15,
   24,  24,  24,  34,  34,  24,  24,  24,
    0,   0,   0,   0,   0,   0,   0,   0
};

const int KnightTable[64] = {
    0, -10,   0,   0,   0,   0, -10,   0,
    0,   0,   0,   5,   5,   0,   0,   0,
    0,   0,   9,  10,  10,   9,   0,   0,
    0,   0,  10,  20,  20,  10,   5,   0,
    5,  10,  15,  20,  20,  15,  10,   5,
    5,  10,  10,  20,  20,  10,  10,   5,
    0,   0,   5,  10,  10,   5,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0
};

const int BishopTable[64] = {
    0,   0, -10,   0,   0, -10,   0,   0,
    0,   6,   0,   7,   7,   0,   6,   0,
    0,   0,  10,  15,  15,  10,   0,   0,
    0,  10,  15,  18,  18,  15,  10,   0,
    0,  10,  15,  18,  18,  15,  10,   0,
    0,   0,  10,  15,  15,  10,   0,   0,
    0,   0,   0,  10,  10,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0
};

const int RookTable[64] = {
   -4,   0,   8,  10,  10,   8,   0,  -4,
    0,   0,   8,  10,  10,   8,   0,   0,
    0,   0,   8,  10,  10,   8,   0,   0,
    0,   0,   8,  10,  10,   8,   0,   0,
    0,   0,   8,  10,  10,   8,   0,   0,
    0,   0,   8,  10,  10,   8,   0,   0,
   15,  15,  15,  15,  15,  15,  15,  15,
    0,   0,   8,  10,  10,   8,   0,   0
};

const int KingTable[64] = {
	0,   5,  20, -10,   0, -10,  20,   5,
  -15, -15, -15, -15, -15, -15, -15, -15,
  -60, -60, -60, -60, -60, -60, -60, -60,
  -60, -60, -60, -60, -60, -60, -60, -60,
  -60, -60, -60, -60, -60, -60, -60, -60,
  -60, -60, -60, -60, -60, -60, -60, -60,
  -60, -60, -60, -60, -60, -60, -60, -60,
  -60, -60, -60, -60, -60, -60, -60, -60
};

const int KingEndTable[64] = {
  -40, -10,   0,   0,   0,   0, -10, -40,
  -10,   0,   5,   5,   5,   5,   0, -10,
    0,   5,  10,  15,  15,  10,   5,   0,
    0,   5,  15,  20,  20,  15,   5,   0,
    0,   5,  15,  20,  20,  15,   5,   0,
    0,   5,  10,  15,  15,  10,   5,   0,
  -10,   0,   5,   5,   5,   5,   0, -10,
  -40, -10,   0,   0,   0,   0, -10, -40
};

// This contains the indices for the tables from the point of view of black (flipped around)
const int mirror[64] = {
   56,  57,  58,  59,  60,  61,  62,  63,
   48,  49,  50,  51,  52,  53,  54,  55,
   40,  41,  42,  43,  44,  45,  46,  47,
   32,  33,  34,  35,  36,  37,  38,  39,
   24,  25,  26,  27,  28,  29,  30,  31,
   16,  17,  18,  19,  20,  21,  22,  23,
    8,   9,  10,  11,  12,  13,  14,  15,
    0,   1,   2,   3,   4,   5,   6,   7
};

// save some writing
#define BC(arg) (CountBits(arg))


// This function returns true if a position is a theoretical draw and otherwise false
// The function assumes there are no queens and/or pawns on the board
int MaterialDraw(board_t *brd)
{
	if(!brd->bb[Both][Rook]){
		if(!brd->bb[Both][Bishop]){
			if(BC(brd->bb[White][Knight]) < 3 && BC(brd->bb[Black][Knight]) < 3) return true;
		}
		else if(!brd->bb[Both][Knight]){
			if(abs(BC(brd->bb[White][Bishop]) - BC(brd->bb[Black][Bishop])) < 2) return true;
		}
		else if((BC(brd->bb[White][Knight]) < 3 && !brd->bb[White][Bishop]) ||
				 (BC(brd->bb[White][Bishop])== 1 && !brd->bb[White][Knight]))
		{
			if((BC(brd->bb[Black][Knight]) < 3 && !brd->bb[Black][Bishop]) ||
			   (BC(brd->bb[Black][Bishop])== 1 && !brd->bb[Black][Knight])){
				return true;
			}
		}
	}
	else if(BC(brd->bb[White][Rook])==1 && BC(brd->bb[Black][Rook])==1){
		if(BC(brd->bb[White][Bishop]&brd->bb[White][Knight]) < 2 &&
		   BC(brd->bb[Black][Bishop]&brd->bb[Black][Knight]) < 2){
			return true;
		}
	}
	else if(BC(brd->bb[White][Rook]) == 1 && !brd->bb[Black][Rook] &&
			   !brd->bb[White][Knight] && !brd->bb[White][Bishop]){
		if(BC(brd->bb[Black][Knight] & brd->bb[Black][Bishop]) == 1 ||
		   BC(brd->bb[Black][Knight] & brd->bb[Black][Bishop]) == 2 ){
			return true;
		}
	}
	else if(BC(brd->bb[Black][Rook]) == 1 && !brd->bb[White][Rook] &&
		   !brd->bb[Black][Knight] && !brd->bb[Black][Bishop]){
		if(BC(brd->bb[White][Knight] & brd->bb[White][Bishop]) == 1 ||
		   BC(brd->bb[White][Knight] & brd->bb[White][Bishop]) == 2 ){
			return true;
		}
	}
	return false;
}

// evaluates the position 'brd' and returns a measure (in centipawns) of how good the position looks 
// it evaluates the position from white's point of view and returns the negated score at the end
// if it is black's turn
int Eval(board_t *brd)
{
	int sq, free;
	U64 b, wpatt, bpatt;

	// the most basic score to start with consisting of material values only
	int score = brd->material[White] - brd->material[Black];
	int closed;

	// determine if we have a closed game
	if(BC(brd->bb[Both][Pawn] & (0x0000ffffffff0000)) >= 9) closed = true;
	else closed = false;

	// if there are no queens/pawns on the board check if we have a theoretical draw
	if(!brd->bb[Both][Pawn] && !brd->bb[Both][Queen]){
		if(MaterialDraw(brd)) return 0;
	}

	wpatt = (((brd->bb[White][Pawn]<<7) & NO_H_FILE) |
			 ((brd->bb[White][Pawn]<<9) & NO_A_FILE));

	bpatt = (((brd->bb[Black][Pawn]>>7) & NO_A_FILE) |
			 ((brd->bb[Black][Pawn]>>9) & NO_H_FILE));


	b = brd->bb[White][Pawn];	// copy the set of positions of white pawns
	while(b){					// as long as a pawn remains
		sq = PopBit(&b);		// remove the last one and get its position

		// if we are in the endgame, use the endgame pawn table
		if(brd->material[Black] <= endMaterial) score += PawnEndTable[sq];
		else if(!brd->bb[Black][Queen] && brd->material[Black] <= noQendMaterial){
			score += PawnEndTable[sq];
		}
		else score += PawnTable[sq];

		// if the pawn is a passed pawn, give it the passed pawn bonus
		if(!(PassedMask[White][sq] & brd->bb[Black][Pawn]))
			score += pawnPassed[RANK(sq)];

		// if it is isolated, give it the isolated pawn penalty
		if(!(IsoMask[sq] & brd->bb[White][Pawn]))
			score += isoPawn;
	}

	// the same as for white pawns but mirrored
	// subtract values from the score for black's pieces
	b = brd->bb[Black][Pawn];
	while(b){
		sq = PopBit(&b);
		if(brd->material[White] <= endMaterial) score -= PawnEndTable[mirror[sq]];
		else if(!brd->bb[White][Queen] && brd->material[White] <= noQendMaterial){
			score -= PawnEndTable[mirror[sq]];
		}
		else score -= PawnTable[mirror[sq]];

		if(!(PassedMask[Black][sq] & brd->bb[White][Pawn]))
			score -= pawnPassed[7-RANK(sq)];
		if(!(IsoMask[sq] & brd->bb[Black][Pawn]))
			score -= isoPawn;
	}

	b = brd->bb[White][Knight];
	while(b){
		sq = PopBit(&b);			// for all white knights
		score += KnightTable[sq];	// add the appropriate score table value
		if(closed) score += closedKnight;	// if we have a closed game adjust the score
		if(!(OutpostMask[White][sq] & brd->bb[Black][Pawn])){
			if((1L<<sq) & 0x00ffffff00000000 & wpatt){
				if((1L<<sq) & 0x003c3c3c00000000 & wpatt) score += outpost;
				else score += sideOutpost;
			}
		}
		if(!CanKnightMove(brd, ~bpatt, sq, White)) score += uselessPiece;
	}
	b = brd->bb[Black][Knight];
	while(b){
		sq = PopBit(&b);
		score -= KnightTable[mirror[sq]];
		if(closed) score -= closedKnight;
		if(!(OutpostMask[Black][sq] & brd->bb[White][Pawn])){
			if((1L<<sq) & 0x00000000ffffff00 & bpatt){
				if((1L<<sq) & 0x000000003c3c3c00 & bpatt) score -= outpost;
				else score -= sideOutpost;
			}
		}
		if(!CanKnightMove(brd, ~wpatt, sq, Black)) score -= uselessPiece;
	}

	b = brd->bb[White][Bishop];
	if(CountBits(b) > 1) score += bishopPair; // Give the bishop pair bonus if there is more than one
	while(b){
		sq = PopBit(&b);
		score += BishopTable[sq];
		if(closed) score += closedBishop;
		if(!(OutpostMask[White][sq] & brd->bb[Black][Pawn])){
			if((1L<<sq) & 0x00ffffff00000000 & wpatt){
				if((1L<<sq) & 0x003c3c3c00000000 & wpatt) score += outpost;
				else score += sideOutpost;
			}
		}
	}
	b = brd->bb[Black][Bishop];
	if(CountBits(b) > 1) score -= bishopPair;
	while(b){
		sq = PopBit(&b);
		score -= BishopTable[mirror[sq]];
		if(closed) score -= closedBishop;
		if(!(OutpostMask[Black][sq] & brd->bb[White][Pawn])){
			if((1L<<sq) & 0x00000000ffffff00 & bpatt){
				if((1L<<sq) & 0x000000003c3c3c00 & bpatt) score -= outpost;
				else score -= sideOutpost;
			}
		}
	}

	b = brd->bb[White][Rook];
	while(b){
		sq = PopBit(&b);
		// Use the rook table values only if we are not in the endgame
		if(brd->material[Black] > endMaterial) score += RookTable[sq];

		if(!(FileMask[FILE(sq)] & brd->bb[Both][Pawn]))
			score += openRook;		// open file bonus
		else if(!(FileMask[FILE(sq)] & brd->bb[White][Pawn]))
			score += semiOpenRook;	// semi-open file bonus

		if(closed) score += closedRook;
	}
	b = brd->bb[Black][Rook];
	while(b){
		sq = PopBit(&b);
		if(brd->material[White] > endMaterial) score -= RookTable[mirror[sq]];

		if(!(FileMask[FILE(sq)] & brd->bb[Both][Pawn]))
			score -= openRook;
		else if(!(FileMask[FILE(sq)] & brd->bb[Black][Pawn]))
			score -= semiOpenRook;
		if(closed) score -= closedRook;
	}

	b = brd->bb[White][Queen];
	while(b){
		sq = PopBit(&b);
		if(!(FileMask[FILE(sq)] & brd->bb[Both][Pawn]))
			score += openQueen;
		else if(!(FileMask[FILE(sq)] & brd->bb[White][Pawn]))
			score += semiOpenQueen;
	}
	b = brd->bb[Black][Queen];
	while(b){
		sq = PopBit(&b);
		if(!(FileMask[FILE(sq)] & brd->bb[Both][Pawn]))
			score -= openQueen;
		else if(!(FileMask[FILE(sq)] & brd->bb[Black][Pawn]))
			score -= semiOpenQueen;
	}

	sq = LOCATEBIT(brd->bb[White][King]);	// get the king's square
	if(brd->material[Black] <= endMaterial) score += KingEndTable[sq];
	else if(!brd->bb[Black][Queen] && brd->material[Black] <= noQendMaterial){
		score += KingEndTable[sq];
	}
	else{
		score += KingTable[sq];
		// Add the king safety bonus for each piece in front of the king 
		// if that piece has the same colour as the king
		if((brd->bb[White][King] << 7) & brd->all[White]) score += kingSafe;
		if((brd->bb[White][King] << 8) & brd->all[White]) score += kingSafe;
		if((brd->bb[White][King] << 9) & brd->all[White]) score += kingSafe;
	}

	sq = LOCATEBIT(brd->bb[Black][King]);
	if(brd->material[White] <= endMaterial) score -= KingEndTable[mirror[sq]];
	else if(!brd->bb[White][Queen] && brd->material[White] <= noQendMaterial){
		score -= KingEndTable[mirror[sq]];
	}
	else{
		score -= KingTable[mirror[sq]];
		if((brd->bb[Black][King] >> 7) & brd->all[Black]) score -= kingSafe;
		if((brd->bb[Black][King] >> 8) & brd->all[Black]) score -= kingSafe;
		if((brd->bb[Black][King] >> 9) & brd->all[Black]) score -= kingSafe;
	}

	if(brd->side == Black) {
		score = -score;	// if it is black to move negate the score
	}

	// if we are doing worse than the opponent but we could force a draw, then return zero
	// this is not the same as the MaterialDraw() check as this can be applied when we have a pawn
	if(score < 0 && !brd->bb[brd->side^1][Pawn] && !brd->bb[brd->side^1][Queen] &&
			!brd->bb[brd->side^1][Rook])
	{
		if(CountBits(brd->bb[brd->side^1][Knight]&brd->bb[brd->side^1][Bishop]) < 2)
			return 0;
		if(!brd->bb[brd->side^1][Bishop] && CountBits(brd->bb[brd->side^1][Knight]) < 3)
			return 0;
	}

	return score;
}

