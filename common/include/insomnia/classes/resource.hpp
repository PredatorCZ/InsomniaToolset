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

template <uint32 id_> struct ResourceLookup : CoreClass {
  static constexpr uint32 ID = id_;
  Hash hash;
  uint32 offset;
  uint32 size;

  bool operator<(const Hash other) const { return hash < other; }
  bool operator==(const Hash other) const { return hash == other; }
  bool operator==(uint32 other) const { return hash.part2 == other; }
};

using ResourceLighting = ResourceLookup<0x1db00>;
using ResourceZones = ResourceLookup<0x1da00>;
using ResourceAnimsets = ResourceLookup<0x1d700>;
using ResourceMobys = ResourceLookup<0x1d600>;
using ResourceShrubs = ResourceLookup<0x1d500>;
using ResourceTies = ResourceLookup<0x1d300>;
using ResourceFoliages = ResourceLookup<0x1d400>;
using ResourceCubemap = ResourceLookup<0x1d200>;
using ResourceShaders = ResourceLookup<0x1d100>;
using ResourceHighmips = ResourceLookup<0x1d1c0>;
using ResourceTextures = ResourceLookup<0x1d180>;
using ResourceCinematics = ResourceLookup<0x1d800>;

static constexpr uint32 ResourceMobyPathLookupId = 0xd200;
static constexpr uint32 ResourceTiePathLookupId = 0x3410;
static constexpr uint32 ResourceShrubPathLookupId = 0xb700;
static constexpr uint32 ResourceCinematicPathLookupId = 0x17d00;

template <uint32 id_> struct ResourceNameLookup : CoreClass {
  static constexpr uint32 ID = id_;
  Hash hash;
  es::PointerX86<char> path;
  uint32 unk;

  bool operator<(const Hash other) const { return hash < other; }
  bool operator==(const Hash other) const { return hash == other; }
  bool operator==(uint32 other) const { return hash.part2 == other; }
};

using ResourceMobyPath = ResourceNameLookup<0x9480>;
using ResourceTiePath = ResourceNameLookup<0x9280>;

struct EffectTextureBuffer : CoreClass {
  static constexpr uint32 ID = 0x5300;
  char data;
};

struct VertexBuffer : CoreClass {
  static constexpr uint32 ID = 0xe200;
  char data;
};

struct IndexBuffer : CoreClass {
  static constexpr uint32 ID = 0xe100;
  uint16 data;
};

struct LevelVertexBuffer : CoreClass {
  static constexpr uint32 ID = 0x9000;
  char data;
};

struct LevelIndexBuffer : CoreClass {
  static constexpr uint32 ID = 0x9100;
  uint16 data;
};
