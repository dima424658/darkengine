#include "material.h"
#include "material_parser.hpp"
#include <resman.h>
#include <resapi.h>
#include <reshelp.h>
#include <storeapi.h>
#include <imgrstyp.h>
#include <resthred.h>
#include <bitmap.h>
#include <config.h>
#include <mprintf.h>
#include <render_backend.h>

#include <memory>
#include <unordered_map>
#include <cmath>

namespace
{
struct Entry
{
   DarkMaterial::Material material;
   IResManHelper *manager = nullptr; // resource manager outlives loaded image data
   std::string path;
   void *renderData = nullptr;
   void (*releaseRenderData)(void *) = nullptr;
   void ReleaseRenderData()
   {
      if (renderData && releaseRenderData)
         releaseRenderData(renderData);
      renderData = nullptr;
   }
   IRes *illumination = nullptr;
   grs_bitmap *illuminationBitmap = nullptr;
   bool mesh = false;
   bool illuminationRead = false;
   bool uvTraced = false;
   ~Entry()
   {
      ReleaseRenderData();
      if (illumination)
      {
         if (illuminationBitmap)
            illumination->Unlock();
         illumination->Release();
      }
   }
};
auto &entries()
{
   static auto *map = new std::unordered_map<const grs_bitmap *, std::shared_ptr<Entry>>;
   return *map;
}
auto &terrain()
{
   // Each frame retains its physical base projection dimensions even when
   // it shares root-authored settings or has several CPU mip levels.
   struct Projection
   {
      float width;
      float height;
   };
   static auto *map = new std::unordered_map<const grs_bitmap *, Projection>;
   return *map;
}
void ApplyFlags(const sTextureMaterial &s, grs_bitmap *bitmap)
{
   if (s.forceOpaque == 1)
      bitmap->flags &= ~(BMF_TRANS | BMF_TLUC);
   else if (s.fullAlpha == 1)
   {
      bitmap->flags |= BMF_TLUC;
      bitmap->flags &= ~BMF_TRANS;
   }
   else if (s.alphaKey == 1)
   {
      bitmap->flags |= BMF_TRANS;
      bitmap->flags &= ~BMF_TLUC;
   }
}
bool Read(IRes *resource, std::string &text)
{
   if (!resource)
      return false;
   const auto size = resource->GetSize();
   bool success = false;
   if (size > 0 && size <= 1024 * 1024)
   {
      const auto *data = static_cast<const char *>(resource->Lock());
      if (data)
      {
         text.assign(data, size);
         resource->Unlock();
         success = true;
      }
   }
   resource->Release();
   return success;
}
} // namespace
void ResMaterialLoad(IResManHelper *helper, IRes *image, grs_bitmap *bitmap)
{
   cAutoResThreadLock lock;
   auto *manager = static_cast<cResMan *>(helper);
   const std::string name = std::string(image->GetName()) + ".mtl";
   std::string initial;
   if (!Read(manager->BindMaterial(image, name.c_str()), initial))
      return;
   auto entry = std::make_shared<Entry>();
   entry->manager = helper;
   char *path = nullptr;
   image->GetCanonPath(&path);
   if (path)
   {
      entry->path = path;
      Free(path);
   }
   entry->mesh = DarkMaterial::Lower(entry->path).compare(0, 4, "mesh") == 0;
   const auto reader = [&](const std::string &file, std::string &text)
   {
      if (!stricmp(file.c_str(), name.c_str()))
      {
         text = initial;
         return true;
      }
      return Read(manager->BindMaterial(image, file.c_str()), text);
   };
   const auto condition = [](const std::string &key)
   {
      if (!config_is_defined(key.c_str()))
         return false;
      int value = 1;
      config_get_int(key.c_str(), &value);
      return value != 0;
   };
   entry->material = DarkMaterial::Parser(reader, condition).Parse(name);
   auto &s = entry->material.settings;
   if (s.terrainWidth > bitmap->w || s.terrainHeight > bitmap->h)
      s.terrainWidth = s.terrainHeight = 0;
   ApplyFlags(s, bitmap);
   for (const auto &directive : entry->material.unsupported)
      mprintf("Material %s%s: unsupported %s\n", entry->path.c_str(), name.c_str(),
              directive.c_str());
   RenderBackendTrace("material source=%s%s terrain=%dx%d tile=%.4fx%.4f illumination=%s",
                      entry->path.c_str(), name.c_str(), s.terrainWidth, s.terrainHeight, s.tileU,
                      s.tileV, entry->material.illumination.c_str());
   entries()[bitmap] = std::move(entry);
}
extern "C" const sTextureMaterial *ResMaterialGet(const grs_bitmap *bitmap)
{
   cAutoResThreadLock lock;
   auto found = entries().find(bitmap);
   return found == entries().end() ? nullptr : &found->second->material.settings;
}
extern "C" void ResMaterialClone(const grs_bitmap *source, grs_bitmap *destination)
{
   cAutoResThreadLock lock;
   auto found = entries().find(source);
   if (found != entries().end())
   {
      const auto entry = found->second;
      entries()[destination] = entry;
      ApplyFlags(entry->material.settings, destination);
   }
   else
      entries().erase(destination);
   const auto projection = terrain().find(source);
   if (projection != terrain().end() && !terrain().count(destination))
      terrain()[destination] = projection->second;
}
extern "C" void ResMaterialForget(const grs_bitmap *bitmap)
{
   cAutoResThreadLock lock;
   terrain().erase(bitmap);
   entries().erase(bitmap);
}
extern "C" grs_bitmap *ResMaterialIllumination(const grs_bitmap *bitmap)
{
   cAutoResThreadLock lock;
   auto found = entries().find(bitmap);
   if (found == entries().end())
      return nullptr;
   // Loading another resource can rehash this registry.
   auto entry = found->second;
   if (!entry->illuminationRead)
   {
      entry->illuminationRead = true;
      if (entry->material.illumination.empty() || entry->material.settings.illumination <= 0 ||
          entry->mesh)
         return nullptr;
      IResMan *manager = nullptr;
      entry->manager->QueryInterface(IID_IResMan, reinterpret_cast<void **>(&manager));
      if (!manager)
         return nullptr;
      const auto &name = entry->material.illumination;
      // Texture references in material files are resource-root relative.
      entry->illumination = manager->Bind(name.c_str(), RESTYPE_IMAGE, nullptr, nullptr, 0);
      manager->Release();
      if (entry->illumination)
         entry->illuminationBitmap = static_cast<grs_bitmap *>(entry->illumination->Lock());
   }
   return entry->illuminationBitmap;
}

