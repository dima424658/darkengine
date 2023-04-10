/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/keycmd.c,v 1.25 2000/02/19 12:14:20 toml Exp $

//place to store input binding command vars

#include <keycmd.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

IInputBinder *g_pInputBinder;

IB_var g_pInputBinderVars[] = {
//{var name, var val, game or user var, aliased, game callback to be called, agg callback to resolve conflicts, always NULL},
   {"forward", "0", IBFLAG_FALLS_THRU_MOD, NULL, IBAddActiveAgg, NULL},
   {"turn", "0", IBFLAG_FALLS_THRU_MOD, NULL, IBAddActiveAgg, NULL},
   {"sidestep", "0", IBFLAG_FALLS_THRU_MOD, NULL, IBAddActiveAgg, NULL},
   {"fly", "0", IBFLAG_FALLS_THRU_MOD, NULL, IBAddActiveAgg, NULL},

   {"mturn", "0", IBFLAG_FALLS_THRU_MOD, NULL, NULL, NULL},
   {"mlook", "0", IBFLAG_FALLS_THRU_MOD, NULL, NULL, NULL},
   {"lookup", "0", IBFLAG_FALLS_THRU_MOD, NULL, NULL, NULL},
   {"lookdown", "0", IBFLAG_FALLS_THRU_MOD, NULL, NULL, NULL},
   {"tiltleft", "0", IBFLAG_FALLS_THRU_MOD, NULL, NULL, NULL},
   {"tiltright", "0", IBFLAG_FALLS_THRU_MOD, NULL, NULL, NULL},
   {"leanleft", "0", IBFLAG_FALLS_THRU_MOD, NULL, NULL, NULL},
   {"leanright", "0", IBFLAG_FALLS_THRU_MOD, NULL, NULL, NULL},
   {"leanforward", "0", IBFLAG_FALLS_THRU_MOD, NULL, NULL, NULL},
   {"block", "0", IBFLAG_FALLS_THRU_MOD, NULL, NULL, NULL},
   {"use_weapon", "0", IBFLAG_FALLS_THRU_MOD, NULL, NULL, NULL},
   {"use_item", "0", IBFLAG_FALLS_THRU_MOD, NULL, NULL, NULL},
   {"jump", "0", IBFLAG_FALLS_THRU_MOD, NULL, NULL, NULL},
   {"crouch", "0", IBFLAG_FALLS_THRU_MOD, NULL, NULL, NULL},

   {"freelookon", "0", IBFLAG_FALLS_THRU_MOD, NULL, NULL, NULL},
   {"creepon", "0", IBFLAG_FALLS_THRU_MOD, NULL, NULL, NULL},
   {"slideon", "0", IBFLAG_FALLS_THRU_MOD, NULL, NULL, NULL},

   //aliases so people have more intuitive cmds in their .bnd file than "forward -2"
   {"+walk", "+forward 1", IBFLAG_ALIAS | IBFLAG_FALLS_THRU_MOD, NULL, NULL, NULL},
   {"-walk", "-forward 1", IBFLAG_ALIAS | IBFLAG_FALLS_THRU_MOD, NULL, NULL, NULL},
   {"walk", "forward toggle 1", IBFLAG_ALIAS | IBFLAG_FALLS_THRU_MOD, NULL, NULL, NULL},
   {"+walkfast", "+forward 2", IBFLAG_ALIAS | IBFLAG_FALLS_THRU_MOD, NULL, NULL, NULL},
   {"-walkfast", "-forward 2", IBFLAG_ALIAS | IBFLAG_FALLS_THRU_MOD, NULL, NULL, NULL},
   {"walkfast", "forward toggle 2", IBFLAG_ALIAS | IBFLAG_FALLS_THRU_MOD, NULL, NULL, NULL},
   {"+back", "+forward -1", IBFLAG_ALIAS | IBFLAG_FALLS_THRU_MOD, NULL, NULL, NULL},
   {"-back", "-forward -1", IBFLAG_ALIAS | IBFLAG_FALLS_THRU_MOD, NULL, NULL, NULL},
   {"back", "forward toggle -1", IBFLAG_ALIAS | IBFLAG_FALLS_THRU_MOD, NULL, NULL, NULL},
   {"+backfast", "+forward -2", IBFLAG_ALIAS | IBFLAG_FALLS_THRU_MOD, NULL, NULL, NULL},
   {"-backfast", "-forward -2", IBFLAG_ALIAS | IBFLAG_FALLS_THRU_MOD, NULL, NULL, NULL},
   {"backfast", "forward toggle -2", IBFLAG_ALIAS | IBFLAG_FALLS_THRU_MOD, NULL, NULL, NULL},
   
   {"+moveleft", "+sidestep -1", IBFLAG_ALIAS | IBFLAG_FALLS_THRU_MOD, NULL, NULL, NULL},
   {"-moveleft", "-sidestep -1", IBFLAG_ALIAS | IBFLAG_FALLS_THRU_MOD, NULL, NULL, NULL},
   {"moveleft", "sidestep toggle -1", IBFLAG_ALIAS | IBFLAG_FALLS_THRU_MOD, NULL, NULL, NULL},
   {"+moveleftfast", "+sidestep -2", IBFLAG_ALIAS | IBFLAG_FALLS_THRU_MOD, NULL, NULL, NULL},
   {"-moveleftfast", "-sidestep -2", IBFLAG_ALIAS | IBFLAG_FALLS_THRU_MOD, NULL, NULL, NULL},
   {"moveleftfast", "sidestep toggle -2", IBFLAG_ALIAS | IBFLAG_FALLS_THRU_MOD, NULL, NULL, NULL},
   {"+moveright", "+sidestep 1", IBFLAG_ALIAS | IBFLAG_FALLS_THRU_MOD, NULL, NULL, NULL},
   {"-moveright", "-sidestep 1", IBFLAG_ALIAS | IBFLAG_FALLS_THRU_MOD, NULL, NULL, NULL},
   {"moveright", "sidestep toggle 1", IBFLAG_ALIAS | IBFLAG_FALLS_THRU_MOD, NULL, NULL, NULL},
   {"+moverightfast", "+sidestep 2", IBFLAG_ALIAS | IBFLAG_FALLS_THRU_MOD, NULL, NULL, NULL},
   {"-moverightfast", "-sidestep 2", IBFLAG_ALIAS | IBFLAG_FALLS_THRU_MOD, NULL, NULL, NULL},
   {"moverightfast", "sidestep toggle 2", IBFLAG_ALIAS | IBFLAG_FALLS_THRU_MOD, NULL, NULL, NULL},

   {"+turnleft", "+turn -1", IBFLAG_ALIAS | IBFLAG_FALLS_THRU_MOD, NULL, NULL, NULL},
   {"-turnleft", "-turn -1", IBFLAG_ALIAS | IBFLAG_FALLS_THRU_MOD, NULL, NULL, NULL},
   {"turnleft", "turn toggle -1", IBFLAG_ALIAS | IBFLAG_FALLS_THRU_MOD, NULL, NULL, NULL},
   {"+turnleftfast", "+turn -2", IBFLAG_ALIAS | IBFLAG_FALLS_THRU_MOD, NULL, NULL, NULL},
   {"-turnleftfast", "-turn -2", IBFLAG_ALIAS | IBFLAG_FALLS_THRU_MOD, NULL, NULL, NULL},
   {"turnleftfast", "turn toggle -2", IBFLAG_ALIAS | IBFLAG_FALLS_THRU_MOD, NULL, NULL, NULL},
   {"+turnright", "+turn 1", IBFLAG_ALIAS | IBFLAG_FALLS_THRU_MOD, NULL, NULL, NULL},
   {"-turnright", "-turn 1", IBFLAG_ALIAS | IBFLAG_FALLS_THRU_MOD, NULL, NULL, NULL},
   {"turnright", "turn toggle 1", IBFLAG_ALIAS | IBFLAG_FALLS_THRU_MOD, NULL, NULL, NULL},
   {"+turnrightfast", "+turn 2", IBFLAG_ALIAS | IBFLAG_FALLS_THRU_MOD, NULL, NULL, NULL},
   {"-turnrightfast", "-turn 2", IBFLAG_ALIAS | IBFLAG_FALLS_THRU_MOD, NULL, NULL, NULL},
   {"turnrightfast", "turn toggle 2", IBFLAG_ALIAS | IBFLAG_FALLS_THRU_MOD, NULL, NULL, NULL},


   {"bow_zoom", "0", 0, NULL, NULL, NULL},
   {"mouse_invert", "0", 0, NULL, NULL, NULL},
   {"lookspring", "0", 0, NULL, NULL, NULL},
   {"freelook", "1", 0, NULL, NULL, NULL},
   {"mouse_sensitivity", "1", 0, NULL, NULL, NULL},

   NULL
};
