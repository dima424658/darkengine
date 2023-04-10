/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/motion/mskltype.h,v 1.5 2000/01/31 09:49:53 adurant Exp $
#pragma once

#ifndef __MSKLTYPE_H
#define __MSKLTYPE_H

#ifdef __cplusplus

// forward declarations of mskill data

//typedef struct sMSkill sMSkill;
typedef class cMSkill cMSkill;

typedef char *tMSkillIdentifier;
typedef cMSkill *tMSkillHandle;

#endif // cplusplus


typedef struct sMSkillDesc sMSkillDesc;
typedef struct sMSkillPhysDesc sMSkillPhysDesc;
typedef struct sMGaitSkillData sMGaitSkillData;

#endif
