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

    Created:    29 November 1995

    Purpose:    This module implements access to the search report
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
#define UTL_LOG_CONTEXT                     (unsigned char *)"com.fsconsult.mps.src.search.report"


/*---------------------------------------------------------------------------*/


/*
** Defines
*/

/* Search report type */
#define SRCH_REPORT_TYPE_UNKNOWN                    (0)
#define SRCH_REPORT_TYPE_FILE                       (1)
#define SRCH_REPORT_TYPE_MEMORY                     (2)

#define SRCH_REPORT_TYPE_VALID(n)                   (((n) >= SRCH_REPORT_TYPE_FILE) && \
                                                            ((n) <= SRCH_REPORT_TYPE_MEMORY))

/* Search report memory string */
#define SRCH_REPORT_MEMORY_STRING                   (unsigned char *)"memory"

/* Search report file protocol url */
#define SRCH_REPORT_FILE_PROTOCOL_URL               (unsigned char *)"file://"


/* Search report mode */
#define SRCH_REPORT_MODE_OFF                        (0)
#define SRCH_REPORT_MODE_READ_ONLY                  (1)
#define SRCH_REPORT_MODE_READ_WRITE                 (2)


/* Search report file name extension */
#define SRCH_REPORT_FILENAME_EXTENSION              (unsigned char *)".rpt"


/* Search report file name pre-amble to strip for linux */
#if defined(linux)
#define SRCH_REPORT_FILENAME_PREAMBLE_TO_STRIP      (unsigned char *)"file"
#endif


/*---------------------------------------------------------------------------*/


/*
** Structures
*/

/* Search report */
struct srchReport {
    unsigned int    uiSearchReportType;                                         /* Search report type */
    unsigned int    uiSearchReportMode;                                         /* Search report mode */

    unsigned char   pucSearchReportDirectoryPath[UTL_FILE_PATH_MAX + 1];        /* Search report directory path */
    unsigned char   pucSearchReportSubDirectoryMask[UTL_FILE_PATH_MAX + 1];     /* Search report subdirectory mask */

    unsigned char   pucSearchReportFilePath[UTL_FILE_PATH_MAX + 1];             /* Search report file path */
    FILE            *pfSearchReportFile;                                        /* Search report file handle */

    void            *pvUtlStringBuffer;                                         /* String buffer for in-memory report */
};


/*---------------------------------------------------------------------------*/


/* 
** Private function prototypes
*/

static int iSrchReportGetReportFilePath (struct srchReport *psrSrchReport,
        unsigned char *pucSearchReportFileName, unsigned char *pucSearchReportFilePath, 
        unsigned int uiSearchReportFilePathLength);


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchReportCreate()

    Purpose:    This function initializes the search report support information,
                basically it allows us to set some globals from the server
                configuration file up front so that we dont need to re-read 
                them all the time. Also we check that we can actually write in
                the directory specified in the server configuration file.
                The assumption is made, though, that we can always write in
                the /tmp directory (which is reasonable).

    Parameters: pssSrchSearch       search structure
                ppvSrchReport       return pointer for the search report structure

    Globals:    none

    Returns:    SRCH error code

