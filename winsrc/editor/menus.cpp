/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Source: r:/t2repos/thief2/src/editor/menus.cpp,v $
// $Author: henrys $
// $Date: 1999/10/29 19:18:50 $
// $Revision: 1.7 $
//
// @Note (toml 08-03-97): This is a temporary solution that will have to be
// rethought when dynamic menus are supported. No sub-menus right now

#include <win32_platform.h>
#include <lg.h>
#include <algorithm>
#include <cctype>
#include <cstring>
#include <string>
#include <vector>

#include <appagg.h>
#include <wappapi.h>

#include <dynarray.h>
#include <str.h>
#include <hashpp.h>
#include <hshpptem.h>
#include <winmenu.h>
#include <config.h>

#include <menus.h>
#include <command.h>
#include <inpbnd_i.h>
#include <mprintf.h>

// Must be last header
#include <dbmem.h>

///////////////////////////////////////////////////////////////////////////////

#define kMenuCommandBase 1000

class cMenuCommands
{
public:
   ~cMenuCommands();

   unsigned NewCommand(const char *);
   const char * Lookup(unsigned);
   void ClearAll();

private:
   cDynArray<const char *> m_CommandTexts;
};

///////////////////////////////////////

inline cMenuCommands::~cMenuCommands()
{
   ClearAll();
}

///////////////////////////////////////

inline unsigned cMenuCommands::NewCommand(const char * pszCommand)
{
   m_CommandTexts.Append(strdup(pszCommand));
   return kMenuCommandBase + m_CommandTexts.Size() - 1;
}

///////////////////////////////////////

inline const char * cMenuCommands::Lookup(unsigned id)
{
   unsigned index = id - kMenuCommandBase;
   if (index < (unsigned)m_CommandTexts.Size())
      return m_CommandTexts[index];
   else
      return NULL;
}

///////////////////////////////////////

void cMenuCommands::ClearAll()
{
   for (int i = 0; i < m_CommandTexts.Size(); i++)
      free((void *)(m_CommandTexts[i]));
   m_CommandTexts.SetSize(0);
}

///////////////////////////////////////////////////////////////////////////////

class cMenuSet
{
public:
   cMenuSet();
   ~cMenuSet();

   void BeginMenu(const char * = NULL);
   void AddItem(const char *, unsigned id);
   void AddSeparator();
   void EndMenu();

   void AttachToWindow(HWND hWnd);
   void DetachFromWindow();

   void DestroyAll();

   cWinMenu* GetMenuByNumber(int number);

private:
   HWND                  m_hWnd;
   int                   m_iCurrentMenu;
   cDynArray<cWinMenu *> m_Menus;
   cDynArray<int>        m_MenuStack;

};

///////////////////////////////////////

inline cMenuSet::cMenuSet()
 : m_hWnd(0),
   m_iCurrentMenu(-1)
{
}

///////////////////////////////////////

inline cMenuSet::~cMenuSet()
{
   DestroyAll();
}

///////////////////////////////////////

inline void cMenuSet::BeginMenu(const char * pszSubMenuName)
{
   cWinMenu * pMenu = new cWinMenu();

   pMenu->CreateMenu();
   m_Menus.Append(pMenu);

   m_MenuStack.Append(m_iCurrentMenu);

   int oldMenu = m_iCurrentMenu;

   m_iCurrentMenu = (int)m_Menus.Size() - 1;

   if (m_iCurrentMenu != 0)
      m_Menus[oldMenu]->AppendMenu(MF_POPUP, (uintptr_t)(HMENU)(*pMenu), pszSubMenuName);

}

///////////////////////////////////////

inline void cMenuSet::EndMenu()
{
   m_iCurrentMenu = m_MenuStack[m_MenuStack.Size() - 1];
   m_MenuStack.SetSize(m_MenuStack.Size() - 1);
}

///////////////////////////////////////

inline void cMenuSet::AddItem(const char * pszMenuItemName, unsigned id)
{
   if (m_Menus.Size() != 0)
      m_Menus[m_iCurrentMenu]->AppendMenu(MF_STRING, id, pszMenuItemName);
}

///////////////////////////////////////

inline void cMenuSet::AddSeparator()
{
   if (m_Menus.Size() != 0)
      m_Menus[m_iCurrentMenu]->AppendMenu(MF_SEPARATOR, 0, 0);
}

///////////////////////////////////////

void cMenuSet::AttachToWindow(HWND hWnd)
{
   if (m_Menus.Size() == 0)
      return;

   if (!hWnd)
      return;

   if (m_hWnd)
      SetMenu(m_hWnd, NULL);

   m_hWnd = hWnd;

   SetMenu(m_hWnd, *(m_Menus[0]));
}

///////////////////////////////////////

void cMenuSet::DetachFromWindow()
{
   if (m_hWnd)
      SetMenu(m_hWnd, NULL);
   m_hWnd = NULL;
}

///////////////////////////////////////

void cMenuSet::DestroyAll()
{
   if (m_Menus.Size() == 0)
      return;

   int iFirst = (m_hWnd) ? 1 : 0;

   for (int i = iFirst; i < m_Menus.Size(); i++)
   {
      delete m_Menus[i];
   }
   m_iCurrentMenu = -1;
   m_MenuStack.SetSize(0);
   m_Menus.SetSize(0);
}

///////////////////////////////////////

cWinMenu* cMenuSet::GetMenuByNumber(int number)
{
    if (number < 0 || number >= m_Menus.Size())
       return NULL;
    return m_Menus[number];
}

///////////////////////////////////////////////////////////////////////////////

typedef cStrHashTable<BOOL> cMenusInProgress;

///////////////////////////////////////////////////////////////////////////////

static cMenuCommands g_MenuCommands;
static cMenuSet      g_MenuSet;

struct sEditorShortcut
{
   std::string key;
   std::string action;
   std::string command;
   int group;
};

static std::vector<sEditorShortcut> g_DeclaredShortcuts;
static HWND g_hShortcutWindow;
static HWND g_hCommandWindow;

enum eShortcutGroup
{
   kShortcutHelp,
   kShortcutCamera,
   kShortcutViews,
   kShortcutSelection,
   kShortcutTransforms,
   kShortcutBrushProperties,
   kShortcutGroups,
   kShortcutWorld,
   kShortcutFiles,
   kShortcutPreview,
   kShortcutAcoustics,
   kShortcutDiagnostics,
   kShortcutMenu,
   kShortcutOther,
};

struct sCommandHelp
{
   const char *command;
   const char *description;
   int group;
};

