// $Header: x:/prj/tech/libsrc/lgd3d/RCS/tmgrold.c 1.20 1997/11/10 23:48:38 KEVIN Exp $
#include <string.h>

#include <dbg.h>
#include <dev2d.h>
#include <lgassert.h>
#include <memall.h>
#include <r3ds.h>

#ifndef SHIP
#include <mprintf.h>
#define mono_printf(x) mprintf x
#else
#define mono_printf(x)
#endif

#include <tmgr.h>
#include <tdrv.h>

#ifdef MONO_SPEW
#define put_mono(c) \
do { \
   uchar *p_mono = (uchar *)0xb0082; \
   *p_mono = c;                      \
} while (0)
#else
#define put_mono(c)
#endif

static int tmgr_init(r3s_texture bm, int num_entries);
static void tmgr_shutdown(void);
static void tmgr_start_frame(int f);
static void tmgr_load_texture(r3s_texture bm);
static void tmgr_unload_texture(r3s_texture bm);
static void tmgr_set_texture(r3s_texture bm);
static void tmgr_set_texture_callback(void);
static void tmgr_stats(void);
static uint tmgr_bytes_loaded(void);

static texture_driver *g_driver;
static texture_manager tmgr;

// Look! it's our one and only export!
texture_manager *get_dopey_texture_manager(texture_driver *driver)
{
   g_driver = driver;
   tmgr.init = tmgr_init;
   tmgr.shutdown = tmgr_shutdown;
   tmgr.start_frame = tmgr_start_frame;
   tmgr.load_texture = tmgr_load_texture;
   tmgr.unload_texture = tmgr_unload_texture;
   tmgr.set_texture = tmgr_set_texture;
   tmgr.set_texture_callback = tmgr_set_texture_callback;
   tmgr.stats = tmgr_stats;
   tmgr.bytes_loaded = tmgr_bytes_loaded;
   return &tmgr;
}


#define BMF_HACK 0x80
#define TMGR_UNLOADED (-1)

static r3s_texture default_bm;
static r3s_texture callback_bm;
static grs_bitmap **bitmap=NULL;
static uchar **bits=NULL;
static int *frame;
static int *size;
static int cur_frame;
static int max_textures;
static int next_id;
static uint bytes_loaded;

static BOOL swapout = FALSE;
static BOOL flushing = FALSE;


// return amount of texture memory loaded this frame
static uint tmgr_bytes_loaded(void)
{
   return bytes_loaded;
}

const char *bad_bm_string = "Bitmap data out of synch.";

#define is_valid(bm) \
   (((uint )bm->bits < (uint )max_textures) && (bm == bitmap[(uint )bm->bits]))

#define validate_bm(bm) AssertMsg(is_valid(bm), bad_bm_string)

// eliminate bitmap pointer from list, mark
// vidmem as ready to be released.
static void do_unload(grs_bitmap *bm)
{
   int i = (int )bm->bits;
   validate_bm(bm);
   bm->flags &= ~BMF_LOADED;
   bm->bits = bits[i];
   bitmap[i] = NULL;
   frame[i] = TMGR_UNLOADED;
   if (bm->flags&BMF_HACK)
      // indicate this entry was hacked so we don't try to swap or release it later.
      bits[i] = NULL;   
   else
      // tell driver to disconnect bitmap from texture
      g_driver->unload_texture(i);

}

// really for sure get rid of this texture
static void release_texture(int i)
{
   grs_bitmap *bm = bitmap[i];
   if (bm!=NULL)
      do_unload(bm);
   frame[i] = 0;
   if (bits[i]!=NULL) {
      g_driver->release_texture(i);
      bits[i] = NULL;
   }
}

#define TMGR_SUCCESS 0
static int do_load(int i, grs_bitmap *bm, int s)
{
   if (!(bm->flags & BMF_HACK))
   {
      if (g_driver->load_texture(i, bm) == TDRV_FAILURE)
         return TMGR_FAILURE;
      bytes_loaded += s;
   }
   bits[i] = bm->bits;
   bitmap[i] = bm;
   size[i] = s;
   bm->bits = (uchar *)i;
   bm->flags |= BMF_LOADED;

   return TMGR_SUCCESS;
}

static void init_bitmap_list(void)
{
   int i;
   for (i=0; i<max_textures; i++) {
      bitmap[i] = NULL;
      frame[i] = 0;
      bits[i] = NULL;
      size[i] = 0;
   }
}

static void do_set_texture(r3s_texture bm)
{
   int n;

   validate_bm(bm);
   n = (int )bm->bits;
   frame[n] = cur_frame;
   if (bm->flags & BMF_HACK) {
      if (!(default_bm->flags&BMF_LOADED))
         tmgr_load_texture(default_bm);
      do_set_texture(default_bm);
      return;
   }

   g_driver->set_texture_id(n);
}

#define TMGR_INVALID_TEXTURE_SIZE -1

#define MAX_SIZE 9
static int alpha_size_table[MAX_SIZE*MAX_SIZE];
static int norm_size_table[MAX_SIZE*MAX_SIZE];

