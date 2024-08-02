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

struct ShrubPrimitive : CoreClass {
  static constexpr uint32 ID = 0xB200;
  uint16 materialIndex;
  uint16 unk1;
  uint16 numVertices;
  uint16 numIndices;
  uint32 vertexBufferOffset;
  uint32 indexOffset;
  uint16 unk4[8];
};

struct Shrub : CoreClass {
  static constexpr uint32 ID = 0xB300;
  float unk0[20];
  es::PointerX86<ShrubPrimitive> primitives;
  uint32 numPrimitives;
  uint32 null00[2];
  uint16 unk1[2];
  float meshScale;
  float unk2[6];
};

struct ShrubInstance : CoreClass {
  static constexpr uint32 ID = 0x9500;
  es::Matrix44 tm;
  float unk0[4];
  es::PointerX86<Shrub> shrub;
  int32 unk1[11];
};
