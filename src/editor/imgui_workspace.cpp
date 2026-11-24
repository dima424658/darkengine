#include <algorithm>
#include <cmath>
#include <functional>
#include <cstdio>
#include <string>
#include <optional>
#include <utility>
#include <vector>

#include <SDL3/SDL.h>
#include <imgui.h>
#include <imgui_internal.h>

#include <lg.h>
#include <darkdlgs/darkdlgs.h>
#include <cmdterm.h>
#include <brushgfh.h>
#include <imgui_pnp.h>
#include <status.h>
#include <txtrpal.h>
#include <imgui_workspace.h>
#include <mediface.h>
#include <region.h>
#include <render_backend.h>
#include <uiedit.h>
#include <viewmgr.h>
#include <viewmgr_.h>
#include <vmwincfg.h>
#include <vumanui.h>

namespace
{
constexpr ImGuiWindowFlags kWorkspaceWindow =
   ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoDocking |
   ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings |
   ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

std::vector<std::function<void()>> actions;
std::optional<Point> pendingSplit;
std::optional<Point> pendingWorkspaceSplit;
std::vector<Rect> cameraDividers;
std::vector<Rect> workspaceDividers;
bool resizing = false;
bool resizeCancelled = false;
Point resizeStart;
Point resizeMouseStart;
std::optional<Point> *activeResize;
bool consoleFocusRequested = false;
bool consoleUnfocusRequested = false;
bool consoleFocused = false;
float consoleFontSize = 13.0f;
char consoleInput[CMDTERM_LINE_LENGTH]{};
char consoleLines[CMDTERM_MAX_LINES][CMDTERM_LINE_LENGTH]{};
unsigned consoleRevision = 0;
int consoleLineCount = 0;
bool consoleFollowOutput = true;

void FocusConsole(const char *prefix, BOOL toggle)
{
   if (toggle && (consoleFocused || consoleFocusRequested))
   {
      consoleUnfocusRequested = true;
      consoleFocusRequested = false;
      return;
   }
   std::snprintf(consoleInput, sizeof(consoleInput), "%s", prefix ? prefix : "");
   consoleFocusRequested = true;
   consoleUnfocusRequested = false;
}

int ConsoleInputCallback(ImGuiInputTextCallbackData *data)
{
   const char *value = nullptr;
   if (data->EventFlag == ImGuiInputTextFlags_CallbackCompletion)
      value = cmdterm_editor_complete(data->Buf);
   else if (data->EventFlag == ImGuiInputTextFlags_CallbackHistory)
      value = cmdterm_editor_history(data->EventKey == ImGuiKey_UpArrow ? -1 : 1);

   if (value)
   {
      data->DeleteChars(0, data->BufTextLen);
      data->InsertChars(0, value);
   }
   return 0;
}

ImVec2 ToImGui(Point point)
{
   int x = point.x;
   int y = point.y;
   RenderBackendLogicalToClientPoint(&x, &y);
   const ImVec2 origin = ImGui::GetMainViewport()->Pos;
   return ImVec2(origin.x + x, origin.y + y);
}

Point ToLogical(float x, float y)
{
   int logicalX = static_cast<int>(std::floor(x));
   int logicalY = static_cast<int>(std::floor(y));
   RenderBackendClientToLogicalPoint(&logicalX, &logicalY);
   return MakePoint(logicalX, logicalY);
}

bool Contains(const Rect &rect, Point point)
{
   return point.x >= rect.ul.x && point.x < rect.lr.x &&
          point.y >= rect.ul.y && point.y < rect.lr.y;
}

bool BeginWorkspaceWindowImpl(const char *name, const Rect &rect, ImGuiWindowFlags flags = 0)
{
   const ImVec2 position = ToImGui(rect.ul);
   const ImVec2 end = ToImGui(rect.lr);
   ImGui::SetNextWindowViewport(ImGui::GetMainViewport()->ID);
   ImGui::SetNextWindowPos(position);
   ImGui::SetNextWindowSize(ImVec2(std::max(1.0f, end.x - position.x),
                                  std::max(1.0f, end.y - position.y)));
   return ImGui::Begin(name, nullptr, kWorkspaceWindow | flags);
}

bool DrawResizeHandle(const char *name, const Rect &rect, Point split, bool resizeWidth, bool resizeHeight,
                      std::optional<Point> &pending)
{
   bool dragging = false;
   ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
   if (BeginWorkspaceWindowImpl(name, rect, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollbar))
   {
      // These hit areas overlap the panel edges on both sides of the divider.
      if (!DarkDlgsModalActive() && !ImGui::IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopup))
         ImGui::BringWindowToDisplayFront(ImGui::GetCurrentWindow());
      ImGui::InvisibleButton("##resize", ImGui::GetContentRegionAvail());
      dragging = ImGui::IsItemActive();
      if (ImGui::IsItemHovered() || dragging)
      {
         ImGui::SetMouseCursor(resizeWidth && resizeHeight ? ImGuiMouseCursor_ResizeAll :
                              resizeWidth ? ImGuiMouseCursor_ResizeEW : ImGuiMouseCursor_ResizeNS);
         ImGui::GetWindowDrawList()->AddRectFilled(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(),
            ImGui::GetColorU32(dragging ? ImGuiCol_SeparatorActive : ImGuiCol_SeparatorHovered));
      }
      if (dragging)
      {
         const ImVec2 mouse = ImGui::GetMousePos();
         const ImVec2 origin = ImGui::GetMainViewport()->Pos;
         const Point point = ToLogical(mouse.x - origin.x, mouse.y - origin.y);
         if (ImGui::IsItemActivated())
         {
            resizeStart = split;
            resizeMouseStart = point;
            resizeCancelled = false;
            activeResize = &pending;
         }
         if (!resizeCancelled)
            pending = MakePoint(resizeStart.x + (resizeWidth ? point.x - resizeMouseStart.x : 0),
                                resizeStart.y + (resizeHeight ? point.y - resizeMouseStart.y : 0));
      }
   }
   ImGui::End();
   ImGui::PopStyleVar();
   return dragging;
}

