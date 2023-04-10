/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/motedit.cpp,v 1.6 2000/02/19 13:11:06 toml Exp $
// XXX @TODO: reset globals on startup

#include <motedit.h>
#include <motedit_.h>
#include <mediface.h>
#include <medmenu.h> // for frame slider update
#include <medplay.h>
#include <motdesc.h>
#include <mp.h>
#include <datapath.h>
#include <medflbox.h>
#include <motmngr.h>

#include <objtype.h>
#include <editbr_.h>
#include <vbrush.h>
#include <brinfo.h>
#include <motrtype.h>
#include <creatapi.h>
#include <creatext.h>
#include <viewmgr.h>
#include <status.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

//mps_motion_node activeMotions[NUM_MOTION_ROLES];

int g_CurMotEditFrame;
int g_LastMotEditFrame;

BOOL g_InMotionEditor=FALSE;

char *g_MotEditMotionName=NULL;

mps_motion *g_MotEditMotion=NULL;

IMotor *g_MotEditTarget=NULL;

int g_MotEditMotionNum=-1;

mxs_vector g_MotEditStartPos;
quat g_MotEditStartRot;


BOOL motionIsPlaying=FALSE;

void MotEditInit()
{
   if(g_InMotionEditor)
      return;

   g_InMotionEditor=TRUE;

   // initialize globals
   g_CurMotEditFrame=0;
   g_LastMotEditFrame=0;
   g_MotEditMotionName=NULL;
   g_MotEditMotion=NULL;
   g_MotEditTarget=NULL;
   g_MotEditMotionNum=-1;
   mx_zero_vec(&g_MotEditStartPos);
   quat_identity(&g_MotEditStartRot);

   CreateMotionEditorInterface();
   PlayOptInit();
   MotEditSetEditObj();
}    

void MotEditClose()
{
   if(g_InMotionEditor)
      DestroyMotionEditorInterface();
   g_InMotionEditor=FALSE;
}

void motedit_cmd(void)
{
   if(g_InMotionEditor)
      MotEditClose();
   else
      MotEditInit();
}

void MotEditSetEditObj()
{
   editBrush *pCur;
   ObjID obj;
   mxs_matrix mat;

   pCur=vBrush_GetSel();
   obj=brObj_ID(pCur);
   const Position *pos=ObjPosGet(obj);

   g_MotEditTarget=CreatureGetMotorInterface(obj);
   if(pos)
   {
      mx_copy_vec(&g_MotEditStartPos,&pos->loc.vec);
      mx_ang2mat(&mat,&pos->fac);
      quat_from_matrix(&g_MotEditStartRot,&mat);
   }
}

multiped *MotEditGetMultiped()
{
   if(g_MotEditTarget)
   {
      return (multiped *)g_MotEditTarget->GetMultiped();
   } else
   {
      return NULL;
   }
}

IMotor *MotEditGetMotor()
{
   return g_MotEditTarget;
}

void MotEditUpdate(float dt)
{
   if(FrameSliderUpdate())
   {
      // change target if necessary
      MotEditSetEditObj();

      // set target to desired frame
      PlayMotionPoseAtFrame(g_CurMotEditFrame);
   } else
   {
      PlayMotionUpdate(dt);
   }

   if(g_MotEditTarget)
   {
      vm_redraw();
   } 
}

/////////////////////////////////////////////////////////////////
// Motion Loading and Saving for Editor

// can only edit motions that are already loaded by game
BOOL MotEditSetEditMotion(char *motname)
{
   int num;
   char spew[80];

   if(!motname)
      return FALSE;

   num=MotDescNameGetNum(motname);
   if(num<0)
   {
      sprintf(spew,"Unable to load %s",motname);
      Status(spew);
      return FALSE;
   }
   g_MotEditMotion=mp_motion_list+num;
   g_MotEditMotionNum=num;
   g_MotEditMotionName=g_MotEditMotion->info.name;

   if(g_MotEditMotion) // check that name is consistent
   {
      if(strcmp(motname,g_MotEditMotion->info.name))      
      {
         Warning(("Inconsistent motion name: was %s but filename is %s\n",g_MotEditMotion->info.name,motname));
         sprintf(spew,"renaming motion from %s to %s",g_MotEditMotion->info.name,motname);
         Status(spew);
         strcpy(g_MotEditMotion->info.name,motname);
      }
   }

   if(g_MotEditMotion)
   {
      int left,right;

      MainInterfaceResetBotSlider(g_MotEditMotion->info.num_frames);
      FlagBoxResetMarks(g_MotEditMotion->info.num_frames);
      g_pMotionSet->GetStartEndFrames(num,&left,&right);
      FlagBoxSetMark(left,0);
      if(right>=0)
         FlagBoxSetMark(right,1);
      MainInterfaceRefreshFlagBox();
   } else
   {
      MainInterfaceResetBotSlider(0);
   }
   sprintf(spew,"%s succesfully loaded",motname);
   Status(spew);
   return TRUE;
}

void MotEditFinishMotionEdit()
{
   char fullname[30],spew[30];
   Datapath path;
   FILE *f;

   if(!g_MotEditMotionName || !g_MotEditMotion)
      return;

   // write out motion info to .mi file in local motion directory

   strcpy(fullname,"motions/");
   strncat(fullname,g_MotEditMotion->info.name,17);
   strcat(fullname,".mi");
   DatapathClear(&path);
   if(NULL!=(f=DatapathOpen(&path,fullname,"wb")))
   {
      mps_motion tmpMot;

      tmpMot=*g_MotEditMotion;

      tmpMot.components=NULL;
      tmpMot.flags=NULL;
      fwrite(&tmpMot,1,sizeof(tmpMot),f);
      fwrite(g_MotEditMotion->components,tmpMot.num_components,sizeof(mps_comp_motion),f);
      fwrite(g_MotEditMotion->flags,tmpMot.num_flags,sizeof(mps_motion_flag),f);
      fclose(f);
      sprintf(spew,"%s saved",g_MotEditMotionName);
   } else
   {
      sprintf(spew,"Unable to save %s",fullname);
   }

   Status(spew);

}
