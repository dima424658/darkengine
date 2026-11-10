#ifndef DARK_VULKAN_SCENE_RENDERER_HPP
#define DARK_VULKAN_SCENE_RENDERER_HPP

#include "vulkan_context.hpp"
#include <render_backend.h>

#include <array>
#include <memory>
#include <unordered_map>
#include <vector>

class cVulkanSceneRenderer
{
public:
   cVulkanSceneRenderer();
   ~cVulkanSceneRenderer();
   cVulkanSceneRenderer(const cVulkanSceneRenderer &) = delete;
   cVulkanSceneRenderer &operator=(const cVulkanSceneRenderer &) = delete;

   bool Initialize(cVulkanContext &context, const char *vertexShaderPath,
      const char *fragmentShaderPath);
   void Shutdown();
   void ResetPipelines();
   void BeginFrame(uint32_t frameIndex);
   bool Draw(vk::CommandBuffer commandBuffer,
      const sRenderBackendVertex *vertices, int vertexCount, int primitive,
      bool useSecondTexture, int logicalWidth, int logicalHeight,
      bool transparentBlack = false);
   void ClearDepth(vk::CommandBuffer commandBuffer);
   void ClearDepthRect(vk::CommandBuffer commandBuffer, int x, int y,
      int width, int height, int logicalWidth, int logicalHeight);
   void ClearCanvasRegion(vk::CommandBuffer commandBuffer, int x, int y,
      int width, int height, int logicalWidth, int logicalHeight);

   void SetDepth(bool compare, bool write);
   void SetBlend(int mode);
   void SetAlphaTest(bool enabled);
   void SetFog(bool enabled, uint32_t rgb);
   void SetGamma(float gamma);
   void SetViewport(int x, int y, int width, int height);
   void SetSampler(int level, bool wrap, bool smooth);
   void BindTexture(int level, void *texture);
   void *CreateTexture(int width, int height, const void *rgba, int rowBytes);
   bool UpdateTexture(void *texture, int width, int height, const void *rgba,
      int rowBytes);
   // Updates a texture owned by this frame slot after BeginFrame has waited
   // for its previous submission to finish. Do not use for shared textures.
   bool UpdateFrameTexture(void *texture, int width, int height,
      const void *rgba, int rowBytes);
   void DestroyTexture(void *texture) noexcept;

private:
   struct sTexture;
   struct sVertexBuffer;
   struct sFrameResources;
   struct sPipelineKey;
   vk::Pipeline GetPipeline(int primitive);
   vk::Sampler GetSampler(bool wrap, bool smooth) const;
   void AllocateVertexBuffer(sVertexBuffer &buffer,
      vk::DeviceSize capacity);
   bool CreateTextureImage(sTexture &texture, int width, int height,
      const void *rgba, int rowBytes);
   bool UpdateTextureInternal(void *texture, int width, int height,
      const void *rgba, int rowBytes, bool frameSlotIdle);
   void DestroyTextureImage(sTexture &texture);

   cVulkanContext *m_context{};
   vk::raii::DescriptorSetLayout m_descriptorLayout{nullptr};
   vk::raii::PipelineLayout m_pipelineLayout{nullptr};
   vk::raii::ShaderModule m_vertexShader{nullptr};
   vk::raii::ShaderModule m_fragmentShader{nullptr};
   vk::raii::Sampler m_samplers[2][2]{{nullptr, nullptr}, {nullptr, nullptr}};
   std::vector<vk::raii::Pipeline> m_pipelines;
   std::unique_ptr<sTexture> m_whiteTexture;
   std::unordered_map<sTexture *, std::unique_ptr<sTexture>> m_textures;
   std::array<std::vector<std::unique_ptr<sTexture>>,
      kVulkanFramesInFlight> m_deferredTextures;
   std::array<std::unique_ptr<sFrameResources>,
      kVulkanFramesInFlight> m_frames;
   void *m_boundTextures[2]{};
   bool m_depthCompare{};
   bool m_depthWrite{};
   bool m_alphaTest{};
   int m_blendMode{};
   bool m_fog{};
   uint32_t m_fogColor{};
   float m_gamma{1.0f};
   bool m_wrap[2]{};
   bool m_smooth[2]{true, true};
   uint32_t m_frameIndex{};
   int m_viewportX{};
   int m_viewportY{};
   int m_viewportWidth{};
   int m_viewportHeight{};
};

#endif
