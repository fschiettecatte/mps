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

    Module:     macros.h

    Author:     Francois Schiettecatte (FS Consulting LLC.)

    Created:    20 February 2005

    Purpose:    This file contains a grab bag of useful macros which did not fit
                anywhere else.

*/


/*---------------------------------------------------------------------------*/


#if !defined(UTL_MACROS_H)
#define UTL_MACROS_H


/*---------------------------------------------------------------------------*/


/*
** Includes
*/

#include "utils.h"


/*---------------------------------------------------------------------------*/


/* C++ wrapper */
#if defined(__cplusplus)
extern "C" {
#endif    /* defined(__cplusplus) */


/*---------------------------------------------------------------------------*/


/*
** Defines
*/

/* Return the maximum of two values */
#if !defined(UTL_MACROS_MAX)
#define UTL_MACROS_MAX(x, y)            (((x) > (y)) ? (x) : (y))
#endif    /* !defined(UTL_MACROS_MAX) */

/* Return the minimum of two values */
#if !defined(UTL_MACROS_MIN)
#define UTL_MACROS_MIN(x, y)            (((x) < (y)) ? (x) : (y))
#endif    /* !defined(UTL_MACROS_MIN) */



/* Macro for for free a NULL terminated list of entries */
#define UTL_MACROS_FREE_NULL_TERMINATED_LIST(ppMacroList) \
    if ( ppMacroList != NULL ) {    \
        unsigned int    uiMacroI = 0; \
        for ( uiMacroI = 0; ppMacroList[uiMacroI] != NULL; uiMacroI++ ) {   \
            s_free(ppMacroList[uiMacroI]); \
        }   \
        s_free(ppMacroList); \
    }


/* Macro for for free a NULL terminated list of entries */
#define UTL_MACROS_FREE_NUMBERED_LIST(ppMacroList, uiMacroListLength) \
    if ( (ppMacroList != NULL) && (uiMacroListLength > 0) ) {   \
        unsigned int    uiMacroI = 0; \
        for ( uiMacroI = 0; uiMacroI < uiMacroListLength; uiMacroI++ ) {    \
            s_free(ppMacroList[uiMacroI]); \
        }   \
        s_free(ppMacroList); \
        uiMacroListLength = 0; \
    }


/*---------------------------------------------------------------------------*/


/* 
** C++ wrapper
*/

#if defined(__cplusplus)
}
#endif    /* defined(__cplusplus) */


/*---------------------------------------------------------------------------*/


#endif    /* !defined(UTL_MACROS_H) */


/*---------------------------------------------------------------------------*/
