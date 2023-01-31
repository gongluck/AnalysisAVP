/*
 * @Author: gongluck
 * @Date: 2020-11-02 17:08:28
 * @Last Modified by: gongluck
 * @Last Modified time: 2023-01-31 18:15:48
 */

#include "h264.h"

#include <iostream>
#include <iomanip>
#include <fstream>
#include <malloc.h>

int main(int argc, char *argv[])
{
  std::cout << "h264 analysis" << std::endl;

  std::cout << "Usage : "
            << "thisfile h264file." << std::endl;

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

  unsigned char c = 0;
  int32_t datalen = 0;
  int step = 0; // 记录0x00的个数
  while (in.read(reinterpret_cast<char *>(&c), 1))
  {
    if (c == 0)
    {
      ++step;
    }
    else if (c == 1 && step >= 2)
    {
      std::streamoff naluflagsize = step > 2 ? 4 : 3;
      datalen += (step > 3 ? step - 3 : 0);
      if (datalen != 0)
      {
        std::cout << std::string(50, '*').c_str() << std::endl;
        std::cout << "nalu size : " << datalen << std::endl;
#ifdef PRINTDATA
        in.seekg(-naluflagsize - datalen, std::ios::cur);
        unsigned char *naludata = static_cast<unsigned char *>(malloc(datalen));
        if (!in.read(reinterpret_cast<char *>(naludata), datalen))
          break;

        NALHEADER *pnalheader = reinterpret_cast<NALHEADER *>(naludata);
        std::cout << *pnalheader << std::endl;

        std::ios::fmtflags f(std::cout.flags());
        for (int i = 1; i < datalen; ++i)
        {
          std::cout << std::setw(2) << std::setfill('0') << std::hex << static_cast<unsigned int>(naludata[i]) << " ";
        }
        std::cout << std::endl;
        std::cout.flags(f);

        in.seekg(naluflagsize, std::ios::cur);
        free(naludata);
#endif // PRINTDATA
      }
      datalen = 0;
      step = 0;
    }
    else
    {
      datalen += step /*00*/ + 1 /*当前B*/;
      step = 0;
    }
  }

  std::cout << std::string(50, '*').c_str() << std::endl;
  std::cout << "nalu size : " << datalen << std::endl;

#ifdef PRINTDATA
  in.close();

  in.open(argv[1], std::ios::binary);
  in.seekg(-datalen, std::ios::end);
  unsigned char *naludata = static_cast<unsigned char *>(malloc(datalen));
  in.read(reinterpret_cast<char *>(naludata), datalen);

  NALHEADER *pnalheader = reinterpret_cast<NALHEADER *>(naludata);
  std::cout << *pnalheader << std::endl;

  std::ios::fmtflags flags(std::cout.flags());
  for (int i = 0; i < datalen; ++i)
  {
    std::cout << std::setw(2) << std::setfill('0') << std::hex << static_cast<unsigned int>(naludata[i]) << " ";
  }
  std::cout << std::endl;
  std::cout.flags(flags);

  free(naludata);
#endif // PRINTDATA

  in.close();

  ///////////////////////////////////////////////////////////////////////////////////////////////////

  uint8_t data[] = {0x00, 0x00, 0x00, 0x01, 0x12, 0x45, 0x55, 0x55, 0x00, 0x00, 0x01, 0x01, 0x18, 0x45, 0x55, 0x55, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x01, 0x01};
  int8_t nalstep = 0;
  int ret = 0;
  int start = ret;
  while (ret >= 0)
  {
    start = ret;
    ret = findnalu(data, start, sizeof(data), &nalstep);
    if (ret >= 0 && ret < sizeof(data))
    {
      std::ios::fmtflags f(std::cout.flags());
      for (int i = start + 1; i > 0 && i < ret - nalstep + 1; ++i)
      {
        std::cout << std::setw(2) << std::setfill('0') << std::hex << static_cast<unsigned int>(data[i]) << " ";
      }
      std::cout << std::endl;
      std::cout.flags(f);
      nalstep = 0;
    }
  }

  std::ios::fmtflags f(std::cout.flags());
  for (int i = start + 1; i > 0 && i < sizeof(data); ++i)
  {
    std::cout << std::setw(2) << std::setfill('0') << std::hex << static_cast<unsigned int>(data[i]) << " ";
  }
  std::cout << std::endl;
  std::cout.flags(f);

  ///////////////////////////////////////////////////////////////////////////////////////////////////

  in.open(argv[1], std::ios::binary);
  if (in.fail())
  {
    std::cerr << "can not open file " << argv[1] << std::endl;
    return -1;
  }

  char buf[1024] = {0};
  ret = start = 0;
  datalen = 0;
  nalstep = 0;
  int readlen = 0;
  while (in.read(reinterpret_cast<char *>(buf), sizeof(buf)))
  {
    readlen = in.gcount();
    while (ret >= 0)
    {
      start = ret;
      ret = findnalu(reinterpret_cast<uint8_t *>(buf), start, readlen, &nalstep);
      if (ret >= 0 && ret < readlen)
      {
        datalen += ret - start - nalstep;
        if (datalen > 0)
        {
          std::cout << std::string(50, '*').c_str() << std::endl;
          std::cout << "nalu size : " << datalen << std::endl;
        }
        datalen = 0;
        nalstep = 0;
        start = 0;
      }
    }
    ret = 0;
    datalen += readlen - start;
  }

  readlen = in.gcount();
  while (ret >= 0)
  {
    start = ret;
    ret = findnalu(reinterpret_cast<uint8_t *>(buf), start, readlen, &nalstep);
    if (ret >= 0 && ret < readlen)
    {
      datalen += ret - start - nalstep;
      std::cout << std::string(50, '*').c_str() << std::endl;
      std::cout << "nalu size : " << datalen << std::endl;
      datalen = 0;
      nalstep = 0;
      start = 0;
    }
  }
  datalen += readlen - start - 1;
  std::cout << std::string(50, '*').c_str() << std::endl;
  std::cout << "nalu size : " << datalen << std::endl;

  in.close();
  return 0;
}
