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

    Module:     retrieval.c

    Author:     Francois Schiettecatte (FS Consulting LLC.)

    Created:    24 February 1994

    Purpose:    This module contains the functions which retrieve documents
                for the user.

*/


/*---------------------------------------------------------------------------*/


/*
** Includes
*/

#include "srch.h"


/*---------------------------------------------------------------------------*/


/*
** Feature defines
*/

/* Context for logging */
#undef UTL_LOG_CONTEXT
#define UTL_LOG_CONTEXT                     (unsigned char *)"com.fsconsult.mps.src.search.retrieval"


/*---------------------------------------------------------------------------*/


/*
** Private functions prototypes
*/

static int iSrchRetrievalResolveDocumentKey (struct srchSearch *pssSrchSearch, struct srchIndex *psiSrchIndex, 
        unsigned char *pucDocumentKey, unsigned char *pucItemName, unsigned char *pucMimeType,
        unsigned char *pucFilePath, unsigned int uiFilePathLength, off_t *pzStartOffset, off_t *pzEndOffset, 
        void **ppvData, unsigned int *puiDataLength);

static int iSrchRetrievalRetrieveFromData (struct srchIndex *psiSrchIndex, unsigned char *pucDocumentKey, 
        unsigned char *pucItemName, unsigned char *pucMimeType, unsigned int uiChunkStart, unsigned int uiChunkEnd, 
        void **ppvData, unsigned int *puiDataLength);

static int iSrchRetrievalRetrieveFromFile (struct srchIndex *psiSrchIndex, unsigned char *pucDocumentKey, 
        unsigned char *pucItemName, unsigned char *pucMimeType, unsigned int uiChunkStart, unsigned int uiChunkEnd, 
        unsigned char *pucFilePath, off_t zStartOffset, off_t zEndOffset, void **ppvData, unsigned int *puiDataLength);


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchRetrievalRetrieveDocument()

    Purpose:    This function gets document.

                It will first check to see if the document requested is a 
                composite document. If it is not the document component will 
                be retrieved and returned. If it is a composite document, the 
                various components will be retrieved, joined together and the 
                chunk requested will returned to the client.

    Parameters: pssSrchSearch       search structure
                psiSrchIndex        search index structure
                pucDocumentKey      document key
                pucItemName         document item name
                pucMimeType         document mime type
                uiChunkType         chunk type
                uiChunkStart        start index of the chunk
                uiChunkEnd          end index of the chunk
                ppvData             return pointer for the data retrieved
                puiDataLength       return pointer for the length of the data retrieved

    Globals:    none

    Returns:    SRCH error code

