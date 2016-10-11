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
#include <memory>
#include "DVBLinkClient.h"
#include "p8-platform/util/StdString.h"
#include "libKODI_guilib.h"

using namespace dvblinkremote;
using namespace dvblinkremotehttp;
using namespace ADDON;

static int default_rec_limit_ = dcrn_keep_all;
static int default_rec_show_type_ = dcrs_record_all;

std::string DVBLinkClient::GetBuildInRecorderObjectID()
{
  std::string result = "";
  DVBLinkRemoteStatusCode status;
  GetPlaybackObjectRequest getPlaybackObjectRequest(m_hostname.c_str(), "");
  getPlaybackObjectRequest.RequestedObjectType = GetPlaybackObjectRequest::REQUESTED_OBJECT_TYPE_ALL;
  getPlaybackObjectRequest.RequestedItemType = GetPlaybackObjectRequest::REQUESTED_ITEM_TYPE_ALL;
  getPlaybackObjectRequest.IncludeChildrenObjectsForRequestedObject = true;
  GetPlaybackObjectResponse getPlaybackObjectResponse;
  if ((status = m_dvblinkRemoteCommunication->GetPlaybackObject(getPlaybackObjectRequest, getPlaybackObjectResponse,
      NULL)) == DVBLINK_REMOTE_STATUS_OK)
  {
    for (std::vector<PlaybackContainer*>::iterator it = getPlaybackObjectResponse.GetPlaybackContainers().begin();
        it < getPlaybackObjectResponse.GetPlaybackContainers().end(); it++)
    {
      PlaybackContainer * container = (PlaybackContainer *) *it;
      if (strcmp(container->SourceID.c_str(), DVBLINK_BUILD_IN_RECORDER_SOURCE_ID) == 0)
      {
        result = container->GetObjectID();
        break;
      }

    }
  }
  return result;
}

DVBLinkClient::DVBLinkClient(CHelper_libXBMC_addon* xbmc, CHelper_libXBMC_pvr* pvr, CHelper_libKODI_guilib* gui,
    std::string clientname, std::string hostname, long port, bool showinfomsg, std::string username,
    std::string password, bool add_episode_to_rec_title, bool group_recordings_by_series, bool no_group_single_rec)
{
  PVR = pvr;
  XBMC = xbmc;
  GUI = gui;
  m_clientname = clientname;
  m_hostname = hostname;
  m_connected = false;
  m_currentChannelId = 0;
  m_showinfomsg = showinfomsg;
  m_add_episode_to_rec_title = add_episode_to_rec_title;
  m_group_recordings_by_series = group_recordings_by_series;
  no_group_single_rec_ = no_group_single_rec;
  setting_margins_supported_ = false;
  favorites_supported_ = false;
  transcoding_supported_ = false;
  transcoding_recordings_supported_ = false;
  timer_idx_seed_ = PVR_TIMER_NO_CLIENT_INDEX + 10; //arbitrary seed number, greater than 

  m_httpClient = new HttpPostClient(XBMC, hostname, port, username, password);
  m_dvblinkRemoteCommunication = DVBLinkRemote::Connect((HttpClient&) *m_httpClient, m_hostname.c_str(), port,
      username.c_str(), password.c_str(), this);

  DVBLinkRemoteStatusCode status;
  m_timerCount = -1;
  m_recordingCount = -1;

  //get server version and build
  GetServerInfoRequest server_info_request;
  ServerInfo si;
  if ((status = m_dvblinkRemoteCommunication->GetServerInfo(server_info_request, si, NULL)) == DVBLINK_REMOTE_STATUS_OK)
  {
    int server_build = atoi(si.build_.c_str());

    //server with build earlier than 11410 does not support setting margins
    setting_margins_supported_ = (server_build >= 11405);

    //server with build earlier than 12700 does not support playing transcoded recordings
    transcoding_recordings_supported_ = (server_build >= 12700);
  }

  GetStreamingCapabilitiesRequest streamin_caps_request;
  StreamingCapabilities streaming_caps;
  status = m_dvblinkRemoteCommunication->GetStreamingCapabilities(streamin_caps_request, streaming_caps, NULL);
  if (status == DVBLINK_REMOTE_STATUS_OK)
    transcoding_supported_ = streaming_caps.IsTranscoderSupported(
        dvblinkremote::StreamingCapabilities::STREAMING_TRANSCODER_H264);

  GetFavoritesRequest favorites_request;
  status = m_dvblinkRemoteCommunication->GetFavorites(favorites_request, channel_favorites_, NULL);
  favorites_supported_ = (status == DVBLINK_REMOTE_STATUS_OK);

  GetChannelsRequest request;
  m_channels = new ChannelList();
  m_stream = new Stream();
  m_live_streamer = NULL;

  std::string error;
  if ((status = m_dvblinkRemoteCommunication->GetChannels(request, *m_channels, &error)) == DVBLINK_REMOTE_STATUS_OK)
  {
    int iChannelUnique = 0;
    for (std::vector<Channel*>::iterator it = m_channels->begin(); it < m_channels->end(); it++)
    {
      Channel* channel = (*it);
      int channel_id = ++iChannelUnique;
      m_channelMap[channel_id] = channel;
      inverse_channel_map_[channel->GetID()] = channel_id;
    }
    m_connected = true;

    XBMC->Log(LOG_INFO, "Connected to DVBLink Server '%s'", m_hostname.c_str());
    if (m_showinfomsg)
    {
      XBMC->QueueNotification(QUEUE_INFO, XBMC->GetLocalizedString(32001), m_hostname.c_str());
      XBMC->QueueNotification(QUEUE_INFO, XBMC->GetLocalizedString(32002), m_channelMap.size());
    }

    m_recordingsid = GetBuildInRecorderObjectID();

    m_recordingsid_by_date = m_recordingsid;
    m_recordingsid_by_date.append(DVBLINK_RECODINGS_BY_DATA_ID);

    m_recordingsid_by_series = m_recordingsid;
    m_recordingsid_by_series.append(DVBLINK_RECODINGS_BY_SERIES_ID);

    m_updating = true;
    CreateThread();
  }
  else
  {
    XBMC->QueueNotification(QUEUE_ERROR, XBMC->GetLocalizedString(32003), m_hostname.c_str(), (int) status);
    XBMC->Log(LOG_ERROR,
        "Could not connect to DVBLink Server '%s' on port '%i' with username '%s' (Error code : %d Description : %s)",
        hostname.c_str(), port, username.c_str(), (int) status, error.c_str());
  }
}

void *DVBLinkClient::Process()
{
  XBMC->Log(LOG_DEBUG, "DVBLinkUpdateProcess:: thread started");
  unsigned int counter = 0;
  while (m_updating)
  {
    if (counter >= 300000)
    {
      counter = 0;
      PVR->TriggerTimerUpdate();
      Sleep(5000);
      PVR->TriggerRecordingUpdate();
    }
    counter += 1000;
    Sleep(1000);
  }
  XBMC->Log(LOG_DEBUG, "DVBLinkUpdateProcess:: thread stopped");
  return NULL;
}

bool DVBLinkClient::GetStatus()
{
  return m_connected;
}

int DVBLinkClient::GetChannelsAmount()
{
  return m_channelMap.size();
}

PVR_ERROR DVBLinkClient::GetChannels(ADDON_HANDLE handle, bool bRadio)
{
  XBMC->Log(LOG_INFO, "Getting channels (%d channels on server)", m_channelMap.size());
  for (std::map<int, Channel*>::iterator it = m_channelMap.begin(); it != m_channelMap.end(); ++it)
  {
    Channel* channel = (*it).second;

    bool isRadio = (channel->GetChannelType() == Channel::CHANNEL_TYPE_RADIO);

    if (isRadio == bRadio)
    {
      PVR_CHANNEL xbmcChannel;
      memset(&xbmcChannel, 0, sizeof(PVR_CHANNEL));
      xbmcChannel.bIsRadio = isRadio;
      if (channel->Number != -1)
      {
        xbmcChannel.iChannelNumber = channel->Number;
        xbmcChannel.iSubChannelNumber = channel->SubNumber;
      }
      xbmcChannel.iEncryptionSystem = 0;
      xbmcChannel.iUniqueId = (*it).first;

      PVR_STRCPY(xbmcChannel.strChannelName, channel->GetName().c_str());

      if (channel->GetLogoUrl().size() > 0)
        PVR_STRCPY(xbmcChannel.strIconPath, channel->GetLogoUrl().c_str());

      PVR->TransferChannelEntry(handle, &xbmcChannel);
    }
  }
  return PVR_ERROR_NO_ERROR;
}

