/*
 *  Copyright (C) 2005-2022 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#include "ASAPCodec.h"

#include <kodi/Filesystem.h>

CASAPCodec::CASAPCodec(const kodi::addon::IInstanceInfo& instance) : CInstanceAudioDecoder(instance)
{
}

CASAPCodec::~CASAPCodec()
{
  if (ctx.asap)
    ASAP_Delete(ctx.asap);
}

bool CASAPCodec::SupportsFile(const std::string& filename)
{
  return ASAPInfo_IsOurFile(filename.c_str());
}

bool CASAPCodec::Init(const std::string& filename,
                      unsigned int filecache,
                      int& channels,
                      int& samplerate,
                      int& bitspersample,
                      int64_t& totaltime,
                      int& bitrate,
                      AudioEngineDataFormat& format,
                      std::vector<AudioEngineChannel>& channellist)
{
  int track = 0;
  const std::string toLoad = kodi::addon::CInstanceAudioDecoder::GetTrack("asap", filename, track);

  kodi::vfs::CFile file;
  if (!file.OpenFile(toLoad, 0))
    return false;

  int len = file.GetLength();
  uint8_t* data = new uint8_t[len];
  file.Read(data, len);
  file.Close();

  ctx.asap = ASAP_New();

  // Now load the module
  if (!ASAP_Load(ctx.asap, toLoad.c_str(), data, len))
  {
    delete[] data;
    return false;
  }
  delete[] data;

  const ASAPInfo* info = ASAP_GetInfo(ctx.asap);

  channels = ASAPInfo_GetChannels(info);
  samplerate = 44100;
  bitspersample = 16;
  totaltime = ASAPInfo_GetDuration(info, track);
  format = AUDIOENGINE_FMT_S16NE;
  if (channels == 1)
    channellist = {AUDIOENGINE_CH_FC};
  else
    channellist = {AUDIOENGINE_CH_FL, AUDIOENGINE_CH_FR};
  bitrate = 0;

  ASAP_PlaySong(ctx.asap, (track > 0 ? track - 1 : 0), totaltime);

  return true;
}

int CASAPCodec::ReadPCM(uint8_t* buffer, size_t size, size_t& actualsize)
{
  actualsize = ASAP_Generate(ctx.asap, buffer, size, ASAPSampleFormat_S16_L_E);

  return actualsize == 0 ? AUDIODECODER_READ_EOF : AUDIODECODER_READ_SUCCESS;
}

int64_t CASAPCodec::Seek(int64_t time)
{
  ASAP_Seek(ctx.asap, time);

  return time;
}

bool CASAPCodec::ReadTag(const std::string& filename, kodi::addon::AudioDecoderInfoTag& tag)
{
  int track = 0;
  const std::string toLoad = kodi::addon::CInstanceAudioDecoder::GetTrack("asap", filename, track);

  kodi::vfs::CFile file;
  if (!file.OpenFile(toLoad, 0))
    return false;

  int len = file.GetLength();
  uint8_t* data = new uint8_t[len];
  file.Read(data, len);
  file.Close();

  ASAP* asap = ASAP_New();

  // Now load the module
  if (!ASAP_Load(asap, toLoad.c_str(), data, len))
  {
    delete[] data;
    return false;
  }

  delete[] data;

  const ASAPInfo* info = ASAP_GetInfo(asap);
  tag.SetArtist(ASAPInfo_GetAuthor(info));
  tag.SetTitle(ASAPInfo_GetTitleOrFilename(info));
  // Kodi gives them beginning with 1, ASAP there need to start with 0, so we reduce it by one.
  tag.SetDuration(ASAPInfo_GetDuration(info, track > 0 ? track - 1 : 0) / 1000);
  if (ASAPInfo_GetYear(info) > 0)
    tag.SetReleaseDate(std::to_string(ASAPInfo_GetYear(info)));
  tag.SetChannels(ASAPInfo_GetChannels(info));
  tag.SetSamplerate(44100);
  tag.SetTrack(track);

  ASAP_Delete(asap);

  return true;
}

int CASAPCodec::TrackCount(const std::string& fileName)
{
  kodi::vfs::CFile file;
  if (!file.OpenFile(fileName, 0))
    return 1;

  int len = file.GetLength();
  uint8_t* data = new uint8_t[len];
  file.Read(data, len);
  file.Close();

  ASAP* asap = ASAP_New();

  // Now load the module
  if (!ASAP_Load(asap, fileName.c_str(), data, len))
  {
    ASAP_Delete(asap);
    delete[] data;
    return 1;
  }
  delete[] data;

  const ASAPInfo* info = ASAP_GetInfo(asap);
  int result = ASAPInfo_GetSongs(info);
  ASAP_Delete(asap);

  return result;
}

//------------------------------------------------------------------------------

class ATTR_DLL_LOCAL CMyAddon : public kodi::addon::CAddonBase
{
public:
  CMyAddon() = default;
  ADDON_STATUS CreateInstance(const kodi::addon::IInstanceInfo& instance,
                              KODI_ADDON_INSTANCE_HDL& hdl) override
  {
    hdl = new CASAPCodec(instance);
    return ADDON_STATUS_OK;
  }
  virtual ~CMyAddon() = default;
};

ADDONCREATOR(CMyAddon)
