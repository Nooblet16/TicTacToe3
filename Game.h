#pragma once

#include <stdlib.h>
#include "Array3.h"

//To test all posible rows and diagonals, you have to go in 13 directions from each cell.
static const int nLineDirs = 13;
static const int lineDirs[nLineDirs][3] = {
	{ 1, 0, 0},
	{ 0, 1, 0},
	{ 0, 0, 1},
	{ 0, 1, 1},
	{ 0,-1, 1},
	{ 1, 0, 1},
	{-1, 0, 1},
	{ 1, 1, 0},
	{-1, 1, 0},
	{ 1, 1, 1},
	{-1, 1, 1},
	{ 1,-1, 1},
	{ 1, 1,-1},
};


struct Game
{
	int size;//Grid size.
	int nToWin;//Winning combination length.
	Array3<int> contents;//Cell contents: empty (0), Player 1 mark (1), Player 2 mark (2)
	Array3<int> winning;//Used to highlight cells comprising the winning combinations.

	Game()
	{
		size = 0;
	}
	void reset(int sz, int ntw)
	{
		if(size != sz){
			//If requested different grid size, allocate new arrays.
			size = sz;
			contents.allocate(size);
			winning.allocate(size);
		}
		nToWin = ntw;
		contents.set(0);
		winning.set(0);
	}
	void markWinningLine(int i, int j, int k, const int dir[3])
	{
		for(int t=0; t<nToWin; t++){
			winning(i,j,k) = true;
			i += dir[0];
			j += dir[1];
			k += dir[2];
		}
	}
	//Calculate the number of non-empty cells in a line. Used in the heuristic algorithm.
	int getNumMarksInLine(int i, int j, int k, const int dir[3])
	{
		int n = 0;
		for(int t=0; t<nToWin; t++){
			if(contents(i,j,k)){
				++n;
			}
			i += dir[0];
			j += dir[1];
			k += dir[2];
		}
		return n;
	}
	//Check if the line can become winning after 1 or more moves.
	//A potential winning line can only have empty cells or marks of the same player.
	bool isLinePotentialWin(int i, int j, int k, const int dir[3])
	{
		int combination = 0;
		for(int t=0; t<nToWin; t++){
			if(i<0 || i>=size) return false;
			if(j<0 || j>=size) return false;
			if(k<0 || k>=size) return false;
			combination |= contents(i,j,k);
			if(combination >= 3){
				return false;
			}
			i += dir[0];
			j += dir[1];
			k += dir[2];
		}
		return true;
	}
	//Check if all cells comprising the line hold the same value.
	bool isLineAllTheSame(int i, int j, int k, const int dir[3])
	{
		int player = contents(i,j,k);
		for(int t=0; t<nToWin; t++){
			if(i<0 || i>=size) return false;
			if(j<0 || j>=size) return false;
			if(k<0 || k>=size) return false;
			if(contents(i,j,k) != player){
				return false;
			}
			i += dir[0];
			j += dir[1];
			k += dir[2];
		}
		return true;
	}
	//Examine the current state of the field and determine the victory, draw or non-final state.
	//Return value:
	//	0: Continue playing
	//	1: Player 1 wins
	//	2: Player 2 wins
	//	3: Draw
	int checkGameState()
	{
		int draw = 3;
		for(int k=0; k<size; k++){
		for(int j=0; j<size; j++){
		for(int i=0; i<size; i++){
			if(contents(i,j,k)){
				for(int d=0; d<nLineDirs; d++){
					if(isLineAllTheSame(i,j,k,lineDirs[d])){
						return contents(i,j,k);//Return winning player.
					}
				}
			}
			for(int d=0; d<nLineDirs; d++){
				if(isLinePotentialWin(i,j,k,lineDirs[d])){
					draw = 0;//At least one potential winning line removes the possibility of draw.
				}
			}
		}
		}
		}
		return draw;
	}

