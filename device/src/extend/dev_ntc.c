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
/* Include ----------------------------------------------------------------- */
#include "extend/dev_ntc.h"

/* NTC device -------------------------------------------------------------- */
MDS_Err_t DEV_NTC_DeviceInit(DEV_NTC_Device_t *ntc, const char *name, const DEV_NTC_Driver_t *driver,
                             MDS_DevHandle_t *handle, const MDS_Arg_t *init)
{
    return (MDS_DevModuleInit((MDS_DevModule_t *)ntc, name, (const MDS_DevDriver_t *)driver, handle, init));
}

MDS_Err_t DEV_NTC_DeviceDeInit(DEV_NTC_Device_t *ntc)
{
    return (MDS_DevModuleDeInit((MDS_DevModule_t *)ntc));
}

DEV_NTC_Device_t *DEV_NTC_DeviceCreate(const char *name, const DEV_NTC_Driver_t *driver, const MDS_Arg_t *init)
{
    return ((DEV_NTC_Device_t *)MDS_DevModuleCreate(sizeof(DEV_NTC_Device_t), name, (const MDS_DevDriver_t *)driver,
                                                    init));
}

MDS_Err_t DEV_NTC_DeviceDestroy(DEV_NTC_Device_t *ntc)
{
    return (MDS_DevModuleDestroy((MDS_DevModule_t *)ntc));
}

void DEV_NTC_DeviceCompensation(DEV_NTC_Device_t *ntc, void (*compensation)(DEV_NTC_Value_t *))
{
    MDS_ASSERT(ntc != NULL);

    ntc->compensation = compensation;
}

MDS_Err_t DEV_NTC_DeviceGetValue(DEV_NTC_Device_t *ntc, DEV_NTC_Value_t *value)
{
    MDS_ASSERT(ntc != NULL);
    MDS_ASSERT(ntc->driver != NULL);
    MDS_ASSERT(ntc->driver->control != NULL);

    MDS_Err_t err = ntc->driver->control(ntc, DEV_NTC_CMD_GET_VALUE, (MDS_Arg_t *)value);
    if (ntc->compensation != NULL) {
        ntc->compensation(value);
    }

    return (err);
}
