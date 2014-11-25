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

    Module:     filter.c

    Author:     Francois Schiettecatte (FS Consulting LLC.)

    Created:    21 January 2006

    Purpose:    This module provides filter processing for search.c 

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
#define UTL_LOG_CONTEXT                     (unsigned char *)"com.fsconsult.mps.src.search.filter"


/*---------------------------------------------------------------------------*/


/*
** Defines
*/

/* Search filter file protocol url */
#define SRCH_FILTER_FILE_PROTOCOL_URL               (unsigned char *)"file://"

/* Search filter file name extension */
#define SRCH_FILTER_FILENAME_EXTENSION              (unsigned char *)".txt"

/* Search filter field name separators */
#define SRCH_FILTER_FIELD_NAME_SEPARATORS           L":="

/* Search filter term separators */
#define SRCH_FILTER_TERM_SEPARATORS                 L", "


/*---------------------------------------------------------------------------*/


/*
** Private functions
*/

int static iSrchFilterGetSearchBitmapFromFilter (struct srchSearch *pssSrchSearch,
        struct srchIndex *psiSrchIndex, unsigned int uiLanguageID, 
        struct srchParserFilter *pspfSrchParserFilter, 
        unsigned int uiStartDocumentID, unsigned int uiEndDocumentID, 
        struct srchBitmap **ppsbSrchBitmap);


static int iSrchFilterProcessTerm (struct srchSearch *pssSrchSearch, struct srchIndex *psiSrchIndex, 
        void *pvLngStemmer, wchar_t *pwcTerm, unsigned char *pucFieldIDBitmap, unsigned int uiFieldIDBitmapLength, 
        wchar_t **ppwcTerm, boolean *pbFieldIDBitmapSet);


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchFilterGetSearchBitmapFromFilters()

    Purpose:    This function collects and searches filters and returns
                a bit map if there are any filters.
                
                A empty search bitmap will be returned if no terms were found.

    Parameters: pssSrchSearch               search structure
                psiSrchIndex                index structure
                uiLanguageID                language ID
                pspfSrchParserFilters       search parser filters
                uiSrchParserFiltersLength   search parser filters length
                uiStartDocumentID           start document ID restriction (0 for no restriction)
                uiEndDocumentID             end document ID restriction (0 for no restriction)
                uiBitmapMergeType           bitmap merge type
                ppsbSrchBitmap              return pointer for the search bitmap structure 

    Globals:    none

    Returns:    SRCH error code

