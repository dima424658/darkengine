/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/framewrk/dbfile.h,v 1.10 2000/01/29 13:20:34 adurant Exp $
#pragma once

#ifndef __DBFILE_H
#define __DBFILE_H
#include <dbtype.h>

//
// Functions for saving and loading
//
// both use the save_dir config variable 
// Load uses the load_path variable as well.
//
// Takes the mask of which database components to save/load, 
// returns the mask of the ones successfully saved/loaded
// 
// (see dbtype.h)
//



EXTERN edbFiletype dbSave(const char* filename, edbFiletype filemask);  // Save the database
EXTERN edbFiletype dbLoad(const char* filename, edbFiletype filemask);  // Load the database

EXTERN edbFiletype dbMergeLoad(const char* filename, edbFiletype filemask);  // Merge-in a database. 

//
// Find a file in the db path
//
EXTERN BOOL dbFind(const char* filename, char* filebuf); 

//
// Fill out the buffer with the current filename
//
EXTERN void dbCurrentFile(char *buf, int len);
EXTERN void dbSetCurrentFile(char *buf);

//
// Get the current file number since the last reset
//
EXTERN int dbCurrentFilenum();

//
// Reset the database
//

EXTERN void dbReset(void); 

//
// Build the "default" database
//

EXTERN void dbBuildDefault(void);

#define DB_FILE_SUFFIX     "COW"
#define DB_MISSION_SUFFIX  "MIS"
#define DB_MAP_SUFFIX      "MAP"
#define DB_GAMESYS_SUFFIX  "GAM"
#define DB_FILE_DIR_VAR    "save_dir" 

EXTERN void dbfile_setup_commands(void);


#endif // __DBFILE_H




