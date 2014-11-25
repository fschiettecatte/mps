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

    Module:     socket.c

    Author:     Francois Schiettecatte (FS Consulting LLC.)

    Created:    16 February 1994

    Purpose:    This module implements all the sockets code which is 
                used for communications.

*/


/*---------------------------------------------------------------------------*/


/*
** Includes
*/

#include "utils.h"


/*---------------------------------------------------------------------------*/


/*
** Externals
*/
extern int    errno;


/*---------------------------------------------------------------------------*/


/*
** Feature defines
*/

/* Context for logging */
#undef UTL_LOG_CONTEXT
#define UTL_LOG_CONTEXT                     (unsigned char *)"com.fsconsult.mps.src.utils.socket"


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlSocketOpenServerTcp()

    Purpose:    This function opens a tcp socket on the specified port for the 
                server to start listening on for incoming client connections.

    Parameters: pucHostName         host name to bind to (set to NULL to bind to all addresses)
                iPort               port
                uiBufferLength      buffer length (set to 0 for default length)
                uiBacklog           backlog
                uiRetries           retries
                uiRetryInterval     retry interval in microseconds
                bBlocking           set to true if the open should block
                                    (use UTL_SOCKET_BLOCKING and UTL_SOCKET_NON_BLOCKING macros)
                piSocket            return pointer for the socket

    Globals:    none

    Returns:    UTL error code