int DVBLinkClient::GetChannelGroupsAmount(void)
{
  if (!favorites_supported_)
    return -1;

  return channel_favorites_.favorites_.size();
}

PVR_ERROR DVBLinkClient::GetChannelGroups(ADDON_HANDLE handle, bool bRadio)
{
  if (!favorites_supported_)
    return PVR_ERROR_NOT_IMPLEMENTED;

  for (size_t i = 0; i < channel_favorites_.favorites_.size(); i++)
  {
    PVR_CHANNEL_GROUP group;
    memset(&group, 0, sizeof(PVR_CHANNEL_GROUP));
    group.bIsRadio = bRadio;
    PVR_STRCPY(group.strGroupName, channel_favorites_.favorites_[i].get_name().c_str());

    PVR->TransferChannelGroup(handle, &group);
  }

  return PVR_ERROR_NO_ERROR;
}

PVR_ERROR DVBLinkClient::GetChannelGroupMembers(ADDON_HANDLE handle, const PVR_CHANNEL_GROUP &group)
{
  if (!favorites_supported_)
    return PVR_ERROR_NOT_IMPLEMENTED;

  for (size_t i = 0; i < channel_favorites_.favorites_.size(); i++)
  {
    if (channel_favorites_.favorites_[i].get_name() != group.strGroupName)
      continue;

    dvblinkremote::ChannelFavorite::favorite_channel_list_t chlist = channel_favorites_.favorites_[i].get_channels();

    for (size_t j = 0; j < chlist.size(); j++)
    {
      if (inverse_channel_map_.find(chlist[j]) != inverse_channel_map_.end())
      {
        dvblinkremote::Channel* ch = m_channelMap[inverse_channel_map_[chlist[j]]];

        bool isRadio = (ch->GetChannelType() == dvblinkremote::Channel::CHANNEL_TYPE_RADIO);

        if (group.bIsRadio != isRadio)
          continue;

        PVR_CHANNEL_GROUP_MEMBER member;
        memset(&member, 0, sizeof(PVR_CHANNEL_GROUP_MEMBER));
        PVR_STRCPY(member.strGroupName, group.strGroupName);
        member.iChannelUniqueId = inverse_channel_map_[chlist[j]];
        if (ch->Number != -1)
          member.iChannelNumber = ch->Number;

        PVR->TransferChannelGroupMember(handle, &member);
      }
    }
  }
  return PVR_ERROR_NO_ERROR;
}

namespace
{
struct TimerType: PVR_TIMER_TYPE
{
  TimerType(unsigned int id, unsigned int attributes, const std::string &description,
      const std::vector<std::pair<int, std::string> > &maxRecordingsValues, int maxRecordingsDefault,
      const std::vector<std::pair<int, std::string> > &dupEpisodesValues, int dupEpisodesDefault)
  {
    memset(this, 0, sizeof(PVR_TIMER_TYPE));

    iId = id;
    iAttributes = attributes;
    iMaxRecordingsSize = maxRecordingsValues.size();
    iMaxRecordingsDefault = maxRecordingsDefault;
    iPreventDuplicateEpisodesSize = dupEpisodesValues.size();
    iPreventDuplicateEpisodesDefault = dupEpisodesDefault;
    strncpy(strDescription, description.c_str(), sizeof(strDescription) - 1);

    int i = 0;
    for (auto it = maxRecordingsValues.begin(); it != maxRecordingsValues.end(); ++it, ++i)
    {
      maxRecordings[i].iValue = it->first;
      strncpy(maxRecordings[i].strDescription, it->second.c_str(), sizeof(maxRecordings[i].strDescription) - 1);
    }

    i = 0;
    for (auto it = dupEpisodesValues.begin(); it != dupEpisodesValues.end(); ++it, ++i)
    {
      preventDuplicateEpisodes[i].iValue = it->first;
      strncpy(preventDuplicateEpisodes[i].strDescription, it->second.c_str(),
          sizeof(preventDuplicateEpisodes[i].strDescription) - 1);
    }
  }
};

}

