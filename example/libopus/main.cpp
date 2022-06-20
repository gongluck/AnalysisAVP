/*
 * @Author: gongluck
 * @Date: 2022-06-16 11:28:46
 * @Last Modified by: gongluck
 * @Last Modified time: 2022-06-20 17:20:43
 */

#include <iostream>
#include <fstream>
#include <opus.h>

#define CHECKERR(err, OK)                                              \
	if (err != OK)                                                       \
	{                                                                    \
		std::cerr << "error : " << err << " in " << __LINE__ << std::endl; \
		exit(err);                                                         \
	}

#define OUTPUTFILE

int main(int argc, char *argv[])
{
	std::cout << "libopus demo" << std::endl;
	std::cout << "Usage : "
						<< "thisfilename pcmfile samplerate channels outfile" << std::endl;
	if (argc < 5)
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
	const int samplerate = atoi(argv[2]);
	const int channels = atoi(argv[3]);
#ifdef OUTPUTFILE
	std::ofstream outfile(argv[4], std::ios::binary);
	std::ofstream outfile2("out.pcm", std::ios::binary);
#endif
	//每个声道样本数
	auto samplesize = samplerate / 1000 * 10; // 10ms
	std::cout << "sample size : " << samplesize << std::endl;
	auto inputsize = sizeof(int16_t) * samplesize * channels;
	std::cout << "input size : " << inputsize << std::endl;
	int16_t *indata = static_cast<int16_t *>(malloc(inputsize));
	int16_t *outbuf = static_cast<int16_t *>(malloc(inputsize));

	// libopus
	int error = OPUS_OK;
	OpusEncoder *encoder = opus_encoder_create(samplerate, channels, OPUS_APPLICATION_AUDIO, &error);
	OpusDecoder *decoder = opus_decoder_create(samplerate, channels, &error);
	error = opus_encoder_ctl(encoder, OPUS_SET_BITRATE(20000)); //波特率
	CHECKERR(error, OPUS_OK)
	error = opus_encoder_ctl(encoder, OPUS_SET_COMPLEXITY(1)); //复杂度1-10
	CHECKERR(error, OPUS_OK)
	error = opus_encoder_ctl(encoder, OPUS_SET_APPLICATION(OPUS_APPLICATION_AUDIO));
	CHECKERR(error, OPUS_OK)
	error = opus_encoder_ctl(encoder, OPUS_SET_SIGNAL(OPUS_SIGNAL_VOICE));
	CHECKERR(error, OPUS_OK)

	const int maxpacket = 1500;
	unsigned char packet[maxpacket] = {0};
	while (pcmin.read(reinterpret_cast<char *>(indata), inputsize))
	{
		//注意接口参数和返回值的说明
		auto len = opus_encode(encoder, indata, samplesize, packet, maxpacket);
		std::cout << "encode len : " << len << std::endl;
		if (len > 0)
		{
#ifdef OUTPUTFILE
			outfile.write(reinterpret_cast<char *>(packet), len);
#endif
		}

		//注意接口参数和返回值的说明
		auto num = opus_decode(decoder, packet, len, outbuf, samplesize, 0);
		std::cout << "decode len : " << num * sizeof(int16_t) << std::endl;
		if (num > 0)
		{
#ifdef OUTPUTFILE
			outfile2.write(reinterpret_cast<char *>(outbuf), num * sizeof(int16_t) * channels);
#endif
		}
	}

	opus_encoder_destroy(encoder);
	opus_decoder_destroy(decoder);

	free(indata);
	free(outbuf);
#ifdef OUTPUTFILE
	outfile.close();
	outfile2.close();
#endif
	pcmin.close();
	return 0;
}