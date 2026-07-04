#ifndef PIXELTEXT_HPP
#define PIXELTEXT_HPP

#include <touchgfx/hal/HAL.hpp>
#include <touchgfx/lcd/LCD.hpp>
#include <touchgfx/widgets/Widget.hpp>
#include <stdint.h>

namespace WatchUi
{
class PixelText : public touchgfx::Widget
{
public:
    PixelText()
        : text(""), color(0xFFFF), alpha(255), scale(1), alignment(touchgfx::LEFT)
    {
    }

    void setText(const char* value)
    {
        text = value ? value : "";
    }

    void setColor(touchgfx::colortype value)
    {
        color = value;
    }

    void setAlpha(uint8_t value)
    {
        alpha = value;
    }

    void setScale(uint8_t value)
    {
        scale = value == 0 ? 1 : value;
    }

    void setAlignment(touchgfx::Alignment value)
    {
        alignment = value;
    }

    virtual touchgfx::Rect getSolidRect() const
    {
        return touchgfx::Rect();
    }

    virtual void draw(const touchgfx::Rect& invalidatedArea) const
    {
        const uint16_t width = textWidth();
        int16_t startX = 0;
        if (alignment == touchgfx::CENTER && width < getWidth())
        {
            startX = static_cast<int16_t>((getWidth() - width) / 2);
        }
        else if (alignment == touchgfx::RIGHT && width < getWidth())
        {
            startX = static_cast<int16_t>(getWidth() - width);
        }

        for (uint16_t i = 0; text[i] != '\0'; ++i)
        {
            drawGlyph(text[i], static_cast<int16_t>(startX + i * glyphAdvance()), invalidatedArea);
        }
    }

private:
    const char* text;
    touchgfx::colortype color;
    uint8_t alpha;
    uint8_t scale;
    touchgfx::Alignment alignment;

    uint16_t glyphAdvance() const
    {
        return static_cast<uint16_t>(6U * scale);
    }

    uint16_t textWidth() const
    {
        uint16_t count = 0;
        while (text[count] != '\0')
        {
            ++count;
        }
        return count == 0 ? 0 : static_cast<uint16_t>(count * glyphAdvance() - scale);
    }

    void drawGlyph(char character, int16_t x, const touchgfx::Rect& invalidatedArea) const
    {
        const uint8_t* glyph = glyphFor(character);
        for (uint8_t row = 0; row < 7; ++row)
        {
            for (uint8_t col = 0; col < 5; ++col)
            {
                if ((glyph[row] & (1U << (4U - col))) != 0)
                {
                    touchgfx::Rect pixel(static_cast<int16_t>(x + col * scale),
                                         static_cast<int16_t>(row * scale),
                                         scale,
                                         scale);
                    touchgfx::Rect dirty = pixel & invalidatedArea;
                    if (!dirty.isEmpty())
                    {
                        translateRectToAbsolute(dirty);
                        touchgfx::HAL::lcd().fillRect(dirty, color, alpha);
                    }
                }
            }
        }
    }

