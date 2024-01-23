/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (C) 2023, Starfive Technology Co., Ltd.
 *
 * upside_params.h - tuning tool's parameters data structures.
 */
#pragma once

#include <cstdint>

typedef             char    STF_CHAR;
typedef          wchar_t    STF_WCHAR;

typedef	             int	STF_INT;
typedef	          int8_t	STF_S8;
typedef	         int16_t	STF_S16;
typedef	         int32_t	STF_S32;
typedef	         int64_t	STF_S64;

typedef	    unsigned int	STF_UINT;
typedef	     uint8_t	    STF_U8;
typedef	    uint16_t	    STF_U16;
typedef	    uint32_t	    STF_U32;
typedef	    uint64_t	    STF_U64;

typedef STF_U8              STF_BOOL8;

typedef float               STF_FLOAT;
typedef double              STF_DOUBLE;

#define EXPO_LOCK_ITEM_MAX                      16

#define AE_WT_TBL_COL_MAX                       16
#define AE_WT_TBL_ROW_MAX                       16

#define EN_EXPO_LOCK_FREQ_MAX                   2

#define SAT_WT_TBL_ITEM_MAX                     32

#pragma pack(push, 1)

typedef struct _ST_EXPO_LOCK {
    STF_U32 u32ExpoValue;                       /** Exposure value. */
    STF_U32 u32SnrExpoTime;                     /** Sensor exposure time. */
} ST_EXPO_LOCK;

typedef struct _ST_EXPO_LOCK_TBL {
    STF_U8 u8ExpoLockCnt;                       /** Exposure lock table entry count. It depends on how many exposure lock item will be loading and using. */
    ST_EXPO_LOCK stExpoLock[EXPO_LOCK_ITEM_MAX]; /** Exposure time  lock tables. */
} ST_EXPO_LOCK_TBL;

typedef struct _ST_SAT_COMP_CRV {
    STF_U8 u8T0;                                /** Threshold 0. */
    STF_U8 u8T1;                                /** Threshold 1. */
    STF_U8 u8BrightnessMax;                     /** Maximum compensation value. */
} ST_SAT_COMP_CRV;

typedef struct _ST_CTL_AE_SETTING {
    //-------------------------------------------------------------------------
    // This section parameters value is assign from setting file.
    STF_BOOL8 bEnable;                          /** Enable/Disable auto exposure time and gain control. */
    STF_U8 u8TargetBrightness;                  /** Target brightness values 0-255. */
    STF_U8 u8TargetBracket;                     /** Target bracket value 0-255. Margin around the target brightness which we will consider as a proper exposure (no changes in the exposure settings will be carried out). */
    STF_U8 u8AeWeightTbl[AE_WT_TBL_ROW_MAX][AE_WT_TBL_COL_MAX]; /** Brightness measurement weight table. */
    STF_U16 u16ExpoGainStep;                    /** Maximum gain change step, format 0.8.8. The exposure gain change range is +/-u16ExpoGainStep. */
    STF_U32 u32AeExposureMax;                   /** The AE exposure time(us) maximum limit. */
    STF_DOUBLE dAeGainMax;                      /** The AE gain maximum limit. */
    STF_DOUBLE dAeAdcMax;                       /** The AE digital gain maximum value. */
    STF_U8 u8ExpoLockFreq;                      /** Exposure lock frequency. 0: 50Hz, and 1: 60Hz. */
    ST_EXPO_LOCK_TBL stExpoLockTbl[EN_EXPO_LOCK_FREQ_MAX]; /** Exposure lock tables,
                                                            * first exposure lock table is for 50Hz,
                                                            * the second exposure lock table is for 60Hz,
                                                            * maximum exposure lock item is 16.
                                                            */
    STF_DOUBLE dRisingCoarseThreshold;          /** EV threshold value for AE into rising coarse mode. Range from 1.1 to 16.0. */
    STF_DOUBLE dFallingCoarseThreshold;         /** EV threshold value for AE into falling coarse mode. Range from 1.1 to 3.0. */
    STF_DOUBLE dCoarseSpeed;                    /** AE update speed on coarse mode. 0.1 is the slowest and the 1.0 is the fastest. */
    ST_SAT_COMP_CRV stOverSatCompCurve;         /** Over saturation compensation curve, it's using for brightness value compensation. */
    ST_SAT_COMP_CRV stUnderSatCompCurve;        /** Under saturation compensation curve, it's using for brightness value compensation. */
    STF_U8 u8OverSatWeightTbl[SAT_WT_TBL_ITEM_MAX]; /** Saturation measurement weight table. */
    STF_U8 u8UnderSatWeightTbl[SAT_WT_TBL_ITEM_MAX]; /** Saturation measurement weight table. */
} ST_CTL_AE_SETTING;

