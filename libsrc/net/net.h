/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: x:/prj/tech/libsrc/net/RCS/net.h 1.9 1999/10/21 15:29:53 MAT Exp $

#ifndef __NET_H
#pragma once
#define __NET_H

#include <dplay.h>
#include <comtools.h>
#include <netiid.h>

#ifndef SHIP
#define NET_ALLOW_SIMULATION
#define NET_ALLOW_TIME_SYNCH
#define NET_ALLOW_ERROR_STRINGS
#endif

// give it up, sheesh
// #define SUPPORT_DIALOG

////////////////////////////////////////////////////////////
// NETWORKING COM INTERFACES
//

#undef INTERFACE
#define INTERFACE INet

DECLARE_INTERFACE_( INet, IDirectPlay4 )
{
   DECLARE_UNKNOWN_PURE();

   /*** IDirectPlay2 methods ***/
   STDMETHOD(AddPlayerToGroup)     (THIS_ DPID, DPID) PURE;
   STDMETHOD(Close)                (THIS) PURE;
   STDMETHOD(CreateGroup)          (THIS_ LPDPID,LPDPNAME,LPVOID,DWORD,DWORD) PURE;
   STDMETHOD(CreatePlayer)         (THIS_ LPDPID,LPDPNAME,HANDLE,LPVOID,DWORD,DWORD) PURE;
   STDMETHOD(DeletePlayerFromGroup)(THIS_ DPID,DPID) PURE;
   STDMETHOD(DestroyGroup)         (THIS_ DPID) PURE;
   STDMETHOD(DestroyPlayer)        (THIS_ DPID) PURE;
   STDMETHOD(EnumGroupPlayers)     (THIS_ DPID,LPGUID,LPDPENUMPLAYERSCALLBACK2,LPVOID,DWORD) PURE;
   STDMETHOD(EnumGroups)           (THIS_ LPGUID,LPDPENUMPLAYERSCALLBACK2,LPVOID,DWORD) PURE;
   STDMETHOD(EnumPlayers)          (THIS_ LPGUID,LPDPENUMPLAYERSCALLBACK2,LPVOID,DWORD) PURE;
   STDMETHOD(EnumSessions)         (THIS_ LPDPSESSIONDESC2,DWORD,LPDPENUMSESSIONSCALLBACK2,LPVOID,DWORD) PURE;
   STDMETHOD(GetCaps)              (THIS_ LPDPCAPS,DWORD) PURE;
   STDMETHOD(GetGroupData)         (THIS_ DPID,LPVOID,LPDWORD,DWORD) PURE;
   STDMETHOD(GetGroupName)         (THIS_ DPID,LPVOID,LPDWORD) PURE;
   STDMETHOD(GetMessageCount)      (THIS_ DPID, LPDWORD) PURE;
   STDMETHOD(GetPlayerAddress)     (THIS_ DPID,LPVOID,LPDWORD) PURE;
   STDMETHOD(GetPlayerCaps)        (THIS_ DPID,LPDPCAPS,DWORD) PURE;
   STDMETHOD(GetPlayerData)        (THIS_ DPID,LPVOID,LPDWORD,DWORD) PURE;
   STDMETHOD(GetPlayerName)        (THIS_ DPID,LPVOID,LPDWORD) PURE;
   STDMETHOD(GetSessionDesc)       (THIS_ LPVOID,LPDWORD) PURE;
   STDMETHOD(Initialize)           (THIS_ LPGUID) PURE;
   STDMETHOD(Open)                 (THIS_ LPDPSESSIONDESC2,DWORD) PURE;
   STDMETHOD(Receive)              (THIS_ LPDPID,LPDPID,DWORD,LPVOID,LPDWORD) PURE;
   STDMETHOD(Send)                 (THIS_ DPID, DPID, DWORD, LPVOID, DWORD) PURE;
   STDMETHOD(SetGroupData)         (THIS_ DPID,LPVOID,DWORD,DWORD) PURE;
   STDMETHOD(SetGroupName)         (THIS_ DPID,LPDPNAME,DWORD) PURE;
   STDMETHOD(SetPlayerData)        (THIS_ DPID,LPVOID,DWORD,DWORD) PURE;
   STDMETHOD(SetPlayerName)        (THIS_ DPID,LPDPNAME,DWORD) PURE;
   STDMETHOD(SetSessionDesc)       (THIS_ LPDPSESSIONDESC2,DWORD) PURE;
   /*** IDirectPlay3 methods ***/
   STDMETHOD(AddGroupToGroup)      (THIS_ DPID, DPID) PURE;
   STDMETHOD(CreateGroupInGroup)   (THIS_ DPID,LPDPID,LPDPNAME,LPVOID,DWORD,DWORD) PURE;
   STDMETHOD(DeleteGroupFromGroup)	(THIS_ DPID,DPID) PURE;	
   STDMETHOD(EnumConnections)     	(THIS_ LPCGUID,LPDPENUMCONNECTIONSCALLBACK,LPVOID,DWORD) PURE;
   STDMETHOD(EnumGroupsInGroup)	(THIS_ DPID,LPGUID,LPDPENUMPLAYERSCALLBACK2,LPVOID,DWORD) PURE;
   STDMETHOD(GetGroupConnectionSettings)(THIS_ DWORD, DPID, LPVOID, LPDWORD) PURE;
   STDMETHOD(InitializeConnection) (THIS_ LPVOID,DWORD) PURE;
   STDMETHOD(SecureOpen)           (THIS_ LPCDPSESSIONDESC2,DWORD,LPCDPSECURITYDESC,LPCDPCREDENTIALS) PURE;
   STDMETHOD(SendChatMessage)      (THIS_ DPID,DPID,DWORD,LPDPCHAT) PURE;
   STDMETHOD(SetGroupConnectionSettings)(THIS_ DWORD,DPID,LPDPLCONNECTION) PURE;
   STDMETHOD(StartSession)         (THIS_ DWORD,DPID) PURE;
   STDMETHOD(GetGroupFlags)        (THIS_ DPID,LPDWORD) PURE;
   STDMETHOD(GetGroupParent)       (THIS_ DPID,LPDPID) PURE;
   STDMETHOD(GetPlayerAccount)     (THIS_ DPID, DWORD, LPVOID, LPDWORD) PURE;
   STDMETHOD(GetPlayerFlags)       (THIS_ DPID,LPDWORD) PURE;
   /*** IDirectPlay4 methods ***/
   STDMETHOD(GetGroupOwner)        (THIS_ DPID, LPDPID) PURE;
   STDMETHOD(SetGroupOwner)        (THIS_ DPID, DPID) PURE;
   STDMETHOD(SendEx)               (THIS_ DPID, DPID, DWORD, LPVOID, DWORD, DWORD, DWORD, LPVOID, LPDWORD) PURE;
   STDMETHOD(GetMessageQueue)      (THIS_ DPID, DPID, DWORD, LPDWORD, LPDWORD) PURE;
   STDMETHOD(CancelMessage)        (THIS_ DWORD, DWORD) PURE;
   STDMETHOD(CancelPriority)       (THIS_ DWORD, DWORD, DWORD) PURE;

    /*** INet methods ***/

   // Creates or joins a session
   // Conn is the description of where to join it "ipx" or "modem" or
   //  "internet"
   // Name is session name to join or create if not found
   // if name is null, just takes first one
   // if created is non-null, it will be set TRUE iff we created a new session
   // if pAddress is non-null, it will be used as the address to connect to;
   //   otherwise, we will pop up a dialog box to ask the user.
   // @OBSOLETE: Use the explicit Host() and Join() methods now.
   STDMETHOD(JoinOrCreate) (THIS_ 
                            char *media, 
                            char *sessionName, 
                            BOOL *created, 
                            void *pAddress) PURE;

   STDMETHOD_(BOOL, Host) (THIS_ char *media, char *sessionName) PURE;
   STDMETHOD_(BOOL, Join) (THIS_
                           char *media,
                           char *sessionName,
                           char *IPAddress) PURE;

   // Returns id of new player, or 0 if failed
   STDMETHOD_(DPID, SimpleCreatePlayer) (THIS_ char *name) PURE;

   // Return the ID of a player given a name
   STDMETHOD_(DPID, IdForName) (THIS_ char *name) PURE;

   // Return the address of the specified player, in the given buffer.
   STDMETHOD(GetPlayerAddress) (THIS_ DPID player, 
                                char *pBuffer,
                                int bufSize) PURE;

   //******* Routines for simulating Internet latencency & lossiness
#ifdef NET_ALLOW_SIMULATION
   STDMETHOD_(void, UseInternetSimulation) (THIS_ BOOL value) PURE;
   // Start simulating an internet environment, using 'internet parameters'

   STDMETHOD_(BOOL, IsSimulatingInternet) (THIS) PURE;
   // is the internet currently being simulated

   STDMETHOD_(void, SetFullInternetParameters) (THIS_ double minLatency,double aveLatency,
                                                double maxLatency,ulong lossPercent, ulong spikePercent,ulong spikeLatency) PURE;
   // for wacky new implementation with full params

   STDMETHOD_(void, SetInternetParameters) (THIS_ double minLatency,double aveLatency,
                                            double maxLatency,ulong lossPercent) PURE;
   // set the latency and lossiness parameters for internet simulation.

#ifdef SUPPORT_DIALOG   
   STDMETHOD_(void, InternetParameterDialog) (THIS) PURE;
   // Query the user for the desired internet parameters (uses "netstats.dll")
#endif   
#endif

   //**** Routines for estimating host time.
   //     (All times refer to tm_get_millesec() return values).

   STDMETHOD_(void, ResetPlayerData)(THIS_ DPID player, void *user_data) PURE;
   // Reset the player's time and user data.  This MUST be called for each player, as
   // they join the game.  Make sure that when you get the CREATEPLAYERORGROUP message
   // for a player you call this routine.  (It is called automatically for existing
   // players when you join a game).  It also should be called when either player's game
   // or this game gets paused (since the time delta's need recalculating).

#ifdef NET_ALLOW_TIME_SYNCH
   STDMETHOD_(double, ToThisGameTime)(THIS_ double player_time, DPID player) PURE;
   // Convert a time from player's time to this games game time.

   STDMETHOD_(double, PlayerTimeScale)(THIS_ DPID player) PURE;
   // return the 'scale' from the formula: player_time = game_time*scale + delta

   STDMETHOD_(double, PlayerTimeDelta)(THIS_ DPID player) PURE;
   // return the 'delta' from the formula: player_time = game_time*scale + delta

   STDMETHOD_(void, UpdatePlayerTime)(THIS_ double game_time, double player_time, DPID player) PURE;
   // A newly recieved message from player included 'time' as player's game time.
#endif

   //********************* Other useful routines.

   STDMETHOD_(const char *, DPlayErrorAsString)(THIS_ HRESULT hr) PURE;
   //Return the string representation of the direct play error hr.
};

