#ifndef PIXELTEXT_HPP
#define PIXELTEXT_HPP

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <touchgfx/Color.hpp>
#include <touchgfx/hal/HAL.hpp>
#include <touchgfx/lcd/LCD.hpp>
#include <touchgfx/widgets/Widget.hpp>

class PixelText : public touchgfx::Widget
{
public:
    PixelText()
        : text(""), color(touchgfx::Color::getColorFromRGB(255, 255, 255)), scale(2)
    {
    }

    void setText(const char* newText)
    {
        invalidate();
        text = newText ? newText : "";
        updateSize();
        invalidate();
    }

    void setColor(touchgfx::colortype newColor)
    {
        invalidate();
        color = newColor;
        invalidate();
    }

    void setScale(uint8_t newScale)
    {
        invalidate();
        scale = newScale ? newScale : 1;
        updateSize();
        invalidate();
    }

    virtual touchgfx::Rect getSolidRect() const
    {
        return touchgfx::Rect();
    }

    virtual void draw(const touchgfx::Rect& area) const
    {
        const int16_t cell = 6 * scale;
        int16_t cursorX = 0;

        for (const char* p = text; *p; ++p)
        {
            const char ch = normalize(*p);
            if (ch != ' ')
            {
                const uint8_t* rows = glyph(ch);
                for (uint8_t row = 0; row < 7; row++)
                {
                    for (uint8_t col = 0; col < 5; col++)
                    {
                        if (rows[row] & (1U << (4 - col)))
                        {
                            touchgfx::Rect pixel(cursorX + col * scale, row * scale, scale, scale);
                            if (pixel.intersect(area))
                            {
                                pixel &= area;
                                translateRectToAbsolute(pixel);
                                touchgfx::HAL::lcd().fillRect(pixel, color);
                            }
                        }
                    }
                }
            }
            cursorX += cell;
        }
    }

private:
    const char* text;
    touchgfx::colortype color;
    uint8_t scale;

    void updateSize()
    {
        const uint16_t length = static_cast<uint16_t>(std::strlen(text));
        setWidth(length ? static_cast<uint16_t>((length * 6 - 1) * scale) : 0);
        setHeight(static_cast<uint16_t>(7 * scale));
    }

    static char normalize(char ch)
    {
        if (ch >= 'a' && ch <= 'z')
        {
            return static_cast<char>(ch - 'a' + 'A');
        }
        return ch;
    }

