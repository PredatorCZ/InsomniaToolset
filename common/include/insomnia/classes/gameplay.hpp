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
#include <span>

struct IGHWHeader;

struct GameplayMoby {
  uint32 classIndex;
  uint32 unk;
  uint32 null[2];
};

struct GameplayInstanceMoby {
  es::PointerX86<IGHWHeader> embeded;
  uint32 unk0;
  float unk1[2];
  Vector position;
  Vector rotataion;
  uint32 null1;
  int32 unk2;
  uint32 null2;
  uint32 embedSize;
  float unk3;
  uint16 mobyClassIndex;
  uint16 null3;
};

template <class C> struct GameplayInstancesGroup {
  es::PointerX86<C> items;
  uint32 numItems;
  uint32 null[2];

  const C *begin() const { return items; }
  const C *end() const { return begin() + numItems; }
  C *begin() { return items; }
  C *end() { return begin() + numItems; }
};

struct GameplayInstances {
  char name[64];
  GameplayInstancesGroup<GameplayInstanceMoby> mobys;
  GameplayInstancesGroup<char> other[6];
  es::PointerX86<char> unk0;
  es::PointerX86<char> unk1;
  uint32 unk2;
  uint32 unk3;
};

struct Gameplay : CoreClass {
  static constexpr uint32 ID = 0x25000;

  es::PointerX86<GameplayMoby> mobys;
  uint32 numMobys;
  uint32 null0[2];
  es::PointerX86<char> data1;
  uint32 unk1;
  uint32 unk2;
  uint32 unk3;
  es::PointerX86<GameplayInstances> instances;
  uint32 unk5;
  uint32 unk6[2];

  std::span<GameplayMoby> Mobys() { return {mobys.Get(), numMobys}; }
  std::span<const GameplayMoby> Mobys() const {
    return {mobys.Get(), numMobys};
  }
};
