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

    Module:     rand.h

    Author:     Francois Schiettecatte (FS Consulting LLC.)

    Created:    20 February 2005

    Purpose:    This is the header file for rand.c

*/


/*---------------------------------------------------------------------------*/


#if !defined(UTL_RAND_H)
#define UTL_RAND_H


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

int iUtlRandSetSeed (unsigned int uiRandSeed);

int iUtlRandGetRand (unsigned int uiRandMax, unsigned int *puiRand);


/*---------------------------------------------------------------------------*/


/* 
** C++ wrapper
*/

#if defined(__cplusplus)
}
#endif    /* defined(__cplusplus) */


/*---------------------------------------------------------------------------*/


#endif    /* !defined(UTL_RAND_H) */


/*---------------------------------------------------------------------------*/
