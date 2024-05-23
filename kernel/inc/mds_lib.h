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
#ifndef __MDS_LIB_H__
#define __MDS_LIB_H__

#endif /* __MDS_LIB_H__ */


/*
 * CMSIS Compiler Generic Header File
 */

#ifndef __CMSIS_COMPILER_H
#define __CMSIS_COMPILER_H

#include <stdint.h>

/*
 * Arm Compiler above 6.10.1 (armclang)
 */
#if defined (__ARMCC_VERSION) && (__ARMCC_VERSION >= 6100100)
  #if __ARM_ARCH_PROFILE == 'A'
    #include "./a-profile/cmsis_armclang_a.h"
  #elif __ARM_ARCH_PROFILE == 'R'
    #include "./r-profile/cmsis_armclang_r.h"
  #elif __ARM_ARCH_PROFILE == 'M'
    #include "./m-profile/cmsis_armclang_m.h"
  #else
    #error "Unknown Arm architecture profile"
  #endif

/*
 * TI Arm Clang Compiler (tiarmclang)
 */
#elif defined (__ti__)
  #if __ARM_ARCH_PROFILE == 'A'
    #error "Core-A is not supported for this compiler"
  #elif __ARM_ARCH_PROFILE == 'R'
    #error "Core-R is not supported for this compiler"
  #elif __ARM_ARCH_PROFILE == 'M'
    #include "m-profile/cmsis_tiarmclang_m.h"
  #else
    #error "Unknown Arm architecture profile"
  #endif


/*
 * LLVM/Clang Compiler
 */
#elif defined ( __clang__ )
  #if __ARM_ARCH_PROFILE == 'A'
    #include "a-profile/cmsis_clang_a.h"
  #elif __ARM_ARCH_PROFILE == 'R'
    #include "r-profile/cmsis_clang_r.h"
  #elif __ARM_ARCH_PROFILE == 'M'
    #include "m-profile/cmsis_clang_m.h"
  #else
    #error "Unknown Arm architecture profile"
  #endif


/*
 * GNU Compiler
 */
#elif defined ( __GNUC__ )
  #if __ARM_ARCH_PROFILE == 'A'
    #include "a-profile/cmsis_gcc_a.h"
  #elif __ARM_ARCH_PROFILE == 'R'
    #include "r-profile/cmsis_gcc_r.h"
  #elif __ARM_ARCH_PROFILE == 'M'
    #include "m-profile/cmsis_gcc_m.h"
  #else
    #error "Unknown Arm architecture profile"
  #endif


/*
 * IAR Compiler
 */
#elif defined ( __ICCARM__ )
  #if __ARM_ARCH_PROFILE == 'A'
    #include "a-profile/cmsis_iccarm_a.h"
  #elif __ARM_ARCH_PROFILE == 'R'
    #include "r-profile/cmsis_iccarm_r.h"
  #elif __ARM_ARCH_PROFILE == 'M'
    #include "m-profile/cmsis_iccarm_m.h"
  #else
    #error "Unknown Arm architecture profile"
  #endif


/*
 * TI Arm Compiler (armcl)
 */
