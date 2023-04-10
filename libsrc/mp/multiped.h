//

#ifndef MULTIPED_H
#define MULTIPED_H

//

#include <mplist.h>

//

#define MAX_OVERLAY_MOTIONS	4

// Max sig joints is determined by our 32-bit motion signature. Need one bit per
// joint, so for more than 32 joints that can have rotations,
// we'll need to go to a 64-bit signature.  If multiped has more than 32 joints,
// make sure all joints >= 32 are "effector" joints that exist for bbox purposes,
// but never have animation data for them.
#define MAX_SIG_JOINTS			32		
#define MAX_JOINTS            48


//
// Data type definitions.
//

typedef struct mps_overlay
{
   MotionSig		sig;
   mps_motion_list	list;
   // mode is -1 while transitioning into, 0 while playing, and 1 while
   // transitioning out of.
   int				mode;
   float			time;
   float			duration;
} mps_overlay;

//

// frame of reference types for multipeds
#define MFRT_REL_START  0
#define MFRT_GLOBAL     1

typedef struct multiped
{
   // Use this for pointer back to object containing multiped, e.g. appbiped or
   // whatever. Will be useful in virtual functions so you can get back to app-
   // specific data above the multiped level.
   void *				app_ptr;	

   int					num_torsos;
   torso *				torsos;
						
   int					num_limbs;
   limb *				limbs;
						
   int					num_joints;
   mxs_vector *		joints;

// Some joint indices needed by multiped library.
   int					root_joint;

   uchar *				joint_map;

//
// For maximum generality, instead of normal vectors (specific to
// articulated-bitmap guys), this should be a list of rotation matrices 
// (or quaternions) for each joint which you'll need for polygonal creatures.
// You can always get the normal, which will be the j-vector of the matrix.

   quat *				rel_orients;
   mxs_vector  		rel_xlat;
   mxs_matrix *		orients;
   mxs_vector *		norm;

   mps_motion_list		main_motion;

   int					num_overlays;
   mps_overlay			overlays[MAX_OVERLAY_MOTIONS];

   mxs_vector			global_pos;
   mxs_matrix			global_orient;

// it gets bigger and bigger.
   quat				target_heading;
   float				th_time;
   float				th_duration;

   int            frame_of_reference; // whether xlat and root rot are in
                                      // world coords or rel to beginning pos. 

} multiped;

//
// Global variables.
//

EXTERN mps_motion * mp_motion_list;
EXTERN int mp_max_motions;
EXTERN int mp_num_motions;
EXTERN float * mp_ramp_table;
EXTERN int mp_joint_map[];
EXTERN void (*mp_compute_limb_normal)(mxs_vector * normal, 
                                      multiped * mp, 
                                      limb * l, 
                                      mxs_vector * end_pos);

//
// Initialize multiped system. Must be called before any other multiped 
// functions. 
// Returns TRUE on success, FALSE on failure.
//
EXTERN bool mp_init(int max_motions);

// set callbacks to use when getting component frame capture data, otherwise,
// default is used.
EXTERN void mp_set_capture_callbacks(mps_component_xlat_callback_func xfunc,mps_component_rot_callback_func rfunc,mps_root_rot_callback_func bfunc);

EXTERN void mp_set_rot_filter_callback(mps_rot_filter_callback rffunc);

// XXX this should really go somewhere else
typedef void (*mps_motion_state_callback)(int mot_num);

EXTERN void mp_set_motion_start_end_callbacks(mps_motion_state_callback startFunc, mps_motion_state_callback endFunc);

//
// Shutdown the multiped system.
//
EXTERN void mp_close(void);

//
// Set up a multiped. ASSUMES num_limbs, num_torsos, limb and torso arrays,
// and position are correct.
EXTERN void mp_init_multiped(multiped * mp);

//
// Corresponding multiped shutdown function. Needs to free some internal
// data, etc. If you don't call this you'll find your memory leaking.
//
EXTERN void mp_free_multiped(multiped * mp);

