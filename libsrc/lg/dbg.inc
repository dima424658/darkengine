
option expr32
option casemap:none

IFNDEF	dbg_inc
dbg_inc	EQU		1

; Begin of file dbg.h
; 		DBG.H		Error/debug system
; 		Rex E. Bradford (REX)
; 
; * $Header: r:/prj/lib/src/lg/rcs/dbg.inc 1.1 1995/05/25 07:00:36 LMFEENEY Exp $
; * $Log: dbg.inc $
;Revision 1.1  1995/05/25  07:00:36  LMFEENEY
;Initial revision
;
;  * Revision 1.24  1994/08/12  17:03:24  jak
;  * Split DbgHandleC() off DbgHandle()
;  * Externed some things for use by dbgpp.cc
;  * Split up decl of DbgSetReportRoutine() to make
;  * C++ parser happy
;  * 
;  * Revision 1.23  1994/08/11  10:32:00  dfan
;  * Make C++ compatible
;  * 
;  * Revision 1.22  1994/03/11  11:32:52  dfan
;  * DBGS shouldn't have been do/whiled
;  * 
;  * Revision 1.21  1994/03/10  15:26:52  eric
;  * Fixed bug in Spew -- wasn't wrapped in do {} while (0), so statements like
;  * if (condition) Spew(("Hellon")); else Error(1, "Byen");
;  * would be evaluated incorrectly.  (else would be bound to wrong if() ).
;  * 
;  * Revision 1.20  1993/09/22  19:05:42  jak
;  * Oops.  Wrong # of args for Assrt().
;  * 
;  * Revision 1.19  1993/09/22  18:51:34  jak
;  * Added null 'Assrt()' macro,
;  * 
;  * Revision 1.18  1993/09/16  10:02:35  dfan
;  * When warnings and spews not on, replace them by do{}while(0), not nothing
;  * Otherwise strange things happen, for instance in conditionals
;  * 
;  * Revision 1.17  1993/08/11  14:54:23  dfan
;  * There was no DBGS macro if DBG_ON wasn't defined
;  * Removed some sarcastic comments in the interest of promoting love and harmony
;  * 
;  * Revision 1.16  1993/08/10  22:44:44  dc
;  * move around stuff to get stuff working for assembler code
;  * 
;  * Revision 1.15  1993/08/10  21:36:19  dc
;  * attempt to fix broken ifdef nesting from r1.13 on the 9th of July 
;  * when dbg macros for asm source files were broken
;  * but i cant test it since i need to make install to really do an h2i
;  * so we will see what happens
;  * 
;  * Revision 1.14  1993/07/26  10:27:45  jak
;  * Modified Assert() macro to have an ELSE clause so that it does not
;  * swallow up an ELSE clause in the caller's code.
;  * Added Assrt() macro to call Assert() with a default message
;  * for the lazy among us.
;  * 
;  * Revision 1.13  1993/07/09  09:32:56  rex
;  * Added Assert(), made dummy macro set when DBG_ON is not defined
;  * 
;  * Revision 1.12  1993/04/22  13:58:55  rex
;  * Changed mono config key install thingy from flag to func ptr
;  * 
;  * Revision 1.11  1993/04/22  11:39:23  rex
;  * Added macro DbgUseKblib()
;  * 
;  * Revision 1.10  1993/03/25  10:50:41  rex
;  * Made AtExit() into a macro, instead of function.
;  * 
;  * Revision 1.9  1993/03/24  12:19:53  matt
;  * Fixed another stupid bug.  You would think I would test these files
;  * before I checked them in.
;  * 
;  * Revision 1.8  1993/03/24  12:15:37  matt
;  * Fixed stupid mistake
;  * 
;  * Revision 1.7  1993/03/24  12:12:25  matt
;  * Added include for assembly macros
;  * 
;  * Revision 1.6  1993/03/04  11:51:41  rex
;  * Fixed macros: DbgSetDbg(), DbgSetMono(), DbgSetFunc()
;  * 
;  * Revision 1.5  1993/02/25  12:51:30  rex
;  * Changed exit-handling functions
;  * 
;  * Revision 1.4  1993/02/17  11:17:49  matt
;  * Added new macro DBGS(), like DBG(), but based on spew flags
;  * 
;  * Revision 1.3  1993/02/04  20:04:32  rex
;  * Changed DbgExit() to Exit(), etc.
;  * 
;  * Revision 1.2  1993/01/29  17:30:25  rex
;  * Added arg to Error()
;  * 
;  * Revision 1.1  1993/01/29  09:47:52  rex
;  * Initial revision
;  * 
; 
	INCLUDE	stdio.inc

	INCLUDE	types.inc

