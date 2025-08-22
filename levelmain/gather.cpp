/*  Gather geometry data
    Copyright(C) 2025 Lukas Cone

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

#include "glm/gtx/dual_quaternion.hpp"
#include "insomnia/gltf.hpp"
#include "insomnia/internal/vertex.hpp"
#include "mikktspace.h"
#include "nlohmann/json.hpp"
#include "project.h"
#include "spike/app_context.hpp"
#include "spike/except.hpp"
#include "spike/gltf.hpp"
#include "spike/io/binreader_stream.hpp"
#include "spike/io/binwritter.hpp"
#include "spike/master_printer.hpp"
#include "spike/reflect/reflector.hpp"
#include "spike/type/float.hpp"
#include "spike/uni/format.hpp"
#include "spike/uni/rts.hpp"
#include <GLES3/gl32.h>
#include <concepts>
#include <fstream>
#include <set>
#include <thread>

std::string_view filters[]{
    "^ps3levelmain.dat$",
};

static AppInfo_s appInfo{
    .header = GatherData_DESC " v" GatherData_VERSION ", " GatherData_COPYRIGHT
                              "Lukas Cone",
    .filters = filters,
};

AppInfo_s *AppInitModule() { return &appInfo; }

using TexCoord = t_Vector2<float16>;

// mobys
struct StaticVertex {
  SVector pos;
  int16 qtangW;
  uint32 qtang;
  TexCoord uv;
};

// ties
struct StaticVertexLM : StaticVertex {
  UCVector2 bghtAlpha;
  UCVector2 lmData;
};

// zones
struct StaticVertexUV2 : StaticVertexLM {
  TexCoord uv1;
};

enum class LoadType : uint8 {
  Void,
  StartRange,
  EndRange,
  BoolTrue,
  Int8,
  Int16,
  Int24,
  Int32,
  Int40,
  Int48,
  Int56,
  Int64,
  UInt8,
  UInt16,
  UInt24,
  UInt32,
  UInt40,
  UInt48,
  UInt56,
  UInt64,
  Float,
  Double,
  Vector2,
  Vector3,
  Vector4,
  Quat,
  DualQuat,
  String8,
  String16,
  GlEnum,
  StringIndex8,
  StringIndex16,
  StringIndex24,

  ResourceRange = 0x40,
  InlineBuffer,
  VertexArrayObject,
  DrawItems,
  CheckResource,
};

struct SaveState : BinWritterRef {
  std::map<std::string, uint32> &strings;
};

void WriteBool(BinWritterRef wr, bool value) {
  if (!value) {
    wr.Write(LoadType::Void);
    return;
  }

  wr.Write(LoadType::BoolTrue);
}

template <std::integral T> void WriteValue(BinWritterRef wr, T value) {
  if (value == 0) {
    wr.Write(LoadType::Void);
    return;
  }

  uint64 val = value;
  using etype = std::underlying_type_t<LoadType>;
  etype typeBase = etype(LoadType::UInt8);

  if (value < 0) {
    val = ~int(value);
    typeBase = etype(LoadType::Int8);
  }

  if (val < 0x100) {
    wr.Write(typeBase);
    wr.WriteBuffer(reinterpret_cast<const char *>(&val), 1);
    return;
  }

  if (val < 0x10000) {
    wr.Write<etype>(typeBase + 1);
    wr.WriteBuffer(reinterpret_cast<const char *>(&val), 2);
    return;
  }

  if (val < 0x1000000) {
    wr.Write<etype>(typeBase + 2);
    wr.WriteBuffer(reinterpret_cast<const char *>(&val), 3);
    return;
  }

  if (val < 0x1'00000000) {
    wr.Write<etype>(typeBase + 3);
    wr.WriteBuffer(reinterpret_cast<const char *>(&val), 4);
    return;
  }

  if (val < 0x100'00000000) {
    wr.Write<etype>(typeBase + 4);
    wr.WriteBuffer(reinterpret_cast<const char *>(&val), 5);
    return;
  }

  if (val < 0x10000'00000000) {
    wr.Write<etype>(typeBase + 5);
    wr.WriteBuffer(reinterpret_cast<const char *>(&val), 6);
    return;
  }

  if (val < 0x1000000'00000000) {
    wr.Write<etype>(typeBase + 6);
    wr.WriteBuffer(reinterpret_cast<const char *>(&val), 7);
    return;
  }

  wr.Write<etype>(typeBase + 7);
  wr.Write(val);
}

void WriteValue(BinWritterRef wr, float value) {
  if (value == 0) {
    wr.Write(LoadType::Void);
    return;
  }

  wr.Write(LoadType::Float);
  wr.Write(value);
}

void WriteValue(BinWritterRef wr, double value) {
  if (value == 0) {
    wr.Write(LoadType::Void);
    return;
  }

  wr.Write(LoadType::Double);
  wr.Write(value);
}

void WriteValue(BinWritterRef wr, const Vector2 &value) {
  if (value == Vector2{}) {
    wr.Write(LoadType::Void);
    return;
  }

  wr.Write(LoadType::Vector2);
  wr.Write(value);
}

void WriteValue(BinWritterRef wr, const Vector &value) {
  if (value == Vector{}) {
    wr.Write(LoadType::Void);
    return;
  }

  wr.Write(LoadType::Vector3);
  wr.Write(value);
}

void WriteValue(BinWritterRef wr, const Vector4 &value) {
  if (value == Vector4{}) {
    wr.Write(LoadType::Void);
    return;
  }

  wr.Write(LoadType::Vector4);
  wr.Write(value);
}

void WriteValue(BinWritterRef wr, const glm::quat &value) {
  if (value == glm::quat{}) {
    wr.Write(LoadType::Void);
    return;
  }

  wr.Write(LoadType::Quat);
  wr.Write(value);
}

void WriteValue(BinWritterRef wr, const glm::dualquat &value) {
  if (value == glm::dualquat{}) {
    wr.Write(LoadType::Void);
    return;
  }

  wr.Write(LoadType::DualQuat);
  wr.Write(value);
}

void WriteValue(SaveState wr, const std::string &value) {
  if (value.empty()) {
    wr.Write(LoadType::Void);
    return;
  }

  auto found = wr.strings.find(value);

  if (found != wr.strings.end()) {
    uint32 id = found->second;

    if (id < 0x100) {
      wr.Write(LoadType::StringIndex8);
      wr.Write<uint8>(id);
      return;
    }

    if (id < 0x10000) {
      wr.Write(LoadType::StringIndex16);
      wr.Write<uint16>(id);
      return;
    }

    wr.Write(LoadType::StringIndex24);
    wr.WriteBuffer(reinterpret_cast<const char *>(id), 3);
    return;
  }

  wr.strings.emplace(value, wr.strings.size());

  if (value.size() < 0x100) {
    wr.Write(LoadType::String8);
    wr.WriteContainerWCount<uint8>(value);
    return;
  }

  wr.Write(LoadType::String16);
  wr.WriteContainerWCount<uint16>(value);
}

void WriteGlEnum(BinWritterRef wr, GLenum value) {
  wr.Write(LoadType::GlEnum);
  wr.Write<uint16>(value);
}

struct ResourceRange {
  uint64 dataOffset;
  uint32 dataSize;
  uint8 resourceId = 0;

  void Write(BinWritterRef wr) const {
    wr.Write(LoadType::ResourceRange);
    WriteValue(wr, dataOffset);
    WriteValue(wr, dataSize);
    WriteValue(wr, resourceId);
  }
};

enum class GfxBufferUsage {
  Index = 0b00001,
  Vertex = 0b00010,
  Uniform = 0b00100,
  Storage = 0b01000,
  CopySrc = 0b10000,
};

enum class GfxBufferFrequencyHint {
  Static = 0x01,
  Dynamic = 0x02,
};

enum class FormatTypeFlags {
  U8 = 0x01,
  U16,
  U32,
  S8,
  S16,
  S32,
  F16,
  F32,

  // Compressed texture formats.
  BC1 = 0x41,
  BC2,
  BC3,
  BC4_UNORM,
  BC4_SNORM,
  BC5_UNORM,
  BC5_SNORM,
  BC6H_UNORM,
  BC6H_SNORM,
  BC7,

  // Special-case packed texture formats.
  U16_PACKED_5551 = 0x61,
  U16_PACKED_565,
  U32_PACKED_1010102,
  S32_PACKED_1010102,

  // Depth/stencil texture formats.
  D24 = 0x81,
  D32F,
  D24S8,
  D32FS8,
};

enum class FormatCompFlags {
  R = 0x01,
  RG = 0x02,
  RGB = 0x03,
  RGBA = 0x04,
};

enum class FormatFlags {
  None = 0b00000000,
  Normalized = 0b00000001,
  sRGB = 0b00000010,
  Depth = 0b00000100,
  Stencil = 0b00001000,
  RenderTarget = 0b00010000,
};

consteval uint32 makeFormat(FormatTypeFlags type, FormatCompFlags comp,
                            FormatFlags flags) {
  return (uint32(type) << 16) | (uint32(comp) << 8) | uint32(flags);
}

consteval FormatFlags operator|(FormatFlags f0, FormatFlags f1) {
  return FormatFlags(uint32(f0) | uint32(f1));
}
// clang-format off
enum class GfxFormat {
    F16_R           = makeFormat(FormatTypeFlags::F16,       FormatCompFlags::R,                FormatFlags::None),
    F16_RG          = makeFormat(FormatTypeFlags::F16,       FormatCompFlags::RG,               FormatFlags::None),
    F16_RGB         = makeFormat(FormatTypeFlags::F16,       FormatCompFlags::RGB,              FormatFlags::None),
    F16_RGBA        = makeFormat(FormatTypeFlags::F16,       FormatCompFlags::RGBA,             FormatFlags::None),
    F32_R           = makeFormat(FormatTypeFlags::F32,       FormatCompFlags::R,                FormatFlags::None),
    F32_RG          = makeFormat(FormatTypeFlags::F32,       FormatCompFlags::RG,               FormatFlags::None),
    F32_RGB         = makeFormat(FormatTypeFlags::F32,       FormatCompFlags::RGB,              FormatFlags::None),
    F32_RGBA        = makeFormat(FormatTypeFlags::F32,       FormatCompFlags::RGBA,             FormatFlags::None),
    U8_R            = makeFormat(FormatTypeFlags::U8,        FormatCompFlags::R,                FormatFlags::None),
    U8_R_NORM       = makeFormat(FormatTypeFlags::U8,        FormatCompFlags::R,                FormatFlags::Normalized),
    U8_RG           = makeFormat(FormatTypeFlags::U8,        FormatCompFlags::RG,               FormatFlags::None),
    U8_RG_NORM      = makeFormat(FormatTypeFlags::U8,        FormatCompFlags::RG,               FormatFlags::Normalized),
    U8_RGB          = makeFormat(FormatTypeFlags::U8,        FormatCompFlags::RGB,              FormatFlags::None),
    U8_RGB_NORM     = makeFormat(FormatTypeFlags::U8,        FormatCompFlags::RGB,              FormatFlags::Normalized),
    U8_RGB_SRGB     = makeFormat(FormatTypeFlags::U8,        FormatCompFlags::RGB,              FormatFlags::sRGB | FormatFlags::Normalized),
    U8_RGBA         = makeFormat(FormatTypeFlags::U8,        FormatCompFlags::RGBA,             FormatFlags::None),
    U8_RGBA_NORM    = makeFormat(FormatTypeFlags::U8,        FormatCompFlags::RGBA,             FormatFlags::Normalized),
    U8_RGBA_SRGB    = makeFormat(FormatTypeFlags::U8,        FormatCompFlags::RGBA,             FormatFlags::sRGB | FormatFlags::Normalized),
    U16_R           = makeFormat(FormatTypeFlags::U16,       FormatCompFlags::R,                FormatFlags::None),
    U16_R_NORM      = makeFormat(FormatTypeFlags::U16,       FormatCompFlags::R,                FormatFlags::Normalized),
    U16_RG_NORM     = makeFormat(FormatTypeFlags::U16,       FormatCompFlags::RG,               FormatFlags::Normalized),
    U16_RGBA_NORM   = makeFormat(FormatTypeFlags::U16,       FormatCompFlags::RGBA,             FormatFlags::Normalized),
    U16_RGB         = makeFormat(FormatTypeFlags::U16,       FormatCompFlags::RGB,              FormatFlags::None),
    U32_R           = makeFormat(FormatTypeFlags::U32,       FormatCompFlags::R,                FormatFlags::None),
    U32_RG          = makeFormat(FormatTypeFlags::U32,       FormatCompFlags::RG,               FormatFlags::None),
    S8_R            = makeFormat(FormatTypeFlags::S8,        FormatCompFlags::R,                FormatFlags::None),
    S8_R_NORM       = makeFormat(FormatTypeFlags::S8,        FormatCompFlags::R,                FormatFlags::Normalized),
    S8_RG_NORM      = makeFormat(FormatTypeFlags::S8,        FormatCompFlags::RG,               FormatFlags::Normalized),
    S8_RGB_NORM     = makeFormat(FormatTypeFlags::S8,        FormatCompFlags::RGB,              FormatFlags::Normalized),
    S8_RGBA_NORM    = makeFormat(FormatTypeFlags::S8,        FormatCompFlags::RGBA,             FormatFlags::Normalized),
    S16_R           = makeFormat(FormatTypeFlags::S16,       FormatCompFlags::R,                FormatFlags::None),
    S16_RG          = makeFormat(FormatTypeFlags::S16,       FormatCompFlags::RG,               FormatFlags::None),
    S16_R_NORM      = makeFormat(FormatTypeFlags::S16,       FormatCompFlags::R,                FormatFlags::Normalized),
    S16_RG_NORM     = makeFormat(FormatTypeFlags::S16,       FormatCompFlags::RG,               FormatFlags::Normalized),
    S16_RGB_NORM    = makeFormat(FormatTypeFlags::S16,       FormatCompFlags::RGB,              FormatFlags::Normalized),
    S16_RGBA        = makeFormat(FormatTypeFlags::S16,       FormatCompFlags::RGBA,             FormatFlags::None),
    S16_RGBA_NORM   = makeFormat(FormatTypeFlags::S16,       FormatCompFlags::RGBA,             FormatFlags::Normalized),
    S32_R           = makeFormat(FormatTypeFlags::S32,       FormatCompFlags::R,                FormatFlags::None),

    // Packed texture formats.
    U16_RGBA_5551   = makeFormat(FormatTypeFlags::U16_PACKED_5551, FormatCompFlags::RGBA, FormatFlags::Normalized),
    U16_RGB_565     = makeFormat(FormatTypeFlags::U16_PACKED_565,  FormatCompFlags::RGB,  FormatFlags::Normalized),
    U32_RGBA_1010102_UNORM = makeFormat(FormatTypeFlags::U32_PACKED_1010102, FormatCompFlags::RGBA, FormatFlags::Normalized),
    S32_RGBA_1010102_NORM = makeFormat(FormatTypeFlags::S32_PACKED_1010102, FormatCompFlags::RGBA, FormatFlags::Normalized),

    // Compressed
    BC1             = makeFormat(FormatTypeFlags::BC1,        FormatCompFlags::RGBA, FormatFlags::Normalized),
    BC1_SRGB        = makeFormat(FormatTypeFlags::BC1,        FormatCompFlags::RGBA, FormatFlags::Normalized | FormatFlags::sRGB),
    BC2             = makeFormat(FormatTypeFlags::BC2,        FormatCompFlags::RGBA, FormatFlags::Normalized),
    BC2_SRGB        = makeFormat(FormatTypeFlags::BC2,        FormatCompFlags::RGBA, FormatFlags::Normalized | FormatFlags::sRGB),
    BC3             = makeFormat(FormatTypeFlags::BC3,        FormatCompFlags::RGBA, FormatFlags::Normalized),
    BC3_SRGB        = makeFormat(FormatTypeFlags::BC3,        FormatCompFlags::RGBA, FormatFlags::Normalized | FormatFlags::sRGB),
    BC4_UNORM       = makeFormat(FormatTypeFlags::BC4_UNORM,  FormatCompFlags::R,    FormatFlags::Normalized),
    BC4_SNORM       = makeFormat(FormatTypeFlags::BC4_SNORM,  FormatCompFlags::R,    FormatFlags::Normalized),
    BC5_UNORM       = makeFormat(FormatTypeFlags::BC5_UNORM,  FormatCompFlags::RG,   FormatFlags::Normalized),
    BC5_SNORM       = makeFormat(FormatTypeFlags::BC5_SNORM,  FormatCompFlags::RG,   FormatFlags::Normalized),
    BC6H_UNORM      = makeFormat(FormatTypeFlags::BC6H_UNORM, FormatCompFlags::RGB,  FormatFlags::Normalized),
    BC6H_SNORM      = makeFormat(FormatTypeFlags::BC6H_SNORM, FormatCompFlags::RGB,  FormatFlags::Normalized),
    BC7             = makeFormat(FormatTypeFlags::BC7,        FormatCompFlags::RGBA, FormatFlags::Normalized),
    BC7_SRGB        = makeFormat(FormatTypeFlags::BC7,        FormatCompFlags::RGBA, FormatFlags::Normalized | FormatFlags::sRGB),

    // Depth/Stencil
    D24             = makeFormat(FormatTypeFlags::D24,        FormatCompFlags::R,  FormatFlags::Depth),
    D24_S8          = makeFormat(FormatTypeFlags::D24S8,      FormatCompFlags::RG, FormatFlags::Depth | FormatFlags::Stencil),
    D32F            = makeFormat(FormatTypeFlags::D32F,       FormatCompFlags::R,  FormatFlags::Depth),
    D32F_S8         = makeFormat(FormatTypeFlags::D32FS8,     FormatCompFlags::RG, FormatFlags::Depth | FormatFlags::Stencil),

    // Special RT formats for preferred backend support.
    U8_RGB_RT       = makeFormat(FormatTypeFlags::U8,         FormatCompFlags::RGB,  FormatFlags::RenderTarget | FormatFlags::Normalized),
    U8_RGBA_RT      = makeFormat(FormatTypeFlags::U8,         FormatCompFlags::RGBA, FormatFlags::RenderTarget | FormatFlags::Normalized),
    U8_RGBA_RT_SRGB = makeFormat(FormatTypeFlags::U8,         FormatCompFlags::RGBA, FormatFlags::RenderTarget | FormatFlags::Normalized | FormatFlags::sRGB),
};
// clang-format on

enum class AttrLocation {
  Position,
  Tangent,
  Texcoord0,
  Texcoord1,
  Color,
};

template <class Ty>
  requires std::is_enum_v<Ty>
void WriteValue(BinWritterRef wr, Ty value) {
  WriteValue(wr, std::underlying_type_t<Ty>(value));
}

void WriteVertexArrayBase(SaveState wr) {
  WriteValue(wr, GfxFormat::S16_RGBA_NORM);
  WriteBool(wr, 0);
  WriteValue(wr, AttrLocation::Position);

  WriteValue(wr, GfxFormat::S32_RGBA_1010102_NORM);
  WriteValue(wr, 8);
  WriteValue(wr, AttrLocation::Tangent);

  WriteValue(wr, GfxFormat::F16_RG);
  WriteValue(wr, 12);
  WriteValue(wr, AttrLocation::Texcoord0);
}

void WriteVertexArrayTie(SaveState wr, bool useUV2) {
  WriteValue(wr, sizeof(StaticVertexLM));
  wr.Write(LoadType::StartRange);
  WriteVertexArrayBase(wr);

  WriteValue(wr, GfxFormat::U8_RG_NORM);
  WriteValue(wr, 16);
  WriteValue(wr, AttrLocation::Color);

  if (useUV2) {
    WriteValue(wr, GfxFormat::F16_RG);
    WriteValue(wr, 20);
    WriteValue(wr, AttrLocation::Texcoord1);
  }

  wr.Write(LoadType::EndRange);
}

void WriteVertexArrayZone(SaveState wr) {
  WriteValue(wr, sizeof(StaticVertexUV2));
  wr.Write(LoadType::StartRange);
  WriteVertexArrayBase(wr);

  WriteValue(wr, GfxFormat::U8_RGBA_NORM);
  WriteValue(wr, 16);
  WriteValue(wr, AttrLocation::Color);

  WriteValue(wr, GfxFormat::F16_RG);
  WriteValue(wr, 20);
  WriteValue(wr, AttrLocation::Texcoord1);

  wr.Write(LoadType::EndRange);
}

void WriteBuffer(BinWritterRef wr, auto buffer, GfxBufferUsage usage,
                 GfxBufferFrequencyHint type = GfxBufferFrequencyHint::Static) {
  buffer(wr);
  WriteValue(wr, type);
  WriteValue(wr, usage);
}

struct AABB {
  Vector min;
  Vector max;

  void Write(BinWritterRef wr) const {
    WriteValue(wr, min);
    WriteValue(wr, max);
  }
};

struct StaticDraw {
  uint32 startIndex;
  uint32 numIndices;
  uint32 material;
  AABB bounds;

  void Write(BinWritterRef wr) const {
    WriteValue(wr, startIndex);
    WriteValue(wr, numIndices);
    WriteValue(wr, material);
    wr.Write(bounds);
  }
};

struct DrawGroup {
  ResourceRange vertexBuffer;
  ResourceRange indexBuffer;
  std::vector<StaticDraw> draws;
};

void WriteValue(SaveState wr, const DrawGroup &group, auto attrs,
                auto vtxBuffer, auto idxBuffer) {
  wr.Write(LoadType::DrawItems);
  wr.Write(LoadType::VertexArrayObject);
  WriteBuffer(wr, vtxBuffer, GfxBufferUsage::Vertex);
  attrs(wr);
  WriteValue(wr, GfxFormat::U16_R);
  WriteBuffer(wr, idxBuffer, GfxBufferUsage::Index);
  wr.Write(LoadType::StartRange);
  wr.WriteContainer(group.draws);
  wr.Write(LoadType::EndRange);
}

struct Moby : DrawGroup {
  struct Bangle {
    uint16 start;
    uint16 count;

    void Write(BinWritterRef wr) const {
      WriteValue(wr, start);
      WriteValue(wr, count);
    }
  };
  std::vector<Bangle> bangles;

  void Write(BinWritterRef wr) const = delete;
};

void WriteValue(SaveState wr, const Moby &item) {
  WriteValue(
      wr, item, WriteVertexArrayBase,
      [&](BinWritterRef wr) { wr.Write(item.vertexBuffer); },
      [&](BinWritterRef wr) { wr.Write(item.indexBuffer); });
  wr.Write(LoadType::StartRange);
  wr.WriteContainer(item.bangles);
  wr.Write(LoadType::EndRange);
}

struct Tie {
  DrawGroup group1uv;
  DrawGroup group2uv;
  uint32 numVertices;

  void Write(BinWritterRef wr) const = delete;
};

void WriteValue(SaveState wr, const Tie &item) {
  wr.Write(LoadType::StartRange);
  WriteBool(wr, item.group1uv.draws.size() > 0);
  if (item.group1uv.draws.size() > 0) {
    WriteValue(
        wr, item.group1uv, [](SaveState wr) { WriteVertexArrayTie(wr, false); },
        [&](BinWritterRef wr) { wr.Write(item.group1uv.vertexBuffer); },
        [&](BinWritterRef wr) { wr.Write(item.group1uv.indexBuffer); });
  }
  WriteBool(wr, item.group2uv.draws.size() > 0);
  if (item.group2uv.draws.size() > 0) {
    WriteValue(
        wr, item.group2uv, [](SaveState wr) { WriteVertexArrayTie(wr, true); },
        [&](BinWritterRef wr) { wr.Write(item.group2uv.vertexBuffer); },
        [&](BinWritterRef wr) { wr.Write(item.group2uv.indexBuffer); });
  }
  wr.Write(LoadType::EndRange);
}

static std::map<uint16, Moby> MOBYS;
static std::mutex MOBY_MTX;
static BinWritter MOBY_WR;

static std::map<uint16, Tie> TIES;
static std::mutex TIE_MTX;
static BinWritter TIE_WR;

using HVector = t_Vector<float16>;
using HVector2 = t_Vector2<float16>;

SVector4 CompressQuat(const glm::dualquat::part_type &q) {
  Vector4A16 val(reinterpret_cast<const Vector4A16 &>(q));
  val *= 0x7fff;
  val = Vector4A16(_mm_round_ps(val._data, _MM_ROUND_NEAREST));
  return val.Convert<int16>();
}

struct TieInstanceDataLMapped {
  Vector4 dual;
  SVector4 real;
  HVector scale;
  float16 lmPageIndex = 0;
  uint32 lmSizeIndex : 8 = 0;
  uint32 lmColorOffset : 24 = 0;

  TieInstanceDataLMapped(const glm::dualquat &dq, Vector scale)
      : dual(reinterpret_cast<const Vector4 &>(dq.dual)),
        real(CompressQuat(dq.real)), scale(scale.Convert<float16>()) {}
};

static std::string DATA_FOLDER;

bool AppInitContext(const std::string &dataFolder) {
  DATA_FOLDER = dataFolder;
  MOBY_WR.Open(dataFolder + "mobys.dat");
  TIE_WR.Open(dataFolder + "ties.dat");
  return true;
}

using MaterialRemaps = std::map<uint16, uint32>;

template <class InVertex = Vertex0, class OutVertex = StaticVertex>
struct MikkUserData {
  std::vector<uint16> indices;
  uni::FormatCodec::fvec positions;
  std::vector<InVertex> vertices;
  std::vector<OutVertex> outVerts;
  Vector4A16 pMin{std::numeric_limits<float>::max()};
  Vector4A16 pMax{std::numeric_limits<float>::min()};
};

template <class InVertex = Vertex0, class OutVertex = StaticVertex>
void GenerateQTangents(MikkUserData<InVertex, OutVertex> &uData) {
  static const auto &NORMCODEC = uni::FormatCodec::Get(uni::FormatDescr{
      .outType = uni::FormatType::NORM,
      .compType = uni::DataType::R11G11B10,
  });

  SMikkTSpaceContext tctx{};
  tctx.m_pUserData = &uData;
  SMikkTSpaceInterface it{};
  it.m_getNumFaces = [](const SMikkTSpaceContext *c) -> int {
    return static_cast<MikkUserData<InVertex, OutVertex> *>(c->m_pUserData)
               ->indices.size() /
           3;
  };
  it.m_getNumVerticesOfFace = [](const SMikkTSpaceContext *, int) { return 3; };
  it.m_getPosition = [](const SMikkTSpaceContext *c, float *out, int face,
                        int vert) {
    auto uData =
        static_cast<MikkUserData<InVertex, OutVertex> *>(c->m_pUserData);
    const uint16 index = uData->indices.at(face * 3 + vert);
    Vector4A16 &pos = uData->positions.at(index);
    memcpy(out, &pos, sizeof(Vector));
  };

  it.m_getNormal = [](const SMikkTSpaceContext *c, float *out, int face,
                      int vert) {
    auto uData =
        static_cast<MikkUserData<InVertex, OutVertex> *>(c->m_pUserData);
    const uint16 index = uData->indices.at(face * 3 + vert);
    uint32 norm;
    memcpy(&norm, &uData->vertices.at(index).normal, 4);

    Vector4A16 outNorm;
    NORMCODEC.GetValue(outNorm, reinterpret_cast<const char *>(&norm));
    memcpy(out, &outNorm, 12);
  };

  it.m_getTexCoord = [](const SMikkTSpaceContext *c, float *out, int face,
                        int vert) {
    auto uData =
        static_cast<MikkUserData<InVertex, OutVertex> *>(c->m_pUserData);
    const uint16 index = uData->indices.at(face * 3 + vert);
    float16 *uv = uData->vertices.at(index).uv0;
    static const auto &UVCODEC = uni::FormatCodec::Get(uni::FormatDescr{
        .outType = uni::FormatType::FLOAT,
        .compType = uni::DataType::R16G16,
    });

    Vector4A16 outNorm;
    UVCODEC.GetValue(outNorm, reinterpret_cast<const char *>(uv));
    memcpy(out, &outNorm, 8);
  };

  it.m_setTSpaceBasic = [](const SMikkTSpaceContext *c, const float *fvTangent,
                           float fSign, int face, int vert) {
    auto uData =
        static_cast<MikkUserData<InVertex, OutVertex> *>(c->m_pUserData);
    uint16 &index = uData->indices.at(face * 3 + vert);
    es::Matrix44 mtx;
    uint32 norm;
    memcpy(&norm, &uData->vertices.at(index).normal, 4);
    NORMCODEC.GetValue(mtx.r3(), reinterpret_cast<const char *>(&norm));

    memcpy(reinterpret_cast<float *>(&mtx.r1()), fvTangent, 12);

    // orthogonalize tangent
    mtx.r1() -= mtx.r3() * mtx.r1().DotV(mtx.r3());
    mtx.r1().Normalize();
    mtx.r2() = mtx.r1().Cross(mtx.r3()) * fSign;
    Vector4A16 t, tang, s;
    mtx.Decompose(t, tang, s);

    // quat rule: q == -q
    if (tang.w < 0) {
      tang *= -1;
    }

    // Set reflection by flipping quat
    if (s.x * s.y * s.z < 0) {
      tang *= -1;
    }

    tang *= Vector4A16(0x1ff, 0x1ff, 0x1ff, 0x7fff);
    tang = Vector4A16(_mm_round_ps(tang._data, _MM_ROUND_NEAREST));

    // Apply bias needed for reflection
    if (tang.w == 0) {
      tang.w = s.x * s.y * s.z;
    }

    // Backward check for normal correctness
    auto pTang =
        tang * Vector4A16(1.f / 0x1ff, 1.f / 0x1ff, 1.f / 0x1ff, 1.f / 0x7fff);
    glm::quat qq(pTang.w, pTang.x, pTang.y, pTang.z);

    {
      auto oNorm = mtx.r3();
      glm::vec3 origNormal(oNorm.x, oNorm.y, oNorm.z);

      glm::vec3 normalLoc(0, 0, qq.w < 0 ? -1 : 1);
      glm::vec3 normalFromQuat = qq * normalLoc;
      glm::vec3 normalDiff = normalFromQuat - origNormal;

      float normalDiffTotal =
          abs(normalDiff.x) + abs(normalDiff.y) + abs(normalDiff.z);

      if (normalDiffTotal > 0.1) {
        printline(normalDiffTotal);
      }
    }

    {
      auto oTang = mtx.r1();
      glm::vec3 origTangent(oTang.x, oTang.y, oTang.z);

      auto pTang = tang * Vector4A16(1.f / 0x1ff, 1.f / 0x1ff, 1.f / 0x1ff,
                                     1.f / 0x7fff);
      glm::quat qq(pTang.w, pTang.x, pTang.y, pTang.z);

      glm::vec3 tangentLoc(qq.w < 0 ? -1 : 1, 0, 0);
      glm::vec3 tangentFromQuat = qq * tangentLoc;
      glm::vec3 tangentDiff = tangentFromQuat - origTangent;

      float tangentDiffTotal =
          abs(tangentDiff.x) + abs(tangentDiff.y) + abs(tangentDiff.z);

      if (tangentDiffTotal > 0.1) {
        printline(tangentDiffTotal);
      }
    }

    {
      auto oTang = mtx.r2();
      glm::vec3 origTangent(oTang.x, oTang.y, oTang.z);

      auto pTang = tang * Vector4A16(1.f / 0x1ff, 1.f / 0x1ff, 1.f / 0x1ff,
                                     1.f / 0x7fff);
      glm::quat qq(pTang.w, pTang.x, pTang.y, pTang.z);

      glm::vec3 tangentLoc(0, qq.w < 0 ? -1 : 1, 0);
      glm::vec3 tangentFromQuat = qq * tangentLoc;
      glm::vec3 tangentDiff = tangentFromQuat - origTangent;

      float tangentDiffTotal =
          abs(tangentDiff.x) + abs(tangentDiff.y) + abs(tangentDiff.z);

      if (tangentDiffTotal > 0.1) {
        printline(tangentDiffTotal);
      }
    }

    auto &oTang = uData->outVerts.at(index);

    if (oTang.qtangW == 0) {
      auto nTang = tang.Convert<int32>();
      oTang.qtang = nTang.x | (nTang.y << 10) | (nTang.z << 20);
      oTang.qtangW = nTang.w;
    } /* else if (oTang != nTang) {
       auto copiedVert = uData->vertices.at(index);
       index = uData->vertices.size();
       uData->vertices.emplace_back(copiedVert);
       uData->tangents.emplace_back(nTang);

       // Print out value in case of close values
       auto v = oTang - nTang;
       if ((abs(v.x) + abs(v.y) + abs(v.z) + abs(v.w)) < 1000) {
         printline(v.X << " " << v.Y << " " << v.Z << " " << v.W);
       }
     }*/
  };

  tctx.m_pInterface = &it;

  genTangSpaceDefault(&tctx);
}

