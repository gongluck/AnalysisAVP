/*
 * @Author: gongluck 
 * @Date: 2020-11-02 23:16:17 
 * @Last Modified by: gongluck
 * @Last Modified time: 2020-11-02 23:19:39
 */

#include "flv.h"
#include "../aac/aac.h"

#include <iomanip>
#include <fstream>

//#define OUTPUTRAW

int main(int argc, char *argv[])
{
	std::cout << "flv analysis" << std::endl;

	std::cout << "Usage : "
			  << "flv flvfile." << std::endl;

	if (argc < 2)
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

	FLVHEADER flvheader = {0};
	in.read(reinterpret_cast<char *>(&flvheader), sizeof(flvheader));
	std::cout << flvheader << std::endl;

	// 保存264数据
	std::ofstream out264("out.264", std::ios::binary);
	char nalutag[] = {0x00, 0x00, 0x00, 0x01};

	// 保存aac数据
	std::ofstream outaac("out.aac", std::ios::binary);
	ADTS adts = {0};
	set_syncword(adts, 0xFFF);
	adts.protection_absent = 1;
	adts.ID = ADTS_ID_MPEG4;
	set_adts_buffer_fullness(adts, 0x7FF);

	while (true)
	{
		FLVINT32 presize = {0};
		if (!in.read(reinterpret_cast<char *>(&presize), 4))
			break;
		std::cout << "presize : " << FLVINT32TOINT(presize) << std::endl;

		std::cout << std::string(50, '*').c_str() << std::endl;
		FLVTAGHEADER tagheader = {0};
		if (!in.read(reinterpret_cast<char *>(&tagheader), sizeof(tagheader)))
			break;
		std::cout << tagheader << std::endl;

		auto datalen = FLVINT24TOINT(tagheader.datalen);
		auto data = new char[datalen];
		if (!in.read(data, datalen))
			break;

		if (tagheader.flvtagtype == FLV_TAG_TYPE_VIDEO)
		{
			std::cout << std::string(50, '-').c_str() << std::endl;
			FLVVIDEOTAG *pvideotag = reinterpret_cast<FLVVIDEOTAG *>(data);
			std::cout << *pvideotag << std::endl;
			std::cout << std::string(50, '-').c_str() << std::endl;
			if (pvideotag->codecid == FLV_VIDEO_CODECID_AVC)
			{
				if (pvideotag->videopacket.avcvideopacket.avcpacketype == AVC_PACKET_HEADER)
				{
					AVCDecoderConfigurationRecordHeader *configheader =
						reinterpret_cast<AVCDecoderConfigurationRecordHeader *>(pvideotag->videopacket.avcvideopacket.avcpacketdata);
					std::cout << *configheader << std::endl;

					SequenceParameterSet *sps =
						reinterpret_cast<SequenceParameterSet *>(configheader->data);
					int datasize = FLVINT16TOINT((sps->sequenceParameterSetLength));
					out264.write(nalutag, 4);
					out264.write(reinterpret_cast<char *>(sps->sequenceParameterSetNALUnit), datasize);
					std::cout << *sps << std::endl;

					PictureParameterSet *pps =
						reinterpret_cast<PictureParameterSet *>(sps->sequenceParameterSetNALUnit + FLVINT16TOINT(sps->sequenceParameterSetLength));
					datasize = FLVINT16TOINT((pps->pictureParameterSetLength));
					out264.write(nalutag, 4);
					out264.write(reinterpret_cast<char *>(pps->pictureParameterSetNALUnit), datasize);
					std::cout << *pps << std::endl;
				}
				else if (pvideotag->videopacket.avcvideopacket.avcpacketype == AVC_PACKET_NALU)
				{
#ifdef OUTPUTRAW
					std::cout << "nalu : ";
					std::ios::fmtflags f(std::cout.flags());
					for (int i = 0; i < FLVINT24TOINT(tagheader.datalen) - offsetof(FLVVIDEOTAG, videopacket.avcvideopacket.avcpacketdata); ++i)
					{
						std::cout << std::setw(2) << std::setfill('0') << std::hex << static_cast<unsigned int>(pvideotag->videopacket.avcvideopacket.avcpacketdata[i]) << " ";
					}
					std::cout.flags(f);
#endif

					FLVINT32 *nalsize = reinterpret_cast<FLVINT32 *>(&pvideotag->videopacket.avcvideopacket.avcpacketdata[0]);
					int datasize = FLVINT32TOINT((*nalsize));
					out264.write(nalutag, 4);
					out264.write(reinterpret_cast<char *>(&pvideotag->videopacket.avcvideopacket.avcpacketdata[0 + 4]), datasize);
				}
				else if (pvideotag->videopacket.avcvideopacket.avcpacketype == AVC_PACKET_END)
				{
					out264.close();
					std::cout << "AVC end of sequence" << std::endl;
				}
			}
		}
		else if (tagheader.flvtagtype == FLV_TAG_TYPE_AUDIO)
		{
			std::cout << std::string(50, '-').c_str() << std::endl;
			FLVAUDIOTAG *paudiotag = reinterpret_cast<FLVAUDIOTAG *>(data);
			std::cout << *paudiotag << std::endl;
			std::cout << std::string(50, '-').c_str() << std::endl;
			if (paudiotag->soundFormat == FLV_SOUND_FORMAT_AAC)
			{
				if (paudiotag->audiopacket.aacaudiopacket.aacpackettype == AAC_PACKET_TYPE_HEAD)
				{
					AudioSpecificConfig *pconfig =
						reinterpret_cast<AudioSpecificConfig *>(paudiotag->audiopacket.aacaudiopacket.data);
					std::cout << *pconfig << std::endl;
					set_channel_configuration(adts, pconfig->ChannelConfiguration);
					adts.sampling_frequency_index = FVLSAMPLEFREQUENCYINDEX((*pconfig));
					adts.profile = pconfig->AudioObjectType - 1;
				}
				else if (paudiotag->audiopacket.aacaudiopacket.aacpackettype == AAC_PACKET_TYPE_RAW)
				{
					auto datasize = FLVINT24TOINT(tagheader.datalen) - offsetof(FLVAUDIOTAG, audiopacket.aacaudiopacket.data);
#ifdef OUTPUTRAW
					std::ios::fmtflags f(std::cout.flags());
					for (int i = 0; i < datasize; ++i)
					{
						std::cout << std::setw(2) << std::setfill('0') << std::hex << static_cast<unsigned int>(paudiotag->audiopacket.aacaudiopacket.data[i]) << " ";
					}
					std::cout.flags(f);
#endif

					set_aac_frame_length(adts, datasize + sizeof(adts));
					outaac.write(reinterpret_cast<char *>(&adts), sizeof(adts));
					outaac.write(reinterpret_cast<char *>(paudiotag->audiopacket.aacaudiopacket.data), datasize);
				}
			}
		}

		delete[] data;
	}

	in.close();
	return 0;
}
