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

    Module:     num.h

    Author:     Francois Schiettecatte (FS Consulting LLC.)

    Created:    20 February 2005

    Purpose:    This file contain number storage macros.

*/


/*---------------------------------------------------------------------------*/


#if !defined(UTL_NUM_H)
#define UTL_NUM_H


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
** Public defines
*/

#define UTL_NUM_ROUND_THRESHOLD                                     (3)         /* threshold where we kick in */
#define UTL_NUM_ROUND_ROUNDING                                      (1)         /* min # of zeros */
#define UTL_NUM_ROUND_DIGITS                                        (4)         /* max # of digits */


/*---------------------------------------------------------------------------*/


/*
** ========================================== 
** === Number storage macros (compressed) ===
** ==========================================
*/


/* This assumes that an unsigned integer is 32 bits
** and an unsigned long is 64 bits.
*/


/* Masks for compressed numbers storage */
#define UTL_NUM_COMPRESSED_CONTINUE_BIT                             (0x80)        /* 1000 0000 */
#define UTL_NUM_COMPRESSED_DATA_MASK                                (0x7F)        /* 0111 1111 */
#define UTL_NUM_COMPRESSED_DATA_BITS                                (7)


/* Macro for reading a compressed number from memory. The number is 
** read starting at pucMacroPtr and is stored in uiMacroValue
*/
/* Newer and faster */
#define UTL_NUM_READ_COMPRESSED_NUMBER(uiMacroValue, pucMacroPtr) \
    {    \
        ASSERT(pucMacroPtr != NULL); \
\
        for ( uiMacroValue = 0; uiMacroValue <<= UTL_NUM_COMPRESSED_DATA_BITS, uiMacroValue += (*pucMacroPtr & UTL_NUM_COMPRESSED_DATA_MASK), *pucMacroPtr & UTL_NUM_COMPRESSED_CONTINUE_BIT; pucMacroPtr++ ); \
\
        pucMacroPtr++; \
    }

/* Older and slower */
/* #define UTL_NUM_READ_COMPRESSED_NUMBER(uiMacroValue, pucMacroPtr) \ */
/*     {    \ */
/*         unsigned char ucMacroByte = '\0'; \ */
/* \ */
/*         ASSERT(pucMacroPtr != NULL); \ */
/* \ */
/*         uiMacroValue = 0; \ */
/* \ */
/*         do {     \ */
/*             uiMacroValue = uiMacroValue << UTL_NUM_COMPRESSED_DATA_BITS; \ */
/*             ucMacroByte = *(pucMacroPtr++); \ */
/*             uiMacroValue += (ucMacroByte & UTL_NUM_COMPRESSED_DATA_MASK); \ */
/*         }    \ */
/*         while ( ucMacroByte & UTL_NUM_COMPRESSED_CONTINUE_BIT ); \ */
/*     } */


/* Macro for skipping over a compressed number in memory  */
#define UTL_NUM_SKIP_COMPRESSED_NUMBER(pucMacroPtr) \
    {    \
        ASSERT(pucMacroPtr != NULL); \
\
        for ( ; *pucMacroPtr & UTL_NUM_COMPRESSED_CONTINUE_BIT; pucMacroPtr++ ); \
\
        pucMacroPtr++; \
    }




/* Macros for unsigned integer compression and storage */


/* Macro to get the number of bytes occupied by a compressed unsigned integer.
** The integer to evaluate should be set in uiMacroValue and the number of 
** bytes is placed in uiMacroSize
*/
#define UTL_NUM_GET_COMPRESSED_UINT_SIZE(uiMacroValue, uiMacroSize) \
    {    \
        ASSERT((uiMacroValue >= 0) && (uiMacroValue <= 0xFFFFFFFF)); \
\
        if ( uiMacroValue < (1UL << 7) ) {    \
            uiMacroSize = 1; \
        }    \
        else if ( uiMacroValue < (1UL << 14) ) {    \
            uiMacroSize = 2; \
        }    \
        else if ( uiMacroValue < (1UL << 21) ) {    \
            uiMacroSize = 3; \
        }    \
        else if ( uiMacroValue < (1UL << 28) ) {    \
            uiMacroSize = 4; \
        }    \
        else {    \
            uiMacroSize = 5; \
        }    \
    }


/* Macro for the maximum number of bytes occupied by a compressed unsigned integer  */
#define UTL_NUM_COMPRESSED_UINT_MAX_SIZE                            (5)


/* Macro to read a compressed unsigned int from memory */
#define UTL_NUM_READ_COMPRESSED_UINT(uiMacroValue, pucMacroPtr)     UTL_NUM_READ_COMPRESSED_NUMBER((uiMacroValue), (pucMacroPtr))


/* Macro to skip a compressed unsigned int in memory */
#define UTL_NUM_SKIP_COMPRESSED_UINT(pucMacroPtr)                   UTL_NUM_SKIP_COMPRESSED_NUMBER((pucMacroPtr))


/* Macro for compressing an unsigned integer and writing it to memory */
#define UTL_NUM_WRITE_COMPRESSED_UINT(uiMacroValue, pucMacroPtr) \
    {    \
        unsigned char ucMacroByte = '\0'; \
        unsigned int uiMacroLocalValue = uiMacroValue; \
        unsigned int uiMacroSize = 0; \
        unsigned int uiMacroBytesLeft = 0; \
\
        ASSERT(uiMacroValue >= 0); \
        ASSERT(pucMacroPtr != NULL); \
\
        UTL_NUM_GET_COMPRESSED_UINT_SIZE(uiMacroLocalValue, uiMacroSize); \
\
        for ( uiMacroBytesLeft = uiMacroSize; uiMacroBytesLeft > 0; uiMacroBytesLeft-- ) {     \
            ucMacroByte = (unsigned char)(uiMacroLocalValue & UTL_NUM_COMPRESSED_DATA_MASK); \
            if ( uiMacroBytesLeft != uiMacroSize )    { \
                ucMacroByte |= UTL_NUM_COMPRESSED_CONTINUE_BIT; \
            }    \
            pucMacroPtr[uiMacroBytesLeft - 1] = ucMacroByte; \
            uiMacroLocalValue >>= UTL_NUM_COMPRESSED_DATA_BITS; \
        }    \
\
        pucMacroPtr += uiMacroSize; \
        ASSERT(uiMacroLocalValue == 0); \
    }






/* Macros for unsigned long compression and storage */


/* Macro to get the number of bytes occupied by a compressed unsigned long.
** The integer to evaluate should be set in ulMacroValue and the number of bytes
** is placed  in uiMacroSize.
*/
#define UTL_NUM_GET_COMPRESSED_ULONG_SIZE(ulMacroValue, uiMacroSize) \
    {    \
        ASSERT((ulMacroValue >= 0) && (ulMacroValue <= 0xFFFFFFFFFFFF)); \
\
        if ( ulMacroValue < (1UL << 7) ) {    \
            uiMacroSize = 1; \
        }    \
        else if ( ulMacroValue < (1UL << 14) ) {    \
            uiMacroSize = 2; \
        }    \
        else if ( ulMacroValue < (1UL << 21) ) {    \
            uiMacroSize = 3; \
        }    \
        else if ( ulMacroValue < (1UL << 28) ) {    \
            uiMacroSize = 4; \
        }    \
        else if ( ulMacroValue < (1UL << 35) ) {    \
            uiMacroSize = 5; \
        }    \
        else if ( ulMacroValue < (1UL << 42) ) {    \
            uiMacroSize = 6; \
        }    \
        else if ( ulMacroValue < (1UL << 49) ) {    \
            uiMacroSize = 7; \
        }    \
        else if ( ulMacroValue < (1UL << 56) ) {    \
            uiMacroSize = 8; \
        }    \
        else if ( ulMacroValue < (1UL << 63) ) {    \
            uiMacroSize = 9; \
        }    \
        else {    \
            uiMacroSize = 10; \
        }    \
    }


/* Macro to get the number of bytes occupied by a compressed unsigned long */
#define UTL_NUM_COMPRESSED_ULONG_MAX_SIZE                           (10)


/* Macro to read a compressed unsigned long from memory */
#define UTL_NUM_READ_COMPRESSED_ULONG(ulMacroValue, pucMacroPtr)    UTL_NUM_READ_COMPRESSED_NUMBER((ulMacroValue), (pucMacroPtr))


/* Macro to skip a compressed unsigned long in memory */
#define UTL_NUM_SKIP_COMPRESSED_ULONG(pucMacroPtr)                   UTL_NUM_SKIP_COMPRESSED_NUMBER((pucMacroPtr))


/* Macro for compressing an unsigned long and writing it to memory */
#define UTL_NUM_WRITE_COMPRESSED_ULONG(ulMacroValue, pucMacroPtr) \
    {    \
        unsigned char ucMacroByte = '\0'; \
        unsigned long ulMacroLocalValue = ulMacroValue; \
        unsigned int uiMacroSize = 0; \
        unsigned int uiMacroBytesLeft = 0; \
\
        ASSERT(ulMacroValue >= 0); \
        ASSERT(pucMacroPtr != NULL); \
\
        UTL_NUM_GET_COMPRESSED_ULONG_SIZE(ulMacroLocalValue, uiMacroSize); \
\
        for ( uiMacroBytesLeft = uiMacroSize; uiMacroBytesLeft > 0; uiMacroBytesLeft-- ) {     \
            ucMacroByte = (unsigned char)(ulMacroLocalValue & UTL_NUM_COMPRESSED_DATA_MASK); \
            if ( uiMacroBytesLeft != uiMacroSize )    { \
                ucMacroByte |= UTL_NUM_COMPRESSED_CONTINUE_BIT; \
            }    \
            pucMacroPtr[uiMacroBytesLeft - 1] = ucMacroByte; \
            ulMacroLocalValue >>= UTL_NUM_COMPRESSED_DATA_BITS; \
        }    \
\
        pucMacroPtr += uiMacroSize; \
        ASSERT(ulMacroLocalValue == 0); \
    }







/* Macros for off_t compression and storage */


/* Macro to get the number of bytes occupied by a compressed off_t */
#define UTL_NUM_GET_COMPRESSED_OFF_T_SIZE(zMacroValue, uiMacroSize) UTL_NUM_GET_COMPRESSED_ULONG_SIZE((zMacroValue), (uiMacroSize))
#define UTL_NUM_COMPRESSED_OFF_T_MAX_SIZE                           (10)


/* Macro to skip a compressed off_t in memory */
#define UTL_NUM_SKIP_COMPRESSED_OFF_T(pucMacroPtr)                  UTL_NUM_SKIP_COMPRESSED_NUMBER((pucMacroPtr))


/* Macro to read a compressed off_t from memory */
#define UTL_NUM_READ_COMPRESSED_OFF_T(zMacroValue, pucMacroPtr)     UTL_NUM_READ_COMPRESSED_NUMBER((zMacroValue), (pucMacroPtr))


/* Macro for compressing an off_t and writing it to memory */
#define UTL_NUM_WRITE_COMPRESSED_OFF_T(zMacroValue, pucMacroPtr) \
    {    \
        unsigned char ucMacroByte = '\0'; \
        off_t zLocalMacroValue = zMacroValue; \
        unsigned int uiMacroSize = 0; \
        unsigned int uiMacroBytesLeft = 0; \
\
        ASSERT(zMacroValue >= 0); \
        ASSERT(pucMacroPtr != NULL); \
\
        UTL_NUM_GET_COMPRESSED_OFF_T_SIZE(zMacroValue, uiMacroSize); \
\
        for ( uiMacroBytesLeft = uiMacroSize; uiMacroBytesLeft > 0; uiMacroBytesLeft-- ) {     \
            ucMacroByte = (unsigned char)(zLocalMacroValue & UTL_NUM_COMPRESSED_DATA_MASK); \
            if ( uiMacroBytesLeft != uiMacroSize )    { \
                ucMacroByte |= UTL_NUM_COMPRESSED_CONTINUE_BIT; \
            }    \
            pucMacroPtr[uiMacroBytesLeft - 1] = ucMacroByte; \
            zLocalMacroValue >>= UTL_NUM_COMPRESSED_DATA_BITS; \
        }    \
\
        pucMacroPtr += uiMacroSize; \
        ASSERT(zLocalMacroValue == 0); \
    }






