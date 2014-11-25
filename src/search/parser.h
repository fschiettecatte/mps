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

    Module:     parser.h

    Author:     Francois Schiettecatte (FS Consulting LLC.)

    Created:    27 January 1994

    Purpose:    This is the header file for parser.c. This file contains all
                the public definitions for accessing the globals.

*/


/*---------------------------------------------------------------------------*/


#if !defined(SRCH_PARSER_H)
#define SRCH_PARSER_H


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

/* Invalid tag hold-all */
#define SRCH_PARSER_INVALID_ID                                              (0)


/* Token IDs */
#define SRCH_PARSER_TOKEN_LBRACKET_ID                                       (1001)
#define SRCH_PARSER_TOKEN_RBRACKET_ID                                       (1002)
#define SRCH_PARSER_TOKEN_LSQUARE_BRACKET_ID                                (1003)
#define SRCH_PARSER_TOKEN_RSQUARE_BRACKET_ID                                (1004)
#define SRCH_PARSER_TOKEN_SINGLE_QUOTE_ID                                   (1005)
#define SRCH_PARSER_TOKEN_DOUBLE_QUOTE_ID                                   (1006)

#define SRCH_PARSER_TOKEN_VALID(n)                                          (((n) >= SRCH_PARSER_TOKEN_LBRACKET_ID) && \
                                                                                    ((n) <= SRCH_PARSER_TOKEN_DOUBLE_QUOTE_ID))


/* Range IDs */
#define SRCH_PARSER_RANGE_EQUAL_ID                                          (2000)
#define SRCH_PARSER_RANGE_NOT_EQUAL_ID                                      (2001)
#define SRCH_PARSER_RANGE_GREATER_ID                                        (2002)
#define SRCH_PARSER_RANGE_LESS_ID                                           (2003)
#define SRCH_PARSER_RANGE_GREATER_OR_EQUAL_ID                               (2004)
#define SRCH_PARSER_RANGE_LESS_OR_EQUAL_ID                                  (2005)

#define SRCH_PARSER_RANGE_VALID(n)                                          (((n) >= SRCH_PARSER_RANGE_EQUAL_ID) && \
                                                                                    ((n) <= SRCH_PARSER_RANGE_LESS_OR_EQUAL_ID))


/* Operator IDs */
#define SRCH_PARSER_OPERATOR_NEAR_ID                                        (3000)
#define SRCH_PARSER_OPERATOR_ADJ_ID                                         (3001)
#define SRCH_PARSER_OPERATOR_OR_ID                                          (3002)
#define SRCH_PARSER_OPERATOR_IOR_ID                                         (3004)
#define SRCH_PARSER_OPERATOR_XOR_ID                                         (3005)
#define SRCH_PARSER_OPERATOR_AND_ID                                         (3006)
#define SRCH_PARSER_OPERATOR_NOT_ID                                         (3007)
#define SRCH_PARSER_OPERATOR_LPAREN_ID                                      (3008)
#define SRCH_PARSER_OPERATOR_RPAREN_ID                                      (3009)

#define SRCH_PARSER_OPERATOR_VALID(n)                                       (((n) >= SRCH_PARSER_OPERATOR_NEAR_ID) && \
                                                                                    ((n) <= SRCH_PARSER_OPERATOR_RPAREN_ID))


/* Modifier IDs */
#define SRCH_PARSER_MODIFIER_UNKNOWN_ID                                     (4000)

#define SRCH_PARSER_MODIFIER_SEARCH_RESULTS_ID                              (4010)      /* Class ID */
#define SRCH_PARSER_MODIFIER_SEARCH_RESULTS_RETURN_ID                       (4011)
#define SRCH_PARSER_MODIFIER_SEARCH_RESULTS_SUPPRESS_ID                     (4012)

#define SRCH_PARSER_MODIFIER_SEARCH_REPORT_ID                               (4020)      /* Class ID */
#define SRCH_PARSER_MODIFIER_SEARCH_REPORT_RETURN_ID                        (4021)
#define SRCH_PARSER_MODIFIER_SEARCH_REPORT_SUPPRESS_ID                      (4022)

#define SRCH_PARSER_MODIFIER_SEARCH_CACHE_ID                                (4030)      /* Class ID */
#define SRCH_PARSER_MODIFIER_SEARCH_CACHE_ENABLE_ID                         (4031)
#define SRCH_PARSER_MODIFIER_SEARCH_CACHE_DISABLE_ID                        (4032)

