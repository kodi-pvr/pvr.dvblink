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

#include "TimeShiftBuffer.h"
using namespace ADDON;
using namespace dvblinkremote;

//base live streaming class

LiveStreamerBase::LiveStreamerBase(CHelper_libXBMC_addon * XBMC, const server_connection_properties& connection_props) :
  m_streamHandle(NULL), connection_props_(connection_props), server_connection_(XBMC, connection_props)
{
  this->XBMC = XBMC;
}

LiveStreamerBase::~LiveStreamerBase(void)
{
  Stop();
}

int LiveStreamerBase::ReadData(unsigned char *pBuffer, unsigned int iBufferSize)
{
  return XBMC->ReadFile(m_streamHandle, pBuffer, iBufferSize);
}

bool LiveStreamerBase::Start(Channel* channel, bool use_transcoder, int width, int height, int bitrate, const std::string& audiotrack)
{
  m_streamHandle = NULL;

  StreamRequest* sr = GetStreamRequest(channel->GetDvbLinkID(), use_transcoder, width, height, bitrate, audiotrack);

  if (sr != NULL)
  {
    DVBLinkRemoteStatusCode status;
    std::string error;
    if ((status = server_connection_.get_connection()->PlayChannel(*sr, stream_, &error)) == DVBLINK_REMOTE_STATUS_OK)
    {
      streampath_ = stream_.GetUrl();
      m_streamHandle = XBMC->OpenFile(streampath_.c_str(), 0);
    }
    else
    {
      XBMC->Log(LOG_ERROR, "Could not start streaming for channel %s (Error code : %d)", channel->GetDvbLinkID().c_str(), (int)status, error.c_str());
    }

    SAFE_DELETE(sr);
  } else
  {
    XBMC->Log(LOG_ERROR, "m_live_streamer->GetStreamRequest returned NULL. (channel %s)", channel->GetDvbLinkID().c_str());
  }

  return m_streamHandle != NULL;
}

void LiveStreamerBase::Stop()
{
  if (m_streamHandle != NULL)
  {
    XBMC->CloseFile(m_streamHandle);
    m_streamHandle = NULL;

    StopStreamRequest * request = new StopStreamRequest(stream_.GetChannelHandle());

    DVBLinkRemoteStatusCode status;
    std::string error;
    if ((status = server_connection_.get_connection()->StopChannel(*request, &error)) != DVBLINK_REMOTE_STATUS_OK)
    {
      XBMC->Log(LOG_ERROR, "Could not stop stream (Error code : %d Description : %s)", (int)status, error.c_str());
    }

    SAFE_DELETE(request);
  }
}

//live streaming class
LiveTVStreamer::LiveTVStreamer(CHelper_libXBMC_addon* XBMC, const server_connection_properties& connection_props) :
  LiveStreamerBase(XBMC, connection_props)
{
}

StreamRequest* LiveTVStreamer::GetStreamRequest(const std::string& dvblink_channel_id, bool use_transcoder, int width, int height, int bitrate, std::string audiotrack)
{
  StreamRequest* streamRequest = NULL;

  TranscodingOptions options(width, height);
  options.SetBitrate(bitrate);
  options.SetAudioTrack(audiotrack);

  if (use_transcoder)
  {
    streamRequest = new H264TSStreamRequest(connection_props_.address_.c_str(), dvblink_channel_id, connection_props_.client_id_.c_str(), options);
  }
  else
  {
    streamRequest = new RawHttpStreamRequest(connection_props_.address_.c_str(), dvblink_channel_id, connection_props_.client_id_.c_str());
  }

  return streamRequest;
}

//timeshifted live streaming class

TimeShiftBuffer::TimeShiftBuffer(CHelper_libXBMC_addon* XBMC, const server_connection_properties& connection_props, bool use_dvblink_timeshift_cmds) :
LiveStreamerBase(XBMC, connection_props), last_pos_req_time_(-1), last_pos_(0), use_dvblink_timeshift_cmds_(use_dvblink_timeshift_cmds)
{
}

TimeShiftBuffer::~TimeShiftBuffer(void)
{
}

