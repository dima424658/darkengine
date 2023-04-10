/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/sampprop.h,v 1.2 2000/01/29 12:41:45 adurant Exp $
#pragma once

#ifndef SAMPPROP_H
#define SAMPPROP_H
#include <property.h>

//
// @INSTRUCTIONS: Rename this file to fooprop.h, change the #ifndef's at the
// top appropriately.
//


//
// @INSTRUCTIONS: PROPERTY INTERFACE
//                 Decide if your property needs a new interface. 
//                 If it doesn't (and most shouldn't), delete the
//                 Property instance declarations below, and skip
//                 To the "instance" stuff below.  
//                  
//                
// @INSTRUCTIONS: For clarity, delete instructions after you follow them. 
//
 
//------------------------------------------------------------
// PROPERTY INTERFACE 
//
//

// @INSTRUCTIONS: Replace "Sample" and "SAMPLE" with an appropriate name for 
//              your property interface, and "SampType" with the 
//              name of your property's value type.  Change all
//              

F_DECLARE_INTERFACE(ISampleProperty);
#undef INTERFACE
#define INTERFACE ISampleProperty


//
// @INSTRUCTIONS: If your interface is a "complex" interface,
//     change DELCARE_SIMPLE_INTERFACE to DECLARE_COMPLEX_INTERFACE
//

DECLARE_SIMPLE_PROPERTY(SampType,Sample);
DECLARE_CREATOR(Sample);

// @INSTRUCTIONS: Grab a GUID for your interface and put it in 
//                propguids.c

//------------------------------------------------------------
// PROPERTY INSTANCE
//
// 

// @Instructions replace "Sample" with an appropriate unique
// name for your property.  Change ISampleProperty to the 
// interface your property uses (whether it's a new interface
// or an old one), and cSampleProperty to the corresponding class.

#define PROP_SAMPLE_NAME "Sample"
// @INSTRUCTIONS: put a call to this in initialization code somewhere.
EXTERN void SamplePropInit(void);




#endif // SAMPPROP_H

