/***************************************************************************
 * Copyright (C) 2012 Marcus Efraimsson.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a 
 * copy of this software and associated documentation files (the "Software"), 
 * to deal in the Software without restriction, including without limitation 
 * the rights to use, copy, modify, merge, publish, distribute, sublicense, 
 * and/or sell copies of the Software, and to permit persons to whom the 
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included 
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS 
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL 
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
 * DEALINGS IN THE SOFTWARE.
 *
 ***************************************************************************/

#include "scheduling.h"
#include "xml_object_serializer.h"

using namespace dvblinkremote;
using namespace dvblinkremoteserialization;

Schedule::Schedule()
{

}

Schedule::Schedule(const DVBLinkScheduleType scheduleType, const std::string& channelId, const int recordingsToKeep, const int marginBefore, const int marginAfter)
  : m_scheduleType(scheduleType), 
    m_channelId(channelId), 
    RecordingsToKeep(recordingsToKeep),
    MarginBefore(marginBefore),
    MarginAfter(marginAfter)
{
  m_id = "";
  UserParameter = "";
  ForceAdd = false;
}

Schedule::Schedule(const DVBLinkScheduleType scheduleType, const std::string& id, const std::string& channelId, const int recordingsToKeep, const int marginBefore, const int marginAfter)
  : m_scheduleType(scheduleType), 
    m_id(id),
    m_channelId(channelId), 
    RecordingsToKeep(recordingsToKeep),
    MarginBefore(marginBefore),
    MarginAfter(marginAfter)
{
  UserParameter = "";
  ForceAdd = false;
}

Schedule::~Schedule()
{

}

std::string& Schedule::GetID() 
{ 
  return m_id; 
}

std::string& Schedule::GetChannelID() 
{ 
  return m_channelId; 
}

Schedule::DVBLinkScheduleType& Schedule::GetScheduleType() 
{ 
  return m_scheduleType; 
}

ManualSchedule::ManualSchedule(const std::string& channelId, const long startTime, const long duration, const long dayMask, const std::string& title, const int recordingsToKeep, const int marginBefore, const int marginAfter)
    : Schedule(SCHEDULE_TYPE_MANUAL, channelId, recordingsToKeep, marginBefore, marginAfter),
    m_startTime(startTime), 
    m_duration(duration), 
    m_dayMask(dayMask), 
    Title(title)
{

}

ManualSchedule::ManualSchedule(const std::string& id, const std::string& channelId, const long startTime, const long duration, const long dayMask, const std::string& title, const int recordingsToKeep, const int marginBefore, const int marginAfter)
    : Schedule(SCHEDULE_TYPE_MANUAL, id, channelId, recordingsToKeep, marginBefore, marginAfter),
    m_startTime(startTime), 
    m_duration(duration), 
    m_dayMask(dayMask), 
    Title(title)
{

}

ManualSchedule::~ManualSchedule()
{

}

long ManualSchedule::GetStartTime() 
{ 
  return m_startTime; 
}

long ManualSchedule::GetDuration() 
{ 
  return m_duration; 
}
    
long ManualSchedule::GetDayMask() 
{ 
  return m_dayMask; 
}

EpgSchedule::EpgSchedule(const std::string& channelId, const std::string& programId, const bool repeat, const bool newOnly, const bool recordSeriesAnytime, const int recordingsToKeep, const int marginBefore, const int marginAfter)
    : Schedule(SCHEDULE_TYPE_BY_EPG, channelId, recordingsToKeep, marginBefore, marginAfter),
    m_programId(programId), 
    Repeat(repeat), 
    NewOnly(newOnly), 
    RecordSeriesAnytime(recordSeriesAnytime)
{

}

EpgSchedule::EpgSchedule(const std::string& id, const std::string& channelId, const std::string& programId, const bool repeat, const bool newOnly, 
    const bool recordSeriesAnytime, const int recordingsToKeep, const int marginBefore, const int marginAfter)
    : Schedule(SCHEDULE_TYPE_BY_EPG, id, channelId, recordingsToKeep, marginBefore, marginAfter),
    m_programId(programId), 
    Repeat(repeat), 
    NewOnly(newOnly), 
    RecordSeriesAnytime(recordSeriesAnytime)
{

}

EpgSchedule::~EpgSchedule()
{

}

std::string& EpgSchedule::GetProgramID() 
{ 
  return m_programId; 
}
////

