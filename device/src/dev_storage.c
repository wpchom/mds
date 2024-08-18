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
#include "dev_storage.h"

/* Storage adaptr ---------------------------------------------------------- */
MDS_Err_t DEV_STORAGE_AdaptrInit(DEV_STORAGE_Adaptr_t *storage, const char *name, const DEV_STORAGE_Driver_t *driver,
                                 MDS_DevHandle_t *handle, const MDS_Arg_t *init)
{
    return (MDS_DevAdaptrInit((MDS_DevAdaptr_t *)storage, name, (const MDS_DevDriver_t *)driver, handle, init));
}

MDS_Err_t DEV_STORAGE_AdaptrDeInit(DEV_STORAGE_Adaptr_t *storage)
{
    return (MDS_DevAdaptrDeInit((MDS_DevAdaptr_t *)storage));
}

DEV_STORAGE_Adaptr_t *DEV_STORAGE_AdaptrCreate(const char *name, const DEV_STORAGE_Driver_t *driver,
                                               const MDS_Arg_t *init)
{
    return ((DEV_STORAGE_Adaptr_t *)MDS_DevAdaptrCreate(sizeof(DEV_STORAGE_Adaptr_t), name,
                                                        (const MDS_DevDriver_t *)driver, init));
}

MDS_Err_t DEV_STORAGE_AdaptrDestroy(DEV_STORAGE_Adaptr_t *storage)
{
    return (MDS_DevAdaptrDestroy((MDS_DevAdaptr_t *)storage));
}

/* Storage periph ---------------------------------------------------------- */
MDS_Err_t DEV_STORAGE_PeriphInit(DEV_STORAGE_Periph_t *periph, const char *name, DEV_STORAGE_Adaptr_t *storage)
{
    MDS_Err_t err = MDS_DevPeriphInit((MDS_DevPeriph_t *)periph, name, (MDS_DevAdaptr_t *)storage);
    if (err == MDS_EOK) {
        periph->object.optick = MDS_DEVICE_PERIPH_TIMEOUT;
    }

    return (err);
}

MDS_Err_t DEV_STORAGE_PeriphDeInit(DEV_STORAGE_Periph_t *periph)
{
    return (MDS_DevPeriphDeInit((MDS_DevPeriph_t *)periph));
}

DEV_STORAGE_Periph_t *DEV_STORAGE_PeriphCreate(const char *name, DEV_STORAGE_Adaptr_t *storage)
{
    DEV_STORAGE_Periph_t *periph = (DEV_STORAGE_Periph_t *)MDS_DevPeriphCreate(sizeof(DEV_STORAGE_Periph_t), name,
                                                                               (MDS_DevAdaptr_t *)storage);
    if (periph != NULL) {
        periph->object.optick = MDS_DEVICE_PERIPH_TIMEOUT;
    }

    return (periph);
}

MDS_Err_t DEV_STORAGE_PeriphDestroy(DEV_STORAGE_Periph_t *periph)
{
    return (MDS_DevPeriphDestroy((MDS_DevPeriph_t *)periph));
}

MDS_Err_t DEV_STORAGE_PeriphOpen(DEV_STORAGE_Periph_t *periph, MDS_Tick_t timeout)
{
    return (MDS_DevPeriphOpen((MDS_DevPeriph_t *)periph, timeout));
}

MDS_Err_t DEV_STORAGE_PeriphClose(DEV_STORAGE_Periph_t *periph)
{
    return (MDS_DevPeriphClose((MDS_DevPeriph_t *)periph));
}

size_t DEV_STORAGE_PeriphBlockNums(DEV_STORAGE_Periph_t *periph)
{
    MDS_ASSERT(periph != NULL);

    return (periph->object.blockNums);
}

size_t DEV_STORAGE_PeriphBlockSize(DEV_STORAGE_Periph_t *periph, size_t block)
{
    MDS_ASSERT(periph != NULL);
    MDS_ASSERT(periph->mount != NULL);
    MDS_ASSERT(periph->mount->driver != NULL);
    MDS_ASSERT(periph->mount->driver->blksize != NULL);

    const DEV_STORAGE_Adaptr_t *storage = periph->mount;

    if (block > periph->object.blockNums) {
        return (0);
    }

    return (storage->driver->blksize(storage, periph->object.blockBase + block));
}

size_t DEV_STORAGE_PeriphTotalSize(DEV_STORAGE_Periph_t *periph)
{
    if (periph->totalSize == 0) {
        for (size_t cnt = 0; cnt < periph->object.blockNums; cnt++) {
            periph->totalSize += DEV_STORAGE_PeriphBlockSize(periph, cnt);
        }
    }

    return (periph->totalSize);
}

MDS_Err_t DEV_STORAGE_PeriphRead(DEV_STORAGE_Periph_t *periph, size_t block, uintptr_t ofs, uint8_t *buff, size_t len)
{
    MDS_ASSERT(periph != NULL);
    MDS_ASSERT(periph->mount != NULL);
    MDS_ASSERT(periph->mount->driver != NULL);
    MDS_ASSERT(periph->mount->driver->read != NULL);

    const DEV_STORAGE_Adaptr_t *storage = periph->mount;

    if (block > periph->object.blockNums) {
        return (MDS_EINVAL);
    }

    DEV_STORAGE_PeriphTotalSize(periph);

    return (storage->driver->read(periph, periph->object.blockBase + block, ofs, buff, len));
}

MDS_Err_t DEV_STORAGE_PeriphProgram(DEV_STORAGE_Periph_t *periph, size_t block, uintptr_t ofs, const uint8_t *buff,
                                    size_t len)
{
    MDS_ASSERT(periph != NULL);
    MDS_ASSERT(periph->mount != NULL);
    MDS_ASSERT(periph->mount->driver != NULL);
    MDS_ASSERT(periph->mount->driver->prog != NULL);

    const DEV_STORAGE_Adaptr_t *storage = periph->mount;

    if (!MDS_DevPeriphIsAccessible((MDS_DevPeriph_t *)periph)) {
        return (MDS_EIO);
    }

    if (block > periph->object.blockNums) {
        return (MDS_EINVAL);
    }

    DEV_STORAGE_PeriphTotalSize(periph);

    return (storage->driver->prog(periph, periph->object.blockBase + block, ofs, buff, len));
}

MDS_Err_t DEV_STORAGE_PeriphErase(DEV_STORAGE_Periph_t *periph, size_t block, size_t nums)
{
    MDS_ASSERT(periph != NULL);
    MDS_ASSERT(periph->mount != NULL);
    MDS_ASSERT(periph->mount->driver != NULL);
    MDS_ASSERT(periph->mount->driver->erase != NULL);

    const DEV_STORAGE_Adaptr_t *storage = periph->mount;

    if (!MDS_DevPeriphIsAccessible((MDS_DevPeriph_t *)periph)) {
        return (MDS_EIO);
    }

    if (block > periph->object.blockNums) {
        return (MDS_EINVAL);
    }

    DEV_STORAGE_PeriphTotalSize(periph);

    return (storage->driver->erase(periph, periph->object.blockBase + block, nums));
}
