// editor_compat.h is force-included by CMake. Keep its Win32-era type names
// from colliding with Objective-C and Carbon declarations in AppKit.
#undef interface
#define BOOL AppKitBOOL
#define Point AppKitPoint
#define Rect AppKitRect
#import <AppKit/AppKit.h>
#undef BOOL
#undef Point
#undef Rect
#define interface struct

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <limits.h>
#include <string>
#include <vector>

#pragma pack(push, 1)
#include <appagg.h>
#include <command.h>
#include <contentctx.h>
#include <config.h>
#include <filemenu.h>
#include <gadget.h>
#include <gcompose.h>
#include <guistyle.h>
#include <render_backend.h>
#include <status.h>
#include <txtrpal.h>
#include <uiedit.h>
#include <viewmgr.h>
#include <vumanui.h>
#include <brushgfh.h>
#include <cmdterm.h>
#include <editor.h>
#include <memall.h>
#include <uiapp.h>
#include <keycmd.h>
#pragma pack(pop)

#undef interface

extern "C" {
void StatusSetRect(Rect *);
void load_file(char *);
void save_world(char *);
void dbBuildDefault(void);
void MenuCommand(unsigned);
void EditorPlatformSetMenuCommandCallback(void (*)(unsigned));
void EditorPlatformMenuCommand(unsigned);
}

guiStyle editStyle;

