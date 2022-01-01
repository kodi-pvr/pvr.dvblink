/*
 *  Copyright (C) 2005-2021 Team Kodi (https://kodi.tv)
 *  Copyright (C) 2012 Palle Ehmsen(Barcode Madness) (http://www.barcodemadness.com)
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#include "DVBLinkClient.h"

#include "addon.h"

#include <kodi/General.h>
#include <kodi/gui/General.h>
#include <memory>

using namespace dvblinkremote;
using namespace dvblinkremotehttp;

static int default_rec_limit_ = dcrn_keep_all;

static int channel_id_start_seed_ = 100;

std::string DVBLinkClient::GetBuildInRecorderObjectID()
{
  std::string result = "";
  DVBLinkRemoteStatusCode status;
  GetPlaybackObjectRequest getPlaybackObjectRequest(connection_props_.address_.c_str(), "");
  getPlaybackObjectRequest.RequestedObjectType =
      GetPlaybackObjectRequest::REQUESTED_OBJECT_TYPE_ALL;
  getPlaybackObjectRequest.RequestedItemType = GetPlaybackObjectRequest::REQUESTED_ITEM_TYPE_ALL;
  getPlaybackObjectRequest.IncludeChildrenObjectsForRequestedObject = true;
  GetPlaybackObjectResponse getPlaybackObjectResponse;

  dvblink_server_connection srv_connection(connection_props_);
  if ((status = srv_connection.get_connection()->GetPlaybackObject(
           getPlaybackObjectRequest, getPlaybackObjectResponse, nullptr)) ==
      DVBLINK_REMOTE_STATUS_OK)
  {
    for (std::vector<PlaybackContainer*>::iterator it =
             getPlaybackObjectResponse.GetPlaybackContainers().begin();
         it < getPlaybackObjectResponse.GetPlaybackContainers().end(); it++)
    {
      PlaybackContainer* container = (PlaybackContainer*)*it;
      if (strcmp(container->SourceID.c_str(), DVBLINK_BUILD_IN_RECORDER_SOURCE_ID) == 0)
      {
        result = container->GetObjectID();
        break;
      }
    }
  }
  return result;
}

void DVBLinkClient::get_server_caps()
{
  DVBLinkRemoteStatusCode status;

  //get server version and build
  GetServerInfoRequest server_info_request;
  ServerInfo si;

  dvblink_server_connection srv_connection(connection_props_);
  if ((status = srv_connection.get_connection()->GetServerInfo(server_info_request, si, nullptr)) ==
      DVBLINK_REMOTE_STATUS_OK)
  {
    server_caps_.server_version_ = si.version_;
    server_caps_.server_build_ = si.build_;
    int server_build = atoi(si.build_.c_str());

    //server with build earlier than 11410 does not support setting margins
    server_caps_.setting_margins_supported_ = (server_build >= 11405);

    //server with build earlier than 12700 does not support playing transcoded recordings
    server_caps_.transcoding_recordings_supported_ = (server_build >= 12700);

    //resume position is supported on server with build 16830 and up
    server_caps_.resume_supported_ = (server_build >= 16830);

    //only dvblink server v6 and up supports timeshift commands
    int v1, v2, v3;
    if (sscanf(si.version_.c_str(), "%d.%d.%d", &v1, &v2, &v3) == 3)
    {
      //timeshift commands are supported in the latest v6 build or in v7 build (aka tv mosaic)
      server_caps_.timeshift_commands_supported_ = (v1 == 6 && server_build >= 14061) || v1 >= 7;

      //start timer any time flag is only supported in v6
      server_caps_.start_any_time_supported_ = (v1 == 6);
    }
  }

  GetStreamingCapabilitiesRequest streamin_caps_request;
  StreamingCapabilities streaming_caps;

  status = srv_connection.get_connection()->GetStreamingCapabilities(streamin_caps_request,
                                                                     streaming_caps, nullptr);
  if (status == DVBLINK_REMOTE_STATUS_OK)
  {
    server_caps_.transcoding_supported_ = streaming_caps.IsTranscoderSupported(
        dvblinkremote::StreamingCapabilities::STREAMING_TRANSCODER_H264);
    server_caps_.recordings_supported_ = streaming_caps.SupportsRecording;
    server_caps_.timeshifting_supported_ = streaming_caps.SupportsTimeShifting;
    server_caps_.device_management_supported_ = streaming_caps.SupportsDeviceManagement;
  }

  GetFavoritesRequest favorites_request;
  status =
      srv_connection.get_connection()->GetFavorites(favorites_request, channel_favorites_, nullptr);
  server_caps_.favorites_supported_ = (status == DVBLINK_REMOTE_STATUS_OK);
}

DVBLinkClient::DVBLinkClient(const CDVBLinkAddon& base,
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
                             int default_rec_show_type)
  : kodi::addon::CInstancePVRClient(instance),
    connection_props_(hostname, port, username, password, clientname),
    m_base(base)
{
  m_connected = false;
  m_currentChannelId = 0;
  m_showinfomsg = showinfomsg;
  m_add_episode_to_rec_title = add_episode_to_rec_title;
  m_group_recordings_by_series = group_recordings_by_series;
  no_group_single_rec_ = no_group_single_rec;
  default_rec_show_type_ = default_rec_show_type;
  timer_idx_seed_ = PVR_TIMER_NO_CLIENT_INDEX + 10; //arbitrary seed number, greater than

  switch (default_update_interval)
  {
    case 0:
      default_update_interval_sec_ = UPDATE_INTERVAL_60_SEC;
      break;
    case 1:
      default_update_interval_sec_ = UPDATE_INTERVAL_120_SEC;
      break;
    case 2:
      default_update_interval_sec_ = UPDATE_INTERVAL_180_SEC;
      break;
    case 3:
      default_update_interval_sec_ = UPDATE_INTERVAL_240_SEC;
      break;
    case 4:
      default_update_interval_sec_ = UPDATE_INTERVAL_300_SEC;
      break;
    case 5:
      default_update_interval_sec_ = UPDATE_INTERVAL_360_SEC;
      break;
    case 6:
      default_update_interval_sec_ = UPDATE_INTERVAL_420_SEC;
      break;
    case 7:
      default_update_interval_sec_ = UPDATE_INTERVAL_480_SEC;
      break;
    case 8:
      default_update_interval_sec_ = UPDATE_INTERVAL_540_SEC;
      break;
    case 9:
      default_update_interval_sec_ = UPDATE_INTERVAL_600_SEC;
      break;
    case 10:
      default_update_interval_sec_ = UPDATE_INTERVAL_1200_SEC;
      break;
    case 11:
      default_update_interval_sec_ = UPDATE_INTERVAL_1800_SEC;
      break;
    case 12:
      default_update_interval_sec_ = UPDATE_INTERVAL_2400_SEC;
      break;
    case 13:
      default_update_interval_sec_ = UPDATE_INTERVAL_3000_SEC;
      break;
    case 14:
      default_update_interval_sec_ = UPDATE_INTERVAL_3600_SEC;
      break;
    default:
      default_update_interval_sec_ = UPDATE_INTERVAL_300_SEC;
      break;
  }

  get_server_caps();

  m_timerCount = -1;
  m_recordingCount = -1;

  GetChannelsRequest request;
  m_live_streamer = nullptr;

  std::string error;
  DVBLinkRemoteStatusCode status;

  dvblinkremote::ChannelList channels;
  dvblink_server_connection srv_connection(connection_props_);
  if ((status = srv_connection.get_connection()->GetChannels(request, channels, &error)) ==
      DVBLINK_REMOTE_STATUS_OK)
  {

    for (size_t i = 0; i < channels.size(); i++)
    {
      dvblinkremote::Channel* ch = channels[i];
      int idx = channel_id_start_seed_ + i;
      m_channels[idx] = new dvblinkremote::Channel(*ch);
      inverse_channel_map_[ch->GetID()] = idx;
    }

    m_connected = true;

    kodi::Log(ADDON_LOG_INFO, "Connected to DVBLink Server '%s'",
              connection_props_.address_.c_str());
    if (m_showinfomsg)
    {
      kodi::QueueFormattedNotification(QUEUE_INFO, kodi::addon::GetLocalizedString(32001).c_str(),
                                       connection_props_.address_.c_str());
      kodi::QueueFormattedNotification(QUEUE_INFO, kodi::addon::GetLocalizedString(32002).c_str(),
                                       m_channels.size());
    }

    if (server_caps_.recordings_supported_)
      m_recordingsid = GetBuildInRecorderObjectID();

    m_recordingsid_by_date = m_recordingsid;
    m_recordingsid_by_date.append(DVBLINK_RECODINGS_BY_DATA_ID);

    m_recordingsid_by_series = m_recordingsid;
    m_recordingsid_by_series.append(DVBLINK_RECODINGS_BY_SERIES_ID);

    m_running = true;
    m_thread = std::thread([&] { Process(); });
  }
  else
  {
    kodi::QueueFormattedNotification(QUEUE_ERROR, kodi::addon::GetLocalizedString(32003).c_str(),
                                     connection_props_.address_.c_str(), (int)status);
    kodi::Log(ADDON_LOG_ERROR,
              "Could not connect to DVBLink Server '%s' on port '%i' with username '%s' (Error "
              "code : %d Description : %s)",
              hostname.c_str(), port, username.c_str(), (int)status, error.c_str());
  }
}

PVR_ERROR DVBLinkClient::GetBackendName(std::string& name)
{
  name = "DVBLink Server";
  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR DVBLinkClient::GetBackendVersion(std::string& version)
{
  if (!m_connected)
    return PVR_ERROR_SERVER_ERROR;

  version = server_caps_.server_version_;
  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR DVBLinkClient::GetBackendHostname(std::string& hostname)
{
  hostname = connection_props_.address_;
  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR DVBLinkClient::GetConnectionString(std::string& connection)
{
  connection = connection_props_.address_;
  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR DVBLinkClient::GetCapabilities(kodi::addon::PVRCapabilities& capabilities)
{
  if (!m_connected)
    return PVR_ERROR_SERVER_ERROR;

  capabilities.SetSupportsEPG(true);
  capabilities.SetSupportsRecordings(server_caps_.recordings_supported_);
  capabilities.SetSupportsRecordingsDelete(server_caps_.recordings_supported_);
  capabilities.SetSupportsRecordingsUndelete(false);
  capabilities.SetSupportsTimers(server_caps_.recordings_supported_);
  capabilities.SetSupportsTV(true);
  capabilities.SetSupportsRadio(true);
  capabilities.SetHandlesInputStream(true);
  capabilities.SetSupportsChannelGroups(server_caps_.favorites_supported_);
  capabilities.SetSupportsRecordingsRename(false);
  capabilities.SetSupportsRecordingsLifetimeChange(false);
  capabilities.SetSupportsDescrambleInfo(false);
  capabilities.SetSupportsLastPlayedPosition(server_caps_.resume_supported_);
  return PVR_ERROR_NO_ERROR;
}

void DVBLinkClient::Process()
{
  kodi::Log(ADDON_LOG_DEBUG, "DVBLinkUpdateProcess:: thread started");

  time_t update_period_timers_sec = 5;
  time_t update_period_recordings_sec = 1;
  time_t now;
  time(&now);
  time_t next_update_time_timers = now + default_update_interval_sec_;
  time_t next_update_time_recordings = now + default_update_interval_sec_;

  while (m_running)
  {
    time(&now);

    if (m_update_timers_repeat)
    {
      next_update_time_timers = now - update_period_timers_sec;
    }
    else if (m_update_timers_now)
    {
      next_update_time_timers = now - update_period_timers_sec;
      m_update_timers_now = false;
    }

    if (now > next_update_time_timers)
    {
      kodi::addon::CInstancePVRClient::TriggerTimerUpdate();
      next_update_time_timers = now + default_update_interval_sec_;
    }

    if (m_update_timers_repeat)
    {
      next_update_time_timers = now + update_period_timers_sec;
      m_update_timers_repeat = false;
    }

    if (m_update_recordings)
    {
      next_update_time_recordings = now + update_period_recordings_sec;
      m_update_recordings = false;
    }

    if (now > next_update_time_recordings)
    {
      kodi::addon::CInstancePVRClient::TriggerRecordingUpdate();
      next_update_time_recordings = now + default_update_interval_sec_;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
  kodi::Log(ADDON_LOG_DEBUG, "DVBLinkUpdateProcess:: thread stopped");
}

bool DVBLinkClient::GetStatus()
{
  return m_connected;
}

PVR_ERROR DVBLinkClient::GetChannelsAmount(int& amount)
{
  if (!m_connected)
    return PVR_ERROR_SERVER_ERROR;

  amount = m_channels.size();
  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR DVBLinkClient::GetChannels(bool radio, kodi::addon::PVRChannelsResultSet& results)
{
  if (!m_connected)
    return PVR_ERROR_SERVER_ERROR;

  kodi::Log(ADDON_LOG_INFO, "Getting channels (%d channels on server)", m_channels.size());
  dvblink_channel_map_t::iterator ch_it = m_channels.begin();
  while (ch_it != m_channels.end())
  {
    Channel* channel = ch_it->second;

    bool isRadio = (channel->GetChannelType() == Channel::CHANNEL_TYPE_RADIO);

    if (isRadio == radio)
    {
      kodi::addon::PVRChannel xbmcChannel;
      xbmcChannel.SetIsRadio(isRadio);

      if (channel->Number > 0)
        xbmcChannel.SetChannelNumber(channel->Number);

      if (channel->SubNumber > 0)
        xbmcChannel.SetSubChannelNumber(channel->SubNumber);

      xbmcChannel.SetEncryptionSystem(0);
      xbmcChannel.SetUniqueId(ch_it->first);

      xbmcChannel.SetChannelName(channel->GetName());

      if (channel->GetLogoUrl().size() > 0)
        xbmcChannel.SetIconPath(channel->GetLogoUrl());

      results.Add(xbmcChannel);
    }

    ++ch_it;
  }
  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR DVBLinkClient::GetChannelGroupsAmount(int& amount)
{
  if (!server_caps_.favorites_supported_)
    return PVR_ERROR_NOT_IMPLEMENTED;

  amount = channel_favorites_.favorites_.size();
  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR DVBLinkClient::GetChannelGroups(bool radio,
                                          kodi::addon::PVRChannelGroupsResultSet& results)
{
  if (!server_caps_.favorites_supported_)
    return PVR_ERROR_NOT_IMPLEMENTED;

  for (size_t i = 0; i < channel_favorites_.favorites_.size(); i++)
  {
    kodi::addon::PVRChannelGroup group;
    group.SetIsRadio(radio);
    group.SetGroupName(channel_favorites_.favorites_[i].get_name());

    results.Add(group);
  }

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR DVBLinkClient::GetChannelGroupMembers(
    const kodi::addon::PVRChannelGroup& group,
    kodi::addon::PVRChannelGroupMembersResultSet& results)
{
  if (!server_caps_.favorites_supported_)
    return PVR_ERROR_NOT_IMPLEMENTED;

  for (size_t i = 0; i < channel_favorites_.favorites_.size(); i++)
  {
    if (channel_favorites_.favorites_[i].get_name() != group.GetGroupName())
      continue;

    dvblinkremote::ChannelFavorite::favorite_channel_list_t chlist =
        channel_favorites_.favorites_[i].get_channels();

    for (size_t j = 0; j < chlist.size(); j++)
    {
      if (inverse_channel_map_.find(chlist[j]) != inverse_channel_map_.end())
      {
        dvblinkremote::Channel* ch = m_channels[inverse_channel_map_[chlist[j]]];

        bool isRadio = (ch->GetChannelType() == dvblinkremote::Channel::CHANNEL_TYPE_RADIO);

        if (group.GetIsRadio() != isRadio)
          continue;

        kodi::addon::PVRChannelGroupMember member;
        member.SetGroupName(group.GetGroupName());
        member.SetChannelUniqueId(inverse_channel_map_[chlist[j]]);
        if (ch->Number > 0)
          member.SetChannelNumber(ch->Number);
        if (ch->SubNumber > 0)
          member.SetSubChannelNumber(ch->SubNumber);

        results.Add(member);
      }
    }
  }
  return PVR_ERROR_NO_ERROR;
}

namespace
{
struct TimerType : kodi::addon::PVRTimerType
{
  TimerType(unsigned int id,
            unsigned int attributes,
            std::string description,
            const std::vector<kodi::addon::PVRTypeIntValue>& maxRecordingsValues,
            int maxRecordingsDefault,
            const std::vector<kodi::addon::PVRTypeIntValue>& dupEpisodesValues,
            int dupEpisodesDefault)
  {
    SetId(id);
    SetAttributes(attributes);
    SetMaxRecordings(maxRecordingsValues);
    SetMaxRecordingsDefault(maxRecordingsDefault);
    SetPreventDuplicateEpisodes(dupEpisodesValues);
    SetPreventDuplicateEpisodesDefault(dupEpisodesDefault);
    SetDescription(description);
  }
};
} // namespace

PVR_ERROR DVBLinkClient::GetTimerTypes(std::vector<kodi::addon::PVRTimerType>& types)
{
  /* PVR_Timer.iMaxRecordings values and presentation. */
  static std::vector<kodi::addon::PVRTypeIntValue> recordingLimitValues;
  if (recordingLimitValues.size() == 0)
  {
    recordingLimitValues.emplace_back(dcrn_keep_all, kodi::addon::GetLocalizedString(32026));
    recordingLimitValues.emplace_back(dcrn_keep_1, kodi::addon::GetLocalizedString(32027));
    recordingLimitValues.emplace_back(dcrn_keep_2, kodi::addon::GetLocalizedString(32028));
    recordingLimitValues.emplace_back(dcrn_keep_3, kodi::addon::GetLocalizedString(32029));
    recordingLimitValues.emplace_back(dcrn_keep_4, kodi::addon::GetLocalizedString(32030));
    recordingLimitValues.emplace_back(dcrn_keep_5, kodi::addon::GetLocalizedString(32031));
    recordingLimitValues.emplace_back(dcrn_keep_6, kodi::addon::GetLocalizedString(32032));
    recordingLimitValues.emplace_back(dcrn_keep_7, kodi::addon::GetLocalizedString(32033));
    recordingLimitValues.emplace_back(dcrn_keep_10, kodi::addon::GetLocalizedString(32034));
  }

  /* PVR_Timer.iPreventDuplicateEpisodes values and presentation.*/
  static std::vector<kodi::addon::PVRTypeIntValue> showTypeValues;
  if (showTypeValues.size() == 0)
  {
    showTypeValues.emplace_back(dcrs_record_all, kodi::addon::GetLocalizedString(32035));
    showTypeValues.emplace_back(dcrs_record_new_only, kodi::addon::GetLocalizedString(32036));
  }

  static std::vector<kodi::addon::PVRTypeIntValue> emptyList;

  static const unsigned int TIMER_MANUAL_ATTRIBS =
      PVR_TIMER_TYPE_IS_MANUAL | PVR_TIMER_TYPE_SUPPORTS_CHANNELS |
      PVR_TIMER_TYPE_SUPPORTS_START_TIME | PVR_TIMER_TYPE_SUPPORTS_END_TIME |
      PVR_TIMER_TYPE_SUPPORTS_START_END_MARGIN;

  static const unsigned int TIMER_EPG_ATTRIBS =
      PVR_TIMER_TYPE_REQUIRES_EPG_TAG_ON_CREATE | PVR_TIMER_TYPE_SUPPORTS_START_END_MARGIN;

  static const unsigned int TIMER_REPEATING_MANUAL_ATTRIBS = PVR_TIMER_TYPE_IS_REPEATING |
                                                             PVR_TIMER_TYPE_SUPPORTS_WEEKDAYS |
                                                             PVR_TIMER_TYPE_SUPPORTS_MAX_RECORDINGS;

  static const unsigned int TIMER_REPEATING_EPG_ATTRIBS =
      PVR_TIMER_TYPE_IS_REPEATING | PVR_TIMER_TYPE_SUPPORTS_RECORD_ONLY_NEW_EPISODES |
      PVR_TIMER_TYPE_SUPPORTS_MAX_RECORDINGS;

  static const unsigned int TIMER_REPEATING_KEYWORD_ATTRIBS =
      PVR_TIMER_TYPE_SUPPORTS_TITLE_EPG_MATCH | PVR_TIMER_TYPE_SUPPORTS_CHANNELS |
      PVR_TIMER_TYPE_SUPPORTS_START_END_MARGIN | PVR_TIMER_TYPE_IS_REPEATING |
      PVR_TIMER_TYPE_SUPPORTS_MAX_RECORDINGS;

  static const unsigned int TIMER_MANUAL_CHILD_ATTRIBUTES =
      PVR_TIMER_TYPE_IS_MANUAL | PVR_TIMER_TYPE_FORBIDS_NEW_INSTANCES;

  static const unsigned int TIMER_EPG_CHILD_ATTRIBUTES =
      PVR_TIMER_TYPE_REQUIRES_EPG_TAG_ON_CREATE | PVR_TIMER_TYPE_FORBIDS_NEW_INSTANCES;

  static const unsigned int TIMER_KEYWORD_CHILD_ATTRIBUTES = PVR_TIMER_TYPE_FORBIDS_NEW_INSTANCES;

  /* Timer types definition.*/
  static std::vector<std::unique_ptr<TimerType>> timerTypes;
  if (timerTypes.size() == 0)
  {
    timerTypes.push_back(
        /* One-shot manual (time and channel based) */
        std::unique_ptr<TimerType>(new TimerType(
            /* Type id. */
            TIMER_ONCE_MANUAL,
            /* Attributes. */
            TIMER_MANUAL_ATTRIBS,
            /* Description. */
            kodi::addon::GetLocalizedString(32037),
            /* Values definitions for attributes. */
            recordingLimitValues, default_rec_limit_, showTypeValues, default_rec_show_type_)));

    timerTypes.push_back(
        /* One-shot epg based */
        std::unique_ptr<TimerType>(new TimerType(
            /* Type id. */
            TIMER_ONCE_EPG,
            /* Attributes. */
            TIMER_EPG_ATTRIBS,
            /* Description. */
            kodi::addon::GetLocalizedString(32038),
            /* Values definitions for attributes. */
            recordingLimitValues, default_rec_limit_, showTypeValues, default_rec_show_type_)));

    timerTypes.push_back(
        /* Read-only one-shot for timers generated by timerec */
        std::unique_ptr<TimerType>(new TimerType(
            /* Type id. */
            TIMER_ONCE_MANUAL_CHILD,
            /* Attributes. */
            TIMER_MANUAL_CHILD_ATTRIBUTES,
            /* Description. */
            kodi::addon::GetLocalizedString(32039),
            /* Values definitions for attributes. */
            recordingLimitValues, default_rec_limit_, showTypeValues, default_rec_show_type_)));

    timerTypes.push_back(
        /* Read-only one-shot for timers generated by autorec */
        std::unique_ptr<TimerType>(new TimerType(
            /* Type id. */
            TIMER_ONCE_EPG_CHILD,
            /* Attributes. */
            TIMER_EPG_CHILD_ATTRIBUTES,
            /* Description. */
            kodi::addon::GetLocalizedString(32040),
            /* Values definitions for attributes. */
            recordingLimitValues, default_rec_limit_, showTypeValues, default_rec_show_type_)));

    timerTypes.push_back(
        /* Child Keyword based */
        std::unique_ptr<TimerType>(new TimerType(
            /* Type id. */
            TIMER_ONCE_KEYWORD_CHILD,
            /* Attributes. */
            TIMER_KEYWORD_CHILD_ATTRIBUTES,
            /* Description. */
            kodi::addon::GetLocalizedString(32041),
            /* Values definitions for attributes. */
            recordingLimitValues, default_rec_limit_, showTypeValues, default_rec_show_type_)));

    timerTypes.push_back(
        /* Repeating manual (time and channel based) Parent */
        std::unique_ptr<TimerType>(new TimerType(
            /* Type id. */
            TIMER_REPEATING_MANUAL,
            /* Attributes. */
            TIMER_MANUAL_ATTRIBS | TIMER_REPEATING_MANUAL_ATTRIBS,
            /* Description. */
            kodi::addon::GetLocalizedString(32042),
            /* Values definitions for attributes. */
            recordingLimitValues, default_rec_limit_, showTypeValues, default_rec_show_type_)));

    unsigned int repeating_epg_attrs = TIMER_EPG_ATTRIBS | TIMER_REPEATING_EPG_ATTRIBS;
    //start timer anytime is supported only in v6
    if (server_caps_.start_any_time_supported_)
      repeating_epg_attrs |= PVR_TIMER_TYPE_SUPPORTS_START_ANYTIME;

    timerTypes.push_back(
        /* Repeating epg based Parent*/
        std::unique_ptr<TimerType>(new TimerType(
            /* Type id. */
            TIMER_REPEATING_EPG,
            /* Attributes. */
            repeating_epg_attrs,
            /* Description. */
            kodi::addon::GetLocalizedString(32043),
            /* Values definitions for attributes. */
            recordingLimitValues, default_rec_limit_, showTypeValues, default_rec_show_type_)));

    timerTypes.push_back(
        /* Repeating Keyword (Generic) based */
        std::unique_ptr<TimerType>(new TimerType(
            /* Type id. */
            TIMER_REPEATING_KEYWORD,
            /* Attributes. */
            TIMER_REPEATING_KEYWORD_ATTRIBS,
            /* Description. */
            kodi::addon::GetLocalizedString(32044),
            /* Values definitions for attributes. */
            recordingLimitValues, default_rec_limit_, showTypeValues, default_rec_show_type_)));
  }

  /* Copy data to target array. */
  for (const auto& it : timerTypes)
    types.emplace_back(*it);

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR DVBLinkClient::GetTimersAmount(int& amount)
{
  amount = m_timerCount;
  return PVR_ERROR_NO_ERROR;
}

