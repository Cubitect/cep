// movegen.c

#include "defs.h"

/* A move is stored inside a 4 byte integer
 *
 * MOVE layout:
 * 
 * 0000 0000  0000 0000  0000 0000  1111 1111 : From 
 * 0000 0000  0000 0000  1111 1111  0000 0000 : To
 * 0000 0000  0000 1111  0000 0000  0000 0000 : Captured Piece
 * 0000 0000  1111 0000  0000 0000  0000 0000 : Promoted Piece
 * 0000 0001  0000 0000  0000 0000  0000 0000 : Pawn Push
 * 0000 0010  0000 0000  0000 0000  0000 0000 : En Passant
 * 0000 0100  0000 0000  0000 0000  0000 0000 : Castle
 * 0111 1000  0000 0000  0000 0000  0000 0000 : Moving Piece
 * 

#define FROM(m)    ((m) & 0xff)
#define TO(m)      (((m)>>8) & 0xff)
#define CAPPCE(m)  (((m)>>16) & 0xf)
#define PROMPCE(m) (((m)>>20) & 0xf)
#define MOVEPCE(m) (((m)>>27) & 0xf)

#define FLAGPP   (0x01000000)
#define FLAGEP   (0x02000000)
#define FLAGCA   (0x04000000)
#define FLAGCAP  (0x020f0000)
#define FLAGPROM (0x00f00000)

#define MOVE(from,to,cap,prom,pce,flag) ((from)|((to)<<8)|((cap)<<16)|((prom)<<20)|((pce)<<27)|(flag))
*/

#define PROMSCORE 999800

// A mask for all the squares where an en passant square can legally be
#define LEGAL_EP (0x0000ff0000ff0000)

// Some masks to check for the presence of any piece between the king and rook for castling
#define ON_WKSIDE (SetMask[F1]|SetMask[G1])
#define ON_WQSIDE (SetMask[B1]|SetMask[C1]|SetMask[D1])
#define ON_BKSIDE (SetMask[F8]|SetMask[G8])
#define ON_BQSIDE (SetMask[B8]|SetMask[C8]|SetMask[D8])

// The AddMove() function family adds a move to the move list
// Each function deals with the different scores used to sort the moves
// according to their type (eg. captures, promotions etc.)
static void AddMove(board_t *brd, mlist_t *list, int move){
	list->move[list->len].move = move;

	if(brd->betaMoves[0][brd->ply] == move)
		list->move[list->len].score = 800000;
	else if(brd->betaMoves[1][brd->ply] == move)
		list->move[list->len].score = 750000;
	else
		list->move[list->len].score = 0;

	list->len++;
}

static void AddMoveScore(board_t *brd, mlist_t *list, int move, int score){
	list->move[list->len].move = move;
	list->move[list->len].score = score;
	list->len++;
}

static void AddMoveCap(board_t *brd, mlist_t *list, int move, int att){
	list->move[list->len].move = move;
	list->move[list->len].score = capScore[CAPPCE(move)][att] + 1000000;
	list->len++;
}

// The pawn moves are treated specially by these in-between functions to check for promotions
static void AddWPawnMove(board_t *brd, mlist_t *list, int from, int to, int flags){
	if(from >= A7){
		AddMoveScore(brd,list,MOVE(from,to,Empty,Queen,Pawn,flags),PROMSCORE+pceMat[Queen]);
		AddMoveScore(brd,list,MOVE(from,to,Empty,Knight,Pawn,flags),PROMSCORE+pceMat[Knight]);
		AddMoveScore(brd,list,MOVE(from,to,Empty,Rook,Pawn,flags),PROMSCORE+pceMat[Rook]);
		AddMoveScore(brd,list,MOVE(from,to,Empty,Bishop,Pawn,flags),PROMSCORE+pceMat[Bishop]);
	}
	else AddMove(brd, list, MOVE(from,to,Empty,Empty,Pawn,flags));
}