template <class InVertex = Vertex0, class OutVertex = StaticVertex>
void ProcessVertices(MikkUserData<InVertex, OutVertex> &uData,
                     Vector4A16 meshScale) {
  static const auto &POSCODEC = uni::FormatCodec::Get(uni::FormatDescr{
      .outType = uni::FormatType::INT,
      .compType = uni::DataType::R16G16B16,
  });

  uData.positions.reserve(uData.vertices.size());

  for (uint32 v = 0; v < uData.vertices.size(); v++) {
    IVector4A16 ipos;
    POSCODEC.GetValue(
        ipos, reinterpret_cast<const char *>(uData.vertices.data() + v));
    Vector4A16 fpos(ipos);
    fpos *= meshScale;
    uData.positions.emplace_back(fpos);
    auto I = [](__m128 i) { return reinterpret_cast<__m128i &>(i); };
    auto F = [](__m128i i) { return reinterpret_cast<__m128 &>(i); };
    uData.pMax._data = F(_mm_max_epi32(I(fpos._data), I(uData.pMax._data)));
    uData.pMin._data = F(_mm_min_epi32(I(fpos._data), I(uData.pMin._data)));
  }

  GenerateQTangents(uData);

  Vector4A16 center = (uData.pMax + uData.pMin) * 0.5;
  Vector4A16 scale = uData.pMax - center;
  Vector4A16 invScale = (Vector4A16(1.f) / scale) * 0x7fff;

  for (uint32 v = 0; auto &f : uData.positions) {
    Vector4A16 npos((f - center) * invScale);
    Vector4A16 rpos(_mm_round_ps(npos._data, _MM_ROUND_NEAREST));
    uData.outVerts.at(v++).pos = Vector(rpos).Convert<int16>();
  }

  for (uint32 v = 0; auto &i : uData.vertices) {
    OutVertex &vtx = uData.outVerts.at(v++);
    vtx.uv = reinterpret_cast<const TexCoord &>(i.uv0);

    if constexpr (std::is_same_v<OutVertex, StaticVertexLM>) {
      int purp = abs(i.purpose);
      vtx.bghtAlpha.x = purp >> 7;
      vtx.bghtAlpha.y = purp << 1;
    }

    if constexpr (std::is_same_v<InVertex, RegionVertex>) {
      int purp = abs(i.purpose);
      vtx.bghtAlpha.x = purp >> 7;
      vtx.bghtAlpha.y = purp << 1;
      vtx.lmData = i.lightColor;
      vtx.uv1 = reinterpret_cast<const TexCoord &>(i.uv1);
    }
  }
}

