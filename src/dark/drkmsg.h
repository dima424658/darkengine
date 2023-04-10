/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/dark/drkmsg.h,v 1.1 1998/08/03 20:07:59 mahk Exp $
#pragma once  
#ifndef __DRKMSG_H
#define __DRKMSG_H

////////////////////////////////////////////////////////////
// DARK MESSAGE WINDOW
//
// A totally remedial system for displaying messages on the screen.
//

//
// Init/Term
//
EXTERN void DarkMessageInit(void); 
EXTERN void DarkMessageTerm(void); 

//
// Display A Message
//

enum 
{
   kNoMsgTimeout = -1, 
   kDefaultMsgTimeout = 5000, 
   kDefaultMsgColor = 0x0,

};


// use the default timeout
EXTERN void DarkMessage(const char* msg); 


// specify a timeout and color (use uiRGB for color)
EXTERN void DarkMessageParams(const char* msg, ulong timeout, int color); 

//
// Frame Update
//
EXTERN void DarkMessageUpdateFrame(void);

#endif // __DRKMSG_H