StreamRequest* TimeShiftBuffer::GetStreamRequest(const std::string& dvblink_channel_id, bool use_transcoder, int width, int height, int bitrate, std::string audiotrack)
{
  StreamRequest* streamRequest = NULL;

  TranscodingOptions options(width, height);
  options.SetBitrate(bitrate);
  options.SetAudioTrack(audiotrack);

  if (use_transcoder)
  {
    streamRequest = new H264TSTimeshiftStreamRequest(connection_props_.address_.c_str(), dvblink_channel_id, connection_props_.client_id_.c_str(), options);
  }
  else
  {
    streamRequest = new RawHttpTimeshiftStreamRequest(connection_props_.address_.c_str(), dvblink_channel_id, connection_props_.client_id_.c_str());
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
  XBMC->CloseFile(m_streamHandle);

  if (use_dvblink_timeshift_cmds_)
  {
    TimeshiftSeekRequest* request = new TimeshiftSeekRequest(stream_.GetChannelHandle(), true, iPosition, iWhence);

    DVBLinkRemoteStatusCode status;
    std::string error;
    if ((status = server_connection_.get_connection()->TimeshiftSeek(*request, &error)) != DVBLINK_REMOTE_STATUS_OK)
    {
      XBMC->Log(LOG_ERROR, "TimeshiftSeek failed (Error code : %d Description : %s)", (int)status, error.c_str());
    } else 
    {
      //update current playback position
      long long length, cur_pos_sec;
      time_t duration;

      GetBufferParams(length, duration, ret_val, cur_pos_sec);
    }

    SAFE_DELETE(request);
  } else
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
  m_streamHandle = XBMC->OpenFile(streampath_.c_str(), 0);

  return ret_val;
}

long long TimeShiftBuffer::Position()
{
  long long ret_val = 0;

  time_t duration;
  long long length, cur_pos_sec;
  GetBufferParams(length, duration, ret_val, cur_pos_sec);

  return ret_val;
}

bool TimeShiftBuffer::ExecuteServerRequest(const std::string& url, std::vector<std::string>& response_values)
{
  bool ret_val = false;
  response_values.clear();

  void* req_handle = XBMC->OpenFile(url.c_str(), 0);
  if (req_handle != NULL)
  {
    char resp_buf[1024];
    unsigned int read = XBMC->ReadFile(req_handle, resp_buf, sizeof(resp_buf));

    if (read > 0)
    {
      //add zero at the end to turn response into string
      resp_buf[read] = '\0';
      char* token = strtok(resp_buf, ",");
      while (token != NULL)
      {
        response_values.push_back(token);
        /* Get next token: */
        token = strtok(NULL, ",");
      }
      ret_val = response_values.size() > 0;
    }

    XBMC->CloseFile(req_handle);
  }

  return ret_val;
}

long long TimeShiftBuffer::Length()
{
  long long ret_val = 0;

  time_t duration;
  long long cur_pos, cur_pos_sec;
  GetBufferParams(ret_val, duration, cur_pos, cur_pos_sec);

  return ret_val;
}

bool TimeShiftBuffer::GetBufferParams(long long& length, time_t& duration, long long& cur_pos, long long& cur_pos_sec)
{
  bool ret_val = false;

  if (use_dvblink_timeshift_cmds_)
  {
    GetTimeshiftStatsRequest* request = new GetTimeshiftStatsRequest(stream_.GetChannelHandle());

    DVBLinkRemoteStatusCode status;
    std::string error;
    TimeshiftStats response;
    if ((status = server_connection_.get_connection()->GetTimeshiftStats(*request, response, &error)) != DVBLINK_REMOTE_STATUS_OK)
    {
      XBMC->Log(LOG_ERROR, "GetTimeshiftStats failed (Error code : %d Description : %s)", (int)status, error.c_str());
    } else
    {
      length = response.curBufferLength;
      duration = (time_t)response.bufferDurationSec;
      cur_pos = response.curPosBytes;
      cur_pos_sec = response.curPosSec;
      ret_val = true;
    }

    SAFE_DELETE(request);
  } else
  {
    std::string req_url = streampath_;
    req_url += "&get_stats=1";

    std::vector<std::string> response_values;
    if (ExecuteServerRequest(req_url, response_values) && response_values.size() == 3)
    {
      length = atoll(response_values[0].c_str());
      duration = (time_t) atoll(response_values[1].c_str());
      cur_pos = atoll(response_values[2].c_str());

      if (length == 0)
        cur_pos_sec = 0;
      else
        cur_pos_sec = cur_pos * duration / length;

      ret_val = true;
    }
  }

  return ret_val;
}

time_t TimeShiftBuffer::GetPlayingTime()
{
  time_t ret_val = last_pos_;

  time_t now;
  now = time(NULL);

  if (last_pos_req_time_ == -1 || now > last_pos_req_time_)
  {
    long long length, cur_pos, cur_pos_sec;
    time_t duration;
    if (GetBufferParams(length, duration, cur_pos, cur_pos_sec))
      ret_val = now - (duration - cur_pos_sec);

    last_pos_ = ret_val;
    last_pos_req_time_ = now;
  }

  return ret_val;
}

time_t TimeShiftBuffer::GetBufferTimeStart()
{
  time_t ret_val = 0;

  time_t now;
  now = time(NULL);

  long long length, cur_pos, cur_pos_sec;
  time_t duration;
  if (GetBufferParams(length, duration, cur_pos, cur_pos_sec))
    ret_val = now - duration;

  return ret_val;
}

time_t TimeShiftBuffer::GetBufferTimeEnd()
{
  time_t now;
  now = time(NULL);

  return now;
}
