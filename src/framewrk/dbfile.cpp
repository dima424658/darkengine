/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/framewrk/dbfile.cpp,v 2.38 2000/03/07 19:57:07 toml Exp $

#include <lg.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <new>
#include <vector>

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
#include <missiondiag.h>
#ifdef EDITOR
#include <brestore.h>
#endif

#include <filemenu.h>

#include <comtools.h>
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

static int gDBLoadDepth = 0;
static BOOL gDBLoadFailed = FALSE;
static char gDBLoadFailure[512] = "";

void dbLoadReportFailure(const char* subsystem, const char* detail)
{
   const char* safeSubsystem = subsystem && *subsystem ? subsystem : "mission-data";
   const char* safeDetail = detail && *detail ? detail : "unspecified load failure";
   MissionDiagnosticsIssue("ERROR",safeSubsystem,safeDetail);
   if (gDBLoadDepth > 0 && !gDBLoadFailed)
   {
      gDBLoadFailed = TRUE;
      _snprintf(gDBLoadFailure,sizeof(gDBLoadFailure)-1,"%s: %s",
                safeSubsystem,safeDetail);
      gDBLoadFailure[sizeof(gDBLoadFailure)-1] = '\0';
   }
}

BOOL dbLoadHasFailed(void)
{
   return gDBLoadFailed;
}

const char* dbLoadFailureReason(void)
{
   return gDBLoadFailure;
}

static void LGAPI database_tagfile_error(ITagFile*, const TagFileTag* tag,
                                         const char* operation,
                                         const char* detail, void*)
{
   char diagnostic[512];
   _snprintf(diagnostic,sizeof(diagnostic)-1,
             "Tag-file %s failed%s%s: %s",
             operation ? operation : "read",
             tag ? " for " : "", tag ? tag->label : "",
             detail ? detail : "invalid data");
   diagnostic[sizeof(diagnostic)-1] = '\0';
   dbLoadReportFailure("chunk-reader",diagnostic);
}

static void begin_load_transaction(void)
{
   if (gDBLoadDepth++ == 0)
   {
      gDBLoadFailed = FALSE;
      gDBLoadFailure[0] = '\0';
      TagFileSetErrorObserver(database_tagfile_error,NULL);
   }
}

static void end_load_transaction(void)
{
   if (gDBLoadDepth > 0 && --gDBLoadDepth == 0)
      TagFileSetErrorObserver(NULL,NULL);
}

struct sCompatibilityChunk
{
   ITagFile* source;
   TagFileTag tag;
   TagVersion version;
   ulong filetype;
   ulong size;
   ulong tailOffset;
   eDBCompatibilityStatus status;
   BOOL retainRaw;
   BOOL hasRecordTails;
   char detail[80];
   std::vector<char> bytes;
};

static std::vector<sCompatibilityChunk> gCompatibilityChunks;
static std::vector<ITagFile*> gCompatibilitySources;
static ITagFile* gCompatibilitySource = NULL;
static ulong gCompatibilityFiletype = 0;
static ulong gCompatibilitySaveMask = 0;

struct sCompatibilityRecordTail
{
   ITagFile* source;
   TagFileTag tag;
   ulong filetype;
   int object;
   ulong recordSize;
   std::vector<char> bytes;
};

static std::vector<sCompatibilityRecordTail> gCompatibilityRecordTails;

struct sDBSourceFormatChunk
{
   ITagFile* source;
   TagFileTag tag;
   TagVersion version;
   ulong filetype;
   ulong size;
   std::vector<char> bytes;
};

static std::vector<sDBSourceFormatChunk> gDBSourceFormat;
static eDBWriteProfile gDBWriteProfile = kDBWritePreserveSource;
static BOOL gDBSourceTargetFrozen = FALSE;

static void clear_compatibility_state()
{
   for (size_t i = 0; i < gCompatibilitySources.size(); ++i)
      gCompatibilitySources[i]->Release();
   gCompatibilitySources.clear();
   gCompatibilityChunks.clear();
   gCompatibilityRecordTails.clear();
}

static const char* compatibility_status_name(eDBCompatibilityStatus status)
{
   switch (status)
   {
      case kDBCompatDecoded:       return "decoded";
      case kDBCompatPrefixDecoded: return "prefix-decoded";
      case kDBCompatRawPreserved:  return "raw-preserved";
      default:                     return "unsupported";
   }
}

static sCompatibilityChunk* find_compatibility_chunk(const TagFileTag* tag)
{
   for (int i = (int)gCompatibilityChunks.size() - 1; i >= 0; --i)
   {
      sCompatibilityChunk& chunk = gCompatibilityChunks[i];
      if (!strcmp(chunk.tag.label,tag->label) &&
          ((!gCompatibilitySaveMask && chunk.source == gCompatibilitySource &&
            (chunk.filetype & gCompatibilityFiletype)) ||
           (gCompatibilitySaveMask && (chunk.filetype & gCompatibilitySaveMask))))
         return &chunk;
   }
   return NULL;
}

