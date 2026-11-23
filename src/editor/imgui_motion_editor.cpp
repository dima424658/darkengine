/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#include <algorithm>
#include <cstdio>

#include <imgui.h>

#include <lg.h>
#include <imgui_workspace.h>
#include <isdesced.h>
#include <isdescst.h>
#include <medflbox.h>
#include <mediface.h>
#include <medmenu.h>
#include <medplay.h>
#include <motedit_.h>
#include <motion.h>
#include <motstruc.h>
#include <prompts.h>
#include <render_backend.h>
#include <sdesc.h>
#include <status.h>
#include <vumanui.h>

namespace
{
enum class MotionPanel { None, SaveLoad, Frame, Header, HeaderInfo };
MotionPanel panel;
Rect bounds;
int views;
int frames;

const char *playopt_loop_names[] = { "PLAY ONCE","PLAY LOOP" };
//const char *playopt_which_names[] = { "MAIN ONLY", "WITH ALL MOTIONS" };
const char *playopt_blend_names[] = { "NO BLENDING", "WITH BLENDING" };
const char *playopt_freq_names[] = { "TIME BASED", "FRAME BASED" };

#define NUM_STRINGS(name) (sizeof(name)/sizeof(name[0]))

#define EnumStuff(name) kFieldFlagUnsigned, 0, NUM_STRINGS(name), NUM_STRINGS(name), name,

static sFieldDesc playOptions_fields[] =
{
   { "Trans Millisec", kFieldTypeFloat, FieldLocation(PlayOptions,trans_duration), },
   { "", kFieldTypeEnum, FieldLocation(PlayOptions,loop), EnumStuff(playopt_loop_names) },
   { "", kFieldTypeEnum, FieldLocation(PlayOptions,blend), EnumStuff(playopt_blend_names) },
   { "", kFieldTypeEnum, FieldLocation(PlayOptions,frame_based), EnumStuff(playopt_freq_names) },
   { "Timewarp", kFieldTypeFloat, FieldLocation(PlayOptions,timewarp), },
   { "Stretch", kFieldTypeFloat, FieldLocation(PlayOptions,stretch), },
};

static sStructDesc playOptions_desc = StructDescBuild(PlayOptions,kStructFlagNone,playOptions_fields);

void StopPlaying()
{
   if (PlayMotionActive())
      PlayMotionStop();
}

void ShowPlayOptions()
{
   StopPlaying();
   panel = MotionPanel::None;
   sStructEditorDesc description = {"Play Options"};
   IStructEditor *editor = CreateStructEditor(&description, &playOptions_desc, &playOptions);
   if (editor && editor->Go(kStructEdModal))
      PlayOptApply();
   SafeRelease(editor);
}

void LoadMotion()
{
   char path[PROMPT_STR_LEN];
   if (prompt_string_arg(g_MotEditMotionName, path))
      MotEditSetEditMotion(path);
}

void EditFlag()
{
   sStructEditorDesc description = {"Set Flag"};
   sFlagValue value = {FlagBoxGetCurrentFlag()};
   IStructEditor *editor = CreateStructEditor(&description, &frame_flag_desc, &value);
   if (editor && editor->Go(kStructEdModal))
      FlagBoxAddFlag(value.value);
   SafeRelease(editor);
}

void ButtonsAtTop(ImVec2 position, float width, float height)
{
   ImGui::SetCursorScreenPos(position);
   const char *labels[] = {"SAVE/LOAD", "PLAY OPTIONS", "FRAME", "HEADER"};
   for (int i = 0; i < 4; ++i)
   {
      if (i)
         ImGui::SameLine(0, 0);
      if (!ImGui::Button(labels[i], ImVec2(width / 4, height)))
         continue;
      if (i == 1)
         QueueEditorWorkspaceAction(ShowPlayOptions);
      else
      {
         const MotionPanel selected = i == 0 ? MotionPanel::SaveLoad : i == 2 ? MotionPanel::Frame : MotionPanel::Header;
         QueueEditorWorkspaceAction([selected] {
            StopPlaying();
            if (selected == MotionPanel::Header && !g_MotEditMotion)
               Status("no motion loaded");
            else
               panel = selected;
         });
      }
   }
}

void DrawPanel(ImVec2 place)
{
   ImGui::SetCursorScreenPos(place);
   ImGui::BeginGroup();
   const ImVec2 button(150, 0);
   switch (panel)
   {
   case MotionPanel::SaveLoad:
      if (ImGui::Button("Load Name", button))
         QueueEditorWorkspaceAction(LoadMotion);
      if (ImGui::Button("Save", button))
         QueueEditorWorkspaceAction(MotEditFinishMotionEdit);
      if (ImGui::Button("Quit MotEditor", button))
         QueueEditorWorkspaceAction(MotEditClose);
      break;
   case MotionPanel::Frame:
      if (ImGui::Button("Set Flag", button))
         QueueEditorWorkspaceAction(EditFlag);
      if (ImGui::Button("Mark Left", button))
         QueueEditorWorkspaceAction([] { FlagBoxSetMark(g_CurMotEditFrame, 0); });
      if (ImGui::Button("Mark Right", button))
         QueueEditorWorkspaceAction([] { FlagBoxSetMark(g_CurMotEditFrame, 1); });
      if (ImGui::Button("Clear Marks", button))
         QueueEditorWorkspaceAction([] { FlagBoxResetMarks(-1); });
      break;
   case MotionPanel::Header:
      if (ImGui::Button("Basic Info", button))
         panel = MotionPanel::HeaderInfo;
      break;
   case MotionPanel::HeaderInfo:
      if (g_MotEditMotion)
      {
         ImGui::TextUnformatted("Basic Motion Header");
         ImGui::Text("Name  %s", g_MotEditMotion->info.name);
         ImGui::Text("Num Frames  %.0f", g_MotEditMotion->info.num_frames);
         ImGui::Text("Capture Frequency  %d", static_cast<int>(g_MotEditMotion->info.freq));
      }
      break;
   case MotionPanel::None:
      break;
   }
   if (panel != MotionPanel::None && ImGui::Button("Done", button))
      panel = MotionPanel::None;
   ImGui::EndGroup();
}

void DrawTimeline(ImVec2 pos, ImVec2 size)
{
   if (size.x <= 0 || size.y <= 0)
      return;
   ImDrawList *draw = ImGui::GetWindowDrawList();
   const ImVec2 end(pos.x + size.x, pos.y + size.y);
   draw->AddRectFilled(pos, end, ImGui::GetColorU32(ImGuiCol_FrameBg));
   if (frames > 0)
   {
      const float slot = size.x / frames;
      draw->PushClipRect(pos, end, true);
      draw->AddRectFilled(ImVec2(pos.x + leftMark * slot, pos.y),
                          ImVec2(pos.x + (rightMark + 1) * slot, end.y), ImGui::GetColorU32(ImGuiCol_Header));
      if (g_MotEditMotion)
         for (int i = 0; i < g_MotEditMotion->num_flags; ++i)
         {
            const auto &flag = g_MotEditMotion->flags[i];
            const float left = pos.x + flag.frame * slot;
            draw->AddRectFilled(ImVec2(left, pos.y + 2),
               ImVec2(left + std::max(2.0f, slot - 1), end.y - 2), ImGui::GetColorU32(ImGuiCol_CheckMark));
         }
      draw->PopClipRect();
   }
}
}

