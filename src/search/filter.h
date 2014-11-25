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

    Module:     filter.h

    Author:     Francois Schiettecatte (FS Consulting LLC.)

    Created:    21 January 2006

    Purpose:    This module provides filter processing for search.c 

*/


#if !defined(SRCH_FILTER_H)
#define SRCH_FILTER_H


/*---------------------------------------------------------------------------*/


/*
** Includes
*/

#include "srch.h"


/*---------------------------------------------------------------------------*/


/*
** Defines
*/

/* Bitmap merge type */
#define SRCH_FILTER_BITMAP_MERGE_TYPE_INVALID           (0)
#define SRCH_FILTER_BITMAP_MERGE_TYPE_XOR               (1)
#define SRCH_FILTER_BITMAP_MERGE_TYPE_OR                (2)
#define SRCH_FILTER_BITMAP_MERGE_TYPE_AND               (3)
#define SRCH_FILTER_BITMAP_MERGE_TYPE_NOT               (4)

#define SRCH_FILTER_BITMAP_MERGE_TYPE_VALID(n)          (((n) >= SRCH_FILTER_BITMAP_MERGE_TYPE_XOR) && \
                                                                ((n) <= SRCH_FILTER_BITMAP_MERGE_TYPE_NOT))


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

int iSrchFilterGetSearchBitmapFromFilters (struct srchSearch *pssSrchSearch, 
        struct srchIndex *psiSrchIndex, unsigned int uiLanguageID, 
        struct srchParserFilter *pspfSrchParserFilters, unsigned int uiSrchParserFiltersLength,
        unsigned int uiStartDocumentID, unsigned int uiEndDocumentID, 
        unsigned int uiBitmapMergeType, struct srchBitmap **ppsbSrchBitmap);


/*---------------------------------------------------------------------------*/


/* 
** C++ wrapper
*/

#if defined(__cplusplus)
}
#endif    /* defined(__cplusplus) */


/*---------------------------------------------------------------------------*/


#endif    /* !defined(SRCH_FILTER_H) */


/*---------------------------------------------------------------------------*/
