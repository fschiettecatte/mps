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

    Module:     server.c

    Author:     Francois Schiettecatte (FS Consulting LLC.)

    Created:    4 February 1994

    Purpose:    This is where the main() is implemented for the
                mpsserver and mpsgateway

*/


/*---------------------------------------------------------------------------*/

/*
** Includes
*/

#include "utils.h"

#include "lng.h"

#include "spi.h"

#include "server.h"


/* To include a protocol, all we need to pull in the appropriate include file */
#include "srvr_lwps.h"
#include "srvr_http.h"


/*---------------------------------------------------------------------------*/


/*
** Feature defines
*/


/* Context for logging */
#undef UTL_LOG_CONTEXT
#define UTL_LOG_CONTEXT                     (unsigned char *)"com.fsconsult.mps.src.server.server"


/* Enable the accept timeout override.
** 
** We do this because there appears to be a sporadic issue where
** accept() actually accepts a connection when it has already been
** accepted by someone else and hence should return that the accept()
** call would have blocked. This causes the connection to sit there
** for the duration of the timeout, waiting for something to happen.
** 
** Note that this happens sporatically and is not readily reproducible,
** and may even be limited to certain versions of linux.
** 
*/
/* #define SRVR_SERVER_ENABLE_ACCEPT_TIMEOUT_OVERRIDE */


/* Enable a threaded server */
#define SRVR_SERVER_ENABLE_THREADED_SERVER


/* Enable this to get the client host name from DNS, this slows things down considerably */
/* #define SRVR_SERVER_ENABLE_CLIENT_HOSTNAME_LOOKUP */


/* Logs the protocol used by the client */
/* #define SRVR_SERVER_ENABLE_PROTOCOL_LOGGING */


/*---------------------------------------------------------------------------*/


/*
** Defines
*/

/* Default index directory path */
#define SRVR_SERVER_INDEX_DIRECTORY_PATH_DEFAULT            (unsigned char *)"./"

/* Default configuration directory path */
#define SRVR_SERVER_CONFIGURATION_DIRECTORY_PATH_DEFAULT    SRVR_SERVER_INDEX_DIRECTORY_PATH_DEFAULT

/* Default temporary directory path */
#define SRVR_SERVER_TEMPORARY_DIRECTORY_PATH_DEFAULT        (unsigned char *)P_tmpdir


/* Session count default */
#define SRVR_SERVER_SESSION_COUNT_DEFAULT                   (200)    


/* Default startup interval between starting child processes - set in milliseconds */
#define SRVR_SERVER_STARTUP_INTERVAL_DEFAULT                (250)


/* Default protocol name */
#define SRVR_SERVER_NET_PROTOCOL_NAME_DEFAULT               UTL_NET_PROTOCOL_TCP_NAME


/* Timeout default - set in milliseconds */
#define SRVR_SERVER_TIMEOUT_DEFAULT                         (60000)    


/* Maximum load default */
#define SRVR_SERVER_LOAD_MAXIMUM_DEFAULT                    (5.0)    


/* Default locale name */
#define SRCH_SERVER_LOCALE_NAME_DEFAULT                     LNG_LOCALE_EN_US_UTF_8_NAME


/* Thread check interval - set in microseconds */
#define SRVR_SERVER_THREAD_CHECK_INTERVAL                   (100000)    


/* Accept timeout override - set in milliseconds */
#if defined(SRVR_SERVER_ENABLE_ACCEPT_TIMEOUT_OVERRIDE)
#define SRVR_SERVER_ACCEPT_TIMEOUT_OVERRIDE                 (5000)
#endif /* defined(SRVR_SERVER_ENABLE_ACCEPT_TIMEOUT_OVERRIDE) */


/* MPS Server and Gateway command names */
#define SRVR_SERVER_MPSSERVER_COMMAND_NAME                  (unsigned char *)"mpsserver"
#define SRVR_SERVER_MPSGATEWAY_COMMAND_NAME                 (unsigned char *)"mpsgateway"


/*---------------------------------------------------------------------------*/


/*
** Structures
*/

/* Server protocol handler structure */
struct srvrServerProtocolHandler {
    unsigned char   *pucProtocolName;                       /* Protocol name string */
    unsigned char   ucProtocolHeaderByte;                   /* Protocol header byte character */
    int             (*iSrvrProtocolHandler)();              /* Protocol handler function */
};


/* Server socket structure */
struct srvrServerSocket {
    unsigned int    uiProtocol;                             /* Protocol - tcp/udp */
    unsigned char   pucHostName[MAXHOSTNAMELEN + 1];        /* Host name */
    int             iPort;                                  /* Port */
};


/*---------------------------------------------------------------------------*/


/*
** Globals 
*/


/* Server protocol handlers */
static struct srvrServerProtocolHandler pssphSrvrServerProtocolHandlersGlobal[] = 
{
#if defined(SRVR_LWPS_H)
    {   SRVR_LWPS_PROTOCOL_NAME,
        SRVR_LWPS_PROTOCOL_HEADER_BYTE,
        iSrvrProtocolHandlerLwps,
    },
#endif    /* defined(SRVR_LWPS_H) */

#if defined(SRVR_HTTP_H)
    {   SRVR_HTTP_PROTOCOL_NAME,
        SRVR_HTTP_PROTOCOL_HEADER_BYTE,
        iSrvrProtocolHandlerHttp,
    },
#endif    /* defined(SRVR_HTTP_H) */

    /* End marker */
    {   NULL,
        '\0',
        NULL,
    },
};



/* Server session global */
static volatile struct srvrServerSession    *psssSrvrServerSessionsGlobal = NULL;
static volatile unsigned int                uiSrvrServerSessionsLengthGlobal = 0;


#if defined(SRVR_SERVER_ENABLE_THREADED_SERVER) 

/* Server thread ID global */
static volatile pthread_t                   zSrvrServerThreadIDGlobal = -1;

/* Server thread count global - -1 indicates that this is not a threaded server */
static volatile int                         iSrvrServerThreadCountGlobal = -1;

/* Server thread mutex to control access to the thread count global */
static volatile pthread_mutex_t             mSrvrServerThreadMutexGlobal = PTHREAD_MUTEX_INITIALIZER;

#endif    /* defined(SRVR_SERVER_ENABLE_THREADED_SERVER) */


/* Server termination requested global */
static volatile boolean                     bSrvrServerTerminationRequestedGlobal = false;


/*---------------------------------------------------------------------------*/


/*
** Private function prototypes
*/

static void vVersion (unsigned char *pucCommandPath);

static void vUsage (unsigned char *pucCommandPath);


static void vSrvrServerNonFatalSignalHandler (int iSignal);

static void vSrvrServerFatalSignalHandler (int iSignal);


static int iSrvrServerSetUserName (unsigned char *pucUserName);


static int iSrvrServerServeSocketClient (struct srvrServerSession *psssSrvrServerSession);

static int iSrvrServerServeClient (struct srvrServerSession *psssSrvrServerSession);


/*---------------------------------------------------------------------------*/


