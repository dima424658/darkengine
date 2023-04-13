/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/framewrk/dbfile.cpp,v 2.38 2000/03/07 19:57:07 toml Exp $

#include <lg.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include <config.h>

#include <dispbase.h>
#include <dbasemsg.h>
#include <dbfile.h>
#include <dbtagfil.h>
#include <tagfile.h>
#include <vernum.h>
#include <filetool.h>
#include <status.h>
#include <command.h>
#include <editsave.h>  // for secret load variables
#include <appsfx.h>
#include <cfgtool.h>

#include <osysbase.h>
#include <objspace.h>

#include <mprintf.h>

#include <filemenu.h>

#include <comtools.h>
#include <wappapi.h>
#include <loopapi.h>
#include <appagg.h>
#include <winui.h>

#include <simtime.h>
#include <simman.h>

#include <btffact.h>

// must be last header
#include <dbmem.h>

#define LOAD_FILE_VAR "world"
#define SAVE_FILE_VAR "world"
#define LOAD_PATH_VAR "load_path"

static char gCurrGameFile[256] = "";
static char gCurrMapFile[256] = "";

static char gCurrFilename[256]; // so people who want to query can find out

// We count the files that get loaded, from the reset on up. This is mainly
// needed for networking, to have an easy way to synchronize which objects
// were loaded from which file:
static int gCurrFilenum = 0;

// @TODO: when maps split off from missions, change this
#define MISSION_FILE_TYPE (kFiletypeMIS | kFiletypeMAP)

struct sFiletypeExtension
{  
   edbFiletype Filetype;
   char ext[4];
};

// Table of file types and their extensions

static sFiletypeExtension file_type_extensions[] = 
{
   { kFiletypeMIS|kFiletypeMAP, "MIS"},
   { kFiletypeMIS, "MIS"},
   { kFiletypeAll, "COW"},
   { kFiletypeGAM, "GAM"},
   { kFiletypeMAP, "MAP"},
   { 0, "COW"} // terminator, default
};

////////////////////////////////////////////////////////////

void reset_objid_space()
{
   AutoAppIPtr(ObjIDManager);
   sObjBounds bounds = { gMinObjID, gMaxObjID }; 

   config_get_int("obj_min",&bounds.min);
   config_get_int("obj_max",&bounds.max);

   pObjIDManager->ResizeObjIDSpace(bounds); 
}

////////////////////////////////////////////////////////////
// DATABASE FILE OPS/COMMANDS
//


static void munge_path(char* fullname, char* filename)
{
   char base[256] = ".";
   char name[256];
   attachPostfix(name,filename,"." DB_MISSION_SUFFIX);
   config_get_raw(DB_FILE_DIR_VAR,base,sizeof(base));
   if (is_full_path(name))
      strcpy(fullname,name);
   else
      diskPathMunge(fullname,base,NULL,name);
}

static BOOL find_for_load(char* fullname, char* filename)
{
   if (find_file_in_config_path(fullname,filename,DB_FILE_DIR_VAR))
      return TRUE;
   if (find_file_in_config_path(fullname,filename,LOAD_PATH_VAR))
      return TRUE;
   return FALSE;
}

BOOL dbFind(const char* filename, char* fullname)
{
   char buf[256];
   if (fullname == NULL) fullname = buf; 
   return find_for_load(fullname,(char*)filename); 
}

void dbCurrentFile(char *buf, int len)
{
   strncpy(buf, gCurrFilename, len);
}

static const char* strip_pathname(const char* filename)
{
   const char* fn = filename;
   // skip ':' and '\' and '/'
   static char skip_these[] = ":/\\";
   for (char* skip = skip_these; *skip != '\0'; skip++)
   {
      const char* found = strrchr(fn,*skip);
      if (found != NULL) fn = found+1;
   }
   return fn;
}

void dbSetCurrentFile(char *buf)
{
   strncpy(gCurrFilename, (char *)strip_pathname(buf),sizeof(gCurrFilename));
   gCurrFilename[sizeof(gCurrFilename)-1] = '\0'; 
}
//----------------------------------------

static void set_file_var(char* var, char* filename, bool save)
{
   char name[256];
   attachPostfix(name,filename,"." DB_FILE_SUFFIX);
   config_set_string(var,name);
   if (save)
      config_set_priority(var,CONFIG_DFT_LO_PRI);   
   else
      config_set_priority(var,PRIORITY_TRANSIENT);
}

