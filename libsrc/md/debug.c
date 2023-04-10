/*
 * $Source: x:/prj/tech/libsrc/md/RCS/debug.c $
 * $Revision: 1.9 $
 * $Author: JAEMZ $
 * $Date: 1998/06/16 12:44:30 $
 *
 * Model Library debugging routines
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <matrix.h>
#include <mdd.h>

// Ascii names of a few enums
char * mdd_sub_names[3] = {"MD_SUB_NONE","MD_SUB_ROT","MD_SUB_SLIDE"};
char * mdd_mat_names[2] = {"MD_MAT_TMAP","MD_MAT_COLOR"};
char * mdd_pgon_names[4] = {"MD_PGON_NONE","MD_PGON_SOLID","MD_PGON_WIRE","MD_PGON_TMAP"};

void md_prn_header(mds_model *h)
{
   printf("ID: %s, ver: %d\n",h->id,h->ver);
   printf("Model: %s\n",h->name);

   printf("sphere radius: %f\n",h->radius);
   printf("max pgon rad: %f\n",h->max_pgon_radius);

   printf("bbox max: "); mx_prn_vec(&h->bmax);
   printf("bbox min: "); mx_prn_vec(&h->bmin);
   printf("parent cen: "); mx_prn_vec(&h->pcen);

   printf("num pgons: %d\n",h->pgons);
   printf("num verts: %d\n",h->verts);
   printf("num parms: %d\n",h->parms);
   printf("num mats:  %d\n",h->mats);
   printf("num vcalls: %d\n",h->vcalls);
   printf("num vhots: %d\n",h->vhots);
   printf("num subobjs: %d\n",h->subobjs);
   printf("model size: %d\n",h->mod_size);
}

void md_prn_subobj(mds_subobj *s)
{
   printf("Subobj: %s\n",s->name);
   printf("   type: %s\n",mdd_sub_names[s->type]);
   printf("   which parm: %d\n",s->parm);
   printf("   min_range: %f\n",s->min_range);
   printf("   max_range: %f\n",s->max_range);
   printf("   Rotation transform:\n");
   mx_prn_trans(&s->trans);
   printf("   child sub:   %d\n",s->child);
   printf("   next sub:    %d\n",s->next);
   printf("   vhot start:  %d\n",s->vhot_start);
   printf("   vhot num:    %d\n",s->vhot_num);
   printf("   point start: %d\n",s->point_start);
   printf("   point num:   %d\n",s->point_num);
   printf("   light start: %d\n",s->light_start);
   printf("   light num:   %d\n",s->light_num);
   printf("   norm start:  %d\n",s->norm_start);
   printf("   norm num:    %d\n",s->norm_num);
   printf("   node start:  %d\n",s->node_start);
}

void md_prn_mat(mds_mat *m)
{
   printf("Material: %s\n",m->name);
   printf("   type:  %s\n",mdd_mat_names[m->type]);
   printf("   slot num: %d\n",m->num);

   if (m->type == MD_MAT_TMAP) {
      printf("   handle:   %d\n",m->handle);
      printf("   uv scale: %f\n",m->uv);
   } else {
      printf("   argb: (%d,%d,%d,%d)\n",(m->handle>>24)&0xFF,(m->handle>>16)&0xFF,(m->handle>>8)&0xFF,(m->handle)&0xFF);
      printf("   ipal index: %d\n",(m->ipal));
   }
}

void md_prn_light(mds_light *l)
{
   mxs_vector v;
   uint lt = l->norm;

   v.x = X_NORM(lt);
   v.y = Y_NORM(lt);
   v.z = Z_NORM(lt);

   printf("mat %d, point %d\n",l->mat,l->point);
   printf("   norm: "); mx_prn_vec(&v);
}

// Prints the current one and advances
// the counter to the next one
mds_pgon *md_prn_pgon(mds_pgon *p)
{
   uchar *next = (uchar*)p;
   int i;

   // adjust for header
   next += sizeof(mds_pgon);

   // adjust for vlist and light list
   next += 2 * p->num * sizeof(ushort);

   printf("index: %d\n",p->index);
   printf("type: %s ",mdd_pgon_names[(p->type)&MD_PGON_PRIM_MASK]);
   if ((p->type&MD_PGON_COLOR_MASK) == MD_PGON_COLOR_PAL)
      printf("| MD_PGON_COLOR_PAL\n");
   else if ((p->type&MD_PGON_COLOR_MASK) == MD_PGON_COLOR_VCOL)
      printf("| MD_PGON_COLOR_VCOLOR\n");

   printf("data %x\n",p->data);
   printf("   num verts %d: ",p->num);
   printf("   norm %d, d = %f\n",p->norm,p->d);
   printf("   verts: ");
   for (i=0;i<p->num;++i) {
      printf("%d ",p->verts[i]);
   }

   // print out lighting values
   printf("\n   lights: ");
   for (;i<2*p->num;++i) {
      printf("%d ",p->verts[i]);
   }

   // if tmap, print out uv list
   if ( ((p->type)&MD_PGON_PRIM_MASK) == MD_PGON_PRIM_TMAP) {
      next += p->num * sizeof(ushort);
      printf("\n   uvs: ");
      for (;i<3*p->num;++i) {
         printf("%d ",p->verts[i]);
      }
   }

   printf("\n");

   return (mds_pgon *)next;
}


void md_prn_sphere_info(mds_sphere *s)
{
   printf("   b sphere cen: "); mx_prn_vec(&s->cen);
   printf("   b sphere rad: %f\n",s->rad);
}


// just pass in an unsigned char
uchar *md_prn_node(uchar *n)
{
   int i;

   if (*n == MD_NODE_SUBOBJ) {
      mds_node_subobj *c = (mds_node_subobj *)n;

      printf("Start of node subobj %d\n",c->index);
      n+=sizeof(mds_node_subobj);
   }

   switch (*n) {
      case MD_NODE_VCALL:
      {
         mds_node_vcall *c = (mds_node_vcall *)n;
         printf("type: MD_NODE_VCALL\n");
         md_prn_sphere_info(&c->sphere);
         printf("   slot: %d\n",c->index);
         n+=sizeof(mds_node_vcall);
         break;
      }
      case MD_NODE_CALL:
      {
         mds_node_call *c = (mds_node_call *)n;

         printf("type: MD_NODE_CALL\n");
         md_prn_sphere_info(&c->sphere);

         printf("   pgons before: ");
         for (i=0;i<c->pgons_before;++i)
            printf("%d ",c->polys[i]);

         printf("\n   call node: %d\n",c->node_call);

         printf("   pgons after: ");
         for (i=0;i<c->pgons_after;++i)
            printf("%d ",c->polys[i+c->pgons_before]);
         printf("\n");

         n+=sizeof(mds_node_call);
         n+=sizeof(ushort)*(c->pgons_before+c->pgons_after);
         break;
      }
      case MD_NODE_SPLIT:
      {
         mds_node_split *c = (mds_node_split *)n;

         printf("type: MD_NODE_SPLIT\n");
         md_prn_sphere_info(&c->sphere);

         printf("   pgons before: ");
         for (i=0;i<c->pgons_before;++i)
            printf("%d ",c->polys[i]);

         printf("\n   split normal: %d, d = %f\n",c->norm,c->d);
         printf("   behind node: %d\n",c->node_behind);
         printf("   front node:  %d\n",c->node_front);

         printf("   pgons after: ");
         for (i=0;i<c->pgons_after;++i)
            printf("%d ",c->polys[i+c->pgons_before]);
         printf("\n");

         n+=sizeof(mds_node_split);
         n+=sizeof(ushort)*(c->pgons_before+c->pgons_after);
         break;
      }
      case MD_NODE_RAW:
      {
         mds_node_raw *c = (mds_node_raw *)n;

         printf("type: MD_NODE_RAW\n");
         md_prn_sphere_info(&c->sphere);

         printf("   pgons: ");
         for (i=0;i<c->num;++i)
            printf("%d ",c->polys[i]);
         printf("\n");

         n+=sizeof(mds_node_raw);
         n+=sizeof(ushort)*(c->num);
         break;
      }
   }

   return n;
}


void md_prn_model(mds_model *m)
{
   mds_subobj *subs;
   mds_mat *mats;
   mds_uv *uvs;
   mds_vhot *vhots;
   mxs_vector *points;
   mds_light *lights;
   mxs_vector *norms;
   uchar * pgons;
   mds_pgon *p;
   uchar *nodes;
   uchar *n;
   int i,j;

   subs  = (mds_subobj*)((uchar *)m + m->subobj_off);
   mats  = (mds_mat*)((uchar *)m+m->mat_off);
   uvs   = (mds_uv*)((uchar *)m+m->uv_off);
   vhots = (mds_vhot*)((uchar *)m+m->vhot_off);
   points= (mxs_vector*)((uchar *)m+m->point_off);
   lights= (mds_light*)((uchar *)m+m->light_off);
   norms = (mxs_vector*)((uchar *)m+m->norm_off);
   pgons = (uchar *)((uchar *)m+m->pgon_off);
   nodes = (uchar *)((uchar *)m+m->node_off);

   // print header
   md_prn_header(m);

   // print subobjs
   for (i=0;i<m->subobjs;++i) {
      printf("\n");
      md_prn_subobj(&subs[i]);
   }

   // print materials
   for (i=0;i<m->mats;++i) {
      printf("\n");
      md_prn_mat(&mats[i]);
   }

   // print uv's
   printf("\nUV's:\n");
   for (i=0;i< (m->vhot_off - m->uv_off)/sizeof(mds_uv);++i) {
      printf("   uv %d: \t%f \t%f\n",i,uvs[i].u,uvs[i].v);
   }

   // print vhots
   printf("\nVhots:\n");
   for (i=0;i< m->vhots;++i) {
      printf("   vhot %d: slot %d ",i,vhots[i].id);
      mx_prn_vec(&vhots[i].v);
   }

   // print points
   printf("\nPoints:\n");
   for (i=0;i<m->verts;++i) {
      printf("   pt %d:\t",i);
      mx_prn_vec(&points[i]);
   }

   // print lights
   printf("\nLight vecs:\n");
   for (i=0;i<(m->norm_off - m->light_off)/sizeof(mds_light);++i) {
      printf("   lt %d: ",i);
      md_prn_light(&lights[i]);
   }

   // print norms
   printf("\nNormals:\n");
   for (i=0;i<(m->pgon_off - m->norm_off)/sizeof(mxs_vector);++i) {
      printf("   norm %d:\t",i);
      mx_prn_vec(&norms[i]);
   }

   // print pgons
   i = 0;
   p = (mds_pgon *)pgons;
   printf("\nPolygons:\n");
   while(i<m->pgons) {
      printf("Pgon %d: ",(int)p - (int)pgons);
      p = md_prn_pgon(p);
      i++;
   }

   // print nodes
   n = nodes;
   printf("\nNodes:\n");
   for (j=0;j<m->subobjs;++j) {
      printf("Subobj %d\n",j);
      n = nodes + subs[j].node_start;
      i = 0;
      while(i<subs[j].node_num) {
         printf("\nNode %d: ",(int)n - (int)nodes);
         n = md_prn_node(n);
         i++;
      }
   }
}
