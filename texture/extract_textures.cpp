/*  InsomniaToolset TextureExtract
    Copyright(C) 2026 Lukas Cone

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
#include "spike/app_context.hpp"
#include "spike/io/binreader_stream.hpp"
#include "spike/master_printer.hpp"
#include <algorithm>

std::string_view filters[]{
    ".tph$",
};

static AppInfo_s appInfo{
    .header = TextureExtract_DESC " v" TextureExtract_VERSION
                                  ", " TextureExtract_COPYRIGHT "Lukas Cone",
    .filters = filters,
};

AppInfo_s *AppInitModule() { return &appInfo; }

void ExtractTexture(AppContext *ctx, const TextureV1 &tex, const char *data,
                    const TextureResource &res) {
  TexelTile tile = TexelTile::Linear;

  auto GetFormat = [&] {
    using T = TextureFormat;
    switch (tex.format) {
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
      PrintError("Invalid texture format: " + std::to_string(int(tex.format)));
      break;
    }

    return TexelInputFormatType::INVALID;
  };

  TexelSwizzle swizzle;

  NewTexelContextCreate tctx{
      .width = tex.width,
      .height = tex.height,
      .baseFormat =
          {
              .type = GetFormat(),
              .swizzle = swizzle,
              .tile = tile,
              .swapPacked = true,
          },
      .depth = std::max(uint16(1),
                        uint16(tex.control3.Get<TextureControl3::depth>())),
      .numMipmaps = uint8(tex.numMips),
      .data = data + tex.offset,
  };

  ctx->ExtractContext()->NewImage(std::to_string(res.hash), tctx);
}

void AppProcessFile(AppContext *ctx) {
  BinReaderRef_e rd(ctx->GetStream());
  IGHW main;
  main.FromStream(rd, Version::RFOM);
  IGHWTOCIteratorConst<TextureV1> textures;
  IGHWTOCIteratorConst<TextureData> textureData;
  IGHWTOCIteratorConst<TextureResource> textureResources;
  IGHW stream;
  stream.FromStream(
      *ctx->RequestFile(ctx->workingFile.ChangeExtension2("tp")).Get(),
      Version::RFOM);

  CatchClasses(main, textures);
  CatchClasses(stream, textureData, textureResources);

  for (uint32 i = 0; auto &tex : textures) {
    ExtractTexture(ctx, tex, &textureData.begin()->data, textureResources.at(i++));
  }
}
