/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

//  $Header: r:/t2repos/thief2/src/portal/wrdbg.c,v 1.4 2000/02/19 13:18:55 toml Exp $
//
//  wrdbg.c
//
//  Debugging stuff

#include <fix.h>
#include <lgsprntf.h>
#include <port.h>
#include <stdio.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

void PortalDumpCell (FILE *out, int cell)
{
   int i, j, v;
   PortalCell *pc = wr_cell[cell];
   char argh[100];

   fprintf (out, "CELL %d\n", cell);
   
   for (i = 0; i < pc->num_vertices; i++)
   {
      Vertex *v = &pc->vpool[i];

      lg_sprintf (argh, "  vertex %3d : %q %q %q\n", i, fix_from_float (v->x),
               fix_from_float (v->y), fix_from_float (v->z));

      fprintf (out, argh);
   }

   fprintf (out, " poly %d-%d are rendered, %d-%d are portals\n", 0,
            pc->num_render_polys - 1, pc->num_polys - pc->num_portal_polys,
            pc->num_polys - 1);

   v = 0;
   for (i = 0; i < pc->num_polys; i++)
   {
      PortalPolygonCore *poly = &pc->poly_list[i];
      if (i < pc->num_polys - pc->num_portal_polys)
         fprintf (out, "  poly %3d : plane %3d, solid, vertices",
                  i, poly->planeid); 
      else
         fprintf (out, "  poly %3d : plane %3d, dest %3d, vertices",
                  i, poly->planeid, poly->destination);
      for (j = 0; j < poly->num_vertices; j++, v++)
      {
         fprintf (out, " %d", pc->vertex_list[v]);
      }
      fprintf (out, "\n");
   }

   for (i = 0; i < pc->num_planes; i++)
   {
      PortalPlane *plane = &pc->plane_list[i];

      lg_sprintf (argh, "  plane %3d:  [%q %q %q %q]\n", i, fix_from_float (plane->normal.x),
                  fix_from_float (plane->normal.y), fix_from_float (plane->normal.z),
                  fix_from_float (plane->plane_constant));
      fprintf (out, argh);
   }
}

void PortalDumpWorld (FILE *out)
{
   int i;

   for (i = 0; i < wr_num_cells; ++i)
      PortalDumpCell (out, i);

}

/*
Local Variables:
typedefs:("FILE" "PortalCell" "Vertex")
End:
*/
