/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/


#define vector_scale  (2 * FIX_UNIT)

void compute_texture_vertex(g3s_vector *dest, g3s_vector *start, int from, int to)
{
   g3s_vector p;
   fix sc;

   p.x = vertex[to].point.x - vertex[from].point.x;
   p.y = vertex[to].point.y - vertex[from].point.y;
   p.z = vertex[to].point.z - vertex[from].point.z;

   sc = fix_sqrt(fix_mul(p.x,p.x) + fix_mul(p.y,p.y) + fix_mul(p.z,p.z));
   sc = fix_div(vector_scale, sc);

   dest->x = start->x + fix_mul(p.x, sc);
   dest->y = start->y + fix_mul(p.y, sc);
   dest->z = start->z + fix_mul(p.z, sc);
}

void compute_normal(g3s_vector *norm, g3s_vector *p0, g3s_vector *p1, g3s_vector *p2)
{
   g3s_vector delta1, delta2, result;

   delta1.x = p1->x - p0->x;
   delta1.y = p1->y - p0->y;
   delta1.z = p1->z - p0->z;

   delta2.x = p2->x - p1->x;
   delta2.y = p2->y - p1->y;
   delta2.z = p2->z - p1->z;

   result.x = fix_mul(delta1.z, delta2.y) - fix_mul(delta1.y, delta2.z);
   result.y = fix_mul(delta1.x, delta2.z) - fix_mul(delta1.z, delta2.x);
   result.z = fix_mul(delta1.y, delta2.x) - fix_mul(delta1.x, delta2.y);

   *norm = result;
}

void compute_surface_tmapping(int r, int n)
{
   int a,b,c,i;
   int p = region[r].vertex_palette;
   fix len;
   g3s_vector temp;

   a = v_palette[p + surf_v[surface[n].vlist_offset + 0]];
   b = v_palette[p + surf_v[surface[n].vlist_offset + 1]];
   c = v_palette[p + surf_v[surface[n].vlist_offset + 2]];

   compute_texture_vertex(&surface[n].u_vect, &vertex[a].point, a, b);
   compute_texture_vertex(&surface[n].v_vect, &surface[n].u_vect, b, c);

   // now compute normal to surface

   for(i=2; i < surface[n].num_vertices; ++i) {
      c = v_palette[p + surf_v[surface[n].vlist_offset + i]];
      compute_normal(&temp, &vertex[a].point, &vertex[b].point, &vertex[c].point);
      len = fix_mul(temp.x, temp.x) + fix_mul(temp.y,temp.y) + fix_mul(temp.z,temp.z);
      if (len > 64) break;
      // probably colinear, so try next point
   }
     
   g3_vec_normalize(&temp);
   surface[n].normal = temp;
}

