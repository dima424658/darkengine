/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/rendtest.c,v 1.6 2000/02/19 13:11:20 toml Exp $
// tests the renderer by doing lots of renderings from various places

#ifndef SHIP

#include <stdlib.h>
#include <stdio.h>
#include <io.h>

#include <timer.h>
#include <mprintf.h>
#include <kbcook.h>
#include <dump.h>

#include <csg.h>
#include <csgbrush.h>
#include <portal.h>
#include <editbr_.h>
#include <modalui.h>  // for ui for rendtest
#include <brinfo.h>
#include <brquery.h>
#include <gedit.h>
#include <ged_csg.h>
#include <rendtest.h>

#include <command.h>
#include <status.h>

#include <sdesc.h>
#include <sdesbase.h>
#include <isdesced.h>
#include <isdescst.h>

////////////////////////
// sdesc/setup for the config

typedef struct {
   // public fields for user   
   int  tests_per_source;
   int  total_count;
   int  view_width;
   int  view_height;
   int  rand_seed;
   float step_size;
   BOOL flat_only;
   BOOL inc_angle;
   BOOL move_straight;
   BOOL brush_centers;
   // internal fields for state
   int  tests_run;
   int  tests_left_at_this_source;
   Position pos;
   BOOL rand_time;
   // probably need more fields here for level validity checking?
} sRendTestParms;

// add a filename?

static sFieldDesc rendtest_fields [] =
{
   {"Total Count",         kFieldTypeInt,   FieldLocation(sRendTestParms,total_count) },
   {"Tests Per Source",    kFieldTypeInt,   FieldLocation(sRendTestParms,tests_per_source) },
   {"View Width (0 full)", kFieldTypeInt,   FieldLocation(sRendTestParms,view_width) },
   {"View Height(0 full)", kFieldTypeInt,   FieldLocation(sRendTestParms,view_height) },
   {"Rand Seed (0=time)",  kFieldTypeInt,   FieldLocation(sRendTestParms,rand_seed) },
   {"Step Size",           kFieldTypeFloat, FieldLocation(sRendTestParms,step_size) },
   {"Flat Only",           kFieldTypeBool,  FieldLocation(sRendTestParms,flat_only) },
   {"Increment Angle",     kFieldTypeBool,  FieldLocation(sRendTestParms,inc_angle) },
   {"Move Straight",       kFieldTypeBool,  FieldLocation(sRendTestParms,move_straight) },
   {"Brush Centers",       kFieldTypeBool,  FieldLocation(sRendTestParms,brush_centers) },
};

static sStructDesc rendtest_struct = StructDescBuild(sRendTestParms,kStructFlagNone,rendtest_fields);

static sRendTestParms rtP={250,0,0,0,0,1.0,FALSE,FALSE,FALSE,FALSE,0,-1};
static BOOL          _rtP_inuse=FALSE;

#define DEFAULT_FNAME "rtest000.try"
static char           cur_write_fname[255];

///////////////////////////
// dialog controllers

// this is really later in the file
void rendtest_run(BOOL new_run);

static void LGAPI rendtest_prop_cb(sStructEditEvent *event, StructEditCBData data)
{
   switch (event->kind)
   {
      case kStructEdOK:
         rendtest_run(TRUE);  // this sets the inuse off, eh?
         break;
      case kStructEdCancel:
         _rtP_inuse=FALSE;
         break;
   }
}

static BOOL rendtest_dialog(void)
{
   IStructEditor *sed;
   sStructEditorDesc editdesc;

   strncpy(editdesc.title, "RendTest Parameters", sizeof(editdesc.title));
   editdesc.flags = kStructEditNoApplyButton;
   sed = CreateStructEditor(&editdesc, &rendtest_struct, &rtP);
   if (!sed)
    { _rtP_inuse=FALSE; return FALSE; }
   IStructEditor_SetCallback(sed,rendtest_prop_cb,&rtP);
   IStructEditor_Go(sed,kStructEdModeless);
   SafeRelease(sed);
   return TRUE;
}

/////////////////////////
// ultra-dorky file api

#include <stdio.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

static BOOL write_rtP(char *fname)
{
   FILE *fp;
   if ((fp=fopen(fname,"wb"))==NULL)
      return FALSE;
   fwrite(&rtP,sizeof(sRendTestParms),1,fp);
   fclose(fp);
   return TRUE;   
}

