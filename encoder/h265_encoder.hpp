/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (C) 2023, Starfive Technology Co., Ltd.
 *
 * h265_encoder.hpp - h265 video encoder.
 */

#pragma once

#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>

#include "encoder.hpp"

#include <OMX_Core.h>
#include <OMX_Component.h>
#include <OMX_Video.h>
#include <OMX_VideoExt.h>
#include <OMX_IndexExt.h>

class H265Encoder : public Encoder
{
public:
	H265Encoder(VideoOptions const *options, StreamInfo const &info);
	~H265Encoder();
	// Encode the given DMABUF.
	void EncodeBuffer(int fd, size_t size, void *mem, StreamInfo const &info, int64_t timestamp_us) override;

	struct EncodeContext
	{
		OMX_HANDLETYPE hComponentEncoder;
		OMX_U32 nFrameBufferSize;
		OMX_U32 nBitrate;
		OMX_U32 nFrameRate;
		OMX_U32 nNumPFrame;
		OMX_STATETYPE comState;
		OMX_U32 nInputBufferCount;
		OMX_BUFFERHEADERTYPE *pInputBufferArray[64];
		OMX_BUFFERHEADERTYPE *pOutputBufferArray[64];
		int msgid;

		OMX_BOOL disableEVnt;
		OMX_BOOL justQuit;
	};
private:
	struct EncodeContext encoderCxt;

	void initOMX();
	void configOMX(StreamInfo const &info);
	void freeResource();

	OMX_CALLBACKTYPE callbacks_;

	std::mutex input_buffers_available_mutex_;
	std::queue<OMX_BUFFERHEADERTYPE *> input_buffers_available_;

	std::mutex output_mutex_;
	std::queue<int64_t> output_timestamps_;

	// This thread just sits waiting for the encoder to finish stuff. It will either:
	// * receive "output" buffers (codec inputs), which we must return to the caller
	// * receive encoded buffers, which we pass to the application.
	void pollThread();
	std::thread poll_thread_;

	// Handle the output buffers in another thread so as not to block the encoder. The
	// application can take its time, after which we return this buffer to the encoder for
	// re-use.
	void outputThread();
	std::thread output_thread_;
	std::queue<OMX_BUFFERHEADERTYPE *> output_queue_;

	bool abortPoll_;
	std::condition_variable output_cond_var_;
	bool abortOutput_;
};
