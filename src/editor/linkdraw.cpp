/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

////////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/editor/linkdraw.cpp,v 1.7 2000/02/11 14:27:44 bfarquha Exp $
//
// Link-drawing system
//

#include <lg.h>
#include <string.h>

#include <appagg.h>
#include <comtools.h>

#include <dlist.h>
#include <relation.h>
#include <lnkquery.h>
#include <linkbase.h>
#include <linkman.h>

#include <wrtype.h>
#include <objpos.h>

#include <ged_line.h>
#include <linkdraw.h>

#include <editbr_.h>
#include <editbr.h>
#include <brlist.h>
#include <brinfo.h>

// Must be last header
#include <dbmem.h>


class cLinkDraw;

typedef cDList<cLinkDraw, 0>     cLinkDrawList;
typedef cDListNode<cLinkDraw, 0> cLinkDrawNode;

struct sColor
{
   int r, g, b;
   sColor(int rr = 0, int gg = 0, int bb = 0) : r(rr),g(gg),b(bb) {};
};

class cLinkDraw : public cLinkDrawNode
{
public:
   char name[64];
   sColor color;
};


static cLinkDrawList LinkDrawList;

////////////////////////////////////////////////////////////////////////////////

void InitDrawnRelations()
{
   // Hey, we might want to do something here someday...
}

////////////////////////////////////////

void TermDrawnRelations()
{
   cLinkDraw *pLinkDraw;

   pLinkDraw = LinkDrawList.GetFirst();
   while (pLinkDraw != NULL)
   {
      delete LinkDrawList.Remove(pLinkDraw);
      pLinkDraw = LinkDrawList.GetFirst();
   }
}

////////////////////////////////////////

void ResetDrawnRelations()
{
   TermDrawnRelations();
   InitDrawnRelations();
}

////////////////////////////////////////////////////////////////////////////////

static cLinkDraw *GetLinkDrawNode(const char *name)
{
   cLinkDraw *pLinkDraw;

   pLinkDraw = LinkDrawList.GetFirst();
   while (pLinkDraw != NULL)
   {
      if (!strcmp(name, pLinkDraw->name))
         return pLinkDraw;

      pLinkDraw = pLinkDraw->GetNext();
   }

   return NULL;
}

////////////////////////////////////////

void AddDrawnRelation(const char *name, ubyte r, ubyte g, ubyte b)
{
   cLinkDraw *pLinkDraw;

   if (GetLinkDrawNode(name) == NULL)
   {
      pLinkDraw = new cLinkDraw;

      strncpy(pLinkDraw->name, name, 64);
      pLinkDraw->color = sColor(r,g,b);

      LinkDrawList.Append(pLinkDraw);
   }
}

////////////////////////////////////////

void AddDrawnRelationID(RelationID relationID, ubyte r, ubyte g, ubyte b)
{
   AutoAppIPtr_(LinkManager, pLinkMan);

   IRelation *rel = pLinkMan->GetRelation(relationID);

   if (rel != NULL)
      AddDrawnRelation(rel->Describe()->name, r, g, b);
   else
      Warning(("AddDrawnRelationID: Relation id %d not found!\n", relationID));

   SafeRelease(rel);
}

////////////////////////////////////////

void RemoveDrawnRelation(const char *name)
{
   cLinkDraw *pLinkDraw;

   if ((pLinkDraw = GetLinkDrawNode(name)) != NULL)
      delete LinkDrawList.Remove(pLinkDraw);
}

////////////////////////////////////////

void RemoveDrawnRelationID(RelationID relationID)
{
   AutoAppIPtr_(LinkManager, pLinkMan);

   IRelation *rel = pLinkMan->GetRelation(relationID);

   if (rel != NULL)
      RemoveDrawnRelation(rel->Describe()->name);
   else
      Warning(("RemoveDrawnRelationID: Relation id %d not found!\n", relationID));

   SafeRelease(rel);
}

////////////////////////////////////////////////////////////////////////////////

// from brrend.c:
extern "C"
{
   bool pointOBBIntersect(editBrush *point, editBrush *brush);
}

// Determine if link originates inside brush.
static BOOL LinkOriginatesInBrush(sLink *pLink, editBrush *us)
{
   editBrush point;
   point.pos = ObjPosGet(pLink->source)->loc.vec;
   return pointOBBIntersect(&point, us);
}


int g_nLinkDrawSourceId = 0;
int g_nLinkDrawSplitId = 0;


// Is this id a child of g_nLinkDrawSplitId?
// *** We're assuming there are no circular patrols that don't include startid
// To do this right, we'd need to mark previously visited nodes, but that's probably overkill
// given our problem space. Still, let's at least not crash:

static int nCrashDetector;

static BOOL IsChild(ObjID startid, ObjID id, IRelation *pRelation, int nLoopCount = 0)
{
   if (startid==id) // We've gone circular, or are just beginning.
   {
      if (nLoopCount)
      {
         nCrashDetector--;
         return FALSE;
      }
      else
         nLoopCount++;
   }

   if (nCrashDetector > 40)
      return FALSE;

   nCrashDetector++;

   ILinkQuery *query = pRelation->Query(LINKOBJ_WILDCARD, id);

   for (; !query->Done(); query->Next())
   {
      sLink link;
      query->Link(&link);
      if (link.source == g_nLinkDrawSplitId)
      {
         SafeRelease(query);
         return TRUE;
      }
      else if (IsChild(startid, link.source, pRelation, nLoopCount))
      {
         SafeRelease(query);
         return TRUE;
      }
   }

   SafeRelease(query);
   nCrashDetector--;
   return FALSE;
}


