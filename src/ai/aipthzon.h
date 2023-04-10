/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aipthzon.h,v 1.3 2000/01/29 12:45:46 adurant Exp $
//
// AI zone-determination header
//
#pragma once

#ifndef __AIPTHZON_H
#define __AIPTHZON_H

#define MAX_AI_PATH_ZONES (65535)

#define AI_ZONE_INVALID   (0)
#define AI_ZONE_SOLO      (65535)
#define AI_ZONE_ALL       (65534)


EXTERN void ResetPathfindZones();
EXTERN void DeterminePathfindZones();
EXTERN void LinkPathfindZones();
EXTERN void FixupPathfindZones();

#endif