*/
int iSrchReportCreate
(
    struct srchSearch *pssSrchSearch,
    void **ppvSrchReport
)
{

    int                 iError = SRCH_NoError;
    struct srchReport   *psrSrchReport = NULL;
    unsigned char       pucSearchReportLocation[UTL_CONFIG_VALUE_MAXIMUM_LENGTH + 1] = {'\0'};


    /* Check the parameters */
    if ( pssSrchSearch == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pssSrchSearch' parameter passed to 'iSrchReportCreate'."); 
        return (SRCH_ReportInvalidSearch);
    }

    if ( ppvSrchReport == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppvSrchReport' parameter passed to 'iSrchReportCreate'."); 
        return (SRCH_ReturnParameterError);
    }


    /* Allocate a search report structure */
    if ( (psrSrchReport = (struct srchReport *)s_malloc((size_t)(sizeof(struct srchReport)))) == NULL ) {
        return (SRCH_MemError);
    } 

    /* Default report type is unknow, search report mode is off, socket descriptor is -1 */
    psrSrchReport->uiSearchReportType = SRCH_REPORT_TYPE_UNKNOWN;
    psrSrchReport->uiSearchReportMode = SRCH_REPORT_MODE_OFF;

    /* Default to NULL */
    psrSrchReport->pvUtlStringBuffer = NULL;


    /* Get the search report location */
    iError = iUtlConfigGetValue(pssSrchSearch->pvUtlConfig, SRCH_SEARCH_CONFIG_SEARCH_REPORT_LOCATION, pucSearchReportLocation, UTL_CONFIG_VALUE_MAXIMUM_LENGTH + 1);
    
    /* Report no error if the symbol was not found, it just wasn't defined */
    if ( iError == UTL_ConfigSymbolNotFound ) {
        iError = SRCH_NoError;
        goto bailFromiSrchReportCreate;
    }
    /* Otherwise report the error if we got one */
    else if ( iError != UTL_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the seach report location from the search configuration, symbol name: '%s', utl error: %d.", SRCH_SEARCH_CONFIG_SEARCH_REPORT_LOCATION, iError); 
        iError = SRCH_ReportCreateFailed;
        goto bailFromiSrchReportCreate;
    }


    /* Check for memory */
    if ( s_strncasecmp(pucSearchReportLocation, SRCH_REPORT_MEMORY_STRING, s_strlen(SRCH_REPORT_MEMORY_STRING)) == 0 ) {

        /* Set the search report type */
        psrSrchReport->uiSearchReportType = SRCH_REPORT_TYPE_MEMORY;

        /* Create the string buffer handle */
        if ( (iError = iUtlStringBufferCreate(&psrSrchReport->pvUtlStringBuffer)) != UTL_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to create the string buffer for the in-memory search report, utl error: %d.", iError);
            iError = SRCH_MemError;
            goto bailFromiSrchReportCreate;
        }

        /* Set the search report mode */
        psrSrchReport->uiSearchReportMode = SRCH_REPORT_MODE_READ_WRITE;
    }

    /* Check for file system root ('/') or file protocol url */
    else if ( (pucSearchReportLocation[0] == UTL_FILE_DIRECTORY_ROOT_CHAR) ||
            (s_strncasecmp(pucSearchReportLocation, SRCH_REPORT_FILE_PROTOCOL_URL, s_strlen(SRCH_REPORT_FILE_PROTOCOL_URL)) == 0) ) {

        unsigned char   *pucSearchReportLocationPtr = NULL;
        unsigned char   pucSearchReportSubDirectoryMask[UTL_CONFIG_VALUE_MAXIMUM_LENGTH + 1] = {'\0'};


        /* Set the search report type */
        psrSrchReport->uiSearchReportType = SRCH_REPORT_TYPE_FILE;

        /* Get a pointer to the search report location path, exclude the file protocol url if it is there */
        pucSearchReportLocationPtr = (s_strncasecmp(pucSearchReportLocation, SRCH_REPORT_FILE_PROTOCOL_URL, s_strlen(SRCH_REPORT_FILE_PROTOCOL_URL)) == 0) ? 
                pucSearchReportLocation + s_strlen(SRCH_REPORT_FILE_PROTOCOL_URL) : pucSearchReportLocation;

        /* Clean the search report location path */
        if ( (iError = iUtlFileCleanPath(pucSearchReportLocationPtr)) != UTL_NoError ) {
            iUtlLogWarn(UTL_LOG_CONTEXT, "Failed to clean the search report directory path: '%s', utl error: %d.", pucSearchReportLocationPtr, iError);
            psrSrchReport->uiSearchReportMode = SRCH_REPORT_MODE_OFF;
        }
        
        /* Check that the search report location path really exists and that we can actually write to it */
        if ( bUtlFileIsDirectory(pucSearchReportLocationPtr) == false ) {
            iUtlLogWarn(UTL_LOG_CONTEXT, "Failed to find the search report directory: '%s'.", pucSearchReportLocationPtr);
            psrSrchReport->uiSearchReportMode = SRCH_REPORT_MODE_OFF;
        }
        else {
    
            /* Copy the cleaned search report location to the the search report directory path, we now know it is a directory */
            s_strnncpy(psrSrchReport->pucSearchReportDirectoryPath, pucSearchReportLocationPtr, UTL_FILE_PATH_MAX + 1);
    
            /* Check to see if we can read from and write to the search report directory */
            if ( (bUtlFilePathRead(psrSrchReport->pucSearchReportDirectoryPath) == true) && (bUtlFilePathWrite(psrSrchReport->pucSearchReportDirectoryPath) == true) ) {
                psrSrchReport->uiSearchReportMode = SRCH_REPORT_MODE_READ_WRITE;
            }
            /* Check to see if we can read from the search report directory */
            else if ( bUtlFilePathRead(psrSrchReport->pucSearchReportDirectoryPath) == true  ) {
                psrSrchReport->uiSearchReportMode = SRCH_REPORT_MODE_READ_ONLY;
            }
            
            /* Log the access level if we dont have full access */    
            if ( psrSrchReport->uiSearchReportMode == SRCH_REPORT_MODE_OFF ) {
                iUtlLogWarn(UTL_LOG_CONTEXT, "Failed to access the search report directory: '%s'.", psrSrchReport->pucSearchReportDirectoryPath);
            }
            else if ( psrSrchReport->uiSearchReportMode == SRCH_REPORT_MODE_READ_ONLY ) {
                iUtlLogInfo(UTL_LOG_CONTEXT, "Read-only access to the search report directory: '%s'.", psrSrchReport->pucSearchReportDirectoryPath);
            }
        }        

        /* Set the default search report subdirectory mask */
        psrSrchReport->pucSearchReportSubDirectoryMask[0] = '\0';

        /* Get the search report subdirectory mask from the configuration file */
        if ( iUtlConfigGetValue(pssSrchSearch->pvUtlConfig, SRCH_SEARCH_CONFIG_SEARCH_REPORT_SUBDIRECTORY_MASK, pucSearchReportSubDirectoryMask, UTL_CONFIG_VALUE_MAXIMUM_LENGTH + 1) == UTL_NoError ) {
            s_strnncpy(psrSrchReport->pucSearchReportSubDirectoryMask, pucSearchReportSubDirectoryMask, UTL_FILE_PATH_MAX + 1);
        }
    }
    
    /* Failed to identify the search report location */
    else {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to identify the seach report location from the search configuration, symbol name: '%s', symbol: '%s'.", 
                SRCH_SEARCH_CONFIG_SEARCH_REPORT_LOCATION, pucSearchReportLocation); 
        iError = SRCH_ReportCreateFailed;
        goto bailFromiSrchReportCreate;
    }



    /* Bail label */
    bailFromiSrchReportCreate:


    /* Handle the error */
    if ( iError == SRCH_NoError ) {

        /* Set the return pointer */
        *ppvSrchReport = psrSrchReport;
    }
    else {

        /* Free resources */
        iSrchReportClose((void *)psrSrchReport);
        psrSrchReport = NULL;
    }


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchReportClose()

    Purpose:    This function frees the search report structure.

    Parameters: pvSrchReport    search report structure

    Globals:    none

    Returns:    SRCH error code