; 	The 4 levels of reporting
;  report & go to DOS
DBG_ERROR		EQU		3t
;  alert user somehow
DBG_WARNUSER		EQU		2t
;  warn developer
DBG_WARNING		EQU		1t
;  be happy
DBG_SPEW		EQU		0t
; 	'sources' combine a bank (0-31 in high 5 bits) and one or many of 27 lower
; 	'slot' bits.  You can make your own sources with DBGSRC() macro.
; 	The bank portion of a source can be extracted using DBGBANK().
NUM_DBG_BANKS		EQU		32t
NUM_DBG_SLOTS		EQU		27t
DBG_SLOT_MASK		EQU		007ffffffh
; 	If DBG_ON is defined, debug system is defined, else macros
; 	are used to compile calls and macros out
; 	The DbgBank structure maintains most information about the
; 	run-time debugging desires of banks & slots.
MAX_BANKNAMELEN		EQU		8t
MAX_SLOTNAMELEN		EQU		8t
DG_DBG		EQU		0t
DG_MONO		EQU		1t
DG_FUNC		EQU		2t
DG_FILE		EQU		3t
DG_NUM		EQU		4t
;  gates for: dbg, spew, warn, mono
;  which log file for each slot
;  bank's name
@t_dbg2		TYPEDEF		PTR PTR SBYTE
;  ptr to bank slot names
;  padding
@tag_dbg0		STRUCT 1t
gate		ulong		4t DUP (?)
file_index		uchar		27t DUP (?)
bank_name		SBYTE		9t DUP (?)
ppBankSlotNames		@t_dbg2		?
pad		SBYTE		8t DUP (?)
@tag_dbg0		ENDS

DbgBank		TYPEDEF		@tag_dbg0

;  64 bytes each
; 	If DBG_ON not defined, most macros and functions are macro'ed to
; 	nothing or (0).  A few functions remain
@proto_dbg53		TYPEDEF		PROTO C :SWORD, :PTR SBYTE, :VARARG
DbgReportError		PROTO		@proto_dbg53

@proto_dbg54		TYPEDEF		PROTO C :PTR SBYTE, :VARARG
DbgReportWarnUser		PROTO		@proto_dbg54

; #define DbgHandle(reportType,src,buff)
@proto_dbg55		TYPEDEF		PROTO C :SWORD, :PTR SBYTE
ReportRoutine		TYPEDEF		@proto_dbg55

@proto_dbg56		TYPEDEF		PROTO C :PTR ReportRoutine
DbgSetReportRoutine		PROTO		@proto_dbg56

;  look ma, an important thing to do
; include assembly macros
; dummy file; converts to 'include dbgmacro.inc'
	INCLUDE	dbgmacro.inc

; 	These routines are in exit.c, and handle exit functions.
@proto_dbg57		TYPEDEF		PROTO C :SWORD, :PTR SBYTE
Exit		PROTO		@proto_dbg57

;  shut down with msg
;  add func to atexit list
EXTERNDEF		C PrintExitMsg:

;  prints exit message
EXTERNDEF		C pExitMsg:PTR SBYTE

; End of file dbg.h

ENDIF