/* Macros for float and double compression and storage */ 


/* Macro for the number of bytes occupied by a compressed float */
#define UTL_NUM_COMPRESSED_FLOAT_SIZE                (sizeof(float))

/* Macro to read a compressed float from memory */
#define UTL_NUM_READ_COMPRESSED_FLOAT(fMacroValue, pucMacroPtr) \
    {    \
        unsigned int uiMacroValue = 0; \
        UTL_NUM_READ_UINT(uiMacroValue, sizeof(float), pucMacroPtr); \
        fMacroValue = *((float *)&uiMacroValue); \
    }

/* Macro for compressing a float and writing it to memory */
#define UTL_NUM_WRITE_COMPRESSED_FLOAT(fMacroValue, pucMacroPtr) \
    {    \
        unsigned int uiMacroValue = *((unsigned int *)&fMacroValue); \
        UTL_NUM_WRITE_UINT(uiMacroValue, sizeof(float), pucMacroPtr); \
    }



/* Macro for the number of bytes occupied by a compressed double */
#define UTL_NUM_COMPRESSED_DOUBLE_SIZE            (sizeof(double))

/* Macro to read a compressed double from memory */
#define UTL_NUM_READ_COMPRESSED_DOUBLE(dMacroValue, pucMacroPtr) \
    {    \
        unsigned long ulMacroValue = 0; \
        UTL_NUM_READ_ULONG(ulMacroValue, sizeof(double), pucMacroPtr); \
        dMacroValue = *((double *)&ulMacroValue); \
    }

/* Macro for compressing a double and writing it to memory */
#define UTL_NUM_WRITE_COMPRESSED_DOUBLE(dMacroValue, pucMacroPtr) \
    {    \
        unsigned long ulMacroValue = *((unsigned long *)&dMacroValue); \
        UTL_NUM_WRITE_ULONG(ulMacroValue, sizeof(double), pucMacroPtr); \
    }


/*---------------------------------------------------------------------------*/


/*
** ============================================ 
** === Number storage macros (uncompressed) ===
** ============================================
*/


/* This assumes that an unsigned integer is 32 bits
** and an unsigned long is 64 bits.
*/



/* Masks for numbers storage */
#define UTL_NUM_DATA_MASK                                           (0xFF)        /* 1111 1111 */
#define UTL_NUM_DATA_BITS                                           (8)



/* Macros for unsigned integer storage */


/* Macro to get the number of bytes occupied by an unsigned integer. 
** The integer to evaluate should be set in uiMacroValue and the number 
** of bytes is placed in uiMacroSize
*/
#define UTL_NUM_GET_UINT_SIZE(uiMacroValue, uiMacroSize) \
    {    \
        ASSERT((uiMacroValue >= 0) && (uiMacroValue <= 0xFFFFFFFF)); \
\
        if ( uiMacroValue < (1UL << 8) ) {    \
            uiMacroSize = 1; \
        }    \
        else if ( uiMacroValue < (1UL << 16) ) {    \
            uiMacroSize = 2; \
        }    \
        else if ( uiMacroValue < (1UL << 24) ) {    \
            uiMacroSize = 3; \
        }    \
        else {    \
            uiMacroSize = 4; \
        }    \
    }


/* Macro for the maximum number of bytes occupied by an unsigned integer  */
#define UTL_NUM_UINT_MAX_SIZE                                       (4)


/* Macro for reading an unsigned integer from memory. The integer is read starting
** at pucMacroPtr, for uiMacroSize bytes, and is stored in uiMacroValue
*/
/* Newer and much faster if the pipeline can be kept filled */
#define UTL_NUM_READ_UINT(uiMacroValue, uiMacroSize, pucMacroPtr) \
    {    \
        ASSERT((uiMacroSize > 0) && (uiMacroSize <= UTL_NUM_UINT_MAX_SIZE)); \
        ASSERT(pucMacroPtr != NULL); \
\
        uiMacroValue = 0; \
\
        switch ( uiMacroSize ) {    \
            case 4: uiMacroValue <<= UTL_NUM_DATA_BITS; uiMacroValue += *pucMacroPtr; pucMacroPtr++; \
            case 3: uiMacroValue <<= UTL_NUM_DATA_BITS; uiMacroValue += *pucMacroPtr; pucMacroPtr++; \
            case 2: uiMacroValue <<= UTL_NUM_DATA_BITS; uiMacroValue += *pucMacroPtr; pucMacroPtr++; \
            case 1: uiMacroValue <<= UTL_NUM_DATA_BITS; uiMacroValue += *pucMacroPtr; pucMacroPtr++; \
        }    \
    }

/* Older and slower */
/* #define UTL_NUM_READ_UINT(uiMacroValue, uiMacroSize, pucMacroPtr) \ */
/*     {    \ */
/*         unsigned char *pucMacroEndPtr = pucMacroPtr + uiMacroSize - 1; \ */
/* \ */
/*         ASSERT(uiMacroSize > 0); \ */
/*         ASSERT(pucMacroPtr != NULL); \ */
/* \ */
/*         for ( uiMacroValue = 0; uiMacroValue <<= UTL_NUM_DATA_BITS, uiMacroValue += *pucMacroPtr, pucMacroPtr < pucMacroEndPtr; pucMacroPtr++ ); \ */
/* \ */
/*         pucMacroPtr++; \ */
/*     } */


/* Macro for writing an unsigned integer to memory */
#define UTL_NUM_WRITE_UINT(uiMacroValue, uiMacroSize, pucMacroPtr) \
    {    \
        unsigned int uiMacroI = 0; \
        unsigned int uiMacroLocalValue = uiMacroValue; \
        /* int iMacroLocalByteCount = 0; */    \
\
        ASSERT(uiMacroValue >= 0); \
        ASSERT((uiMacroSize > 0) && (uiMacroSize <= UTL_NUM_UINT_MAX_SIZE)); \
        ASSERT(pucMacroPtr != NULL); \
\
        /* UTL_NUM_GET_UINT_SIZE(uiMacroLocalValue, iMacroLocalByteCount); */    \
        /* ASSERT(iMacroLocalByteCount <= uiMacroSize); */    \
\
        for ( uiMacroI = 0, pucMacroPtr += uiMacroSize; uiMacroI < uiMacroSize; uiMacroI++ ) {    \
            pucMacroPtr--; \
            *pucMacroPtr = (unsigned char)(uiMacroLocalValue & UTL_NUM_DATA_MASK); \
            uiMacroLocalValue >>= UTL_NUM_DATA_BITS; \
        }    \
\
        ASSERT(uiMacroLocalValue == 0); \
        pucMacroPtr += uiMacroSize; \
    }





/* Macros for unsigned long storage */


/* Macro to get the number of bytes occupied by an unsigned long storage. 
** The integer to evaluate should be set in ulMacroValue and the number 
** of bytes is placed in uiMacroSize
*/
#define UTL_NUM_GET_ULONG_SIZE(ulMacroValue, uiMacroSize) \
    {    \
        ASSERT((uiMacroValue >= 0) && (uiMacroValue <= 0xFFFFFFFFFFFF)); \
\
        if ( ulMacroValue < (1UL << 8) ) {    \
            uiMacroSize = 1; \
        }    \
        else if ( ulMacroValue < (1UL << 16) ) {    \
            uiMacroSize = 2; \
        }    \
        else if ( ulMacroValue < (1UL << 24) ) {    \
            uiMacroSize = 3; \
        }    \
        else if ( ulMacroValue < (1UL << 32) ) {    \
            uiMacroSize = 4; \
        }    \
        else if ( ulMacroValue < (1UL << 40) ) {    \
            uiMacroSize = 5; \
        }    \
        else if ( ulMacroValue < (1UL << 48) ) {    \
            uiMacroSize = 6; \
        }    \
        else if ( ulMacroValue < (1UL << 56) ) {    \
            uiMacroSize = 7; \
        }    \
        else {    \
            uiMacroSize = 8; \
        }    \
    }




/* Macro for the maximum number of bytes occupied by an unsigned long  */
#define UTL_NUM_ULONG_MAX_SIZE                                      (8)


/* Macro for reading an unsigned long from memory. The unsigned long is read starting
** at pucMacroPtr, for uiMacroSize bytes, and is stored in ulMacroValue
*/
/* Newer and much faster if the pipeline can be kept filled */
#define UTL_NUM_READ_ULONG(ulMacroValue, uiMacroSize, pucMacroPtr) \
    {    \
        ASSERT((uiMacroSize > 0) && (uiMacroSize <= UTL_NUM_ULONG_MAX_SIZE)); \
        ASSERT(pucMacroPtr != NULL); \
\
        ulMacroValue = 0; \
\
        switch ( uiMacroSize ) {    \
            case 8: ulMacroValue <<= UTL_NUM_DATA_BITS; ulMacroValue += *pucMacroPtr; pucMacroPtr++; \
            case 7: ulMacroValue <<= UTL_NUM_DATA_BITS; ulMacroValue += *pucMacroPtr; pucMacroPtr++; \
            case 6: ulMacroValue <<= UTL_NUM_DATA_BITS; ulMacroValue += *pucMacroPtr; pucMacroPtr++; \
            case 5: ulMacroValue <<= UTL_NUM_DATA_BITS; ulMacroValue += *pucMacroPtr; pucMacroPtr++; \
            case 4: ulMacroValue <<= UTL_NUM_DATA_BITS; ulMacroValue += *pucMacroPtr; pucMacroPtr++; \
            case 3: ulMacroValue <<= UTL_NUM_DATA_BITS; ulMacroValue += *pucMacroPtr; pucMacroPtr++; \
            case 2: ulMacroValue <<= UTL_NUM_DATA_BITS; ulMacroValue += *pucMacroPtr; pucMacroPtr++; \
            case 1: ulMacroValue <<= UTL_NUM_DATA_BITS; ulMacroValue += *pucMacroPtr; pucMacroPtr++; \
        }    \
    }

/* Older and slower */
/* #define UTL_NUM_READ_ULONG(ulMacroValue, uiMacroSize, pucMacroPtr) \ */
/*     {    \ */
/*         unsigned char *pucMacroEndPtr = pucMacroPtr + uiMacroSize - 1; \ */
/* \ */
/*         ASSERT(uiMacroSize > 0); \ */
/*         ASSERT(pucMacroPtr != NULL); \ */
/* \ */
/*         for ( ulMacroValue = 0; ulMacroValue <<= UTL_NUM_DATA_BITS, ulMacroValue += *pucMacroPtr, pucMacroPtr < pucMacroEndPtr; pucMacroPtr++ ); \ */
/* \ */
/*         pucMacroPtr++; \ */
/*     } */


