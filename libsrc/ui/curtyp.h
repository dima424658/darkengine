/*
 * $Source: r:/prj/lib/src/ui/rcs/curtyp.h $
 * $Revision: 1.1 $
 * $Author: kaboom $
 * $Date: 1993/12/16 07:46:45 $
 *
 * Declarations for cursor types.
 *
 * $Log: curtyp.h $
 * Revision 1.1  1993/12/16  07:46:45  kaboom
 * Initial revision
 * 
 */

/* the saveunder for bitmap cursors */
struct _cursor_saveunder {
   grs_bitmap bm;
   int mapsize;
};
