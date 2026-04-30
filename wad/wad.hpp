#pragma once
#include "spike/util/endian.hpp"

constexpr uint32 CHUNK_SIZE = 0x40000000;

struct ListFile {
  bool isPacked;
  uint8 unk0;
  uint8 unk1;
  uint8 chunk;
  uint32 offset;
  uint32 size;
  uint32 timestamp;
  std::string filePathLocal;
  std::string filePathWorkDir;
};

struct Sector {
  uint32 value;

  operator uint64() const { return value * 0x800; }
  void operator=(uint32 input) { value = input / 0x800; }

  void Swap();
};

struct SectorRange {
  Sector start;
  Sector count;
};

struct Item {
  uint32 chunk;
  SectorRange range;
};

struct LocalizedFiles {
  Item files[10];
};

struct LevelGroup {
  uint32 levelID;
  uint32 timestamp;
  Item null;
  Item ps3FxConduit;
  Item ps3Sound;
  Item decals;
  Item levelVerts;
  Item levelTexs;
  Item levelMain;
  Item levelColl;
  Item ps3Fx;
  Item ps3FxSystemTextureBank;
  Item ps3FxSystemTextureBankHeader;
  Item ps3GamePlay;
  LocalizedFiles ps3GamePlayTags;
  Item ps3Script;
  LocalizedFiles ps3ScriptTags;
  LocalizedFiles ps3Dialogue;
  LocalizedFiles ps3DialogueStream;
  LocalizedFiles ps3LipSync;
  LocalizedFiles ps3DialogueTags;
  Item introPadData;
  Item largedmTextureBank;
  Item largedmTextureBankHeader;
  Item mediumdmTextureBank;
  Item mediumdmTextureBankHeader;
  Item smalldmTextureBank;
  Item smalldmTextureBankHeader;
  Item tinydmTextureBank;
  Item tinydmTextureBankHeader;
  Item largenodeTextureBank;
  Item largenodeTextureBankHeader;
  Item mediumnodeTextureBank;
  Item mediumnodeTextureBankHeader;
  Item smallnodeTextureBank;
  Item smallnodeTextureBankHeader;
  Item ps3SoundStream;
};

struct FontGroup {
  Item data;
  Item textureBank;
  Item textureBankHeader;
};

struct AnarkGroup {
  Item anarkFile;
  uint32 textureBanks;
  uint32 textureBankHeaders;
  LocalizedFiles textTags;
};

struct AnarkGroupFrontEnd : AnarkGroup {
  Item ps3FxConduit;
  Item ps3Sound;
  Item ps3SoundStream;
};

struct GameWAD {
  uint32 blockSize = 0x800;
  uint32 levels;
  Item ps3EffectsTextureBank;
  Item ps3EffectsTextureBankHeader;
  LocalizedFiles coreTextTags;
  FontGroup fonts;
  FontGroup ps3fonts;
  FontGroup japaneseFonts;
  Item ps3Config;
  LocalizedFiles ps3ConfigTags;
  LocalizedFiles ps3Dialogue;
  LocalizedFiles ps3DialogueStream;
  AnarkGroupFrontEnd anarkFrontEnd;
  AnarkGroup anarkLobby;
  AnarkGroup anarkIntel;
  AnarkGroup anarkPause;
  AnarkGroup anarkCredits;
  Item ps3GuiTextureBank;
  Item ps3GuiTextureBankHeader;
  uint32 guiPlates;
  Item anarkVersion;
  LocalizedFiles splashTextureBank;
  LocalizedFiles splashTextureBankHeader;
  Item credits;
  Item creditsStream;
  Item frontEnd;
  Item saveIcon0;
  Item saveIcon0_00;
  Item saveIcon0_09;
  Item gameDataPic1;
  Item gameDataIcon0;
  Item gameDataIcon0_00;
  Item gameDataIcon0_09;
};

struct MovieHeader {
  uint32 unk0[3];
  float unk1;
};

struct MovieWAD : MovieHeader {
  Item movieStream;
  LocalizedFiles subtitles;
};

struct MpegWAD {
  uint32 movies;
};

template <> inline void FByteswapper(SectorRange &item, bool) {
  FByteswapper(item.start);
  FByteswapper(item.count);
}

template <> inline void FByteswapper(Item &item, bool) {
  FByteswapper(item.chunk);
  FByteswapper(item.range);
}

template <> inline void FByteswapper(LocalizedFiles &item, bool) {
  FByteswapper(item.files);
}

