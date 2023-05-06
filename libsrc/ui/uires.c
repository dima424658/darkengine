/*
 * $Source: r:/prj/lib/src/ui/rcs/uires.c $
 * $Revision: 1.3 $
 * $Author: jak $
 * $Date: 1995/05/02 19:16:40 $
 *
 */

#include <uires.h>
#include <string.h>

#include <_ui.h>

struct _uirestempbuffer uiResTempBuffer;

#pragma disable_message(202)
errtype master_load_bitmap_from_res(grs_bitmap *bmp, Id id_num, int i, RefTable *rt, bool tmp_mem, Rect *anchor, uchar *p)
{
   Ref rid;
   FrameDesc *f;
   bool alloced_fdesc = FALSE;
//   extern int memcount;

   if(!RefIndexValid(rt,i)) {
      Warning(("Bitmap index %i invalid!\n",i));
      return(ERR_FREAD);
   }

   rid = MKREF(id_num,i);
   if (uiResTempBuffer.mem == NULL || RefSize(rt,i) > uiResTempBuffer.size)
   {
      Spew(DSRC_UI_Initialization,("damn, we have to malloc...need %d, buffer = %d\n",RefSize(rt,i),uiResTempBuffer.size));
      f = (FrameDesc *)Malloc(RefSize(rt,i));
      alloced_fdesc = TRUE;
   }
   else
   {
      f = (FrameDesc *)uiResTempBuffer.mem;
   }
//   memcount += RefSize(rt,i);
   if (f == NULL)
   {
      Warning(("Could not load bitmap from resource #%d!\n",id_num));
      return(ERR_FREAD);
   }
   RefExtract(rt,rid,f);
   if (anchor != NULL)
      *anchor = f->anchorArea;
   if (!tmp_mem && p == NULL)
      p = (uchar *)Malloc(f->bm.w * f->bm.h * sizeof(uchar));
   if (tmp_mem)
      p = (char*)(f+1);

//   memcount += f->bm.w * f->bm.h * sizeof(uchar);
   if (!tmp_mem) memcpy(p,f+1,f->bm.w * f->bm.h * sizeof(uchar));
   *bmp = f->bm;
   bmp->bits = p;
   if (alloced_fdesc)
      Free(f);
   return(OK);
}

errtype uiLoadRefBitmapCursor(Cursor* c, grs_bitmap* bmp, Ref rid, bool alloc)
{
   errtype retval = OK;  
   Rect anchor;
//   extern int memcount;
   bool buffer_snag = FALSE;

   int numrefs = ResNumRefs(REFID(rid));
   int tsize = REFTABLESIZE(numrefs);

   RefTable *rt = NULL;
   if (uiResTempBuffer.mem != NULL && tsize <= uiResTempBuffer.size)
   {
      rt = (RefTable*)uiResTempBuffer.mem;
      uiResTempBuffer.mem += tsize;
      uiResTempBuffer.size -= tsize;
      ResExtractRefTable(REFID(rid),rt,tsize);
      buffer_snag = TRUE;
   }
   else rt = ResReadRefTable(REFID(rid));
   retval = master_load_bitmap_from_res(bmp, REFID(rid), REFINDEX(rid), rt, FALSE, &anchor,(alloc) ? NULL : bmp->bits);
   if (buffer_snag)
   {
      uiResTempBuffer.mem = (char*)rt;
      uiResTempBuffer.size += tsize;
   }
   else ResFreeRefTable(rt);
   retval = uiMakeBitmapCursor(c,bmp,anchor.ul);
   return retval;
}
