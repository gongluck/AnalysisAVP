/*
 * @Author: gongluck 
 * @Date: 2020-11-02 23:12:00 
 * @Last Modified by:   gongluck 
 * @Last Modified time: 2020-11-02 23:12:00 
 */

#include "aac.h"

#include <iomanip>

uint16_t get_syncword(const ADTS &adts)
{
	return adts.syncwordH << 8 | adts.syncwordL;
}
void set_syncword(ADTS &adts, uint16_t syncword)
{
	adts.syncwordH = syncword >> 8 & 0x0f;
	adts.syncwordL = syncword & 0xff;
}

uint8_t get_channel_configuration(const ADTS &adts)
{
	return adts.channel_configurationH << 2 | adts.channel_configurationL;
}
void set_channel_configuration(ADTS &adts, uint8_t channel_configuration)
{
	adts.channel_configurationH = channel_configuration >> 2 & 0x01;
	adts.channel_configurationL = channel_configuration & 0x03;
}

uint16_t get_aac_frame_length(const ADTS &adts)
{
	return adts.aac_frame_lengthH << 11 | adts.aac_frame_lengthM << 3 | adts.aac_frame_lengthL;
}
void set_aac_frame_length(ADTS &adts, uint16_t aac_frame_length)
{
	adts.aac_frame_lengthH = (aac_frame_length >> 11) & 0x03;
	adts.aac_frame_lengthM = (aac_frame_length >> 3) & 0xff;
	adts.aac_frame_lengthL = (aac_frame_length)&0x07;
}

uint16_t get_adts_buffer_fullness(const ADTS &adts)
{
	return adts.adts_buffer_fullnessH << 6 | adts.adts_buffer_fullnessL;
}
void set_adts_buffer_fullness(ADTS &adts, uint16_t adts_buffer_fullness)
{
	adts.adts_buffer_fullnessH = adts_buffer_fullness >> 6 & 0x1F;
	adts.adts_buffer_fullnessL = adts_buffer_fullness & 0x3f;
}

const char *adts_parse_id(uint8_t id)
{
	switch (id)
	{
	case ADTS_ID_MPEG4:
		return "MPEG-4";
	case ADTS_ID_MPEG2:
		return "MPEG-2";
	default:
		return "UNKNOWN";
	}
}

const char *adts_parse_profile(uint8_t profile)
{
	switch (profile)
	{
	case ADTS_PROFILE_MAIN:
		return "Main profile";
	case ADTS_PROFILE_LC:
		return "Low Complexity Profile(LC)";
	case ADTS_PROFILE_SSR:
		return "Scalable Sampling Rate profile(SSR)";
	case ADTS_PROFILE_LTP:
		return "AAC LTP";
	case ADTS_PROFILE_SBR:
		return "SBR";
	case ADTS_PROFILE_AACSCALABLE:
		return "AAC scalable";
	case ADTS_PROFILE_TWINVQ:
		return "TwinVQ";
	case ADTS_PROFILE_CELP:
		return "CELP";
	case ADTS_PROFILE_HVXC:
		return "HVXC";
	case ADTS_PROFILE_TTSI:
		return "TTSI";
	case ADTS_PROFILE_MAINSYNTHETIC:
		return "Main synthetic";
	case ADTS_PROFILE_WAVESYNTHETIC:
		return "Wavetable synthesis";
	case ADTS_PROFILE_GENERALMIDI:
		return "General MIDI";
	case ADTS_PROFILE_ALGORITHMICFX:
		return "Algorithmic Synthesis and Audio FX";
	case ADTS_PROFILE_FRAACLC:
		return "FR AAC LC";
	default:
		return "UNKNOWN";
	}
}

const char *adts_parse_channelconfiguration(uint8_t channelconfiguration)
{
	switch (channelconfiguration)
	{
	case ADTS_CHANNEL_CONFIGURATION_AOT:
		return "Defined in AOT Specifc Config";
	case ADTS_CHANNEL_CONFIGURATION_SINGLE:
		return "1 channel : front - center";
	case ADTS_CHANNEL_CONFIGURATION_PAIR:
		return "2 channels : front - left, front - right";
	case ADTS_CHANNEL_CONFIGURATION_SINGLEPAIR:
		return "channels : front - center, front - left, front - right";
	case ADTS_CHANNEL_CONFIGURATION_SINGLEPAIRSINGLE:
		return "channels : front - center, front - left, front - right, back - center";
	case ADTS_CHANNEL_CONFIGURATION_SINGLEPAIRPAIR:
		return "channels : front - center, front - left, front - right, back - left, backright";
	case ADTS_CHANNEL_CONFIGURATION_SINGLEPAIRPAIRLEF:
		return "channels : front - center, front - left, front - right, back - left, backright, LFE - channel";
	case ADTS_CHANNEL_CONFIGURATION_SINGLEPAIRPAIRPAIRLEF:
		return "channels : front - center, front - left, front - right, side - left, side - right, back - left, back - right, LFE - channel";
	default:
		return "UNKNOWN";
	}
}

std::ostream &operator<<(std::ostream &os, const ADTS &adts)
{
	std::ios::fmtflags f(std::cout.flags());
	os << "syncword : " << std::setw(3) << std::hex << static_cast<unsigned int>(get_syncword(adts));
	std::cout.flags(f);
	os << "\nID : " << adts_parse_id(adts.ID)
	   << "\nlayer : " << static_cast<unsigned int>(adts.layer)
	   << "\nprotection_absent : " << static_cast<unsigned int>(adts.protection_absent)
	   << "\nprofile : " << adts_parse_profile(adts.profile)
	   << "\nsampling frequency : " << SamplingFrequencies[adts.sampling_frequency_index]
	   << "\nprivate_bit : " << static_cast<unsigned int>(adts.private_bit)
	   << "\nchannel configuration : " << adts_parse_channelconfiguration(get_channel_configuration(adts))
	   << "\noriginal_copy : " << static_cast<unsigned int>(adts.original_copy)
	   << "\nhome : " << static_cast<unsigned int>(adts.home)
	   << "\ncopyright identification bit : " << static_cast<unsigned int>(adts.copyright_identification_bit)
	   << "\ncopyright identification start : " << static_cast<unsigned int>(adts.copyright_identification_start)
	   << "\naac frame length : " << get_aac_frame_length(adts)
	   << "\nadts buffer fullness : " << get_adts_buffer_fullness(adts)
	   << "\nnumber of raw data blocks : " << static_cast<unsigned int>(adts.number_of_raw_data_blocks);
	return os;
}
