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

    Module:     ltrec.c

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

#include "lwps.h"

#include "lng.h"

#include "spi.h"


/*---------------------------------------------------------------------------*/


/*
** Feature defines
*/

/* Context for logging */
#undef UTL_LOG_CONTEXT
#define UTL_LOG_CONTEXT                     (unsigned char *)"com.fsconsult.mps.src.clients.lwps.ltrec"


/*---------------------------------------------------------------------------*/


/*
** Defines
*/

/* Line lengths */
#define LTREC_LONG_LINE_LENGTH                    (51200)


/* Default protocol name and ID */
#define LTREC_NET_PROTOCOL_NAME_DEFAULT            UTL_NET_PROTOCOL_TCP_NAME
#define LTREC_NET_PROTOCOL_ID_DEFAULT            UTL_NET_PROTOCOL_TCP_ID


/* Index name separators */
#define LTREC_INDEX_NAME_SEPARATORS                (unsigned char *)", "


/* Relevance feedback separators */
#define LTREC_RELEVANCE_FEEDBACK_SEPARATORS        (unsigned char *)", "


/* Defaults */
#define    LTREC_HOST_DEFAULT                        (LWPS_PROTOCOL_HOST_DEFAULT)
#define    LTREC_PORT_DEFAULT                        (LWPS_PROTOCOL_PORT_DEFAULT)
#define LTREC_TIMEOUT_DEFAULT                    (LWPS_PROTOCOL_TIMEOUT_DEFAULT)
#define LTREC_INDEX_DEFAULT                        (unsigned char *)"trec7"
#define LTREC_RUN_NAME_DEFAULT                    (unsigned char *)"fsclt7"
#define LTREC_SEARCH_SECTION_NAME_DEFAULT        (unsigned char *)"ques"
#define LTREC_DEFAULT_FEEDBACK_SECTION_NAME        (unsigned char *)"rf"
#define LTREC_TOPIC_FILE_PATH_DEFAULT            (unsigned char *)"topics.351-400"
#define LTREC_MAX_DOCS_DEFAULT                    (1000)    
#define LTREC_DEFAULT_MAX_WEIGHT                (1000)    
#define LTREC_DEFAULT_MIN_WEIGHT                (1)    


/* Default language code */
#define LTREC_LANGUAGE_NAME_DEFAULT                LNG_LANGUAGE_EN_CODE

/* Default locale name */
#define LTREC_LOCALE_NAME_DEFAULT                 LNG_LOCALE_EN_US_UTF_8_NAME


/*---------------------------------------------------------------------------*/


/*
** Private function prototypes
*/

static void vVersion (void);
static void vUsage (unsigned char *pucCommandPath);

static void vLTrecSearchFromTopicFile (unsigned int uiNetProtocolID, unsigned char *pucHostName, int iPort, unsigned int uiTimeOut,
        unsigned char *pucIndexNames, unsigned char *pucLanguageCode, unsigned int uiStartIndex, unsigned int uiEndIndex, 
        unsigned char *pucTopicFilePath, unsigned char *pucRunName, unsigned char *pucSearchSectionName, unsigned char *pucFeedbackSectionName, 
        unsigned int uiRFDocumentCount, unsigned char *pucSearchOptions, boolean bDisplaySearchReport);

