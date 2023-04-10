/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/dark/eidosafp.c,v 1.2 2000/02/19 13:08:59 toml Exp $

#include <stdio.h>
#include <windows.h>

#include <eidosafp.h>
#include <drkafp.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

//-------------------------------------
// The marker files

char  AFP_FilenameA[] = AFP_FILENAME_A;
char  AFP_FilenameB[] = AFP_FILENAME_B;
char  AFP_FilenameC[] = AFP_FILENAME_C;
char  AFP_FilenameD[] = AFP_FILENAME_D;

#if 0
//-------------------------------------
int EIDOS_RAW_AFP_Initialise()
{
	char  drive;
	char  rootPath[] = "a:\\";
	UINT  type;

	for (drive = 'A'; drive <= 'Z'; drive++) {

		rootPath[0] = drive;

		type = GetDriveType(rootPath);		

		if (type == DRIVE_CDROM) {

			int  result;


			AFP_FilenameA[0] = drive;
			AFP_FilenameB[0] = drive;
			AFP_FilenameC[0] = drive;
			AFP_FilenameD[0] = drive;

			AFP_CHECK_A(&result);

			if (result) return 1;
		}
	}
	return 0;
}
#endif

////////////////////////
// go do the AFP craziness

BOOL DarkEngine_AFP_Check(char drive_letter)
{
   BOOL a_ok=TRUE;
#ifdef COPY_PROTECTION
   int result;
   
   AFP_FilenameA[0] = drive_letter;
   AFP_FilenameB[0] = drive_letter;
   AFP_FilenameC[0] = drive_letter;
   AFP_FilenameD[0] = drive_letter;
   
   AFP_CHECK_A(&result);
   a_ok = a_ok && (result!=0);
   // ummmm, shouldnt i check b,c,and d too???
   AFP_CHECK_B(&result);
   a_ok = a_ok && (result!=0);
   AFP_CHECK_C(&result);
   a_ok = a_ok && (result!=0);   
   AFP_CHECK_D(&result);
   a_ok = a_ok && (result!=0);
#endif
   return a_ok;
}

char AFP_QueryDrive()
{
   return (AFP_FilenameA[0]);
}


//-------------------------------------

#define  AFP_VERSION   0x00010001

static int AFP_FindMe[32] = {

	0x8202f232, 0x240bbcc3, 0x4327ad99, 0x74739de4,
	0x2432fa99, 0xff34299d, 0xde329074, 0x43da7892,
	0x23482a21, 0xbe330078, 0x7e932231, 0x532de0ff,
	0xe93a993a, 0x3250923a, 0x430cb88a, 0xf93922a8,
	0xf392a998, 0x4273abbc, 0x234898e8, 0x3ffe8956,
	0x43598eef, 0x234989aa, 0x48888222, 0x344121ef,

	AFP_VERSION,
	AFP_FINAL_DATA_TRACK_TIME, 
	0, 
	0,

	0, 0, 0, 0
};

