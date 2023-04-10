/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/motion/skillset.h,v 1.3 2000/01/31 09:50:00 adurant Exp $
// interface to get information about an object's skill abilities
#pragma once

#ifndef __SKILLSET_H
#define __SKILLSET_H

#include <objtype.h>
#include <skilltyp.h>

BOOL SkillSetGetForObj(const ObjID objID, int *nSkills, tSkillID **ppSkills);



#endif
