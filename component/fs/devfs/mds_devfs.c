#include "../mds_fs.h"
#include "mds_dev.h"

static MDS_Err_t DEVFS_Mount(MDS_FileSystem_t *fs)
{
    UNUSED(fs);

    return (MDS_EOK);
}

static MDS_Err_t DEVFS_Unmount(MDS_FileSystem_t *fs)
{
    UNUSED(fs);

    return (MDS_EOK);
}

static MDS_Err_t DEVFS_Open(MDS_FileDesc_t *fd)
{
    if ((fd->flags & (MDS_OFLAG_APPEND | MDS_OFLAG_EXCL | MDS_OFLAG_CREAT | MDS_OFLAG_TRUNC)) != 0U) {
        return (MDS_EPERM);
    }

    const char *devName = &(fd->node->path[strlen(fd->node->fs->path)]);
    if (*devName == '\0') {
        fd->node->data = (MDS_Arg_t *)MDS_ObjectGetList(MDS_OBJECT_TYPE_DEVICE);
        return (MDS_EOK);
    }

    MDS_Device_t *device = MDS_DeviceFind(devName);
    if (device == NULL) {
        return (MDS_ENODEV);
    }

    fd->node->data = (MDS_Arg_t *)device;

    if (MDS_DeviceIsPeriph(device)) {
        return (MDS_DevPeriphOpen((MDS_DevPeriph_t *)device, MDS_TICK_FOREVER));
    }

    return (MDS_EOK);
}

static MDS_Err_t DEVFS_Close(MDS_FileDesc_t *fd)
{
    MDS_Device_t *device = (MDS_Device_t *)(fd->node->data);
    if (device == NULL) {
        return (MDS_EIO);
    }

    fd->node->data = NULL;

    if ((MDS_Arg_t *)device == (MDS_Arg_t *)MDS_ObjectGetList(MDS_OBJECT_TYPE_DEVICE)) {
        return (MDS_EOK);
    }

    if (MDS_DeviceIsPeriph(device)) {
        return (MDS_DevPeriphClose((MDS_DevPeriph_t *)device));
    }

    return (MDS_EOK);
}

static MDS_Err_t DEVFS_Ioctl(MDS_FileDesc_t *fd, MDS_Item_t cmd, MDS_Arg_t *args)
{
    MDS_Device_t *device = (MDS_Device_t *)(fd->node->data);
    if (device == NULL) {
        return (MDS_EIO);
    }

    if (!MDS_DeviceIsPeriph(device)) {
        const MDS_DevDriver_t *driver = ((MDS_DevModule_t *)device)->driver;
        if ((driver != NULL) && (driver->control != NULL)) {
            return (driver->control(device, cmd, args));
        }
    }

    return (MDS_EIO);
}

static MDS_FileOffset_t DEVFS_Read(MDS_FileDesc_t *fd, uint8_t *buff, MDS_FileSize_t len, MDS_FileOffset_t *pos)
{
    UNUSED(pos);

    if (fd->node->data == (MDS_Arg_t *)MDS_ObjectGetList(MDS_OBJECT_TYPE_DEVICE)) {
        UNUSED(buff);
        UNUSED(len);
        // read device list
        return (0);
    }

    return (-1);
}

static const MDS_FileSystemOps_t G_MDS_DEVFS_OPS = {
    .name = "devfs",
    .mount = DEVFS_Mount,
    .unmount = DEVFS_Unmount,
    .open = DEVFS_Open,
    .close = DEVFS_Close,
    .ioctl = DEVFS_Ioctl,
    .read = DEVFS_Read,
};
MDS_FILE_SYSTEM_EXPORT(G_MDS_DEVFS_OPS);
