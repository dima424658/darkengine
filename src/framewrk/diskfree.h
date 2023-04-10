/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/framewrk/diskfree.h,v 1.1 1998/11/02 03:17:34 dc Exp $

#pragma once

#ifndef __DISKFREE_H
#define __DISKFREE_H

// return free diskspace on root path pointer to by str;
// if str is NULL uses root of current directory.
// if free space > MAX_INT, returns MAX_INT.  (2G)
// if windows call GetDiskFreeSpace fails, returns -1
EXTERN int compute_free_diskspace(char *str);

// checks drive str (using above) messages with windows dialog if under minMB
EXTERN BOOL CheckForDiskspaceAndMessage(char *str, int minMB);

#endif  // __DISKFREE_H
