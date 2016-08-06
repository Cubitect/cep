#ifndef DEFS_H
#define DEFS_H

#ifndef _STDIO_H
#include <stdio.h>
#endif
#ifndef _STDLIB_H
#include <stdlib.h>
#endif
#ifndef _UNISTD_H
#include <unistd.h>
#endif
#ifndef _STRING_H
#include <string.h>
#endif
#ifndef _STDINT_H
#include <stdint.h>
#endif

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#endif

//#define DEBUG

#define PROJECTNAME "cep-alpha 1.0.1"


#define INFINITE (0x7fffffff)

#define LONGTIME (1<<25)
#define TIME_MULTIPLIER 7
#define TIME_FRACTION   0.35

#define MATE (10000)

#define START_FEN  "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"

#define MAXGAMEMOVES 1024
#define MAXPOSMOVES  256
#define MAXDEPTH     80

#define NO_A_FILE  (0xfefefefefefefefeULL)
#define NO_H_FILE  (0x7f7f7f7f7f7f7f7fULL)
#define NO_AB_FILE (0xfcfcfcfcfcfcfcfcULL)
#define NO_GH_FILE (0x3f3f3f3f3f3f3f3fULL)

#define NO_MOVE 0

/***************
 *   Macros    *
 ***************/

// I copied the ASSERT() macro from the 'vice' chess engine by BlueFeverSoft
// It prints an error message if the argument condition is false and DEBUG is defined

#ifndef DEBUG
#define ASSERT(n)
#else
#define ASSERT(n) \
		if(!(n)) { \
			printf("\n%s - Failed\n",#n); \
			printf("On %s \n",__DATE__); \
			printf("At %s \n",__TIME__); \
			printf("In File %s \n",__FILE__); \
			printf("At Line %d\n",__LINE__); \
			exit(1); }
#endif

#define FILE(sq) ((sq)&0x7)
#define RANK(sq) ((sq)>>3)

// Makes a 64 bit random number as rand() may return only 15 bit numbers
#define Rand64() ((U64)( (U64)rand() ^ ((U64)rand()<<15) ^\
				  ((U64)rand()<<30) ^ ((U64)rand()<<45) ^\
				  (((U64)rand()&0xf)<<60) ) )

#define SETBIT(bb,sq) ( (bb) |= SetMask[(sq)] )
#define CLRBIT(bb,sq) ( (bb) &= ClrMask[(sq)] )
#define TESTBIT(bb,sq) ( (bb) & SetMask[(sq)] )

#define LOCATEBIT(bb) ((bb) ? (__builtin_ctzll((bb))) : (63))

// macros to retrieve information from a move which is stored in an integer
#define FROM(m)    ((m) & 0xff)
#define TO(m)      (((m)>>8) & 0xff)
#define CAPPCE(m)  (((m)>>16) & 0xf)
#define PROMPCE(m) (((m)>>20) & 0xf)
#define MOVEPCE(m) (((m)>>27) & 0xf)

// Some flags to categorise move types
// eg. if (move & FLAGCAP) is non-zero then move is a capture (or en passant)
#define FLAGPP   (0x01000000) /* Pawn Push */
#define FLAGEP   (0x02000000) /* En Passant */
#define FLAGCA   (0x04000000) /* Castling */
#define FLAGCAP  (0x020f0000) /* Capture */
#define FLAGPROM (0x00f00000) /* Promotion */

// generates a move integer from the arguments
#define MOVE(from,to,cap,prom,pce,flag) ((from)|((to)<<8)|((cap)<<16)|((prom)<<20)|((pce)<<27)|(flag))

#ifndef __cplusplus
enum { false, true };
#endif

// sets the algebraic notation for the squares to their corresponding bit positions
enum {
	A1, B1, C1, D1, E1, F1, G1, H1,
	A2, B2, C2, D2, E2, F2, G2, H2,
	A3, B3, C3, D3, E3, F3, G3, H3,
	A4, B4, C4, D4, E4, F4, G4, H4,
	A5, B5, C5, D5, E5, F5, G5, H5,
	A6, B6, C6, D6, E6, F6, G6, H6,
	A7, B7, C7, D7, E7, F7, G7, H7,
	A8, B8, C8, D8, E8, F8, G8, H8, NoSq
};

enum { White, Black, Both };
enum { Empty, Pawn, Knight, Bishop, Rook, Queen, King };
// Flags for a bit field representation of the castling permissions
enum { WKCA = 1, WQCA = 2, BKCA = 4, BQCA = 8 };

typedef uint64_t U64;

// structure for an element in the move list
// the score is simply a number assigned to the move and used for move ordering
// the move with the highest score is searched first
typedef struct {
	int move;
	int score;
} move_t;

// the move list structure is filled by the move generation and holds a list with all the 
// pseudo-legal moves (i.e. legal dissregarding check) that can be made in a position
typedef struct {
	move_t move[MAXPOSMOVES];
	int len;
} mlist_t;

// category flags for the transposition table to store the type of cutoff:
// alpha cutoff, beta cutoff or in between (exact)
enum { HFNONE, HFEXACT, HFALPHA, HFBETA };

// structure for an entry in the transposition table
typedef struct {
	U64 hash;	// the hash key of the position
	int move;	// the best move found for the position
	int depth;	// the depth accuracy of the score
	int score;	// how well the position was rated last time
	int flags;	// the type of cutoff
} hashentry_t;