*/
int iSrchFilterGetSearchBitmapFromFilters
(
    struct srchSearch *pssSrchSearch,
    struct srchIndex *psiSrchIndex,
    unsigned int uiLanguageID,
    struct srchParserFilter *pspfSrchParserFilters, 
    unsigned int uiSrchParserFiltersLength,
    unsigned int uiStartDocumentID,
    unsigned int uiEndDocumentID,
    unsigned int uiBitmapMergeType,
    struct srchBitmap **ppsbSrchBitmap
)
{

    int                         iError = SRCH_NoError;
    unsigned int                uiI = 0;
    struct srchParserFilter     *pspfSrchParserFiltersPtr = NULL;


    /* Check the parameters */
    if ( pssSrchSearch == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pssSrchSearch' parameter passed to 'iSrchFilterGetSearchBitmapFromFilters'."); 
        return (SRCH_FilterInvalidSearch);
    }

    if ( psiSrchIndex == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'psiSrchIndex' parameter passed to 'iSrchFilterGetSearchBitmapFromFilters'."); 
        return (SRCH_InvalidIndex);
    }

    if ( uiLanguageID < 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiLanguageID' parameter passed to 'iSrchFilterGetSearchBitmapFromFilters'."); 
        return (SRCH_FilterInvalidLanguageID);
    }

    if ( pspfSrchParserFilters == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pspfSrchParserFilters' parameter passed to 'iSrchFilterGetSearchBitmapFromFilters'."); 
        return (SRCH_FilterInvalidSearchParserFilters);
    }

    if ( uiSrchParserFiltersLength <= 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'uiSrchParserFiltersLength' parameter passed to 'iSrchFilterGetSearchBitmapFromFilters'."); 
        return (SRCH_FilterInvalidSearchParserFiltersLength);
    }

    if ( uiStartDocumentID < 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiStartDocumentID' parameter passed to 'iSrchFilterGetSearchBitmapFromFilters'."); 
        return (SRCH_FilterInvalidDocumentID);
    }

    if ( uiEndDocumentID < 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiEndDocumentID' parameter passed to 'iSrchFilterGetSearchBitmapFromFilters'."); 
        return (SRCH_FilterInvalidDocumentID);
    }

    if ( SRCH_FILTER_BITMAP_MERGE_TYPE_VALID(uiBitmapMergeType) == false ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiBitmapMergeType' parameter passed to 'iSrchFilterGetSearchBitmapFromFilters'."); 
        return (SRCH_FilterInvalidBitmapMergeType);
    }

    if ( ppsbSrchBitmap == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppsbSrchBitmap' parameter passed to 'iSrchFilterGetSearchBitmapFromFilters'."); 
        return (SRCH_ReturnParameterError);
    }


    /* Loop over each entry in the filters */
    for ( uiI = 0, pspfSrchParserFiltersPtr = pspfSrchParserFilters; uiI < uiSrchParserFiltersLength; uiI++, pspfSrchParserFiltersPtr++ ) {
        
        struct srchBitmap   *psbSrchBitmap = NULL;
        
        /* Create a search bitmap from this entry */
        if ( (iError = iSrchFilterGetSearchBitmapFromFilter(pssSrchSearch, psiSrchIndex, uiLanguageID, pspfSrchParserFiltersPtr, 
                uiStartDocumentID, uiEndDocumentID, &psbSrchBitmap)) != SRCH_NoError ) {
            goto bailFromiSrchFilterGetSearchBitmapFromFilters;
        }

        /* Not getting a bitmap means that none of the terms were found, so we allocate an empty bitmap */
        if ( psbSrchBitmap == NULL ) {
            if ( (iError = iSrchBitmapCreate(NULL, psiSrchIndex->uiDocumentCount + 1, false, &psbSrchBitmap)) != SRCH_NoError ) {
                iUtlLogError(UTL_LOG_CONTEXT, "Failed to create a new search bitmap, srch error: %d.", iError);
                goto bailFromiSrchFilterGetSearchBitmapFromFilters;
            }
        }

        /* Hand over the search bitmap if the return search bitmap is NULL */
        if ( *ppsbSrchBitmap == NULL ) {
            *ppsbSrchBitmap = psbSrchBitmap;
        }
        /* Merge the two search bitmaps if a search bitmap was returned */
        else if ( (*ppsbSrchBitmap != NULL) && (psbSrchBitmap != NULL) ) {
        
            /* Merge according to the bitmap merge type */
            if ( uiBitmapMergeType == SRCH_FILTER_BITMAP_MERGE_TYPE_XOR ) {
                iError = iSrchBitmapMergeXOR(*ppsbSrchBitmap, psbSrchBitmap, ppsbSrchBitmap);
            }
            else if ( uiBitmapMergeType == SRCH_FILTER_BITMAP_MERGE_TYPE_OR ) {
                iError = iSrchBitmapMergeOR(*ppsbSrchBitmap, psbSrchBitmap, ppsbSrchBitmap);
            } 
            else if ( uiBitmapMergeType == SRCH_FILTER_BITMAP_MERGE_TYPE_AND ) {
                iError = iSrchBitmapMergeAND(*ppsbSrchBitmap, psbSrchBitmap, ppsbSrchBitmap);
            } 
            else if ( uiBitmapMergeType == SRCH_FILTER_BITMAP_MERGE_TYPE_NOT ) {
                iError = iSrchBitmapMergeNOT(*ppsbSrchBitmap, psbSrchBitmap, ppsbSrchBitmap);
            }
            
            /* Handle error */
            if ( iError != SRCH_NoError ) {
                iUtlLogError(UTL_LOG_CONTEXT, "Failed to merge two search bitmaps from filter: '%ls', srch error: %d.", pspfSrchParserFiltersPtr->pwcFilter, iError); 
                goto bailFromiSrchFilterGetSearchBitmapFromFilters;
            }
        }
    }


    /* Not getting a bitmap means that none of the terms were found, so we allocate an empty bitmap */
    if ( (iError == SRCH_NoError) && (*ppsbSrchBitmap == NULL) ) {
        if ( (iError = iSrchBitmapCreate(NULL, psiSrchIndex->uiDocumentCount + 1, false, ppsbSrchBitmap)) != SRCH_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to create a new search bitmap, srch error: %d.", iError);
            goto bailFromiSrchFilterGetSearchBitmapFromFilters;
        }
    }



    /* Bail label */
    bailFromiSrchFilterGetSearchBitmapFromFilters:


    /* Handle the error */
    if ( iError != SRCH_NoError ) {
        iSrchBitmapFree(*ppsbSrchBitmap);
        *ppsbSrchBitmap = NULL;
    }


    return (iError);

}


/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchFilterGetSearchBitmapFromFilter()

    Purpose:    This function collects and searches filters and returns
                a bit map if there are any filters.
                
                The filter can either be a list name (mapping to a file),
                or a list of terms. If it is a list of terms, then each term has to be
                comma separated and must not contain any spaces.
                
                A search bitmap will not be allocated if no terms were found
                and an existing bitmap was not passed

    Parameters: pssSrchSearch           search structure
                psiSrchIndex            index structure
                uiLanguageID            language ID
                pspfSrchParserFilter    search parser string filter
                uiStartDocumentID       start document ID restriction (0 for no restriction)
                uiEndDocumentID         end document ID restriction (0 for no restriction)
                ppsbSrchBitmap          return pointer for the search bitmap structure 

    Globals:    none

    Returns:    SRCH error code

