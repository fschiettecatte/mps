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

    Module:     sha1.h

    Author:     Francois Schiettecatte (FS Consulting LLC.)

    Created:    6 February 2007

    Purpose:    This is the header file for sha1.c.
*/


/*---------------------------------------------------------------------------*/


#if !defined(UTL_SHA1_H)
#define UTL_SHA1_H


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
** Defines
*/

#define UTL_SHA1_DIGEST_LENGTH          (20)
#define UTL_SHA1_HEX_DIGEST_LENGTH      (UTL_SHA1_DIGEST_LENGTH * 2)


/*---------------------------------------------------------------------------*/


/*
** Public function prototypes
*/

int iUtlSHA1Create (void **ppvUtlSHA1);

int iUtlSHA1Update (void *pvUtlSHA1, unsigned char *pucBuffer, unsigned int uiBufferLength);

int iUtlSHA1Digest (void *pvUtlSHA1, unsigned char *pucDigest, unsigned char *pucHexDigest);

int iUtlSHA1Free (void *pvUtlSHA1);


/*---------------------------------------------------------------------------*/


/* 
** C++ wrapper
*/

#if defined(__cplusplus)
}
#endif    /* defined(__cplusplus) */


/*---------------------------------------------------------------------------*/


#endif    /* !defined(UTL_SHA1_H) */


/*---------------------------------------------------------------------------*/