/* Macro for writing an unsigned long to memory */
#define UTL_NUM_WRITE_ULONG(ulMacroValue, uiMacroSize, pucMacroPtr) \
    {    \
        unsigned int uiMacroI = 0; \
        unsigned long ulMacroLocalValue = ulMacroValue; \
        /* int iMacroLocalByteCount = 0; */     \
\
        ASSERT(ulMacroValue >= 0); \
        ASSERT((uiMacroSize > 0) && (uiMacroSize <= UTL_NUM_ULONG_MAX_SIZE)); \
        ASSERT(pucMacroPtr != NULL); \
\
        /* UTL_NUM_GET_ULONG_SIZE(ulMacroLocalValue, iMacroLocalByteCount); */ \
        /* ASSERT(iMacroLocalByteCount <= uiMacroSize); */    \
\
        for ( uiMacroI = 0, pucMacroPtr += uiMacroSize; uiMacroI < uiMacroSize; uiMacroI++ ) {    \
            pucMacroPtr--; \
            *pucMacroPtr = (unsigned char)(ulMacroLocalValue & UTL_NUM_DATA_MASK); \
            ulMacroLocalValue >>= UTL_NUM_DATA_BITS; \
        }    \
\
        ASSERT(ulMacroLocalValue == 0); \
        pucMacroPtr += uiMacroSize; \
    }






/* Macros for off_t storage */

/* Macro to get the number of bytes occupied by an off_t */
#define UTL_NUM_GET_OFF_T_SIZE(zMacroValue, uiMacroSize)            UTL_NUM_GET_ULONG_SIZE((zMacroValue), (uiMacroSize))



/* Macro for the maximum number of bytes occupied by off_t */
#define UTL_NUM_OFF_T_MAX_SIZE                                      (_FILE_OFFSET_BITS / 8)


/* Macro to read an off_t from memory */
#define UTL_NUM_READ_OFF_T(zMacroValue, uiMacroSize, pucMacroPtr)   UTL_NUM_READ_ULONG((zMacroValue), (uiMacroSize), (pucMacroPtr))


/* Macro for writing an off_t to memory */
#define UTL_NUM_WRITE_OFF_T(zMacroValue, uiMacroSize, pucMacroPtr) \
    {    \
        unsigned int uiMacroI = 0; \
        off_t zMacroLocalValue = zMacroValue; \
        /* int iMacroLocalByteCount = 0; */     \
\
        ASSERT(zMacroValue >= 0); \
        ASSERT(uiMacroSize > 0); \
        ASSERT(pucMacroPtr != NULL); \
\
        /* UTL_NUM_GET_OFF_T_SIZE(zMacroValue, iMacroLocalByteCount); */ \
        /* ASSERT(iMacroLocalByteCount <= uiMacroSize); */    \
\
        for ( uiMacroI = 0, pucMacroPtr += uiMacroSize; uiMacroI < uiMacroSize; uiMacroI++ ) {    \
            pucMacroPtr--; \
            *pucMacroPtr = (unsigned char)(zMacroLocalValue & UTL_NUM_DATA_MASK); \
            zMacroLocalValue >>= UTL_NUM_DATA_BITS; \
        }    \
\
        ASSERT(zMacroLocalValue == 0); \
        pucMacroPtr += uiMacroSize; \
    }





/* Macros for float and double storage */ 

/* Macro for the number of bytes occupied by a float */
#define UTL_NUM_FLOAT_SIZE                                          (UTL_NUM_COMPRESSED_FLOAT_SIZE)

/* Macro to read a float from memory */
#define UTL_NUM_READ_FLOAT(fMacroValue, pucMacroPtr)                UTL_NUM_READ_COMPRESSED_FLOAT((fMacroValue), (pucMacroPtr))

/* Macro for writing a float to memory */
#define UTL_NUM_WRITE_FLOAT(fMacroValue, pucMacroPtr)               UTL_NUM_WRITE_COMPRESSED_FLOAT((fMacroValue), (pucMacroPtr))



/* Macro for the number of bytes occupied by a double */
#define UTL_NUM_DOUBLE_SIZE                                         (UTL_NUM_COMPRESSED_DOUBLE_SIZE)

/* Macro to read a double from memory */
#define UTL_NUM_READ_DOUBLE(dMacroValue, pucMacroPtr)               UTL_NUM_READ_COMPRESSED_DOUBLE((dMacroValue), (pucMacroPtr))

/* Macro for writing a double to memory */
#define UTL_NUM_WRITE_DOUBLE(dMacroValue, pucMacroPtr)              UTL_NUM_WRITE_COMPRESSED_DOUBLE((dMacroValue), (pucMacroPtr))


/*---------------------------------------------------------------------------*/


#if defined(NOT_USED)


/*
** ====================================== 
** === Number storage macros (varint) ===
** ======================================
*/


/* Masks used to mask out a varint in memory */
static unsigned int     uiVarintMaskGlobal[] = {0x0, 0xFF, 0xFFFF, 0xFFFFFF, 0xFFFFFFFF};


/* Structure to store the byte size of each varint */
struct varintSize {
    unsigned char   ucSize1;
    unsigned char   ucSize2;
    unsigned char   ucSize3;
    unsigned char   ucSize4;
};


