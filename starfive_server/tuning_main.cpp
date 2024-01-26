/**
  ******************************************************************************
  * @file tuning_main.c
  * @author  StarFive Isp Team
  * @version  V1.0
  * @date  06/21/2022
  * @brief StarFive ISP tuning server tuning application
  ******************************************************************************
  * @copy
  *
  * THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
  * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE
  * TIME. AS A RESULT, STARFIVE SHALL NOT BE HELD LIABLE FOR ANY
  * DIRECT, INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING
  * FROM THE CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE
  * CODING INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
  *
  * Copyright (C)  2019 - 2022 StarFive Technology Co., Ltd.
  */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>
#include <signal.h>
#include "comm.h"
#include "tuning_base.h"
#include "tuning_main.h"

#include "starfive_server.hpp"

/*-----------------------------------------------------------*/
uint32_t g_alive_count = 1;
BOOL g_terminate_listening = FALSE;
BOOL g_tuning_debug_enable = FALSE;

/*-----------------------------------------------------------*/
int tuning_main_listen_task(StarfiveServer* parent)
{
	SOCKET sock_listen, sock_connect = INVALID_SOCKET;
	struct sockaddr_in inaddr;
	SOCKLEN addrlen = sizeof(inaddr);
	int ret;

	/* Just to prevent compiler warnings. */

	if (parent->serverCfg.tuning_port == 0)
		parent->serverCfg.tuning_port = TUNING_SERVER_PORT;

	main_print_app_info(TUNING_SERVER_NAME, TUNING_SERVER_VERSION, parent->serverCfg.tuning_port);
	tuning_base_initial_client();
	////!!!!tuning_serv_initial();

	sock_listen = socket(AF_INET, SOCK_STREAM, 0);
	if (sock_listen == INVALID_SOCKET)
	{
		err("create socket failed : %d\n", sock_listen);
		return -1;
	}

	struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 10000;
    if (setsockopt(sock_listen, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) == -1) {
        err("setsockopt failed\n");
		return -1;
    }

	memset(&inaddr, 0, sizeof(inaddr));
	inaddr.sin_family = AF_INET;
	inaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	inaddr.sin_port = htons(parent->serverCfg.tuning_port);
	ret = bind(sock_listen, (struct sockaddr*)&inaddr, sizeof(inaddr));
	if (ret == SOCKET_ERROR)
	{
		err("bind socket to address failed : %d\n", ret);
		closesocket(sock_listen);
		return -1;
	}
	ret = listen(sock_listen, TUNING_SERVER_MAX_CONNECTION);
	if (ret == SOCKET_ERROR)
	{
		err("listen socket failed : %d\n", ret);
		closesocket(sock_listen);
		return -1;
	}

	printf("Starting the StarFive tuning server on port %d ... \n", parent->serverCfg.tuning_port);

	do
	{
		{
		std::lock_guard<std::mutex> lock(parent->tuning_mutex_);
		if(parent->tuning_abort_)
			break;
		}

		if (tuning_base_get_client_count() < TUNING_SERVER_MAX_CONNECTION)
		{
			/* Wait for a client to connect. */
			sock_connect = accept(sock_listen, (struct sockaddr*)&inaddr, &addrlen);
			if (sock_connect == INVALID_SOCKET)
			{
				if(sk_errno() == EAGAIN || sk_errno() == EWOULDBLOCK) {
					join_complete_tuning_threads();
					continue;
				} else
				{
					err("accept failed : %d\n", sock_connect);
					return -1;
				}
			}

			get_new_tuning_thread_obj(sock_connect) = std::thread(&tuning_main_connection_task, parent, sock_connect);

			tuning_base_insert_client(sock_connect);
			tuning_base_dump_client();
		}
	} while (true);
	
	
	join_tuning_task_threads();
	main_print_app_end(TUNING_SERVER_NAME);
	return 0;
}

