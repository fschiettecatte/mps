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

    Module:     termsrch.h

    Author:     Francois Schiettecatte (FS Consulting LLC.)

    Created:    21 January 2006

    Purpose:    This module provides term search processing for search.c 

*/


#if !defined(SRCH_TERM_SRCH_H)
#define SRCH_TERM_SRCH_H


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

int iSrchTermSearchGetSearchPostingsListFromTerm (struct srchSearch *pssSrchSearch, struct srchIndex *psiSrchIndex, 
        unsigned char *pucTerm, float fWeight, unsigned char *pucFieldIDBitmap, unsigned int uiFieldIDBitmapLength,
        float fFrequentTermCoverageThreshold, unsigned int uiStartDocumentID, unsigned int uiEndDocumentID,
        struct srchPostingsList **ppsplSrchPostingsList);

int iSrchTermSearchGetSearchWeightsFromTerm (struct srchSearch *pssSrchSearch, struct srchIndex *psiSrchIndex, 
        unsigned char *pucTerm, float fWeight, unsigned char *pucFieldIDBitmap, unsigned int uiFieldIDBitmapLength,
        float fFrequentTermCoverageThreshold, unsigned int uiStartDocumentID, unsigned int uiEndDocumentID,
        struct srchWeight **ppswSrchWeight);

int iSrchTermSearchGetSearchBitmapFromTerm (struct srchSearch *pssSrchSearch, struct srchIndex *psiSrchIndex, 
        unsigned char *pucTerm, unsigned char *pucFieldIDBitmap, unsigned int uiFieldIDBitmapLength, 
        float fFrequentTermCoverageThreshold, unsigned int uiStartDocumentID, unsigned int uiEndDocumentID,
        struct srchBitmap **ppsbSrchBitmap);


/*---------------------------------------------------------------------------*/


/* 
** C++ wrapper
*/

#if defined(__cplusplus)
}
#endif    /* defined(__cplusplus) */


/*---------------------------------------------------------------------------*/


#endif    /* !defined(SRCH_TERM_SRCH_H) */


/*---------------------------------------------------------------------------*/
