/*
 * @Author: gongluck
 * @Date: 2021-10-25 16:43:06
 * @Last Modified by: gongluck
 * @Last Modified time: 2021-10-27 16:13:26
 */

#include <iostream>
#include <fstream>
#include <libde265/de265.h>

int main(int argc, char *argv[])
{
	std::cout << "libde265 demo" << std::endl;
	std::cout << "Usage : "
			  << "thisfilename h265file outfile" << std::endl;
	if (argc < 3)
	{
		std::cerr << "please see the usage message." << std::endl;
		return -1;
	}
	std::ifstream h265in(argv[1], std::ios::binary);
	if (h265in.fail())
	{
		std::cerr << "can not open file " << argv[1] << std::endl;
		return -1;
	}
	std::ofstream outfile(argv[2], std::ios::binary);

	size_t readsize = 1024;
	char *indata = static_cast<char *>(malloc(readsize));

	de265_error err;
	auto hdecoder = de265_new_decoder();
	while (h265in.read(indata, readsize))
	{
		err = de265_push_data(hdecoder, indata, readsize, 0, nullptr);
		if (err != DE265_OK)
		{
			break;
		}

		// decode
		while (true)
		{
			/*
			- decoding successful->err = DE265_OK, more = true
			- decoding stalled->err != DE265_OK, more = true
			- decoding finished->err = DE265_OK, more = false
			- unresolvable error->err != DE265_OK, more = false
			*/
			int more = true;
			err = de265_decode(hdecoder, &more);
			if (err != DE265_OK && more == false)
			{
				break;
			}

			auto img = de265_get_next_picture(hdecoder);
			if (img == nullptr)
			{
				break;
			}

			for (int i = 0; i < 3; ++i)
			{
				int stride;
				auto data = de265_get_image_plane(img, i, &stride);
				auto width = de265_get_image_width(img, i);
				for (int y = 0; y < de265_get_image_height(img, i); ++y)
				{
					outfile.write(reinterpret_cast<const char *>(data) + y * stride, width);
					outfile.flush();
				}
			}
		}
	}

	auto left = h265in.gcount();
	err = de265_push_data(hdecoder, indata, readsize, 0, nullptr);
	err = de265_flush_data(hdecoder);
	// decode
	// decode
	while (true)
	{
		/*
		- decoding successful->err = DE265_OK, more = true
		- decoding stalled->err != DE265_OK, more = true
		- decoding finished->err = DE265_OK, more = false
		- unresolvable error->err != DE265_OK, more = false
		*/
		int more = true;
		err = de265_decode(hdecoder, &more);
		if (err != DE265_OK && more == false)
		{
			break;
		}

		auto img = de265_get_next_picture(hdecoder);
		if (img == nullptr)
		{
			break;
		}

		for (int i = 0; i < 3; ++i)
		{
			int stride;
			auto data = de265_get_image_plane(img, i, &stride);
			auto width = de265_get_image_width(img, i);
			for (int y = 0; y < de265_get_image_height(img, i); ++y)
			{
				outfile.write(reinterpret_cast<const char*>(data) + y * stride, width);
				outfile.flush();
			}
		}
	}

	err = de265_free_decoder(hdecoder);

	free(indata);
	outfile.close();
	h265in.close();
	return 0;
}