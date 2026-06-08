
#include <cstring>

#include "console.hpp"
#include "font.hpp"

Console::Console(PixelWriter &writer, const PixelColor &fg_color, const PixelColor &bg_color)
    : m_writer(writer), m_fg_color(fg_color), m_bg_color(bg_color) {}

void Console::PutString(const char *s)
{
    while (*s)
    {
        if (*s == '\n')
        {
            NewLine();
        }
        else if (m_cursor_column < kColumns - 1)
        {
            WriteAscii(m_writer, 8 * m_cursor_column, 16 * m_cursor_row, *s, m_fg_color);
            m_buffer[m_cursor_row][m_cursor_column] = *s;
            ++m_cursor_column;
        }
        ++s;
    }
}

void Console::NewLine()
{
    m_cursor_column = 0;

    if (m_cursor_row < kRows - 1)
    {
        ++m_cursor_row;
    }
    else
    {
        // 画面をリセット
        for (int y = 0; y < 16 * kRows; ++y)
        {
            for (int x = 0; x < 8 * kColumns; ++x)
            {
                m_writer.Write(x, y, m_bg_color);
            }
        }

        // 1行スクロール
        for (int row = 0; row < kRows - 1; ++row)
        {
            // 上の行にコピーしてスクロールしていく
            memcpy(m_buffer[row], m_buffer[row + 1], kColumns + 1);
            WriteString(m_writer, 0, 16 * row, m_buffer[row], m_fg_color);
        }
        // 末尾の1行をリセット
        memset(m_buffer[kRows - 1], 0, kColumns + 1);
    }
}
