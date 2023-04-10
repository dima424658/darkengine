/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/engfeat/quest.h,v 1.2 2000/01/29 13:19:59 adurant Exp $
#pragma once

#ifndef __QUEST_H
#define __QUEST_H

typedef void (*QuestMoveFunc) (void *buf, size_t elsize, size_t nelem);

EXTERN void QuestSave(QuestMoveFunc moveFunc);
EXTERN void QuestLoad(QuestMoveFunc moveFunc);

#endif