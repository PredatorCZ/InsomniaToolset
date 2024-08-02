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

struct PrimitiveV2 : CoreClass {
  static constexpr uint32 ID = 0xdd00;

  uint32 indexOffset;
  uint32 vertexOffset;
  uint16 materialIndex;
  uint16 numVertices;
  uint8 numJoints;
  uint8 vertexFormat;
  uint8 index;
  uint8 unk4;
  uint32 numIndices;
  uint32 unk5[3];
  es::PointerX86<uint16> joints;
  uint16 unk6[2];
  uint16 unk1[3];
  float unk2[3];
  uint32 unk3;
};

struct Mesh : CoreClass {
  static constexpr uint32 ID = 0xd700;

  es::PointerX86<PrimitiveV2> primitives;
  uint32 numPrimitives;
};

struct Bone {
  uint16 unk;
  int16 parentIndex;
  uint16 child;
  uint16 parentChild;
};

struct Skeleton : CoreClass {
  static constexpr uint32 ID = 0xd300;

  uint16 numBones;
  uint16 rootBone;
  es::PointerX86<Bone> bones;
  es::PointerX86<es::Matrix44> tms0;
  es::PointerX86<es::Matrix44> tms1;
  uint16 unk0;
  uint16 unk1;
  uint32 trsDataBuffer;
  uint32 off2;
};

struct MobyV2 : CoreClass {
  static constexpr uint32 ID = 0xd100;

  float unk00[4];
  uint32 unk01[2];
  uint16 numMeshes;
  uint16 unk11;
  uint16 numBones;
  uint16 unk13;
  uint32 null00;
  es::PointerX86<Mesh> meshes;
  es::PointerX86<Skeleton> skeleton;
  es::PointerX86<char> unkData0;
  es::PointerX86<char> meshTransform;
  uint32 unk011[4];
  float unk02[2];
  uint32 unk032;
  Hash animset;
  uint32 null02[4];
  es::PointerX86<char> unkData1;
  uint32 unk031;
  float meshScale;
  uint32 unk04[2];
  float unk05[2];
  es::PointerX86<char> unkData2;
  es::PointerX86<char> unkData3;
  uint32 unk06[9];
  Hash moby;
  es::PointerX86<char> selfPath;
  es::PointerX86<char> unkData4;
  uint32 unk07[2];
  es::PointerX86<char> unkData5;
  es::PointerX86<char> unkData6;
  uint32 null01[12];
};

struct PrimitiveV1 : CoreClass {
  static constexpr uint32 ID = 0xdd00;

  uint16 materialIndex;
  uint16 numVertices;
  uint16 numIndices;
  uint8 numJoints;
  uint8 vertexFormat;
  uint32 indexOffset;
  uint32 vertexBufferOffset;
  es::PointerX86<uint16> joints;
  uint32 unk[3];
};

struct MeshV1 : CoreClass {
  es::PointerX86<PrimitiveV1> primitives;
  uint32 numPrimitives;
};

struct MobyV1 : CoreClass {
  static constexpr uint32 ID = 0xd100;

  float unk00[4];
  uint16 unk01;
  uint16 unk02;
  uint16 numBones;
  uint16 unk03;
  uint16 numMeshes;
  uint16 mobyId;
  uint16 null00;
  uint8 anotherSet; // bool?
  uint8 null01;
  es::PointerX86<Skeleton> skeleton;
  es::PointerX86<char> unkData0;
  es::PointerX86<MeshV1> meshes;
  es::PointerX86<char> unkData1;
  uint32 null02;
  uint32 indexBufferOffset;
  int32 vertexBufferOffset;
  float meshScale;
  uint32 unkRest[32];
};