/* Structure which tells us the byte size of each varint based on the offset */
static struct varintSize pvsVarintSizesGlobal[] = 
{
    /*   0 - 00 00 00 00 */    {1,    1,    1,    1},        /*   1 - 00 00 00 01 */    {1,    1,    1,    2},        /*   2 - 00 00 00 10 */    {1,    1,    1,    3},        /*   3 - 00 00 00 11 */    {1,    1,    1,    4},
    /*   4 - 00 00 01 00 */    {1,    1,    2,    1},        /*   5 - 00 00 01 01 */    {1,    1,    2,    2},        /*   6 - 00 00 01 10 */    {1,    1,    2,    3},        /*   7 - 00 00 01 11 */    {1,    1,    2,    4},
    /*   8 - 00 00 10 00 */    {1,    1,    3,    1},        /*   9 - 00 00 10 01 */    {1,    1,    3,    2},        /*  10 - 00 00 10 10 */    {1,    1,    3,    3},        /*  11 - 00 00 10 11 */    {1,    1,    3,    4},
    /*  12 - 00 00 11 00 */    {1,    1,    4,    1},        /*  13 - 00 00 11 01 */    {1,    1,    4,    2},        /*  14 - 00 00 11 10 */    {1,    1,    4,    3},        /*  15 - 00 00 11 11 */    {1,    1,    4,    4},
    /*  16 - 00 01 00 00 */    {1,    2,    1,    1},        /*  17 - 00 01 00 01 */    {1,    2,    1,    2},        /*  18 - 00 01 00 10 */    {1,    2,    1,    3},        /*  19 - 00 01 00 11 */    {1,    2,    1,    4},
    /*  20 - 00 01 01 00 */    {1,    2,    2,    1},        /*  21 - 00 01 01 01 */    {1,    2,    2,    2},        /*  22 - 00 01 01 10 */    {1,    2,    2,    3},        /*  23 - 00 01 01 11 */    {1,    2,    2,    4},
    /*  24 - 00 01 10 00 */    {1,    2,    3,    1},        /*  25 - 00 01 10 01 */    {1,    2,    3,    2},        /*  26 - 00 01 10 10 */    {1,    2,    3,    3},        /*  27 - 00 01 10 11 */    {1,    2,    3,    4},
    /*  28 - 00 01 11 00 */    {1,    2,    4,    1},        /*  29 - 00 01 11 01 */    {1,    2,    4,    2},        /*  30 - 00 01 11 10 */    {1,    2,    4,    3},        /*  31 - 00 01 11 11 */    {1,    2,    4,    4},
    /*  32 - 00 10 00 00 */    {1,    3,    1,    1},        /*  33 - 00 10 00 01 */    {1,    3,    1,    2},        /*  34 - 00 10 00 10 */    {1,    3,    1,    3},        /*  35 - 00 10 00 11 */    {1,    3,    1,    4},
    /*  36 - 00 10 01 00 */    {1,    3,    2,    1},        /*  37 - 00 10 01 01 */    {1,    3,    2,    2},        /*  38 - 00 10 01 10 */    {1,    3,    2,    3},        /*  39 - 00 10 01 11 */    {1,    3,    2,    4},
    /*  40 - 00 10 10 00 */    {1,    3,    3,    1},        /*  41 - 00 10 10 01 */    {1,    3,    3,    2},        /*  42 - 00 10 10 10 */    {1,    3,    3,    3},        /*  43 - 00 10 10 11 */    {1,    3,    3,    4},
    /*  44 - 00 10 11 00 */    {1,    3,    4,    1},        /*  45 - 00 10 11 01 */    {1,    3,    4,    2},        /*  46 - 00 10 11 10 */    {1,    3,    4,    3},        /*  47 - 00 10 11 11 */    {1,    3,    4,    4},
    /*  48 - 00 11 00 00 */    {1,    4,    1,    1},        /*  49 - 00 11 00 01 */    {1,    4,    1,    2},        /*  50 - 00 11 00 10 */    {1,    4,    1,    3},        /*  51 - 00 11 00 11 */    {1,    4,    1,    4},
    /*  52 - 00 11 01 00 */    {1,    4,    2,    1},        /*  53 - 00 11 01 01 */    {1,    4,    2,    2},        /*  54 - 00 11 01 10 */    {1,    4,    2,    3},        /*  55 - 00 11 01 11 */    {1,    4,    2,    4},
    /*  56 - 00 11 10 00 */    {1,    4,    3,    1},        /*  57 - 00 11 10 01 */    {1,    4,    3,    2},        /*  58 - 00 11 10 10 */    {1,    4,    3,    3},        /*  59 - 00 11 10 11 */    {1,    4,    3,    4},
    /*  60 - 00 11 11 00 */    {1,    4,    4,    1},        /*  61 - 00 11 11 01 */    {1,    4,    4,    2},        /*  62 - 00 11 11 10 */    {1,    4,    4,    3},        /*  63 - 00 11 11 11 */    {1,    4,    4,    4},
    /*  64 - 01 00 00 00 */    {2,    1,    1,    1},        /*  65 - 01 00 00 01 */    {2,    1,    1,    2},        /*  66 - 01 00 00 10 */    {2,    1,    1,    3},        /*  67 - 01 00 00 11 */    {2,    1,    1,    4},
    /*  68 - 01 00 01 00 */    {2,    1,    2,    1},        /*  69 - 01 00 01 01 */    {2,    1,    2,    2},        /*  70 - 01 00 01 10 */    {2,    1,    2,    3},        /*  71 - 01 00 01 11 */    {2,    1,    2,    4},
    /*  72 - 01 00 10 00 */    {2,    1,    3,    1},        /*  73 - 01 00 10 01 */    {2,    1,    3,    2},        /*  74 - 01 00 10 10 */    {2,    1,    3,    3},        /*  75 - 01 00 10 11 */    {2,    1,    3,    4},
    /*  76 - 01 00 11 00 */    {2,    1,    4,    1},        /*  77 - 01 00 11 01 */    {2,    1,    4,    2},        /*  78 - 01 00 11 10 */    {2,    1,    4,    3},        /*  79 - 01 00 11 11 */    {2,    1,    4,    4},
    /*  80 - 01 01 00 00 */    {2,    2,    1,    1},        /*  81 - 01 01 00 01 */    {2,    2,    1,    2},        /*  82 - 01 01 00 10 */    {2,    2,    1,    3},        /*  83 - 01 01 00 11 */    {2,    2,    1,    4},
    /*  84 - 01 01 01 00 */    {2,    2,    2,    1},        /*  85 - 01 01 01 01 */    {2,    2,    2,    2},        /*  86 - 01 01 01 10 */    {2,    2,    2,    3},        /*  87 - 01 01 01 11 */    {2,    2,    2,    4},
    /*  88 - 01 01 10 00 */    {2,    2,    3,    1},        /*  89 - 01 01 10 01 */    {2,    2,    3,    2},        /*  90 - 01 01 10 10 */    {2,    2,    3,    3},        /*  91 - 01 01 10 11 */    {2,    2,    3,    4},
    /*  92 - 01 01 11 00 */    {2,    2,    4,    1},        /*  93 - 01 01 11 01 */    {2,    2,    4,    2},        /*  94 - 01 01 11 10 */    {2,    2,    4,    3},        /*  95 - 01 01 11 11 */    {2,    2,    4,    4},
    /*  96 - 01 10 00 00 */    {2,    3,    1,    1},        /*  97 - 01 10 00 01 */    {2,    3,    1,    2},        /*  98 - 01 10 00 10 */    {2,    3,    1,    3},        /*  99 - 01 10 00 11 */    {2,    3,    1,    4},
    /* 100 - 01 10 01 00 */    {2,    3,    2,    1},        /* 101 - 01 10 01 01 */    {2,    3,    2,    2},        /* 102 - 01 10 01 10 */    {2,    3,    2,    3},        /* 103 - 01 10 01 11 */    {2,    3,    2,    4},
    /* 104 - 01 10 10 00 */    {2,    3,    3,    1},        /* 105 - 01 10 10 01 */    {2,    3,    3,    2},        /* 106 - 01 10 10 10 */    {2,    3,    3,    3},        /* 107 - 01 10 10 11 */    {2,    3,    3,    4},
    /* 108 - 01 10 11 00 */    {2,    3,    4,    1},        /* 109 - 01 10 11 01 */    {2,    3,    4,    2},        /* 110 - 01 10 11 10 */    {2,    3,    4,    3},        /* 111 - 01 10 11 11 */    {2,    3,    4,    4},
    /* 112 - 01 11 00 00 */    {2,    4,    1,    1},        /* 113 - 01 11 00 01 */    {2,    4,    1,    2},        /* 114 - 01 11 00 10 */    {2,    4,    1,    3},        /* 115 - 01 11 00 11 */    {2,    4,    1,    4},
    /* 116 - 01 11 01 00 */    {2,    4,    2,    1},        /* 117 - 01 11 01 01 */    {2,    4,    2,    2},        /* 118 - 01 11 01 10 */    {2,    4,    2,    3},        /* 119 - 01 11 01 11 */    {2,    4,    2,    4},
    /* 120 - 01 11 10 00 */    {2,    4,    3,    1},        /* 121 - 01 11 10 01 */    {2,    4,    3,    2},        /* 122 - 01 11 10 10 */    {2,    4,    3,    3},        /* 123 - 01 11 10 11 */    {2,    4,    3,    4},
    /* 124 - 01 11 11 00 */    {2,    4,    4,    1},        /* 125 - 01 11 11 01 */    {2,    4,    4,    2},        /* 126 - 01 11 11 10 */    {2,    4,    4,    3},        /* 127 - 01 11 11 11 */    {2,    4,    4,    4},
    /* 128 - 10 00 00 00 */    {3,    1,    1,    1},        /* 129 - 10 00 00 01 */    {3,    1,    1,    2},        /* 130 - 10 00 00 10 */    {3,    1,    1,    3},        /* 131 - 10 00 00 11 */    {3,    1,    1,    4},
    /* 132 - 10 00 01 00 */    {3,    1,    2,    1},        /* 133 - 10 00 01 01 */    {3,    1,    2,    2},        /* 134 - 10 00 01 10 */    {3,    1,    2,    3},        /* 135 - 10 00 01 11 */    {3,    1,    2,    4},
    /* 136 - 10 00 10 00 */    {3,    1,    3,    1},        /* 137 - 10 00 10 01 */    {3,    1,    3,    2},        /* 138 - 10 00 10 10 */    {3,    1,    3,    3},        /* 139 - 10 00 10 11 */    {3,    1,    3,    4},
    /* 140 - 10 00 11 00 */    {3,    1,    4,    1},        /* 141 - 10 00 11 01 */    {3,    1,    4,    2},        /* 142 - 10 00 11 10 */    {3,    1,    4,    3},        /* 143 - 10 00 11 11 */    {3,    1,    4,    4},
    /* 144 - 10 01 00 00 */    {3,    2,    1,    1},        /* 145 - 10 01 00 01 */    {3,    2,    1,    2},        /* 146 - 10 01 00 10 */    {3,    2,    1,    3},        /* 147 - 10 01 00 11 */    {3,    2,    1,    4},
    /* 148 - 10 01 01 00 */    {3,    2,    2,    1},        /* 149 - 10 01 01 01 */    {3,    2,    2,    2},        /* 150 - 10 01 01 10 */    {3,    2,    2,    3},        /* 151 - 10 01 01 11 */    {3,    2,    2,    4},
    /* 152 - 10 01 10 00 */    {3,    2,    3,    1},        /* 153 - 10 01 10 01 */    {3,    2,    3,    2},        /* 154 - 10 01 10 10 */    {3,    2,    3,    3},        /* 155 - 10 01 10 11 */    {3,    2,    3,    4},
    /* 156 - 10 01 11 00 */    {3,    2,    4,    1},        /* 157 - 10 01 11 01 */    {3,    2,    4,    2},        /* 158 - 10 01 11 10 */    {3,    2,    4,    3},        /* 159 - 10 01 11 11 */    {3,    2,    4,    4},
    /* 160 - 10 10 00 00 */    {3,    3,    1,    1},        /* 161 - 10 10 00 01 */    {3,    3,    1,    2},        /* 162 - 10 10 00 10 */    {3,    3,    1,    3},        /* 163 - 10 10 00 11 */    {3,    3,    1,    4},
    /* 164 - 10 10 01 00 */    {3,    3,    2,    1},        /* 165 - 10 10 01 01 */    {3,    3,    2,    2},        /* 166 - 10 10 01 10 */    {3,    3,    2,    3},        /* 167 - 10 10 01 11 */    {3,    3,    2,    4},
    /* 168 - 10 10 10 00 */    {3,    3,    3,    1},        /* 169 - 10 10 10 01 */    {3,    3,    3,    2},        /* 170 - 10 10 10 10 */    {3,    3,    3,    3},        /* 171 - 10 10 10 11 */    {3,    3,    3,    4},
    /* 172 - 10 10 11 00 */    {3,    3,    4,    1},        /* 173 - 10 10 11 01 */    {3,    3,    4,    2},        /* 174 - 10 10 11 10 */    {3,    3,    4,    3},        /* 175 - 10 10 11 11 */    {3,    3,    4,    4},
    /* 176 - 10 11 00 00 */    {3,    4,    1,    1},        /* 177 - 10 11 00 01 */    {3,    4,    1,    2},        /* 178 - 10 11 00 10 */    {3,    4,    1,    3},        /* 179 - 10 11 00 11 */    {3,    4,    1,    4},
    /* 180 - 10 11 01 00 */    {3,    4,    2,    1},        /* 181 - 10 11 01 01 */    {3,    4,    2,    2},        /* 182 - 10 11 01 10 */    {3,    4,    2,    3},        /* 183 - 10 11 01 11 */    {3,    4,    2,    4},
    /* 184 - 10 11 10 00 */    {3,    4,    3,    1},        /* 185 - 10 11 10 01 */    {3,    4,    3,    2},        /* 186 - 10 11 10 10 */    {3,    4,    3,    3},        /* 187 - 10 11 10 11 */    {3,    4,    3,    4},
    /* 188 - 10 11 11 00 */    {3,    4,    4,    1},        /* 189 - 10 11 11 01 */    {3,    4,    4,    2},        /* 190 - 10 11 11 10 */    {3,    4,    4,    3},        /* 191 - 10 11 11 11 */    {3,    4,    4,    4},
    /* 192 - 11 00 00 00 */    {4,    1,    1,    1},        /* 193 - 11 00 00 01 */    {4,    1,    1,    2},        /* 194 - 11 00 00 10 */    {4,    1,    1,    3},        /* 195 - 11 00 00 11 */    {4,    1,    1,    4},
    /* 196 - 11 00 01 00 */    {4,    1,    2,    1},        /* 197 - 11 00 01 01 */    {4,    1,    2,    2},        /* 198 - 11 00 01 10 */    {4,    1,    2,    3},        /* 199 - 11 00 01 11 */    {4,    1,    2,    4},
    /* 200 - 11 00 10 00 */    {4,    1,    3,    1},        /* 201 - 11 00 10 01 */    {4,    1,    3,    2},        /* 202 - 11 00 10 10 */    {4,    1,    3,    3},        /* 203 - 11 00 10 11 */    {4,    1,    3,    4},
    /* 204 - 11 00 11 00 */    {4,    1,    4,    1},        /* 205 - 11 00 11 01 */    {4,    1,    4,    2},        /* 206 - 11 00 11 10 */    {4,    1,    4,    3},        /* 207 - 11 00 11 11 */    {4,    1,    4,    4},
    /* 208 - 11 01 00 00 */    {4,    2,    1,    1},        /* 209 - 11 01 00 01 */    {4,    2,    1,    2},        /* 210 - 11 01 00 10 */    {4,    2,    1,    3},        /* 211 - 11 01 00 11 */    {4,    2,    1,    4},
    /* 212 - 11 01 01 00 */    {4,    2,    2,    1},        /* 213 - 11 01 01 01 */    {4,    2,    2,    2},        /* 214 - 11 01 01 10 */    {4,    2,    2,    3},        /* 215 - 11 01 01 11 */    {4,    2,    2,    4},
    /* 216 - 11 01 10 00 */    {4,    2,    3,    1},        /* 217 - 11 01 10 01 */    {4,    2,    3,    2},        /* 218 - 11 01 10 10 */    {4,    2,    3,    3},        /* 219 - 11 01 10 11 */    {4,    2,    3,    4},
    /* 220 - 11 01 11 00 */    {4,    2,    4,    1},        /* 221 - 11 01 11 01 */    {4,    2,    4,    2},        /* 222 - 11 01 11 10 */    {4,    2,    4,    3},        /* 223 - 11 01 11 11 */    {4,    2,    4,    4},
    /* 224 - 11 10 00 00 */    {4,    3,    1,    1},        /* 225 - 11 10 00 01 */    {4,    3,    1,    2},        /* 226 - 11 10 00 10 */    {4,    3,    1,    3},        /* 227 - 11 10 00 11 */    {4,    3,    1,    4},
    /* 228 - 11 10 01 00 */    {4,    3,    2,    1},        /* 229 - 11 10 01 01 */    {4,    3,    2,    2},        /* 230 - 11 10 01 10 */    {4,    3,    2,    3},        /* 231 - 11 10 01 11 */    {4,    3,    2,    4},
    /* 232 - 11 10 10 00 */    {4,    3,    3,    1},        /* 233 - 11 10 10 01 */    {4,    3,    3,    2},        /* 234 - 11 10 10 10 */    {4,    3,    3,    3},        /* 235 - 11 10 10 11 */    {4,    3,    3,    4},
    /* 236 - 11 10 11 00 */    {4,    3,    4,    1},        /* 237 - 11 10 11 01 */    {4,    3,    4,    2},        /* 238 - 11 10 11 10 */    {4,    3,    4,    3},        /* 239 - 11 10 11 11 */    {4,    3,    4,    4},
    /* 240 - 11 11 00 00 */    {4,    4,    1,    1},        /* 241 - 11 11 00 01 */    {4,    4,    1,    2},        /* 242 - 11 11 00 10 */    {4,    4,    1,    3},        /* 243 - 11 11 00 11 */    {4,    4,    1,    4},
    /* 244 - 11 11 01 00 */    {4,    4,    2,    1},        /* 245 - 11 11 01 01 */    {4,    4,    2,    2},        /* 246 - 11 11 01 10 */    {4,    4,    2,    3},        /* 247 - 11 11 01 11 */    {4,    4,    2,    4},
    /* 248 - 11 11 10 00 */    {4,    4,    3,    1},        /* 249 - 11 11 10 01 */    {4,    4,    3,    2},        /* 250 - 11 11 10 10 */    {4,    4,    3,    3},        /* 251 - 11 11 10 11 */    {4,    4,    3,    4},
    /* 252 - 11 11 11 00 */    {4,    4,    4,    1},        /* 253 - 11 11 11 01 */    {4,    4,    4,    2},        /* 254 - 11 11 11 10 */    {4,    4,    4,    3},        /* 255 - 11 11 11 11 */    {4,    4,    4,    4},
};


