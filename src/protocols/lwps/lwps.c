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

    Module:     lwps.c

    Author:     Francois Schiettecatte (FS Consulting LLC.)

    Created:    20 December 1995

    Purpose:    This module implements a lightweight protocol for parallel
                server searching.

*/


/*---------------------------------------------------------------------------*/


/*
** Includes
*/

#include "utils.h"

#include "lwps.h"
#include "spi.h"


/*---------------------------------------------------------------------------*/


/*
** Feature defines
*/

/* Context for logging */
#undef UTL_LOG_CONTEXT
#define UTL_LOG_CONTEXT                     (unsigned char *)"com.fsconsult.mps.src.protocols.lwps"


/* Enable this to clear the send buffer on a send error - not advisable */
/* #defined LWPS_ENABLE_CLEAR_SEND_BUFFER_ON_SEND_ERROR */


/*---------------------------------------------------------------------------*/


/*
** Defines
*/


/* Protocol header is built up as follows:
**
**  - protocol tag byte             'L'
**  - version number byte           '#'
**  - request/response ID byte      '#'
*/

/* Protocol tag */
#define LWPS_PROTOCOL_TAG                       (unsigned char)'L'        /* First byte in any request/response */

/* Protocol tag size */
#define LWPS_PROTOCOL_TAG_SIZE                  (1)

/* Version size */
#define LWPS_VERSION_SIZE                       (1)

/* Message ID size */
#define LWPS_MESSAGE_ID_SIZE                    (1)

/* Header length */
#define LWPS_HEADER_LENGTH                      (LWPS_PROTOCOL_TAG_SIZE + \
                                                        LWPS_VERSION_SIZE + \
                                                        LWPS_MESSAGE_ID_SIZE)                    


/* Protocol version and init message string, 1.7 */
#define LWPS_PROTOCOL_VERSION_1_7_ID            (17)
#define LWPS_PROTOCOL_INIT_MESSAGE_1_7_NAME     (unsigned char *)"LWPS Client 1.7"

/* Protocol version and init message string, 1.8 */
#define LWPS_PROTOCOL_VERSION_1_8_ID            (18)
#define LWPS_PROTOCOL_INIT_MESSAGE_1_8_NAME     (unsigned char *)"LWPS Client 1.8"

/* Default protocol version, 1.7 */
#define LWPS_PROTOCOL_VERSION_DEFAULT_ID        (LWPS_PROTOCOL_VERSION_1_7_ID)
#define LWPS_PROTOCOL_VERSION_DEFAULT_NAME      (LWPS_PROTOCOL_INIT_MESSAGE_1_7_NAME)


/* Reference ID buffer length */
#define LWPS_REFERENCE_ID_BUFFER_LENGTH         (256)


/*---------------------------------------------------------------------------*/


/*
** Structures
*/

/* lwps structure structure */
struct lwps {
    void            *pvUtlNet;                  /* Utl Net structure */
    unsigned int    uiProtocolVersion;          /* Protocol version */
    unsigned int    uiReferenceID;              /* Reference ID */
};


/*---------------------------------------------------------------------------*/


/*
** Private function prototypes
*/

static int iLwpsHeaderSend (struct lwps *plLwps, unsigned int uiMessageID);
static int iLwpsHeaderReceive (struct lwps *plLwps, unsigned int *puiMessageID);


static int iLwpsStringWrite (struct lwps *plLwps, unsigned char *pucString);
static int iLwpsStringRead (struct lwps *plLwps, unsigned char **ppucString);

static int iLwpsIntegerWrite (struct lwps *plLwps, unsigned int uiInteger);
static int iLwpsIntegerRead (struct lwps *plLwps, unsigned int *puiInteger);

static int iLwpsLongWrite (struct lwps *plLwps, unsigned long ulLong);
static int iLwpsLongRead (struct lwps *plLwps, unsigned long *pulLong);

static int iLwpsFloatWrite (struct lwps *plLwps, float fFloat);
static int iLwpsFloatRead (struct lwps *plLwps, float *pfFloat);

static int iLwpsDoubleWrite (struct lwps *plLwps, double dDouble);
static int iLwpsDoubleRead (struct lwps *plLwps, double *pdDouble);

static int iLwpsDataWrite (struct lwps *plLwps, unsigned char *pucData, 
        unsigned int uiDataLength);
static int iLwpsDataRead (struct lwps *plLwps, unsigned char **ppucData, 
        unsigned int *puiDataLength);


static int iLwpsMapUtlNetError (int iNetError, int iDefaultLwpsError);


/*---------------------------------------------------------------------------*/


