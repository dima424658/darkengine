/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/render/palmgr.c,v 1.25 2000/02/23 09:08:16 toml Exp $
#include <string.h>

#include <memall.h>
#ifdef PLAYTEST
#include <config.h>
#endif
#include <dev2d.h>
#include <dbg.h>
#include <lgassert.h>
#include <lgd3d.h>
#include <cfgdbg.h>

#include <playtest.h>
#include <palmgr.h>

#include <mprintf.h>

#include <memall.h>
#include <dbmem.h>   // must be last header!

// @TBD (toml 02-19-00): Deep cover should get this stuff working for thier situation! I dont have time to deal right now
// Shock is all hardware, so dont do ipals. Thief uses software for loadout models, so
// used reduced (non-terrain) ipals
#ifndef EDITOR
#if defined(SHOCK)
#define HARDWARE_ONLY
#elif defined(THIEF)
#define SOFTWARE_MODELS_ONLY
#endif
#endif

extern BOOL g_lgd3d;

typedef struct pal_info
{
   uchar *pal_data;
   ushort *pal16;
   ushort *ltab816;
   uchar *ipal;
   int ref_count;
} pal_info;

static pal_info *pal_list=NULL;

// call this to verify that we are initialized and have a valid slot
// can call with 0 if you just want to check initialized, as 0 is valid
#ifdef WARN_ON
#define _palmgr_verify_slot(slot, func) palmgr_verify_slot(slot, func)
#else
#define _palmgr_verify_slot(slot, func) palmgr_verify_slot(slot)
#endif

#ifdef WARN_ON
static BOOL palmgr_verify_slot(int slot, char *func)
#else
static BOOL palmgr_verify_slot(int slot)
#endif
{
   if (pal_list==NULL) {
      Warning(("%s: palmgr not initialized!\n", func));
      return FALSE;
   }
   if ((slot < 0) || (slot > 255)) {
      Warning(("%s: slot %d out of range.\n", func, slot));
      return FALSE;
   }
   return TRUE;
}

///////////
// init/shutdown

void palmgr_init(void)
{
   if (pal_list != NULL) {
      Warning(("palmgr_init(): palmgr already initialized!\n"));
      return;
   }
   pal_list = Calloc (256 * sizeof(pal_info));
   if (grd_ltab816_list != NULL)
      Warning(("palmgr_init(): grd_ltab816_list not NULL; replacing anyway...\n"));
   grd_ltab816_list = Calloc(256 * sizeof(ushort *));
}

void palmgr_free_slot(int i)
{
   pal_info *info = &pal_list[i];
   if (info->ltab816) {
      Free(info->ltab816);
      info->ltab816 = NULL;
   }
   if (info->pal16) {
      Free(info->pal16);
      info->pal16 = NULL;
   }
   if (info->pal_data) {
      Free(info->pal_data);
      info->pal_data = NULL;
   }
   if (info->ipal) {
      Free(info->ipal);
      info->ipal = NULL;
   }
   info->ref_count = 0;
}

void palmgr_shutdown(void)
{
   int i;

   if (pal_list==NULL) {
      Warning(("palmgr_shutdown(): palmgr not initialized!\n"));
      return;
   }

   for (i=0;i<256;i++)
   {
      palmgr_free_slot(i);
   }

   if (grd_ltab816_list != NULL) {
      Free(grd_ltab816_list);
      grd_ltab816_list = NULL;
   }
   Free(pal_list);
   pal_list = NULL;
}

// ipal horror... woo-woo
void calc_ipal(uchar *pal_data, uchar *ipal)
{
   grs_rgb bpal[256];
   int i;

#ifdef IPAL_SPEW
   static int ipal_count = 0;
   mprintf("%i calls to calc_ipal\n", ++ipal_count);
#endif

   for (i=0; i<256; i++) {
      uchar r,g,b;
      r = pal_data[3*i];
      g = pal_data[3*i+1];
      b = pal_data[3*i+2];
      bpal[i] = gr_bind_rgb(r, g, b);
   }
   gr_calc_ipal(bpal, ipal);
}

