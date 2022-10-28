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

#include "datas/app_context.hpp"
#include "datas/binreader_stream.hpp"
#include "datas/except.hpp"
#include "datas/fileinfo.hpp"
#include "datas/flags.hpp"
#include "datas/master_printer.hpp"
#include "datas/reflector.hpp"
#include "formats/DDS.hpp"
#include "formats/addr_ps3.hpp"
#include "insomnia/insomnia.hpp"
#include "project.h"
#include "pugixml.hpp"

MAKE_ENUM(ENUMSCOPE(class Filter, Filter), EMEMBER(Mobys), EMEMBER(Ties),
          EMEMBER(Shrubs), EMEMBER(Foliages), EMEMBER(Zones), EMEMBER(Textures),
          EMEMBER(Shaders), EMEMBER(Cinematics), EMEMBER(Animsets),
          EMEMBER(Cubemaps))

static struct AssetExtract : ReflectorBase<AssetExtract> {
  bool convertShaders = true;
  bool legacyDDS = true;
  bool forceLegacyDDS = false;
  bool largestMipmap = true;
  es::Flags<Filter> extractFilter{0xffffu};
} settings;

REFLECT(
    CLASS(AssetExtract),
    MEMBERNAME(convertShaders, "convert-shaders", "s",
               ReflDesc{"Convert shaders into XML format."}),
    MEMBERNAME(legacyDDS, "legacy-dds", "l",
               ReflDesc{
                   "Tries to convert texture into legacy (DX9) DDS format."}),
    MEMBERNAME(forceLegacyDDS, "force-legacy-dds", "f",
               ReflDesc{"Will try to convert some matching formats from DX10 "
                        "to DX9, for example: RG88 to AL88."}),
    MEMBERNAME(largestMipmap, "largest-mipmap-only", "m",
               ReflDesc{"Will try to extract only highest mipmap."}),
    MEMBERNAME(extractFilter, "extract-filter", "e",
               ReflDesc{"Select groups that should be extracted."}), );

es::string_view filters[]{
    "$assetlookup.dat",
    {},
};

static AppInfo_s appInfo{
    AppInfo_s::CONTEXT_VERSION,
    AppMode_e::EXTRACT,
    ArchiveLoadType::ALL,
    AssetExtract_DESC " v" AssetExtract_VERSION ", " AssetExtract_COPYRIGHT
                      "Lukas Cone",
    reinterpret_cast<ReflectorFriend *>(&settings),
    filters,
};

AppInfo_s *AppInitModule() {
  return &appInfo;
}

template <> void FByteswapper(IGHWHeader &input, bool) {
  FByteswapper(input.id);
  FByteswapper(input.numToc);
  FByteswapper(input.dataEnd);
}

template <> void FByteswapper(IGHWTOC &input, bool) {
  FByteswapper(input.id);
  FByteswapper(input.data);
}

