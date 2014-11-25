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

    Module:     language.h

    Author:     Francois Schiettecatte (FS Consulting LLC.)

    Created:    1 May 2004

    Purpose:    This is the header file for language.c.

*/


/*---------------------------------------------------------------------------*/


#if !defined(SRCH_LANGUAGE_H)
#define SRCH_LANGUAGE_H


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

int iSrchLanguageInit (struct srchIndex *psiSrchIndex, 
        unsigned char *pucLanguageCode, unsigned char *pucTokenizerName);

int iSrchLanguageInitFromInfo (struct srchIndex *psiSrchIndex);

int iSrchLanguageGetLanguageCode (struct srchIndex *psiSrchIndex, 
        unsigned char *pucLanguageCode, unsigned int uiLanguageCodeLength);

int iSrchLanguageGetTokenizerName (struct srchIndex *psiSrchIndex, 
        unsigned char *pucTokenizerName, unsigned int uiTokenizerNameLength);


/*---------------------------------------------------------------------------*/


/* 
** C++ wrapper
*/

#if defined(__cplusplus)
}
#endif    /* defined(__cplusplus) */


/*---------------------------------------------------------------------------*/


#endif    /* !defined(SRCH_LANGUAGE_H) */


/*---------------------------------------------------------------------------*/