static void AddWPawnCap(board_t *brd, mlist_t *list, int from, int to, int cap, int flags){
	if(from >= A7){
		AddMoveCap(brd, list, MOVE(from,to,cap,Queen,Pawn,flags), Pawn);
		AddMoveCap(brd, list, MOVE(from,to,cap,Knight,Pawn,flags), Pawn);
		AddMoveCap(brd, list, MOVE(from,to,cap,Rook,Pawn,flags), Pawn);
		AddMoveCap(brd, list, MOVE(from,to,cap,Bishop,Pawn,flags), Pawn);
	}
	else AddMoveCap(brd, list, MOVE(from,to,cap,Empty,Pawn,flags), Pawn);
}

static void AddBPawnMove(board_t *brd, mlist_t *list, int from, int to, int flags){
	if(from <= H2){
		AddMoveScore(brd,list,MOVE(from,to,Empty,Queen,Pawn,flags),PROMSCORE+pceMat[Queen]);
		AddMoveScore(brd,list,MOVE(from,to,Empty,Knight,Pawn,flags),PROMSCORE+pceMat[Knight]);
		AddMoveScore(brd,list,MOVE(from,to,Empty,Rook,Pawn,flags),PROMSCORE+pceMat[Rook]);
		AddMoveScore(brd,list,MOVE(from,to,Empty,Bishop,Pawn,flags),PROMSCORE+pceMat[Bishop]);
	}
	else AddMove(brd, list, MOVE(from,to,Empty,Empty,Pawn,flags));
}

static void AddBPawnCap(board_t *brd, mlist_t *list, int from, int to, int cap, int flags){
	if(from <= H2){
		AddMoveCap(brd, list, MOVE(from,to,cap,Queen,Pawn,flags), Pawn);
		AddMoveCap(brd, list, MOVE(from,to,cap,Knight,Pawn,flags), Pawn);
		AddMoveCap(brd, list, MOVE(from,to,cap,Rook,Pawn,flags), Pawn);
		AddMoveCap(brd, list, MOVE(from,to,cap,Bishop,Pawn,flags), Pawn);
	}
	else AddMoveCap(brd, list, MOVE(from,to,cap,Empty,Pawn,flags), Pawn);
}

// a small function to determine the piece that occupies a certain square
// (the square is given as a bit board with its position set to 1)
int GetPieceSqb(U64 *bb, U64 sqb)
{
	if(bb[Pawn]&sqb) return Pawn;
	if(bb[Rook]&sqb) return Rook;
	if(bb[Bishop]&sqb) return Bishop;
	if(bb[Knight]&sqb) return Knight;
	if(bb[King]&sqb) return King;
	if(bb[Queen]&sqb) return Queen;
	return Empty;
}

