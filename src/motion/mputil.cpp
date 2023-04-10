/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Source: r:/t2repos/thief2/src/motion/mputil.cpp,v $
// $Author: mahk $
// $Date: 1998/10/05 17:26:37 $
// $Revision: 1.4 $
//
// Multiped utilities

#include <lg.h>

#include <mputil.h>

// Must be last header 
#include <dbmem.h>


void MpWriteMotionNode(mps_motion_node *node, MpFileFunc write, 
                       MpFile *file)
{
   write(file, &node->time, sizeof(node->time));
   write(file, &node->handle, sizeof(node->handle));
   write(file, &node->start_frame, sizeof(node->start_frame));
   write(file, &node->frame, sizeof(node->frame));
   write(file, &node->params, sizeof(node->params));
   write(file, &node->base_pos, sizeof(node->base_pos));
   write(file, &node->base_orient, sizeof(node->base_orient));
   write(file, &node->offset, sizeof(node->offset));
   if (node->callback)
      Warning(("MpWriteMotionList: callback on motion %d can't be saved\n",
               node->handle));
   write(file, &node->callback_num, sizeof(node->callback_num));
   write(file, &node->callback_frame, sizeof(node->callback_frame));
   write(file, node->targets, sizeof(*(node->targets)));
   write(file, &node->flag_index, sizeof(node->flag_index));
}

void MpWriteTransitionNode(mps_transition_node *node, MpFileFunc write, 
                       MpFile *file)
{
   write(file, &node->time, sizeof(node->time));
   write(file, &node->duration, sizeof(node->duration));
}

void MpWriteStackNode(mps_stack_node *node, MpFileFunc write, 
                       MpFile *file)
{
   write(file, &node->type, sizeof(node->type));
   if (node->type == MN_MOTION)
      MpWriteMotionNode((mps_motion_node*)node, write, file);
   else
      MpWriteTransitionNode((mps_transition_node*)node, write, file);
}

void MpWriteMotionList(mps_motion_list *list, MpFileFunc write, 
                       MpFile *file)
{
   mps_stack_node *stack;
   int i;
   
   if (list == NULL)
      return;
   write(file, &list->signature, sizeof(list->signature));
   write(file, &list->count, sizeof(list->count));
   stack = list->head;
   for (i=0; i<list->count; i++)
   {
      if (stack == NULL)
      {
         Warning(("MpWriteMotionList: bad motion list\n"));
         return;
      }
      MpWriteStackNode(stack, write, file);
      stack = stack->next;
   }
   write(file, &list->flags, sizeof(list->flags));
}

void MpWriteOverlay(mps_overlay *overlay, MpFileFunc write,
                    MpFile *file)
{
   if (overlay == NULL)
      return;
   write(file, &overlay->sig, sizeof(overlay->sig));
   MpWriteMotionList(&overlay->list, write, file);
   write(file, &overlay->mode, sizeof(overlay->mode));
   write(file, &overlay->time, sizeof(overlay->time));
   write(file, &overlay->duration, sizeof(overlay->duration));
}

void MpWriteMultiped(multiped *mp, MpFileFunc write, MpFile *file)
{
   int i;

   write(file, &mp->app_ptr, sizeof(mp->app_ptr));
   write(file, &mp->num_joints, sizeof(mp->num_joints));
   write(file, &mp->root_joint, sizeof(mp->root_joint));
   write(file, mp->joint_map, mp->num_joints);
   write(file, mp->rel_orients, mp->num_joints*sizeof(quat));
   MpWriteMotionList(&mp->main_motion, write, file);
   write(file, &mp->num_overlays, sizeof(mp->num_overlays));
   for (i=0; i<mp->num_overlays; i++)
      MpWriteOverlay(&mp->overlays[i], write, file);
   write(file, &mp->global_pos, sizeof(mp->global_pos));
   write(file, &mp->global_orient, sizeof(mp->global_orient));
   write(file, &mp->target_heading, sizeof(mp->target_heading));
   write(file, &mp->th_time, sizeof(mp->th_time));
   write(file, &mp->th_duration, sizeof(mp->th_duration));
}

