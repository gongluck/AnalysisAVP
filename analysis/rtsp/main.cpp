/*
 * @Author: gongluck
 * @Date: 2023-02-22 16:03:29
 * @Last Modified by: gongluck
 * @Last Modified time: 2023-02-22 18:26:52
 */

#include <iostream>
#include <string>

#define RTSP_PORT 554
#define RTSP_RTP_PORT 61001
#define RTSP_RTCP_PORT 61002
#define SESSION "1000000"

#define CMD_OPTIONS "OPTIONS"
#define CMD_DESCRIBE "DESCRIBE"
#define CMD_SETUP "SETUP"
#define CMD_PLAY "PLAY"

#ifdef _WIN32
#include <winsock2.h>
#include <windows.h>
#pragma comment(lib, "ws2_32.lib")
int before_main(void)
{
  WSADATA wsaData;
  if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
  {
    std::cout << "init network error : " << GetLastError() << std::endl;
  }
  else
  {
    std::cout << "init network succeed." << std::endl;
  }
  return 0;
}
typedef int func();
#pragma data_seg(".CRT$XIU")
static func *before[] = {before_main};
#pragma data_seg()
#endif

int handle_cmd(SOCKET s, char *request)
{
  std::cout << ">>>>>>>>>>" << std::endl;
  std::cout << request << std::endl;

  char method[10] = {0};
  char url[512] = {0};
  char version[10] = {0};
  char CSeq[5] = {0};
  int rtp_port = 0;
  int rtcp_port = 0;
  char *line = request;
  do
  {
    line = strtok(line, "\r\n");
    if (line == nullptr)
    {
      break;
    }
    if (strstr(line, CMD_OPTIONS) ||
        strstr(line, CMD_DESCRIBE) ||
        strstr(line, CMD_SETUP) ||
        strstr(line, CMD_PLAY))
    {
        sscanf(line, "%s %s %s\r\n", method, url, version);
    }
    else if (strstr(line, "CSeq"))
    {
      sscanf(line, "CSeq: %s\r\n", CSeq);
    }
    else if (!strncmp(line, "Transport:", strlen("Transport:")))
    {
      sscanf(line, "Transport: RTP/AVP/UDP;unicast;client_port=%d-%d\r\n", &rtp_port, &rtcp_port);
    }
    line = nullptr;
  } while (true);

  std::string response;
  if (std::string(method) == CMD_OPTIONS)
  {
    response = "RTSP/1.0 200 OK\r\n";
    response.append("CSeq:");
    response.append(CSeq);
    response.append("\r\n");
    response.append("Public: OPTIONS, DESCRIBE, SETUP, PLAY\r\n");
    response.append("\r\n");
  }
  else if (std::string(method) == CMD_DESCRIBE)
  {
    std::string sdp =
        "v=0\r\n"
        "o=- 0 1 IN IP4 127.0.0.1\r\n"
        "t=0 0\r\n"
        "a=control:*\r\n"
        "m=video 0 RTP/AVP 96\r\n"
        "a=rtpmap:96 H264/90000\r\n";

    response = "RTSP/1.0 200 OK\r\n";
    response.append("CSeq:");
    response.append(CSeq);
    response.append("\r\n");
    response.append("Content-Base: ");
    response.append(url);
    response.append("\r\n");
    response.append("Content-type: application/sdp\r\n");
    response.append("Content-length: ");
    response.append(std::to_string(sdp.size()));
    response.append("\r\n");
    response.append("\r\n");
    response.append(sdp.data());
  }
  else if (std::string(method) == CMD_SETUP)
  {
    response = "RTSP/1.0 200 OK\r\n";
    response.append("CSeq:");
    response.append(CSeq);
    response.append("\r\n");
    response.append("Transport: RTP/AVP;unicast;client_port=");
    response.append(std::to_string(rtp_port));
    response.append("-");
    response.append(std::to_string(rtp_port + 1));
    response.append(";server_port=");
    response.append(std::to_string(RTSP_RTP_PORT));
    response.append("-");
    response.append(std::to_string(RTSP_RTCP_PORT));
    response.append("\r\n");
    response.append("Session: " SESSION "\r\n");
    response.append("\r\n");
  }
  else if (std::string(method) == CMD_PLAY)
  {
    response = "RTSP/1.0 200 OK\r\n";
    response.append("CSeq:");
    response.append(CSeq);
    response.append("\r\n");
    response.append("Range: npt=0.000-\r\n");
    response.append("Session: " SESSION "; timeout = 10\r\n");
    response.append("\r\n");
  }
  if (response.size() > 0)
  {
    std::cout << "<<<<<<<<<<" << std::endl;
    std::cout << response << std::endl;
    auto len = send(s, response.data(), response.size(), 0);
  }

  return 0;
}

int main(int argc, char *argv[])
{
  std::cout << "rtsp server analysis" << std::endl;

  std::cout << "Usage : "
            << "thisfile." << std::endl;

  if (argc < 1)
  {
    std::cerr << "please see the usage message." << std::endl;
    return -1;
  }

  int ret = 0;
  auto listenfd = socket(AF_INET, SOCK_STREAM, 0);
  struct sockaddr_in serveraddr;
  serveraddr.sin_family = AF_INET;
  serveraddr.sin_port = htons(RTSP_PORT);
  serveraddr.sin_addr.s_addr = INADDR_ANY;
  int serveraddr_len = sizeof(serveraddr);
  ret = bind(listenfd, (struct sockaddr *)&serveraddr, serveraddr_len);
  ret = listen(listenfd, 10);

  struct sockaddr_in clientaddr;
  int clientaddr_len = sizeof(clientaddr);
  auto clientfd = accept(listenfd, (struct sockaddr *)&clientaddr, &clientaddr_len);
  if (clientfd == INVALID_SOCKET)
  {
    std::cout << "accept error : " << GetLastError()
              << std::endl;
    exit(-1);
  }
  std::cout << inet_ntoa(clientaddr.sin_addr) << ":" << ntohs(clientaddr.sin_port) << std::endl;

  char buf[1000] = {0};
  while (true)
  {
    auto len = recv(clientfd, buf, sizeof(buf), 0);
    if (len > 0)
    {
      handle_cmd(clientfd, buf);
    }
    else
    {
      break;
    }
  }

  return 0;
}
