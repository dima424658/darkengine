// Host/guest ABI helpers shared by the embedded OSM runtime.

#include <osm_emu/osm32bridge.h>

#pragma pack(push, 8)
#include <algorithm>
#include <vector>
#pragma pack(pop)

namespace
{

bool Fail(std::string *error, const char *message)
{
   if (error)
      *error = message;
   return false;
}

uint32_t Align16(uint32_t size)
{
   if (!size)
      size = 1;
   return (size + 15u) & ~15u;
}

} // namespace

namespace osm32
{

bool ReadStackArgument(const sCpuState &state, const cMemory &memory,
                       unsigned index, uint32_t *value, std::string *error)
{
   const uint64_t address = static_cast<uint64_t>(state.reg[kEsp]) + 4ull +
                            static_cast<uint64_t>(index) * 4ull;
   if (address > 0xffffffffull)
      return Fail(error, "guest stack argument address overflow");
   return memory.Read32(static_cast<uint32_t>(address), value, error);
}

bool ReturnStdcall(sCpuState *state, cMemory *memory, unsigned argumentBytes,
                   uint32_t result, std::string *error)
{
   uint32_t returnAddress;
   if (!memory->Read32(state->reg[kEsp], &returnAddress, error))
      return false;
   const uint64_t newStack = static_cast<uint64_t>(state->reg[kEsp]) + 4ull +
                             argumentBytes;
   if (newStack > 0xffffffffull)
      return Fail(error, "guest stdcall stack overflow");
   state->reg[kEax] = result;
   state->reg[kEsp] = static_cast<uint32_t>(newStack);
   state->eip = returnAddress;
   return true;
}

cGuestHeap::cGuestHeap()
 : m_memory(0),
   m_base(0),
   m_end(0),
   m_next(0)
{
}

bool cGuestHeap::Initialize(cMemory *memory, uint32_t base, uint32_t size,
                            std::string *error)
{
   if (!memory || !size)
      return Fail(error, "invalid guest heap configuration");
   const uint64_t end = static_cast<uint64_t>(base) + size;
   if (end > 0x100000000ull)
      return Fail(error, "guest heap wraps the address space");
   if (!memory->Map(base, size, kMemRead | kMemWrite, error))
      return false;
   m_memory = memory;
   m_base = base;
   m_end = end;
   m_next = base;
   m_blocks.clear();
   return true;
}

cGuestHeap::sBlock *cGuestHeap::Find(uint32_t address)
{
   for (size_t i = 0; i < m_blocks.size(); ++i)
      if (m_blocks[i].address == address)
         return &m_blocks[i];
   return 0;
}

const cGuestHeap::sBlock *cGuestHeap::Find(uint32_t address) const
{
   for (size_t i = 0; i < m_blocks.size(); ++i)
      if (m_blocks[i].address == address)
         return &m_blocks[i];
   return 0;
}

uint32_t cGuestHeap::Alloc(uint32_t size, std::string *error)
{
   if (!m_memory)
   {
      Fail(error, "guest heap is not initialized");
      return 0;
   }
   if (size > 0xfffffff0u)
   {
      Fail(error, "guest allocation is too large");
      return 0;
   }
   const uint32_t capacity = Align16(size);
   for (size_t i = 0; i < m_blocks.size(); ++i)
   {
      sBlock &block = m_blocks[i];
      if (block.free && block.capacity >= capacity)
      {
         block.free = false;
         block.size = size;
         if (error) error->clear();
         return block.address;
      }
   }

   if (static_cast<uint64_t>(m_next) + capacity > m_end)
   {
      Fail(error, "guest heap exhausted");
      return 0;
   }
   sBlock block;
   block.address = m_next;
   block.size = size;
   block.capacity = capacity;
   block.free = false;
   m_blocks.push_back(block);
   m_next += capacity;
   if (error) error->clear();
   return block.address;
}

uint32_t cGuestHeap::Realloc(uint32_t address, uint32_t size,
                             std::string *error)
{
   if (!address)
      return Alloc(size, error);
   if (!size)
   {
      Free(address, error);
      return 0;
   }

   sBlock *old = Find(address);
   if (!old || old->free)
   {
      Fail(error, "guest realloc received an invalid pointer");
      return 0;
   }
   if (size <= old->capacity)
   {
      old->size = size;
      if (error) error->clear();
      return address;
   }

   const uint32_t oldSize = old->size;
   const uint32_t replacement = Alloc(size, error);
   if (!replacement)
      return 0;
   std::vector<uint8_t> copy(oldSize);
   for (uint32_t i = 0; i < oldSize; ++i)
      if (!m_memory->Read8(address + i, &copy[i], error))
         return 0;
   if (!copy.empty() && !m_memory->Load(replacement, &copy[0], copy.size(), error))
      return 0;
   old = Find(address);
   old->free = true;
   if (error) error->clear();
   return replacement;
}

bool cGuestHeap::Free(uint32_t address, std::string *error)
{
   if (!address)
      return true;
   sBlock *block = Find(address);
   if (!block || block->free)
      return Fail(error, "guest free received an invalid pointer");
   block->free = true;
   if (error) error->clear();
   return true;
}

uint32_t cGuestHeap::GetSize(uint32_t address) const
{
   const sBlock *block = Find(address);
   return block && !block->free ? block->size : 0xffffffffu;
}

int cGuestHeap::DidAlloc(uint32_t address) const
{
   const sBlock *block = Find(address);
   return block && !block->free ? 1 : 0;
}

} // namespace osm32
