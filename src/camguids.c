/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/camguids.c,v 1.57 2000/02/19 12:14:13 toml Exp $

//////////////////////////////////////////////////////////////
// GUID INSTANTIATION
//
// This file exists to instantiate all the guids in the app
//

// Include these three files first.
#include <comtools.h>
#include <initguid.h>

// now all the files that got guids from here.
#include <aiguid.h>
#include <2dapp.h>
#include <resapp.h>
#include <loopapp.h>
#include <testmode.h>
#include <testloop.h>
#include <scrnloop.h>
#ifdef EDITOR
#include <editmode.h>
#include <editapp.h>
#include <editgeom.h>
#endif
#include <resloop.h>
#include <uiloop.h>
#include <uiapp.h>
#include <r3app.h>
#include <portapp.h>
#include <gamemode.h>
#include <gameapp.h>
#include <basemode.h>
#include <linkloop.h>
#include <objloop.h>
#include <biploop.h>
#include <tagguid.h>
#include <sndapp.h>
#include <brloop.h>
#include <physloop.h>
#include <netmniid.h>
#include <netloop.h>
#include <plyrloop.h>
#include <simloop.h>
#ifdef AIR
//include <airloop.h>
//include <airinit.h>
#endif
#include <drkloop.h>
#include <drkinit.h>
#include <sndloop.h>
#include <rendloop.h>
#include <wrloop.h>
//#include <disploop.h>
#include <schloop.h>
#include <dlgmode.h>
#include <dlgloop.h>
#include <simtloop.h>
#include <arloop.h>
#include <roomloop.h>
#include <quesloop.h>
#include <antxloop.h>
#include <dspchiid.h>
#include <drkinvid.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 


////////////////////////////////////////////////////////////
// To get a new GUID for the app,
// cut one from the list region below and paste it into
// your header file, then include the header file above.
// be sure to change UUID_IDENTIFIER to the symbolic name
// of your new guid.
//////////////////////////////////////////////////////////////
//
// IF YOU DON'T HAVE THE RCS LOCK FOR THIS FILE,
//       THEN EDIT AT YOUR OWN PERIL!
// IF TWO SYSTEMS USE THE SAME GUID THERE WILL BE TROUBLE!
//
// unlike other errors involving code which is incorrect?
//
// Unlike other errors where the bug is obvious and happens right away.
// If two systems grab the same GUID the app might suddenly break
// one day for mysterious reasons.  The real problem is that you can
// rcsmerge this file, have a clean-looking merge and STILL have a
// merge-related bug.
//
// So for the love of god, please lock this file.
// I don't care how smart you are.
// Save me the anxiety.  Thank you.
//
//////////////////////////////////////////////////////////////


// IF there are no guids, then just use squid.  You're a big kid.  
#if 0
DEFINE_LG_GUID (UUID_IDENTIFIER, 0x76);
DEFINE_LG_GUID(IID_IESndMaterialProperty, 0x140);
#endif


