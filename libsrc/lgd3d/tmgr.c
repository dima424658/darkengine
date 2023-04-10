// $Header: x:/prj/tech/libsrc/lgd3d/RCS/tmgr.c 1.33 1998/06/30 18:36:56 buzzard Exp $
#include <string.h>

#include <dbg.h>
#include <dev2d.h>
#include <lgassert.h>
#include <memall.h>
#include <r3ds.h>

#ifndef SHIP
#include <mprintf.h>
static BOOL spew;
#define mono_printf(x) \
do {              \
   if (spew)      \
      mprintf x ; \
} while (0)
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

static int tmgr_init(r3s_texture bm, int num_entries, int flags);
static void tmgr_shutdown(void);
static void tmgr_start_frame(int f);
static void tmgr_end_frame(void);
static void tmgr_load_texture(r3s_texture bm);
static void tmgr_unload_texture(r3s_texture bm);
static void tmgr_reload_texture(r3s_texture bm);
static void tmgr_set_texture(r3s_texture bm);
static void tmgr_set_texture_callback(void);
static void tmgr_stats(void);
static uint tmgr_bytes_loaded(void);
static BOOL tmgr_get_utilization(float *utilization);
static uchar *tmgr_set_clut(uchar *);
static void tmgr_restore_bits(grs_bitmap *bm);

static texture_driver *g_driver;
static texture_manager tmgr;

// Look! it's our one and only export!
texture_manager *get_dopey_texture_manager(texture_driver *driver)
{
   g_driver = driver;
   tmgr.init = tmgr_init;
   tmgr.shutdown = tmgr_shutdown;
   tmgr.start_frame = tmgr_start_frame;
   tmgr.end_frame = tmgr_end_frame;
   tmgr.load_texture = tmgr_load_texture;
   tmgr.unload_texture = tmgr_unload_texture;
   tmgr.set_texture = tmgr_set_texture;
   tmgr.set_texture_callback = tmgr_set_texture_callback;
   tmgr.stats = tmgr_stats;
   tmgr.bytes_loaded = tmgr_bytes_loaded;
   tmgr.get_utilization = tmgr_get_utilization;
   tmgr.set_clut = tmgr_set_clut;
   tmgr.restore_bits = tmgr_restore_bits;
   tmgr.reload_texture = tmgr_reload_texture;
   return &tmgr;
}


#define BMF_HACK 0x80
#define TMGR_UNLOADED (-1)

typedef struct tmgr_texture_info tmgr_texture_info;

struct tmgr_texture_info {
   grs_bitmap *bitmap; // back ptr to bitmap
   uchar *bits;        // bits ptr from bitmap
   int frame;          // frame last used
   uchar *clut;        // ptr to clut used when loaded
   int next;           // index of next texture of this size
   int size;
   ulong cookie;       // cookie from tmgr. Can reload when these match!
};

typedef struct tmap_chain {
   int size;
   int head;
} tmap_chain;

static tmgr_texture_info *texinfo = NULL;
static tmap_chain *chain_list = NULL;
static int num_texture_sizes = 0;

static uchar *texture_clut;
static r3s_texture default_bm;
static r3s_texture callback_bm;

static int cur_frame;
static int max_textures;
static int next_id;
static uint bytes_loaded; // bytes loaded this frame
static uint texmem_used;
static uint texmem_loaded;

static BOOL swapout = FALSE;
static BOOL overloaded = FALSE;
static BOOL in_frame = FALSE;


// returns old clut
static uchar *tmgr_set_clut(uchar *clut)
{
   uchar *retval = texture_clut;
   texture_clut = clut;
   return retval;
}

// return amount of texture memory loaded this frame
static uint tmgr_bytes_loaded(void)
{
   return bytes_loaded;
}

static BOOL tmgr_get_utilization(float *utilization)
{
   if (overloaded) { // we're really overutilized
      *utilization = 1.0;
      return TRUE;
   }

   if (!swapout)
      // we're not loaded to capacity yet; stats wouldn't be valid
      return FALSE;

   *utilization = ((float )texmem_used)/texmem_loaded;
   return TRUE;
}

const char *bad_bm_string = "Bitmap data out of synch.";