int DVBLinkClient::GetInternalUniqueIdFromChannelId(const std::string& channelId)
{
  dvblink_channel_map_t::iterator ch_it = m_channels.begin();
  while (ch_it != m_channels.end())
  {
    Channel* channel = ch_it->second;
    int id = ch_it->first;

    if (channelId.compare(channel->GetID()) == 0)
    {
      return id;
    }
    ++ch_it;
  }
  return 0;
}

std::string DVBLinkClient::make_timer_hash(const std::string& timer_id,
                                           const std::string& schedule_id)
{
  std::string res = schedule_id + "#" + timer_id;
  return res;
}

bool DVBLinkClient::parse_timer_hash(const char* timer_hash,
                                     std::string& timer_id,
                                     std::string& schedule_id)
{
  bool ret_val = false;

  std::string timer = timer_hash;
  size_t pos = timer.find('#');
  if (pos != std::string::npos)
  {
    timer_id = timer.c_str() + pos + 1;
    schedule_id = timer.substr(0, pos);
    ret_val = true;
  }

  return ret_val;
}

unsigned int DVBLinkClient::get_kodi_timer_idx_from_dvblink(const std::string& id)
{
  std::lock_guard<std::mutex> critsec(m_mutex);

  if (timer_idx_map_.find(id) == timer_idx_map_.end())
    timer_idx_map_[id] = timer_idx_seed_++;

  return timer_idx_map_[id];
}

