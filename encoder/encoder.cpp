/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (C) 2020, Raspberry Pi (Trading) Ltd.
 *
 * encoder.cpp - Video encoder class.
 */

#include <cstring>

#include "encoder.hpp"
#include "h264_encoder.hpp"
#include "mjpeg_encoder.hpp"
#include "null_encoder.hpp"
#include "h265_encoder.hpp"
#include "yuv420_encoder.hpp"

#if LIBAV_PRESENT
#include "libav_encoder.hpp"
#endif

Encoder *Encoder::Create(VideoOptions const *options, const StreamInfo &info)
{
	if (strcasecmp(options->codec.c_str(), "yuv420") == 0)
		return new YUV420Encoder(options);
	else if (strcasecmp(options->codec.c_str(), "h264") == 0)
		return new H264Encoder(options, info);
#if LIBAV_PRESENT
	else if (strcasecmp(options->codec.c_str(), "libav") == 0)
		return new LibAvEncoder(options, info);
#endif
	else if (strcasecmp(options->codec.c_str(), "mjpeg") == 0)
		return new MjpegEncoder(options);
	else if (strcasecmp(options->codec.c_str(), "h265") == 0)
		return new H265Encoder(options, info);
	throw std::runtime_error("Unrecognised codec " + options->codec);
}
