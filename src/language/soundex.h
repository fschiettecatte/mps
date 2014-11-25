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

    Module:     soundex.h

    Author:     Francois Schiettecatte (FS Consulting LLC.)

    Created:    1 April 1994

    Purpose:    Header file for soundex.c, also contains the public
                functions in soundex.c

*/


/*---------------------------------------------------------------------------*/


#if !defined(LNG_SOUNDEX_H)
#define LNG_SOUNDEX_H


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

/* Language soundex names/IDs/length */
#define LNG_SOUNDEX_STANDARD_NAME               (unsigned char *)"standard"
#define LNG_SOUNDEX_ALTERNATIVE_NAME            (unsigned char *)"alternative"

#define LNG_SOUNDEX_ANY_ID                      (0)
#define LNG_SOUNDEX_STANDARD_ID                 (1 << 0)
#define LNG_SOUNDEX_ALTERNATIVE_ID              (1 << 1)

#define LNG_SOUNDEX_NAME_LENGTH                 (11)
#define LNG_SOUNDEX_KEY_LENGTH                  (6)


/*---------------------------------------------------------------------------*/


/*
** Public function prototypes
*/

int iLngSoundexCreateByName (unsigned char *pucSoundexName, 
        unsigned char *pucLanguageCode, void **ppvLngSoundex);

int iLngSoundexCreateByID (unsigned int uiSoundexID, 
        unsigned int uiLanguageID, void **ppvLngSoundex);

int iLngSoundexFree (void *pvLngSoundex);

int iLngSoundexGetSoundexCharacterList (void *pvLngSoundex, 
        wchar_t *pwcTerm, wchar_t **ppwcSoundexCharacterList);

int iLngSoundexGetSoundexKey (void *pvLngSoundex, wchar_t *pwcTerm, 
        wchar_t *pwcSoundexKey, unsigned int uiSoundexKeyLen);


/*---------------------------------------------------------------------------*/


/* 
** C++ wrapper
*/

#if defined(__cplusplus)
}
#endif    /* defined(__cplusplus) */


/*---------------------------------------------------------------------------*/


#endif    /* !defined(LNG_SOUNDEX_H) */


/*---------------------------------------------------------------------------*/