typedef struct _ST_CTL_AE_INFO {
    STF_U32 u32ExpectSnrExpo;                   /** The expect sensor exposure time in microseconds. */
    STF_DOUBLE dExpectSnrAgc;                   /** The expect sensor analog gain value. */
    STF_DOUBLE dExpectAdc;                      /** The expect digital gain value, this digital gain will be used on AWB module. */

    STF_U8 u8CurrentBrightness;                 /** The latest brightness value. */

    STF_U8 u8YOverPer;                          /** Measure over saturation percentage on Y channel, range 0-255(100%).*/
    STF_U8 u8YUnderPer;                         /** Measure under saturation percentage on Y channel, range 0-255(100%).*/
    STF_U8 u8OverOffset;                        /** Over saturation compensation value. */
    STF_U8 u8UnderOffset;                       /** Under saturation compensation value. */

    STF_U32 u32CurrentEV;                       /** The latest exposure value in microseconds. */
    STF_U32 u32ExpectEV;                        /** The expect exposure value in microseconds. */

    STF_U32 u32ActualSnrExpo;                   /** The actually program to sensor exposure time. */
    STF_DOUBLE dActualSnrAgc;                   /** The actually program to sensor analog gain. */
} ST_CTL_AE_INFO;

typedef struct _ST_CTL_AE_PARAM {
    ST_CTL_AE_SETTING stSetting;                /** Control AE setting file parameters. */
    //-------------------------------------------------------------------------
    ST_CTL_AE_INFO stInfo;                      /** Control AE output information parameters. */
} ST_CTL_AE_PARAM;

#define TEMP_LEVEL_MAX                          6

typedef struct _ST_AWB_TEMP {
    STF_U16 u16Temperature;                     /** Light temperature degree. */
    STF_DOUBLE dRedGain;                        /** AWB R channel gain. */
    STF_DOUBLE dBlueGain;                       /** AWB B channel gain. */
} ST_AWB_TEMP;

typedef struct _ST_AWB_TEMP_TBL {
    STF_U8 u8AwbTempTblCnt;                     /** Indicate light temperature table counter. */
    ST_AWB_TEMP stAwbTemp[TEMP_LEVEL_MAX];      /** AWB light temperature and white-balance gain table. */
} ST_AWB_TEMP_TBL;

typedef struct _ST_CTL_AWB_SETTING {
    //-------------------------------------------------------------------------
    // This section parameters value is assign from setting file.
    STF_BOOL8 bEnable;                          /** Enable/Disable AWB gain control. */
    STF_DOUBLE dUpdateSpeed;                    /** White balance gain change speed parameters. */
    ST_AWB_TEMP_TBL stAwbTempTbl;               /** AWB temperature information. */
} ST_CTL_AWB_SETTING;

typedef struct _ST_CTL_AWB_INFO {
    STF_DOUBLE dEvalRedGain;                    /** SC evaluation red gain value. */
    STF_DOUBLE dEvalBlueGain;                   /** SC evaluation blue gain value. */
    STF_DOUBLE dExpectRedGain;                  /** AWB next balance R gain. */
    STF_DOUBLE dExpectBlueGain;                 /** AWB next balance B gain. */
    STF_U16 u16MeasureTemperature;              /** AWB measure temperature degree. */
} ST_CTL_AWB_INFO;

typedef struct _ST_CTL_AWB_PARAM {
    ST_CTL_AWB_SETTING stSetting;               /** Control AWB setting file parameters. */
    //-------------------------------------------------------------------------
    // This section parameters is internal using.
    ST_CTL_AWB_INFO stInfo;                     /** Control AWB output information parameters. */
} ST_CTL_AWB_PARAM;

typedef struct _ST_TEMP_CCM {
    STF_DOUBLE dSMLowMatrix[3][3];              /** Low color matrix. */
    STF_S16 s16SMLowOffset[3];                  /** Low color offset. */
} ST_TEMP_CCM;

typedef struct _ST_CCM_TBL {
    STF_U8 u8CcmTblCnt;                         /** Indicate CCM table counter. */
    ST_TEMP_CCM stTempCcm[TEMP_LEVEL_MAX];      /** Light temperature CCM matrix. */
} ST_CCM_TBL;

typedef struct _ST_CTL_CCM_SETTING {
    //-------------------------------------------------------------------------
    // This section parameters value is assign from setting file.
    STF_BOOL8 bEnable;                          /** Enable/Disable CCM control. */
    ST_CCM_TBL stCcmTbl;                        /** CCM light temperature table. */
} ST_CTL_CCM_SETTING;

