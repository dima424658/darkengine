/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/bintrait.h,v 1.1 1998/07/16 21:33:03 mahk Exp $
#pragma once  
#ifndef __BINTRAIT_H
#define __BINTRAIT_H
#include <comtools.h>

////////////////////////////////////////////////////////////
// Tools for using relations as "binary traits" 
//

F_DECLARE_INTERFACE(ITrait); 
F_DECLARE_INTERFACE(IRelation); 
F_DECLARE_INTERFACE(ILinkQuery); 

//------------------------------------------------------------
// SLOW, UNCACHED VERSIONS
//
//
// Query all links inherited by one or two objs
// 


// All links out from me or my donors to just you, in inheritance order 
// dest may be a wildcard 
EXTERN ILinkQuery* QueryInheritedLinksSingleUncached(IRelation* rel, ObjID src, ObjID dest);  

// All links from me and my donors to you and your donors, in inheritance order
// No wildcards allowed 
EXTERN ILinkQuery* QueryInheritedLinksUncached(IRelation* rel, ObjID src, ObjID dest);  

//------------------------------------------------------------
// CACHED VERSIONS
//

// Make a "trait" for use by these functions.  You should do this once.  
ITrait* MakeTraitFromRelation(IRelation* pRel); 

//
// Query all links inherited by one or two objs
// 

// All links out from me or my donors to just you, in inheritance order 
// dest may be a wildcard 
EXTERN ILinkQuery* QueryInheritedLinksSingle(ITrait* trait, IRelation* rel, ObjID src, ObjID dest);  

// All links from me and my donors to you and your donors, in inheritance order
// No wildcards allowed 
EXTERN ILinkQuery* QueryInheritedLinks(ITrait* trait, IRelation* rel, ObjID src, ObjID dest);  




#endif // __BINTRAIT_H



