#ifndef __mpmstr_h
#define __mpmstr_h

//

#include <res.h>

//

#define MPCT_NULL        0
#define MPCT_TORSO       1
#define MPCT_LIMB        2
#define MPCT_EFFECTOR    3
#define MPCT_HAND        4 //'cuz it's lame

#define MPCF_LEFT     0x1

//
  
typedef struct
{
   uchar ctype; // effector, torso, limb
   uchar flags; // right, left etc.
   Ref ref;
} mpm_cmot_handle;

//

#define MP_MOT_INVALID -1

//

typedef struct
{
	int		num_frames;
	int		frequency;
	ulong	signature;
	uchar	app_type;
	uchar	app_data[63];
} mpm_motion_info;

//

#define MAX_COMPONENTS	22
#define MAX_FLAGS		32	// is this enough?

//

typedef struct
{
	int		frame;
	uint	flag;
} mpm_motion_flag;

typedef struct
{
   int 				handle;
   mpm_motion_info	info;
   int				num_components;
   mpm_cmot_handle	component[MAX_COMPONENTS];
   int				num_flags;
   mpm_motion_flag	flags[MAX_FLAGS];

} mpm_motion;

//

#endif