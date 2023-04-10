/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/sdesparm.cpp,v 1.6 1998/10/05 17:27:53 mahk Exp $

#include <comtools.h>
#include <appagg.h>

#include <scrptbas.h>
#include <sdesparm.h>

#include <sdesbase.h>
#include <sdestool.h>

// Must be last header 
#include <dbmem.h>


static IStructDescTools* SdescTools()
{
   static IStructDescTools* tools = NULL;
   if (tools == NULL)
   {
      tools = AppGetObj(IStructDescTools);
   }
   return tools;
} 

cMultiParm GetParmFromField(const sFieldDesc* field, void *buf)
{
   cMultiParm retval = 0; 

   switch (field->type)
   {
      case kFieldTypeInt:
      case kFieldTypeShort:
      case kFieldTypeBits:
      case kFieldTypeEnum:
      case kFieldTypeFix:
      {
         long val;
         SdescTools()->GetIntegral(field,buf,&val);
         retval = (int)val;
      }
      break;

      case kFieldTypeBool:
      {
         long val;
         SdescTools()->GetIntegral(field,buf,&val);
         retval = (boolean)val;
      }
      break;

      case kFieldTypeFloat:
         retval =  *(float*)((char*)buf + field->offset);
         break;

      case kFieldTypeVector:
         retval = *(mxs_vector*)((char*)buf + field->offset); 
         break;

      default:
      {
         int len = field->size < 64 ? 64 : field->size;
         char* str = new char[len];
         SdescTools()->UnparseField(field,buf,str,len);
         retval = (string)str;
         delete [] str;
      }
      break;
   }
   return retval; 
}

void SetFieldFromParm(const sFieldDesc* field, void *buf, const cMultiParm& val)
{
   switch (field->type)
   {
      case kFieldTypeInt:
      case kFieldTypeShort:
      case kFieldTypeBits:
      case kFieldTypeEnum:
      case kFieldTypeBool:
      case kFieldTypeFix:
         SdescTools()->SetIntegral(field,(int)val,buf);
         break;

      case kFieldTypeFloat:
         *(float*)((char*)buf + field->offset) = (float)val;
         break;

      case kFieldTypeVector:
         *(mxs_vector*)((char*)buf + field->offset) = (mxs_vector)val;
         break;


      default:
         SdescTools()->ParseField(field,(const char*)val,buf);
         break;
   }
}   


void FillStructFromParams(const sStructDesc* sdesc, void* struc, cMultiParm** parms, int nparms)
{
   int n = sdesc->nfields;
   if (n > nparms) n = nparms; 
   for (int i = 0; i < n; i++)
   {
      const sFieldDesc* field  = &sdesc->fields[i]; 
      const cMultiParm& val = *parms[i]; 

      SetFieldFromParm(field,struc,val); 
   }
}

