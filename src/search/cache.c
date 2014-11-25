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

    Module:     cache.c

    Author:     Francois Schiettecatte (FS Consulting LLC.)

    Created:    20 October 1999

    Purpose:    This module implements access to the search cache
                functions. 

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
#define UTL_LOG_CONTEXT                     (unsigned char *)"com.fsconsult.mps.src.search.cache"


/* Enable search short results caching */
#define SRCH_CACHE_ENABLE_SEARCH_SHORT_RESULTS_CACHING

/* Enable search postings caching */
#define SRCH_CACHE_ENABLE_SEARCH_POSTINGS_CACHING

/* Enable search weights caching */
#define SRCH_CACHE_ENABLE_SEARCH_WEIGHTS_CACHING

/* Enable search bitmaps caching */
#define SRCH_CACHE_ENABLE_SEARCH_BITMAPS_CACHING


/* Enable this to use utime() to update the cache file access time and update time,
** normally this in only needed on file systems which have 'noatime' set, but it
** is a very expensive operation
*/
/* #define SRCH_CACHE_ENABLE_UTIME_CACHE_FILE_UPDATE */


/* Enable all cache logging */
/* #define SRCH_CACHE_ENABLE_ALL_CACHE_LOGGING  */


/* Enable search short results cache logging */
/* #define SRCH_CACHE_ENABLE_SEARCH_SHORT_RESULTS_CACHE_LOGGING  */

/* Enable search postings list cache logging */
/* #define SRCH_CACHE_ENABLE_SEARCH_POSTINGS_LIST_CACHE_LOGGING  */

/* Enable search weight cache logging */
/* #define SRCH_CACHE_ENABLE_SEARCH_WEIGHT_CACHE_LOGGING  */

/* Enable search bitmap cache logging */
/* #define SRCH_CACHE_ENABLE_SEARCH_BITMAP_CACHE_LOGGING  */


/*---------------------------------------------------------------------------*/


/*
** Defines
*/

/* Search cache type */
#define SRCH_CACHE_TYPE_UNKNOWN                     (0)
#define SRCH_CACHE_TYPE_FILE                        (1)

#define SRCH_CACHE_TYPE_VALID(n)                    (((n) >= SRCH_CACHE_TYPE_FILE) && \
                                                            ((n) <= SRCH_CACHE_TYPE_FILE))


/* Search cache file protocol url */
#define SRCH_CACHE_FILE_PROTOCOL_URL                (unsigned char *)"file://"


/* Search cache file name extension */
#define SRCH_CACHE_FILENAME_EXT                     (unsigned char *)".cah"


/* Search cache index name symbol and last update time symbol for the search cache subdirectory mask */
#define SRCH_CACHE_INDEX_NAME_SYMBOL                (unsigned char *)"{Index}"
#define SRCH_CACHE_LAST_UPDATE_TIME_SYMBOL          (unsigned char *)"{LastUpdateTime}"


/* Search cache mode */
#define SRCH_CACHE_MODE_OFF                         (0)
#define SRCH_CACHE_MODE_READ_ONLY                   (1)
#define SRCH_CACHE_MODE_READ_WRITE                  (2)

#define SRCH_CACHE_MODE_OFF_STRING                  (unsigned char *)"off"
#define SRCH_CACHE_MODE_READ_ONLY_STRING            (unsigned char *)"read-only"
#define SRCH_CACHE_MODE_READ_WRITE_STRING           (unsigned char *)"read-write"


/* Search cache tags */
#define SRCH_CACHE_MAX_WEIGHT_TAG                   'W'
#define SRCH_CACHE_TOTAL_RESULTS_TAG                'S'
#define SRCH_CACHE_SHORT_RESULTS_ARRAY_TAG          'H'
#define SRCH_CACHE_SEARCH_TEXT_TAG                  'T'
#define SRCH_CACHE_POSITIVE_FEEDBACK_TEXT_TAG       'P'
#define SRCH_CACHE_NEGATIVE_FEEDBACK_TEXT_TAG       'N'

#define SRCH_CACHE_TERM_TYPE_TAG                    'G'
#define SRCH_CACHE_TERM_COUNT_TAG                   'E'
#define SRCH_CACHE_DOCUMENT_COUNT_TAG               'Y'
#define SRCH_CACHE_REQUIRE_TAG                      'U'
#define SRCH_CACHE_POSTINGS_ARRAY_TAG               'O'
#define SRCH_CACHE_TERM_TAG                         'A'
#define SRCH_CACHE_FIELD_NAME_TAG                   'C'

#define SRCH_CACHE_WEIGHTS_ARRAY_LENGTH_TAG         'Z'
#define SRCH_CACHE_WEIGHTS_ARRAY_TAG                'F'

#define SRCH_CACHE_SEACH_REPORT_SNIPPET_TAG         'R'
#define SRCH_CACHE_INDEX_PATH_TAG                   'D'
#define SRCH_CACHE_LAST_UPDATE_TIME_TAG             'L'


/* Lock levels */
#define SRCH_CACHE_LOCK_INVALID                     (0)         /* Invalid lock */
#define SRCH_CACHE_LOCK_SHARED                      (1)         /* Shared lock */
#define SRCH_CACHE_LOCK_EXCLUSIVE                   (2)         /* Exclusive lock */

#define SRCH_CACHE_LOCK_VALID(n)                    (((n) >= SRCH_CACHE_LOCK_SHARED) && \
                                                            ((n) <= SRCH_CACHE_LOCK_EXCLUSIVE))


/* Shared lock timeout in microseconds */
#define SRCH_CACHE_SHARED_LOCK_SLEEP                (0)
#define SRCH_CACHE_SHARED_LOCK_TIMEOUT              (0)

/* Exclusive lock timeout in microseconds */
#define SRCH_CACHE_EXCLUSIVE_LOCK_SLEEP             (100)
#define SRCH_CACHE_EXCLUSIVE_LOCK_TIMEOUT           (500)


/*---------------------------------------------------------------------------*/


/*
** Structures
*/

/* Search cache structure */
struct srchCache {
    unsigned int    uiSearchCacheType;                                          /* Search cache type */
    unsigned int    uiSearchCacheMode;                                          /* Search cache mode */
    unsigned char   pucSearchCacheDirectoryPath[UTL_FILE_PATH_MAX + 1];         /* Search cache directory path */
    unsigned char   pucSearchCacheSubDirectoryMask[UTL_FILE_PATH_MAX + 1];      /* Search cache subdirectory mask */
};


/*---------------------------------------------------------------------------*/


/* 
** Private function prototypes
*/

static int iSrchCacheGetSearchShortResultsSHA1HexDigest (struct srchIndex *psiSrchIndex, wchar_t *pwcSearchText, 
        wchar_t *pwcPositiveFeedbackText, wchar_t *pwcNegativeFeedbackText, unsigned char *pucSHA1HexDigest);

static int iSrchCacheGetSearchPostingsListSHA1HexDigest (struct srchIndex *psiSrchIndex, unsigned int uiLanguageID, 
        struct srchParserTerm *psptSrchParserTerm, unsigned char *pucSHA1HexDigest);

static int iSrchCacheGetSearchWeightSHA1HexDigest (struct srchIndex *psiSrchIndex, wchar_t *pwcWeightsName, 
        unsigned char *pucSHA1HexDigest);

static int iSrchCacheGetSearchBitmapSHA1HexDigest (struct srchIndex *psiSrchIndex, wchar_t *pwcBitmapName, 
        time_t tBitmapLastUpdate, unsigned char *pucSHA1HexDigest);


static int iSrchCacheGetCacheFilePath (struct srchCache *pscSrchCache, struct srchIndex *psiSrchIndex,
        unsigned char *pucSHA1HexDigest, unsigned char *pucSearchCacheFilePath, unsigned int uiSearchCacheFilePathLength);


static int iSrchCacheLockCacheFile (FILE *pfSearchCacheFile, unsigned int uiLockType);

static int iSrchCacheUnlockCacheFile (FILE *pfSearchCacheFile);


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchCacheCreate()

    Purpose:    This function initializes the search cache support information,
                basically it allows us to set some globals from the server
                configuration file up front so that we dont need to re-read 
                them all the time. Also we check that we can actually write in
                the directory specified in the server configuration file.

    Parameters: pssSrchSearch   search structure
                ppvSrchCache    return pointer for the search cache structure

    Globals:    none

    Returns:    SRCH error code

*/
int iSrchCacheCreate
(
    struct srchSearch *pssSrchSearch,
    void **ppvSrchCache
)
{

    int                 iError = SRCH_NoError;
    struct srchCache    *pscSrchCache = NULL;
    unsigned char       pucSearchCacheLocation[UTL_CONFIG_VALUE_MAXIMUM_LENGTH + 1] = {'\0'};
    unsigned char       pucSearchCacheMode[UTL_CONFIG_VALUE_MAXIMUM_LENGTH + 1] = {'\0'};


    /* Check the parameters */
    if ( pssSrchSearch == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pssSrchSearch' parameter passed to 'iSrchCacheCreate'."); 
        return (SRCH_CacheInvalidSearch);
    }

    if ( ppvSrchCache == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppvSrchCache' parameter passed to 'iSrchCacheCreate'."); 
        return (SRCH_ReturnParameterError);
    }


    /* Allocate a search cache */
    if ( (pscSrchCache = (struct srchCache *)s_malloc((size_t)(sizeof(struct srchCache)))) == NULL ) {
        return (SRCH_MemError);
    } 

    /* Default cache type is unknown */
    pscSrchCache->uiSearchCacheType = SRCH_CACHE_TYPE_UNKNOWN;

    /* Default cache mode is off */
    pscSrchCache->uiSearchCacheMode = SRCH_CACHE_MODE_OFF;


    /* Get the search cache location */
    iError = iUtlConfigGetValue(pssSrchSearch->pvUtlConfig, SRCH_SEARCH_CONFIG_SEARCH_CACHE_LOCATION, pucSearchCacheLocation, UTL_CONFIG_VALUE_MAXIMUM_LENGTH + 1);
    
    /* Report no error if the symbol was not found, it just wasn't defined */
    if ( iError == UTL_ConfigSymbolNotFound ) {
        iError = SRCH_NoError;
        goto bailFromiSrchCacheCreate;
    }
    /* Otherwise report the error if we got one */
    else if ( iError != UTL_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the search cache location from the search configuration, symbol name: '%s', utl error: %d.", 
                SRCH_SEARCH_CONFIG_SEARCH_CACHE_LOCATION, iError); 
        iError = SRCH_CacheCreateFailed;
        goto bailFromiSrchCacheCreate;
    }


    /* Check for file system root ('/') or file protocol url */
    if ( (pucSearchCacheLocation[0] == UTL_FILE_DIRECTORY_ROOT_CHAR) ||
            (s_strncasecmp(pucSearchCacheLocation, SRCH_CACHE_FILE_PROTOCOL_URL, s_strlen(SRCH_CACHE_FILE_PROTOCOL_URL)) == 0) ) {

        unsigned char   *pucSearchCacheLocationPtr = NULL;
        unsigned char   pucSearchCacheSubDirectoryMask[UTL_CONFIG_VALUE_MAXIMUM_LENGTH + 1] = {'\0'};

        /* Set the search cache type */
        pscSrchCache->uiSearchCacheType = SRCH_CACHE_TYPE_FILE;

        /* Get a pointer to the search cache location path, exclude the file protocol url if it is there */
        pucSearchCacheLocationPtr = (s_strncasecmp(pucSearchCacheLocation, SRCH_CACHE_FILE_PROTOCOL_URL, s_strlen(SRCH_CACHE_FILE_PROTOCOL_URL)) == 0) ? 
                pucSearchCacheLocation + s_strlen(SRCH_CACHE_FILE_PROTOCOL_URL) : pucSearchCacheLocation;

        /* Clean the search cache location path */
        if ( (iError = iUtlFileCleanPath(pucSearchCacheLocationPtr)) != UTL_NoError ) {
            iUtlLogWarn(UTL_LOG_CONTEXT, "Failed to clean the search cache directory path: '%s', utl error: %d.", pucSearchCacheLocationPtr, iError);
            pscSrchCache->uiSearchCacheMode = SRCH_CACHE_MODE_OFF;
        }

        /* Check that the search cache location really exists and that we can actually write to it */
        if ( bUtlFileIsDirectory(pucSearchCacheLocationPtr) == true ) {
        
            boolean bSearchCacheDirectoryRead = false;
            boolean bSearchCacheDirectoryWrite = false;

            /* Copy the cleaned search cache location to the search cache directory path, we now know it is a directory */
            s_strnncpy(pscSrchCache->pucSearchCacheDirectoryPath, pucSearchCacheLocationPtr, UTL_FILE_PATH_MAX + 1);
            
            /* Get the path read and write states */
            bSearchCacheDirectoryRead = bUtlFilePathRead(pscSrchCache->pucSearchCacheDirectoryPath);
            bSearchCacheDirectoryWrite = bUtlFilePathWrite(pscSrchCache->pucSearchCacheDirectoryPath);

            /* Check to see if we can read from and write to the search cache directory */
            if ( (bSearchCacheDirectoryRead == true) && (bSearchCacheDirectoryWrite == true) ) {
                pscSrchCache->uiSearchCacheMode = SRCH_CACHE_MODE_READ_WRITE;
            }
            /* Check to see if we can read from the search cache directory */
            else if ( bSearchCacheDirectoryRead == true  ) {
                pscSrchCache->uiSearchCacheMode = SRCH_CACHE_MODE_READ_ONLY;
            }
            
            /* Log the access level if we dont have full access */    
            if ( pscSrchCache->uiSearchCacheMode == SRCH_CACHE_MODE_OFF ) {
                iUtlLogWarn(UTL_LOG_CONTEXT, "Failed to access the search cache directory: '%s'.", pscSrchCache->pucSearchCacheDirectoryPath);
            }
            else if ( pscSrchCache->uiSearchCacheMode == SRCH_CACHE_MODE_READ_ONLY ) {
                iUtlLogInfo(UTL_LOG_CONTEXT, "Read-only access to the search cache directory: '%s'.", pscSrchCache->pucSearchCacheDirectoryPath);
            }

            /* Set the default search cache subdirectory mask */
            pscSrchCache->pucSearchCacheSubDirectoryMask[0] = '\0';
    
            /* Get the search cache subdirectory mask from the configuration file */
            if ( iUtlConfigGetValue(pssSrchSearch->pvUtlConfig, SRCH_SEARCH_CONFIG_SEARCH_CACHE_SUBDIRECTORY_MASK, pucSearchCacheSubDirectoryMask, UTL_CONFIG_VALUE_MAXIMUM_LENGTH + 1) == UTL_NoError ) {
                s_strnncpy(pscSrchCache->pucSearchCacheSubDirectoryMask, pucSearchCacheSubDirectoryMask, UTL_FILE_PATH_MAX + 1);
            }
        }
        else {
            iUtlLogWarn(UTL_LOG_CONTEXT, "Failed to find the search cache directory: '%s'.", pucSearchCacheLocationPtr);
            pscSrchCache->uiSearchCacheMode = SRCH_CACHE_MODE_OFF;
        }
    }
    
    /* Failed to identify the search cache location */
    else {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to identify the seach cache location from the search configuration, symbol name: '%s', symbol: '%s'.", 
                SRCH_SEARCH_CONFIG_SEARCH_CACHE_LOCATION, pucSearchCacheLocation); 
        iError = SRCH_CacheCreateFailed;
        goto bailFromiSrchCacheCreate;
    }


    /* At this point the search cache type should be set and the search cache mode should be set to a sensible default */


    /* Get the search cache mode from the configuration file */
    if ( iUtlConfigGetValue(pssSrchSearch->pvUtlConfig, SRCH_SEARCH_CONFIG_SEARCH_CACHE_MODE, pucSearchCacheMode, UTL_CONFIG_VALUE_MAXIMUM_LENGTH + 1) == UTL_NoError ) {
        
        unsigned int    uiSearchCacheMode = SRCH_CACHE_MODE_OFF;
        
        /* Set the cache mode */
        if ( s_strcasecmp(pucSearchCacheMode, SRCH_CACHE_MODE_OFF_STRING) == 0 ) {
            uiSearchCacheMode = SRCH_CACHE_MODE_OFF;
        }
        else if ( s_strcasecmp(pucSearchCacheMode, SRCH_CACHE_MODE_READ_ONLY_STRING) == 0 ) {
            uiSearchCacheMode = SRCH_CACHE_MODE_READ_ONLY;
        }
        else if ( s_strcasecmp(pucSearchCacheMode, SRCH_CACHE_MODE_READ_WRITE_STRING) == 0 ) {
            uiSearchCacheMode = SRCH_CACHE_MODE_READ_WRITE;
        }
        else {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to identify the seach cache mode from the search configuration, symbol name: '%s', symbol: '%s'.", 
                    pucSearchCacheMode, pucSearchCacheMode); 
            iError = SRCH_CacheCreateFailed;
            goto bailFromiSrchCacheCreate;
        }
        

        /* Set the cache mode to off */
        if ( uiSearchCacheMode == SRCH_CACHE_MODE_OFF ) {
            pscSrchCache->uiSearchCacheMode = SRCH_CACHE_MODE_OFF;
        }
        /* Set the cache mode to read-only, warn if already off */
        else if ( uiSearchCacheMode == SRCH_CACHE_MODE_READ_ONLY ) {
            if ( pscSrchCache->uiSearchCacheMode == SRCH_CACHE_MODE_OFF ) {
                iUtlLogWarn(UTL_LOG_CONTEXT, "Search cache is currently '%s', cannot set to '%s', symbol name: '%s''.", 
                        SRCH_CACHE_MODE_OFF_STRING, SRCH_CACHE_MODE_READ_ONLY_STRING, SRCH_SEARCH_CONFIG_SEARCH_CACHE_MODE);
            }
            else {
                pscSrchCache->uiSearchCacheMode = SRCH_CACHE_MODE_READ_ONLY;
            }
        }
        /* Set the cache mode to read-write, warn if already off or read-write */
        else if ( uiSearchCacheMode == SRCH_CACHE_MODE_READ_WRITE ) {
            if ( pscSrchCache->uiSearchCacheMode == SRCH_CACHE_MODE_OFF ) {
                iUtlLogWarn(UTL_LOG_CONTEXT, "Search cache is currently '%s', cannot set to '%s', symbol name: '%s'.", 
                        SRCH_CACHE_MODE_OFF_STRING, SRCH_CACHE_MODE_READ_WRITE_STRING, SRCH_SEARCH_CONFIG_SEARCH_CACHE_MODE);
            }
            else if ( pscSrchCache->uiSearchCacheMode == SRCH_CACHE_MODE_READ_ONLY ) {
                iUtlLogWarn(UTL_LOG_CONTEXT, "Search cache is currently '%s', cannot set to '%s', symbol name: '%s'.", 
                        SRCH_CACHE_MODE_OFF_STRING, SRCH_CACHE_MODE_READ_WRITE_STRING, SRCH_SEARCH_CONFIG_SEARCH_CACHE_MODE);
            }
            else {
                pscSrchCache->uiSearchCacheMode = SRCH_CACHE_MODE_READ_WRITE;
            }
        }
    }



    /* Bail label */
    bailFromiSrchCacheCreate:


    /* Handle the error */
    if ( iError != SRCH_NoError ) {

        /* Free resources */
        iSrchCacheClose((void *)pscSrchCache);
        pscSrchCache = NULL;
    }
    else {

        /* Set the return pointer */
        *ppvSrchCache = pscSrchCache;
    }
    


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchCacheClose()

    Purpose:    This function frees the search cache structure.

    Parameters: pvSrchCache     search cache structure

    Globals:    none

    Returns:    SRCH error code

