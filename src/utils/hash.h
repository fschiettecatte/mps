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

    Module:     hash.h

    Author:     Francois Schiettecatte (FS Consulting LLC.)

    Created:    28 November 1995

    Purpose:    This is the header file for hash.c.

*/


/*---------------------------------------------------------------------------*/


#if !defined(UTL_HASH_H)
#define UTL_HASH_H


/*---------------------------------------------------------------------------*/


/*
** Includes
*/

#include "utils.h"


/*---------------------------------------------------------------------------*/


/* 
** C++ wrapper
*/

#if defined(__cplusplus)
extern "C" {
#endif    /* defined(__cplusplus) */


/*---------------------------------------------------------------------------*/


/*
** Public function prototypes
*/

int iUtlHashCreate (void **ppvUtlHash);

int iUtlHashFree (void *pvUtlHash, boolean bFreeData);

int iUtlHashAdd (void *pvUtlHash, unsigned char *pucKey, void ***pppvDatum);

int iUtlHashLookup (void *pvUtlHash, unsigned char *pucKey, void ***pppvDatum);

int iUtlHashLoopOverKeys (void *pvUtlHash, int (*iUtlHashCallBackFunction)(), ...);

int iUtlHashGetEntryCount (void *pvUtlHash, unsigned int *puiEntryCount);

int iUtlHashGetMemorySize (void *pvUtlHash, size_t *pzMemorySize);

int iUtlHashPrintKeys (void *pvUtlHash, boolean bPrintData);


/*---------------------------------------------------------------------------*/


/* 
** C++ wrapper
*/

#if defined(__cplusplus)
}
#endif    /* defined(__cplusplus) */


/*---------------------------------------------------------------------------*/


#endif    /* !defined(UTL_HASH_H) */


/*---------------------------------------------------------------------------*/