/*-----------------------------------------------------------*/
int tuning_main_connection_task(StarfiveServer* parent, SOCKET sock_connected)
{
	//SOCKET sock_connected = *(SOCKET*)pparameters;
	SOCKET sock_max = sock_connected;
	STCOMDDATA comddata;
	int32_t bytes_received;
	fd_set fds_read, fds_write, fds_except;
	struct timeval tv;
	int result;

	for (;;)
	{
		{
		std::lock_guard<std::mutex> lock(parent->tuning_mutex_);
		if(parent->tuning_abort_)
			break;
		}

		FD_ZERO(&fds_read);
		FD_ZERO(&fds_write);
		FD_ZERO(&fds_except);
		FD_SET(sock_connected, &fds_read);
		FD_SET(sock_connected, &fds_write);
		FD_SET(sock_connected, &fds_except);

		memset(&tv, 0, sizeof(tv));
		tv.tv_sec = 0;
		tv.tv_usec = 10000;
		result = select(sock_max + 1, &fds_read, NULL, &fds_except, &tv);
		if (result > 0)
		{
			if (FD_ISSET(sock_connected, &fds_read))
			{
				/* Receive another block of data into the cRxedData buffer. */
				bytes_received = tuning_base_tcp_recv(sock_connected, &comddata.stComd, sizeof(STCOMD));
				if (bytes_received > 0)
				{
					bytes_received = tuning_main_porcess_data(parent, sock_connected, &comddata);
					if (bytes_received < 0)
					{
						err("tuning_main_porcess_data error ...\n");
						break;
					}
				}
				else
				{
					/* Error (maybe the connected socket already shut down the socket?). */
					err("connected shutdown ...\n");
					break;
				}
			}
			if (FD_ISSET(sock_connected, &fds_except))
			{
				printf("fds_except ...\n");
				break;
			}
		}
	}

	printf("Close the connect %08X ...\n", (uint32_t)sock_connected);
	tuning_base_remove_client(sock_connected);
	tuning_base_dump_client();

	closesocket(sock_connected);
	complete_tuning_thread(sock_connected);

	return 0;
}

int32_t tuning_main_porcess_data(StarfiveServer* parent, SOCKET sock, STCOMDDATA* pcomddata)
{
	int32_t ret = 0;
	int32_t bytes_received = 0;

	if (GEN_SET_FILE == pcomddata->stComd.u32Cmd &&
		CMD_TYPE_FILE == pcomddata->stComd.utPara.cmdPar.u3Type &&
		CMD_DIR_SET == pcomddata->stComd.utPara.cmdPar.u1Dir)
	{
		bytes_received = tuning_base_receive_file(sock, pcomddata);
		if (bytes_received < 0)
			return -1;
	}
	else
	{
		if (pcomddata->stComd.u32Size > 0)
		{
			bytes_received = tuning_base_tcp_recv(sock, pcomddata->szBuffer, pcomddata->stComd.u32Size);
			if (bytes_received != (int32_t)pcomddata->stComd.u32Size)
				return -1;
		}

		ret = tuning_main_parse_command(parent, sock, pcomddata);

	}

	return ret;
}

int32_t tuning_main_parse_command(StarfiveServer* parent, SOCKET sock, STCOMDDATA* pcomddata)
{
	int32_t ret = 0;
	char szcomd[][8] = { "GEN", "ISP" };

	if (g_tuning_debug_enable)
	{
		if (pcomddata->stComd.u32Cmd != GET_GET_SERVER_ALIVE_COUNT)
		{
			printf("ittd_cmd: %u, [%s] ", pcomddata->stComd.u32Cmd, szcomd[pcomddata->stComd.u32Cmd / 10000]);
			printf("ver: %u, ", pcomddata->stComd.u32Ver);
			printf("u1Dir: %u, ", pcomddata->stComd.utPara.cmdPar.u1Dir);
			printf("u3Type: %u, ", pcomddata->stComd.utPara.cmdPar.u3Type);
			printf("u4GroupIdx: %u, ", pcomddata->stComd.utPara.cmdPar.u4GroupIdx);
			printf("u16ParamOrder: %u, ", pcomddata->stComd.utPara.cmdPar.u16ParamOrder);
			printf("utVal: 0x%016llX, \n", (long long unsigned int)pcomddata->stComd.utVal.u64Value);
			printf("utApp.u32Data[0]: 0x%08X, ", pcomddata->stComd.utApp.u32Data[0]);
			printf("utApp.u32Data[1]: 0x%08X, ", pcomddata->stComd.utApp.u32Data[1]);
			printf("utApp.u32Data[2]: 0x%08X, ", pcomddata->stComd.utApp.u32Data[2]);
			printf("utApp.u32Data[3]: 0x%08X, ", pcomddata->stComd.utApp.u32Data[3]);
			printf("utApp.u32Data[4]: 0x%08X, ", pcomddata->stComd.utApp.u32Data[4]);
			printf("datasize: %u \n", pcomddata->stComd.u32Size);
		}
	}

	//if (pcomddata->stComd.u32Cmd >= MODID_GEN_BASE && pcomddata->stComd.u32Cmd < MODID_GEN_BASE + MODID_RANGE)
	if (pcomddata->stComd.u32Cmd < MODID_GEN_BASE + MODID_RANGE)
	{
		ret = tuning_main_parse_general_command(parent, sock, pcomddata);
	}
	else if (pcomddata->stComd.u32Cmd >= MODID_ISP_BASE && pcomddata->stComd.u32Cmd < MODID_ISP_BASE + MODID_RANGE)
	{
		ret = tuning_main_parse_isp_command(parent, sock, pcomddata);
	}

	return ret;
}