void DVBLinkClient::add_schedule_desc(const std::string& id, const schedule_desc& sd)
{
  std::lock_guard<std::mutex> critsec(m_mutex);

  schedule_map_[id] = sd;
}

bool DVBLinkClient::get_schedule_desc(const std::string& id, schedule_desc& sd)
{
  std::lock_guard<std::mutex> critsec(m_mutex);

  if (schedule_map_.find(id) != schedule_map_.end())
  {
    sd = schedule_map_[id];
    return true;
  }

  return false;
}

int DVBLinkClient::GetSchedules(kodi::addon::PVRTimersResultSet& results,
                                const RecordingList& recordings)
{
  //make a map of schedule id->recording list
  std::map<std::string, std::vector<Recording*>> schedule_to_timer_map;
  for (size_t i = 0; i < recordings.size(); i++)
  {
    Recording* rec = recordings[i];
    if (schedule_to_timer_map.find(rec->GetScheduleID()) == schedule_to_timer_map.end())
      schedule_to_timer_map[rec->GetScheduleID()] = std::vector<Recording*>();

    schedule_to_timer_map[rec->GetScheduleID()].push_back(rec);
  }

  int added_count = 0;
  int total_count = 0;

  {
    std::lock_guard<std::mutex> critsec(m_mutex);

    schedule_map_.clear();
  }

  GetSchedulesRequest request;
  StoredSchedules response;

  DVBLinkRemoteStatusCode status;
  std::string error;

  dvblink_server_connection srv_connection(connection_props_);
  if ((status = srv_connection.get_connection()->GetSchedules(request, response, &error)) !=
      DVBLINK_REMOTE_STATUS_OK)
  {
    kodi::Log(ADDON_LOG_ERROR, "Could not get Schedules (Error code : %d Description : %s)",
              (int)status, error.c_str());
    return added_count;
  }

  int schedule_num = response.GetManualSchedules().size() + response.GetEpgSchedules().size();

  kodi::Log(ADDON_LOG_INFO, "Found %d schedules", schedule_num);

  if (m_showinfomsg)
  {
    kodi::QueueFormattedNotification(QUEUE_INFO, kodi::addon::GetLocalizedString(32007).c_str(),
                                     schedule_num);
  }

  //manual schedules
  StoredManualScheduleList& manual_schedules = response.GetManualSchedules();
  for (size_t i = 0; i < manual_schedules.size(); i++)
  {
    add_schedule_desc(manual_schedules[i]->GetID(),
                      schedule_desc(-1, TIMER_ONCE_MANUAL, manual_schedules[i]->MarginBefore,
                                    manual_schedules[i]->MarginAfter));

    if (manual_schedules[i]->GetDayMask() != 0)
    {
      unsigned int kodi_idx = get_kodi_timer_idx_from_dvblink(manual_schedules[i]->GetID());

      add_schedule_desc(manual_schedules[i]->GetID(),
                        schedule_desc(kodi_idx, TIMER_REPEATING_MANUAL,
                                      manual_schedules[i]->MarginBefore,
                                      manual_schedules[i]->MarginAfter));

      kodi::addon::PVRTimer timer;
      timer.SetEPGUid(PVR_TIMER_NO_EPG_UID);

      //misuse strDirectory to keep id of the timer
      timer.SetDirectory(manual_schedules[i]->GetID());
      timer.SetClientIndex(kodi_idx);
      timer.SetClientChannelUid(
          GetInternalUniqueIdFromChannelId(manual_schedules[i]->GetChannelID()));
      timer.SetState(PVR_TIMER_STATE_SCHEDULED);
      timer.SetTimerType(TIMER_REPEATING_MANUAL);
      timer.SetMarginStart(manual_schedules[i]->MarginBefore / 60);
      timer.SetMarginEnd(manual_schedules[i]->MarginAfter / 60);
      timer.SetMaxRecordings(manual_schedules[i]->RecordingsToKeep);

      timer.SetTitle(manual_schedules[i]->Title);
      timer.SetStartTime(manual_schedules[i]->GetStartTime());
      timer.SetEndTime(manual_schedules[i]->GetStartTime() + manual_schedules[i]->GetDuration());

      //change day mask from DVBLink server (Sun - first day) to Kodi format
      long day_mask = manual_schedules[i]->GetDayMask();
      bool bcarry = (day_mask & 0x01) == 0x01;
      int weekdays = (day_mask >> 1);
      if (bcarry)
        weekdays |= 0x40;
      timer.SetWeekdays(weekdays);

      results.Add(timer);
      kodi::Log(ADDON_LOG_INFO, "Added EPG schedule : %s", manual_schedules[i]->GetID().c_str());

      added_count += 1;
    }
    total_count += 1;
  }

  //epg based schedules
  StoredEpgScheduleList& epg_schedules = response.GetEpgSchedules();
  for (size_t i = 0; i < epg_schedules.size(); i++)
  {
    add_schedule_desc(epg_schedules[i]->GetID(),
                      schedule_desc(-1, TIMER_ONCE_EPG, epg_schedules[i]->MarginBefore,
                                    epg_schedules[i]->MarginAfter));

    if (epg_schedules[i]->Repeat)
    {
      unsigned int kodi_idx = get_kodi_timer_idx_from_dvblink(epg_schedules[i]->GetID());
      add_schedule_desc(epg_schedules[i]->GetID(),
                        schedule_desc(kodi_idx, TIMER_REPEATING_EPG, epg_schedules[i]->MarginBefore,
                                      epg_schedules[i]->MarginAfter));

      kodi::addon::PVRTimer timer;

      //misuse strDirectory to keep id of the timer
      timer.SetDirectory(epg_schedules[i]->GetID());
      timer.SetClientIndex(kodi_idx);
      timer.SetClientChannelUid(GetInternalUniqueIdFromChannelId(epg_schedules[i]->GetChannelID()));
      timer.SetState(PVR_TIMER_STATE_SCHEDULED);
      timer.SetTimerType(TIMER_REPEATING_EPG);
      timer.SetMarginStart(epg_schedules[i]->MarginBefore / 60);
      timer.SetMarginEnd(epg_schedules[i]->MarginAfter / 60);

      timer.SetMaxRecordings(epg_schedules[i]->RecordingsToKeep);
      timer.SetStartAnyTime(
          server_caps_.start_any_time_supported_ ? epg_schedules[i]->RecordSeriesAnytime : true);
      timer.SetPreventDuplicateEpisodes(epg_schedules[i]->NewOnly ? dcrs_record_new_only
                                                                  : dcrs_record_all);
      timer.SetTitle(epg_schedules[i]->program_name_);

      if (schedule_to_timer_map.find(epg_schedules[i]->GetID()) != schedule_to_timer_map.end() &&
          !schedule_to_timer_map[epg_schedules[i]->GetID()].empty())
      {
        timer.SetStartTime(
            schedule_to_timer_map[epg_schedules[i]->GetID()].at(0)->GetProgram().GetStartTime());
        timer.SetEndTime(
            timer.GetStartTime() +
            schedule_to_timer_map[epg_schedules[i]->GetID()].at(0)->GetProgram().GetDuration());
      }

      //the original program, used for scheduling, can already be gone for a long time
      timer.SetEPGUid(PVR_TIMER_NO_EPG_UID);

      results.Add(timer);
      kodi::Log(ADDON_LOG_INFO, "Added EPG schedule : %s", epg_schedules[i]->GetID().c_str());

      added_count += 1;
    }
    total_count += 1;
  }

  //epg based schedules
  StoredByPatternScheduleList& bp_schedules = response.GetByPatternSchedules();
  for (size_t i = 0; i < bp_schedules.size(); i++)
  {
    unsigned int kodi_idx = get_kodi_timer_idx_from_dvblink(bp_schedules[i]->GetID());
    add_schedule_desc(bp_schedules[i]->GetID(),
                      schedule_desc(kodi_idx, TIMER_REPEATING_KEYWORD,
                                    bp_schedules[i]->MarginBefore, bp_schedules[i]->MarginAfter));

    kodi::addon::PVRTimer timer;

    //misuse strDirectory to keep id of the timer
    timer.SetDirectory(bp_schedules[i]->GetID());
    timer.SetClientIndex(kodi_idx);
    if (!bp_schedules[i]->GetChannelID().empty())
      timer.SetClientChannelUid(GetInternalUniqueIdFromChannelId(bp_schedules[i]->GetChannelID()));
    else
      timer.SetClientChannelUid(PVR_TIMER_ANY_CHANNEL);

    timer.SetState(PVR_TIMER_STATE_SCHEDULED);
    timer.SetTimerType(TIMER_REPEATING_KEYWORD);
    timer.SetMarginStart(bp_schedules[i]->MarginBefore / 60);
    timer.SetMarginEnd(bp_schedules[i]->MarginAfter / 60);
    timer.SetEPGSearchString(bp_schedules[i]->GetKeyphrase());

    if (schedule_to_timer_map.find(bp_schedules[i]->GetID()) != schedule_to_timer_map.end() &&
        !schedule_to_timer_map[bp_schedules[i]->GetID()].empty())
    {
      timer.SetStartTime(
          schedule_to_timer_map[bp_schedules[i]->GetID()].at(0)->GetProgram().GetStartTime());
      timer.SetEndTime(
          timer.GetStartTime() +
          schedule_to_timer_map[bp_schedules[i]->GetID()].at(0)->GetProgram().GetDuration());
    }

    timer.SetTitle(bp_schedules[i]->GetKeyphrase());
    timer.SetEPGUid(PVR_TIMER_NO_EPG_UID);

    results.Add(timer);
    kodi::Log(ADDON_LOG_INFO, "Added EPG schedule : %s", bp_schedules[i]->GetID().c_str());

    added_count += 1;
    total_count += 1;
  }

  return added_count;
}