#define is_valid(bm) \
   ((bm != NULL) && \
    ((uint )bm->bits < (uint )max_textures) && \
    (bm == texinfo[(uint )bm->bits].bitmap))

#define validate_bm(bm) AssertMsg(is_valid(bm), bad_bm_string)

static void tmgr_restore_bits(grs_bitmap *bm)
{
   if ((bm == NULL)||((bm->flags&BMF_LOADED)==0))
      return;
   validate_bm(bm);
   bm->bits = texinfo[(uint )bm->bits].bits;
}

// eliminate bitmap pointer from list, mark
// vidmem as ready to be released.
static void do_unload(grs_bitmap *bm)
{
   int i;
   tmgr_texture_info *info;

   validate_bm(bm);

   i = (int )bm->bits;
   info = &texinfo[i];

   bm->flags &= ~BMF_LOADED;
   bm->bits = info->bits;
   info->bitmap = NULL;
   info->frame = TMGR_UNLOADED;

   // tell driver to disconnect bitmap from texture
   g_driver->unload_texture(i);
}


// really for sure get rid of this texture
static void release_texture(int i)
{
   tmgr_texture_info *info = &texinfo[i];
   grs_bitmap *bm = info->bitmap;
   if (bm!=NULL)
      do_unload(bm);
   info->frame = 0;
   if (info->bits!=NULL) {
      g_driver->release_texture(i);
      info->bits = NULL;
      texmem_loaded -= info->size;
   }
}

static void set_flags(tdrv_texture_info *info)
{
   info->flags = 0;
   if (gr_get_fill_type() == FILL_BLEND) {
      info->flags |= TF_ALPHA;
   }

   if ((!(info->flags&TF_ALPHA)) && (info->bm->flags&BMF_TRANS)) {
      info->flags |= TF_TRANS;
   }

   if (info->bm->flags & BMF_LOADED)
      info->bits = texinfo[info->id].bits;
   else
      info->bits = info->bm->bits;
}


#define TMGR_SUCCESS 0
static int do_load(tdrv_texture_info *info)
{
   grs_bitmap *bm = info->bm;
   tmgr_texture_info *tmgr_info = &texinfo[info->id];

   AssertMsg((!swapout)||(tmgr_info->size == info->size), "do_load(): size mismatch.");

   if (tmgr_info->bits != NULL) {

      AssertMsg(info->cookie == tmgr_info->cookie,
         "do_load(): cookie doesn't match original.");

      g_driver->reload_texture(info);
   } else {
      if (g_driver->load_texture(info) == TDRV_FAILURE) 
         return TMGR_FAILURE;
      texmem_loaded += info->size;
   }
   bytes_loaded += info->size;
   tmgr_info->clut = texture_clut;
   tmgr_info->bits = bm->bits;
   tmgr_info->bitmap = bm;
   tmgr_info->cookie = info->cookie;
   tmgr_info->size = info->size;
   bm->bits = (uchar *)info->id;
   bm->flags |= BMF_LOADED;

   return TMGR_SUCCESS;
}

static void init_bitmap_list(void)
{
   int i;
   tmgr_texture_info *tmgr_info = texinfo;

   for (i=0; i<max_textures; i++) {
      tmgr_info->bitmap = NULL;
      tmgr_info->bits = NULL;
      tmgr_info->frame = 0;
      tmgr_info->size = 0;
      tmgr_info->clut = NULL;
      tmgr_info++;
   }
}

static void do_set_texture(r3s_texture bm)
{
   int i;
   tmgr_texture_info *info;

   if (bm->flags & BMF_HACK) {
      if (!(default_bm->flags&BMF_LOADED))
         tmgr_load_texture(default_bm);
      do_set_texture(default_bm);
      return;
   }

   validate_bm(bm);

   i = (int )bm->bits;
   info = &texinfo[i];

   if (info->frame != cur_frame) {
      texmem_used += info->size;
      info->frame = cur_frame;
   }
   g_driver->set_texture_id(i);
}

#define TMGR_INVALID_TEXTURE_SIZE ((tmap_chain *)-1)
#define MAX_SIZE 9
static tmap_chain *alpha_size_table[MAX_SIZE*MAX_SIZE];
static tmap_chain *norm8_size_table[MAX_SIZE*MAX_SIZE];
static tmap_chain *norm16_size_table[MAX_SIZE*MAX_SIZE];

