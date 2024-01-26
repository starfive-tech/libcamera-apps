/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (C) 2023, Starfive Technology Co., Ltd.
 *
 * starfiveIPAParams.h - Starfive pipeline module parameters
 */
#pragma once

#define STARFIVE_GIR_COOKIE					((uint64_t)1 << 32)

namespace libcamera {

namespace starfive {

#pragma pack(push, 1)

namespace AGC {
// control parameters
struct ExpoLockTable {
    uint8_t expoLockCnt;
    struct expoLock {
        uint32_t expoValue;
        uint32_t snrExpoTime;
    } expoLocks[16];
};

struct OverSatCompensation {
    uint8_t t0;
    uint8_t t1;
    uint8_t brightMax;
};

#define AE_CTRL_PARAMETERS      bool ctlEnable;     \
    uint8_t targetBrightness;   \
	uint8_t targetBracket;  \
	uint8_t aeWeightTbl[256];   \
	uint16_t expoGainStep;  \
	uint32_t aeExposureMax; \
	double aeGainMax;   \
	double aeAdcMax;    \
	uint8_t expoLockFreq;   \
	struct libcamera::starfive::AGC::ExpoLockTable lockTable50;   \
	struct libcamera::starfive::AGC::ExpoLockTable lockTable60;   \
    double raisingCoarseThres;  \
	double fallingCoarseThres;  \
	double coarseSpeed; \
    struct libcamera::starfive::AGC::OverSatCompensation overComp;    \
	struct libcamera::starfive::AGC::OverSatCompensation underComp;   \
	uint8_t overWeightTable[32];    \
	uint8_t underWeightTable[32];   \
    bool moduleEnable;  \
	uint32_t expectSnrExpo; \
	double expectSnrAgc;    \
	double expectAdc

#define AE_CURRENT_STATUS	uint8_t currentBrightness;	\
	uint8_t yOverPer;	\
	uint8_t yUnderPer;	\
	uint8_t overOffset;	\
	uint8_t underOffset;	\
	uint32_t currentEV;	\
	uint32_t expectEV;	\
	uint32_t actualSnrExpo;	\
	double actualSnrAgc

struct AECtrlParams {
	/*
	bool ctlEnable;
	// setting
	uint8_t targetBrightness;
	uint8_t targetBracket;
	uint8_t aeWeightTbl[256]; // 0 ~ 15
	uint16_t expoGainStep;
	uint32_t aeExposureMax;
	double aeGainMax;
	double aeAdcMax;
	uint8_t expoLockFreq;
	struct ExpoLockTable lockTable50;
	struct ExpoLockTable lockTable60;

	// coarse mode
	double raisingCoarseThres;
	double fallingCoarseThres;
	double coarseSpeed;

	// saturation compensation
	struct OverSatCompensation overComp;
	struct OverSatCompensation underComp;
	uint8_t overWeightTable[32];
	uint8_t underWeightTable[32];

	// manual mode
	bool moduleEnable;
	uint32_t expectSnrExpo;
	double expectSnrAgc;
	double expectAdc;
*/
	AE_CTRL_PARAMETERS;

	// AE Info
/*	uint8_t currentBrightness;
	uint8_t yOverPer;
	uint8_t yUnderPer;
	uint8_t overOffset;
	uint8_t underOffset;

	uint32_t currentEV;
	uint32_t expectEV;

	uint32_t actualSnrExpo;
	double actualSnrAgc;
*/
	AE_CURRENT_STATUS;
};

} // namespace AGC

namespace AWB {
struct TempGain {
	uint16_t temperature;
	double redGain;
	double blueGain;
};

#define AWB_CTRL_PARAMETERS	bool ctlEnable;	\
	double updateSpeed;	\
	int tempGainNum;	\
	struct libcamera::starfive::AWB::TempGain temperatureTable[6]

#define AWB_CURRENT_STATUS	double targetRedGain;	\
	double targetBlueGain;	\
	double expectRedGain;	\
	double expectBlueGain;	\
	uint16_t measureTemperature

struct AWBCtrlParams {
/*
	// Auto mode
	bool ctlEnable;
	double updateSpeed;
	
	// Temperatuer table
	int tempGainNum;
	struct TempGain temperatureTable[6];
*/
	AWB_CTRL_PARAMETERS;
/*
	// AWB info
	double targetRedGain;
	double targetBlueGain;
	double expectRedGain;
	double expectBlueGain;
	uint16_t measureTemperature;
*/
	AWB_CURRENT_STATUS;
};

struct AWBModuleParams {
	// WB module
	bool moduleEnable;