#define SRCH_PARSER_MODIFIER_DEBUG_ID                                       (4040)      /* Class ID */
#define SRCH_PARSER_MODIFIER_DEBUG_ENABLE_ID                                (4041)
#define SRCH_PARSER_MODIFIER_DEBUG_DISABLE_ID                               (4042)

#define SRCH_PARSER_MODIFIER_BOOLEAN_OPERATOR_ID                            (4050)      /* Class ID */
#define SRCH_PARSER_MODIFIER_BOOLEAN_OPERATOR_OR_ID                         (4051)
#define SRCH_PARSER_MODIFIER_BOOLEAN_OPERATOR_IOR_ID                        (4052)
#define SRCH_PARSER_MODIFIER_BOOLEAN_OPERATOR_XOR_ID                        (4053)
#define SRCH_PARSER_MODIFIER_BOOLEAN_OPERATOR_AND_ID                        (4054)
#define SRCH_PARSER_MODIFIER_BOOLEAN_OPERATOR_ADJ_ID                        (4055)
#define SRCH_PARSER_MODIFIER_BOOLEAN_OPERATOR_NEAR_ID                       (4056)

#define SRCH_PARSER_MODIFIER_BOOLEAN_OPERATION_ID                           (4060)      /* Class ID */
#define SRCH_PARSER_MODIFIER_BOOLEAN_OPERATION_RELAXED_ID                   (4061)
#define SRCH_PARSER_MODIFIER_BOOLEAN_OPERATION_STRICT_ID                    (4062)

#define SRCH_PARSER_MODIFIER_OPERATOR_CASE_ID                               (4070)      /* Class ID */
#define SRCH_PARSER_MODIFIER_OPERATOR_CASE_ANY_ID                           (4071)
#define SRCH_PARSER_MODIFIER_OPERATOR_CASE_UPPER_ID                         (4072)
#define SRCH_PARSER_MODIFIER_OPERATOR_CASE_LOWER_ID                         (4073)

#define SRCH_PARSER_MODIFIER_TERM_CASE_ID                                   (4080)      /* Class ID */
#define SRCH_PARSER_MODIFIER_TERM_CASE_DROP_ID                              (4081)
#define SRCH_PARSER_MODIFIER_TERM_CASE_KEEP_ID                              (4082)

#define SRCH_PARSER_MODIFIER_FREQUENT_TERMS_ID                              (4090)      /* Class ID */
#define SRCH_PARSER_MODIFIER_FREQUENT_TERMS_DROP_ID                         (4091)
#define SRCH_PARSER_MODIFIER_FREQUENT_TERMS_KEEP_ID                         (4092)

#define SRCH_PARSER_MODIFIER_SEARCH_TYPE_ID                                 (4100)      /* Class ID */
#define SRCH_PARSER_MODIFIER_SEARCH_TYPE_BOOLEAN_ID                         (4101)
#define SRCH_PARSER_MODIFIER_SEARCH_TYPE_FREETEXT_ID                        (4102)

#define SRCH_PARSER_MODIFIER_DATE_ID                                        (4110)

#define SRCH_PARSER_MODIFIER_UNFIELDED_SEARCH_FIELD_NAMES_ID                (4120)

#define SRCH_PARSER_MODIFIER_SORT_DEFAULT_ID                                (4130)
#define SRCH_PARSER_MODIFIER_SORT_NONE_ID                                   (4131)
#define SRCH_PARSER_MODIFIER_SORT_ID                                        (4132)

#define SRCH_PARSER_MODIFIER_EARLY_COMPLETION_ID                            (4140)      /* Class ID */
#define SRCH_PARSER_MODIFIER_EARLY_COMPLETION_ENABLE_ID                     (4141)
#define SRCH_PARSER_MODIFIER_EARLY_COMPLETION_DISABLE_ID                    (4142)

#define SRCH_PARSER_MODIFIER_TERM_WEIGHT_ID                                 (4150)
#define SRCH_PARSER_MODIFIER_FEEDBACK_TERM_WEIGHT_ID                        (4151)

#define SRCH_PARSER_MODIFIER_FREQUENT_TERM_COVERAGE_THRESHOLD_ID            (4160)

