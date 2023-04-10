/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/ui/panltool.cpp,v 1.16 2000/03/07 19:05:32 adurant Exp $

#include <appagg.h>
#include <panltool.h>
#include <panlmode.h>
#include <imgsrc.h>

#include <command.h>
#include <string.h>
#include <ctype.h>

#include <dev2d.h>
#include <dispapi.h>

#include <scrnmode.h>
#include <scrnman.h>
#include <panlguid.h>

#include <loopapi.h>
#include <loopmsg.h>

#include <config.h>
#include <cfgtool.h>
#include <movie.h>
#include <appsfx.h>
#include <metasnd.h>
#include <str.h>
#include <volconv.h>

// Include these last, please
#include <memall.h>
#include <dbmem.h>
#include <initguid.h>
#include <pnltulid.h>

//////////////////////////////////////////////////////////////
// STATIC IMAGE PANEL
// 

static sPanelModeDesc static_desc = 
{ 
   &LOOPMODE_StaticImagePanel,
   kPanelCenterFG|kPanelAnyKey,
   &GUID_NULL,
   NULL,
}; 

void StaticImagePanel(const char* respath, const char* image)
{
   IPanelMode* panel = CreatePanelMode(&static_desc); 
   IImageSource* fg = CreateResourceImageSource(respath,image); 

   panel->SetImage(kPanelFG,fg);
   SafeRelease(fg);
   panel->Switch(); 
   SafeRelease(panel); 
}

////////////////////////////////////////////////////////////
// MOVIE PANEL 
//////////////////////////////////////////////////////////////

//----------------
// LOOP CLIENT
//

//------------------------------------------------------------
// Loop Client
//

class cMovieClient : public cCTUnaggregated<ILoopClient, &IID_ILoopClient, kCTU_Default>
{
   cStr mMovie; 

public:

   static sLoopClientDesc Descriptor; 


   cMovieClient(const char* m) 
      : mMovie(m)
   {

   };  


   ~cMovieClient()
   {
   }


   STDMETHOD_(short,GetVersion)() { return kVerLoopClient; }; 
   STDMETHOD_(const sLoopClientDesc*, GetDescription)() { return &Descriptor; }; 

   STDMETHOD_(eLoopMessageResult, ReceiveMessage)(eLoopMessage, tLoopMessageData hData);
};

//
// Static: descriptor
//

static ILoopClient* LGAPI MovieClientFactoryFunc(sLoopClientDesc * pDesc, tLoopClientData data);

sLoopClientDesc cMovieClient::Descriptor = 
{
   &LOOPID_MovieClient,
   "Movie Panel Client", 
   kPriorityNormal, 
   kMsgEnd | kMsgsMode ,

   kLCF_Callback,
   MovieClientFactoryFunc, 
   
   NO_LC_DATA,

   {
      { kConstrainAfter, &LOOPID_Panel, kMsgsMode|kMsgsFrame},
      { kNullConstraint },
   }
}; 

//
// Loop message handler
//

STDMETHODIMP_(eLoopMessageResult) cMovieClient::ReceiveMessage(eLoopMessage msg, tLoopMessageData data) 
{
   LoopMsg info; 
   info.raw = data; 
   int volume = 0;

   switch (msg)
   {
      case kMsgEnterMode:
      case kMsgResumeMode:
      {
         BOOL sound = SFXActive(); 
         if (sound)
         {
            metaSndExitPanel(TRUE);
            volume = SFX_GetMasterVolume();
            SFXClose();
         }

         // Movie volume factor:
         // So that movies, mastered at full volume, aren't jarringly loud
         // compared with the attenuated sounds of gameplay.  Essentially, a
         // slider which adjusts the linear volume between the mixer setting
         // and completely quiet.
         float movieVolumeFactor = 1.0F;
         int movieVolume;
         config_get_float( "movie_volume_factor", &movieVolumeFactor );
         if( movieVolumeFactor <= 0.0F )
         {
            movieVolume = -10000;
         }
         else if( movieVolumeFactor >= 1.0F )
         {
            movieVolume = volume;
         }
         else
         {
            float linearVolume = VolMillibelToLinear( (float) volume );
            linearVolume *= movieVolumeFactor;
            movieVolume = (int) VolLinearToMillibel( linearVolume );
         }

         MoviePlaySynchronous(mMovie, movieVolume);
         if (sound)
         {
            metaSndEnterPanel(kMetaSndPanelLast);
            SFXInit();
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

static ILoopClient* LGAPI MovieClientFactoryFunc(sLoopClientDesc * pDesc, tLoopClientData data)
{
   return new cMovieClient((const char*) data);    
}

static ulong movie_factory_id = -1; 

static void create_movie_factory()
{
   if (movie_factory_id == -1)
   {
      const sLoopClientDesc* descs[] = { &cMovieClient::Descriptor, NULL}; 
      ILoopClientFactory* fact = CreateLoopFactory(descs);

      AutoAppIPtr_(LoopManager,pLoopMan); 
      pLoopMan->AddClientFactory(fact,&movie_factory_id); 
      SafeRelease(fact); 
   }
}

////////////////////////////////////////

static sScrnMode movie_scrnmode_desc = 
{
   kScrnModeDimsValid|kScrnModeBitDepthValid, 
   640, 480,
   16, 
}; 

static sPanelModeDesc movie_desc = 
{ 
   &LOOPMODE_Movie,
   kPanelAutoExit, // kPanelAnyKey,
   &LOOPID_MovieClient,
   &movie_scrnmode_desc,
}; 

BOOL MoviePanel(const char* fn) 
{
   if (config_is_defined("no_movies"))
      return FALSE; 

   // Look in the movie path
   char filename[256];
   if (!find_file_in_config_path(filename,(char*)fn,"movie_path"))
      return FALSE;

   sLoopModeInitParm parms[] = { { &LOOPID_MovieClient, (tLoopClientData)(const char*)filename }, { NULL } }; 

   IPanelMode* panel = CreatePanelMode(&movie_desc); 
   panel->Switch(kLoopModePush,parms); 
   SafeRelease(panel); 
   return TRUE; 
}





////////////////////////////////////////////////////////////
// PANEL TOOL INIT/TERM
//

// Forward
void setup_commands(); 


void InitPanelTools()
{
   create_movie_factory(); 
   setup_commands(); 
}

void TermPanelTools()
{

}

////////////////////////////////////////////////////////////
// Commands
//

static void do_book(char* arg)
{
   // chomp trailing whitespace
   char *s = arg+strlen(arg) - 1; 
   while (s > arg && isspace(*s)) 
      *s-- = '\0'; 

   StaticImagePanel(".",arg); 
}

static void do_movie(char* arg)
{
   // chomp trailing whitespace
   char *s = arg+strlen(arg) - 1; 
   while (s > arg && isspace(*s)) 
      *s-- = '\0'; 

   
   MoviePanel(arg); 
}

static Command commands [] = 
{
   { "show_image", FUNC_STRING, do_book, "show_image <resname>: Show an image", HK_ALL },
   { "movie", FUNC_STRING, do_movie, "show a movie", HK_ALL },
};

static void setup_commands()
{
   COMMANDS(commands,HK_ALL); 
}

