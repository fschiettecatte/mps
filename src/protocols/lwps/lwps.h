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

    Module:     lwps.h

    Author:     Francois Schiettecatte (FS Consulting LLC.)

    Created:    20 December 1995

    Purpose:    This module implements a lightweight protocol for client/
                server searching.

*/


/*---------------------------------------------------------------------------*/


#if !defined(LWPS_H)
#define LWPS_H


/*---------------------------------------------------------------------------*/


/*
** Includes
*/

#include "utils.h"
#include "spi.h"


/*---------------------------------------------------------------------------*/


/* C++ wrapper */
#if defined(__cplusplus)
extern "C" {
#endif    /* defined(__cplusplus) */


/*---------------------------------------------------------------------------*/


/*
** Defines
*/

/* Request/response tags */
#define LWPS_INVALID_ID                         (0)

#define LWPS_ERROR_MESSAGE_ID                   (1)

#define LWPS_INIT_REQUEST_ID                    (10)
#define LWPS_INIT_RESPONSE_ID                   (11)
#define LWPS_SEARCH_REQUEST_ID                  (12)
#define LWPS_SEARCH_RESPONSE_TAG                (13)
#define LWPS_RETRIEVAL_REQUEST_ID               (14)
#define LWPS_RETRIEVAL_RESPONSE_ID              (15)
#define LWPS_SERVER_INFO_REQUEST_ID             (16)
#define LWPS_SERVER_INFO_RESPONSE_ID            (17)
#define LWPS_SERVER_INDEX_INFO_REQUEST_ID       (18)
#define LWPS_SERVER_INDEX_INFO_RESPONSE_ID      (19)
#define LWPS_INDEX_INFO_REQUEST_ID              (20)
#define LWPS_INDEX_INFO_RESPONSE_ID             (21)
#define LWPS_INDEX_FIELD_INFO_REQUEST_ID        (22)
#define LWPS_INDEX_FIELD_INFO_RESPONSE_ID       (23)
#define LWPS_INDEX_TERM_INFO_REQUEST_ID         (24)
#define LWPS_INDEX_TERM_INFO_RESPONSE_ID        (25)
#define LWPS_DOCUMENT_INFO_REQUEST_ID           (26)
#define LWPS_DOCUMENT_INFO_RESPONSE_ID          (27)

#define LWPS_MESSAGE_ID_VALID(n)                (((n) >= LWPS_ERROR_MESSAGE_ID) && \
                                                        ((n) <= LWPS_DOCUMENT_INFO_RESPONSE_ID))


/* Protocol URL */
#define LWPS_PROTOCOL_NAME                      (unsigned char *)"lwps"

/* Protocol URL */
#define LWPS_PROTOCOL_URL                       (unsigned char *)"lwps://"

/* Default protocol host */
#define LWPS_PROTOCOL_HOST_DEFAULT              UTL_SOCKET_LOCALHOST_NAME

/* Default protocol port */
#define LWPS_PROTOCOL_PORT_DEFAULT              (9000)

/* Default protocol timeout in milliseconds */
#define LWPS_PROTOCOL_TIMEOUT_DEFAULT           (60000)


/*---------------------------------------------------------------------------*/


/* Error codes */
#define LWPS_NoError                            (0)
#define LWPS_MemError                           (-1)
#define LWPS_ParameterError                     (-2)
#define LWPS_ReturnParameterError               (-3)
#define LWPS_MiscError                          (-4)


#define LWPS_InvalidNet                         (-10)
#define LWPS_InvalidLwps                        (-11)

#define LWPS_FailedReadData                     (-20)
#define LWPS_FailedWriteData                    (-21)
#define LWPS_TimeOut                            (-22)
#define LWPS_SocketClosed                       (-23)

#define LWPS_InvalidProtocolHeader              (-30)
#define LWPS_InvalidProtocolVersion             (-31)
#define LWPS_InvalidMessageID                   (-32)
    
#define LWPS_InvalidReferenceID                 (-40)


