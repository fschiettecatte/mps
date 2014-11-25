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

    Module:     stemmer.h

    Author:     Francois Schiettecatte (FS Consulting LLC.)

    Created:    3 May 2004

    Purpose:    This is the header file for stemmer.c.

*/


/*---------------------------------------------------------------------------*/


#if !defined(SRCH_STEMMER_H)
#define SRCH_STEMMER_H


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

int iSrchStemmerInit (struct srchIndex *psiSrchIndex, unsigned char *pucStemmerName);

int iSrchStemmerInitFromInfo (struct srchIndex *psiSrchIndex);

boolean bSrchStemmerOn (struct srchIndex *psiSrchIndex);

int iSrchStemmerGetName (struct srchIndex *psiSrchIndex, 
        unsigned char *pucStemmerName, unsigned int uiStemmerNameLength);


/*---------------------------------------------------------------------------*/


/* 
** C++ wrapper
*/

#if defined(__cplusplus)
}
#endif    /* defined(__cplusplus) */


/*---------------------------------------------------------------------------*/


#endif    /* !defined(SRCH_STEMMER_H) */


/*---------------------------------------------------------------------------*/
