/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

//
//

#ifndef __DPCAIPTH_H
#define __DPCAIPTH_H

#include <aipthfnd.h>

#pragma once
#pragma pack(4)

///////////////////////////////////////////////////////////////////////////////

extern void DPCAITermPathfinder(void);

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cDPCAIPathfinder
//

class cDPCAIPathfinder : public cAIPathfinder 
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

#endif /* !__DPCAIPTH_H */
