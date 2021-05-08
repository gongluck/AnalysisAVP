/*
 * @Author: gongluck
 * @Date: 2020-10-03 15:36:42
 * @Last Modified by: gongluck
 * @Last Modified time: 2020-10-03 15:37:43
 */

#include <iostream>
#include <fstream>
#include <thread>

#ifdef _WIN32
#include <Windows.h>
#endif

#include "../../../analysis/flv/flv.h"

extern "C"
{
#include "rtmp.h"
}

#define DODELAY 1
const int presentime = 1000;

int main(int argc, char* argv[])
{
	std::cout << "librtmp example" << std::endl;

	std::cout << "Usage : " << "thisfile flvfile pushurl." << std::endl;

	if (argc < 3)
	{
		std::cerr << "please see the usage message." << std::endl;
		return -1;
	}
	std::ifstream in(argv[1], std::ios::binary);
	if (in.fail())
	{
		std::cerr << "can not open file " << argv[1] << std::endl;
		return -1;
	}

#ifdef _WIN32
	WORD version;
	WSADATA wsaData;
	version = MAKEWORD(1, 1);
	WSAStartup(version, &wsaData);
#endif

	RTMP* rtmp = RTMP_Alloc();
	RTMP_Init(rtmp);
	auto rtmpres = RTMP_SetupURL(rtmp, argv[2]);
	RTMP_EnableWrite(rtmp);//推流要设置写
	rtmpres = RTMP_Connect(rtmp, nullptr);
	rtmpres = RTMP_ConnectStream(rtmp, 0);

	FLVHEADER flvheader = { 0 };
	in.read(reinterpret_cast<char*>(&flvheader), sizeof(flvheader));
	std::cout << flvheader << std::endl;

	FLVINT32 presize = { 0 };
	in.read(reinterpret_cast<char*>(&presize), 4);

	RTMPPacket packet = { 0 };
	auto begintime = RTMP_GetTime();
	uint32_t timestamp = 0, now = 0;
	while (true)
	{
		FLVTAGHEADER tagheader = { 0 };
		if (!in.read(reinterpret_cast<char*>(&tagheader), sizeof(tagheader)))
		{
			break;
		}

		auto datalen = FLVINT24TOINT(tagheader.datalen);
		auto data = new char[sizeof(FLVTAGHEADER) + datalen + sizeof(presize)];
		memcpy(data, &tagheader, sizeof(FLVTAGHEADER));
		if (!in.read(data + sizeof(FLVTAGHEADER), static_cast<uint64_t>(datalen) + sizeof(presize)))
		{
			break;
		}

		timestamp = FLVINT32TOINT(tagheader.timestamp);
#if DODELAY
		CALCTIME :
		now = RTMP_GetTime() - begintime;
		if (now < timestamp + presentime)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds((timestamp + presentime - now) / 2));
			goto CALCTIME;
		}
#endif

		//auto len = sizeof(FLVTAGHEADER) + datalen + sizeof(presize);
		//rtmpres = RTMP_Write(rtmp, data, len);//tagheader + data + presize
		//if (rtmpres < len)
		//{
		//	std::cout << rtmpres << "\t" << len << std::endl;
		//	break;
		//}

		rtmpres = RTMPPacket_Alloc(&packet, datalen);//分配packet的buffer
		packet.m_nChannel = 0x03;
		packet.m_headerType = RTMP_PACKET_SIZE_MEDIUM;
		packet.m_packetType = tagheader.flvtagtype;
		packet.m_nTimeStamp = timestamp;
		packet.m_nInfoField2 = 0;
		packet.m_hasAbsTimestamp = 0;
		memcpy(packet.m_body, data + sizeof(FLVTAGHEADER), datalen);
		packet.m_nBodySize = datalen;
		rtmpres = RTMP_SendPacket(rtmp, &packet, 0);
		RTMPPacket_Free(&packet);
		if (rtmpres <= 0)
		{
			break;
		}

		//std::cout << "timestamp " << timestamp << "ms" << std::endl;
		delete[]data;
	}

	RTMP_Close(rtmp);
	RTMP_Free(rtmp);
	in.close();

#ifdef _WIN32
	WSACleanup();
#endif

	return 0;
}
