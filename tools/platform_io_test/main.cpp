// Build against libdark_platform.a with -Ih -lSDL3 -ldl -pthread.
// Runs only in a temporary directory; no retail data is required.
#include <platform_filesystem.h>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fcntl.h>
#include <string>
#include <unistd.h>

static FILE *NativeFopen(const char *path, const char *mode)
{
   return std::fopen(path, mode);
}

#include <platform_posix.h>
extern "C" int PlatformOpen(const char *, int, ...);
extern "C" int PlatformChangeDirectory(const char *);

static void require(bool passed)
{
   if (!passed) { std::fputs("platform I/O check failed\n", stderr); std::exit(1); }
}

int main()
{
   const auto previous = std::filesystem::current_path();
   char temp[] = "/tmp/dark-platform-io-XXXXXX";
   require(mkdtemp(temp) != nullptr);
   std::filesystem::create_directories(std::string(temp) + "/Resources/Textures");
   require(PlatformChangeDirectory(temp) == 0);
   const auto cwd = std::filesystem::current_path();
   require(PlatformDirectoryExists("resources\\TEXTURES"));
   require(!PlatformDirectoryExists("resources/missing"));
   char directoryResolved[4096];
   require(!PlatformResolvePath("resources/textures/NewFolder", directoryResolved, sizeof(directoryResolved)));
   require(PlatformCreateDirectory("resources\\TEXTURES\\NewFolder"));
   require(PlatformDirectoryExists("RESOURCES/textures/newfolder"));
   require(PlatformCreateDirectory("RESOURCES/textures/NEWFOLDER"));
   require(PlatformResolvePath("resources/textures/newfolder", directoryResolved, sizeof(directoryResolved)));
   require(std::filesystem::exists("Resources/Textures/NewFolder"));
   require(!std::filesystem::exists("Resources/Textures/NEWFOLDER"));
   require(!PlatformCreateDirectory("missing-parent/child"));
   require(!PlatformDirectoryExists(nullptr));
   require(!PlatformCreateDirectory(""));
   require(std::filesystem::current_path() == cwd);
   FILE *file = fopen("Resources/Textures/Stone.bin", "wb");
   require(file != nullptr);
   std::fputs("stone", file);
   std::fclose(file);
   require(!PlatformDirectoryExists("resources/textures/stone.bin"));
   require(!PlatformCreateDirectory("resources/textures/STONE.bin"));
   char resolved[4096];
   require(PlatformResolvePath("resources/textures/stone.bin", resolved, sizeof(resolved)));
   require(PlatformResolvePath("RESOURCES/TEXTURES/STONE.BIN", resolved, sizeof(resolved)));
   int readFd = PlatformOpen("resources/textures/stone.bin", O_RDONLY);
   require(readFd >= 0);
   close(readFd);
   file = fopen("resources/textures/stone.bin", "rb");
   require(file && std::fgetc(file) == 's');
   std::fclose(file);
   require(!PlatformResolvePath("resources/new.bin", resolved, sizeof(resolved)));
   int fd = PlatformOpen("Resources/New.bin", O_CREAT | O_WRONLY, 0600);
   require(fd >= 0);
   close(fd);
   require(PlatformResolvePath("resources/new.bin", resolved, sizeof(resolved)));

   // Exact spellings must still win when case-folded names are ambiguous.
   file = NativeFopen("Resources/Textures/STONE.bin", "wb");
   require(file != nullptr);
   std::fclose(file);
   // Refresh directory caches after the native filesystem mutation.
   require(PlatformChangeDirectory(temp) == 0);
   require(PlatformResolvePath("Resources/Textures/Stone.bin", resolved, sizeof(resolved)));
   require(PlatformResolvePath("Resources/Textures/STONE.bin", resolved, sizeof(resolved)));
   require(!PlatformResolvePath("Resources/Textures/sToNe.bin", resolved, sizeof(resolved)));
   require(std::filesystem::remove("Resources/Textures/STONE.bin"));
   require(std::filesystem::remove("Resources/New.bin"));
   require(PlatformChangeDirectory(temp) == 0);
   require(!PlatformResolvePath("resources/new.bin", resolved, sizeof(resolved)));
   require(!PlatformResolvePath("resources/new.bin", resolved, sizeof(resolved)));
   file = fopen("Resources/NEW.bin", "wb");
   require(file != nullptr);
   std::fclose(file);
   require(PlatformResolvePath("resources/new.bin", resolved, sizeof(resolved)));

   const auto start = std::chrono::steady_clock::now();
   for (int i = 0; i < 20000; ++i)
   {
      require(PlatformResolvePath("resources/textures/stone.bin", resolved, sizeof(resolved)));
      file = fopen("resources/textures/stone.bin", "rb");
      require(file != nullptr);
      std::fclose(file);
      require(!PlatformResolvePath("resources/textures/missing.bin", resolved, sizeof(resolved)));
      // Diagnostics frequently reopen existing files during mission loads.
      file = fopen("Resources/NEW.bin", "ab");
      require(file != nullptr);
      std::fclose(file);
   }
   std::printf("20000 resource lookup batches: %.1f ms\n",
      std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count());
   require(PlatformChangeDirectory(previous.c_str()) == 0);
   std::filesystem::remove_all(temp);
}