/*

    Function:   main()

    Purpose:    This function will set up the server and start processing
                incommin requests, note that the server can be set up from
                the inet daemon as well as in background mode.

    Parameters: argc, argv

    Globals:    none

    Returns:    int

*/
int main
(
    int argc,
    char* argv[]
)
{

    int                         iError = SPI_NoError;
    unsigned int                uiI = 0;

    unsigned char               *pucCommandPath = NULL;
    unsigned char               *pucCommandNamePtr = NULL;
    unsigned char               *pucNextArgument = NULL;

    unsigned char               pucConfigurationDirectoryPath[UTL_FILE_PATH_MAX + 1] = {'\0'};
    unsigned char               pucIndexDirectoryPath[UTL_FILE_PATH_MAX + 1] = {'\0'};
    unsigned char               pucTemporaryDirectoryPath[UTL_FILE_PATH_MAX + 1] = {'\0'};

    struct srvrServerSocket     *psssServerSockets = NULL;
    struct srvrServerSocket     *psssServerSocketsPtr = NULL;
    unsigned int                uiServerSocketsLength = 0;

    unsigned char               *pucUserName = NULL;
    unsigned int                uiChildrenCount = 0;
#if defined(SRVR_SERVER_ENABLE_THREADED_SERVER) 
    unsigned int                uiThreadCount = 0;
    unsigned int                uiThreadStackSize = 0;
#endif    /* defined(SRVR_SERVER_ENABLE_THREADED_SERVER) */
    unsigned int                uiStartupInterval = SRVR_SERVER_STARTUP_INTERVAL_DEFAULT;
    boolean                     bDaemonServer = false;
    unsigned char               *pucProcessIDFilePath = NULL;    
    boolean                     bCheck = false;
    unsigned char               *pucLocaleName = SRCH_SERVER_LOCALE_NAME_DEFAULT;

    unsigned char               *pucLogFilePath = UTL_LOG_FILE_STDERR;    
    unsigned int                uiLogLevel = UTL_LOG_LEVEL_INFO;
    
    struct srvrServerSession    *psssSrvrServerSessions = NULL;
    struct srvrServerSession    *psssSrvrServerSession = NULL;
    struct srvrServerSession    *psssSrvrServerSessionsPtr = NULL;
    struct spiSession           *pssSpiSession = NULL;

    boolean                     bSocketServer = false;
    boolean                     bForkServer = false;
    boolean                     bThreadServer = false;



    /* Get the command path */
    pucCommandPath = pucUtlArgsGetNextArg(&argc, &argv);

    /* Get the command name */
    if ( iUtlFileGetPathBase(pucCommandPath, &pucCommandNamePtr) != UTL_NoError ) {
        pucCommandNamePtr = pucCommandPath;
    }


    /* Initialize the log */
    if ( (iError = iUtlLogInit()) != UTL_NoError ) {
        vVersion(pucCommandPath);
        iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to initialize the log, utl error: %d", iError);
    }


    /* Allocate the spi session structure */
    if ( (pssSpiSession = (struct spiSession *)s_malloc((size_t)(sizeof(struct spiSession)))) == NULL ) {
        vVersion(pucCommandPath);
        iUtlLogPanic(UTL_LOG_CONTEXT, "Memory allocation error");
    }

    /* Initialize the spi session structure */
    pssSpiSession->pucIndexDirectoryPath = NULL;
    pssSpiSession->pucConfigurationDirectoryPath = NULL;
    pssSpiSession->pucTemporaryDirectoryPath = NULL;
    pssSpiSession->pvClientPtr = NULL;


    /* Allocate the server session structure */
    if ( (psssSrvrServerSession = (struct srvrServerSession *)s_malloc((size_t)(sizeof(struct srvrServerSession)))) == NULL ) {
        vVersion(pucCommandPath);
        iUtlLogPanic(UTL_LOG_CONTEXT, "Memory allocation error");
    }

    /* Initialize the server session structure */
    psssSrvrServerSession->pssSpiSession = pssSpiSession;
    psssSrvrServerSession->pvUtlNet = NULL;
    psssSrvrServerSession->uiTimeOut = SRVR_SERVER_TIMEOUT_DEFAULT;
    psssSrvrServerSession->dLoadMaximum = SRVR_SERVER_LOAD_MAXIMUM_DEFAULT;
    psssSrvrServerSession->dConnectionLoadMaximum = -1;
    psssSrvrServerSession->dSearchLoadMaximum = -1;
    psssSrvrServerSession->dRetrievalLoadMaximum = -1;
    psssSrvrServerSession->dInformationLoadMaximum = -1;
    psssSrvrServerSession->uiSessionCount = SRVR_SERVER_SESSION_COUNT_DEFAULT;
    psssSrvrServerSession->bActiveSession = false;


    /* Set the server sessions structure and server sessions length globals */
    psssSrvrServerSessionsGlobal = psssSrvrServerSession;
    uiSrvrServerSessionsLengthGlobal = 1;



    /* Put up an error message if there were no arguments defined and bail */
    if ( (pucNextArgument = pucUtlArgsGetNextArg(&argc, &argv)) == NULL ) {
        vVersion(pucCommandPath);
        vUsage(pucCommandPath);
        s_exit(EXIT_SUCCESS);
    }

    /* Cycle through all the arguments */
      while ( pucNextArgument != NULL ) {

        /* Check for socket */
        if ( s_strncmp("--socket=", pucNextArgument, s_strlen("--socket=")) == 0 ) {
        
            unsigned char   pucScanfServerSocket[UTL_STRING_SCANF_FORMAT_LENGTH + 1] = {'\0'};
            int             iStatus = 0;
            unsigned char   pucComponent1[UTL_FILE_PATH_MAX + 1] = {'\0'};
            unsigned char   pucComponent2[UTL_FILE_PATH_MAX + 1] = {'\0'};
            unsigned char   pucComponent3[UTL_FILE_PATH_MAX + 1] = {'\0'};
            unsigned char   *pucProtocolPtr = NULL;
            unsigned char   *pucHostPtr = NULL;
            unsigned char   *pucPortPtr = NULL;
            int             iPort = -1;

        
            /* Get the socket */
            pucNextArgument += s_strlen("--socket=");


            /* Prepare the server socket scan string */
            snprintf(pucScanfServerSocket, UTL_STRING_SCANF_FORMAT_LENGTH + 1, "%%%d[^:]:%%%d[^:]:%%%ds", UTL_FILE_PATH_MAX + 1, UTL_FILE_PATH_MAX + 1, UTL_FILE_PATH_MAX + 1);

            /* Scan the socket to parse out the protocol, the host and the port */
            if ( (iStatus = sscanf(pucNextArgument, pucScanfServerSocket, pucComponent1, pucComponent2, pucComponent3)) <= 0 ) {
                vVersion(pucCommandPath);
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
/* printf("pucProtocolPtr: '%s', pucHostPtr: '%s', pucPortPtr: '%s'\n", pucProtocolPtr, pucHostPtr, pucPortPtr); */

            /* Extract the port */
            iPort = s_strtol(pucPortPtr, NULL, 0);

            /* Check the port number to see if we can access it */
            if ( (iPort < 1024) && (s_getuid() != 0) ) {
                vVersion(pucCommandPath);
                iUtlLogPanic(UTL_LOG_CONTEXT, "Must be super-user to open a port less than or equal to 1024, in socket argument: '%s'", pucNextArgument);
            }

            /* Check the protocol if it was specified */
            if ( (bUtlStringsIsStringNULL(pucProtocolPtr) == false) && 
                    ((s_strcmp(pucProtocolPtr, UTL_NET_PROTOCOL_TCP_NAME) != 0) && (s_strcmp(pucProtocolPtr, UTL_NET_PROTOCOL_UDP_NAME) != 0)) ) {
                iUtlLogPanic(UTL_LOG_CONTEXT, "Invalid protocol in socket argument: '%s'", pucNextArgument);
            }
                
            
            /* Extend the server sockets array by one entry */
            if ( (psssServerSockets = (struct srvrServerSocket *)s_realloc(psssServerSockets,
                    (size_t)(sizeof(struct srvrServerSocket) * (uiServerSocketsLength + 1)))) == NULL ) {
                vVersion(pucCommandPath);
                iUtlLogPanic(UTL_LOG_CONTEXT, "Memory allocation error");
            }
            
            /* Dereference the server sockets pointer, clear the new structure and increment the server sockets length */
            psssServerSocketsPtr = psssServerSockets + uiServerSocketsLength;
            s_memset(psssServerSocketsPtr, 0, sizeof(struct srvrServerSocket));
            uiServerSocketsLength++;
            
            
            /* Set the fields in the server socket structure */
            if ( bUtlStringsIsStringNULL(pucHostPtr) == false ) {
                s_strcpy(psssServerSocketsPtr->pucHostName, pucHostPtr);
            }
            if ( (bUtlStringsIsStringNULL(pucProtocolPtr) == false) && (s_strcmp(pucProtocolPtr, UTL_NET_PROTOCOL_TCP_NAME) == 0) ) {
                psssServerSocketsPtr->uiProtocol = UTL_NET_PROTOCOL_TCP_ID;
            }
            else if ( (bUtlStringsIsStringNULL(pucProtocolPtr) == false) && (s_strcmp(pucProtocolPtr, UTL_NET_PROTOCOL_UDP_NAME) == 0) ) {
                psssServerSocketsPtr->uiProtocol = UTL_NET_PROTOCOL_UDP_ID;
            }
            else {
                psssServerSocketsPtr->uiProtocol = UTL_NET_PROTOCOL_TCP_ID;
            }
            psssServerSocketsPtr->iPort = iPort;        


            /* We have a socket so we are a socket server */
            bSocketServer = true;
        }

        /* Check for timeout */
        else if ( s_strncmp("--timeout=", pucNextArgument, s_strlen("--timeout=")) == 0 ) {

            /* Get the timeout */
            pucNextArgument += s_strlen("--timeout=");

            /* Check the timeout */
            if ( s_strtol(pucNextArgument, NULL, 10) < 0 ) {
                vVersion(pucCommandPath);
                iUtlLogPanic(UTL_LOG_CONTEXT, "Expected the timeout to be equal to or greater than: 0");
            }
            
            /* Set the timeout */
            psssSrvrServerSession->uiTimeOut = s_strtol(pucNextArgument, NULL, 10);
        }

        /* Check for configuration directory */
        else if ( s_strncmp("--configuration-directory=", pucNextArgument, s_strlen("--configuration-directory=")) == 0 ) {

            /* Get and check the user name */
            pucNextArgument += s_strlen("--configuration-directory=");

            /* Get the true configuration directory path */
            if ( (iError = iUtlFileGetTruePath(pucNextArgument, pucConfigurationDirectoryPath, UTL_FILE_PATH_MAX + 1)) != UTL_NoError ) {
                vVersion(pucCommandPath);
                iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to get the true configuration directory path: '%s', utl error: %d", pucNextArgument, iError);
            }

            /* Check that configuration directory path exists */
            if ( bUtlFilePathExists(pucConfigurationDirectoryPath) == false ) {
                vVersion(pucCommandPath);
                iUtlLogPanic(UTL_LOG_CONTEXT, "The configuration directory: '%s', does not exist", pucConfigurationDirectoryPath);
            }

            /* Check that the configuration directory path is a directory */
            if ( bUtlFileIsDirectory(pucConfigurationDirectoryPath) == false ) {
                vVersion(pucCommandPath);
                iUtlLogPanic(UTL_LOG_CONTEXT, "The configuration directory: '%s', is not a directory", pucConfigurationDirectoryPath);
            }

            /* Check that the configuration directory path can be accessed */
            if ( (bUtlFilePathRead(pucConfigurationDirectoryPath) == false) || (bUtlFilePathExec(pucConfigurationDirectoryPath) == false) ) {
                vVersion(pucCommandPath);
                iUtlLogPanic(UTL_LOG_CONTEXT, "The configuration directory: '%s', cannot be accessed", pucConfigurationDirectoryPath);
            }

            /* Set the configuration directory path */
            if ( (pssSpiSession->pucConfigurationDirectoryPath = s_strdup(pucConfigurationDirectoryPath)) == NULL ) {
                vVersion(pucCommandPath);
                iUtlLogPanic(UTL_LOG_CONTEXT, "Memory allocation failed");
            }
        }

        /* Check for true index directory path */
        else if ( s_strncmp("--index-directory=", pucNextArgument, s_strlen("--index-directory=")) == 0 ) {

            /* Get the index directory path */
            pucNextArgument += s_strlen("--index-directory=");

            /* Get the index directory path */
            if ( (iError = iUtlFileGetTruePath(pucNextArgument, pucIndexDirectoryPath, UTL_FILE_PATH_MAX + 1)) != UTL_NoError ) {
                vVersion(pucCommandPath);
                iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to get the true index directory path: '%s', utl error: %d", pucNextArgument, iError);
            }

            /* Check that index directory path exists */
            if ( bUtlFilePathExists(pucIndexDirectoryPath) == false ) {
                vVersion(pucCommandPath);
                iUtlLogPanic(UTL_LOG_CONTEXT, "The index directory: '%s', does not exist", pucIndexDirectoryPath);
            }

            /* Check that the index directory path is a directory */
            if ( bUtlFileIsDirectory(pucIndexDirectoryPath) == false ) {
                vVersion(pucCommandPath);
                iUtlLogPanic(UTL_LOG_CONTEXT, "The index directory: '%s', is not a directory", pucIndexDirectoryPath);
            }

            /* Check that the index directory path can be accessed */
            if ( (bUtlFilePathRead(pucIndexDirectoryPath) == false) || (bUtlFilePathExec(pucIndexDirectoryPath) == false) ) {
                vVersion(pucCommandPath);
                iUtlLogPanic(UTL_LOG_CONTEXT, "The index directory: '%s', cannot be accessed", pucIndexDirectoryPath);
            }

            /* Set the index directory path */
            if ( (pssSpiSession->pucIndexDirectoryPath = s_strdup(pucIndexDirectoryPath)) == NULL ) {
                vVersion(pucCommandPath);
                iUtlLogPanic(UTL_LOG_CONTEXT, "Memory allocation failed");
            }
        }

        /* Check for true temporary directory path */
        else if ( s_strncmp("--temporary-directory=", pucNextArgument, s_strlen("--temporary-directory=")) == 0 ) {

            /* Get the temporary directory path */
            pucNextArgument += s_strlen("--temporary-directory=");

            /* Get the temporary directory path */
            if ( (iError = iUtlFileGetTruePath(pucNextArgument, pucTemporaryDirectoryPath, UTL_FILE_PATH_MAX + 1)) != UTL_NoError ) {
                vVersion(pucCommandPath);
                iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to get the true temporary directory path: '%s', utl error: %d", pucNextArgument, iError);
            }

            /* Check that temporary directory path exists */
            if ( bUtlFilePathExists(pucTemporaryDirectoryPath) == false ) {
                vVersion(pucCommandPath);
                iUtlLogPanic(UTL_LOG_CONTEXT, "The temporary directory: '%s', does not exist", pucTemporaryDirectoryPath);
            }

            /* Check that the temporary directory path is a directory */
            if ( bUtlFileIsDirectory(pucTemporaryDirectoryPath) == false ) {
                vVersion(pucCommandPath);
                iUtlLogPanic(UTL_LOG_CONTEXT, "The temporary directory: '%s', is not a directory", pucTemporaryDirectoryPath);
            }

            /* Check that the temporary directory path can be accessed */
            if ( (bUtlFilePathRead(pucTemporaryDirectoryPath) == false) || (bUtlFilePathExec(pucTemporaryDirectoryPath) == false) ) {
                vVersion(pucCommandPath);
                iUtlLogPanic(UTL_LOG_CONTEXT, "The temporary directory: '%s', cannot be accessed", pucTemporaryDirectoryPath);
            }

            /* Set the temporary directory path */
            if ( (pssSpiSession->pucTemporaryDirectoryPath = s_strdup(pucTemporaryDirectoryPath)) == NULL ) {
                vVersion(pucCommandPath);
                iUtlLogPanic(UTL_LOG_CONTEXT, "Memory allocation failed");
            }
        }

        /* Check for user name */
        else if ( s_strncmp("--user=", pucNextArgument, s_strlen("--user=")) == 0 ) {

            struct passwd   pwPasswd;
            unsigned char   pucBuffer[UTL_FILE_PATH_MAX + 1];
            struct passwd   *ppwPasswdPtr = NULL;

            /* Get and check the user name */
            pucNextArgument += s_strlen("--user=");

            /* Check the password entry for this user name by getting it */
            if ( (s_getpwnam_r(pucNextArgument, &pwPasswd, pucBuffer, UTL_FILE_PATH_MAX + 1, &ppwPasswdPtr) == -1) || (ppwPasswdPtr == NULL) ) {
                vVersion(pucCommandPath);
                iUtlLogPanic(UTL_LOG_CONTEXT, "The user: '%s', does not appear to exist", pucNextArgument);
            }

            /* Only root can setuid */
            if ( s_getuid() != 0 ) {
                vVersion(pucCommandPath);
                iUtlLogPanic(UTL_LOG_CONTEXT, "Must be super-user to set the user ID of the server");
            }

            /* Set the user name */
            pucUserName = pucNextArgument;
        }

        /* Check for children count */
        else if ( s_strncmp("--children=", pucNextArgument, s_strlen("--children=")) == 0 ) {

            /* Get the children count */
            pucNextArgument += s_strlen("--children=");

            /* Check the children count */
            if ( s_strtol(pucNextArgument, NULL, 10) <= 0 ) {
                vVersion(pucCommandPath);
                iUtlLogPanic(UTL_LOG_CONTEXT, "Expected the children count to be greater than: 0");
            }
            
            /* Set the children count */
            uiChildrenCount = s_strtol(pucNextArgument, NULL, 10);
            
            /* And fork the server */
            bForkServer = true;
        }

#if defined(SRVR_SERVER_ENABLE_THREADED_SERVER) 

        /* Check for thread count */
        else if ( s_strncmp("--threads=", pucNextArgument, s_strlen("--threads=")) == 0 ) {

            /* Get the thread count */
            pucNextArgument += s_strlen("--threads=");

            /* Check the thread count */
            if ( s_strtol(pucNextArgument, NULL, 10) <= 0 ) {
                vVersion(pucCommandPath);
                iUtlLogPanic(UTL_LOG_CONTEXT, "Expected the thread count to be greater than: 0");
            }
            
            /* Set the thread count */
            uiThreadCount = s_strtol(pucNextArgument, NULL, 10);
            
            /* And thread the server */
            bThreadServer = true;
        }

        /* Check for thread stack size */
        else if ( s_strncmp("--thread-stack-size=", pucNextArgument, s_strlen("--thread-stack-size=")) == 0 ) {

            /* Get the thread stack size */
            pucNextArgument += s_strlen("--thread-stack-size=");

            /* Check the thread count */
            if ( s_strtol(pucNextArgument, NULL, 10) <= 0 ) {
                vVersion(pucCommandPath);
                iUtlLogPanic(UTL_LOG_CONTEXT, "Expected the thread stack size to be greater than: 0");
            }
            
            /* Set the thread stack size */
            uiThreadStackSize = s_strtol(pucNextArgument, NULL, 10);
            
            /* And thread the server */
            bThreadServer = true;
        }

#endif    /* defined(SRVR_SERVER_ENABLE_THREADED_SERVER) */

        /* Check for sessions count */
        else if ( s_strncmp("--sessions=", pucNextArgument, s_strlen("--sessions=")) == 0 ) {

            /* Get the sessions count */
            pucNextArgument += s_strlen("--sessions=");

            /* Check the sessions count */
            if ( s_strtol(pucNextArgument, NULL, 10) < 0 ) {
                vVersion(pucCommandPath);
                iUtlLogPanic(UTL_LOG_CONTEXT, "Expected the sessions count to be equal to or greater than: 0");
            }
            
            /* Set the sessions count */
            psssSrvrServerSession->uiSessionCount = s_strtol(pucNextArgument, NULL, 10);
        }

        /* Check for startup interval */
        else if ( s_strncmp("--startup-interval=", pucNextArgument, s_strlen("--startup-interval=")) == 0 ) {

            /* Get the startup interval */
            pucNextArgument += s_strlen("--startup-interval=");

            /* Check the startup interval */
            if ( s_strtol(pucNextArgument, NULL, 10) <= 0 ) {
                vVersion(pucCommandPath);
                iUtlLogPanic(UTL_LOG_CONTEXT, "Expected the startup interval to be greater than: 0");
            }
            
            /* Set the startup interval */
            uiStartupInterval = s_strtol(pucNextArgument, NULL, 10);
        }

        /* Check for daemon */
        else if ( s_strcmp("--daemon", pucNextArgument) == 0 ) {

            /* Set the daemon flag */
            bDaemonServer = true;
        }

        /* Check for process ID file path */
        else if ( s_strncmp("--process-id-file=", pucNextArgument, s_strlen("--process-id-file=")) == 0 ) {

            /* Get the process ID file path */
            pucNextArgument += s_strlen("--process-id-file=");

            /* Set the process ID file path */
            pucProcessIDFilePath = pucNextArgument;
        }    

        /* Check for check */
        else if ( s_strcmp("--check", pucNextArgument) == 0 ) {

            /* Set the check flag */
            bCheck = true;
        }

        /* Check for load max */
        else if ( s_strncmp("--max-load=", pucNextArgument, s_strlen("--max-load=")) == 0 ) {

            /* Get the load max */
            pucNextArgument += s_strlen("--max-load=");

            /* Check the load max */
            if ( s_strtod(pucNextArgument, NULL) < 0 ) {
                vVersion(pucCommandPath);
                iUtlLogPanic(UTL_LOG_CONTEXT, "Expected the load max to be equal to or greater than: 0");
            }
            
            /* Set the load max */
            psssSrvrServerSession->dLoadMaximum = s_strtod(pucNextArgument, NULL);
        }

        /* Check for connection load max */
        else if ( s_strncmp("--max-connection-load=", pucNextArgument, s_strlen("--max-connection-load=")) == 0 ) {

            /* Get the connection load max */
            pucNextArgument += s_strlen("--max-connection-load=");

            /* Check the connection load max */
            if ( s_strtod(pucNextArgument, NULL) < 0 ) {
                vVersion(pucCommandPath);
                iUtlLogPanic(UTL_LOG_CONTEXT, "Expected the connection load max to be equal to or greater than: 0");
            }
            
            /* Set the connection load max */
            psssSrvrServerSession->dConnectionLoadMaximum = s_strtod(pucNextArgument, NULL);
        }

        /* Check for search load max */
        else if ( s_strncmp("--max-search-load=", pucNextArgument, s_strlen("--max-search-load=")) == 0 ) {

            /* Get the search load max */
            pucNextArgument += s_strlen("--max-search-load=");

            /* Check the search load max */
            if ( s_strtod(pucNextArgument, NULL) < 0 ) {
                vVersion(pucCommandPath);
                iUtlLogPanic(UTL_LOG_CONTEXT, "Expected the search load max to be equal to or greater than: 0");
            }
            
            /* Set the search load max */
            psssSrvrServerSession->dSearchLoadMaximum = s_strtod(pucNextArgument, NULL);
        }

        /* Check for retrieval load max */
        else if ( s_strncmp("--max-retrieval-load=", pucNextArgument, s_strlen("--max-retrieval-load=")) == 0 ) {

            /* Get the retrieval load max */
            pucNextArgument += s_strlen("--max-retrieval-load=");

            /* Check the retrieval load max */
            if ( s_strtod(pucNextArgument, NULL) < 0 ) {
                vVersion(pucCommandPath);
                iUtlLogPanic(UTL_LOG_CONTEXT, "Expected the retrieval load max to be equal to or greater than: 0");
            }
            
            /* Set the retrieval load max */
            psssSrvrServerSession->dRetrievalLoadMaximum = s_strtod(pucNextArgument, NULL);
        }

        /* Check for information load max */
        else if ( s_strncmp("--max-information-load=", pucNextArgument, s_strlen("--max-information-load=")) == 0 ) {

            /* Get the information load max */
            pucNextArgument += s_strlen("--max-information-load=");

            /* Check the information load max */
            if ( s_strtod(pucNextArgument, NULL) < 0 ) {
                vVersion(pucCommandPath);
                iUtlLogPanic(UTL_LOG_CONTEXT, "Expected the information load max to be equal to or greater than: 0");
            }
            
            /* Set the information load max */
            psssSrvrServerSession->dInformationLoadMaximum = s_strtod(pucNextArgument, NULL);
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
                vVersion(pucCommandPath);
                iUtlLogPanic(UTL_LOG_CONTEXT, "Expected the log level to be greater than or equal to: %d, and less than or equal to: %d", UTL_LOG_LEVEL_MINIMUM, UTL_LOG_LEVEL_MAXIMUM);
            }
            
            /* Set the log level */
            uiLogLevel = s_strtol(pucNextArgument, NULL, 10);
        }

        /* Check for help */
        else if ( (s_strcmp("-?", pucNextArgument) == 0) || (s_strcmp("--help", pucNextArgument) == 0) || (s_strcmp("--usage", pucNextArgument) == 0) ) {
            vVersion(pucCommandPath);
            vUsage(pucCommandPath);
            s_exit(EXIT_SUCCESS);
        }

        /* Check for version */
        else if ( (s_strcmp("--version", pucNextArgument) == 0) ) {
            vVersion(pucCommandPath);
            s_exit(EXIT_SUCCESS);
        }

        /* We have run out of options */
        else {
            vVersion(pucCommandPath);
            iUtlLogPanic(UTL_LOG_CONTEXT, "Invalid option: '%s', try '-?', '--help' or '--usage' for more information", pucNextArgument);
        }

        pucNextArgument = pucUtlArgsGetNextArg(&argc, &argv);

    }


    /* Set the log type */
    if ( (iError = iUtlLogSetType((bThreadServer == true) ? UTL_LOG_TYPE_FULL : UTL_LOG_TYPE_PROCESS)) != UTL_NoError ) {
        vVersion(pucCommandPath);
        iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to set the log type, utl error: %d", iError);
    }

    /* Set the log level */
    if ( (iError = iUtlLogSetLevel(uiLogLevel)) != UTL_NoError ) {
        vVersion(pucCommandPath);
        iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to set the log level, utl error: %d", iError);
    }



    /* Install signal handlers */
    if ( (iError = iUtlSignalsInstallNonFatalHandler((void (*)())vSrvrServerNonFatalSignalHandler)) != UTL_NoError ) {
        vVersion(pucCommandPath);
        iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to install non-fatal signal handler, utl error: %d", iError);
    }

    if ( (iError = iUtlSignalsInstallFatalHandler((void (*)())vSrvrServerFatalSignalHandler)) != UTL_NoError ) {
        vVersion(pucCommandPath);
        iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to install fatal signal handler, utl error: %d", iError);
    }

    if ( (iError = iUtlSignalsInstallHangUpHandler(SIG_IGN)) != UTL_NoError ) {
        vVersion(pucCommandPath);
        iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to install hang-up signal handler, utl error: %d", iError);
    }

    if ( (iError = iUtlSignalsInstallChildHandler(SIG_DFL)) != UTL_NoError ) {
        vVersion(pucCommandPath);
        iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to install child signal handler, utl error: %d", iError);
    }



    /* Check and set the locale, we require utf-8 compliance */
    if ( bLngLocationIsLocaleUTF8(LC_ALL, pucLocaleName) != true ) {
        vVersion(pucCommandPath);
        iUtlLogPanic(UTL_LOG_CONTEXT, "The locale: '%s', does not appear to be utf-8 compliant", pucLocaleName);
    }
    if ( (iError = iLngLocationSetLocale(LC_ALL, pucLocaleName)) != LNG_NoError ) {
        vVersion(pucCommandPath);
        iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to set the locale to: '%s', lng error: %d", pucLocaleName, iError);
    }


    /* Set the index directory path if it was not specified */
    if ( bUtlStringsIsStringNULL(pssSpiSession->pucIndexDirectoryPath) == true ) {
        if ( (pssSpiSession->pucIndexDirectoryPath = s_strdup(SRVR_SERVER_INDEX_DIRECTORY_PATH_DEFAULT)) == NULL ) {
            vVersion(pucCommandPath);
            iUtlLogPanic(UTL_LOG_CONTEXT, "Memory allocation failed");
        }
    }

    /* Set the configuration directory path if it was not specified */
    if ( bUtlStringsIsStringNULL(pssSpiSession->pucConfigurationDirectoryPath) == true ) {
        if ( (pssSpiSession->pucIndexDirectoryPath = s_strdup(SRVR_SERVER_CONFIGURATION_DIRECTORY_PATH_DEFAULT)) == NULL ) {
            vVersion(pucCommandPath);
            iUtlLogPanic(UTL_LOG_CONTEXT, "Memory allocation failed");
        }
    }

    /* Set the temporary directory path if it was not specified */
    if ( bUtlStringsIsStringNULL(pssSpiSession->pucTemporaryDirectoryPath) == true ) {
        if ( (pssSpiSession->pucTemporaryDirectoryPath = s_strdup(SRVR_SERVER_TEMPORARY_DIRECTORY_PATH_DEFAULT)) == NULL ) {
            vVersion(pucCommandPath);
            iUtlLogPanic(UTL_LOG_CONTEXT, "Memory allocation failed");
        }
    }


    /* Check that the index directory path is there and can be accessed */
    if ( !((bUtlFileIsDirectory(pssSpiSession->pucIndexDirectoryPath) == true) || 
            (bUtlFilePathRead(pssSpiSession->pucIndexDirectoryPath) == true) || 
            (bUtlFilePathExec(pssSpiSession->pucIndexDirectoryPath) == true)) ) {
        vVersion(pucCommandPath);
        iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to access the index directory path: '%s'", pssSpiSession->pucIndexDirectoryPath);
    }

    /* Check that the configuration directory path is there and can be accessed */
    if ( !((bUtlFileIsDirectory(pssSpiSession->pucConfigurationDirectoryPath) == true) || 
            (bUtlFilePathRead(pssSpiSession->pucConfigurationDirectoryPath) == true) || 
            (bUtlFilePathExec(pssSpiSession->pucConfigurationDirectoryPath) == true)) ) {
        vVersion(pucCommandPath);
        iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to access the configuration directory path: '%s'", pssSpiSession->pucConfigurationDirectoryPath);
    }

    /* Check that the temporary directory path is there and can be accessed */
    if ( !((bUtlFileIsDirectory(pssSpiSession->pucTemporaryDirectoryPath) == true) || 
            (bUtlFilePathRead(pssSpiSession->pucTemporaryDirectoryPath) == true) || 
            (bUtlFilePathWrite(pssSpiSession->pucTemporaryDirectoryPath) == true) || 
            (bUtlFilePathExec(pssSpiSession->pucTemporaryDirectoryPath) == true)) ) {
        vVersion(pucCommandPath);
        iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to access the temporary directory path: '%s'", pssSpiSession->pucTemporaryDirectoryPath);
    }

    
    /* Run the configuration check if all we are required to do is check */
    if ( bCheck == true ) {

        /* Initialize & shutdown the server to make sure that things are ok before we start to fork */
        if ( (iError = iSpiInitializeServer(pssSpiSession)) != SPI_NoError ) {
            vVersion(pucCommandPath);
            iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to initialize the server, spi error: %d", iError);
        }
    
        /* Shutdown the server */
        if ( (iError = iSpiShutdownServer(pssSpiSession)) != SPI_NoError ) {
            vVersion(pucCommandPath);
            iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to shutdown the server, spi error: %d", iError);
        }
        
        return (EXIT_SUCCESS);
    }

    
    /* Set the load maximum from the default for load maximums that were not set */
    if ( psssSrvrServerSession->dConnectionLoadMaximum <= -1 ) {
        psssSrvrServerSession->dConnectionLoadMaximum = psssSrvrServerSession->dLoadMaximum;
    }
    if ( psssSrvrServerSession->dSearchLoadMaximum <= -1 ) {
        psssSrvrServerSession->dSearchLoadMaximum = psssSrvrServerSession->dLoadMaximum;
    }
    if ( psssSrvrServerSession->dRetrievalLoadMaximum <= -1 ) {
        psssSrvrServerSession->dRetrievalLoadMaximum = psssSrvrServerSession->dLoadMaximum;
    }
    if ( psssSrvrServerSession->dInformationLoadMaximum <= -1 ) {
        psssSrvrServerSession->dInformationLoadMaximum = psssSrvrServerSession->dLoadMaximum;
    }



    /* Socket based server, running from the command line or as a daemon */
    if ( bSocketServer == true ) {

        pid_t           zPid = 0;
        unsigned int    uiCurrentChildCount = 0;
        boolean         bLaunchingServer = true;

        /* Fork the server to place it automagically in background mode */
        if ( (bDaemonServer == true) && ((zPid = s_fork()) > 0) ) {
            return (EXIT_SUCCESS);
        }
        else if ( zPid == -1 ) {
            vVersion(pucCommandPath);
            iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to automagically place the server in the background");
        }

        /* Write out the process ID to a file, we do this as the user who started the
        ** server to make sure we can actually write it, since the server is most likely
        ** to be started as 'root' and switched to 'nobody'
        */
        if ( pucProcessIDFilePath != NULL ) {
            
            FILE    *pfFile = NULL;
            
            /* Create a process ID file */
            if ( (pfFile = s_fopen(pucProcessIDFilePath, "w")) == NULL ) {
                vVersion(pucCommandPath);
                iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to create/open the process id file: '%s'", pucProcessIDFilePath);
            }
            
            /* Write out the process ID */
            if ( fprintf(pfFile, "%d\n", (int)s_getpid()) == -1 ) {
                vVersion(pucCommandPath);
                iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to write the process id to the file: '%s'", pucProcessIDFilePath);
            }
            
            /* Close the process ID file */
            s_fclose(pfFile);
        }

        /* Set the user of the server */
        if ( bUtlStringsIsStringNULL(pucUserName) == false ) {
            if ( (iError = iSrvrServerSetUserName(pucUserName)) != SPI_NoError ) {
                iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to set the user ID of the server to: '%s', srvr error: %d", pucUserName, iError);
                vVersion(pucCommandPath);
            }
        }

        /* Create a server service */
        if ( (iError = iUtlNetCreateServerService(psssSrvrServerSession->uiTimeOut, &psssSrvrServerSession->pvUtlNet)) != UTL_NoError ) {
            vVersion(pucCommandPath);
            iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to create a net server service, utl error: %d", iError);
        }

        /* Loop over the server sockets we want to add and add them */
        for ( psssServerSocketsPtr = psssServerSockets, uiI = 0 ; uiI < uiServerSocketsLength; psssServerSocketsPtr++, uiI++ ) {

            /* Add the protocol */
            if ( (iError = iUtlNetAddServerProtocol(psssSrvrServerSession->pvUtlNet, psssServerSocketsPtr->uiProtocol, 
                    pucUtlStringsGetNULLString(psssServerSocketsPtr->pucHostName), psssServerSocketsPtr->iPort)) != UTL_NoError ) {
                vVersion(pucCommandPath);
                iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to add a protocol to the net server service, protocol: %u, host: '%s', port: %d, utl error: %d", 
                        psssServerSocketsPtr->uiProtocol, psssServerSocketsPtr->pucHostName, psssServerSocketsPtr->iPort, iError);
            }
        }

        /* Free the server sockets */
        s_free(psssServerSockets);

        /* Set the log file path, this checks that we can still access the log file as the new user */
        if ( bUtlStringsIsStringNULL(pucLogFilePath) == false ) {
            if ( (iError = iUtlLogSetFilePath(pucLogFilePath)) != UTL_NoError ) {
                iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to set the log file path, utl error: %d", iError);
                vVersion(pucCommandPath);
            }
        }

        /* Log the copyright message, delayed logging */
        vVersion(pucCommandPath);

        /* Log the process ID, delayed logging */
        if ( pucProcessIDFilePath != NULL ) {
            iUtlLogInfo(UTL_LOG_CONTEXT, "Created process id file: '%s'.", pucProcessIDFilePath);
        }
        else {
            iUtlLogInfo(UTL_LOG_CONTEXT, "Process id to terminate server: %d.", (int)s_getpid());
        }

        /* Log the user change, delayed logging */
        if ( bUtlStringsIsStringNULL(pucUserName) == false ) {
            iUtlLogInfo(UTL_LOG_CONTEXT, "Successfully set the user ID of the server to: '%s'.", pucUserName);
        }


        /* Loop until a termination is requested */
        do {

            /* Fork off a child process if required */
            if ( (bForkServer == false) || ((zPid = s_fork()) == 0) ) {

                /* Reset the log line counter to 1 if we create a new server process */
                if ( bForkServer == true ) {
                    iUtlLogSetLine(1);
                }

#if defined(SRVR_SERVER_ENABLE_THREADED_SERVER) 

                /* Threaded server */
                if ( bThreadServer == true ) {

                    pthread_attr_t    taThreadAttributes;

                    /* Initialize the thread attributes structure */
                    pthread_attr_init(&taThreadAttributes); 

                    
                    /* Set the thread stack size */
                    if ( uiThreadStackSize > 0 ) {

                        size_t  zThreadStackSize = 0; 
                        
                        /* Get the current stack size */
                        pthread_attr_getstacksize(&taThreadAttributes, &zThreadStackSize); 

                        /* Set the new stack size */
                        if ( pthread_attr_setstacksize(&taThreadAttributes, uiThreadStackSize * (1024 * 1024)) == 0 ) {
                            iUtlLogInfo(UTL_LOG_CONTEXT, "Successfully set the thread stack size to: %uMB, (default: %uMB).", 
                                    uiThreadStackSize, (unsigned int)((float)zThreadStackSize / (1024 * 1024)));
                        }
                        else {
                            iUtlLogWarn(UTL_LOG_CONTEXT, "Failed to set the thread stack size to: %uMB, (defaulting to: %uMB).", 
                                    uiThreadStackSize, (unsigned int)((float)zThreadStackSize / (1024 * 1024)));
                        }
                    }


                    /* Allocate the spi session structure array, one for each thread */
                    if ( (psssSrvrServerSessions = (struct srvrServerSession *)s_malloc((size_t)(sizeof(struct srvrServerSession) * uiThreadCount))) == NULL ) {
                        iUtlLogPanic(UTL_LOG_CONTEXT, "Memory allocation error");
                    }

                    /* Loop, populating the spi session structure array */
                    for ( psssSrvrServerSessionsPtr = psssSrvrServerSessions, uiI = 0; uiI < uiThreadCount; psssSrvrServerSessionsPtr++, uiI++ ) {

                        /* Copy the server session structure template into the current entry in the server sessions structure array */
                        s_memcpy(psssSrvrServerSessionsPtr, psssSrvrServerSession, sizeof(struct srvrServerSession));

                        /* Duplicate the spi session */
                        if ( (iError = iSpiDuplicateSession(psssSrvrServerSession->pssSpiSession, &psssSrvrServerSessionsPtr->pssSpiSession)) != UTL_NoError ) {
                            iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to duplicate the spi session structure, spi error: %d");
                        }
                        
                        /* Duplicate the net structure */
                        if ( (iError = iUtlNetDuplicate(psssSrvrServerSession->pvUtlNet, &psssSrvrServerSessionsPtr->pvUtlNet)) != UTL_NoError ) {
                            iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to duplicate the net structure, utl error: %d");
                        }
                    }                                

                    /* Set the spi session structure array globals */
                    psssSrvrServerSessionsGlobal = psssSrvrServerSessions;
                    uiSrvrServerSessionsLengthGlobal = uiThreadCount;
                    
                    /* Set the thread ID global, this allows us to distinguish between the parent thread 
                    ** and the child thread when we are running a threaded server
                    */
                    zSrvrServerThreadIDGlobal = s_pthread_self();

                    /* Initialize the thread count global */
                    iSrvrServerThreadCountGlobal = 0;


                    /* Loop until a termination is requested */
                    do {
            
                        /* Lock the mutex, ignore errors */
                        s_pthread_mutex_lock(&mSrvrServerThreadMutexGlobal);

                        /* Create new threads if the thread count global is below the thread count we want to maintain */
                        if ( iSrvrServerThreadCountGlobal < uiThreadCount ) {
                        
                            /* Loop, creating the threads */
                            for ( psssSrvrServerSessionsPtr = psssSrvrServerSessions, uiI = 0; uiI < uiThreadCount; psssSrvrServerSessionsPtr++, uiI++ ) {
                                
                                /* Activate inactive sessions */
                                if ( psssSrvrServerSessionsPtr->bActiveSession == false ) {
    
                                    int         iStatus = 0;
                                    pthread_t   pThread;

                                    /* Log */
                                    if  ( psssSrvrServerSession->uiSessionCount == 0 ) {
                                        iUtlLogInfo(UTL_LOG_CONTEXT, "Created new thread, unlimited number of sessions.");
                                    }
                                    else {
                                        iUtlLogInfo(UTL_LOG_CONTEXT, "Created new thread, limited to %u session%s.", psssSrvrServerSession->uiSessionCount, 
                                                ((psssSrvrServerSession->uiSessionCount == 1) ? "" : "s"));
                                    }

                                    /* The session is now active */
                                    psssSrvrServerSessionsPtr->bActiveSession = true;

                                    /* Kick off the thread to serve the clients */
                                    if ( (iStatus = s_pthread_create(&pThread, &taThreadAttributes, (void *)iSrvrServerServeSocketClient, (void *)psssSrvrServerSessionsPtr)) != 0 ) {
                                        iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to create a thread");
                                    }
    
                                    /* Detach the thread, we don't want to join it */ 
                                    if ( (iStatus = s_pthread_detach(pThread)) != 0 ) {
                                        iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to detach a thread");
                                    }

                                    /* Increment the thread count since we have started a new thread */
                                    iSrvrServerThreadCountGlobal++;
                                }

                                /* Sleep before starting a new thread if we need to, and we are still launching the server */
                                if ( (bLaunchingServer == true) && (uiStartupInterval > 0) ) {
                                    s_usleep(uiStartupInterval * 1000);
                                }
                            }
                    
                            /* We are done launching the server once we reach this point */
                            bLaunchingServer = false;
                        }
                        
                        /* Unlock the mutex, ignore errors */
                        s_pthread_mutex_unlock(&mSrvrServerThreadMutexGlobal);

                        /* We are running all the threads we can, so we sleep before checking if we need to start new ones */ 
                        s_usleep(SRVR_SERVER_THREAD_CHECK_INTERVAL);
                    
                    } while ( bSrvrServerTerminationRequestedGlobal == false );
                }
                
                /* Unthreaded server */
                else {

#endif    /* defined(SRVR_SERVER_ENABLE_THREADED_SERVER) */

                    /* Log */
                    if  ( psssSrvrServerSession->uiSessionCount == 0 ) {
                        iUtlLogInfo(UTL_LOG_CONTEXT, "Created new server, unlimited number of sessions.");
                    }
                    else {
                        iUtlLogInfo(UTL_LOG_CONTEXT, "Created new server, limited to %u session%s.", psssSrvrServerSession->uiSessionCount, 
                                ((psssSrvrServerSession->uiSessionCount == 1) ? "" : "s"));
                    }

                    /* The session is now active */
                    psssSrvrServerSession->bActiveSession = true;

                    /* Serve the clients, setting up the socket */
                    iError = iSrvrServerServeSocketClient(psssSrvrServerSession);

#if defined(SRVR_SERVER_ENABLE_THREADED_SERVER) 
                }
#endif    /* defined(SRVR_SERVER_ENABLE_THREADED_SERVER) */

                /* Bail here if this is a child, otherwise we break */
                if ( (bForkServer == true) && (zPid == 0) ) {
/*                     iUtlLogDebug(UTL_LOG_CONTEXT, "child exit... (psssSrvrServerSession %s NULL) (psssSrvrServerSessions %s NULL)",  */
/*                             ((psssSrvrServerSession == NULL) ? "==" : "!="), ((psssSrvrServerSessions == NULL) ? "==" : "!=")); */
                    goto bailFromMain;
                }
                else if ( bForkServer == false ) {
                    break;
                }
            }
            else {
            
                int     iStatus = 0;

                /* Check that the fork actually happened */
                if ( (bForkServer == true) && (zPid == -1) ) {
                    iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to fork a new server");
                }

                /* Increment the current child count */
                uiCurrentChildCount++;

/*                 iUtlLogDebug(UTL_LOG_CONTEXT, "uiCurrentChildCount: %u, uiChildrenCount: %u.", uiCurrentChildCount, uiChildrenCount); */

                /* Wait for kids to return if we have reached the maximum child count */
                if ( uiCurrentChildCount >= uiChildrenCount ) {
                
                    /* We are done launching the server if we reach the maximum child count */
                    bLaunchingServer = false;

                    /* Wait for a kid to return */
                    zPid = s_wait(&iStatus);
                    iUtlLogInfo(UTL_LOG_CONTEXT, "Collected child: %u.", (unsigned int)zPid);

                    /* A child as returned, so we decrement the current number of children */
                    uiCurrentChildCount--;

/*                     iUtlLogDebug(UTL_LOG_CONTEXT, "Collected child: %u, uiCurrentChildCount: %u.", (unsigned int)zPid, uiCurrentChildCount); */
                }
            }

            /* Sleep before starting a new child if we need to, and we are still launching the server */
            if ( (bForkServer == true) && (bLaunchingServer == true) && (uiStartupInterval > 0) ) {

#if defined(SRVR_SERVER_ENABLE_THREADED_SERVER) 

                /* Extended sleep if we are a threaded server */
                if ( bThreadServer == true ) {
                    s_usleep(uiStartupInterval * 1000 * uiThreadCount);
                }
                else {

#endif    /* defined(SRVR_SERVER_ENABLE_THREADED_SERVER) */

                    s_usleep(uiStartupInterval * 1000);

#if defined(SRVR_SERVER_ENABLE_THREADED_SERVER) 
                }
#endif    /* defined(SRVR_SERVER_ENABLE_THREADED_SERVER) */
            }

        } while ( bSrvrServerTerminationRequestedGlobal == false );


        /* Delete the process ID file */
        if ( pucProcessIDFilePath != NULL ) {
            
            /* Delete and log the outcome */
            if ( s_remove(pucProcessIDFilePath) == 0 ) {
                iUtlLogInfo(UTL_LOG_CONTEXT, "Deleted process id file: '%s'.", pucProcessIDFilePath);
            }
            else {
                iUtlLogWarn(UTL_LOG_CONTEXT, "Failed to delete process id file: '%s'.", pucProcessIDFilePath);
            }
        }
    }

    /* Not socket based, so we are using stdio, either running off inetd or off the command line */
    else if ( bSocketServer == false ) { 

        /* Set the user of the server */
        if ( bUtlStringsIsStringNULL(pucUserName) == false ) {
            if ( (iError = iSrvrServerSetUserName(pucUserName)) != SPI_NoError ) {
                vVersion(pucCommandPath);
                iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to set the user ID of the server to: '%s', srvr error: %d", pucUserName, iError);
            }
        }

        /* Create an stdio server */
        if ( (iError = iUtlNetCreateServer(UTL_NET_PROTOCOL_STDIO_ID, NULL, 0, psssSrvrServerSession->uiTimeOut, &psssSrvrServerSession->pvUtlNet)) != UTL_NoError ) {
            vVersion(pucCommandPath);
            iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to create an net 'stdio' server, utl error: %d", iError);
        }

        /* Initialize the server */
        if ( (iError = iSpiInitializeServer(psssSrvrServerSession->pssSpiSession)) != SPI_NoError ) {
            vVersion(pucCommandPath);
            iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to initialize the server, spi error: %d", iError);
        }
    
        /* Set the log file path, this checks that we can still access the log file as the new user */
        if ( bUtlStringsIsStringNULL(pucLogFilePath) == false ) {
            if ( (iError = iUtlLogSetFilePath(pucLogFilePath)) != UTL_NoError ) {
                vVersion(pucCommandPath);
                iUtlLogPanic(UTL_LOG_CONTEXT, "Failed to set the log file path, utl error: %d", iError);
            }
        }

        /* Log the copyright message, delayed logging */
        vVersion(pucCommandPath);

        /* Log the user change, delayed logging */
        if ( bUtlStringsIsStringNULL(pucUserName) == false ) {
            iUtlLogInfo(UTL_LOG_CONTEXT, "Successfully set the user ID of the server to: '%s'.", pucUserName);
        }

        /* This session is now active */
        psssSrvrServerSession->bActiveSession = true;

        /* Serve the client, connections on stdio don't use sockets, so we don't need to set them up */
         iSrvrServerServeClient(psssSrvrServerSession);

        /* Shutdown the server */
        if ( (iError = iSpiShutdownServer(psssSrvrServerSession->pssSpiSession)) != SPI_NoError ) {
            iUtlLogWarn(UTL_LOG_CONTEXT, "Failed to shutdown the server, spi error: %d.", iError);
        }
    }

        

    
    /* Bail label */
    bailFromMain:


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "Exiting... (psssSrvrServerSession %s NULL) (psssSrvrServerSessions %s NULL)",  */
/*             ((psssSrvrServerSession == NULL) ? "==" : "!="), ((psssSrvrServerSessions == NULL) ? "==" : "!=")); */


    /* Clean up the server session if it had not already been freed */
    if ( psssSrvrServerSession != NULL ) {

        /* Clear the server sessions global so that we don't do a double free if a signal is generated */
        if ( psssSrvrServerSessionsGlobal == psssSrvrServerSession ) {
            psssSrvrServerSessionsGlobal = NULL;
            uiSrvrServerSessionsLengthGlobal = 0;
        }

        /* Free the spi session structure */
        iSpiFreeSession(psssSrvrServerSession->pssSpiSession);
        psssSrvrServerSession->pssSpiSession = NULL;

        /* Free the net structure */
        iUtlNetFree(psssSrvrServerSession->pvUtlNet);
        psssSrvrServerSession->pvUtlNet = NULL;

        /* Free the server session pointer */
        s_free(psssSrvrServerSession);
    }


    /* Clean up the server sessions array if it had not already been freed */
    if ( psssSrvrServerSessions != NULL ) {
    
        /* Clear the server sessions global so that we don't do a double free if a signal is generated */
        if ( psssSrvrServerSessionsGlobal == psssSrvrServerSessions ) {
            psssSrvrServerSessionsGlobal = NULL;
            uiSrvrServerSessionsLengthGlobal = 0;
        }

        /* Loop over each server session structure in the server sessions array */
        for ( psssSrvrServerSessionsPtr = psssSrvrServerSessions, uiI = 0; uiI < uiSrvrServerSessionsLengthGlobal; psssSrvrServerSessionsPtr++, uiI++ ) {
        
            /* Shutdown the server */
            if ( (iError = iSpiShutdownServer(psssSrvrServerSessionsPtr->pssSpiSession)) != SPI_NoError ) {
                iUtlLogError(UTL_LOG_CONTEXT, "Failed to shutdown the server, spi error: %d.", iError);
            }
                    
            /* Free the spi session structure */
            iSpiFreeSession(psssSrvrServerSessionsPtr->pssSpiSession);
            psssSrvrServerSessionsPtr->pssSpiSession = NULL;

            /* Free the net structure */
            iUtlNetFree(psssSrvrServerSessionsPtr->pvUtlNet);
            psssSrvrServerSessionsPtr->pvUtlNet = NULL;
        }
    
        /* Free the server sessions pointer */
        s_free(psssSrvrServerSessions);
    }


    return (EXIT_SUCCESS);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   vVersion()

    Purpose:    This function list out the version message.

    Parameters: pucCommandPath  command path

    Globals:    none

    Returns:    void

