// io.c

#include "defs.h"

// converts the string 'str' to a "from" and "to" square
// if it finds a match to a move available on the current board, it returns that move
int ParseMove(char *str, board_t *brd)
{
	mlist_t list;
	int to, from, i, prom, move;

	if(str[0] < 'a' || str[0] > 'h') return NO_MOVE;
	if(str[1] < '1' || str[1] > '8') return NO_MOVE;
	if(str[2] < 'a' || str[2] > 'h') return NO_MOVE;
	if(str[3] < '1' || str[3] > '8') return NO_MOVE;
	
	if(str[4]=='q') prom = Queen;
	else if(str[4]=='r') prom = Rook;
	else if(str[4]=='b') prom = Bishop;
	else if(str[4]=='n') prom = Knight;
	else prom = Empty;

	from = ((str[1]-'1')<<3) + (str[0]-'a');
	to   = ((str[3]-'1')<<3) + (str[2]-'a');

	GenMoves(brd, &list);
	for(i = 0; i < list.len; i++){
		move = list.move[i].move;
		if(FROM(move)==from && TO(move)==to){
			if(move | FLAGPROM){
				if(PROMPCE(move) == prom) return move;
			}
			else return move;
		}
	}
	return NO_MOVE;
}

// prints the list of moves in the move list 'list'
void PrMoveList(mlist_t *list)
{
	int i;
	for(i = 0; i < list->len; i++){
		printf("Move%3d: %s\t(score:%4d) %s%c\n",
				i+1, StrMove(list->move[i].move), list->move[i].score,
				(list->move[i].move&FLAGCAP)?"Capture: ":"",
				(list->move[i].move&FLAGCAP)? pceChar[0][CAPPCE(list->move[i].move)]:' ');
	}
}

// converts a move into a human-friendly, readable string
char *StrMove(int m)
{
	static char mvstr[8];
	if(m&FLAGCA) {
		if(FILE( TO(m) ) == 6) sprintf(mvstr, "%s", "0-0 ");
		else if(FILE( TO(m) ) == 2) sprintf(mvstr, "%s", "0-0-0");
		else{
			printf("MOVE ERROR! Incorrect castle attempt.\n");
			mvstr[0] = 0;
		}
		return mvstr;
	}
	sprintf(mvstr, "%c%c%c%c%c", pceChar[0][MOVEPCE(m)], FILE(FROM(m)) + 'a', RANK(FROM(m)) + '1',
							   FILE(TO(m)) + 'a',   RANK(TO(m)) + '1');
	if(PROMPCE(m) != Empty) 
		sprintf(mvstr, "%s%c", mvstr, pceChar[Black][PROMPCE(m)]);
	
	return mvstr;
}

// prints the board 'brd'
void PrintBrd(board_t *brd){
	int sq, pce;
	int f, r;
	printf("    A B C D E F G H\n");
	for(r = 7; r >= 0; r--){
		printf("\n%-4d", 1+r);
		for(f = 0; f < 8; f++){
			sq = (r<<3)+f;
			pce = GetPiece(brd->bb[Both], sq);
			printf("%c ", pceChar[TESTBIT(brd->bb[White][pce],sq)?0:1][pce]);
		}
	}

	printf("\n\n%s to move", (brd->side == White)?"White":"Black");

	printf("\nCastle: ");
	if(brd->castle & WKCA) printf("K"); else printf("-");
	if(brd->castle & WQCA) printf("Q"); else printf("-");
	if(brd->castle & BKCA) printf("k"); else printf("-");
	if(brd->castle & BQCA) printf("q"); else printf("-");

	printf("\nEn Passant: ");
	if(brd->enPas != NoSq) {
		printf("%c%c\n", (brd->enPas&0x7)+'a', (brd->enPas>>3)+'1');
	} else printf("-\n");

	printf("Hash: %" PRIX64 "\nFiftyMoves: %d (ply)\n\n", brd->hash, brd->fifty);
}


