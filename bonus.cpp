// Bonus.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <iterator>
#include <list>
#include <stack>
#include <queue>
#ifdef _MSC_VER
#include <Windows.h>
#endif

#ifndef _MSC_VER
// If it's not Windows, create our own version of COORD struct
typedef struct { int X; int Y; } COORD;
#endif

// how far from the upper left corner to start displaying maze
#define XOFFSET 2
#define YOFFSET 2

typedef std::vector<char> LINE;
typedef std::vector<LINE> MAZE;

class Maze {
private:
	MAZE maze;
	MAZE saveMaze;
	std::list<COORD> solution;
	bool _animate = false;
	int height;		// cache height of maze for convenience
	int xoffset;
	int yoffset;


public:
	Maze(bool animate) : _animate(animate) {}
	int ReadMaze(std::ifstream& mazeFile) {
		int maxwidth = 0;
		height = 0;
		while (mazeFile.good()) {
			char c;
			int width = 0;
			std::vector<char> line;
			while (mazeFile.get(c) && c != '\n' && c != '\r') {
				line.push_back(c);
				width++;
			}
			maze.push_back(line);
			height++;
			c = mazeFile.peek();
			if (c == '\n' || c == '\r') mazeFile.get(c);	// eat any extra LF, or CR, or whatever..
			line.clear();
			if (width > maxwidth) maxwidth = width;
		}
		// keep an original maze copy because our solution is destructive
		saveMaze = maze;
		mazeFile.close();
		xoffset = XOFFSET;	// reset display positions
		yoffset = YOFFSET;
		return maxwidth;
	}

	void print_maze() {

		for (MAZE::iterator mazeIter = maze.begin(); mazeIter != maze.end(); mazeIter++) {
			for (LINE::iterator lineIter = mazeIter->begin(); lineIter != mazeIter->end(); lineIter++) {
				std::cout << *lineIter;
			}
			std::cout << std::endl;
		}
	}

	// helper to determine if potential move is valid
	bool ValidMove(int x, int y) {
		// check for out of bounds
		if (x < 0 || y < 0) return false;
		if (x >= (int)maze[y].size() || y >(int)maze.size()) return false;
		// if we're in bounds only illegal move is a 
		return ('x' != maze[y][x]);
	}

#ifdef _MSC_VER
	// Use Win32 API to display maze in a fixed position (starting at 2,2) on the console screen,
	// so we can update it live while searching. (Todo: create Unix version using curses..)
	void DisplayMaze() {
		int height = maze.size(), line = yoffset;
		DWORD num_written;
		COORD coord = { xoffset, line };
		SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coord);

		for (MAZE::iterator mazeIter = maze.begin(); mazeIter != maze.end(); mazeIter++) {
			std::string line(mazeIter->begin(), mazeIter->end());
			WriteConsoleA(GetStdHandle(STD_OUTPUT_HANDLE), line.c_str(), line.length(), &num_written, NULL);
			coord.Y++;	// move to next line
			SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coord);
		}
		int x, y;
		FindStart(x, y);
		SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), { x + xoffset, y + yoffset });
	}

	void UpdateDisplay(int x, int y) {
		DWORD num_written;
		SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), { x + xoffset, y + yoffset });
		WriteConsoleA(GetStdHandle(STD_OUTPUT_HANDLE), &maze[y][x], 2, &num_written, NULL);
		SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), { x + xoffset, y + yoffset });
	}

	void Message(char *text) {
		int height = maze.size() + yoffset + 1;
		SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), { xoffset, height });
		std::cout << text << std::endl;
	}

	void Position(int x, int y) {
		SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), { x, y });
	}
#else
	void DisplayMaze(int xoffset, int yoffset) {
		print_maze(maze);
	}
	void UpdateDisplay(int x, int y) {
		print_maze(maze);
	}
	void Message(char *text) {
		cout << text << endl;
	}

