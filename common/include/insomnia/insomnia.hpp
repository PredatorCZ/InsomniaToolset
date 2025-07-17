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

#pragma once
#include "classes/detail.hpp"
#include "classes/foliage.hpp"
#include "classes/moby.hpp"
#include "classes/region.hpp"
#include "classes/resource.hpp"
#include "classes/shrub.hpp"
#include "classes/tie.hpp"
#include "classes/zone.hpp"
#include "internal/settings.hpp"
#include "spike/io/bincore_fwd.hpp"
#include "spike/type/bitfield.hpp"
#include <typeinfo>

static const float YARD_TO_M = 0.9144;
static const float M_TO_YARD = 1 / YARD_TO_M;

enum class IGHWTOCArrayType {
  Buffer,
  Array,
};

struct IGHWTOCArrayCounter {
  using Flags = BitMemberDecl<0, 4>;
  using Counter = BitMemberDecl<2, 28>;
  using Type = BitFieldType<uint32, Flags, Counter>;
  Type value;
  auto ArrayType() const { return IGHWTOCArrayType(value.Get<Flags>()); }
  uint32 Count() const { return value.Get<Counter>(); }
};

template <class C> struct IGHWTOCIteratorConst {
  using value_type = C;
  const C *begin_ = nullptr;
  const C *end_ = nullptr;

  auto begin() const { return begin_; }
  auto end() const { return end_; }

  bool Valid() const { return begin_ != end_ && end_ != nullptr; }

  const C &at(size_t index) const {
    auto item = begin() + index;

    if (item >= end()) {
      throw std::out_of_range("Index out of range.");
    }

    return *item;
  }
};

struct IGHWTOC {
  uint32 id;
  es::PointerX86<CoreClass> data;
  IGHWTOCArrayCounter count;
  uint32 size;

  template <class C> auto Iter() const {
    if (C::ID != id) {
      throw std::bad_cast{};
    }

    IGHWTOCIteratorConst<C> iter;
    iter.begin_ = static_cast<const C *>(static_cast<const CoreClass *>(data));

    if (count.ArrayType() == IGHWTOCArrayType::Buffer) {
      iter.end_ = static_cast<const C *>(data.Get() + size);
    } else {
      if (sizeof(C) != size) {
        throw std::bad_cast{};
      }
      iter.end_ = iter.begin_ + count.Count();
    }

    return iter;
  }

  template <class C> bool IsClass() const { return C::ID == id; }
};

struct IGHWHeader {
  static constexpr uint32 ID = CompileFourCC("WHGI");
  uint32 id;
  uint16 versionMajor;
  uint16 versionMinor;
  uint32 numToc;
  uint32 tocEnd;
  uint32 dataEnd;
  uint32 numFixups;
  uint64 DEADDEAD;
};

enum class Version {
  RFOM,
  TOD,
  V2,
  R3,
};

struct IGHW {
  void IS_EXTERN FromStream(BinReaderRef_e rd, Version version);
  auto Header() const {
    return reinterpret_cast<const IGHWHeader *>(buffer.data());
  }
  auto begin() const {
    return reinterpret_cast<const IGHWTOC *>(buffer.data() +
                                             sizeof(IGHWHeader));
  }
  auto end() const {
    return reinterpret_cast<const IGHWTOC *>(buffer.data() + Header()->tocEnd);
  }
  auto begin() {
    return const_cast<IGHWTOC *>(static_cast<const IGHW *>(this)->begin());
  }
  auto end() {
    return const_cast<IGHWTOC *>(static_cast<const IGHW *>(this)->end());
  }

private:
  auto Header() { return reinterpret_cast<IGHWHeader *>(buffer.data()); }
  std::string buffer;
};

template <class... C, class CB>
void CatchClassesLambda(IGHW &main, CB &&callback,
                        IGHWTOCIteratorConst<C> &...classes) {
  bool catched = false;
  auto CatchClass = [&](const IGHWTOC &toc, auto &item) {
    using type = typename std::remove_reference_t<decltype(item)>::value_type;
    if (toc.IsClass<type>()) {
      item = toc.Iter<type>();
      catched = true;
    }
  };

  for (auto &i : main) {
    (CatchClass(i, classes), ...);

    if (catched) {
      catched = false;
    } else {
      callback(i);
    }
  }
}

template <class... C>
void CatchClasses(IGHW &main, IGHWTOCIteratorConst<C> &...classes) {
  auto dummy = [](const IGHWTOC &) {};
  CatchClassesLambda(main, dummy, classes...);
}
