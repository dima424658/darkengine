// Portable PE32 image reader used by the embedded legacy OSM runtime.
// This code only maps the guest image; it never asks the host OS to load it.

#ifndef __OSM32IMG_H
#define __OSM32IMG_H

#pragma pack(push, 8)
#include <stdint.h>
#include <string>
#include <vector>
#pragma pack(pop)

namespace osm32
{

struct sImport
{
   std::string module;
   std::string name;
   uint32_t iatRva;
   uint16_t ordinal;
   bool byOrdinal;
};

class cImage
{
public:
   cImage();

   bool Load(const char *path, std::string *error);
   bool Relocate(uint32_t newBase, std::string *error);

   uint32_t PreferredBase() const { return m_preferredBase; }
   uint32_t LoadedBase() const { return m_loadedBase; }
   uint32_t EntryPoint() const { return m_loadedBase + m_entryPointRva; }
   uint32_t ImageSize() const { return static_cast<uint32_t>(m_image.size()); }

   const uint8_t *Data() const { return m_image.empty() ? 0 : &m_image[0]; }
   uint8_t *Data() { return m_image.empty() ? 0 : &m_image[0]; }

   uint32_t FindExport(const char *name) const;
   const std::vector<sImport> &Imports() const { return m_imports; }

private:
   bool ReadImports(uint32_t rva, uint32_t size, std::string *error);
   bool ReadExports(uint32_t rva, uint32_t size, std::string *error);
   bool ReadRelocations(uint32_t rva, uint32_t size, std::string *error);
   bool Contains(uint32_t rva, size_t size) const;
   const char *StringAt(uint32_t rva) const;

   struct sExport
   {
      std::string name;
      uint32_t rva;
   };

   struct sRelocation
   {
      uint32_t rva;
   };

   std::vector<uint8_t> m_image;
   std::vector<sImport> m_imports;
   std::vector<sExport> m_exports;
   std::vector<sRelocation> m_relocations;
   uint32_t m_preferredBase;
   uint32_t m_loadedBase;
   uint32_t m_entryPointRva;
};

} // namespace osm32

#endif // __OSM32IMG_H