// I'm only adding these as I need them.
#define INet_JoinOrCreate(p, a, b)      COMCall2(p, JoinOrCreate, a, b)
#define INet_SimpleCreatePlayer(p, a)   COMCall1(p, SimpleCreatePlayer, a)
#define INet_IdForName(p, a)            COMCall1(p, IdForName, a)
#define INet_Send(p, a, b, c, d, e)     COMCall5(p, Send, a, b, c, d, e)
#define INet_Receive(p, a, b, c, d, e)  COMCall5(p, Receive, a, b, c, d, e)
#ifdef NET_ALLOW_SIMULATION
#define INet_IsSimulatingInternet(p)    COMCall0(p, IsSimulatingInternet)
#define INet_UseInternetSimulation(p, a) COMCall1(p, UseInternetSimulation, a)
#define INet_SetInternetParameters(p, a, b, c, d) COMCall4(p, SetInternetParameters, a, b, c, d)
#ifdef SUPPORT_DIALOG
#define INet_InternetParameterDialog(p) COMCall0(p, InternetParameterDialog)
#endif
#endif
#define INet_ResetPlayerData(p, a, b)   COMCall2(p, ResetPlayerData, a, b)
#ifdef NET_ALLOW_TIME_SYNCH
#define INet_ToThisGameTime(p, a, b)    COMCall2(p, ToThisGameTime, a, b)
#define INet_PlayerTimeScale(p, a)      COMCall1(p, PlayerTimeScale, a)
#define INet_PlayerTimeDelta(p, a)      COMCall1(p, PlayerTimeDelta, a)
#define INet_UpdatePlayerTime(p, a, b, c) COMCall3(p, UpdatePlayerTime, a, b, c)
#endif
#define INet_GetPlayerData(p, a, b, c, d) COMCall4(p, GetPlayerData, a, b, c, d) 
#define INet_SetPlayerData(p, a, b, c, d) COMCall4(p, SetPlayerData, a, b, c, d) 
#define INet_SendEx(p, a, b, c, d, e, f, g, h, i) COMCall9(p, SendEx, a, b, c, d, e, f, g, h, i)

//
// Factory Function 
//
EXTERN void NetCreate(void);

#endif  // __NET_H