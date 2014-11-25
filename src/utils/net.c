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

    Module:     net.c

    Author:     Francois Schiettecatte (FS Consulting LLC.)

    Created:    29 June 2006

    Purpose:    This module implements buffered tcp communications.

*/


/*---------------------------------------------------------------------------*/

/*
** Includes
*/

#include "utils.h"


/*---------------------------------------------------------------------------*/


/*
** Feature defines
*/

/* Context for logging */
#undef UTL_LOG_CONTEXT
#define UTL_LOG_CONTEXT                     (unsigned char *)"com.fsconsult.mps.src.utils.net"


/*---------------------------------------------------------------------------*/


/*
** Defines
*/

/* Set the net buffer lengths */
#define UTL_NET_BUFFER_LENGTH_UDP               (128 * 1024)
#define UTL_NET_BUFFER_LENGTH_TCP               (10 * 1024)
#define UTL_NET_BUFFER_LENGTH_STDIO             (10 * 1024)


/* Net service */
#define UTL_NET_SERVICE_INVALID                 (0)
#define UTL_NET_SERVICE_SERVER                  (1)
#define UTL_NET_SERVICE_CLIENT                  (2)

#define UTL_NET_SERVICE_VALID(n)                (((n) >= UTL_NET_SERVICE_SERVER) && \
                                                        ((n) <= UTL_NET_SERVICE_CLIENT))


/* Host name and address defaults */
#define UTL_NET_HOST_NAME_DEFAULT               UTL_SOCKET_LOCALHOST_NAME
#define UTL_NET_HOST_ADDRESS_DEFAULT            UTL_SOCKET_LOCALHOST_ADDRESS


/*---------------------------------------------------------------------------*/


/*
** Structures
*/

/* Net protocol structure
**
** TCP:
**  iServerSocket                       server socket, -1 for the client
**  iReadSocket/iWriteSocket            client socket when connected, -1 when disconnected
**
** UDP:
**  iServerSocket                       -1
**  iReadSocket/iWriteSocket            socket
**
** STDIO:
**  iServerSocket                       -1
**  iReadSocket                         stdin
**  iWriteSocket                        stdout
**
*/
struct utlNetProtocol {
    unsigned int            uiNetworkProtocolID;
    unsigned char           *pucHostName;
    int                     iPort;
    int                     iServerSocket;
    int                     iReadSocket;
    int                     iWriteSocket;
    unsigned int            uiReceiveBufferLength;
    unsigned int            uiSendBufferLength;
    struct sockaddr_in      *psiSocketAddress;
};


/* Net structure */
struct utlNet {

    /* Info */
    unsigned int            uiService;
    int                     iTimeOut;
    
    /* Net protocols */
    struct utlNetProtocol   *punpNetProtocols;
    unsigned int            uiNetProtocolsLength;
    struct utlNetProtocol   *punpNetProtocolsPtr;

    /* Buffer management */
    unsigned char           *pucReceiveBuffer;
    unsigned int            uiReceiveBufferLength;
    unsigned char           *pucReceiveBufferStartPtr;
    unsigned char           *pucReceiveBufferEndPtr;

    unsigned char           *pucSendBuffer;
    unsigned int            uiSendBufferLength;
    unsigned char           *pucSendBufferEndPtr;
    
};


/*---------------------------------------------------------------------------*/


/*
** Private function prototypes
*/

static int iUtlNetCreateService (unsigned int uiService, int iTimeOut, struct utlNet **ppunNet);

static int iUtlNetAddProtocol (struct utlNet *punNet, unsigned int uiNetworkProtocolID,
        unsigned char *pucHostName, int iPort);

static int iUtlNetAdjustBuffers (struct utlNet *punNet);

static int iUtlNetSendUdp (struct utlNet *punNet);

static int iUtlNetReceiveUdp (struct utlNet *punNet);

static int iUtlNetSendTcp (struct utlNet *punNet, unsigned char *pucBuffer, 
        unsigned int uiBufferLength);

static int iUtlNetReceiveTcp (struct utlNet *punNet, unsigned char *pucBuffer, 
        unsigned int uiBufferLength);

static int iUtlMapSocketError (int iSocketError, int iDefaultNetError);

static int iUtlNetPrint (struct utlNet *punNet);


/*---------------------------------------------------------------------------*/

/*

    Function:   iUtlNetCreateServer()

    Purpose:    Create a new net server structure and add the protocol

    Parameters: uiNetworkProtocolID     Network protocol ID
                pucHostName             Host name
                iPort                   Port
                iTimeOut                Millisecond timeout 
                                        (0 = return at once, -1 = block until ready)
                ppvUtlNet               Return pointer for the net structure

    Globals:    none

    Returns:    UTL error code

*/
int iUtlNetCreateServer
(
    unsigned int uiNetworkProtocolID, 
    unsigned char *pucHostName, 
    int iPort, 
    int iTimeOut, 
    void **ppvUtlNet
)
{

    int             iError = UTL_NoError;
    struct utlNet   *punNet = NULL;
    
    
/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iUtlNetCreateServer - uiNetworkProtocolID: [%u], pucHostName: [%s], iPort: [%d]",  */
/*             uiNetworkProtocolID, pucUtlStringsGetPrintableString(pucHostName), iPort); */


    /* Check the parameters */
    if ( UTL_NET_PROTOCOL_VALID_ID(uiNetworkProtocolID) == false ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiNetworkProtocolID' parameter passed to 'iUtlNetCreateServer'."); 
        return (UTL_NetInvalidProtocol);
    }

    if ( (iPort < 0) && ((uiNetworkProtocolID == UTL_NET_PROTOCOL_UDP_ID) || (uiNetworkProtocolID == UTL_NET_PROTOCOL_TCP_ID)) ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'iPort' parameter passed to 'iUtlNetCreateServer'."); 
        return (UTL_NetInvalidPort);
    }

    if ( iTimeOut < -1 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'iTimeOut' parameter passed to 'iUtlNetCreateServer'."); 
        return (UTL_NetInvalidTimeOut);
    }

    if ( ppvUtlNet == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppvUtlNet' parameter passed to 'iUtlNetCreateServer'."); 
        return (UTL_ReturnParameterError);
    }



    /* Create the server service */
    if ( (iError = iUtlNetCreateService(UTL_NET_SERVICE_SERVER, iTimeOut, &punNet)) != UTL_NoError ) {
        goto bailFromiUtlNetCreateServer;
    }


    /* Add the protocol */
    if ( (iError = iUtlNetAddProtocol(punNet, uiNetworkProtocolID, pucHostName, iPort)) != UTL_NoError ) {
        goto bailFromiUtlNetCreateServer;
    }


    /* Adjust the buffers */
    if ( (iError = iUtlNetAdjustBuffers(punNet)) != UTL_NoError ) {
        goto bailFromiUtlNetCreateServer;
    }


    /* Bail label */
    bailFromiUtlNetCreateServer:


    /* Handle the error */
    if ( iError == UTL_NoError ) {

        /* Set the return pointer */
        *ppvUtlNet = punNet;
    }
    else {

        /* Free allocations */
        iUtlNetFree((void *)punNet);
    }
    
    
    return (iError);

}


/*---------------------------------------------------------------------------*/

