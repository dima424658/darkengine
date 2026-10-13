#ifdef _WIN32
#include <win32_platform.h>
#endif

#include <contentctx.h>
#include <darklimit.h>

#include <config.h>
#include <cfg.h>
#include <mprintf.h>

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <direct.h>
#endif

// Keep one canonical, bounded representation instead of allowing individual
// subsystems to truncate different path lists.
#define CONTENT_PATH_SIZE DARK_LIMIT_RUNTIME_SEARCH_PATH_CHARS

static char gProfile[32] = "thief2";
static char gContentMode[32] = "configured";
static char gBasePath[DARK_CONTENT_MAX_PATH] = ".";
static char gFMPath[DARK_CONTENT_MAX_PATH] = "";
static char gSavePath[DARK_CONTENT_MAX_PATH] = "saves";
static char gPaths[5][CONTENT_PATH_SIZE];

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
   if (!leaf || !leaf[0])
      _snprintf(output, size - 1, "%s", root ? root : "");
   else if (IsAbsolutePath(leaf))
      _snprintf(output, size - 1, "%s", leaf);
   else
   {
      rootLen = root ? strlen(root) : 0;
      _snprintf(output, size - 1, "%s%s%s", root ? root : "",
                rootLen && root[rootLen - 1] != '\\' && root[rootLen - 1] != '/' ? "\\" : "",
                leaf);
   }
   output[size - 1] = '\0';
}