static tmap_chain *calc_size(tdrv_texture_info *info)
{
   int i,j,w,h;
   tmap_chain **size_table;
   tmap_chain *chain;
   grs_bitmap *bm = info->bm;

   set_flags(info);
   info->scale_w = info->scale_h = 0;

   
   if (info->flags & TF_ALPHA)
      size_table = alpha_size_table;
   else if (bm->type == BMT_FLAT8)
      size_table = norm8_size_table;
   else
      size_table = norm16_size_table;
   

   for (i=0, w=1; i<MAX_SIZE; i++, w+=w)
      if (w==bm->w) break;
   for (j=0, h=1; j<MAX_SIZE; j++, h+=h)
      if (h==bm->h) break;

   if ((i>=MAX_SIZE)||(j>=MAX_SIZE)) {
      mono_printf(("Unsupported texture size: w=%i h=%i\n", bm->w, bm->h));
      bm->flags|=BMF_HACK;
   }

   else while ((chain = size_table[i*MAX_SIZE+j])==NULL)
   {
      if (i<j) {
         i++, info->scale_w++;
      } else {
         j++, info->scale_h++;
         if (j<MAX_SIZE)
            continue;

         mono_printf(("Unsupported texture size: w=%i h=%i\n", bm->w, bm->h));
         bm->flags|=BMF_HACK;
         break;
      }
   }
   if (chain!=NULL)
      info->size = chain->size;

   info->w = bm->w << info->scale_w;
   info->h = bm->h << info->scale_h;
   return chain;
}


static void init_size_table(tmap_chain **size_table, int type)
{
   int i, j;
   int w, h;
   tdrv_texture_info info;
   info.id = 0;

   for (i=0,w=1; i<MAX_SIZE * MAX_SIZE; w+=w, i+=MAX_SIZE) {
      for (j=0,h=1; j<MAX_SIZE; h+=h, j++) {
         info.bm = gr_alloc_bitmap(type, 0, w, h);
         set_flags(&info);
         info.scale_w = info.scale_h = 0;
         info.w = info.bm->w;
         info.h = info.bm->h;
         info.size = 0;
         g_driver->cook_info(&info);
         if (g_driver->load_texture(&info) == TDRV_FAILURE)
         { // can't load this size texture
            size_table[i + j] = TMGR_INVALID_TEXTURE_SIZE;
         } else {
            size_table[i + j] = (tmap_chain *)info.size;
            g_driver->release_texture(info.id);
         }
         gr_free(info.bm);
      }
   }
}

#define MAX_TMAP_SIZES 3*MAX_SIZE*MAX_SIZE

// here we're just trying to figure out how many distinct texture sizes there are...
static int get_num_sizes(tmap_chain **size_list, int num_sizes, tmap_chain **size_table)
{
   int i,j;
   for (i=0; i<MAX_SIZE*MAX_SIZE; i++) {
      if (size_table[i]==TMGR_INVALID_TEXTURE_SIZE)
         continue;

      for (j=0; j<num_sizes; j++) {
         if (size_list[j]==size_table[i])
            break;
      }
      if (j==num_sizes)
         size_list[num_sizes++] = size_table[i];
   }
   return num_sizes;
}

// here we actually munge the size table to have real (tmap_chain *)'s
static int munge_table(int num_sizes, tmap_chain **size_table)
{
   int i,j;
   for (i=0; i<MAX_SIZE*MAX_SIZE; i++) {
      if (size_table[i]==TMGR_INVALID_TEXTURE_SIZE) {
         size_table[i] = NULL;
         continue;
      }
      for (j=0; j<num_sizes; j++) {
         if (chain_list[j].size==(int )size_table[i])
            break;
      }
      if (j==num_sizes) {
         chain_list[num_sizes].head = -1;
         chain_list[num_sizes].size = (int )size_table[i];
         num_sizes++;
      }
      size_table[i] = &chain_list[j];
   }
   return num_sizes;
}


