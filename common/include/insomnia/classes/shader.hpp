/*  InsomniaLib
    Copyright(C) 2021 Lukas Cone
    Texture class derived from RPCS3 project

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
#include "datas/bitfield.hpp"
#include "datas/vectors_simd.hpp"
#include "insomnia/internal/base.hpp"

struct TextureResource : CoreClass {
  static constexpr uint32 ID = 0x5a00;
  uint32 hash;
  uint32 totalSize;

  bool operator==(uint32 other) const { return hash == other; }
};

struct Material : CoreClass {
  static constexpr uint32 ID = 0x5000;
  int32 diffuseMapId;
  int32 normalMapId;
  int32 specularMapId;
  int32 detailMapId;
  uint32 unk0; // flags?
  Vector4A16 values[6];
};

struct TextureFlags {
  using location = BitMemberDecl<3, 2>;
  using isCubemap = BitMemberDecl<2, 1>;
  using useBorder = BitMemberDecl<1, 1>;
  using dimension = BitMemberDecl<0, 4>;
  using Type = BitFieldType<uint8, dimension, useBorder, isCubemap, location>;
};

using TextureFlagsType = TextureFlags::Type;

enum class WrapMode {
  Wrap,
  Mirror,
  ClampOnEdge,
  Border,
  Clamp,
  MirrorOnceClampToEdge,
  MirrorOnceBorder,
  MirrorOnceClamp,
};

enum class MaxAnisotropy {
  x1,
  x2,
  x4,
  x6,
  x8,
  x10,
  x12,
  x16,
};

enum class ZComparisonFunction {
  NOP,
  LT,
  EQ,
  LE,
  GT,
  NOT,
  GE,
  Always,
};

struct TextureAddress {
  using wrapS = BitMemberDecl<7, 4, WrapMode>;
  using anisotropicBias = BitMemberDecl<6, 4>;
  using wrapT = BitMemberDecl<5, 4, WrapMode>;
  using unsignedRemap = BitMemberDecl<4, 4>;
  using wrapR = BitMemberDecl<3, 4, WrapMode>;
  using gamma = BitMemberDecl<2, 4>;
  using signedRemap = BitMemberDecl<1, 4>;
  using zFunc = BitMemberDecl<0, 4, ZComparisonFunction>;
  using Type = BitFieldType<uint32, zFunc, signedRemap, gamma, wrapR,
                            unsignedRemap, wrapT, anisotropicBias, wrapS>;
};

using TextureAddressType = TextureAddress::Type;

struct TextureControl0 {
  using unk0 = BitMemberDecl<6, 2>;
  using alphaKill = BitMemberDecl<5, 1>; // ?
  using unk1 = BitMemberDecl<4, 1>;
  using maxAnisotropy = BitMemberDecl<3, 3, MaxAnisotropy>;
  using maxLod = BitMemberDecl<2, 12>;
  using minLod = BitMemberDecl<1, 12>;
  using isEnabled = BitMemberDecl<0, 1>;
  using Type = BitFieldType<uint32, isEnabled, minLod, maxLod, maxAnisotropy,
                            unk1, alphaKill, unk0>;
};

using TextureControl0Type = TextureControl0::Type;

struct TextureControl3 {
  using pitch = BitMemberDecl<1, 20>;
  using depth = BitMemberDecl<0, 12>;
  using Type = BitFieldType<uint32, depth, pitch>;
};

using TextureControl3Type = TextureControl3::Type;

// NV4097_SET_*TEXTURE_* registry dump
struct Texture : CoreClass {
  static constexpr uint32 ID = 0x5200;
  uint32 offset;
  uint16 numMips;
  uint8 format;
  TextureFlagsType flags;
  TextureAddressType address;
  TextureControl0Type control0;
  TextureControl3Type control3;
  uint32 filter;
  uint16 width;
  uint16 height;
  uint32 borderColor;
};

struct ShaderResourceLookup : CoreClass {
  static constexpr uint32 ID = 0x5d00;
  Hash hash;
  es::PointerX86<char> lookupPath;
  uint32 null;
  uint32 mapHashes[4];
  es::PointerX86<char> mapLookupPaths[4];
};

namespace aggregators {
void IS_EXTERN Material(IGHW &main, pugi::xml_node node);
}
