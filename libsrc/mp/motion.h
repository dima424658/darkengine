// $Header: x:/prj/tech/libsrc/mp/RCS/motion.h 1.20 1998/08/04 13:25:34 kate Exp $
//
// Motion.h -- structures and definitions used by multiped library.
//

#ifndef MOTION_H
#define MOTION_H

//

#include <qt.h>
#include <res.h>

//
// Define a torso segment as follows:
// 
// parent or not. (e.g. abdomen is relative to pelvis).
// fixed points. (e.g. hips, abdomen, shoulders, neck).
//

#define NUM_TORSO_POINTS        16

//
// The pelvis position (relative to the world) changes from frame to frame,
// but the abdomen position (relative to the pelvis) is fixed, so for 
// each abdomen frame all we need to store is a relative rotation,
// not a torso_frame struct.
//
// So if a torso has a parent, its per-frame data is just a quaternion. 
// If not, it's a torso_frame struct.
//

typedef struct torso
{
   int			joint;
   int			parent;					// index into torso list or -1.
   int 		num_fixed_points;
   int			joint_id[NUM_TORSO_POINTS];	
   mxs_vector	pts[NUM_TORSO_POINTS];
} torso;

//

typedef struct torso_frame
{
   mxs_vector	pos;
   quat		orient;
} torso_frame;

//

#define NUM_LIMB_SEGS           16
#define NUM_LIMB_JOINTS	(NUM_LIMB_SEGS+1)

typedef struct limb
{
   int			torso_id;			// which torso does this limb hang off?

   int			bend;				// Which way does it bend? 0 for arms, 1 
   // for legs.

   int			num_segments;
   short 		joint_id[NUM_LIMB_JOINTS];

   // Unit vectors in default direction.
   mxs_vector 	seg[NUM_LIMB_SEGS];

   // Seg lengths for scaling unit vectors.
   mxs_real 	seg_len[NUM_LIMB_SEGS];
} limb;

//
// This is silly, all we need is a quaternion per joint per frame for
// everything but the pelvis.
//
typedef struct limb_frame
{
   quat 	orient[NUM_LIMB_SEGS];
} limb_frame;

//
// Use these values to set the mps_motion_param flags field. The flags value
// determines which other members of the structure are valid. 
//
#define MP_DURATION_SCALE		0x0001
#define MP_BEND					0x0002
#define MP_PLACE_EFFECTOR		0x0004
#define MP_STRETCH				0x0008
#define MP_FIXED_DURATION		0x0010
#define MP_MIRROR				   0x0020
#define MP_VSTRETCH           0x0040
#define MP_VINC           0x0080

//

typedef void (*mp_mirror_func)(quat * rot, mxs_vector * xlat, uint sig);

//

typedef struct mps_effector_place
{
   limb *		l;

   // ALL these times and durations are in milliseconds. 

   // start_approach_time is the time, relative to the start of the motion, when
   // the blending toward the effector target starts.
   float		start_approach_time;

   // placement_time is the time, relative to the start of the motion, when the
   // effector will be at the specified position.
   float		placement_time;

   // placement_duration says how long the effector stays at the target position.
   float		placement_duration;

   // trans_out_duration says how long to take to blend back into the normal
   // motion data after the placement.
   float		trans_out_duration;

   mxs_vector	pos;
   mxs_matrix	orient;

   // If TRUE, uses orient field. Otherwise ignores orient field, uses default
   // hand or foot orient as appropriate. Default should provide reasonable 
   // orientations in most situations.
   bool		use_orient;

   // set absolute to TRUE if position and orientation are absolute, FALSE if  
   // they're relative to base joint.
   bool		absolute;
   bool		stick;
} mps_effector_place;

//
// If you decide some of these parameters are mutually exclusive, make this a
// union to save some space. Also effector_place could be a pointer to an
// mps_effector_place struct, which the user has to allocate space for.
//
typedef struct mps_motion_param
{
   // What are the motion parameters?
   uint				flags;

   // Duration scalar controls speed of motion. Bigger number means motion lasts
   // longer.
   float				duration_scalar;

   // Bend is rotation of path around starting point. Heading changes 2 * bend
   // over course of motion. In radians.
   // SEE ALSO mp_set_heading() for on-the-fly heading changes.
   float				bend;

   // Stretch is scalar applied to linear distance traversed over course of
   // motion. If it's > 1, you get stretch; < 1, compression.
   // jon: stretch redifined to mean horizontal stretch only. This is the
   // way that the locomotion related functions used it anyway.
   float				stretch;

   // jon: added vertical stretch parameter
   float vstretch;

   // jon: added vertical increment parameter
   float vinc;

   // See mps_effector_place structure above.
   mps_effector_place	ep;

   // fixed_duration is for virtual motions which don't have a fixed number of
   // frames. This specifies how long the motion plays before stopping.
   float				fixed_duration;

   mp_mirror_func		mirror;

} mps_motion_param;