    static const uint8_t* glyph(char ch)
    {
        static const uint8_t blank[7] = { 0, 0, 0, 0, 0, 0, 0 };
        static const uint8_t unknown[7] = { 0x0E, 0x11, 0x01, 0x02, 0x04, 0x00, 0x04 };

        switch (ch)
        {
        case 'A': { static const uint8_t g[7] = { 0x0E, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11 }; return g; }
        case 'B': { static const uint8_t g[7] = { 0x1E, 0x11, 0x11, 0x1E, 0x11, 0x11, 0x1E }; return g; }
        case 'C': { static const uint8_t g[7] = { 0x0E, 0x11, 0x10, 0x10, 0x10, 0x11, 0x0E }; return g; }
        case 'D': { static const uint8_t g[7] = { 0x1E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x1E }; return g; }
        case 'E': { static const uint8_t g[7] = { 0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x1F }; return g; }
        case 'F': { static const uint8_t g[7] = { 0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x10 }; return g; }
        case 'G': { static const uint8_t g[7] = { 0x0E, 0x11, 0x10, 0x17, 0x11, 0x11, 0x0F }; return g; }
        case 'H': { static const uint8_t g[7] = { 0x11, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11 }; return g; }
        case 'I': { static const uint8_t g[7] = { 0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x1F }; return g; }
        case 'J': { static const uint8_t g[7] = { 0x01, 0x01, 0x01, 0x01, 0x11, 0x11, 0x0E }; return g; }
        case 'K': { static const uint8_t g[7] = { 0x11, 0x12, 0x14, 0x18, 0x14, 0x12, 0x11 }; return g; }
        case 'L': { static const uint8_t g[7] = { 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1F }; return g; }
        case 'M': { static const uint8_t g[7] = { 0x11, 0x1B, 0x15, 0x15, 0x11, 0x11, 0x11 }; return g; }
        case 'N': { static const uint8_t g[7] = { 0x11, 0x19, 0x15, 0x13, 0x11, 0x11, 0x11 }; return g; }
        case 'O': { static const uint8_t g[7] = { 0x0E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E }; return g; }
        case 'P': { static const uint8_t g[7] = { 0x1E, 0x11, 0x11, 0x1E, 0x10, 0x10, 0x10 }; return g; }
        case 'Q': { static const uint8_t g[7] = { 0x0E, 0x11, 0x11, 0x11, 0x15, 0x12, 0x0D }; return g; }
        case 'R': { static const uint8_t g[7] = { 0x1E, 0x11, 0x11, 0x1E, 0x14, 0x12, 0x11 }; return g; }
        case 'S': { static const uint8_t g[7] = { 0x0F, 0x10, 0x10, 0x0E, 0x01, 0x01, 0x1E }; return g; }
        case 'T': { static const uint8_t g[7] = { 0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04 }; return g; }
        case 'U': { static const uint8_t g[7] = { 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E }; return g; }
        case 'V': { static const uint8_t g[7] = { 0x11, 0x11, 0x11, 0x11, 0x11, 0x0A, 0x04 }; return g; }
        case 'W': { static const uint8_t g[7] = { 0x11, 0x11, 0x11, 0x15, 0x15, 0x15, 0x0A }; return g; }
        case 'X': { static const uint8_t g[7] = { 0x11, 0x11, 0x0A, 0x04, 0x0A, 0x11, 0x11 }; return g; }
        case 'Y': { static const uint8_t g[7] = { 0x11, 0x11, 0x0A, 0x04, 0x04, 0x04, 0x04 }; return g; }
        case 'Z': { static const uint8_t g[7] = { 0x1F, 0x01, 0x02, 0x04, 0x08, 0x10, 0x1F }; return g; }
        case '0': { static const uint8_t g[7] = { 0x0E, 0x11, 0x13, 0x15, 0x19, 0x11, 0x0E }; return g; }
        case '1': { static const uint8_t g[7] = { 0x04, 0x0C, 0x04, 0x04, 0x04, 0x04, 0x0E }; return g; }
        case '2': { static const uint8_t g[7] = { 0x0E, 0x11, 0x01, 0x02, 0x04, 0x08, 0x1F }; return g; }
        case '3': { static const uint8_t g[7] = { 0x1E, 0x01, 0x01, 0x0E, 0x01, 0x01, 0x1E }; return g; }
        case '4': { static const uint8_t g[7] = { 0x02, 0x06, 0x0A, 0x12, 0x1F, 0x02, 0x02 }; return g; }
        case '5': { static const uint8_t g[7] = { 0x1F, 0x10, 0x10, 0x1E, 0x01, 0x11, 0x0E }; return g; }
        case '6': { static const uint8_t g[7] = { 0x06, 0x08, 0x10, 0x1E, 0x11, 0x11, 0x0E }; return g; }
        case '7': { static const uint8_t g[7] = { 0x1F, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08 }; return g; }
        case '8': { static const uint8_t g[7] = { 0x0E, 0x11, 0x11, 0x0E, 0x11, 0x11, 0x0E }; return g; }
        case '9': { static const uint8_t g[7] = { 0x0E, 0x11, 0x11, 0x0F, 0x01, 0x02, 0x0C }; return g; }
        case '.': { static const uint8_t g[7] = { 0, 0, 0, 0, 0, 0x0C, 0x0C }; return g; }
        case ':': { static const uint8_t g[7] = { 0, 0x0C, 0x0C, 0, 0x0C, 0x0C, 0 }; return g; }
        case '%': { static const uint8_t g[7] = { 0x18, 0x19, 0x02, 0x04, 0x08, 0x13, 0x03 }; return g; }
        case '/': { static const uint8_t g[7] = { 0x01, 0x01, 0x02, 0x04, 0x08, 0x10, 0x10 }; return g; }
        case '-': { static const uint8_t g[7] = { 0, 0, 0, 0x1F, 0, 0, 0 }; return g; }
        case '+': { static const uint8_t g[7] = { 0, 0x04, 0x04, 0x1F, 0x04, 0x04, 0 }; return g; }
        case ' ': return blank;
        default: return unknown;
        }
    }
};

#endif // PIXELTEXT_HPP
