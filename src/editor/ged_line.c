/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/ged_line.c,v 1.8 2000/02/19 13:10:45 toml Exp $
// line overlay system for gedit

// this system is responsible for overlaying lines in ged wireframe views
// it contains methods for adding lines to current lists, activating and deactivating them,
//   writing lists to files, reading from files, and so on

#include <stdio.h>
#include <string.h>

#include <lg.h>
#include <matrix.h>
#include <mprintf.h>

#include <command.h>
#include <brrend.h>
#include <gedit.h>
#include <ged_line.h>
#include <uiedit.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

// what channel do new lines go to
int ged_line_load_channel=0;
// what channels are currently being watched
int ged_line_view_channels=1;
// do we auto add the current channel on loads
bool ged_line_autoremote=TRUE;

// internal flags
#define LINE_FLG_USER_MASK (0x0fff)
#define LINE_FLG_POINT (1<<14)  // just a point... woo woo
#define LINE_FLG_INUSE (1<<15)  // not a straight line, but a curved one

typedef struct {
   mxs_vector src, dst;        // start and end of line
   uchar      r,g,b;           // rgb colors
   int        pal;             // cur palettized value
   short      flags;           // someday, we might use these...
   short      channel;         // which channel is this line in?
} gedLineData;                 // look, 32 bytes a piece

static gedLineData *allLines=NULL;
static int          allLineLen=0, allLineAlloced=0;
#define LINE_ALLOC_SIZE 128

#define lineCurChannel(p) (((p)->channel)&(ged_line_view_channels))
#define lineInUse(p)      (((p)->flags)&LINE_FLG_INUSE)
#define lineIdxToPtr(i)   (&(allLines[i]))

////////////////////
// file interface

// load a line data file into channel
int gedLineReadFileToChannel(char *fname)
{
   FILE *fp;

   if ((fp=fopen(fname,"r"))==NULL)
      return -1;
   if (ged_line_autoremote)
      ged_line_view_channels=(1<<ged_line_load_channel);
   while (!feof(fp))
   {
      mxs_vector v1, v2;
      int color=0, flags=0, cnt, c1, c2, c3, s1;

      cnt=fscanf(fp,"GlN:%f %f %f  %f %f %f  %d %d %d %d\n",&v1.x,&v1.y,&v1.z,&v2.x,&v2.y,&v2.z,&s1,&c1,&c2,&c3);
      flags&=LINE_FLG_USER_MASK;
      if (cnt<3)
         continue;
      if (cnt<6)
      {
         flags|=LINE_FLG_POINT;
         gedLineAddPal(&v1,NULL,flags,color);
      }
      else
      {
         if (cnt==7) // just a color
            gedLineAddPal(&v1,&v2,flags,s1);
         else if (cnt==8) // just flags and single color - treat as pal entry
            gedLineAddPal(&v1,&v2,s1,c1);
         else if (cnt==9) // just an rgb
            gedLineAddRGB(&v1,&v2,flags,s1,c1,c2);
         else if (cnt==10)
            gedLineAddRGB(&v1,&v2,s1,c1,c2,c3);
         else
            Warning(("What is up? cnt>10 in gedLineReadFileToChannel\n"));
      }
   }
   fclose(fp);
   return ged_line_load_channel;  // channel used
}

////////////////////
// writing out data

// dump channels to a file
int gedLineWriteChannelsToFile(int channel_mask, char *fname)
{
   FILE *fp;
   int i;

   if ((fp=fopen(fname,"w"))==NULL)
      return -1;
   
   for (i=0; i<allLineLen; i++)
   {
      gedLineData *rv=lineIdxToPtr(i);
      if (lineInUse(rv)&&(channel_mask&(1<<rv->channel)))
      {  // now - the gruesome what type of line so output what stuff
         if (rv->flags&LINE_FLG_POINT)  // first, the base
            fprintf(fp,"GlN:%f %f %f",rv->src.x,rv->src.y,rv->src.z);
         else
            fprintf(fp,"GlN:%f %f %f  %f %f %f",rv->src.x,rv->src.y,rv->src.z,rv->dst.x,rv->dst.y,rv->dst.z);
         fprintf(fp,"%d",rv->flags);
         if ((rv->r|rv->g|rv->b)==0) // all zeroes
            fprintf(fp,"%d",rv->pal);
         else
            fprintf(fp,"%d %d %d",rv->r,rv->g,rv->b);
         fprintf(fp,"\n");
      }
   }
   fclose(fp);
   return channel_mask;
}

////////////////////
// current list maintenance

// internal tool to find a free line to use
static gedLineData *find_free_line(void)
{
   gedLineData *rv=NULL;
   int i;
   
   for (i=0; i<allLineLen; i++)
      if (lineInUse(lineIdxToPtr(i))==0)
      {
         rv=lineIdxToPtr(i);
         break;
      }
   if (rv==NULL)   // none free, need to pick a new one, or allocate more
   {
      if (allLineLen>=allLineAlloced)
      {
         allLineAlloced+=LINE_ALLOC_SIZE;
         if (allLineLen==0)
            allLines=(gedLineData *)Malloc(sizeof(gedLineData)*allLineAlloced);
         else
            allLines=(gedLineData *)Realloc(allLines,sizeof(gedLineData)*allLineAlloced);
      }
      rv=lineIdxToPtr(allLineLen);
   }
   memset(rv,0,sizeof(gedLineData));
   rv->flags|=LINE_FLG_INUSE;
   if (i==allLineLen)
      allLineLen++;
   return rv;
}

