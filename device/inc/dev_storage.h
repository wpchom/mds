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
#ifndef __DEV_STORAGE_H__
#define __DEV_STORAGE_H__

/* Include ----------------------------------------------------------------- */
#include "mds_dev.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Typedef ----------------------------------------------------------------- */
typedef struct DEV_STORAGE_Adaptr DEV_STORAGE_Adaptr_t;
typedef struct DEV_STORAGE_Periph DEV_STORAGE_Periph_t;

typedef struct DEV_STORAGE_Driver {
    MDS_Err_t (*control)(const DEV_STORAGE_Adaptr_t *storage, MDS_Item_t cmd, MDS_Arg_t *arg);
    size_t (*blksize)(const DEV_STORAGE_Adaptr_t *storage, size_t blk);
    MDS_Err_t (*read)(const DEV_STORAGE_Periph_t *periph, size_t blk, uintptr_t ofs, uint8_t *buff, size_t len);
    MDS_Err_t (*prog)(const DEV_STORAGE_Periph_t *periph, size_t blk, uintptr_t ofs, const uint8_t *buff, size_t len);
    MDS_Err_t (*erase)(const DEV_STORAGE_Periph_t *periph, size_t blk, size_t nums);
} DEV_STORAGE_Driver_t;

struct DEV_STORAGE_Adaptr {
    const MDS_Device_t device;
    const DEV_STORAGE_Driver_t *driver;
    const MDS_DevHandle_t *handle;
    const DEV_STORAGE_Periph_t *onwer;
    const MDS_Mutex_t mutex;
};

typedef struct DEV_STORAGE_Object {
    size_t blockBase;
    size_t blockNums;
} DEV_STORAGE_Object_t;

struct DEV_STORAGE_Periph {
    const MDS_Device_t device;
    const DEV_STORAGE_Adaptr_t *mount;

    DEV_STORAGE_Object_t object;

    size_t totalSize;
};

/* Function ---------------------------------------------------------------- */
extern MDS_Err_t DEV_STORAGE_AdaptrInit(DEV_STORAGE_Adaptr_t *storage, const char *name,
                                        const DEV_STORAGE_Driver_t *driver, MDS_DevHandle_t *handle,
                                        const MDS_Arg_t *init);
extern MDS_Err_t DEV_STORAGE_AdaptrDeInit(DEV_STORAGE_Adaptr_t *storage);
extern DEV_STORAGE_Adaptr_t *DEV_STORAGE_AdaptrCreate(const char *name, const DEV_STORAGE_Driver_t *driver,
                                                      const MDS_Arg_t *init);
extern MDS_Err_t DEV_STORAGE_AdaptrDestroy(DEV_STORAGE_Adaptr_t *storage);

extern MDS_Err_t DEV_STORAGE_PeriphInit(DEV_STORAGE_Periph_t *periph, const char *name, DEV_STORAGE_Adaptr_t *storage);
extern MDS_Err_t DEV_STORAGE_PeriphDeInit(DEV_STORAGE_Periph_t *periph);
extern DEV_STORAGE_Periph_t *DEV_STORAGE_PeriphCreate(const char *name, DEV_STORAGE_Adaptr_t *storage);
extern MDS_Err_t DEV_STORAGE_PeriphDestroy(DEV_STORAGE_Periph_t *periph);

extern MDS_Err_t DEV_STORAGE_PeriphOpen(DEV_STORAGE_Periph_t *periph, MDS_Tick_t timeout);
extern MDS_Err_t DEV_STORAGE_PeriphClose(DEV_STORAGE_Periph_t *periph);
extern size_t DEV_STORAGE_PeriphBlockNums(DEV_STORAGE_Periph_t *periph);
extern size_t DEV_STORAGE_PeriphBlockSize(DEV_STORAGE_Periph_t *periph, size_t block);
extern size_t DEV_STORAGE_PeriphTotalSize(DEV_STORAGE_Periph_t *periph);
extern MDS_Err_t DEV_STORAGE_PeriphRead(DEV_STORAGE_Periph_t *periph, size_t block, uintptr_t ofs, uint8_t *buff,
                                        size_t len);
extern MDS_Err_t DEV_STORAGE_PeriphProgram(DEV_STORAGE_Periph_t *periph, size_t block, uintptr_t ofs,
                                           const uint8_t *buff, size_t len);
extern MDS_Err_t DEV_STORAGE_PeriphErase(DEV_STORAGE_Periph_t *periph, size_t block, size_t nums);

#ifdef __cplusplus
}
#endif

#endif /* __DEV_STORAGE_H__ */
