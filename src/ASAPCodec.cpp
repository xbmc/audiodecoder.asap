/*
 *  Copyright (C) 2005-2020 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#include <kodi/addon-instance/AudioDecoder.h>
#include <kodi/Filesystem.h>

extern "C" {
#include "asap.h"
}

struct ASAPContext {
  char author[ASAPInfo_MAX_TEXT_LENGTH + 1];
  char name[ASAPInfo_MAX_TEXT_LENGTH + 1];
  int year;
  int month;
  int day;
  int channels;
  int duration;
  ASAP* asap = nullptr;
};

class ATTRIBUTE_HIDDEN CASAPCodec : public kodi::addon::CInstanceAudioDecoder
{
public:
  CASAPCodec(KODI_HANDLE instance) :
    CInstanceAudioDecoder(instance)
  {
  }

  virtual ~CASAPCodec()
  {
    if (ctx.asap)
      ASAP_Delete(ctx.asap);
  }

  bool Init(const std::string& filename, unsigned int filecache,
            int& channels, int& samplerate,
            int& bitspersample, int64_t& totaltime,
            int& bitrate, AEDataFormat& format,
            std::vector<AEChannel>& channellist) override
  {
    int track=0;
    std::string toLoad(filename);
    if (toLoad.find(".asapstream") != std::string::npos)
    {
      size_t iStart=toLoad.rfind('-') + 1;
      track = atoi(toLoad.substr(iStart, toLoad.size()-iStart-11).c_str())-1;
      //  The directory we are in, is the file
      //  that contains the bitstream to play,
      //  so extract it
      size_t slash = toLoad.rfind('\\');
      if (slash == std::string::npos)
        slash = toLoad.rfind('/');
      toLoad = toLoad.substr(0, slash);
    }

    kodi::vfs::CFile file;
    if (!file.OpenFile(toLoad,0))
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
    format = AE_FMT_S16NE;
    if (channels == 1)
      channellist = { AE_CH_FC };
    else
      channellist = { AE_CH_FL, AE_CH_FR };
    bitrate = 0;

    ASAP_PlaySong(ctx.asap, track, totaltime);

    return true;
  }

  int ReadPCM(uint8_t* buffer, int size, int& actualsize) override
  {
    actualsize = ASAP_Generate(ctx.asap, buffer, size, ASAPSampleFormat_S16_L_E);

    return actualsize == 0;
  }

  int64_t Seek(int64_t time) override
  {
    ASAP_Seek(ctx.asap, time);

    return time;
  }

  bool ReadTag(const std::string& filename, std::string& title,
               std::string& artist, int& length) override
  {
    int track=1;
    std::string toLoad(filename);
    if (toLoad.find(".asapstream") != std::string::npos)
    {
      size_t iStart=toLoad.rfind('-') + 1;
      track = atoi(toLoad.substr(iStart, toLoad.size()-iStart-11).c_str());
      //  The directory we are in, is the file
      //  that contains the bitstream to play,
      //  so extract it
      size_t slash = toLoad.rfind('\\');
      if (slash == std::string::npos)
        slash = toLoad.rfind('/');
      toLoad = toLoad.substr(0, slash);
    }

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
    artist = ASAPInfo_GetAuthor(info);
    title = ASAPInfo_GetTitleOrFilename(info);
    length = ASAPInfo_GetDuration(info, track);

    ASAP_Delete(asap);

    return true;
  }

  int TrackCount(const std::string& fileName) override
  {
    kodi::vfs::CFile file;
    if (!file.OpenFile(fileName,0))
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

private:
  ASAPContext ctx;
};


class ATTRIBUTE_HIDDEN CMyAddon : public kodi::addon::CAddonBase
{
public:
  CMyAddon() = default;
  ADDON_STATUS CreateInstance(int instanceType, std::string instanceID, KODI_HANDLE instance, KODI_HANDLE& addonInstance) override
  {
    addonInstance = new CASAPCodec(instance);
    return ADDON_STATUS_OK;
  }
  virtual ~CMyAddon() = default;
};


ADDONCREATOR(CMyAddon);