#endif

	// Find the start of the maze, as indicated by the 's' character.
	// We return the first one we find, no checking if there might be another 's'
	bool FindStart(int& x, int& y) {
		for (y = 0; y != maze.size(); y++) {
			LINE& line = maze[y];
			for (x = 0; x != line.size(); x++) {
				if (maze[y][x] == 's' || maze[y][x] == 'S') return true;
			}
		}
		return false;	// no start found
	}

	// Check to see if the move is valid.
	// Return 'x' for an invalid move, '.' for moving to a '.', or 'e' for end!
	// Modify the local we move to by placing a '+' there to indicate we've been there.
	// also return '+' if we return to where we've been
	char Move(int& x, int &y, int newX, int newY) {
		if (newX < 0 || newY < 0) return 'x';
		if (newX >= (int)maze[newY].size() || newY >(int)maze.size()) return 'x';
		char c = tolower(maze[newY][newX]);
		switch (c) {
		case 'x':
			return 'x';

		case 'e':
		case 's':
		case '.':
		case '+':
			x = newX;
			y = newY;
			if (c != 'e' && c != 's') maze[y][x] = '+';
			else if ('e' == c) maze[y][x] = 'E';
			UpdateDisplay(x, y);
			return maze[y][x];
		}
		return ' ';
	}

#ifdef _MSC_VER
	// Interactive version -- manually solve the maze using cursor keys!
	bool Walk(int& x, int& y) {
		INPUT_RECORD input;
		int key;
		DWORD numread;
		char result = ' ';

		ReadConsoleInput(GetStdHandle(STD_INPUT_HANDLE), &input, 1, &numread);
		// get keyboard evetns only on key down, otherwise we get both down and up
		if (input.EventType == KEY_EVENT && input.Event.KeyEvent.bKeyDown) {
			key = input.Event.KeyEvent.wVirtualKeyCode;
			switch (key) {
			case VK_LEFT:
				result = Move(x, y, x - 1, y);
				break;

			case VK_RIGHT:
				result = Move(x, y, x + 1, y);
				break;

			case VK_DOWN:
				result = Move(x, y, x, y + 1);
				break;

			case VK_UP:
				result = Move(x, y, x, y - 1);
				break;

			case 'Q':
			case 'q':
				std::cout << "Giving up!";
				exit(0);
				break;
			}
		}
		if (result == 'x') Beep(1200, 120);
		return (result == 'E');
	}
#else
	// empty shell for now..
	bool Walk(int& x, int& y) {

	}