PVR_ERROR DVBLinkClient::GetTimerTypes(PVR_TIMER_TYPE types[], int *size)
{
  /* PVR_Timer.iMaxRecordings values and presentation. */
  static std::vector<std::pair<int, std::string> > recordingLimitValues;
  if (recordingLimitValues.size() == 0)
  {
    recordingLimitValues.push_back(std::make_pair(dcrn_keep_all, XBMC->GetLocalizedString(32026)));
    recordingLimitValues.push_back(std::make_pair(dcrn_keep_1, XBMC->GetLocalizedString(32027)));
    recordingLimitValues.push_back(std::make_pair(dcrn_keep_2, XBMC->GetLocalizedString(32028)));
    recordingLimitValues.push_back(std::make_pair(dcrn_keep_3, XBMC->GetLocalizedString(32029)));
    recordingLimitValues.push_back(std::make_pair(dcrn_keep_4, XBMC->GetLocalizedString(32030)));
    recordingLimitValues.push_back(std::make_pair(dcrn_keep_5, XBMC->GetLocalizedString(32031)));
    recordingLimitValues.push_back(std::make_pair(dcrn_keep_6, XBMC->GetLocalizedString(32032)));
    recordingLimitValues.push_back(std::make_pair(dcrn_keep_7, XBMC->GetLocalizedString(32033)));
    recordingLimitValues.push_back(std::make_pair(dcrn_keep_10, XBMC->GetLocalizedString(32034)));
  }

  /* PVR_Timer.iPreventDuplicateEpisodes values and presentation.*/
  static std::vector<std::pair<int, std::string> > showTypeValues;
  if (showTypeValues.size() == 0)
  {
    showTypeValues.push_back(std::make_pair(dcrs_record_all, XBMC->GetLocalizedString(32035)));
    showTypeValues.push_back(std::make_pair(dcrs_record_new_only, XBMC->GetLocalizedString(32036)));
  }

  static std::vector<std::pair<int, std::string> > emptyList;

  static const unsigned int TIMER_MANUAL_ATTRIBS = PVR_TIMER_TYPE_IS_MANUAL | PVR_TIMER_TYPE_FORBIDS_EPG_TAG_ON_CREATE
      | PVR_TIMER_TYPE_SUPPORTS_CHANNELS | PVR_TIMER_TYPE_SUPPORTS_START_TIME | PVR_TIMER_TYPE_SUPPORTS_END_TIME
      | PVR_TIMER_TYPE_SUPPORTS_START_END_MARGIN;

  static const unsigned int TIMER_CREATED_MANUAL_ATTRIBS = PVR_TIMER_TYPE_IS_MANUAL
      | PVR_TIMER_TYPE_FORBIDS_EPG_TAG_ON_CREATE | PVR_TIMER_TYPE_SUPPORTS_START_END_MARGIN
      | PVR_TIMER_TYPE_FORBIDS_NEW_INSTANCES;

  static const unsigned int TIMER_EPG_ATTRIBS = PVR_TIMER_TYPE_REQUIRES_EPG_TAG_ON_CREATE
      | PVR_TIMER_TYPE_SUPPORTS_START_END_MARGIN;

  static const unsigned int TIMER_REPEATING_MANUAL_ATTRIBS = PVR_TIMER_TYPE_IS_REPEATING
      | PVR_TIMER_TYPE_SUPPORTS_WEEKDAYS | PVR_TIMER_TYPE_SUPPORTS_MAX_RECORDINGS;

  static const unsigned int TIMER_CREATED_REPEATING_MANUAL_ATTRIBS = PVR_TIMER_TYPE_IS_MANUAL
      | PVR_TIMER_TYPE_IS_REPEATING | PVR_TIMER_TYPE_FORBIDS_EPG_TAG_ON_CREATE
      | PVR_TIMER_TYPE_SUPPORTS_START_END_MARGIN | PVR_TIMER_TYPE_SUPPORTS_MAX_RECORDINGS
      | PVR_TIMER_TYPE_FORBIDS_NEW_INSTANCES;

  static const unsigned int TIMER_REPEATING_EPG_ATTRIBS = PVR_TIMER_TYPE_IS_REPEATING
      | PVR_TIMER_TYPE_SUPPORTS_START_ANYTIME | PVR_TIMER_TYPE_SUPPORTS_RECORD_ONLY_NEW_EPISODES
      | PVR_TIMER_TYPE_SUPPORTS_MAX_RECORDINGS;

  static const unsigned int TIMER_REPEATING_KEYWORD_ATTRIBS = PVR_TIMER_TYPE_SUPPORTS_TITLE_EPG_MATCH
      | PVR_TIMER_TYPE_SUPPORTS_CHANNELS | PVR_TIMER_TYPE_SUPPORTS_START_END_MARGIN | PVR_TIMER_TYPE_IS_REPEATING
      | PVR_TIMER_TYPE_SUPPORTS_MAX_RECORDINGS;

  static const unsigned int TIMER_CREATED_REPEATING_KEYWORD_ATTRIBS = PVR_TIMER_TYPE_SUPPORTS_START_END_MARGIN
      | PVR_TIMER_TYPE_IS_REPEATING | PVR_TIMER_TYPE_FORBIDS_NEW_INSTANCES | PVR_TIMER_TYPE_SUPPORTS_MAX_RECORDINGS;

  static const unsigned int TIMER_MANUAL_CHILD_ATTRIBUTES = PVR_TIMER_TYPE_IS_MANUAL
      | PVR_TIMER_TYPE_FORBIDS_EPG_TAG_ON_CREATE | PVR_TIMER_TYPE_FORBIDS_NEW_INSTANCES;

  static const unsigned int TIMER_EPG_CHILD_ATTRIBUTES = PVR_TIMER_TYPE_REQUIRES_EPG_TAG_ON_CREATE
      | PVR_TIMER_TYPE_FORBIDS_NEW_INSTANCES;

  static const unsigned int TIMER_KEYWORD_CHILD_ATTRIBUTES = PVR_TIMER_TYPE_FORBIDS_NEW_INSTANCES;

  /* Timer types definition.*/
  static std::vector<std::unique_ptr<TimerType> > timerTypes;
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
    XBMC->GetLocalizedString(32037),
    /* Values definitions for attributes. */
    recordingLimitValues, default_rec_limit_, showTypeValues, default_rec_show_type_)));

    timerTypes.push_back(
    /* One-shot manual (time and channel based) - already created, disable editing of some attributes*/
    std::unique_ptr<TimerType>(new TimerType(
    /* Type id. */
    TIMER_CREATED_ONCE_MANUAL,
    /* Attributes. */
    TIMER_CREATED_MANUAL_ATTRIBS,
    /* Description. */
    XBMC->GetLocalizedString(32037),
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
    XBMC->GetLocalizedString(32038),
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
    XBMC->GetLocalizedString(32039),
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
    XBMC->GetLocalizedString(32040),
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
    XBMC->GetLocalizedString(32041),
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
    XBMC->GetLocalizedString(32042),
    /* Values definitions for attributes. */
    recordingLimitValues, default_rec_limit_, showTypeValues, default_rec_show_type_)));

    timerTypes.push_back(
    /* Repeating manual (time and channel based) Parent - already created, so disable editing of some attributes*/
    std::unique_ptr<TimerType>(new TimerType(
    /* Type id. */
    TIMER_CREATED_REPEATING_MANUAL,
    /* Attributes. */
    TIMER_CREATED_REPEATING_MANUAL_ATTRIBS,
    /* Description. */
    XBMC->GetLocalizedString(32042),
    /* Values definitions for attributes. */
    recordingLimitValues, default_rec_limit_, showTypeValues, default_rec_show_type_)));

    timerTypes.push_back(
    /* Repeating epg based Parent*/
    std::unique_ptr<TimerType>(new TimerType(
    /* Type id. */
    TIMER_REPEATING_EPG,
    /* Attributes. */
    TIMER_EPG_ATTRIBS | TIMER_REPEATING_EPG_ATTRIBS,
    /* Description. */
    XBMC->GetLocalizedString(32043),
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
    XBMC->GetLocalizedString(32044),
    /* Values definitions for attributes. */
    recordingLimitValues, default_rec_limit_, showTypeValues, default_rec_show_type_)));

    timerTypes.push_back(
    /* Repeating Keyword (Generic) based - already created, disable editing of some elements*/
    std::unique_ptr<TimerType>(new TimerType(
    /* Type id. */
    TIMER_CREATED_REPEATING_KEYWORD,
    /* Attributes. */
    TIMER_CREATED_REPEATING_KEYWORD_ATTRIBS,
    /* Description. */
    XBMC->GetLocalizedString(32044),
    /* Values definitions for attributes. */
    recordingLimitValues, default_rec_limit_, showTypeValues, default_rec_show_type_)));
  }

  /* Copy data to target array. */
  int i = 0;
  for (auto it = timerTypes.begin(); it != timerTypes.end(); ++it, ++i)
    types[i] = **it;

  *size = timerTypes.size();
  return PVR_ERROR_NO_ERROR;
}

int DVBLinkClient::GetTimersAmount()
{
  return m_timerCount;
}

int DVBLinkClient::GetInternalUniqueIdFromChannelId(const std::string& channelId)
{
  for (std::map<int, Channel*>::iterator it = m_channelMap.begin(); it != m_channelMap.end(); ++it)
  {
    Channel * channel = (*it).second;
    int id = (*it).first;
    if (channelId.compare(channel->GetID()) == 0)
    {
      return id;
    }
  }
  return 0;
}

std::string DVBLinkClient::make_timer_hash(const std::string& timer_id, const std::string& schedule_id)
{
  std::string res = schedule_id + "#" + timer_id;
  return res;
}

bool DVBLinkClient::parse_timer_hash(const char* timer_hash, std::string& timer_id, std::string& schedule_id)
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
  if (timer_idx_map_.find(id) == timer_idx_map_.end())
    timer_idx_map_[id] = timer_idx_seed_++;

  return timer_idx_map_[id];
}

