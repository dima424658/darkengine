#include "vulkan_dlss.hpp"
#include <nvsdk_ngx_vk.h>
#include <nvsdk_ngx_helpers_vk.h>
#include <nvsdk_ngx_helpers.h>
#include <algorithm>
#include <filesystem>
#include <stdexcept>
#include <cstdio>
#include <cstring>
#include <set>
namespace dark {
namespace {
void require(NVSDK_NGX_Result result, const char *operation) {
   if (NVSDK_NGX_FAILED(result)) {
      char code[32]; std::snprintf(code, sizeof(code), " (NGX 0x%08x)", unsigned(result));
      throw std::runtime_error(std::string(operation) + code);
   }
}
NVSDK_NGX_FeatureDiscoveryInfo discovery() {
   NVSDK_NGX_FeatureDiscoveryInfo info{};
   info.SDKVersion = NVSDK_NGX_Version_API; info.FeatureID = NVSDK_NGX_Feature_SuperSampling;
   info.Identifier.IdentifierType = NVSDK_NGX_Application_Identifier_Type_Project_Id;
   info.Identifier.v.ProjectDesc = {"1757ab8a-b589-44df-9c37-61b2c27543ab", NVSDK_NGX_ENGINE_TYPE_CUSTOM, "1.0"};
   info.ApplicationDataPath = L".";
   return info;
}
void append(std::vector<const char *> &out, const char *name) {
   if (std::none_of(out.begin(), out.end(), [&](const char *s) { return std::strcmp(s, name) == 0; })) {
      static std::set<std::string> names; // SDK queries can reuse their returned storage.
      out.push_back(names.emplace(name).first->c_str());
   }
}
}
struct VulkanDlss::Impl {
   vk::Device device;
   NVSDK_NGX_Parameter *parameters = nullptr;
   NVSDK_NGX_Handle *feature = nullptr;
   NVSDK_NGX_PerfQuality_Value quality = NVSDK_NGX_PerfQuality_Value_MaxQuality;
   uint32_t width = 0, height = 0, outputWidth = 0, outputHeight = 0;
   bool initialized = false, history = false;
   ~Impl() {
      if (feature) NVSDK_NGX_VULKAN_ReleaseFeature(feature);
      if (parameters) NVSDK_NGX_VULKAN_DestroyParameters(parameters);
      if (initialized) NVSDK_NGX_VULKAN_Shutdown1(device);
   }
};
VulkanDlss::VulkanDlss() : m(std::make_unique<Impl>()) {}
VulkanDlss::~VulkanDlss() = default;
void VulkanDlss::InstanceExtensions(std::vector<const char *> &extensions) {
   uint32_t count = 0; VkExtensionProperties *required = nullptr; const auto info = discovery();
   if (NVSDK_NGX_FAILED(NVSDK_NGX_VULKAN_GetFeatureInstanceExtensionRequirements(&info, &count, &required))) return;
   const auto available = vk::enumerateInstanceExtensionProperties();
   for (unsigned i = 0; i < count; ++i)
      if (std::any_of(available.begin(), available.end(), [&](const auto &e) {return std::strcmp(e.extensionName, required[i].extensionName) == 0;}))
         append(extensions, required[i].extensionName);
}
bool VulkanDlss::DeviceExtensions(vk::Instance instance, vk::PhysicalDevice physical, std::vector<const char *> &extensions) {
   uint32_t count = 0; VkExtensionProperties *required = nullptr; const auto info = discovery();
   if (NVSDK_NGX_FAILED(NVSDK_NGX_VULKAN_GetFeatureDeviceExtensionRequirements(instance, physical, &info, &count, &required))) return false;
   const auto available = physical.enumerateDeviceExtensionProperties();
   for (unsigned i = 0; i < count; ++i)
      if (std::none_of(available.begin(), available.end(), [&](const auto &e) {return std::strcmp(e.extensionName, required[i].extensionName) == 0;})) return false;
   for (unsigned i = 0; i < count; ++i) append(extensions, required[i].extensionName);
   return true;
}

bool VulkanDlss::Initialize(vk::Instance instance, vk::PhysicalDevice physical, vk::Device device, Quality quality,
                            std::string &error) {
   m = std::make_unique<Impl>();
   if (quality == Quality::Off) return true;
   try {
      std::vector<const char *> extensions;
      if (!DeviceExtensions(instance, physical, extensions)) throw std::runtime_error("required DLSS Vulkan extensions unavailable");
      m->device = device;
      switch (quality) {
      case Quality::Balanced: m->quality = NVSDK_NGX_PerfQuality_Value_Balanced; break;
      case Quality::Performance: m->quality = NVSDK_NGX_PerfQuality_Value_MaxPerf; break;
      case Quality::Dlaa: m->quality = NVSDK_NGX_PerfQuality_Value_DLAA; break;
      default: break;
      }
      const std::wstring search = std::filesystem::path(DARK_DLSS_RUNTIME_DIRECTORY).wstring();
      const wchar_t *paths[] = {search.c_str()};
      NVSDK_NGX_FeatureCommonInfo info{};
      info.PathListInfo.Path = paths; info.PathListInfo.Length = 1;
      const auto logs = std::filesystem::temp_directory_path() / "darkengine-ngx";
      std::filesystem::create_directories(logs);
      require(NVSDK_NGX_VULKAN_Init_with_ProjectID("1757ab8a-b589-44df-9c37-61b2c27543ab", NVSDK_NGX_ENGINE_TYPE_CUSTOM,
         "1.0", logs.wstring().c_str(), instance, physical, device, vkGetInstanceProcAddr, vkGetDeviceProcAddr, &info), "initialize DLSS");
      m->initialized = true;
      require(NVSDK_NGX_VULKAN_GetCapabilityParameters(&m->parameters), "query DLSS capabilities");
      int available = 0;
      require(m->parameters->Get(NVSDK_NGX_Parameter_SuperSampling_Available, &available), "query DLSS availability");
      // Use the K transformer for Performance too. The SDK's default M path
      // leaves two internal images undefined on first evaluation with 310.9.16.
      // K shares the validated model used by Quality/Balanced/DLAA.
      m->parameters->Set(NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_Performance,
                         unsigned(NVSDK_NGX_DLSS_Hint_Render_Preset_K));
      if (!available) throw std::runtime_error("DLSS Super Resolution unavailable on this GPU/driver");
      return true;
   } catch (const std::exception &e) { error = e.what(); m = std::make_unique<Impl>(); return false; }
}
bool VulkanDlss::Enabled() const { return m->initialized; }
bool VulkanDlss::Resolution(uint32_t ow, uint32_t oh, uint32_t &w, uint32_t &h, std::string &error) {
   if (!Enabled()) return false;
   try {
      if (m->outputWidth != ow || m->outputHeight != oh) {
         // Feature history is shared by the ordered queue, not by frame slots.
         // Retire both slots before releasing a feature on output resize.
         m->device.waitIdle();
         if (m->feature) { NVSDK_NGX_VULKAN_ReleaseFeature(m->feature); m->feature = nullptr; }
         unsigned maxW, maxH, minW, minH; float sharpness;
         require(NGX_DLSS_GET_OPTIMAL_SETTINGS(m->parameters, ow, oh, m->quality, &m->width, &m->height,
            &maxW, &maxH, &minW, &minH, &sharpness), "query DLSS render resolution");
         if (!m->width || !m->height || m->width > ow || m->height > oh) throw std::runtime_error("invalid DLSS render resolution");
         m->outputWidth = ow; m->outputHeight = oh; m->history = false;
      }
      w = m->width; h = m->height; return true;
   } catch (const std::exception &e) { error = e.what(); return false; }
}
bool VulkanDlss::Record(vk::CommandBuffer command, const Texture &color, const Texture &depth, const Texture &motion,
                        const Texture &output, float jx, float jy, bool reset, std::string &error) {
   try {
      if (!m->feature) {
         NVSDK_NGX_DLSS_Create_Params create{};
         create.Feature.InWidth = color.width; create.Feature.InHeight = color.height;
         create.Feature.InTargetWidth = output.width; create.Feature.InTargetHeight = output.height;
         create.Feature.InPerfQualityValue = m->quality;
         create.InFeatureCreateFlags = NVSDK_NGX_DLSS_Feature_Flags_IsHDR | NVSDK_NGX_DLSS_Feature_Flags_MVLowRes |
                                       NVSDK_NGX_DLSS_Feature_Flags_AutoExposure;
         require(NGX_VULKAN_CREATE_DLSS_EXT1(m->device, command, 1, 1, &m->feature, m->parameters, &create), "create DLSS feature");
         m->history = false;
      }
      auto resource = [](const Texture &t, bool write) {
         return NVSDK_NGX_Create_ImageView_Resource_VK(t.view, t.image, {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1},
            VkFormat(t.format), t.width, t.height, write);
      };
      auto c = resource(color, false), d = resource(depth, false), mv = resource(motion, false), out = resource(output, true);
      NVSDK_NGX_VK_DLSS_Eval_Params eval{};
      eval.Feature.pInColor = &c; eval.Feature.pInOutput = &out;
      eval.pInDepth = &d; eval.pInMotionVectors = &mv;
      eval.InJitterOffsetX = jx; eval.InJitterOffsetY = jy;
      eval.InMVScaleX = eval.InMVScaleY = 1;
      eval.InRenderSubrectDimensions = {color.width, color.height};
      eval.InReset = reset || !m->history;
      eval.InPreExposure = eval.InExposureScale = 1;
      require(NGX_VULKAN_EVALUATE_DLSS_EXT(command, m->feature, m->parameters, &eval), "evaluate DLSS");
      return true;
   } catch (const std::exception &e) { error = e.what(); m->history = false; return false; }
}
void VulkanDlss::CompleteSubmission(bool accepted) {
   m->history = accepted;
   if (!accepted && m->feature) {
      // NGX updates internal layout bookkeeping while recording. A rejected
      // recording cannot be repaired with InReset alone; recreate the feature.
      m->device.waitIdle();
      NVSDK_NGX_VULKAN_ReleaseFeature(m->feature); m->feature = nullptr;
   }
}
}
