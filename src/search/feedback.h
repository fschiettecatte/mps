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

    Module:     feedback.h

    Author:     Francois Schiettecatte (FS Consulting LLC.)

    Created:    21 January 2006

    Purpose:    This module provides feedback processing for search.c 

*/


#if !defined(SRCH_FEEDBACK_H)
#define SRCH_FEEDBACK_H


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

int iSrchFeedbackGetSearchWeightFromFeedbackTexts (struct srchSearch *pssSrchSearch, 
        struct srchIndex *psiSrchIndex, unsigned int uiLanguageID, 
        wchar_t *pwcPositiveFeedbackText, wchar_t *pwcNegativeFeedbackText, 
        unsigned int uiStartDocumentID, unsigned int uiEndDocumentID, 
        struct srchWeight **ppswSrchWeight);


/*---------------------------------------------------------------------------*/


/* 
** C++ wrapper
*/

#if defined(__cplusplus)
}
#endif    /* defined(__cplusplus) */


/*---------------------------------------------------------------------------*/


#endif    /* !defined(SRCH_FEEDBACK_H) */


/*---------------------------------------------------------------------------*/
