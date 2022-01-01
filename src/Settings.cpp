/*
 *  Copyright (C) 2020-2021 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#include "Settings.h"

bool CSettings::Load()
{
  if (!kodi::addon::CheckSettingString("host", m_szHostname))
  {
    /* If setting is unknown fallback to defaults */
    kodi::Log(ADDON_LOG_ERROR, "Couldn't get 'host' setting, falling back to '%s' as default",
              DEFAULT_HOST);
    m_szHostname = DEFAULT_HOST;
  }

  /* read setting "username" from settings.xml */
  if (!kodi::addon::CheckSettingString("username", m_szUsername))
  {
    /* If setting is unknown fallback to defaults */
    kodi::Log(ADDON_LOG_ERROR, "Couldn't get 'username' setting, falling back to default");
    m_szUsername = DEFAULT_USERNAME;
  }

  /* read setting "password" from settings.xml */
  if (!kodi::addon::CheckSettingString("password", m_szPassword))
  {
    /* If setting is unknown fallback to defaults */
    kodi::Log(ADDON_LOG_ERROR, "Couldn't get 'password' setting, leaved empty");
    m_szPassword = DEFAULT_PASSWORD;
  }

  /* Read setting "enable_transcoding" from settings.xml */
  if (!kodi::addon::CheckSettingBoolean("enable_transcoding", m_bUseTranscoding))
  {
    /* If setting is unknown fallback to defaults */
    kodi::Log(ADDON_LOG_ERROR,
              "Couldn't get 'enable_transcoding' setting, falling back to '%s' as default",
              DEFAULT_USETRANSCODING ? "true" : "false");
    m_bUseTranscoding = DEFAULT_USETRANSCODING;
  }

  /* Read setting "port" from settings.xml */
  if (!kodi::addon::CheckSettingInt("port", m_iPort))
  {
    /* If setting is unknown fallback to defaults */
    kodi::Log(ADDON_LOG_ERROR, "Couldn't get 'port' setting, falling back to '%i' as default",
              DEFAULT_PORT);
    m_iPort = DEFAULT_PORT;
  }

  /* Read setting "timeshift" from settings.xml */
  if (!kodi::addon::CheckSettingBoolean("timeshift", m_bUseTimeshift))
  {
    /* If setting is unknown fallback to defaults */
    kodi::Log(ADDON_LOG_ERROR, "Couldn't get 'timeshift' setting, falling back to '%s' as default",
              DEFAULT_USETIMESHIFT ? "true" : "false");
    m_bUseTimeshift = DEFAULT_USETIMESHIFT;
  }

  /* Read setting "showinfomsg" from settings.xml */
  if (!kodi::addon::CheckSettingBoolean("showinfomsg", m_bShowInfoMSG))
  {
    /* If setting is unknown fallback to defaults */
    kodi::Log(ADDON_LOG_ERROR,
              "Couldn't get 'showinfomsg' setting, falling back to '%s' as default",
              DEFAULT_SHOWINFOMSG ? "true" : "false");
    m_bShowInfoMSG = DEFAULT_SHOWINFOMSG;
  }

  /* Read setting "add_rec_episode_info" from settings.xml */
  if (!kodi::addon::CheckSettingBoolean("add_rec_episode_info", m_bAddRecEpisode2title))
  {
    /* If setting is unknown fallback to defaults */
    kodi::Log(ADDON_LOG_ERROR,
              "Couldn't get 'add_rec_episode_info' setting, falling back to '%s' as default",
              DEFAULT_ADDRECEPISODE2TITLE ? "true" : "false");
    m_bAddRecEpisode2title = DEFAULT_ADDRECEPISODE2TITLE;
  }

  /* Read setting "group_recordings_by_series" from settings.xml */
  if (!kodi::addon::CheckSettingBoolean("group_recordings_by_series", m_bGroupRecBySeries))
  {
    /* If setting is unknown fallback to defaults */
    kodi::Log(ADDON_LOG_ERROR,
              "Couldn't get 'group_recordings_by_series' setting, falling back to '%s' as default",
              DEFAULT_GROUPRECBYSERIES ? "true" : "false");
    m_bGroupRecBySeries = DEFAULT_GROUPRECBYSERIES;
  }

  /* Read setting "no_group_for_single_record" from settings.xml */
  if (!kodi::addon::CheckSettingBoolean("no_group_for_single_record", m_bNoGroupSingleRec))
  {
    /* If setting is unknown fallback to defaults */
    kodi::Log(ADDON_LOG_ERROR,
              "Couldn't get 'no_group_for_single_record' setting, falling back to '%s' as default",
              DEFAULT_NOGROUP_SINGLE_REC ? "true" : "false");
    m_bNoGroupSingleRec = DEFAULT_NOGROUP_SINGLE_REC;
  }

  /* Read setting "height" from settings.xml */
  if (!kodi::addon::CheckSettingInt("height", m_iHeight))
  {
    /* If setting is unknown fallback to defaults */
    kodi::Log(ADDON_LOG_ERROR, "Couldn't get 'height' setting, falling back to '%i' as default",
              DEFAULT_HEIGHT);
    m_iHeight = DEFAULT_HEIGHT;
  }

  /* Read setting "width" from settings.xml */
  if (!kodi::addon::CheckSettingInt("width", m_iWidth))
  {
    /* If setting is unknown fallback to defaults */
    kodi::Log(ADDON_LOG_ERROR, "Couldn't get 'width' setting, falling back to '%i' as default",
              DEFAULT_WIDTH);
    m_iWidth = DEFAULT_WIDTH;
  }

  /* Read setting "bitrate" from settings.xml */
  if (!kodi::addon::CheckSettingInt("bitrate", m_iBitrate))
  {
    /* If setting is unknown fallback to defaults */
    kodi::Log(ADDON_LOG_ERROR, "Couldn't get 'bitrate' setting, falling back to '%i' as default",
              DEFAULT_BITRATE);
    m_iBitrate = DEFAULT_BITRATE;
  }

  /* Read setting "audiotrack" from settings.xml */
  if (!kodi::addon::CheckSettingString("audiotrack", m_szAudiotrack))
  {
    /* If setting is unknown fallback to defaults */
    kodi::Log(ADDON_LOG_ERROR, "Couldn't get 'audiotrack' setting, falling back to '%s' as default",
              DEFAULT_AUDIOTRACK);
    m_szAudiotrack = DEFAULT_AUDIOTRACK;
  }

  /* Read setting "default_update_interval" from settings.xml */
  if (!kodi::addon::CheckSettingInt("default_update_interval", m_iDefaultUpdateInterval))
  {
    /* If setting is unknown fallback to defaults */
    kodi::Log(ADDON_LOG_ERROR,
              "Couldn't get 'default_update_interval' setting, falling back to '%i' as default",
              DEFAULT_UPDATE_INTERVAL);
    m_iDefaultUpdateInterval = DEFAULT_UPDATE_INTERVAL;
  }

  /* Read setting "bitrate" from settings.xml */
  if (!kodi::addon::CheckSettingInt("default_record_show_type", m_iDefaultRecShowType))
  {
    /* If setting is unknown fallback to defaults */
    kodi::Log(ADDON_LOG_ERROR,
              "Couldn't get 'default_record_show_type' setting, falling back to '%i' as default",
              DEFAULT_RECORD_SHOW_TYPE);
    m_iDefaultRecShowType = DEFAULT_RECORD_SHOW_TYPE;
  }

  /* Log the current settings for debugging purposes */
  kodi::Log(ADDON_LOG_DEBUG, "settings: enable_transcoding='%i' host='%s', port=%i",
            m_bUseTranscoding, m_szHostname.c_str(), m_iPort);

  return true;
}

