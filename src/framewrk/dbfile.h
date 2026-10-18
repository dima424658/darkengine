/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/framewrk/dbfile.h,v 1.10 2000/01/29 13:20:34 adurant Exp $
#pragma once

#ifndef __DBFILE_H
#define __DBFILE_H
#include <dbtype.h>
#include <tagfile.h>

typedef enum eDBCompatibilityStatus
{
   kDBCompatDecoded,
   kDBCompatPrefixDecoded,
   kDBCompatRawPreserved,
   kDBCompatUnsupported,
} eDBCompatibilityStatus;

// Database output is selected independently from the versions a codec can
// read.  Preserve-source is the normal editor policy; the other profiles are
// explicit export/exchange policies and must never be inferred merely from a
// newer chunk being present in the runtime database.
typedef enum eDBWriteProfile
{
   kDBWritePreserveSource,
   kDBWriteRetail,
   kDBWriteNewDark,
   kDBWriteKCToolsExchange,
} eDBWriteProfile;

// Codec-facing compatibility hooks.  The database layer owns inventory,
// reporting, and DromEd raw preservation; codecs own version acceptance.
EXTERN void dbCompatibilityMarkTag(const TagFileTag* tag,
                                   eDBCompatibilityStatus status,
                                   const char* detail);
EXTERN void dbCompatibilityRetainRawTag(const TagFileTag* tag,
                                        const char* detail);
EXTERN BOOL dbCompatibilityUseRawTag(const TagFileTag* tag);
EXTERN void dbCompatibilityRewriteTag(const TagFileTag* tag,
                                      const char* detail);
EXTERN const void* dbCompatibilityGetRawTag(const TagFileTag* tag,
                                            ulong* size,
                                            TagVersion* version);
EXTERN void dbCompatibilityPreserveRecordTail(const TagFileTag* tag,
                                              int object,
                                              const void* bytes,
                                              ulong size,
                                              ulong recordSize);
EXTERN const void* dbCompatibilityGetRecordTail(const TagFileTag* tag,
                                                int object,
                                                ulong* size,
                                                ulong* recordSize);

// Per-chunk source-format inventory and output-profile selection.  A frozen
// target survives an internal tool round trip (for example KCTools) so that
// importing a temporary exchange COW does not silently change Save's target.
EXTERN eDBWriteProfile dbFormatGetWriteProfile(void);
EXTERN eDBWriteProfile dbFormatSetWriteProfile(eDBWriteProfile profile);
EXTERN BOOL dbFormatSourceTag(const TagFileTag* tag,
                              TagVersion* version,
                              ulong* size);
EXTERN const void* dbFormatSourceRawTag(const TagFileTag* tag,
                                        ulong* size,
                                        TagVersion* version);
EXTERN BOOL dbFormatTargetHasTag(const TagFileTag* tag);
EXTERN BOOL dbFormatUsesNewDark(void);
EXTERN void dbFormatFreezeSourceTarget(BOOL freeze);

// Decoder-facing mission-load failure channel.  Readers should use this for
// malformed or unsupported data that is required to construct a coherent
// database.  The outer load transaction logs the first failure and unwinds to
// a valid empty database instead of allowing partially initialized state to
// reach post-load or gameplay.
EXTERN void dbLoadReportFailure(const char* subsystem, const char* detail);
EXTERN BOOL dbLoadHasFailed(void);
EXTERN const char* dbLoadFailureReason(void);

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




