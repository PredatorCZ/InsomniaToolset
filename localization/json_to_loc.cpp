/*  InsomniaToolset JSON2Localization
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
#include "spike/master_printer.hpp"
#define ES_COPYABLE_POINTER
#include "insomnia/insomnia.hpp"
#include "nlohmann/json.hpp"
#include "project.h"
#include "spike/app_context.hpp"
#include "spike/io/binreader.hpp"
#include "spike/io/binwritter_stream.hpp"
#include "spike/reflect/reflector.hpp"
#include <codecvt>
#include <map>

std::string_view filters[]{
    ".json$",
};

static struct JSON2Localization : ReflectorBase<JSON2Localization> {
  std::string fontsFolder;
} settings;

REFLECT(CLASS(JSON2Localization),
        MEMBERNAME(fontsFolder, "fonts-folder", "f",
                   ReflDesc{"Path to folder with fonts.dat file"}), );

static AppInfo_s appInfo{
    .header =
        JSON2Localization_DESC " v" JSON2Localization_VERSION
                               ", " JSON2Localization_COPYRIGHT "Lukas Cone",
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

LocalizationType StrToType(nlohmann::json &node) {
  const std::string value = node;

  switch (JenkinsHash_(value)) {
  case JenkinsHash_("Core"):
    return LocalizationType::Core;
  case JenkinsHash_("Lobby"):
    return LocalizationType::Lobby;
  case JenkinsHash_("Dialogue"):
    return LocalizationType::Dialogue;
  case JenkinsHash_("Pause"):
    return LocalizationType::Pause;
  case JenkinsHash_("Intel"):
    return LocalizationType::Intel;
  case JenkinsHash_("Frontend"):
    return LocalizationType::Frontend;
  case JenkinsHash_("Movie"):
    return LocalizationType::Movie;
  case JenkinsHash_("Gameplay"):
    return LocalizationType::Gameplay;
  case JenkinsHash_("Script"):
    return LocalizationType::Script;
  case JenkinsHash_("Credits"):
    return LocalizationType::Credits;
  case JenkinsHash_("Config"):
    return LocalizationType::Config;
  default:
    return LocalizationType(int(node));
  }
}

Language StrToLang(nlohmann::json &node) {
  const std::string value = node;

  switch (JenkinsHash_(value)) {
  case JenkinsHash_("en"):
    return Language::en;
  case JenkinsHash_("en_uk"):
  case JenkinsHash_("en_us"): // old mishap
    return Language::en_uk;
  case JenkinsHash_("fr"):
    return Language::fr;
  case JenkinsHash_("de"):
    return Language::de;
  case JenkinsHash_("it"):
    return Language::it;
  case JenkinsHash_("ko"):
    return Language::ko;
  case JenkinsHash_("nl"):
    return Language::nl;
  case JenkinsHash_("pt"):
    return Language::pt;
  case JenkinsHash_("es"):
    return Language::es;
  case JenkinsHash_("ja"):
    return Language::ja;
  default:
    return Language(int(node));
  }
}

struct IGHWWADTOC {
  uint32 id = Localization::ID;
  uint32 data = 80;
  uint32 size = 0;
  uint32 null = 0;
};

struct IGHWWADHeader {
  uint32 id = IGHWHeader::ID;
  uint16 versionMajor = 0;
  uint16 versionMinor = 2;
  uint32 numToc = 4;
  uint32 null = 0;

  IGHWWADTOC toc[4];
};

template <> void FByteswapper(IGHWWADTOC &item, bool) {
  FByteswapper(item.id);
  FByteswapper(item.data);
  FByteswapper(item.size);
}

template <> void FByteswapper(IGHWWADHeader &item, bool) {
  FByteswapper(item.id);
  FByteswapper(item.versionMajor);
  FByteswapper(item.versionMinor);
  FByteswapper(item.numToc);
  FByteswapper(item.null);
  FByteswapper(item.toc);
}

template <> void FByteswapper(Localization &item, bool) {
  FByteswapper(item.type);
  FByteswapper(item.debugTags);
  FByteswapper(item.tags);
  FByteswapper(item.textBuffer);
  FByteswapper(item.numDebugTags);
  FByteswapper(item.numTags);
  FByteswapper(item.language);
}

template <> void FByteswapper(LocalizationTag &item, bool) {
  FByteswapper(item.tag);
  FByteswapper(item.text);
}

void AppProcessFile(AppContext *ctx) {
  nlohmann::json input(nlohmann::json::parse(ctx->GetStream()));

  if (uint32 version = input["version"]; version != 1) {
    throw es::InvalidVersionError(version);
  }

  std::vector<uint32> debugTags;
  if (auto debugTagsNode = input["debug_tags"]; !debugTagsNode.is_null()) {
    debugTags = {debugTagsNode.begin(), debugTagsNode.end()};
  }
  std::map<uint32, std::pair<std::string, uint32>> tags;
  const LocalizationType locType = StrToType(input["type"]);
  for (auto &obj : input["tags"].items()) {
    auto &value = obj.value();
    if (locType == LocalizationType::Dialogue &&
        value.type() != nlohmann::json::value_t::string) {
      tags[std::stoul(obj.key())] =
          std::make_pair<std::string, uint32>(value["text"], value["soundTag"]);
    } else {
      tags[std::stoul(obj.key())] =
          std::make_pair<std::string, uint32>(value, 0);
    }
  }

  std::wstring_convert<std::codecvt_utf8<char16_t>, char16_t> CVT{};
  BinWritterRef_e wr(
      ctx->NewFile(ctx->workingFile.ChangeExtension2("pkg")).str);
  wr.SwapEndian(true);

  IGHWWADHeader HDR{};
  Localization loc{
      .type = locType,
      .debugTags = {},
      .tags = {},
      .textBuffer = {},
      .numDebugTags = uint32(debugTags.size()),
      .numTags = uint32(tags.size()),
      .language = StrToLang(input["language"]),
  };

  es::Dispose(input);

  std::vector<uint32> offsets;

  wr.Write(HDR);
  offsets.emplace_back(wr.Tell() + offsetof(Localization, debugTags));
  offsets.emplace_back(wr.Tell() + offsetof(Localization, tags));
  offsets.emplace_back(wr.Tell() + offsetof(Localization, textBuffer));
  wr.Write(loc);
  wr.ApplyPadding();
  loc.debugTags.Reset(wr.Tell());
  HDR.toc[1].data = wr.Tell();
  if (debugTags.empty()) {
    wr.Write(0);
    HDR.toc[1].size = 4;
  }
  wr.WriteContainer(debugTags);
  wr.ApplyPadding();
  HDR.toc[1].size += 4 * debugTags.size();
  loc.tags.Reset(wr.Tell());
  HDR.toc[2].data = wr.Tell();
  HDR.toc[2].size = 8 * tags.size();
  loc.textBuffer.Reset(wr.Tell() + 8 * tags.size());
  wr.Push();
  wr.Seek(sizeof(HDR));
  wr.Write(loc);
  wr.Pop();
  for (uint32 i = 0; i < tags.size(); i++) {
    offsets.emplace_back(wr.Tell() + i * 8 + 4);
  }
  wr.Skip(8 * tags.size());
  wr.ApplyPadding();
  HDR.toc[3].data = wr.Tell();
  std::vector<LocalizationTag> entries;

  for (auto [tag, text] : tags) {
    LocalizationTag entry;
    entry.tag = tag;
    entry.text.Reset(wr.Tell());
    entries.push_back(entry);

    if (text.first == "<none>") {
      wr.Write(uint32(-1));
      wr.Write(char(0));
      continue;
    }

    std::u16string indices =
        std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t>{}
            .from_bytes(text.first.c_str());
    for (auto &i : indices) {
      bool found = false;
      for (char16_t idx = 0; auto c : CHARACTERS) {
        if (c.ucs2Character == i) {
          i = idx;
          found = true;
          break;
        }

        idx++;
      }

      if (!found) {
        PrintWarning("Unregistered character: ", CVT.to_bytes(i),
                     " for tag: ", tag);
        i = '?';
      }
    }

    auto data = CVT.to_bytes(indices.c_str());
    wr.WriteT(data);

    if (loc.type == LocalizationType::Dialogue) {
      wr.ApplyPadding(4);
      wr.Write(text.second);
    }
  }

  HDR.toc[0].size = sizeof(Localization);
  HDR.toc[1].id = -1U;
  HDR.toc[2].id = -1U;
  HDR.toc[3].id = -1U;
  HDR.toc[3].size = wr.Tell() - HDR.toc[3].data;
  wr.ApplyPadding();
  wr.WriteContainerWCount(offsets);
  wr.Write(0);
  wr.Write(0);
  wr.Pop();
  wr.WriteContainer(entries);
  wr.Seek(0);
  wr.Write(HDR);
}
