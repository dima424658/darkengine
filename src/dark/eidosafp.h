/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

//-------------------------------------
//
//   Eidos Copy Protection 
//
//   Contact : lee.briggs@eidos.co.uk
//						 +44(0)181 6363440
//
//   Date    : 18 September 1998
//   Target  : Generic
//
//-------------------------------------
#pragma once

#ifndef _INCLUDE_EIDOS_AFP_
#define _INCLUDE_EIDOS_AFP_

//-------------------------------------

#ifdef __cplusplus
extern "C" {
#endif

//--- mode ---

#define AFP_DISABLED               0
#define AFP_ACTIVE_PARTIAL         1
#define AFP_ACTIVE_FULL            2

#ifdef FULL_COPY_PROTECTION
#define AFP_MODE                   AFP_ACTIVE_FULL
#else
#define AFP_MODE                   AFP_ACTIVE_PARTIAL
#endif

#define AFP_FINAL_DATA_TRACK_TIME  76  // 72 to test, 76 for real

//--- marker file details ---

#include  "AFPFiles.h"

//--- Globals ---

extern char  AFP_FilenameA[];
extern char  AFP_FilenameB[];
extern char  AFP_FilenameC[];
extern char  AFP_FilenameD[];

//--- Simple macro to check the contents of the file ---

#if  AFP_MODE == AFP_DISABLED

	#define AFP_CHECK(offset, filename, marker, result)			\
	{																												\
		*result = 1;																					\
	}

#endif
#if AFP_MODE == AFP_ACTIVE_PARTIAL

	#define AFP_CHECK(offset, filename, marker, result)			\
	{																								        \
		FILE  *fp = fopen(filename, "rb");						        \
																							        		\
		if (!fp) {															        			\
																									        \
			*result = 0;																        \
																									        \
		} else {																			        \
																									        \
			if (fgetc(fp) == marker)										        \
				*result = 1;															        \
			else																				        \
				*result = 0;															        \
																									        \
			fclose (fp);																        \
		}																							        \
	}

#endif
#if AFP_MODE == AFP_ACTIVE_FULL

	#define AFP_CHECK(offset, filename, marker, result)			   \
	{																								           \
		FILE  *fp = fopen(filename, "rb");						           \
																							        		   \
		if (!fp) {															        			   \
																									           \
			*result = 0;																           \
																									           \
		} else {																			           \
																													   \
			fseek(																								 \
				fp, 																								 \
				offset + ((AFP_FINAL_DATA_TRACK_TIME - 76) * (2048 * 4500)),  \
				SEEK_SET);								        									 \
																									           \
			if (fgetc(fp) == marker)										           \
				*result = 1;															           \
			else																				           \
				*result = 0;															           \
																									           \
			fclose (fp);																           \
		}																							           \
	}

#endif


//--- macros to read the individual files ---

#define AFP_CHECK_A(result) \
	AFP_CHECK(AFP_OFFSET_A, AFP_FilenameA, AFP_MARKER_A, result)

#define AFP_CHECK_B(result) \
	AFP_CHECK(AFP_OFFSET_B, AFP_FilenameB, AFP_MARKER_B, result)

#define AFP_CHECK_C(result) \
	AFP_CHECK(AFP_OFFSET_C, AFP_FilenameC, AFP_MARKER_C, result)

#define AFP_CHECK_D(result) \
	AFP_CHECK(AFP_OFFSET_D, AFP_FilenameD, AFP_MARKER_D, result)

//-------------------------------------

int  AFP_Initialise();
char AFP_QueryDrive();

//-------------------------------------

#ifdef __cplusplus
};
#endif

//-------------------------------------

#endif