/* Macro to get the size of a varint */
#define UTL_NUM_GET_VARINT_SIZE(uiMacroValue, uiMacroSize) \
    {    \
        ASSERT((uiMacroValue >= 0) && (uiMacroValue <= 0xFFFFFFFF)); \
\
        if ( uiMacroValue <= 0xFF ) {    \
            uiMacroSize = 1; \
        }    \
        else if ( uiMacroValue <= 0xFFFF ) {    \
            uiMacroSize = 2; \
        }    \
        else if ( uiMacroValue <= 0xFFFFFF ) {    \
            uiMacroSize = 3; \
        }    \
        else if ( uiMacroValue <= 0xFFFFFFFF ) {    \
            uiMacroSize = 4; \
        }    \
        else {    \
            iUtlLogPanic(UTL_LOG_CONTEXT, "Value exceeds maximum size for a varint, value: %u", uiMacroValue); \
        }    \
    }



/* Macro to write a varint */
#define UTL_NUM_WRITE_VARINT(uiMacroValue, uiMacroSize, pucMacroPtr) \
    {    \
        ASSERT((uiMacroValue >= 0) && (uiMacroValue <= 0xFFFFFFFF)); \
        ASSERT(uiMacroValue >= 0); \
        ASSERT((uiMacroSize >= 1) && (uiMacroSize <= 4)); \
        ASSERT(pucMacroPtr != NULL); \
\
        unsigned char    *pucMacroValuePtr = (unsigned char *)&uiMacroValue; \
\
        switch ( uiMacroSize ) {    \
            case 4: *pucMacroPtr = *pucMacroValuePtr; pucMacroPtr++; pucMacroValuePtr++; \
            case 3: *pucMacroPtr = *pucMacroValuePtr; pucMacroPtr++; pucMacroValuePtr++; \
            case 2: *pucMacroPtr = *pucMacroValuePtr; pucMacroPtr++; pucMacroValuePtr++; \
            case 1: *pucMacroPtr = *pucMacroValuePtr; pucMacroPtr++; pucMacroValuePtr++; \
        }    \
    }


/* Macro to read a varint */
#define UTL_NUM_READ_VARINT(uiMacroValue, uiMacroSize, pucMacroPtr) \
    {    \
        ASSERT((uiMacroSize >= 1) && (uiMacroSize <= 4)); \
        ASSERT(pucMacroPtr != NULL); \
\
        uiMacroValue = (unsigned int)*((unsigned int *)(pucMacroPtr)); \
        uiMacroValue &= uiVarintMaskGlobal[uiMacroSize]; \
        pucMacroPtr += uiMacroSize; \
    }



/* Header size for a varint quad or trio */
#define UTL_NUM_VARINT_HEADER_SIZE                                  (1)

/* Number of bits used to encode the number of bytes in the varint */
#define UTL_NUM_VARINT_HEADER_SIZE_BITS                             (2)


/* Macro to get the size of a varint quad */
#define UTL_NUM_GET_VARINT_QUAD_SIZE(uiMacroValue1, uiMacroValue2, uiMacroValue3, uiMacroValue4, uiMacroSize) \
    {    \
        ASSERT((uiMacroValue1 >= 0) && (uiMacroValue1 <= 0xFFFFFFFF)); \
        ASSERT((uiMacroValue2 >= 0) && (uiMacroValue2 <= 0xFFFFFFFF)); \
        ASSERT((uiMacroValue3 >= 0) && (uiMacroValue3 <= 0xFFFFFFFF)); \
        ASSERT((uiMacroValue4 >= 0) && (uiMacroValue4 <= 0xFFFFFFFF)); \
\
        unsigned int    uiMacroLocalSize = 0; \
\
        uiMacroSize = UTL_NUM_VARINT_HEADER_SIZE; \
\
        UTL_NUM_GET_VARINT_SIZE(uiMacroValue1, uiMacroLocalSize); \
        uiMacroSize += uiMacroLocalSize; \
\
        UTL_NUM_GET_VARINT_SIZE(uiMacroValue2, uiMacroLocalSize); \
        uiMacroSize += uiMacroLocalSize; \
\
        UTL_NUM_GET_VARINT_SIZE(uiMacroValue3, uiMacroLocalSize); \
        uiMacroSize += uiMacroLocalSize; \
\
        UTL_NUM_GET_VARINT_SIZE(uiMacroValue4, uiMacroLocalSize); \
        uiMacroSize += uiMacroLocalSize; \
    }


/* Macro to write a varint quad */
#define UTL_NUM_WRITE_VARINT_QUAD(uiMacroValue1, uiMacroValue2, uiMacroValue3, uiMacroValue4, pucMacroPtr) \
    {    \
        ASSERT((uiMacroValue1 >= 0) && (uiMacroValue1 <= 0xFFFFFFFF)); \
        ASSERT((uiMacroValue2 >= 0) && (uiMacroValue2 <= 0xFFFFFFFF)); \
        ASSERT((uiMacroValue3 >= 0) && (uiMacroValue3 <= 0xFFFFFFFF)); \
        ASSERT((uiMacroValue4 >= 0) && (uiMacroValue4 <= 0xFFFFFFFF)); \
        ASSERT(pucMacroPtr != NULL); \
\
        unsigned char    *pucMacroStartPtr = pucMacroPtr; \
        unsigned char    ucMacroQuartetHeader = '\0'; \
        unsigned int    uiMacroSize = 0; \
\
        pucMacroPtr += UTL_NUM_VARINT_HEADER_SIZE; \
\
        UTL_NUM_GET_VARINT_SIZE(uiMacroValue1, uiMacroSize); \
        ucMacroQuartetHeader |= (uiMacroSize - 1); \
        UTL_NUM_WRITE_VARINT(uiMacroValue1, uiMacroSize, pucMacroPtr); \
\
        UTL_NUM_GET_VARINT_SIZE(uiMacroValue2, uiMacroSize); \
        ucMacroQuartetHeader <<= UTL_NUM_VARINT_HEADER_SIZE_BITS; \
        ucMacroQuartetHeader |= (uiMacroSize - 1); \
        UTL_NUM_WRITE_VARINT(uiMacroValue2, uiMacroSize, pucMacroPtr); \
\
        UTL_NUM_GET_VARINT_SIZE(uiMacroValue3, uiMacroSize); \
        ucMacroQuartetHeader <<= UTL_NUM_VARINT_HEADER_SIZE_BITS; \
        ucMacroQuartetHeader |= (uiMacroSize - 1); \
        UTL_NUM_WRITE_VARINT(uiMacroValue3, uiMacroSize, pucMacroPtr); \
\
        UTL_NUM_GET_VARINT_SIZE(uiMacroValue4, uiMacroSize); \
        ucMacroQuartetHeader <<= UTL_NUM_VARINT_HEADER_SIZE_BITS; \
        ucMacroQuartetHeader |= (uiMacroSize - 1); \
        UTL_NUM_WRITE_VARINT(uiMacroValue4, uiMacroSize, pucMacroPtr); \
\
        *pucMacroStartPtr = ucMacroQuartetHeader; \
    }


/* Macro to read a varint quad */
#define UTL_NUM_READ_VARINT_QUAD(uiMacroValue1, uiMacroValue2, uiMacroValue3, uiMacroValue4, pucMacroPtr) \
    {    \
        ASSERT(pucMacroPtr != NULL); \
\
        struct varintSize    *pvsVarintSizesPtr = pvsVarintSizesGlobal + pucMacroPtr[0]; \
\
        pucMacroPtr += UTL_NUM_VARINT_HEADER_SIZE; \
\
        UTL_NUM_READ_VARINT(uiMacroValue1, pvsVarintSizesPtr->ucSize1, pucMacroPtr) \
        UTL_NUM_READ_VARINT(uiMacroValue2, pvsVarintSizesPtr->ucSize2, pucMacroPtr) \
        UTL_NUM_READ_VARINT(uiMacroValue3, pvsVarintSizesPtr->ucSize3, pucMacroPtr) \
        UTL_NUM_READ_VARINT(uiMacroValue4, pvsVarintSizesPtr->ucSize4, pucMacroPtr) \
    }



/* Macro to get the size of a varint trio */
#define UTL_NUM_GET_VARINT_TRIO_SIZE(uiMacroValue1, uiMacroValue2, uiMacroValue3, uiMacroSize) \
    {    \
        ASSERT((uiMacroValue1 >= 0) && (uiMacroValue1 <= 0xFFFFFFFF)); \
        ASSERT((uiMacroValue2 >= 0) && (uiMacroValue2 <= 0xFFFFFFFF)); \
        ASSERT((uiMacroValue3 >= 0) && (uiMacroValue3 <= 0xFFFFFFFF)); \
\
        unsigned int    uiMacroLocalSize = 0; \
\
        uiMacroSize = UTL_NUM_VARINT_HEADER_SIZE; \
\
        UTL_NUM_GET_VARINT_SIZE(uiMacroValue1, uiMacroLocalSize); \
        uiMacroSize += uiMacroLocalSize; \
\
        UTL_NUM_GET_VARINT_SIZE(uiMacroValue2, uiMacroLocalSize); \
        uiMacroSize += uiMacroLocalSize; \
\
        UTL_NUM_GET_VARINT_SIZE(uiMacroValue3, uiMacroLocalSize); \
        uiMacroSize += uiMacroLocalSize; \
    }


/* Macro to write a varint trio */
#define UTL_NUM_WRITE_VARINT_TRIO(uiMacroValue1, uiMacroValue2, uiMacroValue3, pucMacroPtr) \
    {    \
        ASSERT((uiMacroValue1 >= 0) && (uiMacroValue1 <= 0xFFFFFFFF)); \
        ASSERT((uiMacroValue2 >= 0) && (uiMacroValue2 <= 0xFFFFFFFF)); \
        ASSERT((uiMacroValue3 >= 0) && (uiMacroValue3 <= 0xFFFFFFFF)); \
        ASSERT(pucMacroPtr != NULL); \
\
        unsigned char    *pucMacroStartPtr = pucMacroPtr; \
        unsigned char    ucMacroTrioHeader = '\0'; \
        unsigned int    uiMacroSize = 0; \
\
        pucMacroPtr += UTL_NUM_VARINT_HEADER_SIZE; \
\
        UTL_NUM_GET_VARINT_SIZE(uiMacroValue1, uiMacroSize); \
        ucMacroTrioHeader |= (uiMacroSize - 1); \
        UTL_NUM_WRITE_VARINT(uiMacroValue1, uiMacroSize, pucMacroPtr); \
\
        UTL_NUM_GET_VARINT_SIZE(uiMacroValue2, uiMacroSize); \
        ucMacroTrioHeader <<= UTL_NUM_VARINT_HEADER_SIZE_BITS; \
        ucMacroTrioHeader |= (uiMacroSize - 1); \
        UTL_NUM_WRITE_VARINT(uiMacroValue2, uiMacroSize, pucMacroPtr); \
\
        UTL_NUM_GET_VARINT_SIZE(uiMacroValue3, uiMacroSize); \
        ucMacroTrioHeader <<= UTL_NUM_VARINT_HEADER_SIZE_BITS; \
        ucMacroTrioHeader |= (uiMacroSize - 1); \
        UTL_NUM_WRITE_VARINT(uiMacroValue3, uiMacroSize, pucMacroPtr); \
\
        ucMacroTrioHeader <<= UTL_NUM_VARINT_HEADER_SIZE_BITS; \
\
        *pucMacroStartPtr = ucMacroTrioHeader; \
    }