//

struct multiped;

// Arbitrary callbacks
typedef int (*motion_callback)(struct multiped * mp, int motion_num, int frame, float time_slop, ulong flags);

// reserved callback flags
#define MP_CFLAG_MOT_ABORTED 0x1
#define MP_RESERVED_CFLAG_SHIFT 4

// Pre-defined callbacks must be registered through mp_add_callback
extern motion_callback *mp_callbacks;
#define MP_NUM_CALLBACKS sizeof(int)*8

// MotionSig is a bitfield defining which joints the motion affects.
// If more than 32 joints are needed, this will get more complicated.
typedef unsigned long MotionSig;

// AllJointSig() evaluates to the signature affecting joints 0 to 
// num_joints - 1.
#define AllJointSig(num_joints)			((1 << num_joints) - 1)

#define JointSig(joint_id)      (1<<joint_id)

// Set the specified joint's bit.
#define SetSigJoint(sig, joint_id)		(sig |= (1 << joint_id))

// QuerySigJoint() evaluates to TRUE if the motion affects the specified 
// joint, FALSE if not.
#define QuerySigJoint(sig, joint_id)	((joint_id<MAX_SIG_JOINTS)?(sig & (1 << joint_id)):(0))

// mp_num_joints_by_sig[] is a lookup table. mp_num_joints_by_sig[signature] =
// the number bits set in the signature.
extern int mp_num_joints_by_sig[];

//
// 
//
#if 0
typedef enum
{
   CM_ROT,
   CM_TRANS
} CMType;
#endif // get rid of enums

typedef int CMType;

#define CM_ROT    0
#define CM_TRANS  1

//

typedef struct mps_comp_motion
{
   CMType	type;
   int 	joint_id;
   ulong		handle;
} mps_comp_motion;

//
#if 0
typedef enum
{
   MT_CAPTURE,
   MT_VIRTUAL
} MotionType;
#endif // get rid of enums

typedef int MotionType;

#define MT_CAPTURE 0
#define MT_VIRTUAL 1

//

#define MOTINFO_PAD_SIZE 63
#define MOTINFO_NAME_LEN 11

typedef struct mps_motion_info
{
   MotionType	type;
   MotionSig	sig;
   float	num_frames;
   int		freq;
   int      mot_num;
   char name[MOTINFO_NAME_LEN+1];
   uchar	app_type;
   uchar	app_data[MOTINFO_PAD_SIZE];
} mps_motion_info;

//

#define mp_motion_num_frames(motion) ((mp_motion_list+motion)->info.num_frames)

//
// Virtual motion function.
//
typedef bool (*mp_vm_func)(struct multiped * mp,
                           mps_motion_info * m, 
                           int frame, 
                           quat * rot, 
                           mxs_vector * trans);

//

typedef struct mps_motion_flag
{
   int		frame;
   uint	flags;
} mps_motion_flag;

//

typedef struct mps_motion
{
   mps_motion_info      info;

   union
   {
      struct
      {
         int			num_components;
         mps_comp_motion        *components;
      };
      mp_vm_func	virtual_update;
   };
   int  		num_flags;
   mps_motion_flag *	flags;

} mps_motion;

//

//
// Component Motion Data Callback - for Captured motions
// used to get vector or quat data for a joint for a frame
//
typedef void (*mps_component_xlat_callback_func)(struct multiped *mp, mps_motion_info *mi, mps_comp_motion *cm, float frame, mxs_vector *data);
typedef void (*mps_component_rot_callback_func)(mps_motion_info *mi, mps_comp_motion *cm, float frame, quat *data);
typedef void (*mps_root_rot_callback_func)(struct multiped *mp, mps_motion_info *mi, float frame, quat *data);

// filter callback, to modify rots after individual callbacks made
typedef void (*mps_rot_filter_callback)(struct multiped *mp,mps_motion_info *m,quat *rot,mxs_vector *xlat);

//

#endif