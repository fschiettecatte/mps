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

    Module:     verify.c

    Author:     Francois Schiettecatte (FS Consulting LLC.)

    Created:    5 March 1994

    Purpose:    This little application allows us to verify the inverted
                index created for a index. This is done by accessing
                the index structures using code that is different from the
                code that the search engine uses. This is to ensure that
                the index structures were correctly created.

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
#define UTL_LOG_CONTEXT                     (unsigned char *)"com.fsconsult.mps.src.misc.verify"


/*---------------------------------------------------------------------------*/


/* 
** Defines
*/

/* The first and last keys in the dictionary */
#define UTL_DICT_FIRST_KEY_FLAG                             (unsigned char *)"\40"              /* Ascii 32 */
#define UTL_DICT_LAST_KEY_FLAG                              (unsigned char *)"\377\377"         /* Ascii 255 */


/* These are extracted from ./src/search/document.h */
#define SRCH_DOCUMENT_DATA_ID_SIZE                          (8)
#define SRCH_DOCUMENT_RANK_SIZE                             (4)
#define SRCH_DOCUMENT_TERM_COUNT_SIZE                       (4)
#define SRCH_DOCUMENT_DATE_SIZE                             (6)
#define SRCH_DOCUMENT_LANGUAGE_ID_SIZE                      (2)

#define SRCH_DOCUMENT_ENTRY_LENGTH                          (SRCH_DOCUMENT_DATA_ID_SIZE + \
                                                                    SRCH_DOCUMENT_RANK_SIZE + \
                                                                    SRCH_DOCUMENT_TERM_COUNT_SIZE + \
                                                                    SRCH_DOCUMENT_DATE_SIZE + \
                                                                    SRCH_DOCUMENT_LANGUAGE_ID_SIZE)



/* These are extracted from ./src/search/invert.c */
#define SRCH_INVERT_INDEX_BLOCK_TERM_TYPE_SIZE              (1)
#define SRCH_INVERT_INDEX_BLOCK_TERM_COUNT_SIZE             (4)
#define SRCH_INVERT_INDEX_BLOCK_DOCUMENT_COUNT_SIZE         (4)
#define SRCH_INVERT_INDEX_BLOCK_TERM_SIZE                   (2)
#define SRCH_INVERT_INDEX_BLOCK_INCLUDE_IN_COUNTS_SIZE      (1)

#define SRCH_INVERT_INDEX_BLOCK_HEADER_LENGTH               (SRCH_INVERT_INDEX_BLOCK_TERM_TYPE_SIZE + \
                                                                    SRCH_INVERT_INDEX_BLOCK_TERM_COUNT_SIZE + \
                                                                    SRCH_INVERT_INDEX_BLOCK_DOCUMENT_COUNT_SIZE + \
                                                                    SRCH_INVERT_INDEX_BLOCK_TERM_SIZE + \
                                                                    SRCH_INVERT_INDEX_BLOCK_INCLUDE_IN_COUNTS_SIZE)

/* These are extracted from ./src/search/invert.c */
#define SRCH_INVERT_INDEX_BLOCK_DATA_LENGTH_SIZE            UTL_NUM_UINT_MAX_SIZE


/*---------------------------------------------------------------------------*/


/*
** Defines
*/

/* Default document coverage */
#define VRF_DOCUMENT_COVERAGE_DEFAULT                       (0.0)

/* Default frequent term percentage */
#define VRF_FREQUENT_TERM_COVERAGE_DEFAULT                  (0.0)


/* Number of postings to check when searching for a median */
#define VRF_INDEX_BLOCK_LEN_TO_CHECK_MAX                    (100000)


/* Number of terms to check when searching for a median */
#define VRF_TERM_COUNT_TO_CHECK_MAX                         (100000)


/* Default locale name */
#define VRF_LOCALE_NAME_DEFAULT                             LNG_LOCALE_EN_US_UTF_8_NAME


/*---------------------------------------------------------------------------*/


/*
** Private function prototypes
*/

static void vVersion (void);
static void vUsage (unsigned char *pucCommandPath);

static void    vListTerms (unsigned char *pucIndexDirectoryPath, unsigned char *pucConfigurationDirectoryPath,
        unsigned char *pucIndexName, boolean bListTerms, boolean bListStopTerms, boolean bListIndex, 
        boolean bListCoverage, float fDocumentCoverage, float fFrequentTermCoverage, 
        unsigned char *pucFieldName, unsigned char *pucDocumentKey, unsigned int uiDocumentID);

static int iListTermsCallBack (unsigned char *pucKey, void *pvEntryData, unsigned int uiEntryLength, va_list ap);


static void vListTerm (unsigned char *pucIndexDirectoryPath, unsigned char *pucConfigurationDirectoryPath,
        unsigned char *pucIndexName, unsigned char *pucTerm, boolean bListIndex, boolean bListCoverage,
        unsigned char *pucFieldName, unsigned char *pucDocumentKey, unsigned int uiDocumentID);

static void    vListIndexBlock (struct srchIndex *psiSrchIndex, unsigned long ulIndexBlockID,
        unsigned int uiFieldID, unsigned int uiDocumentID);

static void vListDocuments (unsigned char *pucIndexDirectoryPath, unsigned char *pucConfigurationDirectoryPath,
        unsigned char *pucIndexName);

static void vListDocument (unsigned char *pucIndexDirectoryPath, unsigned char *pucConfigurationDirectoryPath,
        unsigned char *pucIndexName, unsigned char *pucDocumentKey,
        unsigned int uiDocumentID);

static void    vListDocumentKeys (unsigned char *pucIndexDirectoryPath, unsigned char *pucConfigurationDirectoryPath,
        unsigned char *pucIndexName);

static int iListDocumentKeysCallBack (unsigned char *pucKey, void *pvEntryData, unsigned int uiEntryLength, va_list ap);

static void    vListStats (unsigned char *pucIndexDirectoryPath, unsigned char *pucConfigurationDirectoryPath,
        unsigned char *pucIndexName);

static int iListStatsCallBack (unsigned char *pucKey, void *pvEntryData, unsigned int uiEntryLength, va_list ap);

static void vListTemporaryIndexFile (unsigned char *pucTemporaryIndexFilePath, boolean bListIndexBlockHeader,
        boolean bListIndexBlockData);

static void    vTest (void);


/*---------------------------------------------------------------------------*/


