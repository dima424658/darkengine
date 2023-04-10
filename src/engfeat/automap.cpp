/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/engfeat/automap.cpp,v 1.6 1998/04/24 14:11:32 TOML Exp $

#include <automap.h>
#include <autoprop.h>
#include <propert_.h>
#include <dataops_.h>

#include <sdesbase.h>
#include <sdestool.h>

#include <loopapi.h>
#include <panlmode.h>
#include <panlguid.h>
#include <imgsrc.h>
#include <loopmsg.h>

#include <room.h>
#include <rooms.h>
#include <playrobj.h>

#include <config.h>
#include <scrnmode.h>
#include <scrnman.h>

#include <command.h>
#include <ctype.h>

#include <dev2d.h>

//
// INCLUDE THESE LAST
// 
#include <dbmem.h>
#include <initguid.h>
#include <amapguid.h>

static IAutomapProperty* gAutoMapProp = NULL; 

////////////////////////////////////////////////////////////
//
// Automap Property
//

typedef cGenericProperty<IAutomapProperty,&IID_IAutomapProperty, sAutomapProperty*> cAutomapPropertyBase;

class cAutomapProperty : public cAutomapPropertyBase
{
   cClassDataOps< sAutomapProperty> mOps; 

public: 
   cAutomapProperty(const sPropertyDesc* desc, IPropertyStore* store)
      : cAutomapPropertyBase(desc,store)
   {
      SetOps(&mOps); 
   }

   cAutomapProperty(const sPropertyDesc* desc, ePropertyImpl impl)
      : cAutomapPropertyBase(desc,CreateGenericPropertyStore(impl))
   {
      SetOps(&mOps); 
   }

   STANDARD_DESCRIBE_TYPE( sAutomapProperty);
};



// Description
static sPropertyDesc AutomapProp_desc = 
{
   PROP_AUTOMAP_NAME,    // Name
   0,    // Flags
   NULL,
   2,                    // Version
   0,                    // Last ok version (0 means none)
   { "Room", "Automap" },
};

void SetupAutomapProperty(void);

void AutomapPropInit(void)
{
   SetupAutomapProperty();
   gAutoMapProp = new cAutomapProperty(&AutomapProp_desc, kPropertyImplDense);
}

void AutomapPropTerm(void)
{
   SafeRelease(gAutoMapProp);
}

// Structure Descriptor
static sFieldDesc AutomapFields[] = 
{
   { "Page",  kFieldTypeInt,  FieldLocation(sAutomapProperty, page)  },
   { "Location", kFieldTypeInt,  FieldLocation(sAutomapProperty, location) },
};

static sStructDesc AutomapDesc = StructDescBuild(sAutomapProperty, kStructFlagNone, AutomapFields);

static void SetupAutomapProperty(void)
{
   AutoAppIPtr_(StructDescTools,pTools); 
   pTools->Register(&AutomapDesc);
} 

////////////////////////////////////////////////////////////
// AUTOMAP LOOP CLIENT 
// 

class cAutomapClient : public cCTUnaggregated<ILoopClient, &IID_ILoopClient, kCTU_Default>
{
   const char* mAutomap; 

public:

   static sLoopClientDesc Descriptor; 

   cAutomapClient(const char* m) 
      : mAutomap(m)
   {

   };  


   ~cAutomapClient()
   {
   }


   STDMETHOD_(short,GetVersion)() { return kVerLoopClient; }; 
   STDMETHOD_(const sLoopClientDesc*, GetDescription)() { return &Descriptor; }; 

   STDMETHOD_(eLoopMessageResult, ReceiveMessage)(eLoopMessage, tLoopMessageData hData);
};

//
// Static: descriptor
//

static ILoopClient* LGAPI AutomapClientFactoryFunc(sLoopClientDesc * pDesc, tLoopClientData data);

sLoopClientDesc cAutomapClient::Descriptor = 
{
   &LOOPID_AutomapClient,
   "Automap Panel Client", 
   kPriorityNormal, 
   kMsgEnd | kMsgsMode ,

   kLCF_Callback,
   AutomapClientFactoryFunc, 
   
   NO_LC_DATA,

   {
      { kConstrainAfter, &LOOPID_Panel, kMsgsMode|kMsgsFrame},
      { kNullConstraint },
   }
}; 

