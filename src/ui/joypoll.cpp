/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/ui/joypoll.cpp,v 1.4 2000/02/19 13:28:21 toml Exp $

//taken from flight 2 - DL

#include <limits.h>
#include <stdlib.h>

#include <appagg.h>
#include <comtools.h>
#include <dispapi.h>

#include <keydefs.h>
#include <event.h>

#include <joyapi.h>
#include <inpbase.h>

#include <tminit.h>
#include <timer.h>

#include <mprintf.h>

#include <joypoll.h>
#include <gen_bind.h>

#include <config.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

#define MAX_BUTTONS 32
#define DOWN(x)    ((x)|KB_FLAG_DOWN)

static IJoystick *g_pJoystick;


static bool bHasNoRudder = FALSE;
static bool bHasNoThrottle = FALSE;

void DispatchFrameEvent(void);
// return cooked keycode given a string like 'alt_shift_g'
//  bit16 is set if "rep" modifier was present
//ulong ParseKeyName( char *pKeyName );

long joyRangeX, joyMidX;
long joyRangeY, joyMidY;
long joyRangeZ, joyMidZ;
long joyRangeRudder, joyMidRudder;

// array of cooked keycodes for joystick buttons
static ulong buttonKeys[kJoyButtonsMax];

// if bit 16 of buttonKeys is set, button will repeat
#define BUTTON_KEY_REPEAT 0x10000
static float buttonRepeatTime[kJoyButtonsMax];

#define DEFAULT_BUTTON_REPEAT_INTERVAL 0.1
static float buttonRepeatInterval;
#define DEFAULT_BUTTON_REPEAT_DELAY 0.25
static float buttonRepeatDelay;

// an invalid keycode which indicates that this button
//   doesn't generate a keycode OR a button event
#define JOYBUTTON_DISABLED 0xFFFF

// These are to determine if there has been a change
static Point joyAPos = {0,0}; // joystick
static Point joyBPos = {0,0}; // throttle,rudder
static Point joyCPos = {0,0}; // hat
static char  joyButtons[MAX_BUTTONS] =
           { 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
             0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0 };  // 0 is up
// Hat translation values
static Point hatVals[9] = {{0,0},{0,1},{1,0},{0,-1},{-1,0}, {1,1},{1,-1},{-1,-1},{-1,1}};
static int lastHat  = 0; // for spoofing key events
static int hatCodes[5] = {0,KEY_PAD_UP,KEY_PAD_RIGHT,KEY_PAD_DOWN,KEY_PAD_LEFT};

cDarkJoyProcControl g_dark_joy_control;

cDarkJoyProcControl :: cDarkJoyProcControl() : cIBJoyAxisProcess()
{
   SetDeadZoneX(0.1);
   SetDeadZoneY(0.1);
   SetDeadZoneZ(0.0);
   SetDeadZoneR(0.0);
}

// set and get axis ranges   
void UiJoyAxisConfigure(eJoystickObjs axis, long *pRange, long *pMid, int deadZone)
{
   long joyMin, joyMax;
   
   // By default, we don't know what the range is, init to reasonable values
   // in case there is no joystick and player wants to use the mouse, for instance
   *pRange = 1000;
   *pMid = 0;

   if (g_pJoystick)
   {
      g_pJoystick->SetAxisRange(axis, SHRT_MIN, SHRT_MAX);
      g_pJoystick->SetAxisDeadZone(axis, deadZone );

      if (SUCCEEDED(g_pJoystick->GetAxisRange(axis, &joyMin, &joyMax)))
      {
         // round up range to keep us -1<x<1
         *pRange = (joyMax-joyMin)/2+1;
         if (1 > *pRange)
            *pRange = 1;
         // on the (bad) off-chance that the set didn't work properly
         *pMid = (joyMax+joyMin)/2;
      }
   }

}

