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
#include "spike/type/flags.hpp"
#include "spike/type/vectors_simd.hpp"
#include <span>

enum TrackType : uint16 {
  Rotation,
  Scale,
  Position,
};

struct TrackMask {
  uint16 unk : 2;
  uint16 component : 2;
  TrackType type : 2;
  uint16 boneIndex : 10;

  void Swap();
};

static_assert(sizeof(TrackMask) == 2);

struct RootMotionFrame {
  Vector4A16 rotation;
  Vector4A16 scale;
  Vector4A16 translation;
  uint32 unk0[3];
  float unk1;
};

enum class AnimationFlag : uint16 {
  Looping,
  Additive,
  PackedFrames,
};

struct Animation : CoreClass {
  static constexpr uint32 ID = 0xF000;
  uint16 animIndex;
  es::Flags<AnimationFlag> flags;
  uint16 numBones;
  uint16 numFrames;
  es::PointerX86<char> name;
  uint32 loadedTag;
  float unk4;
  float linearSpeed;
  float frameRate;
  es::PointerX86<RootMotionFrame> rootMotion;
  es::PointerX86<char> control;
  es::PointerX86<char> frames;
  uint32 null0[2];
  uint16 refPoseBufferSize;
  uint16 frameStride;
  uint16 numReferenceValues;
  uint16 num16BitTracks;
  uint16 num8bitTracks;
  uint16 unk10;
  uint32 null1;

  uint32 RefPoseValuesOffset() const {
    return numBones * 8 + GetPadding(numBones * 8, 16);
  }

  uint32 RefPoseMasksOffset() const {
    return RefPoseValuesOffset() + numReferenceValues * 2 +
           GetPadding(numReferenceValues * 2, 16);
  }

  uint32 Track16MasksOffset() const {
    return RefPoseMasksOffset() + numReferenceValues * 2 +
           GetPadding(numReferenceValues * 2, 16);
  }

  uint32 Track8MasksOffset() const {
    return Track16MasksOffset() + num16BitTracks * 2 +
           GetPadding(num16BitTracks * 2, 16);
  }

  uint32 Track8BaseValuesOffset() const {
    return Track8MasksOffset() + num8bitTracks * 2 +
           GetPadding(num8bitTracks * 2, 16);
  }

  uint32 BlendMasksOffset() const {
    return Track8BaseValuesOffset() + num8bitTracks * 2 +
           GetPadding(num8bitTracks * 2, 16);
  }

  std::span<SVector4> RefPoseRotations() {
    return {reinterpret_cast<SVector4 *>(control.Get()), numBones};
  }

  std::span<const SVector4> RefPoseRotations() const {
    return {reinterpret_cast<const SVector4 *>(control.Get()), numBones};
  }

  std::span<int16> RefPoseValues() {
    const uint32 offset = RefPoseValuesOffset();
    return {reinterpret_cast<int16 *>(control.Get() + offset),
            numReferenceValues};
  }

  std::span<const int16> RefPoseValues() const {
    const uint32 offset = RefPoseValuesOffset();
    return {reinterpret_cast<const int16 *>(control.Get() + offset),
            numReferenceValues};
  }

  std::span<TrackMask> RefPoseMasks() {
    const uint32 offset = RefPoseMasksOffset();
    return {reinterpret_cast<TrackMask *>(control.Get() + offset),
            numReferenceValues};
  }

  std::span<const TrackMask> RefPoseMasks() const {
    const uint32 offset = RefPoseMasksOffset();
    return {reinterpret_cast<const TrackMask *>(control.Get() + offset),
            numReferenceValues};
  }

  std::span<TrackMask> Track16Masks() {
    const uint32 offset = Track16MasksOffset();
    return {reinterpret_cast<TrackMask *>(control.Get() + offset),
            num16BitTracks};
  }

  std::span<const TrackMask> Track16Masks() const {
    const uint32 offset = Track16MasksOffset();
    return {reinterpret_cast<const TrackMask *>(control.Get() + offset),
            num16BitTracks};
  }

  std::span<TrackMask> Track8Masks() {
    const uint32 offset = Track8MasksOffset();
    return {reinterpret_cast<TrackMask *>(control.Get() + offset),
            num8bitTracks};
  }

  std::span<const TrackMask> Track8Masks() const {
    const uint32 offset = Track8MasksOffset();
    return {reinterpret_cast<const TrackMask *>(control.Get() + offset),
            num8bitTracks};
  }

  std::span<int16> Track8BaseValues() {
    const uint32 offset = Track8BaseValuesOffset();
    return {reinterpret_cast<int16 *>(control.Get() + offset), num8bitTracks};
  }

  std::span<const int16> Track8BaseValues() const {
    const uint32 offset = Track8BaseValuesOffset();
    return {reinterpret_cast<const int16 *>(control.Get() + offset),
            num8bitTracks};
  }

  std::span<uint8> BlendMasks() {
    const uint32 offset = BlendMasksOffset();
    return {reinterpret_cast<uint8 *>(control.Get() + offset), numBones};
  }

  std::span<const uint8> BlendMasks() const {
    const uint32 offset = BlendMasksOffset();
    return {reinterpret_cast<const uint8 *>(control.Get() + offset), numBones};
  }

  std::span<int16> Values16(uint32 frame) {
    return {reinterpret_cast<int16 *>(frames.Get() + frame * frameStride),
            num16BitTracks};
  }

  std::span<const int16> Values16(uint32 frame) const {
    return {reinterpret_cast<const int16 *>(frames.Get() + frame * frameStride),
            num16BitTracks};
  }

  std::span<int8> Values8(uint32 frame) {
    const uint32 offset =
        num16BitTracks * 2 + GetPadding(num16BitTracks * 2, 16);
    return {
        reinterpret_cast<int8 *>(frames.Get() + frame * frameStride + offset),
        num8bitTracks};
  }

  std::span<const int8> Values8(uint32 frame) const {
    const uint32 offset =
        num16BitTracks * 2 + GetPadding(num16BitTracks * 2, 16);
    return {reinterpret_cast<const int8 *>(frames.Get() + frame * frameStride +
                                           offset),
            num8bitTracks};
  }
};
