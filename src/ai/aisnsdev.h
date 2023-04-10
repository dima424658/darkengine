/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aisnsdev.h,v 1.1 1998/08/05 16:15:41 JON Exp $
//
// Device senses
//

#ifndef __AISNSDEV_H
#define __AISNSDEV_H

#include <aibassns.h>

#pragma once
#pragma pack(4)

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIDeviceSenses
//

class cAIDeviceSenses : public cAISenses
{
private:
   virtual floatang GetVisionFacing() const;
};

///////////////////////////////////////////////////////////////////////////////

#pragma pack()

#endif /* !__AISNSDEV_H */