//
// Clone one multiped to into another.  Allocs space necessary for 
// destination multiped, so clone must be freed using mp_free_multiped
// when done to avoid memory leaks.
EXTERN BOOL mp_clone_multiped(multiped *dst, multiped *src);

//
// The following functions allow adding various types of motions to the 
// motion tables. They all return a motion handle for use in subsequent
// calls to start_motion(), stop_motion(), etc. Negative return values
// indicate failure.
//

// Add app-constructed motion. This probably won't be used much, since you'll
// usually be loading motions from a resource. Can be used to add virtual
// motions, or use mp_add_virtual_motion() below.
EXTERN int mp_add_motion(mps_motion * motion);

// Add motion from a resource.
EXTERN int mp_add_res_motion(Id motion_id);

// Add a virtual motion to the motion tables. Returns a motion handle for use 
// in subsequent calls to start_motion(), stop_motion(), etc. Negative return
// value indicates failure.
EXTERN int mp_add_virtual_motion(	mp_vm_func virtual_motion, 
                                int num_frames, 
                                int frequency);

// add a motion callback to the next free slot, return slot number
// -1 if no free slots
EXTERN int mp_add_callback(motion_callback callback);

// set a particular motion callback
EXTERN void mp_set_callback(int callback_num, motion_callback callback);

//
// mp_start_motion() -- start the multiped playing the specified motion.
// If invalid motion_num is specified, function does nothing.
//
// The callback function pointer is optional, pass NULL if you don't want a
// callback. If you provide a callback function pointer, it will be called 
// when the specified frame is reached. If you pass in -1 for the frame, the 
// callback will occur when the last frame is reached. A motion_callback()
// function looks like:
//
// void motion_callback(multiped * mp, int motion_number, int frame);
//
// The frame will be the frame you passed into mp_start_motion().
//

//
// To reduce number of parameters passed and stabilize the API, you need to 
// fill out this structure, mps_start_info, with the appropriate information 
// and pass its address to mp_start_motion().
//
typedef struct mps_start_info
{
   int					motion_num;
   int					start_frame;
   motion_callback		callback;		// can be NULL.
   int  callback_num;           // which multiped library callbacks to make
   int					callback_frame;
   mps_motion_param *	params;			// can be NULL for defaults.
   float				trans_duration;
} mps_start_info;

//

EXTERN void mp_start_motion(multiped * mp, mps_start_info * info);

//
// mp_stop_motion() -- Stops the specified motion ON THE NEXT CALL TO
// mp_update() FOR THIS MULTIPED. In order for stopping motions to work in 
// callbacks, we don't stop motions immediately, but rather flag the motion
// to be stopped on the next update.
//
EXTERN void mp_stop_motion(multiped * mp, int motion_num);

//
// Stops all motions ON THE NEXT CALL TO mp_update() FOR THIS MULTIPED. See
// comments on mp_stop_motion().
//
EXTERN void mp_stop_all_motions(multiped * mp);

//
// mp_update()  -- Call this function as often as you want to update a 
// multiped, probably once per frame. The time delta is elapsed time (in 
// milliseconds) since you last called update for this multiped. Returns any
// flags which have occurred since last update.
//
// app_flags are passed along to app callbacks.  useful for "fake" updates etc
EXTERN uint mp_update(multiped * mp, float time_delta_ms, ulong app_flags);

//
// Multiped locomotion API follows:
//

// mp_get_heading(), mp_set_heading() -- heading is absolute heading in 
// radians, normalized so the result is always between 0 and 2pi. 0 is 
// pointing down the positive x-axis, pi/2 is pointing down positive y-axis,
// etc. dt is how long to take to get to desired heading.
EXTERN BOOL mp_get_heading(multiped * mp, float *pHeading);

