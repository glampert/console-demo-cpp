#pragma once

#include <cstdint>
#include <string>
#include <memory>

namespace console
{

struct Point
{
	int x = 0;
	int y = 0;
	int z = 0;
};

enum class LineStyle : std::uint8_t
{
	Default,
	Double,
};

enum class FillMode : std::uint8_t
{
	Outline,
	Solid,
	Dither1,
	Dither2,
	Dither3,
};

struct Rectangle
{
	Point origin;

	int width  = 0;
	int height = 0;

	LineStyle border = LineStyle::Default;
	FillMode  fill   = FillMode::Outline;
};

struct Line
{
	Point start;
	Point end;

	LineStyle style = LineStyle::Default;
};

struct Colour
{
	std::uint8_t r = 0;
	std::uint8_t g = 0;
	std::uint8_t b = 0;

	static const Colour Black;
	static const Colour White;
	static const Colour Gray;
	static const Colour BrightRed;
	static const Colour BrightGreen;
	static const Colour BrightBlue;
	static const Colour DarkRed;
	static const Colour DarkGreen;
	static const Colour DarkBlue;
};

// Helper class to draw characters, strings and simple geometric shaped to the console screen.
// All draws are buffered until Present() is called.
class Screen final
{
public:

	Screen(const char* title, const int width, const int height);

	// Presents all draws to the console screen.
	void Present();

	// Clears the screen.
	void Clear();

	// Draw single ASCII character.
	void DrawChar(const std::uint8_t ch, const Point& position, const Colour foreground, const Colour background);

	// Draw string (handles newlines '\n' and tabs '\t').
	void DrawText(const char* text, const Point& position, const Colour foreground, const Colour background);
	void DrawText(const std::string& text, const Point& position, const Colour foreground, const Colour background);

	// Draw shapes.
	void DrawRectangle(const Rectangle& rect, const Colour foreground, const Colour background);
	void DrawLine(Line line, const Colour foreground, const Colour background);

	// Check if point inside the screen bounds.
	bool IsWithinBounds(const Point& position) const;

	int Width()  const;
	int Height() const;

private:

	struct Impl;
	const std::shared_ptr<Impl> m_pImpl;
};

void Wait(unsigned int milliseconds);
void DrawDemo(Screen& screen);

} // namespace console
