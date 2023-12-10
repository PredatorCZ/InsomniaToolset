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

void ExtractShaders(AppContext *ctx, const IGHWTOC &segment,
                    TextureRegistry &reg) {
  auto stream = ctx->RequestFile("shaders.dat");
  IGHW main;

  for (auto &item : segment.Iter<ResourceShaders>()) {
    const char *shaderPath = nullptr;
    BinReaderRef_e rd(*stream.Get());
    rd.SetRelativeOrigin(item.offset);
    main.FromStream(rd);
    IGHWTOCIteratorConst<Texture> textures;
    IGHWTOCIteratorConst<ShaderResourceLookup> lookups;
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
    switch (info.format) {
    case 0x85:
      tile = TexelTile::Morton;
      return TexelInputFormatType::RGBA8;
    case 0x81:
      return TexelInputFormatType::R8;
    case 0x86:
    case 0xa6: // srgb???
      return TexelInputFormatType::BC1;
    case 0x88:
    case 0xa8:
      return TexelInputFormatType::BC3;
    case 0x87:
    case 0xa7:
      return TexelInputFormatType::BC2;
    case 0x84:
      tile = TexelTile::Morton;
      return TexelInputFormatType::R5G6B5;
    case 0x83:
      tile = TexelTile::Morton;
      return TexelInputFormatType::RGBA4;
    case 0x8b:
      tile = TexelTile::Morton;
      return TexelInputFormatType::RG8;
    default:
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

void ExtractZones(AppContext *ctx,
                  IGHWTOCIteratorConst<ResourceLighting> ligtmaps,
                  IGHWTOCIteratorConst<ResourceZones> zones) {
  AppContextStream regionStream = std::move(
      ctx->FindFile(std::string(ctx->workingFile.GetFolder()), "^region.dat$"));
  IGHW region;
  region.FromStream(*regionStream.Get());
  IGHWTOCIteratorConst<ZoneHash> zoneHashes;
  IGHWTOCIteratorConst<ZoneNameLookup> zoneNames;

  CatchClasses(region, zoneHashes, zoneNames);

  auto streamZones = ctx->RequestFile("zones.dat");
  auto streamLightMaps = ctx->RequestFile("lighting.dat");
  BinReaderRef_e zonesData(*streamZones.Get());
  char uniBuffer[0x80000]{};

  auto ectx = ctx->ExtractContext();

  for (auto &item : zones) {
    auto foundZoneHash =
        std::find(zoneHashes.begin(), zoneHashes.end(), item.hash);
    std::string workingPath = "zones/";

    if (es::IsEnd(zoneHashes, foundZoneHash)) {
      snprintf(uniBuffer, sizeof(uniBuffer), "%.8" PRIX32 ".%.8" PRIX32,
               item.hash.part1, item.hash.part2);
      workingPath.append(uniBuffer);
    } else {
      workingPath.append(
          zoneNames.at(std::distance(zoneHashes.begin(), foundZoneHash)).name);
    }
    std::string fileName = workingPath + ".zone.irb";
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
    curZone.FromStream(zonesData);
    IGHWTOCIteratorConst<ZoneLightmap> zoneLightmaps;
    IGHWTOCIteratorConst<ZoneShadowMap> zoneShadowmaps;
    IGHWTOCIteratorConst<ZoneDataLookup> zoneDataLookups;
    IGHWTOCIteratorConst<ZoneData> zoneData;
    IGHWTOCIteratorConst<ZoneMap> zoneMaps;
    IGHWTOCIteratorConst<TextureResource> zoneMapRes;

    CatchClasses(curZone, zoneLightmaps, zoneShadowmaps, zoneDataLookups,
                 zoneData, zoneMaps, zoneMapRes);
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

    for (auto &zone0 : zoneData) {
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

void ExtractAnimSets(AppContext *ctx, IGHWTOCIteratorConst<ResourceMobys> mobys,
                     IGHWTOCIteratorConst<ResourceAnimsets> animsets) {
  std::map<Hash, std::vector<std::string>> registry;

  {
    auto stream = ctx->RequestFile("mobys.dat");

    for (auto &moby : mobys) {
      BinReaderRef_e subRd(*stream.Get());
      subRd.SetRelativeOrigin(moby.offset);
      IGHW item;
      item.FromStream(subRd);
      IGHWTOCIteratorConst<Moby> model;

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

void AppProcessFile(AppContext *ctx) {
  BinReaderRef_e rd(ctx->GetStream());
  IGHW main;
  main.FromStream(rd);
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
        ExtractWithLookup(ResourceMobyPathLookupId, mobys, "mobys");
      }
      break;
    case ResourceTies::ID:
      if (settings.extractFilter[Filter::Ties]) {
        ExtractWithLookup(ResourceTiePathLookupId, item.Iter<ResourceTies>(),
                          "ties");
      }
      break;
    case ResourceShrubs::ID:
      if (settings.extractFilter[Filter::Shrubs])
        ExtractWithLookup(ResourceShrubPathLookupId,
                          item.Iter<ResourceShrubs>(), "shrubs");
      break;
    case ResourceCinematics::ID:
      if (settings.extractFilter[Filter::Cinematics]) {
        ExtractWithLookup(ResourceCinematicPathLookupId,
                          item.Iter<ResourceCinematics>(), "cinematics");
      }
      break;

    case ResourceFoliages::ID:
      if (settings.extractFilter[Filter::Foliages]) {
        ExtractSet(item.Iter<ResourceFoliages>(), "foliages");
      }
      break;

    case ResourceCubemap::ID:
      if (settings.extractFilter[Filter::Cubemaps]) {
        ExtractSet(item.Iter<ResourceCubemap>(), "cubemaps");
      }
      break;

    case ResourceShaders::ID:
      if (settings.extractFilter[Filter::Shaders]) {
        ExtractShaders(ctx, item, textureRegistry);
      }
      break;

    default:
      break;
    }
  };

  CatchClassesLambda(main, ExtractCommon, textures, highMips, zones, lightmaps,
                     animsets);

  if (settings.extractFilter[Filter::Animsets]) {
    ExtractAnimSets(ctx, mobys, animsets);
  }

  if (settings.extractFilter[Filter::Textures]) {
    ExtractTextures(ctx, textureRegistry, textures, highMips);
  }

  if (settings.extractFilter[Filter::Zones]) {
    ExtractZones(ctx, lightmaps, zones);
  }
}
