// $Header: x:/prj/tech/libsrc/mp/RCS/mpupdate.c 1.23 1998/06/21 15:13:57 kate Exp $

//
// mpupdate.c -- Multiped animation/update stuff.
//

// @HACK: a multiped now uses "root" to mean two different things.
// it has an mp->root_joint (usually the butt), which is an id used
// to offset into the joints array, and who's position is the
// same as the multiped's position.  It also has a root accessed by
// ROOT_JOINT_ID(mp) which returns the index to use for the relative
// rotation stuff that exists when evaluating/applying a motion.  The
// rotation then gets stuffed into mp_global_orient.  This root joint
// isn't a joint as such.  It's just a construct for computing 
// the global orientation of the multiped.  I know this is lame and
// confusing.  It's an intermediate step in separating out butt locations
// and global locations.            (KATE 5/16/98)


#include <math.h>
#include <string.h>
#include <multiped.h>
#include <mprintf.h>

// NOTE: this should not be used to index into any arrays on multiped itself
#define ROOT_JOINT_ID(m_) ((m_)->num_joints)


// DEBUG

extern char * Jnt[];

//
// this is defined in mpupdate.c
extern mps_component_xlat_callback_func mp_capture_component_xlat_func;
extern mps_component_rot_callback_func mp_capture_component_rot_func;
extern mps_root_rot_callback_func mp_capture_root_rot_func;
extern mps_rot_filter_callback mp_rot_filter_callback;

//
//

quat mp_rot[4][MAX_JOINTS];
static quat mp_net_rot[MAX_JOINTS];
static mxs_vector net_xlat;
ulong g_lAppUpdateFlags=NULL;

//

void mp_make_callbacks(multiped *mp, mps_motion_node *m,float time_slop,ulong flags)
{
   if (m->callback)
   {
      if(m->handle==-1) // XXX how to make this less secret/stupid?
         m->callback(mp, m->handle, m->callback_frame,time_slop,g_lAppUpdateFlags|flags);
      else
         m->callback(mp, (m->handle)&~MP_KILL_FLAG, m->callback_frame,time_slop,g_lAppUpdateFlags|flags);
      m->callback = NULL;
   }
   if (m->callback_num)
   {
      int i;

      for (i=0; i<MP_NUM_CALLBACKS; i++)
      {
         if (m->callback_num&(1<<i))
            if (mp_callbacks[i])
               mp_callbacks[i](mp, m->handle, m->callback_frame,time_slop,g_lAppUpdateFlags);
      }
      m->callback_num = 0;
   }
}

void mp_initial_update(multiped * mp)
{
   int i;
   quat * r = mp_net_rot;
   mxs_vector v;
	
   for (i = 0; i < mp->num_joints+1; i++, r++)
   {
      quat_identity(r);
   }

   mx_zero_vec(&v);

   mp_apply_motion(mp, mp_net_rot, &v, -1);
}

//

void mp_dump_joint_map(multiped * mp)
{
   int i;
   uchar * map = mp->joint_map;

   Spew(MP_SRC, ("Joint map:\n"));
   for (i = 0; i < mp->num_joints; i++, map++)
   {
      Spew(MP_SRC, ("%X ", *map));
   }

   Spew(MP_SRC, ("\n"));
}

/*
void mp_analyze_map(multiped * mp)
{
	int i, * map;
	int num[5];

	memset(num, 0, 5 * sizeof(int));

	map = mp_joint_map;
	for (i = 0; i < mp->num_joints; i++, map++)
	{
		num[(*map) + 1]++;
	}

	for (i = 0; i < 5; i++)
	{
		if (num[i])
		{
			Spew(MP_SRC, ("%d in list %d\n", num[i], i));
		}
	}
}
*/
//