/*

    Function:   main()

    Purpose:    The main function

    Parameters: argc, argv

    Globals:    none

    Returns:    int

*/
int main
(
    int argc,
    char *argv[]
)
{

    int             iError = SPI_NoError;
    unsigned char   *pucCommandPath = NULL;
    unsigned char   *pucNextArgument = NULL;

    unsigned char   pucConfigurationDirectoryPath[UTL_FILE_PATH_MAX + 1] = {'\0'};
    unsigned char   pucIndexDirectoryPath[UTL_FILE_PATH_MAX + 1] = {'\0'};
    unsigned char   *pucIndexName = NULL;

    unsigned char   *pucTerm = NULL;
    unsigned char   *pucFieldName = NULL;
    unsigned char   *pucDocumentKey = NULL;
    unsigned int    uiDocumentID = 0;

    boolean         bListTerms = false;
    boolean         bListTerm = false;
    boolean         bListStopTerms = false;
    boolean         bListIndex = false;
    boolean         bListCoverage = false;
    boolean         bListDocuments = false;
    boolean         bListDocumentKeys = false;
    boolean         bListStats = false;
    boolean         bTest = false;

    unsigned char   pucTemporaryIndexFilePath[UTL_FILE_PATH_MAX + 1] = {'\0'};
    boolean         bListIndexBlockHeader = false;
    boolean         bListIndexBlockData = false;

    float           fDocumentCoverage = VRF_DOCUMENT_COVERAGE_DEFAULT;
    float           fFrequentTermCoverage = VRF_FREQUENT_TERM_COVERAGE_DEFAULT;

    unsigned char   *pucLocaleName = VRF_LOCALE_NAME_DEFAULT;

    unsigned char   *pucLogFilePath = UTL_LOG_FILE_STDERR;    
    unsigned int    uiLogLevel = UTL_LOG_LEVEL_INFO;


    /* Initialize the log */
    if ( (iError = iUtlLogInit()) != UTL_NoError ) {
        vVersion();
        iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to initialize the log, utl error: %d", iError);
    }



    /* Get the command path */
    pucCommandPath = pucUtlArgsGetNextArg(&argc, &argv);

    /* Get the next argument, bail if none found */
    if ( (pucNextArgument = pucUtlArgsGetNextArg(&argc, &argv)) == NULL ) {
        vVersion();
        vUsage(pucCommandPath);
        s_exit(EXIT_SUCCESS);
    }

    /* Process arguments */
    while( pucNextArgument != NULL ) {

        /* Check for configuration directory */
        if ( s_strncmp("--configuration-directory=", pucNextArgument, s_strlen("--configuration-directory=")) == 0 ) {

            /* Get the configuration directory path */
            pucNextArgument += s_strlen("--configuration-directory=");

            /* Get the true configuration directory path */
            if ( (iError = iUtlFileGetTruePath(pucNextArgument, pucConfigurationDirectoryPath, UTL_FILE_PATH_MAX + 1)) != UTL_NoError ) {
                vVersion();
                iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to get the true configuration directory path: '%s', utl error: %d", pucNextArgument, iError);
            }

            /* Check that configuration directory path exists */
            if ( bUtlFilePathExists(pucConfigurationDirectoryPath) == false ) {
                vVersion();
                iUtlLogPanic(UTL_LOG_CONTEXT, "The configuration directory: '%s', does not exist", pucConfigurationDirectoryPath);
            }

            /* Check that the configuration directory path is a directory */
            if ( bUtlFileIsDirectory(pucConfigurationDirectoryPath) == false ) {
                vVersion();
                iUtlLogPanic(UTL_LOG_CONTEXT, "The configuration directory: '%s', is not a directory", pucConfigurationDirectoryPath);
            }

            /* Check that the configuration directory path can be accessed */
            if ( (bUtlFilePathRead(pucConfigurationDirectoryPath) == false) || (bUtlFilePathExec(pucConfigurationDirectoryPath) == false) ) {
                vVersion();
                iUtlLogPanic(UTL_LOG_CONTEXT, "The configuration directory: '%s', cannot be accessed", pucConfigurationDirectoryPath);
            }
        }

        /* Check for index directory */
        else if ( s_strncmp("--index-directory=", pucNextArgument, s_strlen("--index-directory=")) == 0 ) {

            /* Get the index directory path */
            pucNextArgument += s_strlen("--index-directory=");

            /* Get the true index directory path */
            if ( (iError = iUtlFileGetTruePath(pucNextArgument, pucIndexDirectoryPath, UTL_FILE_PATH_MAX + 1)) != UTL_NoError ) {
                vVersion();
                iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to get the true index directory path: '%s', utl error: %d", pucNextArgument, iError);
            }

            /* Check that index directory path exists */
            if ( bUtlFilePathExists(pucIndexDirectoryPath) == false ) {
                vVersion();
                iUtlLogPanic(UTL_LOG_CONTEXT, "The index directory: '%s', does not exist", pucIndexDirectoryPath);
            }

            /* Check that the index directory path is a directory */
            if ( bUtlFileIsDirectory(pucIndexDirectoryPath) == false ) {
                vVersion();
                iUtlLogPanic(UTL_LOG_CONTEXT, "The index directory: '%s', is not a directory", pucIndexDirectoryPath);
            }

            /* Check that the index directory path can be accessed */
            if ( (bUtlFilePathRead(pucIndexDirectoryPath) == false) || (bUtlFilePathExec(pucIndexDirectoryPath) == false) ) {
                vVersion();
                iUtlLogPanic(UTL_LOG_CONTEXT, "The index directory: '%s', cannot be accessed", pucIndexDirectoryPath);
            }
        }

        /* Check for index */
        else if ( s_strncmp("--index=", pucNextArgument, s_strlen("--index=")) == 0 ) {

            /* Get the index name */
            pucNextArgument += s_strlen("--index=");

            /* Set the index name */
            pucIndexName = pucNextArgument;
        }

        /* Check for terms */
        else if ( s_strcmp("--terms", pucNextArgument) == 0 ) {
            
            /* Set the list terms flag */
            bListTerms = true;
        }

        /* Check for stop terms */
        else if ( s_strcmp("--stopterms", pucNextArgument) == 0 ) {
            
            /* Set the list stop terms flag */
            bListStopTerms = true;
        }

        /* Check for all terms */
        else if ( s_strcmp("--allterms", pucNextArgument) == 0 ) {
            
            /* Set the list terms flag */
            bListTerms = true;

            /* Set the list stop terms flag */
            bListStopTerms = true;
        }

        /* Check for term */
        else if ( s_strncmp("--term=", pucNextArgument, s_strlen("--term=")) == 0 ) {

            /* Get the term */
            pucNextArgument += s_strlen("--term=");

            /* Set the term */
            pucTerm = pucNextArgument;

            /* Set the list term flag */
            bListTerm = true;
        }

        /* Check for index */
        else if ( s_strcmp("--index", pucNextArgument) == 0 ) {
            
            /* Set the list index flag */
            bListIndex = true;
        }

        /* Check for coverage */
        else if ( s_strcmp("--coverage", pucNextArgument) == 0 ) {
            
            /* Set the coverage flag */
            bListCoverage = true;
        }

        /* Check for field name */
        else if ( s_strncmp("--field=", pucNextArgument, s_strlen("--field=")) == 0 ) {

            /* Get the field name */
            pucNextArgument += s_strlen("--field=");

            /* Set the field name */
            pucFieldName = pucNextArgument;
        }

        /* Check for document key */
        else if ( s_strncmp("--document-key=", pucNextArgument, s_strlen("--document-key=")) == 0 ) {

            /* Get the document key */
            pucNextArgument += s_strlen("--document-key=");

            /* Set the document key */
            pucDocumentKey = pucNextArgument;
        }

        /* Check for document ID */
        else if ( s_strncmp("--document-id=", pucNextArgument, s_strlen("--document-id=")) == 0 ) {

            /* Get the document ID */
            pucNextArgument += s_strlen("--document-id=");

            /* Set the document ID */
            uiDocumentID = s_strtol(pucNextArgument, NULL, 10);

            /* Check the document ID */
            if ( uiDocumentID <= 0 ) {
                vVersion();
                iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to find a valid document ID: '%s'", pucNextArgument);
            }
        }

        /* Check for document overage */
        else if ( s_strncmp("--document-coverage=", pucNextArgument, s_strlen("--document-coverage=")) == 0 ) {

            /* Get the document coverage */
            pucNextArgument += s_strlen("--document-coverage=");
            
            /* Set the document coverage */
            fDocumentCoverage = s_strtof(pucNextArgument, NULL);
            
            /* Check the coverage */
            if ( (fDocumentCoverage < 0) || (fDocumentCoverage > 100) ) {
                vVersion();
                iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to find a valid coverage: '%s'", pucNextArgument);
            }
        }

        /* Check for frequent term coverage */
        else if ( s_strncmp("--frequent-term-coverage=", pucNextArgument, s_strlen("--frequent-term-coverage=")) == 0 ) {

            /* Get the frequent term coverage */
            pucNextArgument += s_strlen("--frequent-term-coverage=");

            /* Set the frequent term coverage */
            fFrequentTermCoverage = s_strtof(pucNextArgument, NULL);
            
            /* Check the frequent term coverage */
            if ( fFrequentTermCoverage < 0 ) {
                vVersion();
                iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to find a valid frequent term coverage: '%s'", pucNextArgument);
            }
        }

        /* Check for documents */
        else if ( s_strcmp("--documents", pucNextArgument) == 0 ) {
            
            /* Set the list documents flag */
            bListDocuments = true;
        }

        /* Check for document keys */
        else if ( s_strcmp("--document-keys", pucNextArgument) == 0 ) {

            /* Set the list document keys flag */
            bListDocumentKeys = true;
        }

        /* Check for statistics */
        else if ( (s_strcmp("--statistics", pucNextArgument) == 0) || (s_strcmp("--stats", pucNextArgument) == 0) ) {

            /* Set the list stats flag */
            bListStats = true;
        }

        /* Check for temporary index file */
        else if ( s_strncmp("--temporary-index-file=", pucNextArgument, s_strlen("--temporary-index-file=")) == 0 ) {

            /* Get the temporary index file path */
            pucNextArgument += s_strlen("--temporary-index-file=");

            /* Get the true temporary index file path */
            if ( (iError = iUtlFileGetTruePath(pucNextArgument, pucTemporaryIndexFilePath, UTL_FILE_PATH_MAX + 1)) != UTL_NoError ) {
                vVersion();
                iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to get the true temporary index file path: '%s', utl error: %d", pucNextArgument, iError);
            }

            /* Check that temporary index file path exists */
            if ( bUtlFilePathExists(pucTemporaryIndexFilePath) == false ) {
                vVersion();
                iUtlLogPanic(UTL_LOG_CONTEXT, "The temporary index file path: '%s', does not exist", pucTemporaryIndexFilePath);
            }

            /* Check that the temporary index file path is a file */
            if ( bUtlFileIsFile(pucTemporaryIndexFilePath) == false ) {
                vVersion();
                iUtlLogPanic(UTL_LOG_CONTEXT, "The temporary index file path: '%s', is not a file", pucTemporaryIndexFilePath);
            }

            /* Check that the temporary index file path can be accessed */
            if ( bUtlFilePathRead(pucTemporaryIndexFilePath) == false ) {
                vVersion();
                iUtlLogPanic(UTL_LOG_CONTEXT, "The temporary index file path: '%s', cannot be accessed", pucTemporaryIndexFilePath);
            }
        }

        /* Check for index block header */
        else if ( s_strcmp("--index-block-header", pucNextArgument) == 0 ) {

            /* Set the index block header flag */
            bListIndexBlockHeader = true;
        }

        /* Check for index block data */
        else if ( s_strcmp("--index-block-data", pucNextArgument) == 0 ) {

            /* Set the index block data flag */
            bListIndexBlockData = true;
        }

        /* Check for test */
        else if ( s_strcmp("--test", pucNextArgument) == 0 ) {

            /* Set the test flag */
            bTest = true;
        }

        /* Check for locale */
        else if ( s_strncmp("--locale=", pucNextArgument, s_strlen("--locale=")) == 0 ) {

            /* Get the locale */
            pucNextArgument += s_strlen("--locale=");

            /* Set the locale name */
            pucLocaleName = pucNextArgument;
        }    

        /* Check for log file */
        else if ( s_strncmp("--log=", pucNextArgument, s_strlen("--log=")) == 0 ) {

            /* Get the log file */
            pucNextArgument += s_strlen("--log=");

            /* Set the log file path */
            pucLogFilePath = pucNextArgument;
        }    

        /* Check for log level */
        else if ( s_strncmp("--level=", pucNextArgument, s_strlen("--level=")) == 0 ) {

            /* Get the log level */
            pucNextArgument += s_strlen("--level=");

            /* Check the log level */
            if ( (s_strtol(pucNextArgument, NULL, 10) < UTL_LOG_LEVEL_MINIMUM) || (s_strtol(pucNextArgument, NULL, 10) > UTL_LOG_LEVEL_MAXIMUM) ) {
                vVersion();
                iUtlLogPanic(UTL_LOG_CONTEXT, "Expected the log level to be greater than or equal to: %d, and less than or equal to: %d", UTL_LOG_LEVEL_MINIMUM, UTL_LOG_LEVEL_MAXIMUM);
            }
            
            /* Set the log level */
            uiLogLevel = s_strtol(pucNextArgument, NULL, 10);
        }

        /* Check for help */
        else if ( (s_strcmp("-?", pucNextArgument) == 0) || (s_strcmp("--help", pucNextArgument) == 0) || (s_strcmp("--usage", pucNextArgument) == 0) ) {
            vVersion();
            vUsage(pucCommandPath);
            s_exit(EXIT_SUCCESS);
        }

        /* Check for version */
        else if ( (s_strcmp("--version", pucNextArgument) == 0) ) {
            vVersion();
            s_exit(EXIT_SUCCESS);
        }

        /* Everything else */
        else {
            /* Cant recognize this argument */
            vVersion();
            iUtlLogPanic(UTL_LOG_CONTEXT, "Invalid option: '%s', try '-?', '--help' or '--usage' for more information", pucNextArgument);
        }

        /* Get the next argument */
        pucNextArgument = pucUtlArgsGetNextArg(&argc, &argv);
    }


    /* Install signal handlers */
    if ( (iError = iUtlSignalsInstallFatalHandler((void (*)())vUtlSignalsFatalHandler)) != UTL_NoError ) {
        vVersion();
        iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to install fatal signal handler, utl error: %d", iError);
    }

    if ( (iError = iUtlSignalsInstallNonFatalHandler((void (*)())vUtlSignalsNonFatalHandler)) != UTL_NoError ) {
        vVersion();
        iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to install non-fatal signal handler, utl error: %d", iError);
    }

    if ( (iError = iUtlSignalsInstallHangUpHandler(SIG_IGN)) != UTL_NoError ) {
        vVersion();
        iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to install hang-up signal handler, utl error: %d", iError);
    }

    if ( (iError = iUtlSignalsInstallChildHandler((void (*)())vUtlSignalsChildHandler)) != UTL_NoError ) {
        vVersion();
        iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to install child signal handler, utl error: %d", iError);
    }


    /* Set the log file path and the log level */
    if ( bUtlStringsIsStringNULL(pucLogFilePath) == false ) {
        if ( (iError = iUtlLogSetFilePath(pucLogFilePath)) != UTL_NoError ) {
            vVersion();
            iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to set the log file path, utl error: %d", iError);
        }
    }

    if ( (iError = iUtlLogSetLevel(uiLogLevel)) != UTL_NoError ) {
        vVersion();
        iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to set the log level, utl error: %d", iError);
    }


    /* Version message */
    vVersion();


    /* Check and set the locale, we require utf-8 compliance */
    if ( bLngLocationIsLocaleUTF8(LC_ALL, pucLocaleName) != true ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "The locale: '%s', does not appear to be utf-8 compliant", pucLocaleName);
    }
    if ( (iError = iLngLocationSetLocale(LC_ALL, pucLocaleName)) != LNG_NoError ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to set the locale to: '%s', lng error: %d", pucLocaleName, iError);
    }

    
    /* Check for index name, index directory and configuration directory paths if the temporary index file path is not provided */
    if ( bUtlStringsIsStringNULL(pucTemporaryIndexFilePath) == true ) { 

        /* Check for index name */
        if ( bUtlStringsIsStringNULL(pucIndexName) == true ) {
            iUtlLogPanic(UTL_LOG_CONTEXT, "An index name is required");
        }
    
        /* Check for index directory path */
        if ( bUtlStringsIsStringNULL(pucIndexDirectoryPath) == true ) {
            iUtlLogPanic(UTL_LOG_CONTEXT, "An index directory path is required");
        }
    
        /* Check for configuration directory path */
        if ( bUtlStringsIsStringNULL(pucConfigurationDirectoryPath) == true ) {
            iUtlLogPanic(UTL_LOG_CONTEXT, "A configuration directory path is required");
        }
    }


    /* Default to listing terms if we want the index */
/*     if ( (bListIndex == true) && (bListTerms == false) && (bListStopTerms == false) ) { */
/*         bListTerms = true; */
/*         bListStopTerms = false; */
/*     } */
    
    /* Default to listing terms if we specified no term, but specified a field name */
/*     if ( (bUtlStringsIsStringNULL(pucTerm) == true) && (bUtlStringsIsStringNULL(pucFieldName) == false) && (bListTerms == false) && (bListStopTerms == false) ) { */
/*         bListTerms = true; */
/*     } */

    /* Default to listing a term if we specify a term */
/*     if ( (bUtlStringsIsStringNULL(pucTerm) == false) && (bListTerms == false) && (bListStopTerms == false) ) { */
/*         bListTerm = true; */
/*     } */

    /* Default to listing document if we specified a document key/id and nothing else */
/*     if ( ((bUtlStringsIsStringNULL(pucDocumentKey) == false) || (uiDocumentID > 0)) && (bListDocuments == false) && (bListTerms == false) && (bListStopTerms == false) ) { */
/*         bListDocument = true; */
/*     } */
    
    /* Default to listing coverage if the document coverage or the frequent term coverage is set */
/*     if ( (fDocumentCoverage > 0) || (fFrequentTermCoverage > 0) ) { */
/*         bListCoverage = true; */
/*     } */
    
    /* Default to listing index block header if listing index block data is true */
/*     if ( bListIndexBlockData == true ) { */
/*         bListIndexBlockHeader = true; */
/*     } */


    /* Call the appropriate function */
    if ( (bListTerms == true) || (bListStopTerms == true) ) {
        vListTerms(pucIndexDirectoryPath, pucConfigurationDirectoryPath, pucIndexName, bListTerms, bListStopTerms, 
                bListIndex, bListCoverage, fDocumentCoverage, fFrequentTermCoverage, pucFieldName, pucDocumentKey, uiDocumentID);
    }

    else if ( bListTerm == true ) {
        vListTerm(pucIndexDirectoryPath, pucConfigurationDirectoryPath, pucIndexName, pucTerm, bListIndex, bListCoverage, 
                pucFieldName, pucDocumentKey, uiDocumentID);
    }

    else if ( bListDocuments == true ) {
        vListDocuments(pucIndexDirectoryPath, pucConfigurationDirectoryPath, pucIndexName);
    }

    else if ( (bUtlStringsIsStringNULL(pucDocumentKey) == false) || (uiDocumentID > 0) ) {
        vListDocument(pucIndexDirectoryPath, pucConfigurationDirectoryPath, pucIndexName, pucDocumentKey, uiDocumentID);
    }

    else if ( bListDocumentKeys == true ) {
        vListDocumentKeys(pucIndexDirectoryPath, pucConfigurationDirectoryPath, pucIndexName);
    }

    else if ( bListStats == true ) {
        vListStats(pucIndexDirectoryPath, pucConfigurationDirectoryPath, pucIndexName);
    }

    else if ( bUtlStringsIsStringNULL(pucTemporaryIndexFilePath) == false ) {
        vListTemporaryIndexFile(pucTemporaryIndexFilePath, bListIndexBlockHeader, bListIndexBlockData);
    }

    else if ( bTest == true ) {
        vTest();
    }


    printf("\n\nFinished...\n");


    return (EXIT_SUCCESS);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   vVersion()

    Purpose:    This function list out the version message.

    Parameters: none

    Globals:    none

    Returns:    void

