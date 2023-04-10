/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkovcst.h,v 1.26 2000/01/31 09:58:16 adurant Exp $
#pragma once

#ifndef __SHKOVCST_H
#define __SHKOVCST_H

// shock overlay system constants

// Individual overlays
#define kOverlayInv        0
#define kOverlayFrame      1  
#define kOverlayText       2
#define kOverlayRep        3
#define kOverlayBook       4
#define kOverlayComm       5
#define kOverlayContainer  6
#define kOverlayHRM        7
#define kOverlayRadar      8
#define kOverlayLetterbox  9
#define kOverlayOverload   10
#define kOverlayPsi        11
#define kOverlayYorN       12
#define kOverlayKeypad     13
#define kOverlayLook       14
#define kOverlayAmmo       15
#define kOverlayMeters     16
#define kOverlayHUD        17
#define kOverlayStats      18
#define kOverlaySkills     19
#define kOverlayBuyTraits  20
#define kOverlaySetting    21    
#define kOverlayCrosshair  22    
#define kOverlayResearch   23
#define kOverlayPDA        24
#define kOverlayEmail      25
#define kOverlayMap        26
#define kOverlayAlarm      27
#define kOverlayPsiIcons   28
#define kOverlayHackIcon   29
#define kOverlayRadiation  30
#define kOverlayPoison     31
#define kOverlayMiniFrame  32
#define kOverlaySecurity   33
#define kOverlayTicker     34
#define kOverlayBuyStats   35
#define kOverlayBuyTech    36
#define kOverlayBuyWeapon  37
#define kOverlayBuyPsi     38
#define kOverlayTechSkill  39
#define kOverlayMFDGame    40
#define kOverlayTlucText   41
#define kOverlaySecurComp  42
#define kOverlayHackComp   43
#define kOverlayHRMPlug    44
#define kOverlayMiniMap    45
#define kOverlayElevator   46
#define kOverlayVersion    47
#define kOverlayTurret     48
#define kOverlayMouseMode  49
#define kNumOverlays       50


// Change modes
#define kOverlayModeOff    0
#define kOverlayModeOn     1
#define kOverlayModeToggle 2

// Overlay flags
#define kOverlayFlagNone               0x0000
#define kOverlayFlagAlwaysDraw         0x0001
#define kOverlayFlagAlwaysMouse        0x0002
#define kOverlayFlagModal              0x0004
#define kOverlayFlagTranslucent        0x0008
#define kOverlayFlagBufferTranslucent  0x0010

#define DEFAULT_MSG_TIME   5000

#endif