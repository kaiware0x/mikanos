#include <cstdint>

extern "C" void KernelMain(uint64_t frame_buffer_base,
                           uint64_t frame_buffer_size)
{
    auto frame_buffer = reinterpret_cast<uint8_t *>(frame_buffer_base);
    for (auto i = 0ull; i < frame_buffer_size; ++i)
    {
        frame_buffer[i] = i % 256;
    }

    while (1)
        __asm__("hlt");
}
