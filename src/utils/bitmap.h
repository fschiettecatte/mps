/*****************************************************************************
*       Copyright (C) 1993-2011, FS Consulting LLC. All rights reserved      *
*                                                                            *
*  This notice is intended as a precaution against inadvertent publication   *
*  and does not constitute an admission or acknowledgement that publication  *
*  has occurred or constitute a waiver of confidentiality.                   *
*                                                                            *
*  This software is the proprietary and confidential property                *
*  of FS Consulting LLC.                                                     *
*****************************************************************************/


/*

    Module:     bitmap.h

    Author:     Francois Schiettecatte (FS Consulting LLC.)

    Created:    20 January 2005

    Purpose:    This file contain bitmap manipulation macros.

*/


/*---------------------------------------------------------------------------*/


#if !defined(UTL_BITMAP_H)
#define UTL_BITMAP_H


/*---------------------------------------------------------------------------*/


/*
** Includes
*/

#include "utils.h"


/*---------------------------------------------------------------------------*/


/* C++ wrapper */
#if defined(__cplusplus)
extern "C" {
#endif    /* defined(__cplusplus) */


/*---------------------------------------------------------------------------*/


/*
** =========================== 
** === Bitmap manipulation ===
** ===========================
*/


/* Note that it is much easier to shuffle bits to create a bit mask for
** testing bits than it is to use a 'case' statement or an 'if' statement
*/


/* Macros to access a bitmap in a ulong */

#define UTL_BITMAP_SET_BIT_IN_ULONG(ulMacroBitmap, uiMacroBit) \
    {   \
        ulMacroBitmap |= (1UL << (uiMacroBit)); \
    }


#define UTL_BITMAP_CLEAR_BIT_IN_ULONG(ulMacroBitmap, uiMacroBit) \
    {   \
        uiMacroBitmap |= (1UL << (uiMacroBit)); \
        uiMacroBitmap ^= (1UL << (uiMacroBit)); \
    }


#define UTL_BITMAP_IS_BIT_SET_IN_ULONG(ulMacroBitmap, uiMacroBit)           (ulMacroBitmap & (1UL << (uiMacroBit)))


#define UTL_BITMAP_CLEAR_ULONG(ulMacroBitmap)                               (ulMacroBitmap = 0)



/* Macros to access a bitmap in a uint */

#define UTL_BITMAP_SET_BIT_IN_UINT(uiMacroBitmap, uiMacroBit) \
    {   \
        uiMacroBitmap |= (1 << (uiMacroBit)); \
    }


#define UTL_BITMAP_CLEAR_BIT_IN_UINT(uiMacroBitmap, uiMacroBit) \
    {   \
        uiMacroBitmap |= (1 << (uiMacroBit)); \
        uiMacroBitmap ^= (1 << (uiMacroBit)); \
    }


#define UTL_BITMAP_IS_BIT_SET_IN_UINT(uiMacroBitmap, uiMacroBit)            (uiMacroBitmap & (1 << (uiMacroBit)))


#define UTL_BITMAP_CLEAR_UINT(uiMacroBitmap)                                (uiMacroBitmap = 0)



/* Macros to access a bitmap in a pointer */

#define UTL_BITMAP_GET_BITMAP_BYTE_LENGTH(uiMacroBitmapBitLength) \
    ((((uiMacroBitmapBitLength) - 1) / (sizeof(unsigned char) * 8)) + 1)


#define UTL_BITMAP_SET_BIT_IN_POINTER(pucMacroBitmap, uiMacroBit) \
    {  \
        ASSERT((pucMacroBitmap) != NULL); \
        unsigned char   *pucMacroBytePtr = (pucMacroBitmap) + ((uiMacroBit) / (sizeof(unsigned char) * 8)); \
        *pucMacroBytePtr |= (1 << ((uiMacroBit) % (sizeof(unsigned char) * 8))); \
    }


#define UTL_BITMAP_CLEAR_BIT_IN_POINTER(pucMacroBitmap, uiMacroBit) \
    {   \
        ASSERT((pucMacroBitmap) != NULL); \
        unsigned char   *pucMacroBytePtr = (pucMacroBitmap) + ((uiMacroBit) / (sizeof(unsigned char) * 8)); \
        unsigned int    uiMacroByteBit = (uiMacroBit) % (sizeof(unsigned char) * 8); \
        *pucMacroBytePtr |= (1 << uiMacroByteBit); \
        *pucMacroBytePtr ^= (1 << uiMacroByteBit); \
    }


#define UTL_BITMAP_IS_BIT_SET_IN_POINTER(pucMacroBitmap, uiMacroBit) \
    (*((pucMacroBitmap) + ((uiMacroBit) / (sizeof(unsigned char) * 8))) & (1 << ((uiMacroBit) % (sizeof(unsigned char) * 8))))


#define UTL_BITMAP_CLEAR_POINTER(pucMacroBitmap, uiMacroBitmapBitLength) \
    (s_memset(pucMacroBitmap, 0, UTL_BITMAP_GET_BITMAP_BYTE_LENGTH(uiMacroBitmapBitLength)))


#define UTL_BITMAP_AND_COMPARE_POINTERS(pucMacroBitmap1, pucMacroBitmap2, uiMacroBitmapBitLength, bMacroMatch) \
    {   \
        unsigned int    uiMacroI = 0; \
        unsigned char   *pucMacroBitmap1Ptr = pucMacroBitmap1; \
        unsigned char   *pucMacroBitmap2Ptr = pucMacroBitmap2; \
        unsigned int    uiMacroBitmapByteLength = UTL_BITMAP_GET_BITMAP_BYTE_LENGTH(uiMacroBitmapBitLength); \
        for ( uiMacroI = 0, bMacroMatch = false; uiMacroI < (uiMacroBitmapByteLength - 1); uiMacroI++, pucMacroBitmap1Ptr++, pucMacroBitmap2Ptr++ ) {   \
            if ( (*pucMacroBitmap1Ptr & *pucMacroBitmap2Ptr) > 0 ) {    \
                bMacroMatch = true; \
                break; \
            }   \
        }   \
        if ( (bMacroMatch == false) && ((uiMacroBitmapBitLength % 8) > 0) ) {   \
            /* pucMacroBitmap1Ptr and pucMacroBitmap2Ptr will be correctly set if we get here, as will bMacroMatch */   \
            for ( uiMacroI = 0; uiMacroI < (uiMacroBitmapBitLength % 8); uiMacroI++ ) { \
                if ( UTL_BITMAP_IS_BIT_SET_IN_UINT(*pucMacroBitmap1Ptr, uiMacroI) && UTL_BITMAP_IS_BIT_SET_IN_UINT(*pucMacroBitmap2Ptr, uiMacroI) ) {   \
                    bMacroMatch = true; \
                    break; \
                }   \
            }   \
        }   \
    }


#define UTL_BITMAP_OR_COMPARE_POINTERS(pucMacroBitmap1, pucMacroBitmap2, uiMacroBitmapBitLength, bMacroMatch) \
    {   \
        unsigned int    uiMacroI = 0; \
        unsigned char   *pucMacroBitmap1Ptr = pucMacroBitmap1; \
        unsigned char   *pucMacroBitmap2Ptr = pucMacroBitmap2; \
        unsigned int    uiMacroBitmapByteLength = UTL_BITMAP_GET_BITMAP_BYTE_LENGTH(uiMacroBitmapBitLength); \
        for ( uiMacroI = 0; uiMacroI < (uiMacroBitmapByteLength - 1); uiMacroI++, pucMacroBitmap1Ptr++, pucMacroBitmap2Ptr++ ) {    \
            if ( (*pucMacroBitmap1Ptr | *pucMacroBitmap2Ptr) > 0 ) {    \
                bMacroMatch = true; \
                break; \
            }   \
        }   \
        if ( (bMacroMatch == false) && ((uiMacroBitmapBitLength % 8) > 0) ) {   \
            /* pucMacroBitmap1Ptr and pucMacroBitmap2Ptr will be correctly set if we get here, as will bMacroMatch */   \
            for ( uiMacroI = 0, bMacroMatch = false; uiMacroI < (uiMacroBitmapBitLength % 8); uiMacroI++ ) {    \
                if ( UTL_BITMAP_IS_BIT_SET_IN_UINT(*pucMacroBitmap1Ptr, uiMacroI) || UTL_BITMAP_IS_BIT_SET_IN_UINT(*pucMacroBitmap2Ptr, uiMacroI) ) {   \
                    bMacroMatch = true; \
                    break; \
                }   \
            }   \
        }   \
    }


/*---------------------------------------------------------------------------*/


/* 
** C++ wrapper
*/

#if defined(__cplusplus)
}
#endif    /* defined(__cplusplus) */


/*---------------------------------------------------------------------------*/


#endif    /* !defined(UTL_BITMAP_H) */


/*---------------------------------------------------------------------------*/
