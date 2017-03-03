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

#include "request.h"
#include "response.h"
#include "xml_object_serializer.h"

using namespace dvblinkremote;
using namespace dvblinkremoteserialization;

GetTimeshiftStatsRequest::GetTimeshiftStatsRequest(long channelHandle) :
  m_channelHandle(channelHandle)
{ }

GetTimeshiftStatsRequest::~GetTimeshiftStatsRequest()
{ }

long GetTimeshiftStatsRequest::GetChannelHandle()
{
  return m_channelHandle;
}

TimeshiftStats::TimeshiftStats() :
  maxBufferLength(0), curBufferLength(0), curPosBytes(0), bufferDurationSec(0), curPosSec(0)
{ }

TimeshiftStats::TimeshiftStats(TimeshiftStats& timeshiftStats) :
  maxBufferLength(timeshiftStats.maxBufferLength), curBufferLength(timeshiftStats.curBufferLength), curPosBytes(timeshiftStats.curPosBytes), 
  bufferDurationSec(timeshiftStats.bufferDurationSec), curPosSec(timeshiftStats.curPosSec)
{ }

TimeshiftStats::~TimeshiftStats()
{ }

bool GetTimeshiftStatsRequestSerializer::WriteObject(std::string& serializedData, GetTimeshiftStatsRequest& objectGraph)
{
  tinyxml2::XMLElement* rootElement = PrepareXmlDocumentForObjectSerialization("timeshift_status"); 

  rootElement->InsertEndChild(Util::CreateXmlElementWithText(&GetXmlDocument(), "channel_handle", objectGraph.GetChannelHandle()));

  tinyxml2::XMLPrinter* printer = new tinyxml2::XMLPrinter();    
  GetXmlDocument().Accept(printer);
  serializedData = std::string(printer->CStr());
  
  return true;
}

bool TimeshiftStatsSerializer::ReadObject(TimeshiftStats& object, const std::string& xml)
{
  tinyxml2::XMLDocument& doc = GetXmlDocument();
    
  if (doc.Parse(xml.c_str()) == tinyxml2::XML_SUCCESS) {
    tinyxml2::XMLElement* elRoot = doc.FirstChildElement("timeshift_status");
    object.maxBufferLength = Util::GetXmlFirstChildElementTextAsLongLong(elRoot, "max_buffer_length");
    object.curBufferLength = Util::GetXmlFirstChildElementTextAsLongLong(elRoot, "buffer_length");
    object.curPosBytes = Util::GetXmlFirstChildElementTextAsLongLong(elRoot, "cur_pos_bytes");
    object.bufferDurationSec = Util::GetXmlFirstChildElementTextAsLongLong(elRoot, "buffer_duration");
    object.curPosSec = Util::GetXmlFirstChildElementTextAsLongLong(elRoot, "cur_pos_sec");
    return true;
  }

  return false;
}

TimeshiftSeekRequest::TimeshiftSeekRequest(long channelHandle, bool byBytes, long long offset, long whence) :
  m_channelHandle(channelHandle), m_offset(offset), m_whence(whence)
{
  m_seekTypeSwitch = byBytes ? 0 : 1;
}

TimeshiftSeekRequest::~TimeshiftSeekRequest()
{ }

bool TimeshiftSeekRequestSerializer::WriteObject(std::string& serializedData, TimeshiftSeekRequest& objectGraph)
{
  tinyxml2::XMLElement* rootElement = PrepareXmlDocumentForObjectSerialization("timeshift_seek");

  rootElement->InsertEndChild(Util::CreateXmlElementWithText(&GetXmlDocument(), "channel_handle", objectGraph.m_channelHandle));
  rootElement->InsertEndChild(Util::CreateXmlElementWithText(&GetXmlDocument(), "type", objectGraph.m_seekTypeSwitch));
  rootElement->InsertEndChild(Util::CreateXmlElementWithText(&GetXmlDocument(), "offset", objectGraph.m_offset));
  rootElement->InsertEndChild(Util::CreateXmlElementWithText(&GetXmlDocument(), "whence", objectGraph.m_whence));

  tinyxml2::XMLPrinter* printer = new tinyxml2::XMLPrinter();
  GetXmlDocument().Accept(printer);
  serializedData = std::string(printer->CStr());

  return true;
}