/* Macro to read a varint trio */
#define UTL_NUM_READ_VARINT_TRIO(uiMacroValue1, uiMacroValue2, uiMacroValue3, pucMacroPtr) \
    {    \
        ASSERT(pucMacroPtr != NULL); \
\
        struct varintSize    *pvsVarintSizesPtr = pvsVarintSizesGlobal + pucMacroPtr[0]; \
\
        pucMacroPtr += UTL_NUM_VARINT_HEADER_SIZE; \
\
        UTL_NUM_READ_VARINT(uiMacroValue1, pvsVarintSizesPtr->ucSize1, pucMacroPtr) \
        UTL_NUM_READ_VARINT(uiMacroValue2, pvsVarintSizesPtr->ucSize2, pucMacroPtr) \
        UTL_NUM_READ_VARINT(uiMacroValue3, pvsVarintSizesPtr->ucSize3, pucMacroPtr) \
    }


/*---------------------------------------------------------------------------*/


/*
** ============================================== 
** === Number storage macros (compact varint) ===
** ==============================================
*/


/* Masks used to mask out a compact varint in memory */
static unsigned int     uiCompactVarintMaskGlobal[] = {0x0, 0xFF, 0xFFFF, 0xFFFFFF};


/* Structure to store the byte size of each varint - already defined above */
/* struct varintSize { */
/*     unsigned char    ucSize1; */
/*     unsigned char    ucSize2; */
/*     unsigned char    ucSize3; */
/*     unsigned char    ucSize4; */
/* }; */


