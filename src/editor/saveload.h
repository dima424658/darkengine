/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/saveload.h,v 1.8 2000/01/29 13:13:08 adurant Exp $
// a few simple saveload system functions
// in particular, too minimize code duplication/bit rot across our saveload in cow and bl and stuff
#pragma once

#ifndef __SAVELOAD_H
#define __SAVELOAD_H

typedef struct {
   char  user[16];     // last user to save this level
   char  creator[16];  // first user to save the level
   char  fullname[64]; // fancy level name from the "fancy name" dialog
} levelNameInfo;

EXTERN levelNameInfo saveloadLevelInfo;

// actually load or read brushes, use read/write callback as appropriate
//   num_brush can be set to SAVELOAD_NO_COUNT if you want to read as many as you can find
//   size is the size of the currently to be written brush, based on brType of br
EXTERN bool saveloadReadBrushes(bool (*readnext)(editBrush *newbr), int num_brush, BOOL preserve_id);
EXTERN bool saveloadWriteBrushes(bool (*writebr)(editBrush *br, int size), int groupFilter);
EXTERN BOOL saveloadPostLoadBrushes(void);

// reads contiguous model strings in from the file,
//   md_cnt is number to read, name_len is strlen of each name
//   callback should return FALSE if it cant read/write the model
EXTERN bool saveloadReadModels(bool (*readnext)(char *name, int len), int md_cnt, int str_len);
EXTERN bool saveloadWriteModels(bool (*writenxt)(char *name, int len), int md_cnt, int str_len);

// get the current username from the environment, either config file or getenv
EXTERN char *saveloadGetUser(void);

#define SAVELOAD_NO_COUNT (-1)

EXTERN void saveloadZeroAllGroups(void);        // zero all groups
EXTERN void saveloadCleanVer1File(void);        // zap all fields of a ver1 file from 0xDD
EXTERN void saveloadFixAlignmentFields(void);   // fix all texture alignment fields in terrain brushes
EXTERN void saveloadFixIntPrimalIDs(void);      // fix old primal ids from old headers
EXTERN void saveloadFixNonTerrPrimalIDs(void);  // deal with non-terrain brushes with zany primal ids
EXTERN void saveloadFixFlagField(void);         // zero all flag fields

#endif // __SAVELOAD_H
