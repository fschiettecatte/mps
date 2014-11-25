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

    Module:     parser.c

    Author:     Francois Schiettecatte (FS Consulting LLC.)

    Created:    27 January 1994

    Purpose:    This module implements the search parser
    
*/


/*---------------------------------------------------------------------------*/

/*
** Includes
*/

#include "srch.h"


/*---------------------------------------------------------------------------*/


/*
** Feature defines
*/

/* Context for logging */
#undef UTL_LOG_CONTEXT
#define UTL_LOG_CONTEXT                     (unsigned char *)"com.fsconsult.mps.src.search.parser"


/* Enable this to strip control characters */
#define SRCH_PARSER_ENABLE_CONTROL_CHARACTER_STRIPPING

/* Enable this to strip double quotes, '""' becomes '"' */
#define SRCH_PARSER_ENABLE_DOUBLE_QUOTES_STRIPPING

/* Enable this to strip escaped quotes, '\"' becomes '"' */
#define SRCH_PARSER_ENABLE_ESCAPED_QUOTE_STRIPPING

/* Enable this to normalize field names, '-' becomes '_' */
#define SRCH_PARSER_ENABLE_FIELD_NAME_NORMALIZATION


/* Enable to use single quotes for phrases (in addition to double quotes) */
/* #define SRCH_PARSER_ENABLE_SINGLE_QUOTE_FOR_PHRASES */

/* Enables backslash for characters which are otherwise treated as special syntax */
#define SRCH_PARSER_ENABLE_BACKSLASH_ESCAPE

/* Enables compacting of range tokens, 'test >= foo' becomes 'test>=foo' */
/* #define SRCH_PARSER_ENABLE_RANGE_TOKEN_COMPACTING */

/* Enables fieldless operators - '>foo' is treated as a 
** valid range search as opposed to a token 
*/
/* #define SRCH_PARSER_ENABLE_FIELDLESS_OPERATOR */

/* Enable range operators in addition to equals */
/* #define SRCH_PARSER_ENABLE_RANGE_OPERATORS */


/* Enable this to skip parenthesis errors */
#define SRCH_PARSER_ENABLE_PARENTHESIS_ERROR_SKIPPING

/* Enable this to skip bracket errors */
#define SRCH_PARSER_ENABLE_BRACKET_ERROR_SKIPPING

/* Enable this to skip quote errors */
#define SRCH_PARSER_ENABLE_QUOTE_ERROR_SKIPPING

/* Enable this to skip wildcard errors */
#define SRCH_PARSER_ENABLE_WILDCARD_ERROR_SKIPPING

/* Enable this to strip leading wildcards */
#define SRCH_PARSER_LEADING_WILDCARD_STRIPPING

/* Enable this to skip invalid functions */
#define SRCH_PARSER_ENABLE_INVALID_FUNCTION_SKIPPING


/* Enable the metaphone function */
#define SRCH_PARSER_ENABLE_METAPHONE_FUNCTION

/* Enable the soundex function */
#define SRCH_PARSER_ENABLE_SOUNDEX_FUNCTION

/* Enable the phonix function */
#define SRCH_PARSER_ENABLE_PHONIX_FUNCTION

/* Enable the typo function */
#define SRCH_PARSER_ENABLE_TYPO_FUNCTION

/* Enable the regex function */
#define SRCH_PARSER_ENABLE_REGEX_FUNCTION

/* Enable the regexp function */
#define SRCH_PARSER_ENABLE_REGEXP_FUNCTION

/* Enable the literal function */
#define SRCH_PARSER_ENABLE_LITERAL_FUNCTION

/* Enable the range function */
#define SRCH_PARSER_ENABLE_RANGE_FUNCTION


/* Enable the replacing of '[term]' with 'literal[term]' */
#define SRCH_PARSER_ENABLE_LITERAL_REPLACEMENT

/* Enable the replacing of '-term' with 'not term' */
#define SRCH_PARSER_ENABLE_NOT_REPLACEMENT

/* Enable phrasing of CJK/Thai tokens */
#define SRCH_PARSER_ENABLE_CJKT_PHRASING

/* Enable adding CJK/Thai components to searches */
/* #define SRCH_PARSER_ENABLE_CJKT_COMPONENT_ADDITION */


/* Enable stripping of term trailings */
/* #define SRCH_PARSER_ENABLE_STRIPPING_TERM_TRAILINGS */

/* Enable this to prune redundant terms from the term cluster, so if 
** there are multiple instances of 'foobar', it will be reduced to 
** one instance 
*/
#define SRCH_PARSER_ENABLE_TERM_CLUSTER_PRUNNING


/*---------------------------------------------------------------------------*/


/*
** Defines
*/


/* Generic strings */
#define SRCH_PARSER_CHARACTER_COMMA_WSTRING                                         L","


/* Token strings */
#define SRCH_PARSER_CHARACTER_LBRACKET_WCHAR                                        L'{'
#define SRCH_PARSER_CHARACTER_RBRACKET_WCHAR                                        L'}'
#define SRCH_PARSER_CHARACTER_LSQUARE_BRACKET_WCHAR                                 L'['
#define SRCH_PARSER_CHARACTER_RSQUARE_BRACKET_WCHAR                                 L']'

#define SRCH_PARSER_CHARACTER_LBRACKET_WSTRING                                      L"{"
#define SRCH_PARSER_CHARACTER_RBRACKET_WSTRING                                      L"}"
#define SRCH_PARSER_CHARACTER_LSQUARE_BRACKET_WSTRING                               L"["
#define SRCH_PARSER_CHARACTER_RSQUARE_BRACKET_WSTRING                               L"]"

#define SRCH_PARSER_CHARACTER_BRACKETS_WSTRING                                      L"{}"
#define SRCH_PARSER_CHARACTER_SQUARE_BRACKETS_WSTRING                               L"[]"

#define SRCH_PARSER_CHARACTER_DOUBLE_QUOTE_WCHAR                                    L'"'
#define SRCH_PARSER_CHARACTER_SINGLE_QUOTE_WCHAR                                    L'\''
#define SRCH_PARSER_CHARACTER_BACK_SLASH_WCHAR                                      L'\\'

#define SRCH_PARSER_CHARACTER_DOUBLE_QUOTE_WSTRING                                  L"\""
#define SRCH_PARSER_CHARACTER_SINGLE_QUOTE_WSTRING                                  L"'"
#define SRCH_PARSER_CHARACTER_BACK_SLASH_WSTRING                                    L"\\"

#define SRCH_PARSER_CHARACTER_FULL_LIST_WSTRING                                     L"{}[]\"'\\"


/* Range strings */
#define SRCH_PARSER_RANGE_EQUAL_WSTRING                                             L"="
#define SRCH_PARSER_RANGE_COLON_WSTRING                                             L":"
#define SRCH_PARSER_RANGE_NOT_EQUAL_WSTRING                                         L"!="
#define SRCH_PARSER_RANGE_GREATER_WSTRING                                           L">"
#define SRCH_PARSER_RANGE_LESS_WSTRING                                              L"<"
#define SRCH_PARSER_RANGE_GREATER_OR_EQUAL_WSTRING                                  L">="
#define SRCH_PARSER_RANGE_LESS_OR_EQUAL_WSTRING                                     L"<="

#define SRCH_PARSER_RANGE_RESTRICTED_LIST_WSTRING                                   L"=:"
#define SRCH_PARSER_RANGE_FULL_LIST_WSTRING                                         L"=:<>!"
#define SRCH_PARSER_RANGE_MODIFIER_LIST_WSTRING                                     L"=<>!"

#if defined(SRCH_PARSER_ENABLE_RANGE_OPERATORS) 
#define SRCH_PARSER_RANGE_LIST_WSTRING                                              SRCH_PARSER_RANGE_FULL_LIST_WSTRING    
#else
#define SRCH_PARSER_RANGE_LIST_WSTRING                                              SRCH_PARSER_RANGE_RESTRICTED_LIST_WSTRING    
#endif    /* defined(SRCH_PARSER_ENABLE_RANGE_OPERATORS) */ 


/* Operator strings */
#define SRCH_PARSER_OPERATOR_NEAR_WSTRING                                           L"near"
#define SRCH_PARSER_OPERATOR_WITHIN_WSTRING                                         L"within"
#define SRCH_PARSER_OPERATOR_ADJ_WSTRING                                            L"adj"
#define SRCH_PARSER_OPERATOR_OR_WSTRING                                             L"or"
#define SRCH_PARSER_OPERATOR_PIPE_WSTRING                                           L"|"
#define SRCH_PARSER_OPERATOR_DOUBLE_PIPE_WSTRING                                    L"||"
#define SRCH_PARSER_OPERATOR_IOR_WSTRING                                            L"ior"
#define SRCH_PARSER_OPERATOR_XOR_WSTRING                                            L"xor"
#define SRCH_PARSER_OPERATOR_AND_WSTRING                                            L"and"
#define SRCH_PARSER_OPERATOR_AMPERSAND_WSTRING                                      L"&"
#define SRCH_PARSER_OPERATOR_DOUBLE_AMPERSAND_WSTRING                               L"&&"
#define SRCH_PARSER_OPERATOR_PLUS_WSTRING                                           L"+"
#define SRCH_PARSER_OPERATOR_NOT_WSTRING                                            L"not"
#define SRCH_PARSER_OPERATOR_ANDNOT_WSTRING                                         L"andnot"
#define SRCH_PARSER_OPERATOR_MINUS_WSTRING                                          L"-"
#define SRCH_PARSER_OPERATOR_CARET_WSTRING                                          L"^"
#define SRCH_PARSER_OPERATOR_LPAREN_WSTRING                                         L"("
#define SRCH_PARSER_OPERATOR_RPAREN_WSTRING                                         L")"

#define SRCH_PARSER_OPERATOR_PARENS_WSTRING                                         L"()"

#define SRCH_PARSER_OPERATOR_FULL_LIST_WSTRING                                      L"|&+^()"



/* Modifier strings */
#define SRCH_PARSER_MODIFIER_SEARCH_RESULTS_RETURN_WSTRING                          L"{search_results:return}"
#define SRCH_PARSER_MODIFIER_SEARCH_RESULTS_SUPPRESS_WSTRING                        L"{search_results:suppress}"
#define SRCH_PARSER_MODIFIER_SEARCH_RESULTS_RETURN_ABR_WSTRING                      L"{sr:r}"
#define SRCH_PARSER_MODIFIER_SEARCH_RESULTS_SUPPRESS_ABR_WSTRING                    L"{sr:s}"

#define SRCH_PARSER_MODIFIER_SEARCH_REPORT_RETURN_WSTRING                           L"{search_report:return}"
#define SRCH_PARSER_MODIFIER_SEARCH_REPORT_SUPPRESS_WSTRING                         L"{search_report:suppress}"
#define SRCH_PARSER_MODIFIER_SEARCH_REPORT_RETURN_ABR_WSTRING                       L"{sr:r}"
#define SRCH_PARSER_MODIFIER_SEARCH_REPORT_SUPPRESS_ABR_WSTRING                     L"{sr:s}"

#define SRCH_PARSER_MODIFIER_SEARCH_CACHE_ENABLE_WSTRING                            L"{search_cache:enable}"
#define SRCH_PARSER_MODIFIER_SEARCH_CACHE_DISABLE_WSTRING                           L"{search_cache:disable}"
#define SRCH_PARSER_MODIFIER_SEARCH_CACHE_ENABLE_ABR_WSTRING                        L"{sc:e}"
#define SRCH_PARSER_MODIFIER_SEARCH_CACHE_DISABLE_ABR_WSTRING                       L"{sc:d}"

#define SRCH_PARSER_MODIFIER_DEBUG_ENABLE_WSTRING                                   L"{debug:enable}"
#define SRCH_PARSER_MODIFIER_DEBUG_DISABLE_WSTRING                                  L"{debug:disable}"
#define SRCH_PARSER_MODIFIER_DEBUG_ENABLE_ABR_WSTRING                               L"{d:e}"
#define SRCH_PARSER_MODIFIER_DEBUG_DISABLE_ABR_WSTRING                              L"{d:d}"

#define SRCH_PARSER_MODIFIER_BOOLEAN_OPERATOR_OR_WSTRING                            L"{boolean_operator:or}"
#define SRCH_PARSER_MODIFIER_BOOLEAN_OPERATOR_IOR_WSTRING                           L"{boolean_operator:ior}"
#define SRCH_PARSER_MODIFIER_BOOLEAN_OPERATOR_XOR_WSTRING                           L"{boolean_operator:xor}"
#define SRCH_PARSER_MODIFIER_BOOLEAN_OPERATOR_AND_WSTRING                           L"{boolean_operator:and}"
#define SRCH_PARSER_MODIFIER_BOOLEAN_OPERATOR_ADJ_WSTRING                           L"{boolean_operator:adj}"
#define SRCH_PARSER_MODIFIER_BOOLEAN_OPERATOR_NEAR_WSTRING                          L"{boolean_operator:near}"
#define SRCH_PARSER_MODIFIER_BOOLEAN_OPERATOR_OR_ABR_WSTRING                        L"{bo:or}"
#define SRCH_PARSER_MODIFIER_BOOLEAN_OPERATOR_IOR_ABR_WSTRING                       L"{bo:ior}"
#define SRCH_PARSER_MODIFIER_BOOLEAN_OPERATOR_XOR_ABR_WSTRING                       L"{bo:xor}"
#define SRCH_PARSER_MODIFIER_BOOLEAN_OPERATOR_AND_ABR_WSTRING                       L"{bo:and}"
#define SRCH_PARSER_MODIFIER_BOOLEAN_OPERATOR_ADJ_ABR_WSTRING                       L"{bo:adj}"
#define SRCH_PARSER_MODIFIER_BOOLEAN_OPERATOR_NEAR_ABR_WSTRING                      L"{bo:near}"

#define SRCH_PARSER_MODIFIER_BOOLEAN_OPERATION_RELAXED_WSTRING                      L"{boolean_operation:relaxed}"
#define SRCH_PARSER_MODIFIER_BOOLEAN_OPERATION_STRICT_WSTRING                       L"{boolean_operation:strict}"
#define SRCH_PARSER_MODIFIER_BOOLEAN_OPERATION_RELAXED_ABR_WSTRING                  L"{bo:r}"
#define SRCH_PARSER_MODIFIER_BOOLEAN_OPERATION_STRICT_ABR_WSTRING                   L"{bo:s}"

#define SRCH_PARSER_MODIFIER_OPERATOR_CASE_ANY_WSTRING                              L"{operator_case:any}"
#define SRCH_PARSER_MODIFIER_OPERATOR_CASE_UPPER_WSTRING                            L"{operator_case:upper}"
#define SRCH_PARSER_MODIFIER_OPERATOR_CASE_LOWER_WSTRING                            L"{operator_case:lower}"
#define SRCH_PARSER_MODIFIER_OPERATOR_CASE_ANY_ABR_WSTRING                          L"{oc:a}"
#define SRCH_PARSER_MODIFIER_OPERATOR_CASE_UPPER_ABR_WSTRING                        L"{oc:u}"
#define SRCH_PARSER_MODIFIER_OPERATOR_CASE_LOWER_ABR_WSTRING                        L"{oc:l}"

#define SRCH_PARSER_MODIFIER_TERM_CASE_DROP_WSTRING                                 L"{term_case:drop}"
#define SRCH_PARSER_MODIFIER_TERM_CASE_KEEP_WSTRING                                 L"{term_case:keep}"
#define SRCH_PARSER_MODIFIER_TERM_CASE_DROP_ABR_WSTRING                             L"{tc:d}"
#define SRCH_PARSER_MODIFIER_TERM_CASE_KEEP_ABR_WSTRING                             L"{tc:k}"

#define SRCH_PARSER_MODIFIER_FREQUENT_TERMS_DROP_WSTRING                            L"{frequent_terms:drop}"
#define SRCH_PARSER_MODIFIER_FREQUENT_TERMS_KEEP_WSTRING                            L"{frequent_terms:keep}"
#define SRCH_PARSER_MODIFIER_FREQUENT_TERMS_DROP_ABR_WSTRING                        L"{ft:d}"
#define SRCH_PARSER_MODIFIER_FREQUENT_TERMS_KEEP_ABR_WSTRING                        L"{ft:k}"

#define SRCH_PARSER_MODIFIER_SEARCH_TYPE_BOOLEAN_WSTRING                            L"{search_type:boolean}"
#define SRCH_PARSER_MODIFIER_SEARCH_TYPE_FREETEXT_WSTRING                           L"{search_type:freetext}"
#define SRCH_PARSER_MODIFIER_SEARCH_TYPE_BOOLEAN_ABR_WSTRING                        L"{st:b}"
#define SRCH_PARSER_MODIFIER_SEARCH_TYPE_FREETEXT_ABR_WSTRING                       L"{st:f}"


/*
**  {date>1900}
**  {date=1990}
**  {date>1990}
**  {date>=1990}
**  {date<1990}
**  {date<=1990}
**  {date!=1990}
**
** The date modifier can be repeated multiple times, for example:
**
**  {date>=19900000} {date<=20000000}
**
** or:
**
**  {date>=19900000,<=20000000}
**
** This is an ansi date in the format:
**
**  YYYYMMDDHHMMSS
**  YYYYMMDDHHMM
**  YYYYMMDDHH
**  YYYYMMDD
**  YYYYMM
**  YYYY
**
** For example:
**
**  2001                -> 2001
**  200112              -> December 2001
**  20011231            -> 31 December 2001
**  20011231100000      -> 31 December 2001, 10 am
**
** The date numbers can be colon separated for clarity:
**
**  2001:12:31:10:00:00
**  2001:12:31::10:00:00
**
** The date will always be normalized into a date, as follows:
**
**  YYYYMMDDHHMMSS
*/
#define SRCH_PARSER_MODIFIER_DATE_WSTRING                                           L"{date"
#define SRCH_PARSER_MODIFIER_DATE_ABR_WSTRING                                       L"{d"


/*
**  {unfielded_search_field_names:...}
**  
**  {unfielded_search_field_names:title,content}
*/
#define SRCH_PARSER_MODIFIER_UNFIELDED_SEARCH_FIELD_NAMES_WSTRING                   L"{unfielded_search_field_names"
#define SRCH_PARSER_MODIFIER_UNFIELDED_SEARCH_FIELD_NAMES_ABR_WSTRING               L"{usfn"


/*
**  {sort:default} 
**
**  {sort:none}
**
**  {sort:...:...}
**
**  {sort:relevance:asc}        -> by relevance, ascending (internal field)
**  {sort:relevance:desc}       -> by relevance, descending (internal field)
**  {sort:date:asc}             -> by date, ascending (internal field)
**  {sort:date:desc}            -> by date, descending (internal field)
**  {sort:author:asc}           -> by author, ascending (externally defined field)
**  {sort:author:desc}          -> by author, descending (externally defined field)
*/
#define SRCH_PARSER_MODIFIER_SORT_WSTRING                                           L"{sort"
#define SRCH_PARSER_MODIFIER_SORT_DEFAULT_WSTRING                                   L"{sort:default}"
#define SRCH_PARSER_MODIFIER_SORT_NONE_WSTRING                                      L"{sort:none}"
#define SRCH_PARSER_MODIFIER_SORT_ABR_WSTRING                                       L"{s"
#define SRCH_PARSER_MODIFIER_SORT_DEFAULT_ABR_WSTRING                               L"{s:d}"
#define SRCH_PARSER_MODIFIER_SORT_NONE_ABR_WSTRING                                  L"{s:n}"

#define SRCH_PARSER_MODIFIER_EARLY_COMPLETION_ENABLE_WSTRING                        L"{early_completion:enable}"
#define SRCH_PARSER_MODIFIER_EARLY_COMPLETION_DISABLE_WSTRING                       L"{early_completion:disable}"
#define SRCH_PARSER_MODIFIER_EARLY_COMPLETION_ENABLE_ABR_WSTRING                    L"{ec:e}"
#define SRCH_PARSER_MODIFIER_EARLY_COMPLETION_DISABLE_ABR_WSTRING                   L"{ec:d}"


/*
**  {term_weight:...}
**  
**  {feedback_term_weight:...}
**
**  {term_weight:1.0}
**
**  {feedback_term_weight:0.1}
*/
#define SRCH_PARSER_MODIFIER_TERM_WEIGHT_WSTRING                                    L"{term_weight"
#define SRCH_PARSER_MODIFIER_FEEDBACK_TERM_WEIGHT_WSTRING                           L"{feedback_term_weight"
#define SRCH_PARSER_MODIFIER_TERM_WEIGHT_ABR_WSTRING                                L"{tw"
#define SRCH_PARSER_MODIFIER_FEEDBACK_TERM_WEIGHT_ABR_WSTRING                       L"{ftw"


/*    
**  {frequent_term_coverage_threshold:...}
**
**  {frequent_term_coverage_threshold:100}
*/
#define SRCH_PARSER_MODIFIER_FREQUENT_TERM_COVERAGE_THRESHOLD_WSTRING               L"{frequent_term_coverage_threshold"
#define SRCH_PARSER_MODIFIER_FREQUENT_TERM_COVERAGE_THRESHOLD_ABR_WSTRING           L"{ftct"


/*    
**  {feedback_minimum_term_count:...}
**  {feedback_maximum_term_percentage:...}
**  {feedback_maximum_term_coverage_threshold:...}
**
**  {feedback_minimum_term_count:10}
**  {feedback_maximum_term_percentage:25}
**  {frequent_term_coverage_threshold:8}
*/
#define SRCH_PARSER_MODIFIER_FEEDBACK_MINIMUM_TERM_COUNT_WSTRING                    L"{feedback_minimum_term_count"
#define SRCH_PARSER_MODIFIER_FEEDBACK_MAXIMUM_TERM_PERCENTAGE_WSTRING               L"{feedback_maximum_term_percentage"
#define SRCH_PARSER_MODIFIER_FEEDBACK_MAXIMUM_TERM_COVERAGE_THRESHOLD_WSTRING       L"{feedback_maximum_term_coverage_threshold"
#define SRCH_PARSER_MODIFIER_FEEDBACK_MINIMUM_TERM_COUNT_ABR_WSTRING                L"{fmtc"
#define SRCH_PARSER_MODIFIER_FEEDBACK_MAXIMUM_TERM_PERCENTAGE_ABR_WSTRING           L"{fmtp"
#define SRCH_PARSER_MODIFIER_FEEDBACK_MAXIMUM_TERM_COVERAGE_THRESHOLD_ABR_WSTRING   L"{fmtct"


/*
**  {connection_timeout:...}
**  {search_timeout:...}
**  {retrieval_timeout:...}
**  {information_timeout:...}
**
**  {connection_timeout:10}
**  {search_timeout:60000}
**  {retrieval_timeout:5000}
**  {information_timeout:5000}
*/
#define SRCH_PARSER_MODIFIER_CONNECTION_TIMEOUT_WSTRING                             L"{connection_timeout"
#define SRCH_PARSER_MODIFIER_SEARCH_TIMEOUT_WSTRING                                 L"{search_timeout"
#define SRCH_PARSER_MODIFIER_RETRIEVAL_TIMEOUT_WSTRING                              L"{retrieval_timeout"
#define SRCH_PARSER_MODIFIER_INFORMATION_TIMEOUT_WSTRING                            L"{information_timeout"
#define SRCH_PARSER_MODIFIER_CONNECTION_TIMEOUT_ABR_WSTRING                         L"{ct"
#define SRCH_PARSER_MODIFIER_SEARCH_TIMEOUT_ABR_WSTRING                             L"{st"
#define SRCH_PARSER_MODIFIER_RETRIEVAL_TIMEOUT_ABR_WSTRING                          L"{rt"
#define SRCH_PARSER_MODIFIER_INFORMATION_TIMEOUT_ABR_WSTRING                        L"{it"


/*
**  {segments_searched_maximum:...}
**  {segments_searched_minimum:...}
**
**  {segments_searched_maximum:10}
**  {segments_searched_minimum:1}
*/
#define SRCH_PARSER_MODIFIER_MAXIMUM_SEGMENTS_SEARCHED_WSTRING                      L"{segments_searched_maximum"
#define SRCH_PARSER_MODIFIER_MINIMUM_SEGMENTS_SEARCHED_WSTRING                      L"{segments_searched_minimum"
#define SRCH_PARSER_MODIFIER_MAXIMUM_SEGMENTS_SEARCHED_ABR_WSTRING                  L"{ssmx"
#define SRCH_PARSER_MODIFIER_MINIMUM_SEGMENTS_SEARCHED_ABR_WSTRING                  L"{ssmn"


/*
**  {exclusion_filter:...}
**  {inclusion_filter:...}
**
**  {exclusion_filter:term1,term2,term3}
**  {exclusion_filter:field_name=term1,field_name=term2,field_name=term3}
**
**  {inclusion_filter:term1,term2,term3}
**  {inclusion_filter:field_name=term1,field_name=term2,field_name=term3}
*/
#define SRCH_PARSER_MODIFIER_EXCLUSION_FILTER_WSTRING                               L"{exclusion_filter"
#define SRCH_PARSER_MODIFIER_INCLUSION_FILTER_WSTRING                               L"{inclusion_filter"
#define SRCH_PARSER_MODIFIER_EXCLUSION_FILTER_ABR_WSTRING                           L"{ef"
#define SRCH_PARSER_MODIFIER_INCLUSION_FILTER_ABR_WSTRING                           L"{if"


/*
**  {exclusion_list_filter:...}
**  {inclusion_list_filter:...}
**
**  {exclusion_list_filter:term_list}
**
**  {inclusion_list_filter:term_list}
*/
#define SRCH_PARSER_MODIFIER_EXCLUSION_LIST_FILTER_WSTRING                          L"{exclusion_list_filter"
#define SRCH_PARSER_MODIFIER_INCLUSION_LIST_FILTER_WSTRING                          L"{inclusion_list_filter"
#define SRCH_PARSER_MODIFIER_EXCLUSION_LIST_FILTER_ABR_WSTRING                      L"{elf"
#define SRCH_PARSER_MODIFIER_INCLUSION_LIST_FILTER_ABR_WSTRING                      L"{ilf"


/*
**  {language:...[,...]}
**
**  {language:en}
**  {language:en_US,en_UK}
**  {language:en,fr,es,pt}
*/
#define SRCH_PARSER_MODIFIER_LANGUAGE_WSTRING                                       L"{language"
#define SRCH_PARSER_MODIFIER_LANGUAGE_ABR_WSTRING                                   L"{l"


/*
**  {tag:...}
**
**  {tag:searchAPI-10.0.1.240}
**  {tag:searchServer-10.0.1.240}
*/  
#define SRCH_PARSER_MODIFIER_TAG_WSTRING                                            L"{tag"
#define SRCH_PARSER_MODIFIER_TAG_ABR_WSTRING                                        L"{t"


/* Sort order strings */
#define SRCH_PARSER_SORT_ORDER_ASC_WSTRING                                          L"asc"
#define SRCH_PARSER_SORT_ORDER_DESC_WSTRING                                         L"desc"
#define SRCH_PARSER_SORT_ORDER_ASC_ABR_WSTRING                                      L"a"
#define SRCH_PARSER_SORT_ORDER_DESC_ABR_WSTRING                                     L"d"


/* Sort field name strings - commented out because already defined in parser.h, here for reference only */
/* #define SRCH_PARSER_SORT_FIELD_NAME_RELEVANCE_WSTRING                               L"relevance" */
/* #define SRCH_PARSER_SORT_FIELD_NAME_RANK_WSTRING                                    L"rank" */
/* #define SRCH_PARSER_SORT_FIELD_NAME_DATE_WSTRING                                    L"date" */
#define SRCH_PARSER_SORT_FIELD_NAME_RELEVANCE_ABR_WSTRING                           L"r"
#define SRCH_PARSER_SORT_FIELD_NAME_RANK_ABR_WSTRING                                L"rk"
#define SRCH_PARSER_SORT_FIELD_NAME_DATE_ABR_WSTRING                                L"d"


/* Modifier separator string */
#define SRCH_PARSER_MODIFIER_SEPARATOR_WSTRING                                      L":"


/* Modifier date separator string */
#define SRCH_PARSER_MODIFIER_DATE_SEPARATORS_WSTRING                                L":/-_'"


/* Function strings */
#define SRCH_PARSER_FUNCTION_METAPHONE_WSTRING                                      L"metaphone"
#define SRCH_PARSER_FUNCTION_SOUNDEX_WSTRING                                        L"soundex"
#define SRCH_PARSER_FUNCTION_PHONIX_WSTRING                                         L"phonix"
#define SRCH_PARSER_FUNCTION_TYPO_WSTRING                                           L"typo"
#define SRCH_PARSER_FUNCTION_REGEX_WSTRING                                          L"regex"
#define SRCH_PARSER_FUNCTION_REGEXP_WSTRING                                         L"regexp"
#define SRCH_PARSER_FUNCTION_LITERAL_WSTRING                                        L"literal"
#define SRCH_PARSER_FUNCTION_RANGE_WSTRING                                          L"range"

#define SRCH_PARSER_FUNCTION_SOUNDSLIKE_WSTRING                                     L"soundslike"        /* synonym for metaphone[] */


/* Trailer status */
#define    SRCH_PARSER_TRAILER_INVALID                                              (0)
#define    SRCH_PARSER_TRAILER_NONE                                                 (1)
#define    SRCH_PARSER_TRAILER_OPTIONAL                                             (2)
#define    SRCH_PARSER_TRAILER_REQUIRED                                             (3)


/* String minimum length and growth factor */
#define SRCH_PARSER_MINIMUM_LENGTH                                                  (256)
#define SRCH_PARSER_GROWTH_FACTOR                                                   (10)


/* Parser list IDs */
#define SRCH_PARSER_LIST_INVALID_ID                                                 (0)
#define SRCH_PARSER_LIST_RANGE_ID                                                   (1)
#define SRCH_PARSER_LIST_OPERATOR_ID                                                (2)
#define SRCH_PARSER_LIST_MODIFIER_ID                                                (3)
#define SRCH_PARSER_LIST_SORT_ORDER_ID                                              (4)
#define SRCH_PARSER_LIST_SORT_FIELD_NAME_ID                                         (5)
#define SRCH_PARSER_LIST_FUNCTION_ID                                                (6)


/* Default flag settings */
#define SRCH_PARSER_SEARCH_RESULTS_ID_DEFAULT                                       SRCH_PARSER_MODIFIER_SEARCH_RESULTS_RETURN_ID
#define SRCH_PARSER_SEARCH_REPORT_ID_DEFAULT                                        SRCH_PARSER_MODIFIER_SEARCH_REPORT_RETURN_ID
#define SRCH_PARSER_SEARCH_CACHE_ID_DEFAULT                                         SRCH_PARSER_MODIFIER_SEARCH_CACHE_ENABLE_ID
#define SRCH_PARSER_DEBUG_ID_DEFAULT                                                SRCH_PARSER_MODIFIER_DEBUG_DISABLE_ID
#define SRCH_PARSER_EARLY_COMPLETION_ID_DEFAULT                                     SRCH_PARSER_MODIFIER_EARLY_COMPLETION_ENABLE_ID


/* Default ID settings */
#define SRCH_PARSER_BOOLEAN_OPERATOR_ID_DEFAULT                                     SRCH_PARSER_MODIFIER_BOOLEAN_OPERATOR_AND_ID
#define SRCH_PARSER_BOOLEAN_OPERATION_ID_DEFAULT                                    SRCH_PARSER_MODIFIER_BOOLEAN_OPERATION_STRICT_ID
#define SRCH_PARSER_OPERATOR_CASE_ID_DEFAULT                                        SRCH_PARSER_MODIFIER_OPERATOR_CASE_ANY_ID
#define SRCH_PARSER_TERM_CASE_ID_DEFAULT                                            SRCH_PARSER_MODIFIER_TERM_CASE_KEEP_ID
#define SRCH_PARSER_FREQUENT_TERMS_ID_DEFAULT                                       SRCH_PARSER_MODIFIER_FREQUENT_TERMS_KEEP_ID
#define SRCH_PARSER_SEARCH_TYPE_ID_DEFAULT                                          SRCH_PARSER_MODIFIER_SEARCH_TYPE_BOOLEAN_ID

#define SRCH_PARSER_BOOLEAN_OPERATOR_WSTRING_DEFAULT                                SRCH_PARSER_MODIFIER_BOOLEAN_OPERATOR_AND_WSTRING    
#define SRCH_PARSER_BOOLEAN_OPERATORS_WSTRING_DEFAULT                               SRCH_PARSER_MODIFIER_BOOLEAN_OPERATION_STRICT_WSTRING    
#define SRCH_PARSER_OPERATOR_CASE_WSTRING_DEFAULT                                   SRCH_PARSER_MODIFIER_OPERATOR_CASE_ANY_WSTRING    
#define SRCH_PARSER_TERM_CASE_WSTRING_DEFAULT                                       SRCH_PARSER_MODIFIER_TERM_CASE_KEEP_WSTRING    
#define SRCH_PARSER_FREQUENT_TERMS_KEEP_WSTRING_DEFAULT                             SRCH_PARSER_MODIFIER_FREQUENT_TERMS_KEEP_WSTRING    
#define SRCH_PARSER_SEARCH_TYPE_WSTRING_DEFAULT                                     SRCH_PARSER_MODIFIER_SEARCH_TYPE_BOOLEAN_WSTRING    


/* Tokenizer language ID used to fluff up CJK/Thai and strip token trailings */
#define SRCH_PARSER_TOKENIZER_LANGUAGE_ID_DEFAULT                                   LNG_LANGUAGE_ANY_ID





/* Date name wide strings */
#define    SRCH_PARSER_DATE_NAME_TODAY_WSTRING_EN                                   L"today"
#define    SRCH_PARSER_DATE_NAME_YESTERDAY_WSTRING_EN                               L"yesterday"
#define    SRCH_PARSER_DATE_NAME_THIS_WEEK_WSTRING_EN                               L"thisweek"
#define    SRCH_PARSER_DATE_NAME_LAST_WEEK_WSTRING_EN                               L"lastweek"
#define    SRCH_PARSER_DATE_NAME_THIS_MONTH_WSTRING_EN                              L"thismonth"
#define    SRCH_PARSER_DATE_NAME_LAST_MONTH_WSTRING_EN                              L"lastmonth"
#define    SRCH_PARSER_DATE_NAME_THIS_YEAR_WSTRING_EN                               L"thisyear"
#define    SRCH_PARSER_DATE_NAME_LAST_YEAR_WSTRING_EN                               L"lastyear"

#define    SRCH_PARSER_DATE_NAME_SUNDAY_WSTRING_EN                                  L"sunday"
#define    SRCH_PARSER_DATE_NAME_MONDAY_WSTRING_EN                                  L"monday"
#define    SRCH_PARSER_DATE_NAME_TUESDAY_WSTRING_EN                                 L"tuesday"
#define    SRCH_PARSER_DATE_NAME_WEDNESDAY_WSTRING_EN                               L"wednesday"
#define    SRCH_PARSER_DATE_NAME_THURSDAY_WSTRING_EN                                L"thursday"
#define    SRCH_PARSER_DATE_NAME_FRIDAY_WSTRING_EN                                  L"friday"
#define    SRCH_PARSER_DATE_NAME_SATURDAY_WSTRING_EN                                L"saturday"

#define    SRCH_PARSER_DATE_NAME_JANUARY_WSTRING_EN                                 L"january"
#define    SRCH_PARSER_DATE_NAME_FEBRUARY_WSTRING_EN                                L"february"
#define    SRCH_PARSER_DATE_NAME_MARCH_WSTRING_EN                                   L"march"
#define    SRCH_PARSER_DATE_NAME_APRIL_WSTRING_EN                                   L"april"
#define    SRCH_PARSER_DATE_NAME_MAY_WSTRING_EN                                     L"may"
#define    SRCH_PARSER_DATE_NAME_JUNE_WSTRING_EN                                    L"june"
#define    SRCH_PARSER_DATE_NAME_JULY_WSTRING_EN                                    L"july"
#define    SRCH_PARSER_DATE_NAME_AUGUST_WSTRING_EN                                  L"august"
#define    SRCH_PARSER_DATE_NAME_SEPTEMBER_WSTRING_EN                               L"september"
#define    SRCH_PARSER_DATE_NAME_OCTOBER_WSTRING_EN                                 L"october"
#define    SRCH_PARSER_DATE_NAME_NOVEMBER_WSTRING_EN                                L"november"
#define    SRCH_PARSER_DATE_NAME_DECEMBER_WSTRING_EN                                L"december"


#define    SRCH_PARSER_DATE_NAME_TODAY_WSTRING_FR                                   L"aujourdhui"
#define    SRCH_PARSER_DATE_NAME_YESTERDAY_WSTRING_FR                               L"hier"
#define    SRCH_PARSER_DATE_NAME_THIS_WEEK_WSTRING_FR                               L"cettesemaine"
#define    SRCH_PARSER_DATE_NAME_LAST_WEEK_WSTRING_FR                               L"semainederniere"
#define    SRCH_PARSER_DATE_NAME_THIS_MONTH_WSTRING_FR                              L"cemois"
#define    SRCH_PARSER_DATE_NAME_LAST_MONTH_WSTRING_FR                              L"moisdernier"
#define    SRCH_PARSER_DATE_NAME_THIS_YEAR_WSTRING_FR                               L"cetteannee"
#define    SRCH_PARSER_DATE_NAME_LAST_YEAR_WSTRING_FR                               L"anneederniere"

#define    SRCH_PARSER_DATE_NAME_SUNDAY_WSTRING_FR                                  L"dimanche"
#define    SRCH_PARSER_DATE_NAME_MONDAY_WSTRING_FR                                  L"lundi"
#define    SRCH_PARSER_DATE_NAME_TUESDAY_WSTRING_FR                                 L"mardi"
#define    SRCH_PARSER_DATE_NAME_WEDNESDAY_WSTRING_FR                               L"mercredi"
#define    SRCH_PARSER_DATE_NAME_THURSDAY_WSTRING_FR                                L"jeudi"
#define    SRCH_PARSER_DATE_NAME_FRIDAY_WSTRING_FR                                  L"vendredi"
#define    SRCH_PARSER_DATE_NAME_SATURDAY_WSTRING_FR                                L"samedi"

#define    SRCH_PARSER_DATE_NAME_JANUARY_WSTRING_FR                                 L"janvier"
#define    SRCH_PARSER_DATE_NAME_FEBRUARY_WSTRING_FR                                L"fevrier"
#define    SRCH_PARSER_DATE_NAME_MARCH_WSTRING_FR                                   L"mars"
#define    SRCH_PARSER_DATE_NAME_APRIL_WSTRING_FR                                   L"avril"
#define    SRCH_PARSER_DATE_NAME_MAY_WSTRING_FR                                     L"may"
#define    SRCH_PARSER_DATE_NAME_JUNE_WSTRING_FR                                    L"juin"
#define    SRCH_PARSER_DATE_NAME_JULY_WSTRING_FR                                    L"juillet"
#define    SRCH_PARSER_DATE_NAME_AUGUST_WSTRING_FR                                  L"aout"
#define    SRCH_PARSER_DATE_NAME_SEPTEMBER_WSTRING_FR                               L"septembere"
#define    SRCH_PARSER_DATE_NAME_OCTOBER_WSTRING_FR                                 L"octobre"
#define    SRCH_PARSER_DATE_NAME_NOVEMBER_WSTRING_FR                                L"novembre"
#define    SRCH_PARSER_DATE_NAME_DECEMBER_WSTRING_FR                                L"decembre"



#define    SRCH_PARSER_DATE_NAME_INVALID_ID                                         (0)
#define    SRCH_PARSER_DATE_NAME_TODAY_ID                                           (1)
#define    SRCH_PARSER_DATE_NAME_YESTERDAY_ID                                       (2)
#define    SRCH_PARSER_DATE_NAME_THIS_WEEK_ID                                       (3)
#define    SRCH_PARSER_DATE_NAME_LAST_WEEK_ID                                       (4)
#define    SRCH_PARSER_DATE_NAME_THIS_MONTH_ID                                      (5)
#define    SRCH_PARSER_DATE_NAME_LAST_MONTH_ID                                      (6)
#define    SRCH_PARSER_DATE_NAME_THIS_YEAR_ID                                       (7)
#define    SRCH_PARSER_DATE_NAME_LAST_YEAR_ID                                       (8)

#define    SRCH_PARSER_DATE_NAME_SUNDAY_ID                                          (100)
#define    SRCH_PARSER_DATE_NAME_MONDAY_ID                                          (101)
#define    SRCH_PARSER_DATE_NAME_TUESDAY_ID                                         (102)
#define    SRCH_PARSER_DATE_NAME_WEDNESDAY_ID                                       (103)
#define    SRCH_PARSER_DATE_NAME_THURSDAY_ID                                        (104)
#define    SRCH_PARSER_DATE_NAME_FRIDAY_ID                                          (105)
#define    SRCH_PARSER_DATE_NAME_SATURDAY_ID                                        (106)

#define    SRCH_PARSER_DATE_NAME_JANUARY_ID                                         (200)
#define    SRCH_PARSER_DATE_NAME_FEBRUARY_ID                                        (201)
#define    SRCH_PARSER_DATE_NAME_MARCH_ID                                           (202)
#define    SRCH_PARSER_DATE_NAME_APRIL_ID                                           (203)
#define    SRCH_PARSER_DATE_NAME_MAY_ID                                             (204)
#define    SRCH_PARSER_DATE_NAME_JUNE_ID                                            (205)
#define    SRCH_PARSER_DATE_NAME_JULY_ID                                            (206)
#define    SRCH_PARSER_DATE_NAME_AUGUST_ID                                          (207)
#define    SRCH_PARSER_DATE_NAME_SEPTEMBER_ID                                       (208)
#define    SRCH_PARSER_DATE_NAME_OCTOBER_ID                                         (209)
#define    SRCH_PARSER_DATE_NAME_NOVEMBER_ID                                        (210)
#define    SRCH_PARSER_DATE_NAME_DECEMBER_ID                                        (211)


/* Error mapping global structure end marker, needs to be postive */
#define SRCH_PARSER_MAPPING_END_MARKER                                              (1)


/*---------------------------------------------------------------------------*/


/*
** Structures
*/

/* Search parser structure */
struct srchParser {

    unsigned int                    uiLanguageID;                                   /* Language ID */
    unsigned int                    uiTokenizerID;                                  /* Tokenizer ID */

    unsigned int                    uiSearchResultsID;                              /* Search results ID */
    unsigned int                    uiSearchReportID;                               /* Search report ID */
    unsigned int                    uiSearchCacheID;                                /* Search cache ID */
    unsigned int                    uiDebugID;                                      /* Debug ID */
    unsigned int                    uiEarlyCompletionID;                            /* Early completion ID */
    unsigned int                    uiBooleanOperatorID;                            /* Boolean operator ID */
    unsigned int                    uiBooleanOperationID;                           /* Boolean operation ID */
    unsigned int                    uiOperatorCaseID;                               /* Operator case ID */
    unsigned int                    uiTermCaseID;                                   /* Term case ID */
    unsigned int                    uiFrequentTermsID;                              /* Frequent search terms ID */
    unsigned int                    uiSearchTypeID;                                 /* Search type ID */

    struct srchParserNumber         *pspnSrchParserNumberDates;                     /* Parser number dates restrictions from {date#...} */
    unsigned int                    uiSrchParserNumberDatesLength;                  /* Parser number dates restrictions length */

    wchar_t                         *pwcUnfieldedSearchFieldNames;                  /* Unfielded search field names */

    wchar_t                         *pwcSortFieldName;                              /* Sort field name */
    unsigned int                    uiSortOrderID;                                  /* Sort order ID */

    struct srchParserFilter         *pspfSrchParserFilterExclusionFilters;          /* Parser filter exclusion filters from {exclusion_filter:...}/{exclusion_list_filter:...} */
    unsigned int                    uiSrchParserFilterExclusionFiltersLength;       /* Parser filter exclusion filters length */

    struct srchParserFilter         *pspfSrchParserFilterInclusionFilters;          /* Parser filter inclusion filters from {inclusion_filter:...}/{inclusion_list_filter:...} */
    unsigned int                    uiSrchParserFilterInclusionFiltersLength;       /* Parser filter inclusion filters length */

    struct srchParserNumber         *pspnSrchParserNumberLanguageIDs;               /* Parser number language IDs restrictions from {lang:...} */
    unsigned int                    uiSrchParserNumberLanguageIDsLength;            /* Parser number language IDs restrictions length */

    float                           fTermWeight;                                    /* Term weight */
    float                           fFeedbackTermWeight;                            /* Feedback term weight */

    float                           fFrequentTermCoverageThreshold;                 /* Frequent term coverage threshold */

    unsigned int                    uiFeedbackMinimumTermCount;                     /* Feedback minimum term count */
    float                           fFeedbackMaximumTermPercentage;                 /* Feedback maximum term percentage */
    float                           fFeedbackMaximumTermCoverageThreshold;          /* Feedback maximum term coverage threshold */

    unsigned int                    uiConnectionTimeout;                            /* Connection timeout */
    unsigned int                    uiSearchTimeout;                                /* Search timeout */
    unsigned int                    uiRetrievalTimeout;                             /* Retrieval timeout */
    unsigned int                    uiInformationTimeout;                           /* Information timeout */

    unsigned int                    uiMaximumSegmentsSearched;                      /* Maximum segments searched (gateway) */
    unsigned int                    uiMinimumSegmentsSearched;                      /* Minimum segments searched (gateway) */

    unsigned int                    uiSearchTermCount;                              /* Search term count */

    void                            *pvLngTokenizer;                                /* Tokenizer */

    struct srchParserTermCluster    *psptcSrchParserTermCluster;                    /* Parser term cluster structure */

    wchar_t                         *pwcNormalizedSearchText;                       /* Normalized search text */
    wchar_t                         *pwcFullNormalizedSearchText;                   /* Full normalized search text */
    
    struct srchSearch               *pssSrchSearch;                                 /* Reference to the search structure */
};


/* Parser token structure */
struct srchParserToken {
    wchar_t                         *pwcTokenString;                                /* Token string */
    unsigned int                    uiTokenID;                                      /* Token ID */
    wchar_t                         *pwcTokenTrailerString;                         /* Token trailer string, usually characters */                        
    unsigned int                    uiTokenTrailerStatus;                           /* Token trailer status */
};


/* Date name structure */
struct srchParserDateName {
    wchar_t                         *pwcDateName;                                   /* Date name */
    unsigned int                    uiDateNameID;                                   /* Date name ID */
};


/* Replace structure */
struct srchParserReplace {
    wchar_t                         *pwcFromString;                                 /* From string */
    wchar_t                         *pwcToString;                                   /* To string */
};



/* Error mapping structure */
struct srchParserErrorMapping {
    int                             iError;
    unsigned char                   *pucErrorText;
};


/*---------------------------------------------------------------------------*/


/*
** Globals
*/

/* Permissible modifier tokens, no precendence */
static struct srchParserToken pptParserTokenModifierListGlobal[] = 
{
    {   SRCH_PARSER_MODIFIER_SEARCH_RESULTS_RETURN_WSTRING,
        SRCH_PARSER_MODIFIER_SEARCH_RESULTS_RETURN_ID,
        NULL,
        SRCH_PARSER_TRAILER_NONE,
    },
    {   SRCH_PARSER_MODIFIER_SEARCH_RESULTS_SUPPRESS_WSTRING,
        SRCH_PARSER_MODIFIER_SEARCH_RESULTS_SUPPRESS_ID,
        NULL,
        SRCH_PARSER_TRAILER_NONE,
    },
    {   SRCH_PARSER_MODIFIER_SEARCH_RESULTS_RETURN_ABR_WSTRING,
        SRCH_PARSER_MODIFIER_SEARCH_RESULTS_RETURN_ID,
        NULL,
        SRCH_PARSER_TRAILER_NONE,
    },
    {   SRCH_PARSER_MODIFIER_SEARCH_RESULTS_SUPPRESS_ABR_WSTRING,
        SRCH_PARSER_MODIFIER_SEARCH_RESULTS_SUPPRESS_ID,
        NULL,
        SRCH_PARSER_TRAILER_NONE,
    },

    {   SRCH_PARSER_MODIFIER_SEARCH_REPORT_RETURN_WSTRING,
        SRCH_PARSER_MODIFIER_SEARCH_REPORT_RETURN_ID,
        NULL,
        SRCH_PARSER_TRAILER_NONE,
    }, 
    {   SRCH_PARSER_MODIFIER_SEARCH_REPORT_SUPPRESS_WSTRING,
        SRCH_PARSER_MODIFIER_SEARCH_REPORT_SUPPRESS_ID,
        NULL,
        SRCH_PARSER_TRAILER_NONE,
    }, 
    {   SRCH_PARSER_MODIFIER_SEARCH_REPORT_RETURN_ABR_WSTRING,
        SRCH_PARSER_MODIFIER_SEARCH_REPORT_RETURN_ID,
        NULL,
        SRCH_PARSER_TRAILER_NONE,
    }, 
    {   SRCH_PARSER_MODIFIER_SEARCH_REPORT_SUPPRESS_ABR_WSTRING,
        SRCH_PARSER_MODIFIER_SEARCH_REPORT_SUPPRESS_ID,
        NULL,
        SRCH_PARSER_TRAILER_NONE,
    }, 

    {   SRCH_PARSER_MODIFIER_DEBUG_ENABLE_WSTRING,
        SRCH_PARSER_MODIFIER_DEBUG_ENABLE_ID,
        NULL,
        SRCH_PARSER_TRAILER_NONE,
    },
    {   SRCH_PARSER_MODIFIER_DEBUG_DISABLE_WSTRING,
        SRCH_PARSER_MODIFIER_DEBUG_DISABLE_ID,
        NULL,
        SRCH_PARSER_TRAILER_NONE,
    },
    {   SRCH_PARSER_MODIFIER_DEBUG_ENABLE_ABR_WSTRING,
        SRCH_PARSER_MODIFIER_DEBUG_ENABLE_ID,
        NULL,
        SRCH_PARSER_TRAILER_NONE,
    },
    {   SRCH_PARSER_MODIFIER_DEBUG_DISABLE_ABR_WSTRING,
        SRCH_PARSER_MODIFIER_DEBUG_DISABLE_ID,
        NULL,
        SRCH_PARSER_TRAILER_NONE,
    },

    {   SRCH_PARSER_MODIFIER_SEARCH_CACHE_ENABLE_WSTRING,
        SRCH_PARSER_MODIFIER_SEARCH_CACHE_ENABLE_ID,
        NULL,
        SRCH_PARSER_TRAILER_NONE,
    },
    {   SRCH_PARSER_MODIFIER_SEARCH_CACHE_DISABLE_WSTRING,
        SRCH_PARSER_MODIFIER_SEARCH_CACHE_DISABLE_ID,
        NULL,
        SRCH_PARSER_TRAILER_NONE,
    },
    {   SRCH_PARSER_MODIFIER_SEARCH_CACHE_ENABLE_ABR_WSTRING,
        SRCH_PARSER_MODIFIER_SEARCH_CACHE_ENABLE_ID,
        NULL,
        SRCH_PARSER_TRAILER_NONE,
    },
    {   SRCH_PARSER_MODIFIER_SEARCH_CACHE_DISABLE_ABR_WSTRING,
        SRCH_PARSER_MODIFIER_SEARCH_CACHE_DISABLE_ID,
        NULL,
        SRCH_PARSER_TRAILER_NONE,
    },

    {   SRCH_PARSER_MODIFIER_BOOLEAN_OPERATOR_OR_WSTRING,
        SRCH_PARSER_MODIFIER_BOOLEAN_OPERATOR_OR_ID,
        NULL,
        SRCH_PARSER_TRAILER_NONE,
    },
    {   SRCH_PARSER_MODIFIER_BOOLEAN_OPERATOR_IOR_WSTRING,
        SRCH_PARSER_MODIFIER_BOOLEAN_OPERATOR_IOR_ID,
        NULL,
        SRCH_PARSER_TRAILER_NONE,
    },
    {   SRCH_PARSER_MODIFIER_BOOLEAN_OPERATOR_XOR_WSTRING,
        SRCH_PARSER_MODIFIER_BOOLEAN_OPERATOR_XOR_ID,
        NULL,
        SRCH_PARSER_TRAILER_NONE,
    },
    {   SRCH_PARSER_MODIFIER_BOOLEAN_OPERATOR_AND_WSTRING,
        SRCH_PARSER_MODIFIER_BOOLEAN_OPERATOR_AND_ID,
        NULL,
        SRCH_PARSER_TRAILER_NONE,
    },
    {   SRCH_PARSER_MODIFIER_BOOLEAN_OPERATOR_ADJ_WSTRING,
        SRCH_PARSER_MODIFIER_BOOLEAN_OPERATOR_ADJ_ID,
        NULL,
        SRCH_PARSER_TRAILER_NONE,
    },
    {   SRCH_PARSER_MODIFIER_BOOLEAN_OPERATOR_NEAR_WSTRING,
        SRCH_PARSER_MODIFIER_BOOLEAN_OPERATOR_NEAR_ID,
        NULL,
        SRCH_PARSER_TRAILER_NONE,
    },
    {   SRCH_PARSER_MODIFIER_BOOLEAN_OPERATOR_OR_ABR_WSTRING,
        SRCH_PARSER_MODIFIER_BOOLEAN_OPERATOR_OR_ID,
        NULL,
        SRCH_PARSER_TRAILER_NONE,
    },
    {   SRCH_PARSER_MODIFIER_BOOLEAN_OPERATOR_IOR_ABR_WSTRING,
        SRCH_PARSER_MODIFIER_BOOLEAN_OPERATOR_IOR_ID,
        NULL,
        SRCH_PARSER_TRAILER_NONE,
    },
    {   SRCH_PARSER_MODIFIER_BOOLEAN_OPERATOR_XOR_ABR_WSTRING,
        SRCH_PARSER_MODIFIER_BOOLEAN_OPERATOR_XOR_ID,
        NULL,
        SRCH_PARSER_TRAILER_NONE,
    },
    {   SRCH_PARSER_MODIFIER_BOOLEAN_OPERATOR_AND_ABR_WSTRING,
        SRCH_PARSER_MODIFIER_BOOLEAN_OPERATOR_AND_ID,
        NULL,
        SRCH_PARSER_TRAILER_NONE,
    },
    {   SRCH_PARSER_MODIFIER_BOOLEAN_OPERATOR_ADJ_ABR_WSTRING,
        SRCH_PARSER_MODIFIER_BOOLEAN_OPERATOR_ADJ_ID,
        NULL,
        SRCH_PARSER_TRAILER_NONE,
    },
    {   SRCH_PARSER_MODIFIER_BOOLEAN_OPERATOR_NEAR_ABR_WSTRING,
        SRCH_PARSER_MODIFIER_BOOLEAN_OPERATOR_NEAR_ID,
        NULL,
        SRCH_PARSER_TRAILER_NONE,
    },

    {   SRCH_PARSER_MODIFIER_BOOLEAN_OPERATION_RELAXED_WSTRING,
        SRCH_PARSER_MODIFIER_BOOLEAN_OPERATION_RELAXED_ID,
        NULL,
        SRCH_PARSER_TRAILER_NONE,
    },
    {   SRCH_PARSER_MODIFIER_BOOLEAN_OPERATION_STRICT_WSTRING,
        SRCH_PARSER_MODIFIER_BOOLEAN_OPERATION_STRICT_ID,
        NULL,
        SRCH_PARSER_TRAILER_NONE,
    },
    {   SRCH_PARSER_MODIFIER_BOOLEAN_OPERATION_RELAXED_ABR_WSTRING,
        SRCH_PARSER_MODIFIER_BOOLEAN_OPERATION_RELAXED_ID,
        NULL,
        SRCH_PARSER_TRAILER_NONE,
    },
    {   SRCH_PARSER_MODIFIER_BOOLEAN_OPERATION_STRICT_ABR_WSTRING,
        SRCH_PARSER_MODIFIER_BOOLEAN_OPERATION_STRICT_ID,
        NULL,
        SRCH_PARSER_TRAILER_NONE,
    },

    {   SRCH_PARSER_MODIFIER_OPERATOR_CASE_ANY_WSTRING,
        SRCH_PARSER_MODIFIER_OPERATOR_CASE_ANY_ID,
        NULL,
        SRCH_PARSER_TRAILER_NONE,
    },
    {   SRCH_PARSER_MODIFIER_OPERATOR_CASE_UPPER_WSTRING,
        SRCH_PARSER_MODIFIER_OPERATOR_CASE_UPPER_ID,
        NULL,
        SRCH_PARSER_TRAILER_NONE,
    },
    {   SRCH_PARSER_MODIFIER_OPERATOR_CASE_LOWER_WSTRING,
        SRCH_PARSER_MODIFIER_OPERATOR_CASE_LOWER_ID,
        NULL,
        SRCH_PARSER_TRAILER_NONE,
    },
    {   SRCH_PARSER_MODIFIER_OPERATOR_CASE_ANY_ABR_WSTRING,
        SRCH_PARSER_MODIFIER_OPERATOR_CASE_ANY_ID,
        NULL,
        SRCH_PARSER_TRAILER_NONE,
    },
    {   SRCH_PARSER_MODIFIER_OPERATOR_CASE_UPPER_ABR_WSTRING,
        SRCH_PARSER_MODIFIER_OPERATOR_CASE_UPPER_ID,
        NULL,
        SRCH_PARSER_TRAILER_NONE,
    },
    {   SRCH_PARSER_MODIFIER_OPERATOR_CASE_LOWER_ABR_WSTRING,
        SRCH_PARSER_MODIFIER_OPERATOR_CASE_LOWER_ID,
        NULL,
        SRCH_PARSER_TRAILER_NONE,
    },

    {   SRCH_PARSER_MODIFIER_TERM_CASE_DROP_WSTRING,
        SRCH_PARSER_MODIFIER_TERM_CASE_DROP_ID,
        NULL,
        SRCH_PARSER_TRAILER_NONE,
    },
    {   SRCH_PARSER_MODIFIER_TERM_CASE_KEEP_WSTRING,
        SRCH_PARSER_MODIFIER_TERM_CASE_KEEP_ID,
        NULL,
        SRCH_PARSER_TRAILER_NONE,
    },
    {   SRCH_PARSER_MODIFIER_TERM_CASE_DROP_ABR_WSTRING,
        SRCH_PARSER_MODIFIER_TERM_CASE_DROP_ID,
        NULL,
        SRCH_PARSER_TRAILER_NONE,
    },
    {   SRCH_PARSER_MODIFIER_TERM_CASE_KEEP_ABR_WSTRING,
        SRCH_PARSER_MODIFIER_TERM_CASE_KEEP_ID,
        NULL,
        SRCH_PARSER_TRAILER_NONE,
    },

    {   SRCH_PARSER_MODIFIER_FREQUENT_TERMS_DROP_WSTRING,
        SRCH_PARSER_MODIFIER_FREQUENT_TERMS_DROP_ID,
        NULL,
        SRCH_PARSER_TRAILER_NONE,
    },
    {   SRCH_PARSER_MODIFIER_FREQUENT_TERMS_KEEP_WSTRING,
        SRCH_PARSER_MODIFIER_FREQUENT_TERMS_KEEP_ID,
        NULL,
        SRCH_PARSER_TRAILER_NONE,
    },
    {   SRCH_PARSER_MODIFIER_FREQUENT_TERMS_DROP_ABR_WSTRING,
        SRCH_PARSER_MODIFIER_FREQUENT_TERMS_DROP_ID,
        NULL,
        SRCH_PARSER_TRAILER_NONE,
    },
    {   SRCH_PARSER_MODIFIER_FREQUENT_TERMS_KEEP_ABR_WSTRING,
        SRCH_PARSER_MODIFIER_FREQUENT_TERMS_KEEP_ID,
        NULL,
        SRCH_PARSER_TRAILER_NONE,
    },

    {   SRCH_PARSER_MODIFIER_SEARCH_TYPE_BOOLEAN_WSTRING,
        SRCH_PARSER_MODIFIER_SEARCH_TYPE_BOOLEAN_ID,
        NULL,
        SRCH_PARSER_TRAILER_NONE,
    },
    {   SRCH_PARSER_MODIFIER_SEARCH_TYPE_FREETEXT_WSTRING,
        SRCH_PARSER_MODIFIER_SEARCH_TYPE_FREETEXT_ID,
        NULL,
        SRCH_PARSER_TRAILER_NONE,
    },
    {   SRCH_PARSER_MODIFIER_SEARCH_TYPE_BOOLEAN_ABR_WSTRING,
        SRCH_PARSER_MODIFIER_SEARCH_TYPE_BOOLEAN_ID,
        NULL,
        SRCH_PARSER_TRAILER_NONE,
    },
    {   SRCH_PARSER_MODIFIER_SEARCH_TYPE_FREETEXT_ABR_WSTRING,
        SRCH_PARSER_MODIFIER_SEARCH_TYPE_FREETEXT_ID,
        NULL,
        SRCH_PARSER_TRAILER_NONE,
    },

    {   SRCH_PARSER_MODIFIER_DATE_WSTRING,
        SRCH_PARSER_MODIFIER_DATE_ID,
        SRCH_PARSER_RANGE_FULL_LIST_WSTRING,
        SRCH_PARSER_TRAILER_REQUIRED,
    },
    {   SRCH_PARSER_MODIFIER_DATE_ABR_WSTRING,
        SRCH_PARSER_MODIFIER_DATE_ID,
        SRCH_PARSER_RANGE_FULL_LIST_WSTRING,
        SRCH_PARSER_TRAILER_REQUIRED,
    },

    {   SRCH_PARSER_MODIFIER_UNFIELDED_SEARCH_FIELD_NAMES_WSTRING,
        SRCH_PARSER_MODIFIER_UNFIELDED_SEARCH_FIELD_NAMES_ID,
        SRCH_PARSER_MODIFIER_SEPARATOR_WSTRING,
        SRCH_PARSER_TRAILER_REQUIRED,
    },
    {   SRCH_PARSER_MODIFIER_UNFIELDED_SEARCH_FIELD_NAMES_ABR_WSTRING,
        SRCH_PARSER_MODIFIER_UNFIELDED_SEARCH_FIELD_NAMES_ID,
        SRCH_PARSER_MODIFIER_SEPARATOR_WSTRING,
        SRCH_PARSER_TRAILER_REQUIRED,
    },

    {   SRCH_PARSER_MODIFIER_SORT_DEFAULT_WSTRING,
        SRCH_PARSER_MODIFIER_SORT_DEFAULT_ID,
        NULL,
        SRCH_PARSER_TRAILER_NONE,
    },
    {   SRCH_PARSER_MODIFIER_SORT_NONE_WSTRING,
        SRCH_PARSER_MODIFIER_SORT_NONE_ID,
        NULL,
        SRCH_PARSER_TRAILER_NONE,
    },
    {   SRCH_PARSER_MODIFIER_SORT_WSTRING,
        SRCH_PARSER_MODIFIER_SORT_ID,
        SRCH_PARSER_MODIFIER_SEPARATOR_WSTRING,
        SRCH_PARSER_TRAILER_REQUIRED,
    },
    {   SRCH_PARSER_MODIFIER_SORT_DEFAULT_ABR_WSTRING,
        SRCH_PARSER_MODIFIER_SORT_DEFAULT_ID,
        NULL,
        SRCH_PARSER_TRAILER_NONE,
    },
    {   SRCH_PARSER_MODIFIER_SORT_NONE_ABR_WSTRING,
        SRCH_PARSER_MODIFIER_SORT_NONE_ID,
        NULL,
        SRCH_PARSER_TRAILER_NONE,
    },
    {   SRCH_PARSER_MODIFIER_SORT_ABR_WSTRING,
        SRCH_PARSER_MODIFIER_SORT_ID,
        SRCH_PARSER_MODIFIER_SEPARATOR_WSTRING,
        SRCH_PARSER_TRAILER_REQUIRED,
    },

    {   SRCH_PARSER_MODIFIER_EARLY_COMPLETION_ENABLE_WSTRING,
        SRCH_PARSER_MODIFIER_EARLY_COMPLETION_ENABLE_ID,
        NULL,
        SRCH_PARSER_TRAILER_NONE,
    },
    {   SRCH_PARSER_MODIFIER_EARLY_COMPLETION_DISABLE_WSTRING,
        SRCH_PARSER_MODIFIER_EARLY_COMPLETION_DISABLE_ID,
        NULL,
        SRCH_PARSER_TRAILER_NONE,
    },
    {   SRCH_PARSER_MODIFIER_EARLY_COMPLETION_ENABLE_ABR_WSTRING,
        SRCH_PARSER_MODIFIER_EARLY_COMPLETION_ENABLE_ID,
        NULL,
        SRCH_PARSER_TRAILER_NONE,
    },
    {   SRCH_PARSER_MODIFIER_EARLY_COMPLETION_DISABLE_ABR_WSTRING,
        SRCH_PARSER_MODIFIER_EARLY_COMPLETION_DISABLE_ID,
        NULL,
        SRCH_PARSER_TRAILER_NONE,
    },

    {   SRCH_PARSER_MODIFIER_TERM_WEIGHT_WSTRING,
        SRCH_PARSER_MODIFIER_TERM_WEIGHT_ID,
        SRCH_PARSER_MODIFIER_SEPARATOR_WSTRING,
        SRCH_PARSER_TRAILER_REQUIRED,
    },
    {   SRCH_PARSER_MODIFIER_FEEDBACK_TERM_WEIGHT_WSTRING,
        SRCH_PARSER_MODIFIER_FEEDBACK_TERM_WEIGHT_ID,
        SRCH_PARSER_MODIFIER_SEPARATOR_WSTRING,
        SRCH_PARSER_TRAILER_REQUIRED,
    },
    {   SRCH_PARSER_MODIFIER_TERM_WEIGHT_ABR_WSTRING,
        SRCH_PARSER_MODIFIER_TERM_WEIGHT_ID,
        SRCH_PARSER_MODIFIER_SEPARATOR_WSTRING,
        SRCH_PARSER_TRAILER_REQUIRED,
    },
    {   SRCH_PARSER_MODIFIER_FEEDBACK_TERM_WEIGHT_ABR_WSTRING,
        SRCH_PARSER_MODIFIER_FEEDBACK_TERM_WEIGHT_ID,
        SRCH_PARSER_MODIFIER_SEPARATOR_WSTRING,
        SRCH_PARSER_TRAILER_REQUIRED,
    },

    {   SRCH_PARSER_MODIFIER_FREQUENT_TERM_COVERAGE_THRESHOLD_WSTRING,
        SRCH_PARSER_MODIFIER_FREQUENT_TERM_COVERAGE_THRESHOLD_ID,
        SRCH_PARSER_MODIFIER_SEPARATOR_WSTRING,
        SRCH_PARSER_TRAILER_REQUIRED,
    },
    {   SRCH_PARSER_MODIFIER_FREQUENT_TERM_COVERAGE_THRESHOLD_ABR_WSTRING,
        SRCH_PARSER_MODIFIER_FREQUENT_TERM_COVERAGE_THRESHOLD_ID,
        SRCH_PARSER_MODIFIER_SEPARATOR_WSTRING,
        SRCH_PARSER_TRAILER_REQUIRED,
    },

    {   SRCH_PARSER_MODIFIER_FEEDBACK_MINIMUM_TERM_COUNT_WSTRING,
        SRCH_PARSER_MODIFIER_FEEDBACK_MINIMUM_TERM_COUNT_ID,
        SRCH_PARSER_MODIFIER_SEPARATOR_WSTRING,
        SRCH_PARSER_TRAILER_REQUIRED,
    },
    {   SRCH_PARSER_MODIFIER_FEEDBACK_MAXIMUM_TERM_PERCENTAGE_WSTRING,
        SRCH_PARSER_MODIFIER_FEEDBACK_MAXIMUM_TERM_PERCENTAGE_ID,
        SRCH_PARSER_MODIFIER_SEPARATOR_WSTRING,
        SRCH_PARSER_TRAILER_REQUIRED,
    },
    {   SRCH_PARSER_MODIFIER_FEEDBACK_MAXIMUM_TERM_COVERAGE_THRESHOLD_WSTRING,
        SRCH_PARSER_MODIFIER_FEEDBACK_MAXIMUM_TERM_COVERAGE_THRESHOLD_ID,
        SRCH_PARSER_MODIFIER_SEPARATOR_WSTRING,
        SRCH_PARSER_TRAILER_REQUIRED,
    },
    {   SRCH_PARSER_MODIFIER_FEEDBACK_MINIMUM_TERM_COUNT_ABR_WSTRING,
        SRCH_PARSER_MODIFIER_FEEDBACK_MINIMUM_TERM_COUNT_ID,
        SRCH_PARSER_MODIFIER_SEPARATOR_WSTRING,
        SRCH_PARSER_TRAILER_REQUIRED,
    },
    {   SRCH_PARSER_MODIFIER_FEEDBACK_MAXIMUM_TERM_PERCENTAGE_ABR_WSTRING,
        SRCH_PARSER_MODIFIER_FEEDBACK_MAXIMUM_TERM_PERCENTAGE_ID,
        SRCH_PARSER_MODIFIER_SEPARATOR_WSTRING,
        SRCH_PARSER_TRAILER_REQUIRED,
    },
    {   SRCH_PARSER_MODIFIER_FEEDBACK_MAXIMUM_TERM_COVERAGE_THRESHOLD_ABR_WSTRING,
        SRCH_PARSER_MODIFIER_FEEDBACK_MAXIMUM_TERM_COVERAGE_THRESHOLD_ID,
        SRCH_PARSER_MODIFIER_SEPARATOR_WSTRING,
        SRCH_PARSER_TRAILER_REQUIRED,
    },

    {   SRCH_PARSER_MODIFIER_CONNECTION_TIMEOUT_WSTRING,
        SRCH_PARSER_MODIFIER_CONNECTION_TIMEOUT_ID,
        SRCH_PARSER_MODIFIER_SEPARATOR_WSTRING,
        SRCH_PARSER_TRAILER_REQUIRED,
    },
    {   SRCH_PARSER_MODIFIER_SEARCH_TIMEOUT_WSTRING,
        SRCH_PARSER_MODIFIER_SEARCH_TIMEOUT_ID,
        SRCH_PARSER_MODIFIER_SEPARATOR_WSTRING,
        SRCH_PARSER_TRAILER_REQUIRED,
    },
    {   SRCH_PARSER_MODIFIER_RETRIEVAL_TIMEOUT_WSTRING,
        SRCH_PARSER_MODIFIER_RETRIEVAL_TIMEOUT_ID,
        SRCH_PARSER_MODIFIER_SEPARATOR_WSTRING,
        SRCH_PARSER_TRAILER_REQUIRED,
    },
    {   SRCH_PARSER_MODIFIER_INFORMATION_TIMEOUT_WSTRING,
        SRCH_PARSER_MODIFIER_INFORMATION_TIMEOUT_ID,
        SRCH_PARSER_MODIFIER_SEPARATOR_WSTRING,
        SRCH_PARSER_TRAILER_REQUIRED,
    },
    {   SRCH_PARSER_MODIFIER_CONNECTION_TIMEOUT_ABR_WSTRING,
        SRCH_PARSER_MODIFIER_CONNECTION_TIMEOUT_ID,
        SRCH_PARSER_MODIFIER_SEPARATOR_WSTRING,
        SRCH_PARSER_TRAILER_REQUIRED,
    },
    {   SRCH_PARSER_MODIFIER_SEARCH_TIMEOUT_ABR_WSTRING,
        SRCH_PARSER_MODIFIER_SEARCH_TIMEOUT_ID,
        SRCH_PARSER_MODIFIER_SEPARATOR_WSTRING,
        SRCH_PARSER_TRAILER_REQUIRED,
    },
    {   SRCH_PARSER_MODIFIER_RETRIEVAL_TIMEOUT_ABR_WSTRING,
        SRCH_PARSER_MODIFIER_RETRIEVAL_TIMEOUT_ID,
        SRCH_PARSER_MODIFIER_SEPARATOR_WSTRING,
        SRCH_PARSER_TRAILER_REQUIRED,
    },
    {   SRCH_PARSER_MODIFIER_INFORMATION_TIMEOUT_ABR_WSTRING,
        SRCH_PARSER_MODIFIER_INFORMATION_TIMEOUT_ID,
        SRCH_PARSER_MODIFIER_SEPARATOR_WSTRING,
        SRCH_PARSER_TRAILER_REQUIRED,
    },

    {   SRCH_PARSER_MODIFIER_MAXIMUM_SEGMENTS_SEARCHED_WSTRING,
        SRCH_PARSER_MODIFIER_MAXIMUM_SEGMENTS_SEARCHED_ID,
        SRCH_PARSER_MODIFIER_SEPARATOR_WSTRING,
        SRCH_PARSER_TRAILER_REQUIRED,
    },
    {   SRCH_PARSER_MODIFIER_MINIMUM_SEGMENTS_SEARCHED_WSTRING,
        SRCH_PARSER_MODIFIER_MINIMUM_SEGMENTS_SEARCHED_ID,
        SRCH_PARSER_MODIFIER_SEPARATOR_WSTRING,
        SRCH_PARSER_TRAILER_REQUIRED,
    },
    {   SRCH_PARSER_MODIFIER_MAXIMUM_SEGMENTS_SEARCHED_ABR_WSTRING,
        SRCH_PARSER_MODIFIER_MAXIMUM_SEGMENTS_SEARCHED_ID,
        SRCH_PARSER_MODIFIER_SEPARATOR_WSTRING,
        SRCH_PARSER_TRAILER_REQUIRED,
    },
    {   SRCH_PARSER_MODIFIER_MINIMUM_SEGMENTS_SEARCHED_ABR_WSTRING,
        SRCH_PARSER_MODIFIER_MINIMUM_SEGMENTS_SEARCHED_ID,
        SRCH_PARSER_MODIFIER_SEPARATOR_WSTRING,
        SRCH_PARSER_TRAILER_REQUIRED,
    },

    {   SRCH_PARSER_MODIFIER_EXCLUSION_FILTER_WSTRING,
        SRCH_PARSER_MODIFIER_EXCLUSION_FILTER_ID,
        SRCH_PARSER_MODIFIER_SEPARATOR_WSTRING,
        SRCH_PARSER_TRAILER_REQUIRED,
    },
    {   SRCH_PARSER_MODIFIER_INCLUSION_FILTER_WSTRING,
        SRCH_PARSER_MODIFIER_INCLUSION_FILTER_ID,
        SRCH_PARSER_MODIFIER_SEPARATOR_WSTRING,
        SRCH_PARSER_TRAILER_REQUIRED,
    },
    {   SRCH_PARSER_MODIFIER_EXCLUSION_FILTER_ABR_WSTRING,
        SRCH_PARSER_MODIFIER_EXCLUSION_FILTER_ID,
        SRCH_PARSER_MODIFIER_SEPARATOR_WSTRING,
        SRCH_PARSER_TRAILER_REQUIRED,
    },
    {   SRCH_PARSER_MODIFIER_INCLUSION_FILTER_ABR_WSTRING,
        SRCH_PARSER_MODIFIER_INCLUSION_FILTER_ID,
        SRCH_PARSER_MODIFIER_SEPARATOR_WSTRING,
        SRCH_PARSER_TRAILER_REQUIRED,
    },

    {   SRCH_PARSER_MODIFIER_EXCLUSION_LIST_FILTER_WSTRING,
        SRCH_PARSER_MODIFIER_EXCLUSION_LIST_FILTER_ID,
        SRCH_PARSER_MODIFIER_SEPARATOR_WSTRING,
        SRCH_PARSER_TRAILER_REQUIRED,
    },
    {   SRCH_PARSER_MODIFIER_INCLUSION_LIST_FILTER_WSTRING,
        SRCH_PARSER_MODIFIER_INCLUSION_LIST_FILTER_ID,
        SRCH_PARSER_MODIFIER_SEPARATOR_WSTRING,
        SRCH_PARSER_TRAILER_REQUIRED,
    },
    {   SRCH_PARSER_MODIFIER_EXCLUSION_LIST_FILTER_ABR_WSTRING,
        SRCH_PARSER_MODIFIER_EXCLUSION_LIST_FILTER_ID,
        SRCH_PARSER_MODIFIER_SEPARATOR_WSTRING,
        SRCH_PARSER_TRAILER_REQUIRED,
    },
    {   SRCH_PARSER_MODIFIER_INCLUSION_LIST_FILTER_ABR_WSTRING,
        SRCH_PARSER_MODIFIER_INCLUSION_LIST_FILTER_ID,
        SRCH_PARSER_MODIFIER_SEPARATOR_WSTRING,
        SRCH_PARSER_TRAILER_REQUIRED,
    },

    {   SRCH_PARSER_MODIFIER_LANGUAGE_WSTRING,
        SRCH_PARSER_MODIFIER_LANGUAGE_ID,
        SRCH_PARSER_MODIFIER_SEPARATOR_WSTRING,
        SRCH_PARSER_TRAILER_REQUIRED,
    },
    {   SRCH_PARSER_MODIFIER_LANGUAGE_ABR_WSTRING,
        SRCH_PARSER_MODIFIER_LANGUAGE_ID,
        SRCH_PARSER_MODIFIER_SEPARATOR_WSTRING,
        SRCH_PARSER_TRAILER_REQUIRED,
    },

    {   SRCH_PARSER_MODIFIER_TAG_WSTRING,
        SRCH_PARSER_MODIFIER_TAG_ID,
        SRCH_PARSER_MODIFIER_SEPARATOR_WSTRING,
        SRCH_PARSER_TRAILER_REQUIRED,
    },
    {   SRCH_PARSER_MODIFIER_TAG_ABR_WSTRING,
        SRCH_PARSER_MODIFIER_TAG_ID,
        SRCH_PARSER_MODIFIER_SEPARATOR_WSTRING,
        SRCH_PARSER_TRAILER_REQUIRED,
    },

    /* End marker */
    {   NULL,
        SRCH_PARSER_INVALID_ID,
        NULL,
        SRCH_PARSER_TRAILER_INVALID, 
    },
};


/* Permissible sort order tokens, no precendence */
static struct srchParserToken pptParserTokenSortOrderListGlobal[] = 
{
    {   SRCH_PARSER_SORT_ORDER_ASC_WSTRING,
        SRCH_PARSER_SORT_ORDER_ASC_ID,
        NULL,
        SRCH_PARSER_TRAILER_NONE,
    },
    {   SRCH_PARSER_SORT_ORDER_DESC_WSTRING,
        SRCH_PARSER_SORT_ORDER_DESC_ID,
        NULL,
        SRCH_PARSER_TRAILER_NONE,
    },
    {   SRCH_PARSER_SORT_ORDER_ASC_ABR_WSTRING,
        SRCH_PARSER_SORT_ORDER_ASC_ID,
        NULL,
        SRCH_PARSER_TRAILER_NONE,
    },
    {   SRCH_PARSER_SORT_ORDER_DESC_ABR_WSTRING,
        SRCH_PARSER_SORT_ORDER_DESC_ID,
        NULL,
        SRCH_PARSER_TRAILER_NONE,
    },
};


/* Internal sort field name tokens, no precendence */
static struct srchParserToken pptParserTokenSortFieldNameListGlobal[] = 
{
    {   SRCH_PARSER_SORT_FIELD_NAME_RELEVANCE_WSTRING,
        SRCH_PARSER_SORT_FIELD_NAME_RELEVANCE_ID,
        NULL,
        SRCH_PARSER_TRAILER_NONE,
    },
    {   SRCH_PARSER_SORT_FIELD_NAME_RANK_WSTRING,
        SRCH_PARSER_SORT_FIELD_NAME_RANK_ID,
        NULL,
        SRCH_PARSER_TRAILER_NONE,
    },
    {   SRCH_PARSER_SORT_FIELD_NAME_DATE_WSTRING,
        SRCH_PARSER_SORT_FIELD_NAME_DATE_ID,
        NULL,
        SRCH_PARSER_TRAILER_NONE,
    },
    {   SRCH_PARSER_SORT_FIELD_NAME_RELEVANCE_ABR_WSTRING,
        SRCH_PARSER_SORT_FIELD_NAME_RELEVANCE_ID,
        NULL,
        SRCH_PARSER_TRAILER_NONE,
    },
    {   SRCH_PARSER_SORT_FIELD_NAME_RANK_ABR_WSTRING,
        SRCH_PARSER_SORT_FIELD_NAME_RANK_ID,
        NULL,
        SRCH_PARSER_TRAILER_NONE,
    },
    {   SRCH_PARSER_SORT_FIELD_NAME_DATE_ABR_WSTRING,
        SRCH_PARSER_SORT_FIELD_NAME_DATE_ID,
        NULL,
        SRCH_PARSER_TRAILER_NONE,
    },
};


/* Permissible operators tokens, in reverse order of precendence */
static struct srchParserToken pptParserTokenOperatorListGlobal[] = 
{
    {   SRCH_PARSER_OPERATOR_ADJ_WSTRING,
        SRCH_PARSER_OPERATOR_ADJ_ID,
        NULL,
        SRCH_PARSER_TRAILER_NONE,
    },
    {   SRCH_PARSER_OPERATOR_NEAR_WSTRING,
        SRCH_PARSER_OPERATOR_NEAR_ID,
        SRCH_PARSER_CHARACTER_LSQUARE_BRACKET_WSTRING,
        SRCH_PARSER_TRAILER_OPTIONAL,
    },
    {   SRCH_PARSER_OPERATOR_WITHIN_WSTRING,
        SRCH_PARSER_OPERATOR_NEAR_ID,
        SRCH_PARSER_CHARACTER_LSQUARE_BRACKET_WSTRING,
        SRCH_PARSER_TRAILER_OPTIONAL,
    },
    {   SRCH_PARSER_OPERATOR_OR_WSTRING,
        SRCH_PARSER_OPERATOR_OR_ID,
        NULL,
        SRCH_PARSER_TRAILER_NONE,
    },
    {   SRCH_PARSER_OPERATOR_PIPE_WSTRING,
        SRCH_PARSER_OPERATOR_OR_ID,
        NULL,
        SRCH_PARSER_TRAILER_NONE,
    },
    {   SRCH_PARSER_OPERATOR_DOUBLE_PIPE_WSTRING,
        SRCH_PARSER_OPERATOR_OR_ID,    
        NULL,
        SRCH_PARSER_TRAILER_NONE, 
    },
    {   SRCH_PARSER_OPERATOR_IOR_WSTRING,
        SRCH_PARSER_OPERATOR_IOR_ID,
        NULL,
        SRCH_PARSER_TRAILER_NONE,
    },
    {   SRCH_PARSER_OPERATOR_XOR_WSTRING,
        SRCH_PARSER_OPERATOR_XOR_ID,
        NULL,
        SRCH_PARSER_TRAILER_NONE,
    },
    {   SRCH_PARSER_OPERATOR_AND_WSTRING,
        SRCH_PARSER_OPERATOR_AND_ID,
        NULL,
        SRCH_PARSER_TRAILER_NONE,
    },
    {   SRCH_PARSER_OPERATOR_AMPERSAND_WSTRING,
        SRCH_PARSER_OPERATOR_AND_ID,
        NULL,
        SRCH_PARSER_TRAILER_NONE,
    },
    {   SRCH_PARSER_OPERATOR_DOUBLE_AMPERSAND_WSTRING,
        SRCH_PARSER_OPERATOR_AND_ID,
        NULL,
        SRCH_PARSER_TRAILER_NONE,
    },
    {   SRCH_PARSER_OPERATOR_PLUS_WSTRING,
        SRCH_PARSER_OPERATOR_AND_ID,
        NULL,
        SRCH_PARSER_TRAILER_NONE,
    },
    {   SRCH_PARSER_OPERATOR_NOT_WSTRING,
        SRCH_PARSER_OPERATOR_NOT_ID,
        NULL,
        SRCH_PARSER_TRAILER_NONE,
    },
    {   SRCH_PARSER_OPERATOR_ANDNOT_WSTRING,
        SRCH_PARSER_OPERATOR_NOT_ID,
        NULL,
        SRCH_PARSER_TRAILER_NONE,
    },
    {   SRCH_PARSER_OPERATOR_CARET_WSTRING,
        SRCH_PARSER_OPERATOR_NOT_ID,
        NULL,
        SRCH_PARSER_TRAILER_NONE,
    },
    {   SRCH_PARSER_OPERATOR_LPAREN_WSTRING,
        SRCH_PARSER_OPERATOR_LPAREN_ID,
        NULL,
        SRCH_PARSER_TRAILER_NONE,
    },
    {   SRCH_PARSER_OPERATOR_RPAREN_WSTRING,
        SRCH_PARSER_OPERATOR_RPAREN_ID,
        NULL,
        SRCH_PARSER_TRAILER_NONE,
    },

    /* End marker */
    {   NULL,
        SRCH_PARSER_INVALID_ID,
        NULL,
        SRCH_PARSER_TRAILER_INVALID,
    },
};


/* Permissible range tokens, no precendence */
static struct srchParserToken pptParserTokenRangeListGlobal[] = 
{
    {   SRCH_PARSER_RANGE_EQUAL_WSTRING,                    
        SRCH_PARSER_RANGE_EQUAL_ID,
        NULL,
        SRCH_PARSER_TRAILER_NONE,
    },
    {   SRCH_PARSER_RANGE_COLON_WSTRING,
        SRCH_PARSER_RANGE_EQUAL_ID,
        NULL,
        SRCH_PARSER_TRAILER_NONE,
    },
    {   SRCH_PARSER_RANGE_NOT_EQUAL_WSTRING,
        SRCH_PARSER_RANGE_NOT_EQUAL_ID,
        NULL,
        SRCH_PARSER_TRAILER_NONE,
    },
    {   SRCH_PARSER_RANGE_GREATER_WSTRING,
        SRCH_PARSER_RANGE_GREATER_ID,
        NULL,
        SRCH_PARSER_TRAILER_NONE, 
    },
    {   SRCH_PARSER_RANGE_LESS_WSTRING,
        SRCH_PARSER_RANGE_LESS_ID,
        NULL,
        SRCH_PARSER_TRAILER_NONE,
    },
    {   SRCH_PARSER_RANGE_GREATER_OR_EQUAL_WSTRING,
        SRCH_PARSER_RANGE_GREATER_OR_EQUAL_ID,
        NULL,
        SRCH_PARSER_TRAILER_NONE,
    },
    {   SRCH_PARSER_RANGE_LESS_OR_EQUAL_WSTRING,
        SRCH_PARSER_RANGE_LESS_OR_EQUAL_ID,
        NULL,
        SRCH_PARSER_TRAILER_NONE,
    },
    {   NULL,
        SRCH_PARSER_INVALID_ID,
        NULL,
        SRCH_PARSER_TRAILER_INVALID, 
    },
};


/* Permissible function tokens, no precendence */
static struct srchParserToken pptParserTokenFunctionListGlobal[] = 
{
#if defined(SRCH_PARSER_ENABLE_METAPHONE_FUNCTION)
    {   SRCH_PARSER_FUNCTION_METAPHONE_WSTRING,
        SRCH_PARSER_FUNCTION_METAPHONE_ID,
        SRCH_PARSER_CHARACTER_LSQUARE_BRACKET_WSTRING,
        SRCH_PARSER_TRAILER_OPTIONAL,
    },
    {   SRCH_PARSER_FUNCTION_SOUNDSLIKE_WSTRING,
        SRCH_PARSER_FUNCTION_METAPHONE_ID,
        SRCH_PARSER_CHARACTER_LSQUARE_BRACKET_WSTRING,
        SRCH_PARSER_TRAILER_OPTIONAL,
    },
#endif /* defined(SRCH_PARSER_ENABLE_METAPHONE_FUNCTION) */

#if defined(SRCH_PARSER_ENABLE_SOUNDEX_FUNCTION)
    {   SRCH_PARSER_FUNCTION_SOUNDEX_WSTRING,
        SRCH_PARSER_FUNCTION_SOUNDEX_ID,
        SRCH_PARSER_CHARACTER_LSQUARE_BRACKET_WSTRING,
        SRCH_PARSER_TRAILER_OPTIONAL,
    },
#endif /* defined(SRCH_PARSER_ENABLE_SOUNDEX_FUNCTION) */

#if defined(SRCH_PARSER_ENABLE_PHONIX_FUNCTION)
    {   SRCH_PARSER_FUNCTION_PHONIX_WSTRING,
        SRCH_PARSER_FUNCTION_PHONIX_ID,
        SRCH_PARSER_CHARACTER_LSQUARE_BRACKET_WSTRING,
        SRCH_PARSER_TRAILER_OPTIONAL,
    },
#endif /* defined(SRCH_PARSER_ENABLE_PHONIX_FUNCTION) */

#if defined(SRCH_PARSER_ENABLE_TYPO_FUNCTION)
    {   SRCH_PARSER_FUNCTION_TYPO_WSTRING,
        SRCH_PARSER_FUNCTION_TYPO_ID,
        SRCH_PARSER_CHARACTER_LSQUARE_BRACKET_WSTRING,
        SRCH_PARSER_TRAILER_OPTIONAL,
    },
#endif /* defined(SRCH_PARSER_ENABLE_TYPO_FUNCTION) */

#if defined(SRCH_PARSER_ENABLE_REGEX_FUNCTION)
    {   SRCH_PARSER_FUNCTION_REGEX_WSTRING,
        SRCH_PARSER_FUNCTION_REGEX_ID,
        SRCH_PARSER_CHARACTER_LSQUARE_BRACKET_WSTRING,
        SRCH_PARSER_TRAILER_OPTIONAL,
    },
#endif /* defined(SRCH_PARSER_ENABLE_REGEX_FUNCTION) */

#if defined(SRCH_PARSER_ENABLE_REGEXP_FUNCTION)
    {   SRCH_PARSER_FUNCTION_REGEXP_WSTRING,
        SRCH_PARSER_FUNCTION_REGEX_ID,
        SRCH_PARSER_CHARACTER_LSQUARE_BRACKET_WSTRING,
        SRCH_PARSER_TRAILER_OPTIONAL,
    },
#endif /* defined(SRCH_PARSER_ENABLE_REGEXP_FUNCTION) */

#if defined(SRCH_PARSER_ENABLE_LITERAL_FUNCTION)
    {   SRCH_PARSER_FUNCTION_LITERAL_WSTRING,
        SRCH_PARSER_FUNCTION_LITERAL_ID,
        SRCH_PARSER_CHARACTER_LSQUARE_BRACKET_WSTRING,
        SRCH_PARSER_TRAILER_OPTIONAL,
    },
#endif /* defined(SRCH_PARSER_ENABLE_LITERAL_FUNCTION) */

#if defined(SRCH_PARSER_ENABLE_RANGE_FUNCTION)
    {   SRCH_PARSER_FUNCTION_RANGE_WSTRING,
        SRCH_PARSER_FUNCTION_RANGE_ID,
        SRCH_PARSER_CHARACTER_LSQUARE_BRACKET_WSTRING,
        SRCH_PARSER_TRAILER_OPTIONAL,
    },
#endif /* defined(SRCH_PARSER_ENABLE_RANGE_FUNCTION) */

    /* End marker */
    {   NULL,
        SRCH_PARSER_INVALID_ID,
        NULL,
        SRCH_PARSER_TRAILER_INVALID,
    },
};



/* Date name list */
static struct srchParserDateName pspdnParserDateNameListGlobal[] = 
{
    /* English */
    {   SRCH_PARSER_DATE_NAME_TODAY_WSTRING_EN,
        SRCH_PARSER_DATE_NAME_TODAY_ID,
    },
    {   SRCH_PARSER_DATE_NAME_YESTERDAY_WSTRING_EN,
        SRCH_PARSER_DATE_NAME_YESTERDAY_ID,
    },
    {   SRCH_PARSER_DATE_NAME_THIS_WEEK_WSTRING_EN,
        SRCH_PARSER_DATE_NAME_THIS_WEEK_ID,
    },
    {   SRCH_PARSER_DATE_NAME_LAST_WEEK_WSTRING_EN,
        SRCH_PARSER_DATE_NAME_LAST_WEEK_ID,
    },
    {   SRCH_PARSER_DATE_NAME_THIS_MONTH_WSTRING_EN,
        SRCH_PARSER_DATE_NAME_THIS_MONTH_ID,
    },
    {   SRCH_PARSER_DATE_NAME_LAST_MONTH_WSTRING_EN,
        SRCH_PARSER_DATE_NAME_LAST_MONTH_ID,
    },
    {   SRCH_PARSER_DATE_NAME_THIS_YEAR_WSTRING_EN,
        SRCH_PARSER_DATE_NAME_THIS_YEAR_ID,
    },
    {   SRCH_PARSER_DATE_NAME_LAST_YEAR_WSTRING_EN,
        SRCH_PARSER_DATE_NAME_LAST_YEAR_ID,
    },
    {   SRCH_PARSER_DATE_NAME_SUNDAY_WSTRING_EN,
        SRCH_PARSER_DATE_NAME_SUNDAY_ID,
    },
    {   SRCH_PARSER_DATE_NAME_MONDAY_WSTRING_EN,
        SRCH_PARSER_DATE_NAME_MONDAY_ID,
    },
    {   SRCH_PARSER_DATE_NAME_TUESDAY_WSTRING_EN,
        SRCH_PARSER_DATE_NAME_TUESDAY_ID,
    },
    {   SRCH_PARSER_DATE_NAME_WEDNESDAY_WSTRING_EN,
        SRCH_PARSER_DATE_NAME_WEDNESDAY_ID,
    },
    {   SRCH_PARSER_DATE_NAME_THURSDAY_WSTRING_EN,
        SRCH_PARSER_DATE_NAME_THURSDAY_ID,
    },
    {   SRCH_PARSER_DATE_NAME_FRIDAY_WSTRING_EN,
        SRCH_PARSER_DATE_NAME_FRIDAY_ID,
    },
    {   SRCH_PARSER_DATE_NAME_SATURDAY_WSTRING_EN,
        SRCH_PARSER_DATE_NAME_SATURDAY_ID,
    },
    {   SRCH_PARSER_DATE_NAME_JANUARY_WSTRING_EN,
        SRCH_PARSER_DATE_NAME_JANUARY_ID,
    },
    {   SRCH_PARSER_DATE_NAME_FEBRUARY_WSTRING_EN,
        SRCH_PARSER_DATE_NAME_FEBRUARY_ID,
    },
    {   SRCH_PARSER_DATE_NAME_MARCH_WSTRING_EN,
        SRCH_PARSER_DATE_NAME_MARCH_ID,
    },
    {   SRCH_PARSER_DATE_NAME_APRIL_WSTRING_EN,
        SRCH_PARSER_DATE_NAME_APRIL_ID,
    },
    {   SRCH_PARSER_DATE_NAME_MAY_WSTRING_EN,
        SRCH_PARSER_DATE_NAME_MAY_ID,
    },
    {   SRCH_PARSER_DATE_NAME_JUNE_WSTRING_EN,
        SRCH_PARSER_DATE_NAME_JUNE_ID,
    },
    {   SRCH_PARSER_DATE_NAME_JULY_WSTRING_EN,
        SRCH_PARSER_DATE_NAME_JULY_ID,
    },
    {   SRCH_PARSER_DATE_NAME_AUGUST_WSTRING_EN,
        SRCH_PARSER_DATE_NAME_AUGUST_ID,
    },
    {   SRCH_PARSER_DATE_NAME_SEPTEMBER_WSTRING_EN,
        SRCH_PARSER_DATE_NAME_SEPTEMBER_ID,
    },
    {   SRCH_PARSER_DATE_NAME_OCTOBER_WSTRING_EN,
        SRCH_PARSER_DATE_NAME_OCTOBER_ID,
    },
    {   SRCH_PARSER_DATE_NAME_NOVEMBER_WSTRING_EN,
        SRCH_PARSER_DATE_NAME_NOVEMBER_ID,
    },
    {   SRCH_PARSER_DATE_NAME_DECEMBER_WSTRING_EN,
        SRCH_PARSER_DATE_NAME_DECEMBER_ID,
    },

    /* French */
    {   SRCH_PARSER_DATE_NAME_TODAY_WSTRING_FR,
        SRCH_PARSER_DATE_NAME_TODAY_ID,
    },
    {   SRCH_PARSER_DATE_NAME_YESTERDAY_WSTRING_FR,
        SRCH_PARSER_DATE_NAME_YESTERDAY_ID,
    },
    {   SRCH_PARSER_DATE_NAME_THIS_WEEK_WSTRING_FR,
        SRCH_PARSER_DATE_NAME_THIS_WEEK_ID,
    },
    {   SRCH_PARSER_DATE_NAME_LAST_WEEK_WSTRING_FR,
        SRCH_PARSER_DATE_NAME_LAST_WEEK_ID,
    },
    {   SRCH_PARSER_DATE_NAME_THIS_MONTH_WSTRING_FR,
        SRCH_PARSER_DATE_NAME_THIS_MONTH_ID,
    },
    {   SRCH_PARSER_DATE_NAME_LAST_MONTH_WSTRING_FR,
        SRCH_PARSER_DATE_NAME_LAST_MONTH_ID,
    },
    {   SRCH_PARSER_DATE_NAME_THIS_YEAR_WSTRING_FR,
        SRCH_PARSER_DATE_NAME_THIS_YEAR_ID,
    },
    {   SRCH_PARSER_DATE_NAME_LAST_YEAR_WSTRING_FR,
        SRCH_PARSER_DATE_NAME_LAST_YEAR_ID,
    },
    {   SRCH_PARSER_DATE_NAME_SUNDAY_WSTRING_FR,
        SRCH_PARSER_DATE_NAME_SUNDAY_ID,
    },
    {   SRCH_PARSER_DATE_NAME_MONDAY_WSTRING_FR,
        SRCH_PARSER_DATE_NAME_MONDAY_ID,
    },
    {   SRCH_PARSER_DATE_NAME_TUESDAY_WSTRING_FR,
        SRCH_PARSER_DATE_NAME_TUESDAY_ID,
    },
    {   SRCH_PARSER_DATE_NAME_WEDNESDAY_WSTRING_FR,
        SRCH_PARSER_DATE_NAME_WEDNESDAY_ID,
    },
    {   SRCH_PARSER_DATE_NAME_THURSDAY_WSTRING_FR,
        SRCH_PARSER_DATE_NAME_THURSDAY_ID,
    },
    {   SRCH_PARSER_DATE_NAME_FRIDAY_WSTRING_FR,
        SRCH_PARSER_DATE_NAME_FRIDAY_ID,
    },
    {   SRCH_PARSER_DATE_NAME_SATURDAY_WSTRING_FR,
        SRCH_PARSER_DATE_NAME_SATURDAY_ID,
    },
    {   SRCH_PARSER_DATE_NAME_JANUARY_WSTRING_FR,
        SRCH_PARSER_DATE_NAME_JANUARY_ID,
    },
    {   SRCH_PARSER_DATE_NAME_FEBRUARY_WSTRING_FR,
        SRCH_PARSER_DATE_NAME_FEBRUARY_ID,
    },
    {   SRCH_PARSER_DATE_NAME_MARCH_WSTRING_FR,
        SRCH_PARSER_DATE_NAME_MARCH_ID,
    },
    {   SRCH_PARSER_DATE_NAME_APRIL_WSTRING_FR,
        SRCH_PARSER_DATE_NAME_APRIL_ID,
    },
    {   SRCH_PARSER_DATE_NAME_MAY_WSTRING_FR,
        SRCH_PARSER_DATE_NAME_MAY_ID,
    },
    {   SRCH_PARSER_DATE_NAME_JUNE_WSTRING_FR,
        SRCH_PARSER_DATE_NAME_JUNE_ID,
    },
    {   SRCH_PARSER_DATE_NAME_JULY_WSTRING_FR,
        SRCH_PARSER_DATE_NAME_JULY_ID,
    },
    {   SRCH_PARSER_DATE_NAME_AUGUST_WSTRING_FR,
        SRCH_PARSER_DATE_NAME_AUGUST_ID,
    },
    {   SRCH_PARSER_DATE_NAME_SEPTEMBER_WSTRING_FR,
        SRCH_PARSER_DATE_NAME_SEPTEMBER_ID,
    },
    {   SRCH_PARSER_DATE_NAME_OCTOBER_WSTRING_FR,
        SRCH_PARSER_DATE_NAME_OCTOBER_ID,
    },
    {   SRCH_PARSER_DATE_NAME_NOVEMBER_WSTRING_FR,
        SRCH_PARSER_DATE_NAME_NOVEMBER_ID,
    },
    {   SRCH_PARSER_DATE_NAME_DECEMBER_WSTRING_FR,
        SRCH_PARSER_DATE_NAME_DECEMBER_ID,
    },

    /* End marker */
    {   NULL,
        SRCH_PARSER_DATE_NAME_INVALID_ID,
    },
};



/* Replace list - from string and to string need to be the same length */
static struct srchParserReplace psprParserReplaceStringListGlobal[] = 
{
    /* Convert ' ( not ' => ' not ( ' , this is really ugly but needed 
    ** to deal with searches in the following form:
    **
    **  iraq (not domain:5000megs.com domain:4000megs.com domain:3000megs.com)
    **  iraq and ( (bush or war) and (not domain:5000megs.com) )
    **  ( iraq and (bush or war) ) and (not domain:5000megs.com)
    **
    ** By the time searches get here, they are normalized so a 
    ** simple pattern match and replace will work just fine so:
    **
    **  ( iraq and (bush or war) ) and (not domain:5000megs.com)
    **
    ** will look like:
    **
    **  ( iraq and ( bush or war ) ) and ( not domain:5000megs.com )
    */
    {   L" ( not ",
        L" not ( ",
    },

    /* Replace: " and not " with " not " */
/*     {    L" and not ", */
/*          L" not     ", */
/*     }, */

    /* Replace: " or not " with " not " */
/*     {    L" or not ", */
/*          L" not    ", */
/*     }, */

    /* Replace: " adj not " with " not " */
/*     {    L" adj not ", */
/*          L" not     ", */
/*     }, */

    /* Terminator */
    {   NULL,
        NULL,
    },
};


/* Error mapping global */
static struct srchParserErrorMapping pspemSrchParserMappingsGlobal[] = 
{
    {SRCH_ParserInvalidRange,                                   (unsigned char *)"Search contains an invalid range"}, 
    {SRCH_ParserInvalidOperator,                                (unsigned char *)"Search contains an invalid operator"}, 
    {SRCH_ParserInvalidModifier,                                (unsigned char *)"Search contains an invalid modifier"}, 
    {SRCH_ParserInvalidFunction,                                (unsigned char *)"Search contains an invalid function"}, 
    {SRCH_ParserInvalidToken,                                   (unsigned char *)"Search contains an invalid token"}, 
    {SRCH_ParserInvalidBracket,                                 (unsigned char *)"Search contains an uneven number of brackets"}, 
    {SRCH_ParserInvalidQuote,                                   (unsigned char *)"Search contains an uneven number of quotes"}, 
    {SRCH_ParserInvalidWildCard,                                (unsigned char *)"Search contains a wildcard error"}, 
    {SRCH_ParserInvalidSyntax,                                  (unsigned char *)"Search contains a syntax error"}, 
    {SRCH_ParserInvalidOperatorDistance,                        (unsigned char *)"Search contains an invalid operator distance"}, 
    {SRCH_ParserInvalidNotOperator,                             (unsigned char *)"Search contains an invalid use of the 'NOT' operator"}, 
    {SRCH_ParserInvalidSort,                                    (unsigned char *)"Search contains an invalid sort"}, 
    {SRCH_ParserInvalidSortOrder,                               (unsigned char *)"Search contains an invalid sort order"}, 
    {SRCH_ParserInvalidDate,                                    (unsigned char *)"Search contains an invalid date"}, 
    {SRCH_ParserInvalidTermWeight,                              (unsigned char *)"Search contains an invalid term weight"}, 
    {SRCH_ParserInvalidFeedbackTermWeight,                      (unsigned char *)"Search contains an invalid feedback term weight"}, 
    {SRCH_ParserInvalidFrequentTermCoverageThreshold,           (unsigned char *)"Search contains an invalid frequent term coverage threshold"}, 
    {SRCH_ParserInvalidMinimumTermCount,                        (unsigned char *)"Search contains an invalid minimum term count"}, 
    {SRCH_ParserInvalidFeedbackMaximumTermPercentage,           (unsigned char *)"Search contains an invalid feedback maximum term percentage"}, 
    {SRCH_ParserInvalidFeedbackMaximumTermCoverageThreshold,    (unsigned char *)"Search contains an invalid feedback maximum term coverage threshold"}, 
    {SRCH_ParserInvalidConnectionTimeout,                       (unsigned char *)"Search contains an invalid connection timeout"}, 
    {SRCH_ParserInvalidSearchTimeout,                           (unsigned char *)"Search contains an invalid search timeout"}, 
    {SRCH_ParserInvalidRetrievalTimeout,                        (unsigned char *)"Search contains an invalid retrieval timeout"}, 
    {SRCH_ParserInvalidInformationTimeout,                      (unsigned char *)"Search contains an invalid information timeout"}, 
    {SRCH_ParserInvalidSegmentsSearchedMaximum,                 (unsigned char *)"Search contains an invalid segments searched maximum"}, 
    {SRCH_ParserInvalidSegmentsSearchedMinimum,                 (unsigned char *)"Search contains an invalid segments searched minimum"}, 
    {SRCH_ParserInvalidExclusionFilter,                         (unsigned char *)"Search contains an invalid exclusion filter"}, 
    {SRCH_ParserInvalidInclusionFilter,                         (unsigned char *)"Search contains an invalid inclusion filter"}, 
    {SRCH_ParserInvalidLanguage,                                (unsigned char *)"Search contains an invalid language"}, 
    {SRCH_ParserRegexNoMatch,                                   (unsigned char *)"Regex function failed to match"}, 
    {SRCH_ParserRegexBadRpt,                                    (unsigned char *)"Regex contained a ?, * or + which is not preceded by valid regular expression"}, 
    {SRCH_ParserRegexBadBr,                                     (unsigned char *)"Content of \\{ \\} invalid: not a number, number too large, more than two numbers, first larger than second"}, 
    {SRCH_ParserRegexBadPat,                                    (unsigned char *)"Regex contained an invalid use of pattern operators such as group or list"}, 
    {SRCH_ParserRegexEBrace,                                    (unsigned char *)"Regex contained an \\{ \\} imbalance"}, 
    {SRCH_ParserRegexEBrack,                                    (unsigned char *)"Regex contained an [] imbalance"}, 
    {SRCH_ParserRegexERange,                                    (unsigned char *)"Regex contained an invalid use of the range operator, eg. the ending point of the range occurs prior to the starting point"}, 
    {SRCH_ParserRegexECType,                                    (unsigned char *)"Regex contained an invalid character class type"}, 
    {SRCH_ParserRegexECollate,                                  (unsigned char *)"Regex contained an invalid collating element"}, 
    {SRCH_ParserRegexEParen,                                    (unsigned char *)"Regex contained an \\(\\) or () imbalance"}, 
    {SRCH_ParserRegexESubReg,                                   (unsigned char *)"Regex contained an invalid back reference to a subexpression"}, 
    {SRCH_ParserRegexEEscape,                                   (unsigned char *)"Regex contained a trailing \\ in pattern"}, 
    {SRCH_ParserRegexESpace,                                    (unsigned char *)"Regex ran out of memory"}, 
    {SRCH_ParserRegexEEnd,                                      (unsigned char *)"Regex contained a non specific error"}, 
    {SRCH_ParserRegexESize,                                     (unsigned char *)"Regex compiled a regular expression that requires a pattern buffer larger than 64Kb"}, 
    {SRCH_ParserRegexENoSys,                                    (unsigned char *)"Regex contained a function is not supported"}, 
    {SRCH_ParserRegexFailed,                                    (unsigned char *)"Regex failed"}, 
    {SRCH_ParserInvalidParser,                                  (unsigned char *)"Search failed to be parsed by the parser"}, 
    {SRCH_ParserInvalidParser,                                  (unsigned char *)"Search failed to be parsed by the parser"}, 
    {SRCH_PARSER_MAPPING_END_MARKER,                            NULL,}
};


/*---------------------------------------------------------------------------*/


/*
** Private function prototypes
*/

static int iSrchParserSetDefaults (struct srchParser *pspSrchParser);
static int iSrchParserFreeStructureFields (struct srchParser *pspSrchParser);

static int iSrchParserCleanSearch (struct srchParser *pspSrchParser, 
        wchar_t *pwcSearchText, wchar_t **ppwcCleanedSearch);

static int iSrchParserSubstituteTokens (struct srchParser *pspSrchParser, 
        wchar_t *pwcSearchText, wchar_t **ppwcSubstitutedSearch);

static int iSrchParserCheckSyntax (struct srchParser *pspSrchParser,
        wchar_t *pwcSearchText, wchar_t **ppwcCheckedSyntaxSearch);

static int iSrchParserExtractModifiers (struct srchParser *pspSrchParser,
        wchar_t *pwcSearchText, wchar_t **ppwcExtractedSearch);

static int iSrchParserNormalizeSearch (struct srchParser *pspSrchParser, 
        wchar_t *pwcSearchText, wchar_t **ppwcNormalizedSearch);

static int iSrchParserCreateTermCluster (struct srchParser *pspSrchParser, 
        wchar_t *pwcSearchText, struct srchParserTermCluster **ppsptcSrchParserTermCluster);
static int iSrchParserCreateTermCluster_r (struct srchParser *pspSrchParser,
        wchar_t **ppwcSearchText, struct srchParserTermCluster **ppsptcSrchParserTermCluster);
static int iSrchParserAddTermToTermCluster (struct srchParser *pspSrchParser,
    struct srchParserTermCluster *psptcSrchParserTermCluster, wchar_t *pwcToken, wchar_t *pwcTerm, 
    wchar_t *pwcFieldName, wchar_t *pwcRange, wchar_t *pwcFunction, wchar_t *pwcWeight);

static int iSrchParserCheckTermCluster (struct srchParser *pspSrchParser, 
        struct srchParserTermCluster *psptcSrchParserTermCluster);

static int iSrchParserTermClusterToNormalizedSearchText (struct srchParser *pspSrchParser,
        struct srchParserTermCluster *psptcSrchParserTermCluster, void *pvUtlNormalizedSearchWideStringBuffer);

static int iSrchParserFreeTerms (struct srchParserTerm *pstSrchParserTerm, unsigned int uiSrchParserTermLength);
static int iSrchParserFreeTermCluster (struct srchParserTermCluster *psptcSrchParserTermCluster);

static int iSrchParserFreeNumbers (struct srchParserNumber *pspnSrchParserNumbers, 
        unsigned int uiSrchParserNumberLength);

static int iSrchParserFreeFilters (struct srchParserFilter *pspfSrchParserFilters, 
        unsigned int uiSrchParserFiltersLength);


static unsigned int uiSrchParserGetRangeIDFromStartOfString (struct srchParser *pspSrchParser, 
        wchar_t *pwcTokenString);

static unsigned int uiSrchParserGetRangeIDFromEndOfString (struct srchParser *pspSrchParser, 
        wchar_t *pwcTokenString);

static unsigned int uiSrchParserGetIDFromString (struct srchParser *pspSrchParser, 
        wchar_t *pwcTokenString, unsigned int uiTokenListID);



static int iSrchParserGetAnsiDateFromDateName (struct srchParser *pspSrchParser, 
        wchar_t *pwcRange, wchar_t *pwcDateName, unsigned long *pulNormalizedDateValue);


/*---------------------------------------------------------------------------*/


/*
** Macros
*/

/* Macros to get the token IDs from token strings */
#define uiSrchParserGetRangeIDFromString(p, s)              uiSrchParserGetIDFromString((p), (s), SRCH_PARSER_LIST_RANGE_ID)
#define uiSrchParserGetOperatorIDFromString(p, s)           uiSrchParserGetIDFromString((p), (s), SRCH_PARSER_LIST_OPERATOR_ID)
#define uiSrchParserGetModifierIDFromString(p, s)           uiSrchParserGetIDFromString((p), (s), SRCH_PARSER_LIST_MODIFIER_ID)
#define uiSrchParserGetSortOrderIDFromString(p, s)          uiSrchParserGetIDFromString((p), (s), SRCH_PARSER_LIST_SORT_ORDER_ID)
#define uiSrchParserGetSortFieldNameIDFromString(p, s)      uiSrchParserGetIDFromString((p), (s), SRCH_PARSER_LIST_SORT_FIELD_NAME_ID)
#define uiSrchParserGetFunctionIDFromString(p, s)           uiSrchParserGetIDFromString((p), (s), SRCH_PARSER_LIST_FUNCTION_ID)


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchParserCreate()

    Purpose:    This function initializse the search parser. 

    Parameters: pssSrchSearch   search structure
                ppvSrchParser   return pointer for the search parser structure

    Globals:    none

    Returns:    SRCH error code

*/
int iSrchParserCreate
(
    struct srchSearch *pssSrchSearch,
    void **ppvSrchParser
)
{

    int                 iError = SRCH_NoError;
    struct srchParser   *pspSrchParser = NULL;


    /* Check the parameters */
    if ( pssSrchSearch == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pssSrchSearch' parameter passed to 'iSrchParserCreate'."); 
        return (SRCH_ParserInvalidSearch);
    }

    if ( ppvSrchParser == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppvSrchParser' parameter passed to 'iSrchParserCreate'."); 
        return (SRCH_ReturnParameterError);
    }


    /* Allocate a search parser structure */
    if ( (pspSrchParser = (struct srchParser *)s_malloc((size_t)sizeof(struct srchParser))) == NULL ) {
        return (SRCH_MemError);
    }


    /* Add a reference to the search structure */
    pspSrchParser->pssSrchSearch = pssSrchSearch;


    /* Set the search parser defaults */
    if ( (iError = iSrchParserSetDefaults(pspSrchParser)) != SRCH_NoError ) {
        iSrchParserFree((void *)pspSrchParser);
        pspSrchParser = NULL;
        return (SRCH_ParserInitFailed);
    }

    /* Set the return pointer */
    *ppvSrchParser = pspSrchParser;


    return (SRCH_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchParserParse()

    Purpose:    This function parses the search text. 

    Parameters: pvSrchParser    search parser structure
                uiLanguageID    language ID
                uiTokenizerID   tokenizer ID
                pwcSearchText   search text (optional)

    Globals:    none

    Returns:    SRCH error code

*/
int iSrchParserParse
(
    void *pvSrchParser,
    unsigned int uiLanguageID,
    unsigned int uiTokenizerID,
    wchar_t *pwcSearchText
)
{

    int                 iError = SRCH_NoError;
    struct srchParser   *pspSrchParser = (struct srchParser *)pvSrchParser;
    wchar_t             *pwcOriginalSearch = NULL;
    wchar_t             *pwcCleanedSearch = NULL;
    wchar_t             *pwcSubstitutedSearch = NULL;
    wchar_t             *pwcCheckedSyntaxSearch = NULL;
    wchar_t             *pwcExtractedSearch = NULL;
    wchar_t             *pwcNormalizedSearch = NULL;
    wchar_t             *pwcNormalizedSearchText = NULL;
    wchar_t             *pwcFullNormalizedSearchText = NULL;


    /* Check the parameters */
    if ( pvSrchParser == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvSrchParser' parameter passed to 'iSrchParserParse'."); 
        return (SRCH_ParserInvalidParser);
    }


    /* Reset the search parser, return an error if it failed */
    if ( (iError = iSrchParserReset((void *)pspSrchParser)) != SRCH_NoError ) {
        goto bailFromiSrchParserParse;
    }
    
    
    /* Set the search language ID and the search tokenizer ID */
    pspSrchParser->uiLanguageID = uiLanguageID;
    pspSrchParser->uiTokenizerID = uiTokenizerID;

    
    /* Create the tokenizer */
    if ( (iError = iLngTokenizerCreateByID(pspSrchParser->pssSrchSearch->pucConfigurationDirectoryPath, pspSrchParser->uiTokenizerID, 
            SRCH_PARSER_TOKENIZER_LANGUAGE_ID_DEFAULT, &pspSrchParser->pvLngTokenizer)) != LNG_NoError ) {
        iError = SRCH_ParserCreateTokenizerFailed;
        goto bailFromiSrchParserParse;
    }


    /* Duplicate the original search, storing it in a working pointer */
    if ( bUtlStringsIsWideStringNULL(pwcSearchText) == false ) {
        if ( (pwcOriginalSearch = s_wcsdup(pwcSearchText)) == NULL ) {
            iError = SRCH_MemError;
            goto bailFromiSrchParserParse;
        }
    }
    iUtlLogDebug(UTL_LOG_CONTEXT, "iSrchParserParse - pwcOriginalSearch: [%ls], uiLanguageID: [%u], uiTokenizerID: [%u].", 
            pwcUtlStringsGetPrintableWideString(pwcOriginalSearch), uiLanguageID, uiTokenizerID);

    /* Clean the search */
    if ( (iError = iSrchParserCleanSearch(pspSrchParser, pwcOriginalSearch, &pwcCleanedSearch)) != SRCH_NoError ) {
        goto bailFromiSrchParserParse;
    }
    iUtlLogDebug(UTL_LOG_CONTEXT, "iSrchParserParse - pwcCleanedSearch: [%ls].", pwcUtlStringsGetPrintableWideString(pwcCleanedSearch));


    /* Substitute tokens in the search, does nothing right now */
    if ( (iError = iSrchParserSubstituteTokens(pspSrchParser, pwcCleanedSearch, &pwcSubstitutedSearch)) != SRCH_NoError ) {
        pspSrchParser->pwcNormalizedSearchText = pwcSubstitutedSearch;
        pwcCleanedSearch = NULL;
        goto bailFromiSrchParserParse;
    }
    iUtlLogDebug(UTL_LOG_CONTEXT, "iSrchParserParse - pwcSubstitutedSearch: [%ls].", pwcUtlStringsGetPrintableWideString(pwcSubstitutedSearch));


    /* Check the syntax */
    if ( (iError = iSrchParserCheckSyntax(pspSrchParser, pwcSubstitutedSearch, &pwcCheckedSyntaxSearch)) != SRCH_NoError ) {
        pspSrchParser->pwcNormalizedSearchText = pwcCheckedSyntaxSearch;
        pwcSubstitutedSearch = NULL;
        goto bailFromiSrchParserParse;
    }
    iUtlLogDebug(UTL_LOG_CONTEXT, "iSrchParserParse - pwcCheckedSyntaxSearch: [%ls].", pwcUtlStringsGetPrintableWideString(pwcCheckedSyntaxSearch));


    /* Extract the modifiers from the search */
    if ( (iError = iSrchParserExtractModifiers(pspSrchParser, pwcCheckedSyntaxSearch, &pwcExtractedSearch)) != SRCH_NoError ) {
        pspSrchParser->pwcNormalizedSearchText = pwcCheckedSyntaxSearch;
        pwcCheckedSyntaxSearch = NULL;
        goto bailFromiSrchParserParse;
    }
    iUtlLogDebug(UTL_LOG_CONTEXT, "iSrchParserParse - pwcExtractedSearch: [%ls].", pwcUtlStringsGetPrintableWideString(pwcExtractedSearch));


    /* Normalize the search */
    if ( (iError = iSrchParserNormalizeSearch(pspSrchParser, pwcExtractedSearch, &pwcNormalizedSearch)) != SRCH_NoError ) {
        pspSrchParser->pwcNormalizedSearchText = pwcNormalizedSearch;
        pwcExtractedSearch = NULL;
        goto bailFromiSrchParserParse;
    }
    iUtlLogDebug(UTL_LOG_CONTEXT, "iSrchParserParse - pwcNormalizedSearch: [%ls].", pwcUtlStringsGetPrintableWideString(pwcNormalizedSearch));


    /* Parse the search to a term cluster */
    if ( (iError = iSrchParserCreateTermCluster(pspSrchParser, pwcNormalizedSearch, &pspSrchParser->psptcSrchParserTermCluster)) != SRCH_NoError ) {
        pspSrchParser->pwcNormalizedSearchText = pwcSubstitutedSearch;
        pwcSubstitutedSearch = NULL;
        goto bailFromiSrchParserParse;
    }


    /* Check the term cluster */
    if ( pspSrchParser->psptcSrchParserTermCluster != NULL ) {
    
        /* Check the term cluster */
        iError = iSrchParserCheckTermCluster(pspSrchParser, pspSrchParser->psptcSrchParserTermCluster);

        /* Bail on selected errors */
        if ( !((iError == SRCH_NoError) || (iError == SRCH_ParserInvalidNotOperator)) ) {
            goto bailFromiSrchParserParse;
        }

        /* Reset the error */
        iError = SRCH_NoError;
    }
    
    
    /* Get the normalized search text, we do this here to cache it now */
    if ( (iError = iSrchParserGetNormalizedSearchText((void *)pspSrchParser, &pwcNormalizedSearchText)) != SRCH_NoError ) {
        goto bailFromiSrchParserParse;
    }

    /* Get the full normalized search text, we do this here to cache it now */
    if ( (iError = iSrchParserGetFullNormalizedSearchText((void *)pspSrchParser, &pwcFullNormalizedSearchText)) != SRCH_NoError )  {
        goto bailFromiSrchParserParse;
    }

    
    
    if ( bUtlLogIsDebug(UTL_LOG_CONTEXT) == true ) {
        
        if ( pspSrchParser->psptcSrchParserTermCluster != NULL ) {
        
            unsigned int uiLevel = 0;
        
            printf("pwcNormalizedSearchText: [%ls]\n", pwcNormalizedSearchText);
            printf("pwcFullNormalizedSearchText: [%ls]\n", pwcFullNormalizedSearchText);
            printf("Cluster: uiOperatorID: [%ls], iTermDistance: [%d], bDistanceOrderMatters: [%s]\n",
                    pwcUtlStringsGetPrintableWideString(pwcSrchParserGetStringFromID(pspSrchParser, pspSrchParser->psptcSrchParserTermCluster->uiOperatorID)), 
                    pspSrchParser->psptcSrchParserTermCluster->iTermDistance, (pspSrchParser->psptcSrchParserTermCluster->bDistanceOrderMatters == true) ? "true" : "false");
        
            iSrchParserPrintTermCluster((void *)pspSrchParser, pspSrchParser->psptcSrchParserTermCluster, ++uiLevel);
        }
    }



    /* Bail label */
    bailFromiSrchParserParse:


    /* Free all allocated strings */    
    s_free(pwcOriginalSearch);
    s_free(pwcCleanedSearch);
    s_free(pwcSubstitutedSearch);
    s_free(pwcCheckedSyntaxSearch);
    s_free(pwcExtractedSearch);
    s_free(pwcNormalizedSearch);


    /* Free the tokenizer */
    iLngTokenizerFree(pspSrchParser->pvLngTokenizer);
    pspSrchParser->pvLngTokenizer = NULL;


    /* Reset the parser if the parse failed - commented out so that we retain 
    ** the pwcNormalizedSearchText for logging purposes
    */
/*     if ( iError != SRCH_NoError ) { */
/*         iSrchParserReset((void *)pspSrchParser); */
/*     } */


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchParserReset()

    Purpose:    This function resets the search parser. 

    Parameters: pvSrchParser    search parser structure

    Globals:    none

    Returns:    SRCH error code

*/
int iSrchParserReset
(
    void *pvSrchParser
)
{

    int                 iError = SRCH_NoError;
    struct srchParser   *pspSrchParser = (struct srchParser *)pvSrchParser;


    /* Check the parameters */
    if ( pvSrchParser == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvSrchParser' parameter passed to 'iSrchParserReset'."); 
        return (SRCH_ParserInvalidParser);
    }


    /* Free the the search parser structure fields */
    if ( (iError = iSrchParserFreeStructureFields(pspSrchParser)) != SRCH_NoError ) {
        return (iError);
    }


    /* Set the search parser defaults */
    if ( (iError = iSrchParserSetDefaults(pspSrchParser)) != SRCH_NoError ) {
        return (iError);
    }


    return (SRCH_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchParserFree()

    Purpose:    This function frees the search parser. 

    Parameters: pvSrchParser    search parser structure

    Globals:    none

    Returns:    SRCH error code

*/
int iSrchParserFree
(
    void *pvSrchParser
)
{

    int                 iError = SRCH_NoError;
    struct srchParser   *pspSrchParser = (struct srchParser *)pvSrchParser;


    /* Check the parameters */
    if ( pvSrchParser == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvSrchParser' parameter passed to 'iSrchParserFree'."); 
        return (SRCH_ParserInvalidParser);
    }


    /* Free the the search parser structure fields */
    if ( (iError = iSrchParserFreeStructureFields(pspSrchParser)) != SRCH_NoError ) {
        return (iError);
    }

    /* Free the search parser structure */
    s_free(pspSrchParser);


    return (SRCH_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchParserGetErrorText()

    Purpose:    This function return there error text for a 
                given SRCH parser error.

    Parameters: iError              error
                ppucErrorText       return pointer for the error text
                uiErrorTextLength   length of the return pointer

    Globals:    none

    Returns:    SRCH error code

*/
int iSrchParserGetErrorText
(
    int iError, 
    unsigned char *pucErrorText,
    unsigned int uiErrorTextLength
)
{

    struct srchParserErrorMapping   *pspemSrchParserMappingsPtr = NULL;


    /* Check input variables */
    if ( (iError <= SPI_NoError) && (pucErrorText != NULL) && (uiErrorTextLength > 0) ) {
    
        /* Loop over all the errors in the error mapping */
        for ( pspemSrchParserMappingsPtr = pspemSrchParserMappingsGlobal; pspemSrchParserMappingsPtr->iError != SRCH_PARSER_MAPPING_END_MARKER; pspemSrchParserMappingsPtr++ ) {
            
            /* Copy the error text to the return variable if the error matches */
            if ( pspemSrchParserMappingsPtr->iError == iError ) {
                s_strnncpy(pucErrorText, pspemSrchParserMappingsPtr->pucErrorText, uiErrorTextLength);
                return (SRCH_NoError);
            }
        }
    }

    return (SRCH_MiscError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchParserGetModifierID()

    Purpose:    This function returns the modifier ID for the specified modifier class ID.

    Parameters: pvSrchParser        search parser structure
                uiModifierClassID   modifier class ID
                puiModifierID       return pointer for the modifier ID

    Globals:    none

    Returns:    SRCH error code

*/
int iSrchParserGetModifierID
(
    void *pvSrchParser,
    unsigned int uiModifierClassID,
    unsigned int *puiModifierID
)
{

    struct srchParser   *pspSrchParser = (struct srchParser *)pvSrchParser;


    /* Check the parameters */
    if ( pvSrchParser == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvSrchParser' parameter passed to 'iSrchParserGetModifierID'."); 
        return (SRCH_ParserInvalidParser);
    }

    if ( puiModifierID == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'puiModifierID' parameter passed to 'iSrchParserGetModifierID'."); 
        return (SRCH_ReturnParameterError);
    }


    /* Set the return pointer */
    if ( uiModifierClassID == SRCH_PARSER_MODIFIER_SEARCH_RESULTS_ID ) {
        *puiModifierID = pspSrchParser->uiSearchResultsID;
    }
    else if ( uiModifierClassID == SRCH_PARSER_MODIFIER_SEARCH_REPORT_ID ) {
        *puiModifierID = pspSrchParser->uiSearchReportID;
    }
    else if ( uiModifierClassID == SRCH_PARSER_MODIFIER_SEARCH_CACHE_ID ) {
        *puiModifierID = pspSrchParser->uiSearchCacheID;
    }
    else if ( uiModifierClassID == SRCH_PARSER_MODIFIER_DEBUG_ID ) {
        *puiModifierID = pspSrchParser->uiDebugID;
    }
    else if ( uiModifierClassID == SRCH_PARSER_MODIFIER_BOOLEAN_OPERATOR_ID ) {
        *puiModifierID = pspSrchParser->uiBooleanOperatorID;
    }
    else if ( uiModifierClassID == SRCH_PARSER_MODIFIER_BOOLEAN_OPERATION_ID ) {
        *puiModifierID = pspSrchParser->uiBooleanOperationID;
    }
    else if ( uiModifierClassID == SRCH_PARSER_MODIFIER_OPERATOR_CASE_ID ) {
        *puiModifierID = pspSrchParser->uiOperatorCaseID;
    }
    else if ( uiModifierClassID == SRCH_PARSER_MODIFIER_TERM_CASE_ID ) {
        *puiModifierID = pspSrchParser->uiTermCaseID;
    }
    else if ( uiModifierClassID == SRCH_PARSER_MODIFIER_FREQUENT_TERMS_ID ) {
        *puiModifierID = pspSrchParser->uiFrequentTermsID;
    }
    else if ( uiModifierClassID == SRCH_PARSER_MODIFIER_SEARCH_TYPE_ID ) {
        *puiModifierID = pspSrchParser->uiSearchTypeID;
    }
    else if ( uiModifierClassID == SRCH_PARSER_MODIFIER_EARLY_COMPLETION_ID ) {
        *puiModifierID = pspSrchParser->uiEarlyCompletionID;
    }
    else {
        return (SRCH_ParserInvalidModifierClassID);
    }


    return (SRCH_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchParserGetModifierValue()

    Purpose:    This function returns the modifier value for the specified modifier ID.

    Parameters: pvSrchParser        search parser structure
                uiModifierID        modifier ID
                pvModifierValue     return pointer for the modifier value

    Globals:    none

    Returns:    SRCH error code

*/
int iSrchParserGetModifierValue
(
    void *pvSrchParser,
    unsigned int uiModifierID,
    void *pvModifierValue
)
{

    struct srchParser   *pspSrchParser = (struct srchParser *)pvSrchParser;


    /* Check the parameters */
    if ( pvSrchParser == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvSrchParser' parameter passed to 'iSrchParserGetModifierValue'."); 
        return (SRCH_ParserInvalidParser);
    }

    if ( pvModifierValue == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvModifierValue' parameter passed to 'iSrchParserGetModifierID'."); 
        return (SRCH_ReturnParameterError);
    }


    /* Set the return pointer */
    if ( uiModifierID == SRCH_PARSER_MODIFIER_TERM_WEIGHT_ID ) {
        
        float   *pfTermWeight = (float *)pvModifierValue;

        *pfTermWeight = pspSrchParser->fTermWeight;
    }

    else if ( uiModifierID == SRCH_PARSER_MODIFIER_FEEDBACK_TERM_WEIGHT_ID ) {
        
        float   *pfFeedbackTermWeight = (float *)pvModifierValue;

        *pfFeedbackTermWeight = pspSrchParser->fFeedbackTermWeight;
    }

    else if ( uiModifierID == SRCH_PARSER_MODIFIER_FREQUENT_TERM_COVERAGE_THRESHOLD_ID ) {
        
        float   *pfFrequentTermCoverageThreshold = (float *)pvModifierValue;

        *pfFrequentTermCoverageThreshold = pspSrchParser->fFrequentTermCoverageThreshold;
    }

    else if ( uiModifierID == SRCH_PARSER_MODIFIER_FEEDBACK_MINIMUM_TERM_COUNT_ID ) {
        
        unsigned int    *puiFeedbackMinimumTermCount = (unsigned int *)pvModifierValue;

        *puiFeedbackMinimumTermCount = pspSrchParser->uiFeedbackMinimumTermCount;
    }

    else if ( uiModifierID == SRCH_PARSER_MODIFIER_FEEDBACK_MAXIMUM_TERM_PERCENTAGE_ID ) {
        
        float   *pfFeedbackMaximumTermPercentage = (float *)pvModifierValue;

        *pfFeedbackMaximumTermPercentage = pspSrchParser->fFeedbackMaximumTermPercentage;
    }

    else if ( uiModifierID == SRCH_PARSER_MODIFIER_FEEDBACK_MAXIMUM_TERM_COVERAGE_THRESHOLD_ID ) {
        
        float   *pfFeedbackMaximumTermCoverageThreshold = (float *)pvModifierValue;

        *pfFeedbackMaximumTermCoverageThreshold = pspSrchParser->fFeedbackMaximumTermCoverageThreshold;
    }

    else if ( uiModifierID == SRCH_PARSER_MODIFIER_CONNECTION_TIMEOUT_ID ) {
        
        unsigned int    *puiConnectionTimeout = (unsigned int *)pvModifierValue;

        *puiConnectionTimeout = pspSrchParser->uiConnectionTimeout;
    }

    else if ( uiModifierID == SRCH_PARSER_MODIFIER_SEARCH_TIMEOUT_ID ) {
        
        unsigned int    *puiSearchTimeout = (unsigned int *)pvModifierValue;

        *puiSearchTimeout = pspSrchParser->uiSearchTimeout;
    }

    else if ( uiModifierID == SRCH_PARSER_MODIFIER_RETRIEVAL_TIMEOUT_ID ) {
        
        unsigned int    *puiRetrievalTimeout = (unsigned int *)pvModifierValue;

        *puiRetrievalTimeout = pspSrchParser->uiRetrievalTimeout;
    }

    else if ( uiModifierID == SRCH_PARSER_MODIFIER_INFORMATION_TIMEOUT_ID ) {
        
        unsigned int    *puiInformationTimeout = (unsigned int *)pvModifierValue;

        *puiInformationTimeout = pspSrchParser->uiInformationTimeout;
    }

    else if ( uiModifierID == SRCH_PARSER_MODIFIER_MAXIMUM_SEGMENTS_SEARCHED_ID ) {
        
        unsigned int    *puiMaximumSegmentsSearched = (unsigned int *)pvModifierValue;

        *puiMaximumSegmentsSearched = pspSrchParser->uiMaximumSegmentsSearched;
    }

    else if ( uiModifierID == SRCH_PARSER_MODIFIER_MINIMUM_SEGMENTS_SEARCHED_ID ) {
        
        unsigned int    *puiMinimumSegmentsSearched = (unsigned int *)pvModifierValue;

        *puiMinimumSegmentsSearched = pspSrchParser->uiMinimumSegmentsSearched;
    }

    else {
        return (SRCH_ParserInvalidModifierID);
    }


    return (SRCH_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchParserGetDates()

    Purpose:    This function returns the date restrictions if they were
                specified.

    Parameters: pvSrchParser                        search parser structure
                ppspnSrchParserNumberDates          return pointer for the search parser number dates (do not free)
                puiSrchParserNumberDatesLength      return pointer for the number of entries in the search parser number dates

    Globals:    none

    Returns:    SRCH error code

*/
int iSrchParserGetDates
(
    void *pvSrchParser,
    struct srchParserNumber **ppspnSrchParserNumberDates,
    unsigned int *puiSrchParserNumberDatesLength
)
{

    struct srchParser   *pspSrchParser = (struct srchParser *)pvSrchParser;


    /* Check the parameters */
    if ( pvSrchParser == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvSrchParser' parameter passed to 'iSrchParserGetDates'."); 
        return (SRCH_ParserInvalidParser);
    }

    if ( ppspnSrchParserNumberDates == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppspnSrchParserNumberDates' parameter passed to 'iSrchParserGetDates'."); 
        return (SRCH_ReturnParameterError);
    }

    if ( puiSrchParserNumberDatesLength == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'puiSrchParserNumberDatesLength' parameter passed to 'iSrchParserGetDates'."); 
        return (SRCH_ReturnParameterError);
    }


    /* Set the return pointers */
    *ppspnSrchParserNumberDates = pspSrchParser->pspnSrchParserNumberDates;
    *puiSrchParserNumberDatesLength = pspSrchParser->uiSrchParserNumberDatesLength;


    return (SRCH_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchParserGetUnfieldedSearchFieldNames()

    Purpose:    This function returns the unfielded field names if they were
                specified.

    Parameters: pvSrchParser                    search parser structure
                ppwcUnfieldedSearchFieldNames   return pointer for the unfielded field names (do not free)

    Globals:    none

    Returns:    SRCH error code

*/
int iSrchParserGetUnfieldedSearchFieldNames
(
    void *pvSrchParser,
    wchar_t **ppwcUnfieldedSearchFieldNames
)
{

    struct srchParser   *pspSrchParser = (struct srchParser *)pvSrchParser;


    /* Check the parameters */
    if ( pvSrchParser == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvSrchParser' parameter passed to 'iSrchParserGetDates'."); 
        return (SRCH_ParserInvalidParser);
    }

    if ( ppwcUnfieldedSearchFieldNames == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppwcUnfieldedSearchFieldNames' parameter passed to 'iSrchParserGetDates'."); 
        return (SRCH_ReturnParameterError);
    }


    /* Set the return pointers */
    *ppwcUnfieldedSearchFieldNames = pspSrchParser->pwcUnfieldedSearchFieldNames;


    return (SRCH_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchParserGetSort()

    Purpose:    This function returns the sort order information. The sort 
                can be set in the search, failing that it will be set by
                the server configuration file, failing that it will be set to
                the built-in default.

    Parameters: pvSrchParser        search parser structure
                ppwcSortFieldName   return pointer for the sort field name
                puiSortOrderID      return pointer for the sort order ID

    Globals:    none

    Returns:    SRCH error code

*/
int iSrchParserGetSort
(
    void *pvSrchParser,
    wchar_t **ppwcSortFieldName,
    unsigned int *puiSortOrderID
)
{

    struct srchParser   *pspSrchParser = (struct srchParser *)pvSrchParser;


    /* Check the parameters */
    if ( pvSrchParser == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvSrchParser' parameter passed to 'iSrchParserGetSort'."); 
        return (SRCH_ParserInvalidParser);
    }

    if ( ppwcSortFieldName == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppwcSortFieldName' parameter passed to 'iSrchParserGetSort'."); 
        return (SRCH_ReturnParameterError);
    }

    if ( puiSortOrderID == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'puiSortOrderID' parameter passed to 'iSrchParserGetSort'."); 
        return (SRCH_ReturnParameterError);
    }


    /* Set the case of the sort field name */
/*     if ( pspSrchParser->pwcSortFieldName != NULL ) { */
/*         if ( pspSrchParser->uiOperatorCaseID == SRCH_PARSER_MODIFIER_OPERATOR_CASE_UPPER_ID ) { */
/*             pwcLngCaseConvertWideStringToUpperCase(pspSrchParser->pwcSortFieldName); */
/*         } */
/*         else if ( pspSrchParser->uiOperatorCaseID == SRCH_PARSER_MODIFIER_OPERATOR_CASE_LOWER_ID ) { */
/*             pwcLngCaseConvertWideStringToLowerCase(pspSrchParser->pwcSortFieldName); */
/*         } */
/*     } */


    /* Set the return pointers */
    *ppwcSortFieldName = pspSrchParser->pwcSortFieldName;
    *puiSortOrderID = pspSrchParser->uiSortOrderID;


    return (SRCH_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchParserGetExclusionFilters()

    Purpose:    This function returns the exclusion filters.

    Parameters: pvSrchParser                                search parser structure
                ppspfSrchParserFilterExclusionFilters       return pointer for the parser filter exclusion filters (do no free)
                puiSrchParserFilterExclusionFiltersLength   return pointer for the parser filter exclusion filters length

    Globals:    none

    Returns:    SRCH error code

*/
int iSrchParserGetExclusionFilters
(
    void *pvSrchParser,
    struct srchParserFilter **ppspfSrchParserFilterExclusionFilters, 
    unsigned int *puiSrchParserFilterExclusionFiltersLength
)
{

    struct srchParser   *pspSrchParser = (struct srchParser *)pvSrchParser;


    /* Check the parameters */
    if ( pvSrchParser == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvSrchParser' parameter passed to 'iSrchParserGetExclusionFilters'."); 
        return (SRCH_ParserInvalidParser);
    }

    if ( ppspfSrchParserFilterExclusionFilters == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppspfSrchParserFilterExclusionFilters' parameter passed to 'iSrchParserGetExclusionFilters'."); 
        return (SRCH_ReturnParameterError);
    }

    if ( puiSrchParserFilterExclusionFiltersLength == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'puiSrchParserFilterExclusionFiltersLength' parameter passed to 'iSrchParserGetExclusionFilters'."); 
        return (SRCH_ReturnParameterError);
    }


    /* Set the return pointers */
    *ppspfSrchParserFilterExclusionFilters = pspSrchParser->pspfSrchParserFilterExclusionFilters;
    *puiSrchParserFilterExclusionFiltersLength = pspSrchParser->uiSrchParserFilterExclusionFiltersLength;


    return (SRCH_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchParserGetInclusionFilters()

    Purpose:    This function returns the inclusion filters.

    Parameters: pvSrchParser                                search parser structure
                ppspfSrchParserFilterInclusionFilters       return pointer for the parser filter inclusion filters (do no free)
                puiSrchParserFilterInclusionFiltersLength   return pointer for the parser filter inclusion filters length

    Globals:    none

    Returns:    SRCH error code

*/
int iSrchParserGetInclusionFilters
(
    void *pvSrchParser,
    struct srchParserFilter **ppspfSrchParserFilterInclusionFilters, 
    unsigned int *puiSrchParserFilterInclusionFiltersLength
)
{

    struct srchParser   *pspSrchParser = (struct srchParser *)pvSrchParser;


    /* Check the parameters */
    if ( pvSrchParser == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvSrchParser' parameter passed to 'ppspfSrchParserFilterInclusionFilters'."); 
        return (SRCH_ParserInvalidParser);
    }

    if ( ppspfSrchParserFilterInclusionFilters == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppspfSrchParserFilterInclusionFilters' parameter passed to 'ppspfSrchParserFilterInclusionFilters'."); 
        return (SRCH_ReturnParameterError);
    }

    if ( puiSrchParserFilterInclusionFiltersLength == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'puiSrchParserFilterInclusionFiltersLength' parameter passed to 'ppspfSrchParserFilterInclusionFilters'."); 
        return (SRCH_ReturnParameterError);
    }


    /* Set the return pointers */
    *ppspfSrchParserFilterInclusionFilters = pspSrchParser->pspfSrchParserFilterInclusionFilters;
    *puiSrchParserFilterInclusionFiltersLength = pspSrchParser->uiSrchParserFilterInclusionFiltersLength;


    return (SRCH_NoError);

}

/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchParserGetLanguage()

    Purpose:    This function returns the language IDs.

    Parameters: pvSrchParser                            search parser structure
                ppspnSrchParserNumberLanguageIDs        return pointer for the search number language IDs (do no free)
                puiSrchParserNumberLanguageIDsLength    return pointer for the search number language IDs length

    Globals:    none

    Returns:    SRCH error code

*/
int iSrchParserGetLanguage
(
    void *pvSrchParser,
    struct srchParserNumber **ppspnSrchParserNumberLanguageIDs, 
    unsigned int *puiSrchParserNumberLanguageIDsLength
)
{

    struct srchParser   *pspSrchParser = (struct srchParser *)pvSrchParser;


    /* Check the parameters */
    if ( pvSrchParser == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvSrchParser' parameter passed to 'iSrchParserGetLanguage'."); 
        return (SRCH_ParserInvalidParser);
    }

    if ( ppspnSrchParserNumberLanguageIDs == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppspnSrchParserNumberLanguageIDs' parameter passed to 'iSrchParserGetLanguage'."); 
        return (SRCH_ReturnParameterError);
    }

    if ( puiSrchParserNumberLanguageIDsLength == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'puiSrchParserNumberLanguageIDsLength' parameter passed to 'iSrchParserGetLanguage'."); 
        return (SRCH_ReturnParameterError);
    }


    /* Set the return pointer */
    *ppspnSrchParserNumberLanguageIDs = pspSrchParser->pspnSrchParserNumberLanguageIDs;
    *puiSrchParserNumberLanguageIDsLength = pspSrchParser->uiSrchParserNumberLanguageIDsLength;


    return (SRCH_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchParserGetSearchTermCount()

    Purpose:    This function returns the search term count.

    Parameters: pvSrchParser            search parser structure
                puiSearchTermCount      return pointer for the search term count

    Globals:    none

    Returns:    SRCH error code

*/
int iSrchParserGetSearchTermCount
(
    void *pvSrchParser,
    unsigned int *puiSearchTermCount
)
{

    struct srchParser   *pspSrchParser = (struct srchParser *)pvSrchParser;


    /* Check the parameters */
    if ( pvSrchParser == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvSrchParser' parameter passed to 'iSrchParserGetSearchTermCount'."); 
        return (SRCH_ParserInvalidParser);
    }

    if ( puiSearchTermCount == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'puiSearchTermCount' parameter passed to 'iSrchParserGetSearchTermCount'."); 
        return (SRCH_ReturnParameterError);
    }


    /* Set the return pointer */
    *puiSearchTermCount = pspSrchParser->uiSearchTermCount;


    return (SRCH_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchParserGetNormalizedSearchText()

    Purpose:    This function returns a pointer to the normalized search text.

    Parameters: pvSrchParser                search parser structure
                ppwcNormalizedSearchText    return pointer for the normalized search text (do not free)

    Globals:    none

    Returns:    SRCH error code

*/
int iSrchParserGetNormalizedSearchText
(
    void *pvSrchParser,
    wchar_t **ppwcNormalizedSearchText
)
{

    int                 iError = SRCH_NoError;
    struct srchParser   *pspSrchParser = (struct srchParser *)pvSrchParser;
        

    /* Check the parameters */
    if ( pvSrchParser == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvSrchParser' parameter passed to 'iSrchParserGetNormalizedSearchText'."); 
        return (SRCH_ParserInvalidParser);
    }

    if ( ppwcNormalizedSearchText == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppwcNormalizedSearchText' parameter passed to 'iSrchParserGetNormalizedSearchText'."); 
        return (SRCH_ReturnParameterError);
    }


    /* Create a normalized search from the term cluster if it exists, and the normalize search text has not yet been created */
    if ( (pspSrchParser->psptcSrchParserTermCluster != NULL) && (pspSrchParser->pwcNormalizedSearchText == NULL) ) {
        
        void    *pvUtlNormalizedSearchWideStringBuffer = NULL;

        /* Allocate the normalized search string buffer */
        if ( (iError = iUtlWideStringBufferCreate(&pvUtlNormalizedSearchWideStringBuffer)) != UTL_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to create a wide string buffer, utl error: %d.", iError); 
            return (SRCH_MemError);
        }    

        /* Create the normalized search text */
        if ( (iError = iSrchParserTermClusterToNormalizedSearchText(pspSrchParser, pspSrchParser->psptcSrchParserTermCluster, pvUtlNormalizedSearchWideStringBuffer)) != SRCH_NoError ) {
            iUtlWideStringBufferFree(pvUtlNormalizedSearchWideStringBuffer, true);
            pvUtlNormalizedSearchWideStringBuffer = NULL;
            return (iError);
        }

        /* Get the string from the buffer */
        iUtlWideStringBufferGetString(pvUtlNormalizedSearchWideStringBuffer, &pspSrchParser->pwcNormalizedSearchText);

        /* Free the normalized search string buffer, note that we don't release the string */
        iUtlWideStringBufferFree(pvUtlNormalizedSearchWideStringBuffer, false);
        pvUtlNormalizedSearchWideStringBuffer = NULL;
    }
    
    /* Set the return pointer if the normalize search string buffer exists */
    *ppwcNormalizedSearchText = pspSrchParser->pwcNormalizedSearchText;


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchParserGetFullNormalizedSearchText()

    Purpose:    This function returns a pointer to the full normalized search text.

    Parameters: pvSrchParser                    search parser structure
                ppwcFullNormalizedSearchText    return pointer for the full normalized search text (do not free)

    Globals:    none

    Returns:    SRCH error code

*/
int iSrchParserGetFullNormalizedSearchText
(
    void *pvSrchParser,
    wchar_t **ppwcFullNormalizedSearchText
)
{

    int                         iError = SRCH_NoError;
    struct srchParser           *pspSrchParser = (struct srchParser *)pvSrchParser;
    void                        *pvUtlFullNormalizedSearchWideStringBuffer = NULL;
    unsigned int                uiI = 0;
    wchar_t                     pwcBuffer[SRCH_PARSER_MINIMUM_LENGTH + 1] = {L'\0'};
    unsigned char               pucBuffer[SRCH_PARSER_MINIMUM_LENGTH + 1] = {L'\0'};
    struct srchParserNumber     *pspnSrchParserNumberPtr = NULL;
    struct srchParserFilter     *pspfSrchParserFilterPtr = NULL;
        

    /* Check the parameters */
    if ( pvSrchParser == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvSrchParser' parameter passed to 'iSrchParserGetFullNormalizedSearchText'."); 
        return (SRCH_ParserInvalidParser);
    }

    if ( ppwcFullNormalizedSearchText == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppwcFullNormalizedSearchText' parameter passed to 'iSrchParserGetFullNormalizedSearchText'."); 
        return (SRCH_ReturnParameterError);
    }


    /* Return the full normalized search text here if it has already been created */
    if ( pspSrchParser->pwcFullNormalizedSearchText != NULL    ) {
        *ppwcFullNormalizedSearchText = pspSrchParser->pwcFullNormalizedSearchText;
        return (SRCH_NoError);
    }
    

    /* Allocate the full normalized search string buffer */
    if ( (iError = iUtlWideStringBufferCreate(&pvUtlFullNormalizedSearchWideStringBuffer)) != UTL_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to create a wide string buffer, utl error: %d.", iError); 
        iError = SRCH_MemError;
        goto bailFromiSrchParserGetFullNormalizedSearchText;
    }    

    /* Create the normalized search text */
    if ( pspSrchParser->psptcSrchParserTermCluster != NULL ) {
        if ( (iError = iSrchParserTermClusterToNormalizedSearchText(pspSrchParser, pspSrchParser->psptcSrchParserTermCluster, pvUtlFullNormalizedSearchWideStringBuffer)) != SRCH_NoError ) {
            goto bailFromiSrchParserGetFullNormalizedSearchText;
        }
    }
    
    /* Append the unfielded search field names */
    if ( pspSrchParser->pwcUnfieldedSearchFieldNames != NULL ) {
        iUtlWideStringBufferAppend(pvUtlFullNormalizedSearchWideStringBuffer, L" ");
        iUtlWideStringBufferAppend(pvUtlFullNormalizedSearchWideStringBuffer, SRCH_PARSER_MODIFIER_UNFIELDED_SEARCH_FIELD_NAMES_WSTRING);
        iUtlWideStringBufferAppend(pvUtlFullNormalizedSearchWideStringBuffer, SRCH_PARSER_MODIFIER_SEPARATOR_WSTRING);
        iUtlWideStringBufferAppend(pvUtlFullNormalizedSearchWideStringBuffer, pspSrchParser->pwcUnfieldedSearchFieldNames);
        iUtlWideStringBufferAppend(pvUtlFullNormalizedSearchWideStringBuffer, SRCH_PARSER_CHARACTER_RBRACKET_WSTRING);
    }

    /* Append the sort order */
    iUtlWideStringBufferAppend(pvUtlFullNormalizedSearchWideStringBuffer, L" ");
    if ( pspSrchParser->uiSortOrderID == SRCH_PARSER_SORT_ORDER_DEFAULT_ID ) {
        iUtlWideStringBufferAppend(pvUtlFullNormalizedSearchWideStringBuffer, SRCH_PARSER_MODIFIER_SORT_DEFAULT_WSTRING);
    }
    else if ( pspSrchParser->uiSortOrderID == SRCH_PARSER_SORT_ORDER_NONE_ID ) {
        iUtlWideStringBufferAppend(pvUtlFullNormalizedSearchWideStringBuffer, SRCH_PARSER_MODIFIER_SORT_NONE_WSTRING);
    }
    else if ( (pspSrchParser->uiSortOrderID == SRCH_PARSER_SORT_ORDER_ASC_ID) || (pspSrchParser->uiSortOrderID == SRCH_PARSER_SORT_ORDER_DESC_ID) ) {
        
        iUtlWideStringBufferAppend(pvUtlFullNormalizedSearchWideStringBuffer, SRCH_PARSER_MODIFIER_SORT_WSTRING);
        
        iUtlWideStringBufferAppend(pvUtlFullNormalizedSearchWideStringBuffer, SRCH_PARSER_MODIFIER_SEPARATOR_WSTRING);
        iUtlWideStringBufferAppend(pvUtlFullNormalizedSearchWideStringBuffer, pspSrchParser->pwcSortFieldName);
        iUtlWideStringBufferAppend(pvUtlFullNormalizedSearchWideStringBuffer, SRCH_PARSER_MODIFIER_SEPARATOR_WSTRING);
        
        if ( pspSrchParser->uiSortOrderID == SRCH_PARSER_SORT_ORDER_ASC_ID ) {
            iUtlWideStringBufferAppend(pvUtlFullNormalizedSearchWideStringBuffer, SRCH_PARSER_SORT_ORDER_ASC_WSTRING);
        }
        else if ( pspSrchParser->uiSortOrderID == SRCH_PARSER_SORT_ORDER_DESC_ID ) {
            iUtlWideStringBufferAppend(pvUtlFullNormalizedSearchWideStringBuffer, SRCH_PARSER_SORT_ORDER_DESC_WSTRING);
        }

        iUtlWideStringBufferAppend(pvUtlFullNormalizedSearchWideStringBuffer, SRCH_PARSER_CHARACTER_RBRACKET_WSTRING);
    }
    
    /* Append the date restrictions */
    if ( pspSrchParser->pspnSrchParserNumberDates != NULL ) {

        iUtlWideStringBufferAppend(pvUtlFullNormalizedSearchWideStringBuffer, L" ");
        iUtlWideStringBufferAppend(pvUtlFullNormalizedSearchWideStringBuffer, SRCH_PARSER_MODIFIER_DATE_WSTRING);

        for ( uiI = 0, pspnSrchParserNumberPtr = pspSrchParser->pspnSrchParserNumberDates; uiI < pspSrchParser->uiSrchParserNumberDatesLength; uiI++, pspnSrchParserNumberPtr++ ) {
            
            wchar_t     pwcNumber[SRCH_PARSER_MINIMUM_LENGTH + 1] = {L'\0'};
            
            iUtlWideStringBufferAppend(pvUtlFullNormalizedSearchWideStringBuffer, pwcSrchParserGetStringFromID(pvSrchParser, pspnSrchParserNumberPtr->uiRangeID));
            
            swprintf(pwcNumber, SRCH_PARSER_MINIMUM_LENGTH + 1, L"%lu", pspnSrchParserNumberPtr->ulNumber);
            swprintf(pwcBuffer, SRCH_PARSER_MINIMUM_LENGTH + 1, L"%c%c%c%c/%c%c/%c%c-%c%c:%c%c:%c%c", 
                    pwcNumber[0], pwcNumber[1], pwcNumber[2], pwcNumber[3],     /* year */
                    pwcNumber[4], pwcNumber[5],                                 /* month */
                    pwcNumber[6], pwcNumber[7],                                 /* day */
                    pwcNumber[8], pwcNumber[9],                                 /* hour */
                    pwcNumber[10], pwcNumber[11],                               /* minute */
                    pwcNumber[12], pwcNumber[13]);                              /* second */
            
            iUtlWideStringBufferAppend(pvUtlFullNormalizedSearchWideStringBuffer, pwcBuffer);
            
            if ( uiI < (pspSrchParser->uiSrchParserNumberDatesLength - 1) ) {
                iUtlWideStringBufferAppend(pvUtlFullNormalizedSearchWideStringBuffer, SRCH_PARSER_CHARACTER_COMMA_WSTRING);
            }
        }

        iUtlWideStringBufferAppend(pvUtlFullNormalizedSearchWideStringBuffer, SRCH_PARSER_CHARACTER_RBRACKET_WSTRING);
    }

    /* Append the languages */
    if ( pspSrchParser->pspnSrchParserNumberLanguageIDs != NULL ) {
    
        iUtlWideStringBufferAppend(pvUtlFullNormalizedSearchWideStringBuffer, L" ");
        iUtlWideStringBufferAppend(pvUtlFullNormalizedSearchWideStringBuffer, SRCH_PARSER_MODIFIER_LANGUAGE_WSTRING);
        iUtlWideStringBufferAppend(pvUtlFullNormalizedSearchWideStringBuffer, SRCH_PARSER_MODIFIER_SEPARATOR_WSTRING);

        for ( uiI = 0, pspnSrchParserNumberPtr = pspSrchParser->pspnSrchParserNumberLanguageIDs; uiI < pspSrchParser->uiSrchParserNumberLanguageIDsLength; uiI++, pspnSrchParserNumberPtr++ ) {

            /* Get the language code */
            if ( iLngGetLanguageCodeFromID(pspnSrchParserNumberPtr->ulNumber, pucBuffer, SRCH_PARSER_MINIMUM_LENGTH + 1) != LNG_NoError ) {
                iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the language code from the language ID: %lu, lng error: %d.", pspnSrchParserNumberPtr->ulNumber, iError); 
                iError = SRCH_MiscError;
                goto bailFromiSrchParserGetFullNormalizedSearchText;
            }
            
            /* Convert the language to utf-8 */
            if ( iLngConvertUtf8ToWideString_s(pucBuffer, s_strlen(pucBuffer), pwcBuffer, SRCH_PARSER_MINIMUM_LENGTH + 1) != LNG_NoError ) {
                iUtlLogError(UTL_LOG_CONTEXT, "Failed to convert the language from utf-8 to wide character, lng error: %d.", iError); 
                iError = SRCH_MiscError;
                goto bailFromiSrchParserGetFullNormalizedSearchText;
            }

            iUtlWideStringBufferAppend(pvUtlFullNormalizedSearchWideStringBuffer, pwcBuffer);

            if ( uiI < (pspSrchParser->uiSrchParserNumberLanguageIDsLength - 1) ) {
                iUtlWideStringBufferAppend(pvUtlFullNormalizedSearchWideStringBuffer, SRCH_PARSER_CHARACTER_COMMA_WSTRING);
            }
        }
        
        iUtlWideStringBufferAppend(pvUtlFullNormalizedSearchWideStringBuffer, SRCH_PARSER_CHARACTER_RBRACKET_WSTRING);
    }

    /* Append the exclusion & exclusion list filters */
    if ( pspSrchParser->pspfSrchParserFilterExclusionFilters != NULL ) {
        for ( uiI = 0, pspfSrchParserFilterPtr = pspSrchParser->pspfSrchParserFilterExclusionFilters; uiI < pspSrchParser->uiSrchParserFilterExclusionFiltersLength; uiI++, pspfSrchParserFilterPtr++ ) {
            iUtlWideStringBufferAppend(pvUtlFullNormalizedSearchWideStringBuffer, L" ");
            if ( pspfSrchParserFilterPtr->uiFilterTypeID == SRCH_PARSER_FILTER_TYPE_TERMS_ID ) {
                iUtlWideStringBufferAppend(pvUtlFullNormalizedSearchWideStringBuffer, SRCH_PARSER_MODIFIER_EXCLUSION_FILTER_WSTRING);
            }
            else if ( pspfSrchParserFilterPtr->uiFilterTypeID == SRCH_PARSER_FILTER_TYPE_LIST_ID ) {
                iUtlWideStringBufferAppend(pvUtlFullNormalizedSearchWideStringBuffer, SRCH_PARSER_MODIFIER_EXCLUSION_LIST_FILTER_WSTRING);
            }
            iUtlWideStringBufferAppend(pvUtlFullNormalizedSearchWideStringBuffer, SRCH_PARSER_MODIFIER_SEPARATOR_WSTRING);
            iUtlWideStringBufferAppend(pvUtlFullNormalizedSearchWideStringBuffer, pspfSrchParserFilterPtr->pwcFilter);
            iUtlWideStringBufferAppend(pvUtlFullNormalizedSearchWideStringBuffer, SRCH_PARSER_CHARACTER_RBRACKET_WSTRING);
        }
    }

    /* Append the inclusion & inclusion list filters */
    if ( pspSrchParser->pspfSrchParserFilterInclusionFilters != NULL ) {
        for ( uiI = 0, pspfSrchParserFilterPtr = pspSrchParser->pspfSrchParserFilterInclusionFilters; uiI < pspSrchParser->uiSrchParserFilterInclusionFiltersLength; uiI++, pspfSrchParserFilterPtr++ ) {
            iUtlWideStringBufferAppend(pvUtlFullNormalizedSearchWideStringBuffer, L" ");
            if ( pspfSrchParserFilterPtr->uiFilterTypeID == SRCH_PARSER_FILTER_TYPE_TERMS_ID ) {
                iUtlWideStringBufferAppend(pvUtlFullNormalizedSearchWideStringBuffer, SRCH_PARSER_MODIFIER_INCLUSION_FILTER_WSTRING);
            }
            else if ( pspfSrchParserFilterPtr->uiFilterTypeID == SRCH_PARSER_FILTER_TYPE_LIST_ID ) {
                iUtlWideStringBufferAppend(pvUtlFullNormalizedSearchWideStringBuffer, SRCH_PARSER_MODIFIER_INCLUSION_LIST_FILTER_WSTRING);
            }
            iUtlWideStringBufferAppend(pvUtlFullNormalizedSearchWideStringBuffer, SRCH_PARSER_MODIFIER_SEPARATOR_WSTRING);
            iUtlWideStringBufferAppend(pvUtlFullNormalizedSearchWideStringBuffer, pspfSrchParserFilterPtr->pwcFilter);
            iUtlWideStringBufferAppend(pvUtlFullNormalizedSearchWideStringBuffer, SRCH_PARSER_CHARACTER_RBRACKET_WSTRING);
        }
    }

    /* Append the search results modifier */
    iUtlWideStringBufferAppend(pvUtlFullNormalizedSearchWideStringBuffer, L" ");
    iUtlWideStringBufferAppend(pvUtlFullNormalizedSearchWideStringBuffer, pwcSrchParserGetStringFromID(pvSrchParser, pspSrchParser->uiSearchResultsID));
    
    /* Append the search report modifier */
    iUtlWideStringBufferAppend(pvUtlFullNormalizedSearchWideStringBuffer, L" ");
    iUtlWideStringBufferAppend(pvUtlFullNormalizedSearchWideStringBuffer, pwcSrchParserGetStringFromID(pvSrchParser, pspSrchParser->uiSearchReportID));
    
    /* Append the search cache modifier */
    iUtlWideStringBufferAppend(pvUtlFullNormalizedSearchWideStringBuffer, L" ");
    iUtlWideStringBufferAppend(pvUtlFullNormalizedSearchWideStringBuffer, pwcSrchParserGetStringFromID(pvSrchParser, pspSrchParser->uiSearchCacheID));
    
    /* Append the debug modifier */
    iUtlWideStringBufferAppend(pvUtlFullNormalizedSearchWideStringBuffer, L" ");
    iUtlWideStringBufferAppend(pvUtlFullNormalizedSearchWideStringBuffer, pwcSrchParserGetStringFromID(pvSrchParser, pspSrchParser->uiDebugID));
    
    /* Append the early completion modifier */
    iUtlWideStringBufferAppend(pvUtlFullNormalizedSearchWideStringBuffer, L" ");
    iUtlWideStringBufferAppend(pvUtlFullNormalizedSearchWideStringBuffer, pwcSrchParserGetStringFromID(pvSrchParser, pspSrchParser->uiEarlyCompletionID));
    
    /* Append the boolean operator modifier */
    iUtlWideStringBufferAppend(pvUtlFullNormalizedSearchWideStringBuffer, L" ");
    iUtlWideStringBufferAppend(pvUtlFullNormalizedSearchWideStringBuffer, pwcSrchParserGetStringFromID(pvSrchParser, pspSrchParser->uiBooleanOperatorID));

    /* Append the boolean modifier */
    iUtlWideStringBufferAppend(pvUtlFullNormalizedSearchWideStringBuffer, L" ");
    iUtlWideStringBufferAppend(pvUtlFullNormalizedSearchWideStringBuffer, pwcSrchParserGetStringFromID(pvSrchParser, pspSrchParser->uiBooleanOperationID));
    
    /* Append the operator case modifier */
    iUtlWideStringBufferAppend(pvUtlFullNormalizedSearchWideStringBuffer, L" ");
    iUtlWideStringBufferAppend(pvUtlFullNormalizedSearchWideStringBuffer, pwcSrchParserGetStringFromID(pvSrchParser, pspSrchParser->uiOperatorCaseID));
    
    /* Append the search case modifier */
    iUtlWideStringBufferAppend(pvUtlFullNormalizedSearchWideStringBuffer, L" ");
    iUtlWideStringBufferAppend(pvUtlFullNormalizedSearchWideStringBuffer, pwcSrchParserGetStringFromID(pvSrchParser, pspSrchParser->uiTermCaseID));

    /* Append the search terms modifier */
    iUtlWideStringBufferAppend(pvUtlFullNormalizedSearchWideStringBuffer, L" ");
    iUtlWideStringBufferAppend(pvUtlFullNormalizedSearchWideStringBuffer, pwcSrchParserGetStringFromID(pvSrchParser, pspSrchParser->uiFrequentTermsID));

    /* Append the search type modifier */
    iUtlWideStringBufferAppend(pvUtlFullNormalizedSearchWideStringBuffer, L" ");
    iUtlWideStringBufferAppend(pvUtlFullNormalizedSearchWideStringBuffer, pwcSrchParserGetStringFromID(pvSrchParser, pspSrchParser->uiSearchTypeID));
    
    /* Append the term weight */
    if ( pspSrchParser->fTermWeight > 0 ) {
        iUtlWideStringBufferAppend(pvUtlFullNormalizedSearchWideStringBuffer, L" ");
        iUtlWideStringBufferAppend(pvUtlFullNormalizedSearchWideStringBuffer, SRCH_PARSER_MODIFIER_TERM_WEIGHT_WSTRING);
        iUtlWideStringBufferAppend(pvUtlFullNormalizedSearchWideStringBuffer, SRCH_PARSER_MODIFIER_SEPARATOR_WSTRING);
        swprintf(pwcBuffer, SRCH_PARSER_MINIMUM_LENGTH + 1, L"%.2f", pspSrchParser->fTermWeight);
        iUtlWideStringBufferAppend(pvUtlFullNormalizedSearchWideStringBuffer, pwcBuffer);
        iUtlWideStringBufferAppend(pvUtlFullNormalizedSearchWideStringBuffer, SRCH_PARSER_CHARACTER_RBRACKET_WSTRING);
    }

    /* Append the feedback term weight */
    if ( pspSrchParser->fFeedbackTermWeight > 0 ) {
        iUtlWideStringBufferAppend(pvUtlFullNormalizedSearchWideStringBuffer, L" ");
        iUtlWideStringBufferAppend(pvUtlFullNormalizedSearchWideStringBuffer, SRCH_PARSER_MODIFIER_FEEDBACK_TERM_WEIGHT_WSTRING);
        iUtlWideStringBufferAppend(pvUtlFullNormalizedSearchWideStringBuffer, SRCH_PARSER_MODIFIER_SEPARATOR_WSTRING);
        swprintf(pwcBuffer, SRCH_PARSER_MINIMUM_LENGTH + 1, L"%.2f", pspSrchParser->fFeedbackTermWeight);
        iUtlWideStringBufferAppend(pvUtlFullNormalizedSearchWideStringBuffer, pwcBuffer);
        iUtlWideStringBufferAppend(pvUtlFullNormalizedSearchWideStringBuffer, SRCH_PARSER_CHARACTER_RBRACKET_WSTRING);
    }

    /* Append the frequent term coverage threshold */
    if ( pspSrchParser->fFrequentTermCoverageThreshold > 0 ) {
        iUtlWideStringBufferAppend(pvUtlFullNormalizedSearchWideStringBuffer, L" ");
        iUtlWideStringBufferAppend(pvUtlFullNormalizedSearchWideStringBuffer, SRCH_PARSER_MODIFIER_FREQUENT_TERM_COVERAGE_THRESHOLD_WSTRING);
        iUtlWideStringBufferAppend(pvUtlFullNormalizedSearchWideStringBuffer, SRCH_PARSER_MODIFIER_SEPARATOR_WSTRING);
        swprintf(pwcBuffer, SRCH_PARSER_MINIMUM_LENGTH + 1, L"%.2f", pspSrchParser->fFrequentTermCoverageThreshold);
        iUtlWideStringBufferAppend(pvUtlFullNormalizedSearchWideStringBuffer, pwcBuffer);
        iUtlWideStringBufferAppend(pvUtlFullNormalizedSearchWideStringBuffer, SRCH_PARSER_CHARACTER_RBRACKET_WSTRING);
    }

    /* Append the feedback minimum term count */
    if ( pspSrchParser->uiFeedbackMinimumTermCount > 0 ) {
        iUtlWideStringBufferAppend(pvUtlFullNormalizedSearchWideStringBuffer, L" ");
        iUtlWideStringBufferAppend(pvUtlFullNormalizedSearchWideStringBuffer, SRCH_PARSER_MODIFIER_FEEDBACK_MINIMUM_TERM_COUNT_WSTRING);
        iUtlWideStringBufferAppend(pvUtlFullNormalizedSearchWideStringBuffer, SRCH_PARSER_MODIFIER_SEPARATOR_WSTRING);
        swprintf(pwcBuffer, SRCH_PARSER_MINIMUM_LENGTH + 1, L"%u", pspSrchParser->uiFeedbackMinimumTermCount);
        iUtlWideStringBufferAppend(pvUtlFullNormalizedSearchWideStringBuffer, pwcBuffer);
        iUtlWideStringBufferAppend(pvUtlFullNormalizedSearchWideStringBuffer, SRCH_PARSER_CHARACTER_RBRACKET_WSTRING);
    }

    /* Append the feedback maximum term percentage */
    if ( pspSrchParser->fFeedbackMaximumTermPercentage > 0 ) {
        iUtlWideStringBufferAppend(pvUtlFullNormalizedSearchWideStringBuffer, L" ");
        iUtlWideStringBufferAppend(pvUtlFullNormalizedSearchWideStringBuffer, SRCH_PARSER_MODIFIER_FEEDBACK_MAXIMUM_TERM_PERCENTAGE_WSTRING);
        iUtlWideStringBufferAppend(pvUtlFullNormalizedSearchWideStringBuffer, SRCH_PARSER_MODIFIER_SEPARATOR_WSTRING);
        swprintf(pwcBuffer, SRCH_PARSER_MINIMUM_LENGTH + 1, L"%.2f", pspSrchParser->fFeedbackMaximumTermPercentage);
        iUtlWideStringBufferAppend(pvUtlFullNormalizedSearchWideStringBuffer, pwcBuffer);
        iUtlWideStringBufferAppend(pvUtlFullNormalizedSearchWideStringBuffer, SRCH_PARSER_CHARACTER_RBRACKET_WSTRING);
    }

    /* Append the feedback maximum term coverage threshold */
    if ( pspSrchParser->fFeedbackMaximumTermCoverageThreshold > 0 ) {
        iUtlWideStringBufferAppend(pvUtlFullNormalizedSearchWideStringBuffer, L" ");
        iUtlWideStringBufferAppend(pvUtlFullNormalizedSearchWideStringBuffer, SRCH_PARSER_MODIFIER_FEEDBACK_MAXIMUM_TERM_COVERAGE_THRESHOLD_WSTRING);
        iUtlWideStringBufferAppend(pvUtlFullNormalizedSearchWideStringBuffer, SRCH_PARSER_MODIFIER_SEPARATOR_WSTRING);
        swprintf(pwcBuffer, SRCH_PARSER_MINIMUM_LENGTH + 1, L"%.2f", pspSrchParser->fFeedbackMaximumTermCoverageThreshold);
        iUtlWideStringBufferAppend(pvUtlFullNormalizedSearchWideStringBuffer, pwcBuffer);
        iUtlWideStringBufferAppend(pvUtlFullNormalizedSearchWideStringBuffer, SRCH_PARSER_CHARACTER_RBRACKET_WSTRING);
    }

    /* Append the connection timeout */
    if ( pspSrchParser->uiConnectionTimeout > 0 ) {
        iUtlWideStringBufferAppend(pvUtlFullNormalizedSearchWideStringBuffer, L" ");
        iUtlWideStringBufferAppend(pvUtlFullNormalizedSearchWideStringBuffer, SRCH_PARSER_MODIFIER_CONNECTION_TIMEOUT_WSTRING);
        iUtlWideStringBufferAppend(pvUtlFullNormalizedSearchWideStringBuffer, SRCH_PARSER_MODIFIER_SEPARATOR_WSTRING);
        swprintf(pwcBuffer, SRCH_PARSER_MINIMUM_LENGTH + 1, L"%u", pspSrchParser->uiConnectionTimeout);
        iUtlWideStringBufferAppend(pvUtlFullNormalizedSearchWideStringBuffer, pwcBuffer);
        iUtlWideStringBufferAppend(pvUtlFullNormalizedSearchWideStringBuffer, SRCH_PARSER_CHARACTER_RBRACKET_WSTRING);
    }

    /* Append the search timeout */
    if ( pspSrchParser->uiSearchTimeout > 0 ) {
        iUtlWideStringBufferAppend(pvUtlFullNormalizedSearchWideStringBuffer, L" ");
        iUtlWideStringBufferAppend(pvUtlFullNormalizedSearchWideStringBuffer, SRCH_PARSER_MODIFIER_SEARCH_TIMEOUT_WSTRING);
        iUtlWideStringBufferAppend(pvUtlFullNormalizedSearchWideStringBuffer, SRCH_PARSER_MODIFIER_SEPARATOR_WSTRING);
        swprintf(pwcBuffer, SRCH_PARSER_MINIMUM_LENGTH + 1, L"%u", pspSrchParser->uiSearchTimeout);
        iUtlWideStringBufferAppend(pvUtlFullNormalizedSearchWideStringBuffer, pwcBuffer);
        iUtlWideStringBufferAppend(pvUtlFullNormalizedSearchWideStringBuffer, SRCH_PARSER_CHARACTER_RBRACKET_WSTRING);
    }

    /* Append the retrieval timeout */
    if ( pspSrchParser->uiRetrievalTimeout > 0 ) {
        iUtlWideStringBufferAppend(pvUtlFullNormalizedSearchWideStringBuffer, L" ");
        iUtlWideStringBufferAppend(pvUtlFullNormalizedSearchWideStringBuffer, SRCH_PARSER_MODIFIER_RETRIEVAL_TIMEOUT_WSTRING);
        iUtlWideStringBufferAppend(pvUtlFullNormalizedSearchWideStringBuffer, SRCH_PARSER_MODIFIER_SEPARATOR_WSTRING);
        swprintf(pwcBuffer, SRCH_PARSER_MINIMUM_LENGTH + 1, L"%u", pspSrchParser->uiRetrievalTimeout);
        iUtlWideStringBufferAppend(pvUtlFullNormalizedSearchWideStringBuffer, pwcBuffer);
        iUtlWideStringBufferAppend(pvUtlFullNormalizedSearchWideStringBuffer, SRCH_PARSER_CHARACTER_RBRACKET_WSTRING);
    }

    /* Append the information timeout */
    if ( pspSrchParser->uiInformationTimeout > 0 ) {
        iUtlWideStringBufferAppend(pvUtlFullNormalizedSearchWideStringBuffer, L" ");
        iUtlWideStringBufferAppend(pvUtlFullNormalizedSearchWideStringBuffer, SRCH_PARSER_MODIFIER_INFORMATION_TIMEOUT_WSTRING);
        iUtlWideStringBufferAppend(pvUtlFullNormalizedSearchWideStringBuffer, SRCH_PARSER_MODIFIER_SEPARATOR_WSTRING);
        swprintf(pwcBuffer, SRCH_PARSER_MINIMUM_LENGTH + 1, L"%u", pspSrchParser->uiInformationTimeout);
        iUtlWideStringBufferAppend(pvUtlFullNormalizedSearchWideStringBuffer, pwcBuffer);
        iUtlWideStringBufferAppend(pvUtlFullNormalizedSearchWideStringBuffer, SRCH_PARSER_CHARACTER_RBRACKET_WSTRING);
    }

    /* Append the maximum segments searched */
    if ( pspSrchParser->uiMaximumSegmentsSearched > 0 ) {
        iUtlWideStringBufferAppend(pvUtlFullNormalizedSearchWideStringBuffer, L" ");
        iUtlWideStringBufferAppend(pvUtlFullNormalizedSearchWideStringBuffer, SRCH_PARSER_MODIFIER_MAXIMUM_SEGMENTS_SEARCHED_WSTRING);
        iUtlWideStringBufferAppend(pvUtlFullNormalizedSearchWideStringBuffer, SRCH_PARSER_MODIFIER_SEPARATOR_WSTRING);
        swprintf(pwcBuffer, SRCH_PARSER_MINIMUM_LENGTH + 1, L"%u", pspSrchParser->uiMaximumSegmentsSearched);
        iUtlWideStringBufferAppend(pvUtlFullNormalizedSearchWideStringBuffer, pwcBuffer);
        iUtlWideStringBufferAppend(pvUtlFullNormalizedSearchWideStringBuffer, SRCH_PARSER_CHARACTER_RBRACKET_WSTRING);
    }

    /* Append the minimum segments searched */
    if ( pspSrchParser->uiMinimumSegmentsSearched > 0 ) {
        iUtlWideStringBufferAppend(pvUtlFullNormalizedSearchWideStringBuffer, L" ");
        iUtlWideStringBufferAppend(pvUtlFullNormalizedSearchWideStringBuffer, SRCH_PARSER_MODIFIER_MINIMUM_SEGMENTS_SEARCHED_WSTRING);
        iUtlWideStringBufferAppend(pvUtlFullNormalizedSearchWideStringBuffer, SRCH_PARSER_MODIFIER_SEPARATOR_WSTRING);
        swprintf(pwcBuffer, SRCH_PARSER_MINIMUM_LENGTH + 1, L"%u", pspSrchParser->uiMinimumSegmentsSearched);
        iUtlWideStringBufferAppend(pvUtlFullNormalizedSearchWideStringBuffer, pwcBuffer);
        iUtlWideStringBufferAppend(pvUtlFullNormalizedSearchWideStringBuffer, SRCH_PARSER_CHARACTER_RBRACKET_WSTRING);
    }




    /* Bail label */
    bailFromiSrchParserGetFullNormalizedSearchText:
    
    
    /* Handle the error */
    if ( iError == SRCH_NoError ) {

        /* Get the string from the buffer */
        iUtlWideStringBufferGetString(pvUtlFullNormalizedSearchWideStringBuffer, &pspSrchParser->pwcFullNormalizedSearchText);

        /* Free the normalized search string buffer, note that we don't release the string */
        iUtlWideStringBufferFree(pvUtlFullNormalizedSearchWideStringBuffer, false);
        pvUtlFullNormalizedSearchWideStringBuffer = NULL;

        /* Set the return pointer if the normalized search string buffer exists */
        *ppwcFullNormalizedSearchText = pspSrchParser->pwcFullNormalizedSearchText;
    }
    else {

        /* Free the normalized search string buffer */
        iUtlWideStringBufferFree(pvUtlFullNormalizedSearchWideStringBuffer, true);
        pvUtlFullNormalizedSearchWideStringBuffer = NULL;
    }



    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchParserGetTermCluster()

    Purpose:    This function returns a pointer to the RPN search text.

    Parameters: pvSrchParser                    search parser structure
                ppsptcSrchParserTermCluster     return pointer for the search parser term cluster

    Globals:    none

    Returns:    SRCH error code

*/
int iSrchParserGetTermCluster
(
    void *pvSrchParser,
    struct srchParserTermCluster **ppsptcSrchParserTermCluster
)
{

    struct srchParser   *pspSrchParser = (struct srchParser *)pvSrchParser;


    /* Check the parameters */
    if ( pvSrchParser == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvSrchParser' parameter passed to 'iSrchParserGetTermCluster'."); 
        return (SRCH_ParserInvalidParser);
    }

    if ( ppsptcSrchParserTermCluster == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppsptcSrchParserTermCluster' parameter passed to 'iSrchParserGetTermCluster'."); 
        return (SRCH_ReturnParameterError);
    }


    /* Set the return pointer */
    *ppsptcSrchParserTermCluster = pspSrchParser->psptcSrchParserTermCluster;


    return (SRCH_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   pwcSrchParserGetStringFromID()

    Purpose:    This function returns the token string for the passed token ID

    Parameters: pvSrchParser    search parser structure
                uiTokenID       the token ID

    Globals:    pptParserTokenRangeListGlobal, pptParserTokenOperatorListGlobal, 
                        pptParserTokenModifierListGlobal, pptParserTokenSortOrderListGlobal, 
                        pptParserTokenSortFieldNameListGlobal, pptParserTokenFunctionListGlobal

    Returns:    A pointer to the token string, NULL on error

*/
wchar_t *pwcSrchParserGetStringFromID
(
    void *pvSrchParser,
    unsigned int uiTokenID
)
{

    struct srchParserToken  *psptSrchParserTokenListPtr = NULL;


    /* Check the parameters */
    if ( pvSrchParser == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvSrchParser' parameter passed to 'pwcSrchParserGetStringFromID'."); 
        return (NULL);
    }


    /* Check the range token list */
    for ( psptSrchParserTokenListPtr = pptParserTokenRangeListGlobal; psptSrchParserTokenListPtr->pwcTokenString != NULL; psptSrchParserTokenListPtr++ ) {
        if ( psptSrchParserTokenListPtr->uiTokenID == uiTokenID) {
            return (psptSrchParserTokenListPtr->pwcTokenString);
        }
    }


    /* Check the operator token list */
    for ( psptSrchParserTokenListPtr = pptParserTokenOperatorListGlobal; psptSrchParserTokenListPtr->pwcTokenString != NULL; psptSrchParserTokenListPtr++ ) {
        if ( psptSrchParserTokenListPtr->uiTokenID == uiTokenID) {
            return (psptSrchParserTokenListPtr->pwcTokenString);
        }
    }


    /* Check the modifier token list */
    for ( psptSrchParserTokenListPtr = pptParserTokenModifierListGlobal; psptSrchParserTokenListPtr->pwcTokenString != NULL; psptSrchParserTokenListPtr++ ) {
        if ( psptSrchParserTokenListPtr->uiTokenID == uiTokenID) {
            return (psptSrchParserTokenListPtr->pwcTokenString);
        }
    }


    /* Check the sort order token list */
    for ( psptSrchParserTokenListPtr = pptParserTokenSortOrderListGlobal; psptSrchParserTokenListPtr->pwcTokenString != NULL; psptSrchParserTokenListPtr++ ) {
        if ( psptSrchParserTokenListPtr->uiTokenID == uiTokenID) {
            return (psptSrchParserTokenListPtr->pwcTokenString);
        }
    }


    /* Check the sort field name token list */
    for ( psptSrchParserTokenListPtr = pptParserTokenSortFieldNameListGlobal; psptSrchParserTokenListPtr->pwcTokenString != NULL; psptSrchParserTokenListPtr++ ) {
        if ( psptSrchParserTokenListPtr->uiTokenID == uiTokenID) {
            return (psptSrchParserTokenListPtr->pwcTokenString);
        }
    }


    /* Check the function token list */
    for ( psptSrchParserTokenListPtr = pptParserTokenFunctionListGlobal; psptSrchParserTokenListPtr->pwcTokenString != NULL; psptSrchParserTokenListPtr++ ) {
        if ( psptSrchParserTokenListPtr->uiTokenID == uiTokenID) {
            return (psptSrchParserTokenListPtr->pwcTokenString);
        }
    }


    return (NULL);

}


/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchParserSetDefaults()

    Purpose:    This function set the search parser defaults. 

    Parameters: pspSrchParser   search parser structure

    Globals:    none

    Returns:    SRCH error code

*/
static int iSrchParserSetDefaults
(
    struct srchParser *pspSrchParser
)
{

    int             iError = SRCH_NoError;
    unsigned char   pucConfigValue[UTL_CONFIG_VALUE_MAXIMUM_LENGTH + 1] = {'\0'};
    wchar_t         pwcConfigValue[UTL_CONFIG_VALUE_MAXIMUM_LENGTH + 1] = {L'\0'};
    unsigned int    uiTokenID = SRCH_PARSER_INVALID_ID;


    ASSERT(pspSrchParser != NULL);


    /* Initialize the structure variables */
    pspSrchParser->uiLanguageID = LNG_LANGUAGE_ANY_ID;
    pspSrchParser->uiTokenizerID = LNG_TOKENIZER_ANY_ID;
    
    pspSrchParser->uiSearchResultsID = SRCH_PARSER_SEARCH_RESULTS_ID_DEFAULT;
    pspSrchParser->uiSearchReportID = SRCH_PARSER_SEARCH_REPORT_ID_DEFAULT;
    pspSrchParser->uiSearchCacheID = SRCH_PARSER_SEARCH_CACHE_ID_DEFAULT;
    pspSrchParser->uiDebugID = SRCH_PARSER_DEBUG_ID_DEFAULT;
    pspSrchParser->uiEarlyCompletionID = SRCH_PARSER_EARLY_COMPLETION_ID_DEFAULT;

    pspSrchParser->uiBooleanOperatorID = SRCH_PARSER_BOOLEAN_OPERATOR_ID_DEFAULT;
    pspSrchParser->uiBooleanOperationID = SRCH_PARSER_BOOLEAN_OPERATION_ID_DEFAULT;
    pspSrchParser->uiOperatorCaseID = SRCH_PARSER_OPERATOR_CASE_ID_DEFAULT;
    pspSrchParser->uiTermCaseID = SRCH_PARSER_TERM_CASE_ID_DEFAULT;
    pspSrchParser->uiFrequentTermsID = SRCH_PARSER_FREQUENT_TERMS_ID_DEFAULT;
    pspSrchParser->uiSearchTypeID = SRCH_PARSER_SEARCH_TYPE_ID_DEFAULT;

    pspSrchParser->pspnSrchParserNumberDates = NULL;
    pspSrchParser->uiSrchParserNumberDatesLength = 0;

    pspSrchParser->pwcUnfieldedSearchFieldNames = NULL;

    pspSrchParser->pwcSortFieldName = NULL;
    pspSrchParser->uiSortOrderID = SRCH_PARSER_INVALID_ID;
    
    pspSrchParser->pspfSrchParserFilterExclusionFilters = NULL;
    pspSrchParser->uiSrchParserFilterExclusionFiltersLength = 0;

    pspSrchParser->pspfSrchParserFilterInclusionFilters = NULL;
    pspSrchParser->uiSrchParserFilterInclusionFiltersLength = 0;

    pspSrchParser->pspnSrchParserNumberLanguageIDs = NULL;
    pspSrchParser->uiSrchParserNumberLanguageIDsLength = 0;

    pspSrchParser->fTermWeight = 0;
    pspSrchParser->fFeedbackTermWeight = 0;

    pspSrchParser->fFrequentTermCoverageThreshold = 0;

    pspSrchParser->uiFeedbackMinimumTermCount = 0;
    pspSrchParser->fFeedbackMaximumTermPercentage = 0;
    pspSrchParser->fFeedbackMaximumTermCoverageThreshold = 0;

    pspSrchParser->uiConnectionTimeout = 0;
    pspSrchParser->uiSearchTimeout = 0;
    pspSrchParser->uiRetrievalTimeout = 0;
    pspSrchParser->uiInformationTimeout = 0;

    pspSrchParser->uiMaximumSegmentsSearched = 0;
    pspSrchParser->uiMinimumSegmentsSearched = 0;

    pspSrchParser->uiSearchTermCount = 0;
    
    pspSrchParser->psptcSrchParserTermCluster = NULL;

    pspSrchParser->pwcNormalizedSearchText = NULL;
    pspSrchParser->pwcFullNormalizedSearchText = NULL;

    /* Don't set this to NULL!!! Here to prevent that from happening */
/*     pspSrchParser->pssSrchSearch = NULL; */


    /* Set the sort field name and sort order ID to their defaults */
    pspSrchParser->pwcSortFieldName = NULL;
    pspSrchParser->uiSortOrderID = SRCH_PARSER_INVALID_ID;
    
    /* Get the parser sort config value from the configuration file */
    if ( iUtlConfigGetValue(pspSrchParser->pssSrchSearch->pvUtlConfig, SRCH_SEARCH_CONFIG_PARSER_SORT, pucConfigValue, UTL_CONFIG_VALUE_MAXIMUM_LENGTH + 1) == UTL_NoError ) {
        
        wchar_t         *pwcSymbolDataPtr = NULL;
        wchar_t         pwcScanfFormat[UTL_STRING_SCANF_FORMAT_LENGTH + 1] = {L'\0'};
        wchar_t         pwcSortFieldName[SRCH_PARSER_MINIMUM_LENGTH + 1] = {L'\0'};
        wchar_t         *pwcSortFieldNamePtr = NULL;
        unsigned int    uiSortFieldNameID = SRCH_PARSER_INVALID_ID;
        wchar_t         pwcSortOrder[SRCH_PARSER_MINIMUM_LENGTH + 1] = {L'\0'};

        /* Convert the symbol from utf-8 to wide characters */
        if ( (iError = iLngConvertUtf8ToWideString_s(pucConfigValue, 0, pwcConfigValue, UTL_CONFIG_VALUE_MAXIMUM_LENGTH + 1)) != LNG_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to convert the parser sort found in the search configuration file from utf-8 to wide characters, config key: '%s', config value: '%s', lng error: %d.", 
                    SRCH_SEARCH_CONFIG_PARSER_SORT, pucConfigValue, iError); 
            return (SRCH_ParserCharacterSetConversionFailed);
        }

        /* Get a pointer to the colon */
        if ( (pwcSymbolDataPtr = s_wcsstr(pwcConfigValue, SRCH_PARSER_RANGE_COLON_WSTRING)) == NULL ) {
            return (SRCH_ParserInvalidSort);
        }

        /* Scan the sort */
        swprintf(pwcScanfFormat, UTL_STRING_SCANF_FORMAT_LENGTH + 1, L"%ls%%%dl[^%ls]%ls%%%dl[^%ls]%ls", SRCH_PARSER_MODIFIER_SEPARATOR_WSTRING, 
                SRCH_PARSER_MINIMUM_LENGTH, SRCH_PARSER_MODIFIER_SEPARATOR_WSTRING, SRCH_PARSER_MODIFIER_SEPARATOR_WSTRING, 
                SRCH_PARSER_MINIMUM_LENGTH, SRCH_PARSER_CHARACTER_RBRACKET_WSTRING, SRCH_PARSER_CHARACTER_RBRACKET_WSTRING);
        if ( swscanf(pwcSymbolDataPtr, pwcScanfFormat, pwcSortFieldName, pwcSortOrder) != 2 ) {
            return (SRCH_ParserInvalidSort);
        }
        
        /* Add the sort field name and sort order ID */
        if ( (pspSrchParser->uiSortOrderID = uiSrchParserGetSortOrderIDFromString(pspSrchParser, pwcSortOrder)) == SRCH_PARSER_INVALID_ID ) {
            return (SRCH_ParserInvalidSortOrder);
        }

        /* Use the canonical sort field name if this is an internal sort field name, otherwise just use the sort field name */
        if ( (uiSortFieldNameID = uiSrchParserGetSortFieldNameIDFromString(pspSrchParser, pwcSortFieldName)) != SRCH_PARSER_INVALID_ID ) {
            pwcSortFieldNamePtr = pwcSrchParserGetStringFromID(pspSrchParser, uiSortFieldNameID);
        }
        else {
            pwcSortFieldNamePtr = pwcSortFieldName;
        }

        /* Free the sort field name if it is already set */
        s_free(pspSrchParser->pwcSortFieldName);

        /* Add the sort field name */
        if ( (pspSrchParser->pwcSortFieldName = s_wcsdup(pwcSortFieldNamePtr)) == NULL ) {
            return (SRCH_MemError);
        }

        /* Convert the sort field name to lower case */
        pwcLngCaseConvertWideStringToLowerCase(pspSrchParser->pwcSortFieldName);
    }



    /* Set the operator ID to the default of {boolean_operator:and} */
    pspSrchParser->uiBooleanOperatorID = SRCH_PARSER_BOOLEAN_OPERATOR_ID_DEFAULT;

    /* Get the parser boolean operator config value from the configuration file */
    if ( iUtlConfigGetValue(pspSrchParser->pssSrchSearch->pvUtlConfig, SRCH_SEARCH_CONFIG_PARSER_BOOLEAN_OPERATOR, pucConfigValue, UTL_CONFIG_VALUE_MAXIMUM_LENGTH + 1) == UTL_NoError ) {
        
        /* Convert the symbol from utf-8 to wide characters */
        if ( (iError = iLngConvertUtf8ToWideString_s(pucConfigValue, 0, pwcConfigValue, UTL_CONFIG_VALUE_MAXIMUM_LENGTH + 1)) != LNG_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to convert the parser boolean operator found in the search configuration file from utf-8 to wide characters, config key: '%s', config value: '%s', lng error: %d.", 
                    SRCH_SEARCH_CONFIG_PARSER_BOOLEAN_OPERATOR, pucConfigValue, iError); 
            return (SRCH_ParserCharacterSetConversionFailed);
        }

        /* Get the ID from the symbol */
        uiTokenID = uiSrchParserGetModifierIDFromString(pspSrchParser, pwcConfigValue);
        
        /* {boolean_operator:or} */
        if ( uiTokenID == SRCH_PARSER_MODIFIER_BOOLEAN_OPERATOR_OR_ID ) {
            pspSrchParser->uiBooleanOperatorID = SRCH_PARSER_MODIFIER_BOOLEAN_OPERATOR_OR_ID;
        }
        
        /* {boolean_operator:ior} */
        else if ( uiTokenID == SRCH_PARSER_MODIFIER_BOOLEAN_OPERATOR_IOR_ID ) {
            pspSrchParser->uiBooleanOperatorID = SRCH_PARSER_MODIFIER_BOOLEAN_OPERATOR_IOR_ID;
        }
        
        /* {boolean_operator:xor} */
        else if ( uiTokenID == SRCH_PARSER_MODIFIER_BOOLEAN_OPERATOR_XOR_ID ) {
            pspSrchParser->uiBooleanOperatorID = SRCH_PARSER_MODIFIER_BOOLEAN_OPERATOR_XOR_ID;
        }
        
        /* {boolean_operator:and} */
        else if ( uiTokenID == SRCH_PARSER_MODIFIER_BOOLEAN_OPERATOR_AND_ID ) {
            pspSrchParser->uiBooleanOperatorID = SRCH_PARSER_MODIFIER_BOOLEAN_OPERATOR_AND_ID;
        }
        
        /* {boolean_operator:adj} */
        else if ( uiTokenID == SRCH_PARSER_MODIFIER_BOOLEAN_OPERATOR_ADJ_ID ) {
            pspSrchParser->uiBooleanOperatorID = SRCH_PARSER_MODIFIER_BOOLEAN_OPERATOR_ADJ_ID;
        }
        
        /* {boolean_operator:near} */
        else if ( uiTokenID == SRCH_PARSER_MODIFIER_BOOLEAN_OPERATOR_NEAR_ID ) {
            pspSrchParser->uiBooleanOperatorID = SRCH_PARSER_MODIFIER_BOOLEAN_OPERATOR_NEAR_ID;
        }
        
        else {
            iUtlLogWarn(UTL_LOG_CONTEXT, "Invalid parser boolean operator found in the search configuration file, config key: '%s', config value: '%s', using default: '%ls'.", 
                    SRCH_SEARCH_CONFIG_PARSER_BOOLEAN_OPERATOR, pucConfigValue, SRCH_PARSER_BOOLEAN_OPERATOR_WSTRING_DEFAULT);
        }
    }



    /* Set the boolean ID to the default of {booleanstrict} */
    pspSrchParser->uiBooleanOperationID = SRCH_PARSER_BOOLEAN_OPERATION_ID_DEFAULT;

    /* Get the parser boolean operation config value from the configuration file */
    if ( iUtlConfigGetValue(pspSrchParser->pssSrchSearch->pvUtlConfig, SRCH_SEARCH_CONFIG_PARSER_BOOLEAN_OPERATION, pucConfigValue, UTL_CONFIG_VALUE_MAXIMUM_LENGTH + 1) == UTL_NoError ) {

        /* Convert the symbol from utf-8 to wide characters */
        if ( (iError = iLngConvertUtf8ToWideString_s(pucConfigValue, 0, pwcConfigValue, UTL_CONFIG_VALUE_MAXIMUM_LENGTH + 1)) != LNG_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to convert the parser boolean operation found in the search configuration file from utf-8 to wide characters, config key: '%s', config value: '%s', lng error: %d.", 
                    SRCH_SEARCH_CONFIG_PARSER_BOOLEAN_OPERATION, pucConfigValue, iError); 
            return (SRCH_ParserCharacterSetConversionFailed);
        }

        /* Get the ID from the symbol */
        uiTokenID = uiSrchParserGetModifierIDFromString(pspSrchParser, pwcConfigValue);
        
        /* {boolean_operation:relaxed} */
        if ( uiTokenID == SRCH_PARSER_MODIFIER_BOOLEAN_OPERATION_RELAXED_ID ) {
            pspSrchParser->uiBooleanOperationID = SRCH_PARSER_MODIFIER_BOOLEAN_OPERATION_RELAXED_ID;
        }
        
        /* {boolean_operation:strict} */
        else if ( uiTokenID == SRCH_PARSER_MODIFIER_BOOLEAN_OPERATION_STRICT_ID ) {
            pspSrchParser->uiBooleanOperationID = SRCH_PARSER_MODIFIER_BOOLEAN_OPERATION_STRICT_ID;
        }
        
        else {
            iUtlLogWarn(UTL_LOG_CONTEXT, "Invalid parser boolean operation found in the search configuration file, config key: '%s', config value: '%s', using default: '%ls'.", 
                    SRCH_SEARCH_CONFIG_PARSER_BOOLEAN_OPERATION, pucConfigValue, SRCH_PARSER_MODIFIER_BOOLEAN_OPERATION_STRICT_WSTRING);
        }
    }



    /* Set the operator case ID to the default of {operator_case:any} */
    pspSrchParser->uiOperatorCaseID = SRCH_PARSER_OPERATOR_CASE_ID_DEFAULT;

    /* Get the parser operator case config value from the configuration file */
    if ( iUtlConfigGetValue(pspSrchParser->pssSrchSearch->pvUtlConfig, SRCH_SEARCH_CONFIG_PARSER_OPERATOR_CASE, pucConfigValue, UTL_CONFIG_VALUE_MAXIMUM_LENGTH + 1) == UTL_NoError ) {
        
        /* Convert the symbol from utf-8 to wide characters */
        if ( (iError = iLngConvertUtf8ToWideString_s(pucConfigValue, 0, pwcConfigValue, UTL_CONFIG_VALUE_MAXIMUM_LENGTH + 1)) != LNG_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to convert the parser operator case found in the search configuration file from utf-8 to wide characters, config key: '%s', config value: '%s', lng error: %d.", 
                    SRCH_SEARCH_CONFIG_PARSER_OPERATOR_CASE, pucConfigValue, iError); 
            return (SRCH_ParserCharacterSetConversionFailed);
        }

        /* Get the ID from the symbol */
        uiTokenID = uiSrchParserGetModifierIDFromString(pspSrchParser, pwcConfigValue);
        
        /* {operator_case:any} */
        if ( uiTokenID == SRCH_PARSER_MODIFIER_OPERATOR_CASE_ANY_ID ) {
            pspSrchParser->uiOperatorCaseID = SRCH_PARSER_MODIFIER_OPERATOR_CASE_ANY_ID;
        }
        
        /* {operator_case:upper} */
        else if ( uiTokenID == SRCH_PARSER_MODIFIER_OPERATOR_CASE_UPPER_ID ) {
            pspSrchParser->uiOperatorCaseID = SRCH_PARSER_MODIFIER_OPERATOR_CASE_UPPER_ID;
        }
        
        /* {operator_case:lower} */
        else if ( uiTokenID == SRCH_PARSER_MODIFIER_OPERATOR_CASE_LOWER_ID ) {
            pspSrchParser->uiOperatorCaseID = SRCH_PARSER_MODIFIER_OPERATOR_CASE_LOWER_ID;
        }

        else {
            iUtlLogWarn(UTL_LOG_CONTEXT, "Invalid parser operator case found in the search configuration file, config key: '%s', config value: '%s', using default: '%ls'.", 
                    SRCH_SEARCH_CONFIG_PARSER_OPERATOR_CASE, pucConfigValue, SRCH_PARSER_MODIFIER_OPERATOR_CASE_ANY_WSTRING);
        }
    }



    /* Set the search case ID to the default of {term_case:keep} */
    pspSrchParser->uiTermCaseID = SRCH_PARSER_TERM_CASE_ID_DEFAULT;

    /* Get the parser term case config value from the configuration file */
    if ( iUtlConfigGetValue(pspSrchParser->pssSrchSearch->pvUtlConfig, SRCH_SEARCH_CONFIG_PARSER_TERM_CASE, pucConfigValue, UTL_CONFIG_VALUE_MAXIMUM_LENGTH + 1) == UTL_NoError ) {
        
        /* Convert the symbol from utf-8 to wide characters */
        if ( (iError = iLngConvertUtf8ToWideString_s(pucConfigValue, 0, pwcConfigValue, UTL_CONFIG_VALUE_MAXIMUM_LENGTH + 1)) != LNG_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to convert the parser term case found in the search configuration file from utf-8 to wide characters, config key: '%s', config value: '%s', lng error: %d.", 
                    SRCH_SEARCH_CONFIG_PARSER_TERM_CASE, pucConfigValue, iError); 
            return (SRCH_ParserCharacterSetConversionFailed);
        }

        /* Get the ID from the symbol */
        uiTokenID = uiSrchParserGetModifierIDFromString(pspSrchParser, pwcConfigValue);
        
        /* {term_case:keep} */
        if ( uiTokenID == SRCH_PARSER_MODIFIER_TERM_CASE_KEEP_ID ) {
            pspSrchParser->uiTermCaseID = SRCH_PARSER_MODIFIER_TERM_CASE_KEEP_ID;
        }
        
        /* {term_case:drop} */
        else if ( uiTokenID == SRCH_PARSER_MODIFIER_TERM_CASE_DROP_ID ) {
            pspSrchParser->uiTermCaseID = SRCH_PARSER_MODIFIER_TERM_CASE_DROP_ID;
        }

        else {
            iUtlLogWarn(UTL_LOG_CONTEXT, "Invalid parser term case found in the search configuration file, config key: '%s', config value: '%s', using default: '%ls'.", 
                    SRCH_SEARCH_CONFIG_PARSER_TERM_CASE, pucConfigValue, SRCH_PARSER_MODIFIER_TERM_CASE_KEEP_WSTRING);
        }
    }



    /* Set the search terms ID to the default of {frequent_terms:keep} */
    pspSrchParser->uiFrequentTermsID = SRCH_PARSER_FREQUENT_TERMS_ID_DEFAULT;

    /* Get the parser frequent terms config value from the configuration file */
    if ( iUtlConfigGetValue(pspSrchParser->pssSrchSearch->pvUtlConfig, SRCH_SEARCH_CONFIG_PARSER_FREQUENT_TERMS, pucConfigValue, UTL_CONFIG_VALUE_MAXIMUM_LENGTH + 1) == UTL_NoError ) {
        
        /* Convert the symbol from utf-8 to wide characters */
        if ( (iError = iLngConvertUtf8ToWideString_s(pucConfigValue, 0, pwcConfigValue, UTL_CONFIG_VALUE_MAXIMUM_LENGTH + 1)) != LNG_NoError ) {
                iUtlLogError(UTL_LOG_CONTEXT, "Failed to convert the parser frequent terms found in the search configuration file from utf-8 to wide characters, config key: '%s', config value: '%s', lng error: %d.", 
                        SRCH_SEARCH_CONFIG_PARSER_FREQUENT_TERMS, pucConfigValue, iError); 
            return (SRCH_ParserCharacterSetConversionFailed);
        }

        /* Get the ID from the symbol */
        uiTokenID = uiSrchParserGetModifierIDFromString(pspSrchParser, pwcConfigValue);
        
        /* {frequent_terms:keep} */
        if ( uiTokenID == SRCH_PARSER_MODIFIER_FREQUENT_TERMS_KEEP_ID ) {
            pspSrchParser->uiFrequentTermsID = SRCH_PARSER_MODIFIER_FREQUENT_TERMS_KEEP_ID;
        }
        
        /* {frequent_terms:drop} */
        else if ( uiTokenID == SRCH_PARSER_MODIFIER_FREQUENT_TERMS_DROP_ID ) {
            pspSrchParser->uiFrequentTermsID = SRCH_PARSER_MODIFIER_FREQUENT_TERMS_DROP_ID;
        }

        else {
            iUtlLogWarn(UTL_LOG_CONTEXT, "Invalid parser frequent terms found in the search configuration file, config key: '%s', config value: '%s', using default: '%ls'.", 
                    SRCH_SEARCH_CONFIG_PARSER_FREQUENT_TERMS, pucConfigValue, SRCH_PARSER_MODIFIER_FREQUENT_TERMS_KEEP_WSTRING);
        }
    }



    /* Set the search type ID to the default of {search_type:boolean} */
    pspSrchParser->uiSearchTypeID = SRCH_PARSER_SEARCH_TYPE_ID_DEFAULT;

    /* Get the parser search type config value from the configuration file */
    if ( iUtlConfigGetValue(pspSrchParser->pssSrchSearch->pvUtlConfig, SRCH_SEARCH_CONFIG_PARSER_SEARCH_TYPE, pucConfigValue, UTL_CONFIG_VALUE_MAXIMUM_LENGTH + 1) == UTL_NoError ) {
        
        /* Convert the symbol from utf-8 to wide characters */
        if ( (iError = iLngConvertUtf8ToWideString_s(pucConfigValue, 0, pwcConfigValue, UTL_CONFIG_VALUE_MAXIMUM_LENGTH + 1)) != LNG_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to convert the parser search type found in the search configuration file from utf-8 to wide characters, config key: '%s', config value: '%s', lng error: %d.", 
                    SRCH_SEARCH_CONFIG_PARSER_SEARCH_TYPE, pucConfigValue, iError); 
            return (SRCH_ParserCharacterSetConversionFailed);
        }

        /* Get the ID from the symbol */
        uiTokenID = uiSrchParserGetModifierIDFromString(pspSrchParser, pwcConfigValue);
        
        /* {search_type:boolean} */
        if ( uiTokenID == SRCH_PARSER_MODIFIER_SEARCH_TYPE_BOOLEAN_ID ) {
            pspSrchParser->uiSearchTypeID = SRCH_PARSER_MODIFIER_SEARCH_TYPE_BOOLEAN_ID;
        }
        
        /* {search_type:freetext} */
        else if ( uiTokenID == SRCH_PARSER_MODIFIER_SEARCH_TYPE_FREETEXT_ID ) {
            pspSrchParser->uiSearchTypeID = SRCH_PARSER_MODIFIER_SEARCH_TYPE_FREETEXT_ID;
        }

        else {
            iUtlLogWarn(UTL_LOG_CONTEXT, "Invalid parser search type found in the search configuration file, config key: '%s', config value: '%s', using default: '%ls'.", 
                    SRCH_SEARCH_CONFIG_PARSER_SEARCH_TYPE, pucConfigValue, SRCH_PARSER_MODIFIER_SEARCH_TYPE_BOOLEAN_WSTRING);
        }
    }


    /* Make sure that the IDs are set */
    ASSERT(pspSrchParser->uiBooleanOperatorID != SRCH_PARSER_INVALID_ID);
    ASSERT(pspSrchParser->uiBooleanOperationID != SRCH_PARSER_INVALID_ID);
    ASSERT(pspSrchParser->uiOperatorCaseID != SRCH_PARSER_INVALID_ID);
    ASSERT(pspSrchParser->uiTermCaseID != SRCH_PARSER_INVALID_ID);
    ASSERT(pspSrchParser->uiFrequentTermsID != SRCH_PARSER_INVALID_ID);
    ASSERT(pspSrchParser->uiSearchTypeID != SRCH_PARSER_INVALID_ID);


    return (SRCH_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchParserFreeStructureFields()

    Purpose:    This function frees the search parser structure fields. 

    Parameters: pspSrchParser   search parser structure

    Globals:    none

    Returns:    SRCH error code

*/
static int iSrchParserFreeStructureFields
(
    struct srchParser *pspSrchParser
)
{

    ASSERT(pspSrchParser != NULL);


    /* Free the dates */
    iSrchParserFreeNumbers(pspSrchParser->pspnSrchParserNumberDates, pspSrchParser->uiSrchParserNumberDatesLength);
    pspSrchParser->pspnSrchParserNumberDates = NULL;
    pspSrchParser->uiSrchParserNumberDatesLength = 0;

    /* Free the unfielded search field names */
    s_free(pspSrchParser->pwcUnfieldedSearchFieldNames);

    /* Free the sort field name */
    s_free(pspSrchParser->pwcSortFieldName);

    /* Free the exclusion filters */
    iSrchParserFreeFilters(pspSrchParser->pspfSrchParserFilterExclusionFilters, pspSrchParser->uiSrchParserFilterExclusionFiltersLength);
    pspSrchParser->pspfSrchParserFilterExclusionFilters = NULL;
    pspSrchParser->uiSrchParserFilterExclusionFiltersLength = 0;

    /* Free the inclusion filters */
    iSrchParserFreeFilters(pspSrchParser->pspfSrchParserFilterInclusionFilters, pspSrchParser->uiSrchParserFilterInclusionFiltersLength);
    pspSrchParser->pspfSrchParserFilterInclusionFilters = NULL;
    pspSrchParser->uiSrchParserFilterInclusionFiltersLength = 0;

    /* Free the language IDs */
    iSrchParserFreeNumbers(pspSrchParser->pspnSrchParserNumberLanguageIDs, pspSrchParser->uiSrchParserNumberLanguageIDsLength);
    pspSrchParser->pspnSrchParserNumberLanguageIDs = NULL;
    pspSrchParser->uiSrchParserNumberLanguageIDsLength = 0;

    /* Free the tokenizer */
    iLngTokenizerFree(pspSrchParser->pvLngTokenizer);
    pspSrchParser->pvLngTokenizer = NULL;

    /* Free the term cluster */
    iSrchParserFreeTermCluster(pspSrchParser->psptcSrchParserTermCluster);
    pspSrchParser->psptcSrchParserTermCluster = NULL;

    /* Free the normalized search text */
    s_free(pspSrchParser->pwcNormalizedSearchText);

    /* Free the full normalized search text */
    s_free(pspSrchParser->pwcFullNormalizedSearchText);


    return (SRCH_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchParserCleanSearch()

    Purpose:    This function pre-parses out the passed search and makes sure that
                there are spaces between all the terms, operators and brackets.

    Parameters: pspSrchParser       search parser structure
                pwcSearchText       search text (optional)
                ppwcCleanedSearch   return parameter for the cleaned search

    Globals:    none

    Returns:    SRCH error code

*/
static int iSrchParserCleanSearch
(
    struct srchParser *pspSrchParser,
    wchar_t *pwcSearchText,
    wchar_t **ppwcCleanedSearch
)
{

    wchar_t     *pwcCleanedSearch = NULL;
    wchar_t     *pwcCleanedSearchPtr = NULL;
    wchar_t     *pwcSearchTextPtr = NULL;
        

    ASSERT(pspSrchParser != NULL);
    ASSERT(ppwcCleanedSearch != NULL);


    /* Check the search */
    if ( bUtlStringsIsWideStringNULL(pwcSearchText) == true ) {
        return (SRCH_NoError);
    }


#if defined(SRCH_PARSER_ENABLE_CONTROL_CHARACTER_STRIPPING)
    /* Replace control characters with spaces */
    for ( pwcSearchTextPtr = pwcSearchText; *pwcSearchTextPtr != L'\0'; pwcSearchTextPtr++ ) {
/*         if ( iswcntrl(*pwcSearchTextPtr) != 0 ) { */
        if ( *pwcSearchTextPtr < L' ' ) {
            *pwcSearchTextPtr = L' ';
        }
    }
#endif    /* defined(SRCH_PARSER_ENABLE_CONTROL_CHARACTER_STRIPPING) */


#if defined(SRCH_PARSER_ENABLE_DOUBLE_QUOTES_STRIPPING)
    /* Get rid of multiple quotes in strings */
    while ( (pwcSearchTextPtr = (wchar_t *)s_wcsstr(pwcSearchText, L"\"\"")) != NULL ) {
        s_memmove(pwcSearchTextPtr, pwcSearchTextPtr + 1, s_wcslen(pwcSearchTextPtr) * sizeof(wchar_t));
    }
#endif    /* defined(SRCH_PARSER_ENABLE_DOUBLE_QUOTES_STRIPPING) */


#if defined(SRCH_PARSER_ENABLE_ESCAPED_QUOTE_STRIPPING)
    /* Get rid of escaped quotes in strings */
    while ( (pwcSearchTextPtr = (wchar_t *)s_wcsstr(pwcSearchText, L"\\\"")) != NULL ) {
        s_memmove(pwcSearchTextPtr, pwcSearchTextPtr + 1, s_wcslen(pwcSearchTextPtr) * sizeof(wchar_t));
    }
#endif    /* defined(SRCH_PARSER_ENABLE_ESCAPED_QUOTE_STRIPPING) */


    /* Allocate a pointer for the new line */
    if ( (pwcCleanedSearch = (wchar_t *)s_malloc((size_t)(((s_wcslen(pwcSearchText) * SRCH_PARSER_GROWTH_FACTOR) + SRCH_PARSER_MINIMUM_LENGTH) * sizeof(wchar_t)))) == NULL ) {
        return (SRCH_MemError);
    }


    /* Loop over every character in the line */
    for ( pwcSearchTextPtr = pwcSearchText, pwcCleanedSearchPtr = pwcCleanedSearch; *pwcSearchTextPtr != L'\0'; pwcSearchTextPtr++ ) {

        /* Deal with double quotes " */
        if ( *pwcSearchTextPtr == SRCH_PARSER_CHARACTER_DOUBLE_QUOTE_WCHAR ) {

            /* Add in a space */
            if ( pwcCleanedSearchPtr != pwcCleanedSearch ) {
                *pwcCleanedSearchPtr++ = L' ';
            }

            /* Write the operator - single character */
            *pwcCleanedSearchPtr++ = *pwcSearchTextPtr;

            /* Add in a space */
            *pwcCleanedSearchPtr++ = L' ';
        }

#if defined(SRCH_PARSER_ENABLE_SINGLE_QUOTE_FOR_PHRASES)
        /* Deal with single quotes ' */
        else if ( *pwcSearchTextPtr == SRCH_PARSER_CHARACTER_SINGLE_QUOTE_WCHAR ) {

            /* Replace the single quote with a double quote if it is at the start 
            ** or the end of the string, or if it either it's neighbors is a space
            */
            if ( (pwcSearchTextPtr == pwcSearchText) || ((*pwcSearchTextPtr + 1) == L'\0') ||
                    (*(pwcSearchTextPtr - 1) == L' ') || (*(pwcSearchTextPtr + 1) == L' ') ) {

                /* Add in a space */
                if ( pwcCleanedSearchPtr != pwcCleanedSearch ) {
                    *pwcCleanedSearchPtr++ = L' ';
                }
    
                /* Write the operator - double quote character */
                *pwcCleanedSearchPtr++ = SRCH_PARSER_CHARACTER_DOUBLE_QUOTE_WCHAR;
    
                /* Add in a space */
                *pwcCleanedSearchPtr++ = L' ';
            }
            else {
                /* Write the single character */
                *pwcCleanedSearchPtr++ = *pwcSearchTextPtr;
            }
        }
#endif    /* defined(SRCH_PARSER_ENABLE_SINGLE_QUOTE_FOR_PHRASES) */

        /* Deal with parens '()' */
        else if ( s_wcschr(SRCH_PARSER_OPERATOR_PARENS_WSTRING, *pwcSearchTextPtr) != NULL ) {

            /* Add in a space */
            if ( pwcCleanedSearchPtr != pwcCleanedSearch ) {
                *pwcCleanedSearchPtr++ = L' ';
            }

            /* Write the operator - single character */
            *pwcCleanedSearchPtr++ = *pwcSearchTextPtr;

            /* Add in a space */
            *pwcCleanedSearchPtr++ = L' ';
        }

        /* Deal with left square brackets '[' */
        else if ( *pwcSearchTextPtr == SRCH_PARSER_CHARACTER_LSQUARE_BRACKET_WCHAR ) {

            wchar_t        pwcBuffer[SRCH_PARSER_MINIMUM_LENGTH + 1] = {L'\0'};

            /* Back up to the start of the previous token in the new line */
            if ( pwcCleanedSearchPtr > pwcCleanedSearch ) {
                
                wchar_t     *pwcStartPtr = NULL;
                wchar_t     *pwcEndPtr = pwcCleanedSearchPtr - 1;

                /* Back up to the end of the previous token in the new line */
                while ( (pwcEndPtr > pwcCleanedSearch) && (*pwcEndPtr == L' ') ) {
                    pwcEndPtr--;
                }
                pwcEndPtr++;
                
                /* Set the place we start from in looking for the start of the previous token */
                pwcStartPtr = pwcEndPtr - 1;

                /* Back up to the start of the previous token in the new line */
                if ( pwcStartPtr > pwcCleanedSearch ) {

                    /* Back up to the start of the previous token in the new line */
                    while ( (pwcStartPtr > pwcCleanedSearch) && (*pwcStartPtr != L' ') ) {
                        pwcStartPtr--;
                    }
            
                    /* Copy the token to the buffer */
                    s_wcsnncpy(pwcBuffer, pwcStartPtr, (pwcEndPtr - pwcStartPtr) + 1);
                }
            }            

            /* Check if the token in the buffer is a function, if it is a function, 
            ** we gather up the components, otherwise we just copy the token
            */
            if ( uiSrchParserGetFunctionIDFromString(pspSrchParser, pwcBuffer) != SRCH_PARSER_INVALID_ID )  {

                unsigned int    uiIndent = 0;

                /* Back up to the end of the previous token in the new line */
                pwcCleanedSearchPtr -= (pwcCleanedSearchPtr == pwcCleanedSearch) ? 0 : 1;
                while ( (pwcCleanedSearchPtr > pwcCleanedSearch) && (*pwcCleanedSearchPtr == L' ') ) {
                    pwcCleanedSearchPtr--;
                }
                pwcCleanedSearchPtr++;


                /* Write everthing up to the right square bracket, but exclude it */
                while ( *pwcSearchTextPtr != L'\0' ) {

                    /* Scoot forward to the next character in the line if this is a space */
                    if ( *pwcSearchTextPtr == L' ' ) {
                        pwcSearchTextPtr++;
                        continue;
                    }                    

                    /* Increment or decrement the indent */
                    if ( *pwcSearchTextPtr == SRCH_PARSER_CHARACTER_LSQUARE_BRACKET_WCHAR ) {
                        uiIndent++;
                    }
                    else if ( *pwcSearchTextPtr == SRCH_PARSER_CHARACTER_RSQUARE_BRACKET_WCHAR ) {
                        uiIndent--;
                    }

                    /* Write the character */
                    *pwcCleanedSearchPtr++ = *pwcSearchTextPtr++;

                    /* Bail if we have reached the end of the indent (hence square brackets) 
                    ** or if we have run into a delimitor token other than square brackets
                    */
/*                     if ( (uiIndent == 0) ||  */
/*                             ((s_wcschr(SRCH_PARSER_CHARACTER_SQUARE_BRACKETS_WSTRING, *pwcSearchTextPtr) == NULL) && ((s_wcschr(SRCH_PARSER_CHARACTER_FULL_LIST_WSTRING, *pwcSearchTextPtr) != NULL) ||  */
/*                             (s_wcschr(SRCH_PARSER_RANGE_LIST_WSTRING, *pwcSearchTextPtr) != NULL) || (s_wcschr(SRCH_PARSER_OPERATOR_FULL_LIST_WSTRING, *pwcSearchTextPtr) != NULL))) ) { */
                    if ( uiIndent == 0 ) {
                        break;
                    }
                }                    

                /* Add in a space */
                *pwcCleanedSearchPtr++ = L' ';
                
                /* We are breaking so we crank back one space to the character before the NULL so that the for() breaks */
                pwcSearchTextPtr--;

            }
            else {
                /* Write the left square bracket - single character */
                *pwcCleanedSearchPtr++ = *pwcSearchTextPtr;
            }
        }

        /* Deal with left brackets '{' */
        else if ( *pwcSearchTextPtr == SRCH_PARSER_CHARACTER_LBRACKET_WCHAR ) {

            /* Add in a space */
            if ( pwcCleanedSearchPtr != pwcCleanedSearch ) {
                *pwcCleanedSearchPtr++ = L' ';
            }

            /* Write the operator - single character */
            *pwcCleanedSearchPtr++ = *pwcSearchTextPtr++;

            /* Scoot forward to the next token in the line */
            while ( *pwcSearchTextPtr == L' ' ) {
                pwcSearchTextPtr++;
            }
            pwcSearchTextPtr--;
        }

        /* Deal with right brackets '}' */
        else if ( *pwcSearchTextPtr == SRCH_PARSER_CHARACTER_RBRACKET_WCHAR ) {

            /* Back up to the end of the previous token in the new line */
            pwcCleanedSearchPtr--;
            while ( (pwcCleanedSearchPtr > pwcCleanedSearch) && (*pwcCleanedSearchPtr == ' ') ) {
                pwcCleanedSearchPtr--;
            }
            pwcCleanedSearchPtr++;


            /* Write the operator - single character */
            *pwcCleanedSearchPtr++ = *pwcSearchTextPtr;
            
            /* Append a space if the next character is not a space */
            if ( *(pwcSearchTextPtr + 1) != ' ' ) {
                *pwcCleanedSearchPtr++ = ' ';
            }
        }

#if defined(SRCH_PARSER_ENABLE_RANGE_TOKEN_COMPACTING)
        /* Deal with all range tokens '=:<>!' */
        else if ( s_wcschr(SRCH_PARSER_RANGE_LIST_WSTRING, *pwcSearchTextPtr) != NULL ) {

            wchar_t     *pwcEndPtr = NULL;
            wchar_t     pwcBuffer[SRCH_PARSER_MINIMUM_LENGTH + 1] = {L'\0'};

            /* Find the end of the operator */
            pwcEndPtr = pwcSearchTextPtr;
            while ( (s_wcschr(SRCH_PARSER_RANGE_LIST_WSTRING, *pwcEndPtr) != NULL) && (*pwcEndPtr != L'\0') ) {
                pwcEndPtr++;
            }

            /* Copy the operator to the buffer */
            s_wcsnncpy(pwcBuffer, pwcSearchTextPtr, (pwcEndPtr - pwcSearchTextPtr) + 1);

            /* Check to see if the token is a range operator, and handle these cases:
            **
            **  title = term
            **  title= term
            **  title =term
            **
            */ 
/*             if ( (uiSrchParserGetRangeIDFromString(pspSrchParser, pwcBuffer) != SRCH_PARSER_INVALID_ID) && */
/*                     ((((pwcSearchTextPtr - pwcSearchText) > 0) && (*(pwcSearchTextPtr - 1) != L' ')) || (((pwcSearchTextPtr - pwcSearchText) > 1) && (*(pwcSearchTextPtr - 2) != L' ') && (*(pwcSearchTextPtr - 1) == L' '))) && */
/*                     (((*pwcEndPtr != L' ') || (*pwcEndPtr == L' ')) && (*(pwcEndPtr + 1) != L' ')) ) { */

            /* Check to see if the token is a range operator, and handle all cases */
            if ( uiSrchParserGetRangeIDFromString(pspSrchParser, pwcBuffer) != SRCH_PARSER_INVALID_ID ) {


                /* Back up to the end of the previous token in the new line */
                pwcCleanedSearchPtr--;
                while ( (pwcCleanedSearchPtr > pwcCleanedSearch) && (*pwcCleanedSearchPtr == L' ') ) {
                    pwcCleanedSearchPtr--;
                }
                pwcCleanedSearchPtr++;

                /* Write the operator - single/multiple character */
                while ( (s_wcschr(SRCH_PARSER_RANGE_LIST_WSTRING, *pwcSearchTextPtr) != NULL) && (*pwcSearchTextPtr != L'\0') ) {
                    *pwcCleanedSearchPtr++ = *pwcSearchTextPtr++;
                }

                /* Scoot forward to the next token in the line */
                while ( *pwcSearchTextPtr == L' ' ) {
                    pwcSearchTextPtr++;
                }
                pwcSearchTextPtr--;
            }
            else {
                /* Write the token - single character */
                *pwcCleanedSearchPtr++ = *pwcSearchTextPtr;
            }
        }
#endif    /* defined(SRCH_PARSER_ENABLE_RANGE_TOKEN_COMPACTING) */

#if defined(SRCH_PARSER_ENABLE_BACKSLASH_ESCAPE)
        /* Deal with backslash '\' */
        else if ( *pwcSearchTextPtr == SRCH_PARSER_CHARACTER_BACK_SLASH_WCHAR ) {

            /* Write the backslash if it does not preceed a special character,
            ** not that we do not deal with wildcards here
            */
            if ( (s_wcschr(SRCH_PARSER_CHARACTER_FULL_LIST_WSTRING, *(pwcSearchTextPtr + 1)) == NULL) && 
                    (s_wcschr(SRCH_PARSER_RANGE_LIST_WSTRING, *(pwcSearchTextPtr + 1)) == NULL) &&
                    (s_wcschr(SRCH_PARSER_OPERATOR_FULL_LIST_WSTRING, *(pwcSearchTextPtr + 1)) == NULL) ) {
                *pwcCleanedSearchPtr++ = *pwcSearchTextPtr;
            }
        }
#endif    /* defined(SRCH_PARSER_ENABLE_BACKSLASH_ESCAPE) */

        /* Deal with wildcards, we dont want any wild cards to follow a multi wildcard */
        else if ( s_wcschr(SRCH_PARSER_WILDCARDS_WSTRING, *pwcSearchTextPtr) != NULL ) {

            /* Check the previous character, copy the multi wildcard if the previous character was not a multi wildcard */
            if ( ((pwcCleanedSearchPtr > pwcCleanedSearch) && (*(pwcSearchTextPtr - 1) != SRCH_PARSER_WILDCARD_MULTI_WCHAR)) 
                    || (pwcCleanedSearchPtr == pwcCleanedSearch) ) {

                /* Write the multi wildcard - single character */
                *pwcCleanedSearchPtr++ = *pwcSearchTextPtr;
            
            }
        }

        /* Replace the ideographic space character (unicode: x3000, utf8: E3 80 80) with a regular space character */
        else if ( (wint_t)*pwcSearchTextPtr == L'\x3000' ) {

            /* Write the space */
            *pwcCleanedSearchPtr++ = L' ';
        }

        else {

            /* Write the character */
            *pwcCleanedSearchPtr++ = *pwcSearchTextPtr;
        }
    }


    /* Terminate the string */
    *pwcCleanedSearchPtr = L'\0';


    /* Get rid of multiple space strings */
    while ( (pwcCleanedSearchPtr = (wchar_t *)s_wcsstr(pwcCleanedSearch, L"  ")) != NULL ) {
        s_memmove(pwcCleanedSearchPtr, pwcCleanedSearchPtr + 1, s_wcslen(pwcCleanedSearchPtr) * sizeof(wchar_t));
    }


    /* Set the return pointer */
    *ppwcCleanedSearch = pwcCleanedSearch;


    return (SRCH_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchParserSubstituteTokens()

    Purpose:    This function run substitutions on the passed search.

    Parameters: pspSrchParser           search parser structure
                pwcSearchText           search text (optional)
                ppwcSubstitutedSearch   Return parameter for the substituted search

    Globals:    none

    Returns:    SRCH error code

*/
static int iSrchParserSubstituteTokens
(
    struct srchParser *pspSrchParser,
    wchar_t *pwcSearchText,
    wchar_t **ppwcSubstitutedSearch
)
{

    int         iError = SRCH_NoError;
    void        *pvUtlSubstitutedTokensSearchWideStringBuffer = NULL;
    wchar_t     *pwcSearchTextCopy = NULL;
    wchar_t     *pwcSearchTextCopyPtr = NULL;
    wchar_t     *pwcSearchTextCopyEndPtr = NULL;
    wchar_t     *pwcToken = NULL;


    ASSERT(pspSrchParser != NULL);
    ASSERT(ppwcSubstitutedSearch != NULL);


    /* Check the search */
    if ( bUtlStringsIsWideStringNULL(pwcSearchText) == true ) {
        return (SRCH_NoError);
    }


    /* Allocate the substituted search string buffer */
    if ( (iError = iUtlWideStringBufferCreate(&pvUtlSubstitutedTokensSearchWideStringBuffer)) != UTL_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to create a wide string buffer, utl error: %d.", iError); 
        return (SRCH_MemError);
    }    


    /* Make a duplicate copy of the search, we destroy this one */
    if ( (pwcSearchTextCopy = s_wcsdup(pwcSearchText)) == NULL ) {
        return (SRCH_MemError);
    }

    /* Set the search text pointer */
    pwcSearchTextCopyPtr = pwcSearchTextCopy;
    pwcSearchTextCopyEndPtr = pwcSearchTextCopy + wcslen(pwcSearchTextCopy);
    
    /* Loop while we have not reached the end of the search text, 
    ** we use this approach as opposed to s_wcstok() because we 
    ** need to handle modifiers with spaces in them, namely
    ** {inclusion_filter:...} and {exclusion_filter:...}
    */
    while ( pwcSearchTextCopyPtr < pwcSearchTextCopyEndPtr ) {

        /* Set the current token pointer */
        pwcToken = pwcSearchTextCopyPtr;

        /* Get the end of the token, mind the modifiers, they may contain spaces */
        if ( *pwcSearchTextCopyPtr == SRCH_PARSER_CHARACTER_LBRACKET_WCHAR ) {

            /* Advance the search text pointer past the modifier */
            pwcSearchTextCopyPtr = s_wcschr(pwcSearchTextCopyPtr, SRCH_PARSER_CHARACTER_RBRACKET_WCHAR) + 1;
        }
        else {
            /* Advance the search text pointer past the space */
            pwcSearchTextCopyPtr = s_wcschr(pwcSearchTextCopyPtr, L' ');
        }

        /* NULL terminate the token and advance the search text pointer past the NULL, otherwise end it */
        if ( pwcSearchTextCopyPtr != NULL ) {
            *pwcSearchTextCopyPtr = L'\0';
            pwcSearchTextCopyPtr++;
        }
        else {
            pwcSearchTextCopyPtr = pwcSearchTextCopyEndPtr;
        }


        iUtlLogDebug(UTL_LOG_CONTEXT, "iSrchParserSubstituteTokens - pwcToken: [%ls].", pwcToken);


        /* Append the token to the substituted search string */
        iUtlWideStringBufferAppend(pvUtlSubstitutedTokensSearchWideStringBuffer, L" ");
        iUtlWideStringBufferAppend(pvUtlSubstitutedTokensSearchWideStringBuffer, pwcToken);
    }



    /* Bail label */
    bailFromiSrchParserSubstituteTokens:


    s_free(pwcSearchTextCopy);

    /* Handle the error */
    if ( iError == SRCH_NoError ) {
        
        /* Get the string from the checked syntax search string buffer, setting the return pointer */
        iUtlWideStringBufferGetString(pvUtlSubstitutedTokensSearchWideStringBuffer, ppwcSubstitutedSearch);

        /* Free the checked syntax search string buffer, note that we don't release the string */
        iUtlWideStringBufferFree(pvUtlSubstitutedTokensSearchWideStringBuffer, false);
        pvUtlSubstitutedTokensSearchWideStringBuffer = NULL;

        /* Trim the string */
        iUtlStringsTrimWideString(*ppwcSubstitutedSearch);
    }
    else {

        /* Free the checked syntax search string buffer, note that we do release the string */
        iUtlWideStringBufferFree(pvUtlSubstitutedTokensSearchWideStringBuffer, true);
        pvUtlSubstitutedTokensSearchWideStringBuffer = NULL;
    }


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchParserCheckSyntax()

    Purpose:    This function checks the syntax of the passed search.

    Parameters: pspSrchParser               search parser structure
                pwcSearchText               search text (optional)
                ppwcCheckedSyntaxSearch     Return parameter for the checked syntax search

    Globals:    none

    Returns:    SRCH error code

*/
static int iSrchParserCheckSyntax
(
    struct srchParser *pspSrchParser,
    wchar_t *pwcSearchText,
    wchar_t **ppwcCheckedSyntaxSearch
)
{

    int         iError = SRCH_NoError;
    void        *pvUtlCheckedSyntaxSearchWideStringBuffer = NULL;
    wchar_t     *pwcSearchTextCopy = NULL;
    wchar_t     *pwcSearchTextCopyPtr = NULL;
    wchar_t     *pwcSearchTextCopyEndPtr = NULL;
    wchar_t     *pwcToken = NULL;


    ASSERT(pspSrchParser != NULL);
    ASSERT(ppwcCheckedSyntaxSearch != NULL);


    /* Check the search */
    if ( bUtlStringsIsWideStringNULL(pwcSearchText) == true ) {
        return (SRCH_NoError);
    }


    /* Check balanced parenthesis */
#if !defined(SRCH_PARSER_ENABLE_PARENTHESIS_ERROR_SKIPPING)
    {
        wchar_t         *pwcSearchTextPtr = NULL;
        unsigned int    uiParenthesisCount = 0;
    
        /* Check that there are the correct number of parens */
        for ( pwcSearchTextPtr = pwcSearchText, uiParenthesisCount = 0; *pwcSearchTextPtr != L'\0'; pwcSearchTextPtr++ ) {
            if ( s_wcschr(SRCH_PARSER_OPERATOR_RPAREN_WSTRING, *pwcSearchTextPtr) != NULL ) {
                uiParenthesisCount++;
            }
            else if ( s_wcschr(SRCH_PARSER_OPERATOR_LPAREN_WSTRING, *pwcSearchTextPtr) != NULL ) {
                uiParenthesisCount--;
            }
        }
    
        /* If there are an inconsistent number of parens */
        if ( uiParenthesisCount != 0 ) {
            return (SRCH_ParserInvalidBracket);
        }
    }
#endif /* !defined(SRCH_PARSER_ENABLE_PARENTHESIS_ERROR_SKIPPING) */


    /* Check balanced quotes */
#if !defined(SRCH_PARSER_ENABLE_QUOTE_ERROR_SKIPPING)
    {

        wchar_t         *pwcSearchTextPtr = NULL;
        unsigned int    uiDoubleQuoteCount = 0;

        /* Check that there are the correct number of quotes */
        for ( pwcSearchTextPtr = pwcSearchText, uiDoubleQuoteCount = 0; *pwcSearchTextPtr != L'\0'; pwcSearchTextPtr++ ) {
            if ( *pwcSearchTextPtr == SRCH_PARSER_CHARACTER_DOUBLE_QUOTE_WCHAR ) {
                uiDoubleQuoteCount++;
            }
        }
    
        /* If there are an inconsistent number of quotes */
        if ( (uiDoubleQuoteCount % 2) != 0 ) {
            return (SRCH_ParserInvalidQuote);
        }
    }
#endif /* !defined(SRCH_PARSER_ENABLE_QUOTE_ERROR_SKIPPING) */


    /* Allocate the checked syntax search string buffer */
    if ( (iError = iUtlWideStringBufferCreate(&pvUtlCheckedSyntaxSearchWideStringBuffer)) != UTL_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to create a wide string buffer, utl error: %d.", iError); 
        return (SRCH_MemError);
    }    


    /* Check that there are no wildcards on their own */


    /* Make a duplicate copy of the search, we destroy this one */
    if ( (pwcSearchTextCopy = s_wcsdup(pwcSearchText)) == NULL ) {
        return (SRCH_MemError);
    }

    /* Set the search text pointer */
    pwcSearchTextCopyPtr = pwcSearchTextCopy;
    pwcSearchTextCopyEndPtr = pwcSearchTextCopy + wcslen(pwcSearchTextCopy);
    
    /* Loop while we have not reached the end of the search text, 
    ** we use this apprach as opposed to s_wcstok() because we 
    ** need to handle modifiers with spaces in them, namely
    ** {inclusion_filter:...} and {exclusion_filter:...}
    */
    while ( pwcSearchTextCopyPtr < pwcSearchTextCopyEndPtr ) {

        /* Set the current token pointer */
        pwcToken = pwcSearchTextCopyPtr;

        /* Get the end of the token, mind the modifiers, they may contain spaces */
        if ( *pwcSearchTextCopyPtr == SRCH_PARSER_CHARACTER_LBRACKET_WCHAR ) {

            /* Advance the search text pointer past the modifier */
            pwcSearchTextCopyPtr = s_wcschr(pwcSearchTextCopyPtr, SRCH_PARSER_CHARACTER_RBRACKET_WCHAR) + 1;
        }
        else {
            /* Advance the search text pointer past the space */
            pwcSearchTextCopyPtr = s_wcschr(pwcSearchTextCopyPtr, L' ');
        }

        /* NULL terminate the token and advance the search text pointer past the NULL, otherwise end it */
        if ( pwcSearchTextCopyPtr != NULL ) {
            *pwcSearchTextCopyPtr = L'\0';
            pwcSearchTextCopyPtr++;
        }
        else {
            pwcSearchTextCopyPtr = pwcSearchTextCopyEndPtr;
        }


        iUtlLogDebug(UTL_LOG_CONTEXT, "iSrchParserCheckSyntax - pwcToken: [%ls].", pwcToken);


        /* This will never match, but it makes the defines below a little easier to read */        
        if ( s_wcscmp(pwcToken, L" ") == 0 ) {
            ;
        }

#if defined(SRCH_PARSER_ENABLE_MULTI_WILDCARD)
        /* Check for a rogue multi wildcard */
        else if ( s_wcscmp(pwcToken, SRCH_PARSER_WILDCARD_MULTI_WSTRING) == 0 ) {
#if !defined(SRCH_PARSER_ENABLE_WILDCARD_ERROR_SKIPPING)
            iError = SRCH_ParserInvalidWildCard;
            goto bailFromiSrchParserCheckSyntax;
#endif /* !defined(SRCH_PARSER_ENABLE_WILDCARD_ERROR_SKIPPING) */
        }
#endif /* defined(SRCH_PARSER_ENABLE_MULTI_WILDCARD) */

#if defined(SRCH_PARSER_ENABLE_SINGLE_WILDCARD)
        /* Check for a rogue single wildcard */
        else if ( s_wcscmp(pwcToken, SRCH_PARSER_WILDCARD_SINGLE_WSTRING) == 0 ) {
#if !defined(SRCH_PARSER_ENABLE_WILDCARD_ERROR_SKIPPING)
            iError = SRCH_ParserInvalidWildCard;
            goto bailFromiSrchParserCheckSyntax;
#endif /* !defined(SRCH_PARSER_ENABLE_WILDCARD_ERROR_SKIPPING) */
        }
#endif /* defined(SRCH_PARSER_ENABLE_SINGLE_WILDCARD) */

#if defined(SRCH_PARSER_ENABLE_ALPHA_WILDCARD)
        /* Check for a rogue alpha wildcard */
        else if ( s_wcscmp(pwcToken, SRCH_PARSER_WILDCARD_ALPHA_WSTRING) == 0 ) {
#if !defined(SRCH_PARSER_ENABLE_WILDCARD_ERROR_SKIPPING)
            iError = SRCH_ParserInvalidWildCard;
            goto bailFromiSrchParserCheckSyntax;
#endif /* !defined(SRCH_PARSER_ENABLE_WILDCARD_ERROR_SKIPPING) */
        }
#endif /* defined(SRCH_PARSER_ENABLE_ALPHA_WILDCARD) */

#if defined(SRCH_PARSER_ENABLE_NUMERIC_WILDCARD)
        /* Check for a rogue numeric wildcard */
        else if ( s_wcscmp(pwcToken, SRCH_PARSER_WILDCARD_NUMERIC_WSTRING) == 0 ) {
#if !defined(SRCH_PARSER_ENABLE_WILDCARD_ERROR_SKIPPING)
            iError = SRCH_ParserInvalidWildCard;
            goto bailFromiSrchParserCheckSyntax;
#endif /* !defined(SRCH_PARSER_ENABLE_WILDCARD_ERROR_SKIPPING) */
        }
#endif /* defined(SRCH_PARSER_ENABLE_NUMERIC_WILDCARD) */

        
        /* Check for a rogue left bracket */
        else if ( s_wcscmp(pwcToken, SRCH_PARSER_CHARACTER_LBRACKET_WSTRING) == 0 ) {
#if !defined(SRCH_PARSER_ENABLE_BRACKET_ERROR_SKIPPING)
            iError = SRCH_ParserInvalidToken;
            goto bailFromiSrchParserCheckSyntax;
#endif /* !defined(SRCH_PARSER_ENABLE_BRACKET_ERROR_SKIPPING) */
        }

        /* Check for a rogue right bracket */
        else if ( s_wcscmp(pwcToken, SRCH_PARSER_CHARACTER_RBRACKET_WSTRING) == 0 ) {
#if !defined(SRCH_PARSER_ENABLE_BRACKET_ERROR_SKIPPING)
            iError = SRCH_ParserInvalidToken;
            goto bailFromiSrchParserCheckSyntax;
#endif /* !defined(SRCH_PARSER_ENABLE_BRACKET_ERROR_SKIPPING) */
        }


        /* Left and right square brackets are allowed on their own in searches, it just means that they are not used as part of a function */
        else if ( s_wcsncmp(pwcToken, SRCH_PARSER_CHARACTER_LSQUARE_BRACKET_WSTRING, s_wcslen(SRCH_PARSER_CHARACTER_LSQUARE_BRACKET_WSTRING)) == 0 ) {
#if defined(SRCH_PARSER_ENABLE_BRACKET_ERROR_SKIPPING)
            /* Append the token to the checked syntax search string */
            iUtlWideStringBufferAppend(pvUtlCheckedSyntaxSearchWideStringBuffer, L" ");
            iUtlWideStringBufferAppend(pvUtlCheckedSyntaxSearchWideStringBuffer, pwcToken);
#else /* defined(SRCH_PARSER_ENABLE_BRACKET_ERROR_SKIPPING) */
            iError = SRCH_ParserInvalidToken;
            goto bailFromiSrchParserCheckSyntax;
#endif /* defined(SRCH_PARSER_ENABLE_BRACKET_ERROR_SKIPPING) */
        }

        else if ( s_wcsncmp(pwcToken, SRCH_PARSER_CHARACTER_RSQUARE_BRACKET_WSTRING, s_wcslen(SRCH_PARSER_CHARACTER_RSQUARE_BRACKET_WSTRING)) == 0 ) {
#if defined(SRCH_PARSER_ENABLE_BRACKET_ERROR_SKIPPING)
            /* Append the token to the checked syntax search string */
            iUtlWideStringBufferAppend(pvUtlCheckedSyntaxSearchWideStringBuffer, L" ");
            iUtlWideStringBufferAppend(pvUtlCheckedSyntaxSearchWideStringBuffer, pwcToken);
#else /* defined(SRCH_PARSER_ENABLE_BRACKET_ERROR_SKIPPING) */
            iError = SRCH_ParserInvalidToken;
            goto bailFromiSrchParserCheckSyntax;
#endif /* defined(SRCH_PARSER_ENABLE_BRACKET_ERROR_SKIPPING) */
        }

        else {

            /* Append the token to the checked syntax search string */
            iUtlWideStringBufferAppend(pvUtlCheckedSyntaxSearchWideStringBuffer, L" ");
            iUtlWideStringBufferAppend(pvUtlCheckedSyntaxSearchWideStringBuffer, pwcToken);
        }
    }



    /* Bail label */
    bailFromiSrchParserCheckSyntax:


    s_free(pwcSearchTextCopy);

    /* Handle the error */
    if ( iError == SRCH_NoError ) {
        
        /* Get the string from the checked syntax search string buffer, setting the return pointer */
        iUtlWideStringBufferGetString(pvUtlCheckedSyntaxSearchWideStringBuffer, ppwcCheckedSyntaxSearch);

        /* Free the checked syntax search string buffer, note that we don't release the string */
        iUtlWideStringBufferFree(pvUtlCheckedSyntaxSearchWideStringBuffer, false);
        pvUtlCheckedSyntaxSearchWideStringBuffer = NULL;

        /* Trim the string */
        iUtlStringsTrimWideString(*ppwcCheckedSyntaxSearch);
    }
    else {

        /* Free the checked syntax search string buffer, note that we do release the string */
        iUtlWideStringBufferFree(pvUtlCheckedSyntaxSearchWideStringBuffer, true);
        pvUtlCheckedSyntaxSearchWideStringBuffer = NULL;
    }


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchParserExtractModifiers()

    Purpose:    This function performs any extraction on the passed search.
                Currently we only extract the search modifiers.

    Parameters: pspSrchParser           search parser structure
                pwcSearchText           search text (optional)
                ppwcExtractedSearch     Return parameter for the extracted search


    Globals:    none

    Returns:    SRCH error code

*/
static int iSrchParserExtractModifiers
(
    struct srchParser *pspSrchParser,
    wchar_t *pwcSearchText,
    wchar_t **ppwcExtractedSearch
)
{

    int             iError = SRCH_NoError;
    wchar_t         *pwcSearchTextCopy = NULL;
    wchar_t         *pwcSearchTextCopyPtr = NULL;
    wchar_t         *pwcSearchTextCopyEndPtr = NULL;
    void            *pvUtlExtractedSearchWideStringBuffer = NULL;
    unsigned int    uiTokenID = SRCH_PARSER_INVALID_ID;
    wchar_t         *pwcToken = NULL;
    wchar_t         *pwcTokenPtr = NULL;
    wchar_t         pwcScanfFormat[UTL_STRING_SCANF_FORMAT_LENGTH + 1] = {L'\0'};
    wchar_t         pwcRange[SRCH_PARSER_MINIMUM_LENGTH + 1] = {L'\0'};
    wchar_t         pwcValue[SRCH_PARSER_MINIMUM_LENGTH + 1] = {L'\0'};
    boolean         bAllDigits = false;
    unsigned long   ulAnsiDate = 0;


    ASSERT(pspSrchParser != NULL);
    ASSERT(ppwcExtractedSearch != NULL);


    /* Check the search */
    if ( bUtlStringsIsWideStringNULL(pwcSearchText) == true ) {
        return (SRCH_NoError);
    }


    /* Allocate the extracted search string buffer */
    if ( (iError = iUtlWideStringBufferCreate(&pvUtlExtractedSearchWideStringBuffer)) != UTL_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to create a wide string buffer, utl error: %d.", iError); 
        return (SRCH_MemError);
    }    


    /* Make a duplicate copy of the search, we destroy this one */
    if ( (pwcSearchTextCopy = s_wcsdup(pwcSearchText)) == NULL ) {
        iError = SRCH_MemError;
        goto bailFromiSrchParserExtractModifiers;
    }

    /* Set the search text pointer */
    pwcSearchTextCopyPtr = pwcSearchTextCopy;
    pwcSearchTextCopyEndPtr = pwcSearchTextCopy + wcslen(pwcSearchTextCopy);
    
    /* Loop while we have not reached the end of the search text, 
    ** we use this apprach as opposed to s_wcstok() because we 
    ** need to handle modifiers with spaces in them, namely
    ** {inclusion_filter:...} and {exclusion_filter:...}
    */
    while ( pwcSearchTextCopyPtr < pwcSearchTextCopyEndPtr ) {

        /* Set the current token pointer */
        pwcToken = pwcSearchTextCopyPtr;

        /* Get the end of the token, mind the modifiers, they may contain spaces */
        if ( *pwcSearchTextCopyPtr == SRCH_PARSER_CHARACTER_LBRACKET_WCHAR ) {

            /* Advance the search text pointer past the modifier */
            pwcSearchTextCopyPtr = s_wcschr(pwcSearchTextCopyPtr, SRCH_PARSER_CHARACTER_RBRACKET_WCHAR) + 1;
        }
        else {
            /* Advance the search text pointer past the space */
            pwcSearchTextCopyPtr = s_wcschr(pwcSearchTextCopyPtr, L' ');
        }

        /* NULL terminate the token and advance the search text pointer past the NULL, otherwise end it */
        if ( pwcSearchTextCopyPtr != NULL ) {
            *pwcSearchTextCopyPtr = L'\0';
            pwcSearchTextCopyPtr++;
        }
        else {
            pwcSearchTextCopyPtr = pwcSearchTextCopyEndPtr;
        }


        /* Get the modifier ID for this token */
        uiTokenID = uiSrchParserGetModifierIDFromString(pspSrchParser, pwcToken);
        
        iUtlLogDebug(UTL_LOG_CONTEXT, "iSrchParserExtractModifiers - pwcToken: [%ls], uiTokenID: [%u].", pwcToken, uiTokenID);

        /* {search_results:return} */
        if ( uiTokenID == SRCH_PARSER_MODIFIER_SEARCH_RESULTS_RETURN_ID ) {
            pspSrchParser->uiSearchResultsID = SRCH_PARSER_MODIFIER_SEARCH_RESULTS_RETURN_ID;
        }
        
        /* {search_results:suppress} */
        else if ( uiTokenID == SRCH_PARSER_MODIFIER_SEARCH_RESULTS_SUPPRESS_ID ) {
            pspSrchParser->uiSearchResultsID = SRCH_PARSER_MODIFIER_SEARCH_RESULTS_SUPPRESS_ID;
        }
    
        /* {search_report:return} */
        else if ( uiTokenID == SRCH_PARSER_MODIFIER_SEARCH_REPORT_RETURN_ID ) {
            pspSrchParser->uiSearchReportID = SRCH_PARSER_MODIFIER_SEARCH_REPORT_RETURN_ID;
        }
    
        /* {search_report:suppress} */
        else if ( uiTokenID == SRCH_PARSER_MODIFIER_SEARCH_REPORT_SUPPRESS_ID ) {
            pspSrchParser->uiSearchReportID = SRCH_PARSER_MODIFIER_SEARCH_REPORT_SUPPRESS_ID;
        }
    
        /* {search_cache:enable} */
        else if ( uiTokenID == SRCH_PARSER_MODIFIER_SEARCH_CACHE_ENABLE_ID ) {
            pspSrchParser->uiSearchCacheID = SRCH_PARSER_MODIFIER_SEARCH_CACHE_ENABLE_ID;
        }
    
        /* {search_cache:disable} */
        else if ( uiTokenID == SRCH_PARSER_MODIFIER_SEARCH_CACHE_DISABLE_ID ) {
            pspSrchParser->uiSearchCacheID = SRCH_PARSER_MODIFIER_SEARCH_CACHE_DISABLE_ID;
        }
    
        /* {debug:enable} */
        else if ( uiTokenID == SRCH_PARSER_MODIFIER_DEBUG_ENABLE_ID ) {
            pspSrchParser->uiDebugID = SRCH_PARSER_MODIFIER_DEBUG_ENABLE_ID;
        }
    
        /* {debug:disable} */
        else if ( uiTokenID == SRCH_PARSER_MODIFIER_DEBUG_DISABLE_ID ) {
            pspSrchParser->uiDebugID = SRCH_PARSER_MODIFIER_DEBUG_DISABLE_ID;
        }
                
        /* {early_completion:enable} */
        else if ( uiTokenID == SRCH_PARSER_MODIFIER_EARLY_COMPLETION_ENABLE_ID ) {
            pspSrchParser->uiEarlyCompletionID = SRCH_PARSER_MODIFIER_EARLY_COMPLETION_ENABLE_ID;
        }
    
        /* {early_completion:disable} */
        else if ( uiTokenID == SRCH_PARSER_MODIFIER_EARLY_COMPLETION_DISABLE_ID ) {
            pspSrchParser->uiEarlyCompletionID = SRCH_PARSER_MODIFIER_EARLY_COMPLETION_DISABLE_ID;
        }
                
        /* {boolean_operator:or} */
        else if ( uiTokenID == SRCH_PARSER_MODIFIER_BOOLEAN_OPERATOR_OR_ID ) {
            pspSrchParser->uiBooleanOperatorID = SRCH_PARSER_MODIFIER_BOOLEAN_OPERATOR_OR_ID;
        }
                
        /* {boolean_operator:ior} */
        else if ( uiTokenID == SRCH_PARSER_MODIFIER_BOOLEAN_OPERATOR_IOR_ID ) {
            pspSrchParser->uiBooleanOperatorID = SRCH_PARSER_MODIFIER_BOOLEAN_OPERATOR_IOR_ID;
        }
                
        /* {boolean_operator:xor} */
        else if ( uiTokenID == SRCH_PARSER_MODIFIER_BOOLEAN_OPERATOR_XOR_ID ) {
            pspSrchParser->uiBooleanOperatorID = SRCH_PARSER_MODIFIER_BOOLEAN_OPERATOR_XOR_ID;
        }
                
        /* {boolean_operator:and} */
        else if ( uiTokenID == SRCH_PARSER_MODIFIER_BOOLEAN_OPERATOR_AND_ID ) {
            pspSrchParser->uiBooleanOperatorID = SRCH_PARSER_MODIFIER_BOOLEAN_OPERATOR_AND_ID;
        }
                
        /* {boolean_operator:adj} */
        else if ( uiTokenID == SRCH_PARSER_MODIFIER_BOOLEAN_OPERATOR_ADJ_ID ) {
            pspSrchParser->uiBooleanOperatorID = SRCH_PARSER_MODIFIER_BOOLEAN_OPERATOR_ADJ_ID;
        }
                
        /* {boolean_operator:near} */
        else if ( uiTokenID == SRCH_PARSER_MODIFIER_BOOLEAN_OPERATOR_NEAR_ID ) {
            pspSrchParser->uiBooleanOperatorID = SRCH_PARSER_MODIFIER_BOOLEAN_OPERATOR_NEAR_ID;
        }
                
        /* {boolean_operation:relaxed} */
        else if ( uiTokenID == SRCH_PARSER_MODIFIER_BOOLEAN_OPERATION_RELAXED_ID ) {
            pspSrchParser->uiBooleanOperationID = SRCH_PARSER_MODIFIER_BOOLEAN_OPERATION_RELAXED_ID;
        }
                
        /* {boolean_operation:strict} */
        else if ( uiTokenID == SRCH_PARSER_MODIFIER_BOOLEAN_OPERATION_STRICT_ID ) {
            pspSrchParser->uiBooleanOperationID = SRCH_PARSER_MODIFIER_BOOLEAN_OPERATION_STRICT_ID;
        }
                
        /* {operator_case:any} */
        else if ( uiTokenID == SRCH_PARSER_MODIFIER_OPERATOR_CASE_ANY_ID ) {
            pspSrchParser->uiOperatorCaseID = SRCH_PARSER_MODIFIER_OPERATOR_CASE_ANY_ID;
        }
                
        /* {operator_case:upper} */
        else if ( uiTokenID == SRCH_PARSER_MODIFIER_OPERATOR_CASE_UPPER_ID ) {
            pspSrchParser->uiOperatorCaseID = SRCH_PARSER_MODIFIER_OPERATOR_CASE_UPPER_ID;
        }
                
        /* {operator_case:lower} */
        else if ( uiTokenID == SRCH_PARSER_MODIFIER_OPERATOR_CASE_LOWER_ID ) {
            pspSrchParser->uiOperatorCaseID = SRCH_PARSER_MODIFIER_OPERATOR_CASE_LOWER_ID;
        }
                
        /* {term_case:drop} */
        else if ( uiTokenID == SRCH_PARSER_MODIFIER_TERM_CASE_DROP_ID ) {
            pspSrchParser->uiTermCaseID = SRCH_PARSER_MODIFIER_TERM_CASE_DROP_ID;
        }
                
        /* {term_case:keep} */
        else if ( uiTokenID == SRCH_PARSER_MODIFIER_TERM_CASE_KEEP_ID ) {
            pspSrchParser->uiTermCaseID = SRCH_PARSER_MODIFIER_TERM_CASE_KEEP_ID;
        }
            
        /* {frequent_terms:drop} */
        else if ( uiTokenID == SRCH_PARSER_MODIFIER_FREQUENT_TERMS_DROP_ID ) {
            pspSrchParser->uiFrequentTermsID = SRCH_PARSER_MODIFIER_FREQUENT_TERMS_DROP_ID;
        }
                
        /* {frequent_terms:keep} */
        else if ( uiTokenID == SRCH_PARSER_MODIFIER_FREQUENT_TERMS_KEEP_ID ) {
            pspSrchParser->uiFrequentTermsID = SRCH_PARSER_MODIFIER_FREQUENT_TERMS_KEEP_ID;
        }
                
        /* {search_type:boolean} */
        else if ( uiTokenID == SRCH_PARSER_MODIFIER_SEARCH_TYPE_BOOLEAN_ID ) {
            pspSrchParser->uiSearchTypeID = SRCH_PARSER_MODIFIER_SEARCH_TYPE_BOOLEAN_ID;
        }
    
        /* {search_type:freetext} */
        else if ( uiTokenID == SRCH_PARSER_MODIFIER_SEARCH_TYPE_FREETEXT_ID ) {
            pspSrchParser->uiSearchTypeID = SRCH_PARSER_MODIFIER_SEARCH_TYPE_FREETEXT_ID;
        }
    
        /* {date#...} */
        else if ( uiTokenID == SRCH_PARSER_MODIFIER_DATE_ID ) {

            struct srchParserNumber     *pspnSrchParserNumberDatesPtr = NULL;

            /* Scan the date modifier for the range and the date, for example:
            **
            **  {date>1900}
            **  {date=1990}
            **  {date>1990}
            **  {date>=1990}
            **  {date<1990}
            **  {date<=1990}
            **  {date!=1990}
            **  
            ** The date modifier can be repeated multiple times, for example:
            **
            **  {date>=19900000} {date<=20000000}
            **
            ** or:
            **
            **  {date>=19900000,<=20000000}
            **
            ** This is an ansi date in the format:
            **
            **  YYYYMMDDHHMMSS
            **  YYYYMMDDHHMM
            **  YYYYMMDDHH
            **  YYYYMMDD
            **  YYYYMM
            **  YYYY
            **
            ** For example:
            **
            **  2001            -> 2001
            **  200112          -> December 2001
            **  20011231        -> 31 December 2001
            **  20011231100000  -> 31 December 2001, 10 am
            **
            ** The date numbers can be colon separated for clarity:
            **
            **  2001:12:31:10:00:00
            **  2001:12:31::10:00:00
            **
            ** The date will always be normalized into a date, as follows:
            **
            **  YYYYMMDDHHMMSS
            ** 
            */
            
            /* Create the date format */
            swprintf(pwcScanfFormat, UTL_STRING_SCANF_FORMAT_LENGTH + 1, L"%%%dl[%ls]%%%dl[^%ls]%ls", SRCH_PARSER_MINIMUM_LENGTH, 
                    SRCH_PARSER_RANGE_MODIFIER_LIST_WSTRING, SRCH_PARSER_MINIMUM_LENGTH, SRCH_PARSER_CHARACTER_RBRACKET_WSTRING, 
                    SRCH_PARSER_CHARACTER_RBRACKET_WSTRING);

            /* Erase the trailing '}' */
            if ( (pwcTokenPtr = s_wcschr(pwcToken, SRCH_PARSER_CHARACTER_RBRACKET_WCHAR)) != NULL ) {
                *pwcTokenPtr = L'\0';
            }

            /* Get a pointer to the start of the dates */
            pwcTokenPtr = pwcToken + s_wcslen(SRCH_PARSER_MODIFIER_DATE_WSTRING);

            /* Add the dates if there are any */
            if ( s_wcslen(pwcTokenPtr) > 0 ) {
            
                wchar_t     *pwcDatePtr = NULL;
                wchar_t     *pwcDateWcstokPtr = NULL;

                /* Loop parsing the dates */
                for ( pwcDatePtr = s_wcstok(pwcTokenPtr, SRCH_PARSER_CHARACTER_COMMA_WSTRING, &pwcDateWcstokPtr); pwcDatePtr != NULL; 
                        pwcDatePtr = s_wcstok(NULL, SRCH_PARSER_CHARACTER_COMMA_WSTRING, &pwcDateWcstokPtr) ) {
                
                    /* Scan for the date */
                    if ( swscanf(pwcDatePtr, pwcScanfFormat, pwcRange, pwcValue) != 2 ) {
                        iError = SRCH_ParserInvalidDate;
                        goto bailFromiSrchParserExtractModifiers;
                    }
        
                    /* Remove separator characters from the value */
                    iUtlStringsRemoveCharactersFromWideString(pwcValue, SRCH_PARSER_MODIFIER_DATE_SEPARATORS_WSTRING);
        
                    /* Check if the value is all digits */
                    for ( pwcTokenPtr = pwcValue, bAllDigits = true; *pwcTokenPtr != L'\0'; pwcTokenPtr++ ) {
                        if ( iswdigit(*pwcTokenPtr) == 0 ) {
                            bAllDigits = false;
                            break;
                        }
                    }
                    
                    /* If the date value is all digits then we validate and normalize it,
                    ** if it is not all digits then we get the ansi date from it
                    */
                    if ( bAllDigits == true ) {
                        /* Validate and normalize the ansi date */
                        if ( iUtlDateValidateWideAnsiDate(pwcValue, &ulAnsiDate) != UTL_NoError ) {
                            iError = SRCH_ParserInvalidDate;
                            goto bailFromiSrchParserExtractModifiers;
                        }
                    }
                    else {
                        /* Get the ansi date from the date */
                        if ( iSrchParserGetAnsiDateFromDateName(pspSrchParser, pwcRange, pwcValue, &ulAnsiDate) != SRCH_NoError ) {
                            iError = SRCH_ParserInvalidDate;
                            goto bailFromiSrchParserExtractModifiers;
                        }
                    }
        
        
                    /* Extend the search parser dates */
                    if ( (pspnSrchParserNumberDatesPtr = (struct srchParserNumber *)s_realloc(pspSrchParser->pspnSrchParserNumberDates, 
                            (size_t)(sizeof(struct srchParserNumber) * (pspSrchParser->uiSrchParserNumberDatesLength + 1)))) == NULL ) {
                        iError = SRCH_MemError;
                        goto bailFromiSrchParserExtractModifiers;
                    }
                    /* Hand over the pointer */
                    pspSrchParser->pspnSrchParserNumberDates = pspnSrchParserNumberDatesPtr;
        
                    /* Dereference for convenience, clear the newly allocated area and increment the array length */
                    pspnSrchParserNumberDatesPtr = pspSrchParser->pspnSrchParserNumberDates + pspSrchParser->uiSrchParserNumberDatesLength;
                    s_memset(pspnSrchParserNumberDatesPtr, 0, sizeof(struct srchParserNumber));
                    pspSrchParser->uiSrchParserNumberDatesLength++;
        
                    /* Add the date and the range ID - note that we duplicate the normalized value */
                    pspnSrchParserNumberDatesPtr->ulNumber = ulAnsiDate;
                    if ( (pspnSrchParserNumberDatesPtr->uiRangeID = uiSrchParserGetRangeIDFromString(pspSrchParser, pwcRange)) == SRCH_PARSER_INVALID_ID ) {
                        iError = SRCH_ParserInvalidRange;
                        goto bailFromiSrchParserExtractModifiers;
                    }
                }
            }
        }
        
        /* {unfielded_search_field_names:...} */
        else if ( uiTokenID == SRCH_PARSER_MODIFIER_UNFIELDED_SEARCH_FIELD_NAMES_ID ) {

            /* Erase the trailing '}' */
            if ( (pwcTokenPtr = s_wcsstr(pwcToken, SRCH_PARSER_CHARACTER_RBRACKET_WSTRING)) != NULL ) {
                *pwcTokenPtr = L'\0';
            }

            /* Get a pointer to the colon */
            if ( (pwcTokenPtr = s_wcsstr(pwcToken, SRCH_PARSER_RANGE_COLON_WSTRING)) == NULL ) {
                iError = SRCH_ParserInvalidExclusionFilter;
                goto bailFromiSrchParserExtractModifiers;
            }

            /* Add the unfielded search field names if there is anything there */
            if ( s_wcslen(pwcTokenPtr + 1) > 0 ) {

                /* Free the unfielded search field names if it is already set */
                s_free(pspSrchParser->pwcUnfieldedSearchFieldNames);
    
                /* Add the unfielded search field names */
                if ( (pspSrchParser->pwcUnfieldedSearchFieldNames = s_wcsdup(pwcTokenPtr + 1)) == NULL ) {
                    iError = SRCH_MemError;
                    goto bailFromiSrchParserExtractModifiers;
                }

            }
        }

        /* {sort:default} */
        else if ( uiTokenID == SRCH_PARSER_MODIFIER_SORT_DEFAULT_ID ) {

            /* Free the sort field name if it is already set */
            s_free(pspSrchParser->pwcSortFieldName);

            /* Set the sort order ID */
            pspSrchParser->uiSortOrderID = SRCH_PARSER_SORT_ORDER_DEFAULT_ID;
        }                    
        
        /* {sort:none} */
        else if ( uiTokenID == SRCH_PARSER_MODIFIER_SORT_NONE_ID ) {

            /* Free the sort field name if it is already set */
            s_free(pspSrchParser->pwcSortFieldName);

            /* Set the sort order ID */
            pspSrchParser->uiSortOrderID = SRCH_PARSER_SORT_ORDER_NONE_ID;
        }                    
        
        /* {sort:...:...} */
        else if ( uiTokenID == SRCH_PARSER_MODIFIER_SORT_ID ) {

            wchar_t         pwcSortFieldName[SRCH_PARSER_MINIMUM_LENGTH + 1] = {L'\0'};
            wchar_t         *pwcSortFieldNamePtr = NULL;
            unsigned int    uiSortFieldNameID = SRCH_PARSER_INVALID_ID;
            wchar_t         pwcSortOrder[SRCH_PARSER_MINIMUM_LENGTH + 1] = {L'\0'};

            /* Scan the sort modifier for the field name and the sort order
            ** for example:
            **
            **  {sort:relevance:asc}        -> by relevance, ascending (internal field)
            **  {sort:relevance:desc}       -> by relevance, descending (internal field)
            **  {sort:date:asc}             -> by date, ascending (internal field)
            **  {sort:date:desc}            -> by date, descending (internal field)
            **  {sort:author:asc}           -> by author, ascending (externally defined field)
            **  {sort:author:desc}          -> by author, descending (externally defined field)
            **
            */

            /* Get a pointer to the colon */
            if ( (pwcTokenPtr = s_wcsstr(pwcToken, SRCH_PARSER_RANGE_COLON_WSTRING)) == NULL ) {
                iError = SRCH_ParserInvalidSort;
                goto bailFromiSrchParserExtractModifiers;
            }

            /* Scan the sort modifier */
            swprintf(pwcScanfFormat, UTL_STRING_SCANF_FORMAT_LENGTH + 1, L"%ls%%%dl[^%ls]%ls%%%dl[^%ls]%ls", SRCH_PARSER_MODIFIER_SEPARATOR_WSTRING, 
                    SRCH_PARSER_MINIMUM_LENGTH, SRCH_PARSER_MODIFIER_SEPARATOR_WSTRING, SRCH_PARSER_MODIFIER_SEPARATOR_WSTRING, 
                    SRCH_PARSER_MINIMUM_LENGTH, SRCH_PARSER_CHARACTER_RBRACKET_WSTRING, SRCH_PARSER_CHARACTER_RBRACKET_WSTRING);
            if ( swscanf(pwcTokenPtr, pwcScanfFormat, pwcSortFieldName, pwcSortOrder) != 2 ) {
                iError = SRCH_ParserInvalidSort;
                goto bailFromiSrchParserExtractModifiers;
            }
            
            /* Add the sort field name and sort order ID */
            if ( (pspSrchParser->uiSortOrderID = uiSrchParserGetSortOrderIDFromString(pspSrchParser, pwcSortOrder)) == SRCH_PARSER_INVALID_ID ) {
                iError = SRCH_ParserInvalidSortOrder;
                goto bailFromiSrchParserExtractModifiers;
            }

            /* Use the canonical sort field name if this is an internal sort field name, otherwise just use the sort field name */
            if ( (uiSortFieldNameID = uiSrchParserGetSortFieldNameIDFromString(pspSrchParser, pwcSortFieldName)) != SRCH_PARSER_INVALID_ID ) {
                pwcSortFieldNamePtr = pwcSrchParserGetStringFromID(pspSrchParser, uiSortFieldNameID);
            }
            else {
                pwcSortFieldNamePtr = pwcSortFieldName;
            }

            /* Free the sort field name if it is already set */
            s_free(pspSrchParser->pwcSortFieldName);

            /* Add the sort field name */
            if ( (pspSrchParser->pwcSortFieldName = s_wcsdup(pwcSortFieldNamePtr)) == NULL ) {
                iError = SRCH_MemError;
                goto bailFromiSrchParserExtractModifiers;
            }

            /* Convert the sort field name to lower case */
            pwcLngCaseConvertWideStringToLowerCase(pspSrchParser->pwcSortFieldName);
        }
        
        /* {term_weight:...} */
        else if ( uiTokenID == SRCH_PARSER_MODIFIER_TERM_WEIGHT_ID ) {

            float   fTermWeight = 0;
            
            /* Get a pointer to the colon */
            if ( (pwcTokenPtr = s_wcsstr(pwcToken, SRCH_PARSER_RANGE_COLON_WSTRING)) == NULL ) {
                iError = SRCH_ParserInvalidTermWeight;
                goto bailFromiSrchParserExtractModifiers;
            }
            
            /* Get and check the term weight, allow negative weight */
            if ( (fTermWeight = s_wcstof(pwcTokenPtr + 1, NULL)) == 0 ) {
                iError = SRCH_ParserInvalidTermWeight;
                goto bailFromiSrchParserExtractModifiers;
            }
            
            /* Set the term weight */
            pspSrchParser->fTermWeight = fTermWeight;
        }

        /* {feedback_term_weight:...} */
        else if ( uiTokenID == SRCH_PARSER_MODIFIER_FEEDBACK_TERM_WEIGHT_ID ) {

            float   fFeedbackTermWeight = 0;
            
            /* Get a pointer to the colon */
            if ( (pwcTokenPtr = s_wcsstr(pwcToken, SRCH_PARSER_RANGE_COLON_WSTRING)) == NULL ) {
                iError = SRCH_ParserInvalidFeedbackTermWeight;
                goto bailFromiSrchParserExtractModifiers;
            }

            /* Get and check the feedback term weight */
            if ( (fFeedbackTermWeight = s_wcstof(pwcTokenPtr + 1, NULL)) == 0 ) {
                iError = SRCH_ParserInvalidFeedbackTermWeight;
                goto bailFromiSrchParserExtractModifiers;
            }
            
            /* Set the feedback term weight */
            pspSrchParser->fFeedbackTermWeight = fFeedbackTermWeight;
        }

        /* {frequent_term_coverage_threshold:...} */
        else if ( uiTokenID == SRCH_PARSER_MODIFIER_FREQUENT_TERM_COVERAGE_THRESHOLD_ID ) {

            float   fFrequentTermCoverageThreshold = 0;
            
            /* Get a pointer to the colon */
            if ( (pwcTokenPtr = s_wcsstr(pwcToken, SRCH_PARSER_RANGE_COLON_WSTRING)) == NULL ) {
                iError = SRCH_ParserInvalidFrequentTermCoverageThreshold;
                goto bailFromiSrchParserExtractModifiers;
            }

            /* Get and check the frequent term coverage threshold */
            if ( (fFrequentTermCoverageThreshold = s_wcstof(pwcTokenPtr + 1, NULL)) <= 0 ) {
                iError = SRCH_ParserInvalidFrequentTermCoverageThreshold;
                goto bailFromiSrchParserExtractModifiers;
            }
            
            /* Set the frequent term coverage threshold */
            pspSrchParser->fFrequentTermCoverageThreshold = fFrequentTermCoverageThreshold;
        }

        /* {feedback_minimum_term_count:...} */
        else if ( uiTokenID == SRCH_PARSER_MODIFIER_FEEDBACK_MINIMUM_TERM_COUNT_ID ) {

            int     iFeedbackMinimumTermCount = 0;
            
            /* Get a pointer to the colon */
            if ( (pwcTokenPtr = s_wcsstr(pwcToken, SRCH_PARSER_RANGE_COLON_WSTRING)) == NULL ) {
                iError = SRCH_ParserInvalidMinimumTermCount;
                goto bailFromiSrchParserExtractModifiers;
            }

            /* Get and check the feedback minimum term count */
            if ((iFeedbackMinimumTermCount = (int)s_wcstol(pwcTokenPtr + 1, NULL, 10)) <= 0 ) {
                iError = SRCH_ParserInvalidMinimumTermCount;
                goto bailFromiSrchParserExtractModifiers;
            }
            
            /* Set the feedback minimum term count */
            pspSrchParser->uiFeedbackMinimumTermCount = (unsigned int)iFeedbackMinimumTermCount;
        }

        /* {feedback_maximum_term_percentage:...} */
        else if ( uiTokenID == SRCH_PARSER_MODIFIER_FEEDBACK_MAXIMUM_TERM_PERCENTAGE_ID ) {

            float   fFeedbackMaximumTermPercentage = 0;
            
            /* Get a pointer to the colon */
            if ( (pwcTokenPtr = s_wcsstr(pwcToken, SRCH_PARSER_RANGE_COLON_WSTRING)) == NULL ) {
                iError = SRCH_ParserInvalidFeedbackMaximumTermPercentage;
                goto bailFromiSrchParserExtractModifiers;
            }

            /* Get and check the feedback maximum term percentage */
            if ( (fFeedbackMaximumTermPercentage = s_wcstof(pwcTokenPtr + 1, NULL)) <= 0 ) {
                iError = SRCH_ParserInvalidFeedbackMaximumTermPercentage;
                goto bailFromiSrchParserExtractModifiers;
            }
            
            /* Set the feedback maximum term percentage */
            pspSrchParser->fFeedbackMaximumTermPercentage = fFeedbackMaximumTermPercentage;
        }

        /* {feedback_maximum_term_coverage_threshold:...} */
        else if ( uiTokenID == SRCH_PARSER_MODIFIER_FEEDBACK_MAXIMUM_TERM_COVERAGE_THRESHOLD_ID ) {

            float   fFeedbackMaximumTermCoverageThreshold = 0;
            
            /* Get a pointer to the colon */
            if ( (pwcTokenPtr = s_wcsstr(pwcToken, SRCH_PARSER_RANGE_COLON_WSTRING)) == NULL ) {
                iError = SRCH_ParserInvalidFeedbackMaximumTermCoverageThreshold;
                goto bailFromiSrchParserExtractModifiers;
            }

            /* Get and check the feedback maximum term coverage threshold */
            if ( (fFeedbackMaximumTermCoverageThreshold = s_wcstof(pwcTokenPtr + 1, NULL)) <= 0 ) {
                iError = SRCH_ParserInvalidFeedbackMaximumTermCoverageThreshold;
                goto bailFromiSrchParserExtractModifiers;
            }
            
            /* Set the feedback maximum term coverage threshold */
            pspSrchParser->fFeedbackMaximumTermCoverageThreshold = fFeedbackMaximumTermCoverageThreshold;
        }

        /* {connection_timeout:...} */
        else if ( uiTokenID == SRCH_PARSER_MODIFIER_CONNECTION_TIMEOUT_ID ) {

            int     iConnectionTimeout = 0;
            
            /* Get a pointer to the colon */
            if ( (pwcTokenPtr = s_wcsstr(pwcToken, SRCH_PARSER_RANGE_COLON_WSTRING)) == NULL ) {
                iError = SRCH_ParserInvalidConnectionTimeout;
                goto bailFromiSrchParserExtractModifiers;
            }

            /* Get and check the connection timeout */
            if ( (iConnectionTimeout = (int)s_wcstol(pwcTokenPtr + 1, NULL, 10)) <= 0 ) {
                iError = SRCH_ParserInvalidConnectionTimeout;
                goto bailFromiSrchParserExtractModifiers;
            }
            
            /* Set the connection timeout */
            pspSrchParser->uiConnectionTimeout = (unsigned int)iConnectionTimeout;
        }

        /* {search_timeout:...} */
        else if ( uiTokenID == SRCH_PARSER_MODIFIER_SEARCH_TIMEOUT_ID ) {

            int     iSearchTimeout = 0;
            
            /* Get a pointer to the colon */
            if ( (pwcTokenPtr = s_wcsstr(pwcToken, SRCH_PARSER_RANGE_COLON_WSTRING)) == NULL ) {
                iError = SRCH_ParserInvalidSearchTimeout;
                goto bailFromiSrchParserExtractModifiers;
            }

            /* Get and check the search timeout */
            if ( (iSearchTimeout = (int)s_wcstol(pwcTokenPtr + 1, NULL, 10)) <= 0 ) {
                iError = SRCH_ParserInvalidSearchTimeout;
                goto bailFromiSrchParserExtractModifiers;
            }
            
            /* Set the search timeout */
            pspSrchParser->uiSearchTimeout = (unsigned int)iSearchTimeout;
        }

        /* {retrieval_timeout:...} */
        else if ( uiTokenID == SRCH_PARSER_MODIFIER_RETRIEVAL_TIMEOUT_ID ) {

            int     iRetrievalTimeout = 0;
            
            /* Get a pointer to the colon */
            if ( (pwcTokenPtr = s_wcsstr(pwcToken, SRCH_PARSER_RANGE_COLON_WSTRING)) == NULL ) {
                iError = SRCH_ParserInvalidRetrievalTimeout;
                goto bailFromiSrchParserExtractModifiers;
            }

            /* Get and check the retrieval timeout */
            if ( (iRetrievalTimeout = (int)s_wcstol(pwcTokenPtr + 1, NULL, 10)) <= 0 ) {
                iError = SRCH_ParserInvalidRetrievalTimeout;
                goto bailFromiSrchParserExtractModifiers;
            }
            
            /* Set the retrieval timeout */
            pspSrchParser->uiRetrievalTimeout = (unsigned int)iRetrievalTimeout;
        }

        /* {information_timeout:...} */
        else if ( uiTokenID == SRCH_PARSER_MODIFIER_INFORMATION_TIMEOUT_ID ) {

            int     iInformationTimeout = 0;
            
            /* Get a pointer to the colon */
            if ( (pwcTokenPtr = s_wcsstr(pwcToken, SRCH_PARSER_RANGE_COLON_WSTRING)) == NULL ) {
                iError = SRCH_ParserInvalidInformationTimeout;
                goto bailFromiSrchParserExtractModifiers;
            }

            /* Get and check the information timeout */
            if ( (iInformationTimeout = (int)s_wcstol(pwcTokenPtr + 1, NULL, 10)) <= 0 ) {
                iError = SRCH_ParserInvalidInformationTimeout;
                goto bailFromiSrchParserExtractModifiers;
            }
            
            /* Set the information timeout */
            pspSrchParser->uiInformationTimeout = (unsigned int)iInformationTimeout;
        }

        /* {segments_searched_maximum:...} */
        else if ( uiTokenID == SRCH_PARSER_MODIFIER_MAXIMUM_SEGMENTS_SEARCHED_ID ) {

            int     iMaximumSegmentsSearched = 0;
            
            /* Get a pointer to the colon */
            if ( (pwcTokenPtr = s_wcsstr(pwcToken, SRCH_PARSER_RANGE_COLON_WSTRING)) == NULL ) {
                iError = SRCH_ParserInvalidSegmentsSearchedMaximum;
                goto bailFromiSrchParserExtractModifiers;
            }

            /* Get and check the maximum segments searched */
            if ( (iMaximumSegmentsSearched = (int)s_wcstol(pwcTokenPtr + 1, NULL, 10)) <= 0 ) {
                iError = SRCH_ParserInvalidSegmentsSearchedMaximum;
                goto bailFromiSrchParserExtractModifiers;
            }
            
            /* Set the maximum segments searched */
            pspSrchParser->uiMaximumSegmentsSearched = (unsigned int)iMaximumSegmentsSearched;
        }

        /* {segments_searched_minimum:...} */
        else if ( uiTokenID == SRCH_PARSER_MODIFIER_MINIMUM_SEGMENTS_SEARCHED_ID ) {

            int     iMinimumSegmentsSearched = 0;
            
            /* Get a pointer to the colon */
            if ( (pwcTokenPtr = s_wcsstr(pwcToken, SRCH_PARSER_RANGE_COLON_WSTRING)) == NULL ) {
                iError = SRCH_ParserInvalidSegmentsSearchedMinimum;
                goto bailFromiSrchParserExtractModifiers;
            }

            /* Get and check the minimum segments searched */
            if ( (iMinimumSegmentsSearched = (int)s_wcstol(pwcTokenPtr + 1, NULL, 10)) <= 0 ) {
                iError = SRCH_ParserInvalidSegmentsSearchedMinimum;
                goto bailFromiSrchParserExtractModifiers;
            }
            
            /* Set the minimum segments searched */
            pspSrchParser->uiMinimumSegmentsSearched = (unsigned int)iMinimumSegmentsSearched;
        }

        /* {exclusion_filter:.../{exclusion_list_filter:...} */
        else if ( (uiTokenID == SRCH_PARSER_MODIFIER_EXCLUSION_FILTER_ID) || (uiTokenID == SRCH_PARSER_MODIFIER_EXCLUSION_LIST_FILTER_ID) ) {

            struct srchParserFilter     *pspfSrchParserFilterExclusionFiltersPtr = NULL;

            /* Erase the trailing '}' */
            if ( (pwcTokenPtr = s_wcsstr(pwcToken, SRCH_PARSER_CHARACTER_RBRACKET_WSTRING)) != NULL ) {
                *pwcTokenPtr = L'\0';
            }

            /* Get a pointer to the colon */
            if ( (pwcTokenPtr = s_wcsstr(pwcToken, SRCH_PARSER_RANGE_COLON_WSTRING)) == NULL ) {
                iError = SRCH_ParserInvalidExclusionFilter;
                goto bailFromiSrchParserExtractModifiers;
            }

            /* Add the filter if there is anything there */
            if ( s_wcslen(pwcTokenPtr + 1) > 0 ) {
                
                /* Extend the exclusion filters array */
                if ( (pspfSrchParserFilterExclusionFiltersPtr = (struct srchParserFilter *)s_realloc(pspSrchParser->pspfSrchParserFilterExclusionFilters, 
                        (size_t)(sizeof(struct srchParserFilter) * (pspSrchParser->uiSrchParserFilterExclusionFiltersLength + 1)))) == NULL ) {
                    iError = SRCH_MemError;
                    goto bailFromiSrchParserExtractModifiers;
                }
                /* Hand over the pointer */
                pspSrchParser->pspfSrchParserFilterExclusionFilters = pspfSrchParserFilterExclusionFiltersPtr;
        
                /* Dereference for convenience, clear the newly allocated area and increment the array length */
                pspfSrchParserFilterExclusionFiltersPtr = pspSrchParser->pspfSrchParserFilterExclusionFilters + pspSrchParser->uiSrchParserFilterExclusionFiltersLength;
                s_memset(pspfSrchParserFilterExclusionFiltersPtr, 0, sizeof(struct srchParserFilter));
                pspSrchParser->uiSrchParserFilterExclusionFiltersLength++;
        
                /* Add the exclusion filter */
                if ( (pspfSrchParserFilterExclusionFiltersPtr->pwcFilter = s_wcsdup(pwcTokenPtr + 1)) == NULL ) {
                    iError = SRCH_MemError;
                    goto bailFromiSrchParserExtractModifiers;
                }
    
                /* Set the filter type */
                if ( uiTokenID == SRCH_PARSER_MODIFIER_EXCLUSION_FILTER_ID ) {
                    pspfSrchParserFilterExclusionFiltersPtr->uiFilterTypeID = SRCH_PARSER_FILTER_TYPE_TERMS_ID;
                }
                else if ( uiTokenID == SRCH_PARSER_MODIFIER_EXCLUSION_LIST_FILTER_ID ) {
                    pspfSrchParserFilterExclusionFiltersPtr->uiFilterTypeID = SRCH_PARSER_FILTER_TYPE_LIST_ID;
                }
            }
        }

        /* {inclusion_filter:...}/{inclusion_list_filter:...} */
        else if ( (uiTokenID == SRCH_PARSER_MODIFIER_INCLUSION_FILTER_ID) || (uiTokenID == SRCH_PARSER_MODIFIER_INCLUSION_LIST_FILTER_ID) ) {

            struct srchParserFilter     *pspfSrchParserFilterInclusionFiltersPtr = NULL;

            /* Erase the trailing '}' */
            if ( (pwcTokenPtr = s_wcschr(pwcToken, SRCH_PARSER_CHARACTER_RBRACKET_WCHAR)) != NULL ) {
                *pwcTokenPtr = L'\0';
            }

            /* Get a pointer to the colon */
            if ( (pwcTokenPtr = s_wcsstr(pwcToken, SRCH_PARSER_RANGE_COLON_WSTRING)) == NULL ) {
                iError = SRCH_ParserInvalidInclusionFilter;
                goto bailFromiSrchParserExtractModifiers;
            }

            /* Add the filter if there is anything there */
            if ( s_wcslen(pwcTokenPtr + 1) > 0 ) {
            
                /* Extend the inclusion filters array */
                if ( (pspfSrchParserFilterInclusionFiltersPtr = (struct srchParserFilter *)s_realloc(pspSrchParser->pspfSrchParserFilterInclusionFilters, 
                        (size_t)(sizeof(struct srchParserFilter) * (pspSrchParser->uiSrchParserFilterInclusionFiltersLength + 1)))) == NULL ) {
                    iError = SRCH_MemError;
                    goto bailFromiSrchParserExtractModifiers;
                }
                /* Hand over the pointer */
                pspSrchParser->pspfSrchParserFilterInclusionFilters = pspfSrchParserFilterInclusionFiltersPtr;
        
                /* Dereference for convenience, clear the newly allocated area and increment the array length */
                pspfSrchParserFilterInclusionFiltersPtr = pspSrchParser->pspfSrchParserFilterInclusionFilters + pspSrchParser->uiSrchParserFilterInclusionFiltersLength;
                s_memset(pspfSrchParserFilterInclusionFiltersPtr, 0, sizeof(struct srchParserFilter));
                pspSrchParser->uiSrchParserFilterInclusionFiltersLength++;
        
                /* Add the inclusion filter */
                if ( (pspfSrchParserFilterInclusionFiltersPtr->pwcFilter = s_wcsdup(pwcTokenPtr + 1)) == NULL ) {
                    iError = SRCH_MemError;
                    goto bailFromiSrchParserExtractModifiers;
                }
    
                /* Set the filter type */
                if ( uiTokenID == SRCH_PARSER_MODIFIER_INCLUSION_FILTER_ID ) {
                    pspfSrchParserFilterInclusionFiltersPtr->uiFilterTypeID = SRCH_PARSER_FILTER_TYPE_TERMS_ID;
                }
                else if ( uiTokenID == SRCH_PARSER_MODIFIER_INCLUSION_LIST_FILTER_ID ) {
                    pspfSrchParserFilterInclusionFiltersPtr->uiFilterTypeID = SRCH_PARSER_FILTER_TYPE_LIST_ID;
                }
            }
        }

        /* {language:...[,...]} */
        else if ( uiTokenID == SRCH_PARSER_MODIFIER_LANGUAGE_ID ) {

            /* Erase the trailing '}' */
            if ( (pwcTokenPtr = s_wcschr(pwcToken, SRCH_PARSER_CHARACTER_RBRACKET_WCHAR)) != NULL ) {
                *pwcTokenPtr = L'\0';
            }

            /* Get a pointer to the colon */
            if ( (pwcTokenPtr = s_wcsstr(pwcToken, SRCH_PARSER_RANGE_COLON_WSTRING)) == NULL ) {
                iError = SRCH_ParserInvalidLanguage;
                goto bailFromiSrchParserExtractModifiers;
            }

            /* Add the languages if there is anything there */
            if ( s_wcslen(pwcTokenPtr + 1) > 0 ) {
            
                wchar_t     *pwcLanguagePtr = NULL;
                wchar_t     *pwcLanguageWcstokPtr = NULL;

                /* Loop parsing the languages */
                for ( pwcLanguagePtr = s_wcstok(pwcTokenPtr + 1, SRCH_PARSER_CHARACTER_COMMA_WSTRING, &pwcLanguageWcstokPtr); pwcLanguagePtr != NULL; 
                        pwcLanguagePtr = s_wcstok(NULL, SRCH_PARSER_CHARACTER_COMMA_WSTRING, &pwcLanguageWcstokPtr) ) {
                
                    unsigned char               pucLanguage[SRCH_PARSER_MINIMUM_LENGTH + 1] = {L'\0'};
                    unsigned int                uiLanguageID = LNG_LANGUAGE_ANY_ID;
                    struct srchParserNumber     *pspnSrchParserNumberLanguageIDsPtr = NULL;

                    /* Convert the language to utf-8 */
                    if ( iLngConvertWideStringToUtf8_s(pwcLanguagePtr, s_wcslen(pwcLanguagePtr), pucLanguage, SRCH_PARSER_MINIMUM_LENGTH + 1) != LNG_NoError ) {
                        iUtlLogError(UTL_LOG_CONTEXT, "Failed to convert the language from wide characters to utf-8, lng error: %d.", iError);
                        iError = SRCH_ParserCharacterSetConversionFailed;
                        goto bailFromiSrchParserExtractModifiers;
                    }
        
                    /* Get the language ID */
                    if ( (iError = iLngGetLanguageIDFromCode(pucLanguage, &uiLanguageID)) != LNG_NoError ) {
                        iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the language ID from the language code: '%s', lng error: %d.", pucLanguage, iError); 
                        iError = SRCH_ParserInvalidLanguage;
                        goto bailFromiSrchParserExtractModifiers;
                    }

                    /* Extend the language IDs array */
                    if ( (pspnSrchParserNumberLanguageIDsPtr = (struct srchParserNumber *)s_realloc(pspSrchParser->pspnSrchParserNumberLanguageIDs, 
                            (size_t)(sizeof(struct srchParserNumber) * (pspSrchParser->uiSrchParserNumberLanguageIDsLength + 1)))) == NULL ) {
                        iError = SRCH_MemError;
                        goto bailFromiSrchParserExtractModifiers;
                    }
                    /* Hand over the pointer */
                    pspSrchParser->pspnSrchParserNumberLanguageIDs = pspnSrchParserNumberLanguageIDsPtr;
            
                    /* Dereference for convenience, clear the newly allocated area and increment the array length */
                    pspnSrchParserNumberLanguageIDsPtr = pspSrchParser->pspnSrchParserNumberLanguageIDs + pspSrchParser->uiSrchParserNumberLanguageIDsLength;
                    s_memset(pspnSrchParserNumberLanguageIDsPtr, 0, sizeof(struct srchParserNumber));
                    pspSrchParser->uiSrchParserNumberLanguageIDsLength++;
        
                    /* Set the language */
                    pspnSrchParserNumberLanguageIDsPtr->ulNumber = uiLanguageID;
                    pspnSrchParserNumberLanguageIDsPtr->uiRangeID = SRCH_PARSER_RANGE_EQUAL_ID;
                }
            }
        }

        /* {tag:...} */
        else if ( uiTokenID == SRCH_PARSER_MODIFIER_TAG_ID ) {
            ;
        }
                
        /* Unknown modifier */
        else if ( uiTokenID == SRCH_PARSER_MODIFIER_UNKNOWN_ID ) {
            ;
        }
                
        /* All other tokens */
        else {

            /* Append the token to the extracted search string */
            iUtlWideStringBufferAppend(pvUtlExtractedSearchWideStringBuffer, L" ");
            iUtlWideStringBufferAppend(pvUtlExtractedSearchWideStringBuffer, pwcToken);
        }

    }


    
    /* Bail label */
    bailFromiSrchParserExtractModifiers:

    
    /* Free the search copy */
    s_free(pwcSearchTextCopy);    
    
    /* Handle the error */
    if ( iError == SRCH_NoError ) {
        
        /* Get the string from the extracted search string buffer, setting the return pointer */
        iUtlWideStringBufferGetString(pvUtlExtractedSearchWideStringBuffer, ppwcExtractedSearch);

        /* Free the extracted search string buffer, note that we don't release the string */
        iUtlWideStringBufferFree(pvUtlExtractedSearchWideStringBuffer, false);
        pvUtlExtractedSearchWideStringBuffer = NULL;
        
        /* Trim the string */
        iUtlStringsTrimWideString(*ppwcExtractedSearch);
    }
    else {

        /* Free the extracted search string buffer, note that we do release the string */
        iUtlWideStringBufferFree(pvUtlExtractedSearchWideStringBuffer, true);
        pvUtlExtractedSearchWideStringBuffer = NULL;
    }


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchParserNormalizeSearch()

    Purpose:    This function performs any normalizations on the passed search. 
                Currently we perfom the following actions:

                    - Drop the case of the search terms if we need to
                    - Replace '-term' with 'not term'
                    - Fluff up CJK/Thai tokens & Strip token trailings
                    - Convert any quoted strings to adjacency operators
                    - Normalize fields names - field=(foo fee) => (field=foo field=fee)
                    - Add the default operator where needed

    Parameters: pspSrchParser           search parser structure
                pwcSearchText           search text (optional)
                ppwcNormalizedSearch    return parameter for the normalized search


    Globals:    none

    Returns:    SRCH error code

*/
static int iSrchParserNormalizeSearch
(
    struct srchParser *pspSrchParser,
    wchar_t *pwcSearchText,
    wchar_t **ppwcNormalizedSearch
)
{

    int         iError = SRCH_NoError;
    wchar_t     *pwcSearchTextCopy = NULL;
    wchar_t     *pwcSearchTextCopyWcstokPtr = NULL;
    void        *pvUtlNormalizedSearchWideStringBuffer = NULL;
    wchar_t     pwcBuffer[SRCH_PARSER_MINIMUM_LENGTH + 1] = {L'\0'};
    wchar_t     *pwcToken = NULL;
    wchar_t     *pwcPtr = NULL;
    

    ASSERT(pspSrchParser != NULL);
    ASSERT(ppwcNormalizedSearch != NULL);


    /* Check the search */
    if ( bUtlStringsIsWideStringNULL(pwcSearchText) == true ) {
        return (SRCH_NoError);
    }


    /* Make a duplicate copy of the search, we destroy this one */
    if ( (pwcSearchTextCopy = s_wcsdup(pwcSearchText)) == NULL ) {
        iError = SRCH_MemError;
        goto bailFromiSrchParserNormalizeSearch;
    }


    /* Drop the case of the search terms if we need to */
    if ( pspSrchParser->uiTermCaseID == SRCH_PARSER_MODIFIER_TERM_CASE_DROP_ID ) {

        unsigned int    uiTokenID = SRCH_PARSER_INVALID_ID;


        /* Allocate the normalized search string buffer */
        if ( (iError = iUtlWideStringBufferCreate(&pvUtlNormalizedSearchWideStringBuffer)) != UTL_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to create a wide string buffer, utl error: %d.", iError); 
            iError = SRCH_MemError;
            goto bailFromiSrchParserNormalizeSearch;
        }


        /* Get the first token */
        pwcToken = s_wcstok(pwcSearchTextCopy, L" ", (wchar_t **)&pwcSearchTextCopyWcstokPtr);
    
        while ( pwcToken != NULL ) {
            
            /* Check to see if this token is an operator */
            if ( (uiTokenID = uiSrchParserGetOperatorIDFromString(pspSrchParser, pwcToken)) != SRCH_PARSER_INVALID_ID ) {
                
                /* Get the operator, handle the NEAR differently because it can have a range after 
                ** the operator ie NEAR[20] and pwcSrchParserGetStringFromID() returns 'near' only
                */
                s_wcsnncpy(pwcBuffer, (uiTokenID == SRCH_PARSER_OPERATOR_NEAR_ID) ? pwcToken: pwcSrchParserGetStringFromID(pspSrchParser, uiTokenID), SRCH_PARSER_MINIMUM_LENGTH + 1);
            
                /* Convert the case of the default operator as needed */
                if ( pspSrchParser->uiOperatorCaseID == SRCH_PARSER_MODIFIER_OPERATOR_CASE_UPPER_ID ) {
                    pwcLngCaseConvertWideStringToUpperCase(pwcBuffer);
                }
                else if ( pspSrchParser->uiOperatorCaseID == SRCH_PARSER_MODIFIER_OPERATOR_CASE_LOWER_ID ) {
                    pwcLngCaseConvertWideStringToLowerCase(pwcBuffer);
                }
    
                iUtlWideStringBufferAppend(pvUtlNormalizedSearchWideStringBuffer, L" ");
                iUtlWideStringBufferAppend(pvUtlNormalizedSearchWideStringBuffer, pwcBuffer);
            }
            /* All other tokens */
            else {
                
                /* Drop the case of all search terms */ 
                if ( (uiSrchParserGetRangeIDFromString(pspSrchParser, pwcToken) == SRCH_PARSER_INVALID_ID) && 
                        (uiSrchParserGetModifierIDFromString(pspSrchParser, pwcToken) == SRCH_PARSER_INVALID_ID) &&
                        (uiSrchParserGetSortOrderIDFromString(pspSrchParser, pwcToken) == SRCH_PARSER_INVALID_ID) &&
                        (uiSrchParserGetFunctionIDFromString(pspSrchParser, pwcToken) == SRCH_PARSER_INVALID_ID) ) {
                    pwcLngCaseConvertWideStringToLowerCase(pwcToken);
                }
    
                /* Append the token to the normalized search string */
                iUtlWideStringBufferAppend(pvUtlNormalizedSearchWideStringBuffer, L" ");
                iUtlWideStringBufferAppend(pvUtlNormalizedSearchWideStringBuffer, pwcToken);
            }
    
            /* Get the next token */
            pwcToken = s_wcstok(NULL, L" ", (wchar_t **)&pwcSearchTextCopyWcstokPtr);

        }
    
        /* Free the search copy */
        s_free(pwcSearchTextCopy);

        /* Get the string from the normalized search string buffer, setting the search copy */
        iUtlWideStringBufferGetString(pvUtlNormalizedSearchWideStringBuffer, &pwcSearchTextCopy);
    
        /* Free the normalized search string buffer, note that we don't release the string */
        iUtlWideStringBufferFree(pvUtlNormalizedSearchWideStringBuffer, false);
        pvUtlNormalizedSearchWideStringBuffer = NULL;
    
        /* Trim the string */
        iUtlStringsTrimWideString(pwcSearchTextCopy);

        /* Check the string */
        if ( bUtlStringsIsWideStringNULL(pwcSearchTextCopy) == true ) {
            iError = SRCH_NoError;
            goto bailFromiSrchParserNormalizeSearch;
        }
    
        iUtlLogDebug(UTL_LOG_CONTEXT, "iSrchParserNormalizeSearch - dropped case: [%ls].", pwcSearchTextCopy);
    }




#if defined(SRCH_PARSER_ENABLE_LITERAL_REPLACEMENT)

    /* Replace '[term]' with 'literal[term]' */
    {

        /* Allocate the normalized search string buffer */
        if ( (iError = iUtlWideStringBufferCreate(&pvUtlNormalizedSearchWideStringBuffer)) != UTL_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to create a wide string buffer, utl error: %d.", iError); 
            iError = SRCH_MemError;
            goto bailFromiSrchParserNormalizeSearch;
        }


        /* Get the first token */
        pwcToken = s_wcstok(pwcSearchTextCopy, L" ", (wchar_t **)&pwcSearchTextCopyWcstokPtr);
    
        while ( pwcToken != NULL ) {
            
            /* Append a space to the normalized search string */
            iUtlWideStringBufferAppend(pvUtlNormalizedSearchWideStringBuffer, L" ");

            /* Replace '[term]' with 'literal[term]' */
            if ( (pwcToken[0] == SRCH_PARSER_CHARACTER_LSQUARE_BRACKET_WCHAR) && (pwcToken[s_wcslen(pwcToken) - 1] == SRCH_PARSER_CHARACTER_RSQUARE_BRACKET_WCHAR) ) {
                iUtlWideStringBufferAppend(pvUtlNormalizedSearchWideStringBuffer, SRCH_PARSER_FUNCTION_LITERAL_WSTRING);
            }
    
            /* Append the token pointer to the normalized search string */
            iUtlWideStringBufferAppend(pvUtlNormalizedSearchWideStringBuffer, pwcToken);
    
            /* Get the next token */
            pwcToken = s_wcstok(NULL, L" ", (wchar_t **)&pwcSearchTextCopyWcstokPtr);
        }
    
        /* Free the search copy */
        s_free(pwcSearchTextCopy);

        /* Get the string from the normalized search string buffer, setting the search copy */
        iUtlWideStringBufferGetString(pvUtlNormalizedSearchWideStringBuffer, &pwcSearchTextCopy);
    
        /* Free the normalized search string buffer, note that we don't release the string */
        iUtlWideStringBufferFree(pvUtlNormalizedSearchWideStringBuffer, false);
        pvUtlNormalizedSearchWideStringBuffer = NULL;
    
        /* Trim the string */
        iUtlStringsTrimWideString(pwcSearchTextCopy);

        /* Check the string */
        if ( bUtlStringsIsWideStringNULL(pwcSearchTextCopy) == true ) {
            iError = SRCH_NoError;
            goto bailFromiSrchParserNormalizeSearch;
        }
    
        iUtlLogDebug(UTL_LOG_CONTEXT, "iSrchParserNormalizeSearch - replaced '[term]' with 'literal[term]': [%ls].", pwcSearchTextCopy);
    }
#endif    /* defined(SRCH_PARSER_ENABLE_LITERAL_REPLACEMENT) */




#if defined(SRCH_PARSER_ENABLE_LITERAL_REPLACEMENT)

    /* Replace '-term' with 'not term' */
    {

        /* Allocate the normalized search string buffer */
        if ( (iError = iUtlWideStringBufferCreate(&pvUtlNormalizedSearchWideStringBuffer)) != UTL_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to create a wide string buffer, utl error: %d.", iError); 
            iError = SRCH_MemError;
            goto bailFromiSrchParserNormalizeSearch;
        }


        /* Get the 'not' operator */
        s_wcsnncpy(pwcBuffer, pwcSrchParserGetStringFromID(pspSrchParser, SRCH_PARSER_OPERATOR_NOT_ID), SRCH_PARSER_MINIMUM_LENGTH + 1);
    
        /* Convert the case of the 'not' operator as needed */
        if ( pspSrchParser->uiOperatorCaseID == SRCH_PARSER_MODIFIER_OPERATOR_CASE_UPPER_ID ) {
            pwcLngCaseConvertWideStringToUpperCase(pwcBuffer);
        }
        else if ( pspSrchParser->uiOperatorCaseID == SRCH_PARSER_MODIFIER_OPERATOR_CASE_LOWER_ID ) {
            pwcLngCaseConvertWideStringToLowerCase(pwcBuffer);
        }
    
        /* Get the first token */
        pwcToken = s_wcstok(pwcSearchTextCopy, L" ", (wchar_t **)&pwcSearchTextCopyWcstokPtr);
    
        while ( pwcToken != NULL ) {
            
            /* Set the token pointer to the token, this can get side-effected */
            wchar_t     *pwcTokenPtr = pwcToken;
    
            /* Process this token if there is a '-' at the start of the token and it is not an operator */
            if ( (s_wcschr(SRCH_PARSER_OPERATOR_MINUS_WSTRING, pwcToken[0]) != NULL) && 
                    (uiSrchParserGetOperatorIDFromString(pspSrchParser, pwcToken) == SRCH_PARSER_INVALID_ID) &&
                    (uiSrchParserGetRangeIDFromString(pspSrchParser, pwcToken) == SRCH_PARSER_INVALID_ID) && 
                    (uiSrchParserGetModifierIDFromString(pspSrchParser, pwcToken) == SRCH_PARSER_INVALID_ID) &&
                    (uiSrchParserGetSortOrderIDFromString(pspSrchParser, pwcToken) == SRCH_PARSER_INVALID_ID) &&
                    (uiSrchParserGetFunctionIDFromString(pspSrchParser, pwcToken) == SRCH_PARSER_INVALID_ID) ) {
    
                /* Set the token pointer to point to the start of term */
                pwcTokenPtr = pwcToken + 1;
            
                /* Append the 'not' operator to the normalized search string */
                iUtlWideStringBufferAppend(pvUtlNormalizedSearchWideStringBuffer, L" ");
                iUtlWideStringBufferAppend(pvUtlNormalizedSearchWideStringBuffer, pwcBuffer);
            }
    
            /* Append the token pointer to the normalized search string */
            iUtlWideStringBufferAppend(pvUtlNormalizedSearchWideStringBuffer, L" ");
            iUtlWideStringBufferAppend(pvUtlNormalizedSearchWideStringBuffer, pwcTokenPtr);
    
            /* Get the next token */
            pwcToken = s_wcstok(NULL, L" ", (wchar_t **)&pwcSearchTextCopyWcstokPtr);
        }
    
        /* Free the search copy */
        s_free(pwcSearchTextCopy);

        /* Get the string from the normalized search string buffer, setting the search copy */
        iUtlWideStringBufferGetString(pvUtlNormalizedSearchWideStringBuffer, &pwcSearchTextCopy);
    
        /* Free the normalized search string buffer, note that we don't release the string */
        iUtlWideStringBufferFree(pvUtlNormalizedSearchWideStringBuffer, false);
        pvUtlNormalizedSearchWideStringBuffer = NULL;
    
        /* Trim the string */
        iUtlStringsTrimWideString(pwcSearchTextCopy);

        /* Check the string */
        if ( bUtlStringsIsWideStringNULL(pwcSearchTextCopy) == true ) {
            iError = SRCH_NoError;
            goto bailFromiSrchParserNormalizeSearch;
        }
    
        iUtlLogDebug(UTL_LOG_CONTEXT, "iSrchParserNormalizeSearch - replaced '-term' with 'not term': [%ls].", pwcSearchTextCopy);
    }

#endif    /* defined(SRCH_PARSER_ENABLE_NOT_REPLACEMENT) */




#if defined(SRCH_PARSER_ENABLE_CJKT_PHRASING)

    /* 
    ** Phrasing of CJK/Thai tokens:
    **
    **  token       ->      (subtoken adj subtoken)
    **  "token"     ->      "subtoken subtoken"
    **  token       ->      (subtoken adj subtoken adj (subtoken or component or component))
    **
    */
    {
    
        boolean     bInPhrase = false;


        /* Allocate the normalized search string buffer */
        if ( (iError = iUtlWideStringBufferCreate(&pvUtlNormalizedSearchWideStringBuffer)) != UTL_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to create a wide string buffer, utl error: %d.", iError); 
            iError = SRCH_MemError;
            goto bailFromiSrchParserNormalizeSearch;
        }


        /* Get the first token */
        pwcToken = s_wcstok(pwcSearchTextCopy, L" ", (wchar_t **)&pwcSearchTextCopyWcstokPtr);
    
        while ( pwcToken != NULL ) {
            
            wchar_t     *pwcTokenPtr = NULL;
            wchar_t     pwcFieldName[SRCH_PARSER_MINIMUM_LENGTH + 1] = {L'\0'};
            boolean     bStripTrailings = false;        
            boolean     bFluffCJKThai = false;        
    
/*             iUtlLogDebug(UTL_LOG_CONTEXT, "==>> pwcToken: '%ls' <<==", pwcToken); */
    
            /* Check this token if it does not contain any wildcards and is not an operator */
            if ( (s_wcschr(pwcToken, SRCH_PARSER_CHARACTER_DOUBLE_QUOTE_WCHAR) == NULL) &&
#if defined(SRCH_PARSER_ENABLE_MULTI_WILDCARD)
                    (s_wcschr(pwcToken, SRCH_PARSER_WILDCARD_MULTI_WCHAR) == NULL) && 
#endif /* defined(SRCH_PARSER_ENABLE_MULTI_WILDCARD) */
#if defined(SRCH_PARSER_ENABLE_SINGLE_WILDCARD)
                    (s_wcschr(pwcToken, SRCH_PARSER_WILDCARD_SINGLE_WCHAR) == NULL) &&
#endif /* defined(SRCH_PARSER_ENABLE_SINGLE_WILDCARD) */
#if defined(s_wcschr)
                    (s_wcschr(pwcToken, SRCH_PARSER_WILDCARD_ALPHA_WCHAR) == NULL) &&
#endif /* defined(SRCH_PARSER_ENABLE_ALPHA_WILDCARD) */
#if defined(SRCH_PARSER_ENABLE_NUMERIC_WILDCARD)
                    (s_wcschr(pwcToken, SRCH_PARSER_WILDCARD_NUMERIC_WCHAR) == NULL) &&
#endif /* defined(SRCH_PARSER_ENABLE_NUMERIC_WILDCARD) */
                    (uiSrchParserGetOperatorIDFromString(pspSrchParser, pwcToken) == SRCH_PARSER_INVALID_ID) &&
                    (uiSrchParserGetRangeIDFromString(pspSrchParser, pwcToken) == SRCH_PARSER_INVALID_ID) && 
                    (uiSrchParserGetModifierIDFromString(pspSrchParser, pwcToken) == SRCH_PARSER_INVALID_ID) &&
                    (uiSrchParserGetSortOrderIDFromString(pspSrchParser, pwcToken) == SRCH_PARSER_INVALID_ID) &&
                    (uiSrchParserGetFunctionIDFromString(pspSrchParser, pwcToken) == SRCH_PARSER_INVALID_ID) ) {
    
    
                /* Check for a range operator in this token, finding a range operator means that there
                ** is a field name, so we extract the field name and place it in the buffer for later
                ** recovery if we find that this token contains CJK/Thai. When this is done, pwcPtr
                ** will point to the start of the term
                */
                if ( (bUtlStringsIsWideStringUrl(pwcToken) == false) && (pwcTokenPtr = s_wcspbrk(pwcToken, SRCH_PARSER_RANGE_LIST_WSTRING)) != NULL ) {
    
                    /* Increment the pointer to the end of the range operator, which could be one or 
                    ** two bytes long, pwcTokenPtr now points to the start of the term
                    */
                    while ( (s_wcschr(SRCH_PARSER_RANGE_LIST_WSTRING, *pwcTokenPtr) != NULL) && (*pwcTokenPtr != L'\0') ) {
                        pwcTokenPtr++;
                    }
    
                    /* Copy the field name/range operator to the buffer, making sure we don't overflow it */
                    s_wcsnncpy(pwcFieldName, pwcToken, UTL_MACROS_MIN((pwcTokenPtr - pwcToken) + 1, SRCH_PARSER_MINIMUM_LENGTH + 1));
                }
                /* Otherwise we clear the buffer and set pwcTokenPtr to point to the start of the token */
                else {
                    pwcTokenPtr = pwcToken;
                    pwcFieldName[0] = L'\0';
                }
    
    
/*                 iUtlLogDebug(UTL_LOG_CONTEXT, "==>> pwcToken: '%ls', pwcTokenPtr: '%ls' <<==", pwcToken, pwcTokenPtr); */

    
                /* Check the token for any CJK/Thai characters, set the flag and break out if we find any */
                for ( pwcPtr = pwcTokenPtr; *pwcPtr != L'\0'; pwcPtr++ ) {
                    if ( LNG_UNICODE_ENTIRE_CJK_RANGE(*pwcPtr) || LNG_UNICODE_THAI_RANGE(*pwcPtr) ) {
                        bFluffCJKThai = true;
                        break;
                    }
                }
    
                /* Strip the token trailings */
                if ( (bFluffCJKThai == false) && (bUtlStringsIsWideStringNULL(pwcTokenPtr) == false) ) {
                    bStripTrailings = true;        
                }
            }
    
            /* Flag if we are in a phrase or not, we need to do this because CJK/Thai tokens that
            ** get decomposed as subtokens are automatically grouped as a phrase using the 'adj'
            ** operator, which we don't want to do if the token is already a phrase, so:
            **
            **  token       ->      '(subtoken adj subtoken)'
            **  "token"     ->      '"subtoken subtoken"'
            **
            */
            else if ( s_wcscmp(pwcToken, SRCH_PARSER_CHARACTER_DOUBLE_QUOTE_WSTRING) == 0 ) {
                bInPhrase = !bInPhrase;
            }
    
    
            /* CJK characters were found */
            if ( bFluffCJKThai == true ) {
    
                unsigned int    uiLanguageID = SRCH_PARSER_TOKENIZER_LANGUAGE_ID_DEFAULT;
                wchar_t         *pwcSubTokenStartPtr = NULL;
                wchar_t         *pwcSubTokenEndPtr = NULL;
                wchar_t         wcSubTokenEnd = L'\0';
                wchar_t         pwcAdjOperator[SRCH_PARSER_MINIMUM_LENGTH + 1] = {L'\0'};
                wchar_t         pwcOrOperator[SRCH_PARSER_MINIMUM_LENGTH + 1] = {L'\0'};
                boolean         bFirstSubToken = true;
    
    
                /* Copy the ADJ operator to its buffer */
                s_wcsnncpy(pwcAdjOperator, SRCH_PARSER_OPERATOR_ADJ_WSTRING, SRCH_PARSER_MINIMUM_LENGTH + 1);
        
                /* Convert the case of the ADJ operator as needed */
                if ( pspSrchParser->uiOperatorCaseID == SRCH_PARSER_MODIFIER_OPERATOR_CASE_UPPER_ID ) {
                    pwcLngCaseConvertWideStringToUpperCase(pwcAdjOperator);
                }
                else if ( pspSrchParser->uiOperatorCaseID == SRCH_PARSER_MODIFIER_OPERATOR_CASE_LOWER_ID ) {
                    pwcLngCaseConvertWideStringToLowerCase(pwcAdjOperator);
                }
    
    
                /* Copy the OR operator to its buffer */
                s_wcsnncpy(pwcOrOperator, SRCH_PARSER_OPERATOR_OR_WSTRING, SRCH_PARSER_MINIMUM_LENGTH + 1);
        
                /* Convert the case of the OR operator as needed */
                if ( pspSrchParser->uiOperatorCaseID == SRCH_PARSER_MODIFIER_OPERATOR_CASE_UPPER_ID ) {
                    pwcLngCaseConvertWideStringToUpperCase(pwcOrOperator);
                }
                else if ( pspSrchParser->uiOperatorCaseID == SRCH_PARSER_MODIFIER_OPERATOR_CASE_LOWER_ID ) {
                    pwcLngCaseConvertWideStringToLowerCase(pwcOrOperator);
                }
        
/*                 iUtlLogDebug(UTL_LOG_CONTEXT, "==>> FLUFF CJK/Thai <<=="); */
    
                /* Set the language ID, default to ANY if the search language is english, otherwise set it to the passed language ID */
                uiLanguageID = (pspSrchParser->uiLanguageID == LNG_LANGUAGE_EN_ID) ? SRCH_PARSER_TOKENIZER_LANGUAGE_ID_DEFAULT : pspSrchParser->uiLanguageID;
    
/*                 iUtlLogDebug(UTL_LOG_CONTEXT, "uiLanguageID: %u", uiLanguageID); */
    
    
                /* Parse the token, weird because we want to handle any errors gracefully */
                if ( (iError = iLngTokenizerParseString(pspSrchParser->pvLngTokenizer, uiLanguageID, pwcTokenPtr, s_wcslen(pwcTokenPtr))) == LNG_NoError ) {
                    /* Get the first subtoken */
                    iError = iLngTokenizerGetToken(pspSrchParser->pvLngTokenizer, &pwcSubTokenStartPtr, &pwcSubTokenEndPtr);
                }
    
                /* We just append the token to the search string if we failed to parse it */
                if ( (iError != LNG_NoError) || (pwcSubTokenStartPtr == NULL) ) {
                    
                    /* Append a space to the normalized search string */
                    iUtlWideStringBufferAppend(pvUtlNormalizedSearchWideStringBuffer, L" ");
                    
                    /* Restore the field name/range operator in the search string */
                    if ( bUtlStringsIsWideStringNULL(pwcFieldName) == false ) {
                        iUtlWideStringBufferAppend(pvUtlNormalizedSearchWideStringBuffer, pwcFieldName);
                    }
    
                    /* Append the subtoken to the normalized search string */
                    iUtlWideStringBufferAppend(pvUtlNormalizedSearchWideStringBuffer, pwcTokenPtr);
                }
                else {
    
                    /* Append an lparen to the normalized search string if we are not in a phrase */
                    if ( bInPhrase == false ) {
                        iUtlWideStringBufferAppend(pvUtlNormalizedSearchWideStringBuffer, L" ");
                        iUtlWideStringBufferAppend(pvUtlNormalizedSearchWideStringBuffer, SRCH_PARSER_OPERATOR_LPAREN_WSTRING);
                        iUtlWideStringBufferAppend(pvUtlNormalizedSearchWideStringBuffer, L" ");
                    }
    
                    /* Parse the token into subtokens, keep looping while we find tokens */
                    while ( pwcSubTokenStartPtr != NULL ) {
                    
#if defined(SRCH_PARSER_ENABLE_CJKT_COMPONENT_ADDITION)
                        wchar_t     *pwcSubComponentStartPtr = NULL;
                        wchar_t     *pwcSubComponentEndPtr = NULL;
                        wchar_t     wcSubComponentEnd = L'\0';
#endif    /* defined(SRCH_PARSER_ENABLE_CJKT_COMPONENT_ADDITION) */                    
        
        
                        /* Append the ADJ operator to the normalized search string if we are not 
                        ** in a phrase and if this not the first subtoken
                        */
                        if ( bInPhrase == false ) {
                            if ( bFirstSubToken == false ) {
                                iUtlWideStringBufferAppend(pvUtlNormalizedSearchWideStringBuffer, L" ");
                                iUtlWideStringBufferAppend(pvUtlNormalizedSearchWideStringBuffer, pwcAdjOperator);
                                iUtlWideStringBufferAppend(pvUtlNormalizedSearchWideStringBuffer, L" ");
                            }
                            bFirstSubToken = false;
                        }
    
                        /* Append a space to the normalized search string */
                        iUtlWideStringBufferAppend(pvUtlNormalizedSearchWideStringBuffer, L" ");
                        
                        /* Restore the field name/range operator in the search string */
                        if ( bUtlStringsIsWideStringNULL(pwcFieldName) == false ) {
                            iUtlWideStringBufferAppend(pvUtlNormalizedSearchWideStringBuffer, pwcFieldName);
                        }
        
        
#if !defined(SRCH_PARSER_ENABLE_CJKT_COMPONENT_ADDITION)
        
                        /* Save the subtoken end character and NULL terminate the token */ 
                        wcSubTokenEnd = *pwcSubTokenEndPtr;
                        *pwcSubTokenEndPtr = L'\0';
        
/*                         iUtlLogDebug(UTL_LOG_CONTEXT, "==>> pwcSubTokenStartPtr: '%ls' <<==", pwcSubTokenStartPtr); */
    
                        /* Append the subtoken to the normalized search string */
                        iUtlWideStringBufferAppend(pvUtlNormalizedSearchWideStringBuffer, pwcSubTokenStartPtr);
        
                        /* Restore the subtoken end character */
                        *pwcSubTokenEndPtr = wcSubTokenEnd;
        
#else /* !defined(SRCH_PARSER_ENABLE_CJKT_COMPONENT_ADDITION) */
        
                        /* Get the first component for this sub token */
                        if ( iLngTokenizerGetComponent(pspSrchParser->pvLngTokenizer, &pwcSubComponentStartPtr, &pwcSubComponentEndPtr) != LNG_NoError ) {
                            iUtlLogError(UTL_LOG_CONTEXT, "Failed to get a component from the search text, lng error: %d.", iError);
                            iError = SRCH_ParserTokenizationFailed;
                            goto bailFromiSrchParserNormalizeSearch;
                        }
        
                        /* We just append the subtoken if we did not find a component, otherwise we
                        ** append '(subtoken OR component OR component ...)' if we found components
                        */
                        if ( pwcSubComponentStartPtr == NULL ) {
    
                            /* Save the subtoken end character and NULL terminate the token */ 
                            wcSubTokenEnd = *pwcSubTokenEndPtr;
                            *pwcSubTokenEndPtr = L'\0';
        
/*                             iUtlLogDebug(UTL_LOG_CONTEXT, "==>> pwcSubTokenStartPtr: '%ls' <<==", pwcSubTokenStartPtr); */
        
                            /* Append the subtoken to the normalized search string */
                            iUtlWideStringBufferAppend(pvUtlNormalizedSearchWideStringBuffer, pwcSubTokenStartPtr);
        
                            /* Restore the subtoken end character */
                            *pwcSubTokenEndPtr = wcSubTokenEnd;
                        }
                        else {
        
                            /* Append an lparen to the normalized search string */
                            iUtlWideStringBufferAppend(pvUtlNormalizedSearchWideStringBuffer, L" ");
                            iUtlWideStringBufferAppend(pvUtlNormalizedSearchWideStringBuffer, SRCH_PARSER_OPERATOR_LPAREN_WSTRING);
                            iUtlWideStringBufferAppend(pvUtlNormalizedSearchWideStringBuffer, L" ");
                            
                            /* Save the subtoken end character and NULL terminate the token */ 
                            wcSubTokenEnd = *pwcSubTokenEndPtr;
                            *pwcSubTokenEndPtr = L'\0';
        
/*                             iUtlLogDebug(UTL_LOG_CONTEXT, "==>> pwcSubTokenStartPtr: '%ls' <<==", pwcSubTokenStartPtr); */
        
                            /* Append the subtoken to the normalized search string */
                            iUtlWideStringBufferAppend(pvUtlNormalizedSearchWideStringBuffer, pwcSubTokenStartPtr);
        
                            /* Restore the subtoken end character */
                            *pwcSubTokenEndPtr = wcSubTokenEnd;
        
        
                            /* Keep looping while we find components */
                            while ( pwcSubComponentStartPtr != NULL ) {
                        
                                /* Append the OR operator to the normalized search string */
                                iUtlWideStringBufferAppend(pvUtlNormalizedSearchWideStringBuffer, L" ");
                                iUtlWideStringBufferAppend(pvUtlNormalizedSearchWideStringBuffer, pwcOrOperator);
                                iUtlWideStringBufferAppend(pvUtlNormalizedSearchWideStringBuffer, L" ");
        
                                /* Save the component end character and NULL terminate the component */ 
                                wcSubComponentEnd = *pwcSubComponentEndPtr;
                                *pwcSubComponentEndPtr = L'\0';
                    
/*                                 iUtlLogDebug(UTL_LOG_CONTEXT, "==>> pwcSubComponentStartPtr: '%ls' <<==", pwcSubComponentStartPtr); */
        
                                /* Append the component to the normalized search string */
                                iUtlWideStringBufferAppend(pvUtlNormalizedSearchWideStringBuffer, pwcSubComponentStartPtr);
            
                                /* Restore the component end character */
                                *pwcSubComponentEndPtr = wcSubComponentEnd;
            
                                /* Get the next component for this subtoken */
                                if ( iLngTokenizerGetComponent(pspSrchParser->pvLngTokenizer, &pwcSubComponentStartPtr, &pwcSubComponentEndPtr) != LNG_NoError ) {
                                    iUtlLogError(UTL_LOG_CONTEXT, "Failed to get a component from the search text, lng error: %d.", iError);
                                    iError = SRCH_ParserTokenizationFailed;
                                    goto bailFromiSrchParserNormalizeSearch;
                                }
                            }
        
                            /* Append an rparen to the normalized search string */
                            iUtlWideStringBufferAppend(pvUtlNormalizedSearchWideStringBuffer, L" ");
                            iUtlWideStringBufferAppend(pvUtlNormalizedSearchWideStringBuffer, SRCH_PARSER_OPERATOR_RPAREN_WSTRING);
                            iUtlWideStringBufferAppend(pvUtlNormalizedSearchWideStringBuffer, L" ");
                        }
        
#endif    /* !defined(SRCH_PARSER_ENABLE_CJKT_COMPONENT_ADDITION) */                    
        
        
                        /* Get the next token in the token */
                        if ( iLngTokenizerGetToken(pspSrchParser->pvLngTokenizer, &pwcSubTokenStartPtr, &pwcSubTokenEndPtr) != LNG_NoError ) {
                            iUtlLogError(UTL_LOG_CONTEXT, "Failed to get a token from the search text, lng error: %d.", iError);
                            iError = SRCH_ParserTokenizationFailed;
                            goto bailFromiSrchParserNormalizeSearch;
                        }
                    }
    
                    /* Append an rparen to the normalized search string if we are not in a phrase */
                    if ( bInPhrase == false ) {
                        iUtlWideStringBufferAppend(pvUtlNormalizedSearchWideStringBuffer, L" ");
                        iUtlWideStringBufferAppend(pvUtlNormalizedSearchWideStringBuffer, SRCH_PARSER_OPERATOR_RPAREN_WSTRING);
                        iUtlWideStringBufferAppend(pvUtlNormalizedSearchWideStringBuffer, L" ");
                    }
    
                }
            }
            else {
                
/*                 iUtlLogDebug(UTL_LOG_CONTEXT, "==>> APPEND TOKEN <<=="); */
/*                 iUtlLogDebug(UTL_LOG_CONTEXT, "==>> pwcToken: '%ls' <<==", pwcToken); */
    
                /* Append the token pointer to the normalized search string */
                iUtlWideStringBufferAppend(pvUtlNormalizedSearchWideStringBuffer, L" ");
                iUtlWideStringBufferAppend(pvUtlNormalizedSearchWideStringBuffer, pwcToken);
            }
    
    
            /* Get the next token */
            pwcToken = s_wcstok(NULL, L" ", (wchar_t **)&pwcSearchTextCopyWcstokPtr);
        }
    
        /* Free the search copy */
        s_free(pwcSearchTextCopy);

        /* Get the string from the normalized search string buffer, setting the search copy */
        iUtlWideStringBufferGetString(pvUtlNormalizedSearchWideStringBuffer, &pwcSearchTextCopy);
    
        /* Free the normalized search string buffer, note that we don't release the string */
        iUtlWideStringBufferFree(pvUtlNormalizedSearchWideStringBuffer, false);
        pvUtlNormalizedSearchWideStringBuffer = NULL;
    
        /* Trim the string */
        iUtlStringsTrimWideString(pwcSearchTextCopy);

        /* Check the string */
        if ( bUtlStringsIsWideStringNULL(pwcSearchTextCopy) == true ) {
            iError = SRCH_NoError;
            goto bailFromiSrchParserNormalizeSearch;
        }
    
        iUtlLogDebug(UTL_LOG_CONTEXT, "iSrchParserNormalizeSearch - fluffed CJK/Thai: [%ls].", pwcSearchTextCopy);
    }

#endif    /* defined(SRCH_PARSER_ENABLE_CJKT_PHRASING) */




    /* Convert any quoted strings to adjacency operators */
    {

        /* Allocate the normalized search string buffer */
        if ( (iError = iUtlWideStringBufferCreate(&pvUtlNormalizedSearchWideStringBuffer)) != UTL_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to create a wide string buffer, utl error: %d.", iError); 
            iError = SRCH_MemError;
            goto bailFromiSrchParserNormalizeSearch;
        }


        /* Get the first token */
        pwcToken = s_wcstok(pwcSearchTextCopy, L" ", (wchar_t **)&pwcSearchTextCopyWcstokPtr);
    
        while ( pwcToken != NULL ) {
    
            if ( s_wcsstr(SRCH_PARSER_CHARACTER_DOUBLE_QUOTE_WSTRING, pwcToken) != NULL ) {
    
                /* Append an lparen the normalized search string */
                iUtlWideStringBufferAppend(pvUtlNormalizedSearchWideStringBuffer, L" ");
                iUtlWideStringBufferAppend(pvUtlNormalizedSearchWideStringBuffer, SRCH_PARSER_OPERATOR_LPAREN_WSTRING);
                iUtlWideStringBufferAppend(pvUtlNormalizedSearchWideStringBuffer, L" ");
    
                pwcToken = s_wcstok(NULL, L" ", (wchar_t **)&pwcSearchTextCopyWcstokPtr);
    
                while ( pwcToken != NULL ) {
    
                    iUtlWideStringBufferAppend(pvUtlNormalizedSearchWideStringBuffer, pwcToken);
                    iUtlWideStringBufferAppend(pvUtlNormalizedSearchWideStringBuffer, L" ");
    
                    if ( (pwcToken = s_wcstok(NULL, L" ", (wchar_t **)&pwcSearchTextCopyWcstokPtr)) != NULL ) {
                        if ( s_wcsstr(SRCH_PARSER_CHARACTER_DOUBLE_QUOTE_WSTRING, pwcToken) != NULL ) {
                            iUtlWideStringBufferAppend(pvUtlNormalizedSearchWideStringBuffer, SRCH_PARSER_OPERATOR_RPAREN_WSTRING);
                            iUtlWideStringBufferAppend(pvUtlNormalizedSearchWideStringBuffer, L" ");
                            break;
                        }
                        else {
    
                            /* Copy the adjacency operator to the buffer */
                            s_wcsnncpy(pwcBuffer, SRCH_PARSER_OPERATOR_ADJ_WSTRING, SRCH_PARSER_MINIMUM_LENGTH + 1);
                        
                            /* Convert the case of the adjacency operator as needed */
                            if ( pspSrchParser->uiOperatorCaseID == SRCH_PARSER_MODIFIER_OPERATOR_CASE_UPPER_ID ) {
                                pwcLngCaseConvertWideStringToUpperCase(pwcBuffer);
                            }
                            else if ( pspSrchParser->uiOperatorCaseID == SRCH_PARSER_MODIFIER_OPERATOR_CASE_LOWER_ID ) {
                                pwcLngCaseConvertWideStringToLowerCase(pwcBuffer);
                            }
                            iUtlWideStringBufferAppend(pvUtlNormalizedSearchWideStringBuffer, pwcBuffer);
    
                            iUtlWideStringBufferAppend(pvUtlNormalizedSearchWideStringBuffer, L" ");
                        }
                    }
                }
            }
            else {
    
                /* Append the token to the normalized search string */
                iUtlWideStringBufferAppend(pvUtlNormalizedSearchWideStringBuffer, L" ");
                iUtlWideStringBufferAppend(pvUtlNormalizedSearchWideStringBuffer, pwcToken);
            }
    
            /* Get the next token */
            pwcToken = s_wcstok(NULL, L" ", (wchar_t **)&pwcSearchTextCopyWcstokPtr);
        }
    
        /* Free the search copy */
        s_free(pwcSearchTextCopy);

        /* Get the string from the normalized search string buffer, setting the search copy */
        iUtlWideStringBufferGetString(pvUtlNormalizedSearchWideStringBuffer, &pwcSearchTextCopy);
    
        /* Free the normalized search string buffer, note that we don't release the string */
        iUtlWideStringBufferFree(pvUtlNormalizedSearchWideStringBuffer, false);
        pvUtlNormalizedSearchWideStringBuffer = NULL;
    
        /* Trim the string */
        iUtlStringsTrimWideString(pwcSearchTextCopy);

        /* Check the string */
        if ( bUtlStringsIsWideStringNULL(pwcSearchTextCopy) == true ) {
            iError = SRCH_NoError;
            goto bailFromiSrchParserNormalizeSearch;
        }
    
        iUtlLogDebug(UTL_LOG_CONTEXT, "iSrchParserNormalizeSearch - quotes to adj: [%ls].", pwcSearchTextCopy);
    }





    /* Run through the replaces */
    {

        struct srchParserReplace *psprParserReplacePtr = psprParserReplaceStringListGlobal;

        /* Loop over each entry in the replace list */
        for ( psprParserReplacePtr = psprParserReplaceStringListGlobal; psprParserReplacePtr->pwcFromString != NULL; psprParserReplacePtr++ ) {
            
            wchar_t     pwcFromString[SRCH_PARSER_MINIMUM_LENGTH + 1] = {L'\0'};
            wchar_t     pwcToString[SRCH_PARSER_MINIMUM_LENGTH + 1] = {L'\0'};
            
            ASSERT(s_wcslen(psprParserReplacePtr->pwcFromString) == s_wcslen(psprParserReplacePtr->pwcToString));
        
            /* Make a copy of the strings, because we side effect them */
            s_wcsncpy(pwcFromString, psprParserReplacePtr->pwcFromString, SRCH_PARSER_MINIMUM_LENGTH + 1);
            s_wcsncpy(pwcToString, psprParserReplacePtr->pwcToString, SRCH_PARSER_MINIMUM_LENGTH + 1);

            /* Convert the operator case as needed */
            if ( pspSrchParser->uiOperatorCaseID == SRCH_PARSER_MODIFIER_OPERATOR_CASE_UPPER_ID ) {
                pwcLngCaseConvertWideStringToUpperCase(pwcFromString);
                pwcLngCaseConvertWideStringToUpperCase(pwcToString);
            }
            else if ( pspSrchParser->uiOperatorCaseID == SRCH_PARSER_MODIFIER_OPERATOR_CASE_LOWER_ID ) {
                pwcLngCaseConvertWideStringToLowerCase(pwcFromString);
                pwcLngCaseConvertWideStringToLowerCase(pwcToString);
            }

            /* Replace until there is nothing to replace any more, note the use of s_memcpy() to avoid the trailing NULL */
            while ( (pwcPtr = s_wcsstr(pwcSearchTextCopy, pwcFromString)) != NULL ) {
                s_memcpy(pwcPtr, pwcToString, s_wcslen(pwcToString) * sizeof(wchar_t));
            }

/* printf("iSrchParserNormalizeSearch - psprParserReplacePtr->pwcFromString: [%ls].\n", psprParserReplacePtr->pwcFromString); */
/* printf("iSrchParserNormalizeSearch - psprParserReplacePtr->pwcToString: [%ls].\n", psprParserReplacePtr->pwcToString); */
/* printf("iSrchParserNormalizeSearch - pwcSearchTextCopy: [%ls].\n", pwcSearchTextCopy); */
        }
            
        iUtlLogDebug(UTL_LOG_CONTEXT, "iSrchParserNormalizeSearch - replaces: [%ls].", pwcSearchTextCopy);
    }



    /* Normalize fields names - 'field=(foo fee)' => '(field=foo field=fee)' */
    {

        unsigned int    uiRightParenthesisCount = 0;
        unsigned int    uiLeftParenthesisCount = 0;
        unsigned int    uiParenthesisCount = 0;
        unsigned int    uiParenthesisIndex = 0;
        wchar_t         **ppwcFieldName = NULL;
        
        boolean         bInParens = false;


        /* Allocate the normalized search string buffer */
        if ( (iError = iUtlWideStringBufferCreate(&pvUtlNormalizedSearchWideStringBuffer)) != UTL_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to create a wide string buffer, utl error: %d.", iError); 
            iError = SRCH_MemError;
            goto bailFromiSrchParserNormalizeSearch;
        }


        /* Count up the number of left parens */
        for ( pwcPtr = pwcSearchTextCopy, uiLeftParenthesisCount = 0; *pwcPtr != L'\0'; pwcPtr++ ) {
            if ( s_wcschr(SRCH_PARSER_OPERATOR_RPAREN_WSTRING, *pwcPtr) != NULL ) {
                uiLeftParenthesisCount++;
            }
        }
    
        /* Count up the number of right parens */
        for ( pwcPtr = pwcSearchTextCopy, uiRightParenthesisCount = 0; *pwcPtr != L'\0'; pwcPtr++ ) {
            if ( s_wcschr(SRCH_PARSER_OPERATOR_RPAREN_WSTRING, *pwcPtr) != NULL ) {
                uiRightParenthesisCount++;
            }
        }
        
        /* Get the max number of parens */
        uiParenthesisCount = UTL_MACROS_MAX(uiLeftParenthesisCount, uiRightParenthesisCount);
    
        /* Allocate the field name stack accordingly */
        if ( (ppwcFieldName = (wchar_t **)s_malloc((size_t)((uiParenthesisCount + 1) * sizeof(wchar_t **)))) == NULL ) {
            iError = SRCH_MemError;
            goto bailFromiSrchParserNormalizeSearch;
        }
    
    
        /* Get the first token */
        pwcToken = s_wcstok(pwcSearchTextCopy, L" ", (wchar_t **)&pwcSearchTextCopyWcstokPtr);
    
        /* Loop over all the tokens */
        while ( pwcToken != NULL ) {
    
            unsigned int    uiStringLength = 0;
    
            /* Increment/decrement the paren counter depending on what paren we run into,
            ** make sure we dont overflow the array 
            */
            if ( s_wcscmp(SRCH_PARSER_OPERATOR_LPAREN_WSTRING, pwcToken) == 0 ) {
                uiParenthesisIndex += (uiParenthesisIndex == uiParenthesisCount) ? 0 : 1;
                ppwcFieldName[uiParenthesisIndex] = NULL;
                bInParens = true;
            }
            else if ( s_wcscmp(SRCH_PARSER_OPERATOR_RPAREN_WSTRING, pwcToken) == 0 ) {
                uiParenthesisIndex -= (uiParenthesisIndex == 0) ? 0 : 1;
                ppwcFieldName[uiParenthesisIndex] = NULL;
                bInParens = false;
            }
    
    
            /* Get the length of the current token - for optimization purposes only */
            uiStringLength = s_wcslen(pwcToken);
    
    
            /* If the token start and ends in a range ID, we just add it to the new search string */
            if ( (uiSrchParserGetRangeIDFromStartOfString(pspSrchParser, pwcToken) != SRCH_PARSER_INVALID_ID) && 
                    (uiSrchParserGetRangeIDFromEndOfString(pspSrchParser, pwcToken) != SRCH_PARSER_INVALID_ID)) {
                
                /* Append the current token to the new search string */
                iUtlWideStringBufferAppend(pvUtlNormalizedSearchWideStringBuffer, pwcToken);
                iUtlWideStringBufferAppend(pvUtlNormalizedSearchWideStringBuffer, L" ");
    
            }
            /* If the token ends in a range ID, then we extract it as it is a field specifier for a group of terms */
            else if ( uiSrchParserGetRangeIDFromEndOfString(pspSrchParser, pwcToken) != SRCH_PARSER_INVALID_ID ) {
                ppwcFieldName[uiParenthesisIndex] = pwcToken;
            }
            else {
    
                /* We can prepend a field name to the current token if is not an operator and does not contains a range token */
                if ( (uiSrchParserGetOperatorIDFromString(pspSrchParser, pwcToken) == SRCH_PARSER_INVALID_ID) && 
                        (uiSrchParserGetRangeIDFromEndOfString(pspSrchParser, pwcToken) == SRCH_PARSER_INVALID_ID) && (bInParens == true) ) {
    
                    int     iI = 0;
    
                    /* Work out way back up the stack to the last field and append it to the new search string */
                    for ( iI = uiParenthesisIndex; iI >= 0; iI-- ) {
                        if ( ppwcFieldName[iI] != NULL ) {
                            iUtlWideStringBufferAppend(pvUtlNormalizedSearchWideStringBuffer, ppwcFieldName[iI]);
                            break;
                        }
                    }
                }
    
                /* Finally append the current token to the new search string */
                iUtlWideStringBufferAppend(pvUtlNormalizedSearchWideStringBuffer, pwcToken);
                iUtlWideStringBufferAppend(pvUtlNormalizedSearchWideStringBuffer, L" ");
            }
    
            /* Get the next token */
            pwcToken = s_wcstok(NULL, L" ", (wchar_t **)&pwcSearchTextCopyWcstokPtr);
        }
    

        /* Catch odd cases where there are fielded searches with no terms */
        if ( ppwcFieldName[0] != NULL ) {

            int     iI = 0;

            for ( iI = uiParenthesisIndex; iI >= 0; iI-- ) {
                if ( ppwcFieldName[iI] != NULL ) {
                    iUtlWideStringBufferAppend(pvUtlNormalizedSearchWideStringBuffer, L" ");
                    iUtlWideStringBufferAppend(pvUtlNormalizedSearchWideStringBuffer, ppwcFieldName[iI]);
                }
            }
        }


        /* Free the field name stack */
        s_free(ppwcFieldName);

        /* Free the search copy */
        s_free(pwcSearchTextCopy);

        /* Get the string from the normalized search string buffer, setting the search copy */
        iUtlWideStringBufferGetString(pvUtlNormalizedSearchWideStringBuffer, &pwcSearchTextCopy);
    
        /* Free the normalized search string buffer, note that we don't release the string */
        iUtlWideStringBufferFree(pvUtlNormalizedSearchWideStringBuffer, false);
        pvUtlNormalizedSearchWideStringBuffer = NULL;
    
        /* Trim the string */
        iUtlStringsTrimWideString(pwcSearchTextCopy);

        /* Check the string */
        if ( bUtlStringsIsWideStringNULL(pwcSearchTextCopy) == true ) {
            iError = SRCH_NoError;
            goto bailFromiSrchParserNormalizeSearch;
        }
    
        iUtlLogDebug(UTL_LOG_CONTEXT, "iSrchParserNormalizeSearch - normalized fielded searches: [%ls].", pwcSearchTextCopy);
    }



    /* Add the default operator where needed - operators have been normalized and possible crunched, so any
    ** left hanging are terms
    */
    if ( pspSrchParser->uiSearchTypeID == SRCH_PARSER_MODIFIER_SEARCH_TYPE_BOOLEAN_ID ) {

        boolean     bLastTokenWasTerm = false;
        boolean     bLastTokenWasRParen = false;
        boolean     bLastTokenWasLParen = false;


        /* Allocate the normalized search string buffer */
        if ( (iError = iUtlWideStringBufferCreate(&pvUtlNormalizedSearchWideStringBuffer)) != UTL_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to create a wide string buffer, utl error: %d.", iError); 
            iError = SRCH_MemError;
            goto bailFromiSrchParserNormalizeSearch;
        }


        /* Get the default operator */
        if ( pspSrchParser->uiBooleanOperatorID == SRCH_PARSER_MODIFIER_BOOLEAN_OPERATOR_OR_ID ) {
            s_wcsnncpy(pwcBuffer, SRCH_PARSER_OPERATOR_OR_WSTRING, SRCH_PARSER_MINIMUM_LENGTH + 1);
        }
        else if ( pspSrchParser->uiBooleanOperatorID == SRCH_PARSER_MODIFIER_BOOLEAN_OPERATOR_IOR_ID ) {
            s_wcsnncpy(pwcBuffer, SRCH_PARSER_OPERATOR_IOR_WSTRING, SRCH_PARSER_MINIMUM_LENGTH + 1);
        }
        else if ( pspSrchParser->uiBooleanOperatorID == SRCH_PARSER_MODIFIER_BOOLEAN_OPERATOR_XOR_ID ) {
            s_wcsnncpy(pwcBuffer, SRCH_PARSER_OPERATOR_XOR_WSTRING, SRCH_PARSER_MINIMUM_LENGTH + 1);
        }
        else if ( pspSrchParser->uiBooleanOperatorID == SRCH_PARSER_MODIFIER_BOOLEAN_OPERATOR_AND_ID ) {
            s_wcsnncpy(pwcBuffer, SRCH_PARSER_OPERATOR_AND_WSTRING, SRCH_PARSER_MINIMUM_LENGTH + 1);
        }
        else if ( pspSrchParser->uiBooleanOperatorID == SRCH_PARSER_MODIFIER_BOOLEAN_OPERATOR_ADJ_ID ) {
            s_wcsnncpy(pwcBuffer, SRCH_PARSER_OPERATOR_ADJ_WSTRING, SRCH_PARSER_MINIMUM_LENGTH + 1);
        }
        else if ( pspSrchParser->uiBooleanOperatorID == SRCH_PARSER_MODIFIER_BOOLEAN_OPERATOR_NEAR_ID ) {
            s_wcsnncpy(pwcBuffer, SRCH_PARSER_OPERATOR_NEAR_WSTRING, SRCH_PARSER_MINIMUM_LENGTH + 1);
        }


        /* Convert the case of the default operator as needed */
        if ( pspSrchParser->uiOperatorCaseID == SRCH_PARSER_MODIFIER_OPERATOR_CASE_UPPER_ID ) {
            pwcLngCaseConvertWideStringToUpperCase(pwcBuffer);
        }
        else if ( pspSrchParser->uiOperatorCaseID == SRCH_PARSER_MODIFIER_OPERATOR_CASE_LOWER_ID ) {
            pwcLngCaseConvertWideStringToLowerCase(pwcBuffer);
        }
    
    
        /* Get the first token */
        pwcToken = s_wcstok(pwcSearchTextCopy, L" ", (wchar_t **)&pwcSearchTextCopyWcstokPtr);
    
        while ( pwcToken != NULL ) {
    
            /* Append a space to the normalized */
            iUtlWideStringBufferAppend(pvUtlNormalizedSearchWideStringBuffer, L" ");
    
            /* Add the default operator if this is a term and the last token was a term too */
            if ( ((uiSrchParserGetOperatorIDFromString(pspSrchParser, pwcToken) == SRCH_PARSER_INVALID_ID) && (bLastTokenWasTerm == true)) || 
                    ((s_wcscmp(pwcToken, SRCH_PARSER_OPERATOR_LPAREN_WSTRING) == 0) && ((bLastTokenWasTerm == true) || (bLastTokenWasRParen == true))) ) {
                iUtlWideStringBufferAppend(pvUtlNormalizedSearchWideStringBuffer, pwcBuffer);
                iUtlWideStringBufferAppend(pvUtlNormalizedSearchWideStringBuffer, L" ");
            }
    
            /* Add the default operator if this is a term and the last token was a right paren */
            else if ( (bLastTokenWasRParen == true) && (uiSrchParserGetOperatorIDFromString(pspSrchParser, pwcToken) == SRCH_PARSER_INVALID_ID) ) {
                iUtlWideStringBufferAppend(pvUtlNormalizedSearchWideStringBuffer, pwcBuffer);
                iUtlWideStringBufferAppend(pvUtlNormalizedSearchWideStringBuffer, L" ");
            }
    
            /* Add the token */
            iUtlWideStringBufferAppend(pvUtlNormalizedSearchWideStringBuffer, pwcToken);
    
    
            /* Set the flags */
            bLastTokenWasTerm = (uiSrchParserGetOperatorIDFromString(pspSrchParser, pwcToken) == SRCH_PARSER_INVALID_ID);
            bLastTokenWasRParen = (s_wcscmp(pwcToken, SRCH_PARSER_OPERATOR_RPAREN_WSTRING) == 0) ? true : false;
            bLastTokenWasLParen = (s_wcscmp(pwcToken, SRCH_PARSER_OPERATOR_LPAREN_WSTRING) == 0) ? true : false;
    
            /* Get the next token */
            pwcToken = s_wcstok(NULL, L" ", (wchar_t **)&pwcSearchTextCopyWcstokPtr);
        }

        /* Free the search copy */
        s_free(pwcSearchTextCopy);

        /* Get the string from the normalized search string buffer, setting the search copy */
        iUtlWideStringBufferGetString(pvUtlNormalizedSearchWideStringBuffer, &pwcSearchTextCopy);
    
        /* Free the normalized search string buffer, note that we don't release the string */
        iUtlWideStringBufferFree(pvUtlNormalizedSearchWideStringBuffer, false);
        pvUtlNormalizedSearchWideStringBuffer = NULL;
    
        /* Trim the string */
        iUtlStringsTrimWideString(pwcSearchTextCopy);

        /* Check the string */
        if ( bUtlStringsIsWideStringNULL(pwcSearchTextCopy) == true ) {
            iError = SRCH_NoError;
            goto bailFromiSrchParserNormalizeSearch;
        }
    
        iUtlLogDebug(UTL_LOG_CONTEXT, "iSrchParserNormalizeSearch - added default operator: [%ls].", pwcSearchTextCopy);
    }



    /* Bail label */
    bailFromiSrchParserNormalizeSearch:
    

    /* Handle the error */
    if ( iError == SRCH_NoError ) {
        
        /* Set the return pointer if there is a string, other free what is left */
        if ( bUtlStringsIsWideStringNULL(pwcSearchTextCopy) == false ) {
            *ppwcNormalizedSearch = pwcSearchTextCopy;
        }
        else {
            s_free(pwcSearchTextCopy);    
        }
    }
    else {

        /* Free the normalized search string buffer, note that we do release the string */
        iUtlWideStringBufferFree(pvUtlNormalizedSearchWideStringBuffer, true);
        pvUtlNormalizedSearchWideStringBuffer = NULL;

        /* Free the search copy */
        s_free(pwcSearchTextCopy);    
    }


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchParserCreateTermCluster()

    Purpose:    This function is the driver function which converts the
                string search to a term cluster search. The function which
                does the actual work is iSrchParserCreateTermCluster_r()

    Parameters: pspSrchParser                   search parser structure
                pwcSearchText                   search text (optional)
                ppsptcSrchParserTermCluster     return parameter for the search parser term cluster

    Globals:    none

    Returns:    SRCH error code

*/
static int iSrchParserCreateTermCluster
(
    struct srchParser *pspSrchParser,
    wchar_t *pwcSearchText,
    struct srchParserTermCluster **ppsptcSrchParserTermCluster
)
{


    int     iError = SRCH_NoError;
    wchar_t     *pwcSearchTextCopy = NULL;
    wchar_t     *pwcSearchTextCopyPtr = NULL;


    ASSERT(pspSrchParser != NULL);
    ASSERT(ppsptcSrchParserTermCluster != NULL);


    /* Check the search */
    if ( bUtlStringsIsWideStringNULL(pwcSearchText) == true ) {
        return (SRCH_NoError);
    }


    /* Make a duplicate copy of the search, we destroy this one */
    if ( (pwcSearchTextCopy = s_wcsdup(pwcSearchText)) == NULL ) {
        return (SRCH_MemError);
    }

    /* Make a copy of the pointer as it gets modified as the search is parsed */
    pwcSearchTextCopyPtr = pwcSearchTextCopy;


    /* Parse the search out to a term cluster */
    iError = iSrchParserCreateTermCluster_r(pspSrchParser, &pwcSearchTextCopyPtr, ppsptcSrchParserTermCluster);


    /* Free the duplicate copy of the search */
    s_free(pwcSearchTextCopy);


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchParserCreateTermCluster_r()

    Purpose:    This function converted the passed test search to a 
                term cluster search, it is designed to be recursive.

    Parameters: pspSrchParser                   search parser structure
                ppwcSearchText                  search text to parse/return pointer for the remaining search text to parse
                ppsptcSrchParserTermCluster     return parameter for the search parser term cluster

    Globals:    none

    Returns:    SRCH error code

*/
static int iSrchParserCreateTermCluster_r
(
    struct srchParser *pspSrchParser,
    wchar_t **ppwcSearchText,
    struct srchParserTermCluster **ppsptcSrchParserTermCluster
)
{


    int                             iError = SRCH_NoError;
    wchar_t                         *pwcSearchText = NULL;
    wchar_t                         *pwcSearchTextWcstokPtr = NULL;
    wchar_t                         *pwcToken = NULL;
    unsigned int                    uiOperatorID = SRCH_PARSER_INVALID_ID;
    int                             iTermDistance = 0;
    boolean                         bDistanceOrderMatters = false;
    struct srchParserTermCluster    *psptcSrchParserTermCluster = NULL;
    struct srchParserTermCluster    *psptcSrchParserTermClusterCurrent = NULL;
    void                            **ppvTerms = NULL;
    unsigned int                    *puiTermTypeIDs = NULL;


    ASSERT(pspSrchParser != NULL);
    ASSERT(ppwcSearchText != NULL);
    ASSERT(ppsptcSrchParserTermCluster != NULL);


    /* Set the search */
    pwcSearchText = *ppwcSearchText;

    iUtlLogDebug(UTL_LOG_CONTEXT, "iSrchParserCreateTermCluster_r - pwcSearchText [%ls].", pwcUtlStringsGetPrintableWideString(pwcSearchText));

    /* Check for NULLs */
    if ( bUtlStringsIsWideStringNULL(pwcSearchText) == true ) {
        return (SRCH_NoError);
    }



    /* Get the first token */
    if ( (pwcToken = s_wcstok(pwcSearchText, L" ", (wchar_t **)&pwcSearchTextWcstokPtr)) == NULL ) {
        return (SRCH_NoError);
    }


    iUtlLogDebug(UTL_LOG_CONTEXT, "iSrchParserCreateTermCluster_r - pwcToken [%ls].", pwcUtlStringsGetPrintableWideString(pwcToken));


    /* Allocate a new cluster */
    if ( (psptcSrchParserTermCluster = (struct srchParserTermCluster *)s_malloc((size_t)sizeof(struct srchParserTermCluster))) == NULL ) {
        return (SRCH_MemError);
    }

    /* Populate the cluster, note how we set the operator ID, default for free text searches, invalid ID for 
    ** boolean searches, where we extract the operator ID from the boolean operator in the search
    */
    psptcSrchParserTermCluster->ppvTerms = NULL;
    psptcSrchParserTermCluster->puiTermTypeIDs = NULL;
    psptcSrchParserTermCluster->uiTermsLength = 0;

    /* Set the operator */
    if ( pspSrchParser->uiSearchTypeID == SRCH_PARSER_MODIFIER_SEARCH_TYPE_FREETEXT_ID ) {

        if ( pspSrchParser->uiBooleanOperatorID == SRCH_PARSER_MODIFIER_BOOLEAN_OPERATOR_OR_ID ) {
            psptcSrchParserTermCluster->uiOperatorID = SRCH_PARSER_OPERATOR_OR_ID;
        }
        else if ( pspSrchParser->uiBooleanOperatorID == SRCH_PARSER_MODIFIER_BOOLEAN_OPERATOR_IOR_ID ) {
            psptcSrchParserTermCluster->uiOperatorID = SRCH_PARSER_OPERATOR_IOR_ID;
        }
        else if ( pspSrchParser->uiBooleanOperatorID == SRCH_PARSER_MODIFIER_BOOLEAN_OPERATOR_XOR_ID ) {
            psptcSrchParserTermCluster->uiOperatorID = SRCH_PARSER_OPERATOR_XOR_ID;
        }
        else if ( pspSrchParser->uiBooleanOperatorID == SRCH_PARSER_MODIFIER_BOOLEAN_OPERATOR_AND_ID ) {
            psptcSrchParserTermCluster->uiOperatorID = SRCH_PARSER_OPERATOR_AND_ID;
        }
        else if ( pspSrchParser->uiBooleanOperatorID == SRCH_PARSER_MODIFIER_BOOLEAN_OPERATOR_ADJ_ID ) {
            psptcSrchParserTermCluster->uiOperatorID = SRCH_PARSER_OPERATOR_ADJ_ID;
        }
        else if ( pspSrchParser->uiBooleanOperatorID == SRCH_PARSER_MODIFIER_BOOLEAN_OPERATOR_NEAR_ID ) {
            psptcSrchParserTermCluster->uiOperatorID = SRCH_PARSER_OPERATOR_NEAR_ID;
        }
    }
    else {
        psptcSrchParserTermCluster->uiOperatorID = SRCH_PARSER_INVALID_ID;
    }

    psptcSrchParserTermCluster->iTermDistance = 0;
    psptcSrchParserTermCluster->bDistanceOrderMatters = false;


    /* Set the current cluster */
    psptcSrchParserTermClusterCurrent = psptcSrchParserTermCluster;

    /* Set the return pointer */
    *ppsptcSrchParserTermCluster = psptcSrchParserTermClusterCurrent;


    /* Loop over all the tokens in this search */
    while ( pwcToken != NULL ) {

        /* Check if the token is an operator */
        if ( (uiOperatorID = uiSrchParserGetOperatorIDFromString(pspSrchParser, pwcToken)) != SRCH_PARSER_INVALID_ID ) {

            /* Token is an operator */
            wchar_t     pwcScanfFormat[UTL_STRING_SCANF_FORMAT_LENGTH + 1] = {L'\0'};


            /* Check for operator distance */
            int         iStatus = 0;
            wchar_t     *pwcField1 = NULL;


            /* Allocate the parse field */
            if ( (pwcField1 = (wchar_t *)s_malloc((size_t)(sizeof(wchar_t) * (s_wcslen(pwcToken) + 1)))) == NULL ) {
                return (SRCH_MemError);
            }

            /* Parse out the operator distance as a string */
            swprintf(pwcScanfFormat, UTL_STRING_SCANF_FORMAT_LENGTH + 1, L"%%*l[^[][%%%ul[^]]", (unsigned int)s_wcslen(pwcToken));
            iStatus = swscanf(pwcToken, pwcScanfFormat, pwcField1);

            /* Check to see if we got it */
            if ( (iStatus == 1) && (bUtlStringsIsWideStringNULL(pwcField1) == false) ) {

                boolean     bIsNumber = true;
                wchar_t     *pwcPtr = NULL;

                /* Test for a valid number */
                for ( pwcPtr = pwcField1, bIsNumber = true; *pwcPtr != L'\0'; pwcPtr++ ) {
                    if ( (iswdigit(*pwcPtr) == 0) && (*pwcPtr != L'-') && (*pwcPtr != L'+') && (*pwcPtr != L'.') ) {
                        bIsNumber = false;
                        break;
                    }
                }

                if ( bIsNumber == false ) {
                    s_free(pwcField1);
                    return (SRCH_ParserInvalidOperatorDistance);
                }

                iTermDistance = s_wcstol(pwcField1, NULL, 10);
                bDistanceOrderMatters = ((pwcField1[0] == L'-') || (pwcField1[0] == L'+')) ? true : false;
            }
            else {
                iTermDistance = 0;
                bDistanceOrderMatters = false;
            }

            s_free(pwcField1);


/*             iUtlLogDebug(UTL_LOG_CONTEXT, "iSrchParserCreateTermCluster_r - operator [%ls][%d].", pwcUtlStringsGetPrintableWideString(pwcToken), iTermDistance); */

            /* Return if the token is an rparen */
            if ( uiOperatorID == SRCH_PARSER_OPERATOR_RPAREN_ID ) {

/*                 iUtlLogDebug(UTL_LOG_CONTEXT, "iSrchParserCreateTermCluster_r - uiOperatorID == SRCH_PARSER_OPERATOR_RPAREN_ID."); */

                /* Check that this cluster was populated with a term list, is there is
                ** nothing, then this is an empty cluster and we can just free it
                */
                if ( (*ppsptcSrchParserTermCluster)->ppvTerms == NULL ) {
                    iSrchParserFreeTermCluster(*ppsptcSrchParserTermCluster);
                    *ppsptcSrchParserTermCluster = NULL;
                }

                /* Set the search return pointer */
                *ppwcSearchText = pwcSearchTextWcstokPtr;
                return (SRCH_NoError);
            }

            /* Set the operator if it has not yet been set and it is not an lparen */
            else if ( (uiOperatorID != SRCH_PARSER_OPERATOR_LPAREN_ID) && ((psptcSrchParserTermClusterCurrent->uiOperatorID == SRCH_PARSER_INVALID_ID) || 
                    ((psptcSrchParserTermClusterCurrent->uiOperatorID == uiOperatorID) && (psptcSrchParserTermClusterCurrent->iTermDistance == iTermDistance)) ) ) {

/*                 iUtlLogDebug(UTL_LOG_CONTEXT, "iSrchParserCreateTermCluster_r - (uiOperatorID != SRCH_PARSER_OPERATOR_LPAREN_ID)..."); */

                /* Set the cluster operator and term distance */
                psptcSrchParserTermClusterCurrent->uiOperatorID = uiOperatorID;
                psptcSrchParserTermClusterCurrent->iTermDistance = iTermDistance;
                psptcSrchParserTermClusterCurrent->bDistanceOrderMatters = bDistanceOrderMatters;
            }

            else {

/*                iUtlLogDebug(UTL_LOG_CONTEXT, "iSrchParserCreateTermCluster_r - else"); */

                /* We only create a new current cluster if the current one is used and the operator is not an lparen,
                ** this is an optimization (a rather ugly one at that) which saves us from creating empty
                ** clusters
                */
                if ( (uiOperatorID != SRCH_PARSER_OPERATOR_LPAREN_ID) && 
                        ((psptcSrchParserTermClusterCurrent->uiOperatorID != SRCH_PARSER_INVALID_ID) || (psptcSrchParserTermClusterCurrent->puiTermTypeIDs != NULL)) ) { 

/*                     iUtlLogDebug(UTL_LOG_CONTEXT, "iSrchParserCreateTermCluster_r - else - create new cluster"); */

                    /* Allocate a new cluster */
                    if ( (psptcSrchParserTermCluster = (struct srchParserTermCluster *)s_malloc((size_t)sizeof(struct srchParserTermCluster))) == NULL ) {
                        return (SRCH_MemError);
                    }


                    /* Populate the cluster */
                    psptcSrchParserTermCluster->ppvTerms = NULL;
                    psptcSrchParserTermCluster->puiTermTypeIDs = NULL;
                    psptcSrchParserTermCluster->uiTermsLength = 0;
                    psptcSrchParserTermCluster->uiOperatorID = uiOperatorID;
                    psptcSrchParserTermCluster->iTermDistance = iTermDistance;
                    psptcSrchParserTermCluster->bDistanceOrderMatters = bDistanceOrderMatters;

                    /* Extend the term list pointer */
                    if ( (ppvTerms = (void **)s_realloc(psptcSrchParserTermCluster->ppvTerms, 
                            (size_t)(sizeof(void *) * (psptcSrchParserTermCluster->uiTermsLength + 1)))) == NULL ) {
                        return (SRCH_MemError);
                    }

                    /* Hand over the pointer */
                    psptcSrchParserTermCluster->ppvTerms = ppvTerms;


                    /* Extend the term type lists pointer */
                    if ( (puiTermTypeIDs = (unsigned int *)s_realloc(psptcSrchParserTermCluster->puiTermTypeIDs, 
                            (size_t)(sizeof(unsigned int) * (psptcSrchParserTermCluster->uiTermsLength + 1)))) == NULL ) {
                        return (SRCH_MemError);
                    }

                    /* Hand over the pointer */
                    psptcSrchParserTermCluster->puiTermTypeIDs = puiTermTypeIDs;

                    /* Add the current cluster and term type to the appropriate lists */
                    psptcSrchParserTermCluster->ppvTerms[psptcSrchParserTermCluster->uiTermsLength] = (void *)psptcSrchParserTermClusterCurrent;
                    psptcSrchParserTermCluster->puiTermTypeIDs[psptcSrchParserTermCluster->uiTermsLength] = SRCH_PARSER_TERM_TYPE_TERM_CLUSTER_ID;

                    /* Increment the term list length */
                    psptcSrchParserTermCluster->uiTermsLength++;

                    /* Make the new cluster the current cluster */
                    psptcSrchParserTermClusterCurrent = psptcSrchParserTermCluster;

                    /* And set the return pointer */
                    *ppsptcSrchParserTermCluster = psptcSrchParserTermClusterCurrent;
                }


                /* Null out the return parameter */
                psptcSrchParserTermCluster = NULL;

                /* Call ourselves */
                if ( (iError = iSrchParserCreateTermCluster_r(pspSrchParser, &pwcSearchTextWcstokPtr, &psptcSrchParserTermCluster)) != SRCH_NoError ) {
                    /* We have to free the cluster here because it is not attached to the current one yet */
                    iSrchParserFreeTermCluster(psptcSrchParserTermCluster);
                    return (iError);
                }


                /* Add the cluster if it was created */
                if ( (psptcSrchParserTermCluster != NULL) && (psptcSrchParserTermCluster->ppvTerms != NULL) && 
                        (psptcSrchParserTermCluster->puiTermTypeIDs != NULL) ) {

                    /* Extend the term list pointer */
                    if ( (ppvTerms = (void **)s_realloc(psptcSrchParserTermClusterCurrent->ppvTerms, 
                            (size_t)(sizeof(void *) * (psptcSrchParserTermClusterCurrent->uiTermsLength + 1)))) == NULL ) {
                        return (SRCH_MemError);
                    }

                    /* Hand over the pointer */
                    psptcSrchParserTermClusterCurrent->ppvTerms = ppvTerms;


                    /* Extend the term type ID list pointer */
                    if ( (puiTermTypeIDs = (unsigned int *)s_realloc(psptcSrchParserTermClusterCurrent->puiTermTypeIDs, 
                            (size_t)(sizeof(unsigned int) * (psptcSrchParserTermClusterCurrent->uiTermsLength + 1)))) == NULL ) {
                        return (SRCH_MemError);
                    }

                    /* Hand over the pointer */
                    psptcSrchParserTermClusterCurrent->puiTermTypeIDs = puiTermTypeIDs;


                    /* Add the new cluster and term type to the appropriate lists */
                    psptcSrchParserTermClusterCurrent->ppvTerms[psptcSrchParserTermClusterCurrent->uiTermsLength] = (void *)psptcSrchParserTermCluster;
                    psptcSrchParserTermClusterCurrent->puiTermTypeIDs[psptcSrchParserTermClusterCurrent->uiTermsLength] = SRCH_PARSER_TERM_TYPE_TERM_CLUSTER_ID;

                    /* Increment the term list length */
                    psptcSrchParserTermClusterCurrent->uiTermsLength++;
                }
            }
        }
        else {

            /* Token is a term */
            
            wchar_t     *pwcTokenPtr = pwcToken;
            int         iSquareBracketCount = 0;
            boolean     bContainsSquareBrackets = false;

            /* Check that we have an even number of square brackets and that they are ok */
            for ( pwcTokenPtr = pwcToken, iSquareBracketCount = 0, bContainsSquareBrackets = false; *pwcTokenPtr != L'\0'; pwcTokenPtr++ ) {
                if ( *pwcTokenPtr == SRCH_PARSER_CHARACTER_LSQUARE_BRACKET_WCHAR ) {
                    iSquareBracketCount++;
                    bContainsSquareBrackets = true;
                }
                else if ( *pwcTokenPtr == SRCH_PARSER_CHARACTER_RSQUARE_BRACKET_WCHAR ) {
                    iSquareBracketCount--;
                    bContainsSquareBrackets = true;
                }
                
                if ( iSquareBracketCount < 0 ) {
                    break;
                }
            }

            /* Just add the term as-is if there is an uneven number of square brackets or if it does not end in a square bracket if there are square brackets */
            if ( (iSquareBracketCount != 0) || 
                    ((bContainsSquareBrackets == true) && ((pwcToken[s_wcslen(pwcToken) - 1] != SRCH_PARSER_CHARACTER_RSQUARE_BRACKET_WCHAR))) ) {
        
                iUtlLogDebug(UTL_LOG_CONTEXT, "(o) '%ls'", pwcToken);

                /* Add the term to the term cluster */
                if ( (iError = iSrchParserAddTermToTermCluster(pspSrchParser, psptcSrchParserTermClusterCurrent,
                        pwcToken, pwcToken, NULL, NULL, NULL, NULL)) != SRCH_NoError ) {
                    return (iError);
                }
            }
            
            /* Else parse out the term */
            else {
        
                int         iTokenLength = 0;
                int         iStatus1 = 0;
                int         iStatus2 = 0;
                wchar_t     pwcScanfFormat[UTL_STRING_SCANF_FORMAT_LENGTH + 1] = {L'\0'};
                wchar_t     *pwcField1 = NULL;
                wchar_t     *pwcField2 = NULL;
                wchar_t     *pwcField3 = NULL;
                wchar_t     *pwcField4 = NULL;
                wchar_t     *pwcField5 = NULL;
                wchar_t     *pwcField6 = NULL;
                wchar_t     *pwcFieldNamePtr = NULL;
                wchar_t     *pwcRangePtr = NULL;
                wchar_t     *pwcFunctionPtr = NULL;
                wchar_t     *pwcTermPtr = NULL;
                wchar_t     *pwcWeightPtr = NULL;
    
    
                /* Get the length of the token */
                iTokenLength = s_wcslen(pwcToken);
    
                /* Allocate the parse fields */
                if ( (pwcField1 = (wchar_t *)s_malloc((size_t)(sizeof(wchar_t) * (iTokenLength + 1)))) == NULL ) {
                    iError = SRCH_MemError;
                    goto bailFromiSrchParserCreateTermCluster_r;
                }
    
                if ( (pwcField2 = (wchar_t *)s_malloc((size_t)(sizeof(wchar_t) * (iTokenLength + 1)))) == NULL ) {
                    iError = SRCH_MemError;
                    goto bailFromiSrchParserCreateTermCluster_r;
                }
    
                if ( (pwcField3 = (wchar_t *)s_malloc((size_t)(sizeof(wchar_t) * (iTokenLength + 1)))) == NULL ) {
                    iError = SRCH_MemError;
                    goto bailFromiSrchParserCreateTermCluster_r;
                }
    
                if ( (pwcField4 = (wchar_t *)s_malloc((size_t)(sizeof(wchar_t) * (iTokenLength + 1)))) == NULL ) {
                    iError = SRCH_MemError;
                    goto bailFromiSrchParserCreateTermCluster_r;
                }
    
                if ( (pwcField5 = (wchar_t *)s_malloc((size_t)(sizeof(wchar_t) * (iTokenLength + 1)))) == NULL ) {
                    iError = SRCH_MemError;
                    goto bailFromiSrchParserCreateTermCluster_r;
                }
    
                if ( (pwcField6 = (wchar_t *)s_malloc((size_t)(sizeof(wchar_t) * (iTokenLength + 1)))) == NULL ) {
                    iError = SRCH_MemError;
                    goto bailFromiSrchParserCreateTermCluster_r;
                }
    
    
                /* Parse term content */
                swprintf(pwcScanfFormat, UTL_STRING_SCANF_FORMAT_LENGTH + 1, L"%%%dl[^%ls]%ls%%%dl[^%ls%ls]%ls%%%dl[^%ls]", iTokenLength, SRCH_PARSER_CHARACTER_LSQUARE_BRACKET_WSTRING, 
                        SRCH_PARSER_CHARACTER_LSQUARE_BRACKET_WSTRING, iTokenLength, SRCH_PARSER_CHARACTER_RSQUARE_BRACKET_WSTRING, SRCH_PARSER_CHARACTER_LSQUARE_BRACKET_WSTRING, 
                        SRCH_PARSER_CHARACTER_LSQUARE_BRACKET_WSTRING, iTokenLength, SRCH_PARSER_CHARACTER_RSQUARE_BRACKET_WSTRING);
                iStatus1 = swscanf(pwcToken, pwcScanfFormat, pwcField1, pwcField2, pwcField3);
    
                iUtlLogDebug(UTL_LOG_CONTEXT, "(i) '%ls'", pwcScanfFormat);
                iUtlLogDebug(UTL_LOG_CONTEXT, "pwcField1: [%ls], pwcField2: [%ls], pwcField3: [%ls]", pwcField1, pwcField2, pwcField3);
        
#if defined(SRCH_PARSER_ENABLE_RANGE_OPERATORS)
    
                if ( s_wcschr(SRCH_PARSER_RANGE_LIST_WSTRING, pwcField1[0]) == NULL ) {
                    swprintf(pwcScanfFormat, UTL_STRING_SCANF_FORMAT_LENGTH + 1, L"%%%dl[^%ls]%%%dl[%ls]%%%dls", iTokenLength, SRCH_PARSER_RANGE_LIST_WSTRING, iTokenLength, 
                            SRCH_PARSER_RANGE_LIST_WSTRING, iTokenLength);
                    iStatus2 = swscanf(pwcField1, pwcScanfFormat, pwcField4, pwcField5, pwcField6);
                    iUtlLogDebug(UTL_LOG_CONTEXT, "(ii) '%ls'", pwcScanfFormat);
                }
                else {
                    swprintf(pwcScanfFormat, UTL_STRING_SCANF_FORMAT_LENGTH + 1, L"%%%dl[%ls]%%%dls", iTokenLength, SRCH_PARSER_RANGE_LIST_WSTRING, iTokenLength);
                    iStatus2 = swscanf(pwcField1, pwcScanfFormat, pwcField5, pwcField6);
                    iUtlLogDebug(UTL_LOG_CONTEXT, "(iii) '%ls'", pwcScanfFormat);
                }
    
#else     /* defined(SRCH_PARSER_ENABLE_RANGE_OPERATORS) */
    
                if ( (s_wcschr(SRCH_PARSER_RANGE_EQUAL_WSTRING, pwcField1[0]) == NULL) && (s_wcschr(SRCH_PARSER_RANGE_COLON_WSTRING, pwcField1[0]) == NULL) ) {
                    swprintf(pwcScanfFormat, UTL_STRING_SCANF_FORMAT_LENGTH + 1, L"%%%dl[^%ls%ls]%%%dl[%ls%ls]%%%dls", iTokenLength, SRCH_PARSER_RANGE_EQUAL_WSTRING, 
                            SRCH_PARSER_RANGE_COLON_WSTRING, iTokenLength, SRCH_PARSER_RANGE_EQUAL_WSTRING, SRCH_PARSER_RANGE_COLON_WSTRING, iTokenLength);
                    iStatus2 = swscanf(pwcField1, pwcScanfFormat, pwcField4, pwcField5, pwcField6);
                    iUtlLogDebug(UTL_LOG_CONTEXT, "(iv) '%ls'", pwcScanfFormat);
                }
                else {
                    swprintf(pwcScanfFormat, UTL_STRING_SCANF_FORMAT_LENGTH + 1, L"%%%dl[%ls%ls]%%%dls", iTokenLength, SRCH_PARSER_RANGE_EQUAL_WSTRING, SRCH_PARSER_RANGE_COLON_WSTRING, iTokenLength);
                    iStatus2 = swscanf(pwcField1, pwcScanfFormat, pwcField5, pwcField6);
                    iUtlLogDebug(UTL_LOG_CONTEXT, "(v) '%ls'", pwcScanfFormat);
                }
    
#endif    /* defined(SRCH_PARSER_ENABLE_RANGE_OPERATORS) */
    
    
                /* Don't mistake the 'http:' for a field name if the token is a URL, note that we need to check for the 'required' flag */
                if ( s_wcschr(SRCH_PARSER_OPERATOR_PLUS_WSTRING, pwcToken[0]) != NULL ) {
                    if ( bUtlStringsIsWideStringUrl(pwcToken + 1) == true ) {
                        iStatus2 = 1;
                    }
                }
                else {
                    if ( bUtlStringsIsWideStringUrl(pwcToken) == true ) {
                        iStatus2 = 1;
                    }
                }
    
    
                iUtlLogDebug(UTL_LOG_CONTEXT, "[%ls] -> iStatus1 [%d], iStatus2 [%d] - > 1: [%ls], 2: [%ls], 3: [%ls], 4: [%ls], 5: [%ls], 6: [%ls].", 
                        pwcToken, iStatus1, iStatus2, pwcField1, pwcField2, pwcField3, pwcField4, pwcField5, pwcField6);
    
    
                /* '[' */
                if ( iStatus1 == 0 ) {
                    if ( iStatus2 == -1 ) {
                        pwcTermPtr = pwcToken;
                        iUtlLogDebug(UTL_LOG_CONTEXT, "(i) pwcTermPtr: [%ls].", pwcToken);
                    }
                }
                
                /* 'foo' or '=foo' or 'title=foo' or 'yahoo!' */
                else if ( iStatus1 == 1 ) {
                    
                    /* 'foo' */
                    if ( iStatus2 == 1 ) {
                        pwcTermPtr = pwcField1;
                        iUtlLogDebug(UTL_LOG_CONTEXT, "(ii) pwcTermPtr: [%ls].", pwcTermPtr);
                    }
                    /* '=foo' or 'yahoo!' */
                    else if ( iStatus2 == 2 ) {
                        /* Trailing operator - 'yahoo!' */
                        if ( (bUtlStringsIsWideStringNULL(pwcField4) == false) && (bUtlStringsIsWideStringNULL(pwcField5) == false) ) {
                            pwcTermPtr = pwcField1;
                            iUtlLogDebug(UTL_LOG_CONTEXT, "(iii) pwcTermPtr: [%ls].", pwcTermPtr);
                        }
                        /* Invalid leading operator - '!yahoo' */
                        else if ( uiSrchParserGetRangeIDFromString(pspSrchParser, pwcField5) == SRCH_PARSER_INVALID_ID ) {
                            pwcTermPtr = pwcField1;
                            iUtlLogDebug(UTL_LOG_CONTEXT, "(iv) pwcTermPtr: [%ls].", pwcTermPtr);
                        }
                        /* Valid leading operator '=foo' */
                        else {
#if defined(SRCH_PARSER_ENABLE_FIELDLESS_OPERATOR)
                            /* Use this if we allow a term to start with an operator */
                            pwcRangePtr = pwcField5;
                            pwcTermPtr = pwcField6;
                            iUtlLogDebug(UTL_LOG_CONTEXT, "(v) pwcRangePtr: [%ls], pwcTermPtr: [%ls].", pwcRangePtr, pwcTermPtr);
#else    /* defined(SRCH_PARSER_ENABLE_FIELDLESS_OPERATOR) */
                            /* Use this if we don't want a term to start with an operator */
                            pwcTermPtr = pwcField1;
                            iUtlLogDebug(UTL_LOG_CONTEXT, "(v) pwcTermPtr: [%ls].", pwcTermPtr);
#endif    /* defined(SRCH_PARSER_ENABLE_FIELDLESS_OPERATOR) */
                        }
                    }
                    /* 'title=foo' */
                    else if ( iStatus2 == 3 ) {
                        pwcFieldNamePtr = pwcField4;
                        pwcRangePtr = pwcField5;
                        pwcTermPtr = pwcField6;
                        iUtlLogDebug(UTL_LOG_CONTEXT, "(vi) pwcFieldNamePtr: [%ls], pwcRangePtr: [%ls], pwcTermPtr: [%ls].", pwcFieldNamePtr, pwcRangePtr, pwcTermPtr);
                    }
                }
    
                /* 'foo[5.0]' or 'title=foo[5.0]' or 'soundex[foo]' or 'title=soundex[foo]' */
                else if ( iStatus1 == 2 ) {
    
                    wchar_t     *pwcPtr = pwcField2;
                    boolean     bIsNumber = true;
    
                    /* Test for a valid number, assume innocent until proven guilty */
                    for ( pwcPtr = pwcField2, bIsNumber = true; *pwcPtr != L'\0'; pwcPtr++ ) {
                        if ( (iswdigit(*pwcPtr) == 0) && (*pwcPtr != L'.') ) {
                            bIsNumber = false;
                            break;
                        }
                    }
    
                    if ( bIsNumber == true ) {
                        /* 'foo[5.0]' */
                        if ( iStatus2 == 1 ) {
                            pwcTermPtr = pwcField1;
                            pwcWeightPtr = pwcField2;
                            iUtlLogDebug(UTL_LOG_CONTEXT, "(vii) pwcTermPtr: [%ls], pwcWeightPtr: [%ls].", pwcTermPtr, pwcWeightPtr);
                        }
                        /* 'title=foo[5.0]' */
                        else if ( iStatus2 == 3 ) {
                            pwcFieldNamePtr = pwcField4;
                            pwcRangePtr = pwcField5;
                            pwcTermPtr = pwcField6;
                            pwcWeightPtr = pwcField2;
                            iUtlLogDebug(UTL_LOG_CONTEXT, "(viii) pwcFieldNamePtr: [%ls], pwcRangePtr: [%ls], pwcTermPtr: [%ls], pwcWeightPtr: [%ls].", pwcFieldNamePtr, pwcRangePtr, pwcTermPtr, pwcWeightPtr);
                        }
                    }
                    else {
                        /* 'soundex[foo]' */
                        if ( iStatus2 == 1 ) {
                            pwcFunctionPtr = pwcField1;
                            pwcTermPtr = pwcField2;
                            iUtlLogDebug(UTL_LOG_CONTEXT, "(ix) pwcFunctionPtr: [%ls], pwcTermPtr: [%ls].", pwcFunctionPtr, pwcTermPtr);
                        }
                        /*  or 'title=soundex[foo]' */
                        else if ( iStatus2 == 3 ) {
                            pwcFieldNamePtr = pwcField4;
                            pwcRangePtr = pwcField5;
                            pwcFunctionPtr = pwcField6;
                            pwcTermPtr = pwcField2;
                            iUtlLogDebug(UTL_LOG_CONTEXT, "(x) pwcFieldNamePtr: [%ls], pwcRangePtr: [%ls], pwcFunctionPtr: [%ls], pwcTermPtr: [%ls].", pwcFieldNamePtr, pwcRangePtr, pwcFunctionPtr, pwcTermPtr);
                        }
                    }
                }
    
                /* 'soundex[foo[5.0]]' or 'title=soundex[foo[5.0]]' */
                else if ( iStatus1 == 3 ) {
                    /* 'soundex[foo[5.0]]' */
                    if ( iStatus2 == 1 ) {
                        pwcFunctionPtr = pwcField1;
                        pwcTermPtr = pwcField2;
                        pwcWeightPtr = pwcField3;
                        iUtlLogDebug(UTL_LOG_CONTEXT, "(xi) pwcFunctionPtr: [%ls], pwcTermPtr: [%ls], pwcWeightPtr: [%ls].", pwcFunctionPtr, pwcTermPtr, pwcWeightPtr);
                    }
                    /* 'title=soundex[foo[5.0]]' */
                    else if ( iStatus2 == 3 ) {
                        pwcFieldNamePtr = pwcField4;
                        pwcRangePtr = pwcField5;
                        pwcFunctionPtr = pwcField6;
                        pwcTermPtr = pwcField2;
                        pwcWeightPtr = pwcField3;
                        iUtlLogDebug(UTL_LOG_CONTEXT, "(xii) pwcFieldNamePtr: [%ls], pwcRangePtr: [%ls], pwcFunctionPtr: [%ls], pwcTermPtr: [%ls], pwcWeightPtr: [%ls].", 
                                pwcFieldNamePtr, pwcRangePtr, pwcFunctionPtr, pwcTermPtr, pwcWeightPtr);
                    }
                }
    
    
                /* Add the term to the term cluster */
                if ( (iError = iSrchParserAddTermToTermCluster(pspSrchParser, psptcSrchParserTermClusterCurrent,
                        pwcToken, pwcTermPtr, pwcFieldNamePtr, pwcRangePtr, pwcFunctionPtr, pwcWeightPtr)) != SRCH_NoError ) {
                    goto bailFromiSrchParserCreateTermCluster_r;
                }
    
    
    
                /* Bail label - the bail label is here to make sure that we free the fields */
                bailFromiSrchParserCreateTermCluster_r:
    
                /* Free the parse fields */
                s_free(pwcField1);
                s_free(pwcField2);
                s_free(pwcField3);
                s_free(pwcField4);
                s_free(pwcField5);
                s_free(pwcField6);
    
                /* Handle the error */
                if ( iError != SRCH_NoError ) {
                    return (iError);
                }
            }
        }


        /* Get the next token */
        pwcToken = s_wcstok(NULL, L" ", (wchar_t **)&pwcSearchTextWcstokPtr);

        iUtlLogDebug(UTL_LOG_CONTEXT, "iSrchParserCreateTermCluster_r - next pwcToken [%ls].", pwcUtlStringsGetPrintableWideString(pwcToken));
    }


    /* Set the return pointer */
    *ppwcSearchText = pwcSearchTextWcstokPtr;


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchParserAddTermToTermCluster()

    Purpose:    This function converted the passed test search to a 
                term cluster search, it is designed to be recursive.

    Parameters: pspSrchParser                   search parser structure
                psptcSrchParserTermCluster      search parser term cluster
                pwcToken                        token (optional)
                pwcTerm                         term (optional)
                pwcFieldName                    field name (optional)
                pwcRange                        range (optional)
                pwcFunction                     function (optional)
                pwcWeight                       weight (optional)

    Globals:    none

    Returns:    SRCH error code

*/
static int iSrchParserAddTermToTermCluster
(
    struct srchParser *pspSrchParser,
    struct srchParserTermCluster *psptcSrchParserTermCluster,
    wchar_t *pwcToken,
    wchar_t *pwcTerm,
    wchar_t *pwcFieldName,
    wchar_t *pwcRange,
    wchar_t *pwcFunction,
    wchar_t *pwcWeight
)
{


    int                     iError = SRCH_NoError;
    struct srchParserTerm   *psptSrchParserTerm = NULL;
    void                    **ppvTerms = NULL;
    unsigned int            *puiTermTypeIDs = NULL;
    unsigned int            uiRangeID = SRCH_PARSER_INVALID_ID;
    unsigned int            uiFunctionID = SRCH_PARSER_INVALID_ID;


    iUtlLogDebug(UTL_LOG_CONTEXT, "iSrchParserAddTermToTermCluster -  pwcToken: [%ls], pwcTerm: [%ls], pwcFieldName: [%ls], pwcRange: [%ls], pwcFunction: [%ls], pwcWeight: [%ls].", 
            pwcToken, pwcTerm, pwcFieldName, pwcRange, pwcFunction, pwcWeight);


    ASSERT(pspSrchParser != NULL);
    ASSERT(psptcSrchParserTermCluster != NULL);


    /* Skip this if the term was not specified */
    if ( bUtlStringsIsWideStringNULL(pwcTerm) == true ) {
        return (SRCH_NoError);
    }


    /* Allocate a new search parser term */
    if ( (psptSrchParserTerm = (struct srchParserTerm *)s_malloc((size_t)sizeof(struct srchParserTerm))) == NULL ) {
        iError = SRCH_MemError;
        goto bailFromiSrchParserAddTermToTermCluster;
    }
    
    
    /* Set the defaults in the new search parser term */
    psptSrchParserTerm->uiFunctionID = SRCH_PARSER_INVALID_ID;
    psptSrchParserTerm->uiRangeID = SRCH_PARSER_INVALID_ID;
    psptSrchParserTerm->bWildCardSearch = false;
    psptSrchParserTerm->fTermWeight = 0;
    psptSrchParserTerm->bRequired = false;


    /* Set the field name */
    if ( bUtlStringsIsWideStringNULL(pwcFieldName) == false ) {

        /* Extract and set the 'required' flag, and set the term pointer to the start of the term */
        if ( s_wcschr(SRCH_PARSER_OPERATOR_PLUS_WSTRING, pwcFieldName[0]) != NULL ) {
            pwcFieldName++;
            psptSrchParserTerm->bRequired = true;
        }

        /* Make a copy of the field name */
        if ( (psptSrchParserTerm->pwcFieldName = s_wcsdup(pwcFieldName)) == NULL ) {
            iError = SRCH_MemError;
            goto bailFromiSrchParserAddTermToTermCluster;
        }

#if defined(SRCH_PARSER_ENABLE_FIELD_NAME_NORMALIZATION)
        /* Normalize the field name */
        {
            wchar_t     *pwcFieldNamePtr = NULL;

            for ( pwcFieldNamePtr = psptSrchParserTerm->pwcFieldName; *pwcFieldNamePtr != L'\0'; pwcFieldNamePtr++ ) {
                if ( *pwcFieldNamePtr == L'-' ) {
                    *pwcFieldNamePtr = L'_';
                }
            }
        }
#endif    /* defined(SRCH_PARSER_ENABLE_FIELD_NAME_NORMALIZATION) */        

    }
    else {
        psptSrchParserTerm->pwcFieldName = NULL;
    }


    /* Set the range ID, defaults to equal */
    if ( bUtlStringsIsWideStringNULL(pwcRange) == false ) {
        if ( (uiRangeID = uiSrchParserGetRangeIDFromString(pspSrchParser, pwcRange)) != SRCH_PARSER_INVALID_ID ) {
            psptSrchParserTerm->uiRangeID = uiRangeID;
        }
        else {
            iError = SRCH_ParserInvalidRange;
            goto bailFromiSrchParserAddTermToTermCluster;
        }
    }
    else {
        psptSrchParserTerm->uiRangeID = SRCH_PARSER_RANGE_EQUAL_ID;
    }


    /* Set the function ID */
    if ( bUtlStringsIsWideStringNULL(pwcFunction) == false ) {

        if ( (uiFunctionID = uiSrchParserGetFunctionIDFromString(pspSrchParser, pwcFunction)) != SRCH_PARSER_INVALID_ID ) {
            psptSrchParserTerm->uiFunctionID = uiFunctionID;
        }
        else {
#if !defined(SRCH_PARSER_ENABLE_INVALID_FUNCTION_SKIPPING)
            iError = SRCH_ParserInvalidFunction;
            goto bailFromiSrchParserAddTermToTermCluster;
#else
            psptSrchParserTerm->uiFunctionID = SRCH_PARSER_INVALID_ID;
#endif /* defined(SRCH_PARSER_ENABLE_INVALID_FUNCTION_SKIPPING) */
        }

        /* If the function is a literal or a regex, we need to get the term again */                
        if ( (psptSrchParserTerm->uiFunctionID == SRCH_PARSER_FUNCTION_LITERAL_ID) || (psptSrchParserTerm->uiFunctionID == SRCH_PARSER_FUNCTION_REGEX_ID) ) {

            wchar_t     *pwcPtr = NULL;

            /* Get a pointer to the first left square bracket, this should never be NULL */
            if ( (pwcPtr = s_wcschr(pwcToken, SRCH_PARSER_CHARACTER_LSQUARE_BRACKET_WCHAR)) == NULL ) {
                iError = SRCH_ParserInvalidFunction;
                goto bailFromiSrchParserAddTermToTermCluster;
            }
            
            /* Copy the data to the term pointer */
            s_wcsnncpy(pwcTerm, pwcPtr + 1, s_wcslen(pwcPtr) - 1);

            /* NULL out the weight pointer because we do not support terms weights in literal or regex functions */
            pwcWeight = NULL;                    
        }
    }
    else {
        psptSrchParserTerm->uiFunctionID = SRCH_PARSER_INVALID_ID;
    }
                


    /* Extract and set the 'required' flag, and set the term pointer to the start of the term */
    if ( s_wcschr(SRCH_PARSER_OPERATOR_PLUS_WSTRING, pwcTerm[0]) != NULL ) {
        pwcTerm++;
        psptSrchParserTerm->bRequired = true;
    }



    /* Check the term for wildcard errors, we do this here as opposed to further up, where it would
    ** make more sense at first glance, because the term pointer can be side-effected in two places
    */
    {

        wchar_t     *pwcTermPtr = NULL;
        boolean     bWildCardFlag = true;

        /* Check for a term entirely composed of wildcards */
        for ( pwcTermPtr = pwcTerm, bWildCardFlag = true; *pwcTermPtr != L'\0'; pwcTermPtr++ ) {
        
            /* This will never match, but it makes the defines below a little easier to read */        
            if ( *pwcTermPtr == L' ' ) {
                ;
            }

#if defined(SRCH_PARSER_ENABLE_BACKSLASH_ESCAPE)

#if defined(SRCH_PARSER_ENABLE_MULTI_WILDCARD)
            else if ( !((*pwcTermPtr != SRCH_PARSER_WILDCARD_ESCAPE_WCHAR) && (*(pwcTermPtr + 1) == SRCH_PARSER_WILDCARD_MULTI_WCHAR)) ) {
                bWildCardFlag = false;
                break;
            }
#endif /* defined(SRCH_PARSER_ENABLE_MULTI_WILDCARD) */
    
#if defined(SRCH_PARSER_ENABLE_SINGLE_WILDCARD)
            else if ( !((*pwcTermPtr != SRCH_PARSER_WILDCARD_ESCAPE_WCHAR) && (*(pwcTermPtr + 1) == SRCH_PARSER_WILDCARD_SINGLE_WCHAR)) ) {
                bWildCardFlag = false;
                break;
            }
#endif /* defined(SRCH_PARSER_ENABLE_SINGLE_WILDCARD) */
    
#if defined(SRCH_PARSER_ENABLE_ALPHA_WILDCARD)
            else if ( !((*pwcTermPtr != SRCH_PARSER_WILDCARD_ESCAPE_WCHAR) && (*(pwcTermPtr + 1) == SRCH_PARSER_WILDCARD_ALPHA_WCHAR)) ) {
                bWildCardFlag = false;
                break;
            }
#endif /* defined(SRCH_PARSER_ENABLE_ALPHA_WILDCARD) */
    
#if defined(SRCH_PARSER_ENABLE_NUMERIC_WILDCARD)
            else if ( !((*pwcTermPtr != SRCH_PARSER_WILDCARD_ESCAPE_WCHAR) && (*(pwcTermPtr + 1) == SRCH_PARSER_WILDCARD_NUMERIC_WCHAR)) ) {
                bWildCardFlag = false;
                break;
            }
#endif /* defined(SRCH_PARSER_ENABLE_NUMERIC_WILDCARD) */

#else /* defined(SRCH_PARSER_ENABLE_BACKSLASH_ESCAPE) */

#if defined(SRCH_PARSER_ENABLE_MULTI_WILDCARD)
            else if ( *pwcTermPtr != SRCH_PARSER_WILDCARD_MULTI_WCHAR ) {
                bWildCardFlag = false;
                break;
            }
#endif /* defined(SRCH_PARSER_ENABLE_MULTI_WILDCARD) */
    
#if defined(SRCH_PARSER_ENABLE_SINGLE_WILDCARD)
            else if ( *pwcTermPtr != SRCH_PARSER_WILDCARD_SINGLE_WCHAR ) {
                bWildCardFlag = false;
                break;
            }
#endif /* defined(SRCH_PARSER_ENABLE_SINGLE_WILDCARD) */
    
#if defined(SRCH_PARSER_ENABLE_ALPHA_WILDCARD)
            else if ( *pwcTermPtr != SRCH_PARSER_WILDCARD_ALPHA_WCHAR ) {
                bWildCardFlag = false;
                break;
            }
#endif /* defined(SRCH_PARSER_ENABLE_ALPHA_WILDCARD) */
    
#if defined(SRCH_PARSER_ENABLE_NUMERIC_WILDCARD)
            else if ( *pwcTermPtr != SRCH_PARSER_WILDCARD_NUMERIC_WCHAR ) {
                bWildCardFlag = false;
                break;
            }
#endif /* defined(SRCH_PARSER_ENABLE_NUMERIC_WILDCARD) */

#endif /* defined(SRCH_PARSER_ENABLE_BACKSLASH_ESCAPE) */

        }


        /* Return an error if the term was entirely composed of wildcards */
        if ( bWildCardFlag == true ) {
#if defined(SRCH_PARSER_ENABLE_WILDCARD_ERROR_SKIPPING)
            goto bailFromiSrchParserAddTermToTermCluster;
#else /* defined(SRCH_PARSER_ENABLE_WILDCARD_ERROR_SKIPPING) */
            iError = SRCH_ParserInvalidWildCard;
            goto bailFromiSrchParserAddTermToTermCluster;
#endif /* defined(SRCH_PARSER_ENABLE_WILDCARD_ERROR_SKIPPING) */
        }
        else {
#if defined(SRCH_PARSER_LEADING_WILDCARD_STRIPPING)
            /* Update the term pointer to point to the start of the actual text */
            pwcTerm = pwcTermPtr;
#endif /* defined(SRCH_PARSER_LEADING_WILDCARD_STRIPPING) */
        }
    }



    /* Set the term */
    if ( (psptSrchParserTerm->pwcTerm = s_wcsdup(pwcTerm)) == NULL ) {
        iError = SRCH_MemError;
        goto bailFromiSrchParserAddTermToTermCluster;
    }
                


    /* Strip the term trailings */
#if defined(SRCH_PARSER_ENABLE_STRIPPING_TERM_TRAILINGS)
    {
        /* Set the language ID, default to ANY if the search language is english, otherwise set it to the passed language ID */
        unsigned int    uiLanguageID = (pspSrchParser->uiLanguageID == LNG_LANGUAGE_EN_ID) ? SRCH_PARSER_TOKENIZER_LANGUAGE_ID_DEFAULT : pspSrchParser->uiLanguageID;
        
        wchar_t         *pwcTokenEndPtr = NULL;
    
        /* Strip the term trailings */
        if ( iLngTokenizerStripTrailings(pspSrchParser->pvLngTokenizer, uiLanguageID, psptSrchParserTerm->pwcTerm, s_wcslen(psptSrchParserTerm->pwcTerm), &pwcTokenEndPtr) != LNG_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to strip the trailings from a term, lng error: %d.", iError);
            iError = SRCH_ParserTokenizationFailed;
            goto bailFromiSrchParserAddTermToTermCluster;
        }
    
        /* NULL terminate the token */ 
        *pwcTokenEndPtr = L'\0';
    }
#endif /* defined(SRCH_PARSER_ENABLE_STRIPPING_TERM_TRAILINGS) */



    /* Set the wildcard search flag */
    psptSrchParserTerm->bWildCardSearch = false;

    /* Update the wildcard search flag - except for literal searches and regex searches */
    if ( (psptSrchParserTerm->uiFunctionID != SRCH_PARSER_FUNCTION_LITERAL_ID) && (psptSrchParserTerm->uiFunctionID != SRCH_PARSER_FUNCTION_REGEX_ID) ) {

        wchar_t     *pwcTermPtr = NULL;


        /* Check for non-escaped wildcards */
        for ( pwcTermPtr = psptSrchParserTerm->pwcTerm; *pwcTermPtr != L'\0'; pwcTermPtr++ ) {
        
            /* This will never match, but it makes the defines below a little easier to read */        
            if ( *pwcTermPtr == L' ' ) {
                ;
            }

#if defined(SRCH_PARSER_ENABLE_BACKSLASH_ESCAPE)

#if defined(SRCH_PARSER_ENABLE_MULTI_WILDCARD)
            else if ( (*pwcTermPtr != SRCH_PARSER_WILDCARD_ESCAPE_WCHAR) && (*(pwcTermPtr + 1) == SRCH_PARSER_WILDCARD_MULTI_WCHAR) ) {
                psptSrchParserTerm->bWildCardSearch = true;
                break;
            }
#endif /* defined(SRCH_PARSER_ENABLE_MULTI_WILDCARD) */
    
#if defined(SRCH_PARSER_ENABLE_SINGLE_WILDCARD)
            else if ( (*pwcTermPtr != SRCH_PARSER_WILDCARD_ESCAPE_WCHAR) && (*(pwcTermPtr + 1) == SRCH_PARSER_WILDCARD_SINGLE_WCHAR) ) {
                psptSrchParserTerm->bWildCardSearch = true;
                break;
            }
#endif /* defined(SRCH_PARSER_ENABLE_SINGLE_WILDCARD) */
    
#if defined(SRCH_PARSER_ENABLE_ALPHA_WILDCARD)
            else if ( (*pwcTermPtr != SRCH_PARSER_WILDCARD_ESCAPE_WCHAR) && (*(pwcTermPtr + 1) == SRCH_PARSER_WILDCARD_ALPHA_WCHAR) ) {
                psptSrchParserTerm->bWildCardSearch = true;
                break;
            }
#endif /* defined(SRCH_PARSER_ENABLE_ALPHA_WILDCARD) */
    
#if defined(SRCH_PARSER_ENABLE_NUMERIC_WILDCARD)
            else if ( (*pwcTermPtr != SRCH_PARSER_WILDCARD_ESCAPE_WCHAR) && (*(pwcTermPtr + 1) == SRCH_PARSER_WILDCARD_NUMERIC_WCHAR) ) {
                psptSrchParserTerm->bWildCardSearch = true;
                break;
            }
#endif /* defined(SRCH_PARSER_ENABLE_NUMERIC_WILDCARD) */

#else /* defined(SRCH_PARSER_ENABLE_BACKSLASH_ESCAPE) */

#if defined(SRCH_PARSER_ENABLE_MULTI_WILDCARD)
            else if ( *pwcTermPtr == SRCH_PARSER_WILDCARD_MULTI_WCHAR ) {
                psptSrchParserTerm->bWildCardSearch = true;
                break;
            }
#endif /* defined(SRCH_PARSER_ENABLE_MULTI_WILDCARD) */
    
#if defined(SRCH_PARSER_ENABLE_SINGLE_WILDCARD)
            else if ( *pwcTermPtr == SRCH_PARSER_WILDCARD_SINGLE_WCHAR ) {
                psptSrchParserTerm->bWildCardSearch = true;
                break;
            }
#endif /* defined(SRCH_PARSER_ENABLE_SINGLE_WILDCARD) */
    
#if defined(SRCH_PARSER_ENABLE_ALPHA_WILDCARD)
            else if ( *pwcTermPtr == SRCH_PARSER_WILDCARD_ALPHA_WCHAR ) {
                psptSrchParserTerm->bWildCardSearch = true;
                break;
            }
#endif /* defined(SRCH_PARSER_ENABLE_ALPHA_WILDCARD) */
    
#if defined(SRCH_PARSER_ENABLE_NUMERIC_WILDCARD)
            else if ( *pwcTermPtr == SRCH_PARSER_WILDCARD_NUMERIC_WCHAR ) {
                psptSrchParserTerm->bWildCardSearch = true;
                break;
            }
#endif /* defined(SRCH_PARSER_ENABLE_NUMERIC_WILDCARD) */

#endif /* defined(SRCH_PARSER_ENABLE_BACKSLASH_ESCAPE) */

        }
    }



#if defined(SRCH_PARSER_ENABLE_BACKSLASH_ESCAPE)
    /* Strip wildcard escapes - except for literal searches and regex searches */
    if ( (psptSrchParserTerm->uiFunctionID != SRCH_PARSER_FUNCTION_LITERAL_ID) && (psptSrchParserTerm->uiFunctionID != SRCH_PARSER_FUNCTION_REGEX_ID) ) {

        wchar_t     *pwcReadPtr = NULL;
        wchar_t     *pwcWritePtr = NULL;

        /* Check for escaped wildcards */
        for ( pwcReadPtr = psptSrchParserTerm->pwcTerm, pwcWritePtr = psptSrchParserTerm->pwcTerm; *pwcReadPtr != L'\0'; pwcReadPtr++, pwcWritePtr++ ) {
        
            /* This will never match, but it makes the defines below a little easier to read */        
            if ( *pwcReadPtr == L' ' ) {
                ;
            }

#if defined(SRCH_PARSER_ENABLE_MULTI_WILDCARD)
            else if ( (*pwcReadPtr == SRCH_PARSER_WILDCARD_ESCAPE_WCHAR) && (*(pwcReadPtr + 1) == SRCH_PARSER_WILDCARD_MULTI_WCHAR) ) {
                pwcReadPtr++;
            }
#endif /* defined(SRCH_PARSER_ENABLE_MULTI_WILDCARD) */
    
#if defined(SRCH_PARSER_ENABLE_SINGLE_WILDCARD)
            else if ( (*pwcReadPtr == SRCH_PARSER_WILDCARD_ESCAPE_WCHAR) && (*(pwcReadPtr + 1) == SRCH_PARSER_WILDCARD_SINGLE_WCHAR) ) {
                pwcReadPtr++;
            }
#endif /* defined(SRCH_PARSER_ENABLE_SINGLE_WILDCARD) */
    
#if defined(SRCH_PARSER_ENABLE_ALPHA_WILDCARD)
            else if ( (*pwcReadPtr == SRCH_PARSER_WILDCARD_ESCAPE_WCHAR) && (*(pwcReadPtr + 1) == SRCH_PARSER_WILDCARD_ALPHA_WCHAR) ) {
                pwcReadPtr++;
            }
#endif /* defined(SRCH_PARSER_ENABLE_ALPHA_WILDCARD) */
    
#if defined(SRCH_PARSER_ENABLE_NUMERIC_WILDCARD)
            else if ( (*pwcReadPtr == SRCH_PARSER_WILDCARD_ESCAPE_WCHAR) && (*(pwcReadPtr + 1) == SRCH_PARSER_WILDCARD_NUMERIC_WCHAR) ) {
                pwcReadPtr++;
            }
#endif /* defined(SRCH_PARSER_ENABLE_NUMERIC_WILDCARD) */

            /* Copy the character */
            *pwcWritePtr = *pwcReadPtr;
        }
        
        /* Null terminate */
        *pwcWritePtr = L'\0';
    }
#endif /* defined(SRCH_PARSER_ENABLE_NUMERIC_WILDCARD) */

/* printf("iSrchParserAddTermToTermCluster -  pwcToken: [%ls], pwcTerm: [%ls], pwcFieldName: [%ls], pwcRange: [%ls], pwcFunction: [%ls], pwcWeight: [%ls]: psptSrchParserTerm->bWildCardSearch: [%d].\n",  */
/*             pwcToken, pwcTerm, pwcFieldName, pwcRange, pwcFunction, pwcWeight, (int)psptSrchParserTerm->bWildCardSearch); */
/* printf("iSrchParserAddTermToTermCluster -  psptSrchParserTerm->pwcTerm: [%ls].\n", psptSrchParserTerm->pwcTerm); */



    /* Set the weight */
    if ( bUtlStringsIsWideStringNULL(pwcWeight) == false ) {

        boolean     bIsNumber = true;
        wchar_t     *pwcPtr = NULL;

        /* Test for a valid number */
        for ( pwcPtr = pwcWeight, bIsNumber = true; *pwcPtr != L'\0'; pwcPtr++ ) {
            if ( (iswdigit(*pwcPtr) == 0) && (*pwcPtr != L'-') && (*pwcPtr != L'+') && (*pwcPtr != L'.') ) {
                bIsNumber = false;
                break;
            }
        }

        /* Bail if the number is not valid */
        if ( bIsNumber == false ) {
            iError = SRCH_ParserInvalidTermWeight;
            goto bailFromiSrchParserAddTermToTermCluster;
        }

        psptSrchParserTerm->fTermWeight = s_wcstof(pwcWeight, NULL);
    }
    else {
        psptSrchParserTerm->fTermWeight = 0;
    }



#if defined(SRCH_PARSER_ENABLE_TERM_CLUSTER_PRUNNING)

/* printf("psptSrchParserTerm->pwcTerm: '%ls', psptcSrchParserTermCluster->uiOperatorID: %u\n", psptSrchParserTerm->pwcTerm, psptcSrchParserTermCluster->uiOperatorID); */

    /* Check if we can prune this term if we have an OR or an AND */
    if ( (psptcSrchParserTermCluster->uiOperatorID == SRCH_PARSER_OPERATOR_OR_ID) || (psptcSrchParserTermCluster->uiOperatorID == SRCH_PARSER_OPERATOR_AND_ID) ) {

        unsigned int    uiI = 0;

        /* Loop over each entry */
        for ( uiI = 0; uiI < psptcSrchParserTermCluster->uiTermsLength; uiI++ ) {
    
            boolean    bSame = true;

            /* It is a term cluster..., check if it is the same as the one we are trying to add */
            if ( psptcSrchParserTermCluster->puiTermTypeIDs[uiI] == SRCH_PARSER_TERM_TYPE_TERM_ID ) {
    
                struct srchParserTerm   *psptSrchParserTermPtr = (struct srchParserTerm *)psptcSrchParserTermCluster->ppvTerms[uiI];
    
/* printf("===>>> psptSrchParserTermPtr->pwcTerm: '%ls', uiI: %u\n", psptSrchParserTermPtr->pwcTerm, uiI); */

                if ( s_wcscmp(psptSrchParserTermPtr->pwcTerm, psptSrchParserTerm->pwcTerm) != 0 ) {
                    bSame = false;
                }
                
                if ( ((psptSrchParserTermPtr->pwcFieldName == NULL) && (psptSrchParserTerm->pwcFieldName != NULL)) || 
                        ((psptSrchParserTermPtr->pwcFieldName != NULL) && (psptSrchParserTerm->pwcFieldName == NULL)) ) {
                    bSame = false;
                }
    
                if ( (psptSrchParserTermPtr->pwcFieldName != NULL) && (psptSrchParserTerm->pwcFieldName != NULL) &&
                        (s_wcscmp(psptSrchParserTermPtr->pwcFieldName, psptSrchParserTerm->pwcFieldName) != 0) ) {
                    bSame = false;
                }
                
                if ( psptSrchParserTermPtr->uiFunctionID != psptSrchParserTerm->uiFunctionID ) {
                    bSame = false;
                }
                
                if ( psptSrchParserTermPtr->uiRangeID != psptSrchParserTerm->uiRangeID ) {
                    bSame = false;
                }
                
                if ( psptSrchParserTermPtr->bWildCardSearch != psptSrchParserTerm->bWildCardSearch ) {
                    bSame = false;
                }
                
                if ( psptSrchParserTermPtr->fTermWeight != psptSrchParserTerm->fTermWeight ) {
                    bSame = false;
                }
                
                if ( psptSrchParserTermPtr->bRequired != psptSrchParserTerm->bRequired ) {
                    bSame = false;
                }
            }
            /* It is a term cluster... */
            else if ( psptcSrchParserTermCluster->puiTermTypeIDs[uiI] == SRCH_PARSER_TERM_TYPE_TERM_CLUSTER_ID ) {
                bSame = false;
            }

            
            /* If the cluster was the same, we can prune it */
            if ( bSame == true ) {
                s_free(psptSrchParserTerm->pwcTerm);
                s_free(psptSrchParserTerm->pwcFieldName);
                s_free(psptSrchParserTerm);
                return (SRCH_NoError);
            }
        }
    }    
#endif    /* defined(SRCH_PARSER_ENABLE_TERM_CLUSTER_PRUNNING) */

    

    /* Extend the term list pointer */
    if ( (ppvTerms = (void **)s_realloc(psptcSrchParserTermCluster->ppvTerms, 
            (size_t)(sizeof(void *) * (psptcSrchParserTermCluster->uiTermsLength + 1)))) == NULL ) {
        iError = SRCH_MemError;
        goto bailFromiSrchParserAddTermToTermCluster;
    }

    /* Hand over the pointer */
    psptcSrchParserTermCluster->ppvTerms = ppvTerms;

    /* Extend the term type ID list pointer */
    if ( (puiTermTypeIDs = (unsigned int *)s_realloc(psptcSrchParserTermCluster->puiTermTypeIDs, 
            (size_t)(sizeof(unsigned int) * (psptcSrchParserTermCluster->uiTermsLength + 1)))) == NULL ) {
        iError = SRCH_MemError;
        goto bailFromiSrchParserAddTermToTermCluster;
    }

    /* Hand over the pointer */
    psptcSrchParserTermCluster->puiTermTypeIDs = puiTermTypeIDs;

    /* Add the new term and term type to the appropriate lists */
    psptcSrchParserTermCluster->ppvTerms[psptcSrchParserTermCluster->uiTermsLength] = (void *)psptSrchParserTerm;
    psptcSrchParserTermCluster->puiTermTypeIDs[psptcSrchParserTermCluster->uiTermsLength] = SRCH_PARSER_TERM_TYPE_TERM_ID;

    /* Increment the term list length */
    psptcSrchParserTermCluster->uiTermsLength++;


    /* Increment the search term count */
    pspSrchParser->uiSearchTermCount++;



    /* Bail label */
    bailFromiSrchParserAddTermToTermCluster:


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchParserCheckTermCluster()

    Purpose:    This function checks a term cluster. Currently we only check
                that the search is not in the form 'NOT term|search'.

    Parameters: pspSrchParser                   search parser structure
                psptcSrchParserTermCluster      search parser term cluster

    Globals:    none

    Returns:    SRCH error code

*/
static int iSrchParserCheckTermCluster
(
    struct srchParser *pspSrchParser,
    struct srchParserTermCluster *psptcSrchParserTermCluster
)
{

    int             iError = SRCH_NoError;
    unsigned int    uiI = 0;
    

    ASSERT(pspSrchParser != NULL);
    ASSERT(psptcSrchParserTermCluster != NULL);


    /* Return now if the term cluster is a valid operator but not a NOT operator */
    if ( (psptcSrchParserTermCluster->uiOperatorID != SRCH_PARSER_OPERATOR_NOT_ID) && (psptcSrchParserTermCluster->uiOperatorID != SRCH_PARSER_INVALID_ID) ) {
        return (SRCH_NoError);
    }
    
    /* Return an error if this is a 'NOT' operator and there is only one term, ie 'NOT term|search' */
    else if ( (psptcSrchParserTermCluster->uiOperatorID == SRCH_PARSER_OPERATOR_NOT_ID) && (psptcSrchParserTermCluster->uiTermsLength == 1) ) {
        return (SRCH_ParserInvalidNotOperator);
    }


    /* Loop over each entry */
    for ( uiI = 0; uiI < psptcSrchParserTermCluster->uiTermsLength; uiI++ ) {

        /* It is a term cluster... */
        if ( psptcSrchParserTermCluster->puiTermTypeIDs[uiI] == SRCH_PARSER_TERM_TYPE_TERM_CLUSTER_ID ) {

            struct srchParserTermCluster    *pptcParserTermClusterPtr = (struct srchParserTermCluster *)psptcSrchParserTermCluster->ppvTerms[uiI];

            /* Recurse */
            iError = iSrchParserCheckTermCluster(pspSrchParser, pptcParserTermClusterPtr);
            
            /* And bail early with the error code, we dont need to check further */
            break;
        }
    }


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchParserTermClusterToNormalizedSearchText()

    Purpose:    This function creates a bracketed search from a term cluster. 

    Parameters: pspSrchParser                           search parser structure
                psptcSrchParserTermCluster              search parser term cluster
                pvUtlNormalizedSearchWideStringBuffer   normalized search wide string buffer

    Globals:    none

    Returns:    SRCH error code

*/
static int iSrchParserTermClusterToNormalizedSearchText
(
    struct srchParser *pspSrchParser,
    struct srchParserTermCluster *psptcSrchParserTermCluster,
    void *pvUtlNormalizedSearchWideStringBuffer
)
{

    int             iError = SRCH_NoError;
    unsigned int    uiI = 0;
    wchar_t         pwcBuffer[SRCH_PARSER_MINIMUM_LENGTH + 1] = {L'\0'};
    

    ASSERT(pspSrchParser != NULL);
    ASSERT(psptcSrchParserTermCluster != NULL);
    ASSERT(pvUtlNormalizedSearchWideStringBuffer != NULL);


    /* Loop over each entry */
    for ( uiI = 0; uiI < psptcSrchParserTermCluster->uiTermsLength; uiI++ ) {

        /* Add search operator, if we are in between terms, if the operator is valid and if this is a boolean search */
        if ( (uiI > 0) && (psptcSrchParserTermCluster->uiOperatorID != SRCH_PARSER_INVALID_ID) && (pspSrchParser->uiSearchTypeID == SRCH_PARSER_MODIFIER_SEARCH_TYPE_BOOLEAN_ID) ) {
    
            /* Space */
            iUtlWideStringBufferAppend(pvUtlNormalizedSearchWideStringBuffer, L" ");

            /* Get the operator */
            s_wcsnncpy(pwcBuffer, pwcSrchParserGetStringFromID(pspSrchParser, psptcSrchParserTermCluster->uiOperatorID), SRCH_PARSER_MINIMUM_LENGTH + 1);
        
            /* Convert the case of the operator as needed */
            if ( pspSrchParser->uiOperatorCaseID == SRCH_PARSER_MODIFIER_OPERATOR_CASE_UPPER_ID ) {
                pwcLngCaseConvertWideStringToUpperCase(pwcBuffer);
            }
            else if ( pspSrchParser->uiOperatorCaseID == SRCH_PARSER_MODIFIER_OPERATOR_CASE_LOWER_ID ) {
                pwcLngCaseConvertWideStringToLowerCase(pwcBuffer);
            }

            iUtlWideStringBufferAppend(pvUtlNormalizedSearchWideStringBuffer, pwcBuffer);
        
            /* Term distance */
            if ( psptcSrchParserTermCluster->iTermDistance != 0 ) {
                swprintf(pwcBuffer, SRCH_PARSER_MINIMUM_LENGTH + 1, L"[%ls%d]", ((psptcSrchParserTermCluster->bDistanceOrderMatters == true) && (psptcSrchParserTermCluster->iTermDistance > 0)) ? L"+" : L"",
                    psptcSrchParserTermCluster->iTermDistance);
                iUtlWideStringBufferAppend(pvUtlNormalizedSearchWideStringBuffer, pwcBuffer);
            }
        }


        /* Add a space if we are between terms/term clusters */
        if ( uiI > 0 ) {
            /* Space */
            iUtlWideStringBufferAppend(pvUtlNormalizedSearchWideStringBuffer, L" ");
        }
    
            
        /* It is a term... */
        if ( psptcSrchParserTermCluster->puiTermTypeIDs[uiI] == SRCH_PARSER_TERM_TYPE_TERM_ID ) {

            struct srchParserTerm   *pstSrchParserTermPtr = (struct srchParserTerm *)psptcSrchParserTermCluster->ppvTerms[uiI];


            /* Requirement - put this at the start to make it really clear as opposed to embbeding it in the text */
            if ( pstSrchParserTermPtr->bRequired == true ) {
                iUtlWideStringBufferAppend(pvUtlNormalizedSearchWideStringBuffer, SRCH_PARSER_OPERATOR_PLUS_WSTRING);
            }

            /* Field name */
            if ( bUtlStringsIsWideStringNULL(pstSrchParserTermPtr->pwcFieldName) == false ) {
                iUtlWideStringBufferAppend(pvUtlNormalizedSearchWideStringBuffer, pstSrchParserTermPtr->pwcFieldName);
            }
            
            /* Range ID */
#if defined(SRCH_PARSER_ENABLE_FIELDLESS_OPERATOR)
            if ( (pstSrchParserTermPtr->uiRangeID != SRCH_PARSER_INVALID_ID) && (pstSrchParserTermPtr->uiRangeID != SRCH_PARSER_RANGE_EQUAL_ID) ) {
                iUtlWideStringBufferAppend(pvUtlNormalizedSearchWideStringBuffer, pwcSrchParserGetStringFromID(pspSrchParser, pstSrchParserTermPtr->uiRangeID));
            }
#else
            if ( (pstSrchParserTermPtr->uiRangeID != SRCH_PARSER_INVALID_ID) && (bUtlStringsIsWideStringNULL(pstSrchParserTermPtr->pwcFieldName) == false) ) {
                iUtlWideStringBufferAppend(pvUtlNormalizedSearchWideStringBuffer, pwcSrchParserGetStringFromID(pspSrchParser, pstSrchParserTermPtr->uiRangeID));
            }
#endif    /* defined(SRCH_PARSER_ENABLE_FIELDLESS_OPERATOR) */
            
            /* Function ID */
            if ( pstSrchParserTermPtr->uiFunctionID != SRCH_PARSER_INVALID_ID ) {
                iUtlWideStringBufferAppend(pvUtlNormalizedSearchWideStringBuffer, pwcSrchParserGetStringFromID(pspSrchParser, pstSrchParserTermPtr->uiFunctionID));
                iUtlWideStringBufferAppend(pvUtlNormalizedSearchWideStringBuffer, SRCH_PARSER_CHARACTER_LSQUARE_BRACKET_WSTRING);
            }
            
            /* Term */
            if ( bUtlStringsIsWideStringNULL(pstSrchParserTermPtr->pwcTerm) == false ) {
                iUtlWideStringBufferAppend(pvUtlNormalizedSearchWideStringBuffer, pstSrchParserTermPtr->pwcTerm);
            }

            /* Term weight */
            if ( pstSrchParserTermPtr->fTermWeight != 0 ) {
                swprintf(pwcBuffer, SRCH_PARSER_MINIMUM_LENGTH + 1, L"[%.2f]", pstSrchParserTermPtr->fTermWeight);
                iUtlWideStringBufferAppend(pvUtlNormalizedSearchWideStringBuffer, pwcBuffer);
            }

            /* Function ID closure */
            if ( pstSrchParserTermPtr->uiFunctionID != SRCH_PARSER_INVALID_ID ) {
                iUtlWideStringBufferAppend(pvUtlNormalizedSearchWideStringBuffer, SRCH_PARSER_CHARACTER_RSQUARE_BRACKET_WSTRING);
            }
        
        }
        /* It is a term cluster... */
        else if ( psptcSrchParserTermCluster->puiTermTypeIDs[uiI] == SRCH_PARSER_TERM_TYPE_TERM_CLUSTER_ID ) {

            struct srchParserTermCluster    *pptcParserTermClusterPtr = (struct srchParserTermCluster *)psptcSrchParserTermCluster->ppvTerms[uiI];

            /* L paren */
            iUtlWideStringBufferAppend(pvUtlNormalizedSearchWideStringBuffer, SRCH_PARSER_OPERATOR_LPAREN_WSTRING);

            /* Recurse */
            if ( (iError = iSrchParserTermClusterToNormalizedSearchText(pspSrchParser, pptcParserTermClusterPtr, pvUtlNormalizedSearchWideStringBuffer)) != SRCH_NoError ) {
                return (iError);
            }
    
            /* R paren */
            iUtlWideStringBufferAppend(pvUtlNormalizedSearchWideStringBuffer, SRCH_PARSER_OPERATOR_RPAREN_WSTRING);
        }
    }

    
    return (SRCH_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   uiSrchParserGetRangeIDFromStartOfString()

    Purpose:    This function checks the start of a string and returns the ID
                of the range at the start of it.

    Parameters: pspSrchParser       search parser structure
                pwcTokenString      token string to validate

    Globals:    pptParserTokenRangeListGlobal
                        

    Returns:    The token ID if the token is valid, SRCH_PARSER_INVALID_ID otherwise.

*/
static unsigned int uiSrchParserGetRangeIDFromStartOfString
(
    struct srchParser *pspSrchParser,
    wchar_t *pwcTokenString
)
{

    unsigned int                uiTokenStringLength = 0;
    struct srchParserToken      *psptSrchParserTokenListPtr = pptParserTokenRangeListGlobal;


    ASSERT(pspSrchParser != NULL);


    /* Check for NULLs */
    if ( bUtlStringsIsWideStringNULL(pwcTokenString) == true ) {
        return (SRCH_PARSER_INVALID_ID);
    }

    /* Check the string token list */
    for ( psptSrchParserTokenListPtr = pptParserTokenRangeListGlobal, uiTokenStringLength = s_wcslen(pwcTokenString); psptSrchParserTokenListPtr->pwcTokenString != NULL; psptSrchParserTokenListPtr++ ) {

        /* Check for the presence of the starting token */
        if ( (uiTokenStringLength >= s_wcslen(psptSrchParserTokenListPtr->pwcTokenString)) && 
                (s_wcsncmp(pwcTokenString, psptSrchParserTokenListPtr->pwcTokenString, s_wcslen(psptSrchParserTokenListPtr->pwcTokenString)) == 0) ) {

            return (psptSrchParserTokenListPtr->uiTokenID);
        }
    }


    return (SRCH_PARSER_INVALID_ID);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   uiSrchParserGetRangeIDFromEndOfString()

    Purpose:    This function checks the end of a string and returns the ID
                of the range at the end of it.

    Parameters: pspSrchParser       search parser structure
                pwcTokenString      token string to validate

    Globals:    pptParserTokenRangeListGlobal
                        

    Returns:    The token ID if the token is valid, SRCH_PARSER_INVALID_ID otherwise.

*/
static unsigned int uiSrchParserGetRangeIDFromEndOfString
(
    struct srchParser *pspSrchParser,
    wchar_t *pwcTokenString
)
{

    unsigned int                uiTokenStringLength = 0;
    struct srchParserToken      *psptSrchParserTokenListPtr = pptParserTokenRangeListGlobal;


    ASSERT(pspSrchParser != NULL);


    /* Check for NULLs */
    if ( bUtlStringsIsWideStringNULL(pwcTokenString) == true ) {
        return (SRCH_PARSER_INVALID_ID);
    }

    /* Check the string token list */
    for ( psptSrchParserTokenListPtr = pptParserTokenRangeListGlobal, uiTokenStringLength = s_wcslen(pwcTokenString); psptSrchParserTokenListPtr->pwcTokenString != NULL; psptSrchParserTokenListPtr++ ) {

        /* Check for the presence of the trailing token */
        if ( (uiTokenStringLength >= s_wcslen(psptSrchParserTokenListPtr->pwcTokenString)) && 
                (s_wcscmp(pwcTokenString + (uiTokenStringLength - s_wcslen(psptSrchParserTokenListPtr->pwcTokenString)), psptSrchParserTokenListPtr->pwcTokenString) == 0) ) {

            return (psptSrchParserTokenListPtr->uiTokenID);
        }
    }


    return (SRCH_PARSER_INVALID_ID);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   uiSrchParserGetIDFromString()

    Purpose:    This function returns the ID for the passed string.

    Parameters: pspSrchParser       search parser structure
                pwcTokenString      token string to validate
                uiTokenListID       token list ID to use

    Globals:    pptParserTokenRangeListGlobal, pptParserTokenOperatorListGlobal, 
                        pptParserTokenModifierListGlobal, pptParserTokenSortOrderListGlobal, 
                        pptParserTokenSortFieldNameListGlobal, pptParserTokenFunctionListGlobal
                        

    Returns:    The token ID if the string is valid, SRCH_PARSER_INVALID_ID otherwise.

*/
static unsigned int uiSrchParserGetIDFromString
(
    struct srchParser *pspSrchParser,
    wchar_t *pwcTokenString,
    unsigned int uiTokenListID
)
{

    struct srchParserToken      *psptSrchParserTokenListPtr = NULL;


    ASSERT(pspSrchParser != NULL);


    /* Check for NULLs */
    if ( bUtlStringsIsWideStringNULL(pwcTokenString) == true ) {
        return (SRCH_PARSER_INVALID_ID);
    }


    /* Catch the situation where the search is not a boolean search and we are checking for a boolean token */
    if ( (pspSrchParser->uiSearchTypeID != SRCH_PARSER_MODIFIER_SEARCH_TYPE_BOOLEAN_ID) && (uiTokenListID == SRCH_PARSER_LIST_OPERATOR_ID) ) {
        return (SRCH_PARSER_INVALID_ID);
    }


    /* Get the appropriate token list structure */
    if ( uiTokenListID == SRCH_PARSER_LIST_RANGE_ID ) {
        psptSrchParserTokenListPtr = pptParserTokenRangeListGlobal;
    }
    else if ( uiTokenListID == SRCH_PARSER_LIST_OPERATOR_ID ) {
        psptSrchParserTokenListPtr = pptParserTokenOperatorListGlobal;
    }
    else if ( uiTokenListID == SRCH_PARSER_LIST_MODIFIER_ID ) {
        psptSrchParserTokenListPtr = pptParserTokenModifierListGlobal;
    }
    else if ( uiTokenListID == SRCH_PARSER_LIST_SORT_ORDER_ID ) {
        psptSrchParserTokenListPtr = pptParserTokenSortOrderListGlobal;
    }
    else if ( uiTokenListID == SRCH_PARSER_LIST_SORT_FIELD_NAME_ID ) {
        psptSrchParserTokenListPtr = pptParserTokenSortFieldNameListGlobal;
    }
    else if ( uiTokenListID == SRCH_PARSER_LIST_FUNCTION_ID ) {
        psptSrchParserTokenListPtr = pptParserTokenFunctionListGlobal;
    }
    else {
        return (SRCH_PARSER_INVALID_ID);
    }

/*     iUtlLogDebug(UTL_LOG_CONTEXT, "pwcTokenString [%ls].", pwcTokenString); */


    /* Check the string case for the operator token list */
    if ( uiTokenListID == SRCH_PARSER_LIST_OPERATOR_ID ) {
        
        if ( pspSrchParser->uiOperatorCaseID == SRCH_PARSER_MODIFIER_OPERATOR_CASE_UPPER_ID ) {
        
            /* This is an invalid token if it is not all in upper case */
            if ( bLngCaseIsWideStringAllUpperCase(pwcTokenString) == false ) {
                return (SRCH_PARSER_INVALID_ID);
            }
        }
        else if ( pspSrchParser->uiOperatorCaseID == SRCH_PARSER_MODIFIER_OPERATOR_CASE_LOWER_ID ) {
    
            /* This is an invalid token if it is not all in lower case */
            if ( bLngCaseIsWideStringAllLowerCase(pwcTokenString) == false ) {
                return (SRCH_PARSER_INVALID_ID);
            }
        }
        else if ( pspSrchParser->uiOperatorCaseID == SRCH_PARSER_MODIFIER_OPERATOR_CASE_ANY_ID ) {
            ;
        }
    }
    

    /* Check the string token list */
    for ( ; psptSrchParserTokenListPtr->pwcTokenString != NULL; psptSrchParserTokenListPtr++ ) {

/*         if ( uiTokenListID == SRCH_PARSER_LIST_OPERATOR_ID ) { */
/*             iUtlLogDebug(UTL_LOG_CONTEXT, "psptSrchParserTokenListPtr->pwcTokenString [%ls], psptSrchParserTokenListPtr->pwcTokenTrailerString [%sl].",  */
/*                     psptSrchParserTokenListPtr->pwcTokenString, pwcUtlStringsGetPrintableWideString(psptSrchParserTokenListPtr->pwcTokenTrailerString)); */
/*         } */


        /* Are we looking for trailing tokens? */
        if ( psptSrchParserTokenListPtr->pwcTokenTrailerString != NULL ) {
            
            /* There could be a trailing token, so we check it versus its status */
            
            /* Required trailing token */
            if ( psptSrchParserTokenListPtr->uiTokenTrailerStatus == SRCH_PARSER_TRAILER_REQUIRED ) {
            
                /* Check for the presence of the trailing token */
                if ( (s_wcsncasecmp(pwcTokenString, psptSrchParserTokenListPtr->pwcTokenString, s_wcslen(psptSrchParserTokenListPtr->pwcTokenString)) == 0) && 
                        (s_wcslen(pwcTokenString) > s_wcslen(psptSrchParserTokenListPtr->pwcTokenString)) &&
                        (s_wcschr(psptSrchParserTokenListPtr->pwcTokenTrailerString, pwcTokenString[s_wcslen(psptSrchParserTokenListPtr->pwcTokenString)]) != NULL) ) {
                    return (psptSrchParserTokenListPtr->uiTokenID);
                }
            }
            
            /* Optional trailing token */
            else if ( psptSrchParserTokenListPtr->uiTokenTrailerStatus == SRCH_PARSER_TRAILER_OPTIONAL ) {
                    
                /* Check for the presence and absence of the trailing token */
                if ( (s_wcscasecmp(pwcTokenString, psptSrchParserTokenListPtr->pwcTokenString) == 0) ||
                        ((s_wcsncasecmp(pwcTokenString, psptSrchParserTokenListPtr->pwcTokenString, s_wcslen(psptSrchParserTokenListPtr->pwcTokenString)) == 0) && 
                        (s_wcslen(pwcTokenString) > s_wcslen(psptSrchParserTokenListPtr->pwcTokenString)) &&
                        (s_wcschr(psptSrchParserTokenListPtr->pwcTokenTrailerString, pwcTokenString[s_wcslen(psptSrchParserTokenListPtr->pwcTokenString)]) != NULL)) ) {
                    return (psptSrchParserTokenListPtr->uiTokenID);
                }
            }
            
            /* No trailing token */
            else if ( psptSrchParserTokenListPtr->uiTokenTrailerStatus == SRCH_PARSER_TRAILER_NONE ) {
                    
                /* A token was specified in the structure but we are told not to use it - this is really a structure error */
                if ( s_wcscasecmp(pwcTokenString, psptSrchParserTokenListPtr->pwcTokenString) == 0 ) {
                    return (psptSrchParserTokenListPtr->uiTokenID);
                }
            }
        }
        else {
            
            /* There is no trailing token, so we check it versus its status */

            /* Required trailing token */
            if ( psptSrchParserTokenListPtr->uiTokenTrailerStatus == SRCH_PARSER_TRAILER_REQUIRED ) {

                /* A trailing token is required but we did not know what to look for - this is really a structure error */
                return (SRCH_PARSER_INVALID_ID);
            }            

            /* Optional trailing token */
            else if ( psptSrchParserTokenListPtr->uiTokenTrailerStatus == SRCH_PARSER_TRAILER_OPTIONAL ) {

                /* A trailing token is optional,  but we did not know what to look for so we do an exact match - this is really a structure error */
                if ( s_wcscasecmp(pwcTokenString, psptSrchParserTokenListPtr->pwcTokenString) == 0 ) {
                    return (psptSrchParserTokenListPtr->uiTokenID);
                }
            }
            
            /* No trailing token */
            else if ( psptSrchParserTokenListPtr->uiTokenTrailerStatus == SRCH_PARSER_TRAILER_NONE ) {
                    
                /* A trailing token, is not supposed to be there, so we do an exact match */
                if ( s_wcscasecmp(pwcTokenString, psptSrchParserTokenListPtr->pwcTokenString) == 0 ) {
                    return (psptSrchParserTokenListPtr->uiTokenID);
                }
            }
        }
    }
    
    
    
    if ( uiTokenListID == SRCH_PARSER_LIST_MODIFIER_ID ) {

        if ( (s_wcsncasecmp(pwcTokenString, SRCH_PARSER_CHARACTER_LBRACKET_WSTRING, s_wcslen(SRCH_PARSER_CHARACTER_LBRACKET_WSTRING)) == 0) &&
                (s_wcsncasecmp(pwcTokenString + s_wcslen(pwcTokenString) - 1, SRCH_PARSER_CHARACTER_RBRACKET_WSTRING, s_wcslen(SRCH_PARSER_CHARACTER_RBRACKET_WSTRING)) == 0) ) {
            return (SRCH_PARSER_MODIFIER_UNKNOWN_ID);
        }
    }


    return (SRCH_PARSER_INVALID_ID);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchParserGetAnsiDateFromDateName()

    Purpose:    Returns ansi date for the data name string.

    Parameters: pspSrchParser   search parser structure
                pwcRange        range, gets side-effected
                pwcDateName     date name string
                pulAnsiDate     return pointer for the ansi date

    Globals:    none

    Returns:    SRCH error code

*/
static int iSrchParserGetAnsiDateFromDateName
(
    struct srchParser *pspSrchParser,
    wchar_t *pwcRange,
    wchar_t *pwcDateName,
    unsigned long *pulAnsiDate
)
{


    struct srchParserDateName   *pspdnParserDateNamePtr = NULL;
    unsigned int                uiDateNameID = SRCH_PARSER_DATE_NAME_INVALID_ID;
    time_t                      tLocalTime = (time_t)0;
    struct tm                   tmTime;
    unsigned char               pucAnsiDate[15] = {'\0'};


    ASSERT(pspSrchParser != NULL);
    ASSERT(bUtlStringsIsWideStringNULL(pwcRange) == false);
    ASSERT(bUtlStringsIsWideStringNULL(pwcDateName) == false);
    ASSERT(pulAnsiDate != NULL);


    /* Loop over the parser date offset structures to see if we match on one */
    for ( pspdnParserDateNamePtr = pspdnParserDateNameListGlobal; pspdnParserDateNamePtr->uiDateNameID != SRCH_PARSER_DATE_NAME_INVALID_ID; pspdnParserDateNamePtr++ ) {
        if ( s_wcscasecmp(pspdnParserDateNamePtr->pwcDateName, pwcDateName) == 0 ) {
            uiDateNameID = pspdnParserDateNamePtr->uiDateNameID;
            break;
        }
    }
    
    /* Check if we have a valid date offset */
    if ( uiDateNameID == SRCH_PARSER_DATE_NAME_INVALID_ID ) {
        return (SRCH_ParserInvalidDate);
    }
    

    /* Get the current time and extract a time structure from it */
    tLocalTime = s_time(NULL);
    if ( s_localtime_r(&tLocalTime, &tmTime) == NULL ) {
        return (SRCH_ParserInvalidDate);
    }


    /* Process based on date offset */
    switch ( uiDateNameID ) {

        case SRCH_PARSER_DATE_NAME_TODAY_ID:

            /* Create the ansi date string */
            snprintf(pucAnsiDate, 15, "%04d%02d%02d000000", tmTime.tm_year + 1900, tmTime.tm_mon + 1, tmTime.tm_mday);
            
            break;
            
        
        case SRCH_PARSER_DATE_NAME_YESTERDAY_ID:

            /* Wind back 24 hours and work out the local time again */
            tLocalTime -= 60 * 60 * 24;
            if ( s_localtime_r(&tLocalTime, &tmTime) == NULL ) {
                return (SRCH_ParserInvalidDate);
            }
    
            /* Create the ansi date string */
            snprintf(pucAnsiDate, 15, "%04d%02d%02d000000", tmTime.tm_year + 1900, tmTime.tm_mon + 1, tmTime.tm_mday);
            
            break;
            
        
        case SRCH_PARSER_DATE_NAME_THIS_WEEK_ID:

            /* Wind back to the start of the week and work out the local time again */
            tLocalTime -= 60 * 60 * 24 * ((tmTime.tm_wday == 0) ? 6 : (tmTime.tm_wday - 1));
            if ( s_localtime_r(&tLocalTime, &tmTime) == NULL ) {
                return (SRCH_ParserInvalidDate);
            }
    
            /* Create the ansi date string */
            snprintf(pucAnsiDate, 15, "%04d%02d%02d000000", tmTime.tm_year + 1900, tmTime.tm_mon + 1, tmTime.tm_mday);

            /* Side effect the range */
            if ( uiSrchParserGetRangeIDFromString(pspSrchParser, pwcRange) == SRCH_PARSER_RANGE_EQUAL_ID ) {
                s_wcscpy(pwcRange, SRCH_PARSER_RANGE_GREATER_OR_EQUAL_WSTRING);
            }

            break;


        case SRCH_PARSER_DATE_NAME_LAST_WEEK_ID:

            /* Wind back two weeks and work out the local time again */
            tLocalTime -= 60 * 60 * 24 * (tmTime.tm_wday + 6);
            if ( s_localtime_r(&tLocalTime, &tmTime) == NULL ) {
                return (SRCH_ParserInvalidDate);
            }
    
            /* Create the ansi date string */
            snprintf(pucAnsiDate, 15, "%04d%02d%02d000000", tmTime.tm_year + 1900, tmTime.tm_mon + 1, tmTime.tm_mday);

            break;


        case SRCH_PARSER_DATE_NAME_THIS_MONTH_ID:

            /* Create the ansi date string */
            snprintf(pucAnsiDate, 15, "%04d%02d00000000", tmTime.tm_year + 1900, tmTime.tm_mon + 1);

            break;


        case SRCH_PARSER_DATE_NAME_LAST_MONTH_ID:

            /* Decrement the month, crank back a year if needed */
            if ( tmTime.tm_mon > 0 ) {
                tmTime.tm_mon -= 1;
            }
            else {
                tmTime.tm_mon = 11;
                tmTime.tm_year -= 1;
            }
    
            /* Create the ansi date string */
            snprintf(pucAnsiDate, 15, "%04d%02d00000000", tmTime.tm_year + 1900, tmTime.tm_mon + 1);
    
            break;


        case SRCH_PARSER_DATE_NAME_THIS_YEAR_ID:

            /* Create the ansi date string */
            snprintf(pucAnsiDate, 15, "%04d0000000000", tmTime.tm_year + 1900);
    
            break;


        case SRCH_PARSER_DATE_NAME_LAST_YEAR_ID:

            /* Create the ansi date string */
            snprintf(pucAnsiDate, 15, "%04d0000000000", (tmTime.tm_year + 1900) - 1);
    
            break;

     
        case SRCH_PARSER_DATE_NAME_SUNDAY_ID:
        case SRCH_PARSER_DATE_NAME_MONDAY_ID:
        case SRCH_PARSER_DATE_NAME_TUESDAY_ID:
        case SRCH_PARSER_DATE_NAME_WEDNESDAY_ID:
        case SRCH_PARSER_DATE_NAME_THURSDAY_ID:
        case SRCH_PARSER_DATE_NAME_FRIDAY_ID:
        case SRCH_PARSER_DATE_NAME_SATURDAY_ID:

/*             iUtlLogDebug(UTL_LOG_CONTEXT, "(uiDateNameID - SRCH_PARSER_DATE_NAME_SUNDAY: %d, tmTime.tm_wday: %d", uiDateNameID - SRCH_PARSER_DATE_NAME_SUNDAY, tmTime.tm_wday); */

            /* Wind back the day and work out the local time again */
            if ( (uiDateNameID - SRCH_PARSER_DATE_NAME_SUNDAY_ID) <= tmTime.tm_wday ) {
                tLocalTime -= 60 * 60 * 24 * (tmTime.tm_wday - (uiDateNameID - SRCH_PARSER_DATE_NAME_SUNDAY_ID));
/*                iUtlLogDebug(UTL_LOG_CONTEXT, "(tmTime.tm_wday - (uiDateNameID - SRCH_PARSER_DATE_NAME_SUNDAY)): %d", (tmTime.tm_wday - (uiDateNameID - SRCH_PARSER_DATE_NAME_SUNDAY))); */
            }
            else {
                tLocalTime -= 60 * 60 * 24 * (tmTime.tm_wday + (SRCH_PARSER_DATE_NAME_SATURDAY_ID - uiDateNameID) + 1);
/*                iUtlLogDebug(UTL_LOG_CONTEXT, "(tmTime.tm_wday + (SRCH_PARSER_DATE_NAME_SATURDAY - uiDateNameID) + 1): %d", (tmTime.tm_wday + (SRCH_PARSER_DATE_NAME_SATURDAY - uiDateNameID) + 1)); */
            }
            if ( s_localtime_r(&tLocalTime, &tmTime) == NULL ) {
                return (SRCH_ParserInvalidDate);
            }
    
            /* Create the ansi date string */
            snprintf(pucAnsiDate, 15, "%04d%02d%02d000000", tmTime.tm_year + 1900, tmTime.tm_mon + 1, tmTime.tm_mday);
    
            break;


        case SRCH_PARSER_DATE_NAME_JANUARY_ID:
        case SRCH_PARSER_DATE_NAME_FEBRUARY_ID:
        case SRCH_PARSER_DATE_NAME_MARCH_ID:
        case SRCH_PARSER_DATE_NAME_APRIL_ID:
        case SRCH_PARSER_DATE_NAME_MAY_ID:
        case SRCH_PARSER_DATE_NAME_JUNE_ID:
        case SRCH_PARSER_DATE_NAME_JULY_ID:
        case SRCH_PARSER_DATE_NAME_AUGUST_ID:
        case SRCH_PARSER_DATE_NAME_SEPTEMBER_ID:
        case SRCH_PARSER_DATE_NAME_OCTOBER_ID:
        case SRCH_PARSER_DATE_NAME_NOVEMBER_ID:
        case SRCH_PARSER_DATE_NAME_DECEMBER_ID:

            /* Decrement the year if needed */
            if ( (uiDateNameID - SRCH_PARSER_DATE_NAME_JANUARY_ID) > tmTime.tm_mon ) {
                tmTime.tm_year -= 1;
            }

            /* Create the ansi date string */
            snprintf(pucAnsiDate, 15, "%04d%02d00000000", tmTime.tm_year + 1900, (uiDateNameID - SRCH_PARSER_DATE_NAME_JANUARY_ID) + 1);
    
            break;

    
        default:
            return (SRCH_ParserInvalidDate);

    }

    
    /* Convert the ansi date string to number and set the return pointer */
    *pulAnsiDate = s_strtol(pucAnsiDate, NULL, 10);


    return (SRCH_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchParserFreeTerms()

    Purpose:    This function frees the terms. 

    Parameters: pstSrchParserTerm           search parser term structure (optional)
                uiSrchParserTermLength      number of parser terms

    Globals:    none

    Returns:    SRCH error code

*/
static int iSrchParserFreeTerms
(
    struct srchParserTerm *pstSrchParserTerm, 
    unsigned int uiSrchParserTermLength
)
{

    struct srchParserTerm   *pstSrchParserTermPtr = NULL;
    unsigned int            uiI = 0;


    /* Check the search parser term structure */
    if ( pstSrchParserTerm == NULL ) {
        return (SRCH_NoError);
    }


    /* Loop over each entry, freeing as we go along */
    for ( uiI = 0, pstSrchParserTermPtr = pstSrchParserTerm; uiI < uiSrchParserTermLength; uiI++, pstSrchParserTermPtr++ ) {
        s_free(pstSrchParserTermPtr->pwcTerm);
        s_free(pstSrchParserTermPtr->pwcFieldName);
    }
    s_free(pstSrchParserTerm);


    return (SRCH_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchParserFreeTermCluster()

    Purpose:    This function frees the parser term cluster. 

    Parameters: psptcSrchParserTermCluster      search parser term cluster structure (optional)

    Globals:    none

    Returns:    SRCH error code

*/
static int iSrchParserFreeTermCluster
(
    struct srchParserTermCluster *psptcSrchParserTermCluster
)
{

    unsigned int    uiI = 0;


    /* Check search parser term cluster */
    if ( psptcSrchParserTermCluster == NULL ) {
        return (SRCH_NoError);
    }


    /* Loop over each entry */
    for ( uiI = 0; uiI < psptcSrchParserTermCluster->uiTermsLength; uiI++ ) {

        /* It is a term... */
        if ( psptcSrchParserTermCluster->puiTermTypeIDs[uiI] == SRCH_PARSER_TERM_TYPE_TERM_ID ) {
            iSrchParserFreeTerms((struct srchParserTerm *)psptcSrchParserTermCluster->ppvTerms[uiI], 1);
        }
        /* It is a term cluster... */
        else if ( psptcSrchParserTermCluster->puiTermTypeIDs[uiI] == SRCH_PARSER_TERM_TYPE_TERM_CLUSTER_ID ) {
            iSrchParserFreeTermCluster((struct srchParserTermCluster *)psptcSrchParserTermCluster->ppvTerms[uiI]);
        }
    }

    /* Free the lists */
    s_free(psptcSrchParserTermCluster->ppvTerms);
    s_free(psptcSrchParserTermCluster->puiTermTypeIDs);

    /* Finally free the term cluster itself */
    s_free(psptcSrchParserTermCluster);


    return (SRCH_NoError);

}


/*---------------------------------------------------------------------------*/

/*

    Function:   iSrchParserFreeNumbers()

    Purpose:    This function frees the search parser numbers. 

    Parameters: pspnSrchParserNumbers       search parser numbers structure (optional)
                uiSrchParserNumberLength    search parser numbers length

    Globals:    none

    Returns:    SRCH error code

*/
static int iSrchParserFreeNumbers
(
    struct srchParserNumber *pspnSrchParserNumbers, 
    unsigned int uiSrchParserNumberLength
)
{

    /* Check the search parser numbers structure */
    if ( pspnSrchParserNumbers == NULL ) {
        return (SRCH_NoError);
    }


    s_free(pspnSrchParserNumbers);


    return (SRCH_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchParserFreeFilters()

    Purpose:    This function frees the search parser filters. 

    Parameters: pspfSrchParserFilters       search parser filters structure (optional)
                uiSrchParserStringsLength   search parser filters length

    Globals:    none

    Returns:    SRCH error code

*/
static int iSrchParserFreeFilters
(
    struct srchParserFilter *pspfSrchParserFilters, 
    unsigned int uiSrchParserFiltersLength
)
{

    struct srchParserFilter     *pspfSrchParserFiltersPtr = NULL;
    unsigned int                uiI = 0;


    /* Check the search parser filters structure */
    if ( pspfSrchParserFilters == NULL ) {
        return (SRCH_NoError);
    }


    /* Loop over each entry, freeing as we go along */
    for ( uiI = 0, pspfSrchParserFiltersPtr = pspfSrchParserFilters; uiI < uiSrchParserFiltersLength; uiI++, pspfSrchParserFiltersPtr++ ) {
        s_free(pspfSrchParserFiltersPtr->pwcFilter);
    }
    s_free(pspfSrchParserFilters);


    return (SRCH_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchParserPrintTermCluster()

    Purpose:    This function prints the parser term cluster, for debug only. 

    Parameters: pvSrchParser                    search parser structure
                psptcSrchParserTermCluster      search parser term cluster structure (optional)
                uiLevel                         indent level

    Globals:    none

    Returns:    SRCH error code

*/
int iSrchParserPrintTermCluster
(
    void *pvSrchParser,
    struct srchParserTermCluster *psptcSrchParserTermCluster,
    unsigned int uiLevel
)
{

    int                 iError = SRCH_NoError;
    struct srchParser   *pspSrchParser = (struct srchParser *)pvSrchParser;
    unsigned int        uiI = 0;
    wchar_t             pwcBuffer[SRCH_PARSER_MINIMUM_LENGTH + 1] = {'\0'};


    /* Check the search parser structure */
    if ( pspSrchParser == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pspSrchParser' parameter passed to 'iSrchParserPrintTermCluster'."); 
        return (SRCH_ParserInvalidParser);
    }

    /* Check the search parser term cluster structure */
    if ( psptcSrchParserTermCluster == NULL ) {
        return (SRCH_NoError);
    }

    
    /* Nice indentation */
    for ( uiI = 0; uiI < uiLevel; uiI++ ) {
        s_wcsnncat(pwcBuffer, L"  ", SRCH_PARSER_MINIMUM_LENGTH, SRCH_PARSER_MINIMUM_LENGTH + 1);
    }


    /* Loop over each entry */
    for ( uiI = 0; uiI < psptcSrchParserTermCluster->uiTermsLength; uiI++ ) {

        /* It is a term... */
        if ( psptcSrchParserTermCluster->puiTermTypeIDs[uiI] == SRCH_PARSER_TERM_TYPE_TERM_ID ) {

            struct srchParserTerm   *pstSrchParserTermPtr = (struct srchParserTerm *)psptcSrchParserTermCluster->ppvTerms[uiI];

            printf("%ls Term: pwcTerm [%ls], pwcFieldName [%ls], uiRangeID [%ls], uiFunctionID [%ls]\n", pwcBuffer,
                    pwcUtlStringsGetPrintableWideString(pstSrchParserTermPtr->pwcTerm), pwcUtlStringsGetPrintableWideString(pstSrchParserTermPtr->pwcFieldName), 
                    pwcUtlStringsGetPrintableWideString(pwcSrchParserGetStringFromID(pspSrchParser, pstSrchParserTermPtr->uiRangeID)), 
                    pwcUtlStringsGetPrintableWideString(pwcSrchParserGetStringFromID(pspSrchParser, pstSrchParserTermPtr->uiFunctionID)));
            printf("%ls     bWildCardSearch: [%s], fTermWeight [%f], bRequired: [%s]\n", pwcBuffer, (pstSrchParserTermPtr->bWildCardSearch == true) ? "true" : "false", 
                    pstSrchParserTermPtr->fTermWeight, (pstSrchParserTermPtr->bRequired == true) ? "true" : "false");
        }
        /* It is a term cluster... */
        else if ( psptcSrchParserTermCluster->puiTermTypeIDs[uiI] == SRCH_PARSER_TERM_TYPE_TERM_CLUSTER_ID ) {

            struct srchParserTermCluster    *pptcParserTermClusterPtr = (struct srchParserTermCluster *)psptcSrchParserTermCluster->ppvTerms[uiI];

            printf("%ls Cluster: uiOperatorID [%ls], iTermDistance [%d], bDistanceOrderMatters [%s]\n", pwcBuffer,
                    pwcUtlStringsGetPrintableWideString(pwcSrchParserGetStringFromID(pspSrchParser, pptcParserTermClusterPtr->uiOperatorID)), 
                    pptcParserTermClusterPtr->iTermDistance, (pptcParserTermClusterPtr->bDistanceOrderMatters == true) ? "true" : "false");

            iError = iSrchParserPrintTermCluster(pspSrchParser, (struct srchParserTermCluster *)psptcSrchParserTermCluster->ppvTerms[uiI], uiLevel + 2);
        }
    }


    return (SRCH_NoError);

}


/*---------------------------------------------------------------------------*/