*/
static void vVersion
(

)
{

    unsigned char   pucVersionString[UTL_VERSION_STRING_LENGTH + 1] = {'\0'};

    
    /* Copyright message */
    printf("Verify, %s\n", UTL_VERSION_COPYRIGHT_STRING);


    /* Get the version string */
    iUtlVersionGetVersionString(pucVersionString, UTL_VERSION_STRING_LENGTH + 1);

    /* Version message */
    printf("%s\n", pucVersionString);


    return;

}


/*---------------------------------------------------------------------------*/


/*

    Function:   vUsage()

    Purpose:    This function list out all the parameters that irverify
                takes.

    Parameters: pucCommandPath      command path

    Globals:    none

    Returns:    void

*/
static void vUsage
(
    unsigned char *pucCommandPath
)
{

    unsigned char   *pucCommandNamePtr = NULL;


    ASSERT(bUtlStringsIsStringNULL(pucCommandPath) == false);


    /* Get the command name */
    if ( iUtlFileGetPathBase(pucCommandPath, &pucCommandNamePtr) != UTL_NoError ) {
        pucCommandNamePtr = pucCommandPath;
    }


    /* Print out the usage */
    printf("\nUsage for: '%s'. \n", pucUtlStringsGetPrintableString(pucCommandNamePtr));
    printf("\n");

    printf(" General parameters: \n");

    printf("  --configuration-directory=name \n");
    printf("                  Configuration directory, required if index name provided. \n");
    printf("  --index-directory=name \n");
    printf("                  Index directory, required if index name provided. \n");
    printf("  --index=name \n");
    printf("                  Index name, required if the temporary index file is not provided. \n");
    printf("\n");

    printf("Term/index list: \n");
    printf("  --terms         List all regular terms. \n");
    printf("  --stopterms     List all stop terms. \n");
    printf("  --allterms      List all terms, same as '--terms --stopterms'. \n");
    printf("  --term=name     List a specific term \n");
    printf("\n");

    printf("Term/index list options: \n");
    printf("  --index         Include the index for all terms. \n");
    printf("  --coverage      Include the term/document coverage. \n");
    printf("\n");

    printf("Term/index list options/filters: \n");
    printf("  --field=name    Restrict to listing terms in the specified field. \n");
    printf("  --document-key=key \n");
    printf("                  Restrict to listing terms in the document specified by the document key. \n");
    printf("  --document-id=ID \n");
    printf("                  Restrict to listing terms in the document specified by the document ID. \n");
    printf("  --document-coverage=# \n");
    printf("                  Document coverage threshold, defaults to: %3.1f%%, \n", VRF_DOCUMENT_COVERAGE_DEFAULT);
    printf("                  range: 0-100, 0 = means not applied. \n");
    printf("  --frequent-term-coverage=# \n");
    printf("                  Frequent term coverage threshold, defaults to: %3.1f%%, \n", VRF_FREQUENT_TERM_COVERAGE_DEFAULT);
    printf("                  range: 0-.., 0 = means not applied. \n");
    printf("\n");

    printf("Document content/keys list: \n");
    printf("  --documents     List the document. \n");
    printf("  --document-key=key \n");
    printf("                  List the document specified by the document key. \n");
    printf("  --document-id=ID \n");
    printf("                  List the document specified by the document ID. \n");
    printf("  --document-keys List the document keys. \n");
    printf("\n");

    printf("Stats: \n");
    printf("  --statistics|--stats \n");
    printf("                  List dictionary and inverted index statistics. \n");
    printf("\n");

    printf("Temporary index file: \n");
    printf("  --temporary-index-file=name \n");
    printf("                  Temporary index file. \n");
    printf("  --index-block-header \n");
    printf("                  List the index block header. \n");
    printf("  --index-block-data \n");
    printf("                  List the index block data. \n");
    printf("\n");

    printf("Test: \n");
    printf("  --test           Run the test function. \n");
    printf("\n");

    printf(" Locale parameter: \n");
    printf("  --locale=name   Locale name, defaults to '%s', see 'locale' for list of \n", VRF_LOCALE_NAME_DEFAULT);
    printf("                  supported locales, locale chosen must support utf-8. \n");
    printf("\n");

    printf(" Logging parameters: \n");
    printf("  --log=name      Log output file name, defaults to 'stderr', console options: '%s', '%s'. \n", UTL_LOG_FILE_STDOUT, UTL_LOG_FILE_STDERR);
    printf("  --level=#       Log level, defaults to info, %d = debug, %d = info, %d = warn, %d = error, %d = fatal. \n",
            UTL_LOG_LEVEL_DEBUG, UTL_LOG_LEVEL_INFO, UTL_LOG_LEVEL_WARN, UTL_LOG_LEVEL_ERROR, UTL_LOG_LEVEL_FATAL);
    printf("\n");

    printf(" Help & version: \n");
    printf("  -?, --help, --usage \n");
    printf("                  Prints the usage and exits. \n");
    printf("  --version       Prints the version and exits. \n");
    printf("\n");


    return;

}


/*---------------------------------------------------------------------------*/


