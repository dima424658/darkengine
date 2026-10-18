// Portable IA-32 interpreter for legacy OSM modules.

#ifndef __OSM32CPU_H
#define __OSM32CPU_H

#pragma pack(push, 8)
#include <stdint.h>
#include <string>
#include <vector>
#pragma pack(pop)

namespace osm32
{

enum eRegister
{
   kEax,
   kEcx,
   kEdx,
   kEbx,
   kEsp,
   kEbp,
   kEsi,
   kEdi,
   kRegisterCount
};

enum eMemoryAccess
{
   kMemRead = 1,
   kMemWrite = 2,
   kMemExecute = 4
};

struct sCpuState
{
   uint32_t reg[kRegisterCount];
   uint32_t eip;
   uint32_t eflags;
   double x87[8];
   unsigned x87Depth;
   uint16_t x87Status;
   uint16_t x87Control;

   sCpuState();
};

class cMemory
{
public:
   bool Map(uint32_t base, uint32_t size, unsigned access, std::string *error);
   bool Load(uint32_t address, const void *source, size_t size, std::string *error);

   bool Read8(uint32_t address, uint8_t *value, std::string *error) const;
   bool Read16(uint32_t address, uint16_t *value, std::string *error) const;
   bool Read32(uint32_t address, uint32_t *value, std::string *error) const;
   bool Read64(uint32_t address, uint64_t *value, std::string *error) const;
   bool Write8(uint32_t address, uint8_t value, std::string *error);
   bool Write16(uint32_t address, uint16_t value, std::string *error);
   bool Write32(uint32_t address, uint32_t value, std::string *error);
   bool Write64(uint32_t address, uint64_t value, std::string *error);
   bool Fetch8(uint32_t address, uint8_t *value, std::string *error) const;

private:
   struct sRegion
   {
      uint32_t base;
      uint64_t end;
      unsigned access;
      std::vector<uint8_t> bytes;
   };

   const sRegion *Find(uint32_t address, size_t size, unsigned access) const;
   sRegion *Find(uint32_t address, size_t size, unsigned access);
   std::vector<sRegion> m_regions;
};

class iHostCalls
{
public:
   virtual ~iHostCalls() {}
   virtual bool Handles(uint32_t address) const = 0;
   virtual bool Invoke(uint32_t address, sCpuState *state, cMemory *memory,
                       std::string *error) = 0;
};

class cCpu
{
public:
   enum eRunResult
   {
      kRunStopped,
      kRunFault,
      kRunLimit
   };

   cCpu();

   cMemory &Memory() { return m_memory; }
   const cMemory &Memory() const { return m_memory; }
   sCpuState &State() { return m_state; }
   const sCpuState &State() const { return m_state; }
   void SetHostCalls(iHostCalls *host) { m_host = host; }

   bool Push32(uint32_t value, std::string *error);
   bool Pop32(uint32_t *value, std::string *error);
   bool Step(std::string *error);
   eRunResult Run(uint32_t stopAddress, uint64_t instructionLimit,
                  std::string *error);

private:
   struct sDecodedRM
   {
      bool isRegister;
      unsigned reg;
      uint32_t address;
   };

   bool Fetch8(uint8_t *value, std::string *error);
   bool Fetch16(uint16_t *value, std::string *error);
   bool Fetch32(uint32_t *value, std::string *error);
   bool DecodeRM(uint8_t modrm, sDecodedRM *operand, unsigned *regField,
                 std::string *error);
   bool ReadRM32(const sDecodedRM &operand, uint32_t *value,
                 std::string *error) const;
   bool WriteRM32(const sDecodedRM &operand, uint32_t value,
                  std::string *error);
   bool ReadRM16(const sDecodedRM &operand, uint16_t *value,
                 std::string *error) const;
   bool WriteRM16(const sDecodedRM &operand, uint16_t value,
                  std::string *error);
   bool ReadRM8(const sDecodedRM &operand, uint8_t *value,
                std::string *error) const;
   bool WriteRM8(const sDecodedRM &operand, uint8_t value,
                 std::string *error);
   uint8_t ReadReg8(unsigned reg) const;
   void WriteReg8(unsigned reg, uint8_t value);

   void SetLogicFlags(uint32_t result);
   void SetLogicFlags16(uint16_t result);
   void SetLogicFlags8(uint8_t result);
   void SetAddFlags(uint32_t left, uint32_t right, uint32_t result);
   void SetAdcFlags(uint32_t left, uint32_t right, bool carry,
                    uint32_t result);
   void SetAddFlags16(uint16_t left, uint16_t right, uint16_t result);
   void SetAddFlags8(uint8_t left, uint8_t right, uint8_t result);
   void SetAdcFlags8(uint8_t left, uint8_t right, bool carry,
                     uint8_t result);
   void SetSubFlags(uint32_t left, uint32_t right, uint32_t result);
   void SetSbbFlags(uint32_t left, uint32_t right, bool borrow,
                    uint32_t result);
   void SetSubFlags16(uint16_t left, uint16_t right, uint16_t result);
   void SetSubFlags8(uint8_t left, uint8_t right, uint8_t result);
   void SetSbbFlags8(uint8_t left, uint8_t right, bool borrow,
                     uint8_t result);
   void SetIncFlags(uint32_t before, uint32_t result);
   void SetDecFlags(uint32_t before, uint32_t result);
   bool Condition(unsigned condition) const;

   cMemory m_memory;
   sCpuState m_state;
   iHostCalls *m_host;
};

} // namespace osm32

#endif // __OSM32CPU_H
