//
// mpinit.c - Multiped init/shutdown/motion loading stuff.
//

#include <math.h>
#include <string.h>
#include <stdint.h>
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


static void mp_default_capture_comp_xlat_func(multiped *mp,mps_motion_info *m, mps_comp_motion *cm, float frame, mxs_vector *data);
static void mp_default_capture_comp_rot_func(mps_motion_info *m, mps_comp_motion *cm, float frame, quat *data);
// @HACK:
void mp_default_capture_root_rot_func(multiped *mp,mps_motion_info *m, float frame, quat *data);

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

static int mp_motlist_num_writeable_motions(mps_motion_list *list,
                                             mps_motion_node **motion)
{
   mps_stack_node *node;

   for (node = list->head; node; node = node->next)
   {
      if (node->type == MN_MOTION &&
          MP_MOTNUM_FROM_NODE_HANDLE(((mps_motion_node *)node)->handle) !=
             MP_MOTNUM_FROM_NODE_HANDLE(-1))
      {
         if (motion)
            *motion = (mps_motion_node *)node;
         return 1;
      }
   }
   return 0;
}

// Retail saves use 32-bit pointer slots and byte packing. Pointer slots are
// placeholders on disk, never addresses in the current process. Older ports
// also wrote a byte-packed 64-bit variant or the native aligned structure.
#define MP_DISK_NODE_FIELDS(pointer_type) \
   pointer_type prev, next; \
   int32_t type; \
   float time; \
   int32_t handle, start_frame; \
   float frame; \
   struct { \
      uint32_t flags; \
      float duration_scalar, bend, stretch, vstretch, vinc; \
      struct { \
         pointer_type limb; \
         float start_approach_time, placement_time; \
         float placement_duration, trans_out_duration; \
         mxs_vector pos; \
         mxs_matrix orient; \
         uint8_t use_orient, absolute, stick; \
      } ep; \
      float fixed_duration; \
      pointer_type mirror; \
   } params; \
   mxs_vector base_pos; \
   quat base_orient; \
   mxs_vector offset; \
   pointer_type callback; \
   int32_t callback_num, callback_frame; \
   pointer_type targets; \
   int32_t flag_index

#pragma pack(push, 1)
typedef struct { MP_DISK_NODE_FIELDS(uint32_t); } sMotionDisk32;
typedef struct { MP_DISK_NODE_FIELDS(uint64_t); } sMotionDisk64;
#pragma pack(pop)
#undef MP_DISK_NODE_FIELDS

_Static_assert(sizeof(sMotionDisk32) == 191, "retail motion record layout");
_Static_assert(sizeof(sMotionDisk64) == 215, "packed 64-bit motion record layout");

// New records encode the effector limb as an index rather than an address.
#define MP_MOTLIST_INDEXED_LIMB UINT32_C(0x80000000)

// Copy values only. Links, callbacks, mirror functions, limbs and target buffers
// must be supplied by the current process, never copied from a saved address.
#define MP_COPY_MOTION_VALUES(dst, src) do { \
   (dst)->type = (src)->type; \
   (dst)->time = (src)->time; \
   (dst)->handle = (src)->handle; \
   (dst)->start_frame = (src)->start_frame; \
   (dst)->frame = (src)->frame; \
   (dst)->params.flags = (src)->params.flags; \
   (dst)->params.duration_scalar = (src)->params.duration_scalar; \
   (dst)->params.bend = (src)->params.bend; \
   (dst)->params.stretch = (src)->params.stretch; \
   (dst)->params.vstretch = (src)->params.vstretch; \
   (dst)->params.vinc = (src)->params.vinc; \
   (dst)->params.ep.start_approach_time = (src)->params.ep.start_approach_time; \
   (dst)->params.ep.placement_time = (src)->params.ep.placement_time; \
   (dst)->params.ep.placement_duration = (src)->params.ep.placement_duration; \
   (dst)->params.ep.trans_out_duration = (src)->params.ep.trans_out_duration; \
   (dst)->params.ep.pos = (src)->params.ep.pos; \
   (dst)->params.ep.orient = (src)->params.ep.orient; \
   (dst)->params.ep.use_orient = (src)->params.ep.use_orient; \
   (dst)->params.ep.absolute = (src)->params.ep.absolute; \
   (dst)->params.ep.stick = (src)->params.ep.stick; \
   (dst)->params.fixed_duration = (src)->params.fixed_duration; \
   (dst)->base_pos = (src)->base_pos; \
   (dst)->base_orient = (src)->base_orient; \
   (dst)->offset = (src)->offset; \
   (dst)->callback_num = (src)->callback_num; \
   (dst)->callback_frame = (src)->callback_frame; \
   (dst)->flag_index = (src)->flag_index; \
} while (0)

