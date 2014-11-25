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

    Module:     cache.h

    Author:     Francois Schiettecatte (FS Consulting LLC.)

    Created:    20 October 1999

    Purpose:    This is the include file for cache.c.

*/


/*---------------------------------------------------------------------------*/


#if !defined(SRCH_CACHE_H)
#define SRCH_CACHE_H


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
** Public function prototypes
*/

int iSrchCacheCreate (struct srchSearch *pssSrchSearch, void **ppvSrchCache);

int iSrchCacheClose (void *pvSrchCache);


int iSrchCacheSaveSearchShortResults (void *pvSrchCache, struct srchIndex *psiSrchIndex, 
        wchar_t *pwcSearchText, wchar_t *pwcPositiveFeedbackText, wchar_t *pwcNegativeFeedbackText, 
        unsigned int uiSortType, struct srchShortResult *pssrSrchShortResults, unsigned int uiSrchShortResultsLength, 
        unsigned int uiTotalResults, double dMaxSortKey, unsigned char *pucSearchReportSnippet);

int iSrchCacheGetSearchShortResults (void *pvSrchCache, struct srchIndex *psiSrchIndex, 
        wchar_t *pwcSearchText, wchar_t *pwcPositiveFeedbackText, wchar_t *pwcNegativeFeedbackText, 
        unsigned int uiSortType, struct srchShortResult **ppssrSrchShortResults, unsigned int *puiSrchShortResultsLength,
        unsigned int *puiTotalResults, double *pdMaxSortKey, unsigned char **ppucSearchReportSnippet);


int iSrchCacheSaveSearchPostingsList (void *pvSrchCache, struct srchIndex *psiSrchIndex,
        unsigned int uiLanguageID, struct srchParserTerm *psptSrchParserTerm, 
        struct srchPostingsList *psplSrchPostingsList, unsigned char *pucSearchReportSnippet);

int iSrchCacheGetSearchPostingsList (void *pvSrchCache, struct srchIndex *psiSrchIndex, 
        unsigned int uiLanguageID, struct srchParserTerm *psptSrchParserTerm, 
        struct srchPostingsList **ppsplSrchPostingsList, unsigned char **ppucSearchReportSnippet);


int iSrchCacheSaveSearchWeight (void *pvSrchCache, struct srchIndex *psiSrchIndex, 
        wchar_t *pwcWeightName, struct srchWeight *pwSrchWeight, unsigned char *pucSearchReportSnippet);

int iSrchCacheGetSearchWeight (void *pvSrchCache, struct srchIndex *psiSrchIndex, 
        wchar_t *pwcWeightName, struct srchWeight **ppwSrchWeight, unsigned char **ppucSearchReportSnippet);


int iSrchCacheSaveSearchBitmap (void *pvSrchCache, struct srchIndex *psiSrchIndex, 
        wchar_t *pwcBitmapName, time_t tBitmapLastUpdate, struct srchBitmap *psbSrchBitmap);

int iSrchCacheGetSearchBitmap (void *pvSrchCache, struct srchIndex *psiSrchIndex, 
        wchar_t *pwcBitmapName, time_t tBitmapLastUpdate, struct srchBitmap **ppsbSrchBitmap);


/*---------------------------------------------------------------------------*/


/* 
** C++ wrapper
*/

#if defined(__cplusplus)
}
#endif    /* defined(__cplusplus) */


/*---------------------------------------------------------------------------*/


#endif    /* !defined(SRCH_CACHE_H) */


/*---------------------------------------------------------------------------*/
