/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (C) 2023, Starfive Technology Co., Ltd.
 *
 * starfive_server.cpp - StarFive server for PC tuning tool.
 */
#include <chrono>
#include <poll.h>
#include <signal.h>
#include <sys/signalfd.h>
#include <sys/stat.h>
#include <mutex>
#include <string.h>
#include <thread>

#include "core/frame_info.hpp"
#include "core/libcamera_app.hpp"
#include "core/still_options.hpp"

//#include "output/output.hpp"
#include "starfive_server.hpp"
#include "broadcast_main.h"
#include "tuning_main.h"
#include "param_convert.h"

using namespace std::chrono_literals;
using namespace std::placeholders;
using libcamera::Stream;


void StarfiveServer::makeGIRequest()
{
	std::shared_ptr<Camera> &camera = getCamera();

	for(int i = 0; i < GETINFO_REQUEST_NUMBER; i++) {
		std::unique_ptr<Request> request = camera->createRequest(STARFIVE_GIR_COOKIE + i);
		if (!request)
			throw std::runtime_error("failed to make GI request");
		giRequests_.push_back(std::move(request));
		freeGIRequest_.push_back(giRequests_.back().get());
	}
}

void StarfiveServer::requestCompleteSpecial(Request *request)
{
	FrameBuffer *emptyBuffer = request->buffers().begin()->second;
	uint64_t result = emptyBuffer->cookie();
	uint32_t cmdID = (uint32_t)(result >> 8);

	if(result & 1) {
		auto it = parConvertors.find(cmdID);
		if(it != parConvertors.end()) {
			ParamConvertorInterface *pc = it->second;
			pc->toUpType(pc->getDownParam(), pc->getUpBuffer());
			get_info_data_size_ = pc->upTypeSize();
			pc->initDownParam_ = true;
		}
	}

	{
	std::lock_guard<std::mutex> lock(get_info_mutex_);
	get_info_var_.notify_one();
	}

	request->reuse();
	freeGIRequest_.push_back(request);
}

void StarfiveServer::getISPParams(STCOMDDATA *pcomddata)
{
	STCOMD *pcomd = (STCOMD *)pcomddata;
	uint32_t mod_ctl_id = pcomd->u32Cmd - MODID_ISP_BASE;

	auto it = parConvertors.find(mod_ctl_id);
	if(it != parConvertors.end()) {
		ParamConvertorInterface *pc = it->second;

		if(!pc->isDynamic() && pc->initDownParam_) {
			pc->toUpType(pc->getDownParam(), (void *)((uint8_t *)pcomddata + sizeof(STCOMD)));
			pcomddata->stComd.u32Size = pc->upTypeSize();
			return;
		}

		if(freeGIRequest_.empty())
			throw std::runtime_error("no free GI request");
		Request *request = freeGIRequest_.front();
		freeGIRequest_.pop_front();

		if(request->buffers().empty()) {
			if (request->addBuffer(GetStream("still"), giFrameBuffers_[(uint32_t)request->cookie()].get()) < 0)
				throw std::runtime_error("failed to add buffer to GI request");
		}

		FrameBuffer *emptyBuffer = request->buffers().begin()->second;
		emptyBuffer->setCookie(mod_ctl_id << 8);

		uint8_t cmdBuffer[sizeof(libcamera::starfive::control::GetModuleInformation)];
		libcamera::starfive::control::GetModuleInformation *cmd = (libcamera::starfive::control::GetModuleInformation *)cmdBuffer;
		cmd->controlID = pc->getModuleID();
		cmd->reqCookie = request->cookie();
		cmd->buffer = pc->getDownParam();

		libcamera::ControlList cl;
		libcamera::ControlValue ctrlV(libcamera::Span<const uint8_t>(reinterpret_cast<uint8_t *>(cmdBuffer),
			sizeof(cmdBuffer)));
		cl.set(libcamera::starfive::control::GET_INFORMATION, ctrlV);

		request->controls() = std::move(cl);
		
		pc->setUpBuffer((void *)((uint8_t *)pcomddata + sizeof(STCOMD)));
		get_info_data_size_ = 0;

		std::shared_ptr<Camera> &camera = getCamera();
		if (camera->queueRequest(request) < 0)
			throw std::runtime_error("failed to queue GI request");
	}

	{
	std::unique_lock<std::mutex> lock(get_info_mutex_);
	if(get_info_var_.wait_for(lock, std::chrono::seconds(1)) == std::cv_status::timeout) {
		printf("get ISP information timeout.\n");
		pcomddata->stComd.u32Size = 0;
	} else
		pcomddata->stComd.u32Size = get_info_data_size_;
	}
}

