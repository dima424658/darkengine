//
// mpstack.c -- Functions for evaluating a multiped's motion stack.
//

#include <string.h>
#include <fix.h>
#include <math.h>
#include <multiped.h>

// NOTE: this should not be used to index into any arrays on multiped itself
#define ROOT_JOINT_ID(m_) ((m_)->num_joints)

//

void mp_stack_init(mps_motion_stack * stack)
{
   stack->index = 0;	
}

//

void mp_stack_push(mps_motion_stack * stack, mps_stack_node * node)
{
   stack->list[stack->index++] = node;
}

//

mps_stack_node * mp_stack_pop(mps_motion_stack * stack)
{
   mps_stack_node * result;
   if (stack->index)
   {
      stack->index--;
      result = stack->list[stack->index];
   }
   else
   {
      // Stack empty.
      result = NULL;
   }
   return result;
}

//

static mps_motion_stack stack;

//

void mp_evaluate_motions(multiped * mp, mps_motion_list * list, quat * rot, 
                         mxs_vector * xlat)
{
   mps_stack_node * node = NULL;

   // Build the motion stack.
   mp_stack_init(&stack);
   while (node = mp_list_traverse(list, node))
   {
      mp_stack_push(&stack, node);
   }

   //mp_dump_stack(&stack);
   mp_evaluate_stack(mp, rot, xlat);
}

//
// This is the top-level motion stack evaluator.
//
void mp_evaluate_stack(multiped * mp, quat * rot, mxs_vector * xlat)
{
   mps_stack_node * m;

   // DEBUG.
   //mp_dump_stack(&stack);

   m = mp_stack_pop(&stack);

   if (m->type == MN_MOTION)
   {
      // Motion (leaf node).
      mp_evaluate_motion(mp, (mps_motion_node *) m, rot, xlat);
   }
   else
   {
      // Transition.
      mp_evaluate_transition(mp, (mps_transition_node *) m, rot, xlat);
   }
}

//
// This deals with leaf nodes.
// 
void mp_evaluate_motion(multiped * mp, mps_motion_node * m, quat * rot, 
                        mxs_vector * xlat)
{
   int frame;
   float angle, t;
   quat q1, q2;
   mxs_matrix root_mat, mat;
   mxs_vector axis;
   mps_motion * motion;

   if (m->handle >= 0)
   {
      motion = mp_motion_list + m->handle;
	
      if (m->frame == -1)
      {
         // Motion is suspended, waiting for parent transition to end. Use last
         // frame's data.
         frame = motion->info.num_frames - 1;
      }
      else
      {
         frame = (int) floor(m->frame + 0.5);
      }
	
      // Go get the rotation and translational data of the specified motion.
      if (!mp_get_motion_data(mp, motion, frame, rot, xlat))
      {
         m->handle |= MP_END_FLAG;
      }

      if (m->params.flags & MP_MIRROR)
      {
         m->params.mirror(rot, xlat, motion->info.sig);
      }

      if (m->params.flags & MP_PLACE_EFFECTOR)
      {
         mp_place_effector(mp, m, rot);
      }

      // Apply current base rotation to initial motion orientation.
      quat_mul(&q1, &rot[ROOT_JOINT_ID(mp)], &m->base_orient);
      quat_copy(&rot[ROOT_JOINT_ID(mp)], &q1);

      // Get translation from motion start, in case we didn't start at the beginning.
      mx_subeq_vec(xlat, &m->offset);

      if (m->params.flags & MP_BEND)
      {
         if(m->start_frame>=motion->info.num_frames-1)
            t=1.0;
         else
            t = (float) (frame - m->start_frame) / (motion->info.num_frames - 1 - m->start_frame);
		
         angle = t * m->params.bend;
	
         mx_unit_vec(&axis, 2);
         quat_create(&q1, &axis, -2 * angle);

         // Bend the root orientation.
         quat_mul(&q2, &rot[ROOT_JOINT_ID(mp)], &q1);
         quat_copy(&rot[ROOT_JOINT_ID(mp)], &q2);

         // Bend the translation vector.
         mx_mk_rot_z_mat(&mat, mx_rad2ang(angle));
         mx_mat_muleq_vec(&mat, xlat);
      }

      // applying horizontal and vertical stretch
      if (m->params.flags & MP_STRETCH)
      {
         xlat->x *= m->params.stretch;
         xlat->y *= m->params.stretch;
      }
      if (m->params.flags & MP_VSTRETCH)
         xlat->z *= m->params.vstretch;
      if (m->params.flags & MP_VINC)
      {
         float frac;

         if(m->start_frame>=motion->info.num_frames-1)
            frac=1.0;
         else
            frac = (float) (frame - m->start_frame) / (motion->info.num_frames - 1 - m->start_frame);

         xlat->z += m->params.vinc*frac;
      }

      // Now that we have the translational offset, rotate it by the motion's
      // base orientation.
      quat_to_matrix(&root_mat, &m->base_orient);
      mx_mat_muleq_vec(&root_mat, xlat);

      mx_addeq_vec(xlat, &m->base_pos);
      mx_subeq_vec(xlat, &mp->global_pos);
   }
   else
   {
      // @HACK add extra "joint" for base/root rotation
      memcpy(rot, m->targets, sizeof(quat) * (mp->num_joints+1));
      // @HACK added mxs_vector for position, so that can be blended, too. (used to always be zero)
      mx_sub_vec(xlat,(mxs_vector *)(((quat *)(m->targets))+mp->num_joints+1),&mp->global_pos);
   }
}

