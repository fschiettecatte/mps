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

    Module:     lscript.c

    Author:     Francois Schiettecatte (FS Consulting LLC.)

    Created:    30 March 1999

    Purpose:    This is scriptable application allows us
                to test the LWPS protocol.

*/


/*---------------------------------------------------------------------------*/


/*
** Includes
*/

#include "utils.h"

#include "rep.h"

#include "lwps.h"

#include "lng.h"

#include "spi.h"


/*---------------------------------------------------------------------------*/


/*
** Feature defines
*/

/* Context for logging */
#undef UTL_LOG_CONTEXT
#define UTL_LOG_CONTEXT                     (unsigned char *)"com.fsconsult.mps.src.clients.lwps.lscript"


/*---------------------------------------------------------------------------*/


/*
** Defines
*/

/* Line lengths */
#define LSCRIPT_LONG_LINE_LENGTH                        (51200)
#define LSCRIPT_SHORT_LINE_LENGTH                       (1024)


/* Timeout in milliseconds */
#define LSCRIPT_TIMEOUT_DEFAULT                         (LWPS_PROTOCOL_TIMEOUT_DEFAULT)

/* Default interval to wait between starting each thread in seconds */
#define LSCRIPT_THREAD_SLEEP_INTERVAL_DEFAULT           (30)


/* Index name separators */
#define LSCRIPT_INDEX_NAME_SEPARATORS                   (unsigned char *)", "


/* Action defines */
#define LSCRIPT_ACTION_EXIT                             (0)
#define LSCRIPT_ACTION_OPEN_CONNECTION                  (1)
#define LSCRIPT_ACTION_CLOSE_CONNECTION                 (2)
#define LSCRIPT_ACTION_LANGUAGE                         (3)
#define LSCRIPT_ACTION_OPEN_INDEX                       (10)
#define LSCRIPT_ACTION_CLOSE_INDEX                      (11)
#define LSCRIPT_ACTION_SEARCH                           (20)
#define LSCRIPT_ACTION_SEARCH_LIST                      (21)
#define LSCRIPT_ACTION_SEARCH_OFFSETS                   (22)
#define LSCRIPT_ACTION_SEARCH_REPORT                    (23)
#define LSCRIPT_ACTION_RETRIEVE_DOCUMENT                (30)
#define LSCRIPT_ACTION_RETRIEVE_DOCUMENT_BYTES          (31)
#define LSCRIPT_ACTION_SAVE_DOCUMENT                    (32)
#define LSCRIPT_ACTION_ADD_POSITIVE_FEEDBACK            (40)
#define LSCRIPT_ACTION_ADD_POSITIVE_FEEDBACK_TEXT       (41)
#define LSCRIPT_ACTION_CLEAR_POSITIVE_FEEDBACK          (42)
#define LSCRIPT_ACTION_ADD_NEGATIVE_FEEDBACK            (43)
#define LSCRIPT_ACTION_ADD_NEGATIVE_FEEDBACK_TEXT       (44)
#define LSCRIPT_ACTION_CLEAR_NEGATIVE_FEEDBACK          (45)
#define LSCRIPT_ACTION_GET_SERVER_INFO                  (50)
#define LSCRIPT_ACTION_GET_SERVER_INDEX_INFO            (51)
#define LSCRIPT_ACTION_GET_INDEX_INFO                   (52)
#define LSCRIPT_ACTION_GET_INDEX_FIELD_INFO             (53)
#define LSCRIPT_ACTION_GET_INDEX_TERM_INFO              (54)
#define LSCRIPT_ACTION_GET_DOCUMENT_INFO                (55)
#define LSCRIPT_ACTION_SLEEP                            (60)
#define LSCRIPT_ACTION_SKIP                             (70)
#define LSCRIPT_ACTION_RESUME                           (71)


/* Default locale name */
#define LSCRIPT_LOCALE_NAME_DEFAULT                     LNG_LOCALE_EN_US_UTF_8_NAME


/*---------------------------------------------------------------------------*/


/*
** Structures
*/

/* Script structure */
struct lScript {
    unsigned char       *pucScriptFilePath;
    unsigned int        uiTimeout;
    boolean             bCheckOnly;
};


/* Script action structure */
struct lScriptAction {
    int                 iAction;
    unsigned char       *pucActionText;
    unsigned char       *pucActionSyntax;
};


/*---------------------------------------------------------------------------*/


/*
** Globals
*/

/* Actions */
static struct lScriptAction plsaLScriptActionsGlobal[] = 
{
    {LSCRIPT_ACTION_EXIT,                           (unsigned char *)"exit",                    (unsigned char *)"exit"}, 
    {LSCRIPT_ACTION_OPEN_CONNECTION,                (unsigned char *)"openConnection",          (unsigned char *)"openConnection protocol{A} hostName{A} hostPort{N}"}, 
    {LSCRIPT_ACTION_CLOSE_CONNECTION,               (unsigned char *)"closeConnection",         (unsigned char *)"closeConnection"}, 
    {LSCRIPT_ACTION_LANGUAGE,                       (unsigned char *)"language",                (unsigned char *)"language languageCode{A}"}, 
    {LSCRIPT_ACTION_OPEN_INDEX,                     (unsigned char *)"openIndex",               (unsigned char *)"openIndex indexName{A}[,...]"}, 
    {LSCRIPT_ACTION_CLOSE_INDEX,                    (unsigned char *)"closeIndex",              (unsigned char *)"closeIndex"}, 
    {LSCRIPT_ACTION_SEARCH,                         (unsigned char *)"search",                  (unsigned char *)"search searchText{A}"}, 
    {LSCRIPT_ACTION_SEARCH_LIST,                    (unsigned char *)"searchList",              (unsigned char *)"searchList searchText{A}"}, 
    {LSCRIPT_ACTION_SEARCH_OFFSETS,                 (unsigned char *)"searchOffsets",           (unsigned char *)"searchOffsets start{N} end{N}"}, 
    {LSCRIPT_ACTION_SEARCH_REPORT,                  (unsigned char *)"searchReport",            (unsigned char *)"searchReport raw/formatted{N}"}, 
    {LSCRIPT_ACTION_RETRIEVE_DOCUMENT,              (unsigned char *)"retrieveDocument",        (unsigned char *)"retrieveDocument indexName{A} [documentKey{A}] itemName{A} mimeType{A}"}, 
    {LSCRIPT_ACTION_RETRIEVE_DOCUMENT_BYTES,        (unsigned char *)"retrieveDocumentBytes",   (unsigned char *)"retrieveDocumentBytes indexName{A} [documentKey{A}] itemName{A} mimeType{A} start{N} end{N}"}, 
    {LSCRIPT_ACTION_SAVE_DOCUMENT,                  (unsigned char *)"saveDocument",            (unsigned char *)"saveDocument indexName{A} [documentKey{A}] itemName{A} mimeType{A} filename{A}"}, 
    {LSCRIPT_ACTION_ADD_POSITIVE_FEEDBACK,          (unsigned char *)"addPositiveFeedback",     (unsigned char *)"addPositiveFeedback indexName{A} [documentKey{A}] itemName{A} mimeType{A}"}, 
    {LSCRIPT_ACTION_ADD_POSITIVE_FEEDBACK_TEXT,     (unsigned char *)"addPositiveFeedbackText", (unsigned char *)"addPositiveFeedbackText feedbackText{A}"}, 
    {LSCRIPT_ACTION_CLEAR_POSITIVE_FEEDBACK,        (unsigned char *)"clearPositiveFeedback",   (unsigned char *)"clearPositiveFeedback"}, 
    {LSCRIPT_ACTION_ADD_NEGATIVE_FEEDBACK,          (unsigned char *)"addNegativeFeedback",     (unsigned char *)"addNegativeFeedback indexName{A} [documentKey{A}] itemName{A} mimeType{A}"}, 
    {LSCRIPT_ACTION_ADD_NEGATIVE_FEEDBACK_TEXT,     (unsigned char *)"addNegativeFeedbackText", (unsigned char *)"addNegativeFeedbackText feedbackText{A}"}, 
    {LSCRIPT_ACTION_CLEAR_NEGATIVE_FEEDBACK,        (unsigned char *)"clearNegativeFeedback",   (unsigned char *)"clearNegativeFeedback"}, 
    {LSCRIPT_ACTION_GET_SERVER_INFO,                (unsigned char *)"getServerInfo",           (unsigned char *)"getServerInfo"}, 
    {LSCRIPT_ACTION_GET_SERVER_INDEX_INFO,          (unsigned char *)"getServerIndexInfo",      (unsigned char *)"getServerIndexInfo"}, 
    {LSCRIPT_ACTION_GET_INDEX_INFO,                 (unsigned char *)"getIndexInfo",            (unsigned char *)"getIndexInfo indexName{A}"}, 
    {LSCRIPT_ACTION_GET_INDEX_FIELD_INFO,           (unsigned char *)"getIndexFieldInfo",       (unsigned char *)"getIndexFieldInfo indexName{A}"}, 
    {LSCRIPT_ACTION_GET_INDEX_TERM_INFO,            (unsigned char *)"getIndexTermInfo",        (unsigned char *)"getIndexTermInfo indexName{A} regular|stop|wildcard|soundex|metaphone|phonix|typo{A} sensitive|insensitive{A} (term{A}) (fieldName{A})"}, 
    {LSCRIPT_ACTION_GET_DOCUMENT_INFO,              (unsigned char *)"getDocumentInfo",         (unsigned char *)"getDocumentInfo indexName{A} [documentKey{A}]"}, 
    {LSCRIPT_ACTION_SLEEP,                          (unsigned char *)"sleep",                   (unsigned char *)"sleep seconds{N}"}, 
    {LSCRIPT_ACTION_SKIP,                           (unsigned char *)"skip",                    (unsigned char *)"skip"}, 
    {LSCRIPT_ACTION_RESUME,                         (unsigned char *)"resume",                  (unsigned char *)"resume"}, 
    {-1,                                            NULL,                                       NULL}
};


/*---------------------------------------------------------------------------*/


/*
** Private function prototypes
*/

static void vVersion (void);
static void vUsage (unsigned char *pucCommandPath);

static void vLScriptPrintSyntax (void);

