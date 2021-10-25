/*
 * @Author: gongluck
 * @Date: 2021-10-25 16:43:06
 * @Last Modified by: gongluck
 * @Last Modified time: 2021-10-25 17:55:26
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
	int outlen = 0;

	auto framesize = width * height * 3 / 2;
	yuvin.seekg(framesize * 20);
	uint8_t *indata = static_cast<uint8_t *>(malloc(framesize));
	yuvin.read(reinterpret_cast<char *>(indata), framesize);

	// encode
	WebPPicture epic;
	WebPConfig econfig;
	WebPMemoryWriter ewrt;
	if (!WebPConfigPreset(&econfig, WEBP_PRESET_DEFAULT, 10.0f) ||
		!WebPPictureInit(&epic))
	{
		std::cerr << "WebPConfigPreset or WebPPictureInit failed." << std::endl;
		return -1;
	}

	econfig.lossless = 0;		  //是否无损
	econfig.quality = 50;		  //无损时可调节编码复杂度
	WebPValidateConfig(&econfig); // not mandatory, but useful
	epic.use_argb = 0;
	epic.width = width;
	epic.height = height;
	epic.writer = WebPMemoryWrite;
	epic.custom_ptr = &ewrt;
	WebPMemoryWriterInit(&ewrt);
	if (!WebPPictureAlloc(&epic))
	{
		std::cerr << "WebPPictureAlloc failed." << std::endl;
		return -1;
	}

	// 获取y、u、v三个分量各自数据的指针地址
	unsigned char *ybase, *ubase, *vbase;
	ybase = indata;
	ubase = indata + width * height;
	vbase = indata + width * height * 5 / 4;
	int ystride = width;
	int ustride = width / 2;
	int vstride = width / 2;

	int dest_ystride = epic.y_stride;
	int dest_ustride = epic.uv_stride;
	int dest_vstride = epic.uv_stride;
	int widthuv = width / 2;

	unsigned char *dest_y = epic.y;
	for (int i = 0; i < height; i++)
	{
		memcpy(dest_y, ybase, width);
		ybase += ystride;
		dest_y += dest_ystride;
	}

	unsigned char *dest_u = epic.u;
	unsigned char *dest_v = epic.v;
	for (int i = 0; i < height / 2; i++)
	{
		memcpy(dest_u, ubase, widthuv);
		memcpy(dest_v, vbase, widthuv);
		ubase += ustride;
		dest_u += dest_ustride;
		vbase += vstride;
		dest_v += dest_vstride;
	}

	if (!WebPEncode(&econfig, &epic))
	{
		std::cerr << "WebPEncode failed." << std::endl;
		return -1;
	}
	outfile.write(reinterpret_cast<char *>(ewrt.mem), ewrt.size);
	outlen = ewrt.size;
	WebPMemoryWriterClear(&ewrt);
	WebPPictureFree(&epic);

	yuvin.open(argv[4], std::ios::binary);
	yuvin.read(reinterpret_cast<char *>(indata), outlen);
	// decode
	WebPDecoderConfig dconfig;
	WebPDecBuffer *output_buffer = &dconfig.output;
	WebPBitstreamFeatures *bitstream = &dconfig.input;
	if (!WebPInitDecoderConfig(&dconfig))
	{
		std::cerr << "WebPInitDecoderConfig failed." << std::endl;
		return -1;
	}

	output_buffer->colorspace = MODE_RGBA;
	dconfig.output.colorspace = MODE_RGBA;
	dconfig.options.use_threads = 1;
	if (WebPDecode(indata, outlen, &dconfig) != VP8_STATUS_OK)
	{
		std::cerr << "WebPDecode failed." << std::endl;
		return -1;
	}

	outfile.open("./out.rgba", std::ios::binary);
	outfile.write(reinterpret_cast<char *>(output_buffer->u.RGBA.rgba), output_buffer->u.RGBA.size);
	WebPFreeDecBuffer(output_buffer);

	free(indata);
	outfile.close();
	yuvin.close();
	return 0;
}