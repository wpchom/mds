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
#include "mds/dev.h"

/* Define ------------------------------------------------------------------ */
MDS_LOG_MODULE_DECLARE(kernel, CONFIG_MDS_KERNEL_LOG_LEVEL);

/* Typedef ----------------------------------------------------------------- */
enum MDS_DeviceFlag {
    MDS_DEVICE_FLAG_CLOSE = 0x00U,
    MDS_DEVICE_FLAG_OPEN = 0x80U,
    MDS_DEVICE_FLAG_MODULE = 0x04U,
    MDS_DEVICE_FLAG_ADAPTR = 0x02U,
    MDS_DEVICE_FLAG_PERIPH = 0x01U,
};

/* Function ---------------------------------------------------------------- */
MDS_Device_t *MDS_DeviceFind(const char *name)
{
    MDS_Object_t *object = MDS_ObjectFind(MDS_OBJECT_TYPE_DEVICE, name);

    return ((object != NULL) ? (CONTAINER_OF(object, MDS_Device_t, object)) : (NULL));
}

void MDS_DeviceRegisterHook(const MDS_Device_t *device,
                            void (*hook)(const MDS_Device_t *device, MDS_DevCmd_t cmd))
{
    MDS_ASSERT(device != NULL);

    ((MDS_Device_t *)device)->hook = hook;
}

static bool MDS_DeviceIsBusy(MDS_Device_t *device)
{
    if ((device->flag.mask & (MDS_DEVICE_FLAG_MODULE | MDS_DEVICE_FLAG_ADAPTR)) == 0U) {
        return (false);
    }

    MDS_Device_t *nextdev = CONTAINER_OF(device->object.node.next, MDS_Device_t, object.node);
    if (nextdev == NULL) {
        return (false);
    }

    if ((((MDS_DevPeriph_t *)(nextdev))->mount != (void *)(device))) {
        return (false);
    }

    return (true);
}

MDS_Err_t MDS_DevModuleInit(MDS_DevModule_t *module, const char *name,
                            const MDS_DevDriver_t *driver, MDS_DevHandle_t handle, MDS_Arg_t init)
{
    MDS_ASSERT(module != NULL);

    MDS_Err_t err = MDS_ObjectInit(&(module->device.object), MDS_OBJECT_TYPE_DEVICE, name);
    if (MDS_ErrIsSame(err, MDS_EOK)) {
        module->device.flag.mask = MDS_DEVICE_FLAG_MODULE;
        module->handle = handle;
        module->driver = driver;

        if ((driver != NULL) && (driver->control != NULL)) {
            err = driver->control(&(module->device), MDS_DEVICE_CMD_INIT, init);
        }

        if (!MDS_ErrIsSame(err, MDS_EOK)) {
            MDS_ObjectDeInit(&(module->device.object));
        }
    }

    MDS_LOG_D("[device] module(%p) init driver(%p) handle(%p) err(%d)", module, driver, handle.ptr,
              err.errno);

    return (err);
}

MDS_Err_t MDS_DevModuleDeInit(MDS_DevModule_t *module)
{
    MDS_ASSERT(module != NULL);
    MDS_ASSERT((module->device.flag.mask & MDS_DEVICE_FLAG_MODULE) != 0U);
    MDS_ASSERT(!MDS_ObjectIsCreated(&(module->device.object)));

    if (MDS_DeviceIsBusy(&(module->device))) {
        return (MDS_EBUSY);
    }

    MDS_Err_t err = MDS_EOK;
    if ((module->driver != NULL) && (module->driver->control != NULL)) {
        err = module->driver->control(&(module->device), MDS_DEVICE_CMD_DEINIT, MDS_ARG_WITH(NULL));
    }
    if (MDS_ErrIsSame(err, MDS_EOK)) {
        MDS_ObjectDeInit(&(module->device.object));
    }

    return (err);
}