static int mp_get_write_motlist_bufsize(mps_motion_list *list)
{
   return 3 * sizeof(uint32_t) +
      mp_motlist_num_writeable_motions(list, NULL) * sizeof(sMotionDisk32);
}

static void mp_write_motlist(multiped *mp, uchar *buf)
{
   mps_motion_list *list = &mp->main_motion;
   uint32_t header[3];
   mps_motion_node *motion = NULL;
   int count = mp_motlist_num_writeable_motions(list, &motion);

   header[0] = list->signature;
   header[1] = list->flags;
   header[2] = count | MP_MOTLIST_INDEXED_LIMB;
   memcpy(buf, header, sizeof(header));
   if (count)
   {
      sMotionDisk32 disk = {0};
      MP_COPY_MOTION_VALUES(&disk, motion);
      // Zero means no limb; indices are stored plus one.
      if (motion->params.flags & MP_PLACE_EFFECTOR)
      {
         int i;
         for (i = 0; i < mp->num_limbs; ++i)
            if (motion->params.ep.l == &mp->limbs[i])
               disk.params.ep.limb = i + 1;
      }
      memcpy(buf + sizeof(header), &disk, sizeof(disk));
   }
}

static int mp_read_motlist(multiped *mp, const uchar *buf, size_t size,
                           int(*swizzle_func)(int, motion_callback *pcallback))
{
   uint32_t header[3];
   uint32_t count, limb_index = 0;
   mps_motion_node value = {0};
   mps_motion_node *motion;
   int motion_num;

   if (size < sizeof(header))
      return MP_READ_BAD_CONFIG;
   memcpy(header, buf, sizeof(header));
   count = header[2] & ~MP_MOTLIST_INDEXED_LIMB;
   size -= sizeof(header);
   buf += sizeof(header);
   if (count > 1 || (!count && size != 0))
      return MP_READ_BAD_CONFIG;
   if (count)
   {
      if (size == sizeof(sMotionDisk32))
      {
         sMotionDisk32 disk;
         memcpy(&disk, buf, sizeof(disk));
         MP_COPY_MOTION_VALUES(&value, &disk);
         if (header[2] & MP_MOTLIST_INDEXED_LIMB)
            limb_index = disk.params.ep.limb;
      }
      else if (!(header[2] & MP_MOTLIST_INDEXED_LIMB) && size == sizeof(sMotionDisk64))
      {
         sMotionDisk64 disk;
         memcpy(&disk, buf, sizeof(disk));
         MP_COPY_MOTION_VALUES(&value, &disk);
      }
      else if (!(header[2] & MP_MOTLIST_INDEXED_LIMB) && size == sizeof(mps_motion_node))
      {
         mps_motion_node disk;
         memcpy(&disk, buf, sizeof(disk));
         MP_COPY_MOTION_VALUES(&value, &disk);
      }
      else
         return MP_READ_BAD_CONFIG;

      // The old format never saved enough information to reconstruct mirror
      // functions or the limb referenced by an effector placement. Reject only
      // such motions, rather than restoring a dangling process pointer.
      if (value.type != MN_MOTION || (value.params.flags & MP_MIRROR) ||
          ((value.params.flags & MP_PLACE_EFFECTOR) &&
           (limb_index == 0 || limb_index > mp->num_limbs)))
         return MP_READ_BAD_MOTIONS;

      motion_num = MP_MOTNUM_FROM_NODE_HANDLE(value.handle);
      if (swizzle_func)
         motion_num = swizzle_func(motion_num, &value.callback);
      if (motion_num < 0 || motion_num >= mp_num_motions)
         return MP_READ_BAD_MOTIONS;
      value.handle = motion_num | (value.handle & (MP_KILL_FLAG | MP_END_FLAG));
      if (value.params.flags & MP_PLACE_EFFECTOR)
         value.params.ep.l = &mp->limbs[limb_index - 1];
   }

   mp_list_init(&mp->main_motion);
   mp->main_motion.signature = header[0];
   mp->main_motion.flags = header[1];
   if (!count)
      return MP_READ_OKAY;

   motion = (mps_motion_node *)mp_alloc(sizeof(*motion), __FILE__, __LINE__);
   if (!motion)
      return MP_READ_BAD_MOTIONS;
   *motion = value;
   mp_list_add(&mp->main_motion, (mps_stack_node *)motion);
   if (motion->params.flags & MP_PLACE_EFFECTOR)
   {
      const int handle = motion->handle;
      motion->handle = motion_num;
      mp_compute_ep(mp, motion);
      motion->handle = handle;
   }
   return MP_READ_OKAY;
}
#undef MP_COPY_MOTION_VALUES

