/*
 * Copyright (C) Huawei Technologies Co., Ltd. 2012-2015. All rights reserved.
 * foss@huawei.com
 *
 * If distributed as part of the Linux kernel, the following license terms
 * apply:
 *
 * * This program is free software; you can redistribute it and/or modify
 * * it under the terms of the GNU General Public License version 2 and
 * * only version 2 as published by the Free Software Foundation.
 * *
 * * This program is distributed in the hope that it will be useful,
 * * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * * GNU General Public License for more details.
 * *
 * * You should have received a copy of the GNU General Public License
 * * along with this program; if not, write to the Free Software
 * * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA
 *
 * Otherwise, the following license terms apply:
 *
 * * Redistribution and use in source and binary forms, with or without
 * * modification, are permitted provided that the following conditions
 * * are met:
 * * 1) Redistributions of source code must retain the above copyright
 * *    notice, this list of conditions and the following disclaimer.
 * * 2) Redistributions in binary form must reproduce the above copyright
 * *    notice, this list of conditions and the following disclaimer in the
 * *    documentation and/or other materials provided with the distribution.
 * * 3) Neither the name of Huawei nor the names of its contributors may
 * *    be used to endorse or promote products derived from this software
 * *    without specific prior written permission.
 *
 * * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */

/*
 * 版权所有（c）华为技术有限公司 2012-2019
 * 功能描述: OSA库函数头文件
 * 生成日期: 2006年10月3日
 */

#ifndef _V_MINILIB_H
#define _V_MINILIB_H

#include "dopra_def.h"
#include "vos_config.h"
#include "v_typdef.h"
#if (VOS_OS_VER == VOS_WIN32)
#include "string.h"
#endif

#if (VOS_RTOSCK == VOS_OS_VER)
#include "securec.h"
#endif

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cpluscplus */
#endif /* __cpluscplus */

/* errno definiens */
#define VOS_ERRNO_LIB_64DIV32_QUOTIENT_NULL 0x20290010
#define VOS_ERRNO_LIB_64DIV32_DIVIDE_ZERO 0x20290011
#define VOS_ERRNO_LIB_64DIV64_QUOTIENT_NULL 0x20290020
#define VOS_ERRNO_LIB_64DIV64_DIVIDE_ZERO 0x20290021
#define VOS_ERRNO_LIB_PRINTF_INPUTFORMATTOOLONG 0x20290001
#define VOS_ERRNO_LIB_PRINTF_UNKNOWNINTERERROR 0x20290002

#define VOS_ERRNO_LIB_64MUL32_QUOTIENT_NULL 0x20290025
#define VOS_ERRNO_LIB_64MUL32_OVERFLOW 0x20290026

#define VOS_SECUREC_MEM_MAX_LEN (0x7fffffff)

enum E_VOS_MEM_PT {
    STATIC_DOPRA_MEM_PT,
    DYNAMIC_DOPRA_MEM_PT,
    STATIC_MEM_PT,
    DYNAMIC_MEM_PT,
    BLOCK_MEM_PT,
    MEM_PT_BUTT
};

VOS_BOOL VOS_CharIsUpper(VOS_CHAR Char);

VOS_BOOL VOS_CharIsLower(VOS_CHAR Char);

VOS_CHAR VOS_CharToUpper(VOS_CHAR Char);

VOS_CHAR VOS_CharToLower(VOS_CHAR Char);

VOS_VOID VOS_StrToUpper(VOS_CHAR *str);

VOS_VOID VOS_StrToLower(VOS_CHAR *str);

VOS_INT8 VOS_StrNiCmp(const VOS_CHAR *str1, const VOS_CHAR *str2, VOS_SIZE_T length);

VOS_INT VOS_StrCmp(const VOS_CHAR *str1, const VOS_CHAR *str2);

VOS_UINT32 VOS_StrLen(const VOS_CHAR *str);

VOS_CHAR *VOS_StrStr(const VOS_CHAR *str1, const VOS_CHAR *str2);

VOS_CHAR *VOS_StrRChr(const VOS_CHAR *str, VOS_CHAR Char);

VOS_UINT32 VOS_StrNLen(const VOS_CHAR *str, VOS_UINT32 count);

