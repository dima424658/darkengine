/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/prompts.c,v 1.3 2000/02/19 13:11:16 toml Exp $
// basic prompts for asking for data from the user

#include <string.h>
#include <stdlib.h>

#include <lg.h>

#include <sdesc.h>
#include <sdesbase.h>
#include <isdesced.h>
#include <isdescst.h>

#include <prompts.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

///////////////////
// auto-prompters for menu use

static BOOL do_prompt_editor(void *data, sStructDesc *sd, char *title)
{
   IStructEditor *sed;
   sStructEditorDesc editdesc;
   BOOL result;

   strncpy(editdesc.title, title, sizeof(editdesc.title));
   editdesc.flags = kStructEditNoApplyButton; 
   sed = CreateStructEditor(&editdesc, sd, data);
   if (!sed) return FALSE;   // got me, it didnt work

   result = IStructEditor_Go(sed,kStructEdModal); 
   SafeRelease(sed);
   return result;
}

typedef struct { BOOL data; } sBoolPrompt;
static sFieldDesc bool_fields[]={{"Bool Arg",kFieldTypeBool,FieldLocation(sBoolPrompt,data) } };
static sStructDesc bool_struct = StructDescBuild(sBoolPrompt,kStructFlagNone,bool_fields);

BOOL prompt_bool(const char *def, BOOL *value)
{
   sBoolPrompt bdata;
   bdata.data=TRUE;
   if (def)
      if ((def[0]=='f')||(def[0]=='F')||(def[0]=='0'))
         bdata.data=FALSE;
   if (!do_prompt_editor(&bdata,&bool_struct,"Bool Argument"))
      return FALSE;
   *value = bdata.data;
   return TRUE;
}

typedef struct { int data; } sIntPrompt;
static sFieldDesc int_fields[]={{"Int Arg",kFieldTypeInt,FieldLocation(sIntPrompt,data) } };
static sStructDesc int_struct = StructDescBuild(sIntPrompt,kStructFlagNone,int_fields);

BOOL prompt_int(const char *def, int *value)
{
   sIntPrompt idata;
   if (def)
      idata.data=atoi(def);
   else
      idata.data=0;
   if (!do_prompt_editor(&idata,&int_struct,"Int Argument"))
      return FALSE;
   *value = idata.data;
   return TRUE;
}

typedef struct { double data; } sDoublePrompt;
static sFieldDesc double_fields[]={{"Double Arg",kFieldTypeFloat,FieldLocation(sDoublePrompt,data) } };
static sStructDesc double_struct = StructDescBuild(sDoublePrompt,kStructFlagNone,double_fields);

BOOL prompt_double(const char *def, double *value)
{
   sDoublePrompt ddata;
   if (def)
      ddata.data=atof(def);
   else
      ddata.data=0.0;
   if (!do_prompt_editor(&ddata,&double_struct,"Double Argument"))
      return FALSE;
   *value = ddata.data;
   return TRUE;
}

// major memory tracking issue here!!! - not going to work
typedef struct { char data[PROMPT_STR_LEN]; } sStringPrompt;
static sFieldDesc string_fields[]={{"String Arg",kFieldTypeString,FieldLocation(sStringPrompt,data) } };
static sStructDesc string_struct = StructDescBuild(sStringPrompt,kStructFlagNone,string_fields);

// Copy the accepted text to the caller's PROMPT_STR_LEN buffer.
BOOL prompt_string_arg(const char *def, char *buf)
{
   sStringPrompt sdata;
   if (def)
      strncpy(sdata.data,def,PROMPT_STR_LEN);
   else
      strcpy(sdata.data,"");
   sdata.data[PROMPT_STR_LEN-1]='\0';
   if (do_prompt_editor(&sdata,&string_struct,"String Argument"))
   {
      strcpy(buf,sdata.data);
      return TRUE;
   }
   return FALSE;
}

char *prompt_string(const char *def, char *buf)
{
   return prompt_string_arg(def,buf) ? buf : (char *)def;
}
