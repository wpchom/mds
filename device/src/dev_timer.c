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
#include "dev_timer.h"

/* Timer device ------------------------------------------------------------ */
MDS_Err_t DEV_TIMER_DeviceInit(DEV_TIMER_Device_t *timer, const char *name, const DEV_TIMER_Driver_t *driver,
                               MDS_DevHandle_t *handle, const MDS_Arg_t *init)
{
    return (MDS_DevModuleInit((MDS_DevModule_t *)timer, name, (const MDS_DevDriver_t *)driver, handle, init));
}

MDS_Err_t DEV_TIMER_DeviceDeInit(DEV_TIMER_Device_t *timer)
{
    return (MDS_DevModuleDeInit((MDS_DevModule_t *)timer));
}

DEV_TIMER_Device_t *DEV_TIMER_DeviceCreate(const char *name, const DEV_TIMER_Driver_t *driver, const MDS_Arg_t *init)
{
    return ((DEV_TIMER_Device_t *)MDS_DevModuleCreate(sizeof(DEV_TIMER_Device_t), name, (const MDS_DevDriver_t *)driver,
                                                      init));
}

MDS_Err_t DEV_TIMER_DeviceDestroy(DEV_TIMER_Device_t *timer)
{
    return (MDS_DevModuleDestroy((MDS_DevModule_t *)timer));
}

MDS_Err_t DEV_TIMER_DeviceConfig(DEV_TIMER_Device_t *timer, const DEV_TIMER_Config_t *config, MDS_Tick_t timeout)
{
    MDS_ASSERT(timer != NULL);
    MDS_ASSERT(timer->driver != NULL);
    MDS_ASSERT(timer->driver->config != NULL);

    return (timer->driver->config(timer, config, timeout));
}

void DEV_TIMER_DeviceCallback(DEV_TIMER_Device_t *timer, void (*callback)(const DEV_TIMER_Device_t *, MDS_Arg_t *),
                              MDS_Arg_t *arg)
{
    MDS_ASSERT(timer != NULL);

    timer->callback = callback;
    timer->arg = arg;
}

MDS_Err_t DEV_TIMER_DeviceStart(DEV_TIMER_Device_t *timer)
{
    MDS_ASSERT(timer != NULL);
    MDS_ASSERT(timer->driver != NULL);
    MDS_ASSERT(timer->driver->control != NULL);

    return (timer->driver->control(timer, DEV_TIMER_CMD_START, NULL));
}

MDS_Err_t DEV_TIMER_DeviceStop(DEV_TIMER_Device_t *timer)
{
    MDS_ASSERT(timer != NULL);
    MDS_ASSERT(timer->driver != NULL);
    MDS_ASSERT(timer->driver->control != NULL);

    return (timer->driver->control(timer, DEV_TIMER_CMD_STOP, NULL));
}

MDS_Err_t DEV_TIMER_DeviceWait(DEV_TIMER_Device_t *timer, MDS_Tick_t timeout)
{
    MDS_ASSERT(timer != NULL);
    MDS_ASSERT(timer->driver != NULL);
    MDS_ASSERT(timer->driver->control != NULL);

    return (timer->driver->control(timer, DEV_TIMER_CMD_WAIT, (MDS_Arg_t *)&timeout));
}

MDS_Err_t DEV_TIMER_OC_ChannelInit(DEV_TIMER_OC_Channel_t *oc, const char *name, DEV_TIMER_Device_t *timer)
{
    return (MDS_DevPeriphInit((MDS_DevPeriph_t *)oc, name, (MDS_DevAdaptr_t *)timer));
}

MDS_Err_t DEV_TIMER_OC_ChannelDeInit(DEV_TIMER_OC_Channel_t *oc)
{
    return (MDS_DevPeriphDeInit((MDS_DevPeriph_t *)oc));
}

DEV_TIMER_OC_Channel_t *DEV_TIMER_OC_ChannelCreate(const char *name, DEV_TIMER_Device_t *timer)
{
    return ((DEV_TIMER_OC_Channel_t *)MDS_DevPeriphCreate(sizeof(DEV_TIMER_OC_Channel_t), name,
                                                          (MDS_DevAdaptr_t *)timer));
}

MDS_Err_t DEV_TIMER_OC_ChannelDestroy(DEV_TIMER_OC_Channel_t *oc)
{
    return (MDS_DevPeriphDestroy((MDS_DevPeriph_t *)oc));
}

void DEV_TIMER_OC_ChannelCallback(DEV_TIMER_OC_Channel_t *oc,
                                  void (*callback)(const DEV_TIMER_OC_Channel_t *, MDS_Arg_t *), MDS_Arg_t *arg)
{
    MDS_ASSERT(oc != NULL);

    oc->callback = callback;
    oc->arg = arg;
}

