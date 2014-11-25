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

    Module:     report.c

    Author:     Francois Schiettecatte (FS Consulting LLC.)

    Created:    4 April 2004

    Purpose:    This module implements a search report formatter and
                a search report merger. 

*/


/*---------------------------------------------------------------------------*/


/*
** Includes
*/

#include "utils.h"

#include "report.h"


/*---------------------------------------------------------------------------*/


/*
** Feature defines
*/

/* Context for logging */
#undef UTL_LOG_CONTEXT
#define UTL_LOG_CONTEXT                     (unsigned char *)"com.fsconsult.mps.src.report.report"


/*---------------------------------------------------------------------------*/


/*
** Feature defines
*/

/* Enable the for rounding of inaccurate counts */
/* #define REP_TYPE_ENABLE_ROUNDING_FOR_INACCURATE_COUNTS */


/*---------------------------------------------------------------------------*/


/*
** Defines
*/

#define REP_TYPE_UNKNOWN                (0)
#define REP_TYPE_MESSAGE                (1)
#define REP_TYPE_TERM                   (2)
#define REP_TYPE_TERM_EXPANDED          (3)


#define REP_LINE_MAXIMUM_LENGTH         (5120)


/*---------------------------------------------------------------------------*/


/*
** Structures
*/

/* Search type for search term and search expanded */
struct repSearchType {
    unsigned int    uiType;
    unsigned char   *pucSearchMessage;
    unsigned char   *pucSearchTerm;
    unsigned char   *pucSearchField;
    unsigned char   *pucSearchTermStemmed;
    float           fTermWeight;
    int             iTermCount;
    int             iDocCount;
    void            *pvUtlExpandedTermTrie;
};


/* Search type for search term and search expanded */
struct repTrieTermEntry {
    int             iTermCount;
    int             iDocCount;
};


/*---------------------------------------------------------------------------*/


/*
** Private function prototypes
*/

static int iRepHashPrintCallBackFunction (unsigned char *pucKey, void *pvData, va_list ap);

static int iRepTriePrintCallBackFunction (unsigned char *pucKey, void *pvData, va_list ap);


/*---------------------------------------------------------------------------*/


