/*
 * @Author: gongluck
 * @Date: 2020-11-02 17:08:28
 * @Last Modified by: gongluck
 * @Last Modified time: 2023-02-01 17:58:48
 */

#include "rtp.h"

#include <iostream>
#include <fstream>

int main(int argc, char *argv[])
{
  std::cout << "rtp analysis" << std::endl;

  std::cout << "Usage : "
            << "thisfile rtpfile." << std::endl;

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

  RTPHEADER rtpheader = {0};
  in.read(reinterpret_cast<char *>(&rtpheader), sizeof(rtpheader));
  std::cout << rtpheader << std::endl;

  in.close();
  return 0;
}
