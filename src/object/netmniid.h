/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/netmniid.h,v 1.7 2000/01/29 13:23:48 adurant Exp $
#pragma once

// IIDs associated with the network manager and its properties.

DEFINE_LG_GUID(IID_INetManager, 0x1a5);
DEFINE_LG_GUID(IID_IHostObjIDProperty, 0x1f7);
// The following is a for a private property for the net manager.
DEFINE_LG_GUID(IID_INetPlayerProperty, 0x1f1);
DEFINE_LG_GUID(IID_IAvatar, 0x221);
DEFINE_LG_GUID(IID_INetStats, 0x23d);
// The following is for a private property for network heartbeats.
DEFINE_LG_GUID(IID_IHeartbeatListProperty, 0x273);
// This is defined in netapp.h, and should be implemented by the game:
DEFINE_LG_GUID(IID_INetAppServices, 0x334);