int DVBLinkClient::GetSchedules(ADDON_HANDLE handle, const RecordingList& recordings)
{
  //make a map of schedule id->recording list
  std::map<std::string, std::vector<Recording*> > schedule_to_timer_map;
  for (size_t i = 0; i < recordings.size(); i++)
  {
    Recording* rec = recordings[i];
    if (schedule_to_timer_map.find(rec->GetScheduleID()) == schedule_to_timer_map.end())
      schedule_to_timer_map[rec->GetScheduleID()] = std::vector<Recording*>();

    schedule_to_timer_map[rec->GetScheduleID()].push_back(rec);
  }

  int added_count = 0;
  int total_count = 0;

  schedule_map_.clear();

  GetSchedulesRequest request;
  StoredSchedules response;

  DVBLinkRemoteStatusCode status;
  std::string error;
  if ((status = m_dvblinkRemoteCommunication->GetSchedules(request, response, &error)) != DVBLINK_REMOTE_STATUS_OK)
  {
    XBMC->Log(LOG_ERROR, "Could not get Schedules (Error code : %d Description : %s)", (int) status, error.c_str());
    return added_count;
  }

  int schedule_num = response.GetManualSchedules().size() + response.GetEpgSchedules().size();

  XBMC->Log(LOG_INFO, "Found %d schedules", schedule_num);

  if (m_showinfomsg)
  {
    XBMC->QueueNotification(QUEUE_INFO, XBMC->GetLocalizedString(32007), schedule_num);
  }

  //manual schedules
  StoredManualScheduleList& manual_schedules = response.GetManualSchedules();
  for (size_t i = 0; i < manual_schedules.size(); i++)
  {
    schedule_map_[manual_schedules[i]->GetID()] = schedule_desc(-1, TIMER_CREATED_ONCE_MANUAL,
        manual_schedules[i]->MarginBefore, manual_schedules[i]->MarginAfter);

    if (manual_schedules[i]->GetDayMask() != 0)
    {
      unsigned int kodi_idx = get_kodi_timer_idx_from_dvblink(manual_schedules[i]->GetID());
      schedule_map_[manual_schedules[i]->GetID()] = schedule_desc(kodi_idx, TIMER_CREATED_REPEATING_MANUAL,
          manual_schedules[i]->MarginBefore, manual_schedules[i]->MarginAfter);

      PVR_TIMER timer;
      memset(&timer, 0, sizeof(PVR_TIMER));
      timer.iEpgUid = PVR_TIMER_NO_EPG_UID;

      //misuse strDirectory to keep id of the timer
      PVR_STRCPY(timer.strDirectory, manual_schedules[i]->GetID().c_str());
      timer.iClientIndex = kodi_idx;
      timer.iClientChannelUid = GetInternalUniqueIdFromChannelId(manual_schedules[i]->GetChannelID());
      timer.state = PVR_TIMER_STATE_SCHEDULED;
      timer.iTimerType = TIMER_CREATED_REPEATING_MANUAL;
      timer.iMarginStart = manual_schedules[i]->MarginBefore / 60;
      timer.iMarginEnd = manual_schedules[i]->MarginAfter / 60;
      timer.iMaxRecordings = manual_schedules[i]->RecordingsToKeep;

      strncpy(timer.strTitle, manual_schedules[i]->Title.c_str(), sizeof(timer.strTitle) - 1);
      timer.startTime = manual_schedules[i]->GetStartTime();
      timer.endTime = manual_schedules[i]->GetStartTime() + manual_schedules[i]->GetDuration();

      PVR->TransferTimerEntry(handle, &timer);
      XBMC->Log(LOG_INFO, "Added EPG schedule : %s", manual_schedules[i]->GetID().c_str());

      added_count += 1;
    }
    total_count += 1;
  }

  //epg based schedules
  StoredEpgScheduleList& epg_schedules = response.GetEpgSchedules();
  for (size_t i = 0; i < epg_schedules.size(); i++)
  {
    schedule_map_[epg_schedules[i]->GetID()] = schedule_desc(-1, TIMER_ONCE_EPG, epg_schedules[i]->MarginBefore,
        epg_schedules[i]->MarginAfter);

    if (epg_schedules[i]->Repeat)
    {
      unsigned int kodi_idx = get_kodi_timer_idx_from_dvblink(epg_schedules[i]->GetID());
      schedule_map_[epg_schedules[i]->GetID()] = schedule_desc(kodi_idx, TIMER_REPEATING_EPG,
          epg_schedules[i]->MarginBefore, epg_schedules[i]->MarginAfter);

      PVR_TIMER timer;
      memset(&timer, 0, sizeof(PVR_TIMER));

      //misuse strDirectory to keep id of the timer
      PVR_STRCPY(timer.strDirectory, epg_schedules[i]->GetID().c_str());
      timer.iClientIndex = kodi_idx;
      timer.iClientChannelUid = GetInternalUniqueIdFromChannelId(epg_schedules[i]->GetChannelID());
      timer.state = PVR_TIMER_STATE_SCHEDULED;
      timer.iTimerType = TIMER_REPEATING_EPG;
      timer.iMarginStart = epg_schedules[i]->MarginBefore / 60;
      timer.iMarginEnd = epg_schedules[i]->MarginAfter / 60;

      timer.iMaxRecordings = epg_schedules[i]->RecordingsToKeep;
      timer.bStartAnyTime = epg_schedules[i]->RecordSeriesAnytime;
      timer.iPreventDuplicateEpisodes = epg_schedules[i]->NewOnly ? dcrs_record_new_only : dcrs_record_all;
      strncpy(timer.strTitle, epg_schedules[i]->program_name_.c_str(), sizeof(timer.strTitle) - 1);

      if (schedule_to_timer_map.find(epg_schedules[i]->GetID()) != schedule_to_timer_map.end() &&
        !schedule_to_timer_map[epg_schedules[i]->GetID()].empty())
      {
        timer.startTime = schedule_to_timer_map[epg_schedules[i]->GetID()].at(0)->GetProgram().GetStartTime();
        timer.endTime = timer.startTime + schedule_to_timer_map[epg_schedules[i]->GetID()].at(0)->GetProgram().GetDuration();
      }

      //the original program, used for scheduling, can already be gone for a long time
      timer.iEpgUid = PVR_TIMER_NO_EPG_UID;

      PVR->TransferTimerEntry(handle, &timer);
      XBMC->Log(LOG_INFO, "Added EPG schedule : %s", epg_schedules[i]->GetID().c_str());

      added_count += 1;
    }
    total_count += 1;
  }

  //epg based schedules
  StoredByPatternScheduleList& bp_schedules = response.GetByPatternSchedules();
  for (size_t i = 0; i < bp_schedules.size(); i++)
  {
    unsigned int kodi_idx = get_kodi_timer_idx_from_dvblink(bp_schedules[i]->GetID());
    schedule_map_[bp_schedules[i]->GetID()] = schedule_desc(kodi_idx, TIMER_CREATED_REPEATING_KEYWORD,
        bp_schedules[i]->MarginBefore, bp_schedules[i]->MarginAfter);

    PVR_TIMER timer;
    memset(&timer, 0, sizeof(PVR_TIMER));

    //misuse strDirectory to keep id of the timer
    PVR_STRCPY(timer.strDirectory, bp_schedules[i]->GetID().c_str());
    timer.iClientIndex = kodi_idx;
    if (!bp_schedules[i]->GetChannelID().empty())
      timer.iClientChannelUid = GetInternalUniqueIdFromChannelId(bp_schedules[i]->GetChannelID());
    else
      timer.iClientChannelUid = PVR_TIMER_ANY_CHANNEL;

    timer.state = PVR_TIMER_STATE_SCHEDULED;
    timer.iTimerType = TIMER_CREATED_REPEATING_KEYWORD;
    timer.iMarginStart = bp_schedules[i]->MarginBefore / 60;
    timer.iMarginEnd = bp_schedules[i]->MarginAfter / 60;
    strncpy(timer.strEpgSearchString, bp_schedules[i]->GetKeyphrase().c_str(), sizeof(timer.strEpgSearchString) - 1);

    if (schedule_to_timer_map.find(bp_schedules[i]->GetID()) != schedule_to_timer_map.end() &&
      !schedule_to_timer_map[bp_schedules[i]->GetID()].empty())
    {
      timer.startTime = schedule_to_timer_map[bp_schedules[i]->GetID()].at(0)->GetProgram().GetStartTime();
      timer.endTime = timer.startTime + schedule_to_timer_map[bp_schedules[i]->GetID()].at(0)->GetProgram().GetDuration();
    }

    strncpy(timer.strTitle, bp_schedules[i]->GetKeyphrase().c_str(), sizeof(timer.strTitle) - 1);
    timer.iEpgUid = PVR_TIMER_NO_EPG_UID;

    PVR->TransferTimerEntry(handle, &timer);
    XBMC->Log(LOG_INFO, "Added EPG schedule : %s", bp_schedules[i]->GetID().c_str());

    added_count += 1;
    total_count += 1;
  }

  return added_count;
}

