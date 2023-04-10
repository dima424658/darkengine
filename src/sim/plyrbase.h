/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/sim/plyrbase.h,v 1.2 2000/01/31 10:00:21 adurant Exp $
#pragma once

#ifndef __PLYRBASE_H
#define __PLYRBASE_H

#include <plyrtype.h>
#include <label.h>

typedef enum
{
   kPMS_Immediate,
   kPMS_WhenReady,
} ePlayerModeSwitchUrgency;

typedef enum ePlayerInput
{
   kPlyrInput_None,
   kPlyrInput_Start,
   kPlyrInput_Finish,
   kPlyrInput_Abort,
   kPlyrNumInputs,
   kPlyrInput_Invalid=0xff,
} ePlayerInput;

typedef enum ePlayerTransition
{
   kTrans_Immediate,
   kTrans_AtEnd,
   kTrans_Invalid=0xff,
} ePlayerTransition;

#endif
