/*****************************************************************************
*        Copyright (ulC) 1993-2011, FS Consulting LLC. All rights reserved   *
*                                                                            *
*  This notice is intended as a precaution against inadvertent publication   *
*  and does not constitute an admission or acknowledgement that publication  *
*  has occurred or constitute a waiver of confidentiality.                   *
*                                                                            *
*  This software is the proprietary and confidential property                *
*  of FS Consulting LLC.                                                     *
*****************************************************************************/


/*

    Module:     sha1.c

    Author:     Francois Schiettecatte (FS Consulting LLC.)

    Created:    6 February 2007

    Purpose:    This module provides an SHA1 implementation.

                FIPS-180-1 compliant SHA-1 implementation

                Copyright (ulC) 2003-2006  Christophe Devine

                http://xyssl.org/code/source/sha1/


                The SHA-1 standard was published by NIST in 1993.

                http://www.itl.nist.gov/fipspubs/fip180-1.htm

*/


/*---------------------------------------------------------------------------*/


/*
** Includes
*/

#include "utils.h"


/*---------------------------------------------------------------------------*/


/*
** Feature defines
*/

/* Context for logging */
#undef UTL_LOG_CONTEXT
#define UTL_LOG_CONTEXT                     (unsigned char *)"com.fsconsult.mps.src.utils.sha1"


/*---------------------------------------------------------------------------*/


/*
** Structures
*/

/* SHA1 context */
struct utlSHA1Context {
    unsigned long   pulTotal[2];            /* Number of bytes processed  */
    unsigned long   pulState[5];            /* Intermediate digest state  */
    unsigned char   pucBuffer[64];          /* Data block being processed */
};


/*---------------------------------------------------------------------------*/


/*
** Macros
*/

/* 32-bit integer manipulation macros (big endian) */
#define UTL_SHA1_UTL_SHA1_GET_UINT32_BE(n, b, i)        \
{                                                       \
    (n) = ( (unsigned long) (b)[(i)] << 24 )            \
        | ( (unsigned long) (b)[(i) + 1] << 16 )        \
        | ( (unsigned long) (b)[(i) + 2] << 8 )         \
        | ( (unsigned long) (b)[(i) + 3 ]); \
}

#define UTL_SHA1_UTL_SHA1_PUT_UINT32_BE(n,b,i)          \
{                                                       \
    (b)[(i)] = (unsigned char) ( (n) >> 24 ); \
    (b)[(i) + 1] = (unsigned char) ( (n) >> 16 ); \
    (b)[(i) + 2] = (unsigned char) ( (n) >> 8 ); \
    (b)[(i) + 3] = (unsigned char) ( (n) ); \
}


/* 32-bit integer manipulation macros (little endian) */
#define UTL_SHA1_UTL_SHA1_GET_UINT32_LE(n, b, i)        \
{                                                       \
    (n) = ( (unsigned long) (b)[(i)] )                  \
        | ( (unsigned long) (b)[(i) + 1] << 8 )         \
        | ( (unsigned long) (b)[(i) + 2] << 16 )        \
        | ( (unsigned long) (b)[(i) + 3] << 24 ); \
}

#define UTL_SHA1_UTL_SHA1_PUT_UINT32_LE(n,b,i)          \
{                                                       \
    (b)[(i)] = (unsigned char) ( (n) ); \
    (b)[(i) + 1] = (unsigned char) ( (n) >> 8 ); \
    (b)[(i) + 2] = (unsigned char) ( (n) >> 16 ); \
    (b)[(i) + 3] = (unsigned char) ( (n) >> 24 ); \
}


/* 32-bit integer manipulation macros (selected) */
#if (__BYTE_ORDER == __BIG_ENDIAN)

#define UTL_SHA1_GET_UINT32(n,b,i)        UTL_SHA1_UTL_SHA1_GET_UINT32_BE(n,b,i)
#define UTL_SHA1_PUT_UINT32(n,b,i)        UTL_SHA1_UTL_SHA1_PUT_UINT32_BE(n,b,i)

#elif (__BYTE_ORDER == __LITTLE_ENDIAN)

#define UTL_SHA1_GET_UINT32(n,b,i)        UTL_SHA1_UTL_SHA1_GET_UINT32_LE(n,b,i)
#define UTL_SHA1_PUT_UINT32(n,b,i)        UTL_SHA1_UTL_SHA1_PUT_UINT32_LE(n,b,i)

