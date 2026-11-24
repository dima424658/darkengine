#pragma once
#include <cstdint>
#include <memory>
#include <vector>
#include <string>
struct grs_bitmap;
struct IRes;
struct IResManHelper;
struct ResImagePixels {
   uint32_t width=0,height=0;
   std::vector<uint8_t> rgba;
};
// Retain exact decoded channels independently of the legacy 16-bit bitmap.
void ResImageRememberPixels(grs_bitmap *,uint32_t,uint32_t,const void *);
void ResImageRegisterSource(grs_bitmap *,IRes *,IResManHelper *);
void ResImageForget(grs_bitmap *);
uint64_t ResImageSourceRevision(const grs_bitmap *);
std::string ResImageSourceName(const grs_bitmap *);
std::shared_ptr<const ResImagePixels> ResImageExactPixels(const grs_bitmap *);
std::shared_ptr<const ResImagePixels> ResImageCompanion(const grs_bitmap *,const char *suffix);
// Returns true for valid authored virtual terrain dimensions; otherwise the
// caller-provided image dimensions are returned unchanged.
bool ResImageTerrainSize(const grs_bitmap *,uint32_t imageWidth,uint32_t imageHeight,uint32_t &width,uint32_t &height);
