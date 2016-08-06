// init.c

#include "defs.h"

void InitMasks(void){
	int i;
	for(i = 0; i < 64; i++){
		SetMask[i] = 1ULL << i;
		ClrMask[i] = ~SetMask[i];
	}
}

// initialises all the variables used for generating and changing a hash key
void InitHash(void)
{
	int pce, color, sq, i;
	for(color = White; color <= Black; color++){
		for(pce = Pawn; pce <= King; pce++){
			for(sq = 0; sq < 64; sq++){
				pceHash[color][pce][sq] = Rand64();
			}
		}
	}
	for(sq = 0; sq < 65; sq++){
		epHash[sq] = Rand64();
	}
	for(i = 0; i <= 0xf; i++){
		caHash[i] = Rand64();
	}
	sideHash = Rand64();
}

// initialises the masks used in the evaluation
void InitEvalMasks(void)
{
	int i;
	memset(FileMask, 0, sizeof(FileMask));
	memset(RankMask, 0, sizeof(RankMask));
	memset(IsoMask, 0, sizeof(IsoMask));
	memset(PassedMask, 0, sizeof(PassedMask));

	for(i = 0; i < 64; i += 8){
		RankMask[i>>3] |= ((U64)0xff << i);
	}
	for(i = 0; i < 64; i++){
		FileMask[i&0x7] |= (1ULL << i);
	}
	for(i = 0; i < 64; i++){
		if((i&0x7) < 7) IsoMask[i] |= FileMask[(i&0x7)+1];
		if((i&0x7) > 0) IsoMask[i] |= FileMask[(i&0x7)-1];
	}
	for(i = 0; i < 64; i++){
		PassedMask[Black][i] |= FileMask[(i&0x7)];
		if((i&0x7) < 7) PassedMask[Black][i] |= FileMask[(i&0x7)+1];
		if((i&0x7) > 0) PassedMask[Black][i] |= FileMask[(i&0x7)-1];
		PassedMask[White][i] = PassedMask[Black][i];
		PassedMask[Black][i] &= ((1ULL<<i)-1)>>1;
		PassedMask[White][i] &= (~((1ULL<<i)-1)<<7);
	}
	for(i = 0; i < 64; i++){
		if((i&0x7) < 7) OutpostMask[Black][i] |= FileMask[(i&0x7)+1];
		if((i&0x7) > 0) OutpostMask[Black][i] |= FileMask[(i&0x7)-1];
		OutpostMask[White][i] = OutpostMask[Black][i];
		OutpostMask[Black][i] &= ((1ULL<<i)-1)>>1;
		OutpostMask[White][i] &= (~((1ULL<<i)-1)<<7);
	}
	/*
	for(i = 0; i < 64; i++){
		printf("\nMask for sqare %d\n", i);
		PrintBits(OutpostMask[Black][i]);
	}//*/
}

void InitAll(board_t *brd)
{
	InitMasks();
	InitEvalMasks();
	InitHash();
	InitPv(&brd->pv);
	InitCapScores();
}