int32_t tuning_main_parse_general_command(StarfiveServer* parent, SOCKET sock, STCOMDDATA* pcomddata)
{
	int32_t ret = 0;

	if (pcomddata->stComd.utPara.cmdPar.u1Dir == CMD_DIR_SET)
	{
		ret = tuning_main_set_general_command(sock, pcomddata);
	}
	else if (pcomddata->stComd.utPara.cmdPar.u1Dir == CMD_DIR_GET)
	{
		ret = tuning_main_get_general_command(parent, sock, pcomddata);
	}

	return ret;
}

int32_t tuning_main_parse_isp_command(StarfiveServer* parent, SOCKET sock, STCOMDDATA* pcomddata)
{
	int32_t ret = 0;

	if (pcomddata->stComd.utPara.cmdPar.u1Dir == CMD_DIR_SET)
	{
		//STCOMD* pcomd = (STCOMD *)pcomddata;
		//printf("====isp SET   type: %u, cmd: %u========\n", pcomd->utPara.cmdPar.u3Type, pcomd->u32Cmd - MODID_ISP_BASE);
		parent->setISPParams(pcomddata);
	}
	else if (pcomddata->stComd.utPara.cmdPar.u1Dir == CMD_DIR_GET)
	{
		//STCOMD* pcomd = (STCOMD *)pcomddata;
		//printf("====isp GET   type: %u, cmd: %u========\n", pcomd->utPara.cmdPar.u3Type, pcomd->u32Cmd - MODID_ISP_BASE);
		parent->getISPParams(pcomddata);
		ret = tuning_base_send_data_command(sock, (STCOMD*)pcomddata, (void*)pcomddata->szBuffer, pcomddata->stComd.u32Size);
	}

	return ret;
}