PVR_ERROR DVBLinkClient::GetTimers(ADDON_HANDLE handle)
{
  PVR_ERROR result = PVR_ERROR_FAILED;
  P8PLATFORM::CLockObject critsec(m_mutex);

  m_timerCount = 0;

  GetRecordingsRequest recordingsRequest;
  RecordingList recordings;

  DVBLinkRemoteStatusCode status;
  std::string error;
  if ((status = m_dvblinkRemoteCommunication->GetRecordings(recordingsRequest, recordings, &error))
      != DVBLINK_REMOTE_STATUS_OK)
  {
    XBMC->Log(LOG_ERROR, "Could not get timers (Error code : %d Description : %s)", (int) status, error.c_str());
    return result;
  }

  XBMC->Log(LOG_INFO, "Found %d timers", recordings.size());

  if (m_showinfomsg)
  {
    XBMC->QueueNotification(QUEUE_INFO, XBMC->GetLocalizedString(32007), recordings.size());
  }

  //get and process schedules first
  int schedule_count = GetSchedules(handle, recordings);

  unsigned int index = PVR_TIMER_NO_CLIENT_INDEX + 1;
  for (size_t i = 0; i < recordings.size(); i++, index++)
  {
    Recording* rec = recordings[i];

    PVR_TIMER xbmcTimer;
    memset(&xbmcTimer, 0, sizeof(PVR_TIMER));

    xbmcTimer.iTimerType = PVR_TIMER_TYPE_NONE;
    //find parent schedule type
    if (schedule_map_.find(rec->GetScheduleID()) != schedule_map_.end())
    {
      int schedule_type = schedule_map_[rec->GetScheduleID()].schedule_kodi_type;
      switch (schedule_type)
      {
        case TIMER_CREATED_ONCE_MANUAL:
        case TIMER_ONCE_EPG:
          //for once timers - copy parent attribute (there was no separate schedule submitted to kodi)
          xbmcTimer.iTimerType = schedule_type;
        break;
        case TIMER_CREATED_REPEATING_MANUAL:
          xbmcTimer.iTimerType = TIMER_ONCE_MANUAL_CHILD;
          xbmcTimer.iParentClientIndex = get_kodi_timer_idx_from_dvblink(rec->GetScheduleID());
        break;
        case TIMER_REPEATING_EPG:
          xbmcTimer.iTimerType = TIMER_ONCE_EPG_CHILD;
          xbmcTimer.iParentClientIndex = get_kodi_timer_idx_from_dvblink(rec->GetScheduleID());
        break;
        case TIMER_CREATED_REPEATING_KEYWORD:
          xbmcTimer.iTimerType = TIMER_ONCE_KEYWORD_CHILD;
          xbmcTimer.iParentClientIndex = get_kodi_timer_idx_from_dvblink(rec->GetScheduleID());
        break;
      }
      //copy margins
      xbmcTimer.iMarginStart = schedule_map_[rec->GetScheduleID()].schedule_margin_before / 60;
      xbmcTimer.iMarginEnd = schedule_map_[rec->GetScheduleID()].schedule_margin_after / 60;
    }

    xbmcTimer.iClientIndex = get_kodi_timer_idx_from_dvblink(rec->GetID());
    //misuse strDirectory to keep id of the timer
    std::string timer_hash = make_timer_hash(rec->GetID(), rec->GetScheduleID());
    PVR_STRCPY(xbmcTimer.strDirectory, timer_hash.c_str());

    xbmcTimer.iClientChannelUid = GetInternalUniqueIdFromChannelId(rec->GetChannelID());
    xbmcTimer.state = PVR_TIMER_STATE_SCHEDULED;
    if (rec->IsActive)
      xbmcTimer.state = PVR_TIMER_STATE_RECORDING;
    if (rec->IsConflict)
      xbmcTimer.state = PVR_TIMER_STATE_CONFLICT_NOK;
    if (!rec->GetProgram().IsRecord)
      xbmcTimer.state = PVR_TIMER_STATE_CANCELLED;

    xbmcTimer.iEpgUid = rec->GetProgram().GetStartTime();

    xbmcTimer.startTime = rec->GetProgram().GetStartTime();
    xbmcTimer.endTime = rec->GetProgram().GetStartTime() + rec->GetProgram().GetDuration();
    PVR_STRCPY(xbmcTimer.strTitle, rec->GetProgram().GetTitle().c_str());
    PVR_STRCPY(xbmcTimer.strSummary, rec->GetProgram().ShortDescription.c_str());

    int genre_type, genre_subtype;
    SetEPGGenre(rec->GetProgram(), genre_type, genre_subtype);
    if (genre_type == EPG_GENRE_USE_STRING)
    {
      xbmcTimer.iGenreType = EPG_EVENT_CONTENTMASK_UNDEFINED;
    }
    else
    {
      xbmcTimer.iGenreType = genre_type;
      xbmcTimer.iGenreSubType = genre_subtype;
    }

    PVR->TransferTimerEntry(handle, &xbmcTimer);
    XBMC->Log(LOG_INFO, "Added EPG timer : %s", rec->GetProgram().GetTitle().c_str());
  }

  m_timerCount = recordings.size() + schedule_count;

  result = PVR_ERROR_NO_ERROR;
  return result;
}

bool DVBLinkClient::get_dvblink_program_id(std::string& channelId, int start_time, std::string& dvblink_program_id)
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

PVR_ERROR DVBLinkClient::AddTimer(const PVR_TIMER &timer)
{
  PVR_ERROR result = PVR_ERROR_FAILED;
  P8PLATFORM::CLockObject critsec(m_mutex);
  DVBLinkRemoteStatusCode status;
  AddScheduleRequest * addScheduleRequest = NULL;

  int marginBefore = -1;
  int marginAfter = -1;
  if (setting_margins_supported_)
  {
    marginBefore = timer.iMarginStart * 60;
    marginAfter = timer.iMarginEnd * 60;
  }

  int numberToKeep = timer.iMaxRecordings;
  if (numberToKeep < 0)
    numberToKeep = dcrn_keep_all;

  switch (timer.iTimerType)
  {
    case TIMER_ONCE_MANUAL:
    {
      std::string channelId = m_channelMap[timer.iClientChannelUid]->GetID();
      time_t start_time = timer.startTime;
      //check for instant recording
      if (start_time == 0)
        time(&start_time);

      time_t duration = timer.endTime - start_time;
      long day_mask = 0;
      addScheduleRequest = new AddManualScheduleRequest(channelId, start_time, duration, day_mask, timer.strTitle, 0,
          marginBefore, marginAfter);
    }
    break;
    case TIMER_REPEATING_MANUAL:
    {
      std::string channelId = m_channelMap[timer.iClientChannelUid]->GetID();
      time_t start_time = timer.startTime;
      time_t duration = timer.endTime - timer.startTime;
      long day_mask = 0;
      if (timer.iWeekdays > 0)  // repeating timer?
      {
        //change day mask to DVBLink server format (Sun - first day)
        bool bcarry = (timer.iWeekdays & 0x40) == 0x40;
        day_mask = (timer.iWeekdays << 1) & 0x7F;
        if (bcarry)
          day_mask |= 0x01;
        //find first now/future time, which matches the day mask
        start_time = timer.startTime > timer.firstDay ? timer.startTime : timer.firstDay;
        for (size_t i = 0; i < 7; i++)
        {
          tm* local_start_time = localtime(&start_time);
          if (is_bit_set(local_start_time->tm_wday, (unsigned char) day_mask))
            break;
          start_time += time_t(24 * 3600);
        }
      }
      addScheduleRequest = new AddManualScheduleRequest(channelId, start_time, duration, day_mask, timer.strTitle,
          numberToKeep, marginBefore, marginAfter);
    }
    break;
    case TIMER_ONCE_EPG:
    {
      std::string channelId = m_channelMap[timer.iClientChannelUid]->GetID();

      std::string dvblink_program_id;
      if (get_dvblink_program_id(channelId, timer.iEpgUid, dvblink_program_id))
      {
        addScheduleRequest = new AddScheduleByEpgRequest(channelId, dvblink_program_id, false, true, true,
            dcrn_keep_all, marginBefore, marginAfter);
      }
    }
    break;
    case TIMER_REPEATING_EPG:
    {
      std::string channelId = m_channelMap[timer.iClientChannelUid]->GetID();
      bool record_series = true;
      bool newOnly = timer.iPreventDuplicateEpisodes == dcrs_record_all ? false : true;
      bool anytime = timer.bStartAnyTime;

      std::string dvblink_program_id;
      if (get_dvblink_program_id(channelId, timer.iEpgUid, dvblink_program_id))
      {
        addScheduleRequest = new AddScheduleByEpgRequest(channelId, dvblink_program_id, record_series, newOnly, anytime,
            numberToKeep, marginBefore, marginAfter);
      }
    }
    break;
    case TIMER_REPEATING_KEYWORD:
    {
      //empty string is "any channel"
      std::string channelId;
      if (timer.iClientChannelUid != PVR_TIMER_ANY_CHANNEL)
        channelId = m_channelMap[timer.iClientChannelUid]->GetID();

      std::string key_phrase = timer.strEpgSearchString;
      long genre_mask = 0;  //any genre

      addScheduleRequest = new AddScheduleByPatternRequest(channelId, key_phrase, genre_mask, numberToKeep,
          marginBefore, marginAfter);
    }
    break;
  }

  if (addScheduleRequest != NULL)
  {
    std::string error;
    if ((status = m_dvblinkRemoteCommunication->AddSchedule(*addScheduleRequest, &error)) == DVBLINK_REMOTE_STATUS_OK)
    {
      XBMC->Log(LOG_INFO, "Timer added");
      PVR->TriggerTimerUpdate();
      result = PVR_ERROR_NO_ERROR;
    }
    else
    {
      result = PVR_ERROR_FAILED;
      XBMC->Log(LOG_ERROR, "Could not add timer (Error code : %d Description : %s)", (int) status, error.c_str());
    }
    SAFE_DELETE(addScheduleRequest);
  }
  else
  {
    result = PVR_ERROR_FAILED;
  }
  return result;
}