MDS_DevModule_t *MDS_DevModuleCreate(size_t typesz, const char *name, const MDS_DevDriver_t *driver,
                                     MDS_Arg_t init)
{
    MDS_Err_t err = MDS_EOK;
    MDS_DevModule_t *module =
        (MDS_DevModule_t *)MDS_ObjectCreate(typesz, MDS_OBJECT_TYPE_DEVICE, name);
    if (module != NULL) {
        module->device.flag.mask = MDS_DEVICE_FLAG_MODULE;
        module->driver = driver;

        if ((driver == NULL) || (driver->control == NULL)) {
            return (module);
        }

        size_t handlesz = 0;
        driver->control(&(module->device), MDS_DEVICE_CMD_HANDLESZ, MDS_ARG_WITH(&handlesz));
        if (handlesz > 0) {
            module->handle.ptr = MDS_SysMemCalloc(1, handlesz);
            if (module->handle.ptr == NULL) {
                err = MDS_ENOMEM;
            }
        }
        if (MDS_ErrIsSame(err, MDS_EOK)) {
            err = driver->control(&(module->device), MDS_DEVICE_CMD_INIT, init);
        }
        if (!MDS_ErrIsSame(err, MDS_EOK)) {
            MDS_SysMemFree(module->handle.ptr);
            MDS_ObjectDestroy(&(module->device.object));
        }
    }

    MDS_LOG_D("[device] module(%p) create driver(%p) err(%d)", module, driver, err.errno);

    return (module);
}

MDS_Err_t MDS_DevModuleDestroy(MDS_DevModule_t *module)
{
    MDS_ASSERT(module != NULL);
    MDS_ASSERT((module->device.flag.mask & MDS_DEVICE_FLAG_MODULE) != 0U);
    MDS_ASSERT(MDS_ObjectIsCreated(&(module->device.object)));

    if (MDS_DeviceIsBusy(&(module->device))) {
        return (MDS_EBUSY);
    }

    MDS_Err_t err = MDS_EOK;
    if ((module->driver != NULL) && (module->driver->control != NULL)) {
        err = module->driver->control(&(module->device), MDS_DEVICE_CMD_DEINIT, MDS_ARG_WITH(NULL));
    }
    if (MDS_ErrIsSame(err, MDS_EOK)) {
        MDS_SysMemFree(module->handle.ptr);
        MDS_ObjectDestroy(&(module->device.object));
    }

    return (err);
}

MDS_Err_t MDS_DevAdaptrInit(MDS_DevAdaptr_t *adaptr, const char *name,
                            const MDS_DevDriver_t *driver, MDS_DevHandle_t handle, MDS_Arg_t init)
{
    MDS_ASSERT(adaptr != NULL);

    MDS_Err_t err = MDS_ObjectInit(&(adaptr->device.object), MDS_OBJECT_TYPE_DEVICE, name);
    if (MDS_ErrIsSame(err, MDS_EOK)) {
        adaptr->device.flag.mask = MDS_DEVICE_FLAG_ADAPTR;
        adaptr->handle = handle;
        adaptr->driver = driver;

        err = MDS_MutexInit(&(adaptr->mutex), name);
        if (MDS_ErrIsSame(err, MDS_EOK)) {
            if ((driver != NULL) && (driver->control != NULL)) {
                err = driver->control(&(adaptr->device), MDS_DEVICE_CMD_INIT, init);
            }
            if (!MDS_ErrIsSame(err, MDS_EOK)) {
                MDS_MutexDeInit(&(adaptr->mutex));
            }
        }
        if (!MDS_ErrIsSame(err, MDS_EOK)) {
            MDS_ObjectDeInit(&(adaptr->device.object));
        }
    }

    return (err);
}

