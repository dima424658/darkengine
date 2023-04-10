/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#include <lg.h>

#include <linkbase.h>
#include <lnkquery.h>
#include <link.h>
#include <relation.h>
#include <sdesbase.h>
#include <sdesc.h>
#include <simtime.h>

#include <aisuslnk.h>

//must be last header
#include <dbmem.h>

///////////////////////////////////////////////////////////////////////////

static IRelation * g_pAISuspiciousLinks;

///////////////////////////////////////////////////////////////////////////

struct sAISuspiciousLink
{
  ulong TimeSeen;
};

///////////////////////////////////////////////////////////////////////////

static sFieldDesc g_AISuspiciousLinkFieldDesc[] =
{
  { "Time Seen",      kFieldTypeInt,   FieldLocation(sAISuspiciousLink, TimeSeen)},
};

//////////////////////////////////////////////////////////////////////////

static sStructDesc g_AISuspiciousLinkStructDesc = StructDescBuild(sAISuspiciousLink, kStructFlagNone, g_AISuspiciousLinkFieldDesc);

//////////////////////////////////////////////////////////////////////////

void AIInitSuspiciousLinks()
{
  StructDescRegister(&g_AISuspiciousLinkStructDesc);

  sRelationDesc     suspiciouslinkDesc = { "AISuspiciousLink", 0, 0, 0};

  sRelationDataDesc suspiciouslinkDataDesc = { "sAISuspiciousLink", sizeof(sAISuspiciousLink), kRelationDataAutoCreate };
  
  g_pAISuspiciousLinks = CreateStandardRelation(&suspiciouslinkDesc, &suspiciouslinkDataDesc, (kQCaseSetSourceKnown|kQCaseSetBothKnown));
}


///////////////////////////////////////////////////////////////////////

void AITermSuspiciousLinks()
{
  SafeRelease(g_pAISuspiciousLinks);
}

//////////////////////////////////////////////////////////////////////

BOOL AIClearSuspiciousLinks(ObjID viewer, ObjID viewee)
{
   if (viewee == LINKOBJ_WILDCARD)
   {
      LinkDestroyMany(viewer,LINKOBJ_WILDCARD,g_pAISuspiciousLinks->GetID());
      return TRUE;
   }
   else
   {
      LinkID linkID = g_pAISuspiciousLinks->GetSingleLink(viewer,viewee);
      if (!linkID)
         return FALSE;
      else
         LinkDestroy(linkID);
      return TRUE;
   }
}

//////////////////////////////////////////////////////////////////////

BOOL AIGetSingleSuspiciousLink(ObjID viewer, ObjID viewee, LinkID *newlink)
{
  LinkID linkid = g_pAISuspiciousLinks->GetSingleLink(viewer,viewee);
  if (!linkid)
    return FALSE;
  else
    {
      *newlink = linkid;
      return TRUE;
    }
}

/////////////////////////////////////////////////////////////////////

BOOL AIGetSuspiciousLinkTime(LinkID thelink, ulong *thetime)
{
  sAISuspiciousLink *linkdata;
  linkdata = (sAISuspiciousLink *)LinkGetData(thelink);
  
  if (!linkdata)
    return FALSE;
  
  *thetime = linkdata->TimeSeen;
  return TRUE;
}

/////////////////////////////////////////////////////////////////////
BOOL AIIsSuspiciousOfObj(ObjID viewer, ObjID viewee)
{
  LinkID linkid = g_pAISuspiciousLinks->GetSingleLink(viewer,viewee);
  if (!linkid)
    return FALSE;
  else
    return TRUE;
}


/////////////////////////////////////////////////////////////////////
BOOL AICreateSuspiciousLink(ObjID viewer, ObjID viewee)
{
  sAISuspiciousLink linkdata;
  LinkID linkid = g_pAISuspiciousLinks->GetSingleLink(viewer,viewee);
  AssertMsg2((!linkid),"Attempted to create duplicate suspicious link from %d to %d!",viewer,viewee);
  
  linkdata.TimeSeen = GetSimTime();
  linkid = LinkCreateFull(viewer, viewee, g_pAISuspiciousLinks->GetID(), &linkdata);
  if (linkid)
    return TRUE;
  else
    return FALSE;
}
