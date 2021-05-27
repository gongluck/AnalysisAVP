/*
 * @Author: gongluck 
 * @Date: 2021-05-27 20:53:35 
 * @Last Modified by: gongluck
 * @Last Modified time: 2021-05-27 21:38:05
 */

#include <iostream>
#include <fstream>
#include "faac.h"

int main(int argc, char *argv[])
{
	std::cout << "faac demo" << std::endl;
	std::cout << "Usage : "
			  << "thisfilename pcmfile samplerate bytepersample channels outfile" << std::endl;
	if (argc < 6)
	{
		std::cerr << "please see the usage message." << std::endl;
		return -1;
	}
	std::ifstream pcmin(argv[1], std::ios::binary);
	if (pcmin.fail())
	{
		std::cerr << "can not open file " << argv[1] << std::endl;
		return -1;
	}
	const unsigned long isamplerate = atol(argv[2]);
	const int ibytepersample = atoi(argv[3]);
	const unsigned int ichannels = atoi(argv[4]);
	std::ofstream outfile(argv[5], std::ios::binary);

	unsigned long inputsamples = 0;
	unsigned long maxoutputbytes = 0;
	faacEncHandle hencoder = faacEncOpen(isamplerate, ichannels, &inputsamples, &maxoutputbytes);

	faacEncConfiguration *config = faacEncGetCurrentConfiguration(hencoder);
	switch (ibytepersample)
	{
	case 16:
		config->inputFormat = FAAC_INPUT_16BIT;
		break;
	case 24:
		config->inputFormat = FAAC_INPUT_24BIT;
		break;
	case 32:
		config->inputFormat = FAAC_INPUT_32BIT;
		break;
	default:
		config->inputFormat = FAAC_INPUT_NULL;
		break;
	}
	config->outputFormat = ADTS_STREAM;
	config->bitRate = 1000;
	// 重置编码器的配置信息
	auto ret = faacEncSetConfiguration(hencoder, config);

	auto framesize = inputsamples * ibytepersample / 8;
	uint8_t *data = static_cast<uint8_t *>(malloc(framesize));
	unsigned char *outdata = static_cast<unsigned char *>(malloc(maxoutputbytes));
	while (pcmin.read(reinterpret_cast<char *>(data), framesize))
	{
		ret = faacEncEncode(hencoder, reinterpret_cast<int32_t *>(data), inputsamples, outdata, maxoutputbytes);
		if (ret > 0)
		{
			outfile.write(reinterpret_cast<char *>(outdata), ret);
		}
	}
	while (true)
	{
		ret = faacEncEncode(hencoder, nullptr, 0, outdata, maxoutputbytes);
		if (ret > 0)
		{
			outfile.write(reinterpret_cast<char *>(outdata), ret);
		}
		else
		{
			break;
		}
	}

	ret = faacEncClose(hencoder);
	hencoder = nullptr;

	free(data);
	data = nullptr;
	free(outdata);
	outdata = nullptr;

	outfile.flush();
	outfile.close();
	pcmin.close();

	return 0;
}