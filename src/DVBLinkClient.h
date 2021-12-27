/*
 *  Copyright (C) 2005-2021 Team Kodi (https://kodi.tv)
 *  Copyright (C) 2012 Palle Ehmsen(Barcode Madness) (http://www.barcodemadness.com)
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#pragma once

#include "HttpPostClient.h"
#include "RecordingStreamer.h"
#include "TimeShiftBuffer.h"
#include "dvblink_connection.h"
#include "libdvblinkremote/dvblinkremote.h"

#include <atomic>
#include <kodi/addon-instance/PVR.h>
#include <map>
#include <mutex>
#include <thread>

#define DVBLINK_BUILD_IN_RECORDER_SOURCE_ID "8F94B459-EFC0-4D91-9B29-EC3D72E92677"
#define DVBLINK_RECODINGS_BY_DATA_ID "F6F08949-2A07-4074-9E9D-423D877270BB"
#define DVBLINK_RECODINGS_BY_SERIES_ID "0E03FEB8-BD8F-46e7-B3EF-34F6890FB458"

class CDVBLinkAddon;

typedef std::map<std::string, std::string> recording_id_to_url_map_t;

/* timer type ids */
#define TIMER_ONCE_MANUAL (PVR_TIMER_TYPE_NONE + 1)
#define TIMER_ONCE_EPG (PVR_TIMER_TYPE_NONE + 2)
#define TIMER_ONCE_MANUAL_CHILD (PVR_TIMER_TYPE_NONE + 3)
#define TIMER_ONCE_EPG_CHILD (PVR_TIMER_TYPE_NONE + 4)
#define TIMER_ONCE_KEYWORD_CHILD (PVR_TIMER_TYPE_NONE + 5)
#define TIMER_REPEATING_MANUAL (PVR_TIMER_TYPE_NONE + 6)
#define TIMER_REPEATING_EPG (PVR_TIMER_TYPE_NONE + 7)
#define TIMER_REPEATING_KEYWORD (PVR_TIMER_TYPE_NONE + 8)

enum dvblink_client_rec_num_e
{
  dcrn_keep_all = 0,
  dcrn_keep_1 = 1,
  dcrn_keep_2 = 2,
  dcrn_keep_3 = 3,
  dcrn_keep_4 = 4,
  dcrn_keep_5 = 5,
  dcrn_keep_6 = 6,
  dcrn_keep_7 = 7,
  dcrn_keep_10 = 10
};

enum dvblink_client_rec_showtype_e
{
  dcrs_record_all = 0,
  dcrs_record_new_only = 1
};

/* dvblink update interval values */
const int UPDATE_INTERVAL_60_SEC = 60;
const int UPDATE_INTERVAL_120_SEC = 120;
const int UPDATE_INTERVAL_180_SEC = 180;
const int UPDATE_INTERVAL_240_SEC = 240;
const int UPDATE_INTERVAL_300_SEC = 300;
const int UPDATE_INTERVAL_360_SEC = 360;
const int UPDATE_INTERVAL_420_SEC = 420;
const int UPDATE_INTERVAL_480_SEC = 480;
const int UPDATE_INTERVAL_540_SEC = 540;
const int UPDATE_INTERVAL_600_SEC = 600;
const int UPDATE_INTERVAL_1200_SEC = 1200;
const int UPDATE_INTERVAL_1800_SEC = 1800;
const int UPDATE_INTERVAL_2400_SEC = 2400;
const int UPDATE_INTERVAL_3000_SEC = 3000;
const int UPDATE_INTERVAL_3600_SEC = 3600;

struct ATTR_DLL_LOCAL schedule_desc
{
  schedule_desc(unsigned int idx, int type, int margin_before, int margin_after)
  {
    schedule_kodi_idx = idx;
    schedule_kodi_type = type;
    schedule_margin_before = margin_before;
    schedule_margin_after = margin_after;
  }

