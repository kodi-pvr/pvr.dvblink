/*
 *  Copyright (C) 2005-2020 Team Kodi (https://kodi.tv)
 *  Copyright (C) 2012 Palle Ehmsen(Barcode Madness) (http://www.barcodemadness.com)
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#pragma once

#include "p8-platform/os.h"
#include "kodi/libXBMC_pvr.h"
#include "kodi/libXBMC_addon.h"
#include "p8-platform/threads/threads.h"
#include "p8-platform/threads/mutex.h"
#include "p8-platform/util/util.h"
#include "libdvblinkremote/dvblinkremote.h"
#include "HttpPostClient.h"

class RecordingStreamer: public dvblinkremote::DVBLinkRemoteLocker
{
public:
  RecordingStreamer(ADDON::CHelper_libXBMC_addon* xbmc, const std::string& client_id, const std::string& hostname,
      long port, const std::string& username, const std::string& password);
  virtual ~RecordingStreamer();

  bool OpenRecordedStream(const char* recording_id, std::string& url);
  void CloseRecordedStream(void);
  int ReadRecordedStream(unsigned char *pBuffer, unsigned int iBufferSize);
  long long SeekRecordedStream(long long iPosition, int iWhence /* = SEEK_SET */);
  long long LengthRecordedStream(void);
  PVR_ERROR GetStreamTimes(PVR_STREAM_TIMES* stream_times);
protected:
  ADDON::CHelper_libXBMC_addon* xbmc_;
  std::string recording_id_;
  std::string url_;
  long long recording_size_;
  long recording_duration_;
  bool is_in_recording_;
  void* playback_handle_;
  long long cur_pos_;
  std::string client_id_;
  std::string hostname_;
  std::string username_;
  std::string password_;
  HttpPostClient* http_client_;
  dvblinkremote::IDVBLinkRemoteConnection* dvblink_remote_con_;
  long port_;
  time_t prev_check_;
  time_t check_delta_;
  P8PLATFORM::CMutex m_comm_mutex;

  bool get_recording_info(const std::string& recording_id, long long& recording_size, long& recording_duration, bool& is_in_recording);

  virtual void lock()
  {
    m_comm_mutex.Lock();
  }

  virtual void unlock()
  {
    m_comm_mutex.Unlock();
  }
};
