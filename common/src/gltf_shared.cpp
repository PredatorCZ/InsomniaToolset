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

#include "insomnia/gltf.hpp"
#include "nlohmann/json.hpp"
#include <cassert>
#include <set>

void LoadRootMotion(GLTFAni &glMain, const Animation &a, int32 timesAcc,
                    gltf::Animation &glAnim) {
  bool useRotation = false;
  bool useScale = false;
  bool useTranslation = false;

  for (uint16 f = 0; f < a.numFrames; f++) {
    const RootMotionFrame &frame = a.rootMotion[f];

    if (frame.rotation != Vector4A16{0, 0, 0, 1}) {
      useRotation = true;
    }

    if (frame.scale != Vector4A16{1, 1, 1, 0}) {
      useScale = true;
    }
    if (frame.translation != Vector4A16{}) {
      useTranslation = true;
    }
  }

  auto &str = glMain.AnimStream();

  if (useRotation) {
    auto [acc, accId] = glMain.NewAccessor(str, 2);
    acc.type = gltf::Accessor::Type::Vec4;
    acc.componentType = gltf::Accessor::ComponentType::Short;
    acc.normalized = true;
    acc.count = a.numFrames;

    auto &channel = glAnim.channels.emplace_back();
    channel.target.node = 0;
    channel.target.path = "rotation";
    channel.sampler = glAnim.samplers.size();

    auto &sampler = glAnim.samplers.emplace_back();
    sampler.output = accId;
    sampler.input = timesAcc;

    for (uint16 f = 0; f < a.numFrames; f++) {
      Vector4A16 value(a.rootMotion[f].rotation);
      value *= (1.f / 0x7fff);
      value.Normalize();
      value *= 0x7fff;
      value = Vector4A16(_mm_round_ps(value._data, _MM_ROUND_NEAREST));
      str.wr.Write(value.Convert<int16>());
    }
  }

  if (useScale) {
    auto [acc, accId] = glMain.NewAccessor(str, 4);
    acc.type = gltf::Accessor::Type::Vec3;
    acc.componentType = gltf::Accessor::ComponentType::Float;
    acc.count = a.numFrames;

    auto &channel = glAnim.channels.emplace_back();
    channel.target.node = 0;
    channel.target.path = "scale";
    channel.sampler = glAnim.samplers.size();

    auto &sampler = glAnim.samplers.emplace_back();
    sampler.output = accId;
    sampler.input = timesAcc;

    for (uint16 f = 0; f < a.numFrames; f++) {
      str.wr.Write<Vector>(a.rootMotion[f].scale);
    }
  }

  if (useTranslation) {
    auto [acc, accId] = glMain.NewAccessor(str, 4);
    acc.type = gltf::Accessor::Type::Vec3;
    acc.componentType = gltf::Accessor::ComponentType::Float;
    acc.count = a.numFrames;

    auto &channel = glAnim.channels.emplace_back();
    channel.target.node = 0;
    channel.target.path = "translation";
    channel.sampler = glAnim.samplers.size();

    auto &sampler = glAnim.samplers.emplace_back();
    sampler.output = accId;
    sampler.input = timesAcc;

    for (uint16 f = 0; f < a.numFrames; f++) {
      str.wr.Write<Vector>(Vector4A16(a.rootMotion[f].translation * YARD_TO_M));
    }
  }
}

