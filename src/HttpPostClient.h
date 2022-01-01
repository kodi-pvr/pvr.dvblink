/*
 *  Copyright (C) 2005-2021 Team Kodi (https://kodi.tv)
 *  Copyright (C) 2012 Palle Ehmsen(Barcode Madness) (http://www.barcodemadness.com)
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#pragma once

#include "libdvblinkremote/dvblinkremote.h"
#include "libdvblinkremote/dvblinkremotehttp.h"

#include <kodi/AddonBase.h>

class ATTR_DLL_LOCAL HttpPostClient : public dvblinkremotehttp::HttpClient
{
public:
  bool SendRequest(dvblinkremotehttp::HttpWebRequest& request);
  dvblinkremotehttp::HttpWebResponse* GetResponse();
  void GetLastError(std::string& err);
  void UrlEncode(const std::string& str, std::string& outEncodedStr);
  HttpPostClient(const std::string& server,
                 const int serverport,
                 const std::string& username,
                 const std::string& password);

private:
  int SendPostRequest(dvblinkremotehttp::HttpWebRequest& request);
  std::string m_server;
  long m_serverport;
  std::string m_username;
  std::string m_password;
  std::string m_responseData;
  int m_lastReqeuestErrorCode;
};
