/*
 * @Author: gongluck
 * @Date: 2020-11-02 23:16:05
 * @Last Modified by: gongluck
 * @Last Modified time: 2021-05-18 18:54:22
 */

#pragma once

#include <iostream>
#include "../common.h"

// FLV tag type
#define FLV_TAG_TYPE_AUDIO 0x08
#define FLV_TAG_TYPE_VIDEO 0x09
#define FLV_TAG_TYPE_SCRIPT 0x12

// flv video tag frame type
#define FLV_VIDEO_FRAME_KEYFRAME 1   // keyframe(for AVC, a seekable frame) —— 即H.264的IDR帧；
#define FLV_VIDEO_FRAME_IFRAME 2     // inter frame(for AVC, a non - seekable frame) —— H.264的普通I帧；
#define FLV_VIDEO_FRAME_DISPOSABLE 3 // disposable inter frame(H.263 only)
#define FLV_VIDEO_FRAME_GENERATED 4  // generated keyframe(reserved for server use only)
#define FLV_VIDEO_FRAME_COMMAND 5    // video info / command frame

// flv video tag frame codecid
#define FLV_VIDEO_CODECID_JPEG 1    // JPEG (currently unused)
#define FLV_VIDEO_CODECID_H263 2    // Sorenson H.263
#define FLV_VIDEO_CODECID_SCREEN 3  // Screen video
#define FLV_VIDEO_CODECID_ON2VP6 4  // On2 VP6
#define FLV_VIDEO_CODECID_ON2VP6A 5 // On2 VP6 with alpha channel
#define FLV_VIDEO_CODECID_SCREEN2 6 // Screen video version 2
#define FLV_VIDEO_CODECID_AVC 7     // AVC

// AVC packet type
#define AVC_PACKET_HEADER 0 // AVC sequence header
#define AVC_PACKET_NALU 1   // AVC NALU
#define AVC_PACKET_END 2    // AVC end of sequence

// flv sound format
#define FLV_SOUND_FORMAT_PCM 0              // Linear PCM, platform endian
#define FLV_SOUND_FORMAT_ADPCM 1            // ADPCM
#define FLV_SOUND_FORMAT_MP3 2              // MP3
#define FLV_SOUND_FORMAT_PCMLE 3            // inear PCM, little endian
#define FLV_SOUND_FORMAT_NELLYMOSER16MONO 4 // Nellymoser 16-kHz mono
#define FLV_SOUND_FORMAT_NELLYMOSER8MONO 5  // Nellymoser 8-kHz mono
#define FLV_SOUND_FORMAT_NELLYMOSER 6       // Nellymoser
#define FLV_SOUND_FORMAT_G711LA 7           // G.711A-law logarithmic PCM
#define FLV_SOUND_FORMAT_G711MU 8           // G.711mu-law logarithmic PCM
#define FLV_SOUND_FORMAT_RESERVED 9         // reserved
#define FLV_SOUND_FORMAT_AAC 10             // AAC
#define FLV_SOUND_FORMAT_SPEEX 11           // Speex
#define FLV_SOUND_FORMAT_MP3_8 14           // MP3 8-Khz
#define FLV_SOUND_FORMAT_DEVICE 15          // Device-specific sound

// flv sound rate
#define FLV_SOUND_RATE_55 0 // 5.5-kHz
#define FLV_SOUND_RATE_11 1 // 11-kHz
#define FLV_SOUND_RATE_22 2 // 22-kHz
#define FLV_SOUND_RATE_44 3 // 44-kHz

// flv sound size
#define FLV_SOUND_SIZE_8 0  // 8Bit
#define FLV_SOUND_SIZE_16 1 // 16Bit

// flv audio tag sound type
#define FLV_AUDIO_SOUND_MONO 0   //单声道
#define FLV_AUDIO_SOUND_STEREO 1 //双声道

// aac packet type
#define AAC_PACKET_TYPE_HEAD 0 // AAC sequence header
#define AAC_PACKET_TYPE_RAW 1  // raw

#pragma pack(1)
typedef GINT8 FLVINT8;
typedef GINT16 FLVINT16;
typedef GINT24 FLVINT24;
typedef GINT32 FLVINT32;
typedef struct __FLVTIMESTAMP
{
  uint8_t data2;
  uint8_t data3;
  uint8_t data4;
  uint8_t data1;
} FLVTIMESTAMP;

