#include <material_parser.hpp>
#include <cstdio>
#include <map>
#include <stdexcept>

static void Check(bool value, const char *message)
{
   if (!value)
      throw std::runtime_error(message);
}

static DarkMaterial::Material Parse(const std::string &text,
                                    std::map<std::string, std::string> includes = {})
{
   includes["test.mtl"] = text;
   const auto reader = [&](const std::string &name, std::string &contents)
   {
      const auto found = includes.find(name);
      if (found == includes.end())
         return false;
      contents = found->second;
      return true;
   };
   const auto config = [](const std::string &name) { return name == "enabled"; };
   return DarkMaterial::Parser(reader, config).Parse("test.mtl");
}

int main()
{
   try
   {
      auto material = Parse("\xef\xbb\xbf TERRAIN_SCALE 64 128 // dimensions\r\n"
                            "tile_factor 4 -2\nforce_opaque\nforce_full_alpha 0\n"
                            "uv_clamp 1\nno_mipmap 1\nforce_local_pal\n"
                            "ani_rate 33\nani_frames 20\nani_mode PINGPONG REVERSE\n"
                            "force_ani_settings\nillum_map 0.5 \"fam\\illum\\test glow\"\n");
      const auto &s = material.settings;
      Check(s.terrainWidth == 64 && s.terrainHeight == 128, "rectangular terrain dimensions");
      Check(s.tileU == 0.25f && s.tileV == 2, "positive and negative tile factors");
      Check(s.forceOpaque == 1 && s.fullAlpha == 0 && s.uvClamp == 1, "material flags");
      Check(s.noMipmap == 1 && s.localPalette == 1, "mipmap and palette flags");
      Check(s.animationRate == 33 && s.animationFrames == 20 && s.animationMode == 4 &&
                s.forceAnimation == 1,
            "animation settings");
      Check(s.illumination == 0.5f && material.illumination == "fam\\illum\\test glow",
            "quoted illumination reference");
      Check(material.unsupported.empty(), "all parsed directives supported");

      material = Parse("terrain_scale 128\ninclude shared.inc\nterrain_scale 32\n",
                       {{"shared.inc", "terrain_scale 64\ninclude nested.inc\n"},
                        {"nested.inc", "tile_factor 2\n"}});
      Check(material.settings.terrainWidth == 32 && material.settings.tileV == 0.5f,
            "nested textual includes preserve override order");

      material = Parse(".ifdef DX6\nterrain_scale 16\n.else\n"
                       ".ifdef $enabled\ninclude active.inc\n.endif\n.endif\n"
                       ".ifndef $disabled\nforce_opaque\n.endif\n",
                       {{"active.inc", "terrain_scale 64\n"}});
      Check(material.settings.terrainWidth == 64 && material.settings.forceOpaque == 1,
            "renderer/config conditional blocks");

      material =
          Parse("include cycle.inc\nterrain_scale 64\n", {{"cycle.inc", "include test.mtl\n"}});
      Check(material.settings.terrainWidth == 64 && !material.unsupported.empty(),
            "include cycles terminate without discarding valid settings");

      material = Parse("terrain_scale 64\nterrain_scale -1\nterrain_scale 63\n"
                       "tile_factor 0\nani_rate 0\nani_rate -1\nillum_map nan bad\n"
                       "force_opaque 5\nterrain_scale 4294967296\n");
      Check(material.settings.terrainWidth == 64 && material.settings.tileU == 1 &&
                material.settings.animationRate == 0 && material.settings.forceOpaque == -1 &&
                material.illumination.empty(),
            "malformed values do not overwrite valid/default settings");

      for (const char *pass :
           {"render_pass\n{\nuv_clamp 1\ntexture { a b }\n}\n", "render_pass { uv_clamp 1 }\n"})
      {
         material = Parse(std::string(pass) + "terrain_scale 128\n");
         Check(material.settings.uvClamp == -1 && material.settings.terrainWidth == 128,
               "unsupported pass properties cannot leak into base material");
         Check(material.unsupported.size() == 1, "unsupported pass diagnostic is bounded");
      }
      material = Parse("# absent settings\n.ifdef 0\ninclude missing.inc\n.endif\n");
      Check(material.settings.tileU == 1 && material.settings.animationMode == -1 &&
                material.unsupported.empty(),
            "missing properties and inactive includes retain defaults");
      material = Parse("terrain_scale 64\nillum_map 1 \"unterminated\n"
                       "tile_factor +2 -4\nforce_opaque 1");
      Check(material.settings.terrainWidth == 64 && material.illumination.empty() &&
                material.settings.tileU == 0.5f && material.settings.tileV == 4 &&
                material.settings.forceOpaque == 1,
            "an invalid quoted line recovers at newline and accepts EOF without newline");

      material = Parse("illum_map 1 \"glow#//literal path\\name\" # comment\n"
                       "unknown_setting terrain_scale\nUNKNOWN_SETTING other\n"
                       "illum_map 2 terrain_scale\n");
      Check(material.illumination == "terrain_scale" && material.settings.illumination == 2 &&
                material.unsupported.size() == 1 &&
                material.unsupported[0] == "unknown_setting",
            "directive keywords remain ordinary arguments and unknown diagnostics are deduplicated");
      material = Parse("illum_map 1 \"glow#//literal path\\name\" // comment\n");
      Check(material.illumination == "glow#//literal path\\name",
            "comments and backslashes in quoted paths are literal");

      material = Parse("include begin.inc\nterrain_scale 128\n.else\nterrain_scale 16\n"
                       ".endif\ninclude pass.inc\nuv_clamp 1\n}\nforce_opaque 1\n",
                       {{"begin.inc", ".ifdef $enabled\n"},
                        {"pass.inc", "render_pass\n{\n"}});
      Check(material.settings.terrainWidth == 128 && material.settings.uvClamp == -1 &&
                material.settings.forceOpaque == 1 && material.unsupported.size() == 1,
            "condition and render-pass state crosses textual include boundaries");

      material = Parse("include ../escape.inc\ninclude /absolute.inc\n"
                       "include c:drive.inc\nterrain_scale 64\n");
      Check(material.settings.terrainWidth == 64 && material.unsupported.empty(),
            "include traversal and absolute paths are ignored");

      std::puts("PASS material settings, includes, conditionals, animation and malformed input");
      return 0;
   }
   catch (const std::exception &error)
   {
      std::fprintf(stderr, "%s\n", error.what());
      return 1;
   }
}
