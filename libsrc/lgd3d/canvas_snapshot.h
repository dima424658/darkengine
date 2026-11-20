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
   bool Update(const void * pixels, int width, int height, int rowBytes,
               int depth, const uint8_t * palette, bool transparent);

   bool UpdateRect(const void * pixels, int width, int height, int rowBytes,
                   int depth, const uint8_t * palette, bool transparent, int x0,
                   int y0, int x1, int y1);

   // Only use after the owner has verified that this is the same source
   // canvas. The retained snapshot still describes the previous frame, so
   // removed HUD pixels are detected on the next update as well.
   bool ClearOverlay(void * pixels, int width, int height, int rowBytes,
                     int depth) const;

   int Width() const;
   int Height() const;
   int Depth() const;
   int RowBytes() const;
   bool Transparent() const;
   uint64_t Generation() const;
   uint64_t PaletteGeneration() const;
   const std::vector<uint8_t> & Pixels() const;
   const std::array<uint8_t, 1024> & Palette() const;

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
