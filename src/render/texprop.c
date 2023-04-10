/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/render/texprop.c,v 1.9 2000/02/19 12:35:40 toml Exp $
// silly texture property config file craziness for EEE
// this is _not_ shipping code

#include <string.h>

#include <lg.h>
#include <mprintf.h>
#include <config.h>

#include <texprop.h>
#include <iobjsys.h>
#include <appagg.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

// self lit idiocy
BOOL terr_self_lit[TEXMEM_MAX];
#define SELFLITPROP_BASESTR "selflit_"

// Now terrainprop stuff
#define TERRPROP_BASESTR "terrprop_"
#define TERRPROP_LEN     32
// max is in texprop.h
#define NUM_TERR          6 // fields in the struct, really

// the names of each terrain prop schema, ie. terr_schema[i] is terrain i's footfall schema
static char terr_schema[MAX_TERRPROP][TERRPROP_LEN];
int terr_schema_ids[MAX_TERRPROP];  // swizzled schema values for playback

// Texture property stuff stripped out -- it was all horribly obsolete, and
// Mahk indicated that this whole module should go away soon...

// this is all somewhat broken, really
// for terrains 0-31 loads the name for the schema they use
void terrainprop_load(void)
{
   IObjectSystem *objSys=AppGetObj(IObjectSystem);
   int i;

   for (i=0; i<MAX_TERRPROP; i++)
   {
      char varname[64];
      sprintf(varname,"%s%2.2d",TERRPROP_BASESTR,i);
      if (config_get_raw(varname,terr_schema[i],TERRPROP_LEN))
      {  // would be nice to swizzle, error check, and integer code them here
         ObjID our_schema=IObjectSystem_GetObjectNamed(objSys,terr_schema[i]);
         terr_schema_ids[i]=our_schema;
      }
      else
      {
         strcpy(terr_schema[i],"");
         terr_schema_ids[i]=0;
      }
//      mprintf("TerrProp %s: %s (%d)\n",varname,terr_schema[i],terr_schema_ids[i]);
   }
}
