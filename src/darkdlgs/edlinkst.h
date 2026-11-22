/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/edlinkst.h,v 1.4 2000/01/29 13:11:53 adurant Exp $
#pragma once

#ifndef __EDLINKST_H
#define __EDLINKST_H

#include <edlnktyp.h>

//
// Link Editor Flags (part of the editor descriptor)
//

enum eLegacyLinkEditorFlags
{
   kLegacyLinkEditDoneButton   = (1 << 0),
   kLegacyLinkEditCancelButton = (1 << 1),
   kLegacyLinkEditApplyButton  = (1 << 2),
};

//
// Link Editor Descriptor
//

struct LinkEditorDesc
{
   char title[32]; // Title string appearing at top of gadget;
   eLinkEditorFlags flags;
};



#endif // __EDLINKST_H
