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

    Module:     search.h

    Author:     Francois Schiettecatte (FS Consulting LLC.)

    Created:    4 February 1994

    Purpose:    This is the include file for search.c.

*/


#if !defined(SRCH_SEARCH_H)
#define SRCH_SEARCH_H


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
** Defines
*/

/* Special document ID for search reports */
#define SRCH_SEARCH_REPORT_DOCUMENT_ID                      (0)


/* Term weight default */
#define SRCH_SEARCH_TERM_WEIGHT_DEFAULT                     (1.0)

/* Feedback term weight default */
#define SRCH_SEARCH_FEEDBACK_TERM_WEIGHT_DEFAULT            (0.1)


/*---------------------------------------------------------------------------*/


/* 
** Inverse Document Frequency, eight method to choose from
*/

/* No IDF adjustment */
#define SRCH_SEARCH_NO_IDF(uiMacroTermCount, uiMacroDocumentCount, uiMacroTotalDocumentCount) (1.0)


/* Basic IDF adjustment */
#define SRCH_SEARCH_BASIC_IDF(uiMacroTermCount, uiMacroDocumentCount, uiMacroTotalDocumentCount) (1.0 / (uiMacroTermCount))


/* Evolved IDF adjustment */
#define SRCH_SEARCH_EVOLVED_IDF(uiMacroTermCount, uiMacroDocumentCount, uiMacroTotalDocumentCount) \
    (log(1 + ((float)(uiMacroTotalDocumentCount) / (uiMacroDocumentCount))))


/* Sparck Jones 1972 */
#define SRCH_SEARCH_SPARCK72_IDF(uiMacroTermCount, uiMacroDocumentCount, uiMacroTotalDocumentCount) \
    (log(((float)(uiMacroTotalDocumentCount) / (uiMacroDocumentCount)) + 1))
/*     (log2((float)(uiMacroTotalDocumentCount) / (uiMacroDocumentCount))) */


/* Croft 1979 - fails when there is only one document in the index, or the term occurs in all the documents */
#define SRCH_SEARCH_CROFT79_IDF(uiMacroTermCount, uiMacroDocumentCount, uiMacroTotalDocumentCount) \
    (log((float)((uiMacroTotalDocumentCount) - (uiMacroDocumentCount)) / (uiMacroTermCount)))


/* Cosine - fails when there is only one document in the index, or the term occurs in all the documents */
#define SRCH_SEARCH_COSINE_IDF(uiMacroTermCount, uiMacroDocumentCount, uiMacroTotalDocumentCount) \
    (log((float)(uiMacroTotalDocumentCount) / (uiMacroDocumentCount)))


/* InQuery - TREC 7 */
#define SRCH_SEARCH_INQUERY_NDL     (1.0)           /* Normalized document length = (document length) / (average document length) */
#define SRCH_SEARCH_INQUERY_IDF(uiMacroTermCount, uiMacroDocumentCount, uiMacroTotalDocumentCount) \
    (0.4 + (0.6 * ((float)(uiMacroTermCount) / ((uiMacroTermCount) + 0.5 + (1.5 * (SRCH_SEARCH_INQUERY_NDL)))) * \
            (log(((uiMacroTotalDocumentCount) + 0.5) / (uiMacroDocumentCount)) / log((uiMacroTotalDocumentCount) + 1.0))))


/* BM25 - SPIRE 2000 */
#define SRCH_SEARCH_BM25_K1         (1.2)           /* Modifies influence of term frequency, range: 1 - 8 */
#define SRCH_SEARCH_BM25_B          (0.75)          /* Modifies effect of document length, range: 0 - 1 */
#define SRCH_SEARCH_BM25_NDL        (1)             /* Normalized document length = (document length) / (average document length) */
#define SRCH_SEARCH_BM25_IDF(uiMacroTermCount, uiMacroDocumentCount, uiMacroTotalDocumentCount) \
    ((((log(uiMacroTotalDocumentCount) - log(uiMacroDocumentCount)) * ((float)(uiMacroTermCount)) * (SRCH_SEARCH_BM25_K1)) + 1.0) / \
    ((SRCH_SEARCH_BM25_K1 * ((1.0 - SRCH_SEARCH_BM25_B) + (SRCH_SEARCH_BM25_B * SRCH_SEARCH_BM25_NDL))) + (uiMacroDocumentCount)))



/* Map the IDF we want */
#define SRCH_SEARCH_IDF_FACTOR(uiMacroTermCount, uiMacroDocumentCount, uiMacroTotalDocumentCount) \
    SRCH_SEARCH_COSINE_IDF(uiMacroTermCount, uiMacroDocumentCount, uiMacroTotalDocumentCount) 


/*---------------------------------------------------------------------------*/


/*
** Structures
*/

/* Search structure which gets added to the client pointer of the SPI Server Handle */
struct srchSearch {

    unsigned char   pucIndexDirectoryPath[UTL_FILE_PATH_MAX + 1];               /* Copy of the index directory path */
    unsigned char   pucConfigurationDirectoryPath[UTL_FILE_PATH_MAX + 1];       /* Copy of the configuration directory path */
    unsigned char   pucTemporaryDirectoryPath[UTL_FILE_PATH_MAX + 1];           /* Copy of the temporary directory path */

    unsigned char   pucConfigurationFilePath[UTL_FILE_PATH_MAX + 1];            /* Search configuration file path */
    time_t          tConfigurationFileLastStatusChange;                         /* Search configuration file last status change time */

    void            *pvUtlConfig;                                               /* Search configuration structure */
    void            *pvSrchParser;                                              /* Search parser structure */
    void            *pvSrchReport;                                              /* Search report structure */
    void            *pvSrchCache;                                               /* Search cache structure */

    void            *pvLngUnicodeNormalizer;                                    /* Unicode normalizer */
    void            *pvLngConverter;                                            /* Character set converter */
};


/*---------------------------------------------------------------------------*/


/* 
** C++ wrapper
*/

#if defined(__cplusplus)
}
#endif    /* defined(__cplusplus) */


/*---------------------------------------------------------------------------*/


#endif    /* !defined(SRCH_SEARCH_H) */


/*---------------------------------------------------------------------------*/
