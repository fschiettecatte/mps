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

    Module:     net.h

    Author:     Francois Schiettecatte (FS Consulting LLC.)

    Created:    29 June 2006

    Purpose:    This contains the header for net.c

*/


/*---------------------------------------------------------------------------*/


#if !defined(UTL_NET_H)
#define UTL_NET_H


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

/* Protocols */
#define UTL_NET_PROTOCOL_INVALID_ID             (0)
#define UTL_NET_PROTOCOL_UDP_ID                 (1)
#define UTL_NET_PROTOCOL_TCP_ID                 (2)
#define UTL_NET_PROTOCOL_STDIO_ID               (3)

#define UTL_NET_PROTOCOL_VALID_ID(n)            (((n) >= UTL_NET_PROTOCOL_UDP_ID) && \
                                                        ((n) <= UTL_NET_PROTOCOL_STDIO_ID))


/* Protocol name */
#define UTL_NET_PROTOCOL_UDP_NAME               (unsigned char *)"udp"
#define UTL_NET_PROTOCOL_TCP_NAME               (unsigned char *)"tcp"
#define UTL_NET_PROTOCOL_STDIO_NAME             (unsigned char *)"stdio"


/* Protocol types */
#define UTL_NET_PROTOCOL_TYPE_INVALID_ID        (0)
#define UTL_NET_PROTOCOL_TYPE_MESSAGE_ID        (1)
#define UTL_NET_PROTOCOL_TYPE_SESSION_ID        (2)

#define UTL_NET_PROTOCOL_TYPE_VALID_ID(n)       (((n) >= UTL_NET_PROTOCOL_TYPE_MESSAGE_ID) && \
                                                        ((n) <= UTL_NET_PROTOCOL_TYPE_SESSION_ID))


/* Maximum host name length  - default to 64 (this is also the minimum) */
#if !defined(MAXHOSTNAMELEN) || (MAXHOSTNAMELEN < 64)
#undef MAXHOSTNAMELEN
#define MAXHOSTNAMELEN                          (64)
#endif    /* !defined(MAXHOSTNAMELEN) || (MAXHOSTNAMELEN < 64) */


/*---------------------------------------------------------------------------*/


/*
** Public function prototypes
*/

/* Create a server */
int iUtlNetCreateServer (unsigned int uiNetworkProtocolID, unsigned char *pucHostName, 
        int iPort, int iTimeOut, void **ppvUtlNet);    

/* Create a server service */
int iUtlNetCreateServerService (int iTimeOut, void **ppvUtlNet);    

/* Add a protocol to a server */
int iUtlNetAddServerProtocol (void *pvUtlNet, unsigned int uiNetworkProtocolID, 
        unsigned char *pucHostName, int iPort);    


/* Create a client */
int iUtlNetCreateClient (unsigned int uiNetworkProtocolID, unsigned char *pucHostName, 
        int iPort, int iTimeOut, void **ppvUtlNet);    


/* Duplicate a net structure, all data is copied */
int iUtlNetDuplicate (void *pvUtlNet, void **ppvUtlNet);


/* Set timeout */
int iUtlNetSetTimeOut (void *pvUtlNet, int iTimeOut);


/* Receive the message (also accepts the client if needed) */
int iUtlNetReceive (void *pvUtlNet);

/* Read data */
int iUtlNetPeek (void *pvUtlNet, unsigned int uiBufferLength, unsigned char **ppucBuffer);
int iUtlNetSkip (void *pvUtlNet, unsigned int uiBufferLength);
int iUtlNetRead (void *pvUtlNet, unsigned char *pucBuffer, unsigned int uiBufferLength);


/* Write data */
int iUtlNetWrite (void *pvUtlNet, unsigned char *pucBuffer, unsigned int uiBufferLength);

/* Send the message */
int iUtlNetSend (void *pvUtlNet);


/* Check states */
int iUtlNetReadyToSend (void *pvUtlNet, int iTimeOut);
int iUtlNetReadyToReceive (void *pvUtlNet, int iTimeOut);


/* Get error state */
int iUtlNetGetWriteError (void *pvUtlNet);
int iUtlNetGetReadError (void *pvUtlNet);


/* Get message buffer lengths */
int iUtlNetGetReceiveBufferDataLength (void *pvUtlNet, unsigned int *puiBufferLength);
int iUtlNetGetSendBufferDataLength (void *pvUtlNet, unsigned int *puiBufferLength);


/* Reset (clear) message buffers */
int iUtlNetResetReceive (void *pvUtlNet);
int iUtlNetResetSend (void *pvUtlNet);


/* Get the connection status */
int iUtlNetGetConnectionStatus (void *pvUtlNet, boolean *pbConnected);

/* Get the connected protocol */
int iUtlNetGetConnectedProtocol (void *pvUtlNet, unsigned int *puiProtocol);

/* Get the connected protocol type */
int iUtlNetGetConnectedProtocolType (void *pvUtlNet, unsigned int *puiProtocolType);

/* Get the connected client name/address */
int iUtlNetGetConnectedClientName (void *pvUtlNet, unsigned char *pucClientName, 
        unsigned int uiClientNameLen);
int iUtlNetGetConnectedClientAddress (void *pvUtlNet, unsigned char *pucClientAddress, 
        unsigned int uiClientAddressLen);

/* Get the connected host name/address */
int iUtlNetGetConnectedHostName (void *pvUtlNet, unsigned char *pucHostName, 
        unsigned int uiHostNameLen);
int iUtlNetGetConnectedHostAddress (void *pvUtlNet, unsigned char *pucHostAddress, 
        unsigned int uiHostAddressLen);

/* Get the connected port */
int iUtlNetGetConnectedPort (void *pvUtlNet, int *piPort);


/* Close */
int iUtlNetCloseClient (void *pvUtlNet);


/* Free */
int iUtlNetFree (void *pvUtlNet);


/* Get information */
int iUtlNetGetHostName (unsigned char *pucHostName, unsigned int uiHostNameLen);
int iUtlNetGetHostAddress (unsigned char *pucHostAddress, unsigned int uiHostAddressLen);

int iUtlNetConvertStringHostAddress (unsigned char *pucHostAddress, unsigned int *puiHostAddress);
int iUtlNetConvertNumericHostAddress (unsigned int uiHostAddress, 
        unsigned char *pucHostAddress, unsigned int uiHostAddressLen);

int iUtlNetGetNetMaskStringHostAddress (unsigned char *pucHostAddress, 
        unsigned char *pucNetMask, unsigned int uiNetMaskLen);
int iUtlNetGetNetMaskNumericHostAddress (unsigned int uiHostAddress, 
        unsigned int *puiNetMask);
int iUtlNetGetNetMaskBitsStringHostAddress (unsigned char *pucHostAddress, 
        unsigned int *puiNetMaskBits);
int iUtlNetGetNetMaskBitsNumericHostAddress (unsigned int uiHostAddress, 
        unsigned int *puiNetMaskBits);


/*---------------------------------------------------------------------------*/


/* 
** C++ wrapper
*/

#if defined(__cplusplus)
}
#endif    /* defined(__cplusplus) */


/*---------------------------------------------------------------------------*/


#endif    /* !defined(UTL_NET_H) */


/*---------------------------------------------------------------------------*/