int mp_read_multiped(multiped* mp, const void* buf, size_t size,
                     int(*swizzle_func)(int, motion_callback* pcallback))
{
   const uchar *curpos = (const uchar *)buf;
   int num_joints;
   mxs_vector xlat;
   quat rotations[MAX_JOINTS + 1];
   size_t prefix;

   if (size < sizeof(mxs_matrix) + sizeof(num_joints))
      return MP_READ_BAD_CONFIG;
   memcpy(&num_joints, curpos + sizeof(mxs_matrix), sizeof(num_joints));
   if (num_joints < 0 || num_joints > MAX_JOINTS || num_joints != mp->num_joints)
      return MP_READ_BAD_CONFIG;
   prefix = sizeof(mxs_matrix) + sizeof(num_joints) + num_joints +
      sizeof(quat) * (num_joints + 1);
   if (size < prefix)
      return MP_READ_BAD_CONFIG;

   memcpy(&mp->global_orient, curpos, sizeof(mxs_matrix));
   curpos += sizeof(mxs_matrix) + sizeof(num_joints);
   memcpy(mp->joint_map, curpos, num_joints);
   curpos += num_joints;
   memcpy(rotations, curpos, sizeof(quat) * (num_joints + 1));
   mx_zero_vec(&xlat);
   mp_apply_motion(mp, rotations, &xlat, -1);
   return mp_read_motlist(mp, (const uchar *)buf + prefix, size - prefix, swizzle_func);
}

int mp_get_write_buffsize(multiped* mp)
{
   return sizeof(mxs_matrix) + sizeof(int) + mp->num_joints +
      sizeof(quat) * (mp->num_joints + 1) +
      mp_get_write_motlist_bufsize(&mp->main_motion);
}

void mp_write_multiped(multiped* mp, void* buf)
{
   uchar *curpos = (uchar *)buf;

   memcpy(curpos, &mp->global_orient, sizeof(mxs_matrix));
   curpos += sizeof(mxs_matrix);
   memcpy(curpos, &mp->num_joints, sizeof(mp->num_joints));
   curpos += sizeof(mp->num_joints);
   memcpy(curpos, mp->joint_map, mp->num_joints);
   curpos += mp->num_joints;
   memcpy(curpos, mp->rel_orients, sizeof(quat) * (mp->num_joints + 1));
   curpos += sizeof(quat) * (mp->num_joints + 1);
   mp_write_motlist(mp, curpos);
}

void mp_get_multiped_motions(multiped* mp, int* mots, int* max_mots)
{
   mps_motion_node *motion = NULL;

   if (*max_mots <= 0)
      return;
   *max_mots = mp_motlist_num_writeable_motions(&mp->main_motion, &motion);
   if (*max_mots)
      *mots = MP_MOTNUM_FROM_NODE_HANDLE(motion->handle);
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
		m.info.num_frames = (float)mot.info.num_frames;
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
	m.info.num_frames = (float)num_frames;
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
		d = (float)MX_REAL_PI * i / 255.0f;
		*r = (1.0f - cosf(d)) / 2.0f;
	}

// Build signature lookup table. Coun bits in each signature from 0 to 31.
	memset(mp_num_joints_by_sig, 0, sizeof(int) * MAX_JOINTS);
	count = mp_num_joints_by_sig + 1;
	for (i = 1; i < MAX_JOINTS; i++, count++)
	{
		bits = 1 + (int)(log((double)i) / log(2.0));
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

void mp_default_capture_comp_xlat_func(multiped *mp, mps_motion_info *m, mps_comp_motion *cm, float frame, mxs_vector *data)
{
   mxs_vector *vec;
   int iframe = (int)frame;

   vec = (mxs_vector *) RefLock((Ref)cm->handle);
   mx_copy_vec(data, &vec[iframe]);
   RefUnlock((Ref)cm->handle);
}

//

void mp_default_capture_comp_rot_func(mps_motion_info *m, mps_comp_motion *cm, float frame, quat *data)
{
   quat *q;
   int iframe = (int)frame;

   q = (quat *) RefLock((Ref)cm->handle);
   quat_copy(data, &q[iframe]);
   RefUnlock((Ref)cm->handle);
}

//

void mp_default_capture_root_rot_func(multiped *mp, mps_motion_info *m, float frame, quat *data)
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

