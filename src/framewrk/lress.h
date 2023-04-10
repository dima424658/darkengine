/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/framewrk/lress.h,v 1.2 2000/01/29 13:21:14 adurant Exp $
// structures and defines for lresname
#pragma once

#ifndef __LRESS_H
#define __LRESS_H

// abstract types of resName parseable resources
#define _RN_RAW     0
#define _RN_IMAGE   1
#define _RN_ANIM    2
#define _RN_PAL     3
#define _RN_SND     4

#define RN_RAW     (1<<_RN_RAW)
#define RN_IMAGE   (1<<_RN_IMAGE)
#define RN_ANIM    (1<<_RN_ANIM)
#define RN_PAL     (1<<_RN_PAL)
#define RN_SND     (1<<_RN_SND)
#define RN_ANY    ((1<<_RN_SND+1)-1)

// Resname Handles
#define NO_HND      LRES_NO_HND

// ResName name structure
#define LRES_TOKENS_PER   4
#define RN_NAME_LEN      15   // @TODO: sed this out of the code
#define LRES_NAME_LEN    15
#define LRES_NO_WHERE    -1

struct _rn_name {
   char  tokens[LRES_TOKENS_PER];   // 4
   char  name[RN_NAME_LEN];         // 19
   char  where;                     // 20: where is -1 if nowhere, else idx of path
};

#endif  // __LRESS_H