int32_t tuning_main_get_general_command(StarfiveServer* parent, SOCKET sock, STCOMDDATA* pcomddata)
{
	int32_t ret = 0;
	void* pmemaddr = nullptr;
	//printf("---gen GET.   cmd: %u------\n", pcomddata->stComd.u32Cmd - MODID_GEN_BASE);
	switch (pcomddata->stComd.u32Cmd - MODID_GEN_BASE)
	{
		/////////////////////////////////////////////////////////////////////////////////////
		case GEN_GET_SERVER_VERSION:
		{
			pcomddata->stComd.utVal.u32Value = TUNING_SERVER_VERSION;
			if (g_tuning_debug_enable)
				printf("GEN_GET_SERVER_VERSION: 0x%X\n", pcomddata->stComd.utVal.u32Value);
			ret = tuning_base_send_data_command(sock, (STCOMD*)pcomddata, (void*)NULL, 0);
			break;
		}

		/////////////////////////////////////////////////////////////////////////////////////
		case GET_GET_SERVER_ALIVE_COUNT:
		{
			pcomddata->stComd.utVal.u32Value = g_alive_count++;
			if (pcomddata->stComd.utVal.u32Value == 0)
				pcomddata->stComd.utVal.u32Value++;
			ret = tuning_base_send_data_command(sock, (STCOMD*)pcomddata, (void*)NULL, 0);
			break;
		}

		/////////////////////////////////////////////////////////////////////////////////////
		case GEN_GET_REGISTER:
		{
			if (g_tuning_debug_enable)
				printf("GEN_GET_REGISTER: %d\n", pcomddata->stComd.u32Size);
			//!!!!tuning_serv_get_register(pcomddata);
			ret = tuning_base_send_data_command(sock, (STCOMD*)pcomddata, (void*)pcomddata->szBuffer, pcomddata->stComd.u32Size);
			break;
		}

		/////////////////////////////////////////////////////////////////////////////////////
		case GEN_GET_MEMORY:
		{
			if (g_tuning_debug_enable)
				printf("GEN_GET_MEMORY: %d\n", pcomddata->stComd.u32Size);
			//!!!!tuning_serv_get_memory(pcomddata);
			ret = tuning_base_send_data_command(sock, (STCOMD*)pcomddata, (void*)pcomddata->szBuffer, pcomddata->stComd.u32Size);
			break;
		}

		/////////////////////////////////////////////////////////////////////////////////////
		case GEN_GET_IMAGE_INFO:
		{
			if (g_tuning_debug_enable)
				printf("GEN_GET_IMAGE_INFO: %d, Kind: %d\n", pcomddata->stComd.u32Size, pcomddata->stComd.utApp.u32Data[0]);
			parent->getImageInfo(pcomddata);
			ret = tuning_base_send_data_command(sock, (STCOMD*)pcomddata, (void*)NULL, 0);
			break;
		}

		/////////////////////////////////////////////////////////////////////////////////////
		case GEN_GET_IMAGE_DATA:
		{
			if (g_tuning_debug_enable)
				printf("GEN_GET_IMAGE_DATA: %d\n", pcomddata->stComd.u32Size);
			pmemaddr = const_cast<void *>(parent->getImagePop(pcomddata));
			ret = tuning_base_send_data_command(sock, (STCOMD*)pcomddata, (void*)NULL, 0);
			tuning_base_send_packet(sock, pmemaddr, pcomddata->stComd.utApp.u32Data[4]);
			break;
		}

		/////////////////////////////////////////////////////////////////////////////////////
		case GEN_GET_IMAGE_DATA_ONLY:
		{
			if (g_tuning_debug_enable)
				printf("GEN_GET_IMAGE_DATA_ONLY: %d\n", pcomddata->stComd.u32Size);
			//!!!!tuning_serv_get_completed_image(pcomddata, &pmemaddr);
			tuning_base_send_packet(sock, pmemaddr, pcomddata->stComd.utApp.u32Data[4]);
			break;
		}

		/////////////////////////////////////////////////////////////////////////////////////
		case GEN_GET_IMAGE_POP:
		{
			if (g_tuning_debug_enable)
				printf("GEN_GET_IMAGE_POP: %d\n", pcomddata->stComd.u32Size);

			uint32_t pksize = sizeof(struct ISPImageInfo);
			const void *image = parent->getImagePop(pcomddata);
			if(image)
				pksize += pcomddata->stComd.utApp.u32Data[4];
			std::unique_ptr<uint8_t> buffer(new uint8_t[pksize]);
			struct ISPImageInfo *head = (struct ISPImageInfo *)(buffer.get());
			head->width = pcomddata->stComd.utApp.u32Data[0];
			head->height = pcomddata->stComd.utApp.u32Data[1];
			head->stride = pcomddata->stComd.utApp.u32Data[2];
			head->bit = pcomddata->stComd.utApp.u32Data[3];
			head->mosaic = pcomddata->stComd.utVal.u32Value >> 16;
			head->memsize = pcomddata->stComd.utApp.u32Data[4];

			if(image)
			{
				void *data = buffer.get() + sizeof(struct ISPImageInfo);
				memcpy(data, image, head->memsize);
			} else
				head->memsize = 0;
			tuning_base_send_packet(sock, buffer.get(), pksize);
			break;
		}

        /////////////////////////////////////////////////////////////////////////////////////
        case GEN_GET_ISP_VERSION:
        {
            if (g_tuning_debug_enable)
                printf("GEN_GET_ISP_VERSION: %d\n", pcomddata->stComd.u32Size);
            //!!!!tuning_serv_get_isp_version(pcomddata);
            ret = tuning_base_send_data_command(sock, (STCOMD*)pcomddata, (void*)NULL, 0);
            break;
        }

        /////////////////////////////////////////////////////////////////////////////////////
        case GEN_GET_SENSOR_REG:
        {
            if (g_tuning_debug_enable)
                printf("GEN_GET_SENSOR_REG: %d\n", pcomddata->stComd.u32Size);
            //!!!!tuning_serv_get_sensor_reg(pcomddata);
            ret = tuning_base_send_data_command(sock, (STCOMD*)pcomddata, (void*)NULL, 0);
            break;
        }

        /////////////////////////////////////////////////////////////////////////////////////
		case GEN_GET_MODULE_ENABLE:
		{
            if (g_tuning_debug_enable)
                printf("GEN_GET_MODULE_ENABLE: %d\n", pcomddata->stComd.u32Size);
            //!!!!tuning_serv_is_module_enable(pcomddata);
            ret = tuning_base_send_data_command(sock, (STCOMD*)pcomddata, (void*)NULL, 0);
		    break;
		}

        /////////////////////////////////////////////////////////////////////////////////////
		case GEN_GET_MODULE_UPDATE:
        {
            if (g_tuning_debug_enable)
                printf("GEN_GET_MODULE_UPDATE: %d\n", pcomddata->stComd.u32Size);
            //!!!!tuning_serv_is_module_update(pcomddata);
            ret = tuning_base_send_data_command(sock, (STCOMD*)pcomddata, (void*)NULL, 0);
            break;
        }

        /////////////////////////////////////////////////////////////////////////////////////
		case GEN_GET_CONTROL_ENABLE:
        {
            if (g_tuning_debug_enable)
                printf("GEN_GET_CONTROL_ENABLE: %d\n", pcomddata->stComd.u32Size);
            //!!!!tuning_serv_is_control_enable(pcomddata);
            ret = tuning_base_send_data_command(sock, (STCOMD*)pcomddata, (void*)NULL, 0);
            break;
        }

        /////////////////////////////////////////////////////////////////////////////////////
		case GEN_GET_EXPOSURE_GAIN:
        {
            if (g_tuning_debug_enable)
                printf("GEN_GET_EXPOSURE_GAIN: %d\n", pcomddata->stComd.u32Size);
            //!!!!tuning_serv_get_exposure_gain(pcomddata);
            ret = tuning_base_send_data_command(sock, (STCOMD*)pcomddata, (void*)NULL, 0);
            break;
        }

        /////////////////////////////////////////////////////////////////////////////////////
		case GEN_GET_AWB_GAIN:
        {
            if (g_tuning_debug_enable)
                printf("GEN_GET_AWB_GAIN: %d\n", pcomddata->stComd.u32Size);
            //!!!!tuning_serv_get_awb_gain(pcomddata);
            ret = tuning_base_send_data_command(sock, (STCOMD*)pcomddata, (void*)NULL, 0);
            break;
        }
	}

	return ret;
}