// The move generation function fills the move list 'list' with all
// pseudo-legal moves that can be made on the board 'brd'
void GenMoves(board_t *brd, mlist_t *list)
{
	int sq, dir, pce;
	U64 sqb, b, allBoth, allOpp;	// some bit boards

	list->len = 0;
	allBoth = brd->all[Both];		// The set of all pieces
	allOpp = brd->all[brd->side^1];	// The set of all opponent's pieces

	// Non-sliders (Knights)

	b = brd->bb[brd->side][Knight];	// copy the set of positions of our knights
	while(b){
		// Loop through all of the positions by removing the last one each time
		// until there are no more knights
		sq = PopBit(&b);		// get the current knight's position
		for(dir = 0; dir < 8; dir++){
			sqb = 1ULL << sq;	// sq in 64 bit representation

			// Move the knight once in each of the 8 directions by bit shifting.
			// the AND NO_X_FILE prevents the piece to reappear on the other side of the board
			// when it stands on the edge and would otherwise fall off
			switch(dir){
			case 0: sqb = (sqb<< 6) & NO_GH_FILE; break;
			case 1: sqb = (sqb<<15) & NO_H_FILE;  break;
			case 2: sqb = (sqb<<17) & NO_A_FILE;  break;
			case 3: sqb = (sqb<<10) & NO_AB_FILE; break;
			case 4: sqb = (sqb>> 6) & NO_AB_FILE; break;
			case 5: sqb = (sqb>>15) & NO_A_FILE;  break;
			case 6: sqb = (sqb>>17) & NO_H_FILE;  break;
			case 7: sqb = (sqb>>10) & NO_GH_FILE; break;
			}

			if(allBoth & sqb){		// If there is a piece on the new square
				if(allOpp & sqb)	// and it was an opponent's piece,
					// then add the corresponding capture move 
					AddMoveCap(brd, list,MOVE(sq,LOCATEBIT(sqb),
							GetPieceSqb(brd->bb[Both],sqb),Empty,Knight,0), Knight);
			} else if(sqb){
				// otherwise add a normal move
				AddMove(brd, list,MOVE(sq,LOCATEBIT(sqb),Empty,Empty,Knight,0));
			}
		}
	}

	// Diagonal sliding movements

	// This outer for loop executes the code once for bishops and queens only
	for(pce = Bishop;; pce = Queen){
		for(dir = 0; dir < 4; dir++){
			b = brd->bb[brd->side][pce];	// Copy the position set of the current piece
			while(b){
				sq = LOCATEBIT(b);	// Get the position of the last piece
				sqb = SetMask[sq];	// sq in 64 bit board representation
				b &= (b-1);			// Remove the last piece/bit

				for(;;){
					// Like the knight move in all of the 4 different directions
					// But this time repetitively for the sliding movement
					switch(dir){
					case 0: sqb = (sqb<<7) & NO_H_FILE; break;
					case 1: sqb = (sqb<<9) & NO_A_FILE; break;
					case 2: sqb = (sqb>>7) & NO_A_FILE; break;
					case 3: sqb = (sqb>>9) & NO_H_FILE; break;
					}
					if(sqb & allBoth){		// if the piece hit something
						if(sqb & allOpp)	// if it was an opponent's piece
							// Capture it
							AddMoveCap(brd, list,MOVE(sq,LOCATEBIT(sqb),
									GetPieceSqb(brd->bb[Both],sqb),Empty,pce,0), pce);
						break; // and stop as you cannot move further in the current direction
					}
					else if(sqb) AddMove(brd, list,MOVE(sq,LOCATEBIT(sqb),Empty,Empty,pce,0));
					else break; // stop as the piece fell off the board
				}
			}
		}
		if(pce == Queen) break;
	}

	// Rectangular sliding movements
	// The same as for diagonal sliders but rectangular (rooks and queens)
	for(pce = Rook;; pce = Queen){
		for(dir = 0; dir < 4; dir++){
			b = brd->bb[brd->side][pce];
			while(b){
				sq = LOCATEBIT(b);
				sqb = SetMask[sq];
				b &= (b-1);

				for(;;){
					switch(dir){
					case 0: sqb = (sqb<<1) & NO_A_FILE; break;
					case 1: sqb = (sqb<<8); break;
					case 2: sqb = (sqb>>1) & NO_H_FILE; break;
					case 3: sqb = (sqb>>8); break;
					}
					if(sqb & allBoth){
						if(sqb & allOpp)
							AddMoveCap(brd, list,MOVE(sq,LOCATEBIT(sqb),
									GetPieceSqb(brd->bb[Both],sqb),Empty,pce,0), pce);
						break;
					}
					else if(sqb) AddMove(brd, list,MOVE(sq,LOCATEBIT(sqb),Empty,Empty,pce,0));
					else break;
				}
			}
		}
		if(pce == Queen) break;
	}

	// Pawns

	if(brd->side == White){
		b = brd->bb[White][Pawn];
		while(b){
			sq = LOCATEBIT(b); // square number of last white pawn
			sqb = SetMask[sq]; // sq in 64 bit board representation
			b &= (b-1);        // clear the last set bit

			if((sqb<<8)&(~allBoth)){ // Square in front is empty
				AddWPawnMove(brd, list,sq,sq+8,0);
				// If the pawn has not moved and the two squares in front are empty
				// then add a pawn push move
				if(RANK(sq)==1 && (sqb<<16)&(~allBoth))
					AddWPawnMove(brd, list,sq,sq+16,FLAGPP);
			}
			// Diagonal captures and en passant captures
			if((sqb<<7) & NO_H_FILE & allOpp)
				AddWPawnCap(brd, list,sq,sq+7,GetPieceSqb(brd->bb[Both],sqb<<7),0);
			if((sqb<<9) & NO_A_FILE & allOpp)
				AddWPawnCap(brd, list,sq,sq+9,GetPieceSqb(brd->bb[Both],sqb<<9),0);
			if((sqb<<7) & NO_H_FILE & LEGAL_EP & (1ULL<<brd->enPas))
				AddWPawnCap(brd, list,sq,sq+7,Pawn,FLAGEP);
			if((sqb<<9) & NO_A_FILE & LEGAL_EP & (1ULL<<brd->enPas))
				AddWPawnCap(brd, list,sq,sq+9,Pawn,FLAGEP);
		}
	}
	else {	// The same as for white, but moving in the opposite direction
		b = brd->bb[Black][Pawn];
		while(b){
			sq = LOCATEBIT(b); // square number of last black pawn
			sqb = SetMask[sq]; // sq in 64 bit board representation
			b &= (b-1);        // clear the last set bit

			if((sqb>>8)&(~allBoth)){ // Square in front is empty
				AddBPawnMove(brd, list,sq,sq-8,0);
				if(RANK(sq)==6 && (sqb>>16)&(~allBoth))
					AddBPawnMove(brd, list,sq,sq-16,FLAGPP);
			}

			if((sqb>>7) & NO_A_FILE & allOpp)
				AddBPawnCap(brd, list,sq,sq-7,GetPieceSqb(brd->bb[Both],sqb>>7),0);
			if((sqb>>9) & NO_H_FILE & allOpp)
				AddBPawnCap(brd, list,sq,sq-9,GetPieceSqb(brd->bb[Both],sqb>>9),0);
			if((sqb>>7) & NO_A_FILE & LEGAL_EP & (1ULL<<brd->enPas))
				AddBPawnCap(brd, list,sq,sq-7,Pawn,FLAGEP);
			if((sqb>>9) & NO_H_FILE & LEGAL_EP & (1ULL<<brd->enPas))
				AddBPawnCap(brd, list,sq,sq-9,Pawn,FLAGEP);
		}
	}

	b = brd->bb[brd->side][King];	// copy the king position bitboard
	sq = LOCATEBIT(b);
	// no piece loop here as there cannot be more than one king for each side

	// Castling

	if(brd->side == White){
		if(brd->castle & WKCA){	// Castle permission is still there
			if(!(brd->all[Both] & ON_WKSIDE) &&			// there is no piece between king and rook
					!SqAttacked(brd,E1,brd->side^1) &&	// none of the squares are under attack
					!SqAttacked(brd,F1,brd->side^1) &&
					!SqAttacked(brd,G1,brd->side^1))
				AddMove(brd, list,MOVE(E1,G1,Empty,Empty,King,FLAGCA)); // Add the castle move
		}
		if(brd->castle & WQCA){ // the same for the queen side
			if(!(brd->all[Both] & ON_WQSIDE) &&
					!SqAttacked(brd,E1,brd->side^1) &&
					!SqAttacked(brd,D1,brd->side^1) &&
					!SqAttacked(brd,C1,brd->side^1))
				AddMove(brd, list,MOVE(E1,C1,Empty,Empty,King,FLAGCA));
		}
	}
	else {	//The same as for white but on the other side
		if(brd->castle & BKCA){
			if(!(brd->all[Both] & ON_BKSIDE) &&
					!SqAttacked(brd,E8,brd->side^1) &&
					!SqAttacked(brd,F8,brd->side^1) &&
					!SqAttacked(brd,G8,brd->side^1))
				AddMove(brd, list,MOVE(E8,G8,Empty,Empty,King,FLAGCA));
		}
		if(brd->castle & BQCA){
			if(!(brd->all[Both] & ON_BQSIDE) &&
					!SqAttacked(brd,E8,brd->side^1) &&
					!SqAttacked(brd,D8,brd->side^1) &&
					!SqAttacked(brd,C8,brd->side^1))
				AddMove(brd, list,MOVE(E8,C8,Empty,Empty,King,FLAGCA));
		}
	}

	// Normal king moves

	for(dir = 0; dir < 8; dir++){
		sqb = b;	// for all 8 directions copy the king position

		switch(dir){	// and move it
		case 0: sqb = (sqb<<7) & NO_H_FILE; break;
		case 1: sqb = (sqb<<9) & NO_A_FILE; break;
		case 2: sqb = (sqb>>7) & NO_A_FILE; break;
		case 3: sqb = (sqb>>9) & NO_H_FILE; break;
		case 4: sqb = (sqb<<1) & NO_A_FILE; break;
		case 5: sqb = (sqb<<8); break;
		case 6: sqb = (sqb>>1) & NO_H_FILE; break;
		case 7: sqb = (sqb>>8); break;
		}
		if(allBoth & sqb){
			if(allOpp & sqb)
				AddMoveCap(brd, list,MOVE(sq,LOCATEBIT(sqb),
						GetPieceSqb(brd->bb[Both],sqb),Empty,King,0),King);
		} else if(sqb){
			AddMove(brd, list,MOVE(sq,LOCATEBIT(sqb),Empty,Empty,King,0));
		}
	}
}

