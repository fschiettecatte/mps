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

    Module:     srvr_lwps.c

    Author:     Francois Schiettecatte (FS Consulting LLC.)

    Created:    20 December 1995

    Purpose:    This module implements the 'lwps' protocol handler.

*/


/*---------------------------------------------------------------------------*/


/*
** Includes
*/

#include "utils.h"

#include "spi.h"

#include "lwps.h"

#include "server.h"

#include "srvr_lwps.h"


/*---------------------------------------------------------------------------*/


/*
** Feature defines
*/

/* Context for logging */
#undef UTL_LOG_CONTEXT
#define UTL_LOG_CONTEXT                     (unsigned char *)"com.fsconsult.mps.src.server.srvr_lwps"


/* Enable the logging of spi errors */
#define SRVR_LWPS_ENABLE_SPI_ERROR_LOGGING


/*---------------------------------------------------------------------------*/


/*
** Defines
*/

#define SRVR_LWPS_LONG_STRING_LENGTH            (4096)


/* Messages */
#define SRVR_LWPS_LOAD_EXCEEDED_MESSAGE         (unsigned char *)"Rejecting the client request because the server load is too high"


/*---------------------------------------------------------------------------*/


/*
** Private function prototypes
*/

static int iSrvrLwpsHandleInit (struct srvrServerSession *psssSrvrServerSession, void *pvLwps);

static int iSrvrLwpsHandleSearch (struct srvrServerSession *psssSrvrServerSession, void *pvLwps);

static int iSrvrLwpsHandleRetrieval (struct srvrServerSession *psssSrvrServerSession, void *pvLwps);

static int iSrvrLwpsHandleServerInfo (struct srvrServerSession *psssSrvrServerSession, void *pvLwps);

static int iSrvrLwpsHandleServerIndexInfo (struct srvrServerSession *psssSrvrServerSession, void *pvLwps);

static int iSrvrLwpsHandleIndexInfo (struct srvrServerSession *psssSrvrServerSession, void *pvLwps);

static int iSrvrLwpsHandleIndexFieldInfo (struct srvrServerSession *psssSrvrServerSession, void *pvLwps);

static int iSrvrLwpsHandleIndexTermInfo (struct srvrServerSession *psssSrvrServerSession, void *pvLwps);

static int iSrvrLwpsHandleDocumentInfo (struct srvrServerSession *psssSrvrServerSession, void *pvLwps);


static int iSrvrLwpsCheckLoadForRejection (void *pvLwps, unsigned char *pucReferenceID, 
        double dLoadMaximum, boolean *pbRejected);


static int iSrvrLwpsOpenIndex (struct spiSession *pssSpiSession, unsigned char *pucIndexName, 
        void **ppvIndex, unsigned char **ppucErrorString);
static int iSrvrLwpsCloseIndex (struct spiSession *pssSpiSession, void *pvIndex);


static int iSrvrLwpsSearchIndex (struct spiSession *pssSpiSession, unsigned char **ppucIndexNameList, 
        unsigned char *pucLanguageCode, unsigned char *pucSearchText, 
        unsigned char *pucPositiveFeedbackText, unsigned char *pucNegativeFeedbackText, 
        unsigned int uiStartIndex, unsigned int uiEndIndex, 
        struct spiSearchResponse **ppssrSpiSearchResponse, unsigned char **ppucErrorString);

static int iSrvrLwpsRetrieveDocument (struct spiSession *pssSpiSession, unsigned char *pucIndexName,
        unsigned char *pucDocumentKey, unsigned char *pucItemName, unsigned char *pucMimeType, unsigned int uiChunkType, 
        unsigned int uiChunkStart, unsigned int uiChunkEnd, void **ppvData, unsigned int *puiDataLength, 
        unsigned char **ppucErrorString);

static int iSrvrLwpsGetServerInfo (struct spiSession *pssSpiSession, 
        struct spiServerInfo **ppssiSpiServerInfo, unsigned char **ppucErrorString);

static int iSrvrLwpsGetServerIndexInfo (struct spiSession *pssSpiSession,
        struct spiServerIndexInfo **ppssiiSpiServerIndexInfos, unsigned int *puiSpiServerIndexInfosLength, 
        unsigned char **ppucErrorString);

static int iSrvrLwpsGetIndexInfo (struct spiSession *pssSpiSession, 
        unsigned char *pucIndexName, struct spiIndexInfo **ppsiiSpiIndexInfo,
        unsigned char **ppucErrorString);

static int iSrvrLwpsGetIndexFieldInfo (struct spiSession *pssSpiSession,
        unsigned char *pucIndexName, struct spiFieldInfo **psfiSpiFieldInfos, 
        unsigned int *uiSpiFieldInfosLength, unsigned char **ppucErrorString);

static int iSrvrLwpsGetIndexTermInfo (struct spiSession *pssSpiSession,
        unsigned char *pucIndexName, unsigned int uiTermMatch, unsigned int uiTermCase, unsigned char *pucTerm,
        unsigned char *pucFieldName, struct spiTermInfo **ppstiSpiTermInfos, unsigned int *puiSpiTermInfosLength, 
        unsigned char **ppucErrorString);

static int iSrvrLwpsGetDocumentInfo (struct spiSession *pssSpiSession,
        unsigned char *pucIndexName, unsigned char *pucDocumentKey, struct spiDocumentInfo **ppsdiSpiDocumentInfo, 
        unsigned char **ppucErrorString);

static int iSrvrLwpsHandleSpiError (int iError, unsigned char *pucIndexName, unsigned char **ppucErrorString);


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrvrProtocolHandlerLwps()

    Purpose:    Serve an LWPS request, this function reads in the request,
                interprets it and sends the reply to the client.

                When this function is called the client has been accepted and 
                the message has been received.

    Parameters: psssSrvrServerSession   server session structure

    Globals:    none

    Returns:    SPI error code

