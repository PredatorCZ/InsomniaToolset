#pragma once
#include "insomnia/insomnia.hpp"
#include "spike/gltf.hpp"

struct AppContextStream;
struct AppContext;

struct IMGLTF : GLTFModel {
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

void RegionToGltf(IMGLTF &main, IGHW &ighw,
                  IGHWTOCIteratorConst<ResourceShaders> &shaders,
                  AppContextStream &shdStream,
                  IGHWTOCIteratorConst<ResourceTies> ties,
                  IGHWTOCIteratorConst<ResourceShrubs> shrubs,
                  IGHWTOCIteratorConst<ResourceFoliages> foliages,
                  AppContext *ctx, const std::string &workDir);
void GenerateInstances(IMGLTF &main);
