/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (C) 2023, Starfive Technology Co., Ltd.
 *
 * param_convert.h - parameters convertion between libcamera and tuning tool.
 */
#pragma once

#include "upside_params.h"
#include "starfiveIPAParams.h"
#include "starfive_controls.h"

class ParamConvertorInterface
{
public:
    ParamConvertorInterface(uint32_t moduleID) : moduleID_(moduleID), upBuffer_(nullptr) {}
    ParamConvertorInterface() = delete;
    ~ParamConvertorInterface() = default;

    virtual bool isDynamic() = 0;
    virtual void * getDownParam() = 0;
    virtual uint32_t downTypeSize() = 0;
    virtual uint32_t upTypeSize() = 0;
    virtual void toUpType(const void *src, void *dst) = 0;
    virtual void toDownType(const void *src, void *dst) = 0;

    uint32_t getModuleID() { return moduleID_; }
    void setUpBuffer(void *buf) { upBuffer_ = buf; }
    void *getUpBuffer() { return upBuffer_; }

public:
    bool initDownParam_ = false;

private:
    uint32_t moduleID_;
    void *upBuffer_;
};

template<typename downType, typename upType, bool dynamic = false>
class ParamConvertor : public ParamConvertorInterface
{
public:
    ParamConvertor(uint32_t moduleID) : ParamConvertorInterface(moduleID) {}
    ParamConvertor() = delete;

    virtual ~ParamConvertor() = default;

    bool isDynamic() override { return dynamic; };

    void * getDownParam() override { return &downParam_; };

    uint32_t downTypeSize() override { return sizeof(downType); };
    uint32_t upTypeSize() override { return sizeof(upType); };

    void toUpType(const void *src, void *dst) override 
    {
        const downType *in = (const downType *)src;
        upType *out = (upType *)dst;
        toUpTypeBody(in, out);
    }
    void toDownType(const void *src, void *dst) override 
    {
        const upType *in = (const upType *)src;
        downType *out = (downType *)dst;
        toDownTypeBody(in, out);
    }

    virtual void toUpTypeBody(const downType *in, upType *out) {
        const bool *inEnable = (const bool *)in;
        uint8_t *outEnable = (uint8_t *)out;
        *outEnable = *inEnable ? 1 : 0;

        uint32_t downSize = sizeof(downType) - sizeof(bool);
        uint32_t upSize = sizeof(upType) - 1;
        memcpy((uint8_t *)out + 1, (uint8_t *)in + sizeof(bool), 
            (downSize <= upSize) ? downSize : upSize);
    }

    virtual void toDownTypeBody(const upType *in, downType *out) {
        bool *outEnable = (bool *)out;
        const uint8_t *inEnable = (const uint8_t *)in;
        *outEnable = *inEnable ? true : false;

        uint32_t downSize = sizeof(downType) - sizeof(bool);
        uint32_t upSize = sizeof(upType) - 1;
        memcpy((uint8_t *)out + sizeof(bool), (uint8_t *)in + 1, 
            (downSize <= upSize) ? downSize : upSize);
    }

protected:
    downType downParam_;
};

class AECtlParamConvertor : public ParamConvertor<libcamera::starfive::AGC::AECtrlParams, ST_CTL_AE_PARAM, true>
{
public:
    AECtlParamConvertor()
    : ParamConvertor<libcamera::starfive::AGC::AECtrlParams, ST_CTL_AE_PARAM, true>(libcamera::starfive::control::AE_CTRL_INFOR)
    {}

    void toUpTypeBody(const libcamera::starfive::AGC::AECtrlParams *in, ST_CTL_AE_PARAM *out) override
    {
        out->stSetting.bEnable = in->ctlEnable ? 1 : 0;
        memcpy(&out->stSetting.u8TargetBrightness, &in->targetBrightness, sizeof(ST_CTL_AE_SETTING));
        memcpy(&out->stInfo, &(in->expectSnrExpo), sizeof(ST_CTL_AE_INFO));
    }
    void toDownTypeBody(const ST_CTL_AE_PARAM *in, libcamera::starfive::AGC::AECtrlParams *out) override
    {
        out->ctlEnable = in->stSetting.bEnable ? true : false;
        memcpy(&out->targetBrightness, &in->stSetting.u8TargetBrightness, sizeof(ST_CTL_AE_SETTING));
        memcpy(&(out->expectSnrExpo), &in->stInfo, sizeof(ST_CTL_AE_INFO));
    }
};

