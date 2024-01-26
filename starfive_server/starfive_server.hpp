/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (C) 2023, Starfive Technology Co., Ltd.
 *
 * starfive_server.h - StarFive server for PC tuning tool.
 */
#pragma once

#include <unordered_map>
#include "core/libcamera_app.hpp"
#include "core/still_options.hpp"

#include "comm.h"
#include "tuning_base.h"
#include "starfive_controls.h"

#define GETINFO_REQUEST_NUMBER				4

class StarfiveServer : public LibcameraApp
{
public:
	StarfiveServer() : LibcameraApp(std::make_unique<Options>())
	{
		girPlanes_.resize(GETINFO_REQUEST_NUMBER);
		for(int i = 0; i < GETINFO_REQUEST_NUMBER; i++) {
            girPlanes_[i].resize(1);
            girPlanes_[i][0].offset = 0;
		    giFrameBuffers_.push_back(std::make_unique<libcamera::FrameBuffer>(girPlanes_[i], 0));
        }

		initSrImgInfo_ = no_imginfo_init;
	}
	~StarfiveServer()
	{
        {
		std::lock_guard<std::mutex> lock(tuning_mutex_);
		tuning_abort_ = true;
		}
        tuningThread_.join();

        {
		std::lock_guard<std::mutex> lock(broadcast_mutex_);
		broadcast_abort_ = true;
		}
		broadcastThread_.join();
	}

	Options *GetOptions() const { return static_cast<Options *>(options_.get()); }

	void startServer();

    void getISPParams(STCOMDDATA *pcomddata);
    void setISPParams(STCOMDDATA *pcomddata);

	void getImageInfo(STCOMDDATA *pcomddata);

	void preProcess(CompletedRequestPtr &completed_request);

	const void *getImagePop(STCOMDDATA *pcomddata);

protected:
	void requestCompleteSpecial(Request *request) override;
	void stopSpecial() override { freeGIRequest_.clear(); giRequests_.clear(); };

private:
	void makeGIRequest();
	void setLatestBufferMap(CompletedRequestPtr &completed_request);

public:
    SERVER_CFG_S serverCfg;
    
    std::mutex tuning_mutex_;
    bool tuning_abort_;

    std::mutex broadcast_mutex_;
    bool broadcast_abort_;

private:
	std::vector<std::vector<libcamera::FrameBuffer::Plane>> girPlanes_;
	std::vector<std::unique_ptr<libcamera::FrameBuffer>> giFrameBuffers_;
	std::vector<std::unique_ptr<Request>> giRequests_;
	std::list<Request *> freeGIRequest_;

	std::thread tuningThread_;
	std::thread broadcastThread_;

    uint32_t get_info_data_size_;
    std::mutex get_info_mutex_;
    std::condition_variable get_info_var_;

	ISPImageInfo serverImgInfo_[2];	// yuv, raw
	enum {
		no_imginfo_init = 0,
		yuv_imginfo_init = 1,
		raw_imginfo_init = 2,
		both_imginfo_init = 3,
	};
	uint32_t initSrImgInfo_;
	
	std::mutex get_image_mutex_;

	libcamera::Request::BufferMap latestBufferMap_;
};
