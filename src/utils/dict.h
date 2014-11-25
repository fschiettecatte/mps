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

    Module:     dict.h

    Author:     Francois Schiettecatte (FS Consulting LLC.)

    Created:    31 August 2010

    Purpose:    This is the header file for dict.c.

*/


/*---------------------------------------------------------------------------*/


#if !defined(UTL_DICT_H)
#define UTL_DICT_H


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

int iUtlDictCreate (unsigned char *pucDictFilePath, 
        unsigned int uiDictKeyLength, void **ppvUtlDict);

int iUtlDictAddEntry (void *pvUtlDict, unsigned char *pucDictKey,
        void *pvDictEntryData, unsigned int uiDictEntryLength);

int iUtlDictOpen (unsigned char *pucDictFilePath, void **ppvUtlDict);

int iUtlDictGetEntry (void *pvUtlDict, unsigned char *pucDictKey, 
        void **ppvDictEntryData, unsigned int *puiDictEntryLength);

int iUtlDictProcessEntry (void *pvUtlDict, unsigned char *pucDictKey, 
        int (*iUtlDictCallBackFunction)(), ...);

int iUtlDictProcessEntryList (void *pvUtlDict, unsigned char *pucDictKey, 
        int (*iUtlDictCallBackFunction)(), ...);

int iUtlDictClose (void *pvUtlDict);

int iUtlDictList (unsigned char *pucDictFilePath);    


/*---------------------------------------------------------------------------*/


/* 
** C++ wrapper
*/

#if defined(__cplusplus)
}
#endif    /* defined(__cplusplus) */


/*---------------------------------------------------------------------------*/


#endif    /* !defined(UTL_DICT_H) */


/*---------------------------------------------------------------------------*/
