/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/txtpnp.h,v 1.4 2000/01/29 13:13:18 adurant Exp $
#pragma once

#ifndef __TXTPNP_H
#define __TXTPNP_H

// returns TXTPNP_NO_PAL_UP if no palette up, else returns currently chosen texture
EXTERN int txtPnP_pal_query(void);
#define TXTPNP_NO_PAL_UP (-2)

// is it allowed to do attaches?  used by txtrpal and such
EXTERN BOOL txtPnP_allow_attach;

// actually query and control attach/detach state
EXTERN BOOL txtPnP_attached(void);
EXTERN void txtPnP_detach(void);
EXTERN void txtPnP_attach(void);

#endif  // __TXTPNP_H
