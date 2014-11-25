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

    Module:     args.h

    Author:     Francois Schiettecatte (FS Consulting LLC.)

    Created:    20 February 2005

    Purpose:    This is the header file for args.c.

*/


/*---------------------------------------------------------------------------*/


#if !defined(UTL_ARGS_H)
#define UTL_ARGS_H


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
** Public function prototypes
*/

unsigned char *pucUtlArgsPeekNextArg (int *piArgc, char ***pppcArgv);
unsigned char *pucUtlArgsGetNextArg (int *piArgc, char ***pppcArgv);


/*---------------------------------------------------------------------------*/


/* 
** C++ wrapper
*/

#if defined(__cplusplus)
}
#endif    /* defined(__cplusplus) */


/*---------------------------------------------------------------------------*/


#endif    /* !defined(UTL_ARGS_H) */


/*---------------------------------------------------------------------------*/