namespace {
struct EditorMenuCommand {
   std::string command;
};
struct Layout { Point dims; Id id; };

std::vector<EditorMenuCommand> g_menuCommands;
std::vector<NSMenu *> g_menusByNumber;
id g_menuTarget;
int g_pendingWidth;
int g_pendingHeight;
bool g_resizeTracking;
bool g_havePreviousSize;

NSString *nsstring(const char *value)
{
   return [NSString stringWithUTF8String:value ? value : ""];
}

std::string menuTitle(const std::string &title)
{
   std::string result;
   result.reserve(title.size());
   for (size_t i = 0; i < title.size(); ++i) {
      if (title[i] == '&' && i + 1 < title.size())
         continue;
      result.push_back(title[i]);
   }
   return result;
}

std::string menuConfig(const std::string &tag)
{
   char buffer[8192] = {};
   if (!config_get_raw(tag.c_str(), buffer, sizeof(buffer)))
      return {};
   return buffer;
}

void addCommand(NSMenu *menu, const std::string &title,
                const std::string &command)
{
   const NSUInteger index = g_menuCommands.size();
   g_menuCommands.push_back({command});
   NSMenuItem *item = [[NSMenuItem alloc]
      initWithTitle:nsstring(menuTitle(title).c_str())
             action:@selector(runEditorCommand:)
      keyEquivalent:@""];
   [item setTarget:g_menuTarget];
   [item setTag:(NSInteger)index];
   [menu addItem:item];
   [item release];
}

void buildMenu(NSMenu *parent, const std::string &label,
               const std::string &tag, std::vector<std::string> &stack)
{
   if (std::find(stack.begin(), stack.end(), tag) != stack.end())
      return;
   stack.push_back(tag);

   std::string definition = menuConfig(tag);
   if (definition.empty()) {
      stack.pop_back();
      return;
   }

   const bool rootMenu = label.empty();
   const std::string visibleLabel = menuTitle(label);
   NSMenu *submenu = rootMenu ? parent : [[NSMenu alloc] initWithTitle:nsstring(visibleLabel.c_str())];
   g_menusByNumber.push_back(submenu);
   for (int part = 0; !definition.empty() && part < 10; ++part) {
      size_t begin = 0;
      while (begin < definition.size()) {
         size_t end = definition.find('|', begin);
         if (end == std::string::npos)
            end = definition.size();
         std::string entry = definition.substr(begin, end - begin);
         begin = end + 1;
         if (entry.empty())
            continue;

         const size_t colon = entry.find(':');
         std::string text = colon == std::string::npos ? entry : entry.substr(0, colon);
         std::string value = colon == std::string::npos ? entry : entry.substr(colon + 1);
         auto trim = [](std::string &s) {
            const size_t first = s.find_first_not_of(" \t\r\n");
            const size_t last = s.find_last_not_of(" \t\r\n");
            s = first == std::string::npos ? std::string() : s.substr(first, last - first + 1);
         };
         trim(text);
         trim(value);
         if (value.empty())
            continue;

         if (value.compare(0, 5, "menu_") == 0) {
            buildMenu(submenu, text, value, stack);
         } else if (text.compare(0, 3, "sep") == 0) {
            [submenu addItem:[NSMenuItem separatorItem]];
         } else {
            addCommand(submenu, text, value);
         }
      }

      char suffix[4];
      std::snprintf(suffix, sizeof(suffix), "_%d", part + 1);
      definition = menuConfig(tag + suffix);
   }

   if (!rootMenu) {
      NSMenuItem *parentItem = [[NSMenuItem alloc]
         initWithTitle:nsstring(visibleLabel.c_str()) action:nil keyEquivalent:@""];
      [parentItem setSubmenu:submenu];
      [parent addItem:parentItem];
      [parentItem release];
      [submenu release];
   }
   stack.pop_back();
}

void addNativeMenuItem(NSMenu *menu, NSString *title, SEL action,
                       id target)
{
   NSMenuItem *item = [[NSMenuItem alloc] initWithTitle:title
      action:action keyEquivalent:@""];
   [item setTarget:target];
   [menu addItem:item];
   [item release];
}

void buildFallbackMenu(NSMenu *bar, NSApplication *application)
{
   NSMenu *appMenu = [[NSMenu alloc] initWithTitle:@"DromEd"];
   addNativeMenuItem(appMenu, @"About DromEd",
                     @selector(orderFrontStandardAboutPanel:), application);
   [appMenu addItem:[NSMenuItem separatorItem]];
   addNativeMenuItem(appMenu, @"Quit DromEd", @selector(terminate:), application);
   NSMenuItem *appItem = [[NSMenuItem alloc] initWithTitle:@"DromEd"
      action:nil keyEquivalent:@""];
   [appItem setSubmenu:appMenu];
   [bar addItem:appItem];
   [appItem release];
   [appMenu release];

   struct MenuDefinition {
      const char *title;
      const char *items[5];
      const char *commands[5];
   } menus[] = {
      {"File", {"Load World…", "Save Mission", "Clear World", nullptr},
       {"edit_command load_file", "eval world_file edit_command save_mission %s",
        "edit_command clear_world", nullptr}},
      {"Edit", {"Undo", "Redo", nullptr}, {"undo", "redo", nullptr}},
      {"View", {"Redraw All", "Texture Palette", nullptr},
       {"redraw_all", "texture_pal", nullptr}},
      {"Play", {"Enter Game Preview", "Return to Editor", nullptr},
       {"game_mode", "edit_mode", nullptr}},
   };
   for (const MenuDefinition &definition : menus) {
      NSMenu *submenu = [[NSMenu alloc] initWithTitle:nsstring(definition.title)];
      for (int i = 0; definition.items[i]; ++i)
         addCommand(submenu, definition.items[i], definition.commands[i]);
      NSMenuItem *item = [[NSMenuItem alloc]
         initWithTitle:nsstring(definition.title) action:nil keyEquivalent:@""];
      [item setSubmenu:submenu];
      [bar addItem:item];
      [item release];
      [submenu release];
      g_menusByNumber.push_back(submenu);
   }
}

Layout *screen_layout(void)
{
   static Layout layouts[] = {
      {{640, 480}, RES_EditorLayout640x480},
      {{1024, 768}, RES_EditorLayout1024x768},
      {{800, 600}, RES_EditorLayout800x600},
   };
   Region *root = GetRootRegion();
   Point dimensions = MakePoint(RectWidth(root->r), RectHeight(root->r));
   unsigned long bestError = ~0ul;
   size_t best = 0;
   for (size_t i = 0; i < sizeof(layouts) / sizeof(layouts[0]); ++i) {
      const unsigned long error = (unsigned long)abs(dimensions.x - layouts[i].dims.x) *
                                  (unsigned long)abs(dimensions.y - layouts[i].dims.y);
      if (error < bestError) { bestError = error; best = i; }
   }
   return reinterpret_cast<Layout *>(&layouts[best]);
}

Rect *layout_rect(void *layout, int index)
{
   static Rect result;
   Layout *selected = static_cast<Layout *>(layout);
   Region *root = GetRootRegion();
   Point dimensions = MakePoint(RectWidth(root->r), RectHeight(root->r));
   result = *static_cast<Rect *>(RefGet(MKREF(selected->id, index)));
   result.ul.x = result.ul.x * dimensions.x / selected->dims.x;
   result.lr.x = result.lr.x * dimensions.x / selected->dims.x;
   result.ul.y = result.ul.y * dimensions.y / selected->dims.y;
   result.lr.y = result.lr.y * dimensions.y / selected->dims.y;
   return &result;
}
}

@interface DromEdMenuTarget : NSObject
- (void)runEditorCommand:(id)sender;
@end