class AWBCtlParamConvertor : public ParamConvertor<libcamera::starfive::AWB::AWBCtrlParams, ST_CTL_AWB_PARAM, true>
{
public:
    AWBCtlParamConvertor()
    : ParamConvertor<libcamera::starfive::AWB::AWBCtrlParams, ST_CTL_AWB_PARAM, true>(libcamera::starfive::control::AWB_CTRL_INFOR)
    {}

    void toUpTypeBody(const libcamera::starfive::AWB::AWBCtrlParams *in, ST_CTL_AWB_PARAM *out) override
    {
        out->stSetting.bEnable = in->ctlEnable ? 1 : 0;
        out->stSetting.dUpdateSpeed = in->updateSpeed;
        out->stSetting.stAwbTempTbl.u8AwbTempTblCnt = in->tempGainNum;
        memcpy(out->stSetting.stAwbTempTbl.stAwbTemp, in->temperatureTable, sizeof(out->stSetting.stAwbTempTbl.stAwbTemp));
        memcpy(&out->stInfo, &(in->targetRedGain), sizeof(ST_CTL_AWB_INFO));
    }
    void toDownTypeBody(const ST_CTL_AWB_PARAM *in, libcamera::starfive::AWB::AWBCtrlParams *out) override
    {
        out->ctlEnable = in->stSetting.bEnable ? true : false;
        out->updateSpeed = in->stSetting.dUpdateSpeed;
        out->tempGainNum = in->stSetting.stAwbTempTbl.u8AwbTempTblCnt;
        memcpy(out->temperatureTable, in->stSetting.stAwbTempTbl.stAwbTemp, sizeof(in->stSetting.stAwbTempTbl.stAwbTemp));
    }
};

class CCMCtlParamConvertor : public ParamConvertor<libcamera::starfive::CCM::CCMCtrlParams, ST_CTL_CCM_PARAM>
{
public:
    CCMCtlParamConvertor()
    : ParamConvertor<libcamera::starfive::CCM::CCMCtrlParams, ST_CTL_CCM_PARAM>(libcamera::starfive::control::CCM_CTRL_INFOR)
    {}

    void toUpTypeBody(const libcamera::starfive::CCM::CCMCtrlParams *in, ST_CTL_CCM_PARAM *out) override
    {
        out->stSetting.bEnable = in->ctlEnable ? 1 : 0;
        out->stSetting.stCcmTbl.u8CcmTblCnt = in->tempGainNum;
        for(int i = 0; i < 6; i++)
            memcpy(&out->stSetting.stCcmTbl.stTempCcm[i], in->ccmTable[i].matrix, sizeof(ST_TEMP_CCM));
    }
    void toDownTypeBody(const ST_CTL_CCM_PARAM *in, libcamera::starfive::CCM::CCMCtrlParams *out) override
    {
        out->ctlEnable = in->stSetting.bEnable ? true : false;
        out->tempGainNum = in->stSetting.stCcmTbl.u8CcmTblCnt;
        for(int i = 0; i < 6; i++)
            memcpy(out->ccmTable[i].matrix, &in->stSetting.stCcmTbl.stTempCcm[i], sizeof(ST_TEMP_CCM));
    }
};

class DNYUVCtlParamConvertor : public ParamConvertor<libcamera::starfive::DNYUV::DNYUVCtrlParams, ST_CTL_DNYUV_PARAM>
{
public:
    DNYUVCtlParamConvertor()
    : ParamConvertor<libcamera::starfive::DNYUV::DNYUVCtrlParams, ST_CTL_DNYUV_PARAM>(libcamera::starfive::control::DNYUV_CTRL_INFOR)
    {}

    void toUpTypeBody(const libcamera::starfive::DNYUV::DNYUVCtrlParams *in, ST_CTL_DNYUV_PARAM *out) override
    {
        out->stSetting.bEnable = in->ctlEnable ? 1 : 0;
        for(int i = 0; i < 10; i++)
            memcpy(&out->stSetting.stDnYuvCurveTbl[i], in->curveTable[i].yCurve, sizeof(ST_DNYUV_CRV));
    }
    void toDownTypeBody(const ST_CTL_DNYUV_PARAM *in, libcamera::starfive::DNYUV::DNYUVCtrlParams *out) override
    {
        out->ctlEnable = in->stSetting.bEnable ? true : false;
        for(int i = 0; i < 10; i++)
            memcpy(out->curveTable[i].yCurve, &in->stSetting.stDnYuvCurveTbl[i], sizeof(ST_DNYUV_CRV));
    }
};

