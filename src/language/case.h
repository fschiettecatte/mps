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

    Module:     case.h

    Author:     Francois Schiettecatte (FS Consulting LLC.)

    Created:    8 January 2005

    Purpose:    Header file for case.c, also contains the public
                functions in case.c

*/


/*---------------------------------------------------------------------------*/


#if !defined(LNG_CASE_H)
#define LNG_CASE_H


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
** Public function prototypes
*/

/* Functions to upcase/downcase strings */
unsigned char *pucLngCaseConvertStringToLowerCase (unsigned char *pucString);
unsigned char *pucLngCaseConvertStringToUpperCase (unsigned char *pucString);

wchar_t *pwcLngCaseConvertWideStringToLowerCase (wchar_t *pwcString);
wchar_t *pwcLngCaseConvertWideStringToUpperCase (wchar_t *pwcString);


/* Functions to test whether strings are upper/lower case */
boolean bLngCaseDoesStringContainUpperCase (unsigned char *pucString);
boolean bLngCaseDoesStringContainLowerCase (unsigned char *pucString);
boolean bLngCaseDoesStringContainMixedCase (unsigned char *pucString);
boolean bLngCaseDoesStringContainNonAlphabetic (unsigned char *pucString);
boolean bLngCaseDoesStringStartWithUpperCase (unsigned char *pucString);
boolean bLngCaseDoesStringStartWithLowerCase (unsigned char *pucString);
boolean bLngCaseIsStringAllUpperCase (unsigned char *pucString);
boolean bLngCaseIsStringAllLowerCase (unsigned char *pucString);
boolean bLngCaseIsStringAllNumeric (unsigned char *pucString);

boolean bLngCaseDoesWideStringContainUpperCase (wchar_t *pwcString);
boolean bLngCaseDoesWideStringContainLowerCase (wchar_t *pwcString);
boolean bLngCaseDoesWideStringContainMixedCase (wchar_t *pwcString);
boolean bLngCaseDoesWideStringContainNonAlphabetic (wchar_t *pwcString);
boolean bLngCaseDoesWideStringStartWithUpperCase (wchar_t *pwcString);
boolean bLngCaseDoesWideStringStartWithLowerCase (wchar_t *pwcString);
boolean bLngCaseIsWideStringAllUpperCase (wchar_t *pwcString);
boolean bLngCaseIsWideStringAllLowerCase (wchar_t *pwcString);
boolean bLngCaseIsWideStringAllNumeric (wchar_t *pwcString);


/* Functions get a character or string stripped of all accents */
unsigned char ucLngCaseStripAccentFromCharacter (unsigned char ucChar);
unsigned char *pucLngCaseStripAccentsFromString (unsigned char *pucString);

wchar_t wcLngCaseStripAccentFromWideCharacter (wchar_t wcChar);
wchar_t *pwcLngCaseStripAccentsFromWideString (wchar_t *pwcString);


/*---------------------------------------------------------------------------*/


/* 
** C++ wrapper
*/

#if defined(__cplusplus)
}
#endif    /* defined(__cplusplus) */


/*---------------------------------------------------------------------------*/


#endif    /* !defined(LNG_CASE_H) */


/*---------------------------------------------------------------------------*/