static BOOL read_rtP(char *fname)
{
   FILE *fp;
   if ((fp=fopen(fname,"rb"))==NULL)
      return FALSE;
   fread(&rtP,sizeof(sRendTestParms),1,fp);
   fclose(fp);
   return TRUE;
}

/////////////////////////
// actual tester

static float frand_hack(void)
{
   float tmp=(float)((rand()&0x7fff)-16384);
   tmp/=16384;
   return tmp;
}

static void get_rand_vec(mxs_vector *dst, float scale)
{
   dst->x=frand_hack()*scale;
   dst->y=frand_hack()*scale;
   dst->z=frand_hack()*scale;
}

// sit around picking locations making sure they are valid
static BOOL rendtest_pick_source_pos(Position *p)
{
   int br_id=rand()%csg_num_brushes, attempts=0;
   editBrush *br;
   
   do {          // lovely do construction, eh?
      do {
         br=brFind(br_id);
         br_id=(br_id+1)%csg_num_brushes;
      } while (((br==NULL)||(brushGetType(br)==brType_HOTREGION))&&(++attempts<csg_num_brushes));
      if (rtP.brush_centers)
         MakeLocationFromVector(&p->loc,&br->pos);
      else
      {
         mxs_vector tmp_pos=br->pos, tmp_sz=br->sz, rnd;
         get_rand_vec(&rnd,1.0);
         mx_elmuleq_vec(&tmp_sz,&rnd);
         mx_addeq_vec(&tmp_pos,&tmp_sz);
         MakeLocationFromVector(&p->loc,&tmp_pos);
      }
      MakeOrientationFromAngVec(&p->fac,&br->ang);
   } while ((ComputeCellForLocation(&p->loc)==CELL_INVALID)&&(attempts<csg_num_brushes));
   if (rtP.flat_only) p->fac.tx=p->fac.ty=0;
   return attempts<csg_num_brushes;
}

// cnt -1 means new setup
static void rendtest_init_new(void)
{
   int fd;
   
   rtP.rand_time=(rtP.rand_seed==0);
   if (rtP.rand_time)
      rtP.rand_seed=tm_get_millisec_unrecorded();
   srand(rtP.rand_seed);

   rtP.tests_left_at_this_source=-1;
   rtP.tests_run=0;

   if ((fd=dmp_find_free_file(cur_write_fname,"rtest","try"))!=-1)
      close(fd);
}

static grs_canvas *rendtest_maybe_build_subcanvas(void)
{
   int x_off=0, y_off=0;     // subcanvas offsets

   if ((rtP.view_width!=0)&&(rtP.view_width<grd_canvas->bm.w))
      x_off=(grd_canvas->bm.w-rtP.view_width)/2;
   if ((rtP.view_height!=0)&&(rtP.view_height<grd_canvas->bm.h))
      y_off=(grd_canvas->bm.h-rtP.view_height)/2;
   if ( (x_off>0) || (y_off>0) )
      return gr_alloc_sub_canvas(grd_canvas,x_off,y_off,
                               min(grd_canvas->bm.w,rtP.view_width),
                               min(grd_canvas->bm.h,rtP.view_height));
   return NULL;
}

static void do_angle_updates(void)
{
   if (rtP.inc_angle)
   {
      if (!rtP.flat_only)
      { rtP.pos.fac.ty+=rand()&0x7ff; rtP.pos.fac.tx+=rand()&0x7ff; }
      rtP.pos.fac.tz+=rand()&0x7ff;
   }
   else
   {
      if (!rtP.flat_only)
      { rtP.pos.fac.ty=rand()*2; rtP.pos.fac.tx=rand()*2; }
      rtP.pos.fac.tz=rand()*2;
   }
}