/*

    Function:   iLwpsCreate()

    Purpose:    Create a new lwps structure. 

                This function will set up the new lwps structure and return
                a pointer to that structure. This pointer needs to be passed
                back as part of every lwps function. This allows one to access
                lots of lwps sessions at once.
                
                Note that we set the socket state here too, and that the calling
                application should not reset them.

    Parameters: pvUtlNet    Net structure
                ppvLwps     return pointer for the lwps  structure

    Globals:    none

    Returns:    LWPS error code

*/
int iLwpsCreate
(
    void *pvUtlNet,
    void **ppvLwps
)
{

    struct lwps     *plLwps = NULL;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iLwpsCreate - uiTimeOut: [%u]", uiTimeOut); */


    /* Check the parameters */
    if ( pvUtlNet == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvUtlNet' parameter passed to 'iLwpsCreate'."); 
        return (LWPS_InvalidNet);
    }

    if ( ppvLwps == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppvLwps' parameter passed to 'iLwpsCreate'."); 
        return (LWPS_ReturnParameterError);
    }


    /* Allocate the lwps structure handle */
    if ( (plLwps = (struct lwps *)s_malloc((size_t)(sizeof(struct lwps)))) == NULL ) {
        iLwpsFree(plLwps);
        plLwps = NULL;
        return (LWPS_MemError);
    }


    /* Initialize the lwps structure handle */
    plLwps->pvUtlNet = pvUtlNet;
    plLwps->uiProtocolVersion = LWPS_PROTOCOL_VERSION_DEFAULT_ID;
    plLwps->uiReferenceID = 0;


    /* Set the return pointer */
    *ppvLwps = (void *)plLwps;


    return (LWPS_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLwpsFree()

    Purpose:    Free the lwps structure.

                This function will release all the memory resources currently
                held by the lwps. After this the lwps structure will be invalid.

    Parameters: pvLwps  lwps structure

    Globals:    none

    Returns:    LWPS error code

*/
int iLwpsFree
(
    void *pvLwps
)
{

    struct lwps     *plLwps = (struct lwps *)pvLwps;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iLwpsFree"); */


    /* Check the parameters */
    if ( pvLwps == NULL ) {
        iUtlLogDebug(UTL_LOG_CONTEXT, "Null 'pvLwps' parameter passed to 'iLwpsFree'."); 
        return (LWPS_InvalidLwps);
    }


    /* Free the lwps structure handle */
    s_free(plLwps);


    return (LWPS_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLwpsHeaderPeek()

    Purpose:    Peek into the LWPS request/response header, without actually
                reading it.

    Parameters: pvLwps          lwps structure
                puiMessageID    return pointer for the message ID

    Globals:    none

    Returns:    LWPS error code

*/
int iLwpsHeaderPeek
(
    void *pvLwps,
    unsigned int *puiMessageID
)
{

    int             iError = LWPS_NoError;
    struct lwps     *plLwps = (struct lwps *)pvLwps;
    unsigned char   *pucPeekPtr = NULL;
    unsigned char   *pucPtr = NULL;
    unsigned int    uiProtocolVersion = LWPS_PROTOCOL_VERSION_DEFAULT_ID;
    unsigned int    uiMessageID = LWPS_INVALID_ID;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iLwpsHeaderPeek");  */


    /* Check the parameters */
    if ( pvLwps == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvLwps' parameter passed to 'iLwpsHeaderPeek'."); 
        return (LWPS_InvalidLwps);
    }

    if ( puiMessageID == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "puiMessageID 'pvLwps' parameter passed to 'iLwpsHeaderPeek'."); 
        return (LWPS_ReturnParameterError);
    }

    
    /* Peek the header */
    if ( (iError = iUtlNetPeek(plLwps->pvUtlNet, LWPS_HEADER_LENGTH, &pucPeekPtr)) != UTL_NoError ) {
        return(iLwpsMapUtlNetError(iError, LWPS_FailedReadData));
    }
    

    /* Read and check the protocol header tag - first byte */
    if ( pucPeekPtr[0] != LWPS_PROTOCOL_TAG ) {
        return (LWPS_InvalidProtocolHeader);
    }


    /* Read the procotol version - second byte */
    pucPtr = pucPeekPtr + 1;
    UTL_NUM_READ_UINT(uiProtocolVersion, LWPS_VERSION_SIZE, pucPtr);
    ASSERT((pucPeekPtr + 1 + LWPS_VERSION_SIZE) == pucPtr);

    /* Check that we support this version */
    if ( (uiProtocolVersion != LWPS_PROTOCOL_VERSION_1_7_ID) && (uiProtocolVersion != LWPS_PROTOCOL_VERSION_1_8_ID) ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid protocol version: %d, expected: %d or %d.", uiProtocolVersion, 
                LWPS_PROTOCOL_VERSION_1_7_ID, LWPS_PROTOCOL_VERSION_1_8_ID);
        return (LWPS_InvalidProtocolVersion);
    }


    /* Read the message ID - third byte */
    pucPtr = pucPeekPtr + 2;
    UTL_NUM_READ_UINT(uiMessageID, LWPS_MESSAGE_ID_SIZE, pucPtr);
    ASSERT((pucPeekPtr + 2 + LWPS_MESSAGE_ID_SIZE) == pucPtr);


    ASSERT((pucPtr - pucPeekPtr) == LWPS_HEADER_LENGTH);


    /* Set the return pointer */
    *puiMessageID = uiMessageID;


    return (LWPS_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLwpsErrorMessageSend()

    Purpose:    Send an error message

    Parameters: pvLwps              lwps structure
                iErrorCode          error code to send
                pucErrorString      error message to send (optional)
                pucReferenceID      reference ID (optional)

    Globals:    none

    Returns:    LWPS error code

*/
int iLwpsErrorMessageSend
(
    void *pvLwps,
    int iErrorCode,
    unsigned char *pucErrorString,
    unsigned char *pucReferenceID
)
{

    int             iError = LWPS_NoError;
    struct lwps     *plLwps = (struct lwps *)pvLwps;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iLwpsErrorMessageSend - iErrorCode: [%d], pucErrorString: [%s], pucReferenceID: [%s]",  */
/*             iErrorCode, pucErrorString, pucReferenceID);  */


    /* Check the parameters */
    if ( pvLwps == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvLwps' parameter passed to 'iLwpsErrorMessageSend'."); 
        return (LWPS_InvalidLwps);
    }


    /* Send the reply header */
    if ( (iError = iLwpsHeaderSend(plLwps, LWPS_ERROR_MESSAGE_ID)) != LWPS_NoError ) {
        goto bailFromiLwpsErrorMessageSend;
    }


    /* Send the error code - convert the error code to a positive number */
    if ( (iError = iLwpsIntegerWrite(plLwps, abs(iErrorCode))) != LWPS_NoError ) {
        goto bailFromiLwpsErrorMessageSend;
    }


    /* Send the error message */
    if ( (iError = iLwpsStringWrite(plLwps, pucErrorString)) != LWPS_NoError ) {
        goto bailFromiLwpsErrorMessageSend;
    }
    

    /* Send the reference ID string */
    if ( (iError = iLwpsStringWrite(plLwps, pucReferenceID)) != LWPS_NoError ) {
        goto bailFromiLwpsErrorMessageSend;
    }



    /* Bail label */
    bailFromiLwpsErrorMessageSend:


    /* Handle the error */
    if ( iError == LWPS_NoError ) {
    
        /* Send the data */
        if ( (iError = iUtlNetSend(plLwps->pvUtlNet)) != UTL_NoError ) {
            iError = iLwpsMapUtlNetError(iError, LWPS_FailedWriteData);
        }
    }
#if defined(LWPS_ENABLE_CLEAR_SEND_BUFFER_ON_SEND_ERROR)
    else {

        /* Reset the send buffer to clear it */
        if ( (iError = iUtlNetResetSend(plLwps->pvUtlNet)) != UTL_NoError ) {
            iError = iLwpsMapUtlNetError(iError, LWPS_FailedWriteData);
        }
    }
#endif    /* defined(LWPS_ENABLE_CLEAR_SEND_BUFFER_ON_SEND_ERROR) */


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLwpsErrorMessageReceive()

    Purpose:    Receive an error message

    Parameters: pvLwps              lwps structure
                piErrorCode         return pointer for the error code (allocated)
                ppucErrorString     return pointer for the error string (allocated)
                ppucReferenceID     return pointer for the reference ID (allocated)

    Globals:    none

    Returns:    LWPS error code

*/
int iLwpsErrorMessageReceive
(
    void *pvLwps,
    int *piErrorCode,
    unsigned char **ppucErrorString,
    unsigned char **ppucReferenceID
)
{

    int             iError = LWPS_NoError;
    struct lwps     *plLwps = (struct lwps *)pvLwps;
    unsigned int    uiMessageID = LWPS_INVALID_ID;
    int             iErrorCode = SPI_NoError;
    unsigned char   *pucErrorString = NULL;
    unsigned char   *pucReferenceID = NULL;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iLwpsErrorMessageReceive");  */


    /* Check the parameters */
    if ( pvLwps == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvLwps' parameter passed to 'iLwpsErrorMessageReceive'."); 
        return (LWPS_InvalidLwps);
    }

    if ( piErrorCode == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'piErrorCode' parameter passed to 'iLwpsErrorMessageReceive'."); 
        return (LWPS_ReturnParameterError);
    }

    if ( ppucErrorString == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppucErrorString' parameter passed to 'iLwpsErrorMessageReceive'."); 
        return (LWPS_ReturnParameterError);
    }

    if ( ppucReferenceID == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppucReferenceID' parameter passed to 'iLwpsErrorMessageReceive'."); 
        return (LWPS_ReturnParameterError);
    }


    /* Peek the response header, we do this here because we don't want 
    ** to harm the response if it is not what we expected
    */
    if ( (iError = iLwpsHeaderPeek(plLwps, &uiMessageID)) != LWPS_NoError ) {
        goto bailFromiLwpsErrorMessageReceive;
    }


    /* Check that the response header message tag is correct */
    if ( uiMessageID != LWPS_ERROR_MESSAGE_ID) {
        iError = LWPS_InvalidMessageID;
        goto bailFromiLwpsErrorMessageReceive;
    }


    /* Read the response header */
    if ( (iError = iLwpsHeaderReceive(plLwps, &uiMessageID)) != LWPS_NoError ) {
        goto bailFromiLwpsErrorMessageReceive;
    }


    /* Read the error code */
    if ( (iError = iLwpsIntegerRead(plLwps, &iErrorCode)) != LWPS_NoError ) {
        return (iError);
    }

    /* Remember that the protocol cannot send negative numbers
    ** and that errors are always negative 
    */
    iErrorCode = -iErrorCode;


    /* Read the error string */
    if ( (iError = iLwpsStringRead(plLwps, &pucErrorString)) != LWPS_NoError ) {
        return (iError);
    }


    /* Read the reference ID */
    if ( (iError = iLwpsStringRead(plLwps, &pucReferenceID)) != LWPS_NoError ) {
        goto bailFromiLwpsErrorMessageReceive;
    }



    /* Bail label */
    bailFromiLwpsErrorMessageReceive:


    /* Handle the error */
    if ( iError == LWPS_NoError ) {

        /* Set the return pointers */
        *piErrorCode = iErrorCode;
        *ppucErrorString = pucErrorString;
        *ppucReferenceID = pucReferenceID;
    }    
    else {

        /* Free allocations */
        s_free(pucErrorString);
        s_free(pucReferenceID);
    }


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLwpsInitRequestSend()

    Purpose:    Send an init request to a server

    Parameters: pvLwps              lwps structure
                pucUserName         user name (optional)
                pucPassword         password (optional)
                pucReferenceID      reference ID (optional)

    Globals:    none

    Returns:    LWPS error code

*/
int iLwpsInitRequestSend
(
    void *pvLwps,
    unsigned char *pucUserName,
    unsigned char *pucPassword,
    unsigned char *pucReferenceID
)
{

    int             iError = LWPS_NoError;
    struct lwps     *plLwps = (struct lwps *)pvLwps;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iLwpsInitRequestSend");  */


    /* Check the parameters */
    if ( pvLwps == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvLwps' parameter passed to 'iLwpsInitRequestSend'."); 
        return (LWPS_InvalidLwps);
    }


    /* Send the message header */
    if ( (iError = iLwpsHeaderSend(plLwps, LWPS_INIT_REQUEST_ID)) != LWPS_NoError ) {
        goto bailFromiLwpsInitRequestSend;
    }


    /* Send the user name */
    if ( (iError = iLwpsStringWrite(plLwps, pucUserName)) != LWPS_NoError ) {
        goto bailFromiLwpsInitRequestSend;
    }


    /* Send the password */
    if ( (iError = iLwpsStringWrite(plLwps, pucPassword)) != LWPS_NoError ) {
        goto bailFromiLwpsInitRequestSend;
    }


    /* Send the reference ID string */
    if ( (iError = iLwpsStringWrite(plLwps, pucReferenceID)) != LWPS_NoError ) {
        goto bailFromiLwpsInitRequestSend;
    }



    /* Bail label */
    bailFromiLwpsInitRequestSend:


    /* Handle the error */
    if ( iError == LWPS_NoError ) {
    
        /* Send the data */
        if ( (iError = iUtlNetSend(plLwps->pvUtlNet)) != UTL_NoError ) {
            iError = iLwpsMapUtlNetError(iError, LWPS_FailedWriteData);
        }
    }
#if defined(LWPS_ENABLE_CLEAR_SEND_BUFFER_ON_SEND_ERROR)
    else {

        /* Reset the send buffer to clear it */
        if ( (iError = iUtlNetResetSend(plLwps->pvUtlNet)) != UTL_NoError ) {
            iError = iLwpsMapUtlNetError(iError, LWPS_FailedWriteData);
        }
    }
#endif    /* defined(LWPS_ENABLE_CLEAR_SEND_BUFFER_ON_SEND_ERROR) */


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLwpsInitRequestReceive()

    Purpose:    Receive an init request from a client, this function assumes
                that the header has already been read

    Parameters: pvLwps              lwps structure
                ppucUserName        return pointer for the user name (allocated)
                ppucPassWord        return pointer for the password (allocated)
                ppucReferenceID     return pointer for the reference ID (allocated)

    Globals:    none

    Returns:    LWPS error code

*/
int iLwpsInitRequestReceive
(
    void *pvLwps,
    unsigned char **ppucUserName,
    unsigned char **ppucPassWord,
    unsigned char **ppucReferenceID
)
{

    int             iError = LWPS_NoError;
    struct lwps     *plLwps = (struct lwps *)pvLwps;
    unsigned int    uiMessageID = LWPS_INVALID_ID;
    unsigned char   *pucReferenceID = NULL;
    unsigned char   *pucUserName = NULL;
    unsigned char   *pucPassword = NULL;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iLwpsInitRequestReceive");  */


    /* Check the parameters */
    if ( pvLwps == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvLwps' parameter passed to 'iLwpsInitRequestReceive'."); 
        return (LWPS_InvalidLwps);
    }

    if ( ppucUserName == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppucUserName' parameter passed to 'iLwpsInitRequestReceive'."); 
        return (LWPS_ReturnParameterError);
    }

    if ( ppucPassWord == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppucPassWord' parameter passed to 'iLwpsInitRequestReceive'."); 
        return (LWPS_ReturnParameterError);
    }

    if ( ppucReferenceID == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppucReferenceID' parameter passed to 'iLwpsInitRequestReceive'."); 
        return (LWPS_ReturnParameterError);
    }


    /* Peek the response header, we do this here because we don't want 
    ** to harm the response if it is not what we expected
    */
    if ( (iError = iLwpsHeaderPeek(plLwps, &uiMessageID)) != LWPS_NoError ) {
        goto bailFromiLwpsInitRequestReceive;
    }


    /* Check that the response header message tag is correct */
    if ( uiMessageID != LWPS_INIT_REQUEST_ID) {
        iError = LWPS_InvalidMessageID;
        goto bailFromiLwpsInitRequestReceive;
    }


    /* Read the response header */
    if ( (iError = iLwpsHeaderReceive(plLwps, &uiMessageID)) != LWPS_NoError ) {
        goto bailFromiLwpsInitRequestReceive;
    }


    /* Read the user name */
    if ( (iError = iLwpsStringRead(plLwps, &pucUserName)) != LWPS_NoError ) {
        goto bailFromiLwpsInitRequestReceive;
    }


    /* Read the password */
    if ( (iError = iLwpsStringRead(plLwps, &pucPassword)) != LWPS_NoError ) {
        goto bailFromiLwpsInitRequestReceive;
    }


    /* Read the reference ID */
    if ( (iError = iLwpsStringRead(plLwps, &pucReferenceID)) != LWPS_NoError ) {
        goto bailFromiLwpsInitRequestReceive;
    }



    /* Bail label */
    bailFromiLwpsInitRequestReceive:


    /* Handle the error */
    if ( iError == LWPS_NoError ) {

        /* Set the return pointers */
        *ppucUserName = pucUserName;
        *ppucPassWord = pucPassword;
        *ppucReferenceID = pucReferenceID;
    }    
    else {

        /* Free allocations */
        s_free(pucUserName);
        s_free(pucPassword);
        s_free(pucReferenceID);
    }


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLwpsInitRequestHandle()

    Purpose:    Handle the init message negociation by sending an init request
                to a server and receiving the response

    Parameters: pvLwps              lwps structure
                pucUserName         user name (optional)
                pucPassword         password (optional)
                piErrorCode         return pointer for the error code
                ppucErrorString     return pointer for the error string (allocated)

    Globals:    none

    Returns:    LWPS error code

*/
int iLwpsInitRequestHandle
(
    void *pvLwps,
    unsigned char *pucUserName,
    unsigned char *pucPassword,
    int *piErrorCode,
    unsigned char **ppucErrorString
)
{

    int             iError = LWPS_NoError;
    struct lwps     *plLwps = (struct lwps *)pvLwps;
    unsigned char   pucSentReferenceID[LWPS_REFERENCE_ID_BUFFER_LENGTH + 1] = {'\0'};
    unsigned char   *pucReturnedReferenceID = NULL;
    unsigned int    uiMessageID = LWPS_INVALID_ID;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iLwpsInitRequestHandle");  */


    /* Check the parameters */
    if ( pvLwps == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvLwps' parameter passed to 'iLwpsInitRequestHandle'."); 
        return (LWPS_InvalidLwps);
    }

    if ( piErrorCode == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'piErrorCode' parameter passed to 'iLwpsInitRequestHandle'."); 
        return (LWPS_ReturnParameterError);
    }

    if ( ppucErrorString == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppucErrorString' parameter passed to 'iLwpsInitRequestHandle'."); 
        return (LWPS_ReturnParameterError);
    }


    /* Create a reference ID */
    snprintf(pucSentReferenceID, LWPS_REFERENCE_ID_BUFFER_LENGTH + 1, "%u", plLwps->uiReferenceID);

    
    /* Increment the reference ID */
    plLwps->uiReferenceID++;


    /* Clear the returned error code */
    *piErrorCode = SPI_NoError;
    

    /* Send the init request */
    if ( (iError = iLwpsInitRequestSend(plLwps, pucUserName, pucPassword, pucSentReferenceID)) != LWPS_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to send an init request message, lwps error: %d.", iError);
        goto bailFromiLwpsInitRequestHandle;
    }


    /* Receive the data */
    if ( (iError = iUtlNetReceive(plLwps->pvUtlNet)) != UTL_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to receive data, utl error: %d.", iError);
        iError = iLwpsMapUtlNetError(iError, LWPS_FailedReadData);
        goto bailFromiLwpsInitRequestHandle;
    }


    /* Peek the header */
    if ( (iError = iLwpsHeaderPeek(pvLwps, &uiMessageID)) != LWPS_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to peek the lwps header, lwps error: %d.", iError);
        goto bailFromiLwpsInitRequestHandle;
    }


    /* Handle the message */
    if ( uiMessageID == LWPS_INIT_RESPONSE_ID ) {

        /* Receive the init response */
        if ( (iError = iLwpsInitResponseReceive(plLwps, &pucReturnedReferenceID)) != LWPS_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to receive an init response message, lwps error: %d.", iError);
            goto bailFromiLwpsInitRequestHandle;
        }
    }
    else if ( uiMessageID == LWPS_ERROR_MESSAGE_ID ) {

        /* Receive the error message */
        if ( (iError = iLwpsErrorMessageReceive(plLwps, piErrorCode, ppucErrorString, &pucReturnedReferenceID)) != LWPS_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to receive an lwps error message, lwps error: %d.", iError);
            goto bailFromiLwpsInitRequestHandle;
        }
    }
    else {
        iError = LWPS_InvalidMessageID;
        goto bailFromiLwpsInitRequestHandle;
    }


    /* Check that the reference ID was correctly returned */
    if ( !((bUtlStringsIsStringNULL(pucReturnedReferenceID) == false) && (s_strcmp(pucReturnedReferenceID, pucSentReferenceID) == 0)) ) {
        iError = LWPS_InvalidReferenceID;
        goto bailFromiLwpsInitRequestHandle;
    }



    /* Bail label */
    bailFromiLwpsInitRequestHandle:


    /* Free allocated pointers */
    s_free(pucReturnedReferenceID);


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLwpsInitResponseSend()

    Purpose:    Send an init response to a client

    Parameters: pvLwps              lwps structure
                pucReferenceID      reference ID (optional)

    Globals:    none

    Returns:    LWPS error code

*/
int iLwpsInitResponseSend
(
    void *pvLwps,
    unsigned char *pucReferenceID
)
{

    int             iError = LWPS_NoError;
    struct lwps     *plLwps = (struct lwps *)pvLwps;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iLwpsInitResponseSend");  */


    /* Check the parameters */
    if ( pvLwps == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvLwps' parameter passed to 'iLwpsInitResponseSend'."); 
        return (LWPS_InvalidLwps);
    }


    /* Send the reply header */
    if ( (iError = iLwpsHeaderSend(plLwps, LWPS_INIT_RESPONSE_ID)) != LWPS_NoError ) {
        goto bailFromiLwpsInitResponseSend;
    }


    /* Send the reference ID string */
    if ( (iError = iLwpsStringWrite(plLwps, pucReferenceID)) != LWPS_NoError ) {
        goto bailFromiLwpsInitResponseSend;
    }



    /* Bail label */
    bailFromiLwpsInitResponseSend:


    /* Handle the error */
    if ( iError == LWPS_NoError ) {
    
        /* Send the data */
        if ( (iError = iUtlNetSend(plLwps->pvUtlNet)) != UTL_NoError ) {
            iError = iLwpsMapUtlNetError(iError, LWPS_FailedWriteData);
        }
    }
#if defined(LWPS_ENABLE_CLEAR_SEND_BUFFER_ON_SEND_ERROR)
    else {

        /* Reset the send buffer to clear it */
        if ( (iError = iUtlNetResetSend(plLwps->pvUtlNet)) != UTL_NoError ) {
            iError = iLwpsMapUtlNetError(iError, LWPS_FailedWriteData);
        }
    }
#endif    /* defined(LWPS_ENABLE_CLEAR_SEND_BUFFER_ON_SEND_ERROR) */


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLwpsInitResponseReceive()

    Purpose:    Receive an init response from a server

    Parameters: pvLwps              lwps structure
                ppucReferenceID     return pointer for the reference ID (allocated)

    Globals:    none

    Returns:    LWPS error code

*/
int iLwpsInitResponseReceive
(
    void *pvLwps,
    unsigned char **ppucReferenceID
)
{

    int             iError = LWPS_NoError;
    struct lwps     *plLwps = (struct lwps *)pvLwps;
    unsigned int    uiMessageID = LWPS_INVALID_ID;
    unsigned char   *pucReferenceID = NULL;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iLwpsInitResponseReceive");  */


    /* Check the parameters */
    if ( pvLwps == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvLwps' parameter passed to 'iLwpsInitResponseReceive'."); 
        return (LWPS_InvalidLwps);
    }

    if ( ppucReferenceID == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppucReferenceID' parameter passed to 'iLwpsInitResponseReceive'."); 
        return (LWPS_ReturnParameterError);
    }


    /* Peek the response header, we do this here because we don't want 
    ** to harm the response if it is not what we expected
    */
    if ( (iError = iLwpsHeaderPeek(plLwps, &uiMessageID)) != LWPS_NoError ) {
        goto bailFromiLwpsInitResponseReceive;
    }


    /* Check that the response header message tag is correct */
    if ( uiMessageID != LWPS_INIT_RESPONSE_ID) {
        iError = LWPS_InvalidMessageID;
        goto bailFromiLwpsInitResponseReceive;
    }


    /* Read the response header */
    if ( (iError = iLwpsHeaderReceive(plLwps, &uiMessageID)) != LWPS_NoError ) {
        goto bailFromiLwpsInitResponseReceive;
    }


    /* Read the reference ID */
    if ( (iError = iLwpsStringRead(plLwps, &pucReferenceID)) != LWPS_NoError ) {
        goto bailFromiLwpsInitResponseReceive;
    }



    /* Bail label */
    bailFromiLwpsInitResponseReceive:


    /* Handle the error */
    if ( iError == LWPS_NoError ) {

        /* Set the return pointers */
        *ppucReferenceID = pucReferenceID;
    }    


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLwpsInitResponseHandle()

    Purpose:    Handle the init response negociation by receiving an init request
                from a client and sending the response

    Parameters: pvLwps      lwps structure

    Globals:    none

    Returns:    LWPS error code

*/
int iLwpsInitResponseHandle
(
    void *pvLwps
)
{

    int             iError = LWPS_NoError;
    int             iErrorCode = SPI_NoError;
    unsigned char   *pucErrorString = NULL;
    struct lwps     *plLwps = (struct lwps *)pvLwps;
    unsigned char   *pucUserName = NULL;
    unsigned char   *pucPassword = NULL;
    unsigned char   *pucReferenceID = NULL;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iLwpsInitResponseHandle");  */


    /* Check the parameters */
    if ( pvLwps == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvLwps' parameter passed to 'iLwpsInitResponseHandle'."); 
        return (LWPS_InvalidLwps);
    }


    /* Receive the init request */
    if ( (iError = iLwpsInitRequestReceive(plLwps, &pucUserName, &pucPassword, &pucReferenceID)) != LWPS_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to receive an init request message, lwps error: %d.", iError);
        goto bailFromiLwpsInitResponseHandle;
    }


    /* Put in your init code here */
/*     iErrorCode = myInitCode(); */


    /* Handle the init error */
    if ( iErrorCode == SPI_NoError ) {

        /* Send the init response */
        if ( (iError = iLwpsInitResponseSend(plLwps, pucReferenceID)) != LWPS_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to send an lwps search response message, lwps error: %d.", iError);
            goto bailFromiLwpsInitResponseHandle;
        }
    }
    else {

        /* Send the error */
        if ( (iError = iLwpsErrorMessageSend(plLwps, iErrorCode, pucErrorString, pucReferenceID)) != LWPS_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to send an lwps error message, lwps error: %d.", iError);
            goto bailFromiLwpsInitResponseHandle;
        }
    }
    


    /* Bail label */
    bailFromiLwpsInitResponseHandle:


    /* Free allocated pointers */
    s_free(pucUserName);
    s_free(pucPassword);
    s_free(pucReferenceID);


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLwpsSearchRequestSend()

    Purpose:    Send a search request to a server

    Parameters: pvLwps                      lwps structure
                ppucIndexNameList           NULL terminated index name list (optional)
                pucLanguageCode             language code (optional)
                pucSearchText               search text (optional)
                pucPositiveFeedbackText     positive feedback text (optional)
                pucNegativeFeedbackText     negative feedback text (optional)
                uiStartIndex                start index
                uiEndIndex                  end index
                pucReferenceID              reference ID (optional)

    Globals:    none

    Returns:    LWPS error code

*/
int iLwpsSearchRequestSend
(
    void *pvLwps,
    unsigned char **ppucIndexNameList,
    unsigned char *pucLanguageCode,
    unsigned char *pucSearchText,
    unsigned char *pucPositiveFeedbackText,
    unsigned char *pucNegativeFeedbackText,
    unsigned int uiStartIndex,
    unsigned int uiEndIndex,
    unsigned char *pucReferenceID
)
{

    int             iError = LWPS_NoError;
    struct lwps     *plLwps = (struct lwps *)pvLwps;
    unsigned int    uiIndexNameCount = 0;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iLwpsSearchRequestSend");  */


    /* Check the parameters */
    if ( pvLwps == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvLwps' parameter passed to 'iLwpsSearchRequestSend'."); 
        return (LWPS_InvalidLwps);
    }

    if ( uiStartIndex < 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiStartIndex' parameter passed to 'iLwpsSearchRequestSend'."); 
        return (LWPS_ParameterError);
    }

    if ( uiEndIndex < 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiEndIndex' parameter passed to 'iLwpsSearchRequestSend'."); 
        return (LWPS_ParameterError);
    }

    if ( uiStartIndex > uiEndIndex ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiStartIndex' & 'uiEndIndex' parameters passed to 'iLwpsSearchRequestSend'."); 
        return (LWPS_ParameterError);
    }


    /* Send the message header */
    if ( (iError = iLwpsHeaderSend(plLwps, LWPS_SEARCH_REQUEST_ID)) != LWPS_NoError ) {
        goto bailFromiLwpsSearchRequestSend;
    }


    /* Find out how many entries there are the index name list */
    if ( ppucIndexNameList != NULL ) {
        for ( uiIndexNameCount = 0; ppucIndexNameList[uiIndexNameCount] != NULL; uiIndexNameCount++ ) {
            ;
        }
    }


    /* Send the number of index names */
    if ( (iError = iLwpsIntegerWrite(plLwps, uiIndexNameCount)) != LWPS_NoError ) {
        goto bailFromiLwpsSearchRequestSend;
    }


    /* Send the index names */
    if ( ppucIndexNameList != NULL ) {
        for ( uiIndexNameCount = 0; ppucIndexNameList[uiIndexNameCount] != NULL; uiIndexNameCount++ ) {
            if ( (iError = iLwpsStringWrite(plLwps, ppucIndexNameList[uiIndexNameCount])) != LWPS_NoError ) {
                goto bailFromiLwpsSearchRequestSend;
            }
        }
    }


    /* Send the language code */
    if ( (iError = iLwpsStringWrite(plLwps, pucLanguageCode)) != LWPS_NoError ) {
        goto bailFromiLwpsSearchRequestSend;
    }


    /* Send the search text  */
    if ( (iError = iLwpsStringWrite(plLwps, pucSearchText)) != LWPS_NoError ) {
        goto bailFromiLwpsSearchRequestSend;
    }


    /* Send the positive feedback text  */
    if ( (iError = iLwpsStringWrite(plLwps, pucPositiveFeedbackText)) != LWPS_NoError ) {
        goto bailFromiLwpsSearchRequestSend;
    }


    /* Send the negative feedback text  */
    if ( (iError = iLwpsStringWrite(plLwps, pucNegativeFeedbackText)) != LWPS_NoError ) {
        goto bailFromiLwpsSearchRequestSend;
    }


    /* Send the start index */
    if ( (iError = iLwpsIntegerWrite(plLwps, uiStartIndex)) != LWPS_NoError ) {
        goto bailFromiLwpsSearchRequestSend;
    }


    /* Send the end index */
    if ( (iError = iLwpsIntegerWrite(plLwps, uiEndIndex)) != LWPS_NoError ) {
        goto bailFromiLwpsSearchRequestSend;
    }

    /* Send the reference ID string */
    if ( (iError = iLwpsStringWrite(plLwps, pucReferenceID)) != LWPS_NoError ) {
        goto bailFromiLwpsSearchRequestSend;
    }



    /* Bail label */
    bailFromiLwpsSearchRequestSend:


    /* Handle the error */
    if ( iError == LWPS_NoError ) {
    
        /* Send the data */
        if ( (iError = iUtlNetSend(plLwps->pvUtlNet)) != UTL_NoError ) {
            iError = iLwpsMapUtlNetError(iError, LWPS_FailedWriteData);
        }
    }
#if defined(LWPS_ENABLE_CLEAR_SEND_BUFFER_ON_SEND_ERROR)
    else {

        /* Reset the send buffer to clear it */
        if ( (iError = iUtlNetResetSend(plLwps->pvUtlNet)) != UTL_NoError ) {
            iError = iLwpsMapUtlNetError(iError, LWPS_FailedWriteData);
        }
    }
#endif    /* defined(LWPS_ENABLE_CLEAR_SEND_BUFFER_ON_SEND_ERROR) */


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLwpsSearchRequestReceive()

    Purpose:    Receive a search request from a client, this function assumes
                that the header has already been read

    Parameters: pvLwps                      lwps structure
                pppucIndexNameList          return pointer for the NULL terminated index name list (allocated)
                ppucLanguageCode            return pointer for the language code (allocated)
                ppucSearchText              return pointer for the search text (allocated)
                ppucPositiveFeedbackText    return pointer for the positive feedback text (allocated)
                ppucNegativeFeedbackText    return pointer for the negative feedback text (allocated)
                puiStartIndex               return pointer for the start index
                puiEndIndex                 return pointer for the end index
                ppucReferenceID             return pointer for the reference ID (allocated)

    Globals:    none

    Returns:    LWPS error code

*/
int iLwpsSearchRequestReceive
(
    void *pvLwps,
    unsigned char ***pppucIndexNameList,
    unsigned char **ppucLanguageCode,
    unsigned char **ppucSearchText,
    unsigned char **ppucPositiveFeedbackText,
    unsigned char **ppucNegativeFeedbackText,
    unsigned int *puiStartIndex,
    unsigned int *puiEndIndex,
    unsigned char **ppucReferenceID
)
{

    int             iError = LWPS_NoError;
    unsigned int    uiMessageID = LWPS_INVALID_ID;
    struct lwps     *plLwps = (struct lwps *)pvLwps;
    unsigned char   **ppucIndexNameList = NULL;
    unsigned char   *pucIndexName = NULL;
    unsigned char   *pucLanguageCode = NULL;
    unsigned char   *pucSearchText = NULL;
    unsigned char   *pucPositiveFeedbackText = NULL;
    unsigned char   *pucNegativeFeedbackText = NULL;
    unsigned int    uiStartIndex = 0;
    unsigned int    uiEndIndex = 0;
    unsigned char   *pucReferenceID = NULL;
    unsigned int    uiIndexNameCount = 0;
    unsigned int    uiI = 0;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iLwpsSearchRequestReceive");  */


    /* Check the parameters */
    if ( pvLwps == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvLwps' parameter passed to 'iLwpsSearchRequestReceive'."); 
        return (LWPS_InvalidLwps);
    }

    if ( pppucIndexNameList == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pppucIndexNameList' parameter passed to 'iLwpsSearchRequestReceive'."); 
        return (LWPS_ReturnParameterError);
    }

    if ( ppucLanguageCode == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppucLanguageCode' parameter passed to 'iLwpsSearchRequestReceive'."); 
        return (LWPS_ReturnParameterError);
    }

    if ( ppucSearchText == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppucSearchText' parameter passed to 'iLwpsSearchRequestReceive'."); 
        return (LWPS_ReturnParameterError);
    }

    if ( ppucPositiveFeedbackText == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppucPositiveFeedbackText' parameter passed to 'iLwpsSearchRequestReceive'."); 
        return (LWPS_ReturnParameterError);
    }

    if ( ppucNegativeFeedbackText == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppucNegativeFeedbackText' parameter passed to 'iLwpsSearchRequestReceive'."); 
        return (LWPS_ReturnParameterError);
    }

    if ( puiStartIndex == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'puiStartIndex' parameter passed to 'iLwpsSearchRequestReceive'."); 
        return (LWPS_ReturnParameterError);
    }

    if ( puiEndIndex == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'puiEndIndex' parameter passed to 'iLwpsSearchRequestReceive'."); 
        return (LWPS_ReturnParameterError);
    }

    if ( ppucReferenceID == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppucReferenceID' parameter passed to 'iLwpsSearchRequestReceive'."); 
        return (LWPS_ReturnParameterError);
    }


    /* Peek the response header, we do this here because we don't want 
    ** to harm the response if it is not what we expected
    */
    if ( (iError = iLwpsHeaderPeek(plLwps, &uiMessageID)) != LWPS_NoError ) {
        goto bailFromiLwpsSearchRequestReceive;
    }


    /* Check that the response header message tag is correct */
    if ( uiMessageID != LWPS_SEARCH_REQUEST_ID ) {
        iError = LWPS_InvalidMessageID;
        goto bailFromiLwpsSearchRequestReceive;
    }


    /* Read the response header */
    if ( (iError = iLwpsHeaderReceive(plLwps, &uiMessageID)) != LWPS_NoError ) {
        goto bailFromiLwpsSearchRequestReceive;
    }


    /* Read the number of index names */
    if ( (iError = iLwpsIntegerRead(plLwps, &uiIndexNameCount)) != LWPS_NoError ) {
        goto bailFromiLwpsSearchRequestReceive;
    }


    /* Read index names if there are any */
    if ( uiIndexNameCount > 0 ) {

        /* Allocate the index name list */
        if ( (ppucIndexNameList = (unsigned char **)s_malloc((size_t)(sizeof(unsigned char *) * (uiIndexNameCount + 1)))) == NULL ) {
            iError = LWPS_MemError;
            goto bailFromiLwpsSearchRequestReceive;
        }

        /* Read the index names */
        for ( uiI = 0; uiI < uiIndexNameCount; uiI++ ) {
            if ( (iError = iLwpsStringRead(plLwps, &pucIndexName)) != LWPS_NoError ) {
                goto bailFromiLwpsSearchRequestReceive;
            }

            /* Attach the index name to the list and reset the index name pointer */
            ppucIndexNameList[uiI] = pucIndexName;
            pucIndexName = NULL;
        }    

        /* Make sure the final index name pointer is set to NULL */
        ppucIndexNameList[uiI] = NULL;
    }


    /* Read the language code */
    if ( (iError = iLwpsStringRead(plLwps, &pucLanguageCode)) != LWPS_NoError ) {
        goto bailFromiLwpsSearchRequestReceive;
    }


    /* Read the search text */
    if ( (iError = iLwpsStringRead(plLwps, &pucSearchText)) != LWPS_NoError ) {
        goto bailFromiLwpsSearchRequestReceive;
    }


    /* Read the positive feedback text */
    if ( (iError = iLwpsStringRead(plLwps, &pucPositiveFeedbackText)) != LWPS_NoError ) {
        goto bailFromiLwpsSearchRequestReceive;
    }


    /* Read the negative feedback text */
    if ( (iError = iLwpsStringRead(plLwps, &pucNegativeFeedbackText)) != LWPS_NoError ) {
        goto bailFromiLwpsSearchRequestReceive;
    }


    /* Read the start index */
    if ( (iError = iLwpsIntegerRead(plLwps, &uiStartIndex)) != LWPS_NoError ) {
        goto bailFromiLwpsSearchRequestReceive;
    }


    /* Read the end index */
    if ( (iError = iLwpsIntegerRead(plLwps, &uiEndIndex)) != LWPS_NoError ) {
        goto bailFromiLwpsSearchRequestReceive;
    }


    /* Read the reference ID */
    if ( (iError = iLwpsStringRead(plLwps, &pucReferenceID)) != LWPS_NoError ) {
        goto bailFromiLwpsSearchRequestReceive;
    }



    /* Bail label */
    bailFromiLwpsSearchRequestReceive:


    /* Handle the error */
    if ( iError == LWPS_NoError ) {

        /* Set the return pointers */
        *pppucIndexNameList = ppucIndexNameList;
        *ppucLanguageCode = pucLanguageCode;
        *ppucSearchText = pucSearchText;
        *ppucPositiveFeedbackText = pucPositiveFeedbackText;
        *ppucNegativeFeedbackText = pucNegativeFeedbackText;
        *puiStartIndex = uiStartIndex;
        *puiEndIndex = uiEndIndex;
        *ppucReferenceID = pucReferenceID;
    }    
    else {

        /* Free allocations */
        UTL_MACROS_FREE_NULL_TERMINATED_LIST(ppucIndexNameList);
        s_free(pucSearchText);
        s_free(pucPositiveFeedbackText);
        s_free(pucNegativeFeedbackText);
        s_free(pucReferenceID);
    }


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLwpsSearchRequestHandle()

    Purpose:    Handle a search by sending a search request
                to a server and receiving the response

    Parameters: pvLwps                      lwps structure
                ppucIndexNameList           NULL terminated index name list (optional)
                pucLanguageCode             language code (optional)
                pucSearchText               search text (optional)
                pucPositiveFeedbackText     positive feedback text (optional)
                pucNegativeFeedbackText     negative feedback text (optional)
                uiStartIndex                start index
                uiEndIndex                  end index
                ppssrSpiSearchResponse      return pointer for the spi search response structure (allocated)
                piErrorCode                 return pointer for the error code
                ppucErrorString             return pointer for the error string (allocated)

    Globals:    none

    Returns:    LWPS error code

*/
int iLwpsSearchRequestHandle
(
    void *pvLwps,
    unsigned char **ppucIndexNameList,
    unsigned char *pucLanguageCode,
    unsigned char *pucSearchText,
    unsigned char *pucPositiveFeedbackText,
    unsigned char *pucNegativeFeedbackText,
    unsigned int uiStartIndex,
    unsigned int uiEndIndex,
    struct spiSearchResponse **ppssrSpiSearchResponse,
    int *piErrorCode,
    unsigned char **ppucErrorString
)
{

    int             iError = LWPS_NoError;
    struct lwps     *plLwps = (struct lwps *)pvLwps;
    unsigned int    uiMessageID = LWPS_INVALID_ID;
    unsigned char   pucSentReferenceID[LWPS_REFERENCE_ID_BUFFER_LENGTH + 1] = {'\0'};
    unsigned char   *pucReturnedReferenceID = NULL;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iLwpsSearchRequestHandle");  */


    /* Check the parameters */
    if ( pvLwps == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvLwps' parameter passed to 'iLwpsSearchRequestHandle'."); 
        return (LWPS_InvalidLwps);
    }

    if ( uiStartIndex < 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiStartIndex' parameter passed to 'iLwpsSearchRequestHandle'."); 
        return (LWPS_ParameterError);
    }

    if ( uiEndIndex < 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiEndIndex' parameter passed to 'iLwpsSearchRequestHandle'."); 
        return (LWPS_ParameterError);
    }

    if ( uiStartIndex > uiEndIndex ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiStartIndex' & 'uiEndIndex' parameters passed to 'iLwpsSearchRequestHandle'."); 
        return (LWPS_ParameterError);
    }

    if ( ppssrSpiSearchResponse == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppssrSpiSearchResponse' parameter passed to 'iLwpsSearchRequestHandle'."); 
        return (LWPS_ReturnParameterError);
    }

    if ( piErrorCode == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'piErrorCode' parameter passed to 'iLwpsSearchRequestHandle'."); 
        return (LWPS_ReturnParameterError);
    }

    if ( ppucErrorString == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppucErrorString' parameter passed to 'iLwpsSearchRequestHandle'."); 
        return (LWPS_ReturnParameterError);
    }


    /* Create a reference ID */
    snprintf(pucSentReferenceID, LWPS_REFERENCE_ID_BUFFER_LENGTH + 1, "%u", plLwps->uiReferenceID);

    
    /* Increment the reference ID */
    plLwps->uiReferenceID++;


    /* Clear the returned error code */
    *piErrorCode = SPI_NoError;
    

    /* Send the search request */
    if ( (iError = iLwpsSearchRequestSend(plLwps, ppucIndexNameList, pucLanguageCode, pucSearchText, pucPositiveFeedbackText, pucNegativeFeedbackText,
            uiStartIndex, uiEndIndex, pucSentReferenceID)) != LWPS_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to send an lwps search request message, lwps error: %d.", iError);
        goto bailFromiLwpsSearchRequestHandle;
    }


    /* Receive the data */
    if ( (iError = iUtlNetReceive(plLwps->pvUtlNet)) != UTL_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to receive data, utl error: %d.", iError);
        iError = iLwpsMapUtlNetError(iError, LWPS_FailedReadData);
        goto bailFromiLwpsSearchRequestHandle;
    }


    /* Peek the header */
    if ( (iError = iLwpsHeaderPeek(pvLwps, &uiMessageID)) != LWPS_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to peek the lwps header, lwps error: %d.", iError);
        goto bailFromiLwpsSearchRequestHandle;
    }


    /* Handle the message */
    if ( uiMessageID == LWPS_SEARCH_RESPONSE_TAG ) {

        /* Receive the search response */
        if ( (iError = iLwpsSearchResponseReceive(plLwps, ppssrSpiSearchResponse, &pucReturnedReferenceID)) != LWPS_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to receive an lwps search response message, lwps error: %d.", iError);
            goto bailFromiLwpsSearchRequestHandle;
        }
    }
    else if ( uiMessageID == LWPS_ERROR_MESSAGE_ID ) {

        /* Receive the error message */
        if ( (iError = iLwpsErrorMessageReceive(plLwps, piErrorCode, ppucErrorString, &pucReturnedReferenceID)) != LWPS_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to receive an lwps error message, lwps error: %d.", iError);
            goto bailFromiLwpsSearchRequestHandle;
        }
    }
    else {
        iError = LWPS_InvalidMessageID;
        goto bailFromiLwpsSearchRequestHandle;
    }


    /* Check that the reference ID was correctly returned */
    if ( !((bUtlStringsIsStringNULL(pucReturnedReferenceID) == false) && (s_strcmp(pucReturnedReferenceID, pucSentReferenceID) == 0)) ) {
        iError = LWPS_InvalidReferenceID;
        goto bailFromiLwpsSearchRequestHandle;
    }



    /* Bail label */
    bailFromiLwpsSearchRequestHandle:


    /* Free allocated pointers */
    s_free(pucReturnedReferenceID);


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLwpsSearchResponseSend()

    Purpose:    Send a search response to a client

    Parameters: pvLwps                  lwps structure
                pssrSpiSearchResponse   search response structure (required)
                pucReferenceID          reference ID

    Globals:    none

    Returns:    LWPS error code

*/
int iLwpsSearchResponseSend
(
    void *pvLwps,
    struct spiSearchResponse *pssrSpiSearchResponse,
    unsigned char *pucReferenceID
)
{

    int                     iError = LWPS_NoError;
    struct lwps             *plLwps = (struct lwps *)pvLwps;
    unsigned int            uiI = 0, uiJ = 0;
    struct spiSearchResult  *pssrSpiSearchResultsPtr = NULL;
    struct spiDocumentItem  *psdiSpiDocumentItemsPtr = NULL;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iLwpsSearchResponseSend");  */


    /* Check the parameters */
    if ( pvLwps == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvLwps' parameter passed to 'iLwpsSearchResponseSend'."); 
        return (LWPS_InvalidLwps);
    }

    if ( pssrSpiSearchResponse == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'pssrSpiSearchResponse' parameter passed to 'iLwpsSearchResponseSend'."); 
        return (LWPS_ParameterError);
    }


    /* Send the reply header */
    if ( (iError = iLwpsHeaderSend(plLwps, LWPS_SEARCH_RESPONSE_TAG)) != LWPS_NoError ) {
        goto bailFromiLwpsSearchResponseSend;
    }


    /* Send the sort type, we send it here because we need to know
    ** how it is coded at the other end before we read it :)
    */
    if ( (iError = iLwpsIntegerWrite(plLwps, pssrSpiSearchResponse->uiSortType)) != LWPS_NoError ) {
        goto bailFromiLwpsSearchResponseSend;
    }


    /* Send the number of search results */
    if ( (iError = iLwpsIntegerWrite(plLwps, pssrSpiSearchResponse->uiSpiSearchResultsLength)) != LWPS_NoError ) {
        goto bailFromiLwpsSearchResponseSend;
    }


    /* Cycle through the search results and send them, they are already in order */
    for ( uiI = 0, pssrSpiSearchResultsPtr = pssrSpiSearchResponse->pssrSpiSearchResults; uiI < pssrSpiSearchResponse->uiSpiSearchResultsLength; uiI++, pssrSpiSearchResultsPtr++ ) { 

        /* Send the index */
        if ( (iError = iLwpsStringWrite(plLwps, pssrSpiSearchResultsPtr->pucIndexName)) != LWPS_NoError ) {
            goto bailFromiLwpsSearchResponseSend;
        }

        /* Send the document key */
        if ( (iError = iLwpsStringWrite(plLwps, pssrSpiSearchResultsPtr->pucDocumentKey)) != LWPS_NoError ) {
            goto bailFromiLwpsSearchResponseSend;
        }

        /* Send the title */
        if ( (iError = iLwpsStringWrite(plLwps, pssrSpiSearchResultsPtr->pucTitle)) != LWPS_NoError ) {
            goto bailFromiLwpsSearchResponseSend;
        }

        /* Send the sort key */
        switch ( pssrSpiSearchResponse->uiSortType ) {

            case SPI_SORT_TYPE_DOUBLE_ASC:
            case SPI_SORT_TYPE_DOUBLE_DESC:
                if ( (iError = iLwpsDoubleWrite(plLwps, pssrSpiSearchResultsPtr->dSortKey)) != LWPS_NoError ) {
                    goto bailFromiLwpsSearchResponseSend;
                }
                break;

            case SPI_SORT_TYPE_FLOAT_ASC:
            case SPI_SORT_TYPE_FLOAT_DESC:
                if ( (iError = iLwpsFloatWrite(plLwps, pssrSpiSearchResultsPtr->fSortKey)) != LWPS_NoError ) {
                    goto bailFromiLwpsSearchResponseSend;
                }
                break;
            
            case SPI_SORT_TYPE_UINT_ASC:
            case SPI_SORT_TYPE_UINT_DESC:
                if ( (iError = iLwpsIntegerWrite(plLwps, pssrSpiSearchResultsPtr->uiSortKey)) != LWPS_NoError ) {
                    goto bailFromiLwpsSearchResponseSend;
                }
                break;
            
            case SPI_SORT_TYPE_ULONG_ASC:
            case SPI_SORT_TYPE_ULONG_DESC:
                if ( (iError = iLwpsLongWrite(plLwps, pssrSpiSearchResultsPtr->ulSortKey)) != LWPS_NoError ) {
                    goto bailFromiLwpsSearchResponseSend;
                }
                break;
            
            case SPI_SORT_TYPE_UCHAR_ASC:
            case SPI_SORT_TYPE_UCHAR_DESC:
                if ( (iError = iLwpsStringWrite(plLwps, pssrSpiSearchResultsPtr->pucSortKey)) != LWPS_NoError ) {
                    goto bailFromiLwpsSearchResponseSend;
                }
                break;
            
            case SPI_SORT_TYPE_NO_SORT:
            case SPI_SORT_TYPE_UNKNOWN:
                break;
            
            default:
                ASSERT(false);
                break;
        }

        /* Send the language code */
        if ( (iError = iLwpsStringWrite(plLwps, pssrSpiSearchResultsPtr->pucLanguageCode)) != LWPS_NoError ) {
            goto bailFromiLwpsSearchResponseSend;
        }

        /* Send the rank */
        if ( (iError = iLwpsIntegerWrite(plLwps, pssrSpiSearchResultsPtr->uiRank)) != LWPS_NoError ) {
            goto bailFromiLwpsSearchResponseSend;
        }

        /* Send the term count */
        if ( (iError = iLwpsIntegerWrite(plLwps, pssrSpiSearchResultsPtr->uiTermCount)) != LWPS_NoError ) {
            goto bailFromiLwpsSearchResponseSend;
        }

        /* Send the date */
        if ( (iError = iLwpsLongWrite(plLwps, pssrSpiSearchResultsPtr->ulAnsiDate)) != LWPS_NoError ) {
            goto bailFromiLwpsSearchResponseSend;
        }

        /* Send the number of document items */
        if ( (iError = iLwpsIntegerWrite(plLwps, pssrSpiSearchResultsPtr->uiDocumentItemsLength)) != LWPS_NoError ) {
            goto bailFromiLwpsSearchResponseSend;
        }


        /* Cycle through the document items */
        for ( uiJ = 0, psdiSpiDocumentItemsPtr = pssrSpiSearchResultsPtr->psdiSpiDocumentItems; uiJ < pssrSpiSearchResultsPtr->uiDocumentItemsLength; uiJ++, psdiSpiDocumentItemsPtr++ ) {

            /* Send the item name */
            if ( (iError = iLwpsStringWrite(plLwps, psdiSpiDocumentItemsPtr->pucItemName)) != LWPS_NoError ) {
                goto bailFromiLwpsSearchResponseSend;
            }

            /* Send the mime type */
            if ( (iError = iLwpsStringWrite(plLwps, psdiSpiDocumentItemsPtr->pucMimeType)) != LWPS_NoError) {
                goto bailFromiLwpsSearchResponseSend;
            }

            /* Send the URL */
            if ( (iError = iLwpsStringWrite(plLwps, psdiSpiDocumentItemsPtr->pucUrl)) != LWPS_NoError ) {
                goto bailFromiLwpsSearchResponseSend;
            }

            /* Send the length */
            if ( (iError = iLwpsIntegerWrite(plLwps, psdiSpiDocumentItemsPtr->uiLength)) != LWPS_NoError ) {
                goto bailFromiLwpsSearchResponseSend;
            }

            /* Send the data */
            if ( (iError = iLwpsDataWrite(plLwps, (unsigned char *)psdiSpiDocumentItemsPtr->pvData, psdiSpiDocumentItemsPtr->uiDataLength)) != LWPS_NoError ) {
                goto bailFromiLwpsSearchResponseSend;
            }

            /* Send the data length */
            if ( (iError = iLwpsIntegerWrite(plLwps, psdiSpiDocumentItemsPtr->uiDataLength)) != LWPS_NoError ) {
                goto bailFromiLwpsSearchResponseSend;
            }
        }
    }


    /* Send the total results */
    if ( (iError = iLwpsIntegerWrite(plLwps, pssrSpiSearchResponse->uiTotalResults)) != LWPS_NoError ) {
        goto bailFromiLwpsSearchResponseSend;
    }


    /* Send the start index */
    if ( (iError = iLwpsIntegerWrite(plLwps, pssrSpiSearchResponse->uiStartIndex)) != LWPS_NoError ) {
        goto bailFromiLwpsSearchResponseSend;
    }


    /* Send the end index */
    if ( (iError = iLwpsIntegerWrite(plLwps, pssrSpiSearchResponse->uiEndIndex)) != LWPS_NoError ) {
        goto bailFromiLwpsSearchResponseSend;
    }


    /* Send the max sort key */
    if ( (iError = iLwpsDoubleWrite(plLwps, pssrSpiSearchResponse->dMaxSortKey)) != LWPS_NoError ) {
        goto bailFromiLwpsSearchResponseSend;
    }


    /* Send the search time */
    if ( (iError = iLwpsDoubleWrite(plLwps, pssrSpiSearchResponse->dSearchTime)) != LWPS_NoError ) {
        goto bailFromiLwpsSearchResponseSend;
    }


    /* Send the reference ID string */
    if ( (iError = iLwpsStringWrite(plLwps, pucReferenceID)) != LWPS_NoError ) {
        goto bailFromiLwpsSearchResponseSend;
    }



    /* Bail label */
    bailFromiLwpsSearchResponseSend:


    /* Handle the error */
    if ( iError == LWPS_NoError ) {
    
        /* Send the data */
        if ( (iError = iUtlNetSend(plLwps->pvUtlNet)) != UTL_NoError ) {
            iError = iLwpsMapUtlNetError(iError, LWPS_FailedWriteData);
        }
    }
#if defined(LWPS_ENABLE_CLEAR_SEND_BUFFER_ON_SEND_ERROR)
    else {

        /* Reset the send buffer to clear it */
        if ( (iError = iUtlNetResetSend(plLwps->pvUtlNet)) != UTL_NoError ) {
            iError = iLwpsMapUtlNetError(iError, LWPS_FailedWriteData);
        }
    }
#endif    /* defined(LWPS_ENABLE_CLEAR_SEND_BUFFER_ON_SEND_ERROR) */


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLwpsSearchResponseReceive()

    Purpose:    Receive a search response from a server

    Parameters: pvLwps                      lwps structure
                ppssrSpiSearchResponse      return pointer for the spi search response structure (allocated)
                ppucReferenceID             return pointer for the reference ID (allocated)

    Globals:    none

    Returns:    LWPS error code

*/
int iLwpsSearchResponseReceive
(
    void *pvLwps,
    struct spiSearchResponse **ppssrSpiSearchResponse,
    unsigned char **ppucReferenceID
)
{

    int                     iError = LWPS_NoError;
    struct lwps             *plLwps = (struct lwps *)pvLwps;
    unsigned int            uiMessageID = LWPS_INVALID_ID;
    struct spiSearchResult  *pssrSpiSearchResults = NULL;
    unsigned int            uiSpiSearchResultsLength = 0;
    unsigned int            uiTotalResults = 0;
    unsigned int            uiStartIndex = 0;
    unsigned int            uiEndIndex = 0;
    unsigned int            uiSortType = SPI_SORT_TYPE_UNKNOWN;
    double                  dMaxSortKey = 0;
    double                  dSearchTime = 0;
    unsigned char           *pucReferenceID = NULL;
    unsigned int            uiI = 0, uiJ = 0;
    struct spiSearchResult  *pssrSpiSearchResultsPtr = NULL;
    struct spiDocumentItem  *psdiSpiDocumentItemsPtr = NULL;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iLwpsSearchResponseReceive");  */


    /* Check the parameters */
    if ( pvLwps == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvLwps' parameter passed to 'iLwpsSearchResponseReceive'."); 
        return (LWPS_InvalidLwps);
    }

    if ( ppssrSpiSearchResponse == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppssrSpiSearchResponse' parameter passed to 'iLwpsSearchResponseReceive'."); 
        return (LWPS_ReturnParameterError);
    }

    if ( ppucReferenceID == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppucReferenceID' parameter passed to 'iLwpsSearchResponseReceive'."); 
        return (LWPS_ReturnParameterError);
    }


    /* Peek the response header, we do this here because we don't want 
    ** to harm the response if it is not what we expected
    */
    if ( (iError = iLwpsHeaderPeek(plLwps, &uiMessageID)) != LWPS_NoError ) {
        goto bailFromiLwpsSearchResponseReceive;
    }


    /* Check that the response header message tag is correct */
    if ( uiMessageID != LWPS_SEARCH_RESPONSE_TAG ) {
        iError = LWPS_InvalidMessageID;
        goto bailFromiLwpsSearchResponseReceive;
    }


    /* Read the response header */
    if ( (iError = iLwpsHeaderReceive(plLwps, &uiMessageID)) != LWPS_NoError ) {
        goto bailFromiLwpsSearchResponseReceive;
    }


    /* Read the sort type, we read it here because we need to know
    ** how it was coded at the other end
    */
    if ( (iError = iLwpsIntegerRead(plLwps, &uiSortType)) != LWPS_NoError ) {
        goto bailFromiLwpsSearchResponseReceive;
    }


    /* Read the number of search results */
    if ( (iError = iLwpsIntegerRead(plLwps, &uiSpiSearchResultsLength)) != LWPS_NoError ) {
        goto bailFromiLwpsSearchResponseReceive;
    }


    /* Read results if there are any */
    if ( uiSpiSearchResultsLength > 0 ) {

        /* Allocate space for the search results */
        if ( (pssrSpiSearchResults = (struct spiSearchResult *)s_malloc((size_t)(sizeof(struct spiSearchResult) * uiSpiSearchResultsLength))) == NULL ) {
            iError = LWPS_MemError;
            goto bailFromiLwpsSearchResponseReceive;
        }

        /* Cycle through the search results and read them, they are already in order */
        for ( uiI = 0, pssrSpiSearchResultsPtr = pssrSpiSearchResults; uiI < uiSpiSearchResultsLength; uiI++, pssrSpiSearchResultsPtr++ ) { 

            /* Read the index name */
            if ( (iError = iLwpsStringRead(plLwps, &pssrSpiSearchResultsPtr->pucIndexName)) != LWPS_NoError ) {
                goto bailFromiLwpsSearchResponseReceive;
            }

            /* Read the document key */
            if ( (iError = iLwpsStringRead(plLwps, &pssrSpiSearchResultsPtr->pucDocumentKey)) != LWPS_NoError ) {
                goto bailFromiLwpsSearchResponseReceive;
            }

            /* Read the title */
            if ( (iError = iLwpsStringRead(plLwps, &pssrSpiSearchResultsPtr->pucTitle)) != LWPS_NoError ) {
                goto bailFromiLwpsSearchResponseReceive;
            }

            /* Read the sort key */
            switch ( uiSortType ) {
    
                case SPI_SORT_TYPE_DOUBLE_ASC:
                case SPI_SORT_TYPE_DOUBLE_DESC:
                    if ( (iError = iLwpsDoubleRead(plLwps, &pssrSpiSearchResultsPtr->dSortKey)) != LWPS_NoError ) {
                        goto bailFromiLwpsSearchResponseReceive;
                    }
                    break;
    
                case SPI_SORT_TYPE_FLOAT_ASC:
                case SPI_SORT_TYPE_FLOAT_DESC:
                    if ( (iError = iLwpsFloatRead(plLwps, &pssrSpiSearchResultsPtr->fSortKey)) != LWPS_NoError ) {
                        goto bailFromiLwpsSearchResponseReceive;
                    }
                    break;
                
                case SPI_SORT_TYPE_UINT_ASC:
                case SPI_SORT_TYPE_UINT_DESC:
                    if ( (iError = iLwpsIntegerRead(plLwps, &pssrSpiSearchResultsPtr->uiSortKey)) != LWPS_NoError ) {
                        goto bailFromiLwpsSearchResponseReceive;
                    }
                    break;
                
                case SPI_SORT_TYPE_ULONG_ASC:
                case SPI_SORT_TYPE_ULONG_DESC:
                    if ( (iError = iLwpsLongRead(plLwps, &pssrSpiSearchResultsPtr->ulSortKey)) != LWPS_NoError ) {
                        goto bailFromiLwpsSearchResponseReceive;
                    }
                    break;
                
                case SPI_SORT_TYPE_UCHAR_ASC:
                case SPI_SORT_TYPE_UCHAR_DESC:
                    if ( (iError = iLwpsStringRead(plLwps, &pssrSpiSearchResultsPtr->pucSortKey)) != LWPS_NoError ) {
                        goto bailFromiLwpsSearchResponseReceive;
                    }
                    break;
                
                case SPI_SORT_TYPE_NO_SORT:
                case SPI_SORT_TYPE_UNKNOWN:
                    break;
                
                default:
                    ASSERT(false);
                    break;
            }
    
            /* Read the language code */
            if ( (iError = iLwpsStringRead(plLwps, &pssrSpiSearchResultsPtr->pucLanguageCode)) != LWPS_NoError ) {
                goto bailFromiLwpsSearchResponseReceive;
            }

            /* Read the rank */
            if ( (iError = iLwpsIntegerRead(plLwps, &pssrSpiSearchResultsPtr->uiRank)) != LWPS_NoError ) {
                goto bailFromiLwpsSearchResponseReceive;
            }

            /* Read the term count */
            if ( (iError = iLwpsIntegerRead(plLwps, &pssrSpiSearchResultsPtr->uiTermCount)) != LWPS_NoError ) {
                goto bailFromiLwpsSearchResponseReceive;
            }

            /* Read the date */
            if ( (iError = iLwpsLongRead(plLwps, &pssrSpiSearchResultsPtr->ulAnsiDate)) != LWPS_NoError ) {
                goto bailFromiLwpsSearchResponseReceive;
            }

            /* Read the number of document items */
            if ( (iError = iLwpsIntegerRead(plLwps, &pssrSpiSearchResultsPtr->uiDocumentItemsLength)) != LWPS_NoError ) {
                goto bailFromiLwpsSearchResponseReceive;
            }


            /* Add the document items */
            if ( pssrSpiSearchResultsPtr->uiDocumentItemsLength > 0 ) {

                /* Allocate space for the document items */
                if ( (pssrSpiSearchResultsPtr->psdiSpiDocumentItems = (struct spiDocumentItem *)s_malloc((size_t)(sizeof(struct spiDocumentItem) * pssrSpiSearchResultsPtr->uiDocumentItemsLength))) == NULL ) {
                    iError = LWPS_MemError;
                    goto bailFromiLwpsSearchResponseReceive;
                }


                /* Cycle through the document items */
                for ( uiJ = 0, psdiSpiDocumentItemsPtr = pssrSpiSearchResultsPtr->psdiSpiDocumentItems; uiJ < pssrSpiSearchResultsPtr->uiDocumentItemsLength; uiJ++, psdiSpiDocumentItemsPtr++ ) {

                    /* Read the item name */
                    if ( (iError = iLwpsStringRead(plLwps, &psdiSpiDocumentItemsPtr->pucItemName)) != LWPS_NoError ) {
                        goto bailFromiLwpsSearchResponseReceive;
                    }

                    /* Read the mime type */
                    if ( (iError = iLwpsStringRead(plLwps, &psdiSpiDocumentItemsPtr->pucMimeType)) != LWPS_NoError ) {
                        goto bailFromiLwpsSearchResponseReceive;
                    }

                    /* Read the URL */
                    if ( (iError = iLwpsStringRead(plLwps, &psdiSpiDocumentItemsPtr->pucUrl)) != LWPS_NoError ) {
                        goto bailFromiLwpsSearchResponseReceive;
                    }

                    /* Read the length */
                    if ( (iError = iLwpsIntegerRead(plLwps, &psdiSpiDocumentItemsPtr->uiLength)) != LWPS_NoError ) {
                        goto bailFromiLwpsSearchResponseReceive;
                    }

                    /* Read the data */
                    if ( (iError = iLwpsDataRead(plLwps, (unsigned char **)&psdiSpiDocumentItemsPtr->pvData, NULL)) != LWPS_NoError ) {
                        goto bailFromiLwpsSearchResponseReceive;
                    }

                    /* Read the data length */
                    if ( (iError = iLwpsIntegerRead(plLwps, &psdiSpiDocumentItemsPtr->uiDataLength)) != LWPS_NoError ) {
                        goto bailFromiLwpsSearchResponseReceive;
                    }
                }
            }
        }
    }


    /* Read the total results */
    if ( (iError = iLwpsIntegerRead(plLwps, &uiTotalResults)) != LWPS_NoError ) {
        goto bailFromiLwpsSearchResponseReceive;
    }


    /* Read the start index */
    if ( (iError = iLwpsIntegerRead(plLwps, &uiStartIndex)) != LWPS_NoError ) {
        goto bailFromiLwpsSearchResponseReceive;
    }


    /* Read the end index */
    if ( (iError = iLwpsIntegerRead(plLwps, &uiEndIndex)) != LWPS_NoError ) {
        goto bailFromiLwpsSearchResponseReceive;
    }


    /* Read the max sort key */
    if ( (iError = iLwpsDoubleRead(plLwps, &dMaxSortKey)) != LWPS_NoError ) {
        goto bailFromiLwpsSearchResponseReceive;
    }


    /* Read the search time */
    if ( (iError = iLwpsDoubleRead(plLwps, &dSearchTime)) != LWPS_NoError ) {
        goto bailFromiLwpsSearchResponseReceive;
    }


    /* Read the reference ID */
    if ( (iError = iLwpsStringRead(plLwps, &pucReferenceID)) != LWPS_NoError ) {
        goto bailFromiLwpsSearchResponseReceive;
    }



    /* Bail label */
    bailFromiLwpsSearchResponseReceive:

    {
        struct spiSearchResponse *pssrSpiSearchResponse = NULL;

        /* Allocate space for the search response - screwy way to do it because we can't bail at this point but we can let the error flow */
        if ( iError == LWPS_NoError ) {
            if ( (pssrSpiSearchResponse = (struct spiSearchResponse *)s_malloc((size_t)(sizeof(struct spiSearchResponse)))) == NULL ) {
                iError = LWPS_MemError;
            }
        }

        /* Handle the error */
        if ( iError == LWPS_NoError ) {
        
            /* Set the fields in the search response */
            pssrSpiSearchResponse->pssrSpiSearchResults = pssrSpiSearchResults;
            pssrSpiSearchResponse->uiSpiSearchResultsLength = uiSpiSearchResultsLength;
            pssrSpiSearchResponse->uiTotalResults = uiTotalResults;
            pssrSpiSearchResponse->uiStartIndex = uiStartIndex;
            pssrSpiSearchResponse->uiEndIndex = uiEndIndex;
            pssrSpiSearchResponse->uiSortType = uiSortType;
            pssrSpiSearchResponse->dMaxSortKey = dMaxSortKey;
            pssrSpiSearchResponse->dSearchTime = dSearchTime;
        
            /* Set the return pointers */
            *ppssrSpiSearchResponse = pssrSpiSearchResponse;
            *ppucReferenceID = pucReferenceID;
        }
        else {
    
            /* Free allocations */
            iSpiFreeSearchResults(pssrSpiSearchResults, uiSpiSearchResultsLength, uiSortType);
            pssrSpiSearchResults = NULL;
    
            s_free(pucReferenceID);
        }
    }

    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLwpsSearchResponseHandle()

    Purpose:    Handle the search response by receiving an search request
                from a client and sending the response

    Parameters: pvLwps      lwps structure

    Globals:    none

    Returns:    LWPS error code

*/
int iLwpsSearchResponseHandle
(
    void *pvLwps
)
{

    int                         iError = LWPS_NoError;
    int                         iErrorCode = SPI_NoError;
    unsigned char               *pucErrorString = NULL;
    struct lwps                 *plLwps = (struct lwps *)pvLwps;
    unsigned char               **ppucIndexNameList = NULL;
    unsigned char               *pucLanguageCode = NULL;
    unsigned char               *pucSearchText = NULL;
    unsigned char               *pucPositiveFeedbackText = NULL;
    unsigned char               *pucNegativeFeedbackText = NULL;
    unsigned int                uiStartIndex = 0;
    unsigned int                uiEndIndex = 0;
    struct spiSearchResponse    *pssrSpiSearchResponse = NULL;
    unsigned char               *pucReferenceID = NULL;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iLwpsSearchResponseHandle");  */


    /* Check the parameters */
    if ( pvLwps == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvLwps' parameter passed to 'iLwpsSearchResponseHandle'."); 
        return (LWPS_InvalidLwps);
    }


    /* Receive the search request */
    if ( (iError = iLwpsSearchRequestReceive(plLwps, &ppucIndexNameList, &pucLanguageCode, &pucSearchText, &pucPositiveFeedbackText, &pucNegativeFeedbackText, 
            &uiStartIndex, &uiEndIndex, &pucReferenceID)) != LWPS_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to receive an lwps search request message, lwps error: %d.", iError);
        goto bailFromiLwpsSearchResponseHandle;
    }


    /* Put in your search code here */
/*     iErrorCode = mySearchCode(); */
    
    
    /* Handle the search error */
    if ( iErrorCode == SPI_NoError ) {

        /* Send the search response */
        if ( (iError = iLwpsSearchResponseSend(plLwps, pssrSpiSearchResponse, pucReferenceID)) != LWPS_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to send an lwps search response message, lwps error: %d.", iError);
            goto bailFromiLwpsSearchResponseHandle;
        }
    }
    else {

        /* Send the error */
        if ( (iError = iLwpsErrorMessageSend(plLwps, iErrorCode, pucErrorString, pucReferenceID)) != LWPS_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to send an lwps error message, lwps error: %d.", iError);
            goto bailFromiLwpsSearchResponseHandle;
        }
    }
    


    /* Bail label */
    bailFromiLwpsSearchResponseHandle:


    /* Free allocated pointers */
    UTL_MACROS_FREE_NULL_TERMINATED_LIST(ppucIndexNameList);

    s_free(pucSearchText);
    s_free(pucPositiveFeedbackText);
    s_free(pucNegativeFeedbackText);

    iSpiFreeSearchResponse(pssrSpiSearchResponse);
    pssrSpiSearchResponse = NULL;

    s_free(pucReferenceID);


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLwpsRetrievalRequestSend()

    Purpose:    Send a retrieval request to a server

    Parameters: pvLwps              lwps structure
                pucIndexName        index name (optional)
                pucDocumentKey      document key (optional)
                pucItemName         item name (optional)
                pucMimeType         mime type (optional)
                uiChunkType         document chunk type
                uiChunkStart        document chunk start
                uiChunkEnd          document chunk end
                pucReferenceID      reference ID (optional)

    Globals:    none

    Returns:    LWPS error code

*/
int iLwpsRetrievalRequestSend
(
    void *pvLwps,
    unsigned char *pucIndexName,
    unsigned char *pucDocumentKey,
    unsigned char *pucItemName,
    unsigned char *pucMimeType,
    unsigned int uiChunkType,
    unsigned int uiChunkStart,
    unsigned int uiChunkEnd,
    unsigned char *pucReferenceID
)
{

    int             iError = LWPS_NoError;
    struct lwps     *plLwps = (struct lwps *)pvLwps;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iLwpsRetrievalRequestSend");  */


    /* Check the parameters */
    if ( pvLwps == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvLwps' parameter passed to 'iLwpsRetrievalRequestSend'."); 
        return (LWPS_InvalidLwps);
    }

    if ( SPI_CHUNK_TYPE_VALID(uiChunkType) == false ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiChunkType' parameter passed to 'iLwpsRetrievalRequestSend'."); 
        return (LWPS_ParameterError);
    }

    if ( uiChunkStart < 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiChunkStart' parameter passed to 'iLwpsRetrievalRequestSend'."); 
        return (LWPS_ParameterError);
    }

    if ( uiChunkEnd < 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiChunkEnd' parameter passed to 'iLwpsRetrievalRequestSend'."); 
        return (LWPS_ParameterError);
    }

    if ( uiChunkStart > uiChunkEnd ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiChunkStart' & 'uiChunkEnd' parameters passed to 'iLwpsRetrievalRequestSend'."); 
        return (LWPS_ParameterError);
    }


    /* Send the message header */
    if ( (iError = iLwpsHeaderSend(plLwps, LWPS_RETRIEVAL_REQUEST_ID)) != LWPS_NoError ) {
        goto bailFromiLwpsRetrievalRequestSend;
    }


    /* Send the index name */
    if ( (iError = iLwpsStringWrite(plLwps, pucIndexName)) != LWPS_NoError ) {
        goto bailFromiLwpsRetrievalRequestSend;
    }


    /* Send the document key */
    if ( (iError = iLwpsStringWrite(plLwps, pucDocumentKey)) != LWPS_NoError ) {
        goto bailFromiLwpsRetrievalRequestSend;
    }


    /* Send the item name */
    if ( (iError = iLwpsStringWrite(plLwps, pucItemName)) != LWPS_NoError ) {
        goto bailFromiLwpsRetrievalRequestSend;
    }


    /* Send the mime type */
    if ( (iError = iLwpsStringWrite(plLwps, pucMimeType)) != LWPS_NoError ) {
        goto bailFromiLwpsRetrievalRequestSend;
    }


    /* Send the chunk type */
    if ( (iError = iLwpsIntegerWrite(plLwps, uiChunkType)) != LWPS_NoError ) {
        goto bailFromiLwpsRetrievalRequestSend;
    }


    /* Send the chunk start */
    if ( (iError = iLwpsIntegerWrite(plLwps, uiChunkStart)) != LWPS_NoError ) {
        goto bailFromiLwpsRetrievalRequestSend;
    }


    /* Send the chunk end */
    if ( (iError = iLwpsIntegerWrite(plLwps, uiChunkEnd)) != LWPS_NoError ) {
        goto bailFromiLwpsRetrievalRequestSend;
    }


    /* Send the reference ID string */
    if ( (iError = iLwpsStringWrite(plLwps, pucReferenceID)) != LWPS_NoError ) {
        goto bailFromiLwpsRetrievalRequestSend;
    }



    /* Bail label */
    bailFromiLwpsRetrievalRequestSend:


    /* Handle the error */
    if ( iError == LWPS_NoError ) {
    
        /* Send the data */
        if ( (iError = iUtlNetSend(plLwps->pvUtlNet)) != UTL_NoError ) {
            iError = iLwpsMapUtlNetError(iError, LWPS_FailedWriteData);
        }
    }
#if defined(LWPS_ENABLE_CLEAR_SEND_BUFFER_ON_SEND_ERROR)
    else {

        /* Reset the send buffer to clear it */
        if ( (iError = iUtlNetResetSend(plLwps->pvUtlNet)) != UTL_NoError ) {
            iError = iLwpsMapUtlNetError(iError, LWPS_FailedWriteData);
        }
    }
#endif    /* defined(LWPS_ENABLE_CLEAR_SEND_BUFFER_ON_SEND_ERROR) */


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLwpsRetrievalRequestReceive()

    Purpose:    Receive a retrieval request from a client, this function assumes
                that the header has already been read

    Parameters: pvLwps              lwps structure
                ppucIndexName       return pointer for the index name (allocated)
                ppucDocumentKey     return pointer for the document key (allocated)
                ppucItemName        return pointer for the item name (allocated)
                ppucMimeType        return pointer for the mime type (allocated)
                puiChunkType        return pointer for the document chunk type
                puiChunkStart       return pointer for the document chunk start
                puiChunkEnd         return pointer for the document chunk end
                ppucReferenceID     return pointer for the reference ID (allocated)

    Globals:    none

    Returns:    LWPS error code

*/
int iLwpsRetrievalRequestReceive
(
    void *pvLwps,
    unsigned char **ppucIndexName,
    unsigned char **ppucDocumentKey,
    unsigned char **ppucItemName,
    unsigned char **ppucMimeType,
    unsigned int *puiChunkType,
    unsigned int *puiChunkStart,
    unsigned int *puiChunkEnd,
    unsigned char **ppucReferenceID
)
{

    int             iError = LWPS_NoError;
    struct lwps     *plLwps = (struct lwps *)pvLwps;
    unsigned int    uiMessageID = LWPS_INVALID_ID;
    unsigned char   *pucIndexName = NULL;
    unsigned char   *pucDocumentKey = NULL;
    unsigned char   *pucItemName = NULL;
    unsigned char   *pucMimeType = NULL;
    unsigned int    uiChunkType = 0;
    unsigned int    uiChunkStart = 0;
    unsigned int    uiChunkEnd = 0;
    unsigned char   *pucReferenceID = NULL;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iLwpsRetrievalRequestReceive");  */


    /* Check the parameters */
    if ( pvLwps == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvLwps' parameter passed to 'iLwpsRetrievalRequestReceive'."); 
        return (LWPS_InvalidLwps);
    }

    if ( ppucIndexName == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppucIndexName' parameter passed to 'iLwpsRetrievalRequestReceive'."); 
        return (LWPS_ReturnParameterError);
    }

    if ( ppucDocumentKey == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppucDocumentKey' parameter passed to 'iLwpsRetrievalRequestReceive'."); 
        return (LWPS_ReturnParameterError);
    }

    if ( ppucItemName == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppucItemName' parameter passed to 'iLwpsRetrievalRequestReceive'."); 
        return (LWPS_ReturnParameterError);
    }

    if ( ppucMimeType == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppucMimeType' parameter passed to 'iLwpsRetrievalRequestReceive'."); 
        return (LWPS_ReturnParameterError);
    }

    if ( puiChunkType == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'puiChunkType' parameter passed to 'iLwpsRetrievalRequestReceive'."); 
        return (LWPS_ReturnParameterError);
    }

    if ( puiChunkStart == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'puiChunkStart' parameter passed to 'iLwpsRetrievalRequestReceive'."); 
        return (LWPS_ReturnParameterError);
    }

    if ( puiChunkEnd == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'puiChunkEnd' parameter passed to 'iLwpsRetrievalRequestReceive'."); 
        return (LWPS_ReturnParameterError);
    }

    if ( ppucReferenceID == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppucReferenceID' parameter passed to 'iLwpsRetrievalRequestReceive'."); 
        return (LWPS_ReturnParameterError);
    }


    /* Peek the response header, we do this here because we don't want 
    ** to harm the response if it is not what we expected
    */
    if ( (iError = iLwpsHeaderPeek(plLwps, &uiMessageID)) != LWPS_NoError ) {
        goto bailFromiLwpsRetrievalRequestReceive;
    }


    /* Check that the response header message tag is correct */
    if ( uiMessageID != LWPS_RETRIEVAL_REQUEST_ID) {
        iError = LWPS_InvalidMessageID;
        goto bailFromiLwpsRetrievalRequestReceive;
    }


    /* Read the response header */
    if ( (iError = iLwpsHeaderReceive(plLwps, &uiMessageID)) != LWPS_NoError ) {
        goto bailFromiLwpsRetrievalRequestReceive;
    }


    /* Read the index name */
    if ( (iError = iLwpsStringRead(plLwps, &pucIndexName)) != LWPS_NoError ) {
        goto bailFromiLwpsRetrievalRequestReceive;
    }


    /* Read the document key */
    if ( (iError = iLwpsStringRead(plLwps, &pucDocumentKey)) != LWPS_NoError ) {
        goto bailFromiLwpsRetrievalRequestReceive;
    }


    /* Read the item name */
    if ( (iError = iLwpsStringRead(plLwps, &pucItemName)) != LWPS_NoError ) {
        goto bailFromiLwpsRetrievalRequestReceive;
    }


    /* Read the mime type */
    if ( (iError = iLwpsStringRead(plLwps, &pucMimeType)) != LWPS_NoError ) {
        goto bailFromiLwpsRetrievalRequestReceive;
    }


    /* Read the chunk type */
    if ( (iError = iLwpsIntegerRead(plLwps, &uiChunkType)) != LWPS_NoError ) {
        goto bailFromiLwpsRetrievalRequestReceive;
    }


    /* Read the chunk start */
    if ( (iError = iLwpsIntegerRead(plLwps, &uiChunkStart)) != LWPS_NoError ) {
        goto bailFromiLwpsRetrievalRequestReceive;
    }


    /* Read the chunk end */
    if ( (iError = iLwpsIntegerRead(plLwps, &uiChunkEnd)) != LWPS_NoError ) {
        goto bailFromiLwpsRetrievalRequestReceive;
    }


    /* Read the reference ID */
    if ( (iError = iLwpsStringRead(plLwps, &pucReferenceID)) != LWPS_NoError ) {
        goto bailFromiLwpsRetrievalRequestReceive;
    }



    /* Bail label */
    bailFromiLwpsRetrievalRequestReceive:


    /* Handle the error */
    if ( iError == LWPS_NoError ) {

        /* Set the return pointers */
        *ppucIndexName = pucIndexName;
        *ppucDocumentKey = pucDocumentKey;
        *ppucItemName = pucItemName;
        *ppucMimeType = pucMimeType;
        *puiChunkType = uiChunkType;
        *puiChunkStart = uiChunkStart;
        *puiChunkEnd = uiChunkEnd;
        *ppucReferenceID = pucReferenceID;
    }
    else {

        /* Free allocations */
        s_free(pucIndexName);
        s_free(pucDocumentKey);
        s_free(pucItemName);
        s_free(pucMimeType);
        s_free(pucReferenceID);
    }


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLwpsRetrievalRequestHandle()

    Purpose:    Handle a retrieval by sending a retrieval request
                to a server and receiving the response

    Parameters: pvLwps              lwps structure
                pucIndexName        index name (optional)
                pucDocumentKey      document key (optional)
                pucItemName         item name (optional)
                pucMimeType         mime type (optional)
                uiChunkType         document chunk type
                uiChunkStart        document chunk start
                uiChunkEnd          document chunk end
                ppvData             return pointer for the data pointer (allocated)
                puiDataLength       return pointer for the data pointer length
                piErrorCode         return pointer for the error code
                ppucErrorString     return pointer for the error string (allocated)

    Globals:    none

    Returns:    LWPS error code

*/
int iLwpsRetrievalRequestHandle
(
    void *pvLwps,
    unsigned char *pucIndexName,
    unsigned char *pucDocumentKey,
    unsigned char *pucItemName,
    unsigned char *pucMimeType,
    unsigned int uiChunkType,
    unsigned int uiChunkStart,
    unsigned int uiChunkEnd,
    void **ppvData,
    unsigned int *puiDataLength,
    int *piErrorCode,
    unsigned char **ppucErrorString
)
{

    int             iError = LWPS_NoError;
    struct lwps     *plLwps = (struct lwps *)pvLwps;
    unsigned int    uiMessageID = LWPS_INVALID_ID;
    unsigned char   pucSentReferenceID[LWPS_REFERENCE_ID_BUFFER_LENGTH + 1] = {'\0'};
    unsigned char   *pucReturnedReferenceID = NULL;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iLwpsRetrievalRequestHandle");  */


    /* Check the parameters */
    if ( pvLwps == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvLwps' parameter passed to 'iLwpsRetrievalRequestHandle'."); 
        return (LWPS_InvalidLwps);
    }

    if ( SPI_CHUNK_TYPE_VALID(uiChunkType) == false ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiChunkType' parameter passed to 'iLwpsRetrievalRequestHandle'."); 
        return (LWPS_ParameterError);
    }

    if ( uiChunkStart < 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiChunkStart' parameter passed to 'iLwpsRetrievalRequestHandle'."); 
        return (LWPS_ParameterError);
    }

    if ( uiChunkEnd < 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiChunkEnd' parameter passed to 'iLwpsRetrievalRequestHandle'."); 
        return (LWPS_ParameterError);
    }

    if ( uiChunkStart > uiChunkEnd ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiChunkStart' & 'uiChunkEnd' parameters passed to 'iLwpsRetrievalRequestHandle'."); 
        return (LWPS_ParameterError);
    }

    if ( ppvData == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppvData' parameter passed to 'iLwpsRetrievalRequestHandle'."); 
        return (LWPS_ReturnParameterError);
    }

    if ( puiDataLength == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'puiDataLength' parameter passed to 'iLwpsRetrievalRequestHandle'."); 
        return (LWPS_ReturnParameterError);
    }

    if ( piErrorCode == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'piErrorCode' parameter passed to 'iLwpsRetrievalRequestHandle'."); 
        return (LWPS_ReturnParameterError);
    }

    if ( ppucErrorString == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppucErrorString' parameter passed to 'iLwpsRetrievalRequestHandle'."); 
        return (LWPS_ReturnParameterError);
    }


    /* Create a reference ID */
    snprintf(pucSentReferenceID, LWPS_REFERENCE_ID_BUFFER_LENGTH + 1, "%u", plLwps->uiReferenceID);

    
    /* Increment the reference ID */
    plLwps->uiReferenceID++;


    /* Clear the returned error code */
    *piErrorCode = SPI_NoError;
    

    /* Send the retrieval request */
    if ( (iError = iLwpsRetrievalRequestSend(plLwps, pucIndexName, pucDocumentKey, pucItemName, pucMimeType, 
            uiChunkType, uiChunkStart, uiChunkEnd, pucSentReferenceID)) != LWPS_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to send an lwps retrieval request message, lwps error: %d.", iError);
        goto bailFromiLwpsRetrievalRequestHandle;
    }


    /* Receive the data */
    if ( (iError = iUtlNetReceive(plLwps->pvUtlNet)) != UTL_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to receive data, utl error: %d.", iError);
        iError = iLwpsMapUtlNetError(iError, LWPS_FailedReadData);
        goto bailFromiLwpsRetrievalRequestHandle;
    }


    /* Peek the header */
    if ( (iError = iLwpsHeaderPeek(pvLwps, &uiMessageID)) != LWPS_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to peek the lwps header, lwps error: %d.", iError);
        goto bailFromiLwpsRetrievalRequestHandle;
    }


    /* Handle the message */
    if ( uiMessageID == LWPS_RETRIEVAL_RESPONSE_ID ) {

        /* Receive the retrieval response */
        if ( (iError = iLwpsRetrievalResponseReceive(plLwps, ppvData, puiDataLength, &pucReturnedReferenceID)) != LWPS_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to receive an lwps retrieval response message, lwps error: %d.", iError);
            goto bailFromiLwpsRetrievalRequestHandle;
        }
    }
    else if ( uiMessageID == LWPS_ERROR_MESSAGE_ID ) {

        /* Receive the error message */
        if ( (iError = iLwpsErrorMessageReceive(plLwps, piErrorCode, ppucErrorString, &pucReturnedReferenceID)) != LWPS_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to receive an lwps error message, lwps error: %d.", iError);
            goto bailFromiLwpsRetrievalRequestHandle;
        }
    }
    else {
        iError = LWPS_InvalidMessageID;
        goto bailFromiLwpsRetrievalRequestHandle;
    }


    /* Check that the reference ID was correctly returned */
    if ( !((bUtlStringsIsStringNULL(pucReturnedReferenceID) == false) && (s_strcmp(pucReturnedReferenceID, pucSentReferenceID) == 0)) ) {
        iError = LWPS_InvalidReferenceID;
        goto bailFromiLwpsRetrievalRequestHandle;
    }



    /* Bail label */
    bailFromiLwpsRetrievalRequestHandle:


    /* Free allocated pointers */
    s_free(pucReturnedReferenceID);


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLwpsRetrievalResponseSend()

    Purpose:    Send a retrieval response to a client

    Parameters: pvLwps              lwps structure
                pvData              data pointer (optional)
                uiDataLength        data pointer length
                pucReferenceID      reference ID (optional)

    Globals:    none

    Returns:    LWPS error code

*/
int iLwpsRetrievalResponseSend
(
    void *pvLwps,
    void *pvData,
    unsigned int uiDataLength,
    unsigned char *pucReferenceID
)
{

    int             iError = LWPS_NoError;
    struct lwps     *plLwps = (struct lwps *)pvLwps;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iLwpsRetrievalResponseSend");  */


    /* Check the parameters */
    if ( pvLwps == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvLwps' parameter passed to 'iLwpsRetrievalResponseSend'."); 
        return (LWPS_InvalidLwps);
    }

    if ( uiDataLength < 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiDataLength' parameter passed to 'iLwpsRetrievalResponseSend'."); 
        return (LWPS_ParameterError);
    }

    if ( (pvData == NULL) && (uiDataLength > 0) ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'pvData' & 'uiDataLength' parameters passed to 'iLwpsRetrievalResponseSend'."); 
        return (LWPS_ParameterError);
    }


    /* Send the reply header */
    if ( (iError = iLwpsHeaderSend(plLwps, LWPS_RETRIEVAL_RESPONSE_ID)) != LWPS_NoError ) {
        goto bailFromiLwpsRetrievalResponseSend;
    }


    /* Send the data length */
    if ( (iError = iLwpsIntegerWrite(plLwps, uiDataLength)) != LWPS_NoError ) {
        goto bailFromiLwpsRetrievalResponseSend;
    }


    /* Send the data */
    if ( (iError = iLwpsDataWrite(plLwps, pvData, uiDataLength)) != LWPS_NoError ) {
        goto bailFromiLwpsRetrievalResponseSend;
    }


    /* Send the reference ID string */
    if ( (iError = iLwpsStringWrite(plLwps, pucReferenceID)) != LWPS_NoError ) {
        goto bailFromiLwpsRetrievalResponseSend;
    }



    /* Bail label */
    bailFromiLwpsRetrievalResponseSend:


    /* Handle the error */
    if ( iError == LWPS_NoError ) {
    
        /* Send the data */
        if ( (iError = iUtlNetSend(plLwps->pvUtlNet)) != UTL_NoError ) {
            iError = iLwpsMapUtlNetError(iError, LWPS_FailedWriteData);
        }
    }
#if defined(LWPS_ENABLE_CLEAR_SEND_BUFFER_ON_SEND_ERROR)
    else {

        /* Reset the send buffer to clear it */
        if ( (iError = iUtlNetResetSend(plLwps->pvUtlNet)) != UTL_NoError ) {
            iError = iLwpsMapUtlNetError(iError, LWPS_FailedWriteData);
        }
    }
#endif    /* defined(LWPS_ENABLE_CLEAR_SEND_BUFFER_ON_SEND_ERROR) */


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLwpsRetrievalResponseReceive()

    Purpose:    Receive a retrieval response from a server

    Parameters: pvLwps              lwps structure
                ppvData             return pointer for the data pointer (allocated)
                puiDataLength       return pointer for the data pointer length
                ppucReferenceID     return pointer for the reference ID (allocated)

    Globals:    none

    Returns:    LWPS error code

*/
int iLwpsRetrievalResponseReceive
(
    void *pvLwps,
    void **ppvData,
    unsigned int *puiDataLength,
    unsigned char **ppucReferenceID
)
{

    int             iError = LWPS_NoError;
    struct lwps     *plLwps = (struct lwps *)pvLwps;
    unsigned int    uiMessageID = LWPS_INVALID_ID;
    void            *pvData = NULL;
    unsigned int    uiDataLength = 0;
    unsigned char   *pucReferenceID = NULL;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iLwpsRetrievalResponseReceive");  */


    /* Check the parameters */
    if ( pvLwps == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvLwps' parameter passed to 'iLwpsRetrievalResponseReceive'."); 
        return (LWPS_InvalidLwps);
    }

    if ( ppvData == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppvData' parameter passed to 'iLwpsRetrievalResponseReceive'."); 
        return (LWPS_ReturnParameterError);
    }

    if ( puiDataLength == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'puiDataLength' parameter passed to 'iLwpsRetrievalResponseReceive'."); 
        return (LWPS_ReturnParameterError);
    }

    if ( ppucReferenceID == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppucReferenceID' parameter passed to 'iLwpsRetrievalResponseReceive'."); 
        return (LWPS_ReturnParameterError);
    }


    /* Peek the response header, we do this here because we don't want 
    ** to harm the response if it is not what we expected
    */
    if ( (iError = iLwpsHeaderPeek(plLwps, &uiMessageID)) != LWPS_NoError ) {
        goto bailFromiLwpsRetrievalResponseReceive;
    }


    /* Check that the message header message tag is correct */
    if ( uiMessageID != LWPS_RETRIEVAL_RESPONSE_ID) {
        iError = LWPS_InvalidMessageID;
        goto bailFromiLwpsRetrievalResponseReceive;
    }


    /* Read the response header */
    if ( (iError = iLwpsHeaderReceive(plLwps, &uiMessageID)) != LWPS_NoError ) {
        goto bailFromiLwpsRetrievalResponseReceive;
    }


    /* Read the data length */
    if ( (iError = iLwpsIntegerRead(plLwps, &uiDataLength)) != LWPS_NoError ) {
        goto bailFromiLwpsRetrievalResponseReceive;
    }


    /* Read the data */
    if ( (iError = iLwpsDataRead(plLwps, (unsigned char **)&pvData, NULL)) != LWPS_NoError ) {
        goto bailFromiLwpsRetrievalResponseReceive;
    }


    /* Read the reference ID */
    if ( (iError = iLwpsStringRead(plLwps, &pucReferenceID)) != LWPS_NoError ) {
        goto bailFromiLwpsRetrievalResponseReceive;
    }



    /* Bail label */
    bailFromiLwpsRetrievalResponseReceive:


    /* Handle the error */
    if ( iError == LWPS_NoError ) {

        /* Set the return pointers */
        *puiDataLength = uiDataLength;
        *ppvData = pvData;
        *ppucReferenceID = pucReferenceID;
    }
    else {

        /* Free allocations */
        s_free(pvData);
        s_free(pucReferenceID);
    }


    return (iError);


}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLwpsRetrievalResponseHandle()

    Purpose:    Handle the retrieval response by receiving an retrieval request
                from a client and sending the response

    Parameters: pvLwps      lwps structure

    Globals:    none

    Returns:    LWPS error code

*/
int iLwpsRetrievalResponseHandle
(
    void *pvLwps
)
{

    int             iError = LWPS_NoError;
    int             iErrorCode = SPI_NoError;
    unsigned char   *pucErrorString = NULL;
    struct lwps     *plLwps = (struct lwps *)pvLwps;
    unsigned char   *pucIndexName = NULL;
    unsigned char   *pucDocumentKey = NULL;
    unsigned char   *pucItemName = NULL;
    unsigned char   *pucMimeType = NULL;
    unsigned int    uiChunkType = 0;
    unsigned int    uiChunkStart = 0;
    unsigned int    uiChunkEnd = 0;
    void            *pvData = NULL;
    unsigned int    uiDataLength = 0;
    unsigned char   *pucReferenceID = NULL;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iLwpsRetrievalResponseHandle");  */


    /* Check the parameters */
    if ( pvLwps == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvLwps' parameter passed to 'iLwpsRetrievalResponseHandle'."); 
        return (LWPS_InvalidLwps);
    }


    /* Receive the retrieval request */
    if ( (iError = iLwpsRetrievalRequestReceive(plLwps, &pucIndexName, &pucDocumentKey, &pucItemName, &pucMimeType, 
            &uiChunkType, &uiChunkStart, &uiChunkEnd, &pucReferenceID)) != LWPS_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to receive an lwps retrieval request message, lwps error: %d.", iError);
        goto bailFromiLwpsRetrievalResponseHandle;
    }


    /* Put in your retrieval code here */
/*     iErrorCode = myRetrievalCode(); */
    
    
    /* Handle the retrieval error */
    if ( iErrorCode == SPI_NoError ) {

        /* Send the retrieval response */
        if ( (iError = iLwpsRetrievalResponseSend(plLwps, pvData, uiDataLength, pucReferenceID)) != LWPS_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to send an lwps retrieval response message, lwps error: %d.", iError);
            goto bailFromiLwpsRetrievalResponseHandle;
        }
    }
    else {

        /* Send the error */
        if ( (iError = iLwpsErrorMessageSend(plLwps, iErrorCode, pucErrorString, pucReferenceID)) != LWPS_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to send an lwps error message, lwps error: %d.", iError);
            goto bailFromiLwpsRetrievalResponseHandle;
        }
    }



    /* Bail label */
    bailFromiLwpsRetrievalResponseHandle:


    /* Free allocated pointers */
    s_free(pucIndexName);
    s_free(pucDocumentKey);
    s_free(pucItemName);
    s_free(pucMimeType);
    s_free(pvData);
    s_free(pucReferenceID);


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLwpsServerInfoRequestSend()

    Purpose:    Send a server info request to a server

    Parameters: pvLwps              lwps structure
                pucReferenceID      reference ID (optional)

    Globals:    none

    Returns:    LWPS error code

*/
int iLwpsServerInfoRequestSend
(
    void *pvLwps,
    unsigned char *pucReferenceID
)
{

    int             iError = LWPS_NoError;
    struct lwps     *plLwps = (struct lwps *)pvLwps;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iLwpsServerInfoRequestSend");  */


    /* Check the parameters */
    if ( pvLwps == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvLwps' parameter passed to 'iLwpsServerInfoRequestSend'."); 
        return (LWPS_InvalidLwps);
    }


    /* Send the message header */
    if ( (iError = iLwpsHeaderSend(plLwps, LWPS_SERVER_INFO_REQUEST_ID)) != LWPS_NoError ) {
        goto bailFromiLwpsServerInfoRequestSend;
    }


    /* Send the reference ID string */
    if ( (iError = iLwpsStringWrite(plLwps, pucReferenceID)) != LWPS_NoError ) {
        goto bailFromiLwpsServerInfoRequestSend;
    }



    /* Bail label */
    bailFromiLwpsServerInfoRequestSend:


    /* Handle the error */
    if ( iError == LWPS_NoError ) {
    
        /* Send the data */
        if ( (iError = iUtlNetSend(plLwps->pvUtlNet)) != UTL_NoError ) {
            iError = iLwpsMapUtlNetError(iError, LWPS_FailedWriteData);
        }
    }
#if defined(LWPS_ENABLE_CLEAR_SEND_BUFFER_ON_SEND_ERROR)
    else {

        /* Reset the send buffer to clear it */
        if ( (iError = iUtlNetResetSend(plLwps->pvUtlNet)) != UTL_NoError ) {
            iError = iLwpsMapUtlNetError(iError, LWPS_FailedWriteData);
        }
    }
#endif    /* defined(LWPS_ENABLE_CLEAR_SEND_BUFFER_ON_SEND_ERROR) */


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLwpsServerInfoRequestReceive()

    Purpose:    Receive a server info request from a client, this function assumes
                that the header has already been read

    Parameters: pvLwps              lwps structure
                ppucReferenceID     return pointer for the reference ID (allocated)

    Globals:    none

    Returns:    LWPS error code

*/
int iLwpsServerInfoRequestReceive
(
    void *pvLwps,
    unsigned char **ppucReferenceID
)
{

    int             iError = LWPS_NoError;
    struct lwps     *plLwps = (struct lwps *)pvLwps;
    unsigned int    uiMessageID = LWPS_INVALID_ID;
    unsigned char   *pucReferenceID = NULL;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iLwpsServerInfoRequestReceive");  */


    /* Check the parameters */
    if ( pvLwps == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvLwps' parameter passed to 'iLwpsServerInfoRequestReceive'."); 
        return (LWPS_InvalidLwps);
    }

    if ( ppucReferenceID == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppucReferenceID' parameter passed to 'iLwpsServerInfoRequestReceive'."); 
        return (LWPS_ReturnParameterError);
    }


    /* Peek the response header, we do this here because we don't want 
    ** to harm the response if it is not what we expected
    */
    if ( (iError = iLwpsHeaderPeek(plLwps, &uiMessageID)) != LWPS_NoError ) {
        goto bailFromiLwpsServerInfoRequestReceive;
    }


    /* Check that the response header message tag is correct */
    if ( uiMessageID != LWPS_SERVER_INFO_REQUEST_ID) {
        iError = LWPS_InvalidMessageID;
        goto bailFromiLwpsServerInfoRequestReceive;
    }


    /* Read the response header */
    if ( (iError = iLwpsHeaderReceive(plLwps, &uiMessageID)) != LWPS_NoError ) {
        goto bailFromiLwpsServerInfoRequestReceive;
    }


    /* Read the reference ID */
    if ( (iError = iLwpsStringRead(plLwps, &pucReferenceID)) != LWPS_NoError ) {
        goto bailFromiLwpsServerInfoRequestReceive;
    }



    /* Bail label */
    bailFromiLwpsServerInfoRequestReceive:


    /* Handle the error */
    if ( iError == LWPS_NoError ) {

        /* Set the return pointers */
        *ppucReferenceID = pucReferenceID;
    }    
    else {

        /* Free allocations */
        s_free(pucReferenceID);
    }


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLwpsServerInfoRequestHandle()

    Purpose:    Handle a server info by sending a server info request
                to a server and receiving the response

    Parameters: pvLwps                  lwps structure
                ppssiSpiServerInfo      return pointer for the server info structure (allocated)
                piErrorCode             return pointer for the error code
                ppucErrorString         return pointer for the error string (allocated)

    Globals:    none

    Returns:    LWPS error code

*/
int iLwpsServerInfoRequestHandle
(
    void *pvLwps,
    struct spiServerInfo **ppssiSpiServerInfo,
    int *piErrorCode,
    unsigned char **ppucErrorString
)
{

    int             iError = LWPS_NoError;
    struct lwps     *plLwps = (struct lwps *)pvLwps;
    unsigned int    uiMessageID = LWPS_INVALID_ID;
    unsigned char   pucSentReferenceID[LWPS_REFERENCE_ID_BUFFER_LENGTH + 1] = {'\0'};
    unsigned char   *pucReturnedReferenceID = NULL;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iLwpsServerInfoRequestHandle");  */


    /* Check the parameters */
    if ( pvLwps == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvLwps' parameter passed to 'iLwpsServerInfoRequestHandle'."); 
        return (LWPS_InvalidLwps);
    }

    if ( ppssiSpiServerInfo == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppssiSpiServerInfo' parameter passed to 'iLwpsServerInfoRequestHandle'."); 
        return (LWPS_ReturnParameterError);
    }

    if ( piErrorCode == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'piErrorCode' parameter passed to 'iLwpsServerInfoRequestHandle'."); 
        return (LWPS_ReturnParameterError);
    }

    if ( ppucErrorString == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppucErrorString' parameter passed to 'iLwpsServerInfoRequestHandle'."); 
        return (LWPS_ReturnParameterError);
    }


    /* Create a reference ID */
    snprintf(pucSentReferenceID, LWPS_REFERENCE_ID_BUFFER_LENGTH + 1, "%u", plLwps->uiReferenceID);

    
    /* Increment the reference ID */
    plLwps->uiReferenceID++;


    /* Clear the returned error code */
    *piErrorCode = SPI_NoError;
    

    /* Send the server info request */
    if ( (iError = iLwpsServerInfoRequestSend(plLwps, pucSentReferenceID)) != LWPS_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to send an lwps server information request message, lwps error: %d.", iError);
        goto bailFromiLwpsServerInfoRequestHandle;
    }


    /* Receive the data */
    if ( (iError = iUtlNetReceive(plLwps->pvUtlNet)) != UTL_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to receive data, utl error: %d.", iError);
        iError = iLwpsMapUtlNetError(iError, LWPS_FailedReadData);
        goto bailFromiLwpsServerInfoRequestHandle;
    }


    /* Peek the header */
    if ( (iError = iLwpsHeaderPeek(pvLwps, &uiMessageID)) != LWPS_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to peek the lwps header, lwps error: %d.", iError);
        goto bailFromiLwpsServerInfoRequestHandle;
    }


    /* Handle the message */
    if ( uiMessageID == LWPS_SERVER_INFO_RESPONSE_ID ) {

        /* Receive the server info response */
        if ( (iError = iLwpsServerInfoResponseReceive(plLwps, ppssiSpiServerInfo, &pucReturnedReferenceID)) != LWPS_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to receive an lwps server information response message, lwps error: %d.", iError);
            goto bailFromiLwpsServerInfoRequestHandle;
        }
    }
    else if ( uiMessageID == LWPS_ERROR_MESSAGE_ID ) {

        /* Receive the error message */
        if ( (iError = iLwpsErrorMessageReceive(plLwps, piErrorCode, ppucErrorString, &pucReturnedReferenceID)) != LWPS_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to receive an lwps error message, lwps error: %d.", iError);
            goto bailFromiLwpsServerInfoRequestHandle;
        }
    }
    else {
        iError = LWPS_InvalidMessageID;
        goto bailFromiLwpsServerInfoRequestHandle;
    }


    /* Check that the reference ID was correctly returned */
    if ( !((bUtlStringsIsStringNULL(pucReturnedReferenceID) == false) && (s_strcmp(pucReturnedReferenceID, pucSentReferenceID) == 0)) ) {
        iError = LWPS_InvalidReferenceID;
        goto bailFromiLwpsServerInfoRequestHandle;
    }



    /* Bail label */
    bailFromiLwpsServerInfoRequestHandle:


    /* Free allocated pointers */
    s_free(pucReturnedReferenceID);


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLwpsServerInfoResponseSend()

    Purpose:    Send a server info response to a client

    Parameters: pvLwps              lwps structure
                pssiSpiServerInfo   spi server info structure
                pucReferenceID      reference ID (optional)

    Globals:    none

    Returns:    LWPS error code

*/
int iLwpsServerInfoResponseSend
(
    void *pvLwps,
    struct spiServerInfo *pssiSpiServerInfo,
    unsigned char *pucReferenceID
)
{

    int             iError = LWPS_NoError;
    struct lwps     *plLwps = (struct lwps *)pvLwps;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iLwpsServerInfoResponseSend");  */


    /* Check the parameters */
    if ( pvLwps == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvLwps' parameter passed to 'iLwpsServerInfoResponseSend'."); 
        return (LWPS_InvalidLwps);
    }

    if ( pssiSpiServerInfo == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pssiSpiServerInfo' parameter passed to 'iLwpsServerInfoResponseSend'."); 
        return (LWPS_ParameterError);
    }


    /* Send the reply header */
    if ( (iError = iLwpsHeaderSend(plLwps, LWPS_SERVER_INFO_RESPONSE_ID)) != LWPS_NoError ) {
        goto bailFromiLwpsServerInfoResponseSend;
    }


    /* Send the name */
    if ( (iError = iLwpsStringWrite(plLwps, pssiSpiServerInfo->pucName)) != LWPS_NoError ) {
        goto bailFromiLwpsServerInfoResponseSend;
    }


    /* Send the description */
    if ( (iError = iLwpsStringWrite(plLwps, pssiSpiServerInfo->pucDescription)) != LWPS_NoError ) {
        goto bailFromiLwpsServerInfoResponseSend;
    }


    /* Send the admin name */
    if ( (iError = iLwpsStringWrite(plLwps, pssiSpiServerInfo->pucAdminName)) != LWPS_NoError ) {
        goto bailFromiLwpsServerInfoResponseSend;
    }


    /* Send the admin email */
    if ( (iError = iLwpsStringWrite(plLwps, pssiSpiServerInfo->pucAdminEmail)) != LWPS_NoError ) {
        goto bailFromiLwpsServerInfoResponseSend;
    }


    /* Send the index count */
    if ( (iError = iLwpsIntegerWrite(plLwps, pssiSpiServerInfo->uiIndexCount)) != LWPS_NoError ) {
        goto bailFromiLwpsServerInfoResponseSend;
    }


    /* Send the ranking algorithm */
    if ( (iError = iLwpsStringWrite(plLwps, pssiSpiServerInfo->pucRankingAlgorithm)) != LWPS_NoError ) {
        goto bailFromiLwpsServerInfoResponseSend;
    }


    /* Send the min weight */
    if ( (iError = iLwpsDoubleWrite(plLwps, pssiSpiServerInfo->dWeightMinimum)) != LWPS_NoError ) {
        goto bailFromiLwpsServerInfoResponseSend;
    }


    /* Send the max weight */
    if ( (iError = iLwpsDoubleWrite(plLwps, pssiSpiServerInfo->dWeightMaximum)) != LWPS_NoError ) {
        goto bailFromiLwpsServerInfoResponseSend;
    }


    /* Send the reference ID string */
    if ( (iError = iLwpsStringWrite(plLwps, pucReferenceID)) != LWPS_NoError ) {
        goto bailFromiLwpsServerInfoResponseSend;
    }



    /* Bail label */
    bailFromiLwpsServerInfoResponseSend:


    /* Handle the error */
    if ( iError == LWPS_NoError ) {
    
        /* Send the data */
        if ( (iError = iUtlNetSend(plLwps->pvUtlNet)) != UTL_NoError ) {
            iError = iLwpsMapUtlNetError(iError, LWPS_FailedWriteData);
        }
    }
#if defined(LWPS_ENABLE_CLEAR_SEND_BUFFER_ON_SEND_ERROR)
    else {

        /* Reset the send buffer to clear it */
        if ( (iError = iUtlNetResetSend(plLwps->pvUtlNet)) != UTL_NoError ) {
            iError = iLwpsMapUtlNetError(iError, LWPS_FailedWriteData);
        }
    }
#endif    /* defined(LWPS_ENABLE_CLEAR_SEND_BUFFER_ON_SEND_ERROR) */


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLwpsServerInfoResponseReceive()

    Purpose:    Receive a server info response from a server

    Parameters: pvLwps                  lwps structure
                ppssiSpiServerInfo      return pointer for the spi server info structure (allocated)
                ppucReferenceID         return pointer for the reference ID (allocated)

    Globals:    none

    Returns:    LWPS error code

*/
int iLwpsServerInfoResponseReceive
(
    void *pvLwps,
    struct spiServerInfo **ppssiSpiServerInfo,
    unsigned char **ppucReferenceID
)
{

    int                     iError = LWPS_NoError;
    struct lwps             *plLwps = (struct lwps *)pvLwps;
    unsigned int            uiMessageID = LWPS_INVALID_ID;
    struct spiServerInfo    *pssiSpiServerInfo = NULL;
    unsigned char           *pucReferenceID = NULL;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iLwpsServerInfoResponseReceive");  */


    /* Check the parameters */
    if ( pvLwps == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvLwps' parameter passed to 'iLwpsServerInfoResponseReceive'."); 
        return (LWPS_InvalidLwps);
    }

    if ( ppssiSpiServerInfo == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppssiSpiServerInfo' parameter passed to 'iLwpsServerInfoResponseReceive'."); 
        return (LWPS_ReturnParameterError);
    }

    if ( ppucReferenceID == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppucReferenceID' parameter passed to 'iLwpsServerInfoResponseReceive'."); 
        return (LWPS_ReturnParameterError);
    }


    /* Peek the response header, we do this here because we don't want 
    ** to harm the response if it is not what we expected
    */
    if ( (iError = iLwpsHeaderPeek(plLwps, &uiMessageID)) != LWPS_NoError ) {
        goto bailFromiLwpsServerInfoResponseReceive;
    }


    /* Check that the response header message tag is correct */
    if ( uiMessageID != LWPS_SERVER_INFO_RESPONSE_ID) {
        iError = LWPS_InvalidMessageID;
        goto bailFromiLwpsServerInfoResponseReceive;
    }


    /* Read the response header */
    if ( (iError = iLwpsHeaderReceive(plLwps, &uiMessageID)) != LWPS_NoError ) {
        goto bailFromiLwpsServerInfoResponseReceive;
    }


    /* Allocate space for the server info structure */
    if ( (pssiSpiServerInfo = (struct spiServerInfo *)s_malloc((size_t)(sizeof(struct spiServerInfo)))) == NULL ) {
        iError = LWPS_MemError;
        goto bailFromiLwpsServerInfoResponseReceive;
    }


    /* Read the name */
    if ( (iError = iLwpsStringRead(plLwps, &pssiSpiServerInfo->pucName)) != LWPS_NoError ) {
        goto bailFromiLwpsServerInfoResponseReceive;
    }


    /* Read the description */
    if ( (iError = iLwpsStringRead(plLwps, &pssiSpiServerInfo->pucDescription)) != LWPS_NoError ) {
        goto bailFromiLwpsServerInfoResponseReceive;
    }


    /* Read the admin name */
    if ( (iError = iLwpsStringRead(plLwps, &pssiSpiServerInfo->pucAdminName)) != LWPS_NoError ) {
        goto bailFromiLwpsServerInfoResponseReceive;
    }


    /* Read the admin email */
    if ( (iError = iLwpsStringRead(plLwps, &pssiSpiServerInfo->pucAdminEmail)) != LWPS_NoError ) {
        goto bailFromiLwpsServerInfoResponseReceive;
    }


    /* Read the index count */
    if ( (iError = iLwpsIntegerRead(plLwps, &pssiSpiServerInfo->uiIndexCount)) != LWPS_NoError ) {
        goto bailFromiLwpsServerInfoResponseReceive;
    }


    /* Read the ranking algorithm */
    if ( (iError = iLwpsStringRead(plLwps, &pssiSpiServerInfo->pucRankingAlgorithm)) != LWPS_NoError ) {
        goto bailFromiLwpsServerInfoResponseReceive;
    }


    /* Read the min weight */
    if ( (iError = iLwpsDoubleRead(plLwps, &pssiSpiServerInfo->dWeightMinimum)) != LWPS_NoError ) {
        goto bailFromiLwpsServerInfoResponseReceive;
    }


    /* Read the max weight */
    if ( (iError = iLwpsDoubleRead(plLwps, &pssiSpiServerInfo->dWeightMaximum)) != LWPS_NoError ) {
        goto bailFromiLwpsServerInfoResponseReceive;
    }


    /* Read the reference ID */
    if ( (iError = iLwpsStringRead(plLwps, &pucReferenceID)) != LWPS_NoError ) {
        goto bailFromiLwpsServerInfoResponseReceive;
    }



    /* Bail label */
    bailFromiLwpsServerInfoResponseReceive:


    /* Handle the error */
    if ( iError == LWPS_NoError ) {

        /* Set the return pointers */
        *ppssiSpiServerInfo = pssiSpiServerInfo;
        *ppucReferenceID = pucReferenceID;
    }
    else {

        /* Free allocations */
        iSpiFreeServerInfo(pssiSpiServerInfo);
        pssiSpiServerInfo = NULL;

        s_free(pucReferenceID);
    }


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLwpsServerInfoResponseHandle()

    Purpose:    Handle the server info response by receiving an server info request
                from a client and sending the response

    Parameters: pvLwps      lwps structure

    Globals:    none

    Returns:    LWPS error code

*/
int iLwpsServerInfoResponseHandle
(
    void *pvLwps
)
{

    int                     iError = LWPS_NoError;
    int                     iErrorCode = SPI_NoError;
    unsigned char           *pucErrorString = NULL;
    struct lwps             *plLwps = (struct lwps *)pvLwps;
    struct spiServerInfo    *pssiSpiServerInfo = NULL;
    unsigned char           *pucReferenceID = NULL;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iLwpsServerInfoResponseHandle");  */


    /* Check the parameters */
    if ( pvLwps == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvLwps' parameter passed to 'iLwpsServerInfoResponseHandle'."); 
        return (LWPS_InvalidLwps);
    }


    /* Receive the server info request */
    if ( (iError = iLwpsServerInfoRequestReceive(plLwps, &pucReferenceID)) != LWPS_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to receive an lwps server information request message, lwps error: %d.", iError);
        goto bailFromiLwpsServerInfoResponseHandle;
    }


    /* Put in your server info code here */
/*     iErrorCode = myServerInfoCode(); */
    
    
    /* Handle the server info error */
    if ( iErrorCode == SPI_NoError ) {

        /* Send the server info response */
        if ( (iError = iLwpsServerInfoResponseSend(plLwps, pssiSpiServerInfo, pucReferenceID)) != LWPS_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to send an lwps server information response message, lwps error: %d.", iError);
            goto bailFromiLwpsServerInfoResponseHandle;
        }
    }
    else {

        /* Send the error */
        if ( (iError = iLwpsErrorMessageSend(plLwps, iErrorCode, pucErrorString, pucReferenceID)) != LWPS_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to send an lwps error message, lwps error: %d.", iError);
            goto bailFromiLwpsServerInfoResponseHandle;
        }
    }



    /* Bail label */
    bailFromiLwpsServerInfoResponseHandle:


    /* Free allocated pointers */
    iSpiFreeServerInfo(pssiSpiServerInfo);
    pssiSpiServerInfo = NULL;

    s_free(pucReferenceID);


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLwpsServerIndexInfoRequestSend()

    Purpose:    Send a server index info request to a server

    Parameters: pvLwps              lwps structure
                pucReferenceID      reference ID (optional)

    Globals:    none

    Returns:    LWPS error code

*/
int iLwpsServerIndexInfoRequestSend
(
    void *pvLwps,
    unsigned char *pucReferenceID
)
{

    int             iError = LWPS_NoError;
    struct lwps     *plLwps = (struct lwps *)pvLwps;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iLwpsServerIndexInfoRequestSend");  */


    /* Check the parameters */
    if ( pvLwps == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvLwps' parameter passed to 'iLwpsServerIndexInfoRequestSend'."); 
        return (LWPS_InvalidLwps);
    }


    /* Send the message header */
    if ( (iError = iLwpsHeaderSend(plLwps, LWPS_SERVER_INDEX_INFO_REQUEST_ID)) != LWPS_NoError ) {
        goto bailFromiLwpsServerIndexInfoRequestSend;
    }


    /* Send the reference ID string */
    if ( (iError = iLwpsStringWrite(plLwps, pucReferenceID)) != LWPS_NoError ) {
        goto bailFromiLwpsServerIndexInfoRequestSend;
    }



    /* Bail label */
    bailFromiLwpsServerIndexInfoRequestSend:


    /* Handle the error */
    if ( iError == LWPS_NoError ) {
    
        /* Send the data */
        if ( (iError = iUtlNetSend(plLwps->pvUtlNet)) != UTL_NoError ) {
            iError = iLwpsMapUtlNetError(iError, LWPS_FailedWriteData);
        }
    }
#if defined(LWPS_ENABLE_CLEAR_SEND_BUFFER_ON_SEND_ERROR)
    else {

        /* Reset the send buffer to clear it */
        if ( (iError = iUtlNetResetSend(plLwps->pvUtlNet)) != UTL_NoError ) {
            iError = iLwpsMapUtlNetError(iError, LWPS_FailedWriteData);
        }
    }
#endif    /* defined(LWPS_ENABLE_CLEAR_SEND_BUFFER_ON_SEND_ERROR) */


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLwpsServerIndexInfoRequestReceive()

    Purpose:    Receive a server index info request from a client, this
                function assumes that the header has already been read

    Parameters: pvLwps              lwps structure
                ppucReferenceID     return pointer for the reference ID (allocated)

    Globals:    none

    Returns:    LWPS error code

*/
int iLwpsServerIndexInfoRequestReceive
(
    void *pvLwps,
    unsigned char **ppucReferenceID
)
{

    int             iError = LWPS_NoError;
    struct lwps     *plLwps = (struct lwps *)pvLwps;
    unsigned int    uiMessageID = LWPS_INVALID_ID;
    unsigned char   *pucReferenceID = NULL;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iLwpsServerIndexInfoRequestReceive");  */


    /* Check the parameters */
    if ( pvLwps == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvLwps' parameter passed to 'iLwpsServerIndexInfoRequestReceive'."); 
        return (LWPS_InvalidLwps);
    }

    if ( ppucReferenceID == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppucReferenceID' parameter passed to 'iLwpsServerIndexInfoRequestReceive'."); 
        return (LWPS_ReturnParameterError);
    }


    /* Peek the response header, we do this here because we don't want 
    ** to harm the response if it is not what we expected
    */
    if ( (iError = iLwpsHeaderPeek(plLwps, &uiMessageID)) != LWPS_NoError ) {
        goto bailFromiLwpsServerIndexInfoRequestReceive;
    }


    /* Check that the response header message tag is correct */
    if ( uiMessageID != LWPS_SERVER_INDEX_INFO_REQUEST_ID) {
        iError = LWPS_InvalidMessageID;
        goto bailFromiLwpsServerIndexInfoRequestReceive;
    }


    /* Read the response header */
    if ( (iError = iLwpsHeaderReceive(plLwps, &uiMessageID)) != LWPS_NoError ) {
        goto bailFromiLwpsServerIndexInfoRequestReceive;
    }


    /* Read the reference ID */
    if ( (iError = iLwpsStringRead(plLwps, &pucReferenceID)) != LWPS_NoError ) {
        goto bailFromiLwpsServerIndexInfoRequestReceive;
    }



    /* Bail label */
    bailFromiLwpsServerIndexInfoRequestReceive:


    /* Handle the error */
    if ( iError == LWPS_NoError ) {

        /* Set the return pointers */
        *ppucReferenceID = pucReferenceID;
    }    
    else {

        /* Free allocations */
        s_free(pucReferenceID);
    }


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLwpsServerIndexInfoRequestHandle()

    Purpose:    Handle a server index info request by sending a 
                server a server index info request and receiving the response

    Parameters: pvLwps                          lwps structure
                ppssiiSpiServerIndexInfos       return pointer for the array of spi server index info (allocated)
                puiSpiServerIndexInfosLength    return pointer for the number of entries in the spi server index info array
                piErrorCode                     return pointer for the error code
                ppucErrorString                 return pointer for the error string (allocated)

    Globals:    none

    Returns:    LWPS error code

*/
int iLwpsServerIndexInfoRequestHandle
(
    void *pvLwps,
    struct spiServerIndexInfo **ppssiiSpiServerIndexInfos,
    unsigned int *puiSpiServerIndexInfosLength,
    int *piErrorCode,
    unsigned char **ppucErrorString
)
{

    int             iError = LWPS_NoError;
    struct lwps     *plLwps = (struct lwps *)pvLwps;
    unsigned int    uiMessageID = LWPS_INVALID_ID;
    unsigned char   pucSentReferenceID[LWPS_REFERENCE_ID_BUFFER_LENGTH + 1] = {'\0'};
    unsigned char   *pucReturnedReferenceID = NULL;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iLwpsServerIndexInfoRequestHandle");  */


    /* Check the parameters */
    if ( pvLwps == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvLwps' parameter passed to 'iLwpsServerIndexInfoRequestHandle'."); 
        return (LWPS_InvalidLwps);
    }

    if ( ppssiiSpiServerIndexInfos == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppssiiSpiServerIndexInfos' parameter passed to 'iLwpsServerIndexInfoRequestHandle'."); 
        return (LWPS_ReturnParameterError);
    }

    if ( puiSpiServerIndexInfosLength == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'puiSpiServerIndexInfosLength' parameter passed to 'iLwpsServerIndexInfoRequestHandle'."); 
        return (LWPS_ReturnParameterError);
    }

    if ( piErrorCode == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'piErrorCode' parameter passed to 'iLwpsServerIndexInfoRequestHandle'."); 
        return (LWPS_ReturnParameterError);
    }

    if ( ppucErrorString == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppucErrorString' parameter passed to 'iLwpsServerIndexInfoRequestHandle'."); 
        return (LWPS_ReturnParameterError);
    }


    /* Create a reference ID */
    snprintf(pucSentReferenceID, LWPS_REFERENCE_ID_BUFFER_LENGTH + 1, "%u", plLwps->uiReferenceID);

    
    /* Increment the reference ID */
    plLwps->uiReferenceID++;


    /* Clear the returned error code */
    *piErrorCode = SPI_NoError;
    

    /* Send the server index info request */
    if ( (iError = iLwpsServerIndexInfoRequestSend(plLwps, pucSentReferenceID)) != LWPS_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to send an lwps server index information request message, lwps error: %d.", iError);
        goto bailFromiLwpsServerIndexInfoRequestHandle;
    }


    /* Receive the data */
    if ( (iError = iUtlNetReceive(plLwps->pvUtlNet)) != UTL_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to receive data, utl error: %d.", iError);
        iError = iLwpsMapUtlNetError(iError, LWPS_FailedReadData);
        goto bailFromiLwpsServerIndexInfoRequestHandle;
    }


    /* Peek the header */
    if ( (iError = iLwpsHeaderPeek(pvLwps, &uiMessageID)) != LWPS_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to peek the lwps header, lwps error: %d.", iError);
        goto bailFromiLwpsServerIndexInfoRequestHandle;
    }


    /* Handle the message */
    if ( uiMessageID == LWPS_SERVER_INDEX_INFO_RESPONSE_ID ) {

        /* Receive the server index info response */
        if ( (iError = iLwpsServerIndexInfoResponseReceive(plLwps, ppssiiSpiServerIndexInfos, puiSpiServerIndexInfosLength, &pucReturnedReferenceID)) != LWPS_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to receive an lwps server index information response message, lwps error: %d.", iError);
            goto bailFromiLwpsServerIndexInfoRequestHandle;
        }
    }
    else if ( uiMessageID == LWPS_ERROR_MESSAGE_ID ) {

        /* Receive the error message */
        if ( (iError = iLwpsErrorMessageReceive(plLwps, piErrorCode, ppucErrorString, &pucReturnedReferenceID)) != LWPS_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to receive an lwps error message, lwps error: %d.", iError);
            goto bailFromiLwpsServerIndexInfoRequestHandle;
        }
    }
    else {
        iError = LWPS_InvalidMessageID;
        goto bailFromiLwpsServerIndexInfoRequestHandle;
    }


    /* Check that the reference ID was correctly returned */
    if ( !((bUtlStringsIsStringNULL(pucReturnedReferenceID) == false) && (s_strcmp(pucReturnedReferenceID, pucSentReferenceID) == 0)) ) {
        iError = LWPS_InvalidReferenceID;
        goto bailFromiLwpsServerIndexInfoRequestHandle;
    }



    /* Bail label */
    bailFromiLwpsServerIndexInfoRequestHandle:


    /* Free allocated pointers */
    s_free(pucReturnedReferenceID);


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLwpsServerIndexInfoResponseSend()

    Purpose:    Send a server index info response to a client

    Parameters: pvLwps                          lwps structure
                pssiiSpiServerIndexInfos        array of spi server index info structures (optional)
                uiSpiServerIndexInfosLength     number of entries in the spi server index info structure array
                pucReferenceID                  reference ID (optional)

    Globals:    none

    Returns:    LWPS error code

*/
int iLwpsServerIndexInfoResponseSend
(
    void *pvLwps,
    struct spiServerIndexInfo *pssiiSpiServerIndexInfos,
    unsigned int uiSpiServerIndexInfosLength,
    unsigned char *pucReferenceID
)
{

    int                         iError = LWPS_NoError;
    struct lwps                 *plLwps = (struct lwps *)pvLwps;
    unsigned int                uiI = 0;
    struct spiServerIndexInfo   *pssiiSpiServerIndexInfosPtr = NULL;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iLwpsServerIndexInfoResponseSend");  */


    /* Check the parameters */
    if ( pvLwps == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvLwps' parameter passed to 'iLwpsServerIndexInfoResponseSend'."); 
        return (LWPS_InvalidLwps);
    }

    if ( uiSpiServerIndexInfosLength < 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiSpiServerIndexInfosLength' parameter passed to 'iLwpsServerIndexInfoResponseSend'."); 
        return (LWPS_ParameterError);
    }

    if ( (pssiiSpiServerIndexInfos == NULL) && (uiSpiServerIndexInfosLength > 0) ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'pssiiSpiServerIndexInfos' & 'uiSpiServerIndexInfosLength' parameters passed to 'iLwpsServerIndexInfoResponseSend'."); 
        return (LWPS_ParameterError);
    }


    /* Send the reply header */
    if ( (iError = iLwpsHeaderSend(plLwps, LWPS_SERVER_INDEX_INFO_RESPONSE_ID)) != LWPS_NoError ) {
        goto bailFromiLwpsServerIndexInfoResponseSend;
    }


    /* Send the number of entries in the server index info array */
    if ( (iError = iLwpsIntegerWrite(plLwps, uiSpiServerIndexInfosLength)) != LWPS_NoError ) {
        goto bailFromiLwpsServerIndexInfoResponseSend;
    }


    /* Cycle through the server index info entries and send them */
    for ( uiI = 0, pssiiSpiServerIndexInfosPtr = pssiiSpiServerIndexInfos; uiI < uiSpiServerIndexInfosLength; uiI++, pssiiSpiServerIndexInfosPtr++ ) { 

        /* Send the index name */
        if ( (iError = iLwpsStringWrite(plLwps, pssiiSpiServerIndexInfosPtr->pucName)) != LWPS_NoError ) {
            goto bailFromiLwpsServerIndexInfoResponseSend;
        }

        /* Send the index description */
        if ( (iError = iLwpsStringWrite(plLwps, pssiiSpiServerIndexInfosPtr->pucDescription)) != LWPS_NoError ) {
            goto bailFromiLwpsServerIndexInfoResponseSend;
        }
    }


    /* Send the reference ID string */
    if ( (iError = iLwpsStringWrite(plLwps, pucReferenceID)) != LWPS_NoError ) {
        goto bailFromiLwpsServerIndexInfoResponseSend;
    }



    /* Bail label */
    bailFromiLwpsServerIndexInfoResponseSend:


    /* Handle the error */
    if ( iError == LWPS_NoError ) {
    
        /* Send the data */
        if ( (iError = iUtlNetSend(plLwps->pvUtlNet)) != UTL_NoError ) {
            iError = iLwpsMapUtlNetError(iError, LWPS_FailedWriteData);
        }
    }
#if defined(LWPS_ENABLE_CLEAR_SEND_BUFFER_ON_SEND_ERROR)
    else {

        /* Reset the send buffer to clear it */
        if ( (iError = iUtlNetResetSend(plLwps->pvUtlNet)) != UTL_NoError ) {
            iError = iLwpsMapUtlNetError(iError, LWPS_FailedWriteData);
        }
    }
#endif    /* defined(LWPS_ENABLE_CLEAR_SEND_BUFFER_ON_SEND_ERROR) */


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLwpsServerIndexInfoResponseReceive()

    Purpose:    Receive a server index info response from a server

    Parameters: pvLwps                          lwps structure
                ppssiiSpiServerIndexInfos       return pointer for the array of spi server index info structures (allocated)
                puiSpiServerIndexInfosLength    return pointer for the number of entries in the spi server index info array
                ppucReferenceID                 return pointer for the reference ID (allocated)

    Globals:    none

    Returns:    LWPS error code

*/
int iLwpsServerIndexInfoResponseReceive
(
    void *pvLwps,
    struct spiServerIndexInfo **ppssiiSpiServerIndexInfos,
    unsigned int *puiSpiServerIndexInfosLength,
    unsigned char **ppucReferenceID
)
{

    int                         iError = LWPS_NoError;
    struct lwps                 *plLwps = (struct lwps *)pvLwps;
    unsigned int                uiMessageID = LWPS_INVALID_ID;
    struct spiServerIndexInfo   *pssiiSpiServerIndexInfos = NULL;
    unsigned int                uiSpiServerIndexInfosLength = 0;
    unsigned char               *pucReferenceID = NULL;
    unsigned int                uiI = 0;
    struct spiServerIndexInfo   *pssiiSpiServerIndexInfosPtr = NULL;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iLwpsServerIndexInfoResponseReceive");  */


    /* Check the parameters */
    if ( pvLwps == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvLwps' parameter passed to 'iLwpsServerIndexInfoResponseReceive'."); 
        return (LWPS_InvalidLwps);
    }

    if ( ppssiiSpiServerIndexInfos == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppssiiSpiServerIndexInfos' parameter passed to 'iLwpsServerIndexInfoResponseReceive'."); 
        return (LWPS_ReturnParameterError);
    }

    if ( puiSpiServerIndexInfosLength == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'puiSpiServerIndexInfosLength' parameter passed to 'iLwpsServerIndexInfoResponseReceive'."); 
        return (LWPS_ReturnParameterError);
    }

    if ( ppucReferenceID == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppucReferenceID' parameter passed to 'iLwpsServerIndexInfoResponseReceive'."); 
        return (LWPS_ReturnParameterError);
    }


    /* Peek the response header, we do this here because we don't want 
    ** to harm the response if it is not what we expected
    */
    if ( (iError = iLwpsHeaderPeek(plLwps, &uiMessageID)) != LWPS_NoError ) {
        goto bailFromiLwpsServerIndexInfoResponseReceive;
    }


    /* Check that the response header message tag is correct */
    if ( uiMessageID != LWPS_SERVER_INDEX_INFO_RESPONSE_ID ) {
        iError = LWPS_InvalidMessageID;
        goto bailFromiLwpsServerIndexInfoResponseReceive;
    }


    /* Read the response header */
    if ( (iError = iLwpsHeaderReceive(plLwps, &uiMessageID)) != LWPS_NoError ) {
        goto bailFromiLwpsServerIndexInfoResponseReceive;
    }


    /* Read the number of entries in the server index info */
    if ( (iError = iLwpsIntegerRead(plLwps, &uiSpiServerIndexInfosLength)) != LWPS_NoError ) {
        goto bailFromiLwpsServerIndexInfoResponseReceive;
    }


    /* Read the index information if there is any */
    if ( uiSpiServerIndexInfosLength > 0 ) {

        /* Allocate space for the server index info entries */
        if ( (pssiiSpiServerIndexInfos = (struct spiServerIndexInfo *)s_malloc((size_t)(sizeof(struct spiServerIndexInfo) * uiSpiServerIndexInfosLength))) == NULL ) {
            iError = LWPS_MemError;
            goto bailFromiLwpsServerIndexInfoResponseReceive;
        }

        /* Cycle through the server index info entries and read them */
        for ( uiI = 0, pssiiSpiServerIndexInfosPtr = pssiiSpiServerIndexInfos; uiI < uiSpiServerIndexInfosLength; uiI++, pssiiSpiServerIndexInfosPtr++ ) { 

            /* Read the index name */
            if ( (iError = iLwpsStringRead(plLwps, &pssiiSpiServerIndexInfosPtr->pucName)) != LWPS_NoError ) {
                goto bailFromiLwpsServerIndexInfoResponseReceive;
            }

            /* Read the index description */
            if ( (iError = iLwpsStringRead(plLwps, &pssiiSpiServerIndexInfosPtr->pucDescription)) != LWPS_NoError ) {
                goto bailFromiLwpsServerIndexInfoResponseReceive;
            }
        }
    }


    /* Read the reference ID */
    if ( (iError = iLwpsStringRead(plLwps, &pucReferenceID)) != LWPS_NoError ) {
        goto bailFromiLwpsServerIndexInfoResponseReceive;
    }



    /* Bail label */
    bailFromiLwpsServerIndexInfoResponseReceive:


    /* Handle the error */
    if ( iError == LWPS_NoError ) {

        /* Set the return pointers */
        *ppssiiSpiServerIndexInfos = pssiiSpiServerIndexInfos;
        *puiSpiServerIndexInfosLength = uiSpiServerIndexInfosLength;
        *ppucReferenceID = pucReferenceID;
    }
    else {

        /* Free allocations */
        iSpiFreeServerIndexInfo(pssiiSpiServerIndexInfos, uiSpiServerIndexInfosLength);
        pssiiSpiServerIndexInfos = NULL;

        s_free(pucReferenceID);
    }


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLwpsServerIndexInfoResponseHandle()

    Purpose:    Handle the server index info response by receiving a
                server index info request from a client and sending the response

    Parameters: pvLwps      lwps structure

    Globals:    none

    Returns:    LWPS error code

*/
int iLwpsServerIndexInfoResponseHandle
(
    void *pvLwps
)
{

    int                         iError = LWPS_NoError;
    int                         iErrorCode = SPI_NoError;
    unsigned char               *pucErrorString = NULL;
    struct lwps                 *plLwps = (struct lwps *)pvLwps;
    struct spiServerIndexInfo   *pssiiSpiServerIndexInfos = NULL;
    unsigned int                uiSpiServerIndexInfosLength = 0;
    unsigned char               *pucReferenceID = NULL;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iLwpsServerIndexInfoResponseHandle");  */


    /* Check the parameters */
    if ( pvLwps == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvLwps' parameter passed to 'iLwpsServerIndexInfoResponseHandle'."); 
        return (LWPS_InvalidLwps);
    }


    /* Receive the server index info request */
    if ( (iError = iLwpsServerIndexInfoRequestReceive(plLwps, &pucReferenceID)) != LWPS_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to receive an lwps server index information request message, lwps error: %d.", iError);
        goto bailFromiLwpsServerIndexInfoResponseHandle;
    }


    /* Put in your server index info code here */
/*     iErrorCode = myServerIndexInfoCode(); */
    
    
    /* Handle the server index info error */
    if ( iErrorCode == SPI_NoError ) {

        /* Send the server index info response */
        if ( (iError = iLwpsServerIndexInfoResponseSend(plLwps, pssiiSpiServerIndexInfos, uiSpiServerIndexInfosLength, pucReferenceID)) != LWPS_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to send an lwps server index information response message, lwps error: %d.", iError);
            goto bailFromiLwpsServerIndexInfoResponseHandle;
        }
    }
    else {

        /* Send the error */
        if ( (iError = iLwpsErrorMessageSend(plLwps, iErrorCode, pucErrorString, pucReferenceID)) != LWPS_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to send an lwps error message, lwps error: %d.", iError);
            goto bailFromiLwpsServerIndexInfoResponseHandle;
        }
    }



    /* Bail label */
    bailFromiLwpsServerIndexInfoResponseHandle:


    /* Free allocated pointers */
    iSpiFreeServerIndexInfo(pssiiSpiServerIndexInfos, uiSpiServerIndexInfosLength);
    pssiiSpiServerIndexInfos = NULL;
    
    s_free(pucReferenceID);


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLwpsIndexInfoRequestSend()

    Purpose:    Send a index info request to a server 

    Parameters: pvLwps              lwps structure
                pucIndexName        index name (optional)
                pucReferenceID      reference ID (optional)

    Globals:    none

    Returns:    LWPS error code

*/
int iLwpsIndexInfoRequestSend
(
    void *pvLwps,
    unsigned char *pucIndexName,
    unsigned char *pucReferenceID
)
{

    int             iError = LWPS_NoError;
    struct lwps     *plLwps = (struct lwps *)pvLwps;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iLwpsIndexInfoRequestSend");  */


    /* Check the parameters */
    if ( pvLwps == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvLwps' parameter passed to 'iLwpsIndexInfoRequestSend'."); 
        return (LWPS_InvalidLwps);
    }


    /* Send the message header */
    if ( (iError = iLwpsHeaderSend(plLwps, LWPS_INDEX_INFO_REQUEST_ID)) != LWPS_NoError ) {
        goto bailFromiLwpsIndexInfoRequestSend;
    }


    /* Send the index name */
    if ( (iError = iLwpsStringWrite(plLwps, pucIndexName)) != LWPS_NoError ) {
        goto bailFromiLwpsIndexInfoRequestSend;
    }


    /* Send the reference ID string */
    if ( (iError = iLwpsStringWrite(plLwps, pucReferenceID)) != LWPS_NoError ) {
        goto bailFromiLwpsIndexInfoRequestSend;
    }



    /* Bail label */
    bailFromiLwpsIndexInfoRequestSend:


    /* Handle the error */
    if ( iError == LWPS_NoError ) {
    
        /* Send the data */
        if ( (iError = iUtlNetSend(plLwps->pvUtlNet)) != UTL_NoError ) {
            iError = iLwpsMapUtlNetError(iError, LWPS_FailedWriteData);
        }
    }
#if defined(LWPS_ENABLE_CLEAR_SEND_BUFFER_ON_SEND_ERROR)
    else {

        /* Reset the send buffer to clear it */
        if ( (iError = iUtlNetResetSend(plLwps->pvUtlNet)) != UTL_NoError ) {
            iError = iLwpsMapUtlNetError(iError, LWPS_FailedWriteData);
        }
    }
#endif    /* defined(LWPS_ENABLE_CLEAR_SEND_BUFFER_ON_SEND_ERROR) */


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLwpsIndexInfoRequestReceive()

    Purpose:    Receive a index info request from a client, this function assumes
                that the header has already been read

    Parameters: pvLwps              lwps structure
                ppucIndexName       return pointer for the index name (allocated)
                ppucReferenceID     return pointer for the reference ID (allocated)

    Globals:    none

    Returns:    LWPS error code

*/
int iLwpsIndexInfoRequestReceive
(
    void *pvLwps,
    unsigned char **ppucIndexName,
    unsigned char **ppucReferenceID
)
{

    int             iError = LWPS_NoError;
    struct lwps     *plLwps = (struct lwps *)pvLwps;
    unsigned int    uiMessageID = LWPS_INVALID_ID;
    unsigned char   *pucIndexName = NULL;
    unsigned char   *pucReferenceID = NULL;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iLwpsIndexInfoRequestReceive");  */


    /* Check the parameters */
    if ( pvLwps == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvLwps' parameter passed to 'iLwpsIndexInfoRequestReceive'."); 
        return (LWPS_InvalidLwps);
    }

    if ( ppucIndexName == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppucIndexName' parameter passed to 'iLwpsIndexInfoRequestReceive'."); 
        return (LWPS_ReturnParameterError);
    }

    if ( ppucReferenceID == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppucReferenceID' parameter passed to 'iLwpsIndexInfoRequestReceive'."); 
        return (LWPS_ReturnParameterError);
    }


    /* Peek the response header, we do this here because we don't want 
    ** to harm the response if it is not what we expected
    */
    if ( (iError = iLwpsHeaderPeek(plLwps, &uiMessageID)) != LWPS_NoError ) {
        goto bailFromiLwpsIndexInfoRequestReceive;
    }


    /* Check that the response header message tag is correct */
    if ( uiMessageID != LWPS_INDEX_INFO_REQUEST_ID) {
        iError = LWPS_InvalidMessageID;
        goto bailFromiLwpsIndexInfoRequestReceive;
    }


    /* Read the response header */
    if ( (iError = iLwpsHeaderReceive(plLwps, &uiMessageID)) != LWPS_NoError ) {
        goto bailFromiLwpsIndexInfoRequestReceive;
    }


    /* Read the index name */
    if ( (iError = iLwpsStringRead(plLwps, &pucIndexName)) != LWPS_NoError ) {
        goto bailFromiLwpsIndexInfoRequestReceive;
    }


    /* Read the reference ID */
    if ( (iError = iLwpsStringRead(plLwps, &pucReferenceID)) != LWPS_NoError ) {
        goto bailFromiLwpsIndexInfoRequestReceive;
    }



    /* Bail label */
    bailFromiLwpsIndexInfoRequestReceive:


    /* Handle the error */
    if ( iError == LWPS_NoError ) {

        /* Set the return pointers */
        *ppucIndexName = pucIndexName;
        *ppucReferenceID = pucReferenceID;
    }    
    else {

        /* Free allocations */
        s_free(pucIndexName);
        s_free(pucReferenceID);
    }


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLwpsIndexInfoRequestHandle()

    Purpose:    Handle a index info by sending a index info request
                to a server and receiving the response

    Parameters: pvLwps              lwps structure
                pucIndexName        index name (optional)
                ppsiiSpiIndexInfo   return pointer for the spi index info structure (allocated)
                piErrorCode         return pointer for the error code
                ppucErrorString     return pointer for the error string (allocated)

    Globals:    none

    Returns:    LWPS error code

*/
int iLwpsIndexInfoRequestHandle
(
    void *pvLwps,
    unsigned char *pucIndexName,
    struct spiIndexInfo **ppsiiSpiIndexInfo,
    int *piErrorCode,
    unsigned char **ppucErrorString
)
{


    int             iError = LWPS_NoError;
    struct lwps     *plLwps = (struct lwps *)pvLwps;
    unsigned int    uiMessageID = LWPS_INVALID_ID;
    unsigned char   pucSentReferenceID[LWPS_REFERENCE_ID_BUFFER_LENGTH + 1] = {'\0'};
    unsigned char   *pucReturnedReferenceID = NULL;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iLwpsServerInfoRequestHandle");  */


    /* Check the parameters */
    if ( pvLwps == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvLwps' parameter passed to 'iLwpsIndexInfoRequestHandle'."); 
        return (LWPS_InvalidLwps);
    }

    if ( ppsiiSpiIndexInfo == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppsiiSpiIndexInfo' parameter passed to 'iLwpsIndexInfoRequestHandle'."); 
        return (LWPS_ReturnParameterError);
    }

    if ( piErrorCode == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'piErrorCode' parameter passed to 'iLwpsIndexInfoRequestHandle'."); 
        return (LWPS_ReturnParameterError);
    }

    if ( ppucErrorString == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppucErrorString' parameter passed to 'iLwpsIndexInfoRequestHandle'."); 
        return (LWPS_ReturnParameterError);
    }


    /* Create a reference ID */
    snprintf(pucSentReferenceID, LWPS_REFERENCE_ID_BUFFER_LENGTH + 1, "%u", plLwps->uiReferenceID);

    
    /* Increment the reference ID */
    plLwps->uiReferenceID++;


    /* Clear the returned error code */
    *piErrorCode = SPI_NoError;
    

    /* Send the index info request */
    if ( (iError = iLwpsIndexInfoRequestSend(plLwps, pucIndexName, pucSentReferenceID)) != LWPS_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to send an lwps index information request message, lwps error: %d.", iError);
        goto bailFromiLwpsIndexInfoRequestHandle;
    }


    /* Receive the data */
    if ( (iError = iUtlNetReceive(plLwps->pvUtlNet)) != UTL_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to receive data, utl error: %d.", iError);
        iError = iLwpsMapUtlNetError(iError, LWPS_FailedReadData);
        goto bailFromiLwpsIndexInfoRequestHandle;
    }


    /* Peek the header */
    if ( (iError = iLwpsHeaderPeek(pvLwps, &uiMessageID)) != LWPS_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to peek the lwps header, lwps error: %d.", iError);
        goto bailFromiLwpsIndexInfoRequestHandle;
    }


    /* Handle the message */
    if ( uiMessageID == LWPS_INDEX_INFO_RESPONSE_ID ) {

        /* Receive the index info response */
        if ( (iError = iLwpsIndexInfoResponseReceive(plLwps, ppsiiSpiIndexInfo, &pucReturnedReferenceID)) != LWPS_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to receive an lwps index information response message, lwps error: %d.", iError);
            goto bailFromiLwpsIndexInfoRequestHandle;
        }
    }
    else if ( uiMessageID == LWPS_ERROR_MESSAGE_ID ) {

        /* Receive the error message */
        if ( (iError = iLwpsErrorMessageReceive(plLwps, piErrorCode, ppucErrorString, &pucReturnedReferenceID)) != LWPS_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to receive an lwps error message, lwps error: %d.", iError);
            goto bailFromiLwpsIndexInfoRequestHandle;
        }
    }
    else {
        iError = LWPS_InvalidMessageID;
        goto bailFromiLwpsIndexInfoRequestHandle;
    }


    /* Check that the reference ID was correctly returned */
    if ( !((bUtlStringsIsStringNULL(pucReturnedReferenceID) == false) && (s_strcmp(pucReturnedReferenceID, pucSentReferenceID) == 0)) ) {
        iError = LWPS_InvalidReferenceID;
        goto bailFromiLwpsIndexInfoRequestHandle;
    }



    /* Bail label */
    bailFromiLwpsIndexInfoRequestHandle:


    /* Free allocated pointers */
    s_free(pucReturnedReferenceID);


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLwpsIndexInfoResponseSend()

    Purpose:    Send a index info response to a client

    Parameters: pvLwps              lwps structure
                psiiSpiIndexInfo    spi index info structure
                pucReferenceID      reference ID (optional)

    Globals:    none

    Returns:    LWPS error code

*/
int iLwpsIndexInfoResponseSend
(
    void *pvLwps,
    struct spiIndexInfo *psiiSpiIndexInfo,
    unsigned char *pucReferenceID
)
{

    int             iError = LWPS_NoError;
    struct lwps     *plLwps = (struct lwps *)pvLwps;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iLwpsIndexInfoResponseSend");  */


    /* Check the parameters */
    if ( pvLwps == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvLwps' parameter passed to 'iLwpsIndexInfoResponseSend'."); 
        return (LWPS_InvalidLwps);
    }

    if ( psiiSpiIndexInfo == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'psiiSpiIndexInfo' parameter passed to 'iLwpsIndexInfoResponseSend'."); 
        return (LWPS_ParameterError);
    }


    /* Send the reply header */
    if ( (iError = iLwpsHeaderSend(plLwps, LWPS_INDEX_INFO_RESPONSE_ID)) != LWPS_NoError ) {
        goto bailFromiLwpsIndexInfoResponseSend;
    }


    /* Send the index name */
    if ( (iError = iLwpsStringWrite(plLwps, psiiSpiIndexInfo->pucName)) != LWPS_NoError ) {
        goto bailFromiLwpsIndexInfoResponseSend;
    }


    /* Send the index description */
    if ( (iError = iLwpsStringWrite(plLwps, psiiSpiIndexInfo->pucDescription)) != LWPS_NoError ) {
        goto bailFromiLwpsIndexInfoResponseSend;
    }


    /* Send the language code */
    if ( (iError = iLwpsStringWrite(plLwps, psiiSpiIndexInfo->pucLanguageCode)) != LWPS_NoError ) {
        goto bailFromiLwpsIndexInfoResponseSend;
    }


    /* Send the tokenizer name */
    if ( (iError = iLwpsStringWrite(plLwps, psiiSpiIndexInfo->pucTokenizerName)) != LWPS_NoError ) {
        goto bailFromiLwpsIndexInfoResponseSend;
    }


    /* Send the stemmer name */
    if ( (iError = iLwpsStringWrite(plLwps, psiiSpiIndexInfo->pucStemmerName)) != LWPS_NoError ) {
        goto bailFromiLwpsIndexInfoResponseSend;
    }


    /* Send the stop list name */
    if ( (iError = iLwpsStringWrite(plLwps, psiiSpiIndexInfo->pucStopListName)) != LWPS_NoError ) {
        goto bailFromiLwpsIndexInfoResponseSend;
    }


    /* Send the document count */
    if ( (iError = iLwpsIntegerWrite(plLwps, psiiSpiIndexInfo->uiDocumentCount)) != LWPS_NoError ) {
        goto bailFromiLwpsIndexInfoResponseSend;
    }


    /* Send the total term count */
    if ( (iError = iLwpsLongWrite(plLwps, psiiSpiIndexInfo->ulTotalTermCount)) != LWPS_NoError ) {
        goto bailFromiLwpsIndexInfoResponseSend;
    }


    /* Send the unique term count */
    if ( (iError = iLwpsLongWrite(plLwps, psiiSpiIndexInfo->ulUniqueTermCount)) != LWPS_NoError ) {
        goto bailFromiLwpsIndexInfoResponseSend;
    }


    /* Send the total stop term count */
    if ( (iError = iLwpsLongWrite(plLwps, psiiSpiIndexInfo->ulTotalStopTermCount)) != LWPS_NoError ) {
        goto bailFromiLwpsIndexInfoResponseSend;
    }


    /* Send the unique stop term count */
    if ( (iError = iLwpsLongWrite(plLwps, psiiSpiIndexInfo->ulUniqueStopTermCount)) != LWPS_NoError ) {
        goto bailFromiLwpsIndexInfoResponseSend;
    }


    /* Send the access control flag */
    if ( (iError = iLwpsIntegerWrite(plLwps, psiiSpiIndexInfo->uiAccessControl)) != LWPS_NoError ) {
        goto bailFromiLwpsIndexInfoResponseSend;
    }


    /* Send the update frequency flag */
    if ( (iError = iLwpsIntegerWrite(plLwps, psiiSpiIndexInfo->uiUpdateFrequency)) != LWPS_NoError ) {
        goto bailFromiLwpsIndexInfoResponseSend;
    }


    /* Send the last update ansi date */
    if ( (iError = iLwpsLongWrite(plLwps, psiiSpiIndexInfo->ulLastUpdateAnsiDate)) != LWPS_NoError ) {
        goto bailFromiLwpsIndexInfoResponseSend;
    }


    /* Send the case sensitive */
    if ( (iError = iLwpsIntegerWrite(plLwps, psiiSpiIndexInfo->uiCaseSensitive)) != LWPS_NoError ) {
        goto bailFromiLwpsIndexInfoResponseSend;
    }


    /* Send the reference ID string */
    if ( (iError = iLwpsStringWrite(plLwps, pucReferenceID)) != LWPS_NoError ) {
        goto bailFromiLwpsIndexInfoResponseSend;
    }



    /* Bail label */
    bailFromiLwpsIndexInfoResponseSend:


    /* Handle the error */
    if ( iError == LWPS_NoError ) {
    
        /* Send the data */
        if ( (iError = iUtlNetSend(plLwps->pvUtlNet)) != UTL_NoError ) {
            iError = iLwpsMapUtlNetError(iError, LWPS_FailedWriteData);
        }
    }
#if defined(LWPS_ENABLE_CLEAR_SEND_BUFFER_ON_SEND_ERROR)
    else {

        /* Reset the send buffer to clear it */
        if ( (iError = iUtlNetResetSend(plLwps->pvUtlNet)) != UTL_NoError ) {
            iError = iLwpsMapUtlNetError(iError, LWPS_FailedWriteData);
        }
    }
#endif    /* defined(LWPS_ENABLE_CLEAR_SEND_BUFFER_ON_SEND_ERROR) */


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLwpsIndexInfoResponseReceive()

    Purpose:    Receive a index info response from a server

    Parameters: pvLwps              lwps structure
                ppsiiSpiIndexInfo   return pointer for the spi index info structure (allocated)
                ppucReferenceID     return pointer for the reference ID (allocated)

    Globals:    none

    Returns:    LWPS error code

*/
int iLwpsIndexInfoResponseReceive
(
    void *pvLwps,
    struct spiIndexInfo **ppsiiSpiIndexInfo,
    unsigned char **ppucReferenceID
)
{

    int                     iError = LWPS_NoError;
    struct lwps             *plLwps = (struct lwps *)pvLwps;
    unsigned int            uiMessageID = LWPS_INVALID_ID;
    struct spiIndexInfo     *psiiSpiIndexInfo = NULL;
    unsigned char           *pucReferenceID = NULL;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iLwpsIndexInfoResponseReceive");  */


    /* Check the parameters */
    if ( pvLwps == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvLwps' parameter passed to 'iLwpsIndexInfoResponseReceive'."); 
        return (LWPS_InvalidLwps);
    }

    if ( ppsiiSpiIndexInfo == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppsiiSpiIndexInfo' parameter passed to 'iLwpsIndexInfoResponseReceive'."); 
        return (LWPS_ReturnParameterError);
    }

    if ( ppucReferenceID == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppucReferenceID' parameter passed to 'iLwpsIndexInfoResponseReceive'."); 
        return (LWPS_ReturnParameterError);
    }


    /* Peek the response header, we do this here because we don't want 
    ** to harm the response if it is not what we expected
    */
    if ( (iError = iLwpsHeaderPeek(plLwps, &uiMessageID)) != LWPS_NoError ) {
        goto bailFromlbLwpsIndexInfoResponseReceive;
    }


    /* Check that the response header message tag is correct */
    if ( uiMessageID != LWPS_INDEX_INFO_RESPONSE_ID) {
        iError = LWPS_InvalidMessageID;
        goto bailFromlbLwpsIndexInfoResponseReceive;
    }


    /* Read the response header */
    if ( (iError = iLwpsHeaderReceive(plLwps, &uiMessageID)) != LWPS_NoError ) {
        goto bailFromlbLwpsIndexInfoResponseReceive;
    }


    /* Allocate space for the index info structure */
    if ( (psiiSpiIndexInfo = (struct spiIndexInfo *)s_malloc((size_t)(sizeof(struct spiIndexInfo)))) == NULL ) {
        iError = LWPS_MemError;
        goto bailFromlbLwpsIndexInfoResponseReceive;
    }


    /* Read the index name */
    if ( (iError = iLwpsStringRead(plLwps, &psiiSpiIndexInfo->pucName)) != LWPS_NoError ) {
        goto bailFromlbLwpsIndexInfoResponseReceive;
    }


    /* Read the index description */
    if ( (iError = iLwpsStringRead(plLwps, &psiiSpiIndexInfo->pucDescription)) != LWPS_NoError ) {
        goto bailFromlbLwpsIndexInfoResponseReceive;
    }


    /* Read the language code */
    if ( (iError = iLwpsStringRead(plLwps, &psiiSpiIndexInfo->pucLanguageCode)) != LWPS_NoError ) {
        goto bailFromlbLwpsIndexInfoResponseReceive;
    }


    /* Read the tokenizer name */
    if ( (iError = iLwpsStringRead(plLwps, &psiiSpiIndexInfo->pucTokenizerName)) != LWPS_NoError ) {
        goto bailFromlbLwpsIndexInfoResponseReceive;
    }


    /* Read the stemmer name */
    if ( (iError = iLwpsStringRead(plLwps, &psiiSpiIndexInfo->pucStemmerName)) != LWPS_NoError ) {
        goto bailFromlbLwpsIndexInfoResponseReceive;
    }


    /* Read the stop list name */
    if ( (iError = iLwpsStringRead(plLwps, &psiiSpiIndexInfo->pucStopListName)) != LWPS_NoError ) {
        goto bailFromlbLwpsIndexInfoResponseReceive;
    }


    /* Read the document count */
    if ( (iError = iLwpsIntegerRead(plLwps, &psiiSpiIndexInfo->uiDocumentCount)) != LWPS_NoError ) {
        goto bailFromlbLwpsIndexInfoResponseReceive;
    }


    /* Read the total term count */
    if ( (iError = iLwpsLongRead(plLwps, &psiiSpiIndexInfo->ulTotalTermCount)) != LWPS_NoError ) {
        goto bailFromlbLwpsIndexInfoResponseReceive;
    }


    /* Read the unique term count */
    if ( (iError = iLwpsLongRead(plLwps, &psiiSpiIndexInfo->ulUniqueTermCount)) != LWPS_NoError ) {
        goto bailFromlbLwpsIndexInfoResponseReceive;
    }


    /* Read the total stop term count */
    if ( (iError = iLwpsLongRead(plLwps, &psiiSpiIndexInfo->ulTotalStopTermCount)) != LWPS_NoError ) {
        goto bailFromlbLwpsIndexInfoResponseReceive;
    }


    /* Read the unique stop term count */
    if ( (iError = iLwpsLongRead(plLwps, &psiiSpiIndexInfo->ulUniqueStopTermCount)) != LWPS_NoError ) {
        goto bailFromlbLwpsIndexInfoResponseReceive;
    }


    /* Read the access control flag */
    if ( (iError = iLwpsIntegerRead(plLwps, &psiiSpiIndexInfo->uiAccessControl)) != LWPS_NoError ) {
        goto bailFromlbLwpsIndexInfoResponseReceive;
    }


    /* Read the update frequency flag */
    if ( (iError = iLwpsIntegerRead(plLwps, &psiiSpiIndexInfo->uiUpdateFrequency)) != LWPS_NoError ) {
        goto bailFromlbLwpsIndexInfoResponseReceive;
    }


    /* Read the last update ansi date */
    if ( (iError = iLwpsLongRead(plLwps, &psiiSpiIndexInfo->ulLastUpdateAnsiDate)) != LWPS_NoError ) {
        goto bailFromlbLwpsIndexInfoResponseReceive;
    }


    /* Read the case sensitive */
    if ( (iError = iLwpsIntegerRead(plLwps, &psiiSpiIndexInfo->uiCaseSensitive)) != LWPS_NoError ) {
        goto bailFromlbLwpsIndexInfoResponseReceive;
    }


    /* Read the reference ID */
    if ( (iError = iLwpsStringRead(plLwps, &pucReferenceID)) != LWPS_NoError ) {
        goto bailFromlbLwpsIndexInfoResponseReceive;
    }



    /* Bail label */
    bailFromlbLwpsIndexInfoResponseReceive:


    /* Handle the error */
    if ( iError == LWPS_NoError ) {

        /* Set the return pointers */
        *ppsiiSpiIndexInfo = psiiSpiIndexInfo;
        *ppucReferenceID = pucReferenceID;
    }
    else {

        /* Free allocations */
        iSpiFreeIndexInfo(psiiSpiIndexInfo);
        psiiSpiIndexInfo = NULL;

        s_free(pucReferenceID);
    }


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLwpsIndexInfoResponseHandle()

    Purpose:    Handle the index info response by receiving an index info
                request from a client and sending the response

    Parameters: pvLwps      lwps structure

    Globals:    none

    Returns:    LWPS error code

*/
int iLwpsIndexInfoResponseHandle
(
    void *pvLwps
)
{

    int                     iError = LWPS_NoError;
    int                     iErrorCode = SPI_NoError;
    unsigned char           *pucErrorString = NULL;
    struct lwps             *plLwps = (struct lwps *)pvLwps;
    unsigned char           *pucIndexName = NULL;
    struct spiIndexInfo     *psiiSpiIndexInfo = NULL;
    unsigned char           *pucReferenceID = NULL;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iLwpsIndexInfoResponseHandle");  */


    /* Check the parameters */
    if ( pvLwps == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvLwps' parameter passed to 'iLwpsIndexInfoResponseHandle'."); 
        return (LWPS_InvalidLwps);
    }


    /* Receive the index info request */
    if ( (iError = iLwpsIndexInfoRequestReceive(plLwps, &pucIndexName, &pucReferenceID)) != LWPS_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to receive an lwps index information request message, lwps error: %d.", iError);
        goto bailFromiLwpsIndexInfoResponseHandle;
    }


    /* Put in your index info code here */
/*     iErrorCode = myIndexInfoCode(); */
    
    
    /* Handle the index info error */
    if ( iErrorCode == SPI_NoError ) {

        /* Send the index info response */
        if ( (iError = iLwpsIndexInfoResponseSend(plLwps, psiiSpiIndexInfo, pucReferenceID)) != LWPS_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to send an lwps index information response message, lwps error: %d.", iError);
            goto bailFromiLwpsIndexInfoResponseHandle;
        }
    }
    else {

        /* Send the error */
        if ( (iError = iLwpsErrorMessageSend(plLwps, iErrorCode, pucErrorString, pucReferenceID)) != LWPS_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to send an lwps error message, lwps error: %d.", iError);
            goto bailFromiLwpsIndexInfoResponseHandle;
        }
    }



    /* Bail label */
    bailFromiLwpsIndexInfoResponseHandle:


    /* Free allocated pointers */
    s_free(pucIndexName);
    
    iSpiFreeIndexInfo(psiiSpiIndexInfo);
    psiiSpiIndexInfo = NULL;
    
    s_free(pucReferenceID);


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLwpsIndexFieldInfoRequestSend()

    Purpose:    Send a field info request to a server

    Parameters: pvLwps              lwps structure
                pucIndexName        index name (optional)
                pucReferenceID      reference ID (optional)

    Globals:    none

    Returns:    LWPS error code

*/
int iLwpsIndexFieldInfoRequestSend
(
    void *pvLwps,
    unsigned char *pucIndexName,
    unsigned char *pucReferenceID
)
{

    int             iError = LWPS_NoError;
    struct lwps     *plLwps = (struct lwps *)pvLwps;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iLwpsIndexFieldInfoRequestSend");  */


    /* Check the parameters */
    if ( pvLwps == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvLwps' parameter passed to 'iLwpsIndexFieldInfoRequestSend'."); 
        return (LWPS_InvalidLwps);
    }


    /* Send the message header */
    if ( (iError = iLwpsHeaderSend(plLwps, LWPS_INDEX_FIELD_INFO_REQUEST_ID)) != LWPS_NoError ) {
        goto bailFromiLwpsIndexFieldInfoRequestSend;
    }


    /* Send the index name */
    if ( (iError = iLwpsStringWrite(plLwps, pucIndexName)) != LWPS_NoError ) {
        goto bailFromiLwpsIndexFieldInfoRequestSend;
    }


    /* Send the reference ID string */
    if ( (iError = iLwpsStringWrite(plLwps, pucReferenceID)) != LWPS_NoError ) {
        goto bailFromiLwpsIndexFieldInfoRequestSend;
    }



    /* Bail label */
    bailFromiLwpsIndexFieldInfoRequestSend:


    /* Handle the error */
    if ( iError == LWPS_NoError ) {
    
        /* Send the data */
        if ( (iError = iUtlNetSend(plLwps->pvUtlNet)) != UTL_NoError ) {
            iError = iLwpsMapUtlNetError(iError, LWPS_FailedWriteData);
        }
    }
#if defined(LWPS_ENABLE_CLEAR_SEND_BUFFER_ON_SEND_ERROR)
    else {

        /* Reset the send buffer to clear it */
        if ( (iError = iUtlNetResetSend(plLwps->pvUtlNet)) != UTL_NoError ) {
            iError = iLwpsMapUtlNetError(iError, LWPS_FailedWriteData);
        }
    }
#endif    /* defined(LWPS_ENABLE_CLEAR_SEND_BUFFER_ON_SEND_ERROR) */


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLwpsIndexFieldInfoRequestReceive()

    Purpose:    Receive a field info request from a client, this function
                assumes that the header has already been read

    Parameters: pvLwps              lwps structure
                ppucIndexName       return pointer for the index name (allocated)
                ppucReferenceID     return pointer for the reference ID (allocated)

    Globals:    none

    Returns:    LWPS error code

*/
int iLwpsIndexFieldInfoRequestReceive
(
    void *pvLwps,
    unsigned char **ppucIndexName,
    unsigned char **ppucReferenceID
)
{

    int             iError = LWPS_NoError;
    struct lwps     *plLwps = (struct lwps *)pvLwps;
    unsigned int    uiMessageID = LWPS_INVALID_ID;
    unsigned char   *pucIndexName = NULL;
    unsigned char   *pucReferenceID = NULL;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iLwpsIndexFieldInfoRequestReceive");  */


    /* Check the parameters */
    if ( pvLwps == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvLwps' parameter passed to 'iLwpsIndexFieldInfoRequestReceive'."); 
        return (LWPS_InvalidLwps);
    }

    if ( ppucIndexName == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppucIndexName' parameter passed to 'iLwpsIndexFieldInfoRequestReceive'."); 
        return (LWPS_ReturnParameterError);
    }

    if ( ppucReferenceID == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppucReferenceID' parameter passed to 'iLwpsIndexFieldInfoRequestReceive'."); 
        return (LWPS_ReturnParameterError);
    }


    /* Peek the response header, we do this here because we don't want 
    ** to harm the response if it is not what we expected
    */
    if ( (iError = iLwpsHeaderPeek(plLwps, &uiMessageID)) != LWPS_NoError ) {
        goto bailFromiLwpsIndexFieldInfoRequestReceive;
    }


    /* Check that the response header message tag is correct */
    if ( uiMessageID != LWPS_INDEX_FIELD_INFO_REQUEST_ID) {
        iError = LWPS_InvalidMessageID;
        goto bailFromiLwpsIndexFieldInfoRequestReceive;
    }


    /* Read the response header */
    if ( (iError = iLwpsHeaderReceive(plLwps, &uiMessageID)) != LWPS_NoError ) {
        goto bailFromiLwpsIndexFieldInfoRequestReceive;
    }


    /* Read the index name */
    if ( (iError = iLwpsStringRead(plLwps, &pucIndexName)) != LWPS_NoError ) {
        goto bailFromiLwpsIndexFieldInfoRequestReceive;
    }


    /* Read the reference ID */
    if ( (iError = iLwpsStringRead(plLwps, &pucReferenceID)) != LWPS_NoError ) {
        goto bailFromiLwpsIndexFieldInfoRequestReceive;
    }



    /* Bail label */
    bailFromiLwpsIndexFieldInfoRequestReceive:


    /* Handle the error */
    if ( iError == LWPS_NoError ) {

        /* Set the return pointers */
        *ppucIndexName = pucIndexName;
        *ppucReferenceID = pucReferenceID;
    }    
    else {

        /* Free allocations */
        s_free(pucIndexName);
        s_free(pucReferenceID);
    }


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLwpsIndexFieldInfoRequestHandle()

    Purpose:    Handle a field info request by sending a 
                field info request to a server and receiving the response

    Parameters: pvLwps                      lwps structure
                pucIndexName                index name (optional)
                ppsfiSpiFieldInfos          return pointer for the array of spi field info structures (allocated)
                puiSpiFieldInfosLength      return pointer for the number of entries in the spi field info structure array (allocated)
                piErrorCode                 return pointer for the error code
                ppucErrorString             return pointer for the error string (allocated)

    Globals:    none

    Returns:    LWPS error code

*/
int iLwpsIndexFieldInfoRequestHandle
(
    void *pvLwps,
    unsigned char *pucIndexName,
    struct spiFieldInfo **ppsfiSpiFieldInfos,
    unsigned int *puiSpiFieldInfosLength,
    int *piErrorCode,
    unsigned char **ppucErrorString
)
{

    int             iError = LWPS_NoError;
    struct lwps     *plLwps = (struct lwps *)pvLwps;
    unsigned int    uiMessageID = LWPS_INVALID_ID;
    unsigned char   pucSentReferenceID[LWPS_REFERENCE_ID_BUFFER_LENGTH + 1] = {'\0'};
    unsigned char   *pucReturnedReferenceID = NULL;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iLwpsIndexFieldInfoRequestHandle");  */


    /* Check the parameters */
    if ( pvLwps == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvLwps' parameter passed to 'iLwpsIndexFieldInfoRequestHandle'."); 
        return (LWPS_InvalidLwps);
    }

    if ( ppsfiSpiFieldInfos == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppsfiSpiFieldInfos' parameter passed to 'iLwpsIndexFieldInfoRequestHandle'."); 
        return (LWPS_ReturnParameterError);
    }

    if ( puiSpiFieldInfosLength == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'puiSpiFieldInfosLength' parameter passed to 'iLwpsIndexFieldInfoRequestHandle'."); 
        return (LWPS_ReturnParameterError);
    }

    if ( piErrorCode == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'piErrorCode' parameter passed to 'iLwpsIndexFieldInfoRequestHandle'."); 
        return (LWPS_ReturnParameterError);
    }

    if ( ppucErrorString == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppucErrorString' parameter passed to 'iLwpsIndexFieldInfoRequestHandle'."); 
        return (LWPS_ReturnParameterError);
    }


    /* Create a reference ID */
    snprintf(pucSentReferenceID, LWPS_REFERENCE_ID_BUFFER_LENGTH + 1, "%u", plLwps->uiReferenceID);

    
    /* Increment the reference ID */
    plLwps->uiReferenceID++;


    /* Clear the returned error code */
    *piErrorCode = SPI_NoError;
    

    /* Send the index field info request */
    if ( (iError = iLwpsIndexFieldInfoRequestSend(plLwps, pucIndexName, pucSentReferenceID)) != LWPS_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to send an lwps index field information request message, lwps error: %d.", iError);
        goto bailFromiLwpsIndexFieldInfoRequestHandle;
    }


    /* Receive the data */
    if ( (iError = iUtlNetReceive(plLwps->pvUtlNet)) != UTL_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to receive data, utl error: %d.", iError);
        iError = iLwpsMapUtlNetError(iError, LWPS_FailedReadData);
        goto bailFromiLwpsIndexFieldInfoRequestHandle;
    }


    /* Peek the header */
    if ( (iError = iLwpsHeaderPeek(pvLwps, &uiMessageID)) != LWPS_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to peek the lwps header, lwps error: %d.", iError);
        goto bailFromiLwpsIndexFieldInfoRequestHandle;
    }


    /* Handle the message */
    if ( uiMessageID == LWPS_INDEX_FIELD_INFO_RESPONSE_ID ) {

        /* Receive the index field info response */
        if ( (iError = iLwpsIndexFieldInfoResponseReceive(plLwps, ppsfiSpiFieldInfos, puiSpiFieldInfosLength, &pucReturnedReferenceID)) != LWPS_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to receive an lwps index field information response message, lwps error: %d.", iError);
            goto bailFromiLwpsIndexFieldInfoRequestHandle;
        }
    }
    else if ( uiMessageID == LWPS_ERROR_MESSAGE_ID ) {

        /* Receive the error message */
        if ( (iError = iLwpsErrorMessageReceive(plLwps, piErrorCode, ppucErrorString, &pucReturnedReferenceID)) != LWPS_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to receive an lwps error message, lwps error: %d.", iError);
            goto bailFromiLwpsIndexFieldInfoRequestHandle;
        }
    }
    else {
        iError = LWPS_InvalidMessageID;
        goto bailFromiLwpsIndexFieldInfoRequestHandle;
    }


    /* Check that the reference ID was correctly returned */
    if ( !((bUtlStringsIsStringNULL(pucReturnedReferenceID) == false) && (s_strcmp(pucReturnedReferenceID, pucSentReferenceID) == 0)) ) {
        iError = LWPS_InvalidReferenceID;
        goto bailFromiLwpsIndexFieldInfoRequestHandle;
    }



    /* Bail label */
    bailFromiLwpsIndexFieldInfoRequestHandle:


    /* Free allocated pointers */
    s_free(pucReturnedReferenceID);


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLwpsIndexFieldInfoResponseSend()

    Purpose:    Send a field info response to a client

    Parameters: pvLwps                  lwps structure
                psfiSpiFieldInfos       array of spi field info structures (optional)
                uiSpiFieldInfosLength   number of entries in the spi field info structure array
                pucReferenceID          reference ID (optional)

    Globals:    none

    Returns:    LWPS error code

*/
int iLwpsIndexFieldInfoResponseSend
(
    void *pvLwps,
    struct spiFieldInfo *psfiSpiFieldInfos,
    unsigned int uiSpiFieldInfosLength,
    unsigned char *pucReferenceID
)
{

    int                     iError = LWPS_NoError;
    struct lwps             *plLwps = (struct lwps *)pvLwps;
    unsigned int            uiI = 0;
    struct spiFieldInfo     *psfiSpiFieldInfosPtr = NULL;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iLwpsIndexFieldInfoResponseSend");  */


    /* Check the parameters */
    if ( pvLwps == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvLwps' parameter passed to 'iLwpsIndexFieldInfoResponseSend'."); 
        return (LWPS_InvalidLwps);
    }

    if ( uiSpiFieldInfosLength < 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiSpiFieldInfosLength' parameter passed to 'iLwpsIndexFieldInfoResponseSend'."); 
        return (LWPS_ParameterError);
    }

    if ( (psfiSpiFieldInfos == NULL) && (uiSpiFieldInfosLength > 0) ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'psfiSpiFieldInfos' & 'uiSpiFieldInfosLength' parameters passed to 'iLwpsIndexFieldInfoResponseSend'."); 
        return (LWPS_ParameterError);
    }


    /* Send the reply header */
    if ( (iError = iLwpsHeaderSend(plLwps, LWPS_INDEX_FIELD_INFO_RESPONSE_ID)) != LWPS_NoError ) {
        goto bailFromiLwpsIndexFieldInfoResponseSend;
    }


    /* Send the number of entries in the field info array */
    if ( (iError = iLwpsIntegerWrite(plLwps, uiSpiFieldInfosLength)) != LWPS_NoError ) {
        goto bailFromiLwpsIndexFieldInfoResponseSend;
    }


    /* Cycle through the field info entries and send them */
    for ( uiI = 0, psfiSpiFieldInfosPtr = psfiSpiFieldInfos; uiI < uiSpiFieldInfosLength; uiI++, psfiSpiFieldInfosPtr++ ) { 

        /* Send the field name */
        if ( (iError = iLwpsStringWrite(plLwps, psfiSpiFieldInfosPtr->pucName)) != LWPS_NoError ) {
            goto bailFromiLwpsIndexFieldInfoResponseSend;
        }

        /* Send the field description */
        if ( (iError = iLwpsStringWrite(plLwps, psfiSpiFieldInfosPtr->pucDescription)) != LWPS_NoError ) {
            goto bailFromiLwpsIndexFieldInfoResponseSend;
        }

        /* Send the field type */
        if ( (iError = iLwpsIntegerWrite(plLwps, psfiSpiFieldInfosPtr->uiType)) != LWPS_NoError ) {
            goto bailFromiLwpsIndexFieldInfoResponseSend;
        }
    }


    /* Send the reference ID string */
    if ( (iError = iLwpsStringWrite(plLwps, pucReferenceID)) != LWPS_NoError ) {
        goto bailFromiLwpsIndexFieldInfoResponseSend;
    }



    /* Bail label */
    bailFromiLwpsIndexFieldInfoResponseSend:


    /* Handle the error */
    if ( iError == LWPS_NoError ) {
    
        /* Send the data */
        if ( (iError = iUtlNetSend(plLwps->pvUtlNet)) != UTL_NoError ) {
            iError = iLwpsMapUtlNetError(iError, LWPS_FailedWriteData);
        }
    }
#if defined(LWPS_ENABLE_CLEAR_SEND_BUFFER_ON_SEND_ERROR)
    else {

        /* Reset the send buffer to clear it */
        if ( (iError = iUtlNetResetSend(plLwps->pvUtlNet)) != UTL_NoError ) {
            iError = iLwpsMapUtlNetError(iError, LWPS_FailedWriteData);
        }
    }
#endif    /* defined(LWPS_ENABLE_CLEAR_SEND_BUFFER_ON_SEND_ERROR) */


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLwpsIndexFieldInfoResponseReceive()

    Purpose:    Receive a field info response from a server

    Parameters: pvLwps                      lwps structure
                ppsfiSpiFieldInfos          return pointer for the array of spi field info structures (allocated)
                puiSpiFieldInfosLength      return pointer for the number of entries in the spi field info structure array (allocated)
                ppucReferenceID             return pointer for the reference ID (allocated)

    Globals:    none

    Returns:    LWPS error code

*/
int iLwpsIndexFieldInfoResponseReceive
(
    void *pvLwps,
    struct spiFieldInfo **ppsfiSpiFieldInfos,
    unsigned int *puiSpiFieldInfosLength,
    unsigned char **ppucReferenceID
)
{

    int                     iError = LWPS_NoError;
    struct lwps             *plLwps = (struct lwps *)pvLwps;
    unsigned int            uiMessageID = LWPS_INVALID_ID;
    struct spiFieldInfo     *psfiSpiFieldInfos = NULL;
    unsigned int            uiSpiFieldInfosLength = 0;
    unsigned char           *pucReferenceID = NULL;
    unsigned int            uiI = 0;
    struct spiFieldInfo     *psfiSpiFieldInfosPtr = NULL;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iLwpsIndexFieldInfoResponseReceive");  */


    /* Check the parameters */
    if ( pvLwps == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvLwps' parameter passed to 'iLwpsIndexFieldInfoResponseReceive'."); 
        return (LWPS_InvalidLwps);
    }

    if ( ppsfiSpiFieldInfos == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppsfiSpiFieldInfos' parameter passed to 'iLwpsIndexFieldInfoResponseReceive'."); 
        return (LWPS_ReturnParameterError);
    }

    if ( puiSpiFieldInfosLength == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'puiSpiFieldInfosLength' parameter passed to 'iLwpsIndexFieldInfoResponseReceive'."); 
        return (LWPS_ReturnParameterError);
    }

    if ( ppucReferenceID == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppucReferenceID' parameter passed to 'iLwpsIndexFieldInfoResponseReceive'."); 
        return (LWPS_ReturnParameterError);
    }


    /* Peek the response header, we do this here because we don't want 
    ** to harm the response if it is not what we expected
    */
    if ( (iError = iLwpsHeaderPeek(plLwps, &uiMessageID)) != LWPS_NoError ) {
        goto bailFromiLwpsIndexFieldInfoResponseReceive;
    }


    /* Check that the response header message tag is correct */
    if ( uiMessageID != LWPS_INDEX_FIELD_INFO_RESPONSE_ID) {
        iError = LWPS_InvalidMessageID;
        goto bailFromiLwpsIndexFieldInfoResponseReceive;
    }


    /* Read the response header */
    if ( (iError = iLwpsHeaderReceive(plLwps, &uiMessageID)) != LWPS_NoError ) {
        goto bailFromiLwpsIndexFieldInfoResponseReceive;
    }


    /* Read the number of entries in the field info array */
    if ( (iError = iLwpsIntegerRead(plLwps, &uiSpiFieldInfosLength)) != LWPS_NoError ) {
        goto bailFromiLwpsIndexFieldInfoResponseReceive;
    }


    /* Read the field info if there is any */
    if ( uiSpiFieldInfosLength > 0 ) {

        /* Allocate space for the field info array */
        if ( (psfiSpiFieldInfos = (struct spiFieldInfo *)s_malloc((size_t)(sizeof(struct spiFieldInfo) * uiSpiFieldInfosLength))) == NULL ) {
            iError = LWPS_MemError;
            goto bailFromiLwpsIndexFieldInfoResponseReceive;
        }


        /* Cycle through the field info entries and read them */
        for ( uiI = 0, psfiSpiFieldInfosPtr = psfiSpiFieldInfos; uiI < uiSpiFieldInfosLength; uiI++, psfiSpiFieldInfosPtr++ ) { 

            /* Read the field name */
            if ( (iError = iLwpsStringRead(plLwps, &psfiSpiFieldInfosPtr->pucName)) != LWPS_NoError ) {
                goto bailFromiLwpsIndexFieldInfoResponseReceive;
            }

            /* Read the field description */
            if ( (iError = iLwpsStringRead(plLwps, &psfiSpiFieldInfosPtr->pucDescription)) != LWPS_NoError ) {
                goto bailFromiLwpsIndexFieldInfoResponseReceive;
            }

            /* Read the field type */
            if ( (iError = iLwpsIntegerRead(plLwps, &psfiSpiFieldInfosPtr->uiType)) != LWPS_NoError ) {
                goto bailFromiLwpsIndexFieldInfoResponseReceive;
            }
        }
    }


    /* Read the reference ID */
    if ( (iError = iLwpsStringRead(plLwps, &pucReferenceID)) != LWPS_NoError ) {
        goto bailFromiLwpsIndexFieldInfoResponseReceive;
    }



    /* Bail label */
    bailFromiLwpsIndexFieldInfoResponseReceive:


    /* Handle the error */
    if ( iError == LWPS_NoError ) {

        /* Set the return pointers */
        *ppsfiSpiFieldInfos = psfiSpiFieldInfos;
        *puiSpiFieldInfosLength = uiSpiFieldInfosLength;
        *ppucReferenceID = pucReferenceID;
    }
    else {

        /* Free allocations */
        iSpiFreeIndexFieldInfo(psfiSpiFieldInfos, uiSpiFieldInfosLength);
        psfiSpiFieldInfos = NULL;

        s_free(pucReferenceID);
    }


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLwpsIndexFieldInfoResponseHandle()

    Purpose:    Handle the field info response by receiving an 
                field info request from a client and sending the response
    
    Parameters: pvLwps      lwps structure

    Globals:    none

    Returns:    LWPS error code

*/
int iLwpsIndexFieldInfoResponseHandle
(
    void *pvLwps
)
{

    int                     iError = LWPS_NoError;
    int                     iErrorCode = SPI_NoError;
    unsigned char           *pucErrorString = NULL;
    struct lwps             *plLwps = (struct lwps *)pvLwps;
    unsigned char           *pucIndexName = NULL;
    struct spiFieldInfo     *psfiSpiFieldInfos = NULL;
    unsigned int            uiSpiFieldInfosLength = 0;
    unsigned char           *pucReferenceID = NULL;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iLwpsIndexFieldInfoResponseHandle");  */


    /* Check the parameters */
    if ( pvLwps == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvLwps' parameter passed to 'iLwpsIndexFieldInfoResponseHandle'."); 
        return (LWPS_InvalidLwps);
    }


    /* Receive the index field info request */
    if ( (iError = iLwpsIndexFieldInfoRequestReceive(plLwps, &pucIndexName, &pucReferenceID)) != LWPS_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to receive an lwps field information request message, lwps error: %d.", iError);
        goto bailFromiLwpsIndexFieldInfoResponseHandle;
    }


    /* Put in your index field info code here */
/*     iErrorCode = myIndexFieldInfoCode(); */
    
    
    /* Handle the index field info error */
    if ( iErrorCode == SPI_NoError ) {

        /* Send the index field info response */
        if ( (iError = iLwpsIndexFieldInfoResponseSend(plLwps, psfiSpiFieldInfos, uiSpiFieldInfosLength, pucReferenceID)) != LWPS_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to send an lwps index field information response message, lwps error: %d.", iError);
            goto bailFromiLwpsIndexFieldInfoResponseHandle;
        }
    }
    else {

        /* Send the error */
        if ( (iError = iLwpsErrorMessageSend(plLwps, iErrorCode, pucErrorString, pucReferenceID)) != LWPS_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to send an lwps error message, lwps error: %d.", iError);
            goto bailFromiLwpsIndexFieldInfoResponseHandle;
        }
    }



    /* Bail label */
    bailFromiLwpsIndexFieldInfoResponseHandle:


    /* Free allocated pointers */
    s_free(pucIndexName);
    
    iSpiFreeIndexFieldInfo(psfiSpiFieldInfos, uiSpiFieldInfosLength);
    psfiSpiFieldInfos = NULL;

    s_free(pucReferenceID);


    return (iError);


}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLwpsIndexTermInfoRequestSend()

    Purpose:    Send a index term info request to a server

    Parameters: pvLwps              lwps structure
                pucIndexName        index name (optional)
                uiTermMatch         term match
                uiTermCase          term case
                pucTerm             term (optional)
                pucFieldName        field name (optional)
                pucReferenceID      reference ID (optional)

    Globals:    none

    Returns:    LWPS error code

*/
int iLwpsIndexTermInfoRequestSend
(
    void *pvLwps,
    unsigned char *pucIndexName,
    unsigned int uiTermMatch,
    unsigned int uiTermCase,
    unsigned char *pucTerm,
    unsigned char *pucFieldName,
    unsigned char *pucReferenceID
)
{

    int             iError = LWPS_NoError;
    struct lwps     *plLwps = (struct lwps *)pvLwps;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iLwpsIndexTermInfoRequestSend");  */


    /* Check the parameters */
    if ( pvLwps == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvLwps' parameter passed to 'iLwpsIndexTermInfoRequestSend'."); 
        return (LWPS_InvalidLwps);
    }

    if ( SPI_TERM_MATCH_VALID(uiTermMatch) == false ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiTermMatch' parameter passed to 'iLwpsIndexTermInfoRequestSend'."); 
        return (LWPS_ParameterError);
    }

    if ( SPI_TERM_CASE_VALID(uiTermCase) == false ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiTermCase' parameter passed to 'iLwpsIndexTermInfoRequestSend'."); 
        return (LWPS_ParameterError);
    }


    /* Send the message header */
    if ( (iError = iLwpsHeaderSend(plLwps, LWPS_INDEX_TERM_INFO_REQUEST_ID)) != LWPS_NoError ) {
        goto bailFromiLwpsIndexTermInfoRequestSend;
    }


    /* Send the index name */
    if ( (iError = iLwpsStringWrite(plLwps, pucIndexName)) != LWPS_NoError ) {
        goto bailFromiLwpsIndexTermInfoRequestSend;
    }


    /* Send the term match */
    if ( (iError = iLwpsIntegerWrite(plLwps, uiTermMatch)) != LWPS_NoError ) {
        goto bailFromiLwpsIndexTermInfoRequestSend;
    }


    /* Send the term case */
    if ( (iError = iLwpsIntegerWrite(plLwps, uiTermCase)) != LWPS_NoError ) {
        goto bailFromiLwpsIndexTermInfoRequestSend;
    }


    /* Send the term */
    if ( (iError = iLwpsStringWrite(plLwps, pucTerm)) != LWPS_NoError ) {
        goto bailFromiLwpsIndexTermInfoRequestSend;
    }


    /* Send the field name */
    if ( (iError = iLwpsStringWrite(plLwps, pucFieldName)) != LWPS_NoError ) {
        goto bailFromiLwpsIndexTermInfoRequestSend;
    }


    /* Send the reference ID string */
    if ( (iError = iLwpsStringWrite(plLwps, pucReferenceID)) != LWPS_NoError ) {
        goto bailFromiLwpsIndexTermInfoRequestSend;
    }



    /* Bail label */
    bailFromiLwpsIndexTermInfoRequestSend:


    /* Handle the error */
    if ( iError == LWPS_NoError ) {
    
        /* Send the data */
        if ( (iError = iUtlNetSend(plLwps->pvUtlNet)) != UTL_NoError ) {
            iError = iLwpsMapUtlNetError(iError, LWPS_FailedWriteData);
        }
    }
#if defined(LWPS_ENABLE_CLEAR_SEND_BUFFER_ON_SEND_ERROR)
    else {

        /* Reset the send buffer to clear it */
        if ( (iError = iUtlNetResetSend(plLwps->pvUtlNet)) != UTL_NoError ) {
            iError = iLwpsMapUtlNetError(iError, LWPS_FailedWriteData);
        }
    }
#endif    /* defined(LWPS_ENABLE_CLEAR_SEND_BUFFER_ON_SEND_ERROR) */


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLwpsIndexTermInfoRequestReceive()

    Purpose:    Receive a index term info request from a client, this
                function assumes that the header has already been read

    Parameters: pvLwps              lwps structure
                ppucIndexName       return pointer for the index name (allocated)
                puiTermType         return pointer for the term type
                puiTermCase         return pointer for the term case
                ppucTerm            return pointer for the term (allocated)
                ppucFieldName       return pointer for the field name (allocated)
                ppucReferenceID     return pointer for the reference ID (allocated)

    Globals:    none

    Returns:    LWPS error code

*/
int iLwpsIndexTermInfoRequestReceive
(
    void *pvLwps,
    unsigned char **ppucIndexName,
    unsigned int *puiTermType,
    unsigned int *puiTermCase,
    unsigned char **ppucTerm,
    unsigned char **ppucFieldName,
    unsigned char **ppucReferenceID
)
{

    int             iError = LWPS_NoError;
    struct lwps     *plLwps = (struct lwps *)pvLwps;
    unsigned int    uiMessageID = LWPS_INVALID_ID;
    unsigned int    uiTermMatch = SPI_TERM_MATCH_UNKNOWN;
    unsigned int    uiTermCase = SPI_TERM_CASE_UNKNOWN;
    unsigned char   *pucIndexName = NULL;
    unsigned char   *pucTerm = NULL;
    unsigned char   *pucFieldName = NULL;
    unsigned char   *pucReferenceID = NULL;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iLwpsIndexTermInfoRequestReceive");  */


    /* Check the parameters */
    if ( pvLwps == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvLwps' parameter passed to 'iLwpsIndexTermInfoRequestReceive'."); 
        return (LWPS_InvalidLwps);
    }

    if ( ppucIndexName == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppucIndexName' parameter passed to 'iLwpsIndexTermInfoRequestReceive'."); 
        return (LWPS_ReturnParameterError);
    }

    if ( puiTermType == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'puiTermType' parameter passed to 'iLwpsIndexTermInfoRequestReceive'."); 
        return (LWPS_ReturnParameterError);
    }

    if ( puiTermCase == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'puiTermCase' parameter passed to 'iLwpsIndexTermInfoRequestReceive'."); 
        return (LWPS_ReturnParameterError);
    }

    if ( ppucTerm == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppucTerm' parameter passed to 'iLwpsIndexTermInfoRequestReceive'."); 
        return (LWPS_ReturnParameterError);
    }

    if ( ppucFieldName == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppucFieldName' parameter passed to 'iLwpsIndexTermInfoRequestReceive'."); 
        return (LWPS_ReturnParameterError);
    }

    if ( ppucReferenceID == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppucReferenceID' parameter passed to 'iLwpsIndexTermInfoRequestReceive'."); 
        return (LWPS_ReturnParameterError);
    }


    /* Peek the response header, we do this here because we don't want 
    ** to harm the response if it is not what we expected
    */
    if ( (iError = iLwpsHeaderPeek(plLwps, &uiMessageID)) != LWPS_NoError ) {
        goto bailFromiLwpsIndexTermInfoRequestReceive;
    }


    /* Check that the response header message tag is correct */
    if ( uiMessageID != LWPS_INDEX_TERM_INFO_REQUEST_ID) {
        iError = LWPS_InvalidMessageID;
        goto bailFromiLwpsIndexTermInfoRequestReceive;
    }


    /* Read the response header */
    if ( (iError = iLwpsHeaderReceive(plLwps, &uiMessageID)) != LWPS_NoError ) {
        goto bailFromiLwpsIndexTermInfoRequestReceive;
    }


    /* Read the index name */
    if ( (iError = iLwpsStringRead(plLwps, &pucIndexName)) != LWPS_NoError ) {
        goto bailFromiLwpsIndexTermInfoRequestReceive;
    }


    /* Read the term match */
    if ( (iError = iLwpsIntegerRead(plLwps, &uiTermMatch)) != LWPS_NoError ) {
        goto bailFromiLwpsIndexTermInfoRequestReceive;
    }


    /* Read the term case */
    if ( (iError = iLwpsIntegerRead(plLwps, &uiTermCase)) != LWPS_NoError ) {
        goto bailFromiLwpsIndexTermInfoRequestReceive;
    }


    /* Read the term */
    if ( (iError = iLwpsStringRead(plLwps, &pucTerm)) != LWPS_NoError ) {
        goto bailFromiLwpsIndexTermInfoRequestReceive;
    }


    /* Read the field name */
    if ( (iError = iLwpsStringRead(plLwps, &pucFieldName)) != LWPS_NoError ) {
        goto bailFromiLwpsIndexTermInfoRequestReceive;
    }


    /* Read the reference ID */
    if ( (iError = iLwpsStringRead(plLwps, &pucReferenceID)) != LWPS_NoError ) {
        goto bailFromiLwpsIndexTermInfoRequestReceive;
    }



    /* Bail label */
    bailFromiLwpsIndexTermInfoRequestReceive:


    /* Handle the error */
    if ( iError == LWPS_NoError ) {

        /* Set the return pointers */
        *ppucIndexName = pucIndexName;
        *puiTermType = uiTermMatch;
        *puiTermCase = uiTermCase;
        *ppucTerm = pucTerm;
        *ppucFieldName = pucFieldName;
        *ppucReferenceID = pucReferenceID;
    }    
    else {

        /* Free allocations */
        s_free(pucIndexName);
        s_free(pucTerm);
        s_free(pucFieldName);
        s_free(pucReferenceID);
    }


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLwpsIndexTermInfoRequestHandle()

    Purpose:    Handle a index term info request by sending a 
                index term info request to a server and receiving the response

    Parameters: pvLwps                  lwps structure
                pucIndexName            index name (optional)
                uiTermMatch             term match
                uiTermCase              term case
                pucTerm                 term (optional)
                pucFieldName            field name (optional)
                ppstiSpiTermInfos       return pointer for the array of spi index term info structures (allocated)
                puiSpiTermInfosLength   return pointer for the number of entries in the spi index term info structure array (allocated)
                piErrorCode             return pointer for the error code
                ppucErrorString         return pointer for the error string (allocated)

    Globals:    none

    Returns:    LWPS error code

*/
int iLwpsIndexTermInfoRequestHandle
(
    void *pvLwps,
    unsigned char *pucIndexName,
    unsigned int uiTermMatch,
    unsigned int uiTermCase,
    unsigned char *pucTerm,
    unsigned char *pucFieldName,
    struct spiTermInfo **ppstiSpiTermInfos,
    unsigned int *puiSpiTermInfosLength,
    int *piErrorCode,
    unsigned char **ppucErrorString
)
{

    int             iError = LWPS_NoError;
    struct lwps     *plLwps = (struct lwps *)pvLwps;
    unsigned int    uiMessageID = LWPS_INVALID_ID;
    unsigned char   pucSentReferenceID[LWPS_REFERENCE_ID_BUFFER_LENGTH + 1] = {'\0'};
    unsigned char   *pucReturnedReferenceID = NULL;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iLwpsIndexTermInfoRequestHandle");  */


    /* Check the parameters */
    if ( pvLwps == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvLwps' parameter passed to 'iLwpsIndexTermInfoRequestHandle'."); 
        return (LWPS_InvalidLwps);
    }

    if ( SPI_TERM_MATCH_VALID(uiTermMatch) == false ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiTermMatch' parameter passed to 'iLwpsIndexTermInfoRequestHandle'."); 
        return (LWPS_ParameterError);
    }

    if ( SPI_TERM_CASE_VALID(uiTermCase) == false ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiTermCase' parameter passed to 'iLwpsIndexTermInfoRequestHandle'."); 
        return (LWPS_ParameterError);
    }

    if ( ppstiSpiTermInfos == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppstiSpiTermInfos' parameter passed to 'iLwpsIndexTermInfoRequestHandle'."); 
        return (LWPS_ReturnParameterError);
    }

    if ( puiSpiTermInfosLength == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'puiSpiTermInfosLength' parameter passed to 'iLwpsIndexTermInfoRequestHandle'."); 
        return (LWPS_ReturnParameterError);
    }

    if ( piErrorCode == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'piErrorCode' parameter passed to 'iLwpsIndexTermInfoRequestHandle'."); 
        return (LWPS_ReturnParameterError);
    }

    if ( ppucErrorString == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppucErrorString' parameter passed to 'iLwpsIndexTermInfoRequestHandle'."); 
        return (LWPS_ReturnParameterError);
    }


    /* Create a reference ID */
    snprintf(pucSentReferenceID, LWPS_REFERENCE_ID_BUFFER_LENGTH + 1, "%u", plLwps->uiReferenceID);

    
    /* Increment the reference ID */
    plLwps->uiReferenceID++;


    /* Clear the returned error code */
    *piErrorCode = SPI_NoError;
    

    /* Send the index term info request */
    if ( (iError = iLwpsIndexTermInfoRequestSend(plLwps, pucIndexName, uiTermMatch, uiTermCase, pucTerm, pucFieldName, 
            pucSentReferenceID)) != LWPS_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to send an lwps index term information request message, lwps error: %d.", iError);
        goto bailFromiLwpsIndexTermInfoRequestHandle;
    }


    /* Receive the data */
    if ( (iError = iUtlNetReceive(plLwps->pvUtlNet)) != UTL_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to receive data, utl error: %d.", iError);
        iError = iLwpsMapUtlNetError(iError, LWPS_FailedReadData);
        goto bailFromiLwpsIndexTermInfoRequestHandle;
    }


    /* Peek the header */
    if ( (iError = iLwpsHeaderPeek(pvLwps, &uiMessageID)) != LWPS_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to peek the lwps header, lwps error: %d.", iError);
        goto bailFromiLwpsIndexTermInfoRequestHandle;
    }


    /* Handle the message */
    if ( uiMessageID == LWPS_INDEX_TERM_INFO_RESPONSE_ID ) {

        /* Receive the index term info response */
        if ( (iError = iLwpsIndexTermInfoResponseReceive(plLwps, ppstiSpiTermInfos, puiSpiTermInfosLength, &pucReturnedReferenceID)) != LWPS_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to receive an lwps index term information response message, lwps error: %d.", iError);
            goto bailFromiLwpsIndexTermInfoRequestHandle;
        }
    }
    else if ( uiMessageID == LWPS_ERROR_MESSAGE_ID ) {

        /* Receive the error message */
        if ( (iError = iLwpsErrorMessageReceive(plLwps, piErrorCode, ppucErrorString, &pucReturnedReferenceID)) != LWPS_NoError ) {
            goto bailFromiLwpsIndexTermInfoRequestHandle;
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to receive an lwps error message, lwps error: %d.", iError);
        }
    }
    else {
        iError = LWPS_InvalidMessageID;
        goto bailFromiLwpsIndexTermInfoRequestHandle;
    }


    /* Check that the reference ID was correctly returned */
    if ( !((bUtlStringsIsStringNULL(pucReturnedReferenceID) == false) && (s_strcmp(pucReturnedReferenceID, pucSentReferenceID) == 0)) ) {
        iError = LWPS_InvalidReferenceID;
        goto bailFromiLwpsIndexTermInfoRequestHandle;
    }



    /* Bail label */
    bailFromiLwpsIndexTermInfoRequestHandle:


    /* Free allocated pointers */
    s_free(pucReturnedReferenceID);


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLwpsIndexTermInfoResponseSend()

    Purpose:    Send a index term info response to a client

    Parameters: pvLwps                  lwps structure
                pstiSpiTermInfos        array of spi index term info structures (optional)
                uiSpiTermInfosLength    number of entries in the spi index term info structure array
                pucReferenceID          reference ID (optional)

    Globals:    none

    Returns:    LWPS error code

*/
int iLwpsIndexTermInfoResponseSend
(
    void *pvLwps,
    struct spiTermInfo *pstiSpiTermInfos,
    unsigned int uiSpiTermInfosLength,
    unsigned char *pucReferenceID
)
{

    int                     iError = LWPS_NoError;
    struct lwps             *plLwps = (struct lwps *)pvLwps;
    unsigned int            uiI = 0;
    struct spiTermInfo      *pstiSpiTermInfosPtr = NULL;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iLwpsIndexTermInfoResponseSend");  */


    /* Check the parameters */
    if ( pvLwps == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvLwps' parameter passed to 'iLwpsIndexTermInfoResponseSend'."); 
        return (LWPS_InvalidLwps);
    }

    if ( uiSpiTermInfosLength < 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiSpiTermInfosLength' parameter passed to 'iLwpsIndexTermInfoResponseSend'."); 
        return (LWPS_ParameterError);
    }

    if ( (pstiSpiTermInfos == NULL) && (uiSpiTermInfosLength > 0) ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'pstiSpiTermInfos' & 'uiSpiTermInfosLength' parameters passed to 'iLwpsIndexTermInfoResponseSend'."); 
        return (LWPS_ParameterError);
    }


    /* Send the reply header */
    if ( (iError = iLwpsHeaderSend(plLwps, LWPS_INDEX_TERM_INFO_RESPONSE_ID)) != LWPS_NoError ) {
        goto bailFromiLwpsIndexTermInfoResponseSend;
    }


    /* Send the number of entries in the index term info array */
    if ( (iError = iLwpsIntegerWrite(plLwps, uiSpiTermInfosLength)) != LWPS_NoError ) {
        goto bailFromiLwpsIndexTermInfoResponseSend;
    }


    /* Cycle through the index term info entries and send them */
    for ( uiI = 0, pstiSpiTermInfosPtr = pstiSpiTermInfos; uiI < uiSpiTermInfosLength; uiI++, pstiSpiTermInfosPtr++ ) { 

        /* Send the term */
        if ( (iError = iLwpsStringWrite(plLwps, pstiSpiTermInfosPtr->pucTerm)) != LWPS_NoError ) {
            goto bailFromiLwpsIndexTermInfoResponseSend;
        }

        /* Send the term type */
        if ( (iError = iLwpsIntegerWrite(plLwps, pstiSpiTermInfosPtr->uiType)) != LWPS_NoError) {
            goto bailFromiLwpsIndexTermInfoResponseSend;
        }

        /* Send the term count */
        if ( (iError = iLwpsIntegerWrite(plLwps, pstiSpiTermInfosPtr->uiCount)) != LWPS_NoError ) {
            goto bailFromiLwpsIndexTermInfoResponseSend;
        }

        /* Send the document count */
        if ( (iError = iLwpsIntegerWrite(plLwps, pstiSpiTermInfosPtr->uiDocumentCount)) != LWPS_NoError ) {
            goto bailFromiLwpsIndexTermInfoResponseSend;
        }
    }


    /* Send the reference ID string */
    if ( (iError = iLwpsStringWrite(plLwps, pucReferenceID)) != LWPS_NoError ) {
        goto bailFromiLwpsIndexTermInfoResponseSend;
    }



    /* Bail label */
    bailFromiLwpsIndexTermInfoResponseSend:


    /* Handle the error */
    if ( iError == LWPS_NoError ) {
    
        /* Send the data */
        if ( (iError = iUtlNetSend(plLwps->pvUtlNet)) != UTL_NoError ) {
            iError = iLwpsMapUtlNetError(iError, LWPS_FailedWriteData);
        }
    }
#if defined(LWPS_ENABLE_CLEAR_SEND_BUFFER_ON_SEND_ERROR)
    else {

        /* Reset the send buffer to clear it */
        if ( (iError = iUtlNetResetSend(plLwps->pvUtlNet)) != UTL_NoError ) {
            iError = iLwpsMapUtlNetError(iError, LWPS_FailedWriteData);
        }
    }
#endif    /* defined(LWPS_ENABLE_CLEAR_SEND_BUFFER_ON_SEND_ERROR) */


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLwpsIndexTermInfoResponseReceive()

    Purpose:    Receive a index term info response from a server

    Parameters: pvLwps                  lwps structure
                ppstiSpiTermInfos       return pointer for the array of spi index term info structures (allocated)
                puiSpiTermInfosLength   return pointer for the number of entries in the spi index term info structure array
                ppucReferenceID         return pointer for the reference ID (allocated)

    Globals:    none

    Returns:    LWPS error code

*/
int iLwpsIndexTermInfoResponseReceive
(
    void *pvLwps,
    struct spiTermInfo **ppstiSpiTermInfos,
    unsigned int *puiSpiTermInfosLength,
    unsigned char **ppucReferenceID
)
{

    int                     iError = LWPS_NoError;
    struct lwps             *plLwps = (struct lwps *)pvLwps;
    unsigned int            uiMessageID = LWPS_INVALID_ID;
    struct spiTermInfo      *pstiSpiTermInfos = NULL;
    unsigned int            uiSpiTermInfosLength = 0;
    unsigned char           *pucReferenceID = NULL;
    unsigned int            uiI = 0;
    struct spiTermInfo      *pstiSpiTermInfosPtr = NULL;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iLwpsIndexTermInfoResponseReceive");  */


    /* Check the parameters */
    if ( pvLwps == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvLwps' parameter passed to 'iLwpsIndexTermInfoResponseReceive'."); 
        return (LWPS_InvalidLwps);
    }

    if ( ppstiSpiTermInfos == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppstiSpiTermInfos' parameter passed to 'iLwpsIndexTermInfoResponseReceive'."); 
        return (LWPS_ReturnParameterError);
    }

    if ( puiSpiTermInfosLength == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'puiSpiTermInfosLength' parameter passed to 'iLwpsIndexTermInfoResponseReceive'."); 
        return (LWPS_ReturnParameterError);
    }

    if ( ppucReferenceID == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppucReferenceID' parameter passed to 'iLwpsIndexTermInfoResponseReceive'."); 
        return (LWPS_ReturnParameterError);
    }


    /* Peek the response header, we do this here because we don't want 
    ** to harm the response if it is not what we expected
    */
    if ( (iError = iLwpsHeaderPeek(plLwps, &uiMessageID)) != LWPS_NoError ) {
        goto bailFromiLwpsIndexTermInfoResponseReceive;
    }


    /* Check that the response header message tag is correct */
    if ( uiMessageID != LWPS_INDEX_TERM_INFO_RESPONSE_ID) {
        iError = LWPS_InvalidMessageID;
        goto bailFromiLwpsIndexTermInfoResponseReceive;
    }


    /* Read the response header */
    if ( (iError = iLwpsHeaderReceive(plLwps, &uiMessageID)) != LWPS_NoError ) {
        goto bailFromiLwpsIndexTermInfoResponseReceive;
    }


    /* Read the number of entries in the index term info */
    if ( (iError = iLwpsIntegerRead(plLwps, &uiSpiTermInfosLength)) != LWPS_NoError ) {
        goto bailFromiLwpsIndexTermInfoResponseReceive;
    }


    /* Read the terms info */
    if ( uiSpiTermInfosLength > 0 ) {

        /* Allocate space for the index term info array */
        if ( (pstiSpiTermInfos = (struct spiTermInfo *)s_malloc((size_t)(sizeof(struct spiTermInfo) * uiSpiTermInfosLength))) == NULL ) {
            iError = LWPS_MemError;
            goto bailFromiLwpsIndexTermInfoResponseReceive;
        }


        /* Cycle through the index term info entries and read them */
        for ( uiI = 0, pstiSpiTermInfosPtr = pstiSpiTermInfos; uiI < uiSpiTermInfosLength; uiI++, pstiSpiTermInfosPtr++ ) { 

            /* Read the term */
            if ( (iError = iLwpsStringRead(plLwps, &pstiSpiTermInfosPtr->pucTerm)) != LWPS_NoError ) {
                goto bailFromiLwpsIndexTermInfoResponseReceive;
            }


            /* Read the term type */
            if ( (iError = iLwpsIntegerRead(plLwps, &pstiSpiTermInfosPtr->uiType)) != LWPS_NoError ) {
                goto bailFromiLwpsIndexTermInfoResponseReceive;
            }


            /* Read the term count */
            if ( (iError = iLwpsIntegerRead(plLwps, &pstiSpiTermInfosPtr->uiCount)) != LWPS_NoError ) {
                goto bailFromiLwpsIndexTermInfoResponseReceive;
            }


            /* Read the document count */
            if ( (iError = iLwpsIntegerRead(plLwps, &pstiSpiTermInfosPtr->uiDocumentCount)) != LWPS_NoError ) {
                goto bailFromiLwpsIndexTermInfoResponseReceive;
            }
        }
    }


    /* Read the reference ID */
    if ( (iError = iLwpsStringRead(plLwps, &pucReferenceID)) != LWPS_NoError ) {
        goto bailFromiLwpsIndexTermInfoResponseReceive;
    }



    /* Bail label */
    bailFromiLwpsIndexTermInfoResponseReceive:


    /* Handle the error */
    if ( iError == LWPS_NoError ) {

        /* Set the return pointers */
        *ppstiSpiTermInfos = pstiSpiTermInfos;
        *puiSpiTermInfosLength = uiSpiTermInfosLength;
        *ppucReferenceID = pucReferenceID;
    }
    else {

        /* Free allocations */
        iSpiFreeTermInfo(pstiSpiTermInfos, uiSpiTermInfosLength);
        pstiSpiTermInfos = NULL;

        s_free(pucReferenceID);
    }


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLwpsIndexTermInfoResponseHandle()

    Purpose:    Handle the index term info response by receiving an 
                index term info request from a client and sending the response

    Parameters: pvLwps      lwps structure

    Globals:    none

    Returns:    LWPS error code

*/
int iLwpsIndexTermInfoResponseHandle
(
    void *pvLwps
)
{

    int                     iError = LWPS_NoError;
    int                     iErrorCode = SPI_NoError;
    unsigned char           *pucErrorString = NULL;
    struct lwps             *plLwps = (struct lwps *)pvLwps;
    unsigned char           *pucIndexName = NULL;
    unsigned int            uiTermMatch = SPI_TERM_MATCH_UNKNOWN;
    unsigned int            uiTermCase = SPI_TERM_CASE_UNKNOWN;
    unsigned char           *pucTerm = NULL;
    unsigned char           *pucFieldName = NULL;
    struct spiTermInfo      *pstiSpiTermInfos = NULL;
    unsigned int            uiSpiTermInfosLength = 0;
    unsigned char           *pucReferenceID = NULL;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iLwpsIndexTermInfoResponseHandle");  */


    /* Check the parameters */
    if ( pvLwps == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvLwps' parameter passed to 'iLwpsIndexTermInfoResponseHandle'."); 
        return (LWPS_InvalidLwps);
    }


    /* Receive the index term info request */
    if ( (iError = iLwpsIndexTermInfoRequestReceive(plLwps, &pucIndexName, &uiTermMatch, &uiTermCase, &pucTerm, &pucFieldName, &pucReferenceID)) != LWPS_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to receive an lwps index term information request message, lwps error: %d.", iError);
        goto bailFromiLwpsIndexTermInfoResponseHandle;
    }


    /* Put in your index term info code here */
/*     iErrorCode = myIndexTermInfoCode(); */
    
    
    /* Handle the index term info error */
    if ( iErrorCode == SPI_NoError ) {

        /* Send the index term info response */
        if ( (iError = iLwpsIndexTermInfoResponseSend(plLwps, pstiSpiTermInfos, uiSpiTermInfosLength, pucReferenceID)) != LWPS_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to send an lwps index term information response message, lwps error: %d.", iError);
            goto bailFromiLwpsIndexTermInfoResponseHandle;
        }
    }
    else {

        /* Send the error */
        if ( (iError = iLwpsErrorMessageSend(plLwps, iErrorCode, pucErrorString, pucReferenceID)) != LWPS_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to send an lwps error message, lwps error: %d.", iError);
            goto bailFromiLwpsIndexTermInfoResponseHandle;
        }
    }



    /* Bail label */
    bailFromiLwpsIndexTermInfoResponseHandle:


    /* Free allocated pointers */
    s_free(pucIndexName);
    s_free(pucTerm);
    s_free(pucFieldName);

    iSpiFreeTermInfo(pstiSpiTermInfos, uiSpiTermInfosLength);
    pstiSpiTermInfos = NULL;

    s_free(pucReferenceID);


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLwpsDocumentInfoRequestSend()

    Purpose:    Send a document info request to a server

    Parameters: pvLwps              lwps structure
                pucIndexName        index name (optional)
                pucDocumentKey      document key (optional)
                pucReferenceID      reference ID (optional)

    Globals:    none

    Returns:    LWPS error code

*/
int iLwpsDocumentInfoRequestSend
(
    void *pvLwps,
    unsigned char *pucIndexName,
    unsigned char *pucDocumentKey,
    unsigned char *pucReferenceID
)
{

    int             iError = LWPS_NoError;
    struct lwps     *plLwps = (struct lwps *)pvLwps;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iLwpsDocumentInfoRequestSend");  */


    /* Check the parameters */
    if ( pvLwps == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvLwps' parameter passed to 'iLwpsDocumentInfoRequestSend'."); 
        return (LWPS_InvalidLwps);
    }


    /* Send the message header */
    if ( (iError = iLwpsHeaderSend(plLwps, LWPS_DOCUMENT_INFO_REQUEST_ID)) != LWPS_NoError ) {
        goto bailFromiLwpsDocumentInfoRequestSend;
    }


    /* Send the index name */
    if ( (iError = iLwpsStringWrite(plLwps, pucIndexName)) != LWPS_NoError ) {
        goto bailFromiLwpsDocumentInfoRequestSend;
    }


    /* Send the document key */
    if ( (iError = iLwpsStringWrite(plLwps, pucDocumentKey)) != LWPS_NoError ) {
        goto bailFromiLwpsDocumentInfoRequestSend;
    }


    /* Send the reference ID string */
    if ( (iError = iLwpsStringWrite(plLwps, pucReferenceID)) != LWPS_NoError ) {
        goto bailFromiLwpsDocumentInfoRequestSend;
    }



    /* Bail label */
    bailFromiLwpsDocumentInfoRequestSend:


    /* Handle the error */
    if ( iError == LWPS_NoError ) {
    
        /* Send the data */
        if ( (iError = iUtlNetSend(plLwps->pvUtlNet)) != UTL_NoError ) {
            iError = iLwpsMapUtlNetError(iError, LWPS_FailedWriteData);
        }
    }
#if defined(LWPS_ENABLE_CLEAR_SEND_BUFFER_ON_SEND_ERROR)
    else {

        /* Reset the send buffer to clear it */
        if ( (iError = iUtlNetResetSend(plLwps->pvUtlNet)) != UTL_NoError ) {
            iError = iLwpsMapUtlNetError(iError, LWPS_FailedWriteData);
        }
    }
#endif    /* defined(LWPS_ENABLE_CLEAR_SEND_BUFFER_ON_SEND_ERROR) */


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLwpsDocumentInfoRequestReceive()

    Purpose:    Receive a document info request from a client, this
                function assumes that the header has already been read

    Parameters: pvLwps              lwps structure
                ppucIndexName       return pointer for the index name (allocated)
                ppucDocumentKey     return pointer for the document key (allocated)
                ppucReferenceID     return pointer for the reference ID (allocated)

    Globals:    none

    Returns:    LWPS error code

*/
int iLwpsDocumentInfoRequestReceive
(
    void *pvLwps,
    unsigned char **ppucIndexName,
    unsigned char **ppucDocumentKey,
    unsigned char **ppucReferenceID
)
{

    int             iError = LWPS_NoError;
    struct lwps     *plLwps = (struct lwps *)pvLwps;
    unsigned int    uiMessageID = LWPS_INVALID_ID;
    unsigned char   *pucIndexName = NULL;
    unsigned char   *pucDocumentKey = NULL;
    unsigned char   *pucReferenceID = NULL;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iLwpsDocumentInfoRequestReceive");  */


    /* Check the parameters */
    if ( pvLwps == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvLwps' parameter passed to 'iLwpsDocumentInfoRequestReceive'."); 
        return (LWPS_InvalidLwps);
    }

    if ( ppucIndexName == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppucIndexName' parameter passed to 'iLwpsDocumentInfoRequestReceive'."); 
        return (LWPS_ReturnParameterError);
    }

    if ( ppucDocumentKey == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppucDocumentKey' parameter passed to 'iLwpsDocumentInfoRequestReceive'."); 
        return (LWPS_ReturnParameterError);
    }

    if ( ppucReferenceID == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppucReferenceID' parameter passed to 'iLwpsDocumentInfoRequestReceive'."); 
        return (LWPS_ReturnParameterError);
    }


    /* Peek the response header, we do this here because we don't want 
    ** to harm the response if it is not what we expected
    */
    if ( (iError = iLwpsHeaderPeek(plLwps, &uiMessageID)) != LWPS_NoError ) {
        goto bailFromiLwpsDocumentInfoRequestReceive;
    }


    /* Check that the response header message tag is correct */
    if ( uiMessageID != LWPS_DOCUMENT_INFO_REQUEST_ID) {
        iError = LWPS_InvalidMessageID;
        goto bailFromiLwpsDocumentInfoRequestReceive;
    }


    /* Read the response header */
    if ( (iError = iLwpsHeaderReceive(plLwps, &uiMessageID)) != LWPS_NoError ) {
        goto bailFromiLwpsDocumentInfoRequestReceive;
    }


    /* Read the indexbailFromlbLwpsIndexInfoResponseReceive name */
    if ( (iError = iLwpsStringRead(plLwps, &pucIndexName)) != LWPS_NoError ) {
        goto bailFromiLwpsDocumentInfoRequestReceive;
    }


    /* Read the document key */
    if ( (iError = iLwpsStringRead(plLwps, &pucDocumentKey)) != LWPS_NoError ) {
        goto bailFromiLwpsDocumentInfoRequestReceive;
    }


    /* Read the reference ID */
    if ( (iError = iLwpsStringRead(plLwps, &pucReferenceID)) != LWPS_NoError ) {
        goto bailFromiLwpsDocumentInfoRequestReceive;
    }



    /* Bail label */
    bailFromiLwpsDocumentInfoRequestReceive:


    /* Handle the error */
    if ( iError == LWPS_NoError ) {

        /* Set the return pointers */
        *ppucIndexName = pucIndexName;
        *ppucDocumentKey = pucDocumentKey;
        *ppucReferenceID = pucReferenceID;
    }    
    else {

        /* Free allocations */
        s_free(pucIndexName);
        s_free(pucDocumentKey);
        s_free(pucReferenceID);
    }


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLwpsDocumentInfoRequestHandle()

    Purpose:    Handle a document info request by sending a 
                document info request to a server and receiving the response

    Parameters: pvLwps                  lwps structure
                pucIndexName            index name (optional)
                pucDocumentKey          document key (optional)
                ppsdiSpiDocumentInfo    return pointer for the spi document info structure (allocated)
                piErrorCode             return pointer for the error code
                ppucErrorString         return pointer for the error string (allocated)

    Globals:    none

    Returns:    LWPS error code

*/
int iLwpsDocumentInfoRequestHandle
(
    void *pvLwps,
    unsigned char *pucIndexName,
    unsigned char *pucDocumentKey,
    struct spiDocumentInfo **ppsdiSpiDocumentInfo,
    int *piErrorCode,
    unsigned char **ppucErrorString
)
{

    int             iError = LWPS_NoError;
    struct lwps     *plLwps = (struct lwps *)pvLwps;
    unsigned int    uiMessageID = LWPS_INVALID_ID;
    unsigned char   pucSentReferenceID[LWPS_REFERENCE_ID_BUFFER_LENGTH + 1] = {'\0'};
    unsigned char   *pucReturnedReferenceID = NULL;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iLwpsDocumentInfoRequestHandle");  */


    /* Check the parameters */
    if ( pvLwps == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvLwps' parameter passed to 'iLwpsDocumentInfoRequestHandle'."); 
        return (LWPS_InvalidLwps);
    }

    if ( ppsdiSpiDocumentInfo == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppsdiSpiDocumentInfo' parameter passed to 'iLwpsDocumentInfoRequestHandle'."); 
        return (LWPS_ReturnParameterError);
    }

    if ( piErrorCode == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'piErrorCode' parameter passed to 'iLwpsDocumentInfoRequestHandle'."); 
        return (LWPS_ReturnParameterError);
    }

    if ( ppucErrorString == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppucErrorString' parameter passed to 'iLwpsDocumentInfoRequestHandle'."); 
        return (LWPS_ReturnParameterError);
    }


    /* Create a reference ID */
    snprintf(pucSentReferenceID, LWPS_REFERENCE_ID_BUFFER_LENGTH + 1, "%u", plLwps->uiReferenceID);

    
    /* Increment the reference ID */
    plLwps->uiReferenceID++;


    /* Clear the returned error code */
    *piErrorCode = SPI_NoError;
    

    /* Send the document info request */
    if ( (iError = iLwpsDocumentInfoRequestSend(plLwps, pucIndexName, pucDocumentKey, pucSentReferenceID)) != LWPS_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to send an lwps index information request message, lwps error: %d.", iError);
        goto bailFromiLwpsDocumentInfoRequestHandle;
    }


    /* Receive the data */
    if ( (iError = iUtlNetReceive(plLwps->pvUtlNet)) != UTL_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to receive data, utl error: %d.", iError);
        iError = iLwpsMapUtlNetError(iError, LWPS_FailedReadData);
        goto bailFromiLwpsDocumentInfoRequestHandle;
    }


    /* Peek the header */
    if ( (iError = iLwpsHeaderPeek(pvLwps, &uiMessageID)) != LWPS_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to peek the lwps header, lwps error: %d.", iError);
        goto bailFromiLwpsDocumentInfoRequestHandle;
    }


    /* Handle the message */
    if ( uiMessageID == LWPS_DOCUMENT_INFO_RESPONSE_ID ) {

        /* Receive the document info response */
        if ( (iError = iLwpsDocumentInfoResponseReceive(plLwps, ppsdiSpiDocumentInfo, &pucReturnedReferenceID)) != LWPS_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to receive an lwps document information response message, lwps error: %d.", iError);
            goto bailFromiLwpsDocumentInfoRequestHandle;
        }
    }
    else if ( uiMessageID == LWPS_ERROR_MESSAGE_ID ) {

        /* Receive the error message */
        if ( (iError = iLwpsErrorMessageReceive(plLwps, piErrorCode, ppucErrorString, &pucReturnedReferenceID)) != LWPS_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to receive an lwps error message, lwps error: %d.", iError);
            goto bailFromiLwpsDocumentInfoRequestHandle;
        }
    }
    else {
        iError = LWPS_InvalidMessageID;
        goto bailFromiLwpsDocumentInfoRequestHandle;
    }


    /* Check that the reference ID was correctly returned */
    if ( !((bUtlStringsIsStringNULL(pucReturnedReferenceID) == false) && (s_strcmp(pucReturnedReferenceID, pucSentReferenceID) == 0)) ) {
        iError = LWPS_InvalidReferenceID;
        goto bailFromiLwpsDocumentInfoRequestHandle;
    }



    /* Bail label */
    bailFromiLwpsDocumentInfoRequestHandle:


    /* Free allocated pointers */
    s_free(pucReturnedReferenceID);


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLwpsDocumentInfoResponseSend()

    Purpose:    Send a document info response to a client

    Parameters: pvLwps                  lwps structure
                psdiSpiDocumentInfo     spi document info structure
                pucReferenceID          reference ID (optional)

    Globals:    none

    Returns:    LWPS error code

*/
int iLwpsDocumentInfoResponseSend
(
    void *pvLwps,
    struct spiDocumentInfo *psdiSpiDocumentInfo,
    unsigned char *pucReferenceID
)
{

    int                         iError = LWPS_NoError;
    struct lwps                 *plLwps = (struct lwps *)pvLwps;
    unsigned int                uiI = 0;
    struct spiDocumentItem      *psdiSpiDocumentItemsPtr = NULL;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iLwpsDocumentInfoResponseSend");  */


    /* Check the parameters */
    if ( pvLwps == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvLwps' parameter passed to 'iLwpsDocumentInfoResponseSend'."); 
        return (LWPS_InvalidLwps);
    }

    if ( psdiSpiDocumentInfo == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'psdiSpiDocumentInfo' parameter passed to 'iLwpsDocumentInfoResponseSend'."); 
        return (LWPS_ParameterError);
    }


    /* Send the reply header */
    if ( (iError = iLwpsHeaderSend(plLwps, LWPS_DOCUMENT_INFO_RESPONSE_ID)) != LWPS_NoError ) {
        goto bailFromiLwpsDocumentInfoResponseSend;
    }


    /* Send the index name */
    if ( (iError = iLwpsStringWrite(plLwps, psdiSpiDocumentInfo->pucIndexName)) != LWPS_NoError ) {
        goto bailFromiLwpsDocumentInfoResponseSend;
    }

    /* Send the document key */
    if ( (iError = iLwpsStringWrite(plLwps, psdiSpiDocumentInfo->pucDocumentKey)) != LWPS_NoError ) {
        goto bailFromiLwpsDocumentInfoResponseSend;
    }

    /* Send the title */
    if ( (iError = iLwpsStringWrite(plLwps, psdiSpiDocumentInfo->pucTitle)) != LWPS_NoError ) {
        goto bailFromiLwpsDocumentInfoResponseSend;
    }

    /* Send the language code */
    if ( (iError = iLwpsStringWrite(plLwps, psdiSpiDocumentInfo->pucLanguageCode)) != LWPS_NoError ) {
        goto bailFromiLwpsDocumentInfoResponseSend;
    }

    /* Send the rank */
    if ( (iError = iLwpsIntegerWrite(plLwps, psdiSpiDocumentInfo->uiRank)) != LWPS_NoError ) {
        goto bailFromiLwpsDocumentInfoResponseSend;
    }

    /* Send the term count */
    if ( (iError = iLwpsIntegerWrite(plLwps, psdiSpiDocumentInfo->uiTermCount)) != LWPS_NoError ) {
        goto bailFromiLwpsDocumentInfoResponseSend;
    }

    /* Send the date */
    if ( (iError = iLwpsLongWrite(plLwps, psdiSpiDocumentInfo->ulAnsiDate)) != LWPS_NoError ) {
        goto bailFromiLwpsDocumentInfoResponseSend;
    }

    /* Send the number of document items */
    if ( (iError = iLwpsIntegerWrite(plLwps, psdiSpiDocumentInfo->uiDocumentItemsLength)) != LWPS_NoError ) {
        goto bailFromiLwpsDocumentInfoResponseSend;
    }


    /* Cycle through the document items */
    for ( uiI = 0, psdiSpiDocumentItemsPtr = psdiSpiDocumentInfo->psdiSpiDocumentItems; uiI < psdiSpiDocumentInfo->uiDocumentItemsLength; uiI++, psdiSpiDocumentItemsPtr++ ) {

        /* Send the item name */
        if ( (iError = iLwpsStringWrite(plLwps, psdiSpiDocumentItemsPtr->pucItemName)) != LWPS_NoError ) {
            goto bailFromiLwpsDocumentInfoResponseSend;
        }

        /* Send the mime type */
        if ( (iError = iLwpsStringWrite(plLwps, psdiSpiDocumentItemsPtr->pucMimeType)) != LWPS_NoError) {
            goto bailFromiLwpsDocumentInfoResponseSend;
        }

        /* Send the URL */
        if ( (iError = iLwpsStringWrite(plLwps, psdiSpiDocumentItemsPtr->pucUrl)) != LWPS_NoError ) {
            goto bailFromiLwpsDocumentInfoResponseSend;
        }

        /* Send the length */
        if ( (iError = iLwpsIntegerWrite(plLwps, psdiSpiDocumentItemsPtr->uiLength)) != LWPS_NoError ) {
            goto bailFromiLwpsDocumentInfoResponseSend;
        }

        /* Send the data */
        if ( (iError = iLwpsDataWrite(plLwps, (unsigned char *)psdiSpiDocumentItemsPtr->pvData, psdiSpiDocumentItemsPtr->uiDataLength)) != LWPS_NoError ) {
            goto bailFromiLwpsDocumentInfoResponseSend;
        }

        /* Send the data length */
        if ( (iError = iLwpsIntegerWrite(plLwps, psdiSpiDocumentItemsPtr->uiDataLength)) != LWPS_NoError ) {
            goto bailFromiLwpsDocumentInfoResponseSend;
        }
    }


    /* Send the reference ID string */
    if ( (iError = iLwpsStringWrite(plLwps, pucReferenceID)) != LWPS_NoError ) {
        goto bailFromiLwpsDocumentInfoResponseSend;
    }



    /* Bail label */
    bailFromiLwpsDocumentInfoResponseSend:


    /* Handle the error */
    if ( iError == LWPS_NoError ) {
    
        /* Send the data */
        if ( (iError = iUtlNetSend(plLwps->pvUtlNet)) != UTL_NoError ) {
            iError = iLwpsMapUtlNetError(iError, LWPS_FailedWriteData);
        }
    }
#if defined(LWPS_ENABLE_CLEAR_SEND_BUFFER_ON_SEND_ERROR)
    else {

        /* Reset the send buffer to clear it */
        if ( (iError = iUtlNetResetSend(plLwps->pvUtlNet)) != UTL_NoError ) {
            iError = iLwpsMapUtlNetError(iError, LWPS_FailedWriteData);
        }
    }
#endif    /* defined(LWPS_ENABLE_CLEAR_SEND_BUFFER_ON_SEND_ERROR) */


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLwpsDocumentInfoResponseReceive()

    Purpose:    Receive a document info response from a server

    Parameters: pvLwps                  lwps structure
                ppsdiSpiDocumentInfo    return pointer for the spi document info structure (allocated)
                ppucReferenceID         return pointer for the reference ID (allocated)

    Globals:    none

    Returns:    LWPS error code

*/
int iLwpsDocumentInfoResponseReceive
(
    void *pvLwps,
    struct spiDocumentInfo **ppsdiSpiDocumentInfo,
    unsigned char **ppucReferenceID
)
{

    int                         iError = LWPS_NoError;
    struct lwps                 *plLwps = (struct lwps *)pvLwps;
    unsigned int                uiMessageID = LWPS_INVALID_ID;
    struct spiDocumentInfo      *psdiSpiDocumentInfo = NULL;
    unsigned char               *pucReferenceID = NULL;
    unsigned int                uiI = 0;
    struct spiDocumentItem      *psdiSpiDocumentItemsPtr = NULL;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iLwpsDocumentInfoResponseReceive");  */


    /* Check the parameters */
    if ( pvLwps == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvLwps' parameter passed to 'iLwpsDocumentInfoResponseReceive'."); 
        return (LWPS_InvalidLwps);
    }

    if ( ppsdiSpiDocumentInfo == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppsdiSpiDocumentInfo' parameter passed to 'iLwpsDocumentInfoResponseReceive'."); 
        return (LWPS_ReturnParameterError);
    }

    if ( ppucReferenceID == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppucReferenceID' parameter passed to 'iLwpsDocumentInfoResponseReceive'."); 
        return (LWPS_ReturnParameterError);
    }


    /* Peek the response header, we do this here because we don't want 
    ** to harm the response if it is not what we expected
    */
    if ( (iError = iLwpsHeaderPeek(plLwps, &uiMessageID)) != LWPS_NoError ) {
        goto bailFromiLwpsDocumentInfoResponseReceive;
    }


    /* Check that the response header message tag is correct */
    if ( uiMessageID != LWPS_DOCUMENT_INFO_RESPONSE_ID) {
        iError = LWPS_InvalidMessageID;
        goto bailFromiLwpsDocumentInfoResponseReceive;
    }


    /* Read the response header */
    if ( (iError = iLwpsHeaderReceive(plLwps, &uiMessageID)) != LWPS_NoError ) {
        goto bailFromiLwpsDocumentInfoResponseReceive;
    }


    /* Allocate space for the document info */
    if ( (psdiSpiDocumentInfo = (struct spiDocumentInfo *)s_malloc((size_t)sizeof(struct spiDocumentInfo))) == NULL ) {
        iError = LWPS_MemError;
        goto bailFromiLwpsDocumentInfoResponseReceive;
    }

    /* Read the index name */
    if ( (iError = iLwpsStringRead(plLwps, &psdiSpiDocumentInfo->pucIndexName)) != LWPS_NoError ) {
        goto bailFromiLwpsDocumentInfoResponseReceive;
    }

    /* Read the document key */
    if ( (iError = iLwpsStringRead(plLwps, &psdiSpiDocumentInfo->pucDocumentKey)) != LWPS_NoError ) {
        goto bailFromiLwpsDocumentInfoResponseReceive;
    }

    /* Read the title */
    if ( (iError = iLwpsStringRead(plLwps, &psdiSpiDocumentInfo->pucTitle)) != LWPS_NoError ) {
        goto bailFromiLwpsDocumentInfoResponseReceive;
    }

    /* Read the language code */
    if ( (iError = iLwpsStringRead(plLwps, &psdiSpiDocumentInfo->pucLanguageCode)) != LWPS_NoError ) {
        goto bailFromiLwpsDocumentInfoResponseReceive;
    }

    /* Read the rank */
    if ( (iError = iLwpsIntegerRead(plLwps, &psdiSpiDocumentInfo->uiRank)) != LWPS_NoError ) {
        goto bailFromiLwpsDocumentInfoResponseReceive;
    }

    /* Read the term count */
    if ( (iError = iLwpsIntegerRead(plLwps, &psdiSpiDocumentInfo->uiTermCount)) != LWPS_NoError ) {
        goto bailFromiLwpsDocumentInfoResponseReceive;
    }

    /* Read the date */
    if ( (iError = iLwpsLongRead(plLwps, &psdiSpiDocumentInfo->ulAnsiDate)) != LWPS_NoError ) {
        goto bailFromiLwpsDocumentInfoResponseReceive;
    }

    /* Read the number of document items */
    if ( (iError = iLwpsIntegerRead(plLwps, &psdiSpiDocumentInfo->uiDocumentItemsLength)) != LWPS_NoError ) {
        goto bailFromiLwpsDocumentInfoResponseReceive;
    }


    /* Add the document items */
    if ( psdiSpiDocumentInfo->uiDocumentItemsLength > 0 ) {

        /* Allocate space for the document items */
        if ( (psdiSpiDocumentInfo->psdiSpiDocumentItems = (struct spiDocumentItem *)s_malloc((size_t)(sizeof(struct spiDocumentItem) * psdiSpiDocumentInfo->uiDocumentItemsLength))) == NULL ) {
            iError = LWPS_MemError;
            goto bailFromiLwpsDocumentInfoResponseReceive;
        }


        /* Cycle through the document items */
        for ( uiI = 0, psdiSpiDocumentItemsPtr = psdiSpiDocumentInfo->psdiSpiDocumentItems; uiI < psdiSpiDocumentInfo->uiDocumentItemsLength; uiI++, psdiSpiDocumentItemsPtr++ ) {

            /* Read the item name */
            if ( (iError = iLwpsStringRead(plLwps, &psdiSpiDocumentItemsPtr->pucItemName)) != LWPS_NoError ) {
                goto bailFromiLwpsDocumentInfoResponseReceive;
            }

            /* Read the mime type */
            if ( (iError = iLwpsStringRead(plLwps, &psdiSpiDocumentItemsPtr->pucMimeType)) != LWPS_NoError ) {
                goto bailFromiLwpsDocumentInfoResponseReceive;
            }

            /* Read the URL */
            if ( (iError = iLwpsStringRead(plLwps, &psdiSpiDocumentItemsPtr->pucUrl)) != LWPS_NoError ) {
                goto bailFromiLwpsDocumentInfoResponseReceive;
            }

            /* Read the length */
            if ( (iError = iLwpsIntegerRead(plLwps, &psdiSpiDocumentItemsPtr->uiLength)) != LWPS_NoError ) {
                goto bailFromiLwpsDocumentInfoResponseReceive;
            }

            /* Read the data */
            if ( (iError = iLwpsDataRead(plLwps, (unsigned char **)&psdiSpiDocumentItemsPtr->pvData, NULL)) != LWPS_NoError ) {
                goto bailFromiLwpsDocumentInfoResponseReceive;
            }

            /* Read the data length */
            if ( (iError = iLwpsIntegerRead(plLwps, &psdiSpiDocumentItemsPtr->uiDataLength)) != LWPS_NoError ) {
                goto bailFromiLwpsDocumentInfoResponseReceive;
            }
        }
    }


    /* Read the reference ID */
    if ( (iError = iLwpsStringRead(plLwps, &pucReferenceID)) != LWPS_NoError ) {
        goto bailFromiLwpsDocumentInfoResponseReceive;
    }



    /* Bail label */
    bailFromiLwpsDocumentInfoResponseReceive:


    /* Handle the error */
    if ( iError == LWPS_NoError ) {

        /* Set the return pointers */
        *ppsdiSpiDocumentInfo = psdiSpiDocumentInfo;
        *ppucReferenceID = pucReferenceID;
    }
    else {

        /* Free allocations */
        iSpiFreeDocumentInfo(psdiSpiDocumentInfo);
        psdiSpiDocumentInfo = NULL;

        s_free(pucReferenceID);
    }


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLwpsDocumentInfoResponseHandle()

    Purpose:    Handle the document info response by receiving an 
                document info request from a client and sending the response

    Parameters: pvLwps      lwps structure

    Globals:    none

    Returns:    LWPS error code

*/
int iLwpsDocumentInfoResponseHandle
(
    void *pvLwps
)
{

    int                         iError = LWPS_NoError;
    int                         iErrorCode = SPI_NoError;
    unsigned char               *pucErrorString = NULL;
    struct lwps                 *plLwps = (struct lwps *)pvLwps;
    unsigned char               *pucIndexName = NULL;
    unsigned char               *pucDocumentKey = NULL;
    struct spiDocumentInfo      *psdiSpiDocumentInfo = NULL;
    unsigned char               *pucReferenceID = NULL;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iLwpsDocumentInfoResponseHandle");  */


    /* Check the parameters */
    if ( pvLwps == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvLwps' parameter passed to 'iLwpsDocumentInfoResponseHandle'."); 
        return (LWPS_InvalidLwps);
    }


    /* Receive the document info request */
    if ( (iError = iLwpsDocumentInfoRequestReceive(plLwps, &pucIndexName, &pucDocumentKey, &pucReferenceID)) != LWPS_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to receive an lwps document information request message, lwps error: %d.", iError);
        goto bailFromiLwpsDocumentInfoResponseHandle;
    }


    /* Put in your document info code here */
/*     iErrorCode = myDocumentInfoCode(); */
    
    
    /* Handle the document info error */
    if ( iErrorCode == SPI_NoError ) {

        /* Send the document info response */
        if ( (iError = iLwpsDocumentInfoResponseSend(plLwps, psdiSpiDocumentInfo, pucReferenceID)) != LWPS_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to send an lwps document information response message, lwps error: %d.", iError);
            goto bailFromiLwpsDocumentInfoResponseHandle;
        }
    }
    else {

        /* Send the error */
        if ( (iError = iLwpsErrorMessageSend(plLwps, iErrorCode, pucErrorString, pucReferenceID)) != LWPS_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to send an lwps error message, lwps error: %d.", iError);
            goto bailFromiLwpsDocumentInfoResponseHandle;
        }
    }




    /* Bail label */
    bailFromiLwpsDocumentInfoResponseHandle:


    /* Free allocated pointers */
    s_free(pucIndexName);
    s_free(pucDocumentKey);

    iSpiFreeDocumentInfo(psdiSpiDocumentInfo);
    psdiSpiDocumentInfo = NULL;

    s_free(pucReferenceID);


    return (iError);

}


/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/


/*

    Function:   iLwpsHeaderSend()

    Purpose:    Send the LWPS request/response header

    Parameters: plLwps          lwps structure
                uiMessageID     message ID

    Globals:    none

    Returns:    LWPS error code

*/
static int iLwpsHeaderSend
(
    struct lwps *plLwps,
    unsigned int uiMessageID
)
{

    int             iError = LWPS_NoError;
    unsigned char   pucBuffer[LWPS_HEADER_LENGTH + 1] = {'\0'};
    unsigned char   *pucPtr = NULL;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iLwpsHeaderSend - uiMessageID: [%ui]", uiMessageID);  */


    ASSERT(plLwps != NULL);
    ASSERT(LWPS_MESSAGE_ID_VALID(uiMessageID) == true);


    /* Initialize the pointer */
    pucPtr = pucBuffer;

    
    /* Write the protocol tag, and increment the pointer - first byte */
    *pucPtr = LWPS_PROTOCOL_TAG;
    pucPtr++;
    ASSERT((pucPtr - pucBuffer) == LWPS_PROTOCOL_TAG_SIZE);


    /* Write the procotol version, pointer is automatically incremented - second byte */
    UTL_NUM_WRITE_UINT(plLwps->uiProtocolVersion, LWPS_VERSION_SIZE, pucPtr);
    ASSERT((pucPtr - pucBuffer) == (LWPS_PROTOCOL_TAG_SIZE + LWPS_VERSION_SIZE));


    /* Write the message ID, pointer is automatically incremented - third byte */
    UTL_NUM_WRITE_UINT(uiMessageID, LWPS_MESSAGE_ID_SIZE, pucPtr);
    ASSERT((pucPtr - pucBuffer) == (LWPS_PROTOCOL_TAG_SIZE + LWPS_VERSION_SIZE + LWPS_MESSAGE_ID_SIZE));


    /* Anal checks */
    ASSERT((LWPS_PROTOCOL_TAG_SIZE + LWPS_VERSION_SIZE + LWPS_MESSAGE_ID_SIZE) == LWPS_HEADER_LENGTH);
    ASSERT((pucPtr - pucBuffer) == LWPS_HEADER_LENGTH);

    
    /* Write the header */
    if ( (iError = iUtlNetWrite(plLwps->pvUtlNet, pucBuffer, LWPS_HEADER_LENGTH)) != UTL_NoError ) {
        return (iLwpsMapUtlNetError(iError, LWPS_FailedWriteData));
    }


    return (LWPS_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLwpsHeaderReceive()

    Purpose:    Get the LWPS request/response header and decodes it.

    Parameters: plLwps          lwps structure
                puiMessageID    return pointer for the message ID

    Globals:    none

    Returns:    LWPS error code

*/
static int iLwpsHeaderReceive
(
    struct lwps *plLwps,
    unsigned int *puiMessageID
)
{

    int             iError = LWPS_NoError;
    unsigned char   pucBuffer[LWPS_HEADER_LENGTH + 1] = {'\0'};
    unsigned char   *pucPtr = NULL;
    unsigned int    uiProtocolVersion = LWPS_PROTOCOL_VERSION_DEFAULT_ID;
    unsigned int    uiMessageID = LWPS_INVALID_ID;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iLwpsHeaderReceive");  */


    ASSERT(plLwps != NULL);
    ASSERT(puiMessageID != NULL);


    /* Read the header */
    if ( (iError = iUtlNetRead(plLwps->pvUtlNet, pucBuffer, LWPS_HEADER_LENGTH)) != UTL_NoError ) {
        return (iLwpsMapUtlNetError(iError, LWPS_FailedReadData));
    }
    

    /* Initialize the pointer */
    pucPtr = pucBuffer;
    
    /* Read and check the protocol header tag - first byte */
    if ( *pucPtr != LWPS_PROTOCOL_TAG ) {
        return (LWPS_InvalidProtocolHeader);
    }
    
    /* And increment the pointer */
    pucPtr++;
    ASSERT((pucPtr - pucBuffer) == LWPS_PROTOCOL_TAG_SIZE);
    
    
    /* Read the procotol version, pointer is automatically incremented - second byte */
    UTL_NUM_READ_UINT(uiProtocolVersion, LWPS_VERSION_SIZE, pucPtr);
    ASSERT((pucPtr - pucBuffer) == (LWPS_PROTOCOL_TAG_SIZE + LWPS_VERSION_SIZE));

    /* Check that we support this version and set the protocol version in the lwps structure */
    if ( (uiProtocolVersion == LWPS_PROTOCOL_VERSION_1_7_ID) || (uiProtocolVersion == LWPS_PROTOCOL_VERSION_1_8_ID) ) {
        plLwps->uiProtocolVersion = uiProtocolVersion;
    }
    else {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid protocol version:  %d, expected: %d or %d.", uiProtocolVersion, 
                LWPS_PROTOCOL_VERSION_1_7_ID, LWPS_PROTOCOL_VERSION_1_8_ID);
        return (LWPS_InvalidProtocolVersion);
    }


    /* Read the message ID, pointer is automatically incremented - third byte */
    UTL_NUM_READ_UINT(uiMessageID, LWPS_MESSAGE_ID_SIZE, pucPtr);
    ASSERT((pucPtr - pucBuffer) == (LWPS_PROTOCOL_TAG_SIZE + LWPS_VERSION_SIZE + LWPS_MESSAGE_ID_SIZE));
    ASSERT(LWPS_MESSAGE_ID_VALID(uiMessageID) == true);


    /* Anal checks */
    ASSERT((LWPS_PROTOCOL_TAG_SIZE + LWPS_VERSION_SIZE + LWPS_MESSAGE_ID_SIZE) == LWPS_HEADER_LENGTH);
    ASSERT((pucPtr - pucBuffer) == LWPS_HEADER_LENGTH);


    /* Set the return pointer */
    *puiMessageID = uiMessageID;


    return (LWPS_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLwpsStringWrite()

    Purpose:    Write the string

    Parameters: plLwps      lwps structure
                pucString   string

    Globals:    none

    Returns:    LWPS error code

*/
static int iLwpsStringWrite
(
    struct lwps *plLwps,
    unsigned char *pucString
)
{

    ASSERT(plLwps != NULL);


    /* Send the string as data */
    return (iLwpsDataWrite(plLwps, pucString, (pucString != NULL) ? s_strlen(pucString) : 0));

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLwpsStringRead()

    Purpose:    Read the string

    Parameters: plLwps          lwps structure
                ppucString      return pointer for the string (allocated)

    Globals:    none

    Returns:    LWPS error code

*/
static int iLwpsStringRead
(
    struct lwps *plLwps,
    unsigned char **ppucString
)
{
    
    int             iError = LWPS_NoError;
    unsigned char   *pucData = NULL;
    unsigned char   *pucDataPtr = NULL;
    unsigned int    uiDataLength = 0;


    ASSERT(plLwps != NULL);
    ASSERT(ppucString != NULL);


    /* Read the data */
    if ( (iError = iLwpsDataRead(plLwps, &pucData, &uiDataLength)) != LWPS_NoError ) {
        s_free(pucData);
        return (iError);
    }

    /* Was any data to read? */
    if ( uiDataLength == 0 ) {
        return (LWPS_NoError);
    }

    /* NULL terminate the data */
    if ( (iError = iUtlStringsNullTerminateData((void *)pucData, uiDataLength, &pucDataPtr)) != UTL_NoError ) {
        s_free(pucData);
        return (LWPS_MemError);
    }

    /* Set the return pointer */
    *ppucString = pucDataPtr;


    return (LWPS_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLwpsIntegerWrite()

    Purpose:    Write the integer

    Parameters: plLwps      lwps structure
                uiInteger   integer

    Globals:    none

    Returns:    LWPS error code

*/
static int iLwpsIntegerWrite
(
    struct lwps *plLwps,
    unsigned int uiInteger
)
{

    int             iError = LWPS_NoError;
    unsigned char   pucBuffer[UTL_NUM_COMPRESSED_UINT_MAX_SIZE];
    unsigned char   *pucPtr = pucBuffer;


    ASSERT(plLwps != NULL);
    ASSERT(uiInteger >= 0);


    /* Write the integer into the buffer */
    UTL_NUM_WRITE_COMPRESSED_UINT(uiInteger, pucPtr);

    /* Write the buffer */
    if ( (iError = iUtlNetWrite(plLwps->pvUtlNet, pucBuffer, pucPtr - pucBuffer)) != UTL_NoError ) {
        return (iLwpsMapUtlNetError(iError, LWPS_FailedWriteData));
    }


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLwpsIntegerRead()

    Purpose:    Read the integer

    Parameters: plLwps          lwps structure
                puiInteger      return pointer for the integer

    Globals:    none

    Returns:    LWPS error code

*/
static int iLwpsIntegerRead
(
    struct lwps *plLwps,
    unsigned int *puiInteger
)
{

    int             iError = LWPS_NoError;
    unsigned int    uiI = 0;
    boolean         bCompleteNumber = false;
    unsigned char   *pucPeekPtr = NULL;
    unsigned char   *pucPtr = NULL;
    unsigned int    uiInteger = 0;


    ASSERT(plLwps != NULL);
    ASSERT(puiInteger != NULL);


    /* First peek the bytes that are used to represent the compressed interger,
    ** this is a bit of a hack as it forces us to look into the format of a 
    ** compressed interger which the macro is supposed to keep hidden
    */
    for ( uiI = 0, bCompleteNumber = false; uiI < UTL_NUM_COMPRESSED_UINT_MAX_SIZE; uiI++ ) {

        /* Peek an increasing number of bytes */
        if ( (iError = iUtlNetPeek(plLwps->pvUtlNet, uiI + 1, &pucPeekPtr)) != UTL_NoError ) {
            return (iLwpsMapUtlNetError(iError, LWPS_FailedReadData));
        }
        
        /* Break here if the byte does not have the continue bit set */
        if ( !(*(pucPeekPtr + uiI) & UTL_NUM_COMPRESSED_CONTINUE_BIT) ) {
            bCompleteNumber = true;
            break;
        }
    }

    /* Check that we got a complete number */
    if ( bCompleteNumber == false ) {
        return (LWPS_FailedReadData);
    }

    /* Read the integer */
    pucPtr = pucPeekPtr;
    UTL_NUM_READ_COMPRESSED_UINT(uiInteger, pucPtr);
    
    /* Then skip the number of bytes that were actually used to store the integer */
    if ( (iError = iUtlNetSkip(plLwps->pvUtlNet, pucPtr - pucPeekPtr)) != UTL_NoError ) {
        return (iLwpsMapUtlNetError(iError, LWPS_FailedReadData));
    }

    /* Set the return pointer */
    *puiInteger = uiInteger;


    return (LWPS_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLwpsLongWrite()

    Purpose:    Write the long

    Parameters: plLwps      lwps structure
                ulLong      long

    Globals:    none

    Returns:    LWPS error code

*/
static int iLwpsLongWrite
(
    struct lwps *plLwps,
    unsigned long ulLong
)
{

    int             iError = LWPS_NoError;
    unsigned char   pucBuffer[UTL_NUM_COMPRESSED_ULONG_MAX_SIZE];
    unsigned char   *pucPtr = pucBuffer;


    ASSERT(plLwps != NULL);
    ASSERT(ulLong >= 0);


    /* Write the long into the buffer */
    UTL_NUM_WRITE_COMPRESSED_ULONG(ulLong, pucPtr);

    /* Write the buffer */
    if ( (iError = iUtlNetWrite(plLwps->pvUtlNet, pucBuffer, pucPtr - pucBuffer)) != UTL_NoError ) {
        return (iLwpsMapUtlNetError(iError, LWPS_FailedWriteData));
    }


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLwpsLongRead()

    Purpose:    Read the long

    Parameters: plLwps      lwps structure
                pulLong     return pointer for the long

    Globals:    none

    Returns:    LWPS error code

*/
static int iLwpsLongRead
(
    struct lwps *plLwps,
    unsigned long *pulLong
)
{

    int             iError = LWPS_NoError;
    unsigned int    uiI = 0;
    boolean         bCompleteNumber = false;
    unsigned char   *pucPeekPtr = NULL;
    unsigned char   *pucPtr = NULL;
    unsigned long   ulLong = 0;


    ASSERT(plLwps != NULL);
    ASSERT(pulLong != NULL);


    /* First peek the bytes that are used to represent the compressed long,
    ** this is a bit of a hack as it forces us to look into the format of a 
    ** compressed long which the macro is supposed to keep hidden
    */
    for ( uiI = 0, bCompleteNumber = false; uiI < UTL_NUM_COMPRESSED_ULONG_MAX_SIZE; uiI++ ) {

        /* Peek an increasing number of bytes */
        if ( (iError = iUtlNetPeek(plLwps->pvUtlNet, uiI + 1, &pucPeekPtr)) != UTL_NoError ) {
            return (iLwpsMapUtlNetError(iError, LWPS_FailedReadData));
        }
        
        /* Break here if the byte does not have the continue bit set */
        if ( !(*(pucPeekPtr + uiI) & UTL_NUM_COMPRESSED_CONTINUE_BIT) ) {
            bCompleteNumber = true;
            break;
        }
    }

    /* Check that we got a complete number */
    if ( bCompleteNumber == false ) {
        return (LWPS_FailedReadData);
    }

    /* Read the long */
    pucPtr = pucPeekPtr;
    UTL_NUM_READ_COMPRESSED_ULONG(ulLong, pucPtr);
    
    /* Then skip the number of bytes that were actually used to store the long */
    if ( (iError = iUtlNetSkip(plLwps->pvUtlNet, pucPtr - pucPeekPtr)) != UTL_NoError ) {
        return (iLwpsMapUtlNetError(iError, LWPS_FailedReadData));
    }

    /* Set the return pointer */
    *pulLong = ulLong;


    return (LWPS_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLwpsFloatWrite()

    Purpose:    Write the float

    Parameters: plLwps      lwps structure
                fFloat      float

    Globals:    none

    Returns:    LWPS error code

*/
static int iLwpsFloatWrite
(
    struct lwps *plLwps,
    float fFloat
)
{

    int             iError = LWPS_NoError;
    unsigned char   pucBuffer[UTL_NUM_COMPRESSED_FLOAT_SIZE];
    unsigned char   *pucPtr = pucBuffer;


    ASSERT(plLwps != NULL);


    /* Write the float into the buffer */
    UTL_NUM_WRITE_COMPRESSED_FLOAT(fFloat, pucPtr);

    /* Write the buffer */
    if ( (iError = iUtlNetWrite(plLwps->pvUtlNet, pucBuffer, pucPtr - pucBuffer)) != UTL_NoError ) {
        return (iLwpsMapUtlNetError(iError, LWPS_FailedWriteData));
    }


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLwpsFloatRead()

    Purpose:    Read the float

    Parameters: plLwps      lwps structure
                pfFloat     return pointer for the float

    Globals:    none

    Returns:    LWPS error code

*/
static int iLwpsFloatRead
(
    struct lwps *plLwps,
    float *pfFloat
)
{

    int             iError = UTL_NoError;
    unsigned char   pucBuffer[UTL_NUM_COMPRESSED_FLOAT_SIZE];
    unsigned char   *pucPtr = pucBuffer;
    float           fFloat = 0;


    ASSERT(plLwps != NULL);
    ASSERT(pfFloat != NULL);


    /* Read the data into the buffer */
    if ( (iError = iUtlNetRead(plLwps->pvUtlNet, pucBuffer, UTL_NUM_COMPRESSED_FLOAT_SIZE)) != UTL_NoError ) {
        return (iLwpsMapUtlNetError(iError, LWPS_FailedReadData));
    }

    /* Read the float from the buffer */
    UTL_NUM_READ_COMPRESSED_FLOAT(fFloat, pucPtr)

    /* Set the return pointer */
    *pfFloat = fFloat;


    return (LWPS_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLwpsDoubleWrite()

    Purpose:    Write the double

    Parameters: plLwps      lwps structure
                dDouble     double

    Globals:    none

    Returns:    LWPS error code

*/
static int iLwpsDoubleWrite
(
    struct lwps *plLwps,
    double dDouble
)
{

    int             iError = LWPS_NoError;
    unsigned char   pucBuffer[UTL_NUM_COMPRESSED_DOUBLE_SIZE];
    unsigned char   *pucPtr = pucBuffer;

    
    ASSERT(plLwps != NULL);


    /* Write the double into the buffer */
    UTL_NUM_WRITE_COMPRESSED_DOUBLE(dDouble, pucPtr);

    /* Write the buffer */
    if ( (iError = iUtlNetWrite(plLwps->pvUtlNet, pucBuffer, pucPtr - pucBuffer)) != UTL_NoError ) {
        return (iLwpsMapUtlNetError(iError, LWPS_FailedWriteData));
    }


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLwpsDoubleRead()

    Purpose:    Read the double

    Parameters: plLwps      lwps structure
                pdDouble    return pointer for the double

    Globals:    none

    Returns:    LWPS error code

*/
static int iLwpsDoubleRead
(
    struct lwps *plLwps,
    double *pdDouble
)
{

    int             iError = UTL_NoError;
    unsigned char   pucBuffer[UTL_NUM_COMPRESSED_DOUBLE_SIZE];
    unsigned char   *pucPtr = pucBuffer;
    double          dDouble = 0;


    ASSERT(plLwps != NULL);
    ASSERT(pdDouble != NULL);


    /* Read the data into the buffer */
    if ( (iError = iUtlNetRead(plLwps->pvUtlNet, pucBuffer, UTL_NUM_COMPRESSED_DOUBLE_SIZE)) != UTL_NoError ) {
        return (iLwpsMapUtlNetError(iError, LWPS_FailedReadData));
    }

    /* Read the v from the buffer */
    UTL_NUM_READ_COMPRESSED_DOUBLE(dDouble, pucPtr)

    /* Set the return pointer */
    *pdDouble = dDouble;


    return (LWPS_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLwpsDataWrite()

    Purpose:    Write the data

    Parameters: plLwps              lwps structure
                pucBuffer           buffer to send
                uiBufferLength      length of the buffer to send

    Globals:    none

    Returns:    LWPS error code

*/
static int iLwpsDataWrite
(
    struct lwps *plLwps,
    unsigned char *pucData,
    unsigned int uiDataLength
)
{

    int             iError = LWPS_NoError;
    unsigned char   pucBuffer[UTL_NUM_COMPRESSED_UINT_MAX_SIZE];
    unsigned char   *pucPtr = pucBuffer;


    ASSERT(plLwps != NULL);
    ASSERT(((pucData != NULL) && (uiDataLength > 0)) || (uiDataLength == 0));


    /* Write the length of the data into the size buffer */
    UTL_NUM_WRITE_COMPRESSED_UINT(uiDataLength, pucPtr);

    /* Write the size buffer */
    if ( (iError = iUtlNetWrite(plLwps->pvUtlNet, pucBuffer, pucPtr - pucBuffer)) != UTL_NoError ) {
        return (iLwpsMapUtlNetError(iError, LWPS_FailedWriteData));
    }

    /* Write the data */
    if ( (pucBuffer != NULL) && (uiDataLength > 0) ) {
        if ( (iError = iUtlNetWrite(plLwps->pvUtlNet, pucData, uiDataLength)) != UTL_NoError ) {
            return (iLwpsMapUtlNetError(iError, LWPS_FailedWriteData));
        }
    }


    return (LWPS_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLwpsDataRead()

    Purpose:    Read the data

    Parameters: plLwps          lwps structure
                ppucData        return pointer for the data (allocated)
                puiDataLength   return pointer for the data length (optional)

    Globals:    none

    Returns:    LWPS error code

*/
static int iLwpsDataRead
(
    struct lwps *plLwps,
    unsigned char **ppucData,
    unsigned int *puiDataLength
)
{

    int             iError = LWPS_NoError;
    unsigned int    uiI = 0;
    boolean         bCompleteNumber = false;
    unsigned char   *pucPeekPtr = NULL;
    unsigned char   *pucPtr = NULL;
    unsigned char   *pucData = NULL;
    unsigned int    uiDataLength = 0;


    ASSERT(plLwps != NULL);
    ASSERT(ppucData != NULL);
    ASSERT((puiDataLength != NULL) || (puiDataLength == NULL));


    /* First peek the bytes that are used to represent the compressed interger,
    ** this is a bit of a hack as it forces us to look into the format of a 
    ** compressed interger which the macro is supposed to keep hidden
    */
    for ( uiI = 0, bCompleteNumber = false; uiI < UTL_NUM_COMPRESSED_UINT_MAX_SIZE; uiI++ ) {

        /* Peek an increasing number of bytes */
        if ( (iError = iUtlNetPeek(plLwps->pvUtlNet, uiI + 1, &pucPeekPtr)) != UTL_NoError ) {
            return (iLwpsMapUtlNetError(iError, LWPS_FailedReadData));
        }
        
        /* Break here if the byte does not have the continue bit set */
        if ( !(*(pucPeekPtr + uiI) & UTL_NUM_COMPRESSED_CONTINUE_BIT) ) {
            bCompleteNumber = true;
            break;
        }
    }
    
    /* Check that we got a complete number */
    if ( bCompleteNumber == false ) {
        return (LWPS_FailedReadData);
    }
        
    /* Read the data block length */
    pucPtr = pucPeekPtr;
    UTL_NUM_READ_COMPRESSED_UINT(uiDataLength, pucPtr);
    
    /* Then skip the number of bytes that were actually used to store the data block length */
    if ( (iError = iUtlNetSkip(plLwps->pvUtlNet, pucPtr - pucPeekPtr)) != UTL_NoError ) {
        return (iLwpsMapUtlNetError(iError, LWPS_FailedReadData));
    }


    /* Is there any data to read? */
    if ( uiDataLength == 0 ) {
        return (LWPS_NoError);
    }


    /* Allocate space for the data */
    if ( (pucData = (unsigned char *)s_malloc((size_t)(sizeof(unsigned char) * uiDataLength))) == NULL ) {
        return (LWPS_MemError);
    }

    /* Read the data */
    if ( (iError = iUtlNetRead(plLwps->pvUtlNet, pucData, uiDataLength)) != UTL_NoError ) {
        s_free(pucData);
        return (iLwpsMapUtlNetError(iError, LWPS_FailedReadData));
    }


    /* Set the return pointers, note that puiDataLength is optional */
    *ppucData = pucData;
    if ( puiDataLength != NULL ) {
        *puiDataLength = uiDataLength;
    }


    return (LWPS_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLwpsMapUtlNetError()

    Purpose:    Maps a net error to a lwps error

    Parameters: iNetError           net error
                iDefaultLwpsError   default lwps error

    Globals:    none

    Returns:    LWPS error code

*/
static int iLwpsMapUtlNetError
(
    int iNetError,
    int iDefaultLwpsError
)
{

    /* Map the net error to the lwps error */
    switch ( iNetError ) {
    
        case UTL_NoError:
            return (LWPS_NoError);
            
        case UTL_NetTimeOut:
            return (LWPS_TimeOut);
        
        case UTL_NetSendDataFailed:
            return (LWPS_FailedWriteData);
        
        case UTL_NetReceiveDataFailed:
            return (LWPS_FailedReadData);
        
        case UTL_NetSocketClosed:
            return (LWPS_SocketClosed);
    }


    /* Return the default lwps error */
    return (iDefaultLwpsError);

}



/*---------------------------------------------------------------------------*/
