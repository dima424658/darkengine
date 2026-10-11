// Default portable host environment for legacy 32-bit OSM modules.

#include <osm_emu/osm32env.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace
{

const uint32_t kImportTrap = 0xf0000000;
const uint32_t kScriptManTrap = 0xf1000000;
const uint32_t kAllocatorTrap = 0xf1001000;
const uint32_t kServiceTrap = 0xf1002000;
const uint32_t kPrintTrap = 0xf2000000;
const uint32_t kTrapStride = 16;
const uint32_t kProxyBase = 0x60000000;
const uint32_t kScriptMan = kProxyBase + 0x100;
const uint32_t kScriptManVtable = kProxyBase + 0x200;
const uint32_t kAllocator = kProxyBase + 0x300;
const uint32_t kAllocatorVtable = kProxyBase + 0x400;
const uint32_t kCommandLine = kProxyBase + 0x800;
const uint32_t kEnvironment = kProxyBase + 0x840;
const uint32_t kHeapBase = 0x40000000;
const uint32_t kHeapSize = 0x02000000;

void Write32(uint8_t *target, uint32_t value)
{
   target[0] = static_cast<uint8_t>(value);
   target[1] = static_cast<uint8_t>(value >> 8);
   target[2] = static_cast<uint8_t>(value >> 16);
   target[3] = static_cast<uint8_t>(value >> 24);
}

bool Fail(std::string *error, const std::string &message)
{
   if (error)
      *error = message;
   return false;
}

bool IsKernelImport(const osm32::sImport &item, const char *name)
{
   return !item.byOrdinal && item.module == "KERNEL32.dll" &&
          item.name == name;
}

} // namespace

