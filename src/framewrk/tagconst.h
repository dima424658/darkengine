/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/framewrk/tagconst.h,v 1.2 2000/01/31 09:48:45 adurant Exp $
#pragma once

#ifndef __TAGCONST_H
#define __TAGCONST_H

#define TAG_CONSTRAINT_NULL -1

enum eTagFlags_ {kTagSet = 0x001,};

enum eTagType_ {kTagInt, kTagFloat, kTagLabel,};

enum eTagOp_ {kTagValueEq, kTagValueLess, kTagValueGreater,}; 

#endif
