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

    Module:     socket.h

    Author:     Francois Schiettecatte (FS Consulting LLC.)

    Created:    16 February 1994

    Purpose:    This contains the header for socket.c

*/


/*---------------------------------------------------------------------------*/


#if !defined(UTL_SOCKET_H)
#define UTL_SOCKET_H


/*---------------------------------------------------------------------------*/


/*
** Includes
*/

#include "utils.h"


/*---------------------------------------------------------------------------*/


/* 
** C++ wrapper
*/

#if defined(__cplusplus)
extern "C" {
#endif    /* defined(__cplusplus) */


/*---------------------------------------------------------------------------*/


/*
** Defines
*/

/* Maximum connection backlog  - default to 128 (this is also the minimum) */
#if !defined(SOMAXCONN) || (SOMAXCONN < 128)
#undef SOMAXCONN
#define SOMAXCONN                           (128)
#endif    /* !defined(SOMAXCONN) || (SOMAXCONN < 64) */


/* Local host name and address */
#define UTL_SOCKET_LOCALHOST_NAME           (unsigned char *)"localhost"
#define UTL_SOCKET_LOCALHOST_ADDRESS        (unsigned char *)"127.0.0.1"


/* Maximum of queued connections allowable on each port for a tcp socket */
#define UTL_SOCKET_BACKLOG_MAX              (SOMAXCONN)
#define UTL_SOCKET_BACKLOG_DEFAULT          (UTL_SOCKET_BACKLOG_MAX)


/* Default buffer length */
#define UTL_SOCKET_BUFFER_LENGTH_DEFAULT    (0)


/* Maximum number of retries when opening a connection */
#define UTL_SOCKET_RETRIES_MAX              (9)
#define UTL_SOCKET_RETRIES_DEFAULT          (UTL_SOCKET_RETRIES_MAX)


/* Retry interval when opening a connection, 100 microseconds (0.1 millisecond) */
#define UTL_SOCKET_RETRY_INTERVAL_DEFAULT   (100)


/* Used for iUtlSocketOpenTcp() to indicate whether the socket blocks or 
** not, this is just to make the code a bit more readable
*/
#define UTL_SOCKET_BLOCKING                 (true)
#define UTL_SOCKET_NON_BLOCKING             (false)


/*---------------------------------------------------------------------------*/


/*
** Public function prototypes
*/

int iUtlSocketOpenServerTcp (unsigned char *pucHostName, int iPort, 
        unsigned int uiBufferLength, unsigned int uiBacklog, 
        unsigned int uiRetries, unsigned int uiRetryInterval, 
        boolean bBlocking, int *piSocket);

int iUtlSocketAcceptClientTcp (int iSocket, int *piSocket);

int iUtlSocketOpenTcp (unsigned char *pucHostName, int iPort, 
        unsigned int uiBufferLength, unsigned int uiRetries, 
        unsigned int uiRetryInterval, boolean bBlocking, int *piSocket);

int iUtlSocketOpenServerUdp (unsigned char *pucHostName, int iPort, 
        unsigned int uiBufferLength, unsigned int uiRetries, 
        unsigned int uiRetryInterval, boolean bBlocking, int *piSocket);

int iUtlSocketOpenUdp (unsigned char *pucHostName, int iPort, 
        unsigned int uiBufferLength, unsigned int uiRetries, 
        unsigned int uiRetryInterval, boolean bBlocking, int *piSocket);

int iUtlSocketClose (int iSocket);

int iUtlSocketSetBlockingIO (int iSocket);
int iUtlSocketSetNonBlockingIO (int iSocket);

int iUtlSocketGetSendBufferLength (int iSocket, unsigned int *puiBufferLength);
int iUtlSocketGetReceiveBufferLength (int iSocket, unsigned int *puiBufferLength);
int iUtlSocketSetSendBufferLength (int iSocket, unsigned int uiBufferLength);
int iUtlSocketSetReceiveBufferLength (int iSocket, unsigned int uiBufferLength);

int iUtlSocketGetError (int iSocket);

int iUtlSocketReadyToSend (int iSocket, int iTimeOut);
int iUtlSocketReadyToReceive (int iSocket, int iTimeOut);

int iUtlSocketSendTcp (int iSocket, void *pvData, int iDataLen, 
        void **ppvDataEndPtr);
int iUtlSocketReceiveTcp (int iSocket, void *pvData, int iDataLen, 
        void **ppvDataEndPtr);

int iUtlSocketSendUdp (int iSocket, void *pvData, int iDataLen, 
        struct sockaddr_in *psiSocketAddress, void **ppvDataEndPtr);
int iUtlSocketReceiveUdp (int iSocket, void *pvData, int iDataLen, 
        void **ppvDataEndPtr, struct sockaddr_in **ppsiSocketAddress);

int iUtlSocketGetClientNameFromSocket (int iSocket, unsigned char *pucClientName, 
        unsigned int uiClientNameLen);
int iUtlSocketGetClientAddressFromSocket (int iSocket, unsigned char *pucClientAddress, 
        unsigned int uiClientAddressLen);

int iUtlSocketGetClientNameFromSocketAddress (struct sockaddr_in *psiSocketAddress, 
        unsigned char *pucClientName, unsigned int uiClientNameLen);
int iUtlSocketGetClientAddressFromSocketAddress (struct sockaddr_in *psiSocketAddress, 
        unsigned char *pucClientAddress, unsigned int uiClientAddressLen);


/*---------------------------------------------------------------------------*/


/* 
** C++ wrapper
*/

#if defined(__cplusplus)
}
#endif    /* defined(__cplusplus) */


/*---------------------------------------------------------------------------*/


#endif    /* !defined(UTL_SOCKET_H) */


/*---------------------------------------------------------------------------*/



