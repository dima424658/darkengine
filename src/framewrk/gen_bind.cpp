/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/framewrk/gen_bind.cpp,v 1.30 2000/03/09 22:58:23 adurant Exp $

//general binds processing
#include <windows.h>
#include <stdlib.h>
#include <math.h>    // for fabs

#include <event.h>
#include <kbcntry.h>

#include <camera.h>
#include <cfgtool.h>
#include <command.h>
#include <config.h>
#include <contexts.h>
#include <gen_bind.h>
#include <headmove.h>
#include <phclimb.h>
#include <physapi.h>
#include <playtest.h>
#include <plyrmode.h>
#include <plyrspd.h>
#include <playrobj.h>
#include <mprintf.h>

#include <drk_bind.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

IInputBinder *g_pInputBinder;
void (*gInitGameIBVars)();

Region *g_root;
int g_cookie;

// These are for player paralyzing and/or view locking
EXTERN BOOL gNoMoveKeys, gNoLookAround; 
BOOL gNoMoveKeys = FALSE, gNoLookAround = FALSE; 

static BOOL grunon = FALSE; //is run key active?
static BOOL gcreepon = FALSE; //is creep toggle key active?
static BOOL gtogglefast = FALSE; //what is our desired speed toggle setting(faster/slower)
                                 //based on our ForwardProc call?
static BOOL grunpref = FALSE; //does gtoggleon currently override gcreepon?

//actually we cut these numbers in half.  
//but everything currently takes a char, so I have to double.
#define FASTSPEED 4
#define NORMSPEED 2
#define SLOWSPEED 1

/*
-------------------------------------------------
For installing and removing the binding handler
-------------------------------------------------
*/
void InstallIBHandler (ulong context, ulong events, BOOL poll)
{
   bool (*inpbnd_handler)(uiEvent *, Region *, void *);
   g_pInputBinder->GetHandler (&inpbnd_handler);

   uiSlab* slabptr;
   uiGetCurrentSlab(&slabptr);
   g_root = slabptr->creg;
   
   uiInstallRegionHandler (g_root, events, inpbnd_handler, NULL, &g_cookie);
   g_pInputBinder->SetContext (context,poll);
   // we need to poll here because SetContext can early exit with polling
   // this is to prevent stuck keys
   g_pInputBinder->PollAllKeys();
}


void RemoveIBHandler ()
{
   uiRemoveRegionHandler (g_root, g_cookie);
}


void SetGameIBVarsFunc (void (*func)())
{ 
   gInitGameIBVars = func;
}

/*
------------------------------
Relevant states
------------------------------
*/
#define NET_ROTATE_SPEED      (ROTATE_SPEED / PlayerCamera()->zoom)
#define NET_SLOW_ROTATE_SPEED (SLOW_ROTATE_SPEED / PlayerCamera()->zoom)

static BOOL g_on_ground;
static BOOL g_in_water;
static BOOL g_climbing;
static BOOL g_mantling;
static BOOL g_freelookon;

BOOL  g_joystickActive = FALSE;
float g_joystickSensitivity = 1.0;
float g_joystickDeadzone = 0.05;
float g_rudderSensitivity = 1.0;
float g_rudderDeadzone = 0.05;

static void GetStates ()
{
   g_on_ground = PhysObjOnGround (PlayerObject ());
   g_in_water = PhysObjInWater (PlayerObject ());
   g_climbing = PhysObjIsClimbing (PlayerObject ());
   g_mantling = PhysObjIsMantling (PlayerObject ());
}

/*
--------------------------
General binding callbacks
--------------------------
*/
static char *ForwardProc (const char *, const char *val, BOOL)
{

   //   mprintf("Forward %g %d\n",atof(val),atoi(val));
   
   float f = atof (val);
   if ((f != 0) && (GetPlayerMode() == kPM_Dead))
      return NULL;

   // support for special movement-lockout mode
   if ((f != 0) && (gNoMoveKeys))
      return(NULL);

   // @HACK: This is really ugly.  I hate it, but it works and we have
   // to have this key for Thief 2 because it's in the manual.
   // Please do not port this code over to Deep Cover without fixing it.
   // Also RunOnProc and CreepOnProc are affected.  AMSD 3/9/00
   if (f == FASTSPEED)
   {     
      SetForwardState (NORMSPEED);
      gtogglefast = FALSE;
      if (grunpref == 1) //using run
         SetSpeedToggleState(0); //turn off speed toggling.
   }
   else if (f == NORMSPEED)
   {
      SetForwardState (SLOWSPEED);
      gtogglefast = TRUE;
   }
   else if (f == SLOWSPEED)
   {
      SetForwardState((float)SLOWSPEED/2);
      gtogglefast = TRUE;
   }
   else if (f == -1 * FASTSPEED)
   {     
      SetForwardState (-1 * NORMSPEED);
      gtogglefast = FALSE;
   }
   else if (f == -1 * NORMSPEED)
   {
      SetForwardState (-1 * SLOWSPEED);
      gtogglefast = TRUE;
   }
   else if (f == -1 * SLOWSPEED)
   {
      SetForwardState((float)SLOWSPEED/(-2.0));
      gtogglefast = TRUE;
   }
   else 
   {
      SetForwardState(0);
      //set toggle so sidestepping works right.
      gtogglefast = FALSE;
   }

   //This is calling for an assertion that 
   //it is not the case that (grunpref && !grunon)
   //but I'm not going to do an assert here because it's too close
   //to ship.  AMSD 3/9/00
   //if using run and not going to speed up, don't modify speed.
   if ((gtogglefast == FALSE) && (grunpref))
      SetSpeedToggleState(0);
   if ((gcreepon && !grunpref)) //creep on, has control
      SetSpeedToggleState(1); //make sure
   if ((grunon && grunpref && gtogglefast))
      SetSpeedToggleState(1); //going fast, run is on and in control, make sure
   //if not trying to creep, or toggle fast set and run is controlling
   if (!gcreepon || (grunpref && grunon)) 
      SetSpeedToggleMeansFaster(gtogglefast);
   UpdatePlayerSpeed ();
   return NULL;
}

