#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "launchmenu.h"

#include <algorithm>
#include <ctype.h>
#include <stdio.h>
#include <string>
#include <vector>

namespace
{

const char* const kWindowClass = "OpenDarkEngineLaunchMenu";
const char* const kRegistryKey = "Software\\OpenDarkEngine\\Launcher";

enum
{
    kContentList = 100,
    kLaunchButton,
    kExitButton,
    kChooseFolderButton,
};

struct sLaunchItem
{
    eDarkLaunchChoice choice;
    std::string fmName;
    std::string name;
    std::string category;
    std::string description;
};

struct sLaunchMenuState
{
    std::vector<sLaunchItem> items;
    std::string directory;
    std::string gameName;
    std::string registryValue;
    std::string selectedFM;
    int result;
    HWND list;
    HFONT titleFont;
    HFONT headingFont;
    HFONT bodyFont;
    HFONT smallFont;

    sLaunchMenuState()
        : result(kDarkLaunchCancel), list(NULL), titleFont(NULL),
          headingFont(NULL), bodyFont(NULL), smallFont(NULL)
    {
    }
};

static COLORREF kBackground = RGB(12, 15, 15);
static COLORREF kPanel = RGB(23, 28, 27);
static COLORREF kSelected = RGB(47, 42, 28);
static COLORREF kGold = RGB(190, 153, 75);
static COLORREF kText = RGB(222, 216, 194);
static COLORREF kMuted = RGB(151, 148, 134);

static std::string Trim(const std::string& value)
{
    size_t first = 0;
    size_t last = value.size();
    while (first < last && isspace((unsigned char)value[first]))
        ++first;
    while (last > first && isspace((unsigned char)value[last - 1]))
        --last;
    return value.substr(first, last - first);
}

static void ReadFMInfo(const char* directory, std::string& name,
                       std::string& description)
{
    char path[MAX_PATH];
    FILE* file;
    _snprintf(path, sizeof(path) - 1, "%s\\fm.ini", directory);
    path[sizeof(path) - 1] = '\0';
    file = fopen(path, "rt");
    if (!file)
        return;

    char line[2048];
    while (fgets(line, sizeof(line), file))
    {
        char* newline = strpbrk(line, "\r\n");
        if (newline)
            *newline = '\0';
        char* equals = strchr(line, '=');
        if (!equals)
            continue;
        *equals = '\0';
        const std::string key = Trim(line);
        const std::string value = Trim(equals + 1);
        if (_stricmp(key.c_str(), "NiceName") == 0)
            name = value;
        else if (_stricmp(key.c_str(), "Descr") == 0)
            description = value;
    }
    fclose(file);
}

static bool HasMatchingFile(const char* directory, const char* pattern)
{
    char path[MAX_PATH];
    _snprintf(path, sizeof(path) - 1, "%s\\%s", directory, pattern);
    path[sizeof(path) - 1] = '\0';
    WIN32_FIND_DATAA found = {};
    HANDLE search = FindFirstFileA(path, &found);
    if (search == INVALID_HANDLE_VALUE)
        return false;
    bool result = false;
    do
    {
        if (!(found.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
        {
            result = true;
            break;
        }
    } while (FindNextFileA(search, &found));
    FindClose(search);
    return result;
}

static bool LooksLikeFanMission(const char* directory)
{
    static const char* markers[] = {
        "fm.ini", "fmsel.inf", "dark.cfg", "fm.cfg", "startmis.sav"
    };
    char path[MAX_PATH];
    for (int i = 0; i < sizeof(markers) / sizeof(markers[0]); ++i)
    {
        _snprintf(path, sizeof(path) - 1, "%s\\%s", directory, markers[i]);
        path[sizeof(path) - 1] = '\0';
        const DWORD attributes = GetFileAttributesA(path);
        if (attributes != INVALID_FILE_ATTRIBUTES &&
            !(attributes & FILE_ATTRIBUTE_DIRECTORY))
            return true;
    }
    return HasMatchingFile(directory, "*.mis") ||
           HasMatchingFile(directory, "*.gam") ||
           HasMatchingFile(directory, "*.cow");
}

static void AddFanMissions(sLaunchMenuState* state)
{
    char root[MAX_PATH];
    char pattern[MAX_PATH];
    _snprintf(root, sizeof(root) - 1, "%s\\FMs", state->directory.c_str());
    root[sizeof(root) - 1] = '\0';
    _snprintf(pattern, sizeof(pattern) - 1, "%s\\*", root);
    pattern[sizeof(pattern) - 1] = '\0';

    WIN32_FIND_DATAA found = {};
    HANDLE search = FindFirstFileA(pattern, &found);
    if (search == INVALID_HANDLE_VALUE)
        return;
    do
    {
        if (!(found.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ||
            found.cFileName[0] == '.')
            continue;
        char fullPath[MAX_PATH];
        _snprintf(fullPath, sizeof(fullPath) - 1, "%s\\%s", root,
                  found.cFileName);
        fullPath[sizeof(fullPath) - 1] = '\0';
        if (!LooksLikeFanMission(fullPath))
            continue;

        sLaunchItem item;
        item.choice = kDarkLaunchFanMission;
        item.fmName = found.cFileName;
        item.name = found.cFileName;
        item.category = "FAN MISSION";
        item.description = "Launch this fan mission with the installation's configured mod paths.";
        ReadFMInfo(fullPath, item.name, item.description);
        state->items.push_back(item);
    } while (FindNextFileA(search, &found));
    FindClose(search);

    if (state->items.size() > 2)
        std::sort(state->items.begin() + 2, state->items.end(),
                  [](const sLaunchItem& left, const sLaunchItem& right)
                  {
                      return _stricmp(left.name.c_str(), right.name.c_str()) < 0;
                  });
}

static unsigned PathHash(const char* text)
{
    unsigned hash = 2166136261u;
    for (; text && *text; ++text)
    {
        unsigned char value = (unsigned char)*text;
        if (value >= 'A' && value <= 'Z')
            value = (unsigned char)(value + ('a' - 'A'));
        hash = (hash ^ value) * 16777619u;
    }
    return hash;
}

static void MakeRegistryValue(sLaunchMenuState* state, const char* profile)
{
    char value[96];
    _snprintf(value, sizeof(value) - 1, "LastContent_%s_%08X",
              profile ? profile : "unknown", PathHash(state->directory.c_str()));
    value[sizeof(value) - 1] = '\0';
    state->registryValue = value;
}

static std::string LoadLastSelection(const sLaunchMenuState* state)
{
    HKEY key = NULL;
    char value[MAX_PATH] = "";
    DWORD type = 0;
    DWORD bytes = sizeof(value);
    if (RegOpenKeyExA(HKEY_CURRENT_USER, kRegistryKey, 0, KEY_QUERY_VALUE,
                      &key) != ERROR_SUCCESS)
        return std::string();
    const LONG result = RegQueryValueExA(key, state->registryValue.c_str(),
                                         NULL, &type,
                                         reinterpret_cast<BYTE*>(value), &bytes);
    RegCloseKey(key);
    if (result != ERROR_SUCCESS || (type != REG_SZ && type != REG_EXPAND_SZ))
        return std::string();
    value[sizeof(value) - 1] = '\0';
    return value;
}

static void SaveLastSelection(const sLaunchMenuState* state,
                              const sLaunchItem& item)
{
    HKEY key = NULL;
    DWORD disposition = 0;
    if (RegCreateKeyExA(HKEY_CURRENT_USER, kRegistryKey, 0, NULL, 0,
                        KEY_SET_VALUE, NULL, &key, &disposition) != ERROR_SUCCESS)
        return;
    std::string value;
    if (item.choice == kDarkLaunchConfigured)
        value = "configured";
    else if (item.choice == kDarkLaunchVanilla)
        value = "vanilla";
    else
        value = "fm:" + item.fmName;
    RegSetValueExA(key, state->registryValue.c_str(), 0, REG_SZ,
                   reinterpret_cast<const BYTE*>(value.c_str()),
                   (DWORD)value.size() + 1);
    RegCloseKey(key);
}

static int InitialSelection(const sLaunchMenuState* state)
{
    const std::string saved = LoadLastSelection(state);
    for (size_t i = 0; i < state->items.size(); ++i)
    {
        const sLaunchItem& item = state->items[i];
        if ((saved == "configured" && item.choice == kDarkLaunchConfigured) ||
            (saved == "vanilla" && item.choice == kDarkLaunchVanilla) ||
            (saved.size() > 3 && _strnicmp(saved.c_str(), "fm:", 3) == 0 &&
             item.choice == kDarkLaunchFanMission &&
             _stricmp(saved.c_str() + 3, item.fmName.c_str()) == 0))
            return (int)i;
    }
    return 0;
}

static const char* ProfileName(const char* profile)
{
    if (profile && _stricmp(profile, "gold") == 0)
        return "THIEF GOLD";
    if (profile && _stricmp(profile, "tdp") == 0)
        return "THIEF: THE DARK PROJECT";
    return "THIEF II: THE METAL AGE";
}

static bool IsDromEd(const char* executable)
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

static void AddItems(sLaunchMenuState* state)
{
    sLaunchItem vanilla;
    vanilla.choice = kDarkLaunchVanilla;
    vanilla.name = "Original Missions";
    vanilla.category = "ORIGINAL CAMPAIGN";
    vanilla.description =
        "Play the original campaign from the base retail resources only. Mod layers "
        "are disabled, and saves are kept separate from the modded campaign. This is "
        "the recommended and most compatible choice.";
    state->items.push_back(vanilla);

    sLaunchItem configured;
    configured.choice = kDarkLaunchConfigured;
    configured.name = "Original Missions - Modded";
    configured.category = "ORIGINAL CAMPAIGN";
    configured.description =
        "Use cam_mod.ini, uber_mod_path, and mod_path. Installed enhancement packs "
        "may require NewDark features that this engine does not support yet.";
    state->items.push_back(configured);
    AddFanMissions(state);
}

static void DrawTextWithFont(HDC dc, HFONT font, COLORREF color,
                             const char* text, RECT* rect, UINT format)
{
    HFONT oldFont = (HFONT)SelectObject(dc, font);
    SetTextColor(dc, color);
    SetBkMode(dc, TRANSPARENT);
    DrawTextA(dc, text, -1, rect, format);
    SelectObject(dc, oldFont);
}

static void DrawButton(const DRAWITEMSTRUCT* draw, sLaunchMenuState* state)
{
    const bool pressed = (draw->itemState & ODS_SELECTED) != 0;
    HBRUSH background = CreateSolidBrush(pressed ? kSelected : kPanel);
    FillRect(draw->hDC, &draw->rcItem, background);
    DeleteObject(background);
    HPEN border = CreatePen(PS_SOLID, 1, kGold);
    HPEN oldPen = (HPEN)SelectObject(draw->hDC, border);
    HBRUSH oldBrush = (HBRUSH)SelectObject(draw->hDC, GetStockObject(NULL_BRUSH));
    Rectangle(draw->hDC, draw->rcItem.left, draw->rcItem.top,
              draw->rcItem.right, draw->rcItem.bottom);
    SelectObject(draw->hDC, oldBrush);
    SelectObject(draw->hDC, oldPen);
    DeleteObject(border);

    char text[128] = "";
    GetWindowTextA(draw->hwndItem, text, sizeof(text));
    RECT label = draw->rcItem;
    DrawTextWithFont(draw->hDC, state->smallFont, kText, text, &label,
                     DT_CENTER | DT_VCENTER | DT_SINGLELINE);
}

static void DrawListItem(const DRAWITEMSTRUCT* draw, sLaunchMenuState* state)
{
    if (draw->itemID == (UINT)-1 || draw->itemID >= state->items.size())
        return;
    const bool selected = (draw->itemState & ODS_SELECTED) != 0;
    HBRUSH background = CreateSolidBrush(selected ? kSelected : kPanel);
    FillRect(draw->hDC, &draw->rcItem, background);
    DeleteObject(background);

    if (selected)
    {
        HPEN pen = CreatePen(PS_SOLID, 1, kGold);
        HPEN oldPen = (HPEN)SelectObject(draw->hDC, pen);
        HBRUSH oldBrush = (HBRUSH)SelectObject(draw->hDC, GetStockObject(NULL_BRUSH));
        Rectangle(draw->hDC, draw->rcItem.left, draw->rcItem.top,
                  draw->rcItem.right, draw->rcItem.bottom);
        SelectObject(draw->hDC, oldBrush);
        SelectObject(draw->hDC, oldPen);
        DeleteObject(pen);
    }

    const sLaunchItem& item = state->items[draw->itemID];
    RECT category = draw->rcItem;
    category.left += 15;
    category.top += 7;
    category.right -= 10;
    category.bottom = category.top + 16;
    DrawTextWithFont(draw->hDC, state->smallFont, selected ? kGold : kMuted,
                     item.category.c_str(), &category,
                     DT_LEFT | DT_SINGLELINE | DT_END_ELLIPSIS);
    RECT name = draw->rcItem;
    name.left += 15;
    name.top += 24;
    name.right -= 10;
    DrawTextWithFont(draw->hDC, state->bodyFont, kText, item.name.c_str(),
                     &name, DT_LEFT | DT_SINGLELINE | DT_END_ELLIPSIS);
}

static void PaintWindow(HWND window, sLaunchMenuState* state)
{
    PAINTSTRUCT paint;
    HDC dc = BeginPaint(window, &paint);
    RECT client;
    GetClientRect(window, &client);
    HBRUSH background = CreateSolidBrush(kBackground);
    FillRect(dc, &client, background);
    DeleteObject(background);

    RECT title = { 36, 25, client.right - 36, 68 };
    DrawTextWithFont(dc, state->titleFont, kGold, "DARK ENGINE", &title,
                     DT_LEFT | DT_SINGLELINE);
    RECT subtitle = { 38, 70, client.right - 36, 96 };
    const std::string line = state->gameName + "  /  SELECT CONTENT";
    DrawTextWithFont(dc, state->smallFont, kMuted, line.c_str(), &subtitle,
                     DT_LEFT | DT_SINGLELINE);

    RECT divider = { 36, 103, client.right - 36, 104 };
    HBRUSH gold = CreateSolidBrush(kGold);
    FillRect(dc, &divider, gold);

    RECT details = { 438, 126, client.right - 38, 430 };
    HBRUSH panel = CreateSolidBrush(kPanel);
    FillRect(dc, &details, panel);
    DeleteObject(panel);
    FrameRect(dc, &details, gold);

    const int selected = state->list ?
        (int)SendMessageA(state->list, LB_GETCURSEL, 0, 0) : -1;
    if (selected >= 0 && selected < (int)state->items.size())
    {
        const sLaunchItem& item = state->items[selected];
        RECT category = { details.left + 22, details.top + 22,
                          details.right - 22, details.top + 44 };
        DrawTextWithFont(dc, state->smallFont, kGold, item.category.c_str(),
                         &category, DT_LEFT | DT_SINGLELINE);
        RECT name = { details.left + 22, details.top + 53,
                      details.right - 22, details.top + 118 };
        DrawTextWithFont(dc, state->headingFont, kText, item.name.c_str(),
                         &name, DT_LEFT | DT_WORDBREAK | DT_END_ELLIPSIS);
        RECT description = { details.left + 22, details.top + 132,
                             details.right - 22, details.bottom - 22 };
        DrawTextWithFont(dc, state->bodyFont, kMuted,
                         item.description.c_str(), &description,
                         DT_LEFT | DT_TOP | DT_WORDBREAK);
    }
    DeleteObject(gold);

    RECT path = { 38, client.bottom - 30, client.right - 38, client.bottom - 10 };
    DrawTextWithFont(dc, state->smallFont, kMuted, state->directory.c_str(),
                     &path, DT_LEFT | DT_SINGLELINE | DT_PATH_ELLIPSIS);
    EndPaint(window, &paint);
}

static void AcceptSelection(HWND window, sLaunchMenuState* state)
{
    const int selected = (int)SendMessageA(state->list, LB_GETCURSEL, 0, 0);
    if (selected < 0 || selected >= (int)state->items.size())
        return;
    const sLaunchItem& item = state->items[selected];
    state->result = item.choice;
    state->selectedFM = item.fmName;
    SaveLastSelection(state, item);
    DestroyWindow(window);
}

static LRESULT CALLBACK LaunchMenuProc(HWND window, UINT message,
                                       WPARAM wParam, LPARAM lParam)
{
    sLaunchMenuState* state = reinterpret_cast<sLaunchMenuState*>(
        GetWindowLongPtrA(window, GWLP_USERDATA));
    if (message == WM_NCCREATE)
    {
        CREATESTRUCTA* create = reinterpret_cast<CREATESTRUCTA*>(lParam);
        state = reinterpret_cast<sLaunchMenuState*>(create->lpCreateParams);
        SetWindowLongPtrA(window, GWLP_USERDATA, (LONG_PTR)state);
    }
    if (!state)
        return DefWindowProcA(window, message, wParam, lParam);

    switch (message)
    {
        case WM_CREATE:
        {
            state->titleFont = CreateFontA(-31, 0, 0, 0, FW_BOLD, FALSE, FALSE,
                FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                CLEARTYPE_QUALITY, FF_DONTCARE, "Georgia");
            state->headingFont = CreateFontA(-22, 0, 0, 0, FW_BOLD, FALSE, FALSE,
                FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                CLEARTYPE_QUALITY, FF_DONTCARE, "Georgia");
            state->bodyFont = CreateFontA(-17, 0, 0, 0, FW_NORMAL, FALSE, FALSE,
                FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                CLEARTYPE_QUALITY, FF_DONTCARE, "Segoe UI");
            state->smallFont = CreateFontA(-14, 0, 0, 0, FW_BOLD, FALSE, FALSE,
                FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                CLEARTYPE_QUALITY, FF_DONTCARE, "Segoe UI");
            state->list = CreateWindowExA(0, "LISTBOX", "",
                WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_VSCROLL |
                LBS_NOTIFY | LBS_OWNERDRAWFIXED | LBS_NOINTEGRALHEIGHT,
                36, 126, 374, 370, window, (HMENU)kContentList,
                GetModuleHandleA(NULL), NULL);
            SendMessageA(state->list, LB_SETITEMHEIGHT, 0, 58);
            for (size_t i = 0; i < state->items.size(); ++i)
                SendMessageA(state->list, LB_ADDSTRING, 0,
                             (LPARAM)state->items[i].name.c_str());
            SendMessageA(state->list, LB_SETCURSEL, InitialSelection(state), 0);

            CreateWindowExA(0, "BUTTON", "LAUNCH", WS_CHILD | WS_VISIBLE |
                WS_TABSTOP | BS_OWNERDRAW, 438, 455, 175, 41, window,
                (HMENU)kLaunchButton, GetModuleHandleA(NULL), NULL);
            CreateWindowExA(0, "BUTTON", "CHANGE GAME FOLDER", WS_CHILD |
                WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW, 625, 455, 237, 41,
                window, (HMENU)kChooseFolderButton, GetModuleHandleA(NULL), NULL);
            CreateWindowExA(0, "BUTTON", "EXIT", WS_CHILD | WS_VISIBLE |
                WS_TABSTOP | BS_OWNERDRAW, 687, 512, 175, 38, window,
                (HMENU)kExitButton, GetModuleHandleA(NULL), NULL);
            SetFocus(state->list);
            return 0;
        }
        case WM_COMMAND:
            if (LOWORD(wParam) == kContentList &&
                HIWORD(wParam) == LBN_SELCHANGE)
            {
                InvalidateRect(window, NULL, FALSE);
                return 0;
            }
            if (LOWORD(wParam) == kContentList &&
                HIWORD(wParam) == LBN_DBLCLK)
            {
                AcceptSelection(window, state);
                return 0;
            }
            if (LOWORD(wParam) == kLaunchButton)
            {
                AcceptSelection(window, state);
                return 0;
            }
            if (LOWORD(wParam) == kChooseFolderButton)
            {
                state->result = kDarkLaunchChooseGame;
                DestroyWindow(window);
                return 0;
            }
            if (LOWORD(wParam) == kExitButton)
            {
                DestroyWindow(window);
                return 0;
            }
            break;
        case WM_KEYDOWN:
            if (wParam == VK_ESCAPE)
            {
                DestroyWindow(window);
                return 0;
            }
            if (wParam == VK_RETURN)
            {
                AcceptSelection(window, state);
                return 0;
            }
            break;
        case WM_DRAWITEM:
        {
            DRAWITEMSTRUCT* draw = reinterpret_cast<DRAWITEMSTRUCT*>(lParam);
            if (draw->CtlID == kContentList)
                DrawListItem(draw, state);
            else
                DrawButton(draw, state);
            return TRUE;
        }
        case WM_CTLCOLORLISTBOX:
        {
            HDC dc = (HDC)wParam;
            SetBkColor(dc, kPanel);
            SetTextColor(dc, kText);
            static HBRUSH brush = CreateSolidBrush(kPanel);
            return (LRESULT)brush;
        }
        case WM_ERASEBKGND:
            return 1;
        case WM_PAINT:
            PaintWindow(window, state);
            return 0;
        case WM_CLOSE:
            DestroyWindow(window);
            return 0;
        case WM_DESTROY:
            DeleteObject(state->titleFont);
            DeleteObject(state->headingFont);
            DeleteObject(state->bodyFont);
            DeleteObject(state->smallFont);
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProcA(window, message, wParam, lParam);
}

} // namespace

int DarkShowLaunchMenu(const char* directory, const char* profile,
                       const char* executable, char* fmName, int fmNameSize)
{
    if (!directory || !directory[0] || !fmName || fmNameSize <= 0)
        return kDarkLaunchCancel;
    fmName[0] = '\0';

    sLaunchMenuState state;
    state.directory = directory;
    state.gameName = ProfileName(profile);
    if (IsDromEd(executable))
        state.gameName += "  /  DROMED";
    MakeRegistryValue(&state, profile);
    AddItems(&state);

    WNDCLASSEXA windowClass = {};
    windowClass.cbSize = sizeof(windowClass);
    windowClass.style = CS_HREDRAW | CS_VREDRAW;
    windowClass.lpfnWndProc = LaunchMenuProc;
    windowClass.hInstance = GetModuleHandleA(NULL);
    windowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
    windowClass.lpszClassName = kWindowClass;
    RegisterClassExA(&windowClass);

    RECT frame = { 0, 0, 900, 590 };
    AdjustWindowRectEx(&frame, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU |
                       WS_MINIMIZEBOX, FALSE, WS_EX_APPWINDOW);
    const int width = frame.right - frame.left;
    const int height = frame.bottom - frame.top;
    RECT workArea;
    SystemParametersInfoA(SPI_GETWORKAREA, 0, &workArea, 0);
    const int x = workArea.left + (workArea.right - workArea.left - width) / 2;
    const int y = workArea.top + (workArea.bottom - workArea.top - height) / 2;
    HWND window = CreateWindowExA(WS_EX_APPWINDOW, kWindowClass,
        IsDromEd(executable) ? "DromEd Content" : "Thief Content",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        x, y, width, height, NULL, NULL, GetModuleHandleA(NULL), &state);
    if (!window)
        return kDarkLaunchCancel;
    ShowWindow(window, SW_SHOW);
    UpdateWindow(window);

    MSG message;
    while (GetMessageA(&message, NULL, 0, 0) > 0)
    {
        if (!IsDialogMessageA(window, &message))
        {
            TranslateMessage(&message);
            DispatchMessageA(&message);
        }
    }

    if (state.result == kDarkLaunchFanMission)
    {
        strncpy(fmName, state.selectedFM.c_str(), fmNameSize);
        fmName[fmNameSize - 1] = '\0';
    }
    return state.result;
}