// These are the complete built-in editor defaults installed by gen_bind.cpp.
// Keep the command strings exact so argument-bearing commands can be explained
// in terms of what the key actually does, rather than exposing axis numbers or
// other implementation details to the user.
static const sCommandHelp g_BuiltInCommandHelp[] = {
   {"help", "Print matching commands in the command console", kShortcutHelp},
   {"show_command_help", "Open the searchable command reference", kShortcutHelp},
   {"show_keyboard_shortcuts", "Open the keyboard and mouse reference", kShortcutHelp},
   {"edit_command", "Open the command entry box", kShortcutHelp},

   {"cam_rotate 0", "Roll camera right", kShortcutCamera},
   {"cam_rotate 1", "Look camera down", kShortcutCamera},
   {"cam_rotate 2", "Turn camera left", kShortcutCamera},
   {"cam_rotate 3", "Roll camera left", kShortcutCamera},
   {"cam_rotate 4", "Look camera up", kShortcutCamera},
   {"cam_rotate 5", "Turn camera right", kShortcutCamera},
   {"cam_slew 0", "Move camera forward", kShortcutCamera},
   {"cam_slew 1", "Strafe camera left", kShortcutCamera},
   {"cam_slew 2", "Move camera up", kShortcutCamera},
   {"cam_slew 3", "Move camera backward", kShortcutCamera},
   {"cam_slew 4", "Strafe camera right", kShortcutCamera},
   {"cam_slew 5", "Move camera down", kShortcutCamera},
   {"cam_level", "Level camera pitch", kShortcutCamera},
   {"cam_unroll", "Reset camera roll", kShortcutCamera},
   {"cam_spotlight", "Toggle camera spotlight", kShortcutCamera},
   {"num_scroll 1", "Pan 2D view down and left", kShortcutCamera},
   {"num_scroll 2", "Pan 2D view down", kShortcutCamera},
   {"num_scroll 3", "Pan 2D view down and right", kShortcutCamera},
   {"num_scroll 4", "Pan 2D view left", kShortcutCamera},
   {"num_scroll 6", "Pan 2D view right", kShortcutCamera},
   {"num_scroll 7", "Pan 2D view up and left", kShortcutCamera},
   {"num_scroll 8", "Pan 2D view up", kShortcutCamera},
   {"num_scroll 9", "Pan 2D view up and right", kShortcutCamera},
   {"move_game_camera 0.0,0.0,0.0", "Move game camera to editor camera", kShortcutCamera},

   {"cycle_mode", "Cycle viewport render mode", kShortcutViews},
   {"toggle_3d", "Toggle current viewport between 2D and 3D", kShortcutViews},
   {"solo_toggle", "Toggle current viewport between single-pane and four-pane layout", kShortcutViews},
   {"zoom_all 0.5", "Zoom all 2D views in", kShortcutViews},
   {"zoom_all 2.0", "Zoom all 2D views out", kShortcutViews},
   {"global_scale 0", "Decrease global 2D view scale", kShortcutViews},
   {"global_scale 1", "Increase global 2D view scale", kShortcutViews},
   {"cycle_view 0", "Select previous viewport", kShortcutViews},
   {"cycle_view 1", "Select next viewport", kShortcutViews},
   {"cycle_context -1", "Select previous editor context", kShortcutViews},
   {"cycle_context 1", "Select next editor context", kShortcutViews},
   {"redraw_all", "Redraw all editor viewports", kShortcutViews},
   {"xmouse", "Toggle viewport focus following the mouse", kShortcutViews},
   {"edit_mode 640,480", "Switch editor canvas to 640 x 480", kShortcutViews},
   {"edit_mode 800,600", "Switch editor canvas to 800 x 600", kShortcutViews},
   {"edit_mode 1024,768", "Switch editor canvas to 1024 x 768", kShortcutViews},

   {"cycle_brush -1", "Select previous brush or object", kShortcutSelection},
   {"cycle_brush 1", "Select next brush or object", kShortcutSelection},
   {"cycle_face -1", "Select previous face", kShortcutSelection},
   {"cycle_face 1", "Select next face", kShortcutSelection},
   {"insert_brush", "Clone or insert the selected brush", kShortcutSelection},
   {"delete_brush", "Delete selected brush or object", kShortcutSelection},
   {"new_brush 1", "Create a new brush", kShortcutSelection},
   {"edit_command edit_obj", "Open the selected object's property editor", kShortcutSelection},
   {"vBrush_EOT", "Move selection to the end of brush time", kShortcutSelection},
   {"reset_brush", "Reset selected brush dimensions and orientation", kShortcutSelection},
   {"brush_to_room 5", "Convert selected brush to a room brush", kShortcutSelection},

   {"brush_translate 0", "Move brush in the +X direction", kShortcutTransforms},
   {"brush_translate 1", "Move brush in the +Y direction", kShortcutTransforms},
   {"brush_translate 2", "Move brush in the +Z direction", kShortcutTransforms},
   {"brush_translate 3", "Move brush in the -X direction", kShortcutTransforms},
   {"brush_translate 4", "Move brush in the -Y direction", kShortcutTransforms},
   {"brush_translate 5", "Move brush in the -Z direction", kShortcutTransforms},
   {"brush_rotate 0", "Rotate brush in the +X direction", kShortcutTransforms},
   {"brush_rotate 1", "Rotate brush in the +Y direction", kShortcutTransforms},
   {"brush_rotate 2", "Rotate brush in the +Z direction", kShortcutTransforms},
   {"brush_rotate 3", "Rotate brush in the -X direction", kShortcutTransforms},
   {"brush_rotate 4", "Rotate brush in the -Y direction", kShortcutTransforms},
   {"brush_rotate 5", "Rotate brush in the -Z direction", kShortcutTransforms},
   {"brush_stretch 0", "Grow brush along X", kShortcutTransforms},
   {"brush_stretch 1", "Grow brush along Y", kShortcutTransforms},
   {"brush_stretch 2", "Grow brush along Z", kShortcutTransforms},
   {"brush_stretch 3", "Shrink brush along X", kShortcutTransforms},
   {"brush_stretch 4", "Shrink brush along Y", kShortcutTransforms},
   {"brush_stretch 5", "Shrink brush along Z", kShortcutTransforms},

   {"set_medium 0", "Set brush medium to solid", kShortcutBrushProperties},
   {"set_medium 1", "Set brush medium to air", kShortcutBrushProperties},
   {"set_medium 2", "Set brush medium to water", kShortcutBrushProperties},
   {"set_medium 3", "Set brush medium to flood", kShortcutBrushProperties},
   {"cycle_media -1", "Select previous brush medium", kShortcutBrushProperties},
   {"cycle_media 1", "Select next brush medium", kShortcutBrushProperties},
   {"cycle_tex -1", "Select previous texture", kShortcutBrushProperties},
   {"cycle_tex 1", "Select next texture", kShortcutBrushProperties},
   {"texture_pal", "Open the texture palette", kShortcutBrushProperties},
   {"brush_color 1", "Set brush display color 1", kShortcutBrushProperties},
   {"brush_color 2", "Set brush display color 2", kShortcutBrushProperties},
   {"brush_color 3", "Set brush display color 3", kShortcutBrushProperties},

   {"store_group", "Store the current multibrush group", kShortcutGroups},
   {"dissolve_group", "Dissolve the current multibrush group", kShortcutGroups},
   {"cycle_group -1", "Select previous multibrush group", kShortcutGroups},
   {"cycle_group 0", "Select current multibrush group", kShortcutGroups},
   {"cycle_group 1", "Select next multibrush group", kShortcutGroups},
   {"brush_relative", "Toggle brush-relative multibrush transforms", kShortcutGroups},
   {"edit_command save_group", "Save a multibrush group", kShortcutGroups},
   {"edit_command load_group", "Load a multibrush group", kShortcutGroups},

   {"set_grid", "Set grid spacing from the selected brush", kShortcutWorld},
   {"grid_toggle", "Toggle grid snapping", kShortcutWorld},
   {"grid_scale 0.5", "Halve grid spacing", kShortcutWorld},
   {"grid_scale 2.0", "Double grid spacing", kShortcutWorld},
   {"portalize", "Portalize the world", kShortcutWorld},
   {"auto_portalize", "Toggle automatic portalization", kShortcutWorld},
   {"raycast_light", "Calculate raycast lighting", kShortcutWorld},
   {"relight_level 0", "Relight the complete level", kShortcutWorld},
   {"lit_obj_toggle", "Toggle editor lighting on objects", kShortcutWorld},

   {"undo", "Undo last edit", kShortcutFiles},
   {"redo", "Redo last undone edit", kShortcutFiles},
   {"history_cmd -1", "Move backward through command history", kShortcutFiles},
   {"eval world_file edit_command save_mission %s", "Save the current mission", kShortcutFiles},
   {"edit_command load_file", "Open a mission or COW file", kShortcutFiles},
   {"edit_command clear_world", "Clear the current world", kShortcutFiles},
   {"quick_resynch", "Quickly resynchronize editor databases", kShortcutFiles},

   {"mission_loop", "Run the mission loop", kShortcutPreview},
   {"game_mode", "Enter game preview mode", kShortcutPreview},
   {"game_mode 320,240", "Enter game preview at 320 x 240", kShortcutPreview},
   {"game_mode 400,300", "Enter game preview at 400 x 300", kShortcutPreview},
   {"game_mode 512,384", "Enter game preview at 512 x 384", kShortcutPreview},
   {"game_mode 640,480", "Enter game preview at 640 x 480", kShortcutPreview},
   {"game_mode 800,600", "Enter game preview at 800 x 600", kShortcutPreview},
   {"edit_mode", "Return from game preview to the editor", kShortcutPreview},
   {"foot_sounds", "Toggle footstep sounds during preview", kShortcutPreview},
   {"fake_physics", "Toggle fake physics during preview", kShortcutPreview},

   {"set_room_type 1", "Set room acoustics: Small Dead", kShortcutAcoustics},
   {"set_room_type 2", "Set room acoustics: Small Normal", kShortcutAcoustics},
   {"set_room_type 3", "Set room acoustics: Bathroom", kShortcutAcoustics},
   {"set_room_type 4", "Set room acoustics: Living Room", kShortcutAcoustics},
   {"set_room_type 5", "Set room acoustics: Large Normal", kShortcutAcoustics},
   {"set_room_type 6", "Set room acoustics: Auditorium", kShortcutAcoustics},
   {"set_room_type 7", "Set room acoustics: Concert Hall", kShortcutAcoustics},
   {"set_room_type 8", "Set room acoustics: Large Live", kShortcutAcoustics},
   {"set_room_type 9", "Set room acoustics: Caverns", kShortcutAcoustics},
   {"set_room_type 10", "Set room acoustics: Hangar", kShortcutAcoustics},
   {"set_room_type 11", "Set room acoustics: Dead Hallway", kShortcutAcoustics},
   {"set_room_type 15", "Set room acoustics: Outside", kShortcutAcoustics},
   {"set_room_type 21", "Set room acoustics: Sewers", kShortcutAcoustics},
   {"next_room", "Select next room brush", kShortcutAcoustics},

   {"mono_debug", "Open monochrome debug output", kShortcutDiagnostics},
   {"screen_dump", "Save a screenshot", kShortcutDiagnostics},
   {"show_poly_edges", "Toggle polygon-edge display", kShortcutDiagnostics},
   {"show_all_edges", "Toggle all portal-cell edges", kShortcutDiagnostics},
   {"show_cell", "Show the current portal cell", kShortcutDiagnostics},
   {"show_mip", "Show texture mip levels", kShortcutDiagnostics},
   {"show_poly", "Show the current polygon", kShortcutDiagnostics},
   {"render_info 30", "Show detailed renderer information", kShortcutDiagnostics},
   {"render_info 10", "Show renderer information", kShortcutDiagnostics},
   {"hello_debugger", "Break into the debugger", kShortcutDiagnostics},
   {"draw_paths", "Toggle AI path drawing", kShortcutDiagnostics},
   {"follow_test", "Run AI follow test", kShortcutDiagnostics},
   {"draw_links", "Toggle link drawing", kShortcutDiagnostics},
   {"merge_node", "Merge selected AI path node", kShortcutDiagnostics},
   {"patrol_test 2", "Run AI patrol test", kShortcutDiagnostics},
   {"edit_command obj_tree", "Open the object hierarchy", kShortcutDiagnostics},
   {"ai_build_path_database", "Build the AI path database", kShortcutDiagnostics},
   {"draw_ais", "Toggle AI drawing", kShortcutDiagnostics},
   {"draw_path_cells", "Toggle AI path-cell drawing", kShortcutDiagnostics},
   {"show_stats", "Toggle runtime statistics", kShortcutDiagnostics},
   {"draw_path_cell_links", "Toggle AI path-cell link drawing", kShortcutDiagnostics},
   {"path_test", "Run AI path test", kShortcutDiagnostics},
   {"draw_move_suggestions", "Toggle AI movement suggestions", kShortcutDiagnostics},
   {"quit_game", "Exit DromEd", kShortcutDiagnostics},
   {"create_ai", "Create a test AI", kShortcutDiagnostics},
   {"dump_cmds cmd.txt", "Write all commands to cmd.txt", kShortcutDiagnostics},
   {"rend_name_toggle 23", "Toggle renderer object-name display", kShortcutDiagnostics},
   {"stats_full", "Toggle full statistics", kShortcutDiagnostics},
   {"time_stats", "Toggle timing statistics", kShortcutDiagnostics},
};

