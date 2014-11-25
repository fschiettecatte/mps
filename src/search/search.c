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

    Module:     search.c

    Author:     Francois Schiettecatte (FS Consulting LLC.)

    Created:    4 February 1994

    Purpose:    This module implements the guts of the search engine. It is divided
                up into two sections. The first section defines all the externals
                required by spi.h, and the second section contains all the support
                function for the first section. 

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
#define UTL_LOG_CONTEXT                     (unsigned char *)"com.fsconsult.mps.src.search.search"


/* Enable enable skipping over search/feedback text conversion error */
/* #define SRCH_SEARCH_ENABLE_SKIP_TEXT_CONVERSION_ERRORS */


/* Enable document length reweighting, pick at most one */
/* #define SRCH_SEARCH_ENABLE_DOCUMENT_WEIGHT_NORMALIZATION_RAW */
/* #define SRCH_SEARCH_ENABLE_DOCUMENT_WEIGHT_NORMALIZATION_AVERAGE */
#define SRCH_SEARCH_ENABLE_DOCUMENT_WEIGHT_NORMALIZATION_LOG
/* #define SRCH_SEARCH_ENABLE_DOCUMENT_WEIGHT_NORMALIZATION_LOG2 */


/* Enable the logging of intermediate search results for virtual index */
/* #define SRCH_SEARCH_ENABLE_INTERMEDIATE_SEARCH_RESULT_LOGGING_FOR_VIRTUAL_INDICES */


/* Enable this to round off total search results when they are estimated */
/* #define SRCH_SEARCH_ENABLE_ROUNDING_FOR_ESTIMATED_TOTAL_SEARCH_RESULTS */


/* Enable caseless metaphone */
/* #define SRCH_SEARCH_ENABLE_CASELESS_METAPHONE */

/* Enable caseless phonix */
/* #define SRCH_SEARCH_ENABLE_CASELESS_PHONIX */

/* Enable caseless soundex */
/* #define SRCH_SEARCH_ENABLE_CASELESS_SOUNDEX */


/*---------------------------------------------------------------------------*/


/*
** Defines
*/

/* Virtual index open error strings */
#define SRCH_SEARCH_VIRTUAL_INDEX_OPEN_ERROR_IGNORE                 (unsigned char *)"ignore"
#define SRCH_SEARCH_VIRTUAL_INDEX_OPEN_ERROR_FAIL                   (unsigned char *)"fail"


/* Index name separators */
#define SRCH_SEARCH_INDEX_NAME_SEPARATORS                           (unsigned char *)", "


/* Index regex start and end strings */
#define SRCH_SEARCH_INDEX_REGEX_START                               (unsigned char *)"["
#define SRCH_SEARCH_INDEX_REGEX_END                                 (unsigned char *)"]"


/* Type definition for index type */
#define SRCH_SEARCH_INDEX_TYPE_INVALID                              (0)
#define SRCH_SEARCH_INDEX_TYPE_PHYSICAL                             (1)
#define SRCH_SEARCH_INDEX_TYPE_VIRTUAL                              (2)

#define SRCH_SEARCH_INDEX_TYPE_VALID(n)                             (((n) >= SRCH_SEARCH_INDEX_TYPE_PHYSICAL) && \
                                                                            ((n) <= SRCH_SEARCH_INDEX_TYPE_VIRTUAL))


/*---------------------------------------------------------------------------*/


/* Sort field types */
#define SRCH_SEARCH_SORT_FIELD_TYPE_INVALID                         (0)
#define SRCH_SEARCH_SORT_FIELD_TYPE_RELEVANCE                       (1)
#define SRCH_SEARCH_SORT_FIELD_TYPE_RANK                            (2)
#define SRCH_SEARCH_SORT_FIELD_TYPE_DATE                            (3)
#define SRCH_SEARCH_SORT_FIELD_TYPE_ITEM_UINT                       (4)
#define SRCH_SEARCH_SORT_FIELD_TYPE_ITEM_ULONG                      (5)
#define SRCH_SEARCH_SORT_FIELD_TYPE_ITEM_FLOAT                      (6)
#define SRCH_SEARCH_SORT_FIELD_TYPE_ITEM_DOUBLE                     (7)
#define SRCH_SEARCH_SORT_FIELD_TYPE_ITEM_UCHAR                      (8)
#define SRCH_SEARCH_SORT_FIELD_TYPE_NO_SORT                         (9)

#define SRCH_SEARCH_SORT_FIELD_TYPE_VALID(n)                        (((n) >= SRCH_SEARCH_SORT_FIELD_TYPE_RELEVANCE) && \
                                                                            ((n) <= SRCH_SEARCH_SORT_FIELD_TYPE_NO_SORT))


/* Default sort field type */
#define SRCH_SEARCH_SORT_FIELD_TYPE_DEFAULT                         SRCH_SEARCH_SORT_FIELD_TYPE_RELEVANCE

/* Default sort order ID */
#define SRCH_SEARCH_PARSER_SORT_ORDER_ID_DEFAULT                    SRCH_PARSER_SORT_ORDER_DESC_ID


/*---------------------------------------------------------------------------*/


/* Ranking algorithm default */
#define SRCH_SEARCH_RANKING_ALGORITHM_DEFAULT                       (unsigned char *)"fsclt-1"


/*---------------------------------------------------------------------------*/


/* Minimum weight for a document, this is used when we apply relevance feedback
** to a postings list, the weight for a document could be reduced below 0 if
** we have negative relevance feedback, but we need to keep the document in the 
** postings list so we set its weight to the minimal document weight, this is
** because feedback merely reorders documents in the postings list, it does not
** eliminate them (still with me). 
*/
#define SRCH_SEARCH_DOCUMENT_WEIGHT_MINIMAL                         (1e-10)


/*---------------------------------------------------------------------------*/


/* Term distance for NEAR operator default */
#define SRCH_SEARCH_TERM_NEAR_DISTANCE_DEFAULT                      (10)


/*---------------------------------------------------------------------------*/


/* Long string length */
#define SRCH_SEARCH_LONG_STRING_LENGTH                              (4096)


/*---------------------------------------------------------------------------*/


/*
** Structures
*/

/* Search index sort structure */
struct srchSearchIndexSort {
    wchar_t         pwcSortFieldName[SPI_FIELD_NAME_MAXIMUM_LENGTH + 1];                /* Sort field name */
    unsigned int    uiSortOrderID;                                                      /* Sort order ID */
};    


/* Search index structure */
struct srchSearchIndex {
    unsigned int                    uiIndexType;                                        /* Index type */
    unsigned char                   pucIndexName[SPI_INDEX_NAME_MAXIMUM_LENGTH + 1];    /* Index name */
    struct srchIndex                **ppsiSrchIndexList;                                /* Index structure list */
    unsigned int                    uiSrchIndexListLength;                              /* Indices structure list length */
    struct srchSearchIndexSort      *psisSrchSearchIndexSorts;                          /* Search index sorts array */
    unsigned int                    uiSrchSearchIndexSortsLength;                       /* Search index sorts array length */
    boolean                         bIgnoreIndexOpenError;                              /* Open error flag */
};    


/*---------------------------------------------------------------------------*/


/*
** Private function prototypes
*/

static int iSrchSearchCheckSearchConfiguration (struct spiSession *pssSpiSession);

static int iSrchSearchGetIndexName (struct srchSearch *pssSrchSearch, struct srchIndex *psiSrchIndex,
        unsigned char *pucIndexName, unsigned int uiIndexNameLength);

static int iSrchSearchProcessText (struct srchSearch *pssSrchSearch, unsigned char *pucText, wchar_t **ppwcSearchText);

static int iSrchSearchGetShortResultsFromSearch (struct srchSearch *pssSrchSearch, struct srchIndex *psiSrchIndex, 
        unsigned int uiLanguageID, wchar_t *pwcSearchText, wchar_t *pwcPositiveFeedbackText, 
        wchar_t *pwcNegativeFeedbackText, unsigned int uiStartIndex, unsigned int uiEndIndex, 
        unsigned int uiSortFieldType, unsigned int uiSortType, struct srchShortResult **ppssrSrchShortResults, 
        unsigned int *puiSrchShortResultsLength, unsigned int *puiTotalResults, double *pdMaxSortKey);

static int iSrchSearchGetRawResultsFromSearch (struct srchSearch *pssSrchSearch, struct srchIndex *psiSrchIndex,
        unsigned int uiLanguageID, wchar_t *pwcSearchText, wchar_t *pwcPositiveFeedbackText, 
        wchar_t *pwcNegativeFeedbackText, unsigned int uiSortFieldType, unsigned int uiSortType, 
        struct srchPostingsList **ppsplSrchPostingsList, struct srchWeight **ppswSrchWeight, boolean *pbDocumentTable, 
        struct srchBitmap **ppsbSrchBitmapExclusion, struct srchBitmap **ppsbSrchBitmapInclusion);

static int iSrchSearchGetShortResultsFromRawResults (struct srchSearch *pssSrchSearch, struct srchIndex *psiSrchIndex, 
        struct srchPostingsList *psplSrchPostingsList, struct srchWeight *pswSrchWeight, boolean bDocumentTable, 
        struct srchBitmap *psbSrchBitmapExclusion, struct srchBitmap *psbSrchBitmapInclusion, 
        unsigned int uiSortFieldType, unsigned int uiSortType, 
        struct srchShortResult **ppssrSrchShortResults, unsigned int *puiSrchShortResultsLength,
        unsigned int *puiTotalResults, double *pdMaxSortKey);

static int iSrchSearchGetSpiSearchResultsFromShortResults (struct srchSearch *pssSrchSearch, struct srchIndex *psiSrchIndex, 
        struct srchShortResult *pssrSrchShortResults, unsigned int uiSrchShortResultsLength, struct spiSearchResult *pssrSpiSearchResults,
        unsigned int uiSortType);

static int iSrchSearchGetPostingsListFromParserTermCluster (struct srchSearch *pssSrchSearch, struct srchIndex *psiSrchIndex,
        unsigned int uiLanguageID, struct srchParserTermCluster *psptcSrchParserTermCluster, 
        struct srchPostingsList **ppsplSrchPostingsList);

static int iSrchSearchGetPostingsListFromParserTerm (struct srchSearch *pssSrchSearch, struct srchIndex *psiSrchIndex,
        unsigned int uiLanguageID, struct srchParserTerm *psptSrchParserTerm, unsigned int uiStartDocumentID, unsigned int uiEndDocumentID,
        struct srchPostingsList **ppsplSrchPostingsList);


static int iSrchSearchFilterNumberAgainstSrchParserNumbers (unsigned long ulNumber, struct srchParserNumber *ppspnSrchParserNumber, 
        unsigned int uiSrchParserNumberLength);


static int iSrchSearchAddEntryToSpiDocumentItems (unsigned char *pucItemName, unsigned char *pucMimeType, 
        unsigned char *pucUrl, unsigned int uiLength, void *pvData, unsigned int uiDataLength, boolean bCopyData, 
        struct spiDocumentItem **ppsdiSpiDocumentItems, unsigned int *puiDocumentItemsLength);


/*---------------------------------------------------------------------------*/


/* 
** ==============================
** ===  Server SPI Functions  ===
** ==============================
*/


/*

    Function:   iSpiInitializeServer()

    Purpose:    This is called when the server is initialized.

                NOTE - This function may be called more than once

    Parameters: pssSpiSession   spi session structure

    Globals:    none

    Returns:    SPI Error Code

*/
int iSpiInitializeServer
(
    struct spiSession *pssSpiSession
)
{
    
    int                 iError = SPI_NoError;
    struct srchSearch   *pssSrchSearch = NULL;
    unsigned char       pucConfigValue[SRCH_INFO_SYMBOL_MAXIMUM_LENGTH + 1] = {'\0'};


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iSpiInitializeServer."); */


    /* Return an error if the spi session structure is not specified */
    if ( pssSpiSession == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pssSpiSession' parameter passed to 'iSpiInitializeServer'."); 
        return (SPI_InvalidSession);
    }


    /* Check for the index directory path */
    if ( bUtlStringsIsStringNULL(pssSpiSession->pucIndexDirectoryPath) == true ) {
        return (SPI_InvalidIndexDirectory);
    }

    /* Check that that index directory path can be accessed */
    if ( !((bUtlFileIsDirectory(pssSpiSession->pucIndexDirectoryPath) == true) || 
            (bUtlFilePathRead(pssSpiSession->pucIndexDirectoryPath) == true) || 
            (bUtlFilePathExec(pssSpiSession->pucIndexDirectoryPath) == true)) ) {
        return (SPI_InvalidIndexDirectory);
    }


    /* Check for the configuration directory path */
    if ( bUtlStringsIsStringNULL(pssSpiSession->pucConfigurationDirectoryPath) == true ) {
        return (SPI_InvalidConfigurationDirectory);
    }

    /* Check that that configuration directory path can be accessed */
    if ( !((bUtlFileIsDirectory(pssSpiSession->pucConfigurationDirectoryPath) == true) || 
            (bUtlFilePathRead(pssSpiSession->pucConfigurationDirectoryPath) == true) || 
            (bUtlFilePathExec(pssSpiSession->pucConfigurationDirectoryPath) == true)) ) {
        return (SPI_InvalidConfigurationDirectory);
    }


    /* Check for the temporary directory path */
    if ( bUtlStringsIsStringNULL(pssSpiSession->pucTemporaryDirectoryPath) == true ) {
        return (SPI_InvalidTemporaryDirectory);
    }

    /* Check that that temporary directory path can be accessed */
    if ( !((bUtlFileIsDirectory(pssSpiSession->pucTemporaryDirectoryPath) == true) || 
            (bUtlFilePathRead(pssSpiSession->pucTemporaryDirectoryPath) == true) || 
            (bUtlFilePathExec(pssSpiSession->pucTemporaryDirectoryPath) == true)) ) {
        return (SPI_InvalidTemporaryDirectory);
    }


    /* Shutdown the server if the spi session structure is already allocated, this is
    ** to handle situations where iSpiInitializeServer() is called more than once
    ** (note that the search structure is stored in the client pointer of the spi session structure)
    */
    if ( pssSpiSession->pvClientPtr != NULL ) {
        iSpiShutdownServer(pssSpiSession);
    }


    /* Create a search structure */
    if ( (pssSrchSearch = (struct srchSearch *)s_malloc((size_t)(sizeof(struct srchSearch)))) == NULL ) { 
        return (SPI_MemError);
    }

    /* And place it in the client pointer of the spi session structure */
    pssSpiSession->pvClientPtr = (void *)pssSrchSearch;


    /* Make a copy of the index directory */
    s_strnncpy(pssSrchSearch->pucIndexDirectoryPath, pssSpiSession->pucIndexDirectoryPath, UTL_FILE_PATH_MAX + 1);

    /* Make a copy of the configuration directory path */
    s_strnncpy(pssSrchSearch->pucConfigurationDirectoryPath, pssSpiSession->pucConfigurationDirectoryPath, UTL_FILE_PATH_MAX + 1);

    /* Make a copy of the temporary directory path */
    s_strnncpy(pssSrchSearch->pucTemporaryDirectoryPath, pssSpiSession->pucTemporaryDirectoryPath, UTL_FILE_PATH_MAX + 1);


    /* Create the search configuration file path */
    if ( (iError = iUtlFileMergePaths(pssSrchSearch->pucConfigurationDirectoryPath, SRCH_SEARCH_CONFIG_FILE_NAME, pssSrchSearch->pucConfigurationFilePath, UTL_FILE_PATH_MAX + 1)) != UTL_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to create the search configuration file path, search configuration file name: '%s', search configuration directory path: '%s', utl error: %d.", 
                SRCH_SEARCH_CONFIG_FILE_NAME, pssSrchSearch->pucConfigurationDirectoryPath, iError); 
        iError = SPI_InitializeServerFailed;
        goto bailFromiSrchInitializeServer;
    }

    /* Preset the last status change time of the search configuration file */
    pssSrchSearch->tConfigurationFileLastStatusChange = -1;

    /* Open the search configuration - search configuration file may or may not be there */
    if ( (iError = iUtlConfigOpen(pssSrchSearch->pucConfigurationFilePath, UTL_CONFIG_FILE_FLAG_OPTIONAL, (void **)&pssSrchSearch->pvUtlConfig)) != UTL_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to open the search configuration, search configuration file: '%s', utl error: %d.", 
                pssSrchSearch->pucConfigurationFilePath, iError); 
        iError = SPI_InitializeServerFailed;
        goto bailFromiSrchInitializeServer;
    }


    /* Check the version if the search configuration file exists, and get its last status change time */
    if ( bUtlStringsIsStringNULL(pssSrchSearch->pucConfigurationFilePath) == true ) {
        /* Don't complain if the configuration file path is not defined */;
    }
    else if ( bUtlFilePathExists(pssSrchSearch->pucConfigurationFilePath) == false ) {
        iUtlLogWarn(UTL_LOG_CONTEXT, "Failed to find the search configuration file: '%s'.", pssSrchSearch->pucConfigurationFilePath); 
    }
    else if ( bUtlFileIsFile(pssSrchSearch->pucConfigurationFilePath) == false ) {
        iUtlLogWarn(UTL_LOG_CONTEXT, "Failed to find the search configuration file: '%s'.", pssSrchSearch->pucConfigurationFilePath); 
    }
    else if ( bUtlFilePathRead(pssSrchSearch->pucConfigurationFilePath) == false ) {
        iUtlLogWarn(UTL_LOG_CONTEXT, "Failed to access the search configuration file: '%s'.", pssSrchSearch->pucConfigurationFilePath); 
    }
    else {

        /* Get the version */
        if ( (iError = iUtlConfigGetValue(pssSrchSearch->pvUtlConfig, SRCH_SEARCH_CONFIG_VERSION, pucConfigValue, SRCH_INFO_SYMBOL_MAXIMUM_LENGTH + 1)) != UTL_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the version number from the search configuration, config key: '%s', utl error: %d.", SRCH_SEARCH_CONFIG_VERSION, iError); 
            iError = SPI_InitializeServerFailed;
            goto bailFromiSrchInitializeServer;
        }
        else {
            unsigned int    uiMajorVersion = 0;
            unsigned int    uiMinorVersion = 0;
            unsigned int    uiPatchVersion = 0;
            
            /* Scan for the version numbers, no need for a scan string here are these are just numbers */
            if ( sscanf(pucConfigValue, "%u.%u.%u", &uiMajorVersion, &uiMinorVersion, &uiPatchVersion) != 3 ) {
                iUtlLogError(UTL_LOG_CONTEXT, "Invalid version number in the search configuration, expected: %u.%u.%u, config key: '%s', config value: '%s'.", 
                        UTL_VERSION_MAJOR, UTL_VERSION_MINOR, UTL_VERSION_PATCH, SRCH_SEARCH_CONFIG_VERSION, pucConfigValue); 
                iError = SPI_InitializeServerFailed;
                goto bailFromiSrchInitializeServer;
            }
    
            /* Check the version numbers we got against the internal version, note that we don't check the patch version */
            if ( !((uiMajorVersion == UTL_VERSION_MAJOR) && (uiMinorVersion <= UTL_VERSION_MINOR)) ) {
                iUtlLogError(UTL_LOG_CONTEXT, "Invalid version number in the search configuration, expected: %u.%u.%u or less, config key: '%s', config value: '%s'.", 
                        UTL_VERSION_MAJOR, UTL_VERSION_MINOR, UTL_VERSION_PATCH, SRCH_SEARCH_CONFIG_VERSION, pucConfigValue); 
                iError = SPI_InitializeServerFailed;
                goto bailFromiSrchInitializeServer;
            }
        }
        
        
        /* Get the last status change time of the search configuration file */
        if ( (iError = iUtlFileGetPathStatusChangeTimeT(pssSrchSearch->pucConfigurationFilePath, &pssSrchSearch->tConfigurationFileLastStatusChange)) != UTL_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the last status change time of the search configuration file: '%s', utl error: %d.", 
                    pssSrchSearch->pucConfigurationFilePath, iError); 
            iError = SPI_InitializeServerFailed;
            goto bailFromiSrchInitializeServer;
        }
    }


    /* Create the search report */
    if ( (iError = iSrchReportCreate(pssSrchSearch, (void **)&pssSrchSearch->pvSrchReport)) != SRCH_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to initialize the search report, srch error: %d.", iError);
        goto bailFromiSrchInitializeServer;
    }


    /* Create the search cache */
    if ( (iError = iSrchCacheCreate(pssSrchSearch, (void **)&pssSrchSearch->pvSrchCache)) != SRCH_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to initialize the search cache, srch error: %d.", iError);
        goto bailFromiSrchInitializeServer;
    }


    /* Create the search parser */
    if ( (iError = iSrchParserCreate(pssSrchSearch, (void **)&pssSrchSearch->pvSrchParser)) != SRCH_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to initialize the search parser, srch error: %d.", iError);
        goto bailFromiSrchInitializeServer;
    }


    /* Create the unicode normalizer */
    iError = iLngUnicodeNormalizerCreate(pssSrchSearch->pucConfigurationDirectoryPath, (void **)&pssSrchSearch->pvLngUnicodeNormalizer);
    
    /* Handle the error */
    if ( iError == LNG_UnicodeNormalizationUnsupported ) {
        iUtlLogDebug(UTL_LOG_CONTEXT, "Unicode normalization is not supported.");
        iError = SPI_NoError;
    }
    else if ( iError != LNG_NoError ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to create a unicode normalizer, lng error: %d", iError);
        iError = SPI_InitializeServerFailed;
        goto bailFromiSrchInitializeServer;
    }


    /* Create the converter, from utf-8 to wide characters */
    if ( (iError = iLngConverterCreateByName(LNG_CHARACTER_SET_UTF_8_NAME, LNG_CHARACTER_SET_WCHAR_NAME, (void **)&pssSrchSearch->pvLngConverter)) != LNG_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to create a character set converter to convert from utf-8 to wide characters, lng error: %d.", iError);
        iError = SPI_InitializeServerFailed;
        goto bailFromiSrchInitializeServer;
    }



    /* Bail label */
    bailFromiSrchInitializeServer:
    

    /* Handle the error */
    if ( iError != SPI_NoError ) {

        /* Adjust the error */
        iError = SPI_ERROR_VALID(iError) ? iError : SPI_InitializeServerFailed;

        /* Shutdown the server if there was an error initializing it, ignoring errors */
        iSpiShutdownServer(pssSpiSession);
    }


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSpiShutdownServer()

    Purpose:    This is called when the server is being shut down.

                NOTE - This function may be called more than once

    Parameters: pssSpiSession   spi session structure

    Globals:    none

    Returns:    SPI Error Code

*/
int iSpiShutdownServer
(
    struct spiSession *pssSpiSession
)
{
    
    struct srchSearch   *pssSrchSearch = NULL;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iSpiShutdownServer."); */


    /* Return an error if the spi session structure is not specified */
    if ( pssSpiSession == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pssSpiSession' parameter passed to 'iSpiShutdownServer'."); 
        return (SPI_InvalidSession);
    }


    /* Shutdown the server if the spi session structure is allocated, this is to handle 
    ** situations where iSpiShutdownServer() is called more than once
    ** (note that the search structure is stored in the client pointer of the spi session structure)
    */
    if ( pssSpiSession->pvClientPtr != NULL ) {

        /* Dereference the search structure from the client pointer */
        pssSrchSearch = (struct srchSearch *)pssSpiSession->pvClientPtr;

        /* Free the search configuration */
        if ( pssSrchSearch->pvUtlConfig != NULL ) {
            iUtlConfigClose(pssSrchSearch->pvUtlConfig);
            pssSrchSearch->pvUtlConfig = NULL;
        }

        /* Close the search report */
        if ( pssSrchSearch->pvSrchReport != NULL ) {
            iSrchReportClose(pssSrchSearch->pvSrchReport);
            pssSrchSearch->pvSrchReport = NULL;
        }

        /* Close the search cache */
        if ( pssSrchSearch->pvSrchCache != NULL ) {
            iSrchCacheClose(pssSrchSearch->pvSrchCache);
            pssSrchSearch->pvSrchCache = NULL;
        }

        /* Free the search parser */
        if ( pssSrchSearch->pvSrchParser != NULL ) {
            iSrchParserFree(pssSrchSearch->pvSrchParser);
            pssSrchSearch->pvSrchParser = NULL;
        }

        /* Free the unicode normalizer */
        if ( pssSrchSearch->pvLngUnicodeNormalizer != NULL ) {
            iLngUnicodeNormalizerFree(pssSrchSearch->pvLngUnicodeNormalizer);
            pssSrchSearch->pvLngUnicodeNormalizer = NULL;
        }

        /* Free the converter */
        if ( pssSrchSearch->pvLngConverter != NULL ) {
            iLngConverterFree(pssSrchSearch->pvLngConverter);
            pssSrchSearch->pvLngConverter = NULL;
        }
    
        /* Free the search structure */
        s_free(pssSrchSearch);

        /* Null out the client pointer of the spi session structure */
        pssSpiSession->pvClientPtr = NULL;
    }


    /* Return but dont report an error */
    return (SPI_NoError);

}


/*---------------------------------------------------------------------------*/


/* 
** =============================
** ===  Index SPI Functions  ===
** =============================
*/


/*

    Function:   iSpiOpenIndex()

    Purpose:    This function will be called to open a index before any
                operations are performed on it.

                This function must allocate the index structure returned. 
                Note that the index structure is opaque to the SPI framework 
                so you really can  get any size of pointer (even NULL, though 
                this may not be a good idea).

    Parameters: pssSpiSession   spi session structure
                pucIndexName    index name
                ppvIndex        return pointer for the index structure

    Globals:    none

    Returns:    SPI Error Code

*/
int iSpiOpenIndex
(
    struct spiSession *pssSpiSession,
    unsigned char *pucIndexName,
    void **ppvIndex
)
{

    int                         iError = SRCH_NoError;
    struct srchSearch           *pssSrchSearch = NULL;
    unsigned char               pucConfigKey[SRCH_INFO_SYMBOL_MAXIMUM_LENGTH + 1] = {'\0'};
    unsigned char               pucConfigValue[SRCH_INFO_SYMBOL_MAXIMUM_LENGTH + 1] = {'\0'};
    struct srchSearchIndex      *pssiSrchSearchIndex = NULL;
    unsigned int                uiI = 0;

    unsigned char               **ppucIndexNameList = NULL;
    unsigned int                uiIndexNameListLength = 0;

    /* These are here so we are sure they get released in case of an error */
    regex_t                     *prRegex = NULL;
    unsigned char               **ppucDirectoryEntryList = NULL;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iSpiOpenIndex."); */


    /* Check the parameters */
    if ( pssSpiSession == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pssSpiSession' parameter passed to 'iSpiOpenIndex'."); 
        return (SPI_InvalidSession);
    }

    if ( bUtlStringsIsStringNULL(pucIndexName) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pucIndexName' parameter passed to 'iSpiOpenIndex'."); 
        return (SPI_InvalidIndexName);
    }

    if ( ppvIndex == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppvIndex' parameter passed to 'iSpiOpenIndex'."); 
        return (SPI_ReturnParameterError);
    }


    /* Check the search configuration */
    if ( (iError = iSrchSearchCheckSearchConfiguration(pssSpiSession)) != SPI_NoError ) {
        return (SPI_OpenIndexFailed);
    }

    /* Dereference the search structure from the client pointer and check it */
    if ( (pssSrchSearch = (struct srchSearch *)pssSpiSession->pvClientPtr) == NULL ) {
        return (SPI_InvalidSession);
    }


    /* Allocate the search index structure */
    if ( (pssiSrchSearchIndex = (struct srchSearchIndex *)s_malloc((size_t)(sizeof(struct srchSearchIndex)))) == NULL ) {
        iError = SPI_MemError;
        goto bailFromiSrchOpenIndex;
    }
    
    /* Copy the index name */
    s_strnncpy(pssiSrchSearchIndex->pucIndexName, pucIndexName, SPI_INDEX_NAME_MAXIMUM_LENGTH + 1);
    
    /* Set the index list pointer and length */
    pssiSrchSearchIndex->ppsiSrchIndexList = NULL;
    pssiSrchSearchIndex->uiSrchIndexListLength = 0;

    /* Set search sorts to defaults, no sort field name, no sort order */
    pssiSrchSearchIndex->psisSrchSearchIndexSorts = NULL;
    pssiSrchSearchIndex->uiSrchSearchIndexSortsLength = 0;
    
    /* Dont ignore index open errors by default */
    pssiSrchSearchIndex->bIgnoreIndexOpenError = false;



    /* Check the configuration file to see if this is a virtual index */
    if ( iUtlConfigGetValue1(pssSrchSearch->pvUtlConfig, SRCH_SEARCH_CONFIG_VIRTUAL_INDEX, pucIndexName, pucConfigValue, SRCH_INFO_SYMBOL_MAXIMUM_LENGTH + 1) != UTL_NoError ) {

        /* This index is a physical index, so we just add the index name to the index name list */
        unsigned char   **ppucIndexNameListPtr = NULL;

        /* Physical index */
        pssiSrchSearchIndex->uiIndexType = SRCH_SEARCH_INDEX_TYPE_PHYSICAL;

        /* Extend the index names array */
        if ( (ppucIndexNameListPtr = (unsigned char **)s_realloc(ppucIndexNameList, (size_t)(sizeof(struct srchIndex *) * (uiIndexNameListLength + 1)))) == NULL ) {
            iError = SPI_MemError;
            goto bailFromiSrchOpenIndex;
        }
    
        /* Hand over the pointer */
        ppucIndexNameList = ppucIndexNameListPtr;
    
        /* Add the index name */
        if ( (ppucIndexNameList[uiIndexNameListLength] = (unsigned char *)s_strdup(pucIndexName)) == NULL ) {
            iError = SPI_MemError;
            goto bailFromiSrchOpenIndex;
        }

/*         iUtlLogDebug(UTL_LOG_CONTEXT, "pucIndexName: '%s'.", pucIndexName); */
    
        /* Increment the index name list length */
        uiIndexNameListLength++;
    }
    else {

        /* This index is a virtual index, so we need to parse the index names and add them to the index name list */
        unsigned char   *pucIndexNamePtr = NULL;
        unsigned char   *pucIndexNameStrtokPtr = NULL;

        /* Virtual index */
        pssiSrchSearchIndex->uiIndexType = SRCH_SEARCH_INDEX_TYPE_VIRTUAL;

        /* Loop parsing the index names */
        for ( pucIndexNamePtr = s_strtok_r(pucConfigValue, SRCH_SEARCH_INDEX_NAME_SEPARATORS, (char **)&pucIndexNameStrtokPtr); 
                pucIndexNamePtr != NULL; 
                pucIndexNamePtr = s_strtok_r(NULL, SRCH_SEARCH_INDEX_NAME_SEPARATORS, (char **)&pucIndexNameStrtokPtr) ) {

            /* This index name is a regex, so we need to expand it */
            if ( (s_strncmp(pucIndexNamePtr, SRCH_SEARCH_INDEX_REGEX_START, s_strlen(SRCH_SEARCH_INDEX_REGEX_START)) == 0) &&
                    (s_strcmp(pucIndexNamePtr + s_strlen(pucIndexNamePtr) - s_strlen(SRCH_SEARCH_INDEX_REGEX_END), SRCH_SEARCH_INDEX_REGEX_END) == 0) ) {
    
                unsigned char   pucRegex[SRCH_INFO_SYMBOL_MAXIMUM_LENGTH + 1] = {'\0'};
                int             iStatus = 0;
                unsigned char   **ppucDirectoryEntryListPtr = NULL;
    
                /* Extract the regex */
                s_strnncpy(pucRegex, pucIndexNamePtr + s_strlen(SRCH_SEARCH_INDEX_REGEX_START), s_strlen(pucIndexNamePtr) - s_strlen(SRCH_SEARCH_INDEX_REGEX_END));
    
                /* Allocate the buffer for the regex structure */
                if ( (prRegex = (regex_t *)s_malloc((size_t)sizeof(regex_t))) == NULL ) {
                    iError = SPI_MemError;
                    goto bailFromiSrchOpenIndex;
                }
    
                /* Create the regex structure */
                if ( (iStatus = s_regcomp(prRegex, pucRegex, REG_EXTENDED | REG_NOSUB)) != 0 ) {
                    iError = SPI_OpenIndexFailed;
                    goto bailFromiSrchOpenIndex;
                }
                
                /* Scan the index directory for file/directory names, if it has not already been scanned,
                ** we do this so that we don't scan the directory multiple times
                */
                if ( ppucDirectoryEntryList == NULL ) {
                    if ( (iError = iUtlFileScanDirectory(pssSrchSearch->pucIndexDirectoryPath, NULL, NULL, &ppucDirectoryEntryList)) != UTL_NoError ) {
                        iUtlLogError(UTL_LOG_CONTEXT, "Failed to read the contents of the index directory path: '%s', utl error: %d.", pssSrchSearch->pucIndexDirectoryPath, iError);
                        iError = SPI_OpenIndexFailed;
                        goto bailFromiSrchOpenIndex;
                    }
                }

                /* Loop over all the entries picked up in the scan, assembling a 
                ** new index name from the entries which match the regex
                */
                if ( ppucDirectoryEntryList != NULL ) {
                    
                    for ( ppucDirectoryEntryListPtr = ppucDirectoryEntryList; *ppucDirectoryEntryListPtr != NULL; ppucDirectoryEntryListPtr++ ) {
                    
                        /* Add entry if it matches the regex */ 
                        if ( (iStatus = s_regexec(prRegex, *ppucDirectoryEntryListPtr, (size_t)0, NULL, 0)) == 0 ) {
                            
                            unsigned char   **ppucIndexNameListPtr = NULL;
    
                            /* Extend the index name list */
                            if ( (ppucIndexNameListPtr = (unsigned char **)s_realloc(ppucIndexNameList, (size_t)(sizeof(struct srchIndex *) * (uiIndexNameListLength + 1)))) == NULL ) {
                                iError = SPI_MemError;
                                goto bailFromiSrchOpenIndex;
                            }
                        
                            /* Hand over the pointer */
                            ppucIndexNameList = ppucIndexNameListPtr;
                        
                            /* Add the index name */
                            if ( (ppucIndexNameList[uiIndexNameListLength] = (unsigned char *)s_strdup(*ppucDirectoryEntryListPtr)) == NULL ) {
                                iError = SPI_MemError;
                                goto bailFromiSrchOpenIndex;
                            }
    
/*                             iUtlLogDebug(UTL_LOG_CONTEXT, "ppucIndexNameList[%u]: '%s'.", uiIndexNameListLength, ppucIndexNameList[uiIndexNameListLength]); */
                        
                            /* Increment the index name list length */
                            uiIndexNameListLength++;
                        }
                    }
                }
                
                /* Free the regex */
                s_regfree(prRegex);
                s_free(prRegex);
            }

            /* This index name is not a regex, so we just add it to the index name list */
            else {

                unsigned char   **ppucIndexNameListPtr = NULL;

                /* Extend the index name list */
                if ( (ppucIndexNameListPtr = (unsigned char **)s_realloc(ppucIndexNameList, (size_t)(sizeof(struct srchIndex *) * (uiIndexNameListLength + 1)))) == NULL ) {
                    iError = SPI_MemError;
                    goto bailFromiSrchOpenIndex;
                }
            
                /* Hand over the pointer */
                ppucIndexNameList = ppucIndexNameListPtr;
            
                /* Add the index name */
                if ( (ppucIndexNameList[uiIndexNameListLength] = (unsigned char *)s_strdup(pucIndexNamePtr)) == NULL ) {
                    iError = SPI_MemError;
                    goto bailFromiSrchOpenIndex;
                }

/*                 iUtlLogDebug(UTL_LOG_CONTEXT, "ppucIndexNameList[%u]: '%s'.", uiIndexNameListLength, ppucIndexNameList[uiIndexNameListLength]); */
            
                /* Increment the index name list length */
                uiIndexNameListLength++;
            }
        }
        
        /* Free the directory list */
        iUtlFileFreeDirectoryEntryList(ppucDirectoryEntryList);
        ppucDirectoryEntryList = NULL;



        /* Check what the index open error policy is if any */
        if ( iUtlConfigGetValue1(pssSrchSearch->pvUtlConfig, SRCH_SEARCH_CONFIG_VIRTUAL_INDEX_OPEN_ERROR, pucIndexName, pucConfigValue, SRCH_INFO_SYMBOL_MAXIMUM_LENGTH + 1) == UTL_NoError ) {
            if ( s_strncasecmp(pucConfigValue, SRCH_SEARCH_VIRTUAL_INDEX_OPEN_ERROR_IGNORE, s_strlen(SRCH_SEARCH_VIRTUAL_INDEX_OPEN_ERROR_IGNORE)) == 0 ) {
                pssiSrchSearchIndex->bIgnoreIndexOpenError = true;
            }
            else if ( s_strncasecmp(pucConfigValue, SRCH_SEARCH_VIRTUAL_INDEX_OPEN_ERROR_FAIL, s_strlen(SRCH_SEARCH_VIRTUAL_INDEX_OPEN_ERROR_FAIL)) == 0 ) {
                pssiSrchSearchIndex->bIgnoreIndexOpenError = false;
            }
            else {
                iUtlLogWarn(UTL_LOG_CONTEXT, "Invalid virtual index open error handling found in the search configuration file, virtual index: '%s', config key: '%s', config value: '%s', defaulting to: '%s'.", 
                        pucIndexName, pucConfigKey, pucConfigValue, SRCH_SEARCH_VIRTUAL_INDEX_OPEN_ERROR_FAIL);
                pssiSrchSearchIndex->bIgnoreIndexOpenError = false;
            }
        }


        /* Check what the index sorts are if any */
        if ( iUtlConfigGetValue1(pssSrchSearch->pvUtlConfig, SRCH_SEARCH_CONFIG_VIRTUAL_INDEX_SORT_ORDERS, pucIndexName, pucConfigValue, SRCH_INFO_SYMBOL_MAXIMUM_LENGTH + 1) == UTL_NoError ) {

            unsigned char   *pucIndexSortOrderPtr = NULL;
            unsigned char   *pucIndexSortOrdersStrtokPtr = NULL;
            

            /* Loop parsing the index sort orders */
            for ( pucIndexSortOrderPtr = s_strtok_r(pucConfigValue, SRCH_SEARCH_INDEX_NAME_SEPARATORS, (char **)&pucIndexSortOrdersStrtokPtr); 
                    pucIndexSortOrderPtr != NULL; 
                    pucIndexSortOrderPtr = s_strtok_r(NULL, SRCH_SEARCH_INDEX_NAME_SEPARATORS, (char **)&pucIndexSortOrdersStrtokPtr) ) {

                void            *pvSrchParser = NULL;
                wchar_t         pwcIndexSortOrder[SRCH_INFO_SYMBOL_MAXIMUM_LENGTH + 1] = {L'\0'};
                wchar_t         *pwcSortFieldName = NULL;
                unsigned int    uiSortOrderID = SRCH_PARSER_SORT_ORDER_NONE_ID;


                /* Create the search parser */
                if ( (iError = iSrchParserCreate(pssSrchSearch, &pvSrchParser)) != SRCH_NoError ) {
                    iUtlLogError(UTL_LOG_CONTEXT, "Failed to initialize the search parser, srch error: %d.", iError);
                    iError = SPI_OpenIndexFailed;
                    goto bailFromiSrchOpenIndex;
                }
                
                /* Convert the index sort order from utf-8 to wide characters */
                if ( (iError = iLngConvertUtf8ToWideString_s(pucIndexSortOrderPtr, 0, pwcIndexSortOrder, SRCH_INFO_SYMBOL_MAXIMUM_LENGTH + 1)) != LNG_NoError ) {
                    iUtlLogError(UTL_LOG_CONTEXT, "Failed to convert the virtual index sort order from the search configuration file from utf-8 to wide characters, virtual index: '%s', sort order: '%ls', config key: '%s', config value: '%s', lng error: %d.", 
                            pucIndexName, pwcIndexSortOrder, pucConfigKey, pucConfigValue, iError);
                    iError = SPI_OpenIndexFailed;
                    goto bailFromiSrchOpenIndex;
                }

                /* Parse the sort order */
                if ( (iError = iSrchParserParse(pvSrchParser, LNG_LANGUAGE_ANY_ID, LNG_TOKENIZER_FSCLT_1_ID, pwcIndexSortOrder)) != SRCH_NoError ) {
                    iUtlLogError(UTL_LOG_CONTEXT, "Failed to parse the virtual index sort order from the search configuration file, virtual index: '%s', sort order: '%ls', config key: '%s', config value: '%s', srch error: %d.", 
                            pucIndexName, pwcIndexSortOrder, pucConfigKey, pucConfigValue, iError);
                    iError = SPI_OpenIndexFailed;
                    goto bailFromiSrchOpenIndex;
                }
                
                /* Get the sort order */
                if ( (iError = iSrchParserGetSort(pvSrchParser, &pwcSortFieldName, &uiSortOrderID)) != SRCH_NoError ) {
                    iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the virtual index sort order info from the search configuration file, virtual index: '%s', sort order: '%ls', config key: '%s', config value: '%s', srch error: %d.", 
                            pucIndexName, pwcIndexSortOrder, pucConfigKey, pucConfigValue, iError);
                    iError = SPI_OpenIndexFailed;
                    goto bailFromiSrchOpenIndex;
                }

                
                /* Add an entry to the search sorts array if this is a valid sort field name and sort order ID */
                if ( (bUtlStringsIsWideStringNULL(pwcSortFieldName) == false) && 
                        ((uiSortOrderID == SRCH_PARSER_SORT_ORDER_ASC_ID) || (uiSortOrderID == SRCH_PARSER_SORT_ORDER_DESC_ID)) ) {

                    struct srchSearchIndexSort  *psisSrchSearchIndexSortsPtr = NULL;
                
                    /* Extend the search index sorts array */
                    if ( (psisSrchSearchIndexSortsPtr = (struct srchSearchIndexSort *)s_realloc(pssiSrchSearchIndex->psisSrchSearchIndexSorts, 
                            (size_t)(sizeof(struct srchSearchIndexSort) * (pssiSrchSearchIndex->uiSrchSearchIndexSortsLength + 1)))) == NULL ) {
                        iError = SPI_MemError;
                        goto bailFromiSrchOpenIndex;
                    }
                    
                    /* Hand over the pointer */
                    pssiSrchSearchIndex->psisSrchSearchIndexSorts = psisSrchSearchIndexSortsPtr;
                    
                    /* Dereference the entry in the array */
                    psisSrchSearchIndexSortsPtr = pssiSrchSearchIndex->psisSrchSearchIndexSorts + pssiSrchSearchIndex->uiSrchSearchIndexSortsLength;
    
                    /* Copy the sort field name */
                    s_wcsnncpy(psisSrchSearchIndexSortsPtr->pwcSortFieldName, pwcSortFieldName, SPI_FIELD_NAME_MAXIMUM_LENGTH + 1);
                    
                    /* Set the sort order ID */
                    psisSrchSearchIndexSortsPtr->uiSortOrderID = uiSortOrderID;
    
                    /* Increment the number of entries */
                    pssiSrchSearchIndex->uiSrchSearchIndexSortsLength++;
                }
                else {
                    /* Invalid sort order, so we warn the user */
                    iUtlLogWarn(UTL_LOG_CONTEXT, "Invalid virtual index sort order info found in the search configuration file, virtual index: '%s', sort order: '%ls', config key: '%s', config value: '%s'.", 
                            pucIndexName, pwcIndexSortOrder, pucConfigKey, pucConfigValue);
                }
    
                /* Free the search parser */
                iSrchParserFree(pvSrchParser);
                pvSrchParser = NULL;
            }
        }
    }


    
    /* Check the number of indices */
    if ( uiIndexNameListLength == 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to find any physical indices.");
        iError = SPI_OpenIndexFailed;
        goto bailFromiSrchOpenIndex;
    }



    /* Loop over all the entries in the index name list */
    for ( uiI = 0; uiI < uiIndexNameListLength; uiI++ ) {

        struct srchIndex    *psiSrchIndex = NULL;
        struct srchIndex    **ppsiSrchIndexList = NULL;

/*         iUtlLogDebug(UTL_LOG_CONTEXT, "ppucIndexNameList[%u]: '%s'.", uiI, ppucIndexNameList[uiI]); */

        /* Open the index, add it to the list if successfully opened */
        if ( (iError = iSrchIndexOpen(pssSpiSession->pucIndexDirectoryPath, pssSpiSession->pucConfigurationDirectoryPath, 
                ppucIndexNameList[uiI], SRCH_INDEX_INTENT_SEARCH, &psiSrchIndex)) == SRCH_NoError ) {
        
            /* Extend the index array */
            if ( (ppsiSrchIndexList = (struct srchIndex **)s_realloc(pssiSrchSearchIndex->ppsiSrchIndexList, (size_t)(sizeof(struct srchIndex *) *
                    (pssiSrchSearchIndex->uiSrchIndexListLength + 1)))) == NULL ) {
                iError = SPI_MemError;
                goto bailFromiSrchOpenIndex;
            }
        
            /* Hand over the pointer */
            pssiSrchSearchIndex->ppsiSrchIndexList = ppsiSrchIndexList;
        
            /* Add the index structure */
            pssiSrchSearchIndex->ppsiSrchIndexList[pssiSrchSearchIndex->uiSrchIndexListLength] = psiSrchIndex;
        
            /* Increment the number of index */
            pssiSrchSearchIndex->uiSrchIndexListLength++;
        }
        else {
        
            /* Ignore the error if this is a virtual index and we can ignore errors */
            if ( (pssiSrchSearchIndex->uiIndexType == SRCH_SEARCH_INDEX_TYPE_VIRTUAL) && (pssiSrchSearchIndex->bIgnoreIndexOpenError == true) ) {
                
                /* Inform the user */
                iUtlLogWarn(UTL_LOG_CONTEXT, "Failed to open physical index: '%s', part of virtual index: '%s', srch error: %d, skipping...", ppucIndexNameList[uiI], pucIndexName, iError);
            
                /* Reset the error */
                iError = SPI_NoError;
            }
            else {
                /* The error is set by the iSrchIndexOpen() call above */
                goto bailFromiSrchOpenIndex;
            }
        }
    }



    /* Check that we opened at least one physical index */
    if ( (pssiSrchSearchIndex->uiSrchIndexListLength == 0) || (pssiSrchSearchIndex->ppsiSrchIndexList == NULL) ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to open any physical indices.");
        iError = SPI_OpenIndexFailed;
    }



    /* Bail label */
    bailFromiSrchOpenIndex:


    /* Free the directory list */
    iUtlFileFreeDirectoryEntryList(ppucDirectoryEntryList);
    ppucDirectoryEntryList = NULL;

    /* Free the regex */
    if ( prRegex != NULL ) {
        s_regfree(prRegex);
        s_free(prRegex);
    }

    /* Free the index name list */
    UTL_MACROS_FREE_NUMBERED_LIST(ppucIndexNameList, uiIndexNameListLength);


    /* Handle the error */
    if ( iError == SPI_NoError ) {

        /* Set the return pointer from the index info structure */
        *ppvIndex = (void *)pssiSrchSearchIndex;
    }
    else {

        /* Adjust the error */
        iError = SPI_ERROR_VALID(iError) ? iError : SPI_OpenIndexFailed;

        /* Close the index, ignoring errors, this also releases any allocations made for the index info structure */
        iSpiCloseIndex(pssSpiSession, (void *)pssiSrchSearchIndex);
        pssiSrchSearchIndex = NULL;
    }


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSpiCloseIndex()

    Purpose:    This function will be called after all operations performed
                on this index are done.

                This function must free the index structure passed, it will
                not be referenced anymore. Note that the index structure 
                is opaque to the SPI framework so you really can 
                get any size of pointer (even NULL, though this may not be a 
                good idea). The pointer gets allocated in iSpiOpenIndex.

    Parameters: pssSpiSession   spi session structure
                pvIndex         index structure

    Globals:    none

    Returns:    SPI Error Code

*/
int iSpiCloseIndex
(
    struct spiSession *pssSpiSession,
    void *pvIndex
)
{

    int                         iError = SPI_NoError;
    struct srchSearch           *pssSrchSearch = NULL;
    struct srchSearchIndex      *pssiSrchSearchIndex = NULL;
    unsigned int                uiI = 0;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iSpiCloseIndex."); */


    /* Check the parameters */
    if ( pssSpiSession == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pssSpiSession' parameter passed to 'iSpiCloseIndex'."); 
        return (SPI_InvalidSession);
    }

    if ( pvIndex == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvIndex' parameter passed to 'iSpiCloseIndex'."); 
        return (SPI_InvalidIndex);
    }


    /* Check the search configuration */
    if ( (iError = iSrchSearchCheckSearchConfiguration(pssSpiSession)) != SPI_NoError ) {
        return (SPI_CloseIndexFailed);
    }

    /* Dereference the search structure from the client pointer and check it */
    if ( (pssSrchSearch = (struct srchSearch *)pssSpiSession->pvClientPtr) == NULL ) {
        return (SPI_InvalidSession);
    }

    /* Dereference the index structure and check it */
    if ( (pssiSrchSearchIndex = (struct srchSearchIndex *)pvIndex) == NULL ) {
        return (SPI_InvalidIndex);
    }


    /* Close the index in the array and free all allocated memory */
    if ( pssiSrchSearchIndex != NULL ) {
        
        /* Close the index in the index array, ignoring any errors, and free the array */
        if ( pssiSrchSearchIndex->ppsiSrchIndexList != NULL ) {
            for ( uiI = 0; uiI < pssiSrchSearchIndex->uiSrchIndexListLength; uiI++ ) {
                iSrchIndexClose(pssiSrchSearchIndex->ppsiSrchIndexList[uiI]);
            }
            s_free(pssiSrchSearchIndex->ppsiSrchIndexList);
        }
        
        /* Free the search index sorts array */
        s_free(pssiSrchSearchIndex->psisSrchSearchIndexSorts);

        /* Free the search index structure */
        s_free(pssiSrchSearchIndex);
    }


    return (SPI_NoError);

}


/*---------------------------------------------------------------------------*/


/* 
** =================================
** ===  Searching SPI Functions  ===
** =================================
*/


/*

    Function:   iSpiSearchIndex()

    Purpose:    This function searches the index. 

    Parameters: pssSpiSession               spi session structure
                ppvIndexList                pointer to a NULL terminated list of index structures
                pucLanguageCode             language code (optional)
                pucSearchText               search text (optional)
                pucPositiveFeedbackText     positive feedback text (optional)
                pucNegativeFeedbackText     negative feedback text (optional)
                uiStartIndex                start index
                uiEndIndex                  end index, 0 if there is no end index
                ppssrSpiSearchResponse      return pointer for the spi search response structure

    Globals:    none

    Returns:    SPI Error Code

*/
int iSpiSearchIndex
(
    struct spiSession *pssSpiSession,
    void **ppvIndexList,
    unsigned char *pucLanguageCode,
    unsigned char *pucSearchText,
    unsigned char *pucPositiveFeedbackText,
    unsigned char *pucNegativeFeedbackText,
    unsigned int uiStartIndex,
    unsigned int uiEndIndex,
    struct spiSearchResponse **ppssrSpiSearchResponse
)
{

    int                         iError = SPI_NoError;
    unsigned int                uiIndexCount = 0;
    unsigned int                uiLanguageID = LNG_LANGUAGE_ANY_ID;
    unsigned int                uiSearchTokenizerID = LNG_TOKENIZER_ANY_ID;
    wchar_t                     *pwcSearchText = NULL;
    wchar_t                     *pwcPositiveFeedbackText = NULL;
    wchar_t                     *pwcNegativeFeedbackText = NULL;
    int                         iSrchParserError = SPI_NoError;
    struct srchSearch           *pssSrchSearch = NULL;
    unsigned int                uiI = 0, uiJ = 0, uiK = 0;
    struct srchShortResult      *pssrMainSrchShortResults = NULL;
    unsigned int                uiMainSrchShortResultsLength = 0;
    struct srchShortResult      *pssrSrchShortResults = NULL;
    struct srchShortResult      *pssrSrchShortResultsPtr = NULL;
    unsigned int                uiSrchShortResultsLength = 0;
    struct spiSearchResult      *pssrSpiSearchResults = NULL;
    struct spiSearchResult      *pssrSpiSearchResultsPtr = NULL;
    unsigned int                uiSpiSearchResultsLength = 0;
    unsigned int                uiSrchParserSortOrderID = SRCH_PARSER_INVALID_ID;
    wchar_t                     *pwcSrchParserSortFieldName = NULL;
    unsigned int                uiSortFieldType = SRCH_SEARCH_SORT_FIELD_TYPE_INVALID;
    boolean                     bValidSortField = true;
    unsigned int                uiSrchParserBooleanOperatorID = SRCH_PARSER_INVALID_ID;
    unsigned int                uiSrchParserBooleanOperationID = SRCH_PARSER_INVALID_ID;
    unsigned int                uiSrchParserTermCaseID = SRCH_PARSER_INVALID_ID;
    unsigned int                uiSrchParserOperatorCaseID = SRCH_PARSER_INVALID_ID;
    unsigned int                uiSrchParserSearchTypeID = SRCH_PARSER_INVALID_ID;
    unsigned int                uiSrchParserSearchResultsID = SRCH_PARSER_MODIFIER_UNKNOWN_ID;
    unsigned int                uiSrchParserSearchReportID = SRCH_PARSER_MODIFIER_UNKNOWN_ID;
    unsigned int                uiSrchParserDebugID = SRCH_PARSER_MODIFIER_UNKNOWN_ID;
    unsigned int                uiSrchParserEarlyCompletionID = SRCH_PARSER_MODIFIER_UNKNOWN_ID;
    unsigned char               pucSearchReportKey[UTL_FILE_PATH_MAX + 1] = {'\0'};
    unsigned char               pucNumberString[UTL_FILE_PATH_MAX + 1] = {'\0'};
    
    unsigned int                uiTotalResultsMain = 0;
    unsigned int                uiTotalResults = 0;
    unsigned int                uiSortType = SPI_SORT_TYPE_UNKNOWN;
    double                      dMaxSortKeyMain = 0;
    double                      dMaxSortKey = 0;
    
    boolean                     bSearchResultsTotalMainEstimated = false;
    
    unsigned char               pucConfigValue[SRCH_INFO_SYMBOL_MAXIMUM_LENGTH + 1] = {'\0'};
    
    /* To calculate the search time */
    struct timeval              tvSearchStartTimeVal;
    double                      dSearchTime = 0;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iSpiSearchIndex."); */
/*     iUtlLogInfo(UTL_LOG_CONTEXT, "iSpiSearchIndex - search: '%s'.", pucUtlStringsGetSafeString(pucSearchText)); */


    /* Check the parameters */
    if ( pssSpiSession == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pssSpiSession' parameter passed to 'iSpiSearchIndex'."); 
        return (SPI_InvalidSession);
    }

    if ( ppvIndexList == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppvIndexList' parameter passed to 'iSpiSearchIndex'."); 
        return (SPI_InvalidIndex);
    }

    if ( ppvIndexList[0] == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Empty 'ppvIndexList' parameter passed to 'iSpiSearchIndex'."); 
        return (SPI_InvalidIndex);
    }

    if ( uiStartIndex < 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiStartIndex'  parameter passed to 'iSpiSearchIndex'."); 
        return (SPI_InvalidSearchResultsRange);
    }

    if ( uiEndIndex < 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiEndIndex'  parameter passed to 'iSpiSearchIndex'."); 
        return (SPI_InvalidSearchResultsRange);
    }

    if ( uiStartIndex > uiEndIndex ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiStartIndex' and 'uiEndIndex' parameters passed to 'iSpiSearchIndex'."); 
        return (SPI_InvalidSearchResultsRange);
    }

    if ( ppssrSpiSearchResponse == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppssrSpiSearchResponse' parameter passed to 'iSpiSearchIndex'."); 
        return (SPI_ReturnParameterError);
    }

    

    /* Check the search configuration */
    if ( (iError = iSrchSearchCheckSearchConfiguration(pssSpiSession)) != SPI_NoError ) {
        return (SPI_SearchIndexFailed);
    }

    /* Dereference the search structure from the client pointer and check it */
    if ( (pssSrchSearch = (struct srchSearch *)pssSpiSession->pvClientPtr) == NULL ) {
        return (SPI_InvalidSession);
    }


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "pucSearchText: '%s'.", pucUtlStringsGetSafeString(pucSearchText)); */


    /* Count up the number of index we are searching */
    for ( uiI = 0, uiIndexCount = 0; ppvIndexList[uiI] != NULL; uiI++ ) {
        
        /* Dereference the index info structure for convenience */
        struct srchSearchIndex      *pssiSrchSearchIndex = (struct srchSearchIndex *)ppvIndexList[uiI];
    
        /* Increment the index count */
        uiIndexCount += pssiSrchSearchIndex->uiSrchIndexListLength;
    }

    /* Check that at least one index was specified */
    if ( uiIndexCount == 0 ) {
        iError = SPI_InvalidIndex;
        goto bailFromiSrchSearchIndex;
    }


    /* Check the search language code if we were passed one */
    if ( bUtlStringsIsStringNULL(pucLanguageCode) == false ) {
        
        /* Get the language ID for the search language code */
        if ( (iError = iLngGetLanguageIDFromCode(pucLanguageCode, &uiLanguageID)) != LNG_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the language ID from the language code: '%s', lng error: %d.", pucLanguageCode, iError); 
            iError = SRCH_SearchIndexFailed;
            goto bailFromiSrchSearchIndex;
        }
    }
    else {
        iUtlLogWarn(UTL_LOG_CONTEXT, "No search language was specified.");
    }


    /* Get the tokenizer ID from the first index that provides us with one that is valid */
    for ( uiI = 0, uiSearchTokenizerID = LNG_TOKENIZER_ANY_ID; ppvIndexList[uiI] != NULL; uiI++ ) {
        
        /* Dereference the index info structure for convenience */
        struct srchSearchIndex      *pssiSrchSearchIndex = (struct srchSearchIndex *)ppvIndexList[uiI];

        /* Loop over all the index structures in this index info structure */
        for ( uiJ = 0; uiJ < pssiSrchSearchIndex->uiSrchIndexListLength; uiJ++ ) {
        
            /* Dereference the index structure for convenience */
            struct srchIndex *psiSrchIndex = pssiSrchSearchIndex->ppsiSrchIndexList[uiJ];
    
            /* Set the search tokenizer ID from the first valid index tokenizer ID and bail */
            if ( uiSearchTokenizerID == LNG_TOKENIZER_ANY_ID ) {
                uiSearchTokenizerID = psiSrchIndex->uiTokenizerID;
                break;
            }
        }
        
        /* Bail if the search tokenizer ID is set */
        if ( uiSearchTokenizerID != LNG_TOKENIZER_ANY_ID ) {
            break;
        }
    }


    
    /* Process the search text */
    if ( (iError = iSrchSearchProcessText(pssSrchSearch, pucSearchText, &pwcSearchText)) != SRCH_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to process the search text: '%s', srch error: %d.", pucSearchText, iError);
        goto bailFromiSrchSearchIndex;
    }

    /* Process the positive feedback text */
    if ( (iError = iSrchSearchProcessText(pssSrchSearch, pucPositiveFeedbackText, &pwcPositiveFeedbackText)) != SRCH_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to process the positive feedback text: '%s', srch error: %d.", pucPositiveFeedbackText, iError);
        goto bailFromiSrchSearchIndex;
    }

    /* Process the negative feedback text */
    if ( (iError = iSrchSearchProcessText(pssSrchSearch, pucNegativeFeedbackText, &pwcNegativeFeedbackText)) != SRCH_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to process the negative feedback text: '%s', srch error: %d.", pucNegativeFeedbackText, iError);
        goto bailFromiSrchSearchIndex;
    }



    /* Get the search maximum number of documents to return as the result of a search */
    if ( iUtlConfigGetValue(pssSrchSearch->pvUtlConfig, SRCH_SEARCH_CONFIG_SEARCH_MAXIMUM_DOCUMENTS_RETURNED, pucConfigValue, SRCH_INFO_SYMBOL_MAXIMUM_LENGTH + 1) == UTL_NoError ) {
        
        if ( s_strtol(pucConfigValue, NULL, 10) <= 0 ) {
            iUtlLogWarn(UTL_LOG_CONTEXT, "Invalid search maximum documents returned found in the search configuration file, config key: '%s', config value: '%s'.", 
                    SRCH_SEARCH_CONFIG_SEARCH_MAXIMUM_DOCUMENTS_RETURNED, pucConfigValue);
        }
        else {
            
            unsigned int    uiMaxDocumentsReturned = s_strtol(pucConfigValue, NULL, 10);
        
            /* Check to see if we are requesting more documents than we are allowed */
            if ( (uiEndIndex - uiStartIndex) > uiMaxDocumentsReturned ) {
                /* We are, so we log a warning and adjust the number of documents */
                iUtlLogWarn(UTL_LOG_CONTEXT, "Search requested: %u documents, maximum allowed: %u document.", (uiEndIndex - uiStartIndex) + 1, uiMaxDocumentsReturned);
                uiEndIndex = (uiStartIndex + uiMaxDocumentsReturned) - 1;
            }
            else if ( (uiStartIndex == 0) && (uiEndIndex == 0) ) {
                /* We are, so we log a warning and adjust the number of documents */
                iUtlLogWarn(UTL_LOG_CONTEXT, "Search requested all the documents, maximum allowed: %u document.", uiMaxDocumentsReturned);
                uiEndIndex = uiMaxDocumentsReturned - 1;
            }
        }
    }


    
    /* Set the search start time */
    s_gettimeofday(&tvSearchStartTimeVal, NULL);


    /* Create the search report, ignoring any errors, so we don't know if the 
    ** search report is created until we get the search report key later 
    ** on with iSrchReportGetReportKey(), but right now we don't care since
    ** we just won't add the search report to the search results if one 
    ** could not be created
    */
    iSrchReportCreateReport(pssSrchSearch->pvSrchReport);


    /* Parse the search text, delay reporting errors until we have
    ** grabbed reporting and debug flags from the parser
    */
    iSrchParserError = iSrchParserParse(pssSrchSearch->pvSrchParser, uiLanguageID, uiSearchTokenizerID, pwcSearchText);
        

    /* Get the parser search report ID */
    if ( (iError = iSrchParserGetModifierID(pssSrchSearch->pvSrchParser, SRCH_PARSER_MODIFIER_SEARCH_REPORT_ID, &uiSrchParserSearchReportID)) != SRCH_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the parser search report ID, srch error: %d.", iError);
        goto bailFromiSrchSearchIndex;
    }

    /* Get the parser debug ID */
    if ( (iError = iSrchParserGetModifierID(pssSrchSearch->pvSrchParser, SRCH_PARSER_MODIFIER_DEBUG_ID, &uiSrchParserDebugID)) != SRCH_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the parser debug ID, srch error: %d.", iError);
        goto bailFromiSrchSearchIndex;
    }


    /* Check that the search report was created if the search report flag is set to true, 
    ** reset that flag to false if the search report was not created, doing this saves us
    ** from adding an entry to the search results to accomodate the search report
    */
    if ( uiSrchParserSearchReportID == SRCH_PARSER_MODIFIER_SEARCH_REPORT_RETURN_ID ) {
        if ( iSrchReportGetReportKey(pssSrchSearch->pvSrchReport, pucSearchReportKey, UTL_FILE_PATH_MAX + 1) != SRCH_NoError ) {
            uiSrchParserSearchReportID = SRCH_PARSER_MODIFIER_SEARCH_REPORT_SUPPRESS_ID;
        }
    }

    
    /* Search report - extra debug information */
    if ( uiSrchParserDebugID == SRCH_PARSER_MODIFIER_DEBUG_ENABLE_ID ) {
        if ( iUtlDateGetWebDateFromTime((time_t)0, pucNumberString, UTL_FILE_PATH_MAX) == UTL_NoError ) {
            iSrchReportAppend(pssSrchSearch->pvSrchReport, "%s Date: %s\n", REP_SEARCH_DEBUG, pucNumberString);
        }
        else {
            iSrchReportAppend(pssSrchSearch->pvSrchReport, "%s Date: (unknown)\n", REP_SEARCH_DEBUG);
        }
/*         iSrchReportAppend(pssSrchSearch->pvSrchReport, "%s Server: %s\n", REP_SEARCH_DEBUG, UTL_VERSION_COPYRIGHT_MESSAGE); */
        iSrchReportAppend(pssSrchSearch->pvSrchReport, "%s Version: %u.%u.%u\n", REP_SEARCH_DEBUG, UTL_VERSION_MAJOR, UTL_VERSION_MINOR, UTL_VERSION_PATCH);

        iSrchReportAppend(pssSrchSearch->pvSrchReport, "%s Search Language: '%s'\n", REP_SEARCH_DEBUG, pucUtlStringsGetPrintableString(pucLanguageCode));
    }


    /* Finally we check the parser error */
    if ( iSrchParserError != SRCH_NoError ) {
        
        wchar_t         *pwcSrchParserNormalizedSearchText = NULL;
        unsigned char   pucErrorString[SRCH_SEARCH_LONG_STRING_LENGTH + 1] = {'\0'};

        /* Get the parser normalized search text, get the full or regular depending on whether debug is on or not */
        if ( uiSrchParserDebugID == SRCH_PARSER_MODIFIER_DEBUG_ENABLE_ID ) {
            if ( (iError = iSrchParserGetFullNormalizedSearchText(pssSrchSearch->pvSrchParser, &pwcSrchParserNormalizedSearchText)) != SRCH_NoError ) {
                iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the parser full normalized search text, srch error: %d.", iError);
                goto bailFromiSrchSearchIndex;
            }
        }
        else {
            if ( (iError = iSrchParserGetNormalizedSearchText(pssSrchSearch->pvSrchParser, &pwcSrchParserNormalizedSearchText)) != SRCH_NoError ) {
                iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the parser normalized search text, srch serror: %d.", iError);
                goto bailFromiSrchSearchIndex;
            }
        }

        /* Search report */
        if ( bUtlStringsIsWideStringNULL(pwcSearchText) == false ) {
            iSrchReportAppend(pssSrchSearch->pvSrchReport, "%s %ls\n", REP_SEARCH_ORIGINAL, pwcSearchText);
        }
        if ( bUtlStringsIsWideStringNULL(pwcSrchParserNormalizedSearchText) == false ) {
            /* Append the normalized search text to the search report */
            iSrchReportAppend(pssSrchSearch->pvSrchReport, "%s %ls\n", REP_SEARCH_REFORMATTED, pwcSrchParserNormalizedSearchText);
        }
        iSrchReportAppend(pssSrchSearch->pvSrchReport, "%s ", REP_SEARCH_ERROR);


        /* Get the error text, default to an unknown error */
        if ( iSrchParserGetErrorText(iSrchParserError, pucErrorString, SRCH_SEARCH_LONG_STRING_LENGTH + 1) != SRCH_NoError ) {
            s_strnncpy(pucErrorString, "Unknown parser error)", SRCH_SEARCH_LONG_STRING_LENGTH + 1);
        }

        /* Append the search text to the error string */
        if ( bUtlStringsIsWideStringNULL(pwcSearchText) == false ) {
            iUtlLogWarn(UTL_LOG_CONTEXT, "%s, search: '%ls'.", pucErrorString, pwcSearchText);
            iSrchReportAppend(pssSrchSearch->pvSrchReport, "%s, search: '%ls'\n", pucErrorString, pwcSearchText);
        }
        else {
            iUtlLogWarn(UTL_LOG_CONTEXT, "%s.", pucErrorString);
            iSrchReportAppend(pssSrchSearch->pvSrchReport, "%s\n", pucErrorString);
        }
    }
    else {

        /* Get the parser search results ID */
        if ( (iError = iSrchParserGetModifierID(pssSrchSearch->pvSrchParser, SRCH_PARSER_MODIFIER_SEARCH_RESULTS_ID, &uiSrchParserSearchResultsID)) != SRCH_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the parser search results ID, srch error: %d.", iError);
            goto bailFromiSrchSearchIndex;
        }
    
        /* Get the parser early completion ID */
        if ( (iError = iSrchParserGetModifierID(pssSrchSearch->pvSrchParser, SRCH_PARSER_MODIFIER_EARLY_COMPLETION_ID, &uiSrchParserEarlyCompletionID)) != SRCH_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the parser early completion ID, srch error: %d.", iError);
            goto bailFromiSrchSearchIndex;
        }
    
        /* Get the parser boolean operator ID */
        if ( (iError = iSrchParserGetModifierID(pssSrchSearch->pvSrchParser, SRCH_PARSER_MODIFIER_BOOLEAN_OPERATOR_ID, &uiSrchParserBooleanOperatorID)) != SRCH_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the parser boolean operator ID, srch error: %d.", iError);
            goto bailFromiSrchSearchIndex;
        }
    
        /* Get the parser boolean operation ID */
        if ( (iError = iSrchParserGetModifierID(pssSrchSearch->pvSrchParser, SRCH_PARSER_MODIFIER_BOOLEAN_OPERATION_ID, &uiSrchParserBooleanOperationID)) != SRCH_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the parser boolean operation ID, srch error: %d.", iError);
            goto bailFromiSrchSearchIndex;
        }
    
        /* Get the parser term case ID */
        if ( (iError = iSrchParserGetModifierID(pssSrchSearch->pvSrchParser, SRCH_PARSER_MODIFIER_TERM_CASE_ID, &uiSrchParserTermCaseID)) != SRCH_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the parser term case ID, srch error: %d.", iError);
            goto bailFromiSrchSearchIndex;
        }
    
        /* Get the parser operator case ID */
        if ( (iError = iSrchParserGetModifierID(pssSrchSearch->pvSrchParser, SRCH_PARSER_MODIFIER_OPERATOR_CASE_ID, &uiSrchParserOperatorCaseID)) != SRCH_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the parser operator case ID, srch error: %d.", iError);
            goto bailFromiSrchSearchIndex;
        }
    
        /* Get the parser search type ID */
        if ( (iError = iSrchParserGetModifierID(pssSrchSearch->pvSrchParser, SRCH_PARSER_MODIFIER_SEARCH_TYPE_ID, &uiSrchParserSearchTypeID)) != SRCH_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the parser search type ID, srch error: %d.", iError);
            goto bailFromiSrchSearchIndex;
        }

        /* Get the parser sort field name and parser sort order ID */
        if ( (iError = iSrchParserGetSort(pssSrchSearch->pvSrchParser, &pwcSrchParserSortFieldName, &uiSrchParserSortOrderID)) != SRCH_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the parser sort field name and parser sort order ID, srch error: %d.", iError);
            goto bailFromiSrchSearchIndex;
        }


        /* Sort by relevance */
        if ( (pwcSrchParserSortFieldName != NULL) && (s_wcscasecmp(pwcSrchParserSortFieldName, SRCH_PARSER_SORT_FIELD_NAME_RELEVANCE_WSTRING) == 0) ) {
            uiSortFieldType = SRCH_SEARCH_SORT_FIELD_TYPE_RELEVANCE;
        }
        /* Sort by rank */
        else if ( (pwcSrchParserSortFieldName != NULL) && (s_wcscasecmp(pwcSrchParserSortFieldName, SRCH_PARSER_SORT_FIELD_NAME_RANK_WSTRING) == 0) ) {
            uiSortFieldType = SRCH_SEARCH_SORT_FIELD_TYPE_RANK;
        }
        /* Sort by date */
        else if ( (pwcSrchParserSortFieldName != NULL) && (s_wcscasecmp(pwcSrchParserSortFieldName, SRCH_PARSER_SORT_FIELD_NAME_DATE_WSTRING) == 0) ) {
            uiSortFieldType = SRCH_SEARCH_SORT_FIELD_TYPE_DATE;
        }
        /* There is a sort field defined but it is not an internal sort field  */
        else if ( pwcSrchParserSortFieldName != NULL ) {
                
            unsigned char   pucSrchParserSortFieldName[SRCH_INFO_SYMBOL_MAXIMUM_LENGTH + 1] = {'\0'};
            unsigned int    uiItemID = 0;
            unsigned int    uiTempSortFieldType = SRCH_SEARCH_SORT_FIELD_TYPE_DEFAULT;

            /* Convert the sort field name from wide characters to utf-8 */
            if ( (iError = iLngConvertWideStringToUtf8_s(pwcSrchParserSortFieldName, 0, pucSrchParserSortFieldName, SRCH_INFO_SYMBOL_MAXIMUM_LENGTH + 1)) != LNG_NoError ) {
                iUtlLogError(UTL_LOG_CONTEXT, "Failed to convert the sort field name from wide characters to utf-8, lng error: %d.", iError);
                iError = SPI_SearchIndexFailed;
                goto bailFromiSrchSearchIndex;
            }

            /* Loop through all the index and see if the sort field exists in all of them */
            for ( uiI = 0, uiSortFieldType = SRCH_SEARCH_SORT_FIELD_TYPE_INVALID, bValidSortField = true; ppvIndexList[uiI] != NULL; uiI++ ) {
                
                struct srchSearchIndex    *pssiSrchSearchIndex = (struct srchSearchIndex *)ppvIndexList[uiI];

                /* Loop over the index in this index list */
                for ( uiJ = 0; uiJ < pssiSrchSearchIndex->uiSrchIndexListLength; uiJ++ ) {
        
                    boolean             bFoundItem = false;
                    struct srchIndex    *psiSrchIndex = pssiSrchSearchIndex->ppsiSrchIndexList[uiJ];
                
                    /* Loop over each item, set the flag to not found */
                    for ( uiK = 1, bFoundItem = false; ; uiK++ ) {
                        
                        unsigned char   pucItemName[SPI_ITEM_NAME_MAXIMUM_LENGTH + 1] = {'\0'};
                        unsigned char   pucMimeType[SPI_MIME_TYPE_MAXIMUM_LENGTH + 1] = {'\0'};
                        
                        /* Get the item info */
                        if ( (iError = iSrchInfoGetItemInfo(psiSrchIndex, uiK, pucItemName, SPI_ITEM_NAME_MAXIMUM_LENGTH + 1, pucMimeType, SPI_MIME_TYPE_MAXIMUM_LENGTH + 1)) == SRCH_NoError ) {
                            
                            /* If the item name is the same as the sort field, we set the flag telling
                            ** us that the item was found, we set the item name ID and break
                            */
                            if ( s_strcasecmp(pucItemName, pucSrchParserSortFieldName) == 0 ) {
                                bFoundItem = true;
                                uiItemID = uiK;
                                break;
                            }
                        }
                        /* We failed to get the item name, either there are no more items to get
                        ** or this is an error, either way, we cant go on so we break
                        */
                        else {
                            break;
                        }
                    }
                
                
                    /* We set the temp sort field ID to 'item' (as opposed to 'relevance' or 'date')
                    ** if the item name ID is valid, ie if there is an item name which matches the sort field
                    ** else we default to descending relevance
                    */
                    if ( bFoundItem == true ) {
                        
                        unsigned int    uiFieldID = 0;
                        unsigned int    uiFieldType = SRCH_INFO_FIELD_TYPE_NONE_ID;
    
                        /* Set the sort order - default */
                        uiTempSortFieldType = SRCH_SEARCH_SORT_FIELD_TYPE_DEFAULT;
    
                        /* Look up the field ID for the sort field name, this makes the BIG assumption 
                        ** that the indexed field and the stored field are the same, 
                        ** note that we skip over errors
                        */
                        if ( (iError = iSrchInfoGetFieldID(psiSrchIndex, pucSrchParserSortFieldName, &uiFieldID)) == SRCH_NoError ) {
                            
                            /* Get the field type */
                            if ( (iError = iSrchInfoGetFieldInfo(psiSrchIndex, uiFieldID, NULL, 0, NULL, 0, &uiFieldType, NULL)) == SRCH_NoError ) {
    
                                /* Set the sort field type based on the field type */
                                switch ( uiFieldType ) {
                        
                                    case SRCH_INFO_FIELD_TYPE_CHAR_ID:
                                        uiTempSortFieldType = SRCH_SEARCH_SORT_FIELD_TYPE_ITEM_UCHAR;
                                        break;
                        
                                    case SRCH_INFO_FIELD_TYPE_INT_ID:
                                        uiTempSortFieldType = SRCH_SEARCH_SORT_FIELD_TYPE_ITEM_UINT;
                                        break;

                                    case SRCH_INFO_FIELD_TYPE_LONG_ID:
                                        uiTempSortFieldType = SRCH_SEARCH_SORT_FIELD_TYPE_ITEM_ULONG;
                                        break;

                                    case SRCH_INFO_FIELD_TYPE_FLOAT_ID:
                                        uiTempSortFieldType = SRCH_SEARCH_SORT_FIELD_TYPE_ITEM_FLOAT;
                                        break;

                                    case SRCH_INFO_FIELD_TYPE_DOUBLE_ID:
                                        uiTempSortFieldType = SRCH_SEARCH_SORT_FIELD_TYPE_ITEM_DOUBLE;
                                        break;

                                    default:
                                        ASSERT(false);
                                        break;
                                }
                            }
                        }
                    }
                    else {
                        uiTempSortFieldType = SRCH_SEARCH_SORT_FIELD_TYPE_DEFAULT;
                        uiSrchParserSortOrderID = SRCH_SEARCH_PARSER_SORT_ORDER_ID_DEFAULT;
                        bValidSortField = false;
                    }
                    
                    
                    /* Now we set the sort field type to the temp sort field type if
                    ** the former has not been set, else we compare the sort field type
                    ** and the temp sort field type, if these are not the same, which is
                    ** the case if then item is not present in all index, we cant 
                    ** sort by item name so we just used the default
                    */
                    if ( uiSortFieldType == SRCH_SEARCH_SORT_FIELD_TYPE_INVALID ) {
                        uiSortFieldType = uiTempSortFieldType;
                    }
                    else if ( uiSortFieldType != uiTempSortFieldType ) {
                        uiSortFieldType = SRCH_SEARCH_SORT_FIELD_TYPE_DEFAULT;
                        uiSrchParserSortOrderID = SRCH_SEARCH_PARSER_SORT_ORDER_ID_DEFAULT;
                        bValidSortField = false;
                        break;
                    }
                }
            }
        }
        /* Default sort, note that the parser sort order gets changed */
        else if ( uiSrchParserSortOrderID == SRCH_PARSER_SORT_ORDER_DEFAULT_ID ) {
            uiSortFieldType = SRCH_SEARCH_SORT_FIELD_TYPE_DEFAULT;
            uiSrchParserSortOrderID = SRCH_SEARCH_PARSER_SORT_ORDER_ID_DEFAULT;
        }
        /* No sort */
        else if (uiSrchParserSortOrderID == SRCH_PARSER_SORT_ORDER_NONE_ID ) {
            uiSortFieldType = SRCH_SEARCH_SORT_FIELD_TYPE_NO_SORT;
        }
        /* Default sort field and parser sort order */
        else {
            uiSortFieldType = SRCH_SEARCH_SORT_FIELD_TYPE_DEFAULT;
            uiSrchParserSortOrderID = SRCH_SEARCH_PARSER_SORT_ORDER_ID_DEFAULT;
        }


        /* Default the parser sort order if it is not set, though it really should be by now */
        if ( uiSrchParserSortOrderID == SRCH_PARSER_INVALID_ID ) {
            uiSrchParserSortOrderID = SRCH_SEARCH_PARSER_SORT_ORDER_ID_DEFAULT;
        }

    
        /* Set the sort type */
        if ( uiSrchParserSortOrderID == SRCH_PARSER_SORT_ORDER_ASC_ID ) {
        
            switch ( uiSortFieldType ) {
            
                case SRCH_SEARCH_SORT_FIELD_TYPE_ITEM_ULONG:
                case SRCH_SEARCH_SORT_FIELD_TYPE_DATE:
                    uiSortType = SPI_SORT_TYPE_ULONG_ASC;
                    break;

                case SRCH_SEARCH_SORT_FIELD_TYPE_ITEM_FLOAT:
                case SRCH_SEARCH_SORT_FIELD_TYPE_RELEVANCE:
                    uiSortType = SPI_SORT_TYPE_FLOAT_ASC;
                    break;

                case SRCH_SEARCH_SORT_FIELD_TYPE_ITEM_UINT:
                case SRCH_SEARCH_SORT_FIELD_TYPE_RANK:
                    uiSortType = SPI_SORT_TYPE_UINT_ASC;
                    break;

                case SRCH_SEARCH_SORT_FIELD_TYPE_ITEM_DOUBLE:
                    uiSortType = SPI_SORT_TYPE_DOUBLE_ASC;
                    break;

                case SRCH_SEARCH_SORT_FIELD_TYPE_ITEM_UCHAR:
                    uiSortType = SPI_SORT_TYPE_UCHAR_ASC;
                    break;

                default:
                    ASSERT(false);
                    break;
            }
        }
        else if ( uiSrchParserSortOrderID == SRCH_PARSER_SORT_ORDER_DESC_ID ) {
        
            switch ( uiSortFieldType ) {
            
                case SRCH_SEARCH_SORT_FIELD_TYPE_ITEM_ULONG:
                case SRCH_SEARCH_SORT_FIELD_TYPE_DATE:
                    uiSortType = SPI_SORT_TYPE_ULONG_DESC;
                    break;

                case SRCH_SEARCH_SORT_FIELD_TYPE_ITEM_FLOAT:
                case SRCH_SEARCH_SORT_FIELD_TYPE_RELEVANCE:
                    uiSortType = SPI_SORT_TYPE_FLOAT_DESC;
                    break;

                case SRCH_SEARCH_SORT_FIELD_TYPE_ITEM_UINT:
                case SRCH_SEARCH_SORT_FIELD_TYPE_RANK:
                    uiSortType = SPI_SORT_TYPE_UINT_DESC;
                    break;

                case SRCH_SEARCH_SORT_FIELD_TYPE_ITEM_DOUBLE:
                    uiSortType = SPI_SORT_TYPE_DOUBLE_DESC;
                    break;

                case SRCH_SEARCH_SORT_FIELD_TYPE_ITEM_UCHAR:
                    uiSortType = SPI_SORT_TYPE_UCHAR_DESC;
                    break;

                default:
                    ASSERT(false);
                    break;
            }
        }
        else if ( uiSrchParserSortOrderID == SRCH_PARSER_SORT_ORDER_NONE_ID ) {
            uiSortType = SPI_SORT_TYPE_NO_SORT;
        }
        else {
            ASSERT(false);
        }

/*         iUtlLogDebug(UTL_LOG_CONTEXT, "uiSortFieldType = %u, uiSortType = %u", uiSortFieldType, uiSortType); */



        /* Loop through all the index and search each one serially */
        for ( uiI = 0; ppvIndexList[uiI] != NULL; uiI++ ) {
    
            struct srchSearchIndex      *pssiSrchSearchIndex = (struct srchSearchIndex *)ppvIndexList[uiI];

            boolean                     bEarlyCompletion = false;
            boolean                     bReverseOrder = false;
            int                         iIndexListIndex = 0;
            
            unsigned int                uiTotalResultsMainLocal = 0;
            unsigned int                uiTotalResultsLocal = 0;


/*             if ( bUtlLogIsDebug(UTL_LOG_CONTEXT) == true ) { */
/*                 if ( pssiSrchSearchIndex->psisSrchSearchIndexSorts != NULL ) { */
/*                     struct srchSearchIndexSort    *psisSrchSearchIndexSortsPtr = NULL; */
/*                     for ( uiJ = 0, psisSrchSearchIndexSortsPtr = pssiSrchSearchIndex->psisSrchSearchIndexSorts; uiJ < pssiSrchSearchIndex->uiSrchSearchIndexSortsLength; uiJ++, psisSrchSearchIndexSortsPtr++ ) { */
/*                         iUtlLogDebug(UTL_LOG_CONTEXT, "psisSrchSearchIndexSortsPtr->pwcSortFieldName: '%ls', psisSrchSearchIndexSortsPtr->uiSortOrderID: %u",  */
/*                                 pwcUtlStringsGetPrintableWideString(psisSrchSearchIndexSortsPtr->pwcSortFieldName), psisSrchSearchIndexSortsPtr->uiSortOrderID); */
/*                     } */
/*                 } */
/*                 iUtlLogDebug(UTL_LOG_CONTEXT, "pwcSrchParserSortFieldName: '%ls', uiSrchParserSortOrderID: %u", pwcUtlStringsGetPrintableWideString(pwcSrchParserSortFieldName), uiSrchParserSortOrderID); */
/*             } */


            /* Preset the early completion flag */
            bEarlyCompletion = false;

            /* Check for early completion if it was allowed in the search and if we are searching more than one index */
            if ( (uiSrchParserEarlyCompletionID == SRCH_PARSER_MODIFIER_EARLY_COMPLETION_ENABLE_ID) && (pssiSrchSearchIndex->uiSrchIndexListLength > 1) ) {
            
                struct srchSearchIndexSort      *psisSrchSearchIndexSortsPtr = NULL;
                        
                /* Check for early completion if the index has search sort orders, and a sort order ID is set */
                if ( (pssiSrchSearchIndex->psisSrchSearchIndexSorts != NULL) && (uiSrchParserSortOrderID != SRCH_PARSER_SORT_ORDER_NONE_ID) ) {
                
                    /* We can do early completion if the parser sort field name is set and is the same as the index sort field name */
                    if ( pwcSrchParserSortFieldName != NULL ) {
                        
                        /* Loop over all the entries in the search sorts array */
                        for ( uiJ = 0, psisSrchSearchIndexSortsPtr = pssiSrchSearchIndex->psisSrchSearchIndexSorts; uiJ < pssiSrchSearchIndex->uiSrchSearchIndexSortsLength; uiJ++, psisSrchSearchIndexSortsPtr++ ) {
                            
                            /* Set the early completion flag and break out if we match on the field name */
                            if ( s_wcscasecmp(pwcSrchParserSortFieldName, psisSrchSearchIndexSortsPtr->pwcSortFieldName) == 0 ) {
                                bEarlyCompletion = true;
/*                                 iUtlLogDebug(UTL_LOG_CONTEXT, "bEarlyCompletion = true, pwcSrchParserSortFieldName: '%ls'", pwcSrchParserSortFieldName); */
                                break;
                            }
                        }
                    }
                    
                    /* We can do early completion if the sort field is relevance and the index sort field name is relevance */
                    else if ( uiSortFieldType == SRCH_SEARCH_SORT_FIELD_TYPE_RELEVANCE ) {
                        
                        /* Loop over all the entries in the search sorts array */
                        for ( uiJ = 0, psisSrchSearchIndexSortsPtr = pssiSrchSearchIndex->psisSrchSearchIndexSorts; uiJ < pssiSrchSearchIndex->uiSrchSearchIndexSortsLength; uiJ++, psisSrchSearchIndexSortsPtr++ ) {
                            
                            /* Set the early completion flag and break out if the index sort field name is relevance */
                            if ( s_wcscasecmp(psisSrchSearchIndexSortsPtr->pwcSortFieldName, SRCH_PARSER_SORT_FIELD_NAME_RELEVANCE_WSTRING) == 0 ) {
                                bEarlyCompletion = true;
/*                                 iUtlLogDebug(UTL_LOG_CONTEXT, "bEarlyCompletion = true, SRCH_PARSER_SORT_FIELD_NAME_RELEVANCE_WSTRING    : '%ls'", SRCH_PARSER_SORT_FIELD_NAME_RELEVANCE_WSTRING); */
                                break;
                            }
                        }
                    }

                    /* Set the reverse order flag */
                    if ( bEarlyCompletion == true ) {
                        bReverseOrder = (psisSrchSearchIndexSortsPtr->uiSortOrderID != uiSrchParserSortOrderID) ? true : false;
/*                         iUtlLogDebug(UTL_LOG_CONTEXT, "bEarlyCompletion = true, psisSrchSearchIndexSortsPtr->pwcSortFieldName = '%ls', bReverseOrder = %s", psisSrchSearchIndexSortsPtr->pwcSortFieldName, (bReverseOrder == true) ? "true" : "false"); */
                    }            
                }
                
                /* We can do early completion if the sort field is set to no sort */
                else if ( uiSortFieldType == SRCH_SEARCH_SORT_FIELD_TYPE_NO_SORT ) { 
                    /* Set the early completion flag */
                    bEarlyCompletion = true;
/*                     iUtlLogDebug(UTL_LOG_CONTEXT, "bEarlyCompletion = true, uiSortFieldType == SRCH_SEARCH_SORT_FIELD_TYPE_NO_SORT"); */
                }
            }

/*             if ( bEarlyCompletion == false ) { */
/*                 iUtlLogDebug(UTL_LOG_CONTEXT, "bEarlyCompletion = false"); */
/*             } */


            /* Set the initial index list index, we start at one end or the other
            ** of the list depending on whether the order is reversed or not
            */
            iIndexListIndex = (bEarlyCompletion == false) ? 0 : ((bReverseOrder == false) ? 0 : (pssiSrchSearchIndex->uiSrchIndexListLength - 1));

            /* Loop while we are still within the range */
            while ( (iIndexListIndex >= 0) && (iIndexListIndex < pssiSrchSearchIndex->uiSrchIndexListLength) ) {
        
                struct srchIndex    *psiSrchIndex = pssiSrchSearchIndex->ppsiSrchIndexList[iIndexListIndex];


#if defined(SRCH_SEARCH_ENABLE_INTERMEDIATE_SEARCH_RESULT_LOGGING_FOR_VIRTUAL_INDICES)

                /* Set the segment search start time */
                struct timeval    tvSegmentSearchStartTimeVal;
                s_gettimeofday(&tvSegmentSearchStartTimeVal, NULL);

#endif    /* defined(SRCH_SEARCH_ENABLE_INTERMEDIATE_SEARCH_RESULT_LOGGING_FOR_VIRTUAL_INDICES) */

/*                 if ( bEarlyCompletion == true ) { */
/*                     iUtlLogDebug(UTL_LOG_CONTEXT, "Searching: %d - '%s'", iIndexListIndex, psiSrchIndex->pucIndexName); */
/*                 } */

                /* Do the search - note that we override the doc start index, setting it to 0, if this is a multi index search */
                if ( (iError = iSrchSearchGetShortResultsFromSearch(pssSrchSearch, psiSrchIndex, uiLanguageID, pwcSearchText, pwcPositiveFeedbackText,
                        pwcNegativeFeedbackText, ((uiIndexCount > 1) ? 0 : uiStartIndex), uiEndIndex, uiSortFieldType, 
                        uiSortType, &pssrSrchShortResults, &uiSrchShortResultsLength, &uiTotalResults, &dMaxSortKey)) == SRCH_NoError ) {
                    
                    /* Set the search results total */
                    uiTotalResultsLocal += uiTotalResults;
                    uiTotalResultsMainLocal += uiTotalResults;

                    /* Set the main max sort key */
                    dMaxSortKeyMain = UTL_MACROS_MAX(dMaxSortKeyMain, dMaxSortKey);

/*                     if ( bEarlyCompletion == true ) { */
/*                         iUtlLogDebug(UTL_LOG_CONTEXT, "uiTotalResults: %u, cummulated uiTotalResults: %u", uiTotalResults, uiTotalResultsMainLocal); */
/*                     } */

#if defined(SRCH_SEARCH_ENABLE_INTERMEDIATE_SEARCH_RESULT_LOGGING_FOR_VIRTUAL_INDICES)

                    if ( pssiSrchSearchIndex->uiIndexType == SRCH_SEARCH_INDEX_TYPE_VIRTUAL ) {
                    
                        struct timeval      tvSegmentSearchEndTimeVal;
                        struct timeval      tvSegmentSearchDiffTimeVal;
                        double              dSegmentSearchTime = 0;
                    
                        /* Set the search end time */
                        s_gettimeofday(&tvSegmentSearchEndTimeVal, NULL);
                    
                        /* Get the time taken for this search */
                        UTL_DATE_DIFF_TIMEVAL(tvSegmentSearchStartTimeVal, tvSegmentSearchEndTimeVal, tvSegmentSearchDiffTimeVal);
                    
                        /* Turn it into milliseconds */
                        UTL_DATE_TIMEVAL_TO_MILLISECONDS(tvSegmentSearchDiffTimeVal, dSegmentSearchTime);
                    
                        iUtlLogInfo(UTL_LOG_CONTEXT, "Search, index: '%s', search: '%ls', found: %u document%s and returned: %u, in: %.1f milliseconds.", 
                                psiSrchIndex->pucIndexName, pwcUtlStringsGetSafeWideString(pwcSearchText), uiTotalResults, 
                                (uiTotalResults == 1) ? "" : "s", uiSrchShortResultsLength, dSegmentSearchTime);
                    }

#endif    /* defined(SRCH_SEARCH_ENABLE_INTERMEDIATE_SEARCH_RESULT_LOGGING_FOR_VIRTUAL_INDICES) */


                    /* Process the search short results if there were any */
                    if ( uiSrchShortResultsLength > 0 ) {

                        /* Check to see if we already have any short results we have already processed,
                        ** if we do we need to add the new search results to the end of the current
                        ** search results, otherwise we just pass over the pointers and save ourselves
                        ** an allocation and a memory copy
                        */
                        if ( (pssrMainSrchShortResults != NULL) && (uiMainSrchShortResultsLength > 0) ) {
                        
                            struct srchShortResult      *pssrMainSrchShortResultsPtr = NULL;
    
                            /* Make sure we have enough space in our stored short results array for the one we just got */
                            if ( (pssrMainSrchShortResultsPtr = (struct srchShortResult *)s_realloc(pssrMainSrchShortResults, 
                                    (size_t)(sizeof(struct srchShortResult) * (uiMainSrchShortResultsLength + uiSrchShortResultsLength)))) == NULL ) {
                                iError = SPI_MemError;
                                goto bailFromiSrchSearchIndex;
                            }
    
                            /* Hand over the pointer */
                            pssrMainSrchShortResults = pssrMainSrchShortResultsPtr;
    
                            /* Increment the total number of short results retrieved so far */
                            uiMainSrchShortResultsLength += uiSrchShortResultsLength;
    
                            /* Move over the short results array we just got to the stored one */
                            s_memcpy(pssrMainSrchShortResults + (uiMainSrchShortResultsLength - uiSrchShortResultsLength), pssrSrchShortResults, 
                                    uiSrchShortResultsLength * sizeof(struct srchShortResult));
    
                            /* Free the short results returned by the search - note that we dont use the uiSortType()
                            ** function because we dont want to free the pucSortKey if it is allocated
                            */
                            s_free(pssrSrchShortResults);
                        }
                        else {
                            /* Hand over the short results */
                            pssrMainSrchShortResults = pssrSrchShortResults;
                            uiMainSrchShortResultsLength = uiSrchShortResultsLength;
                            pssrSrchShortResults = NULL;
                        }
                    }
                }
                else {

                    /* The error is set by the iSrchSearchGetShortResultsFromSearch() call above */
                    goto bailFromiSrchSearchIndex;
                }
                
                /* We have an opportunity to stop the search early, if the early completion flag is set, 
                ** and we have gathered enough data, and this is not the last index to search
                */
                if ( (bEarlyCompletion == true) && (uiMainSrchShortResultsLength > uiEndIndex) && 
                        (((bReverseOrder == true) && (iIndexListIndex > 0)) || ((bReverseOrder != true) && (iIndexListIndex < (pssiSrchSearchIndex->uiSrchIndexListLength - 1)))) ) {

                    unsigned long   ulDocumentPartialCount = 0;
                    unsigned long   ulDocumentTotalCount = 0;
                    unsigned char   pucNumberString1[UTL_FILE_PATH_MAX + 1] = {'\0'};
                    unsigned char   pucNumberString2[UTL_FILE_PATH_MAX + 1] = {'\0'};

                    /* Add up the number of documents in the index we have searched */
                    for ( uiK = 0; uiK < pssiSrchSearchIndex->uiSrchIndexListLength; uiK++ ) {
                    
                        struct srchIndex    *psdSrchIndexLocal = pssiSrchSearchIndex->ppsiSrchIndexList[uiK];

                        unsigned char       pucIndexName[SPI_INDEX_NAME_MAXIMUM_LENGTH + 1] = {'\0'};
                        unsigned char       pucIndexDescription[SPI_INDEX_DESCRIPTION_MAXIMUM_LENGTH + 1] = {'\0'};
                        unsigned char       *pucIndexDescriptionPtr = NULL;

                        /* Get the index description, default to the index name */
                        if ( iSrchInfoGetDescriptionInfo(psdSrchIndexLocal, pucIndexDescription, SPI_INDEX_DESCRIPTION_MAXIMUM_LENGTH + 1) == SRCH_NoError ) {
                            pucIndexDescriptionPtr = pucIndexDescription;
                        }
                        else if ( iSrchSearchGetIndexName(pssSrchSearch, psdSrchIndexLocal, pucIndexName, SPI_INDEX_NAME_MAXIMUM_LENGTH + 1) == SRCH_NoError ) {
                            pucIndexDescriptionPtr = pucIndexName;
                        }

                        /* Increment the total count */
                        ulDocumentTotalCount += psdSrchIndexLocal->uiDocumentCount;

                        /* Increment the partial count for the index we have searched or add index stats to the search report to fluff up the numbers */
                        if ( ((bReverseOrder != true) && (uiK <= iIndexListIndex)) || ((bReverseOrder == true) && (uiK >= iIndexListIndex)) ) {
                            ulDocumentPartialCount += psdSrchIndexLocal->uiDocumentCount;
                        }
                        else {
                            iSrchReportAppend(pssSrchSearch->pvSrchReport, "%s %s - %u\n", REP_INDEX_NAME, pucUtlStringsGetSafeString(pucIndexDescriptionPtr), uiK);
                            iSrchReportAppend(pssSrchSearch->pvSrchReport, "%s %lu %lu %lu %lu %u\n", REP_INDEX_COUNTS, psdSrchIndexLocal->ulTotalTermCount, 
                                    psdSrchIndexLocal->ulUniqueTermCount, psdSrchIndexLocal->ulTotalStopTermCount, psdSrchIndexLocal->ulUniqueStopTermCount, 
                                    psdSrchIndexLocal->uiDocumentCount);
                        }
                    }

                    /* Guess the total number of search results based on the counts, and override the current value
                    ** (which is wrong anyway since we have not searched all the index
                    */
/*                     iUtlLogDebug(UTL_LOG_CONTEXT, "Original uiTotalResultsMainLocal: %u, uiTotalResultsLocal: %u, ulDocumentPartialCount; %lu, ulDocumentTotalCount: %lu",  */
/*                             uiTotalResultsMainLocal, uiTotalResultsLocal, ulDocumentPartialCount, ulDocumentTotalCount); */
                    
                    uiTotalResultsMainLocal = (uiTotalResultsMainLocal * ((float)ulDocumentTotalCount / ulDocumentPartialCount));

/*                     iUtlLogDebug(UTL_LOG_CONTEXT, "Ajusted uiTotalResultsMainLocal: %u", uiTotalResultsMainLocal); */


                    /* Send a bogus retrieval count to the search report so that the numbers tally */
                    iSrchReportAppend(pssSrchSearch->pvSrchReport, "%s %u 0 0 0 0 0 0\n", REP_RETRIEVAL_COUNTS, uiTotalResultsMainLocal - uiTotalResultsLocal); 
                    
                    /* Tell the user that the search was terminated early */
                    iSrchReportAppend(pssSrchSearch->pvSrchReport, "%s The search was completed early after looking at: %s documents out of: %s documents\n",
                            REP_SEARCH_WARNING, pucUtlStringsFormatUnsignedNumber(ulDocumentPartialCount, pucNumberString1, UTL_FILE_PATH_MAX + 1), 
                            pucUtlStringsFormatUnsignedNumber(ulDocumentTotalCount, pucNumberString2, UTL_FILE_PATH_MAX + 1));
                    
                    /* Set the estimated flag */
                    bSearchResultsTotalMainEstimated = true;

                    /* And break out */
                    break;
                }
            
                
                /* Increment/decrement the index into the index list */
                iIndexListIndex += (bReverseOrder == true) ? -1 : 1;

            }    /* while ( (iIndexListIndex >= 0) && (iIndexListIndex < pssiSrchSearchIndex->uiSrchIndexListLength) ) */


            /* Set the search results total */
            uiTotalResultsMain += uiTotalResultsMainLocal;

/*             iUtlLogDebug(UTL_LOG_CONTEXT, "uiTotalResultsMain: %u, uiTotalResultsMainLocal: %u", uiTotalResultsMain, uiTotalResultsMainLocal); */
        }



#if defined(SRCH_SEARCH_ENABLE_ROUNDING_FOR_ESTIMATED_TOTAL_SEARCH_RESULTS)

        /* If the total number of results was estimated, we round off */
        if ( bSearchResultsTotalMainEstimated == true ) {

            unsigned int    uiTotalResultsMainRounded = 0;

            /* Round off the value and update it */
            if ( iUtlNumRoundNumber((long)uiTotalResultsMain, UTL_NUM_ROUND_THRESHOLD, UTL_NUM_ROUND_ROUNDING, UTL_NUM_ROUND_DIGITS, (long *)&uiTotalResultsMainRounded) == UTL_NoError ) {
                uiTotalResultsMain = uiTotalResultsMainRounded;
                iUtlLogDebug(UTL_LOG_CONTEXT, "Before ==>> uiTotalResultsMain: %u => uiTotalResultsMainRounded: %u", uiTotalResultsMain, uiTotalResultsMainRounded);
            }
        }

#endif    /* defined(SRCH_SEARCH_ENABLE_ROUNDING_FOR_ESTIMATED_TOTAL_SEARCH_RESULTS) */



        /* We have finished the searches, but we need to resort the short results
        ** array if we got any results, if any document are going to be returned
        ** and if we searched more than one index
        */
        if ( (uiMainSrchShortResultsLength > 0) && (uiStartIndex < uiMainSrchShortResultsLength) && (uiIndexCount > 1) ) {
            
            if ( (iError = iSrchShortResultSort(&pssrMainSrchShortResults, uiMainSrchShortResultsLength, uiSortType)) != SRCH_NoError ) {
                iUtlLogError(UTL_LOG_CONTEXT, "Failed to sort the short results: srch error: %d.", iError);
                goto bailFromiSrchSearchIndex;
            }
        }


        /* For debugging */
/*         iSrchShortResultPrint(pssrMainSrchShortResults, uiMainSrchShortResultsLength, uiSortType); */


        /* Splice the short results and handle any errors, the short results will 
        ** already have been spliced if this is a single index search
        */
        if ( (uiIndexCount > 1) && (pssrMainSrchShortResults != NULL) && (uiMainSrchShortResultsLength > 0) ) {
            if ( (iError = iSrchShortResultSplice(&pssrMainSrchShortResults, &uiMainSrchShortResultsLength, uiStartIndex, uiEndIndex, uiSortType)) != SRCH_NoError ) {
                iUtlLogError(UTL_LOG_CONTEXT, "Failed to splice the short results: srch error: %d.", iError);
                goto bailFromiSrchSearchIndex;
            }
        }
        

        /* Set the number of search results from the number of short results */
        uiSpiSearchResultsLength = uiMainSrchShortResultsLength;


        /* We now need to populate the search results, but we only need
        ** to do so if we have at least one search result
        */
        if ( uiSpiSearchResultsLength > 0 ) {

            /* Allocate the search results - add an extra entry for the search report if needed */
            if ( (pssrSpiSearchResults = (struct spiSearchResult *)s_malloc((size_t)(sizeof(struct spiSearchResult) * 
                    (uiSpiSearchResultsLength + ((uiSrchParserSearchReportID == SRCH_PARSER_MODIFIER_SEARCH_REPORT_RETURN_ID) ? 1 : 0)) ))) == NULL ) {
                iError = SPI_MemError;
                goto bailFromiSrchSearchIndex;
            }

            /* Loop over each index we searched */
            for ( uiI = 0; ppvIndexList[uiI] != NULL; uiI++ ) {
        
                struct srchSearchIndex      *pssiSrchSearchIndex = (struct srchSearchIndex *)ppvIndexList[uiI];
        
                /* Loop over the index in this index list */
                for ( uiJ = 0; uiJ < pssiSrchSearchIndex->uiSrchIndexListLength; uiJ++ ) {
            
                    struct srchIndex        *psiSrchIndex = pssiSrchSearchIndex->ppsiSrchIndexList[uiJ];
    
                    /* Fill the search results for that index, note that this adds the actual name of the index for the hit
                    ** not the name of the virtual index if this index was part of a virtual index
                    */
                    if ( (iError != iSrchSearchGetSpiSearchResultsFromShortResults(pssSrchSearch, psiSrchIndex, pssrMainSrchShortResults, 
                            uiMainSrchShortResultsLength, pssrSpiSearchResults, uiSortType)) != SRCH_NoError ) {
                        goto bailFromiSrchSearchIndex;
                    }

                    /* If the index is a virtual one, we need to side effect the document key
                    ** to store the physical index name, so that we know which physical index 
                    ** this document came from and can route future requests to the appropriate 
                    ** physical index, we also 
                    */
                    if ( pssiSrchSearchIndex->uiIndexType == SRCH_SEARCH_INDEX_TYPE_VIRTUAL ) {
                        
                        /* Cycle though all the short results array filling in the search result data */    
                        for ( uiK = 0, pssrSrchShortResultsPtr = pssrMainSrchShortResults, pssrSpiSearchResultsPtr = pssrSpiSearchResults; 
                                uiK < uiMainSrchShortResultsLength; uiK++, pssrSrchShortResultsPtr++, pssrSpiSearchResultsPtr++ ) {
                    
                            /* Process the current index only */
                            if ( pssrSrchShortResultsPtr->psiSrchIndexPtr == psiSrchIndex ) {

                                unsigned int    uiDocumentKeyLength = 0;
                                unsigned char   *pucDocumentKey = NULL;
    
                                /* Calculate the new document key length */
                                uiDocumentKeyLength = s_strlen(pssrSpiSearchResultsPtr->pucIndexName) + s_strlen(pssrSpiSearchResultsPtr->pucDocumentKey) + 2;
    
                                /* Allocate space for the new document key */
                                if ( (pucDocumentKey = (unsigned char *)s_malloc((size_t)(sizeof(unsigned char) * uiDocumentKeyLength))) == NULL ) {
                                    iError = SPI_MemError;
                                    goto bailFromiSrchSearchIndex;
                                }
                                
                                /* Create the new document key */
                                snprintf(pucDocumentKey, uiDocumentKeyLength, "%s/%s", pssrSpiSearchResultsPtr->pucIndexName, pssrSpiSearchResultsPtr->pucDocumentKey);
                                
                                /* Free the old document key and swap in the new document key */
                                s_free(pssrSpiSearchResultsPtr->pucDocumentKey);
                                pssrSpiSearchResultsPtr->pucDocumentKey = pucDocumentKey;
                                
                                /* Free the old index name swap in the name of the virtual index */
                                s_free(pssrSpiSearchResultsPtr->pucIndexName);
                                if ( (pssrSpiSearchResultsPtr->pucIndexName = (unsigned char *)s_strdup(pssiSrchSearchIndex->pucIndexName)) == NULL ) {
                                    iError = SPI_MemError;
                                    goto bailFromiSrchSearchIndex;
                                }
                            }
                        }
                    }
                    /* If the index is a physical one, we leave the document key alone */
                    else if ( pssiSrchSearchIndex->uiIndexType == SRCH_SEARCH_INDEX_TYPE_PHYSICAL ) {
                        ;
                    }
                    /* Invalid index */
                    else {
                        iError = SPI_InvalidIndex;
                        goto bailFromiSrchSearchIndex;
                    }
                }
            }
        }




        /* Free the short results - note that we dont want to free the sort key here as it was handed over to the search results */
        iSrchShortResultFree(pssrMainSrchShortResults, uiMainSrchShortResultsLength, SPI_SORT_TYPE_UNKNOWN);
        pssrMainSrchShortResults = NULL;



        /* Wrap up the search report */

        
        /* Tell the user that they suppressed the results */
        if ( uiSrchParserSearchResultsID == SRCH_PARSER_MODIFIER_SEARCH_RESULTS_SUPPRESS_ID ) {
            iSrchReportAppend(pssSrchSearch->pvSrchReport, "%s No documents were returned as results were suppressed\n", REP_SEARCH_SETTING);
        }

        /* Tell the user how we sorted the results */
        if ( (uiSortFieldType == SRCH_SEARCH_SORT_FIELD_TYPE_RELEVANCE) && (uiSrchParserSortOrderID == SRCH_PARSER_SORT_ORDER_ASC_ID) ) {
            iSrchReportAppend(pssSrchSearch->pvSrchReport, "%s The documents were sorted in ascending relevance order\n", REP_SEARCH_SETTING);
        }
        else if ( (uiSortFieldType == SRCH_SEARCH_SORT_FIELD_TYPE_RELEVANCE) && (uiSrchParserSortOrderID == SRCH_PARSER_SORT_ORDER_DESC_ID) ) {
            if ( bValidSortField == true ) {
                iSrchReportAppend(pssSrchSearch->pvSrchReport, "%s The documents were sorted in descending relevance order\n", REP_SEARCH_SETTING);
            }
            else {
                iSrchReportAppend(pssSrchSearch->pvSrchReport, "%s The documents were sorted in descending relevance order because %s the requested sort field\n", 
                        REP_SEARCH_SETTING, (uiIndexCount > 1) ? "not all the index contained" : "the index did not contain");
            }
        }
        else if ( (uiSortFieldType == SRCH_SEARCH_SORT_FIELD_TYPE_RANK) && (uiSrchParserSortOrderID == SRCH_PARSER_SORT_ORDER_ASC_ID) ) {
            iSrchReportAppend(pssSrchSearch->pvSrchReport, "%s The documents were sorted in ascending rank order\n", REP_SEARCH_SETTING);
        }
        else if ( (uiSortFieldType == SRCH_SEARCH_SORT_FIELD_TYPE_RANK) && (uiSrchParserSortOrderID == SRCH_PARSER_SORT_ORDER_DESC_ID) ) {
            iSrchReportAppend(pssSrchSearch->pvSrchReport, "%s The documents were sorted in descending rank order\n", REP_SEARCH_SETTING);
        }
        else if ( (uiSortFieldType == SRCH_SEARCH_SORT_FIELD_TYPE_DATE) && (uiSrchParserSortOrderID == SRCH_PARSER_SORT_ORDER_ASC_ID) ) {
            iSrchReportAppend(pssSrchSearch->pvSrchReport, "%s The documents were sorted in ascending date order\n", REP_SEARCH_SETTING);
        }
        else if ( (uiSortFieldType == SRCH_SEARCH_SORT_FIELD_TYPE_DATE) && (uiSrchParserSortOrderID == SRCH_PARSER_SORT_ORDER_DESC_ID) ) {
            iSrchReportAppend(pssSrchSearch->pvSrchReport, "%s The documents were sorted in descending date order\n", REP_SEARCH_SETTING);
        }
        else if ( ((uiSortFieldType == SRCH_SEARCH_SORT_FIELD_TYPE_ITEM_DOUBLE) || (uiSortFieldType == SRCH_SEARCH_SORT_FIELD_TYPE_ITEM_FLOAT) || 
                (uiSortFieldType == SRCH_SEARCH_SORT_FIELD_TYPE_ITEM_UINT) || (uiSortFieldType == SRCH_SEARCH_SORT_FIELD_TYPE_ITEM_ULONG) || 
                (uiSortFieldType == SRCH_SEARCH_SORT_FIELD_TYPE_ITEM_UCHAR)) && (uiSrchParserSortOrderID == SRCH_PARSER_SORT_ORDER_ASC_ID) ) {
            iSrchReportAppend(pssSrchSearch->pvSrchReport, "%s The documents were sorted in ascending %ls order\n", REP_SEARCH_SETTING, pwcSrchParserSortFieldName);
        }
        else if ( ((uiSortFieldType == SRCH_SEARCH_SORT_FIELD_TYPE_ITEM_DOUBLE) || (uiSortFieldType == SRCH_SEARCH_SORT_FIELD_TYPE_ITEM_FLOAT) || 
                (uiSortFieldType == SRCH_SEARCH_SORT_FIELD_TYPE_ITEM_UINT) || (uiSortFieldType == SRCH_SEARCH_SORT_FIELD_TYPE_ITEM_ULONG) || 
                (uiSortFieldType == SRCH_SEARCH_SORT_FIELD_TYPE_ITEM_UCHAR)) && (uiSrchParserSortOrderID == SRCH_PARSER_SORT_ORDER_DESC_ID) ) {
            iSrchReportAppend(pssSrchSearch->pvSrchReport, "%s The documents were sorted in descending %ls order\n", REP_SEARCH_SETTING, pwcSrchParserSortFieldName);
        }
        else if ( (uiSortFieldType == SRCH_SEARCH_SORT_FIELD_TYPE_NO_SORT) && (uiSrchParserSortOrderID == SRCH_PARSER_SORT_ORDER_NONE_ID) ) {
            iSrchReportAppend(pssSrchSearch->pvSrchReport, "%s The documents were not sorted\n", REP_SEARCH_SETTING);
        }
    
        /* Tell the user which default operator was applied */
        if ( uiSrchParserBooleanOperatorID == SRCH_PARSER_MODIFIER_BOOLEAN_OPERATOR_OR_ID ) {
            iSrchReportAppend(pssSrchSearch->pvSrchReport, "%s The default boolean operator was set to 'or'\n", REP_SEARCH_SETTING);
        }
        else if ( uiSrchParserBooleanOperatorID == SRCH_PARSER_MODIFIER_BOOLEAN_OPERATOR_IOR_ID ) {
            iSrchReportAppend(pssSrchSearch->pvSrchReport, "%s The default boolean operator was set to 'ior'\n", REP_SEARCH_SETTING);
        }
        else if ( uiSrchParserBooleanOperatorID == SRCH_PARSER_MODIFIER_BOOLEAN_OPERATOR_XOR_ID ) {
            iSrchReportAppend(pssSrchSearch->pvSrchReport, "%s The default boolean operator was set to 'xor'\n", REP_SEARCH_SETTING);
        }
        else if ( uiSrchParserBooleanOperatorID == SRCH_PARSER_MODIFIER_BOOLEAN_OPERATOR_AND_ID ) {
            iSrchReportAppend(pssSrchSearch->pvSrchReport, "%s The default boolean operator was set to 'and'\n", REP_SEARCH_SETTING);
        }
        else if ( uiSrchParserBooleanOperatorID == SRCH_PARSER_MODIFIER_BOOLEAN_OPERATOR_ADJ_ID ) {
            iSrchReportAppend(pssSrchSearch->pvSrchReport, "%s The default boolean operator was set to 'adj'\n", REP_SEARCH_SETTING);
        }
        else if ( uiSrchParserBooleanOperatorID == SRCH_PARSER_MODIFIER_BOOLEAN_OPERATOR_NEAR_ID ) {
            iSrchReportAppend(pssSrchSearch->pvSrchReport, "%s The default boolean operator was set to 'near'\n", REP_SEARCH_SETTING);
        }
        
        /* Tell the user which boolean option they chose */
        if ( uiSrchParserBooleanOperationID == SRCH_PARSER_MODIFIER_BOOLEAN_OPERATION_RELAXED_ID ) {
            iSrchReportAppend(pssSrchSearch->pvSrchReport, "%s The search was evaluated with relaxed boolean operations\n", REP_SEARCH_SETTING);
        }
        else if ( uiSrchParserBooleanOperationID == SRCH_PARSER_MODIFIER_BOOLEAN_OPERATION_STRICT_ID ) {
            iSrchReportAppend(pssSrchSearch->pvSrchReport, "%s The search was evaluated with strict boolean operations\n", REP_SEARCH_SETTING);
        }

        /* Tell the user which search type they chose */
        if ( uiSrchParserSearchTypeID == SRCH_PARSER_MODIFIER_SEARCH_TYPE_BOOLEAN_ID ) {
            iSrchReportAppend(pssSrchSearch->pvSrchReport, "%s The search was evaluated as a boolean search\n", REP_SEARCH_SETTING);
        }
        else if ( uiSrchParserSearchTypeID == SRCH_PARSER_MODIFIER_SEARCH_TYPE_FREETEXT_ID ) {
            iSrchReportAppend(pssSrchSearch->pvSrchReport, "%s The search was evaluated as a free text search\n", REP_SEARCH_SETTING);
        }

        /* Tell the user which search case they chose */
        if ( uiSrchParserTermCaseID == SRCH_PARSER_MODIFIER_TERM_CASE_DROP_ID ) {
            iSrchReportAppend(pssSrchSearch->pvSrchReport, "%s The search was evaluated in a case-insensitive manner\n", REP_SEARCH_SETTING);
        }
        else if ( uiSrchParserTermCaseID == SRCH_PARSER_MODIFIER_TERM_CASE_KEEP_ID ) {
            iSrchReportAppend(pssSrchSearch->pvSrchReport, "%s The search was evaluated in a case-sensitive manner\n", REP_SEARCH_SETTING);
        }

        /* Tell the user which operator case they chose */
        if ( uiSrchParserOperatorCaseID == SRCH_PARSER_MODIFIER_OPERATOR_CASE_ANY_ID ) {
            iSrchReportAppend(pssSrchSearch->pvSrchReport, "%s Search operators were recognised in a case-insensitive manner\n", REP_SEARCH_SETTING);
        }
        else if ( uiSrchParserOperatorCaseID == SRCH_PARSER_MODIFIER_OPERATOR_CASE_UPPER_ID ) {
            iSrchReportAppend(pssSrchSearch->pvSrchReport, "%s Search operators were recognised only if they were in upper case\n", REP_SEARCH_SETTING);
        }
        else if ( uiSrchParserOperatorCaseID == SRCH_PARSER_MODIFIER_OPERATOR_CASE_LOWER_ID ) {
            iSrchReportAppend(pssSrchSearch->pvSrchReport, "%s Search operators were recognised only if they were in lower case\n", REP_SEARCH_SETTING);
        }
    }




    /* Get the search time and optionally log it to the search report */
    {
        struct timeval      tvSearchEndTimeVal;
        struct timeval      tvSearchDiffTimeVal;

        /* Set the search end time */
        s_gettimeofday(&tvSearchEndTimeVal, NULL);

        /* Get the time taken for this search */
        UTL_DATE_DIFF_TIMEVAL(tvSearchStartTimeVal, tvSearchEndTimeVal, tvSearchDiffTimeVal);

        /* Turn it into milliseconds */
        UTL_DATE_TIMEVAL_TO_MILLISECONDS(tvSearchDiffTimeVal, dSearchTime);

        /* Search report */
        if ( uiSrchParserDebugID == SRCH_PARSER_MODIFIER_DEBUG_ENABLE_ID ) {
            iSrchReportAppend(pssSrchSearch->pvSrchReport, "%s The search took %.1f milliseconds\n", REP_SEARCH_DEBUG, dSearchTime);
        }
    }

    /* Close the search report */
    iSrchReportCloseReport(pssSrchSearch->pvSrchReport);


    /* Append search report document it is available */
    if ( uiSrchParserSearchReportID == SRCH_PARSER_MODIFIER_SEARCH_REPORT_RETURN_ID ) {

        struct srchSearchIndex      *pssiSrchSearchIndex =  (struct srchSearchIndex *)ppvIndexList[0];
        unsigned char               pucIndexName[SPI_INDEX_NAME_MAXIMUM_LENGTH + 1] = {'\0'};
        unsigned char               pucDocumentKey[SPI_DOCUMENT_KEY_MAXIMUM_LENGTH + 1] = {'\0'};
        unsigned char               *pucSearchReportText = NULL;


        /* Check the search results */
        ASSERT(((pssrSpiSearchResults != NULL) && (uiSpiSearchResultsLength > 0)) || ((pssrSpiSearchResults == NULL) && (uiSpiSearchResultsLength == 0)));
        
        /* Check the search report key */
        ASSERT(bUtlStringsIsStringNULL(pucSearchReportKey) == false);

        /* Allocate search results if needed for the search report */
        if ( (pssrSpiSearchResults == NULL) && (uiSpiSearchResultsLength == 0) ) {
            if ( (pssrSpiSearchResults = (struct spiSearchResult *)s_malloc(sizeof(struct spiSearchResult) * (uiSpiSearchResultsLength + 1))) == NULL ) {
                iError = SPI_MemError;
                goto bailFromiSrchSearchIndex;
            }
        }
        
        /* Dereference the search result */
        pssrSpiSearchResultsPtr = (pssrSpiSearchResults + uiSpiSearchResultsLength);

        /* Increment the number of search results */
        uiSpiSearchResultsLength++;


        /* Set the index name */
        if ( (pssrSpiSearchResultsPtr->pucIndexName = (unsigned char *)s_strdup(pssiSrchSearchIndex->pucIndexName)) == NULL ) {
            iError = SPI_MemError;
            goto bailFromiSrchSearchIndex;
        }

        /* The search report needs a special document key */
        if ( pssiSrchSearchIndex->uiIndexType == SRCH_SEARCH_INDEX_TYPE_PHYSICAL ) {
            snprintf(pucDocumentKey, SPI_DOCUMENT_KEY_MAXIMUM_LENGTH + 1, "%d %s", SRCH_SEARCH_REPORT_DOCUMENT_ID, pucSearchReportKey);
        }
        else if ( pssiSrchSearchIndex->uiIndexType == SRCH_SEARCH_INDEX_TYPE_VIRTUAL ) {
            
            /* Get the index name for this index */
            if ( (iError = iSrchSearchGetIndexName(pssSrchSearch, pssiSrchSearchIndex->ppsiSrchIndexList[0], pucIndexName, SPI_INDEX_NAME_MAXIMUM_LENGTH + 1)) != SRCH_NoError ) {
                goto bailFromiSrchSearchIndex;
            }
        
            snprintf(pucDocumentKey, SPI_DOCUMENT_KEY_MAXIMUM_LENGTH + 1, "%s/%d %s", pucIndexName, SRCH_SEARCH_REPORT_DOCUMENT_ID, pucSearchReportKey);
        }

        /* Set the document key */
        if ( (pssrSpiSearchResultsPtr->pucDocumentKey = (unsigned char *)s_strdup(pucDocumentKey)) == NULL ) {
            iError = SPI_MemError;
            goto bailFromiSrchSearchIndex;
        }

        /* Set the title */
        if ( (pssrSpiSearchResultsPtr->pucTitle = (unsigned char *)s_strdup(SPI_SEARCH_REPORT_TITLE)) == NULL ) {
            iError = SPI_MemError;
            goto bailFromiSrchSearchIndex;
        }
        
        /* Set the sort key */    
        switch ( uiSortType ) {

            case SPI_SORT_TYPE_DOUBLE_ASC:
            case SPI_SORT_TYPE_DOUBLE_DESC:
                pssrSpiSearchResultsPtr->dSortKey = SPI_SORT_KEY_DOUBLE_MINIMUM;
                break;

            case SPI_SORT_TYPE_FLOAT_ASC:
            case SPI_SORT_TYPE_FLOAT_DESC:
                pssrSpiSearchResultsPtr->fSortKey = SPI_SORT_KEY_FLOAT_MINIMUM;
                break;
            
            case SPI_SORT_TYPE_UINT_ASC:
            case SPI_SORT_TYPE_UINT_DESC:
                pssrSpiSearchResultsPtr->uiSortKey = SPI_SORT_KEY_UINT_MINIMUM;
                break;
            
            case SPI_SORT_TYPE_ULONG_ASC:
            case SPI_SORT_TYPE_ULONG_DESC:
                pssrSpiSearchResultsPtr->ulSortKey = SPI_SORT_KEY_ULONG_MINIMUM;
                break;
            
            case SPI_SORT_TYPE_UCHAR_ASC:
            case SPI_SORT_TYPE_UCHAR_DESC:
                pssrSpiSearchResultsPtr->pucSortKey = NULL;
                break;
            
            case SPI_SORT_TYPE_NO_SORT:
                break;

            case SPI_SORT_TYPE_UNKNOWN:
                break;
            
            default:
                ASSERT(false);
                break;
        }

        /* Set the language code */
        if ( (pssrSpiSearchResultsPtr->pucLanguageCode = (unsigned char *)s_strdup(SPI_SEARCH_REPORT_LANGUAGE_NAME)) == NULL ) {
            iError = SPI_MemError;
            goto bailFromiSrchSearchIndex;
        }
        
        /* Set the rank */
        pssrSpiSearchResultsPtr->uiRank = 0;

        /* Set the term count */
        pssrSpiSearchResultsPtr->uiTermCount = 0;

        /* Set the ansi date */
        pssrSpiSearchResultsPtr->ulAnsiDate = 0;

        /* Get the search report text, null out the pointer if we failed to get the search report */
        if ( iSrchReportGetReportText(pssSrchSearch->pvSrchReport, pucSearchReportKey, &pucSearchReportText) != SRCH_NoError ) {
            pucSearchReportText = NULL;
        }

        /* Set the document item information */
        if ( (iError = iSrchSearchAddEntryToSpiDocumentItems(SPI_SEARCH_REPORT_ITEM_NAME, SPI_SEARCH_REPORT_MIME_TYPE, NULL, 0, 
                pucSearchReportText, (pucSearchReportText != NULL) ? s_strlen(pucSearchReportText) : 0, false, 
                &pssrSpiSearchResultsPtr->psdiSpiDocumentItems, &pssrSpiSearchResultsPtr->uiDocumentItemsLength)) != SRCH_NoError ) {
            s_free(pucSearchReportText);
            goto bailFromiSrchSearchIndex;
        }
    }



    /* Bail label */
    bailFromiSrchSearchIndex:


    {

        unsigned char               pucIndexNames[SPI_INDEX_NAME_MAXIMUM_LENGTH * 3] = {'\0'};
        struct spiSearchResponse    *pssrSpiSearchResponse = NULL;

        /* Create a nice, comma delimited, index name from the index name list for the log */
        for ( uiI = 0; ppvIndexList[uiI] != NULL; uiI++ ) {
            
            /* Dereference the index info structure for convenience */
            struct srchSearchIndex    *pssiSrchSearchIndex = (struct srchSearchIndex *)ppvIndexList[uiI];
        
            /* Create a nice, comma delimited, index name from the index name list */
            if ( bUtlStringsIsStringNULL(pucIndexNames) == false ) {
                s_strnncat(pucIndexNames, ", ", (SPI_INDEX_NAME_MAXIMUM_LENGTH * 3) - 1, SPI_INDEX_NAME_MAXIMUM_LENGTH * 3);
            }
            s_strnncat(pucIndexNames, pssiSrchSearchIndex->pucIndexName, (SPI_INDEX_NAME_MAXIMUM_LENGTH * 3) - 1, SPI_INDEX_NAME_MAXIMUM_LENGTH * 3);
        }


        /* Allocate the search response, screwy way to do it because we can't bail at this point but we can let the error flow */
        if ( iError == SPI_NoError ) {
            if ( (pssrSpiSearchResponse = (struct spiSearchResponse *)s_malloc((size_t)(sizeof(struct spiSearchResponse)))) == NULL ) {
                iError = SPI_MemError;
            }
        }


        /* Handle the error */
        if ( iError == SPI_NoError ) {
            
            unsigned int    uiSearchReportCount = 0;
            
            /* Count up the search reports */
            for ( pssrSpiSearchResultsPtr = pssrSpiSearchResults; pssrSpiSearchResultsPtr < (pssrSpiSearchResults + uiSpiSearchResultsLength); pssrSpiSearchResultsPtr++ ) {

                /* Increment the search report counter when we encounter one */
                if ( (pssrSpiSearchResultsPtr->psdiSpiDocumentItems != NULL) && 
                        (s_strcmp(pssrSpiSearchResultsPtr->psdiSpiDocumentItems->pucItemName, SPI_SEARCH_REPORT_ITEM_NAME) == 0) &&
                        (s_strcmp(pssrSpiSearchResultsPtr->psdiSpiDocumentItems->pucMimeType, SPI_SEARCH_REPORT_MIME_TYPE) == 0) ) {

                    uiSearchReportCount++;
                }
            }
            
            /* Log the search result */
            if ( uiSearchReportCount > 0 ) {
                iUtlLogInfo(UTL_LOG_CONTEXT, "Search, %s: '%s', search: '%ls', found: %u document%s and returned: %u (+%u search report%s), in: %.1f milliseconds.", 
                        (s_strchr(pucIndexNames, ',') != NULL) ? "indices" : "index", pucIndexNames, pwcUtlStringsGetSafeWideString(pwcSearchText),
                        uiTotalResultsMain, (uiTotalResultsMain == 1) ? "" : "s", uiSpiSearchResultsLength - uiSearchReportCount, 
                        uiSearchReportCount, (uiSearchReportCount == 1) ? "" : "s", dSearchTime);
            }
            else {
                iUtlLogInfo(UTL_LOG_CONTEXT, "Search, %s: '%s', search: '%ls', found: %u document%s and returned: %u, in: %.1f milliseconds.", 
                        (s_strchr(pucIndexNames, ',') != NULL) ? "indices" : "index", pucIndexNames, pwcUtlStringsGetSafeWideString(pwcSearchText),
                        uiTotalResultsMain, (uiTotalResultsMain == 1) ? "" : "s", uiSpiSearchResultsLength, dSearchTime);
            }

/*             iSpiPrintSpiSearchResults(pssrSpiSearchResults, uiSpiSearchResultsLength, uiSortType); */

            /* Check the search results */
            ASSERT(((pssrSpiSearchResults != NULL) && (uiSpiSearchResultsLength > 0)) || ((pssrSpiSearchResults == NULL) && (uiSpiSearchResultsLength == 0)));
            
            /* Set the search response values */
            pssrSpiSearchResponse->pssrSpiSearchResults = pssrSpiSearchResults;
            pssrSpiSearchResponse->uiSpiSearchResultsLength = uiSpiSearchResultsLength;
            pssrSpiSearchResponse->uiTotalResults = uiTotalResultsMain;
            pssrSpiSearchResponse->uiStartIndex = (uiSpiSearchResultsLength > 0) ? uiStartIndex : 0;
            pssrSpiSearchResponse->uiEndIndex = ((uiSpiSearchResultsLength - uiSearchReportCount) > 0) ? ((uiStartIndex + (uiSpiSearchResultsLength - uiSearchReportCount)) - 1) : 0;
            pssrSpiSearchResponse->uiSortType = uiSortType;
            pssrSpiSearchResponse->dMaxSortKey = dMaxSortKeyMain;
            pssrSpiSearchResponse->dSearchTime = dSearchTime;

            /* Set the return pointer */
            *ppssrSpiSearchResponse = pssrSpiSearchResponse;
        }
        else {

            /* Adjust the error */
            iError = SPI_ERROR_VALID(iError) ? iError : SPI_SearchIndexFailed;

            /* Log the search result */
            iUtlLogInfo(UTL_LOG_CONTEXT, "Search, %s: '%s', search: '%ls', error: %d.", (s_strchr(pucIndexNames, ',') != NULL) ? "index" : "index", 
                    pucIndexNames, pwcUtlStringsGetSafeWideString(pwcSearchText), iError);

            /* Free the search results */
            iSpiFreeSearchResults(pssrSpiSearchResults, uiSpiSearchResultsLength, uiSortFieldType);
            pssrSpiSearchResults = NULL;

            /* Free the search short results */
            iSrchShortResultFree(pssrSrchShortResults, uiSrchShortResultsLength, uiSortType);
            pssrSrchShortResults = NULL;

            /* Free the main search short results */
            iSrchShortResultFree(pssrMainSrchShortResults, uiMainSrchShortResultsLength, uiSortType);
            pssrMainSrchShortResults = NULL;
        }


        /* Free the search and feedback pointers */
        s_free(pwcSearchText);
        s_free(pwcPositiveFeedbackText);
        s_free(pwcNegativeFeedbackText);
    }


    return (iError);

}


/*---------------------------------------------------------------------------*/


/* 
** =================================
** ===  Retrieval SPI Functions  ===
** =================================
*/


/*

    Function:   iSpiRetrieveDocument()

    Purpose:    This function should return the text/data specified by the document
                ID from chunk = 'uiChunkStart' to chunk <= 'uiChunkEnd'.

                The chunk start and end are meaningless if a document chunk type is
                requested.

    Parameters: pssSpiSession       spi session structure
                pvIndex             index structure
                pucDocumentKey      document key
                pucItemName         document item name
                pucMimeType         document mime type
                uiChunkType         chunk type
                uiChunkStart        start of chunk
                uiChunkEnd          end of chunk
                ppvData             return pointer of data returned
                puiDataLength       return pointer for length of data returned

    Globals:    none

    Returns:    SPI Error Code

*/
int iSpiRetrieveDocument
(
    struct spiSession *pssSpiSession,
    void *pvIndex,
    unsigned char *pucDocumentKey,
    unsigned char *pucItemName,
    unsigned char *pucMimeType,
    unsigned int uiChunkType,
    unsigned int uiChunkStart,
    unsigned int uiChunkEnd,
    void **ppvData,
    unsigned int *puiDataLength
)
{

    int                         iError = SPI_NoError;
    struct srchSearch           *pssSrchSearch = NULL;
    struct srchSearchIndex      *pssiSrchSearchIndex = NULL;
        

/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iSpiRetrieveDocument."); */


    /* Check the parameters */
    if ( pssSpiSession == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pssSpiSession' parameter passed to 'iSpiRetrieveDocument'."); 
        return (SPI_InvalidSession);
    }

    if ( pvIndex == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvIndex' parameter passed to 'iSpiRetrieveDocument'."); 
        return (SPI_InvalidIndex);
    }

    if ( bUtlStringsIsStringNULL(pucDocumentKey) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "NULL or empty 'pucDocumentKey' parameter passed to 'iSpiRetrieveDocument'."); 
        return (SPI_InvalidDocumentKey);
    }

    if ( bUtlStringsIsStringNULL(pucItemName) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "NULL or empty 'pucItemName' parameter passed to 'iSpiRetrieveDocument'."); 
        return (SPI_InvalidItemName);
    }

    if ( bUtlStringsIsStringNULL(pucMimeType) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "NULL or empty 'pucMimeType' parameter passed to 'iSpiRetrieveDocument'."); 
        return (SPI_InvalidMimeType);
    }

    if ( SPI_CHUNK_TYPE_VALID(uiChunkType) == false ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiChunkType' parameter passed to 'iSpiRetrieveDocument'."); 
        return (SPI_InvalidChunkType);
    }

    if ( uiChunkStart < 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiChunkStart' parameter passed to 'iSpiRetrieveDocument'."); 
        return (SPI_InvalidChunkRange);
    }

    if ( uiChunkEnd < 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiChunkStart' parameter passed to 'iSpiRetrieveDocument'."); 
        return (SPI_InvalidChunkRange);
    }

    if ( uiChunkStart > uiChunkEnd ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiChunkStart' and 'uiChunkEnd' parameters passed to 'iSpiRetrieveDocument'."); 
        return (SPI_InvalidChunkRange);
    }

    if ( ppvData == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppvData' parameter passed to 'iSpiRetrieveDocument'."); 
        return (SPI_ReturnParameterError);
    }

    if ( puiDataLength == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'puiDataLength' parameter passed to 'iSpiRetrieveDocument'."); 
        return (SPI_ReturnParameterError);
    }


    /* Check the search configuration */
    if ( (iError = iSrchSearchCheckSearchConfiguration(pssSpiSession)) != SPI_NoError ) {
        return (SPI_RetrieveDocumentFailed);
    }

    /* Dereference the search structure from the client pointer and check it */
    if ( (pssSrchSearch = (struct srchSearch *)pssSpiSession->pvClientPtr) == NULL ) {
        return (SPI_InvalidSession);
    }

    /* Dereference the index structure and check it */
    if ( (pssiSrchSearchIndex = (struct srchSearchIndex *)pvIndex) == NULL ) {
        return (SPI_InvalidIndex);
    }


    /* Get the document from a physical index */
    if ( pssiSrchSearchIndex->uiIndexType == SRCH_SEARCH_INDEX_TYPE_PHYSICAL ) {
        
        /* Retrieve the document */
        iError = iSrchRetrievalRetrieveDocument(pssSrchSearch, pssiSrchSearchIndex->ppsiSrchIndexList[0], pucDocumentKey, 
                pucItemName, pucMimeType, uiChunkType, uiChunkStart, uiChunkEnd, ppvData, puiDataLength);
    }
    /* Get the document from a virtual index */
    else if ( pssiSrchSearchIndex->uiIndexType == SRCH_SEARCH_INDEX_TYPE_VIRTUAL ) {
        
        unsigned char   pucScanfFormat[UTL_STRING_SCANF_FORMAT_LENGTH + 1] = {'\0'};
        unsigned char   pucLocalIndexName[SPI_INDEX_NAME_MAXIMUM_LENGTH + 1] = {'\0'};
        unsigned char   pucLocalDocumentKey[SPI_DOCUMENT_KEY_MAXIMUM_LENGTH + 1] = {'\0'};
        unsigned char   pucIndexName[SPI_INDEX_NAME_MAXIMUM_LENGTH + 1] = {'\0'};
        unsigned int    uiI = 0;

        /* Parse out the document key, it will be in the form 'IndexName/DocumentKey' */
        snprintf(pucScanfFormat, UTL_STRING_SCANF_FORMAT_LENGTH + 1, "%%%d[^/]/%%%ds", SPI_INDEX_NAME_MAXIMUM_LENGTH, SPI_DOCUMENT_KEY_MAXIMUM_LENGTH);
        if ( sscanf(pucDocumentKey, pucScanfFormat, pucLocalIndexName, pucLocalDocumentKey) != 2 ) {
            iError = SPI_InvalidDocumentKey;
            goto bailFromiSrchRetrieveDocument;
        }

        /* Loop over the index array looking for the index for this document */
        for ( uiI = 0; uiI < pssiSrchSearchIndex->uiSrchIndexListLength; uiI++ ) {

            struct srchIndex    *psiSrchIndex = pssiSrchSearchIndex->ppsiSrchIndexList[uiI];

            /* Get the index name for this index */
            if ( (iError = iSrchSearchGetIndexName(pssSrchSearch, psiSrchIndex, pucIndexName, SPI_INDEX_NAME_MAXIMUM_LENGTH + 1)) != SRCH_NoError ) {
                goto bailFromiSrchRetrieveDocument;
            }

            /* Retrieve the document if the index names match */
            if ( s_strcmp(pucLocalIndexName, pucIndexName) == 0 ) {
                
                /* Retrieve the document, bail here if we got it with no error, otherwise continue processing */
                if ( (iError = iSrchRetrievalRetrieveDocument(pssSrchSearch, psiSrchIndex, pucLocalDocumentKey, pucItemName, pucMimeType, 
                        uiChunkType, uiChunkStart, uiChunkEnd, ppvData, puiDataLength)) == SRCH_NoError ) {
                    goto bailFromiSrchRetrieveDocument;
                }
            }
        }
        
        /* We fell through the loop which means that we were not able to 
        ** identify the index from which this document came from
        */
        iError = SPI_RetrieveDocumentFailed;
    }
    /* Invalid index type */
    else {
        iError = SPI_InvalidIndex;
    }



    /* Bail label */
    bailFromiSrchRetrieveDocument:


    /* Log the retrieval */
    {
        
        unsigned char   pucErrorMessage[UTL_FILE_PATH_MAX + 1] = {'\0'};

        /* Put together an error message if there was an error */
        if ( iError != SPI_NoError ) {
    
            /* Adjust the error */
            iError = (SPI_ERROR_VALID(iError) ? iError : SPI_RetrieveDocumentFailed);

            snprintf(pucErrorMessage, UTL_FILE_PATH_MAX, ", error: %d", iError);
        }
        
        /* Log according to chunk type */
        switch ( uiChunkType ) {
    
            case SPI_CHUNK_TYPE_BYTE:
                /* Retrieve by bytes */
                iUtlLogDebug(UTL_LOG_CONTEXT, "Retrieving document, index: '%s', document key: '%s', byte range: %u %u, item: '%s', mime type: '%s'%s.", 
                        pssiSrchSearchIndex->pucIndexName, pucDocumentKey, uiChunkStart, uiChunkEnd, pucItemName, pucMimeType, pucErrorMessage);
                break;
    
            case SPI_CHUNK_TYPE_DOCUMENT:
                /* Retrieve by document */
                iUtlLogDebug(UTL_LOG_CONTEXT, "Retrieving document, index: '%s', document key: '%s', item: '%s', mime type: '%s'%s.", 
                        pssiSrchSearchIndex->pucIndexName, pucDocumentKey, pucItemName, pucMimeType, pucErrorMessage);
                break;
    
            default:
                /* Invalid chuck type */
                iUtlLogDebug(UTL_LOG_CONTEXT, "Retrieving document, index: '%s', document key: '%s', invalid chunk type, item: '%s', mime type: '%s'%s.", 
                        pssiSrchSearchIndex->pucIndexName, pucDocumentKey, pucItemName, pucMimeType, pucErrorMessage);
                break;
        }
    }


    return (iError);

}


/*---------------------------------------------------------------------------*/


/* 
** ==========================================
** ===  Server Information SPI Functions  ===
** ==========================================
*/


/*

    Function:   iSpiGetServerInfo()

    Purpose:    This function should allocate, populate a single spi server info
                structure. If an error is returned, the return pointer
                will be ignored.

    Parameters: pssSpiSession           spi session structure
                ppssiSpiServerInfo      return pointer for the spi server info structure

    Globals:    none

    Returns:    SPI Error Code

*/
int iSpiGetServerInfo
(
    struct spiSession *pssSpiSession,
    struct spiServerInfo **ppssiSpiServerInfo
)
{

    int                         iError = SPI_NoError;
    struct srchSearch           *pssSrchSearch = NULL;
    struct spiServerInfo        *pssiSpiServerInfo = NULL;
    unsigned char               pucHostName[MAXHOSTNAMELEN + 1] = {'\0'};
//     struct spiServerIndexInfo   *pssiiSpiServerIndexInfos = NULL;
/*     unsigned int                uiSpiServerIndexInfosLength = 0; */


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iSpiGetServerInfo."); */


    /* Check the parameters */
    if ( pssSpiSession == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pssSpiSession' parameter passed to 'iSpiGetServerInfo'."); 
        return (SPI_InvalidSession);
    }

    if ( ppssiSpiServerInfo == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppssiSpiServerInfo' parameter passed to 'iSpiGetServerInfo'."); 
        return (SPI_ReturnParameterError);
    }


    /* Check the search configuration */
    if ( (iError = iSrchSearchCheckSearchConfiguration(pssSpiSession)) != SPI_NoError ) {
        return (SPI_GetServerInfoFailed);
    }

    /* Dereference the search structure from the client pointer and check it */
    if ( (pssSrchSearch = (struct srchSearch *)pssSpiSession->pvClientPtr) == NULL ) {
        return (SPI_InvalidSession);
    }


    /* Allocate a spiServerInfo structure */
    if ( (pssiSpiServerInfo = (struct spiServerInfo *)s_malloc((size_t)sizeof(struct spiServerInfo))) == NULL ) {
        iError = SPI_MemError;
        goto bailFromiSrchGetServerInfo;
    }


    /* Get the name */
    if ( iUtlNetGetHostName(pucHostName, MAXHOSTNAMELEN + 1) == UTL_NoError ) {
        if ( (pssiSpiServerInfo->pucName = (unsigned char *)s_strdup(pucHostName)) == NULL ) {
            iError = SPI_MemError;
            goto bailFromiSrchGetServerInfo;
        }
    }
    
    /* Set the description */
    pssiSpiServerInfo->pucDescription = NULL;

    /* Get the number of index and set it - removed for now as it is a very expensive operation */
/*    if ( iSpiGetServerIndexInfo(pssSpiSession, &pssiiSpiServerIndexInfos, &uiSpiServerIndexInfosLength) == SPI_NoError ) {
        pssiSpiServerInfo->uiIndexCount = uiSpiServerIndexInfosLength;
    }
    else {
        pssiSpiServerInfo->uiIndexCount = SPI_SERVER_INDEX_COUNT_UNKNOWN;
    }
    iSpiFreeServerIndexInfo(pssiiSpiServerIndexInfos, uiSpiServerIndexInfosLength);
    pssiiSpiServerIndexInfos = NULL;
*/

    /* Set the number of index to an unknown */
    pssiSpiServerInfo->uiIndexCount = SPI_SERVER_INDEX_COUNT_UNKNOWN;


    /* Set the ranking algorithm */
    if ( (pssiSpiServerInfo->pucRankingAlgorithm = (unsigned char *)s_strdup(SRCH_SEARCH_RANKING_ALGORITHM_DEFAULT)) == NULL ) {
        iError = SPI_MemError;
        goto bailFromiSrchGetServerInfo;
    }

    /* Set the min and max weights */
    pssiSpiServerInfo->dWeightMinimum = SPI_SERVER_WEIGHT_MINIMUM;
    pssiSpiServerInfo->dWeightMaximum = SPI_SERVER_WEIGHT_MAXIMUM;



    /* Bail label */
    bailFromiSrchGetServerInfo:


    /* Handle the error */
    if ( iError == SPI_NoError ) {

        /* Set the return pointer */
        *ppssiSpiServerInfo = pssiSpiServerInfo;
    }
    else {

        /* Adjust the error */
        iError = SPI_ERROR_VALID(iError) ? iError : SPI_GetServerInfoFailed;

        /* Free allocations */
        iSpiFreeServerInfo(pssiSpiServerInfo);
        pssiSpiServerInfo = NULL;
    }


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSpiGetServerIndexInfo()

    Purpose:    This function should allocate, populate and return a array of spi 
                server index info structures. The number of entries in the array 
                should be returned in puiSpiServerIndexInfosLength. If an error is returned, 
                the return pointers will be ignored.

    Parameters: pssSpiSession                   spi session structure
                ppssiiSpiServerIndexInfos       return pointer for an array of spi server index info structures
                puiSpiServerIndexInfosLength    return pointer for the number of entries
                                                in the spi server index info structures array

    Globals:    none

    Returns:    SPI Error Code

*/
int iSpiGetServerIndexInfo
(
    struct spiSession *pssSpiSession,
    struct spiServerIndexInfo **ppssiiSpiServerIndexInfos,
    unsigned int *puiSpiServerIndexInfosLength
)
{

    int                         iError = SPI_NoError;
    struct srchSearch           *pssSrchSearch = NULL;
    struct srchIndex            *psiSrchIndex = NULL;
    unsigned char               pucIndexName[SPI_INDEX_NAME_MAXIMUM_LENGTH + 1] = {'\0'};
    unsigned char               pucIndexDescription[SPI_INDEX_DESCRIPTION_MAXIMUM_LENGTH + 1] = {'\0'};
    unsigned char               **ppucDirectoryEntryList = NULL;
    unsigned char               **ppucDirectoryEntryListPtr = NULL;

    unsigned char               pucSubKeys[SRCH_INFO_SYMBOL_MAXIMUM_LENGTH + 1] = {'\0'};
    unsigned char               *pucSubKeysStrtokPtr = NULL;
    unsigned char               *pucPtr = NULL;

    struct spiServerIndexInfo   *pssiiSpiServerIndexInfos = NULL;
    unsigned int                uiSpiServerIndexInfosLength = 0;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iSpiGetServerIndexInfo."); */


    /* Check the parameters */
    if ( pssSpiSession == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pssSpiSession' parameter passed to 'iSpiGetServerIndexInfo'."); 
        return (SPI_InvalidSession);
    }

    if ( ppssiiSpiServerIndexInfos == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppssiiSpiServerIndexInfos' parameter passed to 'iSpiGetServerIndexInfo'."); 
        return (SPI_ReturnParameterError);
    }

    if ( puiSpiServerIndexInfosLength == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'puiSpiServerIndexInfosLength' parameter passed to 'iSpiGetServerIndexInfo'."); 
        return (SPI_ReturnParameterError);
    }


    /* Check the search configuration */
    if ( (iError = iSrchSearchCheckSearchConfiguration(pssSpiSession)) != SPI_NoError ) {
        return (SPI_GetServerIndexInfoFailed);
    }

    /* Dereference the search structure from the client pointer and check it */
    if ( (pssSrchSearch = (struct srchSearch *)pssSpiSession->pvClientPtr) == NULL ) {
        return (SPI_InvalidSession);
    }


    /* Scan the index directory and pick up all the files */
    if ( (iError = iUtlFileScanDirectory(pssSpiSession->pucIndexDirectoryPath, NULL, NULL, &ppucDirectoryEntryList)) != UTL_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to read the contents of the index directory path: '%s', utl error: %d.", pssSpiSession->pucIndexDirectoryPath, iError);
        iError = SPI_GetServerIndexInfoFailed;
        goto bailFromiSrchGetServerIndexInfo;
    }


    /* Loop over all the directories/files we picked up in the scan */
    if ( ppucDirectoryEntryList != NULL ) {

        for ( ppucDirectoryEntryListPtr = ppucDirectoryEntryList; *ppucDirectoryEntryListPtr != NULL; ppucDirectoryEntryListPtr++ ) {
    
            struct spiServerIndexInfo   *pssiiSpiServerIndexInfosPtr = NULL;
    
            /* See if we can open the index, skip it if we cant, ignore any errors */
            if ( (iError = iSrchIndexOpen(pssSpiSession->pucIndexDirectoryPath, pssSpiSession->pucConfigurationDirectoryPath, 
                    *ppucDirectoryEntryListPtr, SRCH_INDEX_INTENT_SEARCH, &psiSrchIndex)) != SRCH_NoError ) {
                iUtlLogWarn(UTL_LOG_CONTEXT, "Failed to open the index, index: '%s', srch error: %d.", *ppucDirectoryEntryListPtr, iError);
                continue;
            }
    
            /* Add a new entry in our server index info pointer */
            if ( (pssiiSpiServerIndexInfosPtr = (struct spiServerIndexInfo *)s_realloc(pssiiSpiServerIndexInfos, (size_t)((uiSpiServerIndexInfosLength + 1) * sizeof(struct spiServerIndexInfo)))) == NULL ) {
                iError = SPI_MemError;
                goto bailFromiSrchGetServerIndexInfo;
            }
    
            /* Hand over the pointer */
            pssiiSpiServerIndexInfos = pssiiSpiServerIndexInfosPtr;
    
            /* Derefence the server index info pointer for convenience */
            pssiiSpiServerIndexInfosPtr = (pssiiSpiServerIndexInfos + uiSpiServerIndexInfosLength);
        
            /* Set the index name */
            if ( (iError = iSrchSearchGetIndexName(pssSrchSearch, psiSrchIndex, pucIndexName, SPI_INDEX_NAME_MAXIMUM_LENGTH + 1)) != SRCH_NoError ) {
                goto bailFromiSrchGetServerIndexInfo;
            }
    
            /* Set the index name */
            if ( (pssiiSpiServerIndexInfosPtr->pucName = (unsigned char *)s_strdup(pucIndexName)) == NULL ) {
                iError = SPI_MemError;
                goto bailFromiSrchGetServerIndexInfo;
            }
            
            /* Get the index description */
            if ( (iError = iSrchInfoGetDescriptionInfo(psiSrchIndex, pucIndexDescription, SPI_INDEX_DESCRIPTION_MAXIMUM_LENGTH + 1)) != SRCH_NoError ) {
                iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the index description, index: '%s', srch error: %d.", psiSrchIndex->pucIndexName, iError);
                goto bailFromiSrchGetServerIndexInfo;
            }
    
            /* Set the index description */
            if ( (pssiiSpiServerIndexInfosPtr->pucDescription = (unsigned char *)s_strdup(pucIndexDescription)) == NULL ) {
                iError = SPI_MemError;
                goto bailFromiSrchGetServerIndexInfo;
            }
            
            /* Finally, we close the index, ignore any errors */
            iSrchIndexClose(psiSrchIndex);
            psiSrchIndex = NULL;
    
            /* Increment the index info length */
            uiSpiServerIndexInfosLength++;
    
/*             iUtlLogDebug(UTL_LOG_CONTEXT, "iSpiGetServerIndexInfo [%u][%s][%s]", uiSpiServerIndexInfosLength - 1,  */
/*                     pssiiSpiServerIndexInfosPtr->pucName, pucUtlStringsGetPrintableString(pssiiSpiServerIndexInfosPtr->pucDescription)); */
        }
    
        /* Free the directory list */
        iUtlFileFreeDirectoryEntryList(ppucDirectoryEntryList);
        ppucDirectoryEntryList = NULL;
    }



    /* Get the list of virtual index */
    if ( iUtlConfigGetSubKeys(pssSrchSearch->pvUtlConfig, SRCH_SEARCH_CONFIG_VIRTUAL_INDEX, pucSubKeys, SRCH_INFO_SYMBOL_MAXIMUM_LENGTH + 1) == UTL_NoError ) {

        /* Loop adding the index names - dont increment uiSpiServerIndexInfosLength because we are adding to the server index info pointer */
        for ( pucPtr = s_strtok_r(pucSubKeys, UTL_CONFIG_SUBKEY_SEPARATOR, (char **)&pucSubKeysStrtokPtr)/* , uiSpiServerIndexInfosLength = 0 */;
                pucPtr != NULL;
                pucPtr = s_strtok_r(NULL, UTL_CONFIG_SUBKEY_SEPARATOR, (char **)&pucSubKeysStrtokPtr), uiSpiServerIndexInfosLength++ ) {

            struct spiServerIndexInfo   *pssiiSpiServerIndexInfosPtr = NULL;

            /* Add a new entry in our server index info pointer */
            if ( (pssiiSpiServerIndexInfosPtr = (struct spiServerIndexInfo *)s_realloc(pssiiSpiServerIndexInfos, (size_t)((uiSpiServerIndexInfosLength + 1) * sizeof(struct spiServerIndexInfo)))) == NULL ) {
                iError = SPI_MemError;
                goto bailFromiSrchGetServerIndexInfo;
            }
    
            /* Hand over the pointer */
            pssiiSpiServerIndexInfos = pssiiSpiServerIndexInfosPtr;
    
            /* Derefence the server index info pointer for convenience */
            pssiiSpiServerIndexInfosPtr = (pssiiSpiServerIndexInfos + uiSpiServerIndexInfosLength);
        
            /* Set the index name */
            if ( (pssiiSpiServerIndexInfosPtr->pucName = (unsigned char *)s_strdup(pucPtr)) == NULL ) {
                iError = SPI_MemError;
                goto bailFromiSrchGetServerIndexInfo;
            }

            /* Set the index description */
            pssiiSpiServerIndexInfosPtr->pucDescription = NULL;
    
/*             iUtlLogDebug(UTL_LOG_CONTEXT, "iSpiGetServerIndexInfo [%u][%s][%s]", uiSpiServerIndexInfosLength - 1,  */
/*                     pssiiSpiServerIndexInfosPtr->puceName, pucUtlStringsGetPrintableString(pssiiSpiServerIndexInfosPtr->pucDescription)); */
        }
    }



    /* Bail label */
    bailFromiSrchGetServerIndexInfo:


    /* Handle the error */
    if ( iError == SPI_NoError ) {

        /* Set the return pointers if we found any index, otherwise override the error */
        if ( pssiiSpiServerIndexInfos != NULL ) {
            *ppssiiSpiServerIndexInfos = pssiiSpiServerIndexInfos;
            *puiSpiServerIndexInfosLength = uiSpiServerIndexInfosLength;
        }
        else {
            iError = SPI_ServerHasNoIndices;
        }
    }
    else {

        /* Adjust the error */
        iError = SPI_ERROR_VALID(iError) ? iError : SPI_GetServerIndexInfoFailed;

            /* Free the search short results */
        iSpiFreeServerIndexInfo(pssiiSpiServerIndexInfos, uiSpiServerIndexInfosLength);
        pssiiSpiServerIndexInfos = NULL;
    }


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSpiGetIndexInfo()

    Purpose:    This function should allocate and return a single spi index info structuren 
                populated with information pertinent to the index contained in the 
                index structure. If an error is     returned, the return pointer will be ignored.

    Parameters: pssSpiSession       spi session structure
                pvIndex             index structure
                ppsiiSpiIndexInfo   return pointer for the spi index info structure

    Globals:    none

    Returns:    SPI Error Code

*/
int iSpiGetIndexInfo
(
    struct spiSession *pssSpiSession,
    void *pvIndex,
    struct spiIndexInfo **ppsiiSpiIndexInfo
)
{

    int                         iError = SPI_NoError;
    struct srchSearch           *pssSrchSearch = NULL;
    struct srchSearchIndex      *pssiSrchSearchIndex = NULL;
    struct srchIndex            *psiSrchIndex = NULL;
    struct spiIndexInfo         *psiiSpiIndexInfo = NULL;
    unsigned char               pucIndexName[SPI_INDEX_NAME_MAXIMUM_LENGTH + 1] = {'\0'};
    unsigned char               pucIndexDescription[SPI_INDEX_DESCRIPTION_MAXIMUM_LENGTH + 1] = {'\0'};
    unsigned char               pucLanguageCode[SPI_INDEX_LANGUAGE_MAXIMUM_LENGTH + 1] = {'\0'};
    unsigned char               pucTokenizerName[SPI_INDEX_TOKENIZER_MAXIMUM_LENGTH + 1] = {'\0'};
    unsigned char               pucStemmerName[SPI_INDEX_STEMMER_MAXIMUM_LENGTH + 1] = {'\0'};
    unsigned char               pucStopListName[SPI_INDEX_STOP_LIST_MAXIMUM_LENGTH + 1] = {'\0'};
    unsigned int                uiI = 0;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iSpiGetIndexInfo."); */


    /* Check the parameters */
    if ( pssSpiSession == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pssSpiSession' parameter passed to 'iSpiGetIndexInfo'."); 
        return (SPI_InvalidSession);
    }

    if ( pvIndex == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvIndex' parameter passed to 'iSpiGetIndexInfo'."); 
        return (SPI_InvalidIndex);
    }

    if ( ppsiiSpiIndexInfo == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppsiiSpiIndexInfo' parameter passed to 'iSpiGetIndexInfo'."); 
        return (SPI_ReturnParameterError);
    }


    /* Check the search configuration */
    if ( (iError = iSrchSearchCheckSearchConfiguration(pssSpiSession)) != SPI_NoError ) {
        return (SPI_GetIndexInfoFailed);
    }

    /* Dereference the search structure from the client pointer and check it */
    if ( (pssSrchSearch = (struct srchSearch *)pssSpiSession->pvClientPtr) == NULL ) {
        return (SPI_InvalidSession);
    }

    /* Dereference the index structure and check it */
    if ( (pssiSrchSearchIndex = (struct srchSearchIndex *)pvIndex) == NULL ) {
        return (SPI_InvalidIndex);
    }

    /* Check that the index array is valid */
    if ( (pssiSrchSearchIndex->ppsiSrchIndexList == NULL) || (pssiSrchSearchIndex->uiSrchIndexListLength == 0) ) {
        return (SPI_InvalidIndex);
    }

    /* Dereference the first index in the array and check it */
    if ( (psiSrchIndex = pssiSrchSearchIndex->ppsiSrchIndexList[0]) == NULL ) {
        return (SPI_InvalidIndex);
    }


    /* Allocate our return pointer */
    if ( (psiiSpiIndexInfo = (struct spiIndexInfo *)s_malloc((size_t)sizeof(struct spiIndexInfo))) == NULL ) {
        iError = SPI_MemError;
        goto bailFromiSrchGetIndexInfo;
    }

    /* Set the index name for a physical index */
    if ( pssiSrchSearchIndex->uiIndexType == SRCH_SEARCH_INDEX_TYPE_PHYSICAL ) {
        
        /* Get the index name */
        if ( (iError = iSrchSearchGetIndexName(pssSrchSearch, psiSrchIndex, pucIndexName, SPI_INDEX_NAME_MAXIMUM_LENGTH + 1)) != SRCH_NoError ) {
                iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the index name, index: '%s', srch error: %d.", psiSrchIndex->pucIndexName, iError);
            goto bailFromiSrchGetIndexInfo;
        }

        /* Set the index name */
        if ( (psiiSpiIndexInfo->pucName = (unsigned char *)s_strdup(pucIndexName)) == NULL ) {
            iError = SPI_MemError;
            goto bailFromiSrchGetIndexInfo;
        }
    }
    /* Set the index name for a virtual index */
    else if ( pssiSrchSearchIndex->uiIndexType == SRCH_SEARCH_INDEX_TYPE_VIRTUAL ) {
        
        /* Set the index name */
        if ( (psiiSpiIndexInfo->pucName = (unsigned char *)s_strdup(pssiSrchSearchIndex->pucIndexName)) == NULL ) {
            iError = SPI_MemError;
            goto bailFromiSrchGetIndexInfo;
        }
    }
    /* Invalid index type */
    else {
        iError = SPI_InvalidIndex;
        goto bailFromiSrchGetIndexInfo;
    }
    
    /* Get the index description */
    if ( (iError = iSrchInfoGetDescriptionInfo(psiSrchIndex, pucIndexDescription, SPI_INDEX_DESCRIPTION_MAXIMUM_LENGTH + 1)) != SRCH_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the index description from the information file, index: '%s', srch error: %d.", 
                psiSrchIndex->pucIndexName, iError);
        goto bailFromiSrchGetIndexInfo;
    }

    /* Set the index description */
    if ( (psiiSpiIndexInfo->pucDescription = (unsigned char *)s_strdup(pucIndexDescription)) == NULL ) {
        iError = SPI_MemError;
        goto bailFromiSrchGetIndexInfo;
    }
    
    /* Get the language code */
    if ( (iError = iSrchLanguageGetLanguageCode(psiSrchIndex, pucLanguageCode, SPI_INDEX_LANGUAGE_MAXIMUM_LENGTH + 1)) != SRCH_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the language code from the information file, index: '%s', srch error: %d.", 
                psiSrchIndex->pucIndexName, iError);
        goto bailFromiSrchGetIndexInfo;
    }

    /* Set the language code */
    if ( (psiiSpiIndexInfo->pucLanguageCode = (unsigned char *)s_strdup(pucLanguageCode)) == NULL ) {
        iError = SPI_MemError;
        goto bailFromiSrchGetIndexInfo;
    }
    
    /* Get the tokenizer name */
    if ( (iError = iSrchLanguageGetTokenizerName(psiSrchIndex, pucTokenizerName, SPI_INDEX_TOKENIZER_MAXIMUM_LENGTH + 1)) != SRCH_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the tokenizer name from the information file, index: '%s', srch error: %d.", 
                psiSrchIndex->pucIndexName, iError);
        goto bailFromiSrchGetIndexInfo;
    }

    /* Set the tokenizer name */
    if ( (psiiSpiIndexInfo->pucTokenizerName = (unsigned char *)s_strdup(pucTokenizerName)) == NULL ) {
        iError = SPI_MemError;
        goto bailFromiSrchGetIndexInfo;
    }
    
    /* Get the stemmer name */
    if ( (iError = iSrchStemmerGetName(psiSrchIndex, pucStemmerName, SPI_INDEX_STEMMER_MAXIMUM_LENGTH + 1)) != SRCH_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the stemmer name from the information file, index: '%s', srch error: %d.", 
                psiSrchIndex->pucIndexName, iError);
        goto bailFromiSrchGetIndexInfo;
    }

    /* Set the stemmer name */
    if ( (psiiSpiIndexInfo->pucStemmerName = (unsigned char *)s_strdup(pucStemmerName)) == NULL ) {
        iError = SPI_MemError;
        goto bailFromiSrchGetIndexInfo;
    }
    
    /* Get the stop list name */
    if ( (iError = iSrchStopListGetName(psiSrchIndex, pucStopListName, SPI_INDEX_STOP_LIST_MAXIMUM_LENGTH + 1)) != SRCH_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the stop list name from the information file, index: '%s', srch error: %d.", 
                psiSrchIndex->pucIndexName, iError);
        goto bailFromiSrchGetIndexInfo;
    }

    /* Set the stop list name */
    if ( (psiiSpiIndexInfo->pucStopListName = (unsigned char *)s_strdup(pucStopListName)) == NULL ) {
        iError = SPI_MemError;
        goto bailFromiSrchGetIndexInfo;
    }

    /* Set the counts for a physical index */
    if ( pssiSrchSearchIndex->uiIndexType == SRCH_SEARCH_INDEX_TYPE_PHYSICAL ) {
        psiiSpiIndexInfo->uiDocumentCount = psiSrchIndex->uiDocumentCount;
        psiiSpiIndexInfo->ulTotalTermCount = psiSrchIndex->ulTotalTermCount;
        psiiSpiIndexInfo->ulUniqueTermCount = psiSrchIndex->ulUniqueTermCount;
        psiiSpiIndexInfo->ulTotalStopTermCount = psiSrchIndex->ulTotalStopTermCount;
        psiiSpiIndexInfo->ulUniqueStopTermCount = psiSrchIndex->ulUniqueStopTermCount;
    }
    /* Set the counts for a virtual index */
    else if ( pssiSrchSearchIndex->uiIndexType == SRCH_SEARCH_INDEX_TYPE_VIRTUAL ) {

        /* Loop over the index in this index array */
        for ( uiI = 0; uiI < pssiSrchSearchIndex->uiSrchIndexListLength; uiI++ ) {
        
            /* Dereference the index from the array */
            struct srchIndex    *psdSrchIndexLocal = pssiSrchSearchIndex->ppsiSrchIndexList[uiI];

            /* Set the counts */
            psiiSpiIndexInfo->uiDocumentCount += psdSrchIndexLocal->uiDocumentCount;
            psiiSpiIndexInfo->ulTotalTermCount += psdSrchIndexLocal->ulTotalTermCount;
            psiiSpiIndexInfo->ulUniqueTermCount = UTL_MACROS_MAX(psiiSpiIndexInfo->ulUniqueTermCount, psdSrchIndexLocal->ulUniqueTermCount);
            psiiSpiIndexInfo->ulTotalStopTermCount += psdSrchIndexLocal->ulTotalStopTermCount;
            psiiSpiIndexInfo->ulUniqueStopTermCount = UTL_MACROS_MAX(psiiSpiIndexInfo->ulUniqueStopTermCount, psdSrchIndexLocal->ulUniqueStopTermCount);
        }
    }
    /* Invalid index type */
    else {
        iError = SPI_InvalidIndex;
        goto bailFromiSrchGetIndexInfo;
    }
    
    /* Set the access control and update frequency */
    psiiSpiIndexInfo->uiAccessControl = SPI_INDEX_ACCESS_UNKNOWN;
    psiiSpiIndexInfo->uiUpdateFrequency = SPI_INDEX_UPDATE_UNKNOWN;


    /* Set the last update ansi date from the last update time of the information file for this index */
    iUtlDateGetAnsiDateFromTime(psiSrchIndex->tLastUpdateTime, &psiiSpiIndexInfo->ulLastUpdateAnsiDate);

    /* Set the case sensitivity, all index in MPS are case sensitive */
    psiiSpiIndexInfo->uiCaseSensitive = SPI_INDEX_CASE_SENSITIVE;



    /* Bail label */
    bailFromiSrchGetIndexInfo:
    

    /* Handle the error */
    if ( iError == SPI_NoError ) {

        /* Set the return pointer */
        *ppsiiSpiIndexInfo = psiiSpiIndexInfo;
    }
    else {

        /* Adjust the error */
        iError = SPI_ERROR_VALID(iError) ? iError : SPI_GetIndexInfoFailed;

        /* Free allocations */
        iSpiFreeIndexInfo(psiiSpiIndexInfo);
        psiiSpiIndexInfo = NULL;
    }


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSpiGetIndexFieldInfo()

    Purpose:    This function should allocate and return an array of spi field info
                structures, populated with information pertinent to the index contained 
                in the index structure. The number of entries in the array should be returned 
                in puiSpiFieldInfosLength. If an error is returned, the return pointer
                will be ignored.

                Note that returning SPI_IndexHasNoSearchFields is not strictly
                an error, but the field list will also be ignored.

    Parameters: pssSpiSession               spi session structure
                pvIndex                     index structure
                ppsfiSpiFieldInfos          return pointer for an array of spi field info structures
                puiSpiFieldInfosLength      return pointer for the number of entries
                                            in the spi field info structures array

    Globals:    none

    Returns:    SPI Error Code

*/
int iSpiGetIndexFieldInfo
(
    struct spiSession *pssSpiSession,
    void *pvIndex,
    struct spiFieldInfo **ppsfiSpiFieldInfos,
    unsigned int *puiSpiFieldInfosLength
)
{

    int                         iError = SPI_NoError;
    struct srchSearch           *pssSrchSearch = NULL;
    struct srchSearchIndex      *pssiSrchSearchIndex = NULL;
    struct srchIndex            *psiSrchIndex = NULL;
    struct spiFieldInfo         *psfiSpiFieldInfos = NULL;
    unsigned int                uiSpiFieldInfosLength = 0;
    unsigned char               pucFieldName[SPI_FIELD_NAME_MAXIMUM_LENGTH + 1] = {'\0'};
    unsigned char               pucFieldDescription[SPI_FIELD_DESCRIPTION_MAXIMUM_LENGTH + 1] = {'\0'};
    unsigned int                uiFieldType = SRCH_INFO_FIELD_TYPE_NONE_ID;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iSpiGetIndexFieldInfo."); */


    /* Check the parameters */
    if ( pssSpiSession == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pssSpiSession' parameter passed to 'iSpiGetIndexFieldInfo'."); 
        return (SPI_InvalidSession);
    }

    if ( pvIndex == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvIndex' parameter passed to 'iSpiGetIndexFieldInfo'."); 
        return (SPI_InvalidIndex);
    }

    if ( ppsfiSpiFieldInfos == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppsfiSpiFieldInfos' parameter passed to 'iSpiGetIndexFieldInfo'."); 
        return (SPI_ReturnParameterError);
    }

    if ( puiSpiFieldInfosLength == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'puiSpiFieldInfosLength' parameter passed to 'iSpiGetIndexFieldInfo'."); 
        return (SPI_ReturnParameterError);
    }


    /* Check the search configuration */
    if ( (iError = iSrchSearchCheckSearchConfiguration(pssSpiSession)) != SPI_NoError ) {
        return (SPI_GetIndexFieldInfoFailed);
    }

    /* Dereference the search structure from the client pointer and check it */
    if ( (pssSrchSearch = (struct srchSearch *)pssSpiSession->pvClientPtr) == NULL ) {
        return (SPI_InvalidSession);
    }

    /* Dereference the index structure and check it */
    if ( (pssiSrchSearchIndex = (struct srchSearchIndex *)pvIndex) == NULL ) {
        return (SPI_InvalidIndex);
    }

    /* Check that the index array is valid */
    if ( (pssiSrchSearchIndex->ppsiSrchIndexList == NULL) || (pssiSrchSearchIndex->uiSrchIndexListLength == 0) ) {
        return (SPI_InvalidIndex);
    }

    /* Dereference the first index in the array and check it */
    if ( (psiSrchIndex = pssiSrchSearchIndex->ppsiSrchIndexList[0]) == NULL ) {
        return (SPI_InvalidIndex);
    }


    /* Loop reading all the field information we can get our hands on */
    while ( true ) {

        struct spiFieldInfo     *psfiSpiFieldInfosPtr = NULL;

        /* Clear the variables */
        pucFieldName[0] = '\0';
        pucFieldDescription[0] = '\0';
        uiFieldType = SRCH_INFO_FIELD_TYPE_NONE_ID;

        /* Get the field information */
        iError = iSrchInfoGetFieldInfo(psiSrchIndex, uiSpiFieldInfosLength + 1, pucFieldName, SPI_FIELD_NAME_MAXIMUM_LENGTH + 1, pucFieldDescription, SPI_FIELD_DESCRIPTION_MAXIMUM_LENGTH + 1, &uiFieldType, NULL);
        
        /* We have run out of field names to read, so we jump out of the loop, this is not an error */
        if ( iError == SRCH_InfoInvalidFieldID ) {
            iError = SPI_NoError;
            break;
        }
        
        /* Still need to check for real errors */
        if ( iError != SRCH_NoError ) {
            goto bailFromiSrchGetIndexFieldInfo;
        }

        /* Valid field name, add this field to the field name list */
        if ( (psfiSpiFieldInfosPtr = (struct spiFieldInfo *)s_realloc(psfiSpiFieldInfos, (size_t)((uiSpiFieldInfosLength + 1) * sizeof(struct spiFieldInfo)))) == NULL ) {
            iError = SPI_MemError;
            goto bailFromiSrchGetIndexFieldInfo;
        }

        /* Hand over the pointer */
        psfiSpiFieldInfos = psfiSpiFieldInfosPtr;

        /* Dereference the field name pointer for convenience */
        psfiSpiFieldInfosPtr = psfiSpiFieldInfos + uiSpiFieldInfosLength;

        
        /* Set the field name */
        if ( (psfiSpiFieldInfosPtr->pucName = (unsigned char *)s_strdup(pucFieldName)) == NULL ) {
            iError = SPI_MemError;
            goto bailFromiSrchGetIndexFieldInfo;
        }

        /* Set the field description */
        if ( (psfiSpiFieldInfosPtr->pucDescription = (unsigned char *)s_strdup(pucFieldDescription)) == NULL ) {
            iError = SPI_MemError;
            goto bailFromiSrchGetIndexFieldInfo;
        }

        /* Set the field type */
        if ( (uiFieldType == SRCH_INFO_FIELD_TYPE_INT_ID) || (uiFieldType == SRCH_INFO_FIELD_TYPE_LONG_ID) || 
                (uiFieldType == SRCH_INFO_FIELD_TYPE_FLOAT_ID) || (uiFieldType == SRCH_INFO_FIELD_TYPE_DOUBLE_ID) ) {
            psfiSpiFieldInfosPtr->uiType = SPI_FIELD_TYPE_NUMERIC;
        }
        else if ( uiFieldType == SRCH_INFO_FIELD_TYPE_CHAR_ID ) {
            psfiSpiFieldInfosPtr->uiType = SPI_FIELD_TYPE_TEXT;
        }
        else {
            psfiSpiFieldInfosPtr->uiType = SPI_FIELD_TYPE_UNKNOWN;
        }

/*         iUtlLogDebug(UTL_LOG_CONTEXT, "iSpiGetIndexFieldInfo [%u][%s][%s][%u]",  */
/*                 uiSpiFieldInfosLength, (psfiSpiFieldInfos + uiSpiFieldInfosLength)->pucName, */
/*                 pucUtlStringsGetPrintableString((psfiSpiFieldInfos + uiSpiFieldInfosLength)->pucDescription), */
/*                 (psfiSpiFieldInfos + uiSpiFieldInfosLength)->ulType); */

        /* Increment the number of field info entries */
        uiSpiFieldInfosLength++;
    }



    /* Bail label */
    bailFromiSrchGetIndexFieldInfo:


    /* Handle the error */
    if ( iError == SPI_NoError ) {

        /* Set the return pointer if we found any fields, otherwise override the error */
        if ( psfiSpiFieldInfos != NULL ) {
            *ppsfiSpiFieldInfos = psfiSpiFieldInfos;
            *puiSpiFieldInfosLength = uiSpiFieldInfosLength;
        }
        else {
            iError = SPI_IndexHasNoSearchFields;
        }
    }
    else {

        /* Adjust the error */
        iError = SPI_ERROR_VALID(iError) ? iError : SPI_GetIndexFieldInfoFailed;

        /* Free allocations */
        iSpiFreeIndexFieldInfo(psfiSpiFieldInfos, uiSpiFieldInfosLength);
        psfiSpiFieldInfos = NULL;
    }


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSpiGetIndexTermInfo()

    Purpose:    This function should allocate and return an array of spi term info
                structures, populated with information pertinent to the index contained 
                in the index structure. The number of entries in the array should be 
                returned in puiSpiTermInfosLength. If an error is returned, the return pointer
                will be ignored.

                The uiTermMatch specifies which term type we want to match
                on, pucTerm specifies a term to match on and pucFieldName 
                specifies the field name for which we get the terms.

                Note that returning SPI_IndexHasNoTerms is not strictly
                an error, but the term list will be ignored.

    Parameters: pssSpiSession           spi session structure
                pvIndex                 index structure
                uiTermMatch             term match type
                uiTermCase              term case to search for
                pucTerm                 term to match on (optional)
                pucFieldName            field name to match on (optional)
                ppstiSpiTermInfos       return pointer for an array of spi term info structures
                puiSpiTermInfosLength   return pointer for the number of entries
                                        in the spi term info structures array

    Globals:    none

    Returns:    SPI Error Code

*/
int iSpiGetIndexTermInfo
(
    struct spiSession *pssSpiSession,
    void *pvIndex,
    unsigned int uiTermMatch,
    unsigned int uiTermCase,
    unsigned char *pucTerm,
    unsigned char *pucFieldName,
    struct spiTermInfo **ppstiSpiTermInfos,
    unsigned int *puiSpiTermInfosLength
)
{

    int                         iError = SPI_NoError;
    struct srchSearch           *pssSrchSearch = NULL;
    struct srchSearchIndex      *pssiSrchSearchIndex = NULL;
    struct srchIndex            *psiSrchIndex = NULL;
    unsigned int                uiFieldID = 0;
    unsigned char               *pucFieldIDBitmap = NULL;
    struct srchTermDictInfo     *pstdiSrchTermDictInfos = NULL;
    struct srchTermDictInfo     *pstdiSrchTermDictInfosPtr = NULL;
    unsigned int                uiSrchTermDictInfosLength = 0;
    struct spiTermInfo          *pstiSpiTermInfos = NULL;
    unsigned int                uiSpiTermInfosLength = 0;
    unsigned int                uiI = 0;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iSpiGetIndexTermInfo."); */


    /* Check the parameters */
    if ( pssSpiSession == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pssSpiSession' parameter passed to 'iSpiGetIndexTermInfo'."); 
        return (SPI_InvalidSession);
    }

    if ( pvIndex == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvIndex' parameter passed to 'iSpiGetIndexTermInfo'."); 
        return (SPI_InvalidIndex);
    }

    if ( SPI_TERM_MATCH_VALID(uiTermMatch) == false ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiTermMatch' parameter passed to 'iSpiGetIndexTermInfo'."); 
        return (SRCH_InvalidTermMatch);
    }

    if ( SPI_TERM_CASE_VALID(uiTermCase) == false ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiTermCase' parameter passed to 'iSpiGetIndexTermInfo'."); 
        return (SRCH_InvalidTermCase);
    }

    if ( ppstiSpiTermInfos == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppstiSpiTermInfos' parameter passed to 'iSpiGetIndexTermInfo'."); 
        return (SPI_ReturnParameterError);
    }

    if ( puiSpiTermInfosLength == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'puiSpiTermInfosLength' parameter passed to 'iSpiGetIndexTermInfo'."); 
        return (SPI_ReturnParameterError);
    }


    /* Check the search configuration */
    if ( (iError = iSrchSearchCheckSearchConfiguration(pssSpiSession)) != SPI_NoError ) {
        return (SPI_GetIndexTermInfoFailed);
    }

    /* Dereference the search structure from the client pointer and check it */
    if ( (pssSrchSearch = (struct srchSearch *)pssSpiSession->pvClientPtr) == NULL ) {
        return (SPI_InvalidSession);
    }

    /* Dereference the index structure and check it */
    if ( (pssiSrchSearchIndex = (struct srchSearchIndex *)pvIndex) == NULL ) {
        return (SPI_InvalidIndex);
    }

    /* Check that the index array is valid */
    if ( (pssiSrchSearchIndex->ppsiSrchIndexList == NULL) || (pssiSrchSearchIndex->uiSrchIndexListLength == 0) ) {
        return (SPI_InvalidIndex);
    }

    /* Dereference the first index in the array and check it */
    if ( (psiSrchIndex = pssiSrchSearchIndex->ppsiSrchIndexList[0]) == NULL ) {
        return (SPI_InvalidIndex);
    }


    /* Look up the field ID and field options for this field name if it was specified,
    ** set default field options if the field name was not specified
    */
    if ( bUtlStringsIsStringNULL(pucFieldName) == false ) {
        if ( (iError = iSrchInfoGetFieldID(psiSrchIndex, pucFieldName, &uiFieldID)) != SRCH_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the field ID, field name: '%s', index: '%s', srch error: %d.", pucFieldName, psiSrchIndex->pucIndexName, iError);
            goto bailFromiSrchGetIndexTermInfo;
        }
    }


    /* Convert the term to lower case if needed */
    if ( pucTerm != NULL ) {
        if ( (uiTermCase == SPI_TERM_CASE_UNKNOWN) || (uiTermCase == SPI_TERM_CASE_INSENSITIVE) ) {
            pucLngCaseConvertStringToLowerCase(pucTerm);
        }
        else if ( uiTermCase == SPI_TERM_CASE_SENSITIVE ) {
            ;
        }
    }


    /* Allocate a field ID bitmap only if there are any fields other than field ID 0 */
    if ( uiFieldID != 0 ) {

        /* Allocate the field ID bitmap - field ID 0 is not a field */
        if ( (pucFieldIDBitmap = (unsigned char *)s_malloc(sizeof(unsigned char) * UTL_BITMAP_GET_BITMAP_BYTE_LENGTH(psiSrchIndex->uiFieldIDMaximum))) == NULL ) {
            iError = SPI_MemError;
            goto bailFromiSrchGetIndexTermInfo;
        }
    
        /* Set the field ID in the bitmap - field ID 0 is not a field */
        UTL_BITMAP_SET_BIT_IN_POINTER(pucFieldIDBitmap, uiFieldID - 1);
    }


    /* Call the appropriate function */
    if ( uiTermMatch == SPI_TERM_MATCH_REGULAR ) {
        iError = iSrchTermDictLookupRegular(psiSrchIndex, pucFieldIDBitmap, (pucFieldIDBitmap != NULL) ? psiSrchIndex->uiFieldIDMaximum : 0, 
                &pstdiSrchTermDictInfos, &uiSrchTermDictInfosLength);
    }
    else if ( uiTermMatch == SPI_TERM_MATCH_STOP ) {
        iError = iSrchTermDictLookupStop(psiSrchIndex, pucFieldIDBitmap, (pucFieldIDBitmap != NULL) ? psiSrchIndex->uiFieldIDMaximum : 0, 
                &pstdiSrchTermDictInfos, &uiSrchTermDictInfosLength);
    }
    else if ( uiTermMatch == SPI_TERM_MATCH_WILDCARD ) {
        iError = iSrchTermDictLookupWildCard(psiSrchIndex, pucTerm, pucFieldIDBitmap, (pucFieldIDBitmap != NULL) ? psiSrchIndex->uiFieldIDMaximum : 0, 
                &pstdiSrchTermDictInfos, &uiSrchTermDictInfosLength);
    }
    else if ( uiTermMatch == SPI_TERM_MATCH_SOUNDEX ) {
        iError = iSrchTermDictLookupSoundex(psiSrchIndex, psiSrchIndex->uiLanguageID, pucTerm, pucFieldIDBitmap, (pucFieldIDBitmap != NULL) ? psiSrchIndex->uiFieldIDMaximum : 0, 
                &pstdiSrchTermDictInfos, &uiSrchTermDictInfosLength);
    }
    else if ( uiTermMatch == SPI_TERM_MATCH_METAPHONE ) {
        iError = iSrchTermDictLookupMetaphone(psiSrchIndex, psiSrchIndex->uiLanguageID, pucTerm, pucFieldIDBitmap, (pucFieldIDBitmap != NULL) ? psiSrchIndex->uiFieldIDMaximum : 0, 
                &pstdiSrchTermDictInfos, &uiSrchTermDictInfosLength);
    }
    else if ( uiTermMatch == SPI_TERM_MATCH_PHONIX ) {
        iError = iSrchTermDictLookupPhonix(psiSrchIndex, psiSrchIndex->uiLanguageID, pucTerm, pucFieldIDBitmap, (pucFieldIDBitmap != NULL) ? psiSrchIndex->uiFieldIDMaximum : 0, 
                &pstdiSrchTermDictInfos, &uiSrchTermDictInfosLength);
    }
    else if ( uiTermMatch == SPI_TERM_MATCH_TYPO ) {
        iError = iSrchTermDictLookupTypo(psiSrchIndex, psiSrchIndex->uiLanguageID, pucTerm, pucFieldIDBitmap, (pucFieldIDBitmap != NULL) ? psiSrchIndex->uiFieldIDMaximum : 0, 
                &pstdiSrchTermDictInfos, &uiSrchTermDictInfosLength);
    }
    else {
        iError = SPI_GetIndexTermInfoFailed;
        goto bailFromiSrchGetIndexTermInfo;
    }


    /* Convert the SRCH error into an SPI error */
    if ( iError == SRCH_NoError ) {
        iError = SPI_NoError;
    }
    else if ( (iError == SRCH_IndexHasNoTerms) || (iError == SRCH_TermDictTermNotFound) || (iError == SRCH_TermDictTermDoesNotOccur) ) {
        iError = SPI_IndexHasNoTerms;
    }
    else if ( pstdiSrchTermDictInfos == NULL ) {
        iError = SPI_IndexHasNoTerms;
    }
    else {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to expand a term, index: '%s', srch error: %d.", psiSrchIndex->pucIndexName, iError);
        iError = SPI_ERROR_VALID(iError) ? iError : SPI_GetIndexTermInfoFailed;
    }


    /* Allocate and set the spi term info structure if there was no error and if terms we found */
    if ( (iError == SPI_NoError) && (pstdiSrchTermDictInfos != NULL) ) {
        
        struct spiTermInfo      *pstiSpiTermInfosPtr = NULL;
        
        /* Allocate space for the spi term info structure */
        if ( (pstiSpiTermInfos = (struct spiTermInfo *)s_malloc((size_t)(sizeof(struct spiTermInfo) * uiSrchTermDictInfosLength))) == NULL ) {
            iError = SPI_MemError;
            goto bailFromiSrchGetIndexTermInfo;
        }
        
        /* Loop over each entry into the term dict info structure and transfer the information to the spi term info structure */
        for ( pstiSpiTermInfosPtr = pstiSpiTermInfos, pstdiSrchTermDictInfosPtr = pstdiSrchTermDictInfos, uiI = 0; uiI < uiSrchTermDictInfosLength; pstiSpiTermInfosPtr++, pstdiSrchTermDictInfosPtr++, uiI++ ) {
            
            pstiSpiTermInfosPtr->pucTerm = pstdiSrchTermDictInfosPtr->pucTerm;
            pstiSpiTermInfosPtr->uiType = pstdiSrchTermDictInfosPtr->uiTermType;
            pstiSpiTermInfosPtr->uiCount = pstdiSrchTermDictInfosPtr->uiTermCount;
            pstiSpiTermInfosPtr->uiDocumentCount = pstdiSrchTermDictInfosPtr->uiDocumentCount;
            
            /* Null out the term pointer so that we don't double free it */
            pstdiSrchTermDictInfosPtr->pucTerm = NULL;
        }
    }



    /* Bail label */
    bailFromiSrchGetIndexTermInfo:
    
    
    /* Free the term dict info structure */
    iSrchTermDictFreeSearchTermDictInfo(pstdiSrchTermDictInfos, uiSrchTermDictInfosLength);

    /* Free the field ID bit map  */
    s_free(pucFieldIDBitmap);


    /* Handle the error */
    if ( iError == SPI_NoError ) {

        /* Set the return pointers */
        *ppstiSpiTermInfos = pstiSpiTermInfos;
        *puiSpiTermInfosLength = uiSrchTermDictInfosLength;
    }
    else {

        /* Adjust the error */
        iError = SPI_ERROR_VALID(iError) ? iError : SPI_GetIndexTermInfoFailed;

        /* Free allocations */
        iSpiFreeTermInfo(pstiSpiTermInfos, uiSpiTermInfosLength);
        pstiSpiTermInfos = NULL;
    }
    
    
    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSpiGetDocumentInfo()

    Purpose:    This function should allocate, populate and return a single spi document info
                structure. If an error is returned, the return pointer
                will be ignored.

    Parameters: pssSpiSession           spi session structure
                pvIndex                 index structure
                pucDocumentKey          document key
                ppsdiSpiDocumentInfo    return pointer for the spi document info structure

    Globals:    none

    Returns:    SPI Error Code

*/
int iSpiGetDocumentInfo
(
    struct spiSession *pssSpiSession,
    void *pvIndex,
    unsigned char *pucDocumentKey,
    struct spiDocumentInfo **ppsdiSpiDocumentInfo
)
{

    int                         iError = SPI_NoError;
    struct srchSearch           *pssSrchSearch = NULL;
    struct srchSearchIndex      *pssiSrchSearchIndex = NULL;
    struct srchIndex            *psiSrchIndex = NULL;
    struct spiDocumentInfo      *psdiSpiDocumentInfo = NULL;
    unsigned int                uiI = 0;
    unsigned int                uiDocumentID = 0;
    unsigned int                uiLanguageID = 0;
    unsigned char               pucIndexName[SPI_INDEX_NAME_MAXIMUM_LENGTH + 1] = {'\0'};
    unsigned char               pucLanguageCode[SPI_INDEX_LANGUAGE_MAXIMUM_LENGTH + 1] = {'\0'};
    unsigned char               pucItemName[SPI_ITEM_NAME_MAXIMUM_LENGTH + 1] = {'\0'};
    unsigned char               pucMimeType[SPI_MIME_TYPE_MAXIMUM_LENGTH + 1] = {'\0'};
    unsigned int                uiSrchDocumentItemsLength = 0;
    struct srchDocumentItem     *psdiSrchDocumentItems = NULL;
    struct srchDocumentItem     *psdiSrchDocumentItemsPtr = NULL;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iSpiGetDocumentInfo."); */


    /* Check the parameters */
    if ( pssSpiSession == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pssSpiSession' parameter passed to 'iSpiGetDocumentInfo'."); 
        return (SPI_InvalidSession);
    }

    if ( pvIndex == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvIndex' parameter passed to 'iSpiGetDocumentInfo'."); 
        return (SPI_InvalidIndex);
    }

    if ( bUtlStringsIsStringNULL(pucDocumentKey) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pucDocumentKey' parameter passed to 'iSpiGetDocumentInfo'."); 
        return (SPI_InvalidDocumentKey);
    }

    if ( ppsdiSpiDocumentInfo == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppsdiSpiDocumentInfo' parameter passed to 'iSpiGetDocumentInfo'."); 
        return (SPI_ReturnParameterError);
    }


    /* Check the search configuration */
    if ( (iError = iSrchSearchCheckSearchConfiguration(pssSpiSession)) != SPI_NoError ) {
        return (SPI_GetDocumentInfoFailed);
    }

    /* Dereference the search structure from the client pointer and check it */
    if ( (pssSrchSearch = (struct srchSearch *)pssSpiSession->pvClientPtr) == NULL ) {
        return (SPI_InvalidSession);
    }

    /* Dereference the index structure and check it */
    if ( (pssiSrchSearchIndex = (struct srchSearchIndex *)pvIndex) == NULL ) {
        return (SPI_InvalidIndex);
    }

    /* Check that the index array is valid */
    if ( (pssiSrchSearchIndex->ppsiSrchIndexList == NULL) || (pssiSrchSearchIndex->uiSrchIndexListLength == 0) ) {
        return (SPI_InvalidIndex);
    }

    /* Dereference the first index in the array and check it */
    if ( (psiSrchIndex = pssiSrchSearchIndex->ppsiSrchIndexList[0]) == NULL ) {
        return (SPI_InvalidIndex);
    }


    /* Get the document ID for this document key from the document key dictionary for a physical index */
    if ( pssiSrchSearchIndex->uiIndexType == SRCH_SEARCH_INDEX_TYPE_PHYSICAL ) {
        
        /* Look up the document ID */
        if ( (iError = iSrchKeyDictLookup(pssiSrchSearchIndex->ppsiSrchIndexList[0], pucDocumentKey, &uiDocumentID)) != SRCH_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Invalid document key: '%s', index: '%s', srch error: %d.", 
                    pucDocumentKey, pssiSrchSearchIndex->ppsiSrchIndexList[0]->pucIndexName, iError);
            goto bailFromiSrchGetDocumentInfo;
        }
    }
    /* Get the document ID for this document key from the document key dictionary for a virtual index */
    else if ( pssiSrchSearchIndex->uiIndexType == SRCH_SEARCH_INDEX_TYPE_VIRTUAL ) {
        
        unsigned char   pucScanfFormat[UTL_STRING_SCANF_FORMAT_LENGTH + 1] = {'\0'};
        unsigned char   pucLocalIndexName[SPI_INDEX_NAME_MAXIMUM_LENGTH + 1] = {'\0'};
        unsigned char   pucLocalDocumentKey[SPI_DOCUMENT_KEY_MAXIMUM_LENGTH + 1] = {'\0'};
        
        /* Parse out the document key, it will be in the form 'IndexName/DocumentKey' */
        snprintf(pucScanfFormat, UTL_STRING_SCANF_FORMAT_LENGTH + 1, "%%%d[^/]/%%%ds", SPI_INDEX_NAME_MAXIMUM_LENGTH, SPI_DOCUMENT_KEY_MAXIMUM_LENGTH);
        if ( sscanf(pucDocumentKey, pucScanfFormat, pucLocalIndexName, pucLocalDocumentKey) != 2 ) {
            iError = SPI_InvalidDocumentKey;
            goto bailFromiSrchGetDocumentInfo;
        }
            
        /* Loop over the index array looking for the index for this document */
        for ( uiI = 0; uiI < pssiSrchSearchIndex->uiSrchIndexListLength; uiI++ ) {

            /* Get the index name for this index */
            if ( (iError = iSrchSearchGetIndexName(pssSrchSearch, pssiSrchSearchIndex->ppsiSrchIndexList[uiI], pucIndexName, SPI_INDEX_NAME_MAXIMUM_LENGTH + 1)) != SRCH_NoError ) {
                goto bailFromiSrchGetDocumentInfo;
            }

            /* Get the document if the index names match */
            if ( s_strcmp(pucLocalIndexName, pucIndexName) == 0 ) {
                if ( (iError = iSrchKeyDictLookup(pssiSrchSearchIndex->ppsiSrchIndexList[uiI], pucDocumentKey, &uiDocumentID)) != SRCH_NoError ) {
                    iUtlLogError(UTL_LOG_CONTEXT, "Invalid document key: '%s', index: '%s', srch error: %d.", 
                            pucDocumentKey, pssiSrchSearchIndex->ppsiSrchIndexList[uiI]->pucIndexName, iError);
                    goto bailFromiSrchGetDocumentInfo;
                }
            }
        }
    }
    /* Invalid index */
    else {
        iError = SPI_InvalidIndex;
        goto bailFromiSrchGetDocumentInfo;
    }


    /* Valid document, so we allocate the document information structure */
    if ( (psdiSpiDocumentInfo = (struct spiDocumentInfo *)s_malloc((size_t)sizeof(struct spiDocumentInfo))) == NULL ) {
        iError = SPI_MemError;
        goto bailFromiSrchGetDocumentInfo;
    }


    /* Read the document information */
    if ( (iError = iSrchDocumentGetDocumentInfo(psiSrchIndex, uiDocumentID, &psdiSpiDocumentInfo->pucTitle, NULL, 
            &psdiSpiDocumentInfo->uiRank, &psdiSpiDocumentInfo->uiTermCount, &psdiSpiDocumentInfo->ulAnsiDate, 
            &uiLanguageID, &psdiSrchDocumentItems, &uiSrchDocumentItemsLength, 0, false, true, false)) != SRCH_NoError ) {

        iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the document information for document ID: %u, index: '%s', srch error: %d.", 
                uiDocumentID, psiSrchIndex->pucIndexName, iError);
        goto bailFromiSrchGetDocumentInfo;
    }
    

    /* Set the index name for a physical index */
    if ( pssiSrchSearchIndex->uiIndexType == SRCH_SEARCH_INDEX_TYPE_PHYSICAL ) {
        
        /* Get the index name */
        if ( (iError = iSrchSearchGetIndexName(pssSrchSearch, psiSrchIndex, pucIndexName, SPI_INDEX_NAME_MAXIMUM_LENGTH + 1)) != SRCH_NoError ) {
            goto bailFromiSrchGetDocumentInfo;
        }
        
        /* Set the index name */
        if ( (psdiSpiDocumentInfo->pucIndexName = (unsigned char *)s_strdup(pucIndexName)) == NULL ) {
            iError = SPI_MemError;
            goto bailFromiSrchGetDocumentInfo;
        }
    }
    /* Set the index name for a virtual index */
    else if ( pssiSrchSearchIndex->uiIndexType == SRCH_SEARCH_INDEX_TYPE_VIRTUAL ) {
        
        /* Set the index name from the index info structure */
        if ( (psdiSpiDocumentInfo->pucIndexName = (unsigned char *)s_strdup(pssiSrchSearchIndex->pucIndexName)) == NULL ) {
            iError = SPI_MemError;
            goto bailFromiSrchGetDocumentInfo;
        }
    }
    /* Invalid index */
    else {
        iError = SPI_InvalidIndex;
        goto bailFromiSrchGetDocumentInfo;
    }


    /* Set the document key */
    if ( (psdiSpiDocumentInfo->pucDocumentKey = (unsigned char *)s_strdup(pucDocumentKey)) == NULL ) {
        iError = SPI_MemError;
        goto bailFromiSrchGetDocumentInfo;
    }
    

    /* Set the language, if it is valid */
    if ( iLngGetLanguageCodeFromID(uiLanguageID, pucLanguageCode, SPI_INDEX_LANGUAGE_MAXIMUM_LENGTH + 1) == LNG_NoError ) {
        if ( (psdiSpiDocumentInfo->pucLanguageCode = (unsigned char *)s_strdup(pucLanguageCode)) == NULL ) {
            iError = SPI_MemError;
            goto bailFromiSrchGetDocumentInfo;
        }
    }
    

    /* Loop through the document type profile array */
    for ( uiI = 0, psdiSrchDocumentItemsPtr = psdiSrchDocumentItems; uiI < uiSrchDocumentItemsLength; uiI++, psdiSrchDocumentItemsPtr++ ) {

        /* Get the item info for this item ID */
        if ( (iError = iSrchInfoGetItemInfo(psiSrchIndex, psdiSrchDocumentItemsPtr->uiItemID, pucItemName, SPI_ITEM_NAME_MAXIMUM_LENGTH + 1, pucMimeType, SPI_MIME_TYPE_MAXIMUM_LENGTH + 1)) != SRCH_NoError ) {

            /* Bust, this item name ID does not exist for some reason, bail */
            goto bailFromiSrchGetDocumentInfo;
        }


        /* Add the document item/mime type to the document item structure, tell it to take the data rather than copy it */
        if ( (iError = iSrchSearchAddEntryToSpiDocumentItems(pucItemName, pucMimeType, psdiSrchDocumentItemsPtr->pucUrl, psdiSrchDocumentItemsPtr->uiItemLength, 
                psdiSrchDocumentItemsPtr->pvData, psdiSrchDocumentItemsPtr->uiDataLength, false, &psdiSpiDocumentInfo->psdiSpiDocumentItems, &psdiSpiDocumentInfo->uiDocumentItemsLength)) != SRCH_NoError ) {

            goto bailFromiSrchGetDocumentInfo;
        }
        
        /* NULL out the data pointer */
        psdiSrchDocumentItemsPtr->pvData = NULL;
    }



    /* Bail label */
    bailFromiSrchGetDocumentInfo:


    /* Free the type profile array */
    s_free(psdiSrchDocumentItems);


    /* Handle the error */
    if ( iError == SPI_NoError ) {

        /* Set the return pointer */
        *ppsdiSpiDocumentInfo = psdiSpiDocumentInfo;
    }
    else {

        /* Adjust the error */
        iError = SPI_ERROR_VALID(iError) ? iError : SPI_InvalidDocumentKey;

        /* Free allocations */
        iSpiFreeDocumentInfo(psdiSpiDocumentInfo);
        psdiSpiDocumentInfo = NULL;
    }
    
    
    return(iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSpiGetIndexName()

    Purpose:    This function should allocate, populate and return the name of the 
                index for a index structure. If an error is returned, the
                return pointer will be ignored.

    Parameters: pssSpiSession   spi session structure
                pvIndex         index structure
                ppucIndexName   return pointer for the index name

    Globals:    none

    Returns:    SPI Error Code

*/
int iSpiGetIndexName
(
    struct spiSession *pssSpiSession,
    void *pvIndex,
    unsigned char **ppucIndexName
)
{

    int                         iError = SPI_NoError;
    struct srchSearch           *pssSrchSearch = NULL;
    struct srchSearchIndex      *pssiSrchSearchIndex = NULL;
    struct srchIndex            *psiSrchIndex = NULL;
    unsigned char               *pucIndexName = NULL;


/*     UtlLogDebug(UTL_LOG_CONTEXT, "iSpiGetIndexName."); */


    /* Check the parameters */
    if ( pssSpiSession == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pssSpiSession' parameter passed to 'iSpiGetIndexName'."); 
        return (SPI_InvalidSession);
    }

    if ( pvIndex == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvIndex' parameter passed to 'iSpiGetIndexName'."); 
        return (SPI_InvalidIndex);
    }

    if ( ppucIndexName == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppucIndexName' parameter passed to 'iSpiGetIndexName'."); 
        return (SPI_ReturnParameterError);
    }


    /* Check the search configuration */
    if ( (iError = iSrchSearchCheckSearchConfiguration(pssSpiSession)) != SPI_NoError ) {
        return (SPI_GetIndexNameFailed);
    }

    /* Dereference the search structure from the client pointer and check it */
    if ( (pssSrchSearch = (struct srchSearch *)pssSpiSession->pvClientPtr) == NULL ) {
        return (SPI_InvalidSession);
    }

    /* Dereference the index structure and check it */
    if ( (pssiSrchSearchIndex = (struct srchSearchIndex *)pvIndex) == NULL ) {
        return (SPI_InvalidIndex);
    }

    /* Check that the index array is valid */
    if ( (pssiSrchSearchIndex->ppsiSrchIndexList == NULL) || (pssiSrchSearchIndex->uiSrchIndexListLength == 0) ) {
        return (SPI_InvalidIndex);
    }

    /* Dereference the first index in the array and check it */
    if ( (psiSrchIndex = pssiSrchSearchIndex->ppsiSrchIndexList[0]) == NULL ) {
        return (SPI_InvalidIndex);
    }


    /* Get the index name for a physical index */
    if ( pssiSrchSearchIndex->uiIndexType == SRCH_SEARCH_INDEX_TYPE_PHYSICAL ) {
        
        unsigned char   pucLocalIndexName[SPI_INDEX_NAME_MAXIMUM_LENGTH + 1] = {'\0'};
        
        /* Get the index name */
        if ( (iError = iSrchSearchGetIndexName(pssSrchSearch, psiSrchIndex, pucLocalIndexName, SPI_INDEX_NAME_MAXIMUM_LENGTH + 1)) != SRCH_NoError ) {
            goto bailFromiSrchGetIndexName;
        }
        
        /* Set the index name */
        if ( (pucIndexName = (unsigned char *)s_strdup(pucLocalIndexName)) == NULL ) {
            iError = SPI_MemError;
            goto bailFromiSrchGetIndexName;
        }
    }
    /* Get the index name for a virtual index */
    else if ( pssiSrchSearchIndex->uiIndexType == SRCH_SEARCH_INDEX_TYPE_VIRTUAL ) {
        
        /* Set the index name from the index info structure */
        if ( (pucIndexName = (unsigned char *)s_strdup(pssiSrchSearchIndex->pucIndexName)) == NULL ) {
            iError = SPI_MemError;
            goto bailFromiSrchGetIndexName;
        }
    }
    /* Invalid index */
    else {
        iError = SPI_InvalidIndex;
        goto bailFromiSrchGetIndexName;
    }


    
    /* Bail label */
    bailFromiSrchGetIndexName:


    /* Handle the error */
    if ( iError == SPI_NoError ) {
    
        /* Set the return pointer */
        *ppucIndexName = pucIndexName;
    }
    else {

        /* Adjust the error */
        iError = SPI_ERROR_VALID(iError) ? iError : SPI_InvalidIndex;

        /* Free allocations */
        s_free(pucIndexName);
    }
        
    
    return (iError);

}


/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/


/* 
** ===================================================================================
** ============================  Search Engine functions  ============================
** ===================================================================================
*/


/*

    Function:   iSrchSearchCheckSearchConfiguration()

    Purpose:    This function will reinitialize the server if the last status change time
                on the search configuration file changes.
                
                Note that the client pointer in the spi session structure will be updated
                if the server is reinitialized, so it will have to be dereferenced (again)
                after this function is called.
                
                Finally this function is a little odd as it returns an SPI error code
                and not an SRCH error code.

    Parameters: pssSpiSession            spi session structure

    Globals:    none

    Returns:    SPI Error Code

*/
static int iSrchSearchCheckSearchConfiguration
(
    struct spiSession *pssSpiSession
)
{

    int                 iError = SPI_NoError;
    struct srchSearch   *pssSrchSearch = NULL;
    time_t              tConfigurationFileLastStatusChange = (time_t)0;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iSrchSearchCheckSearchConfiguration."); */


    ASSERT(pssSpiSession != NULL);


    /* Dereference the search structure from the client pointer and check it */
    if ( (pssSrchSearch = (struct srchSearch *)pssSpiSession->pvClientPtr) == NULL ) {
        return (SPI_InvalidSession);
    }


    /* Get the last status change time of the search configuration file if it exists and we can access it */
    if ( bUtlFilePathRead(pssSrchSearch->pucConfigurationFilePath) == true ) {
                
        /* Get the last status change time of the search configuration file */
        if ( (iError = iUtlFileGetPathStatusChangeTimeT(pssSrchSearch->pucConfigurationFilePath, &tConfigurationFileLastStatusChange)) != UTL_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the last status change time of the search configuration file: '%s', utl error: %d.", 
                    pssSrchSearch->pucConfigurationFilePath, iError); 
            return (SPI_InitializeServerFailed);
        }

        /* Reinitialize the server if the last status change time has changed */
        if ( pssSrchSearch->tConfigurationFileLastStatusChange != tConfigurationFileLastStatusChange ) {
            iUtlLogInfo(UTL_LOG_CONTEXT, "Reinitializing the search server because the search configuration file: '%s', has changed.", 
                    pssSrchSearch->pucConfigurationFilePath);
            iError = iSpiInitializeServer(pssSpiSession);
        }
    }


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchSearchGetIndexName()

    Purpose:    This function should return the name of the index from a
                index structure

    Parameters: pssSrchSearch       search structure
                psiSrchIndex        index structure
                pucIndexName        return pointer for the index name
                uiIndexNameLength   length of the return pointer

    Globals:    none

    Returns:    SRCH Error Code

*/
static int iSrchSearchGetIndexName
(
    struct srchSearch *pssSrchSearch,
    struct srchIndex *psiSrchIndex,
    unsigned char *pucIndexName,
    unsigned int uiIndexNameLength
)
{


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iSrchSearchGetIndexName."); */


    ASSERT(pssSrchSearch != NULL)
    ASSERT(psiSrchIndex != NULL)
    ASSERT(pucIndexName != NULL)
    ASSERT(uiIndexNameLength > 0)


    /* Copy the index name */    
    s_strnncpy(pucIndexName, psiSrchIndex->pucIndexName, uiIndexNameLength);


    return (SRCH_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchSearchProcessText()

    Purpose:    This function processed the passed utf-8 text and returns an
                allocated pointer to the text in wide character format.
                
                The text is processed as follows:
                    - text is cleaned, new lines are replaced with spaces
                    - text is trimmed
                    - text is unicode normalized if the normalizer is present
                    - text is converted from utf-8 to wide characters

    Parameters: pssSrchSearch   search structure
                pucText         text (optional)
                ppwcText        return pointer for the processed text (allocated)

    Globals:    none

    Returns:    SRCH error code

*/
static int iSrchSearchProcessText
(
    struct srchSearch *pssSrchSearch,
    unsigned char *pucText,
    wchar_t **ppwcText
)
{


    int             iError = SRCH_NoError;
    unsigned char   *pucTextPtr = NULL;
    unsigned char   *pucNormalizedText = NULL;
    unsigned int    uiNormalizedTextLength = 0; 
    wchar_t         *pwcText = NULL;
    unsigned int    uiTextLength = 0; 

#if defined(SRCH_SEARCH_ENABLE_SKIP_TEXT_CONVERSION_ERRORS)
    unsigned int    uiConverterErrorHandling = LNG_CONVERTER_SKIP_ON_ERROR;
#else /* defined(SRCH_SEARCH_ENABLE_SKIP_TEXT_CONVERSION_ERRORS) */
    unsigned int    uiConverterErrorHandling = LNG_CONVERTER_RETURN_ON_ERROR;
#endif /* defined(SRCH_SEARCH_ENABLE_SKIP_TEXT_CONVERSION_ERRORS) */


    ASSERT(pssSrchSearch != NULL);
    ASSERT(ppwcText != NULL);


    /* Return here if the text is null or empty */
    if ( bUtlStringsIsStringNULL(pucText) == true ) {
        return (SRCH_NoError);
    }
        
        
    /* Clean the text, replacing new lines with a space */
    iUtlStringsReplaceCharactersInString(pucText, "\n\r", ' ');


    /* Trim the text */
    iUtlStringsTrimString(pucText);


    /* Preset the text pointer */
    pucTextPtr = pucText;

    /* Normalize the text if we have a normalizer set up */
    if ( pssSrchSearch->pvLngUnicodeNormalizer != NULL ) {
        
        /* Normalize the text, set the text pointer if we succeeded, 
        ** note that pucNormalizedText is allocated so will need to be freed 
        */
        if ( iLngUnicodeNormalizeString(pssSrchSearch->pvLngUnicodeNormalizer, pucText, s_strlen(pucTextPtr), &pucNormalizedText, &uiNormalizedTextLength) == LNG_NoError ) {
            pucTextPtr = pucNormalizedText;
        }
    }
    
    /* At this point pucTextPtr now points to the text */


    /* Convert the text from utf-8 to wide characterss */
    if ( (iError = iLngConverterConvertString(pssSrchSearch->pvLngConverter, uiConverterErrorHandling, pucTextPtr, s_strlen(pucTextPtr), (unsigned char **)&pwcText, &uiTextLength)) != LNG_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to convert text from utf-8 to wide characters, text: '%s', lng error: %d.", pucTextPtr, iError);
        iError = SRCH_SearchCharacterSetConvertionFailed;
        goto bailFromiSrchSearchGetProcessedSearchTexts;
    }



    /* Bail label */
    bailFromiSrchSearchGetProcessedSearchTexts:
    
    
    /* Free allocations */
    s_free(pucNormalizedText);


    /* Handle the error */
    if ( iError == SRCH_NoError ) {

        /* Set the return pointer */
        *ppwcText = pwcText;
    }
    else {

        /* Free allocations */
        s_free(pwcText);
    }


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchSearchGetShortResultsFromSearch()

    Purpose:    This function searches the index and return an array of
                short results.

    Parameters: pssSrchSearch               search structure
                psiSrchIndex                index structure
                uiLanguageID                language ID
                pwcSearchText               search text (optional)
                pwcPositiveFeedbackText     positive feedback text (optional)
                pwcNegativeFeedbackText     negative feedback text (optional)
                uiStartIndex                start index
                uiEndIndex                  end index, 0 if there is no end index
                uiSortFieldType             sort field ID
                uiSortType                  sort type
                ppssrSrchShortResults       return pointer for an array of search short results
                puiSrchShortResultsLength   return pointer for the number of entries in the array of search short results
                puiTotalResults             return pointer for the total results 
                pdMaxSortKey                return pointer for the max weight

    Globals:    none

    Returns:    SRCH error code

*/
static int iSrchSearchGetShortResultsFromSearch
(
    struct srchSearch *pssSrchSearch,
    struct srchIndex *psiSrchIndex,
    unsigned int uiLanguageID,
    wchar_t *pwcSearchText,
    wchar_t *pwcPositiveFeedbackText,
    wchar_t *pwcNegativeFeedbackText,
    unsigned int uiStartIndex,
    unsigned int uiEndIndex,
    unsigned int uiSortFieldType, 
    unsigned int uiSortType,
    struct srchShortResult **ppssrSrchShortResults,
    unsigned int *puiSrchShortResultsLength,
    unsigned int *puiTotalResults,
    double *pdMaxSortKey
)
{

    int                         iError = SRCH_NoError;
    unsigned char               pucIndexName[SPI_INDEX_NAME_MAXIMUM_LENGTH + 1] = {'\0'};
    unsigned char               pucIndexDescription[SPI_INDEX_DESCRIPTION_MAXIMUM_LENGTH + 1] = {'\0'};
    unsigned char               *pucIndexDescriptionPtr = NULL;
    unsigned char               pucStemmerName[SRCH_INFO_SYMBOL_MAXIMUM_LENGTH + 1] = {'\0'};
    struct srchShortResult      *pssrSrchShortResults = NULL;
    unsigned int                uiSrchShortResultsLength = 0;

    struct srchPostingsList     *psplSrchPostingsList = NULL;
    struct srchWeight           *pswSrchWeight = NULL;
    boolean                     bDocumentTable = false;
    struct srchBitmap           *psbSrchBitmapExclusion = NULL;
    struct srchBitmap           *psbSrchBitmapInclusion = NULL;
    off_t                       zSearchReportStartOffset = 0;
    off_t                       zSearchReportEndOffset = 0;
    unsigned char               *pucSearchReportSnippet = NULL;
    unsigned int                uiSrchParserSearchCacheID = SRCH_PARSER_MODIFIER_UNKNOWN_ID;
    wchar_t                     *pwcSrchParserFullNormalizedSearchText = NULL;


    ASSERT(pssSrchSearch != NULL);
    ASSERT(psiSrchIndex != NULL);
    ASSERT(uiLanguageID >= 0);
    ASSERT((bUtlStringsIsWideStringNULL(pwcSearchText) == false) || (bUtlStringsIsWideStringNULL(pwcSearchText) == true));
    ASSERT((bUtlStringsIsWideStringNULL(pwcPositiveFeedbackText) == false) || (bUtlStringsIsWideStringNULL(pwcPositiveFeedbackText) == true));
    ASSERT((bUtlStringsIsWideStringNULL(pwcNegativeFeedbackText) == false) || (bUtlStringsIsWideStringNULL(pwcNegativeFeedbackText) == true));
    ASSERT(uiStartIndex >= 0);
    ASSERT(uiEndIndex >= 0);
    ASSERT(uiEndIndex >= uiStartIndex);
    ASSERT(SRCH_SEARCH_SORT_FIELD_TYPE_VALID(uiSortFieldType) == true);
    ASSERT(SPI_SORT_TYPE_VALID(uiSortType) == true);
    ASSERT(ppssrSrchShortResults != NULL);
    ASSERT(puiSrchShortResultsLength != NULL);
    ASSERT(puiTotalResults != NULL);
    ASSERT(pdMaxSortKey != NULL);


    /* Get the index description, default to the index name */
    pucIndexDescriptionPtr = NULL;
    if ( iSrchInfoGetDescriptionInfo(psiSrchIndex, pucIndexDescription, SPI_INDEX_DESCRIPTION_MAXIMUM_LENGTH + 1) == SRCH_NoError ) {
        pucIndexDescriptionPtr = pucIndexDescription;
    }
    else if ( iSrchSearchGetIndexName(pssSrchSearch, psiSrchIndex, pucIndexName, SPI_INDEX_NAME_MAXIMUM_LENGTH + 1) == SRCH_NoError ) {
        pucIndexDescriptionPtr = pucIndexName;
    }

    /* Add some preamble to the search report */
    iSrchReportAppend(pssSrchSearch->pvSrchReport, "%s %s\n", REP_INDEX_NAME, pucUtlStringsGetSafeString(pucIndexDescriptionPtr));
    iSrchReportAppend(pssSrchSearch->pvSrchReport, "%s %lu %lu %lu %lu %u\n", REP_INDEX_COUNTS, psiSrchIndex->ulTotalTermCount, 
            psiSrchIndex->ulUniqueTermCount, psiSrchIndex->ulTotalStopTermCount, psiSrchIndex->ulUniqueStopTermCount, psiSrchIndex->uiDocumentCount);


    /* Get the name of the stemmer and add it to the search report if we used one */
    if ( (iError = iSrchStemmerGetName(psiSrchIndex, pucStemmerName, SRCH_INFO_SYMBOL_MAXIMUM_LENGTH + 1)) != SRCH_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the stemmer name from the index: srch error: %d.", iError);
        goto bailFromiSrchSearchGetShortResultsFromSearch;
    }
    else {
        iSrchReportAppend(pssSrchSearch->pvSrchReport, "%s %s\n", REP_STEMMER_NAME, pucStemmerName);
    }


    /* Get the parser search cache ID */
    if ( (iError = iSrchParserGetModifierID(pssSrchSearch->pvSrchParser, SRCH_PARSER_MODIFIER_SEARCH_CACHE_ID, &uiSrchParserSearchCacheID)) != SRCH_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the parser search cache ID, srch error: %d.", iError);
        goto bailFromiSrchSearchGetShortResultsFromSearch;
    }

    /* Get the parser full normalized search text if the cache is enabled */
    if ( uiSrchParserSearchCacheID == SRCH_PARSER_MODIFIER_SEARCH_CACHE_ENABLE_ID ) {
        if ( (iError = iSrchParserGetFullNormalizedSearchText(pssSrchSearch->pvSrchParser, &pwcSrchParserFullNormalizedSearchText)) != SRCH_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the parser full normalized search text, srch error: %d.", iError);
            goto bailFromiSrchSearchGetShortResultsFromSearch;
        }
    }



    /* Check the cache */
    if ( uiSrchParserSearchCacheID == SRCH_PARSER_MODIFIER_SEARCH_CACHE_ENABLE_ID ) {
    
        /* Get the short results from the cache */
        if ( (iError = iSrchCacheGetSearchShortResults(pssSrchSearch->pvSrchCache, psiSrchIndex, pwcSrchParserFullNormalizedSearchText, 
                pwcPositiveFeedbackText, pwcNegativeFeedbackText, uiSortType, &pssrSrchShortResults, &uiSrchShortResultsLength, 
                puiTotalResults, pdMaxSortKey, &pucSearchReportSnippet)) == SRCH_NoError ) {

            /* Append the search report snippet we got back to the search report */
            if ( pucSearchReportSnippet != NULL ) {
                iSrchReportAppend(pssSrchSearch->pvSrchReport, "%s", pucSearchReportSnippet);
            }

            /* Free the search report snippet */
            s_free(pucSearchReportSnippet);

            /* Splice the short results */
            if ( (pssrSrchShortResults != NULL) && (uiSrchShortResultsLength > 0) ) {
                if ( (iError = iSrchShortResultSplice(&pssrSrchShortResults, &uiSrchShortResultsLength, uiStartIndex, uiEndIndex, uiSortType)) != SRCH_NoError ) {
                    iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the splice the short results, srch error: %d.", iError);
                    goto bailFromiSrchSearchGetShortResultsFromSearch;
                }
            }


            /* We are all set */
            goto bailFromiSrchSearchGetShortResultsFromSearch;
        }


        /* Get the search report start index, we need to extract the part of the search report that 
        ** gets generated when we run the search and store it in the cache otherwise it gets lost
        */
        if ( iSrchReportGetReportOffset(pssSrchSearch->pvSrchReport, &zSearchReportStartOffset) != SRCH_NoError ) {
            zSearchReportStartOffset = -1;
        }
    }
    
    

    /* Do the search */
    if ( (iError = iSrchSearchGetRawResultsFromSearch(pssSrchSearch, psiSrchIndex, uiLanguageID, pwcSearchText, 
            pwcPositiveFeedbackText, pwcNegativeFeedbackText, uiSortFieldType, uiSortType, &psplSrchPostingsList, &pswSrchWeight, 
            &bDocumentTable, &psbSrchBitmapExclusion, &psbSrchBitmapInclusion)) != SRCH_NoError ) {
        goto bailFromiSrchSearchGetShortResultsFromSearch;
    }



    /* Do we have any results to merge? */
    if ( (psplSrchPostingsList != NULL) || (pswSrchWeight != NULL) || (bDocumentTable == true) ) {

        /* We have now got all our results - merge them into a short results array */
        iError = iSrchSearchGetShortResultsFromRawResults(pssSrchSearch, psiSrchIndex, psplSrchPostingsList, pswSrchWeight, bDocumentTable,
                psbSrchBitmapExclusion, psbSrchBitmapInclusion, uiSortFieldType, uiSortType, &pssrSrchShortResults, &uiSrchShortResultsLength, 
                puiTotalResults, pdMaxSortKey);

        /* Free the search postings list */
        iSrchPostingFreeSrchPostingsList(psplSrchPostingsList);
        psplSrchPostingsList = NULL;

        /* Free the search weight structure */
        iSrchWeightFree(pswSrchWeight);
        pswSrchWeight = NULL;
        
        /* Free the exclusion bitmap */
        iSrchBitmapFree(psbSrchBitmapExclusion);
        psbSrchBitmapExclusion = NULL;

        /* Free the inclusion bitmap */
        iSrchBitmapFree(psbSrchBitmapInclusion);
        psbSrchBitmapInclusion = NULL;

        /* Did we hit an error on merging */
        if ( iError != SRCH_NoError ) {
            goto bailFromiSrchSearchGetShortResultsFromSearch;
        }
    }



    /* Save the short results structure array to the cache */
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

        /* Save the short results structure array in the cache, ignoring any errors */
        iSrchCacheSaveSearchShortResults(pssSrchSearch->pvSrchCache, psiSrchIndex, pwcSrchParserFullNormalizedSearchText, 
                pwcPositiveFeedbackText, pwcNegativeFeedbackText, uiSortType, pssrSrchShortResults, uiSrchShortResultsLength, 
                *puiTotalResults, *pdMaxSortKey, pucSearchReportSnippet);

        /* Free the search report snippet */
        s_free(pucSearchReportSnippet);
    }



    /* Splice the short results */
    if ( (pssrSrchShortResults != NULL) && (uiSrchShortResultsLength > 0) ) {
        if ( (iError = iSrchShortResultSplice(&pssrSrchShortResults, &uiSrchShortResultsLength, uiStartIndex, uiEndIndex, uiSortType)) != SRCH_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the splice the short results: srch error: %d.", iError);
            goto bailFromiSrchSearchGetShortResultsFromSearch;
        }
    }

    
    
    /* Bail label */
    bailFromiSrchSearchGetShortResultsFromSearch:
    

    /* Free the search report snippet */
    s_free(pucSearchReportSnippet);


    /* Handle the error */ 
    if ( iError == SRCH_NoError ) {

        /* Set the return pointers */
        *ppssrSrchShortResults = pssrSrchShortResults;
        *puiSrchShortResultsLength = uiSrchShortResultsLength;
    }
    else {
    
        /* Free the search short results */
        iSrchShortResultFree(pssrSrchShortResults, uiSrchShortResultsLength, uiSortType);
        pssrSrchShortResults = NULL;
    }
    

    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchSearchGetRawResultsFromSearch()

    Purpose:    This function searches for the passed search and
                returns the results as a search postings list structure,
                a weight array and an exclusion bitmap

                If no documents were returned from the search
                then the weights array return pointer will be NULL.

    Parameters: pssSrchSearch               search structure
                psiSrchIndex                search index structure
                uiLanguageID                language ID
                pwcSearchText               search text (optional)
                pwcPositiveFeedbackText     positive feedback text (optional)
                pwcNegativeFeedbackText     negative feedback text (optional)
                uiSortFieldType             sort field ID
                uiSortType                  sort type
                ppsplSrchPostingsList       return pointer for the search postings list structure
                ppswSrchWeight              return pointer for the search weight structure
                pbDocumentTable             return pointer for the document table flag
                ppsbSrchBitmapExclusion     return pointer for the exclusion search bitmap structure
                ppsbSrchBitmapInclusion     return pointer for the inclusion search bitmap structure

    Globals:    none

    Returns:    SRCH error

*/
static int iSrchSearchGetRawResultsFromSearch
(
    struct srchSearch *pssSrchSearch,
    struct srchIndex* psiSrchIndex,
    unsigned int uiLanguageID, 
    wchar_t *pwcSearchText,
    wchar_t *pwcPositiveFeedbackText,
    wchar_t *pwcNegativeFeedbackText,
    unsigned int uiSortFieldType, 
    unsigned int uiSortType,
    struct srchPostingsList **ppsplSrchPostingsList,
    struct srchWeight **ppswSrchWeight,
    boolean *pbDocumentTable,
    struct srchBitmap **ppsbSrchBitmapExclusion,
    struct srchBitmap **ppsbSrchBitmapInclusion
)
{

    int                             iError = SRCH_NoError;
    unsigned int                    uiSrchParserDebugID = SRCH_PARSER_MODIFIER_UNKNOWN_ID;
    struct srchParserTermCluster    *psptcSrchParserTermCluster = NULL;
    wchar_t                         *pwcSrchParserNormalizedSearchText = NULL;
    struct srchParserNumber         *pspnSrchParserNumberDates = NULL;
    struct srchParserNumber         *pspnSrchParserNumberDatesPtr = NULL;
    unsigned int                    uiSrchParserNumberDatesLength = 0;
    struct srchParserFilter         *pspfSrchParserFilterExclusionFilters = NULL;
    unsigned int                    uiSrchParserFilterExclusionFiltersLength = 0;
    struct srchParserFilter         *pspfSrchParserFilterInclusionFilters = NULL;
    unsigned int                    uiSrchParserFilterInclusionFiltersLength = 0;
    struct srchParserNumber         *pspnSrchParserNumberLanguageIDs = NULL;
    unsigned int                    uiSrchParserNumberLanguageIDsLength = 0;
    struct srchPostingsList         *psplSrchPostingsList = NULL;
    struct srchWeight               *pswSrchWeight = NULL;
    unsigned int                    uiI = 0;
    unsigned char                   pucNumberString[UTL_FILE_PATH_MAX + 1] = {'\0'};
    
    unsigned int                    uiStartDocumentID = 0;
    unsigned int                    uiEndDocumentID = 0;


    ASSERT(pssSrchSearch != NULL);
    ASSERT(psiSrchIndex != NULL);
    ASSERT(uiLanguageID >= 0);
    ASSERT((bUtlStringsIsWideStringNULL(pwcSearchText) == false) || (bUtlStringsIsWideStringNULL(pwcSearchText) == true));
    ASSERT((bUtlStringsIsWideStringNULL(pwcPositiveFeedbackText) == false) || (bUtlStringsIsWideStringNULL(pwcPositiveFeedbackText) == true));
    ASSERT((bUtlStringsIsWideStringNULL(pwcNegativeFeedbackText) == false) || (bUtlStringsIsWideStringNULL(pwcNegativeFeedbackText) == true));
    ASSERT(SRCH_SEARCH_SORT_FIELD_TYPE_VALID(uiSortFieldType) == true);
    ASSERT(SPI_SORT_TYPE_VALID(uiSortType) == true);
    ASSERT(ppsplSrchPostingsList != NULL);
    ASSERT(ppswSrchWeight != NULL);
    ASSERT(pbDocumentTable != NULL);
    ASSERT(ppsbSrchBitmapExclusion != NULL);
    ASSERT(ppsbSrchBitmapInclusion != NULL);



    /* Get the parser term cluster */
    if ( (iError = iSrchParserGetTermCluster(pssSrchSearch->pvSrchParser, &psptcSrchParserTermCluster)) != SRCH_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the parser term cluster, srch error: %d.", iError);
        goto bailFromiSrchSearchGetRawResultsFromSearch;
    }

    /* Get the parser date restriction */
    if ( (iError = iSrchParserGetDates(pssSrchSearch->pvSrchParser, &pspnSrchParserNumberDates, &uiSrchParserNumberDatesLength)) != SRCH_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the parser date restriction, srch error: %d.", iError);
        goto bailFromiSrchSearchGetRawResultsFromSearch;
    }

    /* Get the parser exclusion filter */
    if ( (iError = iSrchParserGetExclusionFilters(pssSrchSearch->pvSrchParser, &pspfSrchParserFilterExclusionFilters, &uiSrchParserFilterExclusionFiltersLength)) != SRCH_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the parser exclusion filters, srch error: %d.", iError);
        goto bailFromiSrchSearchGetRawResultsFromSearch;
    }

    /* Get the parser inclusion filter */
    if ( (iError = iSrchParserGetInclusionFilters(pssSrchSearch->pvSrchParser, &pspfSrchParserFilterInclusionFilters, &uiSrchParserFilterInclusionFiltersLength)) != SRCH_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the parser inclusion filters, srch error: %d.", iError);
        goto bailFromiSrchSearchGetRawResultsFromSearch;
    }

    /* Get the parser language ID restriction */
    if ( (iError = iSrchParserGetLanguage(pssSrchSearch->pvSrchParser, &pspnSrchParserNumberLanguageIDs, &uiSrchParserNumberLanguageIDsLength)) != SRCH_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the parser language ID restriction, srch error: %d.", iError);
        goto bailFromiSrchSearchGetRawResultsFromSearch;
    }


    /* Get the parser debug ID */
    if ( (iError = iSrchParserGetModifierID(pssSrchSearch->pvSrchParser, SRCH_PARSER_MODIFIER_DEBUG_ID, &uiSrchParserDebugID)) != SRCH_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the parser debug ID, srch error: %d.", iError);
        goto bailFromiSrchSearchGetRawResultsFromSearch;
    }

    /* Get the parser normalized search text, get the full or regular depending on whether debug is on or not */
    if ( uiSrchParserDebugID == SRCH_PARSER_MODIFIER_DEBUG_ENABLE_ID ) {
        if ( (iError = iSrchParserGetFullNormalizedSearchText(pssSrchSearch->pvSrchParser, &pwcSrchParserNormalizedSearchText)) != SRCH_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the parser full normalized search text, srch error: %d.", iError);
            goto bailFromiSrchSearchGetRawResultsFromSearch;
        }
    }
    else {
        if ( (iError = iSrchParserGetNormalizedSearchText(pssSrchSearch->pvSrchParser, &pwcSrchParserNormalizedSearchText)) != SRCH_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the parser normalized search text, srch error: %d.", iError);
            goto bailFromiSrchSearchGetRawResultsFromSearch;
        }
    }


    /* Catch empty searches here */
    if ( (psptcSrchParserTermCluster == NULL) && 
            (bUtlStringsIsWideStringNULL(pwcPositiveFeedbackText) == true) && (bUtlStringsIsWideStringNULL(pwcNegativeFeedbackText) == true) &&
            (pspnSrchParserNumberDates == NULL) && 
            (pspfSrchParserFilterExclusionFilters == NULL) && (pspfSrchParserFilterInclusionFilters == NULL) &&
            (pspnSrchParserNumberLanguageIDs == NULL) ) {
        iSrchReportAppend(pssSrchSearch->pvSrchReport, "%s The search did not contain any search terms or any relevance feedback documents (positive or negative)\n", REP_SEARCH_ERROR);
        goto bailFromiSrchSearchGetRawResultsFromSearch;
    }


    /* List the original and normalized searches in the search report */
    if ( bUtlStringsIsWideStringNULL(pwcSearchText) == false ) {
        iSrchReportAppend(pssSrchSearch->pvSrchReport, "%s %ls\n", REP_SEARCH_ORIGINAL, pwcSearchText);
    }

    if ( bUtlStringsIsWideStringNULL(pwcSrchParserNormalizedSearchText) == false ) {
        iSrchReportAppend(pssSrchSearch->pvSrchReport, "%s %ls\n", REP_SEARCH_REFORMATTED, pwcSrchParserNormalizedSearchText);
    }

    /* List the date restriction in the search report */
    if ( pspnSrchParserNumberDates != NULL ) {

        iSrchReportAppend(pssSrchSearch->pvSrchReport, "%s The search was restricted to documents created", REP_SEARCH_RESTRICTION);

        /* Loop over each restriction */
        for ( uiI = 0, pspnSrchParserNumberDatesPtr = pspnSrchParserNumberDates; uiI < uiSrchParserNumberDatesLength; uiI++, pspnSrchParserNumberDatesPtr++ ) {

            /* Print an 'and' if there is more than one date */
            if ( uiI > 0 ) {
                iSrchReportAppend(pssSrchSearch->pvSrchReport, " and");
            }

            /* Print the range operator text */
            switch ( pspnSrchParserNumberDatesPtr->uiRangeID ) {

                case SRCH_PARSER_RANGE_NOT_EQUAL_ID:
                    iSrchReportAppend(pssSrchSearch->pvSrchReport, " not on");
                    break;

                case SRCH_PARSER_RANGE_GREATER_OR_EQUAL_ID:
                    iSrchReportAppend(pssSrchSearch->pvSrchReport, " on or after");
                    break;

                case SRCH_PARSER_RANGE_LESS_OR_EQUAL_ID:
                    iSrchReportAppend(pssSrchSearch->pvSrchReport, " on or before");
                    break;

                case SRCH_PARSER_RANGE_GREATER_ID:
                    iSrchReportAppend(pssSrchSearch->pvSrchReport, " after");
                    break;

                case SRCH_PARSER_RANGE_LESS_ID:
                    iSrchReportAppend(pssSrchSearch->pvSrchReport, " before");
                    break;

                case SRCH_PARSER_RANGE_EQUAL_ID:
                    iSrchReportAppend(pssSrchSearch->pvSrchReport, " on");
                    break;
            }

            /* Print the date */
            if ( iUtlDateGetWebDateFromAnsiDate(pspnSrchParserNumberDatesPtr->ulNumber, pucNumberString, UTL_FILE_PATH_MAX + 1) == UTL_NoError ) {
                iSrchReportAppend(pssSrchSearch->pvSrchReport, " %s", pucNumberString);
            }
            else {
                iSrchReportAppend(pssSrchSearch->pvSrchReport, " (invalid date)");
            }
        }
        
        iSrchReportAppend(pssSrchSearch->pvSrchReport, "\n");
    }


    /* Process the parser term cluster */
    if ( psptcSrchParserTermCluster != NULL ) {
        
        if ( (iError = iSrchSearchGetPostingsListFromParserTermCluster(pssSrchSearch, psiSrchIndex, uiLanguageID, 
                psptcSrchParserTermCluster, &psplSrchPostingsList)) != SRCH_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to run the search, srch error: %d.", iError);
            goto bailFromiSrchSearchGetRawResultsFromSearch;
        }

        // Set the start and end document ID if there is a posting list */
        if ( (psplSrchPostingsList != NULL) && (psplSrchPostingsList->pspSrchPostings != NULL) && (psplSrchPostingsList->uiSrchPostingsLength > 0) ) {
            uiStartDocumentID = psplSrchPostingsList->pspSrchPostings->uiDocumentID;
            uiEndDocumentID = (psplSrchPostingsList->pspSrchPostings + (psplSrchPostingsList->uiSrchPostingsLength - 1))->uiDocumentID;
        }
    }


    /* Process any positive feedback or negative feedback */
    if ( (bUtlStringsIsWideStringNULL(pwcPositiveFeedbackText) == false) || (bUtlStringsIsWideStringNULL(pwcNegativeFeedbackText) == false) ) {

        /* Note that a weights array wont be returned if none of the terms were found to be relevant */
        if ( (iError = iSrchFeedbackGetSearchWeightFromFeedbackTexts(pssSrchSearch, psiSrchIndex, uiLanguageID, 
                pwcPositiveFeedbackText, pwcNegativeFeedbackText, uiStartDocumentID, uiEndDocumentID, &pswSrchWeight)) != SRCH_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to run the feedback searches, srch error: %d.", iError);
            goto bailFromiSrchSearchGetRawResultsFromSearch;
        }
    }
    

    /* Process the exclusion filter */
    if ( pspfSrchParserFilterExclusionFilters != NULL ) {

        if ( (iError = iSrchFilterGetSearchBitmapFromFilters(pssSrchSearch, psiSrchIndex, uiLanguageID, pspfSrchParserFilterExclusionFilters, 
                uiSrchParserFilterExclusionFiltersLength, uiStartDocumentID, uiEndDocumentID, SRCH_FILTER_BITMAP_MERGE_TYPE_OR, 
                ppsbSrchBitmapExclusion)) != SRCH_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to run the exclusion filters, srch error: %d.", iError);
            goto bailFromiSrchSearchGetRawResultsFromSearch;
        }
    }


    /* Process the inclusion filter */
    if ( pspfSrchParserFilterInclusionFilters != NULL ) {

        if ( (iError = iSrchFilterGetSearchBitmapFromFilters(pssSrchSearch, psiSrchIndex, uiLanguageID, pspfSrchParserFilterInclusionFilters, 
                uiSrchParserFilterInclusionFiltersLength, uiStartDocumentID, uiEndDocumentID, SRCH_FILTER_BITMAP_MERGE_TYPE_AND, 
                ppsbSrchBitmapInclusion)) != SRCH_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to run the inclusion filters, srch error: %d.", iError);
            goto bailFromiSrchSearchGetRawResultsFromSearch;
        }
    }


    /* Flag the document table if there was no term cluster or search weights, but there are restrictions */
    if ( (psptcSrchParserTermCluster == NULL) && (pswSrchWeight == NULL) && 
            ((pspnSrchParserNumberDates != NULL) || 
            (pspfSrchParserFilterExclusionFilters != NULL) || (pspfSrchParserFilterInclusionFilters != NULL) ||
            (pspnSrchParserNumberLanguageIDs != NULL)) ) {

        *pbDocumentTable = true;
    }



    /* Bail label */
    bailFromiSrchSearchGetRawResultsFromSearch:


    /* Handle the error */
    if ( iError == SRCH_NoError ) {

        /* Set the return pointers */
        *ppsplSrchPostingsList = psplSrchPostingsList;
        *ppswSrchWeight = pswSrchWeight;
    }
    else {

        /* Free the postings list */
        iSrchPostingFreeSrchPostingsList(psplSrchPostingsList);
        psplSrchPostingsList = NULL;

        /* Free the weights array */
        iSrchWeightFree(pswSrchWeight);
        pswSrchWeight = NULL;
    }


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchSearchGetShortResultsFromRawResults()

    Purpose:    This functions merges a search postings list and a weights array
                into a search short results array. The array is sorted.

    Parameters: pssSrchSearch               search structure
                psiSrchIndex                index structure
                psplSrchPostingsList        search postings list structure
                pswSrchWeight               search weight structure
                bDocumentTable              document table flag
                psbSrchBitmapExclusion      exclusion search bitmap structure
                psbSrchBitmapInclusion      inclusion search bitmap structure
                uiSortFieldType             sort field ID
                uiSortType                  sort type
                ppssrSrchShortResults       return pointer for an array of search short results
                puiSrchShortResultsLength   return pointer for the number of entries in the array of search short results
                puiTotalResults             return pointer for the total results
                pdMaxSortKey                return pointer for the max weight

    Globals:    none

    Returns:    SRCH error code

*/
static int iSrchSearchGetShortResultsFromRawResults
(
    struct srchSearch *pssSrchSearch,
    struct srchIndex *psiSrchIndex,
    struct srchPostingsList *psplSrchPostingsList,
    struct srchWeight *pswSrchWeight,
    boolean bDocumentTable,
    struct srchBitmap *psbSrchBitmapExclusion,
    struct srchBitmap *psbSrchBitmapInclusion,
    unsigned int uiSortFieldType, 
    unsigned int uiSortType,
    struct srchShortResult **ppssrSrchShortResults,
    unsigned int *puiSrchShortResultsLength,
    unsigned int *puiTotalResults,
    double *pdMaxSortKey
)
{

    int                         iError = SRCH_NoError;
    unsigned int                uiDocumentID = 0;
    struct srchPosting          *pspSrchPostingsPtr = NULL;
    struct srchPosting          *pspSrchPostingsEnd = NULL;
    struct srchShortResult      *pssrSrchShortResults = NULL;
    struct srchShortResult      *pssrSrchShortResultsPtr = NULL;
    float                       *pfWeightsPtr = NULL;
    float                       *pfWeightsEnd = NULL;
    unsigned int                uiSrchShortResultsLength = 0;
    unsigned int                uiFailedDateMatchCount = 0;
    unsigned int                uiFailedLanguageMatchCount = 0;
    unsigned int                uiFailedTermCountMatchCount = 0;
    unsigned int                uiExcludedDocumentCount = 0;
    unsigned int                uiIncludedDocumentCount = 0;

    struct srchParserNumber     *pspnSrchParserNumberDates = NULL;
    unsigned int                uiSrchParserNumberDatesLength = 0;
    unsigned long               *pulSearchAnsiDate = NULL;
    struct srchParserNumber     *pspnSrchParserNumberLanguageIDs = NULL;
    unsigned int                uiSrchParserNumberLanguageIDsLength = 0;
    struct srchParserNumber     *pspnSrchParserNumberLanguageIDsPtr = NULL;
    unsigned int                uiI = 0;

    unsigned int                uiSrchParserSearchResultsID = SRCH_PARSER_MODIFIER_UNKNOWN_ID;

    unsigned int                uiTotalResults = 0;
    double                      dMaxSortKey = 0;
    
    unsigned int                uiItemID = 0;

#if defined(SRCH_SEARCH_ENABLE_DOCUMENT_WEIGHT_NORMALIZATION_AVERAGE)
    float                       fAverageDocumentTermCount = 0;
#endif /* defined(SRCH_SEARCH_ENABLE_DOCUMENT_WEIGHT_NORMALIZATION_AVERAGE) */


    ASSERT(pssSrchSearch != NULL);
    ASSERT(psiSrchIndex != NULL);
    ASSERT((psplSrchPostingsList != NULL) || (psplSrchPostingsList == NULL));
    ASSERT((pswSrchWeight != NULL) || (pswSrchWeight == NULL));
    ASSERT((bDocumentTable == true) || (bDocumentTable == false));
    ASSERT((psbSrchBitmapExclusion != NULL) || (psbSrchBitmapExclusion == NULL));
    ASSERT((psbSrchBitmapInclusion != NULL) || (psbSrchBitmapInclusion == NULL));
    ASSERT(SRCH_SEARCH_SORT_FIELD_TYPE_VALID(uiSortFieldType) == true);
    ASSERT(SPI_SORT_TYPE_VALID(uiSortType) == true);
    ASSERT(ppssrSrchShortResults != NULL);
    ASSERT(puiSrchShortResultsLength != NULL);
    ASSERT(puiTotalResults != NULL);
    ASSERT(pdMaxSortKey != NULL);


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iSrchSearchGetShortResultsFromRawResults - uiStartIndex : %u, uiEndIndex : %u, psplSrchPostingsList->uiDocumentCount : %u",  */
/*             uiStartIndex, uiEndIndex, (psplSrchPostingsList != NULL) ? psplSrchPostingsList->uiDocumentCount : 0); */


    /* Got postings, and we might also have gotten weights, and if we did we can only 
    ** return documents that are in the postings list, all the weights would do is 
    ** adjust the weights from the postings 
    */
    if ( psplSrchPostingsList != NULL ) {

        struct srchPosting      *pspSrchPostingsLastPtr = NULL;
    
        ASSERT(iSrchPostingCheckSrchPostingsList(psplSrchPostingsList) == SRCH_NoError);

        /* Count up the number of actual results in the postings array */
        for ( pspSrchPostingsPtr = psplSrchPostingsList->pspSrchPostings + 1, pspSrchPostingsLastPtr = psplSrchPostingsList->pspSrchPostings, 
                pspSrchPostingsEnd = psplSrchPostingsList->pspSrchPostings + psplSrchPostingsList->uiSrchPostingsLength,
                uiSrchShortResultsLength = 0; pspSrchPostingsPtr < pspSrchPostingsEnd; pspSrchPostingsPtr++, pspSrchPostingsLastPtr++ ) {

            /* Have we hit a new doc ID? */
            if ( pspSrchPostingsLastPtr->uiDocumentID != pspSrchPostingsPtr->uiDocumentID ) {
                uiSrchShortResultsLength++;
            }
        }
        
        if ( psplSrchPostingsList->uiSrchPostingsLength ) {
            uiSrchShortResultsLength++;
        }
    }
    /* Got weights */
    else if ( pswSrchWeight != NULL ) {

        /* Count up the number of actual results in the weights array, documentID 0 does not exist, hence the '+ 1' */
        for ( pfWeightsPtr = pswSrchWeight->pfWeights + 1, pfWeightsEnd = pswSrchWeight->pfWeights + pswSrchWeight->uiWeightsLength, uiSrchShortResultsLength = 0; 
                pfWeightsPtr < pfWeightsEnd; pfWeightsPtr++ ) {

            /* Increment the number of documents we hit upon */
            if ( *pfWeightsPtr > 0 ) {
                uiSrchShortResultsLength += 1;
            }
        }
    }
    /* Got document table */
    else  if ( bDocumentTable == true ) {
        uiSrchShortResultsLength = psiSrchIndex->uiDocumentCount;
    }

/*     iUtlLogDebug(UTL_LOG_CONTEXT, "uiSrchShortResultsLength : %u", uiSrchShortResultsLength); */

    /* Just return if there were no results */
    if ( uiSrchShortResultsLength == 0 ) {
        iSrchReportAppend(pssSrchSearch->pvSrchReport, "%s 0 0 0 0 0 0 1\n", REP_RETRIEVAL_COUNTS);
        iError = SPI_NoError;
        goto bailFromiSrchSearchGetShortResultsFromRawResults;
    }


    /* Allocate the array of short results based on the initial estimated number of short results */
    if ( (pssrSrchShortResults = (struct srchShortResult *)s_malloc((size_t)(sizeof(struct srchShortResult) * uiSrchShortResultsLength))) == NULL ) {
        iError = SRCH_MemError;
        goto bailFromiSrchSearchGetShortResultsFromRawResults;
    }



    /* Get the parser date/time restriction */
    if ( (iError = iSrchParserGetDates(pssSrchSearch->pvSrchParser, &pspnSrchParserNumberDates, &uiSrchParserNumberDatesLength)) != SRCH_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the parser date restriction, srch error: %d.", iError);
        goto bailFromiSrchSearchGetShortResultsFromRawResults;
    }

    /* Get the parser language ID restriction */
    if ( (iError = iSrchParserGetLanguage(pssSrchSearch->pvSrchParser, &pspnSrchParserNumberLanguageIDs, &uiSrchParserNumberLanguageIDsLength)) != SRCH_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the parser language ID restriction, srch error: %d.", iError);
        goto bailFromiSrchSearchGetShortResultsFromRawResults;
    }
    

    /* Get the canonical language ID for this language */
    if ( pspnSrchParserNumberLanguageIDs != NULL ) {
        
        /* Add the language restriction to the search report */
        iSrchReportAppend(pssSrchSearch->pvSrchReport, "%s The search filtered on the following language%s: ", REP_SEARCH_RESTRICTION, 
                (uiSrchParserNumberLanguageIDsLength > 1) ? "s" : "");

        /* Loop over the languages */
        for ( uiI = 0, pspnSrchParserNumberLanguageIDsPtr = pspnSrchParserNumberLanguageIDs; uiI < uiSrchParserNumberLanguageIDsLength; uiI++, pspnSrchParserNumberLanguageIDsPtr++ ) {
        
            unsigned char   pucLanguageCode[SPI_INDEX_LANGUAGE_MAXIMUM_LENGTH + 1];

            /* Check whether the language is canonical or not, store that information in the range ID */
            if ( (iError = iLngIsLanguageIDCanonical(pspnSrchParserNumberLanguageIDsPtr->ulNumber, (boolean *)&pspnSrchParserNumberLanguageIDsPtr->uiRangeID)) != LNG_NoError ) {
                iUtlLogError(UTL_LOG_CONTEXT, "Failed to find out if language ID: %u is canonical or not, lng error: %d.", (unsigned int)pspnSrchParserNumberLanguageIDsPtr->ulNumber, iError);
                iError = SRCH_MiscError;
                goto bailFromiSrchSearchGetShortResultsFromRawResults;
            }
        
            /* Get the language code for this language ID */
            if ( (iError = iLngGetLanguageCodeFromID(pspnSrchParserNumberLanguageIDsPtr->ulNumber, pucLanguageCode, SPI_INDEX_LANGUAGE_MAXIMUM_LENGTH + 1)) != LNG_NoError ) {
                iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the language code from the language ID: %u, lng error: %d.", (unsigned int)pspnSrchParserNumberLanguageIDsPtr->ulNumber, iError);
                iError = SRCH_MiscError;
                goto bailFromiSrchSearchGetShortResultsFromRawResults;
            }
            
            /* Add the language restriction to the search report */
            iSrchReportAppend(pssSrchSearch->pvSrchReport, "'%s'", pucLanguageCode);
            
            if ( uiI < (uiSrchParserNumberLanguageIDsLength - 1) ) {
                iSrchReportAppend(pssSrchSearch->pvSrchReport, ", ");
            }
        }

        iSrchReportAppend(pssSrchSearch->pvSrchReport, "\n");
    }



    /* We need to get the item name ID for this item name if we are sorting by item, we do make the assumption that if we are
    ** sorting by item then an item name matching the sort field actually exists
    */
    if ( (uiSortFieldType == SRCH_SEARCH_SORT_FIELD_TYPE_ITEM_DOUBLE) || (uiSortFieldType == SRCH_SEARCH_SORT_FIELD_TYPE_ITEM_FLOAT) || 
            (uiSortFieldType == SRCH_SEARCH_SORT_FIELD_TYPE_ITEM_UINT) || (uiSortFieldType == SRCH_SEARCH_SORT_FIELD_TYPE_ITEM_ULONG) || 
            (uiSortFieldType == SRCH_SEARCH_SORT_FIELD_TYPE_ITEM_UCHAR) ) {
        
        unsigned int    uiLocalSortOrderID = SRCH_PARSER_INVALID_ID;
        wchar_t         *pwcSrchParserSortFieldName = NULL;
        unsigned char   pucSrchParserSortFieldName[SRCH_INFO_SYMBOL_MAXIMUM_LENGTH + 1] = {'\0'};


        /* Get the parser sort modifier */
        if ( (iError = iSrchParserGetSort(pssSrchSearch->pvSrchParser, &pwcSrchParserSortFieldName, &uiLocalSortOrderID)) != SRCH_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the parser debug flag, srch error: %d.", iError);
            goto bailFromiSrchSearchGetShortResultsFromRawResults;
        }

        /* Convert the sort field name from wide characters to utf-8 */
        if ( (iError = iLngConvertWideStringToUtf8_s(pwcSrchParserSortFieldName, 0, pucSrchParserSortFieldName, SRCH_INFO_SYMBOL_MAXIMUM_LENGTH + 1)) != LNG_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to convert the sort field name from wide characters to utf-8, lng error: %d.", iError);
            goto bailFromiSrchSearchGetShortResultsFromRawResults;
        }


        /* Loop over each item */
        for ( uiI = 1; ; uiI++ ) {
            
            unsigned char   pucItemName[SPI_ITEM_NAME_MAXIMUM_LENGTH + 1] = {'\0'};
            unsigned char   pucMimeType[SPI_MIME_TYPE_MAXIMUM_LENGTH + 1] = {'\0'};

            /* Get the item info */
            if ( (iError = iSrchInfoGetItemInfo(psiSrchIndex, uiI, pucItemName, SPI_ITEM_NAME_MAXIMUM_LENGTH + 1, pucMimeType, SPI_MIME_TYPE_MAXIMUM_LENGTH + 1)) == SRCH_NoError ) {
                        
                /* If the item name is the same as the sort field, we set the item name ID and break */
                if ( s_strcasecmp(pucItemName, pucSrchParserSortFieldName) == 0 ) {
                    uiItemID = uiI;
                    break;
                }
            }
            /* We failed to get the item name, either there are no more items to get
            ** or this is an error, either way, we cant go on so we break
            */
            else {
                break;
            }
        }
    }



    /* Initialize the number of short results, now we really count them */
    uiSrchShortResultsLength = 0;
    uiTotalResults = 0;


#if defined(SRCH_SEARCH_ENABLE_DOCUMENT_WEIGHT_NORMALIZATION_AVERAGE)
    fAverageDocumentTermCount = ((float)psiSrchIndex->ullTotalTermCount / psiSrchIndex->uiDocumentCount);
#endif /* defined(SRCH_SEARCH_ENABLE_DOCUMENT_WEIGHT_NORMALIZATION_AVERAGE) */


    /* Populate the short results array - got postings, and we might also have gotten weights, 
    ** and if we did we can only return documents that are in the postings list, all the weights 
    ** would do is adjust the weights from the postings 
    */
    if ( psplSrchPostingsList != NULL ) {

        float                       fWeight = 0;

        unsigned long               ulDocumentAnsiDate = 0;
        unsigned int                uiDocumentTermCount = 0;
        unsigned int                uiDocumentRank = 0;
        unsigned int                uiDocumentLanguageID = LNG_LANGUAGE_ANY_ID;
        struct srchDocumentItem     *psdiSrchDocumentItems = NULL;
        unsigned int                uiSrchDocumentItemsLength = 0;
        boolean                     bDocumentLanguageMatch = false;

        /* Set up the return pointers */
        unsigned int                *puiDocumentRank = (uiSortFieldType == SRCH_SEARCH_SORT_FIELD_TYPE_RANK) ? &uiDocumentRank : NULL;
        unsigned long               *pulDocumentAnsiDate = ((pspnSrchParserNumberDates != NULL) || (uiSortFieldType == SRCH_SEARCH_SORT_FIELD_TYPE_DATE)) ? &ulDocumentAnsiDate : NULL;
        unsigned int                *puiDocumentLanguageID = (pspnSrchParserNumberLanguageIDs != NULL) ? &uiDocumentLanguageID : NULL;
        struct srchDocumentItem     **ppsdiSrchDocumentItems = ((uiSortFieldType == SRCH_SEARCH_SORT_FIELD_TYPE_ITEM_FLOAT) || 
                                            (uiSortFieldType == SRCH_SEARCH_SORT_FIELD_TYPE_ITEM_UINT) || 
                                            (uiSortFieldType == SRCH_SEARCH_SORT_FIELD_TYPE_ITEM_ULONG) || 
                                            (uiSortFieldType == SRCH_SEARCH_SORT_FIELD_TYPE_ITEM_FLOAT) || 
                                            (uiSortFieldType == SRCH_SEARCH_SORT_FIELD_TYPE_ITEM_DOUBLE) || 
                                            (uiSortFieldType == SRCH_SEARCH_SORT_FIELD_TYPE_ITEM_UCHAR)) ? &psdiSrchDocumentItems : NULL;
        unsigned int                *puiSrchDocumentItemsLength = ((uiSortFieldType == SRCH_SEARCH_SORT_FIELD_TYPE_ITEM_FLOAT) || 
                                            (uiSortFieldType == SRCH_SEARCH_SORT_FIELD_TYPE_ITEM_UINT) || 
                                            (uiSortFieldType == SRCH_SEARCH_SORT_FIELD_TYPE_ITEM_ULONG) || 
                                            (uiSortFieldType == SRCH_SEARCH_SORT_FIELD_TYPE_ITEM_FLOAT) || 
                                            (uiSortFieldType == SRCH_SEARCH_SORT_FIELD_TYPE_ITEM_DOUBLE) || 
                                            (uiSortFieldType == SRCH_SEARCH_SORT_FIELD_TYPE_ITEM_UCHAR)) ? &uiSrchDocumentItemsLength : NULL;

        /* Dereference the exclusion and inclusion bitmap array */
        unsigned char               *pucExclusionBitmapPtr = (psbSrchBitmapExclusion != NULL) ? psbSrchBitmapExclusion->pucBitmap : NULL;
        unsigned char               *pucInclusionBitmapPtr = (psbSrchBitmapInclusion != NULL) ? psbSrchBitmapInclusion->pucBitmap : NULL;


        /* Initialize our variables */
        pssrSrchShortResultsPtr = pssrSrchShortResults;
        pspSrchPostingsPtr = psplSrchPostingsList->pspSrchPostings;
        pspSrchPostingsEnd = psplSrchPostingsList->pspSrchPostings + psplSrchPostingsList->uiSrchPostingsLength;


        /* Loop while there are postings left */
        while ( pspSrchPostingsPtr < pspSrchPostingsEnd ) {

            /* Skip this entry if there is an exclusion bitmap array and this document is in there */
            if ( pucExclusionBitmapPtr != NULL ) {

                /* Skip this document if the bit is set */
                if ( UTL_BITMAP_IS_BIT_SET_IN_POINTER(pucExclusionBitmapPtr, pspSrchPostingsPtr->uiDocumentID) ) {

                    unsigned int uiExcludedDocumentID = pspSrchPostingsPtr->uiDocumentID;

                    /* Loop while the document ID is the same and there are postings left */
                    while ( (pspSrchPostingsPtr < pspSrchPostingsEnd) && (pspSrchPostingsPtr->uiDocumentID == uiExcludedDocumentID) ) {
                        pspSrchPostingsPtr++;
                    }
                
                    uiExcludedDocumentCount++;
                    continue;
                }
            }


            /* Skip this entry if there is an inclusion bitmap array and this document is *not* in there */
            if ( pucInclusionBitmapPtr != NULL ) {

                /* Skip this document if the bit is *not* set */
                if ( !UTL_BITMAP_IS_BIT_SET_IN_POINTER(pucInclusionBitmapPtr, pspSrchPostingsPtr->uiDocumentID) ) {

                    unsigned int uiIncludedDocumentID = pspSrchPostingsPtr->uiDocumentID;

                    /* Loop while the document ID is the same and there are postings left */
                    while ( (pspSrchPostingsPtr < pspSrchPostingsEnd) && (pspSrchPostingsPtr->uiDocumentID == uiIncludedDocumentID) ) {
                        pspSrchPostingsPtr++;
                    }
                
                    continue;
                }

                uiIncludedDocumentCount++;
            }


            /* Create a new short result */
            pssrSrchShortResultsPtr->uiDocumentID = pspSrchPostingsPtr->uiDocumentID;
            pssrSrchShortResultsPtr->psiSrchIndexPtr = psiSrchIndex;

            /* Initialize the weight */
            fWeight = 0;


            /* Loop while the document ID is the same and there are postings left */
            while ( (pspSrchPostingsPtr < pspSrchPostingsEnd) && (pssrSrchShortResultsPtr->uiDocumentID == pspSrchPostingsPtr->uiDocumentID) ) {

                /* Increment the weight of the current short result */
                fWeight += pspSrchPostingsPtr->fWeight;

                /* Increment posting */
                pspSrchPostingsPtr++;
            }


            /* Increment the weight if we have a weight array and the weight is set for this document */
            if ( (pswSrchWeight != NULL) && (pswSrchWeight->pfWeights[pssrSrchShortResultsPtr->uiDocumentID] != 0) ) {

                /* Increment the weight, note that the weight could be negative */
                fWeight += pswSrchWeight->pfWeights[pssrSrchShortResultsPtr->uiDocumentID];

                /* Apply minimal document weighting if the weight decremented below 0 */
                if ( fWeight < 0 ) {
                    fWeight = SRCH_SEARCH_DOCUMENT_WEIGHT_MINIMAL;
                }
            }


            /* Get the document information for this short result */
            if ( (iError = iSrchDocumentGetDocumentInfo(psiSrchIndex, pssrSrchShortResultsPtr->uiDocumentID, NULL, NULL, puiDocumentRank, 
                    &uiDocumentTermCount, pulDocumentAnsiDate, puiDocumentLanguageID, ppsdiSrchDocumentItems, puiSrchDocumentItemsLength, 
                    uiItemID, false, false, false)) != SRCH_NoError ) {

                iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the document information for document ID: %u, index: '%s', srch error: %d.", 
                        pssrSrchShortResultsPtr->uiDocumentID, psiSrchIndex->pucIndexName, iError);
                goto bailFromiSrchSearchGetShortResultsFromRawResults;
            }


            /* Filter based on dates */
            if ( pspnSrchParserNumberDates != NULL ) {
                if ( iSrchSearchFilterNumberAgainstSrchParserNumbers(ulDocumentAnsiDate, pspnSrchParserNumberDates, uiSrchParserNumberDatesLength) != SRCH_NoError ) {
                    uiFailedDateMatchCount++;
                    continue;
                }
            }


            /* Filter based on language */
            if ( pspnSrchParserNumberLanguageIDs != NULL ) {
                
                /* Loop over the languages */
                for ( uiI = 0, bDocumentLanguageMatch = false, pspnSrchParserNumberLanguageIDsPtr = pspnSrchParserNumberLanguageIDs; 
                        uiI < uiSrchParserNumberLanguageIDsLength; uiI++, pspnSrchParserNumberLanguageIDsPtr++ ) {
                
                    /* Canonicalize the document language ID if the parser language ID is canonical */
                    if ( (boolean)pspnSrchParserNumberLanguageIDsPtr->uiRangeID == true ) {
                        if ( (iError = iLngGetCanonicalLanguageID(uiDocumentLanguageID, &uiDocumentLanguageID)) != UTL_NoError ) {
                            iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the canonical language ID for the language ID: %u, lng error: %d.", uiDocumentLanguageID, iError); 
                            uiFailedLanguageMatchCount++;
                            continue;
                        }
                    }
                    
                    /* Break here if the languages match */
                    if ( pspnSrchParserNumberLanguageIDsPtr->ulNumber == uiDocumentLanguageID ) {
                        bDocumentLanguageMatch = true;
                        break;
                    }
                }
                
                /* Skip this document if the languages match did not match */
                if ( bDocumentLanguageMatch == false ) {
                    uiFailedLanguageMatchCount++;
                    continue;
                }
            }
                
                
#if defined(SRCH_SEARCH_ENABLE_DOCUMENT_WEIGHT_NORMALIZATION_RAW) || defined(SRCH_SEARCH_ENABLE_DOCUMENT_WEIGHT_NORMALIZATION_AVERAGE) || \
        defined(SRCH_SEARCH_ENABLE_DOCUMENT_WEIGHT_NORMALIZATION_LOG) || defined(SRCH_SEARCH_ENABLE_DOCUMENT_WEIGHT_NORMALIZATION_LOG2)

            /* Adjust the weight based on the stated term count */
            if ( uiDocumentTermCount > 0 ) {
                
#if defined(SRCH_SEARCH_ENABLE_DOCUMENT_WEIGHT_NORMALIZATION_RAW)
                /* Division by term count */
                fWeight /= uiDocumentTermCount;
#endif /* defined(SRCH_SEARCH_ENABLE_DOCUMENT_WEIGHT_NORMALIZATION_RAW) */

#if defined(SRCH_SEARCH_ENABLE_DOCUMENT_WEIGHT_NORMALIZATION_AVERAGE)
                /* Division by the normalized term count (term count / average term count) */
/*                 iUtlLogDebug(UTL_LOG_CONTEXT, "%8d -- %7.3f -> %7.3f (%8d, %7.3f)", pssrSrchShortResultsPtr->uiDocumentID, fWeight,  */
/*                         fWeight / ((float)uiDocumentTermCount / fAverageDocumentTermCount), uiDocumentTermCount,  */
/*                         ((float)uiDocumentTermCount / fAverageDocumentTermCount)); */
                fWeight /= ((float)uiDocumentTermCount / fAverageDocumentTermCount);
#endif /* defined(SRCH_SEARCH_ENABLE_DOCUMENT_WEIGHT_NORMALIZATION_AVERAGE) */

#if defined(SRCH_SEARCH_ENABLE_DOCUMENT_WEIGHT_NORMALIZATION_LOG)
                /* Division by the log of term count */
                fWeight /= log(uiDocumentTermCount + 1);
#endif /* defined(SRCH_SEARCH_ENABLE_DOCUMENT_WEIGHT_NORMALIZATION_LOG) */

#if defined(SRCH_SEARCH_ENABLE_DOCUMENT_WEIGHT_NORMALIZATION_LOG2)
                /* Division by the log2 of term count */
/*                 iUtlLogDebug(UTL_LOG_CONTEXT, "%8d -- %7.3f -> %7.3f (%8d, %7.3f)", pssrSrchShortResultsPtr->uiDocumentID, fWeight, 
                        fWeight / log2(uiDocumentTermCount + 1), uiDocumentTermCount, log2(uiDocumentTermCount)); */
                fWeight /= log2(uiDocumentTermCount);
#endif /* defined(SRCH_SEARCH_ENABLE_DOCUMENT_WEIGHT_NORMALIZATION_LOG2) */

            }

#endif    /* defined(SRCH_SEARCH_ENABLE_DOCUMENT_WEIGHT_NORMALIZATION_RAW) || defined(SRCH_SEARCH_ENABLE_DOCUMENT_WEIGHT_NORMALIZATION_AVERAGE) ||
        defined(SRCH_SEARCH_ENABLE_DOCUMENT_WEIGHT_NORMALIZATION_LOG) || defined(SRCH_SEARCH_ENABLE_DOCUMENT_WEIGHT_NORMALIZATION_LOG2) */


            /* Set the sort key and the max sort key */
            switch ( uiSortFieldType ) {
            
                case SRCH_SEARCH_SORT_FIELD_TYPE_RELEVANCE:
                    pssrSrchShortResultsPtr->fSortKey = fWeight;
                    dMaxSortKey = UTL_MACROS_MAX(dMaxSortKey, pssrSrchShortResultsPtr->fSortKey);
                    break;
                
                case SRCH_SEARCH_SORT_FIELD_TYPE_RANK:
                    pssrSrchShortResultsPtr->uiSortKey = uiDocumentRank;
                    dMaxSortKey = UTL_MACROS_MAX(dMaxSortKey, pssrSrchShortResultsPtr->uiSortKey);
                    break;
            
                case SRCH_SEARCH_SORT_FIELD_TYPE_DATE:
                    pssrSrchShortResultsPtr->ulSortKey = ulDocumentAnsiDate;
                    dMaxSortKey = UTL_MACROS_MAX(dMaxSortKey, pssrSrchShortResultsPtr->ulSortKey);
                    break;
            
                case SRCH_SEARCH_SORT_FIELD_TYPE_ITEM_UINT:
                    pssrSrchShortResultsPtr->uiSortKey = (psdiSrchDocumentItems->pvData != NULL) ? s_strtol(psdiSrchDocumentItems->pvData, NULL, 10) : 0;
                    dMaxSortKey = UTL_MACROS_MAX(dMaxSortKey, pssrSrchShortResultsPtr->uiSortKey);
                    s_free(psdiSrchDocumentItems->pvData);
                    s_free(psdiSrchDocumentItems);
                    break;
            
                case SRCH_SEARCH_SORT_FIELD_TYPE_ITEM_ULONG:
                    pssrSrchShortResultsPtr->ulSortKey = (psdiSrchDocumentItems->pvData != NULL) ? s_strtol(psdiSrchDocumentItems->pvData, NULL, 10) : 0;
                    dMaxSortKey = UTL_MACROS_MAX(dMaxSortKey, pssrSrchShortResultsPtr->ulSortKey);
                    s_free(psdiSrchDocumentItems->pvData);
                    s_free(psdiSrchDocumentItems);
                    break;
            
                case SRCH_SEARCH_SORT_FIELD_TYPE_ITEM_FLOAT:
                    pssrSrchShortResultsPtr->fSortKey = (psdiSrchDocumentItems->pvData != NULL) ? s_strtof(psdiSrchDocumentItems->pvData, NULL) : 0;
                    dMaxSortKey = UTL_MACROS_MAX(dMaxSortKey, pssrSrchShortResultsPtr->fSortKey);
                    s_free(psdiSrchDocumentItems->pvData);
                    s_free(psdiSrchDocumentItems);
                    break;
                
                case SRCH_SEARCH_SORT_FIELD_TYPE_ITEM_DOUBLE:
                    pssrSrchShortResultsPtr->dSortKey = (psdiSrchDocumentItems->pvData != NULL) ? s_strtod(psdiSrchDocumentItems->pvData, NULL) : 0;
                    dMaxSortKey = UTL_MACROS_MAX(dMaxSortKey, pssrSrchShortResultsPtr->dSortKey);
                    s_free(psdiSrchDocumentItems->pvData);
                    s_free(psdiSrchDocumentItems);
                    break;
                
                case SRCH_SEARCH_SORT_FIELD_TYPE_ITEM_UCHAR:
                    pssrSrchShortResultsPtr->pucSortKey = (psdiSrchDocumentItems->pvData != NULL) ? (unsigned char *)psdiSrchDocumentItems->pvData : NULL;
                    s_free(psdiSrchDocumentItems);
                    break;

                case SRCH_SEARCH_SORT_FIELD_TYPE_NO_SORT:
                    break;
            
                default:
                    ASSERT(false);
            }


            /* Increment the short results length */
            uiSrchShortResultsLength++;

            /* Increment the short result pointer */
            pssrSrchShortResultsPtr++;
        }
    }
    /* Populate the short results array -     got weights */
    else if ( pswSrchWeight != NULL ) {

        float                       fWeight = 0;

        unsigned long               ulDocumentAnsiDate = 0;
        unsigned int                uiDocumentTermCount = 0;
        unsigned int                uiDocumentRank = 0;
        unsigned int                uiDocumentLanguageID = LNG_LANGUAGE_ANY_ID;
        struct srchDocumentItem     *psdiSrchDocumentItems = NULL;
        unsigned int                uiSrchDocumentItemsLength = 0;
        boolean                     bDocumentLanguageMatch = false;

        /* Set up the return pointers */
        unsigned int                *puiDocumentRank = (uiSortFieldType == SRCH_SEARCH_SORT_FIELD_TYPE_RANK) ? &uiDocumentRank : NULL;
        unsigned long               *pulDocumentAnsiDate = ((pspnSrchParserNumberDates != NULL) || (uiSortFieldType == SRCH_SEARCH_SORT_FIELD_TYPE_DATE)) ? &ulDocumentAnsiDate : NULL;
        unsigned int                *puiDocumentLanguageID = (pspnSrchParserNumberLanguageIDs != NULL) ? &uiDocumentLanguageID : NULL;
        struct srchDocumentItem     **ppsdiSrchDocumentItems = ((uiSortFieldType == SRCH_SEARCH_SORT_FIELD_TYPE_ITEM_FLOAT) || 
                                            (uiSortFieldType == SRCH_SEARCH_SORT_FIELD_TYPE_ITEM_UINT) || 
                                            (uiSortFieldType == SRCH_SEARCH_SORT_FIELD_TYPE_ITEM_ULONG) || 
                                            (uiSortFieldType == SRCH_SEARCH_SORT_FIELD_TYPE_ITEM_FLOAT) || 
                                            (uiSortFieldType == SRCH_SEARCH_SORT_FIELD_TYPE_ITEM_DOUBLE) || 
                                            (uiSortFieldType == SRCH_SEARCH_SORT_FIELD_TYPE_ITEM_UCHAR)) ? &psdiSrchDocumentItems : NULL;
        unsigned int                *puiSrchDocumentItemsLength = ((uiSortFieldType == SRCH_SEARCH_SORT_FIELD_TYPE_ITEM_FLOAT) || 
                                            (uiSortFieldType == SRCH_SEARCH_SORT_FIELD_TYPE_ITEM_UINT) || 
                                            (uiSortFieldType == SRCH_SEARCH_SORT_FIELD_TYPE_ITEM_ULONG) || 
                                            (uiSortFieldType == SRCH_SEARCH_SORT_FIELD_TYPE_ITEM_FLOAT) || 
                                            (uiSortFieldType == SRCH_SEARCH_SORT_FIELD_TYPE_ITEM_DOUBLE) || 
                                            (uiSortFieldType == SRCH_SEARCH_SORT_FIELD_TYPE_ITEM_UCHAR)) ? &uiSrchDocumentItemsLength : NULL;

        /* Dereference the exclusion and inclusion bitmap array */
        unsigned char               *pucExclusionBitmapPtr = (psbSrchBitmapExclusion != NULL) ? psbSrchBitmapExclusion->pucBitmap : NULL;
        unsigned char               *pucInclusionBitmapPtr = (psbSrchBitmapInclusion != NULL) ? psbSrchBitmapInclusion->pucBitmap : NULL;


        /* Loop over each entry in the weights array, documentID 0 does not exist, hence the '+ 1' */
        for ( uiDocumentID = 1, pssrSrchShortResultsPtr = pssrSrchShortResults, pfWeightsPtr = pswSrchWeight->pfWeights + 1, pfWeightsEnd = pswSrchWeight->pfWeights + pswSrchWeight->uiWeightsLength; 
                pfWeightsPtr < pfWeightsEnd; uiDocumentID++, pfWeightsPtr++ ) {

            /* Skip this entry if there is no weight or if the weight is negative */
            if ( *pfWeightsPtr <= 0 ) {
                continue;
            }


            /* Skip this entry if there is an exclusion bitmap array and this document is in there */
            if ( pucExclusionBitmapPtr != NULL ) {

                /* Skip this document if the bit is set */
                if ( UTL_BITMAP_IS_BIT_SET_IN_POINTER(pucExclusionBitmapPtr, uiDocumentID) ) {
                    uiExcludedDocumentCount++;
                    continue;
                }
            }

            /* Skip this entry if there is an inclusion bitmap array and this document is *not* in there */
            if ( pucInclusionBitmapPtr != NULL ) {

                /* Skip this document if the bit is *not* set */
                if ( !UTL_BITMAP_IS_BIT_SET_IN_POINTER(pucInclusionBitmapPtr, uiDocumentID) ) {
                    continue;
                }

                uiIncludedDocumentCount++;
            }


            /* Create a new short result */
            pssrSrchShortResultsPtr->uiDocumentID = uiDocumentID;
            pssrSrchShortResultsPtr->psiSrchIndexPtr = psiSrchIndex;

            /* Initialize the weight */
            fWeight = *pfWeightsPtr;


            /* Get the document information for this short result */
            if ( (iError = iSrchDocumentGetDocumentInfo(psiSrchIndex, pssrSrchShortResultsPtr->uiDocumentID, NULL, NULL, puiDocumentRank, 
                    &uiDocumentTermCount, pulDocumentAnsiDate, puiDocumentLanguageID, ppsdiSrchDocumentItems, puiSrchDocumentItemsLength, 
                    uiItemID, false, false, false)) != SRCH_NoError ) {

                iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the document information for document ID: %u, index: '%s', srch error: %d.", 
                        pssrSrchShortResultsPtr->uiDocumentID, psiSrchIndex->pucIndexName, iError);
                goto bailFromiSrchSearchGetShortResultsFromRawResults;
            }


            /* Filter based on dates */
            if ( pspnSrchParserNumberDates != NULL ) {
                if ( iSrchSearchFilterNumberAgainstSrchParserNumbers(ulDocumentAnsiDate, pspnSrchParserNumberDates, uiSrchParserNumberDatesLength) != SRCH_NoError ) {
                    uiFailedDateMatchCount++;
                    continue;
                }
            }


            /* Filter based on language */
            if ( pspnSrchParserNumberLanguageIDs != NULL ) {
                
                /* Loop over the languages */
                for ( uiI = 0, bDocumentLanguageMatch = false, pspnSrchParserNumberLanguageIDsPtr = pspnSrchParserNumberLanguageIDs; 
                        uiI < uiSrchParserNumberLanguageIDsLength; uiI++, pspnSrchParserNumberLanguageIDsPtr++ ) {
                
                    /* Canonicalize the document language ID if the parser language ID is canonical */
                    if ( (boolean)pspnSrchParserNumberLanguageIDsPtr->uiRangeID == true ) {
                        if ( (iError = iLngGetCanonicalLanguageID(uiDocumentLanguageID, &uiDocumentLanguageID)) != UTL_NoError ) {
                            iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the canonical language ID for the language ID: %u, lng error: %d.", uiDocumentLanguageID, iError); 
                            uiFailedLanguageMatchCount++;
                            continue;
                        }
                    }
                    
                    /* Break here if the languages match */
                    if ( pspnSrchParserNumberLanguageIDsPtr->ulNumber == uiDocumentLanguageID ) {
                        bDocumentLanguageMatch = true;
                        break;
                    }
                }
                
                /* Skip this document if the languages match did not match */
                if ( bDocumentLanguageMatch == false ) {
                    uiFailedLanguageMatchCount++;
                    continue;
                }
            }


#if defined(SRCH_SEARCH_ENABLE_DOCUMENT_WEIGHT_NORMALIZATION_RAW) || defined(SRCH_SEARCH_ENABLE_DOCUMENT_WEIGHT_NORMALIZATION_AVERAGE) || \
        defined(SRCH_SEARCH_ENABLE_DOCUMENT_WEIGHT_NORMALIZATION_LOG) || defined(SRCH_SEARCH_ENABLE_DOCUMENT_WEIGHT_NORMALIZATION_LOG2)

            /* Adjust the weight based on the stated term count */
            if ( uiDocumentTermCount > 0 ) {
                
#if defined(SRCH_SEARCH_ENABLE_DOCUMENT_WEIGHT_NORMALIZATION_RAW)
                /* Division by term count */
                fWeight /= uiDocumentTermCount;
#endif /* defined(SRCH_SEARCH_ENABLE_DOCUMENT_WEIGHT_NORMALIZATION_RAW) */

#if defined(SRCH_SEARCH_ENABLE_DOCUMENT_WEIGHT_NORMALIZATION_AVERAGE)
                /* Division by the normalized term count (term count / average term count) */
/*                 iUtlLogDebug(UTL_LOG_CONTEXT, "%8d -- %7.3f -> %7.3f (%8d, %7.3f)", pssrSrchShortResultsPtr->uiDocumentID, fWeight,  */
/*                         fWeight / ((float)uiDocumentTermCount / fAverageDocumentTermCount), uiDocumentTermCount,  */
/*                         ((float)uiDocumentTermCount / fAverageDocumentTermCount)); */
                fWeight /= ((float)uiDocumentTermCount / fAverageDocumentTermCount);
#endif /* defined(SRCH_SEARCH_ENABLE_DOCUMENT_WEIGHT_NORMALIZATION_AVERAGE) */

#if defined(SRCH_SEARCH_ENABLE_DOCUMENT_WEIGHT_NORMALIZATION_LOG)
                /* Division by the log of term count */
                fWeight /= log(uiDocumentTermCount + 1);
#endif /* defined(SRCH_SEARCH_ENABLE_DOCUMENT_WEIGHT_NORMALIZATION_LOG) */

#if defined(SRCH_SEARCH_ENABLE_DOCUMENT_WEIGHT_NORMALIZATION_LOG2)
                /* Division by the log2 of term count */
/*                 iUtlLogDebug(UTL_LOG_CONTEXT, "%8d -- %7.3f -> %7.3f (%8d, %7.3f)", pssrSrchShortResultsPtr->uiDocumentID, fWeight, 
                        fWeight / log2(uiDocumentTermCount + 1), uiDocumentTermCount, log2(uiDocumentTermCount)); */
                fWeight /= log2(uiDocumentTermCount);
#endif /* defined(SRCH_SEARCH_ENABLE_DOCUMENT_WEIGHT_NORMALIZATION_LOG2) */

            }

#endif    /* defined(SRCH_SEARCH_ENABLE_DOCUMENT_WEIGHT_NORMALIZATION_RAW) || defined(SRCH_SEARCH_ENABLE_DOCUMENT_WEIGHT_NORMALIZATION_AVERAGE) ||
        defined(SRCH_SEARCH_ENABLE_DOCUMENT_WEIGHT_NORMALIZATION_LOG) || defined(SRCH_SEARCH_ENABLE_DOCUMENT_WEIGHT_NORMALIZATION_LOG2) */


            /* Set the sort key and the max sort key */
            switch ( uiSortFieldType ) {
            
                case SRCH_SEARCH_SORT_FIELD_TYPE_RELEVANCE:
                    pssrSrchShortResultsPtr->fSortKey = fWeight;
                    dMaxSortKey = UTL_MACROS_MAX(dMaxSortKey, pssrSrchShortResultsPtr->fSortKey);
                    break;
                
                case SRCH_SEARCH_SORT_FIELD_TYPE_RANK:
                    pssrSrchShortResultsPtr->uiSortKey = uiDocumentRank;
                    dMaxSortKey = UTL_MACROS_MAX(dMaxSortKey, pssrSrchShortResultsPtr->uiSortKey);
                    break;
            
                case SRCH_SEARCH_SORT_FIELD_TYPE_DATE:
                    pssrSrchShortResultsPtr->ulSortKey = ulDocumentAnsiDate;
                    dMaxSortKey = UTL_MACROS_MAX(dMaxSortKey, pssrSrchShortResultsPtr->ulSortKey);
                    break;
            
                case SRCH_SEARCH_SORT_FIELD_TYPE_ITEM_UINT:
                    pssrSrchShortResultsPtr->uiSortKey = (psdiSrchDocumentItems->pvData != NULL) ? s_strtol(psdiSrchDocumentItems->pvData, NULL, 10) : 0;
                    dMaxSortKey = UTL_MACROS_MAX(dMaxSortKey, pssrSrchShortResultsPtr->uiSortKey);
                    s_free(psdiSrchDocumentItems->pvData);
                    s_free(psdiSrchDocumentItems);
                    break;
            
                case SRCH_SEARCH_SORT_FIELD_TYPE_ITEM_ULONG:
                    pssrSrchShortResultsPtr->ulSortKey = (psdiSrchDocumentItems->pvData != NULL) ? s_strtol(psdiSrchDocumentItems->pvData, NULL, 10) : 0;
                    dMaxSortKey = UTL_MACROS_MAX(dMaxSortKey, pssrSrchShortResultsPtr->ulSortKey);
                    s_free(psdiSrchDocumentItems->pvData);
                    s_free(psdiSrchDocumentItems);
                    break;
            
                case SRCH_SEARCH_SORT_FIELD_TYPE_ITEM_FLOAT:
                    pssrSrchShortResultsPtr->fSortKey = (psdiSrchDocumentItems->pvData != NULL) ? s_strtof(psdiSrchDocumentItems->pvData, NULL) : 0;
                    dMaxSortKey = UTL_MACROS_MAX(dMaxSortKey, pssrSrchShortResultsPtr->fSortKey);
                    s_free(psdiSrchDocumentItems->pvData);
                    s_free(psdiSrchDocumentItems);
                    break;
                
                case SRCH_SEARCH_SORT_FIELD_TYPE_ITEM_DOUBLE:
                    pssrSrchShortResultsPtr->dSortKey = (psdiSrchDocumentItems->pvData != NULL) ? s_strtod(psdiSrchDocumentItems->pvData, NULL) : 0;
                    dMaxSortKey = UTL_MACROS_MAX(dMaxSortKey, pssrSrchShortResultsPtr->dSortKey);
                    s_free(psdiSrchDocumentItems->pvData);
                    s_free(psdiSrchDocumentItems);
                    break;
                
                case SRCH_SEARCH_SORT_FIELD_TYPE_ITEM_UCHAR:
                    pssrSrchShortResultsPtr->pucSortKey = (psdiSrchDocumentItems->pvData != NULL) ? (unsigned char *)psdiSrchDocumentItems->pvData : NULL;
                    s_free(psdiSrchDocumentItems);
                    break;

                case SRCH_SEARCH_SORT_FIELD_TYPE_NO_SORT:
                    break;
            
                default:
                    ASSERT(false);
            }


            /* Increment the short results length */
            uiSrchShortResultsLength++;

            /* Increment the short result pointer */
            pssrSrchShortResultsPtr++;
        }
    }
    /* Populate the short results array - got document table */
    else if ( bDocumentTable == true ) {

        float                       fWeight = 0;

        unsigned long               ulDocumentAnsiDate = 0;
        unsigned int                uiDocumentTermCount = 0;
        unsigned int                uiDocumentRank = 0;
        unsigned int                uiDocumentLanguageID = LNG_LANGUAGE_ANY_ID;
        struct srchDocumentItem     *psdiSrchDocumentItems = NULL;
        unsigned int                uiSrchDocumentItemsLength = 0;
        boolean                     bDocumentLanguageMatch = false;

        /* Set up the return pointers */
        unsigned int                *puiDocumentRank = (uiSortFieldType == SRCH_SEARCH_SORT_FIELD_TYPE_RANK) ? &uiDocumentRank : NULL;
        unsigned long               *pulDocumentAnsiDate = ((pspnSrchParserNumberDates != NULL) || (uiSortFieldType == SRCH_SEARCH_SORT_FIELD_TYPE_DATE)) ? &ulDocumentAnsiDate : NULL;
        unsigned int                *puiDocumentLanguageID = (pspnSrchParserNumberLanguageIDs != NULL) ? &uiDocumentLanguageID : NULL;
        struct srchDocumentItem     **ppsdiSrchDocumentItems = ((uiSortFieldType == SRCH_SEARCH_SORT_FIELD_TYPE_ITEM_FLOAT) || 
                                            (uiSortFieldType == SRCH_SEARCH_SORT_FIELD_TYPE_ITEM_UINT) || 
                                            (uiSortFieldType == SRCH_SEARCH_SORT_FIELD_TYPE_ITEM_ULONG) || 
                                            (uiSortFieldType == SRCH_SEARCH_SORT_FIELD_TYPE_ITEM_FLOAT) || 
                                            (uiSortFieldType == SRCH_SEARCH_SORT_FIELD_TYPE_ITEM_DOUBLE) || 
                                            (uiSortFieldType == SRCH_SEARCH_SORT_FIELD_TYPE_ITEM_UCHAR)) ? &psdiSrchDocumentItems : NULL;
        unsigned int                *puiSrchDocumentItemsLength = ((uiSortFieldType == SRCH_SEARCH_SORT_FIELD_TYPE_ITEM_FLOAT) || 
                                            (uiSortFieldType == SRCH_SEARCH_SORT_FIELD_TYPE_ITEM_UINT) || 
                                            (uiSortFieldType == SRCH_SEARCH_SORT_FIELD_TYPE_ITEM_ULONG) || 
                                            (uiSortFieldType == SRCH_SEARCH_SORT_FIELD_TYPE_ITEM_FLOAT) || 
                                            (uiSortFieldType == SRCH_SEARCH_SORT_FIELD_TYPE_ITEM_DOUBLE) || 
                                            (uiSortFieldType == SRCH_SEARCH_SORT_FIELD_TYPE_ITEM_UCHAR)) ? &uiSrchDocumentItemsLength : NULL;

        /* Dereference the exclusion and inclusion bitmap array */
        unsigned char               *pucExclusionBitmapPtr = (psbSrchBitmapExclusion != NULL) ? psbSrchBitmapExclusion->pucBitmap : NULL;
        unsigned char               *pucInclusionBitmapPtr = (psbSrchBitmapInclusion != NULL) ? psbSrchBitmapInclusion->pucBitmap : NULL;


        /* Loop over each document ID, documentID 0 does not exist, hence the '+ 1' */
        for ( uiDocumentID = 1, pssrSrchShortResultsPtr = pssrSrchShortResults; uiDocumentID < psiSrchIndex->uiDocumentCount + 1; uiDocumentID++ ) {

            /* Skip this entry if there is an exclusion bitmap array and this document is in there */
            if ( pucExclusionBitmapPtr != NULL ) {

                /* Skip this document if the bit is set */
                if ( UTL_BITMAP_IS_BIT_SET_IN_POINTER(pucExclusionBitmapPtr, uiDocumentID) != 0 ) {
                    uiExcludedDocumentCount++;
                    continue;
                }
            }

            /* Skip this entry if there is an inclusion bitmap array and this document is *not* in there */
            if ( pucInclusionBitmapPtr != NULL ) {
                
                /* Skip this document if the bit is *not* set */
                if ( !UTL_BITMAP_IS_BIT_SET_IN_POINTER(pucInclusionBitmapPtr, uiDocumentID) ) {
                    continue;
                }

                uiIncludedDocumentCount++;
            }


            /* Create a new short result */
            pssrSrchShortResultsPtr->uiDocumentID = uiDocumentID;
            pssrSrchShortResultsPtr->psiSrchIndexPtr = psiSrchIndex;

            /* Initialize the weight */
            fWeight = 1;


            /* Get the document information for this short result */
            if ( (iError = iSrchDocumentGetDocumentInfo(psiSrchIndex, pssrSrchShortResultsPtr->uiDocumentID, NULL, NULL, puiDocumentRank, 
                    &uiDocumentTermCount, pulDocumentAnsiDate, puiDocumentLanguageID, ppsdiSrchDocumentItems, puiSrchDocumentItemsLength, 
                    uiItemID, false, false, false)) != SRCH_NoError ) {

                iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the document information for document ID: %u, index: '%s', srch error: %d.", 
                        pssrSrchShortResultsPtr->uiDocumentID, psiSrchIndex->pucIndexName, iError);
                goto bailFromiSrchSearchGetShortResultsFromRawResults;
            }


            /* Filter based on dates */
            if ( pspnSrchParserNumberDates != NULL ) {
                if ( iSrchSearchFilterNumberAgainstSrchParserNumbers(ulDocumentAnsiDate, pspnSrchParserNumberDates, uiSrchParserNumberDatesLength) != SRCH_NoError ) {
                    uiFailedDateMatchCount++;
                    continue;
                }
            }


            /* Filter based on language */
            if ( pspnSrchParserNumberLanguageIDs != NULL ) {
                
                /* Loop over the languages */
                for ( uiI = 0, bDocumentLanguageMatch = false, pspnSrchParserNumberLanguageIDsPtr = pspnSrchParserNumberLanguageIDs; 
                        uiI < uiSrchParserNumberLanguageIDsLength; uiI++, pspnSrchParserNumberLanguageIDsPtr++ ) {
                
                    /* Canonicalize the document language ID if the parser language ID is canonical */
                    if ( (boolean)pspnSrchParserNumberLanguageIDsPtr->uiRangeID == true ) {
                        if ( (iError = iLngGetCanonicalLanguageID(uiDocumentLanguageID, &uiDocumentLanguageID)) != UTL_NoError ) {
                            iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the canonical language ID for the language ID: %u, lng error: %d.", uiDocumentLanguageID, iError); 
                            uiFailedLanguageMatchCount++;
                            continue;
                        }
                    }
                    
                    /* Break here if the languages match */
                    if ( pspnSrchParserNumberLanguageIDsPtr->ulNumber == uiDocumentLanguageID ) {
                        bDocumentLanguageMatch = true;
                        break;
                    }
                }
                
                /* Skip this document if the languages match did not match */
                if ( bDocumentLanguageMatch == false ) {
                    uiFailedLanguageMatchCount++;
                    continue;
                }
            }


#if defined(SRCH_SEARCH_ENABLE_DOCUMENT_WEIGHT_NORMALIZATION_RAW) || defined(SRCH_SEARCH_ENABLE_DOCUMENT_WEIGHT_NORMALIZATION_AVERAGE) || \
        defined(SRCH_SEARCH_ENABLE_DOCUMENT_WEIGHT_NORMALIZATION_LOG) || defined(SRCH_SEARCH_ENABLE_DOCUMENT_WEIGHT_NORMALIZATION_LOG2)

            /* Adjust the weight based on the stated term count */
            if ( uiDocumentTermCount > 0 ) {
                
#if defined(SRCH_SEARCH_ENABLE_DOCUMENT_WEIGHT_NORMALIZATION_RAW)
                /* Division by term count */
                fWeight /= uiDocumentTermCount;
#endif /* defined(SRCH_SEARCH_ENABLE_DOCUMENT_WEIGHT_NORMALIZATION_RAW) */

#if defined(SRCH_SEARCH_ENABLE_DOCUMENT_WEIGHT_NORMALIZATION_AVERAGE)
                /* Division by the normalized term count (term count / average term count) */
/*                 iUtlLogDebug(UTL_LOG_CONTEXT, "%8d -- %7.3f -> %7.3f (%8d, %7.3f)", pssrSrchShortResultsPtr->uiDocumentID, fWeight,  */
/*                         fWeight / ((float)uiDocumentTermCount / fAverageDocumentTermCount), uiDocumentTermCount,  */
/*                         ((float)uiDocumentTermCount / fAverageDocumentTermCount)); */
                fWeight /= ((float)uiDocumentTermCount / fAverageDocumentTermCount);
#endif /* defined(SRCH_SEARCH_ENABLE_DOCUMENT_WEIGHT_NORMALIZATION_AVERAGE) */

#if defined(SRCH_SEARCH_ENABLE_DOCUMENT_WEIGHT_NORMALIZATION_LOG)
                /* Division by the log of term count */
                fWeight /= log(uiDocumentTermCount + 1);
#endif /* defined(SRCH_SEARCH_ENABLE_DOCUMENT_WEIGHT_NORMALIZATION_LOG) */

#if defined(SRCH_SEARCH_ENABLE_DOCUMENT_WEIGHT_NORMALIZATION_LOG2)
                /* Division by the log2 of term count */
/*                 iUtlLogDebug(UTL_LOG_CONTEXT, "%8d -- %7.3f -> %7.3f (%8d, %7.3f)", pssrSrchShortResultsPtr->uiDocumentID, fWeight, 
                        fWeight / log2(uiDocumentTermCount + 1), uiDocumentTermCount, log2(uiDocumentTermCount)); */
                fWeight /= log2(uiDocumentTermCount);
#endif /* defined(SRCH_SEARCH_ENABLE_DOCUMENT_WEIGHT_NORMALIZATION_LOG2) */

            }

#endif    /* defined(SRCH_SEARCH_ENABLE_DOCUMENT_WEIGHT_NORMALIZATION_RAW) || defined(SRCH_SEARCH_ENABLE_DOCUMENT_WEIGHT_NORMALIZATION_AVERAGE) ||
        defined(SRCH_SEARCH_ENABLE_DOCUMENT_WEIGHT_NORMALIZATION_LOG) || defined(SRCH_SEARCH_ENABLE_DOCUMENT_WEIGHT_NORMALIZATION_LOG2) */


            /* Set the sort key and the max sort key */
            switch ( uiSortFieldType ) {
            
                case SRCH_SEARCH_SORT_FIELD_TYPE_RELEVANCE:
                    pssrSrchShortResultsPtr->fSortKey = fWeight;
                    dMaxSortKey = UTL_MACROS_MAX(dMaxSortKey, pssrSrchShortResultsPtr->fSortKey);
                    break;
                
                case SRCH_SEARCH_SORT_FIELD_TYPE_RANK:
                    pssrSrchShortResultsPtr->uiSortKey = uiDocumentRank;
                    dMaxSortKey = UTL_MACROS_MAX(dMaxSortKey, pssrSrchShortResultsPtr->uiSortKey);
                    break;
            
                case SRCH_SEARCH_SORT_FIELD_TYPE_DATE:
                    pssrSrchShortResultsPtr->ulSortKey = ulDocumentAnsiDate;
                    dMaxSortKey = UTL_MACROS_MAX(dMaxSortKey, pssrSrchShortResultsPtr->ulSortKey);
                    break;
            
                case SRCH_SEARCH_SORT_FIELD_TYPE_ITEM_UINT:
                    pssrSrchShortResultsPtr->uiSortKey = (psdiSrchDocumentItems->pvData != NULL) ? s_strtol(psdiSrchDocumentItems->pvData, NULL, 10) : 0;
                    dMaxSortKey = UTL_MACROS_MAX(dMaxSortKey, pssrSrchShortResultsPtr->uiSortKey);
                    s_free(psdiSrchDocumentItems->pvData);
                    s_free(psdiSrchDocumentItems);
                    break;
            
                case SRCH_SEARCH_SORT_FIELD_TYPE_ITEM_ULONG:
                    pssrSrchShortResultsPtr->ulSortKey = (psdiSrchDocumentItems->pvData != NULL) ? s_strtol(psdiSrchDocumentItems->pvData, NULL, 10) : 0;
                    dMaxSortKey = UTL_MACROS_MAX(dMaxSortKey, pssrSrchShortResultsPtr->ulSortKey);
                    s_free(psdiSrchDocumentItems->pvData);
                    s_free(psdiSrchDocumentItems);
                    break;
            
                case SRCH_SEARCH_SORT_FIELD_TYPE_ITEM_FLOAT:
                    pssrSrchShortResultsPtr->fSortKey = (psdiSrchDocumentItems->pvData != NULL) ? s_strtof(psdiSrchDocumentItems->pvData, NULL) : 0;
                    dMaxSortKey = UTL_MACROS_MAX(dMaxSortKey, pssrSrchShortResultsPtr->fSortKey);
                    s_free(psdiSrchDocumentItems->pvData);
                    s_free(psdiSrchDocumentItems);
                    break;
                
                case SRCH_SEARCH_SORT_FIELD_TYPE_ITEM_DOUBLE:
                    pssrSrchShortResultsPtr->dSortKey = (psdiSrchDocumentItems->pvData != NULL) ? s_strtod(psdiSrchDocumentItems->pvData, NULL) : 0;
                    dMaxSortKey = UTL_MACROS_MAX(dMaxSortKey, pssrSrchShortResultsPtr->dSortKey);
                    s_free(psdiSrchDocumentItems->pvData);
                    s_free(psdiSrchDocumentItems);
                    break;
                
                case SRCH_SEARCH_SORT_FIELD_TYPE_ITEM_UCHAR:
                    pssrSrchShortResultsPtr->pucSortKey = (psdiSrchDocumentItems->pvData != NULL) ? (unsigned char *)psdiSrchDocumentItems->pvData : NULL;
                    s_free(psdiSrchDocumentItems);
                    break;

                case SRCH_SEARCH_SORT_FIELD_TYPE_NO_SORT:
                    break;
            
                default:
                    ASSERT(false);
            }


            /* Increment the short results length */
            uiSrchShortResultsLength++;

            /* Increment the short result pointer */
            pssrSrchShortResultsPtr++;
        }
    }
    else {
        ASSERT(false);
    }


    /* Now that we have an accurate short results length, we set the search results count from it */
    uiTotalResults = uiSrchShortResultsLength;


    /* Reset the error, it will be set if we failed to read document info 
    ** or if there are deleted documents
    */
    iError = SRCH_NoError;


    /* Search report */
    iSrchReportAppend(pssSrchSearch->pvSrchReport, "%s %u %u %u %u %u %u 1\n", REP_RETRIEVAL_COUNTS, uiSrchShortResultsLength, 
            uiFailedDateMatchCount, uiFailedTermCountMatchCount, uiFailedLanguageMatchCount, uiExcludedDocumentCount, uiIncludedDocumentCount); 


    /* Get the parser search results ID */
    if ( (iError = iSrchParserGetModifierID(pssSrchSearch->pvSrchParser, SRCH_PARSER_MODIFIER_SEARCH_RESULTS_ID, &uiSrchParserSearchResultsID)) != SRCH_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the parser search results ID, srch error: %d.", iError);
        goto bailFromiSrchSearchGetShortResultsFromRawResults;
    }

    /* Suppress the search results if requested */
    if ( uiSrchParserSearchResultsID == SRCH_PARSER_MODIFIER_SEARCH_RESULTS_SUPPRESS_ID ) {
        uiSrchShortResultsLength = 0;
        iError = SPI_NoError;
        goto bailFromiSrchSearchGetShortResultsFromRawResults;
    }


    /* Sort if need be */
    if ( uiSrchShortResultsLength > 0  ) {
        
        if ( (iError = iSrchShortResultSort(&pssrSrchShortResults, uiSrchShortResultsLength, uiSortType)) != SRCH_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to sort the short results: srch error: %d.", iError);
            goto bailFromiSrchSearchGetShortResultsFromRawResults;
        }
    }

/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iSrchSearchGetShortResultsFromRawResults - uiSrchShortResultsLength: %u", uiSrchShortResultsLength); */
/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iSrchSearchGetShortResultsFromRawResults - uiFailedDateMatchCount: %u", uiFailedDateMatchCount); */
/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iSrchSearchGetShortResultsFromRawResults - uiFailedTermCountMatchCount: %u", uiFailedTermCountMatchCount); */
/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iSrchSearchGetShortResultsFromRawResults - uiFailedLanguageMatchCount: %u", uiFailedLanguageMatchCount); */
/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iSrchSearchGetShortResultsFromRawResults - uiExcludedDocumentCount: %u", uiExcludedDocumentCount); */
/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iSrchSearchGetShortResultsFromRawResults - uiIncludedDocumentCount: %u", uiIncludedDocumentCount); */



    /* Bail label */
    bailFromiSrchSearchGetShortResultsFromRawResults:


    /* Free the search ansi data array */
    s_free(pulSearchAnsiDate);


    /* Handle the error */
    if ( iError == SRCH_NoError ) {
    
        /* Set the return pointers if we got search short results, otherwise we free them */
        if ( uiSrchShortResultsLength > 0 ) {

            /* Set the return short results pointer and length, and max weight */
            *ppssrSrchShortResults = pssrSrchShortResults;
            *puiSrchShortResultsLength = uiSrchShortResultsLength;
            *puiTotalResults = uiTotalResults;
            *pdMaxSortKey = dMaxSortKey;
        }
        else {

            /* Free the search short results */
            iSrchShortResultFree(pssrSrchShortResults, uiSrchShortResultsLength, uiSortType);
            pssrSrchShortResults = NULL;
        }
    }
    else {

        /* Free the search short results */
        iSrchShortResultFree(pssrSrchShortResults, uiSrchShortResultsLength, uiSortType);
        pssrSrchShortResults = NULL;
    }


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchSearchGetSpiSearchResultsFromShortResults()

    Purpose:    This function populates the passed spi search result structure array using the
                search short results structure array as a base.
                
                This assumes that the spi search result structure array is allocated

    Parameters: pssSrchSearch               search structure
                psiSrchIndex                index structure
                pssrSrchShortResults        pointer to an allocated array of search short results
                uiSrchShortResultsLength    number of entries in the search short results
                pssrSpiSearchResults        pointer to an allocated array of spi search results
                uiSortType                  sort type

    Globals:    none

    Returns:    SRCH/SPI error code

*/
static int iSrchSearchGetSpiSearchResultsFromShortResults
(
    struct srchSearch *pssSrchSearch,
    struct srchIndex *psiSrchIndex,
    struct srchShortResult *pssrSrchShortResults,
    unsigned int uiSrchShortResultsLength,
    struct spiSearchResult *pssrSpiSearchResults,
    unsigned int uiSortType
)
{

    int                         iError = SRCH_NoError;
    unsigned char               pucIndexName[SPI_INDEX_NAME_MAXIMUM_LENGTH + 1] = {'\0'};
    unsigned int                uiI = 0, uiJ = 0;
    struct spiSearchResult      *pssrSpiSearchResultsPtr = NULL;
    struct srchShortResult      *pssrSrchShortResultsPtr = NULL;


    ASSERT(pssSrchSearch != NULL);
    ASSERT(psiSrchIndex != NULL);
    ASSERT(pssrSrchShortResults != NULL);
    ASSERT(uiSrchShortResultsLength >= 0);
    ASSERT(pssrSpiSearchResults != NULL);
    ASSERT(SPI_SORT_TYPE_VALID(uiSortType) == true);


    /* Get the index name */
    if ( (iError = iSrchSearchGetIndexName(pssSrchSearch, psiSrchIndex, pucIndexName, SPI_INDEX_NAME_MAXIMUM_LENGTH + 1)) != SRCH_NoError ) {
        return (iError);
    }
                

    /* Cycle though all the short results array filling in the search result data */    
    for ( uiI = 0, pssrSpiSearchResultsPtr = pssrSpiSearchResults, pssrSrchShortResultsPtr = pssrSrchShortResults; uiI < uiSrchShortResultsLength; 
            uiI++, pssrSpiSearchResultsPtr++, pssrSrchShortResultsPtr++ ) {

        unsigned int                uiDocumentLanguageID = 0;
        unsigned char               pucLanguageCode[SPI_INDEX_LANGUAGE_MAXIMUM_LENGTH + 1];
        struct srchDocumentItem     *psdiSrchDocumentItems = NULL;
        struct srchDocumentItem     *psdiSrchDocumentItemsPtr = NULL;
        unsigned int                uiSrchDocumentItemsLength = 0;
        unsigned char               pucItemName[SPI_ITEM_NAME_MAXIMUM_LENGTH + 1];
        unsigned char               pucMimeType[SPI_MIME_TYPE_MAXIMUM_LENGTH + 1];

    
        /* Process the current index only */
        if ( pssrSrchShortResultsPtr->psiSrchIndexPtr == psiSrchIndex ) {

            /* Read the document information */
            if ( (iError = iSrchDocumentGetDocumentInfo(psiSrchIndex, pssrSrchShortResultsPtr->uiDocumentID, &pssrSpiSearchResultsPtr->pucTitle, 
                    &pssrSpiSearchResultsPtr->pucDocumentKey, &pssrSpiSearchResultsPtr->uiRank, &pssrSpiSearchResultsPtr->uiTermCount,
                    &pssrSpiSearchResultsPtr->ulAnsiDate, &uiDocumentLanguageID, &psdiSrchDocumentItems, &uiSrchDocumentItemsLength, 
                    0, false, true, false)) != SRCH_NoError ) {

                iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the document information for document ID: %u, index: '%s', srch error: %d.", 
                        pssrSrchShortResultsPtr->uiDocumentID, psiSrchIndex->pucIndexName, iError);
                goto bailFromiSrchSearchGetSpiSearchResultsFromShortResults;
            }
    
            /* Set the sort key from the short result */
            switch ( uiSortType ) {
    
                case SPI_SORT_TYPE_DOUBLE_ASC:
                case SPI_SORT_TYPE_DOUBLE_DESC:
                    pssrSpiSearchResultsPtr->dSortKey = pssrSrchShortResultsPtr->dSortKey;
                    break;
    
                case SPI_SORT_TYPE_FLOAT_ASC:
                case SPI_SORT_TYPE_FLOAT_DESC:
                    pssrSpiSearchResultsPtr->fSortKey = pssrSrchShortResultsPtr->fSortKey;
                    break;
                
                case SPI_SORT_TYPE_UINT_ASC:
                case SPI_SORT_TYPE_UINT_DESC:
                    pssrSpiSearchResultsPtr->uiSortKey = pssrSrchShortResultsPtr->uiSortKey;
                    break;
                
                case SPI_SORT_TYPE_ULONG_ASC:
                case SPI_SORT_TYPE_ULONG_DESC:
                    pssrSpiSearchResultsPtr->ulSortKey = pssrSrchShortResultsPtr->ulSortKey;
                    break;
                
                case SPI_SORT_TYPE_UCHAR_ASC:
                case SPI_SORT_TYPE_UCHAR_DESC:
                    pssrSpiSearchResultsPtr->pucSortKey = pssrSrchShortResultsPtr->pucSortKey;
                    break;
                
                case SPI_SORT_TYPE_NO_SORT:
                    break;
    
                case SPI_SORT_TYPE_UNKNOWN:
                    break;
                
                default:
                    ASSERT(false);
                    break;
            }
    
            /* Set the index name */
            if ( (pssrSpiSearchResultsPtr->pucIndexName = (unsigned char *)s_strdup(pucIndexName)) == NULL ) {
                return (SRCH_MemError);
            }
    
            /* Set the language code, if it is valid */
            if ( iLngGetLanguageCodeFromID(uiDocumentLanguageID, pucLanguageCode, SPI_INDEX_LANGUAGE_MAXIMUM_LENGTH + 1) == LNG_NoError ) {
                if ( (pssrSpiSearchResultsPtr->pucLanguageCode = (unsigned char *)s_strdup(pucLanguageCode)) == NULL ) {
                    return (SRCH_MemError);
                }
            }
    
            /* Loop through the document type profile array */
            for ( uiJ = 0, psdiSrchDocumentItemsPtr = psdiSrchDocumentItems; uiJ < uiSrchDocumentItemsLength; uiJ++, psdiSrchDocumentItemsPtr++ ) {
    
                /* Get the item info for this item ID */
                if ( (iError = iSrchInfoGetItemInfo(psiSrchIndex, psdiSrchDocumentItemsPtr->uiItemID, pucItemName, SPI_ITEM_NAME_MAXIMUM_LENGTH + 1, 
                        pucMimeType, SPI_MIME_TYPE_MAXIMUM_LENGTH + 1)) != SRCH_NoError ) {

                    goto bailFromiSrchSearchGetSpiSearchResultsFromShortResults;
                }
    
                /* Add the document item/mime type to the document item structure, tell it to take the data rather than copy it */
                if ( (iError = iSrchSearchAddEntryToSpiDocumentItems(pucItemName, pucMimeType, psdiSrchDocumentItemsPtr->pucUrl, psdiSrchDocumentItemsPtr->uiItemLength, 
                        psdiSrchDocumentItemsPtr->pvData, psdiSrchDocumentItemsPtr->uiDataLength, false, &pssrSpiSearchResultsPtr->psdiSpiDocumentItems, 
                        &pssrSpiSearchResultsPtr->uiDocumentItemsLength)) != SRCH_NoError ) {
                        
                    goto bailFromiSrchSearchGetSpiSearchResultsFromShortResults;
                }
            }
        
            /* Free the type profile array */
            s_free(psdiSrchDocumentItems);
        }
    }


    /* Bail from reading document information label */
    bailFromiSrchSearchGetSpiSearchResultsFromShortResults:


    return (SRCH_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchSearchGetPostingsListFromParserTermCluster()

    Purpose:    This function searches for the term in a term cluster
                and merges the results into a posting array, passing 
                it back via the search postings list structure 
                return pointer.

                If no document were retrieved from this search,
                the search postings list structure return pointer 
                will be NULL.

    Parameters: pssSrchSearch                   search structure
                psiSrchIndex                    index structure
                uiLanguageID                    language ID
                psptcSrchParserTermCluster      search parser term cluster to process
                ppsplSrchPostingsList           return pointer for the search postings list structure

    Globals:    none

    Returns:    SRCH error code

*/
static int iSrchSearchGetPostingsListFromParserTermCluster
(
    struct srchSearch *pssSrchSearch,
    struct srchIndex *psiSrchIndex,
    unsigned int uiLanguageID, 
    struct srchParserTermCluster *psptcSrchParserTermCluster,
    struct srchPostingsList **ppsplSrchPostingsList
)
{

    int                         iError = SRCH_NoError;
    unsigned int                uiSrchParserBooleanOperationID = SRCH_PARSER_INVALID_ID;
    unsigned int                uiSrchPostingBooleanOperationID = SRCH_POSTING_BOOLEAN_OPERATION_INVALID_ID;
    unsigned int                uiI = 0;
    struct srchPostingsList     *psplSrchPostingsList1 = NULL;
    struct srchPostingsList     *psplSrchPostingsList2 = NULL;
    
    /* ADJ Term distance must be pre-set to 1 */
    int                         iTermDistanceADJ = 1;
    
    /* NEAR Term distance must be pre-set to 0 */
    int                         iTermDistanceNEAR = 0;
    
    boolean                     bSearchCompletionLogged = false;


    ASSERT(pssSrchSearch != NULL);
    ASSERT(psiSrchIndex != NULL);
    ASSERT(uiLanguageID >= 0);
    ASSERT(psptcSrchParserTermCluster != NULL);
    ASSERT(ppsplSrchPostingsList != NULL);


    /* Pre-set the return parameter */
    *ppsplSrchPostingsList = NULL;


    /* Get the parser boolean operation ID */
    if ( (iError = iSrchParserGetModifierID(pssSrchSearch->pvSrchParser, SRCH_PARSER_MODIFIER_BOOLEAN_OPERATION_ID, &uiSrchParserBooleanOperationID)) != SRCH_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the parser boolean operation ID, srch error: %d.", iError);
        return (iError);
    }
    
    /* Set the posting boolean operation ID from the parser boolean operation ID */
    if ( uiSrchParserBooleanOperationID == SRCH_PARSER_MODIFIER_BOOLEAN_OPERATION_STRICT_ID ) {
        uiSrchPostingBooleanOperationID = SRCH_POSTING_BOOLEAN_OPERATION_STRICT_ID;
    }
    else if ( uiSrchParserBooleanOperationID == SRCH_PARSER_MODIFIER_BOOLEAN_OPERATION_RELAXED_ID ) {
        uiSrchPostingBooleanOperationID = SRCH_POSTING_BOOLEAN_OPERATION_RELAXED_ID;
    }
    else {
        uiSrchPostingBooleanOperationID = SRCH_POSTING_BOOLEAN_OPERATION_STRICT_ID;
    }
    

    /* Loop over each entry */
    for ( uiI = 0; uiI < psptcSrchParserTermCluster->uiTermsLength; uiI++ ) {


        /* We can skip this term list entry if the (non-stop) term we just searched for has no occurences, 
        ** we are using strict boolean or the term is required, and the operator boils down to an 
        ** intersection of some sort
        */
        if ( (psplSrchPostingsList2 != NULL) && (psplSrchPostingsList2->uiTermType != SPI_TERM_TYPE_STOP) && (psplSrchPostingsList2->uiSrchPostingsLength == 0) && 
                ((uiSrchPostingBooleanOperationID == SRCH_POSTING_BOOLEAN_OPERATION_STRICT_ID) || 
                    ((uiSrchPostingBooleanOperationID == SRCH_POSTING_BOOLEAN_OPERATION_RELAXED_ID) && (psplSrchPostingsList2->bRequired == true))) &&
                ((psptcSrchParserTermCluster->uiOperatorID == SRCH_PARSER_OPERATOR_AND_ID) || (psptcSrchParserTermCluster->uiOperatorID == SRCH_PARSER_OPERATOR_NOT_ID) ||
                (psptcSrchParserTermCluster->uiOperatorID == SRCH_PARSER_OPERATOR_ADJ_ID) || (psptcSrchParserTermCluster->uiOperatorID == SRCH_PARSER_OPERATOR_NEAR_ID)) ) {
        
            /* Log if we have not logged already */
            if ( bSearchCompletionLogged == false ) {
                iSrchReportAppend(pssSrchSearch->pvSrchReport, "%s The search was completed early due to boolean constraints\n", REP_SEARCH_WARNING);
                bSearchCompletionLogged = true;
            }

            /* Skip to the next term list entry */
            continue;
        }


        /* Hand over the second search postings list structure pointer to the first search postings list structure pointer if it is populated */
        if ( psplSrchPostingsList2 != NULL ) {
            ASSERT(psplSrchPostingsList1 == NULL);
            psplSrchPostingsList1 = psplSrchPostingsList2;
            psplSrchPostingsList2 = NULL;
        }


        /* Its a term!! */
        if ( psptcSrchParserTermCluster->puiTermTypeIDs[uiI] == SRCH_PARSER_TERM_TYPE_TERM_ID ) {

            /* Start and end document ID */
            unsigned int uiStartDocumentID = 0;
            unsigned int uiEndDocumentID = 0;

            // Set the start and end document ID if there is a posting list and the operator is an 'and', a 'not', an 'adj' or a 'near' */
            if ( (psplSrchPostingsList1 != NULL) && (psplSrchPostingsList1->pspSrchPostings != NULL) && (psplSrchPostingsList1->uiSrchPostingsLength > 0) &&
                    ((psptcSrchParserTermCluster->uiOperatorID == SRCH_PARSER_OPERATOR_AND_ID) || (psptcSrchParserTermCluster->uiOperatorID == SRCH_PARSER_OPERATOR_NOT_ID) ||
                    (psptcSrchParserTermCluster->uiOperatorID == SRCH_PARSER_OPERATOR_ADJ_ID) || (psptcSrchParserTermCluster->uiOperatorID == SRCH_PARSER_OPERATOR_NEAR_ID)) ) {
                uiStartDocumentID = psplSrchPostingsList1->pspSrchPostings->uiDocumentID;
                uiEndDocumentID = (psplSrchPostingsList1->pspSrchPostings + (psplSrchPostingsList1->uiSrchPostingsLength - 1))->uiDocumentID;
            }


            /* Get the search postings list structure for this term */
            if ( (iError = iSrchSearchGetPostingsListFromParserTerm(pssSrchSearch, psiSrchIndex, uiLanguageID, 
                    (struct srchParserTerm *)psptcSrchParserTermCluster->ppvTerms[uiI], uiStartDocumentID, uiEndDocumentID, &psplSrchPostingsList2)) != SRCH_NoError ) {
                iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the postings list for a term, index: '%s', srch error: %d.", 
                        psiSrchIndex->pucIndexName, iError);
                return (iError);
            }
        }

        /* Its a term cluster */
        else if ( psptcSrchParserTermCluster->puiTermTypeIDs[uiI] == SRCH_PARSER_TERM_TYPE_TERM_CLUSTER_ID ) {

            /* Call ourselves */
            if ( (iError = iSrchSearchGetPostingsListFromParserTermCluster(pssSrchSearch, psiSrchIndex, uiLanguageID, 
                    (struct srchParserTermCluster *)psptcSrchParserTermCluster->ppvTerms[uiI], &psplSrchPostingsList2)) != SRCH_NoError ) {
                iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the postings list for a term cluster, index: '%s', srch error: %d.", 
                        psiSrchIndex->pucIndexName, iError);
                return (iError);
            }
        }

        /* Ouch - invalid term type ?? */
        else {
            ASSERT(false);
        }


        /* Merge the search postings list structures if the first search postings list structure is defined */
        if ( psplSrchPostingsList1 != NULL ) {

            /* AND */
            if ( psptcSrchParserTermCluster->uiOperatorID == SRCH_PARSER_OPERATOR_AND_ID ) {
                
                /* AND the two search postings list structures */
                if ( (iError = iSrchPostingMergeSrchPostingsListsAND(psplSrchPostingsList1, psplSrchPostingsList2, uiSrchPostingBooleanOperationID, 
                        &psplSrchPostingsList2)) != SRCH_NoError ) {
                    iUtlLogError(UTL_LOG_CONTEXT, "Failed to AND merge postings list, srch error: %d.", iError);
                    return (iError);
                }
                
                /* NULL out the first search postings list structure */
                psplSrchPostingsList1 = NULL;
            }

            /* OR */
            else if ( psptcSrchParserTermCluster->uiOperatorID == SRCH_PARSER_OPERATOR_OR_ID ) {

                /* OR the two search postings list structures */
                if ( (iError = iSrchPostingMergeSrchPostingsListsOR(psplSrchPostingsList1, psplSrchPostingsList2, uiSrchPostingBooleanOperationID, 
                        &psplSrchPostingsList2)) != SRCH_NoError ) {
                    iUtlLogError(UTL_LOG_CONTEXT, "Failed to OR merge postings list, srch error: %d.", iError);
                    return (iError);
                }
                
                /* NULL out the first search postings list structure */
                psplSrchPostingsList1 = NULL;
            }

            /* IOR */
            else if ( psptcSrchParserTermCluster->uiOperatorID == SRCH_PARSER_OPERATOR_IOR_ID ) {

                /* IOR the two search postings list structures */
                if ( (iError = iSrchPostingMergeSrchPostingsListsIOR(psplSrchPostingsList1, psplSrchPostingsList2, uiSrchPostingBooleanOperationID,     
                        &psplSrchPostingsList2)) != SRCH_NoError ) {
                    iUtlLogError(UTL_LOG_CONTEXT, "Failed to IOR merge postings list, srch error: %d.", iError);
                    return (iError);
                }
                
                /* NULL out the first search postings list structure */
                psplSrchPostingsList1 = NULL;
            }

            /* XOR */
            else if ( psptcSrchParserTermCluster->uiOperatorID == SRCH_PARSER_OPERATOR_XOR_ID ) {

                /* XOR the two search postings list structures */
                if ( (iError = iSrchPostingMergeSrchPostingsListsXOR(psplSrchPostingsList1, psplSrchPostingsList2, uiSrchPostingBooleanOperationID, 
                        &psplSrchPostingsList2)) != SRCH_NoError ) {
                    iUtlLogError(UTL_LOG_CONTEXT, "Failed to XOR merge postings list, srch error: %d.", iError);
                    return (iError);
                }
                
                /* NULL out the first search postings list structure */
                psplSrchPostingsList1 = NULL;
            }

            /* NOT */
            else if ( psptcSrchParserTermCluster->uiOperatorID == SRCH_PARSER_OPERATOR_NOT_ID ) {

                /* NOT the two search postings list structures */
                if ( (iError = iSrchPostingMergeSrchPostingsListsNOT(psplSrchPostingsList1, psplSrchPostingsList2, uiSrchPostingBooleanOperationID, 
                        &psplSrchPostingsList2)) != SRCH_NoError ) {
                    iUtlLogError(UTL_LOG_CONTEXT, "Failed to NOT merge postings list, srch error: %d.", iError);
                    return (iError);
                }
                
                /* NULL out the first search postings list structure */
                psplSrchPostingsList1 = NULL;
            }

            /* ADJ */
            else if ( psptcSrchParserTermCluster->uiOperatorID == SRCH_PARSER_OPERATOR_ADJ_ID ) {
            
                /* ADJ the two search postings list structures */
            
                /* Here we need to skip over stop terms in the phrase, but we 
                ** still take account of them in the term distance.
                */
                if ( (psplSrchPostingsList2 != NULL) && (psplSrchPostingsList2->uiTermType == SPI_TERM_TYPE_STOP) ) {
                    
                    /* Increment the term distance */
                    iTermDistanceADJ++;
                    
                    /* Free the stop term search postings list structure */
                    iSrchPostingFreeSrchPostingsList(psplSrchPostingsList2);
                    psplSrchPostingsList2 = NULL;
                    
                    /* Hand the first search postings list structure pointer to the second search postings list structure 
                    ** structure pointer, and null out the second posting structure pointer
                    */
                    psplSrchPostingsList2 = psplSrchPostingsList1;
                    psplSrchPostingsList1 = NULL;
                }
                else {
                    
                    /* ADJ the two search postings list structures */
                    if ( (iError = iSrchPostingMergeSrchPostingsListsADJ(psplSrchPostingsList1, psplSrchPostingsList2, iTermDistanceADJ, 
                            uiSrchPostingBooleanOperationID, &psplSrchPostingsList2)) != SRCH_NoError ) {
                        iUtlLogError(UTL_LOG_CONTEXT, "Failed to ADJ merge postings list, srch error: %d.", iError);
                        return (iError);
                    }
                
                    /* Reset the term distance to 1 for the next term */
                    iTermDistanceADJ = 1;
            
                    /* NULL out the first search postings list structure */
                    psplSrchPostingsList1 = NULL;
                }
            }

            /* NEAR */
            else if ( psptcSrchParserTermCluster->uiOperatorID == SRCH_PARSER_OPERATOR_NEAR_ID ) {

                /* NEAR the two search postings list structures */
            
                /* Here we need to skip over stop terms in the phrase, but we 
                ** still take account of them in the term distance.
                */
                if ( (psplSrchPostingsList2 != NULL) && (psplSrchPostingsList2->uiTermType == SPI_TERM_TYPE_STOP) ) {
                    
                    /* Increment the term distance */
                    iTermDistanceNEAR++;
                    
                    /* Free the stop term search postings list structure */
                    iSrchPostingFreeSrchPostingsList(psplSrchPostingsList2);
                    psplSrchPostingsList2 = NULL;
                    
                    /* Hand the first search postings list structure pointer to the second search postings list structure 
                    ** structure pointer, and null out the second posting structure pointer
                    */
                    psplSrchPostingsList2 = psplSrchPostingsList1;
                    psplSrchPostingsList1 = NULL;
                }
                else {
                    
                    /* Set the term distance, default to SRCH_SEARCH_TERM_NEAR_DISTANCE_DEFAULT */
                    int    iTermDistance = (psptcSrchParserTermCluster->iTermDistance != 0) ? psptcSrchParserTermCluster->iTermDistance : SRCH_SEARCH_TERM_NEAR_DISTANCE_DEFAULT;
                    
                    /* Adjust the term distance */
                    iTermDistance += (iTermDistance > 0) ? iTermDistanceNEAR : -iTermDistanceNEAR;

                    /* NEAR the two search postings list structures */
                    if ( (iError = iSrchPostingMergeSrchPostingsListsNEAR(psplSrchPostingsList1, psplSrchPostingsList2, iTermDistance, 
                            psptcSrchParserTermCluster->bDistanceOrderMatters, uiSrchPostingBooleanOperationID, &psplSrchPostingsList2)) != SRCH_NoError ) {
                        iUtlLogError(UTL_LOG_CONTEXT, "Failed to NEAR merge postings list, srch error: %d.", iError);
                        return (iError);
                    }

                
                    /* Reset the term distance to 0 for the next term */
                    iTermDistanceNEAR = 0;
            
                    /* NULL out the first search postings list structure */
                    psplSrchPostingsList1 = NULL;
                }
            }

            /* Ouch - invalid operator tag ?? */
            else {
/*                 iUtlLogDebug(UTL_LOG_CONTEXT, "iSrchSearchGetPostingsListFromParserTermCluster - psptcSrchParserTermCluster->uiOperatorID [%u].", psptcSrchParserTermCluster->uiOperatorID); */
                ASSERT(false);
            }
        }
    }


    ASSERT(psplSrchPostingsList1 == NULL);


    /* Set the return pointer */
    *ppsplSrchPostingsList = psplSrchPostingsList2;


    return (SRCH_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchSearchGetPostingsListFromParserTerm()

    Purpose:    This function searches for a single term in the index, the actual
                scoring takes place in another function, but this functions handles
                wildcards and field searching.     Due to this the scoring functions
                may be called multiple times.

    Parameters: pssSrchSearch           search structure
                psiSrchIndex            index structure
                uiLanguageID            language ID
                psptSrchParserTerm      search parser term to search for
                uiStartDocumentID       start document ID (0 for no limit)
                uiEndDocumentID         end document ID (0 for no limit)
                ppsplSrchPostingsList   return pointer for the search postings list structure

    Globals:    none

    Returns:    SRCH Error code

*/
static int iSrchSearchGetPostingsListFromParserTerm
(
    struct srchSearch *pssSrchSearch,
    struct srchIndex *psiSrchIndex,
    unsigned int uiLanguageID, 
    struct srchParserTerm *psptSrchParserTerm,
    unsigned int uiStartDocumentID, 
    unsigned int uiEndDocumentID, 
    struct srchPostingsList **ppsplSrchPostingsList
)
{

    int                         iError = SRCH_NoError;
    boolean                     bCachableSearch = false;
    unsigned int                uiSrchParserSearchCacheID = SRCH_PARSER_MODIFIER_UNKNOWN_ID;
    off_t                       zSearchReportStartOffset = 0;
    off_t                       zSearchReportEndOffset = 0;
    unsigned char               *pucSearchReportSnippet = NULL;
    unsigned int                uiFieldOptions = SRCH_INFO_FIELD_OPTION_NONE;
    unsigned char               *pucFieldIDBitmap = NULL;
    boolean                     bValidFieldName = false;
    boolean                     bUpperCaseTerm = false;
    boolean                     bMixedCaseTerm = false;
    boolean                     bLowerCaseTerm = false;
    void                        *pvLngStemmer = NULL;
    unsigned char               *pucTerm = NULL;
    wchar_t                     *pwcTermStemmed = NULL;
    unsigned char               *pucTermStemmed = NULL;
    wchar_t                     *pwcTermLowerCase = NULL;
    unsigned char               *pucTermLowerCase = NULL;
    float                       fTermWeight = SRCH_SEARCH_TERM_WEIGHT_DEFAULT;
    float                       fFrequentTermCoverageThreshold = 0;
    unsigned char               pucConfigValue[SRCH_INFO_SYMBOL_MAXIMUM_LENGTH + 1] = {'\0'};
    boolean                     bTermExpanded = false;

    struct srchTermDictInfo     *pstdiSrchTermDictInfos = NULL;
    struct srchTermDictInfo     *pstdiSrchTermDictInfosPtr = NULL;
    unsigned int                uiSrchTermDictInfosLength = 0;
    unsigned int                uiI = 0;

    struct srchPostingsList     *psplSrchPostingsList = NULL;



    ASSERT(pssSrchSearch != NULL);
    ASSERT(psiSrchIndex != NULL);
    ASSERT(uiLanguageID >= 0);
    ASSERT(psptSrchParserTerm != NULL);
    ASSERT(ppsplSrchPostingsList != NULL);
    ASSERT(uiStartDocumentID >= 0);
    ASSERT(uiEndDocumentID >= 0);




    /*=======================================
    **
    ** Check the cache
    **
    **  - Check if the search is cachable
    **  - Check if the search is cached
    **
    */

    /* Get the parser search cache ID */
    if ( (iError = iSrchParserGetModifierID(pssSrchSearch->pvSrchParser, SRCH_PARSER_MODIFIER_SEARCH_CACHE_ID, &uiSrchParserSearchCacheID)) != SRCH_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the parser search cache ID, srch error: %d.", iError);
        goto bailFromiSrchSearchGetPostingsListFromParserTerm;
    }

    /* Check the cache */
    if ( uiSrchParserSearchCacheID == SRCH_PARSER_MODIFIER_SEARCH_CACHE_ENABLE_ID ) {
    
        /* This is not a cachable search until we say that it is */
        bCachableSearch = false;

        /* Check if there are unfielded search field names if no field name was specified for this term */
        if ( bUtlStringsIsWideStringNULL(psptSrchParserTerm->pwcFieldName) == true ) {
            
            unsigned char   pucUnfieldedSearchFieldNames[SPI_FIELD_NAME_MAXIMUM_LENGTH + 1] = {"\0"};
            
            /* Set the cachable search flag if there are unfielded search field names for this search */
            if ( iSrchInfoGetUnfieldedSearchFieldNames(psiSrchIndex, pucUnfieldedSearchFieldNames, SPI_FIELD_NAME_MAXIMUM_LENGTH + 1) == SRCH_NoError ) {
                bCachableSearch = true;
            }
        }
        /* Check that the field name specified for this term is valid */
        else if ( s_wcscmp(psptSrchParserTerm->pwcFieldName, SRCH_PARSER_WILDCARD_FIELD_NAME_WSTRING) != 0 ) {

            unsigned char   pucFieldName[SRCH_INFO_SYMBOL_MAXIMUM_LENGTH + 1] = {'\0'};
            unsigned int    uiFieldID = 0;
    
            /* Convert the field name from wide characters to utf-8 */
            if ( (iError = iLngConvertWideStringToUtf8_s(psptSrchParserTerm->pwcFieldName, 0, pucFieldName, SRCH_INFO_SYMBOL_MAXIMUM_LENGTH + 1)) != LNG_NoError ) {
                iUtlLogError(UTL_LOG_CONTEXT, "Failed to convert the field name from wide characters to utf-8, lng error: %d.", iError);
                iError = SRCH_SearchCharacterSetConvertionFailed;
                goto bailFromiSrchSearchGetPostingsListFromParserTerm;
            }

            /* Set the cachable search flag if the field name is valid */
            if ( (iError = iSrchInfoGetFieldID(psiSrchIndex, pucFieldName, &uiFieldID)) == SRCH_NoError ) {
                bCachableSearch = true;
            }
        }

        /* Check the cache for a postings list for this parser term */
        if ( bCachableSearch == true ) {

            /* Get the postings list from the search cache */
            if ( (iError = iSrchCacheGetSearchPostingsList(pssSrchSearch->pvSrchCache, psiSrchIndex, uiLanguageID, psptSrchParserTerm, 
                    ppsplSrchPostingsList, &pucSearchReportSnippet)) == SRCH_NoError ) {
    
                /* Append the search report snippet we got back to the search report */
                if ( pucSearchReportSnippet != NULL ) {
                    iSrchReportAppend(pssSrchSearch->pvSrchReport, "%s", pucSearchReportSnippet);
                }
        
                /* Free the search report snippet */
                s_free(pucSearchReportSnippet);
        
                /* We are all set */
                goto bailFromiSrchSearchGetPostingsListFromParserTerm;
            }
    
            /* Get the search report start index, we need to extract the part of the search report that 
            ** gets generated when we run the search and store it in the cache otherwise it gets lost
            */
            if ( iSrchReportGetReportOffset(pssSrchSearch->pvSrchReport, &zSearchReportStartOffset) != SRCH_NoError ) {
                zSearchReportStartOffset = -1;
            }
        }
    }




    /*=======================================
    **
    ** Run some pre-flight checks
    **
    **  - Check for wildcards in functions
    **  - Check for wildcards in ranges
    **
    */

    /* We dont allow wildcards in functions */
    if ( (psptSrchParserTerm->bWildCardSearch == true) && (psptSrchParserTerm->uiFunctionID != SRCH_PARSER_INVALID_ID) ) {
        iSrchReportAppend(pssSrchSearch->pvSrchReport, "%s Wildcard searches are not supported within functions: '%ls[%ls]'\n", REP_SEARCH_MESSAGE, 
                pwcSrchParserGetStringFromID(pssSrchSearch->pvSrchParser, psptSrchParserTerm->uiFunctionID), psptSrchParserTerm->pwcTerm);
        iError = SRCH_NoError;
        goto bailFromiSrchSearchGetPostingsListFromParserTerm;
    }

    /* We dont allow wildcards in ranges  */
    if ( (psptSrchParserTerm->bWildCardSearch == true) && 
            ((psptSrchParserTerm->uiRangeID == SRCH_PARSER_RANGE_NOT_EQUAL_ID) || (psptSrchParserTerm->uiRangeID == SRCH_PARSER_RANGE_GREATER_ID) || 
            (psptSrchParserTerm->uiRangeID == SRCH_PARSER_RANGE_LESS_ID) || (psptSrchParserTerm->uiRangeID == SRCH_PARSER_RANGE_GREATER_OR_EQUAL_ID) || 
            (psptSrchParserTerm->uiRangeID == SRCH_PARSER_RANGE_LESS_OR_EQUAL_ID)) ) {

        iSrchReportAppend(pssSrchSearch->pvSrchReport, "%s Wildcard searches are not supported within ranges: '%ls%ls'\n", REP_SEARCH_MESSAGE, 
                pwcSrchParserGetStringFromID(pssSrchSearch->pvSrchParser, psptSrchParserTerm->uiRangeID), psptSrchParserTerm->pwcTerm);
        iError = SRCH_NoError;
        goto bailFromiSrchSearchGetPostingsListFromParserTerm;
    }




    /*=======================================
    **
    ** Process the field name
    **
    **  - Set the field options
    **  - Check the field name if it is set, updating field options
    **  - Check unfield search option
    **
    */


    /* Set the field options defaults */
    iSrchInfoGetFieldOptionDefaults(psiSrchIndex, &uiFieldOptions);


    /* This is not a valid field name until we validate that it is */
    bValidFieldName = false;

    /* Check the field name if it is set and is not the wildcard field name */
    if ( (bUtlStringsIsWideStringNULL(psptSrchParserTerm->pwcFieldName) == false) && 
            (s_wcscmp(psptSrchParserTerm->pwcFieldName, SRCH_PARSER_WILDCARD_FIELD_NAME_WSTRING) != 0) ) {

        unsigned char   pucFieldName[SRCH_INFO_SYMBOL_MAXIMUM_LENGTH + 1] = {'\0'};
        unsigned int    uiFieldID = 0;
        unsigned int    uiLocalFieldOptions = 0;

        /* Convert the field name from wide characters to utf-8 */
        if ( (iError = iLngConvertWideStringToUtf8_s(psptSrchParserTerm->pwcFieldName, 0, pucFieldName, SRCH_INFO_SYMBOL_MAXIMUM_LENGTH + 1)) != LNG_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to convert the field name from wide characters to utf-8, lng error: %d.", iError);
            iError = SRCH_SearchCharacterSetConvertionFailed;
            goto bailFromiSrchSearchGetPostingsListFromParserTerm;
        }

        /* Look up the field ID for the field name */
        if ( (iError = iSrchInfoGetFieldID(psiSrchIndex, pucFieldName, &uiFieldID)) == SRCH_NoError ) {
                            
            /* Get the field options for this field and set the field options,
            ** we do this using a local variable because we dont want to squash
            ** the existing field options
            */
            if ( (iError = iSrchInfoGetFieldInfo(psiSrchIndex, uiFieldID, NULL, 0, NULL, 0, NULL, &uiLocalFieldOptions)) == SRCH_NoError ) {
                uiFieldOptions = uiLocalFieldOptions;
            }
            
            /* Allocate a field ID bitmap only if there are any fields other than field ID 0 */
            if ( psiSrchIndex->uiFieldIDMaximum > 0 ) {

                /* Allocate the field ID bitmap - field ID 0 is not a field */
                if ( (pucFieldIDBitmap = (unsigned char *)s_malloc(sizeof(unsigned char) * UTL_BITMAP_GET_BITMAP_BYTE_LENGTH(psiSrchIndex->uiFieldIDMaximum))) == NULL ) {
                    iError = SRCH_MemError;
                    goto bailFromiSrchSearchGetPostingsListFromParserTerm;
                }
    
                /* Set the field ID in the bitmap - field ID 0 is not a field */
                UTL_BITMAP_SET_BIT_IN_POINTER(pucFieldIDBitmap, uiFieldID - 1);
            }
            
            /* This is a valid field name, so we display it in the report */
            bValidFieldName = true;
        }
        else {

            /* Log the fact that the field name was not valid so will be ignored */
            iSrchReportAppend(pssSrchSearch->pvSrchReport, "%s The field '%ls' does not exist in this index, ignoring it\n", 
                    REP_SEARCH_WARNING, psptSrchParserTerm->pwcFieldName);
        }
    }



/* vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv */

    /* Check if there is an unfielded search option if the field name was invalid, and if the field name is not the wildcard field name */
    if ( (bValidFieldName == false) && 
            ((bUtlStringsIsWideStringNULL(psptSrchParserTerm->pwcFieldName) == true) || (s_wcscmp(psptSrchParserTerm->pwcFieldName, SRCH_PARSER_WILDCARD_FIELD_NAME_WSTRING) != 0)) ) {
 
        wchar_t         *pwcUnfieldedSearchFieldNames = NULL;
        unsigned char   pucUnfieldedSearchFieldNames[SPI_FIELD_NAME_MAXIMUM_LENGTH + 1] = {"\0"};


        ASSERT(pucFieldIDBitmap == NULL);

        /* Get the unfielded search field names from the parser */
        if ( (iError = iSrchParserGetUnfieldedSearchFieldNames(pssSrchSearch->pvSrchParser, &pwcUnfieldedSearchFieldNames)) != SRCH_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the parser unfielded search field names, srch error: %d.", iError);
            return (iError);
        }

        
        /* Use the unfielded search field names from the parser and, if specified, use them for this search */
        if ( bUtlStringsIsWideStringNULL(pwcUnfieldedSearchFieldNames) == false ) {

            /* Convert the unfielded search field names from wide characters to utf-8 */
            if ( (iError = iLngConvertWideStringToUtf8_s(pwcUnfieldedSearchFieldNames, 0, pucUnfieldedSearchFieldNames, SPI_FIELD_NAME_MAXIMUM_LENGTH + 1)) != LNG_NoError ) {
                iUtlLogError(UTL_LOG_CONTEXT, "Failed to convert the unfielded search field names from wide characters to utf-8, lng error: %d.", iError);
                goto bailFromiSrchSearchGetPostingsListFromParserTerm;
            }
        }

        /* Get the unfielded search field names and, if specified, use them for this search */
        else  if ( iSrchInfoGetUnfieldedSearchFieldNames(psiSrchIndex, pucUnfieldedSearchFieldNames, SPI_FIELD_NAME_MAXIMUM_LENGTH + 1) == SRCH_NoError ) {
            ;
        }

        /* Nothing specified so clear the unfielded search field names, this prevents them from being used */
        else {
            pucUnfieldedSearchFieldNames[0] = '\0';
        }
            

        /* User the unfielded search field names if specified */
        if ( bUtlStringsIsStringNULL(pucUnfieldedSearchFieldNames) == false ) {
            
            unsigned char   *pucUnfieldedSearchFieldNamesStrtokPtr = NULL;
            unsigned char   *pucUnfieldedSearchFieldNamePtr = NULL;
            
            /* Allocate the field ID bitmap, we know there are fields other than field ID 0 since there are unfielded search field names - field ID 0 is not a field */
            if ( (pucFieldIDBitmap = (unsigned char *)s_malloc(sizeof(unsigned char) * UTL_BITMAP_GET_BITMAP_BYTE_LENGTH(psiSrchIndex->uiFieldIDMaximum))) == NULL ) {
                iError = SRCH_MemError;
                goto bailFromiSrchSearchGetPostingsListFromParserTerm;
            }

            /* Search report */
            iSrchReportAppend(pssSrchSearch->pvSrchReport, "%s Unfielded searches are automatically restricted to the following fields: '%s'\n", REP_SEARCH_WARNING, pucUnfieldedSearchFieldNames);

            /* Loop parsing the unfielded search field names, setting the field ID for those fields in the field ID bitmap */
            for ( pucUnfieldedSearchFieldNamePtr = (unsigned char *)s_strtok_r(pucUnfieldedSearchFieldNames, SRCH_INFO_UNFIELDED_SEARCH_FIELD_NAMES_SEPARATORS, (char **)&pucUnfieldedSearchFieldNamesStrtokPtr); 
                    pucUnfieldedSearchFieldNamePtr != NULL; 
                    pucUnfieldedSearchFieldNamePtr = (unsigned char *)s_strtok_r(NULL, SRCH_INFO_UNFIELDED_SEARCH_FIELD_NAMES_SEPARATORS, (char **)&pucUnfieldedSearchFieldNamesStrtokPtr) ) {
    
                unsigned int    uiFieldID = 0;
                
                /* Set the field ID in the bitmap - field ID 0 is not a field */
                if ( iSrchInfoGetFieldID(psiSrchIndex, pucUnfieldedSearchFieldNamePtr, &uiFieldID) == SRCH_NoError ) {
                    UTL_BITMAP_SET_BIT_IN_POINTER(pucFieldIDBitmap, uiFieldID - 1);
                }
            }
        }
    }


//     /* Check if there is an unfielded search option if the field name was invalid, and if the field name is not the wildcard field name */
//     if ( (bValidFieldName == false) && 
//             ((bUtlStringsIsWideStringNULL(psptSrchParserTerm->pwcFieldName) == true) || (s_wcscmp(psptSrchParserTerm->pwcFieldName, SRCH_PARSER_WILDCARD_FIELD_NAME_WSTRING) != 0)) ) {
//     
//         unsigned char   pucUnfieldedSearchFieldNames[SPI_FIELD_NAME_MAXIMUM_LENGTH + 1] = {"\0"};
// 
//         ASSERT(pucFieldIDBitmap == NULL);
// 
//         /* Get the unfielded search field names and, if they exist, use them for this search */
//         if ( iSrchInfoGetUnfieldedSearchFieldNames(psiSrchIndex, pucUnfieldedSearchFieldNames, SPI_FIELD_NAME_MAXIMUM_LENGTH + 1) == SRCH_NoError ) {
//             
//             unsigned char   *pucUnfieldedSearchFieldNamesStrtokPtr = NULL;
//             unsigned char   *pucUnfieldedSearchFieldNamePtr = NULL;
//             
//             /* Allocate the field ID bitmap, we know there are fields other than field ID 0 since there are unfielded search field names - field ID 0 is not a field */
//             if ( (pucFieldIDBitmap = (unsigned char *)s_malloc(sizeof(unsigned char) * UTL_BITMAP_GET_BITMAP_BYTE_LENGTH(psiSrchIndex->uiFieldIDMaximum))) == NULL ) {
//                 iError = SRCH_MemError;
//                 goto bailFromiSrchSearchGetPostingsListFromParserTerm;
//             }
// 
//             /* Search report */
//             iSrchReportAppend(pssSrchSearch->pvSrchReport, "%s Unfielded searches are automatically restricted to the following fields: '%s'\n", REP_SEARCH_WARNING, pucUnfieldedSearchFieldNames);
// 
//             /* Loop parsing the unfielded search field names, setting the field ID for those fields in the field ID bitmap */
//             for ( pucUnfieldedSearchFieldNamePtr = (unsigned char *)s_strtok_r(pucUnfieldedSearchFieldNames, SRCH_INFO_UNFIELDED_SEARCH_FIELD_NAMES_SEPARATORS, (char **)&pucUnfieldedSearchFieldNamesStrtokPtr); 
//                     pucUnfieldedSearchFieldNamePtr != NULL; 
//                     pucUnfieldedSearchFieldNamePtr = (unsigned char *)s_strtok_r(NULL, SRCH_INFO_UNFIELDED_SEARCH_FIELD_NAMES_SEPARATORS, (char **)&pucUnfieldedSearchFieldNamesStrtokPtr) ) {
//     
//                 unsigned int    uiFieldID = 0;
//                 
//                 /* Set the field ID in the bitmap - field ID 0 is not a field */
//                 if ( iSrchInfoGetFieldID(psiSrchIndex, pucUnfieldedSearchFieldNamePtr, &uiFieldID) == SRCH_NoError ) {
//                     UTL_BITMAP_SET_BIT_IN_POINTER(pucFieldIDBitmap, uiFieldID - 1);
//                 }
//             }
//         }
//     }

/* ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ */



    /*=======================================
    **
    ** Process the term
    **
    **  - Convert the term to utf-8
    **  - Convert the term to lower case
    **
    */

    /* Convert the term from wide characters to utf-8, pucTerm is allocated */
    if ( (iError = iLngConvertWideStringToUtf8_d(psptSrchParserTerm->pwcTerm, 0, &pucTerm)) != LNG_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to convert a term from wide characters to utf-8, lng error: %d.", iError);
        iError = SRCH_SearchCharacterSetConvertionFailed;
        goto bailFromiSrchSearchGetPostingsListFromParserTerm;
    }


    /* Duplicate and convert the term to lower case */
    if ( (pwcTermLowerCase = s_wcsdup(psptSrchParserTerm->pwcTerm)) == NULL ) {
        iError = SRCH_MemError;
        goto bailFromiSrchSearchGetPostingsListFromParserTerm;
    }
    pwcLngCaseConvertWideStringToLowerCase(pwcTermLowerCase);

    /* Convert the lower case term from wide characters to utf-8, pucTermLowerCase is allocated */
    if ( (iError = iLngConvertWideStringToUtf8_d(pwcTermLowerCase, 0, &pucTermLowerCase)) != LNG_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to convert a term from wide characters to utf-8, lng error: %d.", iError);
        iError = SRCH_SearchCharacterSetConvertionFailed;
        goto bailFromiSrchSearchGetPostingsListFromParserTerm;
    }


    /*=======================================
    **
    ** Stem the term
    ** 
    **  - Set the term case flags
    **  - Stem the term
    ** 
    ** Upper case terms
    **  - no stemming
    **  - stored in original case and lower case
    **
    ** Mixed case terms
    **  - stemmed
    **  - stored in original case and lower case
    **
    ** Lower case terms
    **  - stemmed
    **  - stored in original case
    */

    /* Set the case flags */
    bUpperCaseTerm = bLngCaseIsWideStringAllUpperCase(psptSrchParserTerm->pwcTerm);
    bMixedCaseTerm = bLngCaseDoesWideStringContainMixedCase(psptSrchParserTerm->pwcTerm);
    bLowerCaseTerm = ((bUpperCaseTerm == false) && (bMixedCaseTerm == false)) ? true : false;

    /* Stem the term if needed, namely if stemming is on and if the term is mixed case or lower case */
    if ( (bSrchInfoFieldOptionStemming(uiFieldOptions) == true) && ((bMixedCaseTerm == true) || (bLowerCaseTerm == true)) ) {

        /* Create the stemmer */
        if ( (iError = iLngStemmerCreateByID(psiSrchIndex->uiStemmerID, uiLanguageID, &pvLngStemmer)) != LNG_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to create a stemmer, lng error: %d.", iError);
            iError = SRCH_SearchCreateStemmerFailed;
            goto bailFromiSrchSearchGetPostingsListFromParserTerm;
        }

        /* Duplicate and stem the term */
        if ( (pwcTermStemmed = s_wcsdup(psptSrchParserTerm->pwcTerm)) == NULL ) {
            iError = SRCH_MemError;
            goto bailFromiSrchSearchGetPostingsListFromParserTerm;
        }
        if ( (iError = iLngStemmerStemTerm(pvLngStemmer, pwcTermStemmed, 0)) != LNG_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to stem a term, lng error: %d", iError);
            iError = SRCH_SearchStemmingFailed;
            goto bailFromiSrchSearchGetPostingsListFromParserTerm;
        }

        /* Convert the stemmed term from wide characters to utf-8, pucTermStemmed is allocated */
        if ( (iError = iLngConvertWideStringToUtf8_d(pwcTermStemmed, 0, &pucTermStemmed)) != LNG_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to convert a term from wide characters to utf-8, lng error: %d.", iError);
            iError = SRCH_SearchCharacterSetConvertionFailed;
            goto bailFromiSrchSearchGetPostingsListFromParserTerm;
        }

        /* Free the stemmer */
        iLngStemmerFree(pvLngStemmer);
        pvLngStemmer = NULL;
    }




    /*=======================================
    **
    ** Process the weights and thresholds
    **
    **  - Set the term weight
    **  - Set the frequent term coverage threshold (defaults to 0)
    **
    */

    /* Set the term weight from the parser term if if was specified with the term */
    if ( psptSrchParserTerm->fTermWeight > 0 ) {
        fTermWeight = psptSrchParserTerm->fTermWeight;
    }
    else {

        /* Get the term weight specified in the search string */
        if ( (iError = iSrchParserGetModifierValue(pssSrchSearch->pvSrchParser, SRCH_PARSER_MODIFIER_TERM_WEIGHT_ID, &fTermWeight)) != SRCH_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the parser term weights, srch error: %d.", iError);
            goto bailFromiSrchSearchGetPostingsListFromParserTerm;
        }


        /* Set the term weight from the config file if it was not specified in the search string */
        if ( fTermWeight == 0 ) {

            /* Set the default term weight */
            fTermWeight = SRCH_SEARCH_TERM_WEIGHT_DEFAULT;

            /* Get the term weight from the config file */
            if ( iUtlConfigGetValue(pssSrchSearch->pvUtlConfig, SRCH_SEARCH_CONFIG_SEARCH_TERM_WEIGHT, pucConfigValue, SRCH_INFO_SYMBOL_MAXIMUM_LENGTH + 1) == UTL_NoError ) {
                if ( s_strtof(pucConfigValue, NULL) <= 0 ) {
                    iUtlLogWarn(UTL_LOG_CONTEXT, "Invalid search term weight found in the search configuration file, config key: '%s', config value: '%s', using default: %d.", 
                            SRCH_SEARCH_CONFIG_SEARCH_TERM_WEIGHT, pucConfigValue, SRCH_SEARCH_TERM_WEIGHT_DEFAULT);
                }
                else {                    
                    fTermWeight = s_strtof(pucConfigValue, NULL);
                }
            }
        }

        /* Set the term weight from the default if all else failed */
        if ( fTermWeight == 0 ) {
            fTermWeight = SRCH_SEARCH_TERM_WEIGHT_DEFAULT;
        }
    }
/*     iUtlLogDebug(UTL_LOG_CONTEXT, "psptSrchParserTerm->pwcTerm: '%ls', fTermWeight: %.2f", psptSrchParserTerm->pwcTerm, fTermWeight); */


    /* Get the frequent term coverage threshold specified in the search string */
    if ( (iError = iSrchParserGetModifierValue(pssSrchSearch->pvSrchParser, SRCH_PARSER_MODIFIER_FREQUENT_TERM_COVERAGE_THRESHOLD_ID, &fFrequentTermCoverageThreshold)) != SRCH_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the parser frequent term coverage threshold, srch error: %d.", iError);
        goto bailFromiSrchSearchGetPostingsListFromParserTerm;
    }
    
    /* Check the config file if the frequent term coverage threshold was not specified in the search string */
    if ( fFrequentTermCoverageThreshold == 0 ) {
    
        /* Get the search frequent term coverage threshold */
        if ( iUtlConfigGetValue(pssSrchSearch->pvUtlConfig, SRCH_SEARCH_CONFIG_SEARCH_FREQUENT_TERM_COVERAGE_THRESHOLD, pucConfigValue, SRCH_INFO_SYMBOL_MAXIMUM_LENGTH + 1) == UTL_NoError ) {
            if ( s_strtof(pucConfigValue, NULL) <= 0 ) {
                iUtlLogWarn(UTL_LOG_CONTEXT, "Invalid frequent term coverage threshold found in the search configuration file, config key: '%s', config value: '%s'.", 
                        SRCH_SEARCH_CONFIG_SEARCH_FREQUENT_TERM_COVERAGE_THRESHOLD, pucConfigValue);
            }
            else {                    
                fFrequentTermCoverageThreshold = s_strtof(pucConfigValue, NULL);
            }
        }
    }
    
    /* The frequent term coverage threshold specified, so we need to check if we can apply it */
    if ( fFrequentTermCoverageThreshold > 0 ) {

        unsigned int    uiSrchParserSearchTermCount = 0;
        unsigned int    uiSrchParserFrequentTermsID = SRCH_PARSER_INVALID_ID;

        /* Get the parser term count */
        if ( (iError = iSrchParserGetSearchTermCount(pssSrchSearch->pvSrchParser, &uiSrchParserSearchTermCount)) != SRCH_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the parser search term count, srch error: %d.", iError);
            goto bailFromiSrchSearchGetPostingsListFromParserTerm;
        }
        
        /* Get the parser frequent terms ID */
        if ( (iError = iSrchParserGetModifierID(pssSrchSearch->pvSrchParser, SRCH_PARSER_MODIFIER_FREQUENT_TERMS_ID, &uiSrchParserFrequentTermsID)) != SRCH_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the parser frequent terms ID, srch error: %d.", iError);
            goto bailFromiSrchSearchGetPostingsListFromParserTerm;
        }
        
        /* Zero out the frequent term coverage threshold if there is only one term in the search or have to keep terms */
        if ( (uiSrchParserSearchTermCount == 1) || (uiSrchParserFrequentTermsID == SRCH_PARSER_MODIFIER_FREQUENT_TERMS_KEEP_ID) ) {
            fFrequentTermCoverageThreshold = 0;
        }
    }




    /*=======================================
    **
    ** Check for expansion
    **
    **  - Expand the term if needed
    **      - Process functions
    **      - Process term ranges
    **      - Process wildcards
    **      (any expansion sets a flag, even on errors) 
    **  - Check for non recoverable errors and no expansion
    **
    */

    /* This term is not expanded until we say it is */
    bTermExpanded = false;

    /* Process metaphone function */
    if ( psptSrchParserTerm->uiFunctionID == SRCH_PARSER_FUNCTION_METAPHONE_ID ) {
#if defined(SRCH_SEARCH_ENABLE_CASELESS_METAPHONE)
        iError = iSrchTermDictLookupMetaphone(psiSrchIndex, uiLanguageID, pucTermLowerCase, pucFieldIDBitmap, (pucFieldIDBitmap != NULL) ? psiSrchIndex->uiFieldIDMaximum : 0, 
                &pstdiSrchTermDictInfos, &uiSrchTermDictInfosLength);
#else
        iError = iSrchTermDictLookupMetaphone(psiSrchIndex, uiLanguageID, pucTerm, pucFieldIDBitmap, (pucFieldIDBitmap != NULL) ? psiSrchIndex->uiFieldIDMaximum : 0, 
                &pstdiSrchTermDictInfos, &uiSrchTermDictInfosLength);
#endif
        bTermExpanded = true;
    }

    /* Process soundex function */
    else if ( psptSrchParserTerm->uiFunctionID == SRCH_PARSER_FUNCTION_SOUNDEX_ID ) {
#if defined(SRCH_SEARCH_ENABLE_CASELESS_PHONIX)
        iError = iSrchTermDictLookupSoundex(psiSrchIndex, uiLanguageID, pucTermLowerCase, pucFieldIDBitmap, (pucFieldIDBitmap != NULL) ? psiSrchIndex->uiFieldIDMaximum : 0, 
                &pstdiSrchTermDictInfos, &uiSrchTermDictInfosLength);
#else
        iError = iSrchTermDictLookupSoundex(psiSrchIndex, uiLanguageID, pucTerm, pucFieldIDBitmap, (pucFieldIDBitmap != NULL) ? psiSrchIndex->uiFieldIDMaximum : 0, 
                &pstdiSrchTermDictInfos, &uiSrchTermDictInfosLength);
#endif
        bTermExpanded = true;
    }

    /* Process phonix function */
    else if ( psptSrchParserTerm->uiFunctionID == SRCH_PARSER_FUNCTION_PHONIX_ID ) {
#if defined(SRCH_SEARCH_ENABLE_CASELESS_SOUNDEX)
        iError = iSrchTermDictLookupPhonix(psiSrchIndex, uiLanguageID, pucTermLowerCase, pucFieldIDBitmap, (pucFieldIDBitmap != NULL) ? psiSrchIndex->uiFieldIDMaximum : 0, 
                &pstdiSrchTermDictInfos, &uiSrchTermDictInfosLength);
#else
        iError = iSrchTermDictLookupPhonix(psiSrchIndex, uiLanguageID, pucTerm, pucFieldIDBitmap, (pucFieldIDBitmap != NULL) ? psiSrchIndex->uiFieldIDMaximum : 0, 
                &pstdiSrchTermDictInfos, &uiSrchTermDictInfosLength);
#endif
        bTermExpanded = true;
    }

    /* Process typo function */
    else if ( psptSrchParserTerm->uiFunctionID == SRCH_PARSER_FUNCTION_TYPO_ID ) {
        iError = iSrchTermDictLookupTypo(psiSrchIndex, uiLanguageID, pucTerm, pucFieldIDBitmap, (pucFieldIDBitmap != NULL) ? psiSrchIndex->uiFieldIDMaximum : 0, 
                &pstdiSrchTermDictInfos, &uiSrchTermDictInfosLength);
        bTermExpanded = true;
    }

    /* Process regex function */
    else if ( psptSrchParserTerm->uiFunctionID == SRCH_PARSER_FUNCTION_REGEX_ID ) {
        iError = iSrchTermDictLookupRegex(psiSrchIndex, pucTerm, pucFieldIDBitmap, (pucFieldIDBitmap != NULL) ? psiSrchIndex->uiFieldIDMaximum : 0, 
                &pstdiSrchTermDictInfos, &uiSrchTermDictInfosLength);
        bTermExpanded = true;
    }

    /* Process literal function */
    else if ( psptSrchParserTerm->uiFunctionID == SRCH_PARSER_FUNCTION_LITERAL_ID ) {
        /* No expansion for literals, this is here for completeness only */
        ;
    }

    /* Process range function */
    else if ( psptSrchParserTerm->uiFunctionID == SRCH_PARSER_FUNCTION_RANGE_ID ) {
        iError = iSrchTermDictLookupTermRange(psiSrchIndex, pucTerm, pucFieldIDBitmap, (pucFieldIDBitmap != NULL) ? psiSrchIndex->uiFieldIDMaximum : 0, 
                psptSrchParserTerm->uiRangeID, &pstdiSrchTermDictInfos, &uiSrchTermDictInfosLength);
        bTermExpanded = true;
    }

    /* Process term ranges */
    else if ( (psptSrchParserTerm->uiRangeID == SRCH_PARSER_RANGE_NOT_EQUAL_ID) || 
                (psptSrchParserTerm->uiRangeID == SRCH_PARSER_RANGE_GREATER_ID) ||
                (psptSrchParserTerm->uiRangeID == SRCH_PARSER_RANGE_LESS_ID) || 
                (psptSrchParserTerm->uiRangeID == SRCH_PARSER_RANGE_GREATER_OR_EQUAL_ID) ||
                (psptSrchParserTerm->uiRangeID == SRCH_PARSER_RANGE_LESS_OR_EQUAL_ID) ) {
            
        iError = iSrchTermDictLookupTermRange(psiSrchIndex, pucTerm, pucFieldIDBitmap, (pucFieldIDBitmap != NULL) ? psiSrchIndex->uiFieldIDMaximum : 0, 
                psptSrchParserTerm->uiRangeID, &pstdiSrchTermDictInfos, &uiSrchTermDictInfosLength);
        bTermExpanded = true;
    }
    
    /* Process wildcards */
    else if ( psptSrchParserTerm->bWildCardSearch == true ) {
        
        unsigned char   *pucTermPtr = NULL;
        
        /* Use the stemmed term if we are stemming on this field and if the term does not end in a wildcard  */
        pucTermPtr = ((pwcTermStemmed != NULL) && (s_strchr(SRCH_PARSER_WILDCARDS_STRING, pucTerm[s_strlen(pucTerm) - 1]) == NULL)) ? pucTermStemmed : pucTerm;

        iError = iSrchTermDictLookupWildCard(psiSrchIndex, pucTermPtr, pucFieldIDBitmap, (pucFieldIDBitmap != NULL) ? psiSrchIndex->uiFieldIDMaximum : 0, 
                &pstdiSrchTermDictInfos, &uiSrchTermDictInfosLength);
        bTermExpanded = true;
    }


    /* Term was flagged as expanded, so we check for non recoverable errors 
    ** (all the errors listed below are recoverable) 
    */
    if ( (bTermExpanded == true) && 
            !((iError == SRCH_NoError) ||
            (iError == SRCH_TermDictTermBadRange) ||
            (iError == SRCH_TermDictTermBadWildCard) ||
            (iError == SRCH_TermDictTermNotFound) || 
            (iError == SRCH_TermDictTermDoesNotOccur)) ) {

        iUtlLogError(UTL_LOG_CONTEXT, "Failed to expand a term, index: '%s', srch error: %d.", psiSrchIndex->pucIndexName, iError);
        goto bailFromiSrchSearchGetPostingsListFromParserTerm;
    }
    
    
    /* Term was flagged as expanded, so we check for no expansion */
    if ( (bTermExpanded == true) && ((pstdiSrchTermDictInfos == NULL) || (uiSrchTermDictInfosLength == 0)) ) {

        /* Create an empty postings list */
        if ( (iError = iSrchPostingCreateSrchPostingsList(SPI_TERM_TYPE_UNKNOWN, SPI_TERM_COUNT_UNKNOWN, SPI_TERM_DOCUMENT_COUNT_UNKNOWN, 
                false, NULL, 0, &psplSrchPostingsList)) != SRCH_NoError ) {
            goto bailFromiSrchSearchGetPostingsListFromParserTerm;
        }

        /* Search report - metaphone, soundex, phonix, typo and regex functions */
        if ( (psptSrchParserTerm->uiFunctionID == SRCH_PARSER_FUNCTION_METAPHONE_ID) || 
                (psptSrchParserTerm->uiFunctionID == SRCH_PARSER_FUNCTION_SOUNDEX_ID) ||
                (psptSrchParserTerm->uiFunctionID == SRCH_PARSER_FUNCTION_PHONIX_ID) || 
                (psptSrchParserTerm->uiFunctionID == SRCH_PARSER_FUNCTION_TYPO_ID) ||
                (psptSrchParserTerm->uiFunctionID == SRCH_PARSER_FUNCTION_REGEX_ID) ) {
            
            iSrchReportAppend(pssSrchSearch->pvSrchReport, "%s %ls[%ls]", REP_SEARCH_TERM_EXPANDED, 
                    pwcSrchParserGetStringFromID(pssSrchSearch->pvSrchParser, psptSrchParserTerm->uiFunctionID), psptSrchParserTerm->pwcTerm);
            iSrchReportAppend(pssSrchSearch->pvSrchReport, " %ls", (bValidFieldName == true) ? psptSrchParserTerm->pwcFieldName : REP_UNFIELDED_WSTRING);
            iSrchReportAppend(pssSrchSearch->pvSrchReport, " %.2f", psptSrchParserTerm->fTermWeight);

            /* It is a frequent term, switch it to a stop term */
            if ( psplSrchPostingsList->uiTermType == SPI_TERM_TYPE_FREQUENT ) {
                psplSrchPostingsList->uiTermType = SPI_TERM_TYPE_STOP;
                iSrchReportAppend(pssSrchSearch->pvSrchReport, "\t%ls %d %d\n", pwcTermLowerCase, REP_TERM_FREQUENT, REP_TERM_FREQUENT);
            }
            /* It is a stop term */
            else if ( psplSrchPostingsList->uiTermType == SPI_TERM_TYPE_STOP ) {
                iSrchReportAppend(pssSrchSearch->pvSrchReport, "\t%ls %d %d\n", pwcTermLowerCase, REP_TERM_STOP, REP_TERM_STOP);
            }
            /* It is a normal term */
            else {

                iSrchReportAppend(pssSrchSearch->pvSrchReport, "\t%ls", pwcTermLowerCase);

                if ( iError == SRCH_TermDictTermDoesNotOccur ) {
                    iSrchReportAppend(pssSrchSearch->pvSrchReport, " 0 0\n");
                }
                else {
                    iSrchReportAppend(pssSrchSearch->pvSrchReport, " %d %d\n", REP_TERM_NON_EXISTENT, REP_TERM_NON_EXISTENT);
                }
            }
        }

        /* Search report - literal function */
        else if ( psptSrchParserTerm->uiFunctionID == SRCH_PARSER_FUNCTION_LITERAL_ID ) {
            /* No expansion for literals, this is here for completeness only */
            ;
        }

        /* Search report - range function */
        else if ( psptSrchParserTerm->uiFunctionID == SRCH_PARSER_FUNCTION_RANGE_ID ) {

            if ( iError == SRCH_TermDictTermBadRange ) {
                iSrchReportAppend(pssSrchSearch->pvSrchReport, "%s Term range search, '%ls%ls%ls[%ls]', is invalid.", REP_SEARCH_MESSAGE, psptSrchParserTerm->pwcFieldName, 
                        pwcSrchParserGetStringFromID(pssSrchSearch->pvSrchParser, psptSrchParserTerm->uiRangeID), 
                        pwcSrchParserGetStringFromID(pssSrchSearch->pvSrchParser, psptSrchParserTerm->uiFunctionID), psptSrchParserTerm->pwcTerm);
                iSrchReportAppend(pssSrchSearch->pvSrchReport, " Term range search can only be done using a valid range, for example: 'title=range[aaa-ccc]',");
                iSrchReportAppend(pssSrchSearch->pvSrchReport, " 'title=range[Aaa-Ccc]', 'title=range[AAA-CCC]' or 'number=range[100-200]'\n");
            }
            else {
                iSrchReportAppend(pssSrchSearch->pvSrchReport, "%s %ls[%ls]", REP_SEARCH_TERM_EXPANDED, 
                        pwcSrchParserGetStringFromID(pssSrchSearch->pvSrchParser, psptSrchParserTerm->uiFunctionID), psptSrchParserTerm->pwcTerm);
                iSrchReportAppend(pssSrchSearch->pvSrchReport, " %ls", (bValidFieldName == true) ? psptSrchParserTerm->pwcFieldName : REP_UNFIELDED_WSTRING);
                iSrchReportAppend(pssSrchSearch->pvSrchReport, " %.2f", psptSrchParserTerm->fTermWeight);
                iSrchReportAppend(pssSrchSearch->pvSrchReport, "\t%ls", psptSrchParserTerm->pwcTerm);

                if ( iError == SRCH_TermDictTermDoesNotOccur ) {
                    iSrchReportAppend(pssSrchSearch->pvSrchReport, " 0 0\n");
                }
                else {
                    iSrchReportAppend(pssSrchSearch->pvSrchReport, " %d %d\n", REP_TERM_NON_EXISTENT, REP_TERM_NON_EXISTENT);
                }
            }
        }
        
        /* Search report, term ranges */
        else if ( (psptSrchParserTerm->uiRangeID == SRCH_PARSER_RANGE_NOT_EQUAL_ID) || 
                (psptSrchParserTerm->uiRangeID == SRCH_PARSER_RANGE_GREATER_ID) ||
                (psptSrchParserTerm->uiRangeID == SRCH_PARSER_RANGE_LESS_ID) || 
                (psptSrchParserTerm->uiRangeID == SRCH_PARSER_RANGE_GREATER_OR_EQUAL_ID) ||
                (psptSrchParserTerm->uiRangeID == SRCH_PARSER_RANGE_LESS_OR_EQUAL_ID) ) {
                
            iSrchReportAppend(pssSrchSearch->pvSrchReport, "%s %ls[%ls]", REP_SEARCH_TERM_EXPANDED, 
                    pwcSrchParserGetStringFromID(pssSrchSearch->pvSrchParser, psptSrchParserTerm->uiRangeID), psptSrchParserTerm->pwcTerm);
            iSrchReportAppend(pssSrchSearch->pvSrchReport, " %ls", (bValidFieldName == true) ? psptSrchParserTerm->pwcFieldName : REP_UNFIELDED_WSTRING);
            iSrchReportAppend(pssSrchSearch->pvSrchReport, " %.2f", psptSrchParserTerm->fTermWeight);
            iSrchReportAppend(pssSrchSearch->pvSrchReport, "\t%ls", psptSrchParserTerm->pwcTerm);

            if ( iError == SRCH_TermDictTermDoesNotOccur ) {
                iSrchReportAppend(pssSrchSearch->pvSrchReport, " 0 0\n");
            }
            else {
                iSrchReportAppend(pssSrchSearch->pvSrchReport, " %d %d\n", REP_TERM_NON_EXISTENT, REP_TERM_NON_EXISTENT);
            }
        }
        
        /* Search report, wildcards */
        else if ( psptSrchParserTerm->bWildCardSearch == true ) {

            if ( iError == SRCH_TermDictTermBadWildCard ) {
                iSrchReportAppend(pssSrchSearch->pvSrchReport, "%s Wildcard search, '%ls', is invalid\n", REP_SEARCH_MESSAGE, psptSrchParserTerm->pwcTerm);
            }
            else {
                iSrchReportAppend(pssSrchSearch->pvSrchReport, "%s %ls", REP_SEARCH_TERM_EXPANDED, psptSrchParserTerm->pwcTerm);
                iSrchReportAppend(pssSrchSearch->pvSrchReport, " %ls", (bValidFieldName == true) ? psptSrchParserTerm->pwcFieldName : REP_UNFIELDED_WSTRING);
                iSrchReportAppend(pssSrchSearch->pvSrchReport, " %.2f", psptSrchParserTerm->fTermWeight);
                iSrchReportAppend(pssSrchSearch->pvSrchReport, "\t%ls", psptSrchParserTerm->pwcTerm);

                if ( iError == SRCH_TermDictTermDoesNotOccur ) {
                    iSrchReportAppend(pssSrchSearch->pvSrchReport, " 0 0\n");
                }
                else {
                    iSrchReportAppend(pssSrchSearch->pvSrchReport, " %d %d\n", REP_TERM_NON_EXISTENT, REP_TERM_NON_EXISTENT);
                }
            }
        }
        

        /* Reset the error and bail */
        iError = SRCH_NoError;
        goto bailFromiSrchSearchGetPostingsListFromParserTerm;
    }
    



    /*=======================================
    **
    ** Search for the term
    **  - search for the term if not expanded
    **  - else search for the expanded term
    **
    */

    /* Non-expanded terms */
    if ( bTermExpanded == false ) {

        unsigned char   *pucTermPtr = NULL;

        /* Search report */
        if ( psptSrchParserTerm->uiFunctionID == SRCH_PARSER_FUNCTION_LITERAL_ID ) {
/*             iSrchReportAppend(pssSrchSearch->pvSrchReport, "%s %ls[%ls]", REP_SEARCH_TERM,  */
/*                     pwcSrchParserGetStringFromID(pssSrchSearch->pvSrchParser, psptSrchParserTerm->uiFunctionID), psptSrchParserTerm->pwcTerm); */
            iSrchReportAppend(pssSrchSearch->pvSrchReport, "%s %ls", REP_SEARCH_TERM, psptSrchParserTerm->pwcTerm);
            iSrchReportAppend(pssSrchSearch->pvSrchReport, " %ls", (bValidFieldName == true) ? psptSrchParserTerm->pwcFieldName : REP_UNFIELDED_WSTRING);
            iSrchReportAppend(pssSrchSearch->pvSrchReport, " %ls", REP_UNSTEMMED_WSTRING);
        }
        else {
            iSrchReportAppend(pssSrchSearch->pvSrchReport, "%s %ls", REP_SEARCH_TERM, psptSrchParserTerm->pwcTerm);
            iSrchReportAppend(pssSrchSearch->pvSrchReport, " %ls", (bValidFieldName == true) ? psptSrchParserTerm->pwcFieldName : REP_UNFIELDED_WSTRING);
            iSrchReportAppend(pssSrchSearch->pvSrchReport, " %ls", (pwcTermStemmed != NULL) ? pwcTermStemmed : REP_UNSTEMMED_WSTRING);
        }
        iSrchReportAppend(pssSrchSearch->pvSrchReport, " %.2f", psptSrchParserTerm->fTermWeight);


        /* Select the stemmed term if we stemmed and if this is not a literal search */
        pucTermPtr = ((pwcTermStemmed != NULL) && (psptSrchParserTerm->uiFunctionID != SRCH_PARSER_FUNCTION_LITERAL_ID)) ? pucTermStemmed : pucTerm;

        /* Look up the term */
        if ( (iError = iSrchTermSearchGetSearchPostingsListFromTerm(pssSrchSearch, psiSrchIndex, pucTermPtr, fTermWeight, pucFieldIDBitmap, 
                (pucFieldIDBitmap != NULL) ? psiSrchIndex->uiFieldIDMaximum : 0, fFrequentTermCoverageThreshold, uiStartDocumentID, uiEndDocumentID, &psplSrchPostingsList)) != SRCH_NoError ) {
            goto bailFromiSrchSearchGetPostingsListFromParserTerm;
        }

        ASSERT(psplSrchPostingsList != NULL);

        /* Set the required flag */
        psplSrchPostingsList->bRequired = psptSrchParserTerm->bRequired;

        /* Turn stop terms into regular terms if this was a fielded search and stop terms are turned off the field, ie they were indexed */
        if ( (psplSrchPostingsList->uiTermType == SPI_TERM_TYPE_STOP) && (pucFieldIDBitmap != NULL) && (bSrchInfoFieldOptionStopTerm(uiFieldOptions) == false) ) {
            psplSrchPostingsList->uiTermType = SPI_TERM_TYPE_REGULAR;
            iSrchReportAppend(pssSrchSearch->pvSrchReport, " %u %u\n", psplSrchPostingsList->uiTermCount, psplSrchPostingsList->uiDocumentCount);
        }
        /* Stop terms */
        else if ( psplSrchPostingsList->uiTermType == SPI_TERM_TYPE_STOP ) {
            iSrchReportAppend(pssSrchSearch->pvSrchReport, " %d %d\n", REP_TERM_STOP, REP_TERM_STOP);
        }
        /* Frequent terms, they get turned into stop terms */
        else if ( psplSrchPostingsList->uiTermType == SPI_TERM_TYPE_FREQUENT ) {
            psplSrchPostingsList->uiTermType = SPI_TERM_TYPE_STOP;
            iSrchReportAppend(pssSrchSearch->pvSrchReport, " %d %d\n", REP_TERM_FREQUENT, REP_TERM_FREQUENT);
        }
        /* Regular terms */
        else if ( psplSrchPostingsList->uiTermType == SPI_TERM_TYPE_REGULAR ) {
            iSrchReportAppend(pssSrchSearch->pvSrchReport, " %u %u\n", psplSrchPostingsList->uiTermCount, psplSrchPostingsList->uiDocumentCount);
        }
        /* Unknown terms */
        else if ( psplSrchPostingsList->uiTermType == SPI_TERM_TYPE_UNKNOWN ) {
            iSrchReportAppend(pssSrchSearch->pvSrchReport, " %d %d\n", REP_TERM_NON_EXISTENT, REP_TERM_NON_EXISTENT);
        }
        /* Invalid terms */
        else {
            iSrchReportAppend(pssSrchSearch->pvSrchReport, " %d %d\n", REP_TERM_NON_EXISTENT, REP_TERM_NON_EXISTENT);
        }
    }


    /* Expanded terms */
    else if ( bTermExpanded == true ) {

        /* Search report */
        if ( psptSrchParserTerm->uiFunctionID != SRCH_PARSER_INVALID_ID ) {
            iSrchReportAppend(pssSrchSearch->pvSrchReport, "%s %ls[%ls]", REP_SEARCH_TERM_EXPANDED, 
                    pwcSrchParserGetStringFromID(pssSrchSearch->pvSrchParser, psptSrchParserTerm->uiFunctionID), psptSrchParserTerm->pwcTerm);
        }
        else {
            if ( psptSrchParserTerm->bWildCardSearch == true ) {
                iSrchReportAppend(pssSrchSearch->pvSrchReport, "%s %ls", REP_SEARCH_TERM_EXPANDED, psptSrchParserTerm->pwcTerm);
            }
            else {
                iSrchReportAppend(pssSrchSearch->pvSrchReport, "%s %ls%ls", REP_SEARCH_TERM_EXPANDED, 
                        pwcSrchParserGetStringFromID(pssSrchSearch->pvSrchParser, psptSrchParserTerm->uiRangeID), psptSrchParserTerm->pwcTerm);
            }
        }
        iSrchReportAppend(pssSrchSearch->pvSrchReport, " %ls", (bValidFieldName == true) ? psptSrchParserTerm->pwcFieldName : REP_UNFIELDED_WSTRING);
        iSrchReportAppend(pssSrchSearch->pvSrchReport, " %.2f", psptSrchParserTerm->fTermWeight);


        /* Create an empty postings list */
        if ( (iError = iSrchPostingCreateSrchPostingsList(SPI_TERM_TYPE_UNKNOWN, SPI_TERM_COUNT_UNKNOWN, SPI_TERM_DOCUMENT_COUNT_UNKNOWN, 
                false, NULL, 0, &psplSrchPostingsList)) != SRCH_NoError ) {
            goto bailFromiSrchSearchGetPostingsListFromParserTerm;
        }


        /* Loop through all the terms in the term list */
        for ( pstdiSrchTermDictInfosPtr = pstdiSrchTermDictInfos, uiI = 0, pvLngStemmer = NULL; uiI < uiSrchTermDictInfosLength; pstdiSrchTermDictInfosPtr++, uiI++ ) {

            wchar_t                     *pwcLocalTerm = NULL;
            struct srchPostingsList     *psplSrchPostingsListTmp = NULL;

            /* Convert the term from utf-8 to wide characters, pwcLocalTerm is allocated */
            if ( (iError = iLngConvertUtf8ToWideString_d(pstdiSrchTermDictInfosPtr->pucTerm, 0, &pwcLocalTerm)) != LNG_NoError ) {
                iUtlLogError(UTL_LOG_CONTEXT, "Failed to convert a term from utf-8 to wide characters, lng error: %d.", iError);
                iError = SRCH_SearchCharacterSetConvertionFailed;
                goto bailFromiSrchSearchGetPostingsListFromParserTerm;
            }

            /* Search for the term */
            iError = iSrchTermSearchGetSearchPostingsListFromTerm(pssSrchSearch, psiSrchIndex, pstdiSrchTermDictInfosPtr->pucTerm, 
                    fTermWeight, pucFieldIDBitmap, (pucFieldIDBitmap != NULL) ? psiSrchIndex->uiFieldIDMaximum : 0, fFrequentTermCoverageThreshold, 
                    uiStartDocumentID, uiEndDocumentID, &psplSrchPostingsListTmp);

            ASSERT(psplSrchPostingsListTmp != NULL);
            
            /* Free the wide character term */
            s_free(pwcLocalTerm);
        
            /* Check for non recoverable errors */
            if ( iError != SRCH_NoError ) {
                goto bailFromiSrchSearchGetPostingsListFromParserTerm;
            }

            /* Set the required flag */
            psplSrchPostingsListTmp->bRequired = psptSrchParserTerm->bRequired;

            iSrchReportAppend(pssSrchSearch->pvSrchReport, "\t%s", pstdiSrchTermDictInfosPtr->pucTerm);

            /* Turn stop terms into regular terms if this was a fielded search and stop terms are turned off the field, ie they were indexed */
            if ( (psplSrchPostingsListTmp->uiTermType == SPI_TERM_TYPE_STOP) && (pucFieldIDBitmap != NULL) && (bSrchInfoFieldOptionStopTerm(uiFieldOptions) == false) ) {
                psplSrchPostingsListTmp->uiTermType = SPI_TERM_TYPE_REGULAR;
                iSrchReportAppend(pssSrchSearch->pvSrchReport, " %u %u", psplSrchPostingsListTmp->uiTermCount, psplSrchPostingsListTmp->uiDocumentCount);
            }
            /* Stop terms */
            else if ( psplSrchPostingsListTmp->uiTermType == SPI_TERM_TYPE_STOP ) {
                iSrchReportAppend(pssSrchSearch->pvSrchReport, " %d %d", REP_TERM_STOP, REP_TERM_STOP);
            }
            /* Frequent terms, they get turned into stop terms */
            else if ( psplSrchPostingsListTmp->uiTermType == SPI_TERM_TYPE_FREQUENT ) {
                psplSrchPostingsListTmp->uiTermType = SPI_TERM_TYPE_STOP;
                iSrchReportAppend(pssSrchSearch->pvSrchReport, " %d %d", REP_TERM_FREQUENT, REP_TERM_FREQUENT);
            }
            /* Regular terms */
            else if ( psplSrchPostingsListTmp->uiTermType == SPI_TERM_TYPE_REGULAR ) {
                iSrchReportAppend(pssSrchSearch->pvSrchReport, " %u %u", psplSrchPostingsListTmp->uiTermCount, psplSrchPostingsListTmp->uiDocumentCount);
            }
            /* Unknown terms */
            else if ( psplSrchPostingsListTmp->uiTermType == SPI_TERM_TYPE_UNKNOWN ) {
                iSrchReportAppend(pssSrchSearch->pvSrchReport, " %d %d", REP_TERM_NON_EXISTENT, REP_TERM_NON_EXISTENT);
            }
            /* Invalid terms */
            else {
                iSrchReportAppend(pssSrchSearch->pvSrchReport, " %d %d", REP_TERM_NON_EXISTENT, REP_TERM_NON_EXISTENT);
            }
    

            /* Just hand over the data and pointer from the temp postings if the postings is not yet populated */
            if ( psplSrchPostingsList->pspSrchPostings == NULL ) {
            
                /* Hand over the pointers */
                psplSrchPostingsList->uiTermType = psplSrchPostingsListTmp->uiTermType;
                psplSrchPostingsList->uiTermCount = psplSrchPostingsListTmp->uiTermCount;
                psplSrchPostingsList->uiDocumentCount = psplSrchPostingsListTmp->uiDocumentCount;
                psplSrchPostingsList->pspSrchPostings = psplSrchPostingsListTmp->pspSrchPostings;
                psplSrchPostingsList->uiSrchPostingsLength = psplSrchPostingsListTmp->uiSrchPostingsLength;
            }
            else {
                
                /* Append the new postings to the existing postings array */
                if ( (psplSrchPostingsListTmp->uiSrchPostingsLength > 0) && (psplSrchPostingsListTmp->pspSrchPostings != NULL) ) {

                    struct srchPosting      *pspSrchPostingsPtr = NULL;

                    /* Extend the existing postings array */
                    if ( (pspSrchPostingsPtr = (struct srchPosting *)s_realloc(psplSrchPostingsList->pspSrchPostings, 
                            (size_t)(sizeof(struct srchPosting) * (psplSrchPostingsList->uiSrchPostingsLength + psplSrchPostingsListTmp->uiSrchPostingsLength)))) == NULL ) {
                        iError = SRCH_MemError;
                        goto bailFromiSrchSearchGetPostingsListFromParserTerm;
                    }

                    /* Hand over the pointer */
                    psplSrchPostingsList->pspSrchPostings = pspSrchPostingsPtr;

                    /* Copy the new postings to the newly extended area */
                    s_memcpy(psplSrchPostingsList->pspSrchPostings + psplSrchPostingsList->uiSrchPostingsLength, psplSrchPostingsListTmp->pspSrchPostings,
                        (sizeof(struct srchPosting) * psplSrchPostingsListTmp->uiSrchPostingsLength));

                    /* Increment the lengths, the doc count is now off, but gets recalculated below */
                    psplSrchPostingsList->uiTermCount += psplSrchPostingsListTmp->uiSrchPostingsLength;
                    psplSrchPostingsList->uiDocumentCount += psplSrchPostingsListTmp->uiDocumentCount;
                    psplSrchPostingsList->uiSrchPostingsLength += psplSrchPostingsListTmp->uiSrchPostingsLength;
                }

                /* Free the temp postings list postings array */
                s_free(psplSrchPostingsListTmp->pspSrchPostings);
            }
            
            /* Free the temp postings list */
            s_free(psplSrchPostingsListTmp);
        }


        /* Post-process the postings if needed */
        if ( (psplSrchPostingsList->pspSrchPostings != NULL) && (uiSrchTermDictInfosLength > 1) ) {

            struct srchPosting      *pspSrchPostingsPtr = NULL;
            struct srchPosting      *pspSrchPostingsEnd = NULL;
            unsigned int            uiDocumentID = 0;

            /* Sort the postings */
            iSrchPostingSortDocumentIDAsc(psplSrchPostingsList->pspSrchPostings, 0, psplSrchPostingsList->uiSrchPostingsLength - 1);

            /* Count up the number of documents in this postings array and set the value in the search postings list structure */
            for ( pspSrchPostingsPtr = psplSrchPostingsList->pspSrchPostings, pspSrchPostingsEnd = psplSrchPostingsList->pspSrchPostings + psplSrchPostingsList->uiSrchPostingsLength, 
                    psplSrchPostingsList->uiDocumentCount = 0, uiDocumentID = 0; pspSrchPostingsPtr < pspSrchPostingsEnd; pspSrchPostingsPtr++ ) {

                if ( pspSrchPostingsPtr->uiDocumentID != uiDocumentID ) {
                    psplSrchPostingsList->uiDocumentCount++;
                    uiDocumentID = pspSrchPostingsPtr->uiDocumentID;
                }
            }
        }

        /* Search report */
        iSrchReportAppend(pssSrchSearch->pvSrchReport, "\t%u %u\n", psplSrchPostingsList->uiTermCount, psplSrchPostingsList->uiDocumentCount);
    }



    
    /*=======================================
    **
    ** Save the search in the cache
    **
    */

    /* Save the search in the cache */
    if ( (uiSrchParserSearchCacheID == SRCH_PARSER_MODIFIER_SEARCH_CACHE_ENABLE_ID) && (bCachableSearch == true) ) {

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
        iSrchCacheSaveSearchPostingsList(pssSrchSearch->pvSrchCache, psiSrchIndex, uiLanguageID, psptSrchParserTerm, psplSrchPostingsList, pucSearchReportSnippet);
    }
    

    

    /* Bail label */
    bailFromiSrchSearchGetPostingsListFromParserTerm:


    /* Free the field IDs bit map */
    s_free(pucFieldIDBitmap);

    /* Free the search report snippet */
    s_free(pucSearchReportSnippet);

    /* Free the term dict information structure */
    if ( pstdiSrchTermDictInfos != NULL ) {
        for ( pstdiSrchTermDictInfosPtr = pstdiSrchTermDictInfos, uiI = 0; uiI < uiSrchTermDictInfosLength; pstdiSrchTermDictInfosPtr++, uiI++ ) {
            s_free(pstdiSrchTermDictInfosPtr->pucTerm);
        }
        s_free(pstdiSrchTermDictInfos);
    }

    /* Free the term */
    s_free(pucTerm);

    /* Free the stemmed term */
    s_free(pwcTermStemmed);

    /* Free the stemmed term */
    s_free(pucTermStemmed);

    /* Free the lower case term */
    s_free(pwcTermLowerCase);

    /* Free the lower case term */
    s_free(pucTermLowerCase);

    /* Free the stemmer */
    iLngStemmerFree(pvLngStemmer);
    pvLngStemmer = NULL;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "pucTerm: '%s', iError: %d, psplSrchPostingsList %s NULL",  */
/*             pucTerm, iError, (psplSrchPostingsList != NULL) ? "!=" : "="); */


    /* Handle the error */
    if ( iError == SRCH_NoError ) {

        /* Set the return pointer */
        *ppsplSrchPostingsList = psplSrchPostingsList;        
    }
    else {

        /* Free the search postings list */
        iSrchPostingFreeSrchPostingsList(psplSrchPostingsList);
        psplSrchPostingsList = NULL;
    }


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchSearchFilterNumberAgainstSrchParserNumbers()

    Purpose:    This function filters a number based on the passed 
                search parser number restrictions.
                
    Parameters: ulNumber                    number to filter
                pspnSrchParserNumbers       search parser numbers structure
                uiSrchParserNumbersLength   search parser numbers structure length

    Globals:    none

    Returns:    SRCH error code, SRCH_NoError if the number passes through the 
                restrictions, SRCH_MiscError does not pass through the restrictions

*/
static int iSrchSearchFilterNumberAgainstSrchParserNumbers
(
    unsigned long ulNumber,
    struct srchParserNumber *pspnSrchParserNumbers,
    unsigned int uiSrchParserNumbersLength
)
{

    struct srchParserNumber     *pspnSrchParserNumbersPtr = NULL;
    unsigned int                uiI = 0;
    boolean                     bPassedRestrictions = false;


    ASSERT(((pspnSrchParserNumbers != NULL) && (uiSrchParserNumbersLength > 0)) ||
            ((pspnSrchParserNumbers == NULL) && (uiSrchParserNumbersLength == 0)));


    /* Check that filters were passed at all */
    if ( pspnSrchParserNumbers == NULL ) {
        return (SRCH_NoError);
    }

        
    /* Loop over each search parser number */
    for ( uiI = 0, pspnSrchParserNumbersPtr = pspnSrchParserNumbers; uiI < uiSrchParserNumbersLength; uiI++, pspnSrchParserNumbersPtr++ ) {
        
        /* Check to see whether the number passes the restrictions or not */
        switch ( pspnSrchParserNumbersPtr->uiRangeID ) {
        
            case SRCH_PARSER_RANGE_EQUAL_ID:
                bPassedRestrictions = ( ulNumber == pspnSrchParserNumbersPtr->ulNumber ) ? true : false;
                break;
        
            case SRCH_PARSER_RANGE_NOT_EQUAL_ID:
                bPassedRestrictions = ( ulNumber != pspnSrchParserNumbersPtr->ulNumber ) ? true : false;
                break;
        
            case SRCH_PARSER_RANGE_GREATER_ID:
                bPassedRestrictions = ( ulNumber > pspnSrchParserNumbersPtr->ulNumber ) ? true : false;
                break;

            case SRCH_PARSER_RANGE_LESS_ID:
                bPassedRestrictions = ( ulNumber < pspnSrchParserNumbersPtr->ulNumber ) ? true : false;
                break;
        
            case SRCH_PARSER_RANGE_GREATER_OR_EQUAL_ID:
                bPassedRestrictions = ( ulNumber >= pspnSrchParserNumbersPtr->ulNumber ) ? true : false;
                break;
        
            case SRCH_PARSER_RANGE_LESS_OR_EQUAL_ID:
                bPassedRestrictions = ( ulNumber <= pspnSrchParserNumbersPtr->ulNumber ) ? true : false;
                break;
                
            default:
                break;
        }
        
        /* Break out of the for() loop if the number failed to pass the restrictions */
        if ( bPassedRestrictions == false ) {
            break;
        }
    }


    /* Return the error code according to whether we passed the restrictions or not */
    return ((bPassedRestrictions == true) ? SRCH_NoError : SRCH_MiscError);
    
}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchSearchAddEntryToSpiDocumentItems()

    Purpose:    Add a document item/mime type to the SPI document items

    Parameters: pucItemName                 item name
                pucMimeType                 mime type
                pucUrl                      document URL
                uiLength                    length
                pvData                      data
                uiDataLength                data length
                bCopyData                   copy the data
                ppsdiSpiDocumentItems       array of spi document item structures
                puiDocumentItemsLength      number of entries in the array of spi document item structures

    Globals:    none

    Returns:    a pointer to the psdiSpiDocumentItems structure, NULL on error

*/
static int iSrchSearchAddEntryToSpiDocumentItems
(
    unsigned char *pucItemName,
    unsigned char *pucMimeType,
    unsigned char *pucUrl,
    unsigned int uiLength,
    void *pvData,
    unsigned int uiDataLength,
    boolean bCopyData,
    struct spiDocumentItem **ppsdiSpiDocumentItems,
    unsigned int *puiDocumentItemsLength
)
{

    int                         iError = SRCH_NoError;
    
    struct spiDocumentItem      *psdiSpiDocumentItems = NULL;
    unsigned int                uiDocumentItemsLength = 0;
    
    struct spiDocumentItem      *psdiSpiDocumentItemsPtr = NULL;

    struct spiDocumentItem      *psdiSpiDocumentItemPtr = NULL;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "psdiSrchSearchAddEntryToSpiDocumentItems - pucItemName [%s], pucMimeType [%s], pucUrl [%s], uiLength [%u], uiDataLength [%u]", */
/*             pucItemName, pucMimeType,  pucUtlStringsGetPrintableString(pucUrl), uiLength, uiDataLength); */


    ASSERT(bUtlStringsIsStringNULL(pucItemName) == false);
    ASSERT(bUtlStringsIsStringNULL(pucMimeType) == false);
    ASSERT(((pvData != NULL) && (uiDataLength != 0)) || ((pvData == NULL) && (uiDataLength == 0)));
    ASSERT(ppsdiSpiDocumentItems != NULL);
    ASSERT(puiDocumentItemsLength != NULL);


    /* Dereference the document items array and document items array length parameters */
    psdiSpiDocumentItems = *ppsdiSpiDocumentItems;
    uiDocumentItemsLength = *puiDocumentItemsLength;
    
    if ( ((psdiSpiDocumentItems == NULL) && (uiDocumentItemsLength != 0)) || ((psdiSpiDocumentItems != NULL) && (uiDocumentItemsLength == 0)) ) {
        return (SRCH_MiscError);
    }



    /* Check to see if the document item/mime type has already been specified */
    if ( psdiSpiDocumentItems != NULL ) {

        unsigned int    uiI = 0;

        /* Check to see if the document item has already been specified */
        for ( uiI = 0, psdiSpiDocumentItemPtr = NULL, psdiSpiDocumentItemsPtr = psdiSpiDocumentItems; uiI < uiDocumentItemsLength; uiI++, psdiSpiDocumentItemsPtr++ ) {
            
            /* Return if this item/mime type has already been specified */
            if ( (s_strcmp(pucItemName, psdiSpiDocumentItemsPtr->pucItemName) == 0) && (s_strcmp(pucMimeType, psdiSpiDocumentItemsPtr->pucMimeType) == 0) ) {
                iError = SRCH_NoError;
                goto bailFromiSrchSearchAddEntryToSpiDocumentItems;
            }
        }
    }



    /* Create a new document item */
    if ( (psdiSpiDocumentItemsPtr = (struct spiDocumentItem *)s_realloc(psdiSpiDocumentItems, (size_t)(sizeof(struct spiDocumentItem) * (uiDocumentItemsLength + 1)))) == NULL ) {
        iError = SRCH_MemError;
        goto bailFromiSrchSearchAddEntryToSpiDocumentItems;
    }
    
    /* Hand over the newly allocated pointer */
    psdiSpiDocumentItems = psdiSpiDocumentItemsPtr;
    
    /* Increment the document items length */
    uiDocumentItemsLength++;

    /* Dereference the newly added document item */
    psdiSpiDocumentItemPtr = (psdiSpiDocumentItems + (uiDocumentItemsLength - 1));

    /* Make a copy of the item name */
    if ( (psdiSpiDocumentItemPtr->pucItemName = (unsigned char *)s_strdup(pucItemName)) == NULL ) {
        iError = SRCH_MemError;
        goto bailFromiSrchSearchAddEntryToSpiDocumentItems;
    }

    /* Make a copy of the mime type */
    if ( (psdiSpiDocumentItemPtr->pucMimeType = (unsigned char *)s_strdup(pucMimeType)) == NULL ) {
        iError = SRCH_MemError;
        goto bailFromiSrchSearchAddEntryToSpiDocumentItems;
    }

    /* Make a copy of the URL, if there is one */
    if ( bUtlStringsIsStringNULL(pucUrl) == false ) {
        if ( (psdiSpiDocumentItemPtr->pucUrl = (unsigned char *)s_strdup(pucUrl)) == NULL ) {
            iError = SRCH_MemError;
            goto bailFromiSrchSearchAddEntryToSpiDocumentItems;
        }
    }
    /* Otherwise just clear the pointer */
    else {
        psdiSpiDocumentItemPtr->pucUrl = NULL;
    }


    /* Set the length */
    psdiSpiDocumentItemPtr->uiLength = uiLength;


    /* Copy/add the data if there is any */
    if ( pvData != NULL ) {
        
        /* Make a copy of the data */
        if ( bCopyData == true ) {
            
            /* Allocate space for the data */
            if ( (psdiSpiDocumentItemPtr->pvData = (void *)s_malloc((size_t)uiDataLength)) == NULL ) {
                iError = SRCH_MemError;
                goto bailFromiSrchSearchAddEntryToSpiDocumentItems;
            }
                
            s_memcpy(psdiSpiDocumentItemPtr->pvData, pvData, uiDataLength);
        }
        
        /* Otherwise just hand over the pointer, and clear it */
        else {
            psdiSpiDocumentItemPtr->pvData = pvData;
            pvData = NULL;
        }
        
        /* Set the data length */
        psdiSpiDocumentItemPtr->uiDataLength = uiDataLength;
    }
    /* Otherwise just clear the pointer */
    else {
        psdiSpiDocumentItemPtr->pvData = NULL;
        psdiSpiDocumentItemPtr->uiDataLength = 0;
    }



    /* Bail label */
    bailFromiSrchSearchAddEntryToSpiDocumentItems:


    /* Handle the error */
    if ( iError == SRCH_NoError ) {
        
        /* Free the data pointer if is still set and we were not to copy it */ 
        if ( bCopyData == false ) {
            s_free(pvData);
        }
    }
    else {
    
        /* Clear the pointer */
        s_memset((psdiSpiDocumentItems + (uiDocumentItemsLength - 1)), 0, sizeof(struct spiDocumentItem));

        /* Free the document item */
        s_free(psdiSpiDocumentItemPtr->pucItemName);
        s_free(psdiSpiDocumentItemPtr->pucMimeType);
        s_free(psdiSpiDocumentItemPtr->pucUrl);
        s_free(psdiSpiDocumentItemPtr->pvData);
        s_free(psdiSpiDocumentItemPtr);
        
        /* Decrement the document items array length */
        uiDocumentItemsLength--;
    }


    /* Set the return pointers */
    *ppsdiSpiDocumentItems = psdiSpiDocumentItems;
    *puiDocumentItemsLength = uiDocumentItemsLength;


    return (SRCH_NoError);

}


/*---------------------------------------------------------------------------*/
