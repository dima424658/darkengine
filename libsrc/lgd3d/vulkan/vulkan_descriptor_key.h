#pragma once
#include <array>
#include <cstdint>

struct sDescriptorKey
{
   // Stable texture identities and sampler indices avoid stale descriptors.
   std::array<uint64_t, 4> values;
   bool operator==(const sDescriptorKey & other) const
   {
      return values == other.values;
   }
};
