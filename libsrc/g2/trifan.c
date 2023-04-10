/*
 * $Source: s:/prj/tech/libsrc/g2/RCS/trifan.c $
 * $Revision: 1.1 $
 * $Author: KEVIN $
 * $Date: 1996/04/10 17:36:52 $
 *
 * Routine to process an ngon as a series of triangles. 
 *
 * This file is part of the g2 library.
 *
 */

#include <plytyp.h>
#include <dbg.h>

void do_lmap(int n, grs_vertex **vpl, void (*shell)())
{
   Assrt(n>=3);
   shell(vpl);
   if (n>3) {
      int i=4;
      grs_vertex *tvpl[3];
      tvpl[0]=vpl[0];
      tvpl[1]=vpl[2];
      tvpl[2]=vpl[3];
      shell(tvpl);

      while (i<n) {
         tvpl[1]=tvpl[2];
         tvpl[2]=vpl[i];
         shell(tvpl);
         i++;
      }
   }
}