	// WB setting
	double redGain;
	double greenGain;
	double blueGain;
};

} // namespace AWB

namespace CCM {
struct CcmInfo {
	uint16_t temperature;
	double matrix[9];
	int16_t offset[3];
};

#define CCM_CTRL_PARAMETERS	bool ctlEnable;	\
	int tempGainNum;	\
	struct libcamera::starfive::CCM::CcmInfo ccmTable[6]

struct CCMCtrlParams {
/*
	// Auto mode
	bool ctlEnable;

	// CCM table
	int tempGainNum;
	struct CcmInfo ccmTable[6];
	*/
	CCM_CTRL_PARAMETERS;
};

struct CCMModuleParams {
	// CCM module
	bool moduleEnable;

	// CCM setting
	struct CcmInfo matrix;
};

} // namespace CCM

namespace DNYUV {
struct Curve {
		uint16_t level;
		uint8_t weight;
};

struct YUVCurve {
	uint16_t iso;
	struct Curve yCurve[7];
	struct Curve uvCurve[7];
};

#define DNYUV_CTRL_PARAMETERS bool ctlEnable;	\
	struct libcamera::starfive::DNYUV::YUVCurve curveTable[10]

struct DNYUVCtrlParams {
/*
	// Auto mode
	bool ctlEnable;

	// DNYUV ISO table
	struct YUVCurve curveTable[10];
*/
	DNYUV_CTRL_PARAMETERS;
};

#define DNYUV_MODULE_PARAMETERS	bool moduleEnable;	\
	uint8_t ysWeight[10];	\
	uint8_t uvsWeight[10]

struct DNYUVModuleParams {
/*
	bool moduleEnable;
	uint8_t ysWeight[10];
	uint8_t uvsWeight[10];
*/
	DNYUV_MODULE_PARAMETERS;
	struct Curve yCurve[7];
	struct Curve uvCurve[7];
};

} // namespace DNYUV

namespace LCCF {
struct Factors {
	uint16_t iso;
	double rF1;
	double rF2;
	double grF1;
	double grF2;
	double gbF1;
	double gbF2;
	double bF1;
	double bF2;
};

#define LCCF_CTRL_PARAMETERS bool ctlEnable;	\
	struct libcamera::starfive::LCCF::Factors table[10]

struct LCCFCtrlParams {
	/*
	bool ctlEnable;

	struct Factors table[10];
	*/
	LCCF_CTRL_PARAMETERS;
};

#define LCCF_MODULE_PARAMETERS	bool moduleEnable;	\
	int16_t centerX;	\
	int16_t centerY;	\
	uint8_t radius

struct LCCFModuleParams {
/*
	bool moduleEnable;
	int16_t centerX;
	int16_t centerY;
	uint8_t radius;
*/
	LCCF_MODULE_PARAMETERS;
	struct Factors curFactor;
};

} // namespace LCCF

namespace SAT {
struct CtlPoint {
	uint16_t threshold;
	double factor;
};

struct SatCurve {
	uint16_t iso;
	struct CtlPoint pt[2];
};

struct YCtlPoint {
	uint16_t x;
	uint16_t y;
};

struct SatInfo {
	struct CtlPoint satCurve[2];
	uint8_t deNrml;
	int16_t uoffset;
	int16_t voffset;
};

#define SAT_CTRL_PARAMETERS bool ctlEnable;	\
	struct libcamera::starfive::SAT::SatCurve isoCurves[10]

struct SATCtrlParams {
	/*
	ctlEnable;
	struct SatCurve isoCurves[10];
	*/
	SAT_CTRL_PARAMETERS;
};

#define SAT_MODULE_PARAMETERS	bool moduleEnable;	\
	struct libcamera::starfive::SAT::YCtlPoint yCurve[2];	\
	double hueDegree;	\
	struct libcamera::starfive::SAT::SatInfo satInfo

struct SATModuleParams {
	/*
	bool moduleEnable;
	struct YCtlPoint yCurve[2];
	double hueDegree;
	struct SatInfo satInfo;
	*/
	SAT_MODULE_PARAMETERS;
};

} // namespace SAT

namespace SHARPEN {
struct CtlPoint {
	uint16_t diff;
	double factor;
};

struct StrengthCurve {
	uint16_t iso;
	struct CtlPoint pt[4];
};

#define SHARPEN_CTRL_PARAMETERS bool ctlEnable;	\
	struct libcamera::starfive::SHARPEN::StrengthCurve isoCurves[10]

struct SHARPENCtrlParams {
	/*
	ctlEnable;
	struct StrengthCurve isoCurves[10];
	*/
	SHARPEN_CTRL_PARAMETERS;
};

#define SHARPEN_MODULE_PARAMETERS	bool moduleEnable;	\
	uint8_t weight[15];	\
	double posFactor;	\
	double negFactor

struct SHARPENModuleParams {
	/*
	bool moduleEnable;
	uint8_t weight[15];
	double posFactor;
	double negFactor;
	*/
	SHARPEN_MODULE_PARAMETERS;