static void vLScriptRunScript (struct lScript *plslScript);


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

    unsigned char   pucScriptFilePath[UTL_FILE_PATH_MAX + 1] = {'\0'};
    unsigned int    uiThreadCount = 0;
    unsigned int    uiSleepInterval = LSCRIPT_THREAD_SLEEP_INTERVAL_DEFAULT;
    unsigned int    uiTimeout = LSCRIPT_TIMEOUT_DEFAULT;
    boolean         bCheckOnly = false;
    
    unsigned char   *pucLocaleName = LSCRIPT_LOCALE_NAME_DEFAULT;

    unsigned char   *pucLogFilePath = UTL_LOG_FILE_STDERR;
    unsigned int    uiLogLevel = UTL_LOG_LEVEL_INFO;

    struct lScript  lslScript;



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

        /* Check for script file path */
        if ( s_strncmp("--script=", pucNextArgument, s_strlen("--script=")) == 0 ) { 

            /* Get the script file path */
            pucNextArgument += s_strlen("--script=");

            /* Get the true script file path */
            if ( (iError = iUtlFileGetTruePath(pucNextArgument, pucScriptFilePath, UTL_FILE_PATH_MAX + 1)) != UTL_NoError ) {
                vVersion();
                iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to get the true script file path: '%s', utl error: %d", pucNextArgument, iError);
            }

            /* Check that script file path exists */
            if ( bUtlFilePathExists(pucNextArgument) == false ) {
                vVersion();
                iUtlLogPanic(UTL_LOG_CONTEXT, "The script file path: '%s', does not exist", pucNextArgument);
            }

            /* Check that the script file path is a file */
            if ( bUtlFileIsFile(pucNextArgument) == false ) {
                vVersion();
                iUtlLogPanic(UTL_LOG_CONTEXT, "The script file path: '%s', is not a file", pucNextArgument);
            }

            /* Check that the script file path can be accessed */
            if ( bUtlFilePathRead(pucNextArgument) == false ) {
                vVersion();
                iUtlLogPanic(UTL_LOG_CONTEXT, "The script file path: '%s', cannot be accessed", pucNextArgument);
            }
        }
        
        /* Check for thread count */
        else if ( s_strncmp("--threads=", pucNextArgument, s_strlen("--threads=")) == 0 ) {

            /* Get the thread count */
            pucNextArgument += s_strlen("--threads=");

            /* Check the thread count */
            if ( s_strtol(pucNextArgument, NULL, 10) <= 0 ) {
                vVersion();
                iUtlLogPanic(UTL_LOG_CONTEXT, "Expected the thread count to be greater than: 0");
            }
            
            /* Set the thread count */
            uiThreadCount = s_strtol(pucNextArgument, NULL, 10);
        }

        /* Check for sleep interval */
        else if ( s_strncmp("--interval=", pucNextArgument, s_strlen("--interval=")) == 0 ) {

            /* Get the sleep interval */
            pucNextArgument += s_strlen("--interval=");

            /* Check the sleep interval */
            if ( s_strtol(pucNextArgument, NULL, 10) < 0 ) {
                vVersion();
                iUtlLogPanic(UTL_LOG_CONTEXT, "Expected the sleep interval to be greater than or equal to: 0");
            }
            
            /* Set the sleep interval */
            uiSleepInterval = s_strtol(pucNextArgument, NULL, 10);
        }

        /* Check for timeout */
        else if ( s_strncmp("--timeout=", pucNextArgument, s_strlen("--timeout=")) == 0 ) {

            /* Get the timeout */
            pucNextArgument += s_strlen("--timeout=");

            /* Check the timeout */
            if ( s_strtol(pucNextArgument, NULL, 10) <= 0 ) {
                vVersion();
                iUtlLogPanic(UTL_LOG_CONTEXT, "Expected the timeout to be greater than: 0");
            }
            
            /* Set the timeout */
            uiTimeout = s_strtol(pucNextArgument, NULL, 10);
        }

        /* Check for check */
        else if ( s_strcmp("--check", pucNextArgument) == 0 ) {
            
            /* Set the check flag */
            bCheckOnly = true;
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
            vLScriptPrintSyntax();
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


    /* Set the lscript structure parameters */
    lslScript.pucScriptFilePath = pucScriptFilePath;
    lslScript.uiTimeout = uiTimeout;
    lslScript.bCheckOnly = bCheckOnly;


    /* Threaded script */
    if ( uiThreadCount > 1 ) {
    
        pthread_t        *ptThreads = NULL;
        int                iStatus = 0;
        unsigned int    uiI = 0;

        /* Allocate memory for the threads array */
        if ( (ptThreads = (pthread_t *)s_malloc((size_t)(sizeof(pthread_t) * uiThreadCount))) == NULL ) {
            iUtlLogPanic(UTL_LOG_CONTEXT, "Memory allocation error");
        }

        /* Loop, creating the threads */
        for ( uiI = 0; uiI < uiThreadCount; uiI++ ) {
            
            /* Kick off the thread to run the script */
            if ( (iStatus = s_pthread_create(&ptThreads[uiI], NULL, (void *)vLScriptRunScript, (void *)&lslScript)) != 0 ) {
                iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to create a thread");
            }
            
            /* Sleep for the required interval, unless we have hit the last thread */
            if ( uiI < (uiThreadCount - 1) ) {
                if ( uiSleepInterval > 0 ) {
                    s_sleep(uiSleepInterval);
                }
            }
        }

        /* Loop, joining the threads */
        for ( uiI = 0; uiI < uiThreadCount; uiI++ ) {
            iStatus = s_pthread_join(ptThreads[uiI], NULL);
        }
        
        /* Free the thread array */
        s_free(ptThreads);
    }

    /* Unthreaded script or we are just checking */
    else {

        /* Call the main function */
        vLScriptRunScript(&lslScript);
    }



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
    printf("LWPS Script, %s\n", UTL_VERSION_COPYRIGHT_STRING);


    /* Get the version string */
    iUtlVersionGetVersionString(pucVersionString, UTL_VERSION_STRING_LENGTH + 1);

    /* Version message */
    printf("%s\n", pucVersionString);


    return;

}


/*---------------------------------------------------------------------------*/


/*

    Function:   vUsage()

    Purpose:    This function list out all the parameters that lscript
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
    printf("  --script=name   Script file path, defaults to 'stdin' if not specified. \n");
    printf("  --threads=#     Number of threads to run, default is not to create any threads. \n");
    printf("  --interval=#    Interval in seconds to wait before starting each thread, default: %d seconds. \n", LSCRIPT_THREAD_SLEEP_INTERVAL_DEFAULT);
    printf("  --timeout=#     LWPS timeout in milliseconds, default: %d milliseconds. \n", LSCRIPT_TIMEOUT_DEFAULT);
    printf("  --check         Parses and checks the script file, reporting errors, but does not execute it. \n");
    printf("\n");

    printf(" Locale parameters: \n");
    printf("  --locale=name   Locale name, defaults to '%s', see 'locale' for list of \n", LSCRIPT_LOCALE_NAME_DEFAULT);
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

    Function:   vLScriptPrintSyntax()

    Purpose:    Print out the syntax that is acceptable

    Parameters: void

    Globals:    none

    Returns:    void

*/
static void vLScriptPrintSyntax
(

)
{

    struct lScriptAction    *plsaLScriptActionsPtr = NULL;


    /* Print all the actions */
    for ( plsaLScriptActionsPtr = plsaLScriptActionsGlobal; plsaLScriptActionsPtr->iAction != -1; plsaLScriptActionsPtr++ ) {
        printf("%s\n", plsaLScriptActionsPtr->pucActionSyntax);
    }


    return;

}


/*---------------------------------------------------------------------------*/