// actually update our position
static BOOL rtP_update_position(void)
{
   if (rtP.tests_left_at_this_source--<0)
   {  // get a new source
      Status("Find new location");
      if (!rendtest_pick_source_pos(&rtP.pos))
      {
         Status("Cant find source pos");
         return FALSE;
      }
      rtP.tests_left_at_this_source=rtP.tests_per_source;
   }
   else
   {  // raycast to a new position
      Location ray_dest, ray_hit;
      mxs_vector ray_end, delta;
      int tries=10;
      
      do {
         if (rtP.move_straight)
         {
            mxs_matrix move_mat;
            mxs_vector move_vec={1,0,0};
            if (tries<10) do_angle_updates(); // if moving straight isnt working, try angle change
            mx_ang2mat(&move_mat,&rtP.pos.fac);
            move_vec.el[0]=frand_hack();
            mx_mat_mul_vec(&delta,&move_mat,&move_vec);
         }
         else
            get_rand_vec(&delta,rtP.step_size);
         mx_add_vec(&ray_end,&rtP.pos.loc.vec,&delta);
         MakeHintedLocationFromVector(&ray_dest,&ray_end,&rtP.pos.loc);
         if (PortalRaycast(&rtP.pos.loc,&ray_dest,&ray_hit,0))
            break;                  // we did hit
      } while (--tries>0);
      if (tries<=0)                 // we failed, backup from hit
      {
         float mag=mx_mag_vec(&delta);
         mx_scaleeq_vec(&delta,mag/2.0);
         mx_addeq_vec(&delta,&ray_hit.vec);
         MakeHintedLocationFromVector(&ray_dest,&delta,&ray_hit);
      }
      do_angle_updates();
      rtP.pos.loc=ray_dest;
   }
   return TRUE;
}

// is it time to quit
static BOOL check_modal_quit(void)
{
   uiEvent ev;
   uiCookedKeyEvent *kev;
   uiMouseEvent *mev;
   if (modal_ui_get_event(&ev))
   {
      BOOL quit=FALSE;
      switch (ev.type)
      {
         case UI_EVENT_KBD_COOKED:
            kev=(uiCookedKeyEvent *)&ev;
            quit=(kev->code&KB_FLAG_DOWN); // quit if we see a down event
            break;
         case UI_EVENT_MOUSE:
            mev=(uiMouseEvent *)&ev;
            quit=(mev->action&(MOUSE_LDOWN|MOUSE_RDOWN));
            break;
      }
      return quit;
   }
   return FALSE;
}

void rendtest_run(BOOL new_run)
{
   grs_canvas *subc=NULL, *initial_canvas=grd_canvas;  // in case user doesnt want full screen
   char buf[100];            // for status messages
   BOOL first_frame=TRUE;

   if (new_run)
      rendtest_init_new();

   modal_ui_start(FALSE);  // sadly

   if ((subc=rendtest_maybe_build_subcanvas())!=NULL)
      gr_push_canvas(subc);

   while ((rtP.total_count<=0) || (rtP.tests_run<rtP.total_count))
   {
      if (new_run)
      {
         if (!rtP_update_position())
            break;
         write_rtP(cur_write_fname);
         portal_render_scene(&rtP.pos,1.0);      // now do the test
         sprintf(buf,"Frame %d of %d",++rtP.tests_run,rtP.total_count);
      }
      else if (first_frame)
      {
         portal_render_scene(&rtP.pos,1.0);      // now do the test
         sprintf(buf,"Rerun of Frame %d of %d",rtP.tests_run,rtP.total_count);
         first_frame=FALSE;
      }

      if (subc)
         gr_push_canvas(initial_canvas);
      Status(buf);
      if (subc)
         gr_pop_canvas();
      modal_ui_update();
      if (check_modal_quit())
         break;
   }
   modal_ui_end();
   if (subc)
   {
      gr_pop_canvas();
      gr_free_sub_canvas(subc);
   }
   if (rtP.rand_time) rtP.rand_seed=0;
   if (new_run)  // didnt crash, lose the record
      remove(cur_write_fname);
   gr_clear(0);
   sprintf(buf,"Rendered %d frames",rtP.tests_run);
   CommandExecute("redraw_all");
   _rtP_inuse=FALSE;
}

// the real command
void do_rendtest_cmd(int cnt)
{
   if (_rtP_inuse)
      Status("I think you have a dialog up");
   else if (gedit_editted)
      Status("Need portalized level to rendtest");
   else if (cnt==0)  // no arguments
      rendtest_dialog();
   else
   {
      rtP.total_count=cnt;
      _rtP_inuse=TRUE;
      rendtest_run(TRUE);
   }
}

static char last_rerun[255]=DEFAULT_FNAME;

void do_rendtest_rerun_cmd(char *fname)
{
   if ((fname!=NULL)&&(fname[0]!='\0'))
      strcpy(last_rerun,fname);
   if (read_rtP(last_rerun))
   {
      _rtP_inuse=TRUE;
      rendtest_run(FALSE);
   }
   else
   {
      char buf[300];
      sprintf(buf,"Cant open rendtest <%s>",last_rerun);
      Status(buf);
   }
}
#endif // SHIP
