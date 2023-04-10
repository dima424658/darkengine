/*
 * $Source: x:/prj/tech/libsrc/skel/RCS/sklist.c $
 * $Revision: 1.7 $
 * $Author: JON $
 * $Date: 1997/01/27 18:31:31 $
 *
 * Skeleton list sorting routines
 *
 * $Log: sklist.c $
 * Revision 1.7  1997/01/27  18:31:31  JON
 * Replace newskel.h with skel.h.
 * 
 * Revision 1.6  1996/08/23  18:16:23  kate
 * ported to new 3d.  also supports decals.
 * 
 * Revision 1.1  1996/06/10  17:14:54  kate
 * Initial revision
 * 
 * Revision 1.5  1995/12/18  22:43:39  JAEMZ
 * Rewrote library to have no model
 * 
 * Revision 1.4  1995/11/27  15:17:41  JAEMZ
 * Added vhot support and added new NONE sort option
 * 
 * Revision 1.3  1995/09/12  18:30:48  JAEMZ
 * Fixed bug
 * 
 * Revision 1.2  1994/04/21  15:23:23  jaemz
 * Added midpoint sorter
 * 
 * Revision 1.1  1994/04/21  13:31:20  jaemz
 * Initial revision
 * 
 * 
 */

#include <string.h>
#include <r3d.h>
#include <mprintf.h>
#include <skel.h>

// All purpose always correct render list sorter
// admittedly a little bare

// Actually only mostly correct.  Uses midpoint sorting,
// sort of a middle of the road kind of general renderer for if
// you don't care
// take the model since it has no model specific info

void sk_midpoint_sort(sks_skel *s,ubyte *rl,r3s_point *tmp)
{
   mxs_real l2[32];  // pointer to tmp sorting data
   int i,p1,p2;

   for (i=0;i<s->segn;++i) {
      p1 = s->sg[i].p1;
      p2 = s->sg[i].p2;
      // simply use z as a distance metric
      l2[i] = tmp[p1].p.z + tmp[p2].p.z;
   }

   // do insertion sort, making largest values float
   // i is the number of previous ones there already
   // push em down
   for (i=0;i<s->segn;++i) {
      for (p1=0;p1<i;++p1) {
         // if new one is greater than this one, push them down
         if (l2[i] > l2[rl[p1]]) {
            for (p2=i;p2>p1;--p2) rl[p2] = rl[p2-1];
            break;
         }
      }
      // new one is where old one was or at bottom of list
      rl[p1] = i;
   }
}

void sk_no_sort(sks_skel *s,ubyte *rl)
{
   int i;

   for (i=0;i<s->segn;++i) {
      rl[i] = i;
   }
}
