// setup joystick for use
void UiJoyInit ()
{
   sInputDeviceIter iter;
   int i;

   IInputManager *g_pInputMan = AppGetObj(IInputManager);
   g_pInputMan->IterStart(&iter, IID_IJoystick);
   if (g_pInputMan->IterFinished(&iter))
      Warning(("No joystick devices found\n"));
   //   else
   //{
      g_pJoystick = (IJoystick*)(g_pInputMan->IterGet(&iter));

      UiJoyAxisConfigure(kJoystickXAxis, &joyRangeX, &joyMidX, 0);
      UiJoyAxisConfigure(kJoystickYAxis, &joyRangeY, &joyMidY, 0);
      UiJoyAxisConfigure(kJoystickZAxis, &joyRangeZ, &joyMidZ, 0);
      UiJoyAxisConfigure(kJoystickRudder, &joyRangeRudder, &joyMidRudder, 0);

  //}
   
   SafeRelease(g_pInputMan);

   // get the user joybutton->key mapping
   for ( i = 0; i < kJoyButtonsMax; i++ ) {
/*
      sprintf( buttonName, "button%d_key", i + 1 );
      keyName[0] = 0;
      config_get_raw( buttonName, keyName, sizeof(keyName) );

      if ( keyName[0] != 0 ) {
         if ( strcmpi( keyName, "off" ) == 0 ) {
            // disable the joystick button entirely
            buttonKeys[i] = JOYBUTTON_DISABLED;
         } else {
            // turn the button into a key
            buttonKeys[i] = ParseKeyName( keyName );
         }
      } else {
*/         // leave the button a button
         buttonKeys[i] = 0;
//      }
   }

   // get the joystick key repeat rate
   buttonRepeatInterval = DEFAULT_BUTTON_REPEAT_INTERVAL;
   config_get_float( "joykey_repeat_interval", &buttonRepeatInterval );
   buttonRepeatDelay = DEFAULT_BUTTON_REPEAT_DELAY;
   config_get_float( "joykey_repeat_delay", &buttonRepeatDelay );

   // get the no-rudder & no-throttle overrides
   bHasNoRudder = config_is_defined( "has_no_rudder" );
   bHasNoThrottle = config_is_defined( "has_no_throttle" );

   // set rudder amidships, so if user has a throttle but no rudder, the throttle
   //  triggered joy event will have a reasonable value for the rudder
   joyBPos.x = joyMidRudder;

   g_pInputBinder->RegisterJoyProcObj(&g_dark_joy_control);
}


//
// This is to create/dispatch/handle joystick events
//
// I guess it's mostly so we can record/playback joystick events
//

void DispatchJoystickMoveEvent(char joyNum, int x, int y)
{
   uiJoyEvent ev;

   uiMakeMotionEvent((uiMouseEvent *)&ev);

   ev.type = UI_EVENT_JOY;
   ev.action = UI_JOY_MOTION;
   ev.joynum = joyNum;
   ev.joypos.x = x;
   ev.joypos.y = y;

   uiDispatchEvent( (uiEvent *)&ev );

}

void DispatchJoystickButtonEvent(char buttNum, char state)
{
   uiJoyEvent ev;

   uiMakeMotionEvent((uiMouseEvent *)&ev);
   ev.type = UI_EVENT_JOY;

   // this is pretty hackly, but the deal is that joystick buttonup
   // and down messages always use the 'button1' types, and the button
   // number goes into the joynum member
   if (state == 0)
      ev.action = UI_JOY_BUTTON1UP;
   else
      ev.action = UI_JOY_BUTTON1DOWN;

   ev.joynum = buttNum;
   ev.joypos.x = 0;
   ev.joypos.y = 0;

   uiDispatchEvent( (uiEvent*)&ev );
}


