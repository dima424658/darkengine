// Build against libdark_platform.a with -Ih -lSDL3 -ldl -pthread.
// Runs only in a temporary directory; no retail data is required.
#include <platform_services.h>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fcntl.h>
#include <string>
#include <unistd.h>

extern "C" FILE *PlatformFopen(const char *, const char *);
extern "C" int PlatformOpen(const char *, int, ...);
extern "C" int PlatformAccess(const char *, int);
extern "C" int PlatformUnlink(const char *);
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
   FILE *file = PlatformFopen("Resources/Textures/Stone.bin", "wb");
   require(file != nullptr);
   std::fputs("stone", file);
   std::fclose(file);
   char resolved[4096];
   require(PlatformResolvePath("resources/textures/stone.bin", resolved, sizeof(resolved)));
   require(PlatformAccess("RESOURCES/TEXTURES/STONE.BIN", R_OK) == 0);
   int readFd = PlatformOpen("resources/textures/stone.bin", O_RDONLY);
   require(readFd >= 0);
   close(readFd);
   file = PlatformFopen("resources/textures/stone.bin", "rb");
   require(file && std::fgetc(file) == 's');
   std::fclose(file);
   require(!PlatformResolvePath("resources/new.bin", resolved, sizeof(resolved)));
   int fd = PlatformOpen("Resources/New.bin", O_CREAT | O_WRONLY, 0600);
   require(fd >= 0);
   close(fd);
   require(PlatformResolvePath("resources/new.bin", resolved, sizeof(resolved)));

   // Exact spellings must still win when case-folded names are ambiguous.
   file = std::fopen("Resources/Textures/STONE.bin", "wb");
   require(file != nullptr);
   std::fclose(file);
   // Refresh directory caches after the native filesystem mutation.
   require(PlatformChangeDirectory(temp) == 0);
   require(PlatformAccess("Resources/Textures/Stone.bin", R_OK) == 0);
   require(PlatformAccess("Resources/Textures/STONE.bin", R_OK) == 0);
   require(PlatformAccess("Resources/Textures/sToNe.bin", R_OK) != 0);
   require(PlatformUnlink("Resources/Textures/STONE.bin") == 0);
   require(PlatformUnlink("resources/new.bin") == 0);
   require(PlatformAccess("resources/new.bin", F_OK) != 0);
   require(!PlatformResolvePath("resources/new.bin", resolved, sizeof(resolved)));
   file = PlatformFopen("Resources/NEW.bin", "wb");
   require(file != nullptr);
   std::fclose(file);
   require(PlatformResolvePath("resources/new.bin", resolved, sizeof(resolved)));

   const auto start = std::chrono::steady_clock::now();
   for (int i = 0; i < 20000; ++i)
   {
      require(PlatformAccess("resources/textures/stone.bin", R_OK) == 0);
      file = PlatformFopen("resources/textures/stone.bin", "rb");
      require(file != nullptr);
      std::fclose(file);
      require(PlatformAccess("resources/textures/missing.bin", F_OK) != 0);
      // Diagnostics frequently reopen existing files during mission loads.
      file = PlatformFopen("Resources/NEW.bin", "ab");
      require(file != nullptr);
      std::fclose(file);
   }
   std::printf("20000 resource lookup batches: %.1f ms\n",
      std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count());
   require(PlatformChangeDirectory(previous.c_str()) == 0);
   std::filesystem::remove_all(temp);
}