int32_t tuning_main_set_general_command([[maybe_unused]] SOCKET sock, STCOMDDATA* pcomddata)
{
	int32_t s32Ret = 0;
	sock = 0;
	//printf("---gen SET.   cmd: %u------\n", pcomddata->stComd.u32Cmd - MODID_GEN_BASE);
	switch (pcomddata->stComd.u32Cmd - MODID_GEN_BASE)
	{
		/////////////////////////////////////////////////////////////////////////////////////
		case GEN_SET_SERVER_DEBUG_ENABLE:
		{
			if (g_tuning_debug_enable)
				printf("GEN_SET_SERVER_DEBUG_ENABLE: %d\n", pcomddata->stComd.utVal.u32Value);
			g_tuning_debug_enable = (BOOL)pcomddata->stComd.utVal.u32Value;
			break;
		}

		/////////////////////////////////////////////////////////////////////////////////////
		case GEN_SET_REGISTER:
		{
			if (g_tuning_debug_enable)
				printf("GEN_SET_REGISTER: %d\n", pcomddata->stComd.u32Size);
			//!!!!tuning_serv_set_register(pcomddata);
			break;
		}

		/////////////////////////////////////////////////////////////////////////////////////
		case GEN_SET_MEMORY:
		{
			if (g_tuning_debug_enable)
				printf("GEN_SET_MEMORY: %d\n", pcomddata->stComd.u32Size);
			//!!!!tuning_serv_set_memory(pcomddata);
			break;
		}

		/////////////////////////////////////////////////////////////////////////////////////
		case GEN_SET_ISP_START:
		{
			if (g_tuning_debug_enable)
				printf("GEN_SET_ISP_START: %d\n", pcomddata->stComd.u32Size);
			//!!!!ispc_main_driver_isp_start();
			break;
		}

		/////////////////////////////////////////////////////////////////////////////////////
		case GEN_SET_ISP_STOP:
		{
			if (g_tuning_debug_enable)
				printf("GEN_SET_ISP_STOP: %d\n", pcomddata->stComd.u32Size);
			//!!!!ispc_main_driver_isp_stop();
			break;
		}

        /////////////////////////////////////////////////////////////////////////////////////
        case GEN_SET_SENSOR_REG:
        {
            if (g_tuning_debug_enable)
                printf("GEN_SET_SENSOR_REG: %d\n", pcomddata->stComd.u32Size);
            //!!!!tuning_serv_set_sensor_reg(pcomddata);
            break;
        }

        /////////////////////////////////////////////////////////////////////////////////////
        case GEN_SET_MODULE_ENABLE:
        {
            if (g_tuning_debug_enable)
                printf("GEN_SET_MODULE_ENABLE: %d\n", pcomddata->stComd.u32Size);
            //!!!!tuning_serv_module_enable(pcomddata);
            break;
        }

        /////////////////////////////////////////////////////////////////////////////////////
        case GEN_SET_MODULE_UPDATE:
        {
            if (g_tuning_debug_enable)
                printf("GEN_SET_MODULE_UPDATE: %d\n", pcomddata->stComd.u32Size);
            //!!!!tuning_serv_module_update(pcomddata);
            break;
        }

        /////////////////////////////////////////////////////////////////////////////////////
        case GEN_SET_CONTROL_ENABLE:
        {
            if (g_tuning_debug_enable)
                printf("GEN_SET_CONTROL_ENABLE: %d\n", pcomddata->stComd.u32Size);
            //!!!!tuning_serv_control_enable(pcomddata);
            break;
        }

        /////////////////////////////////////////////////////////////////////////////////////
        case GEN_SET_EXPOSURE_GAIN:
        {
            if (g_tuning_debug_enable)
                printf("GEN_SET_EXPOSURE_GAIN: %d\n", pcomddata->stComd.u32Size);
            //!!!!tuning_serv_set_exposure_gain(pcomddata);
            break;
        }

        /////////////////////////////////////////////////////////////////////////////////////
        case GEN_SET_AWB_GAIN:
        {
            if (g_tuning_debug_enable)
                printf("GEN_SET_AWB_GAIN: %d\n", pcomddata->stComd.u32Size);
            //!!!!tuning_serv_set_awb_gain(pcomddata);
            break;
        }

        /////////////////////////////////////////////////////////////////////////////////////
        case GEN_SET_SETTING_FILE:
        {
            if (g_tuning_debug_enable)
                printf("GEN_SET_SETTING_FILE: %d\n", pcomddata->stComd.u32Size);
            //!!!!tuning_serv_set_setting_file(pcomddata);
            break;
        }

        /////////////////////////////////////////////////////////////////////////////////////
        case GEN_SET_DEL_SETTING_FILE:
        {
            if (g_tuning_debug_enable)
                printf("GEN_SET_DEL_SETTING_FILE: %d\n", pcomddata->stComd.u32Size);
            //!!!!tuning_serv_set_delete_setting_file(pcomddata);
            break;
        }

        /////////////////////////////////////////////////////////////////////////////////////
        case GEN_SET_RESTORE_SETTING:
        {
            if (g_tuning_debug_enable)
                printf("GEN_SET_RESTORE_SETTING: %d\n", pcomddata->stComd.u32Size);
            //!!!!tuning_serv_set_restore_setting(pcomddata);
            break;
        }
	}

	return s32Ret;
}