PVR_ERROR DVBLinkClient::GetTimers(kodi::addon::PVRTimersResultSet& results)
{
  PVR_ERROR result = PVR_ERROR_FAILED;

  m_timerCount = 0;

  GetRecordingsRequest recordingsRequest;
  RecordingList recordings;

  DVBLinkRemoteStatusCode status;
  std::string error;

  dvblink_server_connection srv_connection(connection_props_);
  if ((status = srv_connection.get_connection()->GetRecordings(recordingsRequest, recordings,
                                                               &error)) != DVBLINK_REMOTE_STATUS_OK)
  {
    kodi::Log(ADDON_LOG_ERROR, "Could not get timers (Error code : %d Description : %s)",
              (int)status, error.c_str());
    return result;
  }

  kodi::Log(ADDON_LOG_INFO, "Found %d timers", recordings.size());

  if (m_showinfomsg)
  {
    kodi::QueueFormattedNotification(QUEUE_INFO, kodi::addon::GetLocalizedString(32007).c_str(),
                                     recordings.size());
  }

  //get and process schedules first
  int schedule_count = GetSchedules(results, recordings);
  int recording_count = 0;

  unsigned int index = PVR_TIMER_NO_CLIENT_INDEX + 1;
  for (size_t i = 0; i < recordings.size(); i++, index++)
  {
    Recording* rec = recordings[i];

    if (!rec->GetProgram().IsRecord)
      continue;

    kodi::addon::PVRTimer xbmcTimer;

    xbmcTimer.SetTimerType(PVR_TIMER_TYPE_NONE);
    //find parent schedule type
    schedule_desc sd;
    if (get_schedule_desc(rec->GetScheduleID(), sd))
    {
      int schedule_type = sd.schedule_kodi_type;
      switch (schedule_type)
      {
        case TIMER_ONCE_MANUAL:
        case TIMER_ONCE_EPG:
          //for once timers - copy parent attribute (there was no separate schedule submitted to kodi)
          xbmcTimer.SetTimerType(schedule_type);
          break;
        case TIMER_REPEATING_MANUAL:
          xbmcTimer.SetTimerType(TIMER_ONCE_MANUAL_CHILD);
          xbmcTimer.SetParentClientIndex(get_kodi_timer_idx_from_dvblink(rec->GetScheduleID()));
          break;
        case TIMER_REPEATING_EPG:
          xbmcTimer.SetTimerType(TIMER_ONCE_EPG_CHILD);
          xbmcTimer.SetParentClientIndex(get_kodi_timer_idx_from_dvblink(rec->GetScheduleID()));
          break;
        case TIMER_REPEATING_KEYWORD:
          xbmcTimer.SetTimerType(TIMER_ONCE_KEYWORD_CHILD);
          xbmcTimer.SetParentClientIndex(get_kodi_timer_idx_from_dvblink(rec->GetScheduleID()));
          break;
      }
      //copy margins
      xbmcTimer.SetMarginStart(sd.schedule_margin_before / 60);
      xbmcTimer.SetMarginEnd(sd.schedule_margin_after / 60);
    }

    xbmcTimer.SetClientIndex(get_kodi_timer_idx_from_dvblink(rec->GetID()));
    //misuse strDirectory to keep id of the timer
    std::string timer_hash = make_timer_hash(rec->GetID(), rec->GetScheduleID());
    xbmcTimer.SetDirectory(timer_hash);

    xbmcTimer.SetClientChannelUid(GetInternalUniqueIdFromChannelId(rec->GetChannelID()));
    xbmcTimer.SetState(PVR_TIMER_STATE_SCHEDULED);
    if (rec->IsActive)
      xbmcTimer.SetState(PVR_TIMER_STATE_RECORDING);
    if (rec->IsConflict)
      xbmcTimer.SetState(PVR_TIMER_STATE_CONFLICT_NOK);
    if (!rec->GetProgram().IsRecord)
      xbmcTimer.SetState(PVR_TIMER_STATE_CANCELLED);

    xbmcTimer.SetEPGUid(rec->GetProgram().GetStartTime());

    xbmcTimer.SetStartTime(rec->GetProgram().GetStartTime());
    xbmcTimer.SetEndTime(rec->GetProgram().GetStartTime() + rec->GetProgram().GetDuration());
    xbmcTimer.SetTitle(rec->GetProgram().GetTitle());
    xbmcTimer.SetSummary(rec->GetProgram().ShortDescription);

    int genre_type, genre_subtype;
    SetEPGGenre(rec->GetProgram(), genre_type, genre_subtype);
    if (genre_type == EPG_GENRE_USE_STRING)
    {
      xbmcTimer.SetGenreType(EPG_EVENT_CONTENTMASK_UNDEFINED);
    }
    else
    {
      xbmcTimer.SetGenreType(genre_type);
      xbmcTimer.SetGenreSubType(genre_subtype);
    }

    results.Add(xbmcTimer);
    recording_count += 1;
    kodi::Log(ADDON_LOG_INFO, "Added EPG timer : %s", rec->GetProgram().GetTitle().c_str());
  }

  m_timerCount = recording_count + schedule_count;

  result = PVR_ERROR_NO_ERROR;
  return result;
}

