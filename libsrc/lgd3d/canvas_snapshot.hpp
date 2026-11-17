#ifndef DARK_CANVAS_SNAPSHOT_HPP
#define DARK_CANVAS_SNAPSHOT_HPP

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <limits>
#include <vector>

// Retain legacy pixel bytes, not expanded RGBA pixels. Two 16-bit pixels (or
// four palette indices) share an RGBA8 texel and are decoded by the GPU.
class cCanvasSnapshot
{
public:
   bool Update(const void *pixels, int width, int height, int rowBytes,
      int depth, const uint8_t *palette, bool transparent)
   {
      return UpdateRect(pixels, width, height, rowBytes, depth, palette,
         transparent, 0, 0, width, height);
   }

   bool UpdateRect(const void *pixels, int width, int height, int rowBytes,
      int depth, const uint8_t *palette, bool transparent,
      int x0, int y0, int x1, int y1)
   {
      if (!pixels || width <= 0 || height <= 0 || rowBytes <= 0 ||
          (depth != 8 && depth != 15 && depth != 16) || (depth == 8 && !palette))
         return false;
      const size_t bytes = static_cast<size_t>(width) * (depth == 8 ? 1 : 2);
      if (bytes > static_cast<size_t>(std::numeric_limits<int>::max()) - 3)
         return false;
      const size_t stride = (bytes + 3) & ~size_t(3);
      if (bytes > static_cast<size_t>(rowBytes) ||
          stride > static_cast<size_t>(std::numeric_limits<int>::max()) ||
          static_cast<size_t>(height) > std::numeric_limits<size_t>::max() / stride ||
          static_cast<size_t>(height - 1) >
             (std::numeric_limits<size_t>::max() - bytes) / static_cast<size_t>(rowBytes))
         return false;

      const bool layoutChanged = width != m_width || height != m_height || depth != m_depth;
      // Allocate before changing retained state so allocation failures leave a
      // usable snapshot. Padding is initialized for deterministic upload bytes.
      if (layoutChanged)
      {
         std::vector<uint8_t> replacement(stride * static_cast<size_t>(height), 0);
         std::vector<uint8_t> zeroRow(bytes, 0);
         std::vector<uint8_t> occupiedRows(height, 0);
         m_pixels.swap(replacement);
         m_zeroRow.swap(zeroRow);
         m_occupiedRows.swap(occupiedRows);
      }
      // A first snapshot must initialize the whole image even if the first
      // flush is small. Later flushes only commit the requested rectangle.
      x0 = layoutChanged ? 0 : std::clamp(x0, 0, width);
      y0 = layoutChanged ? 0 : std::clamp(y0, 0, height);
      x1 = layoutChanged ? width : std::clamp(x1, x0, width);
      y1 = layoutChanged ? height : std::clamp(y1, y0, height);
      const size_t left = static_cast<size_t>(x0) * (depth == 8 ? 1 : 2);
      const size_t span = static_cast<size_t>(x1 - x0) * (depth == 8 ? 1 : 2);
      bool changed = layoutChanged;
      for (int y = y0; y < y1 && span != 0; ++y)
      {
         const auto *source = static_cast<const uint8_t *>(pixels) +
            static_cast<size_t>(y) * static_cast<size_t>(rowBytes);
         auto *destination = m_pixels.data() + static_cast<size_t>(y) * stride;
         if (layoutChanged || std::memcmp(source + left, destination + left, span) != 0)
         {
            std::memcpy(destination + left, source + left, span);
            m_occupiedRows[y] = std::memcmp(destination, m_zeroRow.data(), bytes) != 0;
            changed = true;
         }
      }
      if (depth == 8 && (m_paletteGeneration == 0 ||
          std::memcmp(m_palette.data(), palette, m_palette.size()) != 0))
      {
         std::memcpy(m_palette.data(), palette, m_palette.size());
         for (size_t i = 0; i < 256; ++i)
         {
            std::memcpy(m_rgbaPalette.data() + i * 4, palette + i * 3, 3);
            m_rgbaPalette[i * 4 + 3] = 255;
         }
         ++m_paletteGeneration;
      }
      m_width = width;
      m_height = height;
      m_depth = depth;
      m_rowBytes = static_cast<int>(stride);
      m_transparent = transparent;
      if (changed) ++m_generation;
      return true;
   }

   // Only use after the owner has verified that this is the same source
   // canvas. The retained snapshot still describes the previous frame, so
   // removed HUD pixels are detected on the next update as well.
   bool ClearOverlay(void *pixels, int width, int height, int rowBytes, int depth) const
   {
      if (!pixels || width != m_width || height != m_height || depth != m_depth ||
          rowBytes < static_cast<int>(m_zeroRow.size()) || m_pixels.empty() ||
          static_cast<size_t>(height - 1) >
             (std::numeric_limits<size_t>::max() - m_zeroRow.size()) /
                static_cast<size_t>(rowBytes))
         return false;
      for (int y = 0; y < height; ++y)
         if (m_occupiedRows[y])
            std::memset(static_cast<uint8_t *>(pixels) +
               static_cast<size_t>(y) * static_cast<size_t>(rowBytes), 0, m_zeroRow.size());
      return true;
   }

   int Width() const { return m_width; }
   int Height() const { return m_height; }
   int Depth() const { return m_depth; }
   int RowBytes() const { return m_rowBytes; }
   bool Transparent() const { return m_transparent; }
   uint64_t Generation() const { return m_generation; }
   uint64_t PaletteGeneration() const { return m_paletteGeneration; }
   const std::vector<uint8_t> &Pixels() const { return m_pixels; }
   const std::array<uint8_t, 1024> &Palette() const { return m_rgbaPalette; }

private:
   int m_width{}, m_height{}, m_depth{}, m_rowBytes{};
   bool m_transparent{};
   uint64_t m_generation{}, m_paletteGeneration{};
   std::vector<uint8_t> m_pixels;
   std::vector<uint8_t> m_zeroRow;
   std::vector<uint8_t> m_occupiedRows;
   std::array<uint8_t, 768> m_palette{};
   std::array<uint8_t, 1024> m_rgbaPalette{};
};

#endif
