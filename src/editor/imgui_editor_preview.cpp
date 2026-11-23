#include <algorithm>

#include <imgui_internal.h>

#include <lg.h>
#include <2d.h>
#include <darkdlgs/modal.h>
#include <imgui_pnp.h>
#include <imgui_workspace.h>
#include <scrnman.h>

namespace
{
constexpr int kPreviewSize = 1024;
ImTextureData preview;
bool registered;

class ImagePreview final : public DarkDlgsModal
{
public:
   ImagePreview(const char *name, const grs_bitmap &bitmap)
      : DarkDlgsModal(name, ImVec2(660, 500)), m_bitmap(bitmap)
   {
   }

private:
   void DrawContents() override
   {
      const float sourceScale = std::min(1.0f, static_cast<float>(kPreviewSize) / std::max(m_bitmap.w, m_bitmap.h));
      const int width = std::max(1, static_cast<int>(m_bitmap.w * sourceScale));
      const int height = std::max(1, static_cast<int>(m_bitmap.h * sourceScale));
      if (FirstFrame())
      {
         if (!registered)
         {
            preview.Create(ImTextureFormat_RGBA32, kPreviewSize, kPreviewSize);
            ImGui::RegisterUserTexture(&preview);
            registered = true;
         }
         PaintEditorPreview(preview, 0, 0, width, height, [&] {
            gr_scale_bitmap(const_cast<grs_bitmap *>(&m_bitmap), 0, 0, width, height);
         });
      }
      const ImVec2 space = ImGui::GetContentRegionAvail();
      const float fit = std::min(space.x / width,
         (space.y - ImGui::GetFrameHeightWithSpacing()) / height);
      if (fit > 0)
         ImGui::Image(preview.GetTexRef(), ImVec2(width * fit, height * fit), ImVec2(0, 0),
                      ImVec2(static_cast<float>(width) / kPreviewSize, static_cast<float>(height) / kPreviewSize));
      if (ImGui::Button("Close") || ImGui::IsKeyPressed(ImGuiKey_Escape, false))
         Close();
   }

   const grs_bitmap &m_bitmap;
};
}

void ShowEditorImage(const char *name, const grs_bitmap *bitmap)
{
   if (bitmap && bitmap->w > 0 && bitmap->h > 0)
   {
      ImagePreview dialog(name, *bitmap);
      dialog.Run();
   }
}

void ResetEditorImageRenderer()
{
   if (registered)
   {
      preview.DestroyPixels();
      preview.SetStatus(ImTextureStatus_Destroyed);
      preview.RefCount = 0;
      registered = false;
   }
}

void ShowEditorColorPalette()
{
   class PaletteDialog final : public DarkDlgsModal
   {
   public:
      PaletteDialog() : DarkDlgsModal("Color Palette", ImVec2(410, 230)) {}

   private:
      void DrawContents() override
      {
         const ImVec2 top = ImGui::GetCursorScreenPos();
         const uchar *inverse = ScrnGetInvPalette();
         constexpr float cell = 10;
         for (int column = 0; column < 2; ++column)
            for (int color = 0; color < 256; ++color)
            {
               const uchar *rgb = grd_pal + color * 3;
               if (column && inverse)
               {
                  const long packed = rgb[0] | (rgb[1] << 8) | (rgb[2] << 16);
                  rgb = grd_pal + inverse[gr_index_lrgb(packed)] * 3;
               }
               const ImVec2 p(top.x + (column * 17 + color % 16) * cell, top.y + (color / 16) * cell);
               ImGui::GetWindowDrawList()->AddRectFilled(p, ImVec2(p.x + cell, p.y + cell),
                                                        IM_COL32(rgb[0], rgb[1], rgb[2], 255));
            }
         ImGui::Dummy(ImVec2(cell * 33, cell * 16));
         if (ImGui::Button("Close") || ImGui::IsKeyPressed(ImGuiKey_Escape, false))
            Close();
      }
   } dialog;
   dialog.Run();
}