#endif

	bool SolveRecursive(int oldX, int oldY, int x, int y) {
		if (!ValidMove(x, y)) return false;
		if (maze[y][x] == 'e') {
			maze[y][x] = 'E';
			UpdateDisplay(x, y);
			return true;
		}

		// We save our state at this cell in the maze.
		// We could just make this another recursive parameter.
		// But doing it this way lets us visibly follow the progress,
		// and would enable an iterative solution.
		char here = maze[y][x];
		// first time at this cell, implicitly set it to '0'
		if ('s' == here || '.' == here) here = '0';

		// doing this in a for loop saves a few lines of code
		// arbitrarily decide 1=right, 2=down, 3=left, 4=up
		for (int i = here - '0'; i != 5; i++) {
			int newX, newY;
			// don't retrace where we've been.
			// we always go in the same order
			if (i <= here - '0') continue;
			switch (i) {
			case 1:		// right
				newX = x + 1;
				newY = y;
				break;
			case 2:		// down
				newX = x;
				newY = y + 1;
				break;
			case 3:		// left
				newX = x - 1;
				newY = y;
				break;
			case 4:		// up
				newX = x;
				newY = y - 1;
				break;
			}

			maze[y][x] = '0' + i;
			UpdateDisplay(x, y);
			if (_animate) Sleep(200);

			// save our backtracking step for last
			if (!(newX == oldX && newY == oldY)) {
				// if this path solved it, keep on returning.
				if (SolveRecursive(x, y, newX, newY)) {
					// remember the path
					solution.push_back({ x, y });
					return true;
				}
			}
		}

		// no solution was found from this point, try going back
		return false;
	}

	bool StackSolve(int x, int y) {
		std::stack<COORD> stack;
		int oldX = -1, oldY = -1, newX, newY;
		bool solved = false;

		while (!solved) {
			// if we've hit the 'e', we're done.
			if (maze[y][x] == 'e') {
				maze[y][x] = 'E';
				UpdateDisplay(x, y);
				solved = true;
				break;
			}

			char here = maze[y][x];
			if ('s' == here || '.' == here) here = '0';
			// cache where we came from so we don't do this
			// every time inside the for loop
			if (!stack.empty()) {
				oldX = stack.top().X;
				oldY = stack.top().Y;
			}

			int i;
			// Try various solutions in order
			// arbitrarily decide 1=right, 2=down, 3=left, 4=up
			// pick up from where we left off
			for (i = here - '0' + 1; i != 6; i++) {
				switch (i) {
				case 1:		// right
					newX = x + 1;
					newY = y;
					break;
				case 2:		// down
					newX = x;
					newY = y + 1;
					break;
				case 3:		// left
					newX = x - 1;
					newY = y;
					break;
				case 4:		// up
					newX = x;
					newY = y - 1;
					break;
				case 5:		// case 5 is back whence we came
					break;
				}

				// Case 5 means we already tried all the directions, except
				// the one we came from, and found no success.
				// Now it's time to go back whence we came and try again.
				if (5 == i) {
					// if there is no back whence we came, the maze can't be solved!
					if (stack.empty()) return false;

					COORD back = stack.top();
					stack.pop();
					x = back.X;
					y = back.Y;
					// we could have done this in the switch, but then "break"
					// wouldn't take us out of the for loop!
					break;
				}

				// if the move isn't valid, we don't even try it
				if (!ValidMove(newX, newY)) continue;

				maze[y][x] = '0' + i;
				UpdateDisplay(x, y);
				if (_animate) Sleep(200);

				// we try all the directions except the one we came from
				// (we save that direction for case 5)
				if (!(newX == oldX && newY == oldY)) {

					// remember where we were
					stack.push({ x, y });
					// then go to the new place
					x = newX;
					y = newY;
					break;
				}

			}
		}
		// put our solution in a form compatible with the recursive one
		solution.clear();
		while (!stack.empty()) {
			solution.push_back(stack.top());
			stack.pop();
		}
		return solved;
	}

	bool QueueSolve(int x, int y) {
		std::queue<COORD> queue;
		int oldX = -1, oldY = -1, newX, newY;
		bool solved = false;

		while (!solved) {
			// if we've hit the 'e', we're done.
			if (maze[y][x] == 'e') {
				maze[y][x] = 'E';
				UpdateDisplay(x, y);
				solved = true;
				break;
			}

			char here = maze[y][x];
			if ('s' == here || '.' == here) here = '0';

			int i;
			// Try various solutions in order
			// arbitrarily decide 1=right, 2=down, 3=left, 4=up
			// pick up from where we left off
			for (i = here - '0' + 1; i != 6; i++) {
				switch (i) {
				case 1:		// right
					newX = x + 1;
					newY = y;
					break;
				case 2:		// down
					newX = x;
					newY = y + 1;
					break;
				case 3:		// left
					newX = x - 1;
					newY = y;
					break;
				case 4:		// up
					newX = x;
					newY = y - 1;
					break;
				case 5:		// case 5 is back whence we came
					break;
				}

				// Case 5 means we already tried all the directions, except
				// the one we came from, and found no success.
				// Now it's time to go back whence we came and try again.
				if (5 == i) {
					// if there is no back whence we came, the maze can't be solved!
					if (queue.empty()) return false;

					COORD back = queue.front();
					queue.pop();
					x = back.X;
					y = back.Y;
					// we could have done this in the switch, but then "break"
					// wouldn't take us out of the for loop!
					break;
				}

				// if the move isn't valid, we don't even try it
				if (!ValidMove(newX, newY)) continue;

				maze[y][x] = '0' + i;
				UpdateDisplay(x, y);
				if (_animate) Sleep(150);

				// we try all the directions except the one we came from
				if (!(newX == oldX && newY == oldY)) {
					oldX = x;
					oldY = y;
					// remember where we were
					queue.push({ x, y });
					// then go to the new place
					x = newX;
					y = newY;
					break;
				}

			}
		}
		// put our solution in a form compatible with the recursive one
		solution.clear();
		while (!queue.empty()) {
			solution.push_front(queue.front());
			queue.pop();
		}
		return solved;
	}



	// 
	void DisplaySolution() {
		Position(0, YOFFSET + height + 3);
		for (std::list<COORD>::reverse_iterator solIterator = solution.rbegin(); solIterator != solution.rend(); ++solIterator) {
			std::cout << "(" << solIterator->X << "," << solIterator->Y << ") ";
		}
		std::cout << std::endl;

		// if we animated the maze, leave original on the screen and draw solution below
		if (_animate) yoffset += YOFFSET + height + 6;
		// now animate it

		std::cout << "Replay just the correct solution: " << std::endl;
		maze = saveMaze;	// back to original
		DisplayMaze();
		for (std::list<COORD>::reverse_iterator solIterator = solution.rbegin(); solIterator != solution.rend(); ++solIterator) {
			int x = solIterator->X;
			int y = solIterator->Y;
			maze[y][x] = '+';
			UpdateDisplay(x, y);
			Sleep(200);
		}
		Position(0, yoffset + height + 2);
	}
};


int main(int argc, char *argv[])
{
	bool interactive = false, animate = false;
	bool recursive = false, stack = false, queue = false;
	if (argc < 2) {
		std::cout << "Usage: " << argv[0] << " mazefile [-i]" << std::endl;
		std::cout << " -r for recursive solution (default)" << std::endl;
		std::cout << " -s for Stack solution" << std::endl;
		std::cout << " -q for Queue solution" << std::endl;
		std::cout << " -i for interactive manual solution" << std::endl;
		std::cout << " -a to animate the solution (0.2 seconds per move)" << std::endl;
		exit(1);
	}

	std::ifstream mazeFile;
	// parse command line
	for (int i = 1; i != argc; i++) {
		char *arg = argv[i];
		// command flag?
		if ('-' == *arg) {
			if (tolower(arg[1]) == 'i') interactive = true;
			if (tolower(arg[1]) == 'a') animate = true;
			if (tolower(arg[1]) == 'r') recursive = true;
			if (tolower(arg[1]) == 's') stack = true;
			if (tolower(arg[1]) == 'q') queue = true;
		}
		else {
			mazeFile.open(arg, std::ifstream::in);
			if (mazeFile.fail()) {
				std::cout << argv[0] << ": can't open maze file " << arg << std::endl;
				exit(1);
			}
		}
	}

	Maze *maze = new Maze(animate);
	int x, y;
	bool solved = false;

	maze->ReadMaze(mazeFile);
	maze->DisplayMaze();

	if (!maze->FindStart(x, y)) {
		std::cout << "Maze has no starting point!" << std::endl;
		exit(1);
	}

	if (!interactive) {
		if (recursive || (!stack && !queue)) solved = maze->SolveRecursive(-1, -1, x, y);
		if (stack) solved = maze->StackSolve(x, y);
		if (queue) solved = maze->QueueSolve(x, y);
		if (queue) {}; // TBD
	}
	else {
		// My interactive game for manual play.
		// Not part of the assignment!
		while (!maze->Walk(x, y));
		solved = true;
	}

	if (solved) {
		Beep(1800, 300);
		maze->Message("You win!");
		maze->DisplaySolution();
	}
	else {
		Beep(800, 500);
		maze->Message("No solution!");
	}
	system("pause");
	exit(0);
}