*/
static int iSrchFilterGetSearchBitmapFromFilter
(
    struct srchSearch *pssSrchSearch,
    struct srchIndex *psiSrchIndex,
    unsigned int uiLanguageID,
    struct srchParserFilter *pspfSrchParserFilter, 
    unsigned int uiStartDocumentID,
    unsigned int uiEndDocumentID,
    struct srchBitmap **ppsbSrchBitmap
)
{

    int                         iError = SRCH_NoError;
    void                        *pvLngStemmer = NULL;
    
    unsigned int                uiSrchParserSearchCacheID = SRCH_PARSER_MODIFIER_UNKNOWN_ID;
    
    unsigned char               pucConfigValue[SRCH_INFO_SYMBOL_MAXIMUM_LENGTH + 1] = {'\0'};
    unsigned char               pucFilterFilePath[UTL_FILE_PATH_MAX + 1] = {'\0'};
    FILE                        *pfFilterFile = NULL;
    off_t                       zFilterFileLength = 0;
    unsigned char               *pucFilterFileMappingPtr = NULL;
    time_t                      tFilterFileLastUpdate = (time_t)0;

    wchar_t                     *pwcTerms = NULL;
    wchar_t                     *pwcNormalizedExcludedTermsStrtokPtr = NULL;

    struct srchTermDictInfo     *pstdiSrchTermDictInfos = NULL;
    struct srchTermDictInfo     *pstdiSrchTermDictInfosPtr = NULL;
    unsigned int                uiSrchTermDictInfosLength = 0;
    unsigned int                uiI = 0;
    
    unsigned int                uiNormalizedTermsLength = 0;

    wchar_t                     *pwcTerm = NULL;
    unsigned char               *pucTerm = NULL;
    
    unsigned char               *pucFieldIDBitmap = NULL;

    

/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iSrchFilterGetSearchBitmapFromFilter - pspfSrchParserFilter->pwcFilter: '%ls'", pspfSrchParserFilter->pwcFilter); */


    ASSERT(pssSrchSearch != NULL);
    ASSERT(psiSrchIndex != NULL);
    ASSERT(uiLanguageID >= 0);
    ASSERT(pspfSrchParserFilter != NULL);
    ASSERT(uiStartDocumentID >= 0);
    ASSERT(uiEndDocumentID >= 0);
    ASSERT(ppsbSrchBitmap != NULL);


    /* Create the stemmer */
    if ( (iError = iLngStemmerCreateByID(psiSrchIndex->uiStemmerID, uiLanguageID, &pvLngStemmer)) != LNG_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to create a stemmer, lng error: %d.", iError);
        iError = SRCH_FilterCreateStemmerFailed;
        goto bailFromiSrchFilterGetSearchBitmapFromFilter;
    }

    /* Get the parser search cache ID */
    if ( (iError = iSrchParserGetModifierID(pssSrchSearch->pvSrchParser, SRCH_PARSER_MODIFIER_SEARCH_CACHE_ID, &uiSrchParserSearchCacheID)) != SRCH_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the parser search cache ID, srch error: %d.", iError);
        goto bailFromiSrchFilterGetSearchBitmapFromFilter;
    }


    /* Terms based filter */
    if ( pspfSrchParserFilter->uiFilterTypeID == SRCH_PARSER_FILTER_TYPE_TERMS_ID ) {
        ;
    }
    
    /* List based filter */
    else if ( pspfSrchParserFilter->uiFilterTypeID == SRCH_PARSER_FILTER_TYPE_LIST_ID ) {

        unsigned char   *pucFilterDirectoryPath = NULL;
        unsigned char   *pucFilterName = NULL;
        unsigned char   pucFilterFileName[UTL_FILE_PATH_MAX + 1] = {'\0'};


        /* Check for the filter file if there is a location for these files */
        if ( (iError = iUtlConfigGetValue(pssSrchSearch->pvUtlConfig, SRCH_SEARCH_CONFIG_FILTER_FILES_LOCATION, pucConfigValue, SRCH_INFO_SYMBOL_MAXIMUM_LENGTH + 1)) != UTL_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the file file location from the search configuration, symbol name: '%s', utl error: %d.", 
                    SRCH_SEARCH_CONFIG_FILTER_FILES_LOCATION, iError); 
            iError = SRCH_FilterInvalidFilterFileLocation;
            goto bailFromiSrchFilterGetSearchBitmapFromFilter;
        }

        /* Get a pointer to the filter directory path, exclude the file protocol url if it is there */
        pucFilterDirectoryPath = (s_strncasecmp(pucConfigValue, SRCH_FILTER_FILE_PROTOCOL_URL, s_strlen(SRCH_FILTER_FILE_PROTOCOL_URL)) == 0) ? 
                pucConfigValue + s_strlen(SRCH_FILTER_FILE_PROTOCOL_URL) : pucConfigValue;

        /* Convert the filter from wide characters to utf-8, pucFilterName is allocated */
        if ( (iError = iLngConvertWideStringToUtf8_d(pspfSrchParserFilter->pwcFilter, 0, &pucFilterName)) != LNG_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to convert a filter name from wide characters to utf-8, lng error: %d.", iError);
            iError = SRCH_FilterCharacterSetConvertionFailed;
            goto bailFromiSrchFilterGetSearchBitmapFromFilter;
        }

        /* Create the filter file name */
        snprintf(pucFilterFileName, UTL_FILE_PATH_MAX + 1, "%s%s", pucFilterName, SRCH_FILTER_FILENAME_EXTENSION);
        
        /* Free the utf-8 filter file name */
        s_free(pucFilterName);

        /* Create the filter file path */
        if ( (iError = iUtlFileMergePaths(pucFilterDirectoryPath, pucFilterFileName, pucFilterFilePath, UTL_FILE_PATH_MAX + 1)) != UTL_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to create the filter file path, file file name: '%s', filter directory path: '%s', utl error: %d", 
                    pucFilterFileName, pucFilterDirectoryPath, iError); 
            iError = SRCH_FilterInvalidFilterFile;
            goto bailFromiSrchFilterGetSearchBitmapFromFilter;
        }
    
        /* Check if the filter file is there */
        if ( bUtlFileIsFile(pucFilterFilePath) == false ) {
                iUtlLogError(UTL_LOG_CONTEXT, "Failed to find the filter file: '%s'.", pucFilterFilePath); 
            iError = SRCH_FilterInvalidFilterFile;
            goto bailFromiSrchFilterGetSearchBitmapFromFilter;
        }

        /* Open the filter file */
        if ( (pfFilterFile = s_fopen(pucFilterFilePath, "r")) == NULL ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to open the filter file: '%s'.", pucFilterFilePath); 
            iError = SRCH_FilterInvalidFilterFile;
            goto bailFromiSrchFilterGetSearchBitmapFromFilter;
        }

        /* Get the filter file modification date */            
        if ( (iError = iUtlFileGetFileModificationTimeT(pfFilterFile, &tFilterFileLastUpdate)) != UTL_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the last modification time of the filter file: '%s', utl error: %d.", pucFilterFilePath, iError); 
            iError = SRCH_FilterInvalidFilterFile;
            goto bailFromiSrchFilterGetSearchBitmapFromFilter;
        }
    }


    /* Get the bitmap from the search cache, note that we dont need to differentiate between terms or lists 
    ** since terms will have a tFilterFileLastUpdate of 0 and lists will have a non-zero tFilterFileLastUpdate 
    */
    if ( uiSrchParserSearchCacheID == SRCH_PARSER_MODIFIER_SEARCH_CACHE_ENABLE_ID ) {

        if ( (iError = iSrchCacheGetSearchBitmap(pssSrchSearch->pvSrchCache, psiSrchIndex, pspfSrchParserFilter->pwcFilter, 
                tFilterFileLastUpdate, ppsbSrchBitmap)) == SRCH_NoError ) {

            /* We are all set */
            goto bailFromiSrchFilterGetSearchBitmapFromFilter;
        }
    }

    
    /* Terms based filter */
    if ( pspfSrchParserFilter->uiFilterTypeID == SRCH_PARSER_FILTER_TYPE_TERMS_ID ) {
    
        /* Make a straight copy of the terms */
        if ( (pwcTerms = s_wcsdup(pspfSrchParserFilter->pwcFilter)) == NULL ) {
            iError = SRCH_MemError;
            goto bailFromiSrchFilterGetSearchBitmapFromFilter;
        }
        
        iSrchReportAppend(pssSrchSearch->pvSrchReport, "%s The search filtered on the following terms: '%ls'\n", REP_SEARCH_RESTRICTION, pspfSrchParserFilter->pwcFilter);

    }
    /* List based filter */
    else if ( pspfSrchParserFilter->uiFilterTypeID == SRCH_PARSER_FILTER_TYPE_LIST_ID ) {

        /* Get the file length */
        if ( (iError = iUtlFileGetFileLength(pfFilterFile, &zFilterFileLength)) != UTL_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the length of the filter file: '%s', utl error: %d.", pucFilterFilePath, iError); 
            iError = SRCH_FilterInvalidFilterFile;
            goto bailFromiSrchFilterGetSearchBitmapFromFilter;
        }
        
        /* Map in the entire file */
        if ( (iError = iUtlFileMemoryMap(fileno(pfFilterFile), 0, zFilterFileLength, PROT_READ, (void **)&pucFilterFileMappingPtr) != UTL_NoError) ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to map in the filter file: '%s', utl error: %d.", pucFilterFilePath, iError);
            iError = SRCH_FilterInvalidFilterFile;
            goto bailFromiSrchFilterGetSearchBitmapFromFilter;
        }

        /* Convert the terms from utf-8 to wide characters, pwcTerms is allocated */
        if ( (iError = iLngConvertUtf8ToWideString_d(pucFilterFileMappingPtr, zFilterFileLength, &pwcTerms)) != LNG_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to convert the terms from utf-8 to wide characters, lng error: %d.", iError);
            iError = SRCH_FilterCharacterSetConvertionFailed;
            goto bailFromiSrchFilterGetSearchBitmapFromFilter;
        }

        /* Unmap the filter file */
        iUtlFileMemoryUnMap(pucFilterFileMappingPtr, zFilterFileLength);
        pucFilterFileMappingPtr = NULL;

        /* Close the filter file */
        s_fclose(pfFilterFile);
        pfFilterFile = NULL;

        /* Convert newlines to commas */
        iUtlStringsReplaceCharacterInWideString(pwcTerms, L'\n', L',');

        iSrchReportAppend(pssSrchSearch->pvSrchReport, "%s The search filtered on the following list: '%ls'\n", REP_SEARCH_RESTRICTION, pspfSrchParserFilter->pwcFilter);
    }



    /* Allocate a field ID bitmap only if there are any fields other than field ID 0 */
    if ( psiSrchIndex->uiFieldIDMaximum > 0 ) {

        /* Allocate the field ID bitmap - field ID 0 is not a field */
        if ( (pucFieldIDBitmap = (unsigned char *)s_malloc(sizeof(unsigned char) * UTL_BITMAP_GET_BITMAP_BYTE_LENGTH(psiSrchIndex->uiFieldIDMaximum))) == NULL ) {
            iError = SRCH_MemError;
            goto bailFromiSrchFilterGetSearchBitmapFromFilter;
        }
    }


    /* Loop parsing the terms */
    for ( pwcTerm = s_wcstok(pwcTerms, SRCH_FILTER_TERM_SEPARATORS, (wchar_t **)&pwcNormalizedExcludedTermsStrtokPtr); pwcTerm != NULL; 
            pwcTerm = s_wcstok(NULL, SRCH_FILTER_TERM_SEPARATORS, (wchar_t **)&pwcNormalizedExcludedTermsStrtokPtr) ) {

        /* Trim the string if needed, we do this so that we can handle lists like 'bats, lions, elephants' */
        iUtlStringsTrimWideString(pwcTerm);

        /* Term contains spaces, treat as a phrase */
        if ( s_wcschr(pwcTerm, L' ') != NULL ) {

            wchar_t                     *pwcTermStrtokPtr = NULL;
            wchar_t                     *pwcSubTerm = NULL;
            struct srchPostingsList     *psplSrchPostingsList = NULL;

            /* Get the first subterm - wcstok_r along spaces */
            pwcSubTerm = s_wcstok(pwcTerm, L" ", (wchar_t **)&pwcTermStrtokPtr);

            /* Loop while there are subterm */
            while ( pwcSubTerm != NULL ) {

                boolean     bFieldIDBitmapSet = false;

                /* Clear the bitmap */
                if ( pucFieldIDBitmap != NULL ) {
                    UTL_BITMAP_CLEAR_POINTER(pucFieldIDBitmap, psiSrchIndex->uiFieldIDMaximum);
                }

                /* Process the term, this returns a pointer to the processed term and sets the bitmap as needed */
                if ( (iError = iSrchFilterProcessTerm(pssSrchSearch, psiSrchIndex, pvLngStemmer, pwcSubTerm, pucFieldIDBitmap, psiSrchIndex->uiFieldIDMaximum, 
                        &pwcSubTerm, &bFieldIDBitmapSet)) != SRCH_NoError ) {
                    goto bailFromiSrchFilterGetSearchBitmapFromFilter;
                }

                /* Skip the term if it was stemmed out of existence */
                if ( bUtlStringsIsWideStringNULL(pwcSubTerm) == false ) {

                    unsigned char               *pucSubTerm = NULL;
                    struct srchPostingsList     *psplSrchPostingsTempList = NULL;


                    /* Convert the subterm from wide characters to utf-8, pucSubTerm is allocated */
                    if ( (iError = iLngConvertWideStringToUtf8_d(pwcSubTerm, 0, &pucSubTerm)) != LNG_NoError ) {
                        iUtlLogError(UTL_LOG_CONTEXT, "Failed to convert a filter term from wide characters to utf-8, lng error: %d.", iError);
                        iError = SRCH_FilterCharacterSetConvertionFailed;
                        goto bailFromiSrchFilterGetSearchBitmapFromFilter;
                    }

                    /* Search to postings list */
                    iError = iSrchTermSearchGetSearchPostingsListFromTerm(pssSrchSearch, psiSrchIndex, pucSubTerm, SRCH_SEARCH_TERM_WEIGHT_DEFAULT, 
                            (bFieldIDBitmapSet == true) ? pucFieldIDBitmap : NULL, (bFieldIDBitmapSet == true) ? psiSrchIndex->uiFieldIDMaximum : 0, 
                            0, 0, 0, &psplSrchPostingsTempList);

                    /* Free the subterm */
                    s_free(pucSubTerm);

                    /* Check the returned error */
                    if ( iError != SRCH_NoError ) {
                        goto bailFromiSrchFilterGetSearchBitmapFromFilter;
                    }

                    
                    /* Break here if there was no terms returned as this will kill the phrase */
                    if ( (psplSrchPostingsTempList == NULL) || (psplSrchPostingsTempList->uiSrchPostingsLength == 0) ) {
                        
                        iSrchPostingFreeSrchPostingsList(psplSrchPostingsTempList);
                        psplSrchPostingsTempList = NULL;

                        iSrchPostingFreeSrchPostingsList(psplSrchPostingsList);
                        psplSrchPostingsList = NULL;

                        break;
                    }


                    /* Merge the postings lists, ADJ with a distance of 1 and a strict boolean match */
                    if ( psplSrchPostingsList != NULL ) {
                        if ( (iError = iSrchPostingMergeSrchPostingsListsADJ(psplSrchPostingsList, psplSrchPostingsTempList, 1, SRCH_PARSER_MODIFIER_BOOLEAN_OPERATION_STRICT_ID, &psplSrchPostingsList)) != SRCH_NoError ) {
                            goto bailFromiSrchFilterGetSearchBitmapFromFilter;
                        }
                    }
                    else {
                        psplSrchPostingsList = psplSrchPostingsTempList;
                    }
                }
            
                /* Get the next subterm */
                pwcSubTerm = s_wcstok(NULL, L" ", (wchar_t **)&pwcTermStrtokPtr);
            }
            
            
            /* Set the search bitmap structure from the search postings list structure */
            if ( psplSrchPostingsList != NULL ) {

                struct srchPosting      *pspSrchPostingsPtr = NULL;
                struct srchPosting      *pspSrchPostingsEnd = NULL;
                unsigned int            uiLastDocumentID = 0;
                
                unsigned char           *pucBitmapPtr = NULL;

                /* We only process the search postings list structure if there is something in it */
                if ( psplSrchPostingsList->uiSrchPostingsLength > 0 ) {
                
                    /* Allocate the search bitmap structure if it has not yet been allocated */
                    if ( *ppsbSrchBitmap == NULL ) {
                        if ( (iError = iSrchBitmapCreate(NULL, psiSrchIndex->uiDocumentCount + 1, false, ppsbSrchBitmap)) != SRCH_NoError ) {
                            iUtlLogError(UTL_LOG_CONTEXT, "Failed to create a new search bitmap, srch error: %d.", iError);
                            goto bailFromiSrchFilterGetSearchBitmapFromFilter;
                        }
                    }
    
    
                    /* Dereference the bitmap array */
                    pucBitmapPtr = (*ppsbSrchBitmap)->pucBitmap;
                
                    /* Set the bitmap for these documents  */
                    for ( pspSrchPostingsPtr = psplSrchPostingsList->pspSrchPostings, pspSrchPostingsEnd = psplSrchPostingsList->pspSrchPostings + psplSrchPostingsList->uiSrchPostingsLength; 
                            pspSrchPostingsPtr < pspSrchPostingsEnd; pspSrchPostingsPtr++ ) {
                        
                        /* Set the document in the bitmap if it has not already been set */
                        if ( pspSrchPostingsPtr->uiDocumentID != uiLastDocumentID ) {
                            UTL_BITMAP_SET_BIT_IN_POINTER(pucBitmapPtr, pspSrchPostingsPtr->uiDocumentID);
                            uiLastDocumentID = pspSrchPostingsPtr->uiDocumentID;
                        }
                    }
                }
                
                /* Free the postings list */
                iSrchPostingFreeSrchPostingsList(psplSrchPostingsList);
                psplSrchPostingsList = NULL;
            }
        }

        /* Term does not contain spaces, treat as a term */
        else {

            boolean     bFieldIDBitmapSet = false;

            /* Clear the bitmap */
            if ( pucFieldIDBitmap != NULL ) {
                UTL_BITMAP_CLEAR_POINTER(pucFieldIDBitmap, psiSrchIndex->uiFieldIDMaximum);
            }

            /* Process the term, this returns a pointer to the processed term and sets the bitmap as needed */
            if ( (iError = iSrchFilterProcessTerm(pssSrchSearch, psiSrchIndex, pvLngStemmer, pwcTerm, pucFieldIDBitmap, psiSrchIndex->uiFieldIDMaximum, 
                    &pwcTerm, &bFieldIDBitmapSet)) != SRCH_NoError ) {
                goto bailFromiSrchFilterGetSearchBitmapFromFilter;
            }
    
            /* Skip the term if it was stemmed out of existence */
            if ( bUtlStringsIsWideStringNULL(pwcTerm) == false ) {

                /* Convert the term from wide characters to utf-8, pucTerm is allocated */
                if ( (iError = iLngConvertWideStringToUtf8_d(pwcTerm, 0, &pucTerm)) != LNG_NoError ) {
                    iUtlLogError(UTL_LOG_CONTEXT, "Failed to convert a filter term from wide characters to utf-8, lng error: %d.", iError);
                    iError = SRCH_FilterCharacterSetConvertionFailed;
                    goto bailFromiSrchFilterGetSearchBitmapFromFilter;
                }

                /* Expand the term if it contains a wildcard (or two) */
                if ( s_strpbrk(pucTerm, SRCH_PARSER_WILDCARDS_STRING) != NULL ) {
                
                    /* Expand the term */
                    iError = iSrchTermDictLookupWildCard(psiSrchIndex, pucTerm, (bFieldIDBitmapSet == true) ? pucFieldIDBitmap : NULL, 
                            (bFieldIDBitmapSet == true) ? psiSrchIndex->uiFieldIDMaximum : 0, &pstdiSrchTermDictInfos, &uiSrchTermDictInfosLength);
                    
                    /* Check for non recoverable errors */
                    if ( !((iError == SRCH_NoError) || (iError == SRCH_TermDictTermBadRange) || (iError == SRCH_TermDictTermBadWildCard) ||
                            (iError == SRCH_IndexHasNoTerms) || (iError == SRCH_TermDictTermNotFound) || (iError == SRCH_TermDictTermDoesNotOccur)) ) {
                        goto bailFromiSrchFilterGetSearchBitmapFromFilter;
                    }
                    
                    /* Reset the error */
                    iError = SRCH_NoError;


                    /* Loop through all the terms in the term list */
                    for ( pstdiSrchTermDictInfosPtr = pstdiSrchTermDictInfos, uiI = 0; uiI < uiSrchTermDictInfosLength; pstdiSrchTermDictInfosPtr++, uiI++ ) {

                        /* Search to bitmap */
                        if ( (iError = iSrchTermSearchGetSearchBitmapFromTerm(pssSrchSearch, psiSrchIndex, pstdiSrchTermDictInfosPtr->pucTerm, 
                                (bFieldIDBitmapSet == true) ? pucFieldIDBitmap : NULL, ((bFieldIDBitmapSet == true) ? psiSrchIndex->uiFieldIDMaximum : 0), 
                                0, uiStartDocumentID, uiEndDocumentID, ppsbSrchBitmap)) != SRCH_NoError ) {
                            goto bailFromiSrchFilterGetSearchBitmapFromFilter;
                        }
                    }
                }
                else {
                
                    /* Search to bitmap */
                    if ( (iError = iSrchTermSearchGetSearchBitmapFromTerm(pssSrchSearch, psiSrchIndex, pucTerm, (bFieldIDBitmapSet == true) ? pucFieldIDBitmap : NULL, 
                            ((bFieldIDBitmapSet == true) ? psiSrchIndex->uiFieldIDMaximum : 0), 0, uiStartDocumentID, uiEndDocumentID, ppsbSrchBitmap)) != SRCH_NoError )  {
                        goto bailFromiSrchFilterGetSearchBitmapFromFilter;
                    }
                }


                /* Free the term */
                s_free(pucTerm);
            }
        }
    }



    /* Store the bipmap to cache if the cache is enabled */
    if ( uiSrchParserSearchCacheID == SRCH_PARSER_MODIFIER_SEARCH_CACHE_ENABLE_ID ) {
        if ( *ppsbSrchBitmap != NULL ) {
            iSrchCacheSaveSearchBitmap(pssSrchSearch->pvSrchCache, psiSrchIndex, pspfSrchParserFilter->pwcFilter, tFilterFileLastUpdate, *ppsbSrchBitmap);
        }    
    }    



    /* Bail label */
    bailFromiSrchFilterGetSearchBitmapFromFilter:


    /* Free the stemmer */
    iLngStemmerFree(pvLngStemmer);
    pvLngStemmer = NULL;

    /* List based filter */
    if ( pspfSrchParserFilter->uiFilterTypeID == SRCH_PARSER_FILTER_TYPE_LIST_ID ) {

        /* Unmap the terms file */
        iUtlFileMemoryUnMap(pucFilterFileMappingPtr, zFilterFileLength);
        pucFilterFileMappingPtr = NULL;
    
        /* Close the terms file */
        s_fclose(pfFilterFile);
    }
        
    /* Free the term information structure */
    if ( pstdiSrchTermDictInfos != NULL ) {
        for ( pstdiSrchTermDictInfosPtr = pstdiSrchTermDictInfos, uiI = 0; uiI < uiSrchTermDictInfosLength; pstdiSrchTermDictInfosPtr++, uiI++ ) {
            s_free(pstdiSrchTermDictInfosPtr->pucTerm);
        }
        s_free(pstdiSrchTermDictInfos);
    }

    s_free(pucFieldIDBitmap);

    s_free(pucTerm);
    s_free(pwcTerms);


    /* Handle the error */
    if ( iError != SRCH_NoError ) {
        iSrchBitmapFree(*ppsbSrchBitmap);
        *ppsbSrchBitmap = NULL;
    }


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchFilterProcessTerm()

    Purpose:    This function processes the passed term, stripping the field
                name (if present), setting the field ID bit map if needed
                and stemming the term

    Parameters: pssSrchSearch           search structure
                psiSrchIndex            index structure
                pvLngStemmer            stemmer
                pwcTerm                 term
                pucFieldIDBitmap        field ID bitmap (optional)
                uiFieldIDBitmapLength   field ID bitmap length (optional)
                ppwcTerm                return pointer for the processed term
                pbFieldIDBitmapSet      return flag set to true if the field ID bitmap was set

    Globals:    none

    Returns:    SRCH error code