static void vLTrecSearch (void *pvLwps, unsigned char **ppucIndexNameList, unsigned char *pucLanguageCode, 
        unsigned char *pucSearchText, unsigned char *pucPositiveFeedbackText, unsigned char *pucNegativeFeedbackText, 
        unsigned int uiStartIndex, unsigned int uiEndIndex, 
        unsigned char *pucRunName, unsigned int uiQuestion, unsigned char **ppucRFDocumentIDList, 
        unsigned int uiRFDocumentCount, boolean bDisplaySearchReport);


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

    unsigned int    uiNetProtocolID = LTREC_NET_PROTOCOL_ID_DEFAULT;
    unsigned char   *pucHostName = LTREC_HOST_DEFAULT;    
    int             iPort = LTREC_PORT_DEFAULT;
    unsigned int    uiTimeOut = LTREC_TIMEOUT_DEFAULT;
    unsigned char   *pucIndexNames = LTREC_INDEX_DEFAULT;
    unsigned char   *pucLanguageCode = LTREC_LANGUAGE_NAME_DEFAULT;
    unsigned char   *pucTopicFilePath = LTREC_TOPIC_FILE_PATH_DEFAULT;
    unsigned char   *pucRunName = LTREC_RUN_NAME_DEFAULT;
    unsigned char   *pucSearchSectionName = LTREC_SEARCH_SECTION_NAME_DEFAULT;
    unsigned char   *pucFeedbackSectionName = NULL;
    unsigned int    uiStartIndex = 0;
    unsigned int    uiEndIndex = LTREC_MAX_DOCS_DEFAULT - 1;
    unsigned int    uiRFDocumentCount = 0;
    unsigned char   *pucSearchOptions = NULL;
    boolean          bDisplaySearchReport = false;

    time_t          tStartTime = (time_t)0;
    unsigned int    uiTimeTaken = 0;
    unsigned int    uiHours = 0;
    unsigned int    uiMinutes = 0;
    unsigned int    uiSeconds = 0;

    unsigned char   *pucLocaleName = LTREC_LOCALE_NAME_DEFAULT;

    unsigned char   *pucLogFilePath = UTL_LOG_FILE_STDERR;    
    unsigned int    uiLogLevel = UTL_LOG_LEVEL_INFO;



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

        /* Check for socket */
        if ( s_strncmp("--socket=", pucNextArgument, s_strlen("--socket=")) == 0 ) {
        
            unsigned char   pucScanfSocket[UTL_STRING_SCANF_FORMAT_LENGTH + 1] = {'\0'};
            int             iStatus = 0;
            unsigned char   pucComponent1[UTL_FILE_PATH_MAX + 1] = {'\0'};
            unsigned char   pucComponent2[UTL_FILE_PATH_MAX + 1] = {'\0'};
            unsigned char   pucComponent3[UTL_FILE_PATH_MAX + 1] = {'\0'};
            unsigned char   *pucProtocolPtr = NULL;
            unsigned char   *pucHostPtr = NULL;
            unsigned char   *pucPortPtr = NULL;
        

            /* Get the socket */
            pucNextArgument += s_strlen("--socket=");


            /* Prepare the socket scan string */
            snprintf(pucScanfSocket, UTL_STRING_SCANF_FORMAT_LENGTH + 1, "%%%d[^:]:%%%d[^:]:%%%ds", UTL_FILE_PATH_MAX + 1, UTL_FILE_PATH_MAX + 1, UTL_FILE_PATH_MAX + 1);

            /* Scan the socket to parse out the protocol, the host and the port */
            if ( (iStatus = sscanf(pucNextArgument, pucScanfSocket, pucComponent1, pucComponent2, pucComponent3)) <= 0 ) {
                vVersion();
                iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to parse the socket argument: '%s'", pucNextArgument);
            }

            /* Check the status */
            if ( iStatus == 3 ) {
                
                /* Set the protocol, host name and port pointers from the components */
                pucProtocolPtr = pucComponent1;
                pucHostPtr = pucComponent2;
                pucPortPtr = pucComponent3;
            }
            else if ( iStatus == 2 ) {
                
                /* Set the protocol or host name, and port pointers from the components */
                if ( (s_strcmp(pucComponent1, UTL_NET_PROTOCOL_TCP_NAME) == 0) || (s_strcmp(pucComponent1, UTL_NET_PROTOCOL_UDP_NAME) == 0) ) {
                    pucProtocolPtr = pucComponent1;
                }
                else {
                    pucHostPtr = pucComponent1;
                }
                pucPortPtr = pucComponent2;
            }
            else if ( iStatus == 1 ) {
                /* Set the port pointer from the component */
                pucPortPtr = pucComponent1;
            }
            
            
            /* Set the fields */
            if ( bUtlStringsIsStringNULL(pucHostPtr) == false ) {
                pucHostName = pucHostPtr;
            }
            if ( (bUtlStringsIsStringNULL(pucProtocolPtr) == false) && (s_strcmp(pucProtocolPtr, UTL_NET_PROTOCOL_TCP_NAME) == 0) ) {
                uiNetProtocolID = UTL_NET_PROTOCOL_TCP_ID;
            }
            else if ( (bUtlStringsIsStringNULL(pucProtocolPtr) == false) && (s_strcmp(pucProtocolPtr, UTL_NET_PROTOCOL_UDP_NAME) == 0) ) {
                uiNetProtocolID = UTL_NET_PROTOCOL_UDP_ID;
            }
            else if ( bUtlStringsIsStringNULL(pucProtocolPtr) == false ) {
                iUtlLogPanic(UTL_LOG_CONTEXT, "Invalid protocol in socket argument: '%s'", pucNextArgument);
            }
            else if ( bUtlStringsIsStringNULL(pucProtocolPtr) == false ) {
                uiNetProtocolID = UTL_NET_PROTOCOL_TCP_ID;
            }

            /* Extract the port */
            iPort = s_strtol(pucPortPtr, NULL, 0);
        }

        /* Check for timeout */
        else if ( s_strncmp("--timeout=", pucNextArgument, s_strlen("--timeout=")) == 0 ) {

            /* Get the timeout */
            pucNextArgument += s_strlen("--timeout=");

            /* Check the timeout */
            if ( s_strtol(pucNextArgument, NULL, 10) < 0 ) {
                vVersion();
                iUtlLogPanic(UTL_LOG_CONTEXT, "Expected the timeout to be greater than or equal to: 0");
            }
            
            /* Set the timeout */
            uiTimeOut = s_strtol(pucNextArgument, NULL, 10);
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

            /* Clean the topic file path */
            if ( (iError = iUtlFileCleanPath(pucNextArgument)) != UTL_NoError ) {
                vVersion();
                iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to clean the topic file path: '%s', utl error: %d", pucNextArgument, iError);
            }

            /* Check that topic file path exists */
            if ( bUtlFilePathExists(pucNextArgument) == false ) {
                vVersion();
                iUtlLogPanic(UTL_LOG_CONTEXT, "The topic file path: '%s', does not exist", pucNextArgument);
            }

            /* Check that the topic file path is a file */
            if ( bUtlFileIsFile(pucNextArgument) == false ) {
                vVersion();
                iUtlLogPanic(UTL_LOG_CONTEXT, "The topic file path: '%s', is not a file", pucNextArgument);
            }

            /* Check that the topic file path can be accessed */
            if ( bUtlFilePathRead(pucNextArgument) == false ) {
                vVersion();
                iUtlLogPanic(UTL_LOG_CONTEXT, "The topic file path: '%s', cannot be accessed", pucNextArgument);
            }

            /* Set the topic file path */
            pucTopicFilePath = pucNextArgument;
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
                pucFeedbackSectionName = LTREC_DEFAULT_FEEDBACK_SECTION_NAME;
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


    /* Check that we have a index name */
    if ( bUtlStringsIsStringNULL(pucIndexNames) == true ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "A index name is required");
    }

    /* Check that we have either a search section name or a feedback section name */
    if ( (bUtlStringsIsStringNULL(pucSearchSectionName) == true) && (bUtlStringsIsStringNULL(pucFeedbackSectionName) == true) ) {
        iUtlLogWarn(UTL_LOG_CONTEXT, "Neither a search section name and/or a feedback section name was specified, defaulting to search section name '%s'.\n", LTREC_SEARCH_SECTION_NAME_DEFAULT);
        pucSearchSectionName = LTREC_SEARCH_SECTION_NAME_DEFAULT;
    }


    /* Start timing */
    tStartTime = s_time(NULL);


    /* Run the searches */
    vLTrecSearchFromTopicFile(uiNetProtocolID, pucHostName, iPort, uiTimeOut, pucIndexNames, pucLanguageCode, uiStartIndex, uiEndIndex, 
            pucTopicFilePath, pucRunName, pucSearchSectionName, pucFeedbackSectionName, uiRFDocumentCount, pucSearchOptions, bDisplaySearchReport);


    /* End timing and print time elapsed */
    uiTimeTaken = s_difftime(s_time(NULL), tStartTime);
    uiHours = uiTimeTaken / (60 * 60);
    uiMinutes = (uiTimeTaken - (uiHours * 60 * 60)) / 60;
    uiSeconds = uiTimeTaken - ((uiTimeTaken / 60) * 60);

    if ( uiHours > 0 ) {
        printf("Time taken: %u hour%s, %u minute%s and %u second%s.\n", 
                uiHours, (uiHours == 1) ? "" : "s", uiMinutes, (uiMinutes == 1) ? "" : "s", uiSeconds, (uiSeconds == 1) ? "" : "s");
    }
    else if ( uiMinutes > 0 ) {
        printf("Time taken:  %u minute%s and %u second%s.\n", 
                uiMinutes, (uiMinutes == 1) ? "" : "s", uiSeconds, (uiSeconds == 1) ? "" : "s");
    }
    else {
        printf("Time taken:  %u second%s.\n", uiSeconds, (uiSeconds == 1) ? "" : "s");
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
    fprintf(stderr, "LWPS TREC Search, %s\n", UTL_VERSION_COPYRIGHT_STRING);


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
    printf("  --socket=protocol:host:port | --socket=protocol:port | --socket=host:port | --socket=port \n");
    printf("                  Socket to connect to, using a protocol, host and port combination, \n");
    printf("                  protocol defaults to: '%s', procotols available: '%s', '%s', \n", 
            LTREC_NET_PROTOCOL_NAME_DEFAULT, UTL_NET_PROTOCOL_TCP_NAME, UTL_NET_PROTOCOL_UDP_NAME);
    printf("                      host defaults to: '%s', port defaults to: %d. \n", LTREC_HOST_DEFAULT, LTREC_PORT_DEFAULT);
    printf("  --timeout=#     Timeout in milliseconds, defaults to: %d milliseconds. \n", LTREC_TIMEOUT_DEFAULT);
    printf("  --index=name[%sname[%s...]] \n", LTREC_INDEX_NAME_SEPARATORS, LTREC_INDEX_NAME_SEPARATORS);
    printf("                  Index names, '%s' delimited, defaults to '%s'. \n", LTREC_INDEX_NAME_SEPARATORS, LTREC_INDEX_DEFAULT);
    printf("  --run=name      Run name, defaults to '%s'. \n", LTREC_RUN_NAME_DEFAULT);
    printf("  --maximum-documents=# \n");
    printf("                  Maximum number of documents to retrieve, defaults to: %d. \n", LTREC_MAX_DOCS_DEFAULT);
    printf("  --topic=name    Topic file path, defaults to '%s'. \n", LTREC_TOPIC_FILE_PATH_DEFAULT);
    printf("  --section=name  Topic file search section name, defaults to '%s'. \n", LTREC_SEARCH_SECTION_NAME_DEFAULT);
    printf("  --feedback=[name|number] \n");
    printf("                  Topic file feedback section name or number of documents to feedback \n");
    printf("                  section name defaults to '%s'. \n", LTREC_DEFAULT_FEEDBACK_SECTION_NAME);
    printf("  --options=name  Search options and modifiers to add to a search, such as '{relaxedboolean}'. \n");
    printf("  --report        Retrieve and display the search report following a search. \n");
    printf("  --language=name \n");
    printf("                  Language code, defaults to '%s'. \n", LTREC_LANGUAGE_NAME_DEFAULT);
    printf("\n");

    printf(" Locale parameter: \n");
    printf("  --locale=name   Locale name, defaults to '%s', see 'locale' for list of \n", LTREC_LOCALE_NAME_DEFAULT);
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

    Function:   vLTrecSearchFromTopicFile()

    Purpose:    This function will read in a trec topic file, parse it out and 
                run each search against the server

    Parameters: uiNetProtocolID         network protocol ID
                pucHostName             host name
                iPort                   port
                uiTimeOut               timeout
                pucIndexNames           comma delimited list of index names
                pucLanguageCode         language code (optional)
                uiStartIndex            start index (optional)
                uiEndIndex              end index, 0 if there is no end index
                pucTopicFilePath        topic file path
                pucRunName              run name
                pucSearchSectionName    section name to use for the search string (optional)
                pucFeedbackSectionName  section name to use for feedback string (optional)
                uiRFDocumentCount       number of entries to feedback (if greater than 0)
                pucSearchOptions        search options (optional)
                bDisplaySearchReport    set to true if the search report is to be displayed

    Globals:    none

    Returns:    void

*/
static void vLTrecSearchFromTopicFile
(
    unsigned int uiNetProtocolID,
    unsigned char *pucHostName,
    int iPort,
    unsigned int uiTimeOut,
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

    int             iError = UTL_NoError;
    void            *pvUtlNet = NULL;
    void            *pvLwps = NULL;
    int             iErrorCode = SPI_NoError;
    unsigned char   *pucErrorString = NULL;
    unsigned char   *pucIndexNamesCopy = NULL;
    unsigned char   *pucIndexNamesCopyStrtokPtr = NULL;
    unsigned char   **ppucIndexNameList = NULL;
    unsigned char   pucLine[LTREC_LONG_LINE_LENGTH + 1] = {'\0'};
    FILE            *pfFile = NULL;
    unsigned int    uiQuestion = 0;
    unsigned int    uiCurrentSection = 0;
    unsigned char   *pucPtr = NULL;
    unsigned char   *pucNum = NULL;
    unsigned char   *pucTitle = NULL;
    unsigned char   *pucDesc = NULL;
    unsigned char   *pucNarr = NULL;
    unsigned char   *pucQues = NULL;
    unsigned char   *pucRF = NULL;
    unsigned char   *pucRFStrtokPtr = NULL;
    unsigned char   *pucSearchPtr = NULL;
    unsigned char   *pucPositiveFeedbackPtr = NULL;
    unsigned char   **ppucRFDocumentIDList = NULL;
    unsigned int    uiI = 0;


    ASSERT((uiNetProtocolID == UTL_NET_PROTOCOL_TCP_ID) || (uiNetProtocolID == UTL_NET_PROTOCOL_UDP_ID));
    ASSERT(bUtlStringsIsStringNULL(pucHostName) == false);
    ASSERT(iPort >= 0);
    ASSERT(uiTimeOut >= 0);
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


    /* Open the topics file */
    if ( (pfFile = s_fopen(pucTopicFilePath, "r")) == NULL ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to open the topic file: '%s'", pucTopicFilePath);
    }


    /* Open a connection to the host */
    if ( (iError = iUtlNetCreateClient(uiNetProtocolID, pucHostName, iPort, uiTimeOut, &pvUtlNet)) != UTL_NoError ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to open a net client connection, network protocol: %u, host: '%s', port: %d, utl error: %d", 
                uiNetProtocolID, pucHostName, iPort, iError);
    }

    /* Create a LPWS handle */
    if ( (iError = iLwpsCreate(pvUtlNet, &pvLwps)) != LWPS_NoError ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to create an lwps, lwps error: %d.", iError);
    }

    /* Send the init */
    if ( (iError = iLwpsInitRequestHandle(pvLwps, NULL, NULL, &iErrorCode, &pucErrorString)) != LWPS_NoError) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to handle an lwps init request with: '%s', lwps error: %d.", pucHostName, iError);
    }

    /* Check the returned error code */
    if ( iErrorCode != SPI_NoError) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to handle an lwps search request with: '%s', error code: %d, error text: '%s'.", 
                pucHostName, iError, pucUtlStringsGetPrintableString(pucErrorString));
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
        for ( pucPtr = s_strtok_r(pucIndexNamesCopy, LTREC_INDEX_NAME_SEPARATORS, (char **)&pucIndexNamesCopyStrtokPtr), uiI = 0; 
                pucPtr != NULL; 
                pucPtr = s_strtok_r(NULL, LTREC_INDEX_NAME_SEPARATORS, (char **)&pucIndexNamesCopyStrtokPtr), uiI++ ) {

            /* Add the index name to the index name list */
            if ( (ppucIndexNameList = (unsigned char **)s_realloc(ppucIndexNameList, (size_t)(sizeof(unsigned char *) * (uiI + 2)))) == NULL ) {
                iUtlLogPanic(UTL_LOG_CONTEXT, "Memory allocation error");
            }
            if ( (ppucIndexNameList[uiI] = (unsigned char *)s_strdup(pucPtr)) == NULL ) {
                iUtlLogPanic(UTL_LOG_CONTEXT, "Memory allocation error");
            }
        }

        /* NULL terminate the index name list */
        ppucIndexNameList[uiI + 1] = NULL;

        /* Free the index name copy */
        s_free(pucIndexNamesCopy);
    }



    /* Loop reading in all the lines */
    while ( s_fgets(pucLine, LTREC_LONG_LINE_LENGTH, pfFile) != NULL ) {

        /* Trim string */
        iUtlStringsTrimString(pucLine);

        /* Have we hit a new topic? */
        if ( s_strncasecmp(pucLine, "<top>", 5) == 0 ) {

            /* Loop reading in the lines for this topic */
            while ( s_fgets(pucLine, LTREC_LONG_LINE_LENGTH, pfFile) != NULL ) {

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
                        iUtlStringsAppendString(pucRF,  pucPtr, &pucRF);
                        iUtlStringsAppendString(pucRF,  " ", &pucRF);
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
                        for ( pucPtr = s_strtok_r(pucRF, LTREC_RELEVANCE_FEEDBACK_SEPARATORS, (char **)&pucRFStrtokPtr), uiI = 0;
                                pucPtr != NULL;
                                pucPtr = s_strtok_r(NULL, LTREC_RELEVANCE_FEEDBACK_SEPARATORS, (char **)&pucRFStrtokPtr), uiI++ ) {

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

                    /* Set the search search pointer */
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
                    vLTrecSearch(pvLwps, ppucIndexNameList, pucLanguageCode, pucSearchPtr, pucPositiveFeedbackPtr, NULL, 
                            uiStartIndex, uiEndIndex, pucRunName,  uiQuestion, ppucRFDocumentIDList, uiRFDocumentCount, bDisplaySearchReport);


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
                    UTL_MACROS_FREE_NULL_TERMINATED_LIST(ppucRFDocumentIDList);

                    /* And break */
                    break;

                }
            }
        }
    }


    /* Free the index list */
    UTL_MACROS_FREE_NULL_TERMINATED_LIST(ppucIndexNameList);


    /* Free the lwps structure */
    iLwpsFree(pvLwps);
    pvLwps = NULL;

    /* Free the net structure */
    iUtlNetFree(pvUtlNet);
    pvUtlNet = NULL;


    return;

}


