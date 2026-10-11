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
    const unsigned long long megabytes =
        static_cast<unsigned long long>(info.f_bavail) * info.f_frsize /
        (1024ull * 1024ull);
    return megabytes > INT_MAX ? INT_MAX : static_cast<int>(megabytes);
}

extern "C" BOOL CheckForDiskspaceAndMessage(char *path, int minimumMegabytes)
{
    const int available = compute_free_diskspace(path);
    if (available < 0 || available >= minimumMegabytes)
        return TRUE;
    fprintf(stderr, "Dark Engine needs %d MB free; only %d MB is available.\n",
            minimumMegabytes, available);
    return FALSE;
}

extern "C" void MenuCommand(unsigned)
{
    // The game build has no native application menu on Linux.
}