#define SRCH_PARSER_MODIFIER_FEEDBACK_MINIMUM_TERM_COUNT_ID                 (4170)
#define SRCH_PARSER_MODIFIER_FEEDBACK_MAXIMUM_TERM_PERCENTAGE_ID            (4171)
#define SRCH_PARSER_MODIFIER_FEEDBACK_MAXIMUM_TERM_COVERAGE_THRESHOLD_ID    (4172)

#define SRCH_PARSER_MODIFIER_CONNECTION_TIMEOUT_ID                          (4180)
#define SRCH_PARSER_MODIFIER_SEARCH_TIMEOUT_ID                              (4181)
#define SRCH_PARSER_MODIFIER_RETRIEVAL_TIMEOUT_ID                           (4182)
#define SRCH_PARSER_MODIFIER_INFORMATION_TIMEOUT_ID                         (4182)

#define SRCH_PARSER_MODIFIER_MAXIMUM_SEGMENTS_SEARCHED_ID                   (4190)
#define SRCH_PARSER_MODIFIER_MINIMUM_SEGMENTS_SEARCHED_ID                   (4190)

#define SRCH_PARSER_MODIFIER_EXCLUSION_FILTER_ID                            (4200)
#define SRCH_PARSER_MODIFIER_INCLUSION_FILTER_ID                            (4201)

#define SRCH_PARSER_MODIFIER_EXCLUSION_LIST_FILTER_ID                       (4210)
#define SRCH_PARSER_MODIFIER_INCLUSION_LIST_FILTER_ID                       (4211)

#define SRCH_PARSER_MODIFIER_LANGUAGE_ID                                    (4220)

#define SRCH_PARSER_MODIFIER_TAG_ID                                         (4230)


/* Sort order IDs */
#define SRCH_PARSER_SORT_ORDER_DEFAULT_ID                                   (5000)
#define SRCH_PARSER_SORT_ORDER_NONE_ID                                      (5001)
#define SRCH_PARSER_SORT_ORDER_ASC_ID                                       (5002)
#define SRCH_PARSER_SORT_ORDER_DESC_ID                                      (5003)

#define SRCH_PARSER_SORT_ORDER_VALID(n)                                     (((n) >= SRCH_PARSER_SORT_ORDER_NONE_ID) && \
                                                                                    ((n) <= SRCH_PARSER_SORT_ORDER_DESC_ID))


/* Sort field IDs */
#define SRCH_PARSER_SORT_FIELD_NAME_RELEVANCE_ID                            (6000)
#define SRCH_PARSER_SORT_FIELD_NAME_RANK_ID                                 (6001)
#define SRCH_PARSER_SORT_FIELD_NAME_DATE_ID                                 (6002)

#define SRCH_PARSER_SORT_FIELD_VALID(n)                                     (((n) >= SRCH_PARSER_SORT_FIELD_RELEVANCE_ID) && \
                                                                                    ((n) <= SRCH_PARSER_SORT_FIELD_DATE_ID))


/* Function IDs */
#define SRCH_PARSER_FUNCTION_METAPHONE_ID                                   (7000)
#define SRCH_PARSER_FUNCTION_SOUNDEX_ID                                     (7001)
#define SRCH_PARSER_FUNCTION_PHONIX_ID                                      (7002)
#define SRCH_PARSER_FUNCTION_TYPO_ID                                        (7003)
#define SRCH_PARSER_FUNCTION_REGEX_ID                                       (7004)
#define SRCH_PARSER_FUNCTION_LITERAL_ID                                     (7005)
#define SRCH_PARSER_FUNCTION_RANGE_ID                                       (7006)

#define SRCH_PARSER_FUNCTION_VALID(n)                                       (((n) >= SRCH_PARSER_FUNCTION_METAPHONE_ID) && \
                                                                                    ((n) <= SRCH_PARSER_FUNCTION_RANGE_ID))


/* Sort field names */
#define SRCH_PARSER_SORT_FIELD_NAME_RELEVANCE_WSTRING                       L"relevance"
#define SRCH_PARSER_SORT_FIELD_NAME_RANK_WSTRING                            L"rank"
#define SRCH_PARSER_SORT_FIELD_NAME_DATE_WSTRING                            L"date"

