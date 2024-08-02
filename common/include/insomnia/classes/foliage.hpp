/*  InsomniaLib
    Copyright(C) 2021-2024 Lukas Cone

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
#include "spike/type/matrix44.hpp"

struct FoliageBranchLod {
  uint32 indexOffset;
  uint16 numIndices;
  uint16 unk;
};

struct SpriteLodRange {
  uint32 indexBegin;
  uint32 indexEnd;
  uint32 unk0;
  float unk1;
};

struct SpriteRange {
  uint16 indexBegin;
  uint16 indexEnd;
  uint16 positionsOffset;
  uint16 numSprites;
};

struct Foliage : CoreClass {
  static constexpr uint32 ID = 0xc200;
  uint32 unk0;
  uint16 unk4;
  uint16 unk6;
  uint32 textureIndex;
  uint32 unk5;
  uint32 indexOffset;
  uint32 null0;
  uint32 branchVertexOffset;
  uint32 unk1;
  FoliageBranchLod branchLods[4];
  uint32 spriteVertexOffset;
  uint32 usedSpriteLods;
  SpriteLodRange spriteLodRanges[6];
  float unk2[4];
  es::PointerX86<float> spritePositions;
  uint32 usedSpriteRanges;
  SpriteRange spriteRanges[8];
  float unk3[8];
};

struct FoliageInstance : CoreClass {
  static constexpr uint32 ID = 0x9700;

  es::Matrix44 tm;
  float unk0[33];
  es::PointerX86<Foliage> foliage;
  uint32 unk1[2];
  uint32 unk[4];
};
