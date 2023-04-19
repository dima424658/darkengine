/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: x:/prj/tech/libsrc/gadget/RCS/gadint.h 1.1 1998/07/02 13:20:25 PATMAC Exp $

// gadget lib internals - should be needed only be folks who
//   are implementing their own gadgets

#ifndef __GADINT_H
#define __GADINT_H

#ifndef __GADGET_H
#include <gadget.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif
extern void LGadComputeVarSize(LGadToggle *vt, short *wp, short *hp);
extern void LGadComputeSize(LGadButton *vbutt, short *wp, short *hp);
extern void LGadInitToggle(LGadToggle *vt);
#ifdef __cplusplus
}
#endif

#endif