	struct StrengthCurve yCurve;
};

} // namespace SHARPEN

namespace YCRV {
#define YCRV_CTRL_PARAMETERS bool ctlEnable;	\
	uint16_t updateSpeed;	\
	uint8_t method;	\
	uint16_t maxLimit;	\
	uint16_t minLimit;	\
	uint16_t startLevel;	\
	uint16_t endLevel;	\
	uint8_t startPeriod;	\
	uint8_t endPeriod;	\
	double crvMaxLimit;	\
	double crvMinLimit;	\
	double crvMaxStart;	\
	double crvMinStart;	\
	double crvMaxSlope;	\
	double crvMinSlope;	\
	uint16_t yCurve[65]

#define YCRV_CURRENT_STATUS	uint32_t yHist[64];	\
	uint16_t woDamping[65]

struct YCRVCtrlParams {
	/*
	bool ctlEnable;

	// ycrv_setting
	uint16_t updateSpeed;
	uint8_t method;
	// yhist_minmax_lmtd_with_period
	uint16_t maxLimit;
	uint16_t minLimit;
	uint16_t startLevel;
	uint16_t endLevel;
	uint8_t startPeriod;
	uint8_t endPeriod;
	// yhist_minmax_crv_lmtd
	double crvMaxLimit;
	double crvMinLimit;
	double crvMaxStart;
	double crvMinStart;
	double crvMaxSlope;
	double crvMinSlope;

	// ycrv_info
	uint16_t yCurve[65];
	*/
	YCRV_CTRL_PARAMETERS;

