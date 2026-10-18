// Default portable host environment for legacy 32-bit OSM modules.

#include <osm_emu/osm32env.h>
#include <osm_emu/osm32imports.h>

#pragma pack(push, 8)
#include <chrono>
#include <cmath>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#pragma pack(pop)

namespace
{

const uint32_t kImportTrap = 0xf0000000;
const uint32_t kScriptManTrap = 0xf1000000;
const uint32_t kAllocatorTrap = 0xf1001000;
const uint32_t kServiceTrap = 0xf1002000;
const uint32_t kPrintTrap = 0xf2000000;
const uint32_t kInitializerTrap = 0xf2000010;
const uint32_t kTrapStride = 16;
const uint32_t kProxyBase = 0x60000000;
const uint32_t kScriptMan = kProxyBase + 0x100;
const uint32_t kScriptManVtable = kProxyBase + 0x200;
const uint32_t kAllocator = kProxyBase + 0x300;
const uint32_t kAllocatorVtable = kProxyBase + 0x400;
const uint32_t kCommandLine = kProxyBase + 0x800;
const uint32_t kEnvironment = kProxyBase + 0x840;
const uint32_t kImportDataBase = kProxyBase + 0x900;
const uint32_t kThreadEnvironmentBlock = kProxyBase + 0xa00;
const uint32_t kHeapBase = 0x40000000;
const uint32_t kHeapSize = 0x02000000;
const uint32_t kGuestHeapHandle = kProxyBase + 0xb00;

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

bool ReadCString(const osm32::cMemory &memory, uint32_t address,
                 std::string *value, std::string *error)
{
   value->clear();
   if (!address)
      return Fail(error, "null guest string");
   for (uint32_t i = 0; i < 0x00100000; ++i)
   {
      uint8_t character;
      if (!memory.Read8(address + i, &character, error))
         return false;
      if (!character)
         return true;
      value->push_back(static_cast<char>(character));
   }
   return Fail(error, "unterminated guest string");
}

bool ReadStackDouble(const osm32::sCpuState &state,
                     const osm32::cMemory &memory, unsigned wordOffset,
                     double *value, std::string *error)
{
   uint64_t bits;
   if (!memory.Read64(state.reg[osm32::kEsp] + 4 + wordOffset * 4,
                      &bits, error))
      return false;
   memcpy(value, &bits, sizeof(*value));
   return true;
}

bool ReturnCrtDouble(osm32::sCpuState *state, osm32::cMemory *memory,
                     double value, std::string *error)
{
   if (state->x87Depth == 8)
      return Fail(error, "guest CRT x87 stack overflow");
   for (unsigned i = state->x87Depth; i != 0; --i)
      state->x87[i] = state->x87[i - 1];
   state->x87[0] = value;
   ++state->x87Depth;
   return osm32::ReturnStdcall(state, memory, 0, state->reg[osm32::kEax],
                              error);
}

uint8_t AsciiLower(uint8_t character)
{
   return character >= 'A' && character <= 'Z' ?
      static_cast<uint8_t>(character + ('a' - 'A')) : character;
}

bool IsKernelImport(const osm32::sImport &item, const char *name)
{
   return !item.byOrdinal && item.module == "KERNEL32.dll" &&
          item.name == name;
}

bool IsCrtImport(const osm32::sImport &item, const char *name)
{
   if (item.byOrdinal || item.name != name)
      return false;
   std::string module = item.module;
   for (size_t i = 0; i < module.size(); ++i)
      if (module[i] >= 'A' && module[i] <= 'Z')
         module[i] += 'a' - 'A';
   return module == "msvcr90.dll" || module == "msvcrt.dll";
}

bool IsCrtDataImport(const osm32::sImport &item)
{
   return !item.byOrdinal &&
          (item.name == "_encoded_null" || item.name == "_adjust_fdiv");
}

bool EqualAsciiNoCase(const std::string &left, const std::string &right)
{
   if (left.size() != right.size()) return false;
   for (size_t i = 0; i < left.size(); ++i)
      if (AsciiLower(static_cast<uint8_t>(left[i])) !=
          AsciiLower(static_cast<uint8_t>(right[i])))
         return false;
   return true;
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
   {
      if (getenv("DARK_OSM32_TRACE") &&
          !IsSupportedImport((*m_imports)[i]))
         fprintf(stderr, "OSM32 preflight unsupported import %s!%s\n",
                 (*m_imports)[i].module.c_str(),
                 (*m_imports)[i].byOrdinal ? "<ordinal>" :
                                             (*m_imports)[i].name.c_str());
      Write32(image->Data() + (*m_imports)[i].iatRva,
              IsCrtDataImport((*m_imports)[i]) ?
                 kImportDataBase + static_cast<uint32_t>(i) * 4 :
                 kImportTrap + static_cast<uint32_t>(i) * kTrapStride);
   }

   uint8_t proxies[0x1000] = {};
   const char commandLine[] = "darkengine";
   memcpy(proxies + (kCommandLine - kProxyBase), commandLine,
          sizeof(commandLine));
   // The VC9 DLL startup lock reads fs:[0x18] for the TEB and then uses the
   // stack-base field at TEB+4 as a stable per-thread identity.
   Write32(proxies + (kThreadEnvironmentBlock - kProxyBase) + 0x18,
           kThreadEnvironmentBlock);
   Write32(proxies + (kThreadEnvironmentBlock - kProxyBase) + 4,
           0x70100000);
   Write32(proxies + (kScriptMan - kProxyBase), kScriptManVtable);
   Write32(proxies + (kAllocator - kProxyBase), kAllocatorVtable);
   for (unsigned i = 0; i < 64; ++i)
      Write32(proxies + (kScriptManVtable - kProxyBase) + i * 4,
              kScriptManTrap + i * kTrapStride);
   for (unsigned i = 0; i < 16; ++i)
      Write32(proxies + (kAllocatorVtable - kProxyBase) + i * 4,
              kAllocatorTrap + i * kTrapStride);
   m_atoms.clear();
   m_services.clear();
   m_initializers.clear();

   return memory->Map(kProxyBase, sizeof(proxies), kMemRead | kMemWrite,
                      error) &&
          memory->Load(kProxyBase, proxies, sizeof(proxies), error) &&
          m_heap.Initialize(memory, kHeapBase, kHeapSize, error);
}

bool cEnvironment::Handles(uint32_t address) const
{
   if (address == kPrintTrap || address == kInitializerTrap)
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
         if (getenv("DARK_OSM32_TRACE"))
         {
            uint32_t returnAddress = 0;
            memory->Read32(state->reg[kEsp], &returnAddress, 0);
            fprintf(stderr,
                    "OSM32 allocator Alloc size=%u result=%08x caller=%08x\n",
                    value, value2, returnAddress);
            fflush(stderr);
         }
         return (!value2 && value) ? false :
                ReturnStdcall(state, memory, 8, value2, error);
      case 4:
         if (!ReadStackArgument(*state, *memory, 1, &value, error) ||
             !ReadStackArgument(*state, *memory, 2, &value2, error)) return false;
         value = m_heap.Realloc(value, value2, error);
         return (!value && value2) ? false :
                ReturnStdcall(state, memory, 12, value, error);
      case 5:
         if (!ReadStackArgument(*state, *memory, 1, &value, error))
            return false;
         if (!m_heap.Free(value, error))
         {
            uint32_t returnAddress = 0;
            memory->Read32(state->reg[kEsp], &returnAddress, 0);
            if (error)
            {
               char context[64];
               snprintf(context, sizeof(context), " (caller 0x%08x)",
                        returnAddress);
               *error += context;
            }
            return false;
         }
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
   if (address == kInitializerTrap)
      return ContinueInitializers(state, memory, error);
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

bool cEnvironment::BeginInitializers(bool stopOnError, sCpuState *state,
                                     cMemory *memory, std::string *error)
{
   uint32_t begin, end;
   if (!ReadStackArgument(*state, *memory, 0, &begin, error) ||
       !ReadStackArgument(*state, *memory, 1, &end, error))
      return false;
   if (end < begin || ((end - begin) & 3) != 0 || end - begin > 0x00100000)
      return Fail(error, "invalid guest CRT initializer range");
   sInitializerContext context;
   context.current = begin;
   context.end = end;
   context.stopOnError = stopOnError;
   m_initializers.push_back(context);
   return ContinueInitializers(state, memory, error);
}

bool cEnvironment::ContinueInitializers(sCpuState *state, cMemory *memory,
                                        std::string *error)
{
   if (m_initializers.empty())
      return Fail(error, "unexpected guest CRT initializer return");

   sInitializerContext &context = m_initializers.back();
   if (state->eip == kInitializerTrap && context.stopOnError &&
       state->reg[kEax] != 0)
   {
      const uint32_t result = state->reg[kEax];
      m_initializers.pop_back();
      return ReturnStdcall(state, memory, 0, result, error);
   }

   while (context.current < context.end)
   {
      uint32_t function;
      if (!memory->Read32(context.current, &function, error))
         return false;
      context.current += 4;
      if (!function)
         continue;
      if (state->reg[kEsp] < 4)
         return Fail(error, "guest CRT initializer stack underflow");
      state->reg[kEsp] -= 4;
      if (!memory->Write32(state->reg[kEsp], kInitializerTrap, error))
         return false;
      state->eip = function;
      return true;
   }

   m_initializers.pop_back();
   return ReturnStdcall(state, memory, 0, 0, error);
}

bool cEnvironment::InvokeImport(const sImport &item, sCpuState *state,
                                cMemory *memory, std::string *error)
{
   if (IsCrtImport(item, "toupper") || IsCrtImport(item, "isspace"))
   {
      uint32_t character;
      if (!ReadStackArgument(*state, *memory, 0, &character, error))
         return false;
      uint32_t result = 0;
      if (item.name == "toupper")
         result = character >= 'a' && character <= 'z' ?
            character - ('a' - 'A') : character;
      else
         result = character == ' ' || character == '\t' ||
                  character == '\n' || character == '\r' ||
                  character == '\f' || character == '\v';
      // CRT functions use cdecl; the guest caller removes their arguments.
      return ReturnStdcall(state, memory, 0, result, error);
   }
   if (IsCrtImport(item, "_stricmp") || IsCrtImport(item, "_strnicmp"))
   {
      uint32_t leftAddress, rightAddress, count = 0xffffffffu;
      std::string left, right;
      if (!ReadStackArgument(*state, *memory, 0, &leftAddress, error) ||
          !ReadStackArgument(*state, *memory, 1, &rightAddress, error) ||
          !ReadCString(*memory, leftAddress, &left, error) ||
          !ReadCString(*memory, rightAddress, &right, error) ||
          (item.name == "_strnicmp" &&
           !ReadStackArgument(*state, *memory, 2, &count, error)))
         return false;
      size_t i = 0;
      while (i < count && i < left.size() && i < right.size() &&
             AsciiLower(static_cast<uint8_t>(left[i])) ==
                AsciiLower(static_cast<uint8_t>(right[i])))
         ++i;
      if (i == count)
         return ReturnStdcall(state, memory, 0, 0, error);
      const int result =
         static_cast<int>(AsciiLower(i < left.size() ?
            static_cast<uint8_t>(left[i]) : 0)) -
         static_cast<int>(AsciiLower(i < right.size() ?
            static_cast<uint8_t>(right[i]) : 0));
      return ReturnStdcall(state, memory, 0,
                           static_cast<uint32_t>(result), error);
   }
   if (IsCrtImport(item, "strlen"))
   {
      uint32_t address;
      std::string value;
      if (!ReadStackArgument(*state, *memory, 0, &address, error) ||
          !ReadCString(*memory, address, &value, error))
         return false;
      return ReturnStdcall(state, memory, 0,
                           static_cast<uint32_t>(value.size()), error);
   }
   if (IsCrtImport(item, "strcmp"))
   {
      uint32_t leftAddress, rightAddress;
      std::string left, right;
      if (!ReadStackArgument(*state, *memory, 0, &leftAddress, error) ||
          !ReadStackArgument(*state, *memory, 1, &rightAddress, error) ||
          !ReadCString(*memory, leftAddress, &left, error) ||
          !ReadCString(*memory, rightAddress, &right, error))
         return false;
      const int result = left.compare(right);
      return ReturnStdcall(state, memory, 0,
                           static_cast<uint32_t>(result), error);
   }
   if (IsCrtImport(item, "strcpy") || IsCrtImport(item, "strcat"))
   {
      uint32_t destination, source;
      std::string prefix, value;
      if (!ReadStackArgument(*state, *memory, 0, &destination, error) ||
          !ReadStackArgument(*state, *memory, 1, &source, error) ||
          !ReadCString(*memory, source, &value, error) ||
          (item.name == "strcat" &&
           !ReadCString(*memory, destination, &prefix, error)))
         return false;
      const uint32_t offset = static_cast<uint32_t>(prefix.size());
      for (uint32_t i = 0; i <= value.size(); ++i)
         if (!memory->Write8(destination + offset + i,
                             i < value.size() ?
                                static_cast<uint8_t>(value[i]) : 0,
                             error))
            return false;
      return ReturnStdcall(state, memory, 0, destination, error);
   }
   if (IsCrtImport(item, "strchr") || IsCrtImport(item, "strrchr") ||
       IsCrtImport(item, "strstr"))
   {
      uint32_t address, argument;
      std::string value;
      if (!ReadStackArgument(*state, *memory, 0, &address, error) ||
          !ReadStackArgument(*state, *memory, 1, &argument, error) ||
          !ReadCString(*memory, address, &value, error))
         return false;
      size_t offset = std::string::npos;
      if (item.name == "strstr")
      {
         std::string needle;
         if (!ReadCString(*memory, argument, &needle, error)) return false;
         offset = value.find(needle);
      }
      else if (item.name == "strchr")
         offset = static_cast<char>(argument) == '\0' ? value.size() :
            value.find(static_cast<char>(argument));
      else
         offset = static_cast<char>(argument) == '\0' ? value.size() :
            value.rfind(static_cast<char>(argument));
      return ReturnStdcall(state, memory, 0,
         offset == std::string::npos ? 0 :
            address + static_cast<uint32_t>(offset), error);
   }
   if (IsCrtImport(item, "strcspn") || IsCrtImport(item, "strpbrk") ||
       IsCrtImport(item, "strspn"))
   {
      uint32_t address, setAddress;
      std::string value, set;
      if (!ReadStackArgument(*state, *memory, 0, &address, error) ||
          !ReadStackArgument(*state, *memory, 1, &setAddress, error) ||
          !ReadCString(*memory, address, &value, error) ||
          !ReadCString(*memory, setAddress, &set, error))
         return false;
      size_t offset;
      if (item.name == "strspn")
         offset = value.find_first_not_of(set);
      else
         offset = value.find_first_of(set);
      if (offset == std::string::npos)
         offset = value.size();
      const uint32_t result = item.name == "strpbrk" ?
         (offset == value.size() ? 0 :
            address + static_cast<uint32_t>(offset)) :
         static_cast<uint32_t>(offset);
      return ReturnStdcall(state, memory, 0, result, error);
   }
   if (IsCrtImport(item, "strtol") || IsCrtImport(item, "strtod"))
   {
      uint32_t address, endAddress;
      std::string value;
      if (!ReadStackArgument(*state, *memory, 0, &address, error) ||
          !ReadStackArgument(*state, *memory, 1, &endAddress, error) ||
          !ReadCString(*memory, address, &value, error))
         return false;
      char *end = 0;
      if (item.name == "strtol")
      {
         uint32_t base;
         if (!ReadStackArgument(*state, *memory, 2, &base, error))
            return false;
         const long result = strtol(value.c_str(), &end,
                                    static_cast<int>(base));
         if (endAddress && !memory->Write32(endAddress, address +
               static_cast<uint32_t>(end - value.c_str()), error))
            return false;
         return ReturnStdcall(state, memory, 0,
                              static_cast<uint32_t>(result), error);
      }
      const double result = strtod(value.c_str(), &end);
      if (endAddress && !memory->Write32(endAddress, address +
            static_cast<uint32_t>(end - value.c_str()), error))
         return false;
      return ReturnCrtDouble(state, memory, result, error);
   }
   if (IsCrtImport(item, "cos") || IsCrtImport(item, "exp") ||
       IsCrtImport(item, "log") || IsCrtImport(item, "log10") ||
       IsCrtImport(item, "sin") || IsCrtImport(item, "sqrt") ||
       IsCrtImport(item, "pow"))
   {
      double first;
      if (!ReadStackDouble(*state, *memory, 0, &first, error)) return false;
      double result;
      if (item.name == "cos") result = std::cos(first);
      else if (item.name == "exp") result = std::exp(first);
      else if (item.name == "log") result = std::log(first);
      else if (item.name == "log10") result = std::log10(first);
      else if (item.name == "sin") result = std::sin(first);
      else if (item.name == "sqrt") result = std::sqrt(first);
      else
      {
         double second;
         if (!ReadStackDouble(*state, *memory, 2, &second, error))
            return false;
         result = std::pow(first, second);
      }
      return ReturnCrtDouble(state, memory, result, error);
   }
   if (IsCrtImport(item, "fflush"))
      return ReturnStdcall(state, memory, 0, 0, error);
   if (IsCrtImport(item, "_itoa"))
   {
      uint32_t rawValue, destination, radix;
      if (!ReadStackArgument(*state, *memory, 0, &rawValue, error) ||
          !ReadStackArgument(*state, *memory, 1, &destination, error) ||
          !ReadStackArgument(*state, *memory, 2, &radix, error))
         return false;
      if (!destination || radix < 2 || radix > 36)
         return Fail(error, "invalid guest _itoa argument");

      char converted[40];
      size_t length = 0;
      uint32_t magnitude = rawValue;
      const bool negative = radix == 10 &&
                            static_cast<int32_t>(rawValue) < 0;
      if (negative)
         magnitude = 0u - rawValue;
      do
      {
         const uint32_t digit = magnitude % radix;
         converted[length++] = static_cast<char>(
            digit < 10 ? '0' + digit : 'a' + digit - 10);
         magnitude /= radix;
      } while (magnitude && length < sizeof(converted) - 1);
      if (negative)
         converted[length++] = '-';
      for (size_t i = 0; i < length; ++i)
         if (!memory->Write8(destination + static_cast<uint32_t>(i),
                             static_cast<uint8_t>(converted[length - i - 1]),
                             error))
            return false;
      if (!memory->Write8(destination + static_cast<uint32_t>(length), 0,
                          error))
         return false;
      return ReturnStdcall(state, memory, 0, destination, error);
   }
   if (IsCrtImport(item, "_CIsqrt") || IsCrtImport(item, "_CIsin") ||
       IsCrtImport(item, "_CIcos"))
   {
      if (!state->x87Depth)
         return Fail(error, "guest CRT intrinsic x87 stack underflow");
      if (item.name == "_CIsqrt")
         state->x87[0] = std::sqrt(state->x87[0]);
      else if (item.name == "_CIsin")
         state->x87[0] = std::sin(state->x87[0]);
      else
         state->x87[0] = std::cos(state->x87[0]);
      return ReturnStdcall(state, memory, 0, state->reg[kEax], error);
   }
   if (IsCrtImport(item, "_initterm_e"))
      return BeginInitializers(true, state, memory, error);
   if (IsCrtImport(item, "_initterm"))
      return BeginInitializers(false, state, memory, error);
   if (IsCrtImport(item, "_encode_pointer") ||
       IsCrtImport(item, "_decode_pointer"))
   {
      uint32_t pointer;
      if (!ReadStackArgument(*state, *memory, 0, &pointer, error))
         return false;
      return ReturnStdcall(state, memory, 0, pointer, error);
   }
   if (IsCrtImport(item, "_lock") || IsCrtImport(item, "_unlock") ||
       IsCrtImport(item, "_crt_debugger_hook"))
      return ReturnStdcall(state, memory, 0, 0, error);
   if (IsCrtImport(item, "__dllonexit") || IsCrtImport(item, "_onexit"))
   {
      uint32_t function;
      if (!ReadStackArgument(*state, *memory, 0, &function, error))
         return false;
      // Destructors are process-lifetime cleanup for these modules.  The
      // guest image is discarded as a unit, so retaining the registration is
      // unnecessary, but registration must report success to the CRT.
      return ReturnStdcall(state, memory, 0, function, error);
   }
   if (IsCrtImport(item, "malloc") || IsCrtImport(item, "_malloc_crt"))
   {
      uint32_t size;
      if (!ReadStackArgument(*state, *memory, 0, &size, error))
         return false;
      const uint32_t allocation = m_heap.Alloc(size, error);
      if (!allocation && size)
         return false;
      return ReturnStdcall(state, memory, 0, allocation, error);
   }
   if (IsCrtImport(item, "realloc"))
   {
      uint32_t allocation, size;
      if (!ReadStackArgument(*state, *memory, 0, &allocation, error) ||
          !ReadStackArgument(*state, *memory, 1, &size, error))
         return false;
      const uint32_t resized = m_heap.Realloc(allocation, size, error);
      if (!resized && size)
         return false;
      return ReturnStdcall(state, memory, 0, resized, error);
   }
   if (IsCrtImport(item, "free"))
   {
      uint32_t allocation;
      if (!ReadStackArgument(*state, *memory, 0, &allocation, error))
         return false;
      if (allocation && !m_heap.Free(allocation, error))
         return false;
      return ReturnStdcall(state, memory, 0, 0, error);
   }
   if (IsCrtImport(item, "memset"))
   {
      uint32_t destination, value, count;
      if (!ReadStackArgument(*state, *memory, 0, &destination, error) ||
          !ReadStackArgument(*state, *memory, 1, &value, error) ||
          !ReadStackArgument(*state, *memory, 2, &count, error))
         return false;
      if (count > 0x02000000)
         return Fail(error, "guest memset is too large");
      for (uint32_t i = 0; i < count; ++i)
         if (!memory->Write8(destination + i, static_cast<uint8_t>(value),
                             error))
            return false;
      return ReturnStdcall(state, memory, 0, destination, error);
   }
   if (IsCrtImport(item, "memcpy") || IsCrtImport(item, "memmove"))
   {
      uint32_t destination, source, count;
      if (!ReadStackArgument(*state, *memory, 0, &destination, error) ||
          !ReadStackArgument(*state, *memory, 1, &source, error) ||
          !ReadStackArgument(*state, *memory, 2, &count, error))
         return false;
      if (count > 0x02000000)
         return Fail(error, "guest memory copy is too large");
      std::vector<uint8_t> bytes(count);
      for (uint32_t i = 0; i < count; ++i)
         if (!memory->Read8(source + i, &bytes[i], error))
            return false;
      for (uint32_t i = 0; i < count; ++i)
         if (!memory->Write8(destination + i, bytes[i], error))
            return false;
      return ReturnStdcall(state, memory, 0, destination, error);
   }
   if (IsCrtImport(item, "memcmp") || IsCrtImport(item, "memchr"))
   {
      uint32_t left, rightOrValue, count;
      if (!ReadStackArgument(*state, *memory, 0, &left, error) ||
          !ReadStackArgument(*state, *memory, 1, &rightOrValue, error) ||
          !ReadStackArgument(*state, *memory, 2, &count, error))
         return false;
      for (uint32_t i = 0; i < count; ++i)
      {
         uint8_t a, b;
         if (!memory->Read8(left + i, &a, error)) return false;
         if (item.name == "memchr")
         {
            if (a == static_cast<uint8_t>(rightOrValue))
               return ReturnStdcall(state, memory, 0, left + i, error);
         }
         else
         {
            if (!memory->Read8(rightOrValue + i, &b, error)) return false;
            if (a != b)
               return ReturnStdcall(state, memory, 0,
                  static_cast<uint32_t>(static_cast<int>(a) - b), error);
         }
      }
      return ReturnStdcall(state, memory, 0, 0, error);
   }
   if (IsCrtImport(item, "strncpy"))
   {
      uint32_t destination, source, count;
      if (!ReadStackArgument(*state, *memory, 0, &destination, error) ||
          !ReadStackArgument(*state, *memory, 1, &source, error) ||
          !ReadStackArgument(*state, *memory, 2, &count, error))
         return false;
      bool terminated = false;
      for (uint32_t i = 0; i < count; ++i)
      {
         uint8_t value = 0;
         if (!terminated && !memory->Read8(source + i, &value, error))
            return false;
         if (!value)
            terminated = true;
         if (!memory->Write8(destination + i, value, error))
            return false;
      }
      return ReturnStdcall(state, memory, 0, destination, error);
   }

   if (IsKernelImport(item, "GetVersion"))
      return ReturnStdcall(state, memory, 0, 5, error);
   if (IsKernelImport(item, "DisableThreadLibraryCalls"))
      return ReturnStdcall(state, memory, 4, 1, error);
   if (IsKernelImport(item, "FindAtomA") ||
       IsKernelImport(item, "AddAtomA"))
   {
      uint32_t nameAddress;
      std::string name;
      if (!ReadStackArgument(*state, *memory, 0, &nameAddress, error))
         return false;
      // MAKEINTATOM values name a predefined atom directly.
      if (nameAddress && nameAddress < 0x10000)
         return ReturnStdcall(state, memory, 4, nameAddress, error);
      if (!ReadCString(*memory, nameAddress, &name, error)) return false;
      for (size_t i = 0; i < m_atoms.size(); ++i)
         if (EqualAsciiNoCase(m_atoms[i].name, name))
         {
            if (item.name == "AddAtomA") ++m_atoms[i].references;
            return ReturnStdcall(state, memory, 4,
               0xc000u + static_cast<uint32_t>(i), error);
         }
      if (item.name == "FindAtomA")
         return ReturnStdcall(state, memory, 4, 0, error);
      if (m_atoms.size() >= 0x4000)
         return ReturnStdcall(state, memory, 4, 0, error);
      sAtom atom;
      atom.name = name;
      atom.references = 1;
      m_atoms.push_back(atom);
      return ReturnStdcall(state, memory, 4,
         0xc000u + static_cast<uint32_t>(m_atoms.size() - 1), error);
   }
   if (IsKernelImport(item, "GetAtomNameA"))
   {
      uint32_t atom, destination, capacity;
      if (!ReadStackArgument(*state, *memory, 0, &atom, error) ||
          !ReadStackArgument(*state, *memory, 1, &destination, error) ||
          !ReadStackArgument(*state, *memory, 2, &capacity, error))
         return false;
      if (atom < 0xc000 || atom - 0xc000 >= m_atoms.size() || !capacity)
         return ReturnStdcall(state, memory, 12, 0, error);
      const std::string &name = m_atoms[atom - 0xc000].name;
      const uint32_t copied = static_cast<uint32_t>(
         name.size() < capacity - 1 ? name.size() : capacity - 1);
      for (uint32_t i = 0; i < copied; ++i)
         if (!memory->Write8(destination + i,
                             static_cast<uint8_t>(name[i]), error))
            return false;
      if (!memory->Write8(destination + copied, 0, error)) return false;
      return ReturnStdcall(state, memory, 12, copied, error);
   }
   if (IsKernelImport(item, "GetModuleHandleA"))
   {
      uint32_t name;
      if (!ReadStackArgument(*state, *memory, 0, &name, error)) return false;
      return ReturnStdcall(state, memory, 4, name ? 0 : m_moduleHandle, error);
   }
   if (IsKernelImport(item, "GetProcessHeap"))
      return ReturnStdcall(state, memory, 0, kGuestHeapHandle, error);
   if (IsKernelImport(item, "GlobalAlloc"))
   {
      uint32_t flags, size;
      if (!ReadStackArgument(*state, *memory, 0, &flags, error) ||
          !ReadStackArgument(*state, *memory, 1, &size, error)) return false;
      const uint32_t allocation = m_heap.Alloc(size, error);
      if (!allocation && size) return false;
      if (flags & 0x40)
         for (uint32_t i = 0; i < size; ++i)
            if (!memory->Write8(allocation + i, 0, error)) return false;
      return ReturnStdcall(state, memory, 8, allocation, error);
   }
   if (IsKernelImport(item, "HeapCreate"))
      return ReturnStdcall(state, memory, 12, kGuestHeapHandle, error);
   if (IsKernelImport(item, "HeapDestroy"))
   {
      uint32_t heap;
      if (!ReadStackArgument(*state, *memory, 0, &heap, error)) return false;
      return ReturnStdcall(state, memory, 4,
                           heap == kGuestHeapHandle ? 1 : 0, error);
   }
   if (IsKernelImport(item, "HeapAlloc"))
   {
      uint32_t heap, flags, size;
      if (!ReadStackArgument(*state, *memory, 0, &heap, error) ||
          !ReadStackArgument(*state, *memory, 1, &flags, error) ||
          !ReadStackArgument(*state, *memory, 2, &size, error)) return false;
      if (heap != kGuestHeapHandle)
         return ReturnStdcall(state, memory, 12, 0, error);
      const uint32_t allocation = m_heap.Alloc(size, error);
      if (!allocation && size) return false;
      if (flags & 8)
         for (uint32_t i = 0; i < size; ++i)
            if (!memory->Write8(allocation + i, 0, error)) return false;
      return ReturnStdcall(state, memory, 12, allocation, error);
   }
   if (IsKernelImport(item, "HeapReAlloc"))
   {
      uint32_t heap, flags, allocation, size;
      if (!ReadStackArgument(*state, *memory, 0, &heap, error) ||
          !ReadStackArgument(*state, *memory, 1, &flags, error) ||
          !ReadStackArgument(*state, *memory, 2, &allocation, error) ||
          !ReadStackArgument(*state, *memory, 3, &size, error)) return false;
      if (heap != kGuestHeapHandle)
         return ReturnStdcall(state, memory, 16, 0, error);
      const uint32_t oldSize = m_heap.GetSize(allocation);
      const uint32_t resized = m_heap.Realloc(allocation, size, error);
      if (!resized && size) return false;
      if ((flags & 8) && size > oldSize)
         for (uint32_t i = oldSize; i < size; ++i)
            if (!memory->Write8(resized + i, 0, error)) return false;
      return ReturnStdcall(state, memory, 16, resized, error);
   }
   if (IsKernelImport(item, "HeapFree"))
   {
      uint32_t heap, allocation;
      if (!ReadStackArgument(*state, *memory, 0, &heap, error) ||
          !ReadStackArgument(*state, *memory, 2, &allocation, error))
         return false;
      const bool freed = heap == kGuestHeapHandle &&
                         (!allocation || m_heap.Free(allocation, error));
      return ReturnStdcall(state, memory, 12, freed ? 1 : 0, error);
   }
   if (IsKernelImport(item, "HeapSize"))
   {
      uint32_t heap, allocation;
      if (!ReadStackArgument(*state, *memory, 0, &heap, error) ||
          !ReadStackArgument(*state, *memory, 2, &allocation, error))
         return false;
      const uint32_t size = heap == kGuestHeapHandle ?
         m_heap.GetSize(allocation) : 0xffffffffu;
      return ReturnStdcall(state, memory, 12,
                           size ? size : 0xffffffffu, error);
   }
   if (IsKernelImport(item, "InitializeCriticalSection") ||
       IsKernelImport(item, "DeleteCriticalSection") ||
       IsKernelImport(item, "EnterCriticalSection") ||
       IsKernelImport(item, "LeaveCriticalSection"))
      return ReturnStdcall(state, memory, 4, 0, error);
   if (IsKernelImport(item, "InterlockedCompareExchange"))
   {
      uint32_t destination, exchange, comparand, previous;
      if (!ReadStackArgument(*state, *memory, 0, &destination, error) ||
          !ReadStackArgument(*state, *memory, 1, &exchange, error) ||
          !ReadStackArgument(*state, *memory, 2, &comparand, error) ||
          !memory->Read32(destination, &previous, error))
         return false;
      if (previous == comparand &&
          !memory->Write32(destination, exchange, error))
         return false;
      return ReturnStdcall(state, memory, 12, previous, error);
   }
   if (IsKernelImport(item, "InterlockedExchange"))
   {
      uint32_t destination, exchange, previous;
      if (!ReadStackArgument(*state, *memory, 0, &destination, error) ||
          !ReadStackArgument(*state, *memory, 1, &exchange, error) ||
          !memory->Read32(destination, &previous, error) ||
          !memory->Write32(destination, exchange, error))
         return false;
      return ReturnStdcall(state, memory, 8, previous, error);
   }
   if (IsKernelImport(item, "InterlockedIncrement") ||
       IsKernelImport(item, "InterlockedDecrement"))
   {
      uint32_t destination, previous;
      if (!ReadStackArgument(*state, *memory, 0, &destination, error) ||
          !memory->Read32(destination, &previous, error))
         return false;
      const uint32_t result = item.name == "InterlockedIncrement" ?
         previous + 1 : previous - 1;
      if (!memory->Write32(destination, result, error)) return false;
      return ReturnStdcall(state, memory, 4, result, error);
   }
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
   {
      static const std::chrono::steady_clock::time_point start =
         std::chrono::steady_clock::now();
      const uint64_t milliseconds = static_cast<uint64_t>(
         std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start).count());
      return ReturnStdcall(state, memory, 0,
                           static_cast<uint32_t>(milliseconds), error);
   }
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
