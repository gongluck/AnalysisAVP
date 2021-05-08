/*
 * @Author: gongluck
 * @Date: 2021-04-05 22:55:46
 * @Last Modified by: gongluck
 * @Last Modified time: 2021-04-11 19:38:13
 */

#define _CRT_SECURE_NO_WARNINGS

#include <iostream>

#include "nvEncodeAPI.h"

#include <wrl.h>
#include <d3d11.h>
using Microsoft::WRL::ComPtr;

const int width = 640;
const int height = 480;
const int framerate = 30;

const GUID codecguid = NV_ENC_CODEC_H264_GUID;
const GUID presetguid = NV_ENC_PRESET_P3_GUID;
const NV_ENC_TUNING_INFO truninginfo = NV_ENC_TUNING_INFO_HIGH_QUALITY;

int main(int argc, char* argv[])
{
	/*d3d11*/
	ComPtr<ID3D11Device> device;
	ComPtr<ID3D11DeviceContext> context;
	ComPtr<IDXGIFactory> factory;
	ComPtr<IDXGIAdapter> adapter;
	HRESULT d3d_status = CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)factory.GetAddressOf());
	d3d_status = factory->EnumAdapters(0, adapter.GetAddressOf());
	d3d_status = D3D11CreateDevice(adapter.Get(), D3D_DRIVER_TYPE_UNKNOWN, nullptr, 0,
		nullptr, 0, D3D11_SDK_VERSION, device.GetAddressOf(), nullptr, context.GetAddressOf());
	ComPtr<ID3D11Texture2D> texture;
	D3D11_TEXTURE2D_DESC desc = { 0 };
	desc.Width = width;
	desc.Height = height;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	desc.SampleDesc.Count = 1;
	desc.Usage = D3D11_USAGE_STAGING;
	desc.BindFlags = 0;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	d3d_status = device->CreateTexture2D(&desc, nullptr, texture.GetAddressOf());
	D3D11_MAPPED_SUBRESOURCE texturemap;
	d3d_status = context->Map(texture.Get(), D3D11CalcSubresource(0, 0, 1), D3D11_MAP_WRITE, 0, &texturemap);

	//NVEC初始化
	NVENCSTATUS nv_status = NV_ENC_SUCCESS;
	NV_ENCODE_API_FUNCTION_LIST nvfunclist = { NV_ENCODE_API_FUNCTION_LIST_VER };
	nv_status = NvEncodeAPICreateInstance(&nvfunclist);

	//NVENC参数
	NV_ENC_OPEN_ENCODE_SESSION_EX_PARAMS openparams = { NV_ENC_OPEN_ENCODE_SESSION_EX_PARAMS_VER };
	openparams.device = device.Get();
	openparams.deviceType = NV_ENC_DEVICE_TYPE_DIRECTX;
	openparams.apiVersion = NVENCAPI_VERSION;

	//NVENC实例
	void* hencoder = nullptr;
	nv_status = nvfunclist.nvEncOpenEncodeSessionEx(&openparams, &hencoder);

	//NVENC编码预设参数
	NV_ENC_PRESET_CONFIG presetconfig = { NV_ENC_PRESET_CONFIG_VER, { NV_ENC_CONFIG_VER } };
	nv_status = nvfunclist.nvEncGetEncodePresetConfigEx(hencoder, codecguid, presetguid, truninginfo, &presetconfig);

	//NVENC参数设置
	NV_ENC_INITIALIZE_PARAMS initparams = { NV_ENC_INITIALIZE_PARAMS_VER };
	initparams.encodeGUID = codecguid;
	initparams.presetGUID = presetguid;
	initparams.encodeWidth = width;
	initparams.encodeHeight = height;
	initparams.darWidth = width;
	initparams.darHeight = height;
	initparams.maxEncodeWidth = width;
	initparams.maxEncodeHeight = height;
	initparams.frameRateNum = framerate;
	initparams.frameRateDen = 1;
	initparams.tuningInfo = truninginfo;
	initparams.enablePTD = 1;
	//NV_ENC_CAPS_PARAM capsparam = { NV_ENC_CAPS_PARAM_VER };
	//capsparam.capsToQuery = NV_ENC_CAPS_ASYNC_ENCODE_SUPPORT;
	//nv_status = nvfunclist.nvEncGetEncodeCaps(hencoder, codecguid, &capsparam, reinterpret_cast<int*>(&initparams.enableEncodeAsync));
	NV_ENC_CONFIG enconfig = presetconfig.presetCfg;
	enconfig.gopLength = framerate * 10;
	enconfig.frameIntervalP = 1;
	enconfig.encodeCodecConfig.h264Config.idrPeriod = framerate * 10;
	initparams.encodeConfig = &enconfig;
	nv_status = nvfunclist.nvEncInitializeEncoder(hencoder, &initparams);

	//NVENC输出资源
	NV_ENC_CREATE_BITSTREAM_BUFFER bitstreambuf = { NV_ENC_CREATE_BITSTREAM_BUFFER_VER };
	nv_status = nvfunclist.nvEncCreateBitstreamBuffer(hencoder, &bitstreambuf);

	//NVENC输入资源
	NV_ENC_REGISTER_RESOURCE resource = { NV_ENC_REGISTER_RESOURCE_VER };
	resource.resourceType = NV_ENC_INPUT_RESOURCE_TYPE_DIRECTX;
	resource.resourceToRegister = texture.Get();
	resource.width = width;
	resource.height = height;
	resource.bufferFormat = NV_ENC_BUFFER_FORMAT_ARGB;
	resource.bufferUsage = NV_ENC_INPUT_IMAGE;
	nv_status = nvfunclist.nvEncRegisterResource(hencoder, &resource);
	NV_ENC_MAP_INPUT_RESOURCE mapresource = { NV_ENC_MAP_INPUT_RESOURCE_VER };
	mapresource.registeredResource = resource.registeredResource;
	nv_status = nvfunclist.nvEncMapInputResource(hencoder, &mapresource);

	//NVENC编码参数
	NV_ENC_PIC_PARAMS picparams = { NV_ENC_PIC_PARAMS_VER };
	picparams.pictureStruct = NV_ENC_PIC_STRUCT_FRAME;
	picparams.inputBuffer = mapresource.registeredResource;
	picparams.bufferFmt = NV_ENC_BUFFER_FORMAT_ARGB;
	picparams.inputWidth = width;
	picparams.inputHeight = height;
	picparams.outputBitstream = bitstreambuf.bitstreamBuffer;
	picparams.completionEvent = CreateEvent(nullptr, false, false, nullptr);

	//锁定输出资源参数
	NV_ENC_LOCK_BITSTREAM lockbitstreamdata = { NV_ENC_LOCK_BITSTREAM_VER };
	lockbitstreamdata.outputBitstream = bitstreambuf.bitstreamBuffer;
	lockbitstreamdata.doNotWait = false;

	FILE* inf = fopen(argv[1], "rb");
	FILE* outf = fopen(argv[2], "wb");
	while (true)
	{
		if (!fread(texturemap.pData, width * height * 4, 1, inf))
		{
			break;
		}
		//编码
		nv_status = nvfunclist.nvEncEncodePicture(hencoder, &picparams);
		//锁定输出资源
		nv_status = nvfunclist.nvEncLockBitstream(hencoder, &lockbitstreamdata);

		std::cout << lockbitstreamdata.bitstreamSizeInBytes << std::endl;
		fwrite(lockbitstreamdata.bitstreamBufferPtr, lockbitstreamdata.bitstreamSizeInBytes, 1, outf);
		fflush(outf);

		//解锁输出资源
		nv_status = nvfunclist.nvEncUnlockBitstream(hencoder, &lockbitstreamdata);
	}
	fclose(outf);
	fclose(inf);

	//结束编码
	NV_ENC_PIC_PARAMS endpicparams = { NV_ENC_PIC_PARAMS_VER };
	endpicparams.encodePicFlags = NV_ENC_PIC_FLAG_EOS;
	endpicparams.completionEvent = picparams.completionEvent;
	nv_status = nvfunclist.nvEncEncodePicture(hencoder, &endpicparams);

	//清理资源
	nv_status = nvfunclist.nvEncUnmapInputResource(hencoder, mapresource.registeredResource);
	nv_status = nvfunclist.nvEncUnregisterResource(hencoder, resource.registeredResource);
	context->Unmap(texture.Get(), D3D11CalcSubresource(0, 0, 1));
	CloseHandle(picparams.completionEvent);
	nv_status = nvfunclist.nvEncDestroyBitstreamBuffer(hencoder, bitstreambuf.bitstreamBuffer);
	nv_status = nvfunclist.nvEncDestroyEncoder(hencoder);
	return 0;
}