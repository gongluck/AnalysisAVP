/*
 * @Author: gongluck
 * @Date: 2022-06-24 10:35:37
 * @Last Modified by: gongluck
 * @Last Modified time: 2022-06-24 17:54:04
 */

#include <iostream>
#include <fstream>

extern "C"
{
#include "libswresample/swresample.h"
}

#define CHECKERR(err, OK)                                                \
	if ((err) != (OK))                                                     \
	{                                                                      \
		std::cerr << "error : " << (err) << " in " << __LINE__ << std::endl; \
		exit(err);                                                           \
	}

int main(int argc, char *argv[])
{
	av_log_set_level(AV_LOG_VERBOSE);

	std::cout << "ffmpeg demo" << std::endl;
	std::cout << "Usage : "
						<< "thisfilename pcmfile outfile" << std::endl;
	if (argc < 3)
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
	std::ofstream outfile(argv[2], std::ios::binary);
	if (outfile.fail())
	{
		std::cerr << "can not open file " << argv[2] << std::endl;
		return -1;
	}

	const int insamplerate = 44100;
	const int outsamplerate = 48000;
	const AVSampleFormat informat = AV_SAMPLE_FMT_S16;
	const AVSampleFormat outformat = AV_SAMPLE_FMT_U8;
	const int inlayout = AV_CH_LAYOUT_STEREO;
	const int outlayout = AV_CH_LAYOUT_MONO;
	const int inchannels = av_get_channel_layout_nb_channels(inlayout);
	const int outchannels = av_get_channel_layout_nb_channels(outlayout);

	const int insamplecount = insamplerate * 10 / 1000;
	std::cout << "in sample count : " << insamplecount << std::endl;
	const int inputsize = insamplecount * inchannels * av_get_bytes_per_sample(informat);
	std::cout << "input size : " << inputsize << std::endl;
	uint8_t *indata = static_cast<uint8_t *>(malloc(inputsize));

	const int outsamplecount = outsamplerate * 10 / 1000;
	std::cout << "out sample count : " << outsamplecount << std::endl;
	const int outputsize = outsamplecount * outchannels * av_get_bytes_per_sample(outformat);
	std::cout << "output size : " << outputsize << std::endl;
	uint8_t *outdata = static_cast<uint8_t *>(malloc(outputsize));

	// ffmpeg
	struct SwrContext *swrctx = swr_alloc_set_opts(nullptr, outlayout, outformat, outsamplerate, inlayout, informat, insamplerate, 0, nullptr);
	CHECKERR(swrctx != nullptr, true);
	int ret = swr_init(swrctx);
	CHECKERR(ret, 0);
	while (pcmin.read(reinterpret_cast<char *>(indata), inputsize))
	{
		ret = swr_convert(swrctx, &outdata, outputsize, const_cast<const uint8_t **>(&indata), insamplecount);
		CHECKERR(ret > 0, true);
		outfile.write(reinterpret_cast<char *>(outdata), ret * av_get_bytes_per_sample(outformat) * outchannels);
	}

	swr_free(&swrctx);
	free(indata);
	free(outdata);
	pcmin.close();
	outfile.close();

	return 0;
}
