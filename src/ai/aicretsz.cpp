/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aicretsz.cpp,v 1.3 2000/03/01 16:22:37 toml Exp $
//
//
//


#include <filevar.h>
#include <sdesbase.h>
#include <sdesc.h>

#include <aicretsz.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

///////////////////////////////////////////////////////////////////////////////

sAICreatureSizes g_AIDefaultCreatureSizes = 
{
   {
      // Width    Height
      {  2.41,    6.0833 }
   }
};

///////////////////////////////////////////////////////////////////////////////

sAICreatureSizes * g_pAICreatureSizes;

class cAICreatureSizes : public cFileVar2<sAICreatureSizes>
{
public:   
   cAICreatureSizes()
      : cFileVar2<sAICreatureSizes> (kGameSysVar,
                                   "AICRTSZ",
                                   "AI Creature Sizes",
                                   FILEVAR_TYPE(sAICreatureSizes),
                                   1, 1,
                                   1, 1,
                                   NULL,
                                   &g_pAICreatureSizes,
                                   &g_AIDefaultCreatureSizes)
   {
   }

};

cAICreatureSizes g_AICreatureSizes;  // here is the class itself, woo woo

static sFieldDesc _g_AICreatureSizesFieldDesc[] =
{
   { "Creature Type 0: Width",        kFieldTypeFloat,  FieldLocation(sAICreatureSizes, sizes[0].width) }, 
   { "                 Height",       kFieldTypeFloat,  FieldLocation(sAICreatureSizes, sizes[0].height) }, 
#if 0
   { "Creature Type 1: Width",        kFieldTypeFloat,  FieldLocation(sAICreatureSizes, sizes[1].width) }, 
   { "                 Height",       kFieldTypeFloat,  FieldLocation(sAICreatureSizes, sizes[1].height) }, 

   { "Creature Type 2: Width",        kFieldTypeFloat,  FieldLocation(sAICreatureSizes, sizes[2].width) }, 
   { "                 Height",       kFieldTypeFloat,  FieldLocation(sAICreatureSizes, sizes[2].height) }, 

   { "Creature Type 3: Width",        kFieldTypeFloat,  FieldLocation(sAICreatureSizes, sizes[3].width) }, 
   { "                 Height",       kFieldTypeFloat,  FieldLocation(sAICreatureSizes, sizes[3].height) }, 
#endif
};

static sStructDesc _g_AICreatureSizesStructDesc = StructDescBuild(sAICreatureSizes, kStructFlagNone, _g_AICreatureSizesFieldDesc);

///////////////////////////////////////////////////////////////////////////////

void AICreatureSizeInit()
{
   StructDescRegister(&_g_AICreatureSizesStructDesc);
}

///////////////////////////////////////////////////////////////////////////////