void MpReadMotionNode(mps_motion_node *node, MpFileFunc read,
                      MpFile *file)
{
   read(file, &node->time, sizeof(node->time));
   read(file, &node->handle, sizeof(node->handle));
   read(file, &node->start_frame, sizeof(node->start_frame));
   read(file, &node->frame, sizeof(node->frame));
   read(file, &node->params, sizeof(node->params));
   read(file, &node->base_pos, sizeof(node->base_pos));
   read(file, &node->base_orient, sizeof(node->base_orient));
   read(file, &node->offset, sizeof(node->offset));
   read(file, &node->callback_num, sizeof(node->callback_num));
   read(file, &node->callback_frame, sizeof(node->callback_frame));
   read(file, node->targets, sizeof(*(node->targets)));
   read(file, &node->flag_index, sizeof(node->flag_index));
}

void MpReadTransitionNode(mps_transition_node *node, MpFileFunc read, 
                       MpFile *file)
{
   read(file, &node->time, sizeof(node->time));
   read(file, &node->duration, sizeof(node->duration));
}

void MpReadStackNode(mps_stack_node *node, MpFileFunc read,
                     MpFile *file)
{
   read(file, &node->type, sizeof(node->type));
   if (node->type == MN_MOTION)
      MpReadMotionNode((mps_motion_node*)node, read, file);
   else
      MpReadTransitionNode((mps_transition_node*)node, read, file);
}

void MpReadMotionList(mps_motion_list *list, MpFileFunc read, 
                      MpFile *file)
{
   int i;
   int count;
   mps_motion_node temp;
   mps_stack_node *node;

   read(file, &list->signature, sizeof(list->signature));
   read(file, &count, sizeof(list->count));
   for (i=0; i<list->count; i++)
   {
      MpReadStackNode((mps_stack_node*)&temp, read, file);
      if (temp.type == MN_MOTION)
      {
         node = (mps_stack_node*)mp_alloc(sizeof(mps_motion_node), 
                                          __FILE__, __LINE__);
         memcpy(node, &temp, sizeof(mps_motion_node));
      }
      else
      {
         node = (mps_stack_node*)mp_alloc(sizeof(mps_transition_node),
                                              __FILE__, __LINE__);
         memcpy(node, &temp, sizeof(mps_transition_node));
      }
      mp_list_add(list, node);
   }
   read(file, &list->flags, sizeof(list->flags));
}

void MpReadOverlay(mps_overlay *overlay, MpFileFunc read, MpFile *file)
{
   read(file, &overlay->sig, sizeof(overlay->sig));
   MpReadMotionList(&overlay->list, read, file);
   read(file, &overlay->mode, sizeof(overlay->mode));
   read(file, &overlay->time, sizeof(overlay->time));
   read(file, &overlay->duration, sizeof(overlay->duration));
}

void MpReadMultiped(multiped *mp, MpFileFunc read, MpFile *file)
{
   int i;

   //   mp_free_multiped(mp);
   read(file, &mp->app_ptr, sizeof(mp->app_ptr));
   read(file, &mp->num_joints, sizeof(mp->num_joints));
   read(file, &mp->root_joint, sizeof(mp->root_joint));
   mp->joint_map = (uchar*)mp_alloc(mp->num_joints, 
                                    __FILE__, __LINE__);
   read(file, mp->joint_map, mp->num_joints);
   mp->rel_orients = (quat*)mp_alloc(mp->num_joints*sizeof(quat),
                                     __FILE__, __LINE__);
   read(file, mp->rel_orients, mp->num_joints*sizeof(quat));
   MpReadMotionList(&mp->main_motion, read, file);
   read(file, &mp->num_overlays, sizeof(mp->num_overlays));
   for (i=0; i<mp->num_overlays; i++)
      MpReadOverlay(&mp->overlays[i], read, file);
   read(file, &mp->global_pos, sizeof(mp->global_pos));
   read(file, &mp->global_orient, sizeof(mp->global_orient));
   read(file, &mp->target_heading, sizeof(mp->target_heading));
   read(file, &mp->th_time, sizeof(mp->th_time));
   read(file, &mp->th_duration, sizeof(mp->th_duration));
}
