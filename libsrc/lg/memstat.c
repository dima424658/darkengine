#include <limits.h>
#include <string.h>

#include <lg.h>
#include <memstat.h>
#include <platform_services.h>

int MemStatsFlags = MS_GETSTAT;

int MemStats(MemStat *stats)
{
   sPlatformMemoryStatus memory;

   if (!stats)
      return -1;
   memset(stats, 0, sizeof(*stats));
   if (!PlatformGetMemoryStatus(&memory))
      return 0;

   stats->info.largestFreeBlock =
      memory.available_virtual_bytes > LONG_MAX
         ? LONG_MAX
         : (long)memory.available_virtual_bytes;
   stats->info.totPhysPages = (long)(memory.total_physical_bytes / 4096);
   stats->info.totFreePages = (long)(memory.available_physical_bytes / 4096);
   stats->info.totUnlockedPages = stats->info.totFreePages;
   return 0;
}

int MallocableTotal(int lowMemReserved, uchar value)
{
   sPlatformMemoryStatus memory;
   const ulong kMinCap = 0x600000;
   const ulong kMaxCap = 0x1000000;
   const ulong kNonDynamicReserved = 0x300000;
   ulong target;

   (void)lowMemReserved;
   (void)value;
   if (!PlatformGetMemoryStatus(&memory))
      return kMinCap;

   target = memory.total_physical_bytes > kNonDynamicReserved
               ? memory.total_physical_bytes - kNonDynamicReserved
               : 0;
   if (target > kMaxCap)
      target = kMaxCap;
   return target > kMinCap ? target : kMinCap;
}
