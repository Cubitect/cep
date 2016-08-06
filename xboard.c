// xboard.c

#include "defs.h"
#include <signal.h>

// converts a move to a xboard compatible string
char *StrXmove(int m)
{
	static char mvstr[8];

	sprintf(mvstr, "%c%c%c%c", 
			FILE(FROM(m)) + 'a', RANK(FROM(m)) + '1',
			FILE(TO(m)) + 'a',   RANK(TO(m)) + '1');
	if(m & FLAGPROM) 
		sprintf(mvstr, "%s%c", mvstr, pceChar[Black][PROMPCE(m)]);
	
	return mvstr;
}

// returns the number of times the current position has been reached
int Reps(board_t *brd){
	int r, i;
	for(r = 0, i = 0; i < brd->hisPly; i++){
		if(brd->hash == brd->history[i].hash) r++;
	}
	return r+1;
}

// returns true if there is enough material on the board to mate
int EnoughMaterial(board_t *brd){
	if(brd->bb[Both][Pawn] || brd->bb[Both][Queen] || brd->bb[Both][Rook]) 
		return true;
	if(CountBits(brd->bb[Both][Bishop]) > 1) return true;
	if(CountBits(brd->bb[Both][Knight]) > 1) return true;
	if(brd->bb[Both][Bishop] && brd->bb[Both][Knight]) 
		return true;
	return false;
}

// check if the game has ended; if so, respond appropriately and return true
int CheckResult(board_t *brd)
{
	// draw cases
	if(brd->fifty > 100){
		printf("1/2-1/2 {fifty move rule (claimed by %s)}\n", PROJECTNAME);
		fflush(stdout);
		return true;
	}
	if(Reps(brd) >= 3){
		printf("1/2-1/2 {3-fold move rule (claimed by %s)}\n", PROJECTNAME);
		fflush(stdout);
		return true;
	}
	if(!EnoughMaterial(brd)){
		printf("1/2-1/2 {insufficient material (claimed by %s)}\n", PROJECTNAME);
		fflush(stdout);
		return true;
	}
	
	mlist_t list;
	GenMoves(brd, &list);
  
	int i;
	int Legal = 0;
	
	for(i = 0; i < list.len; i++){
		if(MakeMove(brd, list.move[i].move)){
			TakeBack(brd);
			Legal++;
			break;
		}
    }

	// We have found a legal move in the current position so it cannot be mate or stalemate
	if(Legal > 0) return false;
	
	int check = SqAttacked(brd, LOCATEBIT(brd->bb[brd->side][King]), brd->side^1);
	
	if(check){
		if(brd->side == White){
			printf("0-1 {black mates (claimed by %s)}\n", PROJECTNAME);
			fflush(stdout);
		} else {
			printf("0-1 {white mates (claimed by %s)}\n", PROJECTNAME);
			fflush(stdout);
		}
	}
	else {
		 printf("1/2-1/2 {stalemate (claimed by %s)}\n", PROJECTNAME);
		 fflush(stdout);
	}
	return true;
}

