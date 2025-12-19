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
typedef struct DEV_STORAGE_Periph DEV_STORAGE_Periph_t; // parition

typedef struct DEV_STORAGE_Driver {
    MDS_Err_t (*control)(const DEV_STORAGE_Adaptr_t *flash, MDS_DevCmd_t cmd, MDS_Arg_t *arg);
    MDS_Err_t (*read)(const DEV_STORAGE_Periph_t *periph, uintptr_t ofs, uint8_t *buff, size_t len,
                      size_t *read);
    MDS_Err_t (*write)(const DEV_STORAGE_Periph_t *periph, uintptr_t ofs, const uint8_t *buff,
                       size_t len, size_t *write);
    MDS_Err_t (*erase)(const DEV_STORAGE_Periph_t *periph, uintptr_t ofs, size_t size,
                       size_t *erase);
    size_t (*sector)(const DEV_STORAGE_Periph_t *periph, uintptr_t ofs, uintptr_t *align);
} DEV_STORAGE_Driver_t;

struct DEV_STORAGE_Adaptr {
    const MDS_Device_t device;
    const DEV_STORAGE_Driver_t *driver;
    const MDS_DevHandle_t *handle;
    const DEV_STORAGE_Periph_t *onwer;
    const MDS_Mutex_t mutex;
};

typedef struct DEV_STORAGE_Object {
    uintptr_t baseAddr;
    size_t sectorNums;
} DEV_STORAGE_Object_t;

struct DEV_STORAGE_Periph {
    const MDS_Device_t device;
    const DEV_STORAGE_Adaptr_t *mount;

    DEV_STORAGE_Object_t object;

    size_t totalSize;
};

/* Function ---------------------------------------------------------------- */
MDS_Err_t DEV_STORAGE_AdaptrInit(DEV_STORAGE_Adaptr_t *storage, const char *name,
                                 const DEV_STORAGE_Driver_t *driver, MDS_DevHandle_t *handle,
                                 const MDS_Arg_t *init);
MDS_Err_t DEV_STORAGE_AdaptrDeInit(DEV_STORAGE_Adaptr_t *storage);
DEV_STORAGE_Adaptr_t *DEV_STORAGE_AdaptrCreate(const char *name, const DEV_STORAGE_Driver_t *driver,
                                               const MDS_Arg_t *init);
MDS_Err_t DEV_STORAGE_AdaptrDestroy(DEV_STORAGE_Adaptr_t *storage);

MDS_Err_t DEV_STORAGE_PeriphInit(DEV_STORAGE_Periph_t *periph, const char *name,
                                 DEV_STORAGE_Adaptr_t *storage);
MDS_Err_t DEV_STORAGE_PeriphDeInit(DEV_STORAGE_Periph_t *periph);
DEV_STORAGE_Periph_t *DEV_STORAGE_PeriphCreate(const char *name, DEV_STORAGE_Adaptr_t *storage);
MDS_Err_t DEV_STORAGE_PeriphDestroy(DEV_STORAGE_Periph_t *periph);

MDS_Err_t DEV_STORAGE_PeriphOpen(DEV_STORAGE_Periph_t *periph, MDS_Timeout_t timeout);
MDS_Err_t DEV_STORAGE_PeriphClose(DEV_STORAGE_Periph_t *periph);
size_t DEV_STORAGE_PeriphSectorNums(DEV_STORAGE_Periph_t *periph);
size_t DEV_STORAGE_PeriphSectorSize(DEV_STORAGE_Periph_t *periph, uintptr_t ofs, uintptr_t *align);
size_t DEV_STORAGE_PeriphTotalSize(DEV_STORAGE_Periph_t *periph);
MDS_Err_t DEV_STORAGE_PeriphRead(DEV_STORAGE_Periph_t *periph, uintptr_t ofs, uint8_t *buff,
                                 size_t len, size_t *read);
MDS_Err_t DEV_STORAGE_PeriphWrite(DEV_STORAGE_Periph_t *periph, uintptr_t ofs, const uint8_t *buff,
                                  size_t len, size_t *write);
MDS_Err_t DEV_STORAGE_PeriphErase(DEV_STORAGE_Periph_t *periph, uintptr_t ofs, size_t size,
                                  size_t *erase);

#ifdef __cplusplus
}
#endif

#endif /* __DEV_STORAGE_H__ */
