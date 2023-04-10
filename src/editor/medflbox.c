/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/medflbox.c,v 1.7 2000/02/19 13:10:59 toml Exp $

#include <mp.h>
#include <medflbox.h>
#include <2d.h>
#include <string.h>
#include <motion.h>
#include <motstruc.h>
#include <gadget.h>
#include <motedit_.h>
#include <medmenu.h>
#include <status.h>
#include <mvrflags.h>  // for start/end mot frame
#include <memall.h>
#include <dbmem.h>   // must be last header! 

EXTERN leftMark=0, rightMark=0;
static int num_slots=0;
EXTERN LGadBox *flag_box;

uint FlagBoxGetCurrentFlag()
{
   mps_motion_flag *place;
   int num_flags,i;

   if(g_MotEditMotionNum==MP_MOT_INVALID)
   {
      Warning(("no active main motion\n"));
      return 0;
   }
   place=mp_motion_list[g_MotEditMotionNum].flags;
   num_flags=mp_motion_list[g_MotEditMotionNum].num_flags;
   for(i=0;i<num_flags;i++)
   {
      if(place->frame>=g_CurMotEditFrame)
         break;
      place++;
   }
   if(i<num_flags&&place->frame==g_CurMotEditFrame) // frame already flagged
      return place->flags;
   else
      return 0;
}

static void shiftFlags(mps_motion_flag *dst, mps_motion_flag *src, int num)
{
   mps_motion_flag *tmp=Malloc(num*sizeof(*src));

   if(!tmp)
      return;

   memcpy(tmp,src,num*sizeof(*tmp));
   memcpy(dst,tmp,num*sizeof(*dst));

   Free(tmp);
}

bool FlagBoxAddFlag(ulong value)
{
   mps_motion_flag **mflag,*place;
   int *num_flags,offset;

   if(g_MotEditMotionNum==MP_MOT_INVALID)
   {
      Warning(("no active main motion\n"));
      return FALSE;
   }
   mflag=&mp_motion_list[g_MotEditMotionNum].flags;
   num_flags=&mp_motion_list[g_MotEditMotionNum].num_flags;
   place=*mflag;
   for(offset=0;offset<*num_flags;offset++,place++)
   {
      if(place->frame>=g_CurMotEditFrame)
         break;
   }
   if(offset<*num_flags&&place->frame==g_CurMotEditFrame) // frame already flagged
   {
      if(value)  // just change value
         place->flags=value;
      else           // must remove flag
      {
         (*num_flags)--;
         if(*num_flags)
         {
            shiftFlags(place,place+1,*num_flags-offset);
            *mflag=Realloc(*mflag,(*num_flags)*sizeof(mps_motion_flag));
         }
         else
         {
            mp_free((*mflag));
            *mflag=NULL;
         }
      }
   } else if(value) // add flag
   {
      (*num_flags)++;
      if(*mflag)
      {
         *mflag=Realloc(*mflag,(*num_flags)*sizeof(mps_motion_flag));
         if(offset<*num_flags-1)
         {
            shiftFlags((*mflag)+offset+1,(*mflag)+offset,*num_flags-offset-1);
         }
      } else
      {
         *mflag=mp_alloc((*num_flags)*sizeof(mps_motion_flag),__FILE__,__LINE__);
      }
      (*mflag)[offset].flags=value;
      (*mflag)[offset].frame=g_CurMotEditFrame;
   }
   if(value&MF_START_MOT)
   {
      FlagBoxSetMark(g_CurMotEditFrame,0);
   }
   if(value&MF_END_MOT)
   {
      FlagBoxSetMark(g_CurMotEditFrame,1);
   }
   LGadDrawBox(flag_box,NULL);
   return TRUE;
}

// assumes that grd_canvas is box gadget screen region.
void FlagBoxDrawCallback(void *data, LGadBox *vb)
{
   float slot_w;
   mps_motion_flag *mflag;
   int i;
   int bdepth=(grd_bpp>16)?16:grd_bpp;
   int scale=(0x1<<bdepth)/256;

   gr_clear(3);
   slot_w=((float)grd_bm.w)/num_slots;
   // draw marks (box between left and right
   if(bdepth==8)
      gr_set_fcolor(255);
   else
      gr_set_fcolor(0);
   gr_rect((ushort)(slot_w*leftMark),0,(ushort)(slot_w*(rightMark+1)),grd_bm.h);

   // draw flags
   if(g_MotEditMotionNum==MP_MOT_INVALID)
      return;
   mflag=mp_motion_list[g_MotEditMotionNum].flags;
   for(i=0;i<mp_motion_list[g_MotEditMotionNum].num_flags;i++,mflag++)
   {
      ushort l,r;

      if(bdepth==8)
      {
         gr_set_fcolor((mflag->flags)%256);
      } else
      {
         ulong base_col=(0x1<<3)|(0x1<<9)|(0x1<<13);
         gr_set_fcolor(base_col+(scale/2)*((mflag->flags)%256));
      }
      l=slot_w*mflag->frame+1;
      r=(slot_w*(mflag->frame+1)-1);
      if(r<=l+4)
         r=l+4;
      gr_rect(l,2,r,grd_bm.h-2);
   }
}

void FlagBoxResetMarks(int num_frames)
{
   if(num_frames>0)
      num_slots=num_frames;
   leftMark=0;
   rightMark=num_slots-1;
}

void FlagBoxSetMark(int frame,uchar side)
{
   char spew[60];
   char which[8];

   if(side==0)
   {
      leftMark=frame;
      sprintf(which,"start");
   } else
   {
      rightMark=frame;
      sprintf(which,"end");
   }
   sprintf(spew,"%s frame is %d",which,frame);
   Status(spew);

   if(leftMark>rightMark)
   {
      if(side==0)
         rightMark=num_slots-1;
      else
         leftMark=0;
   }
}   
