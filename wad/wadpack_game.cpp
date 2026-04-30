#include "spike/io/binwritter.hpp"
#include "spike/io/directory_scanner.hpp"
#include "spike/master_printer.hpp"
#include "spike/util/supercore.hpp"
#include "wad.hpp"
#include <chrono>
#include <fstream>
#include <iomanip>
#include <map>
#include <sys/stat.h>

struct FileInfo {
  uint64 size;
  uint64 mtime;
  Item range{};
};

int main(int argc, char *argv[]) {
  std::map<std::string, FileInfo> allFiles;
  es::print::AddPrinterFunction(es::Print);

  {
    DirectoryScanner scan;
    PathFilter ignored;
    ignored.AddFilter(".wad$");
    ignored.AddFilter(".wad.lst$");
    ignored.AddFilter("^auto_generated_padding_*.bin$");
    ignored.AddFilter(".dds$");
    ignored.AddFilter(".bak$");
    ignored.AddFilter(".json$");
    ignored.AddFilter(".self$");
    ignored.AddFilter(".sprx$");
    std::string rootFolder(argv[1]);
    scan.Scan(rootFolder + "/built");
    scan.Scan(rootFolder + "/packed");

    for (auto &f : scan) {
      if (ignored.IsFiltered(f)) {
        continue;
      }

      struct stat fileStat;
      stat(f.c_str(), &fileStat);

      const uint64 fileSize = fileStat.st_size;
      const uint64 lastWriteTime = fileStat.st_mtim.tv_sec;

      std::string_view sv(f);
      sv.remove_prefix(rootFolder.size() + 1);
      allFiles.emplace(sv, FileInfo{fileSize, lastWriteTime});
    }

    scan.Clear();
    scan.Scan(rootFolder + "/game");
    for (auto &f : scan) {
      if (ignored.IsFiltered(f)) {
        continue;
      }

      struct stat fileStat;
      stat(f.c_str(), &fileStat);

      const uint64 fileSize = fileStat.st_size;
      const uint64 lastWriteTime = fileStat.st_mtim.tv_sec;

      std::string_view sv(f);
      sv.remove_prefix(rootFolder.size() + 6);
      allFiles.emplace(sv, FileInfo{fileSize, lastWriteTime});
    }

    scan.Clear();
    scan.ScanFolders(rootFolder + "/data");
    for (auto &fld : scan) {
      DirectoryScanner scan2;
      scan2.Scan(fld);
      for (auto &f : scan2) {
        if (ignored.IsFiltered(f)) {
          continue;
        }

        struct stat fileStat;
        stat(f.c_str(), &fileStat);

        const uint64 fileSize = fileStat.st_size;
        const uint64 lastWriteTime = fileStat.st_mtim.tv_sec;

        std::string_view sv(f);
        sv.remove_prefix(fld.size() + 1);
        allFiles[std::string(sv)] = FileInfo{fileSize, lastWriteTime};
      }
    }
  }

  uint64 curLen = 0;
  std::map<uint32, std::map<std::string_view, FileInfo *>> levels;
  std::map<std::string_view,
           std::map<uint32, std::pair<FileInfo *, FileInfo *>>>
      banks;

  struct PlateLess {
    bool operator()(std::string_view s1, std::string_view s2) const {
      if (s1.starts_with("level") && s2.starts_with("level")) {
        uint32 l1;
        sscanf(s1.data(), "level%d%*s", &l1);
        uint32 l2;
        sscanf(s2.data(), "level%d%*s", &l2);

        return l1 < l2;
      }

      return s1 < s2;
    }
  };

  std::map<std::string_view, std::pair<FileInfo *, FileInfo *>, PlateLess>
      guiPlatesLevels;
  std::pair<FileInfo *, FileInfo *> guiPlateTitle;
  std::pair<FileInfo *, FileInfo *> guiPlateHuman;
  std::pair<FileInfo *, FileInfo *> guiPlateHybrid;
  std::pair<FileInfo *, FileInfo *> guiPlate4Human;

  for (auto &[path, data] : allFiles) {
    if (path.starts_with("packed/movies") || path.ends_with("ps3debug.dat")) {
      continue;
    }
    curLen += GetPadding(curLen, 0x800);
    data.range.chunk = curLen / CHUNK_SIZE;
    data.range.range.start = curLen % CHUNK_SIZE;
    data.range.range.count = data.size + 0x7ff;
    curLen += data.size;

    if (path.starts_with("packed/levels")) {
      uint32 levelId;
      sscanf(path.c_str(), "packed/levels/level%d/%*s", &levelId);
      const size_t found = path.find_last_of('/');
      std::string_view sv(path);
      sv.remove_prefix(found + 1);
      levels[levelId].emplace(sv, &data);
    } else if (path.starts_with("data/levels")) {
      uint32 levelId;
      sscanf(path.c_str(), "data/levels/level%d/%*s", &levelId);
      const size_t found = path.find_last_of('/');
      std::string_view sv(path);
      sv.remove_prefix(found + 1);
      levels[levelId].emplace(sv, &data);
    } else if (path.starts_with("built/anark/")) {
      std::string_view sv(path);
      sv.remove_prefix(12);
      const size_t found = sv.find_first_of('/');
      std::string_view groupName = sv.substr(0, found);
      sv.remove_prefix(found + 1);

      if (sv.starts_with("images/")) {
        sv.remove_prefix(7);
        uint32 bankId = 0;
        sscanf(sv.data(), "texturebank_%d.%*s", &bankId);
        auto &pair = banks[groupName][bankId];

        if (sv.ends_with(".bnkh")) {
          pair.first = &data;
        } else {
          pair.second = &data;
        }
      }
    } else if (path.starts_with("built/gui/plates/")) {
      const size_t found = path.find_last_of('/');
      std::string_view sv(path);
      sv.remove_prefix(found + 1);

      if (sv.ends_with(".tp")) {
        sv.remove_suffix(3);

        if (sv == "title") {
          guiPlateTitle.first = &data;
        } else if (sv == "mp_human") {
          guiPlateHuman.first = &data;
        } else if (sv == "mp_4human") {
          guiPlate4Human.first = &data;
        } else if (sv == "mp_hybrid") {
          guiPlateHybrid.first = &data;
        } else {
          guiPlatesLevels[sv].first = &data;
        }
      } else {
        sv.remove_suffix(4);
        if (sv == "title") {
          guiPlateTitle.second = &data;
        } else if (sv == "mp_human") {
          guiPlateHuman.second = &data;
        } else if (sv == "mp_4human") {
          guiPlate4Human.second = &data;
        } else if (sv == "mp_hybrid") {
          guiPlateHybrid.second = &data;
        } else {
          guiPlatesLevels[sv].second = &data;
        }
      }
    }
  }

  auto LoadLocalizedImpl = [](LocalizedFiles &out, std::string_view prefix,
                              std::string_view sufix, auto &items) {
    static const char chars[]{'e', 'u', 'f', 'i', 'g', 's', 'd', 'p', 'j', 'k'};
    std::string built(prefix);
    built.push_back('.');
    const size_t modIndex = built.size();
    built.append(" .");
    built.append(sufix);

    for (uint32 idx = 0; auto c : chars) {
      built.at(modIndex) = c;
      if (items.contains(built)) {
        if constexpr (std::is_pointer_v<typename std::decay_t<
                          decltype(items)>::mapped_type>) {
          out.files[idx] = items.at(built)->range;
        } else {
          out.files[idx] = items.at(built).range;
        }
      } else {
        PrintWarning("Cannot locate file: ", built);
      }

      idx++;
    }
  };

  auto LoadLocalized = [&allFiles, &LoadLocalizedImpl](LocalizedFiles &out,
                                                       std::string_view prefix,
                                                       std::string_view sufix) {
    LoadLocalizedImpl(out, prefix, sufix, allFiles);
  };

  auto LoadFontGroup = [&allFiles](FontGroup &out, std::string prefix) {
    out.data = allFiles.at(prefix + ".dat").range;
    out.textureBank = allFiles.at(prefix + ".tp").range;
    out.textureBankHeader = allFiles.at(prefix + ".tph").range;
  };

  auto LoadAnarkGroup = [&allFiles, &LoadLocalized](AnarkGroup &out,
                                                    std::string name,
                                                    std::string locName) {
    out.anarkFile =
        allFiles.at("built/anark/" + locName + "/" + name + ".ig").range;
    std::string tagName = "built/localization/" + locName + "_loc_text_tags";
    LoadLocalized(out.textTags, tagName, "pkg");
  };

  GameWAD gameWad{};
  gameWad.ps3EffectsTextureBank =
      allFiles.at("built/effects/ps3effects.tp").range;
  gameWad.ps3EffectsTextureBankHeader =
      allFiles.at("built/effects/ps3effects.tph").range;
  LoadLocalized(gameWad.coreTextTags, "built/localization/core_text_tags",
                "pkg");
  LoadFontGroup(gameWad.fonts, "built/fonts/fonts");
  LoadFontGroup(gameWad.ps3fonts, "built/fonts/ps3fonts");
  LoadFontGroup(gameWad.japaneseFonts, "built/fonts/japanese/fonts");
  gameWad.ps3Config = allFiles.at("packed/game/ps3config.dat").range;
  LoadLocalized(gameWad.ps3ConfigTags, "packed/game/ps3config", "pkg");
  LoadLocalized(gameWad.ps3Dialogue, "built/sound/global/ps3dialogue", "dat");
  LoadLocalized(gameWad.ps3DialogueStream,
                "built/sound/global/ps3dialoguestream", "dat");
  LoadAnarkGroup(gameWad.anarkFrontEnd, "frontend", "frontend");
  gameWad.anarkFrontEnd.ps3FxConduit =
      allFiles.at("built/fxconduit/frontend/ps3fxconduit.dat").range;
  gameWad.anarkFrontEnd.ps3Sound =
      allFiles.at("built/fxconduit/frontend/ps3sound.dat").range;
  gameWad.anarkFrontEnd.ps3SoundStream =
      allFiles.at("built/fxconduit/frontend/ps3soundstream.dat").range;
  LoadAnarkGroup(gameWad.anarkLobby, "i8lobby", "lobby");
  LoadAnarkGroup(gameWad.anarkIntel, "intel", "intel");
  LoadAnarkGroup(gameWad.anarkPause, "pause_menu", "pause");
  LoadAnarkGroup(gameWad.anarkCredits, "credits", "credits");
  gameWad.ps3GuiTextureBank = allFiles.at("built/gui/ps3gui.tp").range;
  gameWad.ps3GuiTextureBankHeader = allFiles.at("built/gui/ps3gui.tph").range;
  gameWad.anarkVersion = allFiles.at("data/anark/version.dat").range;
  LoadLocalized(gameWad.splashTextureBank, "art/hud/splash/splash", "tp");
  LoadLocalized(gameWad.splashTextureBankHeader, "art/hud/splash/splash",
                "tph");
  gameWad.credits = allFiles.at("built/sound/global/credits.dat").range;
  gameWad.creditsStream =
      allFiles.at("built/sound/global/creditsstream.dat").range;
  gameWad.frontEnd = allFiles.at("built/sound/global/frontend.dat").range;
  gameWad.saveIcon0 = allFiles.at("art/disc/savedata/icon0.png").range;
  gameWad.saveIcon0_00 = allFiles.at("art/disc/savedata/icon0_00.png").range;
  gameWad.saveIcon0_09 = allFiles.at("art/disc/savedata/icon0_09.png").range;
  gameWad.gameDataPic1 = allFiles.at("art/disc/gamedata/pic1.png").range;
  gameWad.gameDataIcon0 = allFiles.at("art/disc/gamedata/icon0.png").range;
  gameWad.gameDataIcon0_00 =
      allFiles.at("art/disc/gamedata/icon0_00.png").range;
  gameWad.gameDataIcon0_09 =
      allFiles.at("art/disc/gamedata/icon0_09.png").range;

  BinWritter wrm("game.wad");
  BinWritterRef_e wr(wrm);
  wr.SwapEndian(true);
  wr.Write(gameWad);
  wr.Write(uint32(levels.size()));
  gameWad.levels = wr.Tell();
  for (auto &[lid, items] : levels) {
    auto OptFile = [&items](Item &out, std::string_view name) {
      if (items.contains(name)) {
        out = items.at(name)->range;
      }
    };

    LevelGroup level{};
    level.levelID = lid;
    OptFile(level.ps3FxConduit, "ps3fxconduit.dat");
    OptFile(level.ps3Sound, "ps3sound.dat");
    OptFile(level.decals, "decals_ps3.frz");
    OptFile(level.levelVerts, "ps3levelverts.dat");
    OptFile(level.levelTexs, "ps3leveltexs.dat");
    OptFile(level.levelMain, "ps3levelmain.dat");
    OptFile(level.levelColl, "ps3levelcoll.dat");
    OptFile(level.ps3Fx, "ps3fx.dat");
    OptFile(level.ps3FxSystemTextureBank, "ps3fxsystem.tp");
    OptFile(level.ps3FxSystemTextureBankHeader, "ps3fxsystem.tph");
    OptFile(level.ps3GamePlay, "ps3gameplay.dat");
    LoadLocalizedImpl(level.ps3GamePlayTags, "ps3gameplay", "pkg", items);
    OptFile(level.ps3Script, "ps3script.dat");
    LoadLocalizedImpl(level.ps3ScriptTags, "ps3script", "pkg", items);
    LoadLocalizedImpl(level.ps3Dialogue, "ps3dialogue", "dat", items);
    LoadLocalizedImpl(level.ps3DialogueStream, "ps3dialoguestream", "dat",
                      items);
    LoadLocalizedImpl(level.ps3LipSync, "ps3lipsync", "dat", items);
    LoadLocalizedImpl(level.ps3DialogueTags, "ps3dialogue", "pkg", items);
    OptFile(level.introPadData, "intropaddata.dat");
    OptFile(level.largedmTextureBank, "largedm_ps3guimaptexture.tp");
    OptFile(level.largedmTextureBankHeader, "largedm_ps3guimaptexture.tph");
    OptFile(level.mediumdmTextureBank, "mediumdm_ps3guimaptexture.tp");
    OptFile(level.mediumdmTextureBankHeader, "mediumdm_ps3guimaptexture.tph");
    OptFile(level.smalldmTextureBank, "smalldm_ps3guimaptexture.tp");
    OptFile(level.smalldmTextureBankHeader, "smalldm_ps3guimaptexture.tph");
    OptFile(level.tinydmTextureBank, "tinydm_ps3guimaptexture.tp");
    OptFile(level.tinydmTextureBankHeader, "tinydm_ps3guimaptexture.tph");
    OptFile(level.largenodeTextureBank, "largenode_ps3guimaptexture.tp");
    OptFile(level.largenodeTextureBankHeader, "largenode_ps3guimaptexture.tph");
    OptFile(level.mediumnodeTextureBank, "mediumnode_ps3guimaptexture.tp");
    OptFile(level.mediumnodeTextureBankHeader,
            "mediumnode_ps3guimaptexture.tph");
    OptFile(level.smallnodeTextureBank, "smallnode_ps3guimaptexture.tp");
    OptFile(level.smallnodeTextureBankHeader, "smallnode_ps3guimaptexture.tph");
    OptFile(level.ps3SoundStream, "ps3soundstream.dat");
    wr.Write(level);
  }

  {
    wr.Write(uint32(banks["frontend"].size()));
    gameWad.anarkFrontEnd.textureBankHeaders = wr.Tell();
    for (auto &[_, p] : banks["frontend"]) {
      wr.Write(p.first->range);
    }

    wr.Write(uint32(banks["frontend"].size()));
    gameWad.anarkFrontEnd.textureBanks = wr.Tell();
    for (auto &[_, p] : banks["frontend"]) {
      wr.Write(p.second->range);
    }
  }

  {
    wr.Write(uint32(banks["lobby"].size()));
    gameWad.anarkLobby.textureBankHeaders = wr.Tell();
    for (auto &[_, p] : banks["lobby"]) {
      wr.Write(p.first->range);
    }

    wr.Write(uint32(banks["lobby"].size()));
    gameWad.anarkLobby.textureBanks = wr.Tell();
    for (auto &[_, p] : banks["lobby"]) {
      wr.Write(p.second->range);
    }
  }

  {
    wr.Write(uint32(banks["intel"].size()));
    gameWad.anarkIntel.textureBankHeaders = wr.Tell();
    for (auto &[_, p] : banks["intel"]) {
      wr.Write(p.first->range);
    }

    wr.Write(uint32(banks["intel"].size()));
    gameWad.anarkIntel.textureBanks = wr.Tell();
    for (auto &[_, p] : banks["intel"]) {
      wr.Write(p.second->range);
    }
  }

  {
    wr.Write(uint32(banks["pause"].size()));
    gameWad.anarkPause.textureBankHeaders = wr.Tell();
    for (auto &[_, p] : banks["pause"]) {
      wr.Write(p.first->range);
    }

    wr.Write(uint32(banks["pause"].size()));
    gameWad.anarkPause.textureBanks = wr.Tell();
    for (auto &[_, p] : banks["pause"]) {
      wr.Write(p.second->range);
    }
  }

  {
    wr.Write(uint32(banks["credits"].size()));
    gameWad.anarkCredits.textureBankHeaders = wr.Tell();
    for (auto &[_, p] : banks["credits"]) {
      wr.Write(p.first->range);
    }

    wr.Write(uint32(banks["credits"].size()));
    gameWad.anarkCredits.textureBanks = wr.Tell();
    for (auto &[_, p] : banks["credits"]) {
      wr.Write(p.second->range);
    }
  }

  wr.Write(uint32(guiPlatesLevels.size() + 4));
  gameWad.guiPlates = wr.Tell();
  wr.Write(guiPlateTitle.first->range);
  wr.Write(guiPlateTitle.second->range);
  wr.Write(guiPlateHuman.first->range);
  wr.Write(guiPlateHuman.second->range);
  wr.Write(guiPlateHybrid.first->range);
  wr.Write(guiPlateHybrid.second->range);

  for (auto &[_, p] : guiPlatesLevels) {
    wr.Write(p.first->range);
    wr.Write(p.second->range);
  }

  wr.Write(guiPlate4Human.first->range);
  wr.Write(guiPlate4Human.second->range);

  wr.Seek(0);
  wr.Write(gameWad);

  std::ofstream lst("game.wad.lst");
  lst << "HEADER:          " << sizeof(GameWAD) << '\n';
  lst << "EXTENDED:        " << wr.GetSize() << '\n';
  lst << "MODIFIED_TIME:   "
      << std::chrono::duration_cast<std::chrono::seconds>(
             std::chrono::system_clock::now().time_since_epoch())
             .count()
      << '\n';
  lst << "LENGTH:          " << curLen % CHUNK_SIZE << '\n';
  lst << "FILE_LIST:\n";

  for (auto &[path, data] : allFiles) {
    if (path.starts_with("packed/movies") || path.ends_with("ps3debug.dat")) {
      continue;
    }
    lst << 7 << std::setw(4) << 2 << std::setw(4) << data.range.chunk
        << std::setw(15) << data.range.range.start << std::setw(15) << data.size
        << std::setw(15) << data.mtime << "   " << path << "   x:/i8   " << 1
        << '\n';
  }

  return 0;
}
