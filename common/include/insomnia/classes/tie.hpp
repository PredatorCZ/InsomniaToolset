/*  InsomniaLib
    Copyright(C) 2021-2023 Lukas Cone

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

struct TiePrimitiveV2 : CoreClass {
  static constexpr uint32 ID = 0x3300;

  uint32 indexOffset;
  uint16 vertexOffset0;
  uint16 vertexOffset1;
  uint16 numVertices;
  uint16 unk000;
  uint8 unk00;
  uint8 unk01;
  uint8 unk02;
  uint8 unk03;
  uint32 numIndices;
  uint32 unk0[3];
  uint16 unk08;
  uint16 unk1[3];
  uint16 materialIndex;
  uint16 unk06;
  uint32 unk07;
  float unk2[3];
  uint32 unk3;
};

struct TieV2 : CoreClass {
  static constexpr uint32 ID = 0x3400;

  es::PointerX86<TiePrimitiveV2> primitives;
  es::PointerX86<char> unkData1;
  uint32 unk00;
  uint32 numPrimitives;
  uint32 unk01[2];
  uint32 unk02;
  uint16 unk13;
  uint16 unk14;
  Vector meshScale;
  float unk03[14];
  es::PointerX86<char> selfPath;
  uint32 unk04[6];
};

struct TieVertexBuffer : CoreClass {
  static constexpr uint32 ID = 0x3000;
  char data;
};

struct TieIndexBuffer : CoreClass {
  static constexpr uint32 ID = 0x3200;
  uint16 data;
};

struct TiePrimitiveV1 : CoreClass {
  static constexpr uint32 ID = 0x3300;

  uint16 materialIndex;
  uint16 unk1;
  uint32 indexOffset;
  uint16 numIndices;
  uint16 numVertices;
  uint16 vertexOffset0;
  uint16 vertexOffset1;
  uint32 unk[4];
};

struct TieV1 : CoreClass {
  static constexpr uint32 ID = 0x3400;

  es::PointerX86<TiePrimitiveV1> primitives;
  es::PointerX86<char> unkData1;
  uint16 numMeshes;
  uint16 unk01;
  uint32 unk02;
  uint32 unk13;
  uint32 unk14;
  uint32 offset0;
  uint32 null00;
  Vector meshScale;
  float unk03[5];
};

struct TieInstanceV1 : CoreClass {
  static constexpr uint32 ID = 0x9300;

  es::Matrix44 tm;
  float unk0[16];
  uint32 unk1[3];
  es::PointerX86<TieV1> tie;
  uint32 unk[12];
};

struct TieV1_5 : CoreClass {
  static constexpr uint32 ID = 0x3400;

  es::PointerX86<TiePrimitiveV2> primitives;
  es::PointerX86<char> unkData1;
  uint32 unk00;
  uint32 numMeshes;
  uint32 unk01;
  uint32 vertexBufferOffset0;
  uint32 vertexBufferOffset1;
  uint16 unk13;
  uint16 unk14;
  Vector meshScale;
  float unk03[13];
  uint32 unk04[8];
};

struct TieInstanceV2 : CoreClass {
  static constexpr uint32 ID = 0x9240;

  es::Matrix44 tm;
  float unk0[4]; // probably bounding sphere
  es::PointerX86<TieV1_5> tie;
  es::PointerX86<char> unk;
  uint32 unk5[2];
  float unk2[4];
  int32 unk3[4];
};
