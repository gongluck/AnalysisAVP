/*
 * @Author: gongluck 
 * @Date: 2020-11-02 23:11:22 
 * @Last Modified by:   gongluck 
 * @Last Modified time: 2020-11-02 23:11:22 
 */

#pragma once

//Advanced Audio Coding(⾼级⾳频编码)

#include <stdint.h>
#include <iostream>

//ADTS ID
#define ADTS_ID_MPEG4 0 //标识MPEG-4
#define ADTS_ID_MPEG2 1 //标识MPEG-2

//ADTS profile
#define ADTS_PROFILE_MAIN 0			  //Main profile
#define ADTS_PROFILE_LC 1			  //Low Complexity Profile(LC)
#define ADTS_PROFILE_SSR 2			  //Scalable Sampling Rate profile(SSR)
#define ADTS_PROFILE_LTP 3			  //AAC LTP
#define ADTS_PROFILE_SBR 4			  //SBR
#define ADTS_PROFILE_AACSCALABLE 5	  //AAC scalable
#define ADTS_PROFILE_TWINVQ 6		  //TwinVQ
#define ADTS_PROFILE_CELP 7			  //CELP
#define ADTS_PROFILE_HVXC 8			  //HVXC
									  //9..10	//(reserved)
#define ADTS_PROFILE_TTSI 11		  //TTSI
#define ADTS_PROFILE_MAINSYNTHETIC 12 //Main synthetic
#define ADTS_PROFILE_WAVESYNTHETIC 13 //Wavetable synthesis
#define ADTS_PROFILE_GENERALMIDI 14	  //General MIDI
#define ADTS_PROFILE_ALGORITHMICFX 15 //Algorithmic Synthesis and Audio FX
#define ADTS_PROFILE_FRAACLC 16		  //FR AAC LC

//ADTS sampling frequencies
const int SamplingFrequencies[16] = {
	96000,
	88200,
	64000,
	48000,
	44100,
	32000,
	24000,
	22050,
	16000,
	12000,
	11025,
	8000,
	7350,
	-1 /*reserved*/,
	-1 /*reserved*/,
	-1 /*escape value*/,
};

//ADTS channel configuration
#define ADTS_CHANNEL_CONFIGURATION_AOT 0				   //Defined in AOT Specifc Config
#define ADTS_CHANNEL_CONFIGURATION_SINGLE 1				   //1 channel : front - center
#define ADTS_CHANNEL_CONFIGURATION_PAIR 2				   //2 channels : front - left, front - right
#define ADTS_CHANNEL_CONFIGURATION_SINGLEPAIR 3			   //channels : front - center, front - left, front - right
#define ADTS_CHANNEL_CONFIGURATION_SINGLEPAIRSINGLE 4	   //channels : front - center, front - left, front - right, back - center
#define ADTS_CHANNEL_CONFIGURATION_SINGLEPAIRPAIR 5		   //channels : front - center, front - left, front - right, back - left, backright
#define ADTS_CHANNEL_CONFIGURATION_SINGLEPAIRPAIRLEF 6	   //channels : front - center, front - left, front - right, back - left, backright, LFE - channel
#define ADTS_CHANNEL_CONFIGURATION_SINGLEPAIRPAIRPAIRLEF 8 //channels : front - center, front - left, front - right, side - left, side - right, back - left, back - right, LFE - channel
														   //8..15	//Reserved

typedef struct __ADTS
{
	//16bit
	uint8_t syncwordL : 8;		   //同步头低8位 总是0xFF, all bits must be 1，代表着⼀个ADTS帧的开始
	uint8_t protection_absent : 1; //表示是否误码校验。Warning, set to 1 if there is no CRC and 0 if there is CRC
	uint8_t layer : 2;			   //always: '00'
	uint8_t ID : 1;				   //MPEG标识符，ADTS_ID_XXX
	uint8_t syncwordH : 4;		   //同步头高4位 总是0xF, all bits must be 1，代表着⼀个ADTS帧的开始

	//8bit
	uint8_t channel_configurationH : 1; //表示声道数高1位
	uint8_t private_bit : 1;
	uint8_t sampling_frequency_index : 4; //表示使⽤的采样率下标，通过这个下标在SamplingFrequencies[]数组中查找得知采样率的值
	uint8_t profile : 2;				  //表示使⽤哪个级别的AAC，ADTS_PROFILE_XXX。有些芯⽚只⽀持AAC LC。

	//8bit
	uint8_t aac_frame_lengthH : 2;
	uint8_t copyright_identification_start : 1;
	uint8_t copyright_identification_bit : 1;
	uint8_t home : 1;
	uint8_t original_copy : 1;
	uint8_t channel_configurationL : 2; //表示声道数低2位

	//24bit
	uint8_t aac_frame_lengthM : 8;
	uint8_t adts_buffer_fullnessH : 5;
	uint8_t aac_frame_lengthL : 3;		   //⼀个ADTS帧的⻓度包括ADTS头和AAC原始流.
										   //frame length, this value must include 7 or 9 bytes of header length :
										   //aac_frame_length = (protection_absent == 1 ? 7 : 9) + size(AACFrame)
										   //protection_absent = 0时, header length = 9bytes
										   //protection_absent = 1时, header length = 7bytes
	uint8_t number_of_raw_data_blocks : 2; //表示ADTS帧中有number_of_raw_data_blocks_in_frame + 1个AAC原始帧。
										   //所以说number_of_raw_data_blocks_in_frame == 0 表示说ADTS帧中有⼀个AAC数据块。
	uint8_t adts_buffer_fullnessL : 6;	   //0x7FF 说明是码率可变的码流。
} ADTS;

uint16_t get_syncword(const ADTS &adts);
void set_syncword(ADTS &adts, uint16_t syncword);
uint8_t get_channel_configuration(const ADTS &adts);
void set_channel_configuration(ADTS &adts, uint8_t channel_configuration);
uint16_t get_aac_frame_length(const ADTS &adts);
void set_aac_frame_length(ADTS &adts, uint16_t aac_frame_length);
uint16_t get_adts_buffer_fullness(const ADTS &adts);
void set_adts_buffer_fullness(ADTS &adts, uint16_t adts_buffer_fullness);

const char *adts_parse_id(uint8_t id);
const char *adts_parse_profile(uint8_t profile);
const char *adts_parse_channelconfiguration(uint8_t channelconfiguration);

std::ostream &operator<<(std::ostream &os, const ADTS &adts);
