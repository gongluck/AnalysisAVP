/*
 * @Author: gongluck
 * @Date: 2023-02-01 17:26:58
 * @Last Modified by: gongluck
 * @Last Modified time: 2023-02-01 18:04:26
 */

#include "rtp.h"

#ifdef _WIN32
#include <winsock.h>
#pragma comment(lib, "Ws2_32")
#elif defined(ANDROID)
#include <arpa/inet.h>
#endif

std::ostream &operator<<(std::ostream &os, const RTPHEADER &rtpheader)
{
  os << "rtp_version : " << (int)rtpheader.rtp_version
     << "\nrtp_p_flag : " << (int)rtpheader.rtp_p_flag
     << "\nrtp_x_flag : " << (int)rtpheader.rtp_x_flag
     << "\nrtp_csrc_counts : " << (int)rtpheader.rtp_csrc_counts
     << "\nrtp_m_flag : " << (int)rtpheader.rtp_m_flag
     << "\nrtp_payload_type : " << (int)rtpheader.rtp_payload_type
     << "\nrtp_sequence_num : " << ntohs(rtpheader.rtp_sequence_num)
     << "\nrtp_timestamp : " << ntohl(rtpheader.rtp_timestamp)
     << "\nrtp_ssrc : " << ntohl(rtpheader.rtp_ssrc);
  for (int i = 0; i < rtpheader.rtp_csrc_counts; ++i)
  {
    os << "rtp_csrc " << i << " : " << ntohl(rtpheader.rtp_csrcs[i]);
  }
  return os;
}