*/
int iSrvrProtocolHandlerLwps
(
    struct srvrServerSession *psssSrvrServerSession
)
{

    int             iUtlError = UTL_NoError;
    int             iLwpsError = LWPS_NoError;
    void            *pvLwps = NULL;
    unsigned int    uiProtocolType = UTL_NET_PROTOCOL_TYPE_INVALID_ID;
    unsigned int    uiMessageID = LWPS_INVALID_ID;


    /* Check the parameters */
    if ( psssSrvrServerSession == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'psssSrvrServerSession' parameter passed to 'iSrvrProtocolHandlerLwps'."); 
        return (SPI_ParameterError);
    }


    /* Create an lwps structure */
    if ( (iLwpsError = iLwpsCreate(psssSrvrServerSession->pvUtlNet, &pvLwps)) != LWPS_NoError ) { 
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to create an lwps, lwps error: %d.", iLwpsError);
        goto bailFromiSrvrProtocolHandlerLwps;
    }


    /* Get the connected protocol type */
    if ( (iUtlError = iUtlNetGetConnectedProtocolType(psssSrvrServerSession->pvUtlNet, &uiProtocolType)) != UTL_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the net connected protocol type, utl error: %d.", iUtlError);
        goto bailFromiSrvrProtocolHandlerLwps;
    }


    /* Loop forever, we control the loop from within */
    while ( true ) {

        /* Peek into the header */
        if ( (iLwpsError = iLwpsHeaderPeek(pvLwps, &uiMessageID)) != LWPS_NoError ) {
            goto bailFromiSrvrProtocolHandlerLwps;
        }


        /* Handle the message */
        switch ( uiMessageID ) {

            case LWPS_INIT_REQUEST_ID:
                iLwpsError = iSrvrLwpsHandleInit(psssSrvrServerSession, pvLwps);
                break;


            case LWPS_SEARCH_REQUEST_ID:
                iLwpsError = iSrvrLwpsHandleSearch(psssSrvrServerSession, pvLwps);
                break;


            case LWPS_RETRIEVAL_REQUEST_ID:
                iLwpsError = iSrvrLwpsHandleRetrieval(psssSrvrServerSession, pvLwps);
                break;


            case LWPS_SERVER_INFO_REQUEST_ID:
                iLwpsError = iSrvrLwpsHandleServerInfo(psssSrvrServerSession, pvLwps);
                break;


            case LWPS_SERVER_INDEX_INFO_REQUEST_ID:
                iLwpsError = iSrvrLwpsHandleServerIndexInfo(psssSrvrServerSession, pvLwps);
                break;


            case LWPS_INDEX_INFO_REQUEST_ID:
                iLwpsError = iSrvrLwpsHandleIndexInfo(psssSrvrServerSession, pvLwps);
                break;


            case LWPS_INDEX_FIELD_INFO_REQUEST_ID:
                iLwpsError = iSrvrLwpsHandleIndexFieldInfo(psssSrvrServerSession, pvLwps);
                break;


            case LWPS_INDEX_TERM_INFO_REQUEST_ID:
                iLwpsError = iSrvrLwpsHandleIndexTermInfo(psssSrvrServerSession, pvLwps);
                break;


            case LWPS_DOCUMENT_INFO_REQUEST_ID:
                iLwpsError = iSrvrLwpsHandleDocumentInfo(psssSrvrServerSession, pvLwps);
                break;


            default:
                iUtlLogWarn(UTL_LOG_CONTEXT, "Invalid message from client.");
                goto bailFromiSrvrProtocolHandlerLwps;
        }


        /* Bail out on error */
        if ( iLwpsError != LWPS_NoError ) {
            goto bailFromiSrvrProtocolHandlerLwps;
        }


        /* Break out here if we are connected via a message based protocol */
        if ( uiProtocolType == UTL_NET_PROTOCOL_TYPE_MESSAGE_ID ) {
            break;
        }


        /* Receive the next message */
        if ( (iUtlError = iUtlNetReceive(psssSrvrServerSession->pvUtlNet)) != UTL_NoError ) {
            goto bailFromiSrvrProtocolHandlerLwps;
        }
    }



    /* Bail label */
    bailFromiSrvrProtocolHandlerLwps:


    /* Timeout waiting to receive data */
    if ( iUtlError == UTL_NetTimeOut ) {
        iUtlLogWarn(UTL_LOG_CONTEXT, "The socket timed out while waiting to receive data, utl error: %d.", iUtlError);
    }

    /* Report bad errors */
    else if ( (iUtlError != UTL_NoError) && (iUtlError != UTL_NetSocketClosed) ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to receive an lwps message from the client, utl error: %d.", iUtlError);
    }

    /* Timeout waiting to receive data */
    else if ( iLwpsError == LWPS_TimeOut ) {
        iUtlLogWarn(UTL_LOG_CONTEXT, "The socket timed out while interacting with the client, lwps error: %d.", iLwpsError);
    }

    /* Report bad errors */
    else if ( (iLwpsError != LWPS_NoError) && (iLwpsError != LWPS_SocketClosed) ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to handle the request, lwps error: %d.", iLwpsError);
    }


    /* Free the lwps structure */
    if ( pvLwps != NULL ) {
        iLwpsFree(pvLwps);
    }


    return (SPI_NoError);

}


/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/