*/
int iSrchRetrievalRetrieveDocument
(
    struct srchSearch *pssSrchSearch,
    struct srchIndex *psiSrchIndex,
    unsigned char *pucDocumentKey,
    unsigned char *pucItemName,
    unsigned char *pucMimeType,
    unsigned int uiChunkType,
    unsigned int uiChunkStart,
    unsigned int uiChunkEnd,
    void **ppvData,
    unsigned int *puiDataLength
)
{

    int             iError = SRCH_NoError;
    unsigned char   pucFilePath[UTL_FILE_PATH_MAX + 1] = {'\0'};
    off_t           zStartOffset = 0;
    off_t           zEndOffset = 0;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iSrchRetrievalRetrieveDocument - pucDocumentKey [%s], pucItemName [%s], pucMimeType [%s], uiChunkType [%u], uiChunkStart [%u], uiChunkEnd [%u]",  */
/*             pucUtlStringsGetPrintableString(pucDocumentKey), pucItemName, pucMimeType, uiChunkType, uiChunkStart, uiChunkEnd); */


    /* Check the parameters */
    if ( pssSrchSearch == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pssSrchSearch' parameter passed to 'iSrchRetrievalRetrieveDocument'."); 
        return (SRCH_RetrievalInvalidSearch);
    }

    if ( psiSrchIndex == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'psiSrchIndex' parameter passed to 'iSrchRetrievalRetrieveDocument'."); 
        return (SRCH_InvalidIndex);
    }

    if ( bUtlStringsIsStringNULL(pucDocumentKey) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pucDocumentKey' parameter passed to 'iSrchRetrievalRetrieveDocument'."); 
        return (SRCH_InvalidDocumentKey);
    }

    if ( bUtlStringsIsStringNULL(pucItemName) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pucItemName' parameter passed to 'iSrchRetrievalRetrieveDocument'."); 
        return (SRCH_InvalidItemName);
    }

    if ( bUtlStringsIsStringNULL(pucMimeType) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pucMimeType' parameter passed to 'iSrchRetrievalRetrieveDocument'."); 
        return (SRCH_InvalidMimeType);
    }

    if ( SPI_CHUNK_TYPE_VALID(uiChunkType) == false ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiChunkType' parameter passed to 'iSrchRetrievalRetrieveDocument'."); 
        return (SRCH_InvalidChunkType);
    }

    if ( uiChunkStart < 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiChunkStart' parameter passed to 'iSrchRetrievalRetrieveDocument'."); 
        return (SRCH_InvalidChunkRange);
    }

    if ( uiChunkEnd < 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiChunkEnd' parameter passed to 'iSrchRetrievalRetrieveDocument'."); 
        return (SRCH_InvalidChunkRange);
    }

    if ( uiChunkStart > uiChunkEnd ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiChunkStart' & 'uiChunkEnd' parameters passed to 'iSrchRetrievalRetrieveDocument'."); 
        return (SRCH_InvalidChunkRange);
    }

    if ( ppvData == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppvData' parameter passed to 'iSrchRetrievalRetrieveDocument'."); 
        return (SRCH_ReturnParameterError);
    }

    if ( puiDataLength == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'puiDataLength' parameter passed to 'iSrchRetrievalRetrieveDocument'."); 
        return (SRCH_ReturnParameterError);
    }


    /* Resolve the document key, this also retrieves the document if it is a system
    ** document (such as a search report) or if the document is stored in the index 
    */
    if ( (iError = iSrchRetrievalResolveDocumentKey(pssSrchSearch, psiSrchIndex, pucDocumentKey, pucItemName, pucMimeType,
            pucFilePath, UTL_FILE_PATH_MAX + 1, &zStartOffset, &zEndOffset, ppvData, puiDataLength)) != SRCH_NoError ) {
        return (iError);
    }


    /* See if data was returned for this document */
    if ( (*ppvData != NULL) && (*puiDataLength > 0) ) {

        /* Data was returned, so we need to make a document from it */
        switch ( uiChunkType ) {

            /* Document chunk - override the chunk start and chunk end to retrieve the entire document */
            case SPI_CHUNK_TYPE_DOCUMENT:
                iError = iSrchRetrievalRetrieveFromData(psiSrchIndex, pucDocumentKey, pucItemName, pucMimeType, 0, 0, ppvData, puiDataLength);
                break;

            /* Byte chunk, get the chunk requested from the data */
            case SPI_CHUNK_TYPE_BYTE:
                iError = iSrchRetrievalRetrieveFromData(psiSrchIndex, pucDocumentKey, pucItemName, pucMimeType, uiChunkStart, uiChunkEnd, ppvData, puiDataLength);
                break;

            /* Invalid chunk type */
            default:
                iError = SRCH_InvalidChunkType;
                break;
        }
    }

    /* Retrieve the document if we have a file path for this document */
    else if ( bUtlStringsIsStringNULL(pucFilePath) == false ) {

        /* Route the document retrieval to the correct function */
        switch ( uiChunkType ) {

            /* Document chunk - override the chunk start and chunk end */
            case SPI_CHUNK_TYPE_DOCUMENT:
                iError = iSrchRetrievalRetrieveFromFile(psiSrchIndex, pucDocumentKey,    pucItemName, pucMimeType, 0, 0, 
                        pucFilePath, zStartOffset, zEndOffset, ppvData, puiDataLength);
                break;

            /* Byte chunk, get the chunk requested from the file */
            case SPI_CHUNK_TYPE_BYTE:
                iError = iSrchRetrievalRetrieveFromFile(psiSrchIndex, pucDocumentKey, pucItemName, pucMimeType, uiChunkStart, uiChunkEnd, 
                        pucFilePath, zStartOffset, zEndOffset, ppvData, puiDataLength);
                break;

            /* Invalid chunk type */
            default:
                iError = SRCH_InvalidChunkType;
                break;
        }
    }

    /* Neither data nor file path was returned */
    else {
        iError = SRCH_RetrieveDocumentFailed;
    }


    return (iError);

}


