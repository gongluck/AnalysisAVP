/*
 * @Author: gongluck 
 * @Date: 2020-11-02 23:16:17 
 * @Last Modified by: gongluck
 * @Last Modified time: 2021-05-18 18:46:59
 */

#include "mp4.h"

#include <iostream>
#include <fstream>

//#define OUTPUTRAW

int main(int argc, char *argv[])
{
	std::cout << "mp4 analysis" << std::endl;

	std::cout << "Usage : "
			  << "thisfile mp4file." << std::endl;

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

	in.close();
	return 0;
}
