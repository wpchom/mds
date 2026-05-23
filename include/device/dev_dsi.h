/**
 * Copyright (c) [2022] [pchom]
 * [MDS] is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *          http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 **/
#ifndef __DEV_DSI_H__
#define __DEV_DSI_H__

/* Include ----------------------------------------------------------------- */
#include "mds_dev.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Typedef ----------------------------------------------------------------- */
enum DEV_DSI_Cmd {
    DEV_DSI_CMD_PHY_INIT = MDS_DEVICE_CMD_DRIVER,
    DEV_DSI_CMD_ENTER_ULPM,
    DEV_DSI_CMD_EXIT_ULPM,
    DEV_DSI_CMD_ENTER_ULPM_DATA,
    DEV_DSI_CMD_EXIT_ULPM_DATA,
    DEV_DSI_CMD_REFRESH,
};

typedef enum DEV_DSI_DataLane {
    DEV_DSI_DATALANE_1 = 1,
    DEV_DSI_DATALANE_2 = 2,
    DEV_DSI_DATALANE_4 = 4,
} __attribute__((packed)) DEV_DSI_DataLane_t;

typedef enum DEV_DSI_AutoClkLaneCtrl {
    DEV_DSI_AUTO_CLK_LANE_CTRL_DISABLE,
    DEV_DSI_AUTO_CLK_LANE_CTRL_ENABLE,
} __attribute__((packed)) DEV_DSI_AutoClkLaneCtrl_t;

typedef struct DEV_DSI_PhyTiming {
    uint16_t nsClkHs2Lp;
    uint16_t nsClkLp2Hs;
    uint16_t nsDatHs2Lp;
    uint16_t nsDatLp2Hs;
    uint32_t nsMaxReadTime;
    uint32_t nsStopWait;
} DEV_DSI_PhyTiming_t;

typedef struct DEV_DSI_PhyConfig {
    uint32_t bitRateHz;
    DEV_DSI_DataLane_t dataLane;
    DEV_DSI_AutoClkLaneCtrl_t autoClkLaneCtrl;
    DEV_DSI_PhyTiming_t phyTiming;
} DEV_DSI_PhyConfig_t;

typedef enum DEV_DSI_WorkMode {
    DEV_DSI_WORKMODE_VIDEO,   // Video mode
    DEV_DSI_WORKMODE_COMMAND, // Command mode
} __attribute__((packed)) DEV_DSI_WorkMode_t;

typedef enum DEV_DSI_ColorCoding {
    DEV_DSI_COLOR_CODING_16,
    DEV_DSI_COLOR_CODING_18_PACKED,
    DEV_DSI_COLOR_CODING_18_LOOSELY,
    DEV_DSI_COLOR_CODING_24,
} __attribute__((packed)) DEV_DSI_ColorCoding_t;

typedef enum DEV_DSI_VideoMode {
    DEV_DSI_VIDEO_MODE_BURST,
    DEV_DSI_VIDEO_MODE_NON_BURST_EVENTS,
    DEV_DSI_VIDEO_MODE_NON_BURST_PULSES,
} __attribute__((packed)) DEV_DSI_VideoMode_t;

typedef enum DEV_DSI_ActivePolarity {
    DEV_DSI_ACTIVE_POLAR_LOW = 0,
    DEV_DSI_ACTIVE_POLAR_HIGH = 1,
} __attribute__((packed)) DEV_DSI_ActivePolarity_t;

typedef enum DEV_DSI_EdgePolarity {
    DEV_DSI_EDGE_POLAR_FALLING = 0,
    DEV_DSI_EDGE_POLAR_RISING = 1,
} __attribute__((packed)) DEV_DSI_EdgePolarity_t;

enum DEV_DSI_VideoLowPower {
    DEV_DSI_VIDEO_LP_CE = 0x01U,
    DEV_DSI_VIDEO_LP_HFP = 0x02U,
    DEV_DSI_VIDEO_LP_HBP = 0x04U,
    DEV_DSI_VIDEO_LP_VACT = 0x08U,
    DEV_DSI_VIDEO_LP_VFP = 0x10U,
    DEV_DSI_VIDEO_LP_VBP = 0x20U,
    DEV_DSI_VIDEO_LP_VSA = 0x40U,
};

enum DEV_DSI_LpCmdConfig {
    DEV_DSI_LP_DCS_SHORT_READ_P0 = 0x0001U,
    DEV_DSI_LP_DCS_SHORT_WRITE_P0 = 0x0002U,
    DEV_DSI_LP_DCS_SHORT_WRITE_P1 = 0x0004U,
    DEV_DSI_LP_DCS_LONG_WRITE = 0x0008U,

