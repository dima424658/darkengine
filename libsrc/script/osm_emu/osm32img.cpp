// Portable PE32 image reader used by the embedded legacy OSM runtime.

#include <osm_emu/osm32img.h>

#pragma pack(push, 8)
#include <algorithm>
#include <fstream>
#include <limits>
#include <sstream>
#include <string.h>
#pragma pack(pop)

namespace
{

bool FileRange(const std::vector<uint8_t> &data, size_t offset, size_t size)
{
   return offset <= data.size() && size <= data.size() - offset;
}

uint16_t Read16(const uint8_t *p)
{
   return static_cast<uint16_t>(p[0] | (static_cast<uint16_t>(p[1]) << 8));
}

uint32_t Read32(const uint8_t *p)
{
   return static_cast<uint32_t>(p[0]) |
          (static_cast<uint32_t>(p[1]) << 8) |
          (static_cast<uint32_t>(p[2]) << 16) |
          (static_cast<uint32_t>(p[3]) << 24);
}

void Write32(uint8_t *p, uint32_t value)
{
   p[0] = static_cast<uint8_t>(value);
   p[1] = static_cast<uint8_t>(value >> 8);
   p[2] = static_cast<uint8_t>(value >> 16);
   p[3] = static_cast<uint8_t>(value >> 24);
}

bool Fail(std::string *error, const char *message)
{
   if (error)
      *error = message;
   return false;
}

bool FailAt(std::string *error, const char *message, uint32_t value)
{
   if (error)
   {
      std::ostringstream text;
      text << message << " 0x" << std::hex << value;
      *error = text.str();
   }
   return false;
}

} // namespace

