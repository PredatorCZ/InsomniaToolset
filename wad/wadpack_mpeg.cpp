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

std::pair<std::string_view, MovieHeader>
MakeMovie(uint32 unk0, uint32 unk2, float unk3, std::string_view name) {
  return {
      name,
      MovieHeader{
          .unk0 = {unk0, 0, unk2},
          .unk1 = unk3,
      },
  };
}

std::map<std::string_view, MovieHeader> MOVIES{
    MakeMovie(20, 1, 0.9, "l01_s002_0_a_0100_ci"),
    MakeMovie(21, 1, 0.95, "l02_s011_0_a_0210_ci"),
    MakeMovie(22, 1, 0.9, "l02_s019_0_a_0220_ci"),
    MakeMovie(30, 1, 0.85, "l02_s023_0_a_0300_ct"),
    MakeMovie(3, 0, 0.95, "l03_s026_0_a_park_ci"),
    MakeMovie(31, 1, 0.9, "l03_s029_0_a_0310_ci"),
    MakeMovie(3, 0, 1, "l03_s030_0_b_pupa_ci"),
    MakeMovie(32, 1, 0.9, "l03_s031_0_a_0320_ci"),
    MakeMovie(40, 1, 0.925, "l03_s035_0_a_0400_ct"),
    MakeMovie(41, 1, 0.775, "l04_s042_0_a_0410_ci"),
    MakeMovie(42, 1, 0.8, "l04_s048_0_a_0420_ci"),
    MakeMovie(50, 1, 0.8, "l04_s053_0_a_0500_ct"),
    MakeMovie(51, 1, 0.9, "l05_s054_2_a_0510_ci"),
    MakeMovie(52, 1, 0.8, "l05_s057_0_a_0520_ci"),
    MakeMovie(60, 1, 0.85, "l05_s061_0_a_0600_ct"),
    MakeMovie(61, 1, 0.9, "l06_s063_1_a_0610_ci"),
    MakeMovie(62, 1, 0.875, "l06_s064_0_a_0620_ci"),
    MakeMovie(70, 1, 0.925, "l06_s067_0_a_0700_ct"),
    MakeMovie(71, 1, 0.85, "l07_s068_0_a_0710_ci"),
    MakeMovie(72, 1, 0.9, "l07_s068_2_a_0720_ci"),
    MakeMovie(7, 0, 0.85, "l07_s069_0_a_nod1_ci"),
    MakeMovie(80, 1, 0.8, "l07_s070_0_a_0800_ct"),
    MakeMovie(81, 1, 0.8, "l08_s073_0_a_0810_ci"),
    MakeMovie(82, 1, 0.85, "l08_s079_0_a_0820_ci"),
    MakeMovie(90, 1, 0.875, "l08_s083_0_a_0900_ct"),
    MakeMovie(91, 1, 0.9, "l09_s084_0_a_0910_ci"),
    MakeMovie(100, 1, 0.9, "l09_s085_0_a_1000_ct"),
    MakeMovie(101, 1, 0.775, "l10_s086_0_a_1010_ci"),
    MakeMovie(110, 1, 0.75, "l10_s091_0_a_1100_ct"),
    MakeMovie(11, 0, 1, "l11_s090_1_a_ldnb_ci"),
    MakeMovie(111, 1, 1, "l11_s091_6_a_1110_ci"),
    MakeMovie(112, 1, 0.775, "l11_s091_3_a_1120_ci"),
    MakeMovie(120, 1, 0.775, "l11_s095_0_a_1200_ct"),
    MakeMovie(121, 1, 0.825, "l11_s095_1_a_1210_ct"),
    MakeMovie(12, 0, 0.85, "l12_s098_0_a_core_ci"),
    MakeMovie(12, 0, 0.875, "l12_s100_0_a_flam_ct"),
    MakeMovie(12, 0, 0.875, "l12_s101_0_a_blak_ct"),
    MakeMovie(0, 0, 1, "l13_s104_0_a_attr_xtra"),
    MakeMovie(0, 0, 1, "l13_s104_0_a_attrj_xtra"),
    MakeMovie(0, 0, 1, "l13_s104_0_a_attrk_xtra"),
    MakeMovie(0, 0, 1, "l13_s103_0_a_intro_xtra"),
    MakeMovie(0, 0, 1, "l13_s103_0_a_introj_xtra"),
    MakeMovie(0, 0, 1, "l13_s103_0_a_introk_xtra"),
    MakeMovie(0, 0, 1, "l13_s105_0_a_lp20_xtra"),
    MakeMovie(0, 0, 1, "l13_s105_0_a_lp20j_xtra"),
    MakeMovie(0, 0, 1, "l13_s105_0_a_lp20k_xtra"),
    MakeMovie(0, 0, 1, "l13_s107_0_a_lp42_xtra"),
    MakeMovie(0, 0, 1, "l13_s107_0_a_lp42j_xtra"),
    MakeMovie(0, 0, 1, "l13_s107_0_a_lp42k_xtra"),
    MakeMovie(0, 0, 1, "l13_s108_0_a_lp52_xtra"),
    MakeMovie(0, 0, 1, "l13_s108_0_a_lp52j_xtra"),
    MakeMovie(0, 0, 1, "l13_s108_0_a_lp52k_xtra"),
    MakeMovie(0, 0, 1, "l13_s109_0_a_lp62_xtra"),
    MakeMovie(0, 0, 1, "l13_s109_0_a_lp62j_xtra"),
    MakeMovie(0, 0, 1, "l13_s109_0_a_lp62k_xtra"),
    MakeMovie(0, 0, 1, "l13_s110_0_a_lp111_xtra"),
    MakeMovie(0, 0, 1, "l13_s110_0_a_lp111j_xtra"),
    MakeMovie(0, 0, 1, "l13_s110_0_a_lp111k_xtra"),
    MakeMovie(0, 0, 0.85, "l12_s098_0_a_corecut_ci"),
    MakeMovie(0, 0, 0.85, "l13_s102_0_a_make_xtra")

};

