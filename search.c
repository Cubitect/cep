// search.c

#include "defs.h"

// this keeps the score values for captures for move ordering
// so that "pawn captures queen" is searched before "queen captures pawn"
int capScore[7][7]; /* [Victim][Attacker] */

// This is called every 4096 nodes to check if we have run out of time
// or if we have been interrupted by the GUI
static void CheckUp(searchinfo_t *sinfo)
{
	if(sinfo->infinite == false && GetTime() >= sinfo->stopTime){
		sinfo->stop = true;
	}
	CheckInput(sinfo);
}

// Returns true if the current position is a repetition
int IsRep(const board_t *brd)
{
	int i;
	for(i = brd->hisPly - brd->fifty; i < brd->hisPly-1; i++){
		if(brd->history[i].hash == brd->hash) return true;
	}
	return false;
}

// This function puts the move with the highest score that has not been searched yet
// at the position that is searched next (i.e. the move ordering function)
void SelectNextMove(mlist_t *list, int moveNum)
{
	int i;
	int bestScore = list->move[moveNum].score;
	int bestIndex = moveNum;
	move_t tmp;
	for(i = moveNum; i < list->len; i++){
		if(list->move[i].score > bestScore){
			bestIndex = i;
			bestScore = list->move[i].score;
		}
	}
	tmp = list->move[moveNum];
	list->move[moveNum] = list->move[bestIndex];
	list->move[bestIndex] = tmp;
}

// clearing variables for the search
void ClrForSearch(board_t *brd, searchinfo_t *sinfo)
{
	//ClrPv(&brd->pv);
	brd->ply = 0;
	memset(brd->betaMoves, 0, sizeof(brd->betaMoves));
	sinfo->nodes = 0;
	sinfo->quit = sinfo->stop = false;
	sinfo->fh = 0.0000001;	// cannot be 0 as we will be dividing fhf by fh
	sinfo->fhf = 0.0;
}

// the quiescence search
int Quiece(board_t *brd, int alpha, int beta, searchinfo_t *sinfo)
{
	if(!(sinfo->nodes & 0xfff)) CheckUp(sinfo);

	sinfo->nodes++;

	ASSERT(CheckBrd(brd));

	//if((IsRep(brd) || brd->fifty >= 100) && brd->ply) return 0;
	if(brd->ply >= MAXDEPTH-1) return Eval(brd);

	int score = Eval(brd);

	// we are probably not going to make our position worse by moving so we can say:
	if(score >= beta){	// if we are doing already too good
		return beta;	// we have a beta cutoff and we return
	}
	if(score > alpha){	// if we are better than alpha
		alpha = score;	// then we can set our minimum alpha to the score
	}

	mlist_t list;
	int i;
	score = -INFINITE;

	GenCaps(brd, &list);	// Generate all capture moves

	for(i = 0; i < list.len; i++)	// Loop through the moves
	{
		if(sinfo->stop) return 0;

		SelectNextMove(&list, i);

		if(!MakeMove(brd, list.move[i].move)) continue;

		score = -Quiece(brd, -beta, -alpha, sinfo);

		TakeBack(brd);

		if(score > alpha){
			if(score >= beta){
				return beta;
			}
			alpha = score;
		}
	}

	return alpha;
}