void dbCompatibilityMarkTag(const TagFileTag* tag,
                            eDBCompatibilityStatus status,
                            const char* detail)
{
   sCompatibilityChunk* chunk = find_compatibility_chunk(tag);
   if (!chunk)
      return;
   chunk->status = status;
   if (detail)
   {
      strncpy(chunk->detail,detail,sizeof(chunk->detail));
      chunk->detail[sizeof(chunk->detail)-1] = '\0';
   }
}

void dbCompatibilityRetainRawTag(const TagFileTag* tag, const char* detail)
{
   sCompatibilityChunk* chunk = find_compatibility_chunk(tag);
   if (!chunk)
      return;
   chunk->retainRaw = TRUE;
   if (detail)
   {
      strncpy(chunk->detail,detail,sizeof(chunk->detail));
      chunk->detail[sizeof(chunk->detail)-1] = '\0';
   }
}

BOOL dbCompatibilityUseRawTag(const TagFileTag* tag)
{
   sCompatibilityChunk* chunk = find_compatibility_chunk(tag);
   return chunk && ((chunk->size == 0 &&
                     gDBWriteProfile == kDBWritePreserveSource) ||
                    chunk->retainRaw ||
                    chunk->status == kDBCompatRawPreserved ||
                    (chunk->status == kDBCompatPrefixDecoded &&
                     !chunk->hasRecordTails)) &&
          (chunk->size == 0 || !chunk->bytes.empty());
}

void dbCompatibilityRewriteTag(const TagFileTag* tag, const char* detail)
{
   sCompatibilityChunk* chunk = find_compatibility_chunk(tag);
   if (!chunk)
      return;
   chunk->retainRaw = FALSE;
   chunk->hasRecordTails = TRUE;
   chunk->status = kDBCompatPrefixDecoded;
   if (detail)
   {
      strncpy(chunk->detail,detail,sizeof(chunk->detail));
      chunk->detail[sizeof(chunk->detail)-1] = '\0';
   }
}

const void* dbCompatibilityGetRawTag(const TagFileTag* tag, ulong* size,
                                     TagVersion* version)
{
   if (size)
      *size = 0;
   if (version)
      memset(version,0,sizeof(*version));
   sCompatibilityChunk* chunk = find_compatibility_chunk(tag);
   if (!chunk || chunk->bytes.empty())
      return NULL;
   if (size)
      *size = (ulong)chunk->bytes.size();
   if (version)
      *version = chunk->version;
   return chunk->bytes.data();
}

eDBWriteProfile dbFormatGetWriteProfile(void)
{
   return gDBWriteProfile;
}

eDBWriteProfile dbFormatSetWriteProfile(eDBWriteProfile profile)
{
   eDBWriteProfile previous = gDBWriteProfile;
   gDBWriteProfile = profile;
   return previous;
}

BOOL dbFormatSourceTag(const TagFileTag* tag, TagVersion* version, ulong* size)
{
   for (int i = (int)gDBSourceFormat.size() - 1; i >= 0; --i)
   {
      const sDBSourceFormatChunk& chunk = gDBSourceFormat[i];
      if (!strcmp(chunk.tag.label,tag->label) &&
          (!gCompatibilitySaveMask || (chunk.filetype & gCompatibilitySaveMask)))
      {
         if (version)
            *version = chunk.version;
         if (size)
            *size = chunk.size;
         return TRUE;
      }
   }
   return FALSE;
}

const void* dbFormatSourceRawTag(const TagFileTag* tag, ulong* size,
                                 TagVersion* version)
{
   if (size)
      *size = 0;
   if (version)
      memset(version,0,sizeof(*version));
   for (int i = (int)gDBSourceFormat.size() - 1; i >= 0; --i)
   {
      const sDBSourceFormatChunk& chunk = gDBSourceFormat[i];
      if (!strcmp(chunk.tag.label,tag->label) && !chunk.bytes.empty() &&
          (!gCompatibilitySaveMask ||
           (chunk.filetype & gCompatibilitySaveMask)))
      {
         if (size)
            *size = (ulong)chunk.bytes.size();
         if (version)
            *version = chunk.version;
         return chunk.bytes.data();
      }
   }

   // During an ordinary load/save cycle the compatibility inventory owns the
   // only raw copy.  The source inventory takes ownership only while an
   // external-tool target is frozen, avoiding a second copy of very large
   // blocks such as WREXT in 32-bit DromEd.
   return dbCompatibilityGetRawTag(tag,size,version);
}

