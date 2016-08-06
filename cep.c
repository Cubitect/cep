/*
 ============================================================================
 Name        : cep.c
 Author      : https://github.com/Cubitect
 Version     : Alpha 1.0.1
 Copyright   : GPL 3.0 (https://github.com/Cubitect)
 Description : Chess Engine Project / Chess - Extended Project
 ============================================================================
 */


#include "defs.h"

int main(int argc, char **argv)
{
	board_t brd[1];
   	searchinfo_t info[1];

   	// stop the i/o buffers interfering communication with the GUI
	setbuf(stdin, NULL);
	setbuf(stdout, NULL);

	InitAll(brd);

	printf("%s\n", PROJECTNAME);
	printf("https://github.com/Cubitect/cep\n\n");

	char line[256];
	int compSide = Both;
	int move = NO_MOVE;

	info->time = 12000;
	info->toDepth = MAXDEPTH-1;

	// set up starting position
	ParseFen(brd, START_FEN);

	while(true)
	{
		if(compSide == brd->side)
		{
			info->startTime = GetTime();
			info->stopTime = info->startTime + info->time;

			IterSearch(brd, info, 2);

			GetPvLine(brd, 1);

			if(MakeMove(brd, brd->pvLine[0])){
				printf("move %s\n", StrXmove(brd->pvLine[0]));
			}
			else{
			}
			CheckResult(brd);
			continue;
		}

		printf("cep> ");
		fflush(stdout);

		memset(line, 0, sizeof(line));

		if(scanf("%s", line) == EOF) return 0;

		if(!strcmp(line, "xboard")){
			XboardLoop(brd, info);
			break;
		}
		else if(!strcmp(line, "quit")){
			break;
		}
		if(!strcmp(line, "on")){	// play for the current side
			compSide = brd->side;
			continue;
		}
		if(!strcmp(line, "off")){
			compSide = Both;
			continue;
		}
		else if(!strcmp(line, "st")){	// set the maximum time to make a move
			if(scanf("%d", &info->time) == EOF) return 0;
			info->time *= 1000;
			info->toDepth = MAXDEPTH-1;
			continue;
		}
		else if(!strcmp(line, "sd")){	// set the maximum search depth
			if(scanf("%d", &info->toDepth) == EOF) return 0;
			info->time = LONGTIME;
			continue;
		}
		else if(!strcmp(line, "undo")){
			TakeBack(brd);
			compSide = Both;
			brd->ply = 0;
			continue;
		}
		else if(!strcmp(line, "new")){
			ParseFen(brd, START_FEN);
			compSide = Both;
			continue;
		}
		else if(!strcmp(line, "d")){
			PrintBrd(brd);
			continue;
		}
		else if(!strcmp(line, "db")){

			continue;
		}
		else if(!strcmp(line, "setboard") || !strcmp(line, "fen")){
			if(!fgets(line, 256, stdin)) continue;
			ParseFen(brd, line+1);
			compSide = Both;
			continue;
		}
		else if(!strcmp(line, "eval")){
			printf("Eval: %d\n", Eval(brd));
			continue;
		}

		move = ParseMove(line, brd);

		if(move != NO_MOVE){
			if(!MakeMove(brd, move)){
				printf("Illegal Move\n");
			}
			else {
				CheckResult(brd);
			}
		}
		else{
			printf("Error (unknown command): %s\n", line);
		}
	}

	free(brd->pv.pTable);

	return 0;
}
