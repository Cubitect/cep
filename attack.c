// attack.c

#include "defs.h"


// Finds if square 'sq' is attacked or protected by 'side'
int SqAttacked(const board_t *brd, int sq, int side)
{
	int i;
	U64 b, sqb = SetMask[sq]; // sq as 64 bit board

	// Pawns
	if(side == White){	// Is there an opponent's pawn at the squares where it could attack sq?
		if(sqb & ((brd->bb[White][Pawn]<<7) & NO_H_FILE)) return true;
		if(sqb & ((brd->bb[White][Pawn]<<9) & NO_A_FILE)) return true;
	}
	else {
		if(sqb & ((brd->bb[Black][Pawn]>>7) & NO_A_FILE)) return true;
		if(sqb & ((brd->bb[Black][Pawn]>>9) & NO_H_FILE)) return true;
	}

	// from the square sq, check the diagonals if there is an opponent's queen or bishop

	// Sliding Pieces
	for(i = 0; i < 4; i++){
		b = sqb;
		while(b){
			switch(i){
			case 0: b = (b<<7) & NO_H_FILE; break;
			case 1: b = (b<<9) & NO_A_FILE; break;
			case 2: b = (b>>7) & NO_A_FILE; break;
			case 3: b = (b>>9) & NO_H_FILE; break;
			}
			if(b & (brd->bb[side][Bishop]|brd->bb[side][Queen])) return true;
			if(b & (brd->all[Both])) break;
		}
	}
	
	// from the square sq, check the vertical and horizontal if there is an opponent's queen or rook

	for(i = 0; i < 4; i++){
		b = sqb;
		while(b){
			switch(i){
			case 0: b = (b<<1) & NO_A_FILE; break;
			case 1: b = (b<<8);             break;
			case 2: b = (b>>1) & NO_H_FILE; break;
			case 3: b = (b>>8);             break;
			}
			if(b & (brd->bb[side][Rook]|brd->bb[side][Queen])) return true;
			if(b & (brd->all[Both])) break;
		}
	}


	// Non-Sliding Pieces

	// check the squares from which a knight could attack sq
	if(brd->bb[side][Knight] & ((sqb<< 6) & NO_GH_FILE)) return true;
	if(brd->bb[side][Knight] & ((sqb<<15) & NO_H_FILE) ) return true;
	if(brd->bb[side][Knight] & ((sqb<<17) & NO_A_FILE) ) return true;
	if(brd->bb[side][Knight] & ((sqb<<10) & NO_AB_FILE)) return true;
	if(brd->bb[side][Knight] & ((sqb>> 6) & NO_AB_FILE)) return true;
	if(brd->bb[side][Knight] & ((sqb>>15) & NO_A_FILE) ) return true;
	if(brd->bb[side][Knight] & ((sqb>>17) & NO_H_FILE) ) return true;
	if(brd->bb[side][Knight] & ((sqb>>10) & NO_GH_FILE)) return true;
	
	// check the squares around sq for a king
	if(brd->bb[side][King] & ((sqb<<7) & NO_H_FILE)) return true;
	if(brd->bb[side][King] & ((sqb<<9) & NO_A_FILE)) return true;
	if(brd->bb[side][King] & ((sqb>>7) & NO_A_FILE)) return true;
	if(brd->bb[side][King] & ((sqb>>9) & NO_H_FILE)) return true;
	if(brd->bb[side][King] & ((sqb<<1) & NO_A_FILE)) return true;
	if(brd->bb[side][King] & (sqb<<8))               return true;
	if(brd->bb[side][King] & ((sqb>>1) & NO_H_FILE)) return true;
	if(brd->bb[side][King] & (sqb>>8))               return true;

	return false;
}


// returns true if a knight of colour 'side' on a square 'sq' has a square to go to
int CanKnightMove(board_t *brd, U64 pawnAtt, int sq, int side)
{
	U64 sqb = SetMask[sq];

	// check the squares which the knight can go to
	if(~brd->all[side] & ((sqb<< 6) & NO_GH_FILE & pawnAtt)) return true;
	if(~brd->all[side] & ((sqb<<15) & NO_H_FILE  & pawnAtt)) return true;
	if(~brd->all[side] & ((sqb<<17) & NO_A_FILE  & pawnAtt)) return true;
	if(~brd->all[side] & ((sqb<<10) & NO_AB_FILE & pawnAtt)) return true;
	if(~brd->all[side] & ((sqb>> 6) & NO_AB_FILE & pawnAtt)) return true;
	if(~brd->all[side] & ((sqb>>15) & NO_A_FILE  & pawnAtt)) return true;
	if(~brd->all[side] & ((sqb>>17) & NO_H_FILE  & pawnAtt)) return true;
	if(~brd->all[side] & ((sqb>>10) & NO_GH_FILE & pawnAtt)) return true;

	return false;
}

// returns 1 if a bishop of colour 'side' on a square 'sq' has a square to go to
// returns 2 if the bishop can go 2 in a direction
int CanBishopMove(board_t *brd, int sq, int side)
{
	U64 sqb = SetMask[sq];
	int ret = 0;

	// check the squares which the bishop can go to
	if(~brd->all[side] & ((sqb>>9) & NO_H_FILE)){
		if(~brd->all[side] & ((sqb>>18) & NO_GH_FILE)) return 2;
		ret = 1;
	}
	if(~brd->all[side] & ((sqb>>7) & NO_A_FILE)){
		if(~brd->all[side] & ((sqb>>14) & NO_AB_FILE)) return 2;
		ret = 1;
	}
	if(~brd->all[side] & ((sqb<<9) & NO_A_FILE)){
		if(~brd->all[side] & ((sqb<<18) & NO_AB_FILE)) return 2;
		ret = 1;
	}
	if(~brd->all[side] & ((sqb<<7) & NO_H_FILE)){
		if(~brd->all[side] & ((sqb<<14) & NO_GH_FILE)) return 2;
		ret = 1;
	}
	return ret;
}

