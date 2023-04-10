/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aitagtyp.cpp,v 1.9 2000/02/19 12:45:49 toml Exp $
//
//
//

#include <tagfile.h>
#include <vernum.h>
#include <multparm.h>
#include <str.h>

#include <aitagtyp.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

#if defined (__WATCOMC__)
    #define snprintf    _bprintf
    #define vsnprintf   _vbprintf
#elif defined (__SC__) || defined (_MSC_VER)
    #define snprintf    _snprintf
    #define vsnprintf   _vsnprintf
#else
#error ("Need size-limited printf functions");
#endif

///////////////////////////////////////////////////////////////////////////////

BOOL AICreateTag(ObjID obj, tAISaveLoadSubTag subTag, unsigned addId, TagFileTag * pResult)
{
   if (obj <= 0xffff && subTag <= 0xfff && addId <= 0xff)
   {
      snprintf(pResult->label, sizeof(pResult->label),
               "AI%4x%3x%2x",
               obj,
               subTag,
               addId);
               
      char * p = pResult->label;
      while (*p)
      {
         if (*p == ' ')
            *p = '_';
         p++;
      }
      
      Assert_(strlen(pResult->label) == sizeof(pResult->label) - 1);
      return TRUE;
   }
   CriticalMsg("Illegal AI tag specification");
   return FALSE;
}

///////////////////////////////////////

// Increment this to invalidate all mid-sim saved AI information
int kAIMajorVerNum = 8;

BOOL AIOpenTagBlock(ObjID obj, tAISaveLoadSubTag subTag, unsigned addId, unsigned subTagVer, ITagFile * pTagFile)
{
   HRESULT result;
   TagVersion v;
   
   v.major = kAIMajorVerNum;
   v.minor = subTagVer;
   
   TagFileTag tag;
   
   AICreateTag(obj, subTag, addId, &tag);

   result = pTagFile->OpenBlock(&tag, &v);

   if (result == S_OK)
   {
      if (v.major == kAIMajorVerNum && v.minor == subTagVer)
         return TRUE;
      pTagFile->CloseBlock();
   }

   return FALSE;
}

///////////////////////////////////////

BOOL AITagModeWrite(ITagFile * pTagFile)
{
   return (pTagFile->OpenMode() == kTagOpenWrite);
}

///////////////////////////////////////

void AICloseTagBlock(ITagFile * pTagFile)
{
   pTagFile->CloseBlock();
}

///////////////////////////////////////

void AITagMoveRaw(ITagFile * pTagFile, void * pData, unsigned nBytes)
{
   AssertMsg(pData, "Attempting to save NULL data");
   if (!pData)
      return;
   pTagFile->Move((char *)pData, nBytes);
}

///////////////////////////////////////

void AITagMoveMultiParm(ITagFile * pTagFile, cMultiParm * pParm)
{
   if (!AITagModeWrite(pTagFile))
      ClearParm(pParm);
   
   AITagMove(pTagFile, &pParm->type);

   switch (pParm->type)
   {
      case kMT_Undef:
         break;
      case kMT_Int:
         AITagMove(pTagFile, &pParm->i);
         break;
      case kMT_Float:
         AITagMove(pTagFile, &pParm->f);
         break;
      case kMT_String:
      {
         cStr tempStr;
         if (AITagModeWrite(pTagFile))
         {
            tempStr = pParm->operator const char *();
            AITagMoveString(pTagFile, &tempStr);
         }
         else
         {
            AITagMoveString(pTagFile, &tempStr);
            pParm->psz = tempStr.Detach();
         }
         break;
      }
      case kMT_Vector:
         if (!AITagModeWrite(pTagFile))
            pParm->pVec = new mxs_vector;
         AITagMove(pTagFile, pParm->pVec);
         break;
      default:
         CriticalMsg("Unknown MultiParm type");
   }
}

///////////////////////////////////////////////////////////////////////////////
