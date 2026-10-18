// Host/guest ABI helpers shared by the embedded OSM runtime.

#ifndef __OSM32BRIDGE_H
#define __OSM32BRIDGE_H

#include <osm_emu/osm32cpu.h>

#pragma pack(push, 8)
#include <stdint.h>
#include <string>
#include <vector>
#pragma pack(pop)

namespace osm32
{

bool ReadStackArgument(const sCpuState &state, const cMemory &memory,
                       unsigned index, uint32_t *value, std::string *error);
bool ReturnStdcall(sCpuState *state, cMemory *memory, unsigned argumentBytes,
                   uint32_t result, std::string *error);

class cGuestHeap
{
public:
   cGuestHeap();

   bool Initialize(cMemory *memory, uint32_t base, uint32_t size,
                   std::string *error);
   uint32_t Alloc(uint32_t size, std::string *error);
   uint32_t Realloc(uint32_t address, uint32_t size, std::string *error);
   bool Free(uint32_t address, std::string *error);
   uint32_t GetSize(uint32_t address) const;
   int DidAlloc(uint32_t address) const;

private:
   struct sBlock
   {
      uint32_t address;
      uint32_t size;
      uint32_t capacity;
      bool free;
   };

   sBlock *Find(uint32_t address);
   const sBlock *Find(uint32_t address) const;

   cMemory *m_memory;
   uint32_t m_base;
   uint64_t m_end;
   uint32_t m_next;
   std::vector<sBlock> m_blocks;
};

} // namespace osm32

#endif // __OSM32BRIDGE_H