*/
int iSrchCacheClose
(
    void *pvSrchCache
)
{

    struct srchCache    *pscSrchCache = (struct srchCache *)pvSrchCache;


    /* Check the parameters */
    if ( pvSrchCache == NULL ) {
        iUtlLogDebug(UTL_LOG_CONTEXT, "Null 'pvSrchCache' parameter passed to 'iSrchCacheClose'."); 
        return (SRCH_CacheInvalidCache);
    }


    /* File type */
    if ( pscSrchCache->uiSearchCacheType == SRCH_CACHE_TYPE_FILE ) {
        ;
    }

    /* Unknown type */
    else if ( pscSrchCache->uiSearchCacheType == SRCH_CACHE_TYPE_UNKNOWN ) {
        ;
    }

    
    /* Free the search cache structure */
    s_free(pscSrchCache);


    return (SRCH_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchCacheSaveSearchShortResults()

    Purpose:    This function saves a search short result structure array in the cache.

    Parameters: pvSrchCache                 search cache structure
                psiSrchIndex                search index structure
                pwcSearchText               search text (optional)
                pwcPositiveFeedbackText     positive feedback text (optional)
                pwcNegativeFeedbackText     negative feedback text (optional)
                uiSortType                  sort type
                pssrSrchShortResults        short results (optional)
                uiSrchShortResultsLength    short results length
                uiTotalResults              total results
                dMaxSortKey                 max sort key
                pucSearchReportSnippet      search report snippet (optional)

    Globals:    none

    Returns:    SRCH error code

*/
int iSrchCacheSaveSearchShortResults
(
    void *pvSrchCache,
    struct srchIndex *psiSrchIndex,
    wchar_t *pwcSearchText, 
    wchar_t *pwcPositiveFeedbackText, 
    wchar_t *pwcNegativeFeedbackText, 
    unsigned int uiSortType,
    struct srchShortResult *pssrSrchShortResults, 
    unsigned int uiSrchShortResultsLength,
    unsigned int uiTotalResults,
    double dMaxSortKey,
    unsigned char *pucSearchReportSnippet
)
{

    int                 iError = SRCH_NoError;
    struct srchCache    *pscSrchCache = (struct srchCache *)pvSrchCache;
    unsigned char        pucSearchCacheFilePath[UTL_FILE_PATH_MAX + 1] = {'\0'};
    unsigned char        pucSHA1HexDigest[UTL_SHA1_HEX_DIGEST_LENGTH + 1] = {'\0'};
    FILE                 *pfSearchCacheFile = NULL;
    unsigned char        pucBuffer[BUFSIZ + 1] = {'\0'};
    unsigned char        *pucBufferPtr = NULL;
    unsigned int         uiBufferLength = 0;


#if !defined(SRCH_CACHE_ENABLE_SEARCH_SHORT_RESULTS_CACHING)
    return (SRCH_CacheUnsupportedCache);
#endif    /* !defined(SRCH_CACHE_ENABLE_SEARCH_SHORT_RESULTS_CACHING) */


    /* Check the parameters */
    if ( pvSrchCache == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvSrchCache' parameter passed to 'iSrchCacheSaveSearchShortResults'."); 
        return (SRCH_CacheInvalidCache);
    }

    if ( psiSrchIndex == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'psiSrchIndex' parameter passed to 'iSrchCacheSaveSearchShortResults'."); 
        return (SRCH_InvalidIndex);
    }


    /* Check the cache mode, we can't save a cache file if we cant read and write */
    if ( pscSrchCache->uiSearchCacheMode != SRCH_CACHE_MODE_READ_WRITE ) {
        return (SRCH_CacheInvalidMode);
    }

    /* Check the cache type, we can't handle an unknown cache type */
    if ( pscSrchCache->uiSearchCacheType == SRCH_CACHE_TYPE_UNKNOWN ) {
        return (SRCH_CacheInvalidType);
    }


    /* Create an SHA1 hex digest for this search short results */
    if ( (iError = iSrchCacheGetSearchShortResultsSHA1HexDigest(psiSrchIndex, pwcSearchText, pwcPositiveFeedbackText, pwcNegativeFeedbackText, pucSHA1HexDigest)) != SRCH_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to generate SHA1 signature for the search short results cache object, srch error: %d.", iError);
        return (iError);
    }


    /* File type */
    if ( pscSrchCache->uiSearchCacheType == SRCH_CACHE_TYPE_FILE ) {

        /* Get the search short results cache file path for this search */
        if ( (iError = iSrchCacheGetCacheFilePath(pscSrchCache, psiSrchIndex, pucSHA1HexDigest, pucSearchCacheFilePath, UTL_FILE_PATH_MAX + 1)) != SRCH_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the search short results cache file path, srch error: %d.", iError);
            goto bailFromiSrchCacheSaveSearchShortResults;
        }

        /* Open the search short results cache file */
        if ( (pfSearchCacheFile = s_fopen(pucSearchCacheFilePath, "w")) == NULL ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to create the search short results cache file: '%s'.", pucSearchCacheFilePath);
            iError = SRCH_CacheSaveFailed;
            goto bailFromiSrchCacheSaveSearchShortResults;
        }
    }



    /* Place an exclusive lock on the file */
    if ( (iError = iSrchCacheLockCacheFile(pfSearchCacheFile, SRCH_CACHE_LOCK_EXCLUSIVE)) != SRCH_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to place an exclusive lock on the search short results cache file: '%s', srch error: %d.", pucSearchCacheFilePath, iError);
        goto bailFromiSrchCacheSaveSearchShortResults;
    }


    /* Write out the total results tag */
    sprintf(pucBuffer, "%c", SRCH_CACHE_TOTAL_RESULTS_TAG);
    if ( s_fwrite(pucBuffer, 1, 1, pfSearchCacheFile) != 1 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to write to the search short results cache file: '%s'.", pucSearchCacheFilePath);
        iError = SRCH_CacheSaveFailed;
        goto bailFromiSrchCacheSaveSearchShortResults;
    }

    /* Encode the total results buffer */
    pucBufferPtr = pucBuffer;
    UTL_NUM_WRITE_COMPRESSED_UINT(uiTotalResults, pucBufferPtr);
    uiBufferLength = pucBufferPtr - pucBuffer;

    /* Write out the total results buffer */
    if ( s_fwrite(pucBuffer, uiBufferLength, 1, pfSearchCacheFile) != 1 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to write to the search short results cache file: '%s'.", pucSearchCacheFilePath);
        iError = SRCH_CacheSaveFailed;
        goto bailFromiSrchCacheSaveSearchShortResults;
    }


    /* Write out the max weight tag */
    sprintf(pucBuffer, "%c", SRCH_CACHE_MAX_WEIGHT_TAG);
    if ( s_fwrite(pucBuffer, 1, 1, pfSearchCacheFile) != 1 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to write to the search short results cache file: '%s'.", pucSearchCacheFilePath);
        iError = SRCH_CacheSaveFailed;
        goto bailFromiSrchCacheSaveSearchShortResults;
    }

    /* Encode the max sort key buffer */
    pucBufferPtr = pucBuffer;
    UTL_NUM_WRITE_COMPRESSED_DOUBLE(dMaxSortKey, pucBufferPtr);
    uiBufferLength = pucBufferPtr - pucBuffer;

    /* Write out the max weight buffer */
    if ( s_fwrite(pucBuffer, uiBufferLength, 1, pfSearchCacheFile) != 1 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to write to the search short results cache file: '%s'.", pucSearchCacheFilePath);
        iError = SRCH_CacheSaveFailed;
        goto bailFromiSrchCacheSaveSearchShortResults;
    }


    /* Write out the short results if we have any */
    if ( (pssrSrchShortResults != NULL) && (uiSrchShortResultsLength > 0) ) {
        
        struct srchShortResult      *pssrSrchShortResultPtr = NULL; 
        struct srchShortResult      *pssrSrchShortResultEndPtr = NULL;


        /* Write out the short result tag */
        sprintf(pucBuffer, "%c", SRCH_CACHE_SHORT_RESULTS_ARRAY_TAG);
        if ( s_fwrite(pucBuffer, 1, 1, pfSearchCacheFile) != 1 ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to write to the search short results cache file: '%s'.", pucSearchCacheFilePath);
            iError = SRCH_CacheSaveFailed;
            goto bailFromiSrchCacheSaveSearchShortResults;
        }


        /* Encode the short results length buffer */
        pucBufferPtr = pucBuffer;
        UTL_NUM_WRITE_COMPRESSED_UINT(uiSrchShortResultsLength, pucBufferPtr);
        uiBufferLength = pucBufferPtr - pucBuffer;

        /* Write out the short results length buffer */
        if ( s_fwrite(pucBuffer, uiBufferLength, 1, pfSearchCacheFile) != 1 ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to write to the search short results cache file: '%s'.", pucSearchCacheFilePath);
            iError = SRCH_CacheSaveFailed;
            goto bailFromiSrchCacheSaveSearchShortResults;
        }


        /* Write out the short results */
        for ( pssrSrchShortResultPtr = pssrSrchShortResults, pssrSrchShortResultEndPtr = pssrSrchShortResults + uiSrchShortResultsLength; 
                pssrSrchShortResultPtr < pssrSrchShortResultEndPtr; pssrSrchShortResultPtr++ ) {

            /* Encode the short results buffer */
            pucBufferPtr = pucBuffer;
            
            /* Encode the document ID */
            UTL_NUM_WRITE_COMPRESSED_UINT(pssrSrchShortResultPtr->uiDocumentID, pucBufferPtr);


            /* Create the sort key string, handle everything except for char here */
            switch ( uiSortType ) {
    
                case SPI_SORT_TYPE_DOUBLE_ASC:
                case SPI_SORT_TYPE_DOUBLE_DESC:
                    UTL_NUM_WRITE_COMPRESSED_DOUBLE(pssrSrchShortResultPtr->dSortKey, pucBufferPtr);
                    break;
    
                case SPI_SORT_TYPE_FLOAT_ASC:
                case SPI_SORT_TYPE_FLOAT_DESC:
                    UTL_NUM_WRITE_COMPRESSED_FLOAT(pssrSrchShortResultPtr->fSortKey, pucBufferPtr);
                    break;
                
                case SPI_SORT_TYPE_UINT_ASC:
                case SPI_SORT_TYPE_UINT_DESC:
                    UTL_NUM_WRITE_COMPRESSED_UINT(pssrSrchShortResultPtr->uiSortKey, pucBufferPtr);
                    break;
                
                case SPI_SORT_TYPE_ULONG_ASC:
                case SPI_SORT_TYPE_ULONG_DESC:
                    UTL_NUM_WRITE_COMPRESSED_ULONG(pssrSrchShortResultPtr->ulSortKey, pucBufferPtr);
                    break;
                
                case SPI_SORT_TYPE_UCHAR_ASC:
                case SPI_SORT_TYPE_UCHAR_DESC:
                    break;

                case SPI_SORT_TYPE_NO_SORT:
                    break;
    
                case SPI_SORT_TYPE_UNKNOWN:
                    break;
                
                default:
                    ASSERT(false);
                    break;
            }


            /* Get the buffer length */
            uiBufferLength = pucBufferPtr - pucBuffer;

            /* Write out the short results buffer */
            if ( s_fwrite(pucBuffer, uiBufferLength, 1, pfSearchCacheFile) != 1 ) {
                iUtlLogError(UTL_LOG_CONTEXT, "Failed to write to the search short results cache file: '%s'.", pucSearchCacheFilePath);
                iError = SRCH_CacheSaveFailed;
                goto bailFromiSrchCacheSaveSearchShortResults;
            }

            /* Write out the sort key, handle char here */
            if ( (uiSortType == SPI_SORT_TYPE_UCHAR_ASC) || (uiSortType == SPI_SORT_TYPE_UCHAR_DESC) ) {
                if ( s_fwrite(pssrSrchShortResultPtr->pucSortKey, s_strlen(pssrSrchShortResultPtr->pucSortKey) + 1, 1, pfSearchCacheFile) != 1 ) {
                    iUtlLogError(UTL_LOG_CONTEXT, "Failed to write to the search short results cache file: '%s'.", pucSearchCacheFilePath);
                    iError = SRCH_CacheSaveFailed;
                    goto bailFromiSrchCacheSaveSearchShortResults;
                }
            }
        }
    }


    /* Write out the search report snippet if we have it */
    if ( bUtlStringsIsStringNULL(pucSearchReportSnippet) == false ) {    
    
        /* Write out the search report snippet tag */
        sprintf(pucBuffer, "%c", SRCH_CACHE_SEACH_REPORT_SNIPPET_TAG);
        if ( s_fwrite(pucBuffer, 1, 1, pfSearchCacheFile) != 1 ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to write to the search short results cache file: '%s'.", pucSearchCacheFilePath);
            iError = SRCH_CacheSaveFailed;
            goto bailFromiSrchCacheSaveSearchShortResults;
        }

        /* Write the search report snippet - include terminating NULL */
        if ( s_fwrite(pucSearchReportSnippet, s_strlen(pucSearchReportSnippet) + 1, 1, pfSearchCacheFile) != 1 ) {
            iError = SRCH_CacheSaveFailed;
            goto bailFromiSrchCacheSaveSearchShortResults;
        }
    }


    /* Write out the index name - for tracking purposes */
    {    

        /* Write out the index name tag */
        sprintf(pucBuffer, "%c", SRCH_CACHE_INDEX_PATH_TAG);
        if ( s_fwrite(pucBuffer, 1, 1, pfSearchCacheFile) != 1 ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to write to the search short results cache file: '%s'.", pucSearchCacheFilePath);
            iError = SRCH_CacheSaveFailed;
            goto bailFromiSrchCacheSaveSearchShortResults;
        }

        /* Write the index name - include terminating NULL */
        if ( s_fwrite(psiSrchIndex->pucIndexName, s_strlen(psiSrchIndex->pucIndexName) + 1, 1, pfSearchCacheFile) != 1 ) {
            iError = SRCH_CacheSaveFailed;
            goto bailFromiSrchCacheSaveSearchShortResults;
        }
    }

    
    /* Write out the last update time - for tracking purposes */
    {
        /* Write out the last update time tag */
        sprintf(pucBuffer, "%c", SRCH_CACHE_LAST_UPDATE_TIME_TAG);
        if ( s_fwrite(pucBuffer, 1, 1, pfSearchCacheFile) != 1 ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to write to the search short results cache file: '%s'.", pucSearchCacheFilePath);
            iError = SRCH_CacheSaveFailed;
            goto bailFromiSrchCacheSaveSearchShortResults;
        }
    
        /* Encode the last update time buffer */
        pucBufferPtr = pucBuffer;
        UTL_NUM_WRITE_COMPRESSED_UINT((unsigned long)psiSrchIndex->tLastUpdateTime, pucBufferPtr);
        uiBufferLength = pucBufferPtr - pucBuffer;
    
        /* Write out the last update time buffer */
        if ( s_fwrite(pucBuffer, uiBufferLength, 1, pfSearchCacheFile) != 1 ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to write to the search short results cache file: '%s'.", pucSearchCacheFilePath);
            iError = SRCH_CacheSaveFailed;
            goto bailFromiSrchCacheSaveSearchShortResults;
        }
    }


    /* Write out the search text if we have it - for tracking purposes */
    if ( bUtlStringsIsWideStringNULL(pwcSearchText) == false ) {    

        /* Write out the search text tag */
        sprintf(pucBuffer, "%c", SRCH_CACHE_SEARCH_TEXT_TAG);
        if ( s_fwrite(pucBuffer, 1, 1, pfSearchCacheFile) != 1 ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to write to the search short results cache file: '%s'.", pucSearchCacheFilePath);
            iError = SRCH_CacheSaveFailed;
            goto bailFromiSrchCacheSaveSearchShortResults;
        }

        /* Write the search text */
        if ( fprintf(pfSearchCacheFile, "%ls", pwcSearchText) < 0 ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to write to the search short results cache file: '%s'.", pucSearchCacheFilePath);
            iError = SRCH_CacheSaveFailed;
            goto bailFromiSrchCacheSaveSearchShortResults;
        }
        
        /* Write the terminating null */
        if ( fputc('\0', pfSearchCacheFile) == EOF ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to write to the search short results cache file: '%s'.", pucSearchCacheFilePath);
            iError = SRCH_CacheSaveFailed;
            goto bailFromiSrchCacheSaveSearchShortResults;
        }
    }

    
    /* Write out the positive feedback text if we have it - for tracking purposes */
    if ( bUtlStringsIsWideStringNULL(pwcPositiveFeedbackText) == false ) {    

        /* Write out the positive feedback text tag */
        sprintf(pucBuffer, "%c", SRCH_CACHE_POSITIVE_FEEDBACK_TEXT_TAG);
        if ( s_fwrite(pucBuffer, 1, 1, pfSearchCacheFile) != 1 ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to write to the search short results cache file: '%s'.", pucSearchCacheFilePath);
            iError = SRCH_CacheSaveFailed;
            goto bailFromiSrchCacheSaveSearchShortResults;
        }

        /* Write the positive feedback text */
        if ( fprintf(pfSearchCacheFile, "%ls", pwcPositiveFeedbackText) < 0 ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to write to the search short results cache file: '%s'.", pucSearchCacheFilePath);
            iError = SRCH_CacheSaveFailed;
            goto bailFromiSrchCacheSaveSearchShortResults;
        }
        
        /* Write the terminating null */
        if ( fputc('\0', pfSearchCacheFile) == EOF ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to write to the search short results cache file: '%s'.", pucSearchCacheFilePath);
            iError = SRCH_CacheSaveFailed;
            goto bailFromiSrchCacheSaveSearchShortResults;
        }
    }

    
    /* Write out the negative feedback text if we have it - for tracking purposes */
    if ( bUtlStringsIsWideStringNULL(pwcNegativeFeedbackText) == false ) {    

        /* Write out the negative feedback text tag */
        sprintf(pucBuffer, "%c", SRCH_CACHE_NEGATIVE_FEEDBACK_TEXT_TAG);
        if ( s_fwrite(pucBuffer, 1, 1, pfSearchCacheFile) != 1 ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to write to the search short results cache file: '%s'.", pucSearchCacheFilePath);
            iError = SRCH_CacheSaveFailed;
            goto bailFromiSrchCacheSaveSearchShortResults;
        }

        /* Write the negative feedback text */
        if ( fprintf(pfSearchCacheFile, "%ls", pwcNegativeFeedbackText) < 0 ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to write to the search short results cache file: '%s'.", pucSearchCacheFilePath);
            iError = SRCH_CacheSaveFailed;
            goto bailFromiSrchCacheSaveSearchShortResults;
        }
        
        /* Write the terminating null */
        if ( fputc('\0', pfSearchCacheFile) == EOF ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to write to the search short results cache file: '%s'.", pucSearchCacheFilePath);
            iError = SRCH_CacheSaveFailed;
            goto bailFromiSrchCacheSaveSearchShortResults;
        }
    }



    /* File type */
    if ( pscSrchCache->uiSearchCacheType == SRCH_CACHE_TYPE_FILE ) {
/*         iUtlLogInfo(UTL_LOG_CONTEXT, "Saved search short results in file - pwcSearchText: '%ls' => '%s' (%lu).", pwcSearchText, pucSearchCacheFilePath, (unsigned long)s_ftell(pfSearchCacheFile)); */
/*         iUtlLogDebug(UTL_LOG_CONTEXT, "Saved search short results in file: '%s'.", pucSearchCacheFilePath); */
    }



    /* Bail label */
    bailFromiSrchCacheSaveSearchShortResults:


    /* Handle the error */
    if ( iError == SRCH_NoError ) {

        /* Release the lock we are currently holding */
        iSrchCacheUnlockCacheFile(pfSearchCacheFile);

        /* Close the search short results cache file */
        s_fclose(pfSearchCacheFile);
    }
    else {

        /* File type */
        if ( pscSrchCache->uiSearchCacheType == SRCH_CACHE_TYPE_FILE ) {

            /* Remove the file if it was created */
            if ( pfSearchCacheFile != NULL ) {
            
                int    iLocalError = SRCH_NoError;

                /* Place an exclusive lock on the search short results cache file, just warn if we cant */
                if ( (iLocalError = iSrchCacheLockCacheFile(pfSearchCacheFile, SRCH_CACHE_LOCK_EXCLUSIVE)) != SRCH_NoError ) {
                    iUtlLogWarn(UTL_LOG_CONTEXT, "Failed to place an exclusive lock on the search short results cache file: '%s', srch error: %d.", pucSearchCacheFilePath, iLocalError);
                }
    
                /* Remove the file */
                s_remove(pucSearchCacheFilePath);
    
                /* Close the file  */
                s_fclose(pfSearchCacheFile);
            }
        }
    }


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchCacheGetSearchShortResults()

    Purpose:    This function checks to see if there is a cache file for the
                passed search, and returns the contents of that cache file if
                it is there.

    Parameters: pvSrchCache                 search cache structure
                psiSrchIndex                search index structure
                pwcSearchText               search text (optional)
                pwcPositiveFeedbackText     positive feedback text (optional)
                pwcNegativeFeedbackText     negative feedback text (optional)
                uiSortType                  sort type
                ppssrSrchShortResults       return pointer for the short results (gets allocated)
                puiSrchShortResultsLength   return pointer for the short results length
                puiTotalResults             return pointer for the total results
                pdMaxSortKey                return pointer for the max sort key
                ppucSearchReportSnippet     return pointer for the search report snippet (gets allocated)

    Globals:    none

    Returns:    SRCH error code

*/
int iSrchCacheGetSearchShortResults
(
    void *pvSrchCache,
    struct srchIndex *psiSrchIndex, 
    wchar_t *pwcSearchText, 
    wchar_t *pwcPositiveFeedbackText, 
    wchar_t *pwcNegativeFeedbackText, 
    unsigned int uiSortType, 
    struct srchShortResult **ppssrSrchShortResults, 
    unsigned int *puiSrchShortResultsLength, 
    unsigned int *puiTotalResults,
    double *pdMaxSortKey,
    unsigned char **ppucSearchReportSnippet 
)
{

    int                         iError = SRCH_NoError;
    struct srchCache            *pscSrchCache = (struct srchCache *)pvSrchCache;
    unsigned char               pucSearchCacheFilePath[UTL_FILE_PATH_MAX + 1] = {'\0'};
    unsigned char               pucSHA1HexDigest[UTL_SHA1_HEX_DIGEST_LENGTH + 1] = {'\0'};
    FILE                        *pfSearchCacheFile = NULL;
    float                       dMaxSortKey = 0;
    unsigned int                uiTotalResults = 0;
    struct srchShortResult      *pssrSrchShortResults = NULL; 
    unsigned int                uiSrchShortResultsLength = 0;
    unsigned char               *pucSearchReportSnippet = NULL;
    off_t                       zSearchCacheFileDataLength = 0;
    unsigned char               *pucSearchCacheData = NULL;
    unsigned char               *pucSearchCacheDataPtr = NULL;
    unsigned char               *pucSearchCacheDataEndPtr = NULL;


#if !defined(SRCH_CACHE_ENABLE_SEARCH_SHORT_RESULTS_CACHING)
    return (SRCH_CacheUnsupportedCache);
#endif    /* !defined(SRCH_CACHE_ENABLE_SEARCH_SHORT_RESULTS_CACHING) */


    /* Check the parameters */
    if ( pvSrchCache == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvSrchCache' parameter passed to 'iSrchCacheGetSearchShortResults'."); 
        return (SRCH_CacheInvalidCache);
    }

    if ( psiSrchIndex == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'psiSrchIndex' parameter passed to 'iSrchCacheGetSearchShortResults'."); 
        return (SRCH_InvalidIndex);
    }

    if ( ppssrSrchShortResults == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppssrSrchShortResults' parameter passed to 'iSrchCacheGetSearchShortResults'."); 
        return (SRCH_ReturnParameterError);
    }

    if ( puiSrchShortResultsLength == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'puiSrchShortResultsLength' parameter passed to 'iSrchCacheGetSearchShortResults'."); 
        return (SRCH_ReturnParameterError);
    }

    if ( puiTotalResults == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'puiTotalResults' parameter passed to 'iSrchCacheGetSearchShortResults'."); 
        return (SRCH_ReturnParameterError);
    }

    if ( pdMaxSortKey == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pdMaxSortKey' parameter passed to 'iSrchCacheGetSearchShortResults'."); 
        return (SRCH_ReturnParameterError);
    }

    if ( ppucSearchReportSnippet == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppucSearchReportSnippet' parameter passed to 'iSrchCacheGetSearchShortResults'."); 
        return (SRCH_ReturnParameterError);
    }


    /* Check the cache mode, we cant read a cache file if the cache is off */
    if ( pscSrchCache->uiSearchCacheMode == SRCH_CACHE_MODE_OFF ) {
        return (SRCH_CacheInvalidMode);
    }

    /* Check the cache type, we can't handle an unknown cache type */
    if ( pscSrchCache->uiSearchCacheType == SRCH_CACHE_TYPE_UNKNOWN ) {
        return (SRCH_CacheInvalidType);
    }



    /* Create an SHA1 hex digest for this search short results */
    if ( (iError = iSrchCacheGetSearchShortResultsSHA1HexDigest(psiSrchIndex, pwcSearchText, pwcPositiveFeedbackText, pwcNegativeFeedbackText, pucSHA1HexDigest)) != SRCH_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to generate SHA1 signature for search short results cache file path, srch error: %d.", iError);
        return (iError);
    }


    /* File type */
    if ( pscSrchCache->uiSearchCacheType == SRCH_CACHE_TYPE_FILE ) {

        /* Get the search short results cache file path for this search */
        if ( (iError = iSrchCacheGetCacheFilePath(pscSrchCache, psiSrchIndex, pucSHA1HexDigest, pucSearchCacheFilePath, UTL_FILE_PATH_MAX + 1)) != SRCH_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the search short results cache file path, srch error: %d.", iError);
            iError = SRCH_CacheGetFailed;
            goto bailFromiSrchCacheGetSearchShortResults;
        }
    
        /* Check that there is a search short results cache file */
        if ( bUtlFileIsFile(pucSearchCacheFilePath) == false ) {
            iError = SRCH_CacheGetFailed;
            goto bailFromiSrchCacheGetSearchShortResults;
        }
        
        /* Check that the search short results cache file can be read */
        if ( bUtlFilePathRead(pucSearchCacheFilePath) == false ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to access the search short results cache file: '%s'.", pucSearchCacheFilePath);
            iError = SRCH_CacheGetFailed;
            goto bailFromiSrchCacheGetSearchShortResults;
        }
    
        /* Open the search short results cache file */
        if ( (pfSearchCacheFile = s_fopen(pucSearchCacheFilePath, "r")) == NULL ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to open the search short results cache file: '%s'.", pucSearchCacheFilePath);
            iError = SRCH_CacheGetFailed;
            goto bailFromiSrchCacheGetSearchShortResults;
        }
    
        /* Get the search short results cache file length */
        if ( (iError = iUtlFileGetFileLength(pfSearchCacheFile, &zSearchCacheFileDataLength)) != UTL_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the length of the search short results cache file: '%s', utl error: %d.", pucSearchCacheFilePath, iError);
            iError = SRCH_CacheGetFailed;
            goto bailFromiSrchCacheGetSearchShortResults;
        }
    
        /* Check the length of the cache file */
        if ( zSearchCacheFileDataLength == 0 ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to read from the search short results cache file: '%s', empty file.", pucSearchCacheFilePath);
            iError = SRCH_CacheGetFailed;
            goto bailFromiSrchCacheGetSearchShortResults;
        }
    
        /* Place a shared lock on the search short results cache file */
        if ( (iError = iSrchCacheLockCacheFile(pfSearchCacheFile, SRCH_CACHE_LOCK_SHARED)) != SRCH_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to place a shared lock on the search short results cache file: '%s', srch error: %d.", pucSearchCacheFilePath, iError);
            iError = SRCH_CacheGetFailed;
            goto bailFromiSrchCacheGetSearchShortResults;
        }
        
        /* Map in the entire file */
        if ( (iError = iUtlFileMemoryMap(fileno(pfSearchCacheFile), 0, zSearchCacheFileDataLength, PROT_READ, (void **)&pucSearchCacheData)) != UTL_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to map in the search short results cache file: '%s', utl error: %d.", pucSearchCacheFilePath, iError);
            iError = SRCH_CacheGetFailed;
            goto bailFromiSrchCacheGetSearchShortResults;
        }

        /* Set the cache file pointers */
        pucSearchCacheDataPtr = pucSearchCacheData;
        pucSearchCacheDataEndPtr = pucSearchCacheData + zSearchCacheFileDataLength;

/*         iUtlLogInfo(UTL_LOG_CONTEXT, "Read search short results from file - pwcSearchText: '%ls' => '%s' (%lu).", pwcSearchText, pucSearchCacheFilePath, (unsigned long)zSearchCacheFileDataLength); */
/*         iUtlLogDebug(UTL_LOG_CONTEXT, "Read search short results from file: '%s'.", pucSearchCacheFilePath); */
    }


    ASSERT(pucSearchCacheDataPtr != NULL);
    ASSERT(pucSearchCacheDataEndPtr != NULL);
    ASSERT(pucSearchCacheDataPtr <= pucSearchCacheDataEndPtr);


    /* Loop reading the file while there is stuff to read */
    while ( pucSearchCacheDataPtr < pucSearchCacheDataEndPtr ) {

        /* Total results */
        if ( *pucSearchCacheDataPtr == SRCH_CACHE_TOTAL_RESULTS_TAG ) {
            
            /* Increment the cache file pointer past the tag */
            pucSearchCacheDataPtr++;
    
            /* Decode the total results */
            UTL_NUM_READ_COMPRESSED_UINT(uiTotalResults, pucSearchCacheDataPtr);
        }
        
        /* Max weight */
        else if ( *pucSearchCacheDataPtr == SRCH_CACHE_MAX_WEIGHT_TAG ) {
            
            /* Increment the cache file pointer past the tag */
            pucSearchCacheDataPtr++;
    
            /* Decode the max sort key */
            UTL_NUM_READ_COMPRESSED_DOUBLE(dMaxSortKey, pucSearchCacheDataPtr);
        }
        
        /* Short results */
        else if ( *pucSearchCacheDataPtr == SRCH_CACHE_SHORT_RESULTS_ARRAY_TAG ) {
    
            struct srchShortResult      *pssrSrchShortResultPtr = NULL; 
            struct srchShortResult      *pssrSrchShortResultEndPtr = NULL;

            
            /* Increment the cache file pointer past the tag */
            pucSearchCacheDataPtr++;
    
            /* Decode the short results length */
            UTL_NUM_READ_COMPRESSED_UINT(uiSrchShortResultsLength, pucSearchCacheDataPtr);
    
            /* Allocate the short results */
            if ( (pssrSrchShortResults = (struct srchShortResult *)s_malloc((size_t)(sizeof(struct srchShortResult) * uiSrchShortResultsLength))) == NULL ) {
                iError = SRCH_MemError;
                goto bailFromiSrchCacheGetSearchShortResults;
            }
    
            /* Read in the short results */
            for ( pssrSrchShortResultPtr = pssrSrchShortResults, pssrSrchShortResultEndPtr = pssrSrchShortResults + uiSrchShortResultsLength; 
                    pssrSrchShortResultPtr < pssrSrchShortResultEndPtr; pssrSrchShortResultPtr++ ) {
    
                /* Decode the document ID */
                UTL_NUM_READ_COMPRESSED_UINT(pssrSrchShortResultPtr->uiDocumentID, pucSearchCacheDataPtr);
                
                /* Decode the sort key */
                switch ( uiSortType ) {
        
                    case SPI_SORT_TYPE_DOUBLE_ASC:
                    case SPI_SORT_TYPE_DOUBLE_DESC:
                        UTL_NUM_READ_COMPRESSED_DOUBLE(pssrSrchShortResultPtr->dSortKey, pucSearchCacheDataPtr);
                        break;
        
                    case SPI_SORT_TYPE_FLOAT_ASC:
                    case SPI_SORT_TYPE_FLOAT_DESC:
                        UTL_NUM_READ_COMPRESSED_FLOAT(pssrSrchShortResultPtr->fSortKey, pucSearchCacheDataPtr);
                        break;
                    
                    case SPI_SORT_TYPE_UINT_ASC:
                    case SPI_SORT_TYPE_UINT_DESC:
                        UTL_NUM_READ_COMPRESSED_UINT(pssrSrchShortResultPtr->uiSortKey, pucSearchCacheDataPtr);
                        break;
                    
                    case SPI_SORT_TYPE_ULONG_ASC:
                    case SPI_SORT_TYPE_ULONG_DESC:
                        UTL_NUM_READ_COMPRESSED_ULONG(pssrSrchShortResultPtr->ulSortKey, pucSearchCacheDataPtr);
                        break;
                    
                    case SPI_SORT_TYPE_UCHAR_ASC:
                    case SPI_SORT_TYPE_UCHAR_DESC:
                        if ( (pssrSrchShortResultPtr->pucSortKey = (unsigned char *)s_strdup(pucSearchCacheDataPtr)) == NULL ) {
                            iError = SRCH_MemError;
                            goto bailFromiSrchCacheGetSearchShortResults;
                        }
                        /* Increment the pointer past the character sort key */
                        pucSearchCacheDataPtr = (unsigned char *)s_strchr(pucSearchCacheDataPtr, '\0') + 1;
                        break;
    
                    case SPI_SORT_TYPE_NO_SORT:
                        break;
        
                    case SPI_SORT_TYPE_UNKNOWN:
                        break;
                    
                    default:
                        ASSERT(false);
                        break;
                }
                
                /* Set the index pointer */
                pssrSrchShortResultPtr->psiSrchIndexPtr = psiSrchIndex;
            }
        }
        
        /* Search report snippet */
        else if ( *pucSearchCacheDataPtr == SRCH_CACHE_SEACH_REPORT_SNIPPET_TAG ) {
        
            /* Increment the cache file pointer past the tag */
            pucSearchCacheDataPtr++;
    
            /* Duplicate the search report snippet */
            if ( (pucSearchReportSnippet = (unsigned char *)s_strdup(pucSearchCacheDataPtr)) == NULL ) {
                iError = SRCH_MemError;
                goto bailFromiSrchCacheGetSearchShortResults;
            }

            /* Increment the pointer past the search report snippet */
            pucSearchCacheDataPtr = (unsigned char *)s_strchr(pucSearchCacheDataPtr, '\0') + 1;
        }
        
        /* Index name */
        else if ( *pucSearchCacheDataPtr == SRCH_CACHE_INDEX_PATH_TAG ) {
            
            /* Increment the cache file pointer past the tag */
            pucSearchCacheDataPtr++;    

/*             iUtlLogDebug(UTL_LOG_CONTEXT, "Index name: '%s'.", pucSearchCacheDataPtr); */

            /* Increment the pointer past the index name */
            pucSearchCacheDataPtr = (unsigned char *)s_strchr(pucSearchCacheDataPtr, '\0') + 1;
        }
        
        /* Last update time */
        else if ( *pucSearchCacheDataPtr == SRCH_CACHE_LAST_UPDATE_TIME_TAG ) {
            
            time_t      tLastUpdateTime = (time_t)0;
            
            /* Increment the cache file pointer past the tag */
            pucSearchCacheDataPtr++;
    
            /* Decode the last update time */
            UTL_NUM_READ_COMPRESSED_UINT(tLastUpdateTime, pucSearchCacheDataPtr);

/*             iUtlLogDebug(UTL_LOG_CONTEXT, "Last update time: %lu.", (unsigned long)tLastUpdateTime); */
        }

        /* Search text */
        else if ( *pucSearchCacheDataPtr == SRCH_CACHE_SEARCH_TEXT_TAG ) {
            
            /* Increment the cache file pointer past the tag */
            pucSearchCacheDataPtr++;    

/*             iUtlLogDebug(UTL_LOG_CONTEXT, "Search text: '%ls'.", pucSearchCacheDataPtr); */

            /* Increment the pointer past the search text */
            pucSearchCacheDataPtr = (unsigned char *)s_strchr(pucSearchCacheDataPtr, '\0') + 1;
        }
        
        /* Positive relevance feedback text */
        else if ( *pucSearchCacheDataPtr == SRCH_CACHE_POSITIVE_FEEDBACK_TEXT_TAG ) {
            
            /* Increment the cache file pointer past the tag */
            pucSearchCacheDataPtr++;    

/*             iUtlLogDebug(UTL_LOG_CONTEXT, "Positive relevance feedback text: '%ls'.", pucSearchCacheDataPtr); */

            /* Increment the pointer past the positive feedback text */
            pucSearchCacheDataPtr = (unsigned char *)s_strchr(pucSearchCacheDataPtr, '\0') + 1;
        }
        
        /* Negative relevance feedback text */
        else if ( *pucSearchCacheDataPtr == SRCH_CACHE_NEGATIVE_FEEDBACK_TEXT_TAG ) {
            
            /* Increment the cache file pointer past the tag */
            pucSearchCacheDataPtr++;    

/*             iUtlLogDebug(UTL_LOG_CONTEXT, "Negative relevance feedback text: '%ls'.", pucSearchCacheDataPtr); */

            /* Increment the pointer past the negative feedback text */
            pucSearchCacheDataPtr = (unsigned char *)s_strchr(pucSearchCacheDataPtr, '\0') + 1;
        }
        
        /* Invalid tag, skip over it for now, but where does it end?? */
        else {
        
            /* Increment the cache file pointer past the tag */
            pucSearchCacheDataPtr++;
        }
        
    }



    /* Bail label */
    bailFromiSrchCacheGetSearchShortResults:


    /* File type */
    if ( pscSrchCache->uiSearchCacheType == SRCH_CACHE_TYPE_FILE ) {

        /* Release the memory mapping */    
        iUtlFileMemoryUnMap(pucSearchCacheData, zSearchCacheFileDataLength);
        pucSearchCacheData = NULL;
    }


    /* Handle the error */
    if ( iError == SRCH_NoError ) {
        
        /* File type */
        if ( pscSrchCache->uiSearchCacheType == SRCH_CACHE_TYPE_FILE ) {

#if defined(SRCH_CACHE_ENABLE_UTIME_CACHE_FILE_UPDATE)
            /* Set the access and modification times on the search short results cache file to now */
            if  ( pscSrchCache->uiSearchCacheMode == SRCH_CACHE_MODE_READ_WRITE ) {

                int    iLocalError = UTL_NoError;

                if ( (iLocalError = iUtlFileSetPathUTime(pucSearchCacheFilePath)) != UTL_NoError ) {
                    iUtlLogError(UTL_LOG_CONTEXT, "Failed to set the access and modification times on the search short results cache file: '%s', utl error: %d.", 
                            pucSearchCacheFilePath, iLocalError);
                }
            }
#endif    /* defined(SRCH_CACHE_ENABLE_UTIME_CACHE_FILE_UPDATE) */

            /* Release the lock we are currently holding */
            iSrchCacheUnlockCacheFile(pfSearchCacheFile);

            /* Close the search short results cache file */
            s_fclose(pfSearchCacheFile);

/*             iUtlLogDebug(UTL_LOG_CONTEXT, "Search results were returned from the search short results cache file: '%s'.", pucSearchCacheFilePath); */
        }

        /* Set the return pointers */
        *pdMaxSortKey = dMaxSortKey;
        *puiTotalResults = (uiTotalResults == 0) ? uiSrchShortResultsLength : uiTotalResults;
        *ppssrSrchShortResults = pssrSrchShortResults;
        *puiSrchShortResultsLength = uiSrchShortResultsLength;
        *ppucSearchReportSnippet = pucSearchReportSnippet;

#if defined(SRCH_CACHE_ENABLE_SEARCH_SHORT_RESULTS_CACHE_LOGGING) || defined(SRCH_CACHE_ENABLE_ALL_CACHE_LOGGING)
        iUtlLogInfo(UTL_LOG_CONTEXT, "Search short results were served from cache.");
#endif    /* defined(SRCH_CACHE_ENABLE_SEARCH_SHORT_RESULTS_CACHE_LOGGING) || defined(SRCH_CACHE_ENABLE_ALL_CACHE_LOGGING) */

    }
    else {
        
        /* Free any allocated resources */
        iSrchShortResultFree (pssrSrchShortResults, uiSrchShortResultsLength, uiSortType);
        pssrSrchShortResults = NULL;
        s_free(pucSearchReportSnippet);
    
        /* File type */
        if ( pscSrchCache->uiSearchCacheType == SRCH_CACHE_TYPE_FILE ) {

            /* Remove the file if it is open and we are in read-write mode */
            if ( (pfSearchCacheFile != NULL) && (pscSrchCache->uiSearchCacheMode == SRCH_CACHE_MODE_READ_WRITE) ) {

                int    iLocalError = SRCH_NoError;

                /* Place an exclusive lock on the search short results cache file, just fall through if we failed */
                if ( (iLocalError = iSrchCacheLockCacheFile(pfSearchCacheFile, SRCH_CACHE_LOCK_EXCLUSIVE)) != SRCH_NoError ) {
                    iUtlLogWarn(UTL_LOG_CONTEXT, "Failed to place an exclusive lock on the search short results cache file: '%s', srch error: %d.", pucSearchCacheFilePath, iLocalError);
                }

                /* Remove the file */
/*                 s_remove(pucSearchCacheFilePath); */

                /* Close the file  */
                s_fclose(pfSearchCacheFile);
            }
        }
    }


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchCacheSaveSearchPostingsList()

    Purpose:    This function saves a search postings list structure in the cache.

    Parameters: pvSrchCache                 search cache structure
                psiSrchIndex                search index structure
                uiLanguageID                language ID
                psptSrchParserTerm          search parser term structure
                psplSrchPostingsList        search postings list structure
                pucSearchReportSnippet      search report snippet (optional)

    Globals:    none

    Returns:    SRCH error code

*/
int iSrchCacheSaveSearchPostingsList
(
    void *pvSrchCache,
    struct srchIndex *psiSrchIndex,
    unsigned int uiLanguageID, 
    struct srchParserTerm *psptSrchParserTerm, 
    struct srchPostingsList *psplSrchPostingsList,
    unsigned char *pucSearchReportSnippet
)
{

    int                 iError = SRCH_NoError;
    struct srchCache    *pscSrchCache = (struct srchCache *)pvSrchCache;
    unsigned char       pucSearchCacheFilePath[UTL_FILE_PATH_MAX + 1] = {'\0'};
    unsigned char       pucSHA1HexDigest[UTL_SHA1_HEX_DIGEST_LENGTH + 1] = {'\0'};
    FILE                *pfSearchCacheFile = NULL;
    unsigned char       pucBuffer[BUFSIZ + 1] = {'\0'};
    unsigned char       *pucBufferPtr = NULL;
    unsigned int        uiBufferLength = 0;
    unsigned int        uiRequired = 0;


#if !defined(SRCH_CACHE_ENABLE_SEARCH_POSTINGS_CACHING)
    return (SRCH_CacheUnsupportedCache);
#endif    /* !defined(SRCH_CACHE_ENABLE_SEARCH_POSTINGS_CACHING) */


    /* Check the parameters */
    if ( pvSrchCache == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvSrchCache' parameter passed to 'iSrchCacheSaveSearchPostingsList'."); 
        return (SRCH_CacheInvalidCache);
    }

    if ( psiSrchIndex == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'psiSrchIndex' parameter passed to 'iSrchCacheSaveSearchPostingsList'."); 
        return (SRCH_InvalidIndex);
    }

    if ( psptSrchParserTerm == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'psptSrchParserTerm' parameter passed to 'iSrchCacheSaveSearchPostingsList'."); 
        return (SRCH_CacheInvalidParserTerm);
    }

    if ( psplSrchPostingsList == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'psplSrchPostingsList' parameter passed to 'iSrchCacheSaveSearchPostingsList'."); 
        return (SRCH_ReturnParameterError);
    }


    /* Check the cache mode, we can't save a cache file if we cant read and write */
    if ( pscSrchCache->uiSearchCacheMode != SRCH_CACHE_MODE_READ_WRITE ) {
        return (SRCH_CacheInvalidMode);
    }

    /* Check the cache type, we can't handle an unknown cache type */
    if ( pscSrchCache->uiSearchCacheType == SRCH_CACHE_TYPE_UNKNOWN ) {
        return (SRCH_CacheInvalidType);
    }


    /* Create an SHA1 hex digest for this search postings list */
    if ( (iError = iSrchCacheGetSearchPostingsListSHA1HexDigest(psiSrchIndex, uiLanguageID, psptSrchParserTerm, pucSHA1HexDigest)) != SRCH_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to generate SHA1 signature for the search postings list cache object, srch error: %d.", iError);
        return (iError);
    }


    /* File type */
    if ( pscSrchCache->uiSearchCacheType == SRCH_CACHE_TYPE_FILE ) {

        /* Get the search postings list cache file path for this search */
        if ( (iError = iSrchCacheGetCacheFilePath(pscSrchCache, psiSrchIndex, pucSHA1HexDigest, pucSearchCacheFilePath, UTL_FILE_PATH_MAX + 1)) != SRCH_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the search postings list cache file path, srch error: %d.", iError);
            goto bailFromiSrchCacheSaveSearchPostingsList;
        }

        /* Open the search postings list cache file */
        if ( (pfSearchCacheFile = s_fopen(pucSearchCacheFilePath, "w")) == NULL ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to create the search postings list cache file: '%s'.", pucSearchCacheFilePath);
            iError = SRCH_CacheSaveFailed;
            goto bailFromiSrchCacheSaveSearchPostingsList;
        }
    }



    /* Place an exclusive lock on the file */
    if ( (iError = iSrchCacheLockCacheFile(pfSearchCacheFile, SRCH_CACHE_LOCK_EXCLUSIVE)) != SRCH_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to place an exclusive lock on the search postings list cache file: '%s', srch error: %d.", pucSearchCacheFilePath, iError);
        goto bailFromiSrchCacheSaveSearchPostingsList;
    }


    /* Write out the term type tag */
    sprintf(pucBuffer, "%c", SRCH_CACHE_TERM_TYPE_TAG);
    if ( s_fwrite(pucBuffer, 1, 1, pfSearchCacheFile) != 1 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to write to the search postings list cache file: '%s'.", pucSearchCacheFilePath);
        iError = SRCH_CacheSaveFailed;
        goto bailFromiSrchCacheSaveSearchPostingsList;
    }

    /* Encode the term type buffer */
    pucBufferPtr = pucBuffer;
    UTL_NUM_WRITE_COMPRESSED_UINT(psplSrchPostingsList->uiTermType, pucBufferPtr);
    uiBufferLength = pucBufferPtr - pucBuffer;

    /* Write out the term type buffer */
    if ( s_fwrite(pucBuffer, uiBufferLength, 1, pfSearchCacheFile) != 1 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to write to the search postings list cache file: '%s'.", pucSearchCacheFilePath);
        iError = SRCH_CacheSaveFailed;
        goto bailFromiSrchCacheSaveSearchPostingsList;
    }

    /* Write out the term count tag */
    sprintf(pucBuffer, "%c", SRCH_CACHE_TERM_COUNT_TAG);
    if ( s_fwrite(pucBuffer, 1, 1, pfSearchCacheFile) != 1 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to write to the search postings list cache file: '%s'.", pucSearchCacheFilePath);
        iError = SRCH_CacheSaveFailed;
        goto bailFromiSrchCacheSaveSearchPostingsList;
    }

    /* Encode the term count buffer */
    pucBufferPtr = pucBuffer;
    UTL_NUM_WRITE_COMPRESSED_UINT(psplSrchPostingsList->uiTermCount, pucBufferPtr);
    uiBufferLength = pucBufferPtr - pucBuffer;

    /* Write out the term count buffer */
    if ( s_fwrite(pucBuffer, uiBufferLength, 1, pfSearchCacheFile) != 1 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to write to the search postings list cache file: '%s'.", pucSearchCacheFilePath);
        iError = SRCH_CacheSaveFailed;
        goto bailFromiSrchCacheSaveSearchPostingsList;
    }


    /* Write out the document count tag */
    sprintf(pucBuffer, "%c", SRCH_CACHE_DOCUMENT_COUNT_TAG);
    if ( s_fwrite(pucBuffer, 1, 1, pfSearchCacheFile) != 1 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to write to the search postings list cache file: '%s'.", pucSearchCacheFilePath);
        iError = SRCH_CacheSaveFailed;
        goto bailFromiSrchCacheSaveSearchPostingsList;
    }

    /* Encode the document count buffer */
    pucBufferPtr = pucBuffer;
    UTL_NUM_WRITE_COMPRESSED_UINT(psplSrchPostingsList->uiDocumentCount, pucBufferPtr);
    uiBufferLength = pucBufferPtr - pucBuffer;

    /* Write out the document count buffer */
    if ( s_fwrite(pucBuffer, uiBufferLength, 1, pfSearchCacheFile) != 1 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to write to the search postings list cache file: '%s'.", pucSearchCacheFilePath);
        iError = SRCH_CacheSaveFailed;
        goto bailFromiSrchCacheSaveSearchPostingsList;
    }


    /* Write out the required tag */
    sprintf(pucBuffer, "%c", SRCH_CACHE_REQUIRE_TAG);
    if ( s_fwrite(pucBuffer, 1, 1, pfSearchCacheFile) != 1 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to write to the search postings list cache file: '%s'.", pucSearchCacheFilePath);
        iError = SRCH_CacheSaveFailed;
        goto bailFromiSrchCacheSaveSearchPostingsList;
    }

    /* Encode the required buffer */
    pucBufferPtr = pucBuffer;
    uiRequired = psplSrchPostingsList->bRequired ? 1 : 0;
    UTL_NUM_WRITE_COMPRESSED_UINT(uiRequired, pucBufferPtr);
    uiBufferLength = pucBufferPtr - pucBuffer;

    /* Write out the required buffer */
    if ( s_fwrite(pucBuffer, uiBufferLength, 1, pfSearchCacheFile) != 1 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to write to the search postings list cache file: '%s'.", pucSearchCacheFilePath);
        iError = SRCH_CacheSaveFailed;
        goto bailFromiSrchCacheSaveSearchPostingsList;
    }


    /* Write out the postings list if we have any */
    if ( (psplSrchPostingsList->pspSrchPostings != NULL) && (psplSrchPostingsList->uiSrchPostingsLength > 0) ) {
        
        struct srchPosting        *pspSrchPostingsPtr = NULL; 
        struct srchPosting        *pspSrchPostingsEndPtr = NULL; 


        /* Write out the postings array tag */
        sprintf(pucBuffer, "%c", SRCH_CACHE_POSTINGS_ARRAY_TAG);
        if ( s_fwrite(pucBuffer, 1, 1, pfSearchCacheFile) != 1 ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to write to the search postings list cache file: '%s'.", pucSearchCacheFilePath);
            iError = SRCH_CacheSaveFailed;
            goto bailFromiSrchCacheSaveSearchPostingsList;
        }


        /* Encode the postings list length buffer */
        pucBufferPtr = pucBuffer;
        UTL_NUM_WRITE_COMPRESSED_UINT(psplSrchPostingsList->uiSrchPostingsLength, pucBufferPtr);
        uiBufferLength = pucBufferPtr - pucBuffer;

        /* Write out the postings list length buffer */
        if ( s_fwrite(pucBuffer, uiBufferLength, 1, pfSearchCacheFile) != 1 ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to write to the search postings list cache file: '%s'.", pucSearchCacheFilePath);
            iError = SRCH_CacheSaveFailed;
            goto bailFromiSrchCacheSaveSearchPostingsList;
        }


        /* Write out the postings */
        for ( pspSrchPostingsPtr = psplSrchPostingsList->pspSrchPostings, pspSrchPostingsEndPtr = psplSrchPostingsList->pspSrchPostings + psplSrchPostingsList->uiSrchPostingsLength; 
                pspSrchPostingsPtr < pspSrchPostingsEndPtr; pspSrchPostingsPtr++ ) {

            /* Encode the postings list buffer */
            pucBufferPtr = pucBuffer;
            
            /* Encode the various fields */
            UTL_NUM_WRITE_COMPRESSED_UINT(pspSrchPostingsPtr->uiDocumentID, pucBufferPtr);
            UTL_NUM_WRITE_COMPRESSED_UINT(pspSrchPostingsPtr->uiTermPosition, pucBufferPtr);
            UTL_NUM_WRITE_COMPRESSED_FLOAT(pspSrchPostingsPtr->fWeight, pucBufferPtr);

            /* Get the buffer length */
            uiBufferLength = pucBufferPtr - pucBuffer;

            /* Write out the postings list buffer */
            if ( s_fwrite(pucBuffer, uiBufferLength, 1, pfSearchCacheFile) != 1 ) {
                iUtlLogError(UTL_LOG_CONTEXT, "Failed to write to the search postings list cache file: '%s'.", pucSearchCacheFilePath);
                iError = SRCH_CacheSaveFailed;
                goto bailFromiSrchCacheSaveSearchPostingsList;
            }
        }
    }


    /* Write out the search report snippet if we have it */
    if ( bUtlStringsIsStringNULL(pucSearchReportSnippet) == false ) {    
    
        /* Write out the search report snippet tag */
        sprintf(pucBuffer, "%c", SRCH_CACHE_SEACH_REPORT_SNIPPET_TAG);
        if ( s_fwrite(pucBuffer, 1, 1, pfSearchCacheFile) != 1 ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to write to the search short results cache file: '%s'.", pucSearchCacheFilePath);
            iError = SRCH_CacheSaveFailed;
            goto bailFromiSrchCacheSaveSearchPostingsList;
        }

        /* Write the search report snippet - include terminating NULL */
        if ( s_fwrite(pucSearchReportSnippet, s_strlen(pucSearchReportSnippet) + 1, 1, pfSearchCacheFile) != 1 ) {
            iError = SRCH_CacheSaveFailed;
            goto bailFromiSrchCacheSaveSearchPostingsList;
        }
    }


    /* Write out the index name if we have it - for tracking purposes */
    if ( bUtlStringsIsStringNULL(psiSrchIndex->pucIndexName) == false ) {    

        /* Write out the index name tag */
        sprintf(pucBuffer, "%c", SRCH_CACHE_INDEX_PATH_TAG);
        if ( s_fwrite(pucBuffer, 1, 1, pfSearchCacheFile) != 1 ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to write to the search postings list cache file: '%s'.", pucSearchCacheFilePath);
            iError = SRCH_CacheSaveFailed;
            goto bailFromiSrchCacheSaveSearchPostingsList;
        }

        /* Write the index name - include terminating NULL */
        if ( s_fwrite(psiSrchIndex->pucIndexName, s_strlen(psiSrchIndex->pucIndexName) + 1, 1, pfSearchCacheFile) != 1 ) {
            iError = SRCH_CacheSaveFailed;
            goto bailFromiSrchCacheSaveSearchPostingsList;
        }
    }

    
    /* Write out the last update time - for tracking purposes */
    {
        /* Write out the last update time tag */
        sprintf(pucBuffer, "%c", SRCH_CACHE_LAST_UPDATE_TIME_TAG);
        if ( s_fwrite(pucBuffer, 1, 1, pfSearchCacheFile) != 1 ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to write to the search postings list cache file: '%s'.", pucSearchCacheFilePath);
            iError = SRCH_CacheSaveFailed;
            goto bailFromiSrchCacheSaveSearchPostingsList;
        }
    
        /* Encode the last update time buffer */
        pucBufferPtr = pucBuffer;
        UTL_NUM_WRITE_COMPRESSED_UINT((unsigned long)psiSrchIndex->tLastUpdateTime, pucBufferPtr);
        uiBufferLength = pucBufferPtr - pucBuffer;
    
        /* Write out the last update time buffer */
        if ( s_fwrite(pucBuffer, uiBufferLength, 1, pfSearchCacheFile) != 1 ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to write to the search postings list cache file: '%s'.", pucSearchCacheFilePath);
            iError = SRCH_CacheSaveFailed;
            goto bailFromiSrchCacheSaveSearchPostingsList;
        }
    }


    /* Write out the term if we have it - for tracking purposes */
    if ( bUtlStringsIsWideStringNULL(psptSrchParserTerm->pwcTerm) == false ) {    

        /* Write out the term tag */
        sprintf(pucBuffer, "%c", SRCH_CACHE_TERM_TAG);
        if ( s_fwrite(pucBuffer, 1, 1, pfSearchCacheFile) != 1 ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to write to the search postings list cache file: '%s'.", pucSearchCacheFilePath);
            iError = SRCH_CacheSaveFailed;
            goto bailFromiSrchCacheSaveSearchPostingsList;
        }

        /* Write the term */
        if ( fprintf(pfSearchCacheFile, "%ls", psptSrchParserTerm->pwcTerm) < 0 ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to write to the search postings list cache file: '%s'.", pucSearchCacheFilePath);
            iError = SRCH_CacheSaveFailed;
            goto bailFromiSrchCacheSaveSearchPostingsList;
        }
        
        /* Write the terminating null */
        if ( fputc('\0', pfSearchCacheFile) == EOF ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to write to the search postings list cache file: '%s'.", pucSearchCacheFilePath);
            iError = SRCH_CacheSaveFailed;
            goto bailFromiSrchCacheSaveSearchPostingsList;
        }
    }

    
    /* Write out the field name if we have it - for tracking purposes */
    if ( bUtlStringsIsWideStringNULL(psptSrchParserTerm->pwcFieldName) == false ) {    

        /* Write out the field name tag */
        sprintf(pucBuffer, "%c", SRCH_CACHE_FIELD_NAME_TAG);
        if ( s_fwrite(pucBuffer, 1, 1, pfSearchCacheFile) != 1 ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to write to the search postings list cache file: '%s'.", pucSearchCacheFilePath);
            iError = SRCH_CacheSaveFailed;
            goto bailFromiSrchCacheSaveSearchPostingsList;
        }

        /* Write the field name */
        if ( fprintf(pfSearchCacheFile, "%ls", psptSrchParserTerm->pwcFieldName) < 0 ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to write to the search postings list cache file: '%s'.", pucSearchCacheFilePath);
            iError = SRCH_CacheSaveFailed;
            goto bailFromiSrchCacheSaveSearchPostingsList;
        }
        
        /* Write the terminating null */
        if ( fputc('\0', pfSearchCacheFile) == EOF ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to write to the search postings list cache file: '%s'.", pucSearchCacheFilePath);
            iError = SRCH_CacheSaveFailed;
            goto bailFromiSrchCacheSaveSearchPostingsList;
        }
    }





    /* File type */
    if ( pscSrchCache->uiSearchCacheType == SRCH_CACHE_TYPE_FILE ) {
/*         iUtlLogInfo(UTL_LOG_CONTEXT, "Saved search postings list in file - psptSrchParserTerm->pwcTerm: '%ls' => '%s' (%lu).", psptSrchParserTerm->pwcTerm, pucSearchCacheFilePath, (unsigned long)s_ftell(pfSearchCacheFile)); */
/*         iUtlLogDebug(UTL_LOG_CONTEXT, "Saved search postings list in file: '%s'.", pucSearchCacheFilePath); */
    }



    /* Bail label */
    bailFromiSrchCacheSaveSearchPostingsList:


    /* Handle the error */
    if ( iError == SRCH_NoError ) {

        /* Release the lock we are currently holding */
        iSrchCacheUnlockCacheFile(pfSearchCacheFile);

        /* Close the search postings list cache file */
        s_fclose(pfSearchCacheFile);
    }
    else {

        /* File type */
        if ( pscSrchCache->uiSearchCacheType == SRCH_CACHE_TYPE_FILE ) {

            /* Remove the file if it was created */
            if ( pfSearchCacheFile != NULL ) {
            
                int    iLocalError = SRCH_NoError;

                /* Place an exclusive lock on the search postings list cache file, just warn if we cant */
                if ( (iLocalError = iSrchCacheLockCacheFile(pfSearchCacheFile, SRCH_CACHE_LOCK_EXCLUSIVE)) != SRCH_NoError ) {
                    iUtlLogWarn(UTL_LOG_CONTEXT, "Failed to place an exclusive lock on the search postings list cache file: '%s', srch error: %d.", pucSearchCacheFilePath, iLocalError);
                }
    
                /* Remove the file */
                s_remove(pucSearchCacheFilePath);
    
                /* Close the file  */
                s_fclose(pfSearchCacheFile);
            }
        }
    }


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchCacheGetSearchPostingsList()

    Purpose:    This function checks to see if there is a cache file for the
                passed postings list, and returns the contents of that cache file if
                it is there.

    Parameters: pvSrchCache                 search cache structure
                psiSrchIndex                search index structure
                uiLanguageID                language ID
                psptSrchParserTerm          search parser term structure
                ppsplSrchPostingsList       return pointer for the search postings list structure
                ppucSearchReportSnippet     return pointer for the search report snippet

    Globals:    none

    Returns:    SRCH error code

*/
int iSrchCacheGetSearchPostingsList
(
    void *pvSrchCache,
    struct srchIndex *psiSrchIndex, 
    unsigned int uiLanguageID, 
    struct srchParserTerm *psptSrchParserTerm, 
    struct srchPostingsList **ppsplSrchPostingsList,
    unsigned char **ppucSearchReportSnippet 
)
{

    int                         iError = SRCH_NoError;
    struct srchCache            *pscSrchCache = (struct srchCache *)pvSrchCache;
    unsigned char               pucSearchCacheFilePath[UTL_FILE_PATH_MAX + 1] = {'\0'};
    unsigned char               pucSHA1HexDigest[UTL_SHA1_HEX_DIGEST_LENGTH + 1] = {'\0'};
    FILE                        *pfSearchCacheFile = NULL;
    struct srchPostingsList     *psplSrchPostingsList = NULL;
    unsigned char               *pucSearchReportSnippet = NULL;
    off_t                       zSearchCacheFileDataLength = 0;
    unsigned char               *pucSearchCacheData = NULL;
    unsigned char               *pucSearchCacheDataPtr = NULL;
    unsigned char               *pucSearchCacheDataEndPtr = NULL;
    unsigned int                uiRequired = 0;


#if !defined(SRCH_CACHE_ENABLE_SEARCH_POSTINGS_CACHING)
    return (SRCH_CacheUnsupportedCache);
#endif    /* !defined(SRCH_CACHE_ENABLE_SEARCH_POSTINGS_CACHING) */


    /* Check the parameters */
    if ( pvSrchCache == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvSrchCache' parameter passed to 'iSrchCacheGetSearchPostingsList'."); 
        return (SRCH_CacheInvalidCache);
    }

    if ( psiSrchIndex == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'psiSrchIndex' parameter passed to 'iSrchCacheGetSearchPostingsList'."); 
        return (SRCH_InvalidIndex);
    }

    if ( psptSrchParserTerm == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'psptSrchParserTerm' parameter passed to 'iSrchCacheGetSearchPostingsList'."); 
        return (SRCH_CacheInvalidParserTerm);
    }

    if ( ppsplSrchPostingsList == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppsplSrchPostingsList' parameter passed to 'iSrchCacheGetSearchPostingsList'."); 
        return (SRCH_ReturnParameterError);
    }

    if ( ppucSearchReportSnippet == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppucSearchReportSnippet' parameter passed to 'iSrchCacheGetSearchPostingsList'."); 
        return (SRCH_ReturnParameterError);
    }


    /* Check the search cache structure */
    if ( pscSrchCache == NULL ) {
        return (SRCH_CacheInvalidCache);
    }


    /* Check the cache mode, we cant read a cache file if the cache is off */
    if ( pscSrchCache->uiSearchCacheMode == SRCH_CACHE_MODE_OFF ) {
        return (SRCH_CacheInvalidMode);
    }

    /* Check the cache type, we can't handle an unknown cache type */
    if ( pscSrchCache->uiSearchCacheType == SRCH_CACHE_TYPE_UNKNOWN ) {
        return (SRCH_CacheInvalidType);
    }



    /* Create an SHA1 hex digest for this search postings list */
    if ( (iError = iSrchCacheGetSearchPostingsListSHA1HexDigest(psiSrchIndex, uiLanguageID, psptSrchParserTerm, pucSHA1HexDigest)) != SRCH_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to generate SHA1 signature for search postings list cache file path, utl error: %d.", iError);
        return (iError);
    }

    /* Allocate the search postings list structure */
    if ( (psplSrchPostingsList = (struct srchPostingsList *)s_malloc(sizeof(struct srchPostingsList))) == NULL ) {
        return (SRCH_MemError);
    }


    /* File type */
    if ( pscSrchCache->uiSearchCacheType == SRCH_CACHE_TYPE_FILE ) {

        /* Get the search postings list cache file path for this search */
        if ( (iError = iSrchCacheGetCacheFilePath(pscSrchCache, psiSrchIndex, pucSHA1HexDigest, pucSearchCacheFilePath, UTL_FILE_PATH_MAX + 1)) != SRCH_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the search postings list cache file path, srch error: %d.", iError);
            iError = SRCH_CacheGetFailed;
            goto bailFromiSrchCacheGetSearchPostingsList;
        }
    
        /* Check that there is a search postings list cache file */
        if ( bUtlFileIsFile(pucSearchCacheFilePath) == false ) {
            iError = SRCH_CacheGetFailed;
            goto bailFromiSrchCacheGetSearchPostingsList;
        }
        
        /* Check that the search postings list cache file can be read */
        if ( bUtlFilePathRead(pucSearchCacheFilePath) == false ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to access the search postings list cache file: '%s'.", pucSearchCacheFilePath);
            iError = SRCH_CacheGetFailed;
            goto bailFromiSrchCacheGetSearchPostingsList;
        }
    
        /* Open the search postings list cache file */
        if ( (pfSearchCacheFile = s_fopen(pucSearchCacheFilePath, "r")) == NULL ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to open the search postings list cache file: '%s'.", pucSearchCacheFilePath);
            iError = SRCH_CacheGetFailed;
            goto bailFromiSrchCacheGetSearchPostingsList;
        }
    
        /* Get the search postings list cache file length */
        if ( (iError = iUtlFileGetFileLength(pfSearchCacheFile, &zSearchCacheFileDataLength)) != UTL_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the length of the search postings list cache file: '%s', utl error: %d.", pucSearchCacheFilePath, iError);
            iError = SRCH_CacheGetFailed;
            goto bailFromiSrchCacheGetSearchPostingsList;
        }
    
        /* Check the length of the cache file */
        if ( zSearchCacheFileDataLength == 0 ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to read from the search postings list cache file: '%s', empty file.", pucSearchCacheFilePath);
            iError = SRCH_CacheGetFailed;
            goto bailFromiSrchCacheGetSearchPostingsList;
        }
    
        /* Place a shared lock on the search postings list cache file */
        if ( (iError = iSrchCacheLockCacheFile(pfSearchCacheFile, SRCH_CACHE_LOCK_SHARED)) != SRCH_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to place a shared lock on the search postings list cache file: '%s', srch error: %d.", pucSearchCacheFilePath, iError);
            iError = SRCH_CacheGetFailed;
            goto bailFromiSrchCacheGetSearchPostingsList;
        }
        
        /* Map in the entire file */
        if ( (iError = iUtlFileMemoryMap(fileno(pfSearchCacheFile), 0, zSearchCacheFileDataLength, PROT_READ, (void **)&pucSearchCacheData)) != UTL_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to map in the search postings list cache file: '%s', utl error: %d.", pucSearchCacheFilePath, iError);
            iError = SRCH_CacheGetFailed;
            goto bailFromiSrchCacheGetSearchPostingsList;
        }

        /* Set the cache file pointers */
        pucSearchCacheDataPtr = pucSearchCacheData;
        pucSearchCacheDataEndPtr = pucSearchCacheData + zSearchCacheFileDataLength;

/*         iUtlLogInfo(UTL_LOG_CONTEXT, "Read search postings list from file - psptSrchParserTerm->pwcTerm: '%ls' => '%s' (%lu).", psptSrchParserTerm->pwcTerm, pucSearchCacheFilePath, (unsigned long)zSearchCacheFileDataLength); */
/*         iUtlLogDebug(UTL_LOG_CONTEXT, "Read search postings list from file: '%s'.", pucSearchCacheFilePath); */
    }


    ASSERT(pucSearchCacheDataPtr != NULL);
    ASSERT(pucSearchCacheDataEndPtr != NULL);
    ASSERT(pucSearchCacheDataPtr <= pucSearchCacheDataEndPtr);


    /* Loop reading the file while there is stuff to read */
    while ( pucSearchCacheDataPtr < pucSearchCacheDataEndPtr ) {

        /* Term type */
        if ( *pucSearchCacheDataPtr == SRCH_CACHE_TERM_TYPE_TAG ) {
            
            /* Increment the cache file pointer past the tag */
            pucSearchCacheDataPtr++;
    
            /* Decode the term type */
            UTL_NUM_READ_COMPRESSED_UINT(psplSrchPostingsList->uiTermType, pucSearchCacheDataPtr);
        }
        
        /* Term count */
        else if ( *pucSearchCacheDataPtr == SRCH_CACHE_TERM_COUNT_TAG ) {
            
            /* Increment the cache file pointer past the tag */
            pucSearchCacheDataPtr++;
    
            /* Decode the term count */
            UTL_NUM_READ_COMPRESSED_UINT(psplSrchPostingsList->uiTermCount, pucSearchCacheDataPtr);
        }
        
        /* Document count */
        else if ( *pucSearchCacheDataPtr == SRCH_CACHE_DOCUMENT_COUNT_TAG ) {
            
            /* Increment the cache file pointer past the tag */
            pucSearchCacheDataPtr++;
    
            /* Decode the document count */
            UTL_NUM_READ_COMPRESSED_UINT(psplSrchPostingsList->uiDocumentCount, pucSearchCacheDataPtr);
        }
        
        /* Required */
        else if ( *pucSearchCacheDataPtr == SRCH_CACHE_REQUIRE_TAG ) {
            
            /* Increment the cache file pointer past the tag */
            pucSearchCacheDataPtr++;
    
            /* Decode the required */
            UTL_NUM_READ_COMPRESSED_UINT(uiRequired, pucSearchCacheDataPtr);
            psplSrchPostingsList->bRequired = uiRequired ? 1 : 0;
        }
        
        /* Postings */
        else if ( *pucSearchCacheDataPtr == SRCH_CACHE_POSTINGS_ARRAY_TAG ) {
    
            struct srchPosting      *pspSrchPostingsPtr = NULL; 
            struct srchPosting      *pspSrchPostingsEndPtr = NULL; 
            
            /* Increment the cache file pointer past the tag */
            pucSearchCacheDataPtr++;
    
            /* Decode the postings length */
            UTL_NUM_READ_COMPRESSED_UINT(psplSrchPostingsList->uiSrchPostingsLength, pucSearchCacheDataPtr);
    
            /* Allocate the postings */
            if ( (psplSrchPostingsList->pspSrchPostings = (struct srchPosting *)s_malloc((size_t)(sizeof(struct srchPosting) * psplSrchPostingsList->uiSrchPostingsLength))) == NULL ) {
                iError = SRCH_MemError;
                goto bailFromiSrchCacheGetSearchPostingsList;
            }
    
            /* Read in the postings */
            for ( pspSrchPostingsPtr = psplSrchPostingsList->pspSrchPostings, pspSrchPostingsEndPtr = psplSrchPostingsList->pspSrchPostings + psplSrchPostingsList->uiSrchPostingsLength; 
                    pspSrchPostingsPtr < pspSrchPostingsEndPtr; pspSrchPostingsPtr++ ) {
    
                /* Decode the various fields */
                UTL_NUM_READ_COMPRESSED_UINT(pspSrchPostingsPtr->uiDocumentID, pucSearchCacheDataPtr);
                UTL_NUM_READ_COMPRESSED_UINT(pspSrchPostingsPtr->uiTermPosition, pucSearchCacheDataPtr);
                UTL_NUM_READ_COMPRESSED_FLOAT(pspSrchPostingsPtr->fWeight, pucSearchCacheDataPtr);
            }
        }
        
        /* Search report snippet */
        else if ( *pucSearchCacheDataPtr == SRCH_CACHE_SEACH_REPORT_SNIPPET_TAG ) {
        
            /* Increment the cache file pointer past the tag */
            pucSearchCacheDataPtr++;
    
            /* Duplicate the search report snippet */
            if ( (pucSearchReportSnippet = (unsigned char *)s_strdup(pucSearchCacheDataPtr)) == NULL ) {
                iError = SRCH_MemError;
                goto bailFromiSrchCacheGetSearchPostingsList;
            }

            /* Increment the pointer past the search report snippet */
            pucSearchCacheDataPtr = (unsigned char *)s_strchr(pucSearchCacheDataPtr, '\0') + 1;
        }
        
        /* Index name */
        else if ( *pucSearchCacheDataPtr == SRCH_CACHE_INDEX_PATH_TAG ) {
            
            /* Increment the cache file pointer past the tag */
            pucSearchCacheDataPtr++;    

/*             iUtlLogDebug(UTL_LOG_CONTEXT, "Index name: '%s'.", pucSearchCacheDataPtr); */

            /* Increment the pointer past the index name */
            pucSearchCacheDataPtr = (unsigned char *)s_strchr(pucSearchCacheDataPtr, '\0') + 1;
        }
        
        /* Last update time */
        else if ( *pucSearchCacheDataPtr == SRCH_CACHE_LAST_UPDATE_TIME_TAG ) {
            
            time_t    tLastUpdateTime = (time_t)0;
            
            /* Increment the cache file pointer past the tag */
            pucSearchCacheDataPtr++;
    
            /* Decode the last update time */
            UTL_NUM_READ_COMPRESSED_UINT(tLastUpdateTime, pucSearchCacheDataPtr);

/*             iUtlLogDebug(UTL_LOG_CONTEXT, "Last update time: %lu.", (unsigned long)tLastUpdateTime); */
        }

        /* Term */
        else if ( *pucSearchCacheDataPtr == SRCH_CACHE_TERM_TAG ) {
            
            /* Increment the cache file pointer past the tag */
            pucSearchCacheDataPtr++;    

/*             iUtlLogDebug(UTL_LOG_CONTEXT, "Term: '%ls'.", pucSearchCacheDataPtr); */

            /* Increment the pointer past the term */
            pucSearchCacheDataPtr = (unsigned char *)s_strchr(pucSearchCacheDataPtr, '\0') + 1;
        }
        
        /* Field name */
        else if ( *pucSearchCacheDataPtr == SRCH_CACHE_FIELD_NAME_TAG ) {
            
            /* Increment the cache file pointer past the tag */
            pucSearchCacheDataPtr++;    

/*             iUtlLogDebug(UTL_LOG_CONTEXT, "Field name: '%ls'.", pucSearchCacheDataPtr); */

            /* Increment the pointer past the field name */
            pucSearchCacheDataPtr = (unsigned char *)s_strchr(pucSearchCacheDataPtr, '\0') + 1;
        }
        
        /* Invalid tag, skip over it for now, but where does it end?? */
        else {
        
            /* Increment the cache file pointer past the tag */
            pucSearchCacheDataPtr++;
        }
        
    }



    /* Bail label */
    bailFromiSrchCacheGetSearchPostingsList:


    /* File type */
    if ( pscSrchCache->uiSearchCacheType == SRCH_CACHE_TYPE_FILE ) {

        /* Release the memory mapping */    
        iUtlFileMemoryUnMap(pucSearchCacheData, zSearchCacheFileDataLength);
        pucSearchCacheData = NULL;
    }


    /* Handle the error */
    if ( iError == SRCH_NoError ) {
        
        /* File type */
        if ( pscSrchCache->uiSearchCacheType == SRCH_CACHE_TYPE_FILE ) {

#if defined(SRCH_CACHE_ENABLE_UTIME_CACHE_FILE_UPDATE)
            /* Set the access and modification times on the search postings list cache file to now */
            if  ( pscSrchCache->uiSearchCacheMode == SRCH_CACHE_MODE_READ_WRITE ) {

                int     iLocalError = UTL_NoError;

                if ( (iLocalError = iUtlFileSetPathUTime(pucSearchCacheFilePath)) != UTL_NoError ) {
                    iUtlLogError(UTL_LOG_CONTEXT, "Failed to set the access and modification times on the search postings list cache file: '%s', utl error: %d.", 
                            pucSearchCacheFilePath, iLocalError);
                }
            }
#endif    /* defined(SRCH_CACHE_ENABLE_UTIME_CACHE_FILE_UPDATE) */

            /* Release the lock we are currently holding */
            iSrchCacheUnlockCacheFile(pfSearchCacheFile);

            /* Close the search postings list cache file */
            s_fclose(pfSearchCacheFile);

/*             iUtlLogDebug(UTL_LOG_CONTEXT, "Search results were returned from the search postings list cache file: '%s'.", pucSearchCacheFilePath); */
        }

        /* Set the return pointers */
        *ppsplSrchPostingsList = psplSrchPostingsList;
        *ppucSearchReportSnippet = pucSearchReportSnippet;

#if defined(SRCH_CACHE_ENABLE_SEARCH_POSTINGS_LIST_CACHE_LOGGING) || defined(SRCH_CACHE_ENABLE_ALL_CACHE_LOGGING)
        iUtlLogInfo(UTL_LOG_CONTEXT, "Search postings list was served from cache.");
#endif    /* defined(SRCH_CACHE_ENABLE_SEARCH_POSTINGS_LIST_CACHE_LOGGING) || defined(SRCH_CACHE_ENABLE_ALL_CACHE_LOGGING) */
    
    }
    else {
        
        /* Free any allocated resources */
        iSrchPostingFreeSrchPostingsList(psplSrchPostingsList);
        psplSrchPostingsList = NULL;

        s_free(pucSearchReportSnippet);
    
        /* File type */
        if ( pscSrchCache->uiSearchCacheType == SRCH_CACHE_TYPE_FILE ) {

            /* Remove the file if it is open */
            if ( (pfSearchCacheFile != NULL) && (pscSrchCache->uiSearchCacheMode == SRCH_CACHE_MODE_READ_WRITE) ) {
            
                int     iLocalError = SRCH_NoError;

                /* Place an exclusive lock on the search postings list cache file, just fall through if we failed */
                if ( (iError = iSrchCacheLockCacheFile(pfSearchCacheFile, SRCH_CACHE_LOCK_EXCLUSIVE)) != SRCH_NoError ) {
                    iUtlLogWarn(UTL_LOG_CONTEXT, "Failed to place an exclusive lock on the search postings list cache file: '%s', srch error: %d.", 
                            pucSearchCacheFilePath, iLocalError);
                }

                /* Remove the file */
/*                 s_remove(pucSearchCacheFilePath); */

                /* Close the file  */
                s_fclose(pfSearchCacheFile);
            }
        }
    }


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchCacheSaveSearchWeight()

    Purpose:    This function saves a search weight structure in the cache.

    Parameters: pvSrchCache                 search cache structure
                psiSrchIndex                search index structure
                pwcWeightName               search weight name
                pswSrchWeight               search weight structure
                pucSearchReportSnippet      search report snippet (optional)

    Globals:    none

    Returns:    SRCH error code

*/
int iSrchCacheSaveSearchWeight
(
    void *pvSrchCache,
    struct srchIndex *psiSrchIndex,
    wchar_t *pwcWeightName,
    struct srchWeight *pswSrchWeight,
    unsigned char *pucSearchReportSnippet
)
{

    int                 iError = SRCH_NoError;
    struct srchCache    *pscSrchCache = (struct srchCache *)pvSrchCache;
    unsigned char       pucSearchCacheFilePath[UTL_FILE_PATH_MAX + 1] = {'\0'};
    unsigned char       pucSHA1HexDigest[UTL_SHA1_HEX_DIGEST_LENGTH + 1] = {'\0'};
    FILE                *pfSearchCacheFile = NULL;
    unsigned char       pucBuffer[BUFSIZ + 1] = {'\0'};
    unsigned char       *pucBufferPtr = NULL;
    unsigned int        uiBufferLength = 0;


#if !defined(SRCH_CACHE_ENABLE_SEARCH_WEIGHTS_CACHING)
    return (SRCH_CacheUnsupportedCache);
#endif    /* !defined(SRCH_CACHE_ENABLE_SEARCH_WEIGHTS_CACHING) */


    /* Check the parameters */
    if ( pvSrchCache == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvSrchCache' parameter passed to 'iSrchCacheSaveSearchWeight'."); 
        return (SRCH_CacheInvalidCache);
    }

    if ( psiSrchIndex == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'psiSrchIndex' parameter passed to 'iSrchCacheGetSearchPostingsList'."); 
        return (SRCH_InvalidIndex);
    }

    if ( bUtlStringsIsWideStringNULL(pwcWeightName) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pwcWeightName' parameter passed to 'iSrchCacheSaveSearchWeight'."); 
        return (SRCH_CacheInvalidWeightName);
    }

    if ( pswSrchWeight == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pswSrchWeight' parameter passed to 'iSrchCacheSaveSearchWeight'."); 
        return (SRCH_CacheInvalidWeight);
    }


    /* Check the cache mode, we can't save a cache file if we cant read and write */
    if ( pscSrchCache->uiSearchCacheMode != SRCH_CACHE_MODE_READ_WRITE ) {
        return (SRCH_CacheInvalidMode);
    }

    /* Check the cache type, we can't handle an unknown cache type */
    if ( pscSrchCache->uiSearchCacheType == SRCH_CACHE_TYPE_UNKNOWN ) {
        return (SRCH_CacheInvalidType);
    }



    /* Create an SHA1 hex digest for this search weight structure */
    if ( (iError = iSrchCacheGetSearchWeightSHA1HexDigest(psiSrchIndex, pwcWeightName, pucSHA1HexDigest)) != SRCH_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to generate SHA1 signature for the search weight cache object, srch error: %d.", iError);
        return (iError);
    }


    /* File type */
    if ( pscSrchCache->uiSearchCacheType == SRCH_CACHE_TYPE_FILE ) {

        /* Get the search weight cache file path for this search */
        if ( (iError = iSrchCacheGetCacheFilePath(pscSrchCache, psiSrchIndex, pucSHA1HexDigest, pucSearchCacheFilePath, UTL_FILE_PATH_MAX + 1)) != SRCH_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the search weight cache file path, srch error: %d.", iError);
            goto bailFromiSrchCacheSaveSearchWeight;
        }

        /* Open the search weight cache file */
        if ( (pfSearchCacheFile = s_fopen(pucSearchCacheFilePath, "w")) == NULL ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to create the search weight cache file: '%s'.", pucSearchCacheFilePath);
            iError = SRCH_CacheSaveFailed;
            goto bailFromiSrchCacheSaveSearchWeight;
        }
    }



    /* Place an exclusive lock on the file */
    if ( (iError = iSrchCacheLockCacheFile(pfSearchCacheFile, SRCH_CACHE_LOCK_EXCLUSIVE)) != SRCH_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to place an exclusive lock on the search weight cache file: '%s', srch error: %d.", pucSearchCacheFilePath, iError);
        goto bailFromiSrchCacheSaveSearchWeight;
    }


    /* Write out the search report snippet if we have it */
    if ( bUtlStringsIsStringNULL(pucSearchReportSnippet) == false ) {    
    
        /* Write out the search report snippet tag */
        sprintf(pucBuffer, "%c", SRCH_CACHE_SEACH_REPORT_SNIPPET_TAG);
        if ( s_fwrite(pucBuffer, 1, 1, pfSearchCacheFile) != 1 ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to write to the search weight cache file: '%s'.", pucSearchCacheFilePath);
            iError = SRCH_CacheSaveFailed;
            goto bailFromiSrchCacheSaveSearchWeight;
        }

        /* Write the search report snippet - include terminating NULL */
        if ( s_fwrite(pucSearchReportSnippet, s_strlen(pucSearchReportSnippet) + 1, 1, pfSearchCacheFile) != 1 ) {
            iError = SRCH_CacheSaveFailed;
            goto bailFromiSrchCacheSaveSearchWeight;
        }
    }


    /* Write out the weights array length tag */
    sprintf(pucBuffer, "%c", SRCH_CACHE_WEIGHTS_ARRAY_LENGTH_TAG);
    if ( s_fwrite(pucBuffer, 1, 1, pfSearchCacheFile) != 1 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to write to the search short results cache file: '%s'.", pucSearchCacheFilePath);
        iError = SRCH_CacheSaveFailed;
        goto bailFromiSrchCacheSaveSearchWeight;
    }

    /* Encode the weights array length buffer */
    pucBufferPtr = pucBuffer;
    UTL_NUM_WRITE_COMPRESSED_UINT(pswSrchWeight->uiWeightsLength, pucBufferPtr);
    uiBufferLength = pucBufferPtr - pucBuffer;

    /* Write out the weights array length buffer */
    if ( s_fwrite(pucBuffer, uiBufferLength, 1, pfSearchCacheFile) != 1 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to write to the search short results cache file: '%s'.", pucSearchCacheFilePath);
        iError = SRCH_CacheSaveFailed;
        goto bailFromiSrchCacheSaveSearchWeight;
    }


    /* Write out the weights array tag */
    sprintf(pucBuffer, "%c", SRCH_CACHE_WEIGHTS_ARRAY_TAG);
    if ( s_fwrite(pucBuffer, 1, 1, pfSearchCacheFile) != 1 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to write to the search weight cache file: '%s'.", pucSearchCacheFilePath);
        iError = SRCH_CacheSaveFailed;
        goto bailFromiSrchCacheSaveSearchWeight;
    }

    /* Write out the weight array to the file */
    if ( s_fwrite((unsigned char *)pswSrchWeight->pfWeights, (pswSrchWeight->uiWeightsLength * sizeof(float)), 1, pfSearchCacheFile) != 1 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to write to the search weight cache file: '%s'.", pucSearchCacheFilePath);
        iError = SRCH_CacheSaveFailed;
        goto bailFromiSrchCacheSaveSearchWeight;
    }


    /* File type */
    if ( pscSrchCache->uiSearchCacheType == SRCH_CACHE_TYPE_FILE ) {
/*         iUtlLogInfo(UTL_LOG_CONTEXT, "Saved search weight cache in file - pwcWeightName: '%ls' => '%s' (%lu).", pwcWeightName, pucSearchCacheFilePath, (unsigned long)s_ftell(pfSearchCacheFile)); */
/*         iUtlLogDebug(UTL_LOG_CONTEXT, "Saved search weight cache in file: '%s'.", pucSearchCacheFilePath); */
    }



    /* Bail label */
    bailFromiSrchCacheSaveSearchWeight:


    /* Handle the error */
    if ( iError == SRCH_NoError ) {

        /* Release the lock we are currently holding */
        iSrchCacheUnlockCacheFile(pfSearchCacheFile);

        /* Close the search weight cache file */
        s_fclose(pfSearchCacheFile);
    }
    else {

        /* File type */
        if ( pscSrchCache->uiSearchCacheType == SRCH_CACHE_TYPE_FILE ) {

            /* Remove the file if it was created */
            if ( pfSearchCacheFile != NULL ) {

                int     iLocalError = SRCH_NoError;

                /* Place an exclusive lock on the search weight cache file, just warn if we cant */
                if ( (iLocalError = iSrchCacheLockCacheFile(pfSearchCacheFile, SRCH_CACHE_LOCK_EXCLUSIVE)) != SRCH_NoError ) {
                    iUtlLogWarn(UTL_LOG_CONTEXT, "Failed to place an exclusive lock on the search weight cache file: '%s', srch error: %d.", 
                            pucSearchCacheFilePath, iLocalError);
                }
    
                /* Remove the file */
                s_remove(pucSearchCacheFilePath);
    
                /* Close the file  */
                s_fclose(pfSearchCacheFile);
            }
        }
    }


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchCacheGetSearchWeight()

    Purpose:    This function checks to see if there is a cache file for the
                passed search weight structure, and returns the contents of 
                that cache file if it is there.

    Parameters: pvSrchCache                 search cache structure
                psiSrchIndex                search index structure
                pwcWeightName               search weight name
                ppswSrchWeight              return pointer for the search weight structure
                ppucSearchReportSnippet     return pointer for the search report snippet

    Globals:    none

    Returns:    SRCH error code

*/
int iSrchCacheGetSearchWeight
(
    void *pvSrchCache,
    struct srchIndex *psiSrchIndex, 
    wchar_t *pwcWeightName, 
    struct srchWeight **ppswSrchWeight,
    unsigned char **ppucSearchReportSnippet 
)
{

    int                 iError = SRCH_NoError;
    struct srchCache    *pscSrchCache = (struct srchCache *)pvSrchCache;
    unsigned char       pucSearchCacheFilePath[UTL_FILE_PATH_MAX + 1] = {'\0'};
    unsigned char       pucSHA1HexDigest[UTL_SHA1_HEX_DIGEST_LENGTH + 1] = {'\0'};
    FILE                *pfSearchCacheFile = NULL;
    off_t               zSearchCacheFileDataLength = 0;

    unsigned char       *pucSearchCacheData = NULL;
    unsigned char       *pucSearchCacheDataPtr = NULL;
    unsigned char       *pucSearchCacheDataEndPtr = NULL;

    struct srchWeight   *pswSrchWeight = NULL;
    unsigned char       *pucSearchReportSnippet = NULL;


#if !defined(SRCH_CACHE_ENABLE_SEARCH_WEIGHTS_CACHING)
    return (SRCH_CacheUnsupportedCache);
#endif    /* !defined(SRCH_CACHE_ENABLE_SEARCH_WEIGHTS_CACHING) */


    /* Check the parameters */
    if ( pvSrchCache == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvSrchCache' parameter passed to 'iSrchCacheGetSearchWeight'."); 
        return (SRCH_CacheInvalidCache);
    }

    if ( psiSrchIndex == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'psiSrchIndex' parameter passed to 'iSrchCacheGetSearchWeight'."); 
        return (SRCH_InvalidIndex);
    }

    if ( bUtlStringsIsWideStringNULL(pwcWeightName) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pwcWeightName' parameter passed to 'iSrchCacheGetSearchWeight'."); 
        return (SRCH_CacheInvalidWeightName);
    }

    if ( ppswSrchWeight == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppswSrchWeight' parameter passed to 'iSrchCacheGetSearchWeight'."); 
        return (SRCH_ReturnParameterError);
    }

    if ( ppucSearchReportSnippet == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppucSearchReportSnippet' parameter passed to 'iSrchCacheGetSearchWeight'."); 
        return (SRCH_ReturnParameterError);
    }


    /* Check the cache mode, we cant read a cache file if the cache is off */
    if ( pscSrchCache->uiSearchCacheMode == SRCH_CACHE_MODE_OFF ) {
        return (SRCH_CacheInvalidMode);
    }

    /* Check the cache type, we can't handle an unknown cache type */
    if ( pscSrchCache->uiSearchCacheType == SRCH_CACHE_TYPE_UNKNOWN ) {
        return (SRCH_CacheInvalidType);
    }



    /* Create an SHA1 hex digest for this search weight structure */
    if ( (iError = iSrchCacheGetSearchWeightSHA1HexDigest(psiSrchIndex, pwcWeightName, pucSHA1HexDigest)) != SRCH_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to generate SHA1 signature for search weight cache file path, srch error: %d.", iError);
        return (iError);
    }


    /* Allocate the search weight structure */
    if ( (pswSrchWeight = (struct srchWeight *)s_malloc(sizeof(struct srchWeight))) == NULL ) {
        return (SRCH_MemError);
    }


    /* File type */
    if ( pscSrchCache->uiSearchCacheType == SRCH_CACHE_TYPE_FILE ) {

        /* Get the search weight cache file path for this search */
        if ( (iError = iSrchCacheGetCacheFilePath(pscSrchCache, psiSrchIndex, pucSHA1HexDigest, pucSearchCacheFilePath, UTL_FILE_PATH_MAX + 1)) != SRCH_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the search weight cache file path, srch error: %d.", iError);
            goto bailFromiSrchCacheGetSearchWeight;
        }
    
        /* Check that there is a search weight cache file */
        if ( bUtlFileIsFile(pucSearchCacheFilePath) == false ) {
            iError = SRCH_CacheGetFailed;
            goto bailFromiSrchCacheGetSearchWeight;
        }
        
        /* Check that the search weight cache file can be read */
        if ( bUtlFilePathRead(pucSearchCacheFilePath) == false ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to access the search weight cache file: '%s'.", pucSearchCacheFilePath);
            iError = SRCH_CacheGetFailed;
            goto bailFromiSrchCacheGetSearchWeight;
        }
    
        /* Open the search weight cache file */
        if ( (pfSearchCacheFile = s_fopen(pucSearchCacheFilePath, "r")) == NULL ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to open the search weight cache file: '%s'.", pucSearchCacheFilePath);
            iError = SRCH_CacheGetFailed;
            goto bailFromiSrchCacheGetSearchWeight;
        }
    
        /* Get the search weight cache file length */
        if ( (iError = iUtlFileGetFileLength(pfSearchCacheFile, &zSearchCacheFileDataLength)) != UTL_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the length of the search weight cache file: '%s', utl error: %d.", pucSearchCacheFilePath, iError);
            iError = SRCH_CacheGetFailed;
            goto bailFromiSrchCacheGetSearchWeight;
        }
    
        /* Check the length of the cache file */
        if ( zSearchCacheFileDataLength == 0 ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to read from the search weight cache file: '%s', empty file.", pucSearchCacheFilePath);
            iError = SRCH_CacheGetFailed;
            goto bailFromiSrchCacheGetSearchWeight;
        }
    
        /* Place a shared lock on the search weight cache file */
        if ( (iError = iSrchCacheLockCacheFile(pfSearchCacheFile, SRCH_CACHE_LOCK_SHARED)) != SRCH_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to place a shared lock on the search weight cache file: '%s', srch error: %d.", pucSearchCacheFilePath, iError);
            iError = SRCH_CacheGetFailed;
            goto bailFromiSrchCacheGetSearchWeight;
        }
        
        /* Map in the entire file */
        if ( (iError = iUtlFileMemoryMap(fileno(pfSearchCacheFile), 0, zSearchCacheFileDataLength, PROT_READ, (void **)&pucSearchCacheData)) != UTL_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to map in the search weight cache file: '%s', utl error: %d.", pucSearchCacheFilePath, iError);
            iError = SRCH_CacheGetFailed;
            goto bailFromiSrchCacheGetSearchWeight;
        }

        /* Set the cache file pointers */
        pucSearchCacheDataPtr = pucSearchCacheData;
        pucSearchCacheDataEndPtr = pucSearchCacheData + zSearchCacheFileDataLength;

/*         iUtlLogInfo(UTL_LOG_CONTEXT, "Read search weight cache from file - pwcWeightName: '%ls' => '%s' (%lu).", pwcWeightName, pucSearchCacheFilePath, (unsigned long)zSearchCacheFileDataLength); */
/*         iUtlLogDebug(UTL_LOG_CONTEXT, "Read search weight cache from file: '%s'.", pucSearchCacheFilePath); */
    }



    ASSERT(pucSearchCacheDataPtr != NULL);
    ASSERT(pucSearchCacheDataEndPtr != NULL);
    ASSERT(pucSearchCacheDataPtr <= pucSearchCacheDataEndPtr);


    /* Loop reading the file while there is stuff to read */
    while ( pucSearchCacheDataPtr < pucSearchCacheDataEndPtr ) {

        /* Search report snippet */
        if ( *pucSearchCacheDataPtr == SRCH_CACHE_SEACH_REPORT_SNIPPET_TAG ) {
        
            /* Increment the cache file pointer past the tag */
            pucSearchCacheDataPtr++;
    
            /* Duplicate the search report snippet */
            if ( (pucSearchReportSnippet = (unsigned char *)s_strdup(pucSearchCacheDataPtr)) == NULL ) {
                iError = SRCH_MemError;
                goto bailFromiSrchCacheGetSearchWeight;
            }

            /* Increment the pointer past the search report snippet */
            pucSearchCacheDataPtr = (unsigned char *)s_strchr(pucSearchCacheDataPtr, '\0') + 1;
        }
        
        /* Weights array length */
        else if ( *pucSearchCacheDataPtr == SRCH_CACHE_WEIGHTS_ARRAY_LENGTH_TAG ) {
            
            /* Increment the cache file pointer past the tag */
            pucSearchCacheDataPtr++;
    
            /* Decode the search results total */
            UTL_NUM_READ_COMPRESSED_UINT(pswSrchWeight->uiWeightsLength, pucSearchCacheDataPtr);
        }
        
        /* Weights array */
        else if ( *pucSearchCacheDataPtr == SRCH_CACHE_WEIGHTS_ARRAY_TAG ) {

            /* Increment the cache file pointer past the tag */
            pucSearchCacheDataPtr++;
            
            /* File type */
            if ( pscSrchCache->uiSearchCacheType == SRCH_CACHE_TYPE_FILE ) {

                /* Map in the weights array */
                if ( (iError = iUtlFileMemoryMap(fileno(pfSearchCacheFile), pucSearchCacheDataPtr - pucSearchCacheData, (pswSrchWeight->uiWeightsLength * sizeof(float)), 
                        PROT_READ, (void **)&pswSrchWeight->pfWeights)) != UTL_NoError ) {
                    iUtlLogError(UTL_LOG_CONTEXT, "Failed to map in the search weight cache file: '%s', utl error: %d.", pucSearchCacheFilePath, iError);
                    iError = SRCH_CacheGetFailed;
                    goto bailFromiSrchCacheGetSearchWeight;
                }

                pswSrchWeight->bMappedAllocationFlag = true;
            }
        }

        /* Invalid tag, skip over it for now, but where does it end?? */
        else {
        
            /* Increment the cache file pointer past the tag */
            pucSearchCacheDataPtr++;
        }

    }



    /* Bail label */
    bailFromiSrchCacheGetSearchWeight:


    /* File type */
    if ( pscSrchCache->uiSearchCacheType == SRCH_CACHE_TYPE_FILE ) {

        /* Release the memory mapping */    
        iUtlFileMemoryUnMap(pucSearchCacheData, zSearchCacheFileDataLength);
        pucSearchCacheData = NULL;
    }


    /* Handle the error */
    if ( iError == SRCH_NoError ) {
        
        /* File type */
        if ( pscSrchCache->uiSearchCacheType == SRCH_CACHE_TYPE_FILE ) {

#if defined(SRCH_CACHE_ENABLE_UTIME_CACHE_FILE_UPDATE)
            /* Set the access and modification times on the search weight cache file to now */
            if  ( pscSrchCache->uiSearchCacheMode == SRCH_CACHE_MODE_READ_WRITE ) {
            
                int     iLocalError = UTL_NoError;
            
                if ( (iLocalError = iUtlFileSetPathUTime(pucSearchCacheFilePath)) != UTL_NoError ) {
                    iUtlLogError(UTL_LOG_CONTEXT, "Failed to set the access and modification times on the bitmap cache file: '%s', utl error: %d.", 
                            pucSearchCacheFilePath, iLocalError);
                }
            }
#endif    /* defined(SRCH_CACHE_ENABLE_UTIME_CACHE_FILE_UPDATE) */

            /* Release the lock we are currently holding */
            iSrchCacheUnlockCacheFile(pfSearchCacheFile);

            /* Close the search weight cache file */
            s_fclose(pfSearchCacheFile);

#if defined(SRCH_CACHE_ENABLE_SEARCH_WEIGHT_CACHE_LOGGING) || defined(SRCH_CACHE_ENABLE_ALL_CACHE_LOGGING)
            iUtlLogInfo(UTL_LOG_CONTEXT, "Search weight was served from cache.");
#endif    /* defined(SRCH_CACHE_ENABLE_SEARCH_WEIGHT_CACHE_LOGGING) || defined(SRCH_CACHE_ENABLE_ALL_CACHE_LOGGING) */

        }

        /* Set the return pointers */
        *ppswSrchWeight = pswSrchWeight;
        *ppucSearchReportSnippet = pucSearchReportSnippet;
    }
    else {
        
        /* Free any allocated resources */
        iSrchWeightFree(pswSrchWeight);
        pswSrchWeight = NULL;
    
        /* File type */
        if ( pscSrchCache->uiSearchCacheType == SRCH_CACHE_TYPE_FILE ) {

            /* Remove the file if it is open */
            if ( (pfSearchCacheFile != NULL) && (pscSrchCache->uiSearchCacheMode == SRCH_CACHE_MODE_READ_WRITE) ) {
                
                int     iLocalError = SRCH_NoError;

                /* Place an exclusive lock on the search weight cache file, just fall through if we failed */
                if ( (iError = iSrchCacheLockCacheFile(pfSearchCacheFile, SRCH_CACHE_LOCK_EXCLUSIVE)) != SRCH_NoError ) {
                    iUtlLogWarn(UTL_LOG_CONTEXT, "Failed to place an exclusive lock on the search weight cache file: '%s', srch error: %d.", 
                            pucSearchCacheFilePath, iLocalError);
                }

                /* Remove the file */
/*                 s_remove(pucSearchCacheFilePath); */

                /* Close the file  */
                s_fclose(pfSearchCacheFile);
            }
        }
    }


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchCacheSaveSearchBitmap()

    Purpose:    This function saves a search bitmap in the cache.

    Parameters: pvSrchCache         search cache structure
                psiSrchIndex        search index structure
                pwcBitmapName       bitmap name
                tBitmapLastUpdate   bitmap last update
                psbSrchBitmap       search bitmap structure

    Globals:    none

    Returns:    SRCH error code

*/
int iSrchCacheSaveSearchBitmap
(
    void *pvSrchCache,
    struct srchIndex *psiSrchIndex,
    wchar_t *pwcBitmapName,
    time_t tBitmapLastUpdate, 
    struct srchBitmap *psbSrchBitmap 
)
{

    int                 iError = SRCH_NoError;
    struct srchCache    *pscSrchCache = (struct srchCache *)pvSrchCache;
    unsigned char       pucSearchCacheFilePath[UTL_FILE_PATH_MAX + 1] = {'\0'};
    unsigned char       pucSHA1HexDigest[UTL_SHA1_HEX_DIGEST_LENGTH + 1] = {'\0'};
    FILE                *pfSearchCacheFile = NULL;


#if !defined(SRCH_CACHE_ENABLE_SEARCH_BITMAPS_CACHING)
    return (SRCH_CacheUnsupportedCache);
#endif    /* !defined(SRCH_CACHE_ENABLE_SEARCH_BITMAPS_CACHING) */


    /* Check the parameters */
    if ( pvSrchCache == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvSrchCache' parameter passed to 'iSrchCacheSaveSearchBitmap'."); 
        return (SRCH_CacheInvalidCache);
    }

    if ( psiSrchIndex == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'psiSrchIndex' parameter passed to 'iSrchCacheSaveSearchBitmap'."); 
        return (SRCH_InvalidIndex);
    }

    if ( bUtlStringsIsWideStringNULL(pwcBitmapName) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pwcBitmapName' parameter passed to 'iSrchCacheSaveSearchBitmap'."); 
        return (SRCH_CacheInvalidBitmapName);
    }

    if ( psbSrchBitmap == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'psbSrchBitmap' parameter passed to 'iSrchCacheSaveSearchBitmap'."); 
        return (SRCH_CacheInvalidBitmap);
    }


    /* Check the cache mode, we can't save a cache file if we cant read and write */
    if ( pscSrchCache->uiSearchCacheMode != SRCH_CACHE_MODE_READ_WRITE ) {
        return (SRCH_CacheInvalidMode);
    }

    /* Check the cache type, we can't handle an unknown cache type */
    if ( pscSrchCache->uiSearchCacheType == SRCH_CACHE_TYPE_UNKNOWN ) {
        return (SRCH_CacheInvalidType);
    }



    /* Create an SHA1 hex digest for this search bitmap*/
    if ( (iError = iSrchCacheGetSearchBitmapSHA1HexDigest(psiSrchIndex, pwcBitmapName, tBitmapLastUpdate, pucSHA1HexDigest)) != SRCH_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to generate SHA1 signature for the search bitmap cache object, srch error: %d.", iError);
        return (iError);
    }


    /* File type */
    if ( pscSrchCache->uiSearchCacheType == SRCH_CACHE_TYPE_FILE ) {

        /* Get the search bitmap cache file path for this search */
        if ( (iError = iSrchCacheGetCacheFilePath(pscSrchCache, psiSrchIndex, pucSHA1HexDigest, pucSearchCacheFilePath, UTL_FILE_PATH_MAX + 1)) != SRCH_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the search bitmap cache file path, srch error: %d.", iError);
            goto bailFromiSrchCacheSaveSearchBitmap;
        }

        /* Open the search bitmap cache file */
        if ( (pfSearchCacheFile = s_fopen(pucSearchCacheFilePath, "w")) == NULL ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to create the search bitmap cache file: '%s'.", pucSearchCacheFilePath);
            iError = SRCH_CacheSaveFailed;
            goto bailFromiSrchCacheSaveSearchBitmap;
        }
    }


    /* Place an exclusive lock on the file */
    if ( (iError = iSrchCacheLockCacheFile(pfSearchCacheFile, SRCH_CACHE_LOCK_EXCLUSIVE)) != SRCH_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to place an exclusive lock on the search bitmap cache file: '%s', srch error: %d.", pucSearchCacheFilePath, iError);
        goto bailFromiSrchCacheSaveSearchBitmap;
    }


    /* Write out the search bitmap */
    if ( s_fwrite(psbSrchBitmap->pucBitmap, UTL_BITMAP_GET_BITMAP_BYTE_LENGTH(psbSrchBitmap->uiBitmapLength), 1, pfSearchCacheFile) != 1 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to write to the search bitmap cache file: '%s'.", pucSearchCacheFilePath);
        iError = SRCH_CacheSaveFailed;
        goto bailFromiSrchCacheSaveSearchBitmap;
    }



    /* File type */
    if ( pscSrchCache->uiSearchCacheType == SRCH_CACHE_TYPE_FILE ) {
/*         iUtlLogInfo(UTL_LOG_CONTEXT, "Saved search bitmap cache in file - pwcBitmapName: '%ls' => '%s' (%lu).", pwcBitmapName, pucSearchCacheFilePath, (unsigned long)s_ftell(pfSearchCacheFile)); */
/*         iUtlLogDebug(UTL_LOG_CONTEXT, "Saved search bitmap cache in file: '%s'.", pucSearchCacheFilePath); */
    }



    /* Bail label */
    bailFromiSrchCacheSaveSearchBitmap:


    /* Handle the error */
    if ( iError == SRCH_NoError ) {

        /* Release the lock we are currently holding */
        iSrchCacheUnlockCacheFile(pfSearchCacheFile);

        /* Close the search bitmap cache file */
        s_fclose(pfSearchCacheFile);
    }
    else {

        /* File type */
        if ( pscSrchCache->uiSearchCacheType == SRCH_CACHE_TYPE_FILE ) {

            /* Remove the file if it was created */
            if ( pfSearchCacheFile != NULL ) {
            
                int     iLocalError = SRCH_NoError;

                /* Place an exclusive lock on the search bitmap cache file, just warn if we cant */
                if ( (iLocalError = iSrchCacheLockCacheFile(pfSearchCacheFile, SRCH_CACHE_LOCK_EXCLUSIVE)) != SRCH_NoError ) {
                    iUtlLogWarn(UTL_LOG_CONTEXT, "Failed to place an exclusive lock on the search bitmap cache file: '%s', srch error: %d.", 
                            pucSearchCacheFilePath, iLocalError);
                }
    
                /* Remove the file */
                s_remove(pucSearchCacheFilePath);
        
                /* Close the file  */
                s_fclose(pfSearchCacheFile);
            }
        }
    }


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchCacheGetSearchBitmap()

    Purpose:    This function checks to see if there is a cache file for the
                passed search bitmap, and returns the contents of that cache file if
                it is there.

    Parameters: pvSrchCache         search cache structure
                psiSrchIndex        search index structure
                pwcBitmapName       bitmap name
                tBitmapLastUpdate   bitmap last update
                ppsbSrchBitmap      return pointer for the search bitmap structure 

    Globals:    none

    Returns:    SRCH error code

*/
int iSrchCacheGetSearchBitmap
(
    void *pvSrchCache,
    struct srchIndex *psiSrchIndex, 
    wchar_t *pwcBitmapName, 
    time_t tBitmapLastUpdate, 
    struct srchBitmap **ppsbSrchBitmap
)
{

    int                 iError = SRCH_NoError;
    struct srchCache    *pscSrchCache = (struct srchCache *)pvSrchCache;
    unsigned char       pucSearchCacheFilePath[UTL_FILE_PATH_MAX + 1] = {'\0'};
    unsigned char       pucSHA1HexDigest[UTL_SHA1_HEX_DIGEST_LENGTH + 1] = {'\0'};
    FILE                *pfSearchCacheFile = NULL;
    struct srchBitmap   *psbSrchBitmap = NULL;
    unsigned char       *pucBitmap = NULL;
    unsigned int        uiBitmapLength = 0;
    boolean             bMappedAllocationFlag = false;


#if !defined(SRCH_CACHE_ENABLE_SEARCH_BITMAPS_CACHING)
    return (SRCH_CacheUnsupportedCache);
#endif    /* !defined(SRCH_CACHE_ENABLE_SEARCH_BITMAPS_CACHING) */


    /* Check the parameters */
    if ( pvSrchCache == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvSrchCache' parameter passed to 'iSrchCacheGetSearchBitmap'."); 
        return (SRCH_CacheInvalidCache);
    }

    if ( psiSrchIndex == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'psiSrchIndex' parameter passed to 'iSrchCacheGetSearchBitmap'."); 
        return (SRCH_InvalidIndex);
    }

    if ( bUtlStringsIsWideStringNULL(pwcBitmapName) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pwcBitmapName' parameter passed to 'iSrchCacheGetSearchBitmap'."); 
        return (SRCH_CacheInvalidBitmapName);
    }

    if ( ppsbSrchBitmap == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppsbSrchBitmap' parameter passed to 'iSrchCacheGetSearchBitmap'."); 
        return (SRCH_ReturnParameterError);
    }


    /* Check the cache mode, we cant read a cache file if the cache is off */
    if ( pscSrchCache->uiSearchCacheMode == SRCH_CACHE_MODE_OFF ) {
        return (SRCH_CacheInvalidMode);
    }

    /* Check the cache type, we can't handle an unknown cache type */
    if ( pscSrchCache->uiSearchCacheType == SRCH_CACHE_TYPE_UNKNOWN ) {
        return (SRCH_CacheInvalidType);
    }



    /* Create an SHA1 hex digest for this search bitmap */
    if ( (iError = iSrchCacheGetSearchBitmapSHA1HexDigest(psiSrchIndex, pwcBitmapName, tBitmapLastUpdate, pucSHA1HexDigest)) != SRCH_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to generate SHA1 signature for search bitmap cache file path, srch error: %d.", iError);
        return (iError);
    }


    /* File type */
    if ( pscSrchCache->uiSearchCacheType == SRCH_CACHE_TYPE_FILE ) {

        off_t    zSearchCacheFileDataLength = 0;

        /* Get the search bitmap cache file path for this search */
        if ( (iError = iSrchCacheGetCacheFilePath(pscSrchCache, psiSrchIndex, pucSHA1HexDigest, pucSearchCacheFilePath, UTL_FILE_PATH_MAX + 1)) != SRCH_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the search bitmap cache file path, srch error: %d.", iError);
            iError = SRCH_CacheGetFailed;
            goto bailFromiSrchCacheGetSearchBitmap;
        }
    
        /* Check that there is a search bitmap cache file */
        if ( bUtlFileIsFile(pucSearchCacheFilePath) == false ) {
            iError = SRCH_CacheGetFailed;
            goto bailFromiSrchCacheGetSearchBitmap;
        }
        
        /* Check that the search bitmap cache file can be read */
        if ( bUtlFilePathRead(pucSearchCacheFilePath) == false ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to access the search bitmap cache file: '%s'.", pucSearchCacheFilePath);
            iError = SRCH_CacheGetFailed;
            goto bailFromiSrchCacheGetSearchBitmap;
        }
    
        /* Open the search bitmap cache file */
        if ( (pfSearchCacheFile = s_fopen(pucSearchCacheFilePath, "r")) == NULL ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to open the search bitmap cache file: '%s'.", pucSearchCacheFilePath);
            iError = SRCH_CacheGetFailed;
            goto bailFromiSrchCacheGetSearchBitmap;
        }
    
        /* Get the search bitmap cache file length */
        if ( (iError = iUtlFileGetFileLength(pfSearchCacheFile, &zSearchCacheFileDataLength)) != UTL_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the length of the search bitmap cache file: '%s', utl error: %d.", pucSearchCacheFilePath, iError);
            iError = SRCH_CacheGetFailed;
            goto bailFromiSrchCacheGetSearchBitmap;
        }
    
        /* Check the length of the cache file */
        if ( zSearchCacheFileDataLength == 0 ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to read from the search bitmap cache file: '%s', empty file.", pucSearchCacheFilePath);
            iError = SRCH_CacheGetFailed;
            goto bailFromiSrchCacheGetSearchBitmap;
        }
    
        /* Place a shared lock on the search bitmap cache file */
        if ( (iError = iSrchCacheLockCacheFile(pfSearchCacheFile, SRCH_CACHE_LOCK_SHARED)) != SRCH_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to place a shared lock on the search bitmap cache file: '%s', srch error: %d.", pucSearchCacheFilePath, iError);
            iError = SRCH_CacheGetFailed;
            goto bailFromiSrchCacheGetSearchBitmap;
        }
        
        /* Map in the entire file */
        if ( (iError = iUtlFileMemoryMap(fileno(pfSearchCacheFile), 0, zSearchCacheFileDataLength, PROT_READ, (void **)&pucBitmap)) != UTL_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to map in the search bitmap cache file: '%s', utl error: %d.", pucSearchCacheFilePath, iError);
            iError = SRCH_CacheGetFailed;
            goto bailFromiSrchCacheGetSearchBitmap;
        }


        /* Set the bitmap array length */
        uiBitmapLength = zSearchCacheFileDataLength * (sizeof(unsigned char) * 8);

        
        /* This is a mapped allocation */
        bMappedAllocationFlag = true;

/*         iUtlLogInfo(UTL_LOG_CONTEXT, "Read search bitmap cache from file - pwcBitmapName: '%ls' => '%s' (%ld).", pwcBitmapName, pucSearchCacheFilePath, (unsigned long)zSearchCacheFileDataLength); */
/*         iUtlLogDebug(UTL_LOG_CONTEXT, "Read search bitmap cache from file: '%s'.", pucSearchCacheFilePath); */
    }


    /* Allocate a new search bitmap structure */
    if ( (iError = iSrchBitmapCreate(pucBitmap, uiBitmapLength, bMappedAllocationFlag, &psbSrchBitmap)) != SRCH_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to create a new search bitmap, srch error: %d.", iError);
        return (iError);
    }



    /* Bail label */
    bailFromiSrchCacheGetSearchBitmap:


    /* Handle the error */
    if ( iError == SRCH_NoError ) {
        
        /* File type */
        if ( pscSrchCache->uiSearchCacheType == SRCH_CACHE_TYPE_FILE ) {

#if defined(SRCH_CACHE_ENABLE_UTIME_CACHE_FILE_UPDATE)
            /* Set the access and modification times on the search bitmap cache file to now */
            if  ( pscSrchCache->uiSearchCacheMode == SRCH_CACHE_MODE_READ_WRITE ) {
                
                int     iLocalError = UTL_NoError;
                
                if ( (iLocalError = iUtlFileSetPathUTime(pucSearchCacheFilePath)) != UTL_NoError ) {
                    iUtlLogError(UTL_LOG_CONTEXT, "Failed to set the access and modification times on the search bitmap cache file: '%s', utl error: %d.", 
                            pucSearchCacheFilePath, iLocalError);
                }
            }
#endif    /* defined(SRCH_CACHE_ENABLE_UTIME_CACHE_FILE_UPDATE) */

            /* Release the lock we are currently holding */
            iSrchCacheUnlockCacheFile(pfSearchCacheFile);

            /* Close the search bitmap cache file */
            s_fclose(pfSearchCacheFile);

/*             iUtlLogDebug(UTL_LOG_CONTEXT, "Search results were returned from the search bitmap cache file: '%s'.", pucSearchCacheFilePath); */
        }

        /* Set the return pointer */
        *ppsbSrchBitmap = psbSrchBitmap;

#if defined(SRCH_CACHE_ENABLE_SEARCH_BITMAP_CACHE_LOGGING) || defined(SRCH_CACHE_ENABLE_ALL_CACHE_LOGGING)
        iUtlLogInfo(UTL_LOG_CONTEXT, "Search bitmap was served from cache.");
#endif    /* defined(SRCH_CACHE_ENABLE_SEARCH_BITMAP_CACHE_LOGGING) || defined(SRCH_CACHE_ENABLE_ALL_CACHE_LOGGING) */

    }
    else {
        
        /* Free any allocated resources */
        iSrchBitmapFree(psbSrchBitmap);
        psbSrchBitmap = NULL;
    
        /* File type */
        if ( pscSrchCache->uiSearchCacheType == SRCH_CACHE_TYPE_FILE ) {

            /* Remove the file if it is open */
            if ( (pfSearchCacheFile != NULL) && (pscSrchCache->uiSearchCacheMode == SRCH_CACHE_MODE_READ_WRITE) ) {

                int     iLocalError = SRCH_NoError;

                /* Place an exclusive lock on the search bitmap cache file, just fall through if we failed */
                if ( (iLocalError = iSrchCacheLockCacheFile(pfSearchCacheFile, SRCH_CACHE_LOCK_EXCLUSIVE)) != SRCH_NoError ) {
                    iUtlLogWarn(UTL_LOG_CONTEXT, "Failed to place an exclusive lock on the search bitmap cache file: '%s', srch error: %d.", 
                            pucSearchCacheFilePath, iLocalError);
                }

                /* Remove the file */
/*                 s_remove(pucSearchCacheFilePath); */

                /* Close the file  */
                s_fclose(pfSearchCacheFile);
            }
        }
    }


    return (iError);

}


/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchCacheGetSearchShortResultsSHA1HexDigest()

    Purpose:    This function calculates the SHA1 hex digest for a search short result
                structure array.

    Parameters: psiSrchIndex                search index structure
                pwcSearchText               search text (optional)
                pwcPositiveFeedbackText     positive feedback text (optional)
                pwcNegativeFeedbackText     positive feedback text (optional)
                pucSHA1HexDigest            return pointer for the SHA1 hex digest

    Globals:    none

    Returns:    SRCH error code

*/
static int iSrchCacheGetSearchShortResultsSHA1HexDigest
(
    struct srchIndex *psiSrchIndex,
    wchar_t *pwcSearchText,
    wchar_t *pwcPositiveFeedbackText,
    wchar_t *pwcNegativeFeedbackText,
    unsigned char *pucSHA1HexDigest
)
{

    int             iError = SRCH_NoError;
    void            *pvUtlSHA1 = NULL;
    unsigned char   pucBuffer[UTL_FILE_PATH_MAX + 1] = {'\0'};
    unsigned char   pucSHA1Digest[UTL_SHA1_DIGEST_LENGTH + 1] = {'\0'};

    time_t          tLastUpdateTime = (time_t)0;
    

    ASSERT(psiSrchIndex != NULL);
    ASSERT((bUtlStringsIsWideStringNULL(pwcSearchText) == false) || (bUtlStringsIsWideStringNULL(pwcSearchText) == true));
    ASSERT((bUtlStringsIsWideStringNULL(pwcPositiveFeedbackText) == false) || (bUtlStringsIsWideStringNULL(pwcPositiveFeedbackText) == true));
    ASSERT((bUtlStringsIsWideStringNULL(pwcNegativeFeedbackText) == false) || (bUtlStringsIsWideStringNULL(pwcNegativeFeedbackText) == true));
    ASSERT(pucSHA1HexDigest != NULL);


    /* Create an SHA1 hex digest for this search short result */
    

    /* Init the SHA1 structure */
    if ( (iError = iUtlSHA1Create(&pvUtlSHA1)) != UTL_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to create a SHA1, utl error: %d.", iError);
        iError = SRCH_CacheSHA1Failed;
        goto bailFromiSrchCacheGetSearchShortResultsSHA1HexDigest;
    }


    /* Add the index name */
    if ( (iError = iUtlSHA1Update(pvUtlSHA1, psiSrchIndex->pucIndexName, s_strlen(psiSrchIndex->pucIndexName))) != UTL_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to update a SHA1, utl error: %d.", iError);
        iError = SRCH_CacheSHA1Failed;
        goto bailFromiSrchCacheGetSearchShortResultsSHA1HexDigest;
    }


    /* Add the index last update time, use the one we recovered from the shared memory key if it is set, 
    ** they will be different if a new version of the index has been loaded but not yet pivoted, which
    ** which means that we are still searching the old version of the index and not the new one whose
    ** last update time is in psiSrchIndex->tLastUpdateTime
    */
    snprintf(pucBuffer, UTL_FILE_PATH_MAX + 1, "%lu", (unsigned long)psiSrchIndex->tLastUpdateTime);
    if ( (iError = iUtlSHA1Update(pvUtlSHA1, pucBuffer, s_strlen(pucBuffer))) != UTL_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to update a SHA1, utl error: %d.", iError);
        iError = SRCH_CacheSHA1Failed;
        goto bailFromiSrchCacheGetSearchShortResultsSHA1HexDigest;
    }
    

    /* Add the search text */
    if ( bUtlStringsIsWideStringNULL(pwcSearchText) == false ) {
        if ( (iError = iUtlSHA1Update(pvUtlSHA1, (unsigned char *)pwcSearchText, s_wcslen(pwcSearchText) * sizeof(wchar_t))) != UTL_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to update a SHA1, utl error: %d.", iError);
            iError = SRCH_CacheSHA1Failed;
            goto bailFromiSrchCacheGetSearchShortResultsSHA1HexDigest;
        }
    }    


    /* Add the positive feedback text */
    if ( bUtlStringsIsWideStringNULL(pwcPositiveFeedbackText) == false ) {
        if ( (iError = iUtlSHA1Update(pvUtlSHA1, (unsigned char *)pwcPositiveFeedbackText, s_wcslen(pwcPositiveFeedbackText) * sizeof(wchar_t))) != UTL_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to update a SHA1, utl error: %d.", iError);
            iError = SRCH_CacheSHA1Failed;
            goto bailFromiSrchCacheGetSearchShortResultsSHA1HexDigest;
        }
    }    


    /* Add the negative feedback text */
    if ( bUtlStringsIsWideStringNULL(pwcNegativeFeedbackText) == false ) {
        if ( (iError = iUtlSHA1Update(pvUtlSHA1, (unsigned char *)pwcNegativeFeedbackText, s_wcslen(pwcNegativeFeedbackText) * sizeof(wchar_t))) != UTL_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to update a SHA1, utl error: %d.", iError);
            iError = SRCH_CacheSHA1Failed;
            goto bailFromiSrchCacheGetSearchShortResultsSHA1HexDigest;
        }
    }    


    /* Get the digest of the SHA1 key */
    if ( (iError = iUtlSHA1Digest(pvUtlSHA1, pucSHA1Digest, pucSHA1HexDigest)) != UTL_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to get a SHA1 digest, utl error: %d.", iError);
        iError = SRCH_CacheSHA1Failed;
        goto bailFromiSrchCacheGetSearchShortResultsSHA1HexDigest;
    }


    /* Null terminate the digest */
    pucSHA1HexDigest[UTL_SHA1_HEX_DIGEST_LENGTH] = '\0';



    /* Bail label */
    bailFromiSrchCacheGetSearchShortResultsSHA1HexDigest:
    
    /* Handle the error */
    if ( iError != SRCH_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to create a SHA1 digest for a short results, utl error: %d.", iError);
    }
    
    /* Free the SHA1 structure */
    if ( pvUtlSHA1 != NULL ) {
        iUtlSHA1Free(pvUtlSHA1);
    }


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchCacheGetSearchPostingsListSHA1HexDigest()

    Purpose:    This function calculates the SHA1 hex digest for a search postings list
                structure.

    Parameters: psiSrchIndex            search index structure
                uiLanguageID            language ID
                psptSrchParserTerm      parser term structure
                pucSHA1HexDigest        return pointer for the SHA1 hex digest

    Globals:    none

    Returns:    SRCH error code

*/
static int iSrchCacheGetSearchPostingsListSHA1HexDigest
(
    struct srchIndex *psiSrchIndex,
    unsigned int uiLanguageID, 
    struct srchParserTerm *psptSrchParserTerm,
    unsigned char *pucSHA1HexDigest
)
{

    int             iError = SRCH_NoError;
    void            *pvUtlSHA1 = NULL;
    unsigned char   pucBuffer[UTL_FILE_PATH_MAX + 1] = {'\0'};
    unsigned char   pucSHA1Digest[UTL_SHA1_DIGEST_LENGTH + 1] = {'\0'};
    

    ASSERT(psiSrchIndex != NULL);
    ASSERT(uiLanguageID >= 0);
    ASSERT(psptSrchParserTerm != NULL);
    ASSERT(pucSHA1HexDigest != NULL);


    /* Create an SHA1 hex digest for this search postings list */
    

    /* Init the SHA1 structure */
    if ( (iError = iUtlSHA1Create(&pvUtlSHA1)) != UTL_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to create a SHA1, utl error: %d.", iError);
        iError = SRCH_CacheSHA1Failed;
        goto bailFromiSrchCacheGetSearchPostingsListSHA1HexDigest;
    }
    
    
    /* Add the index name */
    if ( (iError = iUtlSHA1Update(pvUtlSHA1, psiSrchIndex->pucIndexName, s_strlen(psiSrchIndex->pucIndexName))) != UTL_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to update a SHA1, utl error: %d.", iError);
        iError = SRCH_CacheSHA1Failed;
        goto bailFromiSrchCacheGetSearchPostingsListSHA1HexDigest;
    }


    /* Add the index last update time */
    snprintf(pucBuffer, UTL_FILE_PATH_MAX + 1, "%lu", (unsigned long)psiSrchIndex->tLastUpdateTime);
    if ( (iError = iUtlSHA1Update(pvUtlSHA1, pucBuffer, s_strlen(pucBuffer))) != UTL_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to update a SHA1, utl error: %d.", iError);
        iError = SRCH_CacheSHA1Failed;
        goto bailFromiSrchCacheGetSearchPostingsListSHA1HexDigest;
    }
    

    /* Add the language ID */
    snprintf(pucBuffer, UTL_FILE_PATH_MAX + 1, "%u", uiLanguageID);
    if ( (iError = iUtlSHA1Update(pvUtlSHA1, pucBuffer, s_strlen(pucBuffer))) != UTL_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to update a SHA1, utl error: %d.", iError);
        iError = SRCH_CacheSHA1Failed;
        goto bailFromiSrchCacheGetSearchPostingsListSHA1HexDigest;
    }


    /* Add the term */
    if ( (iError = iUtlSHA1Update(pvUtlSHA1, (unsigned char *)psptSrchParserTerm->pwcTerm, s_wcslen(psptSrchParserTerm->pwcTerm) * sizeof(wchar_t))) != UTL_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to update a SHA1, utl error: %d.", iError);
        iError = SRCH_CacheSHA1Failed;
        goto bailFromiSrchCacheGetSearchPostingsListSHA1HexDigest;
    }


    /* Add the field name */
    if ( bUtlStringsIsWideStringNULL(psptSrchParserTerm->pwcFieldName) == false ) {
        if ( (iError = iUtlSHA1Update(pvUtlSHA1, (unsigned char *)psptSrchParserTerm->pwcFieldName, s_wcslen(psptSrchParserTerm->pwcFieldName) * sizeof(wchar_t))) != UTL_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to update a SHA1, utl error: %d.", iError);
            iError = SRCH_CacheSHA1Failed;
            goto bailFromiSrchCacheGetSearchPostingsListSHA1HexDigest;
        }
    }    


    /* Add the function ID */
    snprintf(pucBuffer, UTL_FILE_PATH_MAX + 1, "%u", psptSrchParserTerm->uiFunctionID);
    if ( (iError = iUtlSHA1Update(pvUtlSHA1, pucBuffer, s_strlen(pucBuffer))) != UTL_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to update a SHA1, utl error: %d.", iError);
        iError = SRCH_CacheSHA1Failed;
        goto bailFromiSrchCacheGetSearchPostingsListSHA1HexDigest;
    }


    /* Add the range ID */
    snprintf(pucBuffer, UTL_FILE_PATH_MAX + 1, "%u", psptSrchParserTerm->uiRangeID);
    if ( (iError = iUtlSHA1Update(pvUtlSHA1, pucBuffer, s_strlen(pucBuffer))) != UTL_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to update a SHA1, utl error: %d.", iError);
        iError = SRCH_CacheSHA1Failed;
        goto bailFromiSrchCacheGetSearchPostingsListSHA1HexDigest;
    }


    /* Add the wildcard search flag */
    snprintf(pucBuffer, UTL_FILE_PATH_MAX + 1, "%u", psptSrchParserTerm->bWildCardSearch);
    if ( (iError = iUtlSHA1Update(pvUtlSHA1, pucBuffer, s_strlen(pucBuffer))) != UTL_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to update a SHA1, utl error: %d.", iError);
        iError = SRCH_CacheSHA1Failed;
        goto bailFromiSrchCacheGetSearchPostingsListSHA1HexDigest;
    }


    /* Add the term weight */
    snprintf(pucBuffer, UTL_FILE_PATH_MAX + 1, "%8.2f", psptSrchParserTerm->fTermWeight);
    if ( (iError = iUtlSHA1Update(pvUtlSHA1, pucBuffer, s_strlen(pucBuffer))) != UTL_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to update a SHA1, utl error: %d.", iError);
        iError = SRCH_CacheSHA1Failed;
        goto bailFromiSrchCacheGetSearchPostingsListSHA1HexDigest;
    }


    /* Add the required flag */
    snprintf(pucBuffer, UTL_FILE_PATH_MAX + 1, "%u", psptSrchParserTerm->bRequired);
    if ( (iError = iUtlSHA1Update(pvUtlSHA1, pucBuffer, s_strlen(pucBuffer))) != UTL_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to update a SHA1, utl error: %d.", iError);
        iError = SRCH_CacheSHA1Failed;
        goto bailFromiSrchCacheGetSearchPostingsListSHA1HexDigest;
    }


    /* Get the digest of the SHA1 key */
    if ( (iError = iUtlSHA1Digest(pvUtlSHA1, pucSHA1Digest, pucSHA1HexDigest)) != UTL_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to get a SHA1 digest, utl error: %d.", iError);
        iError = SRCH_CacheSHA1Failed;
        goto bailFromiSrchCacheGetSearchPostingsListSHA1HexDigest;
    }


    /* Null terminate the digest */
    pucSHA1HexDigest[UTL_SHA1_HEX_DIGEST_LENGTH] = '\0';



    /* Bail label */
    bailFromiSrchCacheGetSearchPostingsListSHA1HexDigest:

    
    /* Handle the error */
    if ( iError != SRCH_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to create a SHA1 digest for a postings list, utl error: %d.", iError);
    }
    
    /* Free the SHA1 structure */
    if ( pvUtlSHA1 != NULL ) {
        iUtlSHA1Free(pvUtlSHA1);
    }


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchCacheGetSearchWeightSHA1HexDigest()

    Purpose:    This function calculates the SHA1 hex digest for a search
                weight structure.

    Parameters: psiSrchIndex        search index structure
                pwcWeightName       weight name
                pucSHA1HexDigest    return pointer for the SHA1 hex digest

    Globals:    none

    Returns:    SRCH error code

*/
static int iSrchCacheGetSearchWeightSHA1HexDigest
(
    struct srchIndex *psiSrchIndex,
    wchar_t *pwcWeightName,
    unsigned char *pucSHA1HexDigest
)
{

    int             iError = SRCH_NoError;
    unsigned char   pucBuffer[UTL_FILE_PATH_MAX + 1] = {'\0'};
    void            *pvUtlSHA1 = NULL;
    unsigned char   pucSHA1Digest[UTL_SHA1_DIGEST_LENGTH + 1] = {'\0'};


    ASSERT(psiSrchIndex != NULL);
    ASSERT(bUtlStringsIsWideStringNULL(pwcWeightName) == false);
    ASSERT(pucSHA1HexDigest != NULL);


    /* Create an SHA1 hex digest for this search weight */
    

    /* Init the SHA1 structure */
    if ( (iError = iUtlSHA1Create(&pvUtlSHA1)) != UTL_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to create a SHA1, utl error: %d.", iError);
        iError = SRCH_CacheSHA1Failed;
        goto bailFromiSrchCacheGetSearchWeightSHA1HexDigest;
    }
    
    
    /* Add the index name */
    if ( (iError = iUtlSHA1Update(pvUtlSHA1, psiSrchIndex->pucIndexName, s_strlen(psiSrchIndex->pucIndexName))) != UTL_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to update a SHA1, utl error: %d.", iError);
        iError = SRCH_CacheSHA1Failed;
        goto bailFromiSrchCacheGetSearchWeightSHA1HexDigest;
    }

    
    /* Add the index last update time */
    snprintf(pucBuffer, UTL_FILE_PATH_MAX + 1, "%lu", (unsigned long)psiSrchIndex->tLastUpdateTime);
    if ( (iError = iUtlSHA1Update(pvUtlSHA1, pucBuffer, s_strlen(pucBuffer))) != UTL_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to update a SHA1, utl error: %d.", iError);
        iError = SRCH_CacheSHA1Failed;
        goto bailFromiSrchCacheGetSearchWeightSHA1HexDigest;
    }
    

    /* Add the weight name */
    if ( (iError = iUtlSHA1Update(pvUtlSHA1, (unsigned char *)pwcWeightName, s_wcslen(pwcWeightName) * sizeof(wchar_t))) != UTL_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to update a SHA1, utl error: %d.", iError);
        iError = SRCH_CacheSHA1Failed;
        goto bailFromiSrchCacheGetSearchWeightSHA1HexDigest;
    }

    
    /* Get the digest of the SHA1 key */
    if ( (iError = iUtlSHA1Digest(pvUtlSHA1, pucSHA1Digest, pucSHA1HexDigest)) != UTL_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to get a SHA1 digest, utl error: %d.", iError);
        iError = SRCH_CacheSHA1Failed;
        goto bailFromiSrchCacheGetSearchWeightSHA1HexDigest;
    }


    /* Null terminate the digest */
    pucSHA1HexDigest[UTL_SHA1_HEX_DIGEST_LENGTH] = '\0';



    /* Bail label */
    bailFromiSrchCacheGetSearchWeightSHA1HexDigest:
    

    /* Handle the error */
    if ( iError != SRCH_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to create a SHA1 digest for a weights, utl error: %d.", iError);
    }
    
    /* Free the SHA1 structure */
    if ( pvUtlSHA1 != NULL ) {
        iUtlSHA1Free(pvUtlSHA1);
    }


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchCacheGetSearchBitmapSHA1HexDigest()

    Purpose:    This function calculates the SHA1 hex digest for a search bitmap
                structure.

    Parameters: psiSrchIndex        search index structure
                pwcBitmapName       bitmap name
                tBitmapLastUpdate   bitmap last update
                pucSHA1HexDigest    return pointer for the SHA1 hex digest

    Globals:    none

    Returns:    SRCH error code

*/
static int iSrchCacheGetSearchBitmapSHA1HexDigest
(
    struct srchIndex *psiSrchIndex,
    wchar_t *pwcBitmapName,
    time_t tBitmapLastUpdate,
    unsigned char *pucSHA1HexDigest
)
{

    int             iError = SRCH_NoError;
    unsigned char   pucBuffer[UTL_FILE_PATH_MAX + 1] = {'\0'};
    void            *pvUtlSHA1 = NULL;
    unsigned char   pucSHA1Digest[UTL_SHA1_DIGEST_LENGTH + 1] = {'\0'};


    ASSERT(psiSrchIndex != NULL);
    ASSERT(bUtlStringsIsWideStringNULL(pwcBitmapName) == false);
    ASSERT(tBitmapLastUpdate >= 0);
    ASSERT(pucSHA1HexDigest != NULL);


    /* Create an SHA1 hex digest for this search bitmap */
    

    /* Init the SHA1 structure */
    if ( (iError = iUtlSHA1Create(&pvUtlSHA1)) != UTL_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to create a SHA1, utl error: %d.", iError);
        iError = SRCH_CacheSHA1Failed;
        goto bailFromiSrchCacheGetSearchBitmapSHA1HexDigest;
    }
    
    
    /* Add the index name */
    if ( (iError = iUtlSHA1Update(pvUtlSHA1, psiSrchIndex->pucIndexName, s_strlen(psiSrchIndex->pucIndexName))) != UTL_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to update a SHA1, utl error: %d.", iError);
        iError = SRCH_CacheSHA1Failed;
        goto bailFromiSrchCacheGetSearchBitmapSHA1HexDigest;
    }

    
    /* Add the index last update time */
    snprintf(pucBuffer, UTL_FILE_PATH_MAX + 1, "%lu", (unsigned long)psiSrchIndex->tLastUpdateTime);
    if ( (iError = iUtlSHA1Update(pvUtlSHA1, pucBuffer, s_strlen(pucBuffer))) != UTL_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to update a SHA1, utl error: %d.", iError);
        iError = SRCH_CacheSHA1Failed;
        goto bailFromiSrchCacheGetSearchBitmapSHA1HexDigest;
    }
    

    /* Add the bitmap name */
    if ( (iError = iUtlSHA1Update(pvUtlSHA1, (unsigned char *)pwcBitmapName, s_wcslen(pwcBitmapName) * sizeof(wchar_t))) != UTL_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to update a SHA1, utl error: %d.", iError);
        iError = SRCH_CacheSHA1Failed;
        goto bailFromiSrchCacheGetSearchBitmapSHA1HexDigest;
    }

    
    /* Add the bitmap last update time */
    snprintf(pucBuffer, UTL_FILE_PATH_MAX + 1, "%lu", (unsigned long)tBitmapLastUpdate);
    if ( (iError = iUtlSHA1Update(pvUtlSHA1, pucBuffer, s_strlen(pucBuffer))) != UTL_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to update a SHA1, utl error: %d.", iError);
        iError = SRCH_CacheSHA1Failed;
        goto bailFromiSrchCacheGetSearchBitmapSHA1HexDigest;
    }


    /* Get the digest of the SHA1 key */
    if ( (iError = iUtlSHA1Digest(pvUtlSHA1, pucSHA1Digest, pucSHA1HexDigest)) != UTL_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to get a SHA1 digest, utl error: %d.", iError);
        iError = SRCH_CacheSHA1Failed;
        goto bailFromiSrchCacheGetSearchBitmapSHA1HexDigest;
    }


    /* Null terminate the digest */
    pucSHA1HexDigest[UTL_SHA1_HEX_DIGEST_LENGTH] = '\0';



    /* Bail label */
    bailFromiSrchCacheGetSearchBitmapSHA1HexDigest:
    

    /* Handle the error */
    if ( iError != SRCH_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to create a SHA1 digest for a bitmap, utl error: %d.", iError);
    }
    
    /* Free the SHA1 structure */
    if ( pvUtlSHA1 != NULL ) {
        iUtlSHA1Free(pvUtlSHA1);
    }


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchCacheGetCacheFilePath()

    Purpose:    This function get the search cache file path.

    Parameters: pscSrchCache                    search cache structure
                psiSrchIndex                    search index structure
                pucSHA1HexDigest                SHA1 hex digest
                pucSearchCacheFilePath          return pointer for the search file path
                uiSearchCacheFilePathLength     length of the search file path return pointer

    Globals:    none

    Returns:    SRCH error code

*/
static int iSrchCacheGetCacheFilePath
(
    struct srchCache *pscSrchCache,
    struct srchIndex *psiSrchIndex,
    unsigned char *pucSHA1HexDigest,
    unsigned char *pucSearchCacheFilePath,
    unsigned int uiSearchCacheFilePathLength
)
{

    int             iError = UTL_NoError;
    unsigned char   pucSearchCacheFileName[UTL_FILE_PATH_MAX + 1] = {'\0'};
    
    
    ASSERT(pscSrchCache != NULL);
    ASSERT(psiSrchIndex != NULL);
    ASSERT(bUtlStringsIsStringNULL(pucSHA1HexDigest) == false);
    ASSERT(pucSearchCacheFilePath != NULL);
    ASSERT(uiSearchCacheFilePathLength > 0);


    /* Create the search cache file name */
    snprintf(pucSearchCacheFileName, UTL_FILE_PATH_MAX + 1, "%s%s", pucSHA1HexDigest, SRCH_CACHE_FILENAME_EXT);
    

    /* Create the search cache file path, different methods if we need to create a search cache subdirectory */
    if ( bUtlStringsIsStringNULL(pscSrchCache->pucSearchCacheSubDirectoryMask) == true ) {
        
        /* Create a search cache file path using the seach cache directory as a base */
        if ( (iError = iUtlFileMergePaths(pscSrchCache->pucSearchCacheDirectoryPath, pucSearchCacheFileName, pucSearchCacheFilePath, uiSearchCacheFilePathLength)) != UTL_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to create the search cache file path, search cache file name: '%s', search cache directory path: '%s', utl error: %d.", 
                    pucSearchCacheFileName, pscSrchCache->pucSearchCacheDirectoryPath, iError);
            return (SRCH_CacheGetFilePathFailed);
        }
    }
    else {
    
        unsigned char   pucSearchCacheSubdirectoryPath[UTL_FILE_PATH_MAX + 1] = {'\0'};
        unsigned char   *pucSearchCacheSubdirectoryPathPtr = NULL;
        unsigned char   pucSearchCacheDirectoryPath[UTL_FILE_PATH_MAX + 1] = {'\0'};
        unsigned char   pucLastUpdateTime[UTL_FILE_PATH_MAX + 1] = {'\0'};
        unsigned char   pucBuffer[UTL_FILE_PATH_MAX + 1] = {'\0'};


        /* Copy the search cache subdirectory mask to the search cache subdirectory path */
        s_strnncpy(pucSearchCacheSubdirectoryPath, pscSrchCache->pucSearchCacheSubDirectoryMask, UTL_FILE_PATH_MAX + 1);
    
        /* Replace digits with offsets into the SHA1 hex digest */
        for ( pucSearchCacheSubdirectoryPathPtr = pucSearchCacheSubdirectoryPath; *pucSearchCacheSubdirectoryPathPtr != '\0'; pucSearchCacheSubdirectoryPathPtr++ ) {
            if ( (*pucSearchCacheSubdirectoryPathPtr >= '0') && (*pucSearchCacheSubdirectoryPathPtr <= '9') ) {
                *pucSearchCacheSubdirectoryPathPtr = pucSHA1HexDigest[*pucSearchCacheSubdirectoryPathPtr - '0'];
            }
        }

        
        /* Replace the index name symbol with the index name and copy the buffer back */
        iUtlStringsReplaceStringInString(pucSearchCacheSubdirectoryPath, SRCH_CACHE_INDEX_NAME_SYMBOL, psiSrchIndex->pucIndexName, pucBuffer, UTL_FILE_PATH_MAX + 1);
        s_strnncpy(pucSearchCacheSubdirectoryPath, pucBuffer, UTL_FILE_PATH_MAX + 1);
    
        /* Replace the last update time symbol with the last update time and copy the buffer back */
        sprintf(pucLastUpdateTime, "%lu", (unsigned long)psiSrchIndex->tLastUpdateTime);
        iUtlStringsReplaceStringInString(pucSearchCacheSubdirectoryPath, SRCH_CACHE_LAST_UPDATE_TIME_SYMBOL, pucLastUpdateTime, pucBuffer, UTL_FILE_PATH_MAX + 1);
        s_strnncpy(pucSearchCacheSubdirectoryPath, pucBuffer, UTL_FILE_PATH_MAX + 1);
        
        /* Create the search cache directory path */
        if ( (iError = iUtlFileMergePaths(pscSrchCache->pucSearchCacheDirectoryPath, pucSearchCacheSubdirectoryPath, pucSearchCacheDirectoryPath, UTL_FILE_PATH_MAX + 1)) != UTL_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to create the search cache directory path, search cache subdirectory path: '%s', search cache directory path: '%s', utl error: %d.", 
                    pucSearchCacheSubdirectoryPath, pscSrchCache->pucSearchCacheDirectoryPath, iError);
            return (SRCH_CacheGetFilePathFailed);
        }


        /* Create the search cache directory path if we are in read/write mode */
        if ( pscSrchCache->uiSearchCacheMode == SRCH_CACHE_MODE_READ_WRITE ) {

            /* Create the search cache directory path */
            if ( (iError = iUtlFileCreateDirectoryPath(pucSearchCacheDirectoryPath, pucSearchCacheDirectoryPath + s_strlen(pscSrchCache->pucSearchCacheDirectoryPath), 
                     S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH)) != UTL_NoError ) {
                iUtlLogError(UTL_LOG_CONTEXT, "Failed to create the search cache directory: '%s', utl error: %d.", pucSearchCacheDirectoryPath, iError);
                return (SRCH_CacheCreateDirectoryFailed);
            }
        }


        /* Create a search cache file path using the seach cache directory as a base */
        if ( (iError = iUtlFileMergePaths(pucSearchCacheDirectoryPath, pucSearchCacheFileName, pucSearchCacheFilePath, uiSearchCacheFilePathLength)) != UTL_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to create the search cache subdirectory path, search cache file name: '%s', search cache directory path: '%s', utl error: %d.", 
                    pucSearchCacheFileName, pucSearchCacheDirectoryPath, iError);
            return (SRCH_CacheGetFilePathFailed);
        }
    }


    return (SRCH_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   lSrchCacheLock()

    Purpose:    This function locks the passed file.

    Parameters: pfSearchCacheFile   search cache file pointer
                uiLockType          lock type

    Globals:    none

    Returns:    SRCH error code

*/
static int iSrchCacheLockCacheFile
(
    FILE *pfSearchCacheFile,
    unsigned int uiLockType
)
{

    struct flock    flFLock;
    unsigned int    uiLockSleep = 0;
    unsigned int    uiLockTimeOut = 0;
    unsigned int    uiTimeOut = 0;
    int             iStatus = 0;


    ASSERT(pfSearchCacheFile != NULL);
    ASSERT(SRCH_CACHE_LOCK_VALID(uiLockType) == true);


    /* Set the lock structure, lock sleep and timeout values according to the lock type */
    switch ( uiLockType ) {

        /* Set up for a shared lock */
        case SRCH_CACHE_LOCK_SHARED:
            flFLock.l_type = F_RDLCK;
            flFLock.l_whence = SEEK_SET;
            flFLock.l_start = 0;
            flFLock.l_len = 0;
            flFLock.l_pid = 0;
            uiLockSleep = SRCH_CACHE_SHARED_LOCK_SLEEP;
            uiLockTimeOut = SRCH_CACHE_SHARED_LOCK_TIMEOUT;
            break;
        
        /* Set up for an exclusive lock */
        case SRCH_CACHE_LOCK_EXCLUSIVE:
            flFLock.l_type = F_WRLCK;
            flFLock.l_whence = SEEK_SET;
            flFLock.l_start = 0;
            flFLock.l_len = 0;
            flFLock.l_pid = 0;
            uiLockSleep = SRCH_CACHE_EXCLUSIVE_LOCK_SLEEP;
            uiLockTimeOut = SRCH_CACHE_EXCLUSIVE_LOCK_TIMEOUT;
            break;
        
        default:
            return (SRCH_CacheLockFailed);
    }
    
    
    /* Loop until we timeout */
    do {

        /* Try to place the lock */
        if ( (iStatus = fcntl(fileno(pfSearchCacheFile), F_SETLK, &flFLock)) != -1 ) {
            /* We have the lock, so we break out */
            break;
        }

        /* Go to sleep and increment the timeout */
        s_usleep(uiLockSleep);
        uiTimeOut += uiLockSleep;

    } while ( uiTimeOut < uiLockTimeOut );



    /* Check to see if we managed to place the lock */
    if ( iStatus < 0 ) {
        /* Failed to place a lock */
        return (SRCH_CacheLockFailed);
    }


    return (SRCH_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchCacheUnlockCacheFile()

    Purpose:    This function unlocks the passed file.

    Parameters: pfSearchCacheFile   search cache file pointer

    Globals:    none

    Returns:    SRCH error code

*/
static int iSrchCacheUnlockCacheFile
(
    FILE *pfSearchCacheFile
)
{

    struct flock    flFLock;


    ASSERT(pfSearchCacheFile != NULL);


    /* Set up the flock structure */
    flFLock.l_type = F_UNLCK;
    flFLock.l_whence = SEEK_SET;
    flFLock.l_start = 0;
    flFLock.l_len = 0;
    flFLock.l_pid = 0;

    /* Unlock the file */
    if ( fcntl(fileno(pfSearchCacheFile), F_SETLK, &flFLock) == -1 ) {
        return (SRCH_CacheUnlockFailed);
    }


    return (SRCH_NoError);

}


/*---------------------------------------------------------------------------*/


