/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/pfaceiid.h,v 1.42 2000/02/18 14:19:33 MAT Exp $
#pragma once

//
// GUIDS for property interfaces
//

#ifndef __PFACEIID_H
#define __PFACEIID_H

DEFINE_LG_GUID (IID_IBoolProperty, 0x77);
DEFINE_LG_GUID (IID_IVectorProperty, 0x79);
DEFINE_LG_GUID (IID_IIntProperty, 0x7b);
DEFINE_LG_GUID (IID_ILabelProperty, 0x7c); 
DEFINE_LG_GUID (IID_IFloatProperty, 0x142);
DEFINE_LG_GUID (IID_IStringProperty, 0x159);
DEFINE_LG_GUID (IID_IInvStringProperty, 0x15a);

DEFINE_LG_GUID (IID_IBrushProperty, 0x7d);
DEFINE_LG_GUID (IID_IGlobalIDProperty, 0x7f);

DEFINE_LG_GUID(IID_IBipedLengthsProperty, 0x47);
DEFINE_LG_GUID(IID_IBipedProperty, 0x48);
DEFINE_LG_GUID(IID_IBipedModelProperty, 0x4a);
DEFINE_LG_GUID(IID_ICreatureProperty, 0xbf);
DEFINE_LG_GUID(IID_ICreaturePoseProperty,0x1a4);

//DEFINE_LG_GUID(IID_IInvLabelProperty,0x91); // Not in use
DEFINE_LG_GUID(IID_IScaleProperty,0x92);

DEFINE_LG_GUID(IID_IAIProperty, 0x93);
DEFINE_LG_GUID(IID_IAIReservedProperty, 0x94);

#if 0
DEFINE_LG_GUID(IID_IAIRangedProperty, 0x170);
DEFINE_LG_GUID(IID_IAIMarkerBiasesProperty, 0x171);
#endif

DEFINE_LG_GUID(IID_IVantagePtProperty, 0x19b);
DEFINE_LG_GUID(IID_ICoverPtProperty, 0x19c);
DEFINE_LG_GUID(IID_IAngleLimitProperty, 0x19d);
DEFINE_LG_GUID(IID_IRangedCombatProperty, 0x19e);

DEFINE_LG_GUID(IID_ISchemaPlayParamsProperty, 0x96);
DEFINE_LG_GUID(IID_ISchemaLoopParamsProperty, 0x98);
DEFINE_LG_GUID(IID_ISchemaTagConstraintProperty, 0x9b);

DEFINE_LG_GUID(IID_ISpeechProperty, 0x99);
DEFINE_LG_GUID(IID_IAmbientSoundProperty, 0xa1);

DEFINE_LG_GUID(IID_IPhysTypeProperty, 0x178); 
DEFINE_LG_GUID(IID_IPhysAttrProperty, 0x179);
DEFINE_LG_GUID(IID_IPhysStateProperty, 0x7e);
DEFINE_LG_GUID(IID_IPhysControlProperty, 0x17a);
DEFINE_LG_GUID(IID_IPhysDimsProperty, 0x17b);
DEFINE_LG_GUID(IID_IPhysRopeProperty, 0x185);
DEFINE_LG_GUID(IID_IPhysExplodeProperty, 0x2d5);
DEFINE_LG_GUID(IID_IPhysPPlateProperty, 0x188);

DEFINE_LG_GUID(IID_IMovingTerrainProperty, 0x12e);

DEFINE_LG_GUID(IID_IRotDoorProperty, 0xde);
DEFINE_LG_GUID(IID_ITransDoorProperty, 0xe2);

DEFINE_LG_GUID(IID_IBashParamsProperty, 0x18a);

DEFINE_LG_GUID(IID_IFrobInfoProperty, 0x54);
DEFINE_LG_GUID(IID_IHeatDiskProperty, 0xad);
DEFINE_LG_GUID(IID_ISparkProperty, 0xb2);
DEFINE_LG_GUID(IID_IMeshAttachProperty, 0x137);
DEFINE_LG_GUID(IID_IExtraLightProperty, 0x14a);
DEFINE_LG_GUID(IID_IParticleGroupProperty, 0x158);
DEFINE_LG_GUID(IID_IPGroupLaunchInfoProperty, 0x1f2);
DEFINE_LG_GUID(IID_IFrameAnimationStateProperty, 0x165);
DEFINE_LG_GUID(IID_IFrameAnimationConfigProperty, 0x166);
DEFINE_LG_GUID(IID_IRendFlashProperty, 0x169);
DEFINE_LG_GUID(IID_IColorProperty, 0x180);

// light sources which affect raycast lighting
DEFINE_LG_GUID(IID_ILightProperty, 0xb8);
DEFINE_LG_GUID(IID_IAnimLightProperty, 0xb9);

DEFINE_LG_GUID(IID_IAcousticsProperty, 0xcc);
DEFINE_LG_GUID(IID_IAmbientProperty, 0xc8);
DEFINE_LG_GUID(IID_IAutomapProperty, 0xc9);
DEFINE_LG_GUID(IID_IRoomScriptProperty, 0xca);
DEFINE_LG_GUID(IID_IRoomGravityProperty, 0x143);

DEFINE_LG_GUID(IID_ITweqSimpleProperty, 0x51);
DEFINE_LG_GUID(IID_ITweqSimpleStateProperty, 0x52);
DEFINE_LG_GUID(IID_ITweqVectorProperty, 0x55);
DEFINE_LG_GUID(IID_ITweqVectorStateProperty, 0x4e);
DEFINE_LG_GUID(IID_ITweqJointsProperty, 0x53);
DEFINE_LG_GUID(IID_ITweqJointsStateProperty, 0x75);
DEFINE_LG_GUID(IID_ITweqModelsProperty, 0x56);
DEFINE_LG_GUID(IID_ITweqEmitterProperty, 0x59);
DEFINE_LG_GUID(IID_ITweqLockProperty, 0x17f);
DEFINE_LG_GUID(IID_ITweqLockStateProperty, 0x181);

DEFINE_LG_GUID(IID_IJointPosProperty, 0x80);
DEFINE_LG_GUID(IID_IBitmapWorldspaceProperty, 0x430);
DEFINE_LG_GUID(IID_IScriptProperty, 0xe1);

DEFINE_LG_GUID(IID_IKeyInfoProperty, 0xf5);

DEFINE_LG_GUID(IID_IPuppetProperty, 0xfa);

DEFINE_LG_GUID(IID_IPickCfgProperty, 0x112);
DEFINE_LG_GUID(IID_IPickStateProperty, 0x118);

DEFINE_LG_GUID(IID_IAdvPickTransCfgProperty, 0x3a2);
DEFINE_LG_GUID(IID_IAdvPickSoundCfgProperty, 0x3a3);
DEFINE_LG_GUID(IID_IAdvPickStateCfgProperty, 0x3a4);

DEFINE_LG_GUID(IID_IBeltLinkProperty, 0x383);
DEFINE_LG_GUID(IID_IAltLinkProperty, 0x384);


DEFINE_LG_GUID(IID_IMotPhysLimitsProperty, 0x131);
DEFINE_LG_GUID(IID_IMotGaitDescProperty, 0x136);
DEFINE_LG_GUID(IID_IMotActorTagListProperty, 0x138);
DEFINE_LG_GUID(IID_IMotPlayerLimbOffsetsProperty, 0x15b);

DEFINE_LG_GUID(IID_ISuspiciousProperty, 0x415);

#endif  // __PFACEIID_H