void uiJoystickPoller(void)
{
   // for filtering out button presses from flightstick hats
   static bool bHatHasEverMoved = FALSE; 
   static bool bRudderHasEverMoved = FALSE;
   static bool bFirstPoll = TRUE;
   static int  oldRudderPos;
   static int  oldPOV = -1;

   // spike rejection stuff (could be merged into oldPOV, but I'm tired)
   static int hatHist1 = -1;
   static int hatHist2 = -1;

   Point hatval;
   int l;
   int hat = 0;
   int i;
   sJoyState joyState;
   uchar buttonVal;
   HRESULT hRes;
   float now;

   // This is for retarded machines that 
   // can't deal with having i/o and the 
   // display locked
   // HACK HACK HACK
   IDisplayDevice *pDisp = AppGetObj(IDisplayDevice);
   l = pDisp->BreakLock();

   if (g_pJoystick != NULL)
      hRes = g_pJoystick->GetState(&joyState);
   else
      hRes = E_FAIL;

   pDisp->RestoreLock(l);
   SafeRelease (pDisp);

   if (FAILED(hRes))
      return;

   if ( (joyState.x != joyAPos.x) || (joyState.y != joyAPos.y) )
   {
      DispatchJoystickMoveEvent( 0, joyState.x, joyState.y );
      joyAPos.x = joyState.x;
      joyAPos.y = joyState.y;
   }

   // clamp these to fit in 16-bits
   joyState.rz = ( SHRT_MAX < joyState.rz ) ? SHRT_MAX : joyState.rz;
   joyState.rz = ( SHRT_MIN > joyState.rz ) ? SHRT_MIN : joyState.rz;
   joyState.z = ( SHRT_MAX < joyState.z ) ? SHRT_MAX : joyState.z;
   joyState.z = ( SHRT_MIN > joyState.z ) ? SHRT_MIN : joyState.z;

   if ( bFirstPoll == TRUE ) {
      // ignore the first rudder value - this is to handle screwed
      //  up drivers which say there are rudders when there are none
      bFirstPoll = FALSE;
      oldRudderPos = joyState.rz;
   }
   if ( oldRudderPos != joyState.rz ) {
      bRudderHasEverMoved = TRUE;
   }
   oldRudderPos = joyState.rz;
   if ( !bRudderHasEverMoved ) {
      // if the rudder has never moved, pretend it is centered
      joyState.rz = joyMidRudder;
   }

   if ( (joyState.rz != joyBPos.x) || (joyState.z != joyBPos.y) )
   {
      DispatchJoystickMoveEvent( 1, joyState.rz, joyState.z );
      joyBPos.x = joyState.rz;
      joyBPos.y = joyState.z;

   }

   // Pretend the hat is joystick C
   switch(joyState.POV[0])
   {
      case JOY_HAT_CENTER:
         hat = 0;
         break;
      case JOY_HAT_UP:
         hat = 1;
         break;
      case JOY_HAT_RIGHT:
         hat = 2;
         break;
      case JOY_HAT_DOWN:
         hat = 3;
         break;
      case JOY_HAT_LEFT:
         hat = 4;
         break;
      case JOY_HAT_UPRIGHT:
         hat = 5;
         break;
      case JOY_HAT_DOWNRIGHT:
         hat = 6;
         break;
      case JOY_HAT_DOWNLEFT:
         hat = 7;
         break;
      case JOY_HAT_UPLEFT:
         hat = 8;
         break;
   }

   // has the hat ever moved?
   if (joyState.POV[0] != oldPOV)
   {
      if (oldPOV == -1)
         oldPOV = joyState.POV[0];
      else
         bHatHasEverMoved = TRUE;
   }

   hatval = hatVals[hat];

   // spike reject - if the hat hasn't been held for 2 successive frames, don't
   // let the message code know about it
   if ((hat==hatHist1) && (hat != hatHist2))
   {

      if ( (hatval.x != joyCPos.x) || (hatval.y != joyCPos.y) )
      {
         DispatchJoystickMoveEvent( 2, hatval.x, hatval.y );
         
         joyCPos.x = hatval.x;
         joyCPos.y = hatval.y;
/*
         // Now we dispatch key events to spoof
         if (hat!=lastHat) {
            uiCookedKeyEvent ev;

            // dispatch up event
            if (lastHat!=0) {
               ev.type = UI_EVENT_KBD_COOKED;
               ev.code = hatCodes[lastHat];
               uiDispatchEvent((uiEvent *)&ev);
            }

            // dispatch down
            if (hat!=0) {
               // dispatch down event
               ev.type = UI_EVENT_KBD_COOKED;
               ev.code = DOWN(hatCodes[hat]);
               uiDispatchEvent((uiEvent *)&ev);
            }

            lastHat = hat;
         }
*/
      }

   }
   hatHist2 = hatHist1;
   hatHist1 = hat;

   now = (double)(tm_get_millisec()/1000.0);
   for(i=0;i<MAX_BUTTONS;i++)
   {

      // this ugliness is because a CH hat shows up as a button pattern
      // and our joystick lib doesn't know if you have a hat, and if you don't,
      // 'hat' is always HAT_UP.  Jeez.
      if ( (i>3) || ( !hat ) || (!bHatHasEverMoved))
      {

         buttonVal = (joyState.buttons[i] != 0);
         if ( buttonVal != joyButtons[i] )
         {
            joyButtons[i] = buttonVal;
            DispatchJoystickButtonEvent( i, joyButtons[i] );
            // set first button repeat time
            buttonRepeatTime[i] = now + buttonRepeatDelay;
         } else if ( buttonVal != 0 ) {
            // button is still depressed - see if it should repeat
            if ( (buttonKeys[i] & BUTTON_KEY_REPEAT)
                 && (now > buttonRepeatTime[i]) ) {
               // button has repeated
               buttonRepeatTime[i] = now + buttonRepeatInterval;
               DispatchJoystickButtonEvent( i, joyButtons[i] );
            }
         }
      }
   }    
}


