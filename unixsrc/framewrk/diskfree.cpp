#include <diskfree.h>

#pragma pack(push, 8)
#include <limits.h>
#include <stdio.h>
#include <sys/statvfs.h>
#pragma pack(pop)

extern "C" int compute_free_diskspace(char *path)
{
    struct statvfs info = {};
    if (statvfs(path && *path ? path : ".", &info) != 0)
        return -1;
    const unsigned long long bytes =
        static_cast<unsigned long long>(info.f_bavail) * info.f_frsize;
    return bytes > INT_MAX ? INT_MAX : static_cast<int>(bytes);
}

extern "C" BOOL CheckForDiskspaceAndMessage(char *path, int minimumMegabytes)
{
    const int available = compute_free_diskspace(path);
    const unsigned long long required =
        static_cast<unsigned long long>(minimumMegabytes) * 1024ull * 1024ull;
    if (available < 0 || static_cast<unsigned long long>(available) >= required)
        return TRUE;
    fprintf(stderr, "Dark Engine needs %d MB free; only %d MB is available.\n",
            minimumMegabytes, available / (1024 * 1024));
    return FALSE;
}
