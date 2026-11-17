#include "canvas_snapshot.hpp"
#include "vulkan_scene_renderer.hpp"
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <algorithm>
#include <cstdio>
#include <memory>
#include <stdexcept>

static void Require(bool condition, const char *message)
{
   if (!condition) throw std::runtime_error(message);
}

static void TestSnapshot()
{
   cCanvasSnapshot canvas;
   uint8_t source[] = {1, 2, 3, 99, 4, 5, 6, 99};
   std::array<uint8_t, 768> palette{};
   Require(canvas.Update(source, 3, 2, 4, 8, palette.data(), true), "initial snapshot");
   const auto generation = canvas.Generation();
   Require(canvas.RowBytes() == 4 && canvas.Pixels()[3] == 0 &&
      canvas.Pixels()[4] == 4, "odd-width row layout");
   source[3] = 80;
   Require(canvas.Update(source, 3, 2, 4, 8, palette.data(), true) &&
      canvas.Generation() == generation, "ignore source padding");
   source[5] = 17;
   Require(canvas.Update(source, 3, 2, 4, 8, palette.data(), true) &&
      canvas.Generation() == generation + 1 && canvas.Pixels()[5] == 17, "dirty row");
   const auto paletteGeneration = canvas.PaletteGeneration();
   palette[0] = 42;
   Require(canvas.Update(source, 3, 2, 4, 8, palette.data(), false) &&
      canvas.PaletteGeneration() == paletteGeneration + 1 &&
      canvas.Generation() == generation + 1 && canvas.Palette()[0] == 42 &&
      !canvas.Transparent(), "palette and transparency without pixel changes");
   Require(!canvas.Update(source, 3, 2, 2, 8, palette.data(), false) &&
      canvas.Generation() == generation + 1, "reject short source stride");
   Require(!canvas.Update(source, 0x7fffffff, 2, 4, 16, nullptr, false), "reject overflow");
   Require(canvas.Update(source, 1, 2, 4, 16, nullptr, false) &&
      canvas.Pixels()[0] == 1 && canvas.Pixels()[4] == 4, "depth and dimensions change");
}

