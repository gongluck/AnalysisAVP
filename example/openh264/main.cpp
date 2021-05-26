/*
 * @Author: gongluck 
 * @Date: 2021-05-26 11:51:14 
 * @Last Modified by: gongluck
 * @Last Modified time: 2021-05-26 21:18:54
 */

#include <iostream>
#include <fstream>
#include "codec_api.h"

int main(int argc, char *argv[])
{
	std::cout << "openh264 demo" << std::endl;
	std::cout << "Usage : "
			  << "thisfilename yuvfile width height" << std::endl;
	if (argc < 4)
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
	int width = atoi(argv[2]);
	int height = atoi(argv[3]);
	std::ofstream out264("./out.h264", std::ios::binary);

	ISVCEncoder *encoder = nullptr;
	int ret = WelsCreateSVCEncoder(&encoder);
	SEncParamExt eparam;
	encoder->GetDefaultParams(&eparam);
	eparam.iUsageType = CAMERA_VIDEO_REAL_TIME;
	eparam.fMaxFrameRate = 150;
	eparam.iPicWidth = width;
	eparam.iPicHeight = height;
	eparam.iTargetBitrate = 5000;
	eparam.iRCMode = RC_BITRATE_MODE;
	eparam.iTemporalLayerNum = 1;
	eparam.iSpatialLayerNum = 1;
	eparam.bEnableDenoise = false;
	eparam.bEnableBackgroundDetection = true;
	eparam.bEnableAdaptiveQuant = false;
	eparam.bEnableFrameSkip = false;
	eparam.bEnableLongTermReference = false;
	eparam.uiIntraPeriod = 15u;
	eparam.eSpsPpsIdStrategy = CONSTANT_ID;
	eparam.bPrefixNalAddingCtrl = false;
	eparam.sSpatialLayers[0].iVideoWidth = width;
	eparam.sSpatialLayers[0].iVideoHeight = height;
	eparam.sSpatialLayers[0].fFrameRate = 64;
	eparam.sSpatialLayers[0].iSpatialBitrate = 5000;
	eparam.sSpatialLayers[0].iMaxSpatialBitrate = eparam.iMaxBitrate;
	encoder->InitializeExt(&eparam);

	SFrameBSInfo einfo = {0};
	SSourcePicture pic = {0};
	pic.iPicWidth = eparam.iPicWidth;
	pic.iPicHeight = eparam.iPicHeight;
	pic.iColorFormat = videoFormatI420;
	pic.iStride[0] = pic.iPicWidth;
	pic.iStride[1] = pic.iStride[2] = pic.iPicWidth / 2;

	auto framesize = width * height * 3 / 2;
	uint8_t *data = static_cast<uint8_t *>(malloc(framesize));
	while (in.read(reinterpret_cast<char *>(data), framesize))
	{
		pic.pData[0] = data;
		pic.pData[1] = data + width * height;
		pic.pData[2] = data + width * height * 5 / 4;
		static int index = 0;
		pic.uiTimeStamp = index++ * 41.667;
		std::cout << "++++++++++ipts : " << pic.uiTimeStamp << std::endl;
		ret = encoder->EncodeFrame(&pic, &einfo);
		if (ret >= 0)
		{
			std::cout << "----------opts : " << einfo.uiTimeStamp << std::endl;
			out264.write(reinterpret_cast<char *>(einfo.sLayerInfo[0].pBsBuf), einfo.iFrameSizeInBytes);
		}
	}
	free(data);

	WelsDestroySVCEncoder(encoder);
	encoder = nullptr;

	out264.close();
	in.close();

	///////////////////////////////////////////////////////////////

	in.open("./out.h264", std::ios::binary);
	in.seekg(0, std::ios_base::end);
	const int datalen = in.tellg();
	in.seekg(0, std::ios_base::beg);
	std::ofstream outyuv("./out.yuv", std::ios::binary);

	ISVCDecoder *decoder = nullptr;
	ret = WelsCreateDecoder(&decoder);
	SDecodingParam dparam = {0};
	dparam.sVideoProperty.size = sizeof(dparam.sVideoProperty);
	dparam.sVideoProperty.eVideoBsType = VIDEO_BITSTREAM_DEFAULT;
	dparam.eEcActiveIdc = ERROR_CON_SLICE_COPY;
	dparam.uiTargetDqLayer = UCHAR_MAX;
	dparam.bParseOnly = false;
	ret = decoder->Initialize(&dparam);

	uint8_t *dst[3] = {0};
	data = static_cast<uint8_t *>(malloc(datalen));
	SBufferInfo dinfo = {0};
	in.read(reinterpret_cast<char *>(data), datalen);
	int curpos = 0;
	int slicesize = 0;
	while (curpos < datalen)
	{
		int i = 0;
		for (i = 0; i + curpos < datalen; i++)
		{
			if ((data[curpos + i] == 0 && data[curpos + i + 1] == 0 && data[curpos + i + 2] == 0 && data[curpos + i + 3] == 1 && i > 0) ||
				(data[curpos + i] == 0 && data[curpos + i + 1] == 0 && data[curpos + i + 2] == 1 && i > 0))
			{
				break;
			}
		}
		slicesize = i;

		ret = decoder->DecodeFrame2(data + curpos, slicesize, dst, &dinfo);
		if (ret >= 0 && dinfo.iBufferStatus == 1)
		{
			outyuv.write(reinterpret_cast<char *>(dst[0]), dinfo.UsrData.sSystemBuffer.iStride[0] * dinfo.UsrData.sSystemBuffer.iHeight);
			outyuv.write(reinterpret_cast<char *>(dst[1]), dinfo.UsrData.sSystemBuffer.iStride[1] * dinfo.UsrData.sSystemBuffer.iHeight / 2);
			outyuv.write(reinterpret_cast<char *>(dst[2]), dinfo.UsrData.sSystemBuffer.iStride[1] * dinfo.UsrData.sSystemBuffer.iHeight / 2);
			outyuv.flush();
		}
		curpos += slicesize;
	}

	//flush
	auto left = decoder->GetOption(DECODER_OPTION_NUM_OF_FRAMES_REMAINING_IN_BUFFER, nullptr);
	while (left-- > 0)
	{
		if (decoder->FlushFrame(dst, &dinfo) >= 0 && dinfo.iBufferStatus == 1)
		{
			outyuv.write(reinterpret_cast<char *>(dst[0]), dinfo.UsrData.sSystemBuffer.iStride[0] * dinfo.UsrData.sSystemBuffer.iHeight);
			outyuv.write(reinterpret_cast<char *>(dst[1]), dinfo.UsrData.sSystemBuffer.iStride[1] * dinfo.UsrData.sSystemBuffer.iHeight / 2);
			outyuv.write(reinterpret_cast<char *>(dst[2]), dinfo.UsrData.sSystemBuffer.iStride[1] * dinfo.UsrData.sSystemBuffer.iHeight / 2);
			outyuv.flush();
		}
	}

	WelsDestroyDecoder(decoder);
	decoder = nullptr;

	outyuv.close();
	in.close();

	return 0;
}