// mp_set_heading() returns FALSE if unable to set the heading for any reason.
// Currently it only fails when no main (whole-body) motion is playing.
EXTERN bool mp_set_heading(multiped * mp, float heading, float dt);

// mp_compute_loco() computes the bend and/or stretch needed for the specified
// motion to reach the specified endpoint. ANY BEND OR STRETCH SPECIFIED IN
// THE mps_start_info STRUCT PASSED TO THIS FUNCTION IS IGNORED.
EXTERN void mp_compute_loco_params(float * bend, float * stretch, multiped * mp, 
                            mps_start_info * info, mxs_vector * endpoint);

// If the endpoint is out of the motion's range without an unreasonable amount
// of stretch, call mp_compute_approach() to compute a bend which will leave 
// the multiped facing the endpoint at the end of the motion. ASSUMES ENDPOINT
// IS OUT OF MOTION'S RANGE. Assumes no stretch or compress will be applied.
EXTERN void mp_compute_approach(float * bend, multiped * mp, mps_start_info * start,
                         mxs_vector * endpoint);

EXTERN void mp_compute_pass_through(float * bend, multiped * mp, 
                             mps_start_info * info, mxs_vector * endpoint);

// mp_compute_endpoint() fills in the endpoint of the motion based on the 
// specified multiped's current position and orientation.
EXTERN void mp_compute_endpoint(mxs_vector * endpoint, multiped * mp, 
                         mps_start_info * info);

// This allows you to change bend and stretch mid-motion. Effectively restarts
// the motion (at current frame) with new bend and stretch.
EXTERN void mp_reset_loco_params(multiped * mp, float bend, float stretch);

EXTERN void mp_reset_vstretch(multiped * mp, float vstretch);

// Get the bend and stretch being applied to the current main motion
// If no main motion, return FALSE
EXTERN bool mp_get_loco_params(multiped * mp, float *bend, float *stretch);

EXTERN bool mp_get_vstretch(multiped *mp, float *vstretch);

// mp_start_locomotion() starts specified (locomotive) motion, moving multiped
// to specified endpoint. Automatically deals with bend/stretch.
EXTERN void mp_start_locomotion(	multiped * mp, 
                                mps_start_info * info, 
                                mxs_vector * endpoint);

// Pass in pointer to array of num_points vectors, and it fills in that number
// of equally spaced points sampled from the motion path.
EXTERN void mp_sample_loco_path(multiped * mp, mps_start_info * info, int num_points, 
                         mxs_vector * points);

// Call mp_count_flags() to find out how many times a given set of flags occurs
// in specified motion. To count footfalls, for example, flag footfall frames
// and then count occurrences of relevant flag.
EXTERN int mp_motion_count_flags(int motion_num, int start_frame, uint search_flags);

// do count_flags on the multiped's main motion
EXTERN int mp_count_flags(multiped *mp, uint search_flags);

// Return the next frame on which any of the specified flags occurs
// -1 if no such flags in motion
EXTERN int mp_motion_next_flags(int motion_num, int start_frame, uint search_flags);

// do next_flags on multiped's main motion
EXTERN int mp_next_flags(multiped *mp, uint search_flags);

EXTERN int mp_motion_last_flags(int motion_num, int start_frame, uint search_flags);

EXTERN int mp_last_flags(multiped * mp, uint search_flags);

// Calculate a joint position at a given frame offset based on the
// current main motion. Does nothing if no main motion
EXTERN void mp_forecast_joint_pos(multiped *mp, mps_start_info *info, float frame, 
                           int joint, mxs_vector *pos);

//
// END PUBLIC API. The following stuff is for internal use only, use at your
// own risk.
//