namespace osm32
{

cImage::cImage()
 : m_preferredBase(0),
   m_loadedBase(0),
   m_entryPointRva(0)
{
}

bool cImage::Contains(uint32_t rva, size_t size) const
{
   return rva <= m_image.size() && size <= m_image.size() - rva;
}

const char *cImage::StringAt(uint32_t rva) const
{
   if (!Contains(rva, 1))
      return 0;

   const char *text = reinterpret_cast<const char *>(&m_image[rva]);
   const size_t available = m_image.size() - rva;
   return memchr(text, 0, available) ? text : 0;
}

bool cImage::Load(const char *path, std::string *error)
{
   m_image.clear();
   m_imports.clear();
   m_exports.clear();
   m_relocations.clear();
   m_preferredBase = 0;
   m_loadedBase = 0;
   m_entryPointRva = 0;

   std::ifstream stream(path, std::ios::binary);
   if (!stream)
      return Fail(error, "cannot open OSM image");

   stream.seekg(0, std::ios::end);
   const std::streamoff fileLength = stream.tellg();
   if (fileLength < 0 || static_cast<uint64_t>(fileLength) >
                         (std::numeric_limits<size_t>::max)())
      return Fail(error, "invalid OSM file length");
   stream.seekg(0, std::ios::beg);

   std::vector<uint8_t> file(static_cast<size_t>(fileLength));
   if (!file.empty() &&
       !stream.read(reinterpret_cast<char *>(&file[0]), file.size()))
      return Fail(error, "cannot read OSM image");

   if (!FileRange(file, 0, 0x40) || Read16(&file[0]) != 0x5a4d)
      return Fail(error, "OSM is not an MZ executable");

   const uint32_t peOffset = Read32(&file[0x3c]);
   if (!FileRange(file, peOffset, 24) ||
       Read32(&file[peOffset]) != 0x00004550)
      return Fail(error, "OSM has no valid PE header");

   const uint8_t *coff = &file[peOffset + 4];
   const uint16_t machine = Read16(coff);
   const uint16_t sectionCount = Read16(coff + 2);
   const uint16_t optionalSize = Read16(coff + 16);
   if (machine != 0x014c)
      return FailAt(error, "OSM is not an x86 image; machine is", machine);
   if (!sectionCount || sectionCount > 96)
      return Fail(error, "OSM has an invalid section count");

   const size_t optionalOffset = peOffset + 24;
   if (optionalSize < 96 || !FileRange(file, optionalOffset, optionalSize))
      return Fail(error, "OSM has a truncated PE32 optional header");
   const uint8_t *optional = &file[optionalOffset];
   if (Read16(optional) != 0x010b)
      return Fail(error, "OSM is not a PE32 image");

   m_entryPointRva = Read32(optional + 16);
   m_preferredBase = Read32(optional + 28);
   m_loadedBase = m_preferredBase;
   const uint32_t imageSize = Read32(optional + 56);
   const uint32_t headerSize = Read32(optional + 60);
   if (!imageSize || imageSize > 0x40000000)
      return Fail(error, "OSM has an invalid image size");
   if (m_entryPointRva >= imageSize)
      return Fail(error, "OSM entry point lies outside the image");

   try
   {
      m_image.assign(imageSize, 0);
   }
   catch (...)
   {
      return Fail(error, "cannot allocate the OSM guest image");
   }

   const size_t headersToCopy = (std::min<size_t>)(
      (std::min<size_t>)(headerSize, file.size()), m_image.size());
   if (headersToCopy)
      memcpy(&m_image[0], &file[0], headersToCopy);

   const size_t sectionOffset = optionalOffset + optionalSize;
   if (!FileRange(file, sectionOffset, static_cast<size_t>(sectionCount) * 40))
      return Fail(error, "OSM has a truncated section table");

   for (uint16_t i = 0; i < sectionCount; ++i)
   {
      const uint8_t *section = &file[sectionOffset + i * 40];
      const uint32_t virtualAddress = Read32(section + 12);
      const uint32_t rawSize = Read32(section + 16);
      const uint32_t rawOffset = Read32(section + 20);
      if (!rawSize)
         continue;
      if (!FileRange(file, rawOffset, rawSize) ||
          virtualAddress > m_image.size() ||
          rawSize > m_image.size() - virtualAddress)
         return Fail(error, "OSM section lies outside its file or image");
      memcpy(&m_image[virtualAddress], &file[rawOffset], rawSize);
   }

   const uint32_t directoryCount = optionalSize >= 96 ? Read32(optional + 92) : 0;
   const uint32_t availableDirectories = optionalSize > 96
      ? static_cast<uint32_t>((optionalSize - 96) / 8) : 0;
   const uint32_t usableDirectories = (std::min)(directoryCount, availableDirectories);

   uint32_t exportRva = 0, exportSize = 0;
   uint32_t importRva = 0, importSize = 0;
   uint32_t relocRva = 0, relocSize = 0;
   if (usableDirectories > 0)
   {
      exportRva = Read32(optional + 96);
      exportSize = Read32(optional + 100);
   }
   if (usableDirectories > 1)
   {
      importRva = Read32(optional + 104);
      importSize = Read32(optional + 108);
   }
   if (usableDirectories > 5)
   {
      relocRva = Read32(optional + 136);
      relocSize = Read32(optional + 140);
   }

   if (exportRva && !ReadExports(exportRva, exportSize, error))
      return false;
   if (importRva && !ReadImports(importRva, importSize, error))
      return false;
   if (relocRva && !ReadRelocations(relocRva, relocSize, error))
      return false;

   if (error)
      error->clear();
   return true;
}

bool cImage::ReadExports(uint32_t rva, uint32_t size, std::string *error)
{
   if (!Contains(rva, 40) || (size && !Contains(rva, size)))
      return Fail(error, "OSM export directory lies outside the image");

   const uint8_t *directory = &m_image[rva];
   const uint32_t functionCount = Read32(directory + 20);
   const uint32_t nameCount = Read32(directory + 24);
   const uint32_t functionRvas = Read32(directory + 28);
   const uint32_t nameRvas = Read32(directory + 32);
   const uint32_t ordinals = Read32(directory + 36);
   if (functionCount > 0x100000 || nameCount > 0x100000 ||
       !Contains(functionRvas, static_cast<size_t>(functionCount) * 4) ||
       !Contains(nameRvas, static_cast<size_t>(nameCount) * 4) ||
       !Contains(ordinals, static_cast<size_t>(nameCount) * 2))
      return Fail(error, "OSM export tables are invalid");

   for (uint32_t i = 0; i < nameCount; ++i)
   {
      const uint32_t nameRva = Read32(&m_image[nameRvas + i * 4]);
      const uint16_t ordinal = Read16(&m_image[ordinals + i * 2]);
      const char *name = StringAt(nameRva);
      if (!name || ordinal >= functionCount)
         return Fail(error, "OSM contains an invalid named export");

      sExport item;
      item.name = name;
      item.rva = Read32(&m_image[functionRvas + ordinal * 4]);
      if (!Contains(item.rva, 1))
         return Fail(error, "OSM export target lies outside the image");
      m_exports.push_back(item);
   }
   return true;
}

bool cImage::ReadImports(uint32_t rva, uint32_t size, std::string *error)
{
   if (!Contains(rva, 20) || (size && !Contains(rva, size)))
      return Fail(error, "OSM import directory lies outside the image");

   const uint32_t maxDescriptors = size ? size / 20 : 0x10000;
   bool terminated = false;
   for (uint32_t descriptorIndex = 0;
        descriptorIndex < maxDescriptors; ++descriptorIndex)
   {
      const uint32_t descriptorRva = rva + descriptorIndex * 20;
      if (!Contains(descriptorRva, 20))
         return Fail(error, "OSM import descriptor lies outside the image");
      const uint8_t *descriptor = &m_image[descriptorRva];
      const uint32_t lookupRva = Read32(descriptor);
      const uint32_t moduleRva = Read32(descriptor + 12);
      const uint32_t iatRva = Read32(descriptor + 16);
      if (!lookupRva && !moduleRva && !iatRva)
      {
         terminated = true;
         break;
      }

      const char *module = StringAt(moduleRva);
      const uint32_t thunkRva = lookupRva ? lookupRva : iatRva;
      if (!module || !thunkRva || !iatRva)
         return Fail(error, "OSM contains an invalid import descriptor");

      bool thunkTerminated = false;
      for (uint32_t thunkIndex = 0; thunkIndex < 0x100000; ++thunkIndex)
      {
         const uint32_t lookupEntry = thunkRva + thunkIndex * 4;
         const uint32_t iatEntry = iatRva + thunkIndex * 4;
         if (!Contains(lookupEntry, 4) || !Contains(iatEntry, 4))
            return Fail(error, "OSM import thunk lies outside the image");
         const uint32_t value = Read32(&m_image[lookupEntry]);
         if (!value)
         {
            thunkTerminated = true;
            break;
         }

         sImport item;
         item.module = module;
         item.iatRva = iatEntry;
         item.byOrdinal = (value & 0x80000000u) != 0;
         item.ordinal = static_cast<uint16_t>(value);
         if (!item.byOrdinal)
         {
            if (!Contains(value, 3))
               return Fail(error, "OSM import name lies outside the image");
            const char *name = StringAt(value + 2);
            if (!name)
               return Fail(error, "OSM contains an unterminated import name");
            item.name = name;
         }
         m_imports.push_back(item);
      }
      if (!thunkTerminated)
         return Fail(error, "OSM import thunk table is not terminated");
   }
   if (!terminated)
      return Fail(error, "OSM import directory is not terminated");
   return true;
}

bool cImage::ReadRelocations(uint32_t rva, uint32_t size, std::string *error)
{
   if (!size || !Contains(rva, size))
      return Fail(error, "OSM relocation directory lies outside the image");

   uint32_t offset = 0;
   while (offset < size)
   {
      if (size - offset < 8)
         return Fail(error, "OSM has a truncated relocation block");
      const uint32_t pageRva = Read32(&m_image[rva + offset]);
      const uint32_t blockSize = Read32(&m_image[rva + offset + 4]);
      if (blockSize < 8 || blockSize > size - offset || (blockSize & 1))
         return Fail(error, "OSM has an invalid relocation block");

      const uint32_t entryCount = (blockSize - 8) / 2;
      for (uint32_t i = 0; i < entryCount; ++i)
      {
         const uint16_t entry = Read16(&m_image[rva + offset + 8 + i * 2]);
         const uint16_t type = entry >> 12;
         if (type == 0)
            continue;
         if (type != 3)
            return FailAt(error, "OSM uses unsupported PE32 relocation type", type);
         const uint32_t fixupRva = pageRva + (entry & 0x0fff);
         if (!Contains(fixupRva, 4))
            return Fail(error, "OSM relocation target lies outside the image");
         sRelocation relocation;
         relocation.rva = fixupRva;
         m_relocations.push_back(relocation);
      }
      offset += blockSize;
   }
   return true;
}

bool cImage::Relocate(uint32_t newBase, std::string *error)
{
   const uint32_t delta = newBase - m_loadedBase;
   if (!delta)
      return true;
   if (m_relocations.empty())
      return Fail(error, "OSM cannot be moved because it has no relocations");

   for (size_t i = 0; i < m_relocations.size(); ++i)
   {
      uint8_t *target = &m_image[m_relocations[i].rva];
      Write32(target, Read32(target) + delta);
   }
   m_loadedBase = newBase;
   if (error)
      error->clear();
   return true;
}

uint32_t cImage::FindExport(const char *name) const
{
   for (size_t i = 0; i < m_exports.size(); ++i)
      if (m_exports[i].name == name)
         return m_loadedBase + m_exports[i].rva;
   return 0;
}

} // namespace osm32
