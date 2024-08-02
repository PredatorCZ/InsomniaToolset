#include "spike/gltf.hpp"
#include "insomnia/insomnia.hpp"
#include "insomnia/internal/vertex.hpp"
#include "spike/app_context.hpp"
#include "spike/io/binreader_stream.hpp"

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
    const Mesh &mesh = moby->meshes[i];
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
    const Mesh &mesh = moby->meshes[i];
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

void TieToGltf(IGHWTOCIteratorConst<ResourceShaders> &shaders, IGHW &ighw,
               AppContext *ctx, AppContextStream &shdStream) {
  IGHWTOCIteratorConst<TieV2> ties;
  IGHWTOCIteratorConst<TieVertexBuffer> vertexBuffers;
  IGHWTOCIteratorConst<TieIndexBuffer> indexBuffers;
  IGHWTOCIteratorConst<ShaderResourceLookup> shaderLookups;
  AFileInfo mobyPath;

  auto CatchFileName = [&mobyPath, ctx](const IGHWTOC &toc) {
    if (toc.id == ResourceTiePathLookupId) {
      mobyPath.Load(std::string(ctx->workingFile.GetFolder()) +
                    reinterpret_cast<const char *>(toc.data.Get()));
    }
  };

  CatchClassesLambda(ighw, CatchFileName, ties, vertexBuffers, indexBuffers,
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

  const TieV2 *tie = ties.begin();
  const uint16 *indexBuffer = &indexBuffers.begin()->data;
  const char *vertexBuffer = &vertexBuffers.begin()->data;
  std::map<uint16, uint16> joints;

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
  } attributeMul{tie->meshScale};

  main.scenes.front().nodes.emplace_back(main.nodes.size());
  gltf::Node &glNode = main.nodes.emplace_back();
  glNode.mesh = main.meshes.size();
  glNode.name = "TieMesh";
  gltf::Mesh &glMesh = main.meshes.emplace_back();

  for (uint32 p = 0; p < tie->numPrimitives; p++) {
    const TiePrimitiveV2 &prim = tie->primitives[p];
    gltf::Primitive &glPrim = glMesh.primitives.emplace_back();
    glPrim.material = prim.materialIndex;

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

  main.FinishAndSave(ctx->NewFile(mobyPath.ChangeExtension2("glb")).str, "");
}