namespace osm32
{

cEnvironment::cEnvironment()
 : m_imports(0),
   m_moduleHandle(0),
   m_lastError(0)
{
}

uint32_t cEnvironment::ScriptManagerAddress() const { return kScriptMan; }
uint32_t cEnvironment::PrintAddress() const { return kPrintTrap; }
uint32_t cEnvironment::AllocatorAddress() const { return kAllocator; }

uint32_t cEnvironment::AllocGuest(uint32_t size, std::string *error)
{
   return m_heap.Alloc(size, error);
}

bool cEnvironment::FreeGuest(uint32_t address, std::string *error)
{
   return m_heap.Free(address, error);
}

bool cEnvironment::Prepare(cImage *image, cMemory *memory,
                           std::string *error)
{
   m_imports = &image->Imports();
   m_moduleHandle = image->LoadedBase();
   for (size_t i = 0; i < m_imports->size(); ++i)
      Write32(image->Data() + (*m_imports)[i].iatRva,
              kImportTrap + static_cast<uint32_t>(i) * kTrapStride);

   uint8_t proxies[0x900] = {};
   const char commandLine[] = "darkengine";
   memcpy(proxies + (kCommandLine - kProxyBase), commandLine,
          sizeof(commandLine));
   Write32(proxies + (kScriptMan - kProxyBase), kScriptManVtable);
   Write32(proxies + (kAllocator - kProxyBase), kAllocatorVtable);
   for (unsigned i = 0; i < 64; ++i)
      Write32(proxies + (kScriptManVtable - kProxyBase) + i * 4,
              kScriptManTrap + i * kTrapStride);
   for (unsigned i = 0; i < 16; ++i)
      Write32(proxies + (kAllocatorVtable - kProxyBase) + i * 4,
              kAllocatorTrap + i * kTrapStride);
   m_services.clear();

   return memory->Map(kProxyBase, sizeof(proxies), kMemRead | kMemWrite,
                      error) &&
          memory->Load(kProxyBase, proxies, sizeof(proxies), error) &&
          m_heap.Initialize(memory, kHeapBase, kHeapSize, error);
}

bool cEnvironment::Handles(uint32_t address) const
{
   if (address == kPrintTrap)
      return true;
   if (address >= kScriptManTrap && address < kScriptManTrap + 0x3000)
      return (address - kScriptManTrap) % kTrapStride == 0;
   if (!m_imports || address < kImportTrap)
      return false;
   const uint32_t offset = address - kImportTrap;
   return offset % kTrapStride == 0 &&
          offset / kTrapStride < m_imports->size();
}

bool cEnvironment::InvokeAllocator(unsigned slot, sCpuState *state,
                                   cMemory *memory, std::string *error)
{
   uint32_t object, value, value2;
   if (!ReadStackArgument(*state, *memory, 0, &object, error) ||
       object != kAllocator)
      return Fail(error, "invalid guest allocator object");
   switch (slot)
   {
      case 0:
      {
         uint32_t iid, output, iidData1;
         if (!ReadStackArgument(*state, *memory, 1, &iid, error) ||
             !ReadStackArgument(*state, *memory, 2, &output, error) ||
             !memory->Read32(iid, &iidData1, error))
            return false;
         const bool supported = iidData1 == 2;
         if (!memory->Write32(output, supported ? kAllocator : 0, error))
            return false;
         return ReturnStdcall(state, memory, 12,
                              supported ? 0 : 0x80004002u, error);
      }
      case 1: case 2:
         return ReturnStdcall(state, memory, 4, 1, error);
      case 3:
         if (!ReadStackArgument(*state, *memory, 1, &value, error)) return false;
         value2 = m_heap.Alloc(value, error);
         return (!value2 && value) ? false :
                ReturnStdcall(state, memory, 8, value2, error);
      case 4:
         if (!ReadStackArgument(*state, *memory, 1, &value, error) ||
             !ReadStackArgument(*state, *memory, 2, &value2, error)) return false;
         value = m_heap.Realloc(value, value2, error);
         return (!value && value2) ? false :
                ReturnStdcall(state, memory, 12, value, error);
      case 5:
         if (!ReadStackArgument(*state, *memory, 1, &value, error) ||
             !m_heap.Free(value, error)) return false;
         return ReturnStdcall(state, memory, 8, 0, error);
      case 6:
         if (!ReadStackArgument(*state, *memory, 1, &value, error)) return false;
         return ReturnStdcall(state, memory, 8, m_heap.GetSize(value), error);
      case 7:
         if (!ReadStackArgument(*state, *memory, 1, &value, error)) return false;
         return ReturnStdcall(state, memory, 8,
                              static_cast<uint32_t>(m_heap.DidAlloc(value)),
                              error);
      case 8:
         return ReturnStdcall(state, memory, 4, 0, error);
   }
   return Fail(error, "unsupported guest allocator method");
}

bool cEnvironment::InvokeScriptManager(unsigned slot, sCpuState *state,
                                       cMemory *memory, std::string *error)
{
   uint32_t object;
   if (!ReadStackArgument(*state, *memory, 0, &object, error) ||
       object != kScriptMan)
      return Fail(error, "invalid guest script manager object");
   if (slot == 12) // IScriptMan::GetService
   {
      uint32_t guidAddress;
      uint16_t serviceId;
      if (!ReadStackArgument(*state, *memory, 1, &guidAddress, error) ||
          !memory->Read16(guidAddress, &serviceId, error))
         return false;

      for (size_t i = 0; i < m_services.size(); ++i)
         if (m_services[i].id == serviceId)
         {
            ++m_services[i].references;
            return ReturnStdcall(state, memory, 8, m_services[i].object,
                                 error);
         }

      const unsigned kServiceSlots = 64;
      sServiceProxy proxy;
      proxy.id = serviceId;
      proxy.object = m_heap.Alloc(4, error);
      proxy.vtable = m_heap.Alloc(kServiceSlots * 4, error);
      proxy.references = 1;
      if (!proxy.object || !proxy.vtable ||
          !memory->Write32(proxy.object, proxy.vtable, error))
         return false;
      for (unsigned i = 0; i < kServiceSlots; ++i)
         if (!memory->Write32(proxy.vtable + i * 4,
                              kServiceTrap + i * kTrapStride, error))
            return false;
      m_services.push_back(proxy);
      return ReturnStdcall(state, memory, 8, proxy.object, error);
   }
   char message[80];
   snprintf(message, sizeof(message),
            "unsupported guest IScriptMan slot %u", slot);
   return Fail(error, message);
}

bool cEnvironment::GetScriptServiceId(uint32_t object, uint16_t *id) const
{
   for (size_t i = 0; i < m_services.size(); ++i)
      if (m_services[i].object == object)
      {
         if (id)
            *id = m_services[i].id;
         return true;
      }
   return false;
}

bool cEnvironment::InvokeScriptService(unsigned slot, sCpuState *state,
                                       cMemory *memory, std::string *error)
{
   uint32_t object;
   if (!ReadStackArgument(*state, *memory, 0, &object, error))
      return false;
   sServiceProxy *proxy = 0;
   for (size_t i = 0; i < m_services.size(); ++i)
      if (m_services[i].object == object)
      {
         proxy = &m_services[i];
         break;
      }
   if (!proxy)
      return Fail(error, "invalid guest script service object");
   if (slot == 0)
   {
      uint32_t output;
      if (!ReadStackArgument(*state, *memory, 2, &output, error) ||
          !memory->Write32(output, proxy->object, error))
         return false;
      ++proxy->references;
      return ReturnStdcall(state, memory, 12, 0, error);
   }
   if (slot == 1)
      return ReturnStdcall(state, memory, 4, ++proxy->references, error);
   if (slot == 2)
   {
      if (proxy->references)
         --proxy->references;
      return ReturnStdcall(state, memory, 4, proxy->references, error);
   }
   char message[96];
   snprintf(message, sizeof(message),
            "unsupported guest script service 0x%04x slot %u",
            static_cast<unsigned>(proxy->id), slot);
   return Fail(error, message);
}

void cEnvironment::DebugOutput(const std::string &text)
{
   const char *enabled = getenv("DARK_SCRIPT_DEBUG");
   if (enabled && *enabled)
      fputs(text.c_str(), stderr);
}

bool cEnvironment::Invoke(uint32_t address, sCpuState *state, cMemory *memory,
                          std::string *error)
{
   if (address == kPrintTrap)
      return Fail(error, "guest variadic print call requires marshalling");
   if (address >= kScriptManTrap && address < kAllocatorTrap)
      return InvokeScriptManager((address - kScriptManTrap) / kTrapStride,
                                 state, memory, error);
   if (address >= kAllocatorTrap && address < kServiceTrap)
      return InvokeAllocator((address - kAllocatorTrap) / kTrapStride,
                             state, memory, error);
   if (address >= kServiceTrap && address < kServiceTrap + 0x1000)
      return InvokeScriptService((address - kServiceTrap) / kTrapStride,
                                 state, memory, error);
   if (!m_imports || address < kImportTrap)
      return Fail(error, "invalid guest host trap");
   return InvokeImport((*m_imports)[(address - kImportTrap) / kTrapStride],
                       state, memory, error);
}

bool cEnvironment::InvokeImport(const sImport &item, sCpuState *state,
                                cMemory *memory, std::string *error)
{
   if (IsKernelImport(item, "GetVersion"))
      return ReturnStdcall(state, memory, 0, 5, error);
   if (IsKernelImport(item, "GetModuleHandleA"))
   {
      uint32_t name;
      if (!ReadStackArgument(*state, *memory, 0, &name, error)) return false;
      return ReturnStdcall(state, memory, 4, name ? 0 : m_moduleHandle, error);
   }
   if (IsKernelImport(item, "InitializeCriticalSection") ||
       IsKernelImport(item, "DeleteCriticalSection") ||
       IsKernelImport(item, "EnterCriticalSection") ||
       IsKernelImport(item, "LeaveCriticalSection"))
      return ReturnStdcall(state, memory, 4, 0, error);
   if (IsKernelImport(item, "TlsAlloc"))
   {
      for (size_t i = 0; i < m_tlsUsed.size(); ++i)
         if (!m_tlsUsed[i])
         {
            m_tlsUsed[i] = true;
            m_tlsValues[i] = 0;
            return ReturnStdcall(state, memory, 0,
                                 static_cast<uint32_t>(i), error);
         }
      m_tlsUsed.push_back(true);
      m_tlsValues.push_back(0);
      return ReturnStdcall(state, memory, 0,
         static_cast<uint32_t>(m_tlsUsed.size() - 1), error);
   }
   if (IsKernelImport(item, "TlsSetValue") ||
       IsKernelImport(item, "TlsGetValue") ||
       IsKernelImport(item, "TlsFree"))
   {
      uint32_t index, value = 0;
      if (!ReadStackArgument(*state, *memory, 0, &index, error)) return false;
      const bool valid = index < m_tlsUsed.size() && m_tlsUsed[index];
      if (item.name == "TlsSetValue")
      {
         if (!ReadStackArgument(*state, *memory, 1, &value, error)) return false;
         if (valid) m_tlsValues[index] = value;
         return ReturnStdcall(state, memory, 8, valid ? 1 : 0, error);
      }
      if (item.name == "TlsGetValue")
         return ReturnStdcall(state, memory, 4,
                              valid ? m_tlsValues[index] : 0, error);
      if (valid)
      {
         m_tlsUsed[index] = false;
         m_tlsValues[index] = 0;
      }
      return ReturnStdcall(state, memory, 4, valid ? 1 : 0, error);
   }
   if (IsKernelImport(item, "GetCurrentThreadId"))
      return ReturnStdcall(state, memory, 0, 1, error);
   if (IsKernelImport(item, "GetCurrentThread"))
      return ReturnStdcall(state, memory, 0, 0xfffffffeu, error);
   if (IsKernelImport(item, "GetThreadPriority"))
      return ReturnStdcall(state, memory, 4, 0, error);
   if (IsKernelImport(item, "SetThreadPriority"))
      return ReturnStdcall(state, memory, 8, 1, error);
   if (IsKernelImport(item, "GetTickCount"))
      return ReturnStdcall(state, memory, 0, 0, error);
   if (IsKernelImport(item, "GetCommandLineA"))
      return ReturnStdcall(state, memory, 0, kCommandLine, error);
   if (IsKernelImport(item, "GetEnvironmentStrings") ||
       IsKernelImport(item, "GetEnvironmentStringsA") ||
       IsKernelImport(item, "GetEnvironmentStringsW"))
      return ReturnStdcall(state, memory, 0, kEnvironment, error);
   if (IsKernelImport(item, "FreeEnvironmentStringsA") ||
       IsKernelImport(item, "FreeEnvironmentStringsW"))
      return ReturnStdcall(state, memory, 4, 1, error);

   if (IsKernelImport(item, "WideCharToMultiByte"))
   {
      uint32_t source, sourceCount, destination, destinationCount, used;
      if (!ReadStackArgument(*state, *memory, 2, &source, error) ||
          !ReadStackArgument(*state, *memory, 3, &sourceCount, error) ||
          !ReadStackArgument(*state, *memory, 4, &destination, error) ||
          !ReadStackArgument(*state, *memory, 5, &destinationCount, error) ||
          !ReadStackArgument(*state, *memory, 7, &used, error)) return false;
      uint32_t count = sourceCount;
      if (count == 0xffffffffu)
      {
         count = 0;
         uint16_t value;
         do
         {
            if (count == 0x00100000u ||
                !memory->Read16(source + count * 2, &value, error)) return false;
            ++count;
         } while (value);
      }
      if (destination && destinationCount >= count)
      {
         for (uint32_t i = 0; i < count; ++i)
         {
            uint16_t value;
            if (!memory->Read16(source + i * 2, &value, error) ||
                !memory->Write8(destination + i,
                   value <= 0x7f ? static_cast<uint8_t>(value) : '?', error))
               return false;
         }
         if (used && !memory->Write32(used, 0, error)) return false;
      }
      return ReturnStdcall(state, memory, 32,
         !destination || destinationCount >= count ? count : 0, error);
   }
   if (IsKernelImport(item, "MultiByteToWideChar"))
   {
      uint32_t source, sourceCount, destination, destinationCount;
      if (!ReadStackArgument(*state, *memory, 2, &source, error) ||
          !ReadStackArgument(*state, *memory, 3, &sourceCount, error) ||
          !ReadStackArgument(*state, *memory, 4, &destination, error) ||
          !ReadStackArgument(*state, *memory, 5, &destinationCount, error))
         return false;
      uint32_t count = sourceCount;
      if (count == 0xffffffffu)
      {
         count = 0;
         uint8_t value;
         do
         {
            if (count == 0x00100000u ||
                !memory->Read8(source + count, &value, error)) return false;
            ++count;
         } while (value);
      }
      if (destination && destinationCount >= count)
         for (uint32_t i = 0; i < count; ++i)
         {
            uint8_t value;
            if (!memory->Read8(source + i, &value, error) ||
                !memory->Write16(destination + i * 2, value, error)) return false;
         }
      return ReturnStdcall(state, memory, 24,
         !destination || destinationCount >= count ? count : 0, error);
   }
   if (IsKernelImport(item, "GetStartupInfoA"))
   {
      uint32_t output;
      if (!ReadStackArgument(*state, *memory, 0, &output, error)) return false;
      for (uint32_t i = 0; i < 68; ++i)
         if (!memory->Write8(output + i, 0, error)) return false;
      if (!memory->Write32(output, 68, error)) return false;
      return ReturnStdcall(state, memory, 4, 0, error);
   }
   if (IsKernelImport(item, "GetStdHandle"))
   {
      uint32_t which;
      if (!ReadStackArgument(*state, *memory, 0, &which, error)) return false;
      return ReturnStdcall(state, memory, 4, 0x100u + (0u - which), error);
   }
   if (IsKernelImport(item, "GetFileType"))
      return ReturnStdcall(state, memory, 4, 2, error);
   if (IsKernelImport(item, "SetHandleCount"))
   {
      uint32_t count;
      if (!ReadStackArgument(*state, *memory, 0, &count, error)) return false;
      return ReturnStdcall(state, memory, 4, count, error);
   }
   if (IsKernelImport(item, "SetStdHandle"))
      return ReturnStdcall(state, memory, 8, 1, error);
   if (IsKernelImport(item, "GetACP"))
      return ReturnStdcall(state, memory, 0, 1252, error);
   if (IsKernelImport(item, "GetOEMCP"))
      return ReturnStdcall(state, memory, 0, 437, error);
   if (IsKernelImport(item, "GetCPInfo"))
   {
      uint32_t output;
      if (!ReadStackArgument(*state, *memory, 1, &output, error)) return false;
      for (uint32_t i = 0; i < 20; ++i)
         if (!memory->Write8(output + i, 0, error)) return false;
      if (!memory->Write32(output, 1, error) ||
          !memory->Write8(output + 4, '?', error)) return false;
      return ReturnStdcall(state, memory, 8, 1, error);
   }
   if (IsKernelImport(item, "GetModuleFileNameA"))
   {
      static const char path[] = "guest.osm";
      uint32_t output, capacity;
      if (!ReadStackArgument(*state, *memory, 1, &output, error) ||
          !ReadStackArgument(*state, *memory, 2, &capacity, error)) return false;
      const uint32_t length = static_cast<uint32_t>(sizeof(path) - 1);
      const uint32_t written = capacity <= length ? capacity : length;
      for (uint32_t i = 0; i < written; ++i)
         if (!memory->Write8(output + i, path[i], error)) return false;
      if (capacity > length && !memory->Write8(output + length, 0, error))
         return false;
      return ReturnStdcall(state, memory, 12,
                           capacity <= length ? capacity : length, error);
   }
   if (IsKernelImport(item, "GetFileAttributesA"))
   {
      m_lastError = 2;
      return ReturnStdcall(state, memory, 4, 0xffffffffu, error);
   }
   if (IsKernelImport(item, "GetLastError"))
      return ReturnStdcall(state, memory, 0, m_lastError, error);
   if (IsKernelImport(item, "SetLastError"))
   {
      if (!ReadStackArgument(*state, *memory, 0, &m_lastError, error))
         return false;
      return ReturnStdcall(state, memory, 4, 0, error);
   }
   if (IsKernelImport(item, "GetPrivateProfileIntA"))
   {
      uint32_t fallback;
      if (!ReadStackArgument(*state, *memory, 2, &fallback, error)) return false;
      return ReturnStdcall(state, memory, 16, fallback, error);
   }
   if (IsKernelImport(item, "GetPrivateProfileStringA"))
   {
      uint32_t fallback, output, capacity;
      if (!ReadStackArgument(*state, *memory, 2, &fallback, error) ||
          !ReadStackArgument(*state, *memory, 3, &output, error) ||
          !ReadStackArgument(*state, *memory, 4, &capacity, error)) return false;
      uint32_t copied = 0;
      if (capacity)
      {
         while (copied + 1 < capacity)
         {
            uint8_t value = 0;
            if (fallback && !memory->Read8(fallback + copied, &value, error))
               return false;
            if (!memory->Write8(output + copied, value, error)) return false;
            if (!value) break;
            ++copied;
         }
         if (!memory->Write8(output + copied, 0, error)) return false;
      }
      return ReturnStdcall(state, memory, 24, copied, error);
   }
   if (IsKernelImport(item, "OutputDebugStringA"))
   {
      uint32_t textAddress;
      if (!ReadStackArgument(*state, *memory, 0, &textAddress, error))
         return false;
      std::string text;
      for (unsigned i = 0; i < 4096; ++i)
      {
         uint8_t ch;
         if (!memory->Read8(textAddress + i, &ch, error)) return false;
         if (!ch) break;
         text.push_back(static_cast<char>(ch));
      }
      DebugOutput(text);
      return ReturnStdcall(state, memory, 4, 0, error);
   }

   std::string name = item.module + "!";
   if (item.byOrdinal)
   {
      char ordinal[24];
      snprintf(ordinal, sizeof(ordinal), "#%u",
               static_cast<unsigned>(item.ordinal));
      name += ordinal;
   }
   else
      name += item.name;
   return Fail(error, "unsupported guest import " + name);
}

} // namespace osm32