void CreateMoby(const MobyV1 &moby, BinReaderRef_e stream) {
  Moby &rMoby = MOBYS[moby.mobyId];
  rMoby.vertexBuffer.dataOffset = MOBY_WR.Tell();
  uint32 totalVerts = 0;
  std::vector<uint16> indices;

  for (uint32 i = 0; i < moby.numMeshes; i++) {
    const MeshV1 &mesh = moby.meshes[i];
    rMoby.bangles.emplace_back(
        Moby::Bangle{uint16(rMoby.draws.size()), uint16(mesh.numPrimitives)});
    for (uint32 p = 0; p < mesh.numPrimitives; p++) {
      const PrimitiveV1 &prim = mesh.primitives[p];
      StaticDraw &draw = rMoby.draws.emplace_back();
      draw.material = prim.materialIndex;
      draw.numIndices = prim.numIndices;
      stream.Seek(moby.vertexBufferOffset + prim.vertexBufferOffset);

      if (prim.vertexFormat == 0) {
        MikkUserData uData;
        uData.outVerts.resize(prim.numVertices);
        stream.ReadContainer(uData.vertices, prim.numVertices);
        stream.Seek(moby.indexBufferOffset + prim.indexOffset * 2);
        stream.ReadContainer(uData.indices, prim.numIndices);
        ProcessVertices(uData, moby.meshScale);
        for (uint16 &idx : uData.indices) {
          idx += totalVerts;
        }
        indices.insert(indices.end(), uData.indices.begin(),
                       uData.indices.end());
        MOBY_WR.WriteContainer(uData.vertices);
        draw.bounds.min = uData.pMin;
        draw.bounds.max = uData.pMax;
      } else {
        std::vector<Vertex1> vtx1;
        stream.ReadContainer(vtx1, prim.numVertices);
      }

      totalVerts += prim.numVertices;
    }
  }

  rMoby.vertexBuffer.dataSize = MOBY_WR.Tell() - rMoby.vertexBuffer.dataOffset;
  rMoby.indexBuffer.dataOffset = MOBY_WR.Tell();
  MOBY_WR.WriteContainer(indices);
  rMoby.indexBuffer.dataSize = MOBY_WR.Tell() - rMoby.indexBuffer.dataOffset;

  if (totalVerts >= 0xffff)
    PrintError("MobyVerts: ", totalVerts);
  totalVerts = 0;

  for (StaticDraw &d : rMoby.draws) {
    d.startIndex = totalVerts;
    totalVerts += d.numIndices;
  }
}

