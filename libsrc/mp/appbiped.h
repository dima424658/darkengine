#ifndef APPBIPED_H
#define APPBIPED_H

//copied from:
// $Header: r:/prj/cam/src/RCS/abpstruc.h 1.3 1997/01/02 19:33:31 JON Exp $

//#ifndef __abpstruc_h
//#define __abpstruc_h


#include <multiped.h>
#include <skel.h>

#define NUM_JOINTS      20
#define NUM_NORMALS     9
#define NUM_SEGMENTS    15
// Note: the following two quantities must correspond to the 
// number of torsos/limbs that the motion processor uses
#define NUM_TORSOS      2
#define NUM_LIMBS       5

typedef struct BipappCostume
{
	ushort head;
	ushort body;
	ushort lhand;
	ushort rhand;
	ushort bodpal;
	ushort headpal;
} BipappCostume;

//typedef ushort objID
//#include <obj.h>

typedef struct
{
	multiped m;
	sks_skel skel;
	sks_seginfo sg[NUM_SEGMENTS];    // seginfo arrays required for sks_skel
	mxs_vector joints[NUM_JOINTS];   // joints and normals required for both
	mxs_vector norm[NUM_NORMALS];    // sks_skel and multiped
	mxs_matrix orients[NUM_JOINTS];
	torso torsos[NUM_TORSOS];
	limb limbs[NUM_LIMBS];
} appbiped;

#define LTOE      0
#define RTOE      1
#define LANKLE    2
#define RANKLE    3
#define LKNEE     4
#define RKNEE     5
#define LHIP      6
#define RHIP      7
#define BUTT      8
#define NECK      9
#define LSHLDR   10
#define RSHLDR   11
#define LELBOW   12
#define RELBOW   13
#define LWRIST   14
#define RWRIST   15
#define LFINGER  16
#define RFINGER  17
#define ABDOMEN  18
#define HEAD     19
#define LSHLDRIN 20
#define RSHLDRIN 21
#define LWEAP    22
#define RWEAP    23

#define SEG_LFOOT     0
#define SEG_RFOOT     1
#define SEG_LCALF     2
#define SEG_RCALF     3
#define SEG_LTHIGH    4
#define SEG_RTHIGH    5
#define SEG_BUTT      6
#define SEG_AB        7
#define SEG_HEAD      8
#define SEG_LBICPT    9
#define SEG_RBICPT   10
#define SEG_LFORE    11
#define SEG_RFORE    12
#define SEG_LHAND    13
#define SEG_RHAND    14

#define NORM_PELV    0
#define NORM_RLEG    1
#define NORM_RARM    2
#define NORM_LLEG    3
#define NORM_LARM    4
#define NORM_RHAND   5
#define NORM_LHAND   6
#define NORM_UPTORS  7
#define NORM_HEAD    8

//#endif



#endif	//APPBIPED_H