ByPatternSchedule::ByPatternSchedule(const std::string& id, const std::string& channelId, const std::string& keyphrase, const long genreMask, 
									 const int recordingsToKeep, const int marginBefore, const int marginAfter)
    : Schedule(SCHEDULE_TYPE_BY_PATTERN, id, channelId, recordingsToKeep, marginBefore, marginAfter),
    m_genreMask(genreMask), 
    m_keyphrase(keyphrase)
{

}

ByPatternSchedule::ByPatternSchedule(const std::string& channelId, const std::string& keyphrase, const long genreMask, 
									 const int recordingsToKeep, const int marginBefore, const int marginAfter)
    : Schedule(SCHEDULE_TYPE_BY_PATTERN, channelId, recordingsToKeep, marginBefore, marginAfter),
    m_genreMask(genreMask), 
    m_keyphrase(keyphrase)
{

}

ByPatternSchedule::~ByPatternSchedule()
{

}

long ByPatternSchedule::GetGenreMask() 
{ 
  return m_genreMask; 
}

std::string& ByPatternSchedule::GetKeyphrase() 
{ 
  return m_keyphrase; 
}

AddScheduleRequest::AddScheduleRequest()
{

}

AddScheduleRequest::~AddScheduleRequest()
{

}


AddManualScheduleRequest::AddManualScheduleRequest(const std::string& channelId, const long startTime, const long duration, const long dayMask, const std::string& title, const int recordingsToKeep, const int marginBefore, const int marginAfter)
    : ManualSchedule(channelId, startTime, duration, dayMask, title, recordingsToKeep, marginBefore, marginAfter), AddScheduleRequest(), Schedule(Schedule::SCHEDULE_TYPE_MANUAL, channelId, recordingsToKeep, marginBefore, marginAfter)
{

}

AddManualScheduleRequest::~AddManualScheduleRequest()
{

}

AddScheduleByEpgRequest::AddScheduleByEpgRequest(const std::string& channelId, const std::string& programId, const bool repeat, const bool newOnly, 
    const bool recordSeriesAnytime, const int recordingsToKeep, const int marginBefore, const int marginAfter)
    : EpgSchedule(channelId, programId, repeat, newOnly, recordSeriesAnytime, recordingsToKeep, marginBefore, marginAfter), AddScheduleRequest(), Schedule(Schedule::SCHEDULE_TYPE_BY_EPG, channelId, recordingsToKeep, marginBefore, marginAfter)
{

}

AddScheduleByEpgRequest::~AddScheduleByEpgRequest()
{

}

AddScheduleByPatternRequest::AddScheduleByPatternRequest(const std::string& channelId, const std::string& keyphrase, const long genremask, 
														 const int recordingsToKeep, const int marginBefore, const int marginAfter)
    : ByPatternSchedule(channelId, keyphrase, genremask, recordingsToKeep, marginBefore, marginAfter), AddScheduleRequest(), Schedule(Schedule::SCHEDULE_TYPE_BY_PATTERN, channelId, recordingsToKeep, marginBefore, marginAfter)
{

}

AddScheduleByPatternRequest::~AddScheduleByPatternRequest()
{

}