*/
int iUtlSocketOpenServerTcp
(
    unsigned char *pucHostName,
    int iPort,
    unsigned int uiBufferLength,
    unsigned int uiBacklog,
    unsigned int uiRetries,
    unsigned int uiRetryInterval,
    boolean bBlocking,
    int *piSocket
)
{

    int                     iError = UTL_NoError;
    unsigned int            uiTries = 0;
    int                     iSocket = -1;
    unsigned int            uiOne = 1;
    struct sockaddr_in      siSocketAddress;
    struct hostent          *pheHost = NULL;


    /* Check the parameters */
    if ( iPort < 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'iPort' parameter passed to 'iUtlSocketOpenServerTcp'."); 
        return (UTL_SocketInvalidPort);
    }

    if ( (bBlocking != UTL_SOCKET_BLOCKING) && (bBlocking != UTL_SOCKET_NON_BLOCKING) ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'bBlocking' parameter passed to 'iUtlSocketOpenServerTcp'."); 
        return (UTL_SocketInvalidBlocking);
    }

    if ( piSocket == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'piSocket' parameter passed to 'iUtlSocketOpenServerTcp'."); 
        return (UTL_ReturnParameterError);
    }


    /* Set the backlog */
    if ( uiBacklog > UTL_SOCKET_BACKLOG_MAX ) {
        uiBacklog = UTL_SOCKET_BACKLOG_DEFAULT;
    }

    /* Set the retries */
    if ( uiRetries > UTL_SOCKET_RETRIES_MAX ) {
        uiRetries = UTL_SOCKET_RETRIES_DEFAULT;
    }

    /* Set the retry interval */
    if ( uiRetryInterval <= 0 ) {
        uiRetryInterval = UTL_SOCKET_RETRY_INTERVAL_DEFAULT;
    }


    /* Open the socket, we try up to uiRetries times */
    for ( uiTries = 1, iError = UTL_NoError; uiTries <= (uiRetries + 1); uiTries++ ) {

        /* Open the socket */
        if ( (iSocket = s_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0 ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to create a TCP/IP socket.");
            return (UTL_SocketOpenServerFailed);
        }

        /* Reuse the socket */
        if ( s_setsockopt(iSocket, SOL_SOCKET, SO_REUSEADDR, (void *)&uiOne, sizeof(uiOne)) == -1 ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to set the socket option to reuse the socket.");
            iError = UTL_SocketSetSocketOptionFailed;
            goto bailFromiUtlSocketOpenServerTcp;
        }
        
        /* Set the blocking state on the socket */
        if ( bBlocking == true ) {
            if ( (iError = iUtlSocketSetBlockingIO(iSocket)) != UTL_NoError ) {
                iUtlLogError(UTL_LOG_CONTEXT, "Failed to set the socket to blocking I/O.");
                goto bailFromiUtlSocketOpenServerTcp;
            }
        }
        else {
            if ( (iError = iUtlSocketSetNonBlockingIO(iSocket)) != UTL_NoError ) {
                iUtlLogError(UTL_LOG_CONTEXT, "Failed to set the socket to non-blocking I/O.");
                goto bailFromiUtlSocketOpenServerTcp;
            }
        }

        /* Set the socket buffer lengths */
        if ( uiBufferLength > 0 ) {

            /* Set the socket send buffer length */
            if ( (iError = iUtlSocketSetSendBufferLength(iSocket, uiBufferLength)) != UTL_NoError ) {
                iUtlLogError(UTL_LOG_CONTEXT, "Failed to set the socket send buffer length.");
                goto bailFromiUtlSocketOpenServerTcp;
            }

            /* Set the socket receive buffer length */
            if ( (iError = iUtlSocketSetReceiveBufferLength(iSocket, uiBufferLength)) != UTL_NoError ) {
                iUtlLogError(UTL_LOG_CONTEXT, "Failed to set the socket receive buffer length.");
                goto bailFromiUtlSocketOpenServerTcp;
            }
        }


        /* Clear the socket address structure */
        s_memset((char *)&siSocketAddress, 0, sizeof(siSocketAddress));
    

        /* Listen to all addresses if there is no host name */
        if ( bUtlStringsIsStringNULL(pucHostName) == true ) {
            siSocketAddress.sin_family = AF_INET;
            siSocketAddress.sin_addr.s_addr = htonl(INADDR_ANY);
        }
        else {

            /* Set the host name, assume it is an IP address, otherwise resolve it */
            if ( (siSocketAddress.sin_addr.s_addr = inet_addr(pucHostName)) != -1 ) {
                siSocketAddress.sin_family = AF_INET;
            }
            else {
        
                /* Resolve the host name */
                if ( (pheHost = gethostbyname(pucHostName)) == NULL ) {
                    iUtlLogError(UTL_LOG_CONTEXT, "Failed to resolve host name: '%s'.");
                    iError = UTL_SocketOpenServerFailed;
                    goto bailFromiUtlSocketOpenServerTcp;
                }
        
                siSocketAddress.sin_family = pheHost->h_addrtype;

#if defined(h_addr)
                s_memcpy((void *)&siSocketAddress.sin_addr, pheHost->h_addr_list[0], pheHost->h_length);
#endif    /* defined(h_addr) */

            }
        }

        /* Set the port */
        siSocketAddress.sin_port = htons((unsigned short)iPort);

        /* Bind the socket */
        if ( s_bind(iSocket, (struct sockaddr*)&siSocketAddress, sizeof(siSocketAddress) ) == -1 ) {
            
            /* Cant bind for some reason, so we try to clear the socket if it is in use */
            if ( errno == EADDRINUSE ) {
                /* Try connecting to it */
                s_connect(iSocket, (struct sockaddr *)&siSocketAddress, sizeof (siSocketAddress));
            }
            else {
                iUtlLogError(UTL_LOG_CONTEXT, "Failed bind to a socket on the port: %d.", iPort);
                iError = UTL_SocketOpenServerFailed;
                goto bailFromiUtlSocketOpenServerTcp;
            }
        }
        else {
        
            /* Add start listening */
            if ( s_listen(iSocket, uiBacklog) == 0 ) {
                
                if ( uiTries > 1 ) {
                    iUtlLogInfo(UTL_LOG_CONTEXT, "Cleared socket on the port: %d.", iPort);
                }
                
                /* We are connected and we are done, so we break out of the loop */
                break;
            }

            iUtlLogError(UTL_LOG_CONTEXT, "Failed to listen to the socket on the port: %d.", iPort);
        }
        
        
        /* Warn the user that we are clearing the socket */
        if ( uiTries == 2 ) {
            iUtlLogWarn(UTL_LOG_CONTEXT, "Clearing socket on the port: %d.", iPort);
        }
        
        /* Close and reset the socket */
        iUtlSocketClose(iSocket);
        iSocket = -1;

        /* Sleep to give time for the socket to clear */
        s_usleep(uiRetryInterval);
    }


    
    /* Bail label */
    bailFromiUtlSocketOpenServerTcp:


    /* Handle the error */
    if ( iError == UTL_NoError ) {

        /* Set the return pointer */
        *piSocket = iSocket;
    }
    else {

        /* Close the socket */
        iUtlSocketClose(iSocket);
    }


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlSocketAcceptClientTcp()

    Purpose:    This function accepts a tcp client connection.

    Parameters: iSocket     socket to accept a client connection on
                piSocket    return pointer for the accepted socket

    Globals:    none

    Returns:    UTL error code

*/
int iUtlSocketAcceptClientTcp
(
    int iSocket,
    int *piSocket
)
{

/*     struct sockaddr_in   siSocketAddress; */
/*     int                  iSocketAddressSize = 0; */


    /* Check the parameters */
    if ( iSocket < 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'iSocket' parameter passed to 'iUtlSocketAcceptClientTcp'."); 
        return (UTL_SocketInvalidSocket);
    }

    if ( piSocket == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'piSocket' parameter passed to 'iUtlSocketAcceptClientTcp'."); 
        return (UTL_ReturnParameterError);
    }


    /* Get the size of the socket address structure */
/*     iSocketAddressSize = sizeof(siSocketAddress); */


    /* Loop forever, we control the loop from within */
    while ( true ) {

        /* Clear the global error */
        errno = EOK;

        /* Accept the connection, break out if we succeeded */
/*         if ( (*piSocket = s_accept(iSocket, (struct sockaddr *)&siSocketAddress, &iSocketAddressSize)) != -1 ) { */
        if ( (*piSocket = s_accept(iSocket, NULL, NULL)) != -1 ) {
            break;
        }
        
        /* Handle the error */
        if ( (errno == EAGAIN) || (errno == EWOULDBLOCK) ) {
            /* The socket would block, which means the other end is not ready to read */
            return (UTL_SocketWouldBlock);
        }        
        else if ( errno == EINTR ) {
            /* The socket was interrupted, so we loop and try again */
            ;
        }
        else if ( errno == EBADF ) {
            /* The socket was bad */
            return (UTL_SocketInvalidDescriptor);
        }
        else {
            /* Accept error we cant handle */
/*     iUtlLogDebug(UTL_LOG_CONTEXT, "Error accepting client connection."); */
            return (UTL_SocketAcceptClientFailed);
        }
    }


    return (UTL_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlSocketOpenTcp()

    Purpose:    This function opens a tcp socket connection to the host on the 
                specified post. The tcp socket is then bound to the socket descriptor. 

    Parameters: pucHostName         host name
                iPort               port
                uiBufferLength      buffer length (set to 0 for default length)
                uiRetries           retries
                uiRetryInterval     retry interval in microseconds
                bBlocking           set to true if the open should block
                                    (use UTL_SOCKET_BLOCKING and UTL_SOCKET_NON_BLOCKING macros)
                piSocket            return pointer for the opened socket

    Globals:    none

    Returns:    UTL error code

*/
int iUtlSocketOpenTcp
(
    unsigned char *pucHostName,
    int iPort,
    unsigned int uiBufferLength,
    unsigned int uiRetries,
    unsigned int uiRetryInterval,
    boolean bBlocking,
    int *piSocket
)
{

    int                     iError = UTL_NoError;
    int                     iSocket = -1;
    unsigned int            uiTries = 0;
    struct hostent          *pheHost = NULL;
    struct sockaddr_in      siSocketAddress;


    /* Check the parameters */
    if ( bUtlStringsIsStringNULL(pucHostName) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pucHostName' parameter passed to 'iUtlSocketOpenTcp'."); 
        return (UTL_SocketInvalidHostName);
    }

    if ( iPort < 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'iPort' parameter passed to 'iUtlSocketOpenTcp'."); 
        return (UTL_SocketInvalidPort);
    }

    if ( piSocket == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'piSocket' parameter passed to 'iUtlSocketOpenTcp'."); 
        return (UTL_ReturnParameterError);
    }


    /* Set the retries */
    if ( uiRetries > UTL_SOCKET_RETRIES_MAX ) {
        uiRetries = UTL_SOCKET_RETRIES_DEFAULT;
    }

    /* Set the retry interval */
    if ( uiRetryInterval <= 0 ) {
        uiRetryInterval = UTL_SOCKET_RETRY_INTERVAL_DEFAULT;
    }


    /* Clear the socket address structure */
    s_memset((char *)&siSocketAddress, 0, sizeof(siSocketAddress));


    /* Set the host name, assume it is an IP address, otherwise resolve it */
    if ( (siSocketAddress.sin_addr.s_addr = inet_addr(pucHostName)) != -1 ) {
        siSocketAddress.sin_family = AF_INET;
    }
    else {

        /* Resolve the host name */
        if ( (pheHost = gethostbyname(pucHostName)) == NULL ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to resolve host name: '%s'.", pucHostName);
            return (UTL_SocketOpenClientFailed);
        }

        siSocketAddress.sin_family = pheHost->h_addrtype;
#if defined(h_addr)
        s_memcpy((void *)&siSocketAddress.sin_addr, pheHost->h_addr_list[0], pheHost->h_length);
#endif    /* defined(h_addr) */
    }

    siSocketAddress.sin_port = htons((unsigned short)iPort);


    /* Create the socket */
    if ( (iSocket = s_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to create a TCP/IP socket.");
        return (UTL_SocketOpenClientFailed);
    }

    /* Set the blocking state on the socket */
    if ( bBlocking == true ) {
        if ( (iError = iUtlSocketSetBlockingIO(iSocket)) != UTL_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to set the socket to blocking I/O.");
            goto bailFromiUtlSocketOpenTcp;
        }
    }
    else {
        if ( (iError = iUtlSocketSetNonBlockingIO(iSocket)) != UTL_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to set the socket to non-blocking I/O.");
            goto bailFromiUtlSocketOpenTcp;
        }
    }

    /* Set the socket buffer lengths */
    if ( uiBufferLength > 0 ) {

        /* Set the socket send buffer length */
        if ( (iError = iUtlSocketSetSendBufferLength(iSocket, uiBufferLength)) != UTL_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to set the socket send buffer length.");
            goto bailFromiUtlSocketOpenTcp;
        }

        /* Set the socket receive buffer length */
        if ( (iError = iUtlSocketSetReceiveBufferLength(iSocket, uiBufferLength)) != UTL_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to set the socket receive buffer length.");
            goto bailFromiUtlSocketOpenTcp;
        }
    }


    /* Open the socket, we try up to uiRetries times if we are interrupted */
    for ( uiTries = 1, iError = UTL_NoError; uiTries <= (uiRetries + 1); uiTries++ ) {
        
        /* Reset the error global */
        errno = EOK;

        /* Connect */
        if ( s_connect(iSocket, (struct sockaddr *)&siSocketAddress, sizeof(siSocketAddress)) == 0 ) {
            break;
        }

        /* Handle the error */
        if ( errno == EINTR ) {
            /* Sleep if we were interrupted */
            s_usleep(uiRetryInterval);
        }
        else if ( (bBlocking == UTL_SOCKET_NON_BLOCKING) && (errno == EINPROGRESS) ) {
            /* The socket is set to non-blocking I/O and the connection is in progress */
            break;
        }
        else  {
            iError = UTL_SocketOpenClientFailed;
            goto bailFromiUtlSocketOpenTcp;
        }
    }


    
    /* Bail label */
    bailFromiUtlSocketOpenTcp:


    /* Handle the error */
    if ( iError == UTL_NoError ) {

        /* Set the return pointer */
        *piSocket = iSocket;
    }
    else {

        /* Close the socket */
        iUtlSocketClose(iSocket);
    }


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlSocketOpenServerUdp()

    Purpose:    This function opens a udp socket on the specified port for the 
                server to start listening on for incoming client connections.

    Parameters: pucHostName         host name
                iPort               port
                uiBufferLength      buffer length (set to 0 for default length)
                uiRetries           retries
                uiRetryInterval     retry interval in microseconds
                bBlocking           set to true if the open should block
                                    (use UTL_SOCKET_BLOCKING and UTL_SOCKET_NON_BLOCKING macros)
                piSocket            return pointer for the opened socket

    Globals:    none

    Returns:    UTL error code

*/
int iUtlSocketOpenServerUdp
(
    unsigned char *pucHostName,
    int iPort,
    unsigned int uiBufferLength,
    unsigned int uiRetries,
    unsigned int uiRetryInterval,
    boolean bBlocking,
    int *piSocket
)
{

    int                     iError = UTL_NoError;
    unsigned int            uiTries = 0;
    int                     iSocket = -1;
    unsigned int            uiOne = 1;
    struct sockaddr_in      siSocketAddress;
    struct hostent          *pheHost = NULL;


    /* Check the parameters */
    if ( iPort < 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'iPort' parameter passed to 'iUtlSocketOpenServerUdp'."); 
        return (UTL_SocketInvalidPort);
    }

    if ( piSocket == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'piSocket' parameter passed to 'iUtlSocketOpenServerUdp'."); 
        return (UTL_ReturnParameterError);
    }


    /* Set the retries */
    if ( uiRetries > UTL_SOCKET_RETRIES_MAX ) {
        uiRetries = UTL_SOCKET_RETRIES_DEFAULT;
    }

    /* Set the retry interval */
    if ( uiRetryInterval <= 0 ) {
        uiRetryInterval = UTL_SOCKET_RETRY_INTERVAL_DEFAULT;
    }


    /* Open the socket, we try up to uiRetries times */
    for ( uiTries = 1, iError = UTL_NoError; uiTries <= (uiRetries + 1); uiTries++ ) {

        /* Open the socket */
        if ( (iSocket = s_socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to create a TCP/IP socket.");
            return (UTL_SocketOpenServerFailed);
        }

        /* Reuse the socket */
        if ( s_setsockopt(iSocket, SOL_SOCKET, SO_REUSEADDR, (void *)&uiOne, sizeof(uiOne)) == -1 ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to set the socket option to reuse the socket.");
            iError = UTL_SocketSetSocketOptionFailed;
            goto bailFromiUtlSocketOpenServerUdp;
        }

        /* Set the blocking state on the socket */
        if ( bBlocking == true ) {
            if ( (iError = iUtlSocketSetBlockingIO(iSocket)) != UTL_NoError ) {
                iUtlLogError(UTL_LOG_CONTEXT, "Failed to set the socket to blocking I/O.");
                goto bailFromiUtlSocketOpenServerUdp;
            }
        }
        else {
            if ( (iError = iUtlSocketSetNonBlockingIO(iSocket)) != UTL_NoError ) {
                iUtlLogError(UTL_LOG_CONTEXT, "Failed to set the socket to non-blocking I/O.");
                goto bailFromiUtlSocketOpenServerUdp;
            }
        }

        /* Set the socket buffer lengths */
        if ( uiBufferLength > 0 ) {

            /* Set the socket send buffer length */
            if ( (iError = iUtlSocketSetSendBufferLength(iSocket, uiBufferLength)) != UTL_NoError ) {
                iUtlLogError(UTL_LOG_CONTEXT, "Failed to set the socket send buffer length.");
                goto bailFromiUtlSocketOpenServerUdp;
            }

            /* Set the socket receive buffer length */
            if ( (iError = iUtlSocketSetReceiveBufferLength(iSocket, uiBufferLength)) != UTL_NoError ) {
                iUtlLogError(UTL_LOG_CONTEXT, "Failed to set the socket receive buffer length.");
                goto bailFromiUtlSocketOpenServerUdp;
            }
        }


        /* Clear the socket address structure */
        s_memset((char *)&siSocketAddress, 0, sizeof(siSocketAddress));
    

        /* Listen to all addresses if there is no host name */
        if ( bUtlStringsIsStringNULL(pucHostName) == true ) {
            siSocketAddress.sin_family = AF_INET;
            siSocketAddress.sin_addr.s_addr = htonl(INADDR_ANY);
        }
        else {

            /* Set the host name, assume it is an IP address, otherwise resolve it */
            if ( (siSocketAddress.sin_addr.s_addr = inet_addr(pucHostName)) != -1 ) {
                siSocketAddress.sin_family = AF_INET;
            }
            else {
        
                /* Resolve the host name */
                if ( (pheHost = gethostbyname(pucHostName)) == NULL ) {
                    iUtlLogError(UTL_LOG_CONTEXT, "Failed to resolve host name: '%s'.", pucHostName);
                    iError = UTL_SocketOpenServerFailed;
                    goto bailFromiUtlSocketOpenServerUdp;
                }
        
                siSocketAddress.sin_family = pheHost->h_addrtype;

#if defined(h_addr)
                s_memcpy((void *)&siSocketAddress.sin_addr, pheHost->h_addr_list[0], pheHost->h_length);
#endif    /* defined(h_addr) */

            }
        }

        /* Set the port */
        siSocketAddress.sin_port = htons((unsigned short)iPort);

        /* Bind the socket */
        if ( s_bind(iSocket, (struct sockaddr*)&siSocketAddress, sizeof(siSocketAddress) ) < 0 ) {
            
            /* Cant bind for some reason, so we try to clear the socket if it is in use */
            if ( errno == EADDRINUSE ) {
                /* Try connecting to it */
                s_connect(iSocket, (struct sockaddr *)&siSocketAddress, sizeof (siSocketAddress));
            }
            else {
                iUtlLogError(UTL_LOG_CONTEXT, "Failed bind to a socket on the port: %d.", iPort);
                iError = UTL_SocketOpenServerFailed;
                goto bailFromiUtlSocketOpenServerUdp;
            }
        }
        else {
        
            if ( uiTries > 1 ) {
                iUtlLogInfo(UTL_LOG_CONTEXT, "Cleared socket on the port: %d.", iPort);
            }
                
            /* We are connected and we are done, so we break out of the loop */
            break;
        }
        
        
        /* Warn the user that we are clearing the socket */
        if ( uiTries == 2 ) {
            iUtlLogWarn(UTL_LOG_CONTEXT, "Clearing socket on the port: %d.", iPort);
        }
        
        /* Close and reset the socket */
        iUtlSocketClose(iSocket);
        iSocket = -1;

        /* Sleep to give time for the socket to clear */
        s_usleep(uiRetryInterval);
    }


    
    /* Bail label */
    bailFromiUtlSocketOpenServerUdp:


    /* Handle the error */
    if ( iError == UTL_NoError ) {

        /* Set the return pointer */
        *piSocket = iSocket;
    }
    else {

        /* Close the socket */
        iUtlSocketClose(iSocket);
    }


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlSocketOpenUdp()

    Purpose:    This function opens a udp socket connection to the host on the 
                specified iPort. The socket is then bound to the socket descriptor. 

    Parameters: pucHostName         host name
                iPort               port
                uiBufferLength      buffer length (set to 0 for default length)
                uiRetries           retries
                uiRetryInterval     retry interval in microseconds
                bBlocking           set to true if the open should block
                                    (use UTL_SOCKET_BLOCKING and UTL_SOCKET_NON_BLOCKING macros)
                piSocket            return pointer for the opened socket

    Globals:    none

    Returns:    UTL error code

*/
int iUtlSocketOpenUdp
(
    unsigned char *pucHostName,
    int iPort,
    unsigned int uiBufferLength,
    unsigned int uiRetries,
    unsigned int uiRetryInterval,
    boolean bBlocking,
    int *piSocket
)
{

    int                     iError = UTL_NoError;
    int                     iSocket = -1;
    unsigned int            uiTries = 0;
    struct hostent          *pheHost = NULL;
    struct sockaddr_in      siSocketAddress;


    /* Check the parameters */
    if ( bUtlStringsIsStringNULL(pucHostName) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pucHostName' parameter passed to 'iUtlSocketOpenUdp'."); 
        return (UTL_SocketInvalidHostName);
    }

    if ( iPort < 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'iPort' parameter passed to 'iUtlSocketOpenUdp'."); 
        return (UTL_SocketInvalidPort);
    }

    if ( piSocket == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'piSocket' parameter passed to 'iUtlSocketOpenUdp'."); 
        return (UTL_ReturnParameterError);
    }


    /* Set the retries */
    if ( uiRetries > UTL_SOCKET_RETRIES_MAX ) {
        uiRetries = UTL_SOCKET_RETRIES_DEFAULT;
    }

    /* Set the retry interval */
    if ( uiRetryInterval <= 0 ) {
        uiRetryInterval = UTL_SOCKET_RETRY_INTERVAL_DEFAULT;
    }


    /* Clear the socket address structure */
    s_memset((char *)&siSocketAddress, 0, sizeof(siSocketAddress));


    /* Set the host name, assume it is an IP address, otherwise resolve it */
    if ( (siSocketAddress.sin_addr.s_addr = inet_addr(pucHostName)) != -1 ) {
        siSocketAddress.sin_family = AF_INET;
    }
    else {

        /* Resolve the host name */
        if ( (pheHost = gethostbyname(pucHostName)) == NULL ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to resolve host name: '%s'.", pucHostName);
            return (UTL_SocketOpenClientFailed);
        }

        siSocketAddress.sin_family = pheHost->h_addrtype;
#if defined(h_addr)
        s_memcpy((void *)&siSocketAddress.sin_addr, pheHost->h_addr_list[0], pheHost->h_length);
#endif    /* defined(h_addr) */
    }

    siSocketAddress.sin_port = htons((unsigned short)iPort);

    /* Create the socket */
    if ( (iSocket = s_socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to create a UDP socket.");
        return (UTL_SocketOpenClientFailed);
    }

    /* Set the blocking state on the socket */
    if ( bBlocking == true ) {
        if ( (iError = iUtlSocketSetBlockingIO(iSocket)) != UTL_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to set the socket to blocking I/O.");
            goto bailFromiUtlSocketOpenUdp;
        }
    }
    else {
        if ( (iError = iUtlSocketSetNonBlockingIO(iSocket)) != UTL_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to set the socket to non-blocking I/O.");
            goto bailFromiUtlSocketOpenUdp;
        }
    }

    /* Set the socket buffer lengths */
    if ( uiBufferLength > 0 ) {

        /* Set the socket send buffer length */
        if ( (iError = iUtlSocketSetSendBufferLength(iSocket, uiBufferLength)) != UTL_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to set the socket send buffer length.");
            goto bailFromiUtlSocketOpenUdp;
        }

        /* Set the socket receive buffer length */
        if ( (iError = iUtlSocketSetReceiveBufferLength(iSocket, uiBufferLength)) != UTL_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to set the socket receive buffer length.");
            goto bailFromiUtlSocketOpenUdp;
        }
    }


    /* Open the socket, we try up to uiRetries times if we are interrupted */
    for ( uiTries = 1, iError = UTL_NoError; uiTries <= (uiRetries + 1); uiTries++ ) {
        
        /* Reset the error global */
        errno = EOK;

        /* Connect */
        if ( s_connect(iSocket, (struct sockaddr *)&siSocketAddress, sizeof(siSocketAddress)) == 0 ) {
            break;
        }

        /* Handle the error */
        if ( errno == EINTR ) {
            /* Sleep if we were interrupted */
            s_usleep(uiRetryInterval);
        }
        else if ( (bBlocking == UTL_SOCKET_NON_BLOCKING) && (errno == EINPROGRESS) ) {
            /* The socket is set to non-blocking I/O and the connection is in progress */
            break;
        }
        else  {
            iError = UTL_SocketOpenClientFailed;
            goto bailFromiUtlSocketOpenUdp;
        }
    }


    
    /* Bail label */
    bailFromiUtlSocketOpenUdp:


    /* Handle the error */
    if ( iError == UTL_NoError ) {

        /* Set the return pointer */
        *piSocket = iSocket;
    }
    else {

        /* Close the socket */
        iUtlSocketClose(iSocket);
    }


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlSocketClose()

    Purpose:    This function closes a socket connection.

    Parameters: iSocket     socket

    Globals:    none

    Returns:    UTL error code

*/
int iUtlSocketClose
(
    int iSocket
)
{

    /* Check the parameters */
    if ( iSocket < 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'iSocket' parameter passed to 'iUtlSocketClose'."); 
        return (UTL_SocketInvalidSocket);
    }

    /* Close the socket connection - ignore errors */
    s_close(iSocket);


    return (UTL_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlSocketSetBlockingIO()

    Purpose:    This function sets the socket to blocking IO.

    Parameters: iSocket     socket

    Globals:    none

    Returns:    UTL error code

*/
int iUtlSocketSetBlockingIO
(
    int iSocket
)
{
    
    int     iFlag = 0;
    
    
    /* Check the parameters */
    if ( iSocket < 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'iSocket' parameter passed to 'iUtlSocketSetBlockingIO'."); 
        return (UTL_SocketInvalidSocket);
    }


    /* Turn off non-blocking IO */
    if ( s_ioctl(iSocket, FIONBIO, &iFlag) == -1 ) {
        return (UTL_SocketSetSocketBlockingStateFailed);
    }


    /* Turn off non-blocking IO - NOT */
/*     if ( fcntl(iSocket, F_SETFL, O_NONBLOCK) == -1 ) { */
/*         return (UTL_SocketSetSocketBlockingStateFailed); */
/*     } */


    return (UTL_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlSocketSetNonBlockingIO()

    Purpose:    This function sets the socket buffer length.

    Parameters: iSocket             socket
                uiBufferLength      buffer length

    Globals:    none

    Returns:    UTL error code

*/
int iUtlSocketSetNonBlockingIO
(
    int iSocket
)
{
    
    int     iFlag = 1;
    
    
    /* Check the parameters */
    if ( iSocket < 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'iSocket' parameter passed to 'iUtlSocketSetNonBlockingIO'."); 
        return (UTL_SocketInvalidSocket);
    }


    /* Turn on non-blocking IO */
    if ( s_ioctl(iSocket, FIONBIO, &iFlag) == -1 ) {
        return (UTL_SocketSetSocketBlockingStateFailed);
    }


    /* Turn on non-blocking IO */
/*     if ( fcntl(iSocket, F_SETFL, O_NONBLOCK) == -1 ) { */
/*         return (UTL_SocketSetSocketBlockingStateFailed); */
/*     } */


    return (UTL_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlSocketGetSendBufferLength()

    Purpose:    This function gets the socket send buffer length.

    Parameters: iSocket             socket
                puiBufferLength     return pointer for the buffer length

    Globals:    none

    Returns:    UTL error code

*/
int iUtlSocketGetSendBufferLength
(
    int iSocket, 
    unsigned int *puiBufferLength
)
{
    
    unsigned int    uiBufferLengthSize = sizeof(*puiBufferLength);
    

    /* Check the parameters */
    if ( iSocket < 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'iSocket' parameter passed to 'iUtlSocketGetSendBufferLength'."); 
        return (UTL_SocketInvalidSocket);
    }
    
    if ( puiBufferLength == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'puiBufferLength' parameter passed to 'iUtlSocketGetSendBufferLength'."); 
        return (UTL_SocketInvalidSocket);
    }
    

    /* Check that the socket is not stdin, stdout or stderr, makes the assumption 
    ** that the file descriptors have not been reused for something else 
    */
    if ( (iSocket == fileno(stdin)) || (iSocket == fileno(stdout)) || (iSocket == fileno(stderr)) ) {
        return (UTL_NoError);
    }


    /* Get the socket send buffer length */
    if ( s_getsockopt(iSocket, SOL_SOCKET, SO_SNDBUF, (void *)puiBufferLength, &uiBufferLengthSize) == -1 ) {
        return (UTL_SocketGetSocketOptionFailed);
    }

    
#if defined(linux)

    /* Linux - linux allocates twice the space requested, using the balance for its own control structures,
    ** and that space is included in the buffer length we obtained, so we need to divide it by two to 
    ** get the space actually available to us, see 'man 7 tcp'.
    */

    /* Divide the buffer length in 2 */
    *puiBufferLength /= 2;

#endif    /* defined(linux) */


    return (UTL_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlSocketGetReceiveBufferLength()

    Purpose:    This function gets the socket receive buffer length.

    Parameters: iSocket             socket
                puiBufferLength     return pointer for the buffer length

    Globals:    none

    Returns:    UTL error code

*/
int iUtlSocketGetReceiveBufferLength
(
    int iSocket, 
    unsigned int *puiBufferLength
)
{
    
    unsigned int    uiBufferLengthSize = sizeof(*puiBufferLength);
    
    
    /* Check the parameters */
    if ( iSocket < 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'iSocket' parameter passed to 'iUtlSocketGetReceiveBufferLength'."); 
        return (UTL_SocketInvalidSocket);
    }
    
    if ( puiBufferLength == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'puiBufferLength' parameter passed to 'iUtlSocketGetReceiveBufferLength'."); 
        return (UTL_SocketInvalidSocket);
    }


    /* Check that the socket is not stdin, stdout or stderr, makes the assumption 
    ** that the file descriptors have not been reused for something else 
    */
    if ( (iSocket == fileno(stdin)) || (iSocket == fileno(stdout)) || (iSocket == fileno(stderr)) ) {
        return (UTL_NoError);
    }


    /* Get the socket receive buffer length */
    if ( s_getsockopt(iSocket, SOL_SOCKET, SO_RCVBUF, (void *)puiBufferLength, &uiBufferLengthSize) == -1 ) {
        return (UTL_SocketGetSocketOptionFailed);
    }


#if defined(linux)

    /* Linux - linux allocates twice the space requested, using the balance for its own control structures,
    ** and that space is included in the buffer length we obtained, so we need to divide it by two to 
    ** get the space actually available to us, see 'man 7 tcp'.
    */

    /* Divide the buffer length in 2 */
    *puiBufferLength /= 2;

#endif    /* defined(linux) */


    return (UTL_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlSocketSetSendBufferLength()

    Purpose:    This function sets the socket send buffer length.
    
                This needs to be set before a listen() or a connect() is 
                performed on a socket.

    Parameters: iSocket             socket
                uiBufferLength      buffer length

    Globals:    none

    Returns:    UTL error code

*/
int iUtlSocketSetSendBufferLength
(
    int iSocket, 
    unsigned int uiBufferLength
)
{
    
    /* Check the parameters */
    if ( iSocket < 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'iSocket' parameter passed to 'iUtlSocketGetReceiveBufferLength'."); 
        return (UTL_SocketInvalidSocket);
    }
    
    if ( uiBufferLength <= 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiBufferLength' parameter passed to 'iUtlSocketGetReceiveBufferLength'."); 
        return (UTL_SocketInvalidSocket);
    }


    /* Set the socket send buffer length */
    if ( s_setsockopt(iSocket, SOL_SOCKET, SO_SNDBUF, (void *)&uiBufferLength, sizeof(uiBufferLength)) == -1 ) {
        return (UTL_SocketSetSocketOptionFailed);
    }


    return (UTL_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlSocketSetReceiveBufferLength()

    Purpose:    This function sets the socket receive buffer length.
    
                This needs to be set before a listen() or a connect() is 
                performed on a socket.

    Parameters: iSocket             socket
                uiBufferLength      buffer length

    Globals:    none

    Returns:    UTL error code

*/
int iUtlSocketSetReceiveBufferLength
(
    int iSocket, 
    unsigned int uiBufferLength
)
{
    
    /* Check the parameters */
    if ( iSocket < 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'iSocket' parameter passed to 'iUtlSocketSetReceiveBufferLength'."); 
        return (UTL_SocketInvalidSocket);
    }

    if ( uiBufferLength <= 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiBufferLength' parameter passed to 'iUtlSocketSetReceiveBufferLength'."); 
        return (UTL_SocketInvalidSocket);
    }


    /* Check that the socket is not stdin, stdout or stderr, makes the assumption 
    ** that the file descriptors have not been reused for something else 
    */
    if ( (iSocket == fileno(stdin)) || (iSocket == fileno(stdout)) || (iSocket == fileno(stderr)) ) {
        return (UTL_NoError);
    }


    /* Set the socket receive buffer length */
    if ( s_setsockopt(iSocket, SOL_SOCKET, SO_RCVBUF, (void *)&uiBufferLength, sizeof(uiBufferLength)) == -1 ) {
        return (UTL_SocketSetSocketOptionFailed);
    }


    return (UTL_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlSocketGetError()

    Purpose:    This function get the socket error, this also clears the error.

    Parameters: iSocket     socket

    Globals:    none

    Returns:    UTL error code

*/
int iUtlSocketGetError
(
    int iSocket
)
{
    
    int     iError = 0;
    int     iErrorSize = sizeof(iError);
    
    
    /* Check the parameters */
    if ( iSocket < 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'iSocket' parameter passed to 'iUtlSocketGetError'."); 
        return (UTL_SocketInvalidSocket);
    }


    /* Check that the socket is not stdin, stdout or stderr, makes the assumption 
    ** that the file descriptors have not been reused for something else 
    */
    if ( (iSocket == fileno(stdin)) || (iSocket == fileno(stdout)) || (iSocket == fileno(stderr)) ) {
        return (UTL_NoError);
    }


    /* Get the socket error state */
    if ( s_getsockopt(iSocket, SOL_SOCKET, SO_ERROR, (void *)&iError, &iErrorSize) == -1 ) {
        return (UTL_SocketGetSocketOptionFailed);
    }


    /* Handle the returned error */    
    if ( iError == 0 ) {
        /* No Error */
        iError = UTL_NoError;
    }
    else if ( iError == EPIPE ) {
        /* Socket was closed */
        iError = UTL_SocketClosed;
    }
    else if ( iError == EBADF ) {
        /* Socket was bad */
        iError = UTL_SocketInvalidDescriptor;
    }
    else {
        /* Default error */
        iError = UTL_SocketError;
    }


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlSocketReadyToSend()

    Purpose:    This function checks to see if the socket is ready for sending.

    Parameters: iSocket     socket descriptor
                iTimeOut    millisecond timeout 
                            (0 = return at once, -1 = block until ready)

    Globals:    none

    Returns:    UTL error code

*/
int iUtlSocketReadyToSend
(
    int iSocket,
    int iTimeOut
)
{

    int             iError = UTL_NoError;
    struct pollfd   pfPollFd;
    int             iStatus = 0;


    /* Check the parameters */
    if ( iSocket < 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'iSocket' parameter passed to 'iUtlSocketReadyToSend'."); 
        return (UTL_SocketInvalidSocket);
    }

    if ( (iTimeOut != -1) && (iTimeOut < 0) ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'iTimeOut' parameter passed to 'iUtlSocketReadyToSend'."); 
        return (UTL_SocketInvalidTimeOut);
    }


    /* Set the pollfd structure */
    pfPollFd.fd = iSocket;
    pfPollFd.events = POLLOUT;


    /* Loop forever, we control the loop from within */
    while ( true ) {

        /* Clear the return events from the pollfd structure */
        pfPollFd.revents = 0;

        /* Clear the global error */
        errno = EOK;
        
        /* Issue the poll, break out if we succeeded */
        if ( (iStatus = s_poll(&pfPollFd, 1, iTimeOut)) != -1 ) {
            break;
        }

        /* Handle the error */
        if ( errno == EINTR ) {
            /* The socket was interrupted, so we loop and try again */
            ;
        }
        else if ( errno == EBADF ) {
            /* The socket was bad */
            return (UTL_SocketInvalidDescriptor);
        }
        else {
            /* Poll() error we cant handle */
            return (UTL_SocketPollError);
        }
    }


    /* Check the socket for errors */
    if ( (iError = iUtlSocketGetError(iSocket)) != UTL_NoError ) {
        /* Socket error */
        return (iError);
    }

    /* Check the pollfd structure for errors */
    if ( (pfPollFd.revents & POLLERR) > 0 ) {
        /* The socket errored */
        return (UTL_SocketError);
    }

    /* Check the pollfd structure for hangups */
    if ( (pfPollFd.revents & POLLHUP) > 0 ) {
        /* The socket closed */
        return (UTL_SocketClosed);
    }

    /* Check the pollfd structure for sending */
    if ( (pfPollFd.revents & POLLOUT) > 0 ) {
        /* The socket is ready for sending */
        return (UTL_NoError);
    }


    /* The socket is not ready for sending */
    return (UTL_SocketNotReadyToSend);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlSocketReadyToReceive()

    Purpose:    This function checks to see if the socket is ready to receive.

    Parameters: iSocket     socket
                iTimeOut    millisecond timeout 
                            (0 = return at once, -1 = block until ready)

    Globals:    none

    Returns:    UTL error code

*/
int iUtlSocketReadyToReceive
(
    int iSocket,
    int iTimeOut
)
{

    int             iError = UTL_NoError;
    struct pollfd   pfPollFd;
    int             iStatus = 0;


    /* Check the parameters */
    if ( iSocket < 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'iSocket' parameter passed to 'iUtlSocketReadyToReceive'."); 
        return (UTL_SocketInvalidSocket);
    }

    if ( (iTimeOut != -1) && (iTimeOut < 0) ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'iTimeOut' parameter passed to 'iUtlSocketReadyToReceive'."); 
        return (UTL_SocketInvalidTimeOut);
    }


    /* Set the pollfd structure */
    pfPollFd.fd = iSocket;
    pfPollFd.events = POLLIN;


    /* Loop forever, we control the loop from within */
    while ( true ) {

        /* Clear the return events from the pollfd structure */
        pfPollFd.revents = 0;

        /* Clear the global error */
        errno = EOK;
        
        /* Issue the select, break out if we succeeded */
        if ( (iStatus = s_poll(&pfPollFd, 1, iTimeOut)) != -1 ) {
            break;
        }

        /* Handle the error */
        if ( errno == EINTR ) {
            /* The socket was interrupted, so we loop and try again */
            ;
        }
        else if ( errno == EBADF ) {
            /* The socket was bad */
            return (UTL_SocketInvalidDescriptor);
        }
        else {
            /* Select() error we cant handle */
            return (UTL_SocketPollError);
        }
    }


    /* Check the socket for errors */
    if ( (iError = iUtlSocketGetError(iSocket)) != UTL_NoError ) {
        /* Socket error */
        return (iError);
    }

    /* Check the pollfd structure for errors */
    if ( (pfPollFd.revents & POLLERR) > 0 ) {
        /* The socket errored */
        return (UTL_SocketError);
    }

    /* Check the pollfd structure for hangups */
    if ( (pfPollFd.revents & POLLHUP) > 0 ) {
        /* The socket closed */
        return (UTL_SocketClosed);
    }

    /* Check the pollfd structure for receiving */
    if ( (pfPollFd.revents & POLLIN) > 0 ) {
        /* The socket is ready for receiving */
        return (UTL_NoError);
    }


    /* The socket is not ready for receiving, or something else happened */
    return (UTL_SocketNotReadyToReceive);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlSocketSendTcp()

    Purpose:    This function sends a number of bytes to a tcp socket. 
    
                If the socket is set to blocking, this function will block until all
                the data is sent and return UTL_NoError, or return an error if one
                occured. If UTL_SocketPartialSend is returned and the returned 
                pointer is equal to the passed buffer, then the socket has been closed 
                on us (??).
                
                If the socket is set to non-blocking, this function will return either
                UTL_NoError if all the data was sent, or UTL_SocketWouldBlock if nothing 
                could be sent, or UTL_SocketPartialSend if only some of the data could
                be sent. 
                
                In either case the ppvDataEndPtr return pointer will be set to point to 
                the next byte in the buffer after the last byte that was sent, assuming
                that the return pointer is provided.
                
                Signals interrupting the socket are handled by this function, so dont need
                to be handled by the calling function.

    Parameters: iSocket         socket descriptor
                pvData          buffer to send bytes from
                iDataLen        number of bytes to send
                ppvDataEndPtr   return pointer pointing to the byte following 
                                the last byte sent

    Globals:    none

    Returns:    UTL error code

*/
int iUtlSocketSendTcp
(
    int iSocket,
    void *pvData,
    int iDataLen,
    void **ppvDataEndPtr
)
{

    int     iDataSent = 0;


    /* Check the parameters */
    if ( iSocket < 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'iSocket' parameter passed to 'iUtlSocketSendTcp'."); 
        return (UTL_SocketInvalidSocket);
    }

    if ( pvData == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvData' parameter passed to 'iUtlSocketSendTcp'."); 
        return (UTL_SocketInvalidData);
    }

    if ( iDataLen <= 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'iDataLen' parameter passed to 'iUtlSocketSendTcp'."); 
        return (UTL_SocketInvalidData);
    }

    if ( ppvDataEndPtr == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppvDataEndPtr' parameter passed to 'iUtlSocketSendTcp'."); 
        return (UTL_ReturnParameterError);
    }


    /* Loop forever, we control the loop from inside */
    while ( true ) {
    
        /* Clear the global error */
        errno = EOK;

        /* Send the data, break out if it succeeds - send() works on sockets only, 
        ** while write() works on sockets and stdio
        */
/*         if ( (iDataSent = s_send(iSocket, pvData, iDataLen, 0)) != - 1 ) { */
        if ( (iDataSent = s_write(iSocket, pvData, iDataLen)) != - 1 ) {
            break;
        }

        /* Handle the error */
        if ( (errno == EAGAIN) || (errno == EWOULDBLOCK) ) {
            /* The socket would block, which means the other end is not ready to receive */
            *ppvDataEndPtr = pvData;
            return (UTL_SocketWouldBlock);
        }
        else if ( errno == EINTR ) {
            /* The socket was interrupted, so we loop and try again */
            ;
        }
        else if ( errno == EPIPE ) {
            /* The socket was closed on us */
            return (UTL_SocketClosed);
        }
        else {
            /* We can't handle this error */
            return (UTL_SocketSendFailed);
        }
    }


    /* Check how much data was sent */
    if ( iDataSent == iDataLen ) {
        /* All the data was sent */
        *ppvDataEndPtr = (void *)((unsigned char *)pvData + iDataSent);
        return (UTL_NoError);
    }
    else {
        /* The data was partially sent - this happens when the socket is non-blocking */
        *ppvDataEndPtr = (void *)((unsigned char *)pvData + iDataSent);
        return (UTL_SocketPartialSend);
    }

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlSocketReceiveTcp()

    Purpose:    This function receives a number of bytes from a tcp socket.

                If the socket is set to blocking, this function will block until all
                the data is received and return UTL_NoError, or return an error if one
                occured. If UTL_SocketPartialReceive is returned and the returned 
                pointer is equal to the passed buffer, then the socket has been closed 
                on us.
                
                If the socket is set to non-blocking, this function will return either
                UTL_NoError if all the data was received, or UTL_SocketWouldBlock if 
                nothing could be received, or UTL_SocketPartialReceive if only some 
                of the data could be received. 
                
                In either case the ppvDataEndPtr return pointer will be set to point to 
                the next byte in the buffer after the last byte that was received, assuming
                that the return pointer is provided.
                
                Signals interrupting the socket are handled by this function, so dont need
                to be handled by the calling function.

    Parameters: iSocket         socket descriptor
                pvData          buffer to read bytes into
                iDataLen        number of bytes to read
                ppvDataEndPtr   return pointer pointing to the byte following 
                                the last byte sent 

    Globals:    none

    Returns:    UTL error code

*/
int iUtlSocketReceiveTcp
(
    int iSocket,
    void *pvData,
    int iDataLen,
    void **ppvDataEndPtr
)
{

    int     iDataRead = 0;


    /* Check the parameters */
    if ( iSocket < 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'iSocket' parameter passed to 'iUtlSocketReceiveTcp'."); 
        return (UTL_SocketInvalidSocket);
    }

    if ( pvData == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'iSocket' parameter passed to 'iUtlSocketReceiveTcp'."); 
        return (UTL_SocketInvalidData);
    }

    if ( iDataLen <= 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'iSocket' parameter passed to 'iUtlSocketReceiveTcp'."); 
        return (UTL_SocketInvalidData);
    }

    if ( ppvDataEndPtr == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppvDataEndPtr' parameter passed to 'iUtlSocketReceiveTcp'."); 
        return (UTL_ReturnParameterError);
    }


    /* Loop forever, we control the loop from inside */
    while ( true ) {
    
        /* Clear the global error */
        errno = EOK;

        /* Receive the data, break out if it succeeds - recv() works on sockets only, 
        ** while read() works on sockets and stdio 
        */
/*         if ( (iDataRead = s_recv(iSocket, pvData, iDataLen, 0)) != - 1 ) { */
        if ( (iDataRead = s_read(iSocket, pvData, iDataLen)) != - 1 ) {
            break;
        }

        /* Handle the error */    
        if ( (errno == EAGAIN) || (errno == EWOULDBLOCK) ) {
            /* The socket would block, which means the other end is not ready to send */
            return (UTL_SocketWouldBlock);
        }
        else if ( errno == EINTR ) {
            /* The socket was interrupted, so we loop and try again */
            ;
        }
        else if ( errno == EPIPE ) {
            /* The socket was closed on us */
            return (UTL_SocketClosed);
        }
        else {
            /* We cant handle this error */
            return (UTL_SocketReceiveFailed);
        }
    }


    /* Check how much data was received */
    if ( iDataRead == 0 ) {
        /* Socket closed on us */
        return (UTL_SocketClosed);
    }
    else if ( iDataRead == iDataLen ) {
        /* All the data was read */
        *ppvDataEndPtr = (void *)((unsigned char *)pvData + iDataRead);
        return (UTL_NoError);
    }
    else {
        /* The data was partially read - this happens when the socket is non-blocking */
        *ppvDataEndPtr = (void *)((unsigned char *)pvData + iDataRead);
        return (UTL_SocketPartialReceive);
    }

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlSocketSendUdp()

    Purpose:    This function sends a number of bytes to a udp socket. 

                The ppvDataEndPtr return pointer will be set to point to 
                the next byte in the buffer after the last byte that was 
                sent, assuming that the return pointer is provided.
                
                Signals interrupting the socket are handled by this function, 
                so dont need to be handled by the calling function.

    Parameters: iSocket             socket descriptor
                pvData              buffer to send bytes from
                iDataLen            number of bytes to send
                psiSocketAddress    socket address to send the data to  (optional)
                ppvDataEndPtr       return pointer pointing to the byte following 
                                    the last byte sent  (optional)

    Globals:    none

    Returns:    UTL error code

*/
int iUtlSocketSendUdp
(
    int iSocket,
    void *pvData,
    int iDataLen,
    struct sockaddr_in *psiSocketAddress,
    void **ppvDataEndPtr
)
{

    int     iDataSent = 0;
        

    /* Check the parameters */
    if ( iSocket < 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'iSocket' parameter passed to 'iUtlSocketSendUdp'."); 
        return (UTL_SocketInvalidSocket);
    }

    if ( pvData == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvData' parameter passed to 'iUtlSocketSendUdp'."); 
        return (UTL_SocketInvalidData);
    }

    if ( iDataLen <= 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'iDataLen' parameter passed to 'iUtlSocketSendUdp'."); 
        return (UTL_SocketInvalidData);
    }


    /* Loop forever, we control the loop from inside */
    while ( true ) {
    
        /* Clear the global error */
        errno = EOK;

        /* Send the data, note that psiSocketAddress could be NULL, break out if we succeeded */
        if ( (iDataSent = sendto(iSocket, pvData, iDataLen, 0, (struct sockaddr *)psiSocketAddress, (psiSocketAddress != NULL) ? sizeof(struct sockaddr_in) : 0)) != -1 ) {
            break;
        }

        /* Handle the error */
        if ( (errno == EAGAIN) || (errno == EWOULDBLOCK) ) {
            /* The socket would block, which means the other end is not ready to receive */
            if ( ppvDataEndPtr != NULL ) {
                *ppvDataEndPtr = pvData;
            }
            return (UTL_SocketWouldBlock);
        }
        else if ( errno == EINTR ) {
            /* The socket was interrupted, so we loop and try again */
            ;
        }
        else if ( errno == EPIPE ) {
            /* The socket was closed on us */
            return (UTL_SocketClosed);
        }        
        else {
            /* We can't handle this error */
            return (UTL_SocketSendFailed);
        }
    }


    /* Check how much data was send */
    if ( iDataSent == iDataLen ) {
        /* All the data was sent */
        if ( ppvDataEndPtr != NULL ) {
            *ppvDataEndPtr = (void *)((unsigned char *)pvData + iDataSent);
        }
        return (UTL_NoError);
    }
    else {
        /* The data was partially sent - this happens when the socket is non-blocking */
        if ( ppvDataEndPtr != NULL ) {
            *ppvDataEndPtr = (void *)((unsigned char *)pvData + iDataSent);
        }
        return (UTL_SocketPartialSend);
    }

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlSocketReceiveUdp()

    Purpose:    This function receives a number of bytes from a udp socket.

                The ppvDataEndPtr return pointer will be set to point to 
                the next byte in the buffer after the last byte that was 
                received, assuming that the return pointer is provided.
                
                Signals interrupting the socket are handled by this function, 
                so dont need to be handled by the calling function.

    Parameters: iSocket             socket descriptor
                pvData              buffer to read bytes into
                iDataLen            number of bytes to read
                ppvDataEndPtr       return pointer pointing to the byte following 
                                    the last byte sent (optional)
                ppsiSocketAddress   return pointer for the socket address the udp 
                                    packet came from (optional)

    Globals:    none

    Returns:    UTL error code

*/
int iUtlSocketReceiveUdp
(
    int iSocket,
    void *pvData,
    int iDataLen,
    void **ppvDataEndPtr,
    struct sockaddr_in **ppsiSocketAddress
)
{

    int                     iDataRead = 0;
    struct sockaddr_in      *psiSocketAddress = NULL;
    socklen_t               zSocketAddressLen = 0;


    /* Check the parameters */
    if ( iSocket < 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'iSocket' parameter passed to 'iUtlSocketReceiveUdp'."); 
        return (UTL_SocketInvalidSocket);
    }

    if ( pvData == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'iSocket' parameter passed to 'iUtlSocketReceiveUdp'."); 
        return (UTL_SocketInvalidData);
    }

    if ( iDataLen <= 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'iSocket' parameter passed to 'iUtlSocketReceiveUdp'."); 
        return (UTL_SocketInvalidData);
    }


    /* Allocate a socket address structure to hold the client address from which we received the packet */
    if ( ppsiSocketAddress != NULL ) {
        if ( (psiSocketAddress = (struct sockaddr_in *)s_malloc(sizeof(struct sockaddr_in))) == NULL ) {
            return (UTL_MemError);
        }
        zSocketAddressLen = sizeof(struct sockaddr_in);
    }


    /* Loop forever, we control the loop from inside */
    while ( true ) {
    
        /* Clear the global error */
        errno = EOK;

        /* Receive the data, note that psiSocketAddress could be NULL, break out if we succeeded */
        if ( (iDataRead = recvfrom(iSocket, pvData, iDataLen, 0, (struct sockaddr *)psiSocketAddress, &zSocketAddressLen)) != -1 ) {
            break;
        }

        /* Handle the error */
        if ( (errno == EAGAIN) || (errno == EWOULDBLOCK) ) {
            /* The socket would block, which means the other end is not ready to read */
            s_free(psiSocketAddress);
            return (UTL_SocketWouldBlock);
        }
        else if ( errno == EINTR ) {
            /* The socket was interrupted, so we loop and try again */
            ;
        }
        else if ( errno == EPIPE ) {
            /* The socket was closed on us */
            return (UTL_SocketClosed);
        }
        else {
            /* We cant handle this error */
            s_free(psiSocketAddress);
            return (UTL_SocketReceiveFailed);
        }
    }


    /* Check how much data was received */
    if ( iDataRead == 0 ) {
        /* Socket closed on us */
        return (UTL_SocketClosed);
    }
    else if ( iDataRead == iDataLen ) {
        /* All the data was read */
        if ( ppvDataEndPtr != NULL ) {
            *ppvDataEndPtr = (void *)((unsigned char *)pvData + iDataRead);
        }
        if ( ppsiSocketAddress != NULL ) {
            *ppsiSocketAddress = psiSocketAddress;
        }
        return (UTL_NoError);
    }
    else {
        /* The data was partially read - this happens when the socket is non-blocking */
        if ( ppvDataEndPtr != NULL ) {
            *ppvDataEndPtr = (void *)((unsigned char *)pvData + iDataRead);
        }
        if ( ppsiSocketAddress != NULL ) {
            *ppsiSocketAddress = psiSocketAddress;
        }
        return (UTL_SocketPartialReceive);
    }

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlSocketGetClientNameFromSocket()

    Purpose:    This function returns the host name of the client 
                connected to the socket

    Parameters: iSocket             socket descriptor
                pucClientName       return pointer for the client name
                uiClientNameLen     length of the pointer

    Globals:    none

    Returns:    UTL error code

*/
int iUtlSocketGetClientNameFromSocket
(
    int iSocket,
    unsigned char *pucClientName, 
    unsigned int uiClientNameLen
)
{

    struct sockaddr_in      siSocketAddress;
    int                     lSocketAddressSize = sizeof(siSocketAddress);
    struct hostent          *phePeer = NULL;


    /* Check the parameters */
    if ( iSocket < 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'iSocket' parameter passed to 'iUtlSocketGetClientNameFromSocket'."); 
        return (UTL_SocketInvalidSocket);
    }

    if ( pucClientName == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pucClientName' parameter passed to 'iUtlSocketGetClientNameFromSocket'."); 
        return (UTL_ReturnParameterError);
    }

    if ( uiClientNameLen <= 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiClientNameLen' parameter passed to 'iUtlSocketGetClientNameFromSocket'."); 
        return (UTL_ReturnParameterError);
    }


    /* Get the socket address information for the socket descriptor */
    if ( getpeername(iSocket, (struct sockaddr *)&siSocketAddress, &lSocketAddressSize) != 0 ) {
        return (UTL_SocketGetClientNameFailed);
    }
    
    /* The host address information from the socket address information */
    if ( (phePeer = gethostbyaddr((void *)&siSocketAddress.sin_addr, 4, AF_INET)) == NULL ) {
        return (UTL_SocketGetClientNameFailed);
    }
    
    /* Copy over the client name */
    s_strnncpy(pucClientName, phePeer->h_name, uiClientNameLen);


    return (UTL_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlSocketGetClientAddressFromSocket()

    Purpose:    This function returns the host address of the client 
                connected to the socket

    Parameters: iSocket                 socket descriptor
                pucClientAddress        return pointer for the client address
                uiClientAddressLen      length of the pointer

    Globals:    none

    Returns:    UTL error code

*/
int iUtlSocketGetClientAddressFromSocket
(
    int iSocket,
    unsigned char *pucClientAddress,
    unsigned int uiClientAddressLen
)
{

    struct sockaddr_in      siSocketAddress;
    int                     lSocketAddressSize = sizeof(siSocketAddress);


    /* Check the parameters */
    if ( iSocket < 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'iSocket' parameter passed to 'iUtlSocketGetClientAddressFromSocket'."); 
        return (UTL_SocketInvalidSocket);
    }

    if ( pucClientAddress == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pucClientAddress' parameter passed to 'iUtlSocketGetClientAddressFromSocket'."); 
        return (UTL_ReturnParameterError);
    }

    if ( uiClientAddressLen <= 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiClientAddressLen' parameter passed to 'iUtlSocketGetClientAddressFromSocket'."); 
        return (UTL_ReturnParameterError);
    }


    /* Get the socket address information for the socket descriptor */
    if ( getpeername(iSocket, (struct sockaddr *)&siSocketAddress, &lSocketAddressSize) != 0 ) {
        return (UTL_SocketGetClientAddressFailed);
    }
    
    /* Copy over the client address */
    s_strnncpy(pucClientAddress, inet_ntoa(siSocketAddress.sin_addr), uiClientAddressLen);


    return (UTL_NoError);

}



/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlSocketGetClientNameFromSocketAddress()

    Purpose:    This function returns the host name of the client 
                connected to the socket address

    Parameters: psiSocketAddress    socket address
                pucClientName       return pointer for the client name
                uiClientNameLen     length of the pointer

    Globals:    none

    Returns:    UTL error code

*/
int iUtlSocketGetClientNameFromSocketAddress
(
    struct sockaddr_in *psiSocketAddress,
    unsigned char *pucClientName, 
    unsigned int uiClientNameLen
)
{

    struct hostent      *phePeer = NULL;


    /* Check the parameters */
    if ( psiSocketAddress == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'psiSocketAddress' parameter passed to 'iUtlSocketGetClientNameFromSocketAddress'."); 
        return (UTL_SocketInvalidSocketAddress);
    }

    if ( pucClientName == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pucClientName' parameter passed to 'iUtlSocketGetClientNameFromSocketAddress'."); 
        return (UTL_ReturnParameterError);
    }

    if ( uiClientNameLen <= 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiClientNameLen' parameter passed to 'iUtlSocketGetClientNameFromSocketAddress'."); 
        return (UTL_ReturnParameterError);
    }


    /* The host address information from the socket address information */
    if ( (phePeer = gethostbyaddr((void *)&psiSocketAddress->sin_addr, 4, AF_INET)) == NULL ) {
        return (UTL_SocketGetClientNameFailed);
    }
    
    /* Copy over the client name */
    s_strnncpy(pucClientName, phePeer->h_name, uiClientNameLen);


    return (UTL_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlSocketGetClientAddressFromSocketAddress()

    Purpose:    This function returns the host address of the client 
                connected to the socket address

    Parameters: psiSocketAddress        socket address
                pucClientAddress        return pointer for the client address
                uiClientAddressLen      length of the pointer

    Globals:    none

    Returns:    UTL error code

*/
int iUtlSocketGetClientAddressFromSocketAddress
(
    struct sockaddr_in *psiSocketAddress,
    unsigned char *pucClientAddress,
    unsigned int uiClientAddressLen
)
{


    /* Check the parameters */
    if ( psiSocketAddress == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'psiSocketAddress' parameter passed to 'iUtlSocketGetClientAddressFromSocketAddress'."); 
        return (UTL_SocketInvalidSocketAddress);
    }

    if ( pucClientAddress == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pucClientAddress' parameter passed to 'iUtlSocketGetClientAddressFromSocketAddress'."); 
        return (UTL_ReturnParameterError);
    }

    if ( uiClientAddressLen <= 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiClientAddressLen' parameter passed to 'iUtlSocketGetClientAddressFromSocketAddress'."); 
        return (UTL_ReturnParameterError);
    }


    /* Copy over the client address */
    s_strnncpy(pucClientAddress, inet_ntoa(psiSocketAddress->sin_addr), uiClientAddressLen);


    return (UTL_NoError);

}



/*---------------------------------------------------------------------------*/