/*

    Function:   vListTerms()

    Purpose:    This function lists all the terms in the dictionary 
                and optionally all the postings for each term. This 
                implementation is different from the one in the search
                engine.

    Parameters: pucIndexDirectoryPath           index directory path
                pucConfigurationDirectoryPath   configuration directory path
                pucIndexName                    index name
                bListTerms                      true is the terms are to be listed
                bListStopTerms                  true if the stop terms are to be listed
                bListIndex                      true if the index blocks are to be listed
                bListCoverage                   true if the term/document coverage is to be listed
                fDocumentCoverage               document coverage (optional)
                fFrequentTermCoverage           frequent term coverage (optional)
                pucFieldName                    field name (optional)
                pucDocumentKey                  document key (optional)
                uiDocumentID                    document ID (optional)

    Globals:    none

    Returns:    void

*/
static void vListTerms
(
    unsigned char *pucIndexDirectoryPath,
    unsigned char *pucConfigurationDirectoryPath,
    unsigned char *pucIndexName,
    boolean bListTerms,
    boolean bListStopTerms, 
    boolean bListIndex,
    boolean bListCoverage, 
    float fDocumentCoverage, 
    float fFrequentTermCoverage,
    unsigned char *pucFieldName,
    unsigned char *pucDocumentKey,
    unsigned int uiDocumentID
)
{

    int                 iError = SRCH_NoError;
    struct srchIndex    *psiSrchIndex = NULL;
    unsigned int        uiFieldID = 0;


    ASSERT(bUtlStringsIsStringNULL(pucIndexDirectoryPath) == false);
    ASSERT(bUtlStringsIsStringNULL(pucConfigurationDirectoryPath) == false);
    ASSERT(bUtlStringsIsStringNULL(pucIndexName) == false);


    printf("Listing term dictionary for: '%s'", pucIndexName); 

    if ( bUtlStringsIsStringNULL(pucFieldName) == false ) {
        printf(", restricted to field: '%s'", pucFieldName);
    }

    if ( bUtlStringsIsStringNULL(pucDocumentKey) == false ) {
        printf(", restricted to document key: '%s'", pucDocumentKey);
    }

    printf(".\n");


    /* Open index */
    if ( (iError = iSrchIndexOpen(pucIndexDirectoryPath, pucConfigurationDirectoryPath, pucIndexName, SRCH_INDEX_INTENT_SEARCH, &psiSrchIndex)) != SRCH_NoError ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to open the index: '%s', srch error: %d.", pucIndexName, iError);
    }


    /* Get the field ID */
    if ( bUtlStringsIsStringNULL(pucFieldName) == false ) {
        if ( (iError = iSrchInfoGetFieldID(psiSrchIndex, pucFieldName, &uiFieldID)) != SRCH_NoError ) {
            iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to get the field ID, field name: '%s', index: '%s', srch error: %d", pucFieldName, pucIndexName, iError);
        }
    }

    
    /* Look up the document ID in the document key dictionary - overrides the uiDocumentID */
    if ( bUtlStringsIsStringNULL(pucDocumentKey) == false ) {
        if ( (iError = iSrchKeyDictLookup(psiSrchIndex, pucDocumentKey, &uiDocumentID)) != SRCH_NoError ) {
            iUtlLogPanic(UTL_LOG_CONTEXT, "Invalid document key: '%s', index: '%s', srch error: %d.", pucDocumentKey, psiSrchIndex->pucIndexName, iError);
        }
    }


    /* Print the header */
    if ( bListIndex == false ) {
        printf("\n");
        printf("Term                                            Term Count   Document Count");
        if ( bListCoverage == true ) {
            printf("     Coverage (Term/Document)");
        }
        printf("\n");
    }


    /* Process the term dictionary, start from the first key (promote float to double because of va_list requirements) */
    if ( (iError = iUtlDictProcessEntryList(psiSrchIndex->pvUtlTermDictionary, UTL_DICT_FIRST_KEY_FLAG, (int (*)())iListTermsCallBack, psiSrchIndex, 
            bListTerms, bListStopTerms, bListIndex, bListCoverage, (double)fDocumentCoverage, (double)fFrequentTermCoverage, uiFieldID, uiDocumentID)) != SRCH_NoError ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to process the term dictionary, index: '%s', srch error: %d.", pucIndexName, iError);
    }


    /* Close the index */
    iSrchIndexClose(psiSrchIndex);


    return;

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iListTermsCallBack()

    Purpose:    This function is passed to the dictionary lookup function to
                perform extra processing on a document key.

    Parameters: pucKey          key
                pvEntryData     entry data
                uiEntryLength   entry length
                ap              args (optional)

    Globals:    none

    Returns:    0 to continue processing, non-0 otherwise

*/
static int iListTermsCallBack
(
    unsigned char *pucKey,
    void *pvEntryData,
    unsigned int uiEntryLength,
    va_list ap
)
{

    va_list             ap_;
    struct srchIndex    *psiSrchIndex = NULL;
    boolean             bListTerms = false;
    boolean             bListStopTerms = false;
    boolean             bListIndex = false;
    boolean             bListCoverage = false;
    float               fDocumentCoverage = (float)0;
    float               fFrequentTermCoverage = (float)0;
    unsigned int        uiFieldID = 0;
    unsigned int        uiDocumentID = 0;

    unsigned char       *pucEntryData = NULL;
    unsigned char       *pucEntryDataPtr = NULL;

    unsigned int        uiTermType = 0;
    unsigned int        uiTermCount = 0;
    unsigned int        uiDocumentCount = 0;
    unsigned long       ulIndexBlockID = 0;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iListDocumentKeysCallBack [%s][%u][%s]", pucDocumentKey, uiEntryLength, */
/*             (pvEntryData != NULL) ? "(pvEntryData != NULL)" : "(pvEntryData == NULL)" ); */


    ASSERT(bUtlStringsIsStringNULL(pucKey) == false);
    ASSERT(pvEntryData != NULL);
    ASSERT(uiEntryLength > 0);


    /* Get all our parameters, note that we make a copy of 'ap' */
    va_copy(ap_, ap);
    psiSrchIndex = (struct srchIndex *)va_arg(ap_, struct srchIndex *);
    bListTerms = (boolean)va_arg(ap_, boolean);
    bListStopTerms = (boolean)va_arg(ap_, boolean);
    bListIndex = (boolean)va_arg(ap_, boolean);
    bListCoverage = (boolean)va_arg(ap_, boolean);
    fDocumentCoverage = (float)va_arg(ap_, double);
    fFrequentTermCoverage = (float)va_arg(ap_, double);
    uiFieldID = (unsigned int)va_arg(ap_, unsigned int);
    uiDocumentID = (unsigned int)va_arg(ap_, unsigned int);
    va_end(ap_);


    /* Set the entry data pointers */
    pucEntryData = (unsigned char *)pvEntryData;
    pucEntryDataPtr = pucEntryData;

    /* Extract some information from the entry */ 
    pucEntryDataPtr = (unsigned char *)pvEntryData;
    UTL_NUM_READ_COMPRESSED_UINT(uiTermType, pucEntryDataPtr);
    UTL_NUM_READ_COMPRESSED_UINT(uiTermCount, pucEntryDataPtr);
    UTL_NUM_READ_COMPRESSED_UINT(uiDocumentCount, pucEntryDataPtr);
    UTL_NUM_READ_COMPRESSED_ULONG(ulIndexBlockID, pucEntryDataPtr);


    /* Filter out regular terms if we are not listing them */
    if ( (bListTerms == false) && (uiTermType == SPI_TERM_TYPE_REGULAR) ) {
        return (0);
    }

    /* Filter out stop terms if we are not listing them */
    if ( (bListStopTerms == false) && (uiTermType == SPI_TERM_TYPE_STOP) ) {
        return (0);
    }


    /* Filter on the field ID */
    if ( uiFieldID != 0 ) {
    
        boolean         bListTerm = false;
        unsigned int    uiLocalFieldID = 0;
            
        while ( (pucEntryData + uiEntryLength) > pucEntryDataPtr ) {
            
            UTL_NUM_READ_COMPRESSED_UINT(uiLocalFieldID, pucEntryDataPtr);
            ASSERT(pucEntryDataPtr <= ((unsigned char *)pvEntryData + uiEntryLength));
            
            if ( uiLocalFieldID == uiFieldID ) {
                bListTerm = true;
                break;
            }
            else if ( uiLocalFieldID < uiFieldID ) {
                break;
            }
        }
        
        if ( bListTerm == false ) {
            return (0);
        }
    }



    /* Filter on the document ID */
    if ( uiDocumentID != 0 ) {
        
        int             iError = UTL_NoError;
        unsigned char   *pucIndexBlock = NULL;
        unsigned char   *pucIndexBlockPtr = NULL;
        unsigned int    uiIndexBlockLength = 0;
        unsigned int    uiIndexBlockHeaderLength = 0;
        unsigned int    uiIndexBlockDataLength = 0;

        unsigned int    uiIndexEntryDeltaDocumentID = 0;
        unsigned int    uiIndexEntryTermPosition = 0;
        unsigned int    uiIndexEntryFieldID = 0;
        unsigned int    uiIndexEntryDocumentID = 0;

        boolean         bListTerm = false;

        /* Get the index block data */
        if ( (iError = iUtlDataGetEntry(psiSrchIndex->pvUtlIndexData, ulIndexBlockID, (void **)&pucIndexBlock, &uiIndexBlockLength)) != UTL_NoError ) {
            iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to get index data, index object ID: %lu, index: '%s', utl error: %d.", 
                    ulIndexBlockID, psiSrchIndex->pucIndexName, iError);
        }

        /* Get the size of the index block */
        pucIndexBlockPtr = pucIndexBlock;
        UTL_NUM_READ_COMPRESSED_UINT(uiIndexBlockDataLength, pucIndexBlockPtr);
        ASSERT(uiIndexBlockDataLength < uiIndexBlockLength);
        uiIndexBlockHeaderLength = pucIndexBlockPtr - pucIndexBlock;


        /* Loop over the index block */
        while ( pucIndexBlockPtr < (pucIndexBlock + uiIndexBlockDataLength + uiIndexBlockHeaderLength) ) {

            /* Read the index entry */

            UTL_NUM_READ_COMPRESSED_UINT(uiIndexEntryDeltaDocumentID, pucIndexBlockPtr);
            ASSERT(pucIndexBlockPtr <= (pucIndexBlock + uiIndexBlockDataLength + uiIndexBlockHeaderLength));
            
            UTL_NUM_READ_COMPRESSED_UINT(uiIndexEntryTermPosition, pucIndexBlockPtr);
            ASSERT(pucIndexBlockPtr <= (pucIndexBlock + uiIndexBlockDataLength + uiIndexBlockHeaderLength));
            
            UTL_NUM_READ_COMPRESSED_UINT(uiIndexEntryFieldID, pucIndexBlockPtr);
            ASSERT(pucIndexBlockPtr <= (pucIndexBlock + uiIndexBlockDataLength + uiIndexBlockHeaderLength));

            /* Document IDs get stored as deltas (note that 0 is a valid delta) */
            uiIndexEntryDocumentID += uiIndexEntryDeltaDocumentID;

            /* Check the document ID */
            if ( uiDocumentID == uiIndexEntryDocumentID ) {
                bListTerm = true;
                break;
            }
            else if ( uiDocumentID < uiIndexEntryDocumentID ) {
                break;
            }
        }

        if ( bListTerm == false ) {
            return (0);
        }
    }


    /* Print the term information */
    if ( bListIndex == true ) {
        
        if ( (fDocumentCoverage > 0) && (fDocumentCoverage >= (((float)uiDocumentCount / psiSrchIndex->uiDocumentCount) * 100)) ) {
            return (0);
        }
        
        if ( (fFrequentTermCoverage > 0) && (fFrequentTermCoverage >= (((float)uiTermCount / psiSrchIndex->uiDocumentCount) * 100)) ) {
            return (0);
        }

        printf("Term: '%s', term count: %u, document count: %u", pucKey, uiTermCount, uiDocumentCount);
        
        if ( bListCoverage == true ) {
            printf(", coverage (term/document): %.2f%%/%.2f%%", (((float)uiTermCount / psiSrchIndex->uiDocumentCount) * 100),
                    (((float)uiDocumentCount / psiSrchIndex->uiDocumentCount) * 100));
        }
        
        if ( uiTermType == SPI_TERM_TYPE_STOP ) {
            printf("     (stop term)");
        }

        printf("\n");
        
        vListIndexBlock(psiSrchIndex, ulIndexBlockID, uiFieldID, uiDocumentID);
    }
    else {

        if ( (fDocumentCoverage > 0) && (fDocumentCoverage >= (((float)uiDocumentCount / psiSrchIndex->uiDocumentCount) * 100)) ) {
            return (0);
        }
        
        if ( (fFrequentTermCoverage > 0) && (fFrequentTermCoverage >= (((float)uiTermCount / psiSrchIndex->uiDocumentCount) * 100)) ) {
            return (0);
        }

        printf(" %-40s       %10u       %10u", pucKey, uiTermCount, uiDocumentCount);

        if ( (bListCoverage == true) && ((uiTermCount > 0) || (uiDocumentCount > 0)) ) {
                printf("     %6.2f%%/%6.2f%%", (((float)uiTermCount / psiSrchIndex->uiDocumentCount) * 100),
                        (((float)uiDocumentCount / psiSrchIndex->uiDocumentCount) * 100));
        }
        
        if ( (uiTermType == SPI_TERM_TYPE_STOP) && (bListTerms == true) ) { 
            printf("%s", (((bListCoverage == true) && ((uiTermCount > 0) || (uiDocumentCount > 0))) || (bListCoverage == false)) ? 
                    "     (stop term)" : "                         (stop term)");
        }

        printf("\n");
    }


    return (0);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   vListTerm()

    Purpose:    Dump the contents of the index for a term

    Parameters: pucIndexDirectoryPath           index directory path
                pucConfigurationDirectoryPath   configuration directory path
                pucIndexName                    index name
                pucTerm                         term
                bListIndex                      true if the index blocks are to be listed
                bListCoverage                   true if the term/document coverage is to be listed
                pucFieldName                    field name (optional)
                pucDocumentKey                  document key (optional)
                uiDocumentID                    document ID (optional)

    Globals:    none

    Returns:    void

*/
static void vListTerm
(
    unsigned char *pucIndexDirectoryPath,
    unsigned char *pucConfigurationDirectoryPath,
    unsigned char *pucIndexName,
    unsigned char *pucTerm,
    boolean bListIndex,
    boolean bListCoverage,
    unsigned char *pucFieldName,
    unsigned char *pucDocumentKey,
    unsigned int uiDocumentID
)
{

    int                 iError = SRCH_NoError;

    struct srchIndex    *psiSrchIndex = NULL;

    unsigned int        uiFieldID = 0;
    unsigned int        uiTermType = 0;
    unsigned int        uiTermCount = 0;
    unsigned int        uiDocumentCount = 0;
    unsigned long       ulIndexBlockID = 0;
    
    unsigned char       *pucFieldIDBitmap = NULL;
    unsigned long       uiFieldIDBitmapLength = 0;


    ASSERT(bUtlStringsIsStringNULL(pucIndexDirectoryPath) == false);
    ASSERT(bUtlStringsIsStringNULL(pucConfigurationDirectoryPath) == false);
    ASSERT(bUtlStringsIsStringNULL(pucIndexName) == false);
    ASSERT(bUtlStringsIsStringNULL(pucTerm) == false);


    printf("Listing term: '%s', in index: '%s'", pucTerm, pucIndexName); 

    if ( bUtlStringsIsStringNULL(pucFieldName) == false ) {
        printf(", restricted to field: '%s'", pucFieldName);
    }

    if ( bUtlStringsIsStringNULL(pucDocumentKey) == false ) {
        printf(", restricted to document key: '%s'", pucDocumentKey);
    }

    printf(".\n");


    /* Open the index */
    if ( (iError = iSrchIndexOpen(pucIndexDirectoryPath, pucConfigurationDirectoryPath, pucIndexName, SRCH_INDEX_INTENT_SEARCH, &psiSrchIndex)) != SRCH_NoError ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to open the index: '%s', srch error: %d.", pucIndexName, iError);
    }


    /* Get the field ID, only if there are any fields other than field ID 0 */
    if ( (bUtlStringsIsStringNULL(pucFieldName) == false) && (psiSrchIndex->uiFieldIDMaximum > 0) ) {
    
        /* Set the field bitmap length */
        uiFieldIDBitmapLength = psiSrchIndex->uiFieldIDMaximum;

        /* Allocate the field ID bitmap - field ID 0 is not a field */
        if ( (pucFieldIDBitmap = (unsigned char *)s_malloc(sizeof(unsigned char) * UTL_BITMAP_GET_BITMAP_BYTE_LENGTH(psiSrchIndex->uiFieldIDMaximum))) == NULL ) {
            iUtlLogPanic(UTL_LOG_CONTEXT, "Memory allocation error");
        }

        if ( (iError = iSrchInfoGetFieldID(psiSrchIndex, pucFieldName, &uiFieldID)) != SRCH_NoError ) {
            iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to get the field ID, field name: '%s', index: '%s', srch error: %d", pucFieldName, pucIndexName, iError);
        }
        
        /* Set field ID bitmap - field ID 0 is not a field */
        UTL_BITMAP_SET_BIT_IN_POINTER(pucFieldIDBitmap, uiFieldID - 1);
    }
    

    /* Look up the document ID in the document key dictionary - overrides the document ID */
    if ( bUtlStringsIsStringNULL(pucDocumentKey) == false ) {
        if ( (iError = iSrchKeyDictLookup(psiSrchIndex, pucDocumentKey, &uiDocumentID)) != SRCH_NoError ) {
            iUtlLogPanic(UTL_LOG_CONTEXT, "Invalid document key: '%s', index: '%s', srch error: %d.", pucDocumentKey, psiSrchIndex->pucIndexName, iError);
        }
    }


    /* Look up the term in the term dictionary */
    if ( (iError = iSrchTermDictLookup(psiSrchIndex, pucTerm, pucFieldIDBitmap, uiFieldIDBitmapLength, 
            &uiTermType, &uiTermCount, &uiDocumentCount, &ulIndexBlockID)) != SRCH_NoError ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to lookup the term, srch error: %d", iError);
    }


    /* Print out the term information */
    printf("Term                     : %s\n", pucTerm);
    printf("Term Type                : %u", uiTermType);
    
    if ( uiTermType == SPI_TERM_TYPE_REGULAR ) {
        printf(" (regular)");
    }
    else if ( uiTermType == SPI_TERM_TYPE_STOP ) {
        printf(" (stop)");
    }
    printf("\n");
    
    printf("Term Count               : %u\n", uiTermCount);
    printf("Document Count           : %u\n", uiDocumentCount);
    printf("Index Block ID           : %lu\n", ulIndexBlockID);

    
    if ( bListCoverage == true ) {
        printf("Coverage (term/document) : %.2f%%/%.2f%%\n", (((float)uiTermCount / psiSrchIndex->uiDocumentCount) * 100),
                    (((float)uiDocumentCount / psiSrchIndex->uiDocumentCount) * 100));
    }


    /* List the index block */
    if ( bListIndex == true ) {
        vListIndexBlock(psiSrchIndex, ulIndexBlockID, uiFieldID, uiDocumentID);
    }


    /* Free the field bitmap */
    s_free(pucFieldIDBitmap);

    
    /* Close the index */
    iSrchIndexClose(psiSrchIndex);


    return;

}


/*---------------------------------------------------------------------------*/


/*

    Function:   vListIndexBlock()

    Purpose:    This function lists the index block for a term, You need to pass
                the index block ID. This ID is obtained from the dictionary.

    Parameters: psiSrchIndex        index structure
                ulIndexBlockID      index block ID
                uiFieldID           field ID
                uiDocumentID        document ID

    Globals:    none

    Returns:    void

*/
static void vListIndexBlock
(
    struct srchIndex *psiSrchIndex,
    unsigned long ulIndexBlockID,
    unsigned int uiFieldID,
    unsigned int uiDocumentID
)
{

    int             iError = UTL_NoError;

    unsigned char   *pucIndexBlock = NULL;
    unsigned char   *pucIndexBlockPtr = NULL;
    unsigned int    uiIndexBlockLength = 0;
    unsigned int    uiIndexBlockHeaderLength = 0;
    unsigned int    uiIndexBlockDataLength = 0;

    unsigned int    uiIndexEntryDeltaDocumentID = 0;
    unsigned int    uiIndexEntryTermPosition = 0;
    unsigned int    uiIndexEntryFieldID = 0;
    unsigned int    uiIndexEntryDocumentID = 0;

    boolean         bFlag = false;


    ASSERT(psiSrchIndex != NULL);
    ASSERT(ulIndexBlockID >= 0);
    ASSERT(uiFieldID >= 0);
    ASSERT(uiDocumentID >= 0);


    /* Get the index block data */
    if ( (iError = iUtlDataGetEntry(psiSrchIndex->pvUtlIndexData, ulIndexBlockID, (void **)&pucIndexBlock, &uiIndexBlockLength)) != UTL_NoError ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to get index data, index object ID: %lu, index: '%s', utl error: %d.", 
                ulIndexBlockID, psiSrchIndex->pucIndexName, iError);
    }


    /* Get the size of the index block */
    pucIndexBlockPtr = pucIndexBlock;
    UTL_NUM_READ_COMPRESSED_UINT(uiIndexBlockDataLength, pucIndexBlockPtr);
    ASSERT(uiIndexBlockDataLength < uiIndexBlockLength);
    uiIndexBlockHeaderLength = pucIndexBlockPtr - pucIndexBlock;


    printf("  Index block ID: %lu, Size: %u\n", ulIndexBlockID, uiIndexBlockDataLength);

    /* Loop over the index block */
    while ( pucIndexBlockPtr < (pucIndexBlock + uiIndexBlockDataLength + uiIndexBlockHeaderLength) ) {

        /* Read the index entry */

        UTL_NUM_READ_COMPRESSED_UINT(uiIndexEntryDeltaDocumentID, pucIndexBlockPtr);
        ASSERT(pucIndexBlockPtr <= (pucIndexBlock + uiIndexBlockDataLength + uiIndexBlockHeaderLength));
        
        UTL_NUM_READ_COMPRESSED_UINT(uiIndexEntryTermPosition, pucIndexBlockPtr);
        ASSERT(pucIndexBlockPtr <= (pucIndexBlock + uiIndexBlockDataLength + uiIndexBlockHeaderLength));
        
        UTL_NUM_READ_COMPRESSED_UINT(uiIndexEntryFieldID, pucIndexBlockPtr);
        ASSERT(pucIndexBlockPtr <= (pucIndexBlock + uiIndexBlockDataLength + uiIndexBlockHeaderLength));

        /* Document IDs get stored as deltas (note that 0 is a valid delta) */
        uiIndexEntryDocumentID += uiIndexEntryDeltaDocumentID;
        
        /* Skip fields if we only want to list a specific field */
        if ( (uiFieldID != 0) && (uiFieldID != uiIndexEntryFieldID) ) {
            continue;
        }
        
        /* Skip documents if we only want to list a specific document */
        if ( (uiDocumentID != 0) && (uiDocumentID != uiIndexEntryDocumentID) ) {
            continue;
        }
        
        /* Print the header */
        if ( bFlag == false ) {
            printf("   Document ID      Position      Field ID\n");
            bFlag = true;    
        }
        
        printf("    %10u    %10u    %10u\n", uiIndexEntryDocumentID, uiIndexEntryTermPosition, uiIndexEntryFieldID);
    }
    
    printf("\n");


    return;

}