class LCCFCtlParamConvertor : public ParamConvertor<libcamera::starfive::LCCF::LCCFCtrlParams, ST_CTL_LCCF_PARAM>
{
public:
    LCCFCtlParamConvertor()
    : ParamConvertor<libcamera::starfive::LCCF::LCCFCtrlParams, ST_CTL_LCCF_PARAM>(libcamera::starfive::control::LCCF_CTRL_INFOR)
    {}

    void toUpTypeBody(const libcamera::starfive::LCCF::LCCFCtrlParams *in, ST_CTL_LCCF_PARAM *out) override
    {
        out->stSetting.bEnable = in->ctlEnable ? 1 : 0;
        for(int i = 0; i < 10; i++)
            memcpy(&out->stSetting.stFactorTbl[i], &in->table[i].rF1, sizeof(ST_LCCF_FACTOR));
    }
    void toDownTypeBody(const ST_CTL_LCCF_PARAM *in, libcamera::starfive::LCCF::LCCFCtrlParams *out) override
    {
        out->ctlEnable = in->stSetting.bEnable ? true : false;
        for(int i = 0; i < 10; i++)
            memcpy(&out->table[i].rF1, &in->stSetting.stFactorTbl[i], sizeof(ST_LCCF_FACTOR));
    }
};

class SATCtlParamConvertor : public ParamConvertor<libcamera::starfive::SAT::SATCtrlParams, ST_CTL_SAT_PARAM>
{
public:
    SATCtlParamConvertor()
    : ParamConvertor<libcamera::starfive::SAT::SATCtrlParams, ST_CTL_SAT_PARAM>(libcamera::starfive::control::SAT_CTRL_INFOR)
    {}

    void toUpTypeBody(const libcamera::starfive::SAT::SATCtrlParams *in, ST_CTL_SAT_PARAM *out) override
    {
        out->stSetting.bEnable = in->ctlEnable ? 1 : 0;
        for(int i = 0; i < 10; i++)
            memcpy(&out->stSetting.stSatCurveTbl[i], in->isoCurves[i].pt, sizeof(ST_SAT_CRV));
    }
    void toDownTypeBody(const ST_CTL_SAT_PARAM *in, libcamera::starfive::SAT::SATCtrlParams *out) override
    {
        out->ctlEnable = in->stSetting.bEnable ? true : false;
        for(int i = 0; i < 10; i++)
            memcpy(out->isoCurves[i].pt, &in->stSetting.stSatCurveTbl[i], sizeof(ST_SAT_CRV));
    }
};

class SHARPENCtlParamConvertor : public ParamConvertor<libcamera::starfive::SHARPEN::SHARPENCtrlParams, ST_CTL_SHRP_PARAM>
{
public:
    SHARPENCtlParamConvertor()
    : ParamConvertor<libcamera::starfive::SHARPEN::SHARPENCtrlParams, ST_CTL_SHRP_PARAM>(libcamera::starfive::control::SHRP_CTRL_INFOR)
    {}

    void toUpTypeBody(const libcamera::starfive::SHARPEN::SHARPENCtrlParams *in, ST_CTL_SHRP_PARAM *out) override
    {
        out->stSetting.bEnable = in->ctlEnable ? 1 : 0;
        for(int i = 0; i < 10; i++)
            memcpy(&out->stSetting.stYLevelCrvTbl[i], in->isoCurves[i].pt, sizeof(ST_SHRP_CRV));
    }
    void toDownTypeBody(const ST_CTL_SHRP_PARAM *in, libcamera::starfive::SHARPEN::SHARPENCtrlParams *out) override
    {
        out->ctlEnable = in->stSetting.bEnable ? true : false;
        for(int i = 0; i < 10; i++)
            memcpy(out->isoCurves[i].pt, &in->stSetting.stYLevelCrvTbl[i], sizeof(ST_SHRP_CRV));
    }
};

struct YCRVCoordinateData
{
    static const uint16_t curveX_[65];
};

class YCRVCtlParamConvertor : public ParamConvertor<libcamera::starfive::YCRV::YCRVCtrlParams, ST_CTL_YCRV_PARAM, true>, private YCRVCoordinateData
{
public:
    YCRVCtlParamConvertor()
    : ParamConvertor<libcamera::starfive::YCRV::YCRVCtrlParams, ST_CTL_YCRV_PARAM, true>(libcamera::starfive::control::YCRV_CTRL_INFOR)
    {}

