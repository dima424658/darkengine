/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/lnkdtabl.h,v 1.3 2000/01/29 13:23:42 adurant Exp $
#pragma once

#ifndef __LNKDTABL_H
#define __LNKDTABL_H
#include <linktype.h>
#include <hashset.h>

struct sLinkDataTableElem
{
   LinkID id;
   char data[];
};

class LinkDataTable : public cHashSet<sLinkDataTableElem *,LinkID,cHashFunctions>
{
public:
   LinkDataTable() {};
   tHashSetKey GetKey(tHashSetNode node) const
   {
      return (tHashSetKey)(((sLinkDataTableElem*)node)->id);
   }
};

#endif // __LNKDTABL_H
