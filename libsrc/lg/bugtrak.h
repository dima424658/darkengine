#ifndef __BUGTRAK_H
#define __BUGTRAK_H

/*
 * $Source: r:/prj/lib/src/lg/rcs/bugtrak.h $
 * $Revision: 1.4 $
 * $Author: xemu $
 * $Date: 1993/11/15 19:33:18 $
 *
 * $Log: bugtrak.h $
 * Revision 1.4  1993/11/15  19:33:18  xemu
 * different params to init
 * 
 * Revision 1.3  1993/05/12  12:59:38  xemu
 * new params to init
 * 
 * Revision 1.2  1993/05/01  20:38:35  xemu
 * recommenting
 * 
 * Revision 1.1  1993/05/01  20:28:40  xemu
 * Initial revision
 * 
 * Revision 1.1  1993/04/05  16:35:21  xemu
 * Initial revision
 * 
 *
 */

// Includes
#include "lg.h"  // every file should have this
//#include "error.h"
#include "array.h"

// C Library Includes

// System Library Includes

// Master Game Includes

// Game Library Includes

// Game Object Includes


// Defines
typedef errtype (*BugFunc)(void);

#define LONG_FUNC    0x01
#define SHORT_FUNC   0x02

// Prototypes

// Initializes the bugtrak system, sets up the session file, checks all the environment variables, etc.
errtype bugtrak_init(int max_systems, char *fname, BugFunc bug_report_callback);

// Registers a function with the bugtrak system.  short_proc or long_proc will get called when the bugtrak needs
// to generate system information.  id will get set to the bugtracking id used to just call that one system's
// bugtrak functions.
errtype bugtrak_register(BugFunc short_proc, BugFunc long_proc, int *id);

// Generates a report for that bugtracking system identified by id, of length determined by length (long or short)
// by calling the registered function for that system.  Note that most of the work needs to actually be done by 
// the functions that the bugtracking library calls.
errtype bugtrak_generate(int id, int length);

// Like bugtrak_generate, only gets called for all registered systems.
errtype bugtrak_generate_all(int length);

// Shuts down the bugtrak system, deallocates memory, and indicates the end of the session in the
// session file.
errtype bugtrak_shutdown(int status_code);

// Lets the user generate a bug report from inside the program.  Details to be provided later, but it will probably
// pop up a window with some fields for the user to fill out then will write that to a special file and then
// call all the registered systems.
errtype bugtrak_file_report(void);

// Globals
#ifdef __BUGTRAK_SRC
Array bt_short_table, bt_long_table;
int bt_max_sys, *bt_index_table, bt_num_reg, bt_session_handle;
#else
extern Array bt_short_table, bt_long_table;
extern int bt_max_sys, *bt_index_table, bt_num_reg, bt_session_handle;
#endif

#endif // __BUGTRAK_H