/*

    Function:   iRepMergeSearchReports()

    Purpose:    This function merges the passed search reports. The search reports
                is passed as a NULL terminated array of search reports. These search
                reports can be individual search reports, or concatenated search reports.
                
                A pointer to the merged search report will be returned.

    Parameters: ppucSearchReportList        NULL terminated array of search reports
                ppucSearchReportMerged      return parameter for the merged search report

    Globals:    none

    Returns:    REP error code

*/
int iRepMergeSearchReports
(
    unsigned char **ppucSearchReportList,
    unsigned char **ppucSearchReportMerged
)
{

    int                     iError = REP_NoError;
    unsigned int            uiI = 0;
    
    unsigned char           *pucSearchReportPtr = NULL;
    void                    *pvUtlSearchReportMergedStringBuffer = NULL;
    unsigned char           pucSearchReportLine[REP_LINE_MAXIMUM_LENGTH + 1];


    unsigned char           pucIndexName[REP_LINE_MAXIMUM_LENGTH + 1] = {'\0'};

    boolean                 bGotIndexCounts = 0;
    unsigned long           ulUniqueTermCount = 0;
    unsigned long           ulTotalTermCount = 0;
    unsigned long           ulUniqueStopTermCount = 0;
    unsigned long           ulTotalStopTermCount = 0;
    unsigned int            uiDocumentCount = 0;

    unsigned char           pucStemmerName[REP_LINE_MAXIMUM_LENGTH + 1] = {'\0'};

    unsigned char           pucSearchOriginal[REP_LINE_MAXIMUM_LENGTH + 1] = {'\0'};
    unsigned char           pucSearchReformatted[REP_LINE_MAXIMUM_LENGTH + 1] = {'\0'};

    unsigned char           pucSearchError[REP_LINE_MAXIMUM_LENGTH + 1] = {'\0'};
    void                    *pvUtlSearchErrorsHash = NULL;

    unsigned char           pucSearchSetting[REP_LINE_MAXIMUM_LENGTH + 1] = {'\0'};
    void                    *pvUtlSearchSettingsHash = NULL;

    unsigned char           pucSearchRestriction[REP_LINE_MAXIMUM_LENGTH + 1] = {'\0'};
    void                    *pvUtlSearchRestrictionsHash = NULL;

    unsigned char           pucSearchDebug[REP_LINE_MAXIMUM_LENGTH + 1] = {'\0'};
    void                    *pvUtlSearchDebugsHash = NULL;

    unsigned char           pucSearchWarning[REP_LINE_MAXIMUM_LENGTH + 1] = {'\0'};
    void                    *pvUtlSearchWarningsHash = NULL;

    struct repSearchType    **pprstRepSearchType = NULL;
    struct repSearchType    *prstRepSearchType = NULL;
    unsigned int            uiSearchTypeLen = 0;
    int                     iSearchTypeIndex = -1;

    unsigned char           pucSearchMessage[REP_LINE_MAXIMUM_LENGTH + 1] = {'\0'};
    unsigned char           pucSearchTerm[REP_LINE_MAXIMUM_LENGTH + 1] = {'\0'};
    unsigned char           pucSearchField[REP_LINE_MAXIMUM_LENGTH + 1] = {'\0'};
    unsigned char           pucSearchTermStemmed[REP_LINE_MAXIMUM_LENGTH + 1] = {'\0'};
    float                   fTermWeight = 0.0;
    int                     iTermCount = 0;
    int                     iDocCount = 0;
    
    unsigned int            uiPositiveFeedbackTotalTermCount = 0;
    unsigned int            uiPositiveFeedbackUniqueTermCount = 0;
    unsigned int            uiPositiveFeedbackUsedTermCount = 0;
    unsigned char           pucPositiveFeedbackTerm[REP_LINE_MAXIMUM_LENGTH + 1] = {'\0'};
    void                    *pvUtlPositiveFeedbackTermsHash = NULL;

    unsigned int            uiNegativeFeedbackTotalTermCount = 0;
    unsigned int            uiNegativeFeedbackUniqueTermCount = 0;
    unsigned int            uiNegativeFeedbackUsedTermCount = 0;
    unsigned char           pucNegativeFeedbackTerm[REP_LINE_MAXIMUM_LENGTH + 1] = {'\0'};
    void                    *pvUtlNegativeFeedbackTermsHash = NULL;

    unsigned int            uiDocumentsRetrievedCount = 0;
    unsigned int            uiFailedDateMatchCount = 0;
    unsigned int            uiFailedTermCountMatchCount = 0;
    unsigned int            uiFailedLanguageMatchCount = 0;
    unsigned int            uiExcludedDocumentCount = 0;
    unsigned int            uiIncludedDocumentCount = 0;
    boolean                 bAccurate = true;


    unsigned char           pucIndexNameScanfFormat[UTL_STRING_SCANF_FORMAT_LENGTH + 1] = {'\0'};
    unsigned char           pucIndexCountsScanfFormat[UTL_STRING_SCANF_FORMAT_LENGTH + 1] = {'\0'};
    unsigned char           pucStemmerNameScanfFormat[UTL_STRING_SCANF_FORMAT_LENGTH + 1] = {'\0'};
    unsigned char           pucSearchOriginalScanfFormat[UTL_STRING_SCANF_FORMAT_LENGTH + 1] = {'\0'};
    unsigned char           pucSearchReformattedScanfFormat[UTL_STRING_SCANF_FORMAT_LENGTH + 1] = {'\0'};
    unsigned char           pucSearchErrorScanfFormat[UTL_STRING_SCANF_FORMAT_LENGTH + 1] = {'\0'};
    unsigned char           pucSearchSettingScanfFormat[UTL_STRING_SCANF_FORMAT_LENGTH + 1] = {'\0'};
    unsigned char           pucSearchRestrictionScanfFormat[UTL_STRING_SCANF_FORMAT_LENGTH + 1] = {'\0'};
    unsigned char           pucSearchDebugScanfFormat[UTL_STRING_SCANF_FORMAT_LENGTH + 1] = {'\0'};
    unsigned char           pucSearchWarningScanfFormat[UTL_STRING_SCANF_FORMAT_LENGTH + 1] = {'\0'};
    unsigned char           pucSearchMessageScanfFormat[UTL_STRING_SCANF_FORMAT_LENGTH + 1] = {'\0'};
    unsigned char           pucSearchTermScanfFormat[UTL_STRING_SCANF_FORMAT_LENGTH + 1] = {'\0'};
    unsigned char           pucSearchTermExpandedScanfFormat[UTL_STRING_SCANF_FORMAT_LENGTH + 1] = {'\0'};
    unsigned char           pucSearchTermExpandedTermScanfFormat[UTL_STRING_SCANF_FORMAT_LENGTH + 1] = {'\0'};
    unsigned char           pucSearchTermExpandedCountsScanfFormat[UTL_STRING_SCANF_FORMAT_LENGTH + 1] = {'\0'};
    unsigned char           pucPositiveFeedbackCountsScanfFormat[UTL_STRING_SCANF_FORMAT_LENGTH + 1] = {'\0'};
    unsigned char           pucPositiveFeedbackTermsScanfFormat[UTL_STRING_SCANF_FORMAT_LENGTH + 1] = {'\0'};
    unsigned char           pucNegativeFeedbackCountsScanfFormat[UTL_STRING_SCANF_FORMAT_LENGTH + 1] = {'\0'};
    unsigned char           pucNegativeFeedbackTermsScanfFormat[UTL_STRING_SCANF_FORMAT_LENGTH + 1] = {'\0'};
    unsigned char           pucRetrievalCountsScanfFormat[UTL_STRING_SCANF_FORMAT_LENGTH + 1] = {'\0'};


    /* Check the parameters */
    if ( ppucSearchReportList == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppucSearchReportList' parameter passed to 'iRepMergeSearchReports'."); 
        return (REP_InvalidSearchReportList);
    }

    if ( ppucSearchReportMerged == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppucSearchReportMerged' parameter passed to 'iRepMergeSearchReports'."); 
        return (REP_ReturnParameterError);
    }


    /* Create the scanf formats */
    snprintf(pucIndexNameScanfFormat, UTL_STRING_SCANF_FORMAT_LENGTH + 1, "%%%d[^\n]", REP_LINE_MAXIMUM_LENGTH);
    snprintf(pucIndexCountsScanfFormat, UTL_STRING_SCANF_FORMAT_LENGTH + 1, "%%lu %%lu %%lu %%lu %%u");
    snprintf(pucStemmerNameScanfFormat, UTL_STRING_SCANF_FORMAT_LENGTH + 1, "%%%d[^\n]", REP_LINE_MAXIMUM_LENGTH);
    snprintf(pucSearchOriginalScanfFormat, UTL_STRING_SCANF_FORMAT_LENGTH + 1, "%%%d[^\n]", REP_LINE_MAXIMUM_LENGTH);
    snprintf(pucSearchReformattedScanfFormat, UTL_STRING_SCANF_FORMAT_LENGTH + 1, "%%%d[^\n]", REP_LINE_MAXIMUM_LENGTH);
    snprintf(pucSearchErrorScanfFormat, UTL_STRING_SCANF_FORMAT_LENGTH + 1, "%%%d[^\n]", REP_LINE_MAXIMUM_LENGTH);
    snprintf(pucSearchSettingScanfFormat, UTL_STRING_SCANF_FORMAT_LENGTH + 1, "%%%d[^\n]", REP_LINE_MAXIMUM_LENGTH);
    snprintf(pucSearchRestrictionScanfFormat, UTL_STRING_SCANF_FORMAT_LENGTH + 1, "%%%d[^\n]", REP_LINE_MAXIMUM_LENGTH);
    snprintf(pucSearchDebugScanfFormat, UTL_STRING_SCANF_FORMAT_LENGTH + 1, "%%%d[^\n]", REP_LINE_MAXIMUM_LENGTH);
    snprintf(pucSearchWarningScanfFormat, UTL_STRING_SCANF_FORMAT_LENGTH + 1, "%%%d[^\n]", REP_LINE_MAXIMUM_LENGTH);
    snprintf(pucSearchMessageScanfFormat, UTL_STRING_SCANF_FORMAT_LENGTH + 1, "%%%d[^\n]", REP_LINE_MAXIMUM_LENGTH);
    snprintf(pucSearchTermScanfFormat, UTL_STRING_SCANF_FORMAT_LENGTH + 1, "%%%ds %%%ds %%%ds %%f %%d %%d", REP_LINE_MAXIMUM_LENGTH, REP_LINE_MAXIMUM_LENGTH, REP_LINE_MAXIMUM_LENGTH);
    snprintf(pucSearchTermExpandedScanfFormat, UTL_STRING_SCANF_FORMAT_LENGTH + 1, "%%%ds %%%ds %%f", REP_LINE_MAXIMUM_LENGTH, REP_LINE_MAXIMUM_LENGTH);
    snprintf(pucSearchTermExpandedTermScanfFormat, UTL_STRING_SCANF_FORMAT_LENGTH + 1, "%%%ds %%d %%d", REP_LINE_MAXIMUM_LENGTH);
    snprintf(pucSearchTermExpandedCountsScanfFormat, UTL_STRING_SCANF_FORMAT_LENGTH + 1, "%%d %%d");
    snprintf(pucPositiveFeedbackCountsScanfFormat, UTL_STRING_SCANF_FORMAT_LENGTH + 1, "%%u %%u %%u");
    snprintf(pucPositiveFeedbackTermsScanfFormat, UTL_STRING_SCANF_FORMAT_LENGTH + 1, "%%%ds", REP_LINE_MAXIMUM_LENGTH);
    snprintf(pucNegativeFeedbackCountsScanfFormat, UTL_STRING_SCANF_FORMAT_LENGTH + 1, "%%u %%u %%u");
    snprintf(pucNegativeFeedbackTermsScanfFormat, UTL_STRING_SCANF_FORMAT_LENGTH + 1, "%%%ds", REP_LINE_MAXIMUM_LENGTH);
    snprintf(pucRetrievalCountsScanfFormat, UTL_STRING_SCANF_FORMAT_LENGTH + 1, "%%u %%u %%u %%u %%u %%u %%u");


    /* Loop over each search report in the NULL terminated list */
    for ( uiI = 0; ppucSearchReportList[uiI] != NULL; uiI++ ) {
        
        /* Dereference the search report for convenience */
        unsigned char *pucSearchReport = ppucSearchReportList[uiI];

        /* Loop over each line in the search report */
        for ( pucSearchReportPtr = pucSearchReport; pucSearchReportPtr != NULL; ) {
            
            /* Process the index name */
            if ( s_strncmp(pucSearchReportPtr, REP_INDEX_NAME, s_strlen(REP_INDEX_NAME)) == 0 ) {
                
                /* Extract the index name */
                if ( sscanf(pucSearchReportPtr + s_strlen(REP_INDEX_NAME) + 1, pucIndexNameScanfFormat, pucIndexName) != 1 ) {
                    iUtlLogWarn(UTL_LOG_CONTEXT, "Failed to extract the index name from the search report.");
                    pucIndexName[0] = '\0';
                }
                
/*                 iUtlLogDebug(UTL_LOG_CONTEXT, "reset: [%d]", iSearchTypeIndex); */

                /* Reset the search type index since this is a new index */
                iSearchTypeIndex = -1;
            }
            
            /* Process the index counts */
            else if ( s_strncmp(pucSearchReportPtr, REP_INDEX_COUNTS, s_strlen(REP_INDEX_COUNTS)) == 0 ) {
                
                unsigned long   ulTempTotalTermCount = 0;
                unsigned long   ulTempUniqueTermCount = 0;
                unsigned long   ulTempTotalStopTermCount = 0;
                unsigned long   ulTempUniqueStopTermCount = 0;
                unsigned int    uiTempDocumentCount = 0;

                /* Extract the index counts */
                if ( sscanf(pucSearchReportPtr + s_strlen(REP_INDEX_COUNTS) + 1, pucIndexCountsScanfFormat, &ulTempTotalTermCount, &ulTempUniqueTermCount, 
                        &ulTempTotalStopTermCount, &ulTempUniqueStopTermCount, &uiTempDocumentCount) == 5 ) {

                    ulTotalTermCount += ulTempTotalTermCount;
                    ulUniqueTermCount = UTL_MACROS_MAX(ulTempUniqueTermCount, ulUniqueTermCount);
                    ulTotalStopTermCount += ulTempTotalStopTermCount;
                    ulUniqueStopTermCount = UTL_MACROS_MAX(ulTempUniqueStopTermCount, ulUniqueStopTermCount);
                    uiDocumentCount += uiTempDocumentCount;
                }
                else {
                    iUtlLogWarn(UTL_LOG_CONTEXT, "Failed to extract the index counts from the search report.");
                }
                
                /* Set the flag indicating that we got index counts */
                bGotIndexCounts = true;

            }
    
            /* Process the stemmer name */
            else if ( s_strncmp(pucSearchReportPtr, REP_STEMMER_NAME, s_strlen(REP_STEMMER_NAME)) == 0 ) {
                
                /* Extract the stemmer name */
                if ( sscanf(pucSearchReportPtr + s_strlen(REP_STEMMER_NAME) + 1, pucStemmerNameScanfFormat, pucStemmerName) != 1 ) {
                    iUtlLogWarn(UTL_LOG_CONTEXT, "Failed to extract the stemmer name from the search report.");
                    pucStemmerName[0] = '\0';
                }
            }
            
            /* Process the search original */
            else if ( s_strncmp(pucSearchReportPtr, REP_SEARCH_ORIGINAL, s_strlen(REP_SEARCH_ORIGINAL)) == 0 ) {
                
                /* Extract the search original */
                if ( sscanf(pucSearchReportPtr + s_strlen(REP_SEARCH_ORIGINAL) + 1, pucSearchOriginalScanfFormat, pucSearchOriginal) != 1 ) {
                    iUtlLogWarn(UTL_LOG_CONTEXT, "Failed to extract the search original from the search report.");
                    pucSearchOriginal[0] = '\0';
                }
            }
            
            /* Process the search reformatted */
            else if ( s_strncmp(pucSearchReportPtr, REP_SEARCH_REFORMATTED, s_strlen(REP_SEARCH_REFORMATTED)) == 0 ) {
                
                /* Extract the search reformatted */
                if ( sscanf(pucSearchReportPtr + s_strlen(REP_SEARCH_REFORMATTED) + 1, pucSearchReformattedScanfFormat, pucSearchReformatted) != 1 ) {
                    iUtlLogWarn(UTL_LOG_CONTEXT, "Failed to extract the search reformatted from the search report.");
                    pucSearchReformatted[0] = '\0';
                }
            }
            
            /* Process the search error */
            else if ( s_strncmp(pucSearchReportPtr, REP_SEARCH_ERROR, s_strlen(REP_SEARCH_ERROR)) == 0 ) {
                
                /* Extract the search error */
                if ( sscanf(pucSearchReportPtr + s_strlen(REP_SEARCH_ERROR) + 1, pucSearchErrorScanfFormat, pucSearchError) == 1 ) {
                    
                    void    **ppvDatum = NULL;

                    /* Create the search errors hash if it does not exist */
                    if ( pvUtlSearchErrorsHash == NULL ) {
                        if ( (iError = iUtlHashCreate(&pvUtlSearchErrorsHash)) != UTL_NoError ) {
                            iUtlLogError(UTL_LOG_CONTEXT, "Failed to create the search errors hash, utl error: %d.", iError);
                            goto bailFromRepMerger;
                        }
                    }

                    /* Add the search error to the search errors hash */
                    if ( (iError = iUtlHashAdd(pvUtlSearchErrorsHash, pucSearchError, &ppvDatum)) != UTL_NoError ) {
                        iUtlLogError(UTL_LOG_CONTEXT, "Failed to a the symbol to the search errors hash, symbol: '%s', utl error: %d.", pucSearchError, iError);
                        goto bailFromRepMerger;
                    }
                }
                else {
                    iUtlLogWarn(UTL_LOG_CONTEXT, "Failed to extract the search error from the search report.");
                    pucSearchError[0] = '\0';
                }
            }
            
            /* Process the search setting */
            else if ( s_strncmp(pucSearchReportPtr, REP_SEARCH_SETTING, s_strlen(REP_SEARCH_SETTING)) == 0 ) {
                
                /* Extract the search setting */
                if ( sscanf(pucSearchReportPtr + s_strlen(REP_SEARCH_SETTING) + 1, pucSearchSettingScanfFormat, pucSearchSetting) == 1 ) {
                    
                    void    **ppvDatum = NULL;

                    /* Create the search settings hash if it does not exist */
                    if ( pvUtlSearchSettingsHash == NULL ) {
                        if ( (iError = iUtlHashCreate(&pvUtlSearchSettingsHash)) != UTL_NoError ) {
                            iUtlLogError(UTL_LOG_CONTEXT, "Failed to create the search settings hash, utl error: %d.", iError);
                            goto bailFromRepMerger;
                        }
                    }

                    /* Add the search settings to the search settingshash */
                    if ( (iError = iUtlHashAdd(pvUtlSearchSettingsHash, pucSearchSetting, &ppvDatum)) != UTL_NoError ) {
                        iUtlLogError(UTL_LOG_CONTEXT, "Failed to a the symbol to the search settings hash, symbol: '%s', utl error: %d.", pucSearchSetting, iError);
                        goto bailFromRepMerger;
                    }
                }
                else {
                    iUtlLogWarn(UTL_LOG_CONTEXT, "Failed to extract the search setting from the search report.");
                    pucSearchSetting[0] = '\0';
                }
            }
            
            /* Process the search restriction */
            else if ( s_strncmp(pucSearchReportPtr, REP_SEARCH_RESTRICTION, s_strlen(REP_SEARCH_RESTRICTION)) == 0 ) {
                
                /* Extract the search restriction */
                if ( sscanf(pucSearchReportPtr + s_strlen(REP_SEARCH_RESTRICTION) + 1, pucSearchRestrictionScanfFormat, pucSearchRestriction) == 1 ) {
                    
                    void    **ppvDatum = NULL;

                    /* Create the search restrictions hash if it does not exist */
                    if ( pvUtlSearchRestrictionsHash == NULL ) {
                        if ( (iError = iUtlHashCreate(&pvUtlSearchRestrictionsHash)) != UTL_NoError ) {
                            iUtlLogError(UTL_LOG_CONTEXT, "Failed to create the search restrictions hash, utl error: %d.", iError);
                            goto bailFromRepMerger;
                        }
                    }

                    /* Add the search restriction to the search restrictions hash */
                    if ( (iError = iUtlHashAdd(pvUtlSearchRestrictionsHash, pucSearchRestriction, &ppvDatum)) != UTL_NoError ) {
                        iUtlLogError(UTL_LOG_CONTEXT, "Failed to add a symbol to the search restriction hash, symbol: '%s', utl error: %d.", pucSearchRestriction, iError);
                        goto bailFromRepMerger;
                    }
                }
                else {
                    iUtlLogWarn(UTL_LOG_CONTEXT, "Failed to extract the search restriction from the search report.");
                    pucSearchRestriction[0] = '\0';
                }
            }

            /* Process the search debug */
            else if ( s_strncmp(pucSearchReportPtr, REP_SEARCH_DEBUG, s_strlen(REP_SEARCH_DEBUG)) == 0 ) {
                
                /* Extract the search debug */
                if ( sscanf(pucSearchReportPtr + s_strlen(REP_SEARCH_DEBUG) + 1, pucSearchDebugScanfFormat, pucSearchDebug) == 1 ) {
                    
                    void    **ppvDatum = NULL;

                    /* Create the search debugs hash if it does not exist */
                    if ( pvUtlSearchDebugsHash == NULL ) {
                        if ( (iError = iUtlHashCreate(&pvUtlSearchDebugsHash)) != UTL_NoError ) {
                            iUtlLogError(UTL_LOG_CONTEXT, "Failed to create the search debugs hash, utl error: %d.", iError);
                            goto bailFromRepMerger;
                        }
                    }

                    /* Add the search debug to the search debugs hash */
                    if ( (iError = iUtlHashAdd(pvUtlSearchDebugsHash, pucSearchDebug, &ppvDatum)) != UTL_NoError ) {
                        iUtlLogError(UTL_LOG_CONTEXT, "Failed to add a symbol to the search debug hash, symbol: '%s', utl error: %d.", pucSearchDebug, iError);
                        goto bailFromRepMerger;
                    }
                }
                else {
                    iUtlLogWarn(UTL_LOG_CONTEXT, "Failed to extract the search debug from the search report.");
                    pucSearchRestriction[0] = '\0';
                }
            }

            /* Process the search warning */
            else if ( s_strncmp(pucSearchReportPtr, REP_SEARCH_WARNING, s_strlen(REP_SEARCH_WARNING)) == 0 ) {
                
                /* Extract the search warning */
                if ( sscanf(pucSearchReportPtr + s_strlen(REP_SEARCH_WARNING) + 1, pucSearchWarningScanfFormat, pucSearchWarning) == 1 ) {
                    
                    void    **ppvDatum = NULL;

                    /* Create the search warnings hash if it does not exist */
                    if ( pvUtlSearchWarningsHash == NULL ) {
                        if ( (iError = iUtlHashCreate(&pvUtlSearchWarningsHash)) != UTL_NoError ) {
                            iUtlLogError(UTL_LOG_CONTEXT, "Failed to create the search warnings hash, utl error: %d.", iError);
                            goto bailFromRepMerger;
                        }
                    }

                    /* Add the search error to the search warnings hash */
                    if ( (iError = iUtlHashAdd(pvUtlSearchWarningsHash, pucSearchWarning, &ppvDatum)) != UTL_NoError ) {
                        iUtlLogError(UTL_LOG_CONTEXT, "Failed to add a symbol to the search warnings hash, symbol: '%s', utl error: %d.", pucSearchWarning, iError);
                        goto bailFromRepMerger;
                    }
                }
                else {
                    iUtlLogWarn(UTL_LOG_CONTEXT, "Failed to extract the search warning from the search report.");
                    pucSearchWarning[0] = '\0';
                }
            }


            
            /* Process the search message */
            else if ( s_strncmp(pucSearchReportPtr, REP_SEARCH_MESSAGE, s_strlen(REP_SEARCH_MESSAGE)) == 0 ) {
                
                /* Extract the search message */
                if ( sscanf(pucSearchReportPtr + s_strlen(REP_SEARCH_MESSAGE) + 1, pucSearchMessageScanfFormat, pucSearchMessage) == 1 ) {
                
                    /* Allocate the search types if it is not allocated and allocate a search type */
                    if ( pprstRepSearchType == NULL ) {

                        uiSearchTypeLen = 1;
                        iSearchTypeIndex = 0;

                        if ( (pprstRepSearchType = (struct repSearchType **)s_malloc((size_t)(sizeof(struct repSearchType *) * uiSearchTypeLen))) == NULL ) {
                            iError = REP_MemError;
                            goto bailFromRepMerger;
                        }
                        if ( (prstRepSearchType = (struct repSearchType *)s_malloc((size_t)sizeof(struct repSearchType))) == NULL ) {
                            iError = REP_MemError;
                            goto bailFromRepMerger;
                        }

                        prstRepSearchType->uiType = REP_TYPE_UNKNOWN;
                        pprstRepSearchType[iSearchTypeIndex] = prstRepSearchType;
                    }
                    /* Extend the search types and allocate a search type */
                    else if ( (uiSearchTypeLen - iSearchTypeIndex) == 1 ) {

                        uiSearchTypeLen++;
                        iSearchTypeIndex++;

                        if ( (pprstRepSearchType = (struct repSearchType **)s_realloc(pprstRepSearchType, (size_t)(sizeof(struct repSearchType *) * uiSearchTypeLen))) == NULL ) {
                            iError = REP_MemError;
                            goto bailFromRepMerger;
                        }
                        if ( (prstRepSearchType = (struct repSearchType *)s_malloc((size_t)sizeof(struct repSearchType))) == NULL ) {
                            iError = REP_MemError;
                            goto bailFromRepMerger;
                        }

                        prstRepSearchType->uiType = REP_TYPE_UNKNOWN;
                        pprstRepSearchType[iSearchTypeIndex] = prstRepSearchType;
                    }
                    /* Dereference the search type, it is already there */
                    else {
                        iSearchTypeIndex++;
                        prstRepSearchType = pprstRepSearchType[iSearchTypeIndex];
                    }
                    
/*                     iUtlLogDebug(UTL_LOG_CONTEXT, "[%s][%s]", pucSearchTerm, pucSearchMessage); */

                    /* Populate the search type if it was just allocated */
                    if ( prstRepSearchType->uiType == REP_TYPE_UNKNOWN ) {
                        prstRepSearchType->uiType = REP_TYPE_MESSAGE;
                        if ( (prstRepSearchType->pucSearchMessage = s_strdup(pucSearchMessage)) == NULL ) {
                            iError = REP_MemError;
                            goto bailFromRepMerger;
                        }
                    }
                    /* Check that the new search type matches the one already there */
                    else if ( prstRepSearchType->uiType == REP_TYPE_MESSAGE ) {
                        
                        if ( s_strcmp(prstRepSearchType->pucSearchMessage, pucSearchMessage) != 0 ) {
                            iUtlLogWarn(UTL_LOG_CONTEXT, "Mismatched search message, expected: '%s', found: '%s'.", prstRepSearchType->pucSearchMessage, pucSearchMessage);
                        }
                    }
                    /* We are out of sync */
                    else {
                        iUtlLogWarn(UTL_LOG_CONTEXT, "Out of sync 1.");
                    }
                }
                else {
                    iUtlLogWarn(UTL_LOG_CONTEXT, "Failed to extract the search message from the search report.");
                    pucSearchSetting[0] = '\0';
                }
            }

            /* Process the search term */
            else if ( s_strncmp(pucSearchReportPtr, REP_SEARCH_TERM, s_strlen(REP_SEARCH_TERM)) == 0 ) {
                
                /* Extract the search term */
                if ( sscanf(pucSearchReportPtr + s_strlen(REP_SEARCH_TERM) + 1, pucSearchTermScanfFormat, pucSearchTerm, pucSearchField, pucSearchTermStemmed, &fTermWeight, &iTermCount, &iDocCount) == 6 ) {

                    /* Allocate the search types if it is not allocated and allocate a search type */
                    if ( pprstRepSearchType == NULL ) {

                        uiSearchTypeLen = 1;
                        iSearchTypeIndex = 0;

                        if ( (pprstRepSearchType = (struct repSearchType **)s_malloc((size_t)(sizeof(struct repSearchType *) * uiSearchTypeLen))) == NULL ) {
                            iError = REP_MemError;
                            goto bailFromRepMerger;
                        }
                        if ( (prstRepSearchType = (struct repSearchType *)s_malloc((size_t)sizeof(struct repSearchType))) == NULL ) {
                            iError = REP_MemError;
                            goto bailFromRepMerger;
                        }

                        prstRepSearchType->uiType = REP_TYPE_UNKNOWN;
                        pprstRepSearchType[iSearchTypeIndex] = prstRepSearchType;
                    }
                    /* Extend the search types and allocate a search type */
                    else if ( (uiSearchTypeLen - iSearchTypeIndex) == 1 ) {
                        
                        uiSearchTypeLen++;
                        iSearchTypeIndex++;

                        if ( (pprstRepSearchType = (struct repSearchType **)s_realloc(pprstRepSearchType, (size_t)(sizeof(struct repSearchType *) * uiSearchTypeLen))) == NULL ) {
                            iError = REP_MemError;
                            goto bailFromRepMerger;
                        }
                        if ( (prstRepSearchType = (struct repSearchType *)s_malloc((size_t)(sizeof(struct repSearchType)))) == NULL ) {
                            iError = REP_MemError;
                            goto bailFromRepMerger;
                        }

                        prstRepSearchType->uiType = REP_TYPE_UNKNOWN;
                        pprstRepSearchType[iSearchTypeIndex] = prstRepSearchType;
                    }
                    /* Dereference the search type, it is already there */
                    else {
                        iSearchTypeIndex++;
                        prstRepSearchType = pprstRepSearchType[iSearchTypeIndex];
                    }

/*                     iUtlLogDebug(UTL_LOG_CONTEXT, "[%s][%u][%u]", pucSearchTerm, iSearchTypeIndex, prstRepSearchType->uiType); */
                    
                    /* Populate the search type if it was just allocated */
                    if ( prstRepSearchType->uiType == REP_TYPE_UNKNOWN ) {
                        
                        prstRepSearchType->uiType = REP_TYPE_TERM;
                        
                        if ( (prstRepSearchType->pucSearchTerm = s_strdup(pucSearchTerm)) == NULL ) {
                            iError = REP_MemError;
                            goto bailFromRepMerger;
                        }
                        
                        if ( (prstRepSearchType->pucSearchField = s_strdup(pucSearchField)) == NULL ) {
                            iError = REP_MemError;
                            goto bailFromRepMerger;
                        }
                        
                        if ( (prstRepSearchType->pucSearchTermStemmed = s_strdup(pucSearchTermStemmed)) == NULL ) {
                            iError = REP_MemError;
                            goto bailFromRepMerger;
                        }
    
                        prstRepSearchType->fTermWeight = fTermWeight;
                        prstRepSearchType->iTermCount = iTermCount;
                        prstRepSearchType->iDocCount = iDocCount;
                    }
                    /* Check that the new search type matches the one already there, and update fields as needed */
                    else if ( prstRepSearchType->uiType == REP_TYPE_TERM ) {
                        
                        /* Check that the new search type matches the one already there */
                        if ( s_strcmp(prstRepSearchType->pucSearchTerm, pucSearchTerm) == 0 ) {
                            
                            /* Update the search field, promoting unfielded search to fielded if possible */
                            if ( (pucSearchField != NULL) && ((prstRepSearchType->pucSearchField == NULL) || (s_strcmp(prstRepSearchType->pucSearchField, REP_UNFIELDED_STRING) == 0)) ) {
                                
                                s_free(prstRepSearchType->pucSearchField);
                                
                                if ( (prstRepSearchType->pucSearchField = s_strdup(pucSearchField)) == NULL ) {
                                    iError = REP_MemError;
                                    goto bailFromRepMerger;
                                }
                            }
                            
                            /* Update the stemmed search term, promoting unstemmed search term to stemmed if possible */
                            if ( (pucSearchTermStemmed != NULL) && ((prstRepSearchType->pucSearchTermStemmed == NULL) || (s_strcmp(prstRepSearchType->pucSearchTermStemmed, REP_UNSTEMMED_STRING) == 0)) ) {
                                
                                s_free(prstRepSearchType->pucSearchTermStemmed);
                                
                                if ( (prstRepSearchType->pucSearchTermStemmed = s_strdup(pucSearchTermStemmed)) == NULL ) {
                                    iError = REP_MemError;
                                    goto bailFromRepMerger;
                                }
                            }
                            
                            /* Set the term weight used to the maximum available */
                            prstRepSearchType->fTermWeight = UTL_MACROS_MAX(prstRepSearchType->fTermWeight, fTermWeight);
                            
                            /* Increment the term count, promoting stop word and non-existent word to a real word if possible */
                            if ( (iTermCount != REP_TERM_STOP) && (iTermCount != REP_TERM_NON_EXISTENT) && (iTermCount != REP_TERM_FREQUENT) ) {
                                if ( (prstRepSearchType->iTermCount != REP_TERM_STOP) && (prstRepSearchType->iTermCount != REP_TERM_NON_EXISTENT) && (prstRepSearchType->iTermCount != REP_TERM_FREQUENT) ) {
                                    prstRepSearchType->iTermCount += iTermCount;
                                }
                                else {
                                    prstRepSearchType->iTermCount = iTermCount;
                                }
                            }

                            /* Increment the doc count, promoting stop word and non-existent word to a doc count if possible */
                            if ( (iDocCount != REP_TERM_STOP) && (iDocCount != REP_TERM_NON_EXISTENT) && (iDocCount != REP_TERM_FREQUENT) ) {
                                if ( (prstRepSearchType->iDocCount != REP_TERM_STOP) && (prstRepSearchType->iDocCount != REP_TERM_NON_EXISTENT) && (prstRepSearchType->iDocCount != REP_TERM_FREQUENT) ) {
                                    prstRepSearchType->iDocCount += iDocCount;
                                }
                                else {
                                    prstRepSearchType->iDocCount = iDocCount;
                                }
                            }
                        }
                        /* Mismatch */
                        else {
                            iUtlLogWarn(UTL_LOG_CONTEXT, "Mismatched search term, expected: '%s', found: '%s'.", prstRepSearchType->pucSearchTerm, pucSearchTerm);
                        }
                    }
                    /* We are out of sync */
                    else {
                        iUtlLogWarn(UTL_LOG_CONTEXT, "Out of sync 2.");
                    }
                }
                else {
                    iUtlLogWarn(UTL_LOG_CONTEXT, "Failed to extract the search term from the search report.");
                    pucSearchTerm[0] = '\0';
                    pucSearchField[0] = '\0';
                    pucSearchTermStemmed[0] = '\0';
                    fTermWeight = 0.0;
                    iTermCount = 0;
                    iDocCount = 0;
                }
            }

            /* Process the search term expanded */
            else if ( s_strncmp(pucSearchReportPtr, REP_SEARCH_TERM_EXPANDED, s_strlen(REP_SEARCH_TERM_EXPANDED)) == 0 ) {
    
                /* Extract the search term expanded */
                if ( sscanf(pucSearchReportPtr + s_strlen(REP_SEARCH_TERM_EXPANDED) + 1, pucSearchTermExpandedScanfFormat, pucSearchTerm, pucSearchField, &fTermWeight) == 3 ) {
                
                    /* Allocate the search types if it is not allocated and allocate a search type */
                    if ( pprstRepSearchType == NULL ) {
                        
                        uiSearchTypeLen = 1;
                        iSearchTypeIndex = 0;

                        if ( (pprstRepSearchType = (struct repSearchType **)s_malloc((size_t)(sizeof(struct repSearchType *) * uiSearchTypeLen))) == NULL ) {
                            iError = REP_MemError;
                            goto bailFromRepMerger;
                        }
                        if ( (prstRepSearchType = (struct repSearchType *)s_malloc((size_t)sizeof(struct repSearchType))) == NULL ) {
                            iError = REP_MemError;
                            goto bailFromRepMerger;
                        }
                        
                        prstRepSearchType->uiType = REP_TYPE_UNKNOWN;
                        pprstRepSearchType[iSearchTypeIndex] = prstRepSearchType;
                    }
                    /* Extend the search types and allocate a search type */
                    else if ( (uiSearchTypeLen - iSearchTypeIndex) == 1 ) {
                        
                        uiSearchTypeLen++;
                        iSearchTypeIndex++;

                        if ( (pprstRepSearchType = (struct repSearchType **)s_realloc(pprstRepSearchType, (size_t)(sizeof(struct repSearchType *) * uiSearchTypeLen))) == NULL ) {
                            iError = REP_MemError;
                            goto bailFromRepMerger;
                        }
                        if ( (prstRepSearchType = (struct repSearchType *)s_malloc((size_t)sizeof(struct repSearchType))) == NULL ) {
                            iError = REP_MemError;
                            goto bailFromRepMerger;
                        }
                        
                        prstRepSearchType->uiType = REP_TYPE_UNKNOWN;
                        pprstRepSearchType[iSearchTypeIndex] = prstRepSearchType;
                    }
                    /* Dereference the search type, it is already there */
                    else {
                        iSearchTypeIndex++;
                        prstRepSearchType = pprstRepSearchType[iSearchTypeIndex];
                    }
                    
/*                     iUtlLogDebug(UTL_LOG_CONTEXT, "[%s][%d][%d]", pucSearchTerm, iSearchTypeIndex, prstRepSearchType->uiType); */

                    /* Populate the search type if it was just allocated */
                    if ( prstRepSearchType->uiType == REP_TYPE_UNKNOWN ) {
                        
                        prstRepSearchType->uiType = REP_TYPE_TERM_EXPANDED;
                        
                        if ( (prstRepSearchType->pucSearchTerm = s_strdup(pucSearchTerm)) == NULL ) {
                            iError = REP_MemError;
                            goto bailFromRepMerger;
                        }
                        
                        if ( (prstRepSearchType->pucSearchField = s_strdup(pucSearchField)) == NULL ) {
                            iError = REP_MemError;
                            goto bailFromRepMerger;
                        }

                        prstRepSearchType->fTermWeight = fTermWeight;

                        if ( (iError = iUtlTrieCreate(&prstRepSearchType->pvUtlExpandedTermTrie)) != UTL_NoError ) {
                            iUtlLogError(UTL_LOG_CONTEXT, "Failed to create a trie for the expanded search terms, utl error: %d.", iError);
                            iError = REP_MiscError;
                            goto bailFromRepMerger;
                        }
                    }
                    /* Check that the new search type matches the one already there, and update fields as needed */
                    else if ( prstRepSearchType->uiType == REP_TYPE_TERM_EXPANDED ) {
                        
                        /* Mismatch */
                        if ( s_strcmp(prstRepSearchType->pucSearchTerm, pucSearchTerm) != 0 ) {
                            iUtlLogWarn(UTL_LOG_CONTEXT, "Mismatched search term, expected: '%s', found: '%s'.", prstRepSearchType->pucSearchTerm, pucSearchTerm);
                        }
                    }
                    /* We are out of sync */
                    else {
                        iUtlLogWarn(UTL_LOG_CONTEXT, "Out of sync 3.");
                    }

                    
                    /* Add the rest of the information */
                    if ( (prstRepSearchType->uiType == REP_TYPE_TERM_EXPANDED) && (s_strcmp(prstRepSearchType->pucSearchTerm, pucSearchTerm) == 0) ) {
                        
                        unsigned char   *pucSearchReportEndPtr = NULL;
                        unsigned char   *pucPtr = NULL;

                        /* Update the search field, promoting unfielded search to fielded if possible */
                        if ( (pucSearchField != NULL) && ((prstRepSearchType->pucSearchField == NULL) || (s_strcmp(prstRepSearchType->pucSearchField, REP_UNFIELDED_STRING) == 0)) ) {
                            
                            s_free(prstRepSearchType->pucSearchField);
                            
                            if ( (prstRepSearchType->pucSearchField = s_strdup(pucSearchField)) == NULL ) {
                                iError = REP_MemError;
                                goto bailFromRepMerger;
                            }
                        }

                        /* Set the term weight used to the maximum available */
                        prstRepSearchType->fTermWeight = UTL_MACROS_MAX(prstRepSearchType->fTermWeight, fTermWeight);


                        /* Get the end of the line */
                        if ( (pucSearchReportEndPtr = s_strchr(pucSearchReportPtr, '\n')) == NULL ) {
                            pucSearchReportEndPtr = s_strchr(pucSearchReportPtr, '\0');
                        }
                    
                        /* Get a pointer to the next entry */
                        if ( (pucPtr = s_strchr(pucSearchReportPtr, '\t')) != NULL ) {
                            pucPtr++;
                        }
                        
                        /* Loop while there are entries to process */
                        while ( (pucPtr != NULL) && (pucPtr < pucSearchReportEndPtr) ) {

                            /* Extract the term, term count and doc count */
                            if ( sscanf(pucPtr, pucSearchTermExpandedTermScanfFormat, pucSearchTerm, &iTermCount, &iDocCount) == 3 ) {
                                
                                void                        **ppvTrieTermEntry = NULL;
                                struct repTrieTermEntry     *prtteRepTrieTermEntry = NULL;

                                /* Look up/store the term */
                                if ( (iError = iUtlTrieAdd(prstRepSearchType->pvUtlExpandedTermTrie, pucSearchTerm, &ppvTrieTermEntry)) != UTL_NoError ) {
                                    iUtlLogError(UTL_LOG_CONTEXT, "Failed to add an entry to the expanded search terms trie, expanded search term: '%s', utl error: %d.", pucSearchTerm, iError);
                                    iError = REP_MiscError;
                                    goto bailFromRepMerger;
                                }
                            
                                /* Dereference the pointer to the pointer and check if is it allocated, allocate it if needed */
                                if ( (prtteRepTrieTermEntry = (struct repTrieTermEntry *)*ppvTrieTermEntry) == NULL ) {

                                    if ( (*ppvTrieTermEntry = (struct repTrieTermEntry *)s_malloc((size_t)sizeof(struct repTrieTermEntry))) == NULL ) {
                                        iError = REP_MemError;
                                        goto bailFromRepMerger;
                                    }

                                    prtteRepTrieTermEntry = (struct repTrieTermEntry *)*ppvTrieTermEntry;
                                    prtteRepTrieTermEntry->iTermCount = iTermCount;
                                    prtteRepTrieTermEntry->iDocCount = iDocCount;
                                }
                                else {

                                    /* Increment the term count, promoting stop word and non-existent word to a real word if possible */
                                    if ( (iTermCount != REP_TERM_STOP) && (iTermCount != REP_TERM_NON_EXISTENT) && (iTermCount != REP_TERM_FREQUENT) ) {
                                        if ( (prtteRepTrieTermEntry->iTermCount != REP_TERM_STOP) && (prtteRepTrieTermEntry->iTermCount != REP_TERM_NON_EXISTENT) && (prtteRepTrieTermEntry->iTermCount != REP_TERM_FREQUENT) ) {
                                            prtteRepTrieTermEntry->iTermCount += iTermCount;
                                        }
                                        else {
                                            prtteRepTrieTermEntry->iTermCount = iTermCount;
                                        }
                                    }
        
                                    /* Increment the doc count, promoting stop word and non-existent word to a doc count if possible */
                                    if ( (iDocCount != REP_TERM_STOP) && (iDocCount != REP_TERM_NON_EXISTENT) && (iDocCount != REP_TERM_FREQUENT) ) {
                                        if ( (prtteRepTrieTermEntry->iDocCount != REP_TERM_STOP) && (prtteRepTrieTermEntry->iDocCount != REP_TERM_NON_EXISTENT) && (prtteRepTrieTermEntry->iDocCount != REP_TERM_FREQUENT) ) {
                                            prtteRepTrieTermEntry->iDocCount += iDocCount;
                                        }
                                        else {
                                            prtteRepTrieTermEntry->iDocCount = iDocCount;
                                        }
                                    }
                                }
                            }
                            /* Extract the final term count and doc count */
                            else if ( sscanf(pucPtr, pucSearchTermExpandedCountsScanfFormat, &iTermCount, &iDocCount) == 2 ) {

                                /* Increment the term count, promoting stop word and non-existent word to a real word if possible */
                                if ( (iTermCount != REP_TERM_STOP) && (iTermCount != REP_TERM_NON_EXISTENT) && (iTermCount != REP_TERM_FREQUENT) ) {
                                    if ( (prstRepSearchType->iTermCount != REP_TERM_STOP) && (prstRepSearchType->iTermCount != REP_TERM_NON_EXISTENT) && (prstRepSearchType->iTermCount != REP_TERM_FREQUENT) ) {
                                        prstRepSearchType->iTermCount += iTermCount;
                                    }
                                    else {
                                        prstRepSearchType->iTermCount = iTermCount;
                                    }
                                }
    
                                /* Increment the doc count, promoting stop word and non-existent word to a doc count if possible */
                                if ( (iDocCount != REP_TERM_STOP) && (iDocCount != REP_TERM_NON_EXISTENT) && (iDocCount != REP_TERM_FREQUENT) ) {
                                    if ( (prstRepSearchType->iDocCount != REP_TERM_STOP) && (prstRepSearchType->iDocCount != REP_TERM_NON_EXISTENT) && (prstRepSearchType->iDocCount != REP_TERM_FREQUENT) ) {
                                        prstRepSearchType->iDocCount += iDocCount;
                                    }
                                    else {
                                        prstRepSearchType->iDocCount = iDocCount;
                                    }
                                }
                            }
                            else {
                                iUtlLogWarn(UTL_LOG_CONTEXT, "Failed to extract the search term expanded from the search report (pre).");
                                pucSearchTerm[0] = '\0';
                                iTermCount = 0;
                                iDocCount = 0;
                            }
        
                            /* Get a pointer to the next entry */
                            if ( (pucPtr = s_strchr(pucPtr, '\t')) != NULL ) {
                                pucPtr++;
                            }
                        }
                    }
                }
                else {
                    iUtlLogWarn(UTL_LOG_CONTEXT, "Failed to extract the search term expanded from the search report (post).");
                    pucSearchTerm[0] = '\0';
                    pucSearchField[0] = '\0';
                    fTermWeight = 0.0;
                }
            }


            /* Process the positive feedback counts */
            else if ( s_strncmp(pucSearchReportPtr, REP_POSITIVE_FEEDBACK_COUNTS, s_strlen(REP_POSITIVE_FEEDBACK_COUNTS)) == 0 ) {
                
                unsigned int    uiTempPositiveFeedbackTotalTermCount = 0;
                unsigned int    uiTempPositiveFeedbackUniqueTermCount = 0;
                unsigned int    uiTempPositiveFeedbackUsedTermCount = 0;

                /* Extract the positive feedback counts, incrementing them */
                if ( sscanf(pucSearchReportPtr + s_strlen(REP_POSITIVE_FEEDBACK_COUNTS) + 1, pucPositiveFeedbackCountsScanfFormat,
                        &uiTempPositiveFeedbackTotalTermCount, &uiTempPositiveFeedbackUniqueTermCount, &uiTempPositiveFeedbackUsedTermCount) == 3 ) {
    
                    uiPositiveFeedbackTotalTermCount = UTL_MACROS_MAX(uiTempPositiveFeedbackTotalTermCount, uiPositiveFeedbackTotalTermCount);
                    uiPositiveFeedbackUniqueTermCount = UTL_MACROS_MAX(uiTempPositiveFeedbackUniqueTermCount, uiPositiveFeedbackUniqueTermCount);
                    uiPositiveFeedbackUsedTermCount = UTL_MACROS_MAX(uiTempPositiveFeedbackUsedTermCount, uiPositiveFeedbackUsedTermCount);
                }
                else {
                    iUtlLogWarn(UTL_LOG_CONTEXT, "Failed to extract the positive feedback counts from the search report.");
                }
            }
    
            /* Process the positive feedback term */
            else if ( s_strncmp(pucSearchReportPtr, REP_POSITIVE_FEEDBACK_TERMS, s_strlen(REP_POSITIVE_FEEDBACK_TERMS)) == 0 ) {
                
                unsigned char   *pucSearchReportEndPtr = NULL;
                unsigned char   *pucPtr = NULL;
                
                /* Get the end of the line */
                if ( (pucSearchReportEndPtr = s_strchr(pucSearchReportPtr, '\n')) == NULL ) {
                    pucSearchReportEndPtr = s_strchr(pucSearchReportPtr, '\0');
                }
            
                /* Loop while there are terms to process */
                for ( pucPtr = pucSearchReportPtr + s_strlen(REP_POSITIVE_FEEDBACK_TERMS) + 1; (pucPtr != NULL) && (pucPtr < pucSearchReportEndPtr); ) {
                
                    /* Extract the positive feedback terms */
                    if ( sscanf(pucPtr, pucPositiveFeedbackTermsScanfFormat, pucPositiveFeedbackTerm) == 1 ) {
                    
                        void    **ppvDatum = NULL;
    
                        /* Create the positive feedback terms hash if it does not exist */
                        if ( pvUtlPositiveFeedbackTermsHash == NULL ) {
                            if ( (iError = iUtlHashCreate(&pvUtlPositiveFeedbackTermsHash)) != UTL_NoError ) {
                                iUtlLogError(UTL_LOG_CONTEXT, "Failed to create the positive feedback terms hash, utl error: %d.", iError);
                                goto bailFromRepMerger;
                            }
                        }
    
                        /* Add the positive feedback term to the positive feedback terms */
                        if ( (iError = iUtlHashAdd(pvUtlPositiveFeedbackTermsHash, pucPositiveFeedbackTerm, &ppvDatum)) != UTL_NoError ) {
                            iUtlLogError(UTL_LOG_CONTEXT, "Failed to add a symbol to the positive feedback terms hash, symbol: '%s', utl error: %d.", pucPositiveFeedbackTerm, iError);
                            goto bailFromRepMerger;
                        }
                    }
                    else {
                        iUtlLogWarn(UTL_LOG_CONTEXT, "Failed to extract the positive feedback terms from the search report.");
                        pucPositiveFeedbackTerm[0] = '\0';
                    }
            
                    /* Get a pointer to the next entry */
                    if ( (pucPtr = s_strchr(pucPtr, ' ')) != NULL ) {
                        pucPtr++;
                    }
                }
            }

            /* Process the negative feedback counts */
            else if ( s_strncmp(pucSearchReportPtr, REP_NEGATIVE_FEEDBACK_COUNTS, s_strlen(REP_NEGATIVE_FEEDBACK_COUNTS)) == 0 ) {
                
                unsigned int    uiTempNegativeFeedbackTotalTermCount = 0;
                unsigned int    uiTempNegativeFeedbackUniqueTermCount = 0;
                unsigned int    uiTempNegativeFeedbackUsedTermCount = 0;

                /* Extract the negative feedback counts, incrementing them */
                if ( sscanf(pucSearchReportPtr + s_strlen(REP_NEGATIVE_FEEDBACK_COUNTS) + 1, pucNegativeFeedbackCountsScanfFormat, 
                        &uiTempNegativeFeedbackTotalTermCount, &uiTempNegativeFeedbackUniqueTermCount) == 3 ) {
                    
                    uiNegativeFeedbackTotalTermCount = UTL_MACROS_MAX(uiTempNegativeFeedbackTotalTermCount, uiNegativeFeedbackTotalTermCount);
                    uiNegativeFeedbackUniqueTermCount = UTL_MACROS_MAX(uiTempNegativeFeedbackUniqueTermCount, uiNegativeFeedbackUniqueTermCount);
                    uiNegativeFeedbackUsedTermCount = UTL_MACROS_MAX(uiTempNegativeFeedbackUsedTermCount, uiNegativeFeedbackUsedTermCount);
                }
                else {
                    iUtlLogWarn(UTL_LOG_CONTEXT, "Failed to extract the negative feedback counts from the search report.");
                }
            }
    
            /* Process the negative feedback term */
            else if ( s_strncmp(pucSearchReportPtr, REP_NEGATIVE_FEEDBACK_TERMS, s_strlen(REP_NEGATIVE_FEEDBACK_TERMS)) == 0 ) {
                
                unsigned char   *pucSearchReportEndPtr = NULL;
                unsigned char   *pucPtr = NULL;
                
                /* Get the end of the line */
                if ( (pucSearchReportEndPtr = s_strchr(pucSearchReportPtr, '\n')) == NULL ) {
                    pucSearchReportEndPtr = s_strchr(pucSearchReportPtr, '\0');
                }
            
                /* Loop while there are terms to process */
                for ( pucPtr = pucSearchReportPtr + s_strlen(REP_POSITIVE_FEEDBACK_TERMS) + 1; (pucPtr != NULL) && (pucPtr < pucSearchReportEndPtr); ) {
                
                    /* Extract the negative feedback terms */
                    if ( sscanf(pucPtr, pucNegativeFeedbackTermsScanfFormat, pucNegativeFeedbackTerm) == 1 ) {
                    
                        void    **ppvDatum = NULL;
    
                        /* Create the negative feedback terms hash if it does not exist */
                        if ( pvUtlNegativeFeedbackTermsHash == NULL ) {
                            if ( (iError = iUtlHashCreate(&pvUtlNegativeFeedbackTermsHash)) != UTL_NoError ) {
                                iUtlLogError(UTL_LOG_CONTEXT, "Failed to create the negative feedback terms hash, utl error: %d.", iError);
                                goto bailFromRepMerger;
                            }
                        }
    
                        /* Add the negative feedback term to the negative feedback terms */
                        if ( (iError = iUtlHashAdd(pvUtlNegativeFeedbackTermsHash, pucNegativeFeedbackTerm, &ppvDatum)) != UTL_NoError ) {
                            iUtlLogError(UTL_LOG_CONTEXT, "Failed to add a symbol to the negative feedback terms hash, symbol: '%s', utl error: %d.", pucNegativeFeedbackTerm, iError);
                            goto bailFromRepMerger;
                        }
                    }
                    else {
                        iUtlLogWarn(UTL_LOG_CONTEXT, "Failed to extract the negative feedback terms from the search report.");
                        pucNegativeFeedbackTerm[0] = '\0';
                    }
            
                    /* Get a pointer to the next entry */
                    if ( (pucPtr = s_strchr(pucPtr, ' ')) != NULL ) {
                        pucPtr++;
                    }
                }
            }

            /* Process the documents retrieved */
            else if ( s_strncmp(pucSearchReportPtr, REP_RETRIEVAL_COUNTS, s_strlen(REP_RETRIEVAL_COUNTS)) == 0 ) {
                
                unsigned int    uiDocumentsRetrievedCountTemp = 0;
                unsigned int    uiFailedDateMatchCountTemp = 0;
                unsigned int    uiFailedTermCountMatchCountTemp = 0;
                unsigned int    uiFailedLanguageMatchCountTemp = 0;
                unsigned int    uiExcludedDocumentCountTemp = 0;
                unsigned int    uiIncludedDocumentCountTemp = 0;
                unsigned int    uiAccurateTemp = 0;

                /* Extract the documents retrieved, incrementing them */
                if ( sscanf(pucSearchReportPtr + s_strlen(REP_RETRIEVAL_COUNTS) + 1, pucRetrievalCountsScanfFormat, &uiDocumentsRetrievedCountTemp, &uiFailedDateMatchCountTemp, 
                        &uiFailedTermCountMatchCountTemp, &uiFailedLanguageMatchCountTemp, &uiExcludedDocumentCountTemp, &uiIncludedDocumentCountTemp, &uiAccurateTemp) == 7 ) {

                    /* Increment the counts */
                    uiDocumentsRetrievedCount += uiDocumentsRetrievedCountTemp;
                    uiFailedDateMatchCount += uiFailedDateMatchCountTemp;
                    uiFailedTermCountMatchCount += uiFailedTermCountMatchCountTemp;
                    uiFailedLanguageMatchCount += uiFailedLanguageMatchCountTemp;
                    uiExcludedDocumentCount += uiExcludedDocumentCountTemp;
                    uiIncludedDocumentCount += uiIncludedDocumentCountTemp;
                    
                    /* Set the accuracy flag, once false always false */
                    bAccurate = (bAccurate == false) ? false : ((uiAccurateTemp == 1) ? true : false);
                }
                else {
                    iUtlLogWarn(UTL_LOG_CONTEXT, "Failed to extract the documents retrieved from the search report.");
                }
            }
    
/*             else { */
/*                 iUtlLogDebug(UTL_LOG_CONTEXT, "Unknown tag in search report: '%s'.", pucSearchReportPtr); */
/*             } */
    
    
            /* Get a pointer to the next line */
            if ( (pucSearchReportPtr = s_strchr(pucSearchReportPtr, '\n')) != NULL ) {
                pucSearchReportPtr++;
            }
    
        }
    }



    /* Generate the merged search report */

    /* Allocate the merged search report string buffer */
    if ( (iError = iUtlStringBufferCreate(&pvUtlSearchReportMergedStringBuffer)) != UTL_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to create the merged search report string buffer, utl error: %d.", iError);
        goto bailFromRepMerger;
    }

    
    /* Add the index name */
    if ( bUtlStringsIsStringNULL(pucIndexName) == false ) {
        snprintf(pucSearchReportLine, REP_LINE_MAXIMUM_LENGTH + 1, "%s %s\n", REP_INDEX_NAME, pucIndexName);
        iUtlStringBufferAppend(pvUtlSearchReportMergedStringBuffer, pucSearchReportLine);    
    }


    /* Add the index counts */
    if ( bGotIndexCounts == true ) {
        snprintf(pucSearchReportLine, REP_LINE_MAXIMUM_LENGTH + 1, "%s %lu %lu %lu %lu %u\n", REP_INDEX_COUNTS, ulTotalTermCount, ulUniqueTermCount,
                ulTotalStopTermCount, ulUniqueStopTermCount, uiDocumentCount);
        iUtlStringBufferAppend(pvUtlSearchReportMergedStringBuffer, pucSearchReportLine);    
    }


    /* Add the stemmer name */
    if ( bUtlStringsIsStringNULL(pucStemmerName) == false ) {
        snprintf(pucSearchReportLine, REP_LINE_MAXIMUM_LENGTH + 1, "%s %s\n", REP_STEMMER_NAME, pucStemmerName);
        iUtlStringBufferAppend(pvUtlSearchReportMergedStringBuffer, pucSearchReportLine);    
    }


    /* Add the search original */
    if ( bUtlStringsIsStringNULL(pucSearchOriginal) == false ) {
        snprintf(pucSearchReportLine, REP_LINE_MAXIMUM_LENGTH + 1, "%s %s\n", REP_SEARCH_ORIGINAL, pucSearchOriginal);
        iUtlStringBufferAppend(pvUtlSearchReportMergedStringBuffer, pucSearchReportLine);    
    }


    /* Add the search reformatted */
    if ( bUtlStringsIsStringNULL(pucSearchReformatted) == false ) {
        snprintf(pucSearchReportLine, REP_LINE_MAXIMUM_LENGTH + 1, "%s %s\n", REP_SEARCH_REFORMATTED, pucSearchReformatted);
        iUtlStringBufferAppend(pvUtlSearchReportMergedStringBuffer, pucSearchReportLine);    
    }


    /* Add the search settings */
    if ( pvUtlSearchSettingsHash != NULL ) {
        if ( (iError = iUtlHashLoopOverKeys(pvUtlSearchSettingsHash, (int (*)())iRepHashPrintCallBackFunction, REP_SEARCH_SETTING, "\n", pvUtlSearchReportMergedStringBuffer)) != UTL_NoError ) {
            iUtlLogWarn(UTL_LOG_CONTEXT, "Failed to loop over the search settings hash, utl error: %d.", iError);
        }
    }
    
    
    /* Add the search errors */
    if ( pvUtlSearchErrorsHash != NULL ) {
        if ( (iError = iUtlHashLoopOverKeys(pvUtlSearchErrorsHash, (int (*)())iRepHashPrintCallBackFunction, REP_SEARCH_ERROR, "\n", pvUtlSearchReportMergedStringBuffer)) != UTL_NoError ) {
            iUtlLogWarn(UTL_LOG_CONTEXT, "Failed to loop over the search errors hash, utl error: %d.", iError);
        }
    }


    /* Add the search restrictions */
    if ( pvUtlSearchRestrictionsHash != NULL ) {
        if ( (iError = iUtlHashLoopOverKeys(pvUtlSearchRestrictionsHash, (int (*)())iRepHashPrintCallBackFunction, REP_SEARCH_RESTRICTION, "\n", pvUtlSearchReportMergedStringBuffer)) != UTL_NoError ) {
            iUtlLogWarn(UTL_LOG_CONTEXT, "Failed to loop over the search restrictions hash, utl error: %d.", iError);
        }
    }


    /* Add the search debugs */
    if ( pvUtlSearchDebugsHash != NULL ) {
        if ( (iError = iUtlHashLoopOverKeys(pvUtlSearchDebugsHash, (int (*)())iRepHashPrintCallBackFunction, REP_SEARCH_DEBUG, "\n", pvUtlSearchReportMergedStringBuffer)) != UTL_NoError ) {
            iUtlLogWarn(UTL_LOG_CONTEXT, "Failed to loop over the search debugs hash, utl error: %d.", iError);
        }
    }


    /* Add the search warnings */
    if ( pvUtlSearchWarningsHash != NULL ) {
        if ( (iError = iUtlHashLoopOverKeys(pvUtlSearchWarningsHash, (int (*)())iRepHashPrintCallBackFunction, REP_SEARCH_WARNING, "\n", pvUtlSearchReportMergedStringBuffer)) != UTL_NoError ) {
            iUtlLogWarn(UTL_LOG_CONTEXT, "Failed to loop over the search warnings hash, utl error: %d.", iError);
        }
    }
    

    
    /* Add the search message, search term and search term expanded */
    if ( pprstRepSearchType != NULL ) {
            
        /* Loop over the search types */
        for ( uiI = 0; uiI < uiSearchTypeLen; uiI++ ) {
        
            /* Dereference the search type for convenience */
            prstRepSearchType = pprstRepSearchType[uiI];
            
            /* Process the search message */
            if ( prstRepSearchType->uiType == REP_TYPE_MESSAGE ) {
                snprintf(pucSearchReportLine, REP_LINE_MAXIMUM_LENGTH + 1, "%s %s\n", REP_SEARCH_MESSAGE, prstRepSearchType->pucSearchMessage);
                iUtlStringBufferAppend(pvUtlSearchReportMergedStringBuffer, pucSearchReportLine);
            }
            /* Process the search term */
            else if ( prstRepSearchType->uiType == REP_TYPE_TERM ) {
                snprintf(pucSearchReportLine, REP_LINE_MAXIMUM_LENGTH + 1, "%s %s %s %s %.2f %d %d\n", REP_SEARCH_TERM, prstRepSearchType->pucSearchTerm, 
                        prstRepSearchType->pucSearchField, prstRepSearchType->pucSearchTermStemmed, prstRepSearchType->fTermWeight, prstRepSearchType->iTermCount, prstRepSearchType->iDocCount);
                iUtlStringBufferAppend(pvUtlSearchReportMergedStringBuffer, pucSearchReportLine);
            }
            /* Process the search term expanded */
            else if ( prstRepSearchType->uiType == REP_TYPE_TERM_EXPANDED ) {
        
                snprintf(pucSearchReportLine, REP_LINE_MAXIMUM_LENGTH + 1, "%s %s %s %.2f", REP_SEARCH_TERM_EXPANDED, prstRepSearchType->pucSearchTerm, 
                        prstRepSearchType->pucSearchField, prstRepSearchType->fTermWeight);
                iUtlStringBufferAppend(pvUtlSearchReportMergedStringBuffer, pucSearchReportLine);

                /* Process the trie */
                if ( prstRepSearchType->pvUtlExpandedTermTrie != NULL ) {
                    if ( (iError = iUtlTrieLoop(prstRepSearchType->pvUtlExpandedTermTrie, NULL, (int (*)())iRepTriePrintCallBackFunction, pvUtlSearchReportMergedStringBuffer)) != UTL_NoError ) {
                        iUtlLogError(UTL_LOG_CONTEXT, "Failed to loop over the entries for the expanded search terms trie, utl error: %d.", iError);
                        iError = REP_MiscError;
                        goto bailFromRepMerger;
                    }
                }

                snprintf(pucSearchReportLine, REP_LINE_MAXIMUM_LENGTH + 1, "\t%d %d\n", prstRepSearchType->iTermCount, prstRepSearchType->iDocCount);
                iUtlStringBufferAppend(pvUtlSearchReportMergedStringBuffer, pucSearchReportLine);
            }
        }
    }
    
    
    /* Add the positive feedback counts */
    if ( uiPositiveFeedbackTotalTermCount > 0 ) {
        snprintf(pucSearchReportLine, REP_LINE_MAXIMUM_LENGTH + 1, "%s %u %u %u\n", REP_POSITIVE_FEEDBACK_COUNTS, uiPositiveFeedbackTotalTermCount, uiPositiveFeedbackUniqueTermCount, uiPositiveFeedbackUsedTermCount);
        iUtlStringBufferAppend(pvUtlSearchReportMergedStringBuffer, pucSearchReportLine);    
    }


    /* Add the positive feedback terms */
    if ( pvUtlPositiveFeedbackTermsHash != NULL ) {
        iUtlStringBufferAppend(pvUtlSearchReportMergedStringBuffer, REP_POSITIVE_FEEDBACK_TERMS);    
        iError = iUtlHashLoopOverKeys(pvUtlPositiveFeedbackTermsHash, (int (*)())iRepHashPrintCallBackFunction, "", NULL, pvUtlSearchReportMergedStringBuffer);
        iUtlStringBufferAppend(pvUtlSearchReportMergedStringBuffer, "\n");    
    }
    
    
    /* Add the negative feedback counts */
    if ( uiNegativeFeedbackTotalTermCount > 0 ) {
        snprintf(pucSearchReportLine, REP_LINE_MAXIMUM_LENGTH + 1, "%s %u %u %u\n", REP_NEGATIVE_FEEDBACK_COUNTS, uiNegativeFeedbackTotalTermCount, uiNegativeFeedbackUniqueTermCount, uiNegativeFeedbackUsedTermCount);
        iUtlStringBufferAppend(pvUtlSearchReportMergedStringBuffer, pucSearchReportLine);    
    }

    
    /* Add the negative feedback terms */
    if ( pvUtlNegativeFeedbackTermsHash != NULL ) {
        iUtlStringBufferAppend(pvUtlSearchReportMergedStringBuffer, REP_NEGATIVE_FEEDBACK_TERMS);    
        iError = iUtlHashLoopOverKeys(pvUtlNegativeFeedbackTermsHash, (int (*)())iRepHashPrintCallBackFunction, "", NULL, pvUtlSearchReportMergedStringBuffer);
        iUtlStringBufferAppend(pvUtlSearchReportMergedStringBuffer, "\n");    
    }
    
    
    /* Add the documents retrieved counts */
    snprintf(pucSearchReportLine, REP_LINE_MAXIMUM_LENGTH + 1, "%s %u %u %u %u %u %u %d\n", REP_RETRIEVAL_COUNTS, uiDocumentsRetrievedCount, uiFailedDateMatchCount, 
            uiFailedTermCountMatchCount, uiFailedLanguageMatchCount, uiExcludedDocumentCount, uiIncludedDocumentCount, ((bAccurate == false) ? 0 : 1));
    iUtlStringBufferAppend(pvUtlSearchReportMergedStringBuffer, pucSearchReportLine);    


    
    /* Bail label */
    bailFromRepMerger:
    
    
    /* Handle the error */
    if ( iError == UTL_NoError ) {

        /* Set the return pointer */
        iUtlStringBufferGetString(pvUtlSearchReportMergedStringBuffer, ppucSearchReportMerged);

        /* Free the merged search report string buffer, note that we dont free the string */
        iUtlStringBufferFree(pvUtlSearchReportMergedStringBuffer, false);
        pvUtlSearchReportMergedStringBuffer = NULL;

        /* Set the error */
        iError = REP_NoError;
    }
    else {
        
        /* Free the merged search report string buffer, note that we DO free the string */
        iUtlStringBufferFree(pvUtlSearchReportMergedStringBuffer, true);
        pvUtlSearchReportMergedStringBuffer = NULL;

        /* Set the error */
        iError = REP_MiscError;
    }


    /* Free the search types */
    if ( pprstRepSearchType != NULL ) {
            
        /* Loop over the search types */
        for ( uiI = 0; uiI < uiSearchTypeLen; uiI++ ) {
        
            /* Dereference the search type for convenience &*/
            prstRepSearchType = pprstRepSearchType[uiI];
            
            /* Free any allocated stuff */
            s_free(prstRepSearchType->pucSearchTerm);
            s_free(prstRepSearchType->pucSearchField);
            s_free(prstRepSearchType->pucSearchTermStemmed);
            s_free(prstRepSearchType->pucSearchMessage);

            iUtlTrieFree(prstRepSearchType->pvUtlExpandedTermTrie, true);
            prstRepSearchType->pvUtlExpandedTermTrie = NULL;

            s_free(prstRepSearchType);
        }
        
        /* Free the search types */
        s_free(pprstRepSearchType);
    }


    /* Free the hashes, don't free the datum as there is no datum to free */
    iUtlHashFree(pvUtlSearchErrorsHash, false);
    pvUtlSearchErrorsHash = NULL;

    iUtlHashFree(pvUtlSearchSettingsHash, false);
    pvUtlSearchSettingsHash = NULL;

    iUtlHashFree(pvUtlSearchRestrictionsHash, false);
    pvUtlSearchRestrictionsHash = NULL;

    iUtlHashFree(pvUtlSearchDebugsHash, false);
    pvUtlSearchDebugsHash = NULL;

    iUtlHashFree(pvUtlSearchWarningsHash, false);
    pvUtlSearchWarningsHash = NULL;

    iUtlHashFree(pvUtlPositiveFeedbackTermsHash, false);
    pvUtlPositiveFeedbackTermsHash = NULL;

    iUtlHashFree(pvUtlNegativeFeedbackTermsHash, false);
    pvUtlNegativeFeedbackTermsHash = NULL;


    return (iError);
    
}


