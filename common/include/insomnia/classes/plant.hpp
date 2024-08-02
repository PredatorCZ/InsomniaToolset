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
#include <span>

struct PlantPrimitive : CoreClass {
  static constexpr uint32 ID = 0xC700;

  uint32 vertexBufferOffset;
  uint32 indexOffset;
  uint32 numIndices;
  uint32 unk0;
  float unk1[4];
  uint16 materialIndex;
  uint32 unk2[3];
};


struct PlantClusterInstance {
  es::Matrix44 tm;
  uint32 unk0[2];
  uint32 unk1;
  uint32 unk2;
};

struct PlantClusters : CoreClass {
  static constexpr uint32 ID = 0xC650;

  uint32 unk0;
  uint32 unkOffset;
  uint32 numInstances;
  uint32 instancesOffset;
  uint32 unk4;
  uint32 unk5;
  uint32 null0[2];
  float unk6[3];
  uint32 unk7;
  uint32 null1[20];
  uint32 unk8[12];

  std::span<PlantClusterInstance> Instances() {
    return {reinterpret_cast<PlantClusterInstance *>(reinterpret_cast<char *>(this) +
                                            instancesOffset),
            numInstances};
  }
};