/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchRetrievalResolveDocumentKey()

    Purpose:    This function resolves the document key, and also retrieves 
                the document if it is a system document (such as a search report) 
                or if the document is stored in the index

    Parameters: pssSrchSearch       search structure
                psiSrchIndex        search index structure
                pucDocumentKey      document key
                pucItemName         document item name
                pucMimeType         document mime type
                pucFilePath         return pointer for the path of the file containing
                                    the document
                uiFilePathLength    file path length
                pzStartOffset       returned document start index in the file
                pzEndOffset         returned document end index in the file
                ppvData             return pointer for the data retrieved
                puiDataLength       return pointer for the length of the data retrieved

    Globals:    none

    Returns:    SRCH error code

*/
static int iSrchRetrievalResolveDocumentKey
(
    struct srchSearch *pssSrchSearch,
    struct srchIndex *psiSrchIndex,
    unsigned char *pucDocumentKey,
    unsigned char *pucItemName,
    unsigned char *pucMimeType,
    unsigned char *pucFilePath,
    unsigned int uiFilePathLength,
    off_t *pzStartOffset,
    off_t *pzEndOffset,
    void **ppvData,
    unsigned int *puiDataLength
)
{

    int                         iError = SRCH_NoError;
    unsigned int                uiDocumentID = 0;
    unsigned char               pucScanfFormat[UTL_STRING_SCANF_FORMAT_LENGTH + 1] = {'\0'};
    unsigned char               pucSystemDocumentKey[UTL_FILE_PATH_MAX + 1] = {'\0'};

    unsigned int                uiI = 0, uiJ = 0;
    unsigned char               pucLocalItemName[SPI_ITEM_NAME_MAXIMUM_LENGTH + 1] = {'\0'};
    unsigned char               pucLocalMimeType[SPI_MIME_TYPE_MAXIMUM_LENGTH + 1] = {'\0'};
    struct srchDocumentItem     *psdiSrchDocumentItems = NULL;
    unsigned int                uiSrchDocumentItemsLength = 0;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iSrchRetrievalResolveDocumentKey - pucDocumentKey [%s], pucItemName [%s], pucMimeType [%s]",  */
/*             pucDocumentKey, pucItemName, pucMimeType); */


    ASSERT(pssSrchSearch != NULL);
    ASSERT(psiSrchIndex != NULL);
    ASSERT(bUtlStringsIsStringNULL(pucDocumentKey) == false);
    ASSERT(bUtlStringsIsStringNULL(pucItemName) == false);
    ASSERT(bUtlStringsIsStringNULL(pucMimeType) == false);
    ASSERT(pucFilePath != NULL);
    ASSERT(uiFilePathLength > 0);
    ASSERT(pzStartOffset != NULL);
    ASSERT(pzEndOffset != NULL);
    ASSERT(ppvData != NULL);
    ASSERT(puiDataLength != NULL);


    /* Clear the return pointers as they tell the calling function whether we retrieved 
    ** the data for the document or whether to get the document from a file
    */
    pucFilePath[0] = '\0';
    *pzStartOffset = 0;
    *pzEndOffset = 0;
    *ppvData = NULL;
    *puiDataLength = 0;


    /* Check for system documents first */
    snprintf(pucScanfFormat, UTL_STRING_SCANF_FORMAT_LENGTH + 1, "%d %%%ds", SRCH_SEARCH_REPORT_DOCUMENT_ID, UTL_FILE_PATH_MAX);
    if ( sscanf(pucDocumentKey, pucScanfFormat, pucSystemDocumentKey) == 1 ) {

        /* Potential search report */
        if ( (s_strcmp(SPI_SEARCH_REPORT_ITEM_NAME, pucItemName) == 0) && (s_strcmp(SPI_SEARCH_REPORT_MIME_TYPE, pucMimeType) == 0) ) {
            
            /* Get the search report text */
            if ( (iError = iSrchReportGetReportText(pssSrchSearch->pvSrchReport, pucSystemDocumentKey, (unsigned char **)ppvData)) != SRCH_NoError ) {
                return (iError);
            }

            /* This is a valid search report, so we set the data length and return here */
            *puiDataLength = s_strlen((unsigned char *)*ppvData);

            /* Return */
            return (SRCH_NoError);
        }
    }


    /* Look up the document key in the document key dictionary */
    if ( (iError = iSrchKeyDictLookup(psiSrchIndex, pucDocumentKey, &uiDocumentID)) != SRCH_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid document key: '%s', index: '%s', srch error: %d.", pucDocumentKey, psiSrchIndex->pucIndexName, iError);
        return (iError);
    }

    /* Check that the document ID exists before going any futher */
    if ( (iError = iSrchDocumentValidateDocumentID(psiSrchIndex, uiDocumentID)) != SRCH_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid document ID: %u, document key: '%s', index: '%s', srch error: %d.", 
                uiDocumentID, pucDocumentKey, psiSrchIndex->pucIndexName, iError);
        return (iError);
    }


    /* Get the document info */
    if ( (iError = iSrchDocumentGetDocumentInfo(psiSrchIndex, uiDocumentID, NULL, NULL, NULL, NULL, NULL, NULL, 
            &psdiSrchDocumentItems, &uiSrchDocumentItemsLength, 0, true, false, true)) != SRCH_NoError) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the document information for document ID: %u, document key: '%s', index: '%s', srch error: %d.", 
                uiDocumentID, pucDocumentKey, psiSrchIndex->pucIndexName, iError);
        return (iError);
    }


    /* Loop through all the types, looking for the item name/mime type combination that is being retrieved */
    for ( uiI = 0; uiI < uiSrchDocumentItemsLength; uiI++ ) {

        /* Get the item info for this item ID */
        if ( (iError = iSrchInfoGetItemInfo(psiSrchIndex, (psdiSrchDocumentItems + uiI)->uiItemID, 
                pucLocalItemName, SPI_ITEM_NAME_MAXIMUM_LENGTH + 1, pucLocalMimeType, SPI_MIME_TYPE_MAXIMUM_LENGTH + 1)) != SRCH_NoError ) {

            iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the item info, item ID: %u, document key: '%s', index: '%s', srch error: %d.", 
                    (psdiSrchDocumentItems + uiI)->uiItemID, pucDocumentKey, psiSrchIndex->pucIndexName, iError);

            goto bailFromiSrchRetrievalResolveDocumentKey;
        }

        
        /* Match the item name and mime type */
        if ( (s_strcmp(pucItemName, pucLocalItemName) == 0) && (s_strcmp(pucMimeType, pucLocalMimeType) == 0) ) {

            /* We prefer to retrieve data as opposed to using the file name and offsets */
            if ( ((psdiSrchDocumentItems + uiI)->pvData != NULL) && ((psdiSrchDocumentItems + uiI)->uiDataLength > 0) ) {
                /* Hand over the data information */
                *ppvData = (psdiSrchDocumentItems + uiI)->pvData;
                (psdiSrchDocumentItems + uiI)->pvData = NULL; 
                *puiDataLength = (psdiSrchDocumentItems + uiI)->uiDataLength;
            }
            else {
                /* Copy the file information */
                s_strnncpy(pucFilePath, (psdiSrchDocumentItems + uiI)->pucFilePath, uiFilePathLength);
                *pzStartOffset = (psdiSrchDocumentItems + uiI)->zStartOffset;
                *pzEndOffset = (psdiSrchDocumentItems + uiI)->zEndOffset;
            }
            
            /* Set the error and bail */
            iError = SRCH_NoError;
            goto bailFromiSrchRetrievalResolveDocumentKey;
        }
    }


    /* We fell through so the item name/mime type combination that is being 
    ** retrieved does not exist, so we failed to retrieve the document
    */
    iError = SRCH_RetrieveDocumentFailed;
    iUtlLogError(UTL_LOG_CONTEXT, "Invalid document item/mime typ combination, item name: '%s', mime type: '%s', document key: '%s', index: '%s', srch error: %d.", 
                pucItemName, pucMimeType, pucDocumentKey, psiSrchIndex->pucIndexName, iError);



    /* Bail label */
    bailFromiSrchRetrievalResolveDocumentKey:


    /* Free the type profile */
    if ( psdiSrchDocumentItems != NULL ) {
        for ( uiJ = 0; uiJ < uiSrchDocumentItemsLength; uiJ++ ) {
            s_free((psdiSrchDocumentItems + uiJ)->pvData);
        }
        s_free(psdiSrchDocumentItems);
    }


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchRetrievalRetrieveFromData()

    Purpose:    Retrieves a document chunk from the passed data, 
                side-effects the passed pointers.

    Parameters: psiSrchIndex        search index structure
                pucDocumentKey      document key (for logging only)
                pucItemName         document item name (for logging only)
                pucMimeType         document mime type (for logging only)
                uiChunkStart        start index of the chunk
                uiChunkEnd          end index of the chunk
                ppvData             pointer to the data retrieved (side effected)
                puiDataLength       pointer to the length of the data retrieved (side effected)

    Globals:    none

    Returns:    SRCH error code

