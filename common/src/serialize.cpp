/*  InsomniaLib
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

#include "datas/binreader_stream.hpp"
#include "datas/except.hpp"
#include "insomnia/insomnia.hpp"
#include <map>

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
  FByteswapper(input.unk0);
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

template <> void FByteswapper(ShaderResourceLookup &input, bool) {
  FByteswapper(input.mapHashes);
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

template <class C> void fixupper(CoreClass *data, bool way) {
  FByteswapper(*static_cast<C *>(data), way);
}

struct ClassInfo {
  void (*swap)(CoreClass *, bool);
  size_t size;
};

template <class... C> auto RegisterClasses() {
  return std::initializer_list<std::pair<const uint32, ClassInfo>>{
      {C::ID, {fixupper<C>, sizeof(C)}}...};
}

static const std::map<uint32, ClassInfo> fixups{
    RegisterClasses<ResourceLighting, ResourceZones, ResourceAnimsets,
                    ResourceMobys, ResourceShrubs, ResourceTies,
                    ResourceFoliages, ResourceCubemap, ResourceShaders,
                    ResourceHighmips, ResourceTextures, ResourceCinematics,
                    TextureResource, Material, Texture, ShaderResourceLookup,
                    ZoneHash, ZoneNameLookup, ZoneLightmap, ZoneShadowMap,
                    ZoneDataLookup, ZoneData2Lookup, ZoneData, ZoneMap>(),
};

void IGHW::FromStream(BinReaderRef rd) {
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
      auto ptr = reinterpret_cast<es::PointerX86<uint32> *>(&buffer[0] + f);
      FByteswapper(*ptr);
      ptr->Fixup(buffer.data());
    }
  }

  FByteswapper(*Header());

  for (auto &item : *this) {
    FByteswapper(item, false);
    item.data.Fixup(buffer.data());
    auto found = fixups.find(item.id);

    if (!es::IsEnd(fixups, found)) {
      char *start = reinterpret_cast<char *>(item.data.operator->());
      char *end = nullptr;

      switch (item.count.ArrayType()) {
      case IGHWTOCArrayType::Buffer:
        end = reinterpret_cast<char *>(start) + item.size;
        break;
      case IGHWTOCArrayType::Array:
        end = reinterpret_cast<char *>(start) +
              item.count.Count() * found->second.size;
        break;

      default:
        throw std::runtime_error("Unknown array type");
      }

      while (start < end) {
        found->second.swap(reinterpret_cast<CoreClass *>(start), false);
        start += found->second.size;
      }
    }
  }
}