    void toUpTypeBody(const libcamera::starfive::YCRV::YCRVCtrlParams *in, ST_CTL_YCRV_PARAM *out) override
    {
        out->stSetting.bEnable = in->ctlEnable ? 1 : 0;
        memcpy(&out->stSetting.u16UpdateSpeed, &in->updateSpeed, sizeof(ST_CTL_YCRV_SETTING));
        for(int i = 0; i < 64; i++) {
            out->stInfo.stYHist[i].u8Index = i;
            out->stInfo.stYHist[i].u32YHist = in->yHist[i];
        }
        for(int i = 0; i < 65; i++) {
            out->stInfo.stYCurveWoDamping[i].u16X = curveX_[i];
            out->stInfo.stYCurveWoDamping[i].u16Y = in->woDamping[i];
            out->stInfo.stYCurve[i].u16X = curveX_[i];
            out->stInfo.stYCurve[i].u16Y = in->yCurve[i];
        }
    }
    void toDownTypeBody(const ST_CTL_YCRV_PARAM *in, libcamera::starfive::YCRV::YCRVCtrlParams *out) override
    {
        out->ctlEnable = in->stSetting.bEnable ? true : false;
        memcpy(&out->updateSpeed, &in->stSetting.u16UpdateSpeed, sizeof(ST_CTL_YCRV_SETTING));
    }
};

#define CONVERT_CLASS_STATIC(down, up, module)  \
class down##ModParamConvertor : public ParamConvertor<libcamera::starfive::down::down##ModuleParams, ST_##up##_PARAM>   \
{   \
public: \
    down##ModParamConvertor()   \
    : ParamConvertor<libcamera::starfive::down::down##ModuleParams, ST_##up##_PARAM>(libcamera::starfive::control::module)  \
    {}  \
}

#define CONVERT_CLASS_DYNAMIC(down, up, module)  \
class down##ModParamConvertor : public ParamConvertor<libcamera::starfive::down::down##ModuleParams, ST_##up##_PARAM, true>   \
{   \
public: \
    down##ModParamConvertor()   \
    : ParamConvertor<libcamera::starfive::down::down##ModuleParams, ST_##up##_PARAM, true>(libcamera::starfive::control::module)    \
    {}  \
}

CONVERT_CLASS_DYNAMIC(AWB, AWB, AWB_MOD_INFOR);

CONVERT_CLASS_STATIC(CAR, CAR, CAR_MOD_INFOR);

class CCMModParamConvertor : public ParamConvertor<libcamera::starfive::CCM::CCMModuleParams, ST_CCM_PARAM, true>
{
public:
    CCMModParamConvertor()
    : ParamConvertor<libcamera::starfive::CCM::CCMModuleParams, ST_CCM_PARAM, true>(libcamera::starfive::control::CCM_MOD_INFOR)
    {}

    void toUpTypeBody(const libcamera::starfive::CCM::CCMModuleParams *in, ST_CCM_PARAM *out) override
    {
        out->stSetting.bEnable = in->moduleEnable ? 1 : 0;
        memcpy(&out->stSetting.stMatrixInfo.stSmLow, in->matrix.matrix, sizeof(ST_MATRIX_INFO));
    }
    void toDownTypeBody(const ST_CCM_PARAM *in, libcamera::starfive::CCM::CCMModuleParams *out) override
    {
        out->moduleEnable = in->stSetting.bEnable ? true : false;
        memcpy(out->matrix.matrix, &in->stSetting.stMatrixInfo.stSmLow, sizeof(ST_MATRIX_INFO));
    }
};

CONVERT_CLASS_STATIC(CFA, CFA, CFA_MOD_INFOR);
CONVERT_CLASS_STATIC(CTC, CTC, CTC_MOD_INFOR);
CONVERT_CLASS_STATIC(DBC, DBC, DBC_MOD_INFOR);

class DNYUVModParamConvertor : public ParamConvertor<libcamera::starfive::DNYUV::DNYUVModuleParams, ST_DNYUV_PARAM, true>
{
public:
    DNYUVModParamConvertor()
    : ParamConvertor<libcamera::starfive::DNYUV::DNYUVModuleParams, ST_DNYUV_PARAM, true>(libcamera::starfive::control::DNYUV_MOD_INFOR)
    {}