typedef struct _ST_CTL_CCM_PARAM {
    ST_CTL_CCM_SETTING stSetting;               /** Control CCM setting file parameters. */
} ST_CTL_CCM_PARAM;

#define DNYUV_CRV_PNT_MAX                       7
#define ISO_LEVEL_MAX                           10

typedef struct _ST_DNYUV_PNT {
    STF_U16 u16Level;                           /** Level difference value, range from 0 to 1023. */
    STF_U8 u8Weighting;                         /** Level weighting value, range from 0 to 7. */
} ST_DNYUV_PNT;

typedef struct _ST_DNYUV_CRV {
    ST_DNYUV_PNT stYLevel[DNYUV_CRV_PNT_MAX];   /** Level difference value, range from 0 to 1023. */
    ST_DNYUV_PNT stUvLevel[DNYUV_CRV_PNT_MAX];  /** Level difference value, range from 0 to 1023. */
} ST_DNYUV_CRV;

typedef struct _ST_CTL_DNYUV_SETTING {
    //-------------------------------------------------------------------------
    // This section parameters value is assign from setting file.
    STF_BOOL8 bEnable;                          /** Enable/Disable Y curve control. */
    ST_DNYUV_CRV stDnYuvCurveTbl[ISO_LEVEL_MAX]; /** DnYuv curve table for adaptive 2d de-noise. */
} ST_CTL_DNYUV_SETTING;

typedef struct _ST_CTL_DNYUV_PARAM {
    ST_CTL_DNYUV_SETTING stSetting;             /** Control DNYUV setting file parameters. */
} ST_CTL_DNYUV_PARAM;

#define EN_LCCF_CHN_MAX                         4

typedef struct _ST_LCCF_FACTOR {
    STF_DOUBLE dFactor[EN_LCCF_CHN_MAX][2];
} ST_LCCF_FACTOR;

typedef struct _ST_CTL_LCCF_SETTING {
    //-------------------------------------------------------------------------
    // This section parameters value is assign from setting file.
    STF_BOOL8 bEnable;                          /** Enable/Disable Y curve control. */
    ST_LCCF_FACTOR stFactorTbl[ISO_LEVEL_MAX];  /** Four channel factor parameters table for ISO level control. */
} ST_CTL_LCCF_SETTING;

typedef struct _ST_CTL_LCCF_PARAM {
    ST_CTL_LCCF_SETTING stSetting;              /** Control LCCF setting file parameters. */
} ST_CTL_LCCF_PARAM;

typedef struct _ST_SAT_PNT {
    STF_U16 u16Threshold;                       /** Chroma magnitude amplification threshold, range from 0 to 2047. */
    STF_DOUBLE dFactor;                         /** Chroma magnitude amplification factor, range from 0 to 7.99609375. */
} ST_SAT_PNT;

typedef struct _ST_SAT_CRV {
    ST_SAT_PNT stPoint[2];                      /** Chroma magnitude amplification curve. */
} ST_SAT_CRV;

typedef struct _ST_CTL_SAT_SETTING {
    //-------------------------------------------------------------------------
    // This section parameters value is assign from setting file.
    STF_BOOL8 bEnable;                          /** Enable/Disable Y curve control. */
    ST_SAT_CRV stSatCurveTbl[ISO_LEVEL_MAX];    /** Chroma magnitude amplification curve table for ISO level control. */
} ST_CTL_SAT_SETTING;

typedef struct _ST_CTL_SAT_PARAM {
    ST_CTL_SAT_SETTING stSetting;               /** Control SAT setting file parameters. */
} ST_CTL_SAT_PARAM;

#define SHRP_CRV_PNT_MAX                        4

typedef struct _ST_SHRP_PNT {
    STF_U16 u16Diff;
    STF_DOUBLE dFactor;
} ST_SHRP_PNT;

typedef struct _ST_SHRP_CRV {
    ST_SHRP_PNT stYLevel[SHRP_CRV_PNT_MAX];
} ST_SHRP_CRV;

typedef struct _ST_CTL_SHRP_SETTING {
    //-------------------------------------------------------------------------
    // This section parameters value is assign from setting file.
    STF_BOOL8 bEnable;                          /** Enable/Disable Y curve control. */
    ST_SHRP_CRV stYLevelCrvTbl[ISO_LEVEL_MAX];  /** Y level curve for adaptive sharpness. */
} ST_CTL_SHRP_SETTING;

typedef struct _ST_CTL_SHRP_PARAM {
    ST_CTL_SHRP_SETTING stSetting;              /** Control SHRP setting file parameters. */
} ST_CTL_SHRP_PARAM;