bool AddScheduleRequestSerializer::WriteObject(std::string& serializedData, AddScheduleRequest& objectGraph)
{
  tinyxml2::XMLElement* rootElement = PrepareXmlDocumentForObjectSerialization("schedule");
  
  if (!objectGraph.UserParameter.empty()) {
    rootElement->InsertEndChild(Util::CreateXmlElementWithText(&GetXmlDocument(), "user_param", objectGraph.UserParameter));
  }

  if (objectGraph.ForceAdd) {
    rootElement->InsertEndChild(Util::CreateXmlElementWithText(&GetXmlDocument(), "force_add", objectGraph.ForceAdd));
  }

  rootElement->InsertEndChild(Util::CreateXmlElementWithText(&GetXmlDocument(), "margine_before", objectGraph.MarginBefore));
  rootElement->InsertEndChild(Util::CreateXmlElementWithText(&GetXmlDocument(), "margine_after", objectGraph.MarginAfter));

  if (objectGraph.GetScheduleType() == objectGraph.SCHEDULE_TYPE_MANUAL) {
    AddManualScheduleRequest& addManualScheduleRequest = (AddManualScheduleRequest&)objectGraph;

    tinyxml2::XMLElement* manualElement = GetXmlDocument().NewElement("manual");
    rootElement->InsertEndChild(manualElement);

    manualElement->InsertEndChild(Util::CreateXmlElementWithText(&GetXmlDocument(), "channel_id", addManualScheduleRequest.GetChannelID()));

    if (!addManualScheduleRequest.Title.empty()) {
      manualElement->InsertEndChild(Util::CreateXmlElementWithText(&GetXmlDocument(), "title", addManualScheduleRequest.Title));
    }

    manualElement->InsertEndChild(Util::CreateXmlElementWithText(&GetXmlDocument(), "start_time", addManualScheduleRequest.GetStartTime()));
    manualElement->InsertEndChild(Util::CreateXmlElementWithText(&GetXmlDocument(), "duration", addManualScheduleRequest.GetDuration()));
    manualElement->InsertEndChild(Util::CreateXmlElementWithText(&GetXmlDocument(), "day_mask", addManualScheduleRequest.GetDayMask()));
    manualElement->InsertEndChild(Util::CreateXmlElementWithText(&GetXmlDocument(), "recordings_to_keep", addManualScheduleRequest.RecordingsToKeep));
  }
    
  if (objectGraph.GetScheduleType() == objectGraph.SCHEDULE_TYPE_BY_EPG) {
    AddScheduleByEpgRequest& addScheduleByEpgRequest = (AddScheduleByEpgRequest&)objectGraph;

    tinyxml2::XMLElement* byEpgElement = GetXmlDocument().NewElement("by_epg");
    rootElement->InsertEndChild(byEpgElement);
    byEpgElement->InsertEndChild(Util::CreateXmlElementWithText(&GetXmlDocument(), "channel_id", addScheduleByEpgRequest.GetChannelID()));
    byEpgElement->InsertEndChild(Util::CreateXmlElementWithText(&GetXmlDocument(), "program_id", addScheduleByEpgRequest.GetProgramID()));

    if (addScheduleByEpgRequest.Repeat) {
      byEpgElement->InsertEndChild(Util::CreateXmlElementWithText(&GetXmlDocument(), "repeat", addScheduleByEpgRequest.Repeat));
    }

    if (addScheduleByEpgRequest.NewOnly) {
      byEpgElement->InsertEndChild(Util::CreateXmlElementWithText(&GetXmlDocument(), "new_only", addScheduleByEpgRequest.NewOnly));
    }

    if (!addScheduleByEpgRequest.RecordSeriesAnytime) {
      byEpgElement->InsertEndChild(Util::CreateXmlElementWithText(&GetXmlDocument(), "record_series_anytime", addScheduleByEpgRequest.RecordSeriesAnytime));
    }

    byEpgElement->InsertEndChild(Util::CreateXmlElementWithText(&GetXmlDocument(), "recordings_to_keep", addScheduleByEpgRequest.RecordingsToKeep));
  }    

  if (objectGraph.GetScheduleType() == objectGraph.SCHEDULE_TYPE_BY_PATTERN) {
    AddScheduleByPatternRequest& addByPatternScheduleRequest = (AddScheduleByPatternRequest&)objectGraph;

    tinyxml2::XMLElement* manualElement = GetXmlDocument().NewElement("by_pattern");
    rootElement->InsertEndChild(manualElement);

    manualElement->InsertEndChild(Util::CreateXmlElementWithText(&GetXmlDocument(), "channel_id", addByPatternScheduleRequest.GetChannelID()));

    manualElement->InsertEndChild(Util::CreateXmlElementWithText(&GetXmlDocument(), "recordings_to_keep", addByPatternScheduleRequest.RecordingsToKeep));
    manualElement->InsertEndChild(Util::CreateXmlElementWithText(&GetXmlDocument(), "genre_mask", addByPatternScheduleRequest.GetGenreMask()));
	manualElement->InsertEndChild(Util::CreateXmlElementWithText(&GetXmlDocument(), "key_phrase", addByPatternScheduleRequest.GetKeyphrase()));
  }

  tinyxml2::XMLPrinter* printer = new tinyxml2::XMLPrinter();    
  GetXmlDocument().Accept(printer);
  serializedData = std::string(printer->CStr());
  
  return true;
}

GetSchedulesRequest::GetSchedulesRequest()
{

}

