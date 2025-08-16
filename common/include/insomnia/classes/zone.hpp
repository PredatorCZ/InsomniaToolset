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
#include "spike/type/matrix44.hpp"
#include "shader.hpp"

struct ZoneLightmap : Texture {
  static constexpr uint32 ID = 0x5400;
};

struct ZoneShadowMap : Texture {
  static constexpr uint32 ID = 0x5410;
};

struct ZoneDataLookup : CoreClass {
  static constexpr uint32 ID = 0x72c0;
  Hash hash;
  es::PointerX86<char> name;
  uint32 nullPad;
};

struct ZoneData2Lookup : ZoneDataLookup {
  static constexpr uint32 ID = 0x74c0;
};

struct ZoneMap : Texture {
  static constexpr uint32 ID = 0x5700;
};

struct RegionMesh : CoreClass {
  static constexpr uint32 ID = 0x6200;

  OOBB bounds;
  Vector position;
  uint16 materialIndex0;
  uint16 materialIndex1;
  uint32 unk3;
  uint32 indexOffset;
  uint32 vertexOffset;
  uint16 numIndices;
  uint16 numVerties;
  uint16 unk0;
  uint16 lightMapIndex;
  uint8 unk02;
  uint8 unk00;
  uint8 unk03;
  uint8 unk01;
  uint32 unk2[3];
  float meshScale;
  uint32 unk4[2];
};

struct RegionMeshV2 : CoreClass {
  static constexpr uint32 ID = 0x6200;

  float unk[16];
  uint32 indexOffset;
  uint32 vertexOffset;
  uint16 numIndices;
  uint16 numVerties;
  uint16 unk7[2];
  uint16 materialIndex;
  uint16 unk6;
  uint32 unk2[3];
  Vector position;
  uint32 unk4;
  float unk5[4];
};

struct RegionVertexBuffer : CoreClass {
  static constexpr uint32 ID = 0x6000;
  char data;
};

struct RegionIndexBuffer : CoreClass {
  static constexpr uint32 ID = 0x6100;
  uint16 data;
};

struct ZoneTieLookup : CoreClass {
  static constexpr uint32 ID = 0x7200;
  Hash hash;
};

struct ZoneCubemapLookup : CoreClass {
  static constexpr uint32 ID = 0x72c1;
  Hash hash;
};

struct ZoneShaderLookup : CoreClass {
  static constexpr uint32 ID = 0x71a0;
  Hash hash;
};

struct ZoneFoliageLookup : CoreClass {
  static constexpr uint32 ID = 0x7400;
  Hash hash;
};

struct ZoneShrubLookup : CoreClass {
  static constexpr uint32 ID = 0x7500;
  Hash hash;
};

struct UnkInstanceV2 : CoreClass {
  static constexpr uint32 ID = 0x8c00;
  float unk0[8];
  Vector4A16 bounds[2];
  es::Matrix44 tm;
  float unk[32];
};