uint mp_update(multiped * mp, float dt,ulong app_flags)
{
   int i, trans_param;
   mxs_vector dummy;
   mps_overlay * ov;
   quat * dst;
   float blend;
   bool any_active;
   uint result = 0;

   g_lAppUpdateFlags=app_flags;

   // Go through each motion list, incrementing frames, doing callbacks, updating
   // transition times, etc.
   // Then build and evaluate motion stacks.
   if (mp_update_list(mp, NULL, &mp->main_motion, dt, &result))
   {
      mp_evaluate_motions(mp, &mp->main_motion, &mp_rot[0], &net_xlat);
      any_active = TRUE;
   }
   else
   {
      mx_zero_vec(&net_xlat);
      any_active = FALSE;
   }

   ov = mp->overlays;
   for (i = 0; i < mp->num_overlays; i++, ov++)
   {
      if (mp_list_count(&ov->list))
      {
         if (mp_update_list(mp, ov, &ov->list, dt, &result))
         {
            mp_evaluate_motions(mp, &ov->list, &mp_rot[i+1], &dummy);
            any_active = TRUE;

            // Update overlay transitions.
            Spew(MP_SRC, ("ov->mode: %d frame: %g\n", ov->mode, ((mps_motion_node *) ov->list.head)->frame));
            if (ov->mode)
            {
               ov->time += dt;
               if (ov->time > ov->duration)
               {
                  if (ov->mode == -1)
                  {
                     ov->mode = 0;
                     Spew(MP_SRC, ("Transition into overlay done.\n"));
                  }
                  else if (ov->mode == 1)
                  {
                     uint sig;
                     int j, idx;
                     uchar * map;

                     sig = ov->sig;
                     idx = (mp_list_count(&mp->main_motion)) ? 0 : 0xff;

                     // Remove overlay.
                     mp_list_free(&ov->list);

                     // Slide everything down.
                     memcpy(ov, ov+1, sizeof(mps_overlay) * (MAX_OVERLAY_MOTIONS - i - 1));

                     // Back up indices, since everything has moved down 
                     // one.
                     i--;
                     ov--;
                     Spew(MP_SRC, ("removing overlay\n"));
                     mp->num_overlays--;

                     // Update joint map.
                     map = mp->joint_map;
                     for (j = 0; j < mp->num_joints; j++, map++)
                     {
                        if (QuerySigJoint(sig, j))
                        {
                           *map = idx;
                        }
                     }
                  }
               }
            }
         }
         else
         {
            // List just became empty. Slide everything down and back up 
            // indices.
            uint sig;
            int j, idx;
            uchar * map;

            sig = ov->sig;
            idx = (mp_list_count(&mp->main_motion)) ? 0 : 0xff;

            memcpy(ov, ov+1, sizeof(mps_overlay) * (MAX_OVERLAY_MOTIONS - i - 1));
            i--;
            ov--;
            mp->num_overlays--;

            // Update joint map.
            map = mp->joint_map;
            for (j = 0; j < mp->num_joints; j++)
            {
               if (QuerySigJoint(sig, j))
               {
                  *map = idx;
               }
            }
         }
      }
   }

   // Now we've evaluated the whole-body motion and each of the overlay motions.
   // Build the final net motion list by combining the overlays with the main
   // motion.

   // @TODO: set root orientation!

   if (any_active)
   {
      //	mp_compute_joint_map(mp, mp_joint_map);

      uchar * map;	

      dst = mp_net_rot;
      map = mp->joint_map;

      // set root rotation to that of main motion, since overlays not allowed
      // to affect it
      quat_copy(&mp_net_rot[ROOT_JOINT_ID(mp)],&mp_rot[0][ROOT_JOINT_ID(mp)]);

      for (i = 0; i < mp->num_joints; i++, dst++, map++)
      {
         // joint_map says which overlay is responsible for each joint.
         if (*map == 0)
         {
            // Not in overlays, use main motion if there is one.
            if(mp->main_motion.count)
               quat_copy(dst, &mp_rot[0][i]);
         }
         else if (*map != 0xff)
         {
            ov = mp->overlays + *map - 1;
            if (ov->mode)
            {
               // Get time relative to duration as an integer from 0 to 255.
               if (ov->duration > 0)
               {
                  trans_param = (int) floor(ov->time / ov->duration * 255.0);
               }
               else
               {
                  trans_param = 255;
               }

               blend = mp_ramp_table[trans_param];
               if (ov->mode < 0)
               {
                  // transitioning into overlay.
                  quat_slerp(dst, &mp_rot[0][i], &mp_rot[*map][i], blend);
               }
               else
               {
                  // transitioning out of overlay.
                  quat_slerp(dst, &mp_rot[0][i], &mp_rot[*map][i], 1.0 - blend);
               }
            }
            else
            {
               // normally playing overlay.
               quat_copy(dst, &mp_rot[*map][i]);
            }
         }
      }
      //mp_analyze_map(mp);

      if (mp->th_duration >= 0)
      {
         mp_change_heading(mp, dt);
      }

      mp_apply_motion(mp, mp_net_rot, &net_xlat, -1);
   }

   mp_dump_joint_map(mp);

   g_lAppUpdateFlags=NULL;

   return result;
}

