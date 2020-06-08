/*
 *  Copyright (C) 2020 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#pragma once

#include <kodi/AddonBase.h>

/*  Client Settings default values */
#define DEFAULT_HOST "127.0.0.1"
#define DEFAULT_PORT 8100
#define DEFAULT_USETRANSCODING false
#define DEFAULT_USERNAME ""
#define DEFAULT_PASSWORD ""
#define DEFAULT_SHOWINFOMSG false
#define DEFAULT_HEIGHT 0
#define DEFAULT_WIDTH 0
#define DEFAULT_BITRATE 1024
#define DEFAULT_AUDIOTRACK "eng"
#define DEFAULT_USETIMESHIFT false
#define DEFAULT_ADDRECEPISODE2TITLE true
#define DEFAULT_GROUPRECBYSERIES true
#define DEFAULT_NOGROUP_SINGLE_REC false
#define DEFAULT_UPDATE_INTERVAL 4
#define DEFAULT_RECORD_SHOW_TYPE 1

class ATTRIBUTE_HIDDEN CSettings
{
public:
  CSettings() = default;

  bool Load();
  ADDON_STATUS SetSetting(const std::string& settingName, const kodi::CSettingValue& settingValue);

  const std::string& Hostname() const { return m_szHostname; }
  int Port() const { return m_iPort; }
  bool UseTranscoding() const { return m_bUseTranscoding; }
  const std::string& Username() const { return m_szUsername; }
  const std::string& Password() const { return m_szPassword; }
  bool ShowInfoMSG() const { return m_bShowInfoMSG; }
  int Height() const { return m_iHeight; }
  int Width() const { return m_iWidth; }
  int Bitrate() const { return m_iBitrate; }
  int DefaultUpdateInterval() const { return m_iDefaultUpdateInterval; }
  int DefaultRecShowType() const { return m_iDefaultRecShowType; }
  const std::string& Audiotrack() const { return m_szAudiotrack; }
  bool UseTimeshift() const { return m_bUseTimeshift; }
  bool AddRecEpisode2title() const { return m_bAddRecEpisode2title; }
  bool GroupRecBySeries() const { return m_bGroupRecBySeries; }
  bool NoGroupSingleRec() const { return m_bNoGroupSingleRec; }

private:
  std::string m_szHostname = DEFAULT_HOST;                    ///< The Host name or IP of the DVBLink Server
  int m_iPort = DEFAULT_PORT;                                ///< The DVBLink Connect Server listening port (default: 8080)
  bool m_bUseTranscoding = DEFAULT_USETRANSCODING;            ///< Use transcoding
  std::string m_szUsername = DEFAULT_USERNAME;                ///< Username
  std::string m_szPassword = DEFAULT_PASSWORD;                ///< Password
  bool m_bShowInfoMSG = DEFAULT_SHOWINFOMSG;                  ///< Show information messages
  int m_iHeight = DEFAULT_HEIGHT;                             ///< Height of stream when using transcoding (0: autodetect)
  int m_iWidth = DEFAULT_WIDTH;                               ///< Width of stream when using transcoding (0: autodetect)
  int m_iBitrate = DEFAULT_BITRATE;                           ///< Bitrate of stream when using transcoding
  int m_iDefaultUpdateInterval = DEFAULT_UPDATE_INTERVAL;     ///< Default update interval
  int m_iDefaultRecShowType = DEFAULT_RECORD_SHOW_TYPE;       ///< Default record show type
  std::string m_szAudiotrack = DEFAULT_AUDIOTRACK;            ///< Audiotrack to include in stream when using transcoding
  bool m_bUseTimeshift = DEFAULT_USETIMESHIFT;                ///< Use timeshift
  bool m_bAddRecEpisode2title = DEFAULT_ADDRECEPISODE2TITLE;  ///< Concatenate title and episode info for recordings
  bool m_bGroupRecBySeries = DEFAULT_GROUPRECBYSERIES;        ///< Group Recordings as Directories by series
  bool m_bNoGroupSingleRec = DEFAULT_NOGROUP_SINGLE_REC;      ///< Do not group single recordings
};