BOOL dbFormatTargetHasTag(const TagFileTag* tag)
{
   const BOOL newDarkOnly = !strcmp(tag->label,"WREXT") ||
                            !strcmp(tag->label,"LM_PARAM");
   if (gDBWriteProfile == kDBWriteRetail && newDarkOnly)
      return FALSE;
   if ((gDBWriteProfile == kDBWriteNewDark ||
        gDBWriteProfile == kDBWriteKCToolsExchange) && newDarkOnly)
      return TRUE;
   return dbFormatSourceTag(tag,NULL,NULL);
}

BOOL dbFormatUsesNewDark(void)
{
   return gDBWriteProfile == kDBWriteNewDark ||
          gDBWriteProfile == kDBWriteKCToolsExchange;
}

void dbFormatFreezeSourceTarget(BOOL freeze)
{
   if (freeze && !gDBSourceTargetFrozen)
   {
      for (size_t sourceIndex = 0;
           sourceIndex < gDBSourceFormat.size(); ++sourceIndex)
      {
         sDBSourceFormatChunk& source = gDBSourceFormat[sourceIndex];
         if (!source.bytes.empty())
            continue;
         for (size_t chunkIndex = 0;
              chunkIndex < gCompatibilityChunks.size(); ++chunkIndex)
         {
            sCompatibilityChunk& chunk = gCompatibilityChunks[chunkIndex];
            if (source.source == chunk.source &&
                !strcmp(source.tag.label,chunk.tag.label))
            {
               source.bytes.swap(chunk.bytes);
               break;
            }
         }
      }
   }
   else if (!freeze && gDBSourceTargetFrozen)
   {
      // KCTools writes every input chunk. Once an imported chunk is at least
      // as complete as its original, its current compatibility copy is the
      // canonical raw source. Retain an original only when the tool omitted
      // the block or shortened a suffix (notably RENDPARAMS in some versions).
      for (size_t sourceIndex = 0;
           sourceIndex < gDBSourceFormat.size(); ++sourceIndex)
      {
         sDBSourceFormatChunk& source = gDBSourceFormat[sourceIndex];
         if (source.bytes.empty())
            continue;
         for (size_t chunkIndex = 0;
              chunkIndex < gCompatibilityChunks.size(); ++chunkIndex)
         {
            const sCompatibilityChunk& chunk =
               gCompatibilityChunks[chunkIndex];
            if (!strcmp(source.tag.label,chunk.tag.label) &&
                (source.filetype & chunk.filetype) &&
                chunk.bytes.size() >= source.bytes.size())
            {
               std::vector<char>().swap(source.bytes);
               break;
            }
         }
      }
   }
   gDBSourceTargetFrozen = freeze;
}

void dbCompatibilityPreserveRecordTail(const TagFileTag* tag, int object,
                                       const void* bytes, ulong size,
                                       ulong recordSize)
{
#ifdef EDITOR
   if (!size || !gCompatibilitySource)
      return;
   sCompatibilityRecordTail tail = {};
   tail.source = gCompatibilitySource;
   tail.tag = *tag;
   tail.filetype = gCompatibilityFiletype;
   tail.object = object;
   tail.recordSize = recordSize;
   tail.bytes.resize(size);
   memcpy(tail.bytes.data(),bytes,size);
   gCompatibilityRecordTails.push_back(tail);
   sCompatibilityChunk* chunk = find_compatibility_chunk(tag);
   if (chunk)
      chunk->hasRecordTails = TRUE;
   dbCompatibilityMarkTag(tag,kDBCompatPrefixDecoded,
                          "record extensions retained for round-trip");
#endif
}

const void* dbCompatibilityGetRecordTail(const TagFileTag* tag, int object,
                                         ulong* size, ulong* recordSize)
{
   if (size)
      *size = 0;
   if (recordSize)
      *recordSize = 0;
#ifdef EDITOR
   for (int i = (int)gCompatibilityRecordTails.size() - 1; i >= 0; --i)
   {
      const sCompatibilityRecordTail& tail = gCompatibilityRecordTails[i];
      if (tail.object == object && !strcmp(tail.tag.label,tag->label) &&
          (tail.filetype & gCompatibilitySaveMask))
      {
         if (size)
            *size = (ulong)tail.bytes.size();
         if (recordSize)
            *recordSize = tail.recordSize;
         return tail.bytes.data();
      }
   }
#endif
   return NULL;
}