/* Structure which tells us the byte size of each varint based on the offset */
static struct varintSize pvsCompactVarintSizesGlobal[] = 
{
    /*   0 - 00 00 00 00 */    {0,    0,    0,    0},        /*   1 - 00 00 00 01 */    {0,    0,    0,    1},        /*   2 - 00 00 00 10 */    {0,    0,    0,    2},        /*   3 - 00 00 00 11 */    {0,    0,    0,    3},
    /*   4 - 00 00 01 00 */    {0,    0,    1,    0},        /*   5 - 00 00 01 01 */    {0,    0,    1,    1},        /*   6 - 00 00 01 10 */    {0,    0,    1,    2},        /*   7 - 00 00 01 11 */    {0,    0,    1,    3},
    /*   8 - 00 00 10 00 */    {0,    0,    2,    0},        /*   9 - 00 00 10 01 */    {0,    0,    2,    1},        /*  10 - 00 00 10 10 */    {0,    0,    2,    2},        /*  11 - 00 00 10 11 */    {0,    0,    2,    3},
    /*  12 - 00 00 11 00 */    {0,    0,    3,    0},        /*  13 - 00 00 11 01 */    {0,    0,    3,    1},        /*  14 - 00 00 11 10 */    {0,    0,    3,    2},        /*  15 - 00 00 11 11 */    {0,    0,    3,    3},
    /*  16 - 00 01 00 00 */    {0,    1,    0,    0},        /*  17 - 00 01 00 01 */    {0,    1,    0,    1},        /*  18 - 00 01 00 10 */    {0,    1,    0,    2},        /*  19 - 00 01 00 11 */    {0,    1,    0,    3},
    /*  20 - 00 01 01 00 */    {0,    1,    1,    0},        /*  21 - 00 01 01 01 */    {0,    1,    1,    1},        /*  22 - 00 01 01 10 */    {0,    1,    1,    2},        /*  23 - 00 01 01 11 */    {0,    1,    1,    3},
    /*  24 - 00 01 10 00 */    {0,    1,    2,    0},        /*  25 - 00 01 10 01 */    {0,    1,    2,    1},        /*  26 - 00 01 10 10 */    {0,    1,    2,    2},        /*  27 - 00 01 10 11 */    {0,    1,    2,    3},
    /*  28 - 00 01 11 00 */    {0,    1,    3,    0},        /*  29 - 00 01 11 01 */    {0,    1,    3,    1},        /*  30 - 00 01 11 10 */    {0,    1,    3,    2},        /*  31 - 00 01 11 11 */    {0,    1,    3,    3},
    /*  32 - 00 10 00 00 */    {0,    2,    0,    0},        /*  33 - 00 10 00 01 */    {0,    2,    0,    1},        /*  34 - 00 10 00 10 */    {0,    2,    0,    2},        /*  35 - 00 10 00 11 */    {0,    2,    0,    3},
    /*  36 - 00 10 01 00 */    {0,    2,    1,    0},        /*  37 - 00 10 01 01 */    {0,    2,    1,    1},        /*  38 - 00 10 01 10 */    {0,    2,    1,    2},        /*  39 - 00 10 01 11 */    {0,    2,    1,    3},
    /*  40 - 00 10 10 00 */    {0,    2,    2,    0},        /*  41 - 00 10 10 01 */    {0,    2,    2,    1},        /*  42 - 00 10 10 10 */    {0,    2,    2,    2},        /*  43 - 00 10 10 11 */    {0,    2,    2,    3},
    /*  44 - 00 10 11 00 */    {0,    2,    3,    0},        /*  45 - 00 10 11 01 */    {0,    2,    3,    1},        /*  46 - 00 10 11 10 */    {0,    2,    3,    2},        /*  47 - 00 10 11 11 */    {0,    2,    3,    3},
    /*  48 - 00 11 00 00 */    {0,    3,    0,    0},        /*  49 - 00 11 00 01 */    {0,    3,    0,    1},        /*  50 - 00 11 00 10 */    {0,    3,    0,    2},        /*  51 - 00 11 00 11 */    {0,    3,    0,    3},
    /*  52 - 00 11 01 00 */    {0,    3,    1,    0},        /*  53 - 00 11 01 01 */    {0,    3,    1,    1},        /*  54 - 00 11 01 10 */    {0,    3,    1,    2},        /*  55 - 00 11 01 11 */    {0,    3,    1,    3},
    /*  56 - 00 11 10 00 */    {0,    3,    2,    0},        /*  57 - 00 11 10 01 */    {0,    3,    2,    1},        /*  58 - 00 11 10 10 */    {0,    3,    2,    2},        /*  59 - 00 11 10 11 */    {0,    3,    2,    3},
    /*  60 - 00 11 11 00 */    {0,    3,    3,    0},        /*  61 - 00 11 11 01 */    {0,    3,    3,    1},        /*  62 - 00 11 11 10 */    {0,    3,    3,    2},        /*  63 - 00 11 11 11 */    {0,    3,    3,    3},
    /*  64 - 01 00 00 00 */    {1,    0,    0,    0},        /*  65 - 01 00 00 01 */    {1,    0,    0,    1},        /*  66 - 01 00 00 10 */    {1,    0,    0,    2},        /*  67 - 01 00 00 11 */    {1,    0,    0,    3},
    /*  68 - 01 00 01 00 */    {1,    0,    1,    0},        /*  69 - 01 00 01 01 */    {1,    0,    1,    1},        /*  70 - 01 00 01 10 */    {1,    0,    1,    2},        /*  71 - 01 00 01 11 */    {1,    0,    1,    3},
    /*  72 - 01 00 10 00 */    {1,    0,    2,    0},        /*  73 - 01 00 10 01 */    {1,    0,    2,    1},        /*  74 - 01 00 10 10 */    {1,    0,    2,    2},        /*  75 - 01 00 10 11 */    {1,    0,    2,    3},
    /*  76 - 01 00 11 00 */    {1,    0,    3,    0},        /*  77 - 01 00 11 01 */    {1,    0,    3,    1},        /*  78 - 01 00 11 10 */    {1,    0,    3,    2},        /*  79 - 01 00 11 11 */    {1,    0,    3,    3},
    /*  80 - 01 01 00 00 */    {1,    1,    0,    0},        /*  81 - 01 01 00 01 */    {1,    1,    0,    1},        /*  82 - 01 01 00 10 */    {1,    1,    0,    2},        /*  83 - 01 01 00 11 */    {1,    1,    0,    3},
    /*  84 - 01 01 01 00 */    {1,    1,    1,    0},        /*  85 - 01 01 01 01 */    {1,    1,    1,    1},        /*  86 - 01 01 01 10 */    {1,    1,    1,    2},        /*  87 - 01 01 01 11 */    {1,    1,    1,    3},
    /*  88 - 01 01 10 00 */    {1,    1,    2,    0},        /*  89 - 01 01 10 01 */    {1,    1,    2,    1},        /*  90 - 01 01 10 10 */    {1,    1,    2,    2},        /*  91 - 01 01 10 11 */    {1,    1,    2,    3},
    /*  92 - 01 01 11 00 */    {1,    1,    3,    0},        /*  93 - 01 01 11 01 */    {1,    1,    3,    1},        /*  94 - 01 01 11 10 */    {1,    1,    3,    2},        /*  95 - 01 01 11 11 */    {1,    1,    3,    3},
    /*  96 - 01 10 00 00 */    {1,    2,    0,    0},        /*  97 - 01 10 00 01 */    {1,    2,    0,    1},        /*  98 - 01 10 00 10 */    {1,    2,    0,    2},        /*  99 - 01 10 00 11 */    {1,    2,    0,    3},
    /* 100 - 01 10 01 00 */    {1,    2,    1,    0},        /* 101 - 01 10 01 01 */    {1,    2,    1,    1},        /* 102 - 01 10 01 10 */    {1,    2,    1,    2},        /* 103 - 01 10 01 11 */    {1,    2,    1,    3},
    /* 104 - 01 10 10 00 */    {1,    2,    2,    0},        /* 105 - 01 10 10 01 */    {1,    2,    2,    1},        /* 106 - 01 10 10 10 */    {1,    2,    2,    2},        /* 107 - 01 10 10 11 */    {1,    2,    2,    3},
    /* 108 - 01 10 11 00 */    {1,    2,    3,    0},        /* 109 - 01 10 11 01 */    {1,    2,    3,    1},        /* 110 - 01 10 11 10 */    {1,    2,    3,    2},        /* 111 - 01 10 11 11 */    {1,    2,    3,    3},
    /* 112 - 01 11 00 00 */    {1,    3,    0,    0},        /* 113 - 01 11 00 01 */    {1,    3,    0,    1},        /* 114 - 01 11 00 10 */    {1,    3,    0,    2},        /* 115 - 01 11 00 11 */    {1,    3,    0,    3},
    /* 116 - 01 11 01 00 */    {1,    3,    1,    0},        /* 117 - 01 11 01 01 */    {1,    3,    1,    1},        /* 118 - 01 11 01 10 */    {1,    3,    1,    2},        /* 119 - 01 11 01 11 */    {1,    3,    1,    3},
    /* 120 - 01 11 10 00 */    {1,    3,    2,    0},        /* 121 - 01 11 10 01 */    {1,    3,    2,    1},        /* 122 - 01 11 10 10 */    {1,    3,    2,    2},        /* 123 - 01 11 10 11 */    {1,    3,    2,    3},
    /* 124 - 01 11 11 00 */    {1,    3,    3,    0},        /* 125 - 01 11 11 01 */    {1,    3,    3,    1},        /* 126 - 01 11 11 10 */    {1,    3,    3,    2},        /* 127 - 01 11 11 11 */    {1,    3,    3,    3},
    /* 128 - 10 00 00 00 */    {2,    0,    0,    0},        /* 129 - 10 00 00 01 */    {2,    0,    0,    1},        /* 130 - 10 00 00 10 */    {2,    0,    0,    2},        /* 131 - 10 00 00 11 */    {2,    0,    0,    3},
    /* 132 - 10 00 01 00 */    {2,    0,    1,    0},        /* 133 - 10 00 01 01 */    {2,    0,    1,    1},        /* 134 - 10 00 01 10 */    {2,    0,    1,    2},        /* 135 - 10 00 01 11 */    {2,    0,    1,    3},
    /* 136 - 10 00 10 00 */    {2,    0,    2,    0},        /* 137 - 10 00 10 01 */    {2,    0,    2,    1},        /* 138 - 10 00 10 10 */    {2,    0,    2,    2},        /* 139 - 10 00 10 11 */    {2,    0,    2,    3},
    /* 140 - 10 00 11 00 */    {2,    0,    3,    0},        /* 141 - 10 00 11 01 */    {2,    0,    3,    1},        /* 142 - 10 00 11 10 */    {2,    0,    3,    2},        /* 143 - 10 00 11 11 */    {2,    0,    3,    3},
    /* 144 - 10 01 00 00 */    {2,    1,    0,    0},        /* 145 - 10 01 00 01 */    {2,    1,    0,    1},        /* 146 - 10 01 00 10 */    {2,    1,    0,    2},        /* 147 - 10 01 00 11 */    {2,    1,    0,    3},
    /* 148 - 10 01 01 00 */    {2,    1,    1,    0},        /* 149 - 10 01 01 01 */    {2,    1,    1,    1},        /* 150 - 10 01 01 10 */    {2,    1,    1,    2},        /* 151 - 10 01 01 11 */    {2,    1,    1,    3},
    /* 152 - 10 01 10 00 */    {2,    1,    2,    0},        /* 153 - 10 01 10 01 */    {2,    1,    2,    1},        /* 154 - 10 01 10 10 */    {2,    1,    2,    2},        /* 155 - 10 01 10 11 */    {2,    1,    2,    3},
    /* 156 - 10 01 11 00 */    {2,    1,    3,    0},        /* 157 - 10 01 11 01 */    {2,    1,    3,    1},        /* 158 - 10 01 11 10 */    {2,    1,    3,    2},        /* 159 - 10 01 11 11 */    {2,    1,    3,    3},
    /* 160 - 10 10 00 00 */    {2,    2,    0,    0},        /* 161 - 10 10 00 01 */    {2,    2,    0,    1},        /* 162 - 10 10 00 10 */    {2,    2,    0,    2},        /* 163 - 10 10 00 11 */    {2,    2,    0,    3},
    /* 164 - 10 10 01 00 */    {2,    2,    1,    0},        /* 165 - 10 10 01 01 */    {2,    2,    1,    1},        /* 166 - 10 10 01 10 */    {2,    2,    1,    2},        /* 167 - 10 10 01 11 */    {2,    2,    1,    3},
    /* 168 - 10 10 10 00 */    {2,    2,    2,    0},        /* 169 - 10 10 10 01 */    {2,    2,    2,    1},        /* 170 - 10 10 10 10 */    {2,    2,    2,    2},        /* 171 - 10 10 10 11 */    {2,    2,    2,    3},
    /* 172 - 10 10 11 00 */    {2,    2,    3,    0},        /* 173 - 10 10 11 01 */    {2,    2,    3,    1},        /* 174 - 10 10 11 10 */    {2,    2,    3,    2},        /* 175 - 10 10 11 11 */    {2,    2,    3,    3},
    /* 176 - 10 11 00 00 */    {2,    3,    0,    0},        /* 177 - 10 11 00 01 */    {2,    3,    0,    1},        /* 178 - 10 11 00 10 */    {2,    3,    0,    2},        /* 179 - 10 11 00 11 */    {2,    3,    0,    3},
    /* 180 - 10 11 01 00 */    {2,    3,    1,    0},        /* 181 - 10 11 01 01 */    {2,    3,    1,    1},        /* 182 - 10 11 01 10 */    {2,    3,    1,    2},        /* 183 - 10 11 01 11 */    {2,    3,    1,    3},
    /* 184 - 10 11 10 00 */    {2,    3,    2,    0},        /* 185 - 10 11 10 01 */    {2,    3,    2,    1},        /* 186 - 10 11 10 10 */    {2,    3,    2,    2},        /* 187 - 10 11 10 11 */    {2,    3,    2,    3},
    /* 188 - 10 11 11 00 */    {2,    3,    3,    0},        /* 189 - 10 11 11 01 */    {2,    3,    3,    1},        /* 190 - 10 11 11 10 */    {2,    3,    3,    2},        /* 191 - 10 11 11 11 */    {2,    3,    3,    3},
    /* 192 - 11 00 00 00 */    {3,    0,    0,    0},        /* 193 - 11 00 00 01 */    {3,    0,    0,    1},        /* 194 - 11 00 00 10 */    {3,    0,    0,    2},        /* 195 - 11 00 00 11 */    {3,    0,    0,    3},
    /* 196 - 11 00 01 00 */    {3,    0,    1,    0},        /* 197 - 11 00 01 01 */    {3,    0,    1,    1},        /* 198 - 11 00 01 10 */    {3,    0,    1,    2},        /* 199 - 11 00 01 11 */    {3,    0,    1,    3},
    /* 200 - 11 00 10 00 */    {3,    0,    2,    0},        /* 201 - 11 00 10 01 */    {3,    0,    2,    1},        /* 202 - 11 00 10 10 */    {3,    0,    2,    2},        /* 203 - 11 00 10 11 */    {3,    0,    2,    3},
    /* 204 - 11 00 11 00 */    {3,    0,    3,    0},        /* 205 - 11 00 11 01 */    {3,    0,    3,    1},        /* 206 - 11 00 11 10 */    {3,    0,    3,    2},        /* 207 - 11 00 11 11 */    {3,    0,    3,    3},
    /* 208 - 11 01 00 00 */    {3,    1,    0,    0},        /* 209 - 11 01 00 01 */    {3,    1,    0,    1},        /* 210 - 11 01 00 10 */    {3,    1,    0,    2},        /* 211 - 11 01 00 11 */    {3,    1,    0,    3},
    /* 212 - 11 01 01 00 */    {3,    1,    1,    0},        /* 213 - 11 01 01 01 */    {3,    1,    1,    1},        /* 214 - 11 01 01 10 */    {3,    1,    1,    2},        /* 215 - 11 01 01 11 */    {3,    1,    1,    3},
    /* 216 - 11 01 10 00 */    {3,    1,    2,    0},        /* 217 - 11 01 10 01 */    {3,    1,    2,    1},        /* 218 - 11 01 10 10 */    {3,    1,    2,    2},        /* 219 - 11 01 10 11 */    {3,    1,    2,    3},
    /* 220 - 11 01 11 00 */    {3,    1,    3,    0},        /* 221 - 11 01 11 01 */    {3,    1,    3,    1},        /* 222 - 11 01 11 10 */    {3,    1,    3,    2},        /* 223 - 11 01 11 11 */    {3,    1,    3,    3},
    /* 224 - 11 10 00 00 */    {3,    2,    0,    0},        /* 225 - 11 10 00 01 */    {3,    2,    0,    1},        /* 226 - 11 10 00 10 */    {3,    2,    0,    2},        /* 227 - 11 10 00 11 */    {3,    2,    0,    3},
    /* 228 - 11 10 01 00 */    {3,    2,    1,    0},        /* 229 - 11 10 01 01 */    {3,    2,    1,    1},        /* 230 - 11 10 01 10 */    {3,    2,    1,    2},        /* 231 - 11 10 01 11 */    {3,    2,    1,    3},
    /* 232 - 11 10 10 00 */    {3,    2,    2,    0},        /* 233 - 11 10 10 01 */    {3,    2,    2,    1},        /* 234 - 11 10 10 10 */    {3,    2,    2,    2},        /* 235 - 11 10 10 11 */    {3,    2,    2,    3},
    /* 236 - 11 10 11 00 */    {3,    2,    3,    0},        /* 237 - 11 10 11 01 */    {3,    2,    3,    1},        /* 238 - 11 10 11 10 */    {3,    2,    3,    2},        /* 239 - 11 10 11 11 */    {3,    2,    3,    3},
    /* 240 - 11 11 00 00 */    {3,    3,    0,    0},        /* 241 - 11 11 00 01 */    {3,    3,    0,    1},        /* 242 - 11 11 00 10 */    {3,    3,    0,    2},        /* 243 - 11 11 00 11 */    {3,    3,    0,    3},
    /* 244 - 11 11 01 00 */    {3,    3,    1,    0},        /* 245 - 11 11 01 01 */    {3,    3,    1,    1},        /* 246 - 11 11 01 10 */    {3,    3,    1,    2},        /* 247 - 11 11 01 11 */    {3,    3,    1,    3},
    /* 248 - 11 11 10 00 */    {3,    3,    2,    0},        /* 249 - 11 11 10 01 */    {3,    3,    2,    1},        /* 250 - 11 11 10 10 */    {3,    3,    2,    2},        /* 251 - 11 11 10 11 */    {3,    3,    2,    3},
    /* 252 - 11 11 11 00 */    {3,    3,    3,    0},        /* 253 - 11 11 11 01 */    {3,    3,    3,    1},        /* 254 - 11 11 11 10 */    {3,    3,    3,    2},        /* 255 - 11 11 11 11 */    {3,    3,    3,    3},
};


/* Macro to get the size of a compact varint */
#define UTL_NUM_GET_COMPACT_VARINT_SIZE(uiMacroValue, uiMacroSize) \
    {    \
        ASSERT((uiMacroValue >= 0) && (uiMacroValue <= 0xFFFFFF)); \
\
        if ( uiMacroValue == 0 ) {    \
            uiMacroSize = 0; \
        }    \
        else if ( uiMacroValue <= 0xFF ) {    \
            uiMacroSize = 1; \
        }    \
        else if ( uiMacroValue <= 0xFFFF ) {    \
            uiMacroSize = 2; \
        }    \
        else if ( uiMacroValue <= 0xFFFFFF ) {    \
            uiMacroSize = 3; \
        }    \
        else {    \
            iUtlLogPanic(UTL_LOG_CONTEXT, "Value exceeds maximum size for a compact varint, value: %u", uiMacroValue); \
        }    \
    }



/* Macro to write a compact varint */
#define UTL_NUM_WRITE_COMPACT_VARINT(uiMacroValue, uiMacroSize, pucMacroPtr) \
    {    \
        ASSERT((uiMacroValue >= 0) && (uiMacroValue <= 0xFFFFFF)); \
        ASSERT((uiMacroSize >= 0) && (uiMacroSize <= 3)); \
        ASSERT(pucMacroPtr != NULL); \
\
        unsigned char    *pucMacroValuePtr = (unsigned char *)&uiMacroValue; \
\
        switch ( uiMacroSize ) {    \
            case 3: *pucMacroPtr = *pucMacroValuePtr; pucMacroPtr++; pucMacroValuePtr++; \
            case 2: *pucMacroPtr = *pucMacroValuePtr; pucMacroPtr++; pucMacroValuePtr++; \
            case 1: *pucMacroPtr = *pucMacroValuePtr; pucMacroPtr++; pucMacroValuePtr++; \
            case 0:    ; \
        }    \
    }


