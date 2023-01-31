/*
 * @Author: gongluck
 * @Date: 2020-11-02 17:07:19
 * @Last Modified by: gongluck
 * @Last Modified time: 2023-01-31 17:43:39
 */

#ifndef __H264_H__
#define __H264_H__

// Network Abstract Layer Unit 网络抽象层单元
// Advanced Video Codec 高阶视频编码解码器

#include <stdint.h>
#include <iostream>

// NAL ref idc codes
#define NAL_REF_IDC_PRIORITY_HIGHEST 3
#define NAL_REF_IDC_PRIORITY_HIGH 2
#define NAL_REF_IDC_PRIORITY_LOW 1
#define NAL_REF_IDC_PRIORITY_DISPOSABLE 0

// Table 7-1 NAL unit type codes
#define NAL_UNIT_TYPE_UNSPECIFIED 0                  // Unspecified
#define NAL_UNIT_TYPE_CODED_SLICE_NON_IDR 1          // Coded slice of a non-IDR picture
#define NAL_UNIT_TYPE_CODED_SLICE_DATA_PARTITION_A 2 // Coded slice data partition A
#define NAL_UNIT_TYPE_CODED_SLICE_DATA_PARTITION_B 3 // Coded slice data partition B
#define NAL_UNIT_TYPE_CODED_SLICE_DATA_PARTITION_C 4 // Coded slice data partition C
#define NAL_UNIT_TYPE_CODED_SLICE_IDR 5              // Coded slice of an IDR picture
#define NAL_UNIT_TYPE_SEI 6                          // Supplemental enhancement information (SEI)
#define NAL_UNIT_TYPE_SPS 7                          // Sequence parameter set
#define NAL_UNIT_TYPE_PPS 8                          // Picture parameter set
#define NAL_UNIT_TYPE_AUD 9                          // Access unit delimiter
#define NAL_UNIT_TYPE_END_OF_SEQUENCE 10             // End of sequence
#define NAL_UNIT_TYPE_END_OF_STREAM 11               // End of stream
#define NAL_UNIT_TYPE_FILLER 12                      // Filler data
#define NAL_UNIT_TYPE_SPS_EXT 13                     // Sequence parameter set extension
                                                     // 14..18    // Reserved
#define NAL_UNIT_TYPE_CODED_SLICE_AUX 19             // Coded slice of an auxiliary coded picture without partitioning
                                                     // 20..23    // Reserved
#define NAL_RTP_TYPE_STAP_A 24                       // STAP - A Single - time aggregation packet 5.7.1
#define NAL_RTP_TYPE_STAP_B 25                       // STAP - B Single - time aggregation packet 5.7.1
#define NAL_RTP_TYPE_MTAP16 26                       // MTAP16 Multi - time aggregation packet 5.7.2
#define NAL_RTP_TYPE_MTAP24 27                       // MTAP24 Multi - time aggregation packet 5.7.2
#define NAL_RTP_TYPE_FU_A 28                         // FU - A Fragmentation unit 5.8
#define NAL_RTP_TYPE_FU_B 29                         // FU - B Fragmentation unit 5.8
                                                     // 29..31    // Unspecified

typedef struct __NALHEADER
{
  uint8_t nal_unit_type : 5;      // NAL_UNIT_TYPE_XXX，NAL单元的类型
  uint8_t nal_ref_idc : 2;        // NAL_REF_IDC_PRIORITY_XXX，标识重要性，3最高
  uint8_t forbidden_zero_bit : 1; // 必须为0
} NALHEADER;

const char *nal_parse_idc(uint8_t idc);
const char *nal_parse_type(uint8_t type);

#ifdef __cplusplus
std::ostream &operator<<(std::ostream &os, const NALHEADER &nalheader);
#endif

int32_t findnalu(uint8_t *data, uint32_t start, uint32_t end, int8_t *nalstep);

#endif //__H264_H__