bool DVBLinkClient::get_dvblink_program_id(std::string& channelId,
                                           int start_time,
                                           std::string& dvblink_program_id)
{
  bool ret_val = false;

  EpgSearchResult epgSearchResult;
  if (DoEPGSearch(epgSearchResult, channelId, start_time, start_time))
  {
    if (epgSearchResult.size() > 0 && epgSearchResult.at(0)->GetEpgData().size() > 0)
    {
      dvblink_program_id = epgSearchResult.at(0)->GetEpgData().at(0)->GetID();
      ret_val = true;
    }
  }

  return ret_val;
}

static bool is_bit_set(int bit_num, unsigned char bit_field)
{
  unsigned char mask = (0x01 << bit_num);
  return (bit_field & mask) != 0;
}

PVR_ERROR DVBLinkClient::AddTimer(const kodi::addon::PVRTimer& timer)
{
  PVR_ERROR result = PVR_ERROR_FAILED;

  DVBLinkRemoteStatusCode status;
  AddScheduleRequest* addScheduleRequest = nullptr;

  int marginBefore = -1;
  int marginAfter = -1;
  if (server_caps_.setting_margins_supported_)
  {
    marginBefore = timer.GetMarginStart() * 60;
    marginAfter = timer.GetMarginEnd() * 60;
  }

  int numberToKeep = timer.GetMaxRecordings();
  if (numberToKeep < 0)
    numberToKeep = dcrn_keep_all;

  switch (timer.GetTimerType())
  {
    case TIMER_ONCE_MANUAL:
    {
      std::string channelId = m_channels[timer.GetClientChannelUid()]->GetID();
      time_t start_time = timer.GetStartTime();
      //check for instant recording
      if (start_time == 0)
        time(&start_time);

      time_t duration = timer.GetEndTime() - start_time;
      long day_mask = 0;
      addScheduleRequest =
          new AddManualScheduleRequest(channelId, start_time, duration, day_mask, timer.GetTitle(),
                                       0, marginBefore, marginAfter);
    }
    break;
    case TIMER_REPEATING_MANUAL:
    {
      std::string channelId = m_channels[timer.GetClientChannelUid()]->GetID();
      time_t start_time = timer.GetStartTime();
      time_t duration = timer.GetEndTime() - timer.GetStartTime();
      long day_mask = 0;
      if (timer.GetWeekdays() > 0) // repeating timer?
      {
        //change day mask to DVBLink server format (Sun - first day)
        bool bcarry = (timer.GetWeekdays() & 0x40) == 0x40;
        day_mask = (timer.GetWeekdays() << 1) & 0x7F;
        if (bcarry)
          day_mask |= 0x01;
        //find first now/future time, which matches the day mask
        start_time =
            timer.GetStartTime() > timer.GetFirstDay() ? timer.GetStartTime() : timer.GetFirstDay();
        for (size_t i = 0; i < 7; i++)
        {
          tm* local_start_time = localtime(&start_time);
          if (is_bit_set(local_start_time->tm_wday, (unsigned char)day_mask))
            break;
          start_time += time_t(24 * 3600);
        }
      }
      addScheduleRequest =
          new AddManualScheduleRequest(channelId, start_time, duration, day_mask, timer.GetTitle(),
                                       numberToKeep, marginBefore, marginAfter);
    }
    break;
    case TIMER_ONCE_EPG:
    {
      std::string channelId = m_channels[timer.GetClientChannelUid()]->GetID();

      std::string dvblink_program_id;
      if (get_dvblink_program_id(channelId, timer.GetEPGUid(), dvblink_program_id))
      {
        addScheduleRequest =
            new AddScheduleByEpgRequest(channelId, dvblink_program_id, false, true, true,
                                        dcrn_keep_all, marginBefore, marginAfter);
      }
    }
    break;
    case TIMER_REPEATING_EPG:
    {
      std::string channelId = m_channels[timer.GetClientChannelUid()]->GetID();
      bool record_series = true;
      bool newOnly = timer.GetPreventDuplicateEpisodes() == dcrs_record_all ? false : true;
      bool anytime = server_caps_.start_any_time_supported_ ? timer.GetStartAnyTime() : true;

      std::string dvblink_program_id;
      if (get_dvblink_program_id(channelId, timer.GetEPGUid(), dvblink_program_id))
      {
        addScheduleRequest =
            new AddScheduleByEpgRequest(channelId, dvblink_program_id, record_series, newOnly,
                                        anytime, numberToKeep, marginBefore, marginAfter);
      }
    }
    break;
    case TIMER_REPEATING_KEYWORD:
    {
      //empty string is "any channel"
      std::string channelId;
      if (timer.GetClientChannelUid() != PVR_TIMER_ANY_CHANNEL)
        channelId = m_channels[timer.GetClientChannelUid()]->GetID();

      std::string key_phrase = timer.GetEPGSearchString();
      long genre_mask = 0; //any genre

      addScheduleRequest = new AddScheduleByPatternRequest(channelId, key_phrase, genre_mask,
                                                           numberToKeep, marginBefore, marginAfter);
    }
    break;
  }

  if (addScheduleRequest != nullptr)
  {
    std::string error;

    dvblink_server_connection srv_connection(connection_props_);
    if ((status = srv_connection.get_connection()->AddSchedule(*addScheduleRequest, &error)) ==
        DVBLINK_REMOTE_STATUS_OK)
    {
      kodi::Log(ADDON_LOG_INFO, "Timer added");
      m_update_timers_repeat = true;
      result = PVR_ERROR_NO_ERROR;
    }
    else
    {
      result = PVR_ERROR_FAILED;
      kodi::Log(ADDON_LOG_ERROR, "Could not add timer (Error code : %d Description : %s)",
                (int)status, error.c_str());
    }
    SafeDelete(addScheduleRequest);
  }
  else
  {
    result = PVR_ERROR_FAILED;
  }
  return result;
}

