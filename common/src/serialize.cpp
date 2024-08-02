/*  InsomniaLib
    Copyright(C) 2021-2024 Lukas Cone

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
#include "spike/except.hpp"
#include "spike/io/binreader_stream.hpp"
#include <set>

template <class C>
void fixupper(CoreClass *data, bool way, std::set<void *> &swapped) {
  if (swapped.contains(data)) {
    return;
  }

  swapped.emplace(data);
  FByteswapper(*static_cast<C *>(data), way);
}

template <> void FByteswapper(Hash &input, bool) {
  FByteswapper(input.part1);
  FByteswapper(input.part2);
}

template <uint32 id_> void FByteswapper(ResourceLookup<id_> &input, bool) {
  FByteswapper(input.hash);
  FByteswapper(input.offset);
  FByteswapper(input.size);
}

template <> void FByteswapper(IGHWHeader &input, bool) {
  FByteswapper(input.id);
  FByteswapper(input.numFixups);
  FByteswapper(input.numToc);
  FByteswapper(input.tocEnd);
  FByteswapper(input.versionMajor);
  FByteswapper(input.versionMinor);
  FByteswapper(input.dataEnd);
}

template <> void FByteswapper(IGHWTOC &input, bool way) {
  FByteswapper(input.id);
  FByteswapper(input.size);
  FByteswapper(input.data);
  FByteswapper(input.count.value, way);
}

template <> void FByteswapper(TextureResource &input, bool) {
  FByteswapper(input.hash);
  FByteswapper(input.totalSize);
}

template <> void FByteswapper(Material &input, bool) {
  FByteswapper(input.detailMapId);
  FByteswapper(input.diffuseMapId);
  FByteswapper(input.normalMapId);
  FByteswapper(input.specularMapId);
  FByteswapper(input.values);
}

template <> void FByteswapper(MaterialV1_5 &input, bool) {
  FByteswapper(input.values);
}

template <> void FByteswapper(Texture &input, bool way) {
  FByteswapper(input.address, way);
  FByteswapper(input.borderColor);
  FByteswapper(input.control0, way);
  FByteswapper(input.control3, way);
  FByteswapper(input.filter);
  FByteswapper(input.flags, way);
  FByteswapper(input.height);
  FByteswapper(input.numMips);
  FByteswapper(input.offset);
  FByteswapper(input.width);
}

template <> void FByteswapper(DirectionalLightmapTextureV1 &input, bool way) {
  FByteswapper<Texture>(input, way);
}

template <> void FByteswapper(TextureV1 &input, bool way) {
  FByteswapper<Texture>(input, way);
}

template <> void FByteswapper(BlendmapTextureV1 &input, bool way) {
  FByteswapper<Texture>(input, way);
}

template <> void FByteswapper(LightmapTexture &input, bool way) {
  FByteswapper<Texture>(input, way);
}

template <> void FByteswapper(ShadowmapTexture &input, bool way) {
  FByteswapper<Texture>(input, way);
}

template <> void FByteswapper(MaterialResourceNameLookup &input, bool) {
  FByteswapper(input.mapHashes);
  FByteswapper(input.hash);
}

template <> void FByteswapper(ShaderResourceLookup &input, bool) {
  FByteswapper(input.hash);
}

template <> void FByteswapper(ZoneHash &input, bool) {
  FByteswapper(input.hash);
}

template <> void FByteswapper(ZoneNameLookup &, bool) {}

template <> void FByteswapper(ZoneLightmap &input, bool way) {
  FByteswapper(static_cast<Texture &>(input), way);
}

template <> void FByteswapper(ZoneShadowMap &input, bool way) {
  FByteswapper(static_cast<Texture &>(input), way);
}

template <> void FByteswapper(ZoneDataLookup &input, bool) {
  FByteswapper(input.hash);
}

template <> void FByteswapper(ZoneData2Lookup &input, bool) {
  FByteswapper(input.hash);
}

template <> void FByteswapper(ZoneData &input, bool) {
  FByteswapper(input.transform);
  FByteswapper(input.unk0);
  FByteswapper(input.mainDataOffset);
  FByteswapper(input.lightMapId);
  FByteswapper(input.unk1);
  FByteswapper(input.unk);
}

template <> void FByteswapper(ZoneMap &input, bool way) {
  FByteswapper(static_cast<Texture &>(input), way);
}

template <> void FByteswapper(Bone &input, bool) {
  FByteswapper(input.unk);
  FByteswapper(input.parentIndex);
  FByteswapper(input.child);
  FByteswapper(input.parentChild);
}

template <> void FByteswapper(Skeleton &input, bool) {
  FByteswapper(input.numBones);
  FByteswapper(input.rootBone);
  FByteswapper(input.unk0);
  FByteswapper(input.unk1);

  for (uint32 i = 0; i < input.numBones; i++) {
    FByteswapper(input.bones[i]);
  }

  for (uint32 i = 0; i < input.numBones; i++) {
    FByteswapper(input.tms0[i]);
  }

  for (uint32 i = 0; i < input.numBones; i++) {
    FByteswapper(input.tms1[i]);
  }
}

template <> void FByteswapper(PrimitiveV2 &input, bool) {
  FByteswapper(input.indexOffset);
  FByteswapper(input.vertexOffset);
  FByteswapper(input.materialIndex);
  FByteswapper(input.numVertices);
  FByteswapper(input.numIndices);
  FByteswapper(input.unk5);
  FByteswapper(input.unk6);
  FByteswapper(input.unk1);
  FByteswapper(input.unk2);
  FByteswapper(input.unk3);
}

template <> void FByteswapper(PrimitiveV1 &input, bool) {
  FByteswapper(input.materialIndex);
  FByteswapper(input.numVertices);
  FByteswapper(input.numIndices);
  FByteswapper(input.indexOffset);
  FByteswapper(input.vertexBufferOffset);
  FByteswapper(input.unk);
}

template <> void FByteswapper(Mesh &input, bool) {
  FByteswapper(input.numPrimitives);
}

template <> void FByteswapper(MobyV2 &input, bool) {
  FByteswapper(input.unk00);
  FByteswapper(input.unk01);
  FByteswapper(input.numMeshes);
  FByteswapper(input.unk11);
  FByteswapper(input.numBones);
  FByteswapper(input.unk13);
  FByteswapper(input.null00);
  FByteswapper(input.unk011);
  FByteswapper(input.unk02);
  FByteswapper(input.unk032);
  FByteswapper(input.animset);
  FByteswapper(input.null02);
  FByteswapper(input.unk031);
  FByteswapper(input.meshScale);
  FByteswapper(input.unk04);
  FByteswapper(input.unk05);
  FByteswapper(input.unk06);
  FByteswapper(input.moby);
  FByteswapper(input.unk07);
  FByteswapper(input.null01);

  if (input.skeleton) {
    FByteswapper(*input.skeleton);
  }

  for (uint32 i = 0; i < input.numMeshes; i++) {
    FByteswapper(input.meshes[i]);
  }
}

template <> void FByteswapper(MeshV1 &input, bool) {
  FByteswapper(input.numPrimitives);
}

template <> void FByteswapper(MobyV1 &input, bool) {
  FByteswapper(input.unk00);
  FByteswapper(input.unk01);
  FByteswapper(input.unk02);
  FByteswapper(input.numBones);
  FByteswapper(input.unk03);
  FByteswapper(input.numMeshes);
  FByteswapper(input.mobyId);
  FByteswapper(input.null00);
  FByteswapper(input.null02);
  FByteswapper(input.indexBufferOffset);
  FByteswapper(input.vertexBufferOffset);
  FByteswapper(input.meshScale);

  const uint32 numMeshes = input.numMeshes * (input.anotherSet + 1);

  for (uint32 i = 0; i < numMeshes; i++) {
    FByteswapper(input.meshes[i]);
  }
}

template <>
void fixupper<MobyV1>(CoreClass *data, bool way, std::set<void *> &swapped) {
  if (swapped.contains(data)) {
    return;
  }

  swapped.emplace(data);

  MobyV1 &input = *static_cast<MobyV1 *>(data);

  FByteswapper(input, false);
  fixupper<Skeleton>(input.skeleton, way, swapped);
}

template <> void FByteswapper(TiePrimitiveV2 &input, bool) {
  FByteswapper(input.indexOffset);
  FByteswapper(input.vertexOffset0);
  FByteswapper(input.vertexOffset1);
  FByteswapper(input.numVertices);
  FByteswapper(input.unk000);
  FByteswapper(input.numIndices);
  FByteswapper(input.unk0);
  FByteswapper(input.unk08);
  FByteswapper(input.unk1);
  FByteswapper(input.materialIndex);
  FByteswapper(input.unk06);
  FByteswapper(input.unk07);
  FByteswapper(input.unk2);
  FByteswapper(input.unk3);
}

template <> void FByteswapper(TieV2 &input, bool) {
  FByteswapper(input.unk00);
  FByteswapper(input.numPrimitives);
  FByteswapper(input.unk01);
  FByteswapper(input.unk02);
  FByteswapper(input.unk13);
  FByteswapper(input.unk14);
  FByteswapper(input.meshScale);
  FByteswapper(input.unk03);
  FByteswapper(input.unk04);
}

template <> void FByteswapper(TiePrimitiveV1 &input, bool) {
  FByteswapper(input.materialIndex);
  FByteswapper(input.unk1);
  FByteswapper(input.indexOffset);
  FByteswapper(input.numIndices);
  FByteswapper(input.numVertices);
  FByteswapper(input.vertexOffset0);
  FByteswapper(input.vertexOffset1);
  FByteswapper(input.unk);
}

template <> void FByteswapper(TieV1 &input, bool) {
  FByteswapper(input.numMeshes);
  FByteswapper(input.unk01);
  FByteswapper(input.unk02);
  FByteswapper(input.unk13);
  FByteswapper(input.unk14);
  FByteswapper(input.offset0);
  FByteswapper(input.null00);
  FByteswapper(input.meshScale);
  FByteswapper(input.unk03);
}

template <> void FByteswapper(TieInstanceV1 &input, bool) {
  FByteswapper(input.tm);
  FByteswapper(input.unk0);
  FByteswapper(input.unk1);
  FByteswapper(input.unk);
}

template <> void FByteswapper(RegionMesh &input, bool) {
  FByteswapper(input.unk);
  FByteswapper(input.position);
  FByteswapper(input.materialIndex);
  FByteswapper(input.unk6);
  FByteswapper(input.unk3);
  FByteswapper(input.indexOffset);
  FByteswapper(input.vertexOffset);
  FByteswapper(input.numIndices);
  FByteswapper(input.numVerties);
  FByteswapper(input.unk2);
  FByteswapper(input.meshScale);
  FByteswapper(input.unk4);
}

template <> void FByteswapper(MaterialV1 &input, bool) {
  FByteswapper(input.unk0);
  FByteswapper(input.unk1);
  FByteswapper(input.values);
}

template <> void FByteswapper(ShrubPrimitive &input, bool) {
  FByteswapper(input.materialIndex);
  FByteswapper(input.unk1);
  FByteswapper(input.numVertices);
  FByteswapper(input.numIndices);
  FByteswapper(input.vertexBufferOffset);
  FByteswapper(input.indexOffset);
  FByteswapper(input.unk4);
}

template <> void FByteswapper(Shrub &input, bool) {
  FByteswapper(input.unk0);
  FByteswapper(input.numPrimitives);
  FByteswapper(input.null00);
  FByteswapper(input.unk1);
  FByteswapper(input.meshScale);
  FByteswapper(input.unk2);
}

template <> void FByteswapper(ShrubInstance &input, bool) {
  FByteswapper(input.tm);
  FByteswapper(input.unk0);
  FByteswapper(input.unk1);
}

template <> void FByteswapper(FoliageBranchLod &input, bool) {
  FByteswapper(input.indexOffset);
  FByteswapper(input.numIndices);
  FByteswapper(input.unk);
}

template <> void FByteswapper(SpriteRange &input, bool) {
  FByteswapper(input.indexBegin);
  FByteswapper(input.indexEnd);
  FByteswapper(input.positionsOffset);
  FByteswapper(input.numSprites);
}

template <> void FByteswapper(SpriteLodRange &input, bool) {
  FByteswapper(input.indexBegin);
  FByteswapper(input.indexEnd);
  FByteswapper(input.unk0);
  FByteswapper(input.unk1);
}

template <> void FByteswapper(Foliage &input, bool) {
  FByteswapper(input.textureIndex);
  FByteswapper(input.indexOffset);
  FByteswapper(input.null0);
  FByteswapper(input.branchVertexOffset);
  FByteswapper(input.unk1);
  FByteswapper(input.branchLods);
  FByteswapper(input.spriteVertexOffset);
  FByteswapper(input.usedSpriteLods);
  FByteswapper(input.spriteLodRanges);
  FByteswapper(input.unk2);
  FByteswapper(input.usedSpriteRanges);
  FByteswapper(input.spriteRanges);
  FByteswapper(input.unk3);
}

struct FoliageSpritePositions : CoreClass {
  static constexpr uint32 ID = 0x9150;
  Vector4A16 data;
};

struct NavmeshPositions : CoreClass {
  static constexpr uint32 ID = 0x14100;
  Vector4A16 data;
};

struct NavmeshPositions2 : CoreClass {
  static constexpr uint32 ID = 0x14900;
  Vector4A16 data;
};

template <> void FByteswapper(FoliageSpritePositions &input, bool) {
  FByteswapper(input.data);
}

template <> void FByteswapper(NavmeshPositions &input, bool) {
  FByteswapper(input.data);
}

template <> void FByteswapper(NavmeshPositions2 &input, bool) {
  FByteswapper(input.data);
}

template <> void FByteswapper(FoliageInstance &input, bool) {
  FByteswapper(input.tm);
  FByteswapper(input.unk0);
  FByteswapper(input.unk1);
  FByteswapper(input.unk);
}

template <> void FByteswapper(HighmipTextureData &input, bool) {
  FByteswapper(input.dataOffset);
  FByteswapper(input.dataSize);
  FByteswapper(input.textureIndex);
  FByteswapper(input.unk0);
  FByteswapper(input.unk2);
  FByteswapper(input.null0);
}

template <> void FByteswapper(TieV1_5 &input, bool) {
  FByteswapper(input.unk00);
  FByteswapper(input.numMeshes);
  FByteswapper(input.unk01);
  FByteswapper(input.vertexBufferOffset0);
  FByteswapper(input.vertexBufferOffset1);
  FByteswapper(input.unk13);
  FByteswapper(input.unk14);
  FByteswapper(input.meshScale);
  FByteswapper(input.unk03);
  FByteswapper(input.unk04);
}

template <> void FByteswapper(TieInstanceV2 &input, bool) {
  FByteswapper(input.tm);
  FByteswapper(input.unk0);
  FByteswapper(input.unk5);
  FByteswapper(input.unk2);
  FByteswapper(input.unk3);
}

template <> void FByteswapper(RegionMeshV2 &input, bool) {
  FByteswapper(input.unk);
  FByteswapper(input.indexOffset);
  FByteswapper(input.vertexOffset);
  FByteswapper(input.numIndices);
  FByteswapper(input.numVerties);
  FByteswapper(input.materialIndex);
  FByteswapper(input.unk6);
  FByteswapper(input.unk2);
  FByteswapper(input.position);
  FByteswapper(input.unk4);
  FByteswapper(input.unk5);
  FByteswapper(input.unk7);
}

template <> void FByteswapper(PlantPrimitive &item, bool) {
  FByteswapper(item.vertexBufferOffset);
  FByteswapper(item.indexOffset);
  FByteswapper(item.numIndices);
  FByteswapper(item.unk0);
  FByteswapper(item.unk1);
  FByteswapper(item.materialIndex);
  FByteswapper(item.unk2);
}

template <> void FByteswapper(PlantClusters &item, bool) {
  FByteswapper(item.unk0);
  FByteswapper(item.unkOffset);
  FByteswapper(item.numInstances);
  FByteswapper(item.instancesOffset);
  FByteswapper(item.unk4);
  FByteswapper(item.unk5);
  FByteswapper(item.null0);
  FByteswapper(item.unk6);
  FByteswapper(item.unk7);
  FByteswapper(item.null1);
  FByteswapper(item.unk8);
}

template <> void FByteswapper(PlantClusterInstance &item, bool) {
  FByteswapper(item.tm);
  FByteswapper(item.unk0);
  FByteswapper(item.unk1);
  FByteswapper(item.unk2);
}

struct ClassInfo {
  uint32 id;
  uint16 size;
  bool openEnded;
  void (*swap)(CoreClass *, bool, std::set<void *> &);
};

template <class T>
using is_open_ended = decltype(std::declval<T>().OpenEnded());
template <class C>
constexpr static bool is_open_ended_v = es::is_detected_v<is_open_ended, C>;

template <class... C> auto RegisterClasses() {
  return std::vector<ClassInfo>{
      {C::ID, sizeof(C), is_open_ended_v<C>, fixupper<C>}...};
}

static const std::vector<ClassInfo> FIXUPS[]{
    RegisterClasses<
        MobyV1, PrimitiveV1, TieV1, TiePrimitiveV1, TieInstanceV1, RegionMesh,
        DirectionalLightmapTextureV1, TextureV1, BlendmapTextureV1, MaterialV1,
        ShrubPrimitive, Shrub, ShrubInstance, Foliage, FoliageSpritePositions,
        FoliageInstance, NavmeshPositions, NavmeshPositions2, PlantPrimitive>(),
    RegisterClasses<MaterialV1_5, Texture, MaterialResourceNameLookup, MobyV1,
                    PrimitiveV2, TiePrimitiveV2, HighmipTextureData,
                    LightmapTexture, ShadowmapTexture, TieV1_5, TieInstanceV2,
                    RegionMeshV2>(),
    RegisterClasses<
        ResourceLighting, ResourceZones, ResourceAnimsets, ResourceMobys,
        ResourceShrubs, ResourceTies, ResourceFoliages, ResourceCubemap,
        ResourceShaders, ResourceHighmips, ResourceTextures, ResourceCinematics,
        TextureResource, Material, Texture, MaterialResourceNameLookup,
        ShaderResourceLookup, ZoneHash, ZoneNameLookup, ZoneLightmap,
        ZoneShadowMap, ZoneDataLookup, ZoneData2Lookup, ZoneData, ZoneMap,
        MobyV2, PrimitiveV2, TieV2, TiePrimitiveV2>(),
};

void IGHW::FromStream(BinReaderRef_e rd, Version version) {
  rd.SwapEndian(true);
  IGHWHeader hdr;
  rd.Push();
  rd.Read(hdr);
  rd.Pop();

  if (hdr.id != hdr.ID) {
    throw es::InvalidHeaderError(hdr.id);
  }

  if (hdr.DEADDEAD == 0xDEADDEAD) {
    throw es::InvalidHeaderError();
  }

  rd.ReadContainer(buffer, hdr.dataEnd);

  if (hdr.numFixups) {
    std::vector<uint32> fixups;
    rd.ReadContainer(fixups, hdr.numFixups);

    for (auto f : fixups) {
      f &= 0xfffffff;
      auto ptr = reinterpret_cast<es::PointerX86<uint32> *>(&buffer[0] + f);
      FByteswapper(*ptr);
      ptr->Fixup(buffer.data());
    }
  }

  FByteswapper(*Header());

  std::set<void *> swapped;

  for (auto &item : *this) {
    FByteswapper(item, false);
    item.data.Fixup(buffer.data());
    auto &fixups = FIXUPS[int(version)];
    auto found =
        std::find_if(fixups.begin(), fixups.end(), [&](const ClassInfo &cls) {
          if (cls.id != item.id) {
            return false;
          }

          if (item.count.ArrayType() == IGHWTOCArrayType::Array) {
            return item.size == cls.size;
          }

          return true;
        });

    if (!es::IsEnd(fixups, found)) {
      char *start = reinterpret_cast<char *>(item.data.operator->());
      char *end = nullptr;

      switch (item.count.ArrayType()) {
      case IGHWTOCArrayType::Buffer:
        end = reinterpret_cast<char *>(start) + item.size;
        break;
      case IGHWTOCArrayType::Array:
        end =
            reinterpret_cast<char *>(start) + item.count.Count() * found->size;
        break;

      default:
        throw std::runtime_error("Unknown array type");
      }

      while (start < end) {
        found->swap(reinterpret_cast<CoreClass *>(start), false, swapped);
        start += found->size;

        if (found->openEnded) {
          break;
        }
      }
    }
  }
}
