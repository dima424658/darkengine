#ifdef _WIN32
#include <win32_platform.h>
#endif

#include <contentctx.h>
#include <darklimit.h>

#include <config.h>
#include <cfg.h>
#include <mprintf.h>
#include <platform_services.h>

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#endif

// Keep one canonical, bounded representation instead of allowing individual
// subsystems to truncate different path lists.
#define CONTENT_PATH_SIZE DARK_LIMIT_RUNTIME_SEARCH_PATH_CHARS

static char gProfile[32] = "thief2";
static char gContentMode[32] = "configured";
static char gBasePath[DARK_CONTENT_MAX_PATH] = ".";
static char gFMRoot[DARK_CONTENT_MAX_PATH] = "";
static char gCollectionRoot[DARK_CONTENT_MAX_PATH] = "";
static char gCollectionGoldPath[DARK_CONTENT_MAX_PATH] = "";
static char gCollectionTDPPath[DARK_CONTENT_MAX_PATH] = "";
static char gFMPath[DARK_CONTENT_MAX_PATH] = "";
static char gFMName[DARK_CONTENT_MAX_PATH] = "";
static char gSavePath[DARK_CONTENT_MAX_PATH] = "saves";
static char gSaveFallbackPath[DARK_CONTENT_MAX_PATH] = "";
static BOOL gSavePathExplicit = FALSE;
static char gPaths[5][CONTENT_PATH_SIZE];
static char gSourcePaths[5][CONTENT_PATH_SIZE];
static uint gSourcePathPriorities[5];
static BOOL gSourcePathDefined[5];
static BOOL gSourcePathsCaptured = FALSE;

static const char* kContentPathVariables[5] = {
   "resname_base", "load_path", "script_module_path", "include_path",
   "movie_path"
};

static char* NextToken(char* text, const char* delimiters, char** context)
{
#ifdef _WIN32
   return strtok_s(text, delimiters, context);
#else
   return strtok_r(text, delimiters, context);
#endif
}

static BOOL IsAbsolutePath(const char* path)
{
   return path && (path[0] == '\\' || path[0] == '/' ||
                   (isalpha((unsigned char)path[0]) && path[1] == ':'));
}

static void Trim(char* text)
{
   char* start = text;
   char* end;
   while (*start && isspace((unsigned char)*start))
      ++start;
   if (start != text)
      memmove(text, start, strlen(start) + 1);
   end = text + strlen(text);
   while (end > text && isspace((unsigned char)end[-1]))
      --end;
   *end = '\0';
}

static void JoinPath(char* output, int size, const char* root, const char* leaf)
{
   size_t rootLen;
   if (!output || size <= 0)
      return;
   output[0] = '\0';
   const auto append = [output, size](const char* text)
   {
      if (!text)
         return;
      size_t used = strlen(output);
      size_t available = (size_t)size - used - 1;
      size_t count = strlen(text);
      if (count > available)
         count = available;
      if (count)
         memmove(output + used, text, count);
      output[used + count] = '\0';
   };
   if (!leaf || !leaf[0])
      append(root);
   else if (IsAbsolutePath(leaf))
      append(leaf);
   else
   {
      rootLen = root ? strlen(root) : 0;
#ifdef _WIN32
      const char separator = '\\';
#else
      const char separator = '/';
#endif
      append(root);
      if (rootLen && root[rootLen - 1] != '\\' && root[rootLen - 1] != '/')
         append(separator == '\\' ? "\\" : "/");
      append(leaf);
   }
#ifndef _WIN32
   for (char* character = output; *character; ++character)
      if (*character == '\\')
         *character = '/';
#endif
}

