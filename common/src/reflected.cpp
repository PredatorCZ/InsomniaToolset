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

#include "spike/reflect/reflector.hpp"
#include "spike/reflect/reflector_xml.hpp"
#include "insomnia/insomnia.hpp"
#include "pugixml.hpp"

namespace aggregators_ {
struct Texture {
  const char *path = nullptr;
  uint32 hash = 0;
  uint32 id;
  TextureFlagsType flags;
  TextureAddressType address;
  TextureControl0Type control0;
  TextureControl3Type control3;
  uint32 filter;
  uint32 borderColor;
};

struct Material : ::Material {
  uint32 hash[2]{};
  Material(const ::Material &mat) : ::Material(mat) {}
};
} // namespace aggregators_

REFLECT(CLASS(aggregators_::Texture), //
        MEMBER(id), MEMBER(path), MEMBER(hash), MEMBER(flags), MEMBER(address),
        MEMBER(control0), MEMBER(control3), MEMBER(filter),
        MEMBER(borderColor));

REFLECT(CLASS(aggregators_::Material), //
        MEMBER(hash), MEMBER(diffuseMapId), MEMBER(normalMapId),
        MEMBER(specularMapId), MEMBER(detailMapId), MEMBER(unk0),
        MEMBER(values));

REFLECT(CLASS(TextureFlagsType), //
        BITMEMBERNAME(TextureFlags::location, "location"),
        BITMEMBERNAME(TextureFlags::isCubemap, "isCubemap"),
        BITMEMBERNAME(TextureFlags::useBorder, "useBorder"),
        BITMEMBERNAME(TextureFlags::dimension, "dimension"));

REFLECT(CLASS(TextureAddressType), //
        BITMEMBERNAME(TextureAddress::wrapS, "wrapS"),
        BITMEMBERNAME(TextureAddress::anisotropicBias, "anisotropicBias"),
        BITMEMBERNAME(TextureAddress::wrapT, "wrapT"),
        BITMEMBERNAME(TextureAddress::unsignedRemap, "unsignedRemap"),
        BITMEMBERNAME(TextureAddress::wrapR, "wrapR"),
        BITMEMBERNAME(TextureAddress::gamma, "gamma"),
        BITMEMBERNAME(TextureAddress::signedRemap, "signedRemap"),
        BITMEMBERNAME(TextureAddress::zFunc, "zFunc"));

REFLECT(CLASS(TextureControl0Type), //
        BITMEMBERNAME(TextureControl0::unk0, "unk0"),
        BITMEMBERNAME(TextureControl0::alphaKill, "alphaKill"),
        BITMEMBERNAME(TextureControl0::unk1, "unk1"),
        BITMEMBERNAME(TextureControl0::maxAnisotropy, "maxAnisotropy"),
        BITMEMBERNAME(TextureControl0::maxLod, "maxLod"),
        BITMEMBERNAME(TextureControl0::minLod, "minLod"),
        BITMEMBERNAME(TextureControl0::isEnabled, "isEnabled"));

REFLECT(ENUMERATION(WrapMode),
        ENUM_MEMBER(Wrap),                  //
        ENUM_MEMBER(Mirror),                //
        ENUM_MEMBER(ClampOnEdge),           //
        ENUM_MEMBER(Border),                //
        ENUM_MEMBER(Clamp),                 //
        ENUM_MEMBER(MirrorOnceClampToEdge), //
        ENUM_MEMBER(MirrorOnceBorder),      //
        ENUM_MEMBER(MirrorOnceClamp),       //
);

REFLECT(ENUMERATION(MaxAnisotropy),
        ENUM_MEMBER(x1),  //
        ENUM_MEMBER(x2),  //
        ENUM_MEMBER(x4),  //
        ENUM_MEMBER(x6),  //
        ENUM_MEMBER(x8),  //
        ENUM_MEMBER(x10), //
        ENUM_MEMBER(x12), //
        ENUM_MEMBER(x16), //
);

REFLECT(ENUMERATION(ZComparisonFunction),
        ENUM_MEMBER(NOP),    //
        ENUM_MEMBER(LT),     //
        ENUM_MEMBER(EQ),     //
        ENUM_MEMBER(LE),     //
        ENUM_MEMBER(GT),     //
        ENUM_MEMBER(NOT),    //
        ENUM_MEMBER(GE),     //
        ENUM_MEMBER(Always), //
);

REFLECT(CLASS(TextureControl3Type),
        BITMEMBERNAME(TextureControl3::pitch, "pitch"),
        BITMEMBERNAME(TextureControl3::depth, "depth"));

namespace aggregators {
void Material(IGHW &main, pugi::xml_node node) {
  IGHWTOCIteratorConst<Texture> textures;
  IGHWTOCIteratorConst<ShaderResourceLookup> lookups;
  IGHWTOCIteratorConst<TextureResource> textureResources;
  IGHWTOCIteratorConst<::Material> materials;

  for (auto &i : main) {
    switch (i.id) {
    case Texture::ID:
      textures = i.Iter<Texture>();
      break;
    case ShaderResourceLookup::ID:
      lookups = i.Iter<ShaderResourceLookup>();
      break;
    case TextureResource::ID:
      textureResources = i.Iter<TextureResource>();
      break;
    case Material::ID:
      materials = i.Iter<::Material>();
      break;
    default:
      break;
    }
  }

  if (materials.Valid()) {
    auto matNode = node.append_child("material");
    aggregators_::Material mat(materials.at(0));

    if (lookups.Valid()) {
      auto &curLookup = lookups.at(0);
      mat.hash[0] = curLookup.hash.part1;
      mat.hash[1] = curLookup.hash.part2;
    }

    ReflectorWrap ri(mat);
    ReflectorXMLUtil::SaveV2a(ri, matNode,
                              ReflectorXMLUtil::Flags_StringAsAttribute);
  }

  size_t curItem = 0;

  if (textures.Valid() && textureResources.Valid()) {
    auto texNode = node.append_child("textures");
    for (auto &t : textures) {
      aggregators_::Texture tex;
      tex.address = t.address;
      tex.borderColor = t.borderColor;
      tex.control0 = t.control0;
      tex.control3 = t.control3;
      tex.filter = t.filter;
      tex.flags = t.flags;
      tex.hash = textureResources.at(curItem).hash;
      tex.id = curItem;

      if (lookups.Valid())
        [&] {
          for (auto &l : lookups) {
            size_t curMapIndex = 0;
            for (auto h : l.mapHashes) {
              if (h == tex.hash) {
                tex.path = l.mapLookupPaths[curMapIndex];
                return;
              }

              curMapIndex++;
            }
          }
        }();

      curItem++;

      ReflectorWrap ri(tex);
      ReflectorXMLUtil::SaveV2a(ri, texNode.append_child("texture"),
                                ReflectorXMLUtil::Flags_StringAsAttribute);
    }
  }
}
} // namespace aggregators
