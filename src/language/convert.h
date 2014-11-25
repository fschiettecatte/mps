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

    Module:     convert.h

    Author:     Francois Schiettecatte (FS Consulting LLC.)

    Created:    8 January 2005

    Purpose:    Header file for language support
                functions located in convert.c

*/


/*---------------------------------------------------------------------------*/


#if !defined(LNG_CONVERT_H)
#define LNG_CONVERT_H


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

/* Conversion multiplier */
#define LNG_CONVERTER_WCHAR_TO_UTF_8_MULTIPLIER     (6)


/* Conversion error handling */
#define LNG_CONVERTER_SKIP_ON_ERROR                 (0)
#define LNG_CONVERTER_RETURN_ON_ERROR               (1)


/*---------------------------------------------------------------------------*/


/*
** Public function prototypes
*/

int iLngConverterCreateByName (unsigned char *pucSourceCharacterSetName, 
        unsigned char *pucDestinationCharacterSetName, void **ppvLngConverter);

int iLngConverterCreateByID (unsigned int uiSourceCharacterSetID, 
        unsigned int uiDestinationCharacterSetID, void **ppvLngConverter);

int iLngConverterConvertString (void *pvLngConverter, unsigned int uiErrorHandling,
        unsigned char *pucSourceString, unsigned int uiSourceStringLength, 
        unsigned char **ppucDestinationString, unsigned int *puiDestinationStringLength);

int iLngConverterFree (void *pvLngConverter);



int iLngConvertWideStringToUtf8_d (wchar_t *pwcSourceString, unsigned int uiSourceStringLength, 
        unsigned char **ppucDestinationString);

int iLngConvertWideStringToUtf8_s (wchar_t *pwcSourceString, unsigned int uiSourceStringLength,
        unsigned char *pucDestinationString, unsigned int uiDestinationStringLength);


int iLngConvertUtf8ToWideString_d (unsigned char *pucSourceString, unsigned int uiSourceStringLength, 
        wchar_t **ppwcDestinationString);

int iLngConvertUtf8ToWideString_s (unsigned char *pucSourceString, unsigned int uiSourceStringLength, 
        wchar_t *pwcDestinationString, unsigned int uiDestinationStringLength);


/*---------------------------------------------------------------------------*/


/* 
** C++ wrapper
*/

#if defined(__cplusplus)
}
#endif    /* defined(__cplusplus) */


/*---------------------------------------------------------------------------*/

#endif    /* !defined(LNG_CONVERT_H) */