// pretty tough one, eh... maybe make this a #define at some point
static void remove_line(int idx)
{
   lineIdxToPtr(idx)->flags&=~LINE_FLG_INUSE;
}

static gedLineData *_gedLineInternalAdd(const mxs_vector *p1, const mxs_vector *p2, int flags)
{
   gedLineData *us=find_free_line();
   us->src=*p1;
   us->dst=*p2;
   us->flags|=flags;
   us->channel=(1<<ged_line_load_channel);
   return us;
}

// external API to add a line - need to set global channel first
void gedLineAddPal(const mxs_vector *p1, const mxs_vector *p2, int flags, int color)
{
   gedLineData *us=_gedLineInternalAdd(p1,p2,flags);
   us->pal=color;
   us->r=0x00; us->g=0x00; us->b=0x00;
}

void gedLineAddRGB(const mxs_vector *p1, const mxs_vector *p2, int flags, int r, int g, int b)
{
   gedLineData *us=_gedLineInternalAdd(p1,p2,flags);
   us->pal=gedLineConvertRGB(r,g,b);
   us->r=r; us->g=g; us->b=b;
}

int gedLineConvertRGB(int r, int g, int b)
{
   return uieditFixupColor(uiRGB(r,g,b));
}

////////////////////
// channel/setup control

void gedDeleteChannelLines(int channel)
{
   int i;
   for (i=0; i<allLineLen; i++)
      if (lineInUse(lineIdxToPtr(i)))
         if (lineIdxToPtr(i)->channel==channel)
            remove_line(i);
}

/////////////////////
// actual rendering

// uses current active channel list, iterates over all lines, rendering
// called from w/in a 3d context/brush draw setup
void gedLineRenderAll(void)
{
   int i;
   for (i=0; i<allLineLen; i++)
   {
      gedLineData *rv=lineIdxToPtr(i);
      if (lineInUse(rv)&&lineCurChannel(rv))
      {
         brushLineDraw(&rv->src,&rv->dst,rv->pal);

         // draw arrow
         if (rv->flags & LINE_FLG_DIR)
         {
            mxs_vector arrow_point;
            mxs_vector arrow_dir;
            mxs_vector arrow_base;
            mxs_vector arrow_width;
            mxs_vector arrow_end;
            mxs_vector z_axis;

            mx_add_vec(&arrow_point, &rv->src, &rv->dst);
            mx_scaleeq_vec(&arrow_point, 0.5);

            mx_sub_vec(&arrow_dir, &rv->dst, &rv->src);
            
            if (mx_mag2_vec(&arrow_dir) < 0.001)
               continue;

            mx_normeq_vec(&arrow_dir);
            mx_scale_add_vec(&arrow_base, &arrow_point, &arrow_dir, -2.0);

            mx_unit_vec(&z_axis, 2);

            mx_cross_vec(&arrow_width, &z_axis, &arrow_dir);

            mx_add_vec(&arrow_end, &arrow_base, &arrow_width);
            brushLineDraw(&arrow_point, &arrow_end, rv->pal);

            mx_scaleeq_vec(&arrow_width, -1.0);

            mx_add_vec(&arrow_end, &arrow_base, &arrow_width);
            brushLineDraw(&arrow_point, &arrow_end, rv->pal); 
         }
      }
   }
}

void gedLinePick(void)
{
   // hmmmmmmm
}

////////////////////
// setup/closedown

static void do_switch_channel(int channel_id)
{
   ged_line_view_channels=(1<<channel_id);
}

static void do_load_file(char *fname)
{
   gedLineReadFileToChannel(fname);
}

static void do_clear_channel(int channel_id)
{
   gedDeleteChannelLines(channel_id);
}

static Command gedLineKeys[]=
{
   { "line_autoremote", TOGGLE_BOOL, &ged_line_autoremote, "toggle autochannel switch in line display" },
   { "line_switchchannel", FUNC_INT, do_switch_channel, "set view to single channel id" },
   { "line_viewchannel", VAR_INT, &ged_line_view_channels, "set bitmask of currently visible line channels" },
   { "line_loadchannel", VAR_INT, &ged_line_load_channel, "set channel id to load to" },
   { "line_clearchannel", FUNC_INT, do_clear_channel, "clear all lines in channel id" },
   { "line_loadfile", FUNC_STRING, do_load_file, "load a file to current line channel" },
};

void gedLineInit(void)
{
   COMMANDS(gedLineKeys,HK_BRUSH_EDIT);
}

void gedLineTerm(void)
{
   if (allLineAlloced)
      Free(allLines);
   allLines=NULL;
   allLineLen=allLineAlloced=0;
}