MDS_Err_t MDS_DevAdaptrDeInit(MDS_DevAdaptr_t *adaptr)
{
    MDS_ASSERT(adaptr != NULL);
    MDS_ASSERT((adaptr->device.flag.mask & MDS_DEVICE_FLAG_ADAPTR) != 0U);
    MDS_ASSERT(!MDS_ObjectIsCreated(&(adaptr->device.object)));

    if (MDS_DeviceIsBusy(&adaptr->device)) {
        return (MDS_EBUSY);
    }

    MDS_Err_t err = MDS_EOK;
    if ((adaptr->driver != NULL) && (adaptr->driver->control != NULL)) {
        err = adaptr->driver->control(&(adaptr->device), MDS_DEVICE_CMD_DEINIT, MDS_ARG_WITH(NULL));
    }
    if (MDS_ErrIsSame(err, MDS_EOK)) {
        MDS_MutexDeInit(&(adaptr->mutex));
        MDS_ObjectDeInit(&(adaptr->device.object));
    }

    return (err);
}

MDS_DevAdaptr_t *MDS_DevAdaptrCreate(size_t typesz, const char *name, const MDS_DevDriver_t *driver,
                                     MDS_Arg_t init)
{
    MDS_DevAdaptr_t *adaptr =
        (MDS_DevAdaptr_t *)MDS_ObjectCreate(typesz, MDS_OBJECT_TYPE_DEVICE, name);
    if (adaptr != NULL) {
        adaptr->device.flag.mask = MDS_DEVICE_FLAG_ADAPTR;
        adaptr->driver = driver;

        if ((driver == NULL) || (driver->control == NULL)) {
            return (adaptr);
        }

        size_t handlesz = 0;
        driver->control(&(adaptr->device), MDS_DEVICE_CMD_HANDLESZ, MDS_ARG_WITH(&handlesz));
        if (handlesz > 0) {
            adaptr->handle.ptr = MDS_SysMemCalloc(1, handlesz);
            if (adaptr->handle.ptr == NULL) {
                MDS_ObjectDestroy(&(adaptr->device.object));
                return (NULL);
            }
        }

        MDS_Err_t err = MDS_MutexInit(&(adaptr->mutex), name);
        if (MDS_ErrIsSame(err, MDS_EOK)) {
            err = driver->control(&(adaptr->device), MDS_DEVICE_CMD_INIT, init);
            if (MDS_ErrIsSame(err, MDS_EOK)) {
                return (adaptr);
            }
            MDS_MutexDeInit(&(adaptr->mutex));
        }
        MDS_SysMemFree(adaptr->handle.ptr);
        MDS_ObjectDestroy(&(adaptr->device.object));
    }

    return (NULL);
}

MDS_Err_t MDS_DevAdaptrDestroy(MDS_DevAdaptr_t *adaptr)
{
    MDS_ASSERT(adaptr != NULL);
    MDS_ASSERT((adaptr->device.flag.mask & MDS_DEVICE_FLAG_ADAPTR) != 0U);
    MDS_ASSERT(MDS_ObjectIsCreated(&(adaptr->device.object)));

    if (MDS_DeviceIsBusy(&adaptr->device)) {
        return (MDS_EBUSY);
    }

    MDS_Err_t err = MDS_EOK;
    if ((adaptr->driver != NULL) && (adaptr->driver->control != NULL)) {
        err = adaptr->driver->control(&(adaptr->device), MDS_DEVICE_CMD_DEINIT, MDS_ARG_WITH(NULL));
    }
    if (MDS_ErrIsSame(err, MDS_EOK)) {
        MDS_SysMemFree(adaptr->handle.ptr);
        MDS_MutexDeInit(&(adaptr->mutex));
        MDS_ObjectDestroy(&(adaptr->device.object));
    }

    return (err);
}

