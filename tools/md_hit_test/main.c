// Synthetic model data only: exercise the production MD intersection routines.
#include <mdutil.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

static union { double alignment; unsigned char bytes[2048]; } storage;
static int failures;

static void expect(int condition, const char *name)
{
   printf("%s %s\n", condition ? "PASS" : "FAIL", name);
   if (!condition) ++failures;
}

static mds_model *make_model(int before, int after, int child, int nested)
{
   unsigned char *data = storage.bytes;
   mds_model *model = (mds_model *)data;
   mxs_vector *verts, *norms;
   mds_node_call *call, *inner;
   mds_node_raw *leaf;
   int i, j;
   const float xy[4][2] = {{-2,-2}, {-2,2}, {2,2}, {2,-2}};
   memset(data, 0, sizeof(storage));
   model->ver = 4;
   model->radius = 8;
   model->pgons = 3;
   model->point_off = 256;
   model->norm_off = 512;
   model->pgon_off = 640;
   model->node_off = 1024;
   verts = (mxs_vector *)(data + model->point_off);
   norms = (mxs_vector *)(data + model->norm_off);
   norms[0].z = 1;
   for (i = 0; i < 3; ++i)
   {
      mds_pgon *poly = (mds_pgon *)(data + model->pgon_off + i * 32);
      poly->num = 4;
      poly->d = -(float)i;
      for (j = 0; j < 4; ++j)
      {
         verts[i*4+j].x = xy[j][0];
         verts[i*4+j].y = xy[j][1];
         verts[i*4+j].z = (float)i;
         poly->verts[j] = (ushort)(i*4+j);
      }
   }
   // Match a SUBOBJ -> CALL hierarchy, including nested calls when requested.
   data[model->node_off] = MD_NODE_SUBOBJ;
   call = (mds_node_call *)(data + model->node_off + sizeof(mds_node_subobj));
   call->type = MD_NODE_CALL;
   call->pgons_before = (ushort)before;
   call->pgons_after = (ushort)after;
   if (before) call->polys[0] = 0;
   if (after) call->polys[before] = 64;
   call->node_call = nested ? 64 : 128;
   inner = (mds_node_call *)(data + model->node_off + 64);
   inner->type = MD_NODE_CALL;
   inner->node_call = 128;
   leaf = (mds_node_raw *)(data + model->node_off + 128);
   leaf->type = MD_NODE_RAW;
   leaf->num = (ushort)child;
   if (child) leaf->polys[0] = 32;
   return model;
}

static void check_case(const char *name, int before, int after, int child, int nested)
{
   mds_model *model = make_model(before, after, child, nested);
   mxs_vector a = {0,0,4}, b = {0,0,-2};
   mds_segment_hit_detect_info segment;
   mds_sphere_hit_detect_info sphere;
   ushort polys[3];
   float expected = (4.0f - (after ? 2.0f : child ? 1.0f : 0.0f))/6.0f;
   int early;
   for (early = 0; early < 2; ++early)
   {
      memset(&segment, 0, sizeof(segment));
      segment.a = &a;
      segment.b = &b;
      segment.early_out = (bool)early;
      md_segment_hit_detect(model, &segment);
      expect(segment.hit && (early || fabsf(segment.r - expected) < 0.0001f), name);
      memset(&sphere, 0, sizeof(sphere));
      sphere.sphere.cen.z = 1;
      sphere.sphere.rad = 1.1f;
      sphere.early_out = (bool)early;
      sphere.polys = polys;
      md_sphere_hit_detect(model, &sphere);
      expect(sphere.hit && (early || sphere.num == before + after + child), "sphere CALL polygons");
   }
   a.x = b.x = 10;
   memset(&segment, 0, sizeof(segment));
   segment.a = &a;
   segment.b = &b;
   md_segment_hit_detect(model, &segment);
   expect(!segment.hit, "ray outside model misses");
}

int main(void)
{
   check_case("CALL before polygons", 1, 0, 0, 0);
   check_case("CALL after polygons", 0, 1, 0, 0);
   check_case("CALL child polygons", 0, 0, 1, 0);
   check_case("nested CALL nearest hit", 1, 1, 1, 1);
   printf("RESULT %s failures=%d\n", failures ? "FAIL" : "PASS", failures);
   return failures ? 1 : 0;
}
