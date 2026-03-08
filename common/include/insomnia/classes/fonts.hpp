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

struct FontCharacter {
  uint8 kerningLeft;
  uint8 kerningRight;
  uint16 textureIndex;
  uint16 unk[3];
  uint16 ucs2Character;
  uint32 unk1;
};

struct Font {
  uint32 start;
  uint32 kerningStartLeft;
  uint32 count;
  uint32 kerningStartRight;
};

struct FontKerning {
  uint16 characters[4];
  int16 spacings[4];
};

struct FontTextureInfo {
  float width;
  float height;
  uint32 null0[2];
};

struct FontFile : CoreClass {
  static constexpr uint32 ID = 0x25400;

  es::PointerX86<Font> fonts;
  uint32 null0;
  uint32 numFonts;
  uint32 null5;
  es::PointerX86<FontCharacter> characters;
  uint32 null6;
  uint32 numCharacters;
  uint32 null7;
  es::PointerX86<FontKerning> fontKerning;
  uint32 null1;
  uint32 numKerningEntries;
  uint32 null2;
  es::PointerX86<FontTextureInfo> textureInfos;
  uint32 null3;
  uint32 numTextures;
  uint32 null4;

  std::span<Font> Fonts() { return {fonts.Get(), numFonts}; }
  std::span<FontCharacter> Characters() {
    return {characters.Get(), numCharacters};
  }
  std::span<FontKerning> Kernings() {
    return {fontKerning.Get(), numKerningEntries};
  }
  std::span<FontTextureInfo> TextureInfos() {
    return {textureInfos.Get(), numTextures};
  }

  std::span<const Font> Fonts() const { return {fonts.Get(), numFonts}; }
  std::span<const FontCharacter> Characters() const {
    return {characters.Get(), numCharacters};
  }
  std::span<const FontKerning> Kernings() const {
    return {fontKerning.Get(), numKerningEntries};
  }
  std::span<const FontTextureInfo> TextureInfos() const {
    return {textureInfos.Get(), numTextures};
  }
};