MDS_Err_t MDS_DevAdaptrUpdateOpen(MDS_DevAdaptr_t *adaptr)
{
    MDS_ASSERT(adaptr != NULL);
    MDS_ASSERT((adaptr->device.flag.mask & MDS_DEVICE_FLAG_ADAPTR) != 0U);

    MDS_Err_t err = MDS_EOK;

    if ((adaptr->device.flag.mask & MDS_DEVICE_FLAG_OPEN) != 0U) {
        if ((adaptr->driver != NULL) && (adaptr->driver->control != NULL)) {
            err = adaptr->driver->control(&(adaptr->device), MDS_DEVICE_CMD_OPEN,
                                          MDS_ARG_WITH(adaptr->owner));
        }
    }

    return (err);
}

MDS_Err_t MDS_DevPeriphInit(MDS_DevPeriph_t *periph, const char *name, MDS_DevAdaptr_t *adaptr)
{
    MDS_ASSERT(periph != NULL);
    MDS_ASSERT(adaptr != NULL);

    MDS_Err_t err = MDS_ObjectInit(&(periph->device.object), MDS_OBJECT_TYPE_DEVICE, name);
    if (MDS_ErrIsSame(err, MDS_EOK)) {
        periph->device.flag.mask = MDS_DEVICE_FLAG_PERIPH;
        periph->mount = adaptr;

        MDS_ObjectInfo_t *objInfo = MDS_ObjectGetInfo(MDS_OBJECT_TYPE_DEVICE);

        MDS_Lock_t lock = MDS_CriticalLock(&(objInfo->spinlock));
        MDS_DListRemoveNode(&(periph->device.object.node));
        MDS_DListInsertNodeNext(&(adaptr->device.object.node), &(periph->device.object.node));
        MDS_CriticalRestore(&(objInfo->spinlock), lock);
    }

    return (err);
}

MDS_Err_t MDS_DevPeriphDeInit(MDS_DevPeriph_t *periph)
{
    MDS_ASSERT(periph != NULL);
    MDS_ASSERT((periph->device.flag.mask & MDS_DEVICE_FLAG_PERIPH) != 0U);
    MDS_ASSERT(!MDS_ObjectIsCreated(&(periph->device.object)));

    MDS_Err_t err = MDS_DevPeriphClose(periph);
    if (MDS_ErrIsSame(err, MDS_EOK)) {
        MDS_ObjectDeInit(&(periph->device.object));
    }

    return (err);
}

MDS_DevPeriph_t *MDS_DevPeriphCreate(size_t typesz, const char *name, MDS_DevAdaptr_t *adaptr)
{
    MDS_ASSERT(adaptr != NULL);

    MDS_DevPeriph_t *periph =
        (MDS_DevPeriph_t *)MDS_ObjectCreate(typesz, MDS_OBJECT_TYPE_DEVICE, name);
    if (periph != NULL) {
        periph->device.flag.mask = MDS_DEVICE_FLAG_PERIPH;
        periph->mount = adaptr;

        MDS_ObjectInfo_t *objInfo = MDS_ObjectGetInfo(MDS_OBJECT_TYPE_DEVICE);

        MDS_Lock_t lock = MDS_CriticalLock(&(objInfo->spinlock));
        MDS_DListRemoveNode(&(periph->device.object.node));
        MDS_DListInsertNodeNext(&(adaptr->device.object.node), &(periph->device.object.node));
        MDS_CriticalRestore(&(objInfo->spinlock), lock);
    }

    return (periph);
}

MDS_Err_t MDS_DevPeriphDestroy(MDS_DevPeriph_t *periph)
{
    MDS_ASSERT(periph != NULL);
    MDS_ASSERT((periph->device.flag.mask & MDS_DEVICE_FLAG_PERIPH) != 0U);
    MDS_ASSERT(MDS_ObjectIsCreated(&(periph->device.object)));

    MDS_Err_t err = MDS_DevPeriphClose(periph);
    if (MDS_ErrIsSame(err, MDS_EOK)) {
        MDS_ObjectDestroy(&(periph->device.object));
    }

    return (err);
}