// WARNING: THIS SAVE/RESTORE THING PROBABLY DOESN'T DO WHAT YOU THINK IT DOES.
// Use it carefully, if at all. IT IS NOT REENTRANT. IF YOU NEST THEM, IT WILL 
// BREAK. THERE IS ONE AND ONLY ONE SLOT FOR SAVING AND RESTORING MULTIPED 
// STATES. Also, they don't save and restore the ENTIRE state, just the joint 
// positions, orientations, and rel_orients, including global_pos and 
// global_orient. Motion info is NOT saved.
EXTERN void mp_save_mp_state(multiped * mp);
EXTERN void mp_restore_mp_state(multiped * mp);

//

EXTERN mps_motion_node * mp_get_main_motion(multiped * mp);
EXTERN void mp_change_heading(multiped * mp, float dt);
EXTERN void mp_initial_update(multiped * mp);

EXTERN bool mp_get_motion_data(multiped * mp, 
                        mps_motion * m, 
                        int frame, quat * rot, 
                        mxs_vector * trans);

EXTERN void mp_apply_motion(multiped * mp, quat * rot, mxs_vector * trans, 
                     int stop_joint);

EXTERN void mp_build_tables(void);
EXTERN void mp_compute_ep(multiped * mp, mps_motion_node * m);
EXTERN void mp_get_xlat_at_frame(mxs_vector * xlat, int motion, int frame);
EXTERN void mp_kill_motions(multiped *mp, mps_motion_list * list);
EXTERN void mp_get_config_at_time(multiped * mp, mps_motion_node * m, float time, 
                           int stop_joint);
EXTERN void mp_get_config_at_frame(multiped * mp, mps_motion_node * m, float frame_offset);

//

typedef struct mps_blend_info
{
   int			num_rotations;
   float		param;			// interpolation parameter, 0 <= param <= 1.

   quat *		r1;
   mxs_vector  t1;

   quat *		r2;
   mxs_vector  t2;

   quat *		rot;
   mxs_vector	xlat;
} mps_blend_info;

//

EXTERN mps_motion_node * mp_alloc_motion(multiped * mp, mps_start_info * info);
EXTERN void mp_setup_motion(multiped * mp, mps_motion_node *m);
EXTERN mps_motion_node * mp_setup_current_pose(multiped * mp);

EXTERN BOOL mp_update_list(multiped * mp, mps_overlay * ov, mps_motion_list * list,
                    float dt, uint * flags);

EXTERN void mp_evaluate_motions(multiped * mp, mps_motion_list * list, quat * rot, 
                         mxs_vector * xlat);

EXTERN void mp_evaluate_stack(multiped * mp, quat * rot, mxs_vector * xlat);

EXTERN void mp_evaluate_motion(multiped * mp, mps_motion_node * m, quat * rot, 
                        mxs_vector * xlat);

EXTERN void mp_evaluate_transition(multiped * mp, mps_transition_node * trans, 
                            quat * rot, mxs_vector * xlat);

EXTERN void mp_blend_motions(mps_blend_info * b);
EXTERN void mp_compute_joint_map(multiped * mp, int * map);
EXTERN void mp_compute_arm_normal(mxs_vector * n, multiped * mp, limb * l, 
                           mxs_vector * end_pos);

EXTERN void mp_compute_leg_normal(mxs_vector * n, multiped * mp, limb * l, 
                           mxs_vector * end_pos);

EXTERN void mp_compute_limb_pos(mxs_vector * mid, limb * l, mxs_vector * end, 
                         mxs_vector * normal, int bend);

EXTERN void mp_compute_limb_rot(quat * rot, mxs_matrix * parent, limb * l, 
                         mxs_vector * pos, mxs_vector * normal, 
                         mxs_matrix * end_orient);


EXTERN void mp_solve_limb(multiped * mp, mps_effector_place * ep, quat * rot);

EXTERN void mp_place_effector(multiped * mp, mps_motion_node * m, quat * rot);

// Memory debug stuff.
EXTERN int mp_alloc_count;
EXTERN void * mp_alloc(size_t size, char *, int);
EXTERN void mp_free(void * ptr);

//

#define MP_SRC	DBGSRC(31, 1)

//

#endif