//
// Returns FALSE if list is empty after update, TRUE if not.
//
BOOL mp_update_list(multiped * mp, mps_overlay * ov, mps_motion_list * list, float dt, uint * flags)
{
   float frame_inc;
   mps_motion * motion;
   mps_motion_node * m;
   mps_transition_node * t;
   mps_stack_node * node = NULL, * temp, * temp2;
   bool motion_over;

   if (list->flags)
   {
      // Motions are flagged to be killed.
      mp_kill_motions(mp,list);
      list->flags = 0;
   }

   while (node = mp_list_traverse(list, node))
   {
      if (node->type == MN_MOTION)
      {
         motion_over = FALSE;

         m = (mps_motion_node *) node;
         if (m->handle & MP_END_FLAG)
         {
            motion_over = TRUE;
            m->handle &= ~MP_END_FLAG;
         }

         motion = mp_motion_list + m->handle;

         // Check for suspended motion.
         if (m->frame != -1)
         {
            // If we could assume all motions come in at the same sample
            // frequency, we could do this computation once per frame instead of
            // once per motion per frame.
            if (m->params.flags & MP_DURATION_SCALE)
            {
               dt /= m->params.duration_scalar;
            }

            m->time += dt;

            frame_inc = ((float) motion->info.freq) * dt / (float) 1000;
            m->frame += frame_inc;

            if (m->flag_index >= 0)
            {
               // Check flags between last frame and new frame.
               int new_frame;
               mps_motion_flag * f	= motion->flags + m->flag_index;

               new_frame = (int) floor(m->frame + 0.5);

               while ((f->frame <= new_frame) && (m->flag_index < motion->num_flags))
               {
                  *flags |= f->flags;

                  f++;
                  m->flag_index++;
               }
            }


            // Check for callback. This should always catch it, no matter how
            // far we've gone past the end of the motion.
            if ((m->callback_frame > 0) && 
                (m->callback_frame <= m->frame))
            {
               if(motion->info.type==MT_CAPTURE)
               {
                  float actualFreq=((float) motion->info.freq);
      
                  if (m->params.flags & MP_DURATION_SCALE)
                  {
                     actualFreq /= m->params.duration_scalar;
                  }
                  mp_make_callbacks(mp, m, (m->frame-(float)m->callback_frame)/actualFreq,NULL);
               } else
               {
                  mp_make_callbacks(mp, m, 0, NULL);
               }
            }
            // See if motion is over.
            if (motion->info.num_frames > 0) 
            {
               // Fixed number of frames. See if we've passed the last frame.
               // We're going to round m->frame to the nearest integer, so if
               // there are 10 frames and m->frame is 9.5, we'll overstep the
               // end of the motion.
               if (m->frame >= (motion->info.num_frames - 0.5))
               {
                  motion_over = TRUE;
               }
            }

            if (!motion_over && (m->params.flags & MP_FIXED_DURATION))
            {
               if (m->time > m->params.fixed_duration)
               {
                  motion_over = TRUE;
               }
            }

            if (motion_over)
            {
               // This does "motion over" callbacks when the callback frame is
               // set to -1.
               if (m->callback_frame == -1)
               {
                  if(motion->info.type==MT_CAPTURE)
                  {
                     float actualFreq=((float) motion->info.freq);
      
                     if (m->params.flags & MP_DURATION_SCALE)
                     {
                        actualFreq /= m->params.duration_scalar;
                     }

                     mp_make_callbacks(mp, m, (m->frame-(motion->info.num_frames-0.5))/actualFreq, NULL);
                  } else
                  {
                     mp_make_callbacks(mp, m, 0, NULL);
                  }
               }

               // A motion that's followed by anything else in the list must be
               // part of a transition.
               if (node->next)
               {
                  Spew(MP_SRC, ("motion %d ending while part of transition. Suspending\n", m->handle));
                  // When the frame is set to -1, update will continue
                  // using the last frame of the motion.
                  m->frame = -1;
               }
               else
               {
                  // Motion is over.
                  if (ov)
                  {
                     Spew(MP_SRC, ("Overlay wants to end. Starting transition out.\n"));
                     ov->mode = 1;
                     ov->time = 0;
                     m->frame = -1;
                  }
                  else
                  {
                     Spew(MP_SRC, ("Motion %d over, removing.\n", m->handle));
                     temp = node->prev;
                     mp_list_remove(list, node);
                     node = temp;

                     // Update joint map.
                     if (mp_list_count(list) == 0)
                     {
                        int i;
                        uchar * map = mp->joint_map;
                        for (i = 0; i < mp->num_joints; i++, map++)
                        {
                           if (*map == 0)
                           {
                              *map = 0xff;
                           }
                        }
                     }
                  }
               }
            }
         }
      }
      else
      {
         t = (mps_transition_node *) node;
         t->time += dt;
         if (t->time > t->duration)
         {
            // Transition ends. Remove transition node and old motion
            // node.

            Spew(MP_SRC, ("Transition over, removing.\n"));
			
            // Delete everything that precedes the previous motion.
            temp = node->prev->prev;
            while (temp)
            {
               temp2 = temp->prev;
               mp_list_remove(list, temp);
               temp = temp2;
            }

            // Delete the transition that just ended.
            temp = node->prev;
            mp_list_remove(list, node);
            node = temp;
         }
      }
   }

   return (list->head != NULL);
}

