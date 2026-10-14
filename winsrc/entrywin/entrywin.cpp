///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/winsrc/entrywin/RCS/entrywin.cpp $
// $Author: TOML $
// $Date: 1996/12/12 15:44:01 $
// $Revision: 1.4 $
//
//
// Main entry point for windowed Windows applications
//

#ifdef _WIN32

#include <win32_platform.h>
#include <dbghelp.h>
#include <shlobj.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <lg.h>

#include "launchmenu.h"

#pragma comment(lib, "dbghelp.lib")

EXTERN int LGAPI _AppMain(int argc, const char* argv[]);
EXTERN int LGAPI HeapInit();
extern "C" void* g_pMalloc;

extern "C"
{
	int _g_referenceEntryPoint = 0;
}

static BOOL UsesDarkData(const char* executable)
{
    const char* name = executable;
    const char* slash;

    if (!executable)
        return FALSE;

    slash = strrchr(executable, '\\');
    if (slash)
        name = slash + 1;
    slash = strrchr(name, '/');
    if (slash)
        name = slash + 1;

    return _strnicmp(name, "thief2", 6) == 0 ||
           _strnicmp(name, "dromed", 6) == 0;
}

static BOOL IsDromEdExecutable(const char* executable)
{
    const char* name = executable ? executable : "";
    const char* slash = strrchr(name, '\\');
    if (slash)
        name = slash + 1;
    slash = strrchr(name, '/');
    if (slash)
        name = slash + 1;
    return _strnicmp(name, "dromed", 6) == 0;
}

static void SetPickerEnvironment(BOOL enabled)
{
    SetEnvironmentVariableA("DARK_CONTENT_PICKER", enabled ? "1" : NULL);
    _putenv_s("DARK_CONTENT_PICKER", enabled ? "1" : "");
    if (enabled)
    {
        SetEnvironmentVariableA("DARK_CONTENT_MODE", "vanilla");
        _putenv_s("DARK_CONTENT_MODE", "vanilla");
    }
}

static BOOL FileExistsInDirectory(const char* directory, const char* name)
{
    char path[MAX_PATH];
    DWORD attributes;

    if (!directory || !directory[0])
        return FALSE;

    _snprintf(path, sizeof(path) - 1, "%s\\%s", directory, name);
    path[sizeof(path) - 1] = '\0';
    attributes = GetFileAttributesA(path);
    return attributes != INVALID_FILE_ATTRIBUTES &&
           !(attributes & FILE_ATTRIBUTE_DIRECTORY);
}

static const char* DetectGameProfile(const char* directory)
{
    if (!FileExistsInDirectory(directory, "cam.cfg") ||
        (!FileExistsInDirectory(directory, "DARK.GAM") &&
         !FileExistsInDirectory(directory, "MISS1.MIS")))
        return NULL;

    // Gold adds mission 18.  Thief II has mission 8, while the original
    // Dark Project does not.  These loose mission files are present in the
    // supported retail installs and avoid depending on localized config text.
    if (FileExistsInDirectory(directory, "MISS18.MIS"))
        return "gold";
    if (FileExistsInDirectory(directory, "MISS8.MIS"))
        return "thief2";
    return "tdp";
}

static int CALLBACK SelectFolderCallback(HWND window, UINT message,
                                         LPARAM, LPARAM initialDirectory)
{
    if (message == BFFM_INITIALIZED && initialDirectory)
        SendMessageA(window, BFFM_SETSELECTIONA, TRUE, initialDirectory);
    return 0;
}

static BOOL PromptForDirectory(char* directory, DWORD size, const char* title)
{
    const HRESULT comResult = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    BROWSEINFOA browse = {};
    char displayName[MAX_PATH] = {};
    browse.hwndOwner = GetActiveWindow();
    browse.pszDisplayName = displayName;
    browse.lpszTitle = title;
    browse.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
    browse.lpfn = SelectFolderCallback;
    browse.lParam = (LPARAM)directory;

    LPITEMIDLIST selected = SHBrowseForFolderA(&browse);
    if (!selected)
    {
        if (SUCCEEDED(comResult))
            CoUninitialize();
        return FALSE;
    }
    const BOOL result = SHGetPathFromIDListA(selected, directory);
    CoTaskMemFree(selected);
    if (SUCCEEDED(comResult))
        CoUninitialize();
    if (result)
        directory[size - 1] = '\0';
    return result;
}

static const char* const kLauncherRegistryKey =
    "Software\\OpenDarkEngine\\Launcher";
static const char* const kLastGameFolderValue = "LastGameFolder";