MDS_Err_t DEV_TIMER_OC_ChannelConfig(DEV_TIMER_OC_Channel_t *oc, const DEV_TIMER_OC_Config_t *config)
{
    MDS_ASSERT(oc != NULL);
    MDS_ASSERT(oc->mount != NULL);
    MDS_ASSERT(oc->mount->driver != NULL);
    MDS_ASSERT(oc->mount->driver->oc != NULL);

    return (oc->mount->driver->oc(oc, DEV_TIMER_CMD_OC_CONFIG, config, 0));
}

MDS_Err_t DEV_TIMER_OC_ChannelEnable(DEV_TIMER_OC_Channel_t *oc, bool enabled)
{
    MDS_ASSERT(oc != NULL);
    MDS_ASSERT(oc->mount != NULL);
    MDS_ASSERT(oc->mount->driver != NULL);
    MDS_ASSERT(oc->mount->driver->oc != NULL);

    return (oc->mount->driver->oc(oc, DEV_TIMER_CMD_OC_ENABLE, (DEV_TIMER_OC_Config_t *)(&enabled), 0));
}

MDS_Err_t DEV_TIMER_OC_ChannelDuty(DEV_TIMER_OC_Channel_t *oc, size_t nume, size_t deno)
{
    MDS_ASSERT(oc != NULL);
    MDS_ASSERT(oc->mount != NULL);
    MDS_ASSERT(oc->mount->driver != NULL);
    MDS_ASSERT(oc->mount->driver->oc != NULL);

    return (oc->mount->driver->oc(oc, DEV_TIMER_CMD_OC_DUTY, (void *)(&nume), deno));
}

MDS_Err_t DEV_TIMER_IC_ChannelInit(DEV_TIMER_IC_Channel_t *ic, const char *name, DEV_TIMER_Device_t *timer)
{
    return (MDS_DevPeriphInit((MDS_DevPeriph_t *)ic, name, (MDS_DevAdaptr_t *)timer));
}

MDS_Err_t DEV_TIMER_IC_ChannelDeInit(DEV_TIMER_IC_Channel_t *ic)
{
    return (MDS_DevPeriphDeInit((MDS_DevPeriph_t *)ic));
}

DEV_TIMER_IC_Channel_t *DEV_TIMER_IC_ChannelCreate(const char *name, DEV_TIMER_Device_t *timer)
{
    return ((DEV_TIMER_IC_Channel_t *)MDS_DevPeriphCreate(sizeof(DEV_TIMER_IC_Channel_t), name,
                                                          (MDS_DevAdaptr_t *)timer));
}

MDS_Err_t DEV_TIMER_IC_ChannelDestroy(DEV_TIMER_IC_Channel_t *ic)
{
    return (MDS_DevPeriphDestroy((MDS_DevPeriph_t *)ic));
}

void DEV_TIMER_IC_ChannelCallback(DEV_TIMER_IC_Channel_t *ic,
                                  void (*callback)(const DEV_TIMER_IC_Channel_t *, MDS_Arg_t *), MDS_Arg_t *arg)
{
    MDS_ASSERT(ic != NULL);

    ic->callback = callback;
    ic->arg = arg;
}

MDS_Err_t DEV_TIMER_IC_ChannelConfig(DEV_TIMER_IC_Channel_t *ic, const DEV_TIMER_IC_Config_t *config)
{
    MDS_ASSERT(ic != NULL);
    MDS_ASSERT(ic->mount != NULL);
    MDS_ASSERT(ic->mount->driver != NULL);
    MDS_ASSERT(ic->mount->driver->ic != NULL);

    return (ic->mount->driver->ic(ic, DEV_TIMER_CMD_IC_CONFIG, config, 0));
}

MDS_Err_t DEV_TIMER_IC_ChannelEnable(DEV_TIMER_IC_Channel_t *ic, bool enabled)
{
    MDS_ASSERT(ic != NULL);
    MDS_ASSERT(ic->mount != NULL);
    MDS_ASSERT(ic->mount->driver != NULL);
    MDS_ASSERT(ic->mount->driver->ic != NULL);

    return (ic->mount->driver->ic(ic, DEV_TIMER_CMD_IC_ENABLE, (DEV_TIMER_IC_Config_t *)(&enabled), 0));
}

MDS_Err_t DEV_TIMER_IC_ChannelGetCount(DEV_TIMER_IC_Channel_t *ic, size_t *count)
{
    MDS_ASSERT(ic != NULL);
    MDS_ASSERT(ic->mount != NULL);
    MDS_ASSERT(ic->mount->driver != NULL);
    MDS_ASSERT(ic->mount->driver->ic != NULL);

    return (ic->mount->driver->ic(ic, DEV_TIMER_CMD_IC_GET, (void *)count, 0));
}

MDS_Err_t DEV_TIMER_IC_ChannelSetCount(DEV_TIMER_IC_Channel_t *ic, size_t count)
{
    MDS_ASSERT(ic != NULL);
    MDS_ASSERT(ic->mount != NULL);
    MDS_ASSERT(ic->mount->driver != NULL);
    MDS_ASSERT(ic->mount->driver->ic != NULL);

    return (ic->mount->driver->ic(ic, DEV_TIMER_CMD_IC_SET, (void *)(&count), 0));
}
