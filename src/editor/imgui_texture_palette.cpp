/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#include <algorithm>
#include <array>

#include <imgui.h>
#include <imgui_internal.h>

#include <lg.h>
#include <2d.h>
#include <cmdbutts.h>
#include <editbr.h>
#include <imgui_pnp.h>
#include <imgui_workspace.h>
#include <render_backend.h>
#include <texmem.h>
#include <txtrpal.h>
#include <txtpnp.h>
#include <vumanui.h>

EXTERN BOOL drawTexturetoCanvas(int texture, editBrush *brush, BOOL active);

namespace
{
constexpr int kColumns = 32;
constexpr int kTile = 48;
constexpr int kRows = (TEXMEM_MAX + kColumns - 1) / kColumns;
constexpr int kExtraButtons = 4;

int viewMask;
Rect bounds;
TexturePalCall onSelect;
int selection;
unsigned generation;

ImTextureData atlas;
ImGuiContext *atlasContext;
std::array<bool, TEXMEM_MAX> painted{};
std::array<const grs_bitmap *, TEXMEM_MAX> bitmaps{};

void Select(int texture, bool doubleClick)
{
   if (texture == -TPAL_XTRA_DONE || (onSelect && onSelect(texture)) || doubleClick)
      DestroyTexturePalette();
   else if (texture >= 0)
      TexturePaletteSelect(texture);
}

void DrawTile(int index, ImVec2 size)
{
   if (!atlasContext)
   {
      atlas.Create(ImTextureFormat_RGBA32, kColumns * kTile, kRows * kTile);
      ImGui::RegisterUserTexture(&atlas);
      atlasContext = ImGui::GetCurrentContext();
   }
   const int column = index % kColumns;
   const int row = index / kColumns;
   const grs_bitmap *bitmap = texmemGetTexture(index);
   if (!painted[index] || bitmaps[index] != bitmap)
   {
      PaintEditorPreview(atlas, column * kTile, row * kTile, kTile, kTile,
         [index] { drawTexturetoCanvas(index, nullptr, FALSE); });
      painted[index] = true;
      bitmaps[index] = bitmap;
   }
   ImGui::Image(atlas.GetTexRef(), size,
                ImVec2(static_cast<float>(column) / kColumns, static_cast<float>(row) / kRows),
                ImVec2(static_cast<float>(column + 1) / kColumns, static_cast<float>(row + 1) / kRows));
}
}

void CreateTexturePalette(TexturePalCall update)
{
   if (!viewMask)
      viewMask = vmGrabViews(vmGrabTwoWide, &bounds);
   if (viewMask)
   {
      ++generation;
      onSelect = update;
      painted.fill(false);
   }
}

void DestroyTexturePalette(void)
{
   if (!viewMask)
      return;
   vmReleaseViews(viewMask);
   viewMask = 0;
   onSelect = nullptr;
   txtPnP_attach();
}

void ToggleTexturePalette(TexturePalCall update)
{
   if (viewMask)
      DestroyTexturePalette();
   else
      CreateTexturePalette(update);
}

bool TexturePaletteVisible(void) { return viewMask != 0; }

const Rect *TexturePaletteBounds(void)
{
   if (!viewMask)
      return nullptr;
   vmGetReservedBounds(viewMask, &bounds);
   return &bounds;
}

void TexturePaletteSelect(int texture) { selection = texture; }

void TexturePaletteUpdate(GFHUpdateOp op)
{
   if (op == GFH_FORCE)
      painted.fill(false);
}

void ResetTexturePaletteRenderer()
{
   if (atlasContext)
   {
      atlas.DestroyPixels();
      atlas.SetStatus(ImTextureStatus_Destroyed);
      atlas.RefCount = 0;
      atlasContext = nullptr;
   }
   painted.fill(false);
}

void DrawTexturePalette()
{
   if (!TexturePaletteBounds())
      return;
   ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
   if (BeginEditorWorkspaceWindow("##TexturePalette", bounds, ImGuiWindowFlags_NoScrollbar))
   {
      const int textures = std::clamp(texmemGetMax(), 0, TEXMEM_MAX);
      const int count = textures + kExtraButtons;
      int bitmapWidth = 32 * kExtraButtons;
      int bitmapHeight = 32 * kExtraButtons;
      for (int i = 0; i < textures; ++i)
      {
         const grs_bitmap *bitmap = texmemGetTexture(i);
         bitmapWidth += bitmap->w;
         bitmapHeight += bitmap->h;
      }
      const int columns = std::max(1, RectWidth(&bounds) * count / bitmapWidth);
      const int rows = std::max(1, RectHeight(&bounds) * count / bitmapHeight);
      Point dims = MakePoint(columns, rows);
      while (dims.x * dims.y < count)
      {
         ++dims.x;
         dims.y = std::max(1, dims.x * rows / columns);
      }

      std::array<Rect, TEXMEM_MAX + kExtraButtons> positions;
      LayoutRectangles(&bounds, positions.data(), count, dims, MakePoint(0, 0));
      const ImVec2 origin = ImGui::GetMainViewport()->Pos;
      const char *extra[] = {"Sky", "Remove\nfrom\nLevel", txtPnP_allow_attach ? "detach\nattach" : "put on\nbrush", "Done"};
      for (int i = 0; i < count; ++i)
      {
         const auto &rect = positions[i];
         int x = rect.ul.x, y = rect.ul.y, right = rect.lr.x, bottom = rect.lr.y;
         RenderBackendLogicalToClientPoint(&x, &y);
         RenderBackendLogicalToClientPoint(&right, &bottom);
         if (right <= x || bottom <= y)
            continue;
         const ImVec2 position(origin.x + x, origin.y + y);
         const ImVec2 size(right - x, bottom - y);
         ImGui::PushID(i);
         ImGui::SetCursorScreenPos(position);
         bool clicked;
         if (i < textures)
         {
            DrawTile(i, size);
            ImGui::SetCursorScreenPos(position);
            clicked = ImGui::InvisibleButton("##Texture", size);
            if (i == selection || ImGui::IsItemHovered())
               ImGui::GetWindowDrawList()->AddRect(position, ImVec2(position.x + size.x, position.y + size.y),
                  ImGui::GetColorU32(i == selection ? ImGuiCol_CheckMark : ImGuiCol_Border), 0.0f, 2.0f);
         }
         else
         {
            const char *label = extra[i - textures];
            const ImVec2 text = ImGui::CalcTextSize(label);
            const float scale = std::min({1.0f, (size.x - 4) / text.x, (size.y - 2) / text.y});
            ImGui::PushFont(nullptr, std::max(5.0f, ImGui::GetFontSize() * scale));
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(1, 0));
            clicked = ImGui::Button(label, size);
            ImGui::PopStyleVar();
            ImGui::PopFont();
         }

         const bool doubleClick = ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left);
         if (clicked || doubleClick)
         {
            const int value = i < textures ? i : -(i - textures + 1);
            QueueEditorWorkspaceAction([value, doubleClick, current = generation] {
               if (viewMask && generation == current)
                  Select(value, doubleClick);
            });
         }
         ImGui::PopID();
      }
   }
   ImGui::End();
   ImGui::PopStyleVar();
}
