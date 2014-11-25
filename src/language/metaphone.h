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

    Module:     metaphone.h

    Author:     Francois Schiettecatte (FS Consulting LLC.)

    Created:    10 October 1998

    Purpose:    Header file for metaphone.c, also contains the public
                functions in metaphone.c

*/


/*---------------------------------------------------------------------------*/


#if !defined(LNG_METAPHONE_H)
#define LNG_METAPHONE_H


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

/* Language metaphone names/IDs/length */
#define LNG_METAPHONE_STANDARD_NAME             (unsigned char *)"standard"

#define LNG_METAPHONE_ANY_ID                    (0)
#define LNG_METAPHONE_STANDARD_ID               (1 << 0)

#define LNG_METAPHONE_NAME_LENGTH               (8)
#define LNG_METAPHONE_KEY_LENGTH                (6)


/*---------------------------------------------------------------------------*/


/*
** Public function prototypes
*/

int iLngMetaphoneCreateByName (unsigned char *pucMetaphoneName, 
        unsigned char *pucLanguageCode, void **ppvLngMetaphone);

int iLngMetaphoneCreateByID (unsigned int uiMetaphoneID, 
        unsigned int uiLanguageID, void **ppvLngMetaphone);

int iLngMetaphoneFree (void *pvLngMetaphone);

int iLngMetaphoneGetMetaphoneCharacterList (void *pvLngMetaphone, 
        wchar_t *pwcTerm, wchar_t **ppwcMetaphoneCharacterList);

int iLngMetaphoneGetMetaphoneKey (void *pvLngMetaphone, wchar_t *pwcTerm, 
        wchar_t *pwcMetaphoneKey, unsigned int uiMetaphoneKeyLen);


/*---------------------------------------------------------------------------*/


/* 
** C++ wrapper
*/

#if defined(__cplusplus)
}
#endif    /* defined(__cplusplus) */


/*---------------------------------------------------------------------------*/


#endif    /* !defined(LNG_METAPHONE_H) */


/*---------------------------------------------------------------------------*/
