#include "vulkan_world_renderer.hpp"
namespace dark {
// Keep the native renderer usable without SDK sources, compilers or RT features.
struct VulkanWorldRenderer::Impl {};
VulkanWorldRenderer::VulkanWorldRenderer() : m(std::make_unique<Impl>()) {
}
VulkanWorldRenderer::~VulkanWorldRenderer() = default;
bool VulkanWorldRenderer::Initialize(vk::PhysicalDevice, vk::Device, uint32_t, vk::RenderPass, vk::Format, vk::Format,
                                     const std::string &, std::string &error) {
   error = "RTX SDK lighting was disabled at build time";
   return false;
}
VkCommandBuffer VulkanWorldRenderer::Record(uint32_t, const WorldScene &, uint32_t, uint32_t, std::string &error) {
   error = "RTX SDK lighting is unavailable";
   return VK_NULL_HANDLE;
}
bool VulkanWorldRenderer::ConfigureDlss(vk::Instance, VulkanDlss::Quality, std::string &error) {
   error = "DLSS requires the RTX world renderer"; return false;
}
bool VulkanWorldRenderer::DlssEnabled() const { return false; }
VkImage VulkanWorldRenderer::UpscaledImage(uint32_t) const { return VK_NULL_HANDLE; }
std::array<uint32_t, 2> VulkanWorldRenderer::RenderExtent(uint32_t) const { return {}; }
void VulkanWorldRenderer::CompleteSubmission(bool) {
}
void VulkanWorldRenderer::Composite(vk::CommandBuffer, uint32_t, int, int, uint32_t, uint32_t) {
}
VkImage VulkanWorldRenderer::HdrImage(uint32_t) const {
   return VK_NULL_HANDLE;
}
VkImage VulkanWorldRenderer::NativeDepthImage(uint32_t) const {
   return VK_NULL_HANDLE;
}
VkImage VulkanWorldRenderer::SurfaceImage(uint32_t, uint32_t) const {
   return VK_NULL_HANDLE;
}
VkImage VulkanWorldRenderer::DirectImage(uint32_t, bool, bool) const {
   return VK_NULL_HANDLE;
}
std::array<uint32_t, 4> VulkanWorldRenderer::IndirectStatistics(uint32_t) const {
   return {};
}
VulkanWorldRenderer::Workload VulkanWorldRenderer::GetWorkload(uint32_t) const {
   return {};
}
bool VulkanWorldRenderer::RayTracingEnabled() const {
   return false;
}
void VulkanWorldRenderer::SetRaySettings(const RaySettings &) {
}
VulkanWorldRenderer::TextureWorkload VulkanWorldRenderer::GetTextureWorkload(uint32_t) const {
   return {};
}
void VulkanWorldRenderer::SetTextureCacheBudget(uint64_t) {
}
void VulkanWorldRenderer::Shutdown() {
}
} // namespace dark
