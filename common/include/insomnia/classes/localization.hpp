/*  InsomniaLib
    Copyright(C) 2021-2026 Lukas Cone

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

#pragma once
#include "insomnia/internal/base.hpp"
#include <span>

struct LocalizationTag {
  uint32 tag;
  es::PointerX86<char> text;
};

struct LocalizationV2Tag {
  uint32 tag;
  int32 unk;
  es::PointerX86<char> text;
};

enum class LocalizationType : uint32 {
  Core,
  Lobby,
  Dialogue,
  Pause,
  Intel,
  Frontend,
  Movie,
  Gameplay,
  Script,
  Credits,
  Config,
  // R2
  Cinematic,
};

enum class Language : uint32 {
  en,
  en_uk,
  fr,
  de,
  it,
  ko,
  nl,
  pt,
  es,
  ja,
};

enum class LanguageV2 : uint32 {
  us,
  gb,
  fr,
  it,
  de,
  nl,
  es,
  pt,
  dk,
  se,
  fi,
  no,
  jp = 0xF,
  kr,
  cn,
};

struct Localization : CoreClass {
  static constexpr uint32 ID = 0x2600;

  LocalizationType type;
  es::PointerX86<uint32> debugTags;
  es::PointerX86<LocalizationTag> tags;
  es::PointerX86<char> textBuffer;
  uint32 numDebugTags;
  uint32 numTags;
  Language language;

  std::span<LocalizationTag> Tags() { return {tags.Get(), numTags}; }
  std::span<const LocalizationTag> Tags() const {
    return {tags.Get(), numTags};
  }

  std::span<uint32> DebugTags() { return {debugTags.Get(), numDebugTags}; }
  std::span<const uint32> DebugTags() const {
    return {debugTags.Get(), numDebugTags};
  }
};

struct LocalizationV2 : CoreClass {
  static constexpr uint32 ID = 0x26000;

  LocalizationType type;
  es::PointerX86<uint32> debugTags;
  es::PointerX86<LocalizationV2Tag> tags;
  es::PointerX86<char> textBuffer;
  uint32 numDebugTags;
  uint32 numTags;
  LanguageV2 language;

  std::span<LocalizationV2Tag> Tags() { return {tags.Get(), numTags}; }
  std::span<const LocalizationV2Tag> Tags() const {
    return {tags.Get(), numTags};
  }

  std::span<uint32> DebugTags() { return {debugTags.Get(), numDebugTags}; }
  std::span<const uint32> DebugTags() const {
    return {debugTags.Get(), numDebugTags};
  }
};