int main(int argc, char **argv)
try
{
   TestSnapshot();
   if (argc == 2 && std::string(argv[1]) == "--snapshot-only")
   {
      std::puts("Canvas snapshot checks passed");
      return 0;
   }
   Require(argc == 3, "Usage: vulkan_canvas_test scene.vert.spv scene.frag.spv");
   Require(SDL_Init(SDL_INIT_VIDEO), SDL_GetError());
   constexpr int width = 257, height = 256;
   std::unique_ptr<SDL_Window, decltype(&SDL_DestroyWindow)> window(
      SDL_CreateWindow("Canvas GPU/reference comparison", width * 4, height * 2,
         SDL_WINDOW_VULKAN), &SDL_DestroyWindow);
   Require(window != nullptr, SDL_GetError());
   uint32_t extensionCount = 0;
   const auto extensions = SDL_Vulkan_GetInstanceExtensions(&extensionCount);
   Require(extensions != nullptr, SDL_GetError());
   cVulkanContext context;
   context.SetVSync(false);
   char error[512]{};
   Require(context.Initialize({extensions, extensions + extensionCount},
      [&](vk::Instance instance) {
         VkSurfaceKHR surface{};
         Require(SDL_Vulkan_CreateSurface(window.get(), static_cast<VkInstance>(instance),
            nullptr, &surface), SDL_GetError());
         return vk::SurfaceKHR(surface);
      }, width * 4, height * 2, -1, error, sizeof(error)), error);
   cVulkanSceneRenderer renderer;
   Require(renderer.Initialize(context, argv[1], argv[2]), "renderer initialization");
   renderer.SetDepth(false, false);
   renderer.SetBlend(kRenderBackendBlendAlpha);
   renderer.SetGamma(1.0f);
   const float w = width, h = height;
   const sRenderBackendVertex quad[] = {
      {0,0,0,1, 1,1,1,1, 1, 0,0, 0,0}, {w,0,0,1, 1,1,1,1, 1, 1,0, 1,0},
      {w,h,0,1, 1,1,1,1, 1, 1,1, 1,1}, {0,0,0,1, 1,1,1,1, 1, 0,0, 0,0},
      {w,h,0,1, 1,1,1,1, 1, 1,1, 1,1}, {0,h,0,1, 1,1,1,1, 1, 0,1, 0,1}};
   for (int depth : {8, 15, 16})
      for (bool smooth : {false, true})
         for (bool transparent : {false, true})
         {
            std::array<uint8_t, 768> palette{};
            for (size_t i = 0; i < palette.size(); ++i)
               palette[i] = static_cast<uint8_t>(i * 47 + 29);
            const int sourceStride = width * (depth == 8 ? 1 : 2) + 3;
            std::vector<uint8_t> source(sourceStride * height, 99);
            std::vector<uint8_t> rgba(width * height * 4);
            for (int y = 0; y < height; ++y)
               for (int x = 0; x < width; ++x)
               {
                  const uint32_t raw = (y * width + x) & (depth == 8 ? 255u : 65535u);
                  auto *in = source.data() + y * sourceStride + x * (depth == 8 ? 1 : 2);
                  in[0] = raw & 255;
                  if (depth != 8) in[1] = raw >> 8;
                  auto *out = rgba.data() + (y * width + x) * 4;
                  if (depth == 8) std::copy_n(palette.data() + raw * 3, 3, out);
                  else
                  {
                     out[0] = ((raw >> (depth == 16 ? 11 : 10)) & 31) * 255 / 31;
                     out[1] = ((raw >> 5) & (depth == 16 ? 63 : 31)) * 255 /
                        (depth == 16 ? 63 : 31);
                     out[2] = (raw & 31) * 255 / 31;
                  }
                  out[3] = transparent && raw == 0 ? 0 : 255;
               }
            cCanvasSnapshot canvas;
            Require(canvas.Update(source.data(), width, height, sourceStride, depth,
               palette.data(), transparent), "fixture snapshot");
            void *reference = renderer.CreateTexture(width, height, rgba.data(), width * 4);
            void *packed = renderer.CreateTexture(canvas.RowBytes() / 4, height,
               canvas.Pixels().data(), canvas.RowBytes());
            void *paletteTexture = renderer.CreateTexture(256, 1, canvas.Palette().data(), 1024);
            Require(reference && packed && paletteTexture, "fixture textures");
            vk::CommandBuffer command;
            Require(context.BeginFrame({0.2f, 0.1f, 0.3f, 1.0f}, &command), "begin frame");
            renderer.BeginFrame(context.FrameIndex());
            renderer.SetViewport(0, 0, width * 2, height * 2);
            renderer.SetSampler(0, false, smooth);
            renderer.BindTexture(0, reference);
            Require(renderer.Draw(command, quad, 6, kRenderBackendTriangles,
               false, width, height, transparent), "reference draw");
            renderer.SetViewport(width * 2, 0, width * 2, height * 2);
            renderer.SetSampler(0, false, false);
            renderer.SetSampler(1, false, false);
            renderer.BindTexture(0, packed);
            renderer.BindTexture(1, paletteTexture);
            const uint32_t mode = depth | (transparent ? 256u : 0u) | (smooth ? 512u : 0u);
            Require(renderer.Draw(command, quad, 6, kRenderBackendTriangles,
               false, width, height, transparent, mode), "GPU decode draw");
            Require(context.EndFrame(), "end frame");
            renderer.DestroyTexture(reference);
            renderer.DestroyTexture(packed);
            renderer.DestroyTexture(paletteTexture);
            SDL_PumpEvents();
         }
   std::puts("Rendered 12 canvas comparisons: 8/15/16-bit, nearest/linear, opaque/transparent");
   renderer.Shutdown();
   context.Shutdown();
   window.reset();
   SDL_Quit();
   return 0;
}
catch (const std::exception &error)
{
   std::fprintf(stderr, "%s\n", error.what());
   return 1;
}