void GatherMobys(IGHWTOCIteratorConst<MobyV1> &mobys, BinReaderRef_e stream) {
  for (const MobyV1 &m : mobys) {
    auto found = MOBYS.find(m.mobyId);

    if (found == MOBYS.end()) {
      CreateMoby(m, stream);
    } /*else {
      uint32 numDraws = 0;
      uint32 numVerts = 0;
      uint32 numIndices = 0;

      for (uint32 i = 0; i < m.numMeshes; i++) {
        const MeshV1 &mesh = m.meshes[i];
        numDraws += mesh.numPrimitives;
        for (uint32 p = 0; p < mesh.numPrimitives; p++) {
          const PrimitiveV1 &prim = mesh.primitives[p];
          numVerts += prim.numVertices;
          numIndices += prim.numIndices;
        }
      }

      uint32 numFoundVerts = 0;
      uint32 numFoundIndices = 0;

      for (auto &d : found->second.draws) {
        numFoundIndices += d.numIndices;
        numFoundVerts += d.numVertices;
      }

      if (found->second.draws.size() != numDraws) {
        PrintWarning("Moby drawcount mismatch: ", found->second.draws.size(),
                     " != ", numDraws);
      } else if (numFoundVerts != numVerts) {
        PrintWarning("Moby vertices mismatch: ", numFoundVerts,
                     " != ", numVerts);
      } else if (numFoundIndices != numIndices) {
        PrintWarning("Moby indices mismatch: ", numFoundIndices,
                     " != ", numIndices);
      }
    }*/
  }
}