//
// Here's where we actually set the joint positions and orientations.
//

static quat Orients[MAX_JOINTS];

void mp_apply_motion(multiped * mp, quat * rot, mxs_vector * trans, int stop_joint)
{
   int i, j;
   torso * t = mp->torsos;
   limb * l = mp->limbs;
   mxs_vector v1, *jp;
   mxs_matrix m, * mptr;
   quat * rp, * op;

   // We'll only convert quaternions to matrices when we need to rotate a vector.
   // Matrix mul costs 27 multiples, quaternion mul is only 16. Also, rotating a 
   // vector directly with a quaternion costs 24 multiplies, while multiplying a
   // matrix times a vector is only 9. Converting quat to matrix is 12, so it's 
   // still cheaper to convert to matrix and rotate vector than to rotate vector
   // directly with quat. Of course with FPU maybe counting multiplies is not the
   // answer.

   // set global position and orientation
   mx_addeq_vec(&mp->global_pos, trans);
   mx_copy_vec(&mp->joints[mp->root_joint],&mp->global_pos);

   // compute root orientation
   quat_to_matrix(&mp->global_orient, &rot[ROOT_JOINT_ID(mp)]);
   quat_copy(&Orients[ROOT_JOINT_ID(mp)],&rot[ROOT_JOINT_ID(mp)]);

   for (i = 0; i < mp->num_torsos; i++, t++)
   {
      //
      // Set the main joint's position and orientation. 
      //
      rp = &rot[t->joint];
      jp = &mp->joints[t->joint];

      op = &Orients[t->joint];

      if (t->parent == -1) // is possible to have more than one torso like this
      {
         quat_mul(op, rp, &Orients[ROOT_JOINT_ID(mp)]);
      }
      else
      {
         quat_mul(op, rp, &Orients[mp->torsos[t->parent].joint]);
      }

      if (t->joint == stop_joint)
      {
         goto mp_finished;
      }

      //
      // Set any fixed joints on the torso.
      //
      quat_to_matrix(&m, op);
      for (j = 0; j < t->num_fixed_points; j++)
      {
         mx_mat_mul_vec(&v1, &m, &t->pts[j]);
         mx_add_vec(&mp->joints[t->joint_id[j]], jp, &v1);

         if (t->joint_id[j] == stop_joint)
         {
            goto mp_finished;
         }
      }
   }

   for (i = 0; i < mp->num_limbs; i++, l++)
   {
      // First segment is relative to torso, 2nd relative to 1st, etc.
      for (j = 0; j < l->num_segments; j++)
      {
         rp = &rot[l->joint_id[j]];
         op = &Orients[l->joint_id[j]];

         if (j == 0)
         {
            quat_mul(op, rp, &Orients[mp->torsos[l->torso_id].joint]);
         }
         else
         {
            quat_mul(op, rp, &Orients[l->joint_id[j-1]]);
         }
		
         mx_scale_vec(&v1, &l->seg[j], l->seg_len[j]);

         quat_to_matrix(&m, op);
         mx_mat_muleq_vec(&m, &v1);

         mx_add_vec(&mp->joints[l->joint_id[j+1]], &mp->joints[l->joint_id[j]], &v1);

         if (l->joint_id[j] == stop_joint)
         {
            goto mp_finished;
         }
      }
   }

mp_finished:

   // Save relative orientations for later use.

   // @HACK add extra "joint" for base/root orient
   memcpy(mp->rel_orients, rot, sizeof(quat) * (mp->num_joints+1));
   // this is done so if object gets moved, xlat blending will still work
   mx_copy_vec(&mp->rel_xlat,&mp->global_pos); 

   // Now go through and convert quaternions to matrices.
   rp = Orients;
   mptr = mp->orients;
   for (i = 0; i < mp->num_joints; i++, rp++, mptr++)
   {
      quat_to_matrix(mptr, rp);
   }
}