/*---------------------------------------------------------------------------*/


/*
** Public function prototypes
*/

/* Public functions to set up the protocol handle and tear it down */
int iLwpsCreate (void *pvUtlNet, void **ppvLwps);

int iLwpsFree (void *pvLwps);


/* Public function to decode the header to get the message tag */
int iLwpsHeaderPeek (void *pvLwps, unsigned int *puiMessageID);



/* Public functions to handle errors */
int iLwpsErrorMessageSend (void *pvLwps, int iErrorCode, unsigned char *pucErrorString, 
        unsigned char *pucReferenceID);

int iLwpsErrorMessageReceive (void *pvLwps, int *piErrorCode, 
        unsigned char **ppucErrorString, unsigned char **ppucReferenceID);



/* Public functions to handle init */
int iLwpsInitRequestSend (void *pvLwps, unsigned char *pucUserName, 
        unsigned char *pucPassword, unsigned char *pucReferenceID);

int iLwpsInitRequestReceive (void *pvLwps, unsigned char **ppucUserName, 
        unsigned char **ppucPassWord, unsigned char **ppucReferenceID);

int iLwpsInitRequestHandle (void *pvLwps, unsigned char *pucUserName, 
        unsigned char *pucPassword, int *piErrorCode, unsigned char **ppucErrorString);

int iLwpsInitResponseSend (void *pvLwps, unsigned char *pucReferenceID);

int iLwpsInitResponseReceive (void *pvLwps, unsigned char **ppucReferenceID);

int iLwpsInitResponseHandle (void *pvLwps);



/* Public functions to handle search */
int iLwpsSearchRequestSend (void *pvLwps, unsigned char **ppucIndexNameList, 
        unsigned char *pucLanguageCode,  unsigned char *pucSearchText, 
        unsigned char *pucPositiveFeedbackText, unsigned char *pucNegativeFeedbackText, 
        unsigned int uiStartIndex, unsigned int uiEndIndex, 
        unsigned char *pucReferenceID);

int iLwpsSearchRequestReceive (void *pvLwps, unsigned char ***pppucIndexNameList, 
        unsigned char **ppucLanguageCode, unsigned char **ppucSearchText, 
        unsigned char **ppucPositiveFeedbackText, unsigned char **ppucNegativeFeedbackText, 
        unsigned int *puiStartIndex, unsigned int *puiEndIndex, 
        unsigned char **ppucReferenceID);

int iLwpsSearchRequestHandle (void *pvLwps, unsigned char **ppucIndexNameList, 
        unsigned char *pucLanguageCode, unsigned char *pucSearchText, 
        unsigned char *pucPositiveFeedbackText, unsigned char *pucNegativeFeedbackText, 
        unsigned int uiStartIndex, unsigned int uiEndIndex, 
        struct spiSearchResponse **ppssrSpiSearchResponse, int *piErrorCode, 
        unsigned char **ppucErrorString);

int iLwpsSearchResponseSend (void *pvLwps, struct spiSearchResponse *pssrSpiSearchResponse, 
        unsigned char *pucReferenceID);

int iLwpsSearchResponseReceive (void *pvLwps, struct spiSearchResponse **ppssrSpiSearchResponse, 
        unsigned char **ppucReferenceID);

int iLwpsSearchResponseHandle (void *pvLwps);



/* Public functions to handle retrieval */
int iLwpsRetrievalRequestSend (void *pvLwps, unsigned char *pucIndexName,
        unsigned char *pucDocumentKey, unsigned char *pucItemName, unsigned char *pucMimeType, 
        unsigned int uiChunkType, unsigned int uiChunkStart, unsigned int uiChunkEnd,
        unsigned char *pucReferenceID);

int iLwpsRetrievalRequestReceive (void *pvLwps, unsigned char **ppucIndexName, 
        unsigned char **ppucDocumentKey, unsigned char **ppucItemName, unsigned char **ppucMimeType, 
        unsigned int *puiChunkType, unsigned int *puiChunkStart, 
        unsigned int *puiChunkEnd, unsigned char **ppucReferenceID);

