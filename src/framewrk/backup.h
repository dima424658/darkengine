/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/framewrk/backup.h,v 1.4 1998/08/02 00:24:50 mahk Exp $
#pragma once  
#ifndef __BACKUP_H
#define __BACKUP_H

//
// Simple system for backing up the sim state, intended for use by the editor on the way
// into/out of game mode
//

EXTERN void InitBackup(void);
EXTERN void TermBackup(void); 

EXTERN void BackupMission(void); 
EXTERN void RestoreMissionBackup(void); 
EXTERN void RemoveMissionBackup(void); 
EXTERN BOOL BackupAvailable(void); 

EXTERN BOOL BackupLoading(void); // is this database load a backup load
EXTERN BOOL BackupSaving(void); // is this database save a backup save 

#endif // __BACKUP_H