/*---------------------------------------------------------------------------*/


/*

    Function:   vListDocuments()

    Purpose:    List all the documents for this index

    Parameters: pucIndexDirectoryPath           index directory path
                pucConfigurationDirectoryPath   configuration directory path
                pucIndexName                    index name
                pucDocumentKey                  document key

    Globals:    none

    Returns:    void

*/
static void vListDocuments
(
    unsigned char *pucIndexDirectoryPath,
    unsigned char *pucConfigurationDirectoryPath,
    unsigned char *pucIndexName
)
{

    int                 iError = SRCH_NoError;
    struct srchIndex    *psiSrchIndex = NULL;
    unsigned int        uiDocumentID = 0;
    unsigned int        uiI = 0;


    ASSERT(bUtlStringsIsStringNULL(pucIndexDirectoryPath) == false);
    ASSERT(bUtlStringsIsStringNULL(pucConfigurationDirectoryPath) == false);
    ASSERT(bUtlStringsIsStringNULL(pucIndexName) == false);


    /* Open the index */
    if ( (iError = iSrchIndexOpen(pucIndexDirectoryPath, pucConfigurationDirectoryPath, pucIndexName, SRCH_INDEX_INTENT_SEARCH, &psiSrchIndex)) != SRCH_NoError ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to open the index: '%s', srch error: %d.", pucIndexName, iError);
    }


    /* Loop over all the documents */
    for ( uiDocumentID = 1; uiDocumentID <= psiSrchIndex->uiDocumentCount; uiDocumentID++ ) {
    
        unsigned char   *pucDocumentTableEntryData = NULL;
        unsigned char   *pucDocumentTableEntryDataPtr = NULL;
        unsigned long   ulDocumentDataID = 0;
        unsigned int    uiDocumentRank = 0;
        unsigned int    uiDocumentTermCount = 0;
        unsigned long   ulDocumentAnsiDate = 0;
        unsigned int    uiDocumentLanguageID = 0;

        unsigned char   *pucDocumentDataEntryData = NULL;
        unsigned char   *pucDocumentDataEntryDataPtr = NULL;
        unsigned int    uiDocumentDataEntryLength = 0;
        unsigned char   pucTitle[SPI_TITLE_MAXIMUM_LENGTH + 1] = {'\0'};
        unsigned char   pucDocumentKey[SPI_DOCUMENT_KEY_MAXIMUM_LENGTH + 1] = {'\0'};
        unsigned int    uiDocumentItemsCount = 0;

        /* Get the document table entry data */
        if ( (iError = iUtlTableGetEntry(psiSrchIndex->pvUtlDocumentTable, uiDocumentID, (void **)&pucDocumentTableEntryData)) != SRCH_NoError ) {
            iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to get the document table entry for document ID: '%u', srch error: %d.", uiDocumentID, iError);
        }        
        /* Set the document table entry data pointer */
        pucDocumentTableEntryDataPtr = pucDocumentTableEntryData;

        /* Decode the document table entry data */
        UTL_NUM_READ_ULONG(ulDocumentDataID, SRCH_DOCUMENT_DATA_ID_SIZE, pucDocumentTableEntryDataPtr);
        UTL_NUM_READ_UINT(uiDocumentRank, SRCH_DOCUMENT_RANK_SIZE, pucDocumentTableEntryDataPtr);
        UTL_NUM_READ_UINT(uiDocumentTermCount, SRCH_DOCUMENT_TERM_COUNT_SIZE, pucDocumentTableEntryDataPtr);
        UTL_NUM_READ_ULONG(ulDocumentAnsiDate, SRCH_DOCUMENT_DATE_SIZE, pucDocumentTableEntryDataPtr);
        UTL_NUM_READ_UINT(uiDocumentLanguageID, SRCH_DOCUMENT_LANGUAGE_ID_SIZE, pucDocumentTableEntryDataPtr);
    

        /* Get the document data entry */
        if ( (iError = iUtlDataGetEntry(psiSrchIndex->pvUtlDocumentData, ulDocumentDataID, (void **)&pucDocumentDataEntryData, &uiDocumentDataEntryLength)) != SRCH_NoError ) {
            iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to get the document data entry for document ID: '%u', document data ID: '%lu', srch error: %d.", 
                    uiDocumentID, ulDocumentDataID, iError);
        }

        /* Set the document data entry data pointer */
        pucDocumentDataEntryDataPtr = pucDocumentDataEntryData;

        /* Read in the title */
        s_strnncpy(pucTitle, pucDocumentDataEntryDataPtr, SPI_TITLE_MAXIMUM_LENGTH + 1);
        pucDocumentDataEntryDataPtr = (unsigned char *)s_strchr(pucDocumentDataEntryDataPtr, '\0') + 1;
        ASSERT(pucDocumentDataEntryDataPtr <= (pucDocumentDataEntryDataPtr + uiDocumentDataEntryLength));

        /* Read in the document key */
        s_strnncpy(pucDocumentKey, pucDocumentDataEntryDataPtr, SPI_DOCUMENT_KEY_MAXIMUM_LENGTH + 1);
        pucDocumentDataEntryDataPtr = (unsigned char *)s_strchr(pucDocumentDataEntryDataPtr, '\0') + 1;
        ASSERT(pucDocumentDataEntryDataPtr <= (pucDocumentDataEntryDataPtr + uiDocumentDataEntryLength));

        /* Read the number of document types */
        UTL_NUM_READ_COMPRESSED_UINT(uiDocumentItemsCount, pucDocumentDataEntryDataPtr);
        ASSERT(pucDocumentDataEntryDataPtr <= (pucDocumentDataEntryDataPtr + uiDocumentDataEntryLength));

        /* Print out the document entry information */
        printf("Document ID         : %u\n", uiDocumentID);
        printf("Title               : %s\n", pucTitle);
        printf("Document Key        : %s\n", pucDocumentKey);
        printf("Rank                : %u\n", uiDocumentRank);
        printf("Term Count          : %u\n", uiDocumentTermCount);
        printf("Ansi Date           : %lu\n", ulDocumentAnsiDate);
        printf("Language ID         : %u\n", uiDocumentLanguageID);
        printf("Item Count          : %u\n", uiDocumentItemsCount);

        /* Print the document items */
        for ( uiI = 0; uiI < uiDocumentItemsCount; uiI++ ) {

            unsigned int    uiItemID = 0;
            unsigned int    uiItemLength = 0;
            unsigned char   pucItemName[SPI_ITEM_NAME_MAXIMUM_LENGTH + 1] = {'\0'};
            unsigned char   pucMimeType[SPI_MIME_TYPE_MAXIMUM_LENGTH + 1] = {'\0'};
            unsigned char   pucUrl[SPI_URL_MAXIMUM_LENGTH + 1] = {'\0'};
            unsigned char   pucFilePath[UTL_FILE_PATH_MAX + 1] = {'\0'};
            off_t           zStartOffset = 0;
            off_t           zEndOffset = 0;
            unsigned int    uiDataLength = 0;
            void            *pvEntryData = NULL;

            /* Read the item ID from the buffer (increments the pointer) */
            UTL_NUM_READ_COMPRESSED_UINT(uiItemID, pucDocumentDataEntryDataPtr);
            ASSERT(pucDocumentDataEntryDataPtr <= (pucDocumentDataEntryDataPtr + uiDocumentDataEntryLength));

            /* Read the item length from the buffer (increments the pointer) */
            UTL_NUM_READ_COMPRESSED_UINT(uiItemLength, pucDocumentDataEntryDataPtr);
            ASSERT(pucDocumentDataEntryDataPtr <= (pucDocumentDataEntryDataPtr + uiDocumentDataEntryLength));

            /* Read in the URL (incrementing the pointer) */
            s_strnncpy(pucUrl, pucDocumentDataEntryDataPtr, SPI_URL_MAXIMUM_LENGTH + 1);
            pucDocumentDataEntryDataPtr = (unsigned char *)s_strchr(pucDocumentDataEntryDataPtr, '\0') + 1;
            ASSERT(pucDocumentDataEntryDataPtr <= (pucDocumentDataEntryDataPtr + uiDocumentDataEntryLength));

            /* Read in the file path (incrementing the pointer) */
            s_strnncpy(pucFilePath, pucDocumentDataEntryDataPtr, UTL_FILE_PATH_MAX + 1);
            pucDocumentDataEntryDataPtr = (unsigned char *)s_strchr(pucDocumentDataEntryDataPtr, '\0') + 1;
            ASSERT(pucDocumentDataEntryDataPtr <= (pucDocumentDataEntryDataPtr + uiDocumentDataEntryLength));

            /* Read the start index from the buffer (increments the pointer) */
            UTL_NUM_READ_COMPRESSED_OFF_T(zStartOffset, pucDocumentDataEntryDataPtr);
            ASSERT(pucDocumentDataEntryDataPtr <= (pucDocumentDataEntryDataPtr + uiDocumentDataEntryLength));

            /* Read the end index from the buffer (increments the pointer) */
            UTL_NUM_READ_COMPRESSED_OFF_T(zEndOffset, pucDocumentDataEntryDataPtr);
            ASSERT(pucDocumentDataEntryDataPtr <= (pucDocumentDataEntryDataPtr + uiDocumentDataEntryLength));

            /* Read the data length from the buffer (increments the pointer) */
            UTL_NUM_READ_COMPRESSED_UINT(uiDataLength, pucDocumentDataEntryDataPtr);
            ASSERT(pucDocumentDataEntryDataPtr <= (pucDocumentDataEntryDataPtr + uiDocumentDataEntryLength));

            /* Get a pointer to the data in the buffer */
            pvEntryData = pucDocumentDataEntryDataPtr;

            /* Increment the pointer to the end of the data */
            pucDocumentDataEntryDataPtr += uiDataLength;
            ASSERT(pucDocumentDataEntryDataPtr <= (pucDocumentDataEntryDataPtr + uiDocumentDataEntryLength));

            /* Get the item info for that item id */
            if ( (iError = iSrchInfoGetItemInfo(psiSrchIndex, uiItemID, pucItemName, SPI_ITEM_NAME_MAXIMUM_LENGTH + 1, pucMimeType, SPI_MIME_TYPE_MAXIMUM_LENGTH + 1)) != SRCH_NoError ) {
                iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to get the item info, item ID: %u, index: '%s', srch error: %d", uiItemID, psiSrchIndex->pucIndexName, iError);
            }

            printf("\n");
            printf("  Item Name          : %s\n", pucItemName);
            printf("  Mime Type          : %s\n", pucMimeType);
            printf("  Item Length        : %u\n", uiItemLength);
            printf("  URL                : %s\n" , pucUtlStringsGetPrintableString(pucUrl));
            printf("  File & Offsets     : %s %ld %ld\n", pucUtlStringsGetPrintableString(pucFilePath), (long)zStartOffset, (long)zEndOffset);
            printf("  Data Length        : %u\n", uiDataLength);

            /* Print the data if there is any, but only if it is text based */
            if ( uiDataLength > 0 ) {

                if ( s_strncmp(pucMimeType, "text/", 5) == 0 ) {            

                    /* NULL terminate the data */
                    if ( (iError = iUtlStringsNullTerminateData(pvEntryData, uiDataLength, (unsigned char **)&pvEntryData)) != UTL_NoError ) {
                        iUtlLogError(UTL_LOG_CONTEXT, "Failed to NULL terminate data, utl error: %d.", iError);
                    }

                    printf("  Data               : %s\n", (unsigned char *)pvEntryData);
                    
                    /* Free the data */
                    s_free(pvEntryData);
                }
                else {
                    printf("  Data               : (binary data)\n");
                }
            }
        }

        printf("\n\n");
    }


    /* Close the index */
    iSrchIndexClose(psiSrchIndex);

    return;

}