int iLwpsRetrievalRequestHandle (void *pvLwps, unsigned char *pucIndexName,
        unsigned char *pucDocumentKey, unsigned char *pucItemName, unsigned char *pucMimeType, 
        unsigned int uiChunkType, unsigned int uiChunkStart, unsigned int uiChunkEnd,
        void **ppvData, unsigned int *puiDataLength, int *piErrorCode, 
        unsigned char **ppucErrorString);

int iLwpsRetrievalResponseSend (void *pvLwps, void *pvData, unsigned int uiDataLength, 
        unsigned char *pucReferenceID);

int iLwpsRetrievalResponseReceive (void *pvLwps, void **ppvData, unsigned int *puiDataLength,
        unsigned char **ppucReferenceID);

int iLwpsRetrievalResponseHandle (void *pvLwps);



/* Public functions to handle server info */
int iLwpsServerInfoRequestSend (void *pvLwps, unsigned char *pucReferenceID);

int iLwpsServerInfoRequestReceive (void *pvLwps, unsigned char **ppucReferenceID);

int iLwpsServerInfoRequestHandle (void *pvLwps, struct spiServerInfo **ppssiSpiServerInfo, 
        int *piErrorCode, unsigned char **ppucErrorString);

int iLwpsServerInfoResponseSend (void *pvLwps, struct spiServerInfo *pssiSpiServerInfo,
        unsigned char *pucReferenceID);

int iLwpsServerInfoResponseReceive (void *pvLwps, struct spiServerInfo **ppssiSpiServerInfo, 
        unsigned char **ppucReferenceID);

int iLwpsServerInfoResponseHandle (void *pvLwps);



/* Public functions to handle server index info */
int iLwpsServerIndexInfoRequestSend (void *pvLwps, unsigned char *pucReferenceID);

int iLwpsServerIndexInfoRequestReceive (void *pvLwps, unsigned char **ppucReferenceID);

int iLwpsServerIndexInfoRequestHandle (void *pvLwps, struct spiServerIndexInfo **ppssiiSpiServerIndexInfos, 
        unsigned int *puiSpiServerIndexInfosLength, int *piErrorCode, unsigned char **ppucErrorString);

int iLwpsServerIndexInfoResponseSend (void *pvLwps, struct spiServerIndexInfo *pssiiSpiServerIndexInfos, 
        unsigned int uiSpiServerIndexInfosLength, unsigned char *pucReferenceID);

int iLwpsServerIndexInfoResponseReceive (void *pvLwps, struct spiServerIndexInfo **ppssiiSpiServerIndexInfos, 
        unsigned int *puiSpiServerIndexInfosLength, unsigned char **ppucReferenceID);

int iLwpsServerIndexInfoResponseHandle (void *pvLwps);



/* Public functions to handle index info */
int iLwpsIndexInfoRequestSend (void *pvLwps, unsigned char *pucIndexName, unsigned char *pucReferenceID);

int iLwpsIndexInfoRequestReceive (void *pvLwps, unsigned char **ppucIndexName, unsigned char **ppucReferenceID);

int iLwpsIndexInfoRequestHandle (void *pvLwps, unsigned char *pucIndexName,
        struct spiIndexInfo **ppsiiSpiIndexInfo, int *piErrorCode, unsigned char **ppucErrorString);

int iLwpsIndexInfoResponseSend (void *pvLwps, struct spiIndexInfo *psiiSpiIndexInfo,
        unsigned char *pucReferenceID);

int iLwpsIndexInfoResponseReceive (void *pvLwps, struct spiIndexInfo **ppsiiSpiIndexInfo, 
        unsigned char **ppucReferenceID);

int iLwpsIndexInfoResponseHandle (void *pvLwps);



/* Public functions to handle index field info */
int iLwpsIndexFieldInfoRequestSend (void *pvLwps, unsigned char *pucIndexName, unsigned char *pucReferenceID);

