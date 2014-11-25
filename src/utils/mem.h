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

    Module:     mem.h

    Author:     Francois Schiettecatte (FS Consulting LLC.)

    Created:    19 June 2007

    Purpose:    This file contain mem* style function.

*/


/*---------------------------------------------------------------------------*/


#if !defined(UTL_MEM_H)
#define UTL_MEM_H


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
** Feature defines
*/

/* We automatically turn on strings debugging if DEBUG is set */
#if defined(DEBUG)
#define DEBUG_MEM
#endif    /* defined(DEBUG) */


/*---------------------------------------------------------------------------*/


/*
** Public function prototypes
*/

/* Memory handling functions - dont use those, use the macros below */
#if defined(DEBUG_MEM)
void    *fs_memdup (void *pvSource, size_t zSourceLength, char *pcFile, size_t zLine);
#else
void    *fs_memdup (void *pvSource, size_t zSourceLength);
#endif    /* defined(DEBUG_MEM) */

/* Memory handling macros - use these */
#if defined(DEBUG_MEM)
#define s_memdup(s, n)          fs_memdup((s), (n), (__FILE__), (__LINE__))
#else
#define s_memdup(s, n)          fs_memdup((s), (n))
#endif    /* defined(DEBUG_MEM) */


/*---------------------------------------------------------------------------*/


/* 
** C++ wrapper
*/

#if defined(__cplusplus)
}
#endif    /* defined(__cplusplus) */


/*---------------------------------------------------------------------------*/


#endif    /* !defined(UTL_MEM_H) */


/*---------------------------------------------------------------------------*/
