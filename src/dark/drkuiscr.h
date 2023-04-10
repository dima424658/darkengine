/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/dark/drkuiscr.h,v 1.4 1998/11/02 02:59:27 mahk Exp $
#pragma once  
#ifndef __DRKUISCR_H
#define __DRKUISCR_H

#include <scrptbas.h>
#include <uigame.h>
#include <objscrt.h>

////////////////////////////////////////////////////////////
// DARK UI SCRIPT SERVICE 
//


#ifdef SCRIPT
inline int rgb(int r, int g, int b) { return uiRGB(r,g,b); }; 
#endif SCRIPT

#define DEFAULT_TIMEOUT -1001

DECLARE_SCRIPT_SERVICE(DarkUI,0x19f)
{
   //
   // Send a message to the screen 
   // Optional parameters are color (use rgb function above)
   // and time out (milliseconds)
   //
   STDMETHOD(TextMessage)(const char* message, int color = 0, int timeout = DEFAULT_TIMEOUT) PURE; 

   //
   // Read a book, given resnames for its text and art 
   //
   STDMETHOD(ReadBook)(const char* text, const char* art) PURE; 

   //
   // Get the inventory selections
   //
   STDMETHOD_(object,InvItem)() PURE;
   STDMETHOD_(object,InvWeapon)() PURE;

   STDMETHOD(InvSelect)(object obj) PURE;

   //
   // Look up bound command
   //
   STDMETHOD_(boolean,IsCommandBound)(const string ref cmd) PURE; 
   STDMETHOD_(string,DescribeKeyBinding)(const string ref cmd) PURE; 


}; 


#endif // __DRKUISCR_H
