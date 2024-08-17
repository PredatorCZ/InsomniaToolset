/*  InsomniaToolset LevelmainToGLTF
    Copyright(C) 2024 Lukas Cone

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

#include "insomnia/insomnia.hpp"
#include "insomnia/internal/vertex.hpp"
#include "nlohmann/json.hpp"
#include "project.h"
#include "spike/app_context.hpp"
#include "spike/except.hpp"
#include "spike/gltf.hpp"
#include "spike/io/binreader_stream.hpp"
#include "spike/master_printer.hpp"
#include "spike/reflect/reflector.hpp"
#include "spike/type/float.hpp"
#include "spike/uni/rts.hpp"
#include <set>

std::string_view filters[]{
    "^ps3levelmain.dat$",
};

static AppInfo_s appInfo{
    .header = LevelmainToGLTF_DESC " v" LevelmainToGLTF_VERSION
                                   ", " LevelmainToGLTF_COPYRIGHT "Lukas Cone",
    .filters = filters,
};

AppInfo_s *AppInitModule() { return &appInfo; }

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

private:
  int32 instTrs = -1;
  int32 instScs = -1;
};

struct TextureKey {
  const Texture *tex;
  int32 id = -1;
  bool albedo : 1 = false;
  bool normal : 1 = false;
  bool gloss : 1 = false;
  bool specular : 1 = false;
  bool emissive : 1 = false;
  bool exponent : 1 = false;

  bool operator<(const TextureKey &o) const {
    if (o.emissive == emissive) {
      return tex < o.tex;
    }

    return emissive < o.emissive;
  }
};

struct TexStream : TexelOutput {
  GLTF &main;
  GLTFStream *str = nullptr;
  bool ignore = false;

  TexStream(GLTF &main_) : main{main_} {}

  void SendData(std::string_view data) override {
    if (str) [[likely]] {
      str->wr.WriteContainer(data);
    }
  }
  void NewFile(std::string path) override {
    if (ignore) {
      return;
    }

    str = &main.NewStream(path);
  }
};

void ExtractTexture(AppContext *ctx, std::string path,
                    std::istream &textureStream, TextureKey &info,
                    TexStream *texOut = nullptr) {
  thread_local static std::string tmpBuffer(4 * 2048 * 2048, 0);

  textureStream.clear();
  textureStream.seekg(info.tex->offset);
  textureStream.read(tmpBuffer.data(), tmpBuffer.size());

  TexelTile tile = TexelTile::Linear;

  auto GetFormat = [&] {
    using T = TextureFormat;
    switch (info.tex->format) {
    case T::RGBA8:
      tile = TexelTile::Morton;
      return TexelInputFormatType::RGBA8;
    case T::R8:
      tile = TexelTile::Morton;
      return TexelInputFormatType::R8;
    case T::BC1:
      return TexelInputFormatType::BC1;
    case T::BC3:
      return TexelInputFormatType::BC3;
    case T::BC2:
      return TexelInputFormatType::BC2;
    case T::R5G6B5:
      tile = TexelTile::Morton;
      return TexelInputFormatType::R5G6B5;
    case T::RGBA4:
      tile = TexelTile::Morton;
      return TexelInputFormatType::RGBA4;
    case T::RG8:
      tile = TexelTile::Morton;
      return TexelInputFormatType::RG8;
    default:
      PrintError("Invalid texture format: " +
                 std::to_string(int(info.tex->format)));
      break;
    }

    return TexelInputFormatType::INVALID;
  };

  TexelSwizzle swizzle;

  if (info.normal) {
    swizzle.a = TexelSwizzleType::White;
    swizzle.r = TexelSwizzleType::Alpha;
    swizzle.g = TexelSwizzleType::GreenInverted;
    swizzle.b = TexelSwizzleType::DeriveZ;
  } else if (info.gloss) {
    swizzle.r = swizzle.g = swizzle.b = TexelSwizzleType::RedInverted;

    if (info.specular) {
      swizzle.a = TexelSwizzleType::Green;
    } else {
      swizzle.a = TexelSwizzleType::White;
    }
  } else if (info.emissive) {
    swizzle.r = swizzle.g = swizzle.b = TexelSwizzleType::Blue;
    swizzle.a = TexelSwizzleType::White;
  }

  NewTexelContextCreate tctx{
      .width = info.tex->width,
      .height = info.tex->height,
      .baseFormat =
          {
              .type = GetFormat(),
              .swizzle = swizzle,
              .tile = tile,
              .swapPacked = true,
          },
      .depth = std::max(
          uint16(1), uint16(info.tex->control3.Get<TextureControl3::depth>())),
      .numMipmaps = uint8(info.tex->numMips),
      .data = tmpBuffer.data(),
      .texelOutput = texOut,
      .formatOverride =
          texOut ? TexelContextFormat::UPNG : TexelContextFormat::Config,
  };

  auto EmissiveCheck = [texOut](char *data, uint32 stride, uint32 numTexels) {
    texOut->ignore = true;
    for (uint32 i = 0; i < numTexels; i++, data += stride) {
      if (*data) {
        texOut->ignore = false;
        return;
      }
    }
  };

  if (info.emissive) {
    tctx.postProcess = EmissiveCheck;
  }

  if (texOut) {
    ctx->NewImage(tctx);
  } else {
    ctx->ExtractContext()->NewImage(path + std::to_string(info.id), tctx);
  }
}

int32 TryExtractTexture(AppContext *ctx, GLTF &main, TextureKey key,
                        const Texture *textures, std::istream &textureStream,
                        std::set<TextureKey> &textureRemaps) {
  if (auto found = textureRemaps.find(key); found != textureRemaps.end()) {
    return found->id;
  }

  gltf::Texture glTexture{};
  glTexture.source = main.textures.size();
  gltf::Image glImage{};
  glImage.mimeType = "image/png";
  glImage.name = "texture_" + std::to_string(std::distance(textures, key.tex));

  if (key.emissive) {
    glImage.name.append("_e");
  }

  TexStream tStr{main};

  ExtractTexture(ctx, glImage.name, textureStream, key, &tStr);

  if (!tStr.str) {
    key.id = -1;
    textureRemaps.emplace(key);
    return -1;
  }

  key.id = glTexture.source;
  textureRemaps.emplace(key);
  glImage.bufferView = tStr.str->slot;
  main.textures.emplace_back(glTexture);
  main.images.emplace_back(glImage);
  return glTexture.source;
}

void MakeMaterials(AppContext *ctx, IMGLTF &main,
                   const std::map<uint16, uint16> &materialRemaps,
                   IGHWTOCIteratorConst<MaterialV1> materials,
                   const Texture *textures, std::istream &textureStream,
                   std::set<TextureKey> &textureRemaps) {
  main.materials.resize(materialRemaps.size());

  for (auto [mid, mindex] : materialRemaps) {
    gltf::Material &glMat = main.materials.at(mindex);
    glMat.name = "material_" + std::to_string(mid);
    glMat.pbrMetallicRoughness.metallicFactor = 0;
    const MaterialV1 &mat = materials.at(mid);
    const Texture *albedo = mat.textures[0];
    TextureKey albedoInfo{
        .tex = albedo,
        .albedo = true,
    };
    glMat.pbrMetallicRoughness.baseColorTexture.index = TryExtractTexture(
        ctx, main, albedoInfo, textures, textureStream, textureRemaps);

    if (mat.blendMode == 4) {
      glMat.alphaMode = gltf::Material::AlphaMode::Mask;
    } else if (mat.blendMode) {
      glMat.alphaMode = gltf::Material::AlphaMode::Blend;
    }

    const Texture *normal = mat.textures[1];

    if (normal) {
      if (!mat.useNormalMap) {
        PrintWarning("Material ", mid, " uses normal but no flags");
      }
      TextureKey normalInfo{
          .tex = normal,
          .normal = true,
      };

      glMat.normalTexture.index = TryExtractTexture(
          ctx, main, normalInfo, textures, textureStream, textureRemaps);
    }

    const Texture *special = mat.textures[2];

    if (special) {
      if (!mat.useGlossiness && !mat.useSpecular) {
        PrintWarning("Material ", mid, " uses special but no flags");
      } else {
        TextureKey specInfo{
            .tex = special,
            .gloss = mat.useGlossiness,
            .specular = mat.useSpecular,
        };

        int32 specId = TryExtractTexture(ctx, main, specInfo, textures,
                                         textureStream, textureRemaps);

        if (mat.useSpecular) {
          nlohmann::json &spec =
              glMat.GetExtensionsAndExtras()["extensions"]
                                            ["KHR_materials_specular"];
          spec["specularTexture"]["index"] = specId;
        }

        if (mat.useGlossiness) {
          glMat.pbrMetallicRoughness.metallicRoughnessTexture.index = specId;
        }
      }

      TextureKey emisKey{
          .tex = special,
          .emissive = true,
      };

      glMat.emissiveTexture.index = TryExtractTexture(
          ctx, main, emisKey, textures, textureStream, textureRemaps);
    }
  }
}

void MakeFoliageMaterials(AppContext *ctx, IMGLTF &main,
                          const std::map<uint16, uint16> &materialRemaps,
                          const Texture *textures, std::istream &textureStream,
                          std::set<TextureKey> &textureRemaps) {
  main.materials.resize(materialRemaps.size() + main.materials.size());

  for (auto [mid, mindex] : materialRemaps) {
    gltf::Material &glMat = main.materials.at(mindex);
    glMat.name = "foliage_material_" + std::to_string(mid);
    glMat.doubleSided = true;
    const Texture &albedo = textures[mid];
    TextureKey albedoInfo{
        .tex = &albedo,
        .albedo = true,
    };
    glMat.pbrMetallicRoughness.baseColorTexture.index = TryExtractTexture(
        ctx, main, albedoInfo, textures, textureStream, textureRemaps);
    glMat.alphaMode = gltf::Material::AlphaMode::Mask;
  }
}

struct AttributeMul : AttributeCodec {
  AttributeMul(Vector scale) : mul(scale * YARD_TO_M) {}
  void Sample(uni::FormatCodec::fvec &, const char *, size_t) const override {}
  void Transform(uni::FormatCodec::fvec &in) const override {
    for (Vector4A16 &v : in) {
      v = v * mul;
    }
  }
  bool CanSample() const override { return false; }
  bool CanTransform() const override { return true; }
  bool IsNormalized() const override { return false; }

  Vector4A16 mul;
};

std::set<TextureKey> MobyToGltf(const MobyV1 &moby, AppContext *ctx,
                                BinReaderRef_e stream,
                                IGHWTOCIteratorConst<MaterialV1> materials,
                                const Texture *textures) {
  IMGLTF main;

  const Skeleton *skeleton = moby.skeleton;

  for (uint32 i = 0; i < skeleton->numBones; i++) {
    gltf::Node &glNode = main.nodes.emplace_back();
    glNode.name = std::to_string(i);
    es::Matrix44 tm(skeleton->tms0[i]);

    if (int16 parentIndex = skeleton->bones[i].parentIndex;
        parentIndex == int16(i)) {
      main.scenes.front().nodes.emplace_back(i);
    } else {
      main.nodes.at(parentIndex).children.emplace_back(i);
      es::Matrix44 ptm(skeleton->tms1[parentIndex]);
      tm = ptm * tm;
    }

    tm.r4() *= YARD_TO_M;
    tm.r1().w = 0;
    tm.r2().w = 0;
    tm.r3().w = 0;
    tm.r4().w = 1;

    memcpy(glNode.matrix.data(), &tm, 64);
  }

  std::map<uint16, uint16> joints;
  const uint32 numMeshes = moby.numMeshes * (moby.anotherSet + 1);

  for (uint32 i = 0; i < numMeshes; i++) {
    const MeshV1 &mesh = moby.meshes[i];
    for (uint32 p = 0; p < mesh.numPrimitives; p++) {
      const PrimitiveV1 &prim = mesh.primitives[p];

      if (prim.numJoints < 2) {
        // continue;
      }

      for (uint32 j = 0; j < prim.numJoints; j++) {
        uint16 joint = prim.joints[j];
        FByteswapper(joint);
        joints.try_emplace(joint, joints.size());
      }
    }
  }

  {
    gltf::Skin &skn = main.skins.emplace_back();
    skn.joints.resize(joints.size());
    GLTFStream &ibmStream = main.SkinStream();
    auto [acc, accId] = main.NewAccessor(ibmStream, 16);
    acc.type = gltf::Accessor::Type::Mat4;
    acc.componentType = gltf::Accessor::ComponentType::Float;
    acc.count = joints.size();
    skn.inverseBindMatrices = accId;
    std::vector<es::Matrix44> ibms;
    ibms.resize(joints.size());

    for (auto &[jid, idx] : joints) {
      skn.joints[idx] = jid;
      es::Matrix44 ibm = skeleton->tms1[jid];
      ibm.r4() *= YARD_TO_M;
      ibm.r1().w = 0;
      ibm.r2().w = 0;
      ibm.r3().w = 0;
      ibm.r4().w = 1;
      ibms[idx] = ibm;
    }

    ibmStream.wr.WriteContainer(ibms);
  }

  struct AttributeBoneIndex : AttributeCodec {
    AttributeBoneIndex(const std::map<uint16, uint16> &joints_)
        : joints(joints_) {}
    void Sample(uni::FormatCodec::fvec &out, const char *input,
                size_t stride) const override {
      for (auto &o : out) {
        int16 index = *reinterpret_cast<const int16 *>(input);
        uint16 joint = jointMap[std::abs((index + 1) / 3)];
        FByteswapper(joint);
        o.x = joints.at(joint);
        input += stride;
      }
    }
    void Transform(uni::FormatCodec::fvec &) const override {}
    bool CanSample() const override { return true; }
    bool CanTransform() const override { return false; }
    bool IsNormalized() const override { return false; }

    const std::map<uint16, uint16> &joints;
    const uint16 *jointMap = nullptr;
  } attributeBoneIndex{joints};

  struct AttributeBoneIndices : AttributeCodec {
    AttributeBoneIndices(const std::map<uint16, uint16> &joints_)
        : joints(joints_) {}
    void Sample(uni::FormatCodec::fvec &out, const char *input,
                size_t stride) const override {
      for (auto &o : out) {
        UCVector4 index = *reinterpret_cast<const UCVector4 *>(input);
        for (uint32 i = 0; i < 4; i++) {
          uint16 joint = jointMap[index[i]];
          FByteswapper(joint);
          o[i] = joints.at(joint);
        }
        input += stride;
      }
    }
    void Transform(uni::FormatCodec::fvec &) const override {}
    bool CanSample() const override { return true; }
    bool CanTransform() const override { return false; }
    bool IsNormalized() const override { return false; }

    const std::map<uint16, uint16> &joints;
    const uint16 *jointMap = nullptr;
  } attributeBoneIndices{joints};

  AttributeMul attributeMul{moby.meshScale * 0x7fff};

  std::map<uint16, uint16> materialRemaps;

  for (uint32 i = 0; i < numMeshes; i++) {
    const MeshV1 &mesh = moby.meshes[i];
    if (mesh.numPrimitives == 0 || !mesh.primitives) {
      continue;
    }

    main.scenes.front().nodes.emplace_back(main.nodes.size());
    gltf::Node &glNode = main.nodes.emplace_back();
    glNode.mesh = main.meshes.size();
    glNode.skin = 0;
    glNode.name = "Mesh_" + std::to_string(i);
    gltf::Mesh &glMesh = main.meshes.emplace_back();

    for (uint32 p = 0; p < mesh.numPrimitives; p++) {
      const PrimitiveV1 &prim = mesh.primitives[p];
      gltf::Primitive &glPrim = glMesh.primitives.emplace_back();
      glPrim.material =
          materialRemaps.try_emplace(prim.materialIndex, materialRemaps.size())
              .first->second;
      stream.Seek(moby.vertexBufferOffset + prim.vertexBufferOffset);

      if (prim.vertexFormat == 0) {
        std::vector<Vertex0> vtx0;
        stream.ReadContainer(vtx0, prim.numVertices);

        attributeBoneIndex.jointMap = prim.joints;

        Attribute attrs[]{
            {
                .type = uni::DataType::R16G16B16,
                .format = uni::FormatType::NORM,
                .usage = AttributeType::Position,
                .customCodec = &attributeMul,
            },
            {
                .type = uni::DataType::R16,
                .format = uni::FormatType::INVALID,
                .usage = AttributeType::BoneIndices,
                .customCodec = &attributeBoneIndex,
            },
            {
                .type = uni::DataType::R16G16,
                .format = uni::FormatType::FLOAT,
                .usage = AttributeType::TextureCoordiante,
            },
            {
                .type = uni::DataType::R11G11B10,
                .format = uni::FormatType::NORM,
                .usage = AttributeType::Normal,
            },
        };

        glPrim.attributes =
            main.SaveVertices(vtx0.data(), vtx0.size(), attrs, sizeof(Vertex0));
      } else {
        std::vector<Vertex1> vtx1;
        stream.ReadContainer(vtx1, prim.numVertices);

        attributeBoneIndices.jointMap = prim.joints;

        Attribute attrs[]{
            {
                .type = uni::DataType::R16G16B16A16,
                .format = uni::FormatType::NORM,
                .usage = AttributeType::Position,
                .customCodec = &attributeMul,
            },
            {
                .type = uni::DataType::R8G8B8A8,
                .format = uni::FormatType::INVALID,
                .usage = AttributeType::BoneIndices,
                .customCodec = &attributeBoneIndices,
            },
            {
                .type = uni::DataType::R8G8B8A8,
                .format = uni::FormatType::UNORM,
                .usage = AttributeType::BoneWeights,
            },
            {
                .type = uni::DataType::R16G16,
                .format = uni::FormatType::FLOAT,
                .usage = AttributeType::TextureCoordiante,
            },
            {
                .type = uni::DataType::R11G11B10,
                .format = uni::FormatType::NORM,
                .usage = AttributeType::Normal,
            },
        };

        glPrim.attributes =
            main.SaveVertices(vtx1.data(), vtx1.size(), attrs, sizeof(Vertex1));
      }

      stream.Seek(moby.indexBufferOffset + prim.indexOffset * 2);

      std::vector<uint16> idx;
      stream.ReadContainer(idx, prim.numIndices);

      glPrim.indices = main.SaveIndices(idx.data(), idx.size()).accessorIndex;
    }
  }

  std::set<TextureKey> textureRemaps;
  MakeMaterials(ctx, main, materialRemaps, materials, textures,
                stream.BaseStream(), textureRemaps);

  main.FinishAndSave(ctx->NewFile(std::string(ctx->workingFile.GetFolder()) +
                                  "moby_" + std::to_string(moby.mobyId) +
                                  ".glb")
                         .str,
                     "");

  return textureRemaps;
}

void TieToGltf(const TieV1 &tie, IMGLTF &level,
               const LevelIndexBuffer &idxBuffer,
               const LevelVertexBuffer &vtxBuffer,
               IGHWTOCIteratorConst<TieInstanceV1> tieInstances, uint32 index,
               std::map<uint16, uint16> &materialRemaps) {
  const uint16 *indexBuffer = &idxBuffer.data;
  const char *vertexBuffer = &vtxBuffer.data;

  level.scenes.front().nodes.emplace_back(level.nodes.size());
  gltf::Node &glNode = level.nodes.emplace_back();
  glNode.mesh = level.meshes.size();
  glNode.name = "TieMesh_" + std::to_string(index);
  gltf::Mesh &glMesh = level.meshes.emplace_back();

  AttributeMul attributeMul{tie.meshScale * 0x7fff};

  for (uint32 p = 0; p < tie.numMeshes; p++) {
    const TiePrimitiveV1 &prim = tie.primitives[p];
    gltf::Primitive &glPrim = glMesh.primitives.emplace_back();
    glPrim.material =
        materialRemaps.try_emplace(prim.materialIndex, materialRemaps.size())
            .first->second;

    const uint16 *indices = indexBuffer + prim.indexOffset;
    const Vertex0 *vertices =
        reinterpret_cast<const Vertex0 *>(vertexBuffer + tie.unk13) +
        prim.vertexOffset0;

    std::vector<Vertex0> vtx0(vertices, vertices + prim.numVertices);

    for (auto &v : vtx0) {
      FByteswapper(v);
    }

    Attribute attrs[]{
        {
            .type = uni::DataType::R16G16B16A16,
            .format = uni::FormatType::NORM,
            .usage = AttributeType::Position,
            .customCodec = &attributeMul,
        },
        {
            .type = uni::DataType::R16G16,
            .format = uni::FormatType::FLOAT,
            .usage = AttributeType::TextureCoordiante,
        },
        {
            .type = uni::DataType::R11G11B10,
            .format = uni::FormatType::NORM,
            .usage = AttributeType::Normal,
        },
    };

    glPrim.attributes =
        level.SaveVertices(vtx0.data(), vtx0.size(), attrs, sizeof(Vertex0));

    std::vector<uint16> idx(indices, indices + prim.numIndices);
    for (uint16 &i : idx) {
      FByteswapper(i);
    }

    glPrim.indices = level.SaveIndices(idx.data(), idx.size()).accessorIndex;
  }

  std::vector<es::Matrix44> tms;

  for (auto &inst : tieInstances) {
    if (inst.tie == &tie) {
      es::Matrix44 tm(inst.tm);
      tm.r4() *= YARD_TO_M;
      tm.r1().w = 0;
      tm.r2().w = 0;
      tm.r3().w = 0;
      tm.r4().w = 1;
      tms.emplace_back(tm);
    }
  }

  if (tms.size() == 1) {
    memcpy(glNode.matrix.data(), tms.data(), 64);
  } else if (tms.size() > 1) {
    std::vector<Vector> scales;
    bool processScales = false;

    auto &str = level.GetTranslations();
    auto [accPos, accPosIndex] = level.NewAccessor(str, 4);
    accPos.type = gltf::Accessor::Type::Vec3;
    accPos.componentType = gltf::Accessor::ComponentType::Float;
    accPos.count = tms.size();

    auto [accRot, accRotIndex] = level.NewAccessor(str, 4, 12);
    accRot.type = gltf::Accessor::Type::Vec4;
    accRot.componentType = gltf::Accessor::ComponentType::Short;
    accRot.normalized = true;
    accRot.count = tms.size();
    Vector4A16::SetEpsilon(0.00001f);

    for (const es::Matrix44 &mtx : tms) {
      uni::RTSValue val{};
      mtx.Decompose(val.translation, val.rotation, val.scale);
      scales.emplace_back(val.scale);

      if (!processScales) {
        processScales = val.scale != Vector4A16(1, 1, 1, 0);
      }

      str.wr.Write<Vector>(val.translation);

      val.rotation.Normalize() *= 0x7fff;
      val.rotation =
          Vector4A16(_mm_round_ps(val.rotation._data, _MM_ROUND_NEAREST));
      auto comp = val.rotation.Convert<int16>();
      str.wr.Write(comp);
    }

    auto &attrs =
        glNode.GetExtensionsAndExtras()["extensions"]["EXT_mesh_gpu_instancing"]
                                       ["attributes"];

    attrs["TRANSLATION"] = accPosIndex;
    attrs["ROTATION"] = accRotIndex;

    if (processScales) {
      auto &str = level.GetScales();
      auto [accScale, accScaleIndex] = level.NewAccessor(str, 4);
      accScale.type = gltf::Accessor::Type::Vec3;
      accScale.componentType = gltf::Accessor::ComponentType::Float;
      accScale.count = tms.size();
      str.wr.WriteContainer(scales);
      attrs["SCALE"] = accScaleIndex;
    }
  }
}

void ShrubToGltf(const Shrub &shrub, IMGLTF &level,
                 const LevelIndexBuffer &idxBuffer,
                 const LevelVertexBuffer &vtxBuffer,
                 IGHWTOCIteratorConst<ShrubInstance> shrubInstances,
                 uint32 index, std::map<uint16, uint16> &materialRemaps) {
  const uint16 *indexBuffer = &idxBuffer.data;
  const char *vertexBuffer = &vtxBuffer.data;

  level.scenes.front().nodes.emplace_back(level.nodes.size());
  gltf::Node &glNode = level.nodes.emplace_back();
  glNode.mesh = level.meshes.size();
  glNode.name = "Shrub_" + std::to_string(index);
  gltf::Mesh &glMesh = level.meshes.emplace_back();

  AttributeMul attributeMul{shrub.meshScale / 0x1000};

  for (uint32 p = 0; p < shrub.numPrimitives; p++) {
    const ShrubPrimitive &prim = shrub.primitives[p];
    gltf::Primitive &glPrim = glMesh.primitives.emplace_back();
    glPrim.material =
        materialRemaps.try_emplace(prim.materialIndex, materialRemaps.size())
            .first->second;

    const uint16 *indices = indexBuffer + prim.indexOffset;
    const Vertex0 *vertices = reinterpret_cast<const Vertex0 *>(
        vertexBuffer + prim.vertexBufferOffset);

    std::vector<Vertex0> vtx0(vertices, vertices + prim.numVertices);

    for (auto &v : vtx0) {
      FByteswapper(v);
    }

    Attribute attrs[]{
        {
            .type = uni::DataType::R16G16B16A16,
            .format = uni::FormatType::NORM,
            .usage = AttributeType::Position,
            .customCodec = &attributeMul,
        },
        {
            .type = uni::DataType::R16G16,
            .format = uni::FormatType::FLOAT,
            .usage = AttributeType::TextureCoordiante,
        },
        {
            .type = uni::DataType::R11G11B10,
            .format = uni::FormatType::NORM,
            .usage = AttributeType::Normal,
        },
    };

    glPrim.attributes =
        level.SaveVertices(vtx0.data(), vtx0.size(), attrs, sizeof(Vertex0));

    std::vector<uint16> idx(indices, indices + prim.numIndices);
    for (uint16 &i : idx) {
      FByteswapper(i);
    }

    glPrim.indices = level.SaveIndices(idx.data(), idx.size()).accessorIndex;
  }

  std::vector<es::Matrix44> tms;

  for (auto &inst : shrubInstances) {
    if (inst.shrub == &shrub) {
      es::Matrix44 tm(inst.tm);
      tm.r4() *= YARD_TO_M;
      tm.r1().w = 0;
      tm.r2().w = 0;
      tm.r3().w = 0;
      tm.r4().w = 1;
      tms.emplace_back(tm);
    }
  }

  if (tms.size() == 1) {
    memcpy(glNode.matrix.data(), tms.data(), 64);
  } else if (tms.size() > 1) {
    std::vector<Vector> scales;
    bool processScales = false;

    auto &str = level.GetTranslations();
    auto [accPos, accPosIndex] = level.NewAccessor(str, 4);
    accPos.type = gltf::Accessor::Type::Vec3;
    accPos.componentType = gltf::Accessor::ComponentType::Float;
    accPos.count = tms.size();

    auto [accRot, accRotIndex] = level.NewAccessor(str, 4, 12);
    accRot.type = gltf::Accessor::Type::Vec4;
    accRot.componentType = gltf::Accessor::ComponentType::Short;
    accRot.normalized = true;
    accRot.count = tms.size();
    Vector4A16::SetEpsilon(0.00001f);

    for (const es::Matrix44 &mtx : tms) {
      uni::RTSValue val{};
      mtx.Decompose(val.translation, val.rotation, val.scale);
      scales.emplace_back(val.scale);

      if (!processScales) {
        processScales = val.scale != Vector4A16(1, 1, 1, 0);
      }

      str.wr.Write<Vector>(val.translation);

      val.rotation.Normalize() *= 0x7fff;
      val.rotation =
          Vector4A16(_mm_round_ps(val.rotation._data, _MM_ROUND_NEAREST));
      auto comp = val.rotation.Convert<int16>();
      str.wr.Write(comp);
    }

    auto &attrs =
        glNode.GetExtensionsAndExtras()["extensions"]["EXT_mesh_gpu_instancing"]
                                       ["attributes"];

    attrs["TRANSLATION"] = accPosIndex;
    attrs["ROTATION"] = accRotIndex;

    if (processScales) {
      auto &str = level.GetScales();
      auto [accScale, accScaleIndex] = level.NewAccessor(str, 4);
      accScale.type = gltf::Accessor::Type::Vec3;
      accScale.componentType = gltf::Accessor::ComponentType::Float;
      accScale.count = tms.size();
      str.wr.WriteContainer(scales);
      attrs["SCALE"] = accScaleIndex;
    }
  }
}

void RegionToGltf(IGHWTOCIteratorConst<RegionMesh> items, IMGLTF &level,
                  const LevelIndexBuffer &idxBuffer,
                  const LevelVertexBuffer &vtxBuffer,
                  std::map<uint16, uint16> &materialRemaps) {
  const uint16 *indexBuffer = &idxBuffer.data;
  const char *vertexBuffer = &vtxBuffer.data;

  level.scenes.front().nodes.emplace_back(level.nodes.size());
  gltf::Node &glNode = level.nodes.emplace_back();
  glNode.mesh = level.meshes.size();
  glNode.name = "RegionMesh";
  gltf::Mesh &glMesh = level.meshes.emplace_back();

  for (const RegionMesh &item : items) {
    gltf::Primitive &glPrim = glMesh.primitives.emplace_back();
    glPrim.material =
        materialRemaps.try_emplace(item.materialIndex, materialRemaps.size())
            .first->second;

    const uint16 *indices = indexBuffer + item.indexOffset;
    const RegionVertex *vertices = reinterpret_cast<const RegionVertex *>(
        vertexBuffer + item.vertexOffset);

    std::vector<RegionVertex> vtx0(vertices, vertices + item.numVerties);

    for (auto &v : vtx0) {
      FByteswapper(v);
    }

    AttributeMad attributeMad;
    attributeMad.mul = (Vector4A16(0x7fff) / 0x100) * YARD_TO_M;
    attributeMad.add = (item.position / 0x100) * YARD_TO_M;

    Attribute attrs[]{
        {
            .type = uni::DataType::R16G16B16A16,
            .format = uni::FormatType::NORM,
            .usage = AttributeType::Position,
            .customCodec = &attributeMad,
        },
        {
            .type = uni::DataType::R16G16,
            .format = uni::FormatType::FLOAT,
            .usage = AttributeType::TextureCoordiante,
        },
        {
            .type = uni::DataType::R16G16,
            .format = uni::FormatType::FLOAT,
            .usage = AttributeType::TextureCoordiante,
        },
        {
            .type = uni::DataType::R11G11B10,
            .format = uni::FormatType::NORM,
            .usage = AttributeType::Normal,
        },
    };

    glPrim.attributes = level.SaveVertices(vtx0.data(), vtx0.size(), attrs,
                                           sizeof(RegionVertex));

    std::vector<uint16> idx(indices, indices + item.numIndices);
    for (uint16 &i : idx) {
      FByteswapper(i);
    }

    glPrim.indices = level.SaveIndices(idx.data(), idx.size()).accessorIndex;
  }
}

void FoliageToGltf(const Foliage &foliage, IMGLTF &level,
                   const LevelIndexBuffer &idxBuffer,
                   const LevelVertexBuffer &vtxBuffer,
                   IGHWTOCIteratorConst<FoliageInstance> instances,
                   std::map<uint16, uint16> &materialRemaps, uint32 index) {
  const uint16 *indexBuffer = &idxBuffer.data;
  const char *vertexBuffer = &vtxBuffer.data;

  const uint16 *indices = indexBuffer + foliage.indexOffset;
  const uint32 numVertices =
      (foliage.spriteVertexOffset - foliage.branchVertexOffset) /
      sizeof(BranchVertex);

  level.scenes.front().nodes.emplace_back(level.nodes.size());
  const size_t folNodeIndex = level.nodes.size();
  gltf::Node &glFoliageNode = level.nodes.emplace_back();
  glFoliageNode.name = "Foliage" + std::to_string(index);

  if (numVertices) {
    const BranchVertex *vertices = reinterpret_cast<const BranchVertex *>(
        vertexBuffer + foliage.branchVertexOffset);
    std::vector<BranchVertex> vtx0(vertices, vertices + numVertices);

    glFoliageNode.mesh = level.meshes.size();
    gltf::Mesh &glMesh = level.meshes.emplace_back();

    for (auto &v : vtx0) {
      FByteswapper(v);
    }

    AttributeUnormToSnorm sn;
    AttributeMul attributeMul(YARD_TO_M);

    Attribute attrs[]{
        {
            .type = uni::DataType::R16G16B16A16,
            .format = uni::FormatType::FLOAT,
            .usage = AttributeType::Position,
            .customCodec = &attributeMul,
        },
        {
            .type = uni::DataType::R16G16,
            .format = uni::FormatType::FLOAT,
            .usage = AttributeType::TextureCoordiante,
        },
        {
            .type = uni::DataType::R8G8B8A8,
            .format = uni::FormatType::UNORM,
            .usage = AttributeType::Undefined,
        },
        {
            .type = uni::DataType::R8G8B8,
            .format = uni::FormatType::UNORM,
            .usage = AttributeType::Normal,
            .customCodec = &sn,
        },
    };

    auto attrsa = level.SaveVertices(vtx0.data(), vtx0.size(), attrs,
                                     sizeof(BranchVertex));

    for (auto &r : foliage.branchLods) {
      if (r.numIndices == 0) {
        continue;
      }

      gltf::Primitive &glPrim = glMesh.primitives.emplace_back();
      glPrim.attributes = attrsa;

      std::vector<uint16> idx(indices + r.indexOffset,
                              indices + r.numIndices + r.indexOffset);
      for (uint16 &i : idx) {
        FByteswapper(i);
      }

      glPrim.material =
          materialRemaps
              .try_emplace(foliage.textureIndex,
                           materialRemaps.size() + level.materials.size())
              .first->second;

      glPrim.indices = level.SaveIndices(idx.data(), idx.size()).accessorIndex;

      break; // only 1 lod
    }
  }

  std::vector<es::Matrix44> tms;

  for (auto &inst : instances) {
    if (inst.foliage == &foliage) {
      es::Matrix44 tm(inst.tm);
      tm.r4() *= YARD_TO_M;
      tm.r1().w = 0;
      tm.r2().w = 0;
      tm.r3().w = 0;
      tm.r4().w = 1;
      tms.emplace_back(tm);
    }
  }

  if (tms.size() == 1) {
    memcpy(glFoliageNode.matrix.data(), tms.data(), 64);
  } else if (tms.size() > 1) {
    std::vector<Vector> scales;
    bool processScales = false;

    auto &str = level.GetTranslations();
    auto [accPos, accPosIndex] = level.NewAccessor(str, 4);
    accPos.type = gltf::Accessor::Type::Vec3;
    accPos.componentType = gltf::Accessor::ComponentType::Float;
    accPos.count = tms.size();

    auto [accRot, accRotIndex] = level.NewAccessor(str, 4, 12);
    accRot.type = gltf::Accessor::Type::Vec4;
    accRot.componentType = gltf::Accessor::ComponentType::Short;
    accRot.normalized = true;
    accRot.count = tms.size();
    Vector4A16::SetEpsilon(0.00001f);

    for (const es::Matrix44 &mtx : tms) {
      uni::RTSValue val{};
      mtx.Decompose(val.translation, val.rotation, val.scale);
      scales.emplace_back(val.scale);

      if (!processScales) {
        processScales = val.scale != Vector4A16(1, 1, 1, 0);
      }

      str.wr.Write<Vector>(val.translation);

      val.rotation.Normalize() *= 0x7fff;
      val.rotation =
          Vector4A16(_mm_round_ps(val.rotation._data, _MM_ROUND_NEAREST));
      auto comp = val.rotation.Convert<int16>();
      str.wr.Write(comp);
    }

    auto &attrs =
        level.nodes.at(folNodeIndex)
            .GetExtensionsAndExtras()["extensions"]["EXT_mesh_gpu_instancing"]
                                     ["attributes"];

    attrs["TRANSLATION"] = accPosIndex;
    attrs["ROTATION"] = accRotIndex;

    if (processScales) {
      auto &str = level.GetScales();
      auto [accScale, accScaleIndex] = level.NewAccessor(str, 4);
      accScale.type = gltf::Accessor::Type::Vec3;
      accScale.componentType = gltf::Accessor::ComponentType::Float;
      accScale.count = tms.size();
      str.wr.WriteContainer(scales);
      attrs["SCALE"] = accScaleIndex;
    }
  }

  for (uint32 i = 0; i < foliage.usedSpriteLods; i++) {
    const SpriteLodRange &lod = foliage.spriteLodRanges[i];
    gltf::Mesh glMesh;

    for (uint32 s = 0; s < foliage.usedSpriteRanges; s++) {
      const SpriteRange &r = foliage.spriteRanges[s];

      struct SpriteVertexOut {
        Vector position;
        USVector2 uv;
        uint32 unk;
      };

      std::vector<uint16> idx(indices + r.indexBegin, indices + r.indexEnd);
      uint16 numVertices = 0;
      for (uint16 &i : idx) {
        FByteswapper(i);
        numVertices = std::max(i, numVertices);
      }
      numVertices++;

      const SpriteVertex *vertices = reinterpret_cast<const SpriteVertex *>(
          vertexBuffer + foliage.spriteVertexOffset);
      std::vector<SpriteVertex> inVerts(vertices, vertices + numVertices);
      for (SpriteVertex &i : inVerts) {
        FByteswapper(i);
      }
      std::vector<SpriteVertexOut> outVerts;

      const Vector4 *centers = reinterpret_cast<const Vector4 *>(
          foliage.spritePositions.Get() + r.positionsOffset);

      for (uint32 i = 0; i < r.numSprites; i++) {
        if (uint32 spriteIndex = i * 6 + r.indexBegin;
            spriteIndex < lod.indexBegin || spriteIndex >= lod.indexEnd) {
          continue;
        }

        for (uint32 d = 0; d < 6; d++) {
          uint32 vIndex = idx.at(i * 6 + d);
          const SpriteVertex &vert = inVerts.at(vIndex);
          SpriteVertexOut outVert{
              .position = Vector(centers[i]) +
                          Vector(vert.spriteSize[0], vert.spriteSize[1], 0),
              .uv = vert.uv,
              .unk = vert.unk,
          };

          outVerts.emplace_back(outVert);
        }

        // possible todo: save center vertex (interpolate uvs, as TriangleFan
        // [4, 0, 1, 2, 3])
      }

      AttributeMul attributeMul(YARD_TO_M);

      Attribute attrs[]{
          {
              .type = uni::DataType::R32G32B32,
              .format = uni::FormatType::FLOAT,
              .usage = AttributeType::Position,
              .customCodec = &attributeMul,
          },
          {
              .type = uni::DataType::R16G16,
              .format = uni::FormatType::FLOAT,
              .usage = AttributeType::TextureCoordiante,
          },
      };

      if (outVerts.size() > 0) {
        gltf::Primitive &glPrim = glMesh.primitives.emplace_back();
        glPrim.attributes = level.SaveVertices(outVerts.data(), outVerts.size(),
                                               attrs, sizeof(SpriteVertexOut));
        glPrim.material =
            materialRemaps
                .try_emplace(foliage.textureIndex,
                             materialRemaps.size() + level.materials.size())
                .first->second;
      }
    }

    if (glMesh.primitives.size() > 0) {
      level.nodes.at(folNodeIndex).children.emplace_back(level.nodes.size());
      gltf::Node &glNode = level.nodes.emplace_back();
      glNode.mesh = level.meshes.size();
      glNode.name = "Foliage" + std::to_string(index) + "_Sprites";
      glNode.GetExtensionsAndExtras() =
          level.nodes.at(folNodeIndex).GetExtensionsAndExtras();
      level.meshes.emplace_back(std::move(glMesh));
    }

    break; // only 1 lod
  }
}

void ExtractTextures(AppContext *ctx, std::string path,
                     IGHWTOCIteratorConst<Texture> textures,
                     std::istream &textureStream,
                     std::set<TextureKey> totalTextures = {}) {
  for (uint16 index = 0; const Texture &tex : textures) {
    TextureKey info{
        .tex = &tex,
        .id = index++,
    };

    if (totalTextures.count(info) == 0) {
      ExtractTexture(ctx, path, textureStream, info);
    }
  }
}

void PlantsToGltf(const PlantPrimitive &prim, IMGLTF &level,
                  const LevelIndexBuffer &idxBuffer,
                  const LevelVertexBuffer &vtxBuffer,
                  // IGHWTOCIteratorConst<FoliageInstance> instances,
                  std::map<uint16, uint16> &materialRemaps, uint32 index) {
  const uint16 *indices = &idxBuffer.data + prim.indexOffset;
  std::vector<uint16> idx(indices, indices + prim.numIndices);
  for (uint16 &i : idx) {
    FByteswapper(i);
  }

  level.scenes.front().nodes.emplace_back(level.nodes.size());
  auto &glNode = level.nodes.emplace_back();
  glNode.mesh = level.meshes.size();
  glNode.name = "plant_" + std::to_string(index);
  auto &glMesh = level.meshes.emplace_back();
  auto &glPrim = glMesh.primitives.emplace_back();
  auto outI = level.SaveIndices(idx.data(), idx.size());
  const uint32 numVertices = outI.maxIndex + 1;
  glPrim.indices = outI.accessorIndex;

  glPrim.material =
      materialRemaps.try_emplace(prim.materialIndex, materialRemaps.size())
          .first->second;

  const PlantVertex *vertices = reinterpret_cast<const PlantVertex *>(
      &vtxBuffer.data + prim.vertexBufferOffset);
  std::vector<PlantVertex> vtx0(vertices, vertices + numVertices);

  for (auto &v : vtx0) {
    FByteswapper(v);
  }

  AttributeMul attributeMul(YARD_TO_M);

  Attribute attrs[]{
      {
          .type = uni::DataType::R16G16B16A16,
          .format = uni::FormatType::FLOAT,
          .usage = AttributeType::Position,
          .customCodec = &attributeMul,
      },
      {
          .type = uni::DataType::R8G8B8A8,
          .format = uni::FormatType::UNORM,
          .usage = AttributeType::VertexColor,
      },
      {
          .type = uni::DataType::R16G16,
          .format = uni::FormatType::FLOAT,
          .usage = AttributeType::TextureCoordiante,
      },
  };

  glPrim.attributes =
      level.SaveVertices(vtx0.data(), vtx0.size(), attrs, sizeof(PlantVertex));
}

void AppProcessFile(AppContext *ctx) {
  BinReaderRef_e rd(ctx->GetStream());
  IGHW main;
  main.FromStream(rd, Version::RFOM);
  const std::string workFolder(ctx->workingFile.GetFolder());

  IGHWTOCIteratorConst<MobyV1> mobys;
  IGHWTOCIteratorConst<TieV1> ties;
  IGHWTOCIteratorConst<TieInstanceV1> tieInstances;
  IGHWTOCIteratorConst<RegionMesh> regionMeshes;
  IGHWTOCIteratorConst<DirectionalLightmapTextureV1> lightmaps;
  IGHWTOCIteratorConst<TextureV1> textures;
  IGHWTOCIteratorConst<BlendmapTextureV1> blendMaps;
  IGHWTOCIteratorConst<MaterialV1> materials;
  IGHWTOCIteratorConst<Shrub> shrubs;
  IGHWTOCIteratorConst<ShrubInstance> shrubInstances;
  IGHWTOCIteratorConst<Foliage> foliages;
  IGHWTOCIteratorConst<FoliageInstance> foliageInstances;
  IGHWTOCIteratorConst<PlantPrimitive> plantMeshes;
  auto txStr = ctx->RequestFile(workFolder + "ps3leveltexs.dat");
  auto vtxStr = ctx->RequestFile(workFolder + "ps3levelverts.dat");
  IGHW buffers;
  buffers.FromStream(*vtxStr.Get(), Version::RFOM);
  IGHWTOCIteratorConst<LevelVertexBuffer> verts;
  IGHWTOCIteratorConst<LevelIndexBuffer> indices;
  CatchClasses(buffers, verts, indices);
  CatchClasses(main, mobys, ties, tieInstances, regionMeshes, lightmaps,
               textures, blendMaps, materials, shrubs, shrubInstances, foliages,
               foliageInstances, plantMeshes);
  BinReaderRef_e txRd(*txStr.Get());
  txRd.SwapEndian(true);

  std::set<TextureKey> totalTextures;

  for (const MobyV1 &moby : mobys) {
    totalTextures.merge(
        MobyToGltf(moby, ctx, txRd, materials, textures.begin()));
  }

  {
    IMGLTF level;
    std::map<uint16, uint16> materialRemaps;
    std::map<uint16, uint16> foliageRemaps;

    for (size_t tieIdx = 0; const TieV1 &tie : ties) {
      TieToGltf(tie, level, indices.at(0), verts.at(0), tieInstances, tieIdx++,
                materialRemaps);
    }

    RegionToGltf(regionMeshes, level, indices.at(0), verts.at(0),
                 materialRemaps);

    for (size_t tieIdx = 0; const Shrub &item : shrubs) {
      ShrubToGltf(item, level, indices.at(0), verts.at(0), shrubInstances,
                  tieIdx++, materialRemaps);
    }

    std::set<TextureKey> textureRemaps;
    MakeMaterials(ctx, level, materialRemaps, materials, textures.begin(),
                  txRd.BaseStream(), textureRemaps);

    for (size_t folIdx = 0; const Foliage &foliage : foliages) {
      FoliageToGltf(foliage, level, indices.at(0), verts.at(0),
                    foliageInstances, foliageRemaps, folIdx++);
    }

    MakeFoliageMaterials(ctx, level, foliageRemaps, textures.begin(),
                         txRd.BaseStream(), textureRemaps);

    totalTextures.merge(textureRemaps);
    level.FinishAndSave(ctx->NewFile(workFolder + "level.glb").str, "");
  }

  {
    IMGLTF plants;
    std::map<uint16, uint16> materialRemaps;

    for (size_t index = 0; auto &p : plantMeshes) {
      PlantsToGltf(p, plants, indices.at(0), verts.at(0), materialRemaps,
                   index++);
    }

    std::set<TextureKey> textureRemaps;
    MakeMaterials(ctx, plants, materialRemaps, materials, textures.begin(),
                  txRd.BaseStream(), textureRemaps);
    totalTextures.merge(textureRemaps);

    for (auto &m : plants.materials) {
      m.doubleSided = true;
    }

    plants.FinishAndSave(ctx->NewFile(workFolder + "plants.glb").str, "");
  }

  ExtractTextures(ctx, "lightmap_",
                  reinterpret_cast<IGHWTOCIteratorConst<Texture> &>(lightmaps),
                  txRd.BaseStream());
  ExtractTextures(ctx, "texture_",
                  reinterpret_cast<IGHWTOCIteratorConst<Texture> &>(textures),
                  txRd.BaseStream(), totalTextures);
  ExtractTextures(ctx, "blendmap_",
                  reinterpret_cast<IGHWTOCIteratorConst<Texture> &>(blendMaps),
                  txRd.BaseStream());
}
