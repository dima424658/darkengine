// Portable IA-32 interpreter for legacy OSM modules.

#include <osm_emu/osm32cpu.h>

#pragma pack(push, 8)
#include <algorithm>
#include <cmath>
#include <limits>
#include <sstream>
#include <string.h>
#pragma pack(pop)

namespace
{

const uint32_t kFlagCarry = 0x00000001;
const uint32_t kFlagParity = 0x00000004;
const uint32_t kFlagAdjust = 0x00000010;
const uint32_t kFlagZero = 0x00000040;
const uint32_t kFlagSign = 0x00000080;
const uint32_t kFlagOverflow = 0x00000800;
const uint32_t kFlagDirection = 0x00000400;
const uint32_t kArithmeticFlags = kFlagCarry | kFlagParity | kFlagAdjust |
                                  kFlagZero | kFlagSign | kFlagOverflow;

bool Fail(std::string *error, const char *message)
{
   if (error)
      *error = message;
   return false;
}

bool FailAddress(std::string *error, const char *message, uint32_t address)
{
   if (error)
   {
      std::ostringstream text;
      text << message << " 0x" << std::hex << address;
      *error = text.str();
   }
   return false;
}

bool FailOpcode(std::string *error, const char *message, uint32_t address,
                uint8_t opcode)
{
   if (error)
   {
      std::ostringstream text;
      text << message << " 0x" << std::hex << static_cast<unsigned>(opcode)
           << " at 0x" << address;
      *error = text.str();
   }
   return false;
}

bool EvenParity(uint8_t value)
{
   value ^= value >> 4;
   value &= 0x0f;
   return ((0x9669u >> value) & 1) != 0;
}

int32_t Sign8(uint8_t value)
{
   return static_cast<int8_t>(value);
}

} // namespace