*/
int iSrchReportClose
(
    void *pvSrchReport
)
{

    struct srchReport   *psrSrchReport = (struct srchReport *)pvSrchReport;


    /* Check the parameters */
    if ( pvSrchReport == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvSrchReport' parameter passed to 'iSrchReportClose'."); 
        return (SRCH_ReportInvalidReport);
    }


    /* Memory type */
    if ( psrSrchReport->uiSearchReportType == SRCH_REPORT_TYPE_MEMORY ) {

        /* Free the string buffer handle */
        iUtlStringBufferFree(psrSrchReport->pvUtlStringBuffer, true);
        psrSrchReport->pvUtlStringBuffer = NULL;
    }

    /* File type */
    else if ( psrSrchReport->uiSearchReportType == SRCH_REPORT_TYPE_FILE ) {
        ;
    }

    /* Unknown type */
    else if ( psrSrchReport->uiSearchReportType == SRCH_REPORT_TYPE_UNKNOWN ) {
        ;
    }

    /* Free the search report structure */
    s_free(psrSrchReport);


    return (SRCH_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchReportCreateReport()

    Purpose:    This function creates a new search report.

    Parameters: pvSrchReport    search report structure

    Globals:    none

    Returns:    SRCH error code

*/
int iSrchReportCreateReport
(
    void *pvSrchReport
)
{

    int                 iError = SRCH_NoError;
    struct srchReport   *psrSrchReport = (struct srchReport *)pvSrchReport;


    /* Check the parameters */
    if ( pvSrchReport == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvSrchReport' parameter passed to 'iSrchReportCreateReport'."); 
        return (SRCH_ReportInvalidReport);
    }


    /* Check the report mode, we cant open a report file if we cant read and write */
    if ( psrSrchReport->uiSearchReportMode != SRCH_REPORT_MODE_READ_WRITE ) {
        return (SRCH_ReportInvalidMode);
    }

    /* Check the report type, we can't handle an unknown cache type */
    if ( psrSrchReport->uiSearchReportType == SRCH_REPORT_TYPE_UNKNOWN ) {
        return (SRCH_ReportInvalidType);
    }


    /* Close the current search report */
    iSrchReportCloseReport((void *)psrSrchReport);


    /* Memory type */
    if ( psrSrchReport->uiSearchReportType == SRCH_REPORT_TYPE_MEMORY ) {

        /* Clear the string buffer handle */
        if ( (iError = iUtlStringBufferClear(psrSrchReport->pvUtlStringBuffer)) != UTL_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to clear the string buffer for the in-memory search report, utl error: %d.", iError);
            iError = SRCH_ReportCreateReportFailed;
            goto bailFromiSrchReportCreateReport;
        }
    }

    /* File type */
    else if ( psrSrchReport->uiSearchReportType == SRCH_REPORT_TYPE_FILE ) {
    
        unsigned char   pucTemporaryFilePath[UTL_FILE_PATH_MAX + 1] = {'\0'};
        unsigned char   pucSearchReportFileName[UTL_FILE_PATH_MAX + 1] = {'\0'};
        void            *pvUtlSHA1 = NULL;
        unsigned char   pucSHA1Digest[UTL_SHA1_DIGEST_LENGTH + 1] = {'\0'};
        unsigned char   pucSHA1HexDigest[UTL_SHA1_HEX_DIGEST_LENGTH + 1] = {'\0'};


        /* Init the SHA1 structure */
        if ( (iError = iUtlSHA1Create(&pvUtlSHA1)) != UTL_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to create a SHA1, utl error: %d.", iError);
            iError = SRCH_ReportCreateReportFailed;
            goto bailFromiSrchReportCreateReport;
        }

        /* Get a temporary file path */
        s_tmpnam(pucTemporaryFilePath);

        /* Add the temporary file path */
        if ( (iError = iUtlSHA1Update(pvUtlSHA1, pucTemporaryFilePath, s_strlen(pucTemporaryFilePath))) != UTL_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to update a SHA1, utl error: %d.", iError);
            iError = SRCH_ReportCreateReportFailed;
            goto bailFromiSrchReportCreateReport;
        }
    
        /* Get the digest of the SHA1 key */
        if ( (iError = iUtlSHA1Digest(pvUtlSHA1, pucSHA1Digest, pucSHA1HexDigest)) != UTL_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to get a SHA1 digest, utl error: %d.", iError);
            iError = SRCH_ReportCreateReportFailed;
            goto bailFromiSrchReportCreateReport;
        }

        /* Null terminate the digest */
        pucSHA1HexDigest[UTL_SHA1_HEX_DIGEST_LENGTH] = '\0';

        /* Create the search report file name */
        snprintf(pucSearchReportFileName, UTL_FILE_PATH_MAX + 1, "%s%s", pucSHA1HexDigest, SRCH_REPORT_FILENAME_EXTENSION);

        /* Get the search report file path for this search report file name */
        if ( (iError = iSrchReportGetReportFilePath(psrSrchReport, pucSearchReportFileName, psrSrchReport->pucSearchReportFilePath, UTL_FILE_PATH_MAX + 1)) != SRCH_NoError ) {
            goto bailFromiSrchReportCreateReport;
        }

        /* Open the search report */
        if ( (psrSrchReport->pfSearchReportFile = s_fopen(psrSrchReport->pucSearchReportFilePath, "w+")) == NULL ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to create the search report file: '%s'.", psrSrchReport->pucSearchReportFilePath);
            iError = SRCH_ReportCreateReportFailed;
            goto bailFromiSrchReportCreateReport;
        }
    }



    /* Bail label */
    bailFromiSrchReportCreateReport:


    /* Handle the error */
    if ( iError != SRCH_NoError ) {

        /* Memory type */
        if ( psrSrchReport->uiSearchReportType == SRCH_REPORT_TYPE_MEMORY ) {
            ;
        }

        /* File type */
        else if ( psrSrchReport->uiSearchReportType == SRCH_REPORT_TYPE_FILE ) {

            /* Remove the file */
            s_remove(psrSrchReport->pucSearchReportFilePath);
    
            /* Close the file  */
            s_fclose(psrSrchReport->pfSearchReportFile);
        }

        /* Clear the search report file path */
        psrSrchReport->pucSearchReportFilePath[0] = '\0';
    }


    return (SRCH_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchReportCloseReport()

    Purpose:    This function closes the search report.

    Parameters: pvSrchReport    search report structure

    Globals:    none


    Returns:    SRCH error code

*/
int iSrchReportCloseReport
(
    void *pvSrchReport
)
{

    int                 iError = SRCH_NoError;
    struct srchReport   *psrSrchReport = (struct srchReport *)pvSrchReport;


    /* Check the parameters */
    if ( pvSrchReport == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvSrchReport' parameter passed to 'iSrchReportCloseReport'."); 
        return (SRCH_ReportInvalidReport);
    }


    /* Check the report mode, we cant close a report file if we cant read and write */
    if ( psrSrchReport->uiSearchReportMode != SRCH_REPORT_MODE_READ_WRITE ) {
        return (SRCH_ReportInvalidMode);
    }

    /* Check the report type, we can't handle an unknown cache type */
    if ( psrSrchReport->uiSearchReportType == SRCH_REPORT_TYPE_UNKNOWN ) {
        return (SRCH_ReportInvalidType);
    }


    /* Memory type */
    if ( psrSrchReport->uiSearchReportType == SRCH_REPORT_TYPE_MEMORY ) {
        ;
    }

    /* File type */
    else if ( psrSrchReport->uiSearchReportType == SRCH_REPORT_TYPE_FILE ) {

        /* Skip this if the search report file is not defined */
        if ( psrSrchReport->pfSearchReportFile == NULL ) {
            return (SRCH_NoError);
        }
    
        /* Flush the search report file */
        s_fflush(psrSrchReport->pfSearchReportFile);
        
        /* Close the search report file */    
        s_fclose(psrSrchReport->pfSearchReportFile);

/*         iUtlLogDebug(UTL_LOG_CONTEXT, "Saved search report to file: '%s'.", psrSrchReport->pucSearchReportFilePath); */
    }


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchReportAppend()

    Purpose:    This function appends data to the search report.

    Parameters: pvSrchReport        search report structure
                pcFormatString      format string
                ...                 args (optional)

    Globals:    none

    Returns:    SRCH error code

*/
int iSrchReportAppend
(
    void *pvSrchReport,
    unsigned char *pucFormat,
    ...
)
{

    struct srchReport   *psrSrchReport = (struct srchReport *)pvSrchReport;
    va_list             ap;


    /* Check the parameters */
    if ( pvSrchReport == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvSrchReport' parameter passed to 'iSrchReportAppend'."); 
        return (SRCH_ReportInvalidReport);
    }

    if ( bUtlStringsIsStringNULL(pucFormat) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pucFormat' parameter passed to 'iSrchReportAppend'."); 
        return (SRCH_ReportInvalidFormat);
    }


    /* Check the report mode, we cant appnend to a report file if we cant read and write */
    if ( psrSrchReport->uiSearchReportMode != SRCH_REPORT_MODE_READ_WRITE ) {
        return (SRCH_ReportInvalidMode);
    }

    /* Check the report type, we can't handle an unknown cache type */
    if ( psrSrchReport->uiSearchReportType == SRCH_REPORT_TYPE_UNKNOWN ) {
        return (SRCH_ReportInvalidType);
    }


    /* Memory type */
    if ( psrSrchReport->uiSearchReportType == SRCH_REPORT_TYPE_MEMORY ) {

        unsigned char   pucReportEntry[UTL_FILE_PATH_MAX * 2] = {'\0'};
        
        /* Create the report entry */
        va_start(ap, pucFormat);
        vsnprintf(pucReportEntry, UTL_FILE_PATH_MAX * 2, pucFormat, ap); 
        va_end(ap);

        /* Add the report entry to the string buffer handle */
        iUtlStringBufferAppend(psrSrchReport->pvUtlStringBuffer, pucReportEntry);
    }
    
    /* File type */
    else if ( psrSrchReport->uiSearchReportType == SRCH_REPORT_TYPE_FILE ) {

        /* Print the entry */
        va_start(ap, pucFormat);
        if ( psrSrchReport->pfSearchReportFile != NULL ) {
            vfprintf(psrSrchReport->pfSearchReportFile, pucFormat, ap); 
        }
        va_end(ap);
    }


    return (SRCH_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchReportGetReportKey()

    Purpose:    This function returns the current search report ID.

    Parameters: pvSrchReport                search report structure
                pucSearchReportKey          return pointer for the search report ID
                uiSearchReportKeyLength     length of the search report ID

    Globals:    none

    Returns:    SRCH error code

*/
int iSrchReportGetReportKey
(
    void *pvSrchReport,
    unsigned char *pucSearchReportKey,
    unsigned int uiSearchReportKeyLength
)
{

    int                 iError = SRCH_NoError;
    struct srchReport   *psrSrchReport = (struct srchReport *)pvSrchReport;
    unsigned char       *pucSearchReportFileNamePtr = NULL;


    /* Check the parameters */
    if ( pvSrchReport == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvSrchReport' parameter passed to 'iSrchReportGetReportKey'."); 
        return (SRCH_ReportInvalidReport);
    }

    if ( pucSearchReportKey == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pucSearchReportKey' parameter passed to 'iSrchReportGetReportKey'."); 
        return (SRCH_ReturnParameterError);
    }

    if ( uiSearchReportKeyLength <= 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'uiSearchReportKeyLength' parameter passed to 'iSrchReportGetReportKey'."); 
        return (SRCH_ReturnParameterError);
    }


    /* Check the report mode, we cant get a report ID if we cant read and write */
    if ( psrSrchReport->uiSearchReportMode != SRCH_REPORT_MODE_READ_WRITE ) {
        return (SRCH_ReportInvalidMode);
    }

    /* Check the report type, we can't handle an unknown cache type */
    if ( psrSrchReport->uiSearchReportType == SRCH_REPORT_TYPE_UNKNOWN ) {
        return (SRCH_ReportInvalidType);
    }


    /* Memory type */
    if ( psrSrchReport->uiSearchReportType == SRCH_REPORT_TYPE_MEMORY ) {
        
        /* Create a fictitious name as the search report ID */
        snprintf(pucSearchReportKey, uiSearchReportKeyLength, "%s%s", SRCH_REPORT_MEMORY_STRING, SRCH_REPORT_FILENAME_EXTENSION);
    }
    
    /* File type */
    else if ( psrSrchReport->uiSearchReportType == SRCH_REPORT_TYPE_FILE) {

        /* Check that the report file path exists */
        if ( bUtlStringsIsStringNULL(psrSrchReport->pucSearchReportFilePath) == true ) {
            return (SRCH_ReportGetReportIDFailed);
        }
    
        /* Get a pointer to the base name of the file path */
        if ( (iError = iUtlFileGetPathBase(psrSrchReport->pucSearchReportFilePath, &pucSearchReportFileNamePtr)) != UTL_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the file name from the search report file path: '%s', utl error: %d.", psrSrchReport->pucSearchReportFilePath, iError);
            return (SRCH_ReportGetReportIDFailed);
        }
    
        /* Copy the search report file base name as the ID into the return pointer */
        s_strnncpy(pucSearchReportKey, pucSearchReportFileNamePtr, uiSearchReportKeyLength);
    }


    return (SRCH_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchReportGetReportOffset()

    Purpose:    This function returns the current search report file offset.

    Parameters: pvSrchReport            search report structure
                pzSearchReportOffset    return pointer for the search report offset

    Globals:    none

    Returns:    SRCH error code

*/
int iSrchReportGetReportOffset
(
    void *pvSrchReport,
    off_t *pzSearchReportOffset
)
{

    struct srchReport   *psrSrchReport = (struct srchReport *)pvSrchReport;


    /* Check the parameters */
    if ( pvSrchReport == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvSrchReport' parameter passed to 'iSrchReportGetReportOffset'."); 
        return (SRCH_ReportInvalidReport);
    }

    if ( pzSearchReportOffset == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pzSearchReportOffset' parameter passed to 'iSrchReportGetReportOffset'."); 
        return (SRCH_ReturnParameterError);
    }


    /* Check the report mode, we cant get a report file offset if we cant read and write */
    if ( psrSrchReport->uiSearchReportMode != SRCH_REPORT_MODE_READ_WRITE ) {
        return (SRCH_ReportInvalidMode);
    }

    /* Check the report type, we can't handle an unknown cache type */
    if ( psrSrchReport->uiSearchReportType == SRCH_REPORT_TYPE_UNKNOWN ) {
        return (SRCH_ReportInvalidType);
    }


    /* Memory type */
    if ( psrSrchReport->uiSearchReportType == SRCH_REPORT_TYPE_MEMORY ) {
        
        /* Get the string buffer length */
        iUtlStringBufferGetLength(psrSrchReport->pvUtlStringBuffer, (size_t *)pzSearchReportOffset);
    }
    
    /* File type */
    else if ( psrSrchReport->uiSearchReportType == SRCH_REPORT_TYPE_FILE ) {

        /* Set the return pointer */
        if ( (*pzSearchReportOffset = s_ftell(psrSrchReport->pfSearchReportFile)) == -1 ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the current search report file offset.");
            return (SRCH_ReportGetReportOffsetFailed);
        }
    }


    return (SRCH_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchReportGetReportSnippet()

    Purpose:    This function returns the search report snippet for the currently
                open report.

    Parameters: pvSrchReport                search report structure
                zSearchReportStartOffset    search report start index
                iSearchReportEndOffset      search report end index
                ppucSearchReportSnippet     return pointer for the search report snippet (free)

    Globals:    none

    Returns:    SRCH error code

*/
int iSrchReportGetReportSnippet
(
    void *pvSrchReport,
    off_t zSearchReportStartOffset,
    off_t zSearchReportEndOffset,
    unsigned char **ppucSearchReportSnippet
)
{

    int                 iError = SRCH_NoError;
    struct srchReport   *psrSrchReport = (struct srchReport *)pvSrchReport;
    unsigned char       *pucSearchReportSnippet = NULL;
    off_t               zSearchReportLength = 0;
    off_t               zSearchReportCurrentOffset = 0;
    int                 uiSearchReportSnippetLength = 0;


    /* Check the parameters */
    if ( pvSrchReport == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvSrchReport' parameter passed to 'iSrchReportGetReportSnippet'."); 
        return (SRCH_ReportInvalidReport);
    }

    if ( zSearchReportStartOffset < 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'zSearchReportStartOffset' parameter passed to 'iSrchReportGetReportSnippet'."); 
        return (SRCH_ReportInvalidReportOffset);
    }

    if ( zSearchReportEndOffset < 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'zSearchReportEndOffset' parameter passed to 'iSrchReportGetReportSnippet'."); 
        return (SRCH_ReportInvalidReportOffset);
    }

    if ( zSearchReportStartOffset > zSearchReportEndOffset ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'zSearchReportStartOffset' & 'zSearchReportEndOffset' parameters passed to 'iSrchReportGetReportSnippet'."); 
        return (SRCH_ReportInvalidReportOffset);
    }

    if ( ppucSearchReportSnippet == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppucSearchReportSnippet' parameter passed to 'iSrchReportGetReportSnippet'."); 
        return (SRCH_ReturnParameterError);
    }


    /* Check the report mode, we cant save a report file snippet if we cant read and write */
    if ( psrSrchReport->uiSearchReportMode != SRCH_REPORT_MODE_READ_WRITE ) {
        return (SRCH_ReportInvalidMode);
    }

    /* Check the report type, we can't handle an unknown cache type */
    if ( psrSrchReport->uiSearchReportType == SRCH_REPORT_TYPE_UNKNOWN ) {
        return (SRCH_ReportInvalidType);
    }


    /* Memory type */
    if ( psrSrchReport->uiSearchReportType == SRCH_REPORT_TYPE_MEMORY ) {
        
        unsigned char   *pucSearchReportText = NULL;

        /* Get the string buffer length */
        iUtlStringBufferGetLength(psrSrchReport->pvUtlStringBuffer, (size_t *)&zSearchReportLength);

        /* Check the snippet offsets */
        if ( !((zSearchReportStartOffset >= 0) && (zSearchReportStartOffset <= zSearchReportEndOffset) && (zSearchReportEndOffset <= zSearchReportLength)) ) {
            iError = SRCH_ReportGetReportSnippetFailed;
            goto bailFromiGetSearchReportSnippet;
        }

        /* Get the search report snippet length */
        uiSearchReportSnippetLength = (zSearchReportEndOffset - zSearchReportStartOffset) + 1;
        
        /* Allocate space, make sure that we allocate space for the terminating NULL */
        if ( (pucSearchReportSnippet = (unsigned char *)s_malloc(sizeof(unsigned char) * (uiSearchReportSnippetLength + 1))) == NULL ) {
            iError = SRCH_MemError;
            goto bailFromiGetSearchReportSnippet;
        }
    
        /* Get the string buffer string pointer */
        iUtlStringBufferGetString(psrSrchReport->pvUtlStringBuffer, &pucSearchReportText);

        /* Copy the search report snippet into the return pointer */
        s_strnncpy(pucSearchReportSnippet, pucSearchReportText + zSearchReportStartOffset, uiSearchReportSnippetLength);
    }
    
    /* File type */
    else if ( psrSrchReport->uiSearchReportType == SRCH_REPORT_TYPE_FILE ) {

        /* Make sure we flush the file, some operating systems report funny lengths if it has not been flushed */
        s_fflush(psrSrchReport->pfSearchReportFile);
    

        /* Get the search report length */
        if ( (iError = iUtlFileGetFileLength(psrSrchReport->pfSearchReportFile, &zSearchReportLength)) != UTL_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the file length of the search report file: '%s', utl error: %d.", psrSrchReport->pucSearchReportFilePath, iError);
            iError = SRCH_ReportGetReportSnippetFailed;
            goto bailFromiGetSearchReportSnippet;
        }
    
        /* Check the snippet offsets */
        if ( !((zSearchReportStartOffset >= 0) && (zSearchReportStartOffset <= zSearchReportEndOffset) && (zSearchReportEndOffset <= zSearchReportLength)) ) {
            iError = SRCH_ReportGetReportSnippetFailed;
            goto bailFromiGetSearchReportSnippet;
        }
        
    
        /* Get the search report snippet length */
        uiSearchReportSnippetLength = (zSearchReportEndOffset - zSearchReportStartOffset) + 1;
        
        /* Allocate space, make sure that we allocate space for the terminating NULL */
        if ( (pucSearchReportSnippet = (unsigned char *)s_malloc(sizeof(unsigned char) * (uiSearchReportSnippetLength + 1))) == NULL ) {
            iError = SRCH_MemError;
            goto bailFromiGetSearchReportSnippet;
        }
    
        /* Get the current offset, we need that because we are going to 
        ** seek back in the file to the start of the snippet
        */
        if ( (zSearchReportCurrentOffset = s_ftell(psrSrchReport->pfSearchReportFile)) == -1 ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the current search report file offset.");
            iError = SRCH_ReportGetReportSnippetFailed;
            goto bailFromiGetSearchReportSnippet;
        }
        
        /* Seek to the start of the snippet */
        if ( s_fseek(psrSrchReport->pfSearchReportFile, zSearchReportStartOffset, SEEK_SET) != 0 ) {
            iError = SRCH_ReportGetReportSnippetFailed;
            goto bailFromiGetSearchReportSnippet;
        }
        
        /* Read the snippet */
        if ( s_fread(pucSearchReportSnippet, uiSearchReportSnippetLength, 1, psrSrchReport->pfSearchReportFile) != 1 ) {
            iError = SRCH_ReportGetReportSnippetFailed;
            goto bailFromiGetSearchReportSnippet;
        }
    
        /* Null terminate the snippet */
        pucSearchReportSnippet[uiSearchReportSnippetLength] = '\0';
    }
    


    /* Bail label */
    bailFromiGetSearchReportSnippet:


    /* Memory type */
    if ( psrSrchReport->uiSearchReportType == SRCH_REPORT_TYPE_MEMORY ) {
        ;
    }
    
    /* File type */
    else if ( psrSrchReport->uiSearchReportType == SRCH_REPORT_TYPE_FILE ) {

        /* Restore the current offset, note that we always try to do this */
        if ( s_fseek(psrSrchReport->pfSearchReportFile, zSearchReportCurrentOffset, SEEK_SET) != 0 ) {
            iError = SRCH_ReportGetReportSnippetFailed;
        }
    }

    /* Handle the error */
    if ( iError == SRCH_NoError ) {

        /* Set the return pointer */
        *ppucSearchReportSnippet = pucSearchReportSnippet;
    }
    else {

        /* Free any allocated resources */
        s_free(pucSearchReportSnippet);
    }


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchReportGetReportText()

    Purpose:    This function returns the search report text.

    Parameters: pvSrchReport            search report structure
                pucSearchReportKey      search report ID
                ppucSearchReportText    return pointer for the search report text (allocated)

    Globals:    none

    Returns:    SRCH error code

*/
int iSrchReportGetReportText
(
    void *pvSrchReport,
    unsigned char *pucSearchReportKey,
    unsigned char **ppucSearchReportText
)
{

    int                 iError = SRCH_NoError;
    struct srchReport   *psrSrchReport = (struct srchReport *)pvSrchReport;
    FILE                *pfSearchReportFile = NULL;
    off_t               zSearchReportFileDataLength = 0;
    unsigned char       *pucSearchReportText = NULL;


    /* Check the parameters */
    if ( pvSrchReport == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvSrchReport' parameter passed to 'iSrchReportGetReportSnippet'."); 
        return (SRCH_ReportInvalidReport);
    }

    if ( bUtlStringsIsStringNULL(pucSearchReportKey) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pucSearchReportKey' parameter passed to 'iSrchReportGetReportSnippet'."); 
        return (SRCH_ReportInvalidReportToken);
    }

    if ( ppucSearchReportText == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppucSearchReportText' parameter passed to 'iSrchReportGetReportSnippet'."); 
        return (SRCH_ReturnParameterError);
    }


    /* Check the report mode, we cant get a report text if it is off */
    if ( psrSrchReport->uiSearchReportMode == SRCH_REPORT_MODE_OFF ) {
        return (SRCH_ReportInvalidMode);
    }

    /* Check the report type, we can't handle an unknown cache type */
    if ( psrSrchReport->uiSearchReportType == SRCH_REPORT_TYPE_UNKNOWN ) {
        return (SRCH_ReportInvalidType);
    }


    /* Memory type */
    if ( psrSrchReport->uiSearchReportType == SRCH_REPORT_TYPE_MEMORY ) {

        unsigned char   *pucSearchReportTextPtr = NULL;

        /* Get the string buffer string pointer */
        iUtlStringBufferGetString(psrSrchReport->pvUtlStringBuffer, &pucSearchReportTextPtr);

        /* Duplicate the search report text */
        if ( (pucSearchReportText = s_strdup(pucSearchReportTextPtr)) == NULL ) {
            iError = SRCH_MemError;
            goto bailFromiSrchReportGetReportText;
        }

/*         iUtlLogDebug(UTL_LOG_CONTEXT, "Got search report from memory."); */
    }

    /* File type */
    else if ( psrSrchReport->uiSearchReportType == SRCH_REPORT_TYPE_FILE ) {
        
        unsigned char   pucSearchReportFilePath[UTL_FILE_PATH_MAX + 1] = {'\0'};

        /* Get the search report file path for this search report file name */
        if ( (iError = iSrchReportGetReportFilePath(psrSrchReport, pucSearchReportKey, pucSearchReportFilePath, UTL_FILE_PATH_MAX + 1)) != SRCH_NoError ) {
            goto bailFromiSrchReportGetReportText;
        }
    
        /* Open the search report */
        if ( (pfSearchReportFile = s_fopen(pucSearchReportFilePath, "r")) == NULL ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to open the search report file: '%s'.", pucSearchReportFilePath);
            iError = SRCH_ReportGetReportTextFailed;
            goto bailFromiSrchReportGetReportText;
        }
    
        /* Get the file length */
        if ( (iError = iUtlFileGetFileLength(pfSearchReportFile, &zSearchReportFileDataLength)) != UTL_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the length of the search report file: '%s', utl error: %d.", psrSrchReport->pucSearchReportFilePath, iError);
            iError = SRCH_ReportGetReportTextFailed;
            goto bailFromiSrchReportGetReportText;
        }
        
        /* Allocate the search report text pointer */
        if ( (pucSearchReportText = s_malloc((size_t)(sizeof(unsigned char) * (zSearchReportFileDataLength + 1)))) == NULL ) {
            iError = SRCH_MemError;
            goto bailFromiSrchReportGetReportText;
        }
        
        /* Read the search report text */
        if ( s_fread(pucSearchReportText, zSearchReportFileDataLength, 1, pfSearchReportFile) != 1 ) {
            iError = SRCH_ReportGetReportTextFailed;
            goto bailFromiSrchReportGetReportText;
        }

/*         iUtlLogDebug(UTL_LOG_CONTEXT, "Read search report from file: '%s'.", pucSearchReportFilePath); */
    }
    


    /* Bail label */
    bailFromiSrchReportGetReportText:


    /* Memory type */
    if ( psrSrchReport->uiSearchReportType == SRCH_REPORT_TYPE_MEMORY ) {
        ;
    }

    /* File type */
    else if ( psrSrchReport->uiSearchReportType == SRCH_REPORT_TYPE_FILE ) {
        
        /* Close the search report file */
        s_fclose(pfSearchReportFile);
    }

    /* Handle the error */
    if ( iError == SRCH_NoError ) {

        /* Set the return pointer */
        *ppucSearchReportText = pucSearchReportText;
    }
    else {
        
        /* Free allocations */
        s_free(pucSearchReportText);
    }


    return (iError);

}


/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchReportGetReportFilePath()

    Purpose:    This function gets a file path to the passed search report file name. 

    Parameters: pvSrchReport                    search report structure
                pucSearchReportFileName         search report file name
                pucSearchReportFilePath         return pointer for the search report file path
                uiSearchReportFilePathLength    length of the search report file path

    Globals:    none

    Returns:    DBM error code

*/
static int iSrchReportGetReportFilePath
(
    struct srchReport *psrSrchReport,
    unsigned char *pucSearchReportFileName,
    unsigned char *pucSearchReportFilePath,
    unsigned int uiSearchReportFilePathLength
)
{

    int     iError = SRCH_NoError;


    ASSERT(psrSrchReport != NULL);
    ASSERT(bUtlStringsIsStringNULL(pucSearchReportFileName) == false);
    ASSERT(pucSearchReportFilePath != NULL);
    ASSERT(uiSearchReportFilePathLength > 0);


    /* Create the cache file path, different methods if we need to create a search report subdirectory */
    if ( bUtlStringsIsStringNULL(psrSrchReport->pucSearchReportSubDirectoryMask) == true ) {
        
        /* Create a search report file name */
        if ( (iError = iUtlFileMergePaths(psrSrchReport->pucSearchReportDirectoryPath, pucSearchReportFileName, pucSearchReportFilePath, uiSearchReportFilePathLength)) != UTL_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to create the search report file path, search report file name: '%s', search report directory path: '%s', utl error: %d.", 
                    pucSearchReportFileName, psrSrchReport->pucSearchReportDirectoryPath, iError);
            return (SRCH_ReportGetFilePathFailed);
        }
    }
    else {

        unsigned char   pucSearchReportSubdirectoryPath[UTL_FILE_PATH_MAX + 1] = {'\0'};
        unsigned char   *pucSearchReportSubdirectoryPathPtr = NULL;
        unsigned char   pucSearchReportDirectoryPath[UTL_FILE_PATH_MAX + 1] = {'\0'};


        /* Copy the search report subdirectory mask to the search report subdirectory path */
        s_strnncpy(pucSearchReportSubdirectoryPath, psrSrchReport->pucSearchReportSubDirectoryMask, UTL_FILE_PATH_MAX + 1);
    
        /* Replace digits with offsets into the search report file name */
        for ( pucSearchReportSubdirectoryPathPtr = pucSearchReportSubdirectoryPath; *pucSearchReportSubdirectoryPathPtr != '\0'; pucSearchReportSubdirectoryPathPtr++ ) {
            if ( (*pucSearchReportSubdirectoryPathPtr >= '0') && (*pucSearchReportSubdirectoryPathPtr <= '9') ) {
                *pucSearchReportSubdirectoryPathPtr = pucSearchReportFileName[*pucSearchReportSubdirectoryPathPtr - '0'];
            }
        }


        /* Create the search report directory path if we are in read/write mode */
        if ( psrSrchReport->uiSearchReportMode == SRCH_REPORT_MODE_READ_WRITE ) {

            /* Create the search report directory path */
            if ( (iError = iUtlFileCreateDirectoryPath(pucSearchReportDirectoryPath, pucSearchReportDirectoryPath + s_strlen(psrSrchReport->pucSearchReportDirectoryPath), 
                     S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH)) != UTL_NoError ) {
                iUtlLogError(UTL_LOG_CONTEXT, "Failed to create the search report directory: '%s', utl error: %d.", pucSearchReportDirectoryPath, iError);
                return (SRCH_CacheCreateDirectoryFailed);
            }
        }


        /* Create a search report file path using the seach report directory as a base */
        if ( (iError = iUtlFileMergePaths(pucSearchReportDirectoryPath, pucSearchReportFileName, pucSearchReportFilePath, uiSearchReportFilePathLength)) != UTL_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to create the search report subdirectory path, search report file name: '%s', search report directory path: '%s', utl error: %d.", 
                    pucSearchReportFileName, pucSearchReportDirectoryPath, iError);
            return (SRCH_ReportGetFilePathFailed);
        }
    }


    return (SRCH_NoError);

}


/*---------------------------------------------------------------------------*/
