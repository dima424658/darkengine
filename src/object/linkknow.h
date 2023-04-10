/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/linkknow.h,v 1.2 2000/01/29 13:23:22 adurant Exp $
#pragma once

#ifndef __LINKKNOW_H
#define __LINKKNOW_H
#include <linktype.h>

////////////////////////////////////////////////////////////
// cLinkManagerKnower
//
// Cheesy little class for relations and other things that
// want a quick easy pointer to the link manager.  
// Not for external use. 
//

class cLinkManagerKnower
{
   static ILinkManagerInternal *linkMan;

protected:
   static void SetManager(ILinkManagerInternal* m) { linkMan = m; } ;
   static ILinkManagerInternal* LinkMan(void) { return linkMan;}; 
};

#endif // __LINKKNOW_H


