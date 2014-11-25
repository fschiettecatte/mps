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

    Module:     strbuf.h

    Author:     Francois Schiettecatte (FS Consulting LLC.)

    Created:    8 November 2004

    Purpose:    This is the header file for strbuf.c.

*/


/*---------------------------------------------------------------------------*/


#if !defined(UTL_STRBUF_H)
#define UTL_STRBUF_H


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
** Public function prototypes
*/

int iUtlStringBufferCreate(void **ppvUtlStringBuffer);
int iUtlStringBufferCreateWithString(unsigned char *pucString, void **ppvUtlStringBuffer);
int iUtlStringBufferAppend(void *pvUtlStringBuffer, unsigned char *pucString);
int iUtlStringBufferGetLength(void *pvUtlStringBuffer, size_t *pzStringLength);
int iUtlStringBufferGetString(void *pvUtlStringBuffer, unsigned char **ppucString);
int iUtlStringBufferClear(void *pvUtlStringBuffer);
int iUtlStringBufferFree(void *pvUtlStringBuffer, boolean bFreeString);


int iUtlWideStringBufferCreate(void **ppvUtlWideStringBuffer);
int iUtlWideStringBufferCreateWithString(wchar_t *pwcString, void **ppvUtlWideStringBuffer);
int iUtlWideStringBufferAppend(void *pvUtlWideStringBuffer, wchar_t *pwcString);
int iUtlWideStringBufferGetLength(void *pvUtlWideStringBuffer, size_t *pzStringLength);
int iUtlWideStringBufferGetString(void *pvUtlWideStringBuffer, wchar_t **ppwcString);
int iUtlWideStringBufferClear(void *pvUtlWideStringBuffer);
int iUtlWideStringBufferFree(void *pvUtlWideStringBuffer, boolean bFreeString);


/*---------------------------------------------------------------------------*/


/* 
** C++ wrapper
*/

#if defined(__cplusplus)
}
#endif    /* defined(__cplusplus) */


/*---------------------------------------------------------------------------*/


#endif    /* !defined(UTL_STRBUF_H) */


/*---------------------------------------------------------------------------*/