static BOOL DirectoryExists(const char* path)
{
#ifdef _WIN32
   DWORD attributes = GetFileAttributesA(path);
   return attributes != INVALID_FILE_ATTRIBUTES &&
          (attributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
#else
   return FALSE;
#endif
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
   for (token = strtok_s(copy, "+;", &context); token;
        token = strtok_s(NULL, "+;", &context))
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
   for (token = strtok_s(copy, "+;", &context); token;
        token = strtok_s(NULL, "+;", &context))
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
#ifdef _WIN32
   if (!fmName)
      fmName = getenv("DARK_FM_NAME");
#endif
   if (!fmName && config_get_raw("fm", storage, storageSize))
      fmName = storage;
   return IsSafeFMName(fmName) ? fmName : NULL;
}

static BOOL ResolveFMPath(int argc, const char* argv[], const char* fmName,
                          char* result, int resultSize)
{
   char fmBase[CONTENT_PATH_SIZE] = "FMs";
   const char* commandFMPath;
   char fmRoot[DARK_CONTENT_MAX_PATH];
   if (!fmName)
      return FALSE;

   commandFMPath = CommandValue(argc, argv, "fm_path");
   if (commandFMPath)
   {
      strncpy(fmBase, commandFMPath, sizeof(fmBase));
      fmBase[sizeof(fmBase) - 1] = '\0';
   }
   else
      config_get_raw("fm_path", fmBase, sizeof(fmBase));
   JoinPath(fmRoot, sizeof(fmRoot), gBasePath, fmBase);
   JoinPath(result, resultSize, fmRoot, fmName);
   return DirectoryExists(result);
}

static BOOL IsSafeFMName(const char* name)
{
   return name && name[0] && !IsAbsolutePath(name) &&
          !strstr(name, "..") && !strchr(name, '\\') && !strchr(name, '/');
}

static void SanitizeComponent(char* text)
{
   char* p;
   for (p = text; *p; ++p)
      if (!isalnum((unsigned char)*p) && *p != '-' && *p != '_')
         *p = '_';
}

#ifdef _WIN32
static void CreateDirectories(const char* path)
{
   char partial[DARK_CONTENT_MAX_PATH];
   char* p;
   strncpy(partial, path, sizeof(partial));
   partial[sizeof(partial) - 1] = '\0';
   for (p = partial + 3; *p; ++p)
      if (*p == '\\' || *p == '/')
      {
         char separator = *p;
         *p = '\0';
         CreateDirectoryA(partial, NULL);
         *p = separator;
      }
   CreateDirectoryA(partial, NULL);
}
#endif

static void ConfigureSavePath(const char* fmName, BOOL vanilla)
{
   char root[DARK_CONTENT_MAX_PATH] = "";
   char component[96];
#ifdef _WIN32
   if (!GetEnvironmentVariableA("LOCALAPPDATA", root, sizeof(root)))
      GetEnvironmentVariableA("APPDATA", root, sizeof(root));
#endif
   if (!root[0])
   {
      strncpy(gSavePath, "saves", sizeof(gSavePath));
      return;
   }
   strncpy(component, fmName && fmName[0] ? fmName :
           (vanilla ? "original-vanilla" : "original-configured"),
           sizeof(component));
   component[sizeof(component) - 1] = '\0';
   SanitizeComponent(component);
   _snprintf(gSavePath, sizeof(gSavePath) - 1,
             "%s\\OpenDarkEngine\\Profiles\\%s\\%s\\saves",
             root, gProfile, component);
   gSavePath[sizeof(gSavePath) - 1] = '\0';
#ifdef _WIN32
   CreateDirectories(gSavePath);
#endif
}

static void PublishPath(const char* variable, eDarkContentPath slot)
{
   config_set_string(variable, gPaths[slot]);
   config_set_priority(variable, PRIORITY_TRANSIENT);
}

void DarkContentConfigureProfile(void)
{
   const char* env;

#ifdef _WIN32
   env = getenv("DARK_GAME_PROFILE");
   if (env && env[0])
   {
      strncpy(gProfile, env, sizeof(gProfile));
      gProfile[sizeof(gProfile) - 1] = '\0';
   }
   env = getenv("DARK_GAME_PATH");
   if (env && env[0])
   {
      strncpy(gBasePath, env, sizeof(gBasePath));
      gBasePath[sizeof(gBasePath) - 1] = '\0';
   }
   else
      GetCurrentDirectoryA(sizeof(gBasePath), gBasePath);
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
#ifdef _WIN32
      if (GetFileAttributesA(configPath) != INVALID_FILE_ATTRIBUTES)
         config_load(configPath);
#endif
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

#ifdef _WIN32
   if (!requestedMode)
      requestedMode = getenv("DARK_CONTENT_MODE");
#endif
   vanilla = requestedMode && _stricmp(requestedMode, "vanilla") == 0;

   if (requestedMode && !vanilla && _stricmp(requestedMode, "configured") != 0)
      Warning(("Unknown content mode '%s'; using configured content\n", requestedMode));
   strcpy(gContentMode, vanilla ? "vanilla" : "configured");
   gFMPath[0] = '\0';

   fmName = SelectedFMName(argc, argv, fmStorage, sizeof(fmStorage));

   if (fmName)
   {
      if (!ResolveFMPath(argc, argv, fmName, gFMPath, sizeof(gFMPath)))
      {
         Warning(("Active FM '%s' was not found\n", fmName));
         gFMPath[0] = '\0';
         fmName = NULL;
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

   PublishPath("resname_base", kDarkContentResources);
   PublishPath("load_path", kDarkContentMissions);
   PublishPath("script_module_path", kDarkContentScripts);
   PublishPath("include_path", kDarkContentIncludes);
   PublishPath("movie_path", kDarkContentMovies);
   ConfigureSavePath(fmName, vanilla);

   mprintf("Content profile: %s, mode: %s, base: %s%s%s\n",
           gProfile, gContentMode, gBasePath,
           gFMPath[0] ? ", FM: " : "", gFMPath[0] ? gFMPath : "");
}

const char* DarkContentProfile(void) { return gProfile; }
const char* DarkContentMode(void) { return gContentMode; }
const char* DarkContentBasePath(void) { return gBasePath; }
const char* DarkContentFMPath(void) { return gFMPath; }
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
