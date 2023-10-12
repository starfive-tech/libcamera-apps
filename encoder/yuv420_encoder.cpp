/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (C) 2023, Starfive Technology Co., Ltd.
 *
 * yuv420_encoder.cpp - yuv420 video encoder.
 */

#include <chrono>
#include <iostream>
#include <stdexcept>

#include "yuv420_encoder.hpp"

void NV12ToYUV420(const uint8_t *mem, uint8_t *dst, const StreamInfo &info);

YUV420Encoder::YUV420Encoder(VideoOptions const *options) : Encoder(options), abort_(false)
{
	LOG(2, "Opened YUV420Encoder");
	output_thread_ = std::thread(&YUV420Encoder::outputThread, this);
}

YUV420Encoder::~YUV420Encoder()
{
	abort_ = true;
	output_thread_.join();
	LOG(2, "YUV420Encoder closed");
}

// Push the buffer onto the output queue to be "encoded" and returned.
void YUV420Encoder::EncodeBuffer(int fd, size_t size, void *mem, StreamInfo const &info, int64_t timestamp_us)
{
	if(!streamInfo_.width || !streamInfo_.height || !streamInfo_.stride)
		streamInfo_ = info;
	std::lock_guard<std::mutex> lock(output_mutex_);
	OutputItem item = { mem, size, timestamp_us };
	output_queue_.push(item);
	output_cond_var_.notify_one();
}

// Realistically we would probably want more of a queue as the caller's number
// of buffers limits the amount of queueing possible here...
void YUV420Encoder::outputThread()
{
	OutputItem item;
	while (true)
	{
		{
			std::unique_lock<std::mutex> lock(output_mutex_);
			while (true)
			{
				using namespace std::chrono_literals;
				if (!output_queue_.empty())
				{
					item = output_queue_.front();
					output_queue_.pop();
					break;
				}
				else
					output_cond_var_.wait_for(lock, 200ms);
				if (abort_)
					return;
			}
		}
		// Ensure the input done callback happens before the output ready callback.
		// This is needed as the metadata queue gets pushed in the former, and popped
		// in the latter.
		input_done_callback_(nullptr);

		if(streamInfo_.width && streamInfo_.height && streamInfo_.stride)
		{
			uint8_t *yuv420 = new uint8_t[streamInfo_.width * streamInfo_.height + ((streamInfo_.height * streamInfo_.width) >> 1)];
			if(!yuv420)
				throw std::runtime_error("fail to apply memory.");
			NV12ToYUV420((uint8_t *)(item.mem), yuv420, streamInfo_);
			output_ready_callback_(yuv420, item.length, item.timestamp_us, true);
			delete[] yuv420;
		} else
			output_ready_callback_(item.mem, item.length, item.timestamp_us, true);
	}
}
