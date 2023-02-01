/*
 * @Author: gongluck
 * @Date: 2023-02-01 17:09:02
 * @Last Modified by: gongluck
 * @Last Modified time: 2023-02-01 17:58:34
 */

#ifndef __RTP_H__
#define __RTP_H__

#include <stdint.h>

#ifdef __cplusplus
#include <iostream>
#endif

typedef struct __RTPHEADER
{
  uint8_t rtp_csrc_counts : 4; // CSRC个数
  uint8_t rtp_x_flag : 1;      // X扩展标记位 表示RTP头后是否有自定义扩展头
  uint8_t rtp_p_flag : 1;      // P对齐填充标记
  uint8_t rtp_version : 2;     // RTP版本

  uint8_t rtp_payload_type : 7; // RTP有效载荷类型
  uint8_t rtp_m_flag : 1;       // M标记 对于不同的负载类型有不同含义 荷载H264码流时，某个帧分成多个包进行传输，可以使用该位标记是否为帧的最后一个包

  uint16_t rtp_sequence_num; // RTP序列号
  uint32_t rtp_timestamp;    // 时间戳
  uint32_t rtp_ssrc;         // 同步信源 标识流

  uint32_t rtp_csrcs[]; // 提供信源
} RTPHEADER;

#ifdef __cplusplus
std::ostream &operator<<(std::ostream &os, const RTPHEADER &nalheader);
#endif

#endif //__RTP_H__