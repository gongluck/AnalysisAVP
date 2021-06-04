/*
 * @Author: gongluck 
 * @Date: 2021-01-23 14:12:40 
 * @Last Modified by: gongluck
 * @Last Modified time: 2021-06-04 17:05:35
 */

#define SDL_MAIN_HANDLED

#include <iostream>
#include <fstream>
#include "SDL.h"

#define MY_QUIT		SDL_USEREVENT+1
#define MY_REFRESH	SDL_USEREVENT+2

const int WIDTH = 500;
const int HEIGHT = 300;
const int width = 10;
const int height = 10;

bool exitflag = false;

int mythread(void* param)
{
	while (!exitflag)
	{
		SDL_Event event;
		event.type = MY_REFRESH;
		SDL_PushEvent(&event);
		SDL_Delay(100);
	}

	SDL_Event event;
	event.type = MY_QUIT;
	SDL_PushEvent(&event);

	return 0;
}
void SDLCALL mypcm(void* userdata, Uint8* stream, int len)
{
	auto pcm = static_cast<std::iostream*>(userdata);
	auto buf = static_cast<char*>(malloc(len));
	pcm->read(buf, len);
	if (!pcm)
	{
		free(buf);
		return;
	}
	memcpy(stream, buf, len);
	free(buf);
}

//SDL2_example ../../../../media/gx_yuv420p_320x240.yuv 320 240 ../../../../media/gx_44100_2_s16le.pcm
int main(int argc, char* argv[])
{
	std::cout << "SDL2 demo" << std::endl;
	std::cout << "Usage : " << "thisfilename YUVfile width height PCMfile" << std::endl;
	if (argc < 5)
	{
		std::cerr << "please see the usage message." << std::endl;
		return -1;
	}
	std::ifstream yuv(argv[1], std::ios::binary);
	if (yuv.fail())
	{
		std::cerr << "can not open file " << argv[1] << std::endl;
		return -1;
	}
	auto yuvwidth = atoi(argv[2]);
	auto yuvheight = atoi(argv[3]);
	std::ifstream pcm(argv[4], std::ios::binary);
	if (pcm.fail())
	{
		std::cerr << "can not open file " << argv[4] << std::endl;
		return -1;
	}

	auto ret = SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
	SDL_Window* window = SDL_CreateWindow("SDL2", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WIDTH, HEIGHT, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
	auto renderer = SDL_CreateRenderer(window, -1, 0);
	auto texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, WIDTH, HEIGHT);

	int i = 0;
	while (i++ < 20)
	{
		ret = SDL_SetRenderTarget(renderer, texture);
		ret = SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
		ret = SDL_RenderClear(renderer);
		SDL_Rect rect = { rand() % (WIDTH - width), rand() % (HEIGHT - height), width, height };
		ret = SDL_RenderDrawRect(renderer, &rect);
		ret = SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
		ret = SDL_RenderFillRect(renderer, &rect);

		ret = SDL_SetRenderTarget(renderer, nullptr);
		ret = SDL_RenderCopy(renderer, texture, nullptr, nullptr);

		SDL_RenderPresent(renderer);
		SDL_Delay(100);
	}

	auto yuvtexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING, yuvwidth, yuvheight);
	auto datasize = yuvwidth * yuvheight * 3 / 2;
	auto yuvdata = static_cast<char*>(malloc(datasize));
	auto th = SDL_CreateThread(mythread, nullptr, nullptr);

	SDL_AudioSpec spec = { 0 };
	spec.freq = 44100;
	spec.format = AUDIO_S16SYS;
	spec.channels = 2;
	spec.silence = 0;
	spec.samples = 1024;
	spec.callback = mypcm;
	spec.userdata = &pcm;
	ret = SDL_OpenAudio(&spec, nullptr);
	SDL_PauseAudio(0);

	SDL_Event event = { 0 };
	while (!exitflag)
	{
		ret = SDL_WaitEvent(&event);
		switch (event.type)
		{
		case SDL_KEYDOWN:
			if (event.key.keysym.sym >= SDLK_a && event.key.keysym.sym <= SDLK_z)
			{
				std::cout << char('a' + event.key.keysym.sym - SDLK_a) << " down" << std::endl;
			}
			else if (event.key.keysym.sym == SDLK_ESCAPE)
			{
				SDL_Event event_q;
				event_q.type = MY_QUIT;
				ret = SDL_PushEvent(&event_q);
				break;
			}
			break;
		case SDL_MOUSEBUTTONDOWN:
			if (event.button.button == SDL_BUTTON_LEFT)
			{
				std::cout << "mouse left down" << std::endl;
			}
			else if (event.button.button == SDL_BUTTON_RIGHT)
			{
				std::cout << "mouse right down" << std::endl;
			}
			else
			{
				std::cout << "mouse down" << std::endl;
			}
			break;
		case SDL_MOUSEMOTION:
			std::cout << "mouse move " << event.button.x << ", " << event.button.y << std::endl;
			break;
		case MY_REFRESH:
		{
			yuv.read(yuvdata, datasize);
			if (!yuv)
			{
				exitflag = true;
				break;
			}

			SDL_UpdateTexture(yuvtexture, nullptr, yuvdata, yuvwidth);
			SDL_RenderClear(renderer);
			//SDL_RenderCopy(renderer, yuvtexture, nullptr, nullptr);

			//旋转90°并且铺满渲染区域
			SDL_Point center = { yuvwidth, 0 };//src坐标系
			SDL_Rect  dstrect;//旋转后src坐标系
			dstrect.x = -yuvwidth;
			dstrect.y = 0;
			dstrect.w = yuvwidth;
			dstrect.h = yuvheight;
			SDL_RenderSetScale(renderer, (float)HEIGHT / dstrect.w, (float)WIDTH / dstrect.h);//按比例缩放
			SDL_RenderCopyEx(renderer, yuvtexture, nullptr, &dstrect, -90, &center, SDL_FLIP_NONE);

			SDL_RenderPresent(renderer);
		}
		break;
		case MY_QUIT:
			std::cout << "my quit envent." << std::endl;
			exitflag = true;
			break;
		}
	}

	SDL_WaitThread(th, nullptr);
	SDL_DestroyTexture(yuvtexture);
	SDL_DestroyTexture(texture);
	SDL_DestroyWindow(window);
	SDL_DestroyRenderer(renderer);

	SDL_PauseAudio(1);
	SDL_CloseAudio();

	SDL_Quit();

	free(yuvdata);
	yuv.close();
	pcm.close();

	return 0;
}
