
#include <stdalign.h>

#include <Guid/FileInfo.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Protocol/BlockIo.h>
#include <Protocol/DiskIo2.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/SimpleFileSystem.h>
#include <Uefi.h>

#include "../kernel/elf.hpp"
#include "../kernel/frame_buffer_config.hpp"

#define PAGE_SIZE 4096 // byte

void Halt(void)
{
    while (1)
        __asm__("hlt");
}

// Utility macro for KaiWare
#define KW_HALT_IF_ERROR(x)                                                         \
    do                                                                              \
    {                                                                               \
        if (EFI_ERROR(x))                                                           \
        {                                                                           \
            Print(L"Failed on %s (%s:%d), status=%r\n", #x, __FILE__, __LINE__, x); \
            Halt();                                                                 \
        }                                                                           \
    } while (0)

#define KW_RETURN_IF_ERROR(x) \
    do                        \
    {                         \
        if (EFI_ERROR(x))     \
        {                     \
            return x;         \
        }                     \
    } while (0)

struct MemoryMap
{
    UINTN buffer_size;
    VOID *buffer;
    UINTN map_size;
    UINTN map_key;
    UINTN descriptor_size;
    UINT32 descriptor_version;
};

EFI_STATUS GetMemoryMap(struct MemoryMap *map)
{
    if (map->buffer == NULL)
    {
        return EFI_BUFFER_TOO_SMALL;
    }

    map->map_size = map->buffer_size; // 最大書込みサイズを設定しておく
    return gBS->GetMemoryMap(
        &map->map_size, // 実際のサイズが格納される
        (EFI_MEMORY_DESCRIPTOR *)map->buffer,
        &map->map_key,
        &map->descriptor_size,
        &map->descriptor_version);
}

EFI_STATUS OpenRootDir(EFI_HANDLE image_handle, EFI_FILE_PROTOCOL **root)
{
    EFI_LOADED_IMAGE_PROTOCOL *loaded_image;
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *fs;

    KW_RETURN_IF_ERROR(
        gBS->OpenProtocol(
            image_handle,
            &gEfiLoadedImageProtocolGuid,
            (VOID **)&loaded_image,
            image_handle,
            NULL,
            EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL) //
    );

    KW_RETURN_IF_ERROR(
        gBS->OpenProtocol(
            loaded_image->DeviceHandle,
            &gEfiSimpleFileSystemProtocolGuid,
            (VOID **)&fs,
            image_handle,
            NULL,
            EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL) //
    );

    return fs->OpenVolume(fs, root);
}

const CHAR16 *GetMemoryTypeUnicode(EFI_MEMORY_TYPE type)
{
    switch (type)
    {
    case EfiReservedMemoryType:
        return L"EfiReservedMemoryType";
    case EfiLoaderCode:
        return L"EfiLoaderCode";
    case EfiLoaderData:
        return L"EfiLoaderData";
    case EfiBootServicesCode:
        return L"EfiBootServicesCode";
    case EfiBootServicesData:
        return L"EfiBootServicesData";
    case EfiRuntimeServicesCode:
        return L"EfiRuntimeServicesCode";
    case EfiRuntimeServicesData:
        return L"EfiRuntimeServicesData";
    case EfiConventionalMemory:
        return L"EfiConventionalMemory";
    case EfiUnusableMemory:
        return L"EfiUnusableMemory";
    case EfiACPIReclaimMemory:
        return L"EfiACPIReclaimMemory";
    case EfiACPIMemoryNVS:
        return L"EfiACPIMemoryNVS";
    case EfiMemoryMappedIO:
        return L"EfiMemoryMappedIO";
    case EfiMemoryMappedIOPortSpace:
        return L"EfiMemoryMappedIOPortSpace";
    case EfiPalCode:
        return L"EfiPalCode";
    case EfiPersistentMemory:
        return L"EfiPersistentMemory";
    case EfiMaxMemoryType:
        return L"EfiMaxMemoryType";
    default:
        return L"InvalidMemoryType";
    }
}

/**
 * @brief 与えられた MemoryMap を CSV 形式でファイルに書き出す
 */
EFI_STATUS SaveMemoryMap(struct MemoryMap *map, EFI_FILE_PROTOCOL *file)
{
    CHAR8 buf[256];
    UINTN len;

    CHAR8 *header = "Index, Type, Type(name), PhysicalStart, NumberOfPages, Attribute\n";
    len = AsciiStrLen(header);
    KW_RETURN_IF_ERROR(file->Write(file, &len, header));

    Print(L"map->buffer = %08lx, map->map_size = %08lx\n", map->buffer, map->map_size);

    EFI_PHYSICAL_ADDRESS iter; // 各 Descriptor の先頭物理アドレス
    int i;
    for (iter = (EFI_PHYSICAL_ADDRESS)map->buffer, i = 0;
         iter < (EFI_PHYSICAL_ADDRESS)map->buffer + map->map_size;
         iter += map->descriptor_size, ++i)
    {
        EFI_MEMORY_DESCRIPTOR *desc = (EFI_MEMORY_DESCRIPTOR *)iter;
        len = AsciiSPrint(
            buf, sizeof(buf),
            "%u, %x, %-ls, %08lx, %lx, %lx\n",
            i, desc->Type, GetMemoryTypeUnicode(desc->Type),
            desc->PhysicalStart, desc->NumberOfPages,
            desc->Attribute & 0xffffflu);

        KW_RETURN_IF_ERROR(file->Write(file, &len, buf));
    }

    return EFI_SUCCESS;
}

/**
 * @brief Open "Graphics Output Protocol"
 */
EFI_STATUS OpenGOP(EFI_HANDLE image_handle,
                   EFI_GRAPHICS_OUTPUT_PROTOCOL **gop)
{
    UINTN num_gop_handles = 0;
    EFI_HANDLE *gop_handles = NULL;
    KW_RETURN_IF_ERROR(
        gBS->LocateHandleBuffer(
            ByProtocol,
            &gEfiGraphicsOutputProtocolGuid,
            NULL,
            &num_gop_handles,
            &gop_handles));
    KW_RETURN_IF_ERROR(
        gBS->OpenProtocol(
            gop_handles[0],
            &gEfiGraphicsOutputProtocolGuid,
            (VOID **)gop,
            image_handle,
            NULL,
            EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL));
    FreePool(gop_handles);
    return EFI_SUCCESS;
}

const CHAR16 *GetPixelFormatUnicode(EFI_GRAPHICS_PIXEL_FORMAT fmt)
{
    switch (fmt)
    {
    case PixelRedGreenBlueReserved8BitPerColor:
        return L"PixelRedGreenBlueReserved8BitPerColor";
    case PixelBlueGreenRedReserved8BitPerColor:
        return L"PixelBlueGreenRedReserved8BitPerColor";
    case PixelBitMask:
        return L"PixelBitMask";
    case PixelBltOnly:
        return L"PixelBltOnly";
    case PixelFormatMax:
        return L"PixelFormatMax";
    default:
        return L"InvalidPixelFormat";
    }
}

void CalcLoadAddressRange(Elf64_Ehdr *ehdr, UINT64 *first, UINT64 *last)
{
    Elf64_Phdr *phdr = (Elf64_Phdr *)((UINT64)ehdr + ehdr->e_phoff);
    *first = MAX_UINT64;
    *last = 0;
    for (Elf64_Half i = 0; i < ehdr->e_phnum; ++i)
    {
        if (phdr[i].p_type != PT_LOAD)
            continue;
        *first = MIN(*first, phdr[i].p_vaddr);
        *last = MAX(*last, phdr[i].p_vaddr + phdr[i].p_memsz);
    }
}

void CopyLoadSegments(Elf64_Ehdr *ehdr)
{
    Elf64_Phdr *phdr = (Elf64_Phdr *)((UINT64)ehdr + ehdr->e_phoff);
    for (Elf64_Half i = 0; i < ehdr->e_phnum; ++i)
    {
        if (phdr[i].p_type != PT_LOAD)
            continue;

        UINT64 segm_in_file = (UINT64)ehdr + phdr[i].p_offset;
        CopyMem((VOID *)phdr[i].p_vaddr, (VOID *)segm_in_file, phdr[i].p_filesz);

        UINTN remain_bytes = phdr[i].p_memsz - phdr[i].p_filesz;
        SetMem((VOID *)(phdr[i].p_vaddr + phdr[i].p_filesz), remain_bytes, 0);
    }
}

/**
 * @brief Boot Loader のエントリーポイント
 */
EFI_STATUS EFIAPI UefiMain(
    EFI_HANDLE image_handle,
    EFI_SYSTEM_TABLE *system_table)
{
    // 以降使い回す status 変数を定義しておく
    EFI_STATUS status;

    Print(L"Hello, Mikan World!\n");

    CHAR8 memmap_buf[4 * PAGE_SIZE];
    struct MemoryMap memmap = {sizeof(memmap_buf), memmap_buf, 0, 0, 0, 0};
    KW_HALT_IF_ERROR(GetMemoryMap(&memmap));

    // Root dir の Open
    EFI_FILE_PROTOCOL *root_dir;
    KW_HALT_IF_ERROR(OpenRootDir(image_handle, &root_dir));

    //----------------------------------------------------
    // Memory map file の保存
    //----------------------------------------------------

    // Memory map file の Open
    EFI_FILE_PROTOCOL *memmap_file;
    status = root_dir->Open(
        root_dir, &memmap_file, L"\\memmap",
        EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE, 0);

    if (EFI_ERROR(status))
    {
        Print(L"Failed to open file '\\memmap': %r\n", status);
        Print(L"Ignored.\n");
    }
    else
    {
        // Memory map file の Write
        KW_HALT_IF_ERROR(SaveMemoryMap(&memmap, memmap_file));
        // Memory map file の Close
        KW_HALT_IF_ERROR(memmap_file->Close(memmap_file));
    }

    //----------------------------------------------------
    // Graphics Output Protocol によるピクセル描画
    //----------------------------------------------------

    // gop ハンドルの取得
    EFI_GRAPHICS_OUTPUT_PROTOCOL *gop;
    KW_HALT_IF_ERROR(OpenGOP(image_handle, &gop));
    // 各種情報のPrint
    Print(L"Resolution: %ux%u, Pixel Format: %s, %u pixels/line\n",
          gop->Mode->Info->HorizontalResolution,
          gop->Mode->Info->VerticalResolution,
          GetPixelFormatUnicode(gop->Mode->Info->PixelFormat),
          gop->Mode->Info->PixelsPerScanLine);
    Print(L"Frame Buffer: 0x%0lx - 0x%0lx, Size: %lu bytes\n",
          gop->Mode->FrameBufferBase,
          gop->Mode->FrameBufferBase + gop->Mode->FrameBufferSize,
          gop->Mode->FrameBufferSize);

    //----------------------------------------------------
    // kernel を読み込む
    //----------------------------------------------------

    // kernel_file の取得
    EFI_FILE_PROTOCOL *kernel_file;
    KW_HALT_IF_ERROR(root_dir->Open(
        root_dir, &kernel_file, L"\\kernel.elf", EFI_FILE_MODE_READ, 0));

    // kernel_file の Info の取得
    // EFI_FILE_INFO::FileName 分のサイズを 12 文字分追加
    UINTN file_info_size = sizeof(EFI_FILE_INFO) + sizeof(CHAR16) * 12;
    alignas(alignof(EFI_FILE_INFO)) UINT8 file_info_buffer[file_info_size];
    KW_HALT_IF_ERROR(kernel_file->GetInfo(
        kernel_file, &gEfiFileInfoGuid, &file_info_size, file_info_buffer));

    // kernel_file_size の取得
    EFI_FILE_INFO *file_info = (EFI_FILE_INFO *)file_info_buffer;
    UINTN kernel_file_size = file_info->FileSize;

    // kernel_file の読み込み
    VOID *kernel_buffer;
    KW_HALT_IF_ERROR(gBS->AllocatePool(EfiLoaderData, kernel_file_size, &kernel_buffer));
    KW_HALT_IF_ERROR(kernel_file->Read(kernel_file, &kernel_file_size, kernel_buffer));

    // コピー先のメモリ領域の確保
    Elf64_Ehdr *kernel_ehdr = (Elf64_Ehdr *)kernel_buffer;
    UINT64 kernel_first_addr, kernel_last_addr;
    CalcLoadAddressRange(kernel_ehdr, &kernel_first_addr, &kernel_last_addr);

    UINTN num_pages = (kernel_last_addr - kernel_first_addr + 0xfff) / 0x1000;
    KW_HALT_IF_ERROR(gBS->AllocatePages(
        AllocateAddress, EfiLoaderData, num_pages, &kernel_first_addr));

    // LOAD セグメントのコピー
    CopyLoadSegments(kernel_ehdr);
    Print(L"Kernel: 0x%0lx - 0x%0lx\n", kernel_first_addr, kernel_last_addr);

    KW_HALT_IF_ERROR(gBS->FreePool(kernel_buffer));

    //----------------------------------------------------
    // Boot Service を停止させる
    //----------------------------------------------------

    status = gBS->ExitBootServices(image_handle, memmap.map_key);
    if (EFI_ERROR(status))
    {
        KW_HALT_IF_ERROR(GetMemoryMap(&memmap));
        KW_HALT_IF_ERROR(gBS->ExitBootServices(image_handle, memmap.map_key));
    }

    //----------------------------------------------------
    // Frame buffer config の作成
    //----------------------------------------------------

    struct FrameBufferConfig config = {
        (UINT8 *)gop->Mode->FrameBufferBase,
        gop->Mode->Info->PixelsPerScanLine,
        gop->Mode->Info->HorizontalResolution,
        gop->Mode->Info->VerticalResolution,
        0};
    switch (gop->Mode->Info->PixelFormat)
    {
    case PixelRedGreenBlueReserved8BitPerColor:
        config.pixel_format = kPixelRGBResv8BitPerColor;
        break;
    case PixelBlueGreenRedReserved8BitPerColor:
        config.pixel_format = kPixelBGRResv8BitPerColor;
        break;
    default:
        Print(L"Unimplemented pixel format: %d\n", gop->Mode->Info->PixelFormat);
        Halt();
    }

    //----------------------------------------------------
    // Kernel を起動する
    //----------------------------------------------------

    UINT64 entry_addr = *(UINT64 *)(kernel_first_addr + 24);
    typedef void EntryPointType(const struct FrameBufferConfig *);
    EntryPointType *entry_point = (EntryPointType *)entry_addr;
    entry_point(&config);

    Print(L"All done!\n");

    while (1)
        ;
    return EFI_SUCCESS;
}
