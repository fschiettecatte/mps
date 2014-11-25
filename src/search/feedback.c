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

    Module:     feedback.c

    Author:     Francois Schiettecatte (FS Consulting LLC.)

    Created:    21 January 2006

    Purpose:    This module provides feedback processing for search.c 

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
#define UTL_LOG_CONTEXT                     (unsigned char *)"com.fsconsult.mps.src.search.feedback"


/*---------------------------------------------------------------------------*/


/*
** Structures
*/

/* Search feedback sort structure */
struct srchFeedbackSort {
    unsigned char   *pucTerm;                   /* Term */
    unsigned int    uiTermCountInFeedback;      /* Number of occurrences of this term in feedback text */
    unsigned int    uiTermCount;                /* Number of occurrences of this term */
    unsigned int    uiDocumentCount;            /* Number of documents in which this term occurs */
    float           fWeight;                    /* Term weight */
};    


/*---------------------------------------------------------------------------*/


/*
** Macros
*/

/* Feedback term inclusion defaults, we extract either 10 feedback terms or 
** the top 25% from all the terms presented, whichever is more.
*/
#define SRCH_SEARCH_FEEDBACK_MINIMUM_TERM_COUNT_DEFAULT                 (10)
#define SRCH_SEARCH_FEEDBACK_MAXIMUM_TERM_PERCENTAGE_DEFAULT            (25)


/* Feedback document coverage threshold, terms have to be present in
** 8% of the documents or less to be included
*/
#define SRCH_SEARCH_FEEDBACK_MAXIMUM_TERM_COVERAGE_THRESHOLD_DEFAULT    (8)


/* Feedback type */
#define SRCH_SEARCH_FEEDBACK_TYPE_INVALID                               (0)                
#define SRCH_SEARCH_FEEDBACK_TYPE_POSITIVE                              (1)                
#define SRCH_SEARCH_FEEDBACK_TYPE_NEGATIVE                              (2)                

#define SRCH_SEARCH_FEEDBACK_TYPE_VALID(n)                              (((n) >= SRCH_SEARCH_FEEDBACK_TYPE_POSITIVE) && \
                                                                                ((n) <= SRCH_SEARCH_FEEDBACK_TYPE_NEGATIVE))


/*---------------------------------------------------------------------------*/


/*
** Private function prototypes
*/

static int iSrchFeedbackGetSearchWeightFromFeedbackText (struct srchSearch *pssSrchSearch, 
        struct srchIndex *psiSrchIndex, unsigned int uiLanguageID, 
        wchar_t *pwcFeedbackText, unsigned int uiFeedbackType, unsigned int uiStartDocumentID,
        unsigned int uiEndDocumentID, unsigned char *pucFieldIDBitmap, 
        unsigned int uiFieldIDBitmapLength, struct srchWeight **ppswSrchWeight);

static int iSrchFeedbackAddTermToTermTrie (struct srchIndex *psiSrchIndex, void *pvUtlTermTrie, void *pvLngStemmer, 
        wchar_t *pwcTerm, unsigned int *puiTotalTermCount, unsigned int *puiUniqueTermCount);

static int iSrchFeedbackCallBackFunction (unsigned char *pucKey, void *pvData, va_list ap);

static int iSrchFeedbackSearchWeightFromFeedbackSort (struct srchSearch *pssSrchSearch, struct srchIndex *psiSrchIndex,
        struct srchFeedbackSort *psfsSrchFeedbackSort, unsigned int uiSrchFeedbackSortLength, 
        unsigned int uiTotalTermCount, unsigned int uiFeedbackType, unsigned int uiStartDocumentID,
        unsigned int uiEndDocumentID, struct srchWeight **ppswSrchWeight);

static int iSrchFeedbackCompareByWeightDesc (struct srchFeedbackSort *pssfsSrchSearchFeedbackSort1, 
        struct srchFeedbackSort *pssfsSrchSearchFeedbackSort2);


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchFeedbackGetSearchWeightFromFeedbackTexts()

    Purpose:    This function collects and searches for relevance feedback terms 
                and merges the result into the passed weight array.

    Parameters: pssSrchSearch               search structure
                psiSrchIndex                index structure
                uiLanguageID                language ID
                pwcPositiveFeedbackText     positive feedback text (optional)
                pwcNegativeFeedbackText     negative feedback text (optional)
                uiStartDocumentID           start document ID restriction (0 for no restriction)
                uiEndDocumentID             end document ID restriction (0 for no restriction)
                ppswSrchWeight              return pointer for the search weight structure

    Globals:    none

    Returns:    SRCH error code

