#include "../win32_platform.h"

#include <platform_services.h>

#include <stdio.h>
#include <cstdio>
#include <stdlib.h>
#include <string.h>
#include <conio.h>
#include <malloc.h>
#include <mutex>
#include <string>
#include <vector>

struct sDisplayModeCollector
{
   sPlatformDisplayMode *modes;
   int capacity;
   int count;
};

struct sWin32FileFind
{
   HANDLE handle;
   WIN32_FIND_DATAA data;
};

static void CopyFileFind(const WIN32_FIND_DATAA *source,
                         sPlatformFileFind *destination)
{
   destination->attributes = 0;
   if (source->dwFileAttributes & FILE_ATTRIBUTE_HIDDEN)
      destination->attributes |= kPlatformFileHidden;
   if (source->dwFileAttributes & FILE_ATTRIBUTE_SYSTEM)
      destination->attributes |= kPlatformFileSystem;
   if (source->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
      destination->attributes |= kPlatformFileDirectory;
   destination->size = ((uint64_t)source->nFileSizeHigh << 32) |
                       source->nFileSizeLow;
   strncpy(destination->name, source->cFileName,
           sizeof(destination->name) - 1);
   destination->name[sizeof(destination->name) - 1] = '\0';
}

static void CollectMonitorModes(HMONITOR monitor, sDisplayModeCollector *collector,
                                int *native_width, int *native_height)
{
   MONITORINFOEXA info = {};
   DEVMODEA mode = {};
   DWORD index;

   info.cbSize = sizeof(info);
   if (!GetMonitorInfoA(monitor, &info))
      return;

   if (native_width)
      *native_width = info.rcMonitor.right - info.rcMonitor.left;
   if (native_height)
      *native_height = info.rcMonitor.bottom - info.rcMonitor.top;

   for (index = 0; collector->count < collector->capacity; ++index)
   {
      int i;
      memset(&mode, 0, sizeof(mode));
      mode.dmSize = sizeof(mode);
      if (!EnumDisplaySettingsExA(info.szDevice, index, &mode, 0))
         break;

      for (i = 0; i < collector->count; ++i)
         if (collector->modes[i].width == (int)mode.dmPelsWidth &&
             collector->modes[i].height == (int)mode.dmPelsHeight &&
             collector->modes[i].bit_depth == (int)mode.dmBitsPerPel)
            break;
      if (i != collector->count)
         continue;

      sPlatformDisplayMode *output = &collector->modes[collector->count++];
      output->width = (int)mode.dmPelsWidth;
      output->height = (int)mode.dmPelsHeight;
      output->bit_depth = (int)mode.dmBitsPerPel;
   }
}

static BOOL CALLBACK CollectAllMonitorModes(HMONITOR monitor, HDC, LPRECT,
                                            LPARAM context)
{
   CollectMonitorModes(monitor, (sDisplayModeCollector *)context, NULL, NULL);
   return TRUE;
}

int PlatformGetDisplayModes(ePlatformDisplayScope scope,
                            sPlatformDisplayMode *modes, int capacity,
                            int *native_width, int *native_height)
{
   sDisplayModeCollector collector = { modes, capacity, 0 };

   if (!modes || capacity < 0)
      collector.capacity = 0;
   if (native_width)
      *native_width = 0;
   if (native_height)
      *native_height = 0;

   if (scope == kPlatformAllDisplays)
   {
      EnumDisplayMonitors(NULL, NULL, CollectAllMonitorModes,
                          (LPARAM)&collector);
   }
   else
   {
      HMONITOR monitor;
      HWND window = GetActiveWindow();
      if (window)
         monitor = MonitorFromWindow(window, MONITOR_DEFAULTTONEAREST);
      else
      {
         POINT cursor = {};
         GetCursorPos(&cursor);
         monitor = MonitorFromPoint(cursor, MONITOR_DEFAULTTOPRIMARY);
      }
      CollectMonitorModes(monitor, &collector, native_width, native_height);
   }

   return collector.count;
}

float PlatformGetTextScale(void)
{
   HWND window = GetActiveWindow();
   UINT dpi = window ? GetDpiForWindow(window) : GetDpiForSystem();
   if (!dpi)
      dpi = 96;
   const float scale = (float)dpi / 96.0f;
   return scale < 0.5f ? 0.5f : (scale > 8.0f ? 8.0f : scale);
}

static uint32 ClampMemorySize(DWORDLONG size)
{
   return size > UINT32_MAX ? UINT32_MAX : (uint32)size;
}

BOOL PlatformGetMemoryStatus(sPlatformMemoryStatus *status)
{
   MEMORYSTATUSEX native = {};

   if (!status)
      return FALSE;
   memset(status, 0, sizeof(*status));
   native.dwLength = sizeof(native);
   if (!GlobalMemoryStatusEx(&native))
      return FALSE;

   status->load_percent = native.dwMemoryLoad;
   status->total_physical_bytes = ClampMemorySize(native.ullTotalPhys);
   status->available_physical_bytes = ClampMemorySize(native.ullAvailPhys);
   status->total_page_file_bytes = ClampMemorySize(native.ullTotalPageFile);
   status->available_page_file_bytes = ClampMemorySize(native.ullAvailPageFile);
   status->total_virtual_bytes = ClampMemorySize(native.ullTotalVirtual);
   status->available_virtual_bytes = ClampMemorySize(native.ullAvailVirtual);
   return TRUE;
}

static BOOL MakeUserKey(char *key, size_t size, const char *store)
{
   int count;
   if (!key || !size || !store || !store[0])
      return FALSE;
   count = snprintf(key, size, "Software\\DarkEngineRestored\\%s", store);
   return count > 0 && (size_t)count < size;
}

BOOL PlatformReadUserUInt(const char *store, const char *name, uint32 *value)
{
   char key_name[256];
   HKEY key;
   DWORD type = REG_DWORD;
   DWORD size = sizeof(*value);
   BOOL result;

   if (!name || !value || !MakeUserKey(key_name, sizeof(key_name), store) ||
       RegOpenKeyExA(HKEY_CURRENT_USER, key_name, 0, KEY_QUERY_VALUE, &key) !=
          ERROR_SUCCESS)
      return FALSE;

   result = RegQueryValueExA(key, name, NULL, &type, (BYTE *)value, &size) ==
               ERROR_SUCCESS &&
            type == REG_DWORD && size == sizeof(*value);
   RegCloseKey(key);
   return result;
}

BOOL PlatformWriteUserUInt(const char *store, const char *name, uint32 value)
{
   char key_name[256];
   HKEY key;
   DWORD disposition;
   LONG result;

   if (!name || !MakeUserKey(key_name, sizeof(key_name), store) ||
       RegCreateKeyExA(HKEY_CURRENT_USER, key_name, 0, NULL, 0, KEY_SET_VALUE,
                       NULL, &key, &disposition) != ERROR_SUCCESS)
      return FALSE;

   result = RegSetValueExA(key, name, 0, REG_DWORD, (const BYTE *)&value,
                           sizeof(value));
   RegCloseKey(key);
   return result == ERROR_SUCCESS;
}

const char *PlatformGetLanguageCode(void)
{
   switch (PRIMARYLANGID(GetSystemDefaultLangID()))
   {
      case LANG_FRENCH:  return "fr";
      case LANG_GERMAN:  return "de";
      case LANG_ITALIAN: return "it";
      case LANG_SPANISH: return "es";
      default:           return "en";
   }
}

void PlatformShowError(const char *title, const char *message)
{
   MessageBoxA(NULL, message ? message : "", title ? title : "Dark Engine",
               MB_OK | MB_ICONERROR);
}

BOOL PlatformAskYesNo(const char *title, const char *message)
{
   return MessageBoxA(NULL, message ? message : "",
                      title ? title : "Dark Engine",
                      MB_YESNO | MB_ICONQUESTION | MB_SYSTEMMODAL) == IDYES;
}

int PlatformPopupMenu(const char *const *items, int count,
                      int client_x, int client_y)
{
   HWND window = GetActiveWindow();
   HMENU menu = CreatePopupMenu();
   POINT screen_pos = { client_x, client_y };
   UINT selection;

   if (!window || !menu)
   {
      if (menu)
         DestroyMenu(menu);
      return 0;
   }

   for (int i = 0; i < count; ++i)
      AppendMenuA(menu, MF_STRING, (UINT_PTR)(i + 1), items[i]);

   ClientToScreen(window, &screen_pos);
   selection = TrackPopupMenu(menu,
                              TPM_RETURNCMD | TPM_NONOTIFY | TPM_RIGHTBUTTON,
                              screen_pos.x, screen_pos.y, 0, window, NULL);
   DestroyMenu(menu);
   return (int)selection;
}

uint32 PlatformMilliseconds(void)
{
   return (uint32)GetTickCount();
}

uint64_t PlatformCurrentThreadId(void)
{
   return (uint64_t)GetCurrentThreadId();
}

int PlatformReadKey(void)
{
   return _getch();
}

size_t PlatformAllocationSize(void *allocation)
{
   return allocation ? _msize(allocation) : 0;
}

void PlatformHeapCheck(void)
{
   _heapchk();
}

void PlatformHeapMinimize(void)
{
   _heapmin();
}

void *PlatformLoadLibrary(const char *name)
{
   return (void *)LoadLibraryA(name);
}

void *PlatformFindSymbol(void *library, const char *name)
{
   return library ? (void *)GetProcAddress((HMODULE)library, name) : NULL;
}

void PlatformUnloadLibrary(void *library)
{
   if (library)
      FreeLibrary((HMODULE)library);
}

void PlatformDebugOutput(const char *text)
{
   OutputDebugStringA(text ? text : "");
}

static void PlatformDiagnosticTimestamp(char *buffer, size_t size,
                                        bool milliseconds)
{
   SYSTEMTIME now = {};
   GetLocalTime(&now);
   if (milliseconds)
      std::snprintf(buffer, size, "%02u:%02u:%02u.%03u",
                    now.wHour, now.wMinute, now.wSecond, now.wMilliseconds);
   else
      std::snprintf(buffer, size, "%04u-%02u-%02u %02u:%02u:%02u",
                    now.wYear, now.wMonth, now.wDay, now.wHour, now.wMinute,
                    now.wSecond);
}

static const char *PlatformDiagnosticArchitecture(void)
{
#if defined(_M_X64)
   return "x64";
#else
   return "x86";
#endif
}

static unsigned long long PlatformDiagnosticProcessId(void)
{
   return GetCurrentProcessId();
}

#include <platform_diagnostics.inl>

void PlatformDiagnosticsLogStack(const char *severity, const char *subsystem,
                                 int skip_frames)
{
   void *frames[24] = {};
   const USHORT count = CaptureStackBackTrace(
      (ULONG)(skip_frames < 0 ? 0 : skip_frames + 1),
      (ULONG)(sizeof(frames) / sizeof(frames[0])),frames,NULL);
   const uintptr_t imageBase = (uintptr_t)GetModuleHandleA(NULL);
   char message[1024];
   int used = _snprintf(message,sizeof(message) - 1,
                        "stack image-base=%p",(void *)imageBase);
   if (used < 0)
      used = 0;
   for (USHORT i = 0; i < count && used < (int)sizeof(message) - 32; ++i)
   {
      const uintptr_t address = (uintptr_t)frames[i];
      const uintptr_t relative = address >= imageBase
         ? address - imageBase : address;
      const int written = _snprintf(message + used,sizeof(message) - used - 1,
                                    " %p(+%p)",(void *)address,
                                    (void *)relative);
      if (written <= 0)
         break;
      used += written;
   }
   message[sizeof(message) - 1] = '\0';
   PlatformDiagnosticsLog(severity,subsystem,message);
}

ePlatformAssertAction PlatformShowAssert(const char *message)
{
   int priority = GetThreadPriority(GetCurrentThread());
   SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
   int result = MessageBoxA(NULL, message ? message : "", "Assertion Failed",
                            MB_SYSTEMMODAL | MB_ICONHAND | MB_YESNOCANCEL);
   SetThreadPriority(GetCurrentThread(), priority);
   if (result == IDYES)
      return kPlatformAssertDebug;
   if (result == IDNO)
      return kPlatformAssertExit;
   return kPlatformAssertIgnore;
}

void PlatformDebugBreak(void)
{
   DebugBreak();
}

void PlatformExitProcess(int status)
{
   ExitProcess((UINT)status);
}

BOOL PlatformSetClipboardText(const char *text)
{
   size_t length = text ? strlen(text) : 0;
   HGLOBAL memory = GlobalAlloc(GMEM_MOVEABLE, length + 1);
   if (!memory)
      return FALSE;
   char *destination = (char *)GlobalLock(memory);
   if (!destination)
   {
      GlobalFree(memory);
      return FALSE;
   }
   if (length)
      memcpy(destination, text, length);
   destination[length] = '\0';
   GlobalUnlock(memory);
   if (!OpenClipboard(NULL))
   {
      GlobalFree(memory);
      return FALSE;
   }
   EmptyClipboard();
   if (!SetClipboardData(CF_TEXT, memory))
   {
      CloseClipboard();
      GlobalFree(memory);
      return FALSE;
   }
   CloseClipboard();
   return TRUE;
}

int PlatformGetClipboardText(char *text, int capacity)
{
   int length = 0;
   if (!text || capacity <= 0 || !OpenClipboard(NULL))
      return 0;
   HANDLE data = GetClipboardData(CF_TEXT);
   const char *source = data ? (const char *)GlobalLock(data) : NULL;
   if (source)
   {
      while (source[length] && length < capacity - 1)
      {
         text[length] = source[length];
         ++length;
      }
      text[length] = '\0';
      GlobalUnlock(data);
   }
   CloseClipboard();
   return length;
}

void PlatformSetTextInputActive(BOOL) {}

BOOL PlatformGetCurrentDirectory(char *path, int capacity)
{
   DWORD length;
   if (!path || capacity <= 0)
      return FALSE;
   length = GetCurrentDirectoryA((DWORD)capacity, path);
   return length > 0 && length < (DWORD)capacity;
}

BOOL PlatformGetExecutableDirectory(char *path, int capacity)
{
   if (!path || capacity <= 0)
      return FALSE;
   const DWORD length = GetModuleFileNameA(NULL,path,(DWORD)capacity);
   if (!length || length >= (DWORD)capacity)
   {
      path[0] = '\0';
      return FALSE;
   }
   char *separator = strrchr(path,'\\');
   char *alternate = strrchr(path,'/');
   if (alternate && (!separator || alternate > separator))
      separator = alternate;
   if (!separator)
   {
      path[0] = '\0';
      return FALSE;
   }
   *separator = '\0';
   return TRUE;
}

static std::wstring PlatformUtf8ToWide(const char *text)
{
   if (!text)
      return std::wstring();
   int count = MultiByteToWideChar(CP_UTF8,0,text,-1,NULL,0);
   UINT page = CP_UTF8;
   if (!count)
   {
      page = CP_ACP;
      count = MultiByteToWideChar(page,0,text,-1,NULL,0);
   }
   if (!count)
      return std::wstring();
   std::vector<wchar_t> buffer(count);
   MultiByteToWideChar(page,0,text,-1,buffer.data(),count);
   return std::wstring(buffer.data());
}

static void PlatformAppendQuotedArgument(std::wstring& command,
                                         const std::wstring& argument)
{
   if (!command.empty())
      command.push_back(L' ');
   const BOOL quote = argument.empty() ||
      argument.find_first_of(L" \t\"") != std::wstring::npos;
   if (!quote)
   {
      command += argument;
      return;
   }
   command.push_back(L'\"');
   size_t slashes = 0;
   for (size_t i = 0; i < argument.size(); ++i)
   {
      const wchar_t ch = argument[i];
      if (ch == L'\\')
      {
         ++slashes;
         continue;
      }
      if (ch == L'\"')
      {
         command.append(slashes * 2 + 1,L'\\');
         command.push_back(L'\"');
      }
      else
      {
         command.append(slashes,L'\\');
         command.push_back(ch);
      }
      slashes = 0;
   }
   command.append(slashes * 2,L'\\');
   command.push_back(L'\"');
}

BOOL PlatformRunProcess(const char *executable,
                        const char *const *arguments,
                        int argument_count,
                        const char *working_directory,
                        tPlatformProcessOutput output,
                        void *context,
                        uint32 *exit_code)
{
   if (exit_code)
      *exit_code = UINT32_MAX;
   if (!executable || !executable[0] || argument_count < 0)
      return FALSE;

   const std::wstring application = PlatformUtf8ToWide(executable);
   const std::wstring directory = PlatformUtf8ToWide(working_directory);
   if (application.empty())
      return FALSE;

   std::wstring command;
   PlatformAppendQuotedArgument(command,application);
   for (int i = 0; i < argument_count; ++i)
      PlatformAppendQuotedArgument(command,
         PlatformUtf8ToWide(arguments && arguments[i] ? arguments[i] : ""));
   std::vector<wchar_t> mutableCommand(command.begin(),command.end());
   mutableCommand.push_back(L'\0');

   SECURITY_ATTRIBUTES security = { sizeof(security), NULL, TRUE };
   HANDLE readPipe = NULL;
   HANDLE writePipe = NULL;
   if (!CreatePipe(&readPipe,&writePipe,&security,0))
      return FALSE;
   SetHandleInformation(readPipe,HANDLE_FLAG_INHERIT,0);

   STARTUPINFOW startup = {};
   startup.cb = sizeof(startup);
   startup.dwFlags = STARTF_USESTDHANDLES;
   startup.hStdOutput = writePipe;
   startup.hStdError = writePipe;
   startup.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
   PROCESS_INFORMATION process = {};
   const BOOL created = CreateProcessW(application.c_str(),
      mutableCommand.data(),NULL,NULL,TRUE,CREATE_NO_WINDOW,NULL,
      directory.empty() ? NULL : directory.c_str(),&startup,&process);
   CloseHandle(writePipe);
   if (!created)
   {
      CloseHandle(readPipe);
      return FALSE;
   }

   char buffer[4096];
   BOOL running = TRUE;
   while (running)
   {
      DWORD available = 0;
      while (PeekNamedPipe(readPipe,NULL,0,NULL,&available,NULL) && available)
      {
         DWORD read = 0;
         DWORD request = available < sizeof(buffer) ? available
                                                    : sizeof(buffer);
         if (!ReadFile(readPipe,buffer,request,&read,NULL) || !read)
            break;
         if (output)
            output(buffer,(int)read,context);
      }
      running = WaitForSingleObject(process.hProcess,25) == WAIT_TIMEOUT;
   }
   for (;;)
   {
      DWORD read = 0;
      if (!ReadFile(readPipe,buffer,sizeof(buffer),&read,NULL) || !read)
         break;
      if (output)
         output(buffer,(int)read,context);
   }

   DWORD nativeExit = UINT32_MAX;
   GetExitCodeProcess(process.hProcess,&nativeExit);
   if (exit_code)
      *exit_code = (uint32)nativeExit;
   CloseHandle(readPipe);
   CloseHandle(process.hThread);
   CloseHandle(process.hProcess);
   return TRUE;
}

BOOL PlatformRelaunchSelf(const char *const *arguments,
                          int argument_count,
                          const char *working_directory)
{
   if (argument_count < 0)
      return FALSE;

   wchar_t executable[MAX_PATH];
   const DWORD length = GetModuleFileNameW(NULL,executable,MAX_PATH);
   if (!length || length >= MAX_PATH)
      return FALSE;

   std::wstring command;
   PlatformAppendQuotedArgument(command,executable);
   for (int i = 0; i < argument_count; ++i)
      PlatformAppendQuotedArgument(command,
         PlatformUtf8ToWide(arguments && arguments[i] ? arguments[i] : ""));
   std::vector<wchar_t> mutableCommand(command.begin(),command.end());
   mutableCommand.push_back(L'\0');

   const std::wstring directory = PlatformUtf8ToWide(working_directory);
   STARTUPINFOW startup = {};
   startup.cb = sizeof(startup);
   PROCESS_INFORMATION process = {};
   const BOOL created = CreateProcessW(executable,mutableCommand.data(),
      NULL,NULL,FALSE,0,NULL,directory.empty() ? NULL : directory.c_str(),
      &startup,&process);
   if (!created)
      return FALSE;
   CloseHandle(process.hThread);
   CloseHandle(process.hProcess);
   return TRUE;
}

BOOL PlatformResolvePath(const char *path, char *resolved, int capacity)
{
   if (!path || !resolved || capacity <= 0 ||
       strlen(path) >= (size_t)capacity)
   {
      if (resolved && capacity > 0)
         resolved[0] = '\0';
      return FALSE;
   }
   strcpy(resolved, path);
   return GetFileAttributesA(path) != INVALID_FILE_ATTRIBUTES;
}

BOOL PlatformCopyFiles(const char *pattern, const char *source_dir,
                       const char *destination_dir)
{
   char search[MAX_PATH];
   char source[MAX_PATH];
   char destination[MAX_PATH];
   WIN32_FIND_DATAA data;
   HANDLE find;
   BOOL result = TRUE;

   if (!pattern || !source_dir || !destination_dir ||
       snprintf(search, sizeof(search), "%s\\%s", source_dir, pattern) < 0)
      return FALSE;
   find = FindFirstFileA(search, &data);
   if (find == INVALID_HANDLE_VALUE)
      return FALSE;
   do
   {
      if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
         continue;
      if (snprintf(source, sizeof(source), "%s\\%s", source_dir,
                   data.cFileName) < 0 ||
          snprintf(destination, sizeof(destination), "%s\\%s",
                   destination_dir, data.cFileName) < 0 ||
          !CopyFileA(source, destination, FALSE))
         result = FALSE;
   } while (FindNextFileA(find, &data));
   if (GetLastError() != ERROR_NO_MORE_FILES)
      result = FALSE;
   FindClose(find);
   return result;
}

BOOL PlatformFindFirst(const char *pattern, sPlatformFileFind *find)
{
   if (!pattern || !find)
      return FALSE;
   sWin32FileFind *native =
      (sWin32FileFind *)malloc(sizeof(sWin32FileFind));
   if (!native)
      return FALSE;
   native->handle = FindFirstFileA(pattern, &native->data);
   if (native->handle == INVALID_HANDLE_VALUE)
   {
      free(native);
      return FALSE;
   }
   find->implementation = native;
   CopyFileFind(&native->data, find);
   return TRUE;
}

BOOL PlatformFindNext(sPlatformFileFind *find)
{
   sWin32FileFind *native = find
      ? (sWin32FileFind *)find->implementation : NULL;
   if (!native || !FindNextFileA(native->handle, &native->data))
      return FALSE;
   CopyFileFind(&native->data, find);
   return TRUE;
}

void PlatformFindClose(sPlatformFileFind *find)
{
   sWin32FileFind *native = find
      ? (sWin32FileFind *)find->implementation : NULL;
   if (native)
   {
      FindClose(native->handle);
      free(native);
      find->implementation = NULL;
   }
}

BOOL PlatformMutexInit(sPlatformMutex *mutex)
{
   if (!mutex)
      return FALSE;
   CRITICAL_SECTION *section =
      (CRITICAL_SECTION *)malloc(sizeof(CRITICAL_SECTION));
   if (!section)
      return FALSE;
   InitializeCriticalSection(section);
   mutex->implementation = section;
   return TRUE;
}

void PlatformMutexTerm(sPlatformMutex *mutex)
{
   if (mutex && mutex->implementation)
   {
      CRITICAL_SECTION *section =
         (CRITICAL_SECTION *)mutex->implementation;
      DeleteCriticalSection(section);
      free(section);
      mutex->implementation = NULL;
   }
}

void PlatformMutexLock(sPlatformMutex *mutex)
{
   EnterCriticalSection((CRITICAL_SECTION *)mutex->implementation);
}

void PlatformMutexUnlock(sPlatformMutex *mutex)
{
   LeaveCriticalSection((CRITICAL_SECTION *)mutex->implementation);
}

long PlatformAtomicLoad(volatile long *value)
{
   return InterlockedCompareExchange(value, 0, 0);
}

long PlatformAtomicIncrement(volatile long *value)
{
   return InterlockedIncrement(value);
}

long PlatformAtomicDecrement(volatile long *value)
{
   return InterlockedDecrement(value);
}

long PlatformAtomicExchange(volatile long *value, long replacement)
{
   return InterlockedExchange(value, replacement);
}

long PlatformAtomicAdd(volatile long *value, long amount)
{
   return InterlockedExchangeAdd(value, amount);
}

long PlatformAtomicCompareExchange(volatile long *value, long replacement,
                                   long expected)
{
   return InterlockedCompareExchange(value, replacement, expected);
}
