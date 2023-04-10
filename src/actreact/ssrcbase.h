/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/actreact/ssrcbase.h,v 1.5 2000/01/29 12:44:43 adurant Exp $
#pragma once

#ifndef __SSRCBASE_H
#define __SSRCBASE_H

#include <ssrctype.h>
#include <stimtype.h>
#include <pg8rtype.h>

#include <matrixs.h>

////////////////////////////////////////////////////////////
// BASE TYPES FOR STIMULUS SOURCES
//

typedef struct sStimShape sStimShape;
typedef struct sStimLifeCycle sStimLifeCycle;

//
// Stimulus source shape
//
// The propagator is free to use any sort of structure 
// of this size to represent its spatial source parameters.

struct sStimShape
{
   uchar pad[32];
};

//
// Life cycle
//
// Propagators are free to use these bytes any way they see fit
// to represent its temporal source parameters. 
//

struct sStimLifeCycle
{
   uchar pad[32]; 
};

//
// Stimulus source descriptor
//


struct sStimSourceDesc 
{
   PropagatorID propagator;  // propagator to which source belongs
   tStimLevel level; // source's intensity
   ulong valid_fields; // bitmask of which fields below are valid
   sStimShape shape; // shape as interpreted by propagator
   sStimLifeCycle life; // temporal shape as interpreted by propagator, NULL means default
};

//
// Valid fields bits
//
enum eStimSourceDescValidFields
{
   kStimSrcNothingValid = 0,
   kStimSrcShapeValid   = 1 << 0,
   kStimSrcLifeValid    = 1 << 1,
};


//
// Some special birth dates
//

enum eSourceBirthDates
{
   kSourceBirthDateDead = 0xFFFFFFFE,
   kSourceBirthDateNone = 0xFFFFFFFF,
};


#endif // __SSRCBASE_H







