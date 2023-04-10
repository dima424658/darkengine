/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/framewrk/imgsrc.h,v 1.3 1998/09/18 19:48:49 mahk Exp $
#pragma once  
#ifndef __IMGSRC_H
#define __IMGSRC_H

#include <resapi.h>

//
// "Image source" helper interface 
//
// A simple abstract wrapper around a bitmap that lets us get it when we need it. 
//

#define IID_IImageSource IID_IDataSource

typedef IDataSource IImageSource; 

enum eBitImageSrcFlags
{
   kBitmapSrcCopyBits = 1 << 0,  // delete the bits when you go away 
};
// Some standard image source factories
EXTERN IImageSource* CreateBitmapImageSource(struct grs_bitmap * s, ulong flags);
EXTERN IImageSource* CreateResourceImageSource(const char* path, const char* resname); 

// Uses the old resource system 
EXTERN IImageSource* CreateRefImageSource(ulong ref); 


#endif // __IMGSRC_H