static void LGAPI compatibility_block_observer(ITagFile* file,
                                                const TagFileTag* tag,
                                                const TagVersion*,
                                                ulong size,
                                                ulong consumed,
                                                BOOL closing,
                                                void*)
{
   sCompatibilityChunk* chunk = find_compatibility_chunk(tag);
   if (!chunk)
      return;

   if (file->OpenMode() == kTagOpenWrite)
   {
      // Read-only prefixes use the retained block; property records with
      // preserved suffixes are allowed to use their normal writer.
      return;
   }

   if (!closing)
   {
      chunk->status = kDBCompatDecoded;
      chunk->tailOffset = size;
   }
   else if (chunk->status == kDBCompatRawPreserved ||
            chunk->status == kDBCompatUnsupported)
   {
      // A codec made an explicit classification.  Do not downgrade it merely
      // because it inspected a prefix before deciding the block was unsafe.
   }
   else if (consumed == 0 && size != 0)
   {
#ifdef EDITOR
      chunk->status = kDBCompatRawPreserved;
#else
      chunk->status = kDBCompatUnsupported;
#endif
      chunk->tailOffset = 0;
   }
   else if (consumed < size)
   {
      chunk->status = kDBCompatPrefixDecoded;
      chunk->tailOffset = consumed;
   }
}

static void inventory_compatibility_chunks(ITagFile* file, ulong filetype)
{
   cAutoIPtr<ITagFileIter> iter = file->Iterate();
   if (!iter)
      return;

   BOOL retained = FALSE;
   for (size_t i = 0; i < gCompatibilitySources.size(); ++i)
      if (gCompatibilitySources[i] == file)
         retained = TRUE;
   if (!retained)
   {
      file->AddRef();
      gCompatibilitySources.push_back(file);
   }

   TagFileSetBlockObserver(NULL,NULL);
   for (iter->Start(); !iter->Done(); iter->Next())
   {
      const TagFileTag* tag = iter->Tag();
      BOOL merged = FALSE;
      for (size_t i = 0; i < gCompatibilityChunks.size(); ++i)
      {
         if (gCompatibilityChunks[i].source == file &&
             !strcmp(gCompatibilityChunks[i].tag.label,tag->label))
         {
            gCompatibilityChunks[i].filetype |= filetype;
            merged = TRUE;
            break;
         }
      }
      if (merged)
      {
         if (!gDBSourceTargetFrozen)
            for (size_t formatIndex = 0;
                 formatIndex < gDBSourceFormat.size(); ++formatIndex)
               if (gDBSourceFormat[formatIndex].source == file &&
                   !strcmp(gDBSourceFormat[formatIndex].tag.label,
                           tag->label))
               {
                  gDBSourceFormat[formatIndex].filetype |= filetype;
                  break;
               }
         continue;
      }

      sCompatibilityChunk chunk = {};
      chunk.source = file;
      chunk.tag = *tag;
      chunk.filetype = filetype;
      chunk.size = file->BlockSize(tag);
#ifdef EDITOR
      if (gDBSourceTargetFrozen)
      {
         // The imported block is about to become the current raw copy. Drop
         // an equally complete frozen payload before allocating it, so a
         // baked 187 MB WREXT never needs two simultaneous raw buffers.
         for (size_t sourceIndex = 0;
              sourceIndex < gDBSourceFormat.size(); ++sourceIndex)
         {
            sDBSourceFormatChunk& source = gDBSourceFormat[sourceIndex];
            if (!strcmp(source.tag.label,chunk.tag.label) &&
                (source.filetype & filetype) &&
                chunk.size >= source.bytes.size())
            {
               std::vector<char>().swap(source.bytes);
               break;
            }
         }
      }
      if (chunk.size > INT_MAX)
      {
         char diagnostic[256];
         _snprintf(diagnostic,sizeof(diagnostic)-1,
                   "Chunk %.12s is %lu bytes, exceeding the reader's safe single-block limit",
                   chunk.tag.label,chunk.size);
         diagnostic[sizeof(diagnostic)-1] = '\0';
         dbLoadReportFailure("chunk-reader",diagnostic);
         return;
      }
      chunk.status = kDBCompatRawPreserved;
      try
      {
         chunk.bytes.resize(chunk.size);
      }
      catch (const std::bad_alloc&)
      {
         char diagnostic[256];
         _snprintf(diagnostic,sizeof(diagnostic)-1,
                   "Chunk %.12s requests %lu bytes, which cannot be retained safely",
                   chunk.tag.label,chunk.size);
         diagnostic[sizeof(diagnostic)-1] = '\0';
         dbLoadReportFailure("chunk-reader",diagnostic);
         return;
      }
#else
      chunk.status = kDBCompatUnsupported;
#endif
      // VersionNumsCompare is a historical subtract-based comparator; using
      // UINT_MAX would wrap its result back to -1.  INT_MAX is still an
      // effective wildcard for persisted versions and remains ordered.
      TagVersion version = { 0x7fffffff, 0x7fffffff };
      if (SUCCEEDED(file->OpenBlock(tag,&version)))
      {
         chunk.version = version;
#ifdef EDITOR
         if (chunk.size)
         {
            long read = file->Read(chunk.bytes.data(),(int)chunk.size);
            if (read != (long)chunk.size)
            {
               char diagnostic[256];
               _snprintf(diagnostic,sizeof(diagnostic)-1,
                         "Chunk %.12s is truncated (%ld of %lu bytes)",
                         chunk.tag.label,read,chunk.size);
               diagnostic[sizeof(diagnostic)-1] = '\0';
               dbLoadReportFailure("chunk-reader",diagnostic);
            }
         }
#endif
         file->CloseBlock();
      }
      gCompatibilityChunks.push_back(chunk);

      if (!gDBSourceTargetFrozen)
      {
         BOOL updated = FALSE;
         for (size_t formatIndex = 0;
              formatIndex < gDBSourceFormat.size(); ++formatIndex)
         {
            sDBSourceFormatChunk& format = gDBSourceFormat[formatIndex];
            if (format.source == file &&
                !strcmp(format.tag.label,chunk.tag.label))
            {
               format.version = chunk.version;
               format.size = chunk.size;
               format.filetype |= filetype;
               updated = TRUE;
               break;
            }
         }
         if (!updated)
         {
            sDBSourceFormatChunk format = {};
            format.source = file;
            format.tag = chunk.tag;
            format.version = chunk.version;
            format.filetype = filetype;
            format.size = chunk.size;
            gDBSourceFormat.push_back(format);
         }
      }
   }
}

