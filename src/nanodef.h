/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#pragma once
#ifndef __NANODEF_H
#define __NANODEF_H

// Noun Types
#define kNounExternal   0
#define kNounPersonal   1
#define kNounWeapon     2
#define kNounRobot      3

// Typedefs for Nano structs
typedef struct
{
   int nounType;
   int durability;
   int costH2X;
   char image[16];
   int data[2];
} NanoNoun;

typedef struct
{
   float costMult;
   float effectMult;
   char image[16];
} NanoAdj;

typedef struct
{
   bool active;
   int noun;
   int adj[5];
   int numAdj;
} NanoCombo;

#endif // __NANODEF_H
