/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

////////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/motion/crjoint.h,v 1.3 2000/01/29 13:22:04 adurant Exp $
//
// Creature joint accessor header
//
#pragma once

#ifndef __CRJOINT_H
#define __CRJOINT_H

#include <objtype.h>
#include <matrixs.h>


// List of joints you may request
enum eCreatureJoints
{
   kCJ_Invalid, 
   kCJ_Head,
   kCJ_Neck,
   kCJ_Abdomen,
   kCJ_Butt,

   kCJ_LShoulder,
   kCJ_RShoulder,
   kCJ_LElbow,
   kCJ_RElbow,
   kCJ_LWrist,
   kCJ_RWrist,
   kCJ_LFingers,
   kCJ_RFingers,

   kCJ_LHip,
   kCJ_RHip,
   kCJ_LKnee,
   kCJ_RKnee,
   kCJ_LAnkle,
   kCJ_RAnkle,
   kCJ_LToe,
   kCJ_RToe,
   kCJ_Tail,

   kCJ_NumCreatureJoints,
};

EXTERN const char *g_pJointNames[];

EXTERN int  GetCreatureJointID(ObjID creature, int joint);

#ifdef __cplusplus
EXTERN const mxs_vector &GetCreatureJointPos(ObjID creature, int joint);
EXTERN const mxs_matrix &GetCreatureJointOrient(ObjID creature, int joint);
#endif

EXTERN void GetCreatureJointPosC(ObjID creature, int joint, mxs_vector *vec);
EXTERN void GetCreatureJointOrientC(ObjID creature, int joint, mxs_matrix *mat);

#endif