#elif defined ( __TI_ARM__ )
  #include <cmsis_ccs.h>

  #ifndef   __ASM
    #define __ASM                                  __asm
  #endif
  #ifndef   __INLINE
    #define __INLINE                               inline
  #endif
  #ifndef   __STATIC_INLINE
    #define __STATIC_INLINE                        static inline
  #endif
  #ifndef   __STATIC_FORCEINLINE
    #define __STATIC_FORCEINLINE                   __STATIC_INLINE
  #endif
  #ifndef   __NO_RETURN
    #define __NO_RETURN                            __attribute__((noreturn))
  #endif
  #ifndef   __USED
    #define __USED                                 __attribute__((used))
  #endif
  #ifndef   __WEAK
    #define __WEAK                                 __attribute__((weak))
  #endif
  #ifndef   __PACKED
    #define __PACKED                               __attribute__((packed))
  #endif
  #ifndef   __PACKED_STRUCT
    #define __PACKED_STRUCT                        struct __attribute__((packed))
  #endif
  #ifndef   __PACKED_UNION
    #define __PACKED_UNION                         union __attribute__((packed))
  #endif
  #ifndef   __UNALIGNED_UINT16_WRITE
    __PACKED_STRUCT T_UINT16_WRITE { uint16_t v; };
    #define __UNALIGNED_UINT16_WRITE(addr, val)    (void)((((struct T_UINT16_WRITE *)(void*)(addr))->v) = (val))
  #endif
  #ifndef   __UNALIGNED_UINT16_READ
    __PACKED_STRUCT T_UINT16_READ { uint16_t v; };
    #define __UNALIGNED_UINT16_READ(addr)          (((const struct T_UINT16_READ *)(const void *)(addr))->v)
  #endif
  #ifndef   __UNALIGNED_UINT32_WRITE
    __PACKED_STRUCT T_UINT32_WRITE { uint32_t v; };
    #define __UNALIGNED_UINT32_WRITE(addr, val)    (void)((((struct T_UINT32_WRITE *)(void *)(addr))->v) = (val))
  #endif
  #ifndef   __UNALIGNED_UINT32_READ
    __PACKED_STRUCT T_UINT32_READ { uint32_t v; };
    #define __UNALIGNED_UINT32_READ(addr)          (((const struct T_UINT32_READ *)(const void *)(addr))->v)
  #endif
  #ifndef   __ALIGNED
    #define __ALIGNED(x)                           __attribute__((aligned(x)))
  #endif
  #ifndef   __RESTRICT
    #define __RESTRICT                             __restrict
  #endif
  #ifndef   __COMPILER_BARRIER
    #warning No compiler specific solution for __COMPILER_BARRIER. __COMPILER_BARRIER is ignored.
    #define __COMPILER_BARRIER()                   (void)0
  #endif
  #ifndef __NO_INIT
    #define __NO_INIT                              __attribute__ ((section (".noinit")))
  #endif
  #ifndef __ALIAS
    #define __ALIAS(x)                             __attribute__ ((alias(x)))
  #endif

/*
 * TASKING Compiler
 */
#elif defined ( __TASKING__ )
  /*
   * The CMSIS functions have been implemented as intrinsics in the compiler.
   * Please use "carm -?i" to get an up to date list of all intrinsics,
   * Including the CMSIS ones.
   */

  #ifndef   __ASM
    #define __ASM                                  __asm
  #endif
  #ifndef   __INLINE
    #define __INLINE                               inline
  #endif
  #ifndef   __STATIC_INLINE
    #define __STATIC_INLINE                        static inline
  #endif
  #ifndef   __STATIC_FORCEINLINE
    #define __STATIC_FORCEINLINE                   __STATIC_INLINE
  #endif
  #ifndef   __NO_RETURN
    #define __NO_RETURN                            __attribute__((noreturn))
  #endif
  #ifndef   __USED
    #define __USED                                 __attribute__((used))
  #endif
  #ifndef   __WEAK
    #define __WEAK                                 __attribute__((weak))
  #endif
  #ifndef   __PACKED
    #define __PACKED                               __packed__
  #endif
  #ifndef   __PACKED_STRUCT
    #define __PACKED_STRUCT                        struct __packed__
  #endif
  #ifndef   __PACKED_UNION
    #define __PACKED_UNION                         union __packed__
  #endif
  #ifndef   __UNALIGNED_UINT16_WRITE
    __PACKED_STRUCT T_UINT16_WRITE { uint16_t v; };
    #define __UNALIGNED_UINT16_WRITE(addr, val)    (void)((((struct T_UINT16_WRITE *)(void *)(addr))->v) = (val))
  #endif
  #ifndef   __UNALIGNED_UINT16_READ
    __PACKED_STRUCT T_UINT16_READ { uint16_t v; };
    #define __UNALIGNED_UINT16_READ(addr)          (((const struct T_UINT16_READ *)(const void *)(addr))->v)
  #endif
  #ifndef   __UNALIGNED_UINT32_WRITE
    __PACKED_STRUCT T_UINT32_WRITE { uint32_t v; };
    #define __UNALIGNED_UINT32_WRITE(addr, val)    (void)((((struct T_UINT32_WRITE *)(void *)(addr))->v) = (val))
  #endif
  #ifndef   __UNALIGNED_UINT32_READ
    __PACKED_STRUCT T_UINT32_READ { uint32_t v; };
    #define __UNALIGNED_UINT32_READ(addr)          (((const struct T_UINT32_READ *)(const void *)(addr))->v)
  #endif
  #ifndef   __ALIGNED
    #define __ALIGNED(x)                           __align(x)
  #endif
  #ifndef   __RESTRICT
    #warning No compiler specific solution for __RESTRICT. __RESTRICT is ignored.
    #define __RESTRICT
  #endif
  #ifndef   __COMPILER_BARRIER
    #warning No compiler specific solution for __COMPILER_BARRIER. __COMPILER_BARRIER is ignored.
    #define __COMPILER_BARRIER()                   (void)0
  #endif
  #ifndef __NO_INIT
    #define __NO_INIT                              __attribute__ ((section (".noinit")))
  #endif
  #ifndef __ALIAS
    #define __ALIAS(x)                             __attribute__ ((alias(x)))
  #endif