static BOOL LoadLastGameFolder(char* directory, DWORD size)
{
    HKEY key = NULL;
    DWORD type = 0;
    DWORD bytes = size;
    LONG result = RegOpenKeyExA(HKEY_CURRENT_USER, kLauncherRegistryKey, 0,
                                KEY_QUERY_VALUE, &key);
    if (result != ERROR_SUCCESS)
        return FALSE;

    result = RegQueryValueExA(key, kLastGameFolderValue, NULL, &type,
                              reinterpret_cast<BYTE*>(directory), &bytes);
    RegCloseKey(key);
    if (result != ERROR_SUCCESS || (type != REG_SZ && type != REG_EXPAND_SZ) ||
        !bytes)
        return FALSE;

    directory[size - 1] = '\0';
    const DWORD attributes = GetFileAttributesA(directory);
    return attributes != INVALID_FILE_ATTRIBUTES &&
           (attributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
}

static void SaveLastGameFolder(const char* directory)
{
    HKEY key = NULL;
    DWORD disposition = 0;
    if (RegCreateKeyExA(HKEY_CURRENT_USER, kLauncherRegistryKey, 0, NULL, 0,
                        KEY_SET_VALUE, NULL, &key, &disposition) == ERROR_SUCCESS)
    {
        RegSetValueExA(key, kLastGameFolderValue, 0, REG_SZ,
                       reinterpret_cast<const BYTE*>(directory),
                       static_cast<DWORD>(strlen(directory) + 1));
        RegCloseKey(key);
    }
}

static BOOL HasArgument(int argc, const char* argv[], const char* name)
{
    const size_t nameLength = strlen(name);
    for (int i = 1; i < argc; ++i)
        if (_strnicmp(argv[i], name, nameLength) == 0 &&
            (argv[i][nameLength] == '=' || argv[i][nameLength] == '\0'))
            return TRUE;
    return FALSE;
}

static int SelectContent(const char* directory, const char* profile,
                         const char* executable)
{
    char fmName[MAX_PATH] = "";
    const int choice = DarkShowLaunchMenu(directory, profile, executable,
                                           fmName, sizeof(fmName));
    if (choice == kDarkLaunchConfigured || choice == kDarkLaunchVanilla)
    {
        const char* mode = choice == kDarkLaunchConfigured ?
                           "configured" : "vanilla";
        SetEnvironmentVariableA("DARK_CONTENT_MODE", mode);
        SetEnvironmentVariableA("DARK_FM_NAME", NULL);
        _putenv_s("DARK_CONTENT_MODE", mode);
        _putenv_s("DARK_FM_NAME", "");
        return choice;
    }
    if (choice == kDarkLaunchFanMission)
    {
        SetEnvironmentVariableA("DARK_CONTENT_MODE", "configured");
        SetEnvironmentVariableA("DARK_FM_NAME", fmName);
        _putenv_s("DARK_CONTENT_MODE", "configured");
        _putenv_s("DARK_FM_NAME", fmName);
    }
    return choice;
}

static BOOL PrepareDarkDataDirectory(int argc, const char* argv[])
{
    char directory[MAX_PATH];
    const char* profile = NULL;
    const BOOL interactive = argc == 1 &&
        !HasArgument(argc, argv, "content") &&
        !HasArgument(argc, argv, "fm");

    if (!GetCurrentDirectoryA(sizeof(directory), directory))
        return FALSE;
    profile = DetectGameProfile(directory);
    if (!profile)
        LoadLastGameFolder(directory, sizeof(directory));
    while (!profile)
    {
        if (!PromptForDirectory(directory, sizeof(directory),
                "Select a Thief: The Dark Project, Thief Gold, or Thief II folder"))
            return FALSE;
        profile = DetectGameProfile(directory);
        if (!profile)
            MessageBoxA(NULL,
                        "That folder is not a recognizable Thief installation. Select a folder containing cam.cfg and the retail mission data.",
                        "Thief game data required", MB_OK | MB_ICONERROR);
    }

    for (;;)
    {
        SaveLastGameFolder(directory);
        if (!SetCurrentDirectoryA(directory))
            return FALSE;
        SetEnvironmentVariableA("DARK_GAME_PROFILE", profile);
        SetEnvironmentVariableA("DARK_GAME_PATH", directory);
        // The legacy runtime uses getenv(), whose MSVC environment snapshot is
        // not refreshed by SetEnvironmentVariableA.
        _putenv_s("DARK_GAME_PROFILE", profile);
        _putenv_s("DARK_GAME_PATH", directory);

        // Explicit command lines remain non-interactive for shortcuts and
        // unattended tests. Thief uses the engine-owned content panel;
        // DromEd retains its pre-engine chooser because it has a different
        // primordial mode and lifecycle.
        if (!interactive)
        {
            SetPickerEnvironment(FALSE);
            return TRUE;
        }
        if (!IsDromEdExecutable(argc > 0 ? argv[0] : NULL))
        {
            SetPickerEnvironment(TRUE);
            return TRUE;
        }

        const int choice = SelectContent(directory, profile,
                                         argc > 0 ? argv[0] : NULL);
        if (choice == kDarkLaunchCancel)
            return FALSE;
        if (choice != kDarkLaunchChooseGame)
            return TRUE;

        char selected[MAX_PATH];
        strncpy(selected, directory, sizeof(selected));
        selected[sizeof(selected) - 1] = '\0';
        for (;;)
        {
            if (!PromptForDirectory(selected, sizeof(selected),
                    "Select a Thief: The Dark Project, Thief Gold, or Thief II folder"))
                break;
            const char* selectedProfile = DetectGameProfile(selected);
            if (selectedProfile)
            {
                strncpy(directory, selected, sizeof(directory));
                directory[sizeof(directory) - 1] = '\0';
                profile = selectedProfile;
                break;
            }
            MessageBoxA(NULL,
                        "That folder is not a recognizable Thief installation. Select a folder containing cam.cfg and the retail mission data.",
                        "Thief game data required", MB_OK | MB_ICONERROR);
        }
    }
}

static void WriteCrashLine(HANDLE output, const char* text)
{
    DWORD written;
    if (output && output != INVALID_HANDLE_VALUE)
        WriteFile(output, text, (DWORD)strlen(text), &written, NULL);
}

static LONG WINAPI DarkUnhandledExceptionFilter(EXCEPTION_POINTERS* exceptionInfo)
{
    HANDLE process = GetCurrentProcess();
    HANDLE output = GetStdHandle(STD_ERROR_HANDLE);
    HANDLE log = CreateFileA("darkengine-crash.log", FILE_APPEND_DATA,
                             FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                             OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    char line[1024];

    SYSTEMTIME now;
    GetLocalTime(&now);
    wsprintfA(line, "\r\n[%04u-%02u-%02u %02u:%02u:%02u] ",
              now.wYear, now.wMonth, now.wDay,
              now.wHour, now.wMinute, now.wSecond);
    WriteCrashLine(output, line);
    WriteCrashLine(log, line);

    wsprintfA(line, "Unhandled exception %08X at %p\r\n",
              exceptionInfo->ExceptionRecord->ExceptionCode,
              exceptionInfo->ExceptionRecord->ExceptionAddress);
    WriteCrashLine(output, line);
    WriteCrashLine(log, line);
    if (exceptionInfo->ExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION &&
        exceptionInfo->ExceptionRecord->NumberParameters >= 2)
    {
#if defined(_M_X64)
        wsprintfA(line, "  access=%s address=%p rax=%p rcx=%p rdx=%p rsp=%p rbp=%p\r\n",
                  exceptionInfo->ExceptionRecord->ExceptionInformation[0] ? "write" : "read",
                  (void *)exceptionInfo->ExceptionRecord->ExceptionInformation[1],
                  (void *)exceptionInfo->ContextRecord->Rax,
                  (void *)exceptionInfo->ContextRecord->Rcx,
                  (void *)exceptionInfo->ContextRecord->Rdx,
                  (void *)exceptionInfo->ContextRecord->Rsp,
                  (void *)exceptionInfo->ContextRecord->Rbp);
#else
        wsprintfA(line, "  access=%s address=%p eax=%08X ecx=%08X edx=%08X esp=%08X ebp=%08X\r\n",
                  exceptionInfo->ExceptionRecord->ExceptionInformation[0] ? "write" : "read",
                  (void *)exceptionInfo->ExceptionRecord->ExceptionInformation[1],
                  exceptionInfo->ContextRecord->Eax,
                  exceptionInfo->ContextRecord->Ecx,
                  exceptionInfo->ContextRecord->Edx,
                  exceptionInfo->ContextRecord->Esp,
                  exceptionInfo->ContextRecord->Ebp);
#endif
        WriteCrashLine(output, line);
        WriteCrashLine(log, line);
    }

    // Record the simple frame-pointer chain before asking DbgHelp to walk it.
    // This remains useful when an exception occurs in a system DLL or when
    // StackWalk64 cannot recover an optimized frame.
#if defined(_M_X64)
    ULONG_PTR rawFrame = (ULONG_PTR)exceptionInfo->ContextRecord->Rbp;
#else
    ULONG_PTR rawFrame = (ULONG_PTR)exceptionInfo->ContextRecord->Ebp;
#endif
    for (int depth = 0; depth < 32 && rawFrame; ++depth)
    {
        ULONG_PTR frameWords[2] = {};
        SIZE_T bytesRead = 0;
        if (!ReadProcessMemory(process, (const void*)rawFrame, frameWords,
                               sizeof(frameWords), &bytesRead) ||
            bytesRead != sizeof(frameWords) || !frameWords[1])
            break;
        wsprintfA(line, "  #F%02d %p\r\n", depth, (void*)frameWords[1]);
        WriteCrashLine(output, line);
        WriteCrashLine(log, line);
        if (frameWords[0] <= rawFrame || frameWords[0] - rawFrame > 0x100000)
            break;
        rawFrame = frameWords[0];
    }

    SymSetOptions(SYMOPT_LOAD_LINES | SYMOPT_UNDNAME);
    if (SymInitialize(process, NULL, TRUE))
    {
        CONTEXT context = *exceptionInfo->ContextRecord;
        STACKFRAME64 frame = {};
#if defined(_M_X64)
        const DWORD machineType = IMAGE_FILE_MACHINE_AMD64;
        DWORD64 &programCounter = context.Rip;
        DWORD64 &stackPointer = context.Rsp;
        DWORD64 &framePointer = context.Rbp;
#else
        const DWORD machineType = IMAGE_FILE_MACHINE_I386;
        DWORD &programCounter = context.Eip;
        DWORD &stackPointer = context.Esp;
        DWORD &framePointer = context.Ebp;
#endif
        if (!programCounter && stackPointer)
        {
            // A call through a null function pointer leaves the caller's
            // return address at the top of the stack. Start there so the
            // crash report still identifies the owning call site.
            programCounter = *reinterpret_cast<ULONG_PTR*>(stackPointer);
            stackPointer += sizeof(ULONG_PTR);
        }
        frame.AddrPC.Offset = programCounter;
        frame.AddrPC.Mode = AddrModeFlat;
        frame.AddrFrame.Offset = framePointer;
        frame.AddrFrame.Mode = AddrModeFlat;
        frame.AddrStack.Offset = stackPointer;
        frame.AddrStack.Mode = AddrModeFlat;

        for (int depth = 0; depth < 64 && frame.AddrPC.Offset; ++depth)
        {
            char symbolStorage[sizeof(SYMBOL_INFO) + MAX_SYM_NAME] = {};
            SYMBOL_INFO* symbol = reinterpret_cast<SYMBOL_INFO*>(symbolStorage);
            symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
            symbol->MaxNameLen = MAX_SYM_NAME;
            DWORD64 displacement = 0;

            if (SymFromAddr(process, frame.AddrPC.Offset, &displacement, symbol))
                wsprintfA(line, "  #%02d %p %s+0x%p\r\n", depth,
                          (void *)frame.AddrPC.Offset, symbol->Name, (void *)displacement);
            else
                wsprintfA(line, "  #%02d %p\r\n", depth, (void *)frame.AddrPC.Offset);
            WriteCrashLine(output, line);
            WriteCrashLine(log, line);

            if (!StackWalk64(machineType, process, GetCurrentThread(),
                             &frame, &context, NULL, SymFunctionTableAccess64,
                             SymGetModuleBase64, NULL))
                break;
        }
        SymCleanup(process);
    }

    if (log != INVALID_HANDLE_VALUE)
        CloseHandle(log);

    return EXCEPTION_CONTINUE_SEARCH;
}

int main(int argc, const char* argv[])
{
    SetUnhandledExceptionFilter(DarkUnhandledExceptionFilter);
    if (!g_pMalloc && !HeapInit())
        return 1;

    if (UsesDarkData(argc > 0 ? argv[0] : NULL) &&
        !PrepareDarkDataDirectory(argc, argv))
        return 0;

	return _AppMain(argc, argv);
}

/*
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR pszCmdLine, int fShow)
{
	// Turn Windows command line into classic arguments    @TBD (toml 04-24-96):  won't do quoted arguments correctly
	int argc = 1;
	const char* argv[32];

	argv[0] = "";
	argv[1] = strtok(pszCmdLine, " ");
	while (argv[argc] != NULL && argc < 31)
		argv[++argc] = strtok(NULL, " ");

	return _AppMain(argc, argv);
}
*/

#endif
