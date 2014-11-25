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

    Module:     debug.h

    Author:     Francois Schiettecatte (FS Consulting LLC.)

    Created:    7 July 1998

    Purpose:    Set up debugging defines

*/


/*---------------------------------------------------------------------------*/


#if !defined(UTL_DEBUG_H)
#define UTL_DEBUG_H


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

/* Assertion macro, use this to wrap around a condition that must 
** always be true to test that it is true. Compiles out when DEBUG
** is not defined.
*/

#if defined(DEBUG)

#define ASSERT(f)    \
    if ( !(f) ) {    \
        iUtlLogPanic(UTL_LOG_CONTEXT, "Assertion failed, file: '%s', line: %d", __FILE__, __LINE__); \
    }

#else    /* defined(DEBUG) */

#define ASSERT(f)     {}

#endif    /* defined(DEBUG) */


/*---------------------------------------------------------------------------*/


/* 
** C++ wrapper
*/

#if defined(__cplusplus)
}
#endif    /* defined(__cplusplus) */


/*---------------------------------------------------------------------------*/


#endif    /* !defined(UTL_DEBUG_H) */


/*---------------------------------------------------------------------------*/