static const sCommandHelp *FindBuiltInCommandHelp(const std::string &command)
{
   for (size_t i = 0;
        i < sizeof(g_BuiltInCommandHelp) / sizeof(g_BuiltInCommandHelp[0]); ++i)
      if (!_stricmp(command.c_str(), g_BuiltInCommandHelp[i].command))
         return &g_BuiltInCommandHelp[i];
   return NULL;
}

static bool StartsWith(const std::string &text, const char *prefix)
{
   return text.compare(0, strlen(prefix), prefix) == 0;
}

static std::string Lowercase(const std::string &text)
{
   std::string result(text);
   std::transform(result.begin(), result.end(), result.begin(),
      [](unsigned char c) { return (char)std::tolower(c); });
   return result;
}

static bool RemoveModifier(std::string &key, const char *modifier)
{
   bool found = false;
   std::string before = std::string(modifier) + "+";
   std::string after = std::string("+") + modifier;
   std::string::size_type pos;

   while ((pos = key.find(before)) != std::string::npos)
   {
      key.erase(pos, before.length());
      found = true;
   }
   while ((pos = key.find(after)) != std::string::npos)
   {
      key.erase(pos, after.length());
      found = true;
   }
   if (key == modifier)
   {
      key.clear();
      found = true;
   }
   return found;
}

