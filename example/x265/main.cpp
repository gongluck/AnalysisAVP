/*
 * @Author: gongluck
 * @Date: 2021-10-25 16:43:06
 * @Last Modified by: gongluck
 * @Last Modified time: 2021-10-27 16:13:26
 */

#include <iostream>
#include <fstream>
#include <x265.h>

int main(int argc, char *argv[])
{
	std::cout << "x265 demo" << std::endl;
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
	auto width = std::atoi(argv[2]);
	auto height = std::atoi(argv[3]);
	std::ofstream outfile(argv[4], std::ios::binary);

	size_t framesize = width * height * 3 / 2;
	char *indata = static_cast<char *>(malloc(framesize));

	//encode
	x265_param param;
	x265_param_default(&param);
	param.bRepeatHeaders = 1; // write sps,pps before keyframe
	param.internalCsp = X265_CSP_I420;
	param.sourceWidth = width;
	param.sourceHeight = height;
	param.fpsNum = 25;  // 帧率
	param.fpsDenom = 1; // 帧率
	param.bframes = 0;
	param.rc.rateControlMode = X265_RC_ABR;//平均比特率
	param.rc.bitrate = 248;//kbps
	auto handle = x265_encoder_open(&param);
	//ret = x265_param_apply_profile(&param, "main");
	//ret = x265_param_default_preset(&param, "fast", "zerolatency");
	auto pic_in = x265_picture_alloc();
	x265_picture_init(&param, pic_in);

	x265_nal* nal = nullptr;
	uint32_t i_nal = 0;
	while (yuvin.read(indata, framesize))
	{
		pic_in->planes[0] = indata;
		pic_in->planes[1] = indata + param.sourceWidth * param.sourceHeight;
		pic_in->planes[2] = indata + param.sourceWidth * param.sourceHeight * 5 / 4;
		pic_in->stride[0] = param.sourceWidth;
		pic_in->stride[1] = param.sourceWidth / 2;
		pic_in->stride[2] = param.sourceWidth / 2;

		int ret = x265_encoder_encode(handle, &nal, &i_nal, pic_in, nullptr);
		for (uint32_t i = 0; i < i_nal; ++i)
		{
			outfile.write(reinterpret_cast<char*>(nal[i].payload), nal[i].sizeBytes);
		}
	}

	while (x265_encoder_encode(handle, &nal, &i_nal, nullptr, nullptr)>0)
	{
		for (uint32_t i = 0; i < i_nal; ++i)
		{
			outfile.write(reinterpret_cast<char*>(nal[i].payload), nal[i].sizeBytes);
		}
	}

	x265_encoder_close(handle);
	x265_picture_free(pic_in);

	free(indata);
	outfile.close();
	yuvin.close();
	return 0;
}