GetSchedulesRequest::~GetSchedulesRequest()
{

}

bool GetSchedulesRequestSerializer::WriteObject(std::string& serializedData, GetSchedulesRequest& objectGraph)
{
  PrepareXmlDocumentForObjectSerialization("schedules");    
  tinyxml2::XMLPrinter* printer = new tinyxml2::XMLPrinter();    
  GetXmlDocument().Accept(printer);
  serializedData = std::string(printer->CStr());
  
  return true;
}

StoredManualSchedule::StoredManualSchedule(const std::string& id, const std::string& channelId, const long startTime, const long duration, const long dayMask, const std::string& title)
  : ManualSchedule(id, channelId, startTime, duration, dayMask, title), Schedule(Schedule::SCHEDULE_TYPE_MANUAL,id, channelId)
{

}

StoredManualSchedule::~StoredManualSchedule()
{ }

StoredManualScheduleList::~StoredManualScheduleList()
{
  for (std::vector<StoredManualSchedule*>::const_iterator it = begin(); it < end(); it++) 
  {
    delete (*it);
  }
}

StoredEpgSchedule::StoredEpgSchedule(const std::string& id, const std::string& channelId, const std::string& programId, const bool repeat, const bool newOnly, const bool recordSeriesAnytime)
  : EpgSchedule(id, channelId, programId, repeat, newOnly, recordSeriesAnytime), Schedule(Schedule::SCHEDULE_TYPE_BY_EPG,id, channelId)
{

}

StoredEpgSchedule::~StoredEpgSchedule()
{ }

StoredEpgScheduleList::~StoredEpgScheduleList()
{
  for (std::vector<StoredEpgSchedule*>::const_iterator it = begin(); it < end(); it++) 
  {
    delete (*it);
  }
}

StoredByPatternSchedule::StoredByPatternSchedule(const std::string& id, const std::string& channelId, const std::string& keyPhrase, const long genreMask)
  : ByPatternSchedule(id, channelId, keyPhrase, genreMask), Schedule(Schedule::SCHEDULE_TYPE_BY_PATTERN, id, channelId)
{

}

StoredByPatternSchedule::~StoredByPatternSchedule()
{ }

StoredByPatternScheduleList::~StoredByPatternScheduleList()
{
  for (std::vector<StoredByPatternSchedule*>::const_iterator it = begin(); it < end(); it++) 
  {
    delete (*it);
  }
}

StoredSchedules::StoredSchedules()
{
  m_manualScheduleList = new StoredManualScheduleList();
  m_epgScheduleList = new StoredEpgScheduleList();
  m_bypatternScheduleList = new StoredByPatternScheduleList();
}

StoredSchedules::~StoredSchedules() 
{
  if (m_manualScheduleList) {
    delete m_manualScheduleList;
  }

  if (m_epgScheduleList) {
    delete m_epgScheduleList;
  }

  if (m_bypatternScheduleList) {
    delete m_bypatternScheduleList;
  }
}

StoredManualScheduleList& StoredSchedules::GetManualSchedules() 
{
  return *m_manualScheduleList;
}

StoredEpgScheduleList& StoredSchedules::GetEpgSchedules() 
{
  return *m_epgScheduleList;
}

StoredByPatternScheduleList& StoredSchedules::GetByPatternSchedules()
{
  return *m_bypatternScheduleList;
}

bool GetSchedulesResponseSerializer::ReadObject(StoredSchedules& object, const std::string& xml)
{
  tinyxml2::XMLDocument& doc = GetXmlDocument();
    
  if (doc.Parse(xml.c_str()) == tinyxml2::XML_SUCCESS) {
    tinyxml2::XMLElement* elRoot = doc.FirstChildElement("schedules");
    GetSchedulesResponseXmlDataDeserializer* xmlDataDeserializer = new GetSchedulesResponseXmlDataDeserializer(*this, object);
    elRoot->Accept(xmlDataDeserializer);
    delete xmlDataDeserializer;
    
    return true;
  }

  return false;
}

GetSchedulesResponseSerializer::GetSchedulesResponseXmlDataDeserializer::GetSchedulesResponseXmlDataDeserializer(GetSchedulesResponseSerializer& parent, StoredSchedules& storedSchedules) 
  : m_parent(parent), 
    m_storedSchedules(storedSchedules) 
{ }

