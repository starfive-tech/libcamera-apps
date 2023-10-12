/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (C) 2023, Starfive Technology Co., Ltd.
 *
 * yuv420_encoder.hpp - yuv420 video encoder.
 */

#pragma once

#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>

#include "core/video_options.hpp"
#include "encoder.hpp"

class YUV420Encoder : public Encoder
{
public:
	YUV420Encoder(VideoOptions const *options);
	~YUV420Encoder();
	void EncodeBuffer(int fd, size_t size, void *mem, StreamInfo const &info, int64_t timestamp_us) override;

private:
	StreamInfo streamInfo_;
	void outputThread();

	bool abort_;
	VideoOptions options_;
	struct OutputItem
	{
		void *mem;
		size_t length;
		int64_t timestamp_us;
	};
	std::queue<OutputItem> output_queue_;
	std::mutex output_mutex_;
	std::condition_variable output_cond_var_;
	std::thread output_thread_;
};