@implementation DromEdMenuTarget
- (void)runEditorCommand:(id)sender
{
   const NSInteger index = [sender tag];
   if (index >= 0 && (size_t)index < g_menuCommands.size())
      CommandExecute(const_cast<char *>(g_menuCommands[index].command.c_str()));
}
@end

extern "C" void EditorCreateGUI(void)
{
   Region *root = GetRootRegion();
   void *layout = screen_layout();
   GUIErase(root->r);
   uieditStyleSetup();
   SetCurrentStyle(&editStyle);
   vmCreateGUI(root, layout_rect(layout, REFINDEX(REF_RECT_layViewMan)));
   StatusSetRect(layout_rect(layout, REFINDEX(REF_RECT_layStatus)));
   CreateBrushGFH(layout_rect(layout, REFINDEX(REF_RECT_layGFH)));
   CreateCommandTerminal(LGadCurrentRoot(),
      layout_rect(layout, REFINDEX(REF_RECT_layCommand)),
      kCmdTermScrollable | kCmdTermNoPause);
}

extern "C" void EditorDestroyGUI(void)
{
   if (TexturePaletteVisible())
      DestroyTexturePalette();
   DestroyCommandTerminal();
   DestroyBrushGFH();
   vmDestroyGUI();
   uieditStyleCleanup();
}

extern "C" void EditorStartWindowResizeTracking(void)
{
   g_resizeTracking = true;
   g_havePreviousSize = false;
}

extern "C" void EditorStopWindowResizeTracking(void)
{
   g_resizeTracking = false;
   g_havePreviousSize = false;
}

extern "C" void EditorRestoreWindowRect(void)
{
   // The Cocoa menu bar is outside the SDL client area, so installing the
   // editor menu does not change the window's saved frame.
}

extern "C" int EditorGetPendingWindowSize(int *width, int *height)
{
   int currentWidth, currentHeight;
   if (!g_resizeTracking || !width || !height ||
       !RenderBackendGetClientSize(&currentWidth, &currentHeight) ||
       (currentWidth < 400 || currentHeight < 300))
      return FALSE;
   if (g_havePreviousSize && currentWidth == g_pendingWidth &&
       currentHeight == g_pendingHeight)
      return FALSE;
   g_pendingWidth = currentWidth;
   g_pendingHeight = currentHeight;
   g_havePreviousSize = true;
   *width = currentWidth;
   *height = currentHeight;
   return TRUE;
}

extern "C" void uieditStyleSetup(void)
{
   guiStyle style = {};
   style.colors[0] = uiRGB(255,255,255);
   style.colors[1] = uiRGB(10,10,10);
   style.colors[2] = uiRGB(40,200,200);
   style.colors[3] = uiRGB(255,0,255);
   style.colors[4] = uiRGB(255,255,255);
   style.colors[5] = uiRGB(96,96,96);
   style.colors[6] = uiRGB(255,255,255);
   style.colors[7] = uiRGB(64,64,64);
   style.colors[8] = uiRGB(40,200,200);
   style.colors[9] = uiRGB(255,255,255);
   style.colors[10] = uiRGB(5,5,5);
   uiGameLoadStyle("edit_", &style, nullptr);
   guiCompileStyleColors(&editStyle, &style);
   SetCurrentStyle(&editStyle);
}

extern "C" void uieditStyleCleanup(void)
{
   uiGameUnloadStyle(&editStyle);
}

extern "C" void uieditRedrawAll(void)
{
   LGadDrawBox(VB(LGadCurrentRoot()), nullptr);
}

extern "C" void redraw_all_cmd(void)
{
   uieditRedrawAll();
   vm_redraw();
   StatusDrawStringAll();
}

extern "C" void SetMainMenu(const char *name)
{
   NSApplication *application = [NSApplication sharedApplication];
   [application setActivationPolicy:NSApplicationActivationPolicyRegular];
   EditorPlatformSetMenuCommandCallback(EditorPlatformMenuCommand);
   if (!g_menuTarget)
      g_menuTarget = [[DromEdMenuTarget alloc] init];
   NSMenu *bar = [[NSMenu alloc] initWithTitle:@""];
   g_menuCommands.clear();
   g_menusByNumber.clear();
   if (name) {
      std::vector<std::string> stack;
      buildMenu(bar, "", std::string("menu_") + name, stack);
      if ([bar numberOfItems] == 0)
         buildFallbackMenu(bar, application);
   }
   [application setMainMenu:bar];
   [application activateIgnoringOtherApps:YES];
   [bar release];
}

