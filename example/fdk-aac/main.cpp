/*
 * @Author: gongluck 
 * @Date: 2021-05-29 19:58:23 
 * @Last Modified by:   gongluck 
 * @Last Modified time: 2021-05-29 19:58:23 
 */

#include <iostream>
#include <fstream>
#include "aac.h"
#include "aacenc_lib.h"
#include "aacdecoder_lib.h"

int main(int argc, char *argv[])
{
	std::cout << "fdk-aac demo" << std::endl;
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

	AACENCODER* hencoder = nullptr;
	AACENC_ERROR ret = aacEncOpen(&hencoder, 0, static_cast<CHANNEL_MODE>(ichannels));
	//设置编码规格
	ret = aacEncoder_SetParam(hencoder, AACENC_AOT, AOT_AAC_LC);
	//设置采样率
	ret = aacEncoder_SetParam(hencoder, AACENC_SAMPLERATE, isamplerate);
	//设置声道数
	ret = aacEncoder_SetParam(hencoder, AACENC_CHANNELMODE, static_cast<CHANNEL_MODE>(ichannels));
	//设置码率
	ret = aacEncoder_SetParam(hencoder, AACENC_BITRATE, 1000);
	//设置封装格式为ADTS
	ret = aacEncoder_SetParam(hencoder, AACENC_TRANSMUX, TT_MP4_ADTS);
	
	ret = aacEncEncode(hencoder, nullptr, nullptr, nullptr, nullptr);
	//编码的参数信息
	AACENC_InfoStruct einfo = { 0 };
	ret = aacEncInfo(hencoder, &einfo);

	AACENC_BufDesc inbuf = { 0 }, outbuf = { 0 };
	AACENC_InArgs inargs = { 0 };
	AACENC_OutArgs outargs = { 0 };
	//必须根据einfo.frameLength计算出每次输入的数据大小
	int framesize = einfo.frameLength * ichannels * ibytepersample;
	void* indata = malloc(framesize);
	void* outdata = malloc(einfo.maxOutBufBytes);
	int inidentifier = IN_AUDIO_DATA;
	int isize = ibytepersample;
	int outidentifier = OUT_BITSTREAM_DATA;
	int osize = 1;
	while (pcmin.read(reinterpret_cast<char*>(indata), framesize))
	{
		inargs.numInSamples = einfo.frameLength * ichannels;
		inbuf.numBufs = 1;
		inbuf.bufs = &indata;
		inbuf.bufferIdentifiers = &inidentifier;
		inbuf.bufSizes = &framesize;
		inbuf.bufElSizes = &isize;

		outbuf.numBufs = 1;
		outbuf.bufs = &outdata;
		outbuf.bufferIdentifiers = &outidentifier;
		outbuf.bufSizes = reinterpret_cast<int*>(&einfo.maxOutBufBytes);
		outbuf.bufElSizes = &osize;

		ret = aacEncEncode(hencoder, &inbuf, &outbuf, &inargs, &outargs);
		if(ret == AACENC_OK && outargs.numOutBytes > 0)
		{
			outfile.write(reinterpret_cast<char*>(outdata), outargs.numOutBytes);
		}
	}
	ret = aacEncClose(&hencoder);
	free(outdata);
	free(indata);
	outfile.close();
	pcmin.close();

	///////////////////////////////////////////////////////////////

	std::ifstream aacin(argv[5], std::ios::binary);
	std::ofstream outpcm("./out.pcm", std::ios::binary);

	AAC_DECODER_ERROR dret = AAC_DEC_OK;
	HANDLE_AACDECODER hdecoder = aacDecoder_Open(TT_MP4_ADTS, 1);
	CStreamInfo* dinfo = nullptr;
	const int outbufsize = 4096;
	outdata = malloc(outbufsize);
	UINT valid = 0;
	ADTS adts = { 0 };
	while (aacin.read(reinterpret_cast<char*>(&adts), sizeof(adts)))
	{
		UINT framelength = get_aac_frame_length(adts);
		aacin.seekg(-sizeof(adts), std::ios::cur);

		UCHAR* indata = static_cast<UCHAR*>(malloc(framelength));
		aacin.read(reinterpret_cast<char*>(indata), framelength);
		valid = framelength;
		// 输入完整的adts
		dret = aacDecoder_Fill(hdecoder, reinterpret_cast<UCHAR**>(&indata), &framelength, &valid);
		dret = aacDecoder_DecodeFrame(hdecoder, reinterpret_cast<INT_PCM*>(outdata), outbufsize / sizeof(INT_PCM), 0);
		dinfo = aacDecoder_GetStreamInfo(hdecoder);
		if (dret == AAC_DEC_OK)
		{
			outpcm.write(reinterpret_cast<char*>(outdata), dinfo->numChannels * dinfo->frameSize * sizeof(INT_PCM));
			std::cout << "samplerate : " << dinfo->sampleRate << " channels : " << dinfo->numChannels << std::endl;
		}
		free(indata);
		indata = nullptr;
	}
	aacDecoder_Close(hdecoder);
	hdecoder = nullptr;
	free(outdata);
	outfile.close();
	aacin.close();

	return 0;
}