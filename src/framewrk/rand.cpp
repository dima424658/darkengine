/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Source: r:/t2repos/thief2/src/framewrk/rand.cpp,v $
// $Author: XEMU $
// $Date: 1999/06/05 17:55:16 $
// $Revision: 1.9 $
//

#include <stdlib.h>
#include <lg.h>
#include <appagg.h>
#include <recapi.h>
#include <rnd.h>
#include <rand.h>
#include <hashfns.h>

// Must be last header 
#include <dbmem.h>

//#define COUNT_RANDS

///////////////////////////////////////////////////////////////////////////////

uchar g_R250IncrementTable[250] = { 1,  2,  3,  4,  5,  6,  7,  8,  9, 10,
                                    11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
                                    21, 22, 23, 24, 25, 26, 27, 28, 29, 30,
                                    31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
                                    41, 42, 43, 44, 45, 46, 47, 48, 49, 50,
                                    51, 52, 53, 54, 55, 56, 57, 58, 59, 60,
                                    61, 62, 63, 64, 65, 66, 67, 68, 69, 70,
                                    71, 72, 73, 74, 75, 76, 77, 78, 79, 80,
                                    81, 82, 83, 84, 85, 86, 87, 88, 89, 90,
                                    91, 92, 93, 94, 95, 96, 97, 98, 99,100,
                                    101,102,103,104,105,106,107,108,109,110,
                                    111,112,113,114,115,116,117,118,119,120,
                                    121,122,123,124,125,126,127,128,129,130,
                                    131,132,133,134,135,136,137,138,139,140,
                                    141,142,143,144,145,146,147,148,149,150,
                                    151,152,153,154,155,156,157,158,159,160,
                                    161,162,163,164,165,166,167,168,169,170,
                                    171,172,173,174,175,176,177,178,179,180,
                                    181,182,183,184,185,186,187,188,189,190,
                                    191,192,193,194,195,196,197,198,199,200,
                                    201,202,203,204,205,206,207,208,209,210,
                                    211,212,213,214,215,216,217,218,219,220,
                                    221,222,223,224,225,226,227,228,229,230,
                                    231,232,233,234,235,236,237,238,239,240,
                                    241,242,243,244,245,246,247,248,249,  0 };

unsigned g_R250Index1 = 0;
unsigned g_R250Index2 = 103;
unsigned g_R250Table[250];

///////////////////////////////////////////////////////////////////////////////

extern "C"
__declspec(dllimport) DWORD __stdcall timeGetTime(void);

static BOOL g_fRandInited;
static RNDSTREAM_STD(g_CoreRandStream);

#ifdef COUNT_RANDS
int RandsBuilt=0;
#define OneMoreRand() do { RandsBuilt++; } while (0)
#else
#define OneMoreRand()
#endif

///////////////////////////////////////////////////////////////////////////////

long Rand250()
{
   int i1, i2;

   g_R250Index1 = g_R250IncrementTable[i1 = g_R250Index1];
   g_R250Index2 = g_R250IncrementTable[i2 = g_R250Index2];
   
   return g_R250Table[i1] ^= g_R250Table[i2];
}

void Rand250Seed(int seed)
{
   seed = seed ^ HashLong(seed);
   srand(seed);

   int i;
   unsigned long lo, hi, ll, lh, hh, hl;
   unsigned long msk, bit;

#define IL_RMULT 1103515245
   
   for (i = 0; i < 250; i++)
   {
      lo = seed & 0xffff;
      hi = seed >> 16;
      seed = seed * IL_RMULT + 12345;
      ll = lo * (IL_RMULT  & 0xffff);
      lh = lo * (IL_RMULT >> 16    );  
      hl = hi * (IL_RMULT  & 0xffff);
      hh = hi * (IL_RMULT >> 16    );  
      g_R250Table[i] = (((ll + 12345) >> 16) + lh + hl + (hh << 16)) ^ 
                       (Rnd(&g_CoreRandStream) | (Rnd(&g_CoreRandStream) >> 16)) ^
                       HashLong(seed) ^
                       rand();
   }

   for (i = 3, msk = 0xffffffff, bit = 0x80000000; bit; i += 7) 
   {
      g_R250Table[i] = (g_R250Table[i] & msk) | bit;
      msk >>= 1;
      bit >>= 1;
   }

   g_R250Index1 = 0;
   g_R250Index2 = 103;
   
   for (i = 0; i < 1000; i++)
      Rand250();
}

void AppRandInit()
{
   long seed = timeGetTime();
   
   IRecorder * pRecorder = AppGetObj(IRecorder);
   RecStreamAddOrExtract(pRecorder, &seed, sizeof(ulong), "DarkRandSeed");
   SafeRelease(pRecorder);

   RndSeed(&g_CoreRandStream, seed);
   Rand250Seed(seed);
   
   g_fRandInited = TRUE;
}

int Rand(void)
{
   if (!g_fRandInited)
      AppRandInit();

   OneMoreRand();

#if 0
   return (Rand250() & 0x7fffffff);
#else
// @TBD (toml 10-09-98): track down and kill people relying on 15 bits
   return (Rand250() & 0x00007fff);
#endif
}

int RandRange(int low, int high)
{
   if (low >= high) 
      return low;
   int randval = Rand();
   
   int range = high - low + 1;
   return low + (randval % range);
}

int UnrecordedRand(void)
{
   static BOOL fInited;
   if (!fInited)
   {
      srand(timeGetTime());
      fInited = TRUE;
   }
   OneMoreRand();
   return rand();
}
