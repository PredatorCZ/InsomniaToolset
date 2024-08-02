/*  EffectExtract
    Copyright(C) 2023 Lukas Cone

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
#include "spike/except.hpp"
#include "spike/io/binreader_stream.hpp"
#include <spike/master_printer.hpp>

static AppInfo_s appInfo{
    .header = EffectExtract_DESC " v" EffectExtract_VERSION
                                 ", " EffectExtract_COPYRIGHT "Lukas Cone",
};

AppInfo_s *AppInitModule() { return &appInfo; }

void ExtractTexture(AppExtractContext *ctx, std::string path,
                    const Texture &info, const char *data) {
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
      .data = data,
  };

  ctx->NewImage(path, tctx);
}

void AppProcessFile(AppContext *ctx) {
  BinReaderRef_e rd(ctx->GetStream());
  IGHW main;
  main.FromStream(rd, Version::V2);
  auto dataStream = ctx->RequestFile(std::string(ctx->workingFile.GetFolder()) +
                                     "vfx_system_texel.dat");
  BinReaderRef_e rdd(*dataStream.Get());
  IGHW data;
  data.FromStream(rdd, Version::V2);
  auto ectx = ctx->ExtractContext();

  IGHWTOCIteratorConst<TextureResource> texturResources;
  IGHWTOCIteratorConst<Texture> textures;
  IGHWTOCIteratorConst<EffectTextureBuffer> textureData;

  CatchClasses(main, textures);
  CatchClasses(data, textureData, texturResources);

  for (size_t idx = 0; auto &tex : textures) {
    auto &res = texturResources.at(idx++);
    char tmpBuff[0x10];
    snprintf(tmpBuff, sizeof(tmpBuff), "%.8" PRIX32, res.hash);
    ExtractTexture(ectx, tmpBuff, tex, &textureData.begin()->data + tex.offset);
  }
}
