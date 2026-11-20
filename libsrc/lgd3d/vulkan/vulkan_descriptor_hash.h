#pragma once
#include "vulkan_descriptor_key.h"
#include <cstddef>
#include <functional>

struct sDescriptorHash
{
   size_t operator()(const sDescriptorKey & key) const
   {
      size_t hash = 0;
      for (uint64_t value : key.values)
         hash ^= std::hash<uint64_t>{}(value) + size_t(0x9e3779b9) +
                 (hash << 6) + (hash >> 2);
      return hash;
   }
};
