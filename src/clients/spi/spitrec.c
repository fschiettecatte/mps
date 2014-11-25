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

    Module:     spitrec.c

    Author:     Francois Schiettecatte (FS Consulting LLC.)

    Created:    25 March 1999

    Purpose:    This is an application which gives us a nice way to run
                Trec queries and generate results.

*/


/*---------------------------------------------------------------------------*/


/*
** Includes
*/

#include "utils.h"

#include "lng.h"

#include "spi.h"


/*---------------------------------------------------------------------------*/


/*
** Feature defines
*/

/* Context for logging */
#undef UTL_LOG_CONTEXT
#define UTL_LOG_CONTEXT                     (unsigned char *)"com.fsconsult.mps.src.clients.spi.spitrec"


/*---------------------------------------------------------------------------*/


/*
** Defines
*/

/* Line lengths */
#define SPI_TREC_LONG_LINE_LENGTH                       (51200)


/* Index name separators */
#define SPI_TREC_INDEX_NAME_SEPARATORS                  (unsigned char *)", "


/* Relevance feedback separators */
#define SPI_TREC_RELEVANCE_FEEDBACK_SEPARATORS          (unsigned char *)", "


/* Default index directory path */
#define SPI_TREC_INDEX_DIRECTORY_PATH_DEFAULT           (unsigned char *)"./"

/* Default configuration directory path */
#define SPI_TREC_CONFIGURATION_DIRECTORY_PATH_DEFAULT   SPI_TREC_INDEX_DIRECTORY_PATH_DEFAULT

/* Default temporary directory path */
#define SPI_TREC_TEMPORARY_DIRECTORY_PATH_DEFAULT       (unsigned char *)P_tmpdir


/* Defaults */
#define SPI_TREC_INDEX_NAME_DEFAULT                     (unsigned char *)"trec7"
#define SPI_TREC_RUN_NAME_DEFAULT                       (unsigned char *)"fsclt7"
#define SPI_TREC_SEARCH_SECTION_NAME_DEFAULT            (unsigned char *)"ques"
#define SPI_TREC_DEFAULT_FEEDBACK_SECTION_NAME          (unsigned char *)"rf"
#define SPI_TREC_TOPIC_FILE_PATH_DEFAULT                (unsigned char *)"topics.351-400"
#define SPI_TREC_MAX_DOCS_DEFAULT                       (1000)    
#define SPI_TREC_DEFAULT_MAX_WEIGHT                     (1000)    
#define SPI_TREC_DEFAULT_MIN_WEIGHT                     (1)    


/* Default language code */
#define SPI_TREC_LANGUAGE_NAME_DEFAULT                  LNG_LANGUAGE_EN_CODE

/* Default locale name */
#define SPI_TREC_LOCALE_NAME_DEFAULT                    LNG_LOCALE_EN_US_UTF_8_NAME


/*---------------------------------------------------------------------------*/


/*
** Private function prototypes
*/

static void vVersion (void);
static void vUsage (unsigned char *pucCommandPath);

static void vSpiTrecSearchFromTopicFile (unsigned char *pucIndexDirectoryPath, unsigned char *pucConfigurationDirectoryPath, 
        unsigned char *pucTemporaryDirectoryPath, unsigned char *pucIndexNames, unsigned char *pucLanguageCode, 
        unsigned int uiStartIndex, unsigned int uiEndIndex, 
        unsigned char *pucTopicFilePath, unsigned char *pucRunName, unsigned char *pucSearchSectionName, 
        unsigned char *pucFeedbackSectionName, unsigned int uiRFDocumentCount, unsigned char *pucSearchOptions, 
        boolean bDisplaySearchReport);

