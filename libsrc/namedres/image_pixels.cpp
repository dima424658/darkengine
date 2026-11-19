#include "image_pixels.hpp"
#include "image_pixels_c.h"
#include <resapi.h>
#include <storeapi.h>
#include <reshelp.h>
#include <imgrstyp.h>
#include <resthred.h>
#include <cstring>
#include <string>
#include <unordered_map>
#include <cstdlib>
#include <memall.h>
namespace {
struct Source {
   uint64_t revision=0;
   IResManHelper *helper=nullptr;
   IStore *store=nullptr;
   std::string name,path,fullName;
   std::shared_ptr<ResImagePixels> pixels;
   ~Source(){if(store)store->Release();}
};
auto &sources(){static auto *map=new std::unordered_map<const grs_bitmap *,std::unique_ptr<Source>>;return *map;}
uint64_t nextRevision=0;
Source &source(const grs_bitmap *bitmap) {
   auto &entry=sources()[bitmap];
   if(!entry){entry=std::make_unique<Source>();entry->revision=++nextRevision;}
   return *entry;
}
}
void ResImageRememberPixels(grs_bitmap *bitmap,uint32_t width,uint32_t height,const void *rgba) {
   cAutoResThreadLock lock;
   if(!bitmap || !rgba || !width || !height || uint64_t(width)*height>SIZE_MAX/4)return;
   auto pixels=std::make_shared<ResImagePixels>();pixels->width=width;pixels->height=height;
   const auto *bytes=static_cast<const uint8_t *>(rgba);
   pixels->rgba.assign(bytes,bytes+size_t(width)*height*4);source(bitmap).pixels=std::move(pixels);
}
void ResImageRegisterSource(grs_bitmap *bitmap,IRes *resource,IResManHelper *manager) {
   cAutoResThreadLock lock;
   if(!bitmap || !resource)return;
   auto &entry=source(bitmap);entry.helper=manager;entry.name=resource->GetName();
   const auto dot=entry.name.find_last_of('.');if(dot!=std::string::npos)entry.name.resize(dot);
   char *path=nullptr;resource->GetCanonPath(&path);if(path){entry.path=path;Free(path);}
   entry.fullName=entry.path+entry.name;
   if(entry.store)entry.store->Release();entry.store=resource->GetStore();
}
void ResImageForget(grs_bitmap *bitmap){cAutoResThreadLock lock;sources().erase(bitmap);}
extern "C" void ResImageForgetSource(void *bitmap){ResImageForget(static_cast<grs_bitmap *>(bitmap));}
extern "C" void ResImageCloneSource(const void *original,const void *replacement) {
   cAutoResThreadLock lock;
   const auto found=sources().find(static_cast<const grs_bitmap *>(original));
   if(found==sources().end() || !replacement || original==replacement)return;
   const auto &old=*found->second;auto next=std::make_unique<Source>();
   next->revision=old.revision;next->helper=old.helper;next->name=old.name;next->path=old.path;next->fullName=old.fullName;next->pixels=old.pixels;
   next->store=old.store;if(next->store)next->store->AddRef();
   sources()[static_cast<const grs_bitmap *>(replacement)]=std::move(next);
}
uint64_t ResImageSourceRevision(const grs_bitmap *bitmap) {
   cAutoResThreadLock lock;
   const auto found=sources().find(bitmap);return found==sources().end()?0:found->second->revision;
}
std::string ResImageSourceName(const grs_bitmap *bitmap) {
   cAutoResThreadLock lock;
   const auto found=sources().find(bitmap);return found==sources().end()?"":found->second->fullName.c_str();
}
std::shared_ptr<const ResImagePixels> ResImageExactPixels(const grs_bitmap *bitmap) {
   cAutoResThreadLock lock;
   const auto found=sources().find(bitmap);return found==sources().end()?nullptr:found->second->pixels;
}
std::shared_ptr<const ResImagePixels> ResImageCompanion(const grs_bitmap *bitmap,const char *suffix) {
   cAutoResThreadLock lock;
   const auto found=sources().find(bitmap);if(found==sources().end() || !suffix)return {};
   // Snapshot provenance before nested loads, which can evict other resources.
   const auto &entry=*found->second;if(!entry.helper)return {};
   const std::string baseName=entry.name,path=entry.path;
   IStore *store=entry.store;if(store)store->AddRef();
   const auto releaseStore=[](IStore *p){if(p)p->Release();};
   std::unique_ptr<IStore,decltype(releaseStore)> heldStore(store,releaseStore);
   IResMan *manager=nullptr;
   entry.helper->QueryInterface(IID_IResMan,reinterpret_cast<void **>(&manager));
   if(!manager)return {};
   const auto release=[](IResMan *p){p->Release();};
   std::unique_ptr<IResMan,decltype(release)> owner(manager,release);
   for(const char *extension:{".dds",".png"}) {
      const std::string name=baseName+suffix+extension;
      IRes *resource=manager->Bind(name.c_str(),RESTYPE_IMAGE,nullptr,path.c_str());
      if(!resource && store)resource=manager->BindSpecific(name.c_str(),RESTYPE_IMAGE,store,nullptr,0);
      if(!resource)continue;
      auto *data=static_cast<grs_bitmap *>(resource->Lock());
      auto pixels=data?ResImageExactPixels(data):nullptr;
      if(data)resource->Unlock();resource->Release();
      if(pixels)return pixels;
   }
   return {};
}
