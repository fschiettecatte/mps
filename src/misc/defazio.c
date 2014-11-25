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

    Module:     defazio.c

    Author:     Francois Schiettecatte (FS Consulting LLC.)

    Created:    17 September 1997

    Purpose:    This application runs a deFazio benchmark on the MPS
                search engine. A level of abstraction has been provided
                over the MPS SPI so that it could be ported to other
                search engines. Currently this abstraction layer just
                passes the calls through to the SPI.

*/


/*---------------------------------------------------------------------------*/


/*
** Includes
*/

#include "utils.h"

#include "lng.h"

#include "spi.h"

#include "lwps.h"


/*---------------------------------------------------------------------------*/


/*
** Feature defines
*/

/* Context for logging */
#undef UTL_LOG_CONTEXT
#define UTL_LOG_CONTEXT                     (unsigned char *)"com.fsconsult.mps.src.misc.defazio"


/*---------------------------------------------------------------------------*/


/*
** Defines
*/

/* Default index directory path */
#define DF_INDEX_DIRECTORY_PATH_DEFAULT             (unsigned char *)"./"

/* Default configuration directory path */
#define DF_CONFIGURATION_DIRECTORY_PATH_DEFAULT     DF_INDEX_DIRECTORY_PATH_DEFAULT

/* Default temporary directory path */
#define DF_TEMPORARY_DIRECTORY_PATH_DEFAULT         (unsigned char *)P_tmpdir


#define DF_MINUTES_TO_RUN_DEFAULT                   (60)                /* Defaults to 1 hour */
#define DF_CLIENTS_TO_RUN_DEFAULT                   (1)                 /* Defaults to 1 client */

#define DF_SEARCH_TERM_COUNT_MAX                    (50)                /* Should be 50 */
#define DF_RETRIEVAL_COUNT                          (10)                /* Should be 10 */

#define DF_SEARCH_PAR_TIME                          (20)                /* Seconds, should be 20 */
#define DF_RETRIEVAL_PAR_TIME                       (2)                 /* Seconds, should be 2 */


#define DF_SEARCH_RESIDENCE_TIME_MAX                (30)                /* Seconds, should be 30 */
#define DF_RETRIEVAL_RESIDENCE_TIME_MAX             (5)                 /* Seconds, should be 5 */


/* Maximum number of documents IDs to retrieve for retrieval benchmark */
#define DF_DOCUMENT_ID_COUNT_MAX                    (100001)


/* Maximum length of a search */
#define DF_SEARCH_LENGTH_MAX                        (DF_SEARCH_TERM_COUNT_MAX * 100)


/* Sleep interval when we are printing the countdown */
#define DF_PRINTING_COUNTDOWN_SLEEP                 (1)


/* Default locale name */
#define DF_LOCALE_NAME_DEFAULT                      LNG_LOCALE_EN_US_UTF_8_NAME


/* Defaults */
#define DF_HOST_DEFAULT                             LWPS_PROTOCOL_HOST_DEFAULT
#define DF_PORT_DEFAULT                             LWPS_PROTOCOL_PORT_DEFAULT
#define DF_TIMEOUT_DEFAULT                          LWPS_PROTOCOL_TIMEOUT_DEFAULT


/* Default protocol name and ID */
#define DF_NET_PROTOCOL_NAME_DEFAULT                UTL_NET_PROTOCOL_TCP_NAME
#define DF_NET_PROTOCOL_ID_DEFAULT                  UTL_NET_PROTOCOL_TCP_ID


/*---------------------------------------------------------------------------*/


/*
** Structures
*/


/* Connection structure */
struct dfConnection {
    void                    *pvLwps;
    void                    *pvUtlNet;
};


/* Term info structure */
struct dfTermInfo {
    unsigned char           *pucTerm;
    unsigned int            uiCount;
};


/* Document key structure */
struct dfDocumentKey {
    unsigned char           *pucDocumentKey;
};


/* Server information structure */
struct dfServerInfo {
    unsigned char           *pucIndexDirectoryPath;
    unsigned char           *pucConfigurationDirectoryPath;
    unsigned char           *pucTemporaryDirectoryPath;
    unsigned char           *pucIndexName;
    unsigned int            uiNetProtocolID;
    unsigned char           *pucHostName;
    int                     iPort;
};


/* Index information structure */
struct dfIndexInfo {
    struct dfTermInfo       *pdtiDfTermInfo;
    unsigned int            uiDfTermInfoLength;
    unsigned int            uiTermInfoIndex90;
    unsigned int            uiTermInfoIndex5;
    struct dfDocumentKey    *pddkDfDocumentKey;
    unsigned int            uiDfDocumentIDLength;
};


/* Results information structure */
struct dfResultsInfo {
    double                  dTotalSearchTime;
    double                  dTotalRetrievalTime;
    unsigned int            uiSearchesAbovePar;
    unsigned int            uiRetrievalsAbovePar;
    unsigned int            uiTotalSearchesAttempted;
    unsigned int            uiTotalSearchesRun;
    unsigned int            uiTotalRetrievalsAttempted;
    unsigned int            uiTotalRetrievalsRun;
    unsigned int            *puiSearchResidenceTime;
    unsigned int            *puiRetrievalResidenceTime;
};


/* Benchmark information structure */
struct dfBenchmarkInfo {
    struct dfServerInfo     *pdsiDfServerInfo;
    struct dfIndexInfo      *pdiiDfIndexInfo;
    struct dfResultsInfo    *pdriDfResultsInfo;
    unsigned int            uiMinutesToRun;
};


/*---------------------------------------------------------------------------*/


/*
** Defines (defined here because it needs the dfTermInfo structure)
*/

/* Macro to copy a term info, this just hands over the pointers,
** it does not copy data
*/
#define DF_COPY_TERM_INFO(pti1, pti2) \
    {   \
        (pti1)->pucTerm = (pti2)->pucTerm; \
        (pti1)->uiCount = (pti2)->uiCount; \
    }


#define DF_SWAP_TERM_INFO(pti1, pti2)    \
    {   \
        struct dfTermInfo tiTermInfoMacro; \
            DF_COPY_TERM_INFO(&tiTermInfoMacro, pti1); \
        DF_COPY_TERM_INFO(pti1, pti2); \
        DF_COPY_TERM_INFO(pti2, &tiTermInfoMacro); \
    }


/*---------------------------------------------------------------------------*/


/*
** Private function prototypes
*/

static void vUsage (unsigned char *pucCommandPath);
static void vVersion (void);


static void vDfDeFazioBenchmark (unsigned char *pucIndexDirectoryPath, 
        unsigned char *pucConfigurationDirectoryPath, unsigned char *pucTemporaryDirectoryPath, 
        unsigned char *pucIndexName, unsigned int uiNetProtocolID, unsigned char *pucHostName, 
        int iPort, unsigned int uiClientsToRun, unsigned int uiMinutesToRun);

static int iDfBenchmarkSetup (struct dfServerInfo *pdsiDfServerInfo, 
        struct dfIndexInfo **ppdiiDfIndexInfo);

static int iDfBenchmarkRun (struct dfBenchmarkInfo *pdbiDfBenchmarkInfo);

static int iDfBenchmarkPrintTimeToGo (unsigned int uiMinutesToRun);

static int iDfBenchmarkPrintResults (struct dfResultsInfo *pdriDfResultsInfo);


static int iDfInitializeServer (unsigned char *pucIndexDirectoryPath,
        unsigned char *pucConfigurationDirectoryPath, unsigned char *pucTemporaryDirectoryPath,
        unsigned int uiNetProtocolID, unsigned char *pucHostName, int iPort, 
        struct spiSession **ppssSpiSession, struct dfConnection **ppdcConnection);

static int iDfShutdownServer (struct spiSession *pssSpiSession, 
        struct dfConnection *pdcConnection);

static int iDfOpenIndex (struct spiSession *pssSpiSession, struct dfConnection *pdcConnection,
        unsigned char *pucIndexName, void **ppvIndex);

static int iDfCloseIndex (struct spiSession *pssSpiSession, struct dfConnection *pdcConnection, 
        void *pvIndex);

static int iDfGetIndexInfo (struct spiSession *pssSpiSession, struct dfConnection *pdcConnection,
        void *pvIndex, unsigned int *puiDocumentCount, unsigned long *pulTotalTermCount, 
        unsigned long *pulUniqueTermCount, unsigned long *pulTotalStopTermCount,
        unsigned long *pulUniqueStopTermCount);

static int iDfGetIndexTermInfo (struct spiSession *pssSpiSession, struct dfConnection *pdcConnection, 
        void *pvIndex, struct dfTermInfo **ppdtiDFTermInfo, unsigned int *puiDfTermInfoLength);

static int iDfGetIndexDocumentIDs (struct spiSession *pssSpiSession, struct dfConnection *pdcConnection,
        void *pvIndex, unsigned char *pucLanguageCode, unsigned char *pucSearchText, 
        struct dfDocumentKey **ppddkDfDocumentKey, unsigned int *puiDfDocumentIDLength);

static int iDfSearchIndex (struct spiSession *pssSpiSession, struct dfConnection *pdcConnection,
        void *pvIndex, unsigned char *pucLanguageCode, unsigned char *pucSearchText, 
        unsigned int *puiHitsCount);

static int iDfRetrieveDocument (struct spiSession *pssSpiSession, struct dfConnection *pdcConnection,
        void *pvIndex, unsigned char *pucDocumentKey);


static void vDfSortTermInfoDesc (struct dfTermInfo *pdtiDfTermInfo, 
        int iLocalLeftIndex, int iLocalRightIndex);

