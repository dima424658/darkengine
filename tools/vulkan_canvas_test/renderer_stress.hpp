#pragma once

#include <chrono>

// Exercise replacement of an already-recorded texture, persistent uploads,
// descriptor reuse/eviction, state changes, and vertex-buffer growth.
static void RunRendererStress(cVulkanContext &context, cVulkanSceneRenderer &renderer,
   const sRenderBackendVertex *quad, int width, int height)
{
   renderer.SetDepth(false, false);
   renderer.SetBlend(kRenderBackendBlendOpaque);
   renderer.SetGamma(1.0f);
   renderer.SetSampler(0, false, false);
   renderer.SetSampler(1, false, false);
   renderer.SetFog(false, 0);
   const std::array<std::array<uint8_t, 4>, 3> colors{{
      {{255, 0, 0, 255}}, {{0, 255, 0, 255}}, {{0, 0, 255, 255}}}};
   void *shared = renderer.CreateTexture(1, 1, colors[0].data(), 4);
   Require(shared != nullptr, "replacement fixture texture");
   vk::CommandBuffer command;
   Require(context.BeginFrame({0, 0, 0, 1}, &command), "replacement frame");
   renderer.BeginFrame(context.FrameIndex());
   const int outputWidth = static_cast<int>(context.SwapchainExtent().width);
   const int outputHeight = static_cast<int>(context.SwapchainExtent().height);
   for (int stripe = 0; stripe < 3; ++stripe)
   {
      if (stripe != 0)
         Require(renderer.UpdateTexture(shared, 1, 1, colors[stripe].data(), 4),
            "replace texture after recorded draw");
      const int left = outputWidth * stripe / 3;
      const int right = outputWidth * (stripe + 1) / 3;
      renderer.SetViewport(left, 0, right - left, outputHeight);
      renderer.BindTexture(0, shared);
      Require(renderer.Draw(command, quad, 6, kRenderBackendTriangles, false, width, height),
         "replacement stripe");
   }
   Require(context.EndFrame(), "replacement submission");
   renderer.DestroyTexture(shared);

   std::array<void *, kVulkanFramesInFlight> references{}, streams{};
   std::vector<double> times;
   for (int frame = 0; frame < 12; ++frame)
   {
      const auto start = std::chrono::steady_clock::now();
      Require(context.BeginFrame({0, 0, 0, 1}, &command), "stress frame");
      const auto slot = context.FrameIndex();
      renderer.BeginFrame(slot);
      const std::array<uint8_t, 4> color{{static_cast<uint8_t>(frame * 37 + 43),
         static_cast<uint8_t>(frame * 53 + 71), static_cast<uint8_t>(frame * 29 + 101), 255}};
      const int side = frame < 6 ? 256 : 512;
      const int rowBytes = side * 4 + 16;
      std::vector<uint8_t> pixels(static_cast<size_t>(rowBytes) * side, 99);
      for (int y = 0; y < side; ++y)
         for (int x = 0; x < side; ++x)
            std::copy(color.begin(), color.end(), pixels.begin() + y * rowBytes + x * 4);
      if (!references[slot])
         references[slot] = renderer.CreateTexture(1, 1, color.data(), 4);
      else Require(renderer.UpdateFrameTexture(references[slot], 1, 1, color.data(), 4),
         "reused reference upload");
      if (!streams[slot])
         streams[slot] = renderer.CreateTexture(side, side, pixels.data(), rowBytes);
      else Require(renderer.UpdateFrameTexture(streams[slot], side, side, pixels.data(), rowBytes),
         "reused padded texture upload");
      Require(references[slot] && streams[slot], "stress textures");
      renderer.SetFog(false, 0);
      renderer.SetBlend(kRenderBackendBlendOpaque);
      renderer.SetSampler(0, false, false);
      renderer.SetViewport(0, 0, outputWidth / 2, outputHeight);
      renderer.BindTexture(0, references[slot]);
      Require(renderer.Draw(command, quad, 6, kRenderBackendTriangles, false, width, height),
         "reference before buffer growth");
      renderer.SetViewport(0, 0, 1, 1);
      renderer.BindTexture(0, streams[slot]);
      // 30,000 quads exceed the initial 8 MiB vertex buffer. Keep rasterization
      // to one pixel so this measures submission work rather than fill rate.
      for (int draw = 0; draw < 30000; ++draw)
      {
         const bool alternate = (draw / 512) % 2 != 0;
         renderer.SetBlend(alternate ? kRenderBackendBlendAlpha : kRenderBackendBlendOpaque);
         renderer.SetSampler(0, alternate, alternate);
         renderer.SetFog(alternate, 0x314159);
         Require(renderer.Draw(command, quad, 6, kRenderBackendTriangles, false, width, height),
            "repeated stress draw");
      }
      renderer.SetFog(false, 0);
      renderer.SetBlend(kRenderBackendBlendOpaque);
      renderer.SetSampler(0, false, false);
      renderer.SetViewport(outputWidth / 2, 0, outputWidth / 2, outputHeight);
      Require(renderer.Draw(command, quad, 6, kRenderBackendTriangles, false, width, height),
         "updated texture after buffer growth");
      Require(context.EndFrame(), "stress submission");
      SDL_PumpEvents();
      if (frame >= 4)
         times.push_back(std::chrono::duration<double, std::milli>(
            std::chrono::steady_clock::now() - start).count());
   }
   double sum = 0;
   for (double time : times) sum += time;
   std::sort(times.begin(), times.end());
   std::printf("RENDERER_STRESS frames=%zu draws_per_frame=30002 mean_ms=%.3f p95_ms=%.3f\n",
      times.size(), sum / times.size(), times.back());
   // Overlay drawing must not invalidate the scene's cached texture, viewport,
   // depth comparison, or depth-write state, including for subsequent frames.
   void *red = renderer.CreateTexture(1, 1, colors[0].data(), 4);
   void *green = renderer.CreateTexture(1, 1, colors[1].data(), 4);
   void *blue = renderer.CreateTexture(1, 1, colors[2].data(), 4);
   Require(red && green && blue, "overlay isolation textures");
   Require(context.BeginFrame({0, 0, 0, 1}, &command), "overlay isolation frame");
   renderer.BeginFrame(context.FrameIndex());
   renderer.SetViewport(0, 0, outputWidth, outputHeight);
   renderer.SetDepth(true, true);
   renderer.SetBlend(kRenderBackendBlendOpaque);
   renderer.SetFog(false, 0);
   renderer.SetSampler(0, true, true);
   renderer.BindTexture(0, red);
   std::array<sRenderBackendVertex, 6> sceneQuad;
   std::copy_n(quad, 6, sceneQuad.begin());
   for (auto &vertex : sceneQuad) vertex.z = 0.2f;
   Require(renderer.Draw(command, sceneQuad.data(), 6, kRenderBackendTriangles,
      false, width, height), "initial scene state");
   Require(renderer.DrawOverlay(command, green, nullptr, width, height,
      vk::Rect2D({0, 0}, {static_cast<uint32_t>(outputWidth / 2),
                         static_cast<uint32_t>(outputHeight)}), false, false), "isolated overlay");
   for (auto &vertex : sceneQuad) vertex.z = 0.1f;
   Require(renderer.Draw(command, sceneQuad.data(), 6, kRenderBackendTriangles,
      false, width, height), "restore scene texture and viewport");
   renderer.BindTexture(0, blue);
   for (auto &vertex : sceneQuad) vertex.z = 0.15f;
   Require(renderer.Draw(command, sceneQuad.data(), 6, kRenderBackendTriangles,
      false, width, height), "restore scene depth comparison and writing");
   Require(context.EndFrame(), "overlay isolation submission");
   renderer.DestroyTexture(red);
   renderer.DestroyTexture(green);
   renderer.DestroyTexture(blue);
   for (void *texture : references) renderer.DestroyTexture(texture);
   for (void *texture : streams) renderer.DestroyTexture(texture);
}
