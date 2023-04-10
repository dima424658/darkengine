/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkspflg.h,v 1.2 2000/01/31 09:59:16 adurant Exp $
#pragma once

#ifndef __SHKSPFLG_H
#define __SHKSPFLG_H

typedef enum
{
   kSpawnFlagNone = 0x0,
   kSpawnFlagPopLimit = 0x1,
   kSpawnFlagPlayerDist = 0x2,

   kSpawnFlagDefault = kSpawnFlagPopLimit|kSpawnFlagPlayerDist,
   kSpawnFlagAll  = 0xFFFF,
};

#endif