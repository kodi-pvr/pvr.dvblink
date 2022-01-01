/*
 *  Copyright (C) 2020-2021 Team Kodi (https://kodi.tv)
 *  Copyright (C) 2010-2011 Marcel Groothuis, Fred Hoogduin
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#include "addon.h"

#include "DVBLinkClient.h"

#include <chrono>

ADDON_STATUS CDVBLinkAddon::CreateInstance(const kodi::addon::IInstanceInfo& instance,
                                           KODI_ADDON_INSTANCE_HDL& hdl)
{
  if (instance.IsType(ADDON_INSTANCE_PVR))
  {
    m_settings.Load();

    //generate a guid to use as a client identification
    std::string clientname;
    GenerateUuid(clientname);
    kodi::Log(ADDON_LOG_INFO, "Generated guid %s to use as a DVBLink client ID",
              clientname.c_str());

    DVBLinkClient* client =
        new DVBLinkClient(*this, instance, clientname, m_settings.Hostname(), m_settings.Port(),
                          m_settings.ShowInfoMSG(), m_settings.Username(), m_settings.Password(),
                          m_settings.AddRecEpisode2title(), m_settings.GroupRecBySeries(),
                          m_settings.NoGroupSingleRec(), m_settings.DefaultUpdateInterval(),
                          m_settings.DefaultRecShowType());

    hdl = client;

    if (!client->GetStatus())
      return ADDON_STATUS_LOST_CONNECTION;

    return ADDON_STATUS_OK;
  }

  return ADDON_STATUS_UNKNOWN;
}

ADDON_STATUS CDVBLinkAddon::SetSetting(const std::string& settingName,
                                       const kodi::addon::CSettingValue& settingValue)
{
  return m_settings.SetSetting(settingName, settingValue);
}

void CDVBLinkAddon::GenerateUuid(std::string& uuid)
{
  using namespace std::chrono;

  int64_t seed_value =
      duration_cast<milliseconds>(
          time_point_cast<milliseconds>(high_resolution_clock::now()).time_since_epoch())
          .count();
  seed_value = seed_value % 1000000000;
  srand((unsigned int)seed_value);

  //fill in uuid string from a template
  std::string template_str = "xxxx-xx-xx-xx-xxxxxx";
  for (size_t i = 0; i < template_str.size(); i++)
  {
    if (template_str[i] != '-')
    {
      double a1 = rand();
      double a3 = RAND_MAX;
      unsigned char ch = (unsigned char)(a1 * 255 / a3);
      char buf[16];
      sprintf(buf, "%02x", ch);
      uuid += buf;
    }
    else
    {
      uuid += '-';
    }
  }
}

ADDONCREATOR(CDVBLinkAddon)