extern "C" void ResMaterialSetTerrain(const grs_bitmap *bitmap, const grs_bitmap *base)
{
   cAutoResThreadLock lock;
   // Portal projection uses wlog, which describes row bytes, not texel width.
   terrain()[bitmap] = {std::ldexp(1.0f, base->wlog), float(base->h)};
}
extern "C" int ResMaterialIsTerrain(const grs_bitmap *bitmap)
{
   cAutoResThreadLock lock;
   return terrain().count(bitmap) != 0;
}
extern "C" void ResMaterialUvScale(const grs_bitmap *bitmap, float *u, float *v)
{
   cAutoResThreadLock lock;
   *u = *v = 1;
   auto found = entries().find(bitmap);
   if (found == entries().end())
      return;
   auto &entry = *found->second;
   const auto &s = entry.material.settings;
   // tile_factor is not defined for deformable MESH textures.
   if (!entry.mesh)
   {
      *u = s.tileU;
      *v = s.tileV;
   }
   const auto projection = terrain().find(bitmap);
   if (projection != terrain().end() && s.terrainWidth)
   {
      *u *= projection->second.width / s.terrainWidth;
      *v *= projection->second.height / s.terrainHeight;
   }
   if (!entry.uvTraced)
   {
      entry.uvTraced = true;
      RenderBackendTrace("material-uv path=%s terrain=%d scale=%.4fx%.4f", entry.path.c_str(),
                         int(terrain().count(bitmap) != 0), *u, *v);
   }
}
extern "C" void *ResMaterialRenderData(const grs_bitmap *bitmap)
{
   cAutoResThreadLock lock;
   auto found = entries().find(bitmap);
   return found == entries().end() ? nullptr : found->second->renderData;
}
extern "C" void ResMaterialSetRenderData(const grs_bitmap *bitmap, void *data,
                                         void (*release)(void *))
{
   cAutoResThreadLock lock;
   auto found = entries().find(bitmap);
   if (found == entries().end())
   {
      if (data)
         release(data);
      return;
   }
   auto &entry = *found->second;
   entry.ReleaseRenderData();
   entry.renderData = data;
   entry.releaseRenderData = release;
}
extern "C" void ResMaterialReleaseRenderData()
{
   cAutoResThreadLock lock;
   for (auto &entry : entries())
      entry.second->ReleaseRenderData();
}

extern "C" void ResMaterialReset()
{
   cAutoResThreadLock lock;
   // Unlocking dependent images can recursively call ResMaterialForget.
   // Detach the registry first so those callbacks see an empty registry.
   std::unordered_map<const grs_bitmap *, std::shared_ptr<Entry>> detached;
   detached.swap(entries());
   terrain().clear();
   detached.clear();
}