*/
static int iSrchFilterProcessTerm
(
    struct srchSearch *pssSrchSearch,
    struct srchIndex *psiSrchIndex,
    void *pvLngStemmer,
    wchar_t *pwcTerm,
    unsigned char *pucFieldIDBitmap,
    unsigned int uiFieldIDBitmapLength,
    wchar_t **ppwcTerm,
    boolean *pbFieldIDBitmapSet
)
{

    int             iError = SRCH_NoError;
    unsigned char   pucFieldName[SRCH_INFO_SYMBOL_MAXIMUM_LENGTH + 1] = {'\0'};
    unsigned int    uiFieldID = 0;
    unsigned int    uiFieldOptions = SRCH_INFO_FIELD_OPTION_NONE;
    boolean         bFieldIDBitmapSet = false;
    boolean         bContainsUpperCase = false;
    wchar_t         *pwcPtr = NULL;

    
    ASSERT(pssSrchSearch != NULL);
    ASSERT(psiSrchIndex != NULL);
    ASSERT(pvLngStemmer != NULL);
    ASSERT(bUtlStringsIsStringNULL(pwcTerm) == false);
    ASSERT(((pucFieldIDBitmap == NULL) && (uiFieldIDBitmapLength <= 0)) || ((pucFieldIDBitmap != NULL) && (uiFieldIDBitmapLength > 0)));
    ASSERT(ppwcTerm != NULL);
    ASSERT(pbFieldIDBitmapSet != NULL);


    /* Set the field option defaults */
    iSrchInfoGetFieldOptionDefaults(psiSrchIndex, &uiFieldOptions);


    /* Extract field name if present */
    if ( (bUtlStringsIsWideStringUrl(pwcTerm) == false) && ((pwcPtr = s_wcspbrk(pwcTerm, SRCH_FILTER_FIELD_NAME_SEPARATORS)) != NULL) ) {
        
        wchar_t     *pwcFieldNamePtr = NULL;
    
        /* Set the field name */
        pwcFieldNamePtr = pwcTerm;
        
        /* Set the term */
        pwcTerm = pwcPtr + 1;

        /* Null terminate the field name, separating the term from the field name */
        *pwcPtr = L'\0';

        /* Convert the field name from wide characters to utf-8 */
        if ( (iError = iLngConvertWideStringToUtf8_s(pwcFieldNamePtr, 0, pucFieldName, SRCH_INFO_SYMBOL_MAXIMUM_LENGTH + 1)) != LNG_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to convert a field name from wide characters to utf-8, lng error: %d.", iError);
            return (SRCH_FilterCharacterSetConvertionFailed);
        }

        /* Look up the field ID for this field */
        if ( (iError = iSrchInfoGetFieldID(psiSrchIndex, pucFieldName, &uiFieldID)) == SRCH_NoError ) {
            
            /* Get the field options, reset them to the default if we cant get them */
            if ( (iError = iSrchInfoGetFieldInfo(psiSrchIndex, uiFieldID, NULL, 0, NULL, 0, NULL, &uiFieldOptions)) != SRCH_NoError ) {
                
                /* Set the field option defaults */
                iSrchInfoGetFieldOptionDefaults(psiSrchIndex, &uiFieldOptions);
            }


            /* Set the field ID in the bitmap */
            if ( pucFieldIDBitmap != NULL ) {

                /* Set the field ID in the bitmap - field ID 0 is not a field */
                UTL_BITMAP_SET_BIT_IN_POINTER(pucFieldIDBitmap, uiFieldID - 1);
            
                /* Field ID bit map is set */
                bFieldIDBitmapSet = true;
            }
        }
    }


    /* Check if there is an unfielded search option if the field ID bit map was not set, and if the field name is not the wildcard field name */
    if ( (bFieldIDBitmapSet == false) && ((pucFieldName == NULL) || (s_strcmp(pucFieldName, SRCH_PARSER_WILDCARD_FIELD_NAME_STRING) != 0)) ) {
    
        unsigned char   pucUnfieldedSearchFieldNames[SPI_FIELD_NAME_MAXIMUM_LENGTH + 1] = {"\0"};

        /* Get the unfielded search field names and, if they exist, use them for this search */
        if ( iSrchInfoGetUnfieldedSearchFieldNames(psiSrchIndex, pucUnfieldedSearchFieldNames, SPI_FIELD_NAME_MAXIMUM_LENGTH + 1) == SRCH_NoError ) {
            
            unsigned char   *pucUnfieldedSearchFieldNamePtr = NULL;
            unsigned char   *pucUnfieldedSearchFieldNamesStrtokPtr = NULL;
            

            /* Loop parsing the unfielded search field names */
            for ( pucUnfieldedSearchFieldNamePtr = (unsigned char *)s_strtok_r(pucUnfieldedSearchFieldNames, SRCH_INFO_UNFIELDED_SEARCH_FIELD_NAMES_SEPARATORS, (char **)&pucUnfieldedSearchFieldNamesStrtokPtr); 
                    pucUnfieldedSearchFieldNamePtr != NULL; 
                    pucUnfieldedSearchFieldNamePtr = (unsigned char *)s_strtok_r(NULL, SRCH_INFO_UNFIELDED_SEARCH_FIELD_NAMES_SEPARATORS, (char **)&pucUnfieldedSearchFieldNamesStrtokPtr) ) {

                /* Set the field ID in the bitmap */
                if ( iSrchInfoGetFieldID (psiSrchIndex, pucUnfieldedSearchFieldNamePtr, &uiFieldID) == SRCH_NoError ) {

                    ASSERT(uiFieldID <= uiFieldIDBitmapLength);

                    /* Set the field ID in the bitmap, we know there are fields other than field ID 0 
                    ** since there are unfielded search field names - field ID 0 is not a field 
                    */
                    UTL_BITMAP_SET_BIT_IN_POINTER(pucFieldIDBitmap, uiFieldID - 1);

                    /* Field ID bit map is set */
                    bFieldIDBitmapSet = true;
                }
            }
        }
    }


    /* Set the mixed case flag, which really means 'does this term contain upper case, or non-alphanumerics' */
    bContainsUpperCase = bLngCaseDoesWideStringContainUpperCase(pwcTerm);

    /* Stem the term if we are stemming on this field and if the term does not end in a wildcard  */
    if ( (bSrchInfoFieldOptionStemming(uiFieldOptions) == true) && (bContainsUpperCase == false) && (s_wcschr(SRCH_PARSER_WILDCARDS_WSTRING    , pwcTerm[s_wcslen(pwcTerm) - 1]) == NULL) ) {
        if ( (iError = iLngStemmerStemTerm(pvLngStemmer, pwcTerm, 0)) != LNG_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to stem a term, lng error: %d.", iError);
            return (SRCH_FeedbackStemmingFailed);
        }
    }


    /* Set the return pointers */
    *ppwcTerm = pwcTerm;
    *pbFieldIDBitmapSet = bFieldIDBitmapSet;


    return (SRCH_NoError);

}


/*---------------------------------------------------------------------------*/