*/
static void vVersion
(
    unsigned char *pucCommandPath
)
{

    unsigned char   *pucCommandNamePtr = NULL;
    unsigned char   pucVersionString[UTL_VERSION_STRING_LENGTH + 1] = {'\0'};
    unsigned char   pucTokenizerFeaturesString[LNG_TOKENIZER_FEATURES_STRING_LENGTH + 1] = {'\0'};


    ASSERT(bUtlStringsIsStringNULL(pucCommandPath) == false);
    

    /* Get the command name */
    if ( iUtlFileGetPathBase(pucCommandPath, &pucCommandNamePtr) != UTL_NoError ) {
        pucCommandNamePtr = pucCommandPath;
    }


    /* Version message */
    if ( s_strcmp(pucCommandNamePtr, SRVR_SERVER_MPSSERVER_COMMAND_NAME) == 0 ) {
        iUtlLogInfo(UTL_LOG_CONTEXT, "MPS Information Server, %s", UTL_VERSION_COPYRIGHT_STRING);
    }
    else if ( s_strcmp(pucCommandNamePtr, SRVR_SERVER_MPSGATEWAY_COMMAND_NAME) == 0 ) {
        iUtlLogInfo(UTL_LOG_CONTEXT, "MPS Gateway Server, %s", UTL_VERSION_COPYRIGHT_STRING);
    }
    else {
        iUtlLogInfo(UTL_LOG_CONTEXT, "MPS Unknown Server, %s", UTL_VERSION_COPYRIGHT_STRING);
    }


    /* Get the version string */
    iUtlVersionGetVersionString(pucVersionString, UTL_VERSION_STRING_LENGTH + 1);

    /* Version message */
    iUtlLogInfo(UTL_LOG_CONTEXT, pucVersionString);


    /* Get the tokenizer features string */
    iLngTokenizerGetFeaturesString(pucTokenizerFeaturesString, LNG_TOKENIZER_FEATURES_STRING_LENGTH + 1);

    /* Tokenizer features message */
    iUtlLogInfo(UTL_LOG_CONTEXT, pucTokenizerFeaturesString);


    return;

}


