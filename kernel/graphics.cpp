#include "graphics.hpp"

PixelWriter::PixelWriter(const FrameBufferConfig &config)
    : m_config(config)
{
}
uint8_t *PixelWriter::PixelAt(int x, int y)
{
    return m_config.frame_buffer + 4 * (m_config.pixels_per_scan_line * y + x);
}

void RGBResv8BitPerColorPixelWriter::Write(int x, int y, const PixelColor &c)
{
    auto p = PixelAt(x, y);
    p[0] = c.r;
    p[1] = c.g;
    p[2] = c.b;
}

void BGRResv8BitPerColorPixelWriter::Write(int x, int y, const PixelColor &c)
{
    auto p = PixelAt(x, y);
    p[0] = c.b;
    p[1] = c.g;
    p[2] = c.r;
}
