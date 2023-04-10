// $Header: x:/prj/tech/libsrc/r3d/RCS/clipcon.h 1.8 1998/06/30 18:45:55 buzzard Exp $

#include <clip.h>

typedef struct {
   void *clip_data;
   mxs_plane world_clip[5];
   int   num_user_planes;
   int   max_user_planes;
   int   clip_flags;   // which u,v coordinate type stuff to clip
   mxs_real  near_plane;   // in view space
   r3e_clipmode clip_mode;
} r3s_clip_context;

#define R3_CLIP_I      1
#define R3_CLIP_U      2
#define R3_CLIP_V      4
#define R3_CLIP_UV     6
#define R3_CLIP_ALL    7
#define R3_CLIP_RGB    (1+8+16)   // store RGB as I,*,*,H,D