//----------------------------------------

static edbFiletype guess_Filetype_from_extension(const char* filename)
{
   const char* dot = strrchr(filename,'.');
   if (dot != NULL)
   {
      dot++;
      // Search through the associative array of file types/extensions
      sFiletypeExtension* fte = file_type_extensions;
      for (; fte->Filetype != 0; fte++)
         if (stricmp(dot,fte->ext) == 0) // does extension match
            return fte->Filetype;
   }   
   return 0;
}



static const char* extension_for_filetype(edbFiletype ftype)
{
   sFiletypeExtension* fte = file_type_extensions;
   for (; fte->Filetype != 0; fte++)
      if (fte->Filetype == ftype) // does extension match
         break;
   return fte->ext;
}


// Try every likely extension
static edbFiletype find_for_load_extensionless(char* fullname, char* filename)
{
   sFiletypeExtension* fte = file_type_extensions;
   for (; fte->Filetype != 0; fte++)
   {
      char filebuf[256];
      sprintf(filebuf,"%s.%s",filename,fte->ext);
      if (find_for_load(fullname,filebuf)) 
         return fte->Filetype;
   }
   return 0;
}




//----------------------------------------

static void set_gamesys_file(char* fn)
{
   while(isspace(*fn)) fn++;
   strcpy(gCurrGameFile, strip_pathname(fn));
   StatusField(SF_GAMESYSNAME, gCurrGameFile);
}

////////////////////////////////////////////////////////////
// MISSION HEADER
//
// contains pointers to .MAP and .GAM files 
// (outdated, present for back-compatibility)
//

TagFileTag mhTag = { "MIS_HEADER" };
TagVersion mhVersion = { 0, 1,};

typedef struct
{
   char mapName[256];      // Full path names
   char gamesysName[256];
} MissionHeader;



//------------------------------------------------------------------
// Read the header to get the .MAP and .GAM filenames.
//------------------------------------------------------------------
BOOL ReadMissionHeader(ITagFile *file, char *mapName, char *gamName)
{
   TagVersion v = mhVersion;
   MissionHeader mh;

   if (FAILED(ITagFile_OpenBlock(file, &mhTag, &v)))
      return FALSE;
   ITagFile_Read(file,(char *)&mh, sizeof(mh));
   strcpy(mapName, mh.mapName);
   strcpy(gamName, mh.gamesysName);
   ITagFile_CloseBlock(file);
   return TRUE;
}

////////////////////////////////////////////////////////////
// FILE TYPE TAG 
//
// Contains the mask of what file types we contain
//

static TagFileTag dbTypeTag = { "FILE_TYPE" };
static TagVersion dbTypeVersion = { 0, 1 };

static BOOL write_file_type(ITagFile* file, edbFiletype Filetype)
{
   TagVersion v = dbTypeVersion;
   if (FAILED(file->OpenBlock(&dbTypeTag,&v)))
      return FALSE;
   file->Write((char*)&Filetype,sizeof(Filetype));
   file->CloseBlock();
   return TRUE;
}

static BOOL read_file_type(ITagFile* file, edbFiletype* Filetype)
{
   TagVersion v = dbTypeVersion;
   if (FAILED(file->OpenBlock(&dbTypeTag,&v)))
      return FALSE;
   BOOL retval = file->Read((char*)Filetype,sizeof(*Filetype)) == sizeof(Filetype);
   file->CloseBlock();
   return retval;
}

////////////////////////////////////////////////////////////
// FILE POINTER TAGS
//

struct sFNameTag
{
   ulong Filetype;
   TagFileTag tag;
};

const TagVersion FileNameVersion = { 0, 1};

const sFNameTag FileNameTags[] = 
{
   { kFiletypeMIS, { "MIS_FILE" }},
   { kFiletypeMAP, { "MAP_FILE" }},
   { kFiletypeGAM, { "GAM_FILE"}},
   { 0},
};

static const TagFileTag* FileNameTag(ulong ftype)
{
   for (int i = 0; FileNameTags[i].Filetype != 0; i++)
      if (FileNameTags[i].Filetype == ftype)
         return &FileNameTags[i].tag;
   return NULL;
}

