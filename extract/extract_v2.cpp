/*  InsomniaToolset AssetExtract
    Copyright(C) 2021 Lukas Cone

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
#include "project.h"
#include "pugixml.hpp"
#include "spike/app_context.hpp"
#include "spike/except.hpp"
#include "spike/io/binreader_stream.hpp"
#include "spike/master_printer.hpp"
#include "spike/reflect/reflector.hpp"
#include "spike/type/flags.hpp"

MAKE_ENUM(ENUMSCOPE(class Filter, Filter), EMEMBER(Mobys), EMEMBER(Ties),
          EMEMBER(Shrubs), EMEMBER(Foliages), EMEMBER(Zones), EMEMBER(Textures),
          EMEMBER(Shaders), EMEMBER(Cinematics), EMEMBER(Animsets),
          EMEMBER(Cubemaps))

static struct AssetExtract : ReflectorBase<AssetExtract> {
  bool convertShaders = true;
  es::Flags<Filter> extractFilter{0xffffu};
} settings;

REFLECT(CLASS(AssetExtract),
        MEMBERNAME(convertShaders, "convert-shaders", "s",
                   ReflDesc{"Convert shaders into XML format."}),
        MEMBERNAME(extractFilter, "extract-filter", "e",
                   ReflDesc{"Select groups that should be extracted."}), );

std::string_view filters[]{
    "^assetlookup.dat$",
};

static AppInfo_s appInfo{
    .header = AssetExtract_DESC " v" AssetExtract_VERSION
                                ", " AssetExtract_COPYRIGHT "Lukas Cone",
    .settings = reinterpret_cast<ReflectorFriend *>(&settings),
    .filters = filters,
};

AppInfo_s *AppInitModule() { return &appInfo; }

template <> void FByteswapper(IGHWHeader &input, bool) {
  FByteswapper(input.id);
  FByteswapper(input.numToc);
  FByteswapper(input.dataEnd);
}

template <> void FByteswapper(IGHWTOC &input, bool) {
  FByteswapper(input.id);
  FByteswapper(input.data);
}

bool IGHWSeekClass(BinReaderRef_e rd, uint32 classId) {
  rd.SwapEndian(true);
  IGHWHeader hdr;
  rd.Read(hdr);

  if (hdr.id != hdr.ID) {
    throw es::InvalidHeaderError(hdr.id);
  }

  if (hdr.DEADDEAD == 0xDEADDEAD) {
    throw es::InvalidHeaderError();
  }

  for (size_t i = 0; i < hdr.numToc; i++) {
    IGHWTOC toc;
    rd.Read(toc);

    if (toc.id == classId) {
      rd.Seek(reinterpret_cast<uint32 &>(toc.data));
      return true;
    }
  }

  return false;
}

struct TextureCache {
  std::string path;
  Texture data;
};

using TextureRegistry = std::map<uint32, TextureCache>;

struct XMLContextWritter : pugi::xml_writer {
  XMLContextWritter(AppExtractContext *ctx_) : ctx(ctx_) {}
  void write(const void *data, size_t size) override {
    ctx->SendData({static_cast<const char *>(data), size});
  }
  AppExtractContext *ctx;
};

void ExtractShaders(AppContext *ctx,
                    IGHWTOCIteratorConst<ResourceShaders> &shaders,
                    TextureRegistry &reg) {
  auto stream = ctx->RequestFile("shaders.dat");
  IGHW main;

  for (auto &item : shaders) {
    const char *shaderPath = nullptr;
    BinReaderRef_e rd(*stream.Get());
    rd.SetRelativeOrigin(item.offset);
    main.FromStream(rd, Version::V2);
    IGHWTOCIteratorConst<Texture> textures;
    IGHWTOCIteratorConst<MaterialResourceNameLookup> lookups;
    IGHWTOCIteratorConst<TextureResource> textureResources;
    CatchClasses(main, textures, lookups, textureResources);

    if (textures.Valid() && lookups.Valid()) {
      auto lookup = lookups.begin();
      shaderPath = lookup->lookupPath;
      size_t curHash = 0;

      for (auto h : lookup->mapHashes) {
        if (h && !reg.count(h))
          [&] {
            auto found =
                std::find(textureResources.begin(), textureResources.end(), h);

            if (es::IsEnd(textureResources, found)) {
              printwarning("Cannot find hash " << std::hex << h
                                               << " within material.");
              return;
            }

            const char *texturePath = lookup->mapLookupPaths[curHash];
            size_t textureIndex =
                std::distance(textureResources.begin(), found);

            reg.emplace(h,
                        TextureCache{texturePath, textures.at(textureIndex)});
          }();

        curHash++;
      }

      if (++lookup != lookups.end()) {
        printwarning("Multiple shader lookups detected, not implemented, "
                     "skipped others! Tell maintainer!");
      }
    }

    auto ectx = ctx->ExtractContext();

    if (shaderPath) {
      std::string path = "shaders/";
      path.append(shaderPath).append(".shd");
      if (settings.convertShaders) {
        path.append(".xml");
      }
      ectx->NewFile(path);
    } else {
      char tmpBuff[0x40];
      snprintf(tmpBuff, sizeof(tmpBuff),
               "shaders/%.8" PRIX32 ".%.8" PRIX32 ".shd%s", item.hash.part1,
               item.hash.part2, settings.convertShaders ? ".xml" : "");
      ectx->NewFile(tmpBuff);
    }

    if (settings.convertShaders) {
      pugi::xml_document doc;
      aggregators::Material(main, doc);
      XMLContextWritter xmlwr(ectx);
      doc.save(xmlwr);
    } else {
      char uniBuffer[0x80000]{};
      auto restream = [&](auto instream, size_t size) {
        const size_t numBlocks = size / sizeof(uniBuffer);
        const size_t restBytes = size % sizeof(uniBuffer);

        for (size_t i = 0; i < numBlocks; i++) {
          instream->read(uniBuffer, sizeof(uniBuffer));
          ectx->SendData({uniBuffer, sizeof(uniBuffer)});
        }

        if (restBytes) {
          instream->read(uniBuffer, restBytes);
          ectx->SendData({uniBuffer, restBytes});
        }
      };

      stream->seekg(item.offset);
      restream(stream.Get(), item.size);
    }
  }
}

void ExtractTexture(AppExtractContext *ctx, std::string path,
                    const Texture &info, bool hasHighMipData,
                    AppContextStream &highMipStream,
                    AppContextStream &textureStream,
                    const ResourceHighmips *foundHighMipData,
                    const ResourceTextures *foundTextureData) {
  std::string tmpBuffer;

  textureStream->seekg(foundTextureData->offset);
  if (hasHighMipData) {
    tmpBuffer.resize(foundHighMipData->size + foundTextureData->size);
    highMipStream->seekg(foundHighMipData->offset);
    highMipStream->read(tmpBuffer.data(), foundHighMipData->size);

    textureStream->read(tmpBuffer.data() + foundHighMipData->size,
                        foundTextureData->size);
  } else {
    tmpBuffer.resize(foundTextureData->size);
    textureStream->read(tmpBuffer.data(), foundTextureData->size);
  }

  TexelTile tile = TexelTile::Linear;

  auto GetFormat = [&] {
    using T = TextureFormat;
    switch (info.format) {
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
      PrintError("Invalid texture format: " + std::to_string(int(info.format)));
      break;
    }

    return TexelInputFormatType::INVALID;
  };

  NewTexelContextCreate tctx{
      .width = info.width,
      .height = info.height,
      .baseFormat =
          {
              .type = GetFormat(),
              .tile = tile,
          },
      .depth = std::max(uint16(1),
                        uint16(info.control3.Get<TextureControl3::depth>())),
      .numMipmaps = uint8(info.numMips),
      .data = tmpBuffer.data(),
  };

  ctx->NewImage(path, tctx);
}

void ExtractTextures(AppContext *ctx, const TextureRegistry &reg,
                     IGHWTOCIteratorConst<ResourceTextures> textures,
                     IGHWTOCIteratorConst<ResourceHighmips> highMips) {
  auto textureStream = ctx->RequestFile("textures.dat");
  auto highMipStream = ctx->RequestFile("highmips.dat");
  std::map<std::string, bool> duplicates;

  for (auto &r : reg) {
    auto found = duplicates.find(r.second.path);

    if (es::IsEnd(duplicates, found)) {
      duplicates.emplace(r.second.path, false);
    } else {
      found->second = true;
    }
  }

  auto ectx = ctx->ExtractContext();

  for (auto &r : reg) {
    auto foundTextureData =
        std::find(textures.begin(), textures.end(), r.first);

    auto foundHighMipData =
        std::find(highMips.begin(), highMips.end(), r.first);

    const bool hasTextureData = !es::IsEnd(textures, foundTextureData);
    const bool hasHighMipData = !es::IsEnd(highMips, foundHighMipData);

    if (!hasTextureData && !hasHighMipData) {
      if (!duplicates.count(r.second.path)) {
        printwarning("Missing data for texture " << r.second.path);
      }
      continue;
    }

    ExtractTexture(ectx, "textures/" + r.second.path, r.second.data,
                   hasHighMipData, highMipStream, textureStream,
                   foundHighMipData, foundTextureData);
  }
}

void RegionToGltf(IGHW &ighw, AppContext *ctx,
                  IGHWTOCIteratorConst<ResourceShaders> &shaders,
                  AppContextStream &shdStream,
                  IGHWTOCIteratorConst<ResourceTies> ties,
                  IGHWTOCIteratorConst<ResourceShrubs> shrubs,
                  IGHWTOCIteratorConst<ResourceFoliages> foliages,
                  AFileInfo zonePath);

void ExtractZones(AppContext *ctx,
                  IGHWTOCIteratorConst<ResourceShaders> shaders,
                  IGHWTOCIteratorConst<ResourceTies> ties,
                  IGHWTOCIteratorConst<ResourceShrubs> shrubs,
                  IGHWTOCIteratorConst<ResourceFoliages> foliages,
                  IGHWTOCIteratorConst<ResourceLighting> ligtmaps,
                  IGHWTOCIteratorConst<ResourceZones> zones) {
  auto shdStream = ctx->RequestFile("shaders.dat");
  auto streamZones = ctx->RequestFile("zones.dat");
  auto streamLightMaps = ctx->RequestFile("lighting.dat");
  BinReaderRef_e zonesData(*streamZones.Get());
  char uniBuffer[0x80000]{};

  auto ectx = ctx->ExtractContext();

  for (auto &item : zones) {
    std::string workingPath = "zones/";
    snprintf(uniBuffer, sizeof(uniBuffer), "%.8" PRIX32 ".%.8" PRIX32,
             item.hash.part1, item.hash.part2);
    workingPath.append(uniBuffer);

    std::string fileName = workingPath + ".zone.irb";
    streamZones->seekg(item.offset);
    IGHW main;
    main.FromStream(*streamZones.Get(), Version::V2);
    RegionToGltf(
        main, ctx, shaders, shdStream, ties, shrubs, foliages,
        AFileInfo(std::string(ctx->workingFile.GetFolder()) + fileName));
    ectx->NewFile(fileName);

    auto restream = [&](auto instream, size_t size) {
      const size_t numBlocks = size / sizeof(uniBuffer);
      const size_t restBytes = size % sizeof(uniBuffer);

      for (size_t i = 0; i < numBlocks; i++) {
        instream->read(uniBuffer, sizeof(uniBuffer));
        ectx->SendData({uniBuffer, sizeof(uniBuffer)});
      }

      if (restBytes) {
        instream->read(uniBuffer, restBytes);
        ectx->SendData({uniBuffer, restBytes});
      }
    };

    streamZones->seekg(item.offset);
    restream(streamZones.Get(), item.size);

    auto foundLM = std::find(ligtmaps.begin(), ligtmaps.end(), item.hash);
    if (es::IsEnd(ligtmaps, foundLM)) {
      continue;
    }

    IGHW curZone;
    zonesData.SetRelativeOrigin(item.offset);
    curZone.FromStream(zonesData, Version::V2);
    IGHWTOCIteratorConst<ZoneLightmap> zoneLightmaps;
    IGHWTOCIteratorConst<ZoneShadowMap> zoneShadowmaps;
    IGHWTOCIteratorConst<ZoneDataLookup> zoneDataLookups;
    IGHWTOCIteratorConst<TieInstanceV2> tieInstances;
    IGHWTOCIteratorConst<ZoneMap> zoneMaps;
    IGHWTOCIteratorConst<TextureResource> zoneMapRes;

    CatchClasses(curZone, zoneLightmaps, zoneShadowmaps, zoneDataLookups,
                 tieInstances, zoneMaps, zoneMapRes);
    size_t curZoneMap = 0;

    for (auto &map : zoneMaps) {
      char textureName[0x40];
      snprintf(textureName, sizeof(textureName), "/%.8" PRIX32,
               zoneMapRes.at(curZoneMap++).hash);
      std::string path = workingPath + textureName;
      ResourceTextures res;
      res.offset = map.offset + foundLM->offset;
      res.size = foundLM->size - map.offset,
      ExtractTexture(ectx, path, map, false, streamLightMaps, streamLightMaps,
                     nullptr, &res);
    }

    for (auto &zone0 : tieInstances) {
      if (zone0.lightMapId == -1) {
        continue;
      }

      auto &lookup = zoneDataLookups.at(zone0.lightMapId);

      const char *textureName = lookup.name;
      {
        auto &lm0 = zoneLightmaps.at(zone0.lightMapId);
        std::string path =
            ((workingPath + "/lightmaps/") + textureName) + ".dds";
        ResourceTextures res;
        res.offset = lm0.offset + foundLM->offset;
        res.size = foundLM->size - lm0.offset;
        ExtractTexture(ectx, path, lm0, false, streamLightMaps, streamLightMaps,
                       nullptr, &res);
      }

      {
        auto &lm0 = zoneShadowmaps.at(zone0.lightMapId);
        std::string path =
            ((workingPath + "/shadowmaps/") + textureName) + ".dds";
        ResourceTextures res;
        res.offset = lm0.offset + foundLM->offset;
        res.size = foundLM->size - lm0.offset;
        ExtractTexture(ectx, path, lm0, false, streamLightMaps, streamLightMaps,
                       nullptr, &res);
      }
    }
  }
}

void ShrubToGltf(IGHWTOCIteratorConst<ResourceShaders> &shaders, IGHW &ighw,
                 AppContext *ctx, AppContextStream &shdStream);

void ExtractShrubs(AppContext *ctx,
                   IGHWTOCIteratorConst<ResourceShaders> &shaders,
                   const IGHWTOCIteratorConst<ResourceShrubs> &shrubs) {
  auto stream = ctx->RequestFile("shrubs.dat");
  auto shdStream = ctx->RequestFile("shaders.dat");
  IGHW main;

  for (auto &subItem : shrubs) {
    BinReaderRef_e subRd(*stream.Get());
    subRd.SetRelativeOrigin(subItem.offset);
    main.FromStream(*stream.Get(), Version::V2);
    ShrubToGltf(shaders, main, ctx, shdStream);
  }
}

void ExtractAnimSets(AppContext *ctx, IGHWTOCIteratorConst<ResourceMobys> mobys,
                     IGHWTOCIteratorConst<ResourceAnimsets> animsets) {
  std::map<Hash, std::vector<std::string>> registry;

  {
    auto stream = ctx->RequestFile("mobys.dat");

    for (auto &moby : mobys) {
      BinReaderRef_e subRd(*stream.Get());
      subRd.SetRelativeOrigin(moby.offset);
      IGHW item;
      item.FromStream(subRd, Version::V2);
      IGHWTOCIteratorConst<MobyV2> model;

      CatchClasses(item, model);
      const Hash animHash = model.at(0).animset;
      if (animHash != Hash{}) {
        registry[animHash].emplace_back(model.at(0).selfPath);
      }
    }
  }

  auto stream = ctx->RequestFile("animsets.dat");
  auto ectx = ctx->ExtractContext();
  char uniBuffer[0x80000]{};

  auto restream = [&](auto instream, size_t size) {
    const size_t numBlocks = size / sizeof(uniBuffer);
    const size_t restBytes = size % sizeof(uniBuffer);

    for (size_t i = 0; i < numBlocks; i++) {
      instream->read(uniBuffer, sizeof(uniBuffer));
      ectx->SendData({uniBuffer, sizeof(uniBuffer)});
    }

    if (restBytes) {
      instream->read(uniBuffer, restBytes);
      ectx->SendData({uniBuffer, restBytes});
    }
  };

  for (auto &set : animsets) {
    if (auto found = registry.find(set.hash); found != registry.end()) {
      for (auto &p : found->second) {
        AFileInfo setPath(p);
        std::string animName(
            AFileInfo(setPath.GetFullPathNoExt()).GetFullPathNoExt());
        animName.append(".animset.irb");
        ectx->NewFile(animName);
        stream->seekg(set.offset);
        restream(stream.Get(), set.size);
      }
    } else {
      char tmpBuff[0x40];
      snprintf(tmpBuff, sizeof(tmpBuff), "%s/%.8" PRIX32 ".%.8" PRIX32 ".irb",
               "animsets", set.hash.part1, set.hash.part2);
      ectx->NewFile(tmpBuff);
      stream->seekg(set.offset);
      restream(stream.Get(), set.size);
    }
  }
}

void MobyToGltf(IGHWTOCIteratorConst<ResourceShaders> &shaders, IGHW &ighw,
                AppContext *ctx, AppContextStream &shdSteram);

void ExtractMobys(AppContext *ctx,
                  IGHWTOCIteratorConst<ResourceShaders> &shaders,
                  IGHWTOCIteratorConst<ResourceMobys> &mobys) {
  auto stream = ctx->RequestFile("mobys.dat");
  auto shdStream = ctx->RequestFile("shaders.dat");
  IGHW main;

  for (auto &subItem : mobys) {
    BinReaderRef_e subRd(*stream.Get());
    subRd.SetRelativeOrigin(subItem.offset);
    main.FromStream(*stream.Get(), Version::V2);
    MobyToGltf(shaders, main, ctx, shdStream);
  }
}

void TieToGltf(IGHWTOCIteratorConst<ResourceShaders> &shaders, IGHW &ighw,
               AppContext *ctx, AppContextStream &shdStream);

void ExtractTies(AppContext *ctx,
                 IGHWTOCIteratorConst<ResourceShaders> &shaders,
                 const IGHWTOCIteratorConst<ResourceTies> &ties) {
  auto stream = ctx->RequestFile("ties.dat");
  auto shdStream = ctx->RequestFile("shaders.dat");
  IGHW main;

  for (auto &subItem : ties) {
    BinReaderRef_e subRd(*stream.Get());
    subRd.SetRelativeOrigin(subItem.offset);
    main.FromStream(*stream.Get(), Version::V2);
    TieToGltf(shaders, main, ctx, shdStream);
  }
}

void FoliageToGltf(IGHWTOCIteratorConst<ResourceShaders> &shaders, IGHW &ighw,
                   AppContext *ctx, AppContextStream &shdStream,
                   AFileInfo path);

void ExtractFoliages(AppContext *ctx,
                     IGHWTOCIteratorConst<ResourceShaders> &shaders,
                     const IGHWTOCIteratorConst<ResourceFoliages> &foliages) {
  auto stream = ctx->RequestFile("foliages.dat");
  auto shdStream = ctx->RequestFile("shaders.dat");
  IGHW main;

  for (auto &subItem : foliages) {
    BinReaderRef_e subRd(*stream.Get());
    subRd.SetRelativeOrigin(subItem.offset);
    main.FromStream(*stream.Get(), Version::V2);
    char tmpBuff[0x40];
    snprintf(tmpBuff, sizeof(tmpBuff),
             "foliage/%.8" PRIX32 ".%.8" PRIX32 ".irb", subItem.hash.part1,
             subItem.hash.part2);
    FoliageToGltf(
        shaders, main, ctx, shdStream,
        AFileInfo(std::string(ctx->workingFile.GetFolder()) + tmpBuff));
  }
}

void AppProcessFile(AppContext *ctx) {
  BinReaderRef_e rd(ctx->GetStream());
  IGHW main;
  main.FromStream(rd, Version::V2);
  char uniBuffer[0x80000]{};
  auto ectx = ctx->ExtractContext();

  auto restream = [&](auto instream, size_t size) {
    const size_t numBlocks = size / sizeof(uniBuffer);
    const size_t restBytes = size % sizeof(uniBuffer);

    for (size_t i = 0; i < numBlocks; i++) {
      instream->read(uniBuffer, sizeof(uniBuffer));
      ectx->SendData({uniBuffer, sizeof(uniBuffer)});
    }

    if (restBytes) {
      instream->read(uniBuffer, restBytes);
      ectx->SendData({uniBuffer, restBytes});
    }
  };

  TextureRegistry textureRegistry;
  IGHWTOCIteratorConst<ResourceTextures> textures;
  IGHWTOCIteratorConst<ResourceHighmips> highMips;
  IGHWTOCIteratorConst<ResourceZones> zones;
  IGHWTOCIteratorConst<ResourceLighting> lightmaps;
  IGHWTOCIteratorConst<ResourceAnimsets> animsets;
  IGHWTOCIteratorConst<ResourceMobys> mobys;
  IGHWTOCIteratorConst<ResourceShaders> shaders;
  IGHWTOCIteratorConst<ResourceTies> ties;
  IGHWTOCIteratorConst<ResourceShrubs> shrubs;
  IGHWTOCIteratorConst<ResourceFoliages> foliages;

  auto ExtractWithLookup = [&](auto lookupId, auto iter, auto name) {
    std::string reqName = name + std::string(".dat");
    auto stream = ctx->RequestFile(reqName);

    for (auto &subItem : iter) {
      BinReaderRef_e subRd(*stream.Get());
      subRd.SetRelativeOrigin(subItem.offset);
      if (IGHWSeekClass(subRd, lookupId)) {
        std::string fileName;
        subRd.ReadString(fileName);
        ectx->NewFile(fileName);
      } else {
        char tmpBuff[0x40];
        snprintf(tmpBuff, sizeof(tmpBuff), "%s/%.8" PRIX32 ".%.8" PRIX32 ".irb",
                 name, subItem.hash.part1, subItem.hash.part2);
        ectx->NewFile(tmpBuff);
      }

      stream->seekg(subItem.offset);
      restream(stream.Get(), subItem.size);
    }
  };

  auto ExtractSet = [&](auto iter, auto name) {
    std::string reqName = name + std::string(".dat");
    auto stream = ctx->RequestFile(reqName);

    for (auto &subItem : iter) {
      char tmpBuff[0x40];
      snprintf(tmpBuff, sizeof(tmpBuff), "%s/%.8" PRIX32 ".%.8" PRIX32 ".irb",
               name, subItem.hash.part1, subItem.hash.part2);
      ectx->NewFile(tmpBuff);
      stream->seekg(subItem.offset);
      restream(stream.Get(), subItem.size);
    }
  };

  auto ExtractCommon = [&](const IGHWTOC &item) {
    switch (item.id) {
    case ResourceMobys::ID:
      mobys = item.Iter<ResourceMobys>();
      if (settings.extractFilter[Filter::Mobys]) {
        ExtractMobys(ctx, shaders, mobys);
        ExtractWithLookup(ResourceMobyPathLookupId, mobys, "mobys");
      }
      break;
    case ResourceCinematics::ID:
      if (settings.extractFilter[Filter::Cinematics]) {
        ExtractWithLookup(ResourceCinematicPathLookupId,
                          item.Iter<ResourceCinematics>(), "cinematics");
      }
      break;

    case ResourceCubemap::ID:
      if (settings.extractFilter[Filter::Cubemaps]) {
        ExtractSet(item.Iter<ResourceCubemap>(), "cubemaps");
      }
      break;

    default:
      break;
    }
  };

  CatchClassesLambda(main, ExtractCommon, textures, highMips, zones, lightmaps,
                     animsets, shaders, ties, shrubs, foliages);

  if (settings.extractFilter[Filter::Shrubs]) {
    ExtractShrubs(ctx, shaders, shrubs);
    ExtractWithLookup(ResourceShrubPathLookupId, shrubs, "shrubs");
  }

  if (settings.extractFilter[Filter::Foliages]) {
    ExtractFoliages(ctx, shaders, foliages);
    ExtractSet(foliages, "foliages");
  }

  if (settings.extractFilter[Filter::Ties]) {
    ExtractTies(ctx, shaders, ties);
    ExtractWithLookup(ResourceTiePathLookupId, ties, "ties");
  }

  if (settings.extractFilter[Filter::Shaders]) {
    ExtractShaders(ctx, shaders, textureRegistry);
  }

  if (settings.extractFilter[Filter::Animsets]) {
    ExtractAnimSets(ctx, mobys, animsets);
  }

  if (settings.extractFilter[Filter::Textures]) {
    ExtractTextures(ctx, textureRegistry, textures, highMips);
  }

  if (settings.extractFilter[Filter::Zones]) {
    ExtractZones(ctx, shaders, ties, shrubs, foliages, lightmaps, zones);
  }
}