// The xboard mode
void XboardLoop(board_t *brd, searchinfo_t *info)
{
	int move;
	int overwrite = -1;	// Not part of xboard; to force a certain timeout for the engine
	int analyse = false;
	int post = 0; // Type of output: 0 - none; 1 - xboard; 2 - console
	
	int compSide;
	char line[256], cmd[256];

	signal(SIGINT, SIG_IGN);	// prevent the GUI's interrupt signals from terminating the program

	ParseFen(brd, START_FEN);	// setup starting position by default
	printf("\n");

	compSide = Both;	// I use 'Both' for the computer side if it shall not make a move
	info->quit = false;
	
	while(true) 
	{
		fflush(stdout);

		if(info->quit) return;

		if(compSide == brd->side)	// if it is our turn to move
		{
			if(CheckResult(brd)){
				compSide = Both;	// Has the game ended?
				continue;
			}

			// sort out how much time we can use for the search
			if(overwrite != -1) info->time = overwrite;
			info->startTime = GetTime();
			info->stopTime = info->startTime + info->time;
			info->infinite = false;

			move = IterSearch(brd, info, post);	// Search

			if(MakeMove(brd, move)){
				printf("move %s\n", StrXmove(move));	// send the move to the GUI
			}
			else{
				printf("Illegal move: %s\n", StrMove(brd->pvLine[0]));
			}
			if(CheckResult(brd)) compSide = Both;	// Has the game ended?
			continue;
		}

		if(analyse)
		{
			info->startTime = GetTime();

			info->infinite = true;
			if(CheckResult(brd)) {
				analyse = false;
				continue;
			}
			IterSearch(brd, info, post);

			strcpy(line, intStr);	// copy the input to be interpreted

			if(CheckResult(brd)) analyse = false;
		}
		else if(!fgets(line, 256, stdin)){
			return;
		}

		if(info->quit) return;

		if(line[0]=='\n')
			continue;
		
		sscanf(line, "%s", cmd);

		if(!strcmp(cmd, "protover")){
			printf("feature setboard=1 myname=\"%s\" analyze=1\n", PROJECTNAME);
		}
		else if(!strcmp(cmd, "xboard")){
			continue;
		}
		else if(!strcmp(cmd, "analyze")){
			analyse = true;
			compSide = Both;
			continue;
		}
		else if(!strcmp(cmd, "exit")){
			analyse = false;
			continue;
		}
		else if(!strcmp(cmd, "new")){
			ParseFen(brd, START_FEN);
			compSide = Black;
			continue;
		}
		else if(!strcmp(cmd, "setboard")){
			ParseFen(brd, line + 9);
			continue;
		}
		else if(!strcmp(cmd, "quit")){
			return;
		}
		else if(!strcmp(cmd, "force")){
			compSide = Both;
			continue;
		}
		else if(!strcmp(cmd, "st")){
			sscanf(line, "st %d", &info->time);
			info->time *= 1000;
			info->toDepth = MAXDEPTH-1;
			continue;
		}
		else if(!strcmp(cmd, "sd")){
			sscanf(line, "sd %d", &info->toDepth);
			info->time = LONGTIME;
			continue;
		}
		else if(!strcmp(cmd, "time")){
			sscanf(line, "time %d", &info->time);
			info->time *= 10;
			info->time /= TIME_FRACTION * (double)(41+TIME_MULTIPLIER - (double)(brd->hisPly%80)/2);
			info->time -= 50;
			info->toDepth = MAXDEPTH -1;
			continue;
		}
		else if(!strcmp(cmd, "otim")){
			continue;
		}
		else if(!strcmp(cmd, "post")){
			post = 1;
			continue;
		}
		else if(!strcmp(cmd, "nopost")){
			post = 0;
			continue;
		}
		else if(!strcmp(cmd, "go")){
			compSide = brd->side;
			continue;
		}
		else if(!strcmp(cmd, "undo")){
			TakeBack(brd);
			brd->ply = 0;
			continue;
		}
		else if(!strcmp(cmd, "remove")){
			TakeBack(brd);
			TakeBack(brd);
			brd->ply = 0;
			continue;
		}
		else if(!strcmp(cmd, "result")){
			compSide = Both;
			continue;
		}
		else if(!strcmp(cmd, "printb")){ // not part of xboard
			PrintBrd(brd);
			continue;
		}
		else if(!strcmp(cmd, "fixtime")){ // not part of xboard
			sscanf(line, "fixtime %d", &overwrite);
			continue;
		}
		else if(!strcmp(cmd, "freetime")){ // not part of xboard
			overwrite = -1;
			continue;
		}

		move = ParseMove(line, brd);	// try to interpret the input as move
		
		if(move == NO_MOVE) {
			printf("Error (unknown command): %s\n", cmd);
			continue;
		}
		if(MoveExists(brd, move)){
			StorePvMove(brd, move, 0, 0, HFNONE);
			MakeMove(brd, move);
			brd->ply = 0;
		}
		if(CheckResult(brd)) compSide = Both;
	}
}