static void munge_size_tables(void)
{
   int num_sizes;
   tmap_chain **size_list;

   size_list = (tmap_chain **)Malloc(MAX_TMAP_SIZES*sizeof(tmap_chain *));
   num_sizes = get_num_sizes(size_list, 0, alpha_size_table);
   num_sizes = get_num_sizes(size_list, num_sizes, norm8_size_table);
   num_sizes = get_num_sizes(size_list, num_sizes, norm16_size_table);
   Free(size_list);
   chain_list = (tmap_chain *)Malloc(num_sizes * sizeof(tmap_chain));
   num_texture_sizes = munge_table(0, alpha_size_table);
   num_texture_sizes = munge_table(num_texture_sizes, norm8_size_table);
   num_texture_sizes = munge_table(num_texture_sizes, norm16_size_table);
   AssertMsg(num_texture_sizes==num_sizes,
      "munge_size_tables(): detected bug in get_num_sizes() or munge_table()");
}


static int init_size_tables(void)
{
   gr_set_fill_type(FILL_BLEND);
   init_size_table(alpha_size_table, BMT_FLAT8);
   gr_set_fill_type(FILL_NORM);

   init_size_table(norm8_size_table, BMT_FLAT8);
   init_size_table(norm16_size_table, BMT_FLAT16);

   munge_size_tables();
   return TMGR_SUCCESS;
}

static void dump_all_textures(void)
{
   int i;
   BOOL end_and_start = in_frame;

   swapout = FALSE;
   next_id = 0;
   if (end_and_start)
      g_driver->end_frame();

   g_driver->synchronize();

   for (i=0; i<max_textures; i++)
      release_texture(i);

   if (end_and_start)
      g_driver->start_frame(cur_frame);

   overloaded = TRUE;

   // reset all the size chains
   for (i=0; i<num_texture_sizes; i++)
      chain_list[i].head=-1;
}

void swapout_bitmap(tdrv_texture_info *info, tmap_chain *chain)
{
   tmgr_texture_info *tmgr_info;
   int i,n=-1;
   int min_frame=cur_frame-1;
   BOOL reload = FALSE;

   for (i = chain->head; i>=0; i=tmgr_info->next) {
      int frame;

      tmgr_info = &texinfo[i];

      AssertMsg(info->size == texinfo[i].size, "wrong size!");
      // old enough?
      frame = tmgr_info->frame;

      if (frame >= cur_frame-1)
         continue;
      
      if (reload && (info->cookie != tmgr_info->cookie))
         continue;

      if ((!reload) && (info->cookie == tmgr_info->cookie))
         reload = TRUE;
      else if (frame >= min_frame)
         continue;

      min_frame = frame;
      n = i;
      if (reload && (min_frame==TMGR_UNLOADED))
         // can't do better than this!
         break;
   }

   // if we're not in swapout mode, we only want to reload unloaded textures
   if ((!swapout) && ((!reload) || (min_frame != TMGR_UNLOADED))) {
      info->id = next_id;
      return;
   }

   if (n==-1) { // can't find suitable swapout; dump everything!
      mono_printf(("Can't find suitable swapout candidate; dumping all textures!\n"));

      dump_all_textures();
      info->id = next_id;
   } else {
      if (!reload)
         release_texture(n);
      else if (texinfo[n].bitmap != NULL)
         do_unload(texinfo[n].bitmap);

      info->id = n;
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
      // synch now in case we want to swap out this texture later this frame...
      g_driver->synchronize();
      do_unload(bm);
   } else
      Warning(("bad bitmap pointer for unload!\n"));

   put_mono('.');
}

static void tmgr_reload_texture(grs_bitmap *bm)
{
   tdrv_texture_info info;

   if ((bm == NULL)||((bm->flags&BMF_LOADED)==0)) {
      Warning(("tmgr_reload_texture(): NULL bitmap or bitmap not loaded.\n"));
      return;
   }

   validate_bm(bm);
   info.bm = bm;
   info.id = (int )bm->bits;

   calc_size(&info);
   g_driver->cook_info(&info);

   AssertMsg(info.cookie == texinfo[info.id].cookie,
      "tmgr_reload_texture(): cookie doesn't match original.");

   g_driver->reload_texture(&info);
}

