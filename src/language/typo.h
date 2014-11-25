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

    Module:     typo.h

    Author:     Francois Schiettecatte (FS Consulting LLC.)

    Created:    13 September 1995

    Purpose:    Header file for typo.c, also contains the public
                functions in typo.c

*/


/*---------------------------------------------------------------------------*/


#if !defined(LNG_TYPO_H)
#define LNG_TYPO_H


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

/* Language typo names/IDs/length */
#define LNG_TYPO_STANDARD_NAME          (unsigned char *)"standard"

#define LNG_TYPO_ANY_ID                 (0)
#define LNG_TYPO_STANDARD_ID            (1 << 0)

#define LNG_TYPO_NAME_LENGTH            (8)


/*---------------------------------------------------------------------------*/


/*
** Public function prototypes
*/

int iLngTypoCreateByName (unsigned char *pucTypoName, 
        unsigned char *pucLanguageCode, void **ppvLngTypo);

int iLngTypoCreateByID (unsigned int uiTypoID, 
        unsigned int uiLanguageID, void **ppvLngTypo);

int iLngTypoFree (void *pvLngTypo);

int iLngTypoGetTypoCharacterList (void *pvLngTypo, wchar_t *pwcTerm, 
        wchar_t **ppwcTypoCharacterList);

int iLngTypoGetTypoMatch (void *pvLngTypo, wchar_t *pwcTerm, 
        wchar_t *pwcCandidateTerm, boolean bCaseSensitive, 
        unsigned int uiTypoMaxCount);


/*---------------------------------------------------------------------------*/


/* 
** C++ wrapper
*/

#if defined(__cplusplus)
}
#endif    /* defined(__cplusplus) */


/*---------------------------------------------------------------------------*/


#endif    /* !defined(LNG_TYPO_H) */


/*---------------------------------------------------------------------------*/