    DEV_DSI_LP_GEN_SHORT_READ_P0 = 0x0010U,
    DEV_DSI_LP_GEN_SHORT_READ_P1 = 0x0020U,
    DEV_DSI_LP_GEN_SHORT_READ_P2 = 0x0040U,
    DEV_DSI_LP_GEN_SHORT_WRITE_P0 = 0x0100U,
    DEV_DSI_LP_GEN_SHORT_WRITE_P1 = 0x0200U,
    DEV_DSI_LP_GEN_SHORT_WRITE_P2 = 0x0400U,
    DEV_DSI_LP_GEN_LONG_WRITE = 0x0800U,

    DEV_DSI_LP_MAX_READPKT = 0x4000U,
    DEV_DSI_LP_ACK_REQUEST = 0x8000U,
};

typedef enum DEV_DSI_DataType {
    DEV_DSI_DCS_SHORT_PKT_READ_P0 = 0x06,  // DCS Short Read, 0 Param
    DEV_DSI_DCS_SHORT_PKT_READ_P1 = 0x16,  // DCS Short Read, 1 Param
    DEV_DSI_DCS_SHORT_PKT_WRITE_P0 = 0x05, // DCS Short Write, 0 Param
    DEV_DSI_DCS_SHORT_PKT_WRITE_P1 = 0x15, // DCS Short Write, 1 Param
    DEV_DSI_DCS_LONG_PKT_WRITE = 0x39,     // DCS Long Write

    DEV_DSI_GEN_SHORT_PKT_READ_P0 = 0x04,  // Generic Read, 0 Param
    DEV_DSI_GEN_SHORT_PKT_READ_P1 = 0x14,  // Generic Read, 1 Param
    DEV_DSI_GEN_SHORT_PKT_READ_P2 = 0x24,  // Generic Read, 2 Param
    DEV_DSI_GEN_SHORT_PKT_WRITE_P0 = 0x03, // Generic Short Write, 0 Param
    DEV_DSI_GEN_SHORT_PKT_WRITE_P1 = 0x13, // Generic Short Write, 1 Param
    DEV_DSI_GEN_SHORT_PKT_WRITE_P2 = 0x23, // Generic Short Write, 2 Param
    DEV_DSI_GEN_LONG_PKT_WRITE = 0x29,     // Generic Long Write
} __attribute__((packed)) DEV_DSI_DataType_t;

typedef struct DEV_DSI_VideoConfig {
    DEV_DSI_ColorCoding_t colorCoding;

    DEV_DSI_ActivePolarity_t hsyncActPolar;
    DEV_DSI_ActivePolarity_t vsyncActPolar;
    DEV_DSI_ActivePolarity_t datEnActPolar;

    DEV_DSI_VideoMode_t mode;

    uint32_t pktSize;
    uint32_t numChunk;
    uint32_t nullpkts;

    uint32_t hsa, hbp, hact, hfp;
    uint32_t vsa, vbp, vact, vfp;
    MDS_Mask_t lpFlags;
    uint32_t lpLargestPkts;
    uint32_t lpVactLargestPkts;
} DEV_DSI_VideoConfig_t;

typedef enum DEV_DSI_TeSource {
    DEV_DSI_TESOURCE_DSILINK,
    DEV_DSI_TESOURCE_EXTERNAL,
} __attribute__((packed)) DEV_DSI_TeSource_t;

typedef enum DEV_DSI_AutoRefresh {
    DEV_DSI_AUTO_REFRESH_DISABLE,
    DEV_DSI_AUTO_REFRESH_ENABLE,
} __attribute__((packed)) DEV_DSI_AutoRefresh_t;

typedef enum DEV_DSI_TeAckRequest {
    DEV_DSI_TEACKREQUEST_DISABLE,
    DEV_DSI_TEACKREQUEST_ENABLE,
} __attribute__((packed)) DEV_DSI_TeAckRequest_t;

typedef struct DEV_DSI_CommandConfig {
    DEV_DSI_ColorCoding_t colorCoding;

    DEV_DSI_ActivePolarity_t hsyncActPolar;
    DEV_DSI_ActivePolarity_t vsyncActPolar;
    DEV_DSI_ActivePolarity_t datEnActPolar;

    uint32_t cmdSize;

    DEV_DSI_TeSource_t teSource;
    DEV_DSI_EdgePolarity_t teEdgePolar;
    DEV_DSI_AutoRefresh_t autoRefresh;
    DEV_DSI_TeAckRequest_t teAckReq;
    DEV_DSI_EdgePolarity_t vsyncEdgePolar;
} DEV_DSI_CommandConfig_t;