void DrawCameraDivider(const char *name, const Rect &rect, Point split, bool resizeWidth, bool resizeHeight)
{
   cameraDividers.push_back(rect);
   resizing |= DrawResizeHandle(name, rect, split, resizeWidth, resizeHeight, pendingSplit);
}

void DrawWorkspaceDividers(const sEditorWorkspaceLayout &layout)
{
   workspaceDividers.clear();
   ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
   ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(1, 1));
   const Point split = layout.console.ul;
   constexpr int hitRadius = 4;
   const int consoleEdge = split.x - 1;
   const Rect consoleDivider = {MakePoint(consoleEdge - hitRadius, split.y),
                                MakePoint(consoleEdge + hitRadius, layout.console.lr.y)};
   workspaceDividers.push_back(consoleDivider);
   resizing |= DrawResizeHandle("##ConsoleDivider", consoleDivider, split, true, false, pendingWorkspaceSplit);
   const int bottomEdge = (layout.views.lr.y + split.y) / 2;
   const Rect bottomDivider = {MakePoint(layout.views.ul.x, bottomEdge - hitRadius),
                               MakePoint(layout.console.lr.x, bottomEdge + hitRadius)};
   workspaceDividers.push_back(bottomDivider);
   resizing |= DrawResizeHandle("##BottomDivider", bottomDivider, split, false, true, pendingWorkspaceSplit);
   ImGui::PopStyleVar(2);
}