//
// set midpoint of x, y and rudders to current joystick position
//
void uiSetControlCenter( void )
{
   sJoyState joyState;
   HRESULT hRes;

   if ( g_pJoystick != NULL ) {
      hRes = g_pJoystick->GetState(&joyState);
      if ( SUCCEEDED( hRes ) ) {
         // set midpoints to be current joystick/throttle/rudder position
         joyMidX = joyState.x;
         joyMidY = joyState.y;
         joyMidRudder = joyState.rz;
      }
   }
}

/*
// Okay, this is how we set the global
// joystick state, which is actually more
// useful for most of our stuff than making
// it event based.
bool uiJoystickHandler(uiEvent* pEv, Region* pReg, void* state)
{
  uiJoyEvent* pJ;

  if (pEv->type != UI_EVENT_JOY)
    return FALSE;

  pJ = (uiJoyEvent*)pEv;

  switch(pJ->action)
  {
  case UI_JOY_BUTTON1UP:
    joyButtons[pJ->joynum] = 0;
    gJoyInfo.buttons[pJ->joynum] = FALSE;
    break;
  case UI_JOY_BUTTON1DOWN:
    joyButtons[pJ->joynum] = 1;
    gJoyInfo.buttons[pJ->joynum] = TRUE;
    break;
  case UI_JOY_MOTION:
    switch( pJ->joynum)
    {
    case 0:
      joyAPos.x = pJ->joypos.x;
      joyAPos.y = pJ->joypos.y;
      gJoyInfo.joyAX = (float)(joyAPos.x-joyMidX) / joyRangeX;
      gJoyInfo.joyAY = (float)(joyAPos.y-joyMidY) / joyRangeY;
      
      break;
    case 1:
      joyBPos.x = pJ->joypos.x;
      joyBPos.y = pJ->joypos.y;
      if ( !bHasNoRudder ) {
         gJoyInfo.joyBX = (float)(joyBPos.x-joyMidRudder) / joyRangeRudder;
      }
      if ( !bHasNoThrottle ) {
         gJoyInfo.joyBY = (float)(joyBPos.y-joyMidZ) / joyRangeZ;
      }
      break;
    case 2:
      joyCPos.x = pJ->joypos.x;
      joyCPos.y = pJ->joypos.y;
      gJoyInfo.hatX = (float)joyCPos.x;
      gJoyInfo.hatY = (float)joyCPos.y;
      break;
    }
    break;
  default:
    break;
  }

  return TRUE;
}
*/
