#include "gltf_ighw.hpp"
#include "insomnia/internal/vertex.hpp"
#include "nlohmann/json.hpp"
#include "spike/app_context.hpp"
#include "spike/io/binreader_stream.hpp"
#include "spike/uni/rts.hpp"

void MobyToGltf(IGHWTOCIteratorConst<ResourceShaders> &shaders, IGHW &ighw,
                AppContext *ctx, AppContextStream &shdStream) {
  IGHWTOCIteratorConst<MobyV2> mobys;
  IGHWTOCIteratorConst<VertexBuffer> vertexBuffers;
  IGHWTOCIteratorConst<IndexBuffer> indexBuffers;
  IGHWTOCIteratorConst<ShaderResourceLookup> shaderLookups;
  AFileInfo mobyPath;

  auto CatchFileName = [&mobyPath, ctx](const IGHWTOC &toc) {
    if (toc.id == ResourceMobyPathLookupId) {
      mobyPath.Load(std::string(ctx->workingFile.GetFolder()) +
                    reinterpret_cast<const char *>(toc.data.Get()));
    }
  };

  CatchClassesLambda(ighw, CatchFileName, mobys, vertexBuffers, indexBuffers,
                     shaderLookups);

  IGHW shaderMain;
  GLTFModel main;

  for (const ShaderResourceLookup &lookup : shaderLookups) {
    auto found = std::find(shaders.begin(), shaders.end(), lookup.hash);
    gltf::Material &gmat = main.materials.emplace_back();

    if (found != shaders.end()) {
      BinReaderRef_e rd(*shdStream.Get());
      rd.SetRelativeOrigin(found->offset);
      shaderMain.FromStream(rd, Version::V2);
      IGHWTOCIteratorConst<MaterialResourceNameLookup> lookups;
      CatchClasses(shaderMain, lookups);
      const MaterialResourceNameLookup *lookup = lookups.begin();
      gmat.name = lookup->lookupPath.Get();
    } else {
      gmat.name = std::to_string(lookup.hash.part1);
    }
  }

  const MobyV2 *moby = mobys.begin();
  const Skeleton *skeleton = moby->skeleton;

  for (uint32 i = 0; i < skeleton->numBones; i++) {
    gltf::Node &glNode = main.nodes.emplace_back();
    glNode.name = std::to_string(i);
    es::Matrix44 tm(skeleton->tms0[i]);

    if (int16 parentIndex = skeleton->bones[i].parentIndex; parentIndex < 0) {
      main.scenes.front().nodes.emplace_back(i);
    } else {
      main.nodes.at(parentIndex).children.emplace_back(i);
      es::Matrix44 ptm(skeleton->tms1[parentIndex]);
      tm = ptm * tm;
    }

    tm.r1().w = 0;
    tm.r2().w = 0;
    tm.r3().w = 0;
    tm.r4().w = 1;

    memcpy(glNode.matrix.data(), &tm, 64);
  }

  const uint16 *indexBuffer = &indexBuffers.begin()->data;
  const char *vertexBuffer = &vertexBuffers.begin()->data;
  std::map<uint16, uint16> joints;

  for (uint32 i = 0; i < moby->numMeshes; i++) {
    const MeshV2 &mesh = moby->meshes[i];
    for (uint32 p = 0; p < mesh.numPrimitives; p++) {
      const PrimitiveV2 &prim = mesh.primitives[p];

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

  struct AttributeMul : AttributeCodec {
    AttributeMul(float scale) : mul(scale * 0x7fff) {}
    void Sample(uni::FormatCodec::fvec &, const char *, size_t) const override {
    }
    void Transform(uni::FormatCodec::fvec &in) const override {
      for (Vector4A16 &v : in) {
        v = v * mul;
      }
    }
    bool CanSample() const override { return false; }
    bool CanTransform() const override { return true; }
    bool IsNormalized() const override { return false; }

    Vector4A16 mul;
  } attributeMul{moby->meshScale};

  for (uint32 i = 0; i < moby->numMeshes; i++) {
    const MeshV2 &mesh = moby->meshes[i];
    if (mesh.primitives == 0) {
      continue;
    }

    main.scenes.front().nodes.emplace_back(main.nodes.size());
    gltf::Node &glNode = main.nodes.emplace_back();
    glNode.mesh = main.meshes.size();
    glNode.skin = 0;
    glNode.name = "Mesh_" + std::to_string(i);
    gltf::Mesh &glMesh = main.meshes.emplace_back();

    for (uint32 p = 0; p < mesh.numPrimitives; p++) {
      const PrimitiveV2 &prim = mesh.primitives[p];
      gltf::Primitive &glPrim = glMesh.primitives.emplace_back();
      glPrim.material = prim.materialIndex;

      const uint16 *indices = indexBuffer + prim.indexOffset;
      const char *vertices = vertexBuffer + prim.vertexOffset;

      if (prim.vertexFormat == 0) {
        std::vector<Vertex0> vtx0(reinterpret_cast<const Vertex0 *>(vertices),
                                  reinterpret_cast<const Vertex0 *>(vertices) +
                                      prim.numVertices);

        for (auto &v : vtx0) {
          FByteswapper(v);
        }

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
        std::vector<Vertex1> vtx1(reinterpret_cast<const Vertex1 *>(vertices),
                                  reinterpret_cast<const Vertex1 *>(vertices) +
                                      prim.numVertices);

        for (auto &v : vtx1) {
          FByteswapper(v);
        }

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

      std::vector<uint16> idx(indices, indices + prim.numIndices);
      for (uint16 &i : idx) {
        FByteswapper(i);
      }

      glPrim.indices = main.SaveIndices(idx.data(), idx.size()).accessorIndex;
    }
  }

  main.FinishAndSave(ctx->NewFile(mobyPath.ChangeExtension2("glb")).str, "");
}

template <class Ty>
void ShadersToGltf(GLTF &main, IGHWTOCIteratorConst<Ty> shaderLookups,
                   IGHWTOCIteratorConst<ResourceShaders> &shaders,
                   AppContextStream &shdStream,
                   std::map<Hash, uint32> &materialRemaps) {
  for (const Ty &lookup : shaderLookups) {
    if (auto found = materialRemaps.find(lookup.hash);
        found != materialRemaps.end()) {
      continue;
    }

    materialRemaps.emplace(lookup.hash, materialRemaps.size());
    auto found = std::find(shaders.begin(), shaders.end(), lookup.hash);
    gltf::Material &gmat = main.materials.emplace_back();

    if (found != shaders.end()) {
      BinReaderRef_e rd(*shdStream.Get());
      rd.SetRelativeOrigin(found->offset);
      IGHW shaderMain;
      shaderMain.FromStream(rd, Version::V2);
      IGHWTOCIteratorConst<MaterialResourceNameLookup> lookups;
      CatchClasses(shaderMain, lookups);
      const MaterialResourceNameLookup *lookup = lookups.begin();
      gmat.name = lookup->lookupPath.Get();
    } else {
      gmat.name = std::to_string(lookup.hash.part1);
    }
  }
}

size_t TieToGltf(GLTFModel &main,
                 IGHWTOCIteratorConst<ResourceShaders> &shaders, IGHW &ighw,
                 AppContextStream &shdStream,
                 std::map<Hash, uint32> &materialRemaps) {
  IGHWTOCIteratorConst<TieV2> ties;
  IGHWTOCIteratorConst<TieVertexBuffer> vertexBuffers;
  IGHWTOCIteratorConst<TieIndexBuffer> indexBuffers;
  IGHWTOCIteratorConst<ShaderResourceLookup> shaderLookups;
  AFileInfo tiePath;

  auto CatchFileName = [&tiePath](const IGHWTOC &toc) {
    if (toc.id == ResourceTiePathLookupId) {
      tiePath.Load(reinterpret_cast<const char *>(toc.data.Get()));
    }
  };

  CatchClassesLambda(ighw, CatchFileName, ties, vertexBuffers, indexBuffers,
                     shaderLookups);
  ShadersToGltf(main, shaderLookups, shaders, shdStream, materialRemaps);

  assert(std::distance(ties.begin(), ties.end()) == 1);

  const auto *tie = ties.begin();
  const uint16 *indexBuffer = &indexBuffers.begin()->data;
  const char *vertexBuffer = &vertexBuffers.begin()->data;

  struct AttributeMul : AttributeCodec {
    AttributeMul(Vector scale) : mul(scale * 0x7fff) {}
    void Sample(uni::FormatCodec::fvec &, const char *, size_t) const override {
    }
    void Transform(uni::FormatCodec::fvec &in) const override {
      for (Vector4A16 &v : in) {
        v = v * mul;
      }
    }
    bool CanSample() const override { return false; }
    bool CanTransform() const override { return true; }
    bool IsNormalized() const override { return false; }

    Vector4A16 mul;
  } attributeMul{tie->meshScale * YARD_TO_M};

  main.scenes.front().nodes.emplace_back(main.nodes.size());
  gltf::Node &glNode = main.nodes.emplace_back();
  glNode.mesh = main.meshes.size();
  glNode.name = tiePath.GetFilename();
  gltf::Mesh &glMesh = main.meshes.emplace_back();

  for (uint32 p = 0; p < tie->numPrimitives; p++) {
    const auto &prim = tie->primitives[p];
    gltf::Primitive &glPrim = glMesh.primitives.emplace_back();
    glPrim.material =
        materialRemaps.at(shaderLookups.at(prim.materialIndex).hash);

    const uint16 *indices = indexBuffer + prim.indexOffset;
    const Vertex0 *vertices =
        reinterpret_cast<const Vertex0 *>(vertexBuffer) + prim.vertexOffset0;

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
        main.SaveVertices(vtx0.data(), vtx0.size(), attrs, sizeof(Vertex0));

    std::vector<uint16> idx(indices, indices + prim.numIndices);
    for (uint16 &i : idx) {
      FByteswapper(i);
    }

    glPrim.indices = main.SaveIndices(idx.data(), idx.size()).accessorIndex;
  }

  return main.nodes.size() - 1;
}

void TieToGltf(IGHWTOCIteratorConst<ResourceShaders> &shaders, IGHW &ighw,
               AppContext *ctx, AppContextStream &shdStream) {

  AFileInfo tiePath;

  auto CatchFileName = [&tiePath, ctx](const IGHWTOC &toc) {
    if (toc.id == ResourceTiePathLookupId) {
      tiePath.Load(std::string(ctx->workingFile.GetFolder()) +
                   reinterpret_cast<const char *>(toc.data.Get()));
    }
  };

  CatchClassesLambda(ighw, CatchFileName);

  GLTFModel main;
  std::map<Hash, uint32> materialRemaps;

  TieToGltf(main, shaders, ighw, shdStream, materialRemaps);

  main.FinishAndSave(ctx->NewFile(tiePath.ChangeExtension2("glb")).str, "");
}

size_t ShrubToGltf(GLTFModel &main,
                   IGHWTOCIteratorConst<ResourceShaders> &shaders, IGHW &ighw,
                   AppContextStream &shdStream,
                   std::map<Hash, uint32> &materialRemaps) {
  IGHWTOCIteratorConst<ShrubV2> shrubs;
  IGHWTOCIteratorConst<ShrubVertexBuffer> vertexBuffers;
  IGHWTOCIteratorConst<ShrubIndexBuffer> indexBuffers;
  IGHWTOCIteratorConst<ShaderResourceLookup> shaderLookups;
  AFileInfo shrubPath;

  auto CatchFileName = [&shrubPath](const IGHWTOC &toc) {
    if (toc.id == ResourceShrubPathLookupId) {
      shrubPath.Load(reinterpret_cast<const char *>(toc.data.Get()));
    }
  };

  CatchClassesLambda(ighw, CatchFileName, shrubs, vertexBuffers, indexBuffers,
                     shaderLookups);
  ShadersToGltf(main, shaderLookups, shaders, shdStream, materialRemaps);

  assert(std::distance(shrubs.begin(), shrubs.end()) == 1);

  const ShrubV2 *shrub = shrubs.begin();
  const uint16 *indexBuffer = &indexBuffers.begin()->data;
  const char *vertexBuffer = &vertexBuffers.begin()->data;

  struct AttributeMul : AttributeCodec {
    AttributeMul(Vector scale) : mul(scale) {}
    void Sample(uni::FormatCodec::fvec &, const char *, size_t) const override {
    }
    void Transform(uni::FormatCodec::fvec &in) const override {
      for (Vector4A16 &v : in) {
        v = v * mul;
      }
    }
    bool CanSample() const override { return false; }
    bool CanTransform() const override { return true; }
    bool IsNormalized() const override { return false; }

    Vector4A16 mul;
  } attributeMul{YARD_TO_M * shrub->unk1[2]};

  main.scenes.front().nodes.emplace_back(main.nodes.size());
  gltf::Node &glNode = main.nodes.emplace_back();
  glNode.mesh = main.meshes.size();
  glNode.name = shrubPath.GetFilename();
  gltf::Mesh &glMesh = main.meshes.emplace_back();

  gltf::Primitive &glPrim = glMesh.primitives.emplace_back();
  glPrim.material = materialRemaps.at(shaderLookups.at(0).hash);

  const uint16 *indices = indexBuffer;
  std::vector<uint16> idx(indices, indices + shrub->numIndices);
  for (uint16 &i : idx) {
    FByteswapper(i);
  }

  auto idxResult = main.SaveIndices(idx.data(), idx.size());
  glPrim.indices = idxResult.accessorIndex;

  const ShrubV2Vertex *vertices =
      reinterpret_cast<const ShrubV2Vertex *>(vertexBuffer);

  std::vector<ShrubV2Vertex> vtx0(vertices, vertices + idxResult.maxIndex + 1);

  for (auto &v : vtx0) {
    FByteswapper(v);
  }

  Attribute attrs[]{{
                        .type = uni::DataType::R16G16B16A16,
                        .format = uni::FormatType::NORM,
                        .usage = AttributeType::Position,
                        .customCodec = &attributeMul,
                    },
                    {
                        .type = uni::DataType::R16G16,
                        .format = uni::FormatType::FLOAT,
                        .usage = AttributeType::TextureCoordiante,
                    }};

  glPrim.attributes =
      main.SaveVertices(vtx0.data(), vtx0.size(), attrs, sizeof(ShrubV2Vertex));

  return main.nodes.size() - 1;
}

void ShrubToGltf(IGHWTOCIteratorConst<ResourceShaders> &shaders, IGHW &ighw,
                 AppContext *ctx, AppContextStream &shdStream) {

  AFileInfo shrubPath;

  auto CatchFileName = [&shrubPath, ctx](const IGHWTOC &toc) {
    if (toc.id == ResourceShrubPathLookupId) {
      shrubPath.Load(std::string(ctx->workingFile.GetFolder()) +
                     reinterpret_cast<const char *>(toc.data.Get()));
    }
  };

  CatchClassesLambda(ighw, CatchFileName);

  GLTFModel main;
  std::map<Hash, uint32> materialRemaps;

  ShrubToGltf(main, shaders, ighw, shdStream, materialRemaps);

  main.FinishAndSave(ctx->NewFile(shrubPath.ChangeExtension2("glb")).str, "");
}

size_t FoliageToGltf(GLTFModel &main,
                     IGHWTOCIteratorConst<ResourceShaders> &shaders, IGHW &ighw,
                     AppContextStream &shdStream,
                     std::map<Hash, uint32> &materialRemaps) {
  IGHWTOCIteratorConst<FoliageV2> foliages;
  IGHWTOCIteratorConst<FoliageV2Buffer> buffer;
  IGHWTOCIteratorConst<ShaderResourceLookup> shaderLookups;
  CatchClasses(ighw, foliages, buffer, shaderLookups);
  ShadersToGltf(main, shaderLookups, shaders, shdStream, materialRemaps);

  assert(std::distance(foliages.begin(), foliages.end()) == 1);

  const FoliageV2 *foliage = foliages.begin();

  main.scenes.front().nodes.emplace_back(main.nodes.size());
  const size_t folNodeIndex = main.nodes.size();
  gltf::Node &glFoliageNode = main.nodes.emplace_back();

  const SpriteV2Vertex *corners =
      reinterpret_cast<const SpriteV2Vertex *>(&buffer.at(0).data);
  const FoliageV2Vertex *positions = reinterpret_cast<const FoliageV2Vertex *>(
      &buffer.at(0).data + foliage->spriteVertexOffset);

  glFoliageNode.mesh = main.meshes.size();
  gltf::Mesh &glMesh = main.meshes.emplace_back();

  struct SpriteVertexOut {
    Vector position;
    t_Vector2<float16> uv;
  };

  std::vector<SpriteVertexOut> outVerts;

  // for (uint32 l = 0; l < foliage->numUsedLods; l++) {
  const SpriteV2LodRange &lod = foliage->spriteLodRanges[0];

  for (uint32 vtIndex = lod.cornerBegin; vtIndex < lod.cornerEnd; vtIndex++) {
    SpriteV2Vertex vtx = corners[vtIndex];
    FByteswapper(vtx);
    FoliageV2Vertex position = positions[vtIndex / 4];
    FByteswapper(position);
    Vector2 size = vtx.size.Convert<float>();
    SpriteVertexOut out{
        .position =
            (position.position.Convert<float>() + Vector(size.x, size.y, 0)) *
            YARD_TO_M,
        .uv = vtx.uv,
    };

    outVerts.emplace_back(out);
  }

  uint32 numQuads = (lod.cornerEnd - lod.cornerBegin) / 4;
  std::vector<uint16> indices;
  for (uint32 q = 0; q < numQuads; q++) {
    indices.emplace_back(q * 4);
    indices.emplace_back(q * 4 + 1);
    indices.emplace_back(q * 4 + 2);
    indices.emplace_back(q * 4 + 3);
    indices.emplace_back(q * 4);
    indices.emplace_back(q * 4 + 2);
  }

  auto &prim = glMesh.primitives.emplace_back();
  prim.indices = main.SaveIndices(indices.data(), indices.size()).accessorIndex;
  prim.material = materialRemaps.at(shaderLookups.at(0).hash);

  Attribute attrs[]{
      {
          .type = uni::DataType::R32G32B32,
          .format = uni::FormatType::FLOAT,
          .usage = AttributeType::Position,
      },
      {
          .type = uni::DataType::R16G16,
          .format = uni::FormatType::FLOAT,
          .usage = AttributeType::TextureCoordiante,
      },
  };

  prim.attributes = main.SaveVertices(outVerts.data(), outVerts.size(), attrs,
                                      sizeof(SpriteVertexOut));
  //}

  return folNodeIndex;
}

void FoliageToGltf(IGHWTOCIteratorConst<ResourceShaders> &shaders, IGHW &ighw,
                   AppContext *ctx, AppContextStream &shdStream,
                   AFileInfo path) {
  GLTFModel main;
  std::map<Hash, uint32> materialRemaps;
  FoliageToGltf(main, shaders, ighw, shdStream, materialRemaps);

  main.FinishAndSave(ctx->NewFile(path.ChangeExtension2("glb")).str, "");
}

void Instantiate(IMGLTF &main, gltf::Node &glNode,
                 std::vector<es::Matrix44> &tms) {
  if (tms.size() == 1) {
    memcpy(glNode.matrix.data(), tms.data(), 64);
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

void GatherRegionTies(IMGLTF &main, AppContext *ctx,
                      IGHWTOCIteratorConst<ResourceShaders> &shaders,
                      AppContextStream &shdStream,
                      IGHWTOCIteratorConst<ResourceTies> ties,
                      IGHWTOCIteratorConst<TieInstanceV2> tieInstances,
                      IGHWTOCIteratorConst<ZoneTieLookup> tieLookups,
                      const std::string &workDir) {
  for (auto &inst : tieInstances) {
    es::Matrix44 tm(inst.tm);
    tm.r4() *= YARD_TO_M;
    tm.r1().w = 0;
    tm.r2().w = 0;
    tm.r3().w = 0;
    tm.r4().w = 1;
    main.ties[tieLookups.at(inst.tieIndex).hash].tms.emplace_back(tm);
  }

  auto tieStream = ctx->RequestFile(workDir + "ties.dat");
  BinReaderRef_e subRd(*tieStream.Get());

  for (auto &tie : tieLookups) {
    if (main.ties[tie.hash].nodeIndex > -1) {
      continue;
    }
    auto foundTie = std::find(ties.begin(), ties.end(), tie.hash);
    subRd.SetRelativeOrigin(foundTie->offset);
    IGHW tieData;
    tieData.FromStream(subRd, Version::V2);
    const size_t nodeIndex =
        TieToGltf(main, shaders, tieData, shdStream, main.materialRemaps);
    main.ties[tie.hash].nodeIndex = nodeIndex;
  }
}

void GatherRegionShrubs(IMGLTF &main, AppContext *ctx,
                        IGHWTOCIteratorConst<ResourceShaders> &shaders,
                        AppContextStream &shdStream,
                        IGHWTOCIteratorConst<ResourceShrubs> shrubs,
                        IGHWTOCIteratorConst<ShrubV2Instance> shrubInstances,
                        IGHWTOCIteratorConst<ZoneShrubLookup> shrubLookups,
                        const std::string &workDir) {
  for (auto &inst : shrubInstances) {
    es::Matrix44 tm;
    tm.r1() = inst.r1;
    tm.r2() = inst.r2;
    tm.r3() = tm.r1().Cross(tm.r2());
    tm.r1() *= inst.scale;
    tm.r2() *= inst.scale;
    tm.r3() *= inst.scale;
    tm.r4() = inst.position * YARD_TO_M;
    tm.r4().w = 1;
    tm.Transpose();
    main.shrubs[shrubLookups.at(inst.shrubIndex).hash].tms.emplace_back(tm);
  }

  auto shrubStream = ctx->RequestFile(workDir + "shrubs.dat");
  BinReaderRef_e subRd(*shrubStream.Get());

  for (auto &shrub : shrubLookups) {
    if (main.shrubs[shrub.hash].nodeIndex > -1) {
      continue;
    }
    auto foundShrub = std::find(shrubs.begin(), shrubs.end(), shrub.hash);
    subRd.SetRelativeOrigin(foundShrub->offset);
    IGHW tieData;
    tieData.FromStream(subRd, Version::V2);
    const size_t nodeIndex =
        ShrubToGltf(main, shaders, tieData, shdStream, main.materialRemaps);
    main.shrubs[shrub.hash].nodeIndex = nodeIndex;
  }
}

void GatherRegionFoliages(
    IMGLTF &main, AppContext *ctx,
    IGHWTOCIteratorConst<ResourceShaders> &shaders, AppContextStream &shdStream,
    IGHWTOCIteratorConst<ResourceFoliages> foliages,
    IGHWTOCIteratorConst<FoliageV2Instance> foliageInstances,
    IGHWTOCIteratorConst<ZoneFoliageLookup> foliageLookups,
    const std::string &workDir) {
  for (auto &inst : foliageInstances) {
    es::Matrix44 tm = inst.tm;
    tm.r4() *= YARD_TO_M;
    tm.r4().w = 1;
    tm.Transpose();
    main.foliages[foliageLookups.at(inst.foliageIndex).hash].tms.emplace_back(
        tm);
  }

  auto foliageStream = ctx->RequestFile(workDir + "foliages.dat");
  BinReaderRef_e subRd(*foliageStream.Get());

  for (auto &foliage : foliageLookups) {
    if (main.foliages[foliage.hash].nodeIndex > -1) {
      continue;
    }
    auto foundFoliage =
        std::find(foliages.begin(), foliages.end(), foliage.hash);
    subRd.SetRelativeOrigin(foundFoliage->offset);
    IGHW tieData;
    tieData.FromStream(subRd, Version::V2);
    const size_t nodeIndex =
        FoliageToGltf(main, shaders, tieData, shdStream, main.materialRemaps);
    main.foliages[foliage.hash].nodeIndex = nodeIndex;
  }
}

void RegionToGltf(IMGLTF &main, IGHW &ighw,
                  IGHWTOCIteratorConst<ResourceShaders> &shaders,
                  AppContextStream &shdStream,
                  IGHWTOCIteratorConst<ResourceTies> ties,
                  IGHWTOCIteratorConst<ResourceShrubs> shrubs,
                  IGHWTOCIteratorConst<ResourceFoliages> foliages,
                  AppContext *ctx, const std::string &workDir) {
  IGHWTOCIteratorConst<RegionMeshV2> meshes;
  IGHWTOCIteratorConst<RegionVertexBuffer> vtxBuffer;
  IGHWTOCIteratorConst<RegionIndexBuffer> idxBuffer;
  IGHWTOCIteratorConst<ZoneShaderLookup> shaderLookups;
  IGHWTOCIteratorConst<TieInstanceV2> tieInstances;
  IGHWTOCIteratorConst<ZoneTieLookup> tieLookups;
  IGHWTOCIteratorConst<ZoneShrubLookup> shrubLookups;
  IGHWTOCIteratorConst<ShrubV2Instance> shrubInstances;
  IGHWTOCIteratorConst<FoliageV2Instance> foliageInstances;
  IGHWTOCIteratorConst<ZoneFoliageLookup> foliageLookups;
  CatchClasses(ighw, meshes, vtxBuffer, idxBuffer, shaders, tieInstances,
               tieLookups, shrubLookups, shrubInstances, shaderLookups,
               foliageInstances, foliageLookups);
  ShadersToGltf(main, shaderLookups, shaders, shdStream, main.materialRemaps);

  if (meshes.Valid()) {
    const uint16 *indexBuffer = &idxBuffer.at(0).data;
    const char *vertexBuffer = &vtxBuffer.at(0).data;

    main.scenes.front().nodes.emplace_back(main.nodes.size());
    gltf::Node &glNode = main.nodes.emplace_back();
    glNode.mesh = main.meshes.size();
    glNode.name = "RegionMesh";
    gltf::Mesh &glMesh = main.meshes.emplace_back();

    for (const RegionMeshV2 &item : meshes) {
      gltf::Primitive &glPrim = glMesh.primitives.emplace_back();
      glPrim.material =
          main.materialRemaps.at(shaderLookups.at(item.materialIndex).hash);

      const uint16 *indices = indexBuffer + item.indexOffset / 2;
      const RegionVertexV2 *vertices = reinterpret_cast<const RegionVertexV2 *>(
          vertexBuffer + item.vertexOffset);

      std::vector<RegionVertexV2> vtx0(vertices, vertices + item.numVerties);

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

      glPrim.attributes = main.SaveVertices(vtx0.data(), vtx0.size(), attrs,
                                            sizeof(RegionVertexV2));

      std::vector<uint16> idx(indices, indices + item.numIndices);
      for (uint16 &i : idx) {
        FByteswapper(i);
      }

      glPrim.indices = main.SaveIndices(idx.data(), idx.size()).accessorIndex;
    }
  }

  if (tieInstances.Valid()) {
    GatherRegionTies(main, ctx, shaders, shdStream, ties, tieInstances,
                     tieLookups, workDir);
  }

  if (shrubInstances.Valid()) {
    GatherRegionShrubs(main, ctx, shaders, shdStream, shrubs, shrubInstances,
                       shrubLookups, workDir);
  }

  if (foliageInstances.Valid()) {
    GatherRegionFoliages(main, ctx, shaders, shdStream, foliages,
                         foliageInstances, foliageLookups, workDir);
  }
}

void GenerateInstances(IMGLTF &main) {
  for (auto &[_, tie] : main.ties) {
    Instantiate(main, main.nodes.at(tie.nodeIndex), tie.tms);
  }

  for (auto &[_, shrub] : main.shrubs) {
    Instantiate(main, main.nodes.at(shrub.nodeIndex), shrub.tms);
  }

  for (auto &[_, foliage] : main.foliages) {
    Instantiate(main, main.nodes.at(foliage.nodeIndex), foliage.tms);
  }
}

void RegionToGltf(IGHW &ighw, AppContext *ctx,
                  IGHWTOCIteratorConst<ResourceShaders> &shaders,
                  AppContextStream &shdStream,
                  IGHWTOCIteratorConst<ResourceTies> ties,
                  IGHWTOCIteratorConst<ResourceShrubs> shrubs,
                  IGHWTOCIteratorConst<ResourceFoliages> foliages,
                  AFileInfo zonePath) {
  IMGLTF main;
  RegionToGltf(main, ighw, shaders, shdStream, ties, shrubs, foliages, ctx,
               std::string(ctx->workingFile.GetFolder()));
  GenerateInstances(main);

  main.FinishAndSave(ctx->NewFile(zonePath.ChangeExtension2("glb")).str, "");
}