#define NUM_SHADE_ROWS 16
#define NUM_BRIGHT_ROWS 8
#define NUM_BASE_ROWS    (NUM_SHADE_ROWS+NUM_BRIGHT_ROWS)
#ifdef SOFTWARE_RGB
   #define NUM_EXTRA_ROWS  get_extra_rows()
   #define NUM_LIGHT_ROWS  (NUM_BASE_ROWS+NUM_EXTRA_ROWS)
#else
 #define NUM_LIGHT_ROWS  NUM_BASE_ROWS
#endif

static int get_extra_rows()
{
#if defined(SOFTWARE_RGB)
   static init;
   static BOOL fHardwareOnlyTerrain;

   if (!init)
   {
#if defined(SOFTWARE_MODELS_ONLY)
      fHardwareOnlyTerrain = TRUE;
#else
      fHardwareOnlyTerrain = FALSE;
#endif
#ifdef PLAYTEST
      if (config_is_defined("support_software_terrain_pal"))
         fHardwareOnlyTerrain = FALSE;
      if (config_is_defined("disable_software_terrain_pal"))
         fHardwareOnlyTerrain = TRUE;
#endif

      init = TRUE;
   }
   return (fHardwareOnlyTerrain) ? 0 : (30*8);
#else
   return 0;
#endif
}

static void compute_ltab816(ushort *table, uchar *pal_data, grs_rgb_bitmask  *bitmask)
{
   int i,j;
   gr_compute_ltab816(table, pal_data, NUM_SHADE_ROWS, NUM_BRIGHT_ROWS, bitmask);

#ifdef PLAYTEST
// if star_no_ltab_hack is defined, we do the span sky thing and don't have to hack the lighting table.
   if (config_is_defined("star_no_ltab_hack"))
      return;
#endif

// Hack! can't have any zero entries in our lighting table and have stars
// work correctly in 16 bit.
   for (i=0; i<NUM_BASE_ROWS; i++) {
      for (j=1; j<256; j++) {
         if (table[j]==0)
            table[j] = 1;
      }
      table += 256;
   }
}


#ifdef SOFTWARE_RGB
extern uchar light_pal[];

static void compute_ltab816_rgb(ushort *table, uchar *pal_data, grs_rgb_bitmask  *bitmask)
{
   int low_bits, i, j;
   uchar bshift;
   ulong *high_bitmask;

   if (bitmask->blue==0x1f) {
      low_bits = 2;
      high_bitmask = &bitmask->red;
   } else {
      if (bitmask->red != 0x1f)
         goto bad_bitmask;
      low_bits = 0;
      high_bitmask = &bitmask->blue;
   }
   if (bitmask->green==0x3e0) {
      if (*high_bitmask != 0x7c00)
         goto bad_bitmask;
      bshift = 10;
   } else {
      if ((bitmask->green!=0x7e0)||(*high_bitmask != 0xf800))
         goto bad_bitmask;
      bshift = 11;
   }

   table += 256 * NUM_BASE_ROWS;
   for (i=0; i < NUM_EXTRA_ROWS; ++i) {
      int r2 = light_pal[3*i+low_bits];
      int g2 = light_pal[3*i+1];
      int b2 = light_pal[3*i+2-low_bits];
      for (j=0; j < 256; ++j) {
         int r = pal_data[3*j+low_bits];
         int g = pal_data[3*j+1];
         int b = pal_data[3*j+2-low_bits];
         r=r*r2/256;
         g=g*g2/256;
         b=b*b2/256;
         r=r>>3, g=g>>(13-bshift), b=b>>3;
         *table++ = r+(g<<5)+(b<<bshift);
      }
   }

   return;

bad_bitmask:
   Warning(("compute_ltab816_rgb(): invalid bitmask\n"));
}
#endif

