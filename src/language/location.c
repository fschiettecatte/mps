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

    Module:     location.c

    Author:     Francois Schiettecatte (FS Consulting LLC.)

    Created:    30 April 2004

    Purpose:    This file contains various locale support function

*/


/*---------------------------------------------------------------------------*/


/*
** Includes
*/

#include "lng.h"


/*---------------------------------------------------------------------------*/


/*
** Feature defines
*/

/* Context for logging */
#undef UTL_LOG_CONTEXT
#define UTL_LOG_CONTEXT                     (unsigned char *)"com.fsconsult.mps.src.language.location"


/*---------------------------------------------------------------------------*/


/*
** Defines
*/


/* UTF8 names */
#define LNG_LOCATION_UTF_8_NAME     (unsigned char *)"UTF-8"
#define LNG_LOCATION_UTF8_NAME      (unsigned char *)"UTF8"


/*---------------------------------------------------------------------------*/


/*

    Function:   iLngLocationGetLocale()

    Purpose:    This function gets the current locale name

    Parameters: uiLocaleCategory        Local category
                pucLocaleName           Return pointer for the locale name
                uiLocaleNameLength      Length of the locale name

    Globals:    

    Returns:    An LNG error code

*/
int iLngLocationGetLocale
(
    unsigned int uiLocaleCategory,
    unsigned char *pucLocaleName,
    unsigned int uiLocaleNameLength
)
{
    
    unsigned char   *pucString = NULL;


    /* Check the parameters */
    if ( pucLocaleName == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pucLocaleName' parameter passed to 'iLngLocationGetLocale'."); 
        return (LNG_ReturnParameterError);
    }

    if ( uiLocaleNameLength <= 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiLocaleNameLength' parameter passed to 'iLngLocationGetLocale'."); 
        return (LNG_ReturnParameterError);
    }


    /* Get the locale name for this category */
    if ( (pucString = s_setlocale(uiLocaleCategory, NULL)) != NULL ) {
            
        /* Set the return pointer, and return */
        s_strnncpy(pucLocaleName, pucString, uiLocaleNameLength);

        return (LNG_NoError);
    }


    return (LNG_LocationInvalidLocale);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLngLocationSetLocale()

    Purpose:    This function sets the current locale name

    Parameters: uiLocaleCategory    Local category
                pucLocaleName       Locale name

    Globals:    

    Returns:    An LNG error code

*/
int iLngLocationSetLocale
(
    unsigned int uiLocaleCategory,
    unsigned char *pucLocaleName
)
{
    
    unsigned char   *pucString = NULL;

    
    /* Check the parameters */
    if ( bUtlStringsIsStringNULL(pucLocaleName) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'pucLocaleName' parameter passed to 'iLngLocationSetLocale'."); 
        return (LNG_LocationInvalidLocale);
    }


    /* Set the locale for this category */
    if ( (pucString = s_setlocale(uiLocaleCategory, pucLocaleName)) != NULL ) {
        return (LNG_NoError);
    }


    return (LNG_LocationInvalidLocale);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   bLngLocationIsLocaleUTF8()

    Purpose:    This function checks if the local name is utf-8 compliant

    Parameters: uiLocaleCategory    Local category (ignored if locale name is set)
                pucLocaleName       Locale name to test (optional)

    Globals:    

    Returns:    An LNG error code

*/
boolean bLngLocationIsLocaleUTF8
(
    unsigned int uiLocaleCategory,
    unsigned char *pucLocaleName
)
{

    unsigned char   *pucLocaleNamePtr = NULL;


    /* Set the locale name pointer */
    if ( bUtlStringsIsStringNULL(pucLocaleName) == false ) {
        pucLocaleNamePtr = pucLocaleName;
    }
    else {
        /* Get the locale name for this category */
        if ( (pucLocaleNamePtr = s_setlocale(uiLocaleCategory, NULL)) == NULL ) {
            return (false);
        }
    }


    /* Check that the locale name contains 'UTF-8' or 'UTF8' */
    if ( (s_strcasestr(pucLocaleNamePtr, LNG_LOCATION_UTF_8_NAME) != NULL) || (s_strcasestr(pucLocaleNamePtr, LNG_LOCATION_UTF8_NAME) != NULL) ) {
        return (true);
    }


    return (false);

}


/*---------------------------------------------------------------------------*/