void StarfiveServer::setISPParams(STCOMDDATA *pcomddata)
{
	STCOMD *pcomd = (STCOMD *)pcomddata;
	uint32_t mod_ctl_id = pcomd->u32Cmd - MODID_ISP_BASE;

	auto it = parConvertors.find(mod_ctl_id);
	if(it != parConvertors.end()) {
		ParamConvertorInterface *pc = it->second;
		void *pparamBuf = (void *)((uint8_t *)pcomddata + sizeof(STCOMD));
		pc->toDownType(pparamBuf, pc->getDownParam());

		libcamera::ControlList cl;
		libcamera::ControlValue ctrlV(libcamera::Span<const uint8_t>(reinterpret_cast<uint8_t *>(pc->getDownParam()),
			pc->downTypeSize()));
		cl.set(pc->getModuleID(), ctrlV);
		SetControls(cl);
	}
}

void StarfiveServer::getImageInfo(STCOMDDATA *pcomddata)
{
	if(both_imginfo_init != initSrImgInfo_)
		throw std::runtime_error("can not get image infomation");

	uint32_t kind = pcomddata->stComd.utApp.u32Data[0];
	const ISPImageInfo *imgInfo = serverImgInfo_;
	switch(kind) {
	case EN_MEM_KIND_UO:
		imgInfo = serverImgInfo_;
		break;
	case EN_MEM_KIND_DUMP:
		imgInfo = serverImgInfo_ + 1;
		break;
	default:
		memset(&pcomddata->stComd.utVal, 0, sizeof(UTVALUE));
		memset(&pcomddata->stComd.utApp, 0, sizeof(UTAPPEND));
		return;
	}
	
	pcomddata->stComd.utVal.u64Value = 0;
	pcomddata->stComd.utVal.u32Value = (imgInfo->mosaic << 16) | (kind & 0xFFFF);
	pcomddata->stComd.utApp.u32Data[0] = imgInfo->width;
	pcomddata->stComd.utApp.u32Data[1] = imgInfo->height;
	pcomddata->stComd.utApp.u32Data[2] = imgInfo->stride;
	pcomddata->stComd.utApp.u32Data[3] = imgInfo->bit;
	pcomddata->stComd.utApp.u32Data[4] = imgInfo->memsize;
}

const void *StarfiveServer::getImagePop(STCOMDDATA *pcomddata)
{
	if(latestBufferMap_.empty())
		return nullptr;

	uint32_t kind = pcomddata->stComd.utApp.u32Data[0];
	if(EN_MEM_KIND_UO != kind && EN_MEM_KIND_DUMP != kind)
		return nullptr;

	getImageInfo(pcomddata);

	{
	std::lock_guard<std::mutex> lock(get_image_mutex_);
	if(EN_MEM_KIND_UO == kind) {
		const std::vector<libcamera::Span<uint8_t>> mem = Mmap(latestBufferMap_[StillStream()]);
		return mem[0].data();
	} else {
		const std::vector<libcamera::Span<uint8_t>> mem = Mmap(latestBufferMap_[RawStream()]);
		return mem[0].data();
	}
	}
}

void StarfiveServer::setLatestBufferMap(CompletedRequestPtr &completed_request)
{
	std::lock_guard<std::mutex> lock(get_image_mutex_);
	latestBufferMap_ = completed_request->buffers;
}

void StarfiveServer::startServer()
{
	makeGIRequest();
	resetParConvertors();

	memset(&serverCfg, 0, sizeof(SERVER_CFG_S));
	serverCfg.stream_port = 8552;
	serverCfg.tuning_port = 8550;

	tuning_abort_ = false;
	tuningThread_ = std::thread(&tuning_main_listen_task, this);

	broadcast_abort_ = false;
	broadcastThread_ = std::thread(&broadcast_main_listen_task, this);
}

struct ServerBayerFormat
{
	int bits;
	eMOSAIC order;
};

static const std::map<libcamera::PixelFormat, ServerBayerFormat> bayer_formats_map =
{
	{ libcamera::formats::SRGGB10, { 10, MOSAIC_RGGB } },
	{ libcamera::formats::SGRBG10, { 10, MOSAIC_GRBG } },
	{ libcamera::formats::SBGGR10, { 10, MOSAIC_BGGR } },
	{ libcamera::formats::SGBRG10, { 10, MOSAIC_GBRG } },
	{ libcamera::formats::SRGGB12, { 12, MOSAIC_RGGB } },
	{ libcamera::formats::SGRBG12, { 12, MOSAIC_GRBG } },
	{ libcamera::formats::SBGGR12, { 12, MOSAIC_BGGR } },
	{ libcamera::formats::SGBRG12, { 12, MOSAIC_GBRG } },
	{ libcamera::formats::SRGGB10_CSI2P, { 10, MOSAIC_RGGB } },
	{ libcamera::formats::SGRBG10_CSI2P, { 10, MOSAIC_GRBG } },
	{ libcamera::formats::SBGGR10_CSI2P, { 10, MOSAIC_BGGR } },
	{ libcamera::formats::SGBRG10_CSI2P, { 10, MOSAIC_GBRG } },
	{ libcamera::formats::SRGGB12_CSI2P, { 12, MOSAIC_RGGB } },
	{ libcamera::formats::SGRBG12_CSI2P, { 12, MOSAIC_GRBG } },
	{ libcamera::formats::SBGGR12_CSI2P, { 12, MOSAIC_BGGR } },
	{ libcamera::formats::SGBRG12_CSI2P, { 12, MOSAIC_GBRG } },
	{ libcamera::formats::SRGGB16,       { 16, MOSAIC_RGGB } },
	{ libcamera::formats::SGRBG16,       { 16, MOSAIC_GRBG } },
	{ libcamera::formats::SBGGR16,       { 16, MOSAIC_BGGR } },
	{ libcamera::formats::SGBRG16,       { 16, MOSAIC_GBRG } },
};

