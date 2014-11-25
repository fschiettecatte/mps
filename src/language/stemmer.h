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

    Created:    4 February 1994

    Purpose:    This is the header file for stemmer.c.

*/


/*---------------------------------------------------------------------------*/


#if !defined(LNG_STEMMER_H)
#define LNG_STEMMER_H


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

/* Language stemmer names/IDs */
#define    LNG_STEMMER_NONE_NAME            (unsigned char *)"none"
#define    LNG_STEMMER_PLURAL_NAME          (unsigned char *)"plural"
#define    LNG_STEMMER_PORTER_NAME          (unsigned char *)"porter"
#define    LNG_STEMMER_LOVINS_NAME          (unsigned char *)"lovins"

#define    LNG_STEMMER_ANY_ID               (0)
#define    LNG_STEMMER_NONE_ID              (1 << 0)
#define    LNG_STEMMER_PLURAL_ID            (1 << 1)
#define    LNG_STEMMER_PORTER_ID            (1 << 2)
#define    LNG_STEMMER_LOVINS_ID            (1 << 3)

#define    LNG_STEMMER_NAME_LENGTH          (6)


/*---------------------------------------------------------------------------*/

/*
** Public function prototypes
*/

int iLngStemmerCreateByName (unsigned char *pucStemmerName, 
        unsigned char *pucLanguageCode, void **ppvLngStemmer);

int iLngStemmerCreateByID (unsigned int uiStemmerID, 
        unsigned int uiLanguageID, void **ppvLngStemmer);

int iLngStemmerFree (void *pvLngStemmer);

int iLngStemmerStemTerm (void *pvLngStemmer, wchar_t *pwcTerm, 
        unsigned int uiTermLength);


/*---------------------------------------------------------------------------*/


/* 
** C++ wrapper
*/

#if defined(__cplusplus)
}
#endif    /* defined(__cplusplus) */


/*---------------------------------------------------------------------------*/


#endif    /* !defined(LNG_STEMMER_H) */


/*---------------------------------------------------------------------------*/
