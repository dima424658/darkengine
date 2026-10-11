// Default portable host environment for legacy 32-bit OSM modules.

#ifndef __OSM32ENV_H
#define __OSM32ENV_H

#include <osm_emu/osm32rt.h>

#pragma pack(push, 8)
#include <stdint.h>
#include <string>
#include <vector>
#pragma pack(pop)

namespace osm32
{

// Implements the platform APIs needed by the VC6 runtime embedded in the
// original OSMs. Engine-facing script and service calls remain explicit
// virtual hooks, so no host pointer or platform ABI leaks into guest memory.
class cEnvironment : public iEnvironment
{
public:
   cEnvironment();

   virtual bool Prepare(cImage *image, cMemory *memory,
                        std::string *error);
   virtual uint32_t ScriptManagerAddress() const;
   virtual uint32_t PrintAddress() const;
   virtual uint32_t AllocatorAddress() const;
   virtual uint32_t AllocGuest(uint32_t size, std::string *error);
   virtual bool FreeGuest(uint32_t address, std::string *error);

   virtual bool Handles(uint32_t address) const;
   virtual bool Invoke(uint32_t address, sCpuState *state, cMemory *memory,
                       std::string *error);

protected:
   bool GetScriptServiceId(uint32_t object, uint16_t *id) const;
   virtual bool InvokeScriptManager(unsigned slot, sCpuState *state,
                                    cMemory *memory, std::string *error);
   virtual bool InvokeScriptService(unsigned slot, sCpuState *state,
                                    cMemory *memory, std::string *error);
   virtual void DebugOutput(const std::string &text);

private:
   struct sServiceProxy
   {
      uint16_t id;
      uint32_t object;
      uint32_t vtable;
      uint32_t references;
   };

   bool InvokeAllocator(unsigned slot, sCpuState *state, cMemory *memory,
                        std::string *error);
   bool InvokeImport(const sImport &item, sCpuState *state, cMemory *memory,
                     std::string *error);

   const std::vector<sImport> *m_imports;
   cGuestHeap m_heap;
   uint32_t m_moduleHandle;
   uint32_t m_lastError;
   std::vector<bool> m_tlsUsed;
   std::vector<uint32_t> m_tlsValues;
   std::vector<sServiceProxy> m_services;
};

} // namespace osm32

#endif // __OSM32ENV_H