  schedule_desc()
    : schedule_kodi_idx(PVR_TIMER_NO_CLIENT_INDEX),
      schedule_kodi_type(PVR_TIMER_TYPE_NONE),
      schedule_margin_before(0),
      schedule_margin_after(0)
  {
  }

  unsigned int schedule_kodi_idx;
  int schedule_kodi_type;
  int schedule_margin_before;
  int schedule_margin_after;
};

struct ATTR_DLL_LOCAL dvblink_server_caps
{
  dvblink_server_caps()
    : setting_margins_supported_(false),
      favorites_supported_(false),
      transcoding_supported_(false),
      transcoding_recordings_supported_(false),
      recordings_supported_(false),
      timeshifting_supported_(false),
      device_management_supported_(false),
      timeshift_commands_supported_(false),
      resume_supported_(false),
      start_any_time_supported_(false)
  {
  }

  std::string server_version_;
  std::string server_build_;
  bool setting_margins_supported_;
  bool favorites_supported_;
  bool transcoding_supported_;
  bool transcoding_recordings_supported_;
  bool recordings_supported_;
  bool timeshifting_supported_;
  bool device_management_supported_;
  bool timeshift_commands_supported_;
  bool resume_supported_;
  bool start_any_time_supported_;
};

class ATTR_DLL_LOCAL DVBLinkClient : public kodi::addon::CInstancePVRClient
{
public:
  DVBLinkClient(const CDVBLinkAddon& base,
                const kodi::addon::IInstanceInfo& instance,
                std::string clientname,
                std::string hostname,
                int port,
                bool showinfomsg,
                std::string username,
                std::string password,
                bool add_episode_to_rec_title,
                bool group_recordings_by_series,
                bool no_group_single_rec,
                int default_update_interval,
                int default_rec_show_type);
  ~DVBLinkClient(void);

  PVR_ERROR GetBackendName(std::string& name) override;
  PVR_ERROR GetBackendVersion(std::string& version) override;
  PVR_ERROR GetBackendHostname(std::string& hostname) override;
  PVR_ERROR GetConnectionString(std::string& connection) override;
  PVR_ERROR GetCapabilities(kodi::addon::PVRCapabilities& capabilities) override;
  PVR_ERROR GetDriveSpace(uint64_t& total, uint64_t& used) override;

  PVR_ERROR GetChannelsAmount(int& amount) override;
  PVR_ERROR GetChannels(bool radio, kodi::addon::PVRChannelsResultSet& results) override;

  PVR_ERROR GetChannelGroupsAmount(int& amount) override;
  PVR_ERROR GetChannelGroups(bool radio, kodi::addon::PVRChannelGroupsResultSet& results) override;
  PVR_ERROR GetChannelGroupMembers(const kodi::addon::PVRChannelGroup& group,
                                   kodi::addon::PVRChannelGroupMembersResultSet& results) override;

  PVR_ERROR GetEPGForChannel(int channelUid,
                             time_t start,
                             time_t end,
                             kodi::addon::PVREPGTagsResultSet& results) override;

  PVR_ERROR GetRecordingsAmount(bool deleted, int& amount) override;
  PVR_ERROR GetRecordings(bool deleted, kodi::addon::PVRRecordingsResultSet& results) override;
  PVR_ERROR DeleteRecording(const kodi::addon::PVRRecording& recording) override;
  PVR_ERROR GetRecordingLastPlayedPosition(const kodi::addon::PVRRecording& recording,
                                           int& position) override;
  PVR_ERROR SetRecordingLastPlayedPosition(const kodi::addon::PVRRecording& recording,
                                           int lastplayedposition) override;

  PVR_ERROR GetTimerTypes(std::vector<kodi::addon::PVRTimerType>& types) override;
  PVR_ERROR GetTimersAmount(int& amount) override;
  PVR_ERROR GetTimers(kodi::addon::PVRTimersResultSet& results) override;
  PVR_ERROR AddTimer(const kodi::addon::PVRTimer& timer) override;
  PVR_ERROR DeleteTimer(const kodi::addon::PVRTimer& timer, bool forceDelete) override;
  PVR_ERROR UpdateTimer(const kodi::addon::PVRTimer& timer) override;