void LoadAnimation(GLTFAni &glMain, const Animation &a,
                   const float positionScale) {
  std::map<uint16, std::vector<SVector4>> positions;
  std::map<uint16, std::vector<SVector4>> rotations;
  std::map<uint16, SVector4> positionsStatic;

  for (int32 i = -1; auto &m : a.RefPoseMasks()) {
    assert(m.unk == 2);
    i++;
    if (m.type == TrackType::Position) {
      positionsStatic[m.boneIndex][m.component] = a.RefPoseValues()[i];
      positionsStatic[m.boneIndex].w |= 1 << m.component;
    }
  }

  for (auto &m : a.Track16Masks()) {
    assert(m.unk == 2);
    if (m.type == TrackType::Rotation) {
      if (rotations[m.boneIndex].empty()) {
        rotations[m.boneIndex].resize(a.numFrames,
                                      a.RefPoseRotations()[m.boneIndex]);
      }
    } else if (m.type == TrackType::Position) {
      if (positions[m.boneIndex].empty()) {
        if (positionsStatic.contains(m.boneIndex)) {
          positions[m.boneIndex].resize(a.numFrames,
                                        positionsStatic.at(m.boneIndex));
        } else {
          positions[m.boneIndex].resize(a.numFrames);
        }
      }
    }
  }

  for (auto &m : a.Track8Masks()) {
    assert(m.unk == 2);
    if (m.type == TrackType::Rotation) {
      if (rotations[m.boneIndex].empty()) {
        rotations[m.boneIndex].resize(a.numFrames,
                                      a.RefPoseRotations()[m.boneIndex]);
      }
    } else if (m.type == TrackType::Position) {
      if (positions[m.boneIndex].empty()) {
        positions[m.boneIndex].resize(a.numFrames);
      }
    }
  }

  {
    auto masks = a.Track16Masks();

    for (uint32 f = 0; f < a.numFrames; f++) {
      auto values = a.Values16(f);
      for (uint32 curNode = 0; int16 v : values) {
        TrackMask m = masks[curNode];
        if (m.type == TrackType::Position) {
          positions[m.boneIndex].at(f)[m.component] = v;
        } else if (m.type == TrackType::Rotation) {
          rotations[m.boneIndex].at(f)[m.component] = v;
        }

        curNode++;
      }
    }
  }

  {
    auto masks = a.Track8Masks();
    auto baseValues = a.Track8BaseValues();

    for (uint32 f = 0; f < a.numFrames; f++) {
      auto values = a.Values8(f);
      for (uint32 curNode = 0; int16 v : values) {
        TrackMask m = masks[curNode];
        int16 value = baseValues[curNode] + v;
        if (m.type == TrackType::Position) {
          positions[m.boneIndex].at(f)[m.component] = value;
          positions[m.boneIndex].at(f).w |= 1 << m.component;
        } else if (m.type == TrackType::Rotation) {
          rotations[m.boneIndex].at(f)[m.component] = value;
        }

        curNode++;
      }
    }
  }

  uint32 curTimesAccId = glMain.timesAccId[a.frameRate];

  if (a.numFrames != glMain.maxFrames[a.frameRate]) {
    curTimesAccId = glMain.accessors.size();
    auto &nacc = glMain.accessors.emplace_back(
        glMain.accessors.at(glMain.timesAccId[a.frameRate]));
    nacc.count = a.numFrames;
    nacc.max.back() = a.numFrames > 0 ? (1.f / 30) * (a.numFrames - 1) : 0;
  }

  gltf::Animation &glAnim = glMain.animations.emplace_back();
  glAnim.name = a.name.Get();
  auto &str = glMain.AnimStream();

  for (const auto &[b, r] : positions) {
    auto [acc, accId] = glMain.NewAccessor(str, 4);
    acc.type = gltf::Accessor::Type::Vec3;
    acc.componentType = gltf::Accessor::ComponentType::Float;
    acc.count = a.numFrames;

    auto &channel = glAnim.channels.emplace_back();
    channel.target.node = b;
    channel.target.path = "translation";
    channel.sampler = glAnim.samplers.size();

    auto &sampler = glAnim.samplers.emplace_back();
    sampler.output = accId;
    sampler.input = curTimesAccId;

    auto &refPos = glMain.nodes.at(b).translation;

    for (auto &v : r) {
      Vector4A16 value(v.Convert<float>());
      value *= positionScale * YARD_TO_M;

      for (uint8 i = 0; i < 3; i++) {
        if (!(v.w & (1 << i))) {
          value[i] = refPos[i];
        }
      }

      str.wr.Write<Vector>(value);
    }
  }

  for (const auto &[b, r] : rotations) {
    auto [acc, accId] = glMain.NewAccessor(str, 2);
    acc.type = gltf::Accessor::Type::Vec4;
    acc.componentType = gltf::Accessor::ComponentType::Short;
    acc.normalized = true;
    acc.count = a.numFrames;

    auto &channel = glAnim.channels.emplace_back();
    channel.target.node = b;
    channel.target.path = "rotation";
    channel.sampler = glAnim.samplers.size();

    auto &sampler = glAnim.samplers.emplace_back();
    sampler.output = accId;
    sampler.input = curTimesAccId;

    for (auto &v : r) {
      Vector4A16 value(v.Convert<float>());
      value *= (1.f / 0x7fff);
      value.Normalize();
      value *= 0x7fff;
      value = Vector4A16(_mm_round_ps(value._data, _MM_ROUND_NEAREST));
      str.wr.Write(value.Convert<int16>());
    }
  }

  auto blendMasks = a.BlendMasks();

  for (uint32 i = 0; i < a.numBones; i++) {
    if (rotations.contains(i)) {
      continue;
    }

    if (a.flags[AnimationFlag::Additive] && blendMasks[i] == 0) {
      continue;
    }

    if (glMain.staticTimes < 0) {
      glMain.staticTimes = glMain.accessors.size();
      auto &nacc = glMain.accessors.emplace_back(
          glMain.accessors.at(glMain.timesAccId[a.frameRate]));
      nacc.count = 1;
      nacc.max.back() = 0;
    }

    auto [acc, accId] = glMain.NewAccessor(str, 2);
    acc.type = gltf::Accessor::Type::Vec4;
    acc.componentType = gltf::Accessor::ComponentType::Short;
    acc.normalized = true;
    acc.count = 1;

    auto &channel = glAnim.channels.emplace_back();
    channel.target.node = i;
    channel.target.path = "rotation";
    channel.sampler = glAnim.samplers.size();

    auto &sampler = glAnim.samplers.emplace_back();
    sampler.output = accId;
    sampler.input = glMain.staticTimes;

    Vector4A16 value(a.RefPoseRotations()[i].Convert<float>());
    value *= (1.f / 0x7fff);
    value.Normalize();
    value *= 0x7fff;
    value = Vector4A16(_mm_round_ps(value._data, _MM_ROUND_NEAREST));
    str.wr.Write(value.Convert<int16>());
  }

  for (auto &[b, p] : positionsStatic) {
    if (positions.contains(b)) {
      continue;
    }

    if (glMain.staticTimes < 0) {
      glMain.staticTimes = glMain.accessors.size();

      auto &nacc = glMain.accessors.emplace_back(
          glMain.accessors.at(glMain.timesAccId[a.frameRate]));
      nacc.count = 1;
      nacc.max.back() = 0;
    }

    auto [acc, accId] = glMain.NewAccessor(str, 4);
    acc.type = gltf::Accessor::Type::Vec3;
    acc.componentType = gltf::Accessor::ComponentType::Float;
    acc.count = 1;

    auto &channel = glAnim.channels.emplace_back();
    channel.target.node = b;
    channel.target.path = "translation";
    channel.sampler = glAnim.samplers.size();

    auto &sampler = glAnim.samplers.emplace_back();
    sampler.output = accId;
    sampler.input = glMain.staticTimes;

    auto &refPos = glMain.nodes.at(b).translation;
    Vector4A16 value(p.Convert<float>());
    value *= positionScale * YARD_TO_M;

    for (uint8 i = 0; i < 3; i++) {
      if (!(p.w & (1 << i))) {
        value[i] = refPos[i];
      }
    }

    str.wr.Write<Vector>(value);
  }

  if (a.rootMotion) {
    LoadRootMotion(glMain, a, curTimesAccId, glAnim);
  }
}

