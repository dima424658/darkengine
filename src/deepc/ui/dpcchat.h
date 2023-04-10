/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// The multiplayer chat system. This module hides the gory details of
// machine-to-machine chat behind a fairly small interface.
//
#pragma once

#ifndef __DPCCHAT_H
#define __DPCCHAT_H

// Prepare the chat system. The dimensions are the location and size of the
// chat input area.
EXTERN void DPCChatInit(int x, int y, int width, int height);
// Shut down the chat system.
EXTERN void DPCChatTerm();

// Prep and shutdown the networking for chat. Sadly, this has to be done
// at a different time than the graphics -- the graphics stuff comes and
// goes whenever you leave game mode, but the networking must be
// persistent, and must be set up very early in the game.
EXTERN void DPCChatNetInit();
EXTERN void DPCChatNetTerm();

// Get a chat message from the user, and send it off to the other players
// if it completes.
EXTERN void DPCChatGetInput(char *prefix);

// Returns TRUE iff we are in the middle of getting input from the user.
EXTERN BOOL DPCChatGettingInput();

// Renders the current state of the chat input area. Should be called once
// per frame.
EXTERN void DPCChatDrawInput();

#endif // !__DPCCHAT_H