/*

    Function:   iUtlNetCreateServerService()

    Purpose:    Create a new net server structure, with no protocols

    Parameters: iTimeOut    Millisecond timeout 
                            (0 = return at once, -1 = block until ready)
                ppvUtlNet   Return pointer for the net structure

    Globals:    none

    Returns:    UTL error code

*/
int iUtlNetCreateServerService
(
    int iTimeOut, 
    void **ppvUtlNet
)
{

    int             iError = UTL_NoError;
    struct utlNet   *punNet = NULL;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iUtlNetCreateServerService"); */


    /* Check the parameters */
    if ( iTimeOut < -1 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'iTimeOut' parameter passed to 'iUtlNetCreateServerService'."); 
        return (UTL_NetInvalidTimeOut);
    }

    if ( ppvUtlNet == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppvUtlNet' parameter passed to 'iUtlNetCreateServerService'."); 
        return (UTL_ReturnParameterError);
    }


    /* Create the server service */
    if ( (iError = iUtlNetCreateService(UTL_NET_SERVICE_SERVER, iTimeOut, &punNet)) != UTL_NoError ) {
        goto bailFromiUtlNetCreateServerService;
    }



    /* Bail label */
    bailFromiUtlNetCreateServerService:


    /* Handle the error */
    if ( iError == UTL_NoError ) {

        /* Set the return pointer */
        *ppvUtlNet = punNet;
    }
    else {

        /* Free allocations */
        iUtlNetFree((void *)punNet);
    }
    

    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlNetAddServerProtocol()

    Purpose:    Add a protocol to the net structure (server service)

    Parameters: pvUtlNet                Net structure
                uiNetworkProtocolID     Network protocol ID
                pucHostName             Host name
                iPort                   Port

    Globals:    none

    Returns:    UTL error code

*/
int iUtlNetAddServerProtocol
(
    void *pvUtlNet,
    unsigned int uiNetworkProtocolID, 
    unsigned char *pucHostName, 
    int iPort 
)
{

    int             iError = UTL_NoError;
    struct utlNet   *punNet = (struct utlNet *)pvUtlNet;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iUtlNetAddServerProtocol - uiNetworkProtocolID: [%u], pucHostName: [%s], iPort: [%d]",  */
/*             uiNetworkProtocolID, pucUtlStringsGetPrintableString(pucHostName), iPort); */


    /* Check the parameters */
    if ( pvUtlNet == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvUtlNet' parameter passed to 'iUtlNetAddServerProtocol'."); 
        return (UTL_NetInvalidNet);
    }

    if ( UTL_NET_PROTOCOL_VALID_ID(uiNetworkProtocolID) == false ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiNetworkProtocolID' parameter passed to 'iUtlNetAddServerProtocol'."); 
        return (UTL_NetInvalidProtocol);
    }

    if ( (iPort < 0) && ((uiNetworkProtocolID == UTL_NET_PROTOCOL_UDP_ID) || (uiNetworkProtocolID == UTL_NET_PROTOCOL_TCP_ID)) ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'iPort' parameter passed to 'iUtlNetAddServerProtocol'."); 
        return (UTL_NetInvalidPort);
    }


    /* Check that we are a server service */
    if ( punNet->uiService != UTL_NET_SERVICE_SERVER ) {
        return (UTL_NetNotAServerService);
    }


    /* Add the protocol */
    if ( (iError = iUtlNetAddProtocol(punNet, uiNetworkProtocolID, pucHostName, iPort)) != UTL_NoError ) {
        return (iError);
    }

    /* Adjust the buffers */
    if ( (iError = iUtlNetAdjustBuffers(punNet)) != UTL_NoError ) {
        return (iError);
    }


    return (UTL_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlNetCreateClient()

    Purpose:    Create a new net client structure.

    Parameters: uiNetworkProtocolID     Network protocol ID
                pucHostName             Host name
                iPort                   Port
                iTimeOut                Millisecond timeout 
                                        (0 = return at once, -1 = block until ready)
                ppvUtlNet               Return pointer for the net structure

    Globals:    none

    Returns:    UTL error code

*/
int iUtlNetCreateClient
(
    unsigned int uiNetworkProtocolID, 
    unsigned char *pucHostName, 
    int iPort, 
    int iTimeOut, 
    void **ppvUtlNet
)
{

    int             iError = UTL_NoError;
    struct utlNet   *punNet = NULL;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iUtlNetCreateClient - uiNetworkProtocolID: [%u], pucHostName: [%s], iPort: [%d], iTimeOut: [%d]",  */
/*             uiNetworkProtocolID, pucUtlStringsGetPrintableString(pucHostName), iPort, iTimeOut); */


    /* Check the parameters */
    if ( UTL_NET_PROTOCOL_VALID_ID(uiNetworkProtocolID) == false ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiNetworkProtocolID' parameter passed to 'iUtlNetCreateClient'."); 
        return (UTL_NetInvalidProtocol);
    }

    if ( (bUtlStringsIsStringNULL(pucHostName) == true) && ((uiNetworkProtocolID == UTL_NET_PROTOCOL_UDP_ID) || (uiNetworkProtocolID == UTL_NET_PROTOCOL_TCP_ID)) ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pucHostName' parameter passed to 'iUtlNetCreateClient'."); 
        return (UTL_NetInvalidHostName);
    }

    if ( (iPort < 0) && ((uiNetworkProtocolID == UTL_NET_PROTOCOL_UDP_ID) || (uiNetworkProtocolID == UTL_NET_PROTOCOL_TCP_ID)) ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'iPort' parameter passed to 'iUtlNetCreateClient'."); 
        return (UTL_NetInvalidPort);
    }
    
    if ( iTimeOut < -1 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'iTimeOut' parameter passed to 'iUtlNetCreateClient'."); 
        return (UTL_NetInvalidTimeOut);
    }

    if ( ppvUtlNet == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppvUtlNet' parameter passed to 'iUtlNetCreateClient'."); 
        return (UTL_ReturnParameterError);
    }


    /* Create a client service */
    if ( (iError = iUtlNetCreateService(UTL_NET_SERVICE_CLIENT, iTimeOut, &punNet)) != UTL_NoError ) {
        goto bailFromiUtlNetCreateClient;
    }


    /* Add the protocol */
    if ( (iError = iUtlNetAddProtocol(punNet, uiNetworkProtocolID, pucHostName, iPort)) != UTL_NoError ) {
        goto bailFromiUtlNetCreateClient;
    }


    /* Adjust the buffers */
    if ( (iError = iUtlNetAdjustBuffers(punNet)) != UTL_NoError ) {
        goto bailFromiUtlNetCreateClient;
    }



    /* Bail label */
    bailFromiUtlNetCreateClient:

    
    /* Handle the error */
    if ( iError == UTL_NoError ) {

        /* Set the return pointer */
        *ppvUtlNet = punNet;
    }
    else {

        /* Free allocations */
        iUtlNetFree((void *)punNet);
    }

    
    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlNetDuplicate()

    Purpose:    Duplicate a net server structure, all data is copied

    Parameters: pvUtlNet    Net structure
                ppvUtlNet   Return pointer for the net structure

    Globals:    none

    Returns:    UTL error code

*/
int iUtlNetDuplicate
(
    void *pvUtlNet, 
    void **ppvUtlNet
)
{

    int             iError = UTL_NoError;
    struct utlNet   *punNetSource = (struct utlNet *)pvUtlNet;
    struct utlNet   *punNetTarget = NULL;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iUtlNetDuplicate"); */


    /* Check the parameters */
    if ( pvUtlNet == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvUtlNet' parameter passed to 'iUtlNetDuplicate'."); 
        return (UTL_NetInvalidNet);
    }

    if ( ppvUtlNet == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvUtlNet' parameter passed to 'iUtlNetDuplicate'."); 
        return (UTL_ReturnParameterError);
    }


    /* Allocate the net structure */
    if ( (punNetTarget = (struct utlNet *)s_malloc((size_t)(sizeof(struct utlNet)))) == NULL ) {
        return (UTL_MemError);
    }


    /* Copy the net structure fields */
    punNetTarget->uiService = punNetSource->uiService;
    punNetTarget->iTimeOut = punNetSource->iTimeOut;


    /* Copy the net protocols */
    if ( punNetSource->uiNetProtocolsLength > 0 ) {

        struct utlNetProtocol   *punsNetServicesSourcePtr = NULL;
        struct utlNetProtocol   *punsNetServicesTargetPtr = NULL;
        unsigned int            uiI = 0;

        /* Allocate the net protocols */
        if ( (punNetTarget->punpNetProtocols = (struct utlNetProtocol *)s_malloc(sizeof(struct utlNetProtocol) * punNetSource->uiNetProtocolsLength)) == NULL ) {
            iError = UTL_MemError;
            goto bailFromiUtlNetDuplicate;
        }

        /* Set the net services length */
        punNetTarget->uiNetProtocolsLength = punNetSource->uiNetProtocolsLength;

        /* Loop over the net protocols and copy the fields */
        for ( punsNetServicesSourcePtr = punNetSource->punpNetProtocols, punsNetServicesTargetPtr = punNetTarget->punpNetProtocols, uiI = 0; 
                uiI < punNetSource->uiNetProtocolsLength; punsNetServicesSourcePtr++, punsNetServicesTargetPtr++, uiI++ ) {
        
            punsNetServicesTargetPtr->uiNetworkProtocolID = punsNetServicesSourcePtr->uiNetworkProtocolID;
            punsNetServicesTargetPtr->iPort = punsNetServicesSourcePtr->iPort;

            if ( bUtlStringsIsStringNULL(punsNetServicesSourcePtr->pucHostName) == false ) {
                if ( (punsNetServicesTargetPtr->pucHostName = s_strdup(punsNetServicesSourcePtr->pucHostName)) == NULL ) {
                    iError = UTL_MemError;
                    goto bailFromiUtlNetDuplicate;
                }
            }

            punsNetServicesTargetPtr->iServerSocket = punsNetServicesSourcePtr->iServerSocket;
            punsNetServicesTargetPtr->iReadSocket = punsNetServicesSourcePtr->iReadSocket;
            punsNetServicesTargetPtr->iWriteSocket = punsNetServicesSourcePtr->iWriteSocket;
            punsNetServicesTargetPtr->uiReceiveBufferLength = punsNetServicesSourcePtr->uiReceiveBufferLength;
            punsNetServicesTargetPtr->uiSendBufferLength = punsNetServicesSourcePtr->uiSendBufferLength;

            /* Duplicate the sockaddr_in structure */
            if ( punsNetServicesSourcePtr->psiSocketAddress != NULL ) {

                /* Allocate space for the sockaddr_in structure */
                if ( (punsNetServicesTargetPtr->psiSocketAddress = (struct sockaddr_in *)s_malloc(sizeof(struct sockaddr_in))) == NULL ) {
                    iError = UTL_MemError;
                    goto bailFromiUtlNetDuplicate;
                }

                /* Copy the sockaddr_in structure */
                s_memcpy(punsNetServicesTargetPtr->psiSocketAddress, punsNetServicesSourcePtr->psiSocketAddress, sizeof(struct sockaddr_in));
            }
        }
    }


    /* Copy the receive buffer */
    if ( punNetSource->pucReceiveBuffer != NULL ) {

        unsigned int    uiReceiveBufferStartIndex = 0;
        unsigned int    uiReceiveBufferEndIndex = 0;

        /* Save the offsets */
        uiReceiveBufferStartIndex = punNetSource->pucReceiveBufferStartPtr - punNetSource->pucReceiveBuffer;
        uiReceiveBufferEndIndex = punNetSource->pucReceiveBufferEndPtr - punNetSource->pucReceiveBuffer;

        /* Set the buffer length */
        punNetTarget->uiReceiveBufferLength = punNetSource->uiReceiveBufferLength;

        /* Allocate the buffer */
        if ( (punNetTarget->pucReceiveBuffer = (unsigned char *)s_malloc((size_t)(sizeof(unsigned char) * punNetTarget->uiReceiveBufferLength))) == NULL ) {
            iError = UTL_MemError;
            goto bailFromiUtlNetDuplicate;
        }

        /* Set the pointers */
        punNetTarget->pucReceiveBufferStartPtr = punNetTarget->pucReceiveBuffer + uiReceiveBufferStartIndex;
        punNetTarget->pucReceiveBufferEndPtr = punNetTarget->pucReceiveBuffer + uiReceiveBufferEndIndex;

        /* And copy the data if there is any */
        if ( (punNetSource->pucReceiveBufferEndPtr - punNetSource->pucReceiveBufferStartPtr) > 0 ) {
            s_memcpy(punNetTarget->pucReceiveBufferStartPtr, punNetSource->pucReceiveBufferStartPtr, punNetSource->pucReceiveBufferEndPtr - punNetSource->pucReceiveBufferStartPtr);
        }
    }


    /* Copy the send buffer */
    if ( punNetSource->pucSendBuffer != NULL ) {

        unsigned int    uiSendBufferEndIndex = 0;

        /* Save the index */
        uiSendBufferEndIndex = punNetSource->pucSendBufferEndPtr - punNetSource->pucSendBuffer;

        /* Set the buffer length */
        punNetTarget->uiSendBufferLength = punNetSource->uiSendBufferLength;

        /* Allocate the buffer */
        if ( (punNetTarget->pucSendBuffer = (unsigned char *)s_malloc((size_t)(sizeof(unsigned char) * punNetTarget->uiSendBufferLength))) == NULL ) {
            iError = UTL_MemError;
            goto bailFromiUtlNetDuplicate;
        }

        /* Set the offsets */
        punNetTarget->pucSendBufferEndPtr = punNetTarget->pucSendBuffer + uiSendBufferEndIndex;

        /* And copy the data if there is any */
        if ( (punNetSource->pucSendBufferEndPtr - punNetSource->pucSendBuffer) > 0 ) {
            s_memcpy(punNetTarget->pucReceiveBufferStartPtr, punNetSource->pucSendBuffer, punNetSource->pucSendBufferEndPtr - punNetSource->pucSendBuffer);
        }
    }



    /* Bail label */
    bailFromiUtlNetDuplicate:


    /* Handle the error */
    if ( iError == UTL_NoError ) {

        /* Set the return pointer */
        *ppvUtlNet = punNetTarget;
    }
    else {

        /* Free the target net structure */
        iUtlNetFree(punNetTarget);
    }
    
    
    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlNetSetTimeOut()

    Purpose:    Set the timeout in the net structure.

    Parameters: pvUtlNet    Net structure
                iTimeOut    Millisecond timeout 

    Globals:    none

    Returns:    UTL error code

*/
int iUtlNetSetTimeOut
(
    void *pvUtlNet,
    int iTimeOut 
)
{

    struct utlNet   *punNet = (struct utlNet *)pvUtlNet;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iUtlNetSetTimeOut - iTimeOut: [%d]", iTimeOut); */


    /* Check the parameters */
    if ( pvUtlNet == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvUtlNet' parameter passed to 'iUtlNetSetTimeOut'."); 
        return (UTL_NetInvalidNet);
    }

    if ( iTimeOut < -1 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'iTimeOut' parameter passed to 'iUtlNetSetTimeOut'."); 
        return (UTL_NetInvalidTimeOut);
    }
    
    
    /* Set the timeout */
    punNet->iTimeOut = iTimeOut;
    
    
    return (UTL_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlNetReceive()

    Purpose:    Receive the message, this is really ugly. If we are a client
                we receive all we can within the timeout. If we are a server, 
                we do a select if we are not connected to see which socket
                the connection is coming from. Then for tcp we accept() the 
                client in a non-blocking fashion so we don't block if someone 
                else grabbed the message before us. Then we read the message,
                for tcp and stdio we receive all we can within the timeout,
                for udp we receive the message in non-blocking fashion so we 
                don't block if someone else grabbed the message before us.
                
                This function is really ugly, but it does a good job of
                masking the complexity of receiving a message from socket,
                whatever its protocol, and whether we are a server or a client.

    Parameters: pvUtlNet    Net structure

    Globals:    none

    Returns:    UTL error code

*/
int iUtlNetReceive
(
    void *pvUtlNet
)
{

    int             iError = UTL_NoError;
    struct utlNet   *punNet = (struct utlNet *)pvUtlNet;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iUtlNetReceive");  */


    /* Check the parameters */
    if ( pvUtlNet == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvUtlNet' parameter passed to 'iUtlNetReceive'."); 
        return (UTL_NetInvalidNet);
    }

    
    /* Server service */
    if ( punNet->uiService == UTL_NET_SERVICE_SERVER ) {

        /* Loop forever, we control the loop from within */
        while ( true ) {

            /* Select for an incoming connection if there is no net protocol pointer set */
            if ( punNet->punpNetProtocolsPtr == NULL ) {
        
                struct pollfd           *ppfPollFd = NULL;
                struct pollfd           *ppfPollFdPtr = NULL;
                int                     iPollStatus = 0;
                struct utlNetProtocol   *punpNetProtocolsPtr = NULL;
                unsigned int            uiI = 0;


                /* Allocate the poll structure, note the screwy frees for this */
                if ( (ppfPollFd = (struct pollfd *)s_malloc((size_t)(sizeof(struct pollfd) * punNet->uiNetProtocolsLength))) == NULL ) {
                    return (UTL_MemError);
                }

    
                /* Loop over the net protocols, setting the pollfd structure for each socket  */
                for ( punpNetProtocolsPtr = punNet->punpNetProtocols, ppfPollFdPtr = ppfPollFd, uiI = 0; uiI < punNet->uiNetProtocolsLength; 
                        punpNetProtocolsPtr++, ppfPollFdPtr++, uiI++ ) {
        
                    /* Set the pollfd structure */
                    ppfPollFdPtr->events = POLLIN;

                    /* Set the pollfd structure for each socket for the appropriate socket */
                    if ( punpNetProtocolsPtr->uiNetworkProtocolID == UTL_NET_PROTOCOL_TCP_ID ) {
                        ppfPollFdPtr->fd = punpNetProtocolsPtr->iServerSocket;
                    }
                    else if ( (punpNetProtocolsPtr->uiNetworkProtocolID == UTL_NET_PROTOCOL_UDP_ID) || (punpNetProtocolsPtr->uiNetworkProtocolID == UTL_NET_PROTOCOL_STDIO_ID) ) {
                        ppfPollFdPtr->fd = punpNetProtocolsPtr->iReadSocket;
                    }
                }

    
                /* Loop forever, we control the loop from within */
                while ( true ) {

                    /* Loop over the net protocols, setting the pollfd structure for each socket  */
                    for ( punpNetProtocolsPtr = punNet->punpNetProtocols, ppfPollFdPtr = ppfPollFd, uiI = 0; uiI < punNet->uiNetProtocolsLength; 
                            punpNetProtocolsPtr++, ppfPollFdPtr++, uiI++ ) {
                        ppfPollFdPtr->revents = 0;
                    }
        
                    /* Clear the global error */
                    errno = EOK;
    
                    /* Issue the select, infinite timeout, break out if it succeeded */
                    if ( (iPollStatus = s_poll(ppfPollFd, punNet->uiNetProtocolsLength, -1)) != -1 ) {
                        break;
                    }
            
                    /* Handle the error */
                    if ( errno == EINTR ) {
                        /* If we were interrupted, we just loop */
                        ;
                    }
                    else if ( errno == EBADF ) {
                        /* The socket was bad */
                        s_free(ppfPollFd);
                        return (UTL_NetSocketInvalidDescriptor);
                    }
                    else {
                        /* Select error */
                        s_free(ppfPollFd);
                        return (UTL_NetSocketPollError);
                    }
                }    /* while (true ) */
    
/* iUtlLogInfo(UTL_LOG_CONTEXT, "iUtlNetReceive - iPollStatus: [%d]", iPollStatus); */
            
                /* Return here if no sockets responded, which should not happen since there is an infinite timeout */ 
                if ( iPollStatus == 0 ) {
                    s_free(ppfPollFd);
                    return (UTL_NetAcceptClientFailed);
                }
        
        
                /* Loop over the net protocols, checking the fd_set structure for each socket, punpNetProtocolsPtr will be 
                ** set to a net protocol we can check for a connection
                */
                for ( punpNetProtocolsPtr = punNet->punpNetProtocols, ppfPollFdPtr = ppfPollFd, uiI = 0; uiI < punNet->uiNetProtocolsLength; 
                        punpNetProtocolsPtr++, ppfPollFdPtr++, uiI++ ) { 
        
                    /* Check the socket for errors */
                    if ( (iError = iUtlSocketGetError(punpNetProtocolsPtr->iServerSocket)) != UTL_NoError ) {
                        /* Socket error */
/* iUtlLogInfo(UTL_LOG_CONTEXT, "iUtlNetReceive - iUtlSocketGetError(punpNetProtocolsPtr->iServerSocket): %d", iError); */
                        s_free(ppfPollFd);
                        return (UTL_NetAcceptClientFailed);
                    }

                    /* Check the pollfd structure for errors */
                    if ( (ppfPollFdPtr->revents & POLLERR) > 0 ) {
                        /* The socket errored */
                        s_free(ppfPollFd);
                        return (UTL_NetSocketError);
                    }

                    /* Check the pollfd structure for hangups */
                    if ( (ppfPollFdPtr->revents & POLLHUP) > 0 ) {
                        /* The socket closed */
                        s_free(ppfPollFd);
                        return (UTL_NetSocketClosed);
                    }

                    /* Check the pollfd structure for receiving */
                    if ( (ppfPollFdPtr->revents & POLLIN) > 0 ) {
                        /* The socket is ready for receiving */
                        break;
                    }
                }

                /* Free the poll structure */
                s_free(ppfPollFd);

                
                /* Return here if we did not find the socket, which should not happen since one responded */ 
                if ( uiI >= punNet->uiNetProtocolsLength ) {
                    return (UTL_NetAcceptClientFailed);
                }
        
        
                /* Accept the client if this is a tcp connection */
                if ( punpNetProtocolsPtr->uiNetworkProtocolID == UTL_NET_PROTOCOL_TCP_ID ) {
        
/* iUtlLogDebug(UTL_LOG_CONTEXT, "iUtlNetReceive - UTL_NET_PROTOCOL_TCP_ID");  */
    
                    /* Accept the client on the read socket */
                    if ( (iError = iUtlSocketAcceptClientTcp(punpNetProtocolsPtr->iServerSocket, &punpNetProtocolsPtr->iReadSocket)) != UTL_NoError ) {
        
                        /* The socket would have blocked which means that someone else got to it first, so we loop back */
                        if ( iError == UTL_SocketWouldBlock ) {
/* iUtlLogInfo(UTL_LOG_CONTEXT, "iUtlNetReceive - iError == UTL_SocketWouldBlock (tcp)");  */
                            continue;
                        }
        
/* iUtlLogInfo(UTL_LOG_CONTEXT, "iUtlNetReceive - iError == UTL_NetAcceptClientFailed (tcp)");  */
                        /* Could not handle this error */
                        return (UTL_NetAcceptClientFailed);
                    }

/* iUtlLogInfo(UTL_LOG_CONTEXT, "iUtlNetReceive - iUtlSocketAcceptClientTcp() == UTL_NoError");  */

                    /* Set the socket to non-blocking explicitely, linux requires this */
                    if ( (iError = iUtlSocketSetNonBlockingIO(punpNetProtocolsPtr->iReadSocket)) != UTL_NoError ) {
                        return (UTL_NetAcceptClientFailed);
                    }

                    /* Copy the read socket to the write socket, they are the same for tcp servers */
                    punpNetProtocolsPtr->iWriteSocket = punpNetProtocolsPtr->iReadSocket;
                }
/*                 else if ( punpNetProtocolsPtr->uiNetworkProtocolID == UTL_NET_PROTOCOL_UDP_ID ) { */
/* iUtlLogDebug(UTL_LOG_CONTEXT, "iUtlNetReceive - UTL_NET_PROTOCOL_UDP_ID");  */
/*                 } */
/*                 else if ( punpNetProtocolsPtr->uiNetworkProtocolID == UTL_NET_PROTOCOL_STDIO_ID ) { */
/* iUtlLogDebug(UTL_LOG_CONTEXT, "iUtlNetReceive - UTL_NET_PROTOCOL_STDIO_ID");  */
/*                 } */
                
                /* Set the net protocols pointer */
                punNet->punpNetProtocolsPtr = punpNetProtocolsPtr;
            }
    
    
            /* Check that we are connected */
            if ( (punNet->punpNetProtocolsPtr == NULL) || (punNet->punpNetProtocolsPtr->iReadSocket < 0) ) {
                punNet->punpNetProtocolsPtr = NULL;
                return (UTL_NetInvalidNetProtocol);
            }
        

            /* Receive the data */    
            if ( punNet->punpNetProtocolsPtr->uiNetworkProtocolID == UTL_NET_PROTOCOL_UDP_ID ) {
            
                /* Store the timeout */
                int    iTimeOut = punNet->iTimeOut;

                /* Receive data, override and restore the timeout, we do this to bypass the call to 
                ** iUtlSocketReadyToReceive() in iUtlNetReceiveUdp() so that the call return with 
                ** UTL_NetWouldBlock if there was nothing to receive, which we know since select() 
                ** told us so
                */
                punNet->iTimeOut = -1;
                iError = iUtlNetReceiveUdp(punNet);
                punNet->iTimeOut = iTimeOut;
                
                /* Handle the receive error */
                if ( iError != UTL_NoError ) {
                    
                    /* The socket would have blocked which means that someone else got to it first, so we loop back */
                    if ( iError == UTL_NetWouldBlock ) {
/* iUtlLogDebug(UTL_LOG_CONTEXT, "iUtlNetReceive - iError == UTL_SocketWouldBlock (udp)");  */
                        punNet->punpNetProtocolsPtr = NULL;
                        continue;
                    }
                    
                    /* Could not handle this error */
                    return (iError);
                }
                
                punNet->iTimeOut = iTimeOut;

            }
            else if ( (punNet->punpNetProtocolsPtr->uiNetworkProtocolID == UTL_NET_PROTOCOL_TCP_ID) || (punNet->punpNetProtocolsPtr->uiNetworkProtocolID == UTL_NET_PROTOCOL_STDIO_ID) ) {
            
                /* Receive data, just one byte is enough since iUtlNetReceiveTcp() will receive all there is to receive */
                if ( (iError = iUtlNetReceiveTcp(punNet, NULL, 1)) != UTL_NoError ) {
                    return (iError);
                }
            }
    
    
            /* We have our correction and our data, so we break out */
            break;

        }    /* while (true ) */
    }
    
    /* Client service */
    else if ( punNet->uiService == UTL_NET_SERVICE_CLIENT ) {

        /* Check that we are connected */
        if ( (punNet->punpNetProtocolsPtr == NULL) || (punNet->punpNetProtocolsPtr->iReadSocket < 0) ) {
            return (UTL_NetInvalidNetProtocol);
        }
    
        /* Receive the data */    
        if ( punNet->punpNetProtocolsPtr->uiNetworkProtocolID == UTL_NET_PROTOCOL_UDP_ID ) {
        
            /* Receive data */
            if ( (iError = iUtlNetReceiveUdp(punNet)) != UTL_NoError ) {
                /* Could not handle this error */
                return (iError);
            }
        }
        else if ( (punNet->punpNetProtocolsPtr->uiNetworkProtocolID == UTL_NET_PROTOCOL_TCP_ID) || (punNet->punpNetProtocolsPtr->uiNetworkProtocolID == UTL_NET_PROTOCOL_STDIO_ID) ) {
        
            /* Receive data, just one byte is enough since iUtlNetReceiveTcp() will receive all there is to receive */
            if ( (iError = iUtlNetReceiveTcp(punNet, NULL, 1)) != UTL_NoError ) {
                return (iError);
            }
        }
    }


    return (UTL_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlNetPeek()

    Purpose:    Peek data from the receive buffer, limited by the max size of
                the buffer and by the amount of data actually in the buffer,
                whichever is smallest.

    Parameters: pvUtlNet            Net structure
                uiBufferLength      Length of the buffer to peek
                ppucBuffer          Return pointer to the start of the peeked bytes

    Globals:    none

    Returns:    UTL error code

*/
int iUtlNetPeek
(
    void *pvUtlNet,
    unsigned int uiBufferLength, 
    unsigned char **ppucBuffer
)
{

    int             iError = UTL_NoError;
    struct utlNet   *punNet = (struct utlNet *)pvUtlNet;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iUtlNetPeek - uiBufferLength: [%u]", uiBufferLength);  */


    /* Check the parameters */
    if ( pvUtlNet == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvUtlNet' parameter passed to 'iUtlNetPeek'."); 
        return (UTL_NetInvalidNet);
    }

    if ( uiBufferLength <= 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiBufferLength' parameter passed to 'iUtlNetPeek'."); 
        return (UTL_NetInvalidBufferLength);
    }

    if ( ppucBuffer == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppucBuffer' parameter passed to 'iUtlNetPeek'."); 
        return (UTL_ReturnParameterError);
    }
    
    
    /* Check that we are connected */
    if ( (punNet->punpNetProtocolsPtr == NULL) || (punNet->punpNetProtocolsPtr->iReadSocket < 0) ) {
        return (UTL_NetInvalidNetProtocol);
    }


    /* Peek the data */
    if ( punNet->punpNetProtocolsPtr->uiNetworkProtocolID == UTL_NET_PROTOCOL_UDP_ID ) {
    
        /* Check that there is enough data in the buffer */
        if ( uiBufferLength > (punNet->pucReceiveBufferEndPtr - punNet->pucReceiveBufferStartPtr) ) {
            return (UTL_NetInsufficientData);
        }
    }
    else if ( (punNet->punpNetProtocolsPtr->uiNetworkProtocolID == UTL_NET_PROTOCOL_TCP_ID) || (punNet->punpNetProtocolsPtr->uiNetworkProtocolID == UTL_NET_PROTOCOL_STDIO_ID) ) {

        /* Receive more data if there is not enough data in the buffer */
        if ( uiBufferLength > (punNet->pucReceiveBufferEndPtr - punNet->pucReceiveBufferStartPtr) ) {

            /* Receive data */
            if ( (iError = iUtlNetReceiveTcp(punNet, NULL, uiBufferLength)) != UTL_NoError ) {
                return (iError);
            }
        }
    }


    /* Set the return pointer */
    *ppucBuffer = punNet->pucReceiveBufferStartPtr;


    return (UTL_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlNetSkip()

    Purpose:    Skip data in the receive buffer, limited by the max size of
                the buffer and by the amount of data actually in the buffer,
                whichever is smallest.

    Parameters: pvUtlNet            Net structure
                uiBufferLength      Length of the buffer to skip

    Globals:    none

    Returns:    UTL error code

*/
int iUtlNetSkip
(
    void *pvUtlNet,
    unsigned int uiBufferLength
)
{

    int             iError = UTL_NoError;
    struct utlNet   *punNet = (struct utlNet *)pvUtlNet;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iUtlNetSkip - uiBufferLength: [%u]", uiBufferLength);  */


    /* Check the parameters */
    if ( pvUtlNet == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvUtlNet' parameter passed to 'iUtlNetSkip'."); 
        return (UTL_NetInvalidNet);
    }

    if ( uiBufferLength <= 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiBufferLength' parameter passed to 'iUtlNetSkip'."); 
        return (UTL_NetInvalidBufferLength);
    }

    
    /* Check that we are connected */
    if ( (punNet->punpNetProtocolsPtr == NULL) || (punNet->punpNetProtocolsPtr->iReadSocket < 0) ) {
        return (UTL_NetInvalidNetProtocol);
    }


    /* Peek the data */
    if ( punNet->punpNetProtocolsPtr->uiNetworkProtocolID == UTL_NET_PROTOCOL_UDP_ID ) {
    
        /* Check that there is enough data in the buffer */
        if ( uiBufferLength > (punNet->pucReceiveBufferEndPtr - punNet->pucReceiveBufferStartPtr) ) {
            return (UTL_NetInsufficientData);
        }
    }
    else if ( (punNet->punpNetProtocolsPtr->uiNetworkProtocolID == UTL_NET_PROTOCOL_TCP_ID) || (punNet->punpNetProtocolsPtr->uiNetworkProtocolID == UTL_NET_PROTOCOL_STDIO_ID) ) {

        /* Receive more data if there is not enough data in the buffer */
        if ( uiBufferLength > (punNet->pucReceiveBufferEndPtr - punNet->pucReceiveBufferStartPtr) ) {

            /* Receive data */
            if ( (iError = iUtlNetReceiveTcp(punNet, NULL, uiBufferLength)) != UTL_NoError ) {
                return (iError);
            }
        }
    }


    /* Skip the bytes */
    punNet->pucReceiveBufferStartPtr += uiBufferLength;


    return (UTL_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlNetRead()

    Purpose:    Read data from the receive buffer, udp is limited by the 
                amount of data actually in the buffer, tcp and stdio are 
                limited by the amount of data that is sent from the client,
                whether it is in the buffer or not.

    Parameters: pvUtlNet             Net structure
                pucBuffer           Buffer to receive to
                uiBufferLength      Length of the buffer to read

    Globals:    none

    Returns:    UTL error code

*/
int iUtlNetRead
(
    void *pvUtlNet,
    unsigned char *pucBuffer,
    unsigned int uiBufferLength 
)
{

    int             iError = UTL_NoError;
    struct utlNet   *punNet = (struct utlNet *)pvUtlNet;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iUtlNetRead - uiBufferLength: [%u]", uiBufferLength);  */


    /* Check the parameters */
    if ( pvUtlNet == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvUtlNet' parameter passed to 'iUtlNetRead'."); 
        return (UTL_NetInvalidNet);
    }

    if ( pucBuffer == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pucBuffer' parameter passed to 'iUtlNetRead'."); 
        return (UTL_ReturnParameterError);
    }

    if ( uiBufferLength <= 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiBufferLength' parameter passed to 'iUtlNetRead'."); 
        return (UTL_ReturnParameterError);
    }


    /* Check that we are connected */
    if ( (punNet->punpNetProtocolsPtr == NULL) || (punNet->punpNetProtocolsPtr->iReadSocket < 0) ) {
        return (UTL_NetInvalidNetProtocol);
    }


    /* Read the data */
    if ( punNet->punpNetProtocolsPtr->uiNetworkProtocolID == UTL_NET_PROTOCOL_UDP_ID ) {
    
        /* Check that there is enough data in the receive buffer */
        if ( uiBufferLength > (punNet->pucReceiveBufferEndPtr - punNet->pucReceiveBufferStartPtr) ) {
            return (UTL_NetInsufficientData);
        }

        /* Copy the data from the receive buffer */
        s_memcpy(pucBuffer, punNet->pucReceiveBufferStartPtr, uiBufferLength);

        /* Increment the receive buffer start pointer */
        punNet->pucReceiveBufferStartPtr += uiBufferLength;
    }
    else if ( (punNet->punpNetProtocolsPtr->uiNetworkProtocolID == UTL_NET_PROTOCOL_TCP_ID) || (punNet->punpNetProtocolsPtr->uiNetworkProtocolID == UTL_NET_PROTOCOL_STDIO_ID) ) {

        /* Receive more data if there is not enough in the receive buffer */
        if ( uiBufferLength > (punNet->pucReceiveBufferEndPtr - punNet->pucReceiveBufferStartPtr) ) {

            /* Receive data */
            if ( (iError = iUtlNetReceiveTcp(punNet, pucBuffer, uiBufferLength)) != UTL_NoError ) {
                return (iError);
            }
        }
        else {        

            /* Copy the data from the receive buffer */
            s_memcpy(pucBuffer, punNet->pucReceiveBufferStartPtr, uiBufferLength);

            /* Increment the receive buffer start pointer */
            punNet->pucReceiveBufferStartPtr += uiBufferLength;
        }
    }


    return (UTL_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlNetWrite()

    Purpose:    Write the data to the send buffer, udp is limited by the space 
                left in the buffer, tcp and stdio is limited by the amount of 
                data that can be sent to the client, whether it is in the buffer or not

    Parameters: pvUtlNet        Net structure
                pucBuffer       Buffer to send
                uiDataLength    Length of buffer to write

    Globals:    none

    Returns:    UTL error code

*/
int iUtlNetWrite
(
    void *pvUtlNet,
    unsigned char *pucBuffer,
    unsigned int uiBufferLength
)
{

    int             iError = UTL_NoError;
    struct utlNet   *punNet = (struct utlNet *)pvUtlNet;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iUtlNetWrite - uiBufferLength: [%u]", uiBufferLength);  */


    /* Check the parameters */
    if ( pvUtlNet == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvUtlNet' parameter passed to 'iUtlNetWrite'."); 
        return (UTL_NetInvalidNet);
    }

    if ( pucBuffer == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pucBuffer' parameter passed to 'iUtlNetWrite'."); 
        return (UTL_NetInvalidBuffer);
    }

    if ( uiBufferLength <= 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiBufferLength' parameter passed to 'iUtlNetWrite'."); 
        return (UTL_NetInvalidBufferLength);
    }


    /* Check that we are connected */
    if ( (punNet->punpNetProtocolsPtr == NULL) || (punNet->punpNetProtocolsPtr->iWriteSocket < 0) ) {
        return (UTL_NetInvalidNetProtocol);
    }


    /* Write the data */
    if ( punNet->punpNetProtocolsPtr->uiNetworkProtocolID == UTL_NET_PROTOCOL_UDP_ID ) {

        /* Return an error if there is not enough space to write the data to it */
        if ( uiBufferLength > (punNet->punpNetProtocolsPtr->uiSendBufferLength - (punNet->pucSendBufferEndPtr - punNet->pucSendBuffer) ) ) {
            return (UTL_NetInsufficientBufferSpace);
        }

        /* Copy the data to be sent into the send buffer */
        s_memcpy(punNet->pucSendBufferEndPtr, pucBuffer, uiBufferLength);

        /* Increment the send buffer end pointer */
        punNet->pucSendBufferEndPtr += uiBufferLength;
    }
    else if ( (punNet->punpNetProtocolsPtr->uiNetworkProtocolID == UTL_NET_PROTOCOL_TCP_ID) || (punNet->punpNetProtocolsPtr->uiNetworkProtocolID == UTL_NET_PROTOCOL_STDIO_ID) ) {

        /* Send the data in the send buffer if there is not enough space to write the data to it */
        if ( uiBufferLength > (punNet->uiSendBufferLength - (punNet->pucSendBufferEndPtr - punNet->pucSendBuffer) ) ) {
    
            /* Send the data in the send buffer */
            if ( (iError = iUtlNetSendTcp(punNet, punNet->pucSendBuffer, punNet->pucSendBufferEndPtr - punNet->pucSendBuffer)) != UTL_NoError ) {
                return (iError);
            }

            /* And reset the send buffer end pointer */
            punNet->pucSendBufferEndPtr = punNet->pucSendBuffer;
        }

        /* If the data is longer than the space available in the send buffer, then we just send it all rather that writing it to the send buffer */
        if ( uiBufferLength > punNet->uiSendBufferLength ) {
    
            /* Send the data */
            if ( (iError = iUtlNetSendTcp(punNet, pucBuffer, uiBufferLength)) != UTL_NoError  ) {
                return (iError);
            }
        }
        else {
            /* Copy the data to be sent into the send buffer */
            s_memcpy(punNet->pucSendBufferEndPtr, pucBuffer, uiBufferLength);
    
            /* Increment the send buffer end pointer */
            punNet->pucSendBufferEndPtr += uiBufferLength;
        }
    }


    return (UTL_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlNetSend()

    Purpose:    Send the data in the send buffer.

    Parameters: pvUtlNet    Net structure

    Globals:    none

    Returns:    UTL error code

*/
int iUtlNetSend
(
    void *pvUtlNet
)
{

    int             iError = UTL_NoError;
    struct utlNet   *punNet = (struct utlNet *)pvUtlNet;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iUtlNetSend");  */


    /* Check the parameters */
    if ( pvUtlNet == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvUtlNet' parameter passed to 'iUtlNetSend'."); 
        return (UTL_NetInvalidNet);
    }


    /* Check that we are connected */
    if ( (punNet->punpNetProtocolsPtr == NULL) || (punNet->punpNetProtocolsPtr->iWriteSocket < 0) ) {
        return (UTL_NetInvalidNetProtocol);
    }


    /* Return here if there is no data in the send buffer */
    if ( (punNet->pucSendBufferEndPtr - punNet->pucSendBuffer) == 0 ) {
        return (UTL_NoError);
    }


    /* Send the data */
    if ( punNet->punpNetProtocolsPtr->uiNetworkProtocolID == UTL_NET_PROTOCOL_UDP_ID ) {

        /* Send the data in the send buffer */
        if ( (iError = iUtlNetSendUdp(punNet)) != UTL_NoError ) {
            return (iError);
        }
    }
    else if ( (punNet->punpNetProtocolsPtr->uiNetworkProtocolID == UTL_NET_PROTOCOL_TCP_ID) || (punNet->punpNetProtocolsPtr->uiNetworkProtocolID == UTL_NET_PROTOCOL_STDIO_ID) ) {

        /* Send the data in the send buffer */
        if ( (iError = iUtlNetSendTcp(punNet, punNet->pucSendBuffer, punNet->pucSendBufferEndPtr - punNet->pucSendBuffer)) != UTL_NoError ) {
            return (iError);
        }

        /* And reset the send buffer end pointer */
        punNet->pucSendBufferEndPtr = punNet->pucSendBuffer;
    }


    return (UTL_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlNetReadyToSend()

    Purpose:    Checks if we are ready to send on the connected protocol

    Parameters: pvUtlNet    Net structure
                iTimeOut    Millisecond timeout 
                            (0 = return at once, -1 = block until ready)

    Globals:    none

    Returns:    UTL error code

*/
int iUtlNetReadyToSend
(
    void *pvUtlNet,
    int iTimeOut
)
{

    int             iError = UTL_NoError;
    struct utlNet   *punNet = (struct utlNet *)pvUtlNet;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iUtlNetReadyToSend - iTimeOut: [%d]", iTimeOut); */


    /* Check the parameters */
    if ( pvUtlNet == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvUtlNet' parameter passed to 'iUtlNetReadyToSend'."); 
        return (UTL_NetInvalidNet);
    }

    if ( iTimeOut < -1 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'iTimeOut' parameter passed to 'iUtlNetReadyToSend'."); 
        return (UTL_NetInvalidTimeOut);
    }


    /* Check that we are connected */
    if ( (punNet->punpNetProtocolsPtr == NULL) || (punNet->punpNetProtocolsPtr->iWriteSocket < 0) ) {
        return (UTL_NetInvalidNetProtocol);
    }


    /* See if the socket is ready to send */
    iError = iUtlSocketReadyToSend(punNet->punpNetProtocolsPtr->iWriteSocket, iTimeOut);
    
    /* And map the error */
    iError = iUtlMapSocketError(iError, UTL_NetReadyToSendFailed);


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlNetReadyToReceive()

    Purpose:    Checks if we are ready to receive on the connected protocol

    Parameters: pvUtlNet    Net structure
                iTimeOut    millisecond timeout 
                            (0 = return at once, -1 = block until ready)

    Globals:    none

    Returns:    UTL error code

*/
int iUtlNetReadyToReceive
(
    void *pvUtlNet,
    int iTimeOut
)
{

    int             iError = UTL_NoError;
    struct utlNet   *punNet = (struct utlNet *)pvUtlNet;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iUtlNetReadyToReceive - iTimeOut: [%d]", iTimeOut); */


    /* Check the parameters */
    if ( pvUtlNet == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvUtlNet' parameter passed to 'iUtlNetReadyToReceive'."); 
        return (UTL_NetInvalidNet);
    }

    if ( iTimeOut < -1 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'iTimeOut' parameter passed to 'iUtlNetReadyToReceive'."); 
        return (UTL_NetInvalidTimeOut);
    }


    /* Check that we are connected */
    if ( (punNet->punpNetProtocolsPtr == NULL) || (punNet->punpNetProtocolsPtr->iReadSocket < 0) ) {
        return (UTL_NetInvalidNetProtocol);
    }


    /* See if the socket is ready to receive */
    iError = iUtlSocketReadyToReceive(punNet->punpNetProtocolsPtr->iReadSocket, iTimeOut);
    
    /* And map the error */
    iError = iUtlMapSocketError(iError, UTL_NetReadyToReceiveFailed);

    
    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlNetGetWriteError()

    Purpose:    Get the socket write error

    Parameters: pvUtlNet    Net structure
    
    Globals:    none

    Returns:    UTL error code

*/
int iUtlNetGetWriteError
(
    void *pvUtlNet
)
{

    int             iError = UTL_NoError;
    struct utlNet   *punNet = (struct utlNet *)pvUtlNet;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iUtlNetGetWriteError"); */


    /* Check the parameters */
    if ( pvUtlNet == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvUtlNet' parameter passed to 'iUtlNetGetWriteError'."); 
        return (UTL_NetInvalidNet);
    }


    /* Check that we are connected */
    if ( (punNet->punpNetProtocolsPtr == NULL) || (punNet->punpNetProtocolsPtr->iWriteSocket < 0) ) {
        return (UTL_NetInvalidNetProtocol);
    }


    /* Get the error */
    iError = iUtlSocketGetError(punNet->punpNetProtocolsPtr->iWriteSocket);

    /* And map the error */
    iError = iUtlMapSocketError(iError, UTL_NetSocketError);

    
    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlNetGetReadError()

    Purpose:    Get the socket read error

    Parameters: pvUtlNet    Net structure

    Globals:    none

    Returns:    UTL error code

*/
int iUtlNetGetReadError
(
    void *pvUtlNet
)
{

    int             iError = UTL_NoError;
    struct utlNet   *punNet = (struct utlNet *)pvUtlNet;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iUtlNetGetReadError"); */


    /* Check the parameters */
    if ( pvUtlNet == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvUtlNet' parameter passed to 'iUtlNetGetReadError'."); 
        return (UTL_NetInvalidNet);
    }


    /* Check that we are connected */
    if ( (punNet->punpNetProtocolsPtr == NULL) || (punNet->punpNetProtocolsPtr->iReadSocket < 0) ) {
        return (UTL_NetInvalidNetProtocol);
    }


    /* Get the error */
    iError = iUtlSocketGetError(punNet->punpNetProtocolsPtr->iReadSocket);
    
    /* And map the error */
    iError = iUtlMapSocketError(iError, UTL_NetSocketError);

    
    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlNetGetReceiveBufferDataLength()

    Purpose:    Get the current length of the data in the receive buffer

    Parameters: pvUtlNet            Net structure
                puiBufferLength     Return pointer for the receive buffer data length

    Globals:    none

    Returns:    UTL error code

*/
int iUtlNetGetReceiveBufferDataLength
(
    void *pvUtlNet,
    unsigned int *puiBufferLength 
)
{

    struct utlNet   *punNet = (struct utlNet *)pvUtlNet;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iUtlNetGetReceiveBufferDataLength");  */

    /* Check the parameters */
    if ( pvUtlNet == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvUtlNet' parameter passed to 'iUtlNetGetReceiveBufferDataLength'."); 
        return (UTL_NetInvalidNet);
    }

    if ( puiBufferLength == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'puiBufferLength' parameter passed to 'iUtlNetGetReceiveBufferDataLength'."); 
        return (UTL_ReturnParameterError);
    }


    /* Check that we are connected */
    if ( (punNet->punpNetProtocolsPtr == NULL) || (punNet->punpNetProtocolsPtr->iReadSocket < 0) ) {
        return (UTL_NetInvalidNetProtocol);
    }
    
    
    /* Set the return pointer */
    *puiBufferLength = punNet->pucReceiveBufferEndPtr - punNet->pucReceiveBufferStartPtr;
    

    return (UTL_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlNetGetSendBufferDataLength()

    Purpose:    Get the current length of the data in the send buffer

    Parameters: pvUtlNet            Net structure
                puiBufferLength     Return pointer for the send buffer data length

    Globals:    none

    Returns:    UTL error code

*/
int iUtlNetGetSendBufferDataLength
(
    void *pvUtlNet,
    unsigned int *puiBufferLength 
)
{

    struct utlNet   *punNet = (struct utlNet *)pvUtlNet;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iUtlNetGetSendBufferDataLength");  */

    /* Check the parameters */
    if ( pvUtlNet == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvUtlNet' parameter passed to 'iUtlNetGetSendBufferDataLength'."); 
        return (UTL_NetInvalidNet);
    }

    if ( puiBufferLength == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'puiBufferLength' parameter passed to 'iUtlNetGetSendBufferDataLength'."); 
        return (UTL_ReturnParameterError);
    }


    /* Check that we are connected */
    if ( (punNet->punpNetProtocolsPtr == NULL) || (punNet->punpNetProtocolsPtr->iReadSocket < 0) ) {
        return (UTL_NetInvalidNetProtocol);
    }
    
    
    /* Set the return pointer */
    *puiBufferLength = punNet->pucSendBufferEndPtr - punNet->pucSendBuffer;
    

    return (UTL_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlNetResetReceive()

    Purpose:    Reset the receive buffer pointers. 

    Parameters: pvUtlNet    Net structure

    Globals:    none

    Returns:    UTL error code

*/
int iUtlNetResetReceive
(
    void *pvUtlNet
)
{

    struct utlNet   *punNet = (struct utlNet *)pvUtlNet;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iUtlNetResetReceive"); */


    /* Check the parameters */
    if ( pvUtlNet == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvUtlNet' parameter passed to 'iUtlNetResetReceive'."); 
        return (UTL_NetInvalidNet);
    }
    

    /* Reset the receive buffer pointers */
    punNet->pucReceiveBufferStartPtr = punNet->pucReceiveBuffer;
    punNet->pucReceiveBufferEndPtr = punNet->pucReceiveBuffer;

    
    return (UTL_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlNetResetSend()

    Purpose:    Reset the send buffer pointers. 

    Parameters: pvUtlNet    Net structure

    Globals:    none

    Returns:    UTL error code

*/
int iUtlNetResetSend
(
    void *pvUtlNet
)
{

    struct utlNet   *punNet = (struct utlNet *)pvUtlNet;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iUtlNetResetSend"); */


    /* Check the parameters */
    if ( pvUtlNet == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvUtlNet' parameter passed to 'iUtlNetResetSend'."); 
        return (UTL_NetInvalidNet);
    }
    

    /* Reset the send buffer pointer */
    punNet->pucSendBufferEndPtr = punNet->pucSendBuffer;

    
    return (UTL_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlNetGetConnectionStatus()

    Purpose:    Get the connection status, true if we are connected, false 
                if we are not connected.

    Parameters: pvUtlNet        Net structure
                pbConnected     Return pointer for the connection status

    Globals:    none

    Returns:    UTL error code

*/
int iUtlNetGetConnectionStatus
(
    void *pvUtlNet,
    boolean *pbConnected
)
{

    struct utlNet   *punNet = (struct utlNet *)pvUtlNet;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iUtlNetGetConnectionStatus"); */


    /* Check the parameters */
    if ( pvUtlNet == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvUtlNet' parameter passed to 'iUtlNetGetConnectionStatus'."); 
        return (UTL_NetInvalidNet);
    }
    
    if ( pbConnected == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pbConnected' parameter passed to 'iUtlNetGetConnectionStatus'."); 
        return (UTL_ReturnParameterError);
    }
    

    /* Set the return pointer */ 
    *pbConnected = (punNet->punpNetProtocolsPtr != NULL) ? true : false;

    
    return (UTL_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlNetGetConnectedProtocol()

    Purpose:    Get the protocol ID of the currently connected protocol

    Parameters: pvUtlNet        Net structure
                puiProtocol     Return pointer for the protocol ID of the 
                                currently connected protocol

    Globals:    none

    Returns:    UTL error code

*/
int iUtlNetGetConnectedProtocol
(
    void *pvUtlNet,
    unsigned int *puiProtocol
)
{

    struct utlNet   *punNet = (struct utlNet *)pvUtlNet;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iUtlNetGetConnectedProtocol"); */


    /* Check the parameters */
    if ( pvUtlNet == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvUtlNet' parameter passed to 'iUtlNetGetConnectedProtocol'."); 
        return (UTL_NetInvalidNet);
    }
    
    if ( puiProtocol == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'puiProtocol' parameter passed to 'iUtlNetGetConnectedProtocol'."); 
        return (UTL_ReturnParameterError);
    }
    

    /* Check that we are connected */
    if ( (punNet->punpNetProtocolsPtr == NULL) || (punNet->punpNetProtocolsPtr->iReadSocket < 0) ) {
        return (UTL_NetInvalidNetProtocol);
    }


    /* Set the return pointer */ 
    *puiProtocol = punNet->punpNetProtocolsPtr->uiNetworkProtocolID;


    return (UTL_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlNetGetConnectedProtocolType()

    Purpose:    Get the type of the currently connected protocol

    Parameters: pvUtlNet            Net structure
                puiProtocolType     Return pointer for the type of the currently 
                                    connected protocol

    Globals:    none

    Returns:    UTL error code

*/
int iUtlNetGetConnectedProtocolType
(
    void *pvUtlNet,
    unsigned int *puiProtocolType
)
{

    struct utlNet   *punNet = (struct utlNet *)pvUtlNet;
    unsigned int    uiProtocolType = UTL_NET_PROTOCOL_TYPE_INVALID_ID;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iUtlNetGetConnectedProtocolType"); */


    /* Check the parameters */
    if ( pvUtlNet == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvUtlNet' parameter passed to 'iUtlNetGetConnectedProtocolType'."); 
        return (UTL_NetInvalidNet);
    }
    
    if ( puiProtocolType == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'puiProtocolType' parameter passed to 'iUtlNetGetConnectedProtocolType'."); 
        return (UTL_ReturnParameterError);
    }
    

    /* Check that we are connected */
    if ( (punNet->punpNetProtocolsPtr == NULL) || (punNet->punpNetProtocolsPtr->iReadSocket < 0) ) {
        return (UTL_NetInvalidNetProtocol);
    }


    /* Set the protocol type */    
    switch ( punNet->punpNetProtocolsPtr->uiNetworkProtocolID ) {
        
        case UTL_NET_PROTOCOL_UDP_ID:
            uiProtocolType = UTL_NET_PROTOCOL_TYPE_MESSAGE_ID;
            break;

        case UTL_NET_PROTOCOL_TCP_ID:
            uiProtocolType = UTL_NET_PROTOCOL_TYPE_SESSION_ID;
            break;

        case UTL_NET_PROTOCOL_STDIO_ID:
            uiProtocolType = UTL_NET_PROTOCOL_TYPE_SESSION_ID;
            break;
            
        default:
            uiProtocolType = UTL_NET_PROTOCOL_TYPE_INVALID_ID;
            break;

    }


    /* Set the return pointer */ 
    *puiProtocolType = uiProtocolType;


    return (UTL_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlNetGetConnectedClientName()

    Purpose:    This function returns the host name of the client 
                connected to the socket

    Parameters: pvUtlNet            net structure
                pucClientName       return pointer for the client name
                uiClientNameLen     length of the pointer

    Globals:    none

    Returns:    UTL error code

*/
int iUtlNetGetConnectedClientName
(
    void *pvUtlNet,
    unsigned char *pucClientName, 
    unsigned int uiClientNameLen
)
{

    struct utlNet   *punNet = (struct utlNet *)pvUtlNet;


    /* Check the parameters */
    if ( pvUtlNet == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvUtlNet' parameter passed to 'iUtlNetGetConnectedClientName'."); 
        return (UTL_NetInvalidNet);
    }

    if ( pucClientName == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pucClientName' parameter passed to 'iUtlNetGetConnectedClientName'."); 
        return (UTL_ReturnParameterError);
    }

    if ( uiClientNameLen <= 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiClientNameLen' parameter passed to 'iUtlNetGetConnectedClientName'."); 
        return (UTL_ReturnParameterError);
    }


    /* Check that there is a net service selected */
    if ( punNet->punpNetProtocolsPtr == NULL ) {
        return (UTL_NetInvalidNetProtocol);
    }


    /* Get the client name */
    if ( punNet->punpNetProtocolsPtr->uiNetworkProtocolID == UTL_NET_PROTOCOL_UDP_ID ) {
        return (iUtlSocketGetClientNameFromSocketAddress(punNet->punpNetProtocolsPtr->psiSocketAddress, pucClientName, uiClientNameLen));
    }
    else if ( punNet->punpNetProtocolsPtr->uiNetworkProtocolID == UTL_NET_PROTOCOL_TCP_ID ) {
        return (iUtlSocketGetClientNameFromSocket(punNet->punpNetProtocolsPtr->iReadSocket, pucClientName, uiClientNameLen));
    }
    else if ( punNet->punpNetProtocolsPtr->uiNetworkProtocolID == UTL_NET_PROTOCOL_STDIO_ID ) {
        return (iUtlNetGetHostName(pucClientName, uiClientNameLen));
    }


    return (UTL_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlNetGetConnectedClientAddress()

    Purpose:    This function returns the host address of the client 
                connected to the socket

    Parameters: pvUtlNet                net structure
                pucClientAddress        return pointer for the client address
                uiClientAddressLen      length of the pointer

    Globals:    none

    Returns:    UTL error code

*/
int iUtlNetGetConnectedClientAddress
(
    void *pvUtlNet,
    unsigned char *pucClientAddress,
    unsigned int uiClientAddressLen
)
{

    struct utlNet   *punNet = (struct utlNet *)pvUtlNet;


    /* Check the parameters */
    if ( pvUtlNet == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvUtlNet' parameter passed to 'iUtlNetGetConnectedClientAddress'."); 
        return (UTL_NetInvalidNet);
    }

    if ( pucClientAddress == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pucClientAddress' parameter passed to 'iUtlNetGetConnectedClientAddress'."); 
        return (UTL_ReturnParameterError);
    }

    if ( uiClientAddressLen <= 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiClientAddressLen' parameter passed to 'iUtlNetGetConnectedClientAddress'."); 
        return (UTL_ReturnParameterError);
    }


    /* Check that there is a net service selected */
    if ( punNet->punpNetProtocolsPtr == NULL ) {
        return (UTL_NetInvalidNetProtocol);
    }


    /* Get the client address */
    if ( punNet->punpNetProtocolsPtr->uiNetworkProtocolID == UTL_NET_PROTOCOL_UDP_ID ) {
        return (iUtlSocketGetClientAddressFromSocketAddress(punNet->punpNetProtocolsPtr->psiSocketAddress, pucClientAddress, uiClientAddressLen));
    }
    else if ( punNet->punpNetProtocolsPtr->uiNetworkProtocolID == UTL_NET_PROTOCOL_TCP_ID ) {
        return (iUtlSocketGetClientAddressFromSocket(punNet->punpNetProtocolsPtr->iReadSocket, pucClientAddress, uiClientAddressLen));
    }
    else if ( punNet->punpNetProtocolsPtr->uiNetworkProtocolID == UTL_NET_PROTOCOL_STDIO_ID ) {
        return (iUtlNetGetHostAddress(pucClientAddress, uiClientAddressLen));
    }


    return (UTL_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlNetGetConnectedHostName()

    Purpose:    This function returns the host name of the host 
                connected to the socket

    Parameters: pvUtlNet        net structure
                pucHostName     return pointer for the host name
                uiHostNameLen   length of the pointer

    Globals:    none

    Returns:    UTL error code

*/
int iUtlNetGetConnectedHostName
(
    void *pvUtlNet,
    unsigned char *pucHostName, 
    unsigned int uiHostNameLen
)
{

    struct utlNet   *punNet = (struct utlNet *)pvUtlNet;


    /* Check the parameters */
    if ( pvUtlNet == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvUtlNet' parameter passed to 'iUtlNetGetConnectedHostName'."); 
        return (UTL_NetInvalidNet);
    }

    if ( pucHostName == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pucHostName' parameter passed to 'iUtlNetGetConnectedHostName'."); 
        return (UTL_ReturnParameterError);
    }

    if ( uiHostNameLen <= 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiHostNameLen' parameter passed to 'iUtlNetGetConnectedHostName'."); 
        return (UTL_ReturnParameterError);
    }


    /* Check that there is a net service selected */
    if ( punNet->punpNetProtocolsPtr == NULL ) {
        return (UTL_NetInvalidNetProtocol);
    }


    /* Get the host name */
    if ( (punNet->punpNetProtocolsPtr->uiNetworkProtocolID == UTL_NET_PROTOCOL_UDP_ID) || (punNet->punpNetProtocolsPtr->uiNetworkProtocolID == UTL_NET_PROTOCOL_TCP_ID) ) {
        if ( bUtlStringsIsStringNULL(punNet->punpNetProtocolsPtr->pucHostName) == false ) {
            s_strnncpy(pucHostName, punNet->punpNetProtocolsPtr->pucHostName, uiHostNameLen);
        }
        else {
            return (iUtlNetGetHostName(pucHostName, uiHostNameLen));
        }
    }
    else if ( punNet->punpNetProtocolsPtr->uiNetworkProtocolID == UTL_NET_PROTOCOL_STDIO_ID ) {
        return (iUtlNetGetHostName(pucHostName, uiHostNameLen));
    }


    return (UTL_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlNetGetConnectedHostAddress()

    Purpose:    This function returns the host address of the host 
                connected to the socket

    Parameters: pvUtlNet            net structure
                pucHostAddress      return pointer for the host address
                uiHostAddressLen    length of the pointer

    Globals:    none

    Returns:    UTL error code

*/
int iUtlNetGetConnectedHostAddress
(
    void *pvUtlNet,
    unsigned char *pucHostAddress,
    unsigned int uiHostAddressLen
)
{

    struct utlNet   *punNet = (struct utlNet *)pvUtlNet;


    /* Check the parameters */
    if ( pvUtlNet == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvUtlNet' parameter passed to 'iUtlNetGetConnectedHostAddress'."); 
        return (UTL_NetInvalidNet);
    }

    if ( pucHostAddress == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pucHostAddress' parameter passed to 'iUtlNetGetConnectedHostAddress'."); 
        return (UTL_ReturnParameterError);
    }

    if ( uiHostAddressLen <= 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiHostAddressLen' parameter passed to 'iUtlNetGetConnectedHostAddress'."); 
        return (UTL_ReturnParameterError);
    }


    /* Check that there is a net service selected */
    if ( punNet->punpNetProtocolsPtr == NULL ) {
        return (UTL_NetInvalidNetProtocol);
    }


    /* Get the host address */
    if ( (punNet->punpNetProtocolsPtr->uiNetworkProtocolID == UTL_NET_PROTOCOL_UDP_ID) || (punNet->punpNetProtocolsPtr->uiNetworkProtocolID == UTL_NET_PROTOCOL_TCP_ID) ) {
        if ( bUtlStringsIsStringNULL(punNet->punpNetProtocolsPtr->pucHostName) == false ) {
            s_strnncpy(pucHostAddress, punNet->punpNetProtocolsPtr->pucHostName, uiHostAddressLen);
        }
        else {
            return (iUtlNetGetHostName(pucHostAddress, uiHostAddressLen));
        }
    }
    else if ( punNet->punpNetProtocolsPtr->uiNetworkProtocolID == UTL_NET_PROTOCOL_STDIO_ID ) {
        return (iUtlNetGetHostAddress(pucHostAddress, uiHostAddressLen));
    }


    return (UTL_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlNetGetConnectedPort()

    Purpose:    This function returns the port on which the socket is connected

    Parameters: pvUtlNet    net structure
                piPort      return pointer for the port

    Globals:    none

    Returns:    UTL error code

*/
int iUtlNetGetConnectedPort
(
    void *pvUtlNet,
    int *piPort
)
{

    struct utlNet   *punNet = (struct utlNet *)pvUtlNet;


    /* Check the parameters */
    if ( pvUtlNet == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvUtlNet' parameter passed to 'iUtlNetGetConnectedPort'."); 
        return (UTL_NetInvalidNet);
    }

    if ( piPort == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'piPort' parameter passed to 'iUtlNetGetConnectedPort'."); 
        return (UTL_ReturnParameterError);
    }


    /* Check that there is a net service selected */
    if ( punNet->punpNetProtocolsPtr == NULL ) {
        return (UTL_NetInvalidNetProtocol);
    }


    /* Get the port */
    if ( (punNet->punpNetProtocolsPtr->uiNetworkProtocolID == UTL_NET_PROTOCOL_UDP_ID) || (punNet->punpNetProtocolsPtr->uiNetworkProtocolID == UTL_NET_PROTOCOL_TCP_ID) ) {
        *piPort = punNet->punpNetProtocolsPtr->iPort;
    }
    else if ( punNet->punpNetProtocolsPtr->uiNetworkProtocolID == UTL_NET_PROTOCOL_STDIO_ID ) {
        *piPort = -1;
    }


    return (UTL_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlNetCloseClient()

    Purpose:    Close the client socket in the net structure, this only 
                makes sense when we are a server, and nothing will happen
                if we are a client.

    Parameters: pvUtlNet    Net structure

    Globals:    none

    Returns:    UTL error code

*/
int iUtlNetCloseClient
(
    void *pvUtlNet
)
{

    int             iError = UTL_NoError;
    struct utlNet   *punNet = (struct utlNet *)pvUtlNet;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iUtlNetCloseClient"); */


    /* Check the parameters */
    if ( pvUtlNet == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvUtlNet' parameter passed to 'iUtlNetCloseClient'."); 
        return (UTL_NetInvalidNet);
    }
    

    /* Check that there is a net service selected */
    if ( punNet->punpNetProtocolsPtr == NULL ) {
        return (UTL_NetInvalidNetProtocol);
    }


    /* Close the socket if we are a server */
    if ( punNet->uiService == UTL_NET_SERVICE_SERVER ) {

        /* Process protocols */
        if ( punNet->punpNetProtocolsPtr->uiNetworkProtocolID == UTL_NET_PROTOCOL_TCP_ID ) {

            /* Close the socket */
            if ( punNet->punpNetProtocolsPtr->iReadSocket != -1 ) {
                iUtlSocketClose(punNet->punpNetProtocolsPtr->iReadSocket);
            }

            /* Set the sockets to -1 */
            punNet->punpNetProtocolsPtr->iReadSocket = -1;
            punNet->punpNetProtocolsPtr->iWriteSocket = -1;
        }
        else if ( punNet->punpNetProtocolsPtr->uiNetworkProtocolID == UTL_NET_PROTOCOL_UDP_ID ) {

            /* Free the socket address, udp sockets are never connected so we dont close them */
            s_free(punNet->punpNetProtocolsPtr->psiSocketAddress);
        }
        else if ( punNet->punpNetProtocolsPtr->uiNetworkProtocolID == UTL_NET_PROTOCOL_STDIO_ID ) {
            /* Do nothing for stdio */
            ;
        }


        /* NULL the protocol pointer */
        punNet->punpNetProtocolsPtr = NULL;

        /* Reset the send and receive buffers */
        iUtlNetResetSend((void *)punNet);
        iUtlNetResetReceive((void *)punNet);
    }


    return ((iError != UTL_NoError) ? UTL_NetCloseSocketFailed : UTL_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlNetFree()

    Purpose:    Free a net structure. 

    Parameters: pvUtlNet        Net structure

    Globals:    none

    Returns:    UTL error code

*/
int iUtlNetFree
(
    void *pvUtlNet
)
{

    struct utlNet           *punNet = (struct utlNet *)pvUtlNet;
    struct utlNetProtocol   *punpNetProtocolsPtr = NULL;
    unsigned int            uiI = 0;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iUtlNetFree"); */


    /* Check the parameters */
    if ( pvUtlNet == NULL ) {
        iUtlLogDebug(UTL_LOG_CONTEXT, "Null 'pvUtlNet' parameter passed to 'iUtlNetFree'."); 
        return (UTL_NetInvalidNet);
    }


/*     iUtlNetPrint(pvUtlNet); */

    /* Loop over the net protocols */
    for ( punpNetProtocolsPtr = punNet->punpNetProtocols, uiI = 0; uiI < punNet->uiNetProtocolsLength; punpNetProtocolsPtr++, uiI++ ) {

        /* Free the buffers */
        s_free(punpNetProtocolsPtr->pucHostName);

        /* Close the sockets */
        if ( punNet->uiService == UTL_NET_SERVICE_SERVER ) {
            
            if ( punpNetProtocolsPtr->uiNetworkProtocolID == UTL_NET_PROTOCOL_UDP_ID ) {
                if ( (punpNetProtocolsPtr->iReadSocket != -1) /* && (iUtlSocketGetError(punpNetProtocolsPtr->iReadSocket) == UTL_NoError) */ ) {
                    iUtlSocketClose(punpNetProtocolsPtr->iReadSocket);
                    punpNetProtocolsPtr->iReadSocket = -1;
                }

                /* Free the socket address */
                s_free(punpNetProtocolsPtr->psiSocketAddress);
            }
            else if ( punpNetProtocolsPtr->uiNetworkProtocolID == UTL_NET_PROTOCOL_TCP_ID ) {
                if ( (punpNetProtocolsPtr->iServerSocket != -1) /* && (iUtlSocketGetError(punpNetProtocolsPtr->iServerSocket) == UTL_NoError) */ ) {
                    iUtlSocketClose(punpNetProtocolsPtr->iServerSocket);
                    punpNetProtocolsPtr->iServerSocket = -1;
                }
                if ( (punpNetProtocolsPtr->iReadSocket != -1) /* && (iUtlSocketGetError(punpNetProtocolsPtr->iReadSocket) == UTL_NoError) */ ) {
                    iUtlSocketClose(punpNetProtocolsPtr->iReadSocket);
                    punpNetProtocolsPtr->iReadSocket = -1;
                }
            }
            else if ( punpNetProtocolsPtr->uiNetworkProtocolID == UTL_NET_PROTOCOL_STDIO_ID ) {
                /* Do nothing for stdio */
            }
        }
        else if ( punNet->uiService == UTL_NET_SERVICE_CLIENT ) {

            if ( punpNetProtocolsPtr->uiNetworkProtocolID == UTL_NET_PROTOCOL_UDP_ID ) {
                if ( (punpNetProtocolsPtr->iWriteSocket != -1) /* && (iUtlSocketGetError(punpNetProtocolsPtr->iWriteSocket) == UTL_NoError) */ ) {
                    iUtlSocketClose(punpNetProtocolsPtr->iWriteSocket);
                    punpNetProtocolsPtr->iWriteSocket = -1;
                }
            }
            else if ( punpNetProtocolsPtr->uiNetworkProtocolID == UTL_NET_PROTOCOL_TCP_ID ) {
                if ( (punpNetProtocolsPtr->iWriteSocket != -1) /* && (iUtlSocketGetError(punpNetProtocolsPtr->iWriteSocket) == UTL_NoError) */ ) {
                    iUtlSocketClose(punpNetProtocolsPtr->iWriteSocket);
                    punpNetProtocolsPtr->iWriteSocket = -1;
                }
            }
            else if ( punpNetProtocolsPtr->uiNetworkProtocolID == UTL_NET_PROTOCOL_STDIO_ID ) {
                /* Do nothing for stdio */
            }
        }
    }

    /* Free the buffers, along with the net protocols structure and the net structure */
    s_free(punNet->pucReceiveBuffer);
    s_free(punNet->pucSendBuffer);
    s_free(punNet->punpNetProtocols);
    s_free(punNet);

    
    return (UTL_NoError);

}


/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlNetCreateService()

    Purpose:    Create a new net structure, client or server

    Parameters: uiService   Service
                iTimeOut    Millisecond timeout 
                            (0 = return at once, -1 = block until ready)
                ppunNet     Return pointer for the net structure

    Globals:    none

    Returns:    UTL error code

*/
static int iUtlNetCreateService
(
    unsigned int uiService, 
    int iTimeOut,
    struct utlNet **ppunNet
)
{

    int             iError = UTL_NoError;
    struct utlNet   *punNet = NULL;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iUtlNetCreateService - uiService: [%u]", uiService); */


    ASSERT(UTL_NET_SERVICE_VALID(uiService) == true);
    ASSERT(iTimeOut >= -1);
    ASSERT(ppunNet != NULL);


    /* Allocate the net structure */
    if ( (punNet = (struct utlNet *)s_malloc((size_t)(sizeof(struct utlNet)))) == NULL ) {
        return (UTL_MemError);
    }


    /* Initialize the net structure */
    punNet->uiService = uiService;
    punNet->iTimeOut = iTimeOut;
    punNet->punpNetProtocols = NULL;
    punNet->uiNetProtocolsLength = 0;
    punNet->punpNetProtocolsPtr = NULL;

    punNet->pucReceiveBuffer = NULL;
    punNet->uiReceiveBufferLength = 0;
    punNet->pucReceiveBufferStartPtr = NULL;
    punNet->pucReceiveBufferEndPtr = NULL;

    punNet->pucSendBuffer = NULL;
    punNet->uiSendBufferLength = 0;
    punNet->pucSendBufferEndPtr = NULL;


    /* Set the return pointer */
    *ppunNet = (void *)punNet;


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlNetAddProtocol()

    Purpose:    Adds a new protocol to the net structure, client or server

    Parameters: punNet                  Net structure
                uiNetworkProtocolID     Network protocol ID
                pucHostName             Host name
                iPort                   Port

    Globals:    none

    Returns:    UTL error code

*/
static int iUtlNetAddProtocol
(
    struct utlNet *punNet,
    unsigned int uiNetworkProtocolID, 
    unsigned char *pucHostName, 
    int iPort
)
{

    int                     iError = UTL_NoError;
    struct utlNetProtocol   *punpNetProtocolsPtr = NULL;
    unsigned int            uiI = 0;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iUtlNetAddProtocol - uiNetworkProtocolID: [%u], pucHostName: [%s], iPort: [%d]]",  */
/*             uiNetworkProtocolID, pucUtlStringsGetPrintableString(pucHostName), iPort); */


    ASSERT(punNet != NULL);
    ASSERT(UTL_NET_PROTOCOL_VALID_ID(uiNetworkProtocolID)== true);
    ASSERT(iPort >= 0);

    ASSERT(UTL_NET_SERVICE_VALID(punNet->uiService) == true);
    ASSERT((punNet->uiService == UTL_NET_SERVICE_SERVER) || 
            ((punNet->uiService == UTL_NET_SERVICE_CLIENT) && ((uiNetworkProtocolID == UTL_NET_PROTOCOL_UDP_ID) || (uiNetworkProtocolID == UTL_NET_PROTOCOL_TCP_ID)) && (bUtlStringsIsStringNULL(pucHostName) == false)) ||
            ((punNet->uiService == UTL_NET_SERVICE_CLIENT) && (uiNetworkProtocolID == UTL_NET_PROTOCOL_STDIO_ID)));


    
    /* Check that the protocol has not already been added */
    if ( punNet->punpNetProtocols != NULL ) {

        /* Loop over the  net protocols */
        for ( punpNetProtocolsPtr = punNet->punpNetProtocols, uiI = 0; uiI < punNet->uiNetProtocolsLength; punpNetProtocolsPtr++, uiI++ ) {
            
            /* Compare and reject if it is already there */
            if ( (uiNetworkProtocolID == punpNetProtocolsPtr->uiNetworkProtocolID) && (iPort == punpNetProtocolsPtr->iPort) && 
                (((bUtlStringsIsStringNULL(pucHostName) == true) && (bUtlStringsIsStringNULL(punpNetProtocolsPtr->pucHostName) == true)) || 
                    ((bUtlStringsIsStringNULL(pucHostName) == false) && (bUtlStringsIsStringNULL(punpNetProtocolsPtr->pucHostName) == false) && (s_strcmp(pucHostName, punpNetProtocolsPtr->pucHostName) == 0))) ) {
                
                /* Protocol already exists */
                return (UTL_NetDuplicateProtocol);
            }
        }
    }

    
    /* Extend the net protocols */
    if ( (punpNetProtocolsPtr = (struct utlNetProtocol *)s_realloc(punNet->punpNetProtocols, sizeof(struct utlNetProtocol) * (punNet->uiNetProtocolsLength + 1))) == NULL ) {
        return (UTL_MemError);
    }
    
    /* Hand over the net protocols pointer */
    punNet->punpNetProtocols = punpNetProtocolsPtr;
    
    /* Note that we dont increment the net protocols length (punNet->uiNetProtocolsLength) 
    ** until we reach the end of the function at which point we know that we successfully 
    ** added the protocol
    */

    /* Deferefence the net protocols pointer, this is what we want to use for all subsequent operations here */
    punpNetProtocolsPtr = punNet->punpNetProtocols + punNet->uiNetProtocolsLength;
    
    /* Clear the new entry in the net protocols */
    s_memset(punpNetProtocolsPtr, 0, sizeof(struct utlNetProtocol));


    /* Set the net protocols pointer for the client */
    if ( (punNet->uiService == UTL_NET_SERVICE_CLIENT) || (uiNetworkProtocolID == UTL_NET_PROTOCOL_STDIO_ID) ) {
        punNet->punpNetProtocolsPtr = punNet->punpNetProtocols + punNet->uiNetProtocolsLength;
    }


    /* Fill in the net protocols fields */
    punpNetProtocolsPtr->uiNetworkProtocolID = uiNetworkProtocolID;
    if ( bUtlStringsIsStringNULL(pucHostName) == false ) {
        if ( (punpNetProtocolsPtr->pucHostName = s_strdup(pucHostName)) == NULL ) {
            iError = UTL_MemError;
            goto bailFromiUtlNetAddProtocol;
        }
    }
    punpNetProtocolsPtr->iPort = iPort;
    punpNetProtocolsPtr->iServerSocket = -1;
    punpNetProtocolsPtr->iReadSocket = -1;
    punpNetProtocolsPtr->iWriteSocket = -1;
    punpNetProtocolsPtr->psiSocketAddress = NULL;


    /* Open the socket based on the service we want and the protocol used */
    if ( punNet->uiService == UTL_NET_SERVICE_SERVER ) {

        /* Open the server socket */
        if ( punpNetProtocolsPtr->uiNetworkProtocolID == UTL_NET_PROTOCOL_UDP_ID ) {

            /* Open the socket */
            if ( (iError = iUtlSocketOpenServerUdp(pucHostName, punpNetProtocolsPtr->iPort, UTL_NET_BUFFER_LENGTH_UDP, UTL_SOCKET_RETRIES_DEFAULT, 
                    UTL_SOCKET_RETRY_INTERVAL_DEFAULT, UTL_SOCKET_NON_BLOCKING, &punpNetProtocolsPtr->iReadSocket)) != UTL_NoError ) {
                iError = UTL_NetOpenServerSocketFailed;
                goto bailFromiUtlNetAddProtocol;
            }
            
            /* Set the write socket from the read socket */
            punpNetProtocolsPtr->iWriteSocket = punpNetProtocolsPtr->iReadSocket;
        }
        else if ( punpNetProtocolsPtr->uiNetworkProtocolID == UTL_NET_PROTOCOL_TCP_ID ) {

            if ( (iError = iUtlSocketOpenServerTcp(pucHostName, punpNetProtocolsPtr->iPort, UTL_NET_BUFFER_LENGTH_TCP, UTL_SOCKET_BACKLOG_DEFAULT, 
                    UTL_SOCKET_RETRIES_DEFAULT, UTL_SOCKET_RETRY_INTERVAL_DEFAULT, UTL_SOCKET_NON_BLOCKING, &punpNetProtocolsPtr->iServerSocket)) != UTL_NoError ) {
                iError = UTL_NetOpenServerSocketFailed;
                goto bailFromiUtlNetAddProtocol;
            }
        }
        else if ( punpNetProtocolsPtr->uiNetworkProtocolID == UTL_NET_PROTOCOL_STDIO_ID ) {

            /* Set the read and write sockets from the stdin and stdout file descriptors */
            punpNetProtocolsPtr->iReadSocket = fileno(stdin);
            punpNetProtocolsPtr->iWriteSocket = fileno(stdout);
        }
    }
    else if ( punNet->uiService == UTL_NET_SERVICE_CLIENT ) {

        /* Open the socket */
        if ( punpNetProtocolsPtr->uiNetworkProtocolID == UTL_NET_PROTOCOL_UDP_ID ) {

            /* Open the socket */
            if ( (iError = iUtlSocketOpenUdp(pucHostName, punpNetProtocolsPtr->iPort, UTL_NET_BUFFER_LENGTH_UDP, UTL_SOCKET_RETRIES_DEFAULT, 
                    UTL_SOCKET_RETRY_INTERVAL_DEFAULT, UTL_SOCKET_NON_BLOCKING, &punpNetProtocolsPtr->iWriteSocket)) != UTL_NoError ) {
                iError = UTL_NetOpenClientSocketFailed;
                goto bailFromiUtlNetAddProtocol;
            }

            /* Check if we are ready to send until we timeout */
            if ( (iError = iUtlSocketReadyToSend(punpNetProtocolsPtr->iWriteSocket, punNet->iTimeOut)) != UTL_NoError ) {
                iError = UTL_NetOpenClientSocketFailed;
                goto bailFromiUtlNetAddProtocol;
            }

            /* Check the socket error */
            if ( (iError = iUtlSocketGetError(punpNetProtocolsPtr->iWriteSocket)) != UTL_NoError ) {
                iError = UTL_NetOpenClientSocketFailed;
                goto bailFromiUtlNetAddProtocol;
            }

            /* Set the read socket from the write socket */
            punpNetProtocolsPtr->iReadSocket = punpNetProtocolsPtr->iWriteSocket;
        }
        else if ( punpNetProtocolsPtr->uiNetworkProtocolID == UTL_NET_PROTOCOL_TCP_ID ) {
        
            /* Open the socket */
            if ( (iError = iUtlSocketOpenTcp(pucHostName, punpNetProtocolsPtr->iPort, UTL_NET_BUFFER_LENGTH_TCP, UTL_SOCKET_RETRIES_DEFAULT, 
                    UTL_SOCKET_RETRY_INTERVAL_DEFAULT, UTL_SOCKET_NON_BLOCKING, &punpNetProtocolsPtr->iWriteSocket)) != UTL_NoError ) {
                iError = UTL_NetOpenClientSocketFailed;
                goto bailFromiUtlNetAddProtocol;
            }

            /* Check if we are ready to send until we timeout */
            if ( (iError = iUtlSocketReadyToSend(punpNetProtocolsPtr->iWriteSocket, punNet->iTimeOut)) != UTL_NoError ) {
                iError = UTL_NetOpenClientSocketFailed;
                goto bailFromiUtlNetAddProtocol;
            }

            /* Check the socket error */
            if ( (iError = iUtlSocketGetError(punpNetProtocolsPtr->iWriteSocket)) != UTL_NoError ) {
                iError = UTL_NetOpenClientSocketFailed;
                goto bailFromiUtlNetAddProtocol;
            }

            /* Set the read socket from the write socket */
            punpNetProtocolsPtr->iReadSocket = punpNetProtocolsPtr->iWriteSocket;
        }
        else if ( punpNetProtocolsPtr->uiNetworkProtocolID == UTL_NET_PROTOCOL_STDIO_ID ) {

            /* Set the read and write sockets from the stdin and stdout file descriptors */
            punpNetProtocolsPtr->iReadSocket = fileno(stdin);
            punpNetProtocolsPtr->iWriteSocket = fileno(stdout);
        }
    }


    /* Get the buffer length */
    if ( punNet->uiService == UTL_NET_SERVICE_SERVER ) {

        /* Set and get the socket buffer length */
        if ( punpNetProtocolsPtr->uiNetworkProtocolID == UTL_NET_PROTOCOL_UDP_ID ) {
    
            /* Get the socket send buffer length */
            if ( (iError = iUtlSocketGetSendBufferLength(punpNetProtocolsPtr->iReadSocket, &punpNetProtocolsPtr->uiSendBufferLength)) != UTL_NoError ) {
                iError = UTL_NetGetSocketBufferLengthFailed;
                goto bailFromiUtlNetAddProtocol;
            }

            /* Get the socket receive buffer length */
            if ( (iError = iUtlSocketGetReceiveBufferLength(punpNetProtocolsPtr->iReadSocket, &punpNetProtocolsPtr->uiReceiveBufferLength)) != UTL_NoError ) {
                iError = UTL_NetGetSocketBufferLengthFailed;
                goto bailFromiUtlNetAddProtocol;
            }
        }
        else if ( punpNetProtocolsPtr->uiNetworkProtocolID == UTL_NET_PROTOCOL_TCP_ID ) {
    
            /* Get the socket send buffer length */
            if ( (iError = iUtlSocketGetSendBufferLength(punpNetProtocolsPtr->iServerSocket, &punpNetProtocolsPtr->uiSendBufferLength)) != UTL_NoError ) {
                iError = UTL_NetGetSocketBufferLengthFailed;
                goto bailFromiUtlNetAddProtocol;
            }

            /* Get the socket receive buffer length */
            if ( (iError = iUtlSocketGetReceiveBufferLength(punpNetProtocolsPtr->iServerSocket, &punpNetProtocolsPtr->uiReceiveBufferLength)) != UTL_NoError ) {
                iError = UTL_NetGetSocketBufferLengthFailed;
                goto bailFromiUtlNetAddProtocol;
            }
        }
        else if ( punpNetProtocolsPtr->uiNetworkProtocolID == UTL_NET_PROTOCOL_STDIO_ID ) {
            /* Set the initial buffer length */
            punpNetProtocolsPtr->uiSendBufferLength = UTL_NET_BUFFER_LENGTH_STDIO;
            punpNetProtocolsPtr->uiReceiveBufferLength = UTL_NET_BUFFER_LENGTH_STDIO;
        }
    }
    else if ( punNet->uiService == UTL_NET_SERVICE_CLIENT ) {

        /* Set and get the socket buffer length */
        if ( punpNetProtocolsPtr->uiNetworkProtocolID == UTL_NET_PROTOCOL_UDP_ID ) {
    
            /* Get the socket send buffer length */
            if ( (iError = iUtlSocketGetSendBufferLength(punpNetProtocolsPtr->iWriteSocket, &punpNetProtocolsPtr->uiSendBufferLength)) != UTL_NoError ) {
                iError = UTL_NetGetSocketBufferLengthFailed;
                goto bailFromiUtlNetAddProtocol;
            }

            /* Get the socket receive buffer length */
            if ( (iError = iUtlSocketGetReceiveBufferLength(punpNetProtocolsPtr->iWriteSocket, &punpNetProtocolsPtr->uiReceiveBufferLength)) != UTL_NoError ) {
                iError = UTL_NetGetSocketBufferLengthFailed;
                goto bailFromiUtlNetAddProtocol;
            }
        }
        else if ( punpNetProtocolsPtr->uiNetworkProtocolID == UTL_NET_PROTOCOL_TCP_ID ) {
    
            /* Get the socket send buffer length */
            if ( (iError = iUtlSocketGetSendBufferLength(punpNetProtocolsPtr->iWriteSocket, &punpNetProtocolsPtr->uiSendBufferLength)) != UTL_NoError ) {
                iError = UTL_NetGetSocketBufferLengthFailed;
                goto bailFromiUtlNetAddProtocol;
            }

            /* Get the socket receive buffer length */
            if ( (iError = iUtlSocketGetReceiveBufferLength(punpNetProtocolsPtr->iWriteSocket, &punpNetProtocolsPtr->uiReceiveBufferLength)) != UTL_NoError ) {
                iError = UTL_NetGetSocketBufferLengthFailed;
                goto bailFromiUtlNetAddProtocol;
            }
        }
        else if ( punpNetProtocolsPtr->uiNetworkProtocolID == UTL_NET_PROTOCOL_STDIO_ID ) {
            /* Set the initial buffer length */
            punpNetProtocolsPtr->uiSendBufferLength = UTL_NET_BUFFER_LENGTH_STDIO;
            punpNetProtocolsPtr->uiReceiveBufferLength = UTL_NET_BUFFER_LENGTH_STDIO;

        }
    }


    ASSERT(punpNetProtocolsPtr->uiSendBufferLength > 0);
    ASSERT(punpNetProtocolsPtr->uiReceiveBufferLength > 0);



    /* Bail label */
    bailFromiUtlNetAddProtocol:
    

    /* Handle the error */
    if ( iError == UTL_NoError ) {
            
        /* Increment the net protocol length now that we successfully added the protocol */
        punNet->uiNetProtocolsLength++;

    }
    else {

        /* Close the sockets */
        if ( punNet->uiService == UTL_NET_SERVICE_SERVER ) {
            
            if ( punpNetProtocolsPtr->uiNetworkProtocolID == UTL_NET_PROTOCOL_UDP_ID ) {
                if ( punpNetProtocolsPtr->iReadSocket != -1) {
                    iUtlSocketClose(punpNetProtocolsPtr->iReadSocket);
                    punpNetProtocolsPtr->iReadSocket = -1;
                }
            }
            else if ( punpNetProtocolsPtr->uiNetworkProtocolID == UTL_NET_PROTOCOL_TCP_ID ) {
                if ( punpNetProtocolsPtr->iServerSocket != -1 ) {
                    iUtlSocketClose(punpNetProtocolsPtr->iServerSocket);
                    punpNetProtocolsPtr->iServerSocket = -1;
                }
                if ( punpNetProtocolsPtr->iReadSocket != -1) {
                    iUtlSocketClose(punpNetProtocolsPtr->iReadSocket);
                    punpNetProtocolsPtr->iReadSocket = -1;
                }
            }
            else if ( punpNetProtocolsPtr->uiNetworkProtocolID == UTL_NET_PROTOCOL_STDIO_ID ) {
                /* Do nothing for stdio */
            }
        }
        else if ( punNet->uiService == UTL_NET_SERVICE_CLIENT ) {

            if ( punpNetProtocolsPtr->uiNetworkProtocolID == UTL_NET_PROTOCOL_UDP_ID ) {
                if ( punpNetProtocolsPtr->iWriteSocket != -1 ) {
                    iUtlSocketClose(punpNetProtocolsPtr->iWriteSocket);
                    punpNetProtocolsPtr->iWriteSocket = -1;
                }
            }
            else if ( punpNetProtocolsPtr->uiNetworkProtocolID == UTL_NET_PROTOCOL_TCP_ID ) {
                if ( punpNetProtocolsPtr->iWriteSocket != -1 ) {
                    iUtlSocketClose(punpNetProtocolsPtr->iWriteSocket);
                    punpNetProtocolsPtr->iWriteSocket = -1;
                }
            }
            else if ( punpNetProtocolsPtr->uiNetworkProtocolID == UTL_NET_PROTOCOL_STDIO_ID ) {
                /* Do nothing for stdio */
            }
        }

        /* Free the host name */
        s_free(punpNetProtocolsPtr->pucHostName);
    }


/* iUtlLogDebug(UTL_LOG_CONTEXT, "iUtlNetAddProtocol - iError: [%d]", iError); */
/* iUtlLogDebug(UTL_LOG_CONTEXT, "iUtlNetAddProtocol - punNet->uiNetProtocolsLength: [%u]", punNet->uiNetProtocolsLength); */
/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iUtlNetAddProtocol - punpNetProtocolsPtr->uiSendBufferLength: [%u], punpNetProtocolsPtr->uiReceiveBufferLength: [%u]",  */
/*             punpNetProtocolsPtr->uiSendBufferLength, punpNetProtocolsPtr->uiReceiveBufferLength); */
    
    
    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlNetAdjustBuffers()

    Purpose:    Adjust the buffers in the net structure, making
                sure that we do not lose data

    Parameters: punNet      Net structure

    Globals:    none

    Returns:    UTL error code

*/
static int iUtlNetAdjustBuffers
(
    struct utlNet *punNet
)
{

    
    int                     iError = UTL_NoError;
    struct utlNetProtocol   *punpNetProtocolsPtr = NULL;
    unsigned int            uiI = 0;
    unsigned int            uiMaxReceiveBufferLength = 0;
    unsigned int            uiMaxSendBufferLength = 0;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iUtlNetAdjustBuffers"); */

    ASSERT(punNet != NULL);


    /* Loop over the net protocols, getting the maximum receive buffer length */
    for ( punpNetProtocolsPtr = punNet->punpNetProtocols, uiI = 0; uiI < punNet->uiNetProtocolsLength; punpNetProtocolsPtr++, uiI++ ) {
        uiMaxReceiveBufferLength = UTL_MACROS_MAX(uiMaxReceiveBufferLength, punpNetProtocolsPtr->uiReceiveBufferLength);
    }

/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iUtlNetAdjustBuffers - uiMaxReceiveBufferLength: %u", uiMaxReceiveBufferLength); */

    /* Reallocate the receive buffer */
    if ( uiMaxReceiveBufferLength > punNet->uiReceiveBufferLength ) {

        unsigned int    uiReceiveBufferStartIndex = 0;
        unsigned int    uiReceiveBufferEndIndex = 0;
        unsigned char   *pucReceiveBufferPtr = NULL;

/*         iUtlLogDebug(UTL_LOG_CONTEXT, "iUtlNetAdjustBuffers - punNet->uiReceiveBufferLength: %u -> %u", punNet->uiReceiveBufferLength, uiMaxReceiveBufferLength); */

        /* Set the buffer length */
        punNet->uiReceiveBufferLength = uiMaxReceiveBufferLength;

        /* Save the offsets */
        if ( punNet->pucReceiveBuffer != NULL ) {
            uiReceiveBufferStartIndex = punNet->pucReceiveBufferStartPtr - punNet->pucReceiveBuffer;
            uiReceiveBufferEndIndex = punNet->pucReceiveBufferEndPtr - punNet->pucReceiveBuffer;
        }

        /* Reallocate the buffer */
        if ( (pucReceiveBufferPtr = (unsigned char *)s_realloc(punNet->pucReceiveBuffer, (size_t)(sizeof(unsigned char) * punNet->uiReceiveBufferLength))) == NULL ) {
            iError = UTL_MemError;
            goto bailFromiUtlNetAdjustBuffers;
        }

        /* Set the newly allocated buffer */
        punNet->pucReceiveBuffer = pucReceiveBufferPtr;

        /* Set the pointers */
        punNet->pucReceiveBufferStartPtr = punNet->pucReceiveBuffer + uiReceiveBufferStartIndex;
        punNet->pucReceiveBufferEndPtr = punNet->pucReceiveBuffer + uiReceiveBufferEndIndex;
    }


    /* Loop over the net protocols, getting the maximum send buffer length */
    for ( punpNetProtocolsPtr = punNet->punpNetProtocols, uiI = 0; uiI < punNet->uiNetProtocolsLength; punpNetProtocolsPtr++, uiI++ ) {
        uiMaxSendBufferLength = UTL_MACROS_MAX(uiMaxSendBufferLength, punpNetProtocolsPtr->uiReceiveBufferLength);
    }

/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iUtlNetAdjustBuffers - uiMaxSendBufferLength: %u", uiMaxSendBufferLength); */

    /* Reallocate the send buffer */
    if ( uiMaxSendBufferLength > punNet->uiSendBufferLength ) {

        unsigned int    uiSendBufferEndIndex = 0;
        unsigned char   *pucSendBufferPtr = NULL;

/*         iUtlLogDebug(UTL_LOG_CONTEXT, "iUtlNetAdjustBuffers - punNet->uiSendBufferLength: %u -> %u", punNet->uiSendBufferLength, uiMaxSendBufferLength); */

        /* Set the buffer length */
        punNet->uiSendBufferLength = uiMaxSendBufferLength;

        /* Save the offsets */
        if ( punNet->pucSendBuffer != NULL ) {
            uiSendBufferEndIndex = punNet->pucSendBufferEndPtr - punNet->pucSendBuffer;
        }

        /* Reallocate the buffer */
        if ( (pucSendBufferPtr = (unsigned char *)s_realloc(punNet->pucSendBuffer, (size_t)(sizeof(unsigned char) * punNet->uiSendBufferLength))) == NULL ) {
            iError = UTL_MemError;
            goto bailFromiUtlNetAdjustBuffers;
        }

        /* Set the newly allocated buffer */
        punNet->pucSendBuffer = pucSendBufferPtr;

        /* Set the pointers */
        punNet->pucSendBufferEndPtr = punNet->pucSendBuffer + uiSendBufferEndIndex;
    }


    /* Loop over the net protocols, adjusting the buffer length */
    for ( punpNetProtocolsPtr = punNet->punpNetProtocols, uiI = 0; uiI < punNet->uiNetProtocolsLength; punpNetProtocolsPtr++, uiI++ ) {

        /* Adjust the receive buffer length */
        if ( uiMaxReceiveBufferLength > punpNetProtocolsPtr->uiReceiveBufferLength ) {
/*             iUtlLogDebug(UTL_LOG_CONTEXT, "iUtlNetAdjustBuffers - punpNetProtocolsPtr->uiNetworkProtocolID: %u, punpNetProtocolsPtr->uiReceiveBufferLength: %u -> %u",  */
/*                     punpNetProtocolsPtr->uiNetworkProtocolID, punpNetProtocolsPtr->uiReceiveBufferLength, uiMaxReceiveBufferLength); */
            punpNetProtocolsPtr->uiReceiveBufferLength = uiMaxReceiveBufferLength;
        }

        /* Adjust the send buffer length */
        if ( uiMaxSendBufferLength > punpNetProtocolsPtr->uiSendBufferLength ) {
/*             iUtlLogDebug(UTL_LOG_CONTEXT, "iUtlNetAdjustBuffers - punpNetProtocolsPtr->uiNetworkProtocolID: %u, punpNetProtocolsPtr->uiSendBufferLength: %u -> %u",  */
/*                     punpNetProtocolsPtr->uiNetworkProtocolID, punpNetProtocolsPtr->uiSendBufferLength, uiMaxSendBufferLength); */
            punpNetProtocolsPtr->uiSendBufferLength = uiMaxSendBufferLength;
        }
    }



    /* Bail label */
    bailFromiUtlNetAdjustBuffers:
    

    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlNetSendTcp()

    Purpose:    Send a buffer of data down a tcp socket

    Parameters: punNet              Net structure
                pucBuffer           Buffer to send
                uiBufferLength      Length of the buffer to send

    Globals:    none

    Returns:    UTL error code

*/
static int iUtlNetSendTcp
(
    struct utlNet *punNet,
    unsigned char *pucBuffer,
    unsigned int uiBufferLength
)
{

    int                 iError = UTL_NoError;
    unsigned char       *pucBufferStartPtr = (unsigned char *)pucBuffer;
    unsigned char       *pucBufferEndPtr = NULL;
    unsigned int        uiLocalBufferLength = uiBufferLength;

    int                 iTimeOut = 0;
    struct timeval      tvStartTimeVal;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iUtlNetSendTcp - uiBufferLength: [%u]", uiBufferLength);  */


    ASSERT(punNet != NULL);
    ASSERT(pucBuffer != NULL);
    ASSERT(uiBufferLength > 0);

    ASSERT(punNet->punpNetProtocolsPtr != NULL);
    ASSERT(punNet->punpNetProtocolsPtr->iWriteSocket != -1);


    /* Initialize the timeout variables if there is a timeout */
    if ( punNet->iTimeOut > 0 ) {

        /* Set the initial timeout */
        iTimeOut = punNet->iTimeOut;

        /* Get the start time */
        s_gettimeofday(&tvStartTimeVal, NULL);
    }


    /* Loop forever, it is controlled from within */
    while ( true ) {

        /* Wait for the socket if there is a timeout */
        if ( punNet->iTimeOut >= 0 ) {
                
            /* Wait to send to the client within the timeout */
            iError = iUtlSocketReadyToSend(punNet->punpNetProtocolsPtr->iWriteSocket, iTimeOut);

            /* Handle the error */
            if ( iError == UTL_SocketNotReadyToSend ) {
                /* Timeout waiting to send data */
                return (UTL_NetTimeOut);
            }
            else if ( iError == UTL_SocketClosed ) {
                /* Socket closed */
                return (UTL_NetSocketClosed);
            }
            else if ( iError != UTL_NoError ) {
                /* Error waiting to send data */
                return (UTL_NetSendDataFailed);
            }
        }

        
        /* Send the data, send as many bytes as we can */
        iError = iUtlSocketSendTcp(punNet->punpNetProtocolsPtr->iWriteSocket, pucBufferStartPtr, uiLocalBufferLength, (void **)&pucBufferEndPtr);

/*         iUtlLogDebug(UTL_LOG_CONTEXT, "uiLocalBufferLength: %u, pucBufferStartPtr %u, pucBufferEndPtr %u, pucBufferEndPtr - pucBufferStartPtr %u",  */
/*                 uiLocalBufferLength, (unsigned int)(pucBufferStartPtr), (unsigned int)(pucBufferEndPtr), (unsigned int)(pucBufferEndPtr - pucBufferStartPtr)); */

        /* Check the error */
        if ( (iError == UTL_NoError) || (iError == UTL_SocketWouldBlock) || (iError == UTL_SocketPartialSend) ) {

            /* Adjust the number of bytes to send next time around */
            uiLocalBufferLength -= (pucBufferEndPtr - pucBufferStartPtr);

            /* Set the new end pointer for the next send */
            pucBufferStartPtr = pucBufferEndPtr;

            /* Break now if we have sent all the bytes we wanted to send */
            if ( uiBufferLength <= (pucBufferEndPtr - pucBuffer) ) {
                break;
            }

            /* Adjust the timeout */
            if ( punNet->iTimeOut >= 0 ) {
    
                struct timeval    tvCurrentTimeVal;
                struct timeval    tvDiffTimeVal;
                double            dTimeElapsed = 0;

                /* Get the current time */
                s_gettimeofday(&tvCurrentTimeVal, NULL);

                /* Get the diff time since we started the operaton */
                UTL_DATE_DIFF_TIMEVAL(tvStartTimeVal, tvCurrentTimeVal, tvDiffTimeVal);

                /* Turn the diff time elapsed into milliseconds */
                UTL_DATE_TIMEVAL_TO_MILLISECONDS(tvDiffTimeVal, dTimeElapsed);

                /* Return if we timed out, otherwise decrement the timeout */
                if ( dTimeElapsed >= punNet->iTimeOut ) {
                    return (UTL_NetTimeOut);
                }
                else {
                    iTimeOut = punNet->iTimeOut - dTimeElapsed;
                }
            }
        }
        else if ( iError == UTL_SocketClosed ) {
            /* Socket closed on us */
            return (UTL_NetSocketClosed);
        }
        else {
            /* Other error */
            return (UTL_NetSendDataFailed);
        }
    }


    return (UTL_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlNetReceiveTcp()

    Purpose:    Receive a buffer of data from a tcp socket

    Parameters: punNet              Net structure
                pucBuffer           Buffer to receive to, set to NULL if 
                                    the data should not be copied to the buffer
                uiBufferLength      Length of the buffer to receive

    Globals:    none

    Returns:    UTL error code

*/
static int iUtlNetReceiveTcp
(
    struct utlNet *punNet,
    unsigned char *pucBuffer,
    unsigned int uiBufferLength
)
{

    unsigned int        uiLocalBufferLength = 0;
    unsigned char       *pucBufferPtr = NULL;
    unsigned char       *pucBufferStartPtr = NULL;
    unsigned char       *pucBufferEndPtr = NULL;
    boolean             bBufferReceive = true;
    
    unsigned int        iTimeOut = 0;
    struct timeval      tvStartTimeVal;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iUtlNetReceiveTcp - uiBufferLength [%u]", uiBufferLength);  */


    ASSERT(punNet != NULL);
    ASSERT((pucBuffer != NULL) || (pucBuffer == NULL));
    ASSERT(uiBufferLength > 0);

    ASSERT(punNet->punpNetProtocolsPtr != NULL);
    ASSERT(punNet->punpNetProtocolsPtr->iReadSocket != -1);


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iUtlNetReceiveTcp - uiBufferLength: %u, bytes there: %u",  */
/*             uiBufferLength, (unsigned int)(punNet->pucReceiveBufferEndPtr - punNet->pucReceiveBufferStartPtr)); */

    /* If there are not enough bytes in the buffer, we get some more */
    if ( uiBufferLength > (punNet->pucReceiveBufferEndPtr - punNet->pucReceiveBufferStartPtr) ) {

        /* Is the data block we want to read too big for the buffer? */
        if ( uiBufferLength > punNet->uiReceiveBufferLength ) {

            /* Cant download this much data */
            if ( pucBuffer == NULL ) {
                return (UTL_NetInsufficientBufferSpace);
            }
            
            /* It is, so we move any bytes we have already received for that data
            ** block into our return pointer, and read the remaining bytes for
            ** our data block directly into the return pointer
            */

/*             iUtlLogDebug(UTL_LOG_CONTEXT, "iUtlNetReceiveTcp - it overflows!"); */

            /* Move any bytes we already have to the return buffer */
            if ( punNet->pucReceiveBufferEndPtr > punNet->pucReceiveBufferStartPtr ) {
                s_memcpy(pucBuffer, punNet->pucReceiveBufferStartPtr, (punNet->pucReceiveBufferEndPtr - punNet->pucReceiveBufferStartPtr));
            }

            /* Set the local read pointers */
            uiLocalBufferLength = uiBufferLength - (punNet->pucReceiveBufferEndPtr - punNet->pucReceiveBufferStartPtr);     /* Length to read */
            pucBufferPtr = pucBuffer;                                                                                       /* Start of buffer */
            pucBufferStartPtr = pucBuffer + (punNet->pucReceiveBufferEndPtr - punNet->pucReceiveBufferStartPtr);            /* Current read position */

            /* And reset the receive buffer */
            punNet->pucReceiveBufferStartPtr = punNet->pucReceiveBuffer;
            punNet->pucReceiveBufferEndPtr = punNet->pucReceiveBuffer;

            /* We are not receiving to the buffer */
            bBufferReceive = false;
        }
        else {
            /* It is not, so we just read into our buffer */

/*             iUtlLogDebug(UTL_LOG_CONTEXT, "iUtlNetReceiveTcp - it fits!"); */

            /* Shuffle any bytes we have down to the start of the buffer */
            if ( punNet->pucReceiveBufferEndPtr > punNet->pucReceiveBufferStartPtr ) {
                s_memmove(punNet->pucReceiveBuffer, punNet->pucReceiveBufferStartPtr, (punNet->pucReceiveBufferEndPtr - punNet->pucReceiveBufferStartPtr));
            }

            /* Set the global pointers */
            punNet->pucReceiveBufferEndPtr = punNet->pucReceiveBuffer + (punNet->pucReceiveBufferEndPtr - punNet->pucReceiveBufferStartPtr);
            punNet->pucReceiveBufferStartPtr = punNet->pucReceiveBuffer;

            /* Set the local read pointers */
            uiLocalBufferLength = punNet->uiReceiveBufferLength - (punNet->pucReceiveBufferEndPtr - punNet->pucReceiveBuffer);      /* Length to read */
            pucBufferPtr = punNet->pucReceiveBufferStartPtr;                                                                        /* Start of buffer */
            pucBufferStartPtr = punNet->pucReceiveBufferEndPtr;                                                                     /* Current read position */

            /* We are receiving to the buffer */
            bBufferReceive = true;
        }

        
        /* Initialize the timeout variables if there is a timeout */
        if ( punNet->iTimeOut > 0 ) {

            /* Set the initial timeout */
            iTimeOut = punNet->iTimeOut;

            /* Get the start time */
            s_gettimeofday(&tvStartTimeVal, NULL);
        }


        /* Loop until we have read at least the number of bytes we are looking for, 
        ** the loop is controlled from within 
        */
        while ( true ) {

            int iError = UTL_NoError;

            /* Wait for the socket if there is a timeout */
            if ( punNet->iTimeOut >= 0 ) {
                
                /* Wait to receive from the client within the timeout */
                iError = iUtlSocketReadyToReceive(punNet->punpNetProtocolsPtr->iReadSocket, iTimeOut);
    
                /* Handle the error */
                if ( iError == UTL_SocketNotReadyToReceive ) {
                    /* Timeout waiting to receive data */
/* iUtlLogInfo(UTL_LOG_CONTEXT, "iUtlNetReceiveTcp - iUtlSocketReadyToReceive() == UTL_SocketNotReadyToReceive");  */
                    return (UTL_NetTimeOut);
                }
                else if ( iError == UTL_SocketClosed ) {
                    /* Socket closed */
/* iUtlLogInfo(UTL_LOG_CONTEXT, "iUtlNetReceiveTcp - iUtlSocketReadyToReceive() == UTL_NetSocketClosed");  */
                    return (UTL_NetSocketClosed);
                }
                else if ( iError != UTL_NoError ) {
                    /* Error waiting to receive data */
/* iUtlLogInfo(UTL_LOG_CONTEXT, "iUtlNetReceiveTcp - iUtlSocketReadyToReceive() == %d, mapped to: UTL_NetReceiveDataFailed", iError);  */
                    return (UTL_NetReceiveDataFailed);
                }
            }


            /* Receive from the socket, read as many bytes as we can */
            iError = iUtlSocketReceiveTcp(punNet->punpNetProtocolsPtr->iReadSocket, pucBufferStartPtr, uiLocalBufferLength, (void **)&pucBufferEndPtr);

            /* Check the error */
            if ( (iError == UTL_NoError) || (iError == UTL_SocketWouldBlock) || (iError == UTL_SocketPartialReceive) ) {

/*                 iUtlLogDebug(UTL_LOG_CONTEXT, "iUtlNetReceiveTcp - uiLocalBufferLength: %u, received: %u, utl error: %d", uiLocalBufferLength, (unsigned int)(pucBufferEndPtr - pucBufferStartPtr), iError); */

                /* Adjust the number of bytes to read next time around */
                uiLocalBufferLength -= (pucBufferEndPtr - pucBufferStartPtr);
    
                /* Set the new end pointer for the next read */
                pucBufferStartPtr = pucBufferEndPtr;

                /* Break now if we have read all the bytes we were looking for */
                if ( uiBufferLength <= (pucBufferEndPtr - pucBufferPtr) ) {
                    break;
                }

                /* Adjust the timeout */
                if ( punNet->iTimeOut >= 0 ) {
    
                    struct timeval      tvCurrentTimeVal;
                    struct timeval      tvDiffTimeVal;
                    double              dTimeElapsed = 0;
    
                    /* Get the current time */
                    s_gettimeofday(&tvCurrentTimeVal, NULL);
    
                    /* Get the diff time since we started the operaton */
                    UTL_DATE_DIFF_TIMEVAL(tvStartTimeVal, tvCurrentTimeVal, tvDiffTimeVal);
    
                    /* Turn the diff time into milliseconds */
                    UTL_DATE_TIMEVAL_TO_MILLISECONDS(tvDiffTimeVal, dTimeElapsed);

                    /* Return if we timed out, otherwise decrement the timeout */
                    if ( dTimeElapsed >= punNet->iTimeOut ) {
                        return (UTL_NetTimeOut);
                    }
                    else {
                        iTimeOut = punNet->iTimeOut - dTimeElapsed;
                    }
                }
                
                /* Reset the error */
                iError = UTL_NoError;
            }
            else if ( iError == UTL_SocketClosed ) {
/* iUtlLogInfo(UTL_LOG_CONTEXT, "iUtlNetReceiveTcp - iUtlSocketReceiveTcp() == UTL_NetSocketClosed");  */
                /* Socket close on us */
                return (UTL_NetSocketClosed);
            }
            else {
                /* Other error */
/* iUtlLogInfo(UTL_LOG_CONTEXT, "iUtlNetReceiveTcp - iUtlSocketReceiveTcp() == %d, mapped to: UTL_NetReceiveDataFailed", iError);  */
                return (UTL_NetReceiveDataFailed);
            }
        }


        /* Set the end of the buffer */
        if ( bBufferReceive == true ) {
            /* Set the end pointer for the next read */
            punNet->pucReceiveBufferEndPtr = pucBufferEndPtr;
        }
    }



    /* We need to shuffle pointers and copy data from the buffer if we did a buffer 
    ** read as opposed to a direct read into the return buffer
    */
    if ( bBufferReceive == true ) {

        /* Copy the data read into the buffer */
        if ( pucBuffer != NULL ) {
    
            /* Copy the data from the buffer */
            s_memcpy(pucBuffer, punNet->pucReceiveBufferStartPtr, uiBufferLength);

            /* Increment the read pointer */
            punNet->pucReceiveBufferStartPtr += uiBufferLength;
        }
    }


    return (UTL_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlNetSendUdp()

    Purpose:    Send the data in the send buffer down a udp socket

    Parameters: punNet      Net structure

    Globals:    none

    Returns:    UTL error code

*/
static int iUtlNetSendUdp
(
    struct utlNet *punNet
)
{

    int     iError = UTL_NoError;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iUtlNetSendUdp - length [%d]", (int)(punNet->pucSendBufferEndPtr - punNet->pucSendBuffer));  */


    ASSERT(punNet != NULL);

    ASSERT(punNet->punpNetProtocolsPtr != NULL);
    ASSERT(punNet->punpNetProtocolsPtr->iWriteSocket != -1);


    /* Send the data, note that, if we are a server, we pass the client 
    ** socket address, this is how we know where to send the message 
    */
    iError = iUtlSocketSendUdp(punNet->punpNetProtocolsPtr->iWriteSocket, punNet->pucSendBuffer, punNet->pucSendBufferEndPtr - punNet->pucSendBuffer, 
            (punNet->uiService == UTL_NET_SERVICE_SERVER) ? punNet->punpNetProtocolsPtr->psiSocketAddress : NULL, NULL);
    
    /* Check the error */
    if ( (iError != UTL_NoError) && (iError != UTL_SocketPartialSend) ) {
        return (UTL_NetSendDataFailed);
    }


    /* Free the client socket address pointer */
    if ( punNet->uiService == UTL_NET_SERVICE_SERVER ) {
        s_free(punNet->punpNetProtocolsPtr->psiSocketAddress);
    }


    /* Reset the send buffer pointers, each udp packet is independent */
    iUtlNetResetSend((void *)punNet);


    return (UTL_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlNetReceiveUdp()

    Purpose:    Receive a buffer of data from a udp socket

    Parameters: punNet      Net structure

    Globals:    none

    Returns:    UTL error code

*/
static int iUtlNetReceiveUdp
(
    struct utlNet *punNet
)
{


    int     iError = UTL_NoError;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iUtlNetReceiveUdp - length [%u]", punNet->punpNetProtocolsPtr->uiReceiveBufferLength);  */


    ASSERT(punNet != NULL);

    ASSERT(punNet->punpNetProtocolsPtr != NULL);
    ASSERT(punNet->punpNetProtocolsPtr->iReadSocket != -1);


    /* Wait for the socket if there is a timeout */
    if ( punNet->iTimeOut >= 0 ) {

        /* Wait to receive from the client within the timeout */
        iError = iUtlSocketReadyToReceive(punNet->punpNetProtocolsPtr->iReadSocket, punNet->iTimeOut);

        /* Handle the error */
        if ( iError == UTL_SocketNotReadyToReceive ) {
            /* Timeout waiting to receive data */
            return (UTL_NetTimeOut);
        }
        else if ( iError == UTL_SocketClosed ) {
            /* Socket closed */
            return (UTL_NetSocketClosed);
        }
        else if ( iError != UTL_NoError ) {
            /* Error waiting to receive data */
            return (UTL_NetReceiveDataFailed);
        }
    }


    /* Reset the receive buffer pointers, each udp packet is independent */
    iUtlNetResetReceive((void *)punNet);


    /* Receive data, note that, if we are a server, we pass a return pointer to 
    ** get the client socket address so we know where to send the response
    */
    iError = iUtlSocketReceiveUdp(punNet->punpNetProtocolsPtr->iReadSocket, punNet->pucReceiveBuffer, punNet->punpNetProtocolsPtr->uiReceiveBufferLength, 
            (void **)&punNet->pucReceiveBufferEndPtr, (punNet->uiService == UTL_NET_SERVICE_SERVER) ? &punNet->punpNetProtocolsPtr->psiSocketAddress : NULL);
            
    /* Check the error */
    if ( (iError != UTL_NoError) && (iError != UTL_SocketWouldBlock) && (iError != UTL_SocketPartialReceive) ) {
        return (UTL_NetReceiveDataFailed);
    }


    /* Remap the error*/
    return ((iError == UTL_SocketWouldBlock) ? UTL_NetWouldBlock : UTL_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlMapSocketError()

    Purpose:    Map a socket error to a net error

    Parameters: iSocketError        socket error
                iDefaultNetError    default net error

    Globals:    none

    Returns:    UTL error code

*/
static int iUtlMapSocketError
(
    int iSocketError,
    int iDefaultNetError
)
{

    /* Map the socket error to the net error */
    switch ( iSocketError ) {
    
        case UTL_NoError:
            return (UTL_NoError);
            
        case UTL_SocketInvalidSocket:
            return (UTL_NetInvalidNet);
        
        case UTL_SocketInvalidTimeOut:
            return (UTL_NetInvalidTimeOut);
        
        case UTL_SocketInvalidDescriptor:
            return (UTL_NetSocketInvalidDescriptor);
        
        case UTL_SocketPollError:
            return (UTL_NetSocketPollError);
        
        case UTL_SocketError:
            return (UTL_NetSocketError);
        
        case UTL_SocketNotReadyToSend:
            return (UTL_NetSocketNotReadyToSend);
        
        case UTL_SocketNotReadyToReceive:
            return (UTL_NetSocketNotReadyToReceive);
        
        case UTL_SocketClosed:
            return (UTL_NetSocketClosed);
        
        case UTL_SocketGetSocketOptionFailed:
            return (UTL_SocketGetSocketOptionFailed);
    }


    /* Return the default net error */
    return (iDefaultNetError);

}



/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlNetPrint()

    Purpose:    Print out a net structure. 

    Parameters: punNet      Net structure

    Globals:    none

    Returns:    UTL error code

*/
static int iUtlNetPrint
(
    struct utlNet *punNet
)
{

    struct utlNetProtocol   *punpNetProtocolsPtr = NULL;
    unsigned int            uiI = 0;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iUtlNetPrint"); */

    ASSERT(punNet != NULL);


    printf("\n");
    printf("punNet->uiService                            : %u\n", punNet->uiService);
    printf("punNet->iTimeOut                             : %d\n", punNet->iTimeOut);
    printf("\n");

    for ( punpNetProtocolsPtr = punNet->punpNetProtocols, uiI = 0; uiI < punNet->uiNetProtocolsLength; punpNetProtocolsPtr++, uiI++ ) {
            printf("%s", (punpNetProtocolsPtr == punNet->punpNetProtocolsPtr) ? "*" : " ");
            printf(" punpNetProtocolsPtr->uiNetworkProtocolID    : %d\n", punpNetProtocolsPtr->uiNetworkProtocolID);
            printf("  punpNetProtocolsPtr->iPort                 : %d\n", punpNetProtocolsPtr->iPort);
            printf("  punpNetProtocolsPtr->iServerSocket         : %d\n", punpNetProtocolsPtr->iServerSocket);
            printf("  punpNetProtocolsPtr->iReadSocket           : %d\n", punpNetProtocolsPtr->iReadSocket);
            printf("  punpNetProtocolsPtr->iWriteSocket          : %d\n", punpNetProtocolsPtr->iWriteSocket);
            printf("  punpNetProtocolsPtr->uiReceiveBufferLength : %u\n", punpNetProtocolsPtr->uiReceiveBufferLength);
            printf("  punpNetProtocolsPtr->uiReceiveBufferLength : %u\n", punpNetProtocolsPtr->uiReceiveBufferLength);
            printf("\n");
    }

    printf("punNet->uiReceiveBufferLength                : %u\n", punNet->uiReceiveBufferLength);
    printf("Receive Buffer Size                          : %ld\n", (long int)(punNet->pucReceiveBufferEndPtr - punNet->pucReceiveBufferStartPtr));
    printf("punNet->uiSendBufferLength                   : %u\n", punNet->uiSendBufferLength);
    printf("Send Buffer Size                             : %ld\n", (long int)(punNet->pucSendBufferEndPtr - punNet->pucSendBuffer));

    printf("\n\n\n");


    return (UTL_NoError);

}


/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlNetGetHostName()

    Purpose:    Get the host name

    Parameters: pucHostName     return pointer for the host name
                uiHostNameLen   length of the pointer

    Globals:    none

    Returns:    UTL error code

*/
int iUtlNetGetHostName
(
    unsigned char *pucHostName,
    unsigned int uiHostNameLen
)
{

    /* Check the parameters */
    if ( pucHostName == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pucHostName' parameter passed to 'iUtlNetGetHostName'."); 
        return (UTL_ReturnParameterError);
    }

    if ( uiHostNameLen <= 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiHostNameLen' parameter passed to 'iUtlNetGetHostName'."); 
        return (UTL_ReturnParameterError);
    }


    /* Get the host name, set the default if we cant get it */
    if ( gethostname(pucHostName, uiHostNameLen) != 0 ) {
        s_strnncpy(pucHostName, UTL_NET_HOST_NAME_DEFAULT, uiHostNameLen);
    }


    return (UTL_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlNetGetHostAddress()

    Purpose:    Get the host ip address

    Parameters: pucHostAddress      return pointer for the host address
                uiHostAddressLen    length of the pointer

    Globals:    none

    Returns:    UTL error code

*/
int iUtlNetGetHostAddress
(
    unsigned char *pucHostAddress,
    unsigned int uiHostAddressLen
)
{

    unsigned char   pucHostName[MAXHOSTNAMELEN + 1] = {'\0'};


    /* Check the parameters */
    if ( pucHostAddress == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pucHostAddress' parameter passed to 'iUtlNetGetHostAddress'."); 
        return (UTL_ReturnParameterError);
    }

    if ( uiHostAddressLen <= 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiHostAddressLen' parameter passed to 'iUtlNetGetHostAddress'."); 
        return (UTL_ReturnParameterError);
    }


    /* Get the host name */
    if ( gethostname(pucHostName, MAXHOSTNAMELEN) == 0 ) {

        struct hostent    *pheHost = NULL;

        /* Get the host information by name and check that we got the host address list */
        if ( ((pheHost = gethostbyname(pucHostName)) != NULL) && (pheHost->h_addr_list != NULL) & (pheHost->h_addr_list[0] != NULL) ) {
    
            /* Get the host address list into a dot format host address */
            snprintf(pucHostAddress, uiHostAddressLen, "%d.%d.%d.%d", (unsigned char)pheHost->h_addr_list[0][0], (unsigned char)pheHost->h_addr_list[0][1],
                    (unsigned char)pheHost->h_addr_list[0][2], (unsigned char)pheHost->h_addr_list[0][3]);
        }
    }

    /* Set the host address, set the default if we did not get it */
    if ( bUtlStringsIsStringNULL(pucHostAddress) == true ) {    
        s_strnncpy(pucHostAddress, UTL_NET_HOST_ADDRESS_DEFAULT, uiHostAddressLen);
    }


    return (UTL_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlNetConvertStringHostAddress()

    Purpose:    This function converts a string host address to a numeric host address

    Parameters: pucHostAddress      host address
                puiHostAddress      return pointer for the numeric host address 

    Globals:    none

    Returns:    UTL error code

*/
int iUtlNetConvertStringHostAddress
(
    unsigned char *pucHostAddress, 
    unsigned int *puiHostAddress
)
{


    unsigned int    uiByte1 = 0;
    unsigned int    uiByte2 = 0;
    unsigned int    uiByte3 = 0;
    unsigned int    uiByte4 = 0;
    unsigned int    uiHostAddress = 0;


    /* Check the parameters */
    if ( bUtlStringsIsStringNULL(pucHostAddress) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pucHostAddress' parameter passed to 'iUtlNetConvertStringHostAddress'."); 
        return (UTL_NetInvalidHostAddress);
    }

    if ( puiHostAddress == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'puiHostAddress' parameter passed to 'iUtlNetConvertStringHostAddress'."); 
        return (UTL_ReturnParameterError);
    }


    /* Parse out the four bytes in the host address */
    if ( sscanf(pucHostAddress, "%d.%d.%d.%d", &uiByte1, &uiByte2, &uiByte3, &uiByte4) != 4 ) {
        return (UTL_SocketInvalidHostAddress);
    }

    /* Create the numeric host address from the four bytes */
    uiHostAddress += uiByte1;
    uiHostAddress <<= 8;
    uiHostAddress += uiByte2;
    uiHostAddress <<= 8;
    uiHostAddress += uiByte3;
    uiHostAddress <<= 8;
    uiHostAddress += uiByte4;

    /* Set the return pointer */
    *puiHostAddress = uiHostAddress;


    return (UTL_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlNetConvertNumericHostAddress()

    Purpose:    This function converts a numeric host address to a string host address

    Parameters: uiHostAddress       numeric host address
                pucHostAddress      return pointer for the string host address
                uiHostAddressLen    length of the pointer

    Globals:    none

    Returns:    UTL error code

*/
int iUtlNetConvertNumericHostAddress
(
    unsigned int uiHostAddress,
    unsigned char *pucHostAddress,
    unsigned int uiHostAddressLen
)
{

    unsigned int    uiByte1 = 0;
    unsigned int    uiByte2 = 0;
    unsigned int    uiByte3 = 0;
    unsigned int    uiByte4 = 0;


    /* Check the parameters */
    if ( pucHostAddress == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pucHostAddress' parameter passed to 'iUtlNetConvertNumericHostAddress'."); 
        return (UTL_ReturnParameterError);
    }

    if ( uiHostAddressLen <= 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiHostAddressLen' parameter passed to 'iUtlNetConvertNumericHostAddress'."); 
        return (UTL_ReturnParameterError);
    }


    /* Get the four bytes from the numeric host address */
    uiByte4 = (uiHostAddress & 0x000000FF) >> 0;
    uiByte3 = (uiHostAddress & 0x0000FF00) >> 8;
    uiByte2 = (uiHostAddress & 0x00FF0000) >> 16;
    uiByte1 = (uiHostAddress & 0xFF000000) >> 24;

    /* Create string representation of the host address */
    snprintf(pucHostAddress, uiHostAddressLen, "%u.%u.%u.%u", uiByte1, uiByte2, uiByte3, uiByte4);


    return (UTL_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlNetGetNetMaskStringHostAddress()

    Purpose:    This function returns the netmask for a
                string host address

    Parameters: pucHostAddress      host address
                pucNetMask          return pointer for the net mask
                uiNetMaskLen        length of the pointer

    Globals:    none

    Returns:    UTL error code

*/
int iUtlNetGetNetMaskStringHostAddress
(
    unsigned char *pucHostAddress, 
    unsigned char *pucNetMask, 
    unsigned int uiNetMaskLen
)
{

    int             iError = UTL_NoError;
    unsigned int    uiHostAddress = 0;
    unsigned int    uiNetMask = 0;


    /* Check the parameters */
    if ( bUtlStringsIsStringNULL(pucHostAddress) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pucHostAddress' parameter passed to 'iUtlNetGetNetMaskStringHostAddress'."); 
        return (UTL_NetInvalidHostAddress);
    }

    if ( pucNetMask == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pucNetMask' parameter passed to 'iUtlNetGetNetMaskStringHostAddress'."); 
        return (UTL_ReturnParameterError);
    }

    if ( uiNetMaskLen <= 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiNetMaskLen' parameter passed to 'iUtlNetGetNetMaskStringHostAddress'."); 
        return (UTL_ReturnParameterError);
    }


    /* Convert the string host address to a numeric host address */
    if ( (iError = iUtlNetConvertStringHostAddress(pucHostAddress, &uiHostAddress)) != UTL_NoError ) {
        return (iError);
    }

    /* Get the net mask for this host address */
    if ( (iError = iUtlNetGetNetMaskNumericHostAddress(uiHostAddress, &uiNetMask)) != UTL_NoError ) {
        return (iError);
    }

    /* Convert the net mask to a string host address */
    if ( (iError = iUtlNetConvertNumericHostAddress(uiNetMask, pucNetMask, uiNetMaskLen)) != UTL_NoError ) {
        return (iError);
    }


    return (UTL_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlNetGetNetMaskNumericHostAddress()

    Purpose:    This function returns the netmask for a
                numeric host address

    Parameters: uiHostAddress   host address
                puiNetMask      return pointer for the net mask

    Globals:    none

    Returns:    UTL error code

*/
int iUtlNetGetNetMaskNumericHostAddress
(
    unsigned int uiHostAddress, 
    unsigned int *puiNetMask
)
{

    /* Check the parameters */
    if ( puiNetMask == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'puiNetMask' parameter passed to 'iUtlNetGetNetMaskNumericHostAddress'."); 
        return (UTL_ReturnParameterError);
    }


    /* Copy the netmask */
    if ( (uiHostAddress & 0x80000000) == 0 ) {
        *puiNetMask = 0xFF000000;
    }
    else if ( (uiHostAddress & 0xC0000000) == 0x80000000 ) {
        *puiNetMask = 0xFFFF0000;
    }
    else if ( (uiHostAddress & 0xE0000000) == 0xC0000000 ) {
        *puiNetMask = 0xFFFFFF00;
    }
    else {
        *puiNetMask = 0xFFFFFFF0;
    }


    return (UTL_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlNetGetNetMaskBitsStringHostAddress()

    Purpose:    This function returns the netmask bits for a
                string host address

    Parameters: pucHostAddress      host address
                puiNetMaskBits      return pointer for the net mask bits

    Globals:    none

    Returns:    UTL error code

*/
int iUtlNetGetNetMaskBitsStringHostAddress
(
    unsigned char *pucHostAddress, 
    unsigned int *puiNetMaskBits 
)
{

    int             iError = UTL_NoError;
    unsigned int    uiHostAddress = 0;


    /* Check the parameters */
    if ( bUtlStringsIsStringNULL(pucHostAddress) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pucHostAddress' parameter passed to 'iUtlNetGetNetMaskBitsStringHostAddress'."); 
        return (UTL_NetInvalidHostAddress);
    }

    if ( puiNetMaskBits == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'puiNetMaskBits' parameter passed to 'iUtlNetGetNetMaskBitsStringHostAddress'."); 
        return (UTL_ReturnParameterError);
    }


    /* Convert the string host address to a numeric host address */
    if ( (iError = iUtlNetConvertStringHostAddress(pucHostAddress, &uiHostAddress)) != UTL_NoError ) {
        return (iError);
    }

    /* Get the net mask bits for this host address */
    if ( (iError = iUtlNetGetNetMaskBitsNumericHostAddress(uiHostAddress, puiNetMaskBits)) != UTL_NoError ) {
        return (iError);
    }


    return (UTL_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlNetGetNetMaskBitsNumericHostAddress()

    Purpose:    This function returns the netmask bits for a
                numeric host address

    Parameters: uiHostAddress       host address
                puiNetMaskBits      return pointer for the net mask bits

    Globals:    none

    Returns:    UTL error code

*/
int iUtlNetGetNetMaskBitsNumericHostAddress
(
    unsigned int uiHostAddress, 
    unsigned int *puiNetMaskBits
)
{

    /* Check the parameters */
    if ( puiNetMaskBits == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'puiNetMaskBits' parameter passed to 'iUtlNetGetNetMaskBitsNumericHostAddress'."); 
        return (UTL_ReturnParameterError);
    }


    /* Copy the netmask */
    if ( (uiHostAddress & 0x80000000) == 0 ) {
        *puiNetMaskBits = 8;
    }
    else if ( (uiHostAddress & 0xC0000000) == 0x80000000 ) {
        *puiNetMaskBits = 16;
    }
    else if ( (uiHostAddress & 0xE0000000) == 0xC0000000 ) {
        *puiNetMaskBits = 24;
    }
    else {
        *puiNetMaskBits = 28;
    }


    return (UTL_NoError);

}


/*---------------------------------------------------------------------------*/
