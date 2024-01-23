/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (C) 2023, Starfive Technology Co., Ltd.
 *
 * param_convert.cpp - parameters convertion between libcamera and tuning tool.
 */

#include <string.h>
#include <unordered_map>
#include <memory>

#include "param_convert.h"
#include "tuning_base.h"

const uint16_t YCRVCoordinateData::curveX_[65] = 
{
    0,   16,  32,  48,
    64,  80,  96,  112,
    128, 144, 160, 176,
    192, 208, 224, 240,
    256, 272, 288, 304,
    320, 336, 352, 368,
    384, 400, 416, 432,
    448, 464, 480, 496,
    512, 527, 543, 559,
    575, 591, 607, 623,
    639, 655, 671, 687,
    703, 719, 735, 751,
    767, 783, 799, 815,
    831, 847, 863, 879,
    895, 911, 927, 943,
    959, 975, 991, 1007,
    1023
};

const uint16_t GAMMAModParamConvertor::curveX_[15] = 
{
    0, 32, 64, 96, 128 ,160 ,192, 224, 256, 384, 512, 640, 768, 896, 1023
};

static AECtlParamConvertor aeCtlParamConvertor;
static AWBCtlParamConvertor awbCtlParamConvertor;
static CCMCtlParamConvertor ccmCtlParamConvertor;
static DNYUVCtlParamConvertor dnyuvCtlParamConvertor;
static LCCFCtlParamConvertor lccfCtlParamConvertor;
static SATCtlParamConvertor satCtlParamConvertor;
static SHARPENCtlParamConvertor sharpenCtlParamConvertor;
static YCRVCtlParamConvertor ycrvCtlParamConvertor;
static AWBModParamConvertor awbModParamConvertor;
static CARModParamConvertor carModParamConvertor;
static CCMModParamConvertor ccmModParamConvertor;
static CFAModParamConvertor cfaModParamConvertor;
static CTCModParamConvertor ctcModParamConvertor;
static DBCModParamConvertor dbcModParamConvertor;
static DNYUVModParamConvertor dnyuvModParamConvertor;
static GAMMAModParamConvertor gammaModParamConvertor;
static LCCFModParamConvertor lccfModParamConvertor;
static OBCModParamConvertor obcModParamConvertor;
static OECFModParamConvertor oecfModParamConvertor;
static R2YModParamConvertor r2yModParamConvertor;
static SATModParamConvertor satModParamConvertor;
static STATModParamConvertor statModParamConvertor;
static SHARPENModParamConvertor sharpenModParamConvertor;
static YCRVModParamConvertor ycrvModParamConvertor;

extern const std::unordered_map<uint32_t, ParamConvertorInterface *> parConvertors
{
    { EN_CONTROL_ID_AE, &aeCtlParamConvertor },
    { EN_CONTROL_ID_AWB, &awbCtlParamConvertor },
    { EN_CONTROL_ID_CCM, &ccmCtlParamConvertor },
    { EN_CONTROL_ID_DNYUV, &dnyuvCtlParamConvertor },
    { EN_CONTROL_ID_LCCF, &lccfCtlParamConvertor },
    { EN_CONTROL_ID_SAT, &satCtlParamConvertor },
    { EN_CONTROL_ID_SHRP, &sharpenCtlParamConvertor },
    { EN_CONTROL_ID_YCRV, &ycrvCtlParamConvertor },
    { EN_MODULE_ID_AWB, &awbModParamConvertor },
    { EN_MODULE_ID_CAR, &carModParamConvertor },
    { EN_MODULE_ID_CCM, &ccmModParamConvertor },
    { EN_MODULE_ID_CFA, &cfaModParamConvertor },
    { EN_MODULE_ID_CTC, &ctcModParamConvertor },
    { EN_MODULE_ID_DBC, &dbcModParamConvertor },
    { EN_MODULE_ID_DNYUV, &dnyuvModParamConvertor },
    { EN_MODULE_ID_GMARGB, &gammaModParamConvertor },
    { EN_MODULE_ID_LCCF, &lccfModParamConvertor },
    { EN_MODULE_ID_OBC, &obcModParamConvertor },
    { EN_MODULE_ID_OECF, &oecfModParamConvertor },
    { EN_MODULE_ID_R2Y, &r2yModParamConvertor },
    { EN_MODULE_ID_SAT, &satModParamConvertor },
    { EN_MODULE_ID_SC, &statModParamConvertor },
    { EN_MODULE_ID_SHRP, &sharpenModParamConvertor },
    { EN_MODULE_ID_YCRV, &ycrvModParamConvertor },
};

void resetParConvertors()
{
    for(auto it : parConvertors) {
        it.second->initDownParam_ = false;
        //printf("----mod: %u, dynamic: %u, addr: 0x%lx------\n", it.second->getModuleID(), it.second->isDynamic(), (uint64_t)it.second);
    }
}