//
// Loop message handler
//

STDMETHODIMP_(eLoopMessageResult) cAutomapClient::ReceiveMessage(eLoopMessage msg, tLoopMessageData data) 
{
   LoopMsg info; 
   info.raw = data; 

   switch (msg)
   {
      case kMsgEnterMode:
      case kMsgResumeMode:
      {
         cRoom* player_room = g_pRooms->GetObjRoom(PlayerObject()); 
         if (!player_room) 
            break; 
         ObjID room = player_room->GetObjID(); 
         sAutomapProperty* prop; 
         if (room != OBJ_NULL && gAutoMapProp->Get(room,&prop))
         {
            // @TODO: get the automap location rect for real

            char buf[40]; 
            sprintf(buf,"automap_%d",prop->location);

            int rect[4];
            int cnt = 4; 
            if (config_get_value(buf,CONFIG_INT_TYPE,rect,&cnt) && cnt == 4)
            {

               // Lookup the correct color
               int color[3] = { 255, 255, 255 }; 
               cnt = 3; 
               config_get_value("automap_rect_color",CONFIG_INT_TYPE,color,&cnt);

               int icol = 0; 
               for (int i = 0; i < 3; i++)
               {
                  icol <<= 8; 
                  icol |= color[i] & 0xFF; 
               }

               gr_set_fcolor(gr_make_screen_fcolor(icol));
               gr_box(rect[0],rect[1],rect[2],rect[3]);
            }
         }
      }
      break; 
   }   

   return kLoopDispatchContinue; 
}

//------------------------------------------------------------
// LOOP CLIENT FACTORY 
//
// Let's just use the simple implementation 
//

static ILoopClient* LGAPI AutomapClientFactoryFunc(sLoopClientDesc * pDesc, tLoopClientData data)
{
   return new cAutomapClient((const char*) data);    
}


static ulong automap_factory_id = -1; 

static void create_automap_factory()
{
   if (automap_factory_id == -1)
   {
      const sLoopClientDesc* descs[] = { &cAutomapClient::Descriptor, NULL}; 
      ILoopClientFactory* fact = CreateLoopFactory(descs);

      AutoAppIPtr_(LoopManager,pLoopMan); 
      pLoopMan->AddClientFactory(fact,&automap_factory_id); 
      SafeRelease(fact); 
   }
}

////////////////////////////////////////

static sScrnMode scrnmode_desc = 
{
   kScrnModeDimsValid,
   640, 480,
}; 

static sPanelModeDesc automap_desc = 
{ 
   &LOOPMODE_Automap,
   kPanelCenterFG|kPanelAnyKey,
   &LOOPID_AutomapClient,
   &scrnmode_desc,
}; 

void AutomapPanel(const char* respath, const char* image) 
{
   sLoopModeInitParm parms[] = { { &LOOPID_AutomapClient, (tLoopClientData)image }, { NULL } }; 

   IPanelMode* panel = CreatePanelMode(&automap_desc); 
   IImageSource* fg = CreateResourceImageSource(respath,image); 
   IImageSource* bg = CreateBitmapImageSource(&grd_canvas->bm,kBitmapSrcCopyBits); 
   panel->SetImage(kPanelFG,fg);
   panel->SetImage(kPanelBG,bg);
   SafeRelease(bg); 
   SafeRelease(fg);

   panel->Switch(kLoopModePush,parms); 
   SafeRelease(panel); 
}


void init_commands(void);

void AutomapInit(void)
{
   AutomapPropInit();
   init_commands(); 
   create_automap_factory(); 
}


void AutomapTerm()
{
   AutomapPropTerm(); 
}

//
// Commands
//

static void do_automap(char* arg)
{
   // chomp trailing whitespace
   char *s = arg+strlen(arg) - 1; 
   while (s > arg && isspace(*s)) 
      *s-- = '\0'; 
   AutomapPanel("Maps\\",arg); 
}

static Command commands [] = 
{
   { "automap", FUNC_STRING, do_automap, "Display an automap", HK_ALL },

};

static void init_commands()
{
   COMMANDS(commands,HK_ALL); 
}

