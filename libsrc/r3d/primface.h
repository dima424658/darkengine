// $Header: x:/prj/tech/libsrc/r3d/RCS/primface.h 1.4 1998/06/18 11:26:51 JAEMZ Exp $

   // check three pts to see if forward facing
EXTERN bool r3_check_facing_3(r3s_phandle *vp);

   // find first 3 non-colinear points and check if forward facing
EXTERN bool r3_check_facing_n(int n, r3s_phandle *vp);

   // check viewspace pt & normal to see if forward facing
EXTERN bool r3_check_v_normal_vec(mxs_vector *p, mxs_vector *n);

   // check worldspace pt & normal to see if forward facing
EXTERN bool r3_check_w_normal_vec(mxs_vector *p, mxs_vector *n);

   // check object space pt & normal to see if forward facing
EXTERN bool r3_check_o_normal_vec(mxs_vector *p, mxs_vector *n);

   // check 3d point & normal to see if forward facing

#define r3_check_v_normal_pt(a,b)  r3_check_v_normal_vec(&(a)->p, b)