/* Sort field names (wide characters) */
#define SRCH_PARSER_SORT_FIELD_NAME_RELEVANCE_STRING                        (unsigned char *)"relevance"
#define SRCH_PARSER_SORT_FIELD_NAME_RANK_STRING                             (unsigned char *)"rank"
#define SRCH_PARSER_SORT_FIELD_NAME_DATE_STRING                             (unsigned char *)"date"


/* Wildcard field name */
#define SRCH_PARSER_WILDCARD_FIELD_NAME_STRING                              (unsigned char *)"*"

/* Wildcard field name (wide characters) */
#define SRCH_PARSER_WILDCARD_FIELD_NAME_WSTRING                             (wchar_t *)L"*"


/* Wildcard symbols (characters) */
#define SRCH_PARSER_WILDCARD_ESCAPE_CHAR                                    (unsigned char)'\\'
#define SRCH_PARSER_WILDCARD_MULTI_CHAR                                     (unsigned char)'*'
#define SRCH_PARSER_WILDCARD_SINGLE_CHAR                                    (unsigned char )'?'
#define SRCH_PARSER_WILDCARD_ALPHA_CHAR                                     (unsigned char )'@'
#define SRCH_PARSER_WILDCARD_NUMERIC_CHAR                                   (unsigned char )'%'

/* Wildcard symbols (strings) */
#define SRCH_PARSER_WILDCARD_ESCAPE_STRING                                  (unsigned char)"\\"
#define SRCH_PARSER_WILDCARD_MULTI_STRING                                   (unsigned char)"*"
#define SRCH_PARSER_WILDCARD_SINGLE_STRING                                  (unsigned char )"?"
#define SRCH_PARSER_WILDCARD_ALPHA_STRING                                   (unsigned char )"@"
#define SRCH_PARSER_WILDCARD_NUMERIC_STRING                                 (unsigned char )"%"
#define SRCH_PARSER_WILDCARDS_STRING                                        (unsigned char *)"*?@%"

/* Wildcard symbols (wide characters) */
#define SRCH_PARSER_WILDCARD_ESCAPE_WCHAR                                   L'\\'
#define SRCH_PARSER_WILDCARD_MULTI_WCHAR                                    L'*'
#define SRCH_PARSER_WILDCARD_SINGLE_WCHAR                                   L'?'
#define SRCH_PARSER_WILDCARD_ALPHA_WCHAR                                    L'@'
#define SRCH_PARSER_WILDCARD_NUMERIC_WCHAR                                  L'%'

/* Wildcard symbols (wide strings) */
#define SRCH_PARSER_WILDCARD_ESCAPE_WSTRING                                 L"\\"
#define SRCH_PARSER_WILDCARD_MULTI_WSTRING                                  L"*"
#define SRCH_PARSER_WILDCARD_SINGLE_WSTRING                                 L"?"
#define SRCH_PARSER_WILDCARD_ALPHA_WSTRING                                  L"@"
#define SRCH_PARSER_WILDCARD_NUMERIC_WSTRING                                L"%"
#define SRCH_PARSER_WILDCARDS_WSTRING                                       L"*?@%"



/* Term type IDs */
#define SRCH_PARSER_TERM_TYPE_INVALID_ID                                    (0)
#define SRCH_PARSER_TERM_TYPE_TERM_ID                                       (1)
#define SRCH_PARSER_TERM_TYPE_TERM_CLUSTER_ID                               (2)


/* Filter type IDs */
#define SRCH_PARSER_FILTER_TYPE_INVALID_ID                                  (0)
#define SRCH_PARSER_FILTER_TYPE_TERMS_ID                                    (1)
#define SRCH_PARSER_FILTER_TYPE_LIST_ID                                     (2)


/*---------------------------------------------------------------------------*/


/*
** Feature defines
*/

/* These are shared because they are used in ./src/search/termdict.c */

/* Enable the multi wildcard '*' */
#define SRCH_PARSER_ENABLE_MULTI_WILDCARD

/* Enable the single wildcard '?' */
#define SRCH_PARSER_ENABLE_SINGLE_WILDCARD

/* Enable the alpha wildcard '@' */
#define SRCH_PARSER_ENABLE_ALPHA_WILDCARD

/* Enable the numeric wildcard '%' */
#define SRCH_PARSER_ENABLE_NUMERIC_WILDCARD


/*---------------------------------------------------------------------------*/


/*
** Structures
*/

