/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/framewrk/netvoice.h,v 1.2 2000/01/29 13:21:29 adurant Exp $
//
// Voice chat
//
#pragma once

#ifndef __NETVOICE_H
#define __NETVOICE_H

// Call with NULL to host a session, or with the address of the session
// to join. pAddress may be either a domain name or an IP address.
EXTERN void StartVoiceChat(char *pAddress);
EXTERN void StopVoiceChat();

#endif // !__NETVOICE_H
