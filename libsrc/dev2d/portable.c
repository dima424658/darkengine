/* Portable replacements for the legacy x86 bitmap inner loops. */

#include <string.h>

#include <lg.h>
#include <grd.h>

typedef void (*rsd16_set_fcn)(uchar c, ushort *dst, int count);
typedef void (*rsd16_copy_fcn)(ushort *dst, uchar *src, int count);

static rsd16_set_fcn g_rsd16_set;
static rsd16_copy_fcn g_rsd16_copy;

static unsigned short rsd_u16(const uchar *p)
{
   return (unsigned short)(p[0] | ((unsigned short)p[1] << 8));
}

void flat16_memset(ushort c16, ushort *dst, int count)
{
   int i;

   if (count <= 0)
      return;
   // Full-frame hardware-overlay clears overwhelmingly use color zero. Let
   // the platform's optimized libc handle that multi-megabyte path instead
   // of issuing one unoptimized 16-bit store at a time in debug APKs.
   if (c16 == 0) {
      memset(dst, 0, (size_t)count * sizeof(*dst));
      return;
   }
   for (i = 0; i < count; ++i)
      dst[i] = c16;
}

void flat16_flat16_trans_il(ushort *src, ushort *dst, int count)
{
   int i;
   ushort key = (ushort)grd_chroma_key;
   for (i = 0; i < count; ++i)
      if (src[i] != key)
         dst[i] = src[i];
}

void flat16_flat16_opaque_clut_il(ushort *dst, ushort *src, int count,
                                  ushort *clut16)
{
   int i;
   for (i = 0; i < count; ++i) {
      ushort c = src[i];
      dst[i] = (ushort)(clut16[c & 0xff] + clut16[256 + (c >> 8)]);
   }
}

void flat16_flat8_opaque_inner_loop(ushort *dst, uchar *src, int count)
{
   int i;
   ushort *palette = (ushort *)pixpal;
   for (i = 0; i < count; ++i)
      dst[i] = palette[src[i]];
}

uchar *gd_rsd8_unpack(uchar *src, uchar *dst)
{
   for (;;) {
      unsigned count;
      uchar token = *src++;

      if (token == 0) {
         count = *src++;
         memset(dst, *src++, count);
         dst += count;
      } else if (token < 0x80) {
         count = token;
         memcpy(dst, src, count);
         src += count;
         dst += count;
      } else if (token > 0x80) {
         count = token & 0x7f;
         memset(dst, 0, count);
         dst += count;
      } else {
         unsigned op = rsd_u16(src);
         src += 2;
         if (op == 0)
            return dst;
         if (op < 0x8000) {
            memset(dst, 0, op);
            dst += op;
         } else if (op < 0xc000) {
            count = op & 0x7fff;
            memcpy(dst, src, count);
            src += count;
            dst += count;
         } else {
            count = op & 0x3fff;
            memset(dst, *src++, count);
            dst += count;
         }
      }
   }
}

static void rsd8_advance(uchar **dst, int *remaining, int width,
                         int row_delta, unsigned count)
{
   while (count != 0) {
      unsigned part = count < (unsigned)*remaining ? count : (unsigned)*remaining;
      *dst += part;
      *remaining -= (int)part;
      count -= part;
      if (*remaining == 0) {
         *dst += row_delta;
         *remaining = width;
      }
   }
}

static void rsd8_write(uchar **dst, int *remaining, int width, int row_delta,
                       uchar **src, unsigned count, int run, uchar value)
{
   while (count != 0) {
      unsigned part = count < (unsigned)*remaining ? count : (unsigned)*remaining;
      if (run)
         memset(*dst, value, part);
      else {
         memcpy(*dst, *src, part);
         *src += part;
      }
      *dst += part;
      *remaining -= (int)part;
      count -= part;
      if (*remaining == 0) {
         *dst += row_delta;
         *remaining = width;
      }
   }
}

void flat8_rsd8_blit(uchar *src, uchar *dst, int dst_row, int width)
{
   int remaining = width;
   int row_delta = dst_row - width;

   for (;;) {
      unsigned count;
      uchar token = *src++;
      if (token == 0) {
         uchar value;
         count = *src++;
         value = *src++;
         rsd8_write(&dst, &remaining, width, row_delta, &src, count, 1, value);
      } else if (token < 0x80) {
         count = token;
         rsd8_write(&dst, &remaining, width, row_delta, &src, count, 0, 0);
      } else if (token > 0x80) {
         rsd8_advance(&dst, &remaining, width, row_delta, token & 0x7f);
      } else {
         unsigned op = rsd_u16(src);
         src += 2;
         if (op == 0)
            return;
         if (op < 0x8000)
            rsd8_advance(&dst, &remaining, width, row_delta, op);
         else if (op < 0xc000)
            rsd8_write(&dst, &remaining, width, row_delta,
                       &src, op & 0x7fff, 0, 0);
         else {
            uchar value = *src++;
            rsd8_write(&dst, &remaining, width, row_delta,
                       &src, op & 0x3fff, 1, value);
         }
      }
   }
}

void flat16_rsd8_blit_init(rsd16_set_fcn set_fcn, rsd16_copy_fcn copy_fcn)
{
   g_rsd16_set = set_fcn;
   g_rsd16_copy = copy_fcn;
}

static void rsd16_advance(ushort **dst, int *remaining, int width,
                          int row_delta, unsigned count)
{
   while (count != 0) {
      unsigned part = count < (unsigned)*remaining ? count : (unsigned)*remaining;
      *dst += part;
      *remaining -= (int)part;
      count -= part;
      if (*remaining == 0) {
         *dst = (ushort *)((uchar *)*dst + row_delta);
         *remaining = width;
      }
   }
}

static void rsd16_write(ushort **dst, int *remaining, int width, int row_delta,
                        uchar **src, unsigned count, int run, uchar value)
{
   while (count != 0) {
      unsigned part = count < (unsigned)*remaining ? count : (unsigned)*remaining;
      if (run)
         g_rsd16_set(value, *dst, part);
      else {
         g_rsd16_copy(*dst, *src, part);
         *src += part;
      }
      *dst += part;
      *remaining -= (int)part;
      count -= part;
      if (*remaining == 0) {
         *dst = (ushort *)((uchar *)*dst + row_delta);
         *remaining = width;
      }
   }
}

void flat16_rsd8_blit(uchar *src, uchar *dst8, int dst_row, int width)
{
   ushort *dst = (ushort *)dst8;
   int remaining = width;
   int row_delta = dst_row - 2 * width;

   for (;;) {
      unsigned count;
      uchar token = *src++;
      if (token == 0) {
         uchar value;
         count = *src++;
         value = *src++;
         rsd16_write(&dst, &remaining, width, row_delta, &src, count, 1, value);
      } else if (token < 0x80) {
         count = token;
         rsd16_write(&dst, &remaining, width, row_delta, &src, count, 0, 0);
      } else if (token > 0x80) {
         rsd16_advance(&dst, &remaining, width, row_delta, token & 0x7f);
      } else {
         unsigned op = rsd_u16(src);
         src += 2;
         if (op == 0)
            return;
         if (op < 0x8000)
            rsd16_advance(&dst, &remaining, width, row_delta, op);
         else if (op < 0xc000)
            rsd16_write(&dst, &remaining, width, row_delta,
                        &src, op & 0x7fff, 0, 0);
         else {
            uchar value = *src++;
            rsd16_write(&dst, &remaining, width, row_delta,
                        &src, op & 0x3fff, 1, value);
         }
      }
   }
}