bool IGHWSeekClass(BinReaderRef rd, uint32 classId) {
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

void ExtractShaders(AppExtractContext *ctx, const IGHWTOC &segment,
                    TextureRegistry &reg) {
  auto stream = ctx->ctx->RequestFile("shaders.dat");
  IGHW main;

  for (auto &item : segment.Iter<ResourceShaders>()) {
    const char *shaderPath = nullptr;
    BinReaderRef rd(*stream.Get());
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

    if (shaderPath) {
      std::string path = "shaders/";
      path.append(shaderPath).append(".shd");
      if (settings.convertShaders) {
        path.append(".xml");
      }
      ctx->NewFile(path);
    } else {
      char tmpBuff[0x40];
      snprintf(tmpBuff, sizeof(tmpBuff),
               "shaders/%.8" PRIX32 ".%.8" PRIX32 ".shd%s", item.hash.part1,
               item.hash.part2, settings.convertShaders ? ".xml" : "");
      ctx->NewFile(tmpBuff);
    }

    if (settings.convertShaders) {
      pugi::xml_document doc;
      aggregators::Material(main, doc);
      XMLContextWritter xmlwr(ctx);
      doc.save(xmlwr);
    } else {
      char uniBuffer[0x80000]{};
      auto restream = [&](auto instream, size_t size) {
        const size_t numBlocks = size / sizeof(uniBuffer);
        const size_t restBytes = size % sizeof(uniBuffer);

        for (size_t i = 0; i < numBlocks; i++) {
          instream->read(uniBuffer, sizeof(uniBuffer));
          ctx->SendData({uniBuffer, sizeof(uniBuffer)});
        }

        if (restBytes) {
          instream->read(uniBuffer, restBytes);
          ctx->SendData({uniBuffer, restBytes});
        }
      };

      stream->seekg(item.offset);
      restream(stream.Get(), item.size);
    }
  }
}

void ExtractTexture(AppExtractContext *ctx, const char *path,
                    const Texture &info, bool hasHighMipData,
                    AppContextStream &highMipStream,
                    AppContextStream &textureStream,
                    const ResourceHighmips *foundHighMipData,
                    const ResourceTextures *foundTextureData) {
  char uniBuffer[0x80000]{};
  thread_local static std::string tmpBuffer;
  thread_local static std::string outBuffer;
  DDS main;
  main = DDSFormat_DX10;
  main.width = info.width;
  main.height = info.height;
  main.depth = info.control3.Get<TextureControl3::depth>();
  main.pitchOrLinearSize = info.control3.Get<TextureControl3::pitch>();
  main.NumMipmaps(settings.largestMipmap ? 1 : info.numMips);

  switch (info.format) {
  case 0x85:
    main.dxgiFormat = DXGI_FORMAT_B8G8R8A8_UNORM;
    break;
  case 0x81:
    main.dxgiFormat = DXGI_FORMAT_R8_UNORM;
    break;
  case 0x86:
  case 0xa6: // srgb???
    main.dxgiFormat = DXGI_FORMAT_BC1_UNORM;
    break;
  case 0x88:
  case 0xa8:
    main.dxgiFormat = DXGI_FORMAT_BC3_UNORM;
    break;
  case 0x87:
  case 0xa7:
    main.dxgiFormat = DXGI_FORMAT_BC2_UNORM;
    break;
  case 0x84:
    main.dxgiFormat = DXGI_FORMAT_B5G6R5_UNORM;
    break;
  case 0x83:
    main.dxgiFormat = DXGI_FORMAT_B4G4R4A4_UNORM;
    break;
  case 0x8b:
    main.dxgiFormat = DXGI_FORMAT_R8G8_UNORM;
    break;
  default:
    break;
  }

  main.ComputeBPP();

  bool mustRepack = !IsBC(main.dxgiFormat);
  const uint32 sizetoWrite =
      !settings.legacyDDS || main.ToLegacy(settings.forceLegacyDDS)
          ? main.DDS_SIZE
          : main.LEGACY_SIZE;
  if (settings.legacyDDS && sizetoWrite == main.DDS_SIZE) {
    printwarning("Couldn't convert " << path << " to legacy.")
  }

  ctx->SendData({reinterpret_cast<const char *>(&main), sizetoWrite});

  auto restream = [&](auto instream, size_t size) {
    const size_t numBlocks = size / sizeof(uniBuffer);
    const size_t restBytes = size % sizeof(uniBuffer);

    for (size_t i = 0; i < numBlocks; i++) {
      instream->read(uniBuffer, sizeof(uniBuffer));
      ctx->SendData({uniBuffer, sizeof(uniBuffer)});
    }

    if (restBytes) {
      instream->read(uniBuffer, restBytes);
      ctx->SendData({uniBuffer, restBytes});
    }
  };

  auto repack = [&](auto instream, size_t size, size_t mip = 0) {
    char *repackBuffer = uniBuffer;
    if (size > sizeof(uniBuffer)) {
      tmpBuffer.resize(size);
      repackBuffer = &tmpBuffer[0];
    }

    outBuffer.resize(size);
    instream->read(repackBuffer, size);

    const size_t width = main.width >> mip;
    const size_t height = main.height >> mip;
    MortonSettings mortonSettings(width, height);
    const size_t stride = main.bpp / 8;

    for (size_t p = 0; p < size; p += stride) {
      const size_t coord = p / stride;
      const size_t x = coord % width;
      const size_t y = coord / width;
      memcpy(&outBuffer[p],
             repackBuffer + MortonAddr(x, y, mortonSettings) * stride, stride);
    }

    ctx->SendData({outBuffer.data(), size});
  };

  DDS::Mips mips{};
  main.ComputeBufferSize(mips);

  if (settings.largestMipmap) {
    const size_t mipMapSize =
        mips.sizes[hasHighMipData || !foundHighMipData ? 0 : 1];
    auto &stream = hasHighMipData ? highMipStream : textureStream;
    stream->seekg(hasHighMipData ? foundHighMipData->offset
                                 : foundTextureData->offset);
    if (mustRepack) {
      repack(stream.Get(), mipMapSize);
    } else {
      restream(stream.Get(), mipMapSize);
    }
  } else {
    if (hasHighMipData) {
      highMipStream->seekg(foundHighMipData->offset);

      if (mustRepack) {
        repack(highMipStream.Get(), foundHighMipData->size);
      } else {
        restream(highMipStream.Get(), foundHighMipData->size);
      }
    }

    if (mustRepack) {
      const size_t offsetTweak = hasHighMipData ? mips.sizes[0] : 0;
      for (size_t mip = hasHighMipData; mip < main.mipMapCount; mip++) {
        textureStream->seekg(foundTextureData->offset + mips.offsets[mip] -
                             offsetTweak);
        repack(textureStream.Get(), mips.sizes[mip], mip);
      }
    } else {
      textureStream->seekg(foundTextureData->offset);
      restream(textureStream.Get(), foundTextureData->size);
    }
  }
}

void ExtractTextures(AppExtractContext *ctx, const TextureRegistry &reg,
                     IGHWTOCIteratorConst<ResourceTextures> textures,
                     IGHWTOCIteratorConst<ResourceHighmips> highMips) {
  auto textureStream = ctx->ctx->RequestFile("textures.dat");
  auto highMipStream = ctx->ctx->RequestFile("highmips.dat");
  std::map<std::string, bool> duplicates;

  for (auto &r : reg) {
    auto found = duplicates.find(r.second.path);

    if (es::IsEnd(duplicates, found)) {
      duplicates.emplace(r.second.path, false);
    } else {
      found->second = true;
    }
  }

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

    ctx->NewFile("textures/" + r.second.path + ".dds");
    const auto &info = r.second.data;
    ExtractTexture(ctx, r.second.path.data(), info, hasHighMipData,
                   highMipStream, textureStream, foundHighMipData,
                   foundTextureData);
  }
}

