#include "Screen.h"

#include <cassert>
#include <cstdlib>
#include <algorithm>
#include <vector>

#define NOUSER   // Suppress DrawTextA|W macro
#define NOGDI    // Suppress Rectangle() function
#define NOMINMAX // Suppress min/max macros
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <wincon.h>

namespace console
{

const Colour Colour::Black       = { 0,   0,   0   };
const Colour Colour::White       = { 255, 255, 255 };
const Colour Colour::Gray        = { 128, 128, 128 };
const Colour Colour::BrightRed   = { 255, 0,   0   };
const Colour Colour::BrightGreen = { 0,   255, 0   };
const Colour Colour::BrightBlue  = { 0,   0,   255 };
const Colour Colour::DarkRed     = { 128, 0,   0   };
const Colour Colour::DarkGreen   = { 0,   128, 0   };
const Colour Colour::DarkBlue    = { 0,   0,   128 };

struct Screen::Impl
{
	struct DrawEntry
	{
		std::uint16_t z       = 0xFF;
		std::uint16_t index   = 0;
		std::uint16_t attribs = 0;
		std::uint8_t  ch      = 0;
	};

	bool screenDirty = false;
	std::vector<DrawEntry> buffer;

	struct ConsoleState
	{
		HANDLE                 stdHandle;
		SMALL_RECT             windowRect;
		SMALL_RECT             writeArea;
		COORD                  characterBufferSize;
		COORD                  characterPosition;
		CONSOLE_CURSOR_INFO    cursorInfo;
		std::vector<CHAR_INFO> characterBuffer;
	} consoleState = {};

	Impl() = default;
	Impl(const Impl&) = delete;
	Impl& operator=(const Impl&) = delete;

	void AddCharToBuffer(const std::uint8_t ch, std::uint16_t x, const std::uint16_t y, const std::uint16_t z, const std::uint16_t attribs)
	{
		// Hack: For some reason drawing at 0,0 doesn't seem to work, so I'm scrolling the characterPosition to start at 1 (see below)
		// so we need to compensate here and add one to the x coordinate since x now starts at 1, not zero.
		++x;

		Impl::DrawEntry entry;
		entry.z       = z;
		entry.index   = static_cast<std::uint16_t>(x + (y * consoleState.characterBufferSize.X));
		entry.attribs = attribs;
		entry.ch      = ch;

		// Simple "depth test" and off-screen clipping
		if (entry.index < buffer.size() && entry.z <= buffer[entry.index].z)
		{
			buffer[entry.index] = entry;
		}
	}