void CreateTie(const TieV1 &tie, const LevelIndexBuffer &idxBuffer,
               const LevelVertexBuffer &vtxBuffer) {
  Tie &rTie = TIES[tie.tieId];
  rTie.group1uv.vertexBuffer.dataOffset = TIE_WR.Tell();
  uint32 totalVerts = 0;
  std::vector<uint16> indicesVtx;
  const uint16 *indexBuffer = &idxBuffer.data;
  const char *vertexBuffer = &vtxBuffer.data;

  for (uint32 i = 0; i < tie.numMeshes; i++) {
    const TiePrimitiveV1 &prim = tie.primitives[i];
    if (prim.useUv2) {
      continue;
    }

    StaticDraw &draw = rTie.group1uv.draws.emplace_back();
    draw.material = prim.materialIndex;
    draw.numIndices = prim.numIndices;
    MikkUserData<Vertex0, StaticVertexLM> uData;
    uData.outVerts.resize(prim.numVertices);

    const uint16 *indices = indexBuffer + prim.indexOffset;
    const Vertex0 *vertices = reinterpret_cast<const Vertex0 *>(
                                  vertexBuffer + tie.vertexBufferOffset0) +
                              prim.vertexOffset0;

    uData.vertices = {vertices, vertices + prim.numVertices};

    for (auto &v : uData.vertices) {
      FByteswapper(v);
    }

    uData.indices = {indices, indices + prim.numIndices};
    for (uint16 &i : uData.indices) {
      FByteswapper(i);
    }

    ProcessVertices(uData, tie.meshScale);
    for (uint16 &idx : uData.indices) {
      idx += totalVerts;
    }
    indicesVtx.insert(indicesVtx.end(), uData.indices.begin(),
                      uData.indices.end());
    TIE_WR.WriteContainer(uData.vertices);
    draw.bounds.min = uData.pMin;
    draw.bounds.max = uData.pMax;

    totalVerts += prim.numVertices;
  }

  rTie.group1uv.vertexBuffer.dataSize =
      TIE_WR.Tell() - rTie.group1uv.vertexBuffer.dataOffset;
  rTie.group1uv.indexBuffer.dataOffset = TIE_WR.Tell();
  TIE_WR.WriteContainer(indicesVtx);
  rTie.group1uv.indexBuffer.dataSize =
      TIE_WR.Tell() - rTie.group1uv.indexBuffer.dataOffset;

  if (totalVerts >= 0xffff)
    PrintError("TieVerts: ", totalVerts);
  rTie.numVertices = totalVerts;
  totalVerts = 0;
  indicesVtx.clear();

  for (uint32 i = 0; i < tie.numMeshes; i++) {
    const TiePrimitiveV1 &prim = tie.primitives[i];
    if (!prim.useUv2) {
      continue;
    }

    StaticDraw &draw = rTie.group2uv.draws.emplace_back();
    draw.material = prim.materialIndex;
    draw.numIndices = prim.numIndices;
    MikkUserData<Vertex0, StaticVertexUV2> uData;
    uData.outVerts.resize(prim.numVertices);

    const uint16 *indices = indexBuffer + prim.indexOffset;
    const Vertex0 *vertices = reinterpret_cast<const Vertex0 *>(
                                  vertexBuffer + tie.vertexBufferOffset0) +
                              prim.vertexOffset0;
    const HVector2 *uv2s = reinterpret_cast<const HVector2 *>(
                               vertexBuffer + tie.vertexBufferOffset1) +
                           prim.vertexOffset1;

    uData.vertices = {vertices, vertices + prim.numVertices};

    for (uint32 vi = 0; auto &v : uData.vertices) {
      FByteswapper(v);
      uData.outVerts.at(vi).uv1 = uv2s[vi];
      FByteswapper(uData.outVerts.at(vi).uv1);
      vi++;
    }

    uData.indices = {indices, indices + prim.numIndices};
    for (uint16 &i : uData.indices) {
      FByteswapper(i);
    }

    ProcessVertices(uData, tie.meshScale);
    for (uint16 &idx : uData.indices) {
      idx += totalVerts;
    }
    indicesVtx.insert(indicesVtx.end(), uData.indices.begin(),
                      uData.indices.end());
    TIE_WR.WriteContainer(uData.vertices);
    draw.bounds.min = uData.pMin;
    draw.bounds.max = uData.pMax;

    totalVerts += prim.numVertices;
  }

  rTie.group2uv.vertexBuffer.dataSize =
      TIE_WR.Tell() - rTie.group2uv.vertexBuffer.dataOffset;
  rTie.group2uv.indexBuffer.dataOffset = TIE_WR.Tell();
  TIE_WR.WriteContainer(indicesVtx);
  rTie.group2uv.indexBuffer.dataSize =
      TIE_WR.Tell() - rTie.group2uv.indexBuffer.dataOffset;

  rTie.numVertices += totalVerts;

  if (totalVerts >= 0xffff)
    PrintError("TieVerts: ", totalVerts);
}

