#pragma once
#include "insomnia.hpp"
#include "spike/gltf.hpp"

struct AppContextStream;
struct AppContext;

struct GLTFAni : GLTFModel {
  using GLTFModel::GLTFModel;

  GLTFStream &AnimStream() {
    if (aniStream < 0) {
      auto &newStream = NewStream("anims");
      aniStream = newStream.slot;
      return newStream;
    }
    return Stream(aniStream);
  }

  uint32 timesAccId;
  uint16 maxFrames = 0;
  int32 staticTimes = -1;

private:
  int32 aniStream = -1;
};

struct IMGLTF : GLTFAni {
  GLTFStream &GetTranslations() {
    if (instTrs < 0) {
      auto &str = NewStream("instance-tms", 20);
      instTrs = str.slot;
      return str;
    }

    return Stream(instTrs);
  }

  GLTFStream &GetScales() {
    if (instScs < 0) {
      auto &str = NewStream("instance-scale");
      instScs = str.slot;
      return str;
    }

    return Stream(instScs);
  }

  struct NodeInstances {
    int32 nodeIndex = -1;
    std::vector<es::Matrix44> tms;
  };

  std::map<Hash, uint32> materialRemaps;
  std::map<Hash, NodeInstances> ties;
  std::map<Hash, NodeInstances> shrubs;
  std::map<Hash, NodeInstances> foliages;

private:
  int32 instTrs = -1;
  int32 instScs = -1;
};

void IS_EXTERN RegionToGltf(IMGLTF &main, IGHW &ighw,
                            IGHWTOCIteratorConst<ResourceShaders> &shaders,
                            AppContextStream &shdStream,
                            IGHWTOCIteratorConst<ResourceTies> ties,
                            IGHWTOCIteratorConst<ResourceShrubs> shrubs,
                            IGHWTOCIteratorConst<ResourceFoliages> foliages,
                            AppContext *ctx, const std::string &workDir);
void IS_EXTERN GenerateInstances(IMGLTF &main);
void IS_EXTERN LoadAnimations(GLTFAni &glMain,
                              IGHWTOCIteratorConst<Animation> animations,
                              const int translationShift);
void IS_EXTERN LoadAnimations(GLTFAni &glMain,
                              const es::PointerX86<Animation> *anims,
                              const uint32 numAnimations, const Skeleton *skel);
void IS_EXTERN Instantiate(IMGLTF &main, gltf::Node &glNode,
                           std::vector<es::Matrix44> &tms);
