//
// $Source: r:/prj/lib/src/timer/rcs/tmlow.inc $
// $Revision: 1.2 $
// $Author
// $Date: 1994/10/17 17:44:08 $
//
// Definitions for format of low memory buffer.
//
// This file is part of the timer library.
//
// $Log: tmlow.inc $
// Revision 1.2  1994/10/17  17:44:08  lmfeeney
// use memory formerly used for tmd_ticks to store eax, not
// optimal, but easiest
// 
// Revision 1.1  1993/05/20  15:50:18  kaboom
// Initial revision
// 
#define TML_RECLIST_SIZE (0x10)
#define TML_OLD_REAL_HANDLER (0)
#define TML_DENOM (4           )
#define TML_NEW_DENOM (8           )
#define TML_CUR_REC (12          )
#define TML_GLOBAL_SIZE (16)
#define TML_RECLIST_START (TML_GLOBAL_SIZE)
#define TML_RECLIST_END (TML_RECLIST_START+TMR_SIZE*TML_RECLIST_SIZE)
#define TML_RECLIST_LAST (TML_RECLIST_END-TMR_SIZE)
#define TML_LOWBUF_SIZE (TML_RECLIST_END)