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

    Module:     phonix.h

    Author:     Francois Schiettecatte (FS Consulting LLC.)

    Created:    14 April 1998

    Purpose:    Header file for phonix.c, also contains the public
                functions in phonix.c

*/


/*---------------------------------------------------------------------------*/


#if !defined(LNG_PHONIX_H)
#define LNG_PHONIX_H


/*---------------------------------------------------------------------------*/


/*
** Includes
*/

#include "lng.h"


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

/* Language phonix names/IDs/length */
#define LNG_PHONIX_STANDARD_NAME            (unsigned char *)"standard"

#define LNG_PHONIX_ANY_ID                   (0)
#define LNG_PHONIX_STANDARD_ID              (1 << 0)

#define LNG_PHONIX_NAME_LENGTH              (8)
#define LNG_PHONIX_KEY_LENGTH               (8)


/*---------------------------------------------------------------------------*/


/*
** Public function prototypes
*/

int iLngPhonixCreateByName (unsigned char *pucPhonixName, 
        unsigned char *pucLanguageCode, void **ppvLngPhonix);

int iLngPhonixCreateByID (unsigned int uiPhonixID, 
        unsigned int uiLanguageID, void **ppvLngPhonix);

int iLngPhonixFree (void *pvLngPhonix);

int iLngPhonixGetPhonixCharacterList (void *pvLngPhonix, wchar_t *pwcTerm, 
        wchar_t **ppwcPhonixCharacterList);

int iLngPhonixGetPhonixKey (void *pvLngPhonix, wchar_t *pwcTerm, 
        wchar_t *pwcPhonixKey, unsigned int uiPhonixKeyLen);


/*---------------------------------------------------------------------------*/


/* 
** C++ wrapper
*/

#if defined(__cplusplus)
}
#endif    /* defined(__cplusplus) */


/*---------------------------------------------------------------------------*/


#endif    /* !defined(LNG_PHONIX_H) */


/*---------------------------------------------------------------------------*/