static char *JoyForwardProc (const char *, const char *val, BOOL)
{

   float f = atof (val);
   if ((f != 0) && (GetPlayerMode() == kPM_Dead))
      return NULL;

   // support for special movement-lockout mode
   if ((f!=0) && (gNoMoveKeys))
      return(NULL);
   
   if (!g_joystickActive)
      return NULL;

   if ( fabs(f) < g_joystickDeadzone ) {
      f = 0.0;
   }

   float forward_val = -2 * g_joystickSensitivity * f;

   if (forward_val > 2.0)
      forward_val = 2.0;
   else
   if (forward_val < -2.0)
      forward_val = -2.0;

   SetForwardState(forward_val);
   //AIGH!  What is the right thing here?  AMSD 3/9/00
   gtogglefast = FALSE;
   if (grunpref) //using run
      SetSpeedToggleState(0); //turn off speed toggling
   SetSpeedToggleMeansFaster(gtogglefast);
   UpdatePlayerSpeed();
   return NULL;
}

static char *TurnProc (const char *, const char *val, BOOL)
{
   //   mprintf("Turn %g %d\n",atof(val),atoi(val));
   
   if (gNoLookAround) {
      return NULL;
   }

   // support for special movement-lockout mode
   if ((GetForceSidestepState() && gNoMoveKeys))
     return NULL;

   float f = atof (val);
   if ((f != 0) && (GetPlayerMode() == kPM_Dead))
      return NULL;

   SetTurningState (f);
   UpdatePlayerSpeed ();
   return NULL;
}

static char *JoyTurnProc (const char *, const char *val, BOOL)
{
   float f = atof (val);

   if ((f != 0) && (GetPlayerMode() == kPM_Dead))
      return NULL;

   if (!g_joystickActive)
      return NULL;

   if ( fabs(f) < g_joystickDeadzone ) {
      f = 0.0;
   }

   float turn_val = 2 * g_joystickSensitivity * f;

   if (turn_val > 2.0)
      turn_val = 2.0;
   else
   if (turn_val < -2.0)
      turn_val = -2.0;

   SetTurningState(turn_val);
   UpdatePlayerSpeed();
   return NULL;
}


static char *SidestepProc (const char *, const char *val, BOOL)
{
   float f = atof (val);

   // support for special movement-lockout mode
   if ((f!=0) && (gNoMoveKeys))
      return(NULL);

   if ((f != 0) && (GetPlayerMode() == kPM_Dead))
      return NULL;

   SetSidestepState (f);
   UpdatePlayerSpeed ();
   return NULL;
}

static char *JoySidestepProc (const char *, const char *val, BOOL)
{
   float f = atof (val);

   // support for special movement-lockout mode
   if ((f != 0) && (gNoMoveKeys))
      return(NULL);

   if ((f != 0) && (GetPlayerMode() == kPM_Dead))
      return NULL;

   if (!g_joystickActive)
      return NULL;

   if ( fabs(f) < g_joystickDeadzone ) {
      f = 0.0;
   }

   float sidestep_val = 2 * g_joystickSensitivity * f;

   if (sidestep_val > 2.0)
      sidestep_val = 2.0;
   else
   if (sidestep_val < -2.0)
      sidestep_val = -2.0;

   SetSidestepState(sidestep_val);
   UpdatePlayerSpeed();
   return NULL;
}

