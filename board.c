// board.c

#include "defs.h"

char pceChar[2][8] = { ".PNBRQK", ".pnbrqk" };

void ClrBoard(board_t *brd)
{
	int i;
	memset (brd->bb, 0, sizeof(brd->bb));
	memset (brd->all, 0, sizeof(brd->all));
	memset (brd->material, 0, sizeof(brd->material));
	brd->castle = 0x0;
	brd->enPas = NoSq;
	brd->fifty = 0;
	brd->hisPly = 0;
	brd->ply = 0;
	brd->side = White;

	for(i = 0; i < MAXGAMEMOVES; i++){
		brd->history[i].castle = 0;
		brd->history[i].enPas = NoSq;
		brd->history[i].fifty = 0;
		brd->history[i].hash = 0ULL;
		brd->history[i].move = 0;
	}
	brd->hash = GenHash(brd);
}

void CpyBrd (board_t *to, board_t *from)
{
	int i;
	memcpy(to->bb, from->bb, sizeof(to->bb));
	memcpy(to->all, from->all, sizeof(to->all));
	memcpy(to->material, from->material, sizeof(to->material));
	to->castle = from->castle;
	to->enPas = from->enPas;
	to->fifty = from->fifty;
	to->hisPly = from->hisPly;
	to->ply = from->ply;
	to->side = from->side;
	for(i = 0; i < MAXGAMEMOVES; i++){
		to->history[i].castle = from->history[i].castle;
		to->history[i].enPas = from->history[i].enPas;
		to->history[i].fifty = from->history[i].fifty;
		to->history[i].hash = from->history[i].hash;
		to->history[i].move = from->history[i].move;
	}
	to->hash = from->hash;
}

// parses a string in FEN notation and sets up a board accordingly
// it returns true if it was successful or false and does nothing
// if it could not interpret the string
int ParseFen(board_t *brd, char *fen)
{
	ASSERT(brd != NULL);
	ASSERT(fen != NULL);

	int sq, color, pce;
	board_t pos;
	char sColor[2], sCastle[6], sEnPas[4];


	ClrBoard(&pos);

	for(sq = 56; sq >= 0 && *fen; sq++, fen++){
		switch(*fen){
		case 'p': SETBIT(pos.bb[Black][Pawn],  sq); pos.material[Black] += pceMat[Pawn]; break;
		case 'n': SETBIT(pos.bb[Black][Knight],sq); pos.material[Black] += pceMat[Knight]; break;
		case 'b': SETBIT(pos.bb[Black][Bishop],sq); pos.material[Black] += pceMat[Bishop]; break;
		case 'r': SETBIT(pos.bb[Black][Rook],  sq); pos.material[Black] += pceMat[Rook]; break;
		case 'q': SETBIT(pos.bb[Black][Queen], sq); pos.material[Black] += pceMat[Queen]; break;
		case 'k': SETBIT(pos.bb[Black][King],  sq); pos.material[Black] += pceMat[King]; break;
		case 'P': SETBIT(pos.bb[White][Pawn],  sq); pos.material[White] += pceMat[Pawn]; break;
		case 'N': SETBIT(pos.bb[White][Knight],sq); pos.material[White] += pceMat[Knight]; break;
		case 'B': SETBIT(pos.bb[White][Bishop],sq); pos.material[White] += pceMat[Bishop]; break;
		case 'R': SETBIT(pos.bb[White][Rook],  sq); pos.material[White] += pceMat[Rook]; break;
		case 'Q': SETBIT(pos.bb[White][Queen], sq); pos.material[White] += pceMat[Queen]; break;
		case 'K': SETBIT(pos.bb[White][King],  sq); pos.material[White] += pceMat[King]; break;

		case ' ':
		case '/':
			sq-=17;
			break;
		default:
			if(*fen >= '1' && *fen <= '8'){
				sq += *fen - '1';
			}
			else {
				printf("\nFen Error!\nCharacter '%c'\nSquare %d\n", *fen, sq);
				return -1;
			}
		}
	}
	// "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
	sscanf(fen, "%s%s%s%d%d", sColor, sCastle, sEnPas, &pos.fifty, &pos.hisPly);

	if(*sColor == 'w') pos.side = White;
	else pos.side = Black;

	if(strchr(sCastle, 'K')) pos.castle |= WKCA;
	if(strchr(sCastle, 'Q')) pos.castle |= WQCA;
	if(strchr(sCastle, 'k')) pos.castle |= BKCA;
	if(strchr(sCastle, 'q')) pos.castle |= BQCA;

	if(*sEnPas == '-') pos.enPas = NoSq;
	else{
		pos.enPas = sEnPas[0]-'a'+((sEnPas[1]-'1') << 3);
	}

	pos.hisPly = 2*pos.hisPly + ((pos.side==Black)? 1 : 0 );
	
	for(color = White; color <= Black; color++){
		for(pce = Pawn; pce <= King; pce++){
			pos.all[color] |= pos.bb[color][pce];
			pos.all[Both] |= pos.bb[color][pce];
		}
	}
	
	for(pce = Pawn; pce <= King; pce++){
		pos.bb[Both][pce] = pos.bb[White][pce] | pos.bb[Black][pce];
	}

	pos.hash = GenHash(&pos);

	CpyBrd(brd, &pos);
	ClrPv(&brd->pv);
	return 0;
}

// A pure debugging function that checks that all the bit boards in 'brd' agree with each other
// and that the material scores are still correct
int CheckBrd(const board_t *brd)
{
	int pce;
	int mat[2] = {0, 0};
	U64 all[2] = {0ULL, 0ULL};

	for(pce = Pawn; pce <= King; pce++){
		ASSERT((brd->bb[White][pce]|brd->bb[Black][pce]) == brd->bb[Both][pce]);
		all[White] |= brd->bb[White][pce];
		all[Black] |= brd->bb[Black][pce];
		mat[White] += CountBits(brd->bb[White][pce]) * pceMat[pce];
		mat[Black] += CountBits(brd->bb[Black][pce]) * pceMat[pce];
	}
	ASSERT(all[White]==brd->all[White]);
	ASSERT(all[Black]==brd->all[Black]);

	ASSERT(brd->material[White]==mat[White]);
	ASSERT(brd->material[Black]==mat[Black]);

	return true;
}

