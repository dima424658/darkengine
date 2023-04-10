/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/lnkquery.cpp,v 1.4 1998/10/05 17:26:29 mahk Exp $
#include <lnkquer_.h>
#include <linkint.h>

// must be last header
#include <dbmem.h>

IMPLEMENT_UNAGGREGATABLE_SELF_DELETE(cUnknownLinkQuery,ILinkQuery);

cBaseLinkQuery::cBaseLinkQuery(tQueryDate birthday)
   :Birthday(birthday),locked(FALSE)
{
   if (Birthday == QUERY_DATE_NONE)
   {
      Birthday = LinkMan()->Lock();
      locked = TRUE;
   }
}

cBaseLinkQuery::~cBaseLinkQuery()
{
   if (locked)
      LinkMan()->Unlock(Birthday);
}

////////////////////////////////////////

IMPLEMENT_DELEGATION(cDelegatedLinkQuery);