extern "C" void EditorPlatformMenuCommand(unsigned command)
{
   if (command < g_menuCommands.size())
      CommandExecute(const_cast<char *>(g_menuCommands[command].command.c_str()));
}

extern "C" void SetRadioCheckmark(int menu, int first, int last, int checked)
{
   if (menu < 0 || (size_t)menu >= g_menusByNumber.size())
      return;
   NSMenu *target = g_menusByNumber[menu];
   for (int i = std::max(0, first); i <= last && i < (int)[target numberOfItems]; ++i)
      [[target itemAtIndex:i] setState:(i == checked ? NSControlStateValueOn : NSControlStateValueOff)];
}

extern "C" char *FileDialog(eFileMenuMode mode, sFileMenuFilter *filters,
                              const char *, char *buffer, int length)
{
   if (!buffer || length <= 0)
      return nullptr;
   NSSavePanel *panel = mode == kFileMenuLoad ? [NSOpenPanel openPanel] : [NSSavePanel savePanel];
   [panel setCanCreateDirectories:YES];
   [panel setNameFieldStringValue:nsstring(buffer)];
   std::vector<NSString *> extensions;
   if (filters) {
      for (sFileMenuFilter *filter = filters; filter->text; ++filter) {
         std::string pattern = filter->pattern ? filter->pattern : "";
         size_t start = 0;
         while (start < pattern.size()) {
            size_t end = pattern.find(';', start);
            if (end == std::string::npos) end = pattern.size();
            std::string item = pattern.substr(start, end - start);
            const size_t dot = item.rfind('.');
            if (dot != std::string::npos && dot + 1 < item.size())
               extensions.push_back(nsstring(item.substr(dot + 1).c_str()));
            start = end + 1;
         }
      }
   }
   if (extensions.size()) {
      NSMutableArray *allowedTypes = [NSMutableArray arrayWithCapacity:extensions.size()];
      for (NSString *extension : extensions)
         [allowedTypes addObject:extension];
      [panel setAllowedFileTypes:allowedTypes];
   }
   const char *initial = DarkContentFMPath();
   if (!initial || !initial[0]) initial = DarkContentBasePath();
   if (initial && initial[0])
      [panel setDirectoryURL:[NSURL fileURLWithPath:nsstring(initial) isDirectory:YES]];
   NSInteger response = [panel runModal];
   if (response != NSModalResponseOK)
      return nullptr;
   const char *path = [[[panel URL] path] UTF8String];
   if (!path)
      return nullptr;
   std::strncpy(buffer, path, length - 1);
   buffer[length - 1] = '\0';
   return buffer;
}

extern "C" void FileMenu(void)
{
   NSAlert *alert = [[NSAlert alloc] init];
   [alert setMessageText:@"DromEd File Menu"];
   [alert addButtonWithTitle:@"Load World"];
   [alert addButtonWithTitle:@"Save World"];
   [alert addButtonWithTitle:@"Clear World"];
   [alert addButtonWithTitle:@"Cancel"];
   NSInteger result = [alert runModal];
   [alert release];
   char path[PATH_MAX] = {};
   if (result == NSAlertFirstButtonReturn && FileDialog(kFileMenuLoad, nullptr, nullptr, path, sizeof(path)))
      load_file(path);
   else if (result == NSAlertFirstButtonReturn + 1 && FileDialog(kFileMenuSave, nullptr, nullptr, path, sizeof(path)))
      save_world(path);
   else if (result == NSAlertFirstButtonReturn + 2) {
      NSAlert *confirm = [[NSAlert alloc] init];
      [confirm setMessageText:@"Clear the current world?"];
      [confirm addButtonWithTitle:@"Clear"];
      [confirm addButtonWithTitle:@"Cancel"];
      if ([confirm runModal] == NSAlertFirstButtonReturn)
         dbBuildDefault();
      [confirm release];
   }
}

extern "C" BOOL winui_GetYorN(const char *message)
{
   NSAlert *alert = [[NSAlert alloc] init];
   [alert setMessageText:nsstring(message)];
   [alert addButtonWithTitle:@"No"];
   [alert addButtonWithTitle:@"Yes"];
   BOOL result = [alert runModal] == NSAlertSecondButtonReturn;
   [alert release];
   return result;
}

extern "C" void winui_Text(const char *message)
{
   NSAlert *alert = [[NSAlert alloc] init];
   [alert setMessageText:nsstring(message)];
   [alert addButtonWithTitle:@"OK"];
   [alert runModal];
   [alert release];
}