GetSchedulesResponseSerializer::GetSchedulesResponseXmlDataDeserializer::~GetSchedulesResponseXmlDataDeserializer()
{ }

bool GetSchedulesResponseSerializer::GetSchedulesResponseXmlDataDeserializer::VisitEnter(const tinyxml2::XMLElement& element, const tinyxml2::XMLAttribute* attribute)
{
  if (strcmp(element.Name(), "schedule") == 0) 
  {    	
    std::string scheduleId = Util::GetXmlFirstChildElementText(&element, "schedule_id");
    std::string userParam = Util::GetXmlFirstChildElementText(&element, "user_param");
    bool forceadd = Util::GetXmlFirstChildElementTextAsBoolean(&element, "force_add");

	int margin_before = Util::GetXmlFirstChildElementTextAsInt(&element, "margine_before");
	int margin_after = Util::GetXmlFirstChildElementTextAsInt(&element, "margine_after");

    if (m_parent.HasChildElement(element, "by_epg")) {
      tinyxml2::XMLElement* epg = (tinyxml2::XMLElement*)(&element)->FirstChildElement("by_epg");

      std::string channelid = Util::GetXmlFirstChildElementText(epg, "channel_id");
      std::string programid = Util::GetXmlFirstChildElementText(epg, "program_id");		  
      
	  if (programid.size() > 0)
	  {
		  StoredEpgSchedule* s = new StoredEpgSchedule(scheduleId, channelid, programid);
		  s->ForceAdd = forceadd;
		  s->UserParameter = userParam;
		  s->MarginBefore = margin_before;
		  s->MarginAfter = margin_after;
	      
		  if (m_parent.HasChildElement(*epg, "repeat")) {
			s->Repeat = Util::GetXmlFirstChildElementTextAsBoolean(epg, "repeat");
		  }

		  if (m_parent.HasChildElement(*epg, "new_only")) {
			s->NewOnly = Util::GetXmlFirstChildElementTextAsBoolean(epg, "new_only");
		  }

		  if (m_parent.HasChildElement(*epg, "record_series_anytime")) {
			s->RecordSeriesAnytime = Util::GetXmlFirstChildElementTextAsBoolean(epg, "record_series_anytime");
		  }

		  s->RecordingsToKeep = Util::GetXmlFirstChildElementTextAsInt(epg, "recordings_to_keep");

		  tinyxml2::XMLElement* program_el = epg->FirstChildElement("program");
  		  if (program_el != NULL)
		  {    
			Program* p = new Program();
			ProgramSerializer::Deserialize((XmlObjectSerializer<Response>&)m_parent, *program_el, *p);
			s->program_name_ = p->GetTitle();
			delete p;
		  }

		  m_storedSchedules.GetEpgSchedules().push_back(s);
	  }
    }
    if (m_parent.HasChildElement(element, "manual")) {
      tinyxml2::XMLElement* manual = (tinyxml2::XMLElement*)(&element)->FirstChildElement("manual");

      std::string channelId = Util::GetXmlFirstChildElementText(manual, "channel_id");
      std::string title = Util::GetXmlFirstChildElementText(manual, "title");
      long startTime = Util::GetXmlFirstChildElementTextAsLong(manual, "start_time");
      int duration = Util::GetXmlFirstChildElementTextAsLong(manual, "duration");
      long dayMask = Util::GetXmlFirstChildElementTextAsLong(manual, "day_mask");

	  if (channelId.size() > 0)
	  {
		  StoredManualSchedule* s = new StoredManualSchedule(scheduleId, channelId, startTime, duration, dayMask, title);
		  s->ForceAdd = forceadd;
		  s->UserParameter = userParam;
		  s->MarginBefore = margin_before;
		  s->MarginAfter = margin_after;

		  s->RecordingsToKeep = Util::GetXmlFirstChildElementTextAsInt(manual, "recordings_to_keep");

		  m_storedSchedules.GetManualSchedules().push_back(s);
	  }
    }

    if (m_parent.HasChildElement(element, "by_pattern")) {
      tinyxml2::XMLElement* manual = (tinyxml2::XMLElement*)(&element)->FirstChildElement("by_pattern");

      std::string channelId = Util::GetXmlFirstChildElementText(manual, "channel_id");
      std::string keyphrase = Util::GetXmlFirstChildElementText(manual, "key_phrase");
      long genreMask = Util::GetXmlFirstChildElementTextAsLong(manual, "genre_mask");

	  if (keyphrase.size() > 0 || genreMask != 0)
	  {
		  StoredByPatternSchedule* s = new StoredByPatternSchedule(scheduleId, channelId, keyphrase, genreMask);
		  s->ForceAdd = forceadd;
		  s->UserParameter = userParam;
		  s->MarginBefore = margin_before;
		  s->MarginAfter = margin_after;

		  s->RecordingsToKeep = Util::GetXmlFirstChildElementTextAsInt(manual, "recordings_to_keep");

		  m_storedSchedules.GetByPatternSchedules().push_back(s);
	  }
    }

    return false;
  }

  return true;
}

