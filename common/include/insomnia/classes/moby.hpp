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

struct Animation;

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

struct MobySegment : CoreClass {
  static constexpr uint32 ID = 0xd700;

  es::PointerX86<PrimitiveV2> primitives;
  uint32 numPrimitives;
};

struct Bone {
  enum { FLAG_DONT_INHERIT_SCALE = 1 };
  uint16 flags;
  int16 parentIndex;
  uint16 child;
  uint16 sibling;
};

struct Skeleton : CoreClass {
  static constexpr uint32 ID = 0xd300;

  uint16 numBones;
  uint16 rootBone;
  es::PointerX86<Bone> bones;
  es::PointerX86<es::Matrix44> tms0;
  es::PointerX86<es::Matrix44> tms1;
  uint16 scaleShift;
  uint16 translationShift;
  es::PointerX86<char> spuRefPoseBuffer;
  es::PointerX86<char> unkOffset;
};

struct MobyV2 : CoreClass {
  static constexpr uint32 ID = 0xd100;

  Vector4 boundingSphere;
  int16 bindPoseInverseOffset;
  uint16 flags;
  int16 runtimeEnum;
  uint16 numFrags;
  uint16 numSegments;
  uint16 numShaderSets;
  uint16 numBones;
  uint16 numRenderBoundingSpheres;
  uint32 heapHandles;
  es::PointerX86<MobySegment> segments;
  es::PointerX86<Skeleton> skeleton;
  es::PointerX86<char> renderBoundingSpheres;
  es::PointerX86<char> collPrimitiveProto; // COLL::Object
  es::PointerX86<char> collTriMeshProto;   // COLL::Object
  uint32 indexData;
  uint32 vertexData;
  int32 defaultUpdateEnum;
  float defaultDrawList;
  float defaultUpdateList;
  uint16 numClipData;
  uint16 animQueryHandle;
  Hash animsetHash;
  es::PointerX86<char> animSet;
  es::PointerX86<char> animGameplaySettings;
  es::PointerX86<char> clipData;
  es::PointerX86<char> dynamicJoints;
  es::PointerX86<char> segmentColliionPrimitiveInfo;
  es::PointerX86<char> looseAttSystemInfo;
  float meshScale;
  float texureStreamDistance;
  uint16 shadowMergeGroups;
  float shadowAABBExtend;
  es::PointerX86<char> physics;
  es::PointerX86<char> physicsInfo;
  es::PointerX86<char> physicsDat;
  es::PointerX86<char> bangleJointInfo;
  uint32 numBangles;
  es::PointerX86<uint8> bangleSegmentIds;
  uint64 defaultDraw;
  uint32 numPhaseJointGroups;
  es::PointerX86<uint16> numPhaseJointIndicesPerGroup;
  es::PointerX86<es::PointerX86<uint16>> phaseJointIndices;
  es::PointerX86<uint8> particleDefinition;
  Hash moby;
  es::PointerX86<char> selfPath;
  es::PointerX86<char> bangleGeomSimDat;
  uint64 bangleCheapChunk;
  es::PointerX86<char> navEffDat;
  es::PointerX86<char> navClueDat;
  es::PointerX86<char> morphInfo;
  uint32 looseAttDataSize;
  es::PointerX86<char> interactData;
  es::PointerX86<char> destruction;
  uint32 padding[8];
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
  uint16 numAnimations;
  uint16 numMeshes;
  uint16 mobyId;
  uint16 null00;
  uint8 anotherSet; // bool?
  uint8 null01;
  es::PointerX86<Skeleton> skeleton;
  es::PointerX86<es::PointerX86<Animation>> animations;
  es::PointerX86<MeshV1> meshes;
  es::PointerX86<char> unkData1;
  uint32 null02;
  uint32 indexBufferOffset;
  int32 vertexBufferOffset;
  float meshScale;
  uint32 unkRest[32];
};
