/*  InsomniaToolset Localization2JSON
    Copyright(C) 2026 Lukas Cone

    This program is free software : you can redistribute it and / or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.If not, see <https://www.gnu.org/licenses/>.
*/

#include "insomnia/insomnia.hpp"
#include "nlohmann/json.hpp"
#include "project.h"
#include "spike/app_context.hpp"
#include "spike/io/binreader.hpp"
#include "spike/reflect/reflector.hpp"
#include <codecvt>

std::string_view filters[]{
    ".pkg$",
};

static struct Localization2JSON : ReflectorBase<Localization2JSON> {
  std::string fontsFolder;
} settings;

REFLECT(CLASS(Localization2JSON),
        MEMBERNAME(fontsFolder, "fonts-folder", "f",
                   ReflDesc{"Path to folder with fonts.dat file"}), );

static AppInfo_s appInfo{
    .header =
        Localization2JSON_DESC " v" Localization2JSON_VERSION
                               ", " Localization2JSON_COPYRIGHT "Lukas Cone",
    .settings = reinterpret_cast<ReflectorFriend *>(&settings),
    .filters = filters,
};

AppInfo_s *AppInitModule() { return &appInfo; }

IGHW FONTFILE;
std::span<const FontCharacter> CHARACTERS;
std::span<const Font> FONTS;

bool AppInitContext(const std::string &) {
  if (settings.fontsFolder.empty()) {
    throw std::runtime_error("fonts-folder setting is not set, provide command "
                             "argument or edit config file");
  }
  BinReader rd(settings.fontsFolder + "fonts.dat");
  FONTFILE.FromStream(rd, Version::RFOM);
  IGHWTOCIteratorConst<FontFile> fontFile;
  CatchClasses(FONTFILE, fontFile);
  CHARACTERS = fontFile.begin()->Characters();
  FONTS = fontFile.begin()->Fonts();
  return true;
}

void TypeToStr(nlohmann::json &node, LocalizationType type) {
  switch (type) {
  case LocalizationType::Core:
    node = "Core";
    break;
  case LocalizationType::Lobby:
    node = "Lobby";
    break;
  case LocalizationType::Dialogue:
    node = "Dialogue";
    break;
  case LocalizationType::Pause:
    node = "Pause";
    break;
  case LocalizationType::Intel:
    node = "Intel";
    break;
  case LocalizationType::Frontend:
    node = "Frontend";
    break;
  case LocalizationType::Movie:
    node = "Movie";
    break;
  case LocalizationType::Gameplay:
    node = "Gameplay";
    break;
  case LocalizationType::Script:
    node = "Script";
    break;
  case LocalizationType::Credits:
    node = "Credits";
    break;
  case LocalizationType::Config:
    node = "Config";
    break;
  default:
    node = std::to_string(uint32(type));
    break;
  }
}

void LangToStr(nlohmann::json &node, Language lang) {
  switch (lang) {
  case Language::en:
    node = "en";
    break;
  case Language::en_us:
    node = "en_us";
    break;
  case Language::fr:
    node = "fr";
    break;
  case Language::de:
    node = "de";
    break;
  case Language::it:
    node = "it";
    break;
  case Language::ko:
    node = "ko";
    break;
  case Language::nl:
    node = "nl";
    break;
  case Language::pt:
    node = "pt";
    break;
  case Language::es:
    node = "es";
    break;
  case Language::ja:
    node = "ja";
    break;
  default:
    node = std::to_string(uint32(lang));
    break;
  }
}

void AppProcessFile(AppContext *ctx) {
  BinReaderRef_e rd(ctx->GetStream());
  IGHW main;
  main.FromStream(rd, Version::RFOM);
  IGHWTOCIteratorConst<Localization> localization;
  CatchClasses(main, localization);
  auto hdr = localization.begin();

  nlohmann::json output;
  output["version"] = 1;
  TypeToStr(output["type"], hdr->type);
  LangToStr(output["language"], hdr->language);

  if (hdr->numDebugTags) {
    output["debug_tags"] = hdr->DebugTags();
  }

  auto &tags = output["tags"];
  std::wstring_convert<std::codecvt_utf8<char16_t>, char16_t> CVT{};

  for (auto &t : hdr->Tags()) {
    const char *data = t.text.Get();
    if (data[0] == -1) {
      tags[std::to_string(t.tag)] = "<none>";
      continue;
    }
    std::u16string indices =
        std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t>{}
            .from_bytes(data);
    // Only font 0 found, no need
    // int font = -1;

    for (auto &i : indices) {
      // for (int fid = 0; auto &f : FONTS) {
      //   if (f.start <= i && (f.count + f.start) > i) {
      //     assert(font < 0 || font == fid);
      //     font = fid;
      //   }
      //   fid++;
      // }

      i = CHARACTERS[i].ucs2Character;
    }

    auto &entry = tags[std::to_string(t.tag)];
    entry = CVT.to_bytes(indices);
    // entry["text"] = CVT.to_bytes(indices);
    // entry["fontIndex"] = font;
  }

  auto dumped =
      output.dump(4, ' ', false, nlohmann::detail::error_handler_t::replace);

  ctx->NewFile(ctx->workingFile.ChangeExtension2("json")).str << dumped;
}