  bool OpenLiveStream(const kodi::addon::PVRChannel& channel) override;
  void CloseLiveStream() override;
  int64_t SeekLiveStream(int64_t iPosition, int iWhence) override;
  PVR_ERROR GetStreamTimes(kodi::addon::PVRStreamTimes& stream_times) override;
  int64_t LengthLiveStream() override;
  bool IsRealTimeStream() override;
  int ReadLiveStream(unsigned char* pBuffer, unsigned int iBufferSize) override;

  bool OpenRecordedStream(const kodi::addon::PVRRecording& recording) override;
  void CloseRecordedStream() override;
  int ReadRecordedStream(unsigned char* pBuffer, unsigned int iBufferSize) override;
  int64_t SeekRecordedStream(int64_t iPosition, int iWhence = SEEK_SET) override;
  int64_t LengthRecordedStream() override;

  bool CanPauseStream() override;
  bool CanSeekStream() override;

  bool GetStatus();
  bool GetRecordingURL(const std::string& recording_id,
                       std::string& url,
                       bool use_transcoder,
                       int width,
                       int height,
                       int bitrate,
                       std::string audiotrack);

private:
  bool DoEPGSearch(dvblinkremote::EpgSearchResult& epgSearchResult,
                   const std::string& channelId,
                   const long startTime,
                   const long endTime,
                   const std::string& programId = "");
  void SetEPGGenre(dvblinkremote::ItemMetadata& metadata, int& genre_type, int& genre_subtype);
  std::string GetBuildInRecorderObjectID();
  std::string GetRecordedTVByDateObjectID(const std::string& buildInRecoderObjectID);
  int GetInternalUniqueIdFromChannelId(const std::string& channelId);
  void Process();
  bool get_dvblink_program_id(std::string& channelId,
                              int start_time,
                              std::string& dvblink_program_id);
  int GetSchedules(kodi::addon::PVRTimersResultSet& results,
                   const dvblinkremote::RecordingList& recordings);
  void get_server_caps();

  std::string make_timer_hash(const std::string& timer_id, const std::string& schedule_id);
  bool parse_timer_hash(const char* timer_hash, std::string& timer_id, std::string& schedule_id);
  unsigned int get_kodi_timer_idx_from_dvblink(const std::string& id);
  bool is_valid_ch_idx(int ch_idx);
  void add_schedule_desc(const std::string& id, const schedule_desc& sd);
  bool get_schedule_desc(const std::string& id, schedule_desc& sd);

  typedef std::map<int, dvblinkremote::Channel*> dvblink_channel_map_t;

  bool m_connected;
  dvblink_channel_map_t m_channels;
  int m_currentChannelId;
  long m_timerCount;
  long m_recordingCount;
  std::mutex m_mutex;
  std::mutex live_mutex_;
  server_connection_properties connection_props_;
  LiveStreamerBase* m_live_streamer;
  RecordingStreamer* m_recording_streamer = nullptr;
  bool m_add_episode_to_rec_title;
  bool m_group_recordings_by_series;
  bool m_showinfomsg;
  bool m_update_timers_now;
  bool m_update_timers_repeat;
  bool m_update_recordings;
  std::string m_recordingsid;
  std::string m_recordingsid_by_date;
  std::string m_recordingsid_by_series;
  recording_id_to_url_map_t m_recording_id_to_url_map;
  dvblink_server_caps server_caps_;
  dvblinkremote::ChannelFavorites channel_favorites_;
  std::map<std::string, int> inverse_channel_map_;
  bool no_group_single_rec_;
  time_t default_update_interval_sec_;
  int default_rec_show_type_;
  std::map<std::string, schedule_desc> schedule_map_;
  std::map<std::string, unsigned int> timer_idx_map_;
  unsigned int timer_idx_seed_;

  std::atomic<bool> m_running = {false};
  std::thread m_thread;  

  const CDVBLinkAddon& m_base;
};