int AlphaBeta(board_t *brd, int alpha, int beta, int depth, searchinfo_t *sinfo, int null)
{
	if(depth == 0){
		//return Eval(brd);
		return Quiece(brd, alpha, beta, sinfo);
	}

	if(!(sinfo->nodes & 0xfff)) CheckUp(sinfo);

	sinfo->nodes++;

	ASSERT(CheckBrd(brd));

	// if we are not the root of the tree and we have a repetition
	// or we exceeded the fifty move rule, then we have a draw and return 0
	if((IsRep(brd) || brd->fifty >= 100) && brd->ply) return 0;
	if(brd->ply >= MAXDEPTH-1) return Eval(brd);	// if we are too deep, we return

	// test if we are in check
	int check = SqAttacked(brd, LOCATEBIT(brd->bb[brd->side][King]), brd->side^1);
	if(check) depth++;

	int score = -INFINITE;
	int pvMain = NO_MOVE;
	// check the transposition table if we have searched the current position before
	// if so, return what we found
	if(TestHashTable(brd, &pvMain, &score, alpha, beta, depth)) return score;

	// Before we search, we try how we do if we don't make a move
	// i.e. make a null move
	if( null && !check && brd->ply && depth >= 4 &&
		(brd->all[brd->side]^brd->bb[brd->side][Pawn]^brd->bb[brd->side][King]) )
	{
		MakeMoveNull(brd);
		// We cannot do two null moves in a row so we set the null argument here to false
		score = -AlphaBeta(brd, -beta, -beta + 1, depth-4, sinfo, false);
		TakeBackNull(brd);

		if(sinfo->stop){
			return 0;
		}
		if(score >= beta){
			return beta;
		}
	}

	mlist_t list;
	int i;
	int legal = 0;
	int bestMove = NO_MOVE;
	int bestScore = -INFINITE;
	int oldAlpha = alpha;
	score = -INFINITE;

	GenMoves(brd, &list);

	// we tested our transposition table for the best move 'pvMain'
	// if it exists we will set this move as the first move to be searched
	if(pvMain != NO_MOVE){

		if(MakeMove(brd, pvMain)){
			score = -AlphaBeta(brd, -beta, -alpha, depth-1, sinfo, true);
			TakeBack(brd);

			bestScore = score;
			if(score > alpha){
				if(score >= beta){
					if(!(pvMain & FLAGCAP)){
						brd->betaMoves[1][brd->ply] = brd->betaMoves[0][brd->ply];
						brd->betaMoves[0][brd->ply] = pvMain;
					}
					return beta;
				}
				alpha = score;
				bestMove = pvMain;
			}
		}
		/*/
		for(i = 0; i < list.len; i++){
			if(list.move[i].move == pvMain){
				list.move[i].score = 5000000;
				break;
			}
		}//*/
	}

	for(i = 0; i < list.len; i++) // Loop through all moves
	{
		if(sinfo->stop) return 0;

		SelectNextMove(&list, i);

		if(!MakeMove(brd, list.move[i].move)) continue;

		legal++; // we have found a legal move so we need not check for mate afterwards
		// call AlphaBeta in a negamax fashion
		score = -AlphaBeta(brd, -beta, -alpha, depth-1, sinfo, true);
		TakeBack(brd);

		if(score > bestScore){
			bestScore = score;
			if(score > alpha){
				if(score >= beta){
					if(legal==1) sinfo->fhf++; 	// count the number of beta cutoffs searched first
					sinfo->fh++;				// compared to all beta cutoffs (measure of efficiency)
					if(!(list.move[i].move & FLAGCAP)){
						brd->betaMoves[1][brd->ply] = brd->betaMoves[0][brd->ply];
						brd->betaMoves[0][brd->ply] = list.move[i].move;
					}
					// Store the move in the transposition table as a beta (killer) move
					StorePvMove(brd, bestMove, depth, beta, HFBETA);
					return beta;
				}
				alpha = score;
				bestMove = list.move[i].move;
			}
		}
	}
	if(legal == 0){ // We havn't found a legal move
		if(check){
			// if we are in check it is mate; but we want the shortest path
			// to it so we'll subtract the path length
			return -MATE + brd->ply;
		}
		return 0; // Stalemate
	}

	if(oldAlpha != alpha){
		// Store exact (normal) transposition entry
		StorePvMove(brd, bestMove, depth, bestScore, HFEXACT);
	}
	else {
		// Store alpha cutoff transposition entry
		StorePvMove(brd, bestMove, depth, alpha, HFALPHA);
	}

	return alpha;
}

// performs an alpha beta search with iterative deepening and returns the best move found
int IterSearch(board_t *brd, searchinfo_t *sinfo, int post)
{
	int bestScore = -INFINITE;
	int bestMove = NO_MOVE;
	int moveNum = 0;

	int iterDepth;
	int i;
	int tdif = 0;
	ClrForSearch(brd, sinfo);

	for(iterDepth = 1; iterDepth <= sinfo->toDepth; iterDepth++) // increase depth for each iteration
	{
		if(!sinfo->infinite) tdif = -GetTime();
		bestScore = AlphaBeta(brd, -INFINITE, INFINITE, iterDepth, sinfo, true);
		if(!sinfo->infinite) tdif += GetTime();

		// if we received a stop signal, simply break out and use the best move found so far
		if(sinfo->stop) break;

		// Fill 'pvLine' with the sequence of best moves found
		moveNum = GetPvLine(brd, iterDepth);

		// print some search progress depending on the 'post' format
		if(post == 1){
			printf("%d %d %d %" PRIu64,
				iterDepth, bestScore, (GetTime()-sinfo->startTime)/10, sinfo->nodes
			);
			for(i = 0; i < moveNum; i++){
				printf(" %s", StrXmove(brd->pvLine[i]));
			}
			printf("\n");
		}
		else if (post == 2) {
			printf("score:%7d nodes:%9" PRIu64 " eff: %3d%%   ", bestScore,
					sinfo->nodes, (int)(100*sinfo->fhf/sinfo->fh));
			for(i = 0; i < moveNum; i++){
				printf("%s ", StrMove(brd->pvLine[i]));
			}
			printf("\n");
		}
		bestMove = brd->pvLine[0];

		if(!sinfo->infinite && sinfo->startTime + TIME_MULTIPLIER*tdif > sinfo->stopTime) break;
	}
	return bestMove;
}

void InitCapScores()
{
	int vic, att;
	for(vic = Empty; vic <= King; vic++){
		for(att = Empty; att <= King; att++){
			capScore[vic][att] = pceMat[vic] - pceMat[att]/100;
		}
	}
}


