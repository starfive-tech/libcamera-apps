/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (C) 2020, Raspberry Pi (Trading) Ltd.
 *
 * libcamera_jpeg.cpp - minimal libcamera jpeg capture app.
 */

#include <chrono>

#include "core/libcamera_app.hpp"
#include "core/still_options.hpp"

#include "image/image.hpp"

using namespace std::placeholders;
using libcamera::Stream;

void NV12ToYUV420(const libcamera::Span<uint8_t> &mem, std::vector<libcamera::Span<uint8_t>> &yuv420, StreamInfo &info);

class LibcameraJpegApp : public LibcameraApp
{
public:
	LibcameraJpegApp()
		: LibcameraApp(std::make_unique<StillOptions>())
	{
	}

	StillOptions *GetOptions() const
	{
		return static_cast<StillOptions *>(options_.get());
	}
};

// The main even loop for the application.

static void event_loop(LibcameraJpegApp &app)
{
	StillOptions const *options = app.GetOptions();
	app.OpenCamera();
	app.ConfigureViewfinder();
	app.StartCamera();
	auto start_time = std::chrono::high_resolution_clock::now();

	for (;;)
	{
		LibcameraApp::Msg msg = app.Wait();
		if (msg.type == LibcameraApp::MsgType::Timeout)
		{
			LOG_ERROR("ERROR: Device timeout detected, attempting a restart!!!");
			app.StopCamera();
			app.StartCamera();
			continue;
		}
		if (msg.type == LibcameraApp::MsgType::Quit)
			return;
		else if (msg.type != LibcameraApp::MsgType::RequestComplete)
			throw std::runtime_error("unrecognised message!");

		// In viewfinder mode, simply run until the timeout. When that happens, switch to
		// capture mode.
		if (app.ViewfinderStream())
		{
			auto now = std::chrono::high_resolution_clock::now();
			if (options->timeout && now - start_time > std::chrono::milliseconds(options->timeout))
			{
				Stream *stream = app.ViewfinderStream();
				StreamInfo info = app.GetStreamInfo(stream);
				CompletedRequestPtr &payload = std::get<CompletedRequestPtr>(msg.payload);
				const std::vector<libcamera::Span<uint8_t>> mem = app.Mmap(payload->buffers[stream]);
				std::vector<libcamera::Span<uint8_t>> yuv420;

				NV12ToYUV420(mem[0], yuv420, info);
				app.StopCamera();
				app.Teardown();
				
				jpeg_save(yuv420, info, payload->metadata, options->output, app.CameraModel(), options);
				delete[] yuv420[0].data();
				return;
			}
			else
			{
				CompletedRequestPtr &completed_request = std::get<CompletedRequestPtr>(msg.payload);
				app.ShowPreview(completed_request, app.ViewfinderStream());
			}
		}
	}
}

int main(int argc, char *argv[])
{
	try
	{
		LibcameraJpegApp app;
		StillOptions *options = app.GetOptions();
		if (options->Parse(argc, argv))
		{
			if (options->verbose >= 2)
				options->Print();
			if (options->output.empty())
				throw std::runtime_error("output file name required");

			event_loop(app);
		}
	}
	catch (std::exception const &e)
	{
		LOG_ERROR("ERROR: *** " << e.what() << " ***");
		return -1;
	}
	return 0;
}
