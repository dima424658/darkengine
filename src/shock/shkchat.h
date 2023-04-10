/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkchat.h,v 1.3 2000/01/29 13:40:34 adurant Exp $
//
// The multiplayer chat system. This module hides the gory details of
// machine-to-machine chat behind a fairly small interface.
//
#pragma once

#ifndef __SHKCHAT_H
#define __SHKCHAT_H

// Prepare the chat system. The dimensions are the location and size of the
// chat input area.
EXTERN void ShockChatInit(int x, int y, int width, int height);
// Shut down the chat system.
EXTERN void ShockChatTerm();

// Prep and shutdown the networking for chat. Sadly, this has to be done
// at a different time than the graphics -- the graphics stuff comes and
// goes whenever you leave game mode, but the networking must be
// persistent, and must be set up very early in the game.
EXTERN void ShockChatNetInit();
EXTERN void ShockChatNetTerm();

// Get a chat message from the user, and send it off to the other players
// if it completes.
EXTERN void ShockChatGetInput(char *prefix);

// Returns TRUE iff we are in the middle of getting input from the user.
EXTERN BOOL ShockChatGettingInput();

// Renders the current state of the chat input area. Should be called once
// per frame.
EXTERN void ShockChatDrawInput();

#endif // !__SHKCHAT_H
