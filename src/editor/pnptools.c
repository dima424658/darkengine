/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/pnptools.c,v 1.26 2000/02/19 13:11:12 toml Exp $
// tools for PnP maintenance

#include <stdlib.h>
#include <rect.h>
#include <command.h>
#include <mprintf.h>
#include <pnptools.h>

// this is the easy one, for rectangle building
// note that bx is really on both sides, where by is only on the top
void buildYLineRect(Rect *fill, Rect *src, int vline, float hsplit, int side, int bx, int by)
{
   // use vline for Y
   fill->ul.y=src->ul.y+(RectHeight(src)*vline/NUM_LINES)+by;
   fill->lr.y=src->ul.y+(RectHeight(src)*(vline+1)/NUM_LINES)-1;
   if (side)
   {
      fill->ul.x=src->ul.x+(int)(RectWidth(src)*hsplit);
      fill->lr.x=src->lr.x;
   }
   else
   {
      fill->ul.x=src->ul.x;
      fill->lr.x=src->ul.x+(int)(RectWidth(src)*hsplit);
   }
   fill->ul.x+=bx;
   fill->lr.x-=bx;
}

void PnP_ExecCommandInt(const char *cmd, int val)
{
   char pbuf[16];
   itoa(val,pbuf,10);
   if (!CommandExecuteParam(cmd,pbuf))
      Warning(("Couldnt find command <%s> to execute (w/%d)",cmd,val));
}

void PnP_ExecCommandFloat(const char *cmd, float val)
{
   char pbuf[24];
   gcvt(val,10,pbuf);
   if (!CommandExecuteParam(cmd,pbuf))
      Warning(("Couldnt find command <%s> to execute (w/%s)",cmd,pbuf));
}
