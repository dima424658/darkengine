/*
 * $Source: x:/prj/tech/libsrc/dump/RCS/dump.h $
 * $Revision: 1.4 $
 * $Author: PATMAC $
 * $Date: 1998/07/01 18:03:46 $
 *
 * Screen Dumping library include file
 *
*/

#ifndef __DUMP_H
#define __DUMP_H
#ifdef __cplusplus
extern "C" {
#endif

/* Dumps the current screen to the file, pointed to by the file
 * pointer fp.  You need to give a temporary buffer *buf,
 * pointing to at least 26k of memory.   Allocate it as
 * you will.
*/

// return -1 if unsuccessful
int dmp_gif_dump_screen(int fp,unsigned char *buf);

// needs no buffer, writes out 8 bit pcx or 24 bit
// return -1 if unsuccessful
int dmp_pcx_dump_screen(int fp);

// needs no buffer, writes out 8 bit or 24 bit bmp
// return -1 if unsuccessful
int dmp_bmp_dump_screen(int fp);


/* Finds the free file in a sequence like
 * "<prefix>000.<suffix>", tries to open it,
 * and returns you a pointer to it.
 * maximum prefix is 5 letters long.
 * maximum suffix is 3 letters long
 * numbers the files in oct.
 * Returns -1 if unable to open
*/
int dmp_find_free_file(char *buff,char *prefix,char *suffix);

// Finds a free file of prefix, "pcx" suffix and tries to write.
// returns -1 if unsuccessful
// copies name into buff if successful
int dmp_pcx_file(char *buff,char *prefix);

// Finds a free file of prefix, "bmp" suffix and tries to write.
// returns -1 if unsuccessful
// copies name into buff if successful
int dmp_bmp_file(char *buff,char *prefix);

#ifdef __cplusplus
}
#endif
#endif
