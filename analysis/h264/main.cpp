/*
 * @Author: gongluck 
 * @Date: 2020-11-02 17:08:28 
 * @Last Modified by:   gongluck 
 * @Last Modified time: 2020-11-02 17:08:28 
 */

#include "h264.h"

#include <iostream>
#include <iomanip>
#include <fstream>

int main(int argc, char *argv[])
{
	std::cout << "h264 analysis" << std::endl;

	std::cout << "Usage : "
			  << "h264 h264file." << std::endl;

	if (argc < 2)
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

	unsigned char c = 0;
	int32_t datalen = 0;
	int step = 0; //记录0x00的个数
	while (in.read(reinterpret_cast<char *>(&c), 1))
	{
		if (c == 0)
		{
			++step;
		}
		else if (c == 1 && step >= 2)
		{
			std::streamoff naluflagsize = step > 2 ? 4 : 3;
			datalen += (step > 3 ? step - 3 : 0);
			if (datalen != 0)
			{
				std::cout << std::string(50, '*').c_str() << std::endl;
				std::cout << "nalu size : " << datalen << std::endl;
				in.seekg(-naluflagsize - datalen, std::ios::cur);

				unsigned char *naludata = static_cast<unsigned char *>(malloc(datalen));
				if (!in.read(reinterpret_cast<char *>(naludata), datalen))
					break;

				NALHEADER *pnalheader = reinterpret_cast<NALHEADER *>(naludata);
				std::cout << *pnalheader << std::endl;

				std::ios::fmtflags f(std::cout.flags());
				for (int i = 0; i < datalen; ++i)
				{
					std::cout << std::setw(2) << std::setfill('0') << std::hex << static_cast<unsigned int>(naludata[i]) << " ";
				}
				std::cout << std::endl;
				std::cout.flags(f);
				in.seekg(naluflagsize, std::ios::cur);
			}

			datalen = 0;
			step = 0;
		}
		else
		{
			datalen += step + 1;
			step = 0;
		}
	}

	in.close();

	uint8_t data[] = {
		0x00,
		0x00,
		0x00,
		0x01,
		0x12,
		0x45,
		0x55,
		0x55,
		0x00,
		0x00,
		0x01,
		0x01,
		0x18,
		0x45,
		0x55,
		0x55,
		0x00,
		0x00,
		0x00,
		0x01,
		0x00,
		0x00,
		0x01,
		0x01,
	};
	int8_t nalstep = 0;
	int ret = 0;
	int tmp = ret;
	while (ret >= 0)
	{
		tmp = ret;
		ret = findnalu(data, ret, sizeof(data), &nalstep);
		std::ios::fmtflags f(std::cout.flags());
		for (int i = tmp; i > 0 && i < ret - nalstep; ++i)
		{
			std::cout << std::setw(2) << std::setfill('0') << std::hex << static_cast<unsigned int>(data[i]) << " ";
		}
		std::cout << std::endl;
		std::cout.flags(f);
	}

	return 0;
}
