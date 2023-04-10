/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/engfeat/campaign.cpp,v 1.2 1998/09/22 14:13:37 mahk Exp $

#include <lg.h>
#include <campaign.h>
#include <cmpgnmsg.h>

#include <appagg.h>
#include <aggmemb.h>
#include <lststtem.h>
#include <idispat_.h>
#include <dbasemsg.h>

#include <tagfile.h>

// Include these last
#include <dbmem.h>
#include <initguid.h>
#include <cmpgniid.h>

class cBaseCampaign : public cCTDelegating<ICampaign>,
                      public cCTAggregateMemberControl<kCTU_Default>
{
};

#define INIT_MSG(x,file) { 1 << (x), (x), file }

class cCampaign : public cDispatch<cBaseCampaign>
{
   typedef cDispatch<cBaseCampaign> cParent; 

public:
   cCampaign(IUnknown* pOuter)
   {
      MI_INIT_AGGREGATION_1(pOuter,ICampaign,kPriorityNormal,NULL);
   }

   ~cCampaign() {}

   STDMETHOD(New)()
   {
      sCampaignMsg reset = INIT_MSG(kDatabaseReset,NULL); 
      HRESULT result = SendMsg((sDispatchMsg*)&reset); 
      if (FAILED(result)) return result; 

      sCampaignMsg def = INIT_MSG(kDatabaseDefault, NULL); 
      return SendMsg((sDispatchMsg*)&def); 
   }

   STDMETHOD(Save)(ITagFile* file)
   {
      sCampaignMsg save = INIT_MSG(kDatabaseSave, file); 
      return SendMsg((sDispatchMsg*)&save); 
   }

   STDMETHOD(Load)(ITagFile* file)
   {
      HRESULT result; 
      sCampaignMsg reset = INIT_MSG(kDatabaseReset,NULL); 
      result = SendMsg((sDispatchMsg*)&reset); 
      if (FAILED(result)) return result; 

      sCampaignMsg load = INIT_MSG(kDatabaseLoad, file); 
      result = SendMsg((sDispatchMsg*)&load); 
      if (FAILED(result)) return result; 

      sCampaignMsg postload = INIT_MSG(kDatabasePostLoad, file); 
      result = SendMsg((sDispatchMsg*)&postload); 
      if (FAILED(result)) return result; 

      return result; 
   }


};

void CampaignCreate()
{
   AutoAppIPtr(Unknown); 
   cAutoIPtr<ICampaign>( new cCampaign(pUnknown)); 
}

