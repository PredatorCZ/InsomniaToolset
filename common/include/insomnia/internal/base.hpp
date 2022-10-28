/*  InsomniaLib
    Copyright(C) 2021 Lukas Cone

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
#include "datas/pointer.hpp"
#include "datas/pugi_fwd.hpp"
#include "datas/supercore.hpp"
#include "insomnia/internal/settings.hpp"

struct IGHW;

struct CoreClass {};

struct Hash {
  uint32 part1;
  uint32 part2;

  bool operator==(Hash other) const {
    return part1 == other.part1 && part2 == other.part2;
  }
};