/* Parser term structure */
struct srchParserTerm {
    wchar_t         *pwcTerm;                   /* Term */
    wchar_t         *pwcFieldName;              /* Field name */
    unsigned int    uiFunctionID;               /* Function ID */
    unsigned int    uiRangeID;                  /* Range ID */
    boolean         bWildCardSearch;            /* Wildcard search flag */
    float           fTermWeight;                /* Term weight */
    boolean         bRequired;                  /* Required flag */
};


/* Parser term cluster structure */
struct srchParserTermCluster {
    void            **ppvTerms;                 /* Parser terms */
    unsigned int    *puiTermTypeIDs;            /* Parser term type ID */
    unsigned int    uiTermsLength;              /* Parser terms length */
    unsigned int    uiOperatorID;               /* Operator ID */
    int             iTermDistance;              /* Term distance */
    boolean         bDistanceOrderMatters;      /* Distance order matters flag */
};


/* Parser number structure */
struct srchParserNumber {
    unsigned long   ulNumber;                   /* Number */
    unsigned int    uiRangeID;                  /* Range ID */
};


/* Parser filter structure */
struct srchParserFilter {
    wchar_t         *pwcFilter;                 /* Filter */
    unsigned int    uiFilterTypeID;             /* Filter type ID */
};


/*---------------------------------------------------------------------------*/


/*
** Public function prototypes
*/

int iSrchParserCreate (struct srchSearch *pssSrchSearch, void **ppvSrchParser);
int iSrchParserParse (void *pvSrchParser, unsigned int uiLanguageID, 
        unsigned int uiTokenizerID, wchar_t *pwcSearchText);
int iSrchParserReset (void *pvSrchParser);
int iSrchParserFree (void *pvSrchParser);


int iSrchParserGetErrorText (int iError, unsigned char *pucErrorText, 
        unsigned int uiErrorTextLength);


int iSrchParserGetModifierID (void *pvSrchParser, unsigned int uiModifierClassID, 
        unsigned int *puiModifierID);

int iSrchParserGetModifierValue (void *pvSrchParser, unsigned int uiModifierID, 
        void *pvModifierValue);


int iSrchParserGetDates (void *pvSrchParser, 
        struct srchParserNumber **ppspnSrchParserNumberDates, 
        unsigned int *puiSrchParserNumberDatesLength);

int iSrchParserGetUnfieldedSearchFieldNames (void *pvSrchParser, wchar_t **ppwcUnfieldedSearchFieldNames);

int iSrchParserGetSort (void *pvSrchParser, wchar_t **ppwcSortFieldName, 
        unsigned int *puiSortOrderID);

int iSrchParserGetExclusionFilters (void *pvSrchParser, 
        struct srchParserFilter **ppspfSrchParserFilterExclusionFilters, 
        unsigned int *puiSrchParserFilterExclusionFiltersLength);

int iSrchParserGetInclusionFilters (void *pvSrchParser, 
        struct srchParserFilter **ppspfSrchParserFilterInclusionFilters, 
        unsigned int *puiSrchParserFilterInclusionFiltersLength);

int iSrchParserGetLanguage (void *pvSrchParser, 
        struct srchParserNumber **ppspnSrchParserNumberLanguageIDs, 
        unsigned int *puiSrchParserNumberLanguageIDsLength);


int iSrchParserGetSearchTermCount (void *pvSrchParser, unsigned int *puiSearchTermCount);

int iSrchParserGetTermCluster (void *pvSrchParser, struct srchParserTermCluster **ppsptcSrchParserTermCluster);

int iSrchParserGetNormalizedSearchText (void *pvSrchParser, wchar_t **ppwcNormalizedSearchText);
int iSrchParserGetFullNormalizedSearchText (void *pvSrchParser, wchar_t **ppwcFullNormalizedSearchText);


wchar_t *pwcSrchParserGetStringFromID(void *pvSrchParser, unsigned int uiTokenID);


/* For debug only */
int iSrchParserPrintTermCluster (void *pvSrchParser, struct srchParserTermCluster *psptcSrchParserTermCluster, 
        unsigned int uiLevel);


/*---------------------------------------------------------------------------*/


/* 
** C++ wrapper
*/

#if defined(__cplusplus)
}
#endif    /* defined(__cplusplus) */


/*---------------------------------------------------------------------------*/


#endif    /* !defined(SRCH_PARSER_H) */


/*---------------------------------------------------------------------------*/
