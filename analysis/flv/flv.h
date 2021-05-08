/*
 * @Author: gongluck 
 * @Date: 2020-11-02 23:16:05 
 * @Last Modified by:   gongluck 
 * @Last Modified time: 2020-11-02 23:16:05 
 */

#pragma once

#include <stdint.h>
#include <iostream>

//FLV tag type
#define FLV_TAG_TYPE_AUDIO 0x08
#define FLV_TAG_TYPE_VIDEO 0x09
#define FLV_TAG_TYPE_SCRIPT 0x12

//flv video tag frame type
#define FLV_VIDEO_FRAME_KEYFRAME 1	 //keyframe(for AVC, a seekable frame) —— 即H.264的IDR帧；
#define FLV_VIDEO_FRAME_IFRAME 2	 //inter frame(for AVC, a non - seekable frame) —— H.264的普通I帧；
#define FLV_VIDEO_FRAME_DISPOSABLE 3 //disposable inter frame(H.263 only)
#define FLV_VIDEO_FRAME_GENERATED 4	 //generated keyframe(reserved for server use only)
#define FLV_VIDEO_FRAME_COMMAND 5	 //video info / command frame

//flv video tag frame codecid
#define FLV_VIDEO_CODECID_JPEG 1	//JPEG (currently unused)
#define FLV_VIDEO_CODECID_H263 2	//Sorenson H.263
#define FLV_VIDEO_CODECID_SCREEN 3	//Screen video
#define FLV_VIDEO_CODECID_ON2VP6 4	//On2 VP6
#define FLV_VIDEO_CODECID_ON2VP6A 5 //On2 VP6 with alpha channel
#define FLV_VIDEO_CODECID_SCREEN2 6 //Screen video version 2
#define FLV_VIDEO_CODECID_AVC 7		//AVC

//AVC packet type
#define AVC_PACKET_HEADER 0 //AVC sequence header
#define AVC_PACKET_NALU 1	//AVC NALU
#define AVC_PACKET_END 2	//AVC end of sequence

//flv sound format
#define FLV_SOUND_FORMAT_PCM 0				//Linear PCM, platform endian
#define FLV_SOUND_FORMAT_ADPCM 1			//ADPCM
#define FLV_SOUND_FORMAT_MP3 2				//MP3
#define FLV_SOUND_FORMAT_PCMLE 3			//inear PCM, little endian
#define FLV_SOUND_FORMAT_NELLYMOSER16MONO 4 //Nellymoser 16-kHz mono
#define FLV_SOUND_FORMAT_NELLYMOSER8MONO 5	//Nellymoser 8-kHz mono
#define FLV_SOUND_FORMAT_NELLYMOSER 6		//Nellymoser
#define FLV_SOUND_FORMAT_G711LA 7			//G.711A-law logarithmic PCM
#define FLV_SOUND_FORMAT_G711MU 8			//G.711mu-law logarithmic PCM
#define FLV_SOUND_FORMAT_RESERVED 9			//reserved
#define FLV_SOUND_FORMAT_AAC 10				//AAC
#define FLV_SOUND_FORMAT_SPEEX 11			//Speex
#define FLV_SOUND_FORMAT_MP3_8 14			//MP3 8-Khz
#define FLV_SOUND_FORMAT_DEVICE 15			//Device-specific sound

//flv sound rate
#define FLV_SOUND_RATE_55 0 //5.5-kHz
#define FLV_SOUND_RATE_11 1 //11-kHz
#define FLV_SOUND_RATE_22 2 //22-kHz
#define FLV_SOUND_RATE_44 3 //44-kHz

//flv sound size
#define FLV_SOUND_SIZE_8 0	//8Bit
#define FLV_SOUND_SIZE_16 1 //16Bit

//flv audio tag sound type
#define FLV_AUDIO_SOUND_MONO 0	 //单声道
#define FLV_AUDIO_SOUND_STEREO 1 //双声道

//aac packet type
#define AAC_PACKET_TYPE_HEAD 0 //AAC sequence header
#define AAC_PACKET_TYPE_RAW 1  //raw

#pragma pack(1)
typedef struct __GINT16
{
	uint8_t data1;
	uint8_t data2;
} FLVINT16;
typedef struct __GINT24
{
	uint8_t data1;
	uint8_t data2;
	uint8_t data3;
} FLVINT24;
typedef struct __GINT32
{
	uint8_t data1;
	uint8_t data2;
	uint8_t data3;
	uint8_t data4;
} FLVINT32;
typedef struct __GTIMESTAMP
{
	uint8_t data2;
	uint8_t data3;
	uint8_t data4;
	uint8_t data1;
} FLVTIMESTAMP;