PVR_ERROR DVBLinkClient::DeleteTimer(const kodi::addon::PVRTimer& timer, bool forceDelete)
{
  PVR_ERROR result = PVR_ERROR_FAILED;

  DVBLinkRemoteStatusCode status = DVBLINK_REMOTE_STATUS_ERROR;
  std::string error;
  dvblink_server_connection srv_connection(connection_props_);

  switch (timer.GetTimerType())
  {
    case TIMER_ONCE_MANUAL:
    case TIMER_ONCE_EPG:
    case TIMER_ONCE_MANUAL_CHILD:
    case TIMER_ONCE_EPG_CHILD:
    case TIMER_ONCE_KEYWORD_CHILD:
    {
      //this is a timer

      //timer id hash is kept in strDirectory!
      std::string timer_id;
      std::string schedule_id;
      parse_timer_hash(timer.GetDirectory().c_str(), timer_id, schedule_id);

      RemoveRecordingRequest removeRecording(timer_id);
      status = srv_connection.get_connection()->RemoveRecording(removeRecording, &error);
    }
    break;
    case TIMER_REPEATING_MANUAL:
    case TIMER_REPEATING_EPG:
    case TIMER_REPEATING_KEYWORD:
    {
      //this is a schedule

      //schedule id is in the timer.strDirectory
      std::string schedule_id = timer.GetDirectory();
      RemoveScheduleRequest removeSchedule(schedule_id);
      status = srv_connection.get_connection()->RemoveSchedule(removeSchedule, &error);
    }
    break;
  }

  if (status == DVBLINK_REMOTE_STATUS_OK)
  {
    kodi::Log(ADDON_LOG_INFO, "Timer(s) deleted");
    m_update_timers_now = true;
    result = PVR_ERROR_NO_ERROR;
  }
  else
  {
    kodi::Log(ADDON_LOG_ERROR, "Timer could not be deleted (Error code : %d Description : %s)",
              (int)status, error.c_str());
  }
  return result;
}

PVR_ERROR DVBLinkClient::UpdateTimer(const kodi::addon::PVRTimer& timer)
{
  PVR_ERROR result = PVR_ERROR_NO_ERROR;

  std::string schedule_id;
  switch (timer.GetTimerType())
  {
    case TIMER_ONCE_MANUAL:
    case TIMER_ONCE_EPG:
    {
      //this is a timer

      //timer id hash is kept in strDirectory!
      std::string timer_id;
      parse_timer_hash(timer.GetDirectory().c_str(), timer_id, schedule_id);
    }
    break;
    case TIMER_REPEATING_MANUAL:
    case TIMER_REPEATING_EPG:
    case TIMER_REPEATING_KEYWORD:
    {
      //this is a schedule

      //schedule id is in the timer.strDirectory
      schedule_id = timer.GetDirectory();
    }
    break;
    case TIMER_ONCE_MANUAL_CHILD:
    case TIMER_ONCE_EPG_CHILD:
    case TIMER_ONCE_KEYWORD_CHILD:
      //children entities are not editable
      break;
  }

  if (schedule_id.size() > 0)
  {
    //find original schedule and check its type
    schedule_desc sd;
    if (get_schedule_desc(schedule_id, sd))
    {
      int schedule_type = sd.schedule_kodi_type;

      //we do not support changing schedule type. Only its parameters
      if (timer.GetTimerType() == schedule_type)
      {
        bool new_only = timer.GetPreventDuplicateEpisodes() == dcrs_record_new_only;
        bool recordSeriesAnytime =
            server_caps_.start_any_time_supported_ ? timer.GetStartAnyTime() : true;
        int recordingsToKeep = timer.GetMaxRecordings();
        int margin_before = timer.GetMarginStart() * 60;
        int margin_after = timer.GetMarginEnd() * 60;

        UpdateScheduleRequest update_request(schedule_id, new_only, recordSeriesAnytime,
                                             recordingsToKeep, margin_before, margin_after);

        std::string error;
        dvblink_server_connection srv_connection(connection_props_);
        DVBLinkRemoteStatusCode status =
            srv_connection.get_connection()->UpdateSchedule(update_request, &error);

        if (status == DVBLINK_REMOTE_STATUS_OK)
        {
          kodi::Log(ADDON_LOG_INFO, "Schedule %s was updated", schedule_id.c_str());
          m_update_timers_now = true;
          result = PVR_ERROR_NO_ERROR;
        }
        else
        {
          kodi::Log(ADDON_LOG_ERROR, "Schedule %s update failed (Error code : %d Description : %s)",
                    schedule_id.c_str(), (int)status, error.c_str());
        }
      }
      else
      {
        kodi::Log(ADDON_LOG_ERROR, "Editing schedule type is not supported");
        result = PVR_ERROR_INVALID_PARAMETERS;
      }
    }
  }
  return result;
}

PVR_ERROR DVBLinkClient::GetRecordingsAmount(bool deleted, int& amount)
{
  amount = m_recordingCount;
  return PVR_ERROR_NO_ERROR;
}

std::string DVBLinkClient::GetRecordedTVByDateObjectID(const std::string& buildInRecoderObjectID)
{
  std::string result = "";
  DVBLinkRemoteStatusCode status;

  GetPlaybackObjectRequest getPlaybackObjectRequest(connection_props_.address_.c_str(),
                                                    buildInRecoderObjectID);
  getPlaybackObjectRequest.IncludeChildrenObjectsForRequestedObject = true;
  GetPlaybackObjectResponse getPlaybackObjectResponse;

  dvblink_server_connection srv_connection(connection_props_);
  if ((status = srv_connection.get_connection()->GetPlaybackObject(
           getPlaybackObjectRequest, getPlaybackObjectResponse, nullptr)) ==
      DVBLINK_REMOTE_STATUS_OK)
  {
    for (std::vector<PlaybackContainer*>::iterator it =
             getPlaybackObjectResponse.GetPlaybackContainers().begin();
         it < getPlaybackObjectResponse.GetPlaybackContainers().end(); it++)
    {
      PlaybackContainer* container = (PlaybackContainer*)*it;

      if (container->GetObjectID().find("F6F08949-2A07-4074-9E9D-423D877270BB") !=
          std::string::npos)
      {
        result = container->GetObjectID();
        break;
      }
    }
  }
  return result;
}

PVR_ERROR DVBLinkClient::DeleteRecording(const kodi::addon::PVRRecording& recording)
{
  PVR_ERROR result = PVR_ERROR_FAILED;
  DVBLinkRemoteStatusCode status;
  RemovePlaybackObjectRequest remoteObj(recording.GetRecordingId());

  std::string error;
  dvblink_server_connection srv_connection(connection_props_);
  if ((status = srv_connection.get_connection()->RemovePlaybackObject(remoteObj, &error)) !=
      DVBLINK_REMOTE_STATUS_OK)
  {
    kodi::Log(ADDON_LOG_ERROR,
              "Recording %s could not be deleted (Error code: %d Description : %s)",
              recording.GetTitle().c_str(), (int)status, error.c_str());
    return result;
  }

  kodi::Log(ADDON_LOG_INFO, "Recording %s deleted", recording.GetTitle().c_str());
  m_update_recordings = true;
  result = PVR_ERROR_NO_ERROR;
  return result;
}

static std::string get_subtitle(int season, int episode, const std::string& episode_name, int year)
{
  std::string se_str;
  char buf[1024];

  if (season > 0 || episode > 0)
  {
    se_str += "(";
    if (season > 0)
    {
      sprintf(buf, "S%02d", season);
      se_str += buf;
    }
    if (episode > 0)
    {
      sprintf(buf, "E%02d", episode);
      se_str += buf;
    }
    se_str += ")";
  }

  if (year > 0)
  {
    if (se_str.size() > 0)
      se_str += " ";

    se_str += "[";
    sprintf(buf, "%04d", year);
    se_str += buf;
    se_str += "]";
  }

  if (episode_name.size() > 0)
  {
    if (se_str.size() > 0)
      se_str += " - ";

    se_str += episode_name;
  }

  return se_str;
}

