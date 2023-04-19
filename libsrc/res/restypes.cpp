//      ResTypes.C      Resource type names
//      Rex E. Bradford (REX)
//
//      See the doc RESOURCE.DOC for information.
/*
* $Header: x:/prj/tech/libsrc/res/RCS/restypes.cpp 1.14 1996/09/14 16:10:59 TOML Exp $
* $log: $
*/

#include "res.h"

//  Resource type names

const char *resTypeNames[NUM_RESTYPENAMES] = {
   "UNKNOWN",                                    // RTYPE_UNKNOWN    (aka BIN)
   "STRING",                                     // RTYPE_STRING
   "IMAGE",                                      // RTYPE_IMAGE      (aka IMG)
   "FONT",                                       // RTYPE_FONT
   "ANIM",                                       // RTYPE_ANIM
   "PALL",                                       // RTYPE_PALL
   "SHADTAB",                                    // RTYPE_SHADTAB
   "VOC",                                        // RTYPE_VOC
   "SHAPE",                                      // RTYPE_SHAPE
   "PICT",                                       // RTYPE_PICT
   "B2EXTERN",                                   // RTYPE_B2EXTERN
   "B2RELOC",                                    // RTYPE_B2RELOC
   "B2CODE",                                     // RTYPE_B2CODE
   "B2HEADER",                                   // RTYPE_B2HEADER
   "hey!",                                       // RTYPE_B2RESRVD
   "OBJ3D",                                      // RTYPE_OBJ3D
   "STENCIL",                                    // RTYPE_STENCIL
   "MOVIE",                                      // RTYPE_MOVIE
   "RECT",                                       // RTYPE_RECT
   "PAL565",                                     // RTYPE_PAL565
   "",                                           // 20
   "",                                           // 21
   "",                                           // 22
   "",                                           // 23
   "",                                           // 24
   "",                                           // 25
   "",                                           // 26
   "",                                           // 27
   "",                                           // 28
   "",                                           // 29
   "",                                           // 30
   "",                                           // 31
   "",                                           // 32
   "",                                           // 33
   "",                                           // 34
   "",                                           // 35
   "",                                           // 36
   "",                                           // 37
   "",                                           // 38
   "",                                           // 39
   "",                                           // 40
   "",                                           // 41
   "",                                           // 42
   "",                                           // 43
   "",                                           // 44
   "",                                           // 45
   "",                                           // 46
   "",                                           // 47
   "APP1",                                       // RTYPE_APP
   "APP2",
   "APP3",
   "APP4",
   "APP5",
   "APP6",
   "APP7",
   "APP8",
   "APP9",
   "APP10",
   "APP11",
   "APP12",
   "APP13",
   "APP14",
   "APP15",
   "APP16",
};