/*
 * COSMIC Compiler
 */
#elif defined ( __CSMC__ )
   #include <cmsis_csm.h>

 #ifndef   __ASM
    #define __ASM                                  _asm
  #endif
  #ifndef   __INLINE
    #define __INLINE                               inline
  #endif
  #ifndef   __STATIC_INLINE
    #define __STATIC_INLINE                        static inline
  #endif
  #ifndef   __STATIC_FORCEINLINE
    #define __STATIC_FORCEINLINE                   __STATIC_INLINE
  #endif
  #ifndef   __NO_RETURN
    // NO RETURN is automatically detected hence no warning here
    #define __NO_RETURN
  #endif
  #ifndef   __USED
    #warning No compiler specific solution for __USED. __USED is ignored.
    #define __USED
  #endif
  #ifndef   __WEAK
    #define __WEAK                                 __weak
  #endif
  #ifndef   __PACKED
    #define __PACKED                               @packed
  #endif
  #ifndef   __PACKED_STRUCT
    #define __PACKED_STRUCT                        @packed struct
  #endif
  #ifndef   __PACKED_UNION
    #define __PACKED_UNION                         @packed union
  #endif
  #ifndef   __UNALIGNED_UINT16_WRITE
    __PACKED_STRUCT T_UINT16_WRITE { uint16_t v; };
    #define __UNALIGNED_UINT16_WRITE(addr, val)    (void)((((struct T_UINT16_WRITE *)(void *)(addr))->v) = (val))
  #endif
  #ifndef   __UNALIGNED_UINT16_READ
    __PACKED_STRUCT T_UINT16_READ { uint16_t v; };
    #define __UNALIGNED_UINT16_READ(addr)          (((const struct T_UINT16_READ *)(const void *)(addr))->v)
  #endif
  #ifndef   __UNALIGNED_UINT32_WRITE
    __PACKED_STRUCT T_UINT32_WRITE { uint32_t v; };
    #define __UNALIGNED_UINT32_WRITE(addr, val)    (void)((((struct T_UINT32_WRITE *)(void *)(addr))->v) = (val))
  #endif
  #ifndef   __UNALIGNED_UINT32_READ
    __PACKED_STRUCT T_UINT32_READ { uint32_t v; };
    #define __UNALIGNED_UINT32_READ(addr)          (((const struct T_UINT32_READ *)(const void *)(addr))->v)
  #endif
  #ifndef   __ALIGNED
    #warning No compiler specific solution for __ALIGNED. __ALIGNED is ignored.
    #define __ALIGNED(x)
  #endif
  #ifndef   __RESTRICT
    #warning No compiler specific solution for __RESTRICT. __RESTRICT is ignored.
    #define __RESTRICT
  #endif
  #ifndef   __COMPILER_BARRIER
    #warning No compiler specific solution for __COMPILER_BARRIER. __COMPILER_BARRIER is ignored.
    #define __COMPILER_BARRIER()                   (void)0
  #endif
  #ifndef __NO_INIT
    #define __NO_INIT                              __attribute__ ((section (".noinit")))
  #endif
  #ifndef __ALIAS
    #define __ALIAS(x)                             __attribute__ ((alias(x)))
  #endif

#else
  #error Unknown compiler.
#endif


#endif /* __CMSIS_COMPILER_H */