UpdateScheduleRequest::UpdateScheduleRequest(const std::string& scheduleId, const bool newOnly, const bool recordSeriesAnytime, const int recordingsToKeep, const int margin_before, const int margin_after) 
  : m_scheduleId(scheduleId),
    m_newOnly(newOnly),
    m_recordSeriesAnytime(recordSeriesAnytime),
    m_recordingsToKeep(recordingsToKeep),
	m_margin_before(margin_before),
	m_margin_after(margin_after)
{

}

UpdateScheduleRequest::~UpdateScheduleRequest()
{

}

std::string& UpdateScheduleRequest::GetScheduleID() 
{ 
  return m_scheduleId; 
}

bool UpdateScheduleRequest::IsNewOnly() 
{ 
  return m_newOnly; 
}

bool UpdateScheduleRequest::WillRecordSeriesAnytime() 
{ 
  return m_recordSeriesAnytime; 
}

int UpdateScheduleRequest::GetRecordingsToKeep() 
{ 
  return m_recordingsToKeep; 
}

int UpdateScheduleRequest::GetMarginBefore()
{
	return m_margin_before;
}

int UpdateScheduleRequest::GetMarginAfter()
{
	return m_margin_after;
}

bool UpdateScheduleRequestSerializer::WriteObject(std::string& serializedData, UpdateScheduleRequest& objectGraph)
{
  tinyxml2::XMLElement* rootElement = PrepareXmlDocumentForObjectSerialization("update_schedule");
  rootElement->InsertEndChild(Util::CreateXmlElementWithText(&GetXmlDocument(), "schedule_id", objectGraph.GetScheduleID()));
  rootElement->InsertEndChild(Util::CreateXmlElementWithText(&GetXmlDocument(), "new_only", objectGraph.IsNewOnly()));
  rootElement->InsertEndChild(Util::CreateXmlElementWithText(&GetXmlDocument(), "record_series_anytime", objectGraph.WillRecordSeriesAnytime()));
  rootElement->InsertEndChild(Util::CreateXmlElementWithText(&GetXmlDocument(), "recordings_to_keep", objectGraph.GetRecordingsToKeep()));
  rootElement->InsertEndChild(Util::CreateXmlElementWithText(&GetXmlDocument(), "margine_before", objectGraph.GetMarginBefore()));
  rootElement->InsertEndChild(Util::CreateXmlElementWithText(&GetXmlDocument(), "margine_after", objectGraph.GetMarginAfter()));

  tinyxml2::XMLPrinter* printer = new tinyxml2::XMLPrinter();    
  GetXmlDocument().Accept(printer);
  serializedData = std::string(printer->CStr());
  
  return true;
}

RemoveScheduleRequest::RemoveScheduleRequest(const std::string& scheduleId) 
  : m_scheduleId(scheduleId)
{

}

RemoveScheduleRequest::~RemoveScheduleRequest()
{

}

std::string& RemoveScheduleRequest::GetScheduleID() 
{ 
  return m_scheduleId; 
}

bool RemoveScheduleRequestSerializer::WriteObject(std::string& serializedData, RemoveScheduleRequest& objectGraph)
{
  tinyxml2::XMLElement* rootElement = PrepareXmlDocumentForObjectSerialization("remove_schedule");
  rootElement->InsertEndChild(Util::CreateXmlElementWithText(&GetXmlDocument(), "schedule_id", objectGraph.GetScheduleID()));
    
  tinyxml2::XMLPrinter* printer = new tinyxml2::XMLPrinter();    
  GetXmlDocument().Accept(printer);
  serializedData = std::string(printer->CStr());
 
  return true;
}