void ExtractZones(AppExtractContext *ctx,
                  IGHWTOCIteratorConst<ResourceLighting> ligtmaps,
                  IGHWTOCIteratorConst<ResourceZones> zones) {
  AFileInfo workingfolder(ctx->ctx->workingFile);
  AppContextStream regionStream =
      std::move(ctx->ctx->FindFile(workingfolder.GetFolder(), "$region.dat"));
  IGHW region;
  region.FromStream(*regionStream.Get());
  IGHWTOCIteratorConst<ZoneHash> zoneHashes;
  IGHWTOCIteratorConst<ZoneNameLookup> zoneNames;

  CatchClasses(region, zoneHashes, zoneNames);

  auto streamZones = ctx->ctx->RequestFile("zones.dat");
  auto streamLightMaps = ctx->ctx->RequestFile("lighting.dat");
  BinReaderRef zonesData(*streamZones.Get());
  char uniBuffer[0x80000]{};
  size_t curZoneIndex = 0;

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
    ctx->NewFile(fileName);

    auto restream = [&](auto instream, size_t size) {
      const size_t numBlocks = size / sizeof(uniBuffer);
      const size_t restBytes = size % sizeof(uniBuffer);

      for (size_t i = 0; i < numBlocks; i++) {
        instream->read(uniBuffer, sizeof(uniBuffer));
        ctx->SendData({uniBuffer, sizeof(uniBuffer)});
      }

      if (restBytes) {
        instream->read(uniBuffer, restBytes);
        ctx->SendData({uniBuffer, restBytes});
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
      snprintf(textureName, sizeof(textureName), "/%.8" PRIX32 ".dds",
               zoneMapRes.at(curZoneMap++).hash);
      std::string path = workingPath + textureName;
      ctx->NewFile(path);
      ResourceTextures res;
      res.offset = map.offset + foundLM->offset;
      res.size = es::IsEnd(ligtmaps, ligtmaps.begin() + curZoneIndex + 1)
                     ? foundLM->size
                     : ligtmaps.at(curZoneIndex + 1).offset;
      res.size -= map.offset;
      ExtractTexture(ctx, textureName, map, false, streamLightMaps,
                     streamLightMaps, nullptr, &res);
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
        ctx->NewFile(path);
        ResourceTextures res;
        res.offset = lm0.offset + foundLM->offset;
        res.size = es::IsEnd(ligtmaps, ligtmaps.begin() + curZoneIndex + 1)
                       ? foundLM->size
                       : ligtmaps.at(curZoneIndex + 1).offset;
        res.size -= lm0.offset;
        ExtractTexture(ctx, textureName, lm0, false, streamLightMaps,
                       streamLightMaps, nullptr, &res);
      }

      {
        auto &lm0 = zoneShadowmaps.at(zone0.lightMapId);
        std::string path =
            ((workingPath + "/shadowmaps/") + textureName) + ".dds";
        ctx->NewFile(path);
        ResourceTextures res;
        res.offset = lm0.offset + foundLM->offset;
        res.size = es::IsEnd(ligtmaps, ligtmaps.begin() + curZoneIndex + 1)
                       ? foundLM->size
                       : ligtmaps.at(curZoneIndex + 1).offset;
        res.size -= lm0.offset;
        ExtractTexture(ctx, textureName, lm0, false, streamLightMaps,
                       streamLightMaps, nullptr, &res);
      }
    }
  }
}