MDS_Err_t MDS_DevPeriphOpen(MDS_DevPeriph_t *periph, MDS_Timeout_t timeout)
{
    MDS_ASSERT(periph != NULL);
    MDS_ASSERT(periph->mount != NULL);

    MDS_DevAdaptr_t *adaptr = periph->mount;

    MDS_Err_t err = MDS_MutexAcquire(&(adaptr->mutex), timeout);
    if (!MDS_ErrIsSame(err, MDS_EOK)) {
        return (err);
    }

    if (periph->device.hook != NULL) {
        periph->device.hook(&(periph->device), MDS_DEVICE_CMD_OPEN);
    }

    if ((adaptr->driver != NULL) && (adaptr->driver->control != NULL)) {
        if (adaptr->device.hook != NULL) {
            adaptr->device.hook(&(adaptr->device), MDS_DEVICE_CMD_OPEN);
        }
        err = adaptr->driver->control(&(adaptr->device), MDS_DEVICE_CMD_OPEN, MDS_ARG_WITH(periph));
        if (!MDS_ErrIsSame(err, MDS_EOK)) {
            MDS_MutexRelease(&(adaptr->mutex));
            MDS_LOG_E("[device] periph(%p) open adaptr(%p) failed err=%d", periph, adaptr,
                      err.errno);
            return (err);
        }
    }
    adaptr->owner = periph;
    adaptr->device.flag.mask |= MDS_DEVICE_FLAG_OPEN;
    periph->device.flag.mask |= MDS_DEVICE_FLAG_OPEN;

    return (err);
}

MDS_Err_t MDS_DevPeriphClose(MDS_DevPeriph_t *periph)
{
    MDS_ASSERT(periph != NULL);
    MDS_ASSERT(periph->mount != NULL);

    MDS_Err_t err = MDS_EIO;
    MDS_DevAdaptr_t *adaptr = periph->mount;

    if (adaptr->owner == periph) {
        if ((adaptr->driver != NULL) && (adaptr->driver->control != NULL)) {
            err = adaptr->driver->control(&(adaptr->device), MDS_DEVICE_CMD_CLOSE,
                                          MDS_ARG_WITH(NULL));
            if (!MDS_ErrIsSame(err, MDS_EOK)) {
                MDS_LOG_E("[device] periph(%p) close adaptr(%p) failed err:%d", periph, adaptr,
                          err.errno);
            }
            if (adaptr->device.hook != NULL) {
                adaptr->device.hook(&(adaptr->device), MDS_DEVICE_CMD_CLOSE);
            }
        }
        if (periph->device.hook != NULL) {
            periph->device.hook(&(periph->device), MDS_DEVICE_CMD_CLOSE);
        }
        periph->device.flag.mask &= ~MDS_DEVICE_FLAG_OPEN;
        adaptr->device.flag.mask &= ~MDS_DEVICE_FLAG_OPEN;

        MDS_MutexRelease(&(adaptr->mutex));
    }

    return (err);
}

MDS_DevPeriph_t *MDS_DevPeriphOpenForce(MDS_DevPeriph_t *periph)
{
    MDS_ASSERT(periph != NULL);
    MDS_ASSERT(periph->mount != NULL);

    MDS_Err_t err = MDS_EOK;
    MDS_DevAdaptr_t *adaptr = periph->mount;
    MDS_DevPeriph_t *owner = adaptr->owner;
    bool isOpened = false;

    if ((owner != NULL) && ((owner->device.flag.mask & MDS_DEVICE_FLAG_OPEN) != 0U)) {
        owner->device.flag.mask &= ~MDS_DEVICE_FLAG_OPEN;
        isOpened = true;
    }
    if ((adaptr->driver != NULL) && (adaptr->driver->control != NULL)) {
        adaptr->driver->control(&(adaptr->device), MDS_DEVICE_CMD_CLOSE, MDS_ARG_WITH(NULL));
        err = adaptr->driver->control(&(adaptr->device), MDS_DEVICE_CMD_OPEN, MDS_ARG_WITH(periph));
    }
    if (MDS_ErrIsSame(err, MDS_EOK)) {
        adaptr->owner = periph;
        adaptr->device.flag.mask |= MDS_DEVICE_FLAG_OPEN;
        periph->device.flag.mask |= MDS_DEVICE_FLAG_OPEN;
    }

    return ((isOpened) ? (owner) : (NULL));
}