static void vSpiTrecSearch (struct spiSession *pssSpiSession, unsigned char **ppucIndexNameList, void **ppvIndexList, 
        unsigned char *pucLanguageCode, unsigned char *pucSearchText, unsigned char *pucPositiveFeedbackText, unsigned char *pucNegativeFeedbackText,
        unsigned int uiStartIndex, unsigned int uiEndIndex, unsigned char *pucRunName, unsigned int uiQuestion, 
        unsigned char **ppucRFDocumentIDList, unsigned int uiRFDocumentCount, boolean bDisplaySearchReport);


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

    int             iError = UTL_NoError;
    unsigned char   *pucCommandPath = NULL;
    unsigned char   *pucNextArgument = NULL;

    unsigned char   pucConfigurationDirectoryPath[UTL_FILE_PATH_MAX + 1] = {'\0'};
    unsigned char   pucIndexDirectoryPath[UTL_FILE_PATH_MAX + 1] = {'\0'};
    unsigned char   pucTemporaryDirectoryPath[UTL_FILE_PATH_MAX + 1] = {'\0'};
    unsigned char   *pucIndexNames = SPI_TREC_INDEX_NAME_DEFAULT;
    unsigned char   *pucLanguageCode = SPI_TREC_LANGUAGE_NAME_DEFAULT;
    unsigned char   pucTopicFilePath[UTL_FILE_PATH_MAX + 1] = {'\0'};
    unsigned char   *pucRunName = SPI_TREC_RUN_NAME_DEFAULT;
    unsigned char   *pucSearchSectionName = SPI_TREC_SEARCH_SECTION_NAME_DEFAULT;
    unsigned char   *pucFeedbackSectionName = NULL;
    unsigned int    uiStartIndex = 0;
    unsigned int    uiEndIndex = SPI_TREC_MAX_DOCS_DEFAULT - 1;
    unsigned int    uiRFDocumentCount = 0;
    unsigned char   *pucSearchOptions = NULL;
    boolean         bDisplaySearchReport = false;

    time_t          tStartTime = (time_t)0;
    unsigned int    uiTimeTaken = 0;
    unsigned int    uiHours = 0;
    unsigned int    uiMinutes = 0;
    unsigned int    uiSeconds = 0;

    unsigned char   *pucLocaleName = SPI_TREC_LOCALE_NAME_DEFAULT;

    unsigned char   *pucLogFilePath = UTL_LOG_FILE_STDERR;    
    unsigned int    uiLogLevel = UTL_LOG_LEVEL_INFO;



    /* Set defaults */
    s_strnncpy(pucIndexDirectoryPath, SPI_TREC_INDEX_DIRECTORY_PATH_DEFAULT, UTL_FILE_PATH_MAX + 1);
    s_strnncpy(pucConfigurationDirectoryPath, SPI_TREC_CONFIGURATION_DIRECTORY_PATH_DEFAULT, UTL_FILE_PATH_MAX + 1);
    s_strnncpy(pucTemporaryDirectoryPath, SPI_TREC_TEMPORARY_DIRECTORY_PATH_DEFAULT, UTL_FILE_PATH_MAX + 1);
    s_strnncpy(pucTopicFilePath, SPI_TREC_TOPIC_FILE_PATH_DEFAULT, UTL_FILE_PATH_MAX + 1);


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

        /* Check for index names */
        else if ( s_strncmp("--index=", pucNextArgument, s_strlen("--index=")) == 0 ) {

            /* Get the index names */
            pucNextArgument += s_strlen("--index=");

            /* Set the index names */
            pucIndexNames = pucNextArgument;
        }

        /* Check for run name */
        else if ( s_strncmp("--run=", pucNextArgument, s_strlen("--run=")) == 0 ) {

            /* Get the run name */
            pucNextArgument += s_strlen("--run=");

            /* Set the run name */
            pucRunName = pucNextArgument;
        }

        /* Check for maximum number of documents */
        else if ( s_strncmp("--maximum-documents=", pucNextArgument, s_strlen("--maximum-documents=")) == 0 ) {

            /* Get the maximum number of documents */
            pucNextArgument += s_strlen("--maximum-documents=");

            /* Check the maximum number of documents */
            if ( s_strtol(pucNextArgument, NULL, 10) <= 0 ) {
                vVersion();
                iUtlLogPanic(UTL_LOG_CONTEXT, "Expected the maximum number of documents to be greater than: 0");
            }

            /* Set the maximum number of documents */
            uiEndIndex = s_strtol(pucNextArgument, NULL, 10);
        }    

        /* Check for topic file path */
        else if ( s_strncmp("--topic=", pucNextArgument, s_strlen("--topic=")) == 0 ) {

            /* Get the topic file path */
            pucNextArgument += s_strlen("--topic=");

            /* Get the true topic file path */
            if ( (iError = iUtlFileGetTruePath(pucNextArgument, pucTopicFilePath, UTL_FILE_PATH_MAX + 1)) != UTL_NoError ) {
                vVersion();
                iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to get the true topic file path: '%s', utl error: %d", pucNextArgument, iError);
            }

            /* Check that topic file path exists */
            if ( bUtlFilePathExists(pucTopicFilePath) == false ) {
                vVersion();
                iUtlLogPanic(UTL_LOG_CONTEXT, "The topic file path: '%s', does not exist", pucTopicFilePath);
            }

            /* Check that the topic file path is a file */
            if ( bUtlFileIsFile(pucTopicFilePath) == false ) {
                vVersion();
                iUtlLogPanic(UTL_LOG_CONTEXT, "The topic file path: '%s', is not a file", pucTopicFilePath);
            }

            /* Check that the topic file path can be accessed */
            if ( bUtlFilePathRead(pucTopicFilePath) == false ) {
                vVersion();
                iUtlLogPanic(UTL_LOG_CONTEXT, "The topic file path: '%s', cannot be accessed", pucTopicFilePath);
            }
        }

        /* Check for search section name */
        else if ( s_strncmp("--section=", pucNextArgument, s_strlen("--section=")) == 0 ) {

            /* Get the search section name */
            pucNextArgument += s_strlen("--section=");

            /* Set the search section name */
            pucSearchSectionName = pucNextArgument;
        }

        /* Check for feedback section name */
        else if ( (s_strcmp("--feedback", pucNextArgument) == 0) || (s_strncmp("--feedback=", pucNextArgument, s_strlen("--feedback=")) == 0) ) {

            /* Get the feedback section name */
            if ( s_strncmp("--feedback=", pucNextArgument, s_strlen("--feedback=")) == 0 ) {
                pucNextArgument += s_strlen("--feedback=");
            }
            else {
                pucNextArgument = NULL;
            }

            /* Set the relevance feedback count if possible otherwise fall back to feedback section name */
            if ( (pucNextArgument != NULL) && (s_strtol(pucNextArgument, NULL, 10) > 0) ) {
                /* Set the feedback document count */
                uiRFDocumentCount = s_strtol(pucNextArgument, NULL, 10);
            }
            else if ( pucNextArgument != NULL ) {
                /* Set the feedback section name */
                pucFeedbackSectionName = pucNextArgument;
            }
            else {
                /* Set the feedback section name */
                pucFeedbackSectionName = SPI_TREC_DEFAULT_FEEDBACK_SECTION_NAME;
            }
        }

        /* Check for search options */
        else if ( s_strncmp("--options=", pucNextArgument, s_strlen("--options=")) == 0 ) {

            /* Get the options */
            pucNextArgument += s_strlen("--options=");

            /* Set the options */
            pucSearchOptions = pucNextArgument;
        }    

        /* Check for report display flag */
        else if ( s_strcmp("--report", pucNextArgument) == 0 ) {
    
            /* Set the display search report flag */
            bDisplaySearchReport = true;
        }    

        /* Check for language code */
        else if ( s_strncmp("--language=", pucNextArgument, s_strlen("--language=")) == 0 ) {

            /* Get the language code */
            pucNextArgument += s_strlen("--language=");

            /* Set the language code */
            pucLanguageCode = pucNextArgument;
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


    /* Set the configuration directory path from the index directory path if it is not set */
    if ( bUtlStringsIsStringNULL(pucConfigurationDirectoryPath) == true ) {
        s_strnncpy(pucConfigurationDirectoryPath, pucIndexDirectoryPath, UTL_FILE_PATH_MAX + 1);
    }

    /* Set the temporary directory path from the index directory path if it is not set */
    if ( bUtlStringsIsStringNULL(pucTemporaryDirectoryPath) == true ) {
        s_strnncpy(pucTemporaryDirectoryPath, pucIndexDirectoryPath, UTL_FILE_PATH_MAX + 1);
    }


    /* Check that we have a index name */
    if ( bUtlStringsIsStringNULL(pucIndexNames) == true ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "A index name(s) is required");
    }

    /* Check that we have either a search section name or a feedback section name */
    if ( (bUtlStringsIsStringNULL(pucSearchSectionName) == true) && (bUtlStringsIsStringNULL(pucFeedbackSectionName) == true) ) {
        iUtlLogWarn(UTL_LOG_CONTEXT, "Neither a search section name and/or a feedback section name was specified, defaulting to search section name '%s'.\n", 
                SPI_TREC_SEARCH_SECTION_NAME_DEFAULT);
        pucSearchSectionName = SPI_TREC_SEARCH_SECTION_NAME_DEFAULT;
    }


    /* Start timing */
    tStartTime = s_time(NULL);


    /* Run the searches */
    vSpiTrecSearchFromTopicFile(pucIndexDirectoryPath, pucConfigurationDirectoryPath, pucTemporaryDirectoryPath, pucIndexNames, pucLanguageCode, 
            uiStartIndex, uiEndIndex, pucTopicFilePath, pucRunName, pucSearchSectionName, pucFeedbackSectionName, uiRFDocumentCount, pucSearchOptions, bDisplaySearchReport);


    /* End timing and print time elapsed */
    uiTimeTaken = s_difftime(s_time(NULL), tStartTime);
    uiHours = uiTimeTaken / (60 * 60);
    uiMinutes = (uiTimeTaken - (uiHours * 60 * 60)) / 60;
    uiSeconds = uiTimeTaken - ((uiTimeTaken / 60) * 60);

    if ( uiHours > 0 ) {
        iUtlLogInfo(UTL_LOG_CONTEXT, "Time taken: %u hour%s, %u minute%s and %u second%s.", 
                uiHours, (uiHours == 1) ? "" : "s", uiMinutes, (uiMinutes == 1) ? "" : "s", uiSeconds, (uiSeconds == 1) ? "" : "s");
    }
    else if ( uiMinutes > 0 ) {
        iUtlLogInfo(UTL_LOG_CONTEXT, "Time taken:  %u minute%s and %u second%s.", 
                uiMinutes, (uiMinutes == 1) ? "" : "s", uiSeconds, (uiSeconds == 1) ? "" : "s");
    }
    else {
        iUtlLogInfo(UTL_LOG_CONTEXT, "Time taken:  %u second%s.", uiSeconds, (uiSeconds == 1) ? "" : "s");
    }


    return (EXIT_SUCCESS);    

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
    fprintf(stderr, "SPI TREC Search, %s\n", UTL_VERSION_COPYRIGHT_STRING);


    /* Get the version string */
    iUtlVersionGetVersionString(pucVersionString, UTL_VERSION_STRING_LENGTH + 1);

    /* Version message */
    fprintf(stderr, "%s\n", pucVersionString);


    return;

}


