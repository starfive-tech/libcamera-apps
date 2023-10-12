/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (C) 2023, Starfive Technology Co., Ltd.
 *
 * cvt_color_format.cpp - convert other format to yuv420.
 */

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include "core/still_options.hpp"

void NV12ToYUV420(const uint8_t *mem, uint8_t *dst, const StreamInfo &info)
{
	const uint8_t *src = mem;
	uint32_t width = info.width, height = info.height, stride = info.stride;
	uint8_t *u = dst + height * stride;
	uint8_t *v = u + ((height * width) >> 2);
	uint32_t i;

	memcpy(dst, src, height * stride);

	src += height * stride;
	for(i = 0; i < (height * width) >> 2; i++, src += 2)
		u[i] = src[0], v[i] = src[1];
}

void NV12ToYUV420(const libcamera::Span<uint8_t> &mem, uint8_t *dst, const StreamInfo &info)
{
	NV12ToYUV420((const uint8_t *)(mem.data()), dst, info);
/*	
	const uint8_t *src = mem.data();
	uint32_t width = info.width, height = info.height, stride = info.stride;
	uint8_t *u = dst + height * stride;
	uint8_t *v = u + ((height * width) >> 2);
	uint32_t i;

	memcpy(dst, src, height * stride);

	src += height * stride;
	for(i = 0; i < (height * width) >> 2; i++, src += 2)
		u[i] = src[0], v[i] = src[1];
*/
}

void NV12ToYUV420(const libcamera::Span<uint8_t> &mem, std::vector<libcamera::Span<uint8_t>> &yuv420, StreamInfo &info)
{
	uint32_t imgBufSize = info.height * info.stride + ((info.height * info.width) >> 1);
	uint8_t * yuv420Buf = new uint8_t[imgBufSize];

	if(!yuv420Buf)
		throw std::runtime_error("fail to apply memory");
	
	yuv420.push_back(libcamera::Span<uint8_t>(yuv420Buf, imgBufSize));

	NV12ToYUV420(mem, yuv420Buf, info);
	info.pixel_format = libcamera::formats::YUV420;
}

template<int R>
void NV12ToRGB888(const libcamera::Span<uint8_t> &mem, uint8_t *dst, const StreamInfo &info)
{
	uint32_t width = info.width, height = info.height, stride = info.stride;
	const uint8_t *yRow = mem.data();
	const uint8_t *uv = yRow + height * stride;
	int ri = R ? 2 : 0;
	
	for(uint32_t i = 0; i < height; i++, yRow += stride, dst += 3 * width)
	{
		const uint8_t * curUV = uv;
		uint8_t * curDst = dst;
		for(uint32_t j = 0; j < width; j++, curDst += 3)
		{
			int32_t y = (int32_t)yRow[j] - 16;
			int32_t u = (int32_t)curUV[0] - 128;
			int32_t v = (int32_t)curUV[1] - 128;
			int32_t val;

			val = (1192 * y + 2066 * u - v) >> 10;
			curDst[ri] = val < 0 ? 0 : (val > 255 ? 255 : (uint8_t)val);
			val = (1192 * y - 401 * u - 833 * v) >> 10;
			curDst[1] = val < 0 ? 0 : (val > 255 ? 255 : (uint8_t)val);
			val = (1192 * y - 2 * u + 1634 * v) >> 10;
			curDst[2 - ri] = val < 0 ? 0 : (val > 255 ? 255 : (uint8_t)val);

			if(j & 1)
				curUV += 2;
		}
		if(i & 1)
			uv += width;
	}
}

template<int R>
void RGB888FromNV12(const libcamera::Span<uint8_t> &mem, std::vector<libcamera::Span<uint8_t>> &rgb888, StreamInfo &info)
{
	uint32_t imgBufSize = info.height * info.width * 3;
	uint8_t * rgb888Buf = new uint8_t[imgBufSize];

	if(!rgb888Buf)
		throw std::runtime_error("fail to apply memory");
	
	rgb888.push_back(libcamera::Span<uint8_t>(rgb888Buf, imgBufSize));

	NV12ToRGB888<R>(mem, rgb888Buf, info);
	info.stride = 3 * info.width;
}

void NV12ToRGB888(const libcamera::Span<uint8_t> &mem, std::vector<libcamera::Span<uint8_t>> &rgb888, StreamInfo &info)
{
	RGB888FromNV12<0>(mem, rgb888, info);
	info.pixel_format = libcamera::formats::RGB888;
}

void NV12ToBGR888(const libcamera::Span<uint8_t> &mem, std::vector<libcamera::Span<uint8_t>> &rgb888, StreamInfo &info)
{
	RGB888FromNV12<2>(mem, rgb888, info);
	info.pixel_format = libcamera::formats::BGR888;
}