#define YCRV_PNT                                64
#define YCRV_MAX_PNT                            (YCRV_PNT + 1)

typedef struct _ST_YHIST_PNT {
    STF_U8 u8Index;                             /** Bin index of Y histogram. */
    STF_U32 u32YHist;                           /** Backup the (YUV domain) Y histogram from statistical collection buffer. */
} ST_YHIST_PNT;

typedef struct _ST_YHIST_MIN_MAX_LMTD_W_PER {
    STF_U16 u16MaxLimit;                        /** Y histogram maximum limit value. */
    STF_U16 u16MinLimit;                        /** Y histogram minimum limit value. */
    STF_U16 u16StartLevel;                      /** Y histogram start level value. */
    STF_U16 u16EndLevel;                        /** Y histogram end level value. */
    STF_U8 u8StartPeriod;                       /** u8StartPeriod indicates how many bin will be limited to the start level value at the beginning of the histogram. */
    STF_U8 u8EndPeriod;                         /** u8EndPeriod indicates how many bin will be limited to the end level value at the end of the histogram. */
} ST_YHIST_MIN_MAX_LMTD_W_PER;

typedef struct _ST_YHIST_MIN_MAX_CRV_LMTD {
    STF_DOUBLE dMaxLimit;                     /** Y histogram maximum curve limit value. */
    STF_DOUBLE dMinLimit;                     /** Y histogram minimum curve limit value. */
    STF_DOUBLE dMaxStart;                     /** Y histogram maximum curve start value. */
    STF_DOUBLE dMinStart;                     /** Y histogram minimum curve start value. */
    STF_DOUBLE dMaxSlope;                     /** Y histogram maximum curve slope value. */
    STF_DOUBLE dMinSlope;                     /** Y histogram minimum curve slope value. */
} ST_YHIST_MIN_MAX_CRV_LMTD;

typedef struct _ST_CTL_YCRV_SETTING {
    //-------------------------------------------------------------------------
    // This section parameters value is assign from setting file.
    STF_BOOL8 bEnable;                          /** Enable/Disable Y curve control. */
    STF_U16 u16UpdateSpeed;                     /** Y curve update speed, range 0 (lowest) - 256 (fastest). */
    STF_U8 u8Method;                            /** Y curve dynamic generate method. 0:EN_YCRV_METHOD_MIN_MAX_LMTD_WITH_PERIOD, 1:EN_YCRV_METHOD_EQUALIZATION, 2:EN_YCRV_METHOD_MIN_CRV_MAX_LINE_LMTD, 3:EN_YCRV_METHOD_MIN_MAX_CRV_LMTD. */
    ST_YHIST_MIN_MAX_LMTD_W_PER stYHistMinMaxLmtdWithPeriod; /** Y histogram min-maxed control parameters. */
    ST_YHIST_MIN_MAX_CRV_LMTD stYHistMinMaxCrvLmtd; /** Y histogram min-maxed curve control parameters. */
} ST_CTL_YCRV_SETTING;

typedef struct _ST_POINT
{
    STF_U16 u16X;
    STF_U16 u16Y;
} ST_POINT;

typedef struct _ST_CTL_YCRV_INFO {
    ST_YHIST_PNT stYHist[YCRV_PNT];             /** Backup the (YUV domain) Y histogram from statistical collection buffer. */
    ST_POINT stYCurveWoDamping[YCRV_MAX_PNT];   /** Y curve, this is a global tone mapping curve that do not have apply the update speed factor. */
    ST_POINT stYCurve[YCRV_MAX_PNT];            /** Y curve, this is a global tone mapping curve that have apply the update speed factor. */
} ST_CTL_YCRV_INFO;

typedef struct _ST_CTL_YCRV_PARAM {
    ST_CTL_YCRV_SETTING stSetting;              /** Control YCRV setting file parameters. */
    //-------------------------------------------------------------------------
    // This section parameters is internal using.
    ST_CTL_YCRV_INFO stInfo;                    /** Control YCRV output information parameters. */
} ST_CTL_YCRV_PARAM;

typedef struct _ST_MOD_AWB_SETTING {
    //-------------------------------------------------------------------------
    // This section parameters value is assign from setting file.
    STF_BOOL8 bEnable;                          /** Enable/Disable AWB gain module. */
} ST_MOD_AWB_SETTING;

typedef struct _ST_MOD_AWB_INFO {
    STF_DOUBLE dRedGain;                        /** AWB R channel gain. */
    STF_DOUBLE dBlueGain;                       /** AWB B channel gain. */
    STF_DOUBLE dDigitalGain;                    /** Digital gain. */
} ST_MOD_AWB_INFO;