/*---------------------------------------------------------------------------*/


/*

    Function:   vListDocument()

    Purpose:    Dump the contents of the document for the document key
                or document ID

    Parameters: pucIndexDirectoryPath           index directory path
                pucConfigurationDirectoryPath   configuration directory path
                pucIndexName                    index name
                pucDocumentKey                  document key (optional)
                uiDocumentID                    document ID (optional)

    Globals:    none

    Returns:    void

*/
static void vListDocument
(
    unsigned char *pucIndexDirectoryPath,
    unsigned char *pucConfigurationDirectoryPath,
    unsigned char *pucIndexName,
    unsigned char *pucDocumentKey,
    unsigned int uiDocumentID
)
{

    int                         iError = SRCH_NoError;
    unsigned int                uiI = 0;

    struct srchIndex            *psiSrchIndex = NULL;

    unsigned char               pucTitle[SPI_TITLE_MAXIMUM_LENGTH + 1] = {'\0'};
    unsigned char               *pucDocumentTitlePtr = pucTitle;
    unsigned char               pucLocalDocumentKey[SPI_DOCUMENT_KEY_MAXIMUM_LENGTH + 1] = {'\0'};
    unsigned char               *pucLocalDocumentKeyPtr = pucLocalDocumentKey;
    unsigned int                uiDocumentRank = 0;
    unsigned int                uiDocumentTermCount = 0;
    unsigned long               ulDocumentAnsiDate = 0;
    unsigned int                uiDocumentLanguageID = 0;
    struct srchDocumentItem     *psdiSrchDocumentItems = NULL;
    unsigned int                uiDocumentItemsCount = 0;


    ASSERT(bUtlStringsIsStringNULL(pucIndexDirectoryPath) == false);
    ASSERT(bUtlStringsIsStringNULL(pucConfigurationDirectoryPath) == false);
    ASSERT(bUtlStringsIsStringNULL(pucIndexName) == false);


    /* Print the header */
    if ( bUtlStringsIsStringNULL(pucDocumentKey) == false ) {
        printf("Listing document structure content for document key: '%s', in index: '%s'.\n", pucDocumentKey, pucIndexName); 
    }
    else if ( uiDocumentID > 0 ) {
        printf("Listing document structure content for document ID: %d, in index: '%s'.\n", uiDocumentID, pucIndexName); 
    }


    /* Open the index */
    if ( (iError = iSrchIndexOpen(pucIndexDirectoryPath, pucConfigurationDirectoryPath, pucIndexName, SRCH_INDEX_INTENT_SEARCH, &psiSrchIndex)) != SRCH_NoError ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to open the index: '%s', srch error: %d.", pucIndexName, iError);
    }



    /* Look up the document key in the document key dictionary - overrides the document ID */
    if ( bUtlStringsIsStringNULL(pucDocumentKey) == false ) {
        if ( (iError = iSrchKeyDictLookup(psiSrchIndex, pucDocumentKey, &uiDocumentID)) != SRCH_NoError ) {
            iUtlLogPanic(UTL_LOG_CONTEXT, "Invalid document key: '%s', index: '%s', srch error: %d.", pucDocumentKey, psiSrchIndex->pucIndexName, iError);
        }
    }


    /* Get the document information */
    if ( (iError = iSrchDocumentGetDocumentInfo(psiSrchIndex, uiDocumentID, &pucDocumentTitlePtr, &pucLocalDocumentKeyPtr, &uiDocumentRank, &uiDocumentTermCount, 
            &ulDocumentAnsiDate, &uiDocumentLanguageID, &psdiSrchDocumentItems, &uiDocumentItemsCount, 0, true, true, true)) != SRCH_NoError) {

        iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to get the document information for document ID: %u, index: '%s', srch error: %d.", 
                uiDocumentID, psiSrchIndex->pucIndexName, iError);
    }


    /* Print out the document entry information */
    printf("Document ID         : %u\n", uiDocumentID);
    printf("Title               : %s\n", pucTitle);
    printf("Document Key        : %s\n", pucLocalDocumentKey);
    printf("Rank                : %u\n", uiDocumentRank);
    printf("Term Count          : %u\n", uiDocumentTermCount);
    printf("Ansi Date           : %lu\n", ulDocumentAnsiDate);
    printf("Language ID         : %u\n", uiDocumentLanguageID);
    printf("# of Types          : %u\n", uiDocumentItemsCount);


    for ( uiI = 0; uiI < uiDocumentItemsCount; uiI++ ) {

        unsigned char   pucItemName[SPI_ITEM_NAME_MAXIMUM_LENGTH + 1] = {'\0'};
        unsigned char   pucMimeType[SPI_MIME_TYPE_MAXIMUM_LENGTH + 1] = {'\0'};

        /* Get the item info for this item ID */
        if ( (iError = iSrchInfoGetItemInfo(psiSrchIndex, (psdiSrchDocumentItems + uiI)->uiItemID, pucItemName, SPI_ITEM_NAME_MAXIMUM_LENGTH + 1, 
                pucMimeType, SPI_MIME_TYPE_MAXIMUM_LENGTH + 1)) != SRCH_NoError ) {
            iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to get the item info, item ID: %u, index: '%s', srch error: %d", 
                    (psdiSrchDocumentItems + uiI)->uiItemID, psiSrchIndex->pucIndexName, iError);
        }

        printf("\n");
        printf("  Item ID            : %u\n", (psdiSrchDocumentItems + uiI)->uiItemID);
        printf("  Item Name          : %s\n", pucItemName);
        printf("  Mime Type          : %s\n", pucMimeType);
        printf("  Item Length        : %u\n", (psdiSrchDocumentItems + uiI)->uiItemLength);
        printf("  URL                : %s\n" , pucUtlStringsGetPrintableString((psdiSrchDocumentItems + uiI)->pucUrl));
        printf("  File & offsets     : %s %ld %ld\n", pucUtlStringsGetPrintableString((psdiSrchDocumentItems + uiI)->pucFilePath), 
                (long)(psdiSrchDocumentItems + uiI)->zStartOffset, (long)(psdiSrchDocumentItems + uiI)->zEndOffset);
        printf("  Data Length        : %u\n", (psdiSrchDocumentItems + uiI)->uiDataLength);

        /* Print the data if there is any, but only if it is text based */
        if ( (psdiSrchDocumentItems + uiI)->uiDataLength > 0 ) {
            if ( s_strncmp(pucMimeType, "text/", 5) == 0 ) {            
                printf("  Data               : %s\n", (unsigned char *)(psdiSrchDocumentItems + uiI)->pvData);
                printf("\n");
            }
            else {
                printf("  Data               : (binary data)\n");
            }
        }
    }

    /* Free the type information */
    s_free(psdiSrchDocumentItems);


    /* Close the index */
    iSrchIndexClose(psiSrchIndex);


    return;

}


/*---------------------------------------------------------------------------*/