static std::string DisplayBaseKey(const std::string &base)
{
   struct sKeyName { const char *raw; const char *display; };
   static const sKeyName names[] = {
      {"del", "Delete"}, {"ins", "Insert"}, {"pgup", "Page Up"},
      {"pgdn", "Page Down"}, {"space", "Space"}, {"tab", "Tab"},
      {"home", "Home"}, {"end", "End"}, {"print_screen", "Print Screen"},
      {"keypad_plus", "Numpad +"}, {"keypad_minus", "Numpad -"},
      {"keypad_up", "Numpad 8"}, {"keypad_down", "Numpad 2"},
      {"keypad_left", "Numpad 4"}, {"keypad_right", "Numpad 6"},
      {"keypad_home", "Numpad 7"}, {"keypad_end", "Numpad 1"},
      {"keypad_pgup", "Numpad 9"}, {"keypad_pgdn", "Numpad 3"},
   };
   for (size_t i = 0; i < sizeof(names) / sizeof(names[0]); ++i)
      if (base == names[i].raw)
         return names[i].display;

   std::string result(base);
   if (result.length() == 1 && std::isalpha((unsigned char)result[0]))
      result[0] = (char)std::toupper((unsigned char)result[0]);
   else if (result.length() > 1 && result[0] == 'f' &&
            std::all_of(result.begin() + 1, result.end(),
               [](unsigned char c) { return std::isdigit(c) != 0; }))
      result[0] = 'F';
   return result;
}

static std::string NormalizeKey(const std::string &raw)
{
   std::string base = Lowercase(raw);
   bool ctrl = RemoveModifier(base, "ctrl");
   bool alt = RemoveModifier(base, "alt");
   bool shift = RemoveModifier(base, "shift");
   std::string result;

   if (ctrl) result += "Ctrl+";
   if (alt) result += "Alt+";
   if (shift) result += "Shift+";
   result += DisplayBaseKey(base);
   return result;
}

static std::string CommandName(const std::string &command)
{
   std::string::size_type end = command.find_first_of(" \t");
   return Lowercase(command.substr(0, end));
}

static int ClassifyShortcut(const std::string &command, bool menuShortcut)
{
   std::string name = CommandName(command);
   const sCommandHelp *builtIn = FindBuiltInCommandHelp(command);
   if (builtIn)
      return builtIn->group;
   if (name == "help" || name == "show_command_help" ||
       name == "show_keyboard_shortcuts" || name == "edit_command")
      return kShortcutHelp;
   if (StartsWith(name, "cam_") || name == "num_scroll")
      return kShortcutCamera;
   if (name == "cycle_view" || name == "cycle_mode" ||
       name == "toggle_3d" || name == "zoom_all" ||
       name == "global_scale" || name == "solo_toggle" ||
       name == "cycle_context" || name == "redraw_all")
      return kShortcutViews;
   if (name == "cycle_brush" ||
       name == "insert_brush" || name == "delete_brush" ||
       name == "new_brush" || name == "cycle_face" || name == "find_obj")
      return kShortcutSelection;
   if (name == "brush_translate" || name == "brush_rotate" ||
       name == "brush_stretch")
      return kShortcutTransforms;
   if (name == "cycle_tex" || name == "cycle_media" ||
       name == "set_medium" || name == "texture_pal" ||
       name == "brush_color")
      return kShortcutBrushProperties;
   if (name.find("group") != std::string::npos)
      return kShortcutGroups;
   if (name.find("portal") != std::string::npos ||
       name.find("light") != std::string::npos ||
       name.find("grid") != std::string::npos)
      return kShortcutWorld;
   if (name.find("save") != std::string::npos ||
       name.find("load") != std::string::npos || name == "undo" ||
       name == "redo" || name == "edit_command")
      return kShortcutFiles;
   if (name == "game_mode" || name == "edit_mode" ||
       name == "mission_loop")
      return kShortcutPreview;
   if (name == "set_room_type" || name == "next_room")
      return kShortcutAcoustics;
   if (StartsWith(name, "show_") || StartsWith(name, "draw_") ||
       name.find("debug") != std::string::npos ||
       name.find("stats") != std::string::npos ||
       name.find("path") != std::string::npos || name == "screen_dump")
      return kShortcutDiagnostics;
   if (menuShortcut)
      return kShortcutMenu;
   return kShortcutOther;
}

static std::string HumanizeCommand(const std::string &command,
                                   const std::string &key)
{
   std::string name = CommandName(command);
   const sCommandHelp *builtIn = FindBuiltInCommandHelp(command);
   if (builtIn)
      return builtIn->description;

   // User-supplied bindings that call known commands with novel arguments can
   // still use the registered command description below.
   if (name == "cam_level") return "Level camera pitch";
   if (name == "cam_unroll") return "Reset camera roll";
   if (name == "cam_spotlight") return "Toggle camera spotlight";
   if (name == "cycle_brush") return command.find("-1") != std::string::npos ?
      "Select previous brush or object" : "Select next brush or object";
   if (name == "cycle_view") return "Select another viewport";
   if (name == "toggle_3d") return "Toggle current viewport between 2D and 3D";
   if (name == "cycle_mode") return "Cycle viewport render mode";
   if (name == "zoom_all") return command.find("2.0") != std::string::npos ?
      "Zoom 2D views out" : "Zoom 2D views in";
   if (name == "insert_brush") return "Insert a brush";
   if (name == "delete_brush") return "Delete selected brush or object";
   if (name == "undo") return "Undo";
   if (name == "redo") return "Redo";
   if (name == "portalize") return "Portalize the world";
   if (name == "auto_portalize") return "Toggle automatic portalization";
   if (name == "raycast_light") return "Calculate raycast lighting";
   if (name == "relight_level") return "Relight the level";
   if (name == "game_mode") return "Enter game preview mode";
   if (name == "edit_mode") return "Return to editor mode";
   if (name == "set_room_type") return "Set selected room's EAX room type";
   if (name == "next_room") return "Select next room";

   Command *definition = CommandFind((char *)name.c_str(), (int)name.length());
   if (definition && definition->comment && *definition->comment)
   {
      std::string result(definition->comment);
      result[0] = (char)std::toupper((unsigned char)result[0]);
      return result;
   }

   std::replace(name.begin(), name.end(), '_', ' ');
   if (!name.empty())
      name[0] = (char)std::toupper((unsigned char)name[0]);
   return name;
}