*/
int iSrchFeedbackGetSearchWeightFromFeedbackTexts
(
    struct srchSearch *pssSrchSearch,
    struct srchIndex *psiSrchIndex,
    unsigned int uiLanguageID,
    wchar_t *pwcPositiveFeedbackText,
    wchar_t *pwcNegativeFeedbackText,
    unsigned int uiStartDocumentID,
    unsigned int uiEndDocumentID,
    struct srchWeight **ppswSrchWeight
)
{

    int             iError = SRCH_NoError;
    unsigned int    uiSrchParserSearchCacheID = SRCH_PARSER_MODIFIER_UNKNOWN_ID;
    wchar_t         *pwcWeightName = NULL;
    
    unsigned char   *pucFieldIDBitmap = NULL;

    off_t           zSearchReportStartOffset = 0;
    off_t           zSearchReportEndOffset = 0;
    unsigned char   *pucSearchReportSnippet = NULL;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iSrchFeedbackGetSearchWeightFromFeedbackTexts - pwcPositiveFeedbackText: '%ls', pwcNegativeFeedbackText: '%ls'", pwcPositiveFeedbackText, pwcNegativeFeedbackText); */


    /* Check the parameters */
    if ( pssSrchSearch == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pssSrchSearch' parameter passed to 'iSrchFeedbackGetSearchWeightFromFeedbackTexts'."); 
        return (SRCH_FeedbackInvalidSearch);
    }

    if ( psiSrchIndex == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'psiSrchIndex' parameter passed to 'iSrchFeedbackGetSearchWeightFromFeedbackTexts'."); 
        return (SRCH_InvalidIndex);
    }

    if ( uiLanguageID < 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiLanguageID' parameter passed to 'iSrchFeedbackGetSearchWeightFromFeedbackTexts'."); 
        return (SRCH_FeedbackInvalidLanguageID);
    }

    if ( (bUtlStringsIsWideStringNULL(pwcPositiveFeedbackText) == true) && (bUtlStringsIsWideStringNULL(pwcNegativeFeedbackText) == true) ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pwcPositiveFeedbackText' & 'pwcNegativeFeedbackText' parameters passed to 'iSrchFeedbackGetSearchWeightFromFeedbackTexts'."); 
        return (SRCH_FeedbackInvalidFeedbackText);
    }

    if ( uiStartDocumentID < 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiStartDocumentID' parameter passed to 'iSrchFeedbackGetSearchWeightFromFeedbackTexts'."); 
        return (SRCH_FeedbackInvalidDocumentID);
    }

    if ( uiEndDocumentID < 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiEndDocumentID' parameter passed to 'iSrchFeedbackGetSearchWeightFromFeedbackTexts'."); 
        return (SRCH_FeedbackInvalidDocumentID);
    }

    if ( ppswSrchWeight == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'psiSrchIndex' parameter passed to 'iSrchFeedbackGetSearchWeightFromFeedbackTexts'."); 
        return (SRCH_ReturnParameterError);
    }



    /* Get the parser search cache ID */
    if ( (iError = iSrchParserGetModifierID(pssSrchSearch->pvSrchParser, SRCH_PARSER_MODIFIER_SEARCH_CACHE_ID, &uiSrchParserSearchCacheID)) != SRCH_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the parser search cache ID, srch error: %d.", iError);
        return (iError);
    }

    
    /* Check the cache */
    if ( uiSrchParserSearchCacheID == SRCH_PARSER_MODIFIER_SEARCH_CACHE_ENABLE_ID ) {

        unsigned int    uiWeightNameLength = 0;
        
        /* Create the weight name length */
        uiWeightNameLength += (bUtlStringsIsWideStringNULL(pwcPositiveFeedbackText) == false) ? s_wcslen(pwcPositiveFeedbackText) : 0;
        uiWeightNameLength += (bUtlStringsIsWideStringNULL(pwcNegativeFeedbackText) == false) ? s_wcslen(pwcNegativeFeedbackText) : 0;
        uiWeightNameLength += 1;

        /* Allocate space for the weight name */
        if ( (pwcWeightName = s_malloc((size_t)(uiWeightNameLength * sizeof(wchar_t)))) == NULL ) {
            return (SRCH_MemError);
        }
        
        /* Create the weight name */
        if ( bUtlStringsIsWideStringNULL(pwcPositiveFeedbackText) == false ) {
            s_wcsnncat(pwcWeightName, pwcPositiveFeedbackText, uiWeightNameLength - 1, uiWeightNameLength);
        }
        if ( bUtlStringsIsWideStringNULL(pwcNegativeFeedbackText) == false ) {
            s_wcsnncat(pwcWeightName, pwcNegativeFeedbackText, uiWeightNameLength - 1, uiWeightNameLength);
        }


        /* Get the search weight from the search cache */
        if ( (iError = iSrchCacheGetSearchWeight(pssSrchSearch->pvSrchCache, psiSrchIndex, pwcWeightName, ppswSrchWeight, 
                &pucSearchReportSnippet)) == SRCH_NoError ) {

            /* Append the search report snippet we got back to the search report */
            if ( pucSearchReportSnippet != NULL ) {
                iSrchReportAppend(pssSrchSearch->pvSrchReport, "%s", pucSearchReportSnippet);
            }
    
            /* Free the search report snippet */
            s_free(pucSearchReportSnippet);

            /* We are all set */
            goto bailFromiSrchFeedbackGetSearchWeightFromFeedbackTexts;
        }


        /* Get the search report start index, we need to extract the part of the search report that 
        ** gets generated when we run the search and store it in the cache otherwise it gets lost
        */
        if ( iSrchReportGetReportOffset(pssSrchSearch->pvSrchReport, &zSearchReportStartOffset) != SRCH_NoError ) {
            zSearchReportStartOffset = -1;
        }
    }



    /* Check if there is an unfielded search option */
    {
    
        unsigned char   pucUnfieldedSearchFieldNames[SPI_FIELD_NAME_MAXIMUM_LENGTH + 1] = {"\0"};

        /* Get the unfielded search field names and, if they exist, use them for this search */
        if ( iSrchInfoGetUnfieldedSearchFieldNames(psiSrchIndex, pucUnfieldedSearchFieldNames, SPI_FIELD_NAME_MAXIMUM_LENGTH + 1) == SRCH_NoError ) {
            
            unsigned char   *pucUnfieldedSearchFieldNamePtr = NULL;
            unsigned char   *pucUnfieldedSearchFieldNamesStrtokPtr = NULL;
            
            /* Allocate the field ID bitmap, we know there are fields other than field ID 0 since there are unfielded search field names - field ID 0 is not a field */
            if ( (pucFieldIDBitmap = (unsigned char *)s_malloc(sizeof(unsigned char) * UTL_BITMAP_GET_BITMAP_BYTE_LENGTH(psiSrchIndex->uiFieldIDMaximum))) == NULL ) {
                iError = SRCH_MemError;
                goto bailFromiSrchFeedbackGetSearchWeightFromFeedbackTexts;
            }

            /* Loop parsing the unfielded search field names */
            for ( pucUnfieldedSearchFieldNamePtr = (unsigned char *)s_strtok_r(pucUnfieldedSearchFieldNames, SRCH_INFO_UNFIELDED_SEARCH_FIELD_NAMES_SEPARATORS, (char **)&pucUnfieldedSearchFieldNamesStrtokPtr); 
                    pucUnfieldedSearchFieldNamePtr != NULL; 
                    pucUnfieldedSearchFieldNamePtr = (unsigned char *)s_strtok_r(NULL, SRCH_INFO_UNFIELDED_SEARCH_FIELD_NAMES_SEPARATORS, (char **)&pucUnfieldedSearchFieldNamesStrtokPtr) ) {
    
                unsigned int    uiFieldID = 0;
                
                /* Set the field ID in the bitmap - field ID 0 is not a field */
                if ( iSrchInfoGetFieldID (psiSrchIndex, pucUnfieldedSearchFieldNamePtr, &uiFieldID) == SRCH_NoError ) {
                    UTL_BITMAP_SET_BIT_IN_POINTER(pucFieldIDBitmap, uiFieldID - 1);
                }
            }
        }
    }



    /* Handle positive feedback */
    if ( bUtlStringsIsWideStringNULL(pwcPositiveFeedbackText) == false ) {

        /* Get the search weight from the positive feedback terms */
        if ( (iError = iSrchFeedbackGetSearchWeightFromFeedbackText(pssSrchSearch, psiSrchIndex, uiLanguageID, pwcPositiveFeedbackText, 
                SRCH_SEARCH_FEEDBACK_TYPE_POSITIVE, uiStartDocumentID, uiEndDocumentID, pucFieldIDBitmap, psiSrchIndex->uiFieldIDMaximum, 
                ppswSrchWeight)) != SRCH_NoError ) {
            goto bailFromiSrchFeedbackGetSearchWeightFromFeedbackTexts;
        }
    }
    
    
    /* Handle negative feedback */
    if ( bUtlStringsIsWideStringNULL(pwcNegativeFeedbackText) == false ) {

        /* Get the search weight from the negative feedback terms */
        if ( (iError = iSrchFeedbackGetSearchWeightFromFeedbackText(pssSrchSearch, psiSrchIndex, uiLanguageID, pwcNegativeFeedbackText, 
                SRCH_SEARCH_FEEDBACK_TYPE_NEGATIVE, uiStartDocumentID, uiEndDocumentID, pucFieldIDBitmap, psiSrchIndex->uiFieldIDMaximum, 
                ppswSrchWeight)) != SRCH_NoError ) {
            goto bailFromiSrchFeedbackGetSearchWeightFromFeedbackTexts;
        }
    }



    /* Save the search weight structure to the cache */
    if ( uiSrchParserSearchCacheID == SRCH_PARSER_MODIFIER_SEARCH_CACHE_ENABLE_ID ) {

        /* Get the search report end index */
        if ( iSrchReportGetReportOffset(pssSrchSearch->pvSrchReport, &zSearchReportEndOffset) != SRCH_NoError ) {
            zSearchReportEndOffset = -1;
        }
        
        /* Get the search report snippet, note that the search report snippet is allocated memory and needs to be released */
        if ( (zSearchReportStartOffset >= 0) && (zSearchReportEndOffset >= zSearchReportStartOffset) ) {
            if ( iSrchReportGetReportSnippet(pssSrchSearch->pvSrchReport, zSearchReportStartOffset, zSearchReportEndOffset, &pucSearchReportSnippet) != SRCH_NoError ) {
                pucSearchReportSnippet = NULL;
            }
        }

        /* Save in cache, ignore error */
        if ( *ppswSrchWeight != NULL ) {
            iSrchCacheSaveSearchWeight(pssSrchSearch->pvSrchCache, psiSrchIndex, pwcWeightName, *ppswSrchWeight, pucSearchReportSnippet);
        }
    }    



    /* Bail label */
    bailFromiSrchFeedbackGetSearchWeightFromFeedbackTexts:


    /* Free the field ID bit map */
    s_free(pucFieldIDBitmap);

    /* Free the weight name */
    s_free(pwcWeightName);

    /* Free the search report snippet */
    s_free(pucSearchReportSnippet);


    /* Handle the error */
    if ( iError != SRCH_NoError ) {

        /* Free the search weight structure */
        iSrchWeightFree(*ppswSrchWeight);
        *ppswSrchWeight = NULL;
    }


    return (iError);

}


/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchFeedbackGetSearchWeightFromFeedbackText()

    Purpose:    This function gets the search weight from the feedback text.

    Parameters: pssSrchSearch           search structure
                psiSrchIndex            index structure
                uiLanguageID            language ID
                pwcFeedbackText         feedback text
                uiFeedbackType          feedback type
                uiStartDocumentID       start document ID restriction (0 for no restriction)
                uiEndDocumentID         end document ID restriction (0 for no restriction)
                pucFieldIDBitmap        field ID bitmap (optional)
                uiFieldIDBitmapLength   field ID bitmap length (optional)
                ppswSrchWeight          return pointer for the search weight structure

    Globals:    none

    Returns:    SRCH error code