	YCRV_CURRENT_STATUS;
};

struct YCRVModuleParams {
	bool moduleEnable;
	uint16_t yCurve[65];
};

} // namespace YCRV

namespace CAR {
struct CARModuleParams {
	bool moduleEnable;
};
} // namespace CAR

namespace CFA {
#define CFA_MODULE_PARAMETERS	bool moduleEnable;	\
	uint8_t hvWidth;	\
	uint8_t crossCov

struct CFAModuleParams {
	/*
	bool moduleEnable;
	uint8_t hvWidth;
	uint8_t crossCov;
	*/
	CFA_MODULE_PARAMETERS;
};
} // namespace CFA

namespace CTC {
#define CTC_MODULE_PARAMETERS	bool moduleEnable;	\
	uint8_t smoothMode;	\
	uint8_t detailMode;	\
	uint16_t maxGt;	\
	uint16_t minGt
	
struct CTCModuleParams {
	/*
	bool moduleEnable;
	uint8_t smoothMode;
	uint8_t detailMode;
	uint16_t maxGt;
	uint16_t minGt;
	*/
	CTC_MODULE_PARAMETERS;
};
} // namespace CTC

namespace DBC {
#define DBC_MODULE_PARAMETERS	bool moduleEnable;	\
	uint16_t badGt;	\
	uint16_t badXt
	
struct DBCModuleParams {
	/*
	bool moduleEnable;
	uint16_t badGt;
	uint16_t badXt;
	*/
	DBC_MODULE_PARAMETERS;
};
} // namespace DBC

namespace GAMMA {
#define GAMMA_MODULE_PARAMETERS	bool moduleEnable;	\
	uint16_t gammaCurve[15]
	
struct GAMMAModuleParams {
	/*
	bool moduleEnable;
	uint16_t gammaCurve[15];
	*/
	GAMMA_MODULE_PARAMETERS;
};
} // namespace GAMMA

namespace OBC {
template<typename T>
struct WinCoefs {
	T tl;
	T tr;
	T bl;
	T br;
};

#define OBC_MODULE_PARAMETERS	bool moduleEnable;	\
	uint8_t winWidth;	\
	uint8_t winHeight;	\
	struct libcamera::starfive::OBC::WinCoefs<double> gains[4];	\
	struct libcamera::starfive::OBC::WinCoefs<uint8_t> offsets[4]
	
struct OBCModuleParams {
	/*
	bool moduleEnable;
	uint8_t winWidth;
	uint8_t winHeight;
	*/
	OBC_MODULE_PARAMETERS;
};
} // namespace OBC

namespace OECF {
struct point {
		uint16_t x;
		uint16_t y;
	};

#define OECF_MODULE_PARAMETERS	bool moduleEnable;	\
	struct libcamera::starfive::OECF::point rCurve[16];	\
	struct libcamera::starfive::OECF::point grCurve[16];	\
	struct libcamera::starfive::OECF::point gbCurve[16];	\
	struct libcamera::starfive::OECF::point bCurve[16]
	
struct OECFModuleParams {
	/*
	bool moduleEnable;
	struct point rCurve[16];
	struct point grCurve[16];
	struct point gbCurve[16];
	struct point bCurve[16];
	*/
	OECF_MODULE_PARAMETERS;
};
} // namespace OECF

namespace R2Y {

#define R2Y_MODULE_PARAMETERS	bool moduleEnable;	\
	double matrix[9]
	
struct R2YModuleParams {
	/*
	bool moduleEnable;
	double matrix[9];
	*/
	R2Y_MODULE_PARAMETERS;
};
} // namespace R2Y

namespace STAT {
struct AfConfig {
	uint8_t esHorMode;
	uint8_t esSumMode;
	uint8_t horEn;
	uint8_t verEn;
	uint8_t esVerThr;
	uint16_t esHorThr;
};

struct AwbWs {
	uint8_t awbWsRl;
	uint8_t awbWsRu;
	uint8_t awbWsGrl;
	uint8_t awbWsGru;
	uint8_t awbWsGbl;
	uint8_t awbWsGbu;
	uint8_t awbWsBl;
	uint8_t awbWsBu;
};

struct AwbPoint {
	uint16_t intensity;
	uint8_t weight;
};

struct AwbConfig {
	struct AwbWs wsConfig;
	uint8_t awbCw[169];
	struct AwbPoint pts[17];
};

#define STAT_MODULE_PARAMETERS	bool moduleEnable;	\
	struct libcamera::starfive::STAT::AfConfig afConfig;	\
	struct libcamera::starfive::STAT::AwbConfig awbConfig
	
struct STATModuleParams {
	/*
	bool moduleEnable;
	// SC setting
	struct AfConfig afConfig;
	struct AwbConfig awbConfig;
	*/
	STAT_MODULE_PARAMETERS;

	// SC Info
	uint16_t imgWidth;
	uint16_t imgHeight;
	uint16_t hStart;
	uint16_t vStart;
	uint8_t subWinWidth;
	uint8_t subWinHeight;
	uint8_t hPeriod;
	uint8_t hDeep;
	uint8_t vPeriod;
	uint8_t vDeep;
	uint16_t decWidth;
	uint16_t decHeight;
};
} // namespace STAT
#pragma pack(pop)
} // namespace starfive
} // namespace libcamera