static char *JoyXAxisProc (const char *dummy1, const char *val, BOOL dummy2)
{
   BOOL joy_rotate = ((atof (g_pInputBinder->ProcessCmd ("echo $joy_rotate")) == 0.0) ? FALSE : TRUE)
      && !GetForceSidestepState();
   float f = atof( val );

   BOOL isZero = fabs(f) < g_joystickDeadzone;
   static int lastJoyRotate = -1;

   // make sure that zero values go to the proc which received the last
   //  nonzero value - else you can wind up stuck turning or sidestep
   //  when user releases slide key with joystick not centered
   if ( isZero ) {
      if ( lastJoyRotate != -1 ) {
         joy_rotate = lastJoyRotate;
         lastJoyRotate = -1;
      }
   } else {
      if ( (lastJoyRotate != ((joy_rotate) ? 1 : 0)) && (lastJoyRotate != -1) ) {
         if ( lastJoyRotate )
            JoyTurnProc( dummy1, "0.0", dummy2 );
         else
            JoySidestepProc( dummy1, "0.0", dummy2 );
      }
      lastJoyRotate = (joy_rotate) ? 1 : 0;
   }

   if ( joy_rotate )
      JoyTurnProc( dummy1, val, dummy2 );
   else
      JoySidestepProc( dummy1, val, dummy2 );

   return NULL;
}

static char *CameraHomeProc (const char *, const char *val, BOOL)
{
  Camera* playercam = PlayerCamera();
  CameraAttach(playercam,PlayerObject());
  
  return NULL;  
}

static char *FlyProc (const char *, const char *val, BOOL)
{
#ifdef PLAYTEST
   double dval = atof (val);
   if ((dval != 0) &&(GetPlayerMode() == kPM_Dead))
      return NULL;

   float trans_scale = GetTransSpeedScale();

   if (CheatsActive()) {
      PhysSetGravity(PlayerObject(), FALSE);
      PhysSetBaseFriction(PlayerObject(), 320);

      if (dval == 0.0)
         PhysStopAxisControlVelocity(PlayerObject(), 2);
      else if (dval ==  1.0)
         PhysAxisControlVelocity(PlayerObject(), 2,  MOVE_SPEED * trans_scale);
      else if (dval == -1.0)
         PhysAxisControlVelocity(PlayerObject(), 2, -MOVE_SPEED * trans_scale);
   }
#endif
   return NULL;
}


static char *MTurnProc (const char *, const char *val, BOOL)
{
   if (gNoLookAround || GetPlayerMode() == kPM_Dead)
      return NULL;

   BOOL freelook = (atof (g_pInputBinder->ProcessCmd ("echo $freelook")) == 0.0) ? FALSE : TRUE;

   if (g_freelookon ^ freelook) 
   {
      double dval = atof (val);
      double sens = atof (g_pInputBinder->ProcessCmd ("echo $mouse_sensitivity"));
      headmoveSetRelPosX (dval * sens);
   }

   return NULL;
}


static char *MLookProc (const char *, const char *val, BOOL)
{
   if (gNoLookAround || GetPlayerMode() == kPM_Dead)
      return NULL;

   BOOL freelook = (atof (g_pInputBinder->ProcessCmd ("echo $freelook")) == 0.0) ? FALSE : TRUE;

   //mouse looking
   if (g_freelookon ^ freelook) 
   {
      double dval = atof (val);
      double sens = atof (g_pInputBinder->ProcessCmd ("echo $mouse_sensitivity"));

      char *inverted = g_pInputBinder->ProcessCmd ("echo $mouse_invert");
      if (atof (inverted) == 0.0)
         headmoveSetRelPosY (-dval * sens);//non-inverted
      else
         headmoveSetRelPosY (dval * sens);//inverted
   }
   return NULL;
}

static char *LookProc(const char *, const char *val, BOOL)
{
   if (GetPlayerMode() == kPM_Dead)
      return NULL;
   
   float dval = atof(val);
   float rot_scale = GetRotSpeedScale();
   if (dval == 0.0)
        headmoveKeyboardTiltReset();
   else
   {
     headmoveSetKeyboardTilt(NET_SLOW_ROTATE_SPEED * rot_scale * dval);
     headmoveTouch();
   }
   return NULL;
}

static char *LookUpProc (const char *, const char *val, BOOL)
{
   double dval = atof (val);
   if ((dval != 0) && (GetPlayerMode() == kPM_Dead))
      return NULL;

   float rot_scale = GetRotSpeedScale();
   if (dval == 1.0) {
     headmoveSetKeyboardTilt(NET_SLOW_ROTATE_SPEED * rot_scale);
     headmoveTouch();
   }
   else
     headmoveKeyboardTiltReset();
   return NULL;
}


static char *LookDownProc (const char *, const char *val, BOOL)
{
   double dval = atof (val);
   if ((dval != 0) && (GetPlayerMode() == kPM_Dead))
      return NULL;

   float rot_scale = GetRotSpeedScale();
   if (dval == 1.0) {
     headmoveSetKeyboardTilt(-NET_SLOW_ROTATE_SPEED * rot_scale);
     headmoveTouch ();
   }
   else
     headmoveKeyboardTiltReset();
   return NULL;
}


static char *LookCenterProc (const char *, const char *, BOOL)
{
   if (GetPlayerMode() == kPM_Dead)
      return NULL;

   headmoveKeyboardTiltReset();
   headmoveReset(PlayerCamera(), FALSE);
   
   return NULL;
}

