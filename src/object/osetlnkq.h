/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/osetlnkq.h,v 1.1 1997/09/26 16:02:55 mahk Exp $
#pragma once  
#ifndef __OSETLNKQ_H
#define __OSETLNKQ_H

#include <comtools.h>
#include <objquery.h>
#include <lnkquery.h>

F_DECLARE_INTERFACE(IRelation);


//////////////////////////////////////////////////////////////
// OBJECT SET LINK QUERY
//
// Basically, let F(obj) be a factory that generates a link query based on an object
// Let S be the set of objects yielded by some IObjectQuery.  We want the concatenation
// F(s1) || F(s2) || ...  ||  F(sn)  forall sx in S.   
//

//
// The factory class, F(x)
//

class cLinkQueryFactory 
{
public:
   virtual ~cLinkQueryFactory() {};

   virtual ILinkQuery* Query(ObjID obj) = 0;
};

//
// Query creator 
//
extern ILinkQuery* CreateObjSetLinkQuery(IObjectQuery* objset, cLinkQueryFactory* fact); 

//
// Some useful factories 
//

//
// Source set query 
//
// For some constant relation R and object O 
//
//  F_R_o(x) == R->Query(x,o)
// 
extern cLinkQueryFactory* CreateSourceSetQueryFactory(IRelation* R, ObjID o);

//
// Dest set query 
//
// For some constant relation R and object O 
//
//  F_R_o(x) == R->Query(o,x)
// 
extern cLinkQueryFactory* CreateDestSetQueryFactory(IRelation* R, ObjID o);


#endif // __OSETLNKQ_H
