#include <Library/UefiLib.h>
#include <Uefi.h>

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

EFI_STATUS EFIAPI UefiMain(
    EFI_HANDLE image_handle,
    EFI_SYSTEM_TABLE *system_table)
{
    Print(L"Hello, Mikan World!\n");
    while (1)
        ;
    return EFI_SUCCESS;
}
