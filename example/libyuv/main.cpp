/*
 * @Author: gongluck 
 * @Date: 2021-06-08 10:34:48 
 * @Last Modified by:   gongluck 
 * @Last Modified time: 2021-06-08 10:34:48 
 */

#include <iostream>
#include <fstream>
#include "libyuv.h"

int main(int argc, char *argv[])
{
	std::cout << "libyuv demo" << std::endl;
	std::cout << "Usage : "
			  << "thisfilename yuvfile width height outfile" << std::endl;
	if (argc < 5)
	{
		std::cerr << "please see the usage message." << std::endl;
		return -1;
	}
	std::ifstream yuvin(argv[1], std::ios::binary);
	if (yuvin.fail())
	{
		std::cerr << "can not open file " << argv[1] << std::endl;
		return -1;
	}
	const int width = atoi(argv[2]);
	const int height = atoi(argv[3]);
	std::ofstream outfile(argv[4], std::ios::binary);

	auto framesize = width * height * 3 / 2;
	uint8_t *indata = static_cast<uint8_t *>(malloc(framesize));
	uint8_t *outdata = static_cast<uint8_t *>(malloc(width * height * 4));
	while (yuvin.read(reinterpret_cast<char*>(indata), framesize))
	{
		libyuv::I420ToABGR(indata, width, 
			indata + width * height, width / 2, 
			indata + width * height * 5 / 4, width / 2, 
			outdata, width*4, 
			width, height);
		outfile.write(reinterpret_cast<char*>(outdata), width * height * 4);
	}
	free(indata);
	free(outdata);
	outfile.close();
	yuvin.close();

	return 0;
}