    void toUpTypeBody(const libcamera::starfive::DNYUV::DNYUVModuleParams *in, ST_DNYUV_PARAM *out) override
    {
        out->stSetting.bEnable = in->moduleEnable ? 1 : 0;
        memcpy(out->stSetting.u8YSWeighting, in->ysWeight, sizeof(in->ysWeight));
        memcpy(out->stSetting.u8UvSWeighting, in->uvsWeight, sizeof(in->uvsWeight));
        memcpy(out->stSetting.stYCurve, in->yCurve, sizeof(in->yCurve));
        memcpy(out->stSetting.stUvCurve, in->uvCurve, sizeof(in->uvCurve));
    }
    void toDownTypeBody(const ST_DNYUV_PARAM *in, libcamera::starfive::DNYUV::DNYUVModuleParams *out) override
    {
        out->moduleEnable = in->stSetting.bEnable ? true : false;
        memcpy(out->ysWeight, in->stSetting.u8YSWeighting, sizeof(out->ysWeight));
        memcpy(out->uvsWeight, in->stSetting.u8UvSWeighting, sizeof(out->uvsWeight));
        memcpy(out->yCurve, in->stSetting.stYCurve, sizeof(out->yCurve));
        memcpy(out->uvCurve, in->stSetting.stUvCurve, sizeof(out->uvCurve));
    }
};

class GAMMAModParamConvertor : public ParamConvertor<libcamera::starfive::GAMMA::GAMMAModuleParams, ST_GMARGB_PARAM>
{
public:
    GAMMAModParamConvertor()
    : ParamConvertor<libcamera::starfive::GAMMA::GAMMAModuleParams, ST_GMARGB_PARAM>(libcamera::starfive::control::GMARGB_MOD_INFOR)
    {}

    void toUpTypeBody(const libcamera::starfive::GAMMA::GAMMAModuleParams *in, ST_GMARGB_PARAM *out) override
    {
        out->stSetting.bEnable = in->moduleEnable ? 1 : 0;
        for(int i = 0; i < 15; i++) {
            out->stSetting.stCustomGammaCurve[i].u16X = curveX_[i];
            out->stSetting.stCustomGammaCurve[i].u16Y = in->gammaCurve[i];
        }
    }
    void toDownTypeBody(const ST_GMARGB_PARAM *in, libcamera::starfive::GAMMA::GAMMAModuleParams *out) override
    {
        out->moduleEnable = in->stSetting.bEnable ? true : false;
        for(int i = 0; i < 15; i++)
            out->gammaCurve[i] = in->stSetting.stCustomGammaCurve[i].u16Y;
    }

private:
    static const uint16_t curveX_[15];
};

class LCCFModParamConvertor : public ParamConvertor<libcamera::starfive::LCCF::LCCFModuleParams, ST_LCCF_PARAM, true>
{
public:
    LCCFModParamConvertor()
    : ParamConvertor<libcamera::starfive::LCCF::LCCFModuleParams, ST_LCCF_PARAM, true>(libcamera::starfive::control::LCCF_MOD_INFOR)
    {}

    void toUpTypeBody(const libcamera::starfive::LCCF::LCCFModuleParams *in, ST_LCCF_PARAM *out) override
    {
        out->stSetting.bEnable = in->moduleEnable ? 1 : 0;
        out->stSetting.u16CenterX = in->centerX;
        out->stSetting.u16CenterY = in->centerY;
        out->stSetting.u8Radius = in->radius;
        memcpy(out->stSetting.dRFactor, &in->curFactor.rF1, sizeof(double) * 8);
    }
    void toDownTypeBody(const ST_LCCF_PARAM *in, libcamera::starfive::LCCF::LCCFModuleParams *out) override
    {
        out->moduleEnable = in->stSetting.bEnable ? true : false;
        out->centerX = in->stSetting.u16CenterX;
        out->centerY = in->stSetting.u16CenterY;
        out->radius = in->stSetting.u8Radius;
        memcpy(&out->curFactor.rF1, in->stSetting.dRFactor, sizeof(double) * 8);
    }
};

class OBCModParamConvertor : public ParamConvertor<libcamera::starfive::OBC::OBCModuleParams, ST_OBC_PARAM>
{
public:
    OBCModParamConvertor()
    : ParamConvertor<libcamera::starfive::OBC::OBCModuleParams, ST_OBC_PARAM>(libcamera::starfive::control::OBC_MOD_INFOR)
    {}

