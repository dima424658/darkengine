/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/shock/shkaipth.h,v 1.2 1999/05/28 20:30:29 JON Exp $
//
//
//

#ifndef __SHKAIPTH_H
#define __SHKAIPTH_H

#include <aipthfnd.h>

#pragma once
#pragma pack(4)

///////////////////////////////////////////////////////////////////////////////

extern void ShockAITermPathfinder(void);

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cShockAIPathfinder
//

class cShockAIPathfinder : public cAIPathfinder 
{
public:   
   STDMETHOD_(const char *, GetName)();
   //
   // IAIPathfindControl methods
   //
   STDMETHOD_(BOOL, CanPassDoor)(ObjID door);

private:
   BOOL DoorOpenable(ObjID objID, int depth, BOOL& openable);
   BOOL QuestBlock(ObjID objID) const;
};

///////////////////////////////////////////////////////////////////////////////

#pragma pack()

#endif /* !__SHKAIPTH_H */
