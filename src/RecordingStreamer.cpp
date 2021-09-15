/*
 *  Copyright (C) 2005-2021 Team Kodi (https://kodi.tv)
 *  Copyright (C) 2012 Palle Ehmsen(Barcode Madness) (http://www.barcodemadness.com)
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#include "RecordingStreamer.h"

#include <kodi/addon-instance/inputstream/TimingConstants.h>
#include <time.h>

using namespace dvblinkremotehttp;
using namespace dvblinkremote;

RecordingStreamer::RecordingStreamer(const std::string& client_id,
                                     const std::string& hostname,
                                     int port,
                                     const std::string& username,
                                     const std::string& password)
  : client_id_(client_id),
    hostname_(hostname),
    username_(username),
    password_(password),
    port_(port),
    check_delta_(30)
{
  http_client_ = new HttpPostClient(hostname_, port_, username_, password_);
  dvblink_remote_con_ = DVBLinkRemote::Connect((HttpClient&)*http_client_, hostname_.c_str(), port_,
                                               username_.c_str(), password_.c_str(), this);
}

RecordingStreamer::~RecordingStreamer()
{
  delete dvblink_remote_con_;
  delete http_client_;
}

bool RecordingStreamer::OpenRecordedStream(const std::string& recording_id, std::string& url)
{
  recording_id_ = recording_id;
  url_ = url;
  cur_pos_ = 0;

  prev_check_ = time(nullptr);
  get_recording_info(recording_id_, recording_size_, recording_duration_, is_in_recording_);

  return playback_handle_.OpenFile(url);
}

void RecordingStreamer::CloseRecordedStream(void)
{
  playback_handle_.Close();
  }

  int RecordingStreamer::ReadRecordedStream(unsigned char* pBuffer, unsigned int iBufferSize)
  {
    //if this is recording in progress - update its size every minute
    if (is_in_recording_)
    {
      time_t now = time(nullptr);
      if (now - prev_check_ > check_delta_)
      {
        get_recording_info(recording_id_, recording_size_, recording_duration_, is_in_recording_);

        //reopen original data connection to refresh its file size
        playback_handle_.Close();
        playback_handle_.OpenFile(url_);
        playback_handle_.Seek(cur_pos_, SEEK_SET);

        prev_check_ = now;
      }
    }

    unsigned int n = playback_handle_.Read(pBuffer, iBufferSize);
    cur_pos_ += n;

    return n;
  }

  long long RecordingStreamer::SeekRecordedStream(long long iPosition, int iWhence /* = SEEK_SET */)
  {
    cur_pos_ = playback_handle_.Seek(iPosition, iWhence);
    return cur_pos_;
  }

  long long RecordingStreamer::LengthRecordedStream(void)
  {
    return recording_size_;
  }

  PVR_ERROR RecordingStreamer::GetStreamTimes(kodi::addon::PVRStreamTimes& stream_times)
  {
    stream_times.SetStartTime(0);
    stream_times.SetPTSStart(0);
    stream_times.SetPTSBegin(0);
    stream_times.SetPTSEnd((int64_t)recording_duration_ * STREAM_TIME_BASE);
    return PVR_ERROR_NO_ERROR;
  }

  bool RecordingStreamer::get_recording_info(const std::string& recording_id,
                                             long long& recording_size,
                                             long& recording_duration,
                                             bool& is_in_recording)
  {
    bool ret_val = false;
    recording_size = -1;
    is_in_recording = false;

    GetPlaybackObjectRequest getPlaybackObjectRequest(hostname_.c_str(), recording_id);
    getPlaybackObjectRequest.IncludeChildrenObjectsForRequestedObject = false;
    GetPlaybackObjectResponse getPlaybackObjectResponse;

    std::string error;
    if (dvblink_remote_con_->GetPlaybackObject(getPlaybackObjectRequest, getPlaybackObjectResponse,
                                               &error) != DVBLINK_REMOTE_STATUS_OK)
    {
      kodi::Log(
          ADDON_LOG_ERROR,
          "RecordingStreamer::get_recording_info: Could not get recording info for recording id %s",
          recording_id.c_str());
    }
    else
    {
      PlaybackItemList& item_list = getPlaybackObjectResponse.GetPlaybackItems();
      if (item_list.size() > 0)
      {
        PlaybackItem* item = item_list[0];
        RecordedTvItem* rectv_item = static_cast<RecordedTvItem*>(item);
        recording_size = rectv_item->Size;
        is_in_recording = rectv_item->State == RecordedTvItem::RECORDED_TV_ITEM_STATE_IN_PROGRESS;
        recording_duration = rectv_item->GetMetadata().GetDuration();
        ret_val = true;
      }
      else
      {
      }
    }

    return ret_val;
  }