template <> inline void FByteswapper(LevelGroup &item, bool) {
  FByteswapper(item.levelID);
  FByteswapper(item.timestamp);
  FByteswapper(item.null);
  FByteswapper(item.ps3FxConduit);
  FByteswapper(item.ps3Sound);
  FByteswapper(item.decals);
  FByteswapper(item.levelVerts);
  FByteswapper(item.levelTexs);
  FByteswapper(item.levelMain);
  FByteswapper(item.levelColl);
  FByteswapper(item.ps3Fx);
  FByteswapper(item.ps3FxSystemTextureBank);
  FByteswapper(item.ps3FxSystemTextureBankHeader);
  FByteswapper(item.ps3GamePlay);
  FByteswapper(item.ps3GamePlayTags);
  FByteswapper(item.ps3Script);
  FByteswapper(item.ps3ScriptTags);
  FByteswapper(item.ps3Dialogue);
  FByteswapper(item.ps3DialogueStream);
  FByteswapper(item.ps3LipSync);
  FByteswapper(item.ps3DialogueTags);
  FByteswapper(item.introPadData);
  FByteswapper(item.introPadData);
  FByteswapper(item.largedmTextureBank);
  FByteswapper(item.largedmTextureBankHeader);
  FByteswapper(item.mediumdmTextureBank);
  FByteswapper(item.mediumdmTextureBankHeader);
  FByteswapper(item.smalldmTextureBank);
  FByteswapper(item.smalldmTextureBankHeader);
  FByteswapper(item.tinydmTextureBank);
  FByteswapper(item.tinydmTextureBankHeader);
  FByteswapper(item.largenodeTextureBank);
  FByteswapper(item.largenodeTextureBankHeader);
  FByteswapper(item.mediumnodeTextureBank);
  FByteswapper(item.mediumnodeTextureBankHeader);
  FByteswapper(item.smallnodeTextureBank);
  FByteswapper(item.smallnodeTextureBankHeader);
  FByteswapper(item.ps3SoundStream);
}

template <> inline void FByteswapper(FontGroup &item, bool) {
  FByteswapper(item.data);
  FByteswapper(item.textureBank);
  FByteswapper(item.textureBankHeader);
}

template <> inline void FByteswapper(AnarkGroup &item, bool) {
  FByteswapper(item.anarkFile);
  FByteswapper(item.textureBanks);
  FByteswapper(item.textureBankHeaders);
  FByteswapper(item.textTags);
}

template <> inline void FByteswapper(AnarkGroupFrontEnd &item, bool) {
  FByteswapper(static_cast<AnarkGroup &>(item));
  FByteswapper(item.ps3FxConduit);
  FByteswapper(item.ps3Sound);
  FByteswapper(item.ps3SoundStream);
}

template <> inline void FByteswapper(GameWAD &item, bool) {
  FByteswapper(item.blockSize);
  FByteswapper(item.levels);
  FByteswapper(item.ps3EffectsTextureBank);
  FByteswapper(item.ps3EffectsTextureBankHeader);
  FByteswapper(item.coreTextTags);
  FByteswapper(item.fonts);
  FByteswapper(item.ps3fonts);
  FByteswapper(item.japaneseFonts);
  FByteswapper(item.ps3Config);
  FByteswapper(item.ps3ConfigTags);
  FByteswapper(item.ps3Dialogue);
  FByteswapper(item.ps3DialogueStream);
  FByteswapper(item.anarkFrontEnd);
  FByteswapper(item.anarkLobby);
  FByteswapper(item.anarkIntel);
  FByteswapper(item.anarkPause);
  FByteswapper(item.anarkCredits);
  FByteswapper(item.ps3GuiTextureBank);
  FByteswapper(item.ps3GuiTextureBankHeader);
  FByteswapper(item.guiPlates);
  FByteswapper(item.anarkVersion);
  FByteswapper(item.splashTextureBank);
  FByteswapper(item.splashTextureBankHeader);
  FByteswapper(item.credits);
  FByteswapper(item.creditsStream);
  FByteswapper(item.frontEnd);
  FByteswapper(item.saveIcon0);
  FByteswapper(item.saveIcon0_00);
  FByteswapper(item.saveIcon0_09);
  FByteswapper(item.gameDataPic1);
  FByteswapper(item.gameDataIcon0);
  FByteswapper(item.gameDataIcon0_00);
  FByteswapper(item.gameDataIcon0_09);
}

template <> inline void FByteswapper(std::pair<Item, Item> &item, bool) {
  FByteswapper(item.first);
  FByteswapper(item.second);
}

template <> inline void FByteswapper(MovieHeader &item, bool) {
  FByteswapper(item.unk0);
  FByteswapper(item.unk1);
}

template <> inline void FByteswapper(MovieWAD &item, bool) {
  FByteswapper(static_cast<MovieHeader&>(item));
  FByteswapper(item.movieStream);
  FByteswapper(item.subtitles);
}

template <> inline void FByteswapper(MpegWAD &item, bool) {
  FByteswapper(item.movies);
}
