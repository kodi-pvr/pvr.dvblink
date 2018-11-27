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

GetObjectResumeInfoRequest::GetObjectResumeInfoRequest(const std::string& objectId) :
  m_objectId(objectId)
{ }

GetObjectResumeInfoRequest::~GetObjectResumeInfoRequest()
{ }

SetObjectResumeInfoRequest::SetObjectResumeInfoRequest(const std::string& objectId, int positionSec) :
  m_objectId(objectId), m_positionSec(positionSec)
{ }

SetObjectResumeInfoRequest::~SetObjectResumeInfoRequest()
{ }

ResumeInfo::ResumeInfo() :
  m_positionSec(0)
{ }

ResumeInfo::ResumeInfo(ResumeInfo& resumeInfo) :
  m_positionSec(resumeInfo.m_positionSec)
{ }

ResumeInfo::~ResumeInfo()
{ }

bool GetObjectResumeInfoRequestSerializer::WriteObject(std::string& serializedData, GetObjectResumeInfoRequest& objectGraph)
{
  tinyxml2::XMLElement* rootElement = PrepareXmlDocumentForObjectSerialization("get_resume_info"); 

  rootElement->InsertEndChild(Util::CreateXmlElementWithText(&GetXmlDocument(), "object_id", objectGraph.m_objectId));

  tinyxml2::XMLPrinter* printer = new tinyxml2::XMLPrinter();    
  GetXmlDocument().Accept(printer);
  serializedData = std::string(printer->CStr());
  
  return true;
}

bool SetObjectResumeInfoRequestSerializer::WriteObject(std::string& serializedData, SetObjectResumeInfoRequest& objectGraph)
{
  tinyxml2::XMLElement* rootElement = PrepareXmlDocumentForObjectSerialization("set_resume_info");

  rootElement->InsertEndChild(Util::CreateXmlElementWithText(&GetXmlDocument(), "object_id", objectGraph.m_objectId));
  rootElement->InsertEndChild(Util::CreateXmlElementWithText(&GetXmlDocument(), "pos", objectGraph.m_positionSec));

  tinyxml2::XMLPrinter* printer = new tinyxml2::XMLPrinter();
  GetXmlDocument().Accept(printer);
  serializedData = std::string(printer->CStr());

  return true;
}

bool ResumeInfoSerializer::ReadObject(ResumeInfo& object, const std::string& xml)
{
  tinyxml2::XMLDocument& doc = GetXmlDocument();
    
  if (doc.Parse(xml.c_str()) == tinyxml2::XML_SUCCESS) {
    tinyxml2::XMLElement* elRoot = doc.FirstChildElement("resume_info");
    object.m_positionSec = Util::GetXmlFirstChildElementTextAsInt(elRoot, "pos");
    return true;
  }

  return false;
}