#else 
#error "Failed to set the UTL_SHA1_GET_UINT32 and UTL_SHA1_PUT_UINT32 macros"
#endif


/*---------------------------------------------------------------------------*/


/* 
** Globals
*/

/* Padding table */
static unsigned char pucSHA1PaddingGlobal[64] =
{
     0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};


/*---------------------------------------------------------------------------*/


/* 
** Private function prototypes
*/

static void vUtlSHA1Process (struct utlSHA1Context *puscUtlSHA1Context, 
        unsigned char pucData[64]);


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlSHA1Create()

    Purpose:    SHA1 initialization. Begins an SHA1 operation, writing a new context

    Parameters: ppvSHA1     return pointer for the SHA1 structure        

    Globals:    none

    Returns:    UTL error code

*/
int iUtlSHA1Create 
(
    void **ppvSHA1
)
{

    struct utlSHA1Context       *puscUtlSHA1Context = NULL;
    

    /* Check the parameters */
    if ( ppvSHA1 == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppvSHA1' parameter passed to 'iUtlSHA1Create'."); 
        return (UTL_ReturnParameterError);
    }
    
    
    /* Allocate the SHA1 structure */
    if ( (puscUtlSHA1Context = (struct utlSHA1Context *)s_malloc((size_t)(sizeof(struct utlSHA1Context)))) == NULL ) {
        return (UTL_MemError);
    }

    
    /* Set fields */
    puscUtlSHA1Context->pulTotal[0] = 0;
    puscUtlSHA1Context->pulTotal[1] = 0;

    /* Load magic initialization constants */
    puscUtlSHA1Context->pulState[0] = 0x67452301;
    puscUtlSHA1Context->pulState[1] = 0xEFCDAB89;
    puscUtlSHA1Context->pulState[2] = 0x98BADCFE;
    puscUtlSHA1Context->pulState[3] = 0x10325476;
    puscUtlSHA1Context->pulState[4] = 0xC3D2E1F0;


    /* Set the return pointer */
    *ppvSHA1 = puscUtlSHA1Context;


    return (UTL_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlSHA1Update()

    Purpose:    SHA1 block update operation. Continues an SHA1 message-digest
                operation, processing another message block, and updating the
                context.

    Parameters: pvUtlSHA1           SHA1 structure
                pucBuffer           buffer
                uiBufferLength      buffer length            

    Globals:    none

    Returns:    UTL error code

*/
int iUtlSHA1Update 
(
    void *pvUtlSHA1,
    unsigned char *pucBuffer,
    unsigned int uiBufferLength
)
{
    
    struct utlSHA1Context   *puscUtlSHA1Context = (struct utlSHA1Context *)pvUtlSHA1;
    int                     iFill = 0;
    unsigned long           ulLeft = 0;


    /* Check the parameters */
    if ( pvUtlSHA1 == NULL ) {
        iUtlLogDebug(UTL_LOG_CONTEXT, "Null 'pvUtlSHA1' parameter passed to 'iUtlSHA1Update'."); 
        return (UTL_SHA1InvalidSHA1);
    }

    if ( bUtlStringsIsStringNULL(pucBuffer) == true ) {
/*        iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pucBuffer' parameter passed to 'iUtlSHA1Update'.");  */
/*        return (UTL_SHA1InvalidBuffer); */
    }

    if ( uiBufferLength <= 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiBufferLength' parameter passed to 'iUtlSHA1Update'."); 
        return (UTL_SHA1InvalidBuffer);
    }


    if ( uiBufferLength <= 0 ) {
        return (UTL_NoError);
    }

    ulLeft = puscUtlSHA1Context->pulTotal[0] & 0x3F;
    iFill = 64 - ulLeft;

    puscUtlSHA1Context->pulTotal[0] += uiBufferLength;
    puscUtlSHA1Context->pulTotal[0] &= 0xFFFFFFFF;

    if ( puscUtlSHA1Context->pulTotal[0] < uiBufferLength ) {
        puscUtlSHA1Context->pulTotal[1]++;
    }

    if ( ulLeft && uiBufferLength >= iFill )  {
        s_memcpy(puscUtlSHA1Context->pucBuffer, pucBuffer, iFill);
        vUtlSHA1Process(puscUtlSHA1Context, puscUtlSHA1Context->pucBuffer);
        pucBuffer += iFill;
        uiBufferLength -= iFill;
        ulLeft = 0;
    }

    while ( uiBufferLength >= 64 ) {
        vUtlSHA1Process(puscUtlSHA1Context, pucBuffer);
        pucBuffer += 64;
        uiBufferLength -= 64;
    }

    if ( uiBufferLength > 0 ) {
        s_memcpy(puscUtlSHA1Context->pucBuffer + ulLeft, pucBuffer, uiBufferLength);
    }

    
    return (UTL_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlSHA1Digest()

    Purpose:    SHA1 digest generation. Ends an SHA1 message-digest operation, writing the
                the message digest

    Parameters: pvUtlSHA1       SHA1 structure
                pucDigest       digest buffer
                pucHexDigest    hex digest buffer

    Globals:    pucPaddingGlobal

    Returns:    UTL error code

*/
int iUtlSHA1Digest
(
    void *pvUtlSHA1,
    unsigned char *pucDigest,
    unsigned char *pucHexDigest
)
{

    struct utlSHA1Context   *puscUtlSHA1Context = (struct utlSHA1Context *)pvUtlSHA1;
    unsigned int            uiI = 0;
    unsigned long           ulLast = 0;
    unsigned long           ulPadding = 0;
    unsigned long           ulHigh = 0;
    unsigned long           ulLow = 0;
    unsigned char           pucMsgLength[8];


    /* Check the parameters */
    if ( pvUtlSHA1 == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvUtlSHA1' parameter passed to 'iUtlSHA1Digest'."); 
        return (UTL_SHA1InvalidSHA1);
    }

    if ( pucDigest == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pucDigest' parameter passed to 'iUtlSHA1Digest'."); 
        return (UTL_SHA1InvalidDigestBuffer);
    }

    if ( pucHexDigest == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pucHexDigest' parameter passed to 'iUtlSHA1Digest'."); 
        return (UTL_SHA1InvalidHexDigestBuffer);
    }


    ulHigh = (puscUtlSHA1Context->pulTotal[0] >> 29) | (puscUtlSHA1Context->pulTotal[1] <<    3);
    ulLow = (puscUtlSHA1Context->pulTotal[0] <<     3);

    UTL_SHA1_PUT_UINT32(ulHigh, pucMsgLength, 0);
    UTL_SHA1_PUT_UINT32(ulLow, pucMsgLength, 4);

    ulLast = puscUtlSHA1Context->pulTotal[0] & 0x3F;
    ulPadding = (ulLast < 56) ? (56 - ulLast) : (120 - ulLast);

    iUtlSHA1Update(puscUtlSHA1Context, pucSHA1PaddingGlobal, ulPadding);
    iUtlSHA1Update(puscUtlSHA1Context, pucMsgLength, 8);

    UTL_SHA1_PUT_UINT32(puscUtlSHA1Context->pulState[0], pucDigest, 0);
    UTL_SHA1_PUT_UINT32(puscUtlSHA1Context->pulState[1], pucDigest, 4);
    UTL_SHA1_PUT_UINT32(puscUtlSHA1Context->pulState[2], pucDigest, 8);
    UTL_SHA1_PUT_UINT32(puscUtlSHA1Context->pulState[3], pucDigest, 12);
    UTL_SHA1_PUT_UINT32(puscUtlSHA1Context->pulState[4], pucDigest, 16);


    /* Get the hex digest of the SHA1 key */
    for ( uiI = 0; uiI < UTL_SHA1_DIGEST_LENGTH; uiI++ ) {
        snprintf(pucHexDigest + (uiI * 2), 4, "%02x", pucDigest[uiI]);
    }

    
    return (UTL_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlSHA1Free()

    Purpose:    SHA1 free. Frees all resources allocated to the SHA1 context

    Parameters: pvUtlSHA1       SHA1 structure

    Globals:    none

    Returns:    UTL error code

*/
int iUtlSHA1Free
(
    void *pvUtlSHA1
)
{

    struct utlSHA1Context   *puscUtlSHA1Context = (struct utlSHA1Context *)pvUtlSHA1;


    /* Check the parameters */
    if ( pvUtlSHA1 == NULL ) {
        iUtlLogDebug(UTL_LOG_CONTEXT, "Null 'pvUtlSHA1' parameter passed to 'iUtlSHA1Free'."); 
        return (UTL_SHA1InvalidSHA1);
    }


    /* Free the SHA1 structure */
    s_free(puscUtlSHA1Context);

    
    return (UTL_NoError);

}


/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/


/*

    Function:   vUtlSHA1Process()

    Purpose:    MD5 basic transformation. Transforms state based on block.

    Parameters: puiState    state
                pucBlock    block

    Globals:    none

    Returns:    UTL error code

*/
static void vUtlSHA1Process
(
    struct utlSHA1Context *puscUtlSHA1Context, 
    unsigned char pucData[64]
)
{

    unsigned long    ulTemp = 0;
    unsigned long    pulW[16];
    unsigned long    ulA = 0;
    unsigned long    ulB = 0;
    unsigned long    ulC = 0;
    unsigned long    ulD = 0;
    unsigned long    ulE = 0;


    UTL_SHA1_GET_UINT32(pulW[0],  pucData,  0);
    UTL_SHA1_GET_UINT32(pulW[1],  pucData,  4);
    UTL_SHA1_GET_UINT32(pulW[2],  pucData,  8);
    UTL_SHA1_GET_UINT32(pulW[3],  pucData, 12);
    UTL_SHA1_GET_UINT32(pulW[4],  pucData, 16);
    UTL_SHA1_GET_UINT32(pulW[5],  pucData, 20);
    UTL_SHA1_GET_UINT32(pulW[6],  pucData, 24);
    UTL_SHA1_GET_UINT32(pulW[7],  pucData, 28);
    UTL_SHA1_GET_UINT32(pulW[8],  pucData, 32);
    UTL_SHA1_GET_UINT32(pulW[9],  pucData, 36);
    UTL_SHA1_GET_UINT32(pulW[10], pucData, 40);
    UTL_SHA1_GET_UINT32(pulW[11], pucData, 44);
    UTL_SHA1_GET_UINT32(pulW[12], pucData, 48);
    UTL_SHA1_GET_UINT32(pulW[13], pucData, 52);
    UTL_SHA1_GET_UINT32(pulW[14], pucData, 56);
    UTL_SHA1_GET_UINT32(pulW[15], pucData, 60);

#define UTL_SHA1_S(x,n) ((x << n) | ((x & 0xFFFFFFFF) >> (32 - n)))

#define UTL_SHA1_R(t)                                               \
(                                                                   \
    ulTemp = pulW[(t -  3) & 0x0F] ^ pulW[(t - 8) & 0x0F] ^         \
           pulW[(t - 14) & 0x0F] ^ pulW[t  & 0x0F],                 \
    (pulW[t & 0x0F] = UTL_SHA1_S(ulTemp,1))                         \
)

#define UTL_SHA1_P(a,b,c,d,e,x)                                     \
{                                                                   \
    e += UTL_SHA1_S(a,5) + UTL_SHA1_F(b,c,d) + UTL_SHA1_K + x; b = UTL_SHA1_S(b,30); \
}

    ulA = puscUtlSHA1Context->pulState[0];
    ulB = puscUtlSHA1Context->pulState[1];
    ulC = puscUtlSHA1Context->pulState[2];
    ulD = puscUtlSHA1Context->pulState[3];
    ulE = puscUtlSHA1Context->pulState[4];

#define UTL_SHA1_F(x,y,z) (z ^ (x & (y ^ z)))
#define UTL_SHA1_K 0x5A827999

    UTL_SHA1_P(ulA, ulB, ulC, ulD, ulE, pulW[0]);
    UTL_SHA1_P(ulE, ulA, ulB, ulC, ulD, pulW[1]);
    UTL_SHA1_P(ulD, ulE, ulA, ulB, ulC, pulW[2]);
    UTL_SHA1_P(ulC, ulD, ulE, ulA, ulB, pulW[3]);
    UTL_SHA1_P(ulB, ulC, ulD, ulE, ulA, pulW[4]);
    UTL_SHA1_P(ulA, ulB, ulC, ulD, ulE, pulW[5]);
    UTL_SHA1_P(ulE, ulA, ulB, ulC, ulD, pulW[6]);
    UTL_SHA1_P(ulD, ulE, ulA, ulB, ulC, pulW[7]);
    UTL_SHA1_P(ulC, ulD, ulE, ulA, ulB, pulW[8]);
    UTL_SHA1_P(ulB, ulC, ulD, ulE, ulA, pulW[9]);
    UTL_SHA1_P(ulA, ulB, ulC, ulD, ulE, pulW[10]);
    UTL_SHA1_P(ulE, ulA, ulB, ulC, ulD, pulW[11]);
    UTL_SHA1_P(ulD, ulE, ulA, ulB, ulC, pulW[12]);
    UTL_SHA1_P(ulC, ulD, ulE, ulA, ulB, pulW[13]);
    UTL_SHA1_P(ulB, ulC, ulD, ulE, ulA, pulW[14]);
    UTL_SHA1_P(ulA, ulB, ulC, ulD, ulE, pulW[15]);
    UTL_SHA1_P(ulE, ulA, ulB, ulC, ulD, UTL_SHA1_R(16));
    UTL_SHA1_P(ulD, ulE, ulA, ulB, ulC, UTL_SHA1_R(17));
    UTL_SHA1_P(ulC, ulD, ulE, ulA, ulB, UTL_SHA1_R(18));
    UTL_SHA1_P(ulB, ulC, ulD, ulE, ulA, UTL_SHA1_R(19));

#undef UTL_SHA1_K
#undef UTL_SHA1_F

#define UTL_SHA1_F(x,y,z) (x ^ y ^ z)
#define UTL_SHA1_K 0x6ED9EBA1

    UTL_SHA1_P(ulA, ulB, ulC, ulD, ulE, UTL_SHA1_R(20));
    UTL_SHA1_P(ulE, ulA, ulB, ulC, ulD, UTL_SHA1_R(21));
    UTL_SHA1_P(ulD, ulE, ulA, ulB, ulC, UTL_SHA1_R(22));
    UTL_SHA1_P(ulC, ulD, ulE, ulA, ulB, UTL_SHA1_R(23));
    UTL_SHA1_P(ulB, ulC, ulD, ulE, ulA, UTL_SHA1_R(24));
    UTL_SHA1_P(ulA, ulB, ulC, ulD, ulE, UTL_SHA1_R(25));
    UTL_SHA1_P(ulE, ulA, ulB, ulC, ulD, UTL_SHA1_R(26));
    UTL_SHA1_P(ulD, ulE, ulA, ulB, ulC, UTL_SHA1_R(27));
    UTL_SHA1_P(ulC, ulD, ulE, ulA, ulB, UTL_SHA1_R(28));
    UTL_SHA1_P(ulB, ulC, ulD, ulE, ulA, UTL_SHA1_R(29));
    UTL_SHA1_P(ulA, ulB, ulC, ulD, ulE, UTL_SHA1_R(30));
    UTL_SHA1_P(ulE, ulA, ulB, ulC, ulD, UTL_SHA1_R(31));
    UTL_SHA1_P(ulD, ulE, ulA, ulB, ulC, UTL_SHA1_R(32));
    UTL_SHA1_P(ulC, ulD, ulE, ulA, ulB, UTL_SHA1_R(33));
    UTL_SHA1_P(ulB, ulC, ulD, ulE, ulA, UTL_SHA1_R(34));
    UTL_SHA1_P(ulA, ulB, ulC, ulD, ulE, UTL_SHA1_R(35));
    UTL_SHA1_P(ulE, ulA, ulB, ulC, ulD, UTL_SHA1_R(36));
    UTL_SHA1_P(ulD, ulE, ulA, ulB, ulC, UTL_SHA1_R(37));
    UTL_SHA1_P(ulC, ulD, ulE, ulA, ulB, UTL_SHA1_R(38));
    UTL_SHA1_P(ulB, ulC, ulD, ulE, ulA, UTL_SHA1_R(39));

#undef UTL_SHA1_K
#undef UTL_SHA1_F

#define UTL_SHA1_F(x,y,z) ((x & y) | (z & (x | y)))
#define UTL_SHA1_K 0x8F1BBCDC

    UTL_SHA1_P(ulA, ulB, ulC, ulD, ulE, UTL_SHA1_R(40));
    UTL_SHA1_P(ulE, ulA, ulB, ulC, ulD, UTL_SHA1_R(41));
    UTL_SHA1_P(ulD, ulE, ulA, ulB, ulC, UTL_SHA1_R(42));
    UTL_SHA1_P(ulC, ulD, ulE, ulA, ulB, UTL_SHA1_R(43));
    UTL_SHA1_P(ulB, ulC, ulD, ulE, ulA, UTL_SHA1_R(44));
    UTL_SHA1_P(ulA, ulB, ulC, ulD, ulE, UTL_SHA1_R(45));
    UTL_SHA1_P(ulE, ulA, ulB, ulC, ulD, UTL_SHA1_R(46));
    UTL_SHA1_P(ulD, ulE, ulA, ulB, ulC, UTL_SHA1_R(47));
    UTL_SHA1_P(ulC, ulD, ulE, ulA, ulB, UTL_SHA1_R(48));
    UTL_SHA1_P(ulB, ulC, ulD, ulE, ulA, UTL_SHA1_R(49));
    UTL_SHA1_P(ulA, ulB, ulC, ulD, ulE, UTL_SHA1_R(50));
    UTL_SHA1_P(ulE, ulA, ulB, ulC, ulD, UTL_SHA1_R(51));
    UTL_SHA1_P(ulD, ulE, ulA, ulB, ulC, UTL_SHA1_R(52));
    UTL_SHA1_P(ulC, ulD, ulE, ulA, ulB, UTL_SHA1_R(53));
    UTL_SHA1_P(ulB, ulC, ulD, ulE, ulA, UTL_SHA1_R(54));
    UTL_SHA1_P(ulA, ulB, ulC, ulD, ulE, UTL_SHA1_R(55));
    UTL_SHA1_P(ulE, ulA, ulB, ulC, ulD, UTL_SHA1_R(56));
    UTL_SHA1_P(ulD, ulE, ulA, ulB, ulC, UTL_SHA1_R(57));
    UTL_SHA1_P(ulC, ulD, ulE, ulA, ulB, UTL_SHA1_R(58));
    UTL_SHA1_P(ulB, ulC, ulD, ulE, ulA, UTL_SHA1_R(59));

#undef UTL_SHA1_K
#undef UTL_SHA1_F

#define UTL_SHA1_F(x,y,z) (x ^ y ^ z)
#define UTL_SHA1_K 0xCA62C1D6

    UTL_SHA1_P(ulA, ulB, ulC, ulD, ulE, UTL_SHA1_R(60));
    UTL_SHA1_P(ulE, ulA, ulB, ulC, ulD, UTL_SHA1_R(61));
    UTL_SHA1_P(ulD, ulE, ulA, ulB, ulC, UTL_SHA1_R(62));
    UTL_SHA1_P(ulC, ulD, ulE, ulA, ulB, UTL_SHA1_R(63));
    UTL_SHA1_P(ulB, ulC, ulD, ulE, ulA, UTL_SHA1_R(64));
    UTL_SHA1_P(ulA, ulB, ulC, ulD, ulE, UTL_SHA1_R(65));
    UTL_SHA1_P(ulE, ulA, ulB, ulC, ulD, UTL_SHA1_R(66));
    UTL_SHA1_P(ulD, ulE, ulA, ulB, ulC, UTL_SHA1_R(67));
    UTL_SHA1_P(ulC, ulD, ulE, ulA, ulB, UTL_SHA1_R(68));
    UTL_SHA1_P(ulB, ulC, ulD, ulE, ulA, UTL_SHA1_R(69));
    UTL_SHA1_P(ulA, ulB, ulC, ulD, ulE, UTL_SHA1_R(70));
    UTL_SHA1_P(ulE, ulA, ulB, ulC, ulD, UTL_SHA1_R(71));
    UTL_SHA1_P(ulD, ulE, ulA, ulB, ulC, UTL_SHA1_R(72));
    UTL_SHA1_P(ulC, ulD, ulE, ulA, ulB, UTL_SHA1_R(73));
    UTL_SHA1_P(ulB, ulC, ulD, ulE, ulA, UTL_SHA1_R(74));
    UTL_SHA1_P(ulA, ulB, ulC, ulD, ulE, UTL_SHA1_R(75));
    UTL_SHA1_P(ulE, ulA, ulB, ulC, ulD, UTL_SHA1_R(76));
    UTL_SHA1_P(ulD, ulE, ulA, ulB, ulC, UTL_SHA1_R(77));
    UTL_SHA1_P(ulC, ulD, ulE, ulA, ulB, UTL_SHA1_R(78));
    UTL_SHA1_P(ulB, ulC, ulD, ulE, ulA, UTL_SHA1_R(79));

#undef UTL_SHA1_K
#undef UTL_SHA1_F

    puscUtlSHA1Context->pulState[0] += ulA;
    puscUtlSHA1Context->pulState[1] += ulB;
    puscUtlSHA1Context->pulState[2] += ulC;
    puscUtlSHA1Context->pulState[3] += ulD;
    puscUtlSHA1Context->pulState[4] += ulE;


    return;

}


/*---------------------------------------------------------------------------*/
