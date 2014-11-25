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

    Module:     table.h

    Author:     Francois Schiettecatte (FS Consulting LLC.)

    Created:    29 Aug 2010

    Purpose:    This is the header file for table.c.

*/



/*---------------------------------------------------------------------------*/


#if !defined(UTL_TABLE_H)
#define UTL_TABLE_H


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

int iUtlTableCreate (unsigned char *pucTableFilePath, unsigned int uiTableEntryLength, 
        void **ppvUtlTable);

int iUtlTableAddEntry (void *pvUtlTable, void *pvTableEntryData, unsigned long *pulTableEntryID);

int iUtlTableOpen (unsigned char *pucTableFilePath, void **ppvUtlTable);

int iUtlTableGetEntry (void *pvUtlTable, unsigned long ulTableEntryID, 
        void **ppvTableEntryData);

int iUtlTableProcessEntry (void *pvUtlTable, unsigned long ulTableEntryID, 
        int (*iUtlTableCallBackFunction)(), ...);

int iUtlTableClose (void *pvUtlTable);


/*---------------------------------------------------------------------------*/


/* 
** C++ wrapper
*/

#if defined(__cplusplus)
}
#endif    /* defined(__cplusplus) */


/*---------------------------------------------------------------------------*/


#endif    /* !defined(UTL_TABLE_H) */


/*---------------------------------------------------------------------------*/
