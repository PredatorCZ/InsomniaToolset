#pragma once
#include "spike/uni/rts.hpp"
#include <cstring>
#include <vector>

struct AnimationMachine {
  using Frames = std::vector<Vector4A16>;
  struct Node {
    bool inheritScale : 1 = false;
    bool scaleTranslations : 1 = true;
    int32 parent;
    uint32 id;
    std::vector<uint32> children;
    Vector4A16 refRotation{0, 0, 0, 1};
    Vector4A16 refTranslation{0, 0, 0, 1};
    Vector4A16 refScale{1, 1, 1, 0};
  };

  void Setup(uint32 numNodes, uint32 numFrames_) {
    nodes.resize(numNodes);
    rotationFrames.resize(numNodes);
    translationFrames.resize(numNodes);
    scaleFrames.resize(numNodes);
    maxInputNodes = numNodes;
    numFrames = numFrames_;
  }

  void AddNode(int32 parent, uint32 id) {
    Node &node = nodes.at(id);
    node.id = id;
    node.parent = parent;
    if (parent < 0) {
      rootNodes.push_back(id);
    } else {
      nodes.at(parent).children.push_back(id);
    }
  }

  void NodeRefPose(uint32 nodeId, const float *rotation,
                   const float *translation, const float *scale) {
    Node &node = nodes.at(nodeId);
    memcpy((void *)&node.refRotation, rotation, 16);
    memcpy((void *)&node.refTranslation, translation, 12);
    memcpy((void *)&node.refScale, scale, 12);
  }

  void NodeRefPose(uint32 nodeId, float *rotation, float *translation) {
    Node &node = nodes.at(nodeId);
    memcpy((void *)&node.refRotation, rotation, 16);
    memcpy((void *)&node.refTranslation, translation, 12);
  }

  void NodeRefPose(uint32 nodeId, const Vector4A16 &rotation,
                   const Vector4A16 &translation, const Vector4A16 &scale) {
    Node &node = nodes.at(nodeId);
    node.refRotation = rotation;
    node.refTranslation = translation;
    node.refScale = scale;
  }

  void NodeRefPose(uint32 nodeId, const Vector4A16 &rotation,
                   const Vector4A16 &translation) {
    Node &node = nodes.at(nodeId);
    node.refRotation = rotation;
    node.refTranslation = translation;
  }

  void SetRotationFrames(uint32 nodeId, const Vector4A16 *frames) {
    rotationFrames.at(nodeId).assign(frames, frames + numFrames);
  }

  void SetScaleFrames(uint32 nodeId, const Vector4A16 *frames) {
    scaleFrames.at(nodeId).assign(frames, frames + numFrames);
  }

  void SetTranslationFrames(uint32 nodeId, const Vector4A16 *frames) {
    translationFrames.at(nodeId).assign(frames, frames + numFrames);
  }

  void SetFrames(uint32 nodeId, const uni::RTSValue *frames) {
    Frames &rFrames = rotationFrames.at(nodeId);
    Frames &tFrames = translationFrames.at(nodeId);
    Frames &sFrames = scaleFrames.at(nodeId);
    rFrames.resize(numFrames);
    tFrames.resize(numFrames);
    sFrames.resize(numFrames);

    for (uint32 i = 0; i < numFrames; i++) {
      rFrames[i] = frames[i].rotation;
      tFrames[i] = frames[i].translation;
      sFrames[i] = frames[i].scale;
    }
  }

  void SetRotationStaticFrame(uint32 nodeId, const Vector4A16 &frame) {
    rotationFrames.at(nodeId).assign(numFrames, frame);
  }

  void SetScaleStaticFrame(uint32 nodeId, const Vector4A16 &frame) {
    scaleFrames.at(nodeId).assign(numFrames, frame);
  }

  void SetTranslationStaticFrame(uint32 nodeId, const Vector4A16 &frame) {
    translationFrames.at(nodeId).assign(numFrames, frame);
  }

  void SetStaticFrame(uint32 nodeId, const uni::RTSValue &frame) {
    auto &rFrames = rotationFrames.at(nodeId);
    auto &tFrames = translationFrames.at(nodeId);
    auto &sFrames = scaleFrames.at(nodeId);

    rFrames.assign(numFrames, frame.rotation);
    tFrames.assign(numFrames, frame.translation);
    sFrames.assign(numFrames, frame.scale);
  }

  void Reset();
  void BlendResultAdditive(bool preMult = false);
  void ApplyRootMotion(const Vector4A16 *rotationFrames = nullptr,
                       const Vector4A16 *translationFrames = nullptr);
  void PropagateScaleFrames();

  uint32 maxInputNodes = 0;
  uint32 numFrames = 0;
  std::vector<Node> nodes;
  std::vector<uint32> rootNodes;
  std::vector<Frames> rotationFrames;
  std::vector<Frames> translationFrames;
  std::vector<Frames> scaleFrames;
};