void StarfiveServer::preProcess(CompletedRequestPtr &completed_request)
{
	if(both_imginfo_init != initSrImgInfo_) {
		StreamInfo info;
		if(!(initSrImgInfo_ & yuv_imginfo_init) && StillStream()) {
			info = GetStreamInfo(StillStream());
			serverImgInfo_[0].width = info.width;
			serverImgInfo_[0].height = info.height;
			serverImgInfo_[0].stride = info.stride;
			serverImgInfo_[0].bit = 8;
			serverImgInfo_[0].mosaic = MOSAIC_NONE;
			serverImgInfo_[0].memsize = info.height * info.stride + ((info.height * info.stride) >> 1);
			initSrImgInfo_ |= yuv_imginfo_init;
		}

		if(!(initSrImgInfo_ & raw_imginfo_init) && RawStream()) {
			info = GetStreamInfo(RawStream());
			auto it = bayer_formats_map.find(info.pixel_format);
			if (it == bayer_formats_map.end())
				throw std::runtime_error("unsupported Bayer format");
			ServerBayerFormat const &bayer_format = it->second;

			serverImgInfo_[1].width = info.width;
			serverImgInfo_[1].height = info.height;
			serverImgInfo_[1].stride = info.stride;
			serverImgInfo_[1].bit = bayer_format.bits;
			serverImgInfo_[1].mosaic = bayer_format.order;
			serverImgInfo_[1].memsize = info.height * info.stride;
			initSrImgInfo_ |= raw_imginfo_init;
		}
	}

	setLatestBufferMap(completed_request);
}

static int signal_received;
static void default_signal_handler(int signal_number)
{
	signal_received = signal_number;
	LOG(1, "Received signal " << signal_number);
}
static int get_key_or_signal(Options const *options, pollfd p[1])
{
	int key = 0;
	//if (options->keypress)
	{
		poll(p, 1, 0);
		if (p[0].revents & POLLIN)
		{
			char *user_string = nullptr;
			size_t len;
			[[maybe_unused]] size_t r = getline(&user_string, &len, stdin);
			key = user_string[0];
		}
	}
	//if (options->signal)
	{
		if (signal_received == SIGUSR1)
			key = '\n';
		else if (signal_received == SIGUSR2)
			key = 'x';
		signal_received = 0;
	}
	return key;
}

// The main even loop for the application.

static void event_loop(StarfiveServer &app)
{
	Options const *options = app.GetOptions();
	bool keypress = true; // "signal" mode is much like "keypress" mode

	app.OpenCamera();

	// Monitoring for keypresses and signals.
	signal(SIGUSR1, default_signal_handler);
	signal(SIGUSR2, default_signal_handler);
	pollfd p[1] = { { STDIN_FILENO, POLLIN, 0 } };

	app.ConfigureStill();
	app.StartCamera();

	app.startServer();

	bool keypressed = false;

	for (unsigned int count = 0;; count++)
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

		CompletedRequestPtr &completed_request = std::get<CompletedRequestPtr>(msg.payload);
		int key = get_key_or_signal(options, p);
		if (key == 'x' || key == 'X')
			return;
		if (key == '\n')
			keypressed = true;

		// In viewfinder mode, run until the timeout or keypress. When that happens,
		// if the "--autofocus-on-capture" option was set, trigger an AF scan and wait
		// for it to complete. Then switch to capture mode if an output was requested.
		//if (app.ViewfinderStream())
		if (app.StillStream())
		{
			LOG(2, "Viewfinder frame " << count);

			if (keypressed)
			{
				// Trigger a still capture, unless we timed out in timelapse or keypress mode
				if ((!keypressed && keypress))
					return;

				keypressed = false;
			}

			app.preProcess(completed_request);
			app.ShowPreview(completed_request, app.StillStream());
		}
	}
}

int main(int argc, char *argv[])
{
	try
	{
		StarfiveServer app;
		Options *options = app.GetOptions();
		if (options->Parse(argc, argv))
		{
			if (options->verbose >= 2)
				options->Print();

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
