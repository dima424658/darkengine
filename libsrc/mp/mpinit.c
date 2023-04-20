//
// mpinit.c - Multiped init/shutdown/motion loading stuff.
//

#include <math.h>
#include <string.h>
#include <lg.h>
#include <multiped.h>
#include <motstruc.h>

//
// Some global variables.
//

mps_motion * mp_motion_list = NULL;
int 	mp_max_motions = 0;
int 	mp_num_motions = 0;
float *	mp_ramp_table = NULL;
int		mp_joint_map[MAX_JOINTS];
int		mp_num_joints_by_sig[MAX_JOINTS];
motion_callback *mp_callbacks;


static void mp_default_capture_comp_xlat_func(multiped *mp,mps_motion_info *m, mps_comp_motion *cm, int frame, mxs_vector *data);
static void mp_default_capture_comp_rot_func(mps_motion_info *m, mps_comp_motion *cm, int frame, quat *data);
// @HACK:
void mp_default_capture_root_rot_func(multiped *mp,mps_motion_info *m, int frame, quat *data);

// this is externed by mpupdate.c and mpmot.c
mps_component_xlat_callback_func mp_capture_component_xlat_func=mp_default_capture_comp_xlat_func;
mps_component_rot_callback_func mp_capture_component_rot_func=mp_default_capture_comp_rot_func;
mps_root_rot_callback_func mp_capture_root_rot_func=mp_default_capture_root_rot_func;
mps_rot_filter_callback mp_rot_filter_callback=NULL;

mps_motion_state_callback mp_motion_start_callback=NULL;
mps_motion_state_callback mp_motion_end_callback=NULL;

void (*mp_compute_limb_normal)(mxs_vector * normal, multiped * mp, limb * l, 
							   mxs_vector * end_pos) = NULL;

//
// Some functions.
//
bool mp_init(int max_motions)
{
	bool result;
        int i;

#ifdef MP_LIB_TEST
	DbgSetLogFile(MP_SRC, "mpdbg.txt");
#endif

	mp_max_motions = max_motions;
	mp_num_motions = 0;
	mp_motion_list = (mps_motion *) mp_alloc(max_motions * sizeof(mps_motion), __FILE__, __LINE__);

	result = (mp_motion_list != NULL);

	mp_build_tables();

   mp_callbacks = (motion_callback*)mp_alloc(MP_NUM_CALLBACKS*sizeof(motion_callback),
                                             __FILE__, __LINE__);
   for (i=0; i<MP_NUM_CALLBACKS; i++)
      mp_callbacks[i] = NULL;

   mp_motion_start_callback=NULL;
   mp_motion_end_callback=NULL;
	return result;

}

//

void mp_close(void)
{
	int i;
	mps_motion * m;

	if (mp_motion_list)
	{
		m = mp_motion_list;
		for (i = 0; i < mp_num_motions; i++, m++)
		{
			switch (m->info.type)
			{
				case MT_CAPTURE:
					mp_free(m->components);
					m->components = NULL;
				   	break;
			}

			if (m->num_flags)
			{
				mp_free(m->flags);
				m->flags = NULL;
			}
		}

		mp_free(mp_motion_list);
		mp_motion_list = NULL;
	}
	
   if(mp_ramp_table)
   {
   	mp_free(mp_ramp_table);
	   mp_ramp_table = NULL;
   }

   if(mp_callbacks)
   {
      mp_free(mp_callbacks);
      mp_callbacks=NULL;
   }

	if (mp_alloc_count)
	{
		Spew(MP_SRC, ("memory leak. %d blocks not freed.", mp_alloc_count));
	}
}

//
// Set up the multiped's torso, limb, and effector data, and set the initial
// joint positions by applying identity transformations for all the joint
// rotations.
//
void mp_init_multiped(multiped * mp)
{
	int i;

	mx_identity_mat(&mp->global_orient);

	mp->num_overlays = 0;

   // @HACK: add extra "joint" for base/root orient
	mp->rel_orients = (quat *) mp_alloc(sizeof(quat) * (mp->num_joints+1), __FILE__, __LINE__);
	for (i = 0; i < mp->num_joints; i++)
	{
		quat_identity(&mp->rel_orients[i]);
	}
   mx_zero_vec(&mp->rel_xlat);

// Set up motion lists.
	mp_list_init(&mp->main_motion);
	for (i = 0; i < MAX_OVERLAY_MOTIONS; i++)
	{
		mp_list_init(&mp->overlays[i].list);
	}

// Set up joint map.
	mp->joint_map = (uchar *) mp_alloc(mp->num_joints, __FILE__, __LINE__);
	memset(mp->joint_map, 0xff, mp->num_joints);

	mp->th_duration = -1;
	quat_identity(&mp->target_heading);

// Set initial joint positions.
	mp_initial_update(mp);
}