bool MDS_DevPeriphIsAccessable(MDS_DevPeriph_t *periph)
{
    MDS_ASSERT(periph != NULL);
    MDS_ASSERT(periph->mount != NULL);

    bool accessible = false;

    if ((periph == periph->mount->owner) &&
        ((periph->device.flag.mask & MDS_DEVICE_FLAG_OPEN) == MDS_DEVICE_FLAG_OPEN)) {
        accessible = true;
    }

    return (accessible);
}

bool MDS_DeviceIsPeriph(const MDS_Device_t *device)
{
    MDS_ASSERT(device != NULL);

    return ((device->flag.mask &
             (MDS_DEVICE_FLAG_MODULE | MDS_DEVICE_FLAG_ADAPTR | MDS_DEVICE_FLAG_PERIPH)) ==
            MDS_DEVICE_FLAG_PERIPH);
}

const MDS_DevProbeId_t *MDS_DeviceGetId(const MDS_Device_t *device)
{
    MDS_ASSERT(device != NULL);

    const MDS_DevProbeId_t *id = NULL;

    if (device->hook != NULL) {
        device->hook(device, MDS_DEVICE_CMD_GETID);
    }

    if ((device->flag.mask & (MDS_DEVICE_FLAG_MODULE | MDS_DEVICE_FLAG_ADAPTR)) != 0U) {
        MDS_DevModule_t *module = CONTAINER_OF(device, MDS_DevModule_t, device);
        if ((module->driver != NULL) && (module->driver->control != NULL)) {
            module->driver->control(&(module->device), MDS_DEVICE_CMD_GETID, MDS_ARG_WITH(&id));
        }
    }

    return (id);
}

MDS_Device_t *MDS_DeviceProbeDrivers(const MDS_DevDriver_t **driver, MDS_Device_t *device,
                                     const MDS_DevProbeTable_t drvList[], size_t drvSize)
{
    MDS_ASSERT(device != NULL);

    if (device->hook != NULL) {
        device->hook(device, MDS_DEVICE_CMD_PROBE);
    }

    for (size_t idx = 0; idx < drvSize; idx++) {
        if ((drvList[idx].driver == NULL) || (drvList[idx].driver->control == NULL)) {
            continue;
        }
        MDS_Err_t err =
            drvList[idx].driver->control(device, MDS_DEVICE_CMD_PROBE, MDS_ARG_WITH(device));
        if (!MDS_ErrIsSame(err, MDS_EOK)) {
            continue;
        }
        if (driver != NULL) {
            *driver = drvList[idx].driver;
        }
        if (drvList[idx].callback != NULL) {
            return (drvList[idx].callback(device, drvList[idx].driver));
        }
        break;
    }

    return (NULL);
}

MDS_Err_t MDS_DevModuleDump(const MDS_Device_t *device, MDS_DevDumpData_t *dump)
{
    MDS_ASSERT(device != NULL);
    MDS_ASSERT(dump != NULL);

    if (device->hook != NULL) {
        device->hook(device, MDS_DEVICE_CMD_DUMP);
    }

    MDS_DevModule_t *module = CONTAINER_OF(device, MDS_DevModule_t, device);
    MDS_Err_t err = MDS_EIO;
    if ((module->driver != NULL) && (module->driver->control != NULL)) {
        err = module->driver->control(device, MDS_DEVICE_CMD_DUMP, MDS_ARG_WITH(dump));
    }

    return (err);
}
