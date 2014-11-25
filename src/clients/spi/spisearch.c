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

    Module:     spisearch.c

    Author:     Francois Schiettecatte (FS Consulting LLC.)

    Created:    18 January 2001

    Purpose:    This is an application which gives us a command line
                interface to test the SPI with.

*/


/*---------------------------------------------------------------------------*/


/*
** Include
*/

#include "utils.h"

#include "rep.h"

#include "lng.h"

#include "spi.h"



/*---------------------------------------------------------------------------*/


/*
** Feature defines
*/

/* Context for logging */
#undef UTL_LOG_CONTEXT
#define UTL_LOG_CONTEXT                     (unsigned char *)"com.fsconsult.mps.src.clients.spi.spisearch"


/*---------------------------------------------------------------------------*/


/*
** Defines
*/

/* Line lengths */
#define SPI_SEARCH_SHORT_LINE_LENGTH                        (1024)


/* Index name separators */
#define SPI_SEARCH_INDEX_NAME_SEPARATORS                    (unsigned char *)", "


/* Default index directory path */
#define SPI_SEARCH_INDEX_DIRECTORY_PATH_DEFAULT             (unsigned char *)"./"

/* Default configuration directory path */
#define SPI_SEARCH_CONFIGURATION_DIRECTORY_PATH_DEFAULT     SPI_SEARCH_INDEX_DIRECTORY_PATH_DEFAULT

/* Default temporary directory path */
#define SPI_SEARCH_TEMPORARY_DIRECTORY_PATH_DEFAULT         (unsigned char *)P_tmpdir


/* Default maximum document count */
#define SPI_SEARCH_MAX_DOCS_DEFAULT                         (20)

/* Default language code */
#define SPI_SEARCH_LANGUAGE_NAME_DEFAULT                    LNG_LANGUAGE_EN_CODE

/* Default locale name */
#define SPI_SEARCH_LOCALE_NAME_DEFAULT                      LNG_LOCALE_EN_US_UTF_8_NAME


/*---------------------------------------------------------------------------*/


/*
** Private function prototypes
*/

static void vVersion (void);
static void vUsage (unsigned char *pucCommandPath);

static int iSpiSearchInteractive (unsigned char *pucIndexDirectoryPath, unsigned char *pucConfigurationDirectoryPath, 
        unsigned char *pucTemporaryDirectoryPath, unsigned char *pucIndexNames, unsigned char *pucLanguageCode, 
        unsigned char *pucSearchText, unsigned char *pucPositiveFeedbackText, unsigned char *pucNegativeFeedbackText, 
        unsigned int uiStartIndex, unsigned int uiEndIndex, boolean bLongEntries, boolean bQuit);


/*---------------------------------------------------------------------------*/


