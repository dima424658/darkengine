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

   cCanvasSnapshot partial;
   uint8_t rows[] = {1,0, 0,0, 2,0, 99,99,
                    0,0, 0,0, 0,0, 99,99,
                    0,0, 3,0, 0,0, 99,99};
   Require(partial.UpdateRect(rows, 3, 3, 8, 16, nullptr, true, 1, 2, 2, 3),
      "first partial flush initializes whole snapshot");
   Require(partial.Pixels()[0] == 1 && partial.Pixels()[4] == 2,
      "first flush includes unflushed source pixels");
   auto generationBeforePatch = partial.Generation();
   rows[0] = 77; // Outside the flush: must remain uncommitted.
   rows[18] = 0;
   Require(partial.UpdateRect(rows, 3, 3, 8, 16, nullptr, true, 1, 2, 2, 3) &&
      partial.Pixels()[0] == 1 && partial.Pixels()[18] == 0 &&
      partial.Generation() == generationBeforePatch + 1, "partial commit and removed pixel");
   auto cleared = partial.Pixels();
   for (int y = 0; y < 3; ++y) cleared.at(y * 8 + 6) = cleared.at(y * 8 + 7) = 99;
   Require(partial.ClearOverlay(cleared.data(), 3, 3, 8, 16), "sparse overlay clear");
   for (int y = 0; y < 3; ++y)
   {
      for (int x = 0; x < 6; ++x) Require(cleared[y * 8 + x] == 0, "removed overlay pixel");
      Require(cleared[y * 8 + 6] == 99 && cleared[y * 8 + 7] == 99, "clear preserves padding");
   }
   Require(!partial.ClearOverlay(cleared.data(), 2, 3, 8, 16), "clear rejects different canvas");
   Require(partial.Update(cleared.data(), 3, 3, 8, 16, nullptr, true) &&
      partial.Generation() == generationBeforePatch + 2, "cleared HUD committed");
   Require(partial.UpdateRect(rows, 3, 3, 8, 16, nullptr, true, -5, -5, 1, 1) &&
      partial.Pixels()[0] == 77 && partial.Pixels()[4] == 0, "clipped rectangle");
   const auto unchanged = partial.Generation();
   Require(partial.UpdateRect(rows, 3, 3, 8, 16, nullptr, false, 50, 50, 60, 60) &&
      partial.Generation() == unchanged && !partial.Transparent(), "empty clipped flush");

}

#include "renderer_stress.hpp"

int main(int argc, char **argv)
try
{
   TestSnapshot();
   if (argc == 2 && std::string(argv[1]) == "--snapshot-only")
   {
      std::puts("Canvas snapshot checks passed");
      return 0;
   }
   const bool stress = argc == 4 && std::string(argv[3]) == "--stress";
   Require(argc == 3 || stress, "Usage: vulkan_canvas_test scene.vert.spv scene.frag.spv [--stress]");
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
   if (stress) RunRendererStress(context, renderer, quad, width, height);
   else for (int depth : {8, 15, 16})
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
            // Commit one pixel while deliberately changing another outside
            // the flush, verifying packed-texel neighbors remain retained.
            const int bytesPerPixel = depth == 8 ? 1 : 2;
            std::fill_n(source.data() + bytesPerPixel, bytesPerPixel, 0);
            std::fill_n(source.data(), bytesPerPixel, 0xff);
            Require(canvas.UpdateRect(source.data(), width, height, sourceStride, depth,
               palette.data(), transparent, 1, 0, 2, 1), "partial fixture update");
            if (depth == 8) std::copy_n(palette.data(), 3, rgba.data() + 4);
            else std::fill_n(rgba.data() + 4, 3, 0);
            rgba[7] = transparent ? 0 : 255;
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
            // Material passes temporarily replace textures, samplers, blend,
            // fog and viewport. The following draw must retain the base state.
            renderer.PushDrawState();
            renderer.BindTexture(0, packed);
            renderer.SetSampler(0, true, !smooth);
            renderer.SetBlend(kRenderBackendBlendMultiply);
            renderer.SetAlphaTest(true);
            renderer.SetFog(true, 0xff00ff);
            renderer.SetViewport(1, 1, 1, 1);
            renderer.PushDrawState();
            renderer.SetDepth(true, true);
            renderer.PopDrawState();
            renderer.PopDrawState();
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
   if (!stress) std::puts("Rendered 12 canvas comparisons: 8/15/16-bit, nearest/linear, opaque/transparent");
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
