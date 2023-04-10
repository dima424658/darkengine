///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/compapis/RCS/movconst.h $
// $Author: JON $
// $Date: 1996/09/19 14:58:16 $
// $Revision: 1.1 $
//
// Constants to allow outside functions to access movie player flags
// without having to include movieapi.h and related windows headers
//

#ifndef __MOVCONST_H
#define __MOVCONST_H

// Play movie synchronously
#define kMoviePlayBlock         0x01
// Don't stop the movie on a keystroke or mouse click
#define kMoviePlayNoStop        0x02
// Play the movie using blank horizontal scan lines
#define kMovieBlankLines        0x04

#endif