/*---------------------------------------------------------------------------*/


/*

    Function:   vUsage()

    Purpose:    This function list out all the parameters that the mpsserver
                takes.

    Parameters: pucCommandPath  command path

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
    printf("                  Socket to listen on, using a protocol, host and port combination, \n");
    printf("                  protocol defaults to: '%s', procotols available: '%s', '%s', \n", 
            SRVR_SERVER_NET_PROTOCOL_NAME_DEFAULT, UTL_NET_PROTOCOL_TCP_NAME, UTL_NET_PROTOCOL_UDP_NAME);
    printf("                  host defaults to all addresses, no defaults for port. \n");
    printf("                  This parameter can be used multiple times to specify multiple sockets. \n");
    printf("                  Using this parameter tells the server to use sockets communicate \n");
    printf("                  with clients rather than using 'stdio' ('inetd' mode). \n");
    printf("  --timeout=#     Set the timeout, defaults to: %d, 0 means no timeout. \n", SRVR_SERVER_TIMEOUT_DEFAULT);
    printf("  --configuration-directory=name \n");
    printf("                  Configuration directory, defaults to '%s'.\n", SRVR_SERVER_CONFIGURATION_DIRECTORY_PATH_DEFAULT);

    if ( s_strcmp(pucCommandNamePtr, SRVR_SERVER_MPSSERVER_COMMAND_NAME) == 0 ) {
        printf("  --index-directory=name \n");
        printf("                  Index directory, defaults to '%s'.\n", SRVR_SERVER_INDEX_DIRECTORY_PATH_DEFAULT);
    }
    else if ( s_strcmp(pucCommandNamePtr, SRVR_SERVER_MPSGATEWAY_COMMAND_NAME) == 0 ) {
        ;
    }
    else {
        ;
    }

    printf("  --temporary-directory=name \n");
    printf("                  Temporary directory, defaults to '%s'.\n", SRVR_SERVER_TEMPORARY_DIRECTORY_PATH_DEFAULT);
    printf("  --user=name     If started as root, setuid to user after startup. \n");
    printf("  --children=#    Set the child processes count, defaults is not to fork any child processes. \n");
    printf("                  (this parameter has no effect when the server is running from the internet daemon.) \n");
#if defined(SRVR_SERVER_ENABLE_THREADED_SERVER) 
    printf("  --threads=#     Set the thread count, default is not to create any threads. \n");
    printf("                  (this parameter has no effect when the server is running from the internet daemon.) \n");
    printf("  --thread-stack-size=#\n");
    printf("                  Set the thread stack size in megabytes, defaults to whatever is set by the operating system. \n");
    printf("                  (this parameter has no effect when the server is running from the internet daemon.) \n");
#endif    /* defined(SRVR_SERVER_ENABLE_THREADED_SERVER) */
    printf("  --sessions=#    Set the sessions count, defaults to %d, 0 mean no maximum, \n", SRVR_SERVER_SESSION_COUNT_DEFAULT);
    printf("                  (this parameter has no effect when the server is running from the internet daemon.) \n");
    printf("  --startup-interval=#\n");
    printf("                  Startup interval between forking new child processes to prevent overwhelming services, \n");
    printf("                  defaults to %d, 0 mean no interval, set in milliseconds \n", SRVR_SERVER_STARTUP_INTERVAL_DEFAULT);
    printf("                  (this parameter has no effect when the server is running from the internet daemon.) \n");
    printf("  --daemon        Run the server as a daemon process (detach from the console.) \n");
    printf("  --process-id-file=name\n");
    printf("                  Store the process ID to this file, \n");
    printf("                  (this parameter has no effect when the server is running from the internet daemon.) \n");
    printf("  --check         Check the parameters and configuration, and exit. \n");
    printf("\n");

    printf(" Load parameters: \n");
    printf("  --max-load=#    Set the default maximum load, defaults to: %.2f, 0 means no maximum. \n", SRVR_SERVER_LOAD_MAXIMUM_DEFAULT);
    printf("  --max-connection-load=# \n");
    printf("                  Set the maximum connection load, defaults to: %.2f, 0 means no maximum. \n", SRVR_SERVER_LOAD_MAXIMUM_DEFAULT);
    printf("  --max-search-load=# \n");
    printf("                  Set the maximum search load, defaults to: %.2f, 0 means no maximum. \n", SRVR_SERVER_LOAD_MAXIMUM_DEFAULT);
    printf("  --max-retrieval-load=# \n");
    printf("                  Set the maximum retrieval load, defaults to: %.2f, 0 means no maximum. \n", SRVR_SERVER_LOAD_MAXIMUM_DEFAULT);
    printf("  --max-information-load=# \n");
    printf("                  Set the maximum information load, defaults to: %.2f, 0 means no maximum. \n", SRVR_SERVER_LOAD_MAXIMUM_DEFAULT);
    printf("\n");

    printf(" Locale parameter: \n");
    printf("  --locale=name   Locale name, defaults to '%s', see 'locale' for list of \n", SRCH_SERVER_LOCALE_NAME_DEFAULT);
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

    Function:   vSrvrServerNonFatalSignalHandler()

    Purpose:    To handle non-fatal signals, we shut down the server and 
                bail.

    Parameters: iSignal     signal

    Globals:    none

    Returns:    void

