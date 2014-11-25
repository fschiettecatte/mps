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

    Module:     report.h

    Author:     Francois Schiettecatte (FS Consulting LLC.)

    Created:    4 April 2004

    Purpose:    This is the include file for report.c.

*/


/*---------------------------------------------------------------------------*/


#if !defined(REPORT_H)
#define REPORT_H


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

#define    REP_INDEX_NAME                   (unsigned char *)"IndexName:"
#define    REP_INDEX_COUNTS                 (unsigned char *)"IndexCounts:"
#define    REP_STEMMER_NAME                 (unsigned char *)"StemmerName:"
#define    REP_SEARCH_ORIGINAL              (unsigned char *)"SearchOriginal:"
#define    REP_SEARCH_REFORMATTED           (unsigned char *)"SearchReformatted:"
#define    REP_SEARCH_ERROR                 (unsigned char *)"SearchError:"
#define    REP_SEARCH_SETTING               (unsigned char *)"SearchSetting:"
#define    REP_SEARCH_RESTRICTION           (unsigned char *)"SearchRestriction:"
#define    REP_SEARCH_DEBUG                 (unsigned char *)"SearchDebug:"
#define    REP_SEARCH_WARNING               (unsigned char *)"SearchWarning:"
#define    REP_SEARCH_MESSAGE               (unsigned char *)"SearchMessage:"
#define    REP_SEARCH_TERM                  (unsigned char *)"SearchTerm:"
#define    REP_SEARCH_TERM_EXPANDED         (unsigned char *)"SearchTermExpanded:"
#define    REP_POSITIVE_FEEDBACK_COUNTS     (unsigned char *)"PositiveFeedbackCounts:"
#define    REP_POSITIVE_FEEDBACK_TERMS      (unsigned char *)"PositiveFeedbackTerms:"
#define    REP_NEGATIVE_FEEDBACK_COUNTS     (unsigned char *)"NegativeFeedbackCounts:"
#define    REP_NEGATIVE_FEEDBACK_TERMS      (unsigned char *)"NegativeFeedbackTerms:"
#define    REP_RETRIEVAL_COUNTS             (unsigned char *)"RetrievalCounts:"


#define    REP_TERM_STOP                    (-1)
#define    REP_TERM_NON_EXISTENT            (-2)
#define    REP_TERM_FREQUENT                (-3)


#define    REP_UNFIELDED_STRING             (unsigned char *)"*"
#define    REP_UNSTEMMED_STRING             (unsigned char *)"*"

#define    REP_UNFIELDED_WSTRING            L"*"
#define    REP_UNSTEMMED_WSTRING            L"*"


/*---------------------------------------------------------------------------*/


/* Error codes */
#define REP_NoError                         (0)
#define REP_MemError                        (-1)
#define REP_ParameterError                  (-2)
#define REP_ReturnParameterError            (-3)
#define REP_MiscError                       (-4)


#define REP_InvalidSearchReportList         (-100)
#define REP_InvalidSearchReport             (-101)


/*---------------------------------------------------------------------------*/


/*
** Public function prototypes
*/

int iRepFormatSearchReports (unsigned char *pucSearchReport, 
        unsigned char **ppucSearchReportFormatted);

int iRepMergeSearchReports (unsigned char **ppucSearchReportList, 
        unsigned char **ppucSearchReportMerged);

int iRepMergeAndFormatSearchReports (unsigned char **ppucSearchReportList, 
        unsigned char **ppucSearchReportFormatted);


/*---------------------------------------------------------------------------*/


/* 
** C++ wrapper
*/

#if defined(__cplusplus)
}
#endif    /* defined(__cplusplus) */


/*---------------------------------------------------------------------------*/


#endif    /* !defined(REPORT_H) */


/*---------------------------------------------------------------------------*/
