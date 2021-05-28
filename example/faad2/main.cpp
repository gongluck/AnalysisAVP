/*
 * @Author: gongluck 
 * @Date: 2021-05-28 17:06:35 
 * @Last Modified by:   gongluck 
 * @Last Modified time: 2021-05-28 17:06:35 
 */

#include <iostream>
#include <fstream>
#include "faad.h"
#undef ADTS
#include "aac.h"

int main(int argc, char *argv[])
{
	std::cout << "faac demo" << std::endl;
	std::cout << "Usage : "
			  << "thisfilename aacfile outfile" << std::endl;
	if (argc < 3)
	{
		std::cerr << "please see the usage message." << std::endl;
		return -1;
	}
	std::ifstream aacin(argv[1], std::ios::binary);
	if (aacin.fail())
	{
		std::cerr << "can not open file " << argv[1] << std::endl;
		return -1;
	}
	std::ofstream outfile(argv[2], std::ios::binary);

	faacDecHandle hdecoder = faacDecOpen();
	// Get the current config
	faacDecConfiguration *config = faacDecGetCurrentConfiguration(hdecoder);
	// Set the new configuration
	auto ret = faacDecSetConfiguration(hdecoder, config);

	ADTS adts = { 0 };
	unsigned long samplerate = 0;
	unsigned char channels = 0;
	aacin.read(reinterpret_cast<char*>(&adts), sizeof(adts));
	uint16_t framelength = get_aac_frame_length(adts);
	aacin.seekg(-sizeof(adts), std::ios::cur);
	unsigned char* indata = static_cast<unsigned char*>(malloc(framelength));
	aacin.read(reinterpret_cast<char*>(indata), framelength);
	ret = faacDecInit(hdecoder, indata, framelength, &samplerate, &channels);
	std::cout << "samplerate : " << samplerate << " channels : " << static_cast<int>(channels) << " format : " << static_cast<int>(config->outputFormat) << std::endl;
	free(indata);
	indata = nullptr;

	faacDecFrameInfo info = { 0 };
	while (aacin.read(reinterpret_cast<char*>(&adts), sizeof(adts)))
	{
		framelength = get_aac_frame_length(adts);
		aacin.seekg(-sizeof(adts), std::ios::cur);
		
		unsigned char* indata = static_cast<unsigned char*>(malloc(framelength));
		aacin.read(reinterpret_cast<char*>(indata), framelength);
		auto outdata = faacDecDecode(hdecoder, &info, indata, framelength);
		if (info.error == 0)
		{
			auto outsize = info.samples * info.channels;
			outfile.write(reinterpret_cast<char*>(outdata), outsize);
		}
		free(indata);
		indata = nullptr;
	}

	faacDecClose(hdecoder);
	hdecoder = nullptr;

	free(indata);
	indata = nullptr;

	outfile.flush();
	outfile.close();
	aacin.close();

	return 0;
}