static char *TiltLeftProc (const char *, const char *val, BOOL)
{

   double dval = atof (val);

   // support for special movement-lockout mode
   if ((dval != 0) && (gNoMoveKeys))
      return(NULL);

   if ((dval != 0) && (GetPlayerMode() == kPM_Dead))
      return NULL;

   float rot_scale = GetRotSpeedScale();
   if (dval == 1.0) {
      PhysAxisControlSubModRotationalVelocity (PlayerObject (), PLAYER_HEAD, 0, -ROTATE_SPEED * rot_scale);
      headmoveTouch ();
   }
   else
      PhysStopControlRotationalVelocity (PlayerObject ());
   return NULL;
}


static char *TiltRightProc (const char *, const char *val, BOOL)
{

   double dval = atof (val);

   // support for special movement-lockout mode
   if ((dval!=0) && (gNoMoveKeys))
      return(NULL);

   if ((dval != 0) && (GetPlayerMode() == kPM_Dead))
      return NULL;

   float rot_scale = GetRotSpeedScale();
   if (dval == 1.0) {
      PhysAxisControlSubModRotationalVelocity (PlayerObject (), PLAYER_HEAD, 0, ROTATE_SPEED * rot_scale);
      headmoveTouch ();
   }
   else
      PhysStopControlRotationalVelocity (PlayerObject ());
   return NULL;
}


static char *LeanLeftProc (const char *, const char *val, BOOL)
{
   double dval = atof (val);

   // support for special movement-lockout mode
   if ((dval != 0) && (gNoMoveKeys))
      return(NULL);

   if ((dval != 0) && (GetPlayerMode() == kPM_Dead))
      return NULL;

   GetStates ();

   if (dval == 1.0) {
      if (g_on_ground && !g_in_water) {
         switch (GetPlayerMode ()) {
            case kPM_Stand:
               PlayerMotionActivate (kMoLeanLeft);
            break;
            case kPM_Crouch:
               PlayerMotionActivate (kMoCrouchLeanLeft);
            break;
         }
            
         if ((GetPlayerMode () == kPM_Stand) || (GetPlayerMode () == kPM_Crouch))
            SetLeaningState (TRUE);
      }
   }
   
   else {
      switch (GetPlayerMode ()) {
         case kPM_Stand:  
         case kPM_Crouch:
            PlayerMotionActivate (PlayerMotionGetRest ());
         break;
      }
      SetLeaningState (FALSE);
   }
   return NULL;
}


static char *LeanRightProc (const char *, const char *val, BOOL)
{
   double dval = atof (val);

   // support for special movement-lockout mode
   if ((dval != 0) && (gNoMoveKeys))
      return(NULL);

   if ((dval != 0) && (GetPlayerMode() == kPM_Dead))
      return NULL;

   GetStates ();

   if (dval == 1.0) {
      if (g_on_ground && !g_in_water) {
         switch (GetPlayerMode ()) {
            case kPM_Stand:
               PlayerMotionActivate (kMoLeanRight);
            break;
            case kPM_Crouch:
               PlayerMotionActivate (kMoCrouchLeanRight);
            break;
         }
            
         if ((GetPlayerMode () == kPM_Stand) || (GetPlayerMode () == kPM_Crouch))
            SetLeaningState (TRUE);
      }
   }
   
   else {
      switch (GetPlayerMode ()) {
         case kPM_Stand:  
         case kPM_Crouch:
            PlayerMotionActivate (PlayerMotionGetRest ());
         break;
      }
      SetLeaningState (FALSE);
   }
   return NULL;
}             


static char *LeanForwardProc (const char *, const char *val, BOOL)
{

   double dval = atof (val);

   // support for special movement-lockout mode
   if ((dval != 0) && (gNoMoveKeys))
      return(NULL);

   if ((dval != 0) && (GetPlayerMode() == kPM_Dead))
      return NULL;

   GetStates ();

   if (dval == 1.0) {
      if (g_on_ground && !g_in_water) {
         switch (GetPlayerMode ()) {
            case kPM_Stand:
               PlayerMotionActivate (kMoLeanForward);
            break;
            case kPM_Crouch:
               PlayerMotionActivate (kMoCrouchLeanForward);
            break;
         }
            
         if ((GetPlayerMode () == kPM_Stand) || (GetPlayerMode () == kPM_Crouch))
            SetLeaningState (TRUE);
      }
   }
   
   else {
      switch (GetPlayerMode ()) {
         case kPM_Stand:  
         case kPM_Crouch:
            PlayerMotionActivate (PlayerMotionGetRest ());
         break;
      }
      SetLeaningState (FALSE);
   }
   return NULL;
}             


static char *JumpProc (const char *, const char *val, BOOL)
{
   // support for special movement-lockout mode
   if (gNoMoveKeys)
      return(NULL);

   double dval = atof (val);
   if ((dval != 0) && (GetPlayerMode() == kPM_Dead))
      return NULL;

   GetStates ();
   if (dval == 1.0) {
      SetJumpState (TRUE);
      if ((g_on_ground || g_in_water || g_climbing) && !g_mantling)
         PhysPlayerJump (PlayerObject (), JUMP_SPEED);
   }
   else {
      SetJumpState (FALSE);
      if (g_mantling)
         BreakMantle (PlayerObject ());
   }
   return NULL;
}             


