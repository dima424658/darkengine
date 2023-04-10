/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/framewrk/rn_cnvrt.c,v 1.13 2000/02/19 13:16:33 toml Exp $
// converters for the resname system
// really these want to all be DLL's or something

#include <limits.h>
#include <string.h>
#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>

#include <mprintf.h>
#include <lg.h>
#include <gfile.h>

#include <lresname.h>
#include <lress.h>
#include <rn_cnvrt.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

// I guess this should probably be in a header somewhere
#ifndef __WATCOMC__
# if defined(__OS2__) || defined(__NT__)
#   define NAME_MAX  255      /* maximum filename for HPFS or NTFS */
#   define PATH_MAX  259 /* maximum length of full pathname excl. '\0' */
# else
#   define NAME_MAX  12    /* 8 chars + '.' +  3 chars */
#   define PATH_MAX  143 /* maximum length of full pathname excl. '\0' */
# endif
#endif

#define MAX_IMG_WIDTH  1024
#define MAX_IMG_HEIGHT 1024

typedef struct {
   char ext[5];     // file extension
   int  sys_idx;    // which converter to use, by Index
} ConvertInfo;

// NOTE: keep extensions together
// ie. once it has scanned an extension once
// as soon as it changes, it returns out
ConvertInfo convert_table[]=
{
  { "bin", _RN_RAW },
  { "pcx", _RN_IMAGE },
  { "pcx", _RN_PAL },
  { "gif", _RN_IMAGE },
  { "gif", _RN_PAL },
  { "bmp", _RN_IMAGE },
  { "bmp", _RN_PAL },
  { "cel", _RN_IMAGE },
  { "cel", _RN_PAL },
  //  { "pal", _RN_PAL },
  { "wav", _RN_SND },
  { "voc", _RN_SND },
  { "cal", _RN_RAW },
  { "mi", _RN_RAW },
  { "mc", _RN_RAW },
};

#define CV_TABLE_SIZE (sizeof(convert_table)/sizeof(convert_table[0]))

static BOOL  _rn_secret_clut=FALSE;
static uchar _rn_cnvrt_secret_image_clut[256];

void (*pResnamePaletteCallback)(uchar *pal, void *bm)=NULL;

// already have a valid ipal
void build_rn_clut(uchar *ipal, uchar *pal_raw)
{
   int i;
   if (ipal==NULL) { _rn_secret_clut=FALSE; return; }
   _rn_cnvrt_secret_image_clut[0]=0;
   for (i=1; i<256; i++)
   {  // get color from pal_raw, determine its value in pal_vis
      int r=pal_raw[(3*i)+0], g=pal_raw[(3*i)+1], b=pal_raw[(3*i)+2];
      _rn_cnvrt_secret_image_clut[i]=ipal[gr_index_brgb(gr_bind_rgb(r,g,b))];
#ifdef TALK
      mprintf("clut %d: Turned %x %x %x into %d (%x %x %x)\n",i,r,g,b,_rn_cnvrt_secret_image_clut[i],
         grd_pal[_rn_cnvrt_secret_image_clut[i]*3+0],
         grd_pal[_rn_cnvrt_secret_image_clut[i]*3+1],
         grd_pal[_rn_cnvrt_secret_image_clut[i]*3+2]);
#endif      
   }
   _rn_secret_clut=TRUE;
}

static void clut_my_memory_please(int len, uchar *psrc, uchar *clut)
{
   uchar *p1, *p2;

   p2=psrc+len;
   for (p1=psrc; p1<p2; p1++)
      *p1=clut[*p1];
}

// this reads the file, stores off the first image in it as the named res
// for now, that means it mallocs space for the grs_bitmap + the bits
// and then builds it.  this means in DromEd there will be a whole extra level
// since we then have to copy into the portal speexcial memory
void *_RN_ConvertImage(char *name)
{
   Datapath dp;
   GfileInfo g;
   
   DatapathClear(&dp);
   if (GfileRead(&g,name,&dp))
   {
      grs_bitmap *bm=NULL;
      Point p={0,0};
      uchar *psrc;
      int w,h,i,flags=0;
      bool rv;
      Rect r;

      rv=GfileFindImage(&g.bm,p,&r,254);
      if (rv)
      {
         bm=(grs_bitmap *)Malloc(sizeof(grs_bitmap)+RectWidth(&r)*RectHeight(&r));
         psrc=((uchar *)bm)+sizeof(grs_bitmap);
         GfileGetImage(&g.bm,&r,psrc);
         w=RectWidth(&r); h=RectHeight(&r);
      }
      else if ((g.bm.w<=MAX_IMG_WIDTH)&&(g.bm.h<=MAX_IMG_HEIGHT))
      {            
         bm=(grs_bitmap *)Malloc(sizeof(grs_bitmap)+g.bm.w*g.bm.h);
         psrc=((uchar *)bm)+sizeof(grs_bitmap);
         memcpy(psrc,g.bm.bits,g.bm.w*g.bm.h);
         w=g.bm.w; h=g.bm.h;
      }
      if (_rn_secret_clut)    // clut psrc here if needed
         clut_my_memory_please(w*h,psrc,_rn_cnvrt_secret_image_clut);
      
      // @OPTIMIZE: write this (all) in asm, this is particular
      for (i=0; i<w*h; i++) if (psrc[i]==0) { flags=BMF_TRANS; break; }
      // @TODO: we _cannot_ ship this (the above) this way, it must be fixed
      
      gr_init_bitmap(bm,psrc,BMT_FLAT8,flags,w,h);
      if (bm&&pResnamePaletteCallback)
         (*pResnamePaletteCallback)(g.ppall,bm);
      GfileFree(&g);
      return bm;      // could still be NULL
   }
   return NULL;
}