ADDON_STATUS CSettings::SetSetting(const std::string& settingName,
                                   const kodi::addon::CSettingValue& settingValue)
{
  if (settingName == "host")
  {
    std::string tmp_sHostname;
    kodi::Log(ADDON_LOG_INFO, "Changed Setting 'host' from %s to %s", m_szHostname.c_str(),
              settingValue.GetString().c_str());
    tmp_sHostname = m_szHostname;
    m_szHostname = settingValue.GetString();
    if (tmp_sHostname != m_szHostname)
      return ADDON_STATUS_NEED_RESTART;
  }
  else if (settingName == "username")
  {
    std::string tmp_sUsername;
    kodi::Log(ADDON_LOG_INFO, "Changed Setting 'username' from %s to %s", m_szUsername.c_str(),
              settingValue.GetString().c_str());
    tmp_sUsername = m_szUsername;
    m_szUsername = settingValue.GetString();
    if (tmp_sUsername != m_szUsername)
      return ADDON_STATUS_NEED_RESTART;
  }
  else if (settingName == "password")
  {
    std::string tmp_sPassword;
    kodi::Log(ADDON_LOG_INFO, "Changed Setting 'password' from %s to %s", m_szPassword.c_str(),
              settingValue.GetString().c_str());
    tmp_sPassword = m_szPassword;
    m_szPassword = settingValue.GetString();
    if (tmp_sPassword != m_szPassword)
      return ADDON_STATUS_NEED_RESTART;
  }
  else if (settingName == "enable_transcoding")
  {
    kodi::Log(ADDON_LOG_INFO, "Changed setting 'enable_transcoding' from %u to %u",
              m_bUseTranscoding, settingValue.GetBoolean());
    m_bUseTranscoding = settingValue.GetBoolean();
    return ADDON_STATUS_NEED_RESTART;
  }
  else if (settingName == "port")
  {
    kodi::Log(ADDON_LOG_INFO, "Changed Setting 'port' from %u to %u", m_iPort,
              settingValue.GetInt());
    if (m_iPort != settingValue.GetInt())
    {
      m_iPort = settingValue.GetInt();
      return ADDON_STATUS_NEED_RESTART;
    }
  }
  else if (settingName == "timeshift")
  {
    kodi::Log(ADDON_LOG_INFO, "Changed setting 'timeshift' from %u to %u", m_bUseTimeshift,
              settingValue.GetBoolean());
    m_bUseTimeshift = settingValue.GetBoolean();
    return ADDON_STATUS_NEED_RESTART;
  }
  else if (settingName == "showinfomsg")
  {
    kodi::Log(ADDON_LOG_INFO, "Changed setting 'showinfomsg' from %u to %u", m_bShowInfoMSG,
              settingValue.GetBoolean());
    m_bShowInfoMSG = settingValue.GetBoolean();
  }
  else if (settingName == "add_rec_episode_info")
  {
    kodi::Log(ADDON_LOG_INFO, "Changed setting 'add_rec_episode_info' from %u to %u",
              m_bAddRecEpisode2title, settingValue.GetBoolean());
    m_bAddRecEpisode2title = settingValue.GetBoolean();
    return ADDON_STATUS_NEED_RESTART;
  }
  else if (settingName == "group_recordings_by_series")
  {
    kodi::Log(ADDON_LOG_INFO, "Changed setting 'group_recordings_by_series' from %u to %u",
              m_bGroupRecBySeries, settingValue.GetBoolean());
    m_bGroupRecBySeries = settingValue.GetBoolean();
    return ADDON_STATUS_NEED_RESTART;
  }
  else if (settingName == "no_group_for_single_record")
  {
    kodi::Log(ADDON_LOG_INFO, "Changed setting 'no_group_for_single_record' from %u to %u",
              m_bNoGroupSingleRec, settingValue.GetBoolean());
    m_bNoGroupSingleRec = settingValue.GetBoolean();
    return ADDON_STATUS_NEED_RESTART;
  }
  else if (settingName == "height")
  {
    kodi::Log(ADDON_LOG_INFO, "Changed setting 'height' from %u to %u", m_iHeight,
              settingValue.GetInt());
    m_iHeight = settingValue.GetInt();
  }
  else if (settingName == "width")
  {
    kodi::Log(ADDON_LOG_INFO, "Changed setting 'width' from %u to %u", m_iWidth,
              settingValue.GetInt());
    m_iWidth = settingValue.GetInt();
  }
  else if (settingName == "bitrate")
  {
    kodi::Log(ADDON_LOG_INFO, "Changed setting 'bitrate' from %u to %u", m_iBitrate,
              settingValue.GetInt());
    m_iBitrate = settingValue.GetInt();
  }
  else if (settingName == "audiotrack")
  {
    std::string tmp_sAudiotrack;
    kodi::Log(ADDON_LOG_INFO, "Changed Setting 'audiotrack' from %s to %s", m_szAudiotrack.c_str(),
              settingValue.GetString().c_str());
    tmp_sAudiotrack = m_szAudiotrack;
    m_szAudiotrack = settingValue.GetString();
    if (tmp_sAudiotrack != m_szAudiotrack)
      return ADDON_STATUS_NEED_RESTART;
  }
  else if (settingName == "default_update_interval")
  {
    kodi::Log(ADDON_LOG_INFO, "Changed setting 'default_update_interval' from %u to %u",
              m_iDefaultUpdateInterval, settingValue.GetInt());
    m_iDefaultUpdateInterval = settingValue.GetInt();
  }
  else if (settingName == "default_record_show_type")
  {
    kodi::Log(ADDON_LOG_INFO, "Changed setting 'default_record_show_type' from %u to %u",
              m_iDefaultRecShowType, settingValue.GetInt());
    m_iDefaultRecShowType = settingValue.GetInt();
  }

  return ADDON_STATUS_OK;
}