static const char *ShortcutGroupName(int group)
{
   static const char *names[] = {
      "Help and command entry",
      "Camera movement and orientation",
      "Viewport navigation and display",
      "Selection, creation, and deletion",
      "Brush movement, rotation, and size",
      "Brush media, textures, and display",
      "Multibrush groups",
      "Grid, portalization, and lighting",
      "Files, commands, and edit history",
      "Game preview",
      "Room acoustics (controls retained; EAX audio is not implemented)",
      "Diagnostics and developer tools",
      "Menu commands",
      "Other active bindings",
   };
   return names[group];
}

static void RecordDeclaredShortcut(const char *menuText,
                                   const char *menuCommand)
{
   std::string text(menuText ? menuText : "");
   std::string::size_type open = text.rfind('(');
   std::string::size_type close = text.find(')', open);
   if (open == std::string::npos || close == std::string::npos)
      return;

   std::string key = text.substr(open + 1, close - open - 1);
   std::string lowerKey = Lowercase(key);
   bool functionKey = key.length() >= 2 &&
                      (key[0] == 'F' || key[0] == 'f') &&
                      std::all_of(key.begin() + 1, key.end(),
                         [](unsigned char c) { return std::isdigit(c) != 0; });
   if (lowerKey.find("ctrl") == std::string::npos &&
       lowerKey.find("alt") == std::string::npos &&
       lowerKey.find("shift") == std::string::npos &&
       !functionKey &&
       lowerKey != "del" && lowerKey != "ins")
      return;

   text.erase(open, close - open + 1);
   text.erase(std::remove(text.begin(), text.end(), '&'), text.end());
   while (!text.empty() && (text.back() == '.' || text.back() == ' '))
      text.pop_back();

   sEditorShortcut shortcut;
   shortcut.key = NormalizeKey(key);
   shortcut.action = text;
   shortcut.command = menuCommand ? menuCommand : "";
   shortcut.group = ClassifyShortcut(shortcut.command, true);
   if (shortcut.action.empty())
      shortcut.action = HumanizeCommand(shortcut.command, shortcut.key);
   g_DeclaredShortcuts.push_back(shortcut);
}

static BOOL LGAPI CollectActiveBinding(const char *control,
                                       const char *command, void *data)
{
   std::vector<sEditorShortcut> *shortcuts =
      (std::vector<sEditorShortcut> *)data;
   if (!control || !*control || !command || !*command)
      return TRUE;

   sEditorShortcut shortcut;
   shortcut.key = NormalizeKey(control);
   shortcut.command = command;
   shortcut.action = HumanizeCommand(shortcut.command, shortcut.key);
   shortcut.group = ClassifyShortcut(shortcut.command, false);
   shortcuts->push_back(shortcut);
   return TRUE;
}

static int ShortcutSortRank(const sEditorShortcut &shortcut)
{
   if (shortcut.group != kShortcutCamera)
      return 1000;

   // Present the main camera cluster in the way it is used, not alphabetic
   // order: planar movement, vertical/strafe movement, orientation, then the
   // spatial numpad pan layout and miscellaneous camera commands.
   static const char *cameraOrder[] = {
      "W", "A", "S", "D",
      "Q", "E", "Z", "X", "C",
      "R", "F", "V",
      "1", "2", "3", "/",
      "Numpad 7", "Numpad 8", "Numpad 9",
      "Numpad 4", "Numpad 6",
      "Numpad 1", "Numpad 2", "Numpad 3",
      "Home", "F8",
   };
   for (size_t i = 0; i < sizeof(cameraOrder) / sizeof(cameraOrder[0]); ++i)
      if (!_stricmp(shortcut.key.c_str(), cameraOrder[i]))
         return (int)i;
   return 900;
}

static std::string BuildShortcutText()
{
   std::vector<sEditorShortcut> active;
   std::string text;
   InputBinderForEachBinding(CollectActiveBinding, &active);

   // Parenthetical shortcuts in menus.cfg are annotations, not a separate
   // accelerator table.  If an active input binding uses the same key, it is
   // authoritative and avoids showing contradictory or duplicate actions.
   for (const auto &declared : g_DeclaredShortcuts)
   {
      bool keyAlreadyBound = std::any_of(active.begin(), active.end(),
         [&declared](const sEditorShortcut &entry) {
            return !_stricmp(entry.key.c_str(), declared.key.c_str());
         });
      if (!keyAlreadyBound)
         active.push_back(declared);
   }
   std::stable_sort(active.begin(), active.end(),
      [](const sEditorShortcut &a, const sEditorShortcut &b) {
         if (a.group != b.group)
            return a.group < b.group;
         int rankA = ShortcutSortRank(a);
         int rankB = ShortcutSortRank(b);
         if (rankA != rankB)
            return rankA < rankB;
         return _stricmp(a.key.c_str(), b.key.c_str()) < 0;
      });

   text = "DromEd input reference\r\n"
          "======================\r\n\r\n"
          "Keys are grouped by purpose. User .bnd overrides are reflected here.\r\n";
   if (active.empty())
      text += "(No keyboard bindings are registered in the current editor context.)\r\n";
   else
   {
      int lastGroup = -1;
      for (const auto &shortcut : active)
      {
         if (shortcut.group != lastGroup)
         {
            text += "\r\n";
            text += ShortcutGroupName(shortcut.group);
            text += "\r\n";
            text.append(strlen(ShortcutGroupName(shortcut.group)), '-');
            text += "\r\n";
            lastGroup = shortcut.group;
         }
         text += shortcut.key;
         if (shortcut.key.length() < 24)
            text.append(24 - shortcut.key.length(), ' ');
         else
            text += "  ";
         text += shortcut.action;
         text += "\r\n";
      }
   }

   text += "\r\nViewport mouse controls\r\n"
           "-----------------------\r\n"
           "Left-click              Select a brush/object (or its face in a 3D view)\r\n"
           "Double left-click       Select without starting brush creation\r\n"
           "Shift+double-click      Toggle brush/object in the current multibrush group\r\n"
           "Alt+double-click        Select a terrain-brush vertex or edge in a 2D view\r\n"
           "Left-drag empty 2D      Create a brush; Esc cancels while dragging\r\n"
           "Shift+Left-drag         Move selected brush on the two view axes\r\n"
           "Shift+Right-drag        Move selected brush along the view-normal axis\r\n"
           "Ctrl+Left-drag          Resize selected brush on the two view axes\r\n"
           "Ctrl+Right-drag         Resize selected brush along the view-normal axis\r\n"
           "Alt+Left-drag           Rotate selected brush around the view-normal axis\r\n"
           "Alt+Right-drag          Rotate selected brush around both view axes\r\n"
           "Right-click             Open the viewport context menu\r\n"
           "Left-click after Pan    Place/pan the selected 2D viewport\r\n"
           "Drag pane divider       Resize the editor view panes\r\n";

   text += "\r\nKeyboard bindings come from default.bnd/user.bnd when present, with "
           "DromEd's built-in original defaults used when retail data omits "
           "default.bnd. Mouse gestures are handled directly by the editor.\r\n";
   return text;
}

