/**
  ******************************************************************************
  * @file tuning_base.h
  * @author  StarFive Isp Team
  * @version  V1.0
  * @date  06/21/2022
  * @brief StarFive ISP tuning server tuning base functions header
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

#ifndef __TUNING_BASE_H__
#define __TUNING_BASE_H__

#include <stdint.h>
#include <thread>
#include "comm.h"

#define MODID_RANGE							9999
#define MODID_GEN_BASE						0
#define MODID_ISP_BASE						10000
#define MODID_ISPOST_BASE					20000
#define MODID_H264_BASE						30000
#define CMD_DATA_BUF_SIZE                   (1024 * 16)
#define CMD_FILE_BUF_SIZE                   250

typedef enum _CMD_DIR
{
	CMD_DIR_GET = 0,
	CMD_DIR_SET,
	CMD_DIR_MAX
} CMD_DIR, *PCMD_DIR;

typedef enum _CMD_TYPE
{
	CMD_TYPE_SINGLE = 0,
	CMD_TYPE_GROUP,
	CMD_TYPE_GROUP_ASAP,
	CMD_TYPE_ALL,
	CMD_TYPE_FILE,
	CMD_TYPE_MAX
} CMD_TYPE, *PCMD_TYPE;

typedef enum _GEN_ID
{
	GEN_GET_BASE = 0,
	GEN_GET_SERVER_VERSION,
	GET_GET_SERVER_ALIVE_COUNT,
	GEN_GET_REGISTER,
	GEN_GET_MEMORY,
	GEN_GET_IMAGE_INFO,
	GEN_GET_IMAGE_DATA,
	GEN_GET_IMAGE_DATA_ONLY,
	GEN_GET_IMAGE_POP,
	GEN_GET_ISP_VERSION,
	GEN_GET_MODULE_ENABLE,
	GEN_GET_MODULE_UPDATE,
	GEN_GET_CONTROL_ENABLE,
	GEN_GET_EXPOSURE_GAIN,
	GEN_GET_AWB_GAIN,
	GEN_GET_SENSOR_REG,

	GEN_SET_BASE = 100,
	GEN_SET_SERVER_DEBUG_ENABLE,
	GEN_SET_REGISTER,
	GEN_SET_MEMORY,
	GEN_SET_ISP_START,
	GEN_SET_ISP_STOP,
	GEN_SET_MODULE_ENABLE,
	GEN_SET_MODULE_UPDATE,
	GEN_SET_CONTROL_ENABLE,
	GEN_SET_EXPOSURE_GAIN,
	GEN_SET_AWB_GAIN,
	GEN_SET_SENSOR_REG,
	GEN_SET_SETTING_FILE,
	GEN_SET_DEL_SETTING_FILE,
	GEN_SET_RESTORE_SETTING,

	GEN_SET_FILE = 9988,
} GEN_ID;

typedef enum _EN_MODULE_ID {
    EN_MODULE_ID_SUD_CSI_OFF = 0,   // 00
    EN_MODULE_ID_SUD_ISP_OFF,       // 01
    EN_MODULE_ID_CSI_IN,            // 02
    EN_MODULE_ID_CBAR,              // 03
    EN_MODULE_ID_CROP,              // 04
    EN_MODULE_ID_DC,                // 05
    EN_MODULE_ID_DEC,               // 06
    EN_MODULE_ID_OBA,               // 07
    EN_MODULE_ID_OBC,               // 08
    EN_MODULE_ID_CPD,               // 09
    EN_MODULE_ID_LCBQ,              // 10
    EN_MODULE_ID_SC,                // 11
    EN_MODULE_ID_DUMP,              // 12
    EN_MODULE_ID_ISP_IN,            // 13
    EN_MODULE_ID_DBC,               // 14
    EN_MODULE_ID_CTC,               // 15
    EN_MODULE_ID_STNR,              // 16
    EN_MODULE_ID_OECF,              // 17
    EN_MODULE_ID_OECFHM,            // 18
    EN_MODULE_ID_LCCF,              // 19
    EN_MODULE_ID_LS,                // 20
    EN_MODULE_ID_AWB,               // 21
    EN_MODULE_ID_PF,                // 22
    EN_MODULE_ID_CA,                // 23
    EN_MODULE_ID_CFA,               // 24
    EN_MODULE_ID_CAR,               // 25
    EN_MODULE_ID_CCM,               // 26
    EN_MODULE_ID_LUT,               // 27
    EN_MODULE_ID_GMARGB,            // 28
    EN_MODULE_ID_R2Y,               // 29
    EN_MODULE_ID_WDR,               // 30
    EN_MODULE_ID_YHIST,             // 31
    EN_MODULE_ID_YCRV,              // 32
    EN_MODULE_ID_SHRP,              // 33
    EN_MODULE_ID_DNYUV,             // 34
    EN_MODULE_ID_SAT,               // 35
    EN_MODULE_ID_OUT_UO,            // 36
    EN_MODULE_ID_OUT_SS0,           // 37
    EN_MODULE_ID_OUT_SS1,           // 38
    EN_MODULE_ID_OUT,               // 39
    EN_MODULE_ID_TIL_1_RD,          // 40
    EN_MODULE_ID_TIL_1_WR,          // 41
    EN_MODULE_ID_TIL_2_RD,          // 42
    EN_MODULE_ID_TIL_2_WR,          // 43
    EN_MODULE_ID_TIL,               // 44
    EN_MODULE_ID_BUF,               // 45
    EN_MODULE_ID_SUD_CSI,           // 46
    EN_MODULE_ID_SUD_ISP,           // 47
    EN_MODULE_ID_MAX                // 48
} EN_MODULE_ID;

#define TUNING_TOOL_CONTROL_ID_START_NO         0x80

typedef enum _EN_CONTROL_ID {
    EN_CONTROL_ID_AE = TUNING_TOOL_CONTROL_ID_START_NO,           // 00   // 128
    EN_CONTROL_ID_AWB,              // 01   // 129
    EN_CONTROL_ID_CSI_IN,           // 02   // 130
    EN_CONTROL_ID_CBAR,             // 03   // 131
    EN_CONTROL_ID_CROP,             // 04   // 132
    EN_CONTROL_ID_DC,               // 05   // 133
    EN_CONTROL_ID_DEC,              // 06   // 134
    EN_CONTROL_ID_OBA,              // 07   // 135
    EN_CONTROL_ID_OBC,              // 08   // 136
    EN_CONTROL_ID_CPD,              // 09   // 137
    EN_CONTROL_ID_LCBQ,             // 10   // 138
    EN_CONTROL_ID_SC,               // 11   // 139
    EN_CONTROL_ID_DUMP,             // 12   // 140
    EN_CONTROL_ID_ISP_IN,           // 13   // 141
    EN_CONTROL_ID_DBC,              // 14   // 142
    EN_CONTROL_ID_CTC,              // 15   // 143
    EN_CONTROL_ID_STNR,             // 16   // 144
    EN_CONTROL_ID_OECF,             // 17   // 145
    EN_CONTROL_ID_OECFHM,           // 18   // 146
    EN_CONTROL_ID_LCCF,             // 19   // 147
    EN_CONTROL_ID_LS,               // 20   // 148
    EN_CONTROL_ID_PF,               // 21   // 149
    EN_CONTROL_ID_CA,               // 22   // 150
    EN_CONTROL_ID_CFA,              // 23   // 151
    EN_CONTROL_ID_CAR,              // 24   // 152
    EN_CONTROL_ID_CCM,              // 25   // 153
    EN_CONTROL_ID_LUT,              // 26   // 154
    EN_CONTROL_ID_GMARGB,           // 27   // 155
    EN_CONTROL_ID_R2Y,              // 28   // 156
    EN_CONTROL_ID_WDR,              // 29   // 157
    EN_CONTROL_ID_YHIST,            // 30   // 158
    EN_CONTROL_ID_YCRV,             // 31   // 159
    EN_CONTROL_ID_SHRP,             // 32   // 160
    EN_CONTROL_ID_DNYUV,            // 33   // 161
    EN_CONTROL_ID_SAT,              // 34   // 162
    EN_CONTROL_ID_OUT_UO,           // 35   // 163
    EN_CONTROL_ID_OUT_SS0,          // 36   // 164
    EN_CONTROL_ID_OUT_SS1,          // 37   // 165
    EN_CONTROL_ID_OUT,              // 38   // 166
    EN_CONTROL_ID_TIL_1_RD,         // 39   // 167
    EN_CONTROL_ID_TIL_1_WR,         // 40   // 168
    EN_CONTROL_ID_TIL_2_RD,         // 41   // 169
    EN_CONTROL_ID_TIL_2_WR,         // 42   // 170
    EN_CONTROL_ID_TIL,              // 43   // 171
    EN_CONTROL_ID_BUF,              // 44   // 172
    EN_CONTROL_ID_SUD_CSI,          // 45   // 173
    EN_CONTROL_ID_SUD_ISP,          // 46   // 174
    EN_CONTROL_ID_MAX,              // 47   // 175
} EN_CONTROL_ID;

#define TUNING_TOOL_GEN_GET_ID_START_NO         0x100
typedef enum _EN_GEN_GET_ID {
    EN_GEN_GET_ID_IMG_INFO = TUNING_TOOL_GEN_GET_ID_START_NO,
} EN_GEN_GET_ID;

typedef enum _EN_IMG_MEM_KIND {
    EN_MEM_KIND_UO = 0x0001,
    EN_MEM_KIND_SS0 = 0x0002,
    EN_MEM_KIND_SS1 = 0x0004,
    EN_MEM_KIND_DUMP = 0x0008,
} EN_IMG_MEM_KIND;

typedef enum MOSAICType
{
    MOSAIC_NONE = 0,
    MOSAIC_RGGB,
    MOSAIC_GRBG,
    MOSAIC_GBRG,
    MOSAIC_BGGR
} eMOSAIC;

struct ISPImageInfo
{ 
	uint32_t width;
	uint32_t height;
	uint32_t stride;
	uint32_t bit;
	uint32_t mosaic;
	uint32_t memsize;
};

typedef union _UTVALUE
{
	INT64 i64Value;
	UINT64 u64Value;
	DOUBLE dValue;
	int32_t i32Value;
	uint32_t u32Value;
	FLOAT fValue;
	int16_t i16Value;
	uint16_t u16Value;
	int8_t i8Value;
	uint8_t u8Value;
	BOOL bValue;
} UTVALUE, *PUTVALUE;

typedef union _UTPARA
{
	uint32_t u32Value;
	struct _cmdPar
	{
		uint32_t u16ParamOrder : 16;	//[15:0]:parameter order.
		uint32_t u8Rev16 : 8;           //[23:16]:revered.
		uint32_t u4GroupIdx : 4;        //[27:24}:group index.
		uint32_t u3Type : 3;            //[30:28]:command type.
		uint32_t u1Dir : 1;             //[31]:command direction.
	} cmdPar;
} UTPARA, *PUTPARA;

typedef union _UTAPPEND
{
	uint32_t u32Data[5];
} UTAPPEND, *PUTAPPEND;

typedef struct _STCOMD
{
	uint32_t u32Cmd;
	UTPARA utPara;
	uint32_t u32Ver;
	uint32_t u32Size;
	UTVALUE utVal;
	UTAPPEND utApp;
} STCOMD;

typedef struct _STCOMDDATA
{
	STCOMD stComd;
	char szBuffer[CMD_DATA_BUF_SIZE];
} STCOMDDATA;

typedef struct _STQSDKVER
{
	uint32_t u32Version;
	uint8_t u8QsdkCmdTbl[256];
} STQSDKVER;

typedef struct _SC_AE_WS_R_G
{
	uint32_t u32R;
	uint32_t u32G;
} SC_AE_WS_R_G;

typedef struct _SC_AE_WS_B_Y
{
	uint32_t u32B;
	uint32_t u32Y;
} SC_AE_WS_B_Y;

typedef struct _SC_AWB_PS_R_G
{
	uint32_t u32R;
	uint32_t u32G;
} SC_AWB_PS_R_G;

typedef struct _SC_AWB_PS_B_CNT
{
	uint32_t u32B;
	uint32_t u32CNT;
} SC_AWB_PS_B_CNT;

typedef struct _SC_AWB_WGS_W_RW
{
	uint32_t u32W;
	uint32_t u32RW;
} SC_AWB_WGS_W_RW;

typedef struct _SC_AWB_WGS_GW_BW
{
	uint32_t u32GW;
	uint32_t u32BW;
} SC_AWB_WGS_GW_BW;

typedef struct _SC_AWB_WGS_GRW_GBW
{
	uint32_t u32GRW;
	uint32_t u32GBW;
} SC_AWB_WGS_GRW_GBW;

typedef struct _SC_AF_ES_DAT_CNT
{
	uint32_t u32DAT;
	uint32_t u32CNT;
} SC_AF_ES_DAT_CNT;

typedef struct _SC_AE_HIST_R_G
{
	uint32_t u32R;
	uint32_t u32G;
} SC_AE_HIST_R_G;

typedef struct _SC_AE_HIST_B_Y
{
	uint32_t u32B;
	uint32_t u32Y;
} SC_AE_HIST_B_Y;


typedef struct _SC_DATA
{
	SC_AE_WS_R_G AeWsRG[16];
	SC_AE_WS_B_Y AeWsBY[16];
	SC_AWB_PS_R_G AwbPsRG[16];
	SC_AWB_PS_B_CNT AwbPsBCNT[16];
	SC_AWB_WGS_W_RW AwbWgsWRW[16];
	SC_AWB_WGS_GW_BW AwbWgsGWBW[16];
	SC_AWB_WGS_GRW_GBW AwbWgsGRWGBW[16];
	SC_AF_ES_DAT_CNT AfEsDATCNT[16];
} SC_DATA;

typedef struct _SC_HIST
{
	SC_AE_HIST_R_G AeHistRG[64];
	SC_AE_HIST_B_Y AeHistBY[64];
} SC_HIST;

typedef struct _SC_DUMP
{
	SC_DATA ScData[16];
	SC_HIST ScHist;
} SC_DUMP;

#pragma pack(push, 1)

typedef struct _ST_EXPO_GAIN
{
    uint32_t u32Exposure;
    DOUBLE dGain;
} ST_EXPO_GAIN;

typedef struct _ST_AWB_GAIN
{
    DOUBLE dRGain;
    DOUBLE dBGain;
    DOUBLE dDGain;
} ST_AWB_GAIN;

#pragma pack(pop)


//std::thread &get_tuning_task_thread();
std::thread &get_new_tuning_thread_obj(SOCKET socket);
void join_tuning_task_threads();
void complete_tuning_thread(SOCKET socket);
void join_complete_tuning_threads();
uint32_t tuning_base_get_client_count();
void tuning_base_initial_client(void);
uint16_t tuning_base_insert_client(SOCKET sock);
uint16_t tuning_base_remove_client(SOCKET sock);
void tuning_base_dump_client();
void tuning_base_dump_data(void* pData, uint32_t size);
void tuning_base_client_print(SOCKET sock, char* pstring);
int32_t tuning_base_tcp_send(SOCKET sock, void* pdata, int32_t length);
int32_t tuning_base_tcp_recv(SOCKET sock, void* pdata, int32_t length);
int32_t tuning_base_send_command(SOCKET sock, uint32_t mod, uint32_t dir, uint32_t type, uint32_t datasize, UTVALUE val);
int32_t tuning_base_send_data_command(SOCKET sock, STCOMD* pcomd, void* pdata, uint32_t datasize);
//int32_t tuning_base_send_file(SOCKET sock, char* pfilename, void* pdata, uint32_t datasize);
int32_t tuning_base_receive_file(SOCKET sock, STCOMDDATA* pcomddata);
int32_t tuning_base_send_packet(SOCKET sock, void* pdata, uint32_t datasize);





#endif //__TUNING_BASE_H__