    void toUpTypeBody(const libcamera::starfive::OBC::OBCModuleParams *in, ST_OBC_PARAM *out) override
    {
        const double *srcGain = (const double *)in->gains;
        float *dstGain = (float *)&out->stSetting.stRGain;
        out->stSetting.bEnable = in->moduleEnable ? 1 : 0;
        out->stSetting.u8Width = in->winWidth;
        out->stSetting.u8Height = in->winHeight;
        for(int i = 0; i < 16; i++)
            dstGain[i] = srcGain[i];
        memcpy(&out->stSetting.stROffset, in->offsets, sizeof(in->offsets));
    }
    void toDownTypeBody(const ST_OBC_PARAM *in, libcamera::starfive::OBC::OBCModuleParams *out) override
    {
        const float *srcGain = (const float *)&in->stSetting.stRGain;
        double *dstGain = (double *)out->gains;
        out->moduleEnable = in->stSetting.bEnable ? true : false;
        out->winWidth = in->stSetting.u8Width;
        out->winHeight = in->stSetting.u8Height;
        for(int i = 0; i < 16; i++)
            dstGain[i] = srcGain[i];
        memcpy(out->offsets, &in->stSetting.stROffset, sizeof(out->offsets));
    }
};

CONVERT_CLASS_STATIC(OECF, OECF, OECF_MOD_INFOR);
CONVERT_CLASS_STATIC(R2Y, R2Y, R2Y_MOD_INFOR);
CONVERT_CLASS_DYNAMIC(SAT, SAT, SAT_MOD_INFOR);
CONVERT_CLASS_STATIC(STAT, SC, SC_MOD_INFOR);

class SHARPENModParamConvertor : public ParamConvertor<libcamera::starfive::SHARPEN::SHARPENModuleParams, ST_SHRP_PARAM, true>
{
public:
    SHARPENModParamConvertor()
    : ParamConvertor<libcamera::starfive::SHARPEN::SHARPENModuleParams, ST_SHRP_PARAM, true>(libcamera::starfive::control::SHRP_MOD_INFOR)
    {}

    void toUpTypeBody(const libcamera::starfive::SHARPEN::SHARPENModuleParams *in, ST_SHRP_PARAM *out) override
    {
        out->stSetting.bEnable = in->moduleEnable ? 1 : 0;
        out->stSetting.dPosDirFactor = in->posFactor;
        out->stSetting.dNegDirFactor = in->negFactor;
        memcpy(out->stSetting.u8Weighting, in->weight, sizeof(in->weight));
        memcpy(out->stSetting.stYLevel, &in->yCurve, sizeof(in->yCurve));
    }
    void toDownTypeBody(const ST_SHRP_PARAM *in, libcamera::starfive::SHARPEN::SHARPENModuleParams *out) override
    {
        out->moduleEnable = in->stSetting.bEnable ? true : false;
        out->posFactor = in->stSetting.dPosDirFactor;
        out->negFactor = in->stSetting.dNegDirFactor;
        memcpy(out->weight, in->stSetting.u8Weighting, sizeof(out->weight));
        memcpy(&out->yCurve, in->stSetting.stYLevel, sizeof(out->yCurve));
    }
};

class YCRVModParamConvertor : public ParamConvertor<libcamera::starfive::YCRV::YCRVModuleParams, ST_YCRV_PARAM, true>, private YCRVCoordinateData
{
public:
    YCRVModParamConvertor()
    : ParamConvertor<libcamera::starfive::YCRV::YCRVModuleParams, ST_YCRV_PARAM, true>(libcamera::starfive::control::YCRV_MOD_INFOR)
    {}

    void toUpTypeBody(const libcamera::starfive::YCRV::YCRVModuleParams *in, ST_YCRV_PARAM *out) override
    {
        out->stSetting.bEnable = in->moduleEnable ? 1 : 0;
        for(int i = 0; i < 65; i++) {
            out->stSetting.stYCurve[i].u16X = curveX_[i];
            out->stSetting.stYCurve[i].u16Y = in->yCurve[i];
        }
    }
    void toDownTypeBody(const ST_YCRV_PARAM *in, libcamera::starfive::YCRV::YCRVModuleParams *out) override
    {
        out->moduleEnable = in->stSetting.bEnable ? true : false;
        for(int i = 0; i < 65; i++)
            out->yCurve[i] = in->stSetting.stYCurve[i].u16Y;
    }
};

extern const std::unordered_map<uint32_t, ParamConvertorInterface *> parConvertors;

void resetParConvertors();
