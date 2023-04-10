/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/ui/panlmode.h,v 1.9 1998/09/18 19:49:22 mahk Exp $
#pragma once  
#ifndef __PANLMODE_H
#define __PANLMODE_H
#include <comtools.h>
#include <looptype.h>

F_DECLARE_INTERFACE(IDataSource);
F_DECLARE_INTERFACE(IPanelMode); 

////////////////////////////////////////////////////////////
// Generic Interface Panel Loopmode
//
// This is a lightweight tool for building simple user-interface loopmodes
// 

//
// Descriptor structure 
//

struct sPanelTransition
{
   eLoopModeChangeKind change;  // push/switch/unwindto
   const sLoopInstantiator* mode;     // mode + params
};

typedef struct sPanelTransition sPanelTransition; 

struct sPanelModeDesc 
{
   tLoopModeID* id; // my loopmode id
   ulong flags;  // flags, see below 
   tLoopClientID* client; // extra client for mode
   struct sScrnMode* screen_mode;     // screen mode to use or NULL if no opinion
   const char* pal; // palette resname or NULL if no opinion 
   sPanelTransition transition; // optional "where to go from here", otherwise endmode
};

typedef struct sPanelModeDesc sPanelModeDesc; 

enum ePanelModeFlags
{
   kPanelClearScreen          = 1 << 0, // clear the screen 
   kPanelCenterBG             = 1 << 1, // center the fg bitmap
   kPanelCenterFG             = 1 << 2, // center the fg bitmap
   kPanelAnyKey               = 1 << 3, // exit on a potent input event 
   kPanelAutoExit             = 1 << 4, // exit after the first frame
   kPanelESC                  = 1 << 5, // ESC to exit
   kPanelGrabBG               = 1 << 6, // Use the current screen as your BG
}; 

//
// Kinds of images used by panel modes
//

enum ePanelModeImage
{
   kPanelBG,
   kPanelFG, 
   kPanelCursor, 

   kNumPanelImages,
   kPanelImageInvalid = 0xFFFFFFFF, // ensure 32-bit enum 
};

typedef enum ePanelModeImage ePanelModeImage; 


//------------------------------------------------------------
// "Panel Mode" interface
//

#undef INTERFACE
#define INTERFACE IPanelMode 
DECLARE_INTERFACE_(IPanelMode,IUnknown)
{
   DECLARE_UNKNOWN_PURE();

   //
   // Return my descriptor
   //
   STDMETHOD_(const sPanelModeDesc*,Describe)(THIS) PURE; 

   //
   // Mutate description.  Don't change the loop ID's. 
   // 
   STDMETHOD(SetDescription)(THIS_ const sPanelModeDesc* desc) PURE; 

   //
   // Set/Get Images
   //
   STDMETHOD(SetImage)(THIS_ ePanelModeImage which, IDataSource* image) PURE; 
   STDMETHOD_(IDataSource*,GetImage)(THIS_ ePanelModeImage which) PURE; 

   //
   // Set our mode params
   // 
   STDMETHOD(SetParams)(THIS_ sLoopModeInitParmList parms) PURE; 

   //
   // Describe our loopmode so you can switch to it yourself
   //
   STDMETHOD_(const sLoopInstantiator*, Instantiator)(THIS) PURE; 

   //
   // Switch to the mode.  If params is NULL, uses the params last set by SetParams 
   //
   STDMETHOD(Switch)(THIS_ eLoopModeChangeKind DEFAULT_TO(kLoopModePush),  sLoopModeInitParmList parms DEFAULT_TO(NULL)) PURE;  

   //
   // Exit from this mode, making the appropriate transition to the next mode
   //
   STDMETHOD(Exit)(THIS) PURE; 
   
};

#undef INTERFACE

EXTERN IPanelMode* CreatePanelMode(const sPanelModeDesc* desc);  


#endif // __PANLMODE_H





