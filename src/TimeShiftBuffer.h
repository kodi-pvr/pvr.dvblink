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

#include "libXBMC_addon.h"
#include "p8-platform/os.h"
#include "p8-platform/util/util.h"
#include "dvblink_connection.h"

class LiveStreamerBase
{
public:
  LiveStreamerBase(ADDON::CHelper_libXBMC_addon * XBMC, const server_connection_properties& connection_props);
  virtual ~LiveStreamerBase();

  bool Start(dvblinkremote::Channel* channel, bool use_transcoder, int width, int height, int bitrate, const std::string& audiotrack);
  void Stop();

  virtual int ReadData(unsigned char *pBuffer, unsigned int iBufferSize);

  virtual long long Seek(long long iPosition, int iWhence)
  {
    return -1;
  }
  virtual long long Position()
  {
    return -1;
  }
  virtual long long Length()
  {
    return -1;
  }
  virtual void GetStreamTimes(PVR_STREAM_TIMES* stream_times)
  {
    stream_times->startTime = stream_start_;
    stream_times->ptsStart = 0;
    stream_times->ptsBegin = 0;
    stream_times->ptsEnd = 0;
  }
  virtual bool IsLive()
  {
    return true;
  }

protected:
  virtual dvblinkremote::StreamRequest* GetStreamRequest(const std::string& dvblink_channel_id, bool use_transcoder, int width, int height, int bitrate, std::string audiotrack)
  {
    return NULL;
  }

  void * m_streamHandle;
  ADDON::CHelper_libXBMC_addon * XBMC;
  std::string streampath_;
  server_connection_properties connection_props_;
  dvblink_server_connection server_connection_;
  dvblinkremote::Stream stream_;
  time_t stream_start_;
};

class LiveTVStreamer: public LiveStreamerBase
{
public:
  LiveTVStreamer(ADDON::CHelper_libXBMC_addon * XBMC, const server_connection_properties& connection_props);

protected:
  virtual dvblinkremote::StreamRequest* GetStreamRequest(const std::string& dvblink_channel_id, bool use_transcoder, int width, int height, int bitrate, std::string audiotrack);
};

class TimeShiftBuffer: public LiveStreamerBase
{
  struct buffer_params_t
  {
    long long buffer_length;
    time_t buffer_duration;
    long long cur_pos;
    long long cur_pos_sec;
  };

public:
  TimeShiftBuffer(ADDON::CHelper_libXBMC_addon * XBMC, const server_connection_properties& connection_props, bool use_dvblink_timeshift_cmds);
  ~TimeShiftBuffer(void);

  virtual long long Seek(long long iPosition, int iWhence);
  virtual long long Position();
  virtual long long Length();
  virtual void GetStreamTimes(PVR_STREAM_TIMES* stream_times);
  virtual bool IsLive();

protected:
  virtual dvblinkremote::StreamRequest* GetStreamRequest(const std::string& dvblink_channel_id, bool use_transcoder, int width, int height, int bitrate, std::string audiotrack);
  bool ExecuteServerRequest(const std::string& url, std::vector<std::string>& response_values);
  bool GetBufferParams(buffer_params_t& buffer_params);

  time_t last_pos_req_time_;
  buffer_params_t buffer_params_;
  bool use_dvblink_timeshift_cmds_;
};