/*---------------------------------------------------------------------------*/


/*

    Function:   iRepFormatSearchReports()

    Purpose:    This function takes a seach report and returns a formatted version of it

    Parameters: pucSearchReport             search report
                ppucSearchReportFormatted   return parameter for the formatted search report

    Globals:    none

    Returns:    REP error code

*/
int iRepFormatSearchReports
(
    unsigned char *pucSearchReport,
    unsigned char **ppucSearchReportFormatted
)
{

    int             iError = REP_NoError;

    unsigned char   *pucSearchReportPtr = NULL;
    void            *pvUtlSearchReportFormattedStringBuffer = NULL;

    unsigned char   pucNumberString[REP_LINE_MAXIMUM_LENGTH + 1] = {'\0'};
    unsigned char   pucSearchReportLine[REP_LINE_MAXIMUM_LENGTH + 1];
    unsigned char   *pucPtr = NULL;

    unsigned char   pucIndexName[REP_LINE_MAXIMUM_LENGTH + 1] = {'\0'};

    boolean         bGotIndexCounts = false;
    unsigned long   ulUniqueTermCount = 0;
    unsigned long   ulTotalTermCount = 0;
    unsigned long   ulUniqueStopTermCount = 0;
    unsigned long   ulTotalStopTermCount = 0;
    unsigned int    uiDocumentCount = 0;

    unsigned char   pucStemmerName[REP_LINE_MAXIMUM_LENGTH + 1] = {'\0'};

    unsigned char   pucSearchOriginal[REP_LINE_MAXIMUM_LENGTH + 1] = {'\0'};
    unsigned char   pucSearchReformatted[REP_LINE_MAXIMUM_LENGTH + 1] = {'\0'};

    unsigned char   pucSearchError[REP_LINE_MAXIMUM_LENGTH + 1] = {'\0'};
    void            *pvUtlSearchErrorsStringBuffer = NULL;

    unsigned char   pucSearchSetting[REP_LINE_MAXIMUM_LENGTH + 1] = {'\0'};
    void            *pvUtlSearchSettingsStringBuffer = NULL;

    unsigned char   pucSearchDebug[REP_LINE_MAXIMUM_LENGTH + 1] = {'\0'};
    void            *pvUtlSearchDebugsStringBuffer = NULL;

    unsigned char   pucSearchRestriction[REP_LINE_MAXIMUM_LENGTH + 1] = {'\0'};
    void            *pvUtlSearchRestrictionsStringBuffer = NULL;

    unsigned char   pucSearchWarning[REP_LINE_MAXIMUM_LENGTH + 1] = {'\0'};
    void            *pvUtlSearchWarningsStringBuffer = NULL;

    unsigned char   pucSearchMessage[REP_LINE_MAXIMUM_LENGTH + 1] = {'\0'};
    unsigned char   pucSearchTerm[REP_LINE_MAXIMUM_LENGTH + 1] = {'\0'};
    unsigned char   pucSearchField[REP_LINE_MAXIMUM_LENGTH + 1] = {'\0'};
    unsigned char   pucSearchTermStemmed[REP_LINE_MAXIMUM_LENGTH + 1] = {'\0'};
    float           fTermWeight = 0.0;
    int             iTermCount = 0;
    int             iDocCount = 0;
    void            *pvUtlSearchesStringBuffer = NULL;

    unsigned int    uiPositiveFeedbackTotalTermCount = 0;
    unsigned int    uiPositiveFeedbackUniqueTermCount = 0;
    unsigned int    uiPositiveFeedbackUsedTermCount = 0;
    unsigned char   pucPositiveFeedbackTerm[REP_LINE_MAXIMUM_LENGTH + 1] = {'\0'};
    void            *pvUtlPositiveFeedbackTermsStringBuffer = NULL;

    unsigned int    uiNegativeFeedbackTotalTermCount = 0;
    unsigned int    uiNegativeFeedbackUniqueTermCount = 0;
    unsigned int    uiNegativeFeedbackUsedTermCount = 0;
    unsigned char   pucNegativeFeedbackTerm[REP_LINE_MAXIMUM_LENGTH + 1] = {'\0'};
    void            *pvUtlNegativeFeedbackTermsStringBuffer = NULL;

    unsigned int    uiDocumentsRetrievedCount = 0;
    unsigned int    uiFailedDateMatchCount = 0;
    unsigned int    uiFailedTermCountMatchCount = 0;
    unsigned int    uiFailedLanguageMatchCount = 0;
    unsigned int    uiExcludedDocumentCount = 0;
    unsigned int    uiIncludedDocumentCount = 0;
    boolean         bAccurate = true;


    unsigned char   pucIndexNameScanfFormat[UTL_STRING_SCANF_FORMAT_LENGTH + 1] = {'\0'};
    unsigned char   pucIndexCountsScanfFormat[UTL_STRING_SCANF_FORMAT_LENGTH + 1] = {'\0'};
    unsigned char   pucStemmerNameScanfFormat[UTL_STRING_SCANF_FORMAT_LENGTH + 1] = {'\0'};
    unsigned char   pucSearchOriginalScanfFormat[UTL_STRING_SCANF_FORMAT_LENGTH + 1] = {'\0'};
    unsigned char   pucSearchReformattedScanfFormat[UTL_STRING_SCANF_FORMAT_LENGTH + 1] = {'\0'};
    unsigned char   pucSearchErrorScanfFormat[UTL_STRING_SCANF_FORMAT_LENGTH + 1] = {'\0'};
    unsigned char   pucSearchSettingScanfFormat[UTL_STRING_SCANF_FORMAT_LENGTH + 1] = {'\0'};
    unsigned char   pucSearchRestrictionScanfFormat[UTL_STRING_SCANF_FORMAT_LENGTH + 1] = {'\0'};
    unsigned char   pucSearchDebugScanfFormat[UTL_STRING_SCANF_FORMAT_LENGTH + 1] = {'\0'};
    unsigned char   pucSearchWarningScanfFormat[UTL_STRING_SCANF_FORMAT_LENGTH + 1] = {'\0'};
    unsigned char   pucSearchMessageScanfFormat[UTL_STRING_SCANF_FORMAT_LENGTH + 1] = {'\0'};
    unsigned char   pucSearchTermScanfFormat[UTL_STRING_SCANF_FORMAT_LENGTH + 1] = {'\0'};
    unsigned char   pucSearchTermExpandedScanfFormat[UTL_STRING_SCANF_FORMAT_LENGTH + 1] = {'\0'};
    unsigned char   pucSearchTermExpandedTermScanfFormat[UTL_STRING_SCANF_FORMAT_LENGTH + 1] = {'\0'};
    unsigned char   pucSearchTermExpandedCountsScanfFormat[UTL_STRING_SCANF_FORMAT_LENGTH + 1] = {'\0'};
    unsigned char   pucPositiveFeedbackCountsScanfFormat[UTL_STRING_SCANF_FORMAT_LENGTH + 1] = {'\0'};
    unsigned char   pucPositiveFeedbackTermsScanfFormat[UTL_STRING_SCANF_FORMAT_LENGTH + 1] = {'\0'};
    unsigned char   pucNegativeFeedbackCountsScanfFormat[UTL_STRING_SCANF_FORMAT_LENGTH + 1] = {'\0'};
    unsigned char   pucNegativeFeedbackTermsScanfFormat[UTL_STRING_SCANF_FORMAT_LENGTH + 1] = {'\0'};
    unsigned char   pucRetrievalCountsScanfFormat[UTL_STRING_SCANF_FORMAT_LENGTH + 1] = {'\0'};


    /* Check the parameters */
    if ( bUtlStringsIsStringNULL(pucSearchReport) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pucSearchReport' parameter passed to 'iRepFormatSearchReports'."); 
        return (REP_InvalidSearchReport);
    }

    if ( ppucSearchReportFormatted == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppucSearchReportFormatted' parameter passed to 'iRepFormatSearchReports'."); 
        return (REP_ReturnParameterError);
    }


    /* Create the scanf formats */
    snprintf(pucIndexNameScanfFormat, UTL_STRING_SCANF_FORMAT_LENGTH + 1, "%%%d[^\n]", REP_LINE_MAXIMUM_LENGTH);
    snprintf(pucIndexCountsScanfFormat, UTL_STRING_SCANF_FORMAT_LENGTH + 1, "%%lu %%lu %%lu %%lu %%u");
    snprintf(pucStemmerNameScanfFormat, UTL_STRING_SCANF_FORMAT_LENGTH + 1, "%%%d[^\n]", REP_LINE_MAXIMUM_LENGTH);
    snprintf(pucSearchOriginalScanfFormat, UTL_STRING_SCANF_FORMAT_LENGTH + 1, "%%%d[^\n]", REP_LINE_MAXIMUM_LENGTH);
    snprintf(pucSearchReformattedScanfFormat, UTL_STRING_SCANF_FORMAT_LENGTH + 1, "%%%d[^\n]", REP_LINE_MAXIMUM_LENGTH);
    snprintf(pucSearchErrorScanfFormat, UTL_STRING_SCANF_FORMAT_LENGTH + 1, "%%%d[^\n]", REP_LINE_MAXIMUM_LENGTH);
    snprintf(pucSearchSettingScanfFormat, UTL_STRING_SCANF_FORMAT_LENGTH + 1, "%%%d[^\n]", REP_LINE_MAXIMUM_LENGTH);
    snprintf(pucSearchDebugScanfFormat, UTL_STRING_SCANF_FORMAT_LENGTH + 1, "%%%d[^\n]", REP_LINE_MAXIMUM_LENGTH);
    snprintf(pucSearchRestrictionScanfFormat, UTL_STRING_SCANF_FORMAT_LENGTH + 1, "%%%d[^\n]", REP_LINE_MAXIMUM_LENGTH);
    snprintf(pucSearchWarningScanfFormat, UTL_STRING_SCANF_FORMAT_LENGTH + 1, "%%%d[^\n]", REP_LINE_MAXIMUM_LENGTH);
    snprintf(pucSearchMessageScanfFormat, UTL_STRING_SCANF_FORMAT_LENGTH + 1, "%%%d[^\n]", REP_LINE_MAXIMUM_LENGTH);
    snprintf(pucSearchTermScanfFormat, UTL_STRING_SCANF_FORMAT_LENGTH + 1, "%%%ds %%%ds %%%ds %%f %%d %%d", REP_LINE_MAXIMUM_LENGTH, REP_LINE_MAXIMUM_LENGTH, REP_LINE_MAXIMUM_LENGTH);
    snprintf(pucSearchTermExpandedScanfFormat, UTL_STRING_SCANF_FORMAT_LENGTH + 1, "%%%ds %%%ds %%f", REP_LINE_MAXIMUM_LENGTH, REP_LINE_MAXIMUM_LENGTH);
    snprintf(pucSearchTermExpandedTermScanfFormat, UTL_STRING_SCANF_FORMAT_LENGTH + 1, "%%%ds %%d %%d", REP_LINE_MAXIMUM_LENGTH);
    snprintf(pucSearchTermExpandedCountsScanfFormat, UTL_STRING_SCANF_FORMAT_LENGTH + 1, "%%d %%d");
    snprintf(pucPositiveFeedbackCountsScanfFormat, UTL_STRING_SCANF_FORMAT_LENGTH + 1, "%%u %%u %%u");
    snprintf(pucPositiveFeedbackTermsScanfFormat, UTL_STRING_SCANF_FORMAT_LENGTH + 1, "%%%ds", REP_LINE_MAXIMUM_LENGTH);
    snprintf(pucNegativeFeedbackCountsScanfFormat, UTL_STRING_SCANF_FORMAT_LENGTH + 1, "%%u %%u %%u");
    snprintf(pucNegativeFeedbackTermsScanfFormat, UTL_STRING_SCANF_FORMAT_LENGTH + 1, "%%%ds", REP_LINE_MAXIMUM_LENGTH);
    snprintf(pucRetrievalCountsScanfFormat, UTL_STRING_SCANF_FORMAT_LENGTH + 1, "%%u %%u %%u %%u %%u %%u %%u");



    /* Loop over each line in the search report */
    for ( pucSearchReportPtr = pucSearchReport; pucSearchReportPtr != NULL; ) {
        
        /* Process the index name */
        if ( s_strncmp(pucSearchReportPtr, REP_INDEX_NAME, s_strlen(REP_INDEX_NAME)) == 0 ) {
            
            /* Extract the index name */
            if ( sscanf(pucSearchReportPtr + s_strlen(REP_INDEX_NAME) + 1, pucIndexNameScanfFormat, pucIndexName) != 1 ) {
                iUtlLogWarn(UTL_LOG_CONTEXT, "Failed to extract the index name from the search report.");
                pucIndexName[0] = '\0';
            }
        }
        
        /* Process the index counts */
        else if ( s_strncmp(pucSearchReportPtr, REP_INDEX_COUNTS, s_strlen(REP_INDEX_COUNTS)) == 0 ) {
            
            /* Extract the index counts */
            if ( sscanf(pucSearchReportPtr + s_strlen(REP_INDEX_COUNTS) + 1, pucIndexCountsScanfFormat, &ulTotalTermCount, &ulUniqueTermCount, 
                    &ulTotalStopTermCount, &ulUniqueStopTermCount, &uiDocumentCount) != 5 ) {
                
                iUtlLogWarn(UTL_LOG_CONTEXT, "Failed to extract the index counts from the search report.");
                ulTotalTermCount = 0;
                ulUniqueTermCount = 0;
                ulTotalStopTermCount = 0;
                ulUniqueStopTermCount = 0;
                uiDocumentCount = 0;
            }

            /* Set the flag indicating that we got index counts */
            bGotIndexCounts = true;
        }

        /* Process the stemmer name */
        else if ( s_strncmp(pucSearchReportPtr, REP_STEMMER_NAME, s_strlen(REP_STEMMER_NAME)) == 0 ) {
            
            /* Extract the stemmer name */
            if ( sscanf(pucSearchReportPtr + s_strlen(REP_STEMMER_NAME) + 1, pucStemmerNameScanfFormat, pucStemmerName) != 1 ) {
                iUtlLogWarn(UTL_LOG_CONTEXT, "Failed to extract the stemmer name from the search report.");
                pucStemmerName[0] = '\0';
            }
        }
        
        /* Process the search original */
        else if ( s_strncmp(pucSearchReportPtr, REP_SEARCH_ORIGINAL, s_strlen(REP_SEARCH_ORIGINAL)) == 0 ) {
            
            /* Extract the search original */
            if ( sscanf(pucSearchReportPtr + s_strlen(REP_SEARCH_ORIGINAL) + 1, pucSearchOriginalScanfFormat, pucSearchOriginal) != 1 ) {
                iUtlLogWarn(UTL_LOG_CONTEXT, "Failed to extract the search original from the search report.");
                pucSearchOriginal[0] = '\0';
            }
        }
        
        /* Process the search reformatted */
        else if ( s_strncmp(pucSearchReportPtr, REP_SEARCH_REFORMATTED, s_strlen(REP_SEARCH_REFORMATTED)) == 0 ) {
            
            /* Extract the search reformatted */
            if ( sscanf(pucSearchReportPtr + s_strlen(REP_SEARCH_REFORMATTED) + 1, pucSearchReformattedScanfFormat, pucSearchReformatted) != 1 ) {
                iUtlLogWarn(UTL_LOG_CONTEXT, "Failed to extract the search reformatted from the search report.");
                pucSearchReformatted[0] = '\0';
            }
        }
        
        /* Process the search error */
        else if ( s_strncmp(pucSearchReportPtr, REP_SEARCH_ERROR, s_strlen(REP_SEARCH_ERROR)) == 0 ) {
            
            /* Extract the search error */
            if ( sscanf(pucSearchReportPtr + s_strlen(REP_SEARCH_ERROR) + 1, pucSearchErrorScanfFormat, pucSearchError) == 1 ) {
            
                /* Create the search errors string buffer if it is not does exist */
                if ( pvUtlSearchErrorsStringBuffer == NULL ) {
                    if ( (iError = iUtlStringBufferCreate(&pvUtlSearchErrorsStringBuffer)) != UTL_NoError ) {
                        iUtlLogError(UTL_LOG_CONTEXT, "Failed to create the search errors string buffer, utl error: %d.", iError);
                        iError = REP_MiscError;
                        goto bailFromiRepFormatter;
                    }
                }
                
                /* Add the string to the search errors string buffer */
                snprintf(pucSearchReportLine, REP_LINE_MAXIMUM_LENGTH + 1, " - %s.\n", pucSearchError);
                iUtlStringBufferAppend(pvUtlSearchErrorsStringBuffer, pucSearchReportLine);
            }
            else {
                iUtlLogWarn(UTL_LOG_CONTEXT, "Failed to extract the search error from the search report.");
                pucSearchError[0] = '\0';
            }
        }
        
        /* Process the search setting */
        else if ( s_strncmp(pucSearchReportPtr, REP_SEARCH_SETTING, s_strlen(REP_SEARCH_SETTING)) == 0 ) {
            
            /* Extract the search setting */
            if ( sscanf(pucSearchReportPtr + s_strlen(REP_SEARCH_SETTING) + 1, pucSearchSettingScanfFormat, pucSearchSetting) == 1 ) {
            
                /* Create the search settings string buffer if it is not does exist */
                if ( pvUtlSearchSettingsStringBuffer == NULL ) {
                    if ( (iError = iUtlStringBufferCreate(&pvUtlSearchSettingsStringBuffer)) != UTL_NoError ) {
                        iUtlLogError(UTL_LOG_CONTEXT, "Failed to create the search settings string buffer, utl error: %d.", iError);
                        iError = REP_MiscError;
                        goto bailFromiRepFormatter;
                    }
                }
                
                /* Add the string to the search settings string buffer */
                snprintf(pucSearchReportLine, REP_LINE_MAXIMUM_LENGTH + 1, " - %s.\n", pucSearchSetting);
                iUtlStringBufferAppend(pvUtlSearchSettingsStringBuffer, pucSearchReportLine);
            }
            else {
                iUtlLogWarn(UTL_LOG_CONTEXT, "Failed to extract the search setting from the search report.");
                pucSearchSetting[0] = '\0';
            }
        }

        /* Process the search restriction */
        else if ( s_strncmp(pucSearchReportPtr, REP_SEARCH_RESTRICTION, s_strlen(REP_SEARCH_RESTRICTION)) == 0 ) {
            
            /* Extract the search restriction */
            if ( sscanf(pucSearchReportPtr + s_strlen(REP_SEARCH_RESTRICTION) + 1, pucSearchRestrictionScanfFormat, pucSearchRestriction) == 1 ) {
            
                /* Create the search restrictions string buffer if it is not does exist */
                if ( pvUtlSearchRestrictionsStringBuffer == NULL ) {
                    if ( (iError = iUtlStringBufferCreate(&pvUtlSearchRestrictionsStringBuffer)) != UTL_NoError ) {
                        iUtlLogError(UTL_LOG_CONTEXT, "Failed to create the search restrictions string buffer, utl error: %d.", iError);
                        iError = REP_MiscError;
                        goto bailFromiRepFormatter;
                    }
                }
                
                /* Add the string to the search restrictions string buffer */
                snprintf(pucSearchReportLine, REP_LINE_MAXIMUM_LENGTH + 1, " - %s.\n", pucSearchRestriction);
                iUtlStringBufferAppend(pvUtlSearchRestrictionsStringBuffer, pucSearchReportLine);
            }
            else {
                iUtlLogWarn(UTL_LOG_CONTEXT, "Failed to extract the search restriction from the search report.");
                pucSearchRestriction[0] = '\0';
            }
        }

        /* Process the search debug */
        else if ( s_strncmp(pucSearchReportPtr, REP_SEARCH_DEBUG, s_strlen(REP_SEARCH_DEBUG)) == 0 ) {
            
            /* Extract the search debug */
            if ( sscanf(pucSearchReportPtr + s_strlen(REP_SEARCH_DEBUG) + 1, pucSearchDebugScanfFormat, pucSearchDebug) == 1 ) {
            
                /* Create the search debugs string buffer if it is not does exist */
                if ( pvUtlSearchDebugsStringBuffer == NULL ) {
                    if ( (iError = iUtlStringBufferCreate(&pvUtlSearchDebugsStringBuffer)) != UTL_NoError ) {
                        iUtlLogError(UTL_LOG_CONTEXT, "Failed to create the search debugs string buffer, utl error: %d.", iError);
                        iError = REP_MiscError;
                        goto bailFromiRepFormatter;
                    }
                }
                
                /* Add the string to the search debugs string buffer */
                snprintf(pucSearchReportLine, REP_LINE_MAXIMUM_LENGTH + 1, " - %s.\n", pucSearchDebug);
                iUtlStringBufferAppend(pvUtlSearchDebugsStringBuffer, pucSearchReportLine);
            }
            else {
                iUtlLogWarn(UTL_LOG_CONTEXT, "Failed to extract the search debug from the search report.");
                pucSearchDebug[0] = '\0';
            }
        }

        /* Process the search warning */
        else if ( s_strncmp(pucSearchReportPtr, REP_SEARCH_WARNING, s_strlen(REP_SEARCH_WARNING)) == 0 ) {
            
            /* Extract the search warning */
            if ( sscanf(pucSearchReportPtr + s_strlen(REP_SEARCH_WARNING) + 1, pucSearchWarningScanfFormat, pucSearchWarning) == 1 ) {
            
                /* Create the search warnings string buffer if it is not does exist */
                if ( pvUtlSearchWarningsStringBuffer == NULL ) {
                    if ( (iError = iUtlStringBufferCreate(&pvUtlSearchWarningsStringBuffer)) != UTL_NoError ) {
                        iUtlLogError(UTL_LOG_CONTEXT, "Failed to create the search warnings string buffer, utl error: %d.", iError);
                        iError = REP_MiscError;
                        goto bailFromiRepFormatter;
                    }
                }
                
                /* Add the string to the search warnings string buffer */
                snprintf(pucSearchReportLine, REP_LINE_MAXIMUM_LENGTH + 1, " - %s.\n", pucSearchWarning);
                iUtlStringBufferAppend(pvUtlSearchWarningsStringBuffer, pucSearchReportLine);
            }
            else {
                iUtlLogWarn(UTL_LOG_CONTEXT, "Failed to extract the search warning from the search report.");
                pucSearchWarning[0] = '\0';
            }
        }

        /* Process the search message */
        else if ( s_strncmp(pucSearchReportPtr, REP_SEARCH_MESSAGE, s_strlen(REP_SEARCH_MESSAGE)) == 0 ) {
            
            /* Extract the search message */
            if ( sscanf(pucSearchReportPtr + s_strlen(REP_SEARCH_MESSAGE) + 1, pucSearchMessageScanfFormat, pucSearchMessage) == 1 ) {
            
                /* Create the searches string buffer if it is not does exist */
                if ( pvUtlSearchesStringBuffer == NULL ) {
                    if ( (iError = iUtlStringBufferCreate(&pvUtlSearchesStringBuffer)) != UTL_NoError ) {
                        iUtlLogError(UTL_LOG_CONTEXT, "Failed to create the searches string buffer, utl error: %d.", iError);
                        iError = REP_MiscError;
                        goto bailFromiRepFormatter;
                    }
                }

                snprintf(pucSearchReportLine, REP_LINE_MAXIMUM_LENGTH + 1, "%s.\n\n", pucSearchMessage);
                iUtlStringBufferAppend(pvUtlSearchesStringBuffer, pucSearchReportLine);
            }
            else {
                iUtlLogWarn(UTL_LOG_CONTEXT, "Failed to extract the search message from the search report.");
                pucSearchSetting[0] = '\0';
            }
        }

        /* Process the search term */
        else if ( s_strncmp(pucSearchReportPtr, REP_SEARCH_TERM, s_strlen(REP_SEARCH_TERM)) == 0 ) {
            
            /* Extract the search term */
            if ( sscanf(pucSearchReportPtr + s_strlen(REP_SEARCH_TERM) + 1, pucSearchTermScanfFormat, pucSearchTerm, pucSearchField, pucSearchTermStemmed,
                    &fTermWeight, &iTermCount, &iDocCount) == 6 ) {
            
                /* Create the searches string buffer if it is not does exist */
                if ( pvUtlSearchesStringBuffer == NULL ) {
                    if ( (iError = iUtlStringBufferCreate(&pvUtlSearchesStringBuffer)) != UTL_NoError ) {
                        iUtlLogError(UTL_LOG_CONTEXT, "Failed to create the searches string buffer, utl error: %d.", iError);
                        iError = REP_MiscError;
                        goto bailFromiRepFormatter;
                    }
                }

                snprintf(pucSearchReportLine, REP_LINE_MAXIMUM_LENGTH + 1, "Search for '%s'", pucSearchTerm);
                iUtlStringBufferAppend(pvUtlSearchesStringBuffer, pucSearchReportLine);    

                if ( (bUtlStringsIsStringNULL(pucSearchField) == false) && (s_strcmp(pucSearchField, REP_UNFIELDED_STRING) != 0) ) {
                    snprintf(pucSearchReportLine, REP_LINE_MAXIMUM_LENGTH + 1, ", in the '%s' field", pucSearchField);
                    iUtlStringBufferAppend(pvUtlSearchesStringBuffer, pucSearchReportLine);
                }
                else {
                    iUtlStringBufferAppend(pvUtlSearchesStringBuffer, ", in any field");    
                }
                
                if ( (bUtlStringsIsStringNULL(pucSearchTermStemmed) == false) && (s_strcmp(pucSearchTermStemmed, REP_UNSTEMMED_STRING) != 0) ) {
                    snprintf(pucSearchReportLine, REP_LINE_MAXIMUM_LENGTH + 1, ", stemmed to '%s'", pucSearchTermStemmed);
                    iUtlStringBufferAppend(pvUtlSearchesStringBuffer, pucSearchReportLine);    
                }

                if ( fTermWeight > 0.0 ) {
                    snprintf(pucSearchReportLine, REP_LINE_MAXIMUM_LENGTH + 1, ", term weight: %.2f", fTermWeight);
                    iUtlStringBufferAppend(pvUtlSearchesStringBuffer, pucSearchReportLine);    
                }

                iUtlStringBufferAppend(pvUtlSearchesStringBuffer, ":\n");    
                
                if ( (iTermCount == REP_TERM_STOP) && (iDocCount == REP_TERM_STOP) ) {
                    snprintf(pucSearchReportLine, REP_LINE_MAXIMUM_LENGTH + 1, " '%s' is a stop term and is not indexed", pucSearchTerm);
                    iUtlStringBufferAppend(pvUtlSearchesStringBuffer, pucSearchReportLine);    
                }
                else if ( (iTermCount == REP_TERM_NON_EXISTENT) && (iDocCount == REP_TERM_NON_EXISTENT) ) {
                    snprintf(pucSearchReportLine, REP_LINE_MAXIMUM_LENGTH + 1, " '%s' does not exist", pucSearchTerm);
                    iUtlStringBufferAppend(pvUtlSearchesStringBuffer, pucSearchReportLine);    
                }
                else if ( (iTermCount == REP_TERM_FREQUENT) && (iDocCount == REP_TERM_FREQUENT) ) {
                    snprintf(pucSearchReportLine, REP_LINE_MAXIMUM_LENGTH + 1, " '%s' is a frequent term and was omitted from the search", pucSearchTerm);
                    iUtlStringBufferAppend(pvUtlSearchesStringBuffer, pucSearchReportLine);    
                }
                else {

                    snprintf(pucSearchReportLine, REP_LINE_MAXIMUM_LENGTH + 1, " '%s'", pucSearchTerm);
                    iUtlStringBufferAppend(pvUtlSearchesStringBuffer, pucSearchReportLine);    
                    
                    if ( iTermCount == 0 ) {
                        snprintf(pucSearchReportLine, REP_LINE_MAXIMUM_LENGTH + 1, " has no occurrences");
                    }
                    else if ( iTermCount == 1 ) {
                        snprintf(pucSearchReportLine, REP_LINE_MAXIMUM_LENGTH + 1, " occurs once");
                    }
                    else if ( iTermCount == 2 ) {
                        snprintf(pucSearchReportLine, REP_LINE_MAXIMUM_LENGTH + 1, " occurs twice");
                    }
                    else {
                        snprintf(pucSearchReportLine, REP_LINE_MAXIMUM_LENGTH + 1, " occurs %s times", pucUtlStringsFormatSignedNumber(iTermCount, pucNumberString, REP_LINE_MAXIMUM_LENGTH + 1));
                    }
                    iUtlStringBufferAppend(pvUtlSearchesStringBuffer, pucSearchReportLine);    

                    if ( iDocCount == 0 ) {
                        snprintf(pucSearchReportLine, REP_LINE_MAXIMUM_LENGTH + 1, " in any documents");
                    }
                    else if ( iDocCount == 1 ) {
                        snprintf(pucSearchReportLine, REP_LINE_MAXIMUM_LENGTH + 1, " in one document");
                    }
                    else {
                        snprintf(pucSearchReportLine, REP_LINE_MAXIMUM_LENGTH + 1, " in %s documents", pucUtlStringsFormatSignedNumber(iDocCount, pucNumberString, REP_LINE_MAXIMUM_LENGTH + 1));
                    }
                    iUtlStringBufferAppend(pvUtlSearchesStringBuffer, pucSearchReportLine);    
                }
    
                iUtlStringBufferAppend(pvUtlSearchesStringBuffer, "\n\n");    
            }
            else {
                iUtlLogWarn(UTL_LOG_CONTEXT, "Failed to extract the search term from the search report.");
                pucSearchTerm[0] = '\0';
                pucSearchField[0] = '\0';
                pucSearchTermStemmed[0] = '\0';
                fTermWeight = 0.0;
                iTermCount = 0;
                iDocCount = 0;
            }
        }

        /* Process the search term expanded */
        else if ( s_strncmp(pucSearchReportPtr, REP_SEARCH_TERM_EXPANDED, s_strlen(REP_SEARCH_TERM_EXPANDED)) == 0 ) {

            /* Extract the search term expanded */
            if ( sscanf(pucSearchReportPtr + s_strlen(REP_SEARCH_TERM_EXPANDED) + 1, pucSearchTermExpandedScanfFormat, pucSearchTerm, pucSearchField, &fTermWeight) == 3 ) {
            
                unsigned char *pucSearchReportEndPtr = NULL;
                
                /* Create the searches string buffer if it is not does exist */
                if ( pvUtlSearchesStringBuffer == NULL ) {
                    if ( (iError = iUtlStringBufferCreate(&pvUtlSearchesStringBuffer)) != UTL_NoError ) {
                        iUtlLogError(UTL_LOG_CONTEXT, "Failed to create the searches string buffer, utl error: %d.", iError);
                        iError = REP_MiscError;
                        goto bailFromiRepFormatter;
                    }
                }

                snprintf(pucSearchReportLine, REP_LINE_MAXIMUM_LENGTH + 1, "Search for '%s'", pucSearchTerm);
                iUtlStringBufferAppend(pvUtlSearchesStringBuffer, pucSearchReportLine);    

                if ( (bUtlStringsIsStringNULL(pucSearchField) == false) && (s_strcmp(pucSearchField, REP_UNFIELDED_STRING) != 0) ) {
                    snprintf(pucSearchReportLine, REP_LINE_MAXIMUM_LENGTH + 1, ", in the '%s' field", pucSearchField);
                    iUtlStringBufferAppend(pvUtlSearchesStringBuffer, pucSearchReportLine);    
                }
                else {
                    iUtlStringBufferAppend(pvUtlSearchesStringBuffer, ", in any field");    
                }
                
                if ( fTermWeight > 0.0 ) {
                    snprintf(pucSearchReportLine, REP_LINE_MAXIMUM_LENGTH + 1, ", term weight: %.2f", fTermWeight);
                    iUtlStringBufferAppend(pvUtlSearchesStringBuffer, pucSearchReportLine);    
                }
                
                iUtlStringBufferAppend(pvUtlSearchesStringBuffer, ", expanded to:\n");    
            
                
                /* Get the end of the line */
                if ( (pucSearchReportEndPtr = s_strchr(pucSearchReportPtr, '\n')) == NULL ) {
                    pucSearchReportEndPtr = s_strchr(pucSearchReportPtr, '\0');
                }
            
                /* Get a pointer to the next entry */
                if ( (pucPtr = s_strchr(pucSearchReportPtr, '\t')) != NULL ) {
                    pucPtr++;
                }
                
                /* Loop while there are entries to process */
                while ( (pucPtr != NULL) && (pucPtr < pucSearchReportEndPtr) ) {

                    /* Extract the search term expanded */
                    if ( sscanf(pucPtr, pucSearchTermExpandedTermScanfFormat, pucSearchTerm, &iTermCount, &iDocCount) == 3 ) {

                        if ( (iTermCount == REP_TERM_STOP) && (iDocCount == REP_TERM_STOP) ) {
                            snprintf(pucSearchReportLine, REP_LINE_MAXIMUM_LENGTH + 1, " '%s' is a stop term and is not indexed", pucSearchTerm);
                            iUtlStringBufferAppend(pvUtlSearchesStringBuffer, pucSearchReportLine);    
                        }
                        else if ( (iTermCount == REP_TERM_NON_EXISTENT) && (iDocCount == REP_TERM_NON_EXISTENT) ) {
                            snprintf(pucSearchReportLine, REP_LINE_MAXIMUM_LENGTH + 1, " '%s' does not exist", pucSearchTerm);
                            iUtlStringBufferAppend(pvUtlSearchesStringBuffer, pucSearchReportLine);    
                        }
                        else if ( (iTermCount == REP_TERM_FREQUENT) && (iDocCount == REP_TERM_FREQUENT) ) {
                            snprintf(pucSearchReportLine, REP_LINE_MAXIMUM_LENGTH + 1, " '%s' is a frequent term and was omitted from the search", pucSearchTerm);
                            iUtlStringBufferAppend(pvUtlSearchesStringBuffer, pucSearchReportLine);    
                        }
                        else {
        
                            snprintf(pucSearchReportLine, REP_LINE_MAXIMUM_LENGTH + 1, " '%s'", pucSearchTerm);
                            iUtlStringBufferAppend(pvUtlSearchesStringBuffer, pucSearchReportLine);    
                            
                            if ( iTermCount == 0 ) {
                                snprintf(pucSearchReportLine, REP_LINE_MAXIMUM_LENGTH + 1, " has no occurrences");
                            }
                            else if ( iTermCount == 1 ) {
                                snprintf(pucSearchReportLine, REP_LINE_MAXIMUM_LENGTH + 1, " occurs once");
                            }
                            else if ( iTermCount == 2 ) {
                                snprintf(pucSearchReportLine, REP_LINE_MAXIMUM_LENGTH + 1, " occurs twice");
                            }
                            else {
                                snprintf(pucSearchReportLine, REP_LINE_MAXIMUM_LENGTH + 1, " occurs %s times", pucUtlStringsFormatSignedNumber(iTermCount, pucNumberString, REP_LINE_MAXIMUM_LENGTH + 1));
                            }
                            iUtlStringBufferAppend(pvUtlSearchesStringBuffer, pucSearchReportLine);    
        
                            if ( iDocCount == 0 ) {
                                snprintf(pucSearchReportLine, REP_LINE_MAXIMUM_LENGTH + 1, " in any documents");
                            }
                            else if ( iDocCount == 1 ) {
                                snprintf(pucSearchReportLine, REP_LINE_MAXIMUM_LENGTH + 1, " in one document");
                            }
                            else {
                                snprintf(pucSearchReportLine, REP_LINE_MAXIMUM_LENGTH + 1, " in %s documents", pucUtlStringsFormatSignedNumber(iDocCount, pucNumberString, REP_LINE_MAXIMUM_LENGTH + 1));
                            }
                            iUtlStringBufferAppend(pvUtlSearchesStringBuffer, pucSearchReportLine);    
                        }

                        iUtlStringBufferAppend(pvUtlSearchesStringBuffer, "\n");    
                    }
                    else if ( sscanf(pucPtr, pucSearchTermExpandedCountsScanfFormat, &iTermCount, &iDocCount) == 2 ) {
                            
                        iUtlStringBufferAppend(pvUtlSearchesStringBuffer, "This expansion");    

                        if ( iTermCount == 0 ) {
                            snprintf(pucSearchReportLine, REP_LINE_MAXIMUM_LENGTH + 1, " has no occurrences");
                        }
                        else if ( iTermCount == 1 ) {
                            snprintf(pucSearchReportLine, REP_LINE_MAXIMUM_LENGTH + 1, " occurs once");
                        }
                        else if ( iTermCount == 2 ) {
                            snprintf(pucSearchReportLine, REP_LINE_MAXIMUM_LENGTH + 1, " occurs twice");
                        }
                        else {
                            snprintf(pucSearchReportLine, REP_LINE_MAXIMUM_LENGTH + 1, " occurs %s times", pucUtlStringsFormatSignedNumber(iTermCount, pucNumberString, REP_LINE_MAXIMUM_LENGTH + 1));
                        }
                        iUtlStringBufferAppend(pvUtlSearchesStringBuffer, pucSearchReportLine);    
    
                        if ( iDocCount == 0 ) {
                            snprintf(pucSearchReportLine, REP_LINE_MAXIMUM_LENGTH + 1, " in any documents");
                        }
                        else if ( iDocCount == 1 ) {
                            snprintf(pucSearchReportLine, REP_LINE_MAXIMUM_LENGTH + 1, " in one document");
                        }
                        else {
                            snprintf(pucSearchReportLine, REP_LINE_MAXIMUM_LENGTH + 1, " in %s documents", pucUtlStringsFormatSignedNumber(iDocCount, pucNumberString, REP_LINE_MAXIMUM_LENGTH + 1));
                        }
                        iUtlStringBufferAppend(pvUtlSearchesStringBuffer, pucSearchReportLine);    
                        iUtlStringBufferAppend(pvUtlSearchesStringBuffer, "\n\n");    
                    }
                    else {
                        iUtlLogWarn(UTL_LOG_CONTEXT, "Failed to extract the search term expanded from the search report (pre).");
                        pucSearchTerm[0] = '\0';
                        iTermCount = 0;
                        iDocCount = 0;
                    }

                    /* Get a pointer to the next entry */
                    if ( (pucPtr = s_strchr(pucPtr, '\t')) != NULL ) {
                        pucPtr++;
                    }
                }
            }
            else {
                iUtlLogWarn(UTL_LOG_CONTEXT, "Failed to extract the search term expanded from the search report (post).");
                pucSearchTerm[0] = '\0';
                pucSearchField[0] = '\0';
                fTermWeight = 0.0;
            }
        }

        /* Process the positive feedback counts */
        else if ( s_strncmp(pucSearchReportPtr, REP_POSITIVE_FEEDBACK_COUNTS, s_strlen(REP_POSITIVE_FEEDBACK_COUNTS)) == 0 ) {
            
            /* Extract the positive feedback counts */
            if ( sscanf(pucSearchReportPtr + s_strlen(REP_POSITIVE_FEEDBACK_COUNTS) + 1, pucPositiveFeedbackCountsScanfFormat, 
                    &uiPositiveFeedbackTotalTermCount, &uiPositiveFeedbackUniqueTermCount, &uiPositiveFeedbackUsedTermCount) != 3 ) {
    
                iUtlLogWarn(UTL_LOG_CONTEXT, "Failed to extract the positive feedback counts from the search report.");
                uiPositiveFeedbackTotalTermCount = 0;
                uiPositiveFeedbackUniqueTermCount = 0;
                uiPositiveFeedbackUsedTermCount = 0;
            }
        }

        /* Process the positive feedback term */
        else if ( s_strncmp(pucSearchReportPtr, REP_POSITIVE_FEEDBACK_TERMS, s_strlen(REP_POSITIVE_FEEDBACK_TERMS)) == 0 ) {
            
            unsigned char *pucSearchReportEndPtr = NULL;
            
            /* Get the end of the line */
            if ( (pucSearchReportEndPtr = s_strchr(pucSearchReportPtr, '\n')) == NULL ) {
                pucSearchReportEndPtr = s_strchr(pucSearchReportPtr, '\0');
            }
        
            /* Loop while there are terms to process */
            for ( pucPtr = pucSearchReportPtr + s_strlen(REP_POSITIVE_FEEDBACK_TERMS) + 1; (pucPtr != NULL) && (pucPtr < pucSearchReportEndPtr); ) {
            
                /* Extract the positive feedback terms */
                if ( sscanf(pucPtr, pucPositiveFeedbackTermsScanfFormat, pucPositiveFeedbackTerm) == 1 ) {
            
                    /* Create the positive feedback terms string buffer if it is not does exist */
                    if ( pvUtlPositiveFeedbackTermsStringBuffer == NULL ) {
                        if ( (iError = iUtlStringBufferCreate(&pvUtlPositiveFeedbackTermsStringBuffer)) != UTL_NoError ) {
                            iUtlLogError(UTL_LOG_CONTEXT, "Failed to create the positive feedback terms string buffer, utl error: %d.", iError);
                            iError = REP_MiscError;
                            goto bailFromiRepFormatter;
                        }
                    }
                    
                    /* Add the string to the positive feedback terms string buffer */
                    snprintf(pucSearchReportLine, REP_LINE_MAXIMUM_LENGTH + 1, "    %s\n", pucPositiveFeedbackTerm);
                    iUtlStringBufferAppend(pvUtlPositiveFeedbackTermsStringBuffer, pucSearchReportLine);    
                }
                else {
                    iUtlLogWarn(UTL_LOG_CONTEXT, "Failed to extract the positive feedback terms from the search report.");
                    pucPositiveFeedbackTerm[0] = '\0';
                }
        
                /* Get a pointer to the next entry */
                if ( (pucPtr = s_strchr(pucPtr, ' ')) != NULL ) {
                    pucPtr++;
                }
            }
        }

        /* Process the negative feedback counts */
        else if ( s_strncmp(pucSearchReportPtr, REP_NEGATIVE_FEEDBACK_COUNTS, s_strlen(REP_NEGATIVE_FEEDBACK_COUNTS)) == 0 ) {
            
            /* Extract the negative feedback counts */
            if ( sscanf(pucSearchReportPtr + s_strlen(REP_NEGATIVE_FEEDBACK_COUNTS) + 1, pucNegativeFeedbackCountsScanfFormat, 
                    &uiNegativeFeedbackTotalTermCount, &uiNegativeFeedbackUniqueTermCount, &uiNegativeFeedbackUsedTermCount) != 3 ) {
                
                iUtlLogWarn(UTL_LOG_CONTEXT, "Failed to extract the negative feedback counts from the search report.");
                uiNegativeFeedbackTotalTermCount = 0;
                uiNegativeFeedbackUniqueTermCount = 0;
                uiNegativeFeedbackUsedTermCount = 0;
            }
        }

        /* Process the negative feedback term */
        else if ( s_strncmp(pucSearchReportPtr, REP_NEGATIVE_FEEDBACK_TERMS, s_strlen(REP_NEGATIVE_FEEDBACK_TERMS)) == 0 ) {
            
            unsigned char *pucSearchReportEndPtr = NULL;
            
            /* Get the end of the line */
            if ( (pucSearchReportEndPtr = s_strchr(pucSearchReportPtr, '\n')) == NULL ) {
                pucSearchReportEndPtr = s_strchr(pucSearchReportPtr, '\0');
            }
        
            /* Loop while there are terms to process */
            for ( pucPtr = pucSearchReportPtr + s_strlen(REP_NEGATIVE_FEEDBACK_TERMS) + 1; (pucPtr != NULL) && (pucPtr < pucSearchReportEndPtr); ) {
            
                /* Extract the negative feedback terms */
                if ( sscanf(pucPtr, pucNegativeFeedbackTermsScanfFormat, pucNegativeFeedbackTerm) == 1 ) {
            
                    /* Create the negative feedback terms string buffer if it is not does exist */
                    if ( pvUtlNegativeFeedbackTermsStringBuffer == NULL ) {
                        if ( (iError = iUtlStringBufferCreate(&pvUtlNegativeFeedbackTermsStringBuffer)) != UTL_NoError ) {
                            iUtlLogError(UTL_LOG_CONTEXT, "Failed to create the negative feedback terms string buffer, utl error: %d.", iError);
                            iError = REP_MiscError;
                            goto bailFromiRepFormatter;
                        }
                    }
                    
                    /* Add the string to the negative feedback terms string buffer */
                    snprintf(pucSearchReportLine, REP_LINE_MAXIMUM_LENGTH + 1, "    %s\n", pucNegativeFeedbackTerm);
                    iUtlStringBufferAppend(pvUtlNegativeFeedbackTermsStringBuffer, pucSearchReportLine);    
                }
                else {
                    iUtlLogWarn(UTL_LOG_CONTEXT, "Failed to extract the negative feedback terms from the search report.");
                    pucNegativeFeedbackTerm[0] = '\0';
                }
        
                /* Get a pointer to the next entry */
                if ( (pucPtr = s_strchr(pucPtr, ' ')) != NULL ) {
                    pucPtr++;
                }
            }
        }

        /* Process the documents retrieved */
        else if ( s_strncmp(pucSearchReportPtr, REP_RETRIEVAL_COUNTS, s_strlen(REP_RETRIEVAL_COUNTS)) == 0 ) {
            
            unsigned int    uiAccurateTemp = 0;
            
            /* Extract the documents retrieved */
            if ( sscanf(pucSearchReportPtr + s_strlen(REP_RETRIEVAL_COUNTS) + 1, pucRetrievalCountsScanfFormat, &uiDocumentsRetrievedCount, &uiFailedDateMatchCount, 
                    &uiFailedTermCountMatchCount, &uiFailedLanguageMatchCount, &uiExcludedDocumentCount, &uiIncludedDocumentCount, &uiAccurateTemp) == 7 ) {

                /* Set the accuracy flag, once false always false */
                bAccurate = (bAccurate == false) ? false : ((uiAccurateTemp == 1) ? true : false);
            }
            else {
                
                iUtlLogWarn(UTL_LOG_CONTEXT, "Failed to extract the documents retrieved from the search report.");
                bAccurate = false;
                uiDocumentsRetrievedCount = 0;
                uiFailedDateMatchCount = 0;
                uiFailedTermCountMatchCount = 0;
                uiFailedLanguageMatchCount = 0;
                uiExcludedDocumentCount = 0;
                uiIncludedDocumentCount = 0;
            }
        }

/*         else { */
/*             iUtlLogDebug(UTL_LOG_CONTEXT, "Unknown tag in search report: '%s'.", pucSearchReportPtr); */
/*         } */


        /* Get a pointer to the next line */
        if ( (pucSearchReportPtr = s_strchr(pucSearchReportPtr, '\n')) != NULL ) {
            pucSearchReportPtr++;
        }

    }





    /* Generate the formatted search report */
    
    
    /* Allocate the string buffer for the formatted seach report */
    if ( (iError = iUtlStringBufferCreate(&pvUtlSearchReportFormattedStringBuffer)) != UTL_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to create the formatted search report string buffer, utl error: %d.", iError);
        iError = REP_MiscError;
        goto bailFromiRepFormatter;
    }
    
    
    /* Add the index name */
    if ( bUtlStringsIsStringNULL(pucIndexName) == false ) {
        snprintf(pucSearchReportLine, REP_LINE_MAXIMUM_LENGTH + 1, "Search on index: %s\n", pucIndexName);
        iUtlStringBufferAppend(pvUtlSearchReportFormattedStringBuffer, pucSearchReportLine);    
    }


    /* Add the index counts */
    if ( bGotIndexCounts == true ) {
        snprintf(pucSearchReportLine, REP_LINE_MAXIMUM_LENGTH + 1, "This index contains %s terms", pucUtlStringsFormatUnsignedNumber(ulTotalTermCount, pucNumberString, REP_LINE_MAXIMUM_LENGTH + 1));
        iUtlStringBufferAppend(pvUtlSearchReportFormattedStringBuffer, pucSearchReportLine);    
        snprintf(pucSearchReportLine, REP_LINE_MAXIMUM_LENGTH + 1, " (%s unique), ", pucUtlStringsFormatUnsignedNumber(ulUniqueTermCount, pucNumberString, REP_LINE_MAXIMUM_LENGTH + 1));
        iUtlStringBufferAppend(pvUtlSearchReportFormattedStringBuffer, pucSearchReportLine);    

        if ( (ulTotalStopTermCount > 0) && (ulUniqueStopTermCount > 0) ) {
            snprintf(pucSearchReportLine, REP_LINE_MAXIMUM_LENGTH + 1, "and %s stop terms", pucUtlStringsFormatUnsignedNumber(ulTotalStopTermCount, pucNumberString, REP_LINE_MAXIMUM_LENGTH + 1));
            iUtlStringBufferAppend(pvUtlSearchReportFormattedStringBuffer, pucSearchReportLine);    
            snprintf(pucSearchReportLine, REP_LINE_MAXIMUM_LENGTH + 1, " (%s unique), ", pucUtlStringsFormatUnsignedNumber(ulUniqueStopTermCount, pucNumberString, REP_LINE_MAXIMUM_LENGTH + 1));
            iUtlStringBufferAppend(pvUtlSearchReportFormattedStringBuffer, pucSearchReportLine);    
        }

        snprintf(pucSearchReportLine, REP_LINE_MAXIMUM_LENGTH + 1, "in %s documents.\n", pucUtlStringsFormatUnsignedNumber(uiDocumentCount, pucNumberString, REP_LINE_MAXIMUM_LENGTH + 1));
        iUtlStringBufferAppend(pvUtlSearchReportFormattedStringBuffer, pucSearchReportLine);    
    }


    /* Add the stemmer name */
    if ( bUtlStringsIsStringNULL(pucStemmerName) == false ) {
        snprintf(pucSearchReportLine, REP_LINE_MAXIMUM_LENGTH + 1, "The index was indexed using the '%s' stemmer.\n\n", pucStemmerName);
        iUtlStringBufferAppend(pvUtlSearchReportFormattedStringBuffer, pucSearchReportLine);    
    }


    /* Add the search original */
    if ( bUtlStringsIsStringNULL(pucSearchOriginal) == false ) {
        iUtlStringBufferAppend(pvUtlSearchReportFormattedStringBuffer, "The search:\n    ");    
        iUtlStringBufferAppend(pvUtlSearchReportFormattedStringBuffer, pucSearchOriginal);    
        iUtlStringBufferAppend(pvUtlSearchReportFormattedStringBuffer, "\n\n");    
    }


    /* Add the search reformatted */
    if ( bUtlStringsIsStringNULL(pucSearchReformatted) == false ) {
        iUtlStringBufferAppend(pvUtlSearchReportFormattedStringBuffer, "Is equivalent to:\n    ");    
        iUtlStringBufferAppend(pvUtlSearchReportFormattedStringBuffer, pucSearchReformatted);    
        iUtlStringBufferAppend(pvUtlSearchReportFormattedStringBuffer, "\n\n");    
    }


    /* Add the search error */
    if ( pvUtlSearchErrorsStringBuffer != NULL ) {
        iUtlStringBufferAppend(pvUtlSearchReportFormattedStringBuffer, "Generated the following errors:\n");    
        iUtlStringBufferGetString(pvUtlSearchErrorsStringBuffer, &pucPtr);
        iUtlStringBufferAppend(pvUtlSearchReportFormattedStringBuffer, pucPtr);    
        iUtlStringBufferAppend(pvUtlSearchReportFormattedStringBuffer, "\n\n");    
    }
    else if ( bUtlStringsIsStringNULL(pucSearchReformatted) == true ) {
        iUtlStringBufferAppend(pvUtlSearchReportFormattedStringBuffer, "Contained no search terms.\n\n");    
    }



    /* Add the search */
    if ( pvUtlSearchesStringBuffer != NULL ) {
        iUtlStringBufferGetString(pvUtlSearchesStringBuffer, &pucPtr);    
        iUtlStringBufferAppend(pvUtlSearchReportFormattedStringBuffer, pucPtr);    
    }
    

    /* Add the positive feedback term counts */
    if ( uiPositiveFeedbackTotalTermCount > 0 ) {
        snprintf(pucSearchReportLine, REP_LINE_MAXIMUM_LENGTH + 1, "Positive relevance feedback of %s term%s", 
                pucUtlStringsFormatUnsignedNumber(uiPositiveFeedbackTotalTermCount, pucNumberString, REP_LINE_MAXIMUM_LENGTH + 1), 
                (uiPositiveFeedbackTotalTermCount == 1) ? (unsigned char *)"" : (unsigned char *)"s");
        iUtlStringBufferAppend(pvUtlSearchReportFormattedStringBuffer, pucSearchReportLine);    
        snprintf(pucSearchReportLine, REP_LINE_MAXIMUM_LENGTH + 1, ", of which %s %s unique", 
                pucUtlStringsFormatUnsignedNumber(uiPositiveFeedbackUniqueTermCount, pucNumberString, REP_LINE_MAXIMUM_LENGTH + 1), 
                (uiPositiveFeedbackUniqueTermCount == 1) ? (unsigned char *)"is" : (unsigned char *)"are");
        iUtlStringBufferAppend(pvUtlSearchReportFormattedStringBuffer, pucSearchReportLine);    
        snprintf(pucSearchReportLine, REP_LINE_MAXIMUM_LENGTH + 1, ", and %s %s used.\n", 
                pucUtlStringsFormatUnsignedNumber(uiPositiveFeedbackUsedTermCount, pucNumberString, REP_LINE_MAXIMUM_LENGTH + 1), 
                (uiPositiveFeedbackUsedTermCount == 1) ? (unsigned char *)"was" : (unsigned char *)"were");
        iUtlStringBufferAppend(pvUtlSearchReportFormattedStringBuffer, pucSearchReportLine);    

        /* Add the positive feedback terms */
        if ( pvUtlPositiveFeedbackTermsStringBuffer != NULL ) {
            iUtlStringBufferAppend(pvUtlSearchReportFormattedStringBuffer, "The following terms were found to be useful:\n");    
            iUtlStringBufferGetString(pvUtlPositiveFeedbackTermsStringBuffer, &pucPtr);
            iUtlStringBufferAppend(pvUtlSearchReportFormattedStringBuffer, pucPtr);    
        }
        else {
            iUtlStringBufferAppend(pvUtlSearchReportFormattedStringBuffer, "No terms were found to be useful.\n");    
        }
        iUtlStringBufferAppend(pvUtlSearchReportFormattedStringBuffer, "\n\n");
    }

    
    /* Add the negative feedback term counts */
    if ( uiNegativeFeedbackTotalTermCount > 0 ) {
        snprintf(pucSearchReportLine, REP_LINE_MAXIMUM_LENGTH + 1, "Negative relevance feedback of %s term%s", 
                pucUtlStringsFormatUnsignedNumber(uiNegativeFeedbackTotalTermCount, pucNumberString, REP_LINE_MAXIMUM_LENGTH + 1), 
                (uiNegativeFeedbackTotalTermCount == 1) ? (unsigned char *)"" : (unsigned char *)"s");
        iUtlStringBufferAppend(pvUtlSearchReportFormattedStringBuffer, pucSearchReportLine);    
        snprintf(pucSearchReportLine, REP_LINE_MAXIMUM_LENGTH + 1, ", of which %s %s unique.\n", 
                pucUtlStringsFormatUnsignedNumber(uiNegativeFeedbackUniqueTermCount, pucNumberString, REP_LINE_MAXIMUM_LENGTH + 1), 
                (uiNegativeFeedbackUniqueTermCount == 1) ? (unsigned char *)"is" : (unsigned char *)"are");
        iUtlStringBufferAppend(pvUtlSearchReportFormattedStringBuffer, pucSearchReportLine);    
        snprintf(pucSearchReportLine, REP_LINE_MAXIMUM_LENGTH + 1, ", and %s %s used.\n", 
                pucUtlStringsFormatUnsignedNumber(uiNegativeFeedbackUsedTermCount, pucNumberString, REP_LINE_MAXIMUM_LENGTH + 1), 
                (uiNegativeFeedbackUsedTermCount == 1) ? (unsigned char *)"was" : (unsigned char *)"were");
        iUtlStringBufferAppend(pvUtlSearchReportFormattedStringBuffer, pucSearchReportLine);    

        /* Add the negative feedback terms */
        if ( pvUtlNegativeFeedbackTermsStringBuffer != NULL ) {
            iUtlStringBufferAppend(pvUtlSearchReportFormattedStringBuffer, "The following terms were found to be useful:\n");    
            iUtlStringBufferGetString(pvUtlNegativeFeedbackTermsStringBuffer, &pucPtr);
            iUtlStringBufferAppend(pvUtlSearchReportFormattedStringBuffer, pucPtr);    
        }
        else {
            iUtlStringBufferAppend(pvUtlSearchReportFormattedStringBuffer, "No terms were found to be useful.\n");    
        }
        iUtlStringBufferAppend(pvUtlSearchReportFormattedStringBuffer, "\n\n");    
    }

    
    /* Add the documents retrieved counts */
    if ( uiDocumentsRetrievedCount == 0 ) {
        iUtlStringBufferAppend(pvUtlSearchReportFormattedStringBuffer, "The search returned no documents.\n\n");    
    }
    else {

#if defined(REP_TYPE_ENABLE_ROUNDING_FOR_INACCURATE_COUNTS)

        /* Round off if the numbers are not accurate */
        if ( bAccurate == false ) {

            unsigned int    uiDocumentsRetrievedCountRounded = 0;

            /* Round off the value and update it */
            if ( iUtlNumRoundNumber((long)uiDocumentsRetrievedCount, UTL_NUM_ROUND_THRESHOLD, UTL_NUM_ROUND_ROUNDING, UTL_NUM_ROUND_DIGITS, 
                    (long *)&uiDocumentsRetrievedCountRounded) == UTL_NoError ) {
                uiDocumentsRetrievedCount = uiDocumentsRetrievedCountRounded;
            }
        }

#endif    /* defined(REP_TYPE_ENABLE_ROUNDING_FOR_INACCURATE_COUNTS) */

        snprintf(pucSearchReportLine, REP_LINE_MAXIMUM_LENGTH + 1, "The search retrieved %s%s document%s.\n\n", (bAccurate == true) ? "" : "about ", 
                pucUtlStringsFormatUnsignedNumber(uiDocumentsRetrievedCount, pucNumberString, REP_LINE_MAXIMUM_LENGTH + 1), (uiDocumentsRetrievedCount == 1) ? "" : "s");
        iUtlStringBufferAppend(pvUtlSearchReportFormattedStringBuffer, pucSearchReportLine);    
    }


    if ( uiFailedDateMatchCount > 0 ) {

#if defined(REP_TYPE_ENABLE_ROUNDING_FOR_INACCURATE_COUNTS)

        /* Round off if the numbers are not accurate */
        if ( bAccurate == false ) {

            unsigned int    uiFailedDateMatchCountRounded = 0;

            if ( iUtlNumRoundNumber((long)uiFailedDateMatchCount, UTL_NUM_ROUND_THRESHOLD, UTL_NUM_ROUND_ROUNDING, UTL_NUM_ROUND_DIGITS, 
                    (long *)&uiFailedDateMatchCountRounded) == UTL_NoError ) {
                uiFailedDateMatchCount = uiFailedDateMatchCountRounded;
            }
        }

#endif    /* defined(REP_TYPE_ENABLE_ROUNDING_FOR_INACCURATE_COUNTS) */

        snprintf(pucSearchReportLine, REP_LINE_MAXIMUM_LENGTH + 1, " - %s %s document%s fell outside the date/time restriction.\n", (bAccurate == true) ? "" : "about ", 
                pucUtlStringsFormatUnsignedNumber(uiFailedDateMatchCount, pucNumberString, REP_LINE_MAXIMUM_LENGTH + 1), (uiFailedDateMatchCount == 1) ? "" : "s");
        iUtlStringBufferAppend(pvUtlSearchReportFormattedStringBuffer, pucSearchReportLine);    
    }


    if ( uiFailedTermCountMatchCount > 0 ) {

#if defined(REP_TYPE_ENABLE_ROUNDING_FOR_INACCURATE_COUNTS)

        /* Round off if the numbers are not accurate */
        if ( bAccurate == false ) {

            unsigned int    uiFailedTermCountMatchCountRounded = 0;

            if ( iUtlNumRoundNumber((long)uiFailedTermCountMatchCount, UTL_NUM_ROUND_THRESHOLD, UTL_NUM_ROUND_ROUNDING, UTL_NUM_ROUND_DIGITS, 
                    (long *)&uiFailedDateMatchCountRounded) == UTL_NoError ) {
                uiFailedTermCountMatchCount = uiFailedTermCountMatchCountRounded;
            }
        }

#endif    /* defined(REP_TYPE_ENABLE_ROUNDING_FOR_INACCURATE_COUNTS) */

        snprintf(pucSearchReportLine, REP_LINE_MAXIMUM_LENGTH + 1, " - %s %s document%s fell outside the term count restriction.\n", (bAccurate == true) ? "" : "about ", 
                pucUtlStringsFormatUnsignedNumber(uiFailedTermCountMatchCount, pucNumberString, REP_LINE_MAXIMUM_LENGTH + 1), (uiFailedTermCountMatchCount == 1) ? "" : "s");
        iUtlStringBufferAppend(pvUtlSearchReportFormattedStringBuffer, pucSearchReportLine);    
    }


    if ( uiFailedLanguageMatchCount > 0 ) {

#if defined(REP_TYPE_ENABLE_ROUNDING_FOR_INACCURATE_COUNTS)

        /* Round off if the numbers are not accurate */
        if ( bAccurate == false ) {

            unsigned int    uiFailedLanguageMatchCountRounded = 0;

            if ( iUtlNumRoundNumber((long)uiFailedLanguageMatchCount, UTL_NUM_ROUND_THRESHOLD, UTL_NUM_ROUND_ROUNDING, UTL_NUM_ROUND_DIGITS, 
                    (long *)&uiFailedLanguageMatchCountRounded) == UTL_NoError ) {
                uiFailedLanguageMatchCount = uiFailedLanguageMatchCountRounded;
            }
        }

#endif    /* defined(REP_TYPE_ENABLE_ROUNDING_FOR_INACCURATE_COUNTS) */

        snprintf(pucSearchReportLine, REP_LINE_MAXIMUM_LENGTH + 1, " - %s %s document%s fell outside the language restriction.\n", (bAccurate == true) ? "" : "about ", 
                pucUtlStringsFormatUnsignedNumber(uiFailedLanguageMatchCount, pucNumberString, REP_LINE_MAXIMUM_LENGTH + 1), (uiFailedLanguageMatchCount == 1) ? "" : "s");
        iUtlStringBufferAppend(pvUtlSearchReportFormattedStringBuffer, pucSearchReportLine);    
    }


    if ( uiExcludedDocumentCount > 0 ) {

#if defined(REP_TYPE_ENABLE_ROUNDING_FOR_INACCURATE_COUNTS)

        /* Round off if the numbers are not accurate */
        if ( bAccurate == false ) {

            unsigned int    uiExcludedDocumentCountRounded = 0;

            if ( iUtlNumRoundNumber((long)uiExcludedDocumentCount, UTL_NUM_ROUND_THRESHOLD, UTL_NUM_ROUND_ROUNDING, UTL_NUM_ROUND_DIGITS, 
                    (long *)&uiExcludedDocumentCountRounded) == UTL_NoError ) {
                uiExcludedDocumentCount = uiExcludedDocumentCountRounded;
            }
        }

#endif    /* defined(REP_TYPE_ENABLE_ROUNDING_FOR_INACCURATE_COUNTS) */

        snprintf(pucSearchReportLine, REP_LINE_MAXIMUM_LENGTH + 1, " - %s %s document%s marked as excluded.\n", (bAccurate == true) ? "" : "about ", 
                pucUtlStringsFormatUnsignedNumber(uiExcludedDocumentCount, pucNumberString, REP_LINE_MAXIMUM_LENGTH + 1), (uiExcludedDocumentCount == 1) ? " was" : "s were");
        iUtlStringBufferAppend(pvUtlSearchReportFormattedStringBuffer, pucSearchReportLine);    
    }
/*     else { */
/*         iUtlStringBufferAppend(pvUtlSearchReportFormattedStringBuffer, " - no documents were marked as excluded.\n");     */
/*     } */


    if ( uiIncludedDocumentCount > 0 ) {

#if defined(REP_TYPE_ENABLE_ROUNDING_FOR_INACCURATE_COUNTS)

        /* Round off if the numbers are not accurate */
        if ( bAccurate == false ) {

            unsigned int    uiIncludedDocumentCountRounded = 0;

            if ( iUtlNumRoundNumber((long)uiIncludedDocumentCount, UTL_NUM_ROUND_THRESHOLD, UTL_NUM_ROUND_ROUNDING, UTL_NUM_ROUND_DIGITS,
                    (long *)&uiIncludedDocumentCountRounded) == UTL_NoError ) {
                uiIncludedDocumentCount = uiIncludedDocumentCountRounded;
            }
        }

#endif    /* defined(REP_TYPE_ENABLE_ROUNDING_FOR_INACCURATE_COUNTS) */

        snprintf(pucSearchReportLine, REP_LINE_MAXIMUM_LENGTH + 1, " - %s %s document%s marked as included.\n", (bAccurate == true) ? "" : "about ", 
                pucUtlStringsFormatUnsignedNumber(uiIncludedDocumentCount, pucNumberString, REP_LINE_MAXIMUM_LENGTH + 1), (uiIncludedDocumentCount == 1) ? " was" : "s were");
        iUtlStringBufferAppend(pvUtlSearchReportFormattedStringBuffer, pucSearchReportLine);    
    }
/*     else { */
/*         iUtlStringBufferAppend(pvUtlSearchReportFormattedStringBuffer, " - no documents were marked as included.\n");     */
/*     } */


    iUtlStringBufferAppend(pvUtlSearchReportFormattedStringBuffer, "\n");    




    /* Add the search restrictions */
    if ( pvUtlSearchRestrictionsStringBuffer != NULL ) {
        iUtlStringBufferAppend(pvUtlSearchReportFormattedStringBuffer, "The search was evaluated with the following restrictions:\n");    
        iUtlStringBufferGetString(pvUtlSearchRestrictionsStringBuffer, &pucPtr);
        iUtlStringBufferAppend(pvUtlSearchReportFormattedStringBuffer, pucPtr);    
        iUtlStringBufferAppend(pvUtlSearchReportFormattedStringBuffer, "\n");    
    }

    
    /* Add the search warnings */
    if ( pvUtlSearchWarningsStringBuffer != NULL ) {
        iUtlStringBufferAppend(pvUtlSearchReportFormattedStringBuffer, "The search generated the following warnings:\n");    
        iUtlStringBufferGetString(pvUtlSearchWarningsStringBuffer, &pucPtr);
        iUtlStringBufferAppend(pvUtlSearchReportFormattedStringBuffer, pucPtr);    
        iUtlStringBufferAppend(pvUtlSearchReportFormattedStringBuffer, "\n");    
    }
    
    
    /* Add the search settings */
    if ( pvUtlSearchSettingsStringBuffer != NULL ) {
        iUtlStringBufferAppend(pvUtlSearchReportFormattedStringBuffer, "The search was evaluated with the following settings:\n");    
        iUtlStringBufferGetString(pvUtlSearchSettingsStringBuffer, &pucPtr);
        iUtlStringBufferAppend(pvUtlSearchReportFormattedStringBuffer, pucPtr);    
        iUtlStringBufferAppend(pvUtlSearchReportFormattedStringBuffer, "\n");    
    }
    

    /* Add the search debugs */
    if ( pvUtlSearchDebugsStringBuffer != NULL ) {
        iUtlStringBufferAppend(pvUtlSearchReportFormattedStringBuffer, "The search generated the following debug messages:\n");    
        iUtlStringBufferGetString(pvUtlSearchDebugsStringBuffer, &pucPtr);
        iUtlStringBufferAppend(pvUtlSearchReportFormattedStringBuffer, pucPtr);    
        iUtlStringBufferAppend(pvUtlSearchReportFormattedStringBuffer, "\n");    
    }

    
    
    /* Bail label */
    bailFromiRepFormatter:

    
    /* Handle the error */
    if ( iError == UTL_NoError ) {

        /* Set the return pointer */
        iUtlStringBufferGetString(pvUtlSearchReportFormattedStringBuffer, ppucSearchReportFormatted);

        /* Free the formatted search report string buffer, note that we don't free the string */
        iUtlStringBufferFree(pvUtlSearchReportFormattedStringBuffer, false);
        pvUtlSearchReportFormattedStringBuffer = NULL;

        /* Set the error */
        iError = REP_NoError;
    }
    else {
        
        /* Free the formatted search report string buffer, note that we DO free the string */
        iUtlStringBufferFree(pvUtlSearchReportFormattedStringBuffer, true);
        pvUtlSearchReportFormattedStringBuffer = NULL;

        /* Set the error */
        iError = REP_MiscError;
    }


    /* Free all the other string buffers */
    iUtlStringBufferFree(pvUtlSearchErrorsStringBuffer, true);
    pvUtlSearchErrorsStringBuffer = NULL;

    iUtlStringBufferFree(pvUtlSearchSettingsStringBuffer, true);
    pvUtlSearchSettingsStringBuffer = NULL;

    iUtlStringBufferFree(pvUtlSearchRestrictionsStringBuffer, true);
    pvUtlSearchRestrictionsStringBuffer = NULL;

    iUtlStringBufferFree(pvUtlSearchDebugsStringBuffer, true);
    pvUtlSearchDebugsStringBuffer = NULL;

    iUtlStringBufferFree(pvUtlSearchWarningsStringBuffer, true);
    pvUtlSearchWarningsStringBuffer = NULL;

    iUtlStringBufferFree(pvUtlSearchesStringBuffer, true);
    pvUtlSearchesStringBuffer = NULL;

    iUtlStringBufferFree(pvUtlPositiveFeedbackTermsStringBuffer, true);
    pvUtlPositiveFeedbackTermsStringBuffer = NULL;

    iUtlStringBufferFree(pvUtlNegativeFeedbackTermsStringBuffer, true);
    pvUtlNegativeFeedbackTermsStringBuffer = NULL;


    return (iError);
    
}


