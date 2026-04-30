/*  InsomniaToolset MpegWAD
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
    "^mpeg.wad.lst$",
};

static AppInfo_s appInfo{
    .header =
        MpegWAD_DESC " v" MpegWAD_VERSION ", " MpegWAD_COPYRIGHT "Lukas Cone",
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
  MpegWAD wadHdr;
  rd.Read(wadHdr);
  std::vector<MovieWAD> mpegs;
  rd.ReadContainer(mpegs);

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

  auto printLoc = [&print](const char *name, LocalizedFiles &item) {
    static const char chars[]{'e', 'u', 'f', 'i', 'g', 's', 'd', 'p', 'j', 'k'};
    PrintInfo(name);
    char buffer[] = "  e";

    for (uint32 idx = 0; char c : chars) {
      buffer[2] = c;
      print(buffer, item.files[idx++]);
    }
  };

  for (auto &l : mpegs) {
    PrintInfo("unk0: ", l.unk0[0]);
    PrintInfo("unk1: ", l.unk0[1]);
    PrintInfo("unk2: ", l.unk0[2]);
    PrintInfo("unk3: ", l.unk1);
    print("movieStream", l.movieStream);
    printLoc("subtitles", l.subtitles);
  }
}