/* Macro to read a compact varint */
#define UTL_NUM_READ_COMPACT_VARINT(uiMacroValue, uiMacroSize, pucMacroPtr) \
    {    \
        ASSERT((uiMacroSize >= 0) && (uiMacroSize <= 3)); \
        ASSERT(pucMacroPtr != NULL); \
\
        uiMacroValue = (unsigned int)*((unsigned int *)(pucMacroPtr)); \
        uiMacroValue &= uiCompactVarintMaskGlobal[uiMacroSize]; \
        pucMacroPtr += uiMacroSize; \
    }



/* Header size for a varint quad or trio */
#define UTL_NUM_COMPACT_VARINT_HEADER_SIZE                          (1)

/* Number of bits used to encode the number of bytes in the varint */
#define UTL_NUM_COMPACT_VARINT_HEADER_BYTE_COUNT_BITS               (2)


/* Macro to get the size of a compact varint quad */
#define UTL_NUM_GET_COMPACT_VARINT_QUAD_SIZE(uiMacroValue1, uiMacroValue2, uiMacroValue3, uiMacroValue4, uiMacroSize) \
    {    \
        ASSERT((uiMacroValue1 >= 0) && (uiMacroValue1 <= 0xFFFFFF)); \
        ASSERT((uiMacroValue2 >= 0) && (uiMacroValue2 <= 0xFFFFFF)); \
        ASSERT((uiMacroValue3 >= 0) && (uiMacroValue3 <= 0xFFFFFF)); \
        ASSERT((uiMacroValue4 >= 0) && (uiMacroValue4 <= 0xFFFFFF)); \
\
        unsigned int    uiMacroLocalSize = 0; \
\
        uiMacroSize = UTL_NUM_COMPACT_VARINT_HEADER_SIZE; \
\
        UTL_NUM_GET_COMPACT_VARINT_SIZE(uiMacroValue1, uiMacroLocalSize); \
        uiMacroSize += uiMacroLocalSize; \
\
        UTL_NUM_GET_COMPACT_VARINT_SIZE(uiMacroValue2, uiMacroLocalSize); \
        uiMacroSize += uiMacroLocalSize; \
\
        UTL_NUM_GET_COMPACT_VARINT_SIZE(uiMacroValue3, uiMacroLocalSize); \
        uiMacroSize += uiMacroLocalSize; \
\
        UTL_NUM_GET_COMPACT_VARINT_SIZE(uiMacroValue4, uiMacroLocalSize); \
        uiMacroSize += uiMacroLocalSize; \
    }


/* Macro to write a compact varint quad */
#define UTL_NUM_WRITE_COMPACT_VARINT_QUAD(uiMacroValue1, uiMacroValue2, uiMacroValue3, uiMacroValue4, pucMacroPtr) \
    {    \
        ASSERT((uiMacroValue1 >= 0) && (uiMacroValue1 <= 0xFFFFFF)); \
        ASSERT((uiMacroValue2 >= 0) && (uiMacroValue2 <= 0xFFFFFF)); \
        ASSERT((uiMacroValue3 >= 0) && (uiMacroValue3 <= 0xFFFFFF)); \
        ASSERT((uiMacroValue4 >= 0) && (uiMacroValue4 <= 0xFFFFFF)); \
        ASSERT(pucMacroPtr != NULL); \
\
        unsigned char    *pucMacroStartPtr = pucMacroPtr; \
        unsigned char    ucMacroQuartetHeader = '\0'; \
        unsigned int    uiMacroSize = 0; \
\
        pucMacroPtr += UTL_NUM_COMPACT_VARINT_HEADER_SIZE; \
\
        UTL_NUM_GET_COMPACT_VARINT_SIZE(uiMacroValue1, uiMacroSize); \
        ucMacroQuartetHeader |= uiMacroSize; \
        UTL_NUM_WRITE_COMPACT_VARINT(uiMacroValue1, uiMacroSize, pucMacroPtr); \
\
        UTL_NUM_GET_COMPACT_VARINT_SIZE(uiMacroValue2, uiMacroSize); \
        ucMacroQuartetHeader <<= UTL_NUM_COMPACT_VARINT_HEADER_BYTE_COUNT_BITS; \
        ucMacroQuartetHeader |= uiMacroSize; \
        UTL_NUM_WRITE_COMPACT_VARINT(uiMacroValue2, uiMacroSize, pucMacroPtr); \
\
        UTL_NUM_GET_COMPACT_VARINT_SIZE(uiMacroValue3, uiMacroSize); \
        ucMacroQuartetHeader <<= UTL_NUM_COMPACT_VARINT_HEADER_BYTE_COUNT_BITS; \
        ucMacroQuartetHeader |= uiMacroSize; \
        UTL_NUM_WRITE_COMPACT_VARINT(uiMacroValue3, uiMacroSize, pucMacroPtr); \
\
        UTL_NUM_GET_COMPACT_VARINT_SIZE(uiMacroValue4, uiMacroSize); \
        ucMacroQuartetHeader <<= UTL_NUM_COMPACT_VARINT_HEADER_BYTE_COUNT_BITS; \
        ucMacroQuartetHeader |= uiMacroSize; \
        UTL_NUM_WRITE_COMPACT_VARINT(uiMacroValue4, uiMacroSize, pucMacroPtr); \
\
        *pucMacroStartPtr = ucMacroQuartetHeader; \
    }


/* Macro to read a compact varint quad */
#define UTL_NUM_READ_COMPACT_VARINT_QUAD(uiMacroValue1, uiMacroValue2, uiMacroValue3, uiMacroValue4, pucMacroPtr) \
    {    \
        ASSERT(pucMacroPtr != NULL); \
\
        struct varintSize    *pvsVarintSizesPtr = pvsCompactVarintSizesGlobal + pucMacroPtr[0]; \
\
        pucMacroPtr += UTL_NUM_COMPACT_VARINT_HEADER_SIZE; \
\
        UTL_NUM_READ_COMPACT_VARINT(uiMacroValue1, pvsVarintSizesPtr->ucSize1, pucMacroPtr) \
        UTL_NUM_READ_COMPACT_VARINT(uiMacroValue2, pvsVarintSizesPtr->ucSize2, pucMacroPtr) \
        UTL_NUM_READ_COMPACT_VARINT(uiMacroValue3, pvsVarintSizesPtr->ucSize3, pucMacroPtr) \
        UTL_NUM_READ_COMPACT_VARINT(uiMacroValue4, pvsVarintSizesPtr->ucSize4, pucMacroPtr) \
    }



/* Macro to get the size of a compact varint trio */
#define UTL_NUM_GET_COMPACT_VARINT_TRIO_SIZE(uiMacroValue1, uiMacroValue2, uiMacroValue3, uiMacroSize) \
    {    \
        ASSERT((uiMacroValue1 >= 0) && (uiMacroValue1 <= 0xFFFFFF)); \
        ASSERT((uiMacroValue2 >= 0) && (uiMacroValue2 <= 0xFFFFFF)); \
        ASSERT((uiMacroValue3 >= 0) && (uiMacroValue3 <= 0xFFFFFF)); \
\
        unsigned int    uiMacroLocalSize = 0; \
\
        uiMacroSize = UTL_NUM_COMPACT_VARINT_HEADER_SIZE; \
\
        UTL_NUM_GET_COMPACT_VARINT_SIZE(uiMacroValue1, uiMacroLocalSize); \
        uiMacroSize += uiMacroLocalSize; \
\
        UTL_NUM_GET_COMPACT_VARINT_SIZE(uiMacroValue2, uiMacroLocalSize); \
        uiMacroSize += uiMacroLocalSize; \
\
        UTL_NUM_GET_COMPACT_VARINT_SIZE(uiMacroValue3, uiMacroLocalSize); \
        uiMacroSize += uiMacroLocalSize; \
    }


/* Macro to write a compact varint trio */
#define UTL_NUM_WRITE_COMPACT_VARINT_TRIO(uiMacroValue1, uiMacroValue2, uiMacroValue3, pucMacroPtr) \
    {    \
        ASSERT((uiMacroValue1 >= 0) && (uiMacroValue1 <= 0xFFFFFF)); \
        ASSERT((uiMacroValue2 >= 0) && (uiMacroValue2 <= 0xFFFFFF)); \
        ASSERT((uiMacroValue3 >= 0) && (uiMacroValue3 <= 0xFFFFFF)); \
        ASSERT(pucMacroPtr != NULL); \
\
        unsigned char    *pucMacroStartPtr = pucMacroPtr; \
        unsigned char    ucMacroTrioHeader = '\0'; \
        unsigned int    uiMacroSize = 0; \
\
        pucMacroPtr += UTL_NUM_COMPACT_VARINT_HEADER_SIZE; \
\
        UTL_NUM_GET_COMPACT_VARINT_SIZE(uiMacroValue1, uiMacroSize); \
        ucMacroTrioHeader |= uiMacroSize; \
        UTL_NUM_WRITE_COMPACT_VARINT(uiMacroValue1, uiMacroSize, pucMacroPtr); \
\
        UTL_NUM_GET_COMPACT_VARINT_SIZE(uiMacroValue2, uiMacroSize); \
        ucMacroTrioHeader <<= UTL_NUM_COMPACT_VARINT_HEADER_BYTE_COUNT_BITS; \
        ucMacroTrioHeader |= uiMacroSize; \
        UTL_NUM_WRITE_COMPACT_VARINT(uiMacroValue2, uiMacroSize, pucMacroPtr); \
\
        UTL_NUM_GET_COMPACT_VARINT_SIZE(uiMacroValue3, uiMacroSize); \
        ucMacroTrioHeader <<= UTL_NUM_COMPACT_VARINT_HEADER_BYTE_COUNT_BITS; \
        ucMacroTrioHeader |= uiMacroSize; \
        UTL_NUM_WRITE_COMPACT_VARINT(uiMacroValue3, uiMacroSize, pucMacroPtr); \
\
        ucMacroTrioHeader <<= UTL_NUM_COMPACT_VARINT_HEADER_BYTE_COUNT_BITS; \
\
        *pucMacroStartPtr = ucMacroTrioHeader; \
    }


/* Macro to read a compact varint trio */
#define UTL_NUM_READ_COMPACT_VARINT_TRIO(uiMacroValue1, uiMacroValue2, uiMacroValue3, pucMacroPtr) \
    {    \
        ASSERT(pucMacroPtr != NULL); \
\
        struct varintSize    *pvsVarintSizesPtr = pvsCompactVarintSizesGlobal + pucMacroPtr[0]; \
\
        pucMacroPtr += UTL_NUM_COMPACT_VARINT_HEADER_SIZE; \
\
        UTL_NUM_READ_COMPACT_VARINT(uiMacroValue1, pvsVarintSizesPtr->ucSize1, pucMacroPtr) \
        UTL_NUM_READ_COMPACT_VARINT(uiMacroValue2, pvsVarintSizesPtr->ucSize2, pucMacroPtr) \
        UTL_NUM_READ_COMPACT_VARINT(uiMacroValue3, pvsVarintSizesPtr->ucSize3, pucMacroPtr) \
    }


#endif    /* defined(NOT_USED) */


/*---------------------------------------------------------------------------*/


/*
** Public function prototypes
*/

int iUtlNumRoundNumber (long lNumber, unsigned int uiThreshold, 
        unsigned int uiRounding, unsigned int uiDigits, long *plNumber);


/*---------------------------------------------------------------------------*/


/* 
** C++ wrapper
*/

#if defined(__cplusplus)
}
#endif    /* defined(__cplusplus) */


/*---------------------------------------------------------------------------*/


#endif    /* !defined(UTL_NUM_H) */


/*---------------------------------------------------------------------------*/
