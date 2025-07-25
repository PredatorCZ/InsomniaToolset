#pragma once
#include "spike/type/float.hpp"
#include "spike/type/vectors.hpp"
#include "spike/util/endian.hpp"

struct Vertex0 {
  int16 position[3];
  int16 boneIndex;
  float16 uv[2];
  uint32 normal;
  uint32 tangent;
};

struct Vertex1 {
  int16 position[3];
  int16 unk;
  uint8 bones[4];
  uint8 weights[4];
  float16 uv[2];
  uint32 normal;
  uint32 tangent;
};

struct RegionVertex {
  int16 position[4];
  float16 uv0[2];
  float16 uv1[2];
  uint16 normal[2];
  uint16 tangent[2];
  uint16 unk;
};

struct RegionVertexV2 {
  int16 position[4];
  float16 uv0[2];
  float16 uv1[2];
  uint32 normal;
  uint32 tangent;
};

struct BranchVertex {
  float16 position[4];
  float16 uv[2];
  uint8 tangent[4]; // ??
  uint8 normal[3];
};

struct SpriteVertex {
  float16 spriteSize[2];
  USVector2 uv;
  uint32 unk; // prolly normal
};

struct ShrubVertex {
  int16 position[4];
  UCVector4 color;
  float16 uv[2];
};

struct ShrubV2Vertex {
  int16 position[4];
  float16 uv[2];
  uint32 unk;
};

struct FoliageV2Vertex {
  t_Vector<float16> position;
  int16 unk[4];
};

struct SpriteV2Vertex {
  t_Vector2<float16> size;
  t_Vector2<float16> uv;
};

inline void FByteswapper(SpriteV2Vertex &item) {
  FByteswapper(item.size);
  FByteswapper(item.uv);
}

inline void FByteswapper(FoliageV2Vertex &item) {
  FByteswapper(item.position);
  FByteswapper(item.unk);
}

inline void FByteswapper(ShrubV2Vertex &item) {
  FByteswapper(item.position);
  FByteswapper(item.uv);
}

inline void FByteswapper(ShrubVertex &item) {
  FByteswapper(item.position);
  FByteswapper(item.uv);
}

inline void FByteswapper(SpriteVertex &item) {
  FByteswapper(item.spriteSize);
  FByteswapper(item.unk);
  FByteswapper(item.uv);
}

inline void FByteswapper(RegionVertex &item) {
  FByteswapper(item.position);
  FByteswapper(item.uv0);
  FByteswapper(item.uv1);
  FByteswapper(reinterpret_cast<uint32 &>(item.normal[0]));
  FByteswapper(reinterpret_cast<uint32 &>(item.tangent[0]));
  FByteswapper(item.unk);
}

inline void FByteswapper(RegionVertexV2 &item) {
  FByteswapper(item.position);
  FByteswapper(item.uv0);
  FByteswapper(item.uv1);
  FByteswapper(item.normal);
  FByteswapper(item.tangent);
}

inline void FByteswapper(Vertex0 &item) {
  FByteswapper(item.position);
  FByteswapper(item.boneIndex);
  FByteswapper(item.uv);
  FByteswapper(item.normal);
  FByteswapper(item.tangent);
}

inline void FByteswapper(Vertex1 &item) {
  FByteswapper(item.position);
  FByteswapper(item.unk);
  FByteswapper(item.uv);
  FByteswapper(item.normal);
  FByteswapper(item.tangent);
}

inline void FByteswapper(BranchVertex &item) {
  FByteswapper(item.position);
  FByteswapper(item.uv);
}