typedef struct _ST_AWB_PARAM {
    ST_MOD_AWB_SETTING stSetting;               /** Module AWB setting file parameters. */
    //-------------------------------------------------------------------------
    // This section parameters is internal using.
    ST_MOD_AWB_INFO stInfo;                     /** Module AWB output information parameters. */
} ST_AWB_PARAM;

typedef struct _ST_MOD_CAR_SETTING {
    //-------------------------------------------------------------------------
    // This section parameters value is assign from setting file.
    STF_BOOL8 bEnable;                          /** Enable/Disable CAR module. */
} ST_MOD_CAR_SETTING;

typedef struct _ST_CAR_PARAM {
    ST_MOD_CAR_SETTING stSetting;               /** Module CAR setting file parameters. */
} ST_CAR_PARAM;

typedef struct _ST_CCM_INFO {
    STF_DOUBLE dMatrix[3][3];                   /** Color correction matrix, range -3.9921875 - 3.9921875. */
    STF_S16 s16Offset[3];                       /** R, G and B channel offset, range -1024 - 1023. */
} ST_CCM_INFO;

typedef struct _ST_MATRIX_INFO {
    ST_CCM_INFO stSmLow;                        /** This CCM is low end color correction matrix and offsets. */
} ST_MATRIX_INFO;

typedef struct _ST_MOD_CCM_SETTING {
    //-------------------------------------------------------------------------
    // This section parameters value is assign from setting file.
    STF_BOOL8 bEnable;                          /** Enable/Disable CCM module. */
    ST_MATRIX_INFO stMatrixInfo;                /** Color matrix information. */
} ST_MOD_CCM_SETTING;

typedef struct _ST_CCM_PARAM {
    ST_MOD_CCM_SETTING stSetting;               /** Module CCM setting file parameters. */
} ST_CCM_PARAM;

typedef struct _ST_MOD_CFA_SETTING {
    //-------------------------------------------------------------------------
    // This section parameters value is assign from setting file.
    STF_BOOL8 bEnable;                          /** Enable/Disable CFA module. */
    STF_U8 u8HvWidth;                           /** HV_WIDTH = 2^u8HvWidth, range 0 - 15. */
    STF_U8 u8CrossCov;                          /** Cross covariance weighting, range 0 - 3. */
} ST_MOD_CFA_SETTING;

typedef struct _ST_CFA_PARAM {
    ST_MOD_CFA_SETTING stSetting;               /** Module CFA setting file parameters. */
} ST_CFA_PARAM;

typedef struct _ST_MOD_CTC_SETTING {
    //-------------------------------------------------------------------------
    // This section parameters value is assign from setting file.
    STF_BOOL8 bEnable;                          /** Enable/Disable CTC module. */
    STF_U8 u8SmoothAreaFilterMode;              /** CTC filter mode for smooth area. 0:EN_CTC_SMOOTH_AREA_CENTER_PIXEL, 1:EN_CTC_SMOOTH_AREA_5x5_NEIGHBORHOOD. */
    STF_U8 u8DetailAreaFilterMode;              /** CTC filter mode for detail area. 0:EN_CTC_DETAIL_AREA_AVERAGING_CORRECTION, 1:EN_CTC_DETAIL_AREA_5x5_NEIGHBORHOOD. */
    STF_U16 u16MaxGT;                           /** CTC filter maximum gr and gb channel threshold. */
    STF_U16 u16MinGT;                           /** CTC filter minimum gr and gb channel threshold. */
} ST_MOD_CTC_SETTING;

typedef struct _ST_CTC_PARAM {
    ST_MOD_CTC_SETTING stSetting;               /** Module CTC setting file parameters. */
} ST_CTC_PARAM;

typedef struct _ST_MOD_DBC_SETTING {
    //-------------------------------------------------------------------------
    // This section parameters value is assign from setting file.
    STF_BOOL8 bEnable;                          /** Enable/Disable DBC module. */
    STF_U16 u16BadGT;                           /** DBC filter bad pixel threshold for Gr and Gb channel. */
    STF_U16 u16BadXT;                           /** DBC filter bad pixel threshold for R and B channel. */
} ST_MOD_DBC_SETTING;

typedef struct _ST_DBC_PARAM {
    ST_MOD_DBC_SETTING stSetting;               /** Module DBC setting file parameters. */
} ST_DBC_PARAM;

#define DNYUV_WGHT_CNT_MAX                      10
#define DNYUV_CRV_PNT_MAX                       7

