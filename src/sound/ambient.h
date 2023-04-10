/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/sound/ambient.h,v 1.6 2000/01/29 13:41:39 adurant Exp $
// ambient sound object initial cut
#pragma once

#ifndef __AMBIENT_H
#define __AMBIENT_H

// init run time data structures - ie. clear out arrays and stuff
EXTERN void AmbientRunTimeInit(void);

// this runs a frame of ambient object self update
// should be called once a frame, with head set right
// this might make more sense to query the head
// it is all kinda temporary at the moment
EXTERN void AmbientRunFrame(struct mxs_vector *head_pos);

// reset, clears out handles
EXTERN void AmbientReset();

// for saving & loading in-game
EXTERN BOOL AmbientSave(ITagFile *file);
EXTERN BOOL AmbientLoad(ITagFile *file);

// there is no term yet
// if you want to term, for now, you have to term all schemas, i guess
// this probably needs to be fixed

#ifdef PLAYTEST
EXTERN void AmbientDrawRadii(BOOL all);
#endif

#endif  // __AMBIENT_H
