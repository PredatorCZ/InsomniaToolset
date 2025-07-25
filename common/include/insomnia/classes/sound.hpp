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

struct SCREAMSection {
  enum Type {
    TYPE_BANK,
    TYPE_DATA,
  };

  es::PointerX86<char> data;
  uint32 size;
};

struct SCREAMBankHeader {
  uint32 version;
  uint32 numSections;
  SCREAMSection sections[];
};

struct SCREAMGain {
  uint32 streamOffset : 24;
  uint32 type : 8;
  uint32 unk1;
};

struct SCREAMSound {
  uint8 unk0;
  uint8 unk1;
  uint16 unk01;
  uint8 numGains;
  uint8 unk;
  uint16 flags;
  es::PointerX86<SCREAMGain> gains;
};

struct SCREAMWaveform {
  uint8 unk0;
  uint8 unk1;
  int8 centerNote;
  int8 centerFine;
  uint32 unk2[2];
  uint16 unk3;
  uint16 flags;
  uint32 streamOffset;
  uint32 streamSize;
};

struct SCREAMBank {
  uint32 id;
  uint32 version;
  uint32 flags;
  uint16 null0[5];
  uint16 numSounds;
  uint16 numGains;
  uint16 unk0;
  es::PointerX86<SCREAMSound> sounds;
  es::PointerX86<SCREAMGain> gains;
  uint32 unk2;
  uint32 dataSize0;
  uint32 dataSize1;
  uint32 null2;
  es::PointerX86<char> gainData;
  uint32 unk1;
  uint32 null1;
};

struct SoundNames : CoreClass {
  static constexpr uint32 ID = 0x21100;

  char names[1][64];
};

struct SoundNamesV2 : SoundNames {
  static constexpr uint32 ID = 0x21200;
};

struct Sound {
  uint16 type;
  int16 index;
};

struct Sounds : CoreClass {
  static constexpr uint32 ID = 0x21200;

  uint32 numSounds;
  uint32 null[3];
  Sound sounds[1];

  void OpenEnded();
};

struct SoundsV2 : Sounds {
  static constexpr uint32 ID = 0x21300;
  void OpenEnded();
};

struct SoundBank : CoreClass {
  static constexpr uint32 ID = 0x21000;

  char unk[144];
  SCREAMBankHeader bank;

  void OpenEnded();
};

struct SoundStreams : CoreClass {
  static constexpr uint32 ID = 0x21010;

  char unk0[128];
  es::PointerX86<uint32> streamOffsets;
  es::PointerX86<uint32> unk1;

  void OpenEnded();
};

struct SoundStreamsV2 : SoundStreams {
  static constexpr uint32 ID = 0x21100;
  void OpenEnded();
};