void AppExtractFile(std::istream &stream, AppExtractContext *ctx) {
  BinReaderRef rd(stream);
  IGHW main;
  main.FromStream(rd);
  char uniBuffer[0x80000]{};

  auto restream = [&](auto instream, size_t size) {
    const size_t numBlocks = size / sizeof(uniBuffer);
    const size_t restBytes = size % sizeof(uniBuffer);

    for (size_t i = 0; i < numBlocks; i++) {
      instream->read(uniBuffer, sizeof(uniBuffer));
      ctx->SendData({uniBuffer, sizeof(uniBuffer)});
    }

    if (restBytes) {
      instream->read(uniBuffer, restBytes);
      ctx->SendData({uniBuffer, restBytes});
    }
  };

  TextureRegistry textureRegistry;
  IGHWTOCIteratorConst<ResourceTextures> textures;
  IGHWTOCIteratorConst<ResourceHighmips> highMips;
  IGHWTOCIteratorConst<ResourceZones> zones;
  IGHWTOCIteratorConst<ResourceLighting> lightmaps;

  auto ExtractWithLookup = [&](auto lookupId, auto iter, auto name) {
    std::string reqName = name + std::string(".dat");
    auto stream = ctx->ctx->RequestFile(reqName);

    for (auto &subItem : iter) {
      BinReaderRef subRd(*stream.Get());
      subRd.SetRelativeOrigin(subItem.offset);
      if (IGHWSeekClass(subRd, lookupId)) {
        std::string fileName;
        subRd.ReadString(fileName);
        ctx->NewFile(fileName);
      } else {
        char tmpBuff[0x40];
        snprintf(tmpBuff, sizeof(tmpBuff), "%s/%.8" PRIX32 ".%.8" PRIX32 ".irb",
                 name, subItem.hash.part1, subItem.hash.part2);
        ctx->NewFile(tmpBuff);
      }

      stream->seekg(subItem.offset);
      restream(stream.Get(), subItem.size);
    }
  };

  auto ExtractSet = [&](auto iter, auto name) {
    std::string reqName = name + std::string(".dat");
    auto stream = ctx->ctx->RequestFile(reqName);

    for (auto &subItem : iter) {
      char tmpBuff[0x40];
      snprintf(tmpBuff, sizeof(tmpBuff), "%s/%.8" PRIX32 ".%.8" PRIX32 ".irb",
               name, subItem.hash.part1, subItem.hash.part2);
      ctx->NewFile(tmpBuff);
      stream->seekg(subItem.offset);
      restream(stream.Get(), subItem.size);
    }
  };

  auto ExtractCommon = [&](const IGHWTOC &item) {
    switch (item.id) {
    case ResourceMobys::ID:
      if (settings.extractFilter[Filter::Mobys]) {
        ExtractWithLookup(ResourceMobyPathLookupId, item.Iter<ResourceMobys>(),
                          "mobys");
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

    case ResourceAnimsets::ID:
      if (settings.extractFilter[Filter::Animsets]) {
        ExtractSet(item.Iter<ResourceAnimsets>(), "animsets");
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

  CatchClassesLambda(main, ExtractCommon, textures, highMips, zones, lightmaps);

  // ExtractSet(lightmaps, "lighting");

  if (settings.extractFilter[Filter::Textures]) {
    ExtractTextures(ctx, textureRegistry, textures, highMips);
  }

  if (settings.extractFilter[Filter::Zones]) {
    ExtractZones(ctx, lightmaps, zones);
  }
}
