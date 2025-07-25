/*  InsomniaToolset Region2GLTF
    Copyright(C) 2025 Lukas Cone

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

#include "gltf_ighw.hpp"
#include "project.h"
#include "spike/app_context.hpp"
#include "spike/io/binreader_stream.hpp"

std::string_view filters[]{
    "^region.dat$",
};

static AppInfo_s appInfo{
    .header = Region2GLTF_DESC " v" Region2GLTF_VERSION
                               ", " Region2GLTF_COPYRIGHT "Lukas Cone",
    .filters = filters,
};

AppInfo_s *AppInitModule() { return &appInfo; }

void AppProcessFile(AppContext *ctx) {
  BinReaderRef_e rd(ctx->GetStream());
  IGHW region;
  region.FromStream(rd, Version::V2);
  IMGLTF main;

  IGHWTOCIteratorConst<ZoneHash> zoneHashes;
  IGHWTOCIteratorConst<ZoneNameLookup> zoneNames;
  IGHWTOCIteratorConst<ResourceShaders> shaders;
  IGHWTOCIteratorConst<ResourceZones> zones;
  IGHWTOCIteratorConst<ResourceTies> ties;
  IGHWTOCIteratorConst<ResourceShrubs> shrubs;
  IGHWTOCIteratorConst<ResourceFoliages> foliages;

  CatchClasses(region, zoneHashes, zoneNames);
  std::string_view thisDir = ctx->workingFile.GetFolder();
  thisDir.remove_suffix(1);
  std::string mainDir(AFileInfo(thisDir).GetFolder());

  auto shdStream = ctx->RequestFile(mainDir + "shaders.dat");
  auto streamZones = ctx->RequestFile(mainDir + "zones.dat");
  auto streamAssetLookup = ctx->RequestFile(mainDir + "assetlookup.dat");
  IGHW lookup;
  lookup.FromStream(*streamAssetLookup.Get(), Version::V2);
  CatchClasses(lookup, shaders, zones, ties, shrubs, foliages);

  for (auto &z : zoneHashes) {
    auto foundZone = std::find(zones.begin(), zones.end(), z.hash);
    streamZones->seekg(foundZone->offset);
    IGHW zone;
    zone.FromStream(*streamZones.Get(), Version::V2);
    RegionToGltf(main, zone, shaders, shdStream, ties, shrubs, foliages, ctx,
                 mainDir);
  }

  GenerateInstances(main);
  main.FinishAndSave(ctx->NewFile(ctx->workingFile.ChangeExtension2("glb")).str,
                     "");
}