/*---------------------------------------------------------------------------*/


/*

    Function:   vLTrecSearch()

    Purpose:    This function is used to access the server

    Parameters: pvLwps                      lwps structure
                ppucIndexNameList           index name list
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
static void vLTrecSearch
(
    void *pvLwps,
    unsigned char **ppucIndexNameList,
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

    int                         iError = LWPS_NoError;
    int                         iErrorCode = SPI_NoError;
    unsigned char               *pucErrorString = NULL;
    unsigned char               *pucPtr = NULL;

    unsigned char               *pucPositiveFeedback = NULL;

    struct spiSearchResponse    *pssrSpiSearchResponse = NULL;
    struct spiSearchResult      *pssrSpiSearchResultsPtr = NULL;
    unsigned int                uiI = 0, uiJ = 0;

    struct spiDocumentItem      *psdiSpiDocumentItemsPtr = NULL;

    unsigned int                uiWeight = 0;

    void                        *pvData = NULL;
    unsigned int                uiDataLength = 0;

    unsigned int                uiSearchReportIndex = 0;


    ASSERT(pvLwps != NULL);
    ASSERT(ppucIndexNameList != NULL);
    ASSERT((bUtlStringsIsStringNULL(pucLanguageCode) == false) || (pucLanguageCode == NULL));
    ASSERT((bUtlStringsIsStringNULL(pucSearchText) == false) || (pucSearchText == NULL));
    ASSERT((bUtlStringsIsStringNULL(pucPositiveFeedbackText) == false) || (pucPositiveFeedbackText == NULL));
    ASSERT((bUtlStringsIsStringNULL(pucNegativeFeedbackText) == false) || (pucNegativeFeedbackText == NULL));
    ASSERT(uiStartIndex >= 0);
    ASSERT(uiEndIndex >= 0);
    ASSERT(uiEndIndex >= uiStartIndex);
    ASSERT(bUtlStringsIsStringNULL(pucRunName) == false);
    ASSERT(uiQuestion >= 0);
    ASSERT(uiRFDocumentCount >= 0);
    ASSERT((ppucRFDocumentIDList == NULL) || (ppucRFDocumentIDList != NULL));
    ASSERT((bDisplaySearchReport == true) || (bDisplaySearchReport == false));



    /* Submit a search */
    if ( (iError = iLwpsSearchRequestHandle(pvLwps, ppucIndexNameList, pucLanguageCode, pucSearchText, pucPositiveFeedbackText, pucNegativeFeedbackText, 
            uiStartIndex, uiEndIndex, &pssrSpiSearchResponse, &iErrorCode, &pucErrorString)) != LWPS_NoError ) {

        /* Search failed to run */
        iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to handle an lwps search request, lwps error: %d", iError);
    }
    else {
        /* Check the returned error code */
        if ( iErrorCode != SPI_NoError ) {
            iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to handle an lwps search request, return error code: %d, error text: '%s'", iError, pucUtlStringsGetPrintableString(pucErrorString));
        }
    }


    /* Add documents as relevance feedback if needed */
    if ( (uiRFDocumentCount > 0) || (ppucRFDocumentIDList != NULL) ) {

        if ( uiRFDocumentCount > 0 ) {
            for ( uiI = 1, pssrSpiSearchResultsPtr = pssrSpiSearchResponse->pssrSpiSearchResults; 
                    (uiI <= uiRFDocumentCount) && (uiI <= pssrSpiSearchResponse->uiSpiSearchResultsLength); 
                    uiI++, pssrSpiSearchResultsPtr++ ) {

                /* Download the document */
                if ( (iError = iLwpsRetrievalRequestHandle(pvLwps, pssrSpiSearchResultsPtr->pucIndexName, pssrSpiSearchResultsPtr->pucDocumentKey, 
                        "document", "text/plain", SPI_CHUNK_TYPE_DOCUMENT, 0, 0, &pvData, &uiDataLength, &iErrorCode, &pucErrorString)) != LWPS_NoError ) {

                    iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to handle an lwps retrieval request, lwps error: %d", iError);
                }
                else {

                    /* Check the returned error code */
                    if ( iErrorCode != SPI_NoError ) {
                        iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to handle an lwps retrieval request, returned error code: %d, error text: '%s'", iError, pucUtlStringsGetPrintableString(pucErrorString));
                    }
                    else {
                        fprintf(stderr, "Adding document: %u as RF.\n", uiI);

                        /* Append the data to the string */
                        iUtlStringsAppendData(pucPositiveFeedback, pvData, uiDataLength, &pucPositiveFeedback);

                        /* Free the data */
                        s_free(pvData);
                    }
                }
            }
        }
        else if ( ppucRFDocumentIDList != NULL ) {

            /* Loop over each entry in the RF list */
            for ( uiI = 0; ppucRFDocumentIDList[uiI] != NULL; uiI++ ) {

                boolean bGotIt = false;

                /* Loop over each index */
                for ( uiJ = 0; ppucIndexNameList[uiJ] != NULL; uiJ++ ) {

                    /* Download the document */
                    if ( (iError = iLwpsRetrievalRequestHandle(pvLwps, ppucIndexNameList[uiJ], ppucRFDocumentIDList[uiI], "document", "text/plain", 
                            SPI_CHUNK_TYPE_DOCUMENT, 0, 0, &pvData, &uiDataLength, &iErrorCode, &pucErrorString)) != LWPS_NoError ) {

                        iUtlLogWarn(UTL_LOG_CONTEXT, "Failed to handle an lwps retrieval request, lwps error: %d", iError);
                    }
                    else {

                        /* Free the error string if it was returned */
                        s_free(pucErrorString);

                        /* Check the returned error code */
                        if ( iErrorCode == SPI_NoError ) {
                            bGotIt = true;
                            break;
                        }
                    }
                    
                }

                /* Process the document if we got it, otherwise we report an error */
                if ( bGotIt == true ) {
                    fprintf(stderr, "Adding document ID: '%s' as RF.\n", ppucRFDocumentIDList[uiI]);

                    /* Append the data to the string */
                    iUtlStringsAppendData(pucPositiveFeedback, pvData, uiDataLength, &pucPositiveFeedback);

                    /* Free the document */
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


        /* Submit a search */
        if ( (iError = iLwpsSearchRequestHandle(pvLwps, ppucIndexNameList, pucLanguageCode, pucSearchText, pucPositiveFeedback, NULL, 
                uiStartIndex, uiEndIndex, &pssrSpiSearchResponse, &iErrorCode, &pucErrorString)) != LWPS_NoError ) {

            /* Search failed to run */
            iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to handle an lwps search request, lwps error: %d", iError);
        }
        else {
            /* Check the returned error code */
            if ( iErrorCode != SPI_NoError ) {
                iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to handle an lwps search request, returned error code: %d, error text: '%s'", iError, pucUtlStringsGetPrintableString(pucErrorString));
            }
        }
    }



    fprintf(stderr, "Search Response - Number of Records Returned: %u\n", pssrSpiSearchResponse->uiSpiSearchResultsLength);


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
                                LTREC_DEFAULT_MIN_WEIGHT : (((double)pssrSpiSearchResultsPtr->dSortKey / pssrSpiSearchResponse->dMaxSortKey) * LTREC_DEFAULT_MAX_WEIGHT);
                        break;
        
                    case SPI_SORT_TYPE_FLOAT_ASC:
                    case SPI_SORT_TYPE_FLOAT_DESC:
                        uiWeight = (pssrSpiSearchResultsPtr->fSortKey <= SPI_SORT_KEY_FLOAT_MINIMUM) ? 
                                LTREC_DEFAULT_MIN_WEIGHT : (((double)pssrSpiSearchResultsPtr->fSortKey / pssrSpiSearchResponse->dMaxSortKey) * LTREC_DEFAULT_MAX_WEIGHT);
                        break;
                    
                    case SPI_SORT_TYPE_UINT_ASC:
                    case SPI_SORT_TYPE_UINT_DESC:
                        uiWeight = (pssrSpiSearchResultsPtr->uiSortKey <= SPI_SORT_KEY_UINT_MINIMUM) ? 
                                LTREC_DEFAULT_MIN_WEIGHT : (((double)pssrSpiSearchResultsPtr->uiSortKey / pssrSpiSearchResponse->dMaxSortKey) * LTREC_DEFAULT_MAX_WEIGHT);
                        break;
                    
                    case SPI_SORT_TYPE_ULONG_ASC:
                    case SPI_SORT_TYPE_ULONG_DESC:
                        uiWeight = (pssrSpiSearchResultsPtr->ulSortKey <= SPI_SORT_KEY_ULONG_MINIMUM) ?
                                LTREC_DEFAULT_MIN_WEIGHT : (((double)pssrSpiSearchResultsPtr->ulSortKey / pssrSpiSearchResponse->dMaxSortKey) * LTREC_DEFAULT_MAX_WEIGHT);
                        break;
                    
                    case SPI_SORT_TYPE_UCHAR_ASC:
                    case SPI_SORT_TYPE_UCHAR_DESC:
                        uiWeight = LTREC_DEFAULT_MIN_WEIGHT;
                        break;
                    
                    case SPI_SORT_TYPE_NO_SORT:
                        uiWeight = LTREC_DEFAULT_MIN_WEIGHT;
                        break;
        
                    case SPI_SORT_TYPE_UNKNOWN:
                        uiWeight = LTREC_DEFAULT_MIN_WEIGHT;
                        break;
                    
                    default:
                        uiWeight = LTREC_DEFAULT_MIN_WEIGHT;
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


        /* Download the search report document */
        if ( (iError = iLwpsRetrievalRequestHandle(pvLwps, pssrSpiSearchResultsPtr->pucIndexName, pssrSpiSearchResultsPtr->pucDocumentKey, 
                psdiSpiDocumentItemsPtr->pucItemName, psdiSpiDocumentItemsPtr->pucMimeType, SPI_CHUNK_TYPE_DOCUMENT, 0, 0, &pvData, &uiDataLength, 
                &iErrorCode, &pucErrorString)) != LWPS_NoError ) {

            iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to handle an lwps retrieval request, lwps error: %d", iError);
        }
        else {

            /* Check the returned error code */
            if ( iErrorCode != SPI_NoError ) {
                iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to handle an lwps retrieval request, returned error code: %d, error text: '%s'", 
                        iErrorCode, pucUtlStringsGetPrintableString(pucErrorString));
            }
            else {

                /* NULL terminate the data */
                iUtlStringsNullTerminateData(pvData, uiDataLength, (unsigned char **)&pvData);

                /* Print the data */
                fprintf(stderr, "\n%s\n\n\n", (unsigned char *)pvData);
                s_free(pvData);

                /* Reset the error */
                iError = SPI_NoError;
                iErrorCode = SPI_NoError;
            }
        }
    }            

    /* Free the search response */
    iSpiFreeSearchResponse(pssrSpiSearchResponse);
    pssrSpiSearchResponse = NULL;

    /* Free the feedback text */
    s_free(pucPositiveFeedback);


    return;
}


/*---------------------------------------------------------------------------*/

