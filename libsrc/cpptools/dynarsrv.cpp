#include <dynarsrv.h>
#include <lgassert.h>

BOOL cDABaseSrvFns::DoResize(void **ppItems, unsigned nItemSize, unsigned nNewSlots)
{
   if (nNewSlots)
   {
      void* newP;

      // Note: Allow for scratch block at the end
      if (*ppItems)
         newP = realloc(*ppItems, (nNewSlots + 1) * nItemSize);
      else
         newP = malloc((nNewSlots + 1) * nItemSize);

      AssertMsg(newP, "Dynamic array resize failed");

      if (!newP)
         return FALSE;

      *ppItems = newP;
   }

   else if (*ppItems)
   {
      free(*ppItems);
      *ppItems = 0;
   }

   return TRUE;
}

#ifdef DEBUG

void cDABaseSrvFns::TrackCreate(unsigned nItemSize)
{
   // TODO
}

void cDABaseSrvFns::TrackDestroy()
{
   // TODO
}

#endif // DEBUG