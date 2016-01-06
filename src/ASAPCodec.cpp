/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "libXBMC_addon.h"

extern "C" {
#include "asap.h"
#include "kodi_audiodec_dll.h"

ADDON::CHelper_libXBMC_addon *XBMC           = NULL;

//-- Create -------------------------------------------------------------------
// Called on load. Addon should fully initalize or return error status
//-----------------------------------------------------------------------------
ADDON_STATUS ADDON_Create(void* hdl, void* props)
{
  if (!XBMC)
    XBMC = new ADDON::CHelper_libXBMC_addon;

  if (!XBMC->RegisterMe(hdl))
  {
    delete XBMC, XBMC=NULL;
    return ADDON_STATUS_PERMANENT_FAILURE;
  }

  return ADDON_STATUS_OK;
}

//-- Stop ---------------------------------------------------------------------
// This dll must cease all runtime activities
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------
void ADDON_Stop()
{
}

//-- Destroy ------------------------------------------------------------------
// Do everything before unload of this add-on
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------
void ADDON_Destroy()
{
  XBMC=NULL;
}

//-- HasSettings --------------------------------------------------------------
// Returns true if this add-on use settings
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------
bool ADDON_HasSettings()
{
  return false;
}

//-- GetStatus ---------------------------------------------------------------
// Returns the current Status of this visualisation
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------
ADDON_STATUS ADDON_GetStatus()
{
  return ADDON_STATUS_OK;
}

//-- GetSettings --------------------------------------------------------------
// Return the settings for XBMC to display
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------
unsigned int ADDON_GetSettings(ADDON_StructSetting ***sSet)
{
  return 0;
}

//-- FreeSettings --------------------------------------------------------------
// Free the settings struct passed from XBMC
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------

void ADDON_FreeSettings()
{
}

//-- SetSetting ---------------------------------------------------------------
// Set a specific Setting value (called from XBMC)
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------
ADDON_STATUS ADDON_SetSetting(const char *strSetting, const void* value)
{
  return ADDON_STATUS_OK;
}

//-- Announce -----------------------------------------------------------------
// Receive announcements from XBMC
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------
void ADDON_Announce(const char *flag, const char *sender, const char *message, const void *data)
{

}

struct ASAPContext {
  char author[ASAPInfo_MAX_TEXT_LENGTH + 1];
  char name[ASAPInfo_MAX_TEXT_LENGTH + 1];
  int year;
  int month;
  int day;
  int channels;
  int duration;
  ASAP* asap;
};

void* Init(const char* strFile, unsigned int filecache, int* channels,
           int* samplerate, int* bitspersample, int64_t* totaltime,
           int* bitrate, AEDataFormat* format, const AEChannel** channelinfo)
{
  int track=0;
  std::string toLoad(strFile);
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

  void* file = XBMC->OpenFile(toLoad.c_str(),0);
  if (!file)
    return NULL;

  int len = XBMC->GetFileLength(file);
  uint8_t* data = new uint8_t[len];
  XBMC->ReadFile(file, data, len);
  XBMC->CloseFile(file);

  ASAPContext* result = new ASAPContext;
  result->asap = ASAP_New();

  // Now load the module
  if (!ASAP_Load(result->asap, toLoad.c_str(), data, len))
  {
    delete[] data;
    delete result;
  }
  delete[] data;

  const ASAPInfo* info = ASAP_GetInfo(result->asap);

  *channels = ASAPInfo_GetChannels(info);
  *samplerate = 44100;
  *bitspersample = 16;
  *totaltime = ASAPInfo_GetDuration(info, track);
  *format = AE_FMT_S16NE;
  *channelinfo = NULL;
  *bitrate = 0;

  ASAP_PlaySong(result->asap, track, *totaltime);

  return result;
}

int ReadPCM(void* context, uint8_t* pBuffer, int size, int *actualsize)
{
  if (!context)
    return 1;

  ASAPContext* ctx = (ASAPContext*)context;

  *actualsize = ASAP_Generate(ctx->asap, pBuffer, size, ASAPSampleFormat_S16_L_E);

  return *actualsize == 0;
}

int64_t Seek(void* context, int64_t time)
{
  if (!context)
    return 1;

  ASAPContext* ctx = (ASAPContext*)context;

  ASAP_Seek(ctx->asap, time);

  return time;
}

bool DeInit(void* context)
{
  if (!context)
    return 1;

  ASAPContext* ctx = (ASAPContext*)context;
  ASAP_Delete(ctx->asap);
  delete ctx;

  return true;
}

bool ReadTag(const char* strFile, char* title, char* artist,
             int* length)
{
  int track=1;
  std::string toLoad(strFile);
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
  void* file = XBMC->OpenFile(toLoad.c_str(), 0);
  if (!file)
    return false;

  int len = XBMC->GetFileLength(file);
  uint8_t* data = new uint8_t[len];
  XBMC->ReadFile(file, data, len);
  XBMC->CloseFile(file);

  ASAP* asap = ASAP_New();

  // Now load the module
  if (!ASAP_Load(asap, strFile, data, len))
  {
    delete[] data;
    return false;
  }

  delete[] data;

  const ASAPInfo* info = ASAP_GetInfo(asap);
  strcpy(artist, ASAPInfo_GetAuthor(info));
  strcpy(title, ASAPInfo_GetTitleOrFilename(info));
  *length = ASAPInfo_GetDuration(info, track);

  return true;
}

int TrackCount(const char* strFile)
{
  void* file = XBMC->OpenFile(strFile, 0);
  if (!file)
    return 1;

  int len = XBMC->GetFileLength(file);
  uint8_t* data = new uint8_t[len];
  XBMC->ReadFile(file, data, len);
  XBMC->CloseFile(file);

  ASAP* asap = ASAP_New();

  // Now load the module
  if (!ASAP_Load(asap, strFile, data, len))
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
}