//

static quat mp_qbuffer[2][MAX_JOINTS];
static mxs_vector mp_xbuffer[2];

//
// This deals (recursively) with transitions.
//
// WARNING: This function calls itself.
//
void mp_evaluate_transition(multiped * mp, mps_transition_node * trans, 
                            quat * rot, mxs_vector * xlat)
{
   mps_blend_info blend;
   mps_stack_node * m1, * m2;
   int trans_param;

   m1 = mp_stack_pop(&stack);
   m2 = mp_stack_pop(&stack);

   // m1 is guaranteed to be a leaf node.
   mp_evaluate_motion(mp, (mps_motion_node *) m1, &mp_qbuffer[1], &mp_xbuffer[1]);

   // m2 might be another transition.
   if (m2->type == MN_MOTION)
   {
      mp_evaluate_motion(mp, (mps_motion_node *) m2, &mp_qbuffer[0], &mp_xbuffer[0]);
   }
   else
   {
      mp_evaluate_transition(mp, (mps_transition_node *) m2, &mp_qbuffer[0], &mp_xbuffer[0]);
   }

   // Get time relative to duration as an integer from 0 to 255.
   if (trans->duration > 0)
   {
      trans_param = (int) floor(trans->time / trans->duration * 255.0);
   }
   else
   {
      trans_param = 255;
   }
	
   // Look up weight in ramp table.
   blend.param = mp_ramp_table[trans_param];
	
   // @HACK add "joint" for base/root rotation
   blend.num_rotations = mp->num_joints+1;
   blend.r1 = &mp_qbuffer[0];
   mx_copy_vec(&blend.t1, &mp_xbuffer[0]);
   blend.r2 = &mp_qbuffer[1];
   mx_copy_vec(&blend.t2, &mp_xbuffer[1]);
	
   blend.rot = rot;
   mp_blend_motions(&blend);
   mx_copy_vec(xlat, &blend.xlat);
}

//

void mp_blend_motions(mps_blend_info * b)
{
   int i;
   quat * r1 = b->r1;
   quat * r2 = b->r2;
   quat * dest = b->rot;
   mxs_vector vec;

   for (i = 0; i < b->num_rotations; i++, dest++, r1++, r2++)
   {
      quat_slerp(dest, r1, r2, b->param);
   }

   mx_scale_vec(&b->xlat, &b->t1, 1.0 - b->param);
   mx_scale_vec(&vec, &b->t2, b->param);
   mx_addeq_vec(&b->xlat, &vec);
}

