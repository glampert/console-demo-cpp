#include "Screen.h"
#include <cassert>
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

	bool playerMoveIsValid = false;
	int playerMoveRow = -1, playerMoveColumn = -1;

	const char playerCharacter = 'X';
	const char aiCharacter = 'O';

	// 0=player, 1=AI
	bool winner[2] = { false, false };

	// Blank space = empty
	char boardValues[3][3] = {
		{ ' ', ' ', ' ' },
		{ ' ', ' ', ' ' },
		{ ' ', ' ', ' ' },
	};

	Colour boardColours[3][3] = {};

	for (;;)
	{
		if (playerMoveIsValid)
		{
			std::cout << "AI makes a move...\n";
			console::Wait(1500);

			// Set player move
			boardValues[playerMoveRow][playerMoveColumn]  = playerCharacter;
			boardColours[playerMoveRow][playerMoveColumn] = Colour::BrightRed;

			// Very simple "random" AI: just selects randomly any empty cell to make its move
			bool aiMadeValidMove = false;
			while (!aiMadeValidMove)
			{
				int aiMoveRow    = std::rand() % 3;
				int aiMoveColumn = std::rand() % 3;

				assert(aiMoveRow    <= 2);
				assert(aiMoveColumn <= 2);

				if (boardValues[aiMoveRow][aiMoveColumn] == ' ')
				{
					boardValues[aiMoveRow][aiMoveColumn]  = aiCharacter;
					boardColours[aiMoveRow][aiMoveColumn] = Colour::BrightBlue;
					aiMadeValidMove = true;
				}
			}

			// 0=player, 1=AI
			const char charsToCheck[2] = { playerCharacter, aiCharacter };

			// Check if there is a winner
			for (int j = 0; j < 2; j++)
			{
				for (int i = 0; i < 3; i++)
				{
					// Vertical lines
					if ((boardValues[i][0] == charsToCheck[j]) && (boardValues[i][1] == charsToCheck[j]) && (boardValues[i][2] == charsToCheck[j]))
					{
						winner[j] = true;
					}
					// Horizontal lines
					if ((boardValues[0][i] == charsToCheck[j]) && (boardValues[1][i] == charsToCheck[j]) && (boardValues[2][i] == charsToCheck[j]))
					{
						winner[j] = true;
					}
				}

				// Diagonals
				if ((boardValues[0][0] == charsToCheck[j]) && (boardValues[1][1] == charsToCheck[j]) && (boardValues[2][2] == charsToCheck[j]))
				{
					winner[j] = true;
				}
				if ((boardValues[0][2] == charsToCheck[j]) && (boardValues[1][1] == charsToCheck[j]) && (boardValues[2][0] == charsToCheck[j]))
				{
					winner[j] = true;
				}

				// We can early out if player or AI won
				if (winner[j])
				{
					break;
				}
			}
		}

		screen.Clear();

		// Draw and display the board
		DrawTicTacToeBoard(screen, 1, 7, boardValues, boardColours);
		screen.Present();

		bool restartGame = false;
		if (winner[0]) // player won
		{
			std::cout << "CONGRATULATION, YOU WON!\n";
			console::Wait(1500);
			restartGame = true;
		}
		else if (winner[1]) // ai won
		{
			std::cout << "AI WINS!\n";
			console::Wait(1500);
			restartGame = true;
		}
		else // check for tie
		{
			bool hasEmptyCell = false;
			for (int x = 0; x < 3; x++)
			{
				for (int y = 0; y < 3; y++)
				{
					if (boardValues[x][y] == ' ')
					{
						hasEmptyCell = true;
						break;
					}
				}
			}

			if (!hasEmptyCell)
			{
				std::cout << "TIE GAME!\n";
				console::Wait(1500);
				restartGame = true;
			}
		}

		if (restartGame)
		{
			// Restart the game once the player or AI have won (reset all states)
			playerMoveIsValid = false;
			playerMoveRow = -1;
			playerMoveColumn = -1;

			winner[0] = false;
			winner[1] = false;

			for (int x = 0; x < 3; x++)
			{
				for (int y = 0; y < 3; y++)
				{
					boardValues[x][y]  = ' ';
					boardColours[x][y] = Colour::White;
				}
			}
		}

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

		playerMoveRow    = inputRowAndColumn[0];
		playerMoveColumn = inputRowAndColumn[1];

		// Validate the inputs
		if (playerMoveRow < 0 || playerMoveRow > 2)
		{
			std::cout << "Invalid row! Try again.\n";
			playerMoveIsValid = false;
		}
		else if (playerMoveColumn < 0 || playerMoveColumn > 2)
		{
			std::cout << "Invalid column! Try again.\n";
			playerMoveIsValid = false;
		}
		else
		{
			std::cout << "Your move is: " << playerMoveRow << "," << playerMoveColumn << "\n";
			playerMoveIsValid = true;
		}

		// Sleep for a few milliseconds
		console::Wait(1500);
	}

	return 0;
}
