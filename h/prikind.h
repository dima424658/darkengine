///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/hsrc/RCS/prikind.h $
// $Author: TOML $
// $Date: 1996/07/18 17:07:11 $
// $Revision: 1.2 $
//
// Declarations of prioritization enum
//

#ifndef __PRIKIND_H
#define __PRIKIND_H

///////////////////////////////////////
//
// 8 bit priority value
//

enum ePriorityLevel
    {
    // General terminology
    kPriorityReserved   = 0x0001,
    kPriorityHighest    = 0x1000,
    kPriorityHigh       = 0x2000,
    kPriorityNormal     = 0x4000,
    kPriorityLow        = 0x6000,
    kPriorityLowest     = 0x8000,

    // Specific terms
    kPriorityLibrary    = kPriorityHighest,
    kPriorityAppLibrary = kPriorityHigh,
    kPriorityApp        = kPriorityNormal,
    kPriorityDontCare   = kPriorityNormal
    };

#define ComparePriorities(p1, p2) ((int)((p1) - (p2)))

typedef enum ePriorityLevel ePriorityLevel;

///////////////////////////////////////

#endif /* !__PRIKIND_H */
