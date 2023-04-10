#ifndef __CFG_H
#define __CFG_H
/*
 * $Source: x:/prj/tech/libsrc/config/RCS/cfg.h $
 * $Revision: 1.1 $
 * $Author: mahk $
 * $Date: 1995/11/27 03:19:55 $
 *
 */
#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

///////////////////////////////////////////////////
// CFG.H
//
// A simpler config system API for small systems.
//

//--------------------------------------
// config_load()
// config_save();
//
// Loads or save all config varables in 
// the specified filename.
// 
errtype config_load(char* filename);
#define config_save(filename) config_write_file(fn,NULL);

//--------------------------------------
// config_startup()
//
// Initializes the config system.  
// if the specified filename is not NULL, 
// config_load's that file.
//
errtype config_startup(char* filename);




#ifdef __cplusplus
}
#endif // __cplusplus
#endif // __CFG_H