static char *CrouchProc (const char *, const char *val, BOOL)
{
   // support for special movement-lockout mode
   if (gNoMoveKeys)
      return(NULL);

   double dval = atof (val);
   if ((dval != 0) && (GetPlayerMode() == kPM_Dead))
      return NULL;

   GetStates ();
   if (g_on_ground) {
      if (GetPlayerMode () == kPM_Stand)
         SetPlayerMode (kPM_Crouch);
      else if (GetPlayerMode () == kPM_Crouch)
            SetPlayerMode (kPM_Stand);
   }
   return NULL;
}             

static char *CrouchHoldProc (const char *, const char *val, BOOL)
{
   double dval = atof (val);

   // support for special movement-lockout mode
   if ((dval == 1.0) && (gNoMoveKeys))
      return(NULL);

   if (GetPlayerMode() == kPM_Dead)
      return NULL;

   GetStates ();
   if (dval == 1.0)
   {
      if (g_on_ground && GetPlayerMode () == kPM_Stand)
         SetPlayerMode (kPM_Crouch);
   }
   else
   {
      if (g_on_ground && GetPlayerMode () == kPM_Crouch)
         SetPlayerMode (kPM_Stand);
   }
   return NULL;
}

static char *FreelookOnProc (const char *, const char *val, BOOL)
{
   if (GetPlayerMode() == kPM_Dead)
      return NULL;

   double dval = atof (val);
   if (dval == 0.0) {
      g_freelookon = FALSE;

      if (atof (g_pInputBinder->ProcessCmd ("echo $lookspring")) != 0.0) 
      {
         LookCenterProc (NULL, NULL, 0);
      }
      // this moved out of above clause since we always want to lock down the
      // headmove drift. (Xemu 6/24/99)
      headmoveSetRelPosX (0.0);
      headmoveSetRelPosY (0.0);
   }
   else
      g_freelookon = TRUE;

   return NULL;
}             


static char *CreepOnProc (const char *, const char *val, BOOL)
{
   double dval = atof (val);

   // support for special movement-lockout mode
   if ((dval != 0) && (gNoMoveKeys))
      return(NULL);

   if ((dval != 0) && (GetPlayerMode() == kPM_Dead))
      return NULL;

   gcreepon = (dval == 1.0);
   if (!grunon && gcreepon)
   {
      grunpref = FALSE; //run wasn't on, we were first.
      SetSpeedToggleMeansFaster(FALSE); //creeping, so we go slower.
      SetSpeedToggleState(TRUE); //we know we are creeping, so it's TRUE
   }
   else if (!grunon && !gcreepon)
   {
      SetSpeedToggleMeansFaster(gtogglefast); //set it back to what it should be.
      SetSpeedToggleState(FALSE); //we know we aren't doing anything.
   }
   else if (grunon && gcreepon)
   {
      if (!grunpref)  //this if might succeed if someone has two creepon keys and hits them both.
      {
         SetSpeedToggleMeansFaster(FALSE);
         SetSpeedToggleState(TRUE);
      }
      //otherwise do nothing, we don't have control.
   }
   else if (grunon && !gcreepon)
   {
      grunpref = TRUE; //turned creep off, give it to run
      if (gtogglefast) //we want to speed up, not slow down
      {      
         SetSpeedToggleState(TRUE);      
      }      
      else
      {
         SetSpeedToggleState(FALSE);
      }
      SetSpeedToggleMeansFaster(gtogglefast);
   }
   UpdatePlayerSpeed();

   return NULL;
}             

static char *RunOnProc (const char *, const char *val, BOOL)
{
   double dval = atof (val);

   // support for special movement-lockout mode
   if ((dval != 0) && (gNoMoveKeys))
      return(NULL);

   if ((dval != 0) && (GetPlayerMode() == kPM_Dead))
      return NULL;

   grunon = (dval == 1.0);
   if (!grunon && gcreepon)
   {
      grunpref = FALSE; //turned run off, give it to creep
      SetSpeedToggleMeansFaster(FALSE); //creeping, so we go slower.
      SetSpeedToggleState(TRUE); //we know we are creeping, so it's TRUE
   }
   else if (!grunon && !gcreepon)
   {
      SetSpeedToggleMeansFaster(gtogglefast); //set it back to what it should be.
      SetSpeedToggleState(FALSE); //we know we aren't doing anything.
   }
   else if (grunon && gcreepon)
   {
      if (grunpref)  //this if might succeed if someone has two toggleon keys and hits them both.
      {
         SetSpeedToggleMeansFaster(gtogglefast);
         if (gtogglefast) //only if running do we become speedy
         {         
            SetSpeedToggleState(TRUE);
         }
         else
         {
            SetSpeedToggleState(FALSE);
         }
      }
      //otherwise do nothing, we don't have control.
   }
   else if (grunon && !gcreepon)
   {
      grunpref = TRUE; //toggle has control            
      SetSpeedToggleMeansFaster(gtogglefast);
      if (gtogglefast) //only affect if fast
      {      
         SetSpeedToggleState(TRUE);  
      }
      else
      {
         SetSpeedToggleState(FALSE);
      }
   }
   UpdatePlayerSpeed();

   return NULL;
}             


