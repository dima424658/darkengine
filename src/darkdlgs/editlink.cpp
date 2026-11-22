/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#include "darkdlgs.h"
#include <editlink.h>
#include "edlinkst.h"

static BOOL show_internal_links;

LinkEditor *LinkEditorCreate(ObjID, ObjID, RelationID, LinkEditorDesc *)
{
   DarkDlgsUnavailable("Link editor");
   return NULL;
}

void LinkEditorDestroy(LinkEditor *)
{
}

BOOL LinkEditorShowAllLinks(BOOL newval)
{
   BOOL old = show_internal_links;
   show_internal_links = newval;
   return old;
}
