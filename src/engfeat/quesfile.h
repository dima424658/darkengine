/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/engfeat/quesfile.h,v 1.1 1998/08/12 15:08:24 mahk Exp $
#pragma once  
#ifndef __QUESFILE_H
#define __QUESFILE_H

#include <questapi.H>
////////////////////////////////////////////////////////////
// DUMB QUEST DATA FILE READ
//

EXTERN void QuestDataLoadTagFile(eQuestDataType type, ITagFile* File); 
EXTERN void QuestDataSaveTagFile(eQuestDataType type, ITagFile* File);

#endif // __QUESFILE_H
