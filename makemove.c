// makemove.c

#include "defs.h"

// a table to update castle permissions when a piece moves
const int castlePerm[64] = {
    13, 15, 15, 15, 12, 15, 15, 14,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
     7, 15, 15, 15,  3, 15, 15, 11
};

// clears a piece from the board
static void ClrPiece(board_t *brd, int side, int pce, int sq)
{
	CLRBIT(brd->bb[side][pce],sq);	// Update all the bit boards
	CLRBIT(brd->bb[Both][pce],sq);
	CLRBIT(brd->all[side],sq);
	CLRBIT(brd->all[Both],sq);

	brd->hash ^= pceHash[side][pce][sq];	// hash out the piece

	if(castlePerm[sq] != 0xf){				// update the castle permissions
		brd->hash ^= caHash[brd->castle];	// and the the hash if it changed
		brd->castle &= castlePerm[sq];
		brd->hash ^= caHash[brd->castle];
	}
}

// sets a piece on the board
static void SetPiece(board_t *brd, int side, int pce, int sq)
{
	SETBIT(brd->bb[side][pce],sq);	// Update all the bit boards
	SETBIT(brd->bb[Both][pce],sq);
	SETBIT(brd->all[side],sq);
	SETBIT(brd->all[Both],sq);

	brd->hash ^= pceHash[side][pce][sq];	// hash in the piece
}

// The same as ClrPiece() but does not update the hash
static void ClrPieceBack(board_t *brd, int side, int pce, int sq)
{
	CLRBIT(brd->bb[side][pce],sq);
	CLRBIT(brd->bb[Both][pce],sq);
	CLRBIT(brd->all[side],sq);
	CLRBIT(brd->all[Both],sq);
}

// The same as SetPiece() but does not update the hash
static void SetPieceBack(board_t *brd, int side, int pce, int sq)
{
	SETBIT(brd->bb[side][pce],sq);
	SETBIT(brd->bb[Both][pce],sq);
	SETBIT(brd->all[side],sq);
	SETBIT(brd->all[Both],sq);
}

// makes the move 'move' on the board 'brd' and returns true if it was legal
// else it returns false and does nothing
int MakeMove(board_t *brd, int move)
{
	ASSERT(CheckBrd(brd));
	
	// save the information needed to take the move back to the history
	brd->history[brd->hisPly].move = move;
	brd->history[brd->hisPly].enPas = brd->enPas;
	brd->history[brd->hisPly].castle = brd->castle;
	brd->history[brd->hisPly].fifty = brd->fifty;
	brd->history[brd->hisPly].hash = brd->hash;
	brd->hisPly++;
	brd->ply++;
	
	// if it is a castling move, then move the rook
	if(move & FLAGCA){ // Castling
		switch(TO(move)){
		case G1: // White Kingside
			ClrPiece(brd,White,Rook,H1);
			SetPiece(brd,White,Rook,F1);
			break;
		case C1: // White Queenside
			ClrPiece(brd,White,Rook,A1);
			SetPiece(brd,White,Rook,D1);
			break;
		case G8: // Black Kingside
			ClrPiece(brd,Black,Rook,H8);
			SetPiece(brd,Black,Rook,F8);
			break;
		case C8: // Black Queenside
			ClrPiece(brd,Black,Rook,A8);
			SetPiece(brd,Black,Rook,D8);
			break;
		default: ASSERT(false); break;
		}
	}

	brd->fifty++;

	if(move & FLAGCAP){	// capture move
		brd->fifty = 0;	// reset fifty move rule counter
		if(move & FLAGEP){	// deal with en passant capture
			if(brd->side == White) {
				ClrPiece(brd,Black,Pawn,brd->enPas-8);	// clear the pawn at (enPas - 8) as
				brd->material[Black] -= pceMat[Pawn];	// the captured pawn is below the 'to' square
			}
			else {
				ClrPiece(brd,White,Pawn,brd->enPas+8);
				brd->material[White] -= pceMat[Pawn];
			}
		}
		else {	// otherwise clear the captured piece
			ClrPiece(brd,brd->side^1,CAPPCE(move),TO(move));
			brd->material[brd->side^1] -= pceMat[CAPPCE(move)];	// and update the material
		}
	}

	// actually move the piece
	ClrPiece(brd, brd->side, MOVEPCE(move), FROM(move));
	SetPiece(brd, brd->side, MOVEPCE(move), TO(move));

	if(MOVEPCE(move) == Pawn) {
		if(move & FLAGPROM){	// is it a promotion?
			ClrPiece(brd,brd->side,Pawn,TO(move));				// clear the pawn and
			SetPiece(brd,brd->side,PROMPCE(move),TO(move));		// replace it with the promotion piece
			brd->material[brd->side] += pceMat[PROMPCE(move)] - pceMat[Pawn];	// update material
		}
		brd->fifty = 0;			// reset fifty move counter
	}

	// Determine the new en passant square if any
	brd->hash ^= epHash[brd->enPas];
	if(FLAGPP & move) {
		if(brd->side == White){
			brd->enPas = TO(move) - 8;
		} else{
			brd->enPas = TO(move) + 8;
		}
	} else  brd->enPas = NoSq;
	brd->hash ^= epHash[brd->enPas];

	U64 b;
	b = brd->bb[brd->side][King];

	brd->hash ^= sideHash;
	brd->side ^= 1;	// change the side to move

	// if in check undo everything
	if(SqAttacked(brd,LOCATEBIT(b),brd->side)){
		TakeBack(brd);
		return false;
	}

	return true;
}