PVR_ERROR DVBLinkClient::DeleteTimer(const PVR_TIMER &timer)
{
  PVR_ERROR result = PVR_ERROR_FAILED;
  P8PLATFORM::CLockObject critsec(m_mutex);
  DVBLinkRemoteStatusCode status = DVBLINK_REMOTE_STATUS_ERROR;
  std::string error;

  switch (timer.iTimerType)
  {
    case TIMER_CREATED_ONCE_MANUAL:
    case TIMER_ONCE_EPG:
    case TIMER_ONCE_MANUAL_CHILD:
    case TIMER_ONCE_EPG_CHILD:
    case TIMER_ONCE_KEYWORD_CHILD:
    {
      //this is a timer

      //timer id hash is kept in strDirectory!
      std::string timer_id;
      std::string schedule_id;
      parse_timer_hash(timer.strDirectory, timer_id, schedule_id);

      RemoveRecordingRequest removeRecording(timer_id);
      status = m_dvblinkRemoteCommunication->RemoveRecording(removeRecording, &error);
    }
    break;
    case TIMER_CREATED_REPEATING_MANUAL:
    case TIMER_REPEATING_EPG:
    case TIMER_CREATED_REPEATING_KEYWORD:
    {
      //this is a schedule

      //schedule id is in the timer.strDirectory
      std::string schedule_id = timer.strDirectory;
      RemoveScheduleRequest removeSchedule(schedule_id);
      status = m_dvblinkRemoteCommunication->RemoveSchedule(removeSchedule, &error);
    }
    break;
  }

  if (status == DVBLINK_REMOTE_STATUS_OK)
  {
    XBMC->Log(LOG_INFO, "Timer(s) deleted");
    PVR->TriggerTimerUpdate();
    result = PVR_ERROR_NO_ERROR;
  }
  else
  {
    XBMC->Log(LOG_ERROR, "Timer could not be deleted (Error code : %d Description : %s)", (int) status, error.c_str());
  }
  return result;
}

