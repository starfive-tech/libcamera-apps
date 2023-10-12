/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (C) 2023, Starfive Technology Co., Ltd.
 *
 * h265_encoder.cpp - h265 video encoder.
 */

#include <fcntl.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/msg.h>

#include <linux/videodev2.h>

#include <chrono>
#include <iostream>

#include "h265_encoder.hpp"

#define OMX_INIT_STRUCTURE(a)         \
    memset(&(a), 0, sizeof(a));       \
    (a).nSize = sizeof(a);            \
    (a).nVersion.nVersion = 1;        \
    (a).nVersion.s.nVersionMajor = 1; \
    (a).nVersion.s.nVersionMinor = 1; \
    (a).nVersion.s.nRevision = 1;     \
    (a).nVersion.s.nStep = 1

typedef struct Message
{
    long msg_type;
    OMX_S32 msg_flag;
    OMX_BUFFERHEADERTYPE *pBuffer;
} Message;

static OMX_ERRORTYPE eventHandler(
    OMX_HANDLETYPE hComponent,
    OMX_PTR pAppData,
    OMX_EVENTTYPE eEvent,
    OMX_U32 nData1,
    OMX_U32 nData2,
    OMX_PTR pEventData)
{
    H265Encoder::EncodeContext *pEncodeContext = (H265Encoder::EncodeContext *)pAppData;

    switch (eEvent)
    {
    case OMX_EventPortSettingsChanged:
    {
        OMX_PARAM_PORTDEFINITIONTYPE pOutputPortDefinition;
        OMX_INIT_STRUCTURE(pOutputPortDefinition);
        pOutputPortDefinition.nPortIndex = 1;
        OMX_GetParameter(pEncodeContext->hComponentEncoder, OMX_IndexParamPortDefinition, &pOutputPortDefinition);
        OMX_U32 nOutputBufferSize = pOutputPortDefinition.nBufferSize;
        OMX_U32 nOutputBufferCount = pOutputPortDefinition.nBufferCountMin;

        LOG(2, "enable output port and alloc buffer");
        OMX_SendCommand(pEncodeContext->hComponentEncoder, OMX_CommandPortEnable, 1, NULL);

        for (OMX_U32 i = 0; i < nOutputBufferCount; i++)
        {
            OMX_BUFFERHEADERTYPE *pBuffer = NULL;
            OMX_AllocateBuffer(hComponent, &pBuffer, 1, NULL, nOutputBufferSize);
            pEncodeContext->pOutputBufferArray[i] = pBuffer;
            OMX_FillThisBuffer(hComponent, pBuffer);
        }
    }
    break;
    case OMX_EventBufferFlag:
    {
        Message data;
        data.msg_type = 1;
        data.msg_flag = -1;
        if (msgsnd(pEncodeContext->msgid, (void *)&data, sizeof(data) - sizeof(data.msg_type), 0) == -1)
        {
            LOG_ERROR("msgsnd failed");
        }
    }
    break;
    case OMX_EventCmdComplete:
    {
        switch ((OMX_COMMANDTYPE)(nData1))
        {
        case OMX_CommandStateSet:
        {
            pEncodeContext->comState = (OMX_STATETYPE)(nData2);
        }
		break;
        case OMX_CommandPortDisable:
        {
            if (nData2 == 1)
                pEncodeContext->disableEVnt = OMX_TRUE;
        }
        break;
        default:
        break;
        }
    }
    break;
    case OMX_EventError:
    {
        LOG_ERROR("receive err event " + std::to_string(nData1) + std::to_string(nData2));
        pEncodeContext->justQuit = OMX_TRUE;
    }
    break;
    default:
        break;
    }
    return OMX_ErrorNone;
}

static OMX_ERRORTYPE fillOutputBufferDoneHandler(
    OMX_HANDLETYPE hComponent,
    OMX_PTR pAppData,
    OMX_BUFFERHEADERTYPE *pBuffer)
{
    H265Encoder::EncodeContext *pEncodeContext = (H265Encoder::EncodeContext *)pAppData;

    Message data;
    data.msg_type = 1;
    if (pBuffer->nFlags & OMX_BUFFERFLAG_EOS)
    {
        data.msg_flag = -1;
    }
    else
    {
        data.msg_flag = 1;
        data.pBuffer = pBuffer;
    }
    if (msgsnd(pEncodeContext->msgid, (void *)&data, sizeof(data) - sizeof(data.msg_type), 0) == -1)
    {
        LOG_ERROR("msgsnd failed");
    }

    return OMX_ErrorNone;
}

static OMX_ERRORTYPE emptyBufferDoneHandler(
    OMX_HANDLETYPE hComponent,
    OMX_PTR pAppData,
    OMX_BUFFERHEADERTYPE *pBuffer)
{
    H265Encoder::EncodeContext *pEncodeContext = (H265Encoder::EncodeContext *)pAppData;
    Message data;
    data.msg_type = 1;
    data.msg_flag = 0;
    data.pBuffer = pBuffer;
    if (msgsnd(pEncodeContext->msgid, (void *)&data, sizeof(data) - sizeof(data.msg_type), 0) == -1)
    {
        LOG_ERROR("msgsnd failed");
    }

    return OMX_ErrorNone;
}