BOOL CreateMotionEditorInterface(void)
{
   if (!views)
      views = vmGrabViews(vmGrabTwoWide, &bounds);
   if (!views)
      return FALSE;
   panel = MotionPanel::None;
   frames = 100;
   FlagBoxResetMarks(frames);
   return TRUE;
}

void DestroyMotionEditorInterface(void)
{
   StopPlaying();
   panel = MotionPanel::None;
   vmReleaseViews(views);
   views = 0;
}

void MainInterfaceResetBotSlider(int numFrames)
{
   frames = std::max(0, numFrames);
   g_CurMotEditFrame = g_LastMotEditFrame = 0;
}

const Rect *MotionEditorBounds(void)
{
   if (!views)
      return nullptr;
   vmGetReservedBounds(views, &bounds);
   return &bounds;
}

BOOL FrameSliderUpdate(void)
{
   if (g_LastMotEditFrame == g_CurMotEditFrame)
      return FALSE;
   char text[32];
   std::snprintf(text, sizeof(text), "%d", g_CurMotEditFrame);
   Status(text);
   g_LastMotEditFrame = g_CurMotEditFrame;
   return TRUE;
}

void DrawMotionEditor()
{
   if (!MotionEditorBounds())
      return;
   ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
   if (BeginEditorWorkspaceWindow("##MotionEditor", bounds, ImGuiWindowFlags_NoScrollbar))
   {
      const ImVec2 origin = ImGui::GetWindowPos();
      const ImVec2 size = ImGui::GetWindowSize();
      const float height = std::min(40.0f, size.y / 4);
      const float playWidth = size.x / 4;
      ButtonsAtTop(origin, size.x, height);
      if (panel != MotionPanel::None)
         DrawPanel(ImVec2(origin.x + 2, origin.y + height + 2));

      const float bottom = origin.y + size.y;
      ImGui::SetCursorScreenPos(ImVec2(origin.x, bottom - height));
      if (ImGui::Button(PlayMotionActive() ? "STOP" : "PLAY", ImVec2(playWidth, height)))
         QueueEditorWorkspaceAction([] {
            if (PlayMotionActive())
               StopPlaying();
            else if (g_MotEditMotion)
            {
               panel = MotionPanel::None;
               PlayMotionStart();
            }
         });

      const float timelineX = origin.x + playWidth;
      DrawTimeline(ImVec2(timelineX, bottom - height), ImVec2(size.x - playWidth, height / 2));
      if (frames > 1)
      {
         ImGui::SetCursorScreenPos(ImVec2(timelineX, bottom - height / 2));
         ImGui::SetNextItemWidth(size.x - playWidth);
         ImGui::SliderInt("##Frame", &g_CurMotEditFrame, 0, frames - 1);
         if (ImGui::IsItemActivated())
            QueueEditorWorkspaceAction(StopPlaying);
      }
   }
   ImGui::End();
   ImGui::PopStyleVar();
}