/*---------------------------------------------------------------------------*/


/*

    Function:   iRepMergeAndFormatSearchReports()

    Purpose:    This function merges and formats the passed search reports. The search 
                reports are passed as a NULL terminated array of search reports. These search
                reports can be individual search reports, or concatenated search reports.
                
                A pointer to the formatted search reports will be returned.

    Parameters: ppucSearchReportList        NULL terminated array of search reports
                ppucSearchReportFormatted   return parameter for the formatted search report

    Globals:    none

    Returns:    REP error code

*/
int iRepMergeAndFormatSearchReports
(
    unsigned char **ppucSearchReportList,
    unsigned char **ppucSearchReportFormatted
)
{

    int             iError = UTL_NoError;
    unsigned char   *pucSearchReportMerged = NULL;
    unsigned char   *pucSearchReportFormatted = NULL;


    /* Check the parameters */
    if ( ppucSearchReportList == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppucSearchReportList' parameter passed to 'iRepMergeAndFormatSearchReports'."); 
        return (REP_InvalidSearchReportList);
    }

    if ( ppucSearchReportFormatted == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppucSearchReportFormatted' parameter passed to 'iRepMergeAndFormatSearchReports'."); 
        return (REP_ReturnParameterError);
    }


    /* Merge the search reports */
    if ( (iError = iRepMergeSearchReports(ppucSearchReportList, &pucSearchReportMerged)) != UTL_NoError ) {
        goto bailFromiRepMergerFormatter;
    }
/*     iUtlLogDebug(UTL_LOG_CONTEXT, "\n[%s]\n\n", pucSearchReportMerged); */
                            

    /* Format the search report */
    if ( (iError = iRepFormatSearchReports(pucSearchReportMerged, &pucSearchReportFormatted)) != UTL_NoError ) {
        goto bailFromiRepMergerFormatter;
    }
/*     iUtlLogDebug(UTL_LOG_CONTEXT, "\n%s\n\n", pucSearchReportFormatted); */
                    


    /* Bail label */
    bailFromiRepMergerFormatter:
    

    /* Handle the error */
    if ( iError == UTL_NoError ) {
        
        /* Set the return pointer */
        *ppucSearchReportFormatted = pucSearchReportFormatted;
    }
    else {
        /* Free the formatted search report */
        s_free(pucSearchReportFormatted);
    }

    /* Free the merged search report, needs to be done whether we succeed or not */
    s_free(pucSearchReportMerged);


    return (iError);

}