/*FLV文件头*/
typedef struct __FLVHEADER
{
	uint8_t F;			  //0x46
	uint8_t L;			  //0x4C
	uint8_t V;			  //0x56
	uint8_t flvtype;	  //版本
	uint8_t hasvideo : 1; //是否有视频
	uint8_t : 1;		  //全为0
	uint8_t hasaudio : 1; //是否有音频
	uint8_t : 5;		  //全为0
	FLVINT32 headlen;	  //文件头长度9
} FLVHEADER;
/*****************************/
/*前一个tag的长度(4字节)*/
/*****************************/
/*tag头*/
typedef struct __FLVTAGHEADER
{
	uint8_t flvtagtype;		//FLV_TAG_TYPE_XXX
	FLVINT24 datalen;		//数据区的长度
	FLVTIMESTAMP timestamp; //时间戳
	FLVINT24 streamsid;		//流信息
} FLVTAGHEADER;
/*****************************/
/*tag数据区*/
typedef struct __FLVVIDEOTAG
{
	uint8_t codecid : 4; //编解码器，FLV_VIDEO_CODECID_XXX
	uint8_t type : 4;	 //视频帧类型，FLV_VIDEO_FRAME_XXX
	union
	{
		//codecid == FLV_VIDEO_CODECID_AVC
		struct AVCVIDEOPACKE
		{
			uint8_t avcpacketype; //AVC_PACKET_XXX

			//如果avcpacketype=1，则为时间cts偏移量；否则，为0。当B帧的存在时，视频解码呈现过程中，dts、pts可能不同，cts的计算公式为 pts - dts/90，单位为毫秒；如果B帧不存在，则cts固定为0。
			FLVINT24 compositiontime;

			//avcpacketype=0，则为AVCDecoderConfigurationRecord，H.264 视频解码所需要的参数集（SPS、PPS）
			//avcpacketype=1，则为NALU（一个或多个），data[0-3]是数据长度！
			//如果avcpacketype=2，则为空
			unsigned char avcpacketdata[];
		} avcvideopacket;
	} videopacket;
} FLVVIDEOTAG;
//AVCDecoderConfigurationRecord = AVCDecoderConfigurationRecordHeader + SequenceParameterSet + PictureParameterSet
typedef struct __AVCDecoderConfigurationRecordHeader
{
	uint8_t configurationVersion;
	uint8_t AVCProfileIndication;
	uint8_t profile_compatibility;
	uint8_t AVCLevelIndication;
	uint8_t lengthSizeMinusOne : 2;
	uint8_t : 6;
	uint8_t data[]; //后续数据
} AVCDecoderConfigurationRecordHeader;
typedef struct __SequenceParameterSet
{
	uint8_t numOfSequenceParameterSets : 5;
	uint8_t : 3;
	FLVINT16 sequenceParameterSetLength;
	uint8_t sequenceParameterSetNALUnit[];
} SequenceParameterSet;
typedef struct __PictureParameterSet
{
	uint8_t numOfPictureParameterSets;
	FLVINT16 pictureParameterSetLength;
	uint8_t pictureParameterSetNALUnit[];
} PictureParameterSet;
/*****************************/
/*...........................*/
typedef struct __FLVAUDIOTAG
{
	uint8_t soundtype : 1;	 //FLV_AUDIO_SOUND_XXX
	uint8_t soundSize : 1;	 //FLV_SOUND_SIZE_XXX
	uint8_t soundRate : 2;	 //FLV_SOUND_RATE_XXX
	uint8_t soundFormat : 4; //FLV_SOUND_FORMAT_XXX
	union
	{
		//soundFormat == FLV_SOUND_FORMAT_AAC
		struct AACAUDIOPACKET
		{
			uint8_t aacpackettype; //AAC_PACKET_TYPE_XXX
			uint8_t data[];
		} aacaudiopacket;
	} audiopacket;
} FLVAUDIOTAG;
typedef struct __AudioSpecificConfig
{
	uint8_t SamplingFrequencyIndexH : 3;
	uint8_t AudioObjectType : 5;
	uint8_t : 3;
	uint8_t ChannelConfiguration : 4;
	uint8_t SamplingFrequencyIndexL : 1;
	uint8_t AOTSpecificConfig[];
} AudioSpecificConfig;
/*...........................*/
/*前一个tag的长度(4字节)*/
/*EOF*/

#pragma pack()

#define FLVINT16TOINT(x) (x.data1 << 8 | x.data2)
#define FLVINT24TOINT(x) (x.data1 << 16 | x.data2 << 8 | x.data3)
#define FLVINT32TOINT(x) (x.data1 << 24 | x.data2 << 16 | x.data3 << 8 | x.data4)
#define FVLSAMPLEFREQUENCYINDEX(audiospecificconfig) ((audiospecificconfig.SamplingFrequencyIndexH << 1) | audiospecificconfig.SamplingFrequencyIndexL)

const char *flv_tag_parse_type(uint8_t type);
const char *flv_video_parse_type(uint8_t type);
const char *flv_video_parse_codecid(uint8_t codecid);
const char *flv_audio_parse_type(uint8_t type);
const char *flv_audio_parse_soundsize(uint8_t soundsize);
const char *flv_audio_parse_soundrate(uint8_t rate);
const char *flv_audio_parse_soundformat(uint8_t format);
const char *avc_packet_parse_type(uint8_t type);

std::ostream &operator<<(std::ostream &os, const FLVHEADER &flvheader);
std::ostream &operator<<(std::ostream &os, const FLVTAGHEADER &tagheader);
std::ostream &operator<<(std::ostream &os, const FLVVIDEOTAG &videotag);
std::ostream &operator<<(std::ostream &os, const AVCDecoderConfigurationRecordHeader &configureHeader);
std::ostream &operator<<(std::ostream &os, const SequenceParameterSet &sps);
std::ostream &operator<<(std::ostream &os, const PictureParameterSet &sps);
std::ostream &operator<<(std::ostream &os, const FLVAUDIOTAG &audiotag);
std::ostream &operator<<(std::ostream &os, const AudioSpecificConfig &audiotag);