//

void mp_free_multiped(multiped * mp)
{
	int i;

	mp_free(mp->rel_orients);

	mp_list_free(&mp->main_motion);
	for (i = 0; i < MAX_OVERLAY_MOTIONS; i++)
	{
		mp_list_free(&mp->overlays[i].list);
	}

	mp_free(mp->joint_map);
}


BOOL mp_clone_multiped(multiped *dst, multiped *src)
{
   int i;
   BOOL good=TRUE;
   mxs_vector *pJoints=dst->joints;
   mxs_matrix *pOrients=dst->orients;

   *dst=*src;

// Copy joint position and orientation values
   dst->joints=pJoints;
   dst->orients=pOrients;
   if(dst->joints&&src->joints)
   {
      memcpy(dst->joints,src->joints,dst->num_joints*sizeof(*(dst->joints)));
   }
   if(dst->orients&&src->orients)
   {
      memcpy(dst->orients,src->orients,dst->num_joints*sizeof(*(dst->orients)));
   }

// Set up relative orients
   // @HACK: add extra "joint" for base/root orient
	dst->rel_orients = (quat *) mp_alloc(sizeof(*(dst->rel_orients)) * (dst->num_joints+1), __FILE__, __LINE__);
   memcpy(dst->rel_orients,src->rel_orients,sizeof((*(dst->rel_orients)))*(dst->num_joints+1));
   mx_copy_vec(&dst->rel_xlat,&src->rel_xlat);

// Set up joint map.
	dst->joint_map = (uchar *) mp_alloc(dst->num_joints, __FILE__, __LINE__);
   memcpy(dst->joint_map,src->joint_map,dst->num_joints);

// Set up motion lists
	good=mp_list_clone(&dst->main_motion,&src->main_motion)&&good;
   // want to go through them all even if list cloning fails, since cloning
   // first initialized the motion list
	for (i = 0; i < MAX_OVERLAY_MOTIONS; i++)
	{
		good=mp_list_clone(&dst->overlays[i].list,&src->overlays[i].list)&&good;
	}
   if(!good)
   {
      mp_free_multiped(dst);
      return FALSE;
   }
   return TRUE;
}


//
// All other mp_add_???_motion() functions should call this to do the actual
// motion add.
//
int mp_add_motion(mps_motion * motion)
{
	int result;
	mps_motion * m;

	if (mp_num_motions < mp_max_motions)
	{
		m = mp_motion_list + mp_num_motions;
		memcpy(m, motion, sizeof(mps_motion));
      m->info.mot_num=mp_num_motions;
		result = mp_num_motions++;
	}
	else
	{
		result = -1;
	}

	return result;
}

//

int mp_add_res_motion(Id motion_id)
{
	int result, i;
	mpm_motion mot;
	mpm_cmot_handle * cms;
	mps_comp_motion * cmd;
	mps_motion m;

// Extract the motion header into a temporary structure, then build the
// real motion that we'll keep around.

	if (ResExtract(motion_id, (void *) &mot))
	{
		Spew(MP_SRC, ("mp_add_res_motion(): %d, %d frames\n", mp_num_motions, mot.info.num_frames));

		m.info.type = MT_CAPTURE;
		m.info.sig = mot.info.signature;
		m.info.num_frames = mot.info.num_frames;
		m.info.freq = mot.info.frequency;

		m.num_components = mot.num_components;
		m.components = (mps_comp_motion *) mp_alloc(sizeof(mps_comp_motion) * m.num_components, __FILE__, __LINE__);

		cms = mot.component;
		cmd = m.components;

		for (i = 0; i < m.num_components; i++, cms++, cmd++)
		{
			cmd->type = cms->ctype;
			cmd->joint_id = cms->flags;
			cmd->handle = (ulong)cms->ref;
		}

		m.num_flags = mot.num_flags;
		if (m.num_flags)
		{
			m.flags = (mps_motion_flag *) mp_alloc(sizeof(mps_motion_flag) * m.num_flags, __FILE__, __LINE__);
			memcpy(m.flags, mot.flags, sizeof(mps_motion_flag) * m.num_flags);
		}

		result = mp_add_motion(&m);
	}
	else
	{
		result = -1;
	}

	return result;
}