void *_RN_ConvertPal(char *name)
{
   Datapath dp;
   GfileInfo g;
   
   DatapathClear(&dp);
   if (GfileRead(&g,name,&dp))
   {
      uchar *ppal;
      ppal=(uchar *)Malloc(sizeof(uchar)*768);
      memcpy(ppal,g.ppall,sizeof(uchar)*768);
      GfileFree(&g);
      return ppal;
   }
   return NULL;
}

void *_RN_ConvertRaw(char *name)
{
   uchar *data;
   int hnd, len;
   if ((hnd=open(name,O_RDONLY|O_BINARY))==-1)
      return NULL;
   len=lseek(hnd,0,SEEK_END);
   lseek(hnd,0,SEEK_SET);
   data=(uchar *)Malloc(len);
   if (data!=NULL)
      if (!(read(hnd,data,len)==len))
       { Free(data); data=NULL; }  // didnt get the right amount of data
   close(hnd);
   return data;
}

void *_RN_Unknown(char *name)
{
   Warning(("Cant convert %s - unknown extension\n",name));
   return NULL;
}

typedef void *(*RN_ConvertCallback)(char *name);
   
// from lresname.h, UNKNOWN is 0, IMAGE is 1, ANIM is 2, PAL is 3, SND is 4
RN_ConvertCallback cvrt_calls[]=
{
   _RN_ConvertRaw,
   _RN_ConvertImage,
   _RN_Unknown,
   _RN_ConvertPal,
   _RN_ConvertRaw    // for now, we just move the data into memory for sound
};

// returns NULL if no extension, else beginning of extension within fname, w/o "."
// TODO: really, should make sure the . is past the last slash
//   so you can do moose/camel.dir/harold/elephant right
char *extension_p(char *fname)
{
   char *t;
   t=strrchr(fname,'.');
   if (t==fname+strlen(fname)-1)  // last character means, basically, no extension
      return NULL;
   else if (t==fname) // first character
      return NULL;
   else if (t==NULL)  // no .
      return NULL;
   else
   {  // we should now make sure there are no slashes past here in the string
      if (strchr(t+1,'\\')||strchr(t+1,'/'))
         return NULL;
      return t+1;     // ah-ha... we are real, we found a real . cool
   }
}

// add ext to base_name into fname, deal with double dot fun
static char *add_extension(char *fname, char *base_name, char *ext)
{
   strcpy(fname,base_name);
   if (*(fname+strlen(fname)-1)!='.')  // no dot at the end
      strcat(fname,".");
   strcat(fname,ext);
   return fname;
}

void *RN_ConvertFile(char *name, int allowed, int *type)
{
   int   i;
   char *s, fname[PATH_MAX], *use_name=name;
   void *chunk;
   bool found=FALSE;       // have we seen our string yet?

   // do horrible "do i have an extension or dont i" code
   if ((s=extension_p(name))==NULL)
      use_name=fname;      // try all extensions, use_fname cache
   
   for (i=0; i<CV_TABLE_SIZE; i++)
   {
      int our_type=convert_table[i].sys_idx;
      if ((1<<our_type)&allowed)
         if ((s==NULL)||
             (strnicmp(convert_table[i].ext,s,strlen(convert_table[i].ext))==0))
         {
            if (s==NULL)
               add_extension(fname,name,convert_table[i].ext);
            if ((chunk=(*cvrt_calls[our_type])(use_name))!=NULL)
            {
               *type=1<<our_type;
               return chunk;
            }
            found=TRUE;    // ok, weve passed the strnicmp once, so weve seen us
         }
         else if (found)   // weve already seen our string once
            return NULL;
   }
   return NULL;
}
