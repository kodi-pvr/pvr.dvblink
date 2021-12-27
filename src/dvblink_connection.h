/*
 *  Copyright (C) 2005-2021 Team Kodi (https://kodi.tv)
 *  Copyright (C) 2012 Palle Ehmsen(Barcode Madness) (http://www.barcodemadness.com)
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#pragma once

#include "HttpPostClient.h"
#include "libdvblinkremote/dvblinkremote.h"

#include <mutex>

template<typename T> void SafeDelete(T*& p)
{
  if (p)
  {
    delete p;
    p = nullptr;
  }
}

struct ATTR_DLL_LOCAL server_connection_properties
{
  server_connection_properties(const std::string& address,
                               long port,
                               const std::string& username,
                               const std::string& password,
                               const std::string& client_id)
    : address_(address),
      port_(port),
      username_(username),
      password_(password),
      client_id_(client_id)
  {
  }

  std::string address_;
  long port_;
  std::string username_;
  std::string password_;
  std::string client_id_;
};

class ATTR_DLL_LOCAL dvblink_server_connection : public dvblinkremote::DVBLinkRemoteLocker
{
public:
  dvblink_server_connection(const server_connection_properties& connection_props)
  {
    http_client_ = new HttpPostClient(connection_props.address_, connection_props.port_,
                                      connection_props.username_, connection_props.password_);

    dvblink_connection_ = dvblinkremote::DVBLinkRemote::Connect(
        *http_client_, connection_props.address_.c_str(), connection_props.port_,
        connection_props.username_.c_str(), connection_props.password_.c_str(), this);
  }

  ~dvblink_server_connection()
  {
    SafeDelete(dvblink_connection_);
    SafeDelete(http_client_);
  }

  dvblinkremote::IDVBLinkRemoteConnection* get_connection() { return dvblink_connection_; }

protected:
  virtual void lock() { m_comm_mutex.lock(); }

  virtual void unlock() { m_comm_mutex.unlock(); }

  std::mutex m_comm_mutex;
  HttpPostClient* http_client_;
  dvblinkremote::IDVBLinkRemoteConnection* dvblink_connection_;
};