#define kShortcutEditControl 100

static LRESULT CALLBACK ShortcutWindowProc(HWND hWnd, UINT message,
                                           WPARAM wParam, LPARAM lParam)
{
   switch (message)
   {
      case WM_CREATE:
      {
         CREATESTRUCTA *create = (CREATESTRUCTA *)lParam;
         HWND edit = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT",
            (const char *)create->lpCreateParams,
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL |
            ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL | ES_AUTOHSCROLL |
            ES_READONLY,
            0, 0, 0, 0, hWnd, (HMENU)kShortcutEditControl,
            GetModuleHandle(NULL), NULL);
         HWND close = CreateWindowExA(0, "BUTTON", "Close",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON,
            0, 0, 0, 0, hWnd, (HMENU)IDOK, GetModuleHandle(NULL), NULL);
         SendMessage(edit, WM_SETFONT, (WPARAM)GetStockObject(ANSI_FIXED_FONT),
                     TRUE);
         SendMessage(edit, EM_SETSEL, 0, 0);
         SendMessage(close, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT),
                     TRUE);
         return 0;
      }

      case WM_SIZE:
      {
         int width = LOWORD(lParam);
         int height = HIWORD(lParam);
         MoveWindow(GetDlgItem(hWnd, kShortcutEditControl), 10, 10,
                    max(1, width - 20), max(1, height - 56), TRUE);
         MoveWindow(GetDlgItem(hWnd, IDOK), max(10, width - 100),
                    max(10, height - 36), 90, 26, TRUE);
         return 0;
      }

      case WM_GETMINMAXINFO:
      {
         MINMAXINFO *info = (MINMAXINFO *)lParam;
         info->ptMinTrackSize.x = 480;
         info->ptMinTrackSize.y = 320;
         return 0;
      }

      case WM_COMMAND:
         if (LOWORD(wParam) == IDOK)
         {
            DestroyWindow(hWnd);
            return 0;
         }
         break;

      case WM_CLOSE:
         DestroyWindow(hWnd);
         return 0;

      case WM_DESTROY:
         g_hShortcutWindow = NULL;
         return 0;
   }
   return DefWindowProc(hWnd, message, wParam, lParam);
}

static void ShowKeyboardShortcuts()
{
   static const char kShortcutWindowClass[] = "DromEdShortcutHelp";
   static BOOL registered;
   AutoAppIPtr(WinApp);
   HWND owner = pWinApp->GetMainWnd();

   if (g_hShortcutWindow && IsWindow(g_hShortcutWindow))
   {
      ShowWindow(g_hShortcutWindow, SW_RESTORE);
      SetForegroundWindow(g_hShortcutWindow);
      return;
   }

   if (!registered)
   {
      WNDCLASSA wc;
      memset(&wc, 0, sizeof(wc));
      wc.lpfnWndProc = ShortcutWindowProc;
      wc.hInstance = GetModuleHandle(NULL);
      wc.hCursor = LoadCursor(NULL, IDC_ARROW);
      wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
      wc.lpszClassName = kShortcutWindowClass;
      registered = RegisterClassA(&wc) != 0 ||
                   GetLastError() == ERROR_CLASS_ALREADY_EXISTS;
   }
   if (!registered)
      return;

   std::string shortcuts = BuildShortcutText();
   RECT ownerRect = { 0, 0, 900, 650 };
   GetWindowRect(owner, &ownerRect);
   int width = min(900, max(600, ownerRect.right - ownerRect.left - 80));
   int height = min(650, max(420, ownerRect.bottom - ownerRect.top - 80));
   int x = ownerRect.left + ((ownerRect.right - ownerRect.left) - width) / 2;
   int y = ownerRect.top + ((ownerRect.bottom - ownerRect.top) - height) / 2;

   g_hShortcutWindow = CreateWindowExA(WS_EX_TOOLWINDOW,
      kShortcutWindowClass, "DromEd Keyboard Shortcuts",
      WS_OVERLAPPEDWINDOW | WS_VISIBLE,
      x, y, width, height, owner, NULL, GetModuleHandle(NULL),
      (void *)shortcuts.c_str());
}

struct sRegisteredCommandHelp
{
   std::string name;
   std::string description;
   enum CommandType type;
   bool active;
};

static BOOL LGAPI CollectRegisteredCommand(const Command *command, void *data)
{
   std::vector<sRegisteredCommandHelp> *commands =
      (std::vector<sRegisteredCommandHelp> *)data;
   if (!command || !command->name || !*command->name)
      return TRUE;

   sRegisteredCommandHelp entry;
   entry.name = command->name;
   entry.description = command->comment ? command->comment : "";
   entry.type = command->type;
   entry.active = command_context_ptr &&
                  ((command->contexts & *command_context_ptr) != 0);
   commands->push_back(entry);
   return TRUE;
}

static const char *CommandTypeName(enum CommandType type)
{
   static const char *names[] = {
      "command", "bool argument", "integer argument", "number argument",
      "number argument", "text argument", "boolean variable",
      "integer variable", "text variable", "integer array",
      "number variable", "boolean toggle", "integer toggle",
   };
   return type >= FUNC_VOID && type <= TOGGLE_INT ? names[type] : "unknown";
}

