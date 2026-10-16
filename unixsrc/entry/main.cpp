#include <lg.h>

#pragma pack(push, 8)
#include <cerrno>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <limits.h>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>
#pragma pack(pop)

#include <movie_backend.h>
#include <platform_services.h>

#ifdef __ANDROID__
#include <SDL2/SDL_main.h>
#endif

#include "../render/runtime.h"

extern "C" int LGAPI _AppMain(int argc, const char *argv[]);

extern "C" {
int _g_referenceEntryPoint = 0;
}

namespace
{
bool FileExistsInDirectory(const std::string &directory, const char *name)
{
    std::string path = directory;
    if (!path.empty() && path.back() != '/')
        path.push_back('/');
    path += name;

    char resolved[PATH_MAX];
    struct stat status;
    return PlatformResolvePath(path.c_str(), resolved, sizeof(resolved)) &&
           stat(resolved, &status) == 0 && S_ISREG(status.st_mode);
}

const char *DetectGameProfile(const std::string &directory)
{
    if (!FileExistsInDirectory(directory, "cam.cfg"))
        return nullptr;
    if (FileExistsInDirectory(directory, "MISS18.MIS"))
        return "gold";
    if (FileExistsInDirectory(directory, "MISS8.MIS"))
        return "thief2";
    if (FileExistsInDirectory(directory, "DARK.GAM"))
        return "tdp";
    return nullptr;
}

bool ResolveDirectory(const char *candidate, std::string &directory)
{
    if (!candidate || !candidate[0])
        return false;

    std::string expanded = candidate;
    if (expanded[0] == '~' &&
        (expanded.size() == 1 || expanded[1] == '/'))
    {
        const char *home = std::getenv("HOME");
        if (home && home[0])
            expanded.replace(0, 1, home);
    }

    char resolved[PATH_MAX];
    char canonical[PATH_MAX];
    struct stat status;
    if (!PlatformResolvePath(expanded.c_str(), resolved, sizeof(resolved)) ||
        stat(resolved, &status) != 0 || !S_ISDIR(status.st_mode) ||
        !realpath(resolved, canonical))
        return false;
    directory = canonical;
    return true;
}

void NormalizeEnteredPath(std::string &path)
{
    const std::string whitespace = " \t\r\n";
    const size_t first = path.find_first_not_of(whitespace);
    if (first == std::string::npos)
    {
        path.clear();
        return;
    }
    const size_t last = path.find_last_not_of(whitespace);
    path = path.substr(first, last - first + 1);
    if (path.size() >= 2 &&
        ((path.front() == '\'' && path.back() == '\'') ||
         (path.front() == '"' && path.back() == '"')))
        path = path.substr(1, path.size() - 2);
}

bool SelectGameDirectory(const char *requested, bool interactive,
                         std::string &directory, const char *&profile)
{
    if (requested && requested[0])
    {
        if (ResolveDirectory(requested, directory))
            profile = DetectGameProfile(directory);
        if (profile)
            return true;
        std::fprintf(stderr,
            "Dark Engine: '%s' is not a recognizable Thief installation.\n",
            requested);
    }
    else
    {
        char current[PATH_MAX];
        if (getcwd(current, sizeof(current)) &&
            ResolveDirectory(current, directory))
            profile = DetectGameProfile(directory);
        if (profile)
            return true;

        const char *environmentPath = std::getenv("DARK_GAME_PATH");
        if (environmentPath && environmentPath[0] &&
            ResolveDirectory(environmentPath, directory))
            profile = DetectGameProfile(directory);
        if (profile)
            return true;
    }

    if (!interactive || !isatty(STDIN_FILENO))
    {
        std::fprintf(stderr,
            "Dark Engine requires a retail Thief installation containing "
            "cam.cfg and the mission data.\n"
            "Run from that folder, set DARK_GAME_PATH, or pass "
            "game_path=/path/to/thief.\n");
        return false;
    }

    std::fprintf(stderr,
        "\nDark Engine needs the folder containing your retail Thief game data.\n"
        "Enter the Thief: The Dark Project, Thief Gold, or Thief II folder.\n");
    for (;;)
    {
        std::fprintf(stderr, "Game folder (blank cancels): ");
        std::fflush(stderr);
        std::string selected;
        if (!std::getline(std::cin, selected))
            return false;
        NormalizeEnteredPath(selected);
        if (selected.empty())
            return false;
        if (ResolveDirectory(selected.c_str(), directory))
            profile = DetectGameProfile(directory);
        if (profile)
            return true;
        std::fprintf(stderr,
            "That folder is not a recognizable installation; it must contain "
            "cam.cfg and the retail mission data.\n");
    }
}

bool PrepareDarkDataDirectory(const char *requested, bool interactive)
{
    std::string directory;
    const char *profile = nullptr;
    if (!SelectGameDirectory(requested, interactive, directory, profile))
        return false;
    if (chdir(directory.c_str()) != 0)
    {
        std::fprintf(stderr, "Dark Engine: cannot enter '%s': %s\n",
                     directory.c_str(), std::strerror(errno));
        return false;
    }
    setenv("DARK_GAME_PROFILE", profile, 1);
    setenv("DARK_GAME_PATH", directory.c_str(), 1);
    return true;
}
}