void H265Encoder::initOMX()
{
	if(encoderCxt.hComponentEncoder)
	{
		OMX_FreeHandle(encoderCxt.hComponentEncoder);
		encoderCxt.hComponentEncoder = 0;
    	OMX_Deinit();
	}

	int ret = OMX_Init();
	if(OMX_ErrorNone != ret)
		throw std::runtime_error("run OMX_Init failed. ret is " + std::to_string(ret));
}

void H265Encoder::configOMX(StreamInfo const &info)
{
	// Config callbacks
	OMX_CALLBACKTYPE &callbacks = callbacks_;
	callbacks.EventHandler = eventHandler;
    callbacks.FillBufferDone = fillOutputBufferDoneHandler;
    callbacks.EmptyBufferDone = emptyBufferDoneHandler;

	// Get OMX handle
	OMX_HANDLETYPE hComponentEncoder = NULL;
	{
	char encName[] = "OMX.sf.video_encoder.hevc";

	OMX_GetHandle(&hComponentEncoder, encName, &encoderCxt, &callbacks);
	if(!hComponentEncoder)
		throw std::runtime_error("Can not get OMX handle.");
	}
	encoderCxt.hComponentEncoder = hComponentEncoder;

	// Config input
	OMX_PARAM_PORTDEFINITIONTYPE pInputPortDefinition;
    OMX_INIT_STRUCTURE(pInputPortDefinition);
    pInputPortDefinition.nPortIndex = 0;
    OMX_GetParameter(hComponentEncoder, OMX_IndexParamPortDefinition, &pInputPortDefinition);
    pInputPortDefinition.format.video.nFrameWidth = info.width;
    pInputPortDefinition.format.video.nFrameHeight = info.height;
	pInputPortDefinition.format.video.eColorFormat = OMX_COLOR_FormatYUV420SemiPlanar;
	pInputPortDefinition.format.video.xFramerate = encoderCxt.nFrameRate;
	OMX_SetParameter(hComponentEncoder, OMX_IndexParamPortDefinition, &pInputPortDefinition);
    OMX_GetParameter(hComponentEncoder, OMX_IndexParamPortDefinition, &pInputPortDefinition);

	// Config output
	OMX_PARAM_PORTDEFINITIONTYPE pOutputPortDefinition;
    OMX_INIT_STRUCTURE(pOutputPortDefinition);
    pOutputPortDefinition.nPortIndex = 1;
    OMX_GetParameter(hComponentEncoder, OMX_IndexParamPortDefinition, &pOutputPortDefinition);
    pOutputPortDefinition.format.video.nFrameWidth = info.width;
    pOutputPortDefinition.format.video.nFrameHeight = info.height;
    pOutputPortDefinition.format.video.nBitrate = encoderCxt.nBitrate;
    OMX_SetParameter(hComponentEncoder, OMX_IndexParamPortDefinition, &pOutputPortDefinition);

	if(encoderCxt.nNumPFrame)
	{
		OMX_VIDEO_PARAM_HEVCTYPE hevcType;
		OMX_INIT_STRUCTURE(hevcType);
		hevcType.nPortIndex = 1;
		OMX_GetParameter(hComponentEncoder, static_cast<OMX_INDEXTYPE>(OMX_IndexParamVideoHevc), &hevcType);
		hevcType.nKeyFrameInterval = encoderCxt.nNumPFrame;
		OMX_SetParameter(hComponentEncoder, static_cast<OMX_INDEXTYPE>(OMX_IndexParamVideoHevc), &hevcType);
	}

	// Allocate input buffer
	OMX_SendCommand(hComponentEncoder, OMX_CommandPortDisable, 1, NULL);
	LOG(1, "wait for output port disable");
    while (!encoderCxt.disableEVnt && !encoderCxt.justQuit);
    if (encoderCxt.justQuit)
	{
        freeResource();
		return;
	}
    LOG(1, "output port disabled");

	OMX_SendCommand(hComponentEncoder, OMX_CommandStateSet, OMX_StateIdle, NULL);

	OMX_U32 nInputBufferSize = pInputPortDefinition.nBufferSize;
    OMX_U32 nInputBufferCount = pInputPortDefinition.nBufferCountActual;
	for (OMX_U32 i = 0; i < nInputBufferCount; i++)
    {
        OMX_BUFFERHEADERTYPE *pBuffer = NULL;
        OMX_AllocateBuffer(hComponentEncoder, &pBuffer, 0, NULL, nInputBufferSize);
        encoderCxt.pInputBufferArray[i] = pBuffer;
		input_buffers_available_.push(pBuffer);
    }
	encoderCxt.nInputBufferCount = nInputBufferCount;

	LOG(1, "wait for Component idle");
    while (encoderCxt.comState != OMX_StateIdle && !encoderCxt.justQuit);
    if (encoderCxt.justQuit)
    {
        freeResource();
		return;
	}
    LOG(1, "Component in idle");
}

