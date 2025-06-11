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
#ifndef __MDS_DEV_H__
#define __MDS_DEV_H__

/* Include ----------------------------------------------------------------- */
#include "mds_sys.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Typedef ----------------------------------------------------------------- */
typedef struct MDS_DeviceCmd {
    uint32_t cmd;
} MDS_DeviceCmd_t;

#define MDS_DEVICE_COMMAND(c) ((MDS_DeviceCmd_t) {.cmd = c})

enum MDS_DEVICE_Cmd {
    MDS_DEVICE_CMD_INIT,
    MDS_DEVICE_CMD_DEINIT,
    MDS_DEVICE_CMD_HANDLESZ,
    MDS_DEVICE_CMD_OPEN,
    MDS_DEVICE_CMD_CLOSE,
    MDS_DEVICE_CMD_GETID,
    MDS_DEVICE_CMD_PROBE,
    MDS_DEVICE_CMD_DUMP,

    MDS_DEVICE_CMD_DRIVER,
};

typedef struct MDS_Device MDS_Device_t;
typedef struct MDS_DevModule MDS_DevModule_t;
typedef struct MDS_DevAdaptr MDS_DevAdaptr_t;
typedef struct MDS_DevPeriph MDS_DevPeriph_t;
typedef union MDS_DevHandle {
    void *handle;
} MDS_DevHandle_t;

typedef struct MDS_DevDriver {
    MDS_Err_t (*control)(const MDS_Device_t *device, MDS_DeviceCmd_t cmd, MDS_Arg_t *arg);
} MDS_DevDriver_t;

struct MDS_Device {
    MDS_Object_t object;
    MDS_Mask_t flags;
    void (*hook)(const MDS_Device_t *device, MDS_DeviceCmd_t cmd);
};

struct MDS_DevModule {
    MDS_Device_t device;
    const MDS_DevDriver_t *driver;
    MDS_DevHandle_t *handle;
};

struct MDS_DevAdaptr {
    MDS_Device_t device;
    const MDS_DevDriver_t *driver;
    MDS_DevHandle_t *handle;
    MDS_DevPeriph_t *owner;
    MDS_Mutex_t mutex;
};

struct MDS_DevPeriph {
    MDS_Device_t device;
    MDS_DevAdaptr_t *mount;
};

typedef struct MDS_DevProbeId {
    const char *name;
    uint32_t number;
} MDS_DevProbeId_t;

typedef struct MDS_DevProbeTable {
    const MDS_DevDriver_t *driver;
    MDS_Device_t *(*callback)(const MDS_Device_t *device, const MDS_DevDriver_t *driver);
} MDS_DevProbeTable_t;

typedef struct MDS_DevDumpData {
    const uint8_t *regList;
    uint16_t regNums;
    uint8_t regWidth;
    uint8_t dataWidth;
    uint8_t *outBuff;
    size_t outSize;
} MDS_DevDumpData_t;

/* Function ---------------------------------------------------------------- */
MDS_Device_t *MDS_DeviceFind(const char *name);
void MDS_DeviceRegisterHook(const MDS_Device_t *device,
                            void (*hook)(const MDS_Device_t *device, MDS_DeviceCmd_t cmd));

MDS_Err_t MDS_DevModuleInit(MDS_DevModule_t *module, const char *name,
                            const MDS_DevDriver_t *driver, MDS_DevHandle_t *handle,
                            const MDS_Arg_t *init);
MDS_Err_t MDS_DevModuleDeInit(MDS_DevModule_t *module);
MDS_DevModule_t *MDS_DevModuleCreate(size_t typesz, const char *name,
                                     const MDS_DevDriver_t *driver, const MDS_Arg_t *init);
MDS_Err_t MDS_DevModuleDestroy(MDS_DevModule_t *module);

MDS_Err_t MDS_DevAdaptrInit(MDS_DevAdaptr_t *adaptr, const char *name,
                            const MDS_DevDriver_t *driver, MDS_DevHandle_t *handle,
                            const MDS_Arg_t *init);
MDS_Err_t MDS_DevAdaptrDeInit(MDS_DevAdaptr_t *adaptr);
MDS_DevAdaptr_t *MDS_DevAdaptrCreate(size_t typesz, const char *name,
                                     const MDS_DevDriver_t *driver, const MDS_Arg_t *init);
MDS_Err_t MDS_DevAdaptrDestroy(MDS_DevAdaptr_t *adaptr);
MDS_Err_t MDS_DevAdaptrUpdateOpen(MDS_DevAdaptr_t *adaptr);

MDS_Err_t MDS_DevPeriphInit(MDS_DevPeriph_t *periph, const char *name, MDS_DevAdaptr_t *adaptr);
MDS_Err_t MDS_DevPeriphDeInit(MDS_DevPeriph_t *periph);
MDS_DevPeriph_t *MDS_DevPeriphCreate(size_t typesz, const char *name, MDS_DevAdaptr_t *adaptr);
MDS_Err_t MDS_DevPeriphDestroy(MDS_DevPeriph_t *periph);
MDS_Err_t MDS_DevPeriphOpen(MDS_DevPeriph_t *periph, MDS_Timeout_t timeout);
MDS_Err_t MDS_DevPeriphClose(MDS_DevPeriph_t *periph);
MDS_DevPeriph_t *MDS_DevPeriphOpenForce(MDS_DevPeriph_t *periph);
bool MDS_DevPeriphIsAccessable(MDS_DevPeriph_t *periph);

bool MDS_DeviceIsPeriph(const MDS_Device_t *device);
const MDS_DevProbeId_t *MDS_DeviceGetId(const MDS_Device_t *device);
MDS_Err_t MDS_DevModuleDump(const MDS_Device_t *device, MDS_DevDumpData_t *dump);
MDS_Device_t *MDS_DeviceProbeDrivers(const MDS_DevDriver_t **driver, const MDS_Device_t *device,
                                     const MDS_DevProbeTable_t drvList[], size_t drvSize);

/* Define ------------------------------------------------------------------ */
#define MDS_DEVICE_ARG_HANDLE_SIZE(arg, handleT)                                                  \
    if ((arg) != NULL) {                                                                          \
        *((size_t *)(arg)) = sizeof(handleT);                                                     \
    }

#ifdef __cplusplus
}
#endif

#endif /* __MDS_DEV_H__ */