typedef struct _ST_MOD_DNYUV_SETTING {
    //-------------------------------------------------------------------------
    // This section parameters value is assign from setting file.
    STF_BOOL8 bEnable;                          /** Enable/Disable DNYUV module. */
    STF_U8 u8YSWeighting[DNYUV_WGHT_CNT_MAX];   /** Y channel (spacial) distance to center weighting value, range from 0 to 7. */
    ST_DNYUV_PNT stYCurve[DNYUV_CRV_PNT_MAX];   /** Y channel filter curve. */
    STF_U8 u8UvSWeighting[DNYUV_WGHT_CNT_MAX];  /** UV channel (spacial) distance to center weighting value, range from 0 to 7. */
    ST_DNYUV_PNT stUvCurve[DNYUV_CRV_PNT_MAX];  /** UV channel filter curve. */
} ST_MOD_DNYUV_SETTING;

typedef struct _ST_DNYUV_PARAM {
    ST_MOD_DNYUV_SETTING stSetting;             /** Module DNYUV setting file parameters. */
} ST_DNYUV_PARAM;

#define GMARGB_PNT              15

typedef struct _ST_MOD_GMARGB_SETTING {
    //-------------------------------------------------------------------------
    // This section parameters value is assign from setting file.
    STF_BOOL8 bEnable;                          /** Enable/Disable GMARGB module. */
    ST_POINT stCustomGammaCurve[GMARGB_PNT];    /** Customize gamma curve. */
} ST_MOD_GMARGB_SETTING;

typedef struct _ST_GMARGB_PARAM {
    ST_MOD_GMARGB_SETTING stSetting;            /** Module GMARGB setting file parameters. */
} ST_GMARGB_PARAM;

typedef struct _ST_MOD_LCCF_SETTING {
    //-------------------------------------------------------------------------
    // This section parameters value is assign from setting file.
    STF_BOOL8 bEnable;                          /** Enable/Disable LCCF module. */
    STF_U16 u16CenterX;                         /** X distance from capture window, 15bits. */
    STF_U16 u16CenterY;                         /** Y distance from capture window, 15bits. */
    STF_U8 u8Radius;                            /** Parameter for typical maximum distance (dM = 2^M), 4bits */
    STF_DOUBLE dRFactor[2];                     /** R channel factor parameters. */
    STF_DOUBLE dGrFactor[2];                    /** Gr channel factor parameters. */
    STF_DOUBLE dGbFactor[2];                    /** Gb channel factor parameters. */
    STF_DOUBLE dBFactor[2];                     /** R channel factor parameters. */
} ST_MOD_LCCF_SETTING;

typedef struct _ST_LCCF_PARAM {
    ST_MOD_LCCF_SETTING stSetting;              /** Module LCCF setting file parameters. */
} ST_LCCF_PARAM;

typedef struct _ST_OBC_GAIN {
    STF_FLOAT fTopLeft;
    STF_FLOAT fTopRight;
    STF_FLOAT fBottomLeft;
    STF_FLOAT fBottomRight;
} ST_OBC_GAIN;

typedef struct _ST_OBC_OFFSET {
    STF_U8 u8TopLeft;
    STF_U8 u8TopRight;
    STF_U8 u8BottomLeft;
    STF_U8 u8BottomRight;
} ST_OBC_OFFSET;

typedef struct _ST_MOD_OBC_SETTING {
    //-------------------------------------------------------------------------
    // This section parameters value is assign from setting file.
    STF_BOOL8 bEnable;                          /** Enable/Disable OBC module. */
    STF_U8 u8Width;                             /** Width. 2^u8Width */
    STF_U8 u8Height;                            /** Height. 2^u8Height */
    ST_OBC_GAIN stRGain;                        /** R channel gain. */
    ST_OBC_GAIN stGrGain;                       /** Gr channel gain. */
    ST_OBC_GAIN stGbGain;                       /** Gb channel gain. */
    ST_OBC_GAIN stBGain;                        /** B channel gain. */
    ST_OBC_OFFSET stROffset;                    /** R channel offset. */
    ST_OBC_OFFSET stGrOffset;                   /** Gr channel offset. */
    ST_OBC_OFFSET stGbOffset;                   /** Gb channel offset. */
    ST_OBC_OFFSET stBOffset;                    /** B channel offset. */
} ST_MOD_OBC_SETTING;

typedef struct _ST_OBC_PARAM {
    ST_MOD_OBC_SETTING stSetting;               /** Module OBC setting file parameters. */
} ST_OBC_PARAM;

#define OECF_CURVE_POINT_MAX                    16
#define EN_OECF_CHN_MAX                         4

typedef struct _ST_OECF_CRV {
    ST_POINT stCurve[OECF_CURVE_POINT_MAX];
} ST_OECF_CRV;