*/
static void vSrvrServerNonFatalSignalHandler
(
    int iSignal
)
{

    int                         iError = UTL_NoError;
    struct srvrServerSession    *psssSrvrServerSessionsPtr = NULL;
    unsigned int                uiI = 0;


    ASSERT(iSignal >= 0);

#if defined(SRVR_SERVER_ENABLE_THREADED_SERVER) 

    /* Handle the signal if we are not running in a threaded environment 
    ** or we are the parent thread in a threaded environment
    */
    if ( (zSrvrServerThreadIDGlobal == -1) || (zSrvrServerThreadIDGlobal == s_pthread_self()) ) {

#endif    /* defined(SRVR_SERVER_ENABLE_THREADED_SERVER) */

        /* We intercept the SIGTERM signal if the session is still active, we log the event and 
        ** set the flag telling us that a termination was requested and return 
        */
        if ( iSignal == SIGTERM ) {
        
            /* Check the server sessions array */
            if ( psssSrvrServerSessionsGlobal != NULL ) {
        
                /* Loop over each server session structure in the array */
                for ( psssSrvrServerSessionsPtr = psssSrvrServerSessionsGlobal, uiI = 0; uiI < uiSrvrServerSessionsLengthGlobal; psssSrvrServerSessionsPtr++, uiI++ ) {

                    boolean     bConnected = false;
            
                    /* Get the connection status */
                    if ( (iError = iUtlNetGetConnectionStatus(psssSrvrServerSessionsPtr->pvUtlNet, &bConnected)) != UTL_NoError ) {
                        iUtlLogWarn(UTL_LOG_CONTEXT, "Failed to get the net connection status, utl error: %d.", iError);
                    }
                    else {
                        if ( bConnected == true ) {
                            iUtlLogInfo(UTL_LOG_CONTEXT, "Received a termination signal, will terminate when finished handling the current client.");
                            bSrvrServerTerminationRequestedGlobal = true;
                            return;
                        }
                    }
                }
            }
        }


        /* Clean up the server sessions array */
        if ( psssSrvrServerSessionsGlobal != NULL ) {
            
            /* Loop over each server session structure in the array */
            for ( psssSrvrServerSessionsPtr = psssSrvrServerSessionsGlobal, uiI = 0; uiI < uiSrvrServerSessionsLengthGlobal; psssSrvrServerSessionsPtr++, uiI++ ) {

                /* Shutdown the server */
                if ( (iError = iSpiShutdownServer(psssSrvrServerSessionsPtr->pssSpiSession)) != SPI_NoError ) {
                    iUtlLogWarn(UTL_LOG_CONTEXT, "Failed to shutdown the server, spi error: %d.", iError);
                }
                
                /* Free the spi session structure */
                iSpiFreeSession(psssSrvrServerSessionsPtr->pssSpiSession);
                psssSrvrServerSessionsPtr->pssSpiSession = NULL;
            
                /* Free the net structure */
                iUtlNetFree(psssSrvrServerSessionsPtr->pvUtlNet);
                psssSrvrServerSessionsPtr->pvUtlNet = NULL;
            }
    
            /* Free the server session structure global */
            s_free(psssSrvrServerSessionsGlobal);
        }

        /* Pass the call onto the normal signal handler */
        vUtlSignalsNonFatalHandler(iSignal);

#if defined(SRVR_SERVER_ENABLE_THREADED_SERVER) 
    }
#endif    /* defined(SRVR_SERVER_ENABLE_THREADED_SERVER) */


    return;

}


