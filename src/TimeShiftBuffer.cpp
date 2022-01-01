/*
 *  Copyright (C) 2005-2021 Team Kodi (https://kodi.tv)
 *  Copyright (C) 2012 Palle Ehmsen(Barcode Madness) (http://www.barcodemadness.com)
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#include "TimeShiftBuffer.h"

#include <kodi/General.h>
#include <kodi/addon-instance/inputstream/TimingConstants.h>

using namespace dvblinkremote;

//base live streaming class

LiveStreamerBase::LiveStreamerBase(const server_connection_properties& connection_props)
  : connection_props_(connection_props), server_connection_(connection_props), stream_start_(0)
{
}

LiveStreamerBase::~LiveStreamerBase(void)
{
  Stop();
}

int LiveStreamerBase::ReadData(unsigned char* pBuffer, unsigned int iBufferSize)
{
  return m_streamHandle.Read(pBuffer, iBufferSize);
}

bool LiveStreamerBase::Start(Channel* channel,
                             bool use_transcoder,
                             int width,
                             int height,
                             int bitrate,
                             const std::string& audiotrack)
{
  m_streamHandle.Close();
  stream_start_ = time(nullptr);

  StreamRequest* sr =
      GetStreamRequest(channel->GetDvbLinkID(), use_transcoder, width, height, bitrate, audiotrack);

  if (sr != nullptr)
  {
    DVBLinkRemoteStatusCode status;
    std::string error;
    if ((status = server_connection_.get_connection()->PlayChannel(*sr, stream_, &error)) ==
        DVBLINK_REMOTE_STATUS_OK)
    {
      streampath_ = stream_.GetUrl();
      if (!m_streamHandle.OpenFile(streampath_))
      {
        kodi::Log(ADDON_LOG_ERROR, "Could not open streaming for channel %s",
                  channel->GetDvbLinkID().c_str());
        return false;
      }
    }
    else
    {
      kodi::Log(ADDON_LOG_ERROR, "Could not start streaming for channel %s (Error code : %d Description : %s)",
                channel->GetDvbLinkID().c_str(), (int)status, error.c_str());
      if (status == DVBLINK_REMOTE_STATUS_INVALID_DATA)
        kodi::QueueNotification(QUEUE_ERROR, "", kodi::addon::GetLocalizedString(30007));
    }

    SafeDelete(sr);
  }
  else
  {
    kodi::Log(ADDON_LOG_ERROR, "m_live_streamer->GetStreamRequest returned NULL. (channel %s)",
              channel->GetDvbLinkID().c_str());
  }

  return m_streamHandle.IsOpen();
}

void LiveStreamerBase::Stop()
{
  if (m_streamHandle.IsOpen())
  {
    m_streamHandle.Close();

    StopStreamRequest* request = new StopStreamRequest(stream_.GetChannelHandle());

    DVBLinkRemoteStatusCode status;
    std::string error;
    if ((status = server_connection_.get_connection()->StopChannel(*request, &error)) !=
        DVBLINK_REMOTE_STATUS_OK)
    {
      kodi::Log(ADDON_LOG_ERROR, "Could not stop stream (Error code : %d Description : %s)",
                (int)status, error.c_str());
    }

    SafeDelete(request);
  }
  }

  //live streaming class
  LiveTVStreamer::LiveTVStreamer(const server_connection_properties& connection_props)
    : LiveStreamerBase(connection_props)
  {
  }

  StreamRequest* LiveTVStreamer::GetStreamRequest(const std::string& dvblink_channel_id,
                                                  bool use_transcoder,
                                                  int width,
                                                  int height,
                                                  int bitrate,
                                                  std::string audiotrack)
  {
    StreamRequest* streamRequest = nullptr;

    TranscodingOptions options(width, height);
    options.SetBitrate(bitrate);
    options.SetAudioTrack(audiotrack);

    if (use_transcoder)
    {
      streamRequest =
          new H264TSStreamRequest(connection_props_.address_.c_str(), dvblink_channel_id,
                                  connection_props_.client_id_.c_str(), options);
    }
    else
    {
      streamRequest =
          new RawHttpStreamRequest(connection_props_.address_.c_str(), dvblink_channel_id,
                                   connection_props_.client_id_.c_str());
    }

    return streamRequest;
  }

  //timeshifted live streaming class

  TimeShiftBuffer::TimeShiftBuffer(const server_connection_properties& connection_props,
                                   bool use_dvblink_timeshift_cmds)
    : LiveStreamerBase(connection_props),
      last_pos_req_time_(-1),
      use_dvblink_timeshift_cmds_(use_dvblink_timeshift_cmds)
  {
  }

  TimeShiftBuffer::~TimeShiftBuffer(void)
  {
  }

  StreamRequest* TimeShiftBuffer::GetStreamRequest(const std::string& dvblink_channel_id,
                                                   bool use_transcoder,
                                                   int width,
                                                   int height,
                                                   int bitrate,
                                                   std::string audiotrack)
  {
    StreamRequest* streamRequest = nullptr;

    TranscodingOptions options(width, height);
    options.SetBitrate(bitrate);
    options.SetAudioTrack(audiotrack);

    if (use_transcoder)
    {
      streamRequest =
          new H264TSTimeshiftStreamRequest(connection_props_.address_.c_str(), dvblink_channel_id,
                                           connection_props_.client_id_.c_str(), options);
    }
    else
    {
      streamRequest =
          new RawHttpTimeshiftStreamRequest(connection_props_.address_.c_str(), dvblink_channel_id,
                                            connection_props_.client_id_.c_str());
    }

    return streamRequest;
  }

  long long TimeShiftBuffer::Seek(long long iPosition, int iWhence)
  {
    if (iPosition == 0 && iWhence == SEEK_CUR)
    {
      return Position();
    }

    long long ret_val = 0;

    //close streaming handle before executing seek
    m_streamHandle.Close();

    if (use_dvblink_timeshift_cmds_)
    {
      TimeshiftSeekRequest* request =
          new TimeshiftSeekRequest(stream_.GetChannelHandle(), true, iPosition, iWhence);

      DVBLinkRemoteStatusCode status;
      std::string error;
      if ((status = server_connection_.get_connection()->TimeshiftSeek(*request, &error)) !=
          DVBLINK_REMOTE_STATUS_OK)
      {
        kodi::Log(ADDON_LOG_ERROR, "TimeshiftSeek failed (Error code : %d Description : %s)",
                  (int)status, error.c_str());
      }
      else
      {
        //update current playback position
        buffer_params_t buffer_params;
        GetBufferParams(buffer_params);
        ret_val = buffer_params.cur_pos;
      }

      SafeDelete(request);
    }
    else
    {
      char param_buf[1024];
      sprintf(param_buf, "&seek=%lld&whence=%d", iPosition, iWhence);

      std::string req_url = streampath_;
      req_url += param_buf;

      //execute seek request
      std::vector<std::string> response_values;
      if (ExecuteServerRequest(req_url, response_values))
        ret_val = atoll(response_values[0].c_str());
    }

    //restart streaming
    m_streamHandle.OpenFile(streampath_);

    return ret_val;
  }

  long long TimeShiftBuffer::Position()
  {
    long long ret_val = 0;

    buffer_params_t buffer_params;
    GetBufferParams(buffer_params);
    ret_val = buffer_params.cur_pos;

    return ret_val;
  }

  bool TimeShiftBuffer::ExecuteServerRequest(const std::string& url,
                                             std::vector<std::string>& response_values)
  {
    bool ret_val = false;
    response_values.clear();

    kodi::vfs::CFile req_handle;
    if (req_handle.OpenFile(url))
    {
      char resp_buf[1024];
      unsigned int read = req_handle.Read(resp_buf, sizeof(resp_buf));

      if (read > 0)
      {
        //add zero at the end to turn response into string
        resp_buf[read] = '\0';
        char* token = strtok(resp_buf, ",");
        while (token != nullptr)
        {
          response_values.push_back(token);
          /* Get next token: */
          token = strtok(nullptr, ",");
        }
        ret_val = response_values.size() > 0;
      }

      req_handle.Close();
    }

    return ret_val;
  }

  long long TimeShiftBuffer::Length()
  {
    long long ret_val = 0;

    buffer_params_t buffer_params;
    GetBufferParams(buffer_params);
    ret_val = buffer_params.buffer_length;

    return ret_val;
  }

  void TimeShiftBuffer::GetStreamTimes(kodi::addon::PVRStreamTimes& stream_times)
  {
    time_t now;
    now = time(nullptr);

    buffer_params_t buffer_params;
    GetBufferParams(buffer_params);

    stream_times.SetStartTime(stream_start_);
    stream_times.SetPTSStart(0);
    if (now >= buffer_params.buffer_duration + stream_start_ && now >= stream_start_)
    {
      stream_times.SetPTSBegin((int64_t)(now - buffer_params.buffer_duration - stream_start_) *
                               STREAM_TIME_BASE);
      stream_times.SetPTSEnd((int64_t)(now - stream_start_) * STREAM_TIME_BASE);
    }
    else
    {
      stream_times.SetPTSBegin(0);
      stream_times.SetPTSEnd(0);
    }
  }

  bool TimeShiftBuffer::IsLive()
  {
    buffer_params_t buffer_params;
    GetBufferParams(buffer_params);

    return buffer_params.cur_pos_sec + 10 >=
           buffer_params.buffer_duration; //add 10 seconds to the definition of "live"
  }

  bool TimeShiftBuffer::GetBufferParams(buffer_params_t& buffer_params)
  {
    bool ret_val = false;

    time_t now;
    now = time(nullptr);
    if (last_pos_req_time_ == -1 || now > last_pos_req_time_)
    {
      if (use_dvblink_timeshift_cmds_)
      {
        GetTimeshiftStatsRequest* request =
            new GetTimeshiftStatsRequest(stream_.GetChannelHandle());

        DVBLinkRemoteStatusCode status;
        std::string error;
        TimeshiftStats response;
        if ((status = server_connection_.get_connection()->GetTimeshiftStats(
                 *request, response, &error)) != DVBLINK_REMOTE_STATUS_OK)
        {
          kodi::Log(ADDON_LOG_ERROR, "GetTimeshiftStats failed (Error code : %d Description : %s)",
                    (int)status, error.c_str());
        }
        else
        {
          buffer_params.buffer_length = response.curBufferLength;
          buffer_params.buffer_duration = (time_t)response.bufferDurationSec;
          buffer_params.cur_pos = response.curPosBytes;
          buffer_params.cur_pos_sec = response.curPosSec;
          ret_val = true;
        }

        SafeDelete(request);
      } else
      {
        std::string req_url = streampath_;
        req_url += "&get_stats=1";

        std::vector<std::string> response_values;
        if (ExecuteServerRequest(req_url, response_values) && response_values.size() == 3)
        {
          buffer_params.buffer_length = atoll(response_values[0].c_str());
          buffer_params.buffer_duration = (time_t)atoll(response_values[1].c_str());
          buffer_params.cur_pos = atoll(response_values[2].c_str());

          if (buffer_params.buffer_length == 0)
            buffer_params.cur_pos_sec = 0;
          else
            buffer_params.cur_pos_sec =
                buffer_params.cur_pos * buffer_params.buffer_duration / buffer_params.buffer_length;

          ret_val = true;
        }
      }

      if (ret_val)
      {
        last_pos_req_time_ = now;
        buffer_params_ = buffer_params;
      }
    }
    else
    {
      buffer_params = buffer_params_;
      ret_val = true;
    }

    return ret_val;
  }
