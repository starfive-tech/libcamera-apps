/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * Copyright (C) 2023, Starfive Technology Co., Ltd.
 *
 * starfive_controls.h - Starfive controls definition
 */

#pragma once

#include <libcamera/controls.h>
#include <libcamera/control_ids.h>
#include "starfiveIPAParams.h"

namespace libcamera 
{

namespace starfive
{

namespace control
{

struct GetModuleInformation {
	uint32_t controlID;
	uint32_t reqCookie;
	void *buffer;
};

enum {
    GET_INFORMATION = controls::TEST_PATTERN_MODE + 1024,
    AE_CTRL_INFOR,
    AWB_CTRL_INFOR,
    AWB_MOD_INFOR,
    CCM_CTRL_INFOR,
    CCM_MOD_INFOR,
    DNYUV_CTRL_INFOR,
    DNYUV_MOD_INFOR,
    LCCF_CTRL_INFOR,
    LCCF_MOD_INFOR,
    SAT_CTRL_INFOR,
    SAT_MOD_INFOR,
    SHRP_CTRL_INFOR,
    SHRP_MOD_INFOR,
    YCRV_CTRL_INFOR,
    YCRV_MOD_INFOR,
    CAR_MOD_INFOR,
    CFA_MOD_INFOR,
    CTC_MOD_INFOR,
    DBC_MOD_INFOR,
    GMARGB_MOD_INFOR,
    OBC_MOD_INFOR,
    OECF_MOD_INFOR,
    R2Y_MOD_INFOR,
    SC_MOD_INFOR,
    STARFIVE_MOD_INFOR_END,
};

extern const Control<Span<uint8_t, sizeof(GetModuleInformation)>> GetModuleInfo;
extern const Control<Span<uint8_t, sizeof(AGC::AECtrlParams)>> AECtrlInfo;
extern const Control<Span<uint8_t, sizeof(AWB::AWBCtrlParams)>> AWBCtrlInfo;
extern const Control<Span<uint8_t, sizeof(AWB::AWBModuleParams)>> AWBModInfo;
extern const Control<Span<uint8_t, sizeof(CCM::CCMCtrlParams)>> CCMCtrlInfo;
extern const Control<Span<uint8_t, sizeof(CCM::CCMModuleParams)>> CCMModInfo;
extern const Control<Span<uint8_t, sizeof(DNYUV::DNYUVCtrlParams)>> DNYUVCtrlInfo;
extern const Control<Span<uint8_t, sizeof(DNYUV::DNYUVModuleParams)>> DNYUVModInfo;
extern const Control<Span<uint8_t, sizeof(LCCF::LCCFCtrlParams)>> LCCFCtrlInfo;
extern const Control<Span<uint8_t, sizeof(LCCF::LCCFModuleParams)>> LCCFModInfo;
extern const Control<Span<uint8_t, sizeof(SAT::SATCtrlParams)>> SATCtrlInfo;
extern const Control<Span<uint8_t, sizeof(SAT::SATModuleParams)>> SATModInfo;
extern const Control<Span<uint8_t, sizeof(SHARPEN::SHARPENCtrlParams)>> SHRPCtrlInfo;
extern const Control<Span<uint8_t, sizeof(SHARPEN::SHARPENModuleParams)>> SHRPModInfo;
extern const Control<Span<uint8_t, sizeof(YCRV::YCRVCtrlParams)>> YCRVCtrlInfo;
extern const Control<Span<uint8_t, sizeof(YCRV::YCRVModuleParams)>> YCRVModInfo;
extern const Control<Span<uint8_t, sizeof(CAR::CARModuleParams)>> CARModInfo;
extern const Control<Span<uint8_t, sizeof(CFA::CFAModuleParams)>> CFAModInfo;
extern const Control<Span<uint8_t, sizeof(CTC::CTCModuleParams)>> CTCModInfo;
extern const Control<Span<uint8_t, sizeof(DBC::DBCModuleParams)>> DBCModInfo;
extern const Control<Span<uint8_t, sizeof(GAMMA::GAMMAModuleParams)>> GMARGBModInfo;
extern const Control<Span<uint8_t, sizeof(OBC::OBCModuleParams)>> OBCModInfo;
extern const Control<Span<uint8_t, sizeof(OECF::OECFModuleParams)>> OECFModInfo;
extern const Control<Span<uint8_t, sizeof(R2Y::R2YModuleParams)>> R2YModInfo;
extern const Control<Span<uint8_t, sizeof(STAT::STATModuleParams)>> SCModInfo;

extern const ControlIdMap starfiveControls;
extern const ControlInfoMap::Map starfiveControlMap;

} // namespace control

} // namespace starfive

} // namespace libcamera