	static std::uint16_t ColourToConsoleAttributes(const Colour colour, const int layer)
	{
		enum { Red, Green, Blue, Intensity };

		// layer: foreground=0, background=1
		static constexpr std::uint16_t flags[2][4] = {
			{ FOREGROUND_RED, FOREGROUND_GREEN, FOREGROUND_BLUE, FOREGROUND_INTENSITY },
			{ BACKGROUND_RED, BACKGROUND_GREEN, BACKGROUND_BLUE, BACKGROUND_INTENSITY },
		};

		std::uint16_t attribs = 0;

		// If channel is above 128, add intensity
		if (colour.r != 0)
		{
			attribs |= flags[layer][Red];
			if (colour.r > 128)
			{
				attribs |= flags[layer][Intensity];
			}
		}

		if (colour.g != 0)
		{
			attribs |= flags[layer][Green];
			if (colour.g > 128)
			{
				attribs |= flags[layer][Intensity];
			}
		}

		if (colour.b != 0)
		{
			attribs |= flags[layer][Blue];
			if (colour.b > 128)
			{
				attribs |= flags[layer][Intensity];
			}
		}

		return attribs;
	}
};

Screen::Screen(const char* title, const int width, const int height)
	: m_pImpl{ new Impl() }
{
	assert(title != nullptr);
	assert(width > 0 && height > 0);

	auto& impl = *m_pImpl;
	auto& consoleState = impl.consoleState;

	consoleState.stdHandle = GetStdHandle(STD_OUTPUT_HANDLE);
	assert(consoleState.stdHandle != nullptr && consoleState.stdHandle != INVALID_HANDLE_VALUE);

	CONSOLE_SCREEN_BUFFER_INFO consoleInfo = {};
	BOOL result = GetConsoleScreenBufferInfo(consoleState.stdHandle, &consoleInfo);
	assert(result == TRUE);

	// Clamp to max supported size
	const short consoleW = std::min(static_cast<short>(width),  consoleInfo.dwMaximumWindowSize.X);
	const short consoleH = std::min(static_cast<short>(height), consoleInfo.dwMaximumWindowSize.Y);
	const int consoleSize = consoleW * consoleH;

	consoleState.windowRect          = { 0, 0, consoleW - 1, consoleH - 1 };
	consoleState.writeArea           = consoleState.windowRect;
	consoleState.characterBufferSize = { consoleW, consoleH };
	consoleState.characterPosition   = { 1, 0 }; // Hack: start at x=1 since drawing a char at 0,0 doesn't seem to work...
	consoleState.cursorInfo.bVisible = FALSE;
	consoleState.cursorInfo.dwSize   = 1;

	impl.buffer.resize(consoleSize, Impl::DrawEntry{});
	consoleState.characterBuffer.resize(consoleSize, CHAR_INFO{});

	result = SetConsoleTitleA(title);
	assert(result == TRUE);

	result = SetConsoleWindowInfo(consoleState.stdHandle, TRUE, &consoleState.windowRect);
	assert(result == TRUE);

	result = SetConsoleScreenBufferSize(consoleState.stdHandle, consoleState.characterBufferSize);
	assert(result == TRUE);

	result = SetConsoleCursorInfo(consoleState.stdHandle, &consoleState.cursorInfo);
	assert(result == TRUE);
}

void Screen::Present()
{
	auto& impl = *m_pImpl;
	auto& consoleState = impl.consoleState;

	if (!impl.screenDirty)
	{
		return;
	}

	// Copy to console buffer and clear each entry
	for (Impl::DrawEntry& entry : impl.buffer)
	{
		CHAR_INFO& charInfo = consoleState.characterBuffer[entry.index];
		charInfo.Char.AsciiChar = static_cast<CHAR>(entry.ch);
		charInfo.Attributes = entry.attribs;

		entry = {};
	}

	const BOOL result = WriteConsoleOutputA(consoleState.stdHandle,
		consoleState.characterBuffer.data(),
		consoleState.characterBufferSize,
		consoleState.characterPosition,
		&consoleState.writeArea);
	assert(result == TRUE);

	impl.screenDirty = false;
}

void Screen::Clear()
{
	auto& impl = *m_pImpl;
	auto& consoleState = impl.consoleState;

	for (Impl::DrawEntry& entry : impl.buffer)
		entry = {};

	for (CHAR_INFO& ci : consoleState.characterBuffer)
		ci = {};

	// In case stdio is also used just do a system cls for now.
	std::system("cls");

	impl.screenDirty = false;
}

void Screen::DrawChar(const std::uint8_t ch, const Point& position, const Colour foreground, const Colour background)
{
	if (!IsWithinBounds(position))
	{
		return;
	}

	auto& impl = *m_pImpl;
	impl.screenDirty = true;

	const std::uint16_t attribs = Impl::ColourToConsoleAttributes(foreground, 0) | Impl::ColourToConsoleAttributes(background, 1);

	impl.AddCharToBuffer(ch,
		static_cast<std::uint16_t>(position.x),
		static_cast<std::uint16_t>(position.y),
		static_cast<std::uint16_t>(position.z),
		attribs);
}

void Screen::DrawText(const char* text, const Point& position, const Colour foreground, const Colour background)
{
	assert(text != nullptr);

	if (!IsWithinBounds(position))
	{
		return;
	}

	auto& impl = *m_pImpl;
	impl.screenDirty = true;

	const std::uint16_t attribs = Impl::ColourToConsoleAttributes(foreground, 0) | Impl::ColourToConsoleAttributes(background, 1);
	auto x = static_cast<std::uint16_t>(position.x);
	auto y = static_cast<std::uint16_t>(position.y);
	auto z = static_cast<std::uint16_t>(position.z);

	for (int i = 0; text[i] != '\0'; ++i)
	{
		const std::uint8_t ch = text[i];

		// Handle escape characters
		switch (ch)
		{
		case '\n':
			x = static_cast<std::uint16_t>(position.x);
			++y;
			break;

		case '\t':
			for (int tab = 0; tab < 4; ++tab)
			{
				impl.AddCharToBuffer(' ', x++, y, z, attribs);
			}
			break;

		default:
			impl.AddCharToBuffer(ch, x++, y, z, attribs);
			break;
		}
	}
}

void Screen::DrawText(const std::string& text, const Point& position, const Colour foreground, const Colour background)
{
	DrawText(text.c_str(), position, foreground, background);
}

void Screen::DrawRectangle(const Rectangle& rect, const Colour foreground, const Colour background)
{
	if (!IsWithinBounds(rect.origin))
	{
		return;
	}

	auto& impl = *m_pImpl;
	impl.screenDirty = true;

	static constexpr std::uint8_t fills[5] = {
		219, // Outline
		219, // Solid
		178, // Dither1
		177, // Dither2
		176, // Dither3
	};

	const std::uint16_t attribs = Impl::ColourToConsoleAttributes(foreground, 0) | Impl::ColourToConsoleAttributes(background, 1);
	const auto fill = static_cast<int>(rect.fill);

	auto x = static_cast<std::uint16_t>(rect.origin.x);
	auto y = static_cast<std::uint16_t>(rect.origin.y);
	auto z = static_cast<std::uint16_t>(rect.origin.z);

	// Special case: handle a 1x1 rectangle as a single filled char
	if (rect.width == 1 && rect.height == 1)
	{
		impl.AddCharToBuffer(fills[fill], x, y, z, attribs);
		return;
	}

	const auto w = static_cast<std::uint16_t>(rect.origin.x + rect.width);
	const auto h = static_cast<std::uint16_t>(rect.origin.y + (rect.height / 2)); // Hack: Divide height by 2 since console char height is about twice the width

	if (rect.fill == FillMode::Outline)
	{
		// horizontal/vertical lines and corners
		static constexpr std::uint8_t borders[2][8] = {
			{ 218, 196,  191, 179,  217, 196,  192, 179 }, // Default
			{ 201, 205,  187, 186,  188, 205,  200, 186 }, // Double
		};

		const auto border = static_cast<int>(rect.border);

		int i = 0;
		int b = 0;

		// top
		for (i = 0; x < w; ++x, ++i)
		{
			impl.AddCharToBuffer(borders[border][b + (i != 0)], x, y, z, attribs);
		}
		b += 2;

		// right
		for (i = 0; y < h; ++y, ++i)
		{
			impl.AddCharToBuffer(borders[border][b + (i != 0)], x, y, z, attribs);
		}
		b += 2;

		// bottom
		for (i = 0; x > rect.origin.x; --x, ++i)
		{
			impl.AddCharToBuffer(borders[border][b + (i != 0)], x, y, z, attribs);
		}
		b += 2;

		// left
		for (i = 0; y > rect.origin.y; --y, ++i)
		{
			impl.AddCharToBuffer(borders[border][b + (i != 0)], x, y, z, attribs);
		}
		b += 2;
	}
	else // Solid / Dither
	{
		for (std::uint16_t xi = x; xi < w; ++xi)
		{
			for (std::uint16_t yi = y; yi < h; ++yi)
			{
				impl.AddCharToBuffer(fills[fill], xi, yi, z, attribs);
			}
		}
	}
}

void Screen::DrawLine(Line line, const Colour foreground, const Colour background)
{
	// Out of bounds start point?
	if (line.start.x > Width() || line.start.y > Height() || line.start.z < 0)
	{
		return;
	}

	// Invalid end point?
	if (line.end.x < 0 || line.end.x < line.start.x ||
		line.end.y < 0 || line.end.y < line.start.y)
	{
		return;
	}

	// Clamp start/end
	if (line.start.x < 0) { line.start.x = 0; }
	if (line.start.y < 0) { line.start.y = 0; }

	if (line.end.x > Width())  { line.end.x = Width();  }
	if (line.end.y > Height()) { line.end.y = Height(); }

	auto& impl = *m_pImpl;
	impl.screenDirty = true;

	const std::uint16_t attribs = Impl::ColourToConsoleAttributes(foreground, 0) | Impl::ColourToConsoleAttributes(background, 1);
	const auto lineStyle = static_cast<int>(line.style);

	static constexpr std::uint8_t lines[2][2] = {
		// horizontal, vertical
		{ 196,         179 }, // Default
		{ 205,         186 }, // Double
	};

	auto x = static_cast<std::uint16_t>(line.start.x);
	auto y = static_cast<std::uint16_t>(line.start.y);
	auto z = static_cast<std::uint16_t>(line.start.z);

	const auto w = (line.end.x - line.start.x);
	const auto h = (line.end.y - line.start.y) / 2; // Hack: Divide height by 2 since console char height is about twice the width

	// horizontal
	if (w > 1)
	{
		for (int i = 0; i < w; ++i)
		{
			impl.AddCharToBuffer(lines[lineStyle][0], x++, y, z, attribs);
		}
	}

	// vertical
	if (h > 1)
	{
		for (int i = 0; i < h; ++i)
		{
			impl.AddCharToBuffer(lines[lineStyle][1], x, y++, z, attribs);
		}
	}
}

bool Screen::IsWithinBounds(const Point& position) const
{
	if (position.x < 0 || position.y < 0 || position.z < 0)
	{
		return false;
	}

	if (position.x > Width() || position.y > Height())
	{
		return false;
	}

	return true;
}

int Screen::Width() const
{
	return m_pImpl->consoleState.characterBufferSize.X;
}

int Screen::Height() const
{
	return m_pImpl->consoleState.characterBufferSize.Y;
}

void Wait(unsigned int milliseconds)
{
	::Sleep(milliseconds);
}

void DrawDemo(Screen& screen)
{
	screen.DrawText("Console Drawing Demo.", Point{ 10, 1 }, Colour{ 255, 0, 255 }, Colour::Gray);

	screen.DrawChar('X', Point{ 0, 0 }, Colour::White, Colour::BrightGreen);
	screen.DrawChar('Y', Point{ 1, 0 }, Colour::White, Colour::DarkBlue);
	screen.DrawChar('Z', Point{ 0, 1 }, Colour::White, Colour::DarkBlue);
	screen.DrawChar('X', Point{ 1, 1 }, Colour::White, Colour::DarkGreen);

	// Different depth (z) value (B draws over A: 1 < 5)
	screen.DrawChar('B', Point{ 3, 1, 1 }, Colour::White, Colour::DarkRed);
	screen.DrawChar('A', Point{ 3, 1, 5 }, Colour::BrightGreen, Colour::White);

	screen.DrawChar('X', Point{ 5, 1 }, Colour::BrightRed, Colour::White);
	screen.DrawChar('X', Point{ 6, 1 }, Colour::DarkRed, Colour::Gray);

	screen.DrawChar('Y', Point{ 5, 2 }, Colour::BrightGreen, Colour::White);
	screen.DrawChar('Y', Point{ 6, 2 }, Colour::DarkGreen, Colour::Gray);

	screen.DrawChar('Z', Point{ 5, 3 }, Colour::BrightBlue, Colour::White);
	screen.DrawChar('Z', Point{ 6, 3 }, Colour::DarkBlue, Colour::Gray);

	// Extended ASCII characters (dithered rectangles)
	screen.DrawChar(178, Point{ 8, 20 }, Colour::White, Colour::Black);
	screen.DrawChar(177, Point{ 8, 21 }, Colour::White, Colour::Black);
	screen.DrawChar(176, Point{ 8, 22 }, Colour::White, Colour::Black);

	// Text with newlines and tabs
	screen.DrawText("Line 1\nLine 2\tcontinues.", Point{ 8, 3 }, Colour::BrightRed, Colour::DarkGreen);

	// Lines
	screen.DrawLine(Line{ { 30, 3 }, { 36, 3 }, LineStyle::Default }, Colour::BrightRed,  Colour::Black);
	screen.DrawLine(Line{ { 30, 4 }, { 30, 8 }, LineStyle::Default }, Colour::BrightBlue, Colour::Black);

	screen.DrawLine(Line{ { 30, 6 }, { 36, 6 }, LineStyle::Double }, Colour::BrightRed,  Colour::Black);
	screen.DrawLine(Line{ { 35, 4 }, { 35, 8 }, LineStyle::Double }, Colour::BrightBlue, Colour::Black);

	// Rectangles
	screen.DrawRectangle(Rectangle{ { 15, 10 }, 1, 1,   LineStyle::Default }, Colour::White, Colour::Black);
	screen.DrawRectangle(Rectangle{ { 15, 11 }, 2, 2,   LineStyle::Default }, Colour::White, Colour::Black);
	screen.DrawRectangle(Rectangle{ { 15, 14 }, 10, 10, LineStyle::Default }, Colour::White, Colour::Black);

	screen.DrawRectangle(Rectangle{ { 26, 10 }, 1, 1,   LineStyle::Double }, Colour::White, Colour::Black);
	screen.DrawRectangle(Rectangle{ { 26, 11 }, 2, 2,   LineStyle::Double }, Colour::White, Colour::Black);
	screen.DrawRectangle(Rectangle{ { 26, 14 }, 10, 10, LineStyle::Double }, Colour::White, Colour::Black);

	screen.DrawRectangle(Rectangle{ { 15, 20 }, 1, 1,   LineStyle::Double, FillMode::Solid }, Colour::White, Colour::Black);
	screen.DrawRectangle(Rectangle{ { 15, 22 }, 2, 2,   LineStyle::Double, FillMode::Solid }, Colour::White, Colour::Black);
	screen.DrawRectangle(Rectangle{ { 15, 24 }, 10, 10, LineStyle::Double, FillMode::Solid }, Colour::White, Colour::Black);

	screen.DrawRectangle(Rectangle{ { 26, 20 }, 1, 1,   LineStyle::Default, FillMode::Dither1 }, Colour::White, Colour::Black);
	screen.DrawRectangle(Rectangle{ { 26, 22 }, 2, 2,   LineStyle::Default, FillMode::Dither1 }, Colour::White, Colour::Black);
	screen.DrawRectangle(Rectangle{ { 26, 24 }, 10, 10, LineStyle::Default, FillMode::Dither1 }, Colour::White, Colour::Black);

	screen.DrawRectangle(Rectangle{ { 37, 20 }, 1, 1,   LineStyle::Default, FillMode::Dither2 }, Colour::White, Colour::Black);
	screen.DrawRectangle(Rectangle{ { 37, 22 }, 2, 2,   LineStyle::Default, FillMode::Dither2 }, Colour::White, Colour::Black);
	screen.DrawRectangle(Rectangle{ { 37, 24 }, 10, 10, LineStyle::Default, FillMode::Dither2 }, Colour::White, Colour::Black);

	screen.DrawRectangle(Rectangle{ { 48, 20 }, 1, 1,   LineStyle::Default, FillMode::Dither3 }, Colour::White, Colour::Black);
	screen.DrawRectangle(Rectangle{ { 48, 22 }, 2, 2,   LineStyle::Default, FillMode::Dither3 }, Colour::White, Colour::Black);
	screen.DrawRectangle(Rectangle{ { 48, 24 }, 10, 10, LineStyle::Default, FillMode::Dither3 }, Colour::White, Colour::Black);
}

} // namespace console
