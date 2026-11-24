#include "../../libsrc/lgd3d/vulkan_world_renderer.hpp"
#include "../../libsrc/lgd3d/vulkan_world_features.hpp"
#include <vulkan/vulkan_raii.hpp>
#include <cmath>
#include <cstdio>
#include <stdexcept>
#include <atomic>
static void check(bool ok, const char *message) { if (!ok) throw std::runtime_error(message); }
static std::atomic_uint validationErrors{0};
static VKAPI_ATTR VkBool32 VKAPI_CALL validation(VkDebugUtilsMessageSeverityFlagBitsEXT severity,
   VkDebugUtilsMessageTypeFlagsEXT, const VkDebugUtilsMessengerCallbackDataEXT *data, void *) {
   if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
      ++validationErrors; std::fprintf(stderr,"Validation Error: %s\n",data->pMessage);
   }
   return VK_FALSE;
}
struct Messenger {
   VkInstance instance{}; VkDebugUtilsMessengerEXT messenger{};
   ~Messenger() {
      if (messenger) reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance,"vkDestroyDebugUtilsMessengerEXT"))(instance,messenger,nullptr);
   }
};
int main(int argc, char **argv) {
 try {
   check(argc == 2, "pass generated Vulkan shader directory");
   vk::raii::Context context;
   std::vector<const char *> extensions;
   dark::VulkanDlss::InstanceExtensions(extensions);
   extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
   vk::ApplicationInfo app("Dark DLSS test", 1, "Dark", 1, VK_API_VERSION_1_1);
   auto instance = context.createInstance(vk::InstanceCreateInfo({}, &app, 0, nullptr, uint32_t(extensions.size()), extensions.data()));
   Messenger messenger; messenger.instance = *instance;
   VkDebugUtilsMessengerCreateInfoEXT debug{VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};
   debug.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
   debug.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
   debug.pfnUserCallback = validation;
   const auto createMessenger = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(*instance,"vkCreateDebugUtilsMessengerEXT"));
   check(createMessenger && createMessenger(*instance,&debug,nullptr,&messenger.messenger)==VK_SUCCESS,"validation messenger");
   for (auto physical : (*instance).enumeratePhysicalDevices()) {
      dark::VulkanWorldFeatures features;
      if (!features.Probe(physical, true) || !dark::VulkanDlss::DeviceExtensions(*instance, physical, features.extensions)) continue;
      const auto families = physical.getQueueFamilyProperties(); uint32_t family = 0;
      while (family < families.size() && !(families[family].queueFlags & vk::QueueFlagBits::eGraphics)) ++family;
      check(family < families.size(), "no graphics queue");
      const float priority = 1;
      vk::DeviceQueueCreateInfo qi({}, family, 1, &priority);
      vk::DeviceCreateInfo di({}, 1, &qi, 0, nullptr, uint32_t(features.extensions.size()), features.extensions.data());
      di.pNext = &features.features;
      auto device = physical.createDeviceUnique(di); auto queue = device->getQueue(family, 0);
      auto pool = device->createCommandPoolUnique(vk::CommandPoolCreateInfo(vk::CommandPoolCreateFlagBits::eResetCommandBuffer, family));
      auto commands = device->allocateCommandBuffersUnique(vk::CommandBufferAllocateInfo(*pool, vk::CommandBufferLevel::ePrimary, 1));
      const std::array<vk::AttachmentDescription, 2> attachments{{
         {{}, vk::Format::eR8G8B8A8Unorm, vk::SampleCountFlagBits::e1, vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore,
          vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare, vk::ImageLayout::eUndefined, vk::ImageLayout::eColorAttachmentOptimal},
         {{}, vk::Format::eD32Sfloat, vk::SampleCountFlagBits::e1, vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore,
          vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare, vk::ImageLayout::eUndefined, vk::ImageLayout::eDepthStencilAttachmentOptimal}}};
      vk::AttachmentReference color(0, vk::ImageLayout::eColorAttachmentOptimal), depth(1, vk::ImageLayout::eDepthStencilAttachmentOptimal);
      vk::SubpassDescription subpass({}, vk::PipelineBindPoint::eGraphics, 0, nullptr, 1, &color, nullptr, &depth);
      auto pass = device->createRenderPassUnique(vk::RenderPassCreateInfo({}, attachments, subpass));
      dark::VulkanWorldRenderer renderer; std::string error;
      check(renderer.Initialize(physical, *device, family, *pass, vk::Format::eD32Sfloat, vk::Format::eR8G8B8A8Unorm, argv[1], error), error.c_str());
      dark::VulkanWorldRenderer::RaySettings rays; rays.gi = false; rays.sharc = false; rays.denoiserFrameTimeMilliseconds = 1000.f/60;
      renderer.SetRaySettings(rays);
      auto output = device->createBufferUnique(vk::BufferCreateInfo({}, 8, vk::BufferUsageFlagBits::eTransferDst));
      const auto requirements = device->getBufferMemoryRequirements(*output);
      const auto memory = physical.getMemoryProperties();
      uint32_t type = 0; const auto wanted = vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent;
      while (type < memory.memoryTypeCount && (!(requirements.memoryTypeBits & (1u << type)) || (memory.memoryTypes[type].propertyFlags & wanted) != wanted)) ++type;
      check(type < memory.memoryTypeCount, "no readback memory");
      auto allocation = device->allocateMemoryUnique(vk::MemoryAllocateInfo(requirements.size, type));
      device->bindBufferMemory(*output, *allocation, 0);
      const auto *mapped = static_cast<const uint16_t *>(device->mapMemory(*allocation, 0, 8));
      auto submit = [&](vk::CommandBuffer cmd) {
         auto fence = device->createFenceUnique({}); vk::SubmitInfo info({}, {}, cmd);
         queue.submit(info, *fence); check(device->waitForFences(*fence, VK_TRUE, UINT64_MAX) == vk::Result::eSuccess, "GPU wait failed");
      };
      auto setup = [&] {
         RenderSceneClear();
         RenderSceneMaterial material{}; material.doubleSided = 1; material.baseColor[3] = 1;
         material.emission[0] = 2; material.emission[1] = 1; material.emission[2] = .5f; material.roughness = .8f;
         const auto mat = RenderSceneCreateMaterial(&material);
         RenderSceneVertex vertices[3] = {{{-8,-8,0},{0,0,-1},{0,0}}, {{8,-8,0},{0,0,-1},{1,0}}, {{0,8,0},{0,0,-1},{.5f,1}}};
         uint32_t indices[] = {0,1,2}; RenderSceneMesh mesh{vertices,3,indices,3,mat,1};
         RenderSceneInstance data{}; data.mesh = RenderSceneCreateMesh(&mesh); data.visible = 1;
         for (unsigned i=0; i<4; ++i) data.transform[i*5] = data.previousTransform[i*5] = 1;
         check(RenderSceneCreateInstance(&data) != 0, "create instance");
         RenderSceneCamera camera{}; camera.exposure = 1; camera.lightScale = 1;
         const float eye[] = {0,0,-3}, forward[] = {0,0,1}, right[] = {.1f,0,1}, bottom[] = {0,.1f,1};
         check(RenderSceneCameraFromRays(&camera, eye, forward, right, bottom, .1f, 100), "camera");
         check(RenderSceneSetCamera(&camera), "set camera");
      };
      auto sample = [&](uint32_t slot, uint32_t w, uint32_t h) {
         const auto cmd = renderer.Record(slot, dark::GetWorldScene(), w, h, error);
         check(cmd != VK_NULL_HANDLE, error.c_str()); submit(cmd); renderer.CompleteSubmission(true);
         auto read = *commands[0]; read.reset(); read.begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));
         const vk::Image image = renderer.UpscaledImage(slot);
         vk::ImageMemoryBarrier barrier(vk::AccessFlagBits::eMemoryWrite, vk::AccessFlagBits::eTransferRead,
            vk::ImageLayout::eShaderReadOnlyOptimal, vk::ImageLayout::eTransferSrcOptimal,
            VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED, image, {vk::ImageAspectFlagBits::eColor,0,1,0,1});
         read.pipelineBarrier(vk::PipelineStageFlagBits::eAllCommands, vk::PipelineStageFlagBits::eTransfer, {}, {}, {}, barrier);
         read.copyImageToBuffer(image, vk::ImageLayout::eTransferSrcOptimal, *output,
            vk::BufferImageCopy(0,0,0,{vk::ImageAspectFlagBits::eColor,0,0,1},{int32_t(w/2),int32_t(h/2),0},{1,1,1}));
         barrier.oldLayout = vk::ImageLayout::eTransferSrcOptimal; barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
         barrier.srcAccessMask = vk::AccessFlagBits::eTransferRead; barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
         read.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eAllCommands, {}, {}, {}, barrier);
         vk::MemoryBarrier host(vk::AccessFlagBits::eTransferWrite, vk::AccessFlagBits::eHostRead);
         read.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eHost, {}, host, {}, {});
         read.end(); submit(read);
         for (unsigned c=0; c<3; ++c) {
            const unsigned exponent = (mapped[c] >> 10) & 31, mantissa = mapped[c] & 1023;
            const float value = exponent ? std::ldexp(1+mantissa/1024.f, int(exponent)-15) : std::ldexp(float(mantissa),-24);
            const float expected[] = {2,1,.5f};
            check(!(mapped[c]&0x8000) && exponent != 31 && std::abs(value-expected[c]) < .2f, "DLSS HDR output differs from emissive surface");
         }
      };
      using Quality = dark::VulkanDlss::Quality;
      for (auto quality : {Quality::Quality, Quality::Balanced, Quality::Performance, Quality::Dlaa}) {
         check(renderer.ConfigureDlss(*instance, quality, error), error.c_str()); check(renderer.DlssEnabled(), "DLSS not active");
         setup();
         for (unsigned frame=0; frame<8; ++frame) sample(frame%2, 640, 360);
         const auto extent = renderer.RenderExtent(1);
         check(quality == Quality::Dlaa ? extent == std::array<uint32_t,2>{640,360} : extent[0]<640 && extent[1]<360, "render resolution mode");
         sample(0,800,450); sample(1,800,450); // Shared feature and both frame targets resize.
         setup(); sample(0,800,450); // Scene history reset.
         check(renderer.Record(1,dark::GetWorldScene(),800,450,error)!=VK_NULL_HANDLE,error.c_str());
         renderer.CompleteSubmission(false); // Discard an unsubmitted recording.
         sample(0,800,450); sample(1,800,450);
         std::printf("DLSS mode %u passed: %ux%u -> 640x360, resize/reset/rejection\n", unsigned(quality), extent[0],extent[1]);
      }
      check(renderer.ConfigureDlss(*instance,Quality::Off,error),error.c_str()); check(!renderer.DlssEnabled(),"DLSS off");
      sample(0,640,360); check(renderer.RenderExtent(0)==std::array<uint32_t,2>{640,360},"native extent");
      device->waitIdle(); renderer.Shutdown(); device->unmapMemory(*allocation);
      RenderSceneClear(); check(validationErrors==0,"Vulkan validation errors"); std::puts("DLSS GPU checks passed"); return 0;
   }
   std::puts("SKIP: no Vulkan RTX/DLSS device"); return 77;
 } catch (const std::exception &e) { std::fprintf(stderr,"DLSS test failed: %s\n",e.what()); return 1; }
}
