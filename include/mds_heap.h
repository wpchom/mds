
/* Compiler ---------------------------------------------------------------- */
#if defined(__IAR_SYSTEMS_ICC__)

#define MDS_RETURN_ADDRESS() __get_return_address()

static inline uintptr_t MDS_SysHeapBegin(void)
{
#pragma section(".heap")
    const uintptr_t __HeapBase[] = __section_start(".heap");
    return ((uintptr_t)__HeapBase);
}

static inline uintptr_t MDS_SysHeapLimit(void)
{
#pragma section(".heap")
    const uintptr_t __HeapLimit[] = __section_end(".heap");
    return ((uintptr_t)__HeapLimit);
}

#else

#define MDS_RETURN_ADDRESS() __builtin_return_address(0)

static inline uintptr_t MDS_SysHeapBegin(void)
{
    extern void __HeapBase(void);
    return ((uintptr_t)__HeapBase);
}

static inline uintptr_t MDS_SysHeapLimit(void)
{
    extern void __HeapLimit(void);
    return ((uintptr_t)__HeapLimit);
}

#endif