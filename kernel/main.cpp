#include <cstddef>
#include <cstdint>

#include "font.hpp"
#include "frame_buffer_config.hpp"
#include "graphics.hpp"

extern "C" void __cxa_pure_virtual() {
  while (1) __asm__("hlt");
}

void *operator new(std::size_t size, void *buf)
{
    return buf;
}
void operator delete(void *obj) noexcept
{
}

char pixel_writer_buf[sizeof(RGBResv8BitPerColorPixelWriter)];
PixelWriter *pixel_writer = nullptr;

extern "C" void KernelMain(const FrameBufferConfig &frame_buffer_config)
{
    switch (frame_buffer_config.pixel_format)
    {
    case kPixelRGBResv8BitPerColor:
        pixel_writer = new (pixel_writer_buf) RGBResv8BitPerColorPixelWriter(frame_buffer_config);
        break;
    case kPixelBGRResv8BitPerColor:
        pixel_writer = new (pixel_writer_buf) BGRResv8BitPerColorPixelWriter(frame_buffer_config);
        break;
    default:
        break;
    }

    for (int x = 0; x < frame_buffer_config.horizontal_resolution; ++x)
    {
        for (int y = 0; y < frame_buffer_config.vertical_resolution; ++y)
        {
            pixel_writer->Write(x, y, {255, 255, 255});
        }
    }

    for (int x = 0; x < 200; ++x)
    {
        for (int y = 0; y < 100; ++y)
        {
            pixel_writer->Write(100 + x, 100 + y, {0, 255, 0});
        }
    }

    WriteAscii(*pixel_writer, 50, 50, 'A', {0, 0, 0});
    WriteAscii(*pixel_writer, 66, 50, 'A', {0, 0, 0});

    while (1)
        __asm__("hlt");
}