/*

    Function:   iSrvrLwpsHandleInit()

    Purpose:    Handle the init exchange

    Parameters: psssSrvrServerSession   server session structure
                pvLwps                  lwps structure

    Globals:    none

    Returns:    LWPS error code

*/
static int iSrvrLwpsHandleInit
(
    struct srvrServerSession *psssSrvrServerSession,
    void *pvLwps
)
{

    int             iError = LWPS_NoError;
    unsigned char   *pucUserName = NULL;
    unsigned char   *pucPassword = NULL;
    unsigned char   *pucReferenceID = NULL;
    boolean         bRejected = false;


    ASSERT(psssSrvrServerSession != NULL);    
    ASSERT(pvLwps != NULL);    


    /* Receive the init request */
    if ( (iError = iLwpsInitRequestReceive(pvLwps, &pucUserName, &pucPassword, &pucReferenceID)) != LWPS_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to receive an init request message, lwps error: %d.", iError);
        goto bailFromiSrvrLwpsHandleInit;
    }


    /* Check the current load and reject if the connection load was exceeded */
    if ( (iError = iSrvrLwpsCheckLoadForRejection(pvLwps, pucReferenceID, psssSrvrServerSession->dConnectionLoadMaximum, &bRejected)) != LWPS_NoError ) {
        goto bailFromiSrvrLwpsHandleInit;
    }

    /* Bail if the init request was rejected */
    if ( bRejected == true ) {
        goto bailFromiSrvrLwpsHandleInit;
    }


    /* Send the init response */
    if ( (iError = iLwpsInitResponseSend(pvLwps, pucReferenceID)) != LWPS_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to send an init response message, lwps error: %d.", iError);
        goto bailFromiSrvrLwpsHandleInit;
    }



    /* Bail label */
    bailFromiSrvrLwpsHandleInit:


    /* Free allocated pointers */
    s_free(pucUserName);
    s_free(pucPassword);
    s_free(pucReferenceID);


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrvrLwpsHandleSearch()

    Purpose:    Run a search on a index

    Parameters: psssSrvrServerSession   server session structure
                pvLwps                  lwps structure

    Globals:    none

    Returns:    LWPS error code

*/
static int iSrvrLwpsHandleSearch
(
    struct srvrServerSession *psssSrvrServerSession,
    void *pvLwps
)
{

    int                         iError = LWPS_NoError;
    unsigned char               **ppucIndexNameList = NULL;
    unsigned char               *pucLanguageCode = NULL;
    unsigned char               *pucSearchText = NULL;
    unsigned char               *pucPositiveFeedbackText = NULL;
    unsigned char               *pucNegativeFeedbackText = NULL;
    unsigned int                uiStartIndex = 0;
    unsigned int                uiEndIndex = 0;
    unsigned char               *pucReferenceID = NULL;
    boolean                     bRejected = false;
    struct spiSearchResponse    *pssrSpiSearchResponse = NULL;
    int                         iErrorCode = SPI_NoError;
    unsigned char               *pucErrorString = NULL;


    ASSERT(psssSrvrServerSession != NULL);    
    ASSERT(pvLwps != NULL);    


    /* Receive the search request */
    if ( (iError = iLwpsSearchRequestReceive(pvLwps, &ppucIndexNameList, &pucLanguageCode, &pucSearchText, 
            &pucPositiveFeedbackText, &pucNegativeFeedbackText, &uiStartIndex, &uiEndIndex, &pucReferenceID)) != LWPS_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to receive an lwps search request message, lwps error: %d.", iError);
        goto bailFromiSrvrLwpsHandleSearch;
    }


    /* Check the current load and reject if the search load was exceeded */
    if ( (iError = iSrvrLwpsCheckLoadForRejection(pvLwps, pucReferenceID, psssSrvrServerSession->dSearchLoadMaximum, &bRejected)) != LWPS_NoError ) {
        goto bailFromiSrvrLwpsHandleSearch;
    }

    /* Bail if the search request was rejected */
    if ( bRejected == true ) {
        goto bailFromiSrvrLwpsHandleSearch;
    }


    /* Do the search */
    iErrorCode = iSrvrLwpsSearchIndex(psssSrvrServerSession->pssSpiSession, ppucIndexNameList, pucLanguageCode, pucSearchText, 
            pucPositiveFeedbackText, pucNegativeFeedbackText, uiStartIndex, uiEndIndex, &pssrSpiSearchResponse, &pucErrorString);


    /* Handle the search error */
    if ( iErrorCode == SPI_NoError ) {

        /* Send the search response */
        if ( (iError = iLwpsSearchResponseSend(pvLwps, pssrSpiSearchResponse, pucReferenceID)) != LWPS_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to send an lwps search response message, lwps error: %d.", iError);
            goto bailFromiSrvrLwpsHandleSearch;
        }
    }
    else {

        /* Send the error message */
        if ( (iError = iLwpsErrorMessageSend(pvLwps, iErrorCode, pucErrorString, pucReferenceID)) != LWPS_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to send an lwps error message, lwps error: %d.", iError);
            goto bailFromiSrvrLwpsHandleSearch;
        }
    }



    /* Bail label */
    bailFromiSrvrLwpsHandleSearch:


    /* Free allocated pointers */
    UTL_MACROS_FREE_NULL_TERMINATED_LIST(ppucIndexNameList);

    s_free(pucLanguageCode);
    s_free(pucSearchText);
    s_free(pucPositiveFeedbackText);
    s_free(pucNegativeFeedbackText);

    iSpiFreeSearchResponse(pssrSpiSearchResponse);
    pssrSpiSearchResponse = NULL;

    s_free(pucReferenceID);
    s_free(pucErrorString);


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrvrLwpsHandleRetrieval()

    Purpose:    Run a retrieval on a index

    Parameters: psssSrvrServerSession   server session structure
                pvLwps                  lwps structure

    Globals:    none

    Returns:    LWPS error code

*/
static int iSrvrLwpsHandleRetrieval
(
    struct srvrServerSession *psssSrvrServerSession,
    void *pvLwps
)
{

    int             iError = LWPS_NoError;
    unsigned char   *pucIndexName = NULL;
    unsigned char   *pucDocumentKey = NULL;
    unsigned char   *pucItemName = NULL;
    unsigned char   *pucMimeType = NULL;
    unsigned int    uiChunkType = 0;
    unsigned int    uiChunkStart = 0;
    unsigned int    uiChunkEnd = 0;
    unsigned char   *pucReferenceID = NULL;
    boolean         bRejected = false;
    void            *pvData = NULL;
    unsigned int    uiDataLength = 0;
    int             iErrorCode = SPI_NoError;
    unsigned char   *pucErrorString = NULL;


    ASSERT(psssSrvrServerSession != NULL);    
    ASSERT(pvLwps != NULL);    


    /* Receive the retrieval request */
    if ( (iError = iLwpsRetrievalRequestReceive(pvLwps, &pucIndexName, &pucDocumentKey, &pucItemName, &pucMimeType, 
            &uiChunkType, &uiChunkStart, &uiChunkEnd, &pucReferenceID)) != LWPS_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to receive an lwps retrieval request message, lwps error: %d.", iError);
        goto bailFromiSrvrLwpsHandleRetrieval;
    }


    /* Check the current load and reject if the retrieval load was exceeded */
    if ( (iError = iSrvrLwpsCheckLoadForRejection(pvLwps, pucReferenceID, psssSrvrServerSession->dRetrievalLoadMaximum, &bRejected)) != LWPS_NoError ) {
        goto bailFromiSrvrLwpsHandleRetrieval;
    }

    /* Bail if the retrieval request was rejected */
    if ( bRejected == true ) {
        goto bailFromiSrvrLwpsHandleRetrieval;
    }


    /* Do the retrieval */
    iErrorCode = iSrvrLwpsRetrieveDocument(psssSrvrServerSession->pssSpiSession, pucIndexName, pucDocumentKey, pucItemName, pucMimeType, 
            uiChunkType, uiChunkStart, uiChunkEnd, &pvData, &uiDataLength, &pucErrorString);


    /* Handle the retrieval error */
    if ( iErrorCode == SPI_NoError ) {

        /* Send the retrieval response */
        if ( (iError = iLwpsRetrievalResponseSend(pvLwps, pvData, uiDataLength, pucReferenceID)) != LWPS_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to send an lwps retrieval response message, lwps error: %d.", iError);
            goto bailFromiSrvrLwpsHandleRetrieval;
        }
    }
    else {

        /* Send the error message */
        if ( (iError = iLwpsErrorMessageSend(pvLwps, iErrorCode, pucErrorString, pucReferenceID)) != LWPS_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to send an lwps error message, lwps error: %d.", iError);
            goto bailFromiSrvrLwpsHandleRetrieval;
        }
    }



    /* Bail label */
    bailFromiSrvrLwpsHandleRetrieval:


    /* Free allocated pointers */
    s_free(pucIndexName);
    s_free(pucDocumentKey);
    s_free(pucItemName);
    s_free(pucMimeType);
    s_free(pvData);
    s_free(pucReferenceID);
    s_free(pucErrorString);


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrvrLwpsHandleServerInfo()

    Purpose:    Get the server information

    Parameters: psssSrvrServerSession   server session structure
                pvLwps                  lwps structure

    Globals:    none

    Returns:    LWPS error code

*/
static int iSrvrLwpsHandleServerInfo
(
    struct srvrServerSession *psssSrvrServerSession,
    void *pvLwps
)
{

    int                     iError = LWPS_NoError;
    unsigned char           *pucReferenceID = NULL;
    boolean                 bRejected = false;
    struct spiServerInfo    *pssiSpiServerInfo = NULL;
    int                     iErrorCode = SPI_NoError;
    unsigned char           *pucErrorString = NULL;


    ASSERT(psssSrvrServerSession != NULL);    
    ASSERT(pvLwps != NULL);    


    /* Receive the server info request */
    if ( (iError = iLwpsServerInfoRequestReceive(pvLwps, &pucReferenceID)) != LWPS_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to receive an lwps server information request message, lwps error: %d.", iError);
        goto bailFromiSrvrLwpsHandleServerInfo;
    }


    /* Check the current load and reject if the information load was exceeded */
    if ( (iError = iSrvrLwpsCheckLoadForRejection(pvLwps, pucReferenceID, psssSrvrServerSession->dInformationLoadMaximum, &bRejected)) != LWPS_NoError ) {
        goto bailFromiSrvrLwpsHandleServerInfo;
    }

    /* Bail if the information request was rejected */
    if ( bRejected == true ) {
        goto bailFromiSrvrLwpsHandleServerInfo;
    }


    /* Get the server info */
    iErrorCode = iSrvrLwpsGetServerInfo(psssSrvrServerSession->pssSpiSession, &pssiSpiServerInfo, &pucErrorString);


    /* Handle the server info error */
    if ( iErrorCode == SPI_NoError ) {

        /* Send the server info response */
        if ( (iError = iLwpsServerInfoResponseSend(pvLwps, pssiSpiServerInfo, pucReferenceID)) != LWPS_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to send an lwps server information response message, lwps error: %d.", iError);
            goto bailFromiSrvrLwpsHandleServerInfo;
        }
    }
    else {

        /* Send the error message */
        if ( (iError = iLwpsErrorMessageSend(pvLwps, iErrorCode, pucErrorString, pucReferenceID)) != LWPS_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to send an lwps error message, lwps error: %d.", iError);
            goto bailFromiSrvrLwpsHandleServerInfo;
        }
    }



    /* Bail label */
    bailFromiSrvrLwpsHandleServerInfo:


    /* Free allocated pointers */
    iSpiFreeServerInfo(pssiSpiServerInfo);
    pssiSpiServerInfo = NULL;

    s_free(pucReferenceID);
    s_free(pucErrorString);


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrvrLwpsHandleServerIndexInfo()

    Purpose:    Get the server index information

    Parameters: psssSrvrServerSession   server session structure
                pvLwps                  lwps structure

    Globals:    none

    Returns:    LWPS error code

*/
static int iSrvrLwpsHandleServerIndexInfo
(
    struct srvrServerSession *psssSrvrServerSession,
    void *pvLwps
)
{

    int                         iError = LWPS_NoError;
    unsigned char               *pucReferenceID = NULL;
    boolean                     bRejected = true;
    struct spiServerIndexInfo   *pssiiSpiServerIndexInfos = NULL;
    unsigned int                uiSpiServerIndexInfosLength = 0;
    int                         iErrorCode = SPI_NoError;
    unsigned char               *pucErrorString = NULL;


    ASSERT(psssSrvrServerSession != NULL);    
    ASSERT(pvLwps != NULL);    


    /* Receive the server index info request */
    if ( (iError = iLwpsServerIndexInfoRequestReceive(pvLwps, &pucReferenceID)) != LWPS_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to receive an lwps server index information request message, lwps error: %d.", iError);
        goto bailFromiSrvrLwpsHandleServerIndexInfo;
    }


    /* Check the current load and reject if the information load was exceeded */
    if ( (iError = iSrvrLwpsCheckLoadForRejection(pvLwps, pucReferenceID, psssSrvrServerSession->dInformationLoadMaximum, &bRejected)) != LWPS_NoError ) {
        goto bailFromiSrvrLwpsHandleServerIndexInfo;
    }

    /* Bail if the information request was rejected */
    if ( bRejected == true ) {
        goto bailFromiSrvrLwpsHandleServerIndexInfo;
    }


    /* Get the server index info */
    iErrorCode = iSrvrLwpsGetServerIndexInfo(psssSrvrServerSession->pssSpiSession, &pssiiSpiServerIndexInfos, &uiSpiServerIndexInfosLength, &pucErrorString);


    /* Handle the server index info error */
    if ( iErrorCode == SPI_NoError ) {

        /* Send the server index info response */
        if ( (iError = iLwpsServerIndexInfoResponseSend(pvLwps, pssiiSpiServerIndexInfos, uiSpiServerIndexInfosLength, pucReferenceID)) != LWPS_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to send an lwps server index information response message, lwps error: %d.", iError);
            goto bailFromiSrvrLwpsHandleServerIndexInfo;
        }
    }
    else {

        /* Send the error message */
        if ( (iError = iLwpsErrorMessageSend(pvLwps, iErrorCode, pucErrorString, pucReferenceID)) != LWPS_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to send an lwps error message, lwps error: %d.", iError);
            goto bailFromiSrvrLwpsHandleServerIndexInfo;
        }
    }



    /* Bail label */
    bailFromiSrvrLwpsHandleServerIndexInfo:


    /* Free allocated pointers */
    iSpiFreeServerIndexInfo(pssiiSpiServerIndexInfos, uiSpiServerIndexInfosLength);
    pssiiSpiServerIndexInfos = NULL;

    s_free(pucReferenceID);
    s_free(pucErrorString);


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrvrLwpsHandleIndexInfo()

    Purpose:    Get the index information

    Parameters: psssSrvrServerSession   server session structure
                pvLwps                  lwps structure

    Globals:    none

    Returns:    LWPS error code

*/
static int iSrvrLwpsHandleIndexInfo
(
    struct srvrServerSession *psssSrvrServerSession,
    void *pvLwps
)
{

    int                     iError = LWPS_NoError;
    unsigned char           *pucIndexName = NULL;
    unsigned char           *pucReferenceID = NULL;
    boolean                 bRejected = true;
    struct spiIndexInfo     *psiiSpiIndexInfo = NULL;
    int                     iErrorCode = SPI_NoError;
    unsigned char           *pucErrorString = NULL;


    ASSERT(psssSrvrServerSession != NULL);    
    ASSERT(pvLwps != NULL);    


    /* Receive the index info request */
    if ( (iError = iLwpsIndexInfoRequestReceive(pvLwps, &pucIndexName, &pucReferenceID)) != LWPS_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to receive an lwps index information request message, lwps error: %d.", iError);
        goto bailFromiSrvrLwpsHandleIndexInfo;
    }


    /* Check the current load and reject if the information load was exceeded */
    if ( (iError = iSrvrLwpsCheckLoadForRejection(pvLwps, pucReferenceID, psssSrvrServerSession->dInformationLoadMaximum, &bRejected)) != LWPS_NoError ) {
        goto bailFromiSrvrLwpsHandleIndexInfo;
    }

    /* Bail if the information request was rejected */
    if ( bRejected == true ) {
        goto bailFromiSrvrLwpsHandleIndexInfo;
    }


    /* Get the index info */
    iErrorCode = iSrvrLwpsGetIndexInfo(psssSrvrServerSession->pssSpiSession, pucIndexName, &psiiSpiIndexInfo, &pucErrorString);


    /* Handle the index info error */
    if ( iErrorCode == SPI_NoError ) {

        /* Send the index info response */
        if ( (iError = iLwpsIndexInfoResponseSend(pvLwps, psiiSpiIndexInfo, pucReferenceID)) != LWPS_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to send an lwps index information response message, lwps error: %d.", iError);
            goto bailFromiSrvrLwpsHandleIndexInfo;
        }
    }
    else {

        /* Send the error message */
        if ( (iError = iLwpsErrorMessageSend(pvLwps, iErrorCode, pucErrorString, pucReferenceID)) != LWPS_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to send an lwps error message, lwps error: %d.", iError);
            goto bailFromiSrvrLwpsHandleIndexInfo;
        }
    }



    /* Bail label */
    bailFromiSrvrLwpsHandleIndexInfo:


    /* Free allocated pointers */
    s_free(pucIndexName);
    
    iSpiFreeIndexInfo(psiiSpiIndexInfo);
    psiiSpiIndexInfo = NULL;

    s_free(pucReferenceID);
    s_free(pucErrorString);


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrvrLwpsHandleIndexFieldInfo()

    Purpose:    Get the index field information

    Parameters: psssSrvrServerSession   server session structure
                pvLwps                  lwps structure

    Globals:    none

    Returns:    LWPS error code

*/
static int iSrvrLwpsHandleIndexFieldInfo
(
    struct srvrServerSession *psssSrvrServerSession,
    void *pvLwps
)
{

    int                     iError = LWPS_NoError;
    unsigned char           *pucIndexName = NULL;
    unsigned char           *pucReferenceID = NULL;
    boolean                 bRejected = false;
    struct spiFieldInfo     *psfiSpiFieldInfos = NULL;
    unsigned int            uiSpiFieldInfosLength = 0;
    int                     iErrorCode = SPI_NoError;
    unsigned char           *pucErrorString = NULL;


    ASSERT(psssSrvrServerSession != NULL);    
    ASSERT(pvLwps != NULL);    


    /* Receive the index field info request */
    if ( (iError = iLwpsIndexFieldInfoRequestReceive(pvLwps, &pucIndexName, &pucReferenceID)) != LWPS_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to receive an lwps field information request message, lwps error: %d.", iError);
        goto bailFromiSrvrLwpsHandleIndexFieldInfo;
    }


    /* Check the current load and reject if the information load was exceeded */
    if ( (iError = iSrvrLwpsCheckLoadForRejection(pvLwps, pucReferenceID, psssSrvrServerSession->dInformationLoadMaximum, &bRejected)) != LWPS_NoError ) {
        goto bailFromiSrvrLwpsHandleIndexFieldInfo;
    }

    /* Bail if the information request was rejected */
    if ( bRejected == true ) {
        goto bailFromiSrvrLwpsHandleIndexFieldInfo;
    }


    /* Get the index field info */
    iErrorCode = iSrvrLwpsGetIndexFieldInfo(psssSrvrServerSession->pssSpiSession, pucIndexName, &psfiSpiFieldInfos, &uiSpiFieldInfosLength, &pucErrorString);


    /* Handle the index field info error */
    if ( iErrorCode == SPI_NoError ) {

        /* Send the index field info response */
        if ( (iError = iLwpsIndexFieldInfoResponseSend(pvLwps, psfiSpiFieldInfos, uiSpiFieldInfosLength, pucReferenceID)) != LWPS_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to send an lwps index field information response message, lwps error: %d.", iError);
            goto bailFromiSrvrLwpsHandleIndexFieldInfo;
        }
    }
    else {

        /* Send the error message */
        if ( (iError = iLwpsErrorMessageSend(pvLwps, iErrorCode, pucErrorString, pucReferenceID)) != LWPS_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to send an lwps error message, lwps error: %d.", iError);
            goto bailFromiSrvrLwpsHandleIndexFieldInfo;
        }
    }



    /* Bail label */
    bailFromiSrvrLwpsHandleIndexFieldInfo:


    /* Free allocated pointers */
    s_free(pucIndexName);

    iSpiFreeIndexFieldInfo(psfiSpiFieldInfos, uiSpiFieldInfosLength);
    psfiSpiFieldInfos = NULL;

    s_free(pucReferenceID);
    s_free(pucErrorString);


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrvrLwpsHandleIndexTermInfo()

    Purpose:    Get the index term information

    Parameters: psssSrvrServerSession   server session structure
                pvLwps                  lwps structure

    Globals:    none

    Returns:    LWPS error code

*/
static int iSrvrLwpsHandleIndexTermInfo
(
    struct srvrServerSession *psssSrvrServerSession,
    void *pvLwps
)
{

    int                     iError = LWPS_NoError;
    unsigned char           *pucIndexName = NULL;
    unsigned int            uiTermMatch = SPI_TERM_MATCH_UNKNOWN;
    unsigned int            uiTermCase = SPI_TERM_CASE_UNKNOWN;
    unsigned char           *pucTerm = NULL;
    unsigned char           *pucFieldName = NULL;
    unsigned char           *pucReferenceID = NULL;
    boolean                 bRejected = false;
    struct spiTermInfo      *pstiSpiTermInfos = NULL;
    unsigned int            uiSpiTermInfosLength = 0;
    int                     iErrorCode = SPI_NoError;
    unsigned char           *pucErrorString = NULL;


    ASSERT(psssSrvrServerSession != NULL);    
    ASSERT(pvLwps != NULL);    


    /* Receive the index term info request */
    if ( (iError = iLwpsIndexTermInfoRequestReceive(pvLwps, &pucIndexName, &uiTermMatch, &uiTermCase, &pucTerm, &pucFieldName, &pucReferenceID)) != LWPS_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to receive an lwps index term information request message, lwps error: %d.", iError);
        goto bailFromiSrvrLwpsHandleIndexTermInfo;
    }


    /* Check the current load and reject if the information load was exceeded */
    if ( (iError = iSrvrLwpsCheckLoadForRejection(pvLwps, pucReferenceID, psssSrvrServerSession->dInformationLoadMaximum, &bRejected)) != LWPS_NoError ) {
        goto bailFromiSrvrLwpsHandleIndexTermInfo;
    }

    /* Bail if the information request was rejected */
    if ( bRejected == true ) {
        goto bailFromiSrvrLwpsHandleIndexTermInfo;
    }


    /* Get the index term info */
    iErrorCode = iSrvrLwpsGetIndexTermInfo(psssSrvrServerSession->pssSpiSession, pucIndexName, uiTermMatch, uiTermCase, pucTerm, pucFieldName, 
            &pstiSpiTermInfos, &uiSpiTermInfosLength, &pucErrorString);


    /* Handle the index term info error */
    if ( iErrorCode == SPI_NoError ) {

        /* Send the index term info response */
        if ( (iError = iLwpsIndexTermInfoResponseSend(pvLwps, pstiSpiTermInfos, uiSpiTermInfosLength, pucReferenceID)) != LWPS_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to send an lwps index term information response message, lwps error: %d.", iError);
            goto bailFromiSrvrLwpsHandleIndexTermInfo;
        }
    }
    else {

        /* Send the error message */
        if ( (iError = iLwpsErrorMessageSend(pvLwps, iErrorCode, pucErrorString, pucReferenceID)) != LWPS_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to send an lwps error message, lwps error: %d.", iError);
            goto bailFromiSrvrLwpsHandleIndexTermInfo;
        }
    }



    /* Bail label */
    bailFromiSrvrLwpsHandleIndexTermInfo:


    /* Free allocated pointers */
    s_free(pucIndexName);
    s_free(pucTerm);
    s_free(pucFieldName);

    iSpiFreeTermInfo(pstiSpiTermInfos, uiSpiTermInfosLength);
    pstiSpiTermInfos = NULL;

    s_free(pucReferenceID);
    s_free(pucErrorString);


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrvrLwpsHandleDocumentInfo()

    Purpose:    Get the document information

    Parameters: psssSrvrServerSession   server session structure
                pvLwps                  lwps structure

    Globals:    none

    Returns:    LWPS error code

*/
static int iSrvrLwpsHandleDocumentInfo
(
    struct srvrServerSession *psssSrvrServerSession,
    void *pvLwps
)
{

    int                         iError = LWPS_NoError;
    unsigned char               *pucIndexName = NULL;
    unsigned char               *pucDocumentKey = NULL;
    unsigned char               *pucReferenceID = NULL;
    boolean                     bRejected = false;
    struct spiDocumentInfo      *psdiSpiDocumentInfo = NULL;
    int                         iErrorCode = SPI_NoError;
    unsigned char               *pucErrorString = NULL;


    ASSERT(psssSrvrServerSession != NULL);    
    ASSERT(pvLwps != NULL);    


    /* Receive the document info request */
    if ( (iError = iLwpsDocumentInfoRequestReceive(pvLwps, &pucIndexName, &pucDocumentKey, &pucReferenceID)) != LWPS_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to receive an lwps document information request message, lwps error: %d.", iError);
        goto bailFromiSrvrLwpsHandleDocumentInfo;
    }


    /* Check the current load and reject if the information load was exceeded */
    if ( (iError = iSrvrLwpsCheckLoadForRejection(pvLwps, pucReferenceID, psssSrvrServerSession->dInformationLoadMaximum, &bRejected)) != LWPS_NoError ) {
        goto bailFromiSrvrLwpsHandleDocumentInfo;
    }

    /* Bail if the information request was rejected */
    if ( bRejected == true ) {
        goto bailFromiSrvrLwpsHandleDocumentInfo;
    }


    /* Get the document info */
    iErrorCode = iSrvrLwpsGetDocumentInfo(psssSrvrServerSession->pssSpiSession, pucIndexName, pucDocumentKey, &psdiSpiDocumentInfo, &pucErrorString);


    /* Handle the document info error */
    if ( iErrorCode == SPI_NoError ) {

        /* Send the document info response */
        if ( (iError = iLwpsDocumentInfoResponseSend(pvLwps, psdiSpiDocumentInfo, pucReferenceID)) != LWPS_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to send an lwps document information response message, lwps error: %d.", iError);
            goto bailFromiSrvrLwpsHandleDocumentInfo;
        }
    }
    else {

        /* Send the error message */
        if ( (iError = iLwpsErrorMessageSend(pvLwps, iErrorCode, pucErrorString, pucReferenceID)) != LWPS_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to send an lwps error message, lwps error: %d.", iError);
            goto bailFromiSrvrLwpsHandleDocumentInfo;
        }
    }



    /* Bail label */
    bailFromiSrvrLwpsHandleDocumentInfo:


    /* Free allocated pointers */
    s_free(pucIndexName);
    s_free(pucDocumentKey);
    
    iSpiFreeDocumentInfo(psdiSpiDocumentInfo);
    psdiSpiDocumentInfo = NULL;

    s_free(pucReferenceID);
    s_free(pucErrorString);


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrvrLwpsCheckLoadForRejection()

    Purpose:    Checks whether the load maximum was reached and reject the
                request if so.

    Parameters: pvLwps              lwps structure
                pucReferenceID      lwps reference ID (optional)
                dLoadMaximum        load maximum
                pbRejected          return pointer indicating whether
                                    the connection was rejected

    Globals:    none

    Returns:    SPI error code

*/
static int iSrvrLwpsCheckLoadForRejection
(
    void *pvLwps,
    unsigned char *pucReferenceID,
    double dLoadMaximum,
    boolean *pbRejected
)
{

    int     iError = LWPS_NoError;

    
    ASSERT(pvLwps != NULL);
    ASSERT(pbRejected != NULL);


    /* Initially the connection is not rejected */
    *pbRejected = false;


    /* Check if the load maximum was exceeded */
    if ( dLoadMaximum > 0 ) {
        
        double      dCurrentLoad = -1;

        /* Get the current 1 minute load average and check that against the load maximum */
        if ( (iUtlLoadGetAverages(&dCurrentLoad, NULL, NULL) == UTL_NoError) && (dCurrentLoad > dLoadMaximum) ) {

            /* Send the error message */
            iError = iLwpsErrorMessageSend(pvLwps, SPI_ExceededLoadMaximum, SRVR_LWPS_LOAD_EXCEEDED_MESSAGE, pucReferenceID);
    
            iUtlLogWarn(UTL_LOG_CONTEXT, "Rejecting a client request because the server load is too high, current load: %.2f, maximum load: %.2f.", 
                            dCurrentLoad, dLoadMaximum);
            
            if ( iError != LWPS_NoError ) {
                iUtlLogError(UTL_LOG_CONTEXT, "Failed to send an lwps error message, lwps error: %d.", iError);
            }
    
            /* The connection was rejected */
            *pbRejected = true;
        }
    }
    
    
    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrvrLwpsOpenIndex()

    Purpose:    Intermediate Layer function for opening a index, the call is 
                passed onto the external function which returns an error code.
                This code is processed and an error code/error string is returned
                if needed. If the index was correctly opened, then the index
                object is returned otherwise we return a NULL.

    Parameters: pssSpiSession       spi session structure
                pucIndexName        index name
                ppvIndex            return pointer for the index structure
                ppucErrorString     return pointer for the error string 

    Globals:    none

    Returns:    SPI error code

*/
static int iSrvrLwpsOpenIndex
(
    struct spiSession *pssSpiSession,
    unsigned char *pucIndexName,
    void **ppvIndex,
    unsigned char **ppucErrorString
)
{

    int     iError = SPI_NoError;


    ASSERT(pssSpiSession != NULL);
    ASSERT(bUtlStringsIsStringNULL(pucIndexName) == false);
    ASSERT(ppvIndex != NULL);
    ASSERT(ppucErrorString != NULL);


    /* Open the index */
    if ( (iError = iSpiOpenIndex(pssSpiSession, pucIndexName, ppvIndex)) != SPI_NoError ) {

        /* Handle the error */
        iSrvrLwpsHandleSpiError(iError, pucIndexName, ppucErrorString);

        /* Close the index */
        if ( *ppvIndex != NULL ) {
            iSrvrLwpsCloseIndex(pssSpiSession, *ppvIndex);
            *ppvIndex = NULL;
        }
    }


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrvrLwpsCloseIndex()

    Purpose:    Intermediate layer function for reseting a index, the call is 
                passed onto the external function which returns an error code.

    Parameters: pssSpiSession   spi session structure
                pvIndex         index structure

    Globals:    none

    Returns:    SPI error code

*/
static int iSrvrLwpsCloseIndex
(
    struct spiSession *pssSpiSession,
    void *pvIndex
)
{

    int     iError = SPI_NoError;


    ASSERT(pssSpiSession != NULL);
    ASSERT(pvIndex != NULL);


    /* Close the index */
    iSpiCloseIndex(pssSpiSession, pvIndex);
    pvIndex = NULL;


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrvrLwpsSearchIndex()

    Purpose:    Run a search on the index and populate the results array with
                the result

    Parameters: pssSpiSession               spi session structure
                ppucIndexNameList           NULL terminated index name list
                pucLanguageCode             language code (optional)
                pucSearchText               search text (optional)
                pucPositiveFeedbackText     positive feedback text (optional)
                pucNegativeFeedbackText     negative feedback text (optional)
                uiStartIndex                start index
                uiEndIndex                  end index, 0 if there is no end index
                ppssrSpiSearchResponse      return pointer for the spi search response structure
                ppucErrorString             return pointer for an error string 

    Globals:    none

    Returns:    SPI error code

*/
static int iSrvrLwpsSearchIndex
(
    struct spiSession *pssSpiSession,
    unsigned char **ppucIndexNameList,
    unsigned char *pucLanguageCode,
    unsigned char *pucSearchText,
    unsigned char *pucPositiveFeedbackText,
    unsigned char *pucNegativeFeedbackText,
    unsigned int uiStartIndex,
    unsigned int uiEndIndex,
    struct spiSearchResponse **ppssrSpiSearchResponse,
    unsigned char **ppucErrorString
)
{

    int             iError = SPI_NoError;
    unsigned int    uiI = 0;
    void            *pvIndex = NULL;
    void            **ppvIndexList = NULL;


    ASSERT(pssSpiSession != NULL);
    ASSERT((ppucIndexNameList != NULL) || ((ppucIndexNameList != NULL) && (ppucIndexNameList[0] != NULL)));
    ASSERT(uiStartIndex >= 0);
    ASSERT(uiEndIndex >= 0);
    ASSERT(uiEndIndex >= uiStartIndex);
    ASSERT(ppssrSpiSearchResponse != NULL);
    ASSERT(ppucErrorString != NULL);


    /* Open index, only process if we got a index name list */
    if ( ppucIndexNameList != NULL ) {

        /* Loop while there are index to open */
        for ( uiI = 0; ppucIndexNameList[uiI] != NULL; uiI++ ) {

            void    **ppvIndexListPtr = NULL;

            /* Open index */
            if ( (iError = iSrvrLwpsOpenIndex(pssSpiSession, ppucIndexNameList[uiI], &pvIndex, ppucErrorString)) != SPI_NoError ) {
                /* Failed to open this index, so we bail out */
                goto bailFromiSrvrLwpsSearchIndex;
            }

            /* Add the index to the NULL index list */
            if ( (ppvIndexListPtr = (void **)s_realloc(ppvIndexList, (size_t)(sizeof(void *) * (uiI + 2)))) == NULL ) {
                iError = SPI_MemError;
                goto bailFromiSrvrLwpsSearchIndex;
            }

            /* Hand over the pointer */
            ppvIndexList = ppvIndexListPtr;

            /* Add the search index structure */
            ppvIndexList[uiI] = pvIndex;
        }

        /* Make sure we terminate the NULL index list */
        ppvIndexList[uiI] = NULL;
    }


    /* Run the search  */
    if ( (iError = iSpiSearchIndex(pssSpiSession, ppvIndexList, pucLanguageCode,pucSearchText, pucPositiveFeedbackText, pucNegativeFeedbackText, 
            uiStartIndex, uiEndIndex, ppssrSpiSearchResponse)) != SPI_NoError ) {
        
        unsigned char   pucIndexNames[SPI_INDEX_NAME_MAXIMUM_LENGTH * 3] = {'\0'};
        
        /* Create a nice, comma delimited, index name from the index name list, only process if we got a index name list */
        if ( ppucIndexNameList != NULL ) {
            
            /* Loop over the index */
            for ( uiI = 0; ppucIndexNameList[uiI] != NULL; uiI++ ) {

                /* Create a nice, comma delimited, index name from the index name list */
                if ( bUtlStringsIsStringNULL(pucIndexNames) == false ) {
                    s_strnncat(pucIndexNames, ", ", (SPI_INDEX_NAME_MAXIMUM_LENGTH * 3) - 1, SPI_INDEX_NAME_MAXIMUM_LENGTH * 3);
                }
                s_strnncat(pucIndexNames, ppucIndexNameList[uiI], (SPI_INDEX_NAME_MAXIMUM_LENGTH * 3) - 1, SPI_INDEX_NAME_MAXIMUM_LENGTH * 3);
            }
        }
        
        /* Handle the error */
        iSrvrLwpsHandleSpiError(iError, pucIndexNames, ppucErrorString);
    }



    /* Bail label */
    bailFromiSrvrLwpsSearchIndex:


    /* Close the index and free the index list */
    if ( ppvIndexList != NULL ) {
        for ( uiI = 0; ppvIndexList[uiI] != NULL; uiI++ ) {
            iSrvrLwpsCloseIndex(pssSpiSession, ppvIndexList[uiI]);
        }
        s_free(ppvIndexList);
    }


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrvrLwpsRetrieveDocument()

    Purpose:    Retrieve the document from the index. If the retrieval was
                successful, a pointer to a document will be returned. This 
                pointer will have to be freed by the calling function.

    Parameters: pssSpiSession       spi session structure
                pucIndexName        index name
                pucDocumentKey      document key
                pucItemName         document item name
                pucMimeType         document mime type
                uiChunkType         document chunk type
                uiChunkStart        document chunk start
                uiChunkEnd          document chunk end
                ppvData             return pointer for the data retrieved
                puiDataLength       return pointer for the length of the data retrieved
                ppucErrorString     return pointer for an error string 

    Globals:    none

    Returns:    SPI error code

*/
static int iSrvrLwpsRetrieveDocument
(
    struct spiSession *pssSpiSession,
    unsigned char *pucIndexName,
    unsigned char *pucDocumentKey,
    unsigned char *pucItemName,
    unsigned char *pucMimeType,
    unsigned int uiChunkType,
    unsigned int uiChunkStart,
    unsigned int uiChunkEnd,
    void **ppvData,
    unsigned int *puiDataLength,
    unsigned char **ppucErrorString
)
{

    int     iError = SPI_NoError;
    void    *pvIndex = NULL;


    ASSERT(pssSpiSession != NULL);
    ASSERT(bUtlStringsIsStringNULL(pucIndexName) == false);
    ASSERT(bUtlStringsIsStringNULL(pucDocumentKey) == false);
    ASSERT(bUtlStringsIsStringNULL(pucItemName) == false);
    ASSERT(bUtlStringsIsStringNULL(pucMimeType) == false);
    ASSERT(SPI_CHUNK_TYPE_VALID(uiChunkType) == true);
    ASSERT(uiChunkStart >= 0);
    ASSERT(uiChunkEnd >= 0);
    ASSERT(uiChunkStart >= uiChunkEnd);
    ASSERT(ppvData != NULL);
    ASSERT(puiDataLength != NULL);
    ASSERT(ppucErrorString != NULL);


    /* Open the index */
    if ( (iError = iSrvrLwpsOpenIndex(pssSpiSession, pucIndexName, &pvIndex, ppucErrorString)) != SPI_NoError) {
        return (iError);
    }


    /* Retrieve the document */
    if ( (iError = iSpiRetrieveDocument(pssSpiSession, pvIndex, pucDocumentKey, pucItemName, pucMimeType, uiChunkType, 
            uiChunkStart, uiChunkEnd, ppvData, puiDataLength)) != SPI_NoError ) {
        iSrvrLwpsHandleSpiError(iError, pucIndexName, ppucErrorString);
    }


    /* Close the index */
    iSrvrLwpsCloseIndex(pssSpiSession, pvIndex);
    pvIndex = NULL;


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrvrLwpsGetServerInfo()

    Purpose:    Get the server information

    Parameters: pssSpiSession           spi session structure
                ppssiSpiServerInfo      return pointer for the server information structure
                ppucErrorString         return pointer for an error string 

    Globals:    none

    Returns:    SPI error code

*/
static int iSrvrLwpsGetServerInfo
(
    struct spiSession *pssSpiSession,
    struct spiServerInfo **ppssiSpiServerInfo,
    unsigned char **ppucErrorString
)
{

    int     iError = SPI_NoError;


    ASSERT(pssSpiSession != NULL);
    ASSERT(ppssiSpiServerInfo != NULL);
    ASSERT(ppucErrorString != NULL);


    /* Get the server information */
    if ( (iError = iSpiGetServerInfo(pssSpiSession, ppssiSpiServerInfo)) != SPI_NoError ) {
        iSrvrLwpsHandleSpiError(iError, NULL, ppucErrorString);
    } 


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrvrLwpsGetServerIndexInfo()

    Purpose:    Get the server index information

    Parameters: pssSpiSession                   spi session structure
                ppssiiSpiServerIndexInfos       return pointer for the server index info
                puiSpiServerIndexInfosLength    return pointer for the number of entries in the server index info array
                ppucErrorString                 return pointer for an error string 

    Globals:    none

    Returns:    SPI error code

*/
static int iSrvrLwpsGetServerIndexInfo
(
    struct spiSession *pssSpiSession,
    struct spiServerIndexInfo **ppssiiSpiServerIndexInfos,
    unsigned int *puiSpiServerIndexInfosLength,
    unsigned char **ppucErrorString
)
{

    int     iError = SPI_NoError;


    ASSERT(pssSpiSession != NULL);
    ASSERT(ppssiiSpiServerIndexInfos != NULL);
    ASSERT(puiSpiServerIndexInfosLength != NULL);
    ASSERT(ppucErrorString != NULL);


    /* Get the server index information */
    if ( (iError = iSpiGetServerIndexInfo(pssSpiSession, ppssiiSpiServerIndexInfos, puiSpiServerIndexInfosLength)) != SPI_NoError ) {
        iSrvrLwpsHandleSpiError(iError, NULL, ppucErrorString);
    } 


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrvrLwpsGetIndexInfo()

    Purpose:    Get the index information

    Parameters: pssSpiSession       spi session structure
                pucIndexName        index name
                ppsiiSpiIndexInfo   return pointer for the index info
                ppucErrorString     return pointer for an error string 

    Globals:    none

    Returns:    SPI error code

*/
static int iSrvrLwpsGetIndexInfo
(
    struct spiSession *pssSpiSession,
    unsigned char *pucIndexName,
    struct spiIndexInfo **ppsiiSpiIndexInfo,
    unsigned char **ppucErrorString
)
{

    int     iError = SPI_NoError;
    void    *pvIndex = NULL;


    ASSERT(pssSpiSession != NULL);
    ASSERT(bUtlStringsIsStringNULL(pucIndexName) == false);
    ASSERT(ppsiiSpiIndexInfo != NULL);
    ASSERT(ppucErrorString != NULL);


    /* Open the index */
    if ( (iError = iSrvrLwpsOpenIndex(pssSpiSession, pucIndexName, &pvIndex, ppucErrorString)) != SPI_NoError) {
        return (iError);
    }


    /* Get the index information */
    if ( (iError = iSpiGetIndexInfo(pssSpiSession, pvIndex, ppsiiSpiIndexInfo)) != SPI_NoError ) {
        iSrvrLwpsHandleSpiError(iError, pucIndexName, ppucErrorString);
    } 


    /* Close the index */
    iSrvrLwpsCloseIndex(pssSpiSession, pvIndex);
    pvIndex = NULL;


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrvrLwpsGetIndexFieldInfo()

    Purpose:    Get the index field information

    Parameters: pssSpiSession           spi session structure
                pucIndexName            index name
                psfiSpiFieldInfos       return pointer for the field info
                uiSpiFieldInfosLength   return pointer for the field info length
                ppucErrorString         return pointer for an error string 

    Globals:    none

    Returns:    SPI error code

*/
static int iSrvrLwpsGetIndexFieldInfo
(
    struct spiSession *pssSpiSession,
    unsigned char *pucIndexName,
    struct spiFieldInfo **psfiSpiFieldInfos,
    unsigned int *uiSpiFieldInfosLength,
    unsigned char **ppucErrorString
)
{

    int     iError = SPI_NoError;
    void    *pvIndex = NULL;


    ASSERT(pssSpiSession != NULL);
    ASSERT(bUtlStringsIsStringNULL(pucIndexName) == false);
    ASSERT(psfiSpiFieldInfos != NULL);
    ASSERT(uiSpiFieldInfosLength != NULL);
    ASSERT(ppucErrorString != NULL);


    /* Open the index */
    if ( (iError = iSrvrLwpsOpenIndex(pssSpiSession, pucIndexName, &pvIndex, ppucErrorString)) != SPI_NoError) {
        return (iError);
    }


    /* Get the index field information */
    if ( (iError = iSpiGetIndexFieldInfo(pssSpiSession, pvIndex, psfiSpiFieldInfos, uiSpiFieldInfosLength)) != SPI_NoError ) {
        iSrvrLwpsHandleSpiError(iError, pucIndexName, ppucErrorString);
    } 


    /* Close the index */
    iSrvrLwpsCloseIndex(pssSpiSession, pvIndex);
    pvIndex = NULL;


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrvrLwpsGetIndexTermInfo()

    Purpose:    Get the term information

    Parameters: pssSpiSession           spi session structure
                pucIndexName            index name
                uiTermMatch             term match to search for
                uiTermCase              term case to search for
                pucTerm                 term (optional)
                pucFieldName            field name (optional)
                ppstiSpiTermInfos       return pointer for the index term info
                puiSpiTermInfosLength   return pointer for the number of entries in the index term info array
                ppucErrorString         return pointer for the error string

    Globals:    none

    Returns:    SPI error code

*/
static int iSrvrLwpsGetIndexTermInfo
(
    struct spiSession *pssSpiSession,
    unsigned char *pucIndexName,
    unsigned int uiTermMatch,
    unsigned int uiTermCase,
    unsigned char *pucTerm,
    unsigned char *pucFieldName,
    struct spiTermInfo **ppstiSpiTermInfos,
    unsigned int *puiSpiTermInfosLength,
    unsigned char **ppucErrorString
)
{

    int     iError = SPI_NoError;
    void    *pvIndex = NULL;


    ASSERT(pssSpiSession != NULL);
    ASSERT(bUtlStringsIsStringNULL(pucIndexName) == false);
    ASSERT(SPI_TERM_MATCH_VALID(uiTermMatch) == true);
    ASSERT(SPI_TERM_CASE_VALID(uiTermCase) == true);
    ASSERT(ppstiSpiTermInfos != NULL);
    ASSERT(puiSpiTermInfosLength != NULL);
    ASSERT(ppucErrorString != NULL);


    /* Open the index */
    if ( (iError = iSrvrLwpsOpenIndex(pssSpiSession, pucIndexName, &pvIndex, ppucErrorString)) != SPI_NoError) {
        return (iError);
    }


    /* Get the term information */
    if ( (iError = iSpiGetIndexTermInfo(pssSpiSession, pvIndex, uiTermMatch, uiTermCase, pucTerm, pucFieldName, ppstiSpiTermInfos, puiSpiTermInfosLength)) != SPI_NoError ) {
        iSrvrLwpsHandleSpiError(iError, pucIndexName, ppucErrorString);
    } 


    /* Close the index */
    iSrvrLwpsCloseIndex(pssSpiSession, pvIndex);
    pvIndex = NULL;


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrvrLwpsGetDocumentInfo()

    Purpose:    Get the document information

    Parameters: pssSpiSession           spi session structure
                pucIndexName            index name
                pucDocumentKey          document key
                ppsdiSpiDocumentInfo    return pointer for the document info
                ppucErrorString         return pointer for the error string

    Globals:    none

    Returns:    SPI error code

*/
static int iSrvrLwpsGetDocumentInfo
(
    struct spiSession *pssSpiSession,
    unsigned char *pucIndexName,
    unsigned char *pucDocumentKey,
    struct spiDocumentInfo **ppsdiSpiDocumentInfo,
    unsigned char **ppucErrorString
)
{

    int     iError = SPI_NoError;
    void    *pvIndex = NULL;


    ASSERT(pssSpiSession != NULL);
    ASSERT(bUtlStringsIsStringNULL(pucIndexName) == false);
    ASSERT(bUtlStringsIsStringNULL(pucDocumentKey) == false);
    ASSERT(ppsdiSpiDocumentInfo != NULL);
    ASSERT(ppucErrorString != NULL);


    /* Open the index */
    if ( (iError = iSrvrLwpsOpenIndex(pssSpiSession, pucIndexName, &pvIndex, ppucErrorString)) != SPI_NoError) {
        return (iError);
    }


    /* Get the document information */
    if ( (iError = iSpiGetDocumentInfo(pssSpiSession, pvIndex, pucDocumentKey, ppsdiSpiDocumentInfo)) != SPI_NoError ) {
        iSrvrLwpsHandleSpiError(iError, pucIndexName, ppucErrorString);
    } 


    /* Close the index */
    iSrvrLwpsCloseIndex(pssSpiSession, pvIndex);
    pvIndex = NULL;


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrvrLwpsHandleSpiError()

    Purpose:    Will log an error message corresponding to the
                error code passed and set the error string return
                pointer if that pointer is set

    Parameters: iError              error code
                pucIndexName        index name (optional)
                ppucErrorString     return pointer for an error string (optional)

    Globals:    none

    Returns:    SPI error code

*/
static int iSrvrLwpsHandleSpiError
(
    int iError,
    unsigned char *pucIndexName,
    unsigned char **ppucErrorString
)
{

    unsigned char   pucErrorString[SRVR_LWPS_LONG_STRING_LENGTH + 1] = {'\0'};


    /* Check the error code */
    if ( iError == SPI_NoError ) {
        return (SPI_NoError);
    }


    /* Get the error text, default to an unknown error */
    if ( iSpiGetErrorText(iError, pucErrorString, SRVR_LWPS_LONG_STRING_LENGTH + 1) != SPI_NoError ) {
        s_strnncpy(pucErrorString, "Unknown error", SRVR_LWPS_LONG_STRING_LENGTH + 1);
    }

    /* Append the index name to the error string */
    if ( bUtlStringsIsStringNULL(pucIndexName) == false ) {
        s_strnncat(pucErrorString, ", index: '", SRVR_LWPS_LONG_STRING_LENGTH + 1, SRVR_LWPS_LONG_STRING_LENGTH + 1);
        s_strnncat(pucErrorString, pucIndexName, SRVR_LWPS_LONG_STRING_LENGTH + 1, SRVR_LWPS_LONG_STRING_LENGTH + 1);
        s_strnncat(pucErrorString, "'", SRVR_LWPS_LONG_STRING_LENGTH + 1, SRVR_LWPS_LONG_STRING_LENGTH + 1);
    }


#if defined(SRVR_LWPS_ENABLE_SPI_ERROR_LOGGING)
    /* Log the error */
    iUtlLogError(UTL_LOG_CONTEXT, "%s.", pucErrorString);
#endif /* defined(SRVR_LWPS_ENABLE_SPI_ERROR_LOGGING) */

    
    /* Allocate and set the return pointer if it is set */
    if ( ppucErrorString != NULL ) {
        if ( (*ppucErrorString = (unsigned char *)s_strdup(pucErrorString)) == NULL ) {
            return (SPI_MemError);
        }
    }


    return (SPI_NoError);

}


/*---------------------------------------------------------------------------*/
