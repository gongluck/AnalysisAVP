/*
 * @Author: gongluck
 * @Date: 2020-09-02 23:40:40
 * @Last Modified by: gongluck
 * @Last Modified time: 2021-05-25 17:00:15
 */

#include <iostream>
#include <fstream>
#include "x264.h"

int main(int argc, char *argv[])
{
	std::cout << "x264 demo" << std::endl;
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

	x264_param_t param = {0};
	x264_param_default(&param);
	// 设置preset和tune
	auto ret = x264_param_default_preset(&param, "ultrafast", "zerolatency");
	// 设置profile
	ret = x264_param_apply_profile(&param, "baseline");
	param.i_threads = 5;
	param.i_width = width;
	param.i_height = height;
	param.i_bframe = 3;
	param.i_fps_num = 23;
	param.i_fps_den = 1;
	//b_vfr_input = 0，这时根据fps而不是timebase,timestamps来计算帧间距离。
	//b_vfr_input = 1是X264缺省设置，在zerolatency有 = 0的配置。
	param.b_vfr_input = 0;
	param.i_keyint_max = 250;
	param.rc.i_rc_method = X264_RC_ABR;
	param.rc.i_bitrate = 1500;
	param.rc.i_vbv_max_bitrate = 2500;
	param.i_scenecut_threshold = 40;
	param.i_level_idc = 51;
	param.b_repeat_headers = 1;
	x264_t *handle = x264_encoder_open(&param);

	x264_picture_t pic_in = {0};
	x264_picture_init(&pic_in);
	x264_picture_alloc(&pic_in, X264_CSP_I420, width, height);
	pic_in.img.i_csp = X264_CSP_I420;
	pic_in.img.i_plane = 3;

	x264_picture_t pic_out = {0};
	x264_picture_init(&pic_out);

	x264_nal_t *nal = nullptr;
	int i_nal = 0;

	auto framesize = width * height * 3 / 2;
	uint8_t *data = static_cast<uint8_t *>(malloc(framesize));
	while (yuvin.read(reinterpret_cast<char *>(data), framesize))
	{
		memcpy(pic_in.img.plane[0], data, width * height);
		memcpy(pic_in.img.plane[1], data + width * height, width * height / 4);
		memcpy(pic_in.img.plane[2], data + width * height * 5 / 4, width * height / 4);

		pic_in.i_type = X264_TYPE_AUTO;

		std::cout << "++++++++++ipts : " << pic_in.i_pts++ << std::endl;
		ret = x264_encoder_encode(handle, &nal, &i_nal, &pic_in, &pic_out);
		if (ret > 0)
		{
			std::cout << "----------opts : " << pic_out.i_pts << std::endl;
			outfile.write(reinterpret_cast<char *>(nal[0].p_payload), ret);
		}
	}

	//flush
	while (x264_encoder_encode(handle, &nal, &i_nal, nullptr, &pic_out) > 0)
	{
		std::cout << "----------opts : " << pic_out.i_pts << std::endl;
		outfile.write(reinterpret_cast<char *>(nal[0].p_payload), ret);
	}

	if (handle != nullptr)
	{
		x264_encoder_close(handle);
		handle = nullptr;
	}
	x264_picture_clean(&pic_in);
	free(data);

	outfile.close();
	yuvin.close();

	return 0;
}