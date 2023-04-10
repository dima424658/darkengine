/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/dark/drkuiscr.cpp,v 1.5 2000/02/19 13:08:57 toml Exp $
#include <scrptapi.h>
#include <scrptbas.h>
#include <scrptsrv.h>

#include <drkuiscr.h>
#include <appagg.h>
#include <drkmsg.h>
#include <drkbook.h>
#include <drkplinv.h>
#include <str.h>
#include <drkbndui.h>
#include <drkuires.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 


//
// Dark UI script service
//

DECLARE_SCRIPT_SERVICE_IMPL(cDarkUISrv,DarkUI)
{
   STDMETHOD(TextMessage)(const char* message, int color, int timeout)
   {
      if (timeout == DEFAULT_TIMEOUT)
         timeout = kDefaultMsgTimeout; 
      DarkMessageParams(message,timeout,color); 
      return S_OK; 
   }

   STDMETHOD(ReadBook)(const char* text, const char* art)
      {
         SwitchToDarkBookMode(TRUE,text,art); 
         return S_OK; 
      }

   STDMETHOD_(object,InvItem)()
      {
         AutoAppIPtr(Inventory); 
         return pInventory->Selection(kCurrentItem); 
      }

   STDMETHOD_(object,InvWeapon)()
      {
         AutoAppIPtr(Inventory); 
         return pInventory->Selection(kCurrentWeapon); 
      }

   STDMETHOD(InvSelect)(object obj)
      {
         AutoAppIPtr(Inventory); 
         return pInventory->Select(obj);
      }


   STDMETHOD_(boolean,IsCommandBound)(const string ref cmd)
      {
         cStr strings[1]; 
         int n = GetCmdBinds(cmd,strings,1); 
         return n > 0;
      }

   STDMETHOD_(string,DescribeKeyBinding)(const string ref cmd) 
      {
         cStr strings[16]; 
         int n = GetCmdBinds(cmd,strings,16); 

         if (n > 0)
         {
            cStr or = FetchUIString("misc","between_key_bindings"); 
            cStr result = strings[0]; 

            // concatenate them together 
            for (int i = 1; i < n; i++)
            {
               result += or; 
               result += (const char*)strings[i]; 
            }
            return (const char*)result; 
         }

         return (const char*)FetchUIString("misc","no_key_bindings"); 
      }

   

};

IMPLEMENT_SCRIPT_SERVICE_IMPL(cDarkUISrv,DarkUI);


