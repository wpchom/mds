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
#ifndef __DEV_DMA_H__
#define __DEV_DMA_H__

/* Include ----------------------------------------------------------------- */
#include "mds_dev.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Typedef ----------------------------------------------------------------- */
typedef enum DEV_DMA_Direction {
    DEV_DMA_DIRECTION_P2M,
    DEV_DMA_DIRECTION_M2P,
    DEV_DMA_DIRECTION_M2M,
    DEV_DMA_DIRECTION_P2P,
} DEV_DMA_Direction_t;

typedef enum DEV_DMA_IncMode {
    DEV_DMA_INCMODE_NOINC,
    DEV_DMA_INCMODE_INC,
} DEV_DMA_IncMode_t;

typedef enum DEV_DMA_DataSize {
    DEV_DMA_DATASIZE_1B = 1,
    DEV_DMA_DATASIZE_2B = 2,
    DEV_DMA_DATASIZE_4B = 4,
    DEV_DMA_DATASIZE_8B = 8,
} DEV_DMA_DataSize_t;

typedef enum DEV_DMA_Priority {
    DEV_DMA_PRIORITY_LOW,
    DEV_DMA_PRIORITY_MED,
    DEV_DMA_PRIORITY_HIGH,
    DEV_DMA_PRIORITY_MAX,
} DEV_DMA_Prority_t;

typedef struct DEV_DMA_Config {
    DEV_DMA_Prority_t priority     : 4;
    DEV_DMA_Direction_t direction  : 4;
    DEV_DMA_DataSize_t srcDataSize : 4;
    DEV_DMA_DataSize_t dstDataSize : 4;
    DEV_DMA_IncMode_t srcIncMode   : 4;
    DEV_DMA_IncMode_t dstIncMode   : 4;
} DEV_DMA_Config_t;

typedef struct DEV_DMA_Channel {
    void *DMAx;
    MDS_Mask_t channel;

    MDS_Arg_t *parent;
    void (*errCallback)(struct DEV_DMA_Channel *channel);
    void (*halfCallback)(struct DEV_DMA_Channel *channel);
    void (*cpltCallback)(struct DEV_DMA_Channel *channel);
} DEV_DMA_Channel_t;

#ifdef __cplusplus
}
#endif

#endif /* __DEV_DMA_H__ */