static int calc_size(grs_bitmap *bm)
{
   int i,j,w,h;
   int *size_table;

   if (gr_get_fill_type() == FILL_BLEND)
      size_table = alpha_size_table;
   else
      size_table = norm_size_table;
   

   for (i=0, w=1; i<MAX_SIZE; i++, w+=w)
      if (w==bm->w) break;
   for (j=0, h=1; j<MAX_SIZE; j++, h+=h)
      if (h==bm->h) break;
   if ((i>=MAX_SIZE)||(j>=MAX_SIZE)||
       (size_table[i*MAX_SIZE+j]==TMGR_INVALID_TEXTURE_SIZE))
   {
      mono_printf(("Unsupported texture size: w=%i h=%i\n", bm->w, bm->h));
      bm->flags|=BMF_HACK;
      return bm->w*bm->h;
   }
   return size_table[i*MAX_SIZE+j];
}


static int init_size_table(int *size_table)
{
   int i, j;
   int w, h;
   for (i=0,w=1; i<MAX_SIZE; w+=w, i++) {
      for (j=0,h=1; j<MAX_SIZE; h+=h, j++) {
         grs_bitmap *bm = gr_alloc_bitmap(BMT_FLAT8, 0, w, h);
         int result = g_driver->load_texture(0, bm);
         if (result == TDRV_FAILURE) { // can't load this size texture
            size_table[i*MAX_SIZE + j] = TMGR_INVALID_TEXTURE_SIZE;
         } else {
            int size = g_driver->release_texture(0)-result;
            size_table[i*MAX_SIZE + j] = size;
         }
         gr_free(bm);
      }
   }
   return TMGR_SUCCESS;
}

static int init_size_tables(void)
{
   int result;

   gr_set_fill_type(FILL_BLEND);
   result = init_size_table(alpha_size_table);
   gr_set_fill_type(FILL_NORM);

   if (result==TMGR_SUCCESS)
      result = init_size_table(norm_size_table);

   return result;
}

static void dump_all_unused_textures(void)
{
   int i;
   flushing = TRUE;
   swapout = FALSE;
   next_id = 0;
   put_mono('!');
   for (i=0; i<max_textures; i++) {
#if 0
      // end of list?
      if ((bm == NULL)&&
          (frame[i] != TMGR_UNLOADED))
         break;
#endif
      if (frame[i] < cur_frame)
         release_texture(i);
   }
}

static void dump_all_textures(void)
{
   int i;
   flushing = FALSE;
   swapout = FALSE;
   next_id = 0;
   for (i=0; i<max_textures; i++)
      release_texture(i);
}

void swapout_bitmap(int bm_size)
{
   int i,n;
   int max_age=1;

   for (i=0, n=-1; i<max_textures; i++) {
      int age;
      grs_bitmap *bm = bitmap[i];

      // end of list?
      if ((bm == NULL)&&
          (frame[i] != TMGR_UNLOADED))
         break;
      // correct size?
      if (size[i]!=bm_size)
         continue;
      // not hacked?
      if ((bits[i]==NULL)||
          ((bm!=NULL)&&(bm->flags & BMF_HACK))) {
         if (bm_size!=0)
            continue;
         // swap out hacked texture!
         n = i;
         break;
      }
      // old enough?
      age = cur_frame - frame[i];
      if (age <= max_age)
         continue;

      max_age = age;
      n = i;
   }

   if (n==-1) { // can't find suitable swapout; dump everything!
      mono_printf(("Can't find suitable swapout candidate; dumping all unused textures!\n"));


      // yo yo yo synch everything pending...
      // hopefully this prevents crashes and other weirdness
      g_driver->synchronize();
      dump_all_unused_textures();
   } else {
      release_texture(n);
      next_id = n;
   }
}


static void tmgr_unload_texture(r3s_texture bm)
{
   put_mono('e');
   if ((bm->flags&BMF_LOADED)==0) {
      put_mono('.');
      return;
   }

   if (is_valid(bm)) {
      g_driver->synchronize();
      if (flushing)
         release_texture((int )bm->bits);
      else 
         do_unload(bm);
   } else
      Warning(("bad bitmap pointer for unload!\n"));

   put_mono('.');
}

static void tmgr_load_texture(r3s_texture bm)
{
   int size;

   put_mono('d');
   size = calc_size(bm);

   if (swapout)
      swapout_bitmap(bm->flags&BMF_HACK ? 0 : size);


   do {   
      while (flushing && (frame[next_id]==cur_frame))
         if (++next_id == max_textures) {
            mono_printf(("Out of texture handles while in flush mode. I hate life."));
            g_driver->synchronize();
            dump_all_textures();
         }

      AssertMsg((next_id < max_textures) && (bitmap[next_id] == NULL) && (bits[next_id] == NULL),
         "Hmmm. this really shouldn't happen...\n");

      if (do_load(next_id, bm, size)!=TDRV_FAILURE)
         break;

      if (swapout) {
         mono_printf(("swapout _really_ failed; dumping all unused textures!"));

         // yo yo yo synch everything pending...
         // hopefully this prevents crashes.
         g_driver->synchronize();
         dump_all_unused_textures();
      } else if (flushing) {
         mono_printf(("Out of texture memory while in flush mode. I really hate life."));
         g_driver->synchronize();
         dump_all_textures();
      } else {
         mono_printf(("Out of texture memory; entring swapout mode.\n"));
         swapout = TRUE;
         swapout_bitmap(size);
      }
   } while (TRUE);
   if ((!swapout)&&(!flushing)&&
       (++next_id == max_textures)) {
      mono_printf(("Out of texture handles; entering swapout mode.\n"));
      swapout = TRUE;
   }
   put_mono('.');
}

static void tmgr_set_texture(r3s_texture bm)
{
   put_mono('f');
   if (bm==NULL) {
      g_driver->set_texture_id(TDRV_ID_SOLID);
      put_mono('.');
      return;
   }
   if (bm->flags & BMF_LOADED) {
      do_set_texture(bm);
      put_mono('.');
      return;
   }
   callback_bm = bm;
   g_driver->set_texture_id(TDRV_ID_CALLBACK);
   put_mono('.');
}

static void tmgr_set_texture_callback(void)
{
   put_mono('g');
   AssertMsg(callback_bm!=NULL, "can't load NULL bitmap!\n");
   tmgr_load_texture(callback_bm);
   put_mono('g');
   do_set_texture(callback_bm);
   put_mono('.');
}

static void tmgr_start_frame(int frame)
{
   static uint abl = 0;
   put_mono('c');
   if (frame != cur_frame) {
      cur_frame = frame;
      abl = (abl*7 + bytes_loaded)/8;
      if ((frame & 0xf) == 0)
         mono_printf(("avg bytes downloaded per frame: %i\n", abl));
      bytes_loaded = 0;
      if (flushing) {
         mono_printf(("completing flush...\n"));
         g_driver->synchronize();
         dump_all_textures();
      }
   }
   
   tmgr_set_texture(default_bm);
   put_mono('.');
}

static int tmgr_init(r3s_texture bm, int num_textures)
{
   if (bitmap!=NULL)
      tmgr_shutdown();

   put_mono('a');

   bytes_loaded = 0;
   max_textures = num_textures;
   bitmap = (grs_bitmap **)Malloc(num_textures * (sizeof(*bitmap) + sizeof(*frame) +
      sizeof(*size) + sizeof(*bits)));
   frame = (int *)(&bitmap[num_textures]);
   size = (int *)(&frame[num_textures]);
   bits = (uchar **)(&size[num_textures]);
   init_bitmap_list();
   init_size_tables();
   default_bm = bm;
   swapout = FALSE;
   next_id = 0;
   put_mono('.');
   return TMGR_SUCCESS;
}

static void tmgr_shutdown(void)
{
   if (bitmap == NULL)
      return;

   put_mono('b');
   dump_all_textures();

   Free(bitmap);

   bitmap = NULL;
   put_mono('.');
}




#ifndef SHIP

#define STATINFO_NUM_SIZES 20

typedef struct {
   int size;
   int real;
   int fake;
} tmgr_stat_info;

static tmgr_stat_info statinfo[STATINFO_NUM_SIZES];

static void clear_stats(void)
{
   int i;
   for (i=0; i<STATINFO_NUM_SIZES; i++)
      statinfo[i].size = 0;
}

static void tmgr_stats(void)
{
   int i, faked=0, real=0;

   clear_stats();
   for (i = 0; i<max_textures; i++)
   {
      grs_bitmap *bm = bitmap[i];
      int j, s;
      if (bm==NULL)
         continue;

      if (frame[i]!=cur_frame)
         continue;

      s = size[i];
      if (bm->flags & BMF_HACK)
         faked += s;
      else
         real += s;
      for (j=0; j<STATINFO_NUM_SIZES; j++) {
         if (statinfo[j].size == 0) {
            statinfo[j].size = s;
            if (bm->flags&BMF_HACK) {
               statinfo[j].real = 0;
               statinfo[j].fake = 1;
            } else {
               statinfo[j].real = 1;
               statinfo[j].fake = 0;
            }
            break;
         }
         if (statinfo[j].size == s) {
            if (bm->flags&BMF_HACK)
               statinfo[j].fake++;
            else
               statinfo[j].real++;
            break;
         }
      }
      if (j==STATINFO_NUM_SIZES)
         Warning(("Too few stat texture sizes available.\n"));
   }
   mono_setxy(0,0);
   mprint("Texture Manager Stats:       \n");
   mprintf("total faked: %i total real: %i combined: %i       \n",
      faked, real, faked+real);
   for (i=0; i<STATINFO_NUM_SIZES; i++) {
      if (statinfo[i].size==0)
         break;
      mprintf("size %i: %i real; %i fake      \n",
         statinfo[i].size, statinfo[i].real, statinfo[i].fake);
   }
}
#else
static void tmgr_stats(void) {}
#endif