/*

    Function:   main()

    Purpose:    main, what else ??

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
    unsigned char   pucTemporaryDirectoryPath[UTL_FILE_PATH_MAX + 1] = {'\0'};
    unsigned char   *pucIndexNames = NULL;
    unsigned char   *pucLanguageCode = SPI_SEARCH_LANGUAGE_NAME_DEFAULT;
    unsigned char   *pucSearchText = NULL;
    unsigned char   *pucPositiveFeedbackText = NULL;
    unsigned char   *pucNegativeFeedbackText = NULL;
    unsigned int    uiStartIndex = 0;
    unsigned int    uiEndIndex = SPI_SEARCH_MAX_DOCS_DEFAULT - 1;
    boolean         bLongEntries = false;
    boolean         bQuit = false;

    unsigned char   *pucLocaleName = SPI_SEARCH_LOCALE_NAME_DEFAULT;

    unsigned char   *pucLogFilePath = UTL_LOG_FILE_STDERR;    
    unsigned int    uiLogLevel = UTL_LOG_LEVEL_INFO;


    /* Set defaults */
    s_strnncpy(pucIndexDirectoryPath, SPI_SEARCH_INDEX_DIRECTORY_PATH_DEFAULT, UTL_FILE_PATH_MAX + 1);
    s_strnncpy(pucConfigurationDirectoryPath, SPI_SEARCH_CONFIGURATION_DIRECTORY_PATH_DEFAULT, UTL_FILE_PATH_MAX + 1);
    s_strnncpy(pucTemporaryDirectoryPath, SPI_SEARCH_TEMPORARY_DIRECTORY_PATH_DEFAULT, UTL_FILE_PATH_MAX + 1);


    /* Initialize the log */
    if ( (iError = iUtlLogInit()) != UTL_NoError ) {
        vVersion();
        iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to initialize the log, utl error: %d", iError);
    }


    /* Get the command path */
    pucCommandPath = pucUtlArgsGetNextArg(&argc, &argv);

    /* Check that we got at least one argument */
    if ( (pucNextArgument = pucUtlArgsGetNextArg(&argc, &argv)) == NULL ) {
        vVersion();
        vUsage(pucCommandPath);
        s_exit(EXIT_SUCCESS);
    }

    /* Process arguments */
      while ( pucNextArgument != NULL ) {

        /* Check for configuration directory path */
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

        /* Check for index directory path */
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

        /* Check for temporary directory path */
        else if ( s_strncmp("--temporary-directory=", pucNextArgument, s_strlen("--temporary-directory=")) == 0 ) {

            /* Get the temporary directory path */
            pucNextArgument += s_strlen("--temporary-directory=");

            /* Get the true temporary directory path */
            if ( (iError = iUtlFileGetTruePath(pucNextArgument, pucTemporaryDirectoryPath, UTL_FILE_PATH_MAX + 1)) != UTL_NoError ) {
                vVersion();
                iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to get the true temporary directory path: '%s', utl error: %d", pucNextArgument, iError);
            }

            /* Check that temporary directory path exists */
            if ( bUtlFilePathExists(pucTemporaryDirectoryPath) == false ) {
                vVersion();
                iUtlLogPanic(UTL_LOG_CONTEXT, "The temporary directory: '%s', does not exist", pucTemporaryDirectoryPath);
            }

            /* Check that the temporary directory path is a directory */
            if ( bUtlFileIsDirectory(pucTemporaryDirectoryPath) == false ) {
                vVersion();
                iUtlLogPanic(UTL_LOG_CONTEXT, "The temporary directory: '%s', is not a directory", pucTemporaryDirectoryPath);
            }

            /* Check that the temporary directory path can be accessed */
            if ( (bUtlFilePathRead(pucTemporaryDirectoryPath) == false) || (bUtlFilePathExec(pucTemporaryDirectoryPath) == false) ) {
                vVersion();
                iUtlLogPanic(UTL_LOG_CONTEXT, "The temporary directory: '%s', cannot be accessed", pucTemporaryDirectoryPath);
            }
        }

        /* Check for index name */
        else if ( s_strncmp("--index=", pucNextArgument, s_strlen("--index=")) == 0 ) {

            /* Get the index names */
            pucNextArgument += s_strlen("--index=");

            /* Set the index names */
            pucIndexNames = pucNextArgument;
        }

        /* Check for maximum documents */
        else if ( s_strncmp("--maximum-documents=", pucNextArgument, s_strlen("--maximum-documents=")) == 0 ) {

            /* Get the maximum number of documents */
            pucNextArgument += s_strlen("--maximum-documents=");

            /* Check the maximum number of documents */
            if ( s_strtol(pucNextArgument, NULL, 10) <= 0 ) {
                vVersion();
                iUtlLogPanic(UTL_LOG_CONTEXT, "Expected the maximum number of documents to be greater than: 0");
            }
            
            uiEndIndex = s_strtol(pucNextArgument, NULL, 10) - 1;
        }
        
        /* Check for language code */
        else if ( s_strncmp("--language=", pucNextArgument, s_strlen("--language=")) == 0 ) {

            /* Get the language code */
            pucNextArgument += s_strlen("--language=");

            /* Set the language code */
            pucLanguageCode = pucNextArgument;
        }    

        /* Check for search */
        else if ( s_strncmp("--search=", pucNextArgument, s_strlen("--search=")) == 0 ) {

            /* Get the search */
            pucNextArgument += s_strlen("--search=");

            /* Set the search */
            pucSearchText = pucNextArgument;
        }    

        /* Check for long entries flag */
        else if ( s_strcmp("--long", pucNextArgument) == 0 ) {
            
            /* Set the long entries flag */
            bLongEntries = true;
        }    

        /* Check for quit flag */
        else if ( s_strcmp("--quit", pucNextArgument) == 0 ) {
            
            /* Set the quit flag */
            bQuit = true;
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


    /* Version message */
    vVersion();


    /* Check and set the locale, we require utf-8 compliance */
    if ( bLngLocationIsLocaleUTF8(LC_ALL, pucLocaleName) != true ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "The locale: '%s', does not appear to be utf-8 compliant", pucLocaleName);
    }
    if ( (iError = iLngLocationSetLocale(LC_ALL, pucLocaleName)) != LNG_NoError ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to set the locale to: '%s', lng error: %d", pucLocaleName, iError);
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

    /* Check that we have a index name */
    if ( bUtlStringsIsStringNULL(pucIndexNames) == true ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "A index name(s) is required");
    }


    /* Call the main function */
    iError = iSpiSearchInteractive(pucIndexDirectoryPath, pucConfigurationDirectoryPath, pucTemporaryDirectoryPath, 
            pucIndexNames, pucLanguageCode,  pucSearchText, pucPositiveFeedbackText, pucNegativeFeedbackText, 
            uiStartIndex, uiEndIndex, bLongEntries, bQuit);



    return ((iError == SPI_NoError) ? EXIT_SUCCESS : EXIT_FAILURE);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   vVersion()

    Purpose:    This function list out the version message.

    Parameters: void

    Globals:    none

    Returns:    void

*/
static void vVersion
(

)
{

    unsigned char   pucVersionString[UTL_VERSION_STRING_LENGTH + 1] = {'\0'};

    
    /* Copyright message */
    printf("SPI Search, %s\n", UTL_VERSION_COPYRIGHT_STRING);


    /* Get the version string */
    iUtlVersionGetVersionString(pucVersionString, UTL_VERSION_STRING_LENGTH + 1);

    /* Version message */
    printf("%s\n", pucVersionString);


    return;

}


/*---------------------------------------------------------------------------*/


/*

    Function:   vUsage()

    Purpose:    This function list out all the parameters that the waisindex
                takes.

    Parameters: pucCommandPath        command path

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
    printf("                  Configuration directory, defaults to: '%s'.\n", SPI_SEARCH_CONFIGURATION_DIRECTORY_PATH_DEFAULT);
    printf("  --index-directory=name \n");
    printf("                  Index directory, defaults to: '%s'.\n", SPI_SEARCH_INDEX_DIRECTORY_PATH_DEFAULT);
    printf("  --temporary-directory=name \n");
    printf("                  Temporary directory, defaults to: '%s'.\n", SPI_SEARCH_TEMPORARY_DIRECTORY_PATH_DEFAULT);
    printf("  --index=name[%sname[%s...]] \n", SPI_SEARCH_INDEX_NAME_SEPARATORS, SPI_SEARCH_INDEX_NAME_SEPARATORS);
    printf("                  Index names, '%s' delimited, required. \n", SPI_SEARCH_INDEX_NAME_SEPARATORS);
    printf("  --maximum-documents=# \n");
    printf("                  Maximum number of documents to retrieve, defaults to: %d. \n", SPI_SEARCH_MAX_DOCS_DEFAULT);
    printf("  --language=name \n");
    printf("                  Language code, defaults to '%s'. \n", SPI_SEARCH_LANGUAGE_NAME_DEFAULT);
    printf("  --search=search \n");
    printf("                  Search terms/expression (best to put it in quotes.)\n");
    printf("  --long          Display long entries following a search, defaults to short entries.\n");
    printf("  --quit          Quit after displaying search results.\n");
    printf("\n");

    printf(" Locale parameter: \n");
    printf("  --locale=name   Locale name, defaults to '%s', see 'locale' for list of \n", SPI_SEARCH_LOCALE_NAME_DEFAULT);
    printf("                      supported locales, locale chosen must support utf-8. \n");
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

    Function:   iSpiSearchInteractive()

    Purpose:    main function of this program

    Parameters: pucIndexDirectoryPath           index directory path (optional)
                pucConfigurationDirectoryPath   configuration directory path (optional)
                pucTemporaryDirectoryPath       temporary directory path (optional)
                pucIndexNames                   comma delimited list of index names
                pucLanguageCode                 language code (optional)
                pucSearchText                   search text (optional)
                pucPositiveFeedbackText         positive feedback text (optional)
                pucNegativeFeedbackText         negative feedback text (optional)
                uiStartIndex                    start index
                uiEndIndex                      end index, 0 if there is no end index
                bLongEntries                    true for long entries, false for short ones
                bQuit                           true to quit right after the search, false to request a new search

    Globals:    none

    Returns:    SPI error

*/
static int iSpiSearchInteractive
(
    unsigned char *pucIndexDirectoryPath,
    unsigned char *pucConfigurationDirectoryPath,
    unsigned char *pucTemporaryDirectoryPath,
    unsigned char *pucIndexNames,
    unsigned char *pucLanguageCode,
    unsigned char *pucSearchText,
    unsigned char *pucPositiveFeedbackText,
    unsigned char *pucNegativeFeedbackText,
    unsigned int uiStartIndex, 
    unsigned int uiEndIndex,
    boolean bLongEntries,
    boolean bQuit
)
{

    int                         iError = SPI_NoError;
    struct spiSession           *pssSpiSession = NULL;
    unsigned char               *pucIndexNamesCopy = NULL;
    unsigned char               *pucIndexNamesCopyPtr = NULL;
    unsigned char               *pucIndexNamesCopyStrtokPtr = NULL;
    void                        *pvIndex = NULL;
    void                        **ppvIndexList = NULL;
    unsigned char               *pucPtr = NULL;

    struct spiSearchResponse    *pssrSpiSearchResponse = NULL;
    struct spiSearchResult      *pssrSpiSearchResultsPtr = NULL;
    unsigned int                uiI = 0, uiJ = 0;
    unsigned int                uiSearchReportCount = 0;

    struct spiDocumentItem      *psdiSpiDocumentItemsPtr = NULL;

    void                        *pvData = NULL;
    unsigned int                uiDataLength = 0;

    unsigned int                uiEndIndexSaved = uiEndIndex;

    int                         iSearchResultsIndex = 0;
    unsigned char               pucUserResponse[SPI_SEARCH_SHORT_LINE_LENGTH + 1] = {'\0'};
    unsigned char               pucLocalSearchText[SPI_SEARCH_SHORT_LINE_LENGTH + 1] = {'\0'};

    unsigned char               pucString[SPI_SEARCH_SHORT_LINE_LENGTH + 1] = {'\0'};


    ASSERT((bUtlStringsIsStringNULL(pucIndexDirectoryPath) == false) || (pucIndexDirectoryPath == NULL));
    ASSERT((bUtlStringsIsStringNULL(pucConfigurationDirectoryPath) == false) || (pucConfigurationDirectoryPath == NULL));
    ASSERT((bUtlStringsIsStringNULL(pucTemporaryDirectoryPath) == false) || (pucTemporaryDirectoryPath == NULL));
    ASSERT(bUtlStringsIsStringNULL(pucIndexNames) == false);
    ASSERT((bUtlStringsIsStringNULL(pucLanguageCode) == false) || (pucLanguageCode == NULL));
    ASSERT((bUtlStringsIsStringNULL(pucSearchText) == false) || (pucSearchText == NULL));
    ASSERT((bUtlStringsIsStringNULL(pucPositiveFeedbackText) == false) || (pucPositiveFeedbackText == NULL));
    ASSERT((bUtlStringsIsStringNULL(pucNegativeFeedbackText) == false) || (pucNegativeFeedbackText == NULL));
    ASSERT(uiStartIndex >= 0);
    ASSERT(uiEndIndex >= 0);
    ASSERT(uiEndIndex >= uiStartIndex);
    ASSERT((bLongEntries == true) || (bLongEntries == false));
    ASSERT((bQuit == true) || (bQuit == false));


    /* Allocate a new spi session structure */
    if ( (pssSpiSession = (struct spiSession *)s_malloc((size_t)(sizeof(struct spiSession)))) == NULL ) {
        iError = SPI_MemError;
        goto bailFromiSpiSearchInteractive;
    }

    /* Populate it */
    if ( bUtlStringsIsStringNULL(pucIndexDirectoryPath) == false ) {
        if ( (pssSpiSession->pucIndexDirectoryPath = s_strdup(pucIndexDirectoryPath)) == NULL ) {
            iError = SPI_MemError;
            goto bailFromiSpiSearchInteractive;
        }
    }

    if ( bUtlStringsIsStringNULL(pucConfigurationDirectoryPath) == false ) {
        if ( (pssSpiSession->pucConfigurationDirectoryPath = s_strdup(pucConfigurationDirectoryPath)) == NULL ) {
            iError = SPI_MemError;
            goto bailFromiSpiSearchInteractive;
        }
    }

    if ( bUtlStringsIsStringNULL(pucTemporaryDirectoryPath) == false ) {
        if ( (pssSpiSession->pucTemporaryDirectoryPath = s_strdup(pucTemporaryDirectoryPath)) == NULL ) {
            iError = SPI_MemError;
            goto bailFromiSpiSearchInteractive;
        }
    }

    pssSpiSession->pvClientPtr = NULL;


    /* Initialize the server */
    if ( (iError = iSpiInitializeServer(pssSpiSession)) != SPI_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to initialize the server, spi error: %d", iError);
        return (iError);
    }



    /* Create a copy of the index names, we do this because we parse out
    ** the index names and create a index name list, this destroys
    ** the index names since we use s_strtok_r()
    */
    if ( (pucIndexNamesCopy = (unsigned char *)s_strdup(pucIndexNames)) == NULL ) {
        iError = SPI_MemError;
        goto bailFromiSpiSearchInteractive;
    }

    /* Loop parsing the index names */
    for ( pucIndexNamesCopyPtr = s_strtok_r(pucIndexNamesCopy, SPI_SEARCH_INDEX_NAME_SEPARATORS, (char **)&pucIndexNamesCopyStrtokPtr), uiI = 0; 
            pucIndexNamesCopyPtr != NULL; 
            pucIndexNamesCopyPtr = s_strtok_r(NULL, SPI_SEARCH_INDEX_NAME_SEPARATORS, (char **)&pucIndexNamesCopyStrtokPtr), uiI++ ) {

        void **ppvIndexListPtr = NULL;

        /* Open the index */
        if ( (iError = iSpiOpenIndex(pssSpiSession, pucIndexNamesCopyPtr, &pvIndex)) != SPI_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to open the index: '%s', spi error: %d", pucIndexNamesCopyPtr, iError);
            goto bailFromiSpiSearchInteractive;
        }

        /* Extend the search index structure list */
        if ( (ppvIndexListPtr = (void **)s_realloc(ppvIndexList, (size_t)(sizeof(void *) * (uiI + 2)))) == NULL ) {
            iError = SPI_MemError;
            goto bailFromiSpiSearchInteractive;
        }
        
        /* Hand over the pointer */
        ppvIndexList = ppvIndexListPtr;

        /* Add the search index structure */
        ppvIndexList[uiI] = pvIndex;

        /* NULL terminate index list */
        ppvIndexList[uiI + 1] = NULL;
    }

    /* Free the index name copy */
    s_free(pucIndexNamesCopy);


    /* Make a copy of the search text */
    s_strnncpy(pucLocalSearchText, pucSearchText, SPI_SEARCH_SHORT_LINE_LENGTH + 1);


    /* Loop until the user gets tired of this */
    do {

        /* Search the index */
        if ( (iError = iSpiSearchIndex(pssSpiSession, ppvIndexList, pucLanguageCode, pucLocalSearchText, pucPositiveFeedbackText, pucNegativeFeedbackText, 
                uiStartIndex, uiEndIndex, &pssrSpiSearchResponse)) != SPI_NoError ) {

            iUtlLogError(UTL_LOG_CONTEXT, "Failed to run the search, spi error: %d", iError);
            goto bailFromiSpiSearchInteractive;
        }
        else {

            /* Loop over all the entries in the results array and count up the search reports */
            for ( uiI = 0, pssrSpiSearchResultsPtr = pssrSpiSearchResponse->pssrSpiSearchResults, uiSearchReportCount = 0; 
                    uiI < pssrSpiSearchResponse->uiSpiSearchResultsLength; 
                    uiI++, pssrSpiSearchResultsPtr++ ) {

                if ( (pssrSpiSearchResultsPtr->psdiSpiDocumentItems != NULL) && 
                        (s_strcmp(pssrSpiSearchResultsPtr->psdiSpiDocumentItems->pucItemName, SPI_SEARCH_REPORT_ITEM_NAME) == 0) && 
                        (s_strcmp(pssrSpiSearchResultsPtr->psdiSpiDocumentItems->pucMimeType, SPI_SEARCH_REPORT_MIME_TYPE) == 0) ) {
                    uiSearchReportCount++;
                }
            }


            /* Tell the user how many results we got back */
            printf("Search Response - Number of results found: %u, returned: %u, maximum sort key: %.4f, in: %.1f milliseconds.", pssrSpiSearchResponse->uiTotalResults,  
                    pssrSpiSearchResponse->uiSpiSearchResultsLength - uiSearchReportCount, pssrSpiSearchResponse->dMaxSortKey, pssrSpiSearchResponse->dSearchTime);

            if ( uiSearchReportCount > 0 ) {
                printf(" (+%u search report%s)", uiSearchReportCount, (uiSearchReportCount == 1) ? "" : "s");
            }
            printf(".\n");
            
            if ( bLongEntries == false ) {
                printf(" Hit    Sort Key         Index        Doc. Key    Title\n");
            }
            else {
                printf(" Hit    Sort Key         Index        Doc. Key    Title       Language    Rank    Term Count    Date    Time    Items\n");
            }


            /* Loop over all the entries in the results array and print them out */
            for ( uiI = 0, pssrSpiSearchResultsPtr = pssrSpiSearchResponse->pssrSpiSearchResults; uiI < pssrSpiSearchResponse->uiSpiSearchResultsLength; uiI++, pssrSpiSearchResultsPtr++) {

                printf("%3u    ", uiI + 1 + uiStartIndex);

                /* Print the sort key string */
                switch ( pssrSpiSearchResponse->uiSortType ) {
        
                    case SPI_SORT_TYPE_DOUBLE_ASC:
                    case SPI_SORT_TYPE_DOUBLE_DESC:
                        printf("%9.4f    ", pssrSpiSearchResultsPtr->dSortKey);
                        break;
        
                    case SPI_SORT_TYPE_FLOAT_ASC:
                    case SPI_SORT_TYPE_FLOAT_DESC:
                        printf("%9.4f    ", pssrSpiSearchResultsPtr->fSortKey);
                        break;
                    
                    case SPI_SORT_TYPE_UINT_ASC:
                    case SPI_SORT_TYPE_UINT_DESC:
                        printf("%9u    ", pssrSpiSearchResultsPtr->uiSortKey);
                        break;
                    
                    case SPI_SORT_TYPE_ULONG_ASC:
                    case SPI_SORT_TYPE_ULONG_DESC:
                        printf("%9lu    ", pssrSpiSearchResultsPtr->ulSortKey);
                        break;
                    
                    case SPI_SORT_TYPE_UCHAR_ASC:
                    case SPI_SORT_TYPE_UCHAR_DESC:
                        printf("%9s    ", pucUtlStringsGetPrintableString(pssrSpiSearchResultsPtr->pucSortKey));
                        break;
                    
                    case SPI_SORT_TYPE_NO_SORT:
                        printf("(none)    ");
                        break;
        
                    case SPI_SORT_TYPE_UNKNOWN:
                        printf("(unknown)    ");
                        break;
                    
                    default:
                        printf("(invalid)    ");
                        break;
                }

                printf("%10s    ", pssrSpiSearchResultsPtr->pucIndexName);
            
                printf("%12s    ", pssrSpiSearchResultsPtr->pucDocumentKey);
            
                printf("'%s'    ", pssrSpiSearchResultsPtr->pucTitle);
            

                if ( bLongEntries == true ) {

                    printf("%s    ", pssrSpiSearchResultsPtr->pucLanguageCode);

                    printf("%9u    ", pssrSpiSearchResultsPtr->uiRank);

                    printf("%3u    ", pssrSpiSearchResultsPtr->uiTermCount);

                    printf("%14lu ", pssrSpiSearchResultsPtr->ulAnsiDate);
                    
                    /* Are there any document items for this hit? */
                    if ( pssrSpiSearchResultsPtr->psdiSpiDocumentItems != NULL ) {

                        /* Loop through each item, assemble a readable string from them */
                        for ( uiJ = 0, psdiSpiDocumentItemsPtr = pssrSpiSearchResultsPtr->psdiSpiDocumentItems; uiJ < pssrSpiSearchResultsPtr->uiDocumentItemsLength; uiJ++, psdiSpiDocumentItemsPtr++ ) {

                            printf("  ");
                            
                            if ( (uiJ > 0) && (uiJ < pssrSpiSearchResultsPtr->uiDocumentItemsLength) ) {
                                printf("; ");
                            }

                            printf(psdiSpiDocumentItemsPtr->pucItemName);
                            printf(", ");

                            printf(psdiSpiDocumentItemsPtr->pucMimeType);
                            printf(", ");

                            pucUtlStringsFormatDoubleAsBytes(psdiSpiDocumentItemsPtr->uiLength, pucString, SPI_SEARCH_SHORT_LINE_LENGTH + 1);
                            printf(pucString);
                        }
                    }
                    else {
                        /* No document items are defined */
                        printf("  (no items)\n\n");
                    }
                    printf("  %s\n", pucString);

                }

                printf("\n");
            }


            /* Quit here if requested to do so */
            if ( bQuit == true ) {
                iError = SPI_NoError;
                goto bailFromiSpiSearchInteractive;
            }


            /* Offer to download documents if any were returned */
            if ( pssrSpiSearchResponse->uiSpiSearchResultsLength >= 0 ) {

                /* Keep downloading documents until the user wants out */
                do {            

                    /* Keep looping asking for a document index until we get a  valid answer */
                    do {

                        printf("\nDocument number (#), results range (#-#) or 'q' to quit: ");
                        s_fflush(stdout);
                        if ( s_fgets(pucUserResponse, SPI_SEARCH_SHORT_LINE_LENGTH, stdin) == NULL) {
                            iUtlLogError(UTL_LOG_CONTEXT, "Failed to read 'stdin' for user response.");
                            iError = SPI_MiscError;
                            goto bailFromiSpiSearchInteractive;
                        }

                        /* Trim string */
                        iUtlStringsTrimString(pucUserResponse);

                        /* Check for signal to quit */
                        if ( s_strcmp(pucUserResponse, "q") == 0 ) {
                            iSearchResultsIndex = -1; 
                        }
                        else if ( (pucPtr = s_strchr(pucUserResponse, '-')) != NULL ) {
                            
                            uiStartIndex = s_strtol(pucUserResponse, NULL, 10) - 1;
                            uiEndIndex = s_strtol(pucPtr + 1, NULL, 10) - 1;
                            iSearchResultsIndex = -2;
                    
                            /* Check that the index specified by the user is within the range */
                            if ( uiStartIndex > uiEndIndex ) {
                                /* The index entered by the user was out of range, so tell them so and loop */
                                printf("Entry must be a valid range, you entered: '%s'.\n", pucUserResponse);
                                iSearchResultsIndex = 0; 
                            }

                        }
                        else {

                            /* Convert the user response to a index */
                            iSearchResultsIndex = s_strtol(pucUserResponse, NULL, 10);
                            
                            iSearchResultsIndex -= uiStartIndex;

                            /* Check that the index specified by the user is within the range */
                            if ( !((iSearchResultsIndex > 0) && (iSearchResultsIndex <= pssrSpiSearchResponse->uiSpiSearchResultsLength)) ) {
                                /* The index entered by the user was out of range, so tell them so and loop */
                                printf("Entry must be a number between 1 and %u, you entered: '%s'.\n", pssrSpiSearchResponse->uiSpiSearchResultsLength, pucUserResponse);
                                iSearchResultsIndex = 0; 
                            }
                        }
                    } while ( iSearchResultsIndex == 0 );


                
                    /* Retrieve the document if we have a real one */
                    if ( iSearchResultsIndex > 0 ) {

                        /* Dereference the search result */
                        pssrSpiSearchResultsPtr = (pssrSpiSearchResponse->pssrSpiSearchResults + (iSearchResultsIndex - 1));

                        /* Check for document items */
                        if ( pssrSpiSearchResultsPtr->psdiSpiDocumentItems != NULL ) {
        
                            /* Flag indicating that the search report was retrieved from the index
                            ** as opposed to extracted from the search results
                            */
                            boolean bRetrievedFromIndex = false;
        
                            /* Reset the pointers */
                            pvData = NULL;
                            uiDataLength = 0;

                            /* Get the document from the search result structure if it is there */
                            if ( pssrSpiSearchResultsPtr->psdiSpiDocumentItems->pvData != NULL ) {

                                /* Duplicate the data */
                                if ( (pvData = s_memdup(pssrSpiSearchResultsPtr->psdiSpiDocumentItems->pvData, pssrSpiSearchResultsPtr->psdiSpiDocumentItems->uiDataLength)) == NULL ) {
                                    iError = SPI_MemError;
                                    goto bailFromiSpiSearchInteractive;
                                }

                                /* Hand over the data length */
                                uiDataLength = pssrSpiSearchResultsPtr->psdiSpiDocumentItems->uiDataLength;
                            }
                            else {

                                /* Open the index */
                                if ( (iError = iSpiOpenIndex(pssSpiSession, pssrSpiSearchResultsPtr->pucIndexName, &pvIndex)) != SPI_NoError ) {
                                    iUtlLogError(UTL_LOG_CONTEXT, "Failed to open the index: '%s', spi error: %d.", pssrSpiSearchResultsPtr->pucIndexName, iError);
                                    goto bailFromiSpiSearchInteractive;
                                }
        
                                /* Retrieve the document */
                                if ( (iError = iSpiRetrieveDocument(pssSpiSession, pvIndex, pssrSpiSearchResultsPtr->pucDocumentKey, pssrSpiSearchResultsPtr->psdiSpiDocumentItems->pucItemName, 
                                        pssrSpiSearchResultsPtr->psdiSpiDocumentItems->pucMimeType, SPI_CHUNK_TYPE_DOCUMENT, 0, 0, &pvData, &uiDataLength)) != SPI_NoError ) {
                                    iUtlLogError(UTL_LOG_CONTEXT, "Failed to retrieve the document: '%s', spi error: %d.", pssrSpiSearchResultsPtr->pucDocumentKey, iError);
                                    goto bailFromiSpiSearchInteractive;
                                }
                                    
                                /* Set the flag */
                                bRetrievedFromIndex = true;
                            }
                            
                            
                            /* Print the data */
                            if ( pvData != NULL ) {
    
                                /* NULL terminate the data */
                                if ( (iError = iUtlStringsNullTerminateData(pvData, uiDataLength, (unsigned char **)&pvData)) != UTL_NoError ) {
                                    iError = SPI_MemError;
                                    goto bailFromiSpiSearchInteractive;
                                }

                                /* Format the search report if this is one */
                                if ( (s_strcmp(pssrSpiSearchResultsPtr->psdiSpiDocumentItems->pucItemName, SPI_SEARCH_REPORT_ITEM_NAME) == 0) && 
                                        (s_strcmp(pssrSpiSearchResultsPtr->psdiSpiDocumentItems->pucMimeType, SPI_SEARCH_REPORT_MIME_TYPE) == 0) ) {
    
                                    unsigned char    *pucSearchReport = NULL;
                                    unsigned char    *ppucSearchReportList[2] = {NULL, NULL};
                                    
                                    /* Set the data pointer in the search report list */
                                    ppucSearchReportList[0] = (unsigned char *)pvData;
                                    
                                    /* Merge and format the search reports */
                                    if ( (iError = iRepMergeAndFormatSearchReports(ppucSearchReportList, &pucSearchReport)) != REP_NoError ) {
                                        iUtlLogWarn(UTL_LOG_CONTEXT, "Failed to merge and format the search reports, rep error: %d.", iError);
                                        iError = SPI_NoError;
                                    }
                                    else {
                                        /* Print the formatted search report */
                                        printf("\n[%s]\n\n\n", pucSearchReport);
    
                                        /* Free the formatted search report */
                                        s_free(pucSearchReport);
                                    }
                                }
                                else {
                                    /* Print the data */
                                    printf("\n[%s]\n\n\n", (unsigned char *)pvData);
                                }
                                
                                /* Free the data */
                                s_free(pvData);
                            }
                            
                            /* Close the index */
                            if ( bRetrievedFromIndex == true ) {
                            
                                /* Close the index */
                                if ( (iError = iSpiCloseIndex(pssSpiSession, pvIndex)) != SPI_NoError ) {
                                    iUtlLogError(UTL_LOG_CONTEXT, "Failed to close a index, spi error: %d.", iError);
                                }
                                pvIndex = NULL;
                            }
                        }
                        else {
                            iUtlLogWarn(UTL_LOG_CONTEXT, "Failed to retrieve the document, there are no document items for this document or there are no mime types for the default document item");
                        }
                    }

                } while ( (iSearchResultsIndex != -1) && (iSearchResultsIndex != -2) );

                /* Free the search response */
                iSpiFreeSearchResponse(pssrSpiSearchResponse);
                pssrSpiSearchResponse = NULL;
            }
        }


        /* New search */
        if ( iSearchResultsIndex == -1 ) {
        
            uiStartIndex = 0;
            uiEndIndex = uiEndIndexSaved;
            
            /* Get the next search */
            printf("Search for new terms [type 'q' to quit]: ");
             s_fflush(stdout);

            if ( s_fgets(pucLocalSearchText, SPI_SEARCH_SHORT_LINE_LENGTH, stdin) == NULL) {
                iUtlLogError(UTL_LOG_CONTEXT, "Failed to read 'stdin' for user response.");
                iError = SPI_MiscError;
                goto bailFromiSpiSearchInteractive;
            }

            /* Trim string */
            iUtlStringsTrimString(pucLocalSearchText);
        }

    }  while ( s_strcmp(pucLocalSearchText, "q") != 0 );



    /* Bail label */
    bailFromiSpiSearchInteractive:



    /* Free the search response */
    iSpiFreeSearchResponse(pssrSpiSearchResponse);
    pssrSpiSearchResponse = NULL;


    /* Close the indexes and free the search index structure list */
    if ( ppvIndexList != NULL ) {
        for ( uiI = 0; ppvIndexList[uiI] != NULL; uiI++ ) {
            if ( (iError = iSpiCloseIndex(pssSpiSession, ppvIndexList[uiI])) != SPI_NoError ) {
                iUtlLogError(UTL_LOG_CONTEXT, "Failed to close a index, spi error: %d.", iError);
            }
        }
        s_free(ppvIndexList);
    }


    /* Shutdown the server */
    if ( (iError = iSpiShutdownServer(pssSpiSession)) != SPI_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to shutdown the server, spi error: %d.", iError);
    }

    /* Free the spi session structure */
    iSpiFreeSession(pssSpiSession);
    pssSpiSession = NULL;



    return (iError);

}


/*---------------------------------------------------------------------------*/




