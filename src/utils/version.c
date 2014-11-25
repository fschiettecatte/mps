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

    Module:     version.c

    Author:     Francois Schiettecatte (FS Consulting LLC.)

    Created:    30 March 2010

    Purpose:    This file contains functions for version output.

*/


/*---------------------------------------------------------------------------*/


/*
** Includes
*/

#include "utils.h"


/*---------------------------------------------------------------------------*/


/*
** Feature defines
*/

/* Context for logging */
#undef UTL_LOG_CONTEXT
#define UTL_LOG_CONTEXT                     (unsigned char *)"com.fsconsult.mps.src.utils.version"


/*---------------------------------------------------------------------------*/


/*
** Defines
*/

/* Build message */
#if defined(OPTIMIZED)
#define UTL_VERSION_BUILD_STRING            (unsigned char *)"optimized"
#elif defined(PROFILE)
#define UTL_VERSION_BUILD_STRING            (unsigned char *)"profile"
#elif defined(COVERAGE)
#define UTL_VERSION_BUILD_STRING            (unsigned char *)"coverage"
#elif defined(DEBUG)
#define UTL_VERSION_BUILD_STRING            (unsigned char *)"debug"
#else
#define UTL_VERSION_BUILD_STRING            (unsigned char *)"default"
#endif    /* defined(OPTIMIZED) */


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlVersionGetVersionString()

    Purpose:    This creates the version string

    Parameters: pucString           string
                uiStringLength      string length

    Globals:    none

    Returns:    UTL error code

*/
int iUtlVersionGetVersionString
(
    unsigned char *pucString,
    unsigned int uiStringLength
)
{

    /* Check the parameters */
    if ( pucString == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pucString' parameter passed to 'iUtlVersionGetVersionString'."); 
        return (UTL_ReturnParameterError);
    }

    if ( uiStringLength == 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiStringLength' parameter passed to 'iUtlVersionGetVersionString'."); 
        return (UTL_ParameterError);
    }


    /* Create the version string */
    snprintf(pucString, uiStringLength, "Version: %u.%u.%u - %s (%s, %s).", 
            UTL_VERSION_MAJOR, UTL_VERSION_MINOR, UTL_VERSION_PATCH,
            UTL_VERSION_BUILD_STRING, __DATE__, __TIME__);


    return (UTL_NoError);

}


/*---------------------------------------------------------------------------*/