PVR_ERROR DVBLinkClient::GetRecordings(bool deleted, kodi::addon::PVRRecordingsResultSet& results)
{
  PVR_ERROR result = PVR_ERROR_FAILED;
  DVBLinkRemoteStatusCode status;

  {
    std::lock_guard<std::mutex> critsec(m_mutex);

    m_recording_id_to_url_map.clear();
  }

  GetPlaybackObjectRequest getPlaybackObjectRequest(connection_props_.address_.c_str(),
                                                    m_recordingsid_by_date);
  getPlaybackObjectRequest.IncludeChildrenObjectsForRequestedObject = true;
  GetPlaybackObjectResponse getPlaybackObjectResponse;

  std::string error;

  dvblink_server_connection srv_connection(connection_props_);
  if ((status = srv_connection.get_connection()->GetPlaybackObject(
           getPlaybackObjectRequest, getPlaybackObjectResponse, &error)) !=
      DVBLINK_REMOTE_STATUS_OK)
  {
    kodi::Log(ADDON_LOG_ERROR, "Could not get recordings (Error code : %d Description : %s)",
              (int)status, error.c_str());
    //kodi::QueueFormattedNotification(QUEUE_ERROR, kodi::addon::GetLocalizedString(32004).c_str(), (int)status);
    return result;
  }

  kodi::Log(ADDON_LOG_INFO, "Found %d recordings",
            getPlaybackObjectResponse.GetPlaybackItems().size());

  if (m_showinfomsg)
  {
    kodi::QueueFormattedNotification(QUEUE_INFO, kodi::addon::GetLocalizedString(32009).c_str(),
                                     getPlaybackObjectResponse.GetPlaybackItems().size());
  }

  std::map<std::string, int> schedule_to_num_map;
  if (no_group_single_rec_)
  {
    //build a map with scheule id -> number of recordings
    for (std::vector<PlaybackItem*>::iterator it =
             getPlaybackObjectResponse.GetPlaybackItems().begin();
         it < getPlaybackObjectResponse.GetPlaybackItems().end(); it++)
    {
      RecordedTvItem* tvitem = (RecordedTvItem*)*it;
      if (tvitem->ScheduleId.size() > 0 && tvitem->SeriesSchedule)
      {
        if (schedule_to_num_map.find(tvitem->ScheduleId) == schedule_to_num_map.end())
          schedule_to_num_map[tvitem->ScheduleId] = 0;

        schedule_to_num_map[tvitem->ScheduleId] = schedule_to_num_map[tvitem->ScheduleId] + 1;
      }
    }
  }

  for (std::vector<PlaybackItem*>::iterator it =
           getPlaybackObjectResponse.GetPlaybackItems().begin();
       it < getPlaybackObjectResponse.GetPlaybackItems().end(); it++)
  {
    RecordedTvItem* tvitem = (RecordedTvItem*)*it;
    kodi::addon::PVRRecording xbmcRecording;

    xbmcRecording.SetRecordingId(tvitem->GetObjectID());

    std::string title = tvitem->GetMetadata().GetTitle();
    if (m_add_episode_to_rec_title)
    {
      //form a title as "name - (SxxExx) subtitle" because XBMC does not display episode/season information almost anywhere
      std::string se_str =
          get_subtitle(tvitem->GetMetadata().SeasonNumber, tvitem->GetMetadata().EpisodeNumber,
                       tvitem->GetMetadata().SubTitle, (int)tvitem->GetMetadata().Year);
      if (se_str.size() > 0)
        title += " - " + se_str;
    }
    xbmcRecording.SetTitle(title);
    xbmcRecording.SetEpisodeName(tvitem->GetMetadata().SubTitle);
    if (tvitem->GetMetadata().SeasonNumber > 0)
      xbmcRecording.SetSeriesNumber(tvitem->GetMetadata().SeasonNumber);
    else
      xbmcRecording.SetSeriesNumber(-1);

    if (tvitem->GetMetadata().EpisodeNumber > 0)
      xbmcRecording.SetEpisodeNumber(tvitem->GetMetadata().EpisodeNumber);
    else
      xbmcRecording.SetEpisodeNumber(-1);
    xbmcRecording.SetYear(tvitem->GetMetadata().Year);

    xbmcRecording.SetRecordingTime(tvitem->GetMetadata().GetStartTime());
    xbmcRecording.SetPlot(tvitem->GetMetadata().ShortDescription);
    xbmcRecording.SetPlotOutline(tvitem->GetMetadata().SubTitle);
    {
      std::lock_guard<std::mutex> critsec(m_mutex);

      m_recording_id_to_url_map[xbmcRecording.GetRecordingId()] = tvitem->GetPlaybackUrl();
    }
    xbmcRecording.SetDuration(tvitem->GetMetadata().GetDuration());
    xbmcRecording.SetChannelName(tvitem->ChannelName);
    xbmcRecording.SetThumbnailPath(tvitem->GetThumbnailUrl());
    int genre_type, genre_subtype;
    SetEPGGenre(tvitem->GetMetadata(), genre_type, genre_subtype);
    if (genre_type == EPG_GENRE_USE_STRING)
    {
      xbmcRecording.SetGenreType(EPG_EVENT_CONTENTMASK_UNDEFINED);
    }
    else
    {
      xbmcRecording.SetGenreType(genre_type);
      xbmcRecording.SetGenreSubType(genre_subtype);
    }

    if (m_group_recordings_by_series)
    {
      if (tvitem->ScheduleId.size() > 0 && tvitem->SeriesSchedule &&
          tvitem->ScheduleName.size() > 0)
      {
        bool b = true;

        if (no_group_single_rec_ &&
            schedule_to_num_map.find(tvitem->ScheduleId) != schedule_to_num_map.end() &&
            schedule_to_num_map[tvitem->ScheduleId] < 2)
          b = false;

        if (b)
          xbmcRecording.SetDirectory(tvitem->ScheduleName);
      }
    }

    if (inverse_channel_map_.find(tvitem->ChannelID) != inverse_channel_map_.end())
    {
      int chid = inverse_channel_map_[tvitem->ChannelID];
      xbmcRecording.SetChannelUid(chid);

      dvblinkremote::Channel* ch = m_channels[chid];
      bool isRadio = (ch->GetChannelType() == dvblinkremote::Channel::CHANNEL_TYPE_RADIO);
      xbmcRecording.SetChannelType(isRadio ? PVR_RECORDING_CHANNEL_TYPE_RADIO
                                           : PVR_RECORDING_CHANNEL_TYPE_TV);
    }
    else
    {
      xbmcRecording.SetChannelUid(PVR_CHANNEL_INVALID_UID);
      xbmcRecording.SetChannelType(PVR_RECORDING_CHANNEL_TYPE_UNKNOWN);
    }

    results.Add(xbmcRecording);
  }
  m_recordingCount = getPlaybackObjectResponse.GetPlaybackItems().size();
  result = PVR_ERROR_NO_ERROR;
  return result;
}

bool DVBLinkClient::GetRecordingURL(const std::string& recording_id,
                                    std::string& url,
                                    bool use_transcoder,
                                    int width,
                                    int height,
                                    int bitrate,
                                    std::string audiotrack)
{
  //if transcoding is requested and no transcoder is supported return false
  if ((use_transcoder && !server_caps_.transcoding_supported_) ||
      (use_transcoder && !server_caps_.transcoding_recordings_supported_))
  {
    kodi::QueueNotification(QUEUE_ERROR, "", kodi::addon::GetLocalizedString(32024));
    return false;
  }

  {
    std::lock_guard<std::mutex>critsec(m_mutex);
    if (m_recording_id_to_url_map.find(recording_id) == m_recording_id_to_url_map.end())
      return false;

    url = m_recording_id_to_url_map[recording_id];
  }

  if (use_transcoder)
  {
    int w = width == 0 ? kodi::gui::GetScreenWidth() : width;
    int h = height == 0 ? kodi::gui::GetScreenHeight() : height;

    char buf[1024];
    sprintf(buf, "%s&transcoder=hls&client_id=%s&width=%d&height=%d&bitrate=%d", url.c_str(),
            connection_props_.client_id_.c_str(), w, h, bitrate);
    url = buf;

    if (audiotrack.size() > 0)
      url += "&lng=" + audiotrack;
  }

  return true;
}

PVR_ERROR DVBLinkClient::GetDriveSpace(uint64_t& total, uint64_t& used)
{
  GetRecordingSettingsRequest recordingsettingsrequest;
  total = 0;
  used = 0;
  RecordingSettings settings;
  DVBLinkRemoteStatusCode status;

  dvblink_server_connection srv_connection(connection_props_);
  if ((status = srv_connection.get_connection()->GetRecordingSettings(
           recordingsettingsrequest, settings, nullptr)) == DVBLINK_REMOTE_STATUS_OK)
  {
    total = settings.TotalSpace;
    used = (settings.TotalSpace - settings.AvailableSpace);
  }

  return PVR_ERROR_NO_ERROR;
}

bool DVBLinkClient::OpenLiveStream(const kodi::addon::PVRChannel& channel)
{
  bool use_timeshift = m_base.GetSettings().UseTimeshift();
  bool use_transcoder = m_base.GetSettings().UseTranscoding();
  int width = m_base.GetSettings().Width();
  int height = m_base.GetSettings().Height();
  int bitrate = m_base.GetSettings().Bitrate();
  const std::string& audiotrack = m_base.GetSettings().Audiotrack();
  bool ret_val = false;

  if (!is_valid_ch_idx(channel.GetUniqueId()))
    return false;

  //if transcoding is requested and no transcoder is supported return false
  if (use_transcoder && !server_caps_.transcoding_supported_)
  {
    kodi::QueueNotification(QUEUE_ERROR, "", kodi::addon::GetLocalizedString(32024));
    return false;
  }

  std::lock_guard<std::mutex> critsec(live_mutex_);

  if (m_live_streamer)
    SafeDelete(m_live_streamer);

  if (use_timeshift)
    m_live_streamer =
        new TimeShiftBuffer(connection_props_, server_caps_.timeshift_commands_supported_);
  else
    m_live_streamer = new LiveTVStreamer(connection_props_);

  //adjust transcoded height and width if needed
  int w = width == 0 ? kodi::gui::GetScreenWidth() : width;
  int h = height == 0 ? kodi::gui::GetScreenHeight() : height;

  Channel* c = m_channels[channel.GetUniqueId()];

  if (m_live_streamer->Start(c, use_transcoder, w, h, bitrate, audiotrack))
  {
    m_currentChannelId = channel.GetUniqueId();
    ret_val = true;
  }
  else
  {
    delete m_live_streamer;
    m_live_streamer = nullptr;
  }
  return ret_val;
}

int DVBLinkClient::ReadLiveStream(unsigned char* pBuffer, unsigned int iBufferSize)
{
  if (m_live_streamer)
    return m_live_streamer->ReadData(pBuffer, iBufferSize);
  return 0;
}

int64_t DVBLinkClient::SeekLiveStream(int64_t iPosition, int iWhence)
{
  if (m_live_streamer)
    return m_live_streamer->Seek(iPosition, iWhence);
  return 0;
}

bool DVBLinkClient::IsRealTimeStream()
{
  if (m_live_streamer)
    return m_live_streamer->IsLive();
  return false;
}

PVR_ERROR DVBLinkClient::GetStreamTimes(kodi::addon::PVRStreamTimes& stream_times)
{
  std::lock_guard<std::mutex> critsec(live_mutex_);

  if (m_live_streamer)
  {
    m_live_streamer->GetStreamTimes(stream_times);
    return PVR_ERROR_NO_ERROR;
  }
  if (m_recording_streamer)
  {
    m_recording_streamer->GetStreamTimes(stream_times);
    return PVR_ERROR_NO_ERROR;
  }
  return PVR_ERROR_SERVER_ERROR;
}

int64_t DVBLinkClient::LengthLiveStream()
{
  std::lock_guard<std::mutex> critsec(live_mutex_);

  if (m_live_streamer)
    return m_live_streamer->Length();
  return 0;
}

void DVBLinkClient::CloseLiveStream()
{
  std::lock_guard<std::mutex> critsec(live_mutex_);

  if (m_live_streamer != nullptr)
  {
    m_live_streamer->Stop();
    SafeDelete(m_live_streamer);
    m_live_streamer = nullptr;
  }
}

