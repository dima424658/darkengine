/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/motion/mmnvrfac.h,v 1.2 2000/01/31 09:50:14 adurant Exp $
#pragma once

#ifndef __MMNVRFAC_H
#define __MMNVRFAC_H

// This module contains an array of all the factory methods for creating
// maneuvers

#include <mmanuver.h>

EXTERN cManeuverFactory *g_aManeuverFactoryTable;
EXTERN int g_iNumManeuverFactories;

#endif