static BOOL DirectoryExists(const char* path)
{
#ifdef _WIN32
   DWORD attributes = GetFileAttributesA(path);
   return attributes != INVALID_FILE_ATTRIBUTES &&
          (attributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
#else
   struct stat status;
   char resolved[DARK_CONTENT_MAX_PATH];
   return path && PlatformResolvePath(path, resolved, sizeof(resolved)) &&
          stat(resolved, &status) == 0 && S_ISDIR(status.st_mode);
#endif
}

static BOOL FileExists(const char* path)
{
   char resolved[DARK_CONTENT_MAX_PATH];
   return path && PlatformResolvePath(path, resolved, sizeof(resolved));
}

static const char* DetectProfile(const char* directory)
{
   char path[DARK_CONTENT_MAX_PATH];
   JoinPath(path, sizeof(path), directory, "cam.cfg");
   if (!FileExists(path))
      return NULL;
   JoinPath(path, sizeof(path), directory, "MISS18.MIS");
   if (FileExists(path))
      return "gold";
   JoinPath(path, sizeof(path), directory, "MISS8.MIS");
   if (FileExists(path))
      return "thief2";
   JoinPath(path, sizeof(path), directory, "DARK.GAM");
   return FileExists(path) ? "tdp" : NULL;
}

static void AddPath(char* list, int size, const char* path)
{
   int used;
   if (!path || !path[0] || !DirectoryExists(path))
      return;
   used = (int)strlen(list);
   if (used && used + 1 < size)
      list[used++] = '+';
   if (used < size)
   {
      strncpy(list + used, path, size - used);
      list[size - 1] = '\0';
   }
}

static void AddExpandedList(char* output, int outputSize, const char* list,
                            const char* suffix)
{
   char copy[CONTENT_PATH_SIZE];
   char* token;
   char* context = NULL;
   if (!list || !list[0])
      return;
   strncpy(copy, list, sizeof(copy));
   copy[sizeof(copy) - 1] = '\0';
   for (token = NextToken(copy, "+;", &context); token;
        token = NextToken(NULL, "+;", &context))
   {
      char absolute[DARK_CONTENT_MAX_PATH];
      char suffixed[DARK_CONTENT_MAX_PATH];
      Trim(token);
      JoinPath(absolute, sizeof(absolute), gBasePath, token);
      JoinPath(suffixed, sizeof(suffixed), absolute, suffix);
      AddPath(output, outputSize, suffixed);
   }
}

static void AddConfigPaths(char* output, int outputSize, const char* variable)
{
   char value[CONTENT_PATH_SIZE] = "";
   if (config_get_raw(variable, value, sizeof(value)))
      AddExpandedList(output, outputSize, value, NULL);
}

static void AddLastExpandedPath(char* output, int outputSize, const char* list,
                                const char* suffix)
{
   char copy[CONTENT_PATH_SIZE];
   char selected[DARK_CONTENT_MAX_PATH] = "";
   char* token;
   char* context = NULL;
   if (!list || !list[0])
      return;
   strncpy(copy, list, sizeof(copy));
   copy[sizeof(copy) - 1] = '\0';
   for (token = NextToken(copy, "+;", &context); token;
        token = NextToken(NULL, "+;", &context))
   {
      char absolute[DARK_CONTENT_MAX_PATH];
      char suffixed[DARK_CONTENT_MAX_PATH];
      Trim(token);
      JoinPath(absolute, sizeof(absolute), gBasePath, token);
      JoinPath(suffixed, sizeof(suffixed), absolute, suffix);
      if (DirectoryExists(suffixed))
      {
         strncpy(selected, suffixed, sizeof(selected));
         selected[sizeof(selected) - 1] = '\0';
      }
   }
   AddPath(output, outputSize, selected);
}

static void AddLastConfigPath(char* output, int outputSize,
                              const char* variable)
{
   char value[CONTENT_PATH_SIZE] = "";
   if (config_get_raw(variable, value, sizeof(value)))
      AddLastExpandedPath(output, outputSize, value, NULL);
}

static const char* CommandValue(int argc, const char* argv[], const char* name)
{
   size_t nameLen = strlen(name);
   int i;
   for (i = argc - 1; i >= 1; --i)
      if (_strnicmp(argv[i], name, nameLen) == 0 && argv[i][nameLen] == '=')
         return argv[i] + nameLen + 1;
   return NULL;
}

static BOOL IsSafeFMName(const char* name);

static const char* SelectedFMName(int argc, const char* argv[], char* storage,
                                  int storageSize)
{
   const char* fmName = CommandValue(argc, argv, "fm");
   // An explicit original-content selection must also suppress any fm value
   // inherited from an old environment or installation configuration.
   if (!fmName && CommandValue(argc, argv, "content"))
      return NULL;
   if (!fmName)
      fmName = getenv("DARK_FM_NAME");
   if (!fmName && config_get_raw("fm", storage, storageSize))
      fmName = storage;
   return IsSafeFMName(fmName) ? fmName : NULL;
}

static void ResolveFMRoot(int argc, const char* argv[], char* result,
                          int resultSize)
{
   char fmBase[CONTENT_PATH_SIZE] = "FMs";
   const char* commandFMPath;
   const char* launcherFMRoot;

   commandFMPath = CommandValue(argc, argv, "fm_path");
   if (commandFMPath)
   {
      strncpy(fmBase, commandFMPath, sizeof(fmBase));
      fmBase[sizeof(fmBase) - 1] = '\0';
   }
   else
   {
      launcherFMRoot = getenv("DARK_FM_ROOT");
      if (launcherFMRoot && launcherFMRoot[0])
      {
         strncpy(fmBase, launcherFMRoot, sizeof(fmBase));
         fmBase[sizeof(fmBase) - 1] = '\0';
      }
      else
         config_get_raw("fm_path", fmBase, sizeof(fmBase));
   }
   JoinPath(result, resultSize, gBasePath, fmBase);
}

static BOOL ResolveFMPath(int argc, const char* argv[], const char* fmName,
                          char* result, int resultSize)
{
   char fmRoot[DARK_CONTENT_MAX_PATH];
   if (!fmName)
      return FALSE;
   ResolveFMRoot(argc, argv, fmRoot, sizeof(fmRoot));
   JoinPath(result, resultSize, fmRoot, fmName);
   return DirectoryExists(result);
}

static BOOL IsSafeFMName(const char* name)
{
   return name && name[0] && !IsAbsolutePath(name) &&
          !strstr(name, "..") && !strchr(name, '\\') && !strchr(name, '/');
}

struct sPickerBootstrapLimits
{
   int objMin;
   int objMax;
   int maxRefs;
   int soundChannels;
   int directories;
   int files;
};

static int ClampPickerLimit(const char* variable, long value, int minimum,
                            int maximum, const char* source)
{
   if (value >= minimum && value <= maximum)
      return (int)value;

   char diagnostic[512];
   const int clamped = value < minimum ? minimum : maximum;
   _snprintf(diagnostic, sizeof(diagnostic) - 1,
             "%s requests %s=%ld outside the supported range %d..%d; "
             "using %d for startup capacity", source, variable, value,
             minimum, maximum, clamped);
   diagnostic[sizeof(diagnostic) - 1] = '\0';
   PlatformDiagnosticsLog("ERROR", "content-bootstrap", diagnostic);
   return clamped;
}

static void ReadPickerLimitFile(const char* path,
                                sPickerBootstrapLimits* limits)
{
   char resolved[DARK_CONTENT_MAX_PATH];
   if (!PlatformResolvePath(path, resolved, sizeof(resolved)))
      return;

   FILE* file = fopen(resolved, "rt");
   if (!file)
      return;
   ++limits->files;

   char line[DARK_LIMIT_RUNTIME_CONFIG_LINE_CHARS + 2];
   while (fgets(line, sizeof(line), file))
   {
      char* comment = strchr(line, ';');
      if (comment)
         *comment = '\0';
      Trim(line);
      if (!line[0])
         continue;

      char* valueText = line;
      while (*valueText && !isspace((unsigned char)*valueText))
         ++valueText;
      if (!*valueText)
         continue;
      *valueText++ = '\0';
      while (*valueText && isspace((unsigned char)*valueText))
         ++valueText;

      char* end = NULL;
      const long value = strtol(valueText, &end, 0);
      if (end == valueText)
         continue;

      if (_stricmp(line, "obj_min") == 0)
      {
         const int candidate = ClampPickerLimit(
            line, value, -DARK_LIMIT_RUNTIME_ABSTRACT_OBJECT_IDS, -1,
            resolved);
         if (candidate < limits->objMin)
            limits->objMin = candidate;
      }
      else if (_stricmp(line, "obj_max") == 0)
      {
         const int candidate = ClampPickerLimit(
            line, value, 1, DARK_LIMIT_RUNTIME_CONCRETE_OBJECT_IDS,
            resolved);
         if (candidate > limits->objMax)
            limits->objMax = candidate;
      }
      else if (_stricmp(line, "max_refs") == 0)
      {
         const int candidate = ClampPickerLimit(
            line, value, 1, DARK_LIMIT_RUNTIME_OBJECT_REFS, resolved);
         if (candidate > limits->maxRefs)
            limits->maxRefs = candidate;
      }
      else if (_stricmp(line, "sfx_channels") == 0)
      {
         const int candidate = ClampPickerLimit(
            line, value, 1, DARK_LIMIT_RUNTIME_SOUND_CHANNELS, resolved);
         if (candidate > limits->soundChannels)
            limits->soundChannels = candidate;
      }
   }
   fclose(file);
}

void DarkContentPreparePickerBootstrap(void)
{
   if (!DarkContentShowPicker())
      return;

   sPickerBootstrapLimits limits = {
      -DARK_LIMIT_LEGACY_ABSTRACT_OBJECT_IDS,
      DARK_LIMIT_LEGACY_CONCRETE_OBJECT_IDS,
      DARK_LIMIT_LEGACY_OBJECT_REFS,
      DARK_LIMIT_LEGACY_SOUND_CHANNELS,
      0,
      0
   };
   config_get_int("obj_min", &limits.objMin);
   config_get_int("obj_max", &limits.objMax);
   config_get_int("max_refs", &limits.maxRefs);
   config_get_int("sfx_channels", &limits.soundChannels);

   char fmRoot[DARK_CONTENT_MAX_PATH];
   char pattern[DARK_CONTENT_MAX_PATH];
   ResolveFMRoot(0, NULL, fmRoot, sizeof(fmRoot));
   JoinPath(pattern, sizeof(pattern), fmRoot, "*");

   static const char* files[] = { "DARK.CFG", "fm.cfg", "cam_ext.cfg" };
   sPlatformFileFind find = {};
   if (PlatformFindFirst(pattern, &find))
   {
      do
      {
         if (!(find.attributes & kPlatformFileDirectory) ||
             find.name[0] == '.')
            continue;
         ++limits.directories;
         char directory[DARK_CONTENT_MAX_PATH];
         JoinPath(directory, sizeof(directory), fmRoot, find.name);
         for (int i = 0; i < sizeof(files) / sizeof(files[0]); ++i)
         {
            char path[DARK_CONTENT_MAX_PATH];
            JoinPath(path, sizeof(path), directory, files[i]);
            ReadPickerLimitFile(path, &limits);
         }
      } while (PlatformFindNext(&find));
      PlatformFindClose(&find);
   }

   // Object bounds are consumed by the deferred dbBuildDefault(). The
   // reference and mixer capacities are consumed during kMsgAppInit, before
   // the content panel can return a selection.
   config_set_int("obj_min", limits.objMin);
   config_set_int("obj_max", limits.objMax);
   config_set_int("max_refs", limits.maxRefs);
   config_set_int("sfx_channels", limits.soundChannels);

   char diagnostic[512];
   _snprintf(diagnostic, sizeof(diagnostic) - 1,
             "themed picker bootstrap scanned %d FM director%s (%d config "
             "file%s): obj_min=%d obj_max=%d max_refs=%d sfx_channels=%d",
             limits.directories, limits.directories == 1 ? "y" : "ies",
             limits.files, limits.files == 1 ? "" : "s", limits.objMin,
             limits.objMax, limits.maxRefs, limits.soundChannels);
   diagnostic[sizeof(diagnostic) - 1] = '\0';
   PlatformDiagnosticsLog("INFO", "content-bootstrap", diagnostic);
   mprintf("%s\n", diagnostic);
}

static void SanitizeComponent(char* text)
{
   char* p;
   for (p = text; *p; ++p)
      if (!isalnum((unsigned char)*p) && *p != '-' && *p != '_')
         *p = '_';
}

static void CreateDirectories(const char* path)
{
   char partial[DARK_CONTENT_MAX_PATH];
   char* p;
   strncpy(partial, path, sizeof(partial));
   partial[sizeof(partial) - 1] = '\0';
#ifdef _WIN32
   for (p = partial + 3; *p; ++p)
      if (*p == '\\' || *p == '/')
      {
         char separator = *p;
         *p = '\0';
         CreateDirectoryA(partial, NULL);
         *p = separator;
      }
   CreateDirectoryA(partial, NULL);
#else
   for (p = partial + (partial[0] == '/' ? 1 : 0); *p; ++p)
      if (*p == '/')
      {
         *p = '\0';
         if (partial[0])
            mkdir(partial, 0755);
         *p = '/';
      }
   mkdir(partial, 0755);
#endif
}

static BOOL DirectoryWritable(const char* path)
{
   char probe[DARK_CONTENT_MAX_PATH];
   char probeName[64];
   FILE* file;
   CreateDirectories(path);
   _snprintf(probeName, sizeof(probeName) - 1,
             ".darkengine-write-test-%u", PlatformMilliseconds());
   probeName[sizeof(probeName) - 1] = '\0';
   JoinPath(probe, sizeof(probe), path, probeName);
   file = fopen(probe, "wbx");
   if (!file)
      return FALSE;
   fclose(file);
   remove(probe);
   return TRUE;
}

static void BuildProfileSavePath(char* output, int size, const char* root,
                                 const char* fmName, BOOL vanilla)
{
   char component[96];
   strncpy(component, fmName && fmName[0] ? fmName :
           (vanilla ? "original-vanilla" : "original-configured"),
           sizeof(component));
   component[sizeof(component) - 1] = '\0';
   SanitizeComponent(component);
#ifdef _WIN32
   _snprintf(output, size,
             "%s\\DarkEngineRestored\\Profiles\\%s\\%s\\saves",
             root, gProfile, component);
#else
   _snprintf(output, size,
             "%s/DarkEngineRestored/Profiles/%s/%s/saves",
             root, gProfile, component);
#endif
   output[size - 1] = '\0';
}

static void ConfigureSavePath(int argc, const char* argv[],
                              const char* fmName, BOOL vanilla)
{
   char userRoot[DARK_CONTENT_MAX_PATH] = "";
#ifndef _WIN32
   char localPath[DARK_CONTENT_MAX_PATH];
   char userPath[DARK_CONTENT_MAX_PATH] = "";
#endif
   const char* requested = CommandValue(argc, argv, "save_path");
   if (!requested || !requested[0])
      requested = getenv("DARK_SAVE_PATH");

   gSaveFallbackPath[0] = '\0';
   gSavePathExplicit = requested && requested[0];
   if (gSavePathExplicit)
   {
      strncpy(gSavePath, requested, sizeof(gSavePath));
      gSavePath[sizeof(gSavePath) - 1] = '\0';
      CreateDirectories(gSavePath);
      return;
   }

#ifdef _WIN32
   if (!GetEnvironmentVariableA("LOCALAPPDATA", userRoot, sizeof(userRoot)))
      GetEnvironmentVariableA("APPDATA", userRoot, sizeof(userRoot));
   if (userRoot[0])
      BuildProfileSavePath(gSavePath, sizeof(gSavePath), userRoot, fmName,
                           vanilla);
   else
      JoinPath(gSavePath, sizeof(gSavePath), gBasePath, "saves");
   CreateDirectories(gSavePath);
   // Keep profile-isolated writes, but allow existing retail saves to remain
   // visible.  DarkContentMakeLoadPath only uses this location when the file
   // is absent from the active profile, so profile saves still take priority.
   JoinPath(gSaveFallbackPath, sizeof(gSaveFallbackPath), gBasePath, "saves");
#else
   JoinPath(localPath, sizeof(localPath), gBasePath, "saves");
   // Retail installations commonly spell this directory "SAVES".  Reuse
   // that existing directory on a case-sensitive filesystem instead of
   // creating a second, empty lowercase sibling that hides the old saves
   // from the compatibility lookup layer.
   char resolvedLocalPath[DARK_CONTENT_MAX_PATH];
   if (PlatformResolvePath(localPath, resolvedLocalPath,
                           sizeof(resolvedLocalPath)))
   {
      strncpy(localPath, resolvedLocalPath, sizeof(localPath));
      localPath[sizeof(localPath) - 1] = '\0';
   }
   const char* xdgData = getenv("XDG_DATA_HOME");
   if (xdgData && xdgData[0])
      strncpy(userRoot, xdgData, sizeof(userRoot));
   else
   {
      const char* home = getenv("HOME");
      if (home && home[0])
         JoinPath(userRoot, sizeof(userRoot), home, ".local/share");
   }
   userRoot[sizeof(userRoot) - 1] = '\0';
   if (userRoot[0])
      BuildProfileSavePath(userPath, sizeof(userPath), userRoot, fmName,
                           vanilla);

   if (DirectoryWritable(localPath))
   {
      strncpy(gSavePath, localPath, sizeof(gSavePath));
      if (userPath[0])
         strncpy(gSaveFallbackPath, userPath, sizeof(gSaveFallbackPath));
   }
   else if (userPath[0] && DirectoryWritable(userPath))
   {
      strncpy(gSavePath, userPath, sizeof(gSavePath));
      strncpy(gSaveFallbackPath, localPath, sizeof(gSaveFallbackPath));
      Warning(("Save directory '%s' is not writable; using '%s'\n",
               localPath, userPath));
   }
   else
   {
      // Preserve a deterministic path and let the ordinary save failure UI
      // report the problem when neither location can be made writable.
      strncpy(gSavePath, localPath, sizeof(gSavePath));
   }
   gSavePath[sizeof(gSavePath) - 1] = '\0';
   gSaveFallbackPath[sizeof(gSaveFallbackPath) - 1] = '\0';
#endif
}

static void PublishPath(const char* variable, eDarkContentPath slot)
{
   config_set_string(variable, gPaths[slot]);
   config_set_priority(variable, PRIORITY_TRANSIENT);
}

static void CaptureSourcePaths(void)
{
   int i;
   if (gSourcePathsCaptured)
      return;
   for (i = 0; i < 5; ++i)
   {
      gSourcePathDefined[i] = config_get_raw(kContentPathVariables[i],
                                             gSourcePaths[i],
                                             sizeof(gSourcePaths[i]));
      gSourcePathPriorities[i] = config_get_priority(kContentPathVariables[i]);
   }
   gSourcePathsCaptured = TRUE;
}

static void RestoreSourcePaths(void)
{
   int i;
   for (i = 0; i < 5; ++i)
   {
      if (!gSourcePathDefined[i])
      {
         config_unset(kContentPathVariables[i]);
         continue;
      }
      config_set_string(kContentPathVariables[i], gSourcePaths[i]);
      config_set_priority(kContentPathVariables[i],
                          gSourcePathPriorities[i]);
   }
}

static void SetSelectionEnvironment(const char* mode, const char* fmName)
{
#ifdef _WIN32
   _putenv_s("DARK_CONTENT_MODE", mode ? mode : "configured");
   _putenv_s("DARK_FM_NAME", fmName ? fmName : "");
   _putenv_s("DARK_CONTENT_PICKER", "");
#else
   setenv("DARK_CONTENT_MODE", mode ? mode : "configured", 1);
   if (fmName && fmName[0])
      setenv("DARK_FM_NAME", fmName, 1);
   else
      unsetenv("DARK_FM_NAME");
   unsetenv("DARK_CONTENT_PICKER");
#endif
}

void DarkContentConfigureProfile(void)
{
   const char* env;
   BOOL profileFromEnvironment = FALSE;
   const char* detected;

   env = getenv("DARK_GAME_PROFILE");
   if (env && env[0])
   {
      strncpy(gProfile, env, sizeof(gProfile));
      gProfile[sizeof(gProfile) - 1] = '\0';
      profileFromEnvironment = TRUE;
   }
   env = getenv("DARK_GAME_PATH");
   if (env && env[0])
   {
      strncpy(gBasePath, env, sizeof(gBasePath));
      gBasePath[sizeof(gBasePath) - 1] = '\0';
   }
   else
      PlatformGetCurrentDirectory(gBasePath, sizeof(gBasePath));
   env = getenv("DARK_COLLECTION_ROOT");
   strncpy(gCollectionRoot, env ? env : "", sizeof(gCollectionRoot));
   gCollectionRoot[sizeof(gCollectionRoot) - 1] = '\0';
   env = getenv("DARK_COLLECTION_GOLD_PATH");
   strncpy(gCollectionGoldPath, env ? env : "", sizeof(gCollectionGoldPath));
   gCollectionGoldPath[sizeof(gCollectionGoldPath) - 1] = '\0';
   env = getenv("DARK_COLLECTION_TDP_PATH");
   strncpy(gCollectionTDPPath, env ? env : "", sizeof(gCollectionTDPPath));
   gCollectionTDPPath[sizeof(gCollectionTDPPath) - 1] = '\0';
   if (!profileFromEnvironment &&
       (detected = DetectProfile(gBasePath)) != NULL)
   {
      strncpy(gProfile, detected, sizeof(gProfile));
      gProfile[sizeof(gProfile) - 1] = '\0';
   }
#ifndef _WIN32
   // The OSM compatibility bridge selects the original game's service
   // vtable layout from this environment value. Windows publishes it in the
   // pre-engine launcher; Linux detects the profile here and must publish the
   // same result before script modules are loaded.
   setenv("DARK_GAME_PROFILE", gProfile, 1);
   setenv("DARK_GAME_PATH", gBasePath, 1);
#endif
   // The restored executable contains Thief II's networking client.  Thief 1
   // installations do not provide the matching network configuration/data.
   if (_stricmp(gProfile, "thief2") != 0)
   {
      config_set_int("no_network", 1);
      config_set_priority("no_network", PRIORITY_TRANSIENT);
   }
}

void DarkContentLoadFMConfig(int argc, const char* argv[])
{
   char fmStorage[CONTENT_PATH_SIZE] = "";
   char fmPath[DARK_CONTENT_MAX_PATH] = "";
   char configPath[DARK_CONTENT_MAX_PATH];
   const char* fmName = SelectedFMName(argc, argv, fmStorage,
                                       sizeof(fmStorage));
   static const char* files[] = { "DARK.CFG", "fm.cfg", "cam_ext.cfg" };
   int i;

   if (!ResolveFMPath(argc, argv, fmName, fmPath, sizeof(fmPath)))
      return;

   for (i = 0; i < sizeof(files) / sizeof(files[0]); ++i)
   {
      JoinPath(configPath, sizeof(configPath), fmPath, files[i]);
      if (FileExists(configPath))
         config_load(configPath);
   }
}

void DarkContentConfigure(int argc, const char* argv[])
{
   char value[CONTENT_PATH_SIZE] = "";
   char movieDir[64] = "Movies";
   char fmStorage[CONTENT_PATH_SIZE] = "";
   const char* fmName;
   const char* requestedMode = CommandValue(argc, argv, "content");
   BOOL vanilla;
   int i;

   // Refresh the environment-derived values for callers outside normal
   // startup; during normal startup this has already run before pick_game().
   DarkContentConfigureProfile();
   CaptureSourcePaths();

   if (!requestedMode)
      requestedMode = getenv("DARK_CONTENT_MODE");
   vanilla = requestedMode && _stricmp(requestedMode, "vanilla") == 0;

   if (requestedMode && !vanilla && _stricmp(requestedMode, "configured") != 0)
      Warning(("Unknown content mode '%s'; using configured content\n", requestedMode));
   strcpy(gContentMode, vanilla ? "vanilla" : "configured");
   ResolveFMRoot(argc, argv, gFMRoot, sizeof(gFMRoot));
   gFMPath[0] = '\0';
   gFMName[0] = '\0';

   fmName = SelectedFMName(argc, argv, fmStorage, sizeof(fmStorage));

   if (fmName)
   {
      if (!ResolveFMPath(argc, argv, fmName, gFMPath, sizeof(gFMPath)))
      {
         Warning(("Active FM '%s' was not found\n", fmName));
         gFMPath[0] = '\0';
         fmName = NULL;
      }
      else
      {
         strncpy(gFMName,fmName,sizeof(gFMName));
         gFMName[sizeof(gFMName) - 1] = '\0';
      }
   }

   for (i = 0; i < 5; ++i)
      gPaths[i][0] = '\0';

   // NewDark-compatible precedence: uber mods, active FM, normal mods, base.
   // Vanilla mode deliberately keeps only the final (retail) entry in each
   // configured base list and suppresses both mod layers.
   if (!vanilla && config_get_raw("uber_mod_path", value, sizeof(value)))
      for (i = 0; i < 5; ++i)
         AddExpandedList(gPaths[i], sizeof(gPaths[i]), value,
                         i == kDarkContentMovies ? movieDir : NULL);

   if (gFMPath[0])
   {
      AddPath(gPaths[kDarkContentResources], sizeof(gPaths[0]), gFMPath);
      AddPath(gPaths[kDarkContentMissions], sizeof(gPaths[0]), gFMPath);
      AddPath(gPaths[kDarkContentScripts], sizeof(gPaths[0]), gFMPath);
      AddPath(gPaths[kDarkContentIncludes], sizeof(gPaths[0]), gFMPath);
      if (config_get_raw("fm_movie_dir", movieDir, sizeof(movieDir)) == FALSE)
         strcpy(movieDir, "Movies");
      {
         char path[DARK_CONTENT_MAX_PATH];
         JoinPath(path, sizeof(path), gFMPath, movieDir);
         AddPath(gPaths[kDarkContentMovies], sizeof(gPaths[0]), path);
      }
   }

   if (!vanilla && config_get_raw("mod_path", value, sizeof(value)))
      for (i = 0; i < 5; ++i)
         AddExpandedList(gPaths[i], sizeof(gPaths[i]), value,
                         i == kDarkContentMovies ? movieDir : NULL);

   if (vanilla)
   {
      AddLastConfigPath(gPaths[kDarkContentResources], sizeof(gPaths[0]), "resname_base");
      AddLastConfigPath(gPaths[kDarkContentMissions], sizeof(gPaths[0]), "load_path");
      AddLastConfigPath(gPaths[kDarkContentScripts], sizeof(gPaths[0]), "script_module_path");
      AddLastConfigPath(gPaths[kDarkContentIncludes], sizeof(gPaths[0]), "include_path");
      AddLastConfigPath(gPaths[kDarkContentMovies], sizeof(gPaths[0]), "movie_path");
   }
   else
   {
      AddConfigPaths(gPaths[kDarkContentResources], sizeof(gPaths[0]), "resname_base");
      AddConfigPaths(gPaths[kDarkContentMissions], sizeof(gPaths[0]), "load_path");
      AddConfigPaths(gPaths[kDarkContentScripts], sizeof(gPaths[0]), "script_module_path");
      AddConfigPaths(gPaths[kDarkContentIncludes], sizeof(gPaths[0]), "include_path");
      AddConfigPaths(gPaths[kDarkContentMovies], sizeof(gPaths[0]), "movie_path");
   }

   // GOG's current Gold collection keeps TFIX.GAM beside its Gold and TDP
   // child installs even though the patched Gold missions reference it. Keep
   // the runnable child first, then use the common folder only as a final
   // database dependency root. It must not override an FM, mod, or child file.
   if (gCollectionRoot[0] &&
       _stricmp(gCollectionRoot, gBasePath) != 0)
      AddPath(gPaths[kDarkContentMissions], sizeof(gPaths[0]),
              gCollectionRoot);

   PublishPath("resname_base", kDarkContentResources);
   PublishPath("load_path", kDarkContentMissions);
   PublishPath("script_module_path", kDarkContentScripts);
   PublishPath("include_path", kDarkContentIncludes);
   PublishPath("movie_path", kDarkContentMovies);
   ConfigureSavePath(argc, argv, fmName, vanilla);
   // Explicit command-line selections supersede the environment inherited
   // from the picker process. Publish the resolved selection so crash reports,
   // diagnostics, and out-of-process script bridges identify the active FM.
   // A no-argument launch is still constructing the engine-owned picker here;
   // SetSelectionEnvironment also clears DARK_CONTENT_PICKER, so leave that
   // bootstrap marker intact until the user has made a selection.
   if (!DarkContentShowPicker())
      SetSelectionEnvironment(gContentMode,gFMName[0] ? gFMName : NULL);

   mprintf("Content profile: %s, mode: %s, base: %s%s%s, saves: %s\n",
           gProfile, gContentMode, gBasePath,
           gFMPath[0] ? ", FM: " : "", gFMPath[0] ? gFMPath : "",
           gSavePath);
}

BOOL DarkContentApplySelection(const char* mode, const char* fmName)
{
   const char* arguments[3] = { "darkengine", NULL, NULL };
   char modeArgument[48];
   char fmArgument[DARK_CONTENT_MAX_PATH + 4];
   int argc = 2;

   if (!mode || (_stricmp(mode, "configured") != 0 &&
                 _stricmp(mode, "vanilla") != 0))
      return FALSE;
   if (fmName && fmName[0] && !IsSafeFMName(fmName))
      return FALSE;

   RestoreSourcePaths();
   _snprintf(modeArgument, sizeof(modeArgument) - 1, "content=%s", mode);
   modeArgument[sizeof(modeArgument) - 1] = '\0';
   arguments[1] = modeArgument;
   if (fmName && fmName[0])
   {
      _snprintf(fmArgument, sizeof(fmArgument) - 1, "fm=%s", fmName);
      fmArgument[sizeof(fmArgument) - 1] = '\0';
      arguments[2] = fmArgument;
      argc = 3;
      {
         char resolved[DARK_CONTENT_MAX_PATH];
         if (!ResolveFMPath(argc, arguments, fmName, resolved,
                            sizeof(resolved)))
            return FALSE;
      }
   }

   SetSelectionEnvironment(mode, fmName);
   DarkContentLoadFMConfig(argc, arguments);
   DarkContentConfigure(argc, arguments);
   return !fmName || !fmName[0] || gFMPath[0] != '\0';
}

const char* DarkContentProfile(void) { return gProfile; }
const char* DarkContentMode(void) { return gContentMode; }
const char* DarkContentBasePath(void) { return gBasePath; }
const char* DarkContentFMRoot(void) { return gFMRoot; }
const char* DarkContentCollectionGoldPath(void)
{
   return gCollectionGoldPath;
}
const char* DarkContentCollectionTDPPath(void)
{
   return gCollectionTDPPath;
}
const char* DarkContentFMPath(void) { return gFMPath; }
const char* DarkContentFMName(void) { return gFMName; }
BOOL DarkContentShowPicker(void)
{
   const char* value = getenv("DARK_CONTENT_PICKER");
   return value && value[0] && strcmp(value, "0") != 0;
}
const char* DarkContentSearchPath(eDarkContentPath path)
{
   return path >= kDarkContentResources && path <= kDarkContentMovies ? gPaths[path] : "";
}
const char* DarkContentSaveDirectory(void) { return gSavePath; }

BOOL DarkContentMakeSavePath(char* destination, int size, const char* filename)
{
   if (!destination || size <= 0 || !filename)
      return FALSE;
   JoinPath(destination, size, gSavePath, filename);
   return destination[0] != '\0';
}

BOOL DarkContentMakeLoadPath(char* destination, int size, const char* filename)
{
   if (!DarkContentMakeSavePath(destination, size, filename))
      return FALSE;
   if (FileExists(destination) || gSavePathExplicit || !gSaveFallbackPath[0])
      return TRUE;

   char fallback[DARK_CONTENT_MAX_PATH];
   JoinPath(fallback, sizeof(fallback), gSaveFallbackPath, filename);
   if (FileExists(fallback))
   {
      strncpy(destination, fallback, size);
      destination[size - 1] = '\0';
   }
   return destination[0] != '\0';
}
