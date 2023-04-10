/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/sound/ambbase.h,v 1.7 2000/01/29 13:41:38 adurant Exp $
// ambient sound wackiness for Dark
// internal definitions
#pragma once

#ifndef  __AMBBASE_H
#define  __AMBBASE_H

#include <label.h>

///////////////
// constants and flags
#define MAX_AMB_OBJS          (256)

// static flags
#define AMBFLG_S_ENVIRON      (1<<0)  // this is environmental
#define AMBFLG_S_NOSHARPCURVE (1<<1)  // dont use the sharper attenuation curve for objs
#define AMBFLG_S_TURNEDOFF    (1<<2)  // the amb is currently not playing

#define AMBFLG_S_REMOVE       (1<<3)  // remove the property (oneshot, basically)
#define AMBFLG_S_MUSIC        (1<<4)  // this is music
#define AMBFLG_S_SYNCH        (1<<5)  // synch on downbeats
#define AMBFLG_S_NOFADE       (1<<6)  // do you want this env sound to not fade
#define AMBFLG_S_KILLOBJ      (1<<7)  // kill the object
#define AMBFLG_S_AUTOOFF      (1<<8)  // turn off after firing once

//////////////
// structures

typedef struct {
   int    rad;          // radius, in feet, of effect
   int    over_volume;  // override volume, or 0 to play at schema default
   ushort flags;        // static flags field, ie. designer set
   ushort pad;       // keep us dword aligned   
   Label  schema_name;  // as you would expect
   Label  aux_schema_1; //   aux schemas to play with the base one
   Label  aux_schema_2; //   wooo-wooo
} AmbientSound;

#endif  // __AMBBASE_H