/*

    Function:   vLScriptRunScript()

    Purpose:    main function of this program

    Parameters: plslScript  lscript structure

    Globals:    none

    Returns:    void

*/
static void vLScriptRunScript
(
    struct lScript *plslScript
)
{

    int                         iError = LWPS_NoError;
    int                         iErrorCode = SPI_NoError;
    unsigned char               *pucErrorString = NULL;
    
    FILE                        *pfScriptFile = NULL;

    unsigned char               pucLine[LSCRIPT_LONG_LINE_LENGTH + 1] = {'\0'};
    unsigned int                uiLineLength = 0;
    unsigned char               *pucLineEndPtr = NULL;
    boolean                     bEoF = false;

    struct lScriptAction        *plsaLScriptActionsPtr = NULL;
    unsigned int                uiI = 0, uiJ = 0;
    unsigned char               *pucPtr = NULL;
    unsigned char               pucAction[LSCRIPT_SHORT_LINE_LENGTH + 1] = {'\0'};
    unsigned char               pucScanfFormat[UTL_STRING_SCANF_FORMAT_LENGTH + 1] = {'\0'};
    int                         iStatus = 0;
    
    boolean                     bSkip = false;

    unsigned char               pucSearchReport[UTL_FILE_PATH_MAX + 1] = {'\0'};

    unsigned char               pucProtocol[LSCRIPT_SHORT_LINE_LENGTH + 1] = {'\0'};
    unsigned int                uiNetProtocolID = UTL_NET_PROTOCOL_INVALID_ID;
    unsigned char               pucHostName[MAXHOSTNAMELEN + 1] = {'\0'};
    int                         iPort = LWPS_PROTOCOL_PORT_DEFAULT;
    void                        *pvUtlNet = NULL;
    void                        *pvLwps = NULL;
    unsigned char               *pucUserName = NULL;
    unsigned char               *pucPassword = NULL;
    
    int                         iSleep = 0;

    unsigned char               pucIndexName[SPI_INDEX_NAME_MAXIMUM_LENGTH + 1] = {'\0'};
    unsigned char               *pucIndexNameStrtokPtr = NULL;
    unsigned char               **ppucIndexNameList = NULL;

    unsigned char               pucLanguageCode[UTL_FILE_PATH_MAX + 1] = {'\0'};
    unsigned char               *pucSearchText = NULL;
    unsigned char               *pucPositiveFeedbackText = NULL;
    unsigned char               *pucNegativeFeedbackText = NULL;
    unsigned int                uiStartIndex = 0;
    unsigned int                uiEndIndex = 0;
    struct spiSearchResponse    *pssrSpiSearchResponse = NULL;
    struct spiSearchResult      *pssrSpiSearchResultsPtr = NULL;

    unsigned char               pucDocumentKey[SPI_DOCUMENT_KEY_MAXIMUM_LENGTH + 1] = {'\0'};
    unsigned char               pucItemName[SPI_ITEM_NAME_MAXIMUM_LENGTH + 1] = {'\0'};
    unsigned char               pucMimeType[SPI_MIME_TYPE_MAXIMUM_LENGTH + 1] = {'\0'};
    unsigned int                uiDataLength = 0;
    void                        *pvData = 0;
    unsigned char               *pucData = NULL;
    unsigned char               pucFilePath[UTL_FILE_PATH_MAX + 1] = {'\0'};
    FILE                        *pfFile = NULL;
    unsigned int                uiChunkStart = 0;
    unsigned int                uiChunkEnd = 0;

    struct spiServerInfo        *pssiSpiServerInfo = NULL;
    struct spiServerIndexInfo   *pssiiSpiServerIndexInfos = NULL;
    struct spiServerIndexInfo   *pssiiSpiServerIndexInfosPtr = NULL;
    unsigned int                uiSpiServerIndexInfosLength = 0;
    struct spiIndexInfo         *psiiSpiIndexInfo = NULL;
    struct spiFieldInfo         *psfiSpiFieldInfos = NULL;
    struct spiFieldInfo         *psfiSpiFieldInfosPtr = NULL;
    unsigned int                uiSpiFieldInfosLength = 0;

    struct spiDocumentItem      *psdiSpiDocumentItemsPtr = NULL;
    unsigned char               pucDocItems[LSCRIPT_SHORT_LINE_LENGTH + 1] = {'\0'};
    unsigned char               pucNum[LSCRIPT_SHORT_LINE_LENGTH + 1] = {'\0'};

    struct spiTermInfo          *pstiSpiTermInfos = NULL;
    struct spiTermInfo          *pstiSpiTermInfosPtr = NULL;
    unsigned int                uiSpiTermInfosLength = 0;
    unsigned char               pucTerm[UTL_FILE_PATH_MAX + 1] = {'\0'};
    unsigned char               pucTermMatch[UTL_FILE_PATH_MAX + 1] = {'\0'};
    unsigned int                uiTermMatch = SPI_TERM_MATCH_UNKNOWN;
    unsigned char               pucTermCase[UTL_FILE_PATH_MAX + 1] = {'\0'};
    unsigned int                uiTermCase = SPI_TERM_CASE_UNKNOWN;
    unsigned char               pucFieldName[SPI_FIELD_NAME_MAXIMUM_LENGTH + 1] = {'\0'};

    struct spiDocumentInfo      *psdiSpiDocumentInfo = NULL;

    /* To calculate the search time */
    struct timeval              tvSearchStartTimeVal;
    struct timeval              tvSearchEndTimeVal;
    struct timeval              tvSearchDiffTimeVal;
    double                      dSearchTime = 0;

    unsigned int                uiLineCounter = 0;

    unsigned char               pucSortKey[UTL_FILE_PATH_MAX + 1] = {'\0'};


    ASSERT(plslScript != NULL);


    /* Open the script file from the file path if it was specified, otherwise we read from 'stdin' */
    if ( bUtlStringsIsStringNULL(plslScript->pucScriptFilePath) == false ) {

        /* Open the script file from the file path */
        if ( (pfScriptFile = s_fopen(plslScript->pucScriptFilePath, "r")) == NULL ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to open the script file: '%s'.", plslScript->pucScriptFilePath);
            goto bailFromvLScriptRunScript;
        }
    }
    else {
        /* Read the script file from 'stdin' */
        pfScriptFile = stdin;
    }


    /* Set the start time */
    s_gettimeofday(&tvSearchStartTimeVal, NULL);


    /* Loop forever */
    while ( true ) {
    
        /* Reset the line buffer and the line length */
        pucLine[0] = '\0';
        uiLineLength = 0;
        pucLineEndPtr = NULL;

        /* Loop reading the line, we now support a trailing '\' to indicate the the line continues on the next line */
        while ( true ) {
        
            /* Read the line from the file */
            bEoF = (s_fgets(pucLine + uiLineLength, LSCRIPT_LONG_LINE_LENGTH - uiLineLength, pfScriptFile) == NULL) ? true : false;
        
            /* Increment the line counter */
            uiLineCounter++;

            /* Get the new line length here for optimization */
            uiLineLength = s_strlen(pucLine);

            /* Erase the trailing new line - be platform sensitive, handle PC first, then Unix and Mac  */
            if ( (uiLineLength >= 2) && (pucLine[uiLineLength - 2] == '\r') ) {
                pucLineEndPtr = pucLine + (uiLineLength - 2);
            }
            else if ( (uiLineLength >= 1) && ((pucLine[uiLineLength - 1] == '\n') || (pucLine[uiLineLength - 1] == '\r')) ) {
                pucLineEndPtr = pucLine + (uiLineLength - 1);
            }
            else if ( (uiLineLength >= 1) && (bEoF == true) ) {
                pucLineEndPtr = pucLine + uiLineLength;
            }
            else {
                pucLineEndPtr = NULL;
                break;
            }

            /* Break out if this was a complete line, erasing the trailing new line */
            if ( (pucLine == pucLineEndPtr) || (*(pucLineEndPtr - 1) != '\\') ) {
                *pucLineEndPtr = '\0';
                break;
            }

            /* Erase the the trailing '\', which is before the trailing new line */
            pucLineEndPtr--;
            *pucLineEndPtr = '\0';

            /* Calculate the new line length */
            uiLineLength = pucLineEndPtr - pucLine;
        }


        /* Check to see if we have reached the end of the file */
        if ( (bEoF == true) && (pucLineEndPtr == NULL) ) {
            goto bailFromvLScriptRunScript;
        }

        /* Erase comments, but log them */
        if ( pucLine[0] == '#' ) {
/*             iUtlLogDebug(UTL_LOG_CONTEXT, "Comment: '%s'.", pucLine); */
            pucLine[0] = '\0';
        }

        /* Trim string */
        iUtlStringsTrimString(pucLine);

        /* Check for an empty string */
        if ( bUtlStringsIsStringNULL(pucLine) == true ) {
            continue;
        }


        /* Reset the action pointer */
        plsaLScriptActionsPtr = plsaLScriptActionsGlobal;

        /* Scan for the first token on the line */
        snprintf(pucScanfFormat, UTL_STRING_SCANF_FORMAT_LENGTH + 1, "%%%ds", LSCRIPT_SHORT_LINE_LENGTH);
        if ( sscanf(pucLine, pucScanfFormat, pucAction) != 1 ) {
            /* Nothing on this line */
            continue;
        }

        /* Loop through all the actions and see if the script line makes sense */
        while ( plsaLScriptActionsPtr->iAction != -1 ) {
            /* Check the line */
            if ( s_strcasecmp(plsaLScriptActionsPtr->pucActionText, pucAction) == 0 ) {
                /* There is a match, so we break out */
                break;
            }
            plsaLScriptActionsPtr++;
        }    

        /* Did we find a match? */
        if ( plsaLScriptActionsPtr->iAction == -1 ) {
            /* We did not, so we loop and get the next line */
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to parse line: %u, line text: '%s'.", uiLineCounter, pucLine);
            continue;
        }

        /* Resume */
        if ( plsaLScriptActionsPtr->iAction == LSCRIPT_ACTION_RESUME ) {
            bSkip = false;
        }

        /* Continue if we are to skip */
        if ( bSkip == true ) {
            continue;
        }


        /* First we parse the line */
        switch ( plsaLScriptActionsPtr->iAction ) {

            case LSCRIPT_ACTION_EXIT:
                break;

            case LSCRIPT_ACTION_OPEN_CONNECTION:
                snprintf(pucScanfFormat, UTL_STRING_SCANF_FORMAT_LENGTH + 1, "%%%ds %%%ds %%%ds %%d", LSCRIPT_SHORT_LINE_LENGTH, LSCRIPT_SHORT_LINE_LENGTH, MAXHOSTNAMELEN);
                if ( sscanf(pucLine, pucScanfFormat, pucAction, pucProtocol, pucHostName, &iPort) != 4 ) {
                    iUtlLogError(UTL_LOG_CONTEXT, "Failed to parse line: %u, line text: '%s'.", uiLineCounter, pucLine);
                    continue;
                }
                break;

            case LSCRIPT_ACTION_CLOSE_CONNECTION:
                break;

            case LSCRIPT_ACTION_LANGUAGE:
                snprintf(pucScanfFormat, UTL_STRING_SCANF_FORMAT_LENGTH + 1, "%%%ds %%%ds", LSCRIPT_SHORT_LINE_LENGTH, UTL_FILE_PATH_MAX);
                if ( sscanf(pucLine, pucScanfFormat, pucAction, pucLanguageCode) != 2 ) {
                    iUtlLogError(UTL_LOG_CONTEXT, "Failed to parse line: %u, line text: '%s'.", uiLineCounter, pucLine);
                    continue;
                }
                break;

            case LSCRIPT_ACTION_OPEN_INDEX:
                if ( (pucPtr = (unsigned char *)s_strchr(pucLine, ' ')) != NULL ) {
                    s_strnncpy(pucIndexName, pucPtr + 1, SPI_INDEX_NAME_MAXIMUM_LENGTH + 1);
                }
                else {
                    iUtlLogError(UTL_LOG_CONTEXT, "Failed to parse line: %u, line text: '%s'.", uiLineCounter, pucLine);
                    continue;
                }
                break;

            case LSCRIPT_ACTION_CLOSE_INDEX:
                break;

            case LSCRIPT_ACTION_SEARCH:
            case LSCRIPT_ACTION_SEARCH_LIST:
                s_free(pucSearchText);
                if ( (pucPtr = (unsigned char *)s_strchr(pucLine, ' ')) != NULL ) {
                    if ( (pucSearchText = (unsigned char *)s_strdup(pucPtr + 1)) == NULL ) {
                        iUtlLogPanic(UTL_LOG_CONTEXT, "Memory allocation error");
                    }
                }
                break;

            case LSCRIPT_ACTION_SEARCH_OFFSETS:
                snprintf(pucScanfFormat, UTL_STRING_SCANF_FORMAT_LENGTH + 1, "%%%ds %%u %%u", LSCRIPT_SHORT_LINE_LENGTH);
                iStatus = sscanf(pucLine, pucScanfFormat, pucAction, &uiStartIndex, &uiEndIndex);
                
                if ( iStatus == 3 ) {
                    ;
                }
                else if ( iStatus == 2 ) {
                    ;
                }
                else {
                    iUtlLogError(UTL_LOG_CONTEXT, "Failed to parse line: %u, line text: '%s'.", uiLineCounter, pucLine);
                    continue;
                }
                
                if ( uiStartIndex > uiEndIndex ) {
                    uiStartIndex = 0;
                    uiEndIndex = 0;
                    continue;
                }
                break;

            case LSCRIPT_ACTION_SEARCH_REPORT:
                snprintf(pucScanfFormat, UTL_STRING_SCANF_FORMAT_LENGTH + 1, "%%%ds %%%ds", LSCRIPT_SHORT_LINE_LENGTH, UTL_FILE_PATH_MAX);
                if ( sscanf(pucLine, pucScanfFormat, pucAction, pucSearchReport) != 2 ) {
                    iUtlLogError(UTL_LOG_CONTEXT, "Failed to parse line: %u, line text: '%s'.", uiLineCounter, pucLine);
                    continue;
                }
                break;

            case LSCRIPT_ACTION_RETRIEVE_DOCUMENT:
                snprintf(pucScanfFormat, UTL_STRING_SCANF_FORMAT_LENGTH + 1, "%%%ds %%%ds [%%%d[^]]] %%%ds %%%ds", LSCRIPT_SHORT_LINE_LENGTH, SPI_INDEX_NAME_MAXIMUM_LENGTH, 
                        SPI_DOCUMENT_KEY_MAXIMUM_LENGTH, SPI_ITEM_NAME_MAXIMUM_LENGTH, SPI_MIME_TYPE_MAXIMUM_LENGTH);
                if ( sscanf(pucLine, pucScanfFormat, pucAction, pucIndexName, pucDocumentKey, pucItemName, pucMimeType) != 5 ) {
                    iUtlLogError(UTL_LOG_CONTEXT, "Failed to parse line: %u, line text: '%s'.", uiLineCounter, pucLine);
                    continue;
                }
                break;

            case LSCRIPT_ACTION_RETRIEVE_DOCUMENT_BYTES:
                snprintf(pucScanfFormat, UTL_STRING_SCANF_FORMAT_LENGTH + 1, "%%%ds %%%ds [%%%d[^]]] %%%ds %%%ds %%u %%u", LSCRIPT_SHORT_LINE_LENGTH, SPI_INDEX_NAME_MAXIMUM_LENGTH, 
                        SPI_DOCUMENT_KEY_MAXIMUM_LENGTH, SPI_ITEM_NAME_MAXIMUM_LENGTH, SPI_MIME_TYPE_MAXIMUM_LENGTH);
                if ( sscanf(pucLine, pucScanfFormat, pucAction, pucIndexName, pucDocumentKey, pucItemName, pucMimeType, &uiChunkStart, &uiChunkEnd) != 7 ) {
                    iUtlLogError(UTL_LOG_CONTEXT, "Failed to parse line: %u, line text: '%s'.", uiLineCounter, pucLine);
                    continue;
                }
                break;

            case LSCRIPT_ACTION_SAVE_DOCUMENT:
                snprintf(pucScanfFormat, UTL_STRING_SCANF_FORMAT_LENGTH + 1, "%%%ds %%%ds [%%%d[^]]] %%%ds %%%ds %%%ds", LSCRIPT_SHORT_LINE_LENGTH, SPI_INDEX_NAME_MAXIMUM_LENGTH, 
                        SPI_DOCUMENT_KEY_MAXIMUM_LENGTH, SPI_ITEM_NAME_MAXIMUM_LENGTH, SPI_MIME_TYPE_MAXIMUM_LENGTH, UTL_FILE_PATH_MAX);
                if ( sscanf(pucLine, pucScanfFormat, pucAction, pucIndexName, pucDocumentKey, pucItemName, pucMimeType, pucFilePath) != 6 ) {
                    iUtlLogError(UTL_LOG_CONTEXT, "Failed to parse line: %u, line text: '%s'.", uiLineCounter, pucLine);
                    continue;
                }
                break;

            case LSCRIPT_ACTION_ADD_POSITIVE_FEEDBACK:
                snprintf(pucScanfFormat, UTL_STRING_SCANF_FORMAT_LENGTH + 1, "%%%ds %%%ds [%%%d[^]]] %%%ds %%%ds", LSCRIPT_SHORT_LINE_LENGTH, SPI_INDEX_NAME_MAXIMUM_LENGTH, 
                        SPI_DOCUMENT_KEY_MAXIMUM_LENGTH, SPI_ITEM_NAME_MAXIMUM_LENGTH, SPI_MIME_TYPE_MAXIMUM_LENGTH);
                if ( sscanf(pucLine, pucScanfFormat, pucAction, pucIndexName, pucDocumentKey, pucItemName, pucMimeType) != 5 ) {
                    iUtlLogError(UTL_LOG_CONTEXT, "Failed to parse line: %u, line text: '%s'.", uiLineCounter, pucLine);
                    continue;
                }
                break;

            case LSCRIPT_ACTION_ADD_POSITIVE_FEEDBACK_TEXT:
                if ( (pucPtr = (unsigned char *)s_strchr(pucLine, ' ')) != NULL ) {
                    if ( (iError = iUtlStringsAppendString(pucPositiveFeedbackText, pucPtr + 1, &pucPositiveFeedbackText)) != UTL_NoError ) {
                        iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to append to the positive feedback text, utl error: %d", iError);
                    }
                    if ( (iError = iUtlStringsAppendString(pucPositiveFeedbackText, " ", &pucPositiveFeedbackText)) != UTL_NoError ) {
                        iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to append to the positive feedback text, utl error: %d", iError);
                    }
                }
                break;

            case LSCRIPT_ACTION_CLEAR_POSITIVE_FEEDBACK:
                s_free(pucPositiveFeedbackText);
                break;

            case LSCRIPT_ACTION_ADD_NEGATIVE_FEEDBACK:
                snprintf(pucScanfFormat, UTL_STRING_SCANF_FORMAT_LENGTH + 1, "%%%ds %%%ds [%%%d[^]]] %%%ds %%%ds", LSCRIPT_SHORT_LINE_LENGTH, SPI_INDEX_NAME_MAXIMUM_LENGTH, 
                        SPI_DOCUMENT_KEY_MAXIMUM_LENGTH, SPI_ITEM_NAME_MAXIMUM_LENGTH, SPI_MIME_TYPE_MAXIMUM_LENGTH);
                if ( sscanf(pucLine, pucScanfFormat, pucAction, pucIndexName, pucDocumentKey, pucItemName, pucMimeType) != 5 ) {
                    iUtlLogError(UTL_LOG_CONTEXT, "Failed to parse line: %u, line text: '%s'.", uiLineCounter, pucLine);
                    continue;
                }
                break;

            case LSCRIPT_ACTION_ADD_NEGATIVE_FEEDBACK_TEXT:
                if ( (pucPtr = (unsigned char *)s_strchr(pucLine, ' ')) != NULL ) {
                    if ( (iError = iUtlStringsAppendString(pucNegativeFeedbackText, pucPtr + 1, &pucNegativeFeedbackText)) != UTL_NoError ) {
                        iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to append to the negative feedback text, utl error: %d", iError);
                    }
                    if ( (iError = iUtlStringsAppendString(pucNegativeFeedbackText, " ", &pucNegativeFeedbackText)) != UTL_NoError ) {
                        iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to append to the negative feedback text, utl error: %d", iError);
                    }
                }
                break;

            case LSCRIPT_ACTION_CLEAR_NEGATIVE_FEEDBACK:
                s_free(pucNegativeFeedbackText);
                break;

            case LSCRIPT_ACTION_GET_SERVER_INFO:
            case LSCRIPT_ACTION_GET_SERVER_INDEX_INFO:
                break;

            case LSCRIPT_ACTION_GET_INDEX_INFO:
            case LSCRIPT_ACTION_GET_INDEX_FIELD_INFO:
                snprintf(pucScanfFormat, UTL_STRING_SCANF_FORMAT_LENGTH + 1, "%%%ds %%%ds", LSCRIPT_SHORT_LINE_LENGTH, SPI_INDEX_NAME_MAXIMUM_LENGTH);
                if ( sscanf(pucLine, pucScanfFormat, pucAction, pucIndexName) != 2 ) {
                    iUtlLogError(UTL_LOG_CONTEXT, "Failed to parse line: %u, line text: '%s'.", uiLineCounter, pucLine);
                    continue;
                }
                break;

            case LSCRIPT_ACTION_GET_INDEX_TERM_INFO:
                snprintf(pucScanfFormat, UTL_STRING_SCANF_FORMAT_LENGTH + 1, "%%%ds %%%ds %%%ds %%%ds %%%ds", LSCRIPT_SHORT_LINE_LENGTH, UTL_FILE_PATH_MAX, UTL_FILE_PATH_MAX, UTL_FILE_PATH_MAX, UTL_FILE_PATH_MAX);
                iStatus = sscanf(pucLine, pucScanfFormat, pucAction, pucTermMatch, pucTermCase, pucTerm, pucFieldName);

                if ( iStatus == 5 ) {
                    ;
                }
                else if ( iStatus == 4 ) {
                    if ( !((s_strcasecmp(pucTermMatch, "wildcard") == 0) || 
                            (s_strcasecmp(pucTermMatch, "soundex") == 0) || 
                            (s_strcasecmp(pucTermMatch, "metaphone") == 0) || 
                            (s_strcasecmp(pucTermMatch, "phonix") == 0) || 
                            (s_strcasecmp(pucTermMatch, "typo") == 0)) ) {
                        s_strnncpy(pucFieldName, pucTerm, UTL_FILE_PATH_MAX + 1);
                        pucFieldName[0] = '\0';
                    }
                }
                else if ( iStatus == 3 ) {
                    pucTerm[0] = '\0';
                    pucFieldName[0] = '\0';
                }
                else if ( iStatus == 2 ) {
                    pucTermCase[0] = '\0';
                    pucTerm[0] = '\0';
                    pucFieldName[0] = '\0';
                }
                else {
                    iUtlLogError(UTL_LOG_CONTEXT, "Failed to parse line: %u, line text: '%s'.", uiLineCounter, pucLine);
                    continue;
                }
                
                if ( s_strcasecmp(pucTermMatch, "regular") == 0 ) {
                    uiTermMatch = SPI_TERM_MATCH_REGULAR;
                } 
                else if ( s_strcasecmp(pucTermMatch, "stop") == 0 ) {
                    uiTermMatch = SPI_TERM_MATCH_STOP;
                } 
                else if ( s_strcasecmp(pucTermMatch, "wildcard") == 0 ) {
                    uiTermMatch = SPI_TERM_MATCH_WILDCARD;
                } 
                else if ( s_strcasecmp(pucTermMatch, "soundex") == 0 ) {
                    uiTermMatch = SPI_TERM_MATCH_SOUNDEX;
                } 
                else if ( s_strcasecmp(pucTermMatch, "metaphone") == 0 ) {
                    uiTermMatch = SPI_TERM_MATCH_METAPHONE;
                } 
                else if ( s_strcasecmp(pucTermMatch, "phonix") == 0 ) {
                    uiTermMatch = SPI_TERM_MATCH_PHONIX;
                } 
                else if ( s_strcasecmp(pucTermMatch, "typo") == 0 ) {
                    uiTermMatch = SPI_TERM_MATCH_TYPO;
                }
                else {
                    iUtlLogError(UTL_LOG_CONTEXT, "Failed to parse line: %u, line text: '%s'.", uiLineCounter, pucLine);
                    continue;
                }

                if ( s_strcasecmp(pucTermCase, "sensitive") == 0 ) {
                    uiTermCase = SPI_TERM_CASE_SENSITIVE;
                } 
                else if ( s_strcasecmp(pucTermMatch, "insensitive") == 0 ) {
                    uiTermCase = SPI_TERM_CASE_INSENSITIVE;
                }
                else {
                    uiTermCase = SPI_TERM_CASE_UNKNOWN;
                }

                break;

            case LSCRIPT_ACTION_GET_DOCUMENT_INFO:
                snprintf(pucScanfFormat, UTL_STRING_SCANF_FORMAT_LENGTH + 1, "%%%ds %%%ds [%%%d[^]]]", LSCRIPT_SHORT_LINE_LENGTH, SPI_INDEX_NAME_MAXIMUM_LENGTH, SPI_DOCUMENT_KEY_MAXIMUM_LENGTH);
                if ( sscanf(pucLine, pucScanfFormat, pucAction, pucIndexName, pucDocumentKey) != 3 ) {
                    iUtlLogError(UTL_LOG_CONTEXT, "Failed to parse line: %u, line text: '%s'.", uiLineCounter, pucLine);
                    continue;
                }
                break;

            case LSCRIPT_ACTION_SLEEP:
                snprintf(pucScanfFormat, UTL_STRING_SCANF_FORMAT_LENGTH + 1, "%%%ds %%d", LSCRIPT_SHORT_LINE_LENGTH);
                if ( sscanf(pucLine, pucScanfFormat, pucAction, &iSleep) != 2 ) {
                    iUtlLogError(UTL_LOG_CONTEXT, "Failed to parse line: %u, line text: '%s'.", uiLineCounter, pucLine);
                    continue;
                }
                break;

            case LSCRIPT_ACTION_SKIP:
            case LSCRIPT_ACTION_RESUME:
                break;

            default:
                break;

        } /* Switch (plsaLScriptActionsPtr->iAction) */



        /* If the parse only mode is on, we loop here */
        if ( plslScript->bCheckOnly == true ) {

            switch ( plsaLScriptActionsPtr->iAction ) {

                case LSCRIPT_ACTION_EXIT:
                    printf("Valid: '%s'.\n", plsaLScriptActionsPtr->pucActionText);
                    break;

                case LSCRIPT_ACTION_OPEN_CONNECTION:
                    printf("Valid: '%s' '%s' '%s' %d.\n", plsaLScriptActionsPtr->pucActionText,
                            pucProtocol, pucHostName, iPort);
                    break;

                case LSCRIPT_ACTION_CLOSE_CONNECTION:
                    printf("Valid: '%s'.\n", plsaLScriptActionsPtr->pucActionText);
                    break;

                case LSCRIPT_ACTION_LANGUAGE:
                    printf("Valid: '%s' '%s'.\n", plsaLScriptActionsPtr->pucActionText,
                            pucLanguageCode);
                    break;

                case LSCRIPT_ACTION_OPEN_INDEX:
                    printf("Valid: '%s' '%s'.\n", plsaLScriptActionsPtr->pucActionText, 
                            pucIndexName);
                    break;

                case LSCRIPT_ACTION_CLOSE_INDEX:
                    printf("Valid: %s.\n", plsaLScriptActionsPtr->pucActionText);
                    break;

                case LSCRIPT_ACTION_SEARCH:
                case LSCRIPT_ACTION_SEARCH_LIST:
                    printf("Valid: '%s' '%s'.\n", plsaLScriptActionsPtr->pucActionText, pucSearchText);
                    break;

                case LSCRIPT_ACTION_SEARCH_OFFSETS:
                    printf("Valid: '%s' %u %u.\n", plsaLScriptActionsPtr->pucActionText, 
                            uiStartIndex, uiEndIndex);
                    break;

                case LSCRIPT_ACTION_SEARCH_REPORT:
                    printf("Valid: '%s' '%s'.\n", plsaLScriptActionsPtr->pucActionText, 
                            pucSearchReport);
                    break;

                case LSCRIPT_ACTION_RETRIEVE_DOCUMENT:
                    printf("Valid: '%s' '%s' '%s' '%s' '%s'.\n", plsaLScriptActionsPtr->pucActionText, 
                            pucIndexName, pucDocumentKey, pucItemName, pucMimeType);
                    break;

                case LSCRIPT_ACTION_RETRIEVE_DOCUMENT_BYTES:
                    printf("Valid: '%s' '%s' '%s' '%s' '%s' %u %u.\n", plsaLScriptActionsPtr->pucActionText, 
                            pucIndexName, pucDocumentKey, pucItemName, pucMimeType, uiChunkStart, uiChunkEnd);
                    break;

                case LSCRIPT_ACTION_SAVE_DOCUMENT:
                    printf("Valid: '%s' '%s' '%s' '%s' '%s' '%s'.\n", plsaLScriptActionsPtr->pucActionText, 
                            pucIndexName, pucDocumentKey, pucItemName, pucMimeType, pucFilePath);
                    break;

                case LSCRIPT_ACTION_ADD_POSITIVE_FEEDBACK:
                    printf("Valid: '%s' '%s' '%s' '%s' '%s'.\n", plsaLScriptActionsPtr->pucActionText, 
                            pucIndexName, pucDocumentKey, pucItemName, pucMimeType);
                    break;

                case LSCRIPT_ACTION_ADD_POSITIVE_FEEDBACK_TEXT:
                    printf("Valid: '%s' '%s'.\n", plsaLScriptActionsPtr->pucActionText, 
                            pucPositiveFeedbackText);
                    break;

                case LSCRIPT_ACTION_CLEAR_POSITIVE_FEEDBACK:
                    printf("Valid: '%s'.\n", plsaLScriptActionsPtr->pucActionText);
                    break;

                case LSCRIPT_ACTION_ADD_NEGATIVE_FEEDBACK:
                    printf("Valid: '%s' '%s' '%s' '%s' '%s'.\n", plsaLScriptActionsPtr->pucActionText, 
                            pucIndexName, pucDocumentKey, pucItemName, pucMimeType);
                    break;

                case LSCRIPT_ACTION_ADD_NEGATIVE_FEEDBACK_TEXT:
                    printf("Valid: '%s' '%s'.\n", plsaLScriptActionsPtr->pucActionText, 
                            pucPositiveFeedbackText);
                    break;

                case LSCRIPT_ACTION_CLEAR_NEGATIVE_FEEDBACK:
                    printf("Valid: '%s'.\n", plsaLScriptActionsPtr->pucActionText);
                    break;

                case LSCRIPT_ACTION_GET_SERVER_INFO:
                case LSCRIPT_ACTION_GET_SERVER_INDEX_INFO:
                    printf("Valid: '%s'.\n", plsaLScriptActionsPtr->pucActionText);
                    break;

                case LSCRIPT_ACTION_GET_INDEX_INFO:
                case LSCRIPT_ACTION_GET_INDEX_FIELD_INFO:
                    printf("Valid: '%s' '%s'.\n", plsaLScriptActionsPtr->pucActionText, 
                            pucIndexName);
                    break;

                case LSCRIPT_ACTION_GET_INDEX_TERM_INFO:
                    printf("Valid: '%s' '%s' %u '%s' '%s'.\n", plsaLScriptActionsPtr->pucActionText, 
                            pucIndexName, uiTermMatch, pucTerm, pucFieldName);
                    break;

                case LSCRIPT_ACTION_GET_DOCUMENT_INFO:
                    printf("Valid: '%s' '%s' '%s'.\n", plsaLScriptActionsPtr->pucActionText, 
                            pucIndexName, pucDocumentKey);
                    break;

                case LSCRIPT_ACTION_SLEEP:
                    printf("Valid: '%s' %d.\n", plsaLScriptActionsPtr->pucActionText, iSleep);
                    break;

                case LSCRIPT_ACTION_SKIP:
                    printf("Valid: '%s'.\n", plsaLScriptActionsPtr->pucActionText);
                    break;

                case LSCRIPT_ACTION_RESUME:
                    printf("Valid: '%s'.\n", plsaLScriptActionsPtr->pucActionText);
                    break;

                default:
                    iUtlLogWarn(UTL_LOG_CONTEXT, "Invalid line: %u, line text: '%s'.", uiLineCounter, pucLine);
                    break;

            } /* Switch (plsaLScriptActionsPtr->iAction) */

            /* And loop */
            continue;

        } /* If ( plslScript->bCheckOnly == true ) */



        /* This is where we do the real work */
        switch ( plsaLScriptActionsPtr->iAction ) {

            /* Exit */
            case LSCRIPT_ACTION_EXIT:

                printf("Function - exiting:\n");

                /* Free dynamically allocated stuff */
                s_free(pucSearchText);
                s_free(pucPositiveFeedbackText);
                s_free(pucNegativeFeedbackText);

                /* Free the lwps structure */
                iLwpsFree(pvLwps);
                pvLwps = NULL;

                /* Free the net structure */
                iUtlNetFree(pvUtlNet);
                pvUtlNet = NULL;

                /* Free the index list */
                UTL_MACROS_FREE_NULL_TERMINATED_LIST(ppucIndexNameList);


                /* Set the search end time */
                s_gettimeofday(&tvSearchEndTimeVal, NULL);
        
                /* Get the time taken for this search */
                UTL_DATE_DIFF_TIMEVAL(tvSearchStartTimeVal, tvSearchEndTimeVal, tvSearchDiffTimeVal);
        
                /* Turn it into milliseconds */
                UTL_DATE_TIMEVAL_TO_MILLISECONDS(tvSearchDiffTimeVal, dSearchTime);
        
                /* And log it */
                printf("Time taken: %.1f milliseconds.\n", dSearchTime);

                
                /* Bail */
                goto bailFromvLScriptRunScript;


            /* Open the connection to the server */
            case LSCRIPT_ACTION_OPEN_CONNECTION:

                printf("Function - Open Connection:\n");
    
                /* Set the network protocol */
                if ( s_strcmp(pucProtocol, UTL_NET_PROTOCOL_UDP_NAME) == 0 ) {
                    uiNetProtocolID = UTL_NET_PROTOCOL_UDP_ID;
                }
                else if ( s_strcmp(pucProtocol, UTL_NET_PROTOCOL_TCP_NAME) == 0 ) {
                    uiNetProtocolID = UTL_NET_PROTOCOL_TCP_ID;
                }
                else {
                    iUtlLogError(UTL_LOG_CONTEXT, "Invalid or unsupported network protocol: '%s'.", pucProtocol);
                    goto bailFromvLScriptRunScript;
                }

                /* Open a connection to the host */
                if ( (iError = iUtlNetCreateClient(uiNetProtocolID, pucHostName, iPort, plslScript->uiTimeout, &pvUtlNet)) != UTL_NoError ) {
                    iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to open a net client connection, network protocol: %u, host: '%s', port: %d, utl error: %d", 
                            uiNetProtocolID, pucUtlStringsGetPrintableString(pucHostName), iPort, iError);
                    goto bailFromvLScriptRunScript;
                }


                /* Create a LPWS handle */
                if ( (iError = iLwpsCreate(pvUtlNet, &pvLwps)) != LWPS_NoError ) {
                    iUtlLogError(UTL_LOG_CONTEXT, "Failed to create an lwps, lwps error: %d.", iError);
                    goto bailFromvLScriptRunScript;
                }


                /* Exchange inits if there is a username or password */ 
                if ( (bUtlStringsIsStringNULL(pucUserName) == false) || (bUtlStringsIsStringNULL(pucPassword) == false) ) {

                    /* Send the init */
                    if ( (iError = iLwpsInitRequestHandle(pvLwps, pucUserName, pucPassword, &iErrorCode, &pucErrorString)) != LWPS_NoError ) {
                        iUtlLogError(UTL_LOG_CONTEXT, "Failed to handle an lwps init request, lwps error: %d.", iError);
                        goto bailFromvLScriptRunScript;
                    }
                    else {
    
                        /* Check the returned error code */
                        if ( iError != SPI_NoError ) {
                            iUtlLogError(UTL_LOG_CONTEXT, "Failed to handle an lwps init request, returned error code: %d, error text: '%s'.", iErrorCode, pucUtlStringsGetPrintableString(pucErrorString));
                            s_free(pucErrorString);
                            goto bailFromvLScriptRunScript;
                        }
                    }
                }

                break;


            /* Close the connection to the server */
            case LSCRIPT_ACTION_CLOSE_CONNECTION:

                printf("Function - Close Connection:\n");

                /* Free the lwps structure */
                iLwpsFree(pvLwps);
                pvLwps = NULL;

                /* Free the net structure */
                iUtlNetFree(pvUtlNet);
                pvUtlNet = NULL;

                /* Free the index list */
                UTL_MACROS_FREE_NULL_TERMINATED_LIST(ppucIndexNameList);

                break;


            /* Set the language */
            case LSCRIPT_ACTION_LANGUAGE:

                printf("Action - Setting language to: '%s'.\n", pucLanguageCode);
                break;


            /* Open the index(s) */
            case LSCRIPT_ACTION_OPEN_INDEX:

                printf("Function - Open Index:\n");

                /* Loop parsing the index names */
                for ( pucPtr = s_strtok_r(pucIndexName, LSCRIPT_INDEX_NAME_SEPARATORS, (char **)&pucIndexNameStrtokPtr), uiI = 0; 
                        pucPtr != NULL; 
                        pucPtr = s_strtok_r(NULL, LSCRIPT_INDEX_NAME_SEPARATORS, (char **)&pucIndexNameStrtokPtr), uiI++ ) {

                    /* Add the index name to the index name list */
                    if ( (ppucIndexNameList = (unsigned char **)s_realloc(ppucIndexNameList, (size_t)(sizeof(unsigned char *) * (uiI + 2)))) == NULL ) {
                        iUtlLogPanic(UTL_LOG_CONTEXT, "Memory allocation error");
                    }
                    if ( (ppucIndexNameList[uiI] = (unsigned char *)s_strdup(pucPtr)) == NULL ) {
                        iUtlLogPanic(UTL_LOG_CONTEXT, "Memory allocation error");
                    }

                    /* NULL terminate the index name list */
                    ppucIndexNameList[uiI + 1] = NULL;
                }

                break;


            /* Close the index(s) */
            case LSCRIPT_ACTION_CLOSE_INDEX:

                printf("Function - Close Index:\n");

                /* Free the index list */
                UTL_MACROS_FREE_NULL_TERMINATED_LIST(ppucIndexNameList);

                break;


            /* Run a search */
            case LSCRIPT_ACTION_SEARCH:
            case LSCRIPT_ACTION_SEARCH_LIST:

                printf("Function - Search Indices:\n");

                /* Submit a search */
                if ( (iError = iLwpsSearchRequestHandle(pvLwps, ppucIndexNameList, pucLanguageCode, pucSearchText, pucPositiveFeedbackText, pucNegativeFeedbackText, 
                        uiStartIndex, uiEndIndex, &pssrSpiSearchResponse, &iErrorCode, &pucErrorString)) != LWPS_NoError ) {

                    iUtlLogError(UTL_LOG_CONTEXT, "Failed to handle an lwps search request, lwps error: %d.", iError);
                    goto bailFromvLScriptRunScript;
                }
                else {

                    /* Check the returned error code */
                    if ( iErrorCode != SPI_NoError ) {
                        iUtlLogError(UTL_LOG_CONTEXT, "Failed to handle an lwps search request, returned error code: %d, error text: '%s'.", 
                                iErrorCode, pucUtlStringsGetPrintableString(pucErrorString));
                        s_free(pucErrorString);
                        goto bailFromvLScriptRunScript;
                    }
                    else {

                        unsigned int    uiSearchReportCount = 0;
                
                        /* Count up the search reports */
                        for ( pssrSpiSearchResultsPtr = pssrSpiSearchResponse->pssrSpiSearchResults; 
                                pssrSpiSearchResultsPtr < (pssrSpiSearchResponse->pssrSpiSearchResults + pssrSpiSearchResponse->uiSpiSearchResultsLength); 
                                pssrSpiSearchResultsPtr++ ) {
            
                            /* Increment the search report counter when we encounter one */
                            if ( (pssrSpiSearchResultsPtr->psdiSpiDocumentItems != NULL) && 
                                    (s_strcmp(pssrSpiSearchResultsPtr->psdiSpiDocumentItems->pucItemName, SPI_SEARCH_REPORT_ITEM_NAME) == 0) &&
                                    (s_strcmp(pssrSpiSearchResultsPtr->psdiSpiDocumentItems->pucMimeType, SPI_SEARCH_REPORT_MIME_TYPE) == 0) ) {
            
                                uiSearchReportCount++;
                            }
                        }
    
                        /* Tell the user how many results we got back */
                        if ( uiSearchReportCount > 0 ) {
                            printf("Search: '%s', found: %u document%s and returned: %u (+%u search report%s), maximum sort key: %.4f, search time: %.1f milliseconds.\n", 
                                    pucUtlStringsGetSafeString(pucSearchText), pssrSpiSearchResponse->uiTotalResults, 
                                    (pssrSpiSearchResponse->uiTotalResults == 1) ? "" : "s", 
                                    pssrSpiSearchResponse->uiSpiSearchResultsLength - uiSearchReportCount, uiSearchReportCount, 
                                    (uiSearchReportCount == 1) ? "" : "s", pssrSpiSearchResponse->dMaxSortKey, 
                                    pssrSpiSearchResponse->dSearchTime);
                        }
                        else {
                            printf("Search: '%s', found: %u document%s and returned: %u, maximum sort key: %.4f, search time: %.1f milliseconds.\n", 
                                    pucUtlStringsGetPrintableString(pucSearchText), pssrSpiSearchResponse->uiTotalResults, 
                                    (pssrSpiSearchResponse->uiTotalResults == 1) ? "" : "s", 
                                    pssrSpiSearchResponse->uiSpiSearchResultsLength, pssrSpiSearchResponse->dMaxSortKey,
                                    pssrSpiSearchResponse->dSearchTime);
                        }


                        if ( plsaLScriptActionsPtr->iAction == LSCRIPT_ACTION_SEARCH_LIST ) {

                            /* Loop through all the results */
                            for ( uiI = 0, pssrSpiSearchResultsPtr = pssrSpiSearchResponse->pssrSpiSearchResults; 
                                    uiI < pssrSpiSearchResponse->uiSpiSearchResultsLength; 
                                    uiI++, pssrSpiSearchResultsPtr++ ) {

                                /* Create the sort key string */
                                switch ( pssrSpiSearchResponse->uiSortType ) {
                        
                                    case SPI_SORT_TYPE_DOUBLE_ASC:
                                    case SPI_SORT_TYPE_DOUBLE_DESC:
                                        snprintf(pucSortKey, UTL_FILE_PATH_MAX + 1, "%.4f", pssrSpiSearchResultsPtr->dSortKey);
                                        break;
                        
                                    case SPI_SORT_TYPE_FLOAT_ASC:
                                    case SPI_SORT_TYPE_FLOAT_DESC:
                                        snprintf(pucSortKey, UTL_FILE_PATH_MAX + 1, "%.4f", pssrSpiSearchResultsPtr->fSortKey);
                                        break;
                                    
                                    case SPI_SORT_TYPE_UINT_ASC:
                                    case SPI_SORT_TYPE_UINT_DESC:
                                        snprintf(pucSortKey, UTL_FILE_PATH_MAX + 1, "%u", pssrSpiSearchResultsPtr->uiSortKey);
                                        break;
                                    
                                    case SPI_SORT_TYPE_ULONG_ASC:
                                    case SPI_SORT_TYPE_ULONG_DESC:
                                        snprintf(pucSortKey, UTL_FILE_PATH_MAX + 1, "%lu", pssrSpiSearchResultsPtr->ulSortKey);
                                        break;
                                    
                                    case SPI_SORT_TYPE_UCHAR_ASC:
                                    case SPI_SORT_TYPE_UCHAR_DESC:
                                        snprintf(pucSortKey, UTL_FILE_PATH_MAX + 1, "%s", pucUtlStringsGetPrintableString(pssrSpiSearchResultsPtr->pucSortKey));
                                        break;
                                    
                                    case SPI_SORT_TYPE_NO_SORT:
                                        snprintf(pucSortKey, UTL_FILE_PATH_MAX + 1, "(none)");
                                        break;
                        
                                    case SPI_SORT_TYPE_UNKNOWN:
                                        snprintf(pucSortKey, UTL_FILE_PATH_MAX + 1, "(unknown)");
                                        break;
                                    
                                    default:
                                        snprintf(pucSortKey, UTL_FILE_PATH_MAX + 1, "(invalid)");
                                        break;
                                }

                                /* Are there any document items for this hit? */
                                if ( pssrSpiSearchResultsPtr->psdiSpiDocumentItems != NULL ) {

                                    pucDocItems[0] = '\0';

                                    /* Loop through each item, assemble a readable string from them */
                                    for ( uiJ = 0, psdiSpiDocumentItemsPtr = pssrSpiSearchResultsPtr->psdiSpiDocumentItems; uiJ < pssrSpiSearchResultsPtr->uiDocumentItemsLength; uiJ++, psdiSpiDocumentItemsPtr++ ) {
                                        
                                        if ( (uiJ > 0) && (uiJ < pssrSpiSearchResultsPtr->uiDocumentItemsLength) ) {
                                            s_strnncat(pucDocItems, "; ", LSCRIPT_SHORT_LINE_LENGTH, LSCRIPT_SHORT_LINE_LENGTH + 1);
                                        }
                                        
                                        s_strnncat(pucDocItems, psdiSpiDocumentItemsPtr->pucItemName, LSCRIPT_SHORT_LINE_LENGTH, LSCRIPT_SHORT_LINE_LENGTH + 1);
                                        s_strnncat(pucDocItems, ", ", LSCRIPT_SHORT_LINE_LENGTH, LSCRIPT_SHORT_LINE_LENGTH + 1);

                                        s_strnncat(pucDocItems, psdiSpiDocumentItemsPtr->pucMimeType, LSCRIPT_SHORT_LINE_LENGTH, LSCRIPT_SHORT_LINE_LENGTH + 1);
                                        s_strnncat(pucDocItems, ", ", LSCRIPT_SHORT_LINE_LENGTH, LSCRIPT_SHORT_LINE_LENGTH + 1);

                                        snprintf(pucNum, LSCRIPT_SHORT_LINE_LENGTH + 1, "%u", psdiSpiDocumentItemsPtr->uiLength);
                                        s_strnncat(pucDocItems, pucNum, LSCRIPT_SHORT_LINE_LENGTH, LSCRIPT_SHORT_LINE_LENGTH + 1);
                                    } 
                                }
                                else {
                                    /* No document items are defined */
                                    s_strnncpy(pucDocItems, "(none)", LSCRIPT_SHORT_LINE_LENGTH + 1);
                                }

                                printf("Hit: %u, index: '%s', document key: '%s', title: '%s', sort key: %s, language code: '%s', rank: %u, term count: %u, ansi date: %lu, items: '%s'.\n", 
                                        uiI, pssrSpiSearchResultsPtr->pucIndexName, pssrSpiSearchResultsPtr->pucDocumentKey, pssrSpiSearchResultsPtr->pucTitle, pucSortKey, 
                                        pucUtlStringsGetPrintableString(pssrSpiSearchResultsPtr->pucLanguageCode), pssrSpiSearchResultsPtr->uiRank, pssrSpiSearchResultsPtr->uiTermCount, 
                                        pssrSpiSearchResultsPtr->ulAnsiDate, pucDocItems);
                            }
                        }

                        /* Do we want to get the search report? */
                        if ( (s_strcasecmp(pucSearchReport, "raw") == 0) || (s_strcasecmp(pucSearchReport, "formatted") == 0) ) {
                            
                            unsigned char    **ppucSearchReportList = NULL;
                            unsigned int    uiSearchReportListLen = 0;
                        
                            /* Loop through all the results */
                            for ( uiI = 0, pssrSpiSearchResultsPtr = pssrSpiSearchResponse->pssrSpiSearchResults; 
                                    uiI < pssrSpiSearchResponse->uiSpiSearchResultsLength; 
                                    uiI++, pssrSpiSearchResultsPtr++ ) {

                                /* Check that this document is indeed the search report */
                                if ( (pssrSpiSearchResultsPtr->psdiSpiDocumentItems != NULL) && 
                                        (s_strcmp(pssrSpiSearchResultsPtr->psdiSpiDocumentItems->pucItemName, SPI_SEARCH_REPORT_ITEM_NAME) == 0) &&
                                        (s_strcmp(pssrSpiSearchResultsPtr->psdiSpiDocumentItems->pucMimeType, SPI_SEARCH_REPORT_MIME_TYPE) == 0) ) {

                                    /* Reset the pointers */
                                    pvData = NULL;
                                    uiDataLength = 0;
    
                                    /* Get the document from the search result structure if it is there */
                                    if ( pssrSpiSearchResultsPtr->psdiSpiDocumentItems->pvData != NULL ) {
                    
                                        /* Duplicate the data */
                                        if ( (pvData = s_memdup(pssrSpiSearchResultsPtr->psdiSpiDocumentItems->pvData, pssrSpiSearchResultsPtr->psdiSpiDocumentItems->uiDataLength)) == NULL ) {
                                            iUtlLogPanic(UTL_LOG_CONTEXT, "Memory allocation error");
                                        }
    
                                        /* Hand over the data length */
                                        uiDataLength = pssrSpiSearchResultsPtr->psdiSpiDocumentItems->uiDataLength;
                                    }
                                    /* Otherwise get the document from the server */
                                    else {

                                        if ( (iError = iLwpsRetrievalRequestHandle(pvLwps, pssrSpiSearchResultsPtr->pucIndexName, pssrSpiSearchResultsPtr->pucDocumentKey, 
                                                pssrSpiSearchResultsPtr->psdiSpiDocumentItems->pucItemName, pssrSpiSearchResultsPtr->psdiSpiDocumentItems->pucMimeType, 
                                                SPI_CHUNK_TYPE_DOCUMENT, 0, 0, &pvData, &uiDataLength, &iErrorCode, &pucErrorString)) != LWPS_NoError ) {

                                            iUtlLogError(UTL_LOG_CONTEXT, "Failed to handle an lwps retrieval request, lwps error: %d.", iError);
                                        }
                                        else {
                        
                                            /* Check the returned error code */
                                            if ( iErrorCode != SPI_NoError ) {
                                                iUtlLogError(UTL_LOG_CONTEXT, "Failed to handle an lwps retrieval request, returned error code: %d, error text: '%s.", 
                                                        iErrorCode, pucUtlStringsGetPrintableString(pucErrorString));
                                                s_free(pucErrorString);
                                            }
                                        }
                                    }

                                    /* Add the search report document to the search report array */
                                    if ( pvData != NULL ) {
                                    
                                        /* Increment the length of search reports and extend the array */
                                        uiSearchReportListLen++;
                                        if ( (ppucSearchReportList = (unsigned char **)s_realloc(ppucSearchReportList, (size_t)(sizeof(unsigned char *) * (uiSearchReportListLen + 1)))) == NULL ) {
                                            iUtlLogPanic(UTL_LOG_CONTEXT, "Memory allocation error");
                                        }
                                        
                                        /* NULL terminate the data */
                                        if ( (iError = iUtlStringsNullTerminateData(pvData, uiDataLength, (unsigned char **)&pvData)) != UTL_NoError ) {
                                            iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to NULL terminate data, utl error: %d", iError);
                                        }

                                        /* Set the pointer and NULL terminate the array */
                                        ppucSearchReportList[uiSearchReportListLen - 1] = (unsigned char *)pvData;
                                        ppucSearchReportList[uiSearchReportListLen] = NULL;
                                    }
                                }                                
                            }
                            
                            /* Process the search reports if there are any */
                            if ( ppucSearchReportList != NULL ) {
                                
                                /* Print the raw reports */
                                if ( s_strcasecmp(pucSearchReport, "raw") == 0 ) {
                                    
                                    /* Print the raw search report */
                                    for ( uiI = 0; uiI < uiSearchReportListLen; uiI++ ) {
                                        printf("\n%s\n\n\n",  ppucSearchReportList[uiI]);
                                    }
                                }
                                /* Print the formatted report */
                                else if ( s_strcasecmp(pucSearchReport, "formatted") == 0 ) {
                                
                                    unsigned char *pucSearchReportFormatted = NULL;

                                    /* Merge and format the search reports */
                                    if ( (iError = iRepMergeAndFormatSearchReports(ppucSearchReportList, &pucSearchReportFormatted)) != REP_NoError ) {
                                        iUtlLogWarn(UTL_LOG_CONTEXT, "Failed to merge and format the search reports, rep error: %d.", iError);
                                    }
                                    else {
                                        /* Print the formatted search report */
                                        printf("\n%s\n\n\n", pucSearchReportFormatted);
    
                                        /* Free the formatted search report */
                                        s_free(pucSearchReportFormatted);
                                    }
                                }

                                /* Free the allocated search report */
                                UTL_MACROS_FREE_NUMBERED_LIST(ppucSearchReportList, uiSearchReportListLen);
                            }
                        }

                        /* Free the search response */
                        iSpiFreeSearchResponse(pssrSpiSearchResponse);
                        pssrSpiSearchResponse = NULL;
                    }
                }

                break;


            /* Set the start and end index */
            case LSCRIPT_ACTION_SEARCH_OFFSETS:

                printf("Action - Setting indices to: %u-%u.\n", uiStartIndex, uiEndIndex);
                break;


            /* Set the search get report */
            case LSCRIPT_ACTION_SEARCH_REPORT:

                printf("Action - Getting search report now set to: '%s'.\n", pucSearchReport);
                break;


            /* Retrieve document */
            case LSCRIPT_ACTION_RETRIEVE_DOCUMENT:

                printf("Function - Get Document (document):\n");

                printf("Index: '%s', document key: '%s', item name: '%s', mime type: '%s'.\n",
                        pucIndexName, pucDocumentKey, pucItemName, pucMimeType);

                /* Retrieve the document */
                if ( (iError = iLwpsRetrievalRequestHandle(pvLwps, pucUtlStringsGetNULLString(pucIndexName), pucUtlStringsGetNULLString(pucDocumentKey), 
                        pucUtlStringsGetNULLString(pucItemName), pucUtlStringsGetNULLString(pucMimeType), SPI_CHUNK_TYPE_DOCUMENT,
                        0, 0, &pvData, &uiDataLength, &iErrorCode, &pucErrorString)) != LWPS_NoError ) {
                    iUtlLogError(UTL_LOG_CONTEXT, "Failed to handle an lwps retrieval request, lwps error: %d.", iError);
                    goto bailFromvLScriptRunScript;
                }
                else {

                    /* Check the returned error code */
                    if ( iErrorCode != SPI_NoError) {
                        iUtlLogError(UTL_LOG_CONTEXT, "Failed to handle an lwps retrieval request, returned error code: %d, error text: '%s'.", 
                                iErrorCode, pucUtlStringsGetPrintableString(pucErrorString));
                        s_free(pucErrorString);
                        goto bailFromvLScriptRunScript;
                    }
                    else {

                        /* NULL terminate the data */
                        if ( (iError = iUtlStringsNullTerminateData(pvData, uiDataLength, (unsigned char **)&pvData)) != UTL_NoError ) {
                            iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to NULL terminate data, utl error: %d", iError);
                        }

                        /* Print the data */
                        printf("\n%s\n\n\n", (unsigned char *)pvData);
                        s_free(pvData);
                    }
                }

                break;


            /* Retrieve document by bytes */
            case LSCRIPT_ACTION_RETRIEVE_DOCUMENT_BYTES:

                printf("Function - Get Document (bytes):\n");

                printf("Index: '%s', document key: '%s', item name: '%s', mime type: '%s', chunk start: %u, chunk end: %u.\n",
                        pucIndexName, pucDocumentKey, pucItemName, pucMimeType, uiChunkStart, uiChunkEnd);

                /* Retrieve the document */
                if ( (iError = iLwpsRetrievalRequestHandle(pvLwps, pucUtlStringsGetNULLString(pucIndexName), pucUtlStringsGetNULLString(pucDocumentKey), 
                        pucUtlStringsGetNULLString(pucItemName), pucUtlStringsGetNULLString(pucMimeType), SPI_CHUNK_TYPE_BYTE, 
                        uiChunkStart, uiChunkEnd, &pvData, &uiDataLength, &iErrorCode, &pucErrorString)) != LWPS_NoError ) {
                    iUtlLogError(UTL_LOG_CONTEXT, "Failed to handle an lwps retrieval request, lwps error: %d.", iError);
                    goto bailFromvLScriptRunScript;
                }
                else {

                    /* Check the returned error code */
                    if ( iErrorCode != SPI_NoError ) {
                        iUtlLogError(UTL_LOG_CONTEXT, "Failed to handle an lwps retrieval request, returned error code: %d, error text: '%s.", 
                                iErrorCode, pucUtlStringsGetPrintableString(pucErrorString));
                        s_free(pucErrorString);
                        goto bailFromvLScriptRunScript;
                    }
                    else {

                        /* NULL terminate the data */
                        if ( (iError = iUtlStringsNullTerminateData(pvData, uiDataLength, (unsigned char **)&pvData)) != UTL_NoError ) {
                            iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to NULL terminate data, utl error: %d", iError);
                        }

                        /* Print the data */
                        printf("\n%s\n\n\n", (unsigned char *)pvData);
                        s_free(pvData);
                    }
                }

                break;


            /* Save document */
            case LSCRIPT_ACTION_SAVE_DOCUMENT:

                printf("Function - Save Document (document):\n");

                printf("Index: '%s', document key: '%s', item name: '%s', mime type: '%s'.\n",
                        pucIndexName, pucDocumentKey, pucItemName, pucMimeType);

                /* Retrieve the document */
                if ( (iError = iLwpsRetrievalRequestHandle(pvLwps, pucUtlStringsGetNULLString(pucIndexName), pucUtlStringsGetNULLString(pucDocumentKey), 
                        pucUtlStringsGetNULLString(pucItemName), pucUtlStringsGetNULLString(pucMimeType), SPI_CHUNK_TYPE_DOCUMENT,
                        0, 0, &pvData, &uiDataLength, &iErrorCode, &pucErrorString)) != LWPS_NoError ) {
                    iUtlLogError(UTL_LOG_CONTEXT, "Failed to handle an lwps retrieval request, lwps error: %d.", iError);
                    goto bailFromvLScriptRunScript;
                }
                else {

                    /* Check the returned error code */
                    if ( iErrorCode != SPI_NoError ) {
                        iUtlLogError(UTL_LOG_CONTEXT, "Failed to handle an lwps retrieval request, returned error code: %d, error text: '%s'.", 
                                iErrorCode, pucUtlStringsGetPrintableString(pucErrorString));
                        s_free(pucErrorString);
                        goto bailFromvLScriptRunScript;
                    }
                    else {

                        /* Open the file to write to */
                        if ( (pfFile = s_fopen(pucFilePath, "w")) == NULL ) {
                            iUtlLogError(UTL_LOG_CONTEXT, "Failed to open the file: '%s', to save the document.", pucFilePath);
                        }
                        else {

                             /* Save the document */
                            if ( s_fwrite(pvData, uiDataLength, 1, pfFile) != 1 ) {
                                 iUtlLogError(UTL_LOG_CONTEXT, "Failed to save to document to the file: '%s'.", pucFilePath);
                            }

                            /* Close the file */
                             s_fclose(pfFile);
                         }

                        s_free(pvData);
                    }
                }

                break;


            /* Add positive feedback document */
            case LSCRIPT_ACTION_ADD_POSITIVE_FEEDBACK:

                printf("Function - Adding positive feedback (document):\n");

                printf("Index: '%s', document key: '%s', item name: '%s', mime type: '%s'.\n",
                        pucIndexName, pucDocumentKey, pucItemName, pucMimeType);
            
                /* Retrieve the document */
                if ( (iError = iLwpsRetrievalRequestHandle(pvLwps, pucUtlStringsGetNULLString(pucIndexName), pucUtlStringsGetNULLString(pucDocumentKey), 
                        pucUtlStringsGetNULLString(pucItemName), pucUtlStringsGetNULLString(pucMimeType), SPI_CHUNK_TYPE_DOCUMENT,
                        0, 0, &pvData, &uiDataLength, &iErrorCode, &pucErrorString)) != LWPS_NoError ) {
                    iUtlLogError(UTL_LOG_CONTEXT, "Failed to handle an lwps retrieval request, lwps error: %d.", iError);
                    goto bailFromvLScriptRunScript;
                }
                else {

                    /* Check the returned error code */
                    if ( iErrorCode != SPI_NoError ) {
                        iUtlLogError(UTL_LOG_CONTEXT, "Failed to handle an lwps retrieval request, returned error code: %d, error text: '%s'.", 
                                iErrorCode, pucUtlStringsGetPrintableString(pucErrorString));
                        s_free(pucErrorString);
                        goto bailFromvLScriptRunScript;
                    }
                    else {

                        /* Set the positive feedback pointer */
                        if ( (iError = iUtlStringsAppendData(pucPositiveFeedbackText, pvData, uiDataLength, &pucPositiveFeedbackText)) != UTL_NoError ) {
                            iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to append to the positive feedback text, utl error: %d", iError);
                        }
                        if ( (iError = iUtlStringsAppendString(pucPositiveFeedbackText, " ", &pucPositiveFeedbackText)) != UTL_NoError ) {
                            iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to append to the positive feedback text, utl error: %d", iError);
                        }
                        s_free(pucData);
                    }
                }

                break;


            /* Add positive feedback text */
            case LSCRIPT_ACTION_ADD_POSITIVE_FEEDBACK_TEXT:

                printf("Function - Adding positive feedback (text): '%s'.\n", pucPositiveFeedbackText);
                break;

    
            /* Clear positive feedback text */
            case LSCRIPT_ACTION_CLEAR_POSITIVE_FEEDBACK:

                printf("Function - Clearing positive feedback text.\n");
                break;


            /* Add negative feedback text */
            case LSCRIPT_ACTION_ADD_NEGATIVE_FEEDBACK:

                printf("Function - Adding negative feedback (document):\n");

                printf("Index: '%s', document key: '%s', item name: '%s', mime type: '%s'.\n",
                        pucIndexName, pucDocumentKey, pucItemName, pucMimeType);
            
                /* Retrieve the document */
                if ( (iError = iLwpsRetrievalRequestHandle(pvLwps, pucUtlStringsGetNULLString(pucIndexName), pucUtlStringsGetNULLString(pucDocumentKey), 
                        pucUtlStringsGetNULLString(pucItemName), pucUtlStringsGetNULLString(pucMimeType), SPI_CHUNK_TYPE_DOCUMENT,
                        0, 0, &pvData, &uiDataLength, &iErrorCode, &pucErrorString)) != LWPS_NoError ) {
                    iUtlLogError(UTL_LOG_CONTEXT, "Failed to handle an lwps retrieval request, lwps error: %d.", iError);
                    goto bailFromvLScriptRunScript;
                }
                else {

                    /* Check the returned error code */
                    if ( iErrorCode != SPI_NoError ) {
                        iUtlLogError(UTL_LOG_CONTEXT, "Failed to handle an lwps retrieval request, returned error code: %d, error text: '%s'.", 
                                iErrorCode, pucUtlStringsGetPrintableString(pucErrorString));
                        s_free(pucErrorString);
                        goto bailFromvLScriptRunScript;
                    }
                    else {

                        /* Set the negative feedback pointer */
                        if ( (iError = iUtlStringsAppendData(pucNegativeFeedbackText, pvData, uiDataLength, &pucNegativeFeedbackText)) != UTL_NoError ) {
                            iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to append to the negative feedback text, utl error: %d", iError);
                        }
                        if ( (iError = iUtlStringsAppendString(pucNegativeFeedbackText, " ", &pucNegativeFeedbackText)) != UTL_NoError ) {
                            iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to append to the negative feedback text, utl error: %d", iError);
                        }
                        s_free(pucData);
                    }
                }

                break;

        
            /* Add negative feedback text */
            case LSCRIPT_ACTION_ADD_NEGATIVE_FEEDBACK_TEXT:

                printf("Function - Adding negative feedback (text): '%s'.\n", pucNegativeFeedbackText);
                break;

        
            /* Clear negative feedback text */
            case LSCRIPT_ACTION_CLEAR_NEGATIVE_FEEDBACK:

                printf("Function - Clearing negative feedback text.\n");
                break;


            /* Get server info */
            case LSCRIPT_ACTION_GET_SERVER_INFO:

                printf("Function - Get Server Info:\n");

                if ( (iError = iLwpsServerInfoRequestHandle(pvLwps, &pssiSpiServerInfo, &iErrorCode, &pucErrorString)) != LWPS_NoError ) {
                    iUtlLogError(UTL_LOG_CONTEXT, "Failed to handle an lwps server information request, lwps error: %d.", iError);
                    goto bailFromvLScriptRunScript;
                }
                else {
                    /* Check the returned error code */
                    if ( iErrorCode != SPI_NoError ) {
                        iUtlLogError(UTL_LOG_CONTEXT, "Failed to handle an lwps server information request, returned error code: %d, error text: '%s'.", 
                                iErrorCode, pucUtlStringsGetPrintableString(pucErrorString));
                        s_free(pucErrorString);
                        goto bailFromvLScriptRunScript;
                    }
                    else {

                        printf("Server Description:\n");

                        printf(" Server name             : '%s'\n", pucUtlStringsGetPrintableString(pssiSpiServerInfo->pucName));
                        printf(" Server desc             : '%s'\n", pucUtlStringsGetPrintableString(pssiSpiServerInfo->pucDescription));
                        printf(" Server admin name       : '%s'\n", pucUtlStringsGetPrintableString(pssiSpiServerInfo->pucAdminName));
                        printf(" Server admin email      : '%s'\n", pucUtlStringsGetPrintableString(pssiSpiServerInfo->pucAdminEmail));
                        printf(" Server index #       :  %u\n", pssiSpiServerInfo->uiIndexCount);
                        printf(" Server ranking algorithm: '%s'\n", pucUtlStringsGetPrintableString(pssiSpiServerInfo->pucRankingAlgorithm));
                        printf((pssiSpiServerInfo->dWeightMinimum == SPI_SERVER_WEIGHT_MAXIMUM) ? 
                                " Server min weight       : '-infinity'\n" : " Server min weight       : %.4f\n", pssiSpiServerInfo->dWeightMinimum);
                        printf((pssiSpiServerInfo->dWeightMaximum == SPI_SERVER_WEIGHT_MAXIMUM) ? 
                                " Server max weight       : '+infinity'\n" : " Server max weight       : %.4f\n", pssiSpiServerInfo->dWeightMaximum);

                        iSpiFreeServerInfo(pssiSpiServerInfo);
                        pssiSpiServerInfo  = NULL;
                    }
                }

                break;


            /* List all the index available on a server */
            case LSCRIPT_ACTION_GET_SERVER_INDEX_INFO:

                printf("Function - Get Server Index Info:\n");

                if ( (iError = iLwpsServerIndexInfoRequestHandle(pvLwps, &pssiiSpiServerIndexInfos, &uiSpiServerIndexInfosLength, 
                        &iErrorCode, &pucErrorString)) != LWPS_NoError ) {
                    iUtlLogError(UTL_LOG_CONTEXT, "Failed to handle an lwps server index information request, lwps error: %d.", iError);
                    goto bailFromvLScriptRunScript;
                }
                else {
                    /* Check the returned error code */
                    if ( iErrorCode != SPI_NoError ) {
                        iUtlLogError(UTL_LOG_CONTEXT, "Failed to handle an lwps server index information request, returned error code: %d, error text: '%s'.", 
                                iErrorCode, pucUtlStringsGetPrintableString(pucErrorString));
                        s_free(pucErrorString);
                        goto bailFromvLScriptRunScript;
                    }
                    else {

                        printf("Index Descriptions:\n");

                        printf(" Name               Description \n");
                        for ( uiI = 0, pssiiSpiServerIndexInfosPtr = pssiiSpiServerIndexInfos; uiI < uiSpiServerIndexInfosLength; uiI++, pssiiSpiServerIndexInfosPtr++ ) {
                            snprintf(pucLine, LSCRIPT_SHORT_LINE_LENGTH + 1, " '%%s'%%-%us'%%s'\n", 
                                    (unsigned int)UTL_MACROS_MAX(17 - s_strlen(pucUtlStringsGetPrintableString(pssiiSpiServerIndexInfosPtr->pucName)), 0));
                            printf(pucLine, pucUtlStringsGetPrintableString(pssiiSpiServerIndexInfosPtr->pucName), " ", 
                                    pucUtlStringsGetPrintableString(pssiiSpiServerIndexInfosPtr->pucDescription));
                        }
                        iSpiFreeServerIndexInfo(pssiiSpiServerIndexInfos, uiSpiServerIndexInfosLength);
                        pssiiSpiServerIndexInfos = NULL;
                    }
                }

                break;


            /* List the information available on a index */
            case LSCRIPT_ACTION_GET_INDEX_INFO:

                printf("Function - Get Index Info:\n");

                if ( (iError = iLwpsIndexInfoRequestHandle(pvLwps, pucUtlStringsGetNULLString(pucIndexName), 
                        &psiiSpiIndexInfo, &iErrorCode, &pucErrorString)) != LWPS_NoError ) {
                    iUtlLogError(UTL_LOG_CONTEXT, "Failed to handle an lwps index information request, lwps error: %d.", iError);
                    goto bailFromvLScriptRunScript;
                }
                else {
                    /* Check the returned error code */
                    if ( iErrorCode != SPI_NoError ) {
                        iUtlLogError(UTL_LOG_CONTEXT, "Failed to handle an lwps index information request, returned error code: %d, error text: '%s'.", 
                                iErrorCode, pucUtlStringsGetPrintableString(pucErrorString));
                        s_free(pucErrorString);
                        goto bailFromvLScriptRunScript;
                    }
                    else {

                        printf("Index Description:\n");

                        printf(" Index Name         : '%s'\n", pucUtlStringsGetPrintableString(psiiSpiIndexInfo->pucName));
                        printf(" Index Description  : '%s'\n", pucUtlStringsGetPrintableString(psiiSpiIndexInfo->pucDescription));
                        printf(" Language Code         : '%s'\n", pucUtlStringsGetPrintableString(psiiSpiIndexInfo->pucLanguageCode));
                        printf(" Tokenizer Name        : '%s'\n", pucUtlStringsGetPrintableString(psiiSpiIndexInfo->pucTokenizerName));
                        printf(" Stemmer Name          : '%s'\n", pucUtlStringsGetPrintableString(psiiSpiIndexInfo->pucStemmerName));
                        printf(" Stop List Name        : '%s'\n", pucUtlStringsGetPrintableString(psiiSpiIndexInfo->pucStopListName));
                        printf(" Document Count        :  %u\n", psiiSpiIndexInfo->uiDocumentCount);
                        printf(" Total Term Count      :  %lu\n", psiiSpiIndexInfo->ulTotalTermCount);
                        printf(" Unique Term Count     :  %lu\n", psiiSpiIndexInfo->ulUniqueTermCount);
                        printf(" Total Stop Term Count :  %lu\n", psiiSpiIndexInfo->ulTotalStopTermCount);
                        printf(" Unique Stop Term Count:  %lu\n", psiiSpiIndexInfo->ulUniqueStopTermCount);
                        printf(" Access Control        :  %u\n", psiiSpiIndexInfo->uiAccessControl);
                        printf(" Update Frequency      :  %u\n", psiiSpiIndexInfo->uiUpdateFrequency);
                        printf(" Last Update Ansi Date :  %lu\n", psiiSpiIndexInfo->ulLastUpdateAnsiDate);
                        printf(" Case Sensitive        :  %u\n", psiiSpiIndexInfo->uiCaseSensitive);
                        
                        iSpiFreeIndexInfo(psiiSpiIndexInfo);
                        psiiSpiIndexInfo = NULL;
                    }
                }

                break;


            /* List all the field available for a index */
            case LSCRIPT_ACTION_GET_INDEX_FIELD_INFO:

                printf("Function - Get Index Field Info:\n");

                if ( (iError = iLwpsIndexFieldInfoRequestHandle(pvLwps, pucUtlStringsGetNULLString(pucIndexName), 
                        &psfiSpiFieldInfos, &uiSpiFieldInfosLength, &iErrorCode, &pucErrorString)) != LWPS_NoError ) {
                    iUtlLogError(UTL_LOG_CONTEXT, "Failed to handle an lwps index field information request, lwps error: %d.", iError);
                    goto bailFromvLScriptRunScript;
                }
                else {
                    /* Check the returned error code */
                    if ( iErrorCode != SPI_NoError ) {
                        iUtlLogError(UTL_LOG_CONTEXT, "Failed to handle an lwps index field information request, returned error code: %d, error text: '%s'.", 
                                iErrorCode, pucUtlStringsGetPrintableString(pucErrorString));
                        s_free(pucErrorString);
                        goto bailFromvLScriptRunScript;
                    }
                    else {

                        printf("Index Field Descriptions:\n");

                        printf(" Name               Description                                 Type\n");
                        for ( uiI = 0, psfiSpiFieldInfosPtr = psfiSpiFieldInfos; uiI < uiSpiFieldInfosLength; uiI++, psfiSpiFieldInfosPtr++ ) {
                            
                            snprintf(pucLine, LSCRIPT_SHORT_LINE_LENGTH + 1, " '%%s'%%-%us'%%s'%%-%us%%u\n", 
                                    (unsigned int)UTL_MACROS_MAX(17 - s_strlen(pucUtlStringsGetPrintableString(psfiSpiFieldInfosPtr->pucName)), 0),
                                    (unsigned int)UTL_MACROS_MAX(42 - s_strlen(pucUtlStringsGetPrintableString(psfiSpiFieldInfosPtr->pucDescription)), 0));
                            printf(pucLine, pucUtlStringsGetPrintableString(psfiSpiFieldInfosPtr->pucName), " ",
                                    pucUtlStringsGetPrintableString(psfiSpiFieldInfosPtr->pucDescription), " ", psfiSpiFieldInfosPtr->uiType);
                        }
                        iSpiFreeIndexFieldInfo(psfiSpiFieldInfos, uiSpiFieldInfosLength);
                        psfiSpiFieldInfos = NULL;
                    }
                }

                break;


            /* List all the term available for a index */
            case LSCRIPT_ACTION_GET_INDEX_TERM_INFO:

                printf("Function - Get Index Term Info:\n");

                if ( (iError = iLwpsIndexTermInfoRequestHandle(pvLwps, pucUtlStringsGetNULLString(pucIndexName), 
                        uiTermMatch, uiTermCase, pucUtlStringsGetNULLString(pucTerm), pucUtlStringsGetNULLString(pucFieldName), 
                        &pstiSpiTermInfosPtr, &uiSpiTermInfosLength, &iErrorCode, &pucErrorString)) != LWPS_NoError ) {
                    iUtlLogError(UTL_LOG_CONTEXT, "Failed to handle an lwps index term information request, lwps error: %d.", iError);
                    goto bailFromvLScriptRunScript;
                }
                else {
                    /* Check the returned error code */
                    if ( iErrorCode != SPI_NoError ) {
                        iUtlLogError(UTL_LOG_CONTEXT, "Failed to handle an lwps index term information request, returned error code: %d, error text: '%s'.", 
                                iErrorCode, pucUtlStringsGetPrintableString(pucErrorString));
                        s_free(pucErrorString);
                        goto bailFromvLScriptRunScript;
                    }
                    else {

                        printf("Index Terms:\n");
                        printf(" Term                                         Category   TermCount   DocCount\n");

                        for ( uiI = 0, pstiSpiTermInfosPtr = pstiSpiTermInfos; uiI < uiSpiTermInfosLength; uiI++, pstiSpiTermInfosPtr++ ) {
                            snprintf(pucLine, LSCRIPT_SHORT_LINE_LENGTH + 1, " '%%s'%%-%us%%12u%%12u%%12u\n", 
                                    (unsigned int)UTL_MACROS_MAX(32 - s_strlen(pstiSpiTermInfosPtr->pucTerm), 0));
                            printf(pucLine, pstiSpiTermInfosPtr->pucTerm, " ",
                                    pstiSpiTermInfosPtr->uiType, pstiSpiTermInfosPtr->uiCount, pstiSpiTermInfosPtr->uiDocumentCount);
                        }
                        iSpiFreeTermInfo(pstiSpiTermInfos, uiSpiTermInfosLength);
                        pstiSpiTermInfos = NULL;
                    }
                }

                break;


            /* List the document information */
            case LSCRIPT_ACTION_GET_DOCUMENT_INFO:

                printf("Function - Get Document Info:\n");

                /* Get the document information */
                if ( (iError = iLwpsDocumentInfoRequestHandle(pvLwps, pucUtlStringsGetNULLString(pucIndexName), pucUtlStringsGetNULLString(pucDocumentKey), 
                        &psdiSpiDocumentInfo, &iErrorCode, &pucErrorString)) != LWPS_NoError ) {
                    iUtlLogError(UTL_LOG_CONTEXT, "Failed to handle an lwps document information request, lwps error: %d.", iError);
                    goto bailFromvLScriptRunScript;
                }
                else {

                    /* Check the returned error code */
                    if ( iErrorCode != SPI_NoError ) {
                        iUtlLogError(UTL_LOG_CONTEXT, "Failed to handle an lwps document information request, returned error code: %d, error text: '%s'.", 
                                iErrorCode, pucUtlStringsGetPrintableString(pucErrorString));
                        s_free(pucErrorString);
                        goto bailFromvLScriptRunScript;
                    }
                    else {

                        /* Are there any document items for this document? */
                        if ( psdiSpiDocumentInfo->psdiSpiDocumentItems != NULL ) {
                            pucDocItems[0] = '\0';

                            /* Loop through each item, assemble a readable string from them */
                            for ( uiJ = 0, psdiSpiDocumentItemsPtr = psdiSpiDocumentInfo->psdiSpiDocumentItems; uiJ < psdiSpiDocumentInfo->uiDocumentItemsLength; uiJ++, psdiSpiDocumentItemsPtr++ ) {
                                
                                if ( (uiJ > 0) && (uiJ < psdiSpiDocumentInfo->uiDocumentItemsLength) ) {
                                    s_strnncat(pucDocItems, "; ", LSCRIPT_SHORT_LINE_LENGTH, LSCRIPT_SHORT_LINE_LENGTH + 1);
                                }
                                
                                s_strnncat(pucDocItems, psdiSpiDocumentItemsPtr->pucItemName, LSCRIPT_SHORT_LINE_LENGTH, LSCRIPT_SHORT_LINE_LENGTH + 1);
                                s_strnncat(pucDocItems, ", ", LSCRIPT_SHORT_LINE_LENGTH, LSCRIPT_SHORT_LINE_LENGTH + 1);

                                s_strnncat(pucDocItems, psdiSpiDocumentItemsPtr->pucMimeType, LSCRIPT_SHORT_LINE_LENGTH, LSCRIPT_SHORT_LINE_LENGTH + 1);
                                s_strnncat(pucDocItems, ", ", LSCRIPT_SHORT_LINE_LENGTH, LSCRIPT_SHORT_LINE_LENGTH + 1);

                                snprintf(pucNum, LSCRIPT_SHORT_LINE_LENGTH + 1, "%u", psdiSpiDocumentItemsPtr->uiLength);
                                s_strnncat(pucDocItems, pucNum, LSCRIPT_SHORT_LINE_LENGTH, LSCRIPT_SHORT_LINE_LENGTH + 1);
                            } 
                        }
                        else {
                            /* No document items are defined */
                            s_strnncpy(pucDocItems, "(undefined)", LSCRIPT_SHORT_LINE_LENGTH);
                        }

                        printf("Index: '%s', document key: '%s', title: '%s', language code: '%s', rank: %u, term count: %u, ansi date: %lu, items: '%s'.\n", 
                                psdiSpiDocumentInfo->pucIndexName, psdiSpiDocumentInfo->pucDocumentKey, psdiSpiDocumentInfo->pucTitle, 
                                pucUtlStringsGetPrintableString(psdiSpiDocumentInfo->pucLanguageCode), psdiSpiDocumentInfo->uiRank, 
                                psdiSpiDocumentInfo->uiTermCount, psdiSpiDocumentInfo->ulAnsiDate, pucDocItems);

                        /* Free the document information */
                        iSpiFreeDocumentInfo(psdiSpiDocumentInfo);
                        psdiSpiDocumentInfo = NULL;
                    }
                }

                break;


            /* Sleep */
            case LSCRIPT_ACTION_SLEEP:

                printf("Sleep: %d second%s\n", iSleep, (iSleep == 1) ? "" : "s");
                if ( iSleep > 0 ) {
                    s_sleep(iSleep);
                }
                break;


            /* Skip */
            case LSCRIPT_ACTION_SKIP:

                printf("Action - Skip\n");
                bSkip = true;
                break;


            /* Resume */
            case LSCRIPT_ACTION_RESUME:

                printf("Action - Resume\n");
                bSkip = false;
                break;


            /* default  */
            default:

                iUtlLogWarn(UTL_LOG_CONTEXT, "Invalid line: %u, line text: '%s'.", uiLineCounter, pucLine);
                break;


        }  /* Switch ( plsaLScriptActionsPtr->iAction ) */

    }    /* While ( true ) */



    /* Bail label */
    bailFromvLScriptRunScript:


    /* Close the script file */
    if ( bUtlStringsIsStringNULL(plslScript->pucScriptFilePath) == false ) {
        s_fclose(pfScriptFile);
    }


    return;
}


/*---------------------------------------------------------------------------*/


