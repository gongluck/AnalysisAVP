/*
 * @Author: gongluck 
 * @Date: 2020-11-02 23:15:56 
 * @Last Modified by:   gongluck 
 * @Last Modified time: 2020-11-02 23:15:56 
 */

#include "flv.h"

#include <iomanip>

const char *flv_tag_parse_type(uint8_t type)
{
	switch (type)
	{
	case FLV_TAG_TYPE_AUDIO:
		return "audio tag";
	case FLV_TAG_TYPE_VIDEO:
		return "video tag";
	case FLV_TAG_TYPE_SCRIPT:
		return "script tag";
	default:
		return "UNKNOWN";
	}
}

const char *flv_video_parse_type(uint8_t type)
{
	switch (type)
	{
	case FLV_VIDEO_FRAME_KEYFRAME:
		return "keyframe(for AVC, a seekable frame) —— 即H.264的IDR帧";
	case FLV_VIDEO_FRAME_IFRAME:
		return "inter frame(for AVC, a non - seekable frame) —— H.264的普通I帧";
	case FLV_VIDEO_FRAME_DISPOSABLE:
		return "disposable inter frame(H.263 only)";
	case FLV_VIDEO_FRAME_GENERATED:
		return "generated keyframe(reserved for server use only)";
	case FLV_VIDEO_FRAME_COMMAND:
		return "video info / command frame";
	default:
		return "UNKNOWN";
	}
}

const char *flv_video_parse_codecid(uint8_t codecid)
{
	switch (codecid)
	{
	case FLV_VIDEO_CODECID_JPEG:
		return "JPEG (currently unused)";
	case FLV_VIDEO_CODECID_H263:
		return "Sorenson H.263";
	case FLV_VIDEO_CODECID_SCREEN:
		return "Screen video";
	case FLV_VIDEO_CODECID_ON2VP6:
		return "On2 VP6";
	case FLV_VIDEO_CODECID_ON2VP6A:
		return "On2 VP6 with alpha channel";
	case FLV_VIDEO_CODECID_SCREEN2:
		return "Screen video version 2";
	case FLV_VIDEO_CODECID_AVC:
		return "AVC";
	default:
		return "UNKNOWN";
	}
}

const char *flv_audio_parse_type(uint8_t type)
{
	switch (type)
	{
	case FLV_AUDIO_SOUND_MONO:
		return "mono";
	case FLV_AUDIO_SOUND_STEREO:
		return "stereo";
	default:
		return "UNKNOWN";
	}
}

const char *flv_audio_parse_soundsize(uint8_t soundsize)
{
	switch (soundsize)
	{
	case FLV_SOUND_SIZE_8:
		return "8Bits";
	case FLV_SOUND_SIZE_16:
		return "16Bits";
	default:
		return "UNKNOWN";
	}
}

const char *flv_audio_parse_soundrate(uint8_t soundrate)
{
	switch (soundrate)
	{
	case FLV_SOUND_RATE_55:
		return "5.5-kHz";
	case FLV_SOUND_RATE_11:
		return "11-kHz";
	case FLV_SOUND_RATE_22:
		return "22-kHz";
	case FLV_SOUND_RATE_44:
		return "44-kHz";
	default:
		return "UNKNOWN";
	}
}

const char *flv_audio_parse_soundformat(uint8_t format)
{
	switch (format)
	{
	case FLV_SOUND_FORMAT_PCM:
		return "Linear PCM, platform endian";
	case FLV_SOUND_FORMAT_ADPCM:
		return "ADPCM";
	case FLV_SOUND_FORMAT_MP3:
		return "MP3";
	case FLV_SOUND_FORMAT_PCMLE:
		return "inear PCM, little endian";
	case FLV_SOUND_FORMAT_NELLYMOSER16MONO:
		return "Nellymoser 16-kHz mono";
	case FLV_SOUND_FORMAT_NELLYMOSER8MONO:
		return "Nellymoser 8-kHz mono";
	case FLV_SOUND_FORMAT_NELLYMOSER:
		return "Nellymoser";
	case FLV_SOUND_FORMAT_G711LA:
		return "G.711A-law logarithmic PCM";
	case FLV_SOUND_FORMAT_G711MU:
		return "G.711mu-law logarithmic PCM";
	case FLV_SOUND_FORMAT_RESERVED:
		return "reserved";
	case FLV_SOUND_FORMAT_AAC:
		return "AAC";
	case FLV_SOUND_FORMAT_SPEEX:
		return "Speex";
	case FLV_SOUND_FORMAT_MP3_8:
		return "MP3 8-Khz";
	case FLV_SOUND_FORMAT_DEVICE:
		return "Device-specific sound";
	default:
		return "UNKNOWN";
	}
}