void palmgr_update_pal_slot(int slot)
{
   pal_info *info;
   grs_rgb_bitmask bmask;

   AssertMsg(pal_list!=NULL, "palmgr_update_pal_slot(): palmgr not initialized!");
   info = &pal_list[slot];
   AssertMsg(info->pal_data!=NULL, "palmgr_update_pal_slot(): pal_data is NULL!");
   if (grd_mode >=0)
      switch (grd_bpp)
   {
         case 8:
            if (slot==0)
               gr_set_pal(0, 256, info->pal_data);
            break;
         case 15:
         case 16:
            if (g_lgd3d)
               lgd3d_set_pal_slot(0, 256, info->pal_data, slot);

            gr_get_screen_rgb_bitmask(&bmask);
            if (info->pal16 == NULL)
               info->pal16 = Malloc(256 * sizeof(ushort));
            gr_make_pal16(0, 256, info->pal16, info->pal_data, &bmask);
            gr_set_pal16(info->pal16, slot);

#ifndef HARDWARE_ONLY
            if( info->ltab816 == NULL )
            {
               MallocPushCreditTagged(" [palmgr_update_pal_slot]");
               info->ltab816=Malloc(NUM_LIGHT_ROWS * 256 * sizeof(ushort));
               MallocPopCredit();

               grd_ltab816_list[slot] = info->ltab816;
            }

            // wsf @TBD: it would be really nice to have a "dirty" bit so we know when palette changed,
            // and thus need to recompute these. Such a mechanism could avoid multiple recomputes, and
            // also be used to say "don't compute lighting table" for those screens that do not use it
            // (like most menus).
            // This code used to assume that if a lighting table existed, then it never needed to be
            // reomputed, even though, the palette may have changed. Bad. Caused T2 "store" bugs.
            //
            compute_ltab816(info->ltab816, info->pal_data, &bmask);
#ifdef SOFTWARE_RGB
            compute_ltab816_rgb(info->ltab816, info->pal_data, &bmask);
#endif
#else
            Assert_(info->ltab816 == NULL);
#endif // HARDWARE_ONLY

            break;
   }




      if (info->ipal != NULL)
      {
#ifdef HARDWARE_ONLY
         Assert_(slot == 0);
#endif
         calc_ipal(info->pal_data, info->ipal);
      }

}

// just slam data; don't touch ref count...
void palmgr_set_pal_slot_passively(int start, int n, uchar *pal, int slot)
{
   pal_info *info;

   if (pal_list==NULL)
      palmgr_init();

   if ((slot < 0) || (slot > 255)) {
      Warning(("palmgr_set_pal_slot(): slot out of range.\n"));
      return;
   }

   info = &pal_list[slot];

   if (info->pal_data==NULL)
      info->pal_data = Malloc(768);

   memcpy(info->pal_data + 3*start, pal, 3*n);
}

void palmgr_set_pal_slot(int start, int n, uchar* pal, int slot)
{
   palmgr_set_pal_slot_passively(start,n,pal,slot);
   palmgr_update_pal_slot(slot);
}

// find matching pal and up ref count, or alloc new pal slot
int palmgr_alloc_pal(uchar *pal)
{
   int i;
   pal_info *info;

   if (pal==NULL)
   {
      Warning(("Hey, calling alloc_pal with NULL\n"));
      return 0;
   }

   if (pal_list==NULL)
      palmgr_init();

   for (i=0; i<256; i++) {
      info = &pal_list[i];

      if ((info->pal_data != NULL) &&
          (memcmp(info->pal_data, pal, 768)==0))
      {
         info->ref_count++;
         ConfigSpew("palmgr_spew",("alloc_pal() returning existing palette %d\n"));
         return i;
      }
   }
   // no match; find free slot
   // don't try to replace global palette
   for (i=1; i<256; i++)
   {
      info = &pal_list[i];
      if (info->ref_count <= 0)
         break;
   }

   AssertMsg(i<256, "palmgr_alloc_pal(): no free palette slot!");
   ConfigSpew("palmgr_spew",("alloc_pal() returning new slot %d\n",i));

   // Free ipal if it exists, since we don't necessarily need one.
   if (info->ipal) {
      Free(info->ipal);
      info->ipal = NULL;
   }
   info->ref_count = 1;
   palmgr_set_pal_slot(0, 256, pal, i);
   return i;
}

