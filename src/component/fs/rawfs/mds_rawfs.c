#include "mds_fs.h"
#include "dev_storage.h"

typedef struct MDS_RAWFS_FileSystem {
    size_t writeBlk;
    size_t writeOfs;
    size_t readBlk;
    size_t readOfs;

} MDS_RAWFS_FileSystem_t;

typedef struct MDS_RAWFS_FileDesc {
    MDS_RAWFS_FileSystem_t *fs;
    MDS_Mask_t flag;
} MDS_RAWFS_FileDesc_t;

typedef enum MDS_RAWFS_FileFlag {
    MDS_RAWFS_FLAG_POLL = 0x01U,
    MDS_RAWFS_FLAG_RPOP = 0x02U,
} MDS_RAWFS_FileFlag_t;

MDS_Err_t MDS_RAWFS_Mkfs()
{
}

MDS_Err_t MDS_RAWFS_Mount()
{
}

MDS_Err_t MDS_RAWFS_Unmount()
{
}

MDS_Err_t MDS_RAWFS_FileOpen()
{
}

MDS_Err_t MDS_RAWFS_FileClose()
{
}

MDS_Err_t MDS_RAWFS_FileSize(MDS_RAWFS_FileDesc_t *fd, size_t *used, size_t *size)
{
}

MDS_Err_t MDS_RAWFS_FileRead(MDS_RAWFS_FileDesc_t *fd, uint8_t *buff, size_t len)
{
}

MDS_Err_t MDS_RAWFS_FileWrite(MDS_RAWFS_FileDesc_t *fd, const uint8_t *buff, size_t len)
{
}