static std::string BuildCommandHelpText(const char *filterText)
{
   std::vector<sRegisteredCommandHelp> commands;
   std::string filter = Lowercase(filterText ? filterText : "");
   std::string text;
   size_t shown = 0;

   CommandForEach(CollectRegisteredCommand, &commands);
   std::stable_sort(commands.begin(), commands.end(),
      [](const sRegisteredCommandHelp &a, const sRegisteredCommandHelp &b) {
         return _stricmp(a.name.c_str(), b.name.c_str()) < 0;
      });
   commands.erase(std::unique(commands.begin(), commands.end(),
      [](const sRegisteredCommandHelp &a, const sRegisteredCommandHelp &b) {
         return !_stricmp(a.name.c_str(), b.name.c_str());
      }), commands.end());

   text = "DromEd command reference\r\n"
          "========================\r\n\r\n"
          "Enter commands in DromEd's command pane (the default shortcuts are : and ; ).\r\n"
          "The console command 'help <text>' also prints matching entries; "
          "'dump_cmds <file>' writes the registry to a file.\r\n\r\n";

   for (const auto &command : commands)
   {
      std::string searchable = Lowercase(command.name + " " +
                               command.description + " " +
                               CommandTypeName(command.type));
      if (!filter.empty() && searchable.find(filter) == std::string::npos)
         continue;

      ++shown;
      text += command.name;
      if (command.name.length() < 32)
         text.append(32 - command.name.length(), ' ');
      else
         text += "  ";
      text += "[";
      text += CommandTypeName(command.type);
      text += "]";
      if (!command.active)
         text += " [inactive in current mode]";
      if (!command.description.empty())
      {
         text += "\r\n    ";
         text += command.description;
      }
      text += "\r\n";
   }

   text += "\r\n";
   text += std::to_string(shown);
   text += " of ";
   text += std::to_string(commands.size());
   text += " registered commands shown.\r\n";
   return text;
}

#define kCommandFilterControl 200
#define kCommandTextControl 201

static void RefreshCommandHelp(HWND hWnd)
{
   char filter[256];
   HWND filterControl = GetDlgItem(hWnd, kCommandFilterControl);
   HWND textControl = GetDlgItem(hWnd, kCommandTextControl);
   GetWindowTextA(filterControl, filter, sizeof(filter));
   std::string text = BuildCommandHelpText(filter);
   SetWindowTextA(textControl, text.c_str());
   SendMessage(textControl, EM_SETSEL, 0, 0);
   SendMessage(textControl, EM_SCROLLCARET, 0, 0);
}

static LRESULT CALLBACK CommandHelpWindowProc(HWND hWnd, UINT message,
                                              WPARAM wParam, LPARAM lParam)
{
   switch (message)
   {
      case WM_CREATE:
      {
         HWND label = CreateWindowExA(0, "STATIC", "Filter:",
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            10, 14, 46, 20, hWnd, NULL, GetModuleHandle(NULL), NULL);
         HWND filter = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
            0, 0, 0, 0, hWnd, (HMENU)kCommandFilterControl,
            GetModuleHandle(NULL), NULL);
         HWND text = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "",
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL |
            ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL | ES_AUTOHSCROLL |
            ES_READONLY,
            0, 0, 0, 0, hWnd, (HMENU)kCommandTextControl,
            GetModuleHandle(NULL), NULL);
         HWND close = CreateWindowExA(0, "BUTTON", "Close",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON,
            0, 0, 0, 0, hWnd, (HMENU)IDOK, GetModuleHandle(NULL), NULL);
         SendMessage(label, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT),
                     TRUE);
         SendMessage(filter, WM_SETFONT,
                     (WPARAM)GetStockObject(DEFAULT_GUI_FONT), TRUE);
         SendMessage(text, WM_SETFONT, (WPARAM)GetStockObject(ANSI_FIXED_FONT),
                     TRUE);
         SendMessage(text, EM_SETLIMITTEXT, 2 * 1024 * 1024, 0);
         SendMessage(close, WM_SETFONT,
                     (WPARAM)GetStockObject(DEFAULT_GUI_FONT), TRUE);
         RefreshCommandHelp(hWnd);
         SetFocus(filter);
         return 0;
      }

      case WM_SIZE:
      {
         int width = LOWORD(lParam);
         int height = HIWORD(lParam);
         MoveWindow(GetDlgItem(hWnd, kCommandFilterControl), 62, 10,
                    max(1, width - 72), 24, TRUE);
         MoveWindow(GetDlgItem(hWnd, kCommandTextControl), 10, 44,
                    max(1, width - 20), max(1, height - 90), TRUE);
         MoveWindow(GetDlgItem(hWnd, IDOK), max(10, width - 100),
                    max(10, height - 36), 90, 26, TRUE);
         return 0;
      }

      case WM_GETMINMAXINFO:
      {
         MINMAXINFO *info = (MINMAXINFO *)lParam;
         info->ptMinTrackSize.x = 560;
         info->ptMinTrackSize.y = 360;
         return 0;
      }

      case WM_COMMAND:
         if (LOWORD(wParam) == kCommandFilterControl &&
             HIWORD(wParam) == EN_CHANGE)
         {
            RefreshCommandHelp(hWnd);
            return 0;
         }
         if (LOWORD(wParam) == IDOK)
         {
            DestroyWindow(hWnd);
            return 0;
         }
         break;

      case WM_CLOSE:
         DestroyWindow(hWnd);
         return 0;

      case WM_DESTROY:
         g_hCommandWindow = NULL;
         return 0;
   }
   return DefWindowProc(hWnd, message, wParam, lParam);
}

static void ShowCommandHelp()
{
   static const char kCommandWindowClass[] = "DromEdCommandHelp";
   static BOOL registered;
   AutoAppIPtr(WinApp);
   HWND owner = pWinApp->GetMainWnd();

   if (g_hCommandWindow && IsWindow(g_hCommandWindow))
   {
      ShowWindow(g_hCommandWindow, SW_RESTORE);
      SetForegroundWindow(g_hCommandWindow);
      SetFocus(GetDlgItem(g_hCommandWindow, kCommandFilterControl));
      return;
   }

   if (!registered)
   {
      WNDCLASSA wc;
      memset(&wc, 0, sizeof(wc));
      wc.lpfnWndProc = CommandHelpWindowProc;
      wc.hInstance = GetModuleHandle(NULL);
      wc.hCursor = LoadCursor(NULL, IDC_ARROW);
      wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
      wc.lpszClassName = kCommandWindowClass;
      registered = RegisterClassA(&wc) != 0 ||
                   GetLastError() == ERROR_CLASS_ALREADY_EXISTS;
   }
   if (!registered)
      return;

   RECT ownerRect = { 0, 0, 960, 700 };
   GetWindowRect(owner, &ownerRect);
   int width = min(960, max(640, ownerRect.right - ownerRect.left - 80));
   int height = min(700, max(460, ownerRect.bottom - ownerRect.top - 80));
   int x = ownerRect.left + ((ownerRect.right - ownerRect.left) - width) / 2;
   int y = ownerRect.top + ((ownerRect.bottom - ownerRect.top) - height) / 2;

   // CreateWindowEx returns immediately and never disables the editor owner:
   // this and the shortcut reference are deliberately modeless working aids.
   g_hCommandWindow = CreateWindowExA(WS_EX_TOOLWINDOW,
      kCommandWindowClass, "DromEd Command Reference",
      WS_OVERLAPPEDWINDOW | WS_VISIBLE,
      x, y, width, height, owner, NULL, GetModuleHandle(NULL), NULL);
}