/*FLV文件头*/
typedef struct __FLVHEADER
{
  FLVINT8 F;            // 0x46
  FLVINT8 L;            // 0x4C
  FLVINT8 V;            // 0x56
  FLVINT8 flvtype;      //版本
  FLVINT8 hasvideo : 1; //是否有视频
  FLVINT8 : 1;          //全为0
  FLVINT8 hasaudio : 1; //是否有音频
  FLVINT8 : 5;          //全为0
  FLVINT32 headlen;     //文件头长度9
} FLVHEADER;
/*****************************/
/*前一个tag的长度(4字节)*/
/*****************************/
/*tag头*/
typedef struct __FLVTAGHEADER
{
  FLVINT8 flvtagtype;     // FLV_TAG_TYPE_XXX
  FLVINT24 datalen;       //数据区的长度
  FLVTIMESTAMP timestamp; //时间戳
  FLVINT24 streamsid;     //流信息
} FLVTAGHEADER;
/*****************************/
/*tag数据区*/
typedef struct __FLVVIDEOTAG
{
  FLVINT8 codecid : 4; //编解码器，FLV_VIDEO_CODECID_XXX
  FLVINT8 type : 4;    //视频帧类型，FLV_VIDEO_FRAME_XXX
  union
  {
    // codecid == FLV_VIDEO_CODECID_AVC
    struct AVCVIDEOPACKE
    {
      FLVINT8 avcpacketype; // AVC_PACKET_XXX

      //如果avcpacketype=1，则为时间cts偏移量；否则，为0。当B帧的存在时，视频解码呈现过程中，dts、pts可能不同，cts的计算公式为 pts - dts/90，单位为毫秒；如果B帧不存在，则cts固定为0。
      FLVINT24 compositiontime;

      // avcpacketype=0，则为AVCDecoderConfigurationRecord，H.264 视频解码所需要的参数集（SPS、PPS）
      // avcpacketype=1，则为NALU（一个或多个），data[0-3]是数据长度！
      //如果avcpacketype=2，则为空
      FLVINT8 avcpacketdata[0];
    } avcvideopacket;
  } videopacket;
} FLVVIDEOTAG;
// AVCDecoderConfigurationRecord = AVCDecoderConfigurationRecordHeader + SequenceParameterSet + PictureParameterSet
typedef struct __AVCDecoderConfigurationRecordHeader
{
  FLVINT8 configurationVersion;
  FLVINT8 AVCProfileIndication;
  FLVINT8 profile_compatibility;
  FLVINT8 AVCLevelIndication;
  FLVINT8 lengthSizeMinusOne : 2;
  FLVINT8 : 6;
  FLVINT8 data[0]; //后续数据
} AVCDecoderConfigurationRecordHeader;
typedef struct __SequenceParameterSet
{
  FLVINT8 numOfSequenceParameterSets : 5;
  FLVINT8 : 3;
  FLVINT16 sequenceParameterSetLength;
  FLVINT8 sequenceParameterSetNALUnit[0];
} SequenceParameterSet;
typedef struct __PictureParameterSet
{
  FLVINT8 numOfPictureParameterSets;
  FLVINT16 pictureParameterSetLength;
  FLVINT8 pictureParameterSetNALUnit[0];
} PictureParameterSet;
/*****************************/
/*...........................*/
typedef struct __FLVAUDIOTAG
{
  FLVINT8 soundtype : 1;   // FLV_AUDIO_SOUND_XXX
  FLVINT8 soundSize : 1;   // FLV_SOUND_SIZE_XXX
  FLVINT8 soundRate : 2;   // FLV_SOUND_RATE_XXX
  FLVINT8 soundFormat : 4; // FLV_SOUND_FORMAT_XXX
  union
  {
    // soundFormat == FLV_SOUND_FORMAT_AAC
    struct AACAUDIOPACKET
    {
      FLVINT8 aacpackettype; // AAC_PACKET_TYPE_XXX
      FLVINT8 data[0];
    } aacaudiopacket;
  } audiopacket;
} FLVAUDIOTAG;
typedef struct __AudioSpecificConfig
{
  FLVINT8 SamplingFrequencyIndexH : 3;
  FLVINT8 AudioObjectType : 5;
  FLVINT8 : 3;
  FLVINT8 ChannelConfiguration : 4;
  FLVINT8 SamplingFrequencyIndexL : 1;
  FLVINT8 AOTSpecificConfig[0];
} AudioSpecificConfig;
/*...........................*/
/*前一个tag的长度(4字节)*/
/*EOF*/

#pragma pack()

#define FLVINT16TOINT GINT16TOINT
#define FLVINT24TOINT GINT24TOINT
#define FLVINT32TOINT GINT32TOINT
#define FVLSAMPLEFREQUENCYINDEX(audiospecificconfig) ((audiospecificconfig.SamplingFrequencyIndexH << 1) | audiospecificconfig.SamplingFrequencyIndexL)

const char *flv_tag_parse_type(FLVINT8 type);
const char *flv_video_parse_type(FLVINT8 type);
const char *flv_video_parse_codecid(FLVINT8 codecid);
const char *flv_audio_parse_type(FLVINT8 type);
const char *flv_audio_parse_soundsize(FLVINT8 soundsize);
const char *flv_audio_parse_soundrate(FLVINT8 rate);
const char *flv_audio_parse_soundformat(FLVINT8 format);
const char *avc_packet_parse_type(FLVINT8 type);

std::ostream &operator<<(std::ostream &os, const FLVHEADER &flvheader);
std::ostream &operator<<(std::ostream &os, const FLVTAGHEADER &tagheader);
std::ostream &operator<<(std::ostream &os, const FLVVIDEOTAG &videotag);
std::ostream &operator<<(std::ostream &os, const AVCDecoderConfigurationRecordHeader &configureHeader);
std::ostream &operator<<(std::ostream &os, const SequenceParameterSet &sps);
std::ostream &operator<<(std::ostream &os, const PictureParameterSet &sps);
std::ostream &operator<<(std::ostream &os, const FLVAUDIOTAG &audiotag);
std::ostream &operator<<(std::ostream &os, const AudioSpecificConfig &audiotag);