static char *SlideOnProc (const char *, const char *val, BOOL)
{

   double dval = atof (val);

   // support for special movement-lockout mode
   if ((dval != 0) && (gNoMoveKeys))
      return(NULL);

   if ((dval != 0) && (GetPlayerMode() == kPM_Dead))
      return NULL;

   SetForceSidestepState(dval == 1.0);
   UpdatePlayerSpeed();

   return NULL;
}             


// this is like the mouse turn proc, but for the joystick rudder (rotation around z axis)
static char *RudderTurnProc (const char *, const char *val, BOOL)
{
   if (GetPlayerMode() == kPM_Dead)
      return NULL;

   if (!g_joystickActive)
      return NULL;

   BOOL freelook = (atof (g_pInputBinder->ProcessCmd ("echo $freelook")) == 0.0) ? FALSE : TRUE;

   if (g_freelookon ^ freelook) 
   {
      float fval;
      static float lastVal = 1.0; 
      BOOL isNoise;

      fval = atof(val);
      // ignore small values around zero (noise), but let a 0 through if the
      //  the previous value was nonzero, to avoid leaving view slowly turning
      isNoise = fabs(fval) < g_rudderDeadzone;
      if ( !isNoise || (lastVal != 0.0) ) {
         if ( isNoise ) {
            fval = 0.0;
         }
#define RUDDER_SENSITIVITY_SCALER 200.0
         headmoveSetRelPosX (fval * RUDDER_SENSITIVITY_SCALER * g_rudderSensitivity);
         lastVal = fval;
      }
   }

   return NULL;
}


static char *DummyProc (const char *, const char *, BOOL)
{
   return NULL;
}


