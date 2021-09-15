/*
 *  Copyright (C) 2005-2021 Team Kodi (https://kodi.tv)
 *  Copyright (C) 2012 Palle Ehmsen(Barcode Madness) (http://www.barcodemadness.com)
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#include "HttpPostClient.h"

#include "base64.h"

#include "Socket.h"

#include <kodi/Filesystem.h>
#include <kodi/General.h>

using namespace dvblink;
using namespace dvblinkremotehttp;

/* Converts a hex character to its integer value */
char from_hex(char ch)
{
  return isdigit(ch) ? ch - '0' : tolower(ch) - 'a' + 10;
}

/* Converts an integer value to its hex character*/
char to_hex(char code)
{
  static char hex[] = "0123456789abcdef";
  return hex[code & 15];
}

/* Returns a url-encoded version of str */
/* IMPORTANT: be sure to free() the returned string after use */
char* url_encode(const char* str)
{
  char *pstr = (char*)str, *buf = (char*)malloc(strlen(str) * 3 + 1), *pbuf = buf;
  while (*pstr)
  {
    if (isalnum(*pstr) || *pstr == '-' || *pstr == '_' || *pstr == '.' || *pstr == '~')
      *pbuf++ = *pstr;
    else if (*pstr == ' ')
      *pbuf++ = '+';
    else
      *pbuf++ = '%', *pbuf++ = to_hex(*pstr >> 4), *pbuf++ = to_hex(*pstr & 15);
    pstr++;
  }
  *pbuf = '\0';
  return buf;
}

HttpPostClient::HttpPostClient(const std::string& server,
                               const int serverport,
                               const std::string& username,
                               const std::string& password)
{
  m_server = server;
  m_serverport = serverport;
  m_username = username;
  m_password = password;
}

int HttpPostClient::SendPostRequest(HttpWebRequest& request)
{
  int ret_code = -100;
  std::string buffer;
  std::string message;
  char content_header[100];

  buffer.append("POST /mobile/ HTTP/1.0\r\n");
  sprintf(content_header, "Host: %s:%d\r\n", m_server.c_str(), (int)m_serverport);
  buffer.append(content_header);
  buffer.append("Content-Type: application/x-www-form-urlencoded\r\n");
  if (m_username.compare("") != 0)
  {
    sprintf(content_header, "%s:%s", m_username.c_str(), m_password.c_str());
    sprintf(content_header, "Authorization: Basic %s\r\n",
            base64_encode((const char*)content_header, strlen(content_header)).c_str());
    buffer.append(content_header);
  }
  sprintf(content_header, "Content-Length: %ld\r\n", request.ContentLength);
  buffer.append(content_header);
  buffer.append("\r\n");
  buffer.append(request.GetRequestData());

  Socket sock;

  if (!sock.create())
  {
    return -101;
  }

  int connect_timeout_ms = 15 * 1000; //15 seconds
  if (!sock.connect(m_server, (int)m_serverport))
  {
    return -101;
  }

  ssize_t written_length = sock.send(buffer.c_str(), buffer.length());

  if (written_length != buffer.length())
  {
    sock.close();
    return -102;
  }

  int read_timeout_ms = 30 * 1000; //30 seconds
  const int read_buffer_size = 4096;
  char read_buffer[read_buffer_size];
  ssize_t read_size = 0;
  std::string response;
  while ((read_size = sock.receive(read_buffer, read_buffer_size, 0, read_timeout_ms)) > 0)
    response.append(read_buffer, read_buffer + read_size);

  sock.close();

  if (response.size() > 0)
  {
    //header
    std::string::size_type n = response.find("\r\n");
    if (n != std::string::npos)
    {
      std::string header = response.substr(0, n);
      if (header.find("200 OK") != std::string::npos)
        ret_code = 200;
      if (header.find("401 Unauthorized") != std::string::npos)
        ret_code = -401;

      if (ret_code == 200)
      {
        //body
        const char* body_sep = "\r\n\r\n";
        n = response.find(body_sep);
        if (n != std::string::npos)
        {
          m_responseData.assign(response.c_str() + n + strlen(body_sep));
        }
        else
        {
          ret_code = -105;
        }
      }
    }
    else
    {
      ret_code = -104;
    }
  }
  else
  {
    ret_code = -102;
  }

  //TODO: Use xbmc file code when it allows to post content-type application/x-www-form-urlencoded and authentication
  /*
  kodi::vfs::CFile file;
  if (file.OpenFileForWrite(request.GetUrl()))
  {
    int rc = file.Write(buffer.c_str(), buffer.length());
    if (rc >= 0)
    {
      std::string result;
      result.clear();
      std::string buffer;
      while (file.ReadLine(buffer))
        result.append(buffer);

    }
    else
    {
      kodi::Log(ADDON_LOG_ERROR, "can not write to %s",request.GetUrl().c_str());
    }

    file.Close();
  }
  else
  {
    kodi::Log(ADDON_LOG_ERROR, "can not open %s for write", request.GetUrl().c_str());
  }

  */

  return ret_code;
}

bool HttpPostClient::SendRequest(HttpWebRequest& request)
{
  m_lastReqeuestErrorCode = SendPostRequest(request);
  return (m_lastReqeuestErrorCode == 200);
}

HttpWebResponse* HttpPostClient::GetResponse()
{
  if (m_lastReqeuestErrorCode != 200)
  {
    return nullptr;
  }
  HttpWebResponse* response = new HttpWebResponse(200, m_responseData);
  return response;
}

void HttpPostClient::GetLastError(std::string& err)
{
}

void HttpPostClient::UrlEncode(const std::string& str, std::string& outEncodedStr)
{
  char* encodedstring = url_encode(str.c_str());
  outEncodedStr.append(encodedstring);
  free(encodedstring);
}
