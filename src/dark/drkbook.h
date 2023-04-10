/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/dark/drkbook.h,v 1.2 2000/01/27 14:34:32 adurant Exp $
#pragma once  
#ifndef __DRKBOOK_H
#define __DRKBOOK_H

////////////////////////////////////////////////////////////
// Dark Book API
//

//
// Init/Term
//
EXTERN void DarkBookInit(void); 
EXTERN void DarkBookTerm(void); 

//
// Show a book, given a text resource and an art subdir, both under
// "Books" 
//
EXTERN void SwitchToDarkBookMode(BOOL push, const char *text, const char* art);

// should return TRUE if it can find the art (book.pcx) and text.
// useful for PushMovieOrBookMode
BOOL TestBookExists(const char* text, const char* art);

#endif // __DRKBOOK_H
