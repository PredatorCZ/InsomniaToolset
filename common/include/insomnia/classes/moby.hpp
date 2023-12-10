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

struct Moby : CoreClass {
  static constexpr uint32 ID = 0xd100;

  float unk00[4];
  uint32 unk01[2];
  uint16 unk10;
  uint16 unk11;
  uint16 numBones;
  uint16 unk13;
  uint32 null00;
  es::PointerX86<char> boneRemaps;
  es::PointerX86<char> bones;
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
