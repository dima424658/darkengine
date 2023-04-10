/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/render/antxscpt.h,v 1.2 2000/01/29 13:38:31 adurant Exp $
#pragma once

#ifndef __ANTXSCPT_H
#define __ANTXSCPT_H

#include <scrptsrv.h>
#include <scrptmsg.h>
#include <engscrpt.h>

DECLARE_SCRIPT_SERVICE(AnimTexture, 0x16a)
{
   STDMETHOD(ChangeTexture)(object refobj, const char *fam1, const char *tx1, 
      const char *fam2, const char *tx2) PURE;
};

#endif