int main(int argc, char *argv[])
{
#ifdef __ANDROID__
    char entryMessage[96];
    std::snprintf(entryMessage, sizeof(entryMessage),
                  "[startup] SDL_main begin argc=%d\n", argc);
    PlatformDebugOutput(entryMessage);
    for (int i = 0; i < argc; ++i)
    {
        std::snprintf(entryMessage, sizeof(entryMessage),
                      "[startup] argv[%d]=%.64s\n", i,
                      argv[i] ? argv[i] : "(null)");
        PlatformDebugOutput(entryMessage);
    }
#endif

    bool explicitContent = false;
    const char *requestedGamePath = nullptr;
    std::vector<const char *> arguments;
    arguments.reserve(argc);
    arguments.push_back(argv[0]);
    for (int i = 1; i < argc; ++i)
    {
        if (std::strncmp(argv[i], "movie_test=", 11) == 0)
        {
            const char *path = argv[i] + 11;
            if (!path[0] || !DarkRenderOpen(640, 480,
                                             "Dark Engine movie test"))
                return 1;
            const bool played = MovieBackendPlaySynchronous(path, 0);
            DarkRenderClose();
            return played ? 0 : 1;
        }
        if (std::strncmp(argv[i], "game_path=", 10) == 0)
        {
            requestedGamePath = argv[i] + 10;
            continue;
        }
        arguments.push_back(argv[i]);
        if (std::strncmp(argv[i], "content=", 8) == 0 ||
            std::strncmp(argv[i], "fm=", 3) == 0)
            explicitContent = true;
    }

    const int engineArgc = static_cast<int>(arguments.size());
    const bool interactive = engineArgc == 1 && !explicitContent;
    if (!PrepareDarkDataDirectory(requestedGamePath, interactive))
    {
#ifdef __ANDROID__
        PlatformShowError("Dark Engine",
                          "The Android game data directory could not be prepared.");
#endif
        return 2;
    }

#ifdef __ANDROID__
    PlatformDebugOutput("[startup] game directory prepared\n");
#endif

    // Any non-interactive launch without an explicit content choice still
    // needs a deterministic search path.  Previously only the picker path set
    // this default, so quickstart reached GUI initialization before a retail
    // font path existed.
    const char *environmentContentMode = std::getenv("DARK_CONTENT_MODE");
    if (interactive ||
        (!explicitContent &&
         (!environmentContentMode || !environmentContentMode[0])))
        setenv("DARK_CONTENT_MODE", "vanilla", 1);

    if (interactive)
    {
        setenv("DARK_CONTENT_PICKER", "1", 1);
    }
    else
        unsetenv("DARK_CONTENT_PICKER");

    const int result = _AppMain(engineArgc, arguments.data());
#ifdef __ANDROID__
    char resultMessage[96];
    std::snprintf(resultMessage, sizeof(resultMessage),
                  "[startup] _AppMain returned %d\n", result);
    PlatformDebugOutput(resultMessage);
#endif
    return result;
}