typedef struct _ST_MOD_OECF_SETTING {
    //-------------------------------------------------------------------------
    // This section parameters value is assign from setting file.
    STF_BOOL8 bEnable;                          /** Enable/Disable OECF module. */
    ST_OECF_CRV stOecfCrv[EN_OECF_CHN_MAX];     /** R, Gr, Gb and B channel input. */
} ST_MOD_OECF_SETTING;

typedef struct _ST_OECF_PARAM {
    ST_MOD_OECF_SETTING stSetting;              /** Module OECF setting file parameters. */
} ST_OECF_PARAM;

typedef struct _ST_MOD_R2Y_SETTING {
    //-------------------------------------------------------------------------
    // This section parameters value is assign from setting file.
    STF_BOOL8 bEnable;                          /** Enable/Disable R2Y module. */
    STF_DOUBLE dMatrix[3][3];                   /** RGB to YUV convert matrix. */
} ST_MOD_R2Y_SETTING;

typedef struct _ST_R2Y_PARAM {
    ST_MOD_R2Y_SETTING stSetting;               /** Module R2Y setting file parameters. */
} ST_R2Y_PARAM;

typedef struct _ST_YCRV_PNT {
    STF_U16 u16YInput;                          /** Y channel mapping curve input, range form 0 to 1023. */
    STF_U16 u16YOutput;                         /** Y channel mapping curve output, range form 0 to 1023. */
} ST_YCRV_PNT;

typedef struct _ST_HUE_INFO {
    STF_DOUBLE dDegree;                         /** Hue degree value, range form 0 to 90 degree. */
} ST_HUE_INFO;

typedef struct _ST_SAT_INFO {
    ST_SAT_CRV stSatCurve;                      /** Chroma magnitude amplification curve. */
    STF_U8 u8DeNrml;                            /** Chroma magnitude amplification slope scale down factor (2^-u8DNrm), range from 0 to 15. */
    STF_S16 s16UOffset;                         /** Chroma saturation U offset, range from -1024 to 1023. */
    STF_S16 s16VOffset;                         /** Chroma saturation U offset, range from -1024 to 1023. */
} ST_SAT_INFO;

typedef struct _ST_MOD_SAT_SETTING {
    //-------------------------------------------------------------------------
    // This section parameters value is assign from setting file.
    STF_BOOL8 bEnable;                          /** Enable/Disable SAT module. */
    ST_YCRV_PNT stYCurve[2];                    /** Brightness and contrast adjust parameters. */
    ST_HUE_INFO stHueInfo;                      /** Hue adjust parameters. */
    ST_SAT_INFO stSatInfo;                      /** Saturation adjust parameters. */
} ST_MOD_SAT_SETTING;

typedef struct _ST_SAT_PARAM {
    ST_MOD_SAT_SETTING stSetting;               /** Module SAT setting file parameters. */
} ST_SAT_PARAM;

typedef struct _ST_SC_AF_CFG {
    STF_U8 u8EsHorMode;                         /** Horizontal mode. */
    STF_U8 u8EsSumMode;                         /** Summation mode.  0:Absolute sum, 1:Squared sum. */
    STF_BOOL8 bHorEn;                            /** Horizontal enable. */
    STF_BOOL8 bVerEn;                            /** Vertical enable. */
    STF_U8 u8EsVerThr;                          /** Vertical threshold. */
    STF_U16 u16EsHorThr;                        /** Horizontal threshold. */
} ST_SC_AF_CFG;

typedef struct _ST_SC_AE_AF_PARAM {
    ST_SC_AF_CFG stAfCfg;                       /** AF configuration for SC. */
} ST_SC_AE_AF_PARAM;

typedef struct _ST_SC_AWB_WS_CFG {
    STF_U8 u8AwbWsRL;                           /** SC AWB weighting summation lower boundary of R value. */
    STF_U8 u8AwbWsRU;                           /** SC AWB weighting summation upper boundary of R value. */
    STF_U8 u8AwbWsGrL;                          /** SC AWB weighting summation lower boundary of Gr value. */
    STF_U8 u8AwbWsGrU;                          /** SC AWB weighting summation upper boundary of Gr value. */
    STF_U8 u8AwbWsGbL;                          /** SC AWB weighting summation lower boundary of Gb value. */
    STF_U8 u8AwbWsGbU;                          /** SC AWB weighting summation upper boundary of Gb value. */
    STF_U8 u8AwbWsBL;                           /** SC AWB weighting summation lower boundary of B value. */
    STF_U8 u8AwbWsBU;                           /** SC AWB weighting summation upper boundary of B value. */
} ST_SC_AWB_WS_CFG;

