/*
 * @Author: gongluck
 * @Date: 2021-10-25 16:43:06
 * @Last Modified by: gongluck
 * @Last Modified time: 2021-10-27 16:13:26
 */

#include <iostream>
#include <fstream>
#include <encode.h>
#include <decode.h>

int main(int argc, char *argv[])
{
	std::cout << "libwebp demo" << std::endl;
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
	int webplen = 0;

	auto framesize = width * height * 3 / 2;
	yuvin.seekg(framesize * 20);
	uint8_t *indata = static_cast<uint8_t *>(malloc(framesize));
	yuvin.read(reinterpret_cast<char *>(indata), framesize);

	int eret = 0;
	// encode
	WebPConfig econfig;
	eret = WebPConfigPreset(&econfig, WEBP_PRESET_DEFAULT, 75.0f);
	if (eret != 1)
	{
		std::cerr << "WebPConfigPreset failed, error:" << eret << std::endl;
		return -1;
	}
	econfig.use_sharp_yuv = 1;//可减少色差
	WebPValidateConfig(&econfig); // not mandatory, but useful

	WebPPicture epic;
	eret = WebPPictureInit(&epic);
	if (eret != 1)
	{
		std::cerr << "WebPPictureInit failed, error:" << eret << std::endl;
		return -1;
	}
	epic.use_argb = 0;
	epic.width = width;
	epic.height = height;
	eret = WebPPictureAlloc(&epic);
	if (eret != 1)
	{
		std::cerr << "WebPPictureAlloc failed, error:" << eret << std::endl;
		return -1;
	}

	WebPMemoryWriter writer;
	WebPMemoryWriterInit(&writer);
	epic.writer = WebPMemoryWrite;
	epic.custom_ptr = &writer;

	// 拷贝数据
	unsigned char *ybase = indata;
	unsigned char *dest_y = epic.y;
	for (int i = 0; i < height; i++)
	{
		memcpy(dest_y, ybase, width);
		ybase += width;
		dest_y += epic.y_stride;
	}

	unsigned char *ubase = indata + width * height;
	unsigned char *vbase = indata + width * height * 5 / 4;
	unsigned char *dest_u = epic.u;
	unsigned char *dest_v = epic.v;
	for (int i = 0; i < height / 2; i++)
	{
		memcpy(dest_u, ubase, width / 2);
		memcpy(dest_v, vbase, width / 2);
		ubase += width / 2;
		dest_u += epic.uv_stride;
		vbase += width / 2;
		dest_v += epic.uv_stride;
	}

	eret = WebPEncode(&econfig, &epic);
	if (eret != 1)
	{
		std::cerr << "WebPEncode failed, error:" << eret << std::endl;
		return -1;
	}

	outfile.write(reinterpret_cast<char *>(writer.mem), writer.size);
	outfile.close();
	webplen = writer.size;
	WebPMemoryWriterClear(&writer);
	WebPPictureFree(&epic);

	yuvin.close();
	yuvin.open(argv[4], std::ios::binary);
	yuvin.read(reinterpret_cast<char *>(indata), webplen);
	int dret = 0;
	// decode
	WebPDecoderConfig dconfig;
	dret = WebPInitDecoderConfig(&dconfig);
	if (dret != 1)
	{
		std::cerr << "WebPInitDecoderConfig failed, error:" << dret << std::endl;
		return -1;
	}

	/*dret = WebPGetFeatures(indata, webplen, &dconfig.input);
	if (dret != VP8_STATUS_OK)
	{
		std::cerr << "WebPGetFeatures failed, error:" << dret << std::endl;
		return -1;
	}*/

	dconfig.output.colorspace = MODE_RGB;
	dconfig.options.use_threads = 1;
	// dconfig.output.u.RGBA.rgba = (uint8_t*)malloc(width*height*3);
	// dconfig.output.u.RGBA.stride = width*3;
	// dconfig.output.u.RGBA.size = width * height * 3;
	// dconfig.output.is_external_memory = 1;

	dret = WebPDecode(indata, webplen, &dconfig);
	if (dret != VP8_STATUS_OK)
	{
		std::cerr << "WebPDecode failed, error:" << dret << std::endl;
		return -1;
	}

	outfile.open("./out.rgba", std::ios::binary);
	outfile.write(reinterpret_cast<char *>(dconfig.output.u.RGBA.rgba), dconfig.output.u.RGBA.size);
	// free(dconfig.output.u.RGBA.rgba);
	WebPFreeDecBuffer(&dconfig.output);

	free(indata);
	outfile.close();
	yuvin.close();
	return 0;
}