#define FILEBUF_SIZE 256

static BOOL write_file_ref_tag(ITagFile* file, ulong ftype, const char* filename)
{
   const TagFileTag* tag = FileNameTag(ftype);
   if (tag == NULL) return FALSE;

   TagVersion v = FileNameVersion;
   if (FAILED(file->OpenBlock(tag,&v)))
      return FALSE;

   char buf[FILEBUF_SIZE];
   strncpy(buf,strip_pathname(filename),sizeof(buf));
   buf[sizeof(buf)-1] = '\0';
      
   file->Write(buf,sizeof(buf));
   file->CloseBlock();
   return TRUE;
}

static BOOL read_file_ref_tag(ITagFile* file, ulong ftype, char* filename)
{ 
   const TagFileTag* tag = FileNameTag(ftype);
   if (tag == NULL) return FALSE;

   TagVersion v = FileNameVersion;
   if (FAILED(file->OpenBlock(tag,&v)))
      return FALSE;

   char buf[FILEBUF_SIZE];
   BOOL success = file->Read(buf,sizeof(buf)) == sizeof(buf);
   buf[sizeof(buf)-1] = '\0';

   strcpy(filename,buf);
   file->CloseBlock();
   return success;
}


////////////////////////////////////////////////////////////
// FILE FACTORY 
// 

static cDBFileFactory* file_factory = NULL; 

ITagFile* dbOpenFile(const char* filename, TagFileOpenMode mode)
{
   if (file_factory)
      return file_factory->Open(filename,mode);
   else
      return BufTagFileOpen(filename,mode); 
}



cDBFileFactory* dbSetFileFactory(cDBFileFactory* new_factory)
{
   cDBFileFactory* retval = file_factory;
   file_factory = new_factory;
   return retval; 
}


////////////////////////////////////////////////////////////
// API FUNCTIONS
//

edbFiletype dbSave(const char* filename, edbFiletype filetype)
{
   char fullname[256];
   munge_path(fullname,(char*)filename);


   cAutoIPtr<ITagFile> file(dbOpenFile(fullname,kTagOpenWrite));

   if (file == NULL)
      return 0;

   return dbSaveTagFile(file,filetype);
}


edbFiletype dbSaveTagFile(ITagFile* file, edbFiletype filetype)
{


   // Write out file type
   if (!write_file_type(file,filetype))
      return 0;

   // If we are not saving out a game sys, save out the file ref
   if (!(filetype & kFiletypeGAM))
      write_file_ref_tag(file,kFiletypeGAM,strip_pathname(gCurrGameFile));

   // Likewise, save out the map ref
   if (!(filetype & kFiletypeMAP))
      write_file_ref_tag(file,kFiletypeMAP,strip_pathname(gCurrMapFile));

   DispatchData msg = { filetype | kDatabaseSave, file };
   DispatchMsg(kMsgDatabase,&msg);
   
   return filetype;
}

////////////////////////////////////////


// @TODO: change this to three phases when map/mis splits up
static ulong load_order[] = { kFiletypeGAM, kFiletypeMAP|kFiletypeMIS};
static char* phase_names[] = { "gamesys", "mission"};

typedef BOOL (*find_file_func)(ulong ftype, const char* fname, char* filebuf);
static find_file_func special_file_find = NULL;

#define LOAD_ORDER_PHASES (sizeof(load_order)/sizeof(load_order[0]))

edbFiletype dbLoad(const char* filename, ulong loadtype)
{
   char fullname[256];
   const char* name = filename; 
   if (!file_factory)
   {
      if (!find_for_load(fullname,(char*)filename))
         return 0;
      name = fullname;
   }

   cAutoIPtr<ITagFile> file ( dbOpenFile(name,kTagOpenRead) );
   if (file == NULL)
      return 0;

   edbFiletype retval = dbLoadTagFile(file,loadtype); 

   // So user knows what file he just loaded.
   strncpy(gCurrFilename, (char *)strip_pathname(name),sizeof(gCurrFilename));
   gCurrFilename[sizeof(gCurrFilename)-1] = '\0'; 
   StatusField(SF_FILENAME, gCurrFilename); 

   return retval; 
}

edbFiletype dbMergeLoadTagFile(ITagFile* file, edbFiletype loadtype);


edbFiletype dbMergeLoad(const char* filename, ulong loadtype)
{
   char fullname[256];
   const char* name = filename; 
   if (!file_factory)
   {
      if (!find_for_load(fullname,(char*)filename))
         return 0;
      name = fullname;
   }

   cAutoIPtr<ITagFile> file ( dbOpenFile(name,kTagOpenRead) );
   if (file == NULL)
      return 0;

   edbFiletype retval = dbMergeLoadTagFile(file,loadtype); 

   // So user knows what file he just loaded.
   StatusField(SF_FILENAME, (char*)strip_pathname(fullname));

   return retval; 
}


edbFiletype dbLoadTagFile(ITagFile* file, edbFiletype loadtype)
{
   // We are starting from the ground up:
   gCurrFilenum = 0;

   // Send a reset message
   DispatchData msg = { kDatabaseReset, NULL } ;
   DispatchMsg(kMsgDatabase,&msg);
   
   reset_objid_space();

   edbFiletype retval = dbMergeLoadTagFile(file, loadtype);

   // Post-load all database parts that weren't loaded
   if (~retval & kFiletypeAll)
   {
      DispatchData postload = { kDatabasePostLoad | (kFiletypeAll & ~retval), NULL};
      DispatchMsg(kMsgDatabase,&postload);
   }

   // send a generic post load
   DispatchData postload = { kDatabasePostLoad, NULL};
   DispatchMsg(kMsgDatabase,&postload);

   // Suspend the sim
   AutoAppIPtr_(SimManager,pSim); 
   pSim->SuspendSim();

   return retval; 

}


edbFiletype dbMergeLoadTagFile(ITagFile* file, edbFiletype loadtype)
{
   // The actual type in the file, rather than just whether we've loaded it.
   ulong Filetype = 0;

   //  filenames of the various pieces, corresponding to load order
   char pieceNames[LOAD_ORDER_PHASES][256];
   // and whether we've found them 
   BOOL pieceFound[LOAD_ORDER_PHASES]; 
   memset(pieceFound,0,sizeof(pieceFound));
   
   // Figure out what kinds of stuff are in the file
   if (!read_file_type(file,&Filetype))
   {
      // @TODO: this is back-compatibility stuff, remove when obsolete
      // look for a mission header 
      if (ReadMissionHeader(file,NULL,pieceNames[0]))
      {
         Filetype = MISSION_FILE_TYPE;
         pieceFound[0] = TRUE;
      }

#ifdef NO_MISSION_HEADER_OR_FILETYPE
      else
      {
         Filetype = guess_Filetype_from_extension(fullname);
      }
#else
      else
      {
         // @HACK: I guess we're a cow file 
         Filetype = kFiletypeAll; 
      }
#endif 
   }

   edbFiletype retval = 0;

   // burn through the load order. 
   for (int i = 0; i < LOAD_ORDER_PHASES; i++)
   {
      char msgbuf[256];

      // what piece are we loading now?
      int piece = load_order[i] ;

      // only load the pieces we want
      piece &= loadtype;

      if (piece == 0)
         continue; 

      // okay, we're loading another file:
      gCurrFilenum++;

      // where to load it from
      ITagFile* use_file = NULL;

      // send status message
      sprintf(msgbuf,"Loading %s",phase_names[i]);
      Status(msgbuf);

      // do we have it in the file? 
      // Note: the & DB_PARTITION_MASK is for back-compatibility . 
      // old files don't have obj partitions in their type. 
      if ((Filetype & (piece & DB_PARTITION_MASK)) == (piece & DB_PARTITION_MASK))
      {
         file->AddRef();
         use_file = file;
      }
      else // look for it in the file ref tag
      {
         BOOL success = FALSE;
         char fullpiecename[256] = "";

         pieceFound[i] = pieceFound[i] || read_file_ref_tag(file,piece,pieceNames[i]);
         // we found a file, now lets try to open it
         if (pieceFound[i])
         {
            BOOL found = find_for_load(fullpiecename,pieceNames[i]);
            if (!found && special_file_find != NULL)
               found = special_file_find(piece,pieceNames[i],fullpiecename);

            if (found)
            {
               use_file = dbOpenFile(fullpiecename, kTagOpenRead);
               success = use_file != NULL;

               strcpy(pieceNames[i],strip_pathname(fullpiecename));
            }
         }
         if (!success)
         {
            sprintf(msgbuf,"Failed to open %s file %s",phase_names[i],fullpiecename);
            Status(msgbuf);
            retval = 0;
            break;
         }
      }

      // now use_file is valid.  Load the our piece from it
      DispatchData msg = { kDatabaseLoad | piece, use_file};
      DispatchMsg(kMsgDatabase,&msg);

      SafeRelease(use_file);

      // Now send a post-load for that piece
      msg.subtype = kDatabasePostLoad | piece;
      msg.data = NULL;
      DispatchMsg(kMsgDatabase,&msg);
      retval |= piece;
   }



   if (pieceFound[0])
   {
      // So user knows what gamesys he is using.
      set_gamesys_file(pieceNames[0]);
   }


   return retval;
}

////////////////////////////////////////

void dbReset(void)
{
   gCurrFilenum = 0;

   DispatchData msg = { kDatabaseReset, NULL};
   DispatchMsg(kMsgDatabase,&msg);
   
   reset_objid_space();

   // Clear out current game file.
   set_gamesys_file("");

   StatusField(SF_FILENAME, "");

   // This might want to move out.
   if (config_is_defined(LOAD_FILE_VAR))
      config_unset(LOAD_FILE_VAR);
}


void dbBuildDefault(void)
{
   DispatchData msg = { kDatabaseDefault, NULL};
   dbReset();
   DispatchMsg(kMsgDatabase,&msg);
}

//------------------------------------------------------------
// What's the current file number?
// Mainly interesting to the network code

int dbCurrentFilenum()
{
   return gCurrFilenum;
}

////////////////////////////////
// look, IFDEF EDITOR, dont put game code below this!!!

#ifdef EDITOR

////////////////////////////////////////////////////////////
// COMMANDS
//

//
// File dialog filters
//

static sFileMenuFilter db_file_save_filters[] = 
{
   { "DromEd Mission Files (*.mis)", "*.mis"},
   { "DromEd World Files (*.cow)", "*.cow"},
   { "DromEd GameSys Files (*.gam)", "*.gam"},
   { "DromEd Save Files (*.cow, *.mis, *.gam)", "*.mis;*.cow;*.gam"},
   { "All Files (*.*)", "*.*"},
   { NULL }
};

static sFileMenuFilter db_file_load_filters[] = 
{
   { "DromEd Save Files (*.cow, *.mis, *.gam)", "*.mis;*.cow;*.gam"},
   { "All Files (*.*)", "*.*"},
   { NULL }
};

// little helper for getting the filename

// ok, hack to try and modularize this
// the idea is you either are passing in a string
// or passing null and want the user to pick something

static char _maybe_buf[256]="";  // since it will be used for null inputs only

// @TODO: have a way to give a base filename that isnt null to this
static char *get_filename_maybe_dialog(char *str, eFileMenuMode fMode, const char *type_ext)
{
   if (str != NULL)
      while (isspace(*str)) str++;
   if (str == NULL || *str == '\0')
   {
      sFileMenuFilter* filter = (fMode == kFileMenuSave) ? db_file_save_filters : db_file_load_filters; 
      str=FileDialog(fMode, filter, type_ext, _maybe_buf, sizeof(_maybe_buf));
      if (str == NULL || *str == '\0')
         return NULL;
   }
   return str;
}

static void save_cmd_guts(char* filename, edbFiletype ftype)
{
   const char *type_ext=extension_for_filetype(ftype);
   filename=get_filename_maybe_dialog(filename,kFileMenuSave,type_ext);
   if (filename==NULL) return;

   const char* ext = strrchr(filename,'.');
   if (ext == NULL)
   {
      strcat(filename,".");
      strcat(filename,type_ext);
      ext = type_ext;
   }
   else
   { 
      ext++;
      if (stricmp(ext,type_ext) != 0) // different extentions
      {
         char statbuf[256];
         sprintf(statbuf,"Using .%s extension instead of standard .%s",ext,type_ext);
         Status(statbuf);
      }
   }

   // copy fname into our local buffer
   char filebuf[256];
   munge_path(filebuf,filename);
   filebuf[sizeof(filebuf)-1] = '\0';
   filename = filebuf;

   char backup[16];
   sprintf(backup,"backup.%s",ext);
   CopyFileNamed(backup,filename);   

   edbFiletype result = dbSave(filename,ftype);
   // So user knows what file he just saved.
   StatusField(SF_FILENAME, (char*)strip_pathname(filename));

   if (result > 0)
   {
      Status("Save successful");
      set_file_var(SAVE_FILE_VAR,filename,TRUE);
      set_file_var(LOAD_FILE_VAR,filename,FALSE);
   }
   else
      Status("Save Failed");
}

//
// Last file sim time check
//

static BOOL safe_to_save(void)
{
   int delta = GetSimTime() - GetSimFileTime(); 
   if (delta != 0)
   {
      mprintf("You are trying to save a mission in which sim time has run.\n If you are in full screen hit space or enter to cancel.\n");
      char msg[256];
      sprintf(msg,"The simulation has run for %.02f seconds since this file was last saved or loaded.\nYour file will need *immediate* surgery in order to be used as a shipping level\nSave anyway?",delta*1.0/SIM_TIME_SECOND); 

      return winui_GetYorN(msg); 
   }
   return TRUE; 
}


EXTERN void save_world(char* arg)
{
   if (safe_to_save())
      save_cmd_guts(arg,kFiletypeAll);
}

static void save_mission(char* arg)
{
   if (safe_to_save())
      save_cmd_guts(arg,MISSION_FILE_TYPE);
}

static void save_gamesys(char* arg)
{
   if (safe_to_save())
      save_cmd_guts(arg,kFiletypeGAM);
}



//---------------------------------------------------------------------
// Lets the user choose a .GAM file for the current mission.
//---------------------------------------------------------------------
static void set_gamesys(char *filename)
{  // how to do suffix
   filename=get_filename_maybe_dialog(filename,kFileMenuSave,DB_GAMESYS_SUFFIX);
   if (filename==NULL) return;

   char *ext = strchr(filename, '.');
   if (ext == NULL)
      strcat(filename, "." DB_GAMESYS_SUFFIX);
   set_gamesys_file((char *)strip_pathname(filename));
}

//------------------------------------------------------------
// Loading commands 
//

#ifndef SHIP
static int  ld_snd_hnd=SFX_NO_HND;

static void secret_load_start_sounds(void)
{
   if (config_is_defined("load_snds"))
   {
      sfx_parm loop_parm={64,SFXFLG_LOOP,0,0,0,100,0,0,NULL};
      char snd_name[128];

      if (!config_get_raw("load_snds",snd_name,128))
         strcpy(snd_name,"load_snd");
      if (snd_name[0]=='\0')
         strcpy(snd_name,"load_snd");
      if (strnicmp(snd_name,"null",4))  // if we arent null
         ld_snd_hnd=SFX_Play_Raw(SFX_STATIC, &loop_parm, snd_name);
   }
}

static void secret_load_end_sounds(void)
{
   if (ld_snd_hnd!=SFX_NO_HND)
   {
      SFX_Kill_Hnd(ld_snd_hnd);
      ld_snd_hnd=SFX_NO_HND;
   }
}
#else   // SHIP
#define secret_load_start_sounds()
#define secret_load_end_sounds()
#endif

EXTERN void load_brushes(char* str);

static BOOL load_cmd_file_find(ulong filetype, const char* filename, char* buf)
{
   char msgbuf[256];
   // find a good phase name
   int i;
   for (i = 0; i < LOAD_ORDER_PHASES; i++)
      if (load_order[i] & filetype)
         break;
   if (i < LOAD_ORDER_PHASES)
      sprintf(msgbuf,"Can't find %s file '%s'.\nBrowse?",phase_names[i],filename);
   else
      sprintf(msgbuf,"Can't find file '%s'.\nBrowse?",filename);
      
   if (winui_GetYorN(msgbuf))
   {
      char* result=FileDialog(kFileMenuLoad, db_file_load_filters, extension_for_filetype(filetype), _maybe_buf, sizeof(_maybe_buf));
      strcpy(buf,_maybe_buf);
      return result != NULL && *result != '\0';
   }
   return FALSE;
}

static void load_cmd_guts(char* filename, edbFiletype ftype)
{
   filename=get_filename_maybe_dialog(filename,kFileMenuLoad,extension_for_filetype(ftype));
   if (filename==NULL) return;

   // Look for the file in likely places with likely extensions
   BOOL found = FALSE;
   char filebuf[256];
   char* ext = strrchr(filename,'.');
   if (ext == NULL)  // No extension case
   {
      edbFiletype foundtype = find_for_load_extensionless(filebuf,filename);
      if (foundtype > 0)
      {
         found = TRUE;
      }
   }

   if (!found)
      found = find_for_load(filebuf,filename);
   if (!found)
   {
      char statbuf[256];
      sprintf(statbuf,"can't find %s",filename);
      Status(statbuf);  
      return;
   }

   secret_load_start_sounds();   

   if (!config_is_defined("no_inprog"))
      dbSave("inprog." DB_FILE_SUFFIX, kFiletypeAll);

   filename = filebuf;
   find_file_func oldfunc = special_file_find;
   special_file_find = load_cmd_file_find;
   edbFiletype result = dbLoad(filename,ftype);
   special_file_find = oldfunc;
   if (result > 0)
   {
      Status("Load successful");
      //      set_file_var(SAVE_FILE_VAR,filename,TRUE);
      set_file_var(LOAD_FILE_VAR,filename,TRUE);
   }
   else
      Status("Load Failed");
   secret_load_end_sounds();  
}

//---------------------------------------------------------------------
// For compatability, checks input before loading the .COW file.
//---------------------------------------------------------------------

EXTERN void load_file(char *str)
{
   load_cmd_guts(str,kFiletypeAll);
}

EXTERN void load_gamesys(char* str)
{
   char buf[256];
   attachPostfix(buf,str,"." DB_GAMESYS_SUFFIX);
   load_cmd_guts(buf,kFiletypeGAM);
   set_gamesys_file(buf); 
}

// for init_edit
EXTERN void new_world(void)
{
   char buf[80];
   if (config_get_raw("default_gamesys",buf,sizeof(buf)))
   {
      char fname[256];
      attachPostfix(fname,buf,"." DB_GAMESYS_SUFFIX);
      char fpath[256]; 
      if (find_for_load(fpath,fname))
      {
         dbLoad(fpath,kFiletypeGAM);
         set_gamesys_file(fpath); 
         return; 
      }
   }
   dbBuildDefault(); 
}

//////////////////////////////////////////

//
// Debugging command for dispatch
//

#pragma off(unreferenced)
void dispatch_noise(char* arg)
{
#ifndef SHIP
   ILoop * pLoop = AppGetObj(ILoop);

   uint diags;
   ulong messes;

   pLoop->GetDiagnostics(&diags, &messes);
   
   ulong bits;
   sscanf(arg,"%lx",&bits);

   messes ^= bits;
   mprintf("Loopmode noise now %X\n",messes);
   pLoop->SetDiagnostics(diags,messes);
   SafeRelease(pLoop);

#endif
}
#pragma on(unreferenced)

////////////////////////////////////////

Command dbfile_commands[] = 
{
   { "save_cow",  FUNC_STRING, save_world, "Save world to file" }, 
   { "save_mission",  FUNC_STRING, save_mission, "Save mission to file" }, 
   { "save_gamesys", FUNC_STRING, save_gamesys, "Save gamesys info only" }, 
   { "load_file", FUNC_STRING, load_file, "Load .MIS, .COW, or .GAM files" },
   { "load_gamesys", FUNC_STRING, load_gamesys, "Load .GAM file - Resets the level" },
   { "set_gamesys", FUNC_STRING, set_gamesys, "Set gamesys file for current mission" }, 
   { "clear_world", FUNC_VOID,   dbBuildDefault, "Reset to the empty world"},
   { "dispatch_noise",FUNC_STRING, dispatch_noise, "Toggle dispatch noise for hex message mask\n"},
   { "file_menu", FUNC_VOID, FileMenu, "Bring up the file menu" },
   { "new_world", FUNC_VOID, new_world, "Clear the world and load the gamesys" },
};
#endif // ~EDITOR

void dbfile_setup_commands(void)
{
#ifdef EDITOR
   COMMANDS(dbfile_commands,HK_ALL);
#endif
}