/*---------------------------------------------------------------------------*/


/*

    Function:   vUsage()

    Purpose:    This function list out all the parameters that ltrec
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
    printf("                  Configuration directory, defaults to the index directory. \n");
    printf("  --index-directory=name \n");
    printf("                  Index directory.\n");
    printf("  --temporary-directory=name \n");
    printf("                  Temporary directory, defaults to the index directory. \n");
    printf("  --index=name[%sname[%s...]] \n", SPI_TREC_INDEX_NAME_SEPARATORS, SPI_TREC_INDEX_NAME_SEPARATORS);
    printf("                  Index names, '%s' delimited, defaults to '%s'. \n", SPI_TREC_INDEX_NAME_SEPARATORS, SPI_TREC_INDEX_NAME_DEFAULT);
    printf("  --run=name      Run name, defaults to '%s'. \n", SPI_TREC_RUN_NAME_DEFAULT);
    printf("  --maximum-documents=# \n");
    printf("                  Maximum number of documents to retrieve, defaults to: %d. \n", SPI_TREC_MAX_DOCS_DEFAULT);
    printf("  --topic=name    Topic file path, defaults to '%s'. \n", SPI_TREC_TOPIC_FILE_PATH_DEFAULT);
    printf("  --section=name  Topic file search section name, defaults to '%s'. \n", SPI_TREC_SEARCH_SECTION_NAME_DEFAULT);
    printf("  --feedback=[name|number] \n");
    printf("                  Topic file feedback section name or number of documents to feedback, \n");
    printf("                  section name defaults to '%s'. \n", SPI_TREC_DEFAULT_FEEDBACK_SECTION_NAME);
    printf("  --options=name  Search options and modifiers to add to a search, such as '{relaxedboolean}'. \n");
    printf("  --report        Retrieve and display the search report following a search. \n");
    printf("  --language=name \n");
    printf("                  Language code, defaults to '%s'. \n", SPI_TREC_LANGUAGE_NAME_DEFAULT);
    printf("\n");

    printf(" Locale parameter: \n");
    printf("  --locale=name   Locale name, defaults to '%s', see 'locale' for list of \n", SPI_TREC_LOCALE_NAME_DEFAULT);
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

    Function:   vSpiTrecSearchFromTopicFile()

    Purpose:    This function will read in a trec topic file, parse it out and 
                run each search against the server

    Parameters: pucIndexDirectoryPath           default index directory path (optional)
                pucConfigurationDirectoryPath   default configuration directory path (optional)
                pucTemporaryDirectoryPath       default temporary directory path (optional)
                pucIndexNames                   comma delimited list of index names
                pucLanguageCode                 language code (optional)
                uiStartIndex                    start index
                uiEndIndex                      end index, 0 if there is no end index
                pucTopicFilePath                topic file path
                pucRunName                      run name
                pucSearchSectionName            section name to use for the search (optional)
                pucFeedbackSectionName          section name to use for the feedback (optional)
                uiRFDocumentCount               number of entries to feedback (if greater than 0)
                pucSearchOptions                search options (optional)
                bDisplaySearchReport            set to true if the search report is to be displayed

    Globals:    none

    Returns:    void

*/
static void vSpiTrecSearchFromTopicFile
(
    unsigned char *pucIndexDirectoryPath,
    unsigned char *pucConfigurationDirectoryPath,
    unsigned char *pucTemporaryDirectoryPath,
    unsigned char *pucIndexNames,
    unsigned char *pucLanguageCode,
    unsigned int uiStartIndex, 
    unsigned int uiEndIndex,
    unsigned char *pucTopicFilePath,
    unsigned char *pucRunName,
    unsigned char *pucSearchSectionName,
    unsigned char *pucFeedbackSectionName,
    unsigned int uiRFDocumentCount,
    unsigned char *pucSearchOptions,
    boolean bDisplaySearchReport
)
{

    int                 iError = SPI_NoError;
    struct spiSession   *pssSpiSession = NULL;
    unsigned char       *pucIndexNamesCopy = NULL;
    unsigned char       *pucIndexNamePtr = NULL;
    unsigned char       *pucIndexNamesCopyStrtokPtr = NULL;
    unsigned char       **ppucIndexNameList = NULL;
    void                *pvIndex = NULL;
    void                **ppvIndexList = NULL;
    unsigned char       pucLine[SPI_TREC_LONG_LINE_LENGTH + 1] = {'\0'};
    FILE                *pfFile = NULL;
    unsigned int        uiQuestion = 0;
    unsigned int        uiCurrentSection = 0;
    unsigned char       *pucPtr = NULL;
    unsigned char       *pucNum = NULL;
    unsigned char       *pucTitle = NULL;
    unsigned char       *pucDesc = NULL;
    unsigned char       *pucNarr = NULL;
    unsigned char       *pucQues = NULL;
    unsigned char       *pucRF = NULL;
    unsigned char       *pucRFStrtokPtr = NULL;
    unsigned char       *pucSearchPtr = NULL;
    unsigned char       *pucPositiveFeedbackPtr = NULL;
    unsigned char       **ppucRFDocumentIDList = NULL;
    unsigned int        uiI = 0, uiJ = 0;


    ASSERT((bUtlStringsIsStringNULL(pucIndexDirectoryPath) == false) || (pucIndexDirectoryPath == NULL));
    ASSERT((bUtlStringsIsStringNULL(pucConfigurationDirectoryPath) == false) || (pucConfigurationDirectoryPath == NULL));
    ASSERT((bUtlStringsIsStringNULL(pucTemporaryDirectoryPath) == false) || (pucTemporaryDirectoryPath == NULL));
    ASSERT(bUtlStringsIsStringNULL(pucIndexNames) == false);
    ASSERT((bUtlStringsIsStringNULL(pucLanguageCode) == false) || (pucLanguageCode == NULL));
    ASSERT(uiStartIndex >= 0);
    ASSERT(uiEndIndex >= 0);
    ASSERT(uiEndIndex >= uiStartIndex);
    ASSERT(bUtlStringsIsStringNULL(pucTopicFilePath) == false);
    ASSERT(bUtlStringsIsStringNULL(pucRunName) == false);
    ASSERT((bUtlStringsIsStringNULL(pucSearchSectionName) == false) || (pucSearchSectionName == NULL));
    ASSERT((bUtlStringsIsStringNULL(pucFeedbackSectionName) == false) || (pucFeedbackSectionName == NULL));
    ASSERT(uiRFDocumentCount >= 0);
    ASSERT((bUtlStringsIsStringNULL(pucSearchOptions) == false) || (pucSearchOptions == NULL));
    ASSERT((bDisplaySearchReport == true) || (bDisplaySearchReport == false));


    /* Allocate a new spi session structure */
    if ( (pssSpiSession = (struct spiSession *)s_malloc((size_t)(sizeof(struct spiSession)))) == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Memory allocation error.");
    }

    /* Populate it */
    if ( bUtlStringsIsStringNULL(pucIndexDirectoryPath) == false ) {
        if ( (pssSpiSession->pucIndexDirectoryPath = s_strdup(pucIndexDirectoryPath)) == NULL ) {
            iUtlLogPanic(UTL_LOG_CONTEXT, "Memory allocation error");
        }
    }

    if ( bUtlStringsIsStringNULL(pucConfigurationDirectoryPath) == false ) {
        if ( (pssSpiSession->pucConfigurationDirectoryPath = s_strdup(pucConfigurationDirectoryPath)) == NULL ) {
            iUtlLogPanic(UTL_LOG_CONTEXT, "Memory allocation error");
        }
    }

    if ( bUtlStringsIsStringNULL(pucTemporaryDirectoryPath) == false ) {
        if ( (pssSpiSession->pucTemporaryDirectoryPath = s_strdup(pucTemporaryDirectoryPath)) == NULL ) {
            iUtlLogPanic(UTL_LOG_CONTEXT, "Memory allocation error");
        }
    }
    pssSpiSession->pvClientPtr = NULL;

    /* Initialize the server */
    if ( (iError = iSpiInitializeServer(pssSpiSession)) != SPI_NoError ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to initialize the server, spi error: %d", iError);
    }


    /* Create a copy of the index names, we do this because we parse out
    ** the index names and create a index name list, this destroys
    ** the index names since we use s_strtok_r()
    */
    if ( bUtlStringsIsStringNULL(pucIndexNames) == false ) {

        if ( (pucIndexNamesCopy = (unsigned char *)s_strdup(pucIndexNames)) == NULL ) {
            iUtlLogPanic(UTL_LOG_CONTEXT, "Memory allocation error");
        }

        /* Loop parsing the index names */
        for ( pucIndexNamePtr = s_strtok_r(pucIndexNamesCopy, SPI_TREC_INDEX_NAME_SEPARATORS, (char **)&pucIndexNamesCopyStrtokPtr), uiI = 0; 
                pucIndexNamePtr != NULL; 
                pucIndexNamePtr = s_strtok_r(NULL, SPI_TREC_INDEX_NAME_SEPARATORS, (char **)&pucIndexNamesCopyStrtokPtr), uiI++ ) {

            /* Add the index name to the index name list */
            if ( (ppucIndexNameList = (unsigned char **)s_realloc(ppucIndexNameList, (size_t)(sizeof(unsigned char *) * (uiI + 2)))) == NULL ) {
                iUtlLogPanic(UTL_LOG_CONTEXT, "Memory allocation error");
            }
            if ( (ppucIndexNameList[uiI] = (unsigned char *)s_strdup(pucIndexNamePtr)) == NULL ) {
                iUtlLogPanic(UTL_LOG_CONTEXT, "Memory allocation error");
            }
        }

        /* NULL terminate the index name list */
        ppucIndexNameList[uiI + 1] = NULL;

        /* Free the index name copy */
        s_free(pucIndexNamesCopy);

        /* Loop while there are indexex to open */
        for ( uiI = 0, uiJ = 0; ppucIndexNameList[uiI] != NULL; uiI++ ) {

            /* Open the index and add it to the index list if we opened it, otherwise log the error */
            if ( (iError = iSpiOpenIndex(pssSpiSession, ppucIndexNameList[uiI], &pvIndex)) == SPI_NoError ) {
                if ( (ppvIndexList = (void **)s_realloc(ppvIndexList, (size_t)(sizeof(void *) * (uiJ + 2)))) == NULL ) {
                    iUtlLogPanic(UTL_LOG_CONTEXT, "Memory allocation error");
                }
                ppvIndexList[uiJ] = pvIndex;

                /* Increment the index counter and NULL terminate the index list */
                ppvIndexList[++uiJ] = NULL;
            }
            else {
                iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to open the index: '%s', spi error: %d.", ppucIndexNameList[uiI], iError);
            }
        }
    }



    /* Open the topics file */
    if ( (pfFile = s_fopen(pucTopicFilePath, "r")) == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to open the topic file: '%s'", pucTopicFilePath);
    }

    /* Loop reading in all the lines */
    while ( s_fgets(pucLine, SPI_TREC_LONG_LINE_LENGTH, pfFile) != NULL ) {

        /* Trim string */
        iUtlStringsTrimString(pucLine);

        /* Have we hit a new topic? */
        if ( s_strncasecmp(pucLine, "<top>", 5) == 0 ) {

            /* Loop reading in the lines for this topic */
            while ( s_fgets(pucLine, SPI_TREC_LONG_LINE_LENGTH, pfFile) != NULL ) {

                /* Trim string */
                iUtlStringsTrimString(pucLine);

                /* Is this a new section */
                if ( pucLine[0] == '<' ) {
                    if ( s_strncasecmp(pucLine, "<num>", s_strlen("<num>")) == 0 ) {
                        pucPtr = (s_strchr(pucLine, ':') != NULL) ? (unsigned char *)s_strchr(pucLine, ':') + 1 : pucLine + s_strlen("<num>");
                        iUtlStringsAppendString(pucNum, pucPtr, &pucNum);
                        iUtlStringsAppendString(pucNum, " ", &pucNum);
                        uiCurrentSection = 1;
                    }
                    else if ( s_strncasecmp(pucLine, "<title>", s_strlen("<title>")) == 0 ) {
                        pucPtr = (s_strchr(pucLine, ':') != NULL) ? (unsigned char *)s_strchr(pucLine, ':') + 1 : pucLine + s_strlen("<title>");
                        iUtlStringsAppendString(pucTitle, pucPtr, &pucTitle);
                        iUtlStringsAppendString(pucTitle, " ", &pucTitle);
                        uiCurrentSection = 2;
                    }
                    else if ( s_strncasecmp(pucLine, "<desc>", s_strlen("<desc>")) == 0 ) {
                        pucPtr = (s_strchr(pucLine, ':') != NULL) ? (unsigned char *)s_strchr(pucLine, ':') + 1 : pucLine + s_strlen("<desc>");
                        iUtlStringsAppendString(pucDesc, pucPtr, &pucDesc);
                        iUtlStringsAppendString(pucDesc, " ", &pucDesc);
                        uiCurrentSection = 3;
                    }
                    else if ( s_strncasecmp(pucLine, "<narr>", s_strlen("<narr>")) == 0 ) {
                        pucPtr = (s_strchr(pucLine, ':') != NULL) ? (unsigned char *)s_strchr(pucLine, ':') + 1 : pucLine + s_strlen("<narr>");
                        iUtlStringsAppendString(pucNarr, pucPtr, &pucNarr);
                        iUtlStringsAppendString(pucNarr, " ", &pucNarr);
                        uiCurrentSection = 4;
                    }
                    else if ( s_strncasecmp(pucLine, "<ques>", s_strlen("<ques>")) == 0 ) {
                        pucPtr = (s_strchr(pucLine, ':') != NULL) ? (unsigned char *)s_strchr(pucLine, ':') + 1 : pucLine + s_strlen("<ques>");
                        iUtlStringsAppendString(pucQues, pucPtr, &pucQues);
                        iUtlStringsAppendString(pucQues, " ", &pucQues);
                        uiCurrentSection = 5;
                    }
                    else if ( s_strncasecmp(pucLine, "<rf>", s_strlen("<rf>")) == 0 ) {
                        pucPtr = (s_strchr(pucLine, ':') != NULL) ? (unsigned char *)s_strchr(pucLine, ':') + 1 : pucLine + s_strlen("<rf>");
                        iUtlStringsAppendString(pucRF, pucPtr, &pucRF);
                        iUtlStringsAppendString(pucRF, " ", &pucRF);
                        uiCurrentSection = 6;
                    }
                    else if ( pucLine[0] == '<' ) {
                        /* We hit an unknown section */
                        uiCurrentSection = 0;
                    }
                }
                else {
                    if ( uiCurrentSection == 1 ) {
                        iUtlStringsAppendString(pucNum, pucLine, &pucNum);
                        iUtlStringsAppendString(pucNum, " ", &pucNum);
                    }
                    else if ( uiCurrentSection == 2 ) {
                        iUtlStringsAppendString(pucTitle, pucLine, &pucTitle);
                        iUtlStringsAppendString(pucTitle, " ", &pucTitle);
                    }
                    else if ( uiCurrentSection == 3 ) {
                        iUtlStringsAppendString(pucDesc, pucLine, &pucDesc);
                        iUtlStringsAppendString(pucDesc, " ", &pucDesc);
                    }
                    else if ( uiCurrentSection == 4 ) {
                        iUtlStringsAppendString(pucNarr, pucLine, &pucNarr);
                        iUtlStringsAppendString(pucNarr, " ", &pucNarr);
                    }
                    else if ( uiCurrentSection == 5 ) {
                        iUtlStringsAppendString(pucQues, pucLine, &pucQues);
                        iUtlStringsAppendString(pucQues, " ", &pucQues);
                    }
                    else if ( uiCurrentSection == 6 ) {
                        iUtlStringsAppendString(pucRF, pucLine, &pucRF);
                        iUtlStringsAppendString(pucRF, " ", &pucRF);
                    }
                }

                /* Have we hit the end of the topic? */
                if ( s_strncasecmp(pucLine, "</top>", 6) == 0 ) {

                    /* We have, so we run the question */

                    /* Set the RF  */
                    if ( (bUtlStringsIsStringNULL(pucFeedbackSectionName) == false) && (s_strcmp(pucFeedbackSectionName, "rf") == 0) && (bUtlStringsIsStringNULL(pucRF) == false) ) {
                        /* Set the RF from what we read in the topic file */

                        /* Loop while there is stuff to read */
                        for ( pucPtr = s_strtok_r(pucRF, SPI_TREC_RELEVANCE_FEEDBACK_SEPARATORS, (char **)&pucRFStrtokPtr), uiI = 0;
                                pucPtr != NULL;
                                pucPtr = s_strtok_r(NULL, SPI_TREC_RELEVANCE_FEEDBACK_SEPARATORS, (char **)&pucRFStrtokPtr), uiI++ ) {

                            /* Document keys are stored in lower case */
                            pucLngCaseConvertStringToLowerCase(pucPtr);

                            /* Add this entry to the list */
                            if ( (ppucRFDocumentIDList = (unsigned char **)s_realloc(ppucRFDocumentIDList, (size_t)(sizeof(unsigned char *) * (uiI + 2)))) == NULL ) {
                                iUtlLogPanic(UTL_LOG_CONTEXT, "Memory allocation error");
                            }
                            if ( (ppucRFDocumentIDList[uiI] = (unsigned char *)s_strdup(pucPtr)) == NULL ) {
                                iUtlLogPanic(UTL_LOG_CONTEXT, "Memory allocation error");
                            }
                            
                            /* NULL terminate the list */
                            ppucRFDocumentIDList[uiI + 1] = NULL;
                        }
                    }



                    /* Get the question number */
                    uiQuestion = s_strtol(pucNum, NULL, 10);

                    /* Set the search pointer */
                    if ( bUtlStringsIsStringNULL(pucSearchSectionName) == false ) {
                        if ( s_strcmp(pucSearchSectionName, "ques") == 0 )
                            pucSearchPtr = pucQues;
                        else if ( s_strcmp(pucSearchSectionName, "title") == 0 )
                            pucSearchPtr = pucTitle;
                        else if ( s_strcmp(pucSearchSectionName, "desc") == 0 )
                            pucSearchPtr = pucDesc;
                        else if ( s_strcmp(pucSearchSectionName, "narr") == 0 )
                            pucSearchPtr = pucNarr;
                        else {
                            iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to find the search section: '%s'", pucSearchSectionName);
                        }
                        
                        if ( pucSearchPtr == NULL ) {
                            iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to find the search in search section: '%s'", pucSearchSectionName);
                        }
                        
                        if ( (pucSearchPtr = (unsigned char *)s_strdup(pucSearchPtr)) == NULL ) {
                            iUtlLogPanic(UTL_LOG_CONTEXT, "Memory allocation error");
                        }
                        
                        if ( bUtlStringsIsStringNULL(pucSearchOptions) == false ) {
                            iUtlStringsAppendString(pucSearchPtr, pucSearchOptions, &pucSearchPtr);
                            iUtlStringsAppendString(pucSearchPtr, " ", &pucSearchPtr);
                        }

                        if ( bDisplaySearchReport == false ) {
                            iUtlStringsAppendString(pucSearchPtr, "{noreport} ", &pucSearchPtr);
                        }
                    }


                    /* Set the feedback pointer */
                    if ( bUtlStringsIsStringNULL(pucFeedbackSectionName) == false ) {
                        if ( s_strcmp(pucFeedbackSectionName, "ques") == 0 )
                            pucPositiveFeedbackPtr = pucQues;
                        else if ( s_strcmp(pucFeedbackSectionName, "title") == 0 )
                            pucPositiveFeedbackPtr = pucTitle;
                        else if ( s_strcmp(pucFeedbackSectionName, "desc") == 0 )
                            pucPositiveFeedbackPtr = pucDesc;
                        else if ( s_strcmp(pucFeedbackSectionName, "narr") == 0 )
                            pucPositiveFeedbackPtr = pucNarr;
                        else {
                            iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to find the feedback section: '%s'", pucFeedbackSectionName);
                        }
                    }


                    fprintf(stderr, "Running question: %u.\n", uiQuestion);
                    fprintf(stderr, "Search Text: '%s'.\n", pucUtlStringsGetPrintableString(pucSearchPtr));
                    if ( bUtlStringsIsStringNULL(pucPositiveFeedbackPtr) == false ) {
                        fprintf(stderr, "Feedback Text: '%s'.\n", pucUtlStringsGetPrintableString(pucPositiveFeedbackPtr));
                    }


                    /* Run the search */
                    vSpiTrecSearch(pssSpiSession, ppucIndexNameList, ppvIndexList, pucLanguageCode, pucSearchPtr, pucPositiveFeedbackPtr, NULL, 
                            uiStartIndex, uiEndIndex, pucRunName, uiQuestion, ppucRFDocumentIDList, uiRFDocumentCount, bDisplaySearchReport);

                    /* Space between searches in the log */
                    fprintf(stderr, "\n\n");


                    /* Free allocated memory */
                    s_free(pucSearchPtr);
                    s_free(pucNum);
                    s_free(pucTitle);
                    s_free(pucDesc);
                    s_free(pucNarr);
                    s_free(pucQues);
                    s_free(pucRF);
                    if ( ppucRFDocumentIDList != NULL ) {
                        for ( uiI = 0; ppucRFDocumentIDList[uiI] != NULL; uiI++ ) {
                            s_free(ppucRFDocumentIDList[uiI]);
                        }
                        s_free(ppucRFDocumentIDList);
                    }

                    /* And break */
                    break;

                }
            }
        }
    }



    /* Free the index name list */
    UTL_MACROS_FREE_NULL_TERMINATED_LIST(ppucIndexNameList);


    /* Close the indexes and free the search index structure list */
    if ( ppvIndexList != NULL ) {
        for ( uiI = 0; ppvIndexList[uiI] != NULL; uiI++ ) {
            if ( (iError = iSpiCloseIndex(pssSpiSession, ppvIndexList[uiI])) != SPI_NoError ) {
                iUtlLogWarn(UTL_LOG_CONTEXT, "Failed to close a index, spi error: %d.", iError);
            }
        }
        s_free(ppvIndexList);
    }


    /* Shutdown the server */
    if ( (iError = iSpiShutdownServer(pssSpiSession)) != SPI_NoError ) {
        iUtlLogWarn(UTL_LOG_CONTEXT, "Failed to shutdown the server, spi error: %d.", iError);
    }

    /* Free the spi session structure */
    iSpiFreeSession(pssSpiSession);
    pssSpiSession = NULL;


    return;

}


