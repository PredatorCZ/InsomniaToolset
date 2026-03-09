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

  std::map<float, uint32> timesAccId;
  std::map<float, uint16> maxFrames;
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
                              const Skeleton *skel);
void IS_EXTERN LoadAnimations(GLTFAni &glMain,
                              const es::PointerX86<Animation> *anims,
                              const uint32 numAnimations, const Skeleton *skel);
void IS_EXTERN Instantiate(IMGLTF &main, gltf::Node &glNode,
                           std::vector<es::Matrix44> &tms);

inline void DecodeColor(GLTFModel &level, gltf::Primitive &glPrim,
                        auto &vtxContainer) {
  std::vector<UCVector4> color;

  for (auto &v : vtxContainer) {
    float posw = abs(v.purpose);
    float cl0 = std::min(posw / 0x4000, 1.f);
    float cl1 = posw / 0x80;
    cl1 = cl1 - std::floor(cl1);

    color.emplace_back((Vector4{cl0, cl0, cl0, cl1} * 255).Convert<uint8>());
  }

  glPrim.attributes["COLOR_0"] =
      level.SaveVertices(color.data(), color.size(),
                         {
                             .type = uni::DataType::R8G8B8A8,
                             .format = uni::FormatType::UNORM,
                             .usage = AttributeType::VertexColor,
                         });
}
