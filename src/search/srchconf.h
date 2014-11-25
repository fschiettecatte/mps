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

    Module:     srchconf.h

    Author:     Francois Schiettecatte (FS Consulting LLC.)

    Created:    29 November 1995

    Purpose:    This file defines the configuration file symbol names.

*/


/*---------------------------------------------------------------------------*/


#if !defined(SRCH_SEARCH_CONFIG_H)
#define SRCH_SEARCH_CONFIG_H


/*---------------------------------------------------------------------------*/


/*
** Includes
*/

#include "srch.h"


/*---------------------------------------------------------------------------*/


/* C++ wrapper */
#if defined(__cplusplus)
extern "C" {
#endif    /* defined(__cplusplus) */


/*---------------------------------------------------------------------------*/


/*
** Defines
*/

/*
**  - version
**      - version
**
**  - index configuration
**      - virtual index
**      - virtual index open error
**      - virtual index sort orders
**
**    - parser configuration
**        - parser sort
**        - parser boolean operator
**        - parser boolean operation
**        - parser operator case
**        - parser term case
**        - parser search type
**        - parser frequent terms
**
**    - search configuration
**        - search maximum documents returned
**        - search term weight
**        - search feedback term weight
**        - search frequent term coverage threshold
**        - search feedback minimum term count
**        - search feedback maximum term percentage
**        - search feedback maximum term coverage threshold
**
**    - search report configuration
**        - search report location
**        - search report subdirectory length
**        - search report subdirectory depth
**
**    - search cache configuration
**        - search cache location
**        - search cache mode
**        - search cache subdirectory length
**        - search cache subdirectory depth
**
**    - filter files configuration
**        - filter files location
**
*/    
#define SRCH_SEARCH_CONFIG_FILE_NAME                                            (unsigned char *)"search.cf"
    
#define SRCH_SEARCH_CONFIG_VERSION                                              (unsigned char *)"version"
    
#define SRCH_SEARCH_CONFIG_VIRTUAL_INDEX                                        (unsigned char *)"virtual-index"
#define SRCH_SEARCH_CONFIG_VIRTUAL_INDEX_OPEN_ERROR                             (unsigned char *)"virtual-index-open-error"
#define SRCH_SEARCH_CONFIG_VIRTUAL_INDEX_SORT_ORDERS                            (unsigned char *)"virtual-index-sort-orders"
    
#define SRCH_SEARCH_CONFIG_PARSER_SORT                                          (unsigned char *)"parser-sort"
#define SRCH_SEARCH_CONFIG_PARSER_BOOLEAN_OPERATOR                              (unsigned char *)"parser-boolean-operator"
#define SRCH_SEARCH_CONFIG_PARSER_BOOLEAN_OPERATION                             (unsigned char *)"parser-boolean-operation"
#define SRCH_SEARCH_CONFIG_PARSER_OPERATOR_CASE                                 (unsigned char *)"parser-operator-case"
#define SRCH_SEARCH_CONFIG_PARSER_TERM_CASE                                     (unsigned char *)"parser-term-case"
#define SRCH_SEARCH_CONFIG_PARSER_SEARCH_TYPE                                   (unsigned char *)"parser-search-type"
#define SRCH_SEARCH_CONFIG_PARSER_FREQUENT_TERMS                                (unsigned char *)"parser-frequent-terms"
    
#define SRCH_SEARCH_CONFIG_SEARCH_MAXIMUM_DOCUMENTS_RETURNED                    (unsigned char *)"search-maximum-documents-returned"
#define SRCH_SEARCH_CONFIG_SEARCH_TERM_WEIGHT                                   (unsigned char *)"search-term-weight"
#define SRCH_SEARCH_CONFIG_SEARCH_FEEDBACK_TERM_WEIGHT                          (unsigned char *)"search-feedback-term-weight"
#define SRCH_SEARCH_CONFIG_SEARCH_FREQUENT_TERM_COVERAGE_THRESHOLD              (unsigned char *)"search-frequent-term-coverage-threshold"
#define SRCH_SEARCH_CONFIG_SEARCH_FEEDBACK_MINIMUM_TERM_COUNT                   (unsigned char *)"search-feedback-minimum-term-count"
#define SRCH_SEARCH_CONFIG_SEARCH_FEEDBACK_MAXIMUM_TERM_PERCENTAGE              (unsigned char *)"search-feedback-maximum-term-percentage"
#define SRCH_SEARCH_CONFIG_SEARCH_FEEDBACK_MAXIMUM_TERM_COVERAGE_THRESHOLD      (unsigned char *)"search-feedback-maximum-term-coverage-threshold"
    
#define SRCH_SEARCH_CONFIG_SEARCH_REPORT_LOCATION                               (unsigned char *)"search-report-location"
#define SRCH_SEARCH_CONFIG_SEARCH_REPORT_SUBDIRECTORY_MASK                      (unsigned char *)"search-report-subdirectory-mask"
    
#define SRCH_SEARCH_CONFIG_SEARCH_CACHE_LOCATION                                (unsigned char *)"search-cache-location"
#define SRCH_SEARCH_CONFIG_SEARCH_CACHE_SUBDIRECTORY_MASK                       (unsigned char *)"search-cache-subdirectory-mask"
#define SRCH_SEARCH_CONFIG_SEARCH_CACHE_MODE                                    (unsigned char *)"search-cache-mode"
    
#define SRCH_SEARCH_CONFIG_FILTER_FILES_LOCATION                                (unsigned char *)"filter-files-location"
    

/*---------------------------------------------------------------------------*/


/* 
** C++ wrapper
*/

#if defined(__cplusplus)
}
#endif    /* defined(__cplusplus) */


/*---------------------------------------------------------------------------*/


#endif    /* !defined(SRCH_SEARCH_CONFIG_H) */


/*---------------------------------------------------------------------------*/
