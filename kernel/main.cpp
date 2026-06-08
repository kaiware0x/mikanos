#include <cstddef>
#include <cstdint>
#include <cstdio>

#include "console.hpp"
#include "font.hpp"
#include "frame_buffer_config.hpp"
#include "graphics.hpp"

extern "C" void __cxa_pure_virtual()
{
    while (1)
        __asm__("hlt");
}

void *operator new(std::size_t size, void *buf)
{
    return buf;
}
void operator delete(void *obj) noexcept
{
}

// pixel writer
char pixel_writer_buf[sizeof(RGBResv8BitPerColorPixelWriter)];
PixelWriter *pixel_writer = nullptr;

// console
char console_buf[sizeof(Console)];
Console *console = nullptr;

int printk(const char *format, ...)
{
    va_list ap;
    int result;
    char str[1024];

    va_start(ap, format);
    result = vsprintf(str, format, ap);
    va_end(ap);

    console->PutString(str);
    return result;
}

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

    const PixelColor fg_color = {255, 255, 255};
    const PixelColor bg_color = {0, 0, 0};

    console = new (console_buf) Console(*pixel_writer, fg_color, bg_color);

    for (int x = 0; x < frame_buffer_config.horizontal_resolution; ++x)
    {
        for (int y = 0; y < frame_buffer_config.vertical_resolution; ++y)
        {
            pixel_writer->Write(x, y, bg_color);
        }
    }

    for (int i = 0; i < 30; ++i)
    {
        printk("printk: %d\n", i);
    }

    while (1)
        __asm__("hlt");
}