// This function does the exact same as GenMoves() but generates only capture and promotion moves
void GenCaps(board_t *brd, mlist_t *list)
{
	int sq, dir, pce;
	U64 sqb, b, allBoth, allOpp;

	list->len = 0;
	allBoth = brd->all[Both];
	allOpp = brd->all[brd->side^1];

	// Non-sliders (Knights)

	b = brd->bb[brd->side][Knight];
	while(b){
		sq = PopBit(&b);
		for(dir = 0; dir < 8; dir++){
			sqb = 1ULL << sq;

			switch(dir){
			case 0: sqb = (sqb<< 6) & NO_GH_FILE; break;
			case 1: sqb = (sqb<<15) & NO_H_FILE;  break;
			case 2: sqb = (sqb<<17) & NO_A_FILE;  break;
			case 3: sqb = (sqb<<10) & NO_AB_FILE; break;
			case 4: sqb = (sqb>> 6) & NO_AB_FILE; break;
			case 5: sqb = (sqb>>15) & NO_A_FILE;  break;
			case 6: sqb = (sqb>>17) & NO_H_FILE;  break;
			case 7: sqb = (sqb>>10) & NO_GH_FILE; break;
			}

			if(allBoth & sqb){
				if(allOpp & sqb)
					AddMoveCap(brd, list,MOVE(sq,LOCATEBIT(sqb),
							GetPieceSqb(brd->bb[Both],sqb),Empty,Knight,0), Knight);
			}
		}
	}

	// Sliders Diagonal

	for(pce = Bishop;; pce = Queen){
		for(dir = 0; dir < 4; dir++){
			b = brd->bb[brd->side][pce];
			while(b){
				sq = LOCATEBIT(b);
				sqb = SetMask[sq];
				b &= (b-1);

				for(;;){
					switch(dir){
					case 0: sqb = (sqb<<7) & NO_H_FILE; break;
					case 1: sqb = (sqb<<9) & NO_A_FILE; break;
					case 2: sqb = (sqb>>7) & NO_A_FILE; break;
					case 3: sqb = (sqb>>9) & NO_H_FILE; break;
					}
					if(sqb & allBoth){
						if(sqb & allOpp)
							AddMoveCap(brd, list,MOVE(sq,LOCATEBIT(sqb),
									GetPieceSqb(brd->bb[Both],sqb),Empty,pce,0), pce);
						break;
					}
					else if(!sqb) break;
				}
			}
		}
		if(pce == Queen) break;
	}

	// Sliders Rectangular

	for(pce = Rook;; pce = Queen){
		for(dir = 0; dir < 4; dir++){
			b = brd->bb[brd->side][pce];
			while(b){
				sq = LOCATEBIT(b);
				sqb = SetMask[sq];
				b &= (b-1);

				for(;;){
					switch(dir){
					case 0: sqb = (sqb<<1) & NO_A_FILE; break;
					case 1: sqb = (sqb<<8); break;
					case 2: sqb = (sqb>>1) & NO_H_FILE; break;
					case 3: sqb = (sqb>>8); break;
					}
					if(sqb & allBoth){
						if(sqb & allOpp)
							AddMoveCap(brd, list,MOVE(sq,LOCATEBIT(sqb),
									GetPieceSqb(brd->bb[Both],sqb),Empty,pce,0), pce);
						break;
					}
					else if(!sqb) break;
				}
			}
		}
		if(pce == Queen) break;
	}

	// Pawns

	if(brd->side == White){
		b = brd->bb[White][Pawn];
		while(b){
			sq = LOCATEBIT(b);
			sqb = SetMask[sq];
			b &= (b-1);

			// I decided to include pawn moves that are one move before
			// promotion in the quiescence search
			if(sq >= A6 && (sqb<<8)&(~allBoth)){
				AddWPawnMove(brd, list,sq,sq+8,0);
			}//*/

			if((sqb<<7) & NO_H_FILE & allOpp)
				AddWPawnCap(brd, list,sq,sq+7,GetPieceSqb(brd->bb[Both],sqb<<7),0);
			if((sqb<<9) & NO_A_FILE & allOpp)
				AddWPawnCap(brd, list,sq,sq+9,GetPieceSqb(brd->bb[Both],sqb<<9),0);
			if((sqb<<7) & NO_H_FILE & LEGAL_EP & (1ULL<<brd->enPas))
				AddWPawnCap(brd, list,sq,sq+7,Pawn,FLAGEP);
			if((sqb<<9) & NO_A_FILE & LEGAL_EP & (1ULL<<brd->enPas))
				AddWPawnCap(brd, list,sq,sq+9,Pawn,FLAGEP);
		}
	}
	else {
		b = brd->bb[Black][Pawn];
		while(b){
			sq = LOCATEBIT(b);
			sqb = SetMask[sq];
			b &= (b-1);

			if(sq <= H3 && (sqb>>8)&(~allBoth)){ // Square in front is empty
				AddBPawnMove(brd, list,sq,sq-8,0);
			}//*/

			if((sqb>>7) & NO_A_FILE & allOpp)
				AddBPawnCap(brd, list,sq,sq-7,GetPieceSqb(brd->bb[Both],sqb>>7),0);
			if((sqb>>9) & NO_H_FILE & allOpp)
				AddBPawnCap(brd, list,sq,sq-9,GetPieceSqb(brd->bb[Both],sqb>>9),0);
			if((sqb>>7) & NO_A_FILE & LEGAL_EP & (1ULL<<brd->enPas))
				AddBPawnCap(brd, list,sq,sq-7,Pawn,FLAGEP);
			if((sqb>>9) & NO_H_FILE & LEGAL_EP & (1ULL<<brd->enPas))
				AddBPawnCap(brd, list,sq,sq-9,Pawn,FLAGEP);
		}
	}


	b = brd->bb[brd->side][King];
	sq = LOCATEBIT(b);

	// King

	for(dir = 0; dir < 8; dir++){
		sqb = b;

		switch(dir){
		case 0: sqb = (sqb<<7) & NO_H_FILE; break;
		case 1: sqb = (sqb<<9) & NO_A_FILE; break;
		case 2: sqb = (sqb>>7) & NO_A_FILE; break;
		case 3: sqb = (sqb>>9) & NO_H_FILE; break;
		case 4: sqb = (sqb<<1) & NO_A_FILE; break;
		case 5: sqb = (sqb<<8); break;
		case 6: sqb = (sqb>>1) & NO_H_FILE; break;
		case 7: sqb = (sqb>>8); break;
		}
		if(allBoth & sqb){
			if(allOpp & sqb)
				AddMoveCap(brd, list,MOVE(sq,LOCATEBIT(sqb),
						GetPieceSqb(brd->bb[Both],sqb),Empty,King,0),King);
		}
	}
}