*/
static int iSrchRetrievalRetrieveFromData
(
    struct srchIndex *psiSrchIndex,
    unsigned char *pucDocumentKey,
    unsigned char *pucItemName,
    unsigned char *pucMimeType,
    unsigned int uiChunkStart,
    unsigned int uiChunkEnd,
    void **ppvData,
    unsigned int *puiDataLength
)
{

    int     iError = SRCH_NoError;    


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iSrchRetrievalRetrieveFromData - pucItemName [%s], pucMimeType [%s], uiChunkStart [%u], uiChunkEnd [%u]",  */
/*             pucItemName, pucMimeType, uiChunkStart, uiChunkEnd); */


    ASSERT(psiSrchIndex != NULL);
    ASSERT(bUtlStringsIsStringNULL(pucDocumentKey) == false);
    ASSERT(bUtlStringsIsStringNULL(pucItemName) == false);
    ASSERT(bUtlStringsIsStringNULL(pucMimeType) == false);
    ASSERT(uiChunkStart >= 0);
    ASSERT(uiChunkEnd >= 0);
    ASSERT(uiChunkEnd >= uiChunkStart);
    ASSERT((ppvData != NULL) && (*ppvData != NULL));
    ASSERT((puiDataLength != NULL) && (*puiDataLength > 0));


    /* Since we want the whole document, we dont need to adjust the data in the ppvData buffer, so we just return here */
    if ( (uiChunkStart == 0) && (uiChunkEnd == 0) ) {
        return (SRCH_NoError);
    }


    /* If the start byte is outside the document entirely, there is not much we can do */
    if ( uiChunkStart > *puiDataLength ) {

        iUtlLogWarn(UTL_LOG_CONTEXT, "Retrieval beyond bounds of document, document key: '%s', byte range: %u %u, item: '%s', mime type: '%s', index: '%s'.",
            pucDocumentKey, uiChunkStart, uiChunkEnd, pucItemName, pucMimeType, psiSrchIndex->pucIndexName);

        /* Bail */
        return (SRCH_InvalidChunkRange);
    }


    /* Override the chunk end if it is beyond the end of the document */
    if ( uiChunkEnd > *puiDataLength ) { 
        uiChunkEnd = *puiDataLength;      
    }


    /* Set the return pointers, we may need to move the data in the ppvData buffer
    ** because the client may be asking for an offset into the document
    */
    *puiDataLength = (uiChunkEnd - uiChunkStart) + 1;
    if ( uiChunkStart > 0 ) {
        s_memmove((unsigned char *)*ppvData, (unsigned char *)(*ppvData) + uiChunkStart, *puiDataLength);
    }

    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchRetrievalRetrieveFromFile()

    Purpose:    Retrieves a document chunk from the index.

    Parameters: psiSrchIndex        search index structure
                pucDocumentKey      document key (for logging only)
                pucItemName         document item name (for logging only)
                pucMimeType         document mime type (for logging only)
                uiChunkStart        start index of the chunk
                uiChunkEnd          end index of the chunk
                pucFilePath         file path for the document
                zStartOffset        start index of the document in the document file
                zEndOffset          end index of the document in the document file
                ppvData             return pointer for the data retrieved
                puiDataLength       return pointer for the length of the data retrieved

    Globals:    none

    Returns:    SRCH error code

                Note that the error should always be checked because we may return a 
                document chunk and generate an error such as an out of range error.

*/
static int iSrchRetrievalRetrieveFromFile
(
    struct srchIndex *psiSrchIndex,
    unsigned char *pucDocumentKey,
    unsigned char *pucItemName,
    unsigned char *pucMimeType,
    unsigned int uiChunkStart,
    unsigned int uiChunkEnd,
    unsigned char *pucFilePath,
    off_t zStartOffset,
    off_t zEndOffset,
    void **ppvData,
    unsigned int *puiDataLength
)
{

    int             iError = SRCH_NoError;
    FILE            *pfFile = NULL;
    off_t           zStartByte = 0;
    off_t           zEndByte = 0;
    unsigned int    uiBytesToRead = 0;
    unsigned int    uiBytesRead = 0;    
    unsigned char   *pucBuffer = NULL;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iSrchRetrievalRetrieveFromFile - pucFilePath [%s], zStartOffset [%ld], zEndOffset [%ld], pucItemName [%s], pucMimeType [%s], uiChunkStart [%u], uiChunkEnd [%u]", */
/*             pucFilePath, zStartOffset, zEndOffset, pucItemName, pucMimeType, uiChunkStart, uiChunkEnd); */


    ASSERT(psiSrchIndex != NULL);
    ASSERT(bUtlStringsIsStringNULL(pucDocumentKey) == false);
    ASSERT(bUtlStringsIsStringNULL(pucFilePath) == false);
    ASSERT(zStartOffset >= 0);
    ASSERT(zEndOffset >= 0);
    ASSERT(zEndOffset >= zStartOffset);
    ASSERT(bUtlStringsIsStringNULL(pucItemName) == false);
    ASSERT(bUtlStringsIsStringNULL(pucMimeType) == false);
    ASSERT(uiChunkStart >= 0);
    ASSERT(uiChunkEnd >= 0);
    ASSERT(uiChunkEnd >= uiChunkStart);
    ASSERT(ppvData != NULL);
    ASSERT(puiDataLength != NULL);


    /* Check to see if the file is there */
    if ( bUtlFileIsFile(pucFilePath) == false ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to find document file: '%s', document key: '%s', byte range: %u %u, item: '%s', mime type: '%s', from index: '%s'.", 
                pucFilePath, pucDocumentKey, uiChunkStart, uiChunkEnd, pucItemName, pucMimeType, psiSrchIndex->pucIndexName);
        return (SRCH_RetrieveDocumentFailed);
    }
    
    /* File is there, check that the file can be accessed */
    if ( bUtlFilePathRead(pucFilePath) == false ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to access document file: '%s', document key: '%s', byte range: %u %u, item: '%s', mime type: '%s', from index: '%s'.", 
                pucFilePath, pucDocumentKey, uiChunkStart, uiChunkEnd, pucItemName, pucMimeType, psiSrchIndex->pucIndexName);
        return (SRCH_RetrieveDocumentFailed);
    }


    /* It can be accessed, now we open it */
    if ( (pfFile = s_fopen(pucFilePath, "r")) == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to open document file: '%s', document key: '%s', byte range: %u %u, item: '%s', mime type: '%s', from index: '%s'.", 
                pucFilePath, pucDocumentKey, uiChunkStart, uiChunkEnd, pucItemName, pucMimeType, psiSrchIndex->pucIndexName);
        return (SRCH_RetrieveDocumentFailed);
    }


    /* Get the offsets of the document in the file, if we get passed 0 for both start and end, we get the whole document */
    if ( (uiChunkStart == 0) && (uiChunkEnd == 0) ) {
        zStartByte = zStartOffset;
        zEndByte = zEndOffset;
    }
    else {
        zStartByte = zStartOffset + uiChunkStart;
        zEndByte = zStartOffset + uiChunkEnd;
    }


    /* Check if the start byte is somewhere inside the document */
    if ( (zEndOffset != 0) && (zEndByte > zEndOffset) ) { 

        /* If the start byte is outside the document entirely, there is not much we can do */
        if ( zStartByte > zEndOffset ) {

            iUtlLogWarn(UTL_LOG_CONTEXT, "Retrieval beyond bounds of document, document key: '%s', byte range: %u %u, item: '%s', mime type: '%s', from index: '%s'.",
                    pucDocumentKey, uiChunkStart, uiChunkEnd, pucItemName, pucMimeType, psiSrchIndex->pucIndexName);

            /* Set the error and bail */
            iError = SRCH_InvalidChunkRange;
            goto bailFromiSrchRetrievalRetrieveFromFile;
        }

        zEndByte = zEndOffset;      
    }


    /* See if we can seek to the start position we want to read from */
    if ( s_fseek(pfFile, zStartByte, SEEK_SET) != 0 )   { 

        iUtlLogError(UTL_LOG_CONTEXT, "Failed to seek in document file: '%s', start byte: %ld, end byte: %ld, document key: '%s', byte range: %u %u, item: '%s', mime type: '%s', from index: '%s'.",
                    pucFilePath, zStartByte, zEndByte, pucDocumentKey, uiChunkStart, uiChunkEnd, pucItemName, pucMimeType, psiSrchIndex->pucIndexName);

        /* Set the error and bail */
        iError = SRCH_InvalidChunkRange;
        goto bailFromiSrchRetrievalRetrieveFromFile;
    }


    /* Calculate how many bytes we will read and allocate the memory (with pad) */
    uiBytesToRead = (zEndByte - zStartByte) + 1;
    if ( (pucBuffer = (unsigned char *)s_malloc((size_t)(uiBytesToRead + 1))) == NULL ) {
        iError = SRCH_MemError;
        goto bailFromiSrchRetrievalRetrieveFromFile;
    }


    /* Read in the bytes, did we get all the bytes we wanted? actually it is ok, 
    ** to not read all the bytes we wanted, the client may well have specified a 
    ** range which overshot the end of the file
    */
    if ( (uiBytesRead = s_fread(pucBuffer, (size_t)sizeof(unsigned char), (size_t)uiBytesToRead, pfFile)) != uiBytesToRead ) { 
        iUtlLogWarn(UTL_LOG_CONTEXT, "Retrieval beyond bounds of document file: '%s', start byte: %ld, end byte: %ld, document ID: '%s', byte range: %u %u, item: '%s', mime type: '%s', from index: '%s'.",
                    pucFilePath, zStartByte, zEndByte, pucDocumentKey, uiChunkStart, uiChunkEnd, pucItemName, pucMimeType, psiSrchIndex->pucIndexName);
    }


    /* Set the return pointers */
    *puiDataLength = uiBytesRead;
    *ppvData = (void *)pucBuffer;


    
    /* Bail label */
    bailFromiSrchRetrievalRetrieveFromFile:


    /* Close the file */
    s_fclose(pfFile);


    return (iError);

}


/*---------------------------------------------------------------------------*/