//

int mp_add_virtual_motion(mp_vm_func virtual_motion, int num_frames, int frequency)
{
	int result;
	mps_motion m;

	m.info.type = MT_VIRTUAL;
	m.info.num_frames = num_frames;
	m.info.freq = frequency;
	m.virtual_update = virtual_motion;

	m.num_flags = 0;
	m.flags = NULL;

	result = mp_add_motion(&m);

	return result;
}

//

void mp_build_tables(void)
{
	int i, j, bits;
	int * count;
	float d;
	float * r;

// Build ramp table.
	mp_ramp_table = (float *) mp_alloc(sizeof(float) * 256, __FILE__, __LINE__);

	r = mp_ramp_table;
	for (i = 0; i < 256; i++, r++)
	{
		d = MX_REAL_PI * i / 255;
		*r = (1.0 - cos(d)) / 2.0;
	}

// Build signature lookup table. Coun bits in each signature from 0 to 31.
	memset(mp_num_joints_by_sig, 0, sizeof(int) * MAX_JOINTS);
	count = mp_num_joints_by_sig + 1;
	for (i = 1; i < MAX_JOINTS; i++, count++)
	{
		bits = 1 + log(i) / log(2);
		for (j = 0; j < bits; j++)
		{
			if (i & (1 << j))
			{
				(*count)++;
			}
		}			
	}
}

//
// These are just wrappers for malloc() and free() so I can track down 
// memory leaks.
//

int mp_alloc_count = 0;

void * mp_alloc(size_t size, const char * file, int line)
{
	void * result = Malloc(size);

	//printf("alloc %X %s %d\n", result, file, line);

	mp_alloc_count++;
	return result;
}

//

void mp_free(void * ptr)
{
	//printf("free  %X\n", ptr);
	mp_alloc_count--;
	Free(ptr);
}

//

int mp_add_callback(motion_callback callback)
{
   int i;

   for (i=0; i<MP_NUM_CALLBACKS; i++)
   {
      if (!mp_callbacks[i])
      {
         mp_callbacks[i] = callback;
         return (1<<i);
      }
   }
   return -1;
}

void mp_set_callback(int callback_num, motion_callback callback)
{
   int i;

   for (i=0; i<MP_NUM_CALLBACKS; i++)
   {
      if (callback_num&(1<<i))
         mp_callbacks[i] = callback;
   }
}

//

void mp_default_capture_comp_xlat_func(multiped *mp, mps_motion_info *m, mps_comp_motion *cm, int frame, mxs_vector *data)
{
   mxs_vector *vec;

   vec = (mxs_vector *) RefLock((Ref)cm->handle);
   mx_copy_vec(data, &vec[frame]);
   RefUnlock((Ref)cm->handle);
}

//

void mp_default_capture_comp_rot_func(mps_motion_info *m, mps_comp_motion *cm, int frame, quat *data)
{
   quat *q;

   q = (quat *) RefLock((Ref)cm->handle);
   quat_copy(data, &q[frame]);
   RefUnlock((Ref)cm->handle);
}

//

void mp_default_capture_root_rot_func(multiped *mp, mps_motion_info *m, int frame, quat *data)
{
   quat_identity(data);      
}

//

void mp_set_capture_callbacks(mps_component_xlat_callback_func xfunc,mps_component_rot_callback_func rfunc,mps_root_rot_callback_func bfunc)
{
   mp_capture_component_xlat_func=xfunc;
   mp_capture_component_rot_func=rfunc;
   mp_capture_root_rot_func=bfunc;
}

//

void mp_set_rot_filter_callback(mps_rot_filter_callback rffunc)
{
   mp_rot_filter_callback=rffunc;
}

//

void mp_set_motion_start_end_callbacks(mps_motion_state_callback startFunc, mps_motion_state_callback endFunc)
{
   mp_motion_start_callback=startFunc;
   mp_motion_end_callback=endFunc;
}

