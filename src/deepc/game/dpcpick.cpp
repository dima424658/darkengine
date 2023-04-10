/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// Deep Cover picking - for distant objects 
//

#include <dpcpick.h>

#include <pick.h>

// Must be last header 
#include <dbmem.h>

extern float g_fMaxDistSquared;
extern float Weight(ObjID obj);
 
EXTERN void DPCPickWeighObject(ObjID obj, float pickDistSquared, ObjID* pPickObj, float* pBestPickWeight)
{
   float newPickWeight;
   float temp;

   // save off global, temporarily replace it
   temp = g_fMaxDistSquared;
   g_fMaxDistSquared = pickDistSquared;
   // do pick weighing
   newPickWeight = Weight(obj);
   if (newPickWeight < *pBestPickWeight)
   {
      *pBestPickWeight = newPickWeight;
      *pPickObj = obj;              
   }
   // restore old pick dist
   g_fMaxDistSquared = temp;
}

