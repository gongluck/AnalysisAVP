/*
 * @Author: gongluck
 * @Date: 2020-10-03 15:36:42
 * @Last Modified by: gongluck
 * @Last Modified time: 2021-07-18 15:24:49
 */

#include <iostream>
#include <fstream>
#include <thread>

#ifdef _WIN32
#include <Windows.h>
#endif

extern "C"
{
#include "rtmp.h"
}

int main(int argc, char *argv[])
{
	std::cout << "librtmp pull example" << std::endl;
	std::cout << "Usage : "
			  << "thisfile rtmpurl flvfile." << std::endl;
	if (argc < 3)
	{
		std::cerr << "please see the usage message." << std::endl;
		return -1;
	}
	std::ofstream out(argv[2], std::ios::binary | std::ios::trunc);
	if (out.fail())
	{
		std::cerr << "can not open file " << argv[2] << std::endl;
		return -1;
	}
	std::cout << "pulling : " << argv[1] << std::endl;

#ifdef _WIN32
	WORD version;
	WSADATA wsaData;
	version = MAKEWORD(1, 1);
	WSAStartup(version, &wsaData);
#endif

	RTMP *rtmp = RTMP_Alloc();
	RTMP_Init(rtmp);
	auto rtmpres = RTMP_SetupURL(rtmp, argv[1]);
	rtmpres = RTMP_Connect(rtmp, nullptr);
	rtmpres = RTMP_ConnectStream(rtmp, 0);

	bool stop = false;
	std::thread th([&]
				   {
					   auto data = new char[1024];
					   auto rent = 0;
					   while (!stop)
					   {
						   rent = RTMP_Read(rtmp, data, 1024);
						   if (rent <= 0)
						   {
							   break;
						   }
						   out.write(data, rent);
					   }
					   delete[] data;
				   });

	std::cout << "input char to stop" << std::endl;
	std::cin.get();

	stop = true;
	if (th.joinable())
	{
		th.join();
	}

	RTMP_Close(rtmp);
	RTMP_Free(rtmp);
	out.close();

#ifdef _WIN32
	WSACleanup();
#endif

	return 0;
}
