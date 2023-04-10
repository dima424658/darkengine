/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/propguid.h,v 1.10 2000/02/24 23:41:08 mahk Exp $
#pragma once

DEFINE_LG_GUID (IID_IProperty, 0x74);
//DEFINE_LG_GUID (IID_IPropertyRaw, 0x81);
DEFINE_LG_GUID (IID_IPropertyManager, 0x78);
DEFINE_LG_GUID (IID_IPropertyStore, 0x139); 

// Doesn't quite belong here, but it's lonely
DEFINE_LG_GUID(IID_IObjectSystem, 0x90);
DEFINE_LG_GUID(IID_IObjectNetworking, 0x218);
DEFINE_LG_GUID(IID_IObjIDManager, 0x425);
DEFINE_LG_GUID(IID_IObjIDSink, 0x433);

// Define property timing statistics GUID
DEFINE_LG_GUID (IID_IPropertyStats, 0x1fd);

// The property networking message handler
DEFINE_LG_GUID (IID_IPropertyNetworking, 0x20f);
