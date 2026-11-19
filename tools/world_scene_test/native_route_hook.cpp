// Linux test helper. Feed the normal game keyboard queue on the render thread;
// the native simulation handles movement, turning, collisions and input release.
#include <vulkan/vulkan.h>
#include <dlfcn.h>
#include <link.h>
#include <cstdint>
#include <cstdio>
#include <cstdlib>

#ifndef DARK_INPUT_KEY_OFFSET
#error Compile with the DarkInputKey ELF symbol value from the tested executable
#endif

namespace {
using InputKey = void (*)(unsigned char, int);
InputKey input;
unsigned frames, events;
unsigned warmup;
int mainImage(dl_phdr_info *info, size_t, void *) {
   if (!info->dlpi_name || !info->dlpi_name[0]) {
      input = reinterpret_cast<InputKey>(info->dlpi_addr + DARK_INPUT_KEY_OFFSET);
      return 1;
   }
   return 0;
}
void key(unsigned char scan, bool down) {
   input(scan, down);
   ++events;
}
void advance() {
   if (!input) {
      dl_iterate_phdr(mainImage, nullptr);
      const char *value = std::getenv("DARK_TEST_ROUTE_WARMUP");
      warmup = value ? unsigned(std::strtoul(value, nullptr, 10)) : 120;
      if (!input) std::abort();
   }
   ++frames;
   if (frames <= warmup) return;
   const unsigned phase = (frames - warmup - 1) % 480;
   switch (phase) {
   case 0: key(0x11, true); break;                          // W
   case 60: key(0x11, false); key(0x1f, true); break;        // S
   case 120: key(0x1f, false); key(0xcb, true); break;       // left
   case 150: key(0xcb, false); key(0x11, true); break;
   case 210: key(0x11, false); key(0x1f, true); break;
   case 270: key(0x1f, false); key(0xcd, true); break;       // right
   case 300: key(0xcd, false); key(0x11, true); break;
   case 360: key(0x11, false); key(0x1f, true); break;
   case 420: key(0x1f, false); break;
   }
}
__attribute__((destructor)) void report() {
   std::fprintf(stderr, "Fixture native route world_frames=%u input_events=%u\n", frames, events);
}
}
extern "C" VKAPI_ATTR VkResult VKAPI_CALL vkQueueSubmit(VkQueue queue, uint32_t count,
   const VkSubmitInfo *submits, VkFence fence) {
   static const auto submit = reinterpret_cast<PFN_vkQueueSubmit>(dlsym(RTLD_NEXT, "vkQueueSubmit"));
   const auto result = submit ? submit(queue, count, submits, fence) : VK_ERROR_INITIALIZATION_FAILED;
   bool modern = false;
   for (uint32_t i = 0; i < count; ++i) modern |= submits[i].commandBufferCount == 2;
   if (result == VK_SUCCESS && modern) advance();
   return result;
}