IB_var g_gen_ib_vars[] = {
//{var name, starting var val, flags, game callback to be called, agg callback to resolve conflicts, always NULL},
   {"forward", "0", 0, ForwardProc, IBAddActiveAgg, NULL},
   {"turn", "0", 0, TurnProc, IBAddActiveAgg, NULL},
   {"sidestep", "0", 0, SidestepProc, IBAddActiveAgg, NULL},
   {"camerahome", "0", 0, CameraHomeProc, NULL, NULL},
   {"fly", "0", 0, FlyProc, IBAddActiveAgg, NULL},
   {"zlook", "0", 0, LookProc, IBAddActiveAgg, NULL},

   {"joyforward", "0", 0, JoyForwardProc, IBAddActiveAgg, NULL},
   {"joyxaxis", "0", 0, JoyXAxisProc, IBAddActiveAgg, NULL},
   {"joysidestep", "0", 0, JoySidestepProc, IBAddActiveAgg, NULL},
   {"joyturn", "0", 0, JoyTurnProc, IBAddActiveAgg, NULL},
   {"rudderturn", "0", 0, RudderTurnProc, NULL, NULL},

   {"mturn", "0", 0, MTurnProc, NULL, NULL},
   {"mlook", "0", 0, MLookProc, NULL, NULL},
   {"lookup", "0", 0, LookUpProc, NULL, NULL},
   {"lookdown", "0", 0, LookDownProc, NULL, NULL},
   {"lookcenter", "0", 0, LookCenterProc, NULL, NULL},
   {"tiltleft", "0", 0, TiltLeftProc, NULL, NULL},
   {"tiltright", "0", 0, TiltRightProc, NULL, NULL},
   {"leanleft", "0", 0, LeanLeftProc, NULL, NULL},
   {"leanright", "0", 0, LeanRightProc, NULL, NULL},
   {"leanforward", "0", 0, LeanForwardProc, NULL, NULL},
   {"jump", "0", 0, JumpProc, NULL, NULL},
   {"crouch", "0", 0, CrouchProc, NULL, NULL},
   {"crouchhold", "0", 0, CrouchHoldProc, NULL, NULL},

   {"freelookon", "0", 0, FreelookOnProc, NULL, NULL},
   {"creepon", "0", 0, CreepOnProc, NULL, NULL},
   {"runon", "0", 0, RunOnProc, NULL, NULL},
   {"slideon", "0", 0, SlideOnProc, NULL, NULL},

   //aliases so people have more intuitive cmds in their .bnd file than "forward -2"
   {"+walk", "+forward 2", IBFLAG_ALIAS, NULL, NULL, NULL},
   {"-walk", "-forward 2", IBFLAG_ALIAS, NULL, NULL, NULL},
   {"walk", "forward toggle 2", IBFLAG_ALIAS, NULL, NULL, NULL},
   {"+walkfast", "+forward 4", IBFLAG_ALIAS, NULL, NULL, NULL},
   {"-walkfast", "-forward 4", IBFLAG_ALIAS, NULL, NULL, NULL},
   {"walkfast", "forward toggle 4", IBFLAG_ALIAS, NULL, NULL, NULL},      
   {"+walkslow", "+forward 1", IBFLAG_ALIAS, NULL, NULL, NULL},
   {"-walkslow", "-forward 1", IBFLAG_ALIAS, NULL, NULL, NULL},
   {"walkslow", "forward toggle 1", IBFLAG_ALIAS, NULL, NULL, NULL},
   {"+back", "+forward -2", IBFLAG_ALIAS, NULL, NULL, NULL},
   {"-back", "-forward -2", IBFLAG_ALIAS, NULL, NULL, NULL},
   {"back", "forward toggle -2", IBFLAG_ALIAS, NULL, NULL, NULL},
   {"+backfast", "+forward -4", IBFLAG_ALIAS, NULL, NULL, NULL},
   {"-backfast", "-forward -4", IBFLAG_ALIAS, NULL, NULL, NULL},
   {"backfast", "forward toggle -2", IBFLAG_ALIAS, NULL, NULL, NULL},
   {"+backslow", "+forward -1", IBFLAG_ALIAS, NULL, NULL, NULL},
   {"-backslow", "-forward -1", IBFLAG_ALIAS, NULL, NULL, NULL},
   {"backslow", "forward toggle -1", IBFLAG_ALIAS, NULL, NULL, NULL},

   {"+look_up", "+zlook 1", IBFLAG_ALIAS, NULL, NULL, NULL},
   {"-look_up", "-zlook 1", IBFLAG_ALIAS, NULL, NULL, NULL},
   {"+look_down", "+zlook -1", IBFLAG_ALIAS, NULL, NULL, NULL},
   {"-look_down", "-zlook -1", IBFLAG_ALIAS, NULL, NULL, NULL},
   
   {"+moveleft", "+sidestep -1", IBFLAG_ALIAS, NULL, NULL, NULL},
   {"-moveleft", "-sidestep -1", IBFLAG_ALIAS, NULL, NULL, NULL},
   {"moveleft", "sidestep toggle -1", IBFLAG_ALIAS, NULL, NULL, NULL},
   {"+moveleftfast", "+sidestep -2", IBFLAG_ALIAS, NULL, NULL, NULL},
   {"-moveleftfast", "-sidestep -2", IBFLAG_ALIAS, NULL, NULL, NULL},
   {"moveleftfast", "sidestep toggle -2", IBFLAG_ALIAS, NULL, NULL, NULL},
   {"+moveright", "+sidestep 1", IBFLAG_ALIAS, NULL, NULL, NULL},
   {"-moveright", "-sidestep 1", IBFLAG_ALIAS, NULL, NULL, NULL},
   {"moveright", "sidestep toggle 1", IBFLAG_ALIAS, NULL, NULL, NULL},
   {"+moverightfast", "+sidestep 2", IBFLAG_ALIAS, NULL, NULL, NULL},
   {"-moverightfast", "-sidestep 2", IBFLAG_ALIAS, NULL, NULL, NULL},
   {"moverightfast", "sidestep toggle 2", IBFLAG_ALIAS, NULL, NULL, NULL},

   {"+turnleft", "+turn -1", IBFLAG_ALIAS, NULL, NULL, NULL},
   {"-turnleft", "-turn -1", IBFLAG_ALIAS, NULL, NULL, NULL},
   {"turnleft", "turn toggle -1", IBFLAG_ALIAS, NULL, NULL, NULL},
   {"+turnleftfast", "+turn -2", IBFLAG_ALIAS, NULL, NULL, NULL},
   {"-turnleftfast", "-turn -2", IBFLAG_ALIAS, NULL, NULL, NULL},
   {"turnleftfast", "turn toggle -2", IBFLAG_ALIAS, NULL, NULL, NULL},
   {"+turnright", "+turn 1", IBFLAG_ALIAS, NULL, NULL, NULL},
   {"-turnright", "-turn 1", IBFLAG_ALIAS, NULL, NULL, NULL},
   {"turnright", "turn toggle 1", IBFLAG_ALIAS, NULL, NULL, NULL},
   {"+turnrightfast", "+turn 2", IBFLAG_ALIAS, NULL, NULL, NULL},
   {"-turnrightfast", "-turn 2", IBFLAG_ALIAS, NULL, NULL, NULL},
   {"turnrightfast", "turn toggle 2", IBFLAG_ALIAS, NULL, NULL, NULL},


   {"bow_zoom", "0", 0, DummyProc, NULL, NULL},
   {"joystick_enable", "0", 0, DummyProc, NULL, NULL},
   {"mouse_invert", "0", 0, DummyProc, NULL, NULL},
   {"lookspring", "0", 0, DummyProc, NULL, NULL},
   {"freelook", "1", 0, DummyProc, NULL, NULL},
   {"mouse_sensitivity", "1", 0, DummyProc, NULL, NULL},
   {"joy_rotate", "0", 0, DummyProc, NULL, NULL},

   {"joystick_sensitivity", "1.0", 0, DummyProc, NULL, NULL},
   {"joystick_deadzone", "0.05", 0, DummyProc, NULL, NULL},
   {"rudder_sensitivity", "1.0", 0, DummyProc, NULL, NULL},
   {"rudder_deadzone", "0.05", 0, DummyProc, NULL, NULL},

   NULL
};