void SwapAnimBuffer(Animation &item) {
  if (item.loadedTag == 0xDEADBEEF) {
    return;
  }

  item.loadedTag = 0xDEADBEEF;
  for (auto &q : item.RefPoseRotations()) {
    FByteswapper(static_cast<SVector4 &>(q));
  }

  for (int16 &q : item.RefPoseValues()) {
    FByteswapper(q);
  }

  for (TrackMask &q : item.RefPoseMasks()) {
    FByteswapper(q);
  }

  for (TrackMask &q : item.Track16Masks()) {
    FByteswapper(q);
  }

  for (TrackMask &q : item.Track8Masks()) {
    FByteswapper(q);
  }

  for (int16 &q : item.Track8BaseValues()) {
    FByteswapper(q);
  }

  for (uint32 f = 0; f < item.numFrames; f++) {
    for (int16 &q : item.Values16(f)) {
      FByteswapper(q);
    }
  }
}

void MakeFrames(GLTFAni &glMain) {
  auto &str = glMain.AnimStream();

  for (auto [frameRate, maxFrames] : glMain.maxFrames) {
    auto [timesAcc, timesAccId] = glMain.NewAccessor(str, 4);
    timesAcc.type = gltf::Accessor::Type::Scalar;
    timesAcc.componentType = gltf::Accessor::ComponentType::Float;
    timesAcc.count = maxFrames;
    timesAcc.min.emplace_back(0);
    glMain.timesAccId[frameRate] = timesAccId;

    if (maxFrames < 2) {
      str.wr.Write<float>(0);
      timesAcc.max.emplace_back(0);
    } else {
      auto times =
          gltfutils::MakeSamples(frameRate, (maxFrames - 1) / frameRate);
      str.wr.WriteContainer(times);
      timesAcc.max.emplace_back(times.back());
    }
  }
}