/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/


/*

    Function:   iRepHashPrintCallBackFunction()

    Purpose:    This function is passed as the call back function to iUtlHashLoopOverKeys()
                and gets called for every key in the hash. It will get passed
                pointers to the key currently being processed and to the data
                stored for that key. It will also get passed as a va_list, the 
                parameters that were specified in the call to iUtlHashLoopOverKeys().

    Parameters: pucKey      key
                pvData      data        
                ap          args (optional)

    Globals:    none

    Returns:    0 if successful, non-0 on error
*/
static int iRepHashPrintCallBackFunction
(
    unsigned char *pucKey,
    void *pvData,
    va_list ap
)
{

    va_list         ap_;
    unsigned char   *pucTag = NULL;
    unsigned char   *pucNewLine = NULL;
    void            *pvUtlSearchReportFormattedStringBuffer = NULL;
    unsigned char   pucSearchReportLine[REP_LINE_MAXIMUM_LENGTH + 1];


    ASSERT(bUtlStringsIsStringNULL(pucKey) == false);
/*     ASSERT(pvData != NULL); */


    /* Get all our parameters, note that we make a copy of 'ap' */
    va_copy(ap_, ap);
    pucTag = (unsigned char *)va_arg(ap_, unsigned char *);
    pucNewLine = (unsigned char *)va_arg(ap_, unsigned char *);
    pvUtlSearchReportFormattedStringBuffer = (void *)va_arg(ap_, void *);
    va_end(ap_);


/*     ASSERT(pucTag != NULL); */
/*     ASSERT(pucNewLine != NULL); */
    ASSERT(pvUtlSearchReportFormattedStringBuffer != NULL);


    /* Generate the line */
    snprintf(pucSearchReportLine, REP_LINE_MAXIMUM_LENGTH + 1, "%s%s%s%s", (pucTag != NULL) ? pucTag : (unsigned char *)"", 
            (pucTag != NULL) ? (unsigned char *)" " : (unsigned char *)"", pucKey, (pucNewLine != NULL) ? pucNewLine : (unsigned char *)"");
    

    /* Add it to the formatted search report string buffer */
    iUtlStringBufferAppend(pvUtlSearchReportFormattedStringBuffer, pucSearchReportLine);

    
    return (0);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iRepTriePrintCallBackFunction()

    Purpose:    This function is passed as the call back function to iUtlHashLoopOverKeys()
                and gets called for every key in the hash. It will get passed
                pointers to the key currently being processed and to the data
                stored for that key. It will also get passed as a va_list, the 
                parameters that were specified in the call to iUtlHashLoopOverKeys().

    Parameters: pucKey      key
                pvData      data        
                ap          args (optional)

    Globals:    none

    Returns:    0 if successful, non-0 on error
*/
static int iRepTriePrintCallBackFunction
(
    unsigned char *pucKey,
    void *pvData,
    va_list ap
)
{

    va_list                     ap_;
    unsigned char               pucSearchReportLine[REP_LINE_MAXIMUM_LENGTH + 1];
    struct repTrieTermEntry     *prtteRepTrieTermEntry = NULL;
    void                        *pvUtlSearchReportMergedStringBuffer = NULL;


    ASSERT(bUtlStringsIsStringNULL(pucKey) == false);
    ASSERT(pvData != NULL);

    
    /* Get the trie term entry */
    prtteRepTrieTermEntry = (struct repTrieTermEntry *)pvData;

    /* Get all our parameters, note that we make a copy of 'ap' */
    va_copy(ap_, ap);
    pvUtlSearchReportMergedStringBuffer = (void *)va_arg(ap_, void *);
    va_end(ap_);


    ASSERT(pvUtlSearchReportMergedStringBuffer != NULL);


    /* Generate the line */
    snprintf(pucSearchReportLine, REP_LINE_MAXIMUM_LENGTH + 1, "\t%s %d %d", pucKey, prtteRepTrieTermEntry->iTermCount, prtteRepTrieTermEntry->iDocCount);
    

    /* Add it to the formatted search report string buffer */
    iUtlStringBufferAppend(pvUtlSearchReportMergedStringBuffer, pucSearchReportLine);


    return (0);

}


/*---------------------------------------------------------------------------*/




