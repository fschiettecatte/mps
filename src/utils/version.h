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

    Module:     version.h

    Author:     Francois Schiettecatte (FS Consulting LLC.)

    Created:    11 February 1999

    Purpose:    This file contains the version information

*/


/*---------------------------------------------------------------------------*/


#if !defined(UTL_VERSION_H)
#define UTL_VERSION_H


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
** Defines
*/


/* Version numbers, abstracted */
#define UTL_VERSION_MAJOR                   (11)
#define UTL_VERSION_MINOR                   (0)
#define UTL_VERSION_PATCH                   (0)


/* Copyright message */
#define UTL_VERSION_COPYRIGHT_STRING        (unsigned char *)"Copyright (C) 1993-2011 FS Consulting LLC. This work is protected under U.S. copyright laws and international copyright treaties, and all rights are reserved."


/* Version string length */
#define UTL_VERSION_STRING_LENGTH           (1024)


/*---------------------------------------------------------------------------*/


/*
** Public function prototypes
*/

    
int iUtlVersionGetVersionString (unsigned char *pucString, unsigned int uiStringLength);


/*---------------------------------------------------------------------------*/


/* 
** C++ wrapper
*/

#if defined(__cplusplus)
}
#endif    /* defined(__cplusplus) */


/*---------------------------------------------------------------------------*/


#endif    /* !defined(UTL_VERSION_H) */


/*---------------------------------------------------------------------------*/