PVR_ERROR DVBLinkClient::UpdateTimer(const PVR_TIMER &timer)
{
  PVR_ERROR result = PVR_ERROR_NO_ERROR;
  P8PLATFORM::CLockObject critsec(m_mutex);

  std::string schedule_id;
  switch (timer.iTimerType)
  {
    case TIMER_CREATED_ONCE_MANUAL:
    case TIMER_ONCE_EPG:
    {
      //this is a timer

      //timer id hash is kept in strDirectory!
      std::string timer_id;
      parse_timer_hash(timer.strDirectory, timer_id, schedule_id);
    }
    break;
    case TIMER_CREATED_REPEATING_MANUAL:
    case TIMER_REPEATING_EPG:
    case TIMER_CREATED_REPEATING_KEYWORD:
    {
      //this is a schedule

      //schedule id is in the timer.strDirectory
      schedule_id = timer.strDirectory;
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
    bool new_only = timer.iPreventDuplicateEpisodes == dcrs_record_new_only;
    bool recordSeriesAnytime = timer.bStartAnyTime;
    int recordingsToKeep = timer.iMaxRecordings;
    int margin_before = timer.iMarginStart * 60;
    int margin_after = timer.iMarginEnd * 60;

    UpdateScheduleRequest update_request(schedule_id, new_only, recordSeriesAnytime, recordingsToKeep, margin_before,
        margin_after);

    std::string error;
    DVBLinkRemoteStatusCode status = m_dvblinkRemoteCommunication->UpdateSchedule(update_request, &error);

    if (status == DVBLINK_REMOTE_STATUS_OK)
    {
      XBMC->Log(LOG_INFO, "Schedule %s was updated", schedule_id.c_str());
      PVR->TriggerTimerUpdate();
      result = PVR_ERROR_NO_ERROR;
    }
    else
    {
      XBMC->Log(LOG_ERROR, "Schedule %s update failed (Error code : %d Description : %s)", schedule_id.c_str(),
          (int) status, error.c_str());
    }
  }
  return result;
}

int DVBLinkClient::GetRecordingsAmount()
{

  return m_recordingCount;
}

std::string DVBLinkClient::GetRecordedTVByDateObjectID(const std::string& buildInRecoderObjectID)
{
  std::string result = "";
  DVBLinkRemoteStatusCode status;

  GetPlaybackObjectRequest getPlaybackObjectRequest(m_hostname.c_str(), buildInRecoderObjectID);
  getPlaybackObjectRequest.IncludeChildrenObjectsForRequestedObject = true;
  GetPlaybackObjectResponse getPlaybackObjectResponse;

  if ((status = m_dvblinkRemoteCommunication->GetPlaybackObject(getPlaybackObjectRequest, getPlaybackObjectResponse,
      NULL)) == DVBLINK_REMOTE_STATUS_OK)
  {
    for (std::vector<PlaybackContainer*>::iterator it = getPlaybackObjectResponse.GetPlaybackContainers().begin();
        it < getPlaybackObjectResponse.GetPlaybackContainers().end(); it++)
    {
      PlaybackContainer * container = (PlaybackContainer *) *it;

      if (container->GetObjectID().find("F6F08949-2A07-4074-9E9D-423D877270BB") != std::string::npos)
      {
        result = container->GetObjectID();
        break;
      }
    }
  }
  return result;

}

PVR_ERROR DVBLinkClient::DeleteRecording(const PVR_RECORDING& recording)
{
//  P8PLATFORM::CLockObject critsec(m_mutex);
  PVR_ERROR result = PVR_ERROR_FAILED;
  DVBLinkRemoteStatusCode status;
  RemovePlaybackObjectRequest remoteObj(recording.strRecordingId);

  std::string error;
  if ((status = m_dvblinkRemoteCommunication->RemovePlaybackObject(remoteObj, &error)) != DVBLINK_REMOTE_STATUS_OK)
  {
    XBMC->Log(LOG_ERROR, "Recording %s could not be deleted (Error code: %d Description : %s)", recording.strTitle,
        (int) status, error.c_str());
    return result;
  }

  XBMC->Log(LOG_INFO, "Recording %s deleted", recording.strTitle);
  PVR->TriggerRecordingUpdate();
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

PVR_ERROR DVBLinkClient::GetRecordings(ADDON_HANDLE handle)
{
  P8PLATFORM::CLockObject critsec(m_mutex);
  PVR_ERROR result = PVR_ERROR_FAILED;
  DVBLinkRemoteStatusCode status;
  m_recording_id_to_url_map.clear();

  GetPlaybackObjectRequest getPlaybackObjectRequest(m_hostname.c_str(), m_recordingsid_by_date);
  getPlaybackObjectRequest.IncludeChildrenObjectsForRequestedObject = true;
  GetPlaybackObjectResponse getPlaybackObjectResponse;

  std::string error;
  if ((status = m_dvblinkRemoteCommunication->GetPlaybackObject(getPlaybackObjectRequest, getPlaybackObjectResponse,
      &error)) != DVBLINK_REMOTE_STATUS_OK)
  {
    XBMC->Log(LOG_ERROR, "Could not get recordings (Error code : %d Description : %s)", (int) status, error.c_str());
    //XBMC->QueueNotification(QUEUE_ERROR, XBMC->GetLocalizedString(32004), (int)status);
    return result;
  }

  XBMC->Log(LOG_INFO, "Found %d recordings", getPlaybackObjectResponse.GetPlaybackItems().size());

  if (m_showinfomsg)
  {
    XBMC->QueueNotification(QUEUE_INFO, XBMC->GetLocalizedString(32009),
        getPlaybackObjectResponse.GetPlaybackItems().size());
  }

  std::map<std::string, int> schedule_to_num_map;
  if (no_group_single_rec_)
  {
    //build a map with scheule id -> number of recordings
    for (std::vector<PlaybackItem*>::iterator it = getPlaybackObjectResponse.GetPlaybackItems().begin();
        it < getPlaybackObjectResponse.GetPlaybackItems().end(); it++)
    {
      RecordedTvItem * tvitem = (RecordedTvItem *) *it;
      if (tvitem->ScheduleId.size() > 0 && tvitem->SeriesSchedule)
      {
        if (schedule_to_num_map.find(tvitem->ScheduleId) == schedule_to_num_map.end())
          schedule_to_num_map[tvitem->ScheduleId] = 0;

        schedule_to_num_map[tvitem->ScheduleId] = schedule_to_num_map[tvitem->ScheduleId] + 1;
      }
    }
  }

  for (std::vector<PlaybackItem*>::iterator it = getPlaybackObjectResponse.GetPlaybackItems().begin();
      it < getPlaybackObjectResponse.GetPlaybackItems().end(); it++)
  {
    RecordedTvItem * tvitem = (RecordedTvItem *) *it;
    PVR_RECORDING xbmcRecording;
    memset(&xbmcRecording, 0, sizeof(PVR_RECORDING));

    PVR_STRCPY(xbmcRecording.strRecordingId, tvitem->GetObjectID().c_str());

    std::string title = tvitem->GetMetadata().GetTitle();
    if (m_add_episode_to_rec_title)
    {
      //form a title as "name - (SxxExx) subtitle" because XBMC does not display episode/season information almost anywhere
      std::string se_str = get_subtitle(tvitem->GetMetadata().SeasonNumber, tvitem->GetMetadata().EpisodeNumber,
          tvitem->GetMetadata().SubTitle, (int) tvitem->GetMetadata().Year);
      if (se_str.size() > 0)
        title += " - " + se_str;
    }
    PVR_STRCPY(xbmcRecording.strTitle, title.c_str());
    PVR_STRCPY(xbmcRecording.strEpisodeName, tvitem->GetMetadata().SubTitle.c_str());
    if (tvitem->GetMetadata().SeasonNumber > 0)
      xbmcRecording.iSeriesNumber = tvitem->GetMetadata().SeasonNumber;
    else
      xbmcRecording.iSeriesNumber = -1;

    if (tvitem->GetMetadata().EpisodeNumber > 0)
      xbmcRecording.iEpisodeNumber = tvitem->GetMetadata().EpisodeNumber;
    else
      xbmcRecording.iEpisodeNumber = -1;
    xbmcRecording.iYear = tvitem->GetMetadata().Year;

    xbmcRecording.recordingTime = tvitem->GetMetadata().GetStartTime();
    PVR_STRCPY(xbmcRecording.strPlot, tvitem->GetMetadata().ShortDescription.c_str());
    PVR_STRCPY(xbmcRecording.strPlotOutline, tvitem->GetMetadata().SubTitle.c_str());
    //    PVR_STRCPY(xbmcRecording.strStreamURL, tvitem->GetPlaybackUrl().c_str());
    m_recording_id_to_url_map[xbmcRecording.strRecordingId] = tvitem->GetPlaybackUrl();
    xbmcRecording.iDuration = tvitem->GetMetadata().GetDuration();
    PVR_STRCPY(xbmcRecording.strChannelName, tvitem->ChannelName.c_str());
    PVR_STRCPY(xbmcRecording.strThumbnailPath, tvitem->GetThumbnailUrl().c_str());
    int genre_type, genre_subtype;
    SetEPGGenre(tvitem->GetMetadata(), genre_type, genre_subtype);
    if (genre_type == EPG_GENRE_USE_STRING)
    {
      xbmcRecording.iGenreType = EPG_EVENT_CONTENTMASK_UNDEFINED;
    }
    else
    {
      xbmcRecording.iGenreType = genre_type;
      xbmcRecording.iGenreSubType = genre_subtype;
    }

    if (m_group_recordings_by_series)
    {
      if (tvitem->ScheduleId.size() > 0 && tvitem->SeriesSchedule && tvitem->ScheduleName.size() > 0)
      {
        bool b = true;

        if (no_group_single_rec_ && schedule_to_num_map.find(tvitem->ScheduleId) != schedule_to_num_map.end()
            && schedule_to_num_map[tvitem->ScheduleId] < 2)
          b = false;

        if (b)
          PVR_STRCPY(xbmcRecording.strDirectory, tvitem->ScheduleName.c_str());
      }
    }

    /* TODO: PVR API 5.0.0: Implement this */
    xbmcRecording.iChannelUid = PVR_CHANNEL_INVALID_UID;

    /* TODO: PVR API 5.1.0: Implement this */
    xbmcRecording.channelType = PVR_RECORDING_CHANNEL_TYPE_UNKNOWN;

    PVR->TransferRecordingEntry(handle, &xbmcRecording);

  }
  m_recordingCount = getPlaybackObjectResponse.GetPlaybackItems().size();
  result = PVR_ERROR_NO_ERROR;
  return result;
}

bool DVBLinkClient::GetRecordingURL(const char* recording_id, std::string& url, bool use_transcoder, int width, 
                                    int height, int bitrate, std::string audiotrack)
{
  bool ret_val = false;

  //if transcoding is requested and no transcoder is supported return false
  if ((use_transcoder && !transcoding_supported_) || (use_transcoder && !transcoding_recordings_supported_))
  {
    XBMC->QueueNotification(QUEUE_ERROR, XBMC->GetLocalizedString(32024));
    return false;
  }

  if (m_recording_id_to_url_map.find(recording_id) != m_recording_id_to_url_map.end())
  {
    url = m_recording_id_to_url_map[recording_id];

    if (use_transcoder)
    {
      int w = width == 0 ? GUI->GetScreenWidth() : width;
      int h = height == 0 ? GUI->GetScreenHeight() : height;

      char buf[1024];
      sprintf(buf, "%s&transcoder=hls&client_id=%s&width=%d&height=%d&bitrate=%d", url.c_str(), m_clientname.c_str(), w, h, bitrate);
      url = buf;

      if (audiotrack.size() > 0)
        url += "&lng=" + audiotrack;
    }

    ret_val = true;
  }
  else
  {
    XBMC->Log(LOG_ERROR, "Could not get playback url for recording %s)", recording_id);
  }
  return ret_val;
}

void DVBLinkClient::GetDriveSpace(long long *iTotal, long long *iUsed)
{
//  P8PLATFORM::CLockObject critsec(m_mutex);
  GetRecordingSettingsRequest recordingsettingsrequest;
  *iTotal = 0;
  *iUsed = 0;
  RecordingSettings settings;
  DVBLinkRemoteStatusCode status;
  if ((status = m_dvblinkRemoteCommunication->GetRecordingSettings(recordingsettingsrequest, settings, NULL))
      == DVBLINK_REMOTE_STATUS_OK)
  {
    *iTotal = settings.TotalSpace;
    *iUsed = (settings.TotalSpace - settings.AvailableSpace);
  }
}

int DVBLinkClient::GetCurrentChannelId()
{
  return m_currentChannelId;
}

bool DVBLinkClient::StartStreaming(const PVR_CHANNEL &channel, StreamRequest* streamRequest, std::string& stream_url)
{
  DVBLinkRemoteStatusCode status;
  std::string error;
  if ((status = m_dvblinkRemoteCommunication->PlayChannel(*streamRequest, *m_stream, &error))
      != DVBLINK_REMOTE_STATUS_OK)
  {
    XBMC->Log(LOG_ERROR, "Could not start streaming for channel %i (Error code : %d)", channel.iUniqueId, (int) status,
        error.c_str());
    XBMC->QueueNotification(QUEUE_ERROR, XBMC->GetLocalizedString(32010), channel.strChannelName, (int) status);
    return false;
  }

  m_currentChannelId = channel.iUniqueId;
  stream_url = m_stream->GetUrl();
  return true;
}

bool DVBLinkClient::OpenLiveStream(const PVR_CHANNEL &channel, bool use_timeshift, bool use_transcoder, int width,
    int height, int bitrate, std::string audiotrack)
{
  bool ret_val = false;

  //if transcoding is requested and no transcoder is supported return false
  if (use_transcoder && !transcoding_supported_)
  {
    XBMC->QueueNotification(QUEUE_ERROR, XBMC->GetLocalizedString(32024));
    return false;
  }

  P8PLATFORM::CLockObject critsec(m_mutex);

  if (m_live_streamer)
  {
    SAFE_DELETE(m_live_streamer);
  }
  //time-shifted playback always uses raw http stream type - no transcoding option is possible now
  if (use_timeshift)
    m_live_streamer = new TimeShiftBuffer(XBMC);
  else
    m_live_streamer = new LiveTVStreamer(XBMC);

  //adjust transcoded height and width if needed
  int w = width == 0 ? GUI->GetScreenWidth() : width;
  int h = height == 0 ? GUI->GetScreenHeight() : height;

  Channel * c = m_channelMap[channel.iUniqueId];

  StreamRequest* sr = m_live_streamer->GetStreamRequest(c->GetDvbLinkID(), m_clientname, m_hostname, use_transcoder, w,
      h, bitrate, audiotrack);
  if (sr != NULL)
  {
    std::string url;
    if (StartStreaming(channel, sr, url))
    {
      m_live_streamer->Start(url);
      ret_val = true;
    }
    else
    {
      delete m_live_streamer;
      m_live_streamer = NULL;
    }
    delete sr;
  }
  else
  {
    XBMC->Log(LOG_ERROR, "m_live_streamer->GetStreamRequest returned NULL. (channel %i)", channel.iUniqueId);
    delete m_live_streamer;
    m_live_streamer = NULL;
  }
  return ret_val;
}

int DVBLinkClient::ReadLiveStream(unsigned char *pBuffer, unsigned int iBufferSize)
{
  if (m_live_streamer)
    return m_live_streamer->ReadData(pBuffer, iBufferSize);
  return 0;
}

long long DVBLinkClient::SeekLiveStream(long long iPosition, int iWhence)
{
  if (m_live_streamer)
    return m_live_streamer->Seek(iPosition, iWhence);
  return 0;
}

long long DVBLinkClient::PositionLiveStream(void)
{
  if (m_live_streamer)
    return m_live_streamer->Position();
  return 0;
}
long long DVBLinkClient::LengthLiveStream(void)
{
  if (m_live_streamer)
    return m_live_streamer->Length();
  return 0;
}

time_t DVBLinkClient::GetPlayingTime()
{
  if (m_live_streamer)
    return m_live_streamer->GetPlayingTime();
  return 0;
}

time_t DVBLinkClient::GetBufferTimeStart()
{
  if (m_live_streamer)
    return m_live_streamer->GetBufferTimeStart();
  return 0;
}

time_t DVBLinkClient::GetBufferTimeEnd()
{
  if (m_live_streamer)
    return m_live_streamer->GetBufferTimeEnd();
  return 0;
}

void DVBLinkClient::StopStreaming(bool bUseChlHandle)
{
  P8PLATFORM::CLockObject critsec(m_mutex);
  StopStreamRequest * request;

  if (m_live_streamer != NULL)
  {
    m_live_streamer->Stop();
    SAFE_DELETE(m_live_streamer);
    m_live_streamer = NULL;
  }

  if (bUseChlHandle)
  {
    request = new StopStreamRequest(m_stream->GetChannelHandle());
  }
  else
  {
    request = new StopStreamRequest(m_clientname);
  }

  DVBLinkRemoteStatusCode status;
  std::string error;
  if ((status = m_dvblinkRemoteCommunication->StopChannel(*request, &error)) != DVBLINK_REMOTE_STATUS_OK)
  {
    XBMC->Log(LOG_ERROR, "Could not stop stream (Error code : %d Description : %s)", (int) status, error.c_str());
  }

  SAFE_DELETE(request);
}

void DVBLinkClient::SetEPGGenre(dvblinkremote::ItemMetadata& metadata, int& genre_type, int& genre_subtype)
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
    genre_subtype = metadata.IsCatThriller ? 0x01 : metadata.IsCatScifi ? 0x03 : metadata.IsCatHorror ? 0x03 :
                    metadata.IsCatComedy ? 0x04 : metadata.IsCatSoap ? 0x05 : metadata.IsCatRomance ? 0x06 :
                    metadata.IsCatDrama ? 0x08 : 0;
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

bool DVBLinkClient::DoEPGSearch(EpgSearchResult& epgSearchResult, const std::string& channelId, const long startTime,
    const long endTime, const std::string& programId)
{
  P8PLATFORM::CLockObject critsec(m_mutex);
  EpgSearchRequest epgSearchRequest(channelId, startTime, endTime);
  if (programId.compare("") != 0)
  {
    epgSearchRequest.ProgramID = programId;
  }

  DVBLinkRemoteStatusCode status;

  if ((status = m_dvblinkRemoteCommunication->SearchEpg(epgSearchRequest, epgSearchResult, NULL))
      == DVBLINK_REMOTE_STATUS_OK)
  {
    return true;
  }
  return false;
}

PVR_ERROR DVBLinkClient::GetEPGForChannel(ADDON_HANDLE handle, const PVR_CHANNEL& channel, time_t iStart, time_t iEnd)
{
  PVR_ERROR result = PVR_ERROR_FAILED;
  P8PLATFORM::CLockObject critsec(m_mutex);
  Channel * c = m_channelMap[channel.iUniqueId];
  EpgSearchResult epgSearchResult;

  if (DoEPGSearch(epgSearchResult, c->GetID(), iStart, iEnd))
  {
    for (std::vector<ChannelEpgData*>::iterator it = epgSearchResult.begin(); it < epgSearchResult.end(); it++)
    {
      ChannelEpgData* channelEpgData = (ChannelEpgData*) *it;
      EpgData& epgData = channelEpgData->GetEpgData();
      for (std::vector<Program*>::iterator pIt = epgData.begin(); pIt < epgData.end(); pIt++)
      {
        Program* p = (Program*) *pIt;
        EPG_TAG broadcast;
        memset(&broadcast, 0, sizeof(EPG_TAG));

        broadcast.iUniqueBroadcastId = p->GetStartTime();
        broadcast.strTitle = p->GetTitle().c_str();
        broadcast.iChannelNumber = channel.iChannelNumber;
        broadcast.startTime = p->GetStartTime();
        broadcast.endTime = p->GetStartTime() + p->GetDuration();
        broadcast.strPlot = p->ShortDescription.c_str();
        broadcast.strCast = p->Actors.c_str();
        broadcast.strDirector = p->Directors.c_str();
        broadcast.strWriter = p->Writers.c_str();
        broadcast.iYear = p->Year;
        broadcast.strIconPath = p->Image.c_str();
        broadcast.iGenreType = 0;
        broadcast.iGenreSubType = 0;
        broadcast.strGenreDescription = "";
        broadcast.firstAired = 0;
        broadcast.iParentalRating = 0;
        broadcast.iStarRating = p->Rating;
        broadcast.bNotify = false;
        broadcast.iSeriesNumber = p->SeasonNumber;
        broadcast.iEpisodeNumber = p->EpisodeNumber;
        broadcast.iEpisodePartNumber = 0;
        broadcast.strEpisodeName = p->SubTitle.c_str();
        broadcast.strIMDBNumber = NULL;  // unused
        broadcast.strOriginalTitle = NULL;  // unused
        broadcast.strPlotOutline = NULL;

        int genre_type, genre_subtype;
        SetEPGGenre(*p, genre_type, genre_subtype);
        broadcast.iGenreType = genre_type;
        if (genre_type == EPG_GENRE_USE_STRING)
          broadcast.strGenreDescription = p->Keywords.c_str();
        else
          broadcast.iGenreSubType = genre_subtype;

        broadcast.iFlags = EPG_TAG_FLAG_UNDEFINED;

        PVR->TransferEpgEntry(handle, &broadcast);
      }
    }
    result = PVR_ERROR_NO_ERROR;
  }
  else
  {
    XBMC->Log(LOG_NOTICE, "Not EPG data found for channel : %s with id : %i", channel.strChannelName,
        channel.iUniqueId);
  }
  return result;
}

DVBLinkClient::~DVBLinkClient(void)
{
  m_updating = false;
  if (IsRunning())
  {
    StopThread();
  }

  SAFE_DELETE(m_dvblinkRemoteCommunication);
  SAFE_DELETE(m_httpClient);
  SAFE_DELETE(m_channels);
  SAFE_DELETE(m_stream);
  if (m_live_streamer)
  {
    m_live_streamer->Stop();
    SAFE_DELETE(m_live_streamer);
  }
}
