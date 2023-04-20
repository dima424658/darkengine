/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/filemenu.h,v 1.2 2000/01/29 13:12:03 adurant Exp $
#pragma once

#ifndef __FILEMENU_H
#define __FILEMENU_H

////////////////////////////////////////////////////////////
// FILE GUI API
//

//
// File dialog open modes
//

enum eFileMenuMode 
{
   kFileMenuLoad,
   kFileMenuSave,
};

typedef enum eFileMenuMode eFileMenuMode;

//
// filter for file names
//

struct sFileMenuFilter 
{
   const char* text;  // text filter description
   const char* pattern; // wildcarded file patter
};

typedef struct sFileMenuFilter sFileMenuFilter;

// 
// put up a file dialog, returns the selected file name
// takes a null-terminated list of filters
//

EXTERN char* FileDialog(eFileMenuMode mode, sFileMenuFilter* pFilters, const char* defaultExtension, char* fnamebuf, int buflen);

//
// Bring up a little menu of load and save buttons
// 

EXTERN void FileMenu(void);


#endif // __FILEMENU_H
