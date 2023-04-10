/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/libsrc/gadget/gadscale.h,v 1.4 1996/12/04 15:56:26 xemu Exp $

#ifndef __GADSCALE_H
#define __GADSCALE_H

#include <gadbutt.h>

////////////////////////////////////////////////////////////
// SCALE GADGET
// 
// A scale represents a range of values as a row of bright dots.
// Sort of like an LED bar graph. 
//

//------------------------------------------------------------
// STRUCTURE DEFINITION 
//

typedef struct {
   TOGGLE_GUTS;
   short lit_color;
   short dim_color;
   short dot_h;
   short dot_w;
   short dot_margin;
   short internal_w;
   ushort scale_flags;
} LGadScale;

//------------------------------------------------------------
// ACCESSORS
// 

#define LGadScaleLitColor(b)    (((LGadScale*)b)->lit_color        )
#define LGadScaleDimColor(b)    (((LGadScale*)b)->dim_color        )
#define LGadScaleDotWid(b)      (((LGadScale*)b)->dot_w            )
#define LGadScaleDotHgt(b)      (((LGadScale*)b)->dot_h            )
#define LGadScaleDotMargin(b)      (((LGadScale*)b)->dot_margin            )
#define LGadScaleFlags(b)       (((LGadScale*)b)->scale_flags)
#define LGadScaleIntWid(b)      (((LGadScale*)b)->internal_w)

#define LGadScaleState(b)       LGadToggleVal(b)
#define LGadScaleMax(b)         LGadToggleMax(b)
#define LGadScaleIncrem(b)      LGadToggleIncrem(b)

// Not sure you really need mutators, but what the heck

#define LGadScaleSetLitColor(b,v)    (((LGadScale*)b)->lit_color       = (v))
#define LGadScaleSetDimColor(b,v)    (((LGadScale*)b)->dim_color       = (v))
#define LGadScaleSetDotWid(b,v)      (((LGadScale*)b)->dot_w           = (v))
#define LGadScaleSetDotHgt(b,v)      (((LGadScale*)b)->dot_h           = (v))
#define LGadScaleSetDotMargin(b,v)      (((LGadScale*)b)->dot_margin   = (v))

#define LGadScaleSetState(b,v)       LGadToggleSetVal(b,v)
#define LGadScaleSetMax(b,v)         LGadToggleSetMax(b,v)
#define LGadScaleSetIncrem(b,v)      LGadToggleSetIncrem(b,v)

//------------------------------------------------------------
// SCALE GADGET FUNCTIONS
//

// create a scale, from a mostly-filled-in structure. (outdated)
EXTERN LGadScale *LGadCreateScale(LGadScale *vs, LGadRoot *vr, short x, short y, short w, short h, char paltype);


// create a scale from scratch
EXTERN LGadScale *LGadCreateScaleArgs(LGadScale *vs, LGadRoot *vr, short x, short y, short w, short h, 	
   DrawElement *draw, LGadButtonCallback bfunc, int *val_ptr, short max_val, 
   short increm, short lit_color, short dim_color, short dot_w, short dot_h, short dot_margin, 
   ushort scale_flags, short internal_w, char paltype);

#define SCALE_NORMAL            0
#define SCALE_NOTOGGLE          0x1
#define SCALE_TITLELEFT         0x2
#define SCALE_READONLY          0x4

#endif // __GADSCALE_H

