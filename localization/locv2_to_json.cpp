/*  InsomniaToolset LocalizationV22JSON
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

std::string_view filters[]{
    ".pkg$",
};

static AppInfo_s appInfo{
    .header = LocalizationV22JSON_DESC " v" LocalizationV22JSON_VERSION
                                       ", " LocalizationV22JSON_COPYRIGHT
                                       "Lukas Cone",
    .filters = filters,
};

AppInfo_s *AppInitModule() { return &appInfo; }

void TypeToStr(nlohmann::json &node, LocalizationType type) {
  switch (type) {
  case LocalizationType::Core:
    node = "Core";
    break;
  case LocalizationType::Dialogue:
    node = "Dialogue";
    break;
  case LocalizationType::Cinematic:
    node = "Cinematic";
    break;
  default:
    node = std::to_string(uint32(type));
    break;
  }
}

void LangToStr(nlohmann::json &node, LanguageV2 lang) {
  switch (lang) {
  case LanguageV2::us:
    node = "us";
    break;
  case LanguageV2::gb:
    node = "gb";
    break;
  case LanguageV2::fr:
    node = "fr";
    break;
  case LanguageV2::it:
    node = "it";
    break;
  case LanguageV2::de:
    node = "de";
    break;
  case LanguageV2::nl:
    node = "nl";
    break;
  case LanguageV2::es:
    node = "es";
    break;
  case LanguageV2::pt:
    node = "pt";
    break;
  case LanguageV2::dk:
    node = "dk";
    break;
  case LanguageV2::se:
    node = "se";
    break;
  case LanguageV2::fi:
    node = "fi";
    break;
  case LanguageV2::no:
    node = "no";
    break;
  case LanguageV2::jp:
    node = "jp";
    break;
  case LanguageV2::kr:
    node = "kr";
    break;
  case LanguageV2::cn:
    node = "cn";
    break;
  default:
    node = std::to_string(uint32(lang));
    break;
  }
}

struct DialogueMeta {
  int32 data[4];
  uint16 textLength;
};

template <> void FByteswapper(DialogueMeta &item, bool) {
  FByteswapper(item.data);
  FByteswapper(item.textLength);
}

void AppProcessFile(AppContext *ctx) {
  BinReaderRef_e rd(ctx->GetStream());
  IGHW main;
  main.FromStream(rd, Version::V2);
  IGHWTOCIteratorConst<LocalizationV2> localization;
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

  for (auto &t : hdr->Tags()) {
    const char *data = t.text.Get();
    auto &entry = tags[std::to_string(t.tag)];
    assert(t.unk == -1);

    if (hdr->type == LocalizationType::Cinematic) {
      entry["text"] = data + 8;
      uint32 startTime = reinterpret_cast<const uint32 &>(data[0]);
      uint32 endTime = reinterpret_cast<const uint32 &>(data[4]);
      FByteswapper(startTime);
      FByteswapper(endTime);
      entry["startTime"] = startTime;
      entry["endTime"] = endTime;
    } else if (hdr->type == LocalizationType::Dialogue) {
      entry["text"] = data;
      const char *tagLoc = data + strlen(data) + 1;
      tagLoc += GetPadding(reinterpret_cast<uintptr>(tagLoc), 4);
      DialogueMeta meta = *reinterpret_cast<const DialogueMeta *>(tagLoc);
      FByteswapper(meta);
      entry["data"] = meta.data;
    } else {
      entry = data;
    }
  }

  auto dumped =
      output.dump(4, ' ', false, nlohmann::detail::error_handler_t::replace);

  ctx->NewFile(ctx->workingFile.ChangeExtension2("json")).str << dumped;
}
