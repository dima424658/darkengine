/*
 * $Source: x:/prj/tech/libsrc/lg/RCS/exit.c $
 * $Revision: 1.7 $
 * $Author: TOML $
 * $Date: 1996/10/10 11:04:24 $
 *
 * These few brave routines allow a message to be printed
 * on the console as the program's final act, during the
 * exit() sequence.
 */

#ifdef _WIN32
#include <windows.h>
#endif

#include <io.h>
#include <stdlib.h>
#include <string.h>
#include <lg.h>

void PrintExitMsg(void);
char *pExitMsg;			// message to print on exit
bool exitMsgInstalled;	// has exit message been installed

void SetExitMsg_(char *msg);

//	-------------------------------------------------------------
//		EXIT ROUTINES
//	-------------------------------------------------------------
//
//	Exit() sets message and then exits with error code.
//
//		errcode = error code to return.
//		msg     = message to display on console, or NULL

void Exit(int errcode, char *msg)
{
	SetExitMsg_(msg);
	exit(errcode);
}

//	-----------------------------------------------------------
//
//	SetExitMsg_() sets the exit message & makes sure that the
//	print routine is installed.
//
//		NEXT TIME DBG.H IS CHANGED, the SetExitMsg() macro should
//		be removed & replaced with a prototype of this routine, with
//		_ removed.
//
//		msg = ptr to message

void SetExitMsg_(char *msg)
{
	SetExitMsg(msg);
	if (!exitMsgInstalled)
		{
		AtExit(PrintExitMsg);
		exitMsgInstalled = TRUE;
		}
}

//	-----------------------------------------------------------
//
//	PrintExitMsg() prints exit msg on console (as final act of program).

void PrintExitMsg(void)
{
   if (pExitMsg && *pExitMsg)
#ifdef _WIN32
      MessageBox(NULL, pExitMsg, NULL, MB_OK);
#else
      write(STDOUT_FILENO,pExitMsg,strlen(pExitMsg));
#endif
}
