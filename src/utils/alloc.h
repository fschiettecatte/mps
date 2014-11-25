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


/*{

    Module:     alloc.h

    Author:     Francois Schiettecatte (FS Consulting LLC.)

    Created:    21 January 1996

    Purpose:    This is the header file for alloc.c.

}*/


/*---------------------------------------------------------------------------*/


#if !defined(UTL_ALLOC_H)
#define UTL_ALLOC_H


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
** Public defines
*/

/* Default size in bytes */
#define UTL_ALLOC_BYTE_SIZE_DEFAULT                 (256000)


/*---------------------------------------------------------------------------*/


/*
** Public function prototypes
*/

int iUtlAllocCreate (size_t zAllocationSize, void **ppvUtlAllocator);

int iUtlAllocFree (void *pvUtlAllocator);

int iUtlAllocAllocate (void *pvUtlAllocator, size_t zSize, void **ppvData);

int iUtlAllocReallocate (void *pvUtlAllocator, void *pvData, size_t zDataSize, 
        size_t zSize, void **ppvData);

int iUtlAllocMemorySize (void *pvUtlAllocator, size_t *pzMemorySize);


/*---------------------------------------------------------------------------*/


/* 
** C++ wrapper
*/

#if defined(__cplusplus)
}
#endif    /* defined(__cplusplus) */


/*---------------------------------------------------------------------------*/


#endif    /* !defined(UTL_ALLOC_H) */


/*---------------------------------------------------------------------------*/
