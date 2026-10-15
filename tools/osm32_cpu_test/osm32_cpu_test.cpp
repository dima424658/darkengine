#include <osm_emu/osm32cpu.h>
#include <osm_emu/osm32abi.h>

#include <stdio.h>
#include <string.h>
#include <string>

namespace
{

const uint32_t kCode = 0x00100000;
const uint32_t kStack = 0x00200000;
const uint32_t kStackSize = 0x00010000;
const uint32_t kStop = 0xffff0000;
const uint32_t kHostAdd = 0xf0000100;

bool Prepare(osm32::cCpu *cpu, const uint8_t *code, size_t codeSize,
             std::string *error)
{
   if (!cpu->Memory().Map(kCode, 0x1000,
                          osm32::kMemRead | osm32::kMemExecute, error) ||
       !cpu->Memory().Map(kStack, kStackSize,
                          osm32::kMemRead | osm32::kMemWrite, error) ||
       !cpu->Memory().Load(kCode, code, codeSize, error))
      return false;
   cpu->State().eip = kCode;
   cpu->State().reg[osm32::kEsp] = kStack + kStackSize;
   return true;
}

bool Enter(osm32::cCpu *cpu, const uint32_t *arguments, size_t count,
           std::string *error)
{
   for (size_t i = count; i != 0; --i)
      if (!cpu->Push32(arguments[i - 1], error))
         return false;
   return cpu->Push32(kStop, error);
}

class cTestHost : public osm32::iHostCalls
{
public:
   virtual bool Handles(uint32_t address) const
   {
      return address == kHostAdd;
   }