int main(int argc, char *argv[]) {
  std::map<std::string, FileInfo> allFiles;
  es::print::AddPrinterFunction(es::Print);

  {
    DirectoryScanner scan;
    scan.AddFilter(".pkg$");
    scan.AddFilter(".avi$");
    std::string rootFolder(argv[1]);
    scan.Scan(rootFolder + "/packed/movies");
    scan.Scan(rootFolder + "/game/packed/movies");

    for (auto &f : scan) {
      struct stat fileStat;
      stat(f.c_str(), &fileStat);

      const uint64 fileSize = fileStat.st_size;
      const uint64 lastWriteTime = fileStat.st_mtim.tv_sec;

      std::string_view sv(f);
      sv.remove_prefix(rootFolder.size() + 1);
      if (sv.starts_with("game")) {
        sv.remove_prefix(5);
      }
      allFiles.emplace(sv, FileInfo{fileSize, lastWriteTime});
    }
  }

  uint64 curLen = 0;

  auto LoadLocalized = [&allFiles, &curLen](LocalizedFiles &out,
                                            std::string_view prefix) {
    static const char chars[]{'e', 'u', 'f', 'i', 'g', 's', 'd', 'p', 'j', 'k'};
    std::string built(prefix);
    built.push_back('.');
    const size_t modIndex = built.size();
    built.append(" .pkg");

    for (uint32 idx = 0; auto c : chars) {
      built.at(modIndex) = c;

      for (auto &[n, d] : allFiles) {
        if (n.ends_with(built)) {
          curLen += GetPadding(curLen, 0x800);
          d.range.chunk = curLen / CHUNK_SIZE;
          d.range.range.start = curLen % CHUNK_SIZE;
          d.range.range.count = d.size + 0x7ff;
          curLen += d.size;

          out.files[idx] = d.range;
        }
      }

      idx++;
    }
  };

  MpegWAD mpegWad{.movies = 8};
  BinWritter wrm("mpeg.wad");
  BinWritterRef_e wr(wrm);
  wr.SwapEndian(true);
  wr.Write(mpegWad);
  wr.Write(uint32(MOVIES.size()));
  for (auto &[name, hdr] : MOVIES) {
    MovieWAD movie{hdr};
    for (auto &[n, d] : allFiles) {
      if (n.contains(name) && n.ends_with(".avi")) {
        curLen += GetPadding(curLen, 0x800);
        d.range.chunk = curLen / CHUNK_SIZE;
        d.range.range.start = curLen % CHUNK_SIZE;
        d.range.range.count = d.size + 0x7ff;
        curLen += d.size;

        movie.movieStream = d.range;
      }
    }

    LoadLocalized(movie.subtitles, name);

    wr.Write(movie);
  }

  std::ofstream lst("mpeg.wad.lst");
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
    lst << 7 << std::setw(4) << 2 << std::setw(4) << data.range.chunk
        << std::setw(15) << data.range.range.start << std::setw(15) << data.size
        << std::setw(15) << data.mtime << "   " << path << "   x:/i8   "
        << int(!path.ends_with(".avi")) << '\n';
  }

  return 0;
}