void LoadAnimations(GLTFAni &glMain, const es::PointerX86<Animation> *anims,
                    const uint32 numAnimations, const Skeleton *skel) {
  for (uint32 i = 0; i < numAnimations; i++) {
    glMain.maxFrames[anims[i]->frameRate] =
        std::max(anims[i]->numFrames, glMain.maxFrames[anims[i]->frameRate]);
  }

  MakeFrames(glMain);

  int32 maxBones = -1;

  for (uint32 i = 0; i < numAnimations; i++) {
    const Animation &a = *anims[i];
    if (maxBones < 0 && !a.flags[AnimationFlag::Additive]) {
      maxBones = a.numBones;
    } else if (!a.flags[AnimationFlag::Additive]) {
      assert(maxBones == a.numBones);
    }
  }

  if (maxBones < 0) {
    maxBones = skel->numBones;
  } else {
    assert(maxBones == skel->numBones);
  }

  for (uint32 i = 0; i < numAnimations; i++) {
    const Animation &a = *anims[i];
    if (a.flags[AnimationFlag::Additive]) {
      const_cast<Animation &>(a).numBones = maxBones;
    }

    SwapAnimBuffer(const_cast<Animation &>(a));
  }

  std::set<std::string> usedAnims;
  const float positionScale = 1.f / (0x7fff >> skel->translationShift);

  for (uint32 i = 0; i < numAnimations; i++) {
    const Animation &a = *anims[i];
    if (!usedAnims.contains(a.name.Get())) {
      LoadAnimation(glMain, a, positionScale);
      usedAnims.emplace(a.name.Get());
    }
  }
}

void LoadAnimations(GLTFAni &glMain, IGHWTOCIteratorConst<Animation> animations,
                    const int translationShift) {
  for (auto &a : animations) {
    glMain.maxFrames[a.frameRate] =
        std::max(a.numFrames, glMain.maxFrames[a.frameRate]);
  }

  MakeFrames(glMain);

  int32 maxBones = -1;

  for (auto &a : animations) {
    if (maxBones < 0 && !a.flags[AnimationFlag::Additive]) {
      maxBones = a.numBones;
    } else if (!a.flags[AnimationFlag::Additive]) {
      assert(maxBones == a.numBones);
    }
  }

  for (auto &a : animations) {
    if (a.flags[AnimationFlag::Additive]) {
      assert(maxBones >= 0);
      const_cast<Animation &>(a).numBones = maxBones;
    }

    SwapAnimBuffer(const_cast<Animation &>(a));
  }

  // Guessed magic number
  const float positionScale = 1.f / (0x7500 >> translationShift);

  for (auto &a : animations) {
    LoadAnimation(glMain, a, positionScale);
  }
}

void Instantiate(IMGLTF &main, gltf::Node &glNode,
                 std::vector<es::Matrix44> &tms) {
  if (tms.size() == 1) {
    Vector4A16 rotation, translation, scale;
    tms.front().Decompose(translation, rotation, scale);
    memcpy(glNode.rotation.data(), &rotation, 16);
    memcpy(glNode.translation.data(), &translation, 12);
    memcpy(glNode.scale.data(), &scale, 12);
  } else if (tms.size() > 1) {
    std::vector<Vector> scales;
    bool processScales = false;

    auto &str = main.GetTranslations();
    auto [accPos, accPosIndex] = main.NewAccessor(str, 4);
    accPos.type = gltf::Accessor::Type::Vec3;
    accPos.componentType = gltf::Accessor::ComponentType::Float;
    accPos.count = tms.size();

    auto [accRot, accRotIndex] = main.NewAccessor(str, 4, 12);
    accRot.type = gltf::Accessor::Type::Vec4;
    accRot.componentType = gltf::Accessor::ComponentType::Short;
    accRot.normalized = true;
    accRot.count = tms.size();
    Vector4A16::SetEpsilon(0.00001f);

    for (const es::Matrix44 &mtx : tms) {
      Vector4A16 rotation, translation, scale;
      mtx.Decompose(translation, rotation, scale);
      scales.emplace_back(scale);

      if (!processScales) {
        processScales = scale != Vector4A16(1, 1, 1, 0);
      }

      str.wr.Write<Vector>(translation);

      rotation.Normalize() *= 0x7fff;
      rotation = Vector4A16(_mm_round_ps(rotation._data, _MM_ROUND_NEAREST));
      auto comp = rotation.Convert<int16>();
      str.wr.Write(comp);
    }

    auto &attrs =
        glNode.GetExtensionsAndExtras()["extensions"]["EXT_mesh_gpu_instancing"]
                                       ["attributes"];

    attrs["TRANSLATION"] = accPosIndex;
    attrs["ROTATION"] = accRotIndex;

    if (processScales) {
      auto &str = main.GetScales();
      auto [accScale, accScaleIndex] = main.NewAccessor(str, 4);
      accScale.type = gltf::Accessor::Type::Vec3;
      accScale.componentType = gltf::Accessor::ComponentType::Float;
      accScale.count = tms.size();
      str.wr.WriteContainer(scales);
      attrs["SCALE"] = accScaleIndex;
    }
  }
}
