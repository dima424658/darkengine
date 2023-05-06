/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: x:/prj/tech/libsrc/sound/RCS/volconv.h 1.1 1999/06/07 16:46:30 mwhite Exp $

//
// Sound volume conversion utility functions.
//

#ifndef VOLCONV_H
#define VOLCONV_H

EXTERN float VolLinearToMillibel (float linearVol);
EXTERN float VolMillibelToLinear (float millibels);

#endif // VOLCONV_H