/*lint -function(memcmp,VOS_MemCmp)*/
VOS_INT VOS_MemCmp(const VOS_VOID *dest, const VOS_VOID *src, VOS_SIZE_T count);

#if ((VOS_OSA_CPU == OSA_CPU_CCPU) && ((FEATURE_UE_MODE_CDMA == FEATURE_ON) || (FEATURE_UE_MODE_TDS == FEATURE_ON)))
/*lint -function(memset,V_MemSet)*/
VOS_VOID *V_MemSet_s(VOS_VOID *toSet, VOS_SIZE_T destSize, VOS_CHAR Char, VOS_SIZE_T count, VOS_UINT32 fileId,
                     VOS_INT32 lineNo);

#define VOS_MemSet_s(ToSet, destSize, Char, Count) \
    V_MemSet_s((ToSet), (destSize), (Char), (Count), VOS_FILE_ID, __LINE__)

/*lint -function(memcpy,V_MemCpy)*/
VOS_VOID *V_MemCpy_s(VOS_VOID *dest, VOS_SIZE_T destSize, const VOS_VOID *src, VOS_SIZE_T count, VOS_UINT32 fileId,
                     VOS_INT32 lineNo);

#define VOS_MemCpy_s(Dest, destSize, Src, Count) \
    V_MemCpy_s((Dest), (destSize), (Src), (Count), VOS_FILE_ID, __LINE__)

/*lint -function(memmove,V_MemMove)*/
VOS_VOID *V_MemMove_s(VOS_VOID *dest, VOS_SIZE_T destSize, const VOS_VOID *src, VOS_SIZE_T count, VOS_UINT32 fileId,
                      VOS_INT32 lineNo);

#define VOS_MemMove_s(Dest, destSize, Src, Count) \
    V_MemMove_s((Dest), (destSize), (Src), (Count), VOS_FILE_ID, __LINE__)
#endif

/*lint -function(memchr,VOS_MemChr)*/
VOS_VOID *VOS_MemChr(const VOS_VOID *buf, VOS_UCHAR c, VOS_INT n);

VOS_CHAR *VOS_StrTrim(VOS_CHAR *pcStr);

VOS_UINT32 VOS_64Div32(VOS_UINT32 dividendHigh, VOS_UINT32 dividendLow, VOS_UINT32 divisor, VOS_UINT32 *quotientHigh,
                       VOS_UINT32 *quotientLow, VOS_UINT32 *rtcRemainder);

VOS_UINT32 V_SetErrorNo(VOS_UINT32 errorNo, VOS_UINT32 fileId, VOS_INT32 lineNo);

#define VOS_SetErrorNo(ulErrorNo) V_SetErrorNo((ulErrorNo), VOS_FILE_ID, __LINE__)

VOS_UINT32 VOS_GetErrorNo(VOS_VOID);

void add64(VOS_UINT32 *low, VOS_UINT32 *high, VOS_UINT32 oldlow, VOS_UINT32 oldhigh);

VOS_UINT32 VOS_64Multi32(VOS_UINT32 multiplicandHigh, VOS_UINT32 multiplicandLow, VOS_UINT32 multiplicator,
                         VOS_UINT32 *productHigh, VOS_UINT32 *productLow);

#if (VOS_RTOSCK == VOS_OS_VER)
VOS_UINT32 VOS_CryptRand(VOS_UINT32 *randomData, VOS_UINT32 range);
#endif

VOS_UINT32 VOS_Rand(VOS_UINT32 range);

VOS_VOID VOS_SetSeed(VOS_UINT32 seed);

VOS_SIZE_T VOS_StrSpn(const VOS_CHAR *str1, const VOS_CHAR *str2);

VOS_CHAR *VOS_StrPbrk(const VOS_CHAR *str1, const VOS_CHAR *str2);

VOS_UINT32 V_CheckAllocedMem(const VOS_VOID *dest, VOS_SIZE_T destSize, VOS_UINT32 fileId, VOS_INT32 lineNo);

#define VOS_CheckAllocedMem(dest, destSize) V_CheckAllocedMem((dest), (destSize), VOS_FILE_ID, __LINE__)

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cpluscplus */
#endif /* __cpluscplus */

#endif /* _V_MINILIB_H */