static void report_compatibility_chunks(const char* phase)
{
   unsigned decoded = 0;
   unsigned prefixDecoded = 0;
   unsigned rawPreserved = 0;
   unsigned unclassified = 0;
   unsigned explicitProblems = 0;
   FILE* report = fopen("mission_compatibility.log","a");
   if (report)
      fprintf(report,"Compatibility report: %s\n",phase);
   mprintf("Compatibility report: %s\n",phase);
   for (size_t i = 0; i < gCompatibilityChunks.size(); ++i)
   {
      const sCompatibilityChunk& chunk = gCompatibilityChunks[i];
      if (chunk.source != gCompatibilitySource ||
          !(chunk.filetype & gCompatibilityFiletype))
         continue;
      switch (chunk.status)
      {
         case kDBCompatDecoded:       ++decoded; break;
         case kDBCompatPrefixDecoded: ++prefixDecoded; break;
         case kDBCompatRawPreserved:  ++rawPreserved; break;
         default:                     ++unclassified; break;
      }
      mprintf("  %-12s %u.%u %-14s %u bytes%s%s\n",
              chunk.tag.label,chunk.version.major,chunk.version.minor,
              compatibility_status_name(chunk.status),(unsigned)chunk.size,
              chunk.detail[0] ? " - " : "",chunk.detail);
      if (report)
         fprintf(report,"  %-12s %u.%u %-14s %u bytes%s%s\n",
                 chunk.tag.label,chunk.version.major,chunk.version.minor,
                 compatibility_status_name(chunk.status),(unsigned)chunk.size,
                 chunk.detail[0] ? " - " : "",chunk.detail);

      // The inventory initially classifies unobserved property/relation and
      // per-AI blocks as unsupported. Many are intentionally consumed by
      // generic systems without touching the tag observer, so do not present
      // that raw state as thousands of false errors. Explicit codec details
      // are actionable and belong in the main report; the full inventory
      // remains in mission_compatibility.log.
      if (chunk.detail[0] &&
          (chunk.status == kDBCompatUnsupported ||
           chunk.status == kDBCompatRawPreserved ||
           chunk.status == kDBCompatPrefixDecoded))
      {
         char diagnostic[512];
         snprintf(diagnostic,sizeof(diagnostic),
                  "%s chunk %-12s %u.%u is %s (%u bytes)%s%s",
                  phase,chunk.tag.label,chunk.version.major,
                  chunk.version.minor,compatibility_status_name(chunk.status),
                  (unsigned)chunk.size,chunk.detail[0] ? ": " : "",chunk.detail);
         diagnostic[sizeof(diagnostic)-1] = '\0';
         MissionDiagnosticsIssue(
            chunk.status == kDBCompatUnsupported ? "ERROR" : "WARN",
            "mission-data",diagnostic);
         if (chunk.status == kDBCompatUnsupported)
            ++explicitProblems;
      }
   }
   {
      char diagnostic[512];
      snprintf(diagnostic,sizeof(diagnostic),
               "%s compatibility inventory: %u decoded, %u prefix-decoded, "
               "%u raw-preserved, %u generic/unclassified; %u explicit "
               "problem%s (full inventory: mission_compatibility.log)",
               phase,decoded,prefixDecoded,rawPreserved,unclassified,
               explicitProblems,explicitProblems == 1 ? "" : "s");
      diagnostic[sizeof(diagnostic)-1] = '\0';
      MissionDiagnosticsIssue(explicitProblems ? "ERROR" :
         (prefixDecoded || rawPreserved ? "WARN" : "INFO"),
         "mission-data",diagnostic);
   }
   if (report)
   {
      fputc('\n',report);
      fclose(report);
   }
}

