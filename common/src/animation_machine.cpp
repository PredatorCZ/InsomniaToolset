
#include "animation_machine.hpp"
#include "glm/gtc/quaternion.hpp"
#include "spike/type/matrix44.hpp"

void AnimationMachine::Reset() {
  for (auto &f : rotationFrames) {
    f.clear();
  }

  for (auto &f : translationFrames) {
    f.clear();
  }

  for (auto &f : scaleFrames) {
    f.clear();
  }
}

static Vector4A16 Multiply(Vector4A16 child, Vector4A16 parent) {
  glm::quat p_(parent.w, parent.x, parent.y, parent.z);
  glm::quat c_(child.w, child.x, child.y, child.z);
  auto res = p_ * c_;

  return Vector4A16(res.x, res.y, res.z, res.w);
}

static Vector4A16 TransformPoint(Vector4A16 q, Vector4A16 point) {
  glm::quat q_(q.w, q.x, q.y, q.z);
  glm::vec3 p_(point.x, point.y, point.z);
  auto res = q_ * p_;

  return Vector4A16(res.x, res.y, res.z, 0);
}

void AnimationMachine::BlendResultAdditive(bool preMult) {
  for (uint32 n = 0; n < maxInputNodes; n++) {
    Node &node = nodes.at(n);
    Frames &rFrames = rotationFrames.at(n);
    Frames &tFrames = translationFrames.at(n);
    Frames &sFrames = scaleFrames.at(n);

    if (preMult) {
      for (uint32 f = 0; f < numFrames; f++) {
        Vector4A16 &rf = rFrames[f];
        Vector4A16 &tf = tFrames[f];

        tf = node.refTranslation + TransformPoint(tf, tf);
        rf = Multiply(node.refRotation, rf);
      }
    } else {
      for (auto &f : rFrames) {
        f = Multiply(f, node.refRotation);
      }
      for (auto &f : tFrames) {
        f += node.refTranslation;
      }
    }

    for (auto &f : sFrames) {
      f *= node.refScale;
    }
  }
}

void AnimationMachine::ApplyRootMotion(const Vector4A16 *rotationFrames_,
                                       const Vector4A16 *translationFrames_) {
  Vector4A16 dummy;

  for (uint32 r : rootNodes) {
    Frames &rFrames = rotationFrames.at(r);
    Frames &tFrames = translationFrames.at(r);

    if (rFrames.empty()) {
      SetRotationStaticFrame(r, nodes.at(r).refRotation);
    }

    if (tFrames.empty()) {
      SetTranslationStaticFrame(r, nodes.at(r).refTranslation);
    }

    for (uint32 f = 0; f < numFrames; f++) {
      es::Matrix44 rootMtx;
      rootMtx.Compose(translationFrames_ ? translationFrames_[f] : Vector4A16{},
                      rotationFrames_ ? rotationFrames_[f]
                                      : Vector4A16{0, 0, 0, 1},
                      {1, 1, 1, 0});
      es::Matrix44 refMtx;
      refMtx.Compose(rFrames[f], rFrames[f], {1, 1, 1, 0});
      auto result = refMtx * rootMtx;
      result.Decompose(tFrames[f], rFrames[f], dummy);
    }
  }
}

static void PropagateScaleFramesTree(AnimationMachine &self, uint32 nodeId,
                                     uint32 parentNodeId) {
  AnimationMachine::Node &node = self.nodes.at(nodeId);
  AnimationMachine::Frames &parentNodeFrames =
      self.scaleFrames.at(parentNodeId);
  AnimationMachine::Frames &nodeFrames = self.scaleFrames.at(nodeId);

  if (!node.inheritScale) {
    return;
  }

  for (uint32 c : node.children) {
    PropagateScaleFramesTree(self, c, parentNodeId);
  }

  if (!parentNodeFrames.empty()) {
    if (nodeFrames.empty()) {
      nodeFrames.assign(self.numFrames, node.refScale);
    }

    for (uint32 f = 0; f < self.numFrames; f++) {
      nodeFrames[f] *= parentNodeFrames[f];
    }

    if (node.scaleTranslations) {
      AnimationMachine::Frames &nodeTranslationFrames =
          self.translationFrames.at(nodeId);

      if (nodeTranslationFrames.empty()) {
        nodeTranslationFrames.assign(self.numFrames, node.refTranslation);
      }

      for (uint32 f = 0; f < self.numFrames; f++) {
        nodeTranslationFrames[f] *= parentNodeFrames[f];
      }
    }
  }
}

static void PropagateScaleFrames(AnimationMachine &self, uint32 nodeId,
                                 uint32 parentNodeId) {
  AnimationMachine::Node &node = self.nodes.at(nodeId);
  if (!node.inheritScale) {
    return;
  }

  for (uint32 c : node.children) {
    PropagateScaleFrames(self, c, nodeId);
  }

  PropagateScaleFramesTree(self, nodeId, parentNodeId);
}

void AnimationMachine::PropagateScaleFrames() {
  for (uint32 r : rootNodes) {
    Node &node = nodes.at(r);

    for (uint32 c : node.children) {
      ::PropagateScaleFrames(*this, c, r);
    }
  }
}