static Command g_EditorHelpCommands[] = {
   {"show_keyboard_shortcuts", FUNC_VOID, ShowKeyboardShortcuts,
    "open the modeless keyboard and mouse reference", HK_EDITOR},
   {"show_command_help", FUNC_VOID, ShowCommandHelp,
     "open the modeless searchable command reference", HK_ALL},
};

static void RegisterEditorHelpCommands()
{
   static BOOL registered;
   if (!registered)
   {
      COMMANDS(g_EditorHelpCommands, HK_EDITOR);
      registered = TRUE;
   }
}

static void AppendHelpMenu(cMenuSet &menuSet)
{
   menuSet.BeginMenu("&Help");
   menuSet.AddItem("&Keyboard Shortcuts...",
                   g_MenuCommands.NewCommand("show_keyboard_shortcuts"));
   menuSet.AddItem("&Command Reference...",
                   g_MenuCommands.NewCommand("show_command_help"));
   menuSet.EndMenu();
}

///////////////////////////////////////////////////////////////////////////////

#define kMaxMenuConfigEntry 512

void ParseMenu(const char * pszMenuText, const char * pszMenuTag, cMenuSet & menuSet, cMenusInProgress & menusInProgress)
{
   cStr menuDefStr;
   BOOL foundMenu = config_get_raw(pszMenuTag,
                                   menuDefStr.GetBuffer(kMaxMenuConfigEntry),
                                   kMaxMenuConfigEntry);
   menuDefStr.ReleaseBuffer();
   menuDefStr.Trim();

   if (foundMenu && !menuDefStr.IsEmpty())
   {
      if (menusInProgress.Lookup(pszMenuTag, &foundMenu))
         return;

      menusInProgress.Insert(pszMenuTag, TRUE);
      menuSet.BeginMenu(pszMenuText);

      cStr continuationMenuTag(pszMenuTag);
      int  iCurrentPart = 0;

      continuationMenuTag += "_0";

      while (foundMenu)
      {
         int  indexNextEntry = 0;
         int  indexSemicolon = 0;

         cStr menuEntry;
         cStr menuText;
         cStr menuValue;

         while (menuDefStr[indexNextEntry])
         {
            // Find semicolon
            indexSemicolon = indexNextEntry + menuDefStr.SpanExcluding("|", indexNextEntry);

            // If the string is non-zero, deal with it
            if (indexSemicolon - indexNextEntry != 0)
            {
               menuDefStr.Mid(menuEntry, indexNextEntry, indexSemicolon - indexNextEntry);

               int indexColon = menuEntry.Find(':');
               if (indexColon != -1)
               {
                  menuEntry.Mid(menuText, 0, indexColon);
                  menuEntry.Mid(menuValue, indexColon + 1, menuEntry.GetLength() - (indexColon + 1));
               }
               else
               {
                  menuText = menuEntry;
                  menuValue = menuEntry;
               }
               menuText.Trim();
               menuValue.Trim();

               if (menuValue.Find("menu_") == 0)
               {
                  ParseMenu(menuText, menuValue, menuSet, menusInProgress);
               }
               else if (menuText.Find("sep") == 0)
               {
                  menuSet.AddSeparator();
               }
               else
               {
                   RecordDeclaredShortcut(menuText, menuValue);
                  menuSet.AddItem(menuText, g_MenuCommands.NewCommand(menuValue));
               }

            }

            menuEntry.Empty();
            menuText.Empty();
            menuValue.Empty();

            // Skip to the next non-semicolon
            indexNextEntry = indexSemicolon + menuDefStr.SpanIncluding("|", indexSemicolon);
         }
         iCurrentPart++;
         continuationMenuTag[continuationMenuTag.GetLength() - 1] = '0' + iCurrentPart;

         foundMenu = config_get_raw(continuationMenuTag,
                                    menuDefStr.GetBuffer(kMaxMenuConfigEntry),
                                    kMaxMenuConfigEntry);
         menuDefStr.ReleaseBuffer();
         menuDefStr.Trim();
      }
      if (!_stricmp(pszMenuTag, "menu_edit"))
         AppendHelpMenu(menuSet);
      menuSet.EndMenu();
      menusInProgress.Delete(pszMenuTag);
   }
   else
      menuSet.AddItem(pszMenuText, 0);
}

///////////////////////////////////////////////////////////////////////////////

BOOL CreateMenu(const char * pszMenuTag)
{
   cMenusInProgress menusInProgress;

   ParseMenu(NULL, pszMenuTag, g_MenuSet, menusInProgress);

   return TRUE;
}

///////////////////////////////////////////////////////////////////////////////

void SetMainMenu(const char * pszName)
{
   AutoAppIPtr(WinApp);

   RegisterEditorHelpCommands();

   HWND hWnd = pWinApp->GetMainWnd();

   g_MenuSet.DetachFromWindow();
   g_MenuSet.DestroyAll();
   g_MenuCommands.ClearAll();
   g_DeclaredShortcuts.clear();

   if (pszName)
   {
      cStr menuNameStr;

      menuNameStr = "menu_";
      menuNameStr += pszName;

      menuNameStr.Trim();

      CreateMenu(menuNameStr);

      g_MenuSet.AttachToWindow(hWnd);
   }
}

///////////////////////////////////////////////////////////////////////////////

void MenuCommand(unsigned id)
{
   const char * pszMenuCommand = g_MenuCommands.Lookup(id);

   if (pszMenuCommand)
      CommandExecute((char *)pszMenuCommand);
}

///////////////////////////////////////////////////////////////////////////////

//  Sets an exclusive radio-button style bullet beside a menu item
//
extern "C"
{
    void SetRadioCheckmark (int menu, int firstPos, int lastPos, int checkPos)
    {
        cWinMenu* pMenu = g_MenuSet.GetMenuByNumber(menu);
        if (pMenu)
           pMenu->CheckMenuRadioItem (firstPos, lastPos, checkPos, MF_BYPOSITION);
    }
}