/*

    Function:   vListDocumentKeys()

    Purpose:    This function lists all the entries in the document keys dictionary.
                This includes the document keys and the internal document IDs.

    Parameters: pucIndexDirectoryPath            index directory path
                pucConfigurationDirectoryPath    configuration directory path
                pucIndexName                    index name

    Globals:    none

    Returns:    void

*/
static void vListDocumentKeys
(
    unsigned char *pucIndexDirectoryPath,
    unsigned char *pucConfigurationDirectoryPath,
    unsigned char *pucIndexName
)
{


    int                 iError = SRCH_NoError;
    struct srchIndex    *psiSrchIndex = NULL;


    ASSERT(bUtlStringsIsStringNULL(pucIndexDirectoryPath) == false);
    ASSERT(bUtlStringsIsStringNULL(pucConfigurationDirectoryPath) == false);
    ASSERT(bUtlStringsIsStringNULL(pucIndexName) == false);


    printf("Listing document key dictionary for: '%s'.\n", pucIndexName); 


    /* Open index */
    if ( (iError = iSrchIndexOpen(pucIndexDirectoryPath, pucConfigurationDirectoryPath, pucIndexName, SRCH_INDEX_INTENT_SEARCH, &psiSrchIndex)) != SRCH_NoError ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to open the index: '%s', srch error: %d.", pucIndexName, iError);
    }


    /* Print the report header */    
    printf("Document ID   Document Key\n"); 


    /* Process the document key dictionary, start from the first key */
    if ( (iError = iUtlDictProcessEntryList(psiSrchIndex->pvUtlKeyDictionary, UTL_DICT_FIRST_KEY_FLAG, (int (*)())iListDocumentKeysCallBack)) != SRCH_NoError ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to process the document key dictionary, index: '%s', srch error: %d.", pucIndexName, iError);
    }


    /* Close the index */
    iSrchIndexClose(psiSrchIndex);


    return;

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iListDocumentKeysCallBack()

    Purpose:    This function is passed to the dictionary lookup function to
                perform extra processing on a key.

    Parameters: pucKey              key
                pvEntryData         entry data
                uiEntryLength       entry length
                ap                  args (optional)

    Globals:    none

    Returns:    0 to continue processing, non-0 otherwise

*/
static int iListDocumentKeysCallBack
(
    unsigned char *pucKey,
    void *pvEntryData,
    unsigned int uiEntryLength,
    va_list ap
)
{

    va_list         ap_;
    unsigned char   *pucEntryDataPtr = NULL;
    unsigned int    uiDocumentID = 0;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iListDocumentKeysCallBack [%s][%u][%s]", pucKey, uiEntryLength, */
/*             (pvEntryData != NULL) ? "(pvEntryData != NULL)" : "(pvEntryData == NULL)" ); */


    ASSERT(bUtlStringsIsStringNULL(pucKey) == false);
    ASSERT(pvEntryData != NULL);
    ASSERT(uiEntryLength > 0);


    /* Get all our parameters, note that we make a copy of 'ap' */
    va_copy(ap_, ap);
    va_end(ap_);


    /* Decode the buffer to get the document ID, decode to a variable and copy to the pointer for optimization */
    pucEntryDataPtr = (unsigned char *)pvEntryData;
    UTL_NUM_READ_COMPRESSED_UINT(uiDocumentID, pucEntryDataPtr);

    printf(" %10u     %s\n", uiDocumentID, pucKey);


    return (0);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   vListStats()

    Purpose:    This function lists stats from the index.

    Parameters: pucIndexDirectoryPath           index directory path
                pucConfigurationDirectoryPath   configuration directory path
                pucIndexName                    index name

    Globals:    none

    Returns:    void

*/
static void vListStats
(
    unsigned char *pucIndexDirectoryPath,
    unsigned char *pucConfigurationDirectoryPath,
    unsigned char *pucIndexName
)
{

    int                 iError = SRCH_NoError;
    struct srchIndex    *psiSrchIndex = NULL;

    unsigned int        uiTermTotalLength = 0;
    unsigned long       ulUniqueTermCount = 0;
    unsigned long       ulTotalTermCount = 0;
    unsigned long       ulUniqueStopTermCount = 0;
    unsigned long       ulTotalStopTermCount = 0;
    unsigned int        uiIndexBlockCount = 0;
    unsigned int        uiIndexBlockTotalLength = 0;
    unsigned int        uiIndexBlockMaxLength = 0;
    unsigned int        uiIndexBlockMinLength = UINT_MAX;            /* Prime this with a really large number */

    unsigned int        uiI = 0;

    unsigned int        uiDocumentID = 0;
    unsigned long       uiDocumentCount = 0;
    unsigned int        uiDocumentTermCountMaximum = 0;
    unsigned int        uiDocumentTermCountMinimum = UINT_MAX;        /* Prime this with a really large number */

    unsigned int        puiIndexBlockLengthArray[VRF_INDEX_BLOCK_LEN_TO_CHECK_MAX];
    unsigned int        uiIndexBlockMedian = 0;

    unsigned int        *puiTermCounts[VRF_TERM_COUNT_TO_CHECK_MAX];
    unsigned int        uiTermCountMedian = 0;
    unsigned int        uiTermCountMedianNonZero = 0;

    unsigned char       pucPrintableNumber1[UTL_FILE_PATH_MAX + 1] = {'\0'};
    unsigned char       pucPrintableNumber2[UTL_FILE_PATH_MAX + 1] = {'\0'};


    ASSERT(bUtlStringsIsStringNULL(pucIndexDirectoryPath) == false);
    ASSERT(bUtlStringsIsStringNULL(pucConfigurationDirectoryPath) == false);
    ASSERT(bUtlStringsIsStringNULL(pucIndexName) == false);


    printf("Listing stats for: '%s'.\n", pucIndexName); 


    /* Erase the postings median array  */
    s_memset(puiIndexBlockLengthArray, 0, (sizeof(unsigned int) * VRF_INDEX_BLOCK_LEN_TO_CHECK_MAX));

    /* Erase the term count median array  */
    s_memset(puiTermCounts, 0, (sizeof(unsigned int) * VRF_TERM_COUNT_TO_CHECK_MAX));



    /* Open index */
    if ( (iError = iSrchIndexOpen(pucIndexDirectoryPath, pucConfigurationDirectoryPath, pucIndexName, SRCH_INDEX_INTENT_SEARCH, &psiSrchIndex)) != SRCH_NoError ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to open the index: '%s', srch error: %d.", pucIndexName, iError);
    }


    /* Process the term dictionary, start from the first key */
    if ( (iError = iUtlDictProcessEntryList(psiSrchIndex->pvUtlTermDictionary, UTL_DICT_FIRST_KEY_FLAG, (int (*)())iListStatsCallBack, psiSrchIndex, 
            &uiTermTotalLength, &ulUniqueTermCount, &ulTotalTermCount, &ulUniqueStopTermCount, &ulTotalStopTermCount, 
            &uiIndexBlockCount, &uiIndexBlockTotalLength, &uiIndexBlockMaxLength, &uiIndexBlockMinLength, puiIndexBlockLengthArray)) != SRCH_NoError ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to process the term dictionary, index: '%s', srch error: %d.", pucIndexName, iError);
    }


    /* Loop over all the documents until we hit an error */
    for ( uiDocumentID = 1; ; uiDocumentID++ ) {
    
        unsigned char   *pucDocumentTableEntryData = NULL;
        unsigned char   *pucDocumentTableEntryDataPtr = NULL;
        unsigned long   ulDocumentDataID = 0;
        unsigned int    uiDocumentRank = 0;
        unsigned int    uiDocumentTermCount = 0;
        unsigned long   ulDocumentAnsiDate = 0;
        unsigned int    uiDocumentLanguageID = 0;
    
        /* Get the document table entry data */
        if ( (iError = iUtlTableGetEntry(psiSrchIndex->pvUtlDocumentTable, uiDocumentID, (void **)&pucDocumentTableEntryData)) != SRCH_NoError ) {
/*             iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to get the document table entry for document ID: '%u', srch error: %d.", uiDocumentCount, iError); */
            break;
        }
        
        /* Set the document table entry data pointer */
        pucDocumentTableEntryDataPtr = pucDocumentTableEntryData;

        /* Decode the document table entry data */
        UTL_NUM_READ_ULONG(ulDocumentDataID, SRCH_DOCUMENT_DATA_ID_SIZE, pucDocumentTableEntryDataPtr);
        UTL_NUM_READ_UINT(uiDocumentRank, SRCH_DOCUMENT_RANK_SIZE, pucDocumentTableEntryDataPtr);
        UTL_NUM_READ_UINT(uiDocumentTermCount, SRCH_DOCUMENT_TERM_COUNT_SIZE, pucDocumentTableEntryDataPtr);
        UTL_NUM_READ_ULONG(ulDocumentAnsiDate, SRCH_DOCUMENT_DATE_SIZE, pucDocumentTableEntryDataPtr);
        UTL_NUM_READ_UINT(uiDocumentLanguageID, SRCH_DOCUMENT_LANGUAGE_ID_SIZE, pucDocumentTableEntryDataPtr);
    
        /* Set the actual max and min document term counts */
        uiDocumentTermCountMaximum = UTL_MACROS_MAX(uiDocumentTermCount, uiDocumentTermCountMaximum);
        uiDocumentTermCountMinimum = UTL_MACROS_MIN(uiDocumentTermCount, uiDocumentTermCountMinimum);
            
        if ( uiDocumentTermCount < VRF_TERM_COUNT_TO_CHECK_MAX ) {
            puiTermCounts[uiDocumentTermCount]++;
        }
        
        /* Set the document count */
        uiDocumentCount = uiDocumentID;
    }


    /* Loop through looking for the median postings length */
    for ( uiI = 0, uiIndexBlockMedian = 0; uiI < VRF_INDEX_BLOCK_LEN_TO_CHECK_MAX; uiI++ ) {
        if ( puiIndexBlockLengthArray[uiI] > puiIndexBlockLengthArray[uiIndexBlockMedian] ) {
            uiIndexBlockMedian = uiI;
        }
    }

    /* Loop through looking for the median term count */
    for ( uiI = 0, uiTermCountMedian = 0; uiI < VRF_TERM_COUNT_TO_CHECK_MAX; uiI++ ) {
        if ( puiTermCounts[uiI] > puiTermCounts[uiTermCountMedian] ) {
            uiTermCountMedian = uiI;
        }
    }
    if ( uiTermCountMedian == 0 ) {
        for ( uiI = 1, uiTermCountMedianNonZero = 1; uiI < VRF_TERM_COUNT_TO_CHECK_MAX; uiI++ ) {
            if ( puiTermCounts[uiI] > puiTermCounts[uiTermCountMedianNonZero] ) {
                uiTermCountMedianNonZero = uiI;
            }
        }
    }


    /* Print out the stats */
    printf("\n\nCounts:                                  Stated                    Actual\n\n");
    printf("Documents:                       %15s          %15s  %s\n", 
            pucUtlStringsFormatUnsignedNumber(psiSrchIndex->uiDocumentCount, pucPrintableNumber1, UTL_FILE_PATH_MAX + 1),
            pucUtlStringsFormatUnsignedNumber(uiDocumentCount, pucPrintableNumber2, UTL_FILE_PATH_MAX + 1),
            (psiSrchIndex->uiDocumentCount != uiDocumentCount) ? "<=" : "");
    printf("Total terms:                     %15s          %15s  %s\n", 
            pucUtlStringsFormatUnsignedNumber(psiSrchIndex->ulTotalTermCount, pucPrintableNumber1, UTL_FILE_PATH_MAX + 1),
            pucUtlStringsFormatUnsignedNumber(ulTotalTermCount, pucPrintableNumber2, UTL_FILE_PATH_MAX + 1),
            (psiSrchIndex->ulTotalTermCount != ulTotalTermCount) ? "<=" : "");
    printf("Unique terms:                    %15s          %15s  %s\n", 
            pucUtlStringsFormatUnsignedNumber(psiSrchIndex->ulUniqueTermCount, pucPrintableNumber1, UTL_FILE_PATH_MAX + 1),
            pucUtlStringsFormatUnsignedNumber(ulUniqueTermCount, pucPrintableNumber2, UTL_FILE_PATH_MAX + 1),
            (psiSrchIndex->ulUniqueTermCount != ulUniqueTermCount) ? "<=" : "");
    printf("Total stop terms:                %15s          %15s  %s\n", 
            pucUtlStringsFormatUnsignedNumber(psiSrchIndex->ulTotalStopTermCount, pucPrintableNumber1, UTL_FILE_PATH_MAX + 1),
            pucUtlStringsFormatUnsignedNumber(ulTotalStopTermCount, pucPrintableNumber2, UTL_FILE_PATH_MAX + 1),
            (psiSrchIndex->ulTotalStopTermCount != ulTotalStopTermCount) ? "<=" : "");
    printf("Unique stop terms:               %15s          %15s  %s\n", 
            pucUtlStringsFormatUnsignedNumber(psiSrchIndex->ulUniqueStopTermCount, pucPrintableNumber1, UTL_FILE_PATH_MAX + 1),
            pucUtlStringsFormatUnsignedNumber(ulUniqueStopTermCount, pucPrintableNumber2, UTL_FILE_PATH_MAX + 1),
            (psiSrchIndex->ulUniqueStopTermCount != ulUniqueStopTermCount) ? "<=" : "");
    printf("Max document terms:              %15s          %15s  %s\n", 
            pucUtlStringsFormatUnsignedNumber(psiSrchIndex->uiDocumentTermCountMaximum, pucPrintableNumber1, UTL_FILE_PATH_MAX + 1),
            pucUtlStringsFormatUnsignedNumber(uiDocumentTermCountMaximum, pucPrintableNumber2, UTL_FILE_PATH_MAX + 1),
            (psiSrchIndex->uiDocumentTermCountMaximum != uiDocumentTermCountMaximum) ? "<=" : "");
    printf("Min document terms:              %15s          %15s  %s\n", 
            pucUtlStringsFormatUnsignedNumber(psiSrchIndex->uiDocumentTermCountMinimum, pucPrintableNumber1, UTL_FILE_PATH_MAX + 1),
            pucUtlStringsFormatUnsignedNumber(uiDocumentTermCountMinimum, pucPrintableNumber2, UTL_FILE_PATH_MAX + 1),
            (psiSrchIndex->uiDocumentTermCountMinimum != uiDocumentTermCountMinimum) ? "<=" : "");
    printf("Average document terms:          %15.2f\n", (double)psiSrchIndex->ulTotalTermCount / psiSrchIndex->uiDocumentCount);
    printf("Median document terms:           %15s\n", pucUtlStringsFormatUnsignedNumber(uiTermCountMedian, pucPrintableNumber1, UTL_FILE_PATH_MAX + 1));
    if ( uiTermCountMedian == 0 ) {
        printf("                    (non-zero):  %15s\n", pucUtlStringsFormatUnsignedNumber(uiTermCountMedianNonZero, pucPrintableNumber1, UTL_FILE_PATH_MAX + 1));
    }

    printf("\n\nLengths:\n\n");
    printf("Total terms length:              %15s bytes\n", pucUtlStringsFormatUnsignedNumber(uiTermTotalLength, pucPrintableNumber1, UTL_FILE_PATH_MAX + 1));
    printf("Average term length:             %15.2f bytes\n", (double)uiTermTotalLength / psiSrchIndex->ulUniqueTermCount);
/*     printf("Total term snippets length:      %15s bytes\n", pucUtlStringsFormatUnsignedNumber(uiTermSnippetLength, pucPrintableNumber1, UTL_FILE_PATH_MAX + 1)); */
/*     printf("Average term snippet length:     %15.2f bytes\n", (double)uiTermSnippetLength / psiSrchIndex->ulUniqueTermCount); */

    printf("\n\nIndex:\n\n");
    printf("Index block count:               %15s\n", pucUtlStringsFormatUnsignedNumber(uiIndexBlockCount, pucPrintableNumber1, UTL_FILE_PATH_MAX + 1));
    printf("Total index block length:        %15s bytes\n", pucUtlStringsFormatUnsignedNumber(uiIndexBlockTotalLength, pucPrintableNumber1, UTL_FILE_PATH_MAX + 1));
    printf("Average index block length:      %15.2f bytes\n", (double)uiIndexBlockTotalLength / uiIndexBlockCount);
    printf("Median index block length:       %15s bytes\n", pucUtlStringsFormatUnsignedNumber(uiIndexBlockMedian, pucPrintableNumber1, UTL_FILE_PATH_MAX + 1));
    printf("Maximum index block length:      %15s bytes\n", pucUtlStringsFormatUnsignedNumber(uiIndexBlockMaxLength, pucPrintableNumber1, UTL_FILE_PATH_MAX + 1));
    printf("Minimum index block length:      %15s bytes\n", pucUtlStringsFormatUnsignedNumber(uiIndexBlockMinLength, pucPrintableNumber1, UTL_FILE_PATH_MAX + 1));
    printf("\n");


/*     for ( uiI = 1; uiI < VRF_INDEX_BLOCK_LEN_TO_CHECK_MAX; uiI++ ) { */
/*         printf("%u        %15u\n", uiI, puiIndexBlockLengthArray[uiI]); */
/*     } */


    /* Close the index */
    iSrchIndexClose(psiSrchIndex);


    return;


}


/*---------------------------------------------------------------------------*/


/*

    Function:   iListStatsCallBack()

    Purpose:    This function is passed to the dictionary lookup function to
                perform extra processing on a document key.

    Parameters: pucKey              key
                pvEntryData         entry data
                uiEntryLength       entry length
                ap                  args (optional)

    Globals:    none

    Returns:    0 to continue processing, non-0 otherwise

*/
static int iListStatsCallBack
(
    unsigned char *pucKey,
    void *pvEntryData,
    unsigned int uiEntryLength,
    va_list ap
)
{

    va_list             ap_;
    int                 iError = SRCH_NoError;

    struct srchIndex    *psiSrchIndex = NULL;
    unsigned int        *puiTermTotalLength = NULL;
    unsigned long       *pulUniqueTermCount = 0;
    unsigned long       *pulTotalTermCount = 0;
    unsigned long       *pulUniqueStopTermCount = 0;
    unsigned long       *pulTotalStopTermCount = 0;
    unsigned int        *puiIndexBlockCount = NULL;
    unsigned int        *puiIndexBlockTotalLength = NULL;
    unsigned int        *puiIndexBlockMaxLength = NULL;
    unsigned int        *puiIndexBlockMinLength = NULL;
    unsigned int        *puiIndexBlockLengthArray = NULL;

    unsigned char       *pucEntryData = NULL;
    unsigned char       *pucEntryDataPtr = NULL;

    unsigned int        uiTermType = 0;
    unsigned int        uiTermCount = 0;
    unsigned int        uiDocumentCount = 0;
    unsigned long       ulIndexBlockID = 0;

    unsigned char       *pucIndexBlock = NULL;
    unsigned char       *pucIndexBlockPtr = NULL;
    unsigned int        uiIndexBlockLength = 0;
    unsigned int        uiIndexBlockDataLength = 0;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iListDocumentKeysCallBack [%s][%u][%s]", pucDocumentKey, uiEntryLength, */
/*             (pvEntryData != NULL) ? "(pvEntryData != NULL)" : "(pvEntryData == NULL)" ); */


    ASSERT(bUtlStringsIsStringNULL(pucKey) == false);
    ASSERT(pvEntryData != NULL);
    ASSERT(uiEntryLength > 0);


    /* Get all our parameters, note that we make a copy of 'ap' */
    va_copy(ap_, ap);
    psiSrchIndex = (struct srchIndex *)va_arg(ap_, struct srchIndex *);
    puiTermTotalLength = (unsigned int *)va_arg(ap_, unsigned int *);
    pulUniqueTermCount = (unsigned long *)va_arg(ap_, unsigned long *);
    pulTotalTermCount = (unsigned long *)va_arg(ap_, unsigned long *);
    pulUniqueStopTermCount = (unsigned long *)va_arg(ap_, unsigned long *);
    pulTotalStopTermCount = (unsigned long *)va_arg(ap_, unsigned long *);
    puiIndexBlockCount = (unsigned int *)va_arg(ap_, unsigned int *);
    puiIndexBlockTotalLength = (unsigned int *)va_arg(ap_, unsigned int *);
    puiIndexBlockMaxLength = (unsigned int *)va_arg(ap_, unsigned int *);
    puiIndexBlockMinLength = (unsigned int *)va_arg(ap_, unsigned int *);
    puiIndexBlockLengthArray = (unsigned int *)va_arg(ap_, unsigned int *);
    va_end(ap_);


    /* Increment the total term length */
    *puiTermTotalLength += s_strlen(pucKey);
    
    /* Increment the index block count */
    (*puiIndexBlockCount)++;

    /* Set the entry data pointers */
    pucEntryData = (unsigned char *)pvEntryData;
    pucEntryDataPtr = pucEntryData;

    /* Extract some information from the data pointer, decode to a variable and copy to the pointer for optimization */ 
    pucEntryDataPtr = (unsigned char *)pvEntryData;
    UTL_NUM_READ_COMPRESSED_UINT(uiTermType, pucEntryDataPtr);
    UTL_NUM_READ_COMPRESSED_UINT(uiTermCount, pucEntryDataPtr);
    UTL_NUM_READ_COMPRESSED_UINT(uiDocumentCount, pucEntryDataPtr);
    UTL_NUM_READ_COMPRESSED_ULONG(ulIndexBlockID, pucEntryDataPtr);


    /* Is this a stop term ? */
    if ( uiTermType == SPI_TERM_TYPE_STOP ) {
        /* It is, so we just increment the stop term count */
        (*pulUniqueStopTermCount)++;
        *pulTotalStopTermCount += uiTermCount;
    }
    else if ( uiTermType == SPI_TERM_TYPE_REGULAR ) {

        /* Increment the actual total term count and unique term count */
        if ( bLngCaseDoesStringContainUpperCase(pucKey) == false ) {
            *pulTotalTermCount += uiTermCount;
            (*pulUniqueTermCount)++;
        }
    }


    /* Get the index block data */
    if ( (iError = iUtlDataGetEntry(psiSrchIndex->pvUtlIndexData, ulIndexBlockID, (void **)&pucIndexBlock, &uiIndexBlockLength)) != UTL_NoError ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to get index data, index object ID: %lu, index: '%s', utl error: %d.", 
                ulIndexBlockID, psiSrchIndex->pucIndexName, iError);
    }


    /* Get the size of the index block */
    pucIndexBlockPtr = pucIndexBlock;
    UTL_NUM_READ_COMPRESSED_UINT(uiIndexBlockDataLength, pucIndexBlockPtr);
    ASSERT(uiIndexBlockDataLength < uiIndexBlockLength);
    *puiIndexBlockTotalLength += uiIndexBlockDataLength;

    *puiIndexBlockMaxLength = UTL_MACROS_MAX(*puiIndexBlockMaxLength, uiIndexBlockDataLength);
    *puiIndexBlockMinLength = UTL_MACROS_MIN(*puiIndexBlockMinLength, uiIndexBlockDataLength);

    if ( uiIndexBlockDataLength < VRF_INDEX_BLOCK_LEN_TO_CHECK_MAX ) {
        puiIndexBlockLengthArray[uiIndexBlockDataLength]++;
    }


    return (0);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   vListTemporaryIndexFile()

    Purpose:    This function lists a temporary index file.

    Parameters: pucTemporaryIndexFilePath       temporary index file path
                bListIndexBlockHeader           list index block header
                bListIndexBlockData             list index block data

    Globals:    none

    Returns:    void

*/
static void vListTemporaryIndexFile
(
    unsigned char *pucTemporaryIndexFilePath,
    boolean bListIndexBlockHeader,
    boolean bListIndexBlockData
)
{

    FILE            *pfFile = NULL;

    unsigned int    uiTermType = 0;
    unsigned int    uiTermCount = 0;
    unsigned int    uiIncludeInTermCounts = 0;
    unsigned int    uiDocumentCount = 0;
    unsigned int    uiTermLength = 0;
    unsigned int    uiTermCapacity = 0;
    unsigned char   *pucTerm = NULL;

    unsigned char   pucBuffer[SRCH_INVERT_INDEX_BLOCK_HEADER_LENGTH];
    unsigned char   *pucBufferPtr = NULL;
    
    unsigned int    uiIndexBlockDataLength = 0;
    unsigned int    uiIndexBlockDataCapacity = 0;

    unsigned char   *pucIndexBlockData = NULL;
    unsigned char   *pucIndexBlockDataPtr = NULL;
    unsigned char   *pucIndexBlockDataEndPtr = NULL;

    unsigned int    uiIndexEntryDocumentID = 0;
    unsigned int    uiIndexEntryTermPosition = 0;
    unsigned int    uiIndexEntryFieldID = 0;

    unsigned int    uiIndexEntryPreviousDocumentID = 0;


    ASSERT(bUtlStringsIsStringNULL(pucTemporaryIndexFilePath) == false);


    /* Open the index file */
    if ( (pfFile = s_fopen(pucTemporaryIndexFilePath, "r")) == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to open the temporary index file: '%s'", pucTemporaryIndexFilePath);
    }


    if ( (bListIndexBlockHeader == true) && (bListIndexBlockData == false) ) {
        printf("Term                                          Term Count   Document Count        Term Type          Include      Term Length\n");
/*         printf("Term                                          Term Count   Document Count        Term Type\n"); */
    }
    

    /* Loop while there is stuff to read */
    while ( true ) {
    
        /* Read the header */
        if ( s_fread(pucBuffer, SRCH_INVERT_INDEX_BLOCK_HEADER_LENGTH, 1, pfFile) != 1 ) {

            /* Not an error, it is how we tell that we are done */
            if ( s_feof(pfFile) != 0 ) {
                break;
            }
            else {
                iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to read the index block header from the temporary index file: '%s'", pucTemporaryIndexFilePath);
            }
        }


        /* Decode the header */
        pucBufferPtr = pucBuffer;
        UTL_NUM_READ_UINT(uiTermType, SRCH_INVERT_INDEX_BLOCK_TERM_TYPE_SIZE, pucBufferPtr);
        UTL_NUM_READ_UINT(uiTermCount, SRCH_INVERT_INDEX_BLOCK_TERM_COUNT_SIZE, pucBufferPtr);
        UTL_NUM_READ_UINT(uiDocumentCount, SRCH_INVERT_INDEX_BLOCK_DOCUMENT_COUNT_SIZE, pucBufferPtr);
        UTL_NUM_READ_UINT(uiIncludeInTermCounts, SRCH_INVERT_INDEX_BLOCK_INCLUDE_IN_COUNTS_SIZE, pucBufferPtr);
        UTL_NUM_READ_UINT(uiTermLength, SRCH_INVERT_INDEX_BLOCK_TERM_SIZE, pucBufferPtr);


        /* Allocate space for the term */
        if ( uiTermLength > uiTermCapacity ) {

            uiTermCapacity = uiTermLength + 1;

            if ( (pucTerm = (unsigned char *)s_realloc(pucTerm, (sizeof(unsigned char) * uiTermCapacity))) == NULL ) {
                iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to allocate memory for the term");
            }
        }
    

        /* Read the term and NULL terminate it */
        if ( s_fread(pucTerm, uiTermLength, 1, pfFile) != 1 ) {
            iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to read the term from the temporary index file: '%s'", pucTemporaryIndexFilePath);
        }
        
        pucTerm[uiTermLength] = '\0';


        /* List the index block header */
        if ( (bListIndexBlockHeader == true) && (bListIndexBlockData == false) ) {
            printf(" %-40s       %10u       %10u       %10u       %10u       %10u\n", pucTerm, uiTermCount, uiDocumentCount, uiTermType, uiIncludeInTermCounts, uiTermLength);
/*             printf(" %-40s       %10u       %10u       %10u\n", pucTerm, uiTermCount, uiDocumentCount, uiTermType); */
        }
        
        else if ( (bListIndexBlockHeader == true) && (bListIndexBlockData == true) ) {
            printf("Term: '%s', term count: %u, document count: %u, term type: %u, include in term counts: %u, term length: %u\n", 
                    pucTerm, uiTermCount, uiDocumentCount, uiTermType, uiIncludeInTermCounts, uiTermLength);
        }
        
        
        /* Read the index block data length */
        if ( s_fread(pucBuffer, SRCH_INVERT_INDEX_BLOCK_DATA_LENGTH_SIZE, 1, pfFile) != 1 ) {
            iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to read the index block data length from the temporary index file: '%s'", pucTemporaryIndexFilePath);
        }


        /* Decode the index block data length */
        pucBufferPtr = pucBuffer;
        UTL_NUM_READ_UINT(uiIndexBlockDataLength, SRCH_INVERT_INDEX_BLOCK_DATA_LENGTH_SIZE, pucBufferPtr);
    
        /* Allocate space for the index block data */
        if ( uiIndexBlockDataLength > uiIndexBlockDataCapacity ) {

            uiIndexBlockDataCapacity = uiIndexBlockDataLength;

            if ( (pucIndexBlockData = (unsigned char *)s_realloc(pucIndexBlockData, (sizeof(unsigned char) * uiIndexBlockDataCapacity))) == NULL ) {
                iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to allocate memory for the index block data");
            }
        }
        
    
        /* Read the index block data */
        if ( s_fread(pucIndexBlockData, uiIndexBlockDataLength, 1, pfFile) != 1 ) {
            iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to read the index block data from the temporary index file: '%s'", pucTemporaryIndexFilePath);
        }


        if ( bListIndexBlockData == true ) {
            printf("    Document ID  Term Position       Field ID\n");
        }
    
        /* Decode the index block data */
        for ( pucIndexBlockDataPtr = pucIndexBlockData, pucIndexBlockDataEndPtr = (pucIndexBlockData + uiIndexBlockDataLength); 
                pucIndexBlockDataPtr < pucIndexBlockDataEndPtr; ) {

            /* Read the index entry */
            UTL_NUM_READ_COMPRESSED_UINT(uiIndexEntryDocumentID, pucIndexBlockDataPtr);
            UTL_NUM_READ_COMPRESSED_UINT(uiIndexEntryTermPosition, pucIndexBlockDataPtr);
            UTL_NUM_READ_COMPRESSED_UINT(uiIndexEntryFieldID, pucIndexBlockDataPtr);

            if ( uiIndexEntryPreviousDocumentID != uiIndexEntryDocumentID ) {
                uiIndexEntryPreviousDocumentID = uiIndexEntryDocumentID;
            }
                            
            /* List the index block data */
            if ( bListIndexBlockData == true ) {
                printf("     %10u     %10u     %10u\n", uiIndexEntryDocumentID, uiIndexEntryTermPosition, uiIndexEntryFieldID);
            }
        }

        if ( bListIndexBlockData == true ) {
            printf("\n");
        }
    }
    


    /* Free allocations */
    s_free(pucTerm);
    s_free(pucIndexBlockData);


    /* Close the index file */
    s_fclose(pfFile);


    return;

}


/*---------------------------------------------------------------------------*/


/*

    Function:   vTest()

    Purpose:    This is the test function.

    Parameters: none

    Globals:    none

    Returns:    void

*/
static void vTest
(

)
{

    void            *pvTrie = NULL;
    FILE            *pfFile = NULL;
    unsigned char   pucKey[10000] = {'\0'};
    

    iUtlTrieCreate(&pvTrie);
    
    
    pfFile = s_fopen("/home/francois/stoplist.txt", "r");
    
    while ( fgets(pucKey, 10000, pfFile) != NULL ) {

        pucKey[s_strlen(pucKey) - 1] = '\0';

/*         printf("%s\n", pucKey); */

        iUtlTrieAdd(pvTrie, pucKey, NULL);
    }
    
    s_fclose(pfFile);


/*     iUtlTriePrintKeys(pvTrie, NULL, false); */
/*     printf("\n"); */

    iUtlTriePrintKeys(pvTrie, "whe", false);
    printf("\n");

/*     iUtlTriePrintKeys(pvTrie, "ani", false); */
/*     printf("\n"); */

/*     iUtlTriePrintKeys(pvTrie, "astro", false); */
/*     printf("\n"); */


    iUtlTrieFree(pvTrie, false);


    return;

}


/*---------------------------------------------------------------------------*/

