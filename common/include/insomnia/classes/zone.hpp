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

struct ZoneData : CoreClass {
  static const uint32 ID = 0x7240;
  es::Matrix44 transform;
  float unk0[4];
  int32 null;
  int32 mainDataOffset;
  int16 null0;
  int16 lightMapId;
  int32 null1;
  float unk1[4];
  int32 unk[4];
};

struct RegionMesh : CoreClass {
  static constexpr uint32 ID = 0x6200;

  float unk[16];
  Vector position;
  uint16 materialIndex;
  uint16 unk6;
  uint32 unk3;
  uint32 indexOffset;
  uint32 vertexOffset;
  uint16 numIndices;
  uint16 numVerties;
  uint32 unk2[5];
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
