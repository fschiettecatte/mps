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

    Module:     data.h

    Author:     Francois Schiettecatte (FS Consulting LLC.)

    Created:    29 Aug 2010

    Purpose:    This is the header file for data.c.

*/



/*---------------------------------------------------------------------------*/


#if !defined(UTL_DATA_H)
#define UTL_DATA_H


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

int iUtlDataCreate (unsigned char *pucDataFilePath, void **ppvUtlData);

int iUtlDataAddEntry (void *ppvUtlData, void *pvDataEntryData, 
        unsigned int uiDataEntryLength, unsigned long *pulDataEntryID);

int iUtlDataOpen (unsigned char *pucDataFilePath, void **ppvUtlData);

int iUtlDataGetEntry (void *pvUtlData, unsigned long ulDataEntryID, 
        void **ppvDataEntryData, unsigned int *puiDataEntryLength);

int iUtlDataProcessEntry (void *pvUtlData, unsigned long ulDataEntryID, 
        int (*iUtlDataCallBackFunction)(), ...);

int iUtlDataClose (void *pvUtlData);


/*---------------------------------------------------------------------------*/


/* 
** C++ wrapper
*/

#if defined(__cplusplus)
}
#endif    /* defined(__cplusplus) */


/*---------------------------------------------------------------------------*/


#endif    /* !defined(UTL_DATA_H) */


/*---------------------------------------------------------------------------*/
