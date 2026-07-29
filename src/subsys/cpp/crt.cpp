#include "mds/sys.h"

void *operator new(size_t size)
{
    return (MDS_SysMemAlloc(size));
}

void *operator new[](size_t size)
{
    return (MDS_SysMemAlloc(size));
}

void operator delete(void *ptr)
{
    MDS_SysMemFree(ptr);
}

void operator delete[](void *ptr)
{
    MDS_SysMemFree(ptr);
}

void operator delete(void *ptr, size_t size) noexcept
{
    (void)size;
    MDS_SysMemFree(ptr);
}

void operator delete[](void *ptr, size_t size) noexcept
{
    (void)size;
    MDS_SysMemFree(ptr);
}
