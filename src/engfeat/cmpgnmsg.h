/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/engfeat/cmpgnmsg.h,v 1.2 1998/09/22 14:16:55 mahk Exp $
#pragma once  
#ifndef __CMPGNMSG_H
#define __CMPGNMSG_H

#include <comtools.h>


////////////////////////////////////////////////////////////
// CAMPAIGN MESSAGES
//

enum eCampaignMsg
{
   kCampaignReset       = 1 << 0,   // reset to empty but valid campaign
   kCampaignLoad        = 1 << 1,   // load the campaign from a file
   kCampaignSave        = 1 << 2,   // save the campaign to a file
   kCampaignDefault     = 1 << 3,   // build the "default" campaign
   kCampaignPostLoad    = 1 << 4,   // Post-load cleanup

   kCampaignAll = 0xFF
}; 

F_DECLARE_INTERFACE(ITagFile); 

struct sCampaignMsg // cast message into this type, if you like
{
   ulong kind; 
   ulong db_kind; // database message equivalent 
   ITagFile* file; 
}; 

typedef struct sCampaignMsg sCampaignMsg; 

#endif // __CMPGNMSG_H