void DVBLinkClient::SetEPGGenre(dvblinkremote::ItemMetadata& metadata,
                                int& genre_type,
                                int& genre_subtype)
{
  genre_type = EPG_GENRE_USE_STRING;
  genre_subtype = 0x00;

  if (metadata.IsCatNews)
  {
    genre_type = EPG_EVENT_CONTENTMASK_NEWSCURRENTAFFAIRS;
    genre_subtype = 0x00;
  }

  if (metadata.IsCatDocumentary)
  {
    genre_type = EPG_EVENT_CONTENTMASK_NEWSCURRENTAFFAIRS;
    genre_subtype = 0x03;
  }

  if (metadata.IsCatEducational)
  {
    genre_type = EPG_EVENT_CONTENTMASK_EDUCATIONALSCIENCE;
  }

  if (metadata.IsCatSports)
  {
    genre_type = EPG_EVENT_CONTENTMASK_SPORTS;
  }

  if (metadata.IsCatMovie)
  {
    genre_type = EPG_EVENT_CONTENTMASK_MOVIEDRAMA;
    genre_subtype =
        metadata.IsCatThriller
            ? 0x01
            : metadata.IsCatScifi
                  ? 0x03
                  : metadata.IsCatHorror
                        ? 0x03
                        : metadata.IsCatComedy
                              ? 0x04
                              : metadata.IsCatSoap
                                    ? 0x05
                                    : metadata.IsCatRomance ? 0x06 : metadata.IsCatDrama ? 0x08 : 0;
  }

  if (metadata.IsCatKids)
  {
    genre_type = EPG_EVENT_CONTENTMASK_CHILDRENYOUTH;
  }

  if (metadata.IsCatMusic)
  {
    genre_type = EPG_EVENT_CONTENTMASK_MUSICBALLETDANCE;
  }

  if (metadata.IsCatSpecial)
  {
    genre_type = EPG_EVENT_CONTENTMASK_SPECIAL;
  }
}

bool DVBLinkClient::DoEPGSearch(EpgSearchResult& epgSearchResult,
                                const std::string& channelId,
                                const long startTime,
                                const long endTime,
                                const std::string& programId)
{
  EpgSearchRequest epgSearchRequest(channelId, startTime, endTime);
  if (programId.compare("") != 0)
  {
    epgSearchRequest.ProgramID = programId;
  }

  DVBLinkRemoteStatusCode status;

  dvblink_server_connection srv_connection(connection_props_);
  if ((status = srv_connection.get_connection()->SearchEpg(epgSearchRequest, epgSearchResult,
                                                           nullptr)) == DVBLINK_REMOTE_STATUS_OK)
  {
    return true;
  }
  return false;
}

bool DVBLinkClient::is_valid_ch_idx(int ch_idx)
{
  return m_channels.find(ch_idx) != m_channels.end();
}

PVR_ERROR DVBLinkClient::GetEPGForChannel(int channelUid,
                                          time_t start,
                                          time_t end,
                                          kodi::addon::PVREPGTagsResultSet& results)
{
  if (!m_connected)
    return PVR_ERROR_SERVER_ERROR;

  PVR_ERROR result = PVR_ERROR_FAILED;

  if (!is_valid_ch_idx(channelUid))
    return result;

  Channel* c = m_channels[channelUid];
  EpgSearchResult epgSearchResult;

  if (DoEPGSearch(epgSearchResult, c->GetID(), start, end))
  {
    for (std::vector<ChannelEpgData*>::iterator it = epgSearchResult.begin();
         it < epgSearchResult.end(); it++)
    {
      ChannelEpgData* channelEpgData = (ChannelEpgData*)*it;
      EpgData& epgData = channelEpgData->GetEpgData();
      for (std::vector<Program*>::iterator pIt = epgData.begin(); pIt < epgData.end(); pIt++)
      {
        Program* p = (Program*)*pIt;
        kodi::addon::PVREPGTag broadcast;

        broadcast.SetUniqueBroadcastId(p->GetStartTime());
        broadcast.SetTitle(p->GetTitle());
        broadcast.SetUniqueChannelId(channelUid);
        broadcast.SetStartTime(p->GetStartTime());
        broadcast.SetEndTime(p->GetStartTime() + p->GetDuration());
        broadcast.SetPlot(p->ShortDescription);
        broadcast.SetCast(p->Actors);
        broadcast.SetDirector(p->Directors);
        broadcast.SetWriter(p->Writers);
        broadcast.SetYear(p->Year);
        broadcast.SetIconPath(p->Image);
        broadcast.SetGenreType(0);
        broadcast.SetGenreSubType(0);
        broadcast.SetGenreDescription("");
        broadcast.SetFirstAired("");
        broadcast.SetParentalRating(0);
        broadcast.SetStarRating(p->Rating);
        broadcast.SetSeriesNumber(p->SeasonNumber);
        broadcast.SetEpisodeNumber(p->EpisodeNumber);
        broadcast.SetEpisodePartNumber(EPG_TAG_INVALID_SERIES_EPISODE);
        broadcast.SetEpisodeName(p->SubTitle);
        broadcast.SetIMDBNumber(""); // unused
        broadcast.SetOriginalTitle(""); // unused
        broadcast.SetPlotOutline("");

        int genre_type, genre_subtype;
        SetEPGGenre(*p, genre_type, genre_subtype);
        broadcast.SetGenreType(genre_type);
        if (genre_type == EPG_GENRE_USE_STRING)
          broadcast.SetGenreDescription(p->Keywords);
        else
          broadcast.SetGenreSubType(genre_subtype);

        broadcast.SetFlags(EPG_TAG_FLAG_UNDEFINED);

        results.Add(broadcast);
      }
    }
    result = PVR_ERROR_NO_ERROR;
  }
  else
  {
    kodi::Log(ADDON_LOG_INFO, "Not EPG data found for channel with id : %i", channelUid);
  }
  return result;
}

DVBLinkClient::~DVBLinkClient(void)
{
  m_running = false;
  if (m_thread.joinable())
    m_thread.join();  

  if (m_live_streamer)
  {
    m_live_streamer->Stop();
    SafeDelete(m_live_streamer);
  }

  dvblink_channel_map_t::iterator ch_it = m_channels.begin();
  while (ch_it != m_channels.end())
  {
    delete ch_it->second;
    ++ch_it;
  }
}

PVR_ERROR DVBLinkClient::GetRecordingLastPlayedPosition(const kodi::addon::PVRRecording& recording,
                                                        int& position)
{
  GetObjectResumeInfoRequest request(recording.GetRecordingId());
  ResumeInfo response;

  DVBLinkRemoteStatusCode status;
  dvblink_server_connection srv_connection(connection_props_);
  if ((status = srv_connection.get_connection()->GetObjectResumeInfo(request, response, nullptr)) ==
      DVBLINK_REMOTE_STATUS_OK)
  {
    position = response.m_positionSec;
    return PVR_ERROR_NO_ERROR;
  }
  return PVR_ERROR_SERVER_ERROR;
}

PVR_ERROR DVBLinkClient::SetRecordingLastPlayedPosition(const kodi::addon::PVRRecording& recording,
                                                        int lastplayedposition)
{
  SetObjectResumeInfoRequest request(recording.GetRecordingId(), lastplayedposition);

  DVBLinkRemoteStatusCode status;
  dvblink_server_connection srv_connection(connection_props_);
  if ((status = srv_connection.get_connection()->SetObjectResumeInfo(request, nullptr)) ==
      DVBLINK_REMOTE_STATUS_OK)
  {
    m_update_recordings = true;
    return PVR_ERROR_NO_ERROR;
  }
  return PVR_ERROR_SERVER_ERROR;
}

bool DVBLinkClient::OpenRecordedStream(const kodi::addon::PVRRecording& recording)
{
  //close previous stream to be sure
  CloseRecordedStream();

  bool use_timeshift = m_base.GetSettings().UseTimeshift();
  bool use_transcoder = m_base.GetSettings().UseTranscoding();
  int width = m_base.GetSettings().Width();
  int height = m_base.GetSettings().Height();
  int bitrate = m_base.GetSettings().Bitrate();
  const std::string& audiotrack = m_base.GetSettings().Audiotrack();

  bool ret_val = false;
  std::string url;
  if (GetRecordingURL(recording.GetRecordingId(), url, use_transcoder, width, height, bitrate,
                      audiotrack))
  {
    m_recording_streamer = new RecordingStreamer(
        connection_props_.client_id_, connection_props_.address_, connection_props_.port_,
        connection_props_.username_, connection_props_.password_);
    if (m_recording_streamer->OpenRecordedStream(recording.GetRecordingId(), url))
    {
      ret_val = true;
    }
    else
    {
      delete m_recording_streamer;
      m_recording_streamer = nullptr;
    }
  }
  return ret_val;
}

void DVBLinkClient::CloseRecordedStream()
{
  if (m_recording_streamer != nullptr)
  {
    m_recording_streamer->CloseRecordedStream();
    delete m_recording_streamer;
    m_recording_streamer = nullptr;
  }
}

int DVBLinkClient::ReadRecordedStream(unsigned char* pBuffer, unsigned int iBufferSize)
{
  if (m_recording_streamer != nullptr)
    return m_recording_streamer->ReadRecordedStream(pBuffer, iBufferSize);

  return -1;
}

int64_t DVBLinkClient::SeekRecordedStream(int64_t iPosition, int iWhence /* = SEEK_SET */)
{
  if (m_recording_streamer != nullptr)
    return m_recording_streamer->SeekRecordedStream(iPosition, iWhence);

  return -1;
}

int64_t DVBLinkClient::LengthRecordedStream()
{
  if (m_recording_streamer != nullptr)
    return m_recording_streamer->LengthRecordedStream();

  return -1;
}

bool DVBLinkClient::CanPauseStream()
{
  return m_recording_streamer != nullptr ||
         (m_live_streamer != nullptr && m_base.GetSettings().UseTimeshift());
}

bool DVBLinkClient::CanSeekStream()
{
  return m_recording_streamer != nullptr ||
         (m_live_streamer != nullptr && m_base.GetSettings().UseTimeshift());
}