*/
static int iSrchFeedbackGetSearchWeightFromFeedbackText
(
    struct srchSearch *pssSrchSearch,
    struct srchIndex *psiSrchIndex,
    unsigned int uiLanguageID,
    wchar_t *pwcFeedbackText,
    unsigned int uiFeedbackType,
    unsigned int uiStartDocumentID,
    unsigned int uiEndDocumentID,
    unsigned char *pucFieldIDBitmap,
    unsigned int uiFieldIDBitmapLength,
    struct srchWeight **ppswSrchWeight
)
{

    int                         iError = SRCH_NoError;
    void                        *pvLngTokenizer = NULL;
    void                        *pvLngStemmer = NULL;
    void                        *pvUtlTermTrie = NULL;
    wchar_t                     *pwcTermStartPtr = NULL;
    wchar_t                     *pwcTermEndPtr = NULL;
    wchar_t                     wcTermEnd = L'\0';
    unsigned int                uiTotalTermCount = 0;
    unsigned int                uiUniqueTermCount = 0;
    struct srchFeedbackSort     *psfsSrchFeedbackSort = NULL;
    unsigned int                uiSrchFeedbackSortLength = 0;
    unsigned int                uiFeedbackSortIndex = 0;
    unsigned int                uiI = 0;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iSrchFeedbackGetSearchWeightFromFeedbackText - [%ls]", pwcFeedbackText); */


    ASSERT(pssSrchSearch != NULL);
    ASSERT(psiSrchIndex != NULL);
    ASSERT(uiLanguageID >= 0);
    ASSERT(SRCH_SEARCH_FEEDBACK_TYPE_VALID(uiFeedbackType) == true);
    ASSERT(uiStartDocumentID >= 0);
    ASSERT(uiEndDocumentID >= 0);
    ASSERT(((pucFieldIDBitmap == NULL) && (uiFieldIDBitmapLength <= 0)) || ((pucFieldIDBitmap != NULL) && (uiFieldIDBitmapLength > 0)));
    ASSERT(bUtlStringsIsWideStringNULL(pwcFeedbackText) == false);
    ASSERT(ppswSrchWeight != NULL);



    /* Bail here if there is no feedback text */
    if ( bUtlStringsIsWideStringNULL(pwcFeedbackText) == true ) {
        iError = SRCH_FeedbackInvalidFeedbackText;
        goto bailFromiSrchFeedbackGetSearchWeightFromFeedbackText;
    }


    /* Create the tokenizer  */
    if ( (iError = iLngTokenizerCreateByID(pssSrchSearch->pucConfigurationDirectoryPath, psiSrchIndex->uiTokenizerID, uiLanguageID, &pvLngTokenizer)) != LNG_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to create a tokenizer for relevance feedback, lng error: %d.", iError);
        iError = SRCH_FeedbackCreateTokenizerFailed;
        goto bailFromiSrchFeedbackGetSearchWeightFromFeedbackText;
    }

    /* Create the stemmer */
    if ( (iError = iLngStemmerCreateByID(psiSrchIndex->uiStemmerID, uiLanguageID, &pvLngStemmer)) != LNG_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to create a stemmer for relevance feedback, lng error: %d.", iError);
        iError = SRCH_FeedbackCreateStemmerFailed;
        goto bailFromiSrchFeedbackGetSearchWeightFromFeedbackText;
    }

    /* Create the trie */
    if ( (iError = iUtlTrieCreate(&pvUtlTermTrie)) != UTL_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to create a trie for relevance feedback, utl error: %d.", iError);
        iError = SRCH_FeedbackCreateTrieFailed;
        goto bailFromiSrchFeedbackGetSearchWeightFromFeedbackText;
    }


    /* Parse the normalized feedback text */
    if ( (iError = iLngTokenizerParseString(pvLngTokenizer, uiLanguageID, pwcFeedbackText, s_wcslen(pwcFeedbackText))) != LNG_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to parse the feedback text, lng error: %d.", iError);
        iError = SRCH_FeedbackTokenizationFailed;
        goto bailFromiSrchFeedbackGetSearchWeightFromFeedbackText;
    }

    /* Get the first token in the normalized feedback text */
    if ( (iError = iLngTokenizerGetToken(pvLngTokenizer, &pwcTermStartPtr, &pwcTermEndPtr)) != LNG_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to get a token from the feedback text, lng error: %d.", iError);
        iError = SRCH_FeedbackTokenizationFailed;
        goto bailFromiSrchFeedbackGetSearchWeightFromFeedbackText;
    }

    /* Parse the text, keep looping while we find tokens */
    while ( pwcTermStartPtr != NULL ) {

        wchar_t     *pwcComponentStartPtr = NULL;
        wchar_t     *pwcComponentEndPtr = NULL;
        wchar_t     wcComponentEnd = L'\0';


        /* Skip numbers */
        if ( iswdigit(*pwcTermStartPtr) != 0 ) {

            /* Get the next token in the normalized feedback text */
            if ( (iError = iLngTokenizerGetToken(pvLngTokenizer, &pwcTermStartPtr, &pwcTermEndPtr)) != LNG_NoError ) {
                iUtlLogError(UTL_LOG_CONTEXT, "Failed to get a token from the feedback text, lng error: %d.", iError);
                iError = SRCH_FeedbackTokenizationFailed;
                goto bailFromiSrchFeedbackGetSearchWeightFromFeedbackText;
            }

            continue;
        } 


        /* Save the term end character and NULL terminate the term */ 
        wcTermEnd = *pwcTermEndPtr;
        *pwcTermEndPtr = L'\0';

        /* Add the term to the feedback trie */
        if ( (iError = iSrchFeedbackAddTermToTermTrie(psiSrchIndex, pvUtlTermTrie, pvLngStemmer, pwcTermStartPtr, &uiTotalTermCount, &uiUniqueTermCount)) != SRCH_NoError ) {
            goto bailFromiSrchFeedbackGetSearchWeightFromFeedbackText;
        }

        /* Restore the term end character */
        *pwcTermEndPtr = wcTermEnd;
    

        /* Get the first component for this token */
        if ( (iError = iLngTokenizerGetComponent(pvLngTokenizer, &pwcComponentStartPtr, &pwcComponentEndPtr)) != LNG_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to get a component from the feedback text, lng error: %d.", iError);
            iError = SRCH_FeedbackTokenizationFailed;
            goto bailFromiSrchFeedbackGetSearchWeightFromFeedbackText;
        }
        
        /* Keep looping while we find components */
        while ( pwcComponentStartPtr != NULL ) {
    
            /* Save the component end character and NULL terminate the component */ 
            wcComponentEnd = *pwcComponentEndPtr;
            *pwcComponentEndPtr = L'\0';

            /* Add the component to the feedback trie */
            if ( (iError = iSrchFeedbackAddTermToTermTrie(psiSrchIndex, pvUtlTermTrie, pvLngStemmer, pwcComponentStartPtr, &uiTotalTermCount, &uiUniqueTermCount)) != SRCH_NoError ) {
                goto bailFromiSrchFeedbackGetSearchWeightFromFeedbackText;
            }
        
            /* Restore the component end character */
            *pwcComponentEndPtr = wcComponentEnd;

            /* Get the next component for this token */
            if ( (iError = iLngTokenizerGetComponent(pvLngTokenizer, &pwcComponentStartPtr, &pwcComponentEndPtr)) != LNG_NoError ) {
                iUtlLogError(UTL_LOG_CONTEXT, "Failed to get a component from the feedback text, lng error: %d.", iError);
                iError = SRCH_FeedbackTokenizationFailed;
                goto bailFromiSrchFeedbackGetSearchWeightFromFeedbackText;
            }
        }


        /* Get the next token in the normalized feedback text */
        if ( (iError = iLngTokenizerGetToken(pvLngTokenizer, &pwcTermStartPtr, &pwcTermEndPtr)) != LNG_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to get a token from the feedback text, lng error: %d.", iError);
            iError = SRCH_FeedbackTokenizationFailed;
            goto bailFromiSrchFeedbackGetSearchWeightFromFeedbackText;
        }
    }



    /* Excute the feedback if there were any terms */
    if ( uiUniqueTermCount > 0 ) {

        /* Set the size of the feedback sort array from the unique term count */
        uiSrchFeedbackSortLength = uiUniqueTermCount;

        /* Allocate an feedback sort array */
        if ( (psfsSrchFeedbackSort = (struct srchFeedbackSort *)s_malloc((size_t)(uiSrchFeedbackSortLength * sizeof(struct srchFeedbackSort)))) == NULL ) {
            iError = SRCH_MemError;
            goto bailFromiSrchFeedbackGetSearchWeightFromFeedbackText;
        }

        /* Loop through each entry in the trie calling the call back function for each entry */
        if ( (iError = iUtlTrieLoop(pvUtlTermTrie, NULL, (int (*)())iSrchFeedbackCallBackFunction, pssSrchSearch, psiSrchIndex, 
                pucFieldIDBitmap, uiFieldIDBitmapLength, psfsSrchFeedbackSort, uiSrchFeedbackSortLength, &uiFeedbackSortIndex)) != UTL_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to loop over the positive relevance feedback terms trie, utl error: %d.", iError);
            iError = SRCH_FeedbackFailed;
            goto bailFromiSrchFeedbackGetSearchWeightFromFeedbackText;
        }

        /* Loop through each entry in the term list calling the call back function for each entry */
        if ( (iError = iSrchFeedbackSearchWeightFromFeedbackSort(pssSrchSearch, psiSrchIndex, psfsSrchFeedbackSort, uiSrchFeedbackSortLength, 
                uiTotalTermCount, uiFeedbackType, uiStartDocumentID, uiEndDocumentID, ppswSrchWeight)) != SRCH_NoError ) {
            goto bailFromiSrchFeedbackGetSearchWeightFromFeedbackText;
        }

        /* Free the feedback sort array */
        for ( uiI = 0; uiI < uiSrchFeedbackSortLength; uiI++ ) {
            s_free((psfsSrchFeedbackSort + uiI)->pucTerm);
        }
        s_free(psfsSrchFeedbackSort);
    }
    


    /* Bail label */
    bailFromiSrchFeedbackGetSearchWeightFromFeedbackText:


    /* Free the tokenizer structure */
    iLngTokenizerFree(pvLngTokenizer);
    pvLngTokenizer = NULL;

    /* Free the stemmer */
    iLngStemmerFree(pvLngStemmer);
    pvLngStemmer = NULL;

    /* Free the trie */
    iUtlTrieFree(pvUtlTermTrie, false);
    pvUtlTermTrie = NULL;


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchFeedbackAddTermToTermTrie()

    Purpose:    This function adds a feedback term to the trie.

    Parameters: psiSrchIndex            index structure
                pvUtlTermTrie           term trie 
                pvLngStemmer            stemmer 
                pwcTerm                 term
                puiTotalTermCount       return pointer for the total term count    
                puiUniqueTermCount      return pointer for the unique term count    

    Globals:    none

    Returns:    SRCH error code

*/
static int iSrchFeedbackAddTermToTermTrie
(
    struct srchIndex *psiSrchIndex,
    void *pvUtlTermTrie,
    void *pvLngStemmer,
    wchar_t *pwcTerm,
    unsigned int *puiTotalTermCount,
    unsigned int *puiUniqueTermCount
)
{

    int             iError = SRCH_NoError;
    wchar_t         *pwcFeedbackTermCopy = NULL;
    unsigned int    *puiData = NULL;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iSrchFeedbackAddTermToTermTrie - [%ls]", pwcTerm); */


    ASSERT(psiSrchIndex != NULL);
    ASSERT(pvUtlTermTrie != NULL);
    ASSERT(pvLngStemmer != NULL);
    ASSERT(bUtlStringsIsWideStringNULL(pwcTerm) == false);
    ASSERT(puiTotalTermCount != NULL);
    ASSERT(puiUniqueTermCount != NULL);


    /* Make a copy of the feedback term, this is because we side-effect it when we stem it */
    if ( (pwcFeedbackTermCopy = s_wcsdup(pwcTerm)) == NULL ) {
        iError = SRCH_MemError;
        goto bailFromiSrchFeedbackAddTermToTermTrie;
    }


    /* Add the term in its original case state if it all in upper case */
    if ( bLngCaseIsWideStringAllUpperCase(pwcFeedbackTermCopy) == true ) {

        unsigned char   *pucTerm = NULL;

        /* Convert the term from wide characters to utf-8, pucTerm is allocated */
        if ( (iError = iLngConvertWideStringToUtf8_d(pwcFeedbackTermCopy, 0, &pucTerm)) != LNG_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to convert a feedback term from wide characters to utf-8, lng error: %d.", iError);
            iError = SRCH_FeedbackCharacterSetConvertionFailed;
            goto bailFromiSrchFeedbackAddTermToTermTrie;
        }

        /* Truncate the term if it is too long */
        if ( s_strlen(pucTerm) > psiSrchIndex->uiTermLengthMaximum ) {
            pucTerm[psiSrchIndex->uiTermLengthMaximum] = '\0';
        }

        /* Look up/store the term - skip it if it cant be looked-up/stored */
        if ( (iError = iUtlTrieAdd(pvUtlTermTrie, pucTerm, (void ***)&puiData)) != UTL_NoError ) {
            iUtlLogWarn(UTL_LOG_CONTEXT, "Failed to add an entry to the relevance feedback terms trie, term: '%s', utl error: %d.", pucTerm, iError);
            s_free(pucTerm);
            iError = SRCH_NoError;
            goto bailFromiSrchFeedbackAddTermToTermTrie;
        }

        /* Increment our counts */
        (*puiTotalTermCount)++;
        (*puiData) += 1;
        (*puiUniqueTermCount) += (*puiData == 1) ? 1 : 0;
        
        /* Free the term */
        s_free(pucTerm);
        pucTerm = NULL;
    }

    /* Convert the term to lower case and add it */
    {

        unsigned char   *pucTerm = NULL;

        /* Convert the term to lower case */
        pwcLngCaseConvertWideStringToLowerCase(pwcFeedbackTermCopy);

        /* Stem the term */
        if ( (iError = iLngStemmerStemTerm(pvLngStemmer, pwcFeedbackTermCopy, 0)) != LNG_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to stem a term, lng error: %d.", iError);
            iError = SRCH_FeedbackStemmingFailed;
            goto bailFromiSrchFeedbackAddTermToTermTrie;
        }

        /* Skip the term if it was stemmed out of existence */
        if ( bUtlStringsIsWideStringNULL(pwcFeedbackTermCopy) == true ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to convert a feedback term to utf-8, lng error: %d.", iError);
            iError = SRCH_NoError;
            goto bailFromiSrchFeedbackAddTermToTermTrie;
        }
        
        /* Convert the term from wide characters to utf-8, pucTerm is allocated */
        if ( (iError = iLngConvertWideStringToUtf8_d(pwcFeedbackTermCopy, 0, &pucTerm)) != LNG_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to convert a feedback term from wide characters to utf-8, lng error: %d.", iError);
            iError = SRCH_FeedbackCharacterSetConvertionFailed;
            goto bailFromiSrchFeedbackAddTermToTermTrie;
        }

        /* Look up/store the term - skip it if it cant be looked-up/stored */
        if ( (iError = iUtlTrieAdd(pvUtlTermTrie, pucTerm, (void ***)&puiData)) != UTL_NoError ) {
            iUtlLogWarn(UTL_LOG_CONTEXT, "Failed to add an entry to the relevance feedback terms trie, term: '%s', utl error: %d.", pucTerm, iError);
            s_free(pucTerm);
            iError = SRCH_NoError;
            goto bailFromiSrchFeedbackAddTermToTermTrie;
        }

        /* Increment our counts */
        (*puiTotalTermCount)++;
        (*puiData) += 1;
        (*puiUniqueTermCount) += (*puiData == 1) ? 1 : 0;

        /* Free the term */
        s_free(pucTerm);
    }



    /* Bail label */
    bailFromiSrchFeedbackAddTermToTermTrie:


    /* Free the feedback term copy */
    s_free(pwcFeedbackTermCopy);


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchFeedbackCallBackFunction()

    Purpose:    This function is passed as the call back function to iUtlTrieLoop()
                and gets called for every key in the trie. It will get passed
                pointers to the key currently being processed and to the data
                stored for that key. It will also get passed as a va_list, the 
                parameters that were specified in the call to iUtlTrieLoop().

    Parameters: pucKey      key (term)
                pvData      data        
                ap          args (optional)

    Globals:    none

    Returns:    0 if successful, -1 on error

*/
static int iSrchFeedbackCallBackFunction
(
    unsigned char *pucKey,
    void *pvData,
    va_list ap
)
{

    va_list                     ap_;
    int                         iError = SRCH_NoError;
    struct srchSearch           *pssSrchSearch = NULL;
    struct srchIndex            *psiSrchIndex = NULL;
    unsigned char               *pucFieldIDBitmap = NULL;
    unsigned int                uiFieldIDBitmapLength = 0;
    struct srchFeedbackSort     *psfsSrchFeedbackSort = NULL;
    unsigned int                uiSrchFeedbackSortLength = 0;
    struct srchFeedbackSort     *psfsSrchFeedbackSortPtr = NULL;
    unsigned int                *puiSrchSearchFeedbackSortIndex = NULL;
    unsigned int                uiTermType = 0;
    unsigned int                uiTermCount = 0;
    unsigned int                uiDocumentCount = 0;
    unsigned long               ulIndexBlockID = 0;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iSrchFeedbackCallBackFunction - [%s][%u]", pucKey, (unsigned int)pvData); */


    ASSERT(bUtlStringsIsStringNULL(pucKey) == false);
    ASSERT(pvData != NULL);


    /* Get all our parameters, note that we make a copy of 'ap' */
    va_copy(ap_, ap);
    pssSrchSearch = (struct srchSearch *)va_arg(ap_, struct srchSearch *);
    psiSrchIndex = (struct srchIndex *)va_arg(ap_, struct srchIndex *);
    pucFieldIDBitmap = (unsigned char *)va_arg(ap, unsigned char *);
    uiFieldIDBitmapLength = (unsigned int)va_arg(ap, unsigned int);
    psfsSrchFeedbackSort = (struct srchFeedbackSort *)va_arg(ap_, struct srchFeedbackSort *);
    uiSrchFeedbackSortLength = (unsigned int)va_arg(ap_, unsigned int);
    puiSrchSearchFeedbackSortIndex = (unsigned int *)va_arg(ap_, unsigned int *);
    va_end(ap_);


    ASSERT(pssSrchSearch != NULL);
    ASSERT(psiSrchIndex != NULL);
    ASSERT(((pucFieldIDBitmap == NULL) && (uiFieldIDBitmapLength <= 0)) || ((pucFieldIDBitmap != NULL) && (uiFieldIDBitmapLength > 0)));
    ASSERT(psfsSrchFeedbackSort != NULL);
    ASSERT(uiSrchFeedbackSortLength > 0);
    ASSERT(puiSrchSearchFeedbackSortIndex != NULL);
    ASSERT(*puiSrchSearchFeedbackSortIndex < uiSrchFeedbackSortLength);


    /* Dereference the search feedback sort structure */
    psfsSrchFeedbackSortPtr = psfsSrchFeedbackSort + (*puiSrchSearchFeedbackSortIndex);


    /* Make a copy of the key and the data, the data contains the term count in the feedback text */
    if ( (psfsSrchFeedbackSortPtr->pucTerm = (unsigned char *)s_strdup(pucKey)) == NULL ) {
        return (-1);
    }

    /* This cast complains on 64 bit architectures */
    psfsSrchFeedbackSortPtr->uiTermCountInFeedback = (unsigned int)pvData;


    /* Preset the term count, the document count, and the weight */
    psfsSrchFeedbackSortPtr->uiTermCount = 0;
    psfsSrchFeedbackSortPtr->uiDocumentCount = 0;
    psfsSrchFeedbackSortPtr->fWeight = 0;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "pucTerm - [%s]", psfsSrchFeedbackSortPtr->pucTerm); */

    /* Look up the term */
    if ( (iError = iSrchTermDictLookup(psiSrchIndex, psfsSrchFeedbackSortPtr->pucTerm, pucFieldIDBitmap, uiFieldIDBitmapLength, 
            &uiTermType, &uiTermCount, &uiDocumentCount, &ulIndexBlockID)) == SRCH_NoError ) {

        /* If this term is a regular term, we work out its IDF weight */
        if ( uiTermType == SPI_TERM_TYPE_REGULAR ) {

            /* Set the term count and the document count */
            psfsSrchFeedbackSortPtr->uiTermCount = uiTermCount;
            psfsSrchFeedbackSortPtr->uiDocumentCount = uiDocumentCount;

            /* This works out the IDF of the term */
            psfsSrchFeedbackSortPtr->fWeight += SRCH_SEARCH_IDF_FACTOR(uiTermCount, uiDocumentCount, psiSrchIndex->uiDocumentCount) * psfsSrchFeedbackSortPtr->uiTermCountInFeedback;
        }
    }


    /* Increment the indent */
    (*puiSrchSearchFeedbackSortIndex)++;


    return (0);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchFeedbackSearchWeightFromFeedbackSort()

    Purpose:    This function sorts the search feedback sort structure array 
                into descending weight order and searches for the top N terms.

    Parameters: pssSrchSearch               search structure
                psiSrchIndex                index structure
                psfsSrchFeedbackSort        search feedback sort structure array
                uiSrchFeedbackSortLength    search feedback sort structure array length
                uiTotalTermCount            total number of feedback terms
                uiFeedbackType              feedback type
                uiStartDocumentID           start document ID restriction (0 for no restriction)
                uiEndDocumentID             end document ID restriction (0 for no restriction)
                ppswSrchWeight              return pointer for the search weight structure

    Globals:    none

    Returns:    SRCH error code

*/
static int iSrchFeedbackSearchWeightFromFeedbackSort
(
    struct srchSearch *pssSrchSearch,
    struct srchIndex *psiSrchIndex,
    struct srchFeedbackSort *psfsSrchFeedbackSort,
    unsigned int uiSrchFeedbackSortLength,
    unsigned int uiTotalTermCount,
    unsigned int uiFeedbackType,
    unsigned int uiStartDocumentID,
    unsigned int uiEndDocumentID,
    struct srchWeight **ppswSrchWeight
)
{

    int                         iError = SRCH_NoError;
    unsigned int                uiSrchParserSearchTermCount = 0;
    unsigned int                uiI = 0;
    struct srchFeedbackSort     *psfsSrchFeedbackSortPtr = NULL;
    unsigned int                uiUsedTermCount = 0;
    unsigned int                uiMaxTermCount = 0;
    boolean                     bSearchWeightAllocatedHere = false;
    unsigned char               pucConfigValue[SRCH_INFO_SYMBOL_MAXIMUM_LENGTH + 1] = {'\0'};
    float                       fTermWeightDefault = SRCH_SEARCH_TERM_WEIGHT_DEFAULT;
    float                       fFeedbackTermWeightDefault = SRCH_SEARCH_FEEDBACK_TERM_WEIGHT_DEFAULT;
    float                       fFeedbackTermWeight = SRCH_SEARCH_FEEDBACK_TERM_WEIGHT_DEFAULT;
    float                       fFrequentTermCoverageThreshold = 0.0;
    unsigned int                uiFeedbackMinimumTermCount = 0;
    float                       fFeedbackMaximumTermPercentage = SRCH_SEARCH_FEEDBACK_MAXIMUM_TERM_PERCENTAGE_DEFAULT;
    float                       fFeedbackMaximumTermCoverageThreshold = SRCH_SEARCH_FEEDBACK_MAXIMUM_TERM_COVERAGE_THRESHOLD_DEFAULT;
    

/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iSrchFeedbackSearchWeightFromFeedbackSort"); */


    ASSERT(pssSrchSearch != NULL);
    ASSERT(psiSrchIndex != NULL);
    ASSERT(psfsSrchFeedbackSort != NULL);
    ASSERT(uiSrchFeedbackSortLength > 0);
    ASSERT(uiTotalTermCount > 0);
    ASSERT(SRCH_SEARCH_FEEDBACK_TYPE_VALID(uiFeedbackType) == true);
    ASSERT(uiStartDocumentID >= 0);
    ASSERT(uiEndDocumentID >= 0);
    ASSERT(ppswSrchWeight != NULL);



    /* Get the search parser term count */
    if ( (iError = iSrchParserGetSearchTermCount(pssSrchSearch->pvSrchParser, &uiSrchParserSearchTermCount)) != SRCH_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the search parser search term count, srch error: %d.", iError);
        return (iError);
    }
    

    /* Get the term weight specified in the search string */
    if ( (iError = iSrchParserGetModifierValue(pssSrchSearch->pvSrchParser, SRCH_PARSER_MODIFIER_TERM_WEIGHT_ID, &fTermWeightDefault)) != SRCH_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the parser term weight, srch error: %d.", iError);
        return (iError);
    }

    /* Get the feedback term weight specified in the search string */
    if ( (iError = iSrchParserGetModifierValue(pssSrchSearch->pvSrchParser, SRCH_PARSER_MODIFIER_FEEDBACK_TERM_WEIGHT_ID, &fFeedbackTermWeightDefault)) != SRCH_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the parser feedback term weight, srch error: %d.", iError);
        return (iError);
    }


    /* Set the term weight from the config file if it was not specified in the search string */
    if ( fTermWeightDefault == 0 ) {

        /* Set the default term weight */
        fTermWeightDefault = SRCH_SEARCH_TERM_WEIGHT_DEFAULT;
        
        /* Get the term weight from the config file */
        if ( iUtlConfigGetValue(pssSrchSearch->pvUtlConfig, SRCH_SEARCH_CONFIG_SEARCH_TERM_WEIGHT, pucConfigValue, SRCH_INFO_SYMBOL_MAXIMUM_LENGTH + 1) == UTL_NoError ) {
            if ( s_strtof(pucConfigValue, NULL) <= 0 ) {
                iUtlLogWarn(UTL_LOG_CONTEXT, "Invalid search term weight value found in the server configuration file, symbol name: '%s', symbol: '%s', defaulting to: %.2f.", 
                        SRCH_SEARCH_CONFIG_SEARCH_TERM_WEIGHT, pucConfigValue, SRCH_SEARCH_TERM_WEIGHT_DEFAULT);
            }
            else {                    
                fTermWeightDefault = s_strtof(pucConfigValue, NULL);
            }
        }
    }
/*     iUtlLogDebug(UTL_LOG_CONTEXT, "fTermWeightDefault: %.2f", fTermWeightDefault); */


    /* Set the feedback term weight from the config file if it was not specified in the search string */
    if ( fFeedbackTermWeightDefault == 0 ) {

        /* Set the default feedback term weight */
        fFeedbackTermWeightDefault = SRCH_SEARCH_FEEDBACK_TERM_WEIGHT_DEFAULT;

        /* Get the feedback term weight */
        if ( iUtlConfigGetValue(pssSrchSearch->pvUtlConfig, SRCH_SEARCH_CONFIG_SEARCH_FEEDBACK_TERM_WEIGHT, pucConfigValue, SRCH_INFO_SYMBOL_MAXIMUM_LENGTH + 1) == UTL_NoError ) {
            if ( s_strtof(pucConfigValue, NULL) <= 0 ) {
                iUtlLogWarn(UTL_LOG_CONTEXT, "Invalid search feedback term weight value found in the server configuration file, symbol name: '%s', symbol: '%s' defaulting to: %.2f.", 
                        SRCH_SEARCH_CONFIG_SEARCH_FEEDBACK_TERM_WEIGHT, pucConfigValue, SRCH_SEARCH_FEEDBACK_TERM_WEIGHT_DEFAULT);
            }
            else {                    
                fFeedbackTermWeightDefault = s_strtof(pucConfigValue, NULL);
            }
        }
    }
/*     iUtlLogDebug(UTL_LOG_CONTEXT, "fFeedbackTermWeightDefault: %.2f", fFeedbackTermWeightDefault); */



    /* Get the frequent term coverage threshold specified in the search string */
    if ( (iError = iSrchParserGetModifierValue(pssSrchSearch->pvSrchParser, SRCH_PARSER_MODIFIER_FREQUENT_TERM_COVERAGE_THRESHOLD_ID, &fFrequentTermCoverageThreshold)) != SRCH_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the parser frequent term info, srch error: %d.", iError);
        return (iError);
    }
    
    /* Check the config file if the frequent term coverage threshold was not specified in the search string */
    if ( fFrequentTermCoverageThreshold == 0 ) {
    
        /* Get the search frequent term coverage threshold */
        if ( iUtlConfigGetValue(pssSrchSearch->pvUtlConfig, SRCH_SEARCH_CONFIG_SEARCH_FREQUENT_TERM_COVERAGE_THRESHOLD, pucConfigValue, SRCH_INFO_SYMBOL_MAXIMUM_LENGTH + 1) == UTL_NoError ) {
            if ( s_strtof(pucConfigValue, NULL) <= 0 ) {
                iUtlLogWarn(UTL_LOG_CONTEXT, "Invalid search frequent term coverage threshold value found in the server configuration file, symbol name: '%s', symbol: '%s'.", 
                        SRCH_SEARCH_CONFIG_SEARCH_FREQUENT_TERM_COVERAGE_THRESHOLD, pucConfigValue);
            }
            else {                    
                fFrequentTermCoverageThreshold = s_strtof(pucConfigValue, NULL);
            }
        }
    }
    
    /* The frequent term coverage threshold specified, so we need to check if we can apply it */
    if ( fFrequentTermCoverageThreshold > 0 ) {

        unsigned int    uiSrchParserFrequentTermsID = 0;

        /* Get the parser frequent terms ID */
        if ( (iError = iSrchParserGetModifierID(pssSrchSearch->pvSrchParser, SRCH_PARSER_MODIFIER_FREQUENT_TERMS_ID, &uiSrchParserFrequentTermsID)) != SRCH_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the parser frequent terms ID, srch error: %d.", iError);
            return (iError);
        }
        
        /* Zero out the frequent term coverage threshold if there is only one term in the search or have to keep terms */
        if ( (uiSrchParserSearchTermCount == 1) || (uiSrchParserFrequentTermsID == SRCH_PARSER_MODIFIER_FREQUENT_TERMS_KEEP_ID) ) {
            fFrequentTermCoverageThreshold = 0;
        }
    }
/*     iUtlLogDebug(UTL_LOG_CONTEXT, "fFrequentTermCoverageThreshold: %.2f", fFrequentTermCoverageThreshold); */



    /* Get the feedback minimum term count information */
    if ( (iError = iSrchParserGetModifierValue(pssSrchSearch->pvSrchParser, SRCH_PARSER_MODIFIER_FEEDBACK_MINIMUM_TERM_COUNT_ID, &uiFeedbackMinimumTermCount)) != SRCH_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the parser feedback minimum term count, srch error: %d.", iError);
        return (iError);
    }

    /* Set the feedback minimum term count from the config file if it was not specified in the search string */
    if ( uiFeedbackMinimumTermCount <= 0 ) {

        /* Set the default feedback minimum term count */
        uiFeedbackMinimumTermCount = SRCH_SEARCH_FEEDBACK_MINIMUM_TERM_COUNT_DEFAULT;

        /* Get the feedback minimum term count from the config file */
        if ( iUtlConfigGetValue(pssSrchSearch->pvUtlConfig, SRCH_SEARCH_CONFIG_SEARCH_FEEDBACK_MINIMUM_TERM_COUNT, pucConfigValue, SRCH_INFO_SYMBOL_MAXIMUM_LENGTH + 1) == UTL_NoError ) {
            if ( s_strtol(pucConfigValue, NULL, 10) <= 0 ) {
                iUtlLogWarn(UTL_LOG_CONTEXT, "Invalid search search feedback minimum term count value found in the server configuration file, symbol name: '%s', symbol: '%s', defaulting to: %d.", 
                        SRCH_SEARCH_CONFIG_SEARCH_FEEDBACK_MINIMUM_TERM_COUNT, pucConfigValue, SRCH_SEARCH_FEEDBACK_MINIMUM_TERM_COUNT_DEFAULT);
            }
            else {                    
                uiFeedbackMinimumTermCount = s_strtol(pucConfigValue, NULL, 10);
            }
        }
    }
/*     iUtlLogDebug(UTL_LOG_CONTEXT, "uiFeedbackMinimumTermCount: %u", uiFeedbackMinimumTermCount); */


    /* Get the feedback maximum term percentage information */
    if ( (iError = iSrchParserGetModifierValue(pssSrchSearch->pvSrchParser, SRCH_PARSER_MODIFIER_FEEDBACK_MAXIMUM_TERM_PERCENTAGE_ID, &fFeedbackMaximumTermPercentage)) != SRCH_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the parser feedback maximum term percentage, srch error: %d.", iError);
        return (iError);
    }
    
    /* Set the feedback maximum term percentage from the config file if it was not specified in the search string */
    if ( fFeedbackMaximumTermPercentage <= 0 ) {

        /* Set the default feedback maximum term percentage */
        fFeedbackMaximumTermPercentage = SRCH_SEARCH_FEEDBACK_MAXIMUM_TERM_PERCENTAGE_DEFAULT;

        /* Get the feedback maximum term percentage */
        if ( iUtlConfigGetValue(pssSrchSearch->pvUtlConfig, SRCH_SEARCH_CONFIG_SEARCH_FEEDBACK_MAXIMUM_TERM_PERCENTAGE, pucConfigValue, SRCH_INFO_SYMBOL_MAXIMUM_LENGTH + 1) == UTL_NoError ) {
            if ( s_strtof(pucConfigValue, NULL) <= 0 ) {
                iUtlLogWarn(UTL_LOG_CONTEXT, "Invalid search feedback maximum term percentage value found in the server configuration file, symbol name: '%s', symbol: '%s', defaulting to: %.2f.", 
                        SRCH_SEARCH_CONFIG_SEARCH_FEEDBACK_MAXIMUM_TERM_PERCENTAGE, pucConfigValue, SRCH_SEARCH_FEEDBACK_MAXIMUM_TERM_PERCENTAGE_DEFAULT);
            }
            else {                    
                fFeedbackMaximumTermPercentage = s_strtof(pucConfigValue, NULL);
            }
        }
    }
/*     iUtlLogDebug(UTL_LOG_CONTEXT, "fFeedbackMaximumTermPercentage: %.2f", fFeedbackMaximumTermPercentage); */


    /* Get the feedback maximum term coverage threshold information */
    if ( (iError = iSrchParserGetModifierValue(pssSrchSearch->pvSrchParser, SRCH_PARSER_MODIFIER_FEEDBACK_MAXIMUM_TERM_COVERAGE_THRESHOLD_ID, &fFeedbackMaximumTermCoverageThreshold)) != SRCH_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the parser feedback maximum term coverage threshold, srch error: %d.", iError);
        return (iError);
    }
    
    /* Set the feedback maximum term coverage threshold from the config file if it was not specified in the search string */
    if ( fFeedbackMaximumTermCoverageThreshold <= 0 ) {

        /* Set the default feedback maximum term coverage threshold */
        fFeedbackMaximumTermCoverageThreshold = SRCH_SEARCH_FEEDBACK_MAXIMUM_TERM_COVERAGE_THRESHOLD_DEFAULT;

        /* Get the feedback maximum term coverage threshold */
        if ( iUtlConfigGetValue(pssSrchSearch->pvUtlConfig, SRCH_SEARCH_CONFIG_SEARCH_FEEDBACK_MAXIMUM_TERM_COVERAGE_THRESHOLD, pucConfigValue, SRCH_INFO_SYMBOL_MAXIMUM_LENGTH + 1) == UTL_NoError ) {
            if ( s_strtof(pucConfigValue, NULL) <= 0 ) {
                iUtlLogWarn(UTL_LOG_CONTEXT, "Invalid search feedback maximum term coverage threshold value found in the server configuration file, symbol name: '%s', symbol: '%s', defaulting to: %.2f.", 
                        SRCH_SEARCH_CONFIG_SEARCH_FEEDBACK_MAXIMUM_TERM_COVERAGE_THRESHOLD, pucConfigValue, SRCH_SEARCH_FEEDBACK_MAXIMUM_TERM_COVERAGE_THRESHOLD_DEFAULT);
            }
            else {                    
                fFeedbackMaximumTermCoverageThreshold = s_strtof(pucConfigValue, NULL);
            }
        }
    }
/*     iUtlLogDebug(UTL_LOG_CONTEXT, "fFeedbackMaximumTermCoverageThreshold: %.2f", fFeedbackMaximumTermCoverageThreshold); */


    /* Sort the relevance feedback terms in descending weight */
    s_qsort(psfsSrchFeedbackSort, uiSrchFeedbackSortLength, sizeof(struct srchFeedbackSort), (int (*)(const void *, const void *))iSrchFeedbackCompareByWeightDesc);


/*     printf("Term                   TermCount    DocCount      Weight     Coverage\n"); */
/*     for ( uiI = 0, psfsSrchFeedbackSortPtr = psfsSrchFeedbackSort; uiI < uiSrchFeedbackSortLength; uiI++, psfsSrchFeedbackSortPtr++ ) { */
/*         printf("%-20s  %10u  %10u     %7.3f     %7.6f %s\n", psfsSrchFeedbackSortPtr->pucTerm, psfsSrchFeedbackSortPtr->uiTermCount,  */
/*                 psfsSrchFeedbackSortPtr->uiDocumentCount, psfsSrchFeedbackSortPtr->fWeight, ((float)psfsSrchFeedbackSortPtr->uiDocumentCount / psiSrchIndex->uiDocumentCount) * 100, */
/*                 (((float)psfsSrchFeedbackSortPtr->uiDocumentCount / psiSrchIndex->uiDocumentCount) * 100) >= fFeedbackMaximumTermCoverageThreshold ? "<<" : ""); */
/*     } */


    /* Calculate the suggested maximum number of terms to use for relevance feedback */
    uiMaxTermCount = UTL_MACROS_MAX(((float)uiSrchFeedbackSortLength * (fFeedbackMaximumTermPercentage / 100)), uiFeedbackMinimumTermCount);


    /* First we count up the number of terms we plan to use, we need that information to work out the term weight */
    for ( uiI = 0, uiUsedTermCount = 0, psfsSrchFeedbackSortPtr = psfsSrchFeedbackSort; (uiI < uiSrchFeedbackSortLength) && (uiUsedTermCount < uiMaxTermCount); 
            uiI++, psfsSrchFeedbackSortPtr++ ) {

        /* Only apply terms which:
        **    - occurs more than once in the index
        **    - occurs in more than one document
        **    - occurs in #% or less of the documents (fFeedbackMaximumTermCoverageThreshold)
        */
/*         if ( (psfsSrchFeedbackSortPtr->uiTermCount > 1) && (psfsSrchFeedbackSortPtr->uiDocumentCount > 1) && */
/*                 (((float)psfsSrchFeedbackSortPtr->uiDocumentCount / psiSrchIndex->uiDocumentCount) <= (fFeedbackMaximumTermCoverageThreshold / 100)) ) { */
        if ( ((float)psfsSrchFeedbackSortPtr->uiDocumentCount / psiSrchIndex->uiDocumentCount) <= (fFeedbackMaximumTermCoverageThreshold / 100) ) {

            /* Increment the number of terms we are going to apply */
            uiUsedTermCount++;
        }
    }
    
    
    /* Return now if we are not going to use any terms */
    if ( uiUsedTermCount == 0 ) {
        iSrchReportAppend(pssSrchSearch->pvSrchReport, "%s Feedback was submitted but was not used because it was not useful\n", REP_SEARCH_WARNING);
        return (SRCH_NoError);
    }


    /* Set the term weight for feedback */
    if ( (uiSrchParserSearchTermCount > 0) && (uiUsedTermCount > 0) ) {

        /* Calculate the feedback term weight */
        fFeedbackTermWeight = (float)uiSrchParserSearchTermCount / (log(uiUsedTermCount) + 1);

        /* Make sure that we don't weigh more than the default term weight */ 
        fFeedbackTermWeight = UTL_MACROS_MIN(fFeedbackTermWeight, fTermWeightDefault);

        /* Make sure that we don't weigh less than the default feedback weight */ 
        fFeedbackTermWeight = UTL_MACROS_MAX(fFeedbackTermWeight, fFeedbackTermWeightDefault);
    }
    else {
        fFeedbackTermWeight = fFeedbackTermWeightDefault;
    }
/*     iUtlLogDebug(UTL_LOG_CONTEXT, "Intermediate - fFeedbackTermWeight: %.2f", fFeedbackTermWeight); */


    /* Set the term weight for feedback based on the feedback type, fail if the feedback type as incorrect */
    if ( uiFeedbackType == SRCH_SEARCH_FEEDBACK_TYPE_POSITIVE ) {
        fFeedbackTermWeight = fFeedbackTermWeight;
    }
    else if ( uiFeedbackType == SRCH_SEARCH_FEEDBACK_TYPE_NEGATIVE ) {
        fFeedbackTermWeight = -fFeedbackTermWeight;
    }
    else {
        return (SRCH_MiscError);
    }
/*     iUtlLogDebug(UTL_LOG_CONTEXT, "Final - fFeedbackTermWeight: %.2f", fFeedbackTermWeight); */

/*     iUtlLogDebug(UTL_LOG_CONTEXT, "uiMaxTermCount: %u, uiUsedTermCount: %u, uiSrchParserSearchTermCount: %u, fTermWeight: %.2f.", uiMaxTermCount, uiUsedTermCount, uiSrchParserSearchTermCount, fTermWeight); */


    /* Set the flag telling this function that the weights array was allocated here
    ** as opposed to above us, so it is our responsibility to release it if something 
    ** goes wrong
    */
    if ( *ppswSrchWeight == NULL ) {
        bSearchWeightAllocatedHere = true;
    }


    /* Search report */
    if ( uiFeedbackType == SRCH_SEARCH_FEEDBACK_TYPE_POSITIVE ) {
        iSrchReportAppend(pssSrchSearch->pvSrchReport, "%s", REP_POSITIVE_FEEDBACK_TERMS);
    }
    else if ( uiFeedbackType == SRCH_SEARCH_FEEDBACK_TYPE_NEGATIVE ) { 
        iSrchReportAppend(pssSrchSearch->pvSrchReport, "%s", REP_NEGATIVE_FEEDBACK_TERMS);
    }


/*     printf("Term                   TermCount    DocCount      Weight     Coverage\n"); */

    /* Apply the terms to the search */
    for ( uiI = 0, uiUsedTermCount = 0, psfsSrchFeedbackSortPtr = psfsSrchFeedbackSort; (uiI < uiSrchFeedbackSortLength) && (uiUsedTermCount < uiMaxTermCount); 
            uiI++, psfsSrchFeedbackSortPtr++ ) {

        /* Only apply terms which:
        **  - occurs more than once in the index
        **  - occurs in more than one document
        **  - occurs in #% of the documents or less (fFeedbackMaximumTermCoverageThreshold)
        */
/*         if ( (psfsSrchFeedbackSortPtr->uiTermCount > 1) && (psfsSrchFeedbackSortPtr->uiDocumentCount > 1) && */
/*                 (((float)psfsSrchFeedbackSortPtr->uiDocumentCount / psiSrchIndex->uiDocumentCount) <= (fFeedbackMaximumTermCoverageThreshold / 100)) ) { */
        if ( ((float)psfsSrchFeedbackSortPtr->uiDocumentCount / psiSrchIndex->uiDocumentCount) <= (fFeedbackMaximumTermCoverageThreshold / 100) ) {

            /* Search report */
            iSrchReportAppend(pssSrchSearch->pvSrchReport, " %s", psfsSrchFeedbackSortPtr->pucTerm);
            
/*             printf("%-20s  %10u  %10u     %7.3f     %7.6f %s\n", psfsSrchFeedbackSortPtr->pucTerm, psfsSrchFeedbackSortPtr->uiTermCount,  */
/*                     psfsSrchFeedbackSortPtr->uiDocumentCount, psfsSrchFeedbackSortPtr->fWeight, ((float)psfsSrchFeedbackSortPtr->uiDocumentCount / psiSrchIndex->uiDocumentCount) * 100, */
/*                     (((float)psfsSrchFeedbackSortPtr->uiDocumentCount / psiSrchIndex->uiDocumentCount) * 100) >= fFeedbackMaximumTermCoverageThreshold ? "<<" : ""); */

            /* Weight the term, it has field id 0 */
            iError = iSrchTermSearchGetSearchWeightsFromTerm(pssSrchSearch, psiSrchIndex, psfsSrchFeedbackSortPtr->pucTerm, fFeedbackTermWeight, 
                    NULL, 0, fFrequentTermCoverageThreshold, uiStartDocumentID, uiEndDocumentID, ppswSrchWeight);

            /* Check the returned error */
            if ( (iError != SRCH_NoError) && (iError != SRCH_TermDictTermNotFound) && (iError != SRCH_TermDictTermDoesNotOccur) ) {
                
                /* Free the weights array if we allocated it here */
                if ( bSearchWeightAllocatedHere == true ) {
                    iSrchWeightFree(*ppswSrchWeight);
                    *ppswSrchWeight = NULL;
                }
                
                return (iError);
            }
            
            /* Increment the number of terms we have applied */
            uiUsedTermCount++;
        }
    }


    /* Search report */
    if ( uiFeedbackType == SRCH_SEARCH_FEEDBACK_TYPE_POSITIVE ) {
        iSrchReportAppend(pssSrchSearch->pvSrchReport, "\n%s %u %u %u\n", REP_POSITIVE_FEEDBACK_COUNTS, uiTotalTermCount, uiSrchFeedbackSortLength, uiUsedTermCount);
    }
    else if ( uiFeedbackType == SRCH_SEARCH_FEEDBACK_TYPE_NEGATIVE ) {
        iSrchReportAppend(pssSrchSearch->pvSrchReport, "\n%s %u %u %u\n", REP_NEGATIVE_FEEDBACK_COUNTS, uiTotalTermCount, uiSrchFeedbackSortLength, uiUsedTermCount);
    }

    
    /* Log */
    iUtlLogInfo(UTL_LOG_CONTEXT, "Relevance feedback terms, total: %u, unique: %u, used: %u.", uiTotalTermCount, uiSrchFeedbackSortLength, uiUsedTermCount);


    return (SRCH_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchFeedbackCompareByWeightDesc()

    Purpose:    This functions takes a two search feedback sort structures and compares their weight.
                This function is used by the qsort call in iSrchFeedbackSearchWeightFromFeedbackSort().

    Parameters: pssfsSrchSearchFeedbackSort1    pointer to an feedback sort structure
                pssfsSrchSearchFeedbackSort2    pointer to an feedback sort structure

    Globals:    none

    Returns:    1 if pssfsSrchSearchFeedbackSort2 > pssfsSrchSearchFeedbackSort1, 
                -1 if pssfsSrchSearchFeedbackSort1 > pssfsSrchSearchFeedbackSort2, 
                and 0 if pssfsSrchSearchFeedbackSort1 == pssfsSrchSearchFeedbackSort2

*/
static int iSrchFeedbackCompareByWeightDesc
(
    struct srchFeedbackSort *pssfsSrchSearchFeedbackSort1,
    struct srchFeedbackSort *pssfsSrchSearchFeedbackSort2
)
{

    ASSERT(pssfsSrchSearchFeedbackSort1 != NULL);
    ASSERT(pssfsSrchSearchFeedbackSort2 != NULL);


    /* It is most likely that two entries will be the same,
    ** so process that first 
    */
    if ( pssfsSrchSearchFeedbackSort1->fWeight == pssfsSrchSearchFeedbackSort2->fWeight ) {
        return (0);
    }
    else if ( pssfsSrchSearchFeedbackSort1->fWeight < pssfsSrchSearchFeedbackSort2->fWeight ) {
        return (1);
    }
    else { 
        return (-1);
    }

}


/*---------------------------------------------------------------------------*/