H265Encoder::H265Encoder(VideoOptions const *options, StreamInfo const &info)
	: Encoder(options)
{
	memset(&encoderCxt, 0, sizeof(EncodeContext));

	// Apply any options->
	encoderCxt.nBitrate = options->bitrate ? options->bitrate : 3000000;
	encoderCxt.nNumPFrame = options->intra ? options->intra - 1 : 14;
	encoderCxt.nFrameRate = 30;
	encoderCxt.nFrameBufferSize = info.width * info.height * 3 / 2;

	encoderCxt.disableEVnt = OMX_FALSE;
	encoderCxt.justQuit = OMX_FALSE;

	OMX_S32 msgid = -1;
    msgid = msgget(IPC_PRIVATE, 0666 | IPC_CREAT);
    if (msgid < 0)
        throw std::runtime_error("get ipc_id error");
    encoderCxt.msgid = msgid;

	// Initialize OMX
	initOMX();

	// Config OMX
	configOMX(info);

    abortOutput_ = false;
	output_thread_ = std::thread(&H265Encoder::outputThread, this);
    abortPoll_ = false;
	poll_thread_ = std::thread(&H265Encoder::pollThread, this);
}

H265Encoder::~H265Encoder()
{
	freeResource();

    abortPoll_ = true;

    Message data;
    data.msg_type = 1;
    data.msg_flag = -1;
    if (msgsnd(encoderCxt.msgid, (void *)&data, sizeof(data) - sizeof(data.msg_type), 0) == -1)
    {
        LOG_ERROR("msgsnd failed");
    }

	poll_thread_.join();
    abortOutput_ = true;
	output_thread_.join();
}

void H265Encoder::freeResource()
{
	if (encoderCxt.comState == OMX_StateExecuting)
    {
        OMX_SendCommand(encoderCxt.hComponentEncoder, OMX_CommandStateSet, OMX_StateIdle, NULL);
        LOG(1, "wait for Component idle");
        while (encoderCxt.comState != OMX_StateIdle && !encoderCxt.justQuit);
        LOG(1, "Component in idle");
    }

	if(encoderCxt.hComponentEncoder)
	{
		OMX_FreeHandle(encoderCxt.hComponentEncoder);
		OMX_Deinit();
	}
}

void H265Encoder::EncodeBuffer(int fd, size_t size, void *mem, StreamInfo const &info, int64_t timestamp_us)
{
	OMX_BUFFERHEADERTYPE *pInputBuffer = nullptr;
	{
		std::lock_guard<std::mutex> lock(input_buffers_available_mutex_);
		if (input_buffers_available_.empty())
			throw std::runtime_error("no buffers available to queue codec input");

		pInputBuffer = input_buffers_available_.front();
		input_buffers_available_.pop();

        output_timestamps_.push(timestamp_us);
	}

	memcpy(pInputBuffer->pBuffer, mem, encoderCxt.nFrameBufferSize);
	pInputBuffer->nFlags = 0x10;
    pInputBuffer->nFilledLen = encoderCxt.nFrameBufferSize;

	input_done_callback_(nullptr);

	OMX_EmptyThisBuffer(encoderCxt.hComponentEncoder, pInputBuffer);
}

void H265Encoder::pollThread()
{
	LOG(1, "start process");
    OMX_SendCommand(encoderCxt.hComponentEncoder, OMX_CommandStateSet, OMX_StateExecuting, NULL);

	Message data;
    while (OMX_TRUE)
    {
        ssize_t ret = msgrcv(encoderCxt.msgid, (void *)&data, BUFSIZ, 0, 0);
        if (ret == -1)
            throw std::runtime_error("msgrcv failed with errno: " + std::to_string(errno));
        
        switch (data.msg_flag)
        {
        case 0:
        {
			std::lock_guard<std::mutex> lock(input_buffers_available_mutex_);
			input_buffers_available_.push(data.pBuffer);
        }
        break;
        case 1:
        {
			{
				std::lock_guard<std::mutex> lock(output_mutex_);
                output_queue_.push(data.pBuffer);
                output_cond_var_.notify_one();
			}
 		}
        break;
        case -1:
        {
            if(abortPoll_)
                return;
        }
        break;
        default:
            break;
        }
        if(abortPoll_)
            return;
    }
}

void H265Encoder::outputThread()
{
    while (true)
    {
        int64_t timestamp = 0;
        OMX_BUFFERHEADERTYPE *pBuffer = nullptr;
        {
            std::unique_lock<std::mutex> lock(output_mutex_);
            if (output_queue_.empty())
            {
                if(abortOutput_)
                    return;
                else
                    output_cond_var_.wait_for(lock, 200ms);
            } else
            {
                pBuffer = output_queue_.front();
                output_queue_.pop();
                if(!output_timestamps_.empty())
                {
                    timestamp = output_timestamps_.front();
                    output_timestamps_.pop();
                }
            }
        }

        if(pBuffer)
        {
            output_ready_callback_(pBuffer->pBuffer, pBuffer->nFilledLen, timestamp, true);
            OMX_FillThisBuffer(encoderCxt.hComponentEncoder, pBuffer);
        }
    }
}