#pragma once

#include <types.h>
#include <stddef.h>
#include <stdint.h>

#pragma pack(push, 1)
typedef struct sPlatformMemoryStatus
{
   uint32 load_percent;
   uint32 total_physical_bytes;
   uint32 available_physical_bytes;
   uint32 total_page_file_bytes;
   uint32 available_page_file_bytes;
   uint32 total_virtual_bytes;
   uint32 available_virtual_bytes;
} sPlatformMemoryStatus;
#pragma pack(pop)

EXTERN BOOL PlatformGetMemoryStatus(sPlatformMemoryStatus *status);

EXTERN size_t PlatformAllocationSize(void *allocation);

EXTERN void PlatformHeapCheck(void);

EXTERN void PlatformHeapMinimize(void);
