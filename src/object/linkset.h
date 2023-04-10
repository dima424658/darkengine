/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/linkset.h,v 1.1 1997/10/06 19:05:37 mahk Exp $
#pragma once  
#ifndef __LINKSET_H
#define __LINKSET_H

#include <linktype.h>
#include <listset.h>

////////////////////////////////////////////////////////////
// LINK SET CLASS
//


class cLinkSet : public cSimpleListSet<LinkID> 
{
public: 

   // Spit out a link query
   ILinkQuery* Query(tQueryDate bday = QUERY_DATE_NONE); 
   ILinkQuery* PatternQuery(const struct sLinkTemplate* pattern, tQueryDate bday = QUERY_DATE_NONE); 
};



#endif // __LINKSET_H