void GatherTies(IGHWTOCIteratorConst<TieV1> &ties,
                const LevelIndexBuffer &idxBuffer,
                const LevelVertexBuffer &vtxBuffer) {
  for (const TieV1 &t : ties) {
    auto found = TIES.find(t.tieId);

    if (found == TIES.end()) {
      CreateTie(t, idxBuffer, vtxBuffer);
    } /*else {
      uint32 numVerts = 0;
      uint32 numIndices = 0;

      for (uint32 i = 0; i < t.numMeshes; i++) {
        const TiePrimitiveV1 &prim = t.primitives[i];
        numVerts += prim.numVertices;
        numIndices += prim.numIndices;
      }

      uint32 numFoundVerts = 0;
      uint32 numFoundIndices = 0;

      for (auto &d : found->second.draws) {
        numFoundIndices += d.numIndices;
        numFoundVerts += d.numVertices;
      }

      if (found->second.draws.size() != t.numMeshes) {
        PrintWarning("Tie drawcount mismatch: ", found->second.draws.size(),
                     " != ", t.numMeshes);
      } else if (numFoundVerts != numVerts) {
        PrintWarning("Tie vertices mismatch: ", numFoundVerts,
                     " != ", numVerts);
      } else if (numFoundIndices != numIndices) {
        PrintWarning("Tie indices mismatch: ", numFoundIndices,
                     " != ", numIndices);
      }
    }*/
  }
}
#include <cmath>
#include <fstream>
/*void GatherTieInstances(IGHWTOCIteratorConst<TieInstanceV1> &instances,
                        uint16 levelIndex) {
  auto &rInsts = ZONES[levelIndex].tieInstances;
  // todo extract chroma

  for (auto &i : instances) {
    Vector4A16 position, rotation, scale;
    i.tm.Decompose(position, rotation, scale);
    const glm::vec3 &gPos = reinterpret_cast<const glm::vec3 &>(position);
    glm::quat tmq(rotation.w, rotation.x, rotation.y, rotation.z);
    glm::dualquat dq(tmq, gPos);
    rInsts[i.tie->tieId].emplace_back(InstanceTm{
        .tm = dq,
        .scale = scale,
        .lightMap = i.lightMapIndex,
    });
  }
}*/

