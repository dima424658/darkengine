#include <r3d.h>
#include <stdio.h>
#include <stdlib.h>
#include <fault.h>

#include <lg.h>

bool r3_start_frame_context(r3s_context *ctxt);

double cur_time;


// create a block, transform and build the box
// level is level of subobjects to go down
draw_box(int lev,double s, grs_bitmap *bm)
{
   static mxs_vector v[8];
   static r3s_point p[8];
   static r3s_phandle vlist[4];
   int i,j;

   r3_set_2d_clip(TRUE);
   r3_set_polygon_context(R3_PL_POLYGON | R3_PL_TEXTURE);
   r3_set_texture(bm);

   r3_start_block();

   // top 
   v[0].z = -s;
   v[0].y = -s;
   v[0].x = s;

   v[1].z = -s;
   v[1].y = s;
   v[1].x = s;

   v[2].z = s;
   v[2].y = s;
   v[2].x = s;

   v[3].z = s;
   v[3].y = -s;
   v[3].x = s;

   for (i=0;i<4;++i) {
      v[i+4] = v[i];
      v[i+4].x -= 2*s;
   }

   r3_transform_block(8,p,v);

   gr_set_fcolor(1);

   for (i=0; i < 6; ++i) {
      switch(i) {
         case 0: vlist[3] = p+0; vlist[2] = p+1; vlist[1] = p+2; vlist[0] = p+3; break;
         case 1: vlist[0] = p+4; vlist[1] = p+5; vlist[2] = p+6; vlist[3] = p+7; break;
         case 2: vlist[3] = p+3; vlist[2] = p+2; vlist[1] = p+6; vlist[0] = p+7; break;
         case 3: vlist[3] = p+2; vlist[2] = p+1; vlist[1] = p+5; vlist[0] = p+6; break;
         case 4: vlist[3] = p+1; vlist[2] = p+0; vlist[1] = p+4; vlist[0] = p+5; break;
         case 5: vlist[3] = p+0; vlist[2] = p+3; vlist[1] = p+7; vlist[0] = p+4; break;
      }
      vlist[0]->grp.u = vlist[3]->grp.u = 0;
      vlist[1]->grp.u = vlist[2]->grp.u = 64;
      vlist[0]->grp.v = vlist[1]->grp.v = 0;
      vlist[2]->grp.v = vlist[3]->grp.v = 64;
      r3_draw_tmap(4,vlist,bm);
   }
   r3_end_block();

   if (lev>0) {
      int ax = lev%3;
      mxs_vector v;

      mx_unit_vec(&v,ax);
      mx_scaleeq_vec(&v,s*2);

      r3_start_object_n(&v,6.28*cur_time/(30.0*s*s),ax);
//      r3_start_object_x(&v,3.14159/4);
      draw_box(lev-1,s*.5, bm);
      r3_end_object();
   }
}

grs_canvas *off_canv;

blit()
{
   gr_set_canvas(grd_visible_canvas);
   gr_bitmap(&(off_canv->bm),0,0);
   gr_set_canvas(off_canv);
}

grs_bitmap *testcore_build_tmap(int shift, int seed)
{
   grs_bitmap *us;               // where we will go
   uchar *dummy_tm;              // store the damn bitmap
   int x, y;                     // loop vars
   int size, sdown;              // pixel size (not exp), shift down
   int v[4];                     // the 4 colors to use

   if (seed!=0) srand(seed);     // setup colors
   v[0]=rand()&0xff; v[1]=rand()&0xff; v[2]=rand()&0xff; v[3]=rand()&0xff;

   size=1<<shift;                // we are 2n pixels across
   (shift>=4)?sdown=shift-3:sdown=0;      // 8 or fewer checkers on the texture

   dummy_tm=(uchar *)Malloc(size*size);
   us=(grs_bitmap *)Malloc(sizeof(grs_bitmap));
   if ((dummy_tm==NULL)||(us==NULL)) return NULL;
   for (x=0; x<size; x++)
      for (y=0; y<size; y++)
         dummy_tm[(x*size)+y]=x^y;
   gr_init_bm(us,dummy_tm,BMT_FLAT8,0,size,size);
   return us;
}

void main(int argc,char **argv)
{
   grs_screen *screen;
   grs_bitmap *bm;
   r3s_context con;
   mxs_vector obj;
   float j;

   obj.x = 2.0;
   obj.y = 0;
   obj.z = 0;

   // init the mode, make a screen
   gr_init();
   gr_set_mode(GRM_640x480x8,TRUE);
   screen = gr_alloc_screen(grd_mode_info[grd_mode].w,grd_mode_info[grd_mode].h);
   gr_set_screen(screen);

   ex_startup(EXM_DIVIDE_ERR);

   r3_init_defaults(-1,-1,-1,-1);

   off_canv = gr_alloc_canvas(BMT_FLAT8,640,480);
   gr_set_canvas(off_canv);

   // get us our context
   r3_alloc_context(&con,0);

   //set its ass
   r3_start_frame_context(&con);

   bm = testcore_build_tmap(6, 0);

   // set camera

   // need to add to matrix.h rotate transform

   for (j=0;j<100;j+=.1) {
      cur_time=j;

      obj.x = 1.0 + atoi(argv[1])/2 + (float)j/10+2.0;
      obj.y = j>30?(float)(j-30)/10:0;
      obj.z = j>60?(float)(j-60)/10:0;

      if (j<30) {
         r3_start_object_z(&obj,6.28*(float)(j)/30.0);
      } else if (j<60) {
         r3_start_object_y(&obj,6.28*(float)(j)/30.0);
      } else {
         r3_start_object_x(&obj,6.28*(float)(j)/30.0);
      }   
         // set the object frame of reference
//         r3_start_object(&obj);
//      


      // draw box
      gr_clear(0);

      draw_box(atoi(argv[1]),1.0, bm);

      blit();
      

      r3_end_object();
   }


   gr_close();

}