/*---------------------------------------------------------------------------*/


/*

    Function:   vSrvrServerFatalSignalHandler()

    Purpose:    To handle fatal signals, we shut down the server and 
                bail.

    Parameters: iSignal     signal

    Globals:    none

    Returns:    void

*/
static void vSrvrServerFatalSignalHandler
(
    int iSignal
)
{

    int                         iError = SPI_NoError;
    struct srvrServerSession    *psssSrvrServerSessionsPtr = NULL;
    unsigned int                uiI = 0;


    ASSERT(iSignal >= 0);


#if defined(SRVR_SERVER_ENABLE_THREADED_SERVER) 

    /* Handle the signal if we are not running in a threaded environment 
    ** or we are the parent thread in a threaded environment
    */
    if ( (zSrvrServerThreadIDGlobal == -1) || (zSrvrServerThreadIDGlobal == s_pthread_self()) ) {

#endif    /* defined(SRVR_SERVER_ENABLE_THREADED_SERVER) */

        /* Clean up the server sessions array */
        if ( psssSrvrServerSessionsGlobal != NULL ) {
            
            /* Loop over each server session structure in the array */
            for ( psssSrvrServerSessionsPtr = psssSrvrServerSessionsGlobal, uiI = 0; uiI < uiSrvrServerSessionsLengthGlobal; psssSrvrServerSessionsPtr++, uiI++ ) {
                
                /* Shutdown the server */
                if ( (iError = iSpiShutdownServer(psssSrvrServerSessionsPtr->pssSpiSession)) != SPI_NoError ) {
                    iUtlLogWarn(UTL_LOG_CONTEXT, "Failed to shutdown the server, spi error: %d.", iError);
                }

                /* Free the spi session structure */
                iSpiFreeSession(psssSrvrServerSessionsPtr->pssSpiSession);
                psssSrvrServerSessionsPtr->pssSpiSession = NULL;

                /* Free the net structure */
                iUtlNetFree(psssSrvrServerSessionsPtr->pvUtlNet);
                psssSrvrServerSessionsPtr->pvUtlNet = NULL;
            }

            /* Free the server session structure global */
            s_free(psssSrvrServerSessionsGlobal);
        }

        /* Pass the call onto the normal signal handler */
        vUtlSignalsFatalHandler(iSignal);

#if defined(SRVR_SERVER_ENABLE_THREADED_SERVER) 
    }
#endif    /* defined(SRVR_SERVER_ENABLE_THREADED_SERVER) */


    return;

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrvrServerSetUserName()

    Purpose:    This function sets the user name of the server and the
                owner of the log file.

    Parameters: pucUserName     user name

    Globals:    none

    Returns:    SPI error code

*/
static int iSrvrServerSetUserName
(
    unsigned char *pucUserName
)
{

    int             iError = UTL_NoError;
    struct passwd   pwPasswd;
    unsigned char   pucBuffer[UTL_FILE_PATH_MAX + 1];
    struct passwd   *ppwPasswdPtr = NULL;


    ASSERT(bUtlStringsIsStringNULL(pucUserName) == false);


    /* Only root can setuid */
    if ( s_getuid() != 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to set the owner to: '%s', must be super-user.", pucUserName);
          return (SPI_MiscError);
    }

    /* Get the password entry for this user */
    if ( s_getpwnam_r(pucUserName, &pwPasswd, pucBuffer, UTL_FILE_PATH_MAX + 1, &ppwPasswdPtr) == -1 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the password information for the user: '%s'.", pucUserName);
          return (SPI_MiscError);
    }

    if ( ppwPasswdPtr == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "The user: '%s', does not appear to exist.", pucUserName);
          return (SPI_MiscError);
    }

    /* Set the log file owner */
    if ( (iError = iUtlLogSetOwnerName(pucUserName)) != UTL_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to set the log owner to: '%s', utl error: %d.", pucUserName, iError);
          return (SPI_MiscError);
    }

    /* Set the user ID */
    if ( s_setuid(ppwPasswdPtr->pw_uid) < 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to set user ID of the server to that of the user: '%s'.", pucUserName);
          return (SPI_MiscError);
    }


      return (SPI_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrvrServerServeSocketClient()

    Purpose:    This function serves as a wrapper around iSrvrServerServeClient(),
                doing two things: first it handles the socket setup and teardown,
                second it will loop handling sessions as required by the 
                uiSessionCount specified in the server session structure.

    Parameters: psssSrvrServerSession       Server session

    Globals:    none

    Returns:    SPI error code

*/
static int iSrvrServerServeSocketClient
(
    struct srvrServerSession *psssSrvrServerSession
)
{

    int             iError = SPI_NoError;
    unsigned int    uiCurrentSessionCount = 0;


    ASSERT(psssSrvrServerSession != NULL);


    /* Initialize the server */
    if ( (iError = iSpiInitializeServer(psssSrvrServerSession->pssSpiSession)) != SPI_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to initialize the server, spi error: %d.", iError);
        goto bailFromiSrvrServerServeSocketClients;
    }


    /* Set the timeout */
    if ( (iError = iUtlNetSetTimeOut(psssSrvrServerSession->pvUtlNet, psssSrvrServerSession->uiTimeOut)) != UTL_NoError ) {
        iUtlLogWarn(UTL_LOG_CONTEXT, "Failed to set the net timeout, utl error: %d.", iError);
        iError = SPI_NoError;
    }


    /* Accept client connections while we are still under the maximum session count 
    ** if it is set, or forever if there is no maximum session count
    */
    while ( (uiCurrentSessionCount < psssSrvrServerSession->uiSessionCount) || (psssSrvrServerSession->uiSessionCount == 0) ) {

#if defined(SRVR_SERVER_ENABLE_ACCEPT_TIMEOUT_OVERRIDE)
        /* Temporarily override the timeout just for the accept */
        if ( (iError = iUtlNetSetTimeOut(psssSrvrServerSession->pvUtlNet, SRVR_SERVER_ACCEPT_TIMEOUT_OVERRIDE)) != UTL_NoError ) {
            iUtlLogWarn(UTL_LOG_CONTEXT, "Failed to set the net timeout, utl error: %d.", iError);
            iError = SPI_NoError;
        }
#endif    /* defined(SRVR_SERVER_ENABLE_ACCEPT_TIMEOUT_OVERRIDE) */

        /* Accept a client connection and receive the message */
        if ( (iError = iUtlNetReceive(psssSrvrServerSession->pvUtlNet)) != UTL_NoError ) {

            /* Timeout waiting to receive data */
            if ( iError == UTL_NetTimeOut ) {
                iUtlLogWarn(UTL_LOG_CONTEXT, "The socket timed out while receiving data, utl error: %d", iError);
            }
            /* Socket closed on us */
            else if ( iError == UTL_NetSocketClosed ) {
                ;
            }
            /* Report bad errors */
            else {
                iUtlLogError(UTL_LOG_CONTEXT, "Failed to accept a connection or receive a message from a client, utl error: %d", iError);
            }
        }
        else {

#if defined(SRVR_SERVER_ENABLE_ACCEPT_TIMEOUT_OVERRIDE)
            /* Restore the timeout */
            if ( (iError = iUtlNetSetTimeOut(psssSrvrServerSession->pvUtlNet, psssSrvrServerSession->uiTimeOut)) != UTL_NoError ) {
                iUtlLogWarn(UTL_LOG_CONTEXT, "Failed to set the net timeout, utl error: %d.", iError);
                iError = SPI_NoError;
            }
#endif    /* defined(SRVR_SERVER_ENABLE_ACCEPT_TIMEOUT_OVERRIDE) */

            /* Serve the client */
            iSrvrServerServeClient(psssSrvrServerSession);
        }
    

        /* Close the client connection */
        iUtlNetCloseClient(psssSrvrServerSession->pvUtlNet);

        /* Check to see if a termination was requested, break out of this loop if it was */
        if ( bSrvrServerTerminationRequestedGlobal == true ) {
            iUtlLogInfo(UTL_LOG_CONTEXT, "Finished handling client, proceeding with termination.");
            break;
        }

        /* Increment the current session count */
        if ( psssSrvrServerSession->uiSessionCount > 0 ) {
            uiCurrentSessionCount++;
        }
    }


    /* Log */
    if ( psssSrvrServerSession->uiSessionCount > 0 ) {
        iUtlLogInfo(UTL_LOG_CONTEXT, "Shutting down server, %u session%s handled.", uiCurrentSessionCount, ((uiCurrentSessionCount == 1) ? "" : "s"));
    }



    /* Bail label */
    bailFromiSrvrServerServeSocketClients:


    /* Shutdown the server */
    if ( (iError = iSpiShutdownServer(psssSrvrServerSession->pssSpiSession)) != SPI_NoError ) {
        iUtlLogWarn(UTL_LOG_CONTEXT, "Failed to shutdown the server, spi error: %d.", iError);
    }


#if defined(SRVR_SERVER_ENABLE_THREADED_SERVER) 

    /* Decrement the thread count if this is a threaded server, and inactivate the session in either case */
    if ( iSrvrServerThreadCountGlobal != -1 ) {

        /* Lock the mutex, ignore errors */
        s_pthread_mutex_lock(&mSrvrServerThreadMutexGlobal);

        /* Decrement the thread count */
        iSrvrServerThreadCountGlobal--;

        /* This session is now inactive */
        psssSrvrServerSession->bActiveSession = false;

        /* Unlock the mutex, ignore errors */
        s_pthread_mutex_unlock(&mSrvrServerThreadMutexGlobal);
    }
    else {

#endif    /* defined(SRVR_SERVER_ENABLE_THREADED_SERVER) */

        /* This session is now inactive */
        psssSrvrServerSession->bActiveSession = false;

#if defined(SRVR_SERVER_ENABLE_THREADED_SERVER) 
    }
#endif    /* defined(SRVR_SERVER_ENABLE_THREADED_SERVER) */


    return (SPI_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrvrServerServeClient()

    Purpose:    This function will read the first byte in the message a client 
                sends us and decide which protocol is being sent. It will then
                hand over to the appropriate protocol handler. The handler will
                then take appropriate action based on the message. The structure
                of a handler is roughtly as follows:

                    - read the message (perhaps also using a message length)
                    - decode the message
                    - perform any actions required
                    - code up a reply
                    - send the reply to the client
                    - loop back to step 1 if needed

    Parameters: psssSrvrServerSession   Server session

    Globals:    none

    Returns:    SPI error code

*/
static int iSrvrServerServeClient
(
    struct srvrServerSession *psssSrvrServerSession
)
{

    int                                 iError = SPI_NoError;
#if defined(SRVR_SERVER_ENABLE_CLIENT_HOSTNAME_LOOKUP)
    unsigned char                       pucClientName[MAXHOSTNAMELEN + 1] = {'\0'};
#endif
    unsigned char                       pucClientAddress[MAXHOSTNAMELEN + 1] = {'\0'};
    unsigned char                       *pucBufferPtr = NULL;
    struct srvrServerProtocolHandler    *psphSrvrProtocolHandler = pssphSrvrServerProtocolHandlersGlobal;
    

    ASSERT(psssSrvrServerSession != NULL);


    /* Log where the connection is coming from  */
#if defined(SRVR_SERVER_ENABLE_CLIENT_HOSTNAME_LOOKUP)

    /* Get the client name and address for this connection */
    iUtlNetGetConnectedClientName(psssSrvrServerSession->pvUtlNet, pucClientName, MAXHOSTNAMELEN + 1);
    iUtlNetGetConnectedClientAddress(psssSrvrServerSession->pvUtlNet, pucClientAddress, MAXHOSTNAMELEN + 1);

    if ( (bUtlStringsIsStringNULL(pucClientName) == false) && (s_strcmp(pucClientName, pucClientAddress) != 0) ) {
        iUtlLogInfo(UTL_LOG_CONTEXT, "Accepted connection from a client on: '%s' [%s].", pucClientName, pucClientAddress);
    }
    else if ( bUtlStringsIsStringNULL(pucClientAddress) == false ) {
        iUtlLogInfo(UTL_LOG_CONTEXT, "Accepted connection from a client on: [%s].", pucClientAddress);
    }
    else {
        iUtlLogWarn(UTL_LOG_CONTEXT, "Failed to determine the client address.");
    }

#else     /* defined(SRVR_SERVER_ENABLE_CLIENT_HOSTNAME_LOOKUP) */

    /* Get the client address for this connection */
    iUtlNetGetConnectedClientAddress(psssSrvrServerSession->pvUtlNet, pucClientAddress, MAXHOSTNAMELEN + 1);

    if ( bUtlStringsIsStringNULL(pucClientAddress) == false ) {
        iUtlLogInfo(UTL_LOG_CONTEXT, "Accepted connection from a client on: [%s].", pucClientAddress);
    }
    else {
        iUtlLogWarn(UTL_LOG_CONTEXT, "Failed to determine the client address.");
    }

#endif    /* defined(SRVR_SERVER_ENABLE_CLIENT_HOSTNAME_LOOKUP) */




    /* Peek into the network connection for single byte with which to recognize the protocol */
    if ( (iError = iUtlNetPeek(psssSrvrServerSession->pvUtlNet, 1, &pucBufferPtr)) != UTL_NoError ) {

        /* Timeout waiting to receive data */
        if ( iError == UTL_NetTimeOut ) {
            iUtlLogError(UTL_LOG_CONTEXT, "The socket timed out while peeking data.");
        }
        /* Report bad errors */
        else if ( iError != UTL_NetSocketClosed ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to receive a message from a client, utl error: %d.", iError);
        }

        iError = SPI_MiscError;
        goto bailFromiSrvrServerServeClient;
    }


    /* Check the header byte we have read against the known protocol 
    ** header bytes and decided which protocol this message is for
    */
    for ( psphSrvrProtocolHandler = pssphSrvrServerProtocolHandlersGlobal; psphSrvrProtocolHandler->iSrvrProtocolHandler != NULL; 
            psphSrvrProtocolHandler++ ) {

        /* Check the protocol frist byte, if we have a match, we set the protocol ID 
        ** and break out, note that this leaves the psphSrvrProtocolHandler pointer 
        ** pointing to the protocol identified 
        */
        if ( pucBufferPtr[0] == psphSrvrProtocolHandler->ucProtocolHeaderByte ) {
            break;
        }
    }


    /* Call the appropriate protocol handler function */
    if ( psphSrvrProtocolHandler->iSrvrProtocolHandler != NULL ) {

#if defined(SRVR_SERVER_ENABLE_PROTOCOL_LOGGING)
        iUtlLogDebug(UTL_LOG_CONTEXT, "%s protocol selected.", psphSrvrProtocolHandler->pucProtocolName);
#endif

        /* Call the protocol handler */
        iError = psphSrvrProtocolHandler->iSrvrProtocolHandler(psssSrvrServerSession);
    }
    else {

        /* Could not identify the protocol */
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid protocol - header byte: '%c', value: %d.", pucBufferPtr[0], (int)pucBufferPtr[0]);
        
        iError = SPI_MiscError;
    }



    /* Bail label */
    bailFromiSrvrServerServeClient:


    /* Log the fact that we are done, except if termination was requested in which
    ** case this message gets logged elsewhere
    */
    if ( bSrvrServerTerminationRequestedGlobal == false ) {
        iUtlLogInfo(UTL_LOG_CONTEXT, "Finished handling client.");
    }


    return (iError);

}


/*---------------------------------------------------------------------------*/