void DrawCameraPanes()
{
   cameraDividers.clear();
   Rect bounds;
   Point split;
   if (!vmWinCfgGetBounds(&bounds, &split))
      return;

   ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
   ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(1, 1));
   ImDrawList *draw = ImGui::GetBackgroundDrawList(ImGui::GetMainViewport());
   for (int i = 0; i < MAX_CAMERAS_REAL; ++i)
   {
      Point origin, size;
      if (!VIEW_AVAIL(i) || !vmGetRegionInfo(i, &origin, &size))
         continue;
      if (const char *title = vm_get_pane_title(i))
      {
         const ImVec2 top = ToImGui(origin);
         const ImVec2 bottom = ToImGui(MakePoint(origin.x + size.x, origin.y + size.y));
         draw->PushClipRect(top, bottom, true);
         draw->AddText(ImVec2(top.x + 2, top.y + 1), ImGui::GetColorU32(ImGuiCol_Text), title);
         draw->PopClipRect();
      }
   }

   if (!vmSoloed && vmAllViewsAvailable())
   {
      const int x = bounds.ul.x + split.x;
      const int y = bounds.ul.y + split.y;
      const int layout = vmWinCfgGetLayout();
      if (layout == vmWINCFG_2by2)
      {
         DrawCameraDivider("##ViewDividerTop", {MakePoint(x - 2, bounds.ul.y), MakePoint(x + 5, y - 2)}, split, true, false);
         DrawCameraDivider("##ViewDividerBottom", {MakePoint(x - 2, y + 4), MakePoint(x + 5, bounds.lr.y)}, split, true, false);
         DrawCameraDivider("##ViewDividerLeft", {MakePoint(bounds.ul.x, y - 2), MakePoint(x - 2, y + 4)}, split, false, true);
         DrawCameraDivider("##ViewDividerRight", {MakePoint(x + 5, y - 2), MakePoint(bounds.lr.x, y + 4)}, split, false, true);
         DrawCameraDivider("##ViewDividerCenter", {MakePoint(x - 2, y - 2), MakePoint(x + 5, y + 4)}, split, true, true);
      }
      else if (layout == vmWINCFG_1by3_VERT)
         DrawCameraDivider("##ViewDivider", {MakePoint(x - 3, bounds.ul.y), MakePoint(x + 3, bounds.lr.y)}, split, true, false);
      else if (layout == vmWINCFG_1by3_HORIZ)
         DrawCameraDivider("##ViewDivider", {MakePoint(bounds.ul.x, y - 2), MakePoint(bounds.lr.x, y + 2)}, split, false, true);
   }
   ImGui::PopStyleVar(2);
}
void DrawConsole(const Rect &rect)
{
   const int count = cmdterm_editor_output(consoleLines, &consoleRevision);
   if (count >= 0)
      consoleLineCount = count;

   ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(4, 3));
   ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
   if (BeginWorkspaceWindowImpl("##CommandConsole", rect, ImGuiWindowFlags_NoScrollbar))
   {
      if (ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows) && ImGui::GetIO().KeyCtrl)
      {
         ImGui::SetItemKeyOwner(ImGuiKey_MouseWheelY);
         consoleFontSize = std::clamp(consoleFontSize + ImGui::GetIO().MouseWheel * 2.0f, 9.0f, 36.0f);
      }
      ImGui::PushFont(nullptr, consoleFontSize);
      const bool unfocus = std::exchange(consoleUnfocusRequested, false);
      if (unfocus)
      {
         ImGui::SetWindowFocus(nullptr);
         consoleFocused = false;
      }

      const char *suggestion = consoleFocused ? cmdterm_editor_suggestion(consoleInput, 0) : nullptr;
      const float footer = (suggestion ? ImGui::GetTextLineHeightWithSpacing() : 0.0f) +
         ImGui::GetFrameHeightWithSpacing();
      if (ImGui::BeginChild("##CommandOutput", ImVec2(0, -footer), ImGuiChildFlags_None,
                            ImGuiWindowFlags_HorizontalScrollbar))
      {
         const float lineHeight = ImGui::GetTextLineHeightWithSpacing();
         const int first = static_cast<int>(ImGui::GetScrollY() / lineHeight);
         const int visible = static_cast<int>(std::ceil(ImGui::GetWindowHeight() / lineHeight));
         cmdterm_editor_visible_range(first, std::max(0, std::min(visible, consoleLineCount - first)));
         ImGuiListClipper clipper;
         clipper.Begin(consoleLineCount);
         while (clipper.Step())
            for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; ++i)
               ImGui::TextUnformatted(consoleLines[i][0] ? consoleLines[i] : " ");

         if (count >= 0 && consoleFollowOutput)
            ImGui::SetScrollHereY(1.0f);
         else
            consoleFollowOutput = ImGui::GetScrollY() >= ImGui::GetScrollMaxY() - 1.0f;

         if (consoleFocused && ImGui::IsKeyPressed(ImGuiKey_PageUp))
            ImGui::SetScrollY(ImGui::GetScrollY() - ImGui::GetWindowHeight());
         if (consoleFocused && ImGui::IsKeyPressed(ImGuiKey_PageDown))
            ImGui::SetScrollY(ImGui::GetScrollY() + ImGui::GetWindowHeight());

         if (ImGui::BeginPopupContextWindow("##ConsoleCopy"))
         {
            if (ImGui::MenuItem("Copy visible output"))
               actions.emplace_back([] { cmdterm_execute_command("console_copy visible"); });
            if (ImGui::MenuItem("Copy all output"))
               actions.emplace_back([] { cmdterm_execute_command("console_copy all"); });
            ImGui::EndPopup();
         }
      }
      ImGui::EndChild();

      if (suggestion)
         ImGui::TextDisabled("%s    Tab completes", suggestion);

      if (std::exchange(consoleFocusRequested, false))
      {
         ImGui::SetWindowFocus();
         ImGui::SetKeyboardFocusHere();
      }
      ImGui::SetNextItemWidth(-1);
      if (ImGui::InputText("##CommandInput", consoleInput, sizeof(consoleInput),
                           ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackCompletion |
                           ImGuiInputTextFlags_CallbackHistory, ConsoleInputCallback) && consoleInput[0])
      {
         actions.emplace_back([command = std::string(consoleInput)]() mutable {
            cmdterm_editor_execute(command.data());
         });
         consoleInput[0] = '\0';
         consoleFocusRequested = true;
         consoleFollowOutput = true;
      }
      consoleFocused = ImGui::IsItemActive() && !unfocus;
      ImGui::PopFont();
   }
   ImGui::End();
   ImGui::PopStyleVar(2);
}

