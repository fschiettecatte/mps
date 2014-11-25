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

    Module:     spi.c

    Author:     Francois Schiettecatte (FS Consulting LLC.)

    Created:    7 May 2004

    Purpose:    This file contains support functions for the SPI.

*/


/*---------------------------------------------------------------------------*/


/*
** Includes
*/

#include "spi.h"


/*---------------------------------------------------------------------------*/


/*
** Feature defines
*/

/* Context for logging */
#undef UTL_LOG_CONTEXT
#define UTL_LOG_CONTEXT                     (unsigned char *)"com.fsconsult.mps.src.spi.spi"


/*---------------------------------------------------------------------------*/


/*
** Defines
*/

/* Error mapping global structure end marker, needs to be postive */
#define SPI_ERROR_MAPPING_END_MARKER        (1)


/*---------------------------------------------------------------------------*/


/*
** Structures
*/

/* Error mapping structure */
struct spiErrorMapping {
    int             iError;
    unsigned char   *pucErrorText;
};


/*---------------------------------------------------------------------------*/


/*
** Globals
*/

/* Error mapping global */
static struct spiErrorMapping psemSpiErrorMappingsGlobal[] = 
{
    {SPI_NoError,                           (unsigned char *)"No error"}, 
    {SPI_MemError,                          (unsigned char *)"Memory error"}, 
    {SPI_ParameterError,                    (unsigned char *)"Parameter error"}, 
    {SPI_ReturnParameterError,              (unsigned char *)"Return parameter error"}, 
    {SPI_MiscError,                         (unsigned char *)"Miscelaneous error"}, 
    {SPI_ExceededLoadMaximum,               (unsigned char *)"Exceeded load maximum"}, 
    {SPI_InvalidSession,                    (unsigned char *)"Invalid session"}, 
    {SPI_InvalidIndexDirectory,             (unsigned char *)"Invalid index directory"}, 
    {SPI_InvalidConfigurationDirectory,     (unsigned char *)"Invalid configuration directory"}, 
    {SPI_InitializeServerFailed,            (unsigned char *)"Failed to initialize server"}, 
    {SPI_ShutdownServerFailed,              (unsigned char *)"Failed to shutdown server"}, 
    {SPI_InvalidIndexName,                  (unsigned char *)"Invalid index name"}, 
    {SPI_InvalidIndex,                      (unsigned char *)"Invalid index"}, 
    {SPI_OpenIndexFailed,                   (unsigned char *)"Failed to open index"}, 
    {SPI_CloseIndexFailed,                  (unsigned char *)"Failed to close index"}, 
    {SPI_InvalidLanguageCode,               (unsigned char *)"Invalid search language code"}, 
    {SPI_InvalidSearchText,                 (unsigned char *)"Invalid search text"}, 
    {SPI_InvalidPositiveFeedbackText,       (unsigned char *)"Invalid search positive feedback text"}, 
    {SPI_InvalidNegativeFeedbackText,       (unsigned char *)"Invalid search negative feedback text"}, 
    {SPI_InvalidSearchResultsRange,         (unsigned char *)"Invalid search results range"}, 
    {SPI_SearchIndexFailed,                 (unsigned char *)"Failed to search index"}, 
    {SPI_InvalidDocumentKey,                (unsigned char *)"Invalid document key"}, 
    {SPI_InvalidItemName,                   (unsigned char *)"Invalid item name"}, 
    {SPI_InvalidMimeType,                   (unsigned char *)"Invalid mime type"}, 
    {SPI_InvalidChunkType,                  (unsigned char *)"Invalid chunk type"}, 
    {SPI_InvalidChunkRange,                 (unsigned char *)"Invalid chunk range"}, 
    {SPI_RetrieveDocumentFailed,            (unsigned char *)"Failed to retrieve document"}, 
    {SPI_GetServerInfoFailed,               (unsigned char *)"Failed to get server information"}, 
    {SPI_GetServerIndexInfoFailed,          (unsigned char *)"Failed to get server index information"}, 
    {SPI_ServerHasNoIndices,                (unsigned char *)"Server has no index"}, 
    {SPI_GetIndexInfoFailed,                (unsigned char *)"Failed to get index information"}, 
    {SPI_GetIndexFieldInfoFailed,           (unsigned char *)"Failed to get index field information"}, 
    {SPI_IndexHasNoSearchFields,            (unsigned char *)"Index has no search fields"}, 
    {SPI_InvalidTermMatch,                  (unsigned char *)"Invalid term march"}, 
    {SPI_InvalidTermCase,                   (unsigned char *)"Invalid term case"}, 
    {SPI_InvalidTerm,                       (unsigned char *)"Invalid term"}, 
    {SPI_InvalidFieldName,                  (unsigned char *)"Invalid field name"}, 
    {SPI_GetIndexTermInfoFailed,            (unsigned char *)"Failed to get index term information"}, 
    {SPI_IndexHasNoTerms,                   (unsigned char *)"Index has no terms"}, 
    {SPI_GetDocumentInfoFailed,             (unsigned char *)"Failed to get document information"}, 
    {SPI_GetIndexNameFailed,                (unsigned char *)"Failed to get index name"}, 
    {SPI_ERROR_MAPPING_END_MARKER,          NULL,}
};


/*---------------------------------------------------------------------------*/


/*
** Private function prototypes
*/

static int iSpiQuickSortSearchResultsDoubleAsc (struct spiSearchResult *pssrSpiSearchResults, 
        int iSpiSearchResultsLeftIndex, int iSpiSearchResultsRightIndex);
static int iSpiQuickSortSearchResultsDoubleDesc (struct spiSearchResult *pssrSpiSearchResults,  
        int iSpiSearchResultsLeftIndex, int iSpiSearchResultsRightIndex);

static int iSpiQuickSortSearchResultsFloatAsc (struct spiSearchResult *pssrSpiSearchResults,  
        int iSpiSearchResultsLeftIndex, int iSpiSearchResultsRightIndex);
static int iSpiQuickSortSearchResultsFloatDesc (struct spiSearchResult *pssrSpiSearchResults,  
        int iSpiSearchResultsLeftIndex, int iSpiSearchResultsRightIndex);

static int iSpiQuickSortSearchResultsUIntAsc (struct spiSearchResult *pssrSpiSearchResults,  
        int iSpiSearchResultsLeftIndex, int iSpiSearchResultsRightIndex);
static int iSpiQuickSortSearchResultsUIntDesc (struct spiSearchResult *pssrSpiSearchResults,  
        int iSpiSearchResultsLeftIndex, int iSpiSearchResultsRightIndex);

static int iSpiQuickSortSearchResultsULongAsc (struct spiSearchResult *pssrSpiSearchResults,  
        int iSpiSearchResultsLeftIndex, int iSpiSearchResultsRightIndex);
static int iSpiQuickSortSearchResultsULongDesc (struct spiSearchResult *pssrSpiSearchResults,  
        int iSpiSearchResultsLeftIndex, int iSpiSearchResultsRightIndex);

static int iSpiQuickSortSearchResultsCharAsc (struct spiSearchResult *pssrSpiSearchResults,  
        int iSpiSearchResultsLeftIndex, int iSpiSearchResultsRightIndex);
static int iSpiQuickSortSearchResultsCharDesc (struct spiSearchResult *pssrSpiSearchResults,  
        int iSpiSearchResultsLeftIndex, int iSpiSearchResultsRightIndex);


/*---------------------------------------------------------------------------*/


/* 
** =========================================
** ===  Structure Duplication Functions  ===
** =========================================
*/


