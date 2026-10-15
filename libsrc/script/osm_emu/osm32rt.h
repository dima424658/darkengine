// Portable execution and ABI wrapper for legacy 32-bit OSM modules.

#ifndef __OSM32RT_H
#define __OSM32RT_H

#include <osm_emu/osm32bridge.h>
#include <osm_emu/osm32img.h>

#pragma pack(push, 8)
#include <stdint.h>
#include <string>
#include <vector>
#pragma pack(pop)

namespace osm32
{

struct sGuestClass
{
   std::string module;
   std::string name;
   std::string base;
   uint32_t factory;
};

enum eValueType
{
   kValueUndefined = 0,
   kValueInteger = 1,
   kValueFloat = 2,
   kValueString = 3,
   kValueVector = 4,
   kValueObject = 5
};

struct sValue
{
   eValueType type;
   uint32_t bits;
   std::string text;
   float vector[3];

   sValue();
};

struct sMessage
{
   int32_t from;
   int32_t to;
   std::string name;
   uint32_t time;
   int32_t flags;
   sValue data[3];
   // Bytes following the x86 sScrMsg base, encoded by a message schema in the
   // native adapter. This keeps host object layout out of guest memory.
   std::vector<uint8_t> extension;
   struct sStringPatch
   {
      uint32_t offset;
      std::string value;
   };
   // Native pointers are never copied into an extension. These patches
   // allocate guest-owned strings and write their 32-bit guest addresses.
   std::vector<sStringPatch> extensionStrings;
   struct sValuePatch
   {
      uint32_t offset;
      sValue value;
   };
   // cMultiParm has an eight-byte fixed guest representation, but strings
   // and vectors referenced by it require guest-owned allocations.
   std::vector<sValuePatch> extensionValues;
};

// Supplies the host objects and imported functions visible to a guest OSM.
// Implementations must use guest addresses only; host pointers never cross
// this interface or enter guest memory.
class iEnvironment : public iHostCalls
{
public:
   virtual bool Prepare(cImage *image, cMemory *memory,
                        std::string *error) = 0;
   virtual uint32_t ScriptManagerAddress() const = 0;
   virtual uint32_t PrintAddress() const = 0;
   virtual uint32_t AllocatorAddress() const = 0;
   virtual uint32_t AllocGuest(uint32_t size, std::string *error) = 0;
   virtual bool FreeGuest(uint32_t address, std::string *error) = 0;
};

class cModule
{
public:
   cModule();
   ~cModule();

   bool Load(const char *path, const char *moduleName,
             iEnvironment *environment, std::string *error);
   void Unload();

   const std::string &Name() const { return m_name; }
   const std::vector<sGuestClass> &Classes() const { return m_classes; }
   uint32_t GuestModule() const { return m_module; }

   bool CreateScript(const sGuestClass &scriptClass, int32_t objectId,
                     uint32_t *script, std::string *error);
   bool CallCom(uint32_t object, unsigned slot, const uint32_t *arguments,
                size_t argumentCount, uint32_t *result,
                std::string *error);
   bool ReceiveMessage(uint32_t script, const sMessage &message,
                       sValue *reply, uint32_t debugAction, uint32_t *result,
                       std::string *error);
   bool ReadString(uint32_t address, std::string *value,
                   std::string *error) const;

   cMemory &Memory() { return m_cpu.Memory(); }
   const cMemory &Memory() const { return m_cpu.Memory(); }

private:
   bool Call(uint32_t function, const uint32_t *arguments,
             size_t argumentCount, uint32_t *result, std::string *error);
   bool CopyString(const char *value, uint32_t *address,
                   std::string *error);
   bool ReadClasses(std::string *error);
   bool WriteValue(uint32_t address, const sValue &value,
                   std::vector<uint32_t> *allocations, std::string *error);
   bool ReadValue(uint32_t address, sValue *value, std::string *error) const;
   void FreeAllocations(const std::vector<uint32_t> &allocations);

   cImage m_image;
   cCpu m_cpu;
   iEnvironment *m_environment;
   uint32_t m_module;
   unsigned m_callDepth;
   std::string m_name;
   std::vector<sGuestClass> m_classes;
};

} // namespace osm32

#endif // __OSM32RT_H
