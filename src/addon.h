/*
 *  Copyright (C) 2005-2021 Team Kodi (https://kodi.tv)
 *  Copyright (C) 2012 Palle Ehmsen(Barcode Madness) (http://www.barcodemadness.com)
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#pragma once

#include "Settings.h"

#include <kodi/AddonBase.h>

class cPVRClientArgusTV;

class ATTR_DLL_LOCAL CDVBLinkAddon : public kodi::addon::CAddonBase
{
public:
  CDVBLinkAddon() = default;

  ADDON_STATUS CreateInstance(const kodi::addon::IInstanceInfo& instance,
                              KODI_ADDON_INSTANCE_HDL& hdl) override;

  ADDON_STATUS SetSetting(const std::string& settingName,
                          const kodi::addon::CSettingValue& settingValue) override;
  const CSettings& GetSettings() const { return m_settings; }

private:
  void GenerateUuid(std::string& uuid);

  CSettings m_settings;
};
