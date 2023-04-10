/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/ged_rmap.c,v 1.2 2000/02/19 13:10:47 toml Exp $
// support for remapping the brush ids

#include <string.h>
#include <lg.h>

#include <editbr.h>
#include <brlist.h>
#include <brinfo.h>

#include <csg.h>
#include <csgbrush.h>
#include <ged_rmap.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

#define MAX_CALLBACKS 16

static ged_remap_callback remap_callbacks[MAX_CALLBACKS];

BOOL ged_remap_add_callback(ged_remap_callback cb)
{
   int i;
   for (i=0; i<MAX_CALLBACKS; i++)
      if (remap_callbacks[i]==NULL)
      {
         remap_callbacks[i]=cb;
         return TRUE;
      }
   return FALSE;
}

BOOL ged_remap_rem_callback(ged_remap_callback cb)
{
   int i;
   for (i=0; i<MAX_CALLBACKS; i++)
      if (remap_callbacks[i]==cb)
      {
         remap_callbacks[i]=NULL;
         return TRUE;
      }
   return FALSE;
}

// @TODO: make this have an installable callback for all systems who care
// compress brush id space, build new csg_brush list
int ged_remap_brushes(void)
{
   int newid=1, hIter, i, max_id=0;
   editBrush *us;
   int *mapping;

   us=blistIterStart(&hIter);
   while (us!=NULL)
   {  // check brush_id
      if (us->br_id>max_id) max_id=us->br_id;
      us=blistIterNext(hIter);
   }
   AssertMsg(max_id<MAX_CSG_BRUSHES,"Too Many Brushes in Level");
   mapping=(int *)Malloc(sizeof(int)*MAX_CSG_BRUSHES);
   memset(mapping,0xff,sizeof(int)*MAX_CSG_BRUSHES);
   us=blistIterStart(&hIter);
   while (us!=NULL)
   {  // check brush_id
      editBrush *nxt=blistIterNext(hIter);
      if (us->br_id<MAX_CSG_BRUSHES)
      {
         mapping[us->br_id]=newid;     // this mapping is 
         csg_brush[newid]=us;
         if (us->br_id>newid)          // if under newid, we have already remapped
            csg_brush[us->br_id]=NULL; //   so dont step on it
         us->br_id=newid++;
      }
      else
      {
         Warning(("Punting brush %d\n",us->br_id));
         // should somehow delete it, eh?
      }
      us=nxt;
   }
   for (i=0; i<MAX_CALLBACKS; i++)
      if (remap_callbacks[i]!=NULL)
         (*remap_callbacks[i])(mapping);
   remap_csg_database(mapping);  // this will TRASH mapping
   // @TODO: wake up the GFH
   Free(mapping);
   return newid;   // new max
}

