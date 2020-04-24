/*
 *  Copyright (C) 2005-2020 Team Kodi (https://kodi.tv)
 *  Copyright (C) 2012 Palle Ehmsen(Barcode Madness) (http://www.barcodemadness.com)
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#pragma once

#include "libdvblinkremote/dvblinkremote.h"
#include "libdvblinkremote/dvblinkremotehttp.h"
#include "kodi/libXBMC_addon.h"

class HttpPostClient: public dvblinkremotehttp::HttpClient
{
public:
  bool SendRequest(dvblinkremotehttp::HttpWebRequest& request);
  dvblinkremotehttp::HttpWebResponse* GetResponse();
  void GetLastError(std::string& err);
  void UrlEncode(const std::string& str, std::string& outEncodedStr);
  HttpPostClient(ADDON::CHelper_libXBMC_addon *XBMC, const std::string& server, const int serverport,
      const std::string& username, const std::string& password);

private:
  int SendPostRequest(dvblinkremotehttp::HttpWebRequest& request);
  std::string m_server;
  long m_serverport;
  std::string m_username;
  std::string m_password;
  ADDON::CHelper_libXBMC_addon *XBMC;
  std::string m_responseData;
  int m_lastReqeuestErrorCode;

};