//

#define ROOT_JOINT_ID(m_) ((m_)->num_joints)

//

bool mp_get_motion_data(multiped * mp, 
                        mps_motion * m, 
                        int frame, 
                        quat * rot, 
                        mxs_vector * trans)
{
   int i;
   mps_comp_motion * cm;
   bool result;

   switch (m->info.type)
   {
      case MT_CAPTURE:
         // Copy the capture data for the relevant frame.
         cm = m->components;

         // follow convention that first component has xlat
         mp_capture_component_xlat_func(mp,&m->info,cm,frame,trans);
         // compute root orientation
         mp_capture_root_rot_func(mp,&m->info,frame,&rot[ROOT_JOINT_ID(mp)]);

         cm++;
         for (i = 1; i < m->num_components; i++, cm++)
         {
            mp_capture_component_rot_func(&m->info,cm,frame,&rot[cm->joint_id]);
         }


         result = TRUE;
         break;

      case MT_VIRTUAL:
         result = m->virtual_update(mp, &m->info, frame, rot, trans);
         if (!result)
         {
            Spew(MP_SRC, ("Virtual update returned false. Flagging motion\n"));
         }
         break;
   }
   if(mp_rot_filter_callback)
   {
      mp_rot_filter_callback(mp,&m->info,rot,trans);
   }

   return result;
}

//
// Deal with heading change.
//
void mp_change_heading(multiped * mp, float dt)
{
   quat q1, q2;
   float t;
   mps_motion_node * mm = mp_get_main_motion(mp);

   mp->th_time += dt;
   if (mp->th_time < mp->th_duration)
   {
      int ramp_index = (int) floor(mp->th_time / mp->th_duration * 255.0);
      t = mp_ramp_table[ramp_index];
      quat_scale(&q1, &mp->target_heading, t);
      quat_mul(&q2, &mp_net_rot[ROOT_JOINT_ID(mp)], &q1);
      quat_copy(&mp_net_rot[ROOT_JOINT_ID(mp)], &q2);
   }
   else
   {
      quat_mul(&q1, &mp_net_rot[ROOT_JOINT_ID(mp)], &mp->target_heading);
      quat_copy(&mp_net_rot[ROOT_JOINT_ID(mp)], &q1);
      quat_copy(&mm->base_orient, &q1);
      mp->th_duration = -1;
   }
}

//

void mp_kill_motions(multiped *mp,mps_motion_list * list)
{
   mps_stack_node * node = NULL;

   while (node = mp_list_traverse(list, node))
   {
      if (node->type == MN_MOTION)
      {
         mps_motion_node * m = (mps_motion_node *) node;
         if (m->handle & MP_KILL_FLAG)
         {
            mps_stack_node * temp;
            // Kill motion. If there's anything after this motion in the list,
            // it's part of a transition, so remove the transition node too.
            if (node->next)
            {
               mps_stack_node * tn;

               if (node->next->type == MN_MOTION)
               {
                  // Motion is first in transition pair. Transition node is
                  // node->next->next;
                  tn = node->next->next;
                  temp = node->prev;
               }
               else 
               {
                  // Motion is second in transition pair. Transition node is
                  // node->next;
                  tn = node->next;
                  // if first motion in transition pair is suspended, delete
                  // that, too, since it is waiting for transition to end.
                  temp = node->prev;
                  AssertMsg(temp->type==MN_MOTION,"mp_kill_motions: bad motion list");
                  if(((mps_motion_node *)temp)->frame==-1)
                  {
                     mps_stack_node *prev_mot=temp;

                     temp=temp->prev;
                     mp_list_remove(list,prev_mot);
                  }
               }

               mp_list_remove(list, tn);
            }
            else
            {
               temp = NULL;
            }
            mp_make_callbacks(mp,(mps_motion_node *)node,0,MP_CFLAG_MOT_ABORTED); // since motion aborted
            mp_list_remove(list, node);
            node = temp;
         }
      }
   }
}

//
/*
  void mp_compute_joint_map(multiped * mp, int * map)
  {
  int i, j;
  mps_overlay * o = mp->overlays;

  memset(map, 0xff, sizeof(int) * mp->num_joints);
  for (i = 0; i < mp->num_overlays; i++, o++)
  {
  for (j = 0; j < mp->num_joints; j++)
  {
  if (QuerySigJoint(o->sig, j))
  {
  map[j] = i;
  }
  }
  }
  }
  */
//