	void markWinningLines()
	{
		for(int k=0; k<size; k++){
		for(int j=0; j<size; j++){
		for(int i=0; i<size; i++){
			if(contents(i,j,k)){
				for(int d=0; d<nLineDirs; d++){
					if(isLineAllTheSame(i,j,k,lineDirs[d])){
						markWinningLine(i,j,k,lineDirs[d]);
					}
				}
			}
		}
		}
		}
	}

	
	//Heuristic AI decision routine.
	//Simple and fast, makes fairly smart but beatable AI.
	/*
		The algorithm is as follows.

		1. Check for winning conditions.
		Check if there are any "winning" cells and randomly occupy one of
		them. The effect is that either the AI wins, or it blocks the opponent
		from winning. It is easy to make the AI prefer its own winning to
		blocking the opponent, but for now it's random.

		2. Evaluate the "importance" of unoccupied cells.
		Create an array of cell "weights" and initialize with zeros.
		For each unoccupied cell, examine all lines (rows and diagonals)
		containing it. If a line is potentially winning (either empty or has
		only one player's marks in it), count the number of marks on that line
		and add to the cell's weight.
		Again, the players are not distinguished, so larger weight means both
		a higher probability or winning and blocking the opponent.

		3. Randomly occupy one of the cells with maximum weight.
	*/
	int heuristicMove(int player)
	{
		//First, check the winning conditions
		for(int k=0; k<size; k++){
		for(int j=0; j<size; j++){
		for(int i=0; i<size; i++){
			for(int d=0; d<nLineDirs; d++){
				if(isLinePotentialWin(i,j,k,lineDirs[d])){
					if(getNumMarksInLine(i,j,k,lineDirs[d]) == nToWin-1){//The line is one step from winning.
						int i1 = i;
						int j1 = j;
						int k1 = k;
						for(int t=0; t<nToWin; t++){
							if(! contents(i1,j1,k1)){
								return contents.index(i1, j1, k1);//Return the only empty cell in this line.
							}
							i1 += lineDirs[d][0];
							j1 += lineDirs[d][1];
							k1 += lineDirs[d][2];
						}
					}
				}
			}
		}
		}
		}
		
		//Calculate importance or "weight" of empty cells.
		Array3<int> weight(size);
		weight.set(0);
		for(int k=0; k<size; k++){
		for(int j=0; j<size; j++){
		for(int i=0; i<size; i++){
			for(int d=0; d<nLineDirs; d++){
				if(! isLinePotentialWin(i,j,k,lineDirs[d])){
					continue; //Skip lines that have mixed marks and can't ever become winning.
				}
				int i1 = i;
				int j1 = j;
				int k1 = k;
				int w = getNumMarksInLine(i,j,k,lineDirs[d]);
				for(int t=0; t<nToWin; t++){
					if(! contents(i1,j1,k1)){
						weight(i1,j1,k1) += w;
					}
					i1 += lineDirs[d][0];
					j1 += lineDirs[d][1];
					k1 += lineDirs[d][2];
				}
			}
		}
		}
		}
		
		//Calculate the number of cells with the maximum weight.
		int maxW = 0;
		int nMaxW = 0;
		for(int i=0; i<size*size*size; i++){
			if(weight[i] > maxW){
				maxW = weight[i];
				nMaxW = 0;
			}
			if(weight[i] == maxW){
				++nMaxW;
			}
		}
		//Pick one of those cells randomly.
		int k = nMaxW*rand()/RAND_MAX;
		if(k >= nMaxW) k = nMaxW-1;
		for(int i=0; i<size*size*size; i++){
			if(weight[i] == maxW){
				--k;
				if(k <= 0){
					return i;
				}
			}
		}
		return 0;
	}

	//AI decision routine picking a random empty cell.
	//Makes for stupid, trivially beatable AI.
	int randomMove()
	{
		int nEmptyCells = 0;
		for(int i=0; i<contents.bufferSize(); i++){
			if(contents[i] == 0){
				++nEmptyCells;
			}
		}
		int k = nEmptyCells * rand() / RAND_MAX;
		if(k >= nEmptyCells) k = nEmptyCells - 1;
		for(int i=0; i<contents.bufferSize(); i++){
			if(contents[i] == 0){
				--k;
				if(k <= 0){
					return i;
				}
			}
		}
		return 0;
	}
	
	
	//Minmax routine.
	//Theoretically, makes a perfect player.
	//But it's unacceptably slow with depth > 3 even on a 3x3x3 grid.
	int minmax(int player, int turn, int& move, int depth)
	{
		static const int winScore = 1;
		if(depth <= 0){
			move = 0;
			return 0;
		}
		int best = (turn == player) ? INT_MIN : INT_MAX;
		for(int i=0; i<contents.bufferSize(); i++){
			if(contents[i] == 0){
				contents[i] = turn;
				int score = 0;
				int winner = checkGameState();
				if(winner == 0){//Non-final state, invoke minmax on it.
					int move = 0;
					score = minmax(player, 3-turn, move, depth-1);
				}else if(winner == 3){//Draw
					score = 0;
				}else{//Either the player or the opponent wins.
					score = (winner == player) ? winScore : -winScore;
				}
				contents[i] = 0;//Undo mark
				if(turn == player){//Player's turn, maximize score
					if(score > best){
						best = score;
						move = i;
					}
					if(best >= winScore){
						break;//Early out, an attempt to cull some branches.
					}
				}else{//Opponent's turn, minimize score
					if(score < best){
						best = score;
						move = i;
					}
					if(best <= -winScore){
						break;//Early out, an attempt to cull some branches.
					}
				}
			}
		}
		return best;
	}

	//Minmax AI decision routine.
	int minmaxMove(int player)
	{
		int move = 0;
		minmax(player, player, move, 3);
		return move;
	}
};
