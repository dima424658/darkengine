/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/motion/crettype.h,v 1.6 2000/01/31 09:50:08 adurant Exp $
#pragma once

#ifndef __CRETTYPE_H
#define __CRETTYPE_H

typedef struct sCreatureHandle sCreatureHandle;

typedef struct sCreatureAttachInfo sCreatureAttachInfo;

typedef struct sCrPhysModOffset sCrPhysModOffset;

typedef struct sCrPhysModOffsetTable sCrPhysModOffsetTable;

typedef struct sCreatureDesc sCreatureDesc;

#ifdef __cplusplus
typedef class cCreature cCreature;

typedef class cCreatureFactory cCreatureFactory;
#endif

#define kCrWeap_Invalid -1 // invalid weapon type
#define kCreatureTypeInvalid -1

#endif