/*

    Function:   iSpiDuplicateSession

    Purpose:    This function duplicates an spi session structure.

    Parameters: pssSpiSession       pointer to an spi session structure
                ppssSpiSession      return pointer to a duplicate of the spi session structure

    Globals:    none

    Returns:    SPI Error Code

*/
int iSpiDuplicateSession
(
    struct spiSession *pssSpiSession,
    struct spiSession **ppssSpiSession
)
{

    unsigned int        iError = SPI_NoError;
    struct spiSession   *pssSpiSessionCopy = NULL;


    /* Duplicate the structure and copy the components */
    if ( (pssSpiSession != NULL) && (ppssSpiSession != NULL) ) {

        if ( (pssSpiSessionCopy = (struct spiSession *)s_malloc(sizeof(struct spiSession))) == NULL ) {
            iError = SPI_MemError;
            goto bailFromiSpiDuplicateSession;
        }
            
        if ( pssSpiSession->pucIndexDirectoryPath != NULL ) {
            if ( (pssSpiSessionCopy->pucIndexDirectoryPath = s_strdup(pssSpiSession->pucIndexDirectoryPath)) == NULL ) {
                iError = SPI_MemError;
                goto bailFromiSpiDuplicateSession;
            }
        }

        if ( pssSpiSession->pucConfigurationDirectoryPath != NULL ) {
            if ( (pssSpiSessionCopy->pucConfigurationDirectoryPath = s_strdup(pssSpiSession->pucConfigurationDirectoryPath)) == NULL ) {
                iError = SPI_MemError;
                goto bailFromiSpiDuplicateSession;
            }
        }

        if ( pssSpiSession->pucTemporaryDirectoryPath != NULL ) {
            if ( (pssSpiSessionCopy->pucTemporaryDirectoryPath = s_strdup(pssSpiSession->pucTemporaryDirectoryPath)) == NULL ) {
                iError = SPI_MemError;
                goto bailFromiSpiDuplicateSession;
            }
        }

        pssSpiSessionCopy->pvClientPtr = pssSpiSession->pvClientPtr;
    }
    else {
        iError = SPI_MiscError;
        goto bailFromiSpiDuplicateSession;
    }



    /* Bail label */
    bailFromiSpiDuplicateSession:


    /* Handle the error */
    if ( iError == SPI_NoError ) {
        *ppssSpiSession = pssSpiSessionCopy;
    }
    else {
        iSpiFreeSession(pssSpiSessionCopy);
        pssSpiSessionCopy = NULL;
    }


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSpiDuplicateServerInfo

    Purpose:    This function duplicates an spi server info structure.

    Parameters: pssiSpiServerInfo       pointer to an spi server info structure
                ppssiSpiServerInfo      return pointer to a duplicate of the spi server info structure

    Globals:    none

    Returns:    SPI Error Code

*/
int iSpiDuplicateServerInfo
(
    struct spiServerInfo *pssiSpiServerInfo,
    struct spiServerInfo **ppssiSpiServerInfo
)
{

    unsigned int            iError = SPI_NoError;
    struct spiServerInfo    *pssiSpiServerInfoCopy = NULL;


    /* Duplicate the structure and copy the components */
    if ( (pssiSpiServerInfo != NULL) && (ppssiSpiServerInfo != NULL) ) {

        if ( (pssiSpiServerInfoCopy = (struct spiServerInfo *)s_malloc(sizeof(struct spiServerInfo))) == NULL ) {
            iError = SPI_MemError;
            goto bailFromiSpiDuplicateServerInfo;
        }
            
        if ( pssiSpiServerInfo->pucName != NULL ) {
            if ( (pssiSpiServerInfoCopy->pucName = s_strdup(pssiSpiServerInfo->pucName)) == NULL ) {
                iError = SPI_MemError;
                goto bailFromiSpiDuplicateServerInfo;
            }
        }
            
        if ( pssiSpiServerInfo->pucDescription != NULL ) {
            if ( (pssiSpiServerInfoCopy->pucDescription = s_strdup(pssiSpiServerInfo->pucDescription)) == NULL ) {
                iError = SPI_MemError;
                goto bailFromiSpiDuplicateServerInfo;
            }
        }
    
        if ( pssiSpiServerInfo->pucAdminName != NULL ) {
            if ( (pssiSpiServerInfoCopy->pucAdminName = s_strdup(pssiSpiServerInfo->pucAdminName)) == NULL ) {
                iError = SPI_MemError;
                goto bailFromiSpiDuplicateServerInfo;
            }
        }
    
        if ( pssiSpiServerInfo->pucAdminEmail != NULL ) {
            if ( (pssiSpiServerInfoCopy->pucAdminEmail = s_strdup(pssiSpiServerInfo->pucAdminEmail)) == NULL ) {
                iError = SPI_MemError;
                goto bailFromiSpiDuplicateServerInfo;
            }
        }

        pssiSpiServerInfoCopy->uiIndexCount = pssiSpiServerInfo->uiIndexCount;

        if ( pssiSpiServerInfo->pucRankingAlgorithm != NULL ) {
            if ( (pssiSpiServerInfoCopy->pucRankingAlgorithm = s_strdup(pssiSpiServerInfo->pucRankingAlgorithm)) == NULL ) {
                iError = SPI_MemError;
                goto bailFromiSpiDuplicateServerInfo;
            }
        }

        pssiSpiServerInfoCopy->dWeightMinimum = pssiSpiServerInfo->dWeightMinimum;
        pssiSpiServerInfoCopy->dWeightMaximum = pssiSpiServerInfo->dWeightMaximum;
    }
    else {
        iError = SPI_MiscError;
        goto bailFromiSpiDuplicateServerInfo;
    }



    /* Bail label */
    bailFromiSpiDuplicateServerInfo:


    /* Handle the error */
    if ( iError == SPI_NoError ) {
        *ppssiSpiServerInfo = pssiSpiServerInfoCopy;
    }
    else {
        iSpiFreeServerInfo(pssiSpiServerInfoCopy);
        pssiSpiServerInfoCopy = NULL;
    }


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSpiDuplicateServerIndexInfo

    Purpose:    This function duplicates an spi server index info structure array.

    Parameters: pssiiSpiServerIndexInfos        pointer to an array of spi server index info structures
                uiSpiServerIndexInfosLength     length of the spi server index info structure array
                ppssiiSpiServerIndexInfos       return pointer to a duplicate of an array of 
                                                spi server index info structures

    Globals:    none

    Returns:    SPI Error Code

*/
int iSpiDuplicateServerIndexInfo
(
    struct spiServerIndexInfo *pssiiSpiServerIndexInfos,
    unsigned int uiSpiServerIndexInfosLength,
    struct spiServerIndexInfo **ppssiiSpiServerIndexInfos
)
{

    unsigned int                iError = SPI_NoError;
    struct spiServerIndexInfo   *pssiiSpiServerIndexInfosCopy = NULL;
    struct spiServerIndexInfo   *pssiiSpiServerIndexInfosCopyPtr = NULL;
    struct spiServerIndexInfo   *pssiiSpiServerIndexInfosPtr = NULL;
    unsigned int                uiI = 0;


    /* Duplicate the structure and copy the components */
    if ( (pssiiSpiServerIndexInfos != NULL) && (uiSpiServerIndexInfosLength > 0) && (ppssiiSpiServerIndexInfos != NULL) ) {
        
        if ( (pssiiSpiServerIndexInfosCopy = (struct spiServerIndexInfo *)s_malloc((size_t)(uiSpiServerIndexInfosLength * sizeof(struct spiServerIndexInfo)))) == NULL ) {
            iError = SPI_MemError;
            goto bailFromiSpiDuplicateServerIndexInfo;
        }

        for ( uiI = 0, pssiiSpiServerIndexInfosPtr = pssiiSpiServerIndexInfos, pssiiSpiServerIndexInfosCopyPtr = pssiiSpiServerIndexInfosCopy; 
                uiI < uiSpiServerIndexInfosLength; uiI++, pssiiSpiServerIndexInfosPtr++, pssiiSpiServerIndexInfosCopyPtr++ ) {
    
            if ( (pssiiSpiServerIndexInfosCopyPtr->pucName = s_strdup(pssiiSpiServerIndexInfosPtr->pucName)) == NULL ) {
                iError = SPI_MemError;
                goto bailFromiSpiDuplicateServerIndexInfo;
            }

            if ( pssiiSpiServerIndexInfosCopyPtr->pucDescription != NULL ) {
                if ( (pssiiSpiServerIndexInfosCopyPtr->pucDescription = s_strdup(pssiiSpiServerIndexInfosPtr->pucDescription)) == NULL ) {
                    iError = SPI_MemError;
                    goto bailFromiSpiDuplicateServerIndexInfo;
                }
            }
        }
    }
    else {
        iError = SPI_MiscError;
        goto bailFromiSpiDuplicateServerIndexInfo;
    }



    /* Bail label */
    bailFromiSpiDuplicateServerIndexInfo:


    /* Handle the error */
    if ( iError == SPI_NoError ) {
        *ppssiiSpiServerIndexInfos = pssiiSpiServerIndexInfosCopy;
    }
    else {
        iSpiFreeServerIndexInfo(pssiiSpiServerIndexInfosCopy, uiSpiServerIndexInfosLength);
        pssiiSpiServerIndexInfosCopy = NULL;
    }


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSpiDuplicateIndexInfo

    Purpose:    This function duplicated an spi index info structure.

    Parameters: psiiSpiIndexInfo    pointer to an spi index info structure
                ppsiiSpiIndexInfo   return pointer to a duplicate of the spi index info structure

    Globals:    none

    Returns:    SPI Error Code

*/
int iSpiDuplicateIndexInfo
(
    struct spiIndexInfo *psiiSpiIndexInfo,
    struct spiIndexInfo **ppsiiSpiIndexInfo
)
{

    unsigned int            iError = SPI_NoError;
    struct spiIndexInfo     *psiiSpiIndexInfoCopy = NULL;


    /* Duplicate the structure and copy the components */
    if ( (psiiSpiIndexInfo != NULL) && (ppsiiSpiIndexInfo != NULL) ) {

        if ( (psiiSpiIndexInfoCopy = (struct spiIndexInfo *)s_malloc(sizeof(struct spiIndexInfo))) == NULL ) {
            iError = SPI_MemError;
            goto bailFromiSpiDuplicateIndexInfo;
        }
        
        if ( (psiiSpiIndexInfoCopy->pucName = s_strdup(psiiSpiIndexInfo->pucName)) == NULL ) {
            iError = SPI_MemError;
            goto bailFromiSpiDuplicateIndexInfo;
        }
        
        if ( psiiSpiIndexInfoCopy->pucDescription != NULL ) {
            if ( (psiiSpiIndexInfoCopy->pucDescription = s_strdup(psiiSpiIndexInfo->pucDescription)) == NULL ) {
                iError = SPI_MemError;
                goto bailFromiSpiDuplicateIndexInfo;
            }
        }
            
        if ( psiiSpiIndexInfoCopy->pucLanguageCode != NULL ) {
            if ( (psiiSpiIndexInfoCopy->pucLanguageCode = s_strdup(psiiSpiIndexInfo->pucLanguageCode)) == NULL ) {
                iError = SPI_MemError;
                goto bailFromiSpiDuplicateIndexInfo;
            }
        }
            
        if ( psiiSpiIndexInfoCopy->pucTokenizerName != NULL ) {
            if ( (psiiSpiIndexInfoCopy->pucTokenizerName = s_strdup(psiiSpiIndexInfo->pucTokenizerName)) == NULL ) {
                iError = SPI_MemError;
                goto bailFromiSpiDuplicateIndexInfo;
            }
        }
            
        if ( psiiSpiIndexInfoCopy->pucStemmerName != NULL ) {
            if ( (psiiSpiIndexInfoCopy->pucStemmerName = s_strdup(psiiSpiIndexInfo->pucStemmerName)) == NULL ) {
                iError = SPI_MemError;
                goto bailFromiSpiDuplicateIndexInfo;
            }
        }
            
        if ( psiiSpiIndexInfoCopy->pucStopListName != NULL ) {
            if ( (psiiSpiIndexInfoCopy->pucStopListName = s_strdup(psiiSpiIndexInfo->pucStopListName)) == NULL ) {
                iError = SPI_MemError;
                goto bailFromiSpiDuplicateIndexInfo;
            }
        }
        
        psiiSpiIndexInfoCopy->uiDocumentCount = psiiSpiIndexInfo->uiDocumentCount;
        psiiSpiIndexInfoCopy->ulUniqueTermCount = psiiSpiIndexInfo->ulUniqueTermCount;
        psiiSpiIndexInfoCopy->ulTotalTermCount = psiiSpiIndexInfo->ulTotalTermCount;
        psiiSpiIndexInfoCopy->ulUniqueStopTermCount = psiiSpiIndexInfo->ulUniqueStopTermCount;
        psiiSpiIndexInfoCopy->ulTotalStopTermCount = psiiSpiIndexInfo->ulTotalStopTermCount;
        psiiSpiIndexInfoCopy->uiAccessControl = psiiSpiIndexInfo->uiAccessControl;
        psiiSpiIndexInfoCopy->uiUpdateFrequency = psiiSpiIndexInfo->uiUpdateFrequency;
        psiiSpiIndexInfoCopy->ulLastUpdateAnsiDate = psiiSpiIndexInfo->ulLastUpdateAnsiDate;
        psiiSpiIndexInfoCopy->uiCaseSensitive = psiiSpiIndexInfo->uiCaseSensitive;
    }
    else {
        iError = SPI_MiscError;
        goto bailFromiSpiDuplicateIndexInfo;
    }



    /* Bail label */
    bailFromiSpiDuplicateIndexInfo:


    /* Handle the error */
    if ( iError == SPI_NoError ) {
        *ppsiiSpiIndexInfo = psiiSpiIndexInfoCopy;
    }
    else {
        iSpiFreeIndexInfo(psiiSpiIndexInfoCopy);
        psiiSpiIndexInfoCopy = NULL;
    }


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSpiDuplicateIndexFieldInfo

    Purpose:    This function duplicate an spi field info structure array.

    Parameters: psfiSpiFieldInfos       pointer to an array of spi field info structures
                uiSpiFieldInfosLength   length of the spi field info structure array
                ppsfiSpiFieldInfos      return pointer to a duplicate of an array of spi field info structures

    Globals:    none

    Returns:    SPI Error Code

*/
int iSpiDuplicateIndexFieldInfo
(
    struct spiFieldInfo *psfiSpiFieldInfos,
    unsigned int uiSpiFieldInfosLength,
    struct spiFieldInfo **ppsfiSpiFieldInfos
)
{

    unsigned int            iError = SPI_NoError;
    struct spiFieldInfo     *psfiSpiFieldInfosCopy = NULL;
    struct spiFieldInfo     *psfiSpiFieldInfosCopyPtr = NULL;
    struct spiFieldInfo     *psfiSpiFieldInfosPtr = NULL;
    unsigned int            uiI = 0;


    /* Duplicate the structure and copy the components */
    if ( (psfiSpiFieldInfos != NULL) && (uiSpiFieldInfosLength > 0) && (ppsfiSpiFieldInfos != NULL) ) {

        if ( (psfiSpiFieldInfosCopy = (struct spiFieldInfo *)s_malloc((size_t)(uiSpiFieldInfosLength * sizeof(struct spiFieldInfo)))) == NULL ) {
            iError = SPI_MemError;
            goto bailFromiSpiDuplicateIndexFieldInfo;
        }
        
        for ( uiI = 0, psfiSpiFieldInfosPtr = psfiSpiFieldInfos, psfiSpiFieldInfosCopyPtr = psfiSpiFieldInfosCopy; 
                uiI < uiSpiFieldInfosLength; uiI++, psfiSpiFieldInfosPtr++, psfiSpiFieldInfosCopyPtr++ ) {

            if ( (psfiSpiFieldInfosCopyPtr->pucName = s_strdup(psfiSpiFieldInfosPtr->pucName)) == NULL ) {
                iError = SPI_MemError;
                goto bailFromiSpiDuplicateIndexFieldInfo;
            }

            if ( psfiSpiFieldInfosCopyPtr->pucDescription != NULL ) {
                if ( (psfiSpiFieldInfosCopyPtr->pucDescription = s_strdup(psfiSpiFieldInfosPtr->pucDescription)) == NULL ) {
                    iError = SPI_MemError;
                    goto bailFromiSpiDuplicateIndexFieldInfo;
                }
            }
            
            psfiSpiFieldInfosCopyPtr->uiType = psfiSpiFieldInfosPtr->uiType;
        }
    }
    else {
        iError = SPI_MiscError;
        goto bailFromiSpiDuplicateIndexFieldInfo;
    }



    /* Bail label */
    bailFromiSpiDuplicateIndexFieldInfo:


    /* Handle the error */
    if ( iError == SPI_NoError ) {
        *ppsfiSpiFieldInfos = psfiSpiFieldInfosCopy;
    }
    else {
        iSpiFreeIndexFieldInfo(psfiSpiFieldInfosCopy, uiSpiFieldInfosLength);
        psfiSpiFieldInfosCopy = NULL;
    }


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSpiDuplicateTermInfo

    Purpose:    This function duplicates an spi term info structure array.

    Parameters: pstiSpiTermInfos        pointer to an array of spi term info structures
                uiSpiTermInfosLength    length of the spi term info structure array
                ppstiSpiTermInfos       return pointer to a duplicate of an array of spi term info structures

    Globals:    none

    Returns:    SPI Error Code

*/
int iSpiDuplicateTermInfo
(
    struct spiTermInfo *pstiSpiTermInfos,
    unsigned int uiSpiTermInfosLength,
    struct spiTermInfo **ppstiSpiTermInfos
)
{

    unsigned int            iError = SPI_NoError;
    struct spiTermInfo      *pstiSpiTermInfosCopy = NULL;
    struct spiTermInfo      *pstiSpiTermInfosCopyPtr = NULL;
    struct spiTermInfo      *pstiSpiTermInfosPtr = NULL;
    unsigned int            uiI = 0;


    /* Duplicate the structure and copy the components */
    if ( (pstiSpiTermInfos != NULL) && (uiSpiTermInfosLength > 0) && (ppstiSpiTermInfos != NULL) ) {
    
        if ( (pstiSpiTermInfosCopy = (struct spiTermInfo *)s_malloc((size_t)(uiSpiTermInfosLength * sizeof(struct spiTermInfo)))) == NULL ) {
            iError = SPI_MemError;
            goto bailFromiSpiDuplicateTermInfo;
        }

        for ( uiI = 0, pstiSpiTermInfosPtr = pstiSpiTermInfos, pstiSpiTermInfosCopyPtr = pstiSpiTermInfosCopy; 
                uiI < uiSpiTermInfosLength; uiI++, pstiSpiTermInfosPtr++, pstiSpiTermInfosCopyPtr++ ) {

            if ( (pstiSpiTermInfosCopyPtr->pucTerm = s_strdup(pstiSpiTermInfosPtr->pucTerm)) == NULL ) {
                iError = SPI_MemError;
                goto bailFromiSpiDuplicateTermInfo;
            }
            
            pstiSpiTermInfosCopyPtr->uiType = pstiSpiTermInfosPtr->uiType;
            pstiSpiTermInfosCopyPtr->uiCount = pstiSpiTermInfosPtr->uiCount;
            pstiSpiTermInfosCopyPtr->uiDocumentCount = pstiSpiTermInfosPtr->uiDocumentCount;
        }
    }
    else {
        iError = SPI_MiscError;
        goto bailFromiSpiDuplicateTermInfo;
    }



    /* Bail label */
    bailFromiSpiDuplicateTermInfo:


    /* Handle the error */
    if ( iError == SPI_NoError ) {
        *ppstiSpiTermInfos = pstiSpiTermInfosCopy;
    }
    else {
        iSpiFreeTermInfo(pstiSpiTermInfosCopy, uiSpiTermInfosLength);
        pstiSpiTermInfosCopy = NULL;
    }


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSpiDuplicateDocumentInfo

    Purpose:    This function duplicate an spi document info structure.

    Parameters: psdiSpiDocumentInfo     pointer to an spi document info structure
                ppsdiSpiDocumentInfo    return pointer to a duplicate of an spi document info structure

    Globals:    none

    Returns:    SPI Error Code

*/
int iSpiDuplicateDocumentInfo
(
    struct spiDocumentInfo *psdiSpiDocumentInfo,
    struct spiDocumentInfo **ppsdiSpiDocumentInfo
)
{

    unsigned int                iError = SPI_NoError;
    struct spiDocumentInfo      *psdiSpiDocumentInfoCopy = NULL;
    struct spiDocumentItem      *psdiSpiDocumentItemsCopy = NULL;
    struct spiDocumentItem      *psdiSpiDocumentItemsCopyPtr = NULL;
    struct spiDocumentItem      *psdiSpiDocumentItemsPtr = NULL;
    unsigned int                uiI = 0;


    if ( (psdiSpiDocumentInfo != NULL) && (ppsdiSpiDocumentInfo != NULL) ) {

        if ( (psdiSpiDocumentInfoCopy = (struct spiDocumentInfo *)s_malloc(sizeof(struct spiDocumentInfo))) == NULL ) {
            iError = SPI_MemError;
            goto bailFromiSpiDuplicateDocumentInfo;
        }

        if ( (psdiSpiDocumentInfoCopy->pucIndexName = s_strdup(psdiSpiDocumentInfo->pucIndexName)) == NULL ) {
            iError = SPI_MemError;
            goto bailFromiSpiDuplicateDocumentInfo;
        }

        if ( (psdiSpiDocumentInfoCopy->pucDocumentKey = s_strdup(psdiSpiDocumentInfo->pucDocumentKey)) == NULL ) {
            iError = SPI_MemError;
            goto bailFromiSpiDuplicateDocumentInfo;
        }

        if ( (psdiSpiDocumentInfoCopy->pucTitle = s_strdup(psdiSpiDocumentInfo->pucTitle)) == NULL ) {
            iError = SPI_MemError;
            goto bailFromiSpiDuplicateDocumentInfo;
        }

        if ( psdiSpiDocumentInfo->pucLanguageCode != NULL ) {
            if ( (psdiSpiDocumentInfoCopy->pucLanguageCode = s_strdup(psdiSpiDocumentInfo->pucLanguageCode)) == NULL ) {
                iError = SPI_MemError;
                goto bailFromiSpiDuplicateDocumentInfo;
            }
        }

        if ( (psdiSpiDocumentInfo->psdiSpiDocumentItems != NULL) && (psdiSpiDocumentInfo->uiDocumentItemsLength > 0) ) {

            if ( (psdiSpiDocumentItemsCopy = (struct spiDocumentItem *)s_malloc((size_t)(psdiSpiDocumentInfo->uiDocumentItemsLength * sizeof(struct spiDocumentItem)))) == NULL ) {
                iError = SPI_MemError;
                goto bailFromiSpiDuplicateDocumentInfo;
            }

            psdiSpiDocumentInfoCopy->psdiSpiDocumentItems = psdiSpiDocumentItemsCopy;
            psdiSpiDocumentInfoCopy->uiDocumentItemsLength = psdiSpiDocumentInfo->uiDocumentItemsLength;

            for ( uiI = 0, psdiSpiDocumentItemsPtr = psdiSpiDocumentInfo->psdiSpiDocumentItems, psdiSpiDocumentItemsCopyPtr = psdiSpiDocumentInfoCopy->psdiSpiDocumentItems; 
                    uiI < psdiSpiDocumentInfo->uiDocumentItemsLength; uiI++, psdiSpiDocumentItemsPtr++, psdiSpiDocumentItemsCopyPtr++ ) {
            
                if ( (psdiSpiDocumentItemsCopyPtr->pucItemName = s_strdup(psdiSpiDocumentItemsPtr->pucItemName)) == NULL ) {
                    iError = SPI_MemError;
                    goto bailFromiSpiDuplicateDocumentInfo;
                }

                if ( (psdiSpiDocumentItemsCopyPtr->pucMimeType = s_strdup(psdiSpiDocumentItemsPtr->pucMimeType)) == NULL ) {
                    iError = SPI_MemError;
                    goto bailFromiSpiDuplicateDocumentInfo;
                }
                
                psdiSpiDocumentItemsCopyPtr->uiLength = psdiSpiDocumentItemsPtr->uiLength;

                if ( psdiSpiDocumentItemsCopyPtr->pucUrl != NULL ) {
                    if ( (psdiSpiDocumentItemsCopyPtr->pucUrl = s_strdup(psdiSpiDocumentItemsPtr->pucUrl)) == NULL ) {
                        iError = SPI_MemError;
                        goto bailFromiSpiDuplicateDocumentInfo;
                    }
                }
                
                if ( psdiSpiDocumentItemsCopyPtr->pvData != NULL ) {
                
                    if ( (psdiSpiDocumentItemsCopyPtr->pvData = (void *)s_malloc((size_t)(psdiSpiDocumentItemsPtr->uiDataLength * sizeof(unsigned char)))) == NULL ) {
                        iError = SPI_MemError;
                        goto bailFromiSpiDuplicateDocumentInfo;
                    }
                    
                    s_memcpy(psdiSpiDocumentItemsCopyPtr->pvData, psdiSpiDocumentItemsPtr->pvData, psdiSpiDocumentItemsPtr->uiDataLength);
                    
                    psdiSpiDocumentItemsCopyPtr->uiDataLength = psdiSpiDocumentItemsPtr->uiDataLength;
                }
            }
        }
        
        psdiSpiDocumentInfoCopy->uiRank = psdiSpiDocumentInfo->uiRank;
        psdiSpiDocumentInfoCopy->uiTermCount = psdiSpiDocumentInfo->uiTermCount;
        psdiSpiDocumentInfoCopy->ulAnsiDate = psdiSpiDocumentInfo->ulAnsiDate;
    }
    else {
        iError = SPI_MiscError;
        goto bailFromiSpiDuplicateDocumentInfo;
    }



    /* Bail label */
    bailFromiSpiDuplicateDocumentInfo:


    /* Handle the error */
    if ( iError == SPI_NoError ) {
        *ppsdiSpiDocumentInfo = psdiSpiDocumentInfoCopy;
    }
    else {
        iSpiFreeDocumentInfo(psdiSpiDocumentInfoCopy);
        psdiSpiDocumentInfoCopy = NULL;
    }


    return (iError);

}


/*---------------------------------------------------------------------------*/


/* 
** =====================================
** ===  Structure Freeing Functions  ===
** =====================================
*/


/*

    Function:   iSpiFreeSession

    Purpose:    This function frees an spi session structure.

    Parameters: pssSpiSession   pointer to an spi session structure

    Globals:    none

    Returns:    SPI Error Code

*/
int iSpiFreeSession
(
    struct spiSession *pssSpiSession
)
{

    if ( pssSpiSession != NULL ) {
        s_free(pssSpiSession->pucIndexDirectoryPath);
        s_free(pssSpiSession->pucConfigurationDirectoryPath);
        s_free(pssSpiSession->pucTemporaryDirectoryPath);
        s_free(pssSpiSession);
    }


    return (SPI_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSpiFreeServerInfo

    Purpose:    This function frees an spi server info structure.

    Parameters: pssiSpiServerInfo   pointer to an spi server info structure

    Globals:    none

    Returns:    SPI Error Code

*/
int iSpiFreeServerInfo
(
    struct spiServerInfo *pssiSpiServerInfo
)
{

    if ( pssiSpiServerInfo != NULL ) {
        s_free(pssiSpiServerInfo->pucName);
        s_free(pssiSpiServerInfo->pucDescription);
        s_free(pssiSpiServerInfo->pucAdminName);
        s_free(pssiSpiServerInfo->pucAdminEmail);
        s_free(pssiSpiServerInfo->pucRankingAlgorithm);
        s_free(pssiSpiServerInfo);
    }


    return (SPI_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSpiFreeServerIndexInfo

    Purpose:    This function frees an spi server index info structure array.

    Parameters: pssiiSpiServerIndexInfos        pointer to an array of spi server index info structures
                uiSpiServerIndexInfosLength     length of the spi server index info structure array

    Globals:    none

    Returns:    SPI Error Code

*/
int iSpiFreeServerIndexInfo
(
    struct spiServerIndexInfo *pssiiSpiServerIndexInfos,
    unsigned int uiSpiServerIndexInfosLength
)
{

    struct spiServerIndexInfo   *pssiiSpiServerIndexInfosPtr = NULL;
    unsigned int                uiI = 0;


    if ( (pssiiSpiServerIndexInfos != NULL) && (uiSpiServerIndexInfosLength > 0) ) {
        
        for ( uiI = 0, pssiiSpiServerIndexInfosPtr = pssiiSpiServerIndexInfos; uiI < uiSpiServerIndexInfosLength; uiI++, pssiiSpiServerIndexInfosPtr++ ) {

            s_free(pssiiSpiServerIndexInfosPtr->pucName);
            s_free(pssiiSpiServerIndexInfosPtr->pucDescription);
        }

        s_free(pssiiSpiServerIndexInfos);
    }


    return (SPI_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSpiFreeIndexInfo

    Purpose:    This function frees an spi index info structure.

    Parameters: psiiSpiIndexInfo    pointer to an spi index info structure

    Globals:    none

    Returns:    SPI Error Code

*/
int iSpiFreeIndexInfo
(
    struct spiIndexInfo *psiiSpiIndexInfo
)
{

    if ( psiiSpiIndexInfo != NULL ) {
        s_free(psiiSpiIndexInfo->pucName);
        s_free(psiiSpiIndexInfo->pucDescription);
        s_free(psiiSpiIndexInfo->pucLanguageCode);
        s_free(psiiSpiIndexInfo->pucTokenizerName);
        s_free(psiiSpiIndexInfo->pucStemmerName);
        s_free(psiiSpiIndexInfo->pucStopListName);
        s_free(psiiSpiIndexInfo);
    }


    return (SPI_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSpiFreeIndexFieldInfo

    Purpose:    This function frees an spi field info structure array.

    Parameters: psfiSpiFieldInfos       pointer to an array of spi field info structures
                uiSpiFieldInfosLength   length of the spi field info structure array

    Globals:    none

    Returns:    SPI Error Code

*/
int iSpiFreeIndexFieldInfo
(
    struct spiFieldInfo *psfiSpiFieldInfos,
    unsigned int uiSpiFieldInfosLength
)
{

    struct spiFieldInfo     *psfiSpiFieldInfosPtr = NULL;
    unsigned int            uiI = 0;


    if ( (psfiSpiFieldInfos != NULL) && (uiSpiFieldInfosLength > 0) ) {

        for ( uiI = 0, psfiSpiFieldInfosPtr = psfiSpiFieldInfos; uiI < uiSpiFieldInfosLength; uiI++, psfiSpiFieldInfosPtr++ ) {

            s_free(psfiSpiFieldInfosPtr->pucName);
            s_free(psfiSpiFieldInfosPtr->pucDescription);
        }

        s_free(psfiSpiFieldInfos);
    }


    return (SPI_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSpiFreeTermInfo

    Purpose:    This function frees an spi term info structure array.

    Parameters: pstiSpiTermInfos        pointer to an array of spi term info structures
                uiSpiTermInfosLength    length of the spi term info structure array

    Globals:    none

    Returns:    SPI Error Code

*/
int iSpiFreeTermInfo
(
    struct spiTermInfo *pstiSpiTermInfos,
    unsigned int uiSpiTermInfosLength
)
{

    struct spiTermInfo      *pstiSpiTermInfosPtr = NULL;
    unsigned int            uiI = 0;


    if ( (pstiSpiTermInfos != NULL) && (uiSpiTermInfosLength > 0) ) {
    
        for ( uiI = 0, pstiSpiTermInfosPtr = pstiSpiTermInfos; uiI < uiSpiTermInfosLength; uiI++, pstiSpiTermInfosPtr++ ) {

            s_free(pstiSpiTermInfosPtr->pucTerm);
        }

        s_free(pstiSpiTermInfos);
    }


    return (SPI_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSpiFreeDocumentInfo

    Purpose:    This function frees an spi document info structure.

    Parameters: psdiSpiDocumentInfo     pointer to an spi document info structure

    Globals:    none

    Returns:    SPI Error Code

*/
int iSpiFreeDocumentInfo
(
    struct spiDocumentInfo *psdiSpiDocumentInfo
)
{

    struct spiDocumentItem      *psdiSpiDocumentItemsPtr = NULL;
    unsigned int                uiI = 0;


    if ( psdiSpiDocumentInfo != NULL ) {

        if ( (psdiSpiDocumentInfo->psdiSpiDocumentItems != NULL) && (psdiSpiDocumentInfo->uiDocumentItemsLength > 0) ) {

            for ( uiI = 0, psdiSpiDocumentItemsPtr = psdiSpiDocumentInfo->psdiSpiDocumentItems; uiI < psdiSpiDocumentInfo->uiDocumentItemsLength; uiI++, psdiSpiDocumentItemsPtr++ ) {
            
                s_free(psdiSpiDocumentItemsPtr->pucItemName);
                s_free(psdiSpiDocumentItemsPtr->pucUrl);
                s_free(psdiSpiDocumentItemsPtr->pvData);
            }
            
            s_free(psdiSpiDocumentInfo->psdiSpiDocumentItems);
        }
        
        s_free(psdiSpiDocumentInfo->pucIndexName);
        s_free(psdiSpiDocumentInfo->pucDocumentKey);
        s_free(psdiSpiDocumentInfo->pucTitle);
        s_free(psdiSpiDocumentInfo->pucLanguageCode);
        s_free(psdiSpiDocumentInfo);
    }


    return (SPI_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSpiFreeSearchResultComponents

    Purpose:    This function frees an spi search result structure components, but not
                the search result itself.

    Parameters: pssrSpiSearchResult     pointer to a spi search result structure
                uiSortType              sort type 

    Globals:    none

    Returns:    SPI Error Code

*/
int iSpiFreeSearchResultComponents
(
    struct spiSearchResult *pssrSpiSearchResult,
    unsigned int uiSortType
)
{

    struct spiDocumentItem      *psdiSpiDocumentItemsPtr = NULL;
    unsigned int                uiI = 0;


    if ( pssrSpiSearchResult != NULL ) {

        if ( (pssrSpiSearchResult->psdiSpiDocumentItems != NULL) && (pssrSpiSearchResult->uiDocumentItemsLength > 0) ) {
            
            for ( uiI = 0, psdiSpiDocumentItemsPtr = pssrSpiSearchResult->psdiSpiDocumentItems; uiI < pssrSpiSearchResult->uiDocumentItemsLength; uiI++, psdiSpiDocumentItemsPtr++ ) {
                
                s_free(psdiSpiDocumentItemsPtr->pucItemName);
                s_free(psdiSpiDocumentItemsPtr->pucMimeType);
                s_free(psdiSpiDocumentItemsPtr->pucUrl);
                s_free(psdiSpiDocumentItemsPtr->pvData);
            }
        }
    
        s_free(pssrSpiSearchResult->pucIndexName);
        s_free(pssrSpiSearchResult->pucDocumentKey);
        s_free(pssrSpiSearchResult->pucTitle);
        s_free(pssrSpiSearchResult->pucLanguageCode);
        s_free(pssrSpiSearchResult->psdiSpiDocumentItems);
    
        if ( (uiSortType == SPI_SORT_TYPE_UCHAR_ASC) || (uiSortType == SPI_SORT_TYPE_UCHAR_DESC) ) {
            s_free(pssrSpiSearchResult->pucSortKey);
        }
    }    


    return (SPI_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSpiFreeSearchResult

    Purpose:    This function frees an spi search result structure.

    Parameters: pssrSpiSearchResult     pointer to a spi search result structure
                uiSortType              sort type 

    Globals:    none

    Returns:    SPI Error Code

*/
int iSpiFreeSearchResult
(
    struct spiSearchResult *pssrSpiSearchResult,
    unsigned int uiSortType
)
{

    if ( pssrSpiSearchResult != NULL ) {
        iSpiFreeSearchResultComponents(pssrSpiSearchResult, uiSortType);
        s_free(pssrSpiSearchResult);
    }    


    return (SPI_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSpiFreeSearchResults

    Purpose:    This function frees an spi search results array.

    Parameters: pssrSpiSearchResults        pointer to an array of spi search results array
                uiSpiSearchResultsLength    length of the spi search results array
                uiSortType                  sort type 

    Globals:    none

    Returns:    SPI Error Code

*/
int iSpiFreeSearchResults
(
    struct spiSearchResult *pssrSpiSearchResults,
    unsigned int uiSpiSearchResultsLength,
    unsigned int uiSortType
)
{

    struct spiSearchResult      *pshSearchResultsPtr = NULL;
    unsigned int                uiI = 0;


    if ( pssrSpiSearchResults != NULL ) {
    
        if ( uiSpiSearchResultsLength > 0 ) {

            for ( uiI = 0, pshSearchResultsPtr = pssrSpiSearchResults; uiI < uiSpiSearchResultsLength; uiI++, pshSearchResultsPtr++ ) {
                iSpiFreeSearchResultComponents(pshSearchResultsPtr, uiSortType);
            }
        }

        s_free(pssrSpiSearchResults);
    }



    return (SPI_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSpiFreeSearchResponse

    Purpose:    This function frees a spi search response structure.

    Parameters: pssrSpiSearchResponse   pointer to a spi search response structure

    Globals:    none

    Returns:    SPI Error Code

*/
int iSpiFreeSearchResponse
(
    struct spiSearchResponse *pssrSpiSearchResponse
)
{

    if ( pssrSpiSearchResponse != NULL ) {
    
        iSpiFreeSearchResults(pssrSpiSearchResponse->pssrSpiSearchResults, pssrSpiSearchResponse->uiSpiSearchResultsLength, pssrSpiSearchResponse->uiSortType);

        s_free(pssrSpiSearchResponse);
    }


    return (SPI_NoError);

}


/*---------------------------------------------------------------------------*/


/* 
** ===========================
** ===  Splicing Function  ===
** ===========================
*/


/*

    Function:   iSpiSpliceSearchResults

    Purpose:    This function splices an spi search results array.

    Parameters: ppssrSpiSearchResults           pass and return pointer for an spi search results array
                puiSpiSearchResultsLength       pass and return pointer for the length of the spi search results array
                uiSpiSearchResultsStartIndex    search results start index
                uiSpiSearchResultsEndIndex      search results end index
                uiSortType                      sort type 

    Globals:    none

    Returns:    SPI Error Code

*/
int iSpiSpliceSearchResults
(
    struct spiSearchResult **ppssrSpiSearchResults,
    unsigned int *puiSpiSearchResultsLength,
    unsigned int uiSpiSearchResultsStartIndex, 
    unsigned int uiSpiSearchResultsEndIndex,
    unsigned int uiSortType
)
{


    struct spiSearchResult      *pssrSpiSearchResults = NULL;
    unsigned int                uiSpiSearchResultsLength = 0;


    /* Check the parameters */
    if ( ppssrSpiSearchResults == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppssrSpiSearchResults' parameter passed to 'iSpiSpliceSearchResults'."); 
        return (SPI_ParameterError);
    }

    if ( puiSpiSearchResultsLength == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'puiSrchShortResultsLength' parameter passed to 'iSpiSpliceSearchResults'."); 
        return (SPI_ParameterError);
    }

    if ( *puiSpiSearchResultsLength < 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'puiSrchShortResultsLength' parameter passed to 'iSpiSpliceSearchResults'."); 
        return (SPI_ParameterError);
    }

    if ( uiSpiSearchResultsStartIndex < 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiSpiSearchResultsStartIndex' parameter passed to 'iSpiSpliceSearchResults'."); 
        return (SPI_ParameterError);
    }

    if ( uiSpiSearchResultsEndIndex < 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiSpiSearchResultsEndIndex' parameter passed to 'iSpiSpliceSearchResults'."); 
        return (SPI_ParameterError);
    }

    if ( uiSpiSearchResultsStartIndex > uiSpiSearchResultsEndIndex ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiSpiSearchResultsStartIndex' & 'uiSpiSearchResultsEndIndex' parameters passed to 'iSpiSpliceSearchResults'."); 
        return (SPI_ParameterError);
    }

    if ( SPI_SORT_TYPE_VALID(uiSortType) == false ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiSortOrder' parameter passed to 'iSpiSpliceSearchResults'."); 
        return (SPI_ParameterError);
    }



    /* Set our working pointer */
    pssrSpiSearchResults = *ppssrSpiSearchResults;
    uiSpiSearchResultsLength = *puiSpiSearchResultsLength;


    /* Do we need to splice out the results? */
    if ( (uiSpiSearchResultsStartIndex > 0) || ((uiSpiSearchResultsEndIndex > 0) && (uiSpiSearchResultsLength > (uiSpiSearchResultsEndIndex + 1))) ) {
        
        struct spiSearchResult      *pssrSpiSearchResultsPtr = NULL;
        struct spiSearchResult      *pssrSpiSearchResultsCopyToPtr = NULL;
        struct spiSearchResult      *pssrSpiSearchResultsCopyFromPtr = NULL;
        unsigned int                uiSpiSearchReportCount = 0;
        unsigned int                uiSpiSearchResultsActualStartIndex = 0;
        unsigned int                uiSpiSearchResultsActualEndIndex = 0;
        

        /* Count up the search reports */
        for ( pssrSpiSearchResultsPtr = pssrSpiSearchResults; pssrSpiSearchResultsPtr < (pssrSpiSearchResults + uiSpiSearchResultsLength); pssrSpiSearchResultsPtr++ ) {

            if ( (pssrSpiSearchResultsPtr->psdiSpiDocumentItems != NULL) && 
                    (s_strcmp(pssrSpiSearchResultsPtr->psdiSpiDocumentItems->pucItemName, SPI_SEARCH_REPORT_ITEM_NAME) == 0) &&
                    (s_strcmp(pssrSpiSearchResultsPtr->psdiSpiDocumentItems->pucMimeType, SPI_SEARCH_REPORT_MIME_TYPE) == 0) ) {

                uiSpiSearchReportCount++;
            }
        }


        /* Do we need to set the actual start index? */
        if ( uiSpiSearchResultsStartIndex > 0 ) {
            /* Set the actual start index to the start index, or to include any search reports */
            uiSpiSearchResultsActualStartIndex = (uiSpiSearchResultsStartIndex > (uiSpiSearchResultsLength - uiSpiSearchReportCount)) ? 
                    (uiSpiSearchResultsLength - uiSpiSearchReportCount) : uiSpiSearchResultsStartIndex;
        }


        /* Set the actual end index (note that we need to exclude the search report count from the search result length) */
        if ( (uiSpiSearchResultsEndIndex > 0) && ((uiSpiSearchResultsLength - uiSpiSearchReportCount) > (uiSpiSearchResultsEndIndex + 1)) ) {
            
            struct spiSearchResult      *pssrSpiSearchResultsOuterPtr = NULL;
            struct spiSearchResult      *pssrSpiSearchResultsInnerPtr = NULL;

            /* Set the actual end index, we need to include the search reports */
            uiSpiSearchResultsActualEndIndex = uiSpiSearchResultsEndIndex + uiSpiSearchReportCount;

            /* Shuffle up the search reports to the returned part of the search results */

            /* The outer loop works from the end of the search results we got, we only cover the results we are not returning */
            for ( pssrSpiSearchResultsOuterPtr = pssrSpiSearchResults + (uiSpiSearchResultsLength - 1), pssrSpiSearchResultsInnerPtr = pssrSpiSearchResults + uiSpiSearchResultsActualEndIndex;
                    pssrSpiSearchResultsOuterPtr > pssrSpiSearchResults + uiSpiSearchResultsActualEndIndex; pssrSpiSearchResultsOuterPtr-- ) {

                /* Have we hit a search report? */
                if ( (pssrSpiSearchResultsOuterPtr->psdiSpiDocumentItems != NULL) && 
                        (s_strcmp(pssrSpiSearchResultsOuterPtr->psdiSpiDocumentItems->pucItemName, SPI_SEARCH_REPORT_ITEM_NAME) == 0) &&
                        (s_strcmp(pssrSpiSearchResultsOuterPtr->psdiSpiDocumentItems->pucMimeType, SPI_SEARCH_REPORT_MIME_TYPE) == 0) ) {

                    /* The inner loop works from the end of the search results we are returning, we are moving back to the start */
                    for ( ; pssrSpiSearchResultsInnerPtr > pssrSpiSearchResults; pssrSpiSearchResultsInnerPtr-- ) {

                        /* Check that we dont over-write a search report */
                        if ( !((pssrSpiSearchResultsInnerPtr->psdiSpiDocumentItems != NULL) && 
                                (s_strcmp(pssrSpiSearchResultsInnerPtr->psdiSpiDocumentItems->pucItemName, SPI_SEARCH_REPORT_ITEM_NAME) == 0) &&
                                (s_strcmp(pssrSpiSearchResultsInnerPtr->psdiSpiDocumentItems->pucMimeType, SPI_SEARCH_REPORT_MIME_TYPE) == 0)) ) {


                            /* Swap the two search results */
                            switch ( uiSortType ) {

                                case SPI_SORT_TYPE_DOUBLE_ASC:
                                case SPI_SORT_TYPE_DOUBLE_DESC:
                                    SPI_SWAP_SEARCH_RESULT_DOUBLE(pssrSpiSearchResultsOuterPtr, pssrSpiSearchResultsInnerPtr);
                                    break;
            
                                case SPI_SORT_TYPE_FLOAT_ASC:
                                case SPI_SORT_TYPE_FLOAT_DESC:
                                    SPI_SWAP_SEARCH_RESULT_FLOAT(pssrSpiSearchResultsOuterPtr, pssrSpiSearchResultsInnerPtr);
                                    break;
                                
                                case SPI_SORT_TYPE_UINT_ASC:
                                case SPI_SORT_TYPE_UINT_DESC:
                                    SPI_SWAP_SEARCH_RESULT_UINT(pssrSpiSearchResultsOuterPtr, pssrSpiSearchResultsInnerPtr);
                                    break;
                                
                                case SPI_SORT_TYPE_ULONG_ASC:
                                case SPI_SORT_TYPE_ULONG_DESC:
                                    SPI_SWAP_SEARCH_RESULT_ULLONG(pssrSpiSearchResultsOuterPtr, pssrSpiSearchResultsInnerPtr);
                                    break;
                                
                                case SPI_SORT_TYPE_UCHAR_ASC:
                                case SPI_SORT_TYPE_UCHAR_DESC:
                                    SPI_SWAP_SEARCH_RESULT_UCHAR(pssrSpiSearchResultsOuterPtr, pssrSpiSearchResultsInnerPtr);
                                    break;
                                
                                case SPI_SORT_TYPE_NO_SORT:
                                case SPI_SORT_TYPE_UNKNOWN:
                                    SPI_SWAP_SEARCH_RESULT(pssrSpiSearchResultsOuterPtr, pssrSpiSearchResultsInnerPtr);
                                    break;
                                
                                default:
                                    ASSERT(false);
                                    break;
                            }
                            break;
                        }
                    }
                }
            }
        }
        else {
            uiSpiSearchResultsActualEndIndex = (uiSpiSearchResultsLength - 1);
        }
        

        /* Check that all went well */
        ASSERT((uiSpiSearchResultsActualStartIndex > 0) || (uiSpiSearchResultsActualEndIndex > 0));


        /* Free search results that are before the start of the splice */
        if ( uiSpiSearchResultsActualStartIndex > 0 ) {
            for ( pssrSpiSearchResultsPtr = pssrSpiSearchResults; pssrSpiSearchResultsPtr < (pssrSpiSearchResults + uiSpiSearchResultsActualStartIndex); pssrSpiSearchResultsPtr++ ) {
                iSpiFreeSearchResultComponents(pssrSpiSearchResultsPtr, uiSortType);
                pssrSpiSearchResultsPtr = NULL;                
            }
        }

        /* Free search results that are beyond the end of the splice */
        if ( uiSpiSearchResultsLength > (uiSpiSearchResultsActualEndIndex + 1) ) {
            for ( pssrSpiSearchResultsPtr = pssrSpiSearchResults + (uiSpiSearchResultsActualEndIndex + 1); pssrSpiSearchResultsPtr < (pssrSpiSearchResults + uiSpiSearchResultsLength); pssrSpiSearchResultsPtr++ ) {
                iSpiFreeSearchResultComponents(pssrSpiSearchResultsPtr, uiSortType);
                pssrSpiSearchResultsPtr = NULL;
            }
        }

        /* Calculate the actual number of search results we want to splice out */
        uiSpiSearchResultsLength = (uiSpiSearchResultsActualEndIndex - uiSpiSearchResultsActualStartIndex) + 1;

        /* Splice out the search results, this may mean that we shuffle them up the array */
        if ( (uiSpiSearchResultsLength > 0) && (uiSpiSearchResultsActualStartIndex > 0) ) {
        
            /* Shuffle up */
            for ( pssrSpiSearchResultsCopyToPtr = pssrSpiSearchResults, pssrSpiSearchResultsCopyFromPtr = pssrSpiSearchResults + uiSpiSearchResultsActualStartIndex;
                    pssrSpiSearchResultsCopyFromPtr < pssrSpiSearchResults + (uiSpiSearchResultsActualStartIndex + uiSpiSearchResultsLength);
                    pssrSpiSearchResultsCopyToPtr++, pssrSpiSearchResultsCopyFromPtr++) {
                    
                /* Copy the search result */
                switch ( uiSortType ) {

                    case SPI_SORT_TYPE_DOUBLE_ASC:
                    case SPI_SORT_TYPE_DOUBLE_DESC:
                        SPI_COPY_SEARCH_RESULT_DOUBLE(pssrSpiSearchResultsCopyToPtr, pssrSpiSearchResultsCopyFromPtr);
                        break;

                    case SPI_SORT_TYPE_FLOAT_ASC:
                    case SPI_SORT_TYPE_FLOAT_DESC:
                        SPI_COPY_SEARCH_RESULT_FLOAT(pssrSpiSearchResultsCopyToPtr, pssrSpiSearchResultsCopyFromPtr);
                        break;
                    
                    case SPI_SORT_TYPE_UINT_ASC:
                    case SPI_SORT_TYPE_UINT_DESC:
                        SPI_COPY_SEARCH_RESULT_UINT(pssrSpiSearchResultsCopyToPtr, pssrSpiSearchResultsCopyFromPtr);
                        break;
                    
                    case SPI_SORT_TYPE_ULONG_ASC:
                    case SPI_SORT_TYPE_ULONG_DESC:
                        SPI_COPY_SEARCH_RESULT_ULLONG(pssrSpiSearchResultsCopyToPtr, pssrSpiSearchResultsCopyFromPtr);
                        break;
                    
                    case SPI_SORT_TYPE_UCHAR_ASC:
                    case SPI_SORT_TYPE_UCHAR_DESC:
                        SPI_COPY_SEARCH_RESULT_UCHAR(pssrSpiSearchResultsCopyToPtr, pssrSpiSearchResultsCopyFromPtr);
                        break;
                    
                    case SPI_SORT_TYPE_NO_SORT:
                    case SPI_SORT_TYPE_UNKNOWN:
                        SPI_COPY_SEARCH_RESULT(pssrSpiSearchResultsCopyToPtr, pssrSpiSearchResultsCopyFromPtr);
                        break;
                    
                    default:
                        ASSERT(false);
                        break;
                }
            }
        }
    }


    /* Release the short results if the final number was 0 */
    if ( uiSpiSearchResultsLength == 0 ) {
        /* Free the search results pointer, all data was already freed above */
        s_free(pssrSpiSearchResults);
    }


    /* Set the return pointers */
    *ppssrSpiSearchResults = pssrSpiSearchResults;
    *puiSpiSearchResultsLength = uiSpiSearchResultsLength;


    return (SPI_NoError);

}


/*---------------------------------------------------------------------------*/


/* 
** ===========================
** ===  Sorting Functions  ===
** ===========================
*/


/*

    Function:   iSpiSortSearchResults()

    Purpose:    This function sorts a search results array.

    Parameters: pssrSpiSearchResults        pointer to an array of spi search results array
                uiSpiSearchResultsLength    length of the spi search results array
                uiSortType                  sort type 

    Globals:    none

    Returns:    SPI Error Code

*/
int iSpiSortSearchResults
(
    struct spiSearchResult *pssrSpiSearchResults,
    unsigned int uiSpiSearchResultsLength,
    unsigned int uiSortType
)
{

    int     iError = SPI_NoError;


    if ( (pssrSpiSearchResults != NULL) && (uiSpiSearchResultsLength > 0) ) {

        switch ( uiSortType ) {

            case SPI_SORT_TYPE_DOUBLE_ASC:
                iError = iSpiQuickSortSearchResultsDoubleAsc(pssrSpiSearchResults, 0, uiSpiSearchResultsLength - 1);
                break;

            case SPI_SORT_TYPE_DOUBLE_DESC:
                iError = iSpiQuickSortSearchResultsDoubleDesc(pssrSpiSearchResults, 0, uiSpiSearchResultsLength - 1);
                break;

            case SPI_SORT_TYPE_FLOAT_ASC:
                iError = iSpiQuickSortSearchResultsFloatAsc(pssrSpiSearchResults, 0, uiSpiSearchResultsLength - 1);
                break;

            case SPI_SORT_TYPE_FLOAT_DESC:
                iError = iSpiQuickSortSearchResultsFloatDesc(pssrSpiSearchResults, 0, uiSpiSearchResultsLength - 1);
                break;
            
            case SPI_SORT_TYPE_UINT_ASC:
                iError = iSpiQuickSortSearchResultsUIntAsc(pssrSpiSearchResults, 0, uiSpiSearchResultsLength - 1);
                break;

            case SPI_SORT_TYPE_UINT_DESC:
                iError = iSpiQuickSortSearchResultsUIntDesc(pssrSpiSearchResults, 0, uiSpiSearchResultsLength - 1);
                break;
            
            case SPI_SORT_TYPE_ULONG_ASC:
                iError = iSpiQuickSortSearchResultsULongAsc(pssrSpiSearchResults, 0, uiSpiSearchResultsLength - 1);
                break;

            case SPI_SORT_TYPE_ULONG_DESC:
                iError = iSpiQuickSortSearchResultsULongDesc(pssrSpiSearchResults, 0, uiSpiSearchResultsLength - 1);
                break;
            
            case SPI_SORT_TYPE_UCHAR_ASC:
                iError = iSpiQuickSortSearchResultsCharAsc(pssrSpiSearchResults, 0, uiSpiSearchResultsLength - 1);
                break;

            case SPI_SORT_TYPE_UCHAR_DESC:
                iError = iSpiQuickSortSearchResultsCharDesc(pssrSpiSearchResults, 0, uiSpiSearchResultsLength - 1);
                break;
            
            case SPI_SORT_TYPE_NO_SORT:
            case SPI_SORT_TYPE_UNKNOWN:
                break;
            
            default:
                ASSERT(false);
                break;
        }
    }


    return (iError);

}


/*---------------------------------------------------------------------------*/


/* 
** ==============================
** ===  Error Text Functions  ===
** ==============================
*/


/*

    Function:   iSpiGetErrorText()

    Purpose:    This function return there error text for a given SPI error.

    Parameters: iError              error
                ppucErrorText       return pointer for the error text
                uiErrorTextLength   length of the return pointer

    Globals:    none

    Returns:    SRCH error code

*/
int iSpiGetErrorText
(
    int iError, 
    unsigned char *pucErrorText,
    unsigned int uiErrorTextLength
)
{

    struct spiErrorMapping      *psemSpiErrorMappingsPtr = NULL;


    /* Check input variables */
    if ( (iError <= SPI_NoError) && (pucErrorText != NULL) && (uiErrorTextLength > 0) ) {
    
        /* Loop over all the errors in the error mapping */
        for ( psemSpiErrorMappingsPtr = psemSpiErrorMappingsGlobal; psemSpiErrorMappingsPtr->iError != SPI_ERROR_MAPPING_END_MARKER; psemSpiErrorMappingsPtr++ ) {
            
            /* Copy the error text to the return variable if the error matches */
            if ( psemSpiErrorMappingsPtr->iError == iError ) {
                s_strnncpy(pucErrorText, psemSpiErrorMappingsPtr->pucErrorText, uiErrorTextLength);
                return (SPI_NoError);
            }
        }
    }

    return (SPI_MiscError);

}


/*---------------------------------------------------------------------------*/


/* 
** ============================
** ===  Printing Functions  ===
** ============================
*/


/*

    Function:   iSpiPrintSpiSearchResponse()

    Purpose:    This function prints out the contents of a spi 
                search response structure.

    Parameters: pssrSpiSearchResponse       pointer to a spi search response structure

    Globals:    none

    Returns:    SRCH error code

*/
int iSpiPrintSpiSearchResponse
(
    struct spiSearchResponse *pssrSpiSearchResponse
)
{

    /* Check input variables */
    if ( pssrSpiSearchResponse == NULL ) {
        iUtlLogInfo(UTL_LOG_CONTEXT, "iSpiPrintSpiSearchResponse - pssrSpiSearchResponse pointer is NULL");
    }


    if ( pssrSpiSearchResponse == NULL ) {
        return (SPI_NoError);
    }


    printf("uiTotalResults [%u], uiStartIndex [%u], uiEndIndex [%u], uiSortType [%u], dMaxSortKey [%.4f], dSearchTime [%.3f]\n", 
            pssrSpiSearchResponse->uiTotalResults, pssrSpiSearchResponse->uiStartIndex, pssrSpiSearchResponse->uiEndIndex, 
            pssrSpiSearchResponse->uiSortType, pssrSpiSearchResponse->dMaxSortKey, pssrSpiSearchResponse->dSearchTime);


    iSpiPrintSpiSearchResults(pssrSpiSearchResponse->pssrSpiSearchResults, pssrSpiSearchResponse->uiSpiSearchResultsLength, pssrSpiSearchResponse->uiSortType);


    return (SPI_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSpiPrintSpiSearchResults()

    Purpose:    This function prints out the contents of an array of spi 
                search result structures.

    Parameters: pssrSpiSearchResults        pointer to an array of spi search result structures
                uiSpiSearchResultsLength    length of the spi search results array
                uiSortType                  sort type 

    Globals:    none

    Returns:    SRCH error code

*/
int iSpiPrintSpiSearchResults
(
    struct spiSearchResult *pssrSpiSearchResults,
    unsigned int uiSpiSearchResultsLength,
    unsigned int uiSortType
)
{

    unsigned int                uiI = 0;
    struct spiSearchResult      *pssrSpiSearchResultsPtr = NULL;


    /* Check input variables */
    if ( pssrSpiSearchResults == NULL ) {
        iUtlLogInfo(UTL_LOG_CONTEXT, "iSpiPrintSpiSearchResults - pssrSpiSearchResults pointer is NULL");
    }

    if ( uiSpiSearchResultsLength <= 0 ) {
        iUtlLogInfo(UTL_LOG_CONTEXT, "iSpiPrintSpiSearchResults - uiSpiSearchResultsLength = %u", uiSpiSearchResultsLength);
    }

    if ( (pssrSpiSearchResults == NULL) || (uiSpiSearchResultsLength <= 0) ) {
        return (SPI_NoError);
    }


    /* Print out the search results */
    for ( uiI = 0, pssrSpiSearchResultsPtr = pssrSpiSearchResults; uiI < uiSpiSearchResultsLength; uiI++, pssrSpiSearchResultsPtr++ ) {

        struct spiDocumentItem      *psdiSpiDocumentItemsPtr = NULL;
        unsigned char               pucDocItems[UTL_FILE_PATH_MAX + 1] = {'\0'};
        unsigned char               pucNum[UTL_FILE_PATH_MAX + 1] = {'\0'};
        unsigned char               pucSortKey[UTL_FILE_PATH_MAX + 1] = {'\0'};
        unsigned int                uiJ = 0;

    
        /* Create the sort key string */
        switch ( uiSortType ) {

            case SPI_SORT_TYPE_DOUBLE_ASC:
            case SPI_SORT_TYPE_DOUBLE_DESC:
                snprintf(pucSortKey, UTL_FILE_PATH_MAX + 1, "dSortKey [%9.4f]", pssrSpiSearchResultsPtr->dSortKey);
                break;

            case SPI_SORT_TYPE_FLOAT_ASC:
            case SPI_SORT_TYPE_FLOAT_DESC:
                snprintf(pucSortKey, UTL_FILE_PATH_MAX + 1, "fSortKey [%9.4f]", pssrSpiSearchResultsPtr->fSortKey);
                break;
            
            case SPI_SORT_TYPE_UINT_ASC:
            case SPI_SORT_TYPE_UINT_DESC:
                snprintf(pucSortKey, UTL_FILE_PATH_MAX + 1, "uiSortKey [%u]", pssrSpiSearchResultsPtr->uiSortKey);
                break;
            
            case SPI_SORT_TYPE_ULONG_ASC:
            case SPI_SORT_TYPE_ULONG_DESC:
                snprintf(pucSortKey, UTL_FILE_PATH_MAX + 1, "ulSortKey [%lu]", pssrSpiSearchResultsPtr->ulSortKey);
                break;
            
            case SPI_SORT_TYPE_UCHAR_ASC:
            case SPI_SORT_TYPE_UCHAR_DESC:
                snprintf(pucSortKey, UTL_FILE_PATH_MAX + 1, "pucSortKey [%s]", pucUtlStringsGetPrintableString(pssrSpiSearchResultsPtr->pucSortKey));
                break;
            
            case SPI_SORT_TYPE_NO_SORT:
                snprintf(pucSortKey, UTL_FILE_PATH_MAX + 1, "SortKey [none]");
                break;

            case SPI_SORT_TYPE_UNKNOWN:
                snprintf(pucSortKey, UTL_FILE_PATH_MAX + 1, "SortKey [unknown]");
                break;
            
            default:
                snprintf(pucSortKey, UTL_FILE_PATH_MAX + 1, "SortKey [invalid]");
                break;
        }
    
        
        /* Are there any document items for this hit? */
        if ( pssrSpiSearchResultsPtr->psdiSpiDocumentItems != NULL ) {

            /* Loop through each item, assemble a readable string from them */
            for ( uiJ = 0, psdiSpiDocumentItemsPtr = pssrSpiSearchResultsPtr->psdiSpiDocumentItems; uiJ < pssrSpiSearchResultsPtr->uiDocumentItemsLength; uiJ++, psdiSpiDocumentItemsPtr++ ) {

                if ( (uiJ > 0) && (uiJ < pssrSpiSearchResultsPtr->uiDocumentItemsLength) ) {
                    s_strnncat(pucDocItems, "; ", UTL_FILE_PATH_MAX, UTL_FILE_PATH_MAX + 1);
                }

                s_strnncat(pucDocItems, psdiSpiDocumentItemsPtr->pucItemName, UTL_FILE_PATH_MAX, UTL_FILE_PATH_MAX + 1);
                s_strnncat(pucDocItems, ", ", UTL_FILE_PATH_MAX, UTL_FILE_PATH_MAX + 1);

                s_strnncat(pucDocItems, psdiSpiDocumentItemsPtr->pucMimeType, UTL_FILE_PATH_MAX, UTL_FILE_PATH_MAX + 1);
                s_strnncat(pucDocItems, ", ", UTL_FILE_PATH_MAX, UTL_FILE_PATH_MAX + 1);

                snprintf(pucNum, UTL_FILE_PATH_MAX + 1, "%u", psdiSpiDocumentItemsPtr->uiLength);
                s_strnncat(pucDocItems, pucNum, UTL_FILE_PATH_MAX, UTL_FILE_PATH_MAX + 1);
            } 
        }
        else {
            /* No document items are defined */
            s_strnncpy(pucDocItems, "None", UTL_FILE_PATH_MAX + 1);
        }

        printf("[%u], pucIndexName [%s], pucDocumentKey [%s], pucTitle [%s], %s, pucLanguageCode [%s], uiRank [%u], uiTermCount [%u], ulAnsiDate [%lu], Items [%s]\n", 
                uiI, pssrSpiSearchResultsPtr->pucIndexName, pssrSpiSearchResultsPtr->pucDocumentKey, pssrSpiSearchResultsPtr->pucTitle, pucSortKey, 
                pucUtlStringsGetPrintableString(pssrSpiSearchResultsPtr->pucLanguageCode), pssrSpiSearchResultsPtr->uiRank, pssrSpiSearchResultsPtr->uiTermCount, 
                pssrSpiSearchResultsPtr->ulAnsiDate, pucDocItems);
    }


    return (SPI_NoError);

}


/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/


/*

    Function:   iSpiQuickSortSearchResultsDoubleAsc()

    Purpose:    This function sorts a search results array in ascending double order, it
                implements a quicksort algorithm for speed. The reason we use
                this guy rather than qsort() is for sheer speed, we get a 
                2:1 to 3:1 speed increase because the condition is inline.

    Parameters: pssrSpiSearchResults            pointer to an spi search results array
                iSpiSearchResultsLeftIndex      left hand index in the array to sort 
                iSpiSearchResultsRightIndex     right hand index in the array to sort 

    Globals:    none

    Returns:    SPI Error Code

*/
static int iSpiQuickSortSearchResultsDoubleAsc
(
    struct spiSearchResult *pssrSpiSearchResults,
    int iSpiSearchResultsLeftIndex,
    int iSpiSearchResultsRightIndex
)
{

    int                         iSpiSearchResultsLocalLeftIndex = 0;
    int                         iSpiSearchResultsLocalRightIndex = 0;
    struct spiSearchResult      *pssrSpiSearchResultsRightIndexPtr = pssrSpiSearchResults + iSpiSearchResultsRightIndex;


    if ( (pssrSpiSearchResults != NULL) && (iSpiSearchResultsRightIndex > iSpiSearchResultsLeftIndex) ) {

        iSpiSearchResultsLocalLeftIndex = iSpiSearchResultsLeftIndex - 1; 
        iSpiSearchResultsLocalRightIndex = iSpiSearchResultsRightIndex;
        
        for ( ;; ) {

            while ( (++iSpiSearchResultsLocalLeftIndex <= iSpiSearchResultsRightIndex) && 
                    ((pssrSpiSearchResults + iSpiSearchResultsLocalLeftIndex)->dSortKey < pssrSpiSearchResultsRightIndexPtr->dSortKey) );
            
            while ( (--iSpiSearchResultsLocalRightIndex > iSpiSearchResultsLeftIndex) && 
                    ((pssrSpiSearchResults + iSpiSearchResultsLocalRightIndex)->dSortKey > pssrSpiSearchResultsRightIndexPtr->dSortKey) );

            if ( iSpiSearchResultsLocalLeftIndex >= iSpiSearchResultsLocalRightIndex ) {
                break;
            }
            
            SPI_SWAP_SEARCH_RESULT_DOUBLE((pssrSpiSearchResults + iSpiSearchResultsLocalRightIndex), (pssrSpiSearchResults + iSpiSearchResultsLocalLeftIndex));
        }
        
        if ( iSpiSearchResultsLocalLeftIndex != iSpiSearchResultsRightIndex ) {
            SPI_SWAP_SEARCH_RESULT_DOUBLE(pssrSpiSearchResultsRightIndexPtr, (pssrSpiSearchResults + iSpiSearchResultsLocalLeftIndex));
        }
        
        iSpiQuickSortSearchResultsDoubleAsc(pssrSpiSearchResults, iSpiSearchResultsLeftIndex, iSpiSearchResultsLocalLeftIndex - 1);
        iSpiQuickSortSearchResultsDoubleAsc(pssrSpiSearchResults, iSpiSearchResultsLocalLeftIndex + 1, iSpiSearchResultsRightIndex);
    }


    return (SPI_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSpiQuickSortSearchResultsDoubleDesc()

    Purpose:    This function sorts a search results array in descending double order, it
                implements a quicksort algorithm for speed. The reason we use
                this guy rather than qsort() is for sheer speed, we get a 
                2:1 to 3:1 speed increase because the condition is inline.

    Parameters: pssrSpiSearchResults            pointer to an spi search results array
                iSpiSearchResultsLeftIndex      left hand index in the array to sort 
                iSpiSearchResultsRightIndex     right hand index in the array to sort 

    Globals:    none

    Returns:    SPI Error Code

*/
static int iSpiQuickSortSearchResultsDoubleDesc
(
    struct spiSearchResult *pssrSpiSearchResults,
    int iSpiSearchResultsLeftIndex,
    int iSpiSearchResultsRightIndex
)
{

    int                         iSpiSearchResultsLocalLeftIndex = 0;
    int                         iSpiSearchResultsLocalRightIndex = 0;
    struct spiSearchResult      *pssrSpiSearchResultsRightIndexPtr = pssrSpiSearchResults + iSpiSearchResultsRightIndex;


    if ( (pssrSpiSearchResults != NULL) && (iSpiSearchResultsRightIndex > iSpiSearchResultsLeftIndex) ) {

        iSpiSearchResultsLocalLeftIndex = iSpiSearchResultsLeftIndex - 1; 
        iSpiSearchResultsLocalRightIndex = iSpiSearchResultsRightIndex;
        
        for ( ;; ) {

            while ( (++iSpiSearchResultsLocalLeftIndex <= iSpiSearchResultsRightIndex) && 
                    ((pssrSpiSearchResults + iSpiSearchResultsLocalLeftIndex)->dSortKey > pssrSpiSearchResultsRightIndexPtr->dSortKey) );

            while ( (--iSpiSearchResultsLocalRightIndex > iSpiSearchResultsLeftIndex) && 
                    ((pssrSpiSearchResults + iSpiSearchResultsLocalRightIndex)->dSortKey < pssrSpiSearchResultsRightIndexPtr->dSortKey) );

            if ( iSpiSearchResultsLocalLeftIndex >= iSpiSearchResultsLocalRightIndex ) {
                break;
            }
            
            SPI_SWAP_SEARCH_RESULT_DOUBLE((pssrSpiSearchResults + iSpiSearchResultsLocalRightIndex), (pssrSpiSearchResults + iSpiSearchResultsLocalLeftIndex));
        }
        
        if ( iSpiSearchResultsLocalLeftIndex != iSpiSearchResultsRightIndex ) {
            SPI_SWAP_SEARCH_RESULT_DOUBLE(pssrSpiSearchResultsRightIndexPtr, (pssrSpiSearchResults + iSpiSearchResultsLocalLeftIndex));
        }
        
        iSpiQuickSortSearchResultsDoubleDesc(pssrSpiSearchResults, iSpiSearchResultsLeftIndex, iSpiSearchResultsLocalLeftIndex - 1);
        iSpiQuickSortSearchResultsDoubleDesc(pssrSpiSearchResults, iSpiSearchResultsLocalLeftIndex + 1, iSpiSearchResultsRightIndex);
    }


    return (SPI_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSpiQuickSortSearchResultsFloatAsc()

    Purpose:    This function sorts a search results array in ascending float order, it
                implements a quicksort algorithm for speed. The reason we use
                this guy rather than qsort() is for sheer speed, we get a 
                2:1 to 3:1 speed increase because the condition is inline.

    Parameters: pssrSpiSearchResults            pointer to an spi search results array
                iSpiSearchResultsLeftIndex      left hand index in the array to sort 
                iSpiSearchResultsRightIndex     right hand index in the array to sort 

    Globals:    none

    Returns:    SPI Error Code

*/
static int iSpiQuickSortSearchResultsFloatAsc
(
    struct spiSearchResult *pssrSpiSearchResults,
    int iSpiSearchResultsLeftIndex,
    int iSpiSearchResultsRightIndex
)
{

    int                         iSpiSearchResultsLocalLeftIndex = 0;
    int                         iSpiSearchResultsLocalRightIndex = 0;
    struct spiSearchResult      *pssrSpiSearchResultsRightIndexPtr = pssrSpiSearchResults + iSpiSearchResultsRightIndex;


    if ( (pssrSpiSearchResults != NULL) && (iSpiSearchResultsRightIndex > iSpiSearchResultsLeftIndex) ) {

        iSpiSearchResultsLocalLeftIndex = iSpiSearchResultsLeftIndex - 1; 
        iSpiSearchResultsLocalRightIndex = iSpiSearchResultsRightIndex;
        
        for ( ;; ) {

            while ( (++iSpiSearchResultsLocalLeftIndex <= iSpiSearchResultsRightIndex) && 
                    ((pssrSpiSearchResults + iSpiSearchResultsLocalLeftIndex)->fSortKey < pssrSpiSearchResultsRightIndexPtr->fSortKey) );
            
            while ( (--iSpiSearchResultsLocalRightIndex > iSpiSearchResultsLeftIndex) && 
                    ((pssrSpiSearchResults + iSpiSearchResultsLocalRightIndex)->fSortKey > pssrSpiSearchResultsRightIndexPtr->fSortKey) );

            if ( iSpiSearchResultsLocalLeftIndex >= iSpiSearchResultsLocalRightIndex ) {
                break;
            }
            
            SPI_SWAP_SEARCH_RESULT_FLOAT((pssrSpiSearchResults + iSpiSearchResultsLocalRightIndex), (pssrSpiSearchResults + iSpiSearchResultsLocalLeftIndex));
        }
        
        if ( iSpiSearchResultsLocalLeftIndex != iSpiSearchResultsRightIndex ) {
            SPI_SWAP_SEARCH_RESULT_FLOAT(pssrSpiSearchResultsRightIndexPtr, (pssrSpiSearchResults + iSpiSearchResultsLocalLeftIndex));
        }
        
        iSpiQuickSortSearchResultsFloatAsc(pssrSpiSearchResults, iSpiSearchResultsLeftIndex, iSpiSearchResultsLocalLeftIndex - 1);
        iSpiQuickSortSearchResultsFloatAsc(pssrSpiSearchResults, iSpiSearchResultsLocalLeftIndex + 1, iSpiSearchResultsRightIndex);
    }


    return (SPI_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSpiQuickSortSearchResultsFloatDesc()

    Purpose:    This function sorts a search results array in descending float order, it
                implements a quicksort algorithm for speed. The reason we use
                this guy rather than qsort() is for sheer speed, we get a 
                2:1 to 3:1 speed increase because the condition is inline.

    Parameters: pssrSpiSearchResults            pointer to an spi search results array
                iSpiSearchResultsLeftIndex      left hand index in the array to sort 
                iSpiSearchResultsRightIndex     right hand index in the array to sort 

    Globals:    none

    Returns:    SPI Error Code

*/
static int iSpiQuickSortSearchResultsFloatDesc
(
    struct spiSearchResult *pssrSpiSearchResults,
    int iSpiSearchResultsLeftIndex,
    int iSpiSearchResultsRightIndex
)
{

    int                         iSpiSearchResultsLocalLeftIndex = 0;
    int                         iSpiSearchResultsLocalRightIndex = 0;
    struct spiSearchResult      *pssrSpiSearchResultsRightIndexPtr = pssrSpiSearchResults + iSpiSearchResultsRightIndex;


    if ( (pssrSpiSearchResults != NULL) && (iSpiSearchResultsRightIndex > iSpiSearchResultsLeftIndex) ) {

        iSpiSearchResultsLocalLeftIndex = iSpiSearchResultsLeftIndex - 1; 
        iSpiSearchResultsLocalRightIndex = iSpiSearchResultsRightIndex;
        
        for ( ;; ) {

            while ( (++iSpiSearchResultsLocalLeftIndex <= iSpiSearchResultsRightIndex) && 
                    ((pssrSpiSearchResults + iSpiSearchResultsLocalLeftIndex)->fSortKey > pssrSpiSearchResultsRightIndexPtr->fSortKey) );

            while ( (--iSpiSearchResultsLocalRightIndex > iSpiSearchResultsLeftIndex) && 
                    ((pssrSpiSearchResults + iSpiSearchResultsLocalRightIndex)->fSortKey < pssrSpiSearchResultsRightIndexPtr->fSortKey) );

            if ( iSpiSearchResultsLocalLeftIndex >= iSpiSearchResultsLocalRightIndex ) {
                break;
            }
            
            SPI_SWAP_SEARCH_RESULT_FLOAT((pssrSpiSearchResults + iSpiSearchResultsLocalRightIndex), (pssrSpiSearchResults + iSpiSearchResultsLocalLeftIndex));
        }
        
        if ( iSpiSearchResultsLocalLeftIndex != iSpiSearchResultsRightIndex ) {
            SPI_SWAP_SEARCH_RESULT_FLOAT(pssrSpiSearchResultsRightIndexPtr, (pssrSpiSearchResults + iSpiSearchResultsLocalLeftIndex));
        }
        
        iSpiQuickSortSearchResultsFloatDesc(pssrSpiSearchResults, iSpiSearchResultsLeftIndex, iSpiSearchResultsLocalLeftIndex - 1);
        iSpiQuickSortSearchResultsFloatDesc(pssrSpiSearchResults, iSpiSearchResultsLocalLeftIndex + 1, iSpiSearchResultsRightIndex);
    }


    return (SPI_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSpiQuickSortSearchResultsUIntAsc()

    Purpose:    This function sorts a search results array in ascending unsigned integer order, 
                it implements a quicksort algorithm for speed. The reason we use
                this guy rather than qsort() is for sheer speed, we get a 
                2:1 to 3:1 speed increase because the condition is inline.

    Parameters: pssrSpiSearchResults            pointer to an spi search results array
                iSpiSearchResultsLeftIndex      left hand index in the array to sort 
                iSpiSearchResultsRightIndex     right hand index in the array to sort 

    Globals:    none

    Returns:    SPI Error Code

*/
static int iSpiQuickSortSearchResultsUIntAsc
(
    struct spiSearchResult *pssrSpiSearchResults,
    int iSpiSearchResultsLeftIndex,
    int iSpiSearchResultsRightIndex
)
{

    int                         iSpiSearchResultsLocalLeftIndex = 0;
    int                         iSpiSearchResultsLocalRightIndex = 0;
    struct spiSearchResult      *pssrSpiSearchResultsRightIndexPtr = pssrSpiSearchResults + iSpiSearchResultsRightIndex;


    if ( (pssrSpiSearchResults != NULL) && (iSpiSearchResultsRightIndex > iSpiSearchResultsLeftIndex) ) {

        iSpiSearchResultsLocalLeftIndex = iSpiSearchResultsLeftIndex - 1; 
        iSpiSearchResultsLocalRightIndex = iSpiSearchResultsRightIndex;
        
        for ( ;; ) {
            
            while ( (++iSpiSearchResultsLocalLeftIndex <= iSpiSearchResultsRightIndex) && 
                    ((pssrSpiSearchResults + iSpiSearchResultsLocalLeftIndex)->uiSortKey < pssrSpiSearchResultsRightIndexPtr->uiSortKey) );

            while ( (--iSpiSearchResultsLocalRightIndex > iSpiSearchResultsLeftIndex) && 
                    ((pssrSpiSearchResults + iSpiSearchResultsLocalRightIndex)->uiSortKey > pssrSpiSearchResultsRightIndexPtr->uiSortKey) );

            if ( iSpiSearchResultsLocalLeftIndex >= iSpiSearchResultsLocalRightIndex ) {
                break;
            }
            
            SPI_SWAP_SEARCH_RESULT_UINT((pssrSpiSearchResults + iSpiSearchResultsLocalRightIndex), (pssrSpiSearchResults + iSpiSearchResultsLocalLeftIndex));
        }
        
        if ( iSpiSearchResultsLocalLeftIndex != iSpiSearchResultsRightIndex ) {
            SPI_SWAP_SEARCH_RESULT_UINT(pssrSpiSearchResultsRightIndexPtr, (pssrSpiSearchResults + iSpiSearchResultsLocalLeftIndex));
        }

        iSpiQuickSortSearchResultsUIntAsc(pssrSpiSearchResults, iSpiSearchResultsLeftIndex, iSpiSearchResultsLocalLeftIndex - 1);
        iSpiQuickSortSearchResultsUIntAsc(pssrSpiSearchResults, iSpiSearchResultsLocalLeftIndex + 1, iSpiSearchResultsRightIndex);
    }


    return (SPI_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSpiQuickSortSearchResultsUIntDesc()

    Purpose:    This function sorts a search results array in descending unsigned integer order, 
                it implements a quicksort algorithm for speed. The reason we use
                this guy rather than qsort() is for sheer speed, we get a 
                2:1 to 3:1 speed increase because the condition is inline.

    Parameters: pssrSpiSearchResults            pointer to an spi search results array
                iSpiSearchResultsLeftIndex      left hand index in the array to sort 
                iSpiSearchResultsRightIndex     right hand index in the array to sort 

    Globals:    none

    Returns:    SPI Error Code

*/
static int iSpiQuickSortSearchResultsUIntDesc
(
    struct spiSearchResult *pssrSpiSearchResults,
    int iSpiSearchResultsLeftIndex,
    int iSpiSearchResultsRightIndex
)
{

    int                         iSpiSearchResultsLocalLeftIndex = 0;
    int                         iSpiSearchResultsLocalRightIndex = 0;
    struct spiSearchResult      *pssrSpiSearchResultsRightIndexPtr = pssrSpiSearchResults + iSpiSearchResultsRightIndex;


    if ( (pssrSpiSearchResults != NULL) && (iSpiSearchResultsRightIndex > iSpiSearchResultsLeftIndex) ) {

        iSpiSearchResultsLocalLeftIndex = iSpiSearchResultsLeftIndex - 1; 
        iSpiSearchResultsLocalRightIndex = iSpiSearchResultsRightIndex;
        
        for ( ;; ) {

            while ( (++iSpiSearchResultsLocalLeftIndex <= iSpiSearchResultsRightIndex) && 
                    ((pssrSpiSearchResults + iSpiSearchResultsLocalLeftIndex)->uiSortKey > pssrSpiSearchResultsRightIndexPtr->uiSortKey) );

            while ( (--iSpiSearchResultsLocalRightIndex > iSpiSearchResultsLeftIndex) && 
                    ((pssrSpiSearchResults + iSpiSearchResultsLocalRightIndex)->uiSortKey < pssrSpiSearchResultsRightIndexPtr->uiSortKey) );

            if ( iSpiSearchResultsLocalLeftIndex >= iSpiSearchResultsLocalRightIndex ) {
                break;
            }
            
            SPI_SWAP_SEARCH_RESULT_UINT((pssrSpiSearchResults + iSpiSearchResultsLocalRightIndex), (pssrSpiSearchResults + iSpiSearchResultsLocalLeftIndex));
        }
        
        if ( iSpiSearchResultsLocalLeftIndex != iSpiSearchResultsRightIndex ) {
            SPI_SWAP_SEARCH_RESULT_UINT(pssrSpiSearchResultsRightIndexPtr, (pssrSpiSearchResults + iSpiSearchResultsLocalLeftIndex));
        }

        iSpiQuickSortSearchResultsUIntDesc(pssrSpiSearchResults, iSpiSearchResultsLeftIndex, iSpiSearchResultsLocalLeftIndex - 1);
        iSpiQuickSortSearchResultsUIntDesc(pssrSpiSearchResults, iSpiSearchResultsLocalLeftIndex + 1, iSpiSearchResultsRightIndex);
    }


    return (SPI_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSpiQuickSortSearchResultsULongAsc()

    Purpose:    This function sorts a search results array in ascending unsigned long order, 
                it implements a quicksort algorithm for speed. The reason we use
                this guy rather than qsort() is for sheer speed, we get a 
                2:1 to 3:1 speed increase because the condition is inline.

    Parameters: pssrSpiSearchResults            pointer to an spi search results array
                iSpiSearchResultsLeftIndex      left hand index in the array to sort 
                iSpiSearchResultsRightIndex     right hand index in the array to sort 

    Globals:    none

    Returns:    SPI Error Code

*/
static int iSpiQuickSortSearchResultsULongAsc
(
    struct spiSearchResult *pssrSpiSearchResults,
    int iSpiSearchResultsLeftIndex,
    int iSpiSearchResultsRightIndex
)
{

    int                         iSpiSearchResultsLocalLeftIndex = 0;
    int                         iSpiSearchResultsLocalRightIndex = 0;
    struct spiSearchResult      *pssrSpiSearchResultsRightIndexPtr = pssrSpiSearchResults + iSpiSearchResultsRightIndex;


    if ( (pssrSpiSearchResults != NULL) && (iSpiSearchResultsRightIndex > iSpiSearchResultsLeftIndex) ) {

        iSpiSearchResultsLocalLeftIndex = iSpiSearchResultsLeftIndex - 1; 
        iSpiSearchResultsLocalRightIndex = iSpiSearchResultsRightIndex;
        
        for ( ;; ) {
            
            while ( (++iSpiSearchResultsLocalLeftIndex <= iSpiSearchResultsRightIndex) && 
                    ((pssrSpiSearchResults + iSpiSearchResultsLocalLeftIndex)->ulSortKey < pssrSpiSearchResultsRightIndexPtr->ulSortKey) );

            while ( (--iSpiSearchResultsLocalRightIndex > iSpiSearchResultsLeftIndex) && 
                    ((pssrSpiSearchResults + iSpiSearchResultsLocalRightIndex)->ulSortKey > pssrSpiSearchResultsRightIndexPtr->ulSortKey) );

            if ( iSpiSearchResultsLocalLeftIndex >= iSpiSearchResultsLocalRightIndex ) {
                break;
            }
            
            SPI_SWAP_SEARCH_RESULT_ULLONG((pssrSpiSearchResults + iSpiSearchResultsLocalRightIndex), (pssrSpiSearchResults + iSpiSearchResultsLocalLeftIndex));
        }
        
        if ( iSpiSearchResultsLocalLeftIndex != iSpiSearchResultsRightIndex ) {
            SPI_SWAP_SEARCH_RESULT_ULLONG(pssrSpiSearchResultsRightIndexPtr, (pssrSpiSearchResults + iSpiSearchResultsLocalLeftIndex));
        }

        iSpiQuickSortSearchResultsULongAsc(pssrSpiSearchResults, iSpiSearchResultsLeftIndex, iSpiSearchResultsLocalLeftIndex - 1);
        iSpiQuickSortSearchResultsULongAsc(pssrSpiSearchResults, iSpiSearchResultsLocalLeftIndex + 1, iSpiSearchResultsRightIndex);
    }


    return (SPI_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSpiQuickSortSearchResultsULongDesc()

    Purpose:    This function sorts a search results array in descending unsigned long order, 
                it implements a quicksort algorithm for speed. The reason we use
                this guy rather than qsort() is for sheer speed, we get a 
                2:1 to 3:1 speed increase because the condition is inline.

    Parameters: pssrSpiSearchResults            pointer to an spi search results array
                iSpiSearchResultsLeftIndex      left hand index in the array to sort 
                iSpiSearchResultsRightIndex     right hand index in the array to sort 

    Globals:    none

    Returns:    SPI Error Code

*/
static int iSpiQuickSortSearchResultsULongDesc
(
    struct spiSearchResult *pssrSpiSearchResults,
    int iSpiSearchResultsLeftIndex,
    int iSpiSearchResultsRightIndex
)
{

    int                         iSpiSearchResultsLocalLeftIndex = 0;
    int                         iSpiSearchResultsLocalRightIndex = 0;
    struct spiSearchResult      *pssrSpiSearchResultsRightIndexPtr = pssrSpiSearchResults + iSpiSearchResultsRightIndex;


    if ( (pssrSpiSearchResults != NULL) && (iSpiSearchResultsRightIndex > iSpiSearchResultsLeftIndex) ) {

        iSpiSearchResultsLocalLeftIndex = iSpiSearchResultsLeftIndex - 1; 
        iSpiSearchResultsLocalRightIndex = iSpiSearchResultsRightIndex;
        
        for ( ;; ) {

            while ( (++iSpiSearchResultsLocalLeftIndex <= iSpiSearchResultsRightIndex) && 
                    ((pssrSpiSearchResults + iSpiSearchResultsLocalLeftIndex)->ulSortKey > pssrSpiSearchResultsRightIndexPtr->ulSortKey) );

            while ( (--iSpiSearchResultsLocalRightIndex > iSpiSearchResultsLeftIndex) && 
                    ((pssrSpiSearchResults + iSpiSearchResultsLocalRightIndex)->ulSortKey < pssrSpiSearchResultsRightIndexPtr->ulSortKey) );

            if ( iSpiSearchResultsLocalLeftIndex >= iSpiSearchResultsLocalRightIndex ) {
                break;
            }
            
            SPI_SWAP_SEARCH_RESULT_ULLONG((pssrSpiSearchResults + iSpiSearchResultsLocalRightIndex), (pssrSpiSearchResults + iSpiSearchResultsLocalLeftIndex));
        }
        
        if ( iSpiSearchResultsLocalLeftIndex != iSpiSearchResultsRightIndex ) {
            SPI_SWAP_SEARCH_RESULT_ULLONG(pssrSpiSearchResultsRightIndexPtr, (pssrSpiSearchResults + iSpiSearchResultsLocalLeftIndex));
        }

        iSpiQuickSortSearchResultsULongDesc(pssrSpiSearchResults, iSpiSearchResultsLeftIndex, iSpiSearchResultsLocalLeftIndex - 1);
        iSpiQuickSortSearchResultsULongDesc(pssrSpiSearchResults, iSpiSearchResultsLocalLeftIndex + 1, iSpiSearchResultsRightIndex);
    }


    return (SPI_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSpiQuickSortSearchResultsCharAsc()

    Purpose:    This function sorts a search results array in ascending character order, it
                implements a quicksort algorithm for speed. The reason we use
                this guy rather than qsort() is for sheer speed, we get a 
                2:1 to 3:1 speed increase because the condition is inline.

    Parameters: pssrSpiSearchResults            pointer to an spi search results array
                iSpiSearchResultsLeftIndex      left hand index in the array to sort 
                iSpiSearchResultsRightIndex     right hand index in the array to sort 

    Globals:    none

    Returns:    SPI Error Code

*/
static int iSpiQuickSortSearchResultsCharAsc
(
    struct spiSearchResult *pssrSpiSearchResults,
    int iSpiSearchResultsLeftIndex,
    int iSpiSearchResultsRightIndex
)
{

    int                         iSpiSearchResultsLocalLeftIndex = 0;
    int                         iSpiSearchResultsLocalRightIndex = 0;
    struct spiSearchResult      *pssrSpiSearchResultsRightIndexPtr = pssrSpiSearchResults + iSpiSearchResultsRightIndex;


    if ( (pssrSpiSearchResults != NULL) && (iSpiSearchResultsRightIndex > iSpiSearchResultsLeftIndex) ) {

        iSpiSearchResultsLocalLeftIndex = iSpiSearchResultsLeftIndex - 1; 
        iSpiSearchResultsLocalRightIndex = iSpiSearchResultsRightIndex;
        
        for ( ;; ) {

            while ( (++iSpiSearchResultsLocalLeftIndex <= iSpiSearchResultsRightIndex) && 
                    (s_strcoll((pssrSpiSearchResults + iSpiSearchResultsLocalLeftIndex)->pucSortKey, pssrSpiSearchResultsRightIndexPtr->pucSortKey) < 0) );

            while ( (--iSpiSearchResultsLocalRightIndex > iSpiSearchResultsLeftIndex) && 
                    (s_strcoll((pssrSpiSearchResults + iSpiSearchResultsLocalRightIndex)->pucSortKey, pssrSpiSearchResultsRightIndexPtr->pucSortKey) > 0) );

            if ( iSpiSearchResultsLocalLeftIndex >= iSpiSearchResultsLocalRightIndex ) {
                break;
            }
            
            SPI_SWAP_SEARCH_RESULT_UCHAR((pssrSpiSearchResults + iSpiSearchResultsLocalRightIndex), (pssrSpiSearchResults + iSpiSearchResultsLocalLeftIndex));
        }
        
        if ( iSpiSearchResultsLocalLeftIndex != iSpiSearchResultsRightIndex ) {
            SPI_SWAP_SEARCH_RESULT_UCHAR(pssrSpiSearchResultsRightIndexPtr, (pssrSpiSearchResults + iSpiSearchResultsLocalLeftIndex));
        }
        
        iSpiQuickSortSearchResultsCharAsc(pssrSpiSearchResults, iSpiSearchResultsLeftIndex, iSpiSearchResultsLocalLeftIndex - 1);
        iSpiQuickSortSearchResultsCharAsc(pssrSpiSearchResults, iSpiSearchResultsLocalLeftIndex + 1, iSpiSearchResultsRightIndex);
    }

    
    return (SPI_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSpiQuickSortSearchResultsCharDesc()

    Purpose:    This function sorts a search results array in descending character order, it
                implements a quicksort algorithm for speed. The reason we use
                this guy rather than qsort() is for sheer speed, we get a 
                2:1 to 3:1 speed increase because the condition is inline.

    Parameters: pssrSpiSearchResults            pointer to an spi search results array
                iSpiSearchResultsLeftIndex      left hand index in the array to sort 
                iSpiSearchResultsRightIndex     right hand index in the array to sort 

    Globals:    none

    Returns:    SPI Error Code

*/
static int iSpiQuickSortSearchResultsCharDesc
(
    struct spiSearchResult *pssrSpiSearchResults,
    int iSpiSearchResultsLeftIndex,
    int iSpiSearchResultsRightIndex
)
{

    int                         iSpiSearchResultsLocalLeftIndex = 0;
    int                         iSpiSearchResultsLocalRightIndex = 0;
    struct spiSearchResult      *pssrSpiSearchResultsRightIndexPtr = pssrSpiSearchResults + iSpiSearchResultsRightIndex;


    if ( (pssrSpiSearchResults != NULL) && (iSpiSearchResultsRightIndex > iSpiSearchResultsLeftIndex) ) {

        iSpiSearchResultsLocalLeftIndex = iSpiSearchResultsLeftIndex - 1; 
        iSpiSearchResultsLocalRightIndex = iSpiSearchResultsRightIndex;
        
        for ( ;; ) {

            while ( (++iSpiSearchResultsLocalLeftIndex <= iSpiSearchResultsRightIndex) && 
                    (s_strcoll((pssrSpiSearchResults + iSpiSearchResultsLocalLeftIndex)->pucSortKey, pssrSpiSearchResultsRightIndexPtr->pucSortKey) > 0) );

            while ( (--iSpiSearchResultsLocalRightIndex > iSpiSearchResultsLeftIndex) && 
                    (s_strcoll((pssrSpiSearchResults + iSpiSearchResultsLocalRightIndex)->pucSortKey, pssrSpiSearchResultsRightIndexPtr->pucSortKey) < 0) );

            if ( iSpiSearchResultsLocalLeftIndex >= iSpiSearchResultsLocalRightIndex ) {
                break;
            }
            
            SPI_SWAP_SEARCH_RESULT_UCHAR((pssrSpiSearchResults + iSpiSearchResultsLocalRightIndex), (pssrSpiSearchResults + iSpiSearchResultsLocalLeftIndex));
        }

        if ( iSpiSearchResultsLocalLeftIndex != iSpiSearchResultsRightIndex ) {
            SPI_SWAP_SEARCH_RESULT_UCHAR(pssrSpiSearchResultsRightIndexPtr, (pssrSpiSearchResults + iSpiSearchResultsLocalLeftIndex));
        }
        
        iSpiQuickSortSearchResultsCharDesc(pssrSpiSearchResults, iSpiSearchResultsLeftIndex, iSpiSearchResultsLocalLeftIndex - 1);
        iSpiQuickSortSearchResultsCharDesc(pssrSpiSearchResults, iSpiSearchResultsLocalLeftIndex + 1, iSpiSearchResultsRightIndex);
    }


    return (SPI_NoError);

}


/*---------------------------------------------------------------------------*/