// just decrement ref count...
void palmgr_release_slot(int slot)
{
   if (!_palmgr_verify_slot(slot, "palmgr_release_slot()"))
      return;
   if (pal_list[slot].ref_count<=0)
   {
      Warning(("palmgr_release_slot(): ref count (%d) <= 0 for slot %x !",pal_list[slot].ref_count,slot));
      return;
   }
   pal_list[slot].ref_count--;
}

void palmgr_add_ref_slot(int slot)
{
   if (!_palmgr_verify_slot(slot, "palmgr_add_ref_slot()"))
      return;

   AssertMsg1(pal_list[slot].pal_data != NULL, "palmgr_add_ref_slot(): slot %i has been freed!", slot);

   pal_list[slot].ref_count++;
}

void palmgr_free_ipal(int slot)
{
   pal_info *info;

   if (!_palmgr_verify_slot(slot, "palmgr_free_ipal()"))
      return;

   info = &pal_list[slot];
   if (info->ipal) {
      Free(info->ipal);
      info->ipal = NULL;
   }
}

uchar *palmgr_get_ipal(int slot)
{
   pal_info *info;

   if (!_palmgr_verify_slot(slot, "palmgr_get_ipal()"))
      return NULL;

   info = &pal_list[slot];

   // it's bad for ya
#ifdef HARDWARE_ONLY
   if (slot)
   {
      Assert_(info->ipal == NULL);
      return NULL;
   }
#endif

   if (info->ipal)
      return info->ipal;
   if (info->pal_data==NULL) {
      Warning(("palmgr_get_ipal(): palette slot %d not in use.\n",slot));
      return NULL;
   }
#ifndef SHIP
   if (slot!=0)
      ConfigSpew("ipal_spew",("Note: Generating new ipal in get for slot %d\n",slot));
#endif
   if (slot)
      MallocPushCreditTagged(" [palmgr_get_ipal]");
   info->ipal = Malloc(32768);
   if (slot)
      MallocPopCredit();
   calc_ipal(info->pal_data, info->ipal);
   return info->ipal;
}

uchar *palmgr_get_pal(int slot)
{
   if (!_palmgr_verify_slot(slot, "palmgr_get_pal()"))
      return NULL;
   return pal_list[slot].pal_data;
}

void palmgr_recompute_all(void)
{
   int i;

   if (pal_list==NULL) {
      palmgr_init();
      return;
   }

   // global palette is handled seperately by scrnman

   if (grd_bpp<15)
   {
      palmgr_update_pal_slot(0);
      return;
   }

   for (i=1; i<256; i++)
   {
      if (pal_list[i].pal_data != NULL) {
         if (pal_list[i].ref_count <= 0)
            palmgr_free_slot(i); // give up the memory
         else
            palmgr_update_pal_slot(i);
      }
   }
}

#ifdef PLAYTEST
#include <mprintf.h>
int palmgr_get_current_count(BOOL mono_print)
{
   int cnt=0, i;

   for (i=0; i<256; i++)
      if (pal_list[i].pal_data!=NULL)
      {
         cnt++;
         if (mono_print)
            mprintf("Using slot %d pal ptr %x has ref_count %d\n",i,pal_list[i].pal_data,pal_list[i].ref_count);
      }
   if (mono_print)
      mprintf("Total count %d\n",cnt);
   return cnt;
}
#endif
