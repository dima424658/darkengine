/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#ifndef __SCHEDSTR_H
#define __SCHEDSTR_H

#include <matrix.h>
#include <fix.h>

typedef struct weap_data {
   mxs_real        implied_duration;
   short       num_frames;
   int         close;
   short       base_joint_id;
   short       weap_pos_id;    // End of weapon joint
   short       weap_id;        // The type of weapon
   short       eff_pos_id;
   ushort incidental;
   mxs_real efflen;
   mxs_real weaplen;

} weap_data;

typedef struct effweap_capture {
   mxs_matrix e_data;        // num_frames array of end position 
                              // of the weapon.
   mxs_matrix eff_data;      // num_frames array of effector position.    
} effweap_capture;


typedef struct Effweap_sched {

   int type;
   mxs_real        start_time;
   mxs_real        duration;
   short   num_frames;
   int Rel_or_Abs;
   int         close;
   short       base_joint_id;
   short       weap_pos_id;    // End of weapon joint
   int table_id;
   weap_data weap_params;
   mxs_real efflen;
   mxs_real weaplen;

   void *start_end_data;
   void (*start_or_end)(void *,int);
   mxs_real (*speed)(mxs_real);
   mxs_real do_time_increment;
   //   void (*do_sched)(bipeds_info *,mxs_real, mxs_real);
   //void (*do_simpsched)(bipeds_info *,mxs_real, mxs_real);
   //void (*set_sched)(bipeds_info *,mxs_real, int, int);
   void (*do_sched)(void *,void *,mxs_real, mxs_real);
   void (*do_simpsched)(void *,void *,mxs_real, mxs_real);
   void (*set_sched)(void *,void *,mxs_real, int, int);

} Effweap_sched;

typedef struct effec_data {
   mxs_real    implied_duration;
   short   num_frames;
   int         close;
   short   base_joint_id;
   short   eff_pos_id;
   ushort incidental;
   mxs_real    len1;           // length of single or heel to toe segment
   mxs_real    len2;           // length of ankle to heel segment (if needed)

} effec_data;

typedef struct effec_capture {
   mxs_matrix e_data;     // num_frames array of end position (rel to base
                           // joint and normal to limbs.
   mxs_angvec ang_data;
} effec_capture;

typedef struct Effec_sched {
   int     type;
   int     table_id;
   mxs_real        start_time;
   mxs_real        duration;
   short   num_frames;
   int Rel_or_Abs;
   int         close;
   short   base_joint_id;
   short   eff_pos_id;
   effec_data effec_params;
   mxs_real    len1;           // length of single or heel to toe segment
   mxs_real    len2;           // length of ankle to heel segment (if needed)

   void *start_end_data;
   void (*start_or_end)(void *,int);
   mxs_real (*speed)(mxs_real);
   mxs_real do_time_increment;
   //   void (*do_sched)(bipeds_info *,mxs_real, mxs_real);
   //void (*do_simpsched)(bipeds_info *,mxs_real, mxs_real);
   //void (*set_sched)(bipeds_info *,mxs_real, int, int);
   void (*do_sched)(void *,void *,mxs_real, mxs_real);
   void (*do_simpsched)(void *,void *,mxs_real, mxs_real);
   void (*set_sched)(void *,void *,mxs_real, int, int);

} Effec_sched;

typedef struct limb_data {
   mxs_real    implied_duration;
   short   num_frames;
   int         close;
   short   base_joint_id;
   short   end_joint_id;
   ushort incidental;
   mxs_real    len1;           // length of single or heel to toe segment
   mxs_real    len2;           // length of ankle to heel segment (if needed)

} limb_data;

typedef struct limb_capture {
   mxs_vector e_data;    // num_frames array of end position rel to base
                          // joint and normal to limbs.
   mxs_vector n_data;    // num_frames array of normal direction 
                          // rel to base joint and normal to upperarm 
                          // or thigh.
   mxs_vector f_data;    // num_frames array of normal direction 
                          // rel to base joint and normal to forearm or calf.
} limb_capture;

typedef struct Limb_sched {

   int         type;
   int         table_id;
   mxs_real        start_time;
   mxs_real        duration;
   short   num_frames;
   int Rel_or_Abs;
   int         close;
   short   base_joint_id;
   short   end_joint_id;
   short   norm_id;
   short   fore_id;
   limb_data   limb_params;   
   mxs_real    len1;           // length of single or heel to toe segment
   mxs_real    len2;           // length of ankle to heel segment (if needed)

   void *start_end_data;
   void (*start_or_end)(int);
   mxs_real (*speed)(mxs_real);
   mxs_real do_time_increment;
   //   void (*do_sched)(bipeds_info *,mxs_real, mxs_real);
   //void (*do_simpsched)(bipeds_info *,mxs_real, mxs_real);
   //void (*set_sched)(bipeds_info *,mxs_real, int, int);
   void (*do_sched)(void *,void *,mxs_real, mxs_real);
   void (*do_simpsched)(void *,void *,mxs_real, mxs_real);
   void (*set_sched)(void *,void *,mxs_real, int, int);
} Limb_sched;

typedef struct trunk_data {
   mxs_real    implied_duration;
   short   num_frames;
   int         close;
   ushort  incidental;
   mxs_real    spine_len;        // Length from the butt to the neck.
   mxs_real    hip_height;
   mxs_real    hip_width;
   mxs_real    rshldr_len;
   mxs_real    lshldr_len;
   mxs_real    inshoul_width;
   mxs_real    shoul_per;
   mxs_real    ab_per;
} trunk_data;

typedef struct trunk_capture {
   mxs_vector base;
   mxs_matrix base_orient; // num_frames array of base position (rel to base at
                          // time 0)
   mxs_angvec base_angs;
   mxs_matrix ab_orient; // num_frames array of neck position (rel to base)
   mxs_angvec ab_angs;
   mxs_vector rshldr;
   mxs_vector lshldr;
} trunk_capture;


typedef struct Trunk_sched {
   int         type;
   int         table_id;
   mxs_real        start_time;
   mxs_real        duration;
   short   num_frames;
   int     Rel_or_Abs;
   int         close;
   trunk_data  trunk_params;
   mxs_vector  start_pos;
   mxs_vector  end_pos;
   mxs_real    spine_len;        // Length from the butt to the neck.
   mxs_real    hip_height;
   mxs_real    hip_width;
   mxs_real    rshldr_len;
   mxs_real    lshldr_len;
   mxs_real    shoul_per;
   mxs_real    ab_per;

   void *start_end_data;
   void (*start_or_end)(int);
   mxs_real (*speed)(mxs_real);
   mxs_real do_time_increment;
   //   void (*do_sched)(bipeds_info *,mxs_real, mxs_real);
   //void (*do_simpsched)(bipeds_info *,mxs_real, mxs_real);
   //void (*set_sched)(bipeds_info *,mxs_real, int, int);
   void (*do_sched)(void *,void *,mxs_real, mxs_real);
   void (*do_simpsched)(void *,void *,mxs_real, mxs_real);
   void (*set_sched)(void *,void *,mxs_real, int, int);
} Trunk_sched;

typedef union hand_capture {
      effec_capture effec_table;
      effweap_capture weap_table;
} hand_capture;

typedef union hand_sched {
     Effec_sched     empt_hand;
     Effweap_sched weap_hand;     
} Hand_sched;

#define LS_EDATA_OFFSET (sizeof(long)*2+sizeof(short)*4+sizeof(fix)*4)

#define TS_DATA_OFFSET  (sizeof(long)*3+sizeof(short)*3+sizeof(fix)*3)
#endif






