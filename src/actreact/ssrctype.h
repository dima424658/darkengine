/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/actreact/ssrctype.h,v 1.3 2000/01/29 12:44:45 adurant Exp $
#pragma once

#ifndef __SSRCTYPE_H
#define __SSRCTYPE_H

#include <comtools.h>

////////////////////////////////////////////////////////////
// FORWARD-DECLARED TYPES FOR STIMULUS SOURCES
//


//
// Scayluhs
//

typedef ulong StimSourceID; // Unique stimulus source ID
#define SRCID_NULL 0


//
// Strukchuhs
//

typedef struct sStimSourceDesc sStimSourceDesc; // source descriptor

//
// Intuhfaces
//

F_DECLARE_INTERFACE(IStimSources);
F_DECLARE_INTERFACE(IStimSourceQuery);





#endif // __SSRCTYPE_H
