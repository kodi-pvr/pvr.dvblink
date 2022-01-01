/*
 *  Copyright (C) 2005-2021 Team Kodi (https://kodi.tv)
 *  Copyright (C) 2012 Palle Ehmsen(Barcode Madness) (http://www.barcodemadness.com)
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#pragma once

#include "dvblink_connection.h"

#include <kodi/Filesystem.h>
#include <kodi/addon-instance/pvr/Stream.h>

class ATTR_DLL_LOCAL LiveStreamerBase
{
public:
  LiveStreamerBase(const server_connection_properties& connection_props);
  virtual ~LiveStreamerBase();

  bool Start(dvblinkremote::Channel* channel,
             bool use_transcoder,
             int width,
             int height,
             int bitrate,
             const std::string& audiotrack);
  void Stop();

  virtual int ReadData(unsigned char* pBuffer, unsigned int iBufferSize);

  virtual long long Seek(long long iPosition, int iWhence) { return -1; }
  virtual long long Position() { return -1; }
  virtual long long Length() { return -1; }
  virtual void GetStreamTimes(kodi::addon::PVRStreamTimes& stream_times)
  {
    stream_times.SetStartTime(stream_start_);
    stream_times.SetPTSStart(0);
    stream_times.SetPTSBegin(0);
    stream_times.SetPTSEnd(0);
  }
  virtual bool IsLive() { return true; }

protected:
  virtual dvblinkremote::StreamRequest* GetStreamRequest(const std::string& dvblink_channel_id,
                                                         bool use_transcoder,
                                                         int width,
                                                         int height,
                                                         int bitrate,
                                                         std::string audiotrack)
  {
    return nullptr;
  }

  kodi::vfs::CFile m_streamHandle;
  std::string streampath_;
  server_connection_properties connection_props_;
  dvblink_server_connection server_connection_;
  dvblinkremote::Stream stream_;
  time_t stream_start_;
};

class ATTR_DLL_LOCAL LiveTVStreamer : public LiveStreamerBase
{
public:
  LiveTVStreamer(const server_connection_properties& connection_props);

protected:
  virtual dvblinkremote::StreamRequest* GetStreamRequest(const std::string& dvblink_channel_id,
                                                         bool use_transcoder,
                                                         int width,
                                                         int height,
                                                         int bitrate,
                                                         std::string audiotrack);
};

class ATTR_DLL_LOCAL TimeShiftBuffer : public LiveStreamerBase
{
  struct buffer_params_t
  {
    long long buffer_length;
    time_t buffer_duration;
    long long cur_pos;
    long long cur_pos_sec;
  };

public:
  TimeShiftBuffer(const server_connection_properties& connection_props,
                  bool use_dvblink_timeshift_cmds);
  ~TimeShiftBuffer(void);

  virtual long long Seek(long long iPosition, int iWhence);
  virtual long long Position();
  virtual long long Length();
  virtual void GetStreamTimes(kodi::addon::PVRStreamTimes& stream_times);
  virtual bool IsLive();

protected:
  virtual dvblinkremote::StreamRequest* GetStreamRequest(const std::string& dvblink_channel_id,
                                                         bool use_transcoder,
                                                         int width,
                                                         int height,
                                                         int bitrate,
                                                         std::string audiotrack);
  bool ExecuteServerRequest(const std::string& url, std::vector<std::string>& response_values);
  bool GetBufferParams(buffer_params_t& buffer_params);

  time_t last_pos_req_time_;
  buffer_params_t buffer_params_;
  bool use_dvblink_timeshift_cmds_;
};
