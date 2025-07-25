/*  InsomniaToolset SoundExtract
    Copyright(C) 2017-2025 Lukas Cone

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
#include "spike/format/WAVE.hpp"
#include "spike/format/XVAG.hpp"
#include "spike/io/binreader_stream.hpp"
#include <algorithm>

std::string_view filters[]{
    "^ps3sound.dat$",
    "^ps3dialogue.*.dat$",
    "^resident_dialogue.*.dat$",
    "^resident_sound.dat$",
};

static AppInfo_s appInfo{
    .header = SoundExtract_DESC " v" SoundExtract_VERSION
                                        ", " SoundExtract_COPYRIGHT
                                        "Lukas Cone",
    .filters = filters,
};

AppInfo_s *AppInitModule() { return &appInfo; }

template <> void FByteswapper(SCREAMWaveform &item, bool) {
  FByteswapper(item.unk2);
  FByteswapper(item.unk3);
  FByteswapper(item.flags);
  FByteswapper(item.streamOffset);
  FByteswapper(item.streamSize);
}

// https://github.com/open-goal/jak-project/blob/master/game/sound/989snd/util.cpp
const uint16 NotePitchTable[] = {
    0x8000, 0x879C, 0x8FAC, 0x9837, 0xA145, 0xAADC, 0xB504, 0xBFC8, 0xCB2F,
    0xD744, 0xE411, 0xF1A1, 0x8000, 0x800E, 0x801D, 0x802C, 0x803B, 0x804A,
    0x8058, 0x8067, 0x8076, 0x8085, 0x8094, 0x80A3, 0x80B1, 0x80C0, 0x80CF,
    0x80DE, 0x80ED, 0x80FC, 0x810B, 0x811A, 0x8129, 0x8138, 0x8146, 0x8155,
    0x8164, 0x8173, 0x8182, 0x8191, 0x81A0, 0x81AF, 0x81BE, 0x81CD, 0x81DC,
    0x81EB, 0x81FA, 0x8209, 0x8218, 0x8227, 0x8236, 0x8245, 0x8254, 0x8263,
    0x8272, 0x8282, 0x8291, 0x82A0, 0x82AF, 0x82BE, 0x82CD, 0x82DC, 0x82EB,
    0x82FA, 0x830A, 0x8319, 0x8328, 0x8337, 0x8346, 0x8355, 0x8364, 0x8374,
    0x8383, 0x8392, 0x83A1, 0x83B0, 0x83C0, 0x83CF, 0x83DE, 0x83ED, 0x83FD,
    0x840C, 0x841B, 0x842A, 0x843A, 0x8449, 0x8458, 0x8468, 0x8477, 0x8486,
    0x8495, 0x84A5, 0x84B4, 0x84C3, 0x84D3, 0x84E2, 0x84F1, 0x8501, 0x8510,
    0x8520, 0x852F, 0x853E, 0x854E, 0x855D, 0x856D, 0x857C, 0x858B, 0x859B,
    0x85AA, 0x85BA, 0x85C9, 0x85D9, 0x85E8, 0x85F8, 0x8607, 0x8617, 0x8626,
    0x8636, 0x8645, 0x8655, 0x8664, 0x8674, 0x8683, 0x8693, 0x86A2, 0x86B2,
    0x86C1, 0x86D1, 0x86E0, 0x86F0, 0x8700, 0x870F, 0x871F, 0x872E, 0x873E,
    0x874E, 0x875D, 0x876D, 0x877D, 0x878C};

uint16 sceSdNote2Pitch(uint16 center_note, uint16 center_fine, uint16 note,
                       short fine) {
  int32 _fine;
  int32 _fine2;
  int32 _note;
  int32 offset1, offset2;
  int32 val;
  int32 val2;
  int32 val3;
  int32 ret;

  _fine = fine + (uint16)center_fine;
  _fine2 = _fine;

  if (_fine < 0)
    _fine2 = _fine + 127;

  _fine2 = _fine2 / 128;
  _note = note + _fine2 - center_note;
  val3 = _note / 6;

  if (_note < 0)
    val3--;

  offset2 = _fine - _fine2 * 128;

  if (_note < 0)
    val2 = -1;
  else
    val2 = 0;
  if (val3 < 0)
    val3--;

  val2 = (val3 / 2) - val2;
  val = val2 - 2;
  offset1 = _note - (val2 * 12);

  if ((offset1 < 0) || ((offset1 == 0) && (offset2 < 0))) {
    offset1 = offset1 + 12;
    val = val2 - 3;
  }

  if (offset2 < 0) {
    offset1 = (offset1 - 1) + _fine2;
    offset2 += (_fine2 + 1) * 128;
  }

  ret = (NotePitchTable[offset1] * NotePitchTable[offset2 + 12]) / 0x10000;

  if (val < 0)
    ret = (ret + (1 << (-val - 1))) >> -val;

  return (uint16)ret;
}

uint16 PS1Note2Pitch(int8 center_note, int8 center_fine, int16 note,
                     int16 fine) {
  bool ps1_note = false;
  if (center_note >= 0) {
    ps1_note = true;
  } else {
    center_note = -center_note;
  }

  uint16 pitch = sceSdNote2Pitch(center_note, center_fine, note, fine);
  if (ps1_note) {
    pitch = (0x10F4A * pitch) >> 16;
  }

  return pitch;
}

static const int8 xa_adpcm_table[5][2] = {
    {0, 0}, {60, 0}, {115, -52}, {98, -55}, {122, -60}};

void DecodeBlock(const uint8 *data, std::array<int16, 28> &samples,
                 int32 &prevSample, int32 &ppSample) {
  uint8 filter = uint8(data[0]) >> 4;
  uint8 shift = data[0] & 0xf;
  uint8 flags = data[1] & 0x7;

  if (flags == 0x7) {
    samples = {};
    prevSample = 0;
    ppSample = 0;
    return;
  }

  for (int i = 0; i < 28; i++) {
    int32 scale = data[2 + i / 2];
    scale <<= 28 - ((i % 2) * 4);
    scale >>= 28;
    scale *= 1 << 12;
    scale >>= shift;
    int32 sample = scale + (prevSample * xa_adpcm_table[filter][0] +
                            ppSample * xa_adpcm_table[filter][1]) /
                               64;
    sample = std::clamp<int32>(sample, std::numeric_limits<int16>::min(),
                               std::numeric_limits<int16>::max());
    ppSample = prevSample;
    prevSample = sample;
    samples[i] = sample;
  }
}

void ConvertSound(const SCREAMWaveform &wave, AppExtractContext *ectx,
                  const char *data) {
  const uint8 *dataStart =
      reinterpret_cast<const uint8 *>(data + wave.streamOffset);
  const uint8 *dataEnd = dataStart + wave.streamSize;
  const uint32 numBlocks = (dataEnd - dataStart) / 16;
  const bool usePCM = wave.flags & 0x80;
  uint32 sampleRate =
      (PS1Note2Pitch(wave.centerNote, wave.centerFine, 0x3C, 0x00) *
       0.00024414062) *
      48000;

  const uint32 dataSize = usePCM ? (dataEnd - dataStart) : numBlocks * 28 * 2;
  uint32 totalWavSize =
      sizeof(RIFFHeader) + sizeof(WAVE_fmt) + sizeof(WAVE_data) + dataSize;
  RIFFHeader wHdr(totalWavSize);
  WAVE_fmt wFmt(WAVE_FORMAT::PCM);
  wFmt.sampleRate = sampleRate;
  wFmt.CalcData();
  WAVE_data wData(dataSize);

  ectx->SendData({reinterpret_cast<const char *>(&wHdr), sizeof(RIFFHeader)});
  ectx->SendData({reinterpret_cast<const char *>(&wFmt), sizeof(WAVE_fmt)});
  ectx->SendData({reinterpret_cast<const char *>(&wData), sizeof(WAVE_data)});

  if (usePCM) {
    std::vector<uint16> data(reinterpret_cast<const uint16 *>(dataStart),
                             reinterpret_cast<const uint16 *>(dataEnd));
    for (auto &d : data) {
      FByteswapper(d);
    }
    ectx->SendData({reinterpret_cast<const char *>(data.data()), dataSize});
    return;
  }

  int32 prevSample = 0;
  int32 ppSample = 0;

  for (uint32 b = 0; b < numBlocks; b++) {
    std::array<int16, 28> samples;
    DecodeBlock(dataStart + 16 * b, samples, prevSample, ppSample);
    ectx->SendData({reinterpret_cast<const char *>(samples.data()), 2 * 28});
  }
}

struct VPK {
  static constexpr uint32 ID = CompileFourCC(" KPV");
  uint32 id;
  uint32 channelSize;
  uint32 dataOffset;
  uint32 channelBlockSize;
  uint32 sampleRate;
  uint32 numChannels;
  uint32 unk1[2];
};

struct VAGp {
  static constexpr uint32 ID = CompileFourCC("pGAV");
  static constexpr uint32 ID_BE = CompileFourCC("VAGp");
  uint32 id;
  uint32 null0[2];
  uint32 dataSize;
  uint32 sampleRate;
  uint32 null1[7];
};

void ConvertSound(VPK &hdr, BinReaderRef rd, AppExtractContext *ectx) {
  rd.Seek(hdr.dataOffset);
  hdr.channelBlockSize /= 2;

  const uint32 numSamplesPerChannel = (hdr.channelSize / 16) * 28;
  const uint32 dataSize = numSamplesPerChannel * hdr.numChannels * 2;
  uint32 totalWavSize =
      sizeof(RIFFHeader) + sizeof(WAVE_fmt) + sizeof(WAVE_data) + dataSize;
  RIFFHeader wHdr(totalWavSize);
  WAVE_fmt wFmt(WAVE_FORMAT::PCM);
  wFmt.sampleRate = hdr.sampleRate;
  wFmt.channels = hdr.numChannels;
  wFmt.CalcData();
  WAVE_data wData(dataSize);

  ectx->SendData({reinterpret_cast<const char *>(&wHdr), sizeof(RIFFHeader)});
  ectx->SendData({reinterpret_cast<const char *>(&wFmt), sizeof(WAVE_fmt)});
  ectx->SendData({reinterpret_cast<const char *>(&wData), sizeof(WAVE_data)});

  struct ChannelData {
    std::vector<std::array<int16, 28>> samples;
    std::vector<uint8> data;
    int32 prevSample = 0;
    int32 ppSample = 0;
  };

  std::vector<ChannelData> channelData(hdr.numChannels);
  const uint32 numBlocks = hdr.channelSize / hdr.channelBlockSize;

  auto Decode = [&](uint32 blockSize) {
    const uint32 numLanesPerBlock = blockSize / 16;

    for (auto &c : channelData) {
      c.samples.resize(numLanesPerBlock);
      rd.ReadContainer(c.data, hdr.channelBlockSize);

      for (uint32 l = 0; l < numLanesPerBlock; l++) {
        DecodeBlock(c.data.data() + 16 * l, c.samples[l], c.prevSample,
                    c.ppSample);
      }
    }

    for (uint32 l = 0; l < numLanesPerBlock; l++) {
      std::vector<int16> samples;

      for (uint32 s = 0; s < 28; s++) {
        for (uint32 c = 0; c < hdr.numChannels; c++) {
          samples.push_back(channelData.at(c).samples.at(l).at(s));
        }
      }

      ectx->SendData(
          {reinterpret_cast<const char *>(samples.data()), 2 * samples.size()});
    }
  };

  for (uint32 b = 0; b < numBlocks; b++) {
    Decode(hdr.channelBlockSize);
  }

  if (uint32 rest = hdr.channelSize % hdr.channelBlockSize; rest > 0) {
    Decode(rest);
  }
}

void ConvertSound(VAGp &hdr, BinReaderRef rd, AppExtractContext *ectx) {
  const uint32 numBlocks = hdr.dataSize / 16;
  const uint32 numSamples = numBlocks * 28;
  const uint32 dataSize = numSamples * 2;
  uint32 totalWavSize =
      sizeof(RIFFHeader) + sizeof(WAVE_fmt) + sizeof(WAVE_data) + dataSize;
  RIFFHeader wHdr(totalWavSize);
  WAVE_fmt wFmt(WAVE_FORMAT::PCM);
  wFmt.sampleRate = hdr.sampleRate;
  wFmt.CalcData();
  WAVE_data wData(dataSize);

  ectx->SendData({reinterpret_cast<const char *>(&wHdr), sizeof(RIFFHeader)});
  ectx->SendData({reinterpret_cast<const char *>(&wFmt), sizeof(WAVE_fmt)});
  ectx->SendData({reinterpret_cast<const char *>(&wData), sizeof(WAVE_data)});

  int32 prevSample = 0;
  int32 ppSample = 0;
  std::vector<uint8> data;
  rd.ReadContainer(data, hdr.dataSize);

  for (uint32 b = 0; b < numBlocks; b++) {
    std::array<int16, 28> samples;
    DecodeBlock(data.data() + 16 * b, samples, prevSample, ppSample);
    ectx->SendData({reinterpret_cast<const char *>(samples.data()), 2 * 28});
  }
}

void ConvertSound(XVAGHeader &hdr, BinReaderRef rd, AppExtractContext *ectx) {
  XVAGfmat xfmt;

  for (uint8 i = 0; i < hdr.numBlocks; i++) {
    rd.Push();
    XVAGChunk chunk;
    rd.Read(chunk);
    FByteswapper(chunk.size);

    if (chunk.id == XVAGFMAT) {
      rd.Pop();
      rd.Read(xfmt);
      FArraySwapper(xfmt);
      break;
    } else {
      rd.Skip(chunk.size);
    }
  }

  rd.Seek(hdr.size);

  // xfmt.bufferSize = (xfmt.numSamples / 28) * 16 * xfmt.numChannels;

  const uint32 numSamples = (xfmt.bufferSize / 16) * 28;
  const uint32 dataSize =
      xfmt.format == XVAGFormat::PS_ADPCM ? numSamples * 2 : xfmt.bufferSize;
  uint32 totalWavSize =
      sizeof(RIFFHeader) + sizeof(WAVE_fmt) + sizeof(WAVE_data) + dataSize;
  RIFFHeader wHdr(totalWavSize);
  WAVE_fmt wFmt(xfmt.format == XVAGFormat::PS_ADPCM ? WAVE_FORMAT::PCM
                                                    : WAVE_FORMAT::MPEG);
  wFmt.sampleRate = xfmt.sampleRate;
  wFmt.channels = xfmt.numChannels;
  wFmt.CalcData();
  WAVE_data wData(dataSize);

  ectx->SendData({reinterpret_cast<const char *>(&wHdr), sizeof(RIFFHeader)});
  ectx->SendData({reinterpret_cast<const char *>(&wFmt), sizeof(WAVE_fmt)});
  ectx->SendData({reinterpret_cast<const char *>(&wData), sizeof(WAVE_data)});

  if (xfmt.format == XVAGFormat::MPEG) {
    std::string buffer;
    rd.ReadContainer(buffer, dataSize);
    ectx->SendData(buffer);
    return;
  } else if (xfmt.format != XVAGFormat::PS_ADPCM) {
    throw std::runtime_error("Unhandled xvag format");
  }

  if (xfmt.interleave != 1) {
    throw std::runtime_error("Unhandled xvag interleave");
  }

  struct ChannelData {
    std::array<int16, 28> samples;
    int32 prevSample = 0;
    int32 ppSample = 0;
  };

  std::vector<ChannelData> channelData(xfmt.numChannels);
  const uint32 numBlocks = (xfmt.bufferSize / 16) / xfmt.numChannels;

  for (uint32 b = 0; b < numBlocks; b++) {
    for (auto &c : channelData) {
      uint8 lane[16];
      rd.Read(lane);
      DecodeBlock(lane, c.samples, c.prevSample, c.ppSample);
    }

    std::vector<int16> samples;

    for (uint32 s = 0; s < 28; s++) {
      for (uint32 c = 0; c < xfmt.numChannels; c++) {
        samples.push_back(channelData.at(c).samples.at(s));
      }
    }

    ectx->SendData(
        {reinterpret_cast<const char *>(samples.data()), 2 * samples.size()});
  }
}

void ConvertSound(BinReaderRef rd, AppExtractContext *ectx) {
  uint32 id;
  rd.Push();
  rd.Read(id);
  rd.Pop();

  if (id == VPK::ID) {
    VPK hdr;
    rd.Read(hdr);
    ConvertSound(hdr, rd, ectx);
  } else if (id == VAGp::ID) {
    VAGp hdr;
    rd.Read(hdr);
    ConvertSound(hdr, rd, ectx);
  } else if (id == VAGp::ID_BE) {
    VAGp hdr;
    rd.Read(hdr);
    FArraySwapper(hdr);
    ConvertSound(hdr, rd, ectx);
  } else if (id == XVAGID) {
    XVAGHeader hdr;
    rd.Read(hdr);
    FByteswapper(hdr.size);
    ConvertSound(hdr, rd, ectx);
  } else {
    throw es::InvalidHeaderError(id);
  }
}

void AppProcessFile(AppContext *ctx) {
  std::string streamName(ctx->workingFile.GetFolder());
  Version version = Version::RFOM;

  if (ctx->workingFile.GetFilename().starts_with("ps3sound")) {
    streamName = "ps3soundstream.dat";
  } else if (ctx->workingFile.GetFilename().starts_with("ps3dialogue")) {
    AFileInfo tp(ctx->workingFile.GetFilename());
    streamName = "ps3dialoguestream";
    streamName.append(tp.GetExtension());
    streamName.append(".dat");
  } else if (ctx->workingFile.GetFilename().starts_with("resident_sound")) {
    streamName = "streaming_sound.dat";
    version = Version::V2;
  } else if (ctx->workingFile.GetFilename().starts_with("resident_dialogue")) {
    AFileInfo tp(ctx->workingFile.GetFilename());
    streamName = "streaming_dialogue";
    streamName.append(tp.GetExtension());
    streamName.append(".dat");
    version = Version::V2;
  }

  BinReaderRef_e rd(ctx->GetStream());
  IGHW main;
  main.FromStream(rd, version);
  IGHWTOCIteratorConst<Sounds> sounds;
  IGHWTOCIteratorConst<SoundNames> soundNames;
  IGHWTOCIteratorConst<SoundBank> soundBank;
  IGHWTOCIteratorConst<SoundStreams> streams;

  IGHWTOCIteratorConst<SoundsV2> soundsV2;
  IGHWTOCIteratorConst<SoundNamesV2> soundNamesV2;
  IGHWTOCIteratorConst<SoundStreamsV2> streamsV2;

  CatchClasses(main, sounds, soundNames, soundBank, streams, soundsV2,
               soundNamesV2, streamsV2);

  const Sounds &snds = soundsV2.Valid() ? soundsV2.at(0) : sounds.at(0);
  const SoundNames &sndNames =
      soundsV2.Valid() ? soundNamesV2.at(0) : soundNames.at(0);
  const SoundBank *sndBank = soundBank.begin();
  const SCREAMBank *bank = nullptr;
  const char *data = nullptr;

  if (sndBank) {
    bank = reinterpret_cast<const SCREAMBank *>(
        sndBank->bank.sections[SCREAMSection::TYPE_BANK].data.Get());
    data = sndBank->bank.sections[SCREAMSection::TYPE_DATA].data;
  }

  AppContextStream str;
  auto ectx = ctx->ExtractContext();

  for (uint32 i = 0; i < snds.numSounds; i++) {
    const Sound &snd = snds.sounds[i];
    if (snd.index < 0) {
      continue;
    }
    if (snd.type == 0) {
      const SCREAMSound &sSound = bank->sounds[snd.index];

      for (uint8 g = 0; g < sSound.numGains; g++) {
        const SCREAMGain &gain = sSound.gains[g];
        if (gain.type == 1) {
          std::string fileName(sndNames.names[i]);
          fileName.push_back('_');
          fileName.append(std::to_string(g));
          fileName.append(".wav");
          ectx->NewFile(fileName);

          SCREAMWaveform wform = *reinterpret_cast<const SCREAMWaveform *>(
              bank->gainData.Get() + gain.streamOffset);
          FByteswapper(wform);
          ConvertSound(wform, ectx, data);
        }
      }
    } else {
      if (!str) {
        str = ctx->RequestFile(streamName);
      }

      uint32 streamOffset = streams.Valid()
                                ? streams.at(0).streamOffsets[snd.index]
                                : streamsV2.at(0).streamOffsets[snd.index];
      FByteswapper(streamOffset);
      BinReaderRef rdStr(*str.Get());
      rdStr.SetRelativeOrigin(streamOffset);

      std::string fileName(sndNames.names[i]);
      fileName.append(".wav");
      ectx->NewFile(fileName);
      ConvertSound(rdStr, ectx);
    }
  }
}
