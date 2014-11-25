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

    Module:     tokenizer.h

    Author:     Francois Schiettecatte (FS Consulting LLC.)

    Created:    30 April 2004

    Purpose:    This is the header file for tokenizer.c.

*/


/*---------------------------------------------------------------------------*/


#if !defined(LNG_TOKENIZER_H)
#define LNG_TOKENIZER_H


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

/* Language tokenizer names/IDs - the IDs are set up as a bitmap, but are not meant to be used as such */
#define LNG_TOKENIZER_FSCLT_1_NAME              (unsigned char *)"fsclt-1"
#define LNG_TOKENIZER_FSCLT_2_NAME              (unsigned char *)"fsclt-2"

#define LNG_TOKENIZER_ANY_ID                    (0)
#define LNG_TOKENIZER_FSCLT_1_ID                (1 << 0)
#define LNG_TOKENIZER_FSCLT_2_ID                (1 << 1)

#define LNG_TOKENIZER_NAME_LENGTH               (10)


/* Tokenizer features string length */
#define LNG_TOKENIZER_FEATURES_STRING_LENGTH    (1024)


/*---------------------------------------------------------------------------*/


/*
** Public function prototypes
*/

int iLngTokenizerCreateByName (unsigned char *pucConfigurationDirectoryPath, 
        unsigned char *pucTokenizerName, unsigned char *pucLanguageCode, void **ppvLngTokenizer);

int iLngTokenizerCreateByID (unsigned char *pucConfigurationDirectoryPath, 
        unsigned int uiTokenizerID, unsigned int uiLanguageID, void **ppvLngTokenizer);

int iLngTokenizerFree (void *pvLngTokenizer);


int iLngTokenizerParseString (void *pvLngTokenizer, unsigned int uiLanguageID, 
        wchar_t *pwcString, unsigned int uiStringLength);

int iLngTokenizerGetToken (void *pvLngTokenizer, wchar_t **ppwcTokenStart, 
        wchar_t **ppwcTokenEnd);

int iLngTokenizerGetComponent (void *pvLngTokenizer, wchar_t **ppwcComponentStart, 
        wchar_t **ppwcComponentEnd);

int iLngTokenizerIsTokenNormalized (void *pvLngTokenizer, boolean *pbNormalized);


int iLngTokenizerStripTrailings (void *pvLngTokenizer, unsigned int uiLanguageID,
        wchar_t *pwcToken, unsigned int uiTokenLength, wchar_t **ppwcTokenEnd);


int iLngTokenizerGetFeaturesString (unsigned char *pucString, unsigned int uiStringLength);


/*---------------------------------------------------------------------------*/


/* 
** C++ wrapper
*/

#if defined(__cplusplus)
}
#endif    /* defined(__cplusplus) */


/*---------------------------------------------------------------------------*/


#endif    /* !defined(LNG_TOKENIZER_H) */


/*---------------------------------------------------------------------------*/