/*
-------------------------------------------------------------------------------------
Master bind cmd processor. Will receive any cmds the binding system did not pick up
and pass them along to the game's command system.
-------------------------------------------------------------------------------------
*/
char *GenericBind (const char *cmd, const char *val, BOOL already_down)
{
   CommandExecute (cmd);
   return NULL;
}


static uchar GetCountryCode()
{
   unsigned short WhichLang;
   WhichLang = PRIMARYLANGID(GetSystemDefaultLangID());

   switch(WhichLang) {
      case LANG_ENGLISH:
         return KBC_US;
         break;
      case LANG_FRENCH:
         return KBC_FR;
         break;
      case LANG_GERMAN:
         return KBC_GR;
         break;
      case LANG_ITALIAN:
         return KBC_IT;
         break;
      case LANG_SPANISH:
         return KBC_SP;
         break;
      default:
         return KBC_US;
   }
}

static void SetCountryKeyboard()
{
   int lang = GetCountryCode();
   config_get_int("kb_country",&lang);
   kb_set_country(lang);
}

void InitIBVars ()
{
   g_pInputBinder->VarSet (g_gen_ib_vars);
   //game-specific
   if (gInitGameIBVars)
      gInitGameIBVars ();

   //load bnd files
   char gamebnd[128];
   config_get_raw ("game", gamebnd, 128);
   //remove trailing space
   char* s;
   for (s = gamebnd; *s != ' ' && *s != '\0'; s++);
   *s = '\0'; 
   strcat (gamebnd, ".bnd");

   //get the paths of the bnd files
   char gamebnd_path[256];
   Verify (find_file_in_config_path (gamebnd_path, gamebnd, "include_path")); 
   char def_path[256];
   Verify (find_file_in_config_path (def_path, "default.bnd", "include_path"));
   
   //creates the needed contexts
#ifdef EDITOR
   g_pInputBinder->SetContext (HK_BRUSH_EDIT, TRUE);
#endif //EDITOR
   g_pInputBinder->SetContext (HK_GAME_MODE, TRUE);
   g_pInputBinder->SetContext (HK_GAME2_MODE, TRUE);
   //since we set the mode to game mode, any non-prefixed bind will be put in game context
   g_pInputBinder->LoadBndFile (def_path, HK_GAME_MODE|HK_GAME2_MODE, NULL);
   g_pInputBinder->LoadBndFile (def_path, HK_GAME_MODE, "game");
   g_pInputBinder->LoadBndFile (def_path, HK_GAME2_MODE, "game2");
   g_pInputBinder->LoadBndFile (def_path, HK_ALL, "all");
#ifdef EDITOR
   //input binder will recognize brush edit as editor mode
   g_pInputBinder->LoadBndFile (def_path, HK_BRUSH_EDIT, "edit");
   g_pInputBinder->LoadBndFile ("user.bnd", HK_BRUSH_EDIT, "edit");
#endif //EDITOR
   // if user.bnd exists, load it, else load the game specific bnd file
   FILE *pFile = fopen( "user.bnd", "r" );
   if ( pFile ) {
      //user's local version
      fclose( pFile );
      g_pInputBinder->LoadBndFile ("user.bnd", HK_GAME_MODE, NULL);
      g_pInputBinder->LoadBndFile ("user2.bnd", HK_GAME2_MODE, NULL);
   } else {
      //game specific bnd file
      g_pInputBinder->LoadBndFile (gamebnd_path, HK_GAME_MODE|HK_GAME2_MODE, NULL);
      g_pInputBinder->LoadBndFile (gamebnd_path, HK_GAME_MODE, "game");
      g_pInputBinder->LoadBndFile (gamebnd_path, HK_GAME2_MODE, "game2");
   }

   //the command context is for when the command box is open. there are no binds in it,
   //so that no key events are eaten by the raw handler
   g_pInputBinder->SetContext (HK_COMMAND_MODE, TRUE);
#ifdef EDITOR
   g_pInputBinder->SetContext (HK_BRUSH_EDIT, TRUE);
#endif //EDITOR
   
   g_pInputBinder->SetMasterProcessCallback (GenericBind);

   SetCountryKeyboard();

   g_joystickActive = (atof (g_pInputBinder->ProcessCmd ("echo $joystick_enable")) == 0.0) ? FALSE : TRUE;

   // @TODO: this should get replaced with somewhere that this is set.
   if ( !config_get_float("joystick_sensitivity", &g_joystickSensitivity) ) {
      g_joystickSensitivity = atof (g_pInputBinder->ProcessCmd ("echo $joystick_sensitivity"));
   }
   g_joystickDeadzone = atof (g_pInputBinder->ProcessCmd ("echo $joystick_deadzone"));
   g_rudderSensitivity = atof (g_pInputBinder->ProcessCmd ("echo $rudder_sensitivity"));
   g_rudderDeadzone = atof (g_pInputBinder->ProcessCmd ("echo $rudder_deadzone"));

}