static int iDfGetRandomTermInfoIndex (unsigned int uiDfTermInfoLength, 
        unsigned int uiTermInfoIndex90, unsigned int uiTermInfoIndex5);



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
    unsigned char   pucTemporaryDirectoryPath[UTL_FILE_PATH_MAX + 1] = {'\0'};

    unsigned int    uiNetProtocolID = DF_NET_PROTOCOL_ID_DEFAULT;
    unsigned char   *pucHostName = DF_HOST_DEFAULT;
    int             iPort = DF_PORT_DEFAULT;
    unsigned char   *pucIndexName = NULL;
    unsigned int    uiClientsToRun = DF_CLIENTS_TO_RUN_DEFAULT;
    unsigned int    uiMinutesToRun = DF_MINUTES_TO_RUN_DEFAULT;

    unsigned char   *pucLocaleName = DF_LOCALE_NAME_DEFAULT;

    unsigned char   *pucLogFilePath = UTL_LOG_FILE_STDERR;    
    unsigned int    uiLogLevel = UTL_LOG_LEVEL_INFO;



    /* Set defaults */
    s_strnncpy(pucIndexDirectoryPath, DF_INDEX_DIRECTORY_PATH_DEFAULT, UTL_FILE_PATH_MAX + 1);
    s_strnncpy(pucConfigurationDirectoryPath, DF_CONFIGURATION_DIRECTORY_PATH_DEFAULT, UTL_FILE_PATH_MAX + 1);
    s_strnncpy(pucTemporaryDirectoryPath, DF_TEMPORARY_DIRECTORY_PATH_DEFAULT, UTL_FILE_PATH_MAX + 1);


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

    /* Cycle through all the arguments */
    while( pucNextArgument != NULL ) {

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

        /* Check for socket */
        else if ( s_strncmp("--socket=", pucNextArgument, s_strlen("--socket=")) == 0 ) {
        
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
        
        /* Check for index name */
        else if ( s_strncmp("--index=", pucNextArgument, s_strlen("--index=")) == 0 ) {

            /* Get the index name */
            pucNextArgument += s_strlen("--index=");

            /* Set the index name */
            pucIndexName = pucNextArgument;
        }

        /* Check for clients */
        else if ( s_strncmp("--clients=", pucNextArgument, s_strlen("--clients=")) == 0 ) {

            /* Get the number of clients to run */
            pucNextArgument += s_strlen("--clients=");

            /* Check to see if the number of clients is valid */
            if ( s_strtol(pucNextArgument, NULL, 10) <= 0 ) {
                vVersion();
                iUtlLogPanic(UTL_LOG_CONTEXT, "Expected the number of client to be greater than or equal to: 0");
            }

            /* Set the clients to run */
            uiClientsToRun = s_strtol(pucNextArgument, NULL, 10);
        }    

        /* Check for time */
        else if ( s_strncmp("--time=", pucNextArgument, s_strlen("--time=")) == 0 ) {

            /* Get the time */
            pucNextArgument += s_strlen("--time=");

            /* Check to see if the number of minutes is valid */
            if ( s_strtol(pucNextArgument, NULL, 10) <= 0 ) {
                vVersion();
                iUtlLogPanic(UTL_LOG_CONTEXT, "Expected the number of minutes to be greater than or equal to: 0");
            }

            /* Set the minutes to run */
            uiMinutesToRun = s_strtol(pucNextArgument, NULL, 10);

            /* Check the period */
            if ( pucNextArgument[s_strlen(pucNextArgument) - 1] == 'h' ) {
                /* Hours */
                uiMinutesToRun *= 60;
            }
            else if ( pucNextArgument[s_strlen(pucNextArgument) - 1] == 'd' ) {
                /* Days */
                uiMinutesToRun *= 60 * 24;
            }
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


    /* Check that a index was specified */
    if ( bUtlStringsIsStringNULL(pucIndexName) == true ) {
        iUtlLogPanic(UTL_LOG_CONTEXT, "A index name is required");
    }


    /* Go for it */
    vDfDeFazioBenchmark(pucIndexDirectoryPath, pucConfigurationDirectoryPath, pucTemporaryDirectoryPath,
            pucIndexName, uiNetProtocolID, pucHostName, iPort, uiClientsToRun, uiMinutesToRun);


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
    printf("deFazio Benchmark, %s\n", UTL_VERSION_COPYRIGHT_STRING);


    /* Get the version string */
    iUtlVersionGetVersionString(pucVersionString, UTL_VERSION_STRING_LENGTH + 1);

    /* Version message */
    printf("%s\n", pucVersionString);


    return;

}


/*---------------------------------------------------------------------------*/


/*

    Function:   vUsage()

    Purpose:    This function list out all the parameters that defazio
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
    printf("                  Configuration directory, defaults to: '%s'.\n", DF_CONFIGURATION_DIRECTORY_PATH_DEFAULT);
    printf("  --index-directory=name \n");
    printf("                  Index directory, defaults to: '%s'.\n", DF_INDEX_DIRECTORY_PATH_DEFAULT);
    printf("  --temporary-directory=name \n");
    printf("                  Temporary directory, defaults to: '%s'.\n", DF_TEMPORARY_DIRECTORY_PATH_DEFAULT);
    printf("  --socket=protocol:host:port | --socket=protocol:port | --socket=host:port | --socket=port \n");
    printf("                  Socket to connect to, using a protocol, host and port combination, \n");
    printf("                  protocol defaults to: '%s', procotols available: '%s', '%s', \n", 
            DF_NET_PROTOCOL_NAME_DEFAULT, UTL_NET_PROTOCOL_TCP_NAME, UTL_NET_PROTOCOL_UDP_NAME);
    printf("                  host defaults to: '%s', port defaults to: %d, \n", DF_HOST_DEFAULT, DF_PORT_DEFAULT);
    printf("                  optional - for remote access only. \n");
    printf("  --index=name \n");
    printf("                  Index name, required. \n");
    printf("  --clients=#     Number of clients to run, defaults to: %d client, \n", DF_CLIENTS_TO_RUN_DEFAULT);
    printf("  --time=#        Number of minutes to run, defaults to: %d minutes, \n", DF_MINUTES_TO_RUN_DEFAULT);
    printf("                  default is in minutes, otherwise h or d can be appended to the \n");
    printf("                  number to specify hours or days respectively, for example: \n");
    printf("                      -time 20    indicates 20 minutes, \n");
    printf("                      -time 20h   indicates 20 hours, \n");
    printf("                      -time 20d   indicates 20 days, \n");
/*     printf("  --dontstop      Dont stop on search or retrieval errors, default is to stop.\n"); */
    printf("\n");

    printf(" Locale parameter: \n");
    printf("  --locale=name   Locale name, defaults to '%s', see 'locale' for list of \n", DF_LOCALE_NAME_DEFAULT);
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
** =============================
** ===  Benchmark Functions  ===
** =============================
*/


/*

    Function:   vDfDeFazioBenchmark()

    Purpose:    Runs the deFazio benchmark on the index, the sequence of
                events is as follows:
                    - initialize and load the various structures
                    - run the benchmark
                    - print the results
                    - free any allocated resources

    Parameters: pucIndexDirectoryPath           index directory path (optional)
                pucConfigurationDirectoryPath   configuration directory path (optional)
                pucTemporaryDirectoryPath       temporary directory path (optional)
                pucIndexName                    index name (required)
                uiNetProtocolID                 network protocol (optional)
                pucHostName                     host name (optional)
                iPort                           port (optional)
                uiClientsToRun                  number of clients to run
                uiMinutesToRun                  number of minutes to run

    Globals:    none

    Returns:    void

*/
static void vDfDeFazioBenchmark
(
    unsigned char *pucIndexDirectoryPath,
    unsigned char *pucConfigurationDirectoryPath,
    unsigned char *pucTemporaryDirectoryPath,
    unsigned char *pucIndexName,
    unsigned int uiNetProtocolID,
    unsigned char *pucHostName,
    int iPort,
    unsigned int uiClientsToRun,
    unsigned int uiMinutesToRun
)
{


    int                         iError = SPI_NoError;
    unsigned int                uiI = 0, uiJ = 0;
    struct dfBenchmarkInfo      *pdbiDfBenchmarkInfoMaster = NULL;
    struct dfBenchmarkInfo      *pdbiDfBenchmarkInfo = NULL;
    struct dfBenchmarkInfo      *pdbiDfBenchmarkInfoPtr = NULL;

    pthread_t                   *ptThreads = NULL;
    int                         iStatus = 0;
    void                        *pvStatus = NULL;


    ASSERT((bUtlStringsIsStringNULL(pucIndexDirectoryPath) == false) || (bUtlStringsIsStringNULL(pucIndexDirectoryPath) == true));
    ASSERT((bUtlStringsIsStringNULL(pucConfigurationDirectoryPath) == false) || (bUtlStringsIsStringNULL(pucConfigurationDirectoryPath) == true));
    ASSERT((bUtlStringsIsStringNULL(pucTemporaryDirectoryPath) == false) || (bUtlStringsIsStringNULL(pucTemporaryDirectoryPath) == true));
    ASSERT(bUtlStringsIsStringNULL(pucIndexName) == false);
    ASSERT((uiNetProtocolID == UTL_NET_PROTOCOL_TCP_ID) || (uiNetProtocolID == UTL_NET_PROTOCOL_UDP_ID));
    ASSERT(((bUtlStringsIsStringNULL(pucHostName) == false) && (iPort > 0)) || ((bUtlStringsIsStringNULL(pucHostName) == true) && (iPort <= 0)));
    ASSERT(uiClientsToRun > 0);
    ASSERT(uiMinutesToRun > 0);


    /* Initialize the random seed */
    iUtlRandSetSeed(s_time(NULL));


    /* Allocate the threads array - one per client and one for the countdown thread */
    if ( (ptThreads = (pthread_t *)s_malloc((size_t)(sizeof(pthread_t) * (uiClientsToRun + 1)))) == NULL ) {
        iError = SPI_MemError;
        goto bailFromvDfDeFazio;
    }



    /* Allocate the master benchmark information structure */
    if ( (pdbiDfBenchmarkInfoMaster = (struct dfBenchmarkInfo *)s_malloc((size_t)(sizeof(struct dfBenchmarkInfo)))) == NULL ) {
        iError = SPI_MemError;
        goto bailFromvDfDeFazio;
    }

    /* Allocate the server information structure */
    if ( (pdbiDfBenchmarkInfoMaster->pdsiDfServerInfo = (struct dfServerInfo *)s_malloc((size_t)(sizeof(struct dfServerInfo)))) == NULL ) {
        iError = SPI_MemError;
        goto bailFromvDfDeFazio;
    }

    /* Allocate the master results information structure */
    if ( (pdbiDfBenchmarkInfoMaster->pdriDfResultsInfo = (struct dfResultsInfo *)s_malloc((size_t)(sizeof(struct dfResultsInfo)))) == NULL ) {
        iError = SPI_MemError;
        goto bailFromvDfDeFazio;
    }

    /* Allocate the master search residence time array */
    if ( (pdbiDfBenchmarkInfoMaster->pdriDfResultsInfo->puiSearchResidenceTime = (unsigned int *)s_malloc((size_t)(sizeof(unsigned int *) * (DF_SEARCH_RESIDENCE_TIME_MAX + 1)))) == NULL ) {
        iError = SPI_MemError;
        goto bailFromvDfDeFazio;
    }

    /* Allocate the master retrieval residence time array */
    if ( (pdbiDfBenchmarkInfoMaster->pdriDfResultsInfo->puiRetrievalResidenceTime = (unsigned int *)s_malloc((size_t)(sizeof(unsigned int *) * (DF_RETRIEVAL_RESIDENCE_TIME_MAX + 1)))) == NULL ) {
        iError = SPI_MemError;
        goto bailFromvDfDeFazio;
    }


    /* Allocate the benchmark information structure array - one per client */
    if ( (pdbiDfBenchmarkInfo = (struct dfBenchmarkInfo *)s_malloc((size_t)(sizeof(struct dfBenchmarkInfo) * uiClientsToRun))) == NULL ) {
        iError = SPI_MemError;
        goto bailFromvDfDeFazio;
    }




    /* Set the fields in the server information structure */
    if ( bUtlStringsIsStringNULL(pucIndexDirectoryPath) == false ) {
        if ( (pdbiDfBenchmarkInfoMaster->pdsiDfServerInfo->pucIndexDirectoryPath = (unsigned char *)s_strdup(pucIndexDirectoryPath)) == NULL ) {
            iError = SPI_MemError;
            goto bailFromvDfDeFazio;
        }
    }
    if ( bUtlStringsIsStringNULL(pucConfigurationDirectoryPath) == false ) {
        if ( (pdbiDfBenchmarkInfoMaster->pdsiDfServerInfo->pucConfigurationDirectoryPath = (unsigned char *)s_strdup(pucConfigurationDirectoryPath)) == NULL ) {
            iError = SPI_MemError;
            goto bailFromvDfDeFazio;
        }
    }
    if ( bUtlStringsIsStringNULL(pucTemporaryDirectoryPath) == false ) {
        if ( (pdbiDfBenchmarkInfoMaster->pdsiDfServerInfo->pucTemporaryDirectoryPath = (unsigned char *)s_strdup(pucTemporaryDirectoryPath)) == NULL ) {
            iError = SPI_MemError;
            goto bailFromvDfDeFazio;
        }
    }
    if ( bUtlStringsIsStringNULL(pucIndexName) == false ) {
        if ( (pdbiDfBenchmarkInfoMaster->pdsiDfServerInfo->pucIndexName = (unsigned char *)s_strdup(pucIndexName)) == NULL ) {
            iError = SPI_MemError;
            goto bailFromvDfDeFazio;
        }
    }
    pdbiDfBenchmarkInfoMaster->pdsiDfServerInfo->uiNetProtocolID = uiNetProtocolID;
    if ( bUtlStringsIsStringNULL(pucHostName) == false ) {
        if ( (pdbiDfBenchmarkInfoMaster->pdsiDfServerInfo->pucHostName = (unsigned char *)s_strdup(pucHostName)) == NULL ) {
            iError = SPI_MemError;
            goto bailFromvDfDeFazio;
        }
    }
    pdbiDfBenchmarkInfoMaster->pdsiDfServerInfo->iPort = iPort;

    /* Set the fields in the benchmark information structure */
    pdbiDfBenchmarkInfoMaster->uiMinutesToRun = uiMinutesToRun;




    /* Set up the benchmark */
    if ( (iError = iDfBenchmarkSetup(pdbiDfBenchmarkInfoMaster->pdsiDfServerInfo, &pdbiDfBenchmarkInfoMaster->pdiiDfIndexInfo)) != SPI_NoError ) {
        goto bailFromvDfDeFazio;
    }




    /* Print up the benchmark profile */
    printf("Benchmark Profile:");
    printf(" Number of clients to run          :%15u\n", uiClientsToRun);
    printf(" Minutes to run                    :%15u\n", pdbiDfBenchmarkInfoMaster->uiMinutesToRun);
    printf(" Max search terms per search       :%15d\n",  DF_SEARCH_TERM_COUNT_MAX);
    printf(" Document Retrievals per iteration :%15d\n",  DF_RETRIEVAL_COUNT);
    printf("\n");
    printf("Running the benchmark...\n");
    printf("\n");




    /* Start the threads which will run the benchmark */
    for ( uiI = 0, pdbiDfBenchmarkInfoPtr = pdbiDfBenchmarkInfo; uiI < uiClientsToRun; uiI++, pdbiDfBenchmarkInfoPtr++ ) {

        /* Set the benchmark information from the master benchmark information,
        ** note that we dont duplicate the pointers, this means we have to be 
        ** careful what we free later on
        */
        pdbiDfBenchmarkInfoPtr->pdiiDfIndexInfo = pdbiDfBenchmarkInfoMaster->pdiiDfIndexInfo;
        pdbiDfBenchmarkInfoPtr->pdsiDfServerInfo = pdbiDfBenchmarkInfoMaster->pdsiDfServerInfo;
        pdbiDfBenchmarkInfoPtr->uiMinutesToRun = pdbiDfBenchmarkInfoMaster->uiMinutesToRun;

        /* Create the thread */
        if ( (iStatus = s_pthread_create(&ptThreads[uiI], NULL, (void *)iDfBenchmarkRun, (void *)pdbiDfBenchmarkInfoPtr)) != 0 ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to create a thread to run the benchmark.");
            iError = SPI_MiscError;
            goto bailFromvDfDeFazio;
        }
    }

    /* Start the countdown thread */
    if ( (iStatus = s_pthread_create(&ptThreads[uiClientsToRun], NULL, (void *)iDfBenchmarkPrintTimeToGo, (void *)pdbiDfBenchmarkInfoMaster->uiMinutesToRun)) != 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to create a thread to run the timer.");
        iError = SPI_MiscError;
        goto bailFromvDfDeFazio;
    }



    /* Sleep for the duration and wake up when we are done */
    s_sleep(pdbiDfBenchmarkInfoMaster->uiMinutesToRun * 60);



    /* Gather the benchmark threads and concatenate the results into a single structure */
    for ( uiI = 0, pdbiDfBenchmarkInfoPtr = pdbiDfBenchmarkInfo; uiI < uiClientsToRun; uiI++, pdbiDfBenchmarkInfoPtr++ ) {
        
        /* Join each threads */
        if ( (iStatus = s_pthread_join(ptThreads[uiI], &pvStatus)) != 0 ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to join to a thread.");
            iError = SPI_MiscError;
            goto bailFromvDfDeFazio;
        }
        
        /* Check the status pointer, this contains the error code reported by the function we threaded */
        if ( (unsigned int)pvStatus != SPI_NoError  ) {
            goto bailFromvDfDeFazio;
        }
        
    
        /* Concatenate the results into the master results structure */
        pdbiDfBenchmarkInfoMaster->pdriDfResultsInfo->dTotalSearchTime += pdbiDfBenchmarkInfoPtr->pdriDfResultsInfo->dTotalSearchTime;
        pdbiDfBenchmarkInfoMaster->pdriDfResultsInfo->dTotalRetrievalTime += pdbiDfBenchmarkInfoPtr->pdriDfResultsInfo->dTotalRetrievalTime;
        pdbiDfBenchmarkInfoMaster->pdriDfResultsInfo->uiSearchesAbovePar += pdbiDfBenchmarkInfoPtr->pdriDfResultsInfo->uiSearchesAbovePar;
        pdbiDfBenchmarkInfoMaster->pdriDfResultsInfo->uiRetrievalsAbovePar += pdbiDfBenchmarkInfoPtr->pdriDfResultsInfo->uiRetrievalsAbovePar;
        pdbiDfBenchmarkInfoMaster->pdriDfResultsInfo->uiTotalSearchesAttempted += pdbiDfBenchmarkInfoPtr->pdriDfResultsInfo->uiTotalSearchesAttempted;
        pdbiDfBenchmarkInfoMaster->pdriDfResultsInfo->uiTotalSearchesRun += pdbiDfBenchmarkInfoPtr->pdriDfResultsInfo->uiTotalSearchesRun;
        pdbiDfBenchmarkInfoMaster->pdriDfResultsInfo->uiTotalRetrievalsAttempted += pdbiDfBenchmarkInfoPtr->pdriDfResultsInfo->uiTotalRetrievalsAttempted;
        pdbiDfBenchmarkInfoMaster->pdriDfResultsInfo->uiTotalRetrievalsRun += pdbiDfBenchmarkInfoPtr->pdriDfResultsInfo->uiTotalRetrievalsRun;

        for ( uiJ = 0; uiJ < (DF_SEARCH_RESIDENCE_TIME_MAX + 1); uiJ++ ) {
            pdbiDfBenchmarkInfoMaster->pdriDfResultsInfo->puiSearchResidenceTime[uiJ] += pdbiDfBenchmarkInfoPtr->pdriDfResultsInfo->puiSearchResidenceTime[uiJ];
        }

        for ( uiJ = 0; uiJ < (DF_RETRIEVAL_RESIDENCE_TIME_MAX + 1); uiJ++ ) {
            pdbiDfBenchmarkInfoMaster->pdriDfResultsInfo->puiRetrievalResidenceTime[uiJ] += pdbiDfBenchmarkInfoPtr->pdriDfResultsInfo->puiRetrievalResidenceTime[uiJ];
        }
    }


    /* Gather the countdown thread */
    if ( (iStatus = s_pthread_join(ptThreads[uiClientsToRun], &pvStatus)) != 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to join the timer thread.");
        iError = SPI_MiscError;
        goto bailFromvDfDeFazio;
    }

    /* Check the status pointer, this contains the error code reported by the function we threaded */
    if ( (unsigned int)pvStatus != SPI_NoError  ) {
        goto bailFromvDfDeFazio;
    }



    /* Print the results */
    if ( (iError = iDfBenchmarkPrintResults(pdbiDfBenchmarkInfoMaster->pdriDfResultsInfo)) != SPI_NoError ) {
        goto bailFromvDfDeFazio;
    }




    /* Bail label */
    bailFromvDfDeFazio:


    /* Free the resources */
    {

        struct dfTermInfo       *pdtiDfTermInfoPtr = NULL;
        struct dfDocumentKey    *pddkDfDocumentKeyPtr = NULL;


        /* Free the result information */
        if ( pdbiDfBenchmarkInfo != NULL ) {
            for ( uiI = 0, pdbiDfBenchmarkInfoPtr = pdbiDfBenchmarkInfo; uiI < uiClientsToRun; uiI++, pdbiDfBenchmarkInfoPtr++ ) {
                if ( pdbiDfBenchmarkInfoPtr->pdriDfResultsInfo != NULL ) {
                    s_free(pdbiDfBenchmarkInfoPtr->pdriDfResultsInfo->puiSearchResidenceTime);
                    s_free(pdbiDfBenchmarkInfoPtr->pdriDfResultsInfo->puiRetrievalResidenceTime);
                    s_free(pdbiDfBenchmarkInfoPtr->pdriDfResultsInfo);
                }
            }
            s_free(pdbiDfBenchmarkInfo);
        }


        /* Free the master benchmark information */
        if ( pdbiDfBenchmarkInfoMaster != NULL ) {

            /* Free the master benchmark information */
            if ( pdbiDfBenchmarkInfoMaster->pdiiDfIndexInfo != NULL ) {

                for ( uiI = 0, pdtiDfTermInfoPtr = pdbiDfBenchmarkInfoMaster->pdiiDfIndexInfo->pdtiDfTermInfo; 
                        uiI < pdbiDfBenchmarkInfoMaster->pdiiDfIndexInfo->uiDfTermInfoLength; uiI++, pdtiDfTermInfoPtr++ ) {
                    s_free(pdtiDfTermInfoPtr->pucTerm);
                }
                s_free(pdbiDfBenchmarkInfoMaster->pdiiDfIndexInfo->pdtiDfTermInfo);

                for ( uiI = 0, pddkDfDocumentKeyPtr = pdbiDfBenchmarkInfoMaster->pdiiDfIndexInfo->pddkDfDocumentKey; 
                        uiI < pdbiDfBenchmarkInfoMaster->pdiiDfIndexInfo->uiDfDocumentIDLength; uiI++, pddkDfDocumentKeyPtr++ ) {
                    s_free(pddkDfDocumentKeyPtr->pucDocumentKey);
                }
                s_free(pdbiDfBenchmarkInfoMaster->pdiiDfIndexInfo->pddkDfDocumentKey);

                s_free(pdbiDfBenchmarkInfoMaster->pdiiDfIndexInfo);
            }


            /* Free the master results information */
            if ( pdbiDfBenchmarkInfoMaster->pdriDfResultsInfo != NULL ) {
                s_free(pdbiDfBenchmarkInfoMaster->pdriDfResultsInfo->puiSearchResidenceTime);
                s_free(pdbiDfBenchmarkInfoMaster->pdriDfResultsInfo->puiRetrievalResidenceTime);
                s_free(pdbiDfBenchmarkInfoMaster->pdriDfResultsInfo);
            }


            /* Free the master server information */
            if ( pdbiDfBenchmarkInfoMaster->pdsiDfServerInfo != NULL ) {
                s_free(pdbiDfBenchmarkInfoMaster->pdsiDfServerInfo->pucIndexDirectoryPath);
                s_free(pdbiDfBenchmarkInfoMaster->pdsiDfServerInfo->pucConfigurationDirectoryPath);
                s_free(pdbiDfBenchmarkInfoMaster->pdsiDfServerInfo->pucTemporaryDirectoryPath);
                s_free(pdbiDfBenchmarkInfoMaster->pdsiDfServerInfo->pucIndexName);
                s_free(pdbiDfBenchmarkInfoMaster->pdsiDfServerInfo->pucHostName);
                s_free(pdbiDfBenchmarkInfoMaster->pdsiDfServerInfo);
            }

            s_free(pdbiDfBenchmarkInfoMaster);
        }


        /* Free the threads array */
        s_free(ptThreads);
    }

    printf("\n");


    return;

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iDfBenchmarkSetup()

    Purpose:    Set up the deFazio benchmark:
                    - initialize the server
                    - open the index
                    - get term information from the index
                    - total up the occurrences to get the total number of occurrences
                    - sort out the terms by order of decreasing occurrence
                    - get two pointers:
                        - to the 90% level of total occurrences starting at the top of the terms
                        - to the 5% level of total occurrences starting at the bottom of the terms
                    - get a list of document keys for subsequent retrieval
                    - close the datase
                    - shutdown the server
                    - allocate and populate the benchmark information structure

    Parameters: pdsiDfServerInfo        server information
                ppdiiDfIndexInfo        index information structure return pointer

    Globals:    none

    Returns:    SPI error code

*/
static int iDfBenchmarkSetup
(
    struct dfServerInfo *pdsiDfServerInfo,
    struct dfIndexInfo **ppdiiDfIndexInfo
)
{

    int                     iError = 0;
    struct spiSession       *pssSpiSession = NULL;
    struct dfConnection     *pdcConnection = NULL;
    void                    *pvIndex = NULL;
    unsigned int            uiDocumentCount = 0;
    unsigned long           ulUniqueTermCount = 0;
    unsigned long           ulTotalTermCount = 0;
    unsigned long           ulUniqueStopTermCount = 0;
    unsigned long           ulTotalStopTermCount = 0;
    struct dfTermInfo       *pdtiDfTermInfo = NULL;
    struct dfTermInfo       *pdtiDfTermInfoPtr = NULL;
    unsigned int            uiDfTermInfoLength = 0;
    unsigned int            uiTotalTermOccurrenceCount = 0;
    unsigned int            uiTermOccurrenceCount90 = 0;
    unsigned int            uiTermOccurrenceCount5 = 0;
    unsigned int            uiTermOccurrenceCount = 0;
    unsigned int            uiTermInfoIndex90 = 0;
    unsigned int            uiTermInfoIndex5 = 0;
    struct dfDocumentKey    *pddkDfDocumentKey = NULL;
    struct dfDocumentKey    *pddkDfDocumentKeyPtr = NULL;
    unsigned int            uiDfDocumentIDLength = 0;
    unsigned int            uiI = 0;
    unsigned char           pucPrintableNumber[UTL_FILE_PATH_MAX + 1] = {'\0'};
    unsigned char           pucPrintableNumber2[UTL_FILE_PATH_MAX + 1] = {'\0'};

    unsigned char           *pucLanguageCode = NULL;

    struct dfIndexInfo      *pdiiDfIndexInfo = NULL;


    ASSERT(pdsiDfServerInfo != NULL);
    ASSERT(ppdiiDfIndexInfo != NULL);

    ASSERT((pdsiDfServerInfo->uiNetProtocolID == UTL_NET_PROTOCOL_TCP_ID) || (pdsiDfServerInfo->uiNetProtocolID == UTL_NET_PROTOCOL_UDP_ID));
    ASSERT(((bUtlStringsIsStringNULL(pdsiDfServerInfo->pucHostName) == false) && (pdsiDfServerInfo->iPort > 0)) || 
            ((bUtlStringsIsStringNULL(pdsiDfServerInfo->pucHostName) == true) && (pdsiDfServerInfo->iPort <= 0)));


    /* Initialize the server */
    if ( (iError = iDfInitializeServer(pdsiDfServerInfo->pucIndexDirectoryPath, pdsiDfServerInfo->pucConfigurationDirectoryPath, pdsiDfServerInfo->pucTemporaryDirectoryPath, 
            pdsiDfServerInfo->uiNetProtocolID, pdsiDfServerInfo->pucHostName, pdsiDfServerInfo->iPort, &pssSpiSession, &pdcConnection)) != SPI_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to initialize the server, spi error: %d.", iError);
        goto bailFromiInitializeBenchmark;
    }


    /* Open the index */
    if ( (iError = iDfOpenIndex(pssSpiSession, pdcConnection, pdsiDfServerInfo->pucIndexName, &pvIndex)) != SPI_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to open the index, spi error: %d.", iError);
        goto bailFromiInitializeBenchmark;
    }



    /* Get stats from the index */
    printf("Getting stats from the index:\n");
    s_fflush(stdout);
    if ( (iError = iDfGetIndexInfo(pssSpiSession, pdcConnection, pvIndex, &uiDocumentCount, &ulTotalTermCount, &ulUniqueTermCount, &ulTotalStopTermCount, &ulUniqueStopTermCount)) != SPI_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the index information, spi error: %d.", iError);
        goto bailFromiInitializeBenchmark;
    }
    printf(" Document count                    :%15s\n", pucUtlStringsFormatUnsignedNumber(uiDocumentCount, pucPrintableNumber, UTL_FILE_PATH_MAX + 1));
    printf(" Total term count                  :%15s\n", pucUtlStringsFormatUnsignedNumber(ulTotalTermCount, pucPrintableNumber, UTL_FILE_PATH_MAX + 1));
    printf(" Unique term count                 :%15s\n", pucUtlStringsFormatUnsignedNumber(ulUniqueTermCount, pucPrintableNumber, UTL_FILE_PATH_MAX + 1));
    printf(" Total stop term count             :%15s\n", pucUtlStringsFormatUnsignedNumber(ulTotalStopTermCount, pucPrintableNumber, UTL_FILE_PATH_MAX + 1));
    printf(" Unique stop term count            :%15s\n", pucUtlStringsFormatUnsignedNumber(ulUniqueStopTermCount, pucPrintableNumber, UTL_FILE_PATH_MAX + 1));
    printf("\n");



    /* Get the terms from the index, we want normals terms, no stop terms */
    printf("Getting all the terms from the index:\n");
    s_fflush(stdout);
    if ( (iError = iDfGetIndexTermInfo(pssSpiSession, pdcConnection, pvIndex, &pdtiDfTermInfo, &uiDfTermInfoLength)) != SPI_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the terms from the index, spi error: %d.", iError);
        goto bailFromiInitializeBenchmark;
    }


    /* Check that we got at least one term */
    if ( uiDfTermInfoLength <= 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to get a single term from the index.");
        iError = SPI_MiscError;
        goto bailFromiInitializeBenchmark;
    }


    /* Count up the terms to get a total term info for this index */
    for ( uiI = 0, uiTotalTermOccurrenceCount = 0, pdtiDfTermInfoPtr = pdtiDfTermInfo; uiI < uiDfTermInfoLength; uiI++, pdtiDfTermInfoPtr++) {
        uiTotalTermOccurrenceCount += pdtiDfTermInfoPtr->uiCount;
    }
    printf("                                    %15s  %15s\n", "Terms", "Unique Terms");
    printf(" Total                             :%15s  %15s\n", 
            pucUtlStringsFormatUnsignedNumber(uiTotalTermOccurrenceCount, pucPrintableNumber, UTL_FILE_PATH_MAX + 1), 
            pucUtlStringsFormatUnsignedNumber(uiDfTermInfoLength, pucPrintableNumber2, UTL_FILE_PATH_MAX + 1));



    /* Cross check with stated stats */
    ASSERT((ulTotalTermCount == uiTotalTermOccurrenceCount) || ((ulTotalTermCount < 0) && (uiTotalTermOccurrenceCount>= 0)));
    ASSERT((ulUniqueTermCount == uiDfTermInfoLength) || ((ulUniqueTermCount < 0) && (uiDfTermInfoLength>= 0)));



    /* Sort the term info array in order of decreasing occurrence */
    vDfSortTermInfoDesc(pdtiDfTermInfo, 0, uiDfTermInfoLength - 1);



    /* Get high use segment - 90% index */
    uiTermOccurrenceCount90 = (float)uiTotalTermOccurrenceCount * 0.9;
    for ( uiI = 0, uiTermOccurrenceCount = 0, pdtiDfTermInfoPtr = pdtiDfTermInfo; uiI < uiDfTermInfoLength; uiI++, pdtiDfTermInfoPtr++ ) {
        uiTermOccurrenceCount += pdtiDfTermInfoPtr->uiCount;
        if ( uiTermOccurrenceCount >= uiTermOccurrenceCount90 ) {
            uiTermInfoIndex90 = uiI;
            break;
        }
    }
    printf(" 90%% index                        :%15s  %15s\n",
            pucUtlStringsFormatUnsignedNumber(uiTermOccurrenceCount90, pucPrintableNumber, UTL_FILE_PATH_MAX + 1), 
            pucUtlStringsFormatUnsignedNumber(uiTermInfoIndex90, pucPrintableNumber2, UTL_FILE_PATH_MAX + 1));


    /* Get the low use segment - 5% index */
    uiTermOccurrenceCount5 = (float)uiTotalTermOccurrenceCount * 0.05;
    for ( uiI = uiDfTermInfoLength, uiTermOccurrenceCount = 0, pdtiDfTermInfoPtr = pdtiDfTermInfo + (uiDfTermInfoLength - 1); uiI > 0; uiI--, pdtiDfTermInfoPtr-- ) {
        uiTermOccurrenceCount += pdtiDfTermInfoPtr->uiCount;
        if ( uiTermOccurrenceCount >= uiTermOccurrenceCount5 ) {
            uiTermInfoIndex5 = (uiI + 1);
            break;
        }
    }
    printf(" 5%% index                         :%15s  %15s\n",
            pucUtlStringsFormatUnsignedNumber(uiTermOccurrenceCount5, pucPrintableNumber, UTL_FILE_PATH_MAX + 1), 
            pucUtlStringsFormatUnsignedNumber(uiTermInfoIndex5, pucPrintableNumber2, UTL_FILE_PATH_MAX + 1));



    /* Get the moderate use segment - 5% remaining between the low use and high use segments */

        /* No need to do that, they are between uiTermInfoIndex90 and uiTermInfoIndex5 */


    printf("\n");



    /* Here we need to run a search that will return lots of documents
    ** which we can use for retrieval, so we take the term which has
    ** the greatest number of occurrences and use that as our search text
    */
    printf("Getting document keys from the index:\n");
    s_fflush(stdout);
    if ( (iError = iDfGetIndexDocumentIDs(pssSpiSession, pdcConnection, pvIndex, pucLanguageCode, 
            pdtiDfTermInfo->pucTerm, &pddkDfDocumentKey, &uiDfDocumentIDLength)) != SPI_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to get document keys from the index, spi error: %d.", iError);
        goto bailFromiInitializeBenchmark;
    }

    /* Check that we got at least one document key */
    if ( uiDfDocumentIDLength <= 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to get a single document key from the index.");
        iError = SPI_MiscError;
        goto bailFromiInitializeBenchmark;
    }

    printf(" Got %s document keys.\n\n", pucUtlStringsFormatUnsignedNumber(uiDfDocumentIDLength, pucPrintableNumber, UTL_FILE_PATH_MAX + 1));



    /* Close the index */
    if ( (iError = iDfCloseIndex(pssSpiSession, pdcConnection, pvIndex)) != SPI_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to close the index, spi error: %d.", iError);
        goto bailFromiInitializeBenchmark;
    }


    /* Shutdown the server */
    if ( (iError = iDfShutdownServer(pssSpiSession, pdcConnection)) != SPI_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to shutdown the server, spi error: %d.", iError);
        goto bailFromiInitializeBenchmark;
    }



    /* Allocate the index information structure */
    if ( (pdiiDfIndexInfo = (struct dfIndexInfo *)s_malloc((size_t)(sizeof(struct dfIndexInfo)))) == NULL ) {
        iError = SPI_MemError;
        goto bailFromiInitializeBenchmark;
    }



    /* Bail label */
    bailFromiInitializeBenchmark:

    /* Handle the error */
    if ( iError == SPI_NoError ) {

        /* Set the benchmark information structure fields */
        pdiiDfIndexInfo->pdtiDfTermInfo = pdtiDfTermInfo;
        pdiiDfIndexInfo->uiDfTermInfoLength = uiDfTermInfoLength;
        pdiiDfIndexInfo->uiTermInfoIndex90 = uiTermInfoIndex90;
        pdiiDfIndexInfo->uiTermInfoIndex5 = uiTermInfoIndex5;
        pdiiDfIndexInfo->pddkDfDocumentKey = pddkDfDocumentKey;
        pdiiDfIndexInfo->uiDfDocumentIDLength = uiDfDocumentIDLength;

        /* Set the return pointers */
        *ppdiiDfIndexInfo = pdiiDfIndexInfo;
    }
    else {

        /* Free allocated resources */
        s_free(pdiiDfIndexInfo);
        
        /* Free the document key information */
        for ( uiI = 0,  pddkDfDocumentKeyPtr = pddkDfDocumentKey; uiI < uiDfDocumentIDLength; uiI++, pddkDfDocumentKeyPtr++ ) {
            s_free(pddkDfDocumentKeyPtr->pucDocumentKey);
        }
        s_free(pddkDfDocumentKey);
        
        /* Release the term info information */
        for ( uiI = 0,  pdtiDfTermInfoPtr = pdtiDfTermInfo; uiI < uiDfTermInfoLength; uiI++, pdtiDfTermInfoPtr++ ) {
            s_free(pdtiDfTermInfoPtr->pucTerm);
        }
        s_free(pdtiDfTermInfo);
        
        /* Close the index */
        if ( pvIndex != NULL ) {
            iDfCloseIndex(pssSpiSession, pdcConnection, pvIndex);
        }
        
        /* Shutdown the server */
        if ( pssSpiSession != NULL ) {
            iDfShutdownServer(pssSpiSession, pdcConnection);
        }
    }


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iDfBenchmarkRun()

    Purpose:    Runs the deFazio benchmark:
                    - initialize the server
                    - open the index
                    - set up a loop running for a set amount of time
                        - pick 50 terms and create a search from them
                            - run this search on the system
                            - retrieve 10 documents picked at random from the document key list
                    - close the datase
                    - shutdown the server
                    - allocate and populate the results information structure

    Parameters: pdbiDfBenchmarkInfo     benchmark information

    Globals:    none

    Returns:    SPI error code

*/
static int iDfBenchmarkRun
(
    struct dfBenchmarkInfo *pdbiDfBenchmarkInfo
)
{

    int                     iError = 0;
    time_t                  tEndTime = (time_t)0;
    time_t                  tTimeToGo = (time_t)0;
    struct spiSession       *pssSpiSession = NULL;
    struct dfConnection     *pdcConnection = NULL;
    void                    *pvIndex = NULL;
    struct dfTermInfo       *pdtiDfTermInfoPtr = NULL;
    struct dfDocumentKey    *pddkDfDocumentKeyPtr = NULL;
    unsigned char           *pucLanguageCode = NULL;
    unsigned char           *pucSearchText = NULL;
    unsigned int            uiTermsAdded = 0;
    unsigned int            uiTermPairsAdded = 0;
    unsigned int            uiSearchTermNum = 0;
    unsigned int            uiTerm = 0;
    unsigned int            uiProximityOperator = 0;
    unsigned int            uiBooleanOperator = 0;
    unsigned int            uiTermInfoIndex = 0;
    unsigned int            uiHitsCount = 0;
    unsigned int            ulDocumentIndex = 0;
    unsigned int            uiI = 0;


    struct timeval          tvStartTimeVal;
    struct timeval          tvEndTimeVal;
    struct timeval          tvSearchTimeVal;
    struct timeval          tvTotalSearchTimeVal;
    double                  dTotalSearchTime = 0;
    struct timeval          tvRetrievalTimeVal;
    struct timeval          tvTotalRetrievalTimeVal;
    double                  dTotalRetrievalTime = 0;

    unsigned int            uiSearchesAbovePar = 0;
    unsigned int            uiRetrievalsAbovePar = 0;

    unsigned int            uiTotalSearchesAttempted = 0;
    unsigned int            uiTotalSearchesRun = 0;
    unsigned int            uiTotalRetrievalsAttempted = 0;
    unsigned int            uiTotalRetrievalsRun = 0;

    double                  dSearchTime = 0;
    unsigned int            *puiSearchResidenceTime = NULL;
    double                  dRetrievalTime = 0;
    unsigned int            *puiRetrievalResidenceTime = NULL;

    struct dfServerInfo     *pdsiDfServerInfo = pdbiDfBenchmarkInfo->pdsiDfServerInfo;
    struct dfIndexInfo      *pdiiDfIndexInfo = pdbiDfBenchmarkInfo->pdiiDfIndexInfo;
    struct dfResultsInfo    *pdriDfResultsInfo = NULL;


    ASSERT(pdbiDfBenchmarkInfo != NULL);

    ASSERT(pdsiDfServerInfo != NULL);
    ASSERT((pdsiDfServerInfo->uiNetProtocolID == UTL_NET_PROTOCOL_TCP_ID) || (pdsiDfServerInfo->uiNetProtocolID == UTL_NET_PROTOCOL_UDP_ID));
    ASSERT(((bUtlStringsIsStringNULL(pdsiDfServerInfo->pucHostName) == false) && (pdsiDfServerInfo->iPort > 0)) || 
            ((bUtlStringsIsStringNULL(pdsiDfServerInfo->pucHostName) == true) && (pdsiDfServerInfo->iPort <= 0)));
    ASSERT(pdiiDfIndexInfo != NULL);
    ASSERT(pdiiDfIndexInfo->pdtiDfTermInfo != NULL);
    ASSERT(pdiiDfIndexInfo->uiDfTermInfoLength > 0);
    ASSERT(pdiiDfIndexInfo->uiTermInfoIndex90 > 0);
    ASSERT(pdiiDfIndexInfo->uiTermInfoIndex5 > 0);
    ASSERT(pdiiDfIndexInfo->pddkDfDocumentKey != NULL);
    ASSERT(pdiiDfIndexInfo->uiDfDocumentIDLength > 0);


    /* Initialize any static structure */
    s_memset(&tvTotalSearchTimeVal, 0, sizeof(struct timeval));
    s_memset(&tvTotalRetrievalTimeVal, 0, sizeof(struct timeval));


    /* Allocate the search residence time array */
    if ( (puiSearchResidenceTime = (unsigned int *)s_malloc((size_t)(sizeof(unsigned int *) * (DF_SEARCH_RESIDENCE_TIME_MAX + 1)))) == NULL ) {
        iError = SPI_MemError;
        goto bailFromiDfRunBenchmark;
    }

    /* Allocate the retrieval residence time array */
    if ( (puiRetrievalResidenceTime = (unsigned int *)s_malloc((size_t)(sizeof(unsigned int *) * (DF_RETRIEVAL_RESIDENCE_TIME_MAX + 1)))) == NULL ) {
        iError = SPI_MemError;
        goto bailFromiDfRunBenchmark;
    }



    /* Initialize the server */
    if ( (iError = iDfInitializeServer(pdsiDfServerInfo->pucIndexDirectoryPath, pdsiDfServerInfo->pucConfigurationDirectoryPath, pdsiDfServerInfo->pucTemporaryDirectoryPath, 
            pdsiDfServerInfo->uiNetProtocolID, pdsiDfServerInfo->pucHostName, pdsiDfServerInfo->iPort, &pssSpiSession, &pdcConnection)) != SPI_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to initialize the server, spi error: %d.", iError);
        goto bailFromiDfRunBenchmark;
    }


    /* Open the index */
    if ( (iError = iDfOpenIndex(pssSpiSession, pdcConnection, pdsiDfServerInfo->pucIndexName, &pvIndex)) != SPI_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to open the index, spi error: %d.", iError);
        goto bailFromiDfRunBenchmark;
    }


    /* Work out until when we should run */
    tEndTime = s_time(NULL) + (time_t)(pdbiDfBenchmarkInfo->uiMinutesToRun * 60);


    /* Start the loop */
    while ( (tTimeToGo = (tEndTime - s_time(NULL))) >= (time_t)0 ) {

        /* Allocate space for the search text, assume 100 bytes per term */
        if ( (pucSearchText = (unsigned char *)s_malloc((size_t)(sizeof(unsigned char) * DF_SEARCH_LENGTH_MAX))) == NULL ) {
            iError = SPI_MemError;
            goto bailFromiDfRunBenchmark;
        }

        /* Get the number of search terms we are going to use for this search */
        iUtlRandGetRand(DF_SEARCH_TERM_COUNT_MAX - 1, &uiSearchTermNum);
        uiSearchTermNum += 1;

        /* Initialize the number of terms added thus far */
        uiTermsAdded = 0;
        uiTermPairsAdded = 0;

        /* We build the search by picking terms from the term info array */
        do {


            /* Open a bracket if this is the first term pair */
            if ( uiTermPairsAdded == 0 ) {
                s_strnncat(pucSearchText, "( ", DF_SEARCH_LENGTH_MAX, DF_SEARCH_LENGTH_MAX + 1);
            }


            /* Get the term type we add as a term pair, if this is the
            ** last iteration we forcibly apply a single term only
            */
            if ( uiTermsAdded == uiSearchTermNum ) {
                uiTerm = 0;
            }
            else {
                iUtlRandGetRand(1, &uiTerm);
            }


            /* Handle the term selected */
            switch ( uiTerm ) {

                /* Single term */
                case 0:
                    /* Get the term info index */
                    uiTermInfoIndex = iDfGetRandomTermInfoIndex(pdiiDfIndexInfo->uiDfTermInfoLength, pdiiDfIndexInfo->uiTermInfoIndex90, pdiiDfIndexInfo->uiTermInfoIndex5);
                    pdtiDfTermInfoPtr = pdiiDfIndexInfo->pdtiDfTermInfo + uiTermInfoIndex;

                    /* Append the term to the search text */
                    s_strnncat(pucSearchText, pdtiDfTermInfoPtr->pucTerm, DF_SEARCH_LENGTH_MAX, DF_SEARCH_LENGTH_MAX + 1);
                    s_strnncat(pucSearchText, " ", DF_SEARCH_LENGTH_MAX, DF_SEARCH_LENGTH_MAX + 1);

                    /* Increment the number of terms added thus far */
                    uiTermsAdded++;

                    break;

                /* Proximity term */
                case 1:

                    /* Get the term info index */
                    uiTermInfoIndex = iDfGetRandomTermInfoIndex(pdiiDfIndexInfo->uiDfTermInfoLength, pdiiDfIndexInfo->uiTermInfoIndex90, pdiiDfIndexInfo->uiTermInfoIndex5);
                    pdtiDfTermInfoPtr = pdiiDfIndexInfo->pdtiDfTermInfo + uiTermInfoIndex;

                    /* Append the term to the search text */
                    s_strnncat(pucSearchText, "( ", DF_SEARCH_LENGTH_MAX, DF_SEARCH_LENGTH_MAX + 1);
                    s_strnncat(pucSearchText, pdtiDfTermInfoPtr->pucTerm, DF_SEARCH_LENGTH_MAX, DF_SEARCH_LENGTH_MAX + 1);
                    s_strnncat(pucSearchText, " ", DF_SEARCH_LENGTH_MAX, DF_SEARCH_LENGTH_MAX + 1);

                    /* Increment the number of terms added thus far */
                    uiTermsAdded++;

                    /* Get the proximity operator to apply */
                    iUtlRandGetRand(2, &uiProximityOperator);

                    /* Handle the proximity operator */
                    switch ( uiProximityOperator ) {

                        /* Phase operator */
                        case 0:
                            s_strnncat(pucSearchText, "ADJ ", DF_SEARCH_LENGTH_MAX, DF_SEARCH_LENGTH_MAX + 1);
                            break;


                        /* Within sentence operator */
                        case 1:
                            s_strnncat(pucSearchText, "NEAR[10] ", DF_SEARCH_LENGTH_MAX, DF_SEARCH_LENGTH_MAX + 1);
                            break;


                        /* Within paragraph operator */
                        case 2:
                            s_strnncat(pucSearchText, "NEAR[50] ", DF_SEARCH_LENGTH_MAX, DF_SEARCH_LENGTH_MAX + 1);
                            break;
                    }


                    /* Get the term info index */
                    uiTermInfoIndex = iDfGetRandomTermInfoIndex(pdiiDfIndexInfo->uiDfTermInfoLength, pdiiDfIndexInfo->uiTermInfoIndex90, pdiiDfIndexInfo->uiTermInfoIndex5);
                    pdtiDfTermInfoPtr = pdiiDfIndexInfo->pdtiDfTermInfo + uiTermInfoIndex;

                    /* Append the term to the search text */
                    s_strnncat(pucSearchText, pdtiDfTermInfoPtr->pucTerm, DF_SEARCH_LENGTH_MAX, DF_SEARCH_LENGTH_MAX + 1);
                    s_strnncat(pucSearchText, " ) ", DF_SEARCH_LENGTH_MAX, DF_SEARCH_LENGTH_MAX + 1);

                    /* Increment the number of terms added thus far */
                    uiTermsAdded++;

                    break;
            }



            /* Increment the term pair counter */
            uiTermPairsAdded++;


            /* Close the bracket if this is the second term pair and reset the term pair counter */
            if ( uiTermPairsAdded == 2 ) {
                s_strnncat(pucSearchText, ") ", DF_SEARCH_LENGTH_MAX, DF_SEARCH_LENGTH_MAX + 1);
                uiTermPairsAdded = 0;
            }



            /* Add a boolean operator if this is not the last term to be added */
            if ( uiTermsAdded < uiSearchTermNum ) {

                /* Get the boolean operator to apply */
                iUtlRandGetRand(2, &uiBooleanOperator);

                switch ( uiBooleanOperator ) {

                    /* OR */
                    case 0:
                        s_strnncat(pucSearchText, "OR ", DF_SEARCH_LENGTH_MAX, DF_SEARCH_LENGTH_MAX + 1);
                        break;


                    /* AND */
                    case 1:
                        s_strnncat(pucSearchText, "AND ", DF_SEARCH_LENGTH_MAX, DF_SEARCH_LENGTH_MAX + 1);
                        break;


                    /* NOT */
                    case 2:
                        s_strnncat(pucSearchText, "NOT ", DF_SEARCH_LENGTH_MAX, DF_SEARCH_LENGTH_MAX + 1);
                        break;
                }
            }


        } while ( uiTermsAdded < uiSearchTermNum );


        /* Make sure we close off the bracket for this term pair */
        if ( uiTermPairsAdded != 0 ) {
            s_strnncat(pucSearchText, " ) ", DF_SEARCH_LENGTH_MAX, DF_SEARCH_LENGTH_MAX + 1);
        }


        /* Increment the number of searches attempted */
        uiTotalSearchesAttempted++;

        /* Start the timer */
        s_gettimeofday(&tvStartTimeVal, NULL);

        /* Run the search */
/*         printf("Search: '%s'\n", pucSearchText); */

        if ( (iError = iDfSearchIndex(pssSpiSession, pdcConnection, pvIndex, pucLanguageCode, pucSearchText, &uiHitsCount)) == 0 ) {

            /* Stop the timer */
            s_gettimeofday(&tvEndTimeVal, NULL);

            /* Get the time taken for this search */
            UTL_DATE_DIFF_TIMEVAL(tvStartTimeVal, tvEndTimeVal, tvSearchTimeVal);

            /* Increment the below par counter if this search was on or below par */
            if ( tvSearchTimeVal.tv_sec <= DF_SEARCH_PAR_TIME ) {
                uiSearchesAbovePar++;
            }

            /* Increment the total time taken for searches */
            UTL_DATE_ADD_TIMEVAL(tvTotalSearchTimeVal, tvSearchTimeVal, tvTotalSearchTimeVal);

            /* Work out the search time and add it to the search time array */
            UTL_DATE_TIMEVAL_TO_SECONDS(tvSearchTimeVal, dSearchTime);
            puiSearchResidenceTime[UTL_MACROS_MIN((unsigned int)dSearchTime, DF_SEARCH_RESIDENCE_TIME_MAX)]++;

            /* Increment the number of searches run */
            uiTotalSearchesRun++;

        }


        /* Release the search text */
        s_free(pucSearchText);



        /* Retrieve DF_RETRIEVAL_COUNT documents */
        for ( uiI = 0; uiI < DF_RETRIEVAL_COUNT; uiI++ ) {

            /* First we pick the document */
            iUtlRandGetRand(pdiiDfIndexInfo->uiDfDocumentIDLength - 1, &ulDocumentIndex);

            /* Reference the document key entry from the array */
            pddkDfDocumentKeyPtr = pdiiDfIndexInfo->pddkDfDocumentKey + ulDocumentIndex;


            /* Increment the number of retrievals attempted */
            uiTotalRetrievalsAttempted++;

            /* Start the timer */
            s_gettimeofday(&tvStartTimeVal, NULL);

            /* Retrieve the document */
            if ( (iError = iDfRetrieveDocument(pssSpiSession, pdcConnection, pvIndex, pddkDfDocumentKeyPtr->pucDocumentKey)) == 0 ) {

                /* Stop the timer */
                s_gettimeofday(&tvEndTimeVal, NULL);

                /* Get the time taken for retrievals */
                UTL_DATE_DIFF_TIMEVAL(tvStartTimeVal, tvEndTimeVal, tvRetrievalTimeVal);

                /* Increment the below par counter if this retrieval cycle was on or below par */
                if ( tvRetrievalTimeVal.tv_sec <= DF_RETRIEVAL_PAR_TIME ) {
                    uiRetrievalsAbovePar++;
                }

                /* Increment the total time taken for retrievals */
                UTL_DATE_ADD_TIMEVAL(tvTotalRetrievalTimeVal, tvRetrievalTimeVal, tvTotalRetrievalTimeVal);

                /* Work out the retrieval time and add it to the retrieval time array */
                UTL_DATE_TIMEVAL_TO_SECONDS(tvRetrievalTimeVal, dRetrievalTime);
                puiRetrievalResidenceTime[UTL_MACROS_MIN((unsigned int)dRetrievalTime, DF_RETRIEVAL_RESIDENCE_TIME_MAX)]++;

                /* Increment the number of retrievals run */
                uiTotalRetrievalsRun++;
            }
        }
    }



    /* Convert the total search time to a double */
    UTL_DATE_TIMEVAL_TO_SECONDS(tvTotalSearchTimeVal, dTotalSearchTime);
    UTL_DATE_TIMEVAL_TO_SECONDS(tvTotalRetrievalTimeVal, dTotalRetrievalTime);



    /* Close the index */
    if ( (iError = iDfCloseIndex(pssSpiSession, pdcConnection, pvIndex)) != SPI_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to close the index, spi error: %d.", iError);
        goto bailFromiDfRunBenchmark;
    }


    /* Shutdown the server */
    if ( (iError = iDfShutdownServer(pssSpiSession, pdcConnection)) != SPI_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to shutdown the server, spi error: %d.", iError);
        goto bailFromiDfRunBenchmark;
    }


    /* Allocate the results information structure */
    if ( (pdriDfResultsInfo = (struct dfResultsInfo *)s_malloc((size_t)(sizeof(struct dfResultsInfo)))) == NULL ) {
        iError = SPI_MemError;
        goto bailFromiDfRunBenchmark;
    }



    /* Bail label */
    bailFromiDfRunBenchmark:

    /* Handle the error */
    if ( iError == SPI_NoError ) {

        /* Set the benchmark information structure fields */
        pdriDfResultsInfo->dTotalSearchTime = dTotalSearchTime;
        pdriDfResultsInfo->dTotalRetrievalTime = dTotalRetrievalTime;
        pdriDfResultsInfo->uiSearchesAbovePar = uiSearchesAbovePar;
        pdriDfResultsInfo->uiRetrievalsAbovePar = uiRetrievalsAbovePar;
        pdriDfResultsInfo->uiTotalSearchesAttempted = uiTotalSearchesAttempted;
        pdriDfResultsInfo->uiTotalSearchesRun = uiTotalSearchesRun;
        pdriDfResultsInfo->uiTotalRetrievalsAttempted = uiTotalRetrievalsAttempted;
        pdriDfResultsInfo->uiTotalRetrievalsRun = uiTotalRetrievalsRun;
        pdriDfResultsInfo->puiSearchResidenceTime = puiSearchResidenceTime;
        pdriDfResultsInfo->puiRetrievalResidenceTime = puiRetrievalResidenceTime;

        /* Set the return pointers */
        pdbiDfBenchmarkInfo->pdriDfResultsInfo = pdriDfResultsInfo;
    }
    else {

        /* Free allocated resources */
        s_free(puiSearchResidenceTime);
        s_free(puiRetrievalResidenceTime);
        s_free(pdriDfResultsInfo);

        /* Close the index */
        if ( pvIndex != NULL ) {
            iDfCloseIndex(pssSpiSession, pdcConnection, pvIndex);
        }

        /* Shutdown the server */
        if ( pssSpiSession != NULL ) {
            iDfShutdownServer(pssSpiSession, pdcConnection);
        }
    }


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iDfBenchmarkPrintTimeToGo()

    Purpose:    Prints the time to go for a run, this needs to be run as a 
                thread

    Parameters: uiMinutesToRun      number of minutes to run    

    Globals:    none

    Returns:    SPI error code

*/
static int iDfBenchmarkPrintTimeToGo
(
    unsigned int uiMinutesToRun
)
{

    time_t      tEndTime = (time_t)0;
    time_t      tTimeToGo = (time_t)0;


    ASSERT(uiMinutesToRun >= 0);


    /* Work out until when we should run */
    tEndTime = s_time(NULL) + (time_t)(uiMinutesToRun * 60);

    /* Start the loop */
    while ( (tTimeToGo = (tEndTime - s_time(NULL))) >= (time_t)0 ) {

        /* Display the count-down */
        printf("\rSeconds to go: %10u", (unsigned int)tTimeToGo);
        s_fflush(stdout);

        /* Sleep */
        s_sleep(DF_PRINTING_COUNTDOWN_SLEEP);
    }

    /* Erase the count-down */
    printf("\r                             ");


    return (SPI_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iDfBenchmarkPrintResults()

    Purpose:    Print the deFazio benchmark results:
                    - print up stats

    Parameters: pdriDfResultsInfo       results information structure

    Globals:    none

    Returns:    SPI error code

*/
static int iDfBenchmarkPrintResults
(
    struct dfResultsInfo *pdriDfResultsInfo
)
{

    unsigned int    uiI = 0;


    ASSERT(pdriDfResultsInfo != NULL);

    ASSERT(pdriDfResultsInfo->puiSearchResidenceTime != NULL);
    ASSERT(pdriDfResultsInfo->puiRetrievalResidenceTime != NULL);


    /* Print the stats */
    printf("\n");
    printf("Timings (secs)    Overall      Transactions      Per Transaction      %% Above Par\n");
    printf("Searches  %15.6f        %10u      %15.6f      %10u%%\n", 
            pdriDfResultsInfo->dTotalSearchTime, pdriDfResultsInfo->uiTotalSearchesRun, 
            (pdriDfResultsInfo->uiTotalSearchesRun > 0) ? (pdriDfResultsInfo->dTotalSearchTime / pdriDfResultsInfo->uiTotalSearchesRun) : 0.0, 
            (pdriDfResultsInfo->uiTotalSearchesRun > 0) ? (unsigned int)(((double)pdriDfResultsInfo->uiSearchesAbovePar / pdriDfResultsInfo->uiTotalSearchesRun) * 100) : 0 );
    printf("Retrievals%15.6f        %10u      %15.6f      %10u%%\n", 
            pdriDfResultsInfo->dTotalRetrievalTime, pdriDfResultsInfo->uiTotalRetrievalsRun, 
            (pdriDfResultsInfo->uiTotalRetrievalsRun > 0) ? (pdriDfResultsInfo->dTotalRetrievalTime / pdriDfResultsInfo->uiTotalRetrievalsRun) : 0.0, 
            (pdriDfResultsInfo->uiTotalRetrievalsRun > 0) ? (unsigned int)(((double)pdriDfResultsInfo->uiRetrievalsAbovePar / pdriDfResultsInfo->uiTotalRetrievalsRun) * 100) : 0 );
    printf("\n");
    printf("Search transactions completed per minute   : %15.6f\n", 
            (pdriDfResultsInfo->uiTotalSearchesRun > 0) ? ((double)60 / (pdriDfResultsInfo->dTotalSearchTime / pdriDfResultsInfo->uiTotalSearchesRun)) : 0.0 ); 
    printf("Retrieval transactions completed per minute: %15.6f\n", 
            (pdriDfResultsInfo->uiTotalRetrievalsRun > 0) ? ((double)60 / (pdriDfResultsInfo->dTotalRetrievalTime / pdriDfResultsInfo->uiTotalRetrievalsRun)) : 0.0 ); 


    if ( pdriDfResultsInfo->uiTotalSearchesRun < pdriDfResultsInfo->uiTotalSearchesAttempted  ) {
        printf("Failed to run: %u searches out of: %u submitted.\n", pdriDfResultsInfo->uiTotalSearchesAttempted - pdriDfResultsInfo->uiTotalSearchesRun, 
                pdriDfResultsInfo->uiTotalSearchesAttempted); 
    }
    if ( pdriDfResultsInfo->uiTotalRetrievalsRun < pdriDfResultsInfo->uiTotalRetrievalsAttempted  ) {
        printf("Failed to retrieve: %u document out of: %u requested.\n", pdriDfResultsInfo->uiTotalRetrievalsAttempted - pdriDfResultsInfo->uiTotalRetrievalsRun, 
                pdriDfResultsInfo->uiTotalRetrievalsAttempted); 
    }
    printf("\n");


    /* Print the residence times */
    printf("Search residence times (graph 2):\n");
    printf("%9s%u         %10u\n", "<", 1, pdriDfResultsInfo->puiSearchResidenceTime[0]);
    for ( uiI = 1 ; uiI < DF_SEARCH_RESIDENCE_TIME_MAX; uiI++ ) {
        printf("%10u         %10u\n", uiI, pdriDfResultsInfo->puiSearchResidenceTime[uiI]);
    }
    printf("%8s%u         %10u\n", ">", DF_SEARCH_RESIDENCE_TIME_MAX - 1, pdriDfResultsInfo->puiSearchResidenceTime[DF_SEARCH_RESIDENCE_TIME_MAX]);
    printf("\n");

    printf("Retrieval residence times (graph 2):\n");
    printf("%9s%u         %10u\n", "<", 1, pdriDfResultsInfo->puiRetrievalResidenceTime[0]);
    for ( uiI = 1 ; uiI < DF_RETRIEVAL_RESIDENCE_TIME_MAX; uiI++ ) {
        printf("%10u         %10u\n", uiI, pdriDfResultsInfo->puiRetrievalResidenceTime[uiI]);
    }
    printf("%9s%u         %10u\n", ">", DF_RETRIEVAL_RESIDENCE_TIME_MAX - 1, pdriDfResultsInfo->puiRetrievalResidenceTime[DF_RETRIEVAL_RESIDENCE_TIME_MAX]);
    printf("\n");


    return (SPI_NoError);

}


/*---------------------------------------------------------------------------*/


/* 
** ================================
** ===  Index Access Functions  ===
** ================================
*/


/*

    Function:   iDfInitializeServer()

    Purpose:    This is called when the server needs to be initialized, 
                and sets the contents of the spi session structure.

    Parameters: pucIndexDirectoryPath           index directory path (optional)
                pucConfigurationDirectoryPath   configuration directory path (optional)
                pucTemporaryDirectoryPath       temporary directory path (optional)
                uiNetProtocolID                 network protocol (optional)
                pucHostName                     host name (optional)
                iPort                           port (optional)
                ppssSpiSession                  return pointer for the spi session structure
                ppdcConnection                  return pointer for the connection structure

    Globals:    none

    Returns:    SPI error code

*/
static int iDfInitializeServer
(
    unsigned char *pucIndexDirectoryPath,
    unsigned char *pucConfigurationDirectoryPath,
    unsigned char *pucTemporaryDirectoryPath,
    unsigned int uiNetProtocolID,
    unsigned char *pucHostName,
    int iPort,
    struct spiSession **ppssSpiSession,
    struct dfConnection **ppdcConnection
)
{

    int                     iError = SPI_NoError;
    struct spiSession       *pssSpiSession = NULL;
    struct dfConnection     *pdcConnection = NULL;


    ASSERT((bUtlStringsIsStringNULL(pucIndexDirectoryPath) == false) || (bUtlStringsIsStringNULL(pucIndexDirectoryPath) == true));
    ASSERT((bUtlStringsIsStringNULL(pucConfigurationDirectoryPath) == false) || (bUtlStringsIsStringNULL(pucConfigurationDirectoryPath) == true));
    ASSERT((bUtlStringsIsStringNULL(pucTemporaryDirectoryPath) == false) || (bUtlStringsIsStringNULL(pucTemporaryDirectoryPath) == true));
    ASSERT((uiNetProtocolID == UTL_NET_PROTOCOL_TCP_ID) || (uiNetProtocolID == UTL_NET_PROTOCOL_UDP_ID));
    ASSERT(((bUtlStringsIsStringNULL(pucHostName) == false) && (iPort > 0)) || ((bUtlStringsIsStringNULL(pucHostName) == true) && (iPort <= 0)));
    ASSERT(ppssSpiSession != NULL);
    ASSERT(ppdcConnection != NULL);


    /* Allocate a new spi session structure */
    if ( (pssSpiSession = (struct spiSession *)s_malloc((size_t)(sizeof(struct spiSession)))) == NULL ) {
        iError = SPI_MemError;
        goto bailFromiInitializeServer;
    }


    /* Populate it */
    if ( bUtlStringsIsStringNULL(pucIndexDirectoryPath) == false ) {
        if ( (pssSpiSession->pucIndexDirectoryPath = s_strdup(pucIndexDirectoryPath)) == NULL ) {
            iError = SPI_MemError;
            goto bailFromiInitializeServer;
        }
    }

    if ( bUtlStringsIsStringNULL(pucConfigurationDirectoryPath) == false ) {
        if ( (pssSpiSession->pucConfigurationDirectoryPath = s_strdup(pucConfigurationDirectoryPath)) == NULL ) {
            iError = SPI_MemError;
            goto bailFromiInitializeServer;
        }
    }

    if ( bUtlStringsIsStringNULL(pucTemporaryDirectoryPath) == false ) {
        if ( (pssSpiSession->pucTemporaryDirectoryPath = s_strdup(pucTemporaryDirectoryPath)) == NULL ) {
            iError = SPI_MemError;
            goto bailFromiInitializeServer;
        }
    }

    pssSpiSession->pvClientPtr = NULL;



    /* Local */
    if ( bUtlStringsIsStringNULL(pucHostName) == true ) {
    
        /* Initialize the server */
        if ( (iError = iSpiInitializeServer(pssSpiSession)) != SPI_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to initialize the server, spi error: %d.", iError);
            goto bailFromiInitializeServer;
        }
    }

    /* Remote */
    else {

        void            *pvUtlNet = NULL;
        void            *pvLwps = NULL;
        unsigned char   *pucUserName = NULL;
        unsigned char   *pucPassword = NULL;
        int             iErrorCode = SPI_NoError;
        unsigned char   *pucErrorString = NULL;


        /* Open a connection to the remote server */
        if ( (iError = iUtlNetCreateClient(uiNetProtocolID, pucHostName, iPort, DF_TIMEOUT_DEFAULT, &pvUtlNet)) != UTL_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to open a net client connection, network protocol: %u, host: '%s', port: %d, utl error: %d", 
                    uiNetProtocolID, pucHostName, iPort, iError);
            iError = SPI_InitializeServerFailed;
            goto bailFromiInitializeServer;
        }

        /* Create a LPWS handle from the connection */
        if ( (iError = iLwpsCreate(pvUtlNet, &pvLwps)) != LWPS_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to create an lwps, lwps error: %d.", iError);
            iError = SPI_InitializeServerFailed;
            goto bailFromiInitializeServer;
        }

        /* Send the init */
        if ( (iError = iLwpsInitRequestHandle(pvLwps, pucUserName, pucPassword, &iErrorCode, &pucErrorString)) != LWPS_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to handle an lwps init request, lwps error: %d.", iError);
            iError = SPI_InitializeServerFailed;
            goto bailFromiInitializeServer;
        }
        else {

            /* Check for errors */
            if ( iErrorCode != SPI_NoError ) {
                iUtlLogError(UTL_LOG_CONTEXT, "Failed to exchanging inits, error code: %d, error text: '%s'.", 
                        iErrorCode, pucUtlStringsGetPrintableString(pucErrorString));
                s_free(pucErrorString);
                iError = iErrorCode;
                goto bailFromiInitializeServer;
            }
        }

        /* Create a conection structure */
        if ( (pdcConnection = (struct dfConnection *)s_malloc(sizeof(struct dfConnection))) == NULL ) {
            iError = SPI_MemError;
            goto bailFromiInitializeServer;
        }

        /* Set the server structure variables */
        pdcConnection->pvUtlNet = pvUtlNet;
        pdcConnection->pvLwps = pvLwps;
    }



    /* Bail label */
    bailFromiInitializeServer:

    /* Handle the error */
    if ( iError == SPI_NoError ) {

        /* Set the return pointers */
        *ppssSpiSession = pssSpiSession;
        *ppdcConnection = pdcConnection;
    }
    else {

        /* Shutdown the search engine */
        iSpiShutdownServer(pssSpiSession);

        /* Free the spi session structure */
        iSpiFreeSession(pssSpiSession);
        pssSpiSession = NULL;

        /* Free the connection structure */
        if ( pdcConnection != NULL ) {

            /* Free the net structure */
            iUtlNetFree(pdcConnection->pvUtlNet);
            pdcConnection->pvUtlNet = NULL;
    
            /* Free the lwps structure */
            iLwpsFree(pdcConnection->pvLwps);
            pdcConnection->pvLwps = NULL;
    
            /* Free the connection structure */
            s_free(pdcConnection);
        }
    }


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iDfShutdownServer()

    Purpose:    This is called when the server needs to be shutdown.

    Parameters: pssSpiSession       pointer to the spi session structure
                pdcConnection       pointer to the connection structure

    Globals:    none

    Returns:    SPI error code

*/
static int iDfShutdownServer
(
    struct spiSession *pssSpiSession,
    struct dfConnection *pdcConnection
)
{

    int     iError = SPI_NoError;


    ASSERT(pssSpiSession != NULL);
    ASSERT((pdcConnection != NULL) || (pdcConnection == NULL));


    /* Local */
    if ( pdcConnection == NULL ) {

        /* Close the search engine */
        iSpiShutdownServer(pssSpiSession);

        /* Free the spi session structure */
        iSpiFreeSession(pssSpiSession);
        pssSpiSession = NULL;
    }

    /* Remote */
    else if ( pdcConnection != NULL ) {

        /* Shutdown the search engine */
        iSpiShutdownServer(pssSpiSession);

        /* Free the spi session structure */
        iSpiFreeSession(pssSpiSession);
        pssSpiSession = NULL;

        /* Free the net structure */
        iUtlNetFree(pdcConnection->pvUtlNet);
        pdcConnection->pvUtlNet = NULL;

        /* Free the lwps structure */
        iLwpsFree(pdcConnection->pvLwps);
        pdcConnection->pvLwps = NULL;

        /* Free the connection structure */
        s_free(pdcConnection);
    }


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iDfOpenIndex()

    Purpose:    This function opens a index and returns a handle to that
                index. The content of this handle will vary from server to
                server.

    Parameters: pssSpiSession       pointer to the spi session structure
                pdcConnection       pointer to the connection structure
                pucIndexName        index name
                ppvIndex            return pointer for a index structure

    Globals:    none

    Returns:    SPI error code

*/
static int iDfOpenIndex
(
    struct spiSession *pssSpiSession,
    struct dfConnection *pdcConnection,
    unsigned char *pucIndexName,
    void **ppvIndex
)
{

    int         iError = SPI_NoError;
    void        *pvIndex = NULL;


    ASSERT(pssSpiSession != NULL);
    ASSERT((pdcConnection != NULL) || (pdcConnection == NULL));
    ASSERT(bUtlStringsIsStringNULL(pucIndexName) == false);
    ASSERT(ppvIndex != NULL);


    /* Local */
    if ( pdcConnection == NULL ) {

        /* Open the index */
        if ( (iError = iSpiOpenIndex(pssSpiSession, pucIndexName, &pvIndex)) != SPI_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to open the index, spi error: %d.", iError);
            goto bailFromiDfOpenIndex;
        }

    }

    /* Remote */
    else if ( pdcConnection != NULL ) {

        /* Duplicate the index name */
        if ( (pvIndex = (void *)s_strdup(pucIndexName)) == NULL ) {
            iError = SPI_MemError;
            goto bailFromiDfOpenIndex;
        }
    }



    /* Bail label */
    bailFromiDfOpenIndex:

    /* Handle the error */
    if ( iError == SPI_NoError ) {

        /* Set the return pointer */
        *ppvIndex = pvIndex;
    }
    else {

        /* Free allocated resources */
        iDfCloseIndex(pssSpiSession, pdcConnection, pvIndex);
    }


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iDfCloseIndex()

    Purpose:    This function closes the index.

    Parameters: pssSpiSession       pointer to the spi session structure
                pdcConnection       pointer to the connection structure
                pvIndex             pointer to the index structure

    Globals:    none

    Returns:    SPI error code

*/
static int iDfCloseIndex
(
    struct spiSession *pssSpiSession,
    struct dfConnection *pdcConnection,
    void *pvIndex
)
{

    ASSERT(pssSpiSession != NULL);
    ASSERT((pdcConnection != NULL) || (pdcConnection == NULL));
    ASSERT(pvIndex != NULL);


    /* Local */
    if ( pdcConnection == NULL ) {

        /* Close the index */
        iSpiCloseIndex(pssSpiSession, pvIndex);
        pvIndex = NULL;
    }

    /* Remote */
    else if ( pdcConnection != NULL ) {

        unsigned char    *pucIndexName = (unsigned char *)pvIndex;

        /* Free the index name */
        s_free(pucIndexName);
    }


    return (SPI_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iDfGetIndexInfo()

    Purpose:    This is gets the term counts from the index

    Parameters: pssSpiSession               pointer to the spi session structure
                pdcConnection               pointer to the connection structure
                pvIndex                     pointer to the index structure
                puiDocumentCount            return pointer for the document count
                pulTotalTermCount           return pointer for the total term count
                pulUniqueTermCount          return pointer for the unique term count
                pulTotalStopTermCount       return pointer for the total stop term count
                pulUniqueStopTermCount      return pointer for the unique stop term count

    Globals:    none

    Returns:    SPI error code

*/
static int iDfGetIndexInfo
(
    struct spiSession *pssSpiSession,
    struct dfConnection *pdcConnection,
    void *pvIndex,
    unsigned int *puiDocumentCount,
    unsigned long *pulTotalTermCount,
    unsigned long *pulUniqueTermCount,
    unsigned long *pulTotalStopTermCount,
    unsigned long *pulUniqueStopTermCount
)
{


    int                     iError = SPI_NoError;
    struct spiIndexInfo     *psiiSpiIndexInfo = NULL;


    ASSERT(pssSpiSession != NULL);
    ASSERT((pdcConnection != NULL) || (pdcConnection == NULL));
    ASSERT(pvIndex != NULL);
    ASSERT(puiDocumentCount != NULL);
    ASSERT(pulTotalTermCount != NULL);
    ASSERT(pulUniqueTermCount != NULL);
    ASSERT(pulTotalStopTermCount != NULL);
    ASSERT(pulUniqueStopTermCount != NULL);


    /* Local */
    if ( pdcConnection == NULL ) {

        /* Get the index information */
        if ( (iError = iSpiGetIndexInfo(pssSpiSession, pvIndex, &psiiSpiIndexInfo)) != SPI_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the index information, spi error: %d.", iError);
            goto bailFromiDfGetIndexInfo;
        }
    }

    /* Remote */
    else if ( pdcConnection != NULL ) {

        unsigned char   *pucIndexName = (unsigned char *)pvIndex;
        int             iErrorCode = SPI_NoError;
        unsigned char   *pucErrorString = NULL;

        /* Get the index information */
        if ( (iError = iLwpsIndexInfoRequestHandle(pdcConnection->pvLwps, pucIndexName, &psiiSpiIndexInfo, &iErrorCode, &pucErrorString)) != LWPS_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to handle an lwps index info request, lwps error: %d.", iError);
            iError = SPI_GetIndexInfoFailed;
            goto bailFromiDfGetIndexInfo;
        }
        else {
            /* Check for errors */
            if ( iErrorCode != SPI_NoError ) {
                iUtlLogError(UTL_LOG_CONTEXT, "Failed to retrieve the index information, error code: %d, error text: '%s'.", 
                        iErrorCode, pucUtlStringsGetPrintableString(pucErrorString));
                s_free(pucErrorString);
                iError = iErrorCode;
                goto bailFromiDfGetIndexInfo;
            }
        }
    }



    /* Bail label */
    bailFromiDfGetIndexInfo:

    /* Handle the error */
    if ( iError == SPI_NoError ) {

        /* Set the return pointers */
        *puiDocumentCount = psiiSpiIndexInfo->uiDocumentCount;
        *pulTotalTermCount = psiiSpiIndexInfo->ulTotalTermCount;
        *pulUniqueTermCount = psiiSpiIndexInfo->ulUniqueTermCount;
        *pulTotalStopTermCount = psiiSpiIndexInfo->ulTotalStopTermCount;
        *pulUniqueStopTermCount = psiiSpiIndexInfo->ulUniqueStopTermCount;
    }

    /* Free the index information */
    iSpiFreeIndexInfo(psiiSpiIndexInfo);
    psiiSpiIndexInfo = NULL;


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iDfGetIndexTermInfo()

    Purpose:    This function gets the term counts from the index

    Parameters: pssSpiSession           pointer to the spi session structure
                pdcConnection           pointer to the connection structure
                pvIndex                 pointer to the index structure
                ppdtiDFTermInfo         return pointer for a term info array
                puiDfTermInfoLength     return pointer to hold the length of the term info array

    Globals:    none

    Returns:    SPI error code

*/
static int iDfGetIndexTermInfo
(
    struct spiSession *pssSpiSession,
    struct dfConnection *pdcConnection,
    void *pvIndex,
    struct dfTermInfo **ppdtiDFTermInfo,
    unsigned int *puiDfTermInfoLength
)
{

    int                     iError = SPI_NoError;
    struct dfTermInfo       *pdtiDfTermInfo = NULL;
    struct dfTermInfo       *pdtiDfTermInfoPtr = NULL;
    struct spiTermInfo      *ptiSpiTermInfo = NULL;
    struct spiTermInfo      *ptiSpiTermInfoPtr = NULL;
    unsigned int            uiSpiTermInfosLength = 0;
    unsigned int            uiI = 0;


    ASSERT(pssSpiSession != NULL);
    ASSERT((pdcConnection != NULL) || (pdcConnection == NULL));
    ASSERT(pvIndex != NULL);
    ASSERT(ppdtiDFTermInfo != NULL);
    ASSERT(puiDfTermInfoLength != NULL);


    /* Local */
    if ( pdcConnection == NULL ) {

        /* Get the terms */
        if ( (iError = iSpiGetIndexTermInfo(pssSpiSession, pvIndex, SPI_TERM_MATCH_REGULAR, SPI_TERM_CASE_INSENSITIVE, NULL, NULL,
                    &ptiSpiTermInfo, &uiSpiTermInfosLength)) != SPI_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the term information, spi error: %d.", iError);
            goto bailFromiDfGetIndexTermInfo;
        }
    }
    
    /* Remote */
    else if ( pdcConnection != NULL ) {

        unsigned char   *pucIndexName = (unsigned char *)pvIndex;
        int             iErrorCode = SPI_NoError;
        unsigned char   *pucErrorString = NULL;


        /* Get the terms */
        if ( (iError = iLwpsIndexTermInfoRequestHandle(pdcConnection->pvLwps, pucIndexName,  SPI_TERM_MATCH_REGULAR, SPI_TERM_CASE_INSENSITIVE, NULL, NULL, 
                &ptiSpiTermInfo, &uiSpiTermInfosLength, &iErrorCode, &pucErrorString)) != LWPS_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to handle an lwps index term info request, lwps error: %d.", iError);
            iError = SPI_GetIndexTermInfoFailed;
            goto bailFromiDfGetIndexTermInfo;
        }
        else {
            /* Check for errors */
            if ( iErrorCode != SPI_NoError ) {
                iUtlLogError(UTL_LOG_CONTEXT, "Failed to retrieve the term information, error code: %d, error text: '%s'.", 
                        iErrorCode, pucUtlStringsGetPrintableString(pucErrorString));
                s_free(pucErrorString);
                iError = iErrorCode;
                goto bailFromiDfGetIndexTermInfo;
            }
        }
    }



    /* Allocate the new term info array */
    if ( (pdtiDfTermInfo = (struct dfTermInfo *)s_malloc((size_t)(sizeof(struct dfTermInfo) * uiSpiTermInfosLength))) == NULL ) {
        iError = SPI_MemError;
        goto bailFromiDfGetIndexTermInfo;
    }


    /* Transfer the terms into the term info array */
    for ( uiI = 0, pdtiDfTermInfoPtr = pdtiDfTermInfo, ptiSpiTermInfoPtr = ptiSpiTermInfo; uiI < uiSpiTermInfosLength; uiI++, pdtiDfTermInfoPtr++, ptiSpiTermInfoPtr++) {

        /* Transfer the term pointer and NULL out the source
        ** pointer so that it is not freed by iSpiFreeTermInfo()
        */
        pdtiDfTermInfoPtr->pucTerm = ptiSpiTermInfoPtr->pucTerm;
        ptiSpiTermInfoPtr->pucTerm = NULL;

        /* Transfer the term count */
        pdtiDfTermInfoPtr->uiCount = ptiSpiTermInfoPtr->uiCount;
    }



    /* Bail label */
    bailFromiDfGetIndexTermInfo:

    /* Handle the error */
    if ( iError == SPI_NoError ) {

        /* Set the return pointers */
        *ppdtiDFTermInfo = pdtiDfTermInfo;
        *puiDfTermInfoLength = uiSpiTermInfosLength;
    }

    /* Free the term information */
    iSpiFreeTermInfo(ptiSpiTermInfo, uiSpiTermInfosLength);
    ptiSpiTermInfo = NULL;


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iDfGetIndexDocumentIDs()

    Purpose:    This returns a number of document keys from the index.
                Ideally we want to return a largish number of documents.
                If the search engine has a way to return all the document
                IDs, that is great otherwise we have to run a broad search
                which will return us lots of documents and use that.

                If a search is passed in, we will use that, otherwise we
                will generate our own search.

                We also return up to 100,000 document keys.

    Parameters: pssSpiSession               pointer to the spi session structure
                pdcConnection               pointer to the connection structure
                pvIndex                     pointer to the index structure
                pucLanguageCode             language code
                pucSearchText               search text
                ppddkDfDocumentKey          return pointer for the document key array
                puiDfDocumentIDLength       return pointer to hold the length of the document key array

    Globals:    none

    Returns:    SPI error code

*/
static int iDfGetIndexDocumentIDs
(
    struct spiSession *pssSpiSession,
    struct dfConnection *pdcConnection,
    void *pvIndex,
    unsigned char *pucLanguageCode,
    unsigned char *pucSearchText,
    struct dfDocumentKey **ppddkDfDocumentKey,
    unsigned int *puiDfDocumentIDLength
)
{

    int                         iError = SPI_NoError;    
    unsigned int                uStartIndex = 0;
    unsigned int                uiEndIndex = DF_DOCUMENT_ID_COUNT_MAX;
    struct spiSearchResponse    *pssrSpiSearchResponse = NULL;
    struct spiSearchResult      *pssrSpiSearchResultsPtr = NULL;
    struct dfDocumentKey        *pddkDfDocumentKey = NULL;
    unsigned int                uiDfDocumentIDLength = 0;
    struct dfDocumentKey        *pddkDfDocumentKeyPtr = NULL;
    unsigned int                uiI = 0;


    ASSERT(pssSpiSession != NULL);
    ASSERT((pdcConnection != NULL) || (pdcConnection == NULL));
    ASSERT(pvIndex != NULL);
    ASSERT((bUtlStringsIsStringNULL(pucLanguageCode) == false) || (bUtlStringsIsStringNULL(pucLanguageCode) == true));
    ASSERT((bUtlStringsIsStringNULL(pucSearchText) == false) || (bUtlStringsIsStringNULL(pucSearchText) == true));
    ASSERT(ppddkDfDocumentKey != NULL);
    ASSERT(puiDfDocumentIDLength != NULL);


    /* Local */
    if ( pdcConnection == NULL ) {

        void    *ppvIndexList[2] = {NULL, NULL};

        /* Set the index structure */
        ppvIndexList[0] = pvIndex;
        ppvIndexList[1] = NULL;

        /* Run the search, here we need to run a search that will return lots of document keys */
        if ( (iError = iSpiSearchIndex(pssSpiSession, ppvIndexList, pucLanguageCode, (bUtlStringsIsStringNULL(pucSearchText) == false) ? pucSearchText : (unsigned char *)"a*", 
                    NULL, NULL, uStartIndex, uiEndIndex, &pssrSpiSearchResponse)) != SPI_NoError ) {

            iUtlLogError(UTL_LOG_CONTEXT, "Failed to search the index, spi error: %d.", iError);
            goto bailFromiDfGetIndexDocumentIDs;
        }
    }
    
    /* Remote */
    else if ( pdcConnection != NULL ) {

        unsigned char   *pucIndexName = (unsigned char *)pvIndex;
        unsigned char   *ppucIndexNameList[2] = {NULL, NULL};
        int             iErrorCode = SPI_NoError;
        unsigned char   *pucErrorString = NULL;


        /* Set the index structure */
        ppucIndexNameList[0] = pucIndexName;
        ppucIndexNameList[1] = NULL;

        /* Run the search, here we need to run a search that will return lots of document keys */
        if ( (iError = iLwpsSearchRequestHandle(pdcConnection->pvLwps, ppucIndexNameList, pucLanguageCode, pucUtlStringsGetDefaultString(pucSearchText, (unsigned char *)"a*"), NULL, NULL, 
                uStartIndex, uiEndIndex, &pssrSpiSearchResponse, &iErrorCode, &pucErrorString)) != LWPS_NoError ) {

            iUtlLogError(UTL_LOG_CONTEXT, "Failed to handle an lwps search request, lwps error: %d.", iError);
            iError = SPI_SearchIndexFailed;
            goto bailFromiDfGetIndexDocumentIDs;
        }
        else {

            /* Check for errors */
            if ( iErrorCode != SPI_NoError ) {
                iUtlLogError(UTL_LOG_CONTEXT, "Failed to search the index, error code: %d, error text: '%s'.", iErrorCode, pucUtlStringsGetPrintableString(pucErrorString));
                s_free(pucErrorString);
                iError = iErrorCode;
                goto bailFromiDfGetIndexDocumentIDs;
            }
        }
    }



    /* Set the document key array length, ommit the search report at the end */
    uiDfDocumentIDLength = (pssrSpiSearchResponse->uiSpiSearchResultsLength - 1);


    /* Allocate the new document key array */
    if ( (pddkDfDocumentKey = (struct dfDocumentKey *)s_malloc((size_t)(sizeof(struct dfDocumentKey) * uiDfDocumentIDLength))) == NULL ) {
        iError = SPI_MemError;
        goto bailFromiDfGetIndexDocumentIDs;
    }


    /* Transfer the document keys into the document key array */
    for ( uiI = 0, pddkDfDocumentKeyPtr = pddkDfDocumentKey, pssrSpiSearchResultsPtr = pssrSpiSearchResponse->pssrSpiSearchResults; 
            uiI < uiDfDocumentIDLength; uiI++, pddkDfDocumentKeyPtr++, pssrSpiSearchResultsPtr++ ) {

        /* Transfer the document key pointer and NULL out the source pointer so that it is not freed by iSpiFreeSearchResults() */
        pddkDfDocumentKeyPtr->pucDocumentKey = pssrSpiSearchResultsPtr->pucDocumentKey;
        pssrSpiSearchResultsPtr->pucDocumentKey = NULL;
    }



    /* Bail label */
    bailFromiDfGetIndexDocumentIDs:

    /* Handle the error */
    if ( iError == SPI_NoError ) {

        /* Set the return pointers */
        *ppddkDfDocumentKey = pddkDfDocumentKey;
        *puiDfDocumentIDLength = uiDfDocumentIDLength;
    }

    /* Free the search response we got from the index */
    iSpiFreeSearchResponse(pssrSpiSearchResponse);
    pssrSpiSearchResponse = NULL;


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iDfSearchIndex()

    Purpose:    This runs a search against the index and returns the
                number of results we got.

    Parameters: pssSpiSession       pointer to the spi session structure
                pdcConnection       pointer to the connection structure
                pvIndex             pointer to the index structure
                pucLanguageCode     language code
                pucSearchText       search text
                puiHitsCount        return pointer for the number of results we got

    Globals:    none

    Returns:    SPI error code

*/
static int iDfSearchIndex
(
    struct spiSession *pssSpiSession,
    struct dfConnection *pdcConnection,
    void *pvIndex,
    unsigned char *pucLanguageCode,
    unsigned char *pucSearchText,
    unsigned int *puiHitsCount
)
{

    int                         iError = SPI_NoError;
    unsigned int                uStartIndex = 0;
    unsigned int                uiEndIndex = 0;
    struct spiSearchResponse    *pssrSpiSearchResponse = NULL;


    ASSERT(pssSpiSession != NULL);
    ASSERT((pdcConnection != NULL) || (pdcConnection == NULL));
    ASSERT(pvIndex != NULL);
    ASSERT((bUtlStringsIsStringNULL(pucLanguageCode) == false) || (bUtlStringsIsStringNULL(pucLanguageCode) == true));
    ASSERT((bUtlStringsIsStringNULL(pucSearchText) == false) || (bUtlStringsIsStringNULL(pucSearchText) == true));
    ASSERT(puiHitsCount != NULL);


    /* Local */
    if ( pdcConnection == NULL ) {

        void    *ppvIndexList[2] = {NULL, NULL};

        /* Set the index structure */
        ppvIndexList[0] = pvIndex;
        ppvIndexList[1] = NULL;

        /* Run the search */
        if ( (iError = iSpiSearchIndex(pssSpiSession, ppvIndexList, pucLanguageCode, pucSearchText, NULL, NULL, 
                    uStartIndex, uiEndIndex, &pssrSpiSearchResponse)) != SPI_NoError ) {

            iUtlLogError(UTL_LOG_CONTEXT, "Failed to search the index, spi error: %d.", iError);
            goto bailFromiDfSearchIndex;
        }
    }
    
    /* Remote */
    else if ( pdcConnection != NULL ) {

        unsigned char   *pucIndexName = (unsigned char *)pvIndex;
        unsigned char   *ppucIndexNameList[2] = {NULL, NULL};
        int             iErrorCode = SPI_NoError;
        unsigned char   *pucErrorString = NULL;


        /* Set the index structure */
        ppucIndexNameList[0] = pucIndexName;
        ppucIndexNameList[1] = NULL;

        /* Run the search */
        if ( (iError = iLwpsSearchRequestHandle(pdcConnection->pvLwps, ppucIndexNameList, pucLanguageCode, pucSearchText, NULL, NULL, 
                uStartIndex, uiEndIndex, &pssrSpiSearchResponse, &iErrorCode, &pucErrorString)) != LWPS_NoError ) {

            iUtlLogError(UTL_LOG_CONTEXT, "Failed to handle an lwps search request, lwps error: %d.", iError);
            iError = SPI_SearchIndexFailed;
            goto bailFromiDfSearchIndex;
        }
        else {

            /* Check for errors */
            if ( iErrorCode != SPI_NoError ) {
                iUtlLogError(UTL_LOG_CONTEXT, "Failed to search the index, error code: %d, error text: '%s'.", iErrorCode, pucUtlStringsGetPrintableString(pucErrorString));
                s_free(pucErrorString);
                iError = iErrorCode;
                goto bailFromiDfSearchIndex;
            }
        }
    }



    /* Bail label */
    bailFromiDfSearchIndex:

    /* Handle the error */
    if ( iError == SPI_NoError ) {

        /* Set the return pointers */
        *puiHitsCount = pssrSpiSearchResponse->uiSpiSearchResultsLength;
    }

    /* Free the search response we got from the index */
    iSpiFreeSearchResponse(pssrSpiSearchResponse);
    pssrSpiSearchResponse = NULL;


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iDfRetrieveDocument()

    Purpose:    This gets a document from the index.

    Parameters: pssSpiSession       pointer to the spi session structure
                pdcConnection       pointer to the connection structure
                pvIndex             pointer to the index structure
                pucDocumentKey      document key

    Globals:    none

    Returns:    0 on success, -1 on error

*/
static int iDfRetrieveDocument
(
    struct spiSession *pssSpiSession,
    struct dfConnection *pdcConnection,
    void *pvIndex,
    unsigned char *pucDocumentKey
)
{

    int             iError = SPI_NoError;
    void            *pvData = NULL;
    unsigned int    uiDataLength = 0;


    ASSERT(pssSpiSession != NULL);
    ASSERT((pdcConnection != NULL) || (pdcConnection == NULL));
    ASSERT(pvIndex != NULL);
    ASSERT(bUtlStringsIsStringNULL(pucDocumentKey) == false);


    /* Local */
    if ( pdcConnection == NULL ) {

        /* Get the document, note that we default the document item name and mime type */
        if ( (iError = iSpiRetrieveDocument(pssSpiSession, pvIndex, pucDocumentKey, "document", "text/plain", 
                SPI_CHUNK_TYPE_DOCUMENT, 0, 0, &pvData, &uiDataLength)) != SPI_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the document, spi error: %d.", iError);
            goto bailFromiRetrieveDocument;
        }
    }
    
    /* Remote */
    else if ( pdcConnection != NULL ) {

        unsigned char   *pucIndexName = (unsigned char *)pvIndex;
        int             iErrorCode = SPI_NoError;
        unsigned char   *pucErrorString = NULL;


        /* Get the document, note that we default the document item name and mime type */
        if ( (iError = iLwpsRetrievalRequestHandle(pdcConnection->pvLwps, pucIndexName, pucDocumentKey, "document", "text/plain", 
                    SPI_CHUNK_TYPE_DOCUMENT, 0, 0, &pvData, &uiDataLength, &iErrorCode, &pucErrorString)) != LWPS_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to handle an lwps retrieval request, lwps error: %d.", iError);
            iError = SPI_RetrieveDocumentFailed;
            goto bailFromiRetrieveDocument;
        }
        else {

            /* Check for errors */
            if ( iErrorCode != SPI_NoError ) {
                iUtlLogError(UTL_LOG_CONTEXT, "Failed to retrieve the document, error code: %d, error text: '%s'.", 
                        iErrorCode, pucUtlStringsGetPrintableString(pucErrorString));
                s_free(pucErrorString);
                iError = iErrorCode;
                goto bailFromiRetrieveDocument;
            }
        }
    }



    /* Bail label */
    bailFromiRetrieveDocument:

    /* Free the document */
    s_free(pvData);


    /* End of document is not really an error */
    return (iError);
}


/*---------------------------------------------------------------------------*/


/* 
** ========================================
** ===  Miscellanous Support Functions  ===
** ========================================
*/


/*

    Function:   vDfSortTermInfoDesc()

    Purpose:    This functions sorts a dfTermInfo array in descending term info order, it
                implements a quicksort algorithm for speed. The reason we use
                this guy rather than qsort() is for sheer speed, we get a 
                2:1 to 3:1 speed increase because the condition is inline.

    Parameters: pdtiDfTermInfo      pointer to a term info array
                iLocalLeftIndex     left hand index in the array to sort 
                iLocalRightIndex    right hand index in the array to sort 

    Globals:    none

    Returns:    void

*/
static void vDfSortTermInfoDesc
(
    struct dfTermInfo *pdtiDfTermInfo,
    int iLeftIndex,
    int iRightIndex
)
{
    int                 iLocalLeftIndex = 0;
    int                 iLocalRightIndex = 0;
    struct dfTermInfo   *pdtiDfTermInfoRightIndexPtr = pdtiDfTermInfo + iRightIndex;


    ASSERT(pdtiDfTermInfo != NULL)


    if ( iRightIndex > iLeftIndex ) {

        iLocalLeftIndex = iLeftIndex - 1; 
        iLocalRightIndex = iRightIndex;

        for ( ;; ) {

            while ( (++iLocalLeftIndex <= iRightIndex) && ((pdtiDfTermInfo + iLocalLeftIndex)->uiCount > pdtiDfTermInfoRightIndexPtr->uiCount) );
            while ( (--iLocalRightIndex > iLeftIndex) && ((pdtiDfTermInfo + iLocalRightIndex)->uiCount < pdtiDfTermInfoRightIndexPtr->uiCount) );

            if ( iLocalLeftIndex >= iLocalRightIndex ) {
                break;
            }

            DF_SWAP_TERM_INFO((pdtiDfTermInfo + iLocalRightIndex), (pdtiDfTermInfo + iLocalLeftIndex));
        }

        if ( iLocalLeftIndex != iRightIndex ) {
            DF_SWAP_TERM_INFO(pdtiDfTermInfoRightIndexPtr, (pdtiDfTermInfo + iLocalLeftIndex));
        }

        vDfSortTermInfoDesc(pdtiDfTermInfo, iLeftIndex, iLocalLeftIndex - 1);
        vDfSortTermInfoDesc(pdtiDfTermInfo, iLocalLeftIndex + 1, iRightIndex);
    }


    return;

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iDfGetRandomTermInfoIndex()

    Purpose:    This gets a random term info index into the term 
                count structure according to the deFazio rules for 
                term selection

    Parameters: uiDfTermInfoLength      length of the array
                uiTermInfoIndex90       high boundary
                uiTermInfoIndex5        low boundary

    Globals:    none

    Returns:    the term info index

*/
static int iDfGetRandomTermInfoIndex
(
    unsigned int uiDfTermInfoLength,
    unsigned int uiTermInfoIndex90,
    unsigned int uiTermInfoIndex5
)
{

    unsigned int    uiSegment = 0;
    unsigned int    uiTermInfoIndex = 0;


    ASSERT(uiDfTermInfoLength > 0);
    ASSERT((uiTermInfoIndex90 > 0) && (uiTermInfoIndex90 < uiDfTermInfoLength) && (uiTermInfoIndex90 < uiTermInfoIndex5));
    ASSERT((uiTermInfoIndex5 > 0) && (uiTermInfoIndex5 < uiDfTermInfoLength) && (uiTermInfoIndex5 > uiTermInfoIndex90));


    /* First we pick the segment from which we will pick this term */
    iUtlRandGetRand(2, &uiSegment);

    /* Then we pick the term from the chosen segment */
    switch ( uiSegment ) {

        /* High use segment */
        case 0:
            iUtlRandGetRand(uiTermInfoIndex90, &uiTermInfoIndex);
            ASSERT((uiTermInfoIndex >= 0) && (uiTermInfoIndex <= uiTermInfoIndex90));
            break;

        /* Moderate use segment */
        case 1:
            iUtlRandGetRand(uiTermInfoIndex5 - (uiTermInfoIndex90 + 1), &uiTermInfoIndex);
            uiTermInfoIndex += (uiTermInfoIndex90 + 1);
            ASSERT((uiTermInfoIndex > uiTermInfoIndex90) && (uiTermInfoIndex <= uiTermInfoIndex5));
            break;

        /* Low use segment */
        case 2:
            iUtlRandGetRand((uiDfTermInfoLength - 1) - (uiTermInfoIndex5 + 1), &uiTermInfoIndex);
            uiTermInfoIndex += (uiTermInfoIndex5 + 1);
            ASSERT((uiTermInfoIndex > uiTermInfoIndex5) && (uiTermInfoIndex <= (uiDfTermInfoLength - 1)));
            break;
    }


    return (uiTermInfoIndex);

}


/*---------------------------------------------------------------------------*/
