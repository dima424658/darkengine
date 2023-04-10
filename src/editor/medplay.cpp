/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/medplay.cpp,v 1.8 2000/02/19 13:11:04 toml Exp $
// XXX @TODO: either get rid of cur_role stuff, or make it work

#include <medplay.h>
#include <motedit_.h>
#include <medmenu.h>
#include <config.h>
#include <string.h>
#include <motion.h>
#include <multiped.h>
#include <motstruc.h>
#include <status.h>
#include <motdmnge.h>
#include <mvrutils.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

PlayOptions playOptions;

mps_motion_param playParams = {MP_BEND|MP_STRETCH|MP_VSTRETCH|MP_DURATION_SCALE, 0, 0, 1, 1};

// this will need to check various play
// option stuff, and selected regions 


int cur_role=MROLE_MAIN;
static bool stopMotion=FALSE;

void PlayOptApply()
{
   if(playOptions.which_motions==PLAY_ALL_MOTIONS)
   {
      cur_role=MROLE_MAIN;
   } else
   {
      cur_role=MROLE_MAIN;
   }
}

void PlayOptInit()
{
   memset(&playOptions,0,sizeof(playOptions));

   if(config_is_defined("TRANS_OFFSET"))
   {
      int num=1;
      config_get_value("TRANS_OFFSET",CONFIG_INT_TYPE,&playOptions.trans_offset,&num);
   }
   if(config_is_defined("TRANS_DURATION"))
   {
      int num=1,val;
      config_get_value("TRANS_DURATION",CONFIG_INT_TYPE,&val,&num);
      playOptions.trans_duration=(float)val;
   }
   if(config_is_defined("PLAY_LOOP"))
      playOptions.loop=PLAY_LOOP;
   if(config_is_defined("PLAY_ALL_MOTIONS"))
      playOptions.which_motions=PLAY_ALL_MOTIONS;
   if(config_is_defined("PLAY_NORMAL_BLEND"))
      playOptions.blend=PLAY_NORMAL_BLEND;
   if(config_is_defined("PLAY_FRAME_BASED"))
      playOptions.frame_based=PLAY_FRAME_BASED;

   playOptions.timewarp=1.0;
   playOptions.stretch=1.0;

   PlayOptApply();
}

#define SIM_FRAME_RESOLUTION 400

void PlayMotionUpdate(float dt)
{
   int i;
   int freq;
   multiped *targ;
   int flags=0;

   if(NULL==(targ=MotEditGetMultiped()))
   {
      return;
   }

   if(stopMotion)
   {
      PlayButtonFunc(MOUSE_LUP,NULL,NULL);
      stopMotion=FALSE;
      return;
   }
#if 0
   if(cur_role<0)
      return;
#endif
   if(g_MotEditMotionNum<0)
      return;

   if(!motionIsPlaying)
      return;
   if(playOptions.frame_based) // one motion frame per render frame
   {
      freq=mp_motion_list[g_MotEditMotionNum].info.freq;
      flags=mp_update(targ,1000/freq,NULL);
   } else // Frame-rate independent
   {
      for(i=SIM_FRAME_RESOLUTION;i<dt;i+=SIM_FRAME_RESOLUTION)
      {
         flags|=mp_update(targ,SIM_FRAME_RESOLUTION,NULL);
      }
      flags|=mp_update(targ,dt-(i-SIM_FRAME_RESOLUTION),NULL);
   }
   if(flags)
      MvrProcessStandardFlags(MotEditGetMotor(),NULL,flags);
}

#pragma off(unreferenced)
int PlayEnd(multiped *m,int motion_number,int frame, float time_slop, ulong flags)
{
   if(playOptions.which_motions==PLAY_ALL_MOTIONS)
   {
      cur_role=(cur_role+1)%NUM_MOTION_ROLES;
      if(playOptions.loop==PLAY_STRAIGHT)
      {
         if(!cur_role)
            cur_role=-1;
      } 
   } else {
      if (playOptions.loop==PLAY_LOOP)
         cur_role=MROLE_MAIN;
      else
         cur_role=-1;
   }
   if(0==(flags&MP_CFLAG_MOT_ABORTED)) // motion ended naturally
   {
      if(playOptions.loop==PLAY_LOOP)
         PlayMotionStart();
      else
         stopMotion=TRUE;
   }
   return 1;
}
#pragma on(unreferenced)

void PlayMotionStop()
{
   multiped *targ;

   if(!motionIsPlaying)
      return;

   motionIsPlaying=FALSE;
   if(NULL!=(targ=MotEditGetMultiped()))
   {
      mp_stop_all_motions(targ);
      mp_update(targ,0,NULL);
   }
   PlayOptApply();
   Status(""); // clear out "motion playing" spew
}

EXTERN int leftMark, rightMark;

// XXX TO DO: scale for size
BOOL PlayMotionStart()
{
   mps_start_info info;
   multiped *targ;

   // change target if necessary
   MotEditSetEditObj();

   if(g_MotEditMotionNum<0)
   {
      PlayButtonFunc(MOUSE_LUP,NULL,NULL);
      Status("no motion loaded to play");
      // XXX TO DO: warn that no motion loaded
      return FALSE;
   }
   if(NULL==(targ=MotEditGetMultiped()))
   {
      PlayButtonFunc(MOUSE_LUP,NULL,NULL);
      Status("no multiped selected");
      return FALSE;
   }

   motionIsPlaying=TRUE;

   info.motion_num=g_MotEditMotionNum;
   info.start_frame=leftMark;
   info.callback_frame=rightMark;
   info.callback=PlayEnd;
   info.params=&playParams;
   info.params->duration_scalar=playOptions.timewarp;
   info.params->stretch=playOptions.stretch;
   if(playOptions.blend)
      info.trans_duration=playOptions.trans_duration;
   else
      info.trans_duration=0;
   mp_start_motion(targ,&info);

   // @TODO: start virtual motion if editmotion is an overlay.

   if(g_MotEditMotionName)
   {
      char spew[40];

      sprintf(spew,"Playing motion %s",g_MotEditMotionName);
      Status(spew);
   } else
   {
      Status("Playing nameless motion");
   }

   return TRUE;
}

void PlayMotionPoseAtFrame(int frame)
{
   mps_motion_node node;
   multiped *targ;
   mxs_vector xlat;
   IMotor *pMotor;

   if(NULL==(targ=MotEditGetMultiped()))
      return;

   if(g_MotEditMotionNum<0)
      return;

   quat *rot=new quat[targ->num_joints+1];

   // intialize xlat and rots, in case motion is an overlay
   mx_zero_vec(&xlat);
   int i;
   for(i=0;i<targ->num_joints+1;i++)
   {
      quat_identity(&rot[i]);
   }

   memset(&node,0,sizeof(node));
   node.frame=frame;
   node.handle=g_MotEditMotionNum;

   // want these to be zero if global frame of ref
   if(((pMotor=MotEditGetMotor())!=NULL)&&pMotor->GetMotFrameOfReference()!=kMFRT_RelStart)
   {
      mx_zero_vec(&node.base_pos);
      quat_identity(&node.base_orient);
   } else
   {
      mx_copy_vec(&node.base_pos,&g_MotEditStartPos);
      quat_copy(&node.base_orient,&g_MotEditStartRot);
   }

   MotDmngeLock(g_MotEditMotionNum);
   mp_evaluate_motion(targ,&node,rot,&xlat);
   mp_apply_motion(targ,rot,&xlat,-1);
   MotDmngeUnlock(g_MotEditMotionNum);

   delete rot;
}
