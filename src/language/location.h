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

    Module:     location.h

    Author:     Francois Schiettecatte (FS Consulting LLC.)

    Created:    30 April 2004

    Purpose:    Header file for locale support
                functions located in location.c

*/


/*---------------------------------------------------------------------------*/


#if !defined(LNG_LOCATION_H)
#define LNG_LOCATION_H


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

/* Locale names */
#define LNG_LOCALE_POSIX_NAME           (unsigned char *)"POSIX"            /* 7 bit ascii (unused) */
#define LNG_LOCALE_EN_US_UTF_8_NAME     (unsigned char *)"en_US.UTF-8"      /* Unicode UTF-8 */

#define    LNG_LOCALE_NAME_LENGTH       (11)


/*---------------------------------------------------------------------------*/


/*
** Public function prototypes
*/

int iLngLocationGetLocale (unsigned int uiLocaleCategory, unsigned char *pucLocaleName, 
        unsigned int uiLocaleNameLen);

int iLngLocationSetLocale (unsigned int uiLocaleCategory, unsigned char *pucLocaleName);

boolean bLngLocationIsLocaleUTF8 (unsigned int uiLocaleCategory, 
        unsigned char *pucLocaleName);


/*---------------------------------------------------------------------------*/


/* 
** C++ wrapper
*/

#if defined(__cplusplus)
}
#endif    /* defined(__cplusplus) */


/*---------------------------------------------------------------------------*/


#endif    /* !defined(LNG_LOCATION_H) */


/*---------------------------------------------------------------------------*/
