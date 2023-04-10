/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/engfeat/campaign.h,v 1.2 1998/09/30 14:11:07 mahk Exp $
#pragma once  
#ifndef __CAMPAIGN_H
#define __CAMPAIGN_H
#include <comtools.h>
#include <idispatc.h>

//////////////////////////////////////////////////////////// 
// THE CAMPAIGN MANAGER (Agg member)
//
// The "campaign" is the state of the (multi-mission) game.  
// It is saved into a tag file, much like the world database
// This API provides the message channel (in the form of an IMessageDispatch)
// through which the campaign manager state is saved and loaded.
// 

F_DECLARE_INTERFACE(ICampaign);
F_DECLARE_INTERFACE(ITagFile); 

//
// ICampaign, derived from IMessageDispatch 
//
#undef INTERFACE 
#define INTERFACE ICampaign
DECLARE_INTERFACE_(ICampaign,IMessageDispatch)
{
   DECLARE_UNKNOWN_PURE(); 
   DECLARE_DISPATCH_PURE(); 

   STDMETHOD(New)(THIS) PURE;
   STDMETHOD(Save)(THIS_ ITagFile* file) PURE; 
   STDMETHOD(Load)(THIS_ ITagFile* file) PURE; 
}; 

#undef INTERFACE

EXTERN void CampaignCreate(void); 

#endif // __CAMPAIGN_H