static BOOL suppress_profile_chunk(const TagFileTag& tag)
{
   if (gDBWriteProfile == kDBWriteRetail)
      return !strcmp(tag.label,"WREXT") ||
             !strcmp(tag.label,"LM_PARAM");
   if (dbFormatUsesNewDark())
      return !strcmp(tag.label,"WRRGB") || !strcmp(tag.label,"WR");
   return FALSE;
}

static void replay_compatibility_chunks(ITagFile* file, ulong filetype)
{
#ifdef EDITOR
   for (int i = (int)gCompatibilityChunks.size() - 1; i >= 0; --i)
   {
      const sCompatibilityChunk& chunk = gCompatibilityChunks[i];
      if (suppress_profile_chunk(chunk.tag))
         continue;
      if (!(chunk.filetype & filetype) ||
          file->BlockSize(&chunk.tag) != kTagNoSuchBlock)
         continue;
      TagVersion version = chunk.version;
      if (SUCCEEDED(file->OpenBlock(&chunk.tag,&version)))
      {
         if (!chunk.bytes.empty())
            file->Write(chunk.bytes.data(),(int)chunk.bytes.size());
         file->CloseBlock();
      }
   }

   // An external tool may understand and rewrite only a subset of the
   // database.  Restore any source chunk it omitted, while never replacing a
   // block already emitted by a live codec or by the imported tool output.
   for (int i = (int)gDBSourceFormat.size() - 1; i >= 0; --i)
   {
      const sDBSourceFormatChunk& chunk = gDBSourceFormat[i];
      if (suppress_profile_chunk(chunk.tag))
         continue;
      if (!(chunk.filetype & filetype) ||
          (chunk.size != 0 && chunk.bytes.empty()) ||
          file->BlockSize(&chunk.tag) != kTagNoSuchBlock)
         continue;
      TagVersion version = chunk.version;
      if (SUCCEEDED(file->OpenBlock(&chunk.tag,&version)))
      {
         file->Write(chunk.bytes.data(),(int)chunk.bytes.size());
         file->CloseBlock();
      }
   }
#endif
}

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
   // Preserve explicit absolute paths without allowing the implicit working
   // directory probe used by the legacy helper for ordinary relative names.
   if (is_full_path(filename))
   {
      return find_file_in_config_path(fullname,filename,DB_FILE_DIR_VAR);
   }

   if (find_file_in_config_path_only(fullname,filename,DB_FILE_DIR_VAR))
   {
      return TRUE;
   }
   // load_path is the resolved content-priority list (uber mod, FM, normal
   // mods, retail).  Do not let the process working directory jump ahead of
   // it: that would make a retail Miss1.mis mask a configured replacement.
   if (find_file_in_config_path_only(fullname,filename,LOAD_PATH_VAR))
   {
      return TRUE;
   }
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

static void set_file_var(const char* var, const char* filename, bool save)
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

static void set_gamesys_file(const char* fn)
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
   long bytesRead = file->Read((char*)Filetype,sizeof(*Filetype));
   BOOL retval = bytesRead == sizeof(*Filetype);
   file->CloseBlock();
   if (!retval)
      dbLoadReportFailure("mission-data","FILE_TYPE block is truncated");
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
   if (!success)
   {
      char diagnostic[160];
      _snprintf(diagnostic,sizeof(diagnostic)-1,
                "%.12s file-reference block is truncated",tag->label);
      diagnostic[sizeof(diagnostic)-1] = '\0';
      dbLoadReportFailure("mission-data",diagnostic);
   }
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

   gCompatibilitySaveMask = filetype;
   TagFileSetBlockObserver(compatibility_block_observer,NULL);
   DispatchData msg = { filetype | kDatabaseSave, file };
   DispatchMsg(kMsgDatabase,&msg);
   TagFileSetBlockObserver(NULL,NULL);
   replay_compatibility_chunks(file,filetype);
   gCompatibilitySaveMask = 0;
   
   return filetype;
}

////////////////////////////////////////


// @TODO: change this to three phases when map/mis splits up
static ulong load_order[] = { kFiletypeGAM, kFiletypeMAP|kFiletypeMIS};
static const char* phase_names[] = { "gamesys", "mission"};