namespace osm32
{

sCpuState::sCpuState()
 : eip(0),
   eflags(0x00000002),
   x87Depth(0),
   x87Status(0),
   x87Control(0x037f)
{
   memset(reg, 0, sizeof(reg));
   memset(x87, 0, sizeof(x87));
}

const cMemory::sRegion *cMemory::Find(uint32_t address, size_t size,
                                      unsigned access) const
{
   const uint64_t end = static_cast<uint64_t>(address) + size;
   if (end > 0x100000000ull)
      return 0;
   for (size_t i = 0; i < m_regions.size(); ++i)
   {
      const sRegion &region = m_regions[i];
      if (address >= region.base && end <= region.end &&
          (region.access & access) == access)
         return &region;
   }
   return 0;
}

cMemory::sRegion *cMemory::Find(uint32_t address, size_t size, unsigned access)
{
   return const_cast<sRegion *>(
      static_cast<const cMemory *>(this)->Find(address, size, access));
}

bool cMemory::Map(uint32_t base, uint32_t size, unsigned access,
                  std::string *error)
{
   if (!size)
      return Fail(error, "cannot map an empty guest region");
   const uint64_t end = static_cast<uint64_t>(base) + size;
   if (end > 0x100000000ull)
      return Fail(error, "guest region wraps the 32-bit address space");

   for (size_t i = 0; i < m_regions.size(); ++i)
      if (base < m_regions[i].end && end > m_regions[i].base)
         return Fail(error, "guest memory regions overlap");

   sRegion region;
   region.base = base;
   region.end = end;
   region.access = access;
   try
   {
      region.bytes.assign(size, 0);
      m_regions.push_back(region);
   }
   catch (...)
   {
      return Fail(error, "cannot allocate guest memory");
   }

   std::sort(m_regions.begin(), m_regions.end(),
      [](const sRegion &left, const sRegion &right) {
         return left.base < right.base;
      });
   if (error)
      error->clear();
   return true;
}

bool cMemory::Load(uint32_t address, const void *source, size_t size,
                   std::string *error)
{
   sRegion *region = Find(address, size, 0);
   if (!region)
      return FailAddress(error, "guest load targets unmapped memory at", address);
   memcpy(&region->bytes[address - region->base], source, size);
   return true;
}

bool cMemory::Read8(uint32_t address, uint8_t *value, std::string *error) const
{
   const sRegion *region = Find(address, 1, kMemRead);
   if (!region)
      return FailAddress(error, "guest read fault at", address);
   *value = region->bytes[address - region->base];
   return true;
}

bool cMemory::Read16(uint32_t address, uint16_t *value, std::string *error) const
{
   const sRegion *region = Find(address, 2, kMemRead);
   if (!region)
      return FailAddress(error, "guest read fault at", address);
   const uint8_t *p = &region->bytes[address - region->base];
   *value = static_cast<uint16_t>(p[0] | (static_cast<uint16_t>(p[1]) << 8));
   return true;
}

bool cMemory::Read32(uint32_t address, uint32_t *value, std::string *error) const
{
   const sRegion *region = Find(address, 4, kMemRead);
   if (!region)
      return FailAddress(error, "guest read fault at", address);
   const uint8_t *p = &region->bytes[address - region->base];
   *value = static_cast<uint32_t>(p[0]) |
            (static_cast<uint32_t>(p[1]) << 8) |
            (static_cast<uint32_t>(p[2]) << 16) |
            (static_cast<uint32_t>(p[3]) << 24);
   return true;
}

bool cMemory::Read64(uint32_t address, uint64_t *value, std::string *error) const
{
   const sRegion *region = Find(address, 8, kMemRead);
   if (!region)
      return FailAddress(error, "guest read fault at", address);
   const uint8_t *p = &region->bytes[address - region->base];
   *value = static_cast<uint64_t>(p[0]) |
            (static_cast<uint64_t>(p[1]) << 8) |
            (static_cast<uint64_t>(p[2]) << 16) |
            (static_cast<uint64_t>(p[3]) << 24) |
            (static_cast<uint64_t>(p[4]) << 32) |
            (static_cast<uint64_t>(p[5]) << 40) |
            (static_cast<uint64_t>(p[6]) << 48) |
            (static_cast<uint64_t>(p[7]) << 56);
   return true;
}

bool cMemory::Write8(uint32_t address, uint8_t value, std::string *error)
{
   sRegion *region = Find(address, 1, kMemWrite);
   if (!region)
      return FailAddress(error, "guest write fault at", address);
   region->bytes[address - region->base] = value;
   return true;
}

bool cMemory::Write16(uint32_t address, uint16_t value, std::string *error)
{
   sRegion *region = Find(address, 2, kMemWrite);
   if (!region)
      return FailAddress(error, "guest write fault at", address);
   uint8_t *p = &region->bytes[address - region->base];
   p[0] = static_cast<uint8_t>(value);
   p[1] = static_cast<uint8_t>(value >> 8);
   return true;
}

bool cMemory::Write32(uint32_t address, uint32_t value, std::string *error)
{
   sRegion *region = Find(address, 4, kMemWrite);
   if (!region)
      return FailAddress(error, "guest write fault at", address);
   uint8_t *p = &region->bytes[address - region->base];
   p[0] = static_cast<uint8_t>(value);
   p[1] = static_cast<uint8_t>(value >> 8);
   p[2] = static_cast<uint8_t>(value >> 16);
   p[3] = static_cast<uint8_t>(value >> 24);
   return true;
}

bool cMemory::Write64(uint32_t address, uint64_t value, std::string *error)
{
   sRegion *region = Find(address, 8, kMemWrite);
   if (!region)
      return FailAddress(error, "guest write fault at", address);
   uint8_t *p = &region->bytes[address - region->base];
   for (unsigned i = 0; i < 8; ++i)
      p[i] = static_cast<uint8_t>(value >> (i * 8));
   return true;
}

bool cMemory::Fetch8(uint32_t address, uint8_t *value, std::string *error) const
{
   const sRegion *region = Find(address, 1, kMemExecute);
   if (!region)
      return FailAddress(error, "guest execute fault at", address);
   *value = region->bytes[address - region->base];
   return true;
}

cCpu::cCpu()
 : m_host(0)
{
}

bool cCpu::Push32(uint32_t value, std::string *error)
{
   m_state.reg[kEsp] -= 4;
   if (!m_memory.Write32(m_state.reg[kEsp], value, error))
   {
      m_state.reg[kEsp] += 4;
      return false;
   }
   return true;
}

bool cCpu::Pop32(uint32_t *value, std::string *error)
{
   uint32_t popped;
   if (!m_memory.Read32(m_state.reg[kEsp], &popped, error))
      return false;
   m_state.reg[kEsp] += 4;
   *value = popped;
   return true;
}

bool cCpu::Fetch8(uint8_t *value, std::string *error)
{
   if (!m_memory.Fetch8(m_state.eip, value, error))
      return false;
   ++m_state.eip;
   return true;
}

bool cCpu::Fetch16(uint16_t *value, std::string *error)
{
   uint8_t low, high;
   if (!Fetch8(&low, error) || !Fetch8(&high, error))
      return false;
   *value = static_cast<uint16_t>(low | (static_cast<uint16_t>(high) << 8));
   return true;
}

bool cCpu::Fetch32(uint32_t *value, std::string *error)
{
   uint16_t low, high;
   if (!Fetch16(&low, error) || !Fetch16(&high, error))
      return false;
   *value = static_cast<uint32_t>(low) | (static_cast<uint32_t>(high) << 16);
   return true;
}

bool cCpu::DecodeRM(uint8_t modrm, sDecodedRM *operand, unsigned *regField,
                    std::string *error)
{
   const unsigned mod = modrm >> 6;
   const unsigned rm = modrm & 7;
   *regField = (modrm >> 3) & 7;
   if (mod == 3)
   {
      operand->isRegister = true;
      operand->reg = rm;
      operand->address = 0;
      return true;
   }

   operand->isRegister = false;
   operand->reg = 0;
   uint32_t address = 0;
   bool needsDisp32 = false;
   if (rm == 4)
   {
      uint8_t sib;
      if (!Fetch8(&sib, error))
         return false;
      const unsigned scale = sib >> 6;
      const unsigned index = (sib >> 3) & 7;
      const unsigned base = sib & 7;
      if (index != 4)
         address += m_state.reg[index] << scale;
      if (base == 5 && mod == 0)
         needsDisp32 = true;
      else
         address += m_state.reg[base];
   }
   else if (rm == 5 && mod == 0)
      needsDisp32 = true;
   else
      address = m_state.reg[rm];

   if (mod == 1)
   {
      uint8_t displacement;
      if (!Fetch8(&displacement, error))
         return false;
      address += Sign8(displacement);
   }
   else if (mod == 2 || needsDisp32)
   {
      uint32_t displacement;
      if (!Fetch32(&displacement, error))
         return false;
      address += displacement;
   }
   operand->address = address;
   return true;
}

bool cCpu::ReadRM32(const sDecodedRM &operand, uint32_t *value,
                    std::string *error) const
{
   if (operand.isRegister)
   {
      *value = m_state.reg[operand.reg];
      return true;
   }
   return m_memory.Read32(operand.address, value, error);
}

bool cCpu::WriteRM32(const sDecodedRM &operand, uint32_t value,
                     std::string *error)
{
   if (operand.isRegister)
   {
      m_state.reg[operand.reg] = value;
      return true;
   }
   return m_memory.Write32(operand.address, value, error);
}

bool cCpu::ReadRM16(const sDecodedRM &operand, uint16_t *value,
                    std::string *error) const
{
   if (operand.isRegister)
   {
      *value = static_cast<uint16_t>(m_state.reg[operand.reg]);
      return true;
   }
   return m_memory.Read16(operand.address, value, error);
}

bool cCpu::WriteRM16(const sDecodedRM &operand, uint16_t value,
                     std::string *error)
{
   if (operand.isRegister)
   {
      m_state.reg[operand.reg] = (m_state.reg[operand.reg] & 0xffff0000u) |
                                 value;
      return true;
   }
   return m_memory.Write16(operand.address, value, error);
}

bool cCpu::ReadRM8(const sDecodedRM &operand, uint8_t *value,
                   std::string *error) const
{
   if (operand.isRegister)
   {
      *value = ReadReg8(operand.reg);
      return true;
   }
   return m_memory.Read8(operand.address, value, error);
}

bool cCpu::WriteRM8(const sDecodedRM &operand, uint8_t value,
                    std::string *error)
{
   if (operand.isRegister)
   {
      WriteReg8(operand.reg, value);
      return true;
   }
   return m_memory.Write8(operand.address, value, error);
}

uint8_t cCpu::ReadReg8(unsigned reg) const
{
   return reg < 4 ? static_cast<uint8_t>(m_state.reg[reg])
                  : static_cast<uint8_t>(m_state.reg[reg - 4] >> 8);
}

void cCpu::WriteReg8(unsigned reg, uint8_t value)
{
   if (reg < 4)
      m_state.reg[reg] = (m_state.reg[reg] & 0xffffff00u) | value;
   else
   {
      const unsigned base = reg - 4;
      m_state.reg[base] = (m_state.reg[base] & 0xffff00ffu) |
                          (static_cast<uint32_t>(value) << 8);
   }
}

void cCpu::SetLogicFlags(uint32_t result)
{
   m_state.eflags &= ~kArithmeticFlags;
   if (!result)
      m_state.eflags |= kFlagZero;
   if (result & 0x80000000u)
      m_state.eflags |= kFlagSign;
   if (EvenParity(static_cast<uint8_t>(result)))
      m_state.eflags |= kFlagParity;
}

void cCpu::SetLogicFlags16(uint16_t result)
{
   m_state.eflags &= ~kArithmeticFlags;
   if (!result)
      m_state.eflags |= kFlagZero;
   if (result & 0x8000)
      m_state.eflags |= kFlagSign;
   if (EvenParity(static_cast<uint8_t>(result)))
      m_state.eflags |= kFlagParity;
}

void cCpu::SetLogicFlags8(uint8_t result)
{
   m_state.eflags &= ~kArithmeticFlags;
   if (!result)
      m_state.eflags |= kFlagZero;
   if (result & 0x80)
      m_state.eflags |= kFlagSign;
   if (EvenParity(result))
      m_state.eflags |= kFlagParity;
}

void cCpu::SetAddFlags(uint32_t left, uint32_t right, uint32_t result)
{
   m_state.eflags &= ~kArithmeticFlags;
   if (static_cast<uint64_t>(left) + right > 0xffffffffull)
      m_state.eflags |= kFlagCarry;
   if (((left ^ right ^ result) & 0x10) != 0)
      m_state.eflags |= kFlagAdjust;
   if (!result)
      m_state.eflags |= kFlagZero;
   if (result & 0x80000000u)
      m_state.eflags |= kFlagSign;
   if ((~(left ^ right) & (left ^ result) & 0x80000000u) != 0)
      m_state.eflags |= kFlagOverflow;
   if (EvenParity(static_cast<uint8_t>(result)))
      m_state.eflags |= kFlagParity;
}

void cCpu::SetAdcFlags(uint32_t left, uint32_t right, bool carry,
                       uint32_t result)
{
   m_state.eflags &= ~kArithmeticFlags;
   const uint64_t unsignedResult = static_cast<uint64_t>(left) + right + carry;
   const int64_t signedResult = static_cast<int64_t>(static_cast<int32_t>(left)) +
      static_cast<int32_t>(right) + carry;
   if (unsignedResult > 0xffffffffull)
      m_state.eflags |= kFlagCarry;
   if ((left & 0x0f) + (right & 0x0f) + carry > 0x0f)
      m_state.eflags |= kFlagAdjust;
   if (!result)
      m_state.eflags |= kFlagZero;
   if (result & 0x80000000u)
      m_state.eflags |= kFlagSign;
   if (signedResult < -0x80000000ll || signedResult > 0x7fffffffll)
      m_state.eflags |= kFlagOverflow;
   if (EvenParity(static_cast<uint8_t>(result)))
      m_state.eflags |= kFlagParity;
}

void cCpu::SetAddFlags16(uint16_t left, uint16_t right, uint16_t result)
{
   m_state.eflags &= ~kArithmeticFlags;
   if (static_cast<unsigned>(left) + right > 0xffff)
      m_state.eflags |= kFlagCarry;
   if (((left ^ right ^ result) & 0x10) != 0)
      m_state.eflags |= kFlagAdjust;
   if (!result)
      m_state.eflags |= kFlagZero;
   if (result & 0x8000)
      m_state.eflags |= kFlagSign;
   if ((~(left ^ right) & (left ^ result) & 0x8000) != 0)
      m_state.eflags |= kFlagOverflow;
   if (EvenParity(static_cast<uint8_t>(result)))
      m_state.eflags |= kFlagParity;
}

void cCpu::SetAddFlags8(uint8_t left, uint8_t right, uint8_t result)
{
   m_state.eflags &= ~kArithmeticFlags;
   if (static_cast<unsigned>(left) + right > 0xff)
      m_state.eflags |= kFlagCarry;
   if (((left ^ right ^ result) & 0x10) != 0)
      m_state.eflags |= kFlagAdjust;
   if (!result)
      m_state.eflags |= kFlagZero;
   if (result & 0x80)
      m_state.eflags |= kFlagSign;
   if ((~(left ^ right) & (left ^ result) & 0x80) != 0)
      m_state.eflags |= kFlagOverflow;
   if (EvenParity(result))
      m_state.eflags |= kFlagParity;
}

void cCpu::SetAdcFlags8(uint8_t left, uint8_t right, bool carry,
                        uint8_t result)
{
   m_state.eflags &= ~kArithmeticFlags;
   const unsigned unsignedResult =
      static_cast<unsigned>(left) + right + carry;
   const int signedResult = static_cast<int8_t>(left) +
                            static_cast<int8_t>(right) + carry;
   if (unsignedResult > 0xff)
      m_state.eflags |= kFlagCarry;
   if ((left & 0x0f) + (right & 0x0f) + carry > 0x0f)
      m_state.eflags |= kFlagAdjust;
   if (!result)
      m_state.eflags |= kFlagZero;
   if (result & 0x80)
      m_state.eflags |= kFlagSign;
   if (signedResult < -128 || signedResult > 127)
      m_state.eflags |= kFlagOverflow;
   if (EvenParity(result))
      m_state.eflags |= kFlagParity;
}

void cCpu::SetSubFlags(uint32_t left, uint32_t right, uint32_t result)
{
   m_state.eflags &= ~kArithmeticFlags;
   if (left < right)
      m_state.eflags |= kFlagCarry;
   if (((left ^ right ^ result) & 0x10) != 0)
      m_state.eflags |= kFlagAdjust;
   if (!result)
      m_state.eflags |= kFlagZero;
   if (result & 0x80000000u)
      m_state.eflags |= kFlagSign;
   if (((left ^ right) & (left ^ result) & 0x80000000u) != 0)
      m_state.eflags |= kFlagOverflow;
   if (EvenParity(static_cast<uint8_t>(result)))
      m_state.eflags |= kFlagParity;
}

void cCpu::SetSbbFlags(uint32_t left, uint32_t right, bool borrow,
                       uint32_t result)
{
   m_state.eflags &= ~kArithmeticFlags;
   const uint64_t subtrahend = static_cast<uint64_t>(right) + borrow;
   const int64_t signedResult = static_cast<int64_t>(static_cast<int32_t>(left)) -
      static_cast<int32_t>(right) - borrow;
   if (static_cast<uint64_t>(left) < subtrahend)
      m_state.eflags |= kFlagCarry;
   if (static_cast<unsigned>(left & 0x0f) <
       static_cast<unsigned>(right & 0x0f) + borrow)
      m_state.eflags |= kFlagAdjust;
   if (!result)
      m_state.eflags |= kFlagZero;
   if (result & 0x80000000u)
      m_state.eflags |= kFlagSign;
   if (signedResult < -0x80000000ll || signedResult > 0x7fffffffll)
      m_state.eflags |= kFlagOverflow;
   if (EvenParity(static_cast<uint8_t>(result)))
      m_state.eflags |= kFlagParity;
}

void cCpu::SetSubFlags16(uint16_t left, uint16_t right, uint16_t result)
{
   m_state.eflags &= ~kArithmeticFlags;
   if (left < right)
      m_state.eflags |= kFlagCarry;
   if (((left ^ right ^ result) & 0x10) != 0)
      m_state.eflags |= kFlagAdjust;
   if (!result)
      m_state.eflags |= kFlagZero;
   if (result & 0x8000)
      m_state.eflags |= kFlagSign;
   if (((left ^ right) & (left ^ result) & 0x8000) != 0)
      m_state.eflags |= kFlagOverflow;
   if (EvenParity(static_cast<uint8_t>(result)))
      m_state.eflags |= kFlagParity;
}

void cCpu::SetSubFlags8(uint8_t left, uint8_t right, uint8_t result)
{
   m_state.eflags &= ~kArithmeticFlags;
   if (left < right)
      m_state.eflags |= kFlagCarry;
   if (((left ^ right ^ result) & 0x10) != 0)
      m_state.eflags |= kFlagAdjust;
   if (!result)
      m_state.eflags |= kFlagZero;
   if (result & 0x80)
      m_state.eflags |= kFlagSign;
   if (((left ^ right) & (left ^ result) & 0x80) != 0)
      m_state.eflags |= kFlagOverflow;
   if (EvenParity(result))
      m_state.eflags |= kFlagParity;
}

void cCpu::SetSbbFlags8(uint8_t left, uint8_t right, bool borrow,
                        uint8_t result)
{
   m_state.eflags &= ~kArithmeticFlags;
   const unsigned subtrahend = static_cast<unsigned>(right) + borrow;
   const int signedResult = static_cast<int8_t>(left) -
                            static_cast<int8_t>(right) - borrow;
   if (static_cast<unsigned>(left) < subtrahend)
      m_state.eflags |= kFlagCarry;
   if (static_cast<unsigned>(left & 0x0f) <
       static_cast<unsigned>(right & 0x0f) + borrow)
      m_state.eflags |= kFlagAdjust;
   if (!result)
      m_state.eflags |= kFlagZero;
   if (result & 0x80)
      m_state.eflags |= kFlagSign;
   if (signedResult < -128 || signedResult > 127)
      m_state.eflags |= kFlagOverflow;
   if (EvenParity(result))
      m_state.eflags |= kFlagParity;
}

void cCpu::SetIncFlags(uint32_t before, uint32_t result)
{
   const uint32_t carry = m_state.eflags & kFlagCarry;
   SetAddFlags(before, 1, result);
   m_state.eflags = (m_state.eflags & ~kFlagCarry) | carry;
}

void cCpu::SetDecFlags(uint32_t before, uint32_t result)
{
   const uint32_t carry = m_state.eflags & kFlagCarry;
   SetSubFlags(before, 1, result);
   m_state.eflags = (m_state.eflags & ~kFlagCarry) | carry;
}

bool cCpu::Condition(unsigned condition) const
{
   const bool carry = (m_state.eflags & kFlagCarry) != 0;
   const bool parity = (m_state.eflags & kFlagParity) != 0;
   const bool zero = (m_state.eflags & kFlagZero) != 0;
   const bool sign = (m_state.eflags & kFlagSign) != 0;
   const bool overflow = (m_state.eflags & kFlagOverflow) != 0;
   switch (condition & 15)
   {
      case 0: return overflow;
      case 1: return !overflow;
      case 2: return carry;
      case 3: return !carry;
      case 4: return zero;
      case 5: return !zero;
      case 6: return carry || zero;
      case 7: return !carry && !zero;
      case 8: return sign;
      case 9: return !sign;
      case 10: return parity;
      case 11: return !parity;
      case 12: return sign != overflow;
      case 13: return sign == overflow;
      case 14: return zero || sign != overflow;
      case 15: return !zero && sign == overflow;
   }
   return false;
}

bool cCpu::Step(std::string *error)
{
   if (m_host && m_host->Handles(m_state.eip))
      return m_host->Invoke(m_state.eip, &m_state, &m_memory, error);

   const uint32_t instructionAddress = m_state.eip;
   uint8_t opcode;
   if (!Fetch8(&opcode, error))
      return false;

   bool repeat = false;
   bool repeatNotEqual = false;
   bool operand16 = false;
   while (opcode == 0xf2 || opcode == 0xf3 || opcode == 0x66)
   {
      if (opcode == 0xf2 || opcode == 0xf3)
      {
         repeat = true;
         repeatNotEqual = opcode == 0xf2;
      }
      else
         operand16 = true;
      if (!Fetch8(&opcode, error))
         return false;
   }

   if (opcode == 0xa4 || opcode == 0xa5 ||
       opcode == 0xaa || opcode == 0xab ||
       opcode == 0xae || opcode == 0xaf)
   {
      const unsigned width = (opcode & 1) ? (operand16 ? 2 : 4) : 1;
      const int32_t delta = (m_state.eflags & kFlagDirection)
         ? -static_cast<int32_t>(width) : static_cast<int32_t>(width);
      uint32_t count = repeat ? m_state.reg[kEcx] : 1;
      while (count)
      {
         --count;
         if (opcode == 0xa4)
         {
            uint8_t value;
            if (!m_memory.Read8(m_state.reg[kEsi], &value, error) ||
                !m_memory.Write8(m_state.reg[kEdi], value, error))
               return false;
         }
         else if (opcode == 0xa5)
         {
            if (operand16)
            {
               uint16_t value;
               if (!m_memory.Read16(m_state.reg[kEsi], &value, error) ||
                   !m_memory.Write16(m_state.reg[kEdi], value, error))
                  return false;
            }
            else
            {
               uint32_t value;
               if (!m_memory.Read32(m_state.reg[kEsi], &value, error) ||
                   !m_memory.Write32(m_state.reg[kEdi], value, error))
                  return false;
            }
         }
         else if (opcode == 0xaa)
         {
            if (!m_memory.Write8(m_state.reg[kEdi], ReadReg8(0), error))
               return false;
         }
         else if (operand16)
         {
            if (opcode == 0xaf)
            {
               uint16_t value;
               const uint16_t accumulator =
                  static_cast<uint16_t>(m_state.reg[kEax]);
               if (!m_memory.Read16(m_state.reg[kEdi], &value, error))
                  return false;
               SetSubFlags16(accumulator, value,
                             static_cast<uint16_t>(accumulator - value));
            }
            else if (!m_memory.Write16(m_state.reg[kEdi],
                                       static_cast<uint16_t>(m_state.reg[kEax]),
                                       error))
               return false;
         }
         else if (opcode == 0xae)
         {
            uint8_t value;
            const uint8_t accumulator = ReadReg8(0);
            if (!m_memory.Read8(m_state.reg[kEdi], &value, error))
               return false;
            SetSubFlags8(accumulator, value,
                         static_cast<uint8_t>(accumulator - value));
         }
         else if (opcode == 0xaf)
         {
            uint32_t value;
            const uint32_t accumulator = m_state.reg[kEax];
            if (!m_memory.Read32(m_state.reg[kEdi], &value, error))
               return false;
            SetSubFlags(accumulator, value, accumulator - value);
         }
         else if (!m_memory.Write32(m_state.reg[kEdi], m_state.reg[kEax],
                                    error))
            return false;

         if (opcode == 0xa4 || opcode == 0xa5)
            m_state.reg[kEsi] += delta;
         m_state.reg[kEdi] += delta;

         if (repeat && (opcode == 0xae || opcode == 0xaf))
         {
            const bool equal = (m_state.eflags & kFlagZero) != 0;
            if ((repeatNotEqual && equal) || (!repeatNotEqual && !equal))
               break;
         }
      }
      if (repeat)
         m_state.reg[kEcx] = count;
      return true;
   }

   if (operand16)
   {
      if (opcode >= 0x40 && opcode <= 0x4f)
      {
         const unsigned reg = opcode & 7;
         const uint16_t before = static_cast<uint16_t>(m_state.reg[reg]);
         const bool decrement = opcode >= 0x48;
         const uint16_t result = static_cast<uint16_t>(
            decrement ? before - 1 : before + 1);
         const uint32_t carry = m_state.eflags & kFlagCarry;
         if (decrement)
            SetSubFlags16(before, 1, result);
         else
            SetAddFlags16(before, 1, result);
         m_state.eflags = (m_state.eflags & ~kFlagCarry) | carry;
         m_state.reg[reg] = (m_state.reg[reg] & 0xffff0000u) | result;
         return true;
      }
      if (opcode == 0x05 || opcode == 0x0d || opcode == 0x25 ||
          opcode == 0x2d || opcode == 0x35 || opcode == 0x3d)
      {
         uint16_t immediate;
         if (!Fetch16(&immediate, error)) return false;
         const uint16_t left = static_cast<uint16_t>(m_state.reg[kEax]);
         uint16_t result;
         switch (opcode)
         {
            case 0x05: result = left + immediate;
                       SetAddFlags16(left, immediate, result); break;
            case 0x0d: result = left | immediate;
                       SetLogicFlags16(result); break;
            case 0x25: result = left & immediate;
                       SetLogicFlags16(result); break;
            case 0x2d: result = left - immediate;
                       SetSubFlags16(left, immediate, result); break;
            case 0x35: result = left ^ immediate;
                       SetLogicFlags16(result); break;
            default:   result = left - immediate;
                       SetSubFlags16(left, immediate, result); return true;
         }
         m_state.reg[kEax] = (m_state.reg[kEax] & 0xffff0000u) | result;
         return true;
      }
      if (opcode == 0xa1 || opcode == 0xa3)
      {
         uint32_t address;
         if (!Fetch32(&address, error))
            return false;
         if (opcode == 0xa3)
            return m_memory.Write16(address,
               static_cast<uint16_t>(m_state.reg[kEax]), error);
         uint16_t value;
         if (!m_memory.Read16(address, &value, error))
            return false;
         m_state.reg[kEax] = (m_state.reg[kEax] & 0xffff0000u) | value;
         return true;
      }
      if (opcode >= 0xb8 && opcode <= 0xbf)
      {
         uint16_t value;
         if (!Fetch16(&value, error))
            return false;
         const unsigned reg = opcode - 0xb8;
         m_state.reg[reg] = (m_state.reg[reg] & 0xffff0000u) | value;
         return true;
      }
      if (opcode == 0x01 || opcode == 0x03 ||
          opcode == 0x09 || opcode == 0x0b ||
          opcode == 0x21 || opcode == 0x23 ||
          opcode == 0x29 || opcode == 0x2b ||
          opcode == 0x31 || opcode == 0x33 ||
          opcode == 0x39 || opcode == 0x3b)
      {
         uint8_t modrm;
         sDecodedRM operand;
         unsigned regField;
         uint16_t memoryValue;
         if (!Fetch8(&modrm, error) ||
             !DecodeRM(modrm, &operand, &regField, error) ||
             !ReadRM16(operand, &memoryValue, error))
            return false;
         const bool registerDestination = (opcode & 2) != 0;
         const uint16_t registerValue =
            static_cast<uint16_t>(m_state.reg[regField]);
         const uint16_t left = registerDestination
            ? registerValue : memoryValue;
         const uint16_t right = registerDestination
            ? memoryValue : registerValue;
         uint16_t result = 0;
         switch (opcode & 0xfc)
         {
            case 0x00: result = left + right; SetAddFlags16(left, right, result); break;
            case 0x08: result = left | right; SetLogicFlags16(result); break;
            case 0x20: result = left & right; SetLogicFlags16(result); break;
            case 0x28: result = left - right; SetSubFlags16(left, right, result); break;
            case 0x30: result = left ^ right; SetLogicFlags16(result); break;
            case 0x38: result = left - right; SetSubFlags16(left, right, result); return true;
         }
         if (registerDestination)
            m_state.reg[regField] = (m_state.reg[regField] & 0xffff0000u) |
                                    result;
         else
            return WriteRM16(operand, result, error);
         return true;
      }
      if (opcode == 0x89 || opcode == 0x8b)
      {
         uint8_t modrm;
         sDecodedRM operand;
         unsigned regField;
         if (!Fetch8(&modrm, error) ||
             !DecodeRM(modrm, &operand, &regField, error))
            return false;
         if (opcode == 0x89)
            return WriteRM16(operand,
               static_cast<uint16_t>(m_state.reg[regField]), error);
         uint16_t value;
         if (!ReadRM16(operand, &value, error))
            return false;
         m_state.reg[regField] = (m_state.reg[regField] & 0xffff0000u) | value;
         return true;
      }
      if (opcode == 0x85)
      {
         uint8_t modrm;
         sDecodedRM operand;
         unsigned regField;
         uint16_t value;
         if (!Fetch8(&modrm, error) ||
             !DecodeRM(modrm, &operand, &regField, error) ||
             !ReadRM16(operand, &value, error))
            return false;
         SetLogicFlags16(value &
                         static_cast<uint16_t>(m_state.reg[regField]));
         return true;
      }
      if (opcode == 0xc7)
      {
         uint8_t modrm;
         sDecodedRM operand;
         unsigned operation;
         uint16_t value;
         if (!Fetch8(&modrm, error) ||
             !DecodeRM(modrm, &operand, &operation, error))
            return false;
         if (operation != 0)
            return Fail(error, "invalid 16-bit MOV immediate encoding");
         if (!Fetch16(&value, error)) return false;
         return WriteRM16(operand, value, error);
      }
      if (opcode == 0xff)
      {
         uint8_t modrm;
         sDecodedRM operand;
         unsigned operation;
         uint16_t before;
         if (!Fetch8(&modrm, error) ||
             !DecodeRM(modrm, &operand, &operation, error) ||
             !ReadRM16(operand, &before, error))
            return false;
         if (operation != 0 && operation != 1)
            return Fail(error, "unsupported IA-32 16-bit FF operation");
         const uint16_t result = static_cast<uint16_t>(
            operation == 0 ? before + 1 : before - 1);
         const uint32_t carry = m_state.eflags & kFlagCarry;
         if (operation == 0)
            SetAddFlags16(before, 1, result);
         else
            SetSubFlags16(before, 1, result);
         m_state.eflags = (m_state.eflags & ~kFlagCarry) | carry;
         return WriteRM16(operand, result, error);
      }
      if (opcode == 0xf7)
      {
         uint8_t modrm;
         sDecodedRM operand;
         unsigned operation;
         uint16_t value;
         if (!Fetch8(&modrm, error) ||
             !DecodeRM(modrm, &operand, &operation, error) ||
             !ReadRM16(operand, &value, error))
            return false;
         if (operation == 0)
         {
            uint16_t immediate;
            if (!Fetch16(&immediate, error)) return false;
            SetLogicFlags16(value & immediate);
            return true;
         }
         if (operation == 2)
            return WriteRM16(operand, static_cast<uint16_t>(~value), error);
         if (operation == 3)
         {
            const uint16_t result = static_cast<uint16_t>(0 - value);
            SetSubFlags16(0, value, result);
            return WriteRM16(operand, result, error);
         }
         return Fail(error, "unsupported IA-32 16-bit F7 operation");
      }
      if (opcode == 0x81 || opcode == 0x83)
      {
         uint8_t modrm;
         sDecodedRM operand;
         unsigned operation;
         uint16_t left, immediate;
         if (!Fetch8(&modrm, error) ||
             !DecodeRM(modrm, &operand, &operation, error) ||
             !ReadRM16(operand, &left, error))
            return false;
         if (opcode == 0x81)
         {
            if (!Fetch16(&immediate, error)) return false;
         }
         else
         {
            uint8_t value;
            if (!Fetch8(&value, error)) return false;
            immediate = static_cast<uint16_t>(Sign8(value));
         }
         uint16_t result;
         switch (operation)
         {
            case 0: result = left + immediate; SetAddFlags16(left, immediate, result); break;
            case 1: result = left | immediate; SetLogicFlags16(result); break;
            case 4: result = left & immediate; SetLogicFlags16(result); break;
            case 5: result = left - immediate; SetSubFlags16(left, immediate, result); break;
            case 6: result = left ^ immediate; SetLogicFlags16(result); break;
            case 7: result = left - immediate; SetSubFlags16(left, immediate, result); return true;
            default: return Fail(error, "unsupported IA-32 16-bit immediate arithmetic operation");
         }
         return WriteRM16(operand, result, error);
      }
      if (opcode == 0xc1 || opcode == 0xd1 || opcode == 0xd3)
      {
         uint8_t modrm;
         sDecodedRM operand;
         unsigned operation;
         uint16_t value;
         if (!Fetch8(&modrm, error) ||
             !DecodeRM(modrm, &operand, &operation, error) ||
             !ReadRM16(operand, &value, error))
            return false;
         uint8_t rawCount = 1;
         if (opcode == 0xc1 && !Fetch8(&rawCount, error)) return false;
         if (opcode == 0xd3) rawCount = ReadReg8(1);
         unsigned count = rawCount & 31;
         if (!count) return true;

         uint16_t result = value;
         bool carry = false;
         switch (operation)
         {
            case 0: // ROL
               count %= 16;
               if (!count) return true;
               result = static_cast<uint16_t>((value << count) |
                                              (value >> (16 - count)));
               carry = (result & 1) != 0;
               m_state.eflags &= ~(kFlagCarry | kFlagOverflow);
               if (carry) m_state.eflags |= kFlagCarry;
               if (count == 1 && (((result >> 15) & 1) !=
                                  static_cast<unsigned>(carry)))
                  m_state.eflags |= kFlagOverflow;
               break;
            case 1: // ROR
               count %= 16;
               if (!count) return true;
               result = static_cast<uint16_t>((value >> count) |
                                              (value << (16 - count)));
               carry = (result & 0x8000u) != 0;
               m_state.eflags &= ~(kFlagCarry | kFlagOverflow);
               if (carry) m_state.eflags |= kFlagCarry;
               if (count == 1 && (((result >> 15) ^ (result >> 14)) & 1))
                  m_state.eflags |= kFlagOverflow;
               break;
            case 4: case 6: // SHL/SAL
               carry = count <= 16 && ((value >> (16 - count)) & 1) != 0;
               result = count < 16 ? static_cast<uint16_t>(value << count) : 0;
               SetLogicFlags16(result);
               if (carry) m_state.eflags |= kFlagCarry;
               if (count == 1 && (((result >> 15) & 1) !=
                                  static_cast<unsigned>(carry)))
                  m_state.eflags |= kFlagOverflow;
               break;
            case 5: // SHR
               carry = count <= 16 && ((value >> (count - 1)) & 1) != 0;
               result = count < 16 ? static_cast<uint16_t>(value >> count) : 0;
               SetLogicFlags16(result);
               if (carry) m_state.eflags |= kFlagCarry;
               if (count == 1 && (value & 0x8000u))
                  m_state.eflags |= kFlagOverflow;
               break;
            case 7: // SAR
               carry = count <= 16 && ((value >> (count - 1)) & 1) != 0;
               result = count < 16 ? static_cast<uint16_t>(
                  static_cast<int16_t>(value) >> count) :
                  static_cast<uint16_t>((value & 0x8000u) ? 0xffffu : 0);
               SetLogicFlags16(result);
               if (carry) m_state.eflags |= kFlagCarry;
               break;
            default:
               return Fail(error, "unsupported IA-32 16-bit shift operation");
         }
         return WriteRM16(operand, result, error);
      }
      return FailAddress(error, "unsupported IA-32 operand-size override at",
                         instructionAddress);
   }

   if (opcode >= 0x40 && opcode <= 0x47)
   {
      const unsigned reg = opcode - 0x40;
      const uint32_t before = m_state.reg[reg];
      const uint32_t result = before + 1;
      m_state.reg[reg] = result;
      SetIncFlags(before, result);
      return true;
   }
   if (opcode >= 0x48 && opcode <= 0x4f)
   {
      const unsigned reg = opcode - 0x48;
      const uint32_t before = m_state.reg[reg];
      const uint32_t result = before - 1;
      m_state.reg[reg] = result;
      SetDecFlags(before, result);
      return true;
   }
   if (opcode >= 0x50 && opcode <= 0x57)
      return Push32(m_state.reg[opcode - 0x50], error);
   if (opcode >= 0x58 && opcode <= 0x5f)
      return Pop32(&m_state.reg[opcode - 0x58], error);
   if (opcode >= 0x70 && opcode <= 0x7f)
   {
      uint8_t displacement;
      if (!Fetch8(&displacement, error))
         return false;
      if (Condition(opcode - 0x70))
         m_state.eip += Sign8(displacement);
      return true;
   }
   if (opcode >= 0xb0 && opcode <= 0xb7)
   {
      uint8_t value;
      if (!Fetch8(&value, error))
         return false;
      WriteReg8(opcode - 0xb0, value);
      return true;
   }
   if (opcode >= 0xb8 && opcode <= 0xbf)
      return Fetch32(&m_state.reg[opcode - 0xb8], error);

   switch (opcode)
   {
      case 0x00: case 0x02:
      case 0x08: case 0x0a:
      case 0x10: case 0x12:
      case 0x18: case 0x1a:
      case 0x20: case 0x22:
      case 0x28: case 0x2a:
      case 0x30: case 0x32:
      case 0x38: case 0x3a:
      {
         uint8_t modrm;
         sDecodedRM operand;
         unsigned regField;
         uint8_t memoryValue;
         if (!Fetch8(&modrm, error) ||
             !DecodeRM(modrm, &operand, &regField, error) ||
             !ReadRM8(operand, &memoryValue, error))
            return false;
         const bool registerDestination = (opcode & 2) != 0;
         const uint8_t left = registerDestination
            ? ReadReg8(regField) : memoryValue;
         const uint8_t right = registerDestination
            ? memoryValue : ReadReg8(regField);
         uint8_t result = 0;
         const bool carry = (m_state.eflags & kFlagCarry) != 0;
         switch (opcode & 0xfc)
         {
            case 0x00: result = left + right; SetAddFlags8(left, right, result); break;
            case 0x08: result = left | right; SetLogicFlags8(result); break;
            case 0x10:
               result = left + right + carry;
               SetAdcFlags8(left, right, carry, result);
               break;
            case 0x18:
               result = left - right - carry;
               SetSbbFlags8(left, right, carry, result);
               break;
            case 0x20: result = left & right; SetLogicFlags8(result); break;
            case 0x28: result = left - right; SetSubFlags8(left, right, result); break;
            case 0x30: result = left ^ right; SetLogicFlags8(result); break;
            case 0x38: result = left - right; SetSubFlags8(left, right, result); return true;
         }
         if (registerDestination)
            WriteReg8(regField, result);
         else
            return WriteRM8(operand, result, error);
         return true;
      }

      case 0x01: case 0x03:
      case 0x09: case 0x0b:
      case 0x11: case 0x13:
      case 0x19: case 0x1b:
      case 0x21: case 0x23:
      case 0x29: case 0x2b:
      case 0x31: case 0x33:
      case 0x39: case 0x3b:
      {
         uint8_t modrm;
         sDecodedRM operand;
         unsigned regField;
         uint32_t memoryValue;
         if (!Fetch8(&modrm, error) ||
             !DecodeRM(modrm, &operand, &regField, error) ||
             !ReadRM32(operand, &memoryValue, error))
            return false;
         const bool registerDestination = (opcode & 2) != 0;
         const uint32_t left = registerDestination
            ? m_state.reg[regField] : memoryValue;
         const uint32_t right = registerDestination
            ? memoryValue : m_state.reg[regField];
         uint32_t result = 0;
         const bool carry = (m_state.eflags & kFlagCarry) != 0;
         switch (opcode & 0xfc)
         {
            case 0x00: result = left + right; SetAddFlags(left, right, result); break;
            case 0x08: result = left | right; SetLogicFlags(result); break;
            case 0x10:
               result = left + right + carry;
               SetAdcFlags(left, right, carry, result);
               break;
            case 0x18:
               result = left - right - carry;
               SetSbbFlags(left, right, carry, result);
               break;
            case 0x20: result = left & right; SetLogicFlags(result); break;
            case 0x28: result = left - right; SetSubFlags(left, right, result); break;
            case 0x30: result = left ^ right; SetLogicFlags(result); break;
            case 0x38: result = left - right; SetSubFlags(left, right, result); return true;
         }
         if (registerDestination)
            m_state.reg[regField] = result;
         else
            return WriteRM32(operand, result, error);
         return true;
      }

      case 0x04: case 0x0c: case 0x14: case 0x1c:
      case 0x24: case 0x2c: case 0x34: case 0x3c:
      {
         uint8_t immediate;
         if (!Fetch8(&immediate, error))
            return false;
         const uint8_t left = ReadReg8(0);
         uint8_t result;
         const bool carry = (m_state.eflags & kFlagCarry) != 0;
         switch (opcode)
         {
            case 0x04: result = left + immediate; SetAddFlags8(left, immediate, result); break;
            case 0x0c: result = left | immediate; SetLogicFlags8(result); break;
            case 0x14:
               result = left + immediate + carry;
               SetAdcFlags8(left, immediate, carry, result);
               break;
            case 0x1c:
               result = left - immediate - carry;
               SetSbbFlags8(left, immediate, carry, result);
               break;
            case 0x24: result = left & immediate; SetLogicFlags8(result); break;
            case 0x2c: result = left - immediate; SetSubFlags8(left, immediate, result); break;
            case 0x34: result = left ^ immediate; SetLogicFlags8(result); break;
            default: result = left - immediate; SetSubFlags8(left, immediate, result); return true;
         }
         WriteReg8(0, result);
         return true;
      }

      case 0x05: case 0x0d: case 0x25: case 0x2d: case 0x35: case 0x3d:
      {
         uint32_t immediate;
         if (!Fetch32(&immediate, error))
            return false;
         const uint32_t left = m_state.reg[kEax];
         uint32_t result;
         switch (opcode)
         {
            case 0x05: result = left + immediate; SetAddFlags(left, immediate, result); break;
            case 0x0d: result = left | immediate; SetLogicFlags(result); break;
            case 0x25: result = left & immediate; SetLogicFlags(result); break;
            case 0x2d: result = left - immediate; SetSubFlags(left, immediate, result); break;
            case 0x35: result = left ^ immediate; SetLogicFlags(result); break;
            default: result = left - immediate; SetSubFlags(left, immediate, result); return true;
         }
         m_state.reg[kEax] = result;
         return true;
      }

      case 0x64:
      {
         // VC9's DLL startup reads the thread environment block through an
         // FS-segment MOV (notably fs:[18h], the TEB self pointer).  The
         // emulator exposes the small TEB image in its proxy page.
         const uint32_t kGuestThreadEnvironmentBlock = 0x60000a00;
         uint8_t nestedOpcode;
         if (!Fetch8(&nestedOpcode, error))
            return false;
         if (nestedOpcode == 0x8b)
         {
            uint8_t modrm;
            sDecodedRM operand;
            unsigned regField;
            uint32_t value;
            if (!Fetch8(&modrm, error) ||
                !DecodeRM(modrm, &operand, &regField, error))
               return false;
            if (!operand.isRegister)
               operand.address += kGuestThreadEnvironmentBlock;
            if (!ReadRM32(operand, &value, error))
               return false;
            m_state.reg[regField] = value;
            return true;
         }
         if (nestedOpcode == 0x89)
         {
            uint8_t modrm;
            sDecodedRM operand;
            unsigned regField;
            if (!Fetch8(&modrm, error) ||
                !DecodeRM(modrm, &operand, &regField, error))
               return false;
            if (!operand.isRegister)
               operand.address += kGuestThreadEnvironmentBlock;
            return WriteRM32(operand, m_state.reg[regField], error);
         }
         if (nestedOpcode == 0xff)
         {
            uint8_t modrm;
            sDecodedRM operand;
            unsigned operation;
            uint32_t value;
            if (!Fetch8(&modrm, error) ||
                !DecodeRM(modrm, &operand, &operation, error))
               return false;
            if (operation != 6)
               return Fail(error, "unsupported IA-32 FS FF operation");
            if (!operand.isRegister)
               operand.address += kGuestThreadEnvironmentBlock;
            return ReadRM32(operand, &value, error) && Push32(value, error);
         }
         if (nestedOpcode == 0xa1 || nestedOpcode == 0xa3)
         {
            uint32_t offset;
            if (!Fetch32(&offset, error))
               return false;
            const uint32_t address = kGuestThreadEnvironmentBlock + offset;
            if (nestedOpcode == 0xa1)
               return m_memory.Read32(address, &m_state.reg[kEax], error);
            return m_memory.Write32(address, m_state.reg[kEax], error);
         }
         return FailOpcode(error, "unsupported IA-32 FS opcode",
                           instructionAddress, nestedOpcode);
      }

      case 0x68:
      {
         uint32_t value;
         return Fetch32(&value, error) && Push32(value, error);
      }

      case 0x6a:
      {
         uint8_t value;
         return Fetch8(&value, error) &&
                Push32(static_cast<uint32_t>(Sign8(value)), error);
      }

      case 0x69: case 0x6b:
      {
         uint8_t modrm;
         sDecodedRM operand;
         unsigned regField;
         uint32_t source, immediate;
         if (!Fetch8(&modrm, error) ||
             !DecodeRM(modrm, &operand, &regField, error) ||
             !ReadRM32(operand, &source, error))
            return false;
         if (opcode == 0x69)
         {
            if (!Fetch32(&immediate, error)) return false;
         }
         else
         {
            uint8_t value;
            if (!Fetch8(&value, error)) return false;
            immediate = static_cast<uint32_t>(Sign8(value));
         }
         const int64_t product = static_cast<int64_t>(static_cast<int32_t>(source)) *
                                 static_cast<int32_t>(immediate);
         m_state.reg[regField] = static_cast<uint32_t>(product);
         m_state.eflags &= ~(kFlagCarry | kFlagOverflow);
         if (product != static_cast<int32_t>(product))
            m_state.eflags |= kFlagCarry | kFlagOverflow;
         return true;
      }

      case 0x80:
      {
         uint8_t modrm;
         sDecodedRM operand;
         unsigned operation;
         uint8_t left, immediate;
         if (!Fetch8(&modrm, error) ||
             !DecodeRM(modrm, &operand, &operation, error) ||
             !ReadRM8(operand, &left, error) ||
             !Fetch8(&immediate, error))
            return false;
         uint8_t result;
         const bool carry = (m_state.eflags & kFlagCarry) != 0;
         switch (operation)
         {
            case 0: result = left + immediate; SetAddFlags8(left, immediate, result); break;
            case 1: result = left | immediate; SetLogicFlags8(result); break;
            case 2:
               result = left + immediate + carry;
               SetAdcFlags8(left, immediate, carry, result);
               break;
            case 3:
               result = left - immediate - carry;
               SetSbbFlags8(left, immediate, carry, result);
               break;
            case 4: result = left & immediate; SetLogicFlags8(result); break;
            case 5: result = left - immediate; SetSubFlags8(left, immediate, result); break;
            case 6: result = left ^ immediate; SetLogicFlags8(result); break;
            case 7: result = left - immediate; SetSubFlags8(left, immediate, result); return true;
            default: return Fail(error, "unsupported IA-32 byte immediate arithmetic operation");
         }
         return WriteRM8(operand, result, error);
      }

      case 0x81: case 0x83:
      {
         uint8_t modrm;
         sDecodedRM operand;
         unsigned operation;
         uint32_t left, immediate;
         if (!Fetch8(&modrm, error) ||
             !DecodeRM(modrm, &operand, &operation, error) ||
             !ReadRM32(operand, &left, error))
            return false;
         if (opcode == 0x81)
         {
            if (!Fetch32(&immediate, error)) return false;
         }
         else
         {
            uint8_t value;
            if (!Fetch8(&value, error)) return false;
            immediate = static_cast<uint32_t>(Sign8(value));
         }
         uint32_t result;
         const bool carry = (m_state.eflags & kFlagCarry) != 0;
         switch (operation)
         {
            case 0: result = left + immediate; SetAddFlags(left, immediate, result); break;
            case 1: result = left | immediate; SetLogicFlags(result); break;
            case 2:
               result = left + immediate + carry;
               SetAdcFlags(left, immediate, carry, result);
               break;
            case 3:
               result = left - immediate - carry;
               SetSbbFlags(left, immediate, carry, result);
               break;
            case 4: result = left & immediate; SetLogicFlags(result); break;
            case 5: result = left - immediate; SetSubFlags(left, immediate, result); break;
            case 6: result = left ^ immediate; SetLogicFlags(result); break;
            case 7: result = left - immediate; SetSubFlags(left, immediate, result); return true;
            default: return Fail(error, "unsupported IA-32 immediate arithmetic operation");
         }
         return WriteRM32(operand, result, error);
      }

      case 0x84: case 0x85:
      {
         uint8_t modrm;
         sDecodedRM operand;
         unsigned regField;
         if (!Fetch8(&modrm, error) ||
             !DecodeRM(modrm, &operand, &regField, error))
            return false;
         if (opcode == 0x84)
         {
            uint8_t value;
            if (!ReadRM8(operand, &value, error)) return false;
            SetLogicFlags8(value & ReadReg8(regField));
         }
         else
         {
            uint32_t value;
            if (!ReadRM32(operand, &value, error)) return false;
            SetLogicFlags(value & m_state.reg[regField]);
         }
         return true;
      }

      case 0x86: case 0x87:
      {
         uint8_t modrm;
         sDecodedRM operand;
         unsigned regField;
         if (!Fetch8(&modrm, error) ||
             !DecodeRM(modrm, &operand, &regField, error))
            return false;
         if (opcode == 0x86)
         {
            uint8_t value;
            if (!ReadRM8(operand, &value, error) ||
                !WriteRM8(operand, ReadReg8(regField), error))
               return false;
            WriteReg8(regField, value);
            return true;
         }
         uint32_t value;
         if (!ReadRM32(operand, &value, error) ||
             !WriteRM32(operand, m_state.reg[regField], error))
            return false;
         m_state.reg[regField] = value;
         return true;
      }

      case 0x88: case 0x8a:
      {
         uint8_t modrm;
         sDecodedRM operand;
         unsigned regField;
         if (!Fetch8(&modrm, error) ||
             !DecodeRM(modrm, &operand, &regField, error))
            return false;
         if (opcode == 0x88)
            return WriteRM8(operand, ReadReg8(regField), error);
         uint8_t value;
         if (!ReadRM8(operand, &value, error))
            return false;
         WriteReg8(regField, value);
         return true;
      }

      case 0x89: case 0x8b:
      {
         uint8_t modrm;
         sDecodedRM operand;
         unsigned regField;
         if (!Fetch8(&modrm, error) ||
             !DecodeRM(modrm, &operand, &regField, error))
            return false;
         if (opcode == 0x89)
            return WriteRM32(operand, m_state.reg[regField], error);
         return ReadRM32(operand, &m_state.reg[regField], error);
      }

      case 0x8d:
      {
         uint8_t modrm;
         sDecodedRM operand;
         unsigned regField;
         if (!Fetch8(&modrm, error) ||
             !DecodeRM(modrm, &operand, &regField, error))
            return false;
         if (operand.isRegister)
            return Fail(error, "LEA requires a memory operand");
         m_state.reg[regField] = operand.address;
         return true;
      }

      case 0x90:
         return true;

      case 0x99: // CDQ
         m_state.reg[kEdx] = (m_state.reg[kEax] & 0x80000000u) ?
                             0xffffffffu : 0;
         return true;

      case 0x9b: // WAIT/FWAIT; floating-point execution is synchronous here.
         return true;

      case 0x9c: // PUSHFD
         return Push32(m_state.eflags, error);

      case 0x9d: // POPFD
         if (!Pop32(&m_state.eflags, error))
            return false;
         m_state.eflags |= 0x00000002u;
         return true;

      case 0xa9:
      {
         uint32_t immediate;
         if (!Fetch32(&immediate, error))
            return false;
         SetLogicFlags(m_state.reg[kEax] & immediate);
         return true;
      }

      case 0xa8:
      {
         uint8_t immediate;
         if (!Fetch8(&immediate, error))
            return false;
         SetLogicFlags8(ReadReg8(0) & immediate);
         return true;
      }

      case 0xa0:
      {
         uint32_t address;
         uint8_t value;
         if (!Fetch32(&address, error) ||
             !m_memory.Read8(address, &value, error))
            return false;
         WriteReg8(0, value);
         return true;
      }

      case 0xa1:
      {
         uint32_t address;
         return Fetch32(&address, error) &&
                m_memory.Read32(address, &m_state.reg[kEax], error);
      }

      case 0xa3:
      {
         uint32_t address;
         return Fetch32(&address, error) &&
                m_memory.Write32(address, m_state.reg[kEax], error);
      }

      case 0xa2:
      {
         uint32_t address;
         return Fetch32(&address, error) &&
                m_memory.Write8(address, ReadReg8(0), error);
      }

      case 0xc2:
      {
         uint16_t bytes;
         uint32_t target;
         if (!Fetch16(&bytes, error) || !Pop32(&target, error))
            return false;
         m_state.reg[kEsp] += bytes;
         m_state.eip = target;
         return true;
      }

      case 0xc1: case 0xd1: case 0xd3:
      {
         uint8_t modrm;
         sDecodedRM operand;
         unsigned operation;
         uint32_t value;
         if (!Fetch8(&modrm, error) ||
             !DecodeRM(modrm, &operand, &operation, error) ||
             !ReadRM32(operand, &value, error))
            return false;

         uint8_t rawCount = 1;
         if (opcode == 0xc1 && !Fetch8(&rawCount, error))
            return false;
         if (opcode == 0xd3)
            rawCount = ReadReg8(1);
         const unsigned count = rawCount & 31;
         if (!count)
            return true;

         uint32_t result;
         bool carry;
         switch (operation)
         {
            case 0: // ROL
               result = (value << count) | (value >> (32 - count));
               carry = (result & 1) != 0;
               m_state.eflags &= ~(kFlagCarry | kFlagOverflow);
               if (carry) m_state.eflags |= kFlagCarry;
               if (count == 1 && (((result >> 31) & 1) !=
                                  static_cast<unsigned>(carry)))
                  m_state.eflags |= kFlagOverflow;
               break;
            case 1: // ROR
               result = (value >> count) | (value << (32 - count));
               carry = (result & 0x80000000u) != 0;
               m_state.eflags &= ~(kFlagCarry | kFlagOverflow);
               if (carry) m_state.eflags |= kFlagCarry;
               if (count == 1 && (((result >> 31) ^ (result >> 30)) & 1))
                  m_state.eflags |= kFlagOverflow;
               break;
            case 4: case 6: // SHL/SAL
               carry = ((value >> (32 - count)) & 1) != 0;
               result = value << count;
               SetLogicFlags(result);
               if (carry) m_state.eflags |= kFlagCarry;
               if (count == 1 && (((result >> 31) & 1) !=
                                  static_cast<unsigned>(carry)))
                  m_state.eflags |= kFlagOverflow;
               break;
            case 5: // SHR
               carry = ((value >> (count - 1)) & 1) != 0;
               result = value >> count;
               SetLogicFlags(result);
               if (carry) m_state.eflags |= kFlagCarry;
               if (count == 1 && (value & 0x80000000u))
                  m_state.eflags |= kFlagOverflow;
               break;
            case 7: // SAR
               carry = ((value >> (count - 1)) & 1) != 0;
               result = static_cast<uint32_t>(static_cast<int32_t>(value) >> count);
               SetLogicFlags(result);
               if (carry) m_state.eflags |= kFlagCarry;
               break;
            default:
               return Fail(error, "unsupported IA-32 shift operation");
         }
         return WriteRM32(operand, result, error);
      }

      case 0xc0: case 0xd0: case 0xd2:
      {
         uint8_t modrm;
         sDecodedRM operand;
         unsigned operation;
         uint8_t value;
         if (!Fetch8(&modrm, error) ||
             !DecodeRM(modrm, &operand, &operation, error) ||
             !ReadRM8(operand, &value, error))
            return false;

         uint8_t rawCount = 1;
         if (opcode == 0xc0 && !Fetch8(&rawCount, error)) return false;
         if (opcode == 0xd2) rawCount = ReadReg8(1);
         unsigned count = rawCount & 31;
         if (!count) return true;

         uint8_t result = value;
         bool carry = false;
         switch (operation)
         {
            case 0: // ROL
               count %= 8;
               if (!count) return true;
               result = static_cast<uint8_t>((value << count) |
                                             (value >> (8 - count)));
               carry = (result & 1) != 0;
               m_state.eflags &= ~(kFlagCarry | kFlagOverflow);
               if (carry) m_state.eflags |= kFlagCarry;
               if (count == 1 && (((result >> 7) & 1) !=
                                  static_cast<unsigned>(carry)))
                  m_state.eflags |= kFlagOverflow;
               break;
            case 1: // ROR
               count %= 8;
               if (!count) return true;
               result = static_cast<uint8_t>((value >> count) |
                                             (value << (8 - count)));
               carry = (result & 0x80) != 0;
               m_state.eflags &= ~(kFlagCarry | kFlagOverflow);
               if (carry) m_state.eflags |= kFlagCarry;
               if (count == 1 && (((result >> 7) ^ (result >> 6)) & 1))
                  m_state.eflags |= kFlagOverflow;
               break;
            case 4: case 6: // SHL/SAL
               carry = count <= 8 && ((value >> (8 - count)) & 1) != 0;
               result = count < 8 ? static_cast<uint8_t>(value << count) : 0;
               SetLogicFlags8(result);
               if (carry) m_state.eflags |= kFlagCarry;
               if (count == 1 && (((result >> 7) & 1) !=
                                  static_cast<unsigned>(carry)))
                  m_state.eflags |= kFlagOverflow;
               break;
            case 5: // SHR
               carry = count <= 8 && ((value >> (count - 1)) & 1) != 0;
               result = count < 8 ? static_cast<uint8_t>(value >> count) : 0;
               SetLogicFlags8(result);
               if (carry) m_state.eflags |= kFlagCarry;
               if (count == 1 && (value & 0x80))
                  m_state.eflags |= kFlagOverflow;
               break;
            case 7: // SAR
               carry = count <= 8 && ((value >> (count - 1)) & 1) != 0;
               result = count < 8 ? static_cast<uint8_t>(
                  static_cast<int8_t>(value) >> count) :
                  static_cast<uint8_t>((value & 0x80) ? 0xff : 0);
               SetLogicFlags8(result);
               if (carry) m_state.eflags |= kFlagCarry;
               break;
            default:
               return Fail(error, "unsupported IA-32 byte shift operation");
         }
         return WriteRM8(operand, result, error);
      }

      case 0xc3:
      {
         uint32_t target;
         if (!Pop32(&target, error))
            return false;
         m_state.eip = target;
         return true;
      }

      case 0xc6:
      {
         uint8_t modrm;
         sDecodedRM operand;
         unsigned operation;
         uint8_t value;
         if (!Fetch8(&modrm, error) ||
             !DecodeRM(modrm, &operand, &operation, error) ||
             !Fetch8(&value, error))
            return false;
         if (operation != 0)
            return Fail(error, "invalid byte MOV immediate encoding");
         return WriteRM8(operand, value, error);
      }

      case 0xc7:
      {
         uint8_t modrm;
         sDecodedRM operand;
         unsigned operation;
         uint32_t value;
         if (!Fetch8(&modrm, error) ||
             !DecodeRM(modrm, &operand, &operation, error) ||
             !Fetch32(&value, error))
            return false;
         if (operation != 0)
            return Fail(error, "invalid MOV immediate encoding");
         return WriteRM32(operand, value, error);
      }

      case 0xc9:
      {
         m_state.reg[kEsp] = m_state.reg[kEbp];
         return Pop32(&m_state.reg[kEbp], error);
      }

      case 0xd8:
      {
         uint8_t modrm;
         sDecodedRM operand;
         unsigned operation;
         if (!Fetch8(&modrm, error) ||
             !DecodeRM(modrm, &operand, &operation, error))
            return false;
         if (!m_state.x87Depth)
            return Fail(error, "x87 stack underflow");

         double value;
         if (operand.isRegister)
         {
            const unsigned index = modrm & 7;
            if (index >= m_state.x87Depth)
               return Fail(error, "x87 stack register is empty");
            value = m_state.x87[index];
         }
         else
         {
            uint32_t bits;
            float single;
            if (!m_memory.Read32(operand.address, &bits, error))
               return false;
            memcpy(&single, &bits, sizeof(single));
            value = single;
         }

         if (operation == 2 || operation == 3) // FCOM/FCOMP
         {
            const double top = m_state.x87[0];
            m_state.x87Status &= ~0x4500u;
            if (std::isnan(top) || std::isnan(value))
               m_state.x87Status |= 0x4500u;
            else if (top < value)
               m_state.x87Status |= 0x0100u;
            else if (top == value)
               m_state.x87Status |= 0x4000u;
            if (operation == 3)
            {
               for (unsigned i = 1; i < m_state.x87Depth; ++i)
                  m_state.x87[i - 1] = m_state.x87[i];
               --m_state.x87Depth;
            }
            return true;
         }

         switch (operation)
         {
            case 0: m_state.x87[0] += value; break;
            case 1: m_state.x87[0] *= value; break;
            case 4: m_state.x87[0] -= value; break;
            case 5: m_state.x87[0] = value - m_state.x87[0]; break;
            case 6: m_state.x87[0] /= value; break;
            case 7: m_state.x87[0] = value / m_state.x87[0]; break;
         }
         return true;
      }

      case 0xd7: // XLAT byte ptr [EBX + AL]
      {
         uint8_t value;
         const uint32_t address = m_state.reg[kEbx] +
            (m_state.reg[kEax] & 0xffu);
         if (!m_memory.Read8(address, &value, error))
            return false;
         WriteReg8(0, value);
         return true;
      }

      case 0xd9:
      {
         uint8_t second;
         if (!Fetch8(&second, error))
            return false;
         if ((second & 0xf8u) == 0xc0u) // FLD ST(i)
         {
            const unsigned index = second & 7u;
            if (index >= m_state.x87Depth)
               return Fail(error, "x87 stack register is empty");
            if (m_state.x87Depth == 8)
               return Fail(error, "x87 stack overflow");
            const double value = m_state.x87[index];
            for (unsigned i = m_state.x87Depth; i != 0; --i)
               m_state.x87[i] = m_state.x87[i - 1];
            m_state.x87[0] = value;
            ++m_state.x87Depth;
            return true;
         }
         if ((second & 0xf8u) == 0xc8u) // FXCH ST(i)
         {
            const unsigned index = second & 7u;
            if (index >= m_state.x87Depth)
               return Fail(error, "x87 stack register is empty");
            std::swap(m_state.x87[0], m_state.x87[index]);
            return true;
         }
         if (second == 0xe0) // FCHS
         {
            if (!m_state.x87Depth) return Fail(error, "x87 stack underflow");
            m_state.x87[0] = -m_state.x87[0];
            return true;
         }
         if (second == 0xe1) // FABS
         {
            if (!m_state.x87Depth) return Fail(error, "x87 stack underflow");
            m_state.x87[0] = std::fabs(m_state.x87[0]);
            return true;
         }
         if (second == 0xe4) // FTST
         {
            if (!m_state.x87Depth) return Fail(error, "x87 stack underflow");
            const double value = m_state.x87[0];
            m_state.x87Status &= ~0x4500u;
            if (std::isnan(value))
               m_state.x87Status |= 0x4500u;
            else if (value < 0.0)
               m_state.x87Status |= 0x0100u;
            else if (value == 0.0)
               m_state.x87Status |= 0x4000u;
            return true;
         }
         if (second == 0xe5) // FXAM
         {
            m_state.x87Status &= ~0x4700u;
            if (!m_state.x87Depth)
            {
               m_state.x87Status |= 0x4100u;
               return true;
            }
            const double value = m_state.x87[0];
            if (std::signbit(value))
               m_state.x87Status |= 0x0200u;
            switch (std::fpclassify(value))
            {
               case FP_NAN:       m_state.x87Status |= 0x0100u; break;
               case FP_INFINITE:  m_state.x87Status |= 0x0500u; break;
               case FP_ZERO:      m_state.x87Status |= 0x4000u; break;
               case FP_SUBNORMAL: m_state.x87Status |= 0x4400u; break;
               default:           m_state.x87Status |= 0x0400u; break;
            }
            return true;
         }
         if (second == 0xe8)
         {
            if (m_state.x87Depth == 8)
               return Fail(error, "x87 stack overflow");
            for (unsigned i = m_state.x87Depth; i != 0; --i)
               m_state.x87[i] = m_state.x87[i - 1];
            m_state.x87[0] = 1.0;
            ++m_state.x87Depth;
            return true;
         }
         if (second == 0xee) // FLDZ
         {
            if (m_state.x87Depth == 8)
               return Fail(error, "x87 stack overflow");
            for (unsigned i = m_state.x87Depth; i != 0; --i)
               m_state.x87[i] = m_state.x87[i - 1];
            m_state.x87[0] = 0.0;
            ++m_state.x87Depth;
            return true;
         }
         if (second == 0xfa) // FSQRT
         {
            if (!m_state.x87Depth) return Fail(error, "x87 stack underflow");
            m_state.x87[0] = std::sqrt(m_state.x87[0]);
            return true;
         }
         sDecodedRM operand;
         unsigned operation;
         if (!DecodeRM(second, &operand, &operation, error))
            return false;
         if (operand.isRegister)
            return FailAddress(error, "unsupported IA-32 D9 opcode at",
                               instructionAddress);
         if (operation == 0) // FLD m32real
         {
            if (m_state.x87Depth == 8)
               return Fail(error, "x87 stack overflow");
            uint32_t bits;
            float value;
            if (!m_memory.Read32(operand.address, &bits, error))
               return false;
            memcpy(&value, &bits, sizeof(value));
            for (unsigned i = m_state.x87Depth; i != 0; --i)
               m_state.x87[i] = m_state.x87[i - 1];
            m_state.x87[0] = value;
            ++m_state.x87Depth;
            return true;
         }
         if (operation == 2 || operation == 3) // FST/FSTP m32real
         {
            if (!m_state.x87Depth)
               return Fail(error, "x87 stack underflow");
            const float value = static_cast<float>(m_state.x87[0]);
            uint32_t bits;
            memcpy(&bits, &value, sizeof(bits));
            if (!m_memory.Write32(operand.address, bits, error))
               return false;
            if (operation == 3)
            {
               for (unsigned i = 1; i < m_state.x87Depth; ++i)
                  m_state.x87[i - 1] = m_state.x87[i];
               --m_state.x87Depth;
            }
            return true;
         }
         if (operation == 5)
         {
            uint16_t value;
            if (!m_memory.Read16(operand.address, &value, error))
               return false;
            m_state.x87Control = value;
            return true;
         }
         if (operation == 7)
            return m_memory.Write16(operand.address, m_state.x87Control, error);
         return FailAddress(error, "unsupported IA-32 D9 memory operation at",
                            instructionAddress);
      }

      case 0xda:
      {
         uint8_t modrm;
         sDecodedRM operand;
         unsigned operation;
         if (!Fetch8(&modrm, error) ||
             !DecodeRM(modrm, &operand, &operation, error))
            return false;
         if (operand.isRegister)
            return FailAddress(error, "unsupported IA-32 DA opcode at",
                               instructionAddress);
         if (!m_state.x87Depth)
            return Fail(error, "x87 stack underflow");
         uint32_t bits;
         if (!m_memory.Read32(operand.address, &bits, error))
            return false;
         const double value = static_cast<int32_t>(bits);
         if (operation == 2 || operation == 3) // FICOM/FICOMP m32int
         {
            const double top = m_state.x87[0];
            m_state.x87Status &= ~0x4500u;
            if (std::isnan(top))
               m_state.x87Status |= 0x4500u;
            else if (top < value)
               m_state.x87Status |= 0x0100u;
            else if (top == value)
               m_state.x87Status |= 0x4000u;
            if (operation == 3)
            {
               for (unsigned i = 1; i < m_state.x87Depth; ++i)
                  m_state.x87[i - 1] = m_state.x87[i];
               --m_state.x87Depth;
            }
            return true;
         }
         switch (operation)
         {
            case 0: m_state.x87[0] += value; break;
            case 1: m_state.x87[0] *= value; break;
            case 4: m_state.x87[0] -= value; break;
            case 5: m_state.x87[0] = value - m_state.x87[0]; break;
            case 6: m_state.x87[0] /= value; break;
            case 7: m_state.x87[0] = value / m_state.x87[0]; break;
         }
         return true;
      }

      case 0xdb:
      {
         uint8_t second;
         if (!Fetch8(&second, error))
            return false;
         if (second == 0xe2) // FNCLEX
         {
            m_state.x87Status &= 0x7f00u;
            return true;
         }
         if (second == 0xe3) // FNINIT
         {
            m_state.x87Depth = 0;
            m_state.x87Status = 0;
            m_state.x87Control = 0x037f;
            return true;
         }
         sDecodedRM operand;
         unsigned operation;
         if (!DecodeRM(second, &operand, &operation, error))
            return false;
         if (!operand.isRegister && operation == 0) // FILD m32int
         {
            if (m_state.x87Depth == 8)
               return Fail(error, "x87 stack overflow");
            uint32_t value;
            if (!m_memory.Read32(operand.address, &value, error))
               return false;
            for (unsigned i = m_state.x87Depth; i != 0; --i)
               m_state.x87[i] = m_state.x87[i - 1];
            m_state.x87[0] = static_cast<int32_t>(value);
            ++m_state.x87Depth;
            return true;
         }
         if (!operand.isRegister && (operation == 2 || operation == 3))
         {
            // FIST/FISTP m32int.  MSVC's default x87 control word rounds to
            // nearest; the unusual modes are retained in bits 10-11.
            if (!m_state.x87Depth)
               return Fail(error, "x87 stack underflow");
            double rounded;
            switch ((m_state.x87Control >> 10) & 3)
            {
               case 1: rounded = std::floor(m_state.x87[0]); break;
               case 2: rounded = std::ceil(m_state.x87[0]); break;
               case 3: rounded = std::trunc(m_state.x87[0]); break;
               default: rounded = std::nearbyint(m_state.x87[0]); break;
            }
            const int32_t value =
               rounded < -2147483648.0 || rounded > 2147483647.0 ||
               std::isnan(rounded) ? static_cast<int32_t>(0x80000000u) :
               static_cast<int32_t>(rounded);
            if (!m_memory.Write32(operand.address,
                                  static_cast<uint32_t>(value), error))
               return false;
            if (operation == 3)
            {
               for (unsigned i = 1; i < m_state.x87Depth; ++i)
                  m_state.x87[i - 1] = m_state.x87[i];
               --m_state.x87Depth;
            }
            return true;
         }
         return FailAddress(error, "unsupported IA-32 DB opcode at",
                            instructionAddress);
      }

      case 0xdc:
      {
         uint8_t modrm;
         sDecodedRM operand;
         unsigned operation;
         uint64_t bits;
         double value;
         if (!Fetch8(&modrm, error) ||
             !DecodeRM(modrm, &operand, &operation, error))
            return false;
         if (operand.isRegister)
         {
            const unsigned index = modrm & 7;
            if (index >= m_state.x87Depth)
               return Fail(error, "x87 stack register is empty");
            const double top = m_state.x87[0];
            switch (operation)
            {
               case 0: m_state.x87[index] += top; break;
               case 1: m_state.x87[index] *= top; break;
               case 4: m_state.x87[index] = top - m_state.x87[index]; break;
               case 5: m_state.x87[index] -= top; break;
               case 6: m_state.x87[index] = top / m_state.x87[index]; break;
               case 7: m_state.x87[index] /= top; break;
               default:
                  return Fail(error, "unsupported x87 register comparison");
            }
            return true;
         }
         if (!m_state.x87Depth)
            return Fail(error, "x87 stack underflow");
         if (!m_memory.Read64(operand.address, &bits, error))
            return false;
         memcpy(&value, &bits, sizeof(value));
         switch (operation)
         {
            case 0: m_state.x87[0] += value; break;
            case 1: m_state.x87[0] *= value; break;
            case 2: case 3:
            {
               const double top = m_state.x87[0];
               m_state.x87Status &= ~0x4500u;
               if (std::isnan(top) || std::isnan(value))
                  m_state.x87Status |= 0x4500u;
               else if (top < value)
                  m_state.x87Status |= 0x0100u;
               else if (top == value)
                  m_state.x87Status |= 0x4000u;
               if (operation == 3)
               {
                  for (unsigned i = 1; i < m_state.x87Depth; ++i)
                     m_state.x87[i - 1] = m_state.x87[i];
                  --m_state.x87Depth;
               }
               break;
            }
            case 4: m_state.x87[0] -= value; break;
            case 5: m_state.x87[0] = value - m_state.x87[0]; break;
            case 6: m_state.x87[0] /= value; break;
            case 7: m_state.x87[0] = value / m_state.x87[0]; break;
         }
         return true;
      }

      case 0xdd:
      {
         uint8_t modrm;
         sDecodedRM operand;
         unsigned operation;
         if (!Fetch8(&modrm, error) ||
             !DecodeRM(modrm, &operand, &operation, error))
            return false;
         if (operand.isRegister)
            return Fail(error, "unsupported x87 register operation");
         if (operation == 0)
         {
            uint64_t bits;
            double value;
            if (m_state.x87Depth == 8)
               return Fail(error, "x87 stack overflow");
            if (!m_memory.Read64(operand.address, &bits, error))
               return false;
            memcpy(&value, &bits, sizeof(value));
            for (unsigned i = m_state.x87Depth; i != 0; --i)
               m_state.x87[i] = m_state.x87[i - 1];
            m_state.x87[0] = value;
            ++m_state.x87Depth;
            return true;
         }
         if (operation == 2 || operation == 3)
         {
            uint64_t bits;
            if (!m_state.x87Depth)
               return Fail(error, "x87 stack underflow");
            memcpy(&bits, &m_state.x87[0], sizeof(bits));
            if (!m_memory.Write64(operand.address, bits, error))
               return false;
            if (operation == 3)
            {
               for (unsigned i = 1; i < m_state.x87Depth; ++i)
                  m_state.x87[i - 1] = m_state.x87[i];
               --m_state.x87Depth;
            }
            return true;
         }
         if (operation == 7) // FNSTSW m16
            return m_memory.Write16(operand.address, m_state.x87Status,
                                    error);
         return Fail(error, "unsupported IA-32 DD operation");
      }

      case 0xde:
      {
         uint8_t second;
         if (!Fetch8(&second, error)) return false;
         const unsigned operation = second & 0xf8u;
         const unsigned stackIndex = second & 7u;
         if (operation == 0xc0u || operation == 0xc8u ||
             operation == 0xe0u || operation == 0xe8u ||
             operation == 0xf0u || operation == 0xf8u)
         {
            if (!stackIndex || m_state.x87Depth <= stackIndex)
               return Fail(error, "x87 stack underflow");
            const double top = m_state.x87[0];
            const double other = m_state.x87[stackIndex];
            double result;
            switch (operation)
            {
               case 0xc0u: result = other + top; break;
               case 0xc8u: result = other * top; break;
               case 0xe0u: result = top - other; break;
               case 0xe8u: result = other - top; break;
               case 0xf0u: result = top / other; break;
               default:    result = other / top; break;
            }
            m_state.x87[stackIndex] = result;
            for (unsigned i = 1; i < m_state.x87Depth; ++i)
               m_state.x87[i - 1] = m_state.x87[i];
            --m_state.x87Depth;
            return true;
         }
         if (second != 0xd9)
            return FailAddress(error, "unsupported IA-32 DE opcode at",
                               instructionAddress);
         if (m_state.x87Depth < 2)
            return Fail(error, "x87 stack underflow");
         const double left = m_state.x87[0];
         const double right = m_state.x87[1];
         m_state.x87Status &= ~0x4500u;
         if (std::isnan(left) || std::isnan(right))
            m_state.x87Status |= 0x4500u;
         else if (left < right)
            m_state.x87Status |= 0x0100u;
         else if (left == right)
            m_state.x87Status |= 0x4000u;
         for (unsigned i = 2; i < m_state.x87Depth; ++i)
            m_state.x87[i - 2] = m_state.x87[i];
         m_state.x87Depth -= 2;
         return true;
      }

      case 0xdf:
      {
         uint8_t second;
         if (!Fetch8(&second, error))
            return false;
         if (second == 0xe0)
         {
            m_state.reg[kEax] = (m_state.reg[kEax] & 0xffff0000u) |
                                m_state.x87Status;
            return true;
         }
         sDecodedRM operand;
         unsigned operation;
         if (!DecodeRM(second, &operand, &operation, error))
            return false;
         if (operand.isRegister)
            return FailAddress(error, "unsupported IA-32 DF opcode at",
                               instructionAddress);
         if (operation == 0 || operation == 5) // FILD m16int/m64int
         {
            if (m_state.x87Depth == 8)
               return Fail(error, "x87 stack overflow");
            double value;
            if (operation == 0)
            {
               uint16_t bits;
               if (!m_memory.Read16(operand.address, &bits, error))
                  return false;
               value = static_cast<int16_t>(bits);
            }
            else
            {
               uint64_t bits;
               if (!m_memory.Read64(operand.address, &bits, error))
                  return false;
               value = static_cast<double>(static_cast<int64_t>(bits));
            }
            for (unsigned i = m_state.x87Depth; i != 0; --i)
               m_state.x87[i] = m_state.x87[i - 1];
            m_state.x87[0] = value;
            ++m_state.x87Depth;
            return true;
         }
         if (operation == 2 || operation == 3 || operation == 7)
         {
            // FIST/FISTP m16int and FISTP m64int.
            if (!m_state.x87Depth)
               return Fail(error, "x87 stack underflow");
            double rounded;
            switch ((m_state.x87Control >> 10) & 3)
            {
               case 1: rounded = std::floor(m_state.x87[0]); break;
               case 2: rounded = std::ceil(m_state.x87[0]); break;
               case 3: rounded = std::trunc(m_state.x87[0]); break;
               default: rounded = std::nearbyint(m_state.x87[0]); break;
            }
            bool written;
            if (operation == 7)
            {
               const uint64_t value =
                  rounded < -9223372036854775808.0 ||
                  rounded >= 9223372036854775808.0 ||
                  std::isnan(rounded) ? 0x8000000000000000ULL :
                  static_cast<uint64_t>(static_cast<int64_t>(rounded));
               written = m_memory.Write64(operand.address, value, error);
            }
            else
            {
               const uint16_t value =
                  rounded < -32768.0 || rounded > 32767.0 ||
                  std::isnan(rounded) ? 0x8000u :
                  static_cast<uint16_t>(static_cast<int16_t>(rounded));
               written = m_memory.Write16(operand.address, value, error);
            }
            if (!written) return false;
            if (operation == 3 || operation == 7)
            {
               for (unsigned i = 1; i < m_state.x87Depth; ++i)
                  m_state.x87[i - 1] = m_state.x87[i];
               --m_state.x87Depth;
            }
            return true;
         }
         return FailAddress(error, "unsupported IA-32 DF opcode at",
                            instructionAddress);
      }

      case 0xe8:
      {
         uint32_t displacement;
         if (!Fetch32(&displacement, error))
            return false;
         const uint32_t returnAddress = m_state.eip;
         if (!Push32(returnAddress, error))
            return false;
         m_state.eip += displacement;
         return true;
      }

      case 0xe9:
      {
         uint32_t displacement;
         if (!Fetch32(&displacement, error))
            return false;
         m_state.eip += displacement;
         return true;
      }

      case 0xeb:
      {
         uint8_t displacement;
         if (!Fetch8(&displacement, error))
            return false;
         m_state.eip += Sign8(displacement);
         return true;
      }

      case 0xf6:
      {
         uint8_t modrm;
         sDecodedRM operand;
         unsigned operation;
         uint8_t value;
         if (!Fetch8(&modrm, error) ||
             !DecodeRM(modrm, &operand, &operation, error) ||
             !ReadRM8(operand, &value, error))
            return false;
         if (operation == 0)
         {
            uint8_t immediate;
            if (!Fetch8(&immediate, error)) return false;
            SetLogicFlags8(value & immediate);
            return true;
         }
         if (operation == 2)
            return WriteRM8(operand, static_cast<uint8_t>(~value), error);
         if (operation == 3)
         {
            const uint8_t result = static_cast<uint8_t>(0 - value);
            SetSubFlags8(0, value, result);
            return WriteRM8(operand, result, error);
         }
         return Fail(error, "unsupported IA-32 F6 operation");
      }

      case 0xf7:
      {
         uint8_t modrm;
         sDecodedRM operand;
         unsigned operation;
         uint32_t value;
         if (!Fetch8(&modrm, error) ||
             !DecodeRM(modrm, &operand, &operation, error) ||
             !ReadRM32(operand, &value, error))
            return false;
         if (operation == 0)
         {
            uint32_t immediate;
            if (!Fetch32(&immediate, error)) return false;
            SetLogicFlags(value & immediate);
            return true;
         }
         if (operation == 2)
            return WriteRM32(operand, ~value, error);
         if (operation == 3)
         {
            const uint32_t result = 0 - value;
            SetSubFlags(0, value, result);
            return WriteRM32(operand, result, error);
         }
         if (operation == 4)
         {
            const uint64_t product =
               static_cast<uint64_t>(m_state.reg[kEax]) * value;
            m_state.reg[kEax] = static_cast<uint32_t>(product);
            m_state.reg[kEdx] = static_cast<uint32_t>(product >> 32);
            m_state.eflags &= ~(kFlagCarry | kFlagOverflow);
            if (m_state.reg[kEdx])
               m_state.eflags |= kFlagCarry | kFlagOverflow;
            return true;
         }
         if (operation == 5)
         {
            const int64_t product =
               static_cast<int64_t>(static_cast<int32_t>(m_state.reg[kEax])) *
               static_cast<int32_t>(value);
            const uint64_t bits = static_cast<uint64_t>(product);
            m_state.reg[kEax] = static_cast<uint32_t>(bits);
            m_state.reg[kEdx] = static_cast<uint32_t>(bits >> 32);
            m_state.eflags &= ~(kFlagCarry | kFlagOverflow);
            if (product != static_cast<int32_t>(product))
               m_state.eflags |= kFlagCarry | kFlagOverflow;
            return true;
         }
         if (operation == 6)
         {
            if (!value)
               return Fail(error, "IA-32 unsigned division by zero");
            const uint64_t dividend =
               (static_cast<uint64_t>(m_state.reg[kEdx]) << 32) |
               m_state.reg[kEax];
            const uint64_t quotient = dividend / value;
            if (quotient > 0xffffffffull)
               return Fail(error, "IA-32 unsigned division overflow");
            m_state.reg[kEax] = static_cast<uint32_t>(quotient);
            m_state.reg[kEdx] = static_cast<uint32_t>(dividend % value);
            return true;
         }
         if (operation == 7)
         {
            const int32_t divisor = static_cast<int32_t>(value);
            if (!divisor)
               return Fail(error, "IA-32 signed division by zero");
            const uint64_t dividendBits =
               (static_cast<uint64_t>(m_state.reg[kEdx]) << 32) |
               m_state.reg[kEax];
            int64_t dividend;
            memcpy(&dividend, &dividendBits, sizeof(dividend));
            if (dividend == (std::numeric_limits<int64_t>::min)() &&
                divisor == -1)
               return Fail(error, "IA-32 signed division overflow");
            const int64_t quotient = dividend / divisor;
            if (quotient < (std::numeric_limits<int32_t>::min)() ||
                quotient > (std::numeric_limits<int32_t>::max)())
               return Fail(error, "IA-32 signed division overflow");
            m_state.reg[kEax] = static_cast<uint32_t>(quotient);
            m_state.reg[kEdx] = static_cast<uint32_t>(dividend % divisor);
            return true;
         }
         return Fail(error, "unsupported IA-32 F7 operation");
      }

      case 0xfc:
         m_state.eflags &= ~kFlagDirection;
         return true;

      case 0xfd:
         m_state.eflags |= kFlagDirection;
         return true;

      case 0xfe:
      {
         uint8_t modrm;
         sDecodedRM operand;
         unsigned operation;
         uint8_t value;
         if (!Fetch8(&modrm, error) ||
             !DecodeRM(modrm, &operand, &operation, error) ||
             !ReadRM8(operand, &value, error))
            return false;
         const uint32_t carry = m_state.eflags & kFlagCarry;
         if (operation == 0)
         {
            const uint8_t result = static_cast<uint8_t>(value + 1);
            SetAddFlags8(value, 1, result);
            m_state.eflags = (m_state.eflags & ~kFlagCarry) | carry;
            return WriteRM8(operand, result, error);
         }
         if (operation == 1)
         {
            const uint8_t result = static_cast<uint8_t>(value - 1);
            SetSubFlags8(value, 1, result);
            m_state.eflags = (m_state.eflags & ~kFlagCarry) | carry;
            return WriteRM8(operand, result, error);
         }
         return Fail(error, "unsupported IA-32 FE operation");
      }

      case 0xff:
      {
         uint8_t modrm;
         sDecodedRM operand;
         unsigned operation;
         uint32_t value;
         if (!Fetch8(&modrm, error) ||
             !DecodeRM(modrm, &operand, &operation, error) ||
             !ReadRM32(operand, &value, error))
            return false;
         switch (operation)
         {
            case 0:
            {
               const uint32_t result = value + 1;
               SetIncFlags(value, result);
               return WriteRM32(operand, result, error);
            }
            case 1:
            {
               const uint32_t result = value - 1;
               SetDecFlags(value, result);
               return WriteRM32(operand, result, error);
            }
            case 2:
            {
               const uint32_t returnAddress = m_state.eip;
               if (!Push32(returnAddress, error)) return false;
               m_state.eip = value;
               return true;
            }
            case 4: m_state.eip = value; return true;
            case 6: return Push32(value, error);
            default: return Fail(error, "unsupported IA-32 FF operation");
         }
      }

      case 0x0f:
      {
         uint8_t second;
         if (!Fetch8(&second, error))
            return false;
         if (second == 0xa2) // CPUID
         {
            if (m_state.reg[kEax] == 0)
            {
               m_state.reg[kEax] = 1;
               m_state.reg[kEbx] = 0x756e6547u; // "Genu"
               m_state.reg[kEdx] = 0x49656e69u; // "ineI"
               m_state.reg[kEcx] = 0x6c65746eu; // "ntel"
            }
            else if (m_state.reg[kEax] == 1)
            {
               // Report an i686 baseline without optional SIMD extensions;
               // the interpreter implements the scalar CRT path.
               m_state.reg[kEax] = 0x00000600u;
               m_state.reg[kEbx] = 0;
               m_state.reg[kEcx] = 0;
               m_state.reg[kEdx] = 0;
            }
            else
               m_state.reg[kEax] = m_state.reg[kEbx] =
                  m_state.reg[kEcx] = m_state.reg[kEdx] = 0;
            return true;
         }
         if (second >= 0x80 && second <= 0x8f)
         {
            uint32_t displacement;
            if (!Fetch32(&displacement, error)) return false;
            if (Condition(second - 0x80)) m_state.eip += displacement;
            return true;
         }
         if (second >= 0x90 && second <= 0x9f)
         {
            uint8_t modrm;
            sDecodedRM operand;
            unsigned ignored;
            if (!Fetch8(&modrm, error) ||
                !DecodeRM(modrm, &operand, &ignored, error))
               return false;
            return WriteRM8(operand, Condition(second - 0x90) ? 1 : 0,
                            error);
         }
         if (second == 0xaf || second == 0xb6 || second == 0xb7 ||
             second == 0xbe || second == 0xbf)
         {
            uint8_t modrm;
            sDecodedRM operand;
            unsigned regField;
            if (!Fetch8(&modrm, error) ||
                !DecodeRM(modrm, &operand, &regField, error))
               return false;
            if (second == 0xaf)
            {
               uint32_t source;
               if (!ReadRM32(operand, &source, error)) return false;
               const int64_t product =
                  static_cast<int64_t>(static_cast<int32_t>(m_state.reg[regField])) *
                  static_cast<int32_t>(source);
               m_state.reg[regField] = static_cast<uint32_t>(product);
               m_state.eflags &= ~(kFlagCarry | kFlagOverflow);
               if (product != static_cast<int32_t>(product))
                  m_state.eflags |= kFlagCarry | kFlagOverflow;
               return true;
            }
            if (second == 0xb6 || second == 0xbe)
            {
               uint8_t value;
               if (!ReadRM8(operand, &value, error)) return false;
               m_state.reg[regField] = second == 0xbe
                  ? static_cast<uint32_t>(static_cast<int32_t>(static_cast<int8_t>(value)))
                  : value;
               return true;
            }
            uint16_t value;
            if (!ReadRM16(operand, &value, error)) return false;
            m_state.reg[regField] = second == 0xbf
               ? static_cast<uint32_t>(static_cast<int32_t>(static_cast<int16_t>(value)))
               : value;
            return true;
         }
         return FailOpcode(error, "unsupported IA-32 0F opcode", instructionAddress,
                           second);
      }
   }

   return FailOpcode(error, "unsupported IA-32 opcode", instructionAddress,
                     opcode);
}

cCpu::eRunResult cCpu::Run(uint32_t stopAddress, uint64_t instructionLimit,
                           std::string *error)
{
   for (uint64_t count = 0; count < instructionLimit; ++count)
   {
      if (m_state.eip == stopAddress)
      {
         if (error) error->clear();
         return kRunStopped;
      }
      const uint32_t instructionAddress = m_state.eip;
      if (!Step(error))
      {
         if (error && error->find("guest eip") == std::string::npos)
         {
            char context[192];
            uint32_t frame = m_state.reg[kEbp];
            uint32_t callers[4] = { 0, 0, 0, 0 };
            unsigned callerCount = 0;
            while (frame && callerCount < 4)
            {
               uint32_t next, caller;
               if (!m_memory.Read32(frame, &next, 0) ||
                   !m_memory.Read32(frame + 4, &caller, 0))
                  break;
               callers[callerCount++] = caller;
               if (next <= frame) break;
               frame = next;
            }
            snprintf(context, sizeof(context),
                     " at guest eip 0x%08x (eax=%08x ecx=%08x "
                     "ebp=%08x callers=%08x,%08x,%08x,%08x)",
                     instructionAddress, m_state.reg[kEax],
                     m_state.reg[kEcx], m_state.reg[kEbp], callers[0],
                     callers[1], callers[2], callers[3]);
            *error += context;
         }
         return kRunFault;
      }
   }
   if (error)
      *error = "guest instruction limit exceeded";
   return kRunLimit;
}

} // namespace osm32