void DrawStatus(const Rect &rect)
{
   const ImVec2 top = ToImGui(rect.ul);
   const ImVec2 bottom = ToImGui(rect.lr);
   if (bottom.y <= top.y)
      return;

   const float width = bottom.x - top.x;
   ImDrawList *draw = ImGui::GetBackgroundDrawList(ImGui::GetMainViewport());
   draw->AddRectFilled(top, bottom, IM_COL32(255, 255, 255, 255));
   for (int field = 0; field < SF_NUM_FIELDS; ++field)
   {
      const float x = top.x + width * StatusFieldPosition(field) * 0.01f;
      const float end = top.x + width * StatusFieldPosition(field + 1) * 0.01f;
      draw->PushClipRect(ImVec2(x, top.y), ImVec2(end, bottom.y), true);
      draw->AddText(ImVec2(x, top.y + std::max(0.0f, (bottom.y - top.y - ImGui::GetFontSize()) / 2)),
                    IM_COL32(5, 5, 5, 255), StatusGetField(field));
      draw->PopClipRect();
   }
}
}

bool BeginEditorWorkspaceWindow(const char *name, const Rect &rect, unsigned flags)
{
   return BeginWorkspaceWindowImpl(name, rect, flags);
}

void InitializeEditorWorkspace(void)
{
   cmdterm_set_editor_focus(FocusConsole);
}

void ShutdownEditorWorkspace(void)
{
   cmdterm_set_editor_focus(nullptr);
   ResetEditorWorkspaceInput();
}

void PrepareEditorWorkspace()
{
   if (auto split = std::exchange(pendingSplit, std::nullopt); split && EditorWorkspaceLayout())
      vmWinCfgSetSplit(*split);
}

void DrawEditorWorkspace()
{
   if (const auto *layout = EditorWorkspaceLayout())
   {
      resizing = false;
      ImGui::BeginDisabled(DarkDlgsModalActive());
      DrawCameraPanes();
      DrawConsole(layout->console);
      ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
      ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(1, 1));
      if (BeginWorkspaceWindowImpl("##BrushControls", layout->tools, ImGuiWindowFlags_NoScrollbar))
         DrawEditorPanels();
      ImGui::End();
      ImGui::PopStyleVar(2);
      DrawTexturePalette();
      DrawMotionEditor();
      DrawWorkspaceDividers(*layout);
      ImGui::EndDisabled();
      DrawStatus(layout->status);
   }
}

void QueueEditorWorkspaceAction(std::function<void()> action)
{
   actions.push_back(std::move(action));
}

void ExecuteEditorWorkspaceActions()
{
   if (auto split = std::exchange(pendingWorkspaceSplit, std::nullopt))
      EditorResizeWorkspace(*split);
   auto pending = std::exchange(actions, {});
   for (const auto &action : pending)
      action();
}

void ResetEditorWorkspaceInput()
{
   actions.clear();
   pendingSplit.reset();
   pendingWorkspaceSplit.reset();
   workspaceDividers.clear();
   cameraDividers.clear();
   resizing = false;
   resizeCancelled = false;
   activeResize = nullptr;
   consoleFocused = false;
   consoleFocusRequested = false;
   consoleUnfocusRequested = false;
}

bool EditorWorkspaceOwnsPointer(float x, float y, int button)
{
   const auto *layout = EditorWorkspaceLayout();
   if (!layout)
      return false;
   const Point point = ToLogical(x, y);
   if (const Rect *motion = MotionEditorBounds(); motion && Contains(*motion, point))
      return true;
   if (const Rect *palette = TexturePaletteBounds(); palette && Contains(*palette, point))
      return true;
   if (Contains(layout->console, point) || Contains(layout->status, point) || Contains(layout->tools, point))
      return true;
   if (std::any_of(workspaceDividers.begin(), workspaceDividers.end(), [&](const Rect &rect) { return Contains(rect, point); }))
      return true;
   return button != SDL_BUTTON_RIGHT &&
          (resizing || std::any_of(cameraDividers.begin(), cameraDividers.end(), [&](const Rect &rect) { return Contains(rect, point); }));
}

bool EditorWorkspaceUsesCameraCursor(float x, float y)
{
   if (!EditorWorkspaceLayout() || EditorWorkspaceOwnsPointer(x, y))
      return false;

   const Point point = ToLogical(x, y);
   for (int i = 0; i < MAX_CAMERAS_REAL; ++i)
   {
      Point origin, size;
      if (VIEW_AVAIL(i) && vmGetRegionInfo(i, &origin, &size) &&
          Contains({origin, MakePoint(origin.x + size.x, origin.y + size.y)}, point))
         return true;
   }
   return false;
}

bool CancelEditorWorkspaceResize()
{
   if (!resizing || !activeResize)
      return false;
   *activeResize = resizeStart;
   resizeCancelled = true;
   return true;
}

bool EditorWorkspaceOwnsKeyboard()
{
   return consoleFocused || consoleFocusRequested || resizing ||
      (ImGui::GetCurrentContext() && ImGui::GetIO().WantTextInput);
}