/*---------------------------------------------------------------------------*/


/*

    Function:   vSpiTrecSearch()

    Purpose:    This function is used to access the server

    Parameters: pssSpiSession               spi session structure
                ppucIndexNameList           index name list
                ppvIndexList                search index structure list
                pucLanguageCode             language code (optional)
                pucSearchText               search text (optional)
                pucPositiveFeedbackText     positive feedback text (optional)
                pucNegativeFeedbackText     negative feedback text (optional)
                uiStartIndex                start index
                uiEndIndex                  end index, 0 if there is no end index
                pucRunName                  run name
                uiQuestion                  question number
                ppucRFDocumentIDList        document ID list of entries to feeback (optional)
                uiRFDocumentCount           number of entries to feedback (if greater than 0)
                bDisplaySearchReport        set to true if the search report is to be displayed

    Globals:    none

    Returns:    SPI error code

*/
static void vSpiTrecSearch
(
    struct spiSession *pssSpiSession,
    unsigned char **ppucIndexNameList,
    void **ppvIndexList,
    unsigned char *pucLanguageCode,
    unsigned char *pucSearchText,
    unsigned char *pucPositiveFeedbackText,
    unsigned char *pucNegativeFeedbackText,
    unsigned int uiStartIndex, 
    unsigned int uiEndIndex,
    unsigned char *pucRunName,
    unsigned int uiQuestion,
    unsigned char **ppucRFDocumentIDList,
    unsigned int uiRFDocumentCount,
    boolean bDisplaySearchReport
)
{

    int                         iError = SPI_NoError;

    unsigned char               *pucPositiveFeedback = NULL;
    unsigned char               *pucPtr = NULL;

    struct spiSearchResponse    *pssrSpiSearchResponse = NULL;
    struct spiSearchResult      *pssrSpiSearchResultsPtr = NULL;
    unsigned int                uiI = 0, uiJ = 0;

    struct spiDocumentItem      *psdiSpiDocumentItemsPtr = NULL;

    unsigned int                uiWeight = 0;

    void                        *pvData = NULL;
    unsigned int                uiDataLength = 0;

    unsigned int                uiSearchReportIndex = 0;

    void                        *pvIndex = NULL;


    ASSERT(pssSpiSession != NULL);
    ASSERT(ppucIndexNameList != NULL);
    ASSERT(ppvIndexList != NULL);
    ASSERT((bUtlStringsIsStringNULL(pucLanguageCode) == false) || (pucLanguageCode == NULL));
    ASSERT((bUtlStringsIsStringNULL(pucSearchText) == false) || (pucSearchText == NULL));
    ASSERT((bUtlStringsIsStringNULL(pucPositiveFeedbackText) == false) || (pucPositiveFeedbackText == NULL));
    ASSERT((bUtlStringsIsStringNULL(pucNegativeFeedbackText) == false) || (pucNegativeFeedbackText == NULL));
    ASSERT(uiStartIndex >= 0);
    ASSERT(uiEndIndex >= 0);
    ASSERT(uiEndIndex >= uiStartIndex);
    ASSERT(bUtlStringsIsStringNULL(pucRunName) == false);
    ASSERT(uiQuestion >= 0);
    ASSERT((ppucRFDocumentIDList == NULL) || (ppucRFDocumentIDList != NULL));
    ASSERT(uiRFDocumentCount >= 0);
    ASSERT((bDisplaySearchReport == true) || (bDisplaySearchReport == false));


    /* Search the index */
    if ( (iError = iSpiSearchIndex(pssSpiSession, ppvIndexList, pucLanguageCode, pucSearchText, pucPositiveFeedbackText, pucNegativeFeedbackText, 
            uiStartIndex, uiEndIndex, &pssrSpiSearchResponse)) != SPI_NoError ) {

        iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to submit the search, spi error: %d.", iError);
    }


    /* Add documents as relevance feedback if needed */
    if ( (uiRFDocumentCount > 0) || (ppucRFDocumentIDList != NULL) ) {

        if ( uiRFDocumentCount > 0 ) {
            
            for ( uiI = 1, pssrSpiSearchResultsPtr = pssrSpiSearchResponse->pssrSpiSearchResults; 
                    (uiI <= uiRFDocumentCount) && (uiI <= pssrSpiSearchResponse->uiSpiSearchResultsLength); 
                    uiI++, pssrSpiSearchResultsPtr++ ) {

                /* Open the index */
                if ( (iError = iSpiOpenIndex(pssSpiSession, pssrSpiSearchResultsPtr->pucIndexName, &pvIndex)) != SPI_NoError ) {
                    iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to open the index: '%s', spi error: %d", pssrSpiSearchResultsPtr->pucIndexName, iError);
                }

                /* Retrieve the document */
                iError = iSpiRetrieveDocument(pssSpiSession, pvIndex, pssrSpiSearchResultsPtr->pucDocumentKey, "document", 
                        "text/plain", SPI_CHUNK_TYPE_DOCUMENT, 0, 0, &pvData, &uiDataLength);

                /* Close the index - ignore errors */
                iSpiCloseIndex(pssSpiSession, pvIndex);
                pvIndex = NULL;

                /* Catch the retrieval error */
                if ( iError != SPI_NoError ) {
                    iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to retrieve the document, spi error: %d.", iError);
                }
                
                fprintf(stderr, "Adding document: %u as RF.\n", uiI);

                /* Append the data to the string */
                iUtlStringsAppendData(pucPositiveFeedback, pvData, uiDataLength, &pucPositiveFeedback);

                /* Free the data */
                s_free(pvData);
            }
        }
        else if ( ppucRFDocumentIDList != NULL ) {

            /* Loop over each entry in the RF list */
            for ( uiI = 0; ppucRFDocumentIDList[uiI] != NULL; uiI++ ) {

                boolean bGotIt = false;

                /* Loop over each index */
                for ( uiJ = 0; ppucIndexNameList[uiJ] != NULL; uiJ++ ) {

                    /* Open the index */
                    if ( (iError = iSpiOpenIndex(pssSpiSession, ppucIndexNameList[uiJ], &pvIndex)) != SPI_NoError ) {
                        iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to open the index: '%s', spi error: %d", ppucIndexNameList[uiJ], iError);
                    }

                    /* Retrieve the document */
                    if ( (iError = iSpiRetrieveDocument(pssSpiSession, pvIndex, pssrSpiSearchResultsPtr->pucDocumentKey, "document", 
                            "text/plain", SPI_CHUNK_TYPE_DOCUMENT, 0, 0, &pvData, &uiDataLength)) != SPI_NoError ) {
                        iUtlLogWarn(UTL_LOG_CONTEXT, "Failed to retrieve document, spi error: %d.", iError);
                    }

                    /* Close the index - ignore errors */
                    iSpiCloseIndex(pssSpiSession, pvIndex);
                    pvIndex = NULL;

                    /* Catch the retrieval error */
                    if ( iError == SPI_NoError ) {
                        bGotIt = true;
                        break;
                    }
                }

                /* Process the document if we got it, otherwise we report an error */
                if ( bGotIt == true ) {

                    fprintf(stderr, "Adding document ID: '%s' as RF.\n", ppucRFDocumentIDList[uiI]);

                    /* Append the data to the string */
                    iUtlStringsAppendData(pucPositiveFeedback, pvData, uiDataLength, &pucPositiveFeedback);

                    /* Free the data */
                    s_free(pvData);
                }
                else {
                    iUtlLogWarn(UTL_LOG_CONTEXT, "Failed to add document ID: '%s' as RF.", ppucRFDocumentIDList[uiI]);
                }
            }
        }


        /* Free the search response */
        iSpiFreeSearchResponse(pssrSpiSearchResponse);
        pssrSpiSearchResponse = NULL;


        /* Search the index */
        if ( (iError = iSpiSearchIndex(pssSpiSession, ppvIndexList, pucLanguageCode, pucSearchText, pucPositiveFeedback, NULL, 
                uiStartIndex, uiEndIndex, &pssrSpiSearchResponse)) != SPI_NoError ) {

            iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to submit the search, spi error: %d.", iError);
        }
    }



    fprintf(stderr, "Search Response - Number of Records Returned: %u.\n", pssrSpiSearchResponse->uiSpiSearchResultsLength);


    /* We only need to display the search results if we got any */
    if ( pssrSpiSearchResponse->uiSpiSearchResultsLength > 0 ) {    

        /* Loop over all the entries in the results array and print them out */
        for ( uiI = 0, pssrSpiSearchResultsPtr = pssrSpiSearchResponse->pssrSpiSearchResults; uiI < pssrSpiSearchResponse->uiSpiSearchResultsLength; uiI++, pssrSpiSearchResultsPtr++ ) { 

            /* Intercept the search report */
            if ( (pssrSpiSearchResultsPtr->psdiSpiDocumentItems != NULL) && 
                    (s_strcmp(pssrSpiSearchResultsPtr->psdiSpiDocumentItems->pucItemName, SPI_SEARCH_REPORT_ITEM_NAME) == 0) &&
                    (s_strcmp(pssrSpiSearchResultsPtr->psdiSpiDocumentItems->pucMimeType, SPI_SEARCH_REPORT_MIME_TYPE) == 0) ) {

                uiSearchReportIndex = uiI;
            }
            else {
                /* Make sure we truncate the title */
                if ( (pucPtr = (unsigned char *)s_strchr(pssrSpiSearchResultsPtr->pucTitle, ' ')) != NULL ) {
                    *pucPtr = '\0';
                }

                /* Normalize the weight */
                switch ( pssrSpiSearchResponse->uiSortType ) {
        
                    case SPI_SORT_TYPE_DOUBLE_ASC:
                    case SPI_SORT_TYPE_DOUBLE_DESC:
                        uiWeight = (pssrSpiSearchResultsPtr->dSortKey <= SPI_SORT_KEY_DOUBLE_MINIMUM) ?  
                                SPI_TREC_DEFAULT_MIN_WEIGHT : (((double)pssrSpiSearchResultsPtr->dSortKey / pssrSpiSearchResponse->dMaxSortKey) * SPI_TREC_DEFAULT_MAX_WEIGHT);
                        break;
        
                    case SPI_SORT_TYPE_FLOAT_ASC:
                    case SPI_SORT_TYPE_FLOAT_DESC:
                        uiWeight = (pssrSpiSearchResultsPtr->fSortKey <= SPI_SORT_KEY_FLOAT_MINIMUM) ? 
                                SPI_TREC_DEFAULT_MIN_WEIGHT : (((double)pssrSpiSearchResultsPtr->fSortKey / pssrSpiSearchResponse->dMaxSortKey) * SPI_TREC_DEFAULT_MAX_WEIGHT);
                        break;
                    
                    case SPI_SORT_TYPE_UINT_ASC:
                    case SPI_SORT_TYPE_UINT_DESC:
                        uiWeight = (pssrSpiSearchResultsPtr->uiSortKey <= SPI_SORT_KEY_UINT_MINIMUM) ? 
                                SPI_TREC_DEFAULT_MIN_WEIGHT : (((double)pssrSpiSearchResultsPtr->uiSortKey / pssrSpiSearchResponse->dMaxSortKey) * SPI_TREC_DEFAULT_MAX_WEIGHT);
                        break;
                    
                    case SPI_SORT_TYPE_ULONG_ASC:
                    case SPI_SORT_TYPE_ULONG_DESC:
                        uiWeight = (pssrSpiSearchResultsPtr->ulSortKey <= SPI_SORT_KEY_ULONG_MINIMUM) ?
                                SPI_TREC_DEFAULT_MIN_WEIGHT : (((double)pssrSpiSearchResultsPtr->ulSortKey / pssrSpiSearchResponse->dMaxSortKey) * SPI_TREC_DEFAULT_MAX_WEIGHT);
                        break;
                    
                    case SPI_SORT_TYPE_UCHAR_ASC:
                    case SPI_SORT_TYPE_UCHAR_DESC:
                        uiWeight = SPI_TREC_DEFAULT_MIN_WEIGHT;
                        break;
                    
                    case SPI_SORT_TYPE_NO_SORT:
                        uiWeight = SPI_TREC_DEFAULT_MIN_WEIGHT;
                        break;
        
                    case SPI_SORT_TYPE_UNKNOWN:
                        uiWeight = SPI_TREC_DEFAULT_MIN_WEIGHT;
                        break;
                    
                    default:
                        uiWeight = SPI_TREC_DEFAULT_MIN_WEIGHT;
                        break;
                }

                printf("%4u  Q0  %-25s  %-10u  %-10u  %s\n", uiQuestion, pssrSpiSearchResultsPtr->pucTitle, uiI, uiWeight, pucRunName);
            }
        }    
    }    


    /* Do we want to display the search report? */
    if ( bDisplaySearchReport == true ) {

        /* Dereference the search result */
        pssrSpiSearchResultsPtr = (pssrSpiSearchResponse->pssrSpiSearchResults + uiSearchReportIndex);

        /* Dereference the default item and the default mime type */
        if ( (psdiSpiDocumentItemsPtr = pssrSpiSearchResultsPtr->psdiSpiDocumentItems) == NULL ) {
            iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to retrieve the document, there are no document items for this document");
        }

        /* Open the index */
        if ( (iError = iSpiOpenIndex(pssSpiSession, pssrSpiSearchResultsPtr->pucIndexName, &pvIndex)) != SPI_NoError ) {
            iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to open the index: '%s', spi error: %d", pssrSpiSearchResultsPtr->pucIndexName, iError);
        }

        /* Retrieve the document */
        iError = iSpiRetrieveDocument(pssSpiSession, pvIndex, pssrSpiSearchResultsPtr->pucDocumentKey, psdiSpiDocumentItemsPtr->pucItemName, 
                psdiSpiDocumentItemsPtr->pucMimeType, SPI_CHUNK_TYPE_DOCUMENT, 0, 0, &pvData, &uiDataLength);

        /* Close the index - ignore errors */
        iSpiCloseIndex(pssSpiSession, pvIndex);
        pvIndex = NULL;

        /* Catch the retrieval error */
        if ( iError != SPI_NoError ) {
            iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to retrieve the document, spi error: %d.", iError);
        }

        /* NULL terminate the data */
        iUtlStringsNullTerminateData(pvData, uiDataLength, (unsigned char **)&pvData);

        /* Print the data */
        fprintf(stderr, "\n%s\n\n\n", (unsigned char *)pvData);
        s_free(pvData);
    }


    /* Free the search response */
    iSpiFreeSearchResponse(pssrSpiSearchResponse);
    pssrSpiSearchResponse = NULL;

    /* Free the feedback text */
    s_free(pucPositiveFeedback);


    return;
}


/*---------------------------------------------------------------------------*/

