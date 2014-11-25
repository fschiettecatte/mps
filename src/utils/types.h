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

    Module:     types.h

    Author:     Francois Schiettecatte (FS Consulting LLC.)

    Created:    19 August 2004

    Purpose:    This is the header file for type definition

*/


/*---------------------------------------------------------------------------*/


#if !defined(UTL_TYPES_H)
#define UTL_TYPES_H


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
** Types
*/

#if !defined(boolean)
typedef unsigned int            boolean;
#endif    /* !defined(boolean) */ 

#if !defined(false)
#define false                   ((boolean)0) 
#endif    /* !defined(false) */

#if !defined(true)
#define true                    (!false)
#endif    /* !defined(true) */


/*---------------------------------------------------------------------------*/


/* 
** C++ wrapper
*/

#if defined(__cplusplus)
}
#endif    /* defined(__cplusplus) */


/*---------------------------------------------------------------------------*/


#endif    /* !defined(UTL_TYPES_H) */


/*---------------------------------------------------------------------------*/