void AppProcessFile(AppContext *ctx) {
  BinReaderRef_e rd(ctx->GetStream());
  IGHW main;
  main.FromStream(rd, Version::RFOM);
  std::string workFolder(ctx->workingFile.GetFolder());

  IGHWTOCIteratorConst<MobyV1> mobys;
  IGHWTOCIteratorConst<TieV1> ties;
  IGHWTOCIteratorConst<TieInstanceV1> tieInstances;
  IGHWTOCIteratorConst<RegionMesh> regionMeshes;
  IGHWTOCIteratorConst<DirectionalLightmapTextureV1> lightmaps;
  IGHWTOCIteratorConst<TextureV1> textures;
  IGHWTOCIteratorConst<BlendmapTextureV1> blendMaps;
  IGHWTOCIteratorConst<MaterialV1> materials;
  IGHWTOCIteratorConst<DetailCluster> detailClusters;
  IGHWTOCIteratorConst<Foliage> foliages;
  IGHWTOCIteratorConst<Shrub> shrubs;
  auto txStr = ctx->RequestFile(workFolder + "ps3leveltexs.dat");
  auto vtxStr = ctx->RequestFile(workFolder + "ps3levelverts.dat");
  IGHW buffers;
  buffers.FromStream(*vtxStr.Get(), Version::RFOM);
  IGHWTOCIteratorConst<LevelVertexBuffer> verts;
  IGHWTOCIteratorConst<LevelIndexBuffer> indices;
  CatchClasses(buffers, verts, indices);
  CatchClasses(main, mobys, ties, tieInstances, regionMeshes, lightmaps,
               textures, blendMaps, materials, detailClusters, foliages,
               shrubs);
  BinReaderRef_e txRd(*txStr.Get());
  txRd.SwapEndian(true);

  workFolder.pop_back();
  auto found = workFolder.rfind("level");
  BinWritter wr(DATA_FOLDER + workFolder.substr(found));
  std::map<std::string, uint32> strings;
  SaveState wrState(wr, strings);
  WriteValue(wr, 1); // revision

  const uint16 *indexBuffer = &indices.at(0).data;
  const char *vertexBuffer = &verts.at(0).data;
  DrawGroup rZone{};
  uint32 totalVerts = 0;
  std::vector<uint16> indicesVtx;
  std::stringstream vstr;
  BinWritterRef wrs(vstr);

  auto SaveCluster = [&] {
    WriteValue(
        wrState, rZone, WriteVertexArrayZone,
        [&](BinWritterRef wr) {
          wr.Write(LoadType::InlineBuffer);
          std::string str = std::move(vstr).str();
          WriteValue(wr, str.size());
          wr.WriteContainer(str);
        },
        [&](BinWritterRef wr) {
          wr.Write(LoadType::InlineBuffer);
          WriteValue(wr, indicesVtx.size() * 2);
          wr.WriteContainer(indicesVtx);
        });
  };

  wr.Write(LoadType::StartRange);

  for (const RegionMesh &item : regionMeshes) {
    if (totalVerts + item.numVerties >= 0xffff) {
      SaveCluster();
      indicesVtx.clear();
      rZone = {};
      totalVerts = 0;
    }
    StaticDraw &draw = rZone.draws.emplace_back();
    draw.material = item.materialIndex0;
    draw.numIndices = item.numIndices;
    MikkUserData<RegionVertex, StaticVertexUV2> uData;
    uData.outVerts.resize(item.numVerties);
    const uint16 *indices = indexBuffer + item.indexOffset;
    uData.indices = {indices, indices + item.numIndices};
    for (uint16 &i : uData.indices) {
      FByteswapper(i);
    }
    const RegionVertex *vertices = reinterpret_cast<const RegionVertex *>(
        vertexBuffer + item.vertexOffset);

    uData.vertices = {vertices, vertices + item.numVerties};

    for (auto &v : uData.vertices) {
      FByteswapper(v);
    }

    const float meshScale = 1.f / 0x100;
    ProcessVertices(uData, meshScale);
    for (uint16 &idx : uData.indices) {
      idx += totalVerts;
    }
    indicesVtx.insert(indicesVtx.end(), uData.indices.begin(),
                      uData.indices.end());
    wrs.WriteContainer(uData.vertices);
    Vector4A16 bMin(uData.pMin + Vector4A16(item.position));
    Vector4A16 bMax(uData.pMax + Vector4A16(item.position));
    draw.bounds.min = bMin;
    draw.bounds.max = bMax;

    totalVerts += item.numVerties;
  }

  SaveCluster();
  wr.Write(LoadType::EndRange);

  {
    struct TieInstance {
      std::vector<TieInstanceDataLMapped> tms;
      std::vector<const TieInstanceV1 *> data;
    };
    std::map<uint16, TieInstance> zTieInstances;
    std::map<uint16, const TieV1 *> zTies;

    for (auto &t : ties) {
      zTies.emplace(t.tieId, &t);
    }

    for (auto &i : tieInstances) {
      Vector4A16 position, rotation, scale;
      i.tm.Decompose(position, rotation, scale);
      const glm::vec3 &gPos = reinterpret_cast<const glm::vec3 &>(position);
      glm::quat tmq(rotation.w, rotation.x, rotation.y, rotation.z);
      glm::dualquat dq(tmq, gPos);
      zTieInstances[i.tie->tieId].tms.emplace_back(dq, scale);
      zTieInstances[i.tie->tieId].data.emplace_back(&i);
    }

    for (auto &[id, idt] : zTieInstances) {
      Tie *tie = nullptr;

      {
        std::lock_guard lg(TIE_MTX);
        auto found = TIES.find(id);

        if (found == TIES.end()) {
          CreateTie(*zTies.at(id), indices.at(0), verts.at(0));
          tie = &TIES.at(id);
        } else {
          tie = &found->second;
        }
      }

      std::stringstream tstr;
      WriteValue(SaveState{tstr, strings}, *tie);
      std::string sstr = std::move(tstr).str();
      wr.Write(LoadType::CheckResource);
      WriteValue(wr, 0x10000 | id);
      WriteValue(wr, sstr.size());
      wr.WriteContainer(sstr);

      uint32 numLMInstances = 0;
      uint32 colorBufferOffset = 0;
      std::map<uint64, std::vector<uint16>> lightMaps;
      std::map<uint16, uint8> lightMapSizes;

      for (int32 ii = -1; auto &d : idt.data) {
        ii++;
        if (d->lightMapIndex == 0xffff) {
          continue;
        }

        auto &lightMap = lightmaps.at(d->lightMapIndex);
        auto &cLm = lightMaps[lightMap.width];
        auto &iLm = idt.tms.at(ii);
        iLm.lmPageIndex = cLm.size();
        iLm.lmColorOffset = colorBufferOffset;
        colorBufferOffset += tie->numVertices;
        cLm.emplace_back(d->lightMapIndex);

        if (auto found = lightMapSizes.find(lightMap.width);
            found != lightMapSizes.end()) {
          iLm.lmSizeIndex = found->second;
        } else {
          iLm.lmSizeIndex = lightMapSizes.size();
          lightMapSizes.emplace(lightMap.width, lightMapSizes.size());
        }

        numLMInstances++;
      }

      // Write instances
      WriteBuffer(
          wr,
          [&](BinWritterRef wr) {
            wr.Write(LoadType::InlineBuffer);
            WriteValue(wr, idt.tms.size() * sizeof(TieInstanceDataLMapped));
            wr.WriteContainer(idt.tms);
          },
          GfxBufferUsage::Storage);

      // Write instance color buffers
      WriteBuffer(
          wr,
          [&](BinWritterRef wr) {
            wr.Write(LoadType::InlineBuffer);
            WriteValue(wr, tie->numVertices * 2 * idt.data.size());

            for (auto &d : idt.data) {
              txRd.Seek(d->offset1);
              std::string buffer;
              txRd.ReadContainer(buffer, tie->numVertices * 2);
              wr.WriteContainer(buffer);
            }
          },
          GfxBufferUsage::Storage);

      WriteBool(wr, numLMInstances > 0);
      if (numLMInstances > 0) {
        WriteBuffer(
            wr,
            [&](BinWritterRef wr) {
              wr.Write(LoadType::InlineBuffer);
              WriteValue(wr, tie->numVertices * 2 * numLMInstances);

              for (auto &d : idt.data) {
                if (d->lightMapIndex == 0xffff) {
                  continue;
                }

                std::string buffer;
                txRd.Seek(d->offset0);
                txRd.ReadContainer(buffer, tie->numVertices * 2);
                wr.WriteContainer(buffer);
              }
            },
            GfxBufferUsage::Storage);
      }
    }
  }
}