typedef BOOL (*find_file_func)(ulong ftype, const char* fname, char* filebuf);
static find_file_func special_file_find = NULL;

#define LOAD_ORDER_PHASES (sizeof(load_order)/sizeof(load_order[0]))

edbFiletype dbLoad(const char* filename, ulong loadtype)
{
   char fullname[256];
   const char* name = filename; 
   MissionDiagnosticsBegin(filename);
   begin_load_transaction();
   if (!file_factory)
   {
      if (!find_for_load(fullname,(char*)filename))
      {
         char diagnostic[512];
         snprintf(diagnostic,sizeof(diagnostic),
                  "Mission file not found: %s",filename ? filename : "(null)");
         diagnostic[sizeof(diagnostic)-1] = '\0';
         MissionDiagnosticsIssue("ERROR","mission-data",diagnostic);
         MissionDiagnosticsFinish(FALSE);
         end_load_transaction();
         return 0;
      }
      name = fullname;
   }

   cAutoIPtr<ITagFile> file ( dbOpenFile(name,kTagOpenRead) );
   if (file == NULL)
   {
      char diagnostic[512];
      snprintf(diagnostic,sizeof(diagnostic),
               "Mission file could not be opened: %s",name ? name : "(null)");
      diagnostic[sizeof(diagnostic)-1] = '\0';
      MissionDiagnosticsIssue("ERROR","mission-data",diagnostic);
      MissionDiagnosticsFinish(FALSE);
      end_load_transaction();
      return 0;
   }

   edbFiletype retval = dbLoadTagFile(file,loadtype); 
   MissionDiagnosticsFinish(retval != 0);
   end_load_transaction();

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
   begin_load_transaction();
   if (!file)
   {
      dbLoadReportFailure("mission-data","Mission loader received no tag file");
      end_load_transaction();
      return 0;
   }
   // We are starting from the ground up:
   gCurrFilenum = 0;
   TagFileSetBlockObserver(NULL,NULL);
   clear_compatibility_state();
   if (!gDBSourceTargetFrozen)
      gDBSourceFormat.clear();

   // Send a reset message
   DispatchData msg = { kDatabaseReset, NULL } ;
   DispatchMsg(kMsgDatabase,&msg);
   
   reset_objid_space();

   edbFiletype retval = dbMergeLoadTagFile(file, loadtype);

   // A referenced piece (most commonly the .gam) may be unavailable or the
   // user may cancel the browse dialog.  The old path continued by sending
   // post-load messages to a database that had just been reset and only
   // partially reconstructed, which made cancellation crash-prone.  Leave a
   // valid empty editor database instead and do not post-load missing pieces.
   if (retval == 0 || dbLoadHasFailed())
   {
      dbBuildDefault();
      AutoAppIPtr_(SimManager,pSim);
      pSim->SuspendSim();
      end_load_transaction();
      return 0;
   }

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

   end_load_transaction();
   return retval; 

}


edbFiletype dbMergeLoadTagFile(ITagFile* file, edbFiletype loadtype)
{
   begin_load_transaction();
   if (!file)
   {
      dbLoadReportFailure("mission-data","Merge loader received no tag file");
      end_load_transaction();
      return 0;
   }
   // The actual type in the file, rather than just whether we've loaded it.
   ulong Filetype = 0;

   //  filenames of the various pieces, corresponding to load order
   char pieceNames[LOAD_ORDER_PHASES][256];
   memset(pieceNames,0,sizeof(pieceNames));
   // and whether we've found them 
   BOOL pieceFound[LOAD_ORDER_PHASES]; 
   memset(pieceFound,0,sizeof(pieceFound));
   
   // Figure out what kinds of stuff are in the file
   if (!read_file_type(file,&Filetype))
   {
      if (dbLoadHasFailed())
      {
         end_load_transaction();
         return 0;
      }
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
            if (!pieceFound[i])
               snprintf(msgbuf,sizeof(msgbuf),
                        "Mission has no %s file reference",phase_names[i]);
            else if (!fullpiecename[0])
               snprintf(msgbuf,sizeof(msgbuf),
                        "Referenced %s file was not found: %s",
                        phase_names[i],pieceNames[i]);
            else
               snprintf(msgbuf,sizeof(msgbuf),
                        "Failed to open %s file: %s",
                        phase_names[i],fullpiecename);
            msgbuf[sizeof(msgbuf)-1] = '\0';
            Status(msgbuf);
            MissionDiagnosticsIssue("ERROR","mission-data",msgbuf);
            retval = 0;
            break;
         }
      }

      // now use_file is valid.  Load the our piece from it
      inventory_compatibility_chunks(use_file,piece);
      if (dbLoadHasFailed())
      {
         SafeRelease(use_file);
         retval = 0;
         break;
      }
      gCompatibilitySource = use_file;
      gCompatibilityFiletype = piece;
      dbCompatibilityMarkTag(&dbTypeTag,kDBCompatDecoded,"database header");
      TagFileSetBlockObserver(compatibility_block_observer,NULL);
      DispatchData msg = { (uint)(kDatabaseLoad | piece), use_file};
      DispatchMsg(kMsgDatabase,&msg);
      TagFileSetBlockObserver(NULL,NULL);
      report_compatibility_chunks(phase_names[i]);
      gCompatibilitySource = NULL;
      gCompatibilityFiletype = 0;

      if (dbLoadHasFailed())
      {
         SafeRelease(use_file);
         retval = 0;
         break;
      }

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


   if (dbLoadHasFailed())
      retval = 0;
   end_load_transaction();
   return retval;
}

////////////////////////////////////////

void dbReset(void)
{
   gCurrFilenum = 0;
   TagFileSetBlockObserver(NULL,NULL);
   clear_compatibility_state();
   if (!gDBSourceTargetFrozen)
      gDBSourceFormat.clear();

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
#ifdef EDITOR
       // Object load callbacks run before inherited HasBrush values settle.
       // Rebuild the transient editor brushes at the next completed frame.
       if (result & (kFiletypeMAP | kFiletypeMIS))
          ScheduleBrushSelectionRestore();
#endif
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

static const char* db_write_profile_name(eDBWriteProfile profile)
{
   switch (profile)
   {
      case kDBWriteRetail:          return "retail";
      case kDBWriteNewDark:         return "newdark";
      case kDBWriteKCToolsExchange: return "kctools-exchange";
      default:                      return "preserve";
   }
}

static void db_format_report(void)
{
   mprintf("Database write profile: %s\n",
           db_write_profile_name(gDBWriteProfile));
   mprintf("Source chunk inventory (%u chunks):\n",
           (uint)gDBSourceFormat.size());
   for (size_t i = 0; i < gDBSourceFormat.size(); ++i)
   {
      const sDBSourceFormatChunk& chunk = gDBSourceFormat[i];
      const sCompatibilityChunk* compatibility = NULL;
      for (int compatibilityIndex =
              (int)gCompatibilityChunks.size() - 1;
           compatibilityIndex >= 0; --compatibilityIndex)
      {
         const sCompatibilityChunk& candidate =
            gCompatibilityChunks[compatibilityIndex];
         if (!strcmp(candidate.tag.label,chunk.tag.label) &&
             (candidate.filetype & chunk.filetype) &&
             candidate.source == chunk.source)
         {
            compatibility = &candidate;
            break;
         }
      }
      if (!compatibility)
         for (int compatibilityIndex =
                 (int)gCompatibilityChunks.size() - 1;
              compatibilityIndex >= 0; --compatibilityIndex)
         {
            const sCompatibilityChunk& candidate =
               gCompatibilityChunks[compatibilityIndex];
            if (!strcmp(candidate.tag.label,chunk.tag.label) &&
                (candidate.filetype & chunk.filetype))
            {
               compatibility = &candidate;
               break;
            }
         }
      mprintf("  %-12s %u.%u %8lu bytes  filetype %08lx  %s%s%s\n",
              chunk.tag.label,chunk.version.major,chunk.version.minor,
              chunk.size,chunk.filetype,
              compatibility ? compatibility_status_name(compatibility->status)
                            : "not-observed",
              compatibility && compatibility->detail[0] ? " - " : "",
              compatibility && compatibility->detail[0]
                 ? compatibility->detail : "");
   }
}

static void db_set_write_profile(char* name)
{
   if (!name)
      name = "";
   while (isspace((unsigned char)*name))
      ++name;
   if (!_stricmp(name,"preserve") || !_stricmp(name,"source"))
      gDBWriteProfile = kDBWritePreserveSource;
   else if (!_stricmp(name,"retail") || !_stricmp(name,"olddark"))
      gDBWriteProfile = kDBWriteRetail;
   else if (!_stricmp(name,"newdark"))
      gDBWriteProfile = kDBWriteNewDark;
   else
   {
      Warning(("Unknown database write profile '%s'. Use preserve, retail, or newdark.\n",
               name));
      return;
   }
   mprintf("Database write profile: %s\n",
           db_write_profile_name(gDBWriteProfile));
}

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
   { "db_format_report", FUNC_VOID, db_format_report, "show source chunk versions and output profile" },
   { "db_write_profile", FUNC_STRING, db_set_write_profile, "set output profile: preserve, retail, or newdark" },
};
#endif // ~EDITOR

void dbfile_setup_commands(void)
{
#ifdef EDITOR
   COMMANDS(dbfile_commands,HK_ALL);
#endif
}

