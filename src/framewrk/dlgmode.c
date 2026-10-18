/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/framewrk/dlgmode.c,v 1.2 2000/02/19 12:29:42 toml Exp $

#include <loopapi.h>
#include <dlgmode.h>
#include <scrnloop.h>
#include <dlgloop.h>
#include <uiloop.h>
#include <scrnman.h>
#include <scrnmode.h>
#include <editor.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

static tLoopClientID* DialogLoopModeClients[] =
{
   &LOOPID_Dialog,
   &LOOPID_ScrnMan,
   &LOOPID_UI,
};

sLoopModeDesc DialogLoopMode =
{
   { &LOOPID_DialogMode, "Dialog mode"}, 
   DialogLoopModeClients,
   sizeof(DialogLoopModeClients)/sizeof(DialogLoopModeClients[0]),
};




static sScrnMode _mode =
{
   kScrnModeAllValid,
   640, 480,
   8,
   kScrnModeFullScreen,
};

static ScrnManContext _scrnmode = { { &_mode } };

static uiLoopContext _uidata = 
{
   NULL,
};

static sLoopModeInitParm _InitContext[] =
{
   { &LOOPID_ScrnMan, (tLoopClientData)&_scrnmode}, 
   { &LOOPID_UI, (tLoopClientData)&_uidata}, 

   { NULL, } // terminator
};

static sLoopInstantiator _instantiator = 
{
   &LOOPID_DialogMode,
   0,
   _InitContext,
};

sLoopInstantiator* GetDialogLoopInst()
{
   return &_instantiator;
}