// structure to hold a large table of transposition entries and its size (in elements)
// the table is dynamically allocated to make potential size changes possible
typedef struct {
	hashentry_t *pTable;
	int len;
} pvtable_t;

// the information to take back a move
typedef struct{
	int move;
	int enPas;
	int castle;
	int fifty;
	U64 hash;
} undo_t;

// this structure holds all the information to represent the board
typedef struct {
	// The bit boards that hold the position of all the pieces
	// indexed by color (white, black and both) and piece type
	// each bit board represents the set of positions for all pieces of that type and color
	// e.g. if there is a white pawn on e4 then the corresponding (28th) bit will be set
	// in the bit board 'bb[White][Pawn]' as well as in 'bb[Both][Pawn]'
	U64 bb[3][7];
	// the collective positions of all pieces indexed by colour
	U64 all[3];
	
	int side;		// side to move
	
	int enPas;		// en passant square
	int castle;		// castle permissions
	int fifty;		// number of half moves since the last capture or pawn move
	
	U64 hash;		// a (hopefully) unique number generated from the position
	
	int ply;		// number of half moves since the start of the search tree
	int hisPly;		// half moves since the start of the game
	
	int material[2];		// a material record for both colours to speed up the evaluation

	undo_t history[MAXGAMEMOVES];	// a record to take back any move made
	pvtable_t pv;			// the principle variation in form of a transposition table
	int pvLine[MAXDEPTH];	// the actual sequence of best moves found (extracted from the pv)

	// a record for some of the beta cutoff moves (killers), up to two for each depth
	// these are searched before normal moves to increase the efficiency of alpha beta pruning
	int betaMoves[2][MAXDEPTH];

} board_t;

// some information that is passed on inside the search
typedef struct {
	int startTime;	// search start time (in milliseconds)
	int stopTime;	// when the search has to stop
	int time;		// time allowed for search (usually stopTime - startTime)
	int toDepth;	// maximum search depth

	U64 nodes;		// a count of nodes searched
	int quit;		// if non-zero then the program should exit
	int stop;		// if non-zero then stop searching
	int infinite;	// is time control in effect

	float fh, fhf;	// some values to determine the efficiency of the move ordering
} searchinfo_t;

/***************
 *   Globals   *
 ***************/

// bitboard.c
extern U64 SetMask[64];
extern U64 ClrMask[64];

extern U64 FileMask[8];
extern U64 RankMask[8];

extern U64 IsoMask[64];
extern U64 PassedMask[2][64];
extern U64 OutpostMask[2][64];

// hash.c
extern U64 pceHash[2][7][64];
extern U64 epHash[65];
extern U64 caHash[16];
extern U64 sideHash;

// board.c
extern char pceChar[2][8];

// eval.c
extern const int pceMat[7];

// search.c
extern int capScore[7][7];

// misc.c
extern char intStr[128];

// pv.c
extern int pvSize;

/***************
 *  Functions  *
 ***************/

/* init.c */
extern void InitAll (board_t *brd);

/* bitboard.c */
extern void PrintBits( U64 b );
extern int PopBit(U64 *bb);
extern int CountBits(U64 b);
extern int GetPiece(U64 *bb, int sq);

/* board.c */
extern void ClrBoard(board_t *brd);
extern int ParseFen(board_t *brd, char *fen);
extern int CheckBrd(const board_t *brd);

/* hash.c */
extern U64 GenHash(const board_t *brd);

/* attack.c */
extern int SqAttacked(const board_t *brd, int sq, int side);
extern int CanKnightMove(board_t *brd, U64 pawnAtt, int sq, int side);
extern int CanBishopMove(board_t *brd, int sq, int side);

/* movegen.c */
extern void GenMoves(board_t *brd, mlist_t *list);
extern void GenCaps(board_t *brd, mlist_t *list);

/* makemove.c */
extern int MakeMove(board_t *brd, int move);
extern void TakeBack(board_t *brd);
extern int MoveExists(board_t *brd, int move);
extern void MakeMoveNull(board_t *brd);
extern void TakeBackNull(board_t *brd);

/* io.c */
extern void PrintBrd(board_t *brd);
extern int ParseMove(char *str, board_t *brd);
extern char *StrMove(int m);
extern void PrMoveList(mlist_t *list);

/* pv.c */
extern void ClrPv(pvtable_t *pTable);
extern void InitPv(pvtable_t *pTable);
extern void StorePvMove(board_t *brd, int move, int depth, int score, int flags);
extern int TestBrdPv(board_t *brd);
extern int TestHashTable(board_t *brd, int *move, int *score, int alpha, int beta, int depth);
extern int GetPvLine(board_t *brd, int depth);

/* search.c */
extern int IterSearch(board_t *brd, searchinfo_t *sinfo, int xboard);
extern void InitCapScores();

/* eval.c */
extern int Eval(board_t *brd);

/* misc.c */
extern int GetTime(void);
extern void CheckInput(searchinfo_t *sinfo);

/* xboard.c */
extern char *StrXmove(int m);
extern int CheckResult(board_t *brd);
extern void XboardLoop(board_t *brd, searchinfo_t *sinfo);


#endif

