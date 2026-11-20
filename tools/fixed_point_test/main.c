#include <fix.h>
#include <assert.h>
#include <stdio.h>

_Static_assert(fix_make(-1, 0x8000) == -32768, "negative fractional bit pattern");
_Static_assert(fix_make(-32768, 0) == INT32_MIN, "minimum fixed-point value");
static const fix constants[] = {fix_make(0, 0), fix_make(1, 0), fix_make(-1, 32767)};

int main(void)
{
   const int fractions[] = {0, 1, 32767, 32768, 65535};
   assert(constants[0] == 0 && constants[1] == 65536 && constants[2] == -32769);
   for (int integer = -32768; integer <= 32767; ++integer)
      for (size_t i = 0; i < sizeof(fractions) / sizeof(fractions[0]); ++i)
      {
         const int64_t expected = (int64_t)integer * 65536 + fractions[i];
         assert(fix_make(integer, fractions[i]) == expected);
      }
   assert(fix_make(UINT32_C(0x12348000), 0) == INT32_MIN);
   int integer = -1, fraction = 32768;
   assert(fix_make(integer++, fraction++) == -32768);
   assert(integer == 0 && fraction == 32769);
   puts("Fixed-point constants, negative fractions, limits, and single evaluation passed");
   return 0;
}
