/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/sound/schfile.h,v 1.2 2000/01/29 13:41:51 adurant Exp $
#pragma once

#ifndef __SCHFILE_H
#define __SCHFILE_H

// function for reading a schema file
typedef void (*fSchemaRead)(char *filename);

// read all files from res path and local dir that match "what" string
void SchemaFilesLoadFromDir(const char *where, const char *what, fSchemaRead readFunc);


#endif // Schfile_h

