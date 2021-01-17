/*
 *  Copyright (C) 2005-2020 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

// TEMPORARY: Hardcoded for tests as the new not work on install
#include "/home/alwin/Development/Kodi/kodi-Matrix/xbmc/addons/kodi-dev-kit/include/kodi2/c-api/general.h"
#include "/home/alwin/Development/Kodi/kodi-Matrix/xbmc/addons/kodi-dev-kit/include/kodi2/c-api/filesystem.h"
#include "/home/alwin/Development/Kodi/kodi-Matrix/xbmc/addons/kodi-dev-kit/include/kodi2/c-api/addon-instance/audio_decoder.h"

#include "asap.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

struct ASAPContext
{
  char author[ASAPInfo_MAX_TEXT_LENGTH + 1];
  char name[ASAPInfo_MAX_TEXT_LENGTH + 1];
  int year;
  int month;
  int day;
  int channels;
  int duration;
  struct ASAP* asap;
};

KODI_AUDIODECODER_HDL audiodecoder_create(KODI_OWN_HDL kodi_hdl)
{
  struct ASAPContext* ctx = malloc(sizeof(struct ASAPContext));
  memset(ctx, 0, sizeof(struct ASAPContext));
  return ctx;
}

void audiodecoder_destroy(KODI_AUDIODECODER_HDL hdl)
{
  free(hdl);
  return;
}

bool audiodecoder_init(KODI_AUDIODECODER_HDL hdl,
                       const char* file,
                       unsigned int filecache,
                       int* channels,
                       int* samplerate,
                       int* bitspersample,
                       int64_t* totaltime,
                       int* bitrate,
                       enum AudioEngineDataFormat* format,
                       enum AudioEngineChannel info[][AUDIOENGINE_CH_MAX])
{
  struct ASAPContext* ctx = (struct ASAPContext*)hdl;

  int track = 0;
  char* toLoad = strdup(file);
  if (strstr(toLoad, ".asapstream") != NULL)
  {
    const char* pos = strrchr(toLoad, '-') + 1;
    if (pos == NULL)
    {
      free(toLoad);
      return false;
    }

    track = atoi(pos) - 1;
    //  The directory we are in, is the file
    //  that contains the bitstream to play,
    //  so extract it
    pos = strrchr(toLoad, '\\');
    if (pos == NULL)
      pos = strrchr(toLoad, '/');
    if (pos == NULL)
    {
      free(toLoad);
      return false;
    }
    toLoad[toLoad-pos] = 0;
  }

  KODI_FILE_HDL fileHdl = kodi_vfs_file_open(file, 0);
  if (!fileHdl)
    return false;
  int len = kodi_vfs_file_get_length(fileHdl);
  uint8_t* data = malloc(len);
  kodi_vfs_file_read(fileHdl, data, len);
  kodi_vfs_file_close(fileHdl);

  ctx->asap = ASAP_New();

  // Now load the module
  if (!ASAP_Load(ctx->asap, toLoad, data, len))
  {
    free(data);
    free(toLoad);
    return false;
  }
  free(data);

  const ASAPInfo* asap_info = ASAP_GetInfo(ctx->asap);

  *channels = ASAPInfo_GetChannels(asap_info);
  *samplerate = 44100;
  *bitspersample = 16;
  *totaltime = ASAPInfo_GetDuration(asap_info, track);
  *format = AUDIOENGINE_FMT_S16NE;
  if (*channels == 1)
  {
    (*info)[0] = AUDIOENGINE_CH_FC;
    (*info)[1] = AUDIOENGINE_CH_NULL;
  }
  else
  {
    (*info)[0] = AUDIOENGINE_CH_FL;
    (*info)[1] = AUDIOENGINE_CH_FR;
    (*info)[2] = AUDIOENGINE_CH_NULL;
  }
  *bitrate = 0;

  ASAP_PlaySong(ctx->asap, track, *totaltime);

  return true;
}

int audiodecoder_read_pcm(KODI_AUDIODECODER_HDL hdl, uint8_t* buffer, int size, int* actualsize)
{
  struct ASAPContext* ctx = (struct ASAPContext*)hdl;
  *actualsize = ASAP_Generate(ctx->asap, buffer, size, ASAPSampleFormat_S16_L_E);
  return actualsize == 0;
}

int64_t audiodecoder_seek(KODI_AUDIODECODER_HDL hdl, int64_t time)
{
  struct ASAPContext* ctx = (struct ASAPContext*)hdl;
  ASAP_Seek(ctx->asap, time);
  return time;
}

bool audiodecoder_read_tag(KODI_AUDIODECODER_HDL hdl,
                           const char* file,
                           struct AUDIO_DECODER_INFO_TAG* tag)
{
  int track = 0;
  char* toLoad = strdup(file);
  if (strstr(toLoad, ".asapstream") != NULL)
  {
    const char* pos = strrchr(toLoad, '-') + 1;
    if (pos == NULL)
    {
      free(toLoad);
      return false;
    }

    track = atoi(pos) - 1;
    //  The directory we are in, is the file
    //  that contains the bitstream to play,
    //  so extract it
    pos = strrchr(toLoad, '\\');
    if (pos == NULL)
      pos = strrchr(toLoad, '/');
    if (pos == NULL)
    {
      free(toLoad);
      return false;
    }
    toLoad[toLoad-pos] = 0;
  }

  KODI_FILE_HDL fileHdl = kodi_vfs_file_open(toLoad, 0);
  if (!fileHdl)
    return false;

  int len = kodi_vfs_file_get_length(fileHdl);
  uint8_t* data = malloc(len);
  kodi_vfs_file_read(fileHdl, data, len);
  kodi_vfs_file_close(fileHdl);

  ASAP* asap = ASAP_New();

  // Now load the module
  if (!ASAP_Load(asap, toLoad, data, len))
  {
    free(data);
    return false;
  }

  free(data);

  const ASAPInfo* info = ASAP_GetInfo(asap);
  tag->artist = strdup(ASAPInfo_GetAuthor(info));
  tag->title = strdup(ASAPInfo_GetTitleOrFilename(info));
  tag->duration = (ASAPInfo_GetDuration(info, track) / 1000);
  if (ASAPInfo_GetYear(info) > 0)
  {
    char str[12];
    sprintf(str, "%d", ASAPInfo_GetYear(info));
    tag->release_date = strdup(str);
  }
  tag->channels = ASAPInfo_GetChannels(info);
  tag->samplerate = 44100;
  tag->track = track;

  ASAP_Delete(asap);

  return true;
}

int audiodecoder_track_count(KODI_AUDIODECODER_HDL hdl, const char* file)
{
  KODI_FILE_HDL fileHdl = kodi_vfs_file_open(file, 0);
  if (!fileHdl)
    return 1;

  int len = kodi_vfs_file_get_length(fileHdl);
  uint8_t* data = malloc(len);
  kodi_vfs_file_read(fileHdl, data, len);
  kodi_vfs_file_close(fileHdl);

  ASAP* asap = ASAP_New();

  // Now load the module
  if (!ASAP_Load(asap, file, data, len))
  {
    ASAP_Delete(asap);
    free(data);
    return 1;
  }
  free(data);

  const ASAPInfo* info = ASAP_GetInfo(asap);
  int result = ASAPInfo_GetSongs(info);
  ASAP_Delete(asap);

  return result;
}

ADDON_STATUS2 create_instance(KODI_ADDON_HDL hdl,
                             enum ADDON_INSTANCE2 instanceType,
                             const char* instanceID,
                             KODI_OWN_HDL instance,
                             KODI_ADDON_HDL* addonInstance,
                             KODI_ADDON_HDL parent)
{
  static AUDIODECODER_FUNC func = {
      audiodecoder_create, audiodecoder_destroy,  audiodecoder_init,       audiodecoder_read_pcm,
      audiodecoder_seek,   audiodecoder_read_tag, audiodecoder_track_count};
  *addonInstance = &func;
  return ADDON_STATUS2_OK;
}

int main(int argc, char** argv)
{
  static ADDON_FUNC func = {NULL,
                            NULL,
                            create_instance,
                            NULL,
                            NULL,
                            NULL,
                            NULL,
                            NULL};

  KODI_IFC_HDL hdl = kodi_init(__KODI_API_1__, argc, argv, &func, true, false);
  if (!hdl)
    return -1;

  while (1)
  {
    if (!kodi_process(hdl))
      break;
  }

  kodi_deinit(hdl);
  return 0;
}