typedef struct _ST_AWB_IW_PNT {
    STF_U16 u16Intensity;                       /** SC AWB intensity value. Range from 0 to 256. */
    STF_U8 u8Weight;                            /** SC AWB intensity weight value. Range from 0 to 15. */
} ST_AWB_IW_PNT;

typedef struct _ST_SC_AWB_PARAM {
    ST_SC_AWB_WS_CFG stAwbWsCfg;                /** AWB weighting summation method configuration for SC. */
    STF_U8 u8AwbCW[13][13];                     /** SC AWB color weighting table, range from 0 to 15. */
    ST_AWB_IW_PNT stAwbIWCurve[17];             /** SC AWB intensity weighting curve, weight range from 0 to 15. */
} ST_SC_AWB_PARAM;

typedef struct _ST_SC_SETTING {
    //-------------------------------------------------------------------------
    // This section parameters value is assign from setting file.
    STF_BOOL8 bEnable;                          /** Enable/Disable SC AE/AF/AWB module. */
    ST_SC_AE_AF_PARAM stScAeAfParam;            /** AE and AF parameters for SC. */
    ST_SC_AWB_PARAM stScAwbParam;               /** AWB parameters for SC. */
} ST_SC_SETTING;

typedef struct _ST_SIZE
{
    STF_U16 u16Cx;
    STF_U16 u16Cy;
} ST_SIZE;

typedef struct _ST_SC_CROP_DEC {
    STF_U16 u16HStart;                          /** Horizontal starting point for SC cropping. Since hardware bug issue, H_Start cannot less than 4. */
    STF_U16 u16VStart;                          /** Vertical starting point for SC cropping. Since hardware bug issue, V_Start cannot less than 4. */
    STF_U8 u8SubWinWidth;                       /** Width of SC sub-window, u8SubWinWidth have to be odd number. (u8SubWinWidth + 1) */
    STF_U8 u8SubWinHeight;                      /** Height of SC sub-window, u8SubWinHeight have to be odd number. (u8SubWinHeight + 1) */
    STF_U8 u8HPeriod;                           /** Horizontal period(zero base) for input image decimation. */
    STF_U8 u8HKeep;                             /** Horizontal Keep(zero base) for input image decimation. */
    STF_U8 u8VPeriod;                           /** Vertical period(zero base) for input image decimation. */
    STF_U8 u8VKeep;                             /** Vertical Keep(zero base) for input image decimation. */
    ST_SIZE stDecSize;                          /** stDecSize is the size after SC decimation, it's only for check. */
} ST_SC_CROP_DEC;

typedef struct _ST_MOD_SC_INFO {
    ST_SIZE stImgSize;                          /** Image size. */
    ST_SC_CROP_DEC stCropDec;                   /** Input image cropping and decimation for SC.
                                                 * You don't need to set this structure
                                                 * value, it will be calculated and
                                                 * assigned by call
                                                 * STFMOD_ISP_SC_UpdateScDecimation()
                                                 * function.
                                                 */
} ST_MOD_SC_INFO;

typedef struct _ST_SC_PARAM {
    ST_SC_SETTING stSetting;                    /** Module SC setting file parameters. */
    //-------------------------------------------------------------------------
    // This section parameters is internal using.
    ST_MOD_SC_INFO stInfo;                      /** Module SC output information parameters. */
}ST_SC_PARAM;

typedef struct _ST_MOD_SHRP_SETTING {
    //-------------------------------------------------------------------------
    // This section parameters value is assign from setting file.
    STF_BOOL8 bEnable;                          /** Enable/Disable SHRP module. */
    STF_U8 u8Weighting[15];                     /** Sharpness weighting table. */
    ST_SHRP_PNT stYLevel[SHRP_CRV_PNT_MAX];     /** Y level curve. This is a strength curve base on the Y level different. */
    STF_DOUBLE dPosDirFactor;                   /** Position direction factor. */
    STF_DOUBLE dNegDirFactor;                   /** Negative direction factor. */
} ST_MOD_SHRP_SETTING;

typedef struct _ST_SHRP_PARAM {
    ST_MOD_SHRP_SETTING stSetting;              /** Module SHRP setting file parameters. */
} ST_SHRP_PARAM;

typedef struct _ST_MOD_YCRV_SETTING {
    //-------------------------------------------------------------------------
    // This section parameters value is assign from setting file.
    STF_BOOL8 bEnable;                          /** Enable/Disable YCRV module. */
    ST_POINT stYCurve[YCRV_MAX_PNT];            /** Y curve value. */
} ST_MOD_YCRV_SETTING;

typedef struct _ST_YCRV_PARAM {
    ST_MOD_YCRV_SETTING stSetting;              /** Module YCRV setting file parameters. */
} ST_YCRV_PARAM;

#pragma pack(pop)
