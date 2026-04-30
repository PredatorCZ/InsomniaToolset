/*  InsomniaToolset GameWAD
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

#include "project.h"
#include "spike/app_context.hpp"
#include "spike/except.hpp"
#include "spike/io/binreader_stream.hpp"
#include "spike/master_printer.hpp"
#include "wad.hpp"

std::string_view filters[]{
    "^game.wad.lst$",
};

static AppInfo_s appInfo{
    .header =
        GameWAD_DESC " v" GameWAD_VERSION ", " GameWAD_COPYRIGHT "Lukas Cone",
    .filters = filters,
};

AppInfo_s *AppInitModule() { return &appInfo; }

void AppProcessFile(AppContext *ctx) {
  uint32 wadSize;
  uint32 wadHeaderSize;
  uint32 length;
  uint32 modifiedTime;
  std::vector<ListFile> files;

  std::istream &lstStr = ctx->GetStream();
  std::string line;

  while (!std::getline(lstStr, line).eof()) {
    if (line.starts_with("HEADER:")) {
      sscanf(line.c_str(), "%*s %d", &wadHeaderSize);
    } else if (line.starts_with("EXTENDED:")) {
      sscanf(line.c_str(), "%*s %d", &wadSize);
    } else if (line.starts_with("MODIFIED_TIME:")) {
      sscanf(line.c_str(), "%*s %d", &modifiedTime);
    } else if (line.starts_with("LENGTH:")) {
      sscanf(line.c_str(), "%*s %d", &length);
    } else if (line.starts_with("FILE_LIST:")) {
      break;
    }
  }

  while (!std::getline(lstStr, line).eof()) {
    ListFile curFile;
    char path[0x100];
    char folder[0x100];
    uint32 isPacked;
    uint32 unk0;
    uint32 unk1;
    uint32 chunk;
    sscanf(line.c_str(), "%d%d%d%d%d%d%s%s%d", &unk0, &unk1, &chunk,
           &curFile.offset, &curFile.size, &curFile.timestamp, path, folder,
           &isPacked);
    curFile.unk0 = unk0;
    curFile.unk1 = unk1;
    curFile.chunk = chunk;
    curFile.filePathLocal = path;
    curFile.filePathWorkDir = folder;
    curFile.isPacked = isPacked;
    files.emplace_back(curFile);
  }

  auto wadStr =
      ctx->RequestFile(std::string(ctx->workingFile.GetFullPathNoExt()));
  BinReaderRef_e rd(*wadStr.Get());
  rd.SwapEndian(true);
  GameWAD wadHdr;
  rd.Read(wadHdr);
  std::vector<LevelGroup> levels;
  rd.ReadContainer(levels);
  std::vector<Item> anarkTextureBanks[10];
  std::vector<std::pair<Item, Item>> plates;
  for (auto &i : anarkTextureBanks) {
    rd.ReadContainer(i);
  }

  rd.ReadContainer(plates);

  auto print = [&files](const char *name, Item &i) {
    const uint32 start = i.range.start;

    if (i.range.count == 0) {
      PrintInfo(name, ": ", "<empty>");
    } else {
      for (auto &f : files) {
        if (f.chunk == i.chunk && f.offset == start) {
          PrintInfo(name, ": ", f.filePathLocal);
          break;
        }
      }
    }
  };

  auto printFnt = [&print](const char *name, FontGroup &item) {
    PrintInfo(name);
    print("  data", item.data);
    print("  textureBank", item.textureBank);
    print("  textureBankHeader", item.textureBankHeader);
  };

  auto printLoc = [&print](const char *name, LocalizedFiles &item) {
    static const char chars[]{'e', 'u', 'f', 'i', 'g', 's', 'd', 'p', 'j', 'k'};
    PrintInfo(name);
    char buffer[] = "  e";

    for (uint32 idx = 0; char c : chars) {
      buffer[2] = c;
      print(buffer, item.files[idx++]);
    }
  };

  print("ps3EffectsTextureBank", wadHdr.ps3EffectsTextureBank);
  print("ps3EffectsTextureBankHeader", wadHdr.ps3EffectsTextureBankHeader);
  printLoc("coreTextTags", wadHdr.coreTextTags);
  printFnt("fonts", wadHdr.fonts);
  printFnt("ps3fonts", wadHdr.ps3fonts);
  printFnt("japaneseFonts", wadHdr.japaneseFonts);
  print("ps3Config", wadHdr.ps3Config);
  printLoc("ps3ConfigTags", wadHdr.ps3ConfigTags);
  printLoc("ps3Dialogue", wadHdr.ps3Dialogue);
  printLoc("ps3DialogueStream", wadHdr.ps3DialogueStream);
  /*AnarkGroupFrontEnd anarkFrontEnd;
  AnarkGroup anarkLobby;
  AnarkGroup anarkIntel;
  AnarkGroup anarkPause;
  AnarkGroup anarkCredits;*/
  print("ps3GuiTextureBank", wadHdr.ps3GuiTextureBank);
  print("ps3GuiTextureBankHeader", wadHdr.ps3GuiTextureBankHeader);
  print("anarkVersion", wadHdr.anarkVersion);
  printLoc("splashTextureBank", wadHdr.splashTextureBank);
  printLoc("splashTextureBankHeader", wadHdr.splashTextureBankHeader);
  print("credits", wadHdr.credits);
  print("creditsStream", wadHdr.creditsStream);
  print("frontEnd", wadHdr.frontEnd);
  print("saveIcon0", wadHdr.saveIcon0);
  print("saveIcon0_00", wadHdr.saveIcon0_00);
  print("saveIcon0_09", wadHdr.saveIcon0_09);
  print("gameDataPic1", wadHdr.gameDataPic1);
  print("gameDataIcon0", wadHdr.gameDataIcon0);
  print("gameDataIcon0_00", wadHdr.gameDataIcon0_00);
  print("gameDataIcon0_09", wadHdr.gameDataIcon0_09);

  for (auto &l : levels) {
    print("  null", l.null);
    print("  ps3FxConduit", l.ps3FxConduit);
    print("  ps3Sound", l.ps3Sound);
    print("  decals", l.decals);
    print("  levelVerts", l.levelVerts);
    print("  levelTexs", l.levelTexs);
    print("  levelMain", l.levelMain);
    print("  levelColl", l.levelColl);
    print("  ps3Fx", l.ps3Fx);
    print("  ps3FxSystemTextureBank", l.ps3FxSystemTextureBank);
    print("  ps3FxSystemTextureBankHeader", l.ps3FxSystemTextureBankHeader);
    print("  ps3GamePlay", l.ps3GamePlay);
    printLoc("  ps3GamePlayTags", l.ps3GamePlayTags);
    print("  ps3Script", l.ps3Script);
    printLoc("  ps3ScriptTags", l.ps3ScriptTags);
    printLoc("  ps3Dialogue", l.ps3Dialogue);
    printLoc("  ps3DialogueStream", l.ps3DialogueStream);
    printLoc("  ps3LipSync", l.ps3LipSync);
    printLoc("  ps3DialogueTags", l.ps3DialogueTags);
    print("  introPadData", l.introPadData);
    print("  largedmTextureBank", l.largedmTextureBank);
    print("  largedmTextureBankHeader", l.largedmTextureBankHeader);
    print("  mediumdmTextureBank", l.mediumdmTextureBank);
    print("  mediumdmTextureBankHeader", l.mediumdmTextureBankHeader);
    print("  smalldmTextureBank", l.smalldmTextureBank);
    print("  smalldmTextureBankHeader", l.smalldmTextureBankHeader);
    print("  tinydmTextureBank", l.tinydmTextureBank);
    print("  tinydmTextureBankHeader", l.tinydmTextureBankHeader);
    print("  largenodeTextureBank", l.largenodeTextureBank);
    print("  largenodeTextureBankHeader", l.largenodeTextureBankHeader);
    print("  mediumnodeTextureBank", l.mediumnodeTextureBank);
    print("  mediumnodeTextureBankHeader", l.mediumnodeTextureBankHeader);
    print("  smallnodeTextureBank", l.smallnodeTextureBank);
    print("  smallnodeTextureBankHeader", l.smallnodeTextureBankHeader);
    print("  ps3SoundStream", l.ps3SoundStream);
  }

  for (auto &t : anarkTextureBanks) {
    for (auto &i : t) {
      print("anarkTextureBank", i);
    }
  }

  for (auto &[i, j] : plates) {
    print("plateTextureBank", i);
    print("plateTextureBankHeader", j);
  }
}