    static const uint8_t* glyphFor(char character)
    {
        if (character >= 'a' && character <= 'z')
        {
            character = static_cast<char>(character - ('a' - 'A'));
        }

        static const uint8_t blank[7] = {0, 0, 0, 0, 0, 0, 0};
        static const uint8_t colon[7] = {0, 4, 4, 0, 4, 4, 0};
        static const uint8_t slash[7] = {1, 2, 2, 4, 8, 8, 16};
        static const uint8_t percent[7] = {17, 2, 4, 8, 16, 17, 0};
        static const uint8_t minus[7] = {0, 0, 0, 31, 0, 0, 0};
        static const uint8_t plus[7] = {0, 4, 4, 31, 4, 4, 0};

        static const uint8_t n0[7] = {14, 17, 19, 21, 25, 17, 14};
        static const uint8_t n1[7] = {4, 12, 4, 4, 4, 4, 14};
        static const uint8_t n2[7] = {14, 17, 1, 2, 4, 8, 31};
        static const uint8_t n3[7] = {30, 1, 1, 14, 1, 1, 30};
        static const uint8_t n4[7] = {2, 6, 10, 18, 31, 2, 2};
        static const uint8_t n5[7] = {31, 16, 30, 1, 1, 17, 14};
        static const uint8_t n6[7] = {6, 8, 16, 30, 17, 17, 14};
        static const uint8_t n7[7] = {31, 1, 2, 4, 8, 8, 8};
        static const uint8_t n8[7] = {14, 17, 17, 14, 17, 17, 14};
        static const uint8_t n9[7] = {14, 17, 17, 15, 1, 2, 12};

        static const uint8_t A[7] = {14, 17, 17, 31, 17, 17, 17};
        static const uint8_t B[7] = {30, 17, 17, 30, 17, 17, 30};
        static const uint8_t C[7] = {14, 17, 16, 16, 16, 17, 14};
        static const uint8_t D[7] = {30, 17, 17, 17, 17, 17, 30};
        static const uint8_t E[7] = {31, 16, 16, 30, 16, 16, 31};
        static const uint8_t F[7] = {31, 16, 16, 30, 16, 16, 16};
        static const uint8_t G[7] = {14, 17, 16, 23, 17, 17, 15};
        static const uint8_t H[7] = {17, 17, 17, 31, 17, 17, 17};
        static const uint8_t I[7] = {14, 4, 4, 4, 4, 4, 14};
        static const uint8_t J[7] = {7, 2, 2, 2, 18, 18, 12};
        static const uint8_t K[7] = {17, 18, 20, 24, 20, 18, 17};
        static const uint8_t L[7] = {16, 16, 16, 16, 16, 16, 31};
        static const uint8_t M[7] = {17, 27, 21, 21, 17, 17, 17};
        static const uint8_t N[7] = {17, 25, 21, 19, 17, 17, 17};
        static const uint8_t O[7] = {14, 17, 17, 17, 17, 17, 14};
        static const uint8_t P[7] = {30, 17, 17, 30, 16, 16, 16};
        static const uint8_t Q[7] = {14, 17, 17, 17, 21, 18, 13};
        static const uint8_t R[7] = {30, 17, 17, 30, 20, 18, 17};
        static const uint8_t S[7] = {15, 16, 16, 14, 1, 1, 30};
        static const uint8_t T[7] = {31, 4, 4, 4, 4, 4, 4};
        static const uint8_t U[7] = {17, 17, 17, 17, 17, 17, 14};
        static const uint8_t V[7] = {17, 17, 17, 17, 10, 10, 4};
        static const uint8_t W[7] = {17, 17, 17, 21, 21, 21, 10};
        static const uint8_t X[7] = {17, 17, 10, 4, 10, 17, 17};
        static const uint8_t Y[7] = {17, 17, 10, 4, 4, 4, 4};
        static const uint8_t Z[7] = {31, 1, 2, 4, 8, 16, 31};

        switch (character)
        {
        case '0': return n0;
        case '1': return n1;
        case '2': return n2;
        case '3': return n3;
        case '4': return n4;
        case '5': return n5;
        case '6': return n6;
        case '7': return n7;
        case '8': return n8;
        case '9': return n9;
        case ':': return colon;
        case '/': return slash;
        case '%': return percent;
        case '-': return minus;
        case '+': return plus;
        case 'A': return A;
        case 'B': return B;
        case 'C': return C;
        case 'D': return D;
        case 'E': return E;
        case 'F': return F;
        case 'G': return G;
        case 'H': return H;
        case 'I': return I;
        case 'J': return J;
        case 'K': return K;
        case 'L': return L;
        case 'M': return M;
        case 'N': return N;
        case 'O': return O;
        case 'P': return P;
        case 'Q': return Q;
        case 'R': return R;
        case 'S': return S;
        case 'T': return T;
        case 'U': return U;
        case 'V': return V;
        case 'W': return W;
        case 'X': return X;
        case 'Y': return Y;
        case 'Z': return Z;
        default: return blank;
        }
    }
};
} // namespace WatchUi

#endif // PIXELTEXT_HPP
