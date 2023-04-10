/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/lnkremap.h,v 1.1 1998/04/29 13:44:35 mahk Exp $
#pragma once  
#ifndef __LNKREMAP_H
#define __LNKREMAP_H

//
// Link ID remapping for load
//
// A simple tool used by link stores to resolve ID conflicts between links. 
//

EXTERN void ClearLinkMappingTable(void); 

EXTERN void AddLinkMappingToTable(LinkID targ, LinkID src); 

EXTERN LinkID LinkRemapOnLoad(LinkID link); 


#endif // __LNKREMAP_H
