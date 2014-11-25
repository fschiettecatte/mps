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

    Module:     shortrslt.h

    Author:     Francois Schiettecatte (FS Consulting LLC.)

    Created:    21 January 2006

    Purpose:    This module provides support functions for search.c for 
                processing short results 

*/


#if !defined(SRCH_SHORT_RESULT_H)
#define SRCH_SHORT_RESULT_H


/*---------------------------------------------------------------------------*/


/*
** Includes
*/

#include "srch.h"


/*---------------------------------------------------------------------------*/


/* 
** C++ wrapper
*/

#if defined(__cplusplus)
extern "C" {
#endif    /* defined(__cplusplus) */


/*---------------------------------------------------------------------------*/


/*
** Structures
*/

/* Short result structure */
struct srchShortResult {
    unsigned int        uiDocumentID;           /* Document ID */
    union {
        double          dSortKey;               /* Double sort key */
        float           fSortKey;               /* Float sort key */
        unsigned int    uiSortKey;              /* Unsigned integer sort key - 32 bit */
        unsigned long   ulSortKey;              /* Unsigned long sort key - 64 bit */
        unsigned char   *pucSortKey;            /* Character sort key */
    };
    struct srchIndex    *psiSrchIndexPtr;       /* Search index structure */
};


/*---------------------------------------------------------------------------*/


/*
** Public function prototypes
*/

int iSrchShortResultSort (struct srchShortResult **ppssrSrchShortResults, 
        unsigned int uiSrchShortResultsLeftIndex, unsigned int uiSortType);


int iSrchShortResultSplice (struct srchShortResult **ppssrSrchShortResults,
        unsigned int *puiSrchShortResultsLength, unsigned int uiSrchShortResultsStartIndex, 
        unsigned int uiSrchShortResultsEndIndex, unsigned int uiSortType);


int iSrchShortResultFree (struct srchShortResult *pssrSrchShortResults, 
        unsigned int uiSrchShortResultsLength, unsigned int uiSortType);

int iSrchShortResultPrint (struct srchShortResult *pssrSrchShortResults,  
        unsigned int uiSrchShortResultsLength, unsigned int uiSortType);


/*---------------------------------------------------------------------------*/


/* 
** C++ wrapper
*/

#if defined(__cplusplus)
}
#endif    /* defined(__cplusplus) */


/*---------------------------------------------------------------------------*/


#endif    /* !defined(SRCH_SHORT_RESULT_H) */


/*---------------------------------------------------------------------------*/
