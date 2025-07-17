/*  InsomniaLib
    Copyright(C) 2021-2025 Lukas Cone

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

struct Detail : CoreClass {
  static constexpr uint32 ID = 0xB200;
  uint16 materialIndex;
  uint16 unk1;
  uint16 numVertices;
  uint16 numIndices;
  uint32 vertexBufferOffset;
  uint32 indexOffset;
  Vector meshScale;
  float null0;
};

struct DetailCluster : CoreClass {
  static constexpr uint32 ID = 0xB300;
  float unk0[20];
  es::PointerX86<Detail> primitives;
  uint32 numPrimitives;
  uint32 null00[2];
  uint16 unk1[2];
  float unk2[7];
};

struct DetailInstance : CoreClass {
  static constexpr uint32 ID = 0x9500;
  es::Matrix44 tm;
  Vector unk0;
  float unk2;
  es::PointerX86<DetailCluster> cluster;
  int32 unk1[11];
};