// Is this id a parent of g_nLinkDrawSplitId?
// *** We're assuming there are no circular patrols that don't include startid
// To do this right, we'd need to mark previously visited nodes, but that's probably overkill
// given our problem space.
static BOOL IsParent(ObjID startid, ObjID id, IRelation *pRelation, int nLoopCount = 0)
{
   if (startid==id) // We've gone circular, or are just beginning.
   {
      if (nLoopCount)
      {
         nCrashDetector--;
         return FALSE;
      }
      else
         nLoopCount++;
   }

   if (nCrashDetector > 40)
      return FALSE;

   nCrashDetector++;

   ILinkQuery *query = pRelation->Query(id, LINKOBJ_WILDCARD);

   for (; !query->Done(); query->Next())
   {
      sLink link;
      query->Link(&link);
      if (link.dest == g_nLinkDrawSplitId)
      {
         SafeRelease(query);
         return TRUE;
      }
      else if (IsParent(startid, link.dest, pRelation, nLoopCount))
      {
         SafeRelease(query);
         return TRUE;
      }
   }

   SafeRelease(query);
   nCrashDetector--;
   return FALSE;
}


// This is SLOW, but then again, it doesn't need to be fast.
// See if this Obj is anywhere on branching relation.
static BOOL ObjIsOnSplitPath(ObjID id, IRelation *pRelation)
{
   if (id==g_nLinkDrawSplitId)
      return TRUE;

   nCrashDetector = 0;
   if (IsParent(id, id, pRelation))
      return TRUE;

   nCrashDetector = 0;
   if (IsChild(id, id, pRelation))
      return TRUE;

   return FALSE;
}


// Go through and see if any brushes are me-only. If so, only draw links that originate within this brush.
static BOOL LinkIsDrawable(sLink *pLink, IRelation *pRelation)
{
   editBrush *us;
   int hIter;
   BOOL bFoundBrush = FALSE;

   if ((g_nLinkDrawSourceId > 0) && (pLink->source != g_nLinkDrawSourceId))
      return FALSE;

   if ((g_nLinkDrawSplitId > 0) && !ObjIsOnSplitPath(pLink->source, pRelation))
      return FALSE;

   us=blistIterStart(&hIter);
   while ((us!=NULL) && !bFoundBrush)
   {
      if ((brushGetType(us)==brType_HOTREGION) && brHot_IsMEONLY(us))
      {
         bFoundBrush = TRUE;
         blistIterDone(hIter);
      }
      else
         us=blistIterNext(hIter);
   }

   return !bFoundBrush || LinkOriginatesInBrush(pLink, us);
}



void DrawRelations()
{
   AutoAppIPtr_(LinkManager, pLinkMan);
   cLinkDraw *pLinkDraw;

   gedDeleteChannelLines(1<<LINE_CH_LINKS);

   ged_line_load_channel = LINE_CH_LINKS;
   ged_line_view_channels |= (1<<LINE_CH_LINKS);

   pLinkDraw = LinkDrawList.GetFirst();
   while (pLinkDraw != NULL)
   {
      IRelation *pRelation = pLinkMan->GetRelationNamed(pLinkDraw->name);

      if (pRelation != NULL)
      {
         ILinkQuery *query = pRelation->Query(LINKOBJ_WILDCARD, LINKOBJ_WILDCARD);

         for (; !query->Done(); query->Next())
         {
            sLink link;
            query->Link(&link);

            if (LinkIsDrawable(&link, pRelation))
               gedLineAddRGB(&ObjPosGet(link.source)->loc.vec, &ObjPosGet(link.dest)->loc.vec, LINE_FLG_DIR, pLinkDraw->color.r, pLinkDraw->color.g, pLinkDraw->color.b);
         }

         SafeRelease(query);
      }
      else
         Warning(("DrawRelations: relation %s not found!\n", pLinkDraw->name));

      SafeRelease(pRelation);

      pLinkDraw = pLinkDraw->GetNext();
   }
}

////////////////////////////////////////////////////////////////////////////////

BOOL ObjHasDrawnLinks(ObjID objID)
{
   AutoAppIPtr_(LinkManager, pLinkMan);
   cLinkDraw  *pLinkDraw;
   ILinkQuery *query;

   pLinkDraw = LinkDrawList.GetFirst();
   while (pLinkDraw != NULL)
   {
      IRelation *pRelation = pLinkMan->GetRelationNamed(pLinkDraw->name);

      if (pRelation != NULL)
      {
         query = pRelation->Query(objID, LINKOBJ_WILDCARD);

         if (!query->Done())
         {
            SafeRelease(query);
            SafeRelease(pRelation);

            return TRUE;
         }

         SafeRelease(query);

         query = pRelation->Query(LINKOBJ_WILDCARD, objID);

         if (!query->Done())
         {
            SafeRelease(query);
            SafeRelease(pRelation);

            return TRUE;
         }

         SafeRelease(query);
      }
      else
         Warning(("DrawRelations: relation %s not found!\n", pLinkDraw->name));

      SafeRelease(pRelation);

      pLinkDraw = pLinkDraw->GetNext();
   }

   return FALSE;
}



