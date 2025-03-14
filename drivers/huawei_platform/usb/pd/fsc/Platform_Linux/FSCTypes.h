/*
 * FSCTypes.h
 *
 * FSCTypes driver
 *
 * Copyright (c) 2012-2020 Huawei Technologies Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */

#ifndef _FUSB30X_FSCTYPES_H_
#define _FUSB30X_FSCTYPES_H_

#define FSC_HOSTCOMM_BUFFER_SIZE    64  // Length of the hostcomm buffer, needed in both core and platform

#if defined(FSC_PLATFORM_LINUX)

/* Specify an extension for GCC based compilers */
#if defined(__GNUC__)
#define __EXTENSION __extension__
#else
#define __EXTENSION
#endif

#if !defined(__PACKED)
    #define __PACKED
#endif

/* get linux-specific type definitions (NULL, size_t, etc) */
#include <linux/types.h>

typedef _Bool FSC_BOOL;
#ifdef FALSE
#undef FALSE
#endif
#define FALSE (1 == 0)

#ifdef TRUE
#undef TRUE
#endif
#define TRUE (1 == 1)

#ifndef FSC_S8
typedef __s8                FSC_S8;                                            // 8-bit signed
#endif // FSC_S8

#ifndef FSC_S16
typedef __s16               FSC_S16;                                           // 16-bit signed
#endif // FSC_S16

#ifndef FSC_S32
typedef __s32               FSC_S32;                                           // 32-bit signed
#endif // FSC_S32

#ifndef FSC_S64
typedef __s64               FSC_S64;                                           // 64-bit signed
#endif // FSC_S64

#ifndef FSC_U8
typedef __u8                FSC_U8;                                            // 8-bit unsigned
#endif // FSC_U8

#ifndef FSC_U16
typedef __u16               FSC_U16;                                           // 16-bit unsigned
#endif // FSC_U16

#ifndef FSC_U32
typedef __u32               FSC_U32;                                           // 32-bit unsigned
#endif // FSC_U32

#undef __EXTENSION

#endif // FSC_PLATFORM_LINUX

#endif /* _FUSB30X_FSCTYPES_H_ */