static void tmgr_load_texture(r3s_texture bm)
{
   tdrv_texture_info info;
   tmap_chain *chain;

   put_mono('d');

   if (bm->flags & BMF_LOADED)
   {  // must have meant _re_load ;)
      tmgr_reload_texture(bm);
      return;
   }

   info.bm = bm;

   chain = calc_size(&info);

   if (bm->flags & BMF_HACK)
      return;

   g_driver->cook_info(&info);

   swapout_bitmap(&info, chain);

   do {
      put_mono('1');
      if (do_load(&info)!=TDRV_FAILURE)
         break;

      if (swapout) {
         put_mono('2');
         mono_printf(("swapout _really_ failed; dumping all textures!"));
         dump_all_textures();
         info.id = next_id;
      } else {
         put_mono('3');
         mono_printf(("Out of texture memory; entring swapout mode.\n"));
         swapout = TRUE;
         swapout_bitmap(&info, chain);
      }
   } while (TRUE);

   put_mono('4');
   if ((!swapout)&&(info.id==next_id)) {
      // add new texture to head of chain...
      texinfo[next_id].next = chain->head;
      chain->head = next_id;
      AssertMsg(texinfo[next_id].size == chain->size, "wrong size!");

      if (++next_id == max_textures) {
         mono_printf(("Out of texture handles; entering swapout mode.\n"));
         swapout = TRUE;
      }
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
      validate_bm(bm);
      if (texture_clut == texinfo[(int )bm->bits].clut) {
         do_set_texture(bm);
         put_mono('.');
         return;
      }
      mono_printf(("unloading to resynch with new texture clut!\n"));
      tmgr_unload_texture(bm);
      put_mono('f');
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
      overloaded = FALSE;
      abl = (abl*7 + bytes_loaded)/8;
      if ((frame & 0xf) == 0)
         mono_printf(("avg bytes downloaded per frame: %i        \n", abl));
      bytes_loaded = 0;
      texmem_used = 0;
   }
   in_frame = TRUE;
   put_mono('.');
}

static void tmgr_end_frame(void)
{
   in_frame = FALSE;
}

static int tmgr_init(r3s_texture bm, int num_textures, int flags)
{
   if (texinfo!=NULL)
      tmgr_shutdown();

#ifndef SHIP
   spew = ((flags & TMGRF_SPEW) != 0);
#endif

   put_mono('a');
   bytes_loaded = 0;
   texmem_loaded = 0;
   max_textures = num_textures;
   texinfo = (tmgr_texture_info *)Malloc(num_textures * sizeof(*texinfo));
   init_bitmap_list();
   init_size_tables();
   default_bm = bm;
   swapout = FALSE;
   overloaded = FALSE;
   in_frame = FALSE;
   texture_clut = NULL;
   next_id = 0;
   put_mono('.');
   return TMGR_SUCCESS;
}

static void tmgr_shutdown(void)
{
   if (texinfo == NULL)
      return;

   put_mono('b');
   dump_all_textures();

   Free(texinfo);
   texinfo = NULL;

   if (chain_list != NULL) {
      Free(chain_list);
      chain_list = NULL;
   }

   put_mono('.');
}




#ifndef SHIP

static void tmgr_stats(void)
{
   int i, current_total=0, global_total=0;

   mono_setxy(0,0);
   mprint("Texture Manager Stats:       \n");
   mprintf("texmem_loaded: %i  texmem_used: %i  swapout: %i      \n",
      texmem_loaded, texmem_used, swapout);
   for (i = 0; i<num_texture_sizes; i++)
   {
      int size = chain_list[i].size;
      int index = chain_list[i].head;
      int current=0, total=0;

      while (index >= 0)
      {
         AssertMsg(size == texinfo[index].size, "wrong size!");
         if (texinfo[index].bits != NULL) {
            total++;
            if (texinfo[index].frame == cur_frame)
               current++;
         }
         index = texinfo[index].next;
      }
      if (total > 0) {
         mprintf("size %i: this frame: %i total: %i            \n",
            size, current, total);
         current_total += current*size;
         global_total += total*size;
      }
   }
   mprintf("checksum:       \n");
   mprintf("texmem_loaded: %i  texmem_used: %i               \n",
      global_total, current_total);
   AssertMsg(global_total==texmem_loaded, "stats incorrect!");
}
#else
static void tmgr_stats(void) {}
#endif

