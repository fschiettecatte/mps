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

    Module:     keydict.h

    Author:     Francois Schiettecatte (FS Consulting LLC.)

    Created:    14 September 1995

    Purpose:    This is the header file for keydict.c.

*/


/*---------------------------------------------------------------------------*/


#if !defined(SRCH_KEYDICT_H)
#define SRCH_KEYDICT_H


/*---------------------------------------------------------------------------*/


/*
** Includes
*/

#include "srch.h"


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

int iSrchKeyDictGenerate (struct srchIndex *psiSrchIndex);

int iSrchKeyDictLookup (struct srchIndex *psiSrchIndex, 
        unsigned char *pucDocumentKey, unsigned int *puiDocumentID);


/*---------------------------------------------------------------------------*/


/* 
** C++ wrapper
*/

#if defined(__cplusplus)
}
#endif    /* defined(__cplusplus) */


/*---------------------------------------------------------------------------*/


#endif    /* !defined(SRCH_KEYDICT_H) */


/*---------------------------------------------------------------------------*/