const char *avc_packet_parse_type(uint8_t type)
{
	switch (type)
	{
	case AVC_PACKET_HEADER:
		return "AVC sequence header";
	case AVC_PACKET_NALU:
		return "AVC NALU";
	case AVC_PACKET_END:
		return "AVC end of sequence";
	default:
		return "UNKNOWN";
	}
}

std::ostream &operator<<(std::ostream &os, const FLVHEADER &flvheader)
{
	os << "f : " << static_cast<char>(flvheader.F)
	   << "\nl : " << static_cast<char>(flvheader.L)
	   << "\nv : " << static_cast<char>(flvheader.V)
	   << "\ntype : " << static_cast<int>(flvheader.flvtype)
	   << "\nvideo : " << static_cast<int>(flvheader.hasvideo)
	   << "\naudio : " << static_cast<int>(flvheader.hasaudio);
	return os;
}

std::ostream &operator<<(std::ostream &os, const FLVVIDEOTAG &videotag)
{
	os << "type : " << flv_video_parse_type(videotag.type)
	   << "\ncodecid : " << flv_video_parse_codecid(videotag.codecid)
	   << "\navcpacket type : " << avc_packet_parse_type(videotag.videopacket.avcvideopacket.avcpacketype)
	   << "\navcpacket compositiontime : " << FLVINT24TOINT(videotag.videopacket.avcvideopacket.compositiontime);
	return os;
}

std::ostream &operator<<(std::ostream &os, const FLVTAGHEADER &tagheader)
{
	os << "type : " << flv_tag_parse_type(tagheader.flvtagtype)
	   << "\ndatalen : " << FLVINT24TOINT(tagheader.datalen)
	   << "\ntimestamp : " << FLVINT32TOINT(tagheader.timestamp)
	   << "\nstreamsid : " << FLVINT24TOINT(tagheader.streamsid);
	return os;
}

std::ostream &operator<<(std::ostream &os, const SequenceParameterSet &sps)
{
	os << "numOfSequenceParameterSets : " << static_cast<unsigned int>(sps.numOfSequenceParameterSets)
	   << "\nsequenceParameterSetLength : " << FLVINT16TOINT(sps.sequenceParameterSetLength)
	   << "\nsps : ";
	std::ios::fmtflags f(std::cout.flags());
	for (int i = 0; i < FLVINT16TOINT(sps.sequenceParameterSetLength); ++i)
	{
		os << std::setw(2) << std::setfill('0') << std::hex << static_cast<unsigned int>(sps.sequenceParameterSetNALUnit[i]) << " ";
	}
	std::cout.flags(f);
	return os;
}

std::ostream &operator<<(std::ostream &os, const PictureParameterSet &pps)
{
	os << "numOfPictureParameterSets : " << static_cast<unsigned int>(pps.numOfPictureParameterSets)
	   << "\npictureParameterSetLength : " << FLVINT16TOINT(pps.pictureParameterSetLength)
	   << "\npps : ";
	std::ios::fmtflags f(std::cout.flags());
	for (int i = 0; i < FLVINT16TOINT(pps.pictureParameterSetLength); ++i)
	{
		os << std::setw(2) << std::setfill('0') << std::hex << static_cast<unsigned int>(pps.pictureParameterSetNALUnit[i]) << " ";
	}
	std::cout.flags(f);
	return os;
}

std::ostream &operator<<(std::ostream &os, const FLVAUDIOTAG &audiotag)
{
	os << "soundtype : " << flv_audio_parse_type(audiotag.soundtype)
	   << "\nsoundSize : " << flv_audio_parse_soundsize(audiotag.soundSize)
	   << "\nsoundRate : " << flv_audio_parse_soundrate(audiotag.soundRate)
	   << "\nsoundFormat : " << flv_audio_parse_soundformat(audiotag.soundFormat);
	return os;
}

std::ostream &operator<<(std::ostream &os, const AudioSpecificConfig &audiospecificonfig)
{
	os << "SamplingFrequencyIndex : " << static_cast<unsigned int>(FVLSAMPLEFREQUENCYINDEX(audiospecificonfig))
	   << "\nAudioObjectType : " << static_cast<unsigned int>(audiospecificonfig.AudioObjectType)
	   << "\nChannelConfiguration : " << static_cast<unsigned int>(audiospecificonfig.ChannelConfiguration);
	return os;
}

std::ostream &operator<<(std::ostream &os, const AVCDecoderConfigurationRecordHeader &configureHeader)
{
	os << "configurationVersion : " << static_cast<unsigned int>(configureHeader.configurationVersion)
	   << "\nAVCProfileIndication : " << static_cast<unsigned int>(configureHeader.AVCProfileIndication)
	   << "\nprofile_compatibility : " << static_cast<unsigned int>(configureHeader.profile_compatibility)
	   << "\nAVCLevelIndication : " << static_cast<unsigned int>(configureHeader.AVCLevelIndication)
	   << "\nlengthSizeMinusOne : " << static_cast<unsigned int>(configureHeader.lengthSizeMinusOne);
	return os;
}
