#include "Screen.h"
#include <sstream>
#include <iostream>
using namespace console;

void DrawTicTacToeBoard(Screen& screen, const int x, const int y, const char boardValues[3][3], const Colour boardColours[3][3])
{
	// Top-side numbers
	screen.DrawChar('0', Point{ x + 2,  y }, Colour::White, Colour::Black);
	screen.DrawChar('1', Point{ x + 6,  y }, Colour::White, Colour::Black);
	screen.DrawChar('2', Point{ x + 10, y }, Colour::White, Colour::Black);

	// Right-hand side numbers
	screen.DrawChar('0', Point{ x + 14,  y + 2 }, Colour::White, Colour::Black);
	screen.DrawChar('1', Point{ x + 14,  y + 4 }, Colour::White, Colour::Black);
	screen.DrawChar('2', Point{ x + 14,  y + 6 }, Colour::White, Colour::Black);

	// Outline box
	screen.DrawRectangle(Rectangle{ { x, y + 1 }, 12, 13, LineStyle::Double }, Colour::White, Colour::Black);

	// Vertical lines
	screen.DrawLine(Line{ { x + 4, y + 2 }, { x + 4, y + 13 }, LineStyle::Double }, Colour::White, Colour::Black);
	screen.DrawLine(Line{ { x + 8, y + 2 }, { x + 8, y + 13 }, LineStyle::Double }, Colour::White, Colour::Black);

	// Horizontal lines
	screen.DrawLine(Line{ { x + 1, y + 3 }, { x + 12, y + 3 }, LineStyle::Double }, Colour::White, Colour::Black);
	screen.DrawLine(Line{ { x + 1, y + 5 }, { x + 12, y + 5 }, LineStyle::Double }, Colour::White, Colour::Black);

	// Row[0] (top)
	screen.DrawChar(boardValues[0][0], Point{ x + 2,  y + 2 }, boardColours[0][0], Colour::Black);
	screen.DrawChar(boardValues[0][1], Point{ x + 6,  y + 2 }, boardColours[0][1], Colour::Black);
	screen.DrawChar(boardValues[0][2], Point{ x + 10, y + 2 }, boardColours[0][2], Colour::Black);

	// Row[1] (middle)
	screen.DrawChar(boardValues[1][0], Point{ x + 2,  y + 4 }, boardColours[1][0], Colour::Black);
	screen.DrawChar(boardValues[1][1], Point{ x + 6,  y + 4 }, boardColours[1][1], Colour::Black);
	screen.DrawChar(boardValues[1][2], Point{ x + 10, y + 4 }, boardColours[1][2], Colour::Black);

	// Row[2] (bottom)
	screen.DrawChar(boardValues[2][0], Point{ x + 2,  y + 6 }, boardColours[2][0], Colour::Black);
	screen.DrawChar(boardValues[2][1], Point{ x + 6,  y + 6 }, boardColours[2][1], Colour::Black);
	screen.DrawChar(boardValues[2][2], Point{ x + 10, y + 6 }, boardColours[2][2], Colour::Black);
}

int main()
{
	Screen screen{ "Console Tic-Tac-Toe", 64, 32 };

	bool userMoveIsValid = false;
	int row = -1, column = -1;

	for (;;)
	{
		if (userMoveIsValid)
		{
			std::cout << "AI makes a move...\n";
			console::Wait(1500);

			// TODO: Implement the computer AI and check for win/lose/draw conditions.
			// - Update the board accordingly.
		}

		screen.Clear();

		char boardValues[3][3] = {
			{ 'X', 'O', 'O' },
			{ 'O', 'X', 'O' },
			{ 'O', 'O', 'X' },
		};

		Colour boardColours[3][3] = {
			{ Colour::BrightRed, Colour::White,     Colour::White     },
			{ Colour::White,     Colour::BrightRed, Colour::White     },
			{ Colour::White,     Colour::White,     Colour::BrightRed },
		};

		// Draw and display the board
		DrawTicTacToeBoard(screen, 1, 7, boardValues, boardColours);
		screen.Present();

		// Header text
		std::cout << "Enter row and column for your move\n"
		             "(separated by a comma, e.g.: 0,1) or 'exit' to quit.\n\n";
		std::cout << "> ";

		// Wait for user input
		std::string input;
		std::cin >> input;

		// Quit if "exit" was typed
		if (input == "exit")
		{
			break;
		}

		// Parse the row and column number entered by the user
		int inputRowAndColumn[2] = { -1, -1 };
		std::stringstream parser{ input };

		for (int i = 0; (i < 2 && parser.good()); ++i)
		{
			std::string subString;
			std::getline(parser, subString, ',');

			try {
				inputRowAndColumn[i] = std::stoi(subString);
			}
			catch (...) {}
		}

		row    = inputRowAndColumn[0];
		column = inputRowAndColumn[1];

		// Validate the inputs
		if (row < 0 || row > 2)
		{
			std::cout << "Invalid row! Try again.\n";
			userMoveIsValid = false;
		}
		else if (column < 0 || column > 2)
		{
			std::cout << "Invalid column! Try again.\n";
			userMoveIsValid = false;
		}
		else
		{
			std::cout << "Your move is: " << row << "," << column << "\n";
			userMoveIsValid = true;
		}

		// Sleep for a few milliseconds
		console::Wait(1500);
	}

	return 0;
}