// reverses MakeMove()
void TakeBack(board_t *brd)
{
	ASSERT(CheckBrd(brd));

	brd->hisPly--;
	brd->ply--;
	int move = brd->history[brd->hisPly].move;

	brd->side ^= 1;

	if(move & FLAGPROM){
		ClrPieceBack(brd,brd->side,PROMPCE(move),TO(move));
		brd->material[brd->side] -= pceMat[PROMPCE(move)] - pceMat[Pawn];
	}

	ClrPieceBack(brd,brd->side,MOVEPCE(move),TO(move));
	SetPieceBack(brd,brd->side,MOVEPCE(move),FROM(move));

	if (move & FLAGCA){
		switch(TO(move)){
		case G1: // White Kingside
			SetPieceBack(brd,White,Rook,H1);
			ClrPieceBack(brd,White,Rook,F1);
			break;
		case C1: // White Queenside
			SetPieceBack(brd,White,Rook,A1);
			ClrPieceBack(brd,White,Rook,D1);
			break;
		case G8: // Black Kingside
			SetPieceBack(brd,Black,Rook,H8);
			ClrPieceBack(brd,Black,Rook,F8);
			break;
		case C8: // Black Queenside
			SetPieceBack(brd,Black,Rook,A8);
			ClrPieceBack(brd,Black,Rook,D8);
			break;
		default: ASSERT(false); break;
		}
	} else 	if(move & FLAGCAP){
		if(move & FLAGEP){
			if(brd->side == White){
				SetPieceBack(brd,Black,Pawn,TO(move)-8);
				brd->material[Black] += pceMat[Pawn];
			} else{
				SetPieceBack(brd,White,Pawn,TO(move)+8);
				brd->material[White] += pceMat[Pawn];
			}
		} else{
			SetPieceBack(brd,brd->side^1,CAPPCE(move),TO(move));
			brd->material[brd->side^1] += pceMat[CAPPCE(move)];
		}
	}

	brd->castle = brd->history[brd->hisPly].castle;
	brd->fifty = brd->history[brd->hisPly].fifty;
	brd->enPas = brd->history[brd->hisPly].enPas;
	brd->hash = brd->history[brd->hisPly].hash;
}

// returns true if the move 'move' exists on the current board and is legal to make
int MoveExists(board_t *brd, int move)
{
	if(move == NO_MOVE) return false;

	mlist_t list;
	GenMoves(brd, &list);

	int i;
	for(i = 0; i < list.len; i++){	// Loop through all available moves
		if(list.move[i].move == move){	// if the moves are equal
			if(!MakeMove(brd,list.move[i].move)) continue;	// check it is legal
			TakeBack(brd);
			return true;
		}
	}
	return false;
}

// Makes a move without moving pieces (essentially switching side)
void MakeMoveNull(board_t *brd)
{
	ASSERT(CheckBrd(brd));

	brd->history[brd->hisPly].move = NO_MOVE;
	brd->history[brd->hisPly].enPas = brd->enPas;
	brd->history[brd->hisPly].castle = brd->castle;
	brd->history[brd->hisPly].fifty = brd->fifty;
	brd->history[brd->hisPly].hash = brd->hash;
	brd->hisPly++;
	brd->ply++;

	brd->fifty++;

	brd->hash ^= epHash[brd->enPas];
	brd->enPas = NoSq;
	brd->hash ^= epHash[brd->enPas];

	brd->hash ^= sideHash;
	brd->side ^= 1;
}

// Takes back a move without moving pieces (essentially switching side back)
void TakeBackNull(board_t *brd)
{
	ASSERT(CheckBrd(brd));

	brd->hisPly--;
	brd->ply--;

	brd->side ^= 1;

	brd->castle = brd->history[brd->hisPly].castle;
	brd->fifty = brd->history[brd->hisPly].fifty;
	brd->enPas = brd->history[brd->hisPly].enPas;
	brd->hash = brd->history[brd->hisPly].hash;
}