//

void mp_dump_stack(mps_motion_stack * stack)
{
   int i;
   mps_stack_node * node;
   mps_motion_node * m;
   mps_transition_node * t;

   Spew(MP_SRC, ("\nDumping motion stack:\n"));
   for (i = 0; i < stack->index; i++)
   {
      node = stack->list[i];
      switch (node->type)
      {
         case MN_MOTION:
            m = (mps_motion_node *) node;
            Spew(MP_SRC, ("motion node: %d\n", m->handle));
            break;
         case MN_TRANSITION:
            t = (mps_transition_node *) node;
            Spew(MP_SRC, ("transition node\n"));
            break;
         default:
            Spew(MP_SRC, ("ERROR: Unknown stack node type.\n"));
            break;
      }
   }

   Spew(MP_SRC, ("Stack dump done.\n\n"));
}

//

void mp_place_effector(multiped * mp, mps_motion_node * m, quat * rot)
{
   // Use target rotations. The first time this is called to compute
   // the target rotations, so we obviously don't want to use the
   // targets then. 
   if (m->targets)
   {
      int i;
      mps_effector_place * ep = &m->params.ep;
      quat * q = m->targets, dest;
      float t;

      if (m->time >= ep->start_approach_time)
      {
         if (m->time <= ep->placement_time)
         {
            // approaching effector target.
            float t;
            int delta = ep->placement_time - ep->start_approach_time;
            i = (m->time - ep->start_approach_time) / (float) delta * 255.0;
            t = mp_ramp_table[i];

            for (i = 0; i < ep->l->num_segments; i++, q++)
            {
               quat_slerp(&dest, &rot[ep->l->joint_id[i]], q, t);
               quat_copy(&rot[ep->l->joint_id[i]], &dest);
            }
         }
         else
         {
            // Reached effector target.
            if ((m->time - ep->placement_time) < ep->placement_duration)
            {
               if (ep->stick)
               {
                  // Use last frame's joint positions and orientations. Not 
                  // perfect, but should work fine at reasonably high frame
                  // rates.
                  quat r[4];

                  mp_solve_limb(mp, ep, r);
                  q = r;
                  for (i = 0; i < ep->l->num_segments; i++, q++)
                  {
                     quat_copy(&rot[ep->l->joint_id[i]], q);
                  }

                  /*
                    // Interpolate between start and end placement rotations.

                    i = (m->time - ep->placement_time) / (float) ep->placement_duration * 255.0;
                    t = mp_ramp_table[i];

                    for (i = 0; i < ep->l->num_segments; i++, q++)
                    {
                    quat_slerp(&dest, q, q + ep->l->num_segments, t);
                    quat_copy(&rot[ep->l->joint_id[i]], &dest);
                    }
                    */
               }
               else
               {
                  // Use targets for placement duration.
                  for (i = 0; i < ep->l->num_segments; i++, q++)
                  {
                     quat_copy(&rot[ep->l->joint_id[i]], q);
                  }
               }
            }
            else
            {
               // Placement duration has expired. Transition out.
               int out = ep->placement_time + ep->placement_duration;
               if (m->time <= (out + ep->trans_out_duration))
               {
                  i = (m->time - out) / ep->trans_out_duration * 255.0;
                  t = mp_ramp_table[i];

                  if (ep->stick)
                  {
                     // Use end placement rotations rather than start.
                     q += ep->l->num_segments;
                  }

                  for (i = 0; i < ep->l->num_segments; i++, q++)
                  {
                     quat_slerp(&dest, q, &rot[ep->l->joint_id[i]], t);
                     quat_copy(&rot[ep->l->joint_id[i]], &dest);
                  }
               }
            }
         }
      }
   }
}

//