typedef struct DEV_DSI_Config {
    uint8_t virtualChannel;
    DEV_DSI_WorkMode_t workMode;

    MDS_Mask_t lpCmdFlags;
    union {
        DEV_DSI_VideoConfig_t vidCfg;
        DEV_DSI_CommandConfig_t cmdCfg;
    };
} DEV_DSI_Config_t;

typedef struct DEV_DSI_Object {
    MDS_Timeout_t timeout;
    // uint32_t usHsTx;
    // uint32_t usLpRx;
    // uint32_t usHsRead;
    // uint32_t usLpRead;
    // uint32_t usHsWrite;
    // uint32_t usLpWrite;
    // uint32_t usBta;
    // bool modeHsWritePresp;
} DEV_DSI_Object_t;

typedef struct DEV_DSI_Adaptr DEV_DSI_Adaptr_t;
typedef struct DEV_DSI_Periph DEV_DSI_Periph_t;

typedef struct DEV_DSI_Driver {
    MDS_Err_t (*control)(const DEV_DSI_Adaptr_t *dsi, MDS_DevCmd_t cmd, MDS_Arg_t arg);
    MDS_Err_t (*write)(const DEV_DSI_Periph_t *periph, DEV_DSI_DataType_t type, const uint8_t *tx,
                       size_t nums);
    MDS_Err_t (*read)(const DEV_DSI_Periph_t *periph, DEV_DSI_DataType_t type, const uint8_t *tx,
                      uint8_t *rx, size_t nums);
} DEV_DSI_Driver_t;

struct DEV_DSI_Adaptr {
    const MDS_Device_t device;
    const MDS_DevHandle_t handle;
    const DEV_DSI_Driver_t *driver;
    const DEV_DSI_Periph_t *owner;
    const MDS_Mutex_t mutex;

    DEV_DSI_PhyConfig_t phyCfg;
};

struct DEV_DSI_Periph {
    const MDS_Device_t device;
    const DEV_DSI_Adaptr_t *mount;

    DEV_DSI_Object_t object;
    DEV_DSI_Config_t config;
};

/* Function ---------------------------------------------------------------- */
MDS_Err_t DEV_DSI_AdaptrInit(DEV_DSI_Adaptr_t *dsi, const char *name,
                             const DEV_DSI_Driver_t *driver, MDS_DevHandle_t handle,
                             MDS_Arg_t init);
MDS_Err_t DEV_DSI_AdaptrDeInit(DEV_DSI_Adaptr_t *dsi);
DEV_DSI_Adaptr_t *DEV_DSI_AdaptrCreate(const char *name, const DEV_DSI_Driver_t *driver,
                                       MDS_Arg_t init);
MDS_Err_t DEV_DSI_AdaptrDestroy(DEV_DSI_Adaptr_t *dsi);

MDS_Err_t DEV_DSI_PeriphInit(DEV_DSI_Periph_t *periph, const char *name, DEV_DSI_Adaptr_t *dsi);
MDS_Err_t DEV_DSI_PeriphDeInit(DEV_DSI_Periph_t *periph);
DEV_DSI_Periph_t *DEV_DSI_PeriphCreate(const char *name, DEV_DSI_Adaptr_t *dsi);
MDS_Err_t DEV_DSI_PeriphDestroy(DEV_DSI_Periph_t *periph);

MDS_Err_t DEV_DSI_PeriphOpen(DEV_DSI_Periph_t *periph, MDS_Timeout_t timeout);
MDS_Err_t DEV_DSI_PeriphClose(DEV_DSI_Periph_t *periph);

MDS_Err_t DEV_DSI_PeriphRefresh(DEV_DSI_Periph_t *periph);
MDS_Err_t DEV_DSI_PeriphWrite(DEV_DSI_Periph_t *periph, DEV_DSI_DataType_t type, const uint8_t *tx,
                              size_t nums);
MDS_Err_t DEV_DSI_PeriphRead(DEV_DSI_Periph_t *periph, DEV_DSI_DataType_t type, const uint8_t *tx,
                             uint8_t *rx, size_t nums);

MDS_Err_t DEV_DSI_AdaptrPhyInit(DEV_DSI_Adaptr_t *dsi, const DEV_DSI_PhyConfig_t *phyCfg);
MDS_Err_t DEV_DSI_AdaptrEnterULPM(DEV_DSI_Adaptr_t *dsi);
MDS_Err_t DEV_DSI_AdaptrExitULPM(DEV_DSI_Adaptr_t *dsi);
MDS_Err_t DEV_DSI_AdaptrEnterULPMData(DEV_DSI_Adaptr_t *dsi);
MDS_Err_t DEV_DSI_AdaptrExitULPMData(DEV_DSI_Adaptr_t *dsi);

#ifdef __cplusplus
}
#endif

#endif /* __DEV_DSI_H__ */