int iLwpsIndexFieldInfoRequestReceive (void *pvLwps, unsigned char **ppucIndexName,
        unsigned char **ppucReferenceID);

int iLwpsIndexFieldInfoRequestHandle (void *pvLwps, unsigned char *pucIndexName,
        struct spiFieldInfo **ppsfiSpiFieldInfos, unsigned int *puiSpiFieldInfosLength, 
        int *piErrorCode, unsigned char **ppucErrorString);

int iLwpsIndexFieldInfoResponseSend (void *pvLwps, struct spiFieldInfo *psfiSpiFieldInfos, 
        unsigned int uiSpiFieldInfosLength, unsigned char *pucReferenceID);

int iLwpsIndexFieldInfoResponseReceive (void *pvLwps, struct spiFieldInfo **ppsfiSpiFieldInfos, 
        unsigned int *puiSpiFieldInfosLength, unsigned char **ppucReferenceID);

int iLwpsIndexFieldInfoResponseHandle (void *pvLwps);



/* Public functions to handle index term info */
int iLwpsIndexTermInfoRequestSend (void *pvLwps, unsigned char *pucIndexName,
        unsigned int uiTermMatch, unsigned int uiTermCase, unsigned char *pucTerm, 
        unsigned char *pucFieldName, unsigned char *pucReferenceID);

int iLwpsIndexTermInfoRequestReceive (void *pvLwps, unsigned char **ppucIndexName, 
        unsigned int *puiTermType, unsigned int *puiTermCase, unsigned char **ppucTerm, 
        unsigned char **ppucFieldName, unsigned char **ppucReferenceID);

int iLwpsIndexTermInfoRequestHandle (void *pvLwps, unsigned char *pucIndexName,
        unsigned int uiTermMatch, unsigned int uiTermCase, unsigned char *pucTerm, 
        unsigned char *pucFieldName, struct spiTermInfo **ppstiSpiTermInfos, 
        unsigned int *puiSpiTermInfosLength, int *piErrorCode, unsigned char **ppucErrorString);


int iLwpsIndexTermInfoResponseSend (void *pvLwps, struct spiTermInfo *pstiSpiTermInfos,
        unsigned int uiSpiTermInfosLength, unsigned char *pucReferenceID);

int iLwpsIndexTermInfoResponseReceive (void *pvLwps, struct spiTermInfo **ppstiSpiTermInfos,
        unsigned int *puiSpiTermInfosLength, unsigned char **ppucReferenceID);

int iLwpsIndexTermInfoResponseHandle (void *pvLwps);



/* Public functions to handle document info */
int iLwpsDocumentInfoRequestSend (void *pvLwps, unsigned char *pucIndexName,
        unsigned char *pucDocumentKey, unsigned char *pucReferenceID);

int iLwpsDocumentInfoRequestReceive (void *pvLwps, unsigned char **ppucIndexName, 
        unsigned char **ppucDocumentKey, unsigned char **ppucReferenceID);

int iLwpsDocumentInfoRequestHandle (void *pvLwps, unsigned char *pucIndexName,
        unsigned char *pucDocumentKey, struct spiDocumentInfo **ppsdiSpiDocumentInfo,
        int *piErrorCode, unsigned char **ppucErrorString);


int iLwpsDocumentInfoResponseSend (void *pvLwps, struct spiDocumentInfo *psdiSpiDocumentInfo,
        unsigned char *pucReferenceID);

int iLwpsDocumentInfoResponseReceive (void *pvLwps, struct spiDocumentInfo **ppsdiSpiDocumentInfo,
        unsigned char **ppucReferenceID);

int iLwpsDocumentInfoResponseHandle (void *pvLwps);


/*---------------------------------------------------------------------------*/


/* C++ wrapper */
#if defined(__cplusplus)
}
#endif    /* defined(__cplusplus) */


/*---------------------------------------------------------------------------*/


#endif    /* !defined(LWPS_H) */


/*---------------------------------------------------------------------------*/

