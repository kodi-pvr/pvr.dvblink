/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://xbmc.org
 
 *      Copyright (C) 2012 Palle Ehmsen(Barcode Madness)
 *      http://www.barcodemadness.com
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 51 Franklin Street, Fifth Floor, Boston,
 *  MA 02110-1301  USA
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#pragma once

#include "p8-platform/os.h"
#include "p8-platform/threads/threads.h"
#include "p8-platform/threads/mutex.h"
#include "p8-platform/util/util.h"
#include "libdvblinkremote/dvblinkremote.h"
#include "HttpPostClient.h"
#include "xbmc_pvr_types.h"
#include "libXBMC_addon.h"
#include "libXBMC_pvr.h"
#include "libKODI_guilib.h"

struct server_connection_properties
{
  server_connection_properties(const std::string& address, long port, const std::string& username, const std::string& password, const std::string& client_id) :
    address_(address), port_(port), username_(username), password_(password), client_id_(client_id)
  {}

  std::string address_;
  long port_;
  std::string username_;
  std::string password_;
  std::string client_id_;
};

class dvblink_server_connection : public dvblinkremote::DVBLinkRemoteLocker
{
  public:
    dvblink_server_connection(ADDON::CHelper_libXBMC_addon* xbmc, const server_connection_properties& connection_props)
    {
      http_client_ = new HttpPostClient(xbmc, connection_props.address_, connection_props.port_, connection_props.username_, connection_props.password_);

      dvblink_connection_ = dvblinkremote::DVBLinkRemote::Connect(*http_client_, connection_props.address_.c_str(), connection_props.port_,
        connection_props.username_.c_str(), connection_props.password_.c_str(), this);
    }

    ~dvblink_server_connection()
    {
      SAFE_DELETE(dvblink_connection_);
      SAFE_DELETE(http_client_);
    }

    dvblinkremote::IDVBLinkRemoteConnection* get_connection() { return dvblink_connection_ ;}

protected:
  virtual void lock()
    {
      m_comm_mutex.Lock();
    }

    virtual void unlock()
    {
      m_comm_mutex.Unlock();
    }

    P8PLATFORM::CMutex m_comm_mutex;
    HttpPostClient* http_client_;
    dvblinkremote::IDVBLinkRemoteConnection* dvblink_connection_;
};