   virtual bool Invoke(uint32_t, osm32::sCpuState *state,
                       osm32::cMemory *memory, std::string *error)
   {
      uint32_t returnAddress, left, right;
      if (!memory->Read32(state->reg[osm32::kEsp], &returnAddress, error) ||
          !memory->Read32(state->reg[osm32::kEsp] + 4, &left, error) ||
          !memory->Read32(state->reg[osm32::kEsp] + 8, &right, error))
         return false;
      state->reg[osm32::kEax] = left + right;
      state->reg[osm32::kEsp] += 12;
      state->eip = returnAddress;
      return true;
   }
};

bool TestStdcall()
{
   // int f(int a, int b, int c) { return a * b + c; }
   static const uint8_t code[] = {
      0x55,                         // push ebp
      0x8b, 0xec,                   // mov ebp,esp
      0x8b, 0x45, 0x08,             // mov eax,[ebp+8]
      0x0f, 0xaf, 0x45, 0x0c,       // imul eax,[ebp+12]
      0x03, 0x45, 0x10,             // add eax,[ebp+16]
      0x5d,                         // pop ebp
      0xc2, 0x0c, 0x00              // ret 12
   };
   const uint32_t arguments[] = { 6, 7, 8 };
   osm32::cCpu cpu;
   std::string error;
   if (!Prepare(&cpu, code, sizeof(code), &error) ||
       !Enter(&cpu, arguments, 3, &error) ||
       cpu.Run(kStop, 100, &error) != osm32::cCpu::kRunStopped ||
       cpu.State().reg[osm32::kEax] != 50)
   {
      fprintf(stderr, "stdcall: FAIL: %s eax=%u\n", error.c_str(),
              cpu.State().reg[osm32::kEax]);
      return false;
   }
   puts("stdcall arithmetic: PASS");
   return true;
}

bool TestBranches()
{
   // int sum(int n) { int r=0; while (n) r += n--; return r; }
   static const uint8_t code[] = {
      0x31, 0xc0,                   // xor eax,eax
      0x8b, 0x4c, 0x24, 0x04,       // mov ecx,[esp+4]
      0x85, 0xc9,                   // test ecx,ecx
      0x74, 0x05,                   // jz done
      0x01, 0xc8,                   // add eax,ecx
      0x49,                         // dec ecx
      0x75, 0xfb,                   // jnz loop
      0xc2, 0x04, 0x00              // ret 4
   };
   const uint32_t arguments[] = { 100 };
   osm32::cCpu cpu;
   std::string error;
   if (!Prepare(&cpu, code, sizeof(code), &error) ||
       !Enter(&cpu, arguments, 1, &error) ||
       cpu.Run(kStop, 1000, &error) != osm32::cCpu::kRunStopped ||
       cpu.State().reg[osm32::kEax] != 5050)
   {
      fprintf(stderr, "branches: FAIL: %s eax=%u\n", error.c_str(),
              cpu.State().reg[osm32::kEax]);
      return false;
   }
   puts("branches and flags: PASS");
   return true;
}

bool TestHostCall()
{
   static const uint8_t code[] = {
      0x6a, 0x07,                   // push 7
      0x6a, 0x05,                   // push 5
      0xb8, 0x00, 0x01, 0x00, 0xf0, // mov eax,0xf0000100
      0xff, 0xd0,                   // call eax
      0xc3                          // ret
   };
   osm32::cCpu cpu;
   cTestHost host;
   std::string error;
   cpu.SetHostCalls(&host);
   if (!Prepare(&cpu, code, sizeof(code), &error) ||
       !Enter(&cpu, 0, 0, &error) ||
       cpu.Run(kStop, 100, &error) != osm32::cCpu::kRunStopped ||
       cpu.State().reg[osm32::kEax] != 12)
   {
      fprintf(stderr, "host call: FAIL: %s eax=%u\n", error.c_str(),
              cpu.State().reg[osm32::kEax]);
      return false;
   }
   puts("host call trap: PASS");
   return true;
}

bool TestBytesAndShifts()
{
   static const uint8_t code[] = {
      0xb9, 0x00, 0x55, 0x00, 0x00, // mov ecx,0x5500
      0x31, 0xd2,                   // xor edx,edx
      0x8a, 0xd5,                   // mov dl,ch
      0x80, 0xfa, 0x55,             // cmp dl,0x55
      0x75, 0x17,                   // jne failure
      0xb8, 0x01, 0x00, 0x00, 0x80, // mov eax,0x80000001
      0xc1, 0xe8, 0x01,             // shr eax,1
      0xd1, 0xe0,                   // shl eax,1
      0xd1, 0xf8,                   // sar eax,1
      0xb1, 0x04,                   // mov cl,4
      0xd3, 0xc8,                   // ror eax,cl
      0x3c, 0x00,                   // cmp al,0
      0x75, 0x03,                   // jne failure
      0x01, 0xd0,                   // add eax,edx
      0xc3,                         // ret
      0x31, 0xc0,                   // failure: xor eax,eax
      0xc3                          // ret
   };
   osm32::cCpu cpu;
   std::string error;
   if (!Prepare(&cpu, code, sizeof(code), &error) ||
       !Enter(&cpu, 0, 0, &error) ||
       cpu.Run(kStop, 100, &error) != osm32::cCpu::kRunStopped ||
       cpu.State().reg[osm32::kEax] != 0x0c000055)
   {
      fprintf(stderr, "bytes/shifts: FAIL: %s eax=%08x\n", error.c_str(),
              cpu.State().reg[osm32::kEax]);
      return false;
   }
   puts("byte registers and shifts: PASS");
   return true;
}

bool TestStringStore()
{
   static const uint8_t code[] = {
      0x8d, 0x7c, 0x24, 0xf0,       // lea edi,[esp-16]
      0xb8, 0x78, 0x56, 0x34, 0x12, // mov eax,0x12345678
      0xb9, 0x03, 0x00, 0x00, 0x00, // mov ecx,3
      0xf3, 0xab,                   // rep stosd
      0xc3                          // ret
   };
   osm32::cCpu cpu;
   std::string error;
   uint32_t first = 0, second = 0, third = 0;
   const uint32_t destination = kStack + kStackSize - 20;
   if (!Prepare(&cpu, code, sizeof(code), &error) ||
       !Enter(&cpu, 0, 0, &error) ||
       cpu.Run(kStop, 100, &error) != osm32::cCpu::kRunStopped ||
       !cpu.Memory().Read32(destination, &first, &error) ||
       !cpu.Memory().Read32(destination + 4, &second, &error) ||
       !cpu.Memory().Read32(destination + 8, &third, &error) ||
       first != 0x12345678 || second != first || third != first ||
       cpu.State().reg[osm32::kEcx] != 0)
   {
      fprintf(stderr, "string store: FAIL: %s values=%08x,%08x,%08x\n",
              error.c_str(), first, second, third);
      return false;
   }
   puts("repeated string store: PASS");
   return true;
}

bool TestByteShiftAndSetCondition()
{
   static const uint8_t code[] = {
      0xb0, 0x81,                   // mov al,0x81
      0xc0, 0xe8, 0x01,             // shr al,1
      0x3c, 0x40,                   // cmp al,0x40
      0x0f, 0x95, 0xc1,             // setne cl
      0x0f, 0x94, 0xc2,             // sete dl
      0x0f, 0xb6, 0xc2,             // movzx eax,dl
      0xc3                          // ret
   };
   osm32::cCpu cpu;
   std::string error;
   if (!Prepare(&cpu, code, sizeof(code), &error) ||
       !Enter(&cpu, 0, 0, &error) ||
       cpu.Run(kStop, 100, &error) != osm32::cCpu::kRunStopped ||
       cpu.State().reg[osm32::kEax] != 1 ||
       (cpu.State().reg[osm32::kEcx] & 0xff) != 0)
   {
      fprintf(stderr, "byte shift/setcc: FAIL: %s eax=%08x ecx=%08x\n",
              error.c_str(), cpu.State().reg[osm32::kEax],
              cpu.State().reg[osm32::kEcx]);
      return false;
   }
   puts("byte shift and setcc: PASS");
   return true;
}

bool TestAbsoluteByteMoves()
{
   static const uint32_t dataAddress = kStack + kStackSize - 32;
   static const uint8_t code[] = {
      0xb8, 0x00, 0x56, 0x34, 0x12, // mov eax,0x12345600
      0xa0, 0xe0, 0xff, 0x20, 0x00, // mov al,byte ptr [0x20ffe0]
      0xa2, 0xe1, 0xff, 0x20, 0x00, // mov byte ptr [0x20ffe1],al
      0xc3                          // ret
   };
   osm32::cCpu cpu;
   std::string error;
   uint8_t copied = 0;
   if (!Prepare(&cpu, code, sizeof(code), &error) ||
       !cpu.Memory().Write8(dataAddress, 0x7b, &error) ||
       !Enter(&cpu, 0, 0, &error) ||
       cpu.Run(kStop, 100, &error) != osm32::cCpu::kRunStopped ||
       !cpu.Memory().Read8(dataAddress + 1, &copied, &error) ||
       cpu.State().reg[osm32::kEax] != 0x1234567b || copied != 0x7b)
   {
      fprintf(stderr, "absolute byte moves: FAIL: %s eax=%08x byte=%02x\n",
              error.c_str(), cpu.State().reg[osm32::kEax], copied);
      return false;
   }
   puts("absolute byte moves: PASS");
   return true;
}

bool TestX87SingleArithmetic()
{
   static const uint32_t leftAddress = kStack + kStackSize - 48;
   static const uint32_t rightAddress = leftAddress + 4;
   static const uint32_t scaleAddress = leftAddress + 8;
   static const uint32_t resultAddress = leftAddress + 12;
   static const uint8_t code[] = {
      0xd9, 0x05, 0xd0, 0xff, 0x20, 0x00, // fld [0x20ffd0]
      0xd8, 0x25, 0xd4, 0xff, 0x20, 0x00, // fsub [0x20ffd4]
      0xd8, 0x0d, 0xd8, 0xff, 0x20, 0x00, // fmul [0x20ffd8]
      0xd9, 0x1d, 0xdc, 0xff, 0x20, 0x00, // fstp [0x20ffdc]
      0xa1, 0xdc, 0xff, 0x20, 0x00,       // mov eax,[0x20ffdc]
      0xc3                                // ret
   };
   const float values[] = { 10.0f, 2.0f, 3.0f };
   uint32_t expected;
   const float expectedFloat = 24.0f;
   memcpy(&expected, &expectedFloat, sizeof(expected));
   osm32::cCpu cpu;
   std::string error;
   if (!Prepare(&cpu, code, sizeof(code), &error) ||
       !cpu.Memory().Load(leftAddress, values, sizeof(values), &error) ||
       !Enter(&cpu, 0, 0, &error) ||
       cpu.Run(kStop, 100, &error) != osm32::cCpu::kRunStopped ||
       cpu.State().reg[osm32::kEax] != expected ||
       cpu.State().x87Depth != 0)
   {
      fprintf(stderr, "x87 single arithmetic: FAIL: %s eax=%08x depth=%u\n",
              error.c_str(), cpu.State().reg[osm32::kEax],
              cpu.State().x87Depth);
      return false;
   }
   puts("x87 single arithmetic: PASS");
   return true;
}

bool TestX87Integer64Store()
{
   static const uint32_t valueAddress = kStack + kStackSize - 64;
   static const uint32_t resultAddress = valueAddress + 8;
   static const uint8_t code[] = {
      0xd9, 0x05, 0xc0, 0xff, 0x20, 0x00, // fld [0x20ffc0]
      0xdf, 0x3d, 0xc8, 0xff, 0x20, 0x00, // fistp qword ptr [0x20ffc8]
      0xa1, 0xc8, 0xff, 0x20, 0x00,       // mov eax,[0x20ffc8]
      0xc3                                // ret
   };
   const float input = 42.0f;
   uint64_t result = 0;
   osm32::cCpu cpu;
   std::string error;
   if (!Prepare(&cpu, code, sizeof(code), &error) ||
       !cpu.Memory().Load(valueAddress, &input, sizeof(input), &error) ||
       !Enter(&cpu, 0, 0, &error) ||
       cpu.Run(kStop, 100, &error) != osm32::cCpu::kRunStopped ||
       !cpu.Memory().Read64(resultAddress, &result, &error) ||
       result != 42 || cpu.State().x87Depth != 0)
   {
      fprintf(stderr, "x87 integer64 store: FAIL: %s result=%llu depth=%u\n",
              error.c_str(), static_cast<unsigned long long>(result),
              cpu.State().x87Depth);
      return false;
   }
   puts("x87 integer64 store: PASS");
   return true;
}

bool TestX87PopArithmetic()
{
   static const uint32_t leftAddress = kStack + kStackSize - 80;
   static const uint32_t rightAddress = leftAddress + 4;
   static const uint8_t code[] = {
      0xd9, 0x05, 0xb0, 0xff, 0x20, 0x00, // fld [0x20ffb0]
      0xd9, 0x05, 0xb4, 0xff, 0x20, 0x00, // fld [0x20ffb4]
      0xde, 0xc1,                         // faddp st(1),st
      0xd9, 0x1d, 0xb0, 0xff, 0x20, 0x00, // fstp [0x20ffb0]
      0xc3                                // ret
   };
   const float values[] = { 3.0f, 4.0f };
   uint32_t resultBits = 0;
   float result = 0.0f;
   osm32::cCpu cpu;
   std::string error;
   if (!Prepare(&cpu, code, sizeof(code), &error) ||
       !cpu.Memory().Load(leftAddress, values, sizeof(values), &error) ||
       !Enter(&cpu, 0, 0, &error) ||
       cpu.Run(kStop, 100, &error) != osm32::cCpu::kRunStopped ||
       !cpu.Memory().Read32(leftAddress, &resultBits, &error))
   {
      fprintf(stderr, "x87 pop arithmetic: FAIL: %s\n", error.c_str());
      return false;
   }
   memcpy(&result, &resultBits, sizeof(result));
   if (result != 7.0f || cpu.State().x87Depth != 0)
   {
      fprintf(stderr, "x87 pop arithmetic: FAIL: result=%g depth=%u\n",
              result, cpu.State().x87Depth);
      return false;
   }
   puts("x87 pop arithmetic: PASS");
   return true;
}

bool TestX87ExamineAndStatusStore()
{
   static const uint32_t valueAddress = kStack + kStackSize - 96;
   static const uint32_t statusAddress = valueAddress + 4;
   static const uint8_t code[] = {
      0xd9, 0x05, 0xa0, 0xff, 0x20, 0x00, // fld [0x20ffa0]
      0xd9, 0xe5,                         // fxam
      0xdd, 0x3d, 0xa4, 0xff, 0x20, 0x00, // fnstsw [0x20ffa4]
      0xc3                                // ret
   };
   const float value = -0.0f;
   uint16_t status = 0;
   osm32::cCpu cpu;
   std::string error;
   if (!Prepare(&cpu, code, sizeof(code), &error) ||
       !cpu.Memory().Load(valueAddress, &value, sizeof(value), &error) ||
       !Enter(&cpu, 0, 0, &error) ||
       cpu.Run(kStop, 100, &error) != osm32::cCpu::kRunStopped ||
       !cpu.Memory().Read16(statusAddress, &status, &error) ||
       (status & 0x4700u) != 0x4200u)
   {
      fprintf(stderr, "x87 examine/status: FAIL: %s status=%04x\n",
              error.c_str(), status);
      return false;
   }
   puts("x87 examine and status store: PASS");
   return true;
}

bool TestXlat()
{
   static const uint32_t tableAddress = kStack + kStackSize - 128;
   static const uint8_t code[] = {
      0xbb, 0x80, 0xff, 0x20, 0x00, // mov ebx,0x20ff80
      0xb8, 0x02, 0x00, 0x00, 0x00, // mov eax,2
      0xd7,                         // xlat
      0xc3                          // ret
   };
   const uint8_t table[] = { 10, 20, 30, 40 };
   osm32::cCpu cpu;
   std::string error;
   if (!Prepare(&cpu, code, sizeof(code), &error) ||
       !cpu.Memory().Load(tableAddress, table, sizeof(table), &error) ||
       !Enter(&cpu, 0, 0, &error) ||
       cpu.Run(kStop, 100, &error) != osm32::cCpu::kRunStopped ||
       cpu.State().reg[osm32::kEax] != 30)
   {
      fprintf(stderr, "xlat: FAIL: %s eax=%08x\n", error.c_str(),
              cpu.State().reg[osm32::kEax]);
      return false;
   }
   puts("xlat: PASS");
   return true;
}

bool TestX87RegisterAndSqrt()
{
   static const uint32_t firstAddress = kStack + kStackSize - 144;
   static const uint32_t secondAddress = firstAddress + 4;
   static const uint8_t code[] = {
      0xd9, 0x05, 0x70, 0xff, 0x20, 0x00, // fld [0x20ff70] (9)
      0xd9, 0x05, 0x74, 0xff, 0x20, 0x00, // fld [0x20ff74] (4)
      0xd9, 0xc9,                         // fxch st(1)
      0xd9, 0xfa,                         // fsqrt -> 3
      0xde, 0xc1,                         // faddp -> 7
      0xd9, 0xe4,                         // ftst
      0xd9, 0x1d, 0x70, 0xff, 0x20, 0x00, // fstp [0x20ff70]
      0xc3                                // ret
   };
   const float values[] = { 9.0f, 4.0f };
   uint32_t resultBits = 0;
   float result = 0.0f;
   osm32::cCpu cpu;
   std::string error;
   if (!Prepare(&cpu, code, sizeof(code), &error) ||
       !cpu.Memory().Load(firstAddress, values, sizeof(values), &error) ||
       !Enter(&cpu, 0, 0, &error) ||
       cpu.Run(kStop, 100, &error) != osm32::cCpu::kRunStopped ||
       !cpu.Memory().Read32(firstAddress, &resultBits, &error))
   {
      fprintf(stderr, "x87 register/sqrt: FAIL: %s\n", error.c_str());
      return false;
   }
   memcpy(&result, &resultBits, sizeof(result));
   if (result != 7.0f || (cpu.State().x87Status & 0x4500u) != 0)
   {
      fprintf(stderr, "x87 register/sqrt: FAIL: result=%g status=%04x\n",
              result, cpu.State().x87Status);
      return false;
   }
   puts("x87 register exchange and sqrt: PASS");
   return true;
}

bool TestGuestAbiMaps()
{
   struct sGoldServiceLayout
   {
      uint16_t id;
      uint8_t lastSlot;
   };
   static const sGoldServiceLayout layouts[] = {
      { 0x00d7,  7 }, { 0x00da, 11 }, { 0x00df, 24 }, { 0x00e5, 14 },
      { 0x00ee, 15 }, { 0x00ef,  9 }, { 0x00f1, 18 }, { 0x00f4, 13 },
      { 0x00f6,  8 }, { 0x00fb,  5 }, { 0x00fd,  5 }, { 0x00fe,  7 },
      { 0x010d,  5 }, { 0x010e,  9 }, { 0x0111, 10 }, { 0x0115, 11 },
      { 0x0141, 11 }, { 0x0150,  7 }, { 0x0152,  9 }, { 0x0153,  7 },
      { 0x015d,  8 }, { 0x016a,  5 }, { 0x016c, 10 }, { 0x017d,  7 },
      { 0x019f, 11 }, { 0x01a0, 10 }, { 0x01b4,  7 }, { 0x01f8,  5 }
   };
   struct sMethodMap
   {
      osm32::eAbiService service;
      uint8_t guestSlot;
      uint8_t nativeSlot;
   };
   static const sMethodMap goldMappings[] = {
      { osm32::kAbiServiceObject, 22, 24 },
      { osm32::kAbiServiceObject, 23, 25 },
      { osm32::kAbiServiceObject, 24, 26 },
      { osm32::kAbiServiceProperty, 8, 9 },
      { osm32::kAbiServiceProperty, 9, 10 },
      { osm32::kAbiServiceProperty, 10, 11 },
      { osm32::kAbiServiceProperty, 11, 12 },
      { osm32::kAbiServicePhysics, 10, 12 },
      { osm32::kAbiServicePhysics, 11, 13 },
      { osm32::kAbiServiceSound, 5, 7 },
      { osm32::kAbiServiceSound, 6, 6 },
      { osm32::kAbiServiceSound, 7, 5 },
      { osm32::kAbiServiceSound, 8, 8 },
      { osm32::kAbiServiceSound, 9, 11 },
      { osm32::kAbiServiceSound, 10, 10 },
      { osm32::kAbiServiceSound, 11, 9 },
      { osm32::kAbiServiceSound, 12, 12 },
      { osm32::kAbiServiceAI, 6, 7 },
      { osm32::kAbiServiceAI, 7, 6 }
   };
   unsigned slot = 0;
   bool passed =
      osm32::GuestAbiFromProfile("gold") == osm32::kGuestAbiDark1 &&
      osm32::GuestAbiFromProfile("TDP") == osm32::kGuestAbiDark1 &&
      osm32::GuestAbiFromProfile("thief2") == osm32::kGuestAbiDark2 &&
      !osm32::ResolveServiceMethod(osm32::kGuestAbiDark1,
         osm32::kAbiServiceProperty, 12, &slot) &&
      osm32::ResolveServiceMethod(osm32::kGuestAbiDark2,
         osm32::kAbiServiceSound, 5, &slot) && slot == 7 &&
      osm32::ResolveServiceMethod(osm32::kGuestAbiDark1,
         osm32::kAbiServiceAI, 6, &slot) && slot == 7 &&
      !osm32::ResolveServiceMethod(osm32::kGuestAbiDark1,
         osm32::kAbiServicePhysics, 12, &slot) &&
      !osm32::ResolveServiceMethod(osm32::kGuestAbiDark1,
         osm32::kAbiServiceSound, 19, &slot) &&
      !osm32::ResolveServiceMethod(osm32::kGuestAbiDark1,
         osm32::kAbiServiceAI, 15, &slot) &&
      !osm32::GuestPostMessageHasFlags(osm32::kGuestAbiDark1) &&
      osm32::GuestPostMessageHasFlags(osm32::kGuestAbiDark2) &&
      !osm32::GuestSoundHasNetworkArgument(osm32::kGuestAbiDark1) &&
      !osm32::GuestStimulateHasSourceArgument(osm32::kGuestAbiDark1);

   for (size_t i = 0;
        i < sizeof(goldMappings) / sizeof(goldMappings[0]); ++i)
      passed = passed &&
         osm32::ResolveServiceMethod(osm32::kGuestAbiDark1,
                                     goldMappings[i].service,
                                     goldMappings[i].guestSlot, &slot) &&
         slot == goldMappings[i].nativeSlot;

   for (size_t i = 0; i < sizeof(layouts) / sizeof(layouts[0]); ++i)
      passed = passed &&
         osm32::GuestServiceMethodExists(osm32::kGuestAbiDark1,
                                         layouts[i].id, 3) &&
         osm32::GuestServiceMethodExists(osm32::kGuestAbiDark1,
                                         layouts[i].id,
                                         layouts[i].lastSlot) &&
         !osm32::GuestServiceMethodExists(osm32::kGuestAbiDark1,
                                          layouts[i].id,
                                          layouts[i].lastSlot + 1);
   passed = passed &&
      osm32::GuestServiceMethodExists(osm32::kGuestAbiDark1, 0x7fff, 5);
   if (!passed)
   {
      fprintf(stderr, "guest ABI maps: FAIL\n");
      return false;
   }
   puts("guest ABI maps: PASS");
   return true;
}

bool TestX87IntegerArithmetic()
{
   static const uint32_t realAddress = kStack + kStackSize - 160;
   static const uint32_t integerAddress = realAddress + 4;
   static const uint8_t code[] = {
      0xd9, 0x05, 0x60, 0xff, 0x20, 0x00, // fld [0x20ff60]
      0xda, 0x0d, 0x64, 0xff, 0x20, 0x00, // fimul dword ptr [0x20ff64]
      0xd9, 0x1d, 0x60, 0xff, 0x20, 0x00, // fstp [0x20ff60]
      0xc3                                // ret
   };
   const float input = 2.5f;
   const int32_t multiplier = 4;
   uint32_t resultBits = 0;
   float result = 0.0f;
   osm32::cCpu cpu;
   std::string error;
   if (!Prepare(&cpu, code, sizeof(code), &error) ||
       !cpu.Memory().Load(realAddress, &input, sizeof(input), &error) ||
       !cpu.Memory().Load(integerAddress, &multiplier, sizeof(multiplier),
                          &error) ||
       !Enter(&cpu, 0, 0, &error) ||
       cpu.Run(kStop, 100, &error) != osm32::cCpu::kRunStopped ||
       !cpu.Memory().Read32(realAddress, &resultBits, &error))
   {
      fprintf(stderr, "x87 integer arithmetic: FAIL: %s\n", error.c_str());
      return false;
   }
   memcpy(&result, &resultBits, sizeof(result));
   if (result != 10.0f || cpu.State().x87Depth != 0)
   {
      fprintf(stderr, "x87 integer arithmetic: FAIL: result=%g depth=%u\n",
              result, cpu.State().x87Depth);
      return false;
   }
   puts("x87 integer arithmetic: PASS");
   return true;
}

} // namespace

int main()
{
   return TestGuestAbiMaps() &&
          TestStdcall() && TestBranches() && TestHostCall() &&
          TestBytesAndShifts() && TestStringStore() &&
          TestByteShiftAndSetCondition() && TestAbsoluteByteMoves() &&
          TestX87SingleArithmetic() && TestX87Integer64Store() &&
          TestX87PopArithmetic() && TestX87ExamineAndStatusStore() &&
          TestXlat() && TestX87RegisterAndSqrt() &&
          TestX87IntegerArithmetic() ? 0 : 1;
}
