/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/traittyp.h,v 1.4 2000/01/29 13:25:17 adurant Exp $
#pragma once

#ifndef __TRAITTYP_H
#define __TRAITTYP_H
#include <objtype.h>
#include <osystype.h>
#include <comtools.h>

////////////////////////////////////////////////////////////
// BASE TYPES FOR TRAITS
//

// 
// COM interfaces
//
F_DECLARE_INTERFACE(ITrait);
F_DECLARE_INTERFACE(ITraitManager);
F_DECLARE_INTERFACE(IObjectQuery);
F_DECLARE_INTERFACE(IDonorQuery);

//
// Hierarchy types
//

typedef long tMetaPropertyPriority;

//
// Trait/query types
//
typedef struct sTraitDesc sTraitDesc;
typedef ulong eTraitQueryType;
typedef struct sTraitPredicate sTraitPredicate;

//
// Hierarchy listener types
//

typedef struct sHierarchyMsg sHierarchyMsg; 
typedef void* HierarchyListenerData;
typedef void (LGAPI* HierarchyListenFunc)(const sHierarchyMsg* msg, HierarchyListenerData data); 


#endif // __TRAITTYP_H


