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

    Module:     document.c

    Author:     Francois Schiettecatte (FS Consulting LLC.)

    Created:    14 September 1995

    Purpose:    This module contains all the functions which make up the 
                document management interface of the search engine.

                Here we can store/retrieve document information, delete
                documents and check document IDs.

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
#define UTL_LOG_CONTEXT                     (unsigned char *)"com.fsconsult.mps.src.search.document"


/* Enable this to require document title */
/* #define SRCH_DOCUMENT_REQUIRE_DOCUMENT_TITLE */

/* Enable this to require document items */
/* #define SRCH_DOCUMENT_REQUIRE_DOCUMENT_ITEMS */


/*---------------------------------------------------------------------------*/


/*
** Private function prototypes
*/ 

/* Document table management functions */
static int iSrchDocumentGetDocumentTableEntry (struct srchIndex *psiSrchIndex, unsigned int uiDocumentID,
        unsigned long *pulDocumentDataID, unsigned int *puiRank, unsigned int *puiTermCount,
        unsigned long *pulAnsiDate, unsigned int *puiLanguageID);

static int iSrchDocumentSaveDocumentTableEntry (struct srchIndex *psiSrchIndex, unsigned int uiDocumentID,
        unsigned long ulDocumentDataID, unsigned int uiRank, unsigned int uiTermCount,
        unsigned long ulAnsiDate, unsigned int uiLanguageID);

static int iSrchDocumentReadDocumentTableEntryCallBack (unsigned long ulEntryID, void *pvEntryData,
        unsigned int uiEntryLength, va_list ap);


/* Document data management functions */
static int iSrchDocumentGetDocumentDataEntry (struct srchIndex *psiSrchIndex, unsigned long ulDocumentDataID,
        unsigned char **ppucTitle, unsigned char **ppucDocumentKey, 
        struct srchDocumentItem **ppsdiSrchDocumentItems, unsigned int *puiSrchDocumentItemsLength,
        unsigned int uiItemID, boolean bLoadTypeFilePaths, boolean bLoadTypeUrls, boolean bLoadStoredData);

static int iSrchDocumentSaveDocumentDataEntry (struct srchIndex *psiSrchIndex, unsigned char *pucTitle,
        unsigned char *pucDocumentKey, struct srchDocumentItem *psdiSrchDocumentItems, unsigned int uiSrchDocumentItemsLength, 
        unsigned long *pulDocumentDataID);


/*---------------------------------------------------------------------------*/


/* 
** ========================================
** ===  Document Information Management ===
** ========================================
*/


/*

    Function:   iSrchDocumentGetDocumentInfo()

    Purpose:    This function gets the document information in the index
                for the specified document

                The document title and document key information will only
                be retrieved if the return pointers are supplied.

                The document type profile will only be retrieved if the 
                return pointers are supplied as well as the return 
                pointers for the document title and document key 

    Parameters: psiSrchIndex                    search index structure
                uiDocumentID                    document ID to read
                ppucTitle                       return pointer for the title (optional)
                ppucDocumentKey                 return pointer for the document key (optional)
                puiRank                         return pointer for the rank (optional)
                puiTermCount                    return pointer for the term count (optional)
                pulAnsiDate                     return pointer for the ansi date (optional)
                puiLanguageID                   return pointer for the language ID (optional)
                ppsdiSrchDocumentItems          return pointer for the search document item structures, passed as an array
                                                of search document item structures (optional)
                puiSrchDocumentItemsLength      return pointer for the number of search document item structures, passed as the 
                                                number of search document item structures entries allocated in the array (optional)
                uiItemID                        Item ID to read into ppsdiSrchDocumentItems (0 to ignore)
                bLoadTypeFilePaths              set to true to load type file paths into ppsdiSrchDocumentItems
                bLoadTypeUrls                   set to true to load type URLs into ppsdiSrchDocumentItems
                bLoadStoredData                 set to true to load stored data into ppsdiSrchDocumentItems

    Globals:    none

    Returns:    SRCH error code

*/
int iSrchDocumentGetDocumentInfo
(
    struct srchIndex *psiSrchIndex,
    unsigned int uiDocumentID,
    unsigned char **ppucTitle,
    unsigned char **ppucDocumentKey,
    unsigned int *puiRank,
    unsigned int *puiTermCount,
    unsigned long *pulAnsiDate,
    unsigned int *puiLanguageID,
    struct srchDocumentItem **ppsdiSrchDocumentItems,
    unsigned int *puiSrchDocumentItemsLength,
    unsigned int uiItemID,
    boolean bLoadTypeFilePaths,
    boolean bLoadTypeUrls,
    boolean bLoadStoredData
)
{

    int             iError = SRCH_NoError;
    unsigned long   ulDocumentDataID = 0;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iSrchDocumentGetDocumentInfo: %u", uiDocumentID); */


    /* Check the parameters */
    if ( psiSrchIndex == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'psiSrchIndex' parameter passed to 'iSrchDocumentGetDocumentInfo'."); 
        return (SRCH_InvalidIndex);
    }

    if ( uiDocumentID <= 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiDocumentID' parameter passed to 'iSrchDocumentGetDocumentInfo'."); 
        return (SRCH_DocumentInvalidDocumentID);
    }


    /* Get the document table entry information */
    if ( (iError = iSrchDocumentGetDocumentTableEntry(psiSrchIndex, uiDocumentID, &ulDocumentDataID, puiRank, puiTermCount, 
            pulAnsiDate, puiLanguageID)) != SRCH_NoError ) {
        return (iError);
    }


    /* Get the title and document item information if return pointers were specified */
    if ( (ppucTitle != NULL) || ((ppucTitle != NULL) && (*ppucTitle != NULL)) || 
            (ppucDocumentKey != NULL) || ((ppucDocumentKey != NULL) && (*ppucDocumentKey != NULL)) ||
            ((ppsdiSrchDocumentItems != NULL) && (puiSrchDocumentItemsLength != NULL)) ) {

        /* Get the document data entry */
        if ( (iError = iSrchDocumentGetDocumentDataEntry(psiSrchIndex, ulDocumentDataID, ppucTitle, ppucDocumentKey, 
                ppsdiSrchDocumentItems, puiSrchDocumentItemsLength, uiItemID, bLoadTypeFilePaths, bLoadTypeUrls, bLoadStoredData)) != SRCH_NoError ) {
            return (iError);
        }
    }


    return (SRCH_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchDocumentSaveDocumentInfo()

    Purpose:    This function sets the document information in the index
                for the document currently being indexed.

                The function first adds the document data entry to the repository
                and then adds an entry to the document table.

                If we are updating the index, it will check to see if the
                document key already exists in the index.

    Parameters: psiSrchIndex                search index structure
                uiDocumentID                document ID
                pucTitle                    title
                pucDocumentKey              document key
                uiRank                      rank
                uiTermCount                 term count
                ulAnsiDate                  ansi date
                uiLanguageID                language ID
                psdiSrchDocumentItems       array of search document item structures
                uiSrchDocumentItemsLength   number of entries in the search document item structures array

    Globals:    none

    Returns:    SRCH Error code

*/
int iSrchDocumentSaveDocumentInfo
(
    struct srchIndex *psiSrchIndex,
    unsigned int uiDocumentID,
    unsigned char *pucTitle,
    unsigned char *pucDocumentKey,
    unsigned int uiRank,
    unsigned int uiTermCount,
    unsigned long ulAnsiDate,
    unsigned int uiLanguageID,
    struct srchDocumentItem *psdiSrchDocumentItems,
    unsigned int uiSrchDocumentItemsLength
)
{

    int             iError = SRCH_NoError;
    unsigned long   ulDocumentDataID = 0;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iSrchDocumentSaveDocumentInfo"); */


    /* Check the parameters */
    if ( psiSrchIndex == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'psiSrchIndex' parameter passed to 'iSrchDocumentSaveDocumentInfo'."); 
        return (SRCH_InvalidIndex);
    }

    if ( psiSrchIndex->psibSrchIndexBuild == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'psiSrchIndex->psibSrchIndexBuild' parameter passed to 'iSrchDocumentSaveDocumentInfo'."); 
        return (SRCH_InvalidIndex);
    }

    if ( uiDocumentID <= 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiDocumentID' parameter passed to 'iSrchDocumentSaveDocumentInfo'."); 
        return (SRCH_DocumentInvalidDocumentID);
    }

#if defined(SRCH_DOCUMENT_REQUIRE_DOCUMENT_TITLE)
    if ( bUtlStringsIsStringNULL(pucTitle) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pucTitle' parameter passed to 'iSrchDocumentSaveDocumentInfo'."); 
        return (SRCH_DocumentInvalidDocumentTitle);
    }
#endif    /* defined(SRCH_DOCUMENT_REQUIRE_DOCUMENT_TITLE) */

    if ( bUtlStringsIsStringNULL(pucDocumentKey) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pucDocumentKey' parameter passed to 'iSrchDocumentSaveDocumentInfo'."); 
        return (SRCH_DocumentInvalidDocumentKey);
    }

#if defined(SRCH_DOCUMENT_REQUIRE_DOCUMENT_ITEMS)
    if ( psdiSrchDocumentItems == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'psdiSrchDocumentItems' parameter passed to 'iSrchDocumentSaveDocumentInfo'."); 
        return (SRCH_DocumentInvalidDocumentItems);
    }

    if ( uiSrchDocumentItemsLength <= 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiSrchDocumentItemsLength' parameter passed to 'iSrchDocumentSaveDocumentInfo'."); 
        return (SRCH_DocumentInvalidDocumentItemsLength);
    }
#endif    /* defined(SRCH_DOCUMENT_REQUIRE_DOCUMENT_ITEMS) */



    /* Save the document data entry, this generates a document data ID which is then stored in the document table */
    if ( (iError = iSrchDocumentSaveDocumentDataEntry(psiSrchIndex, pucTitle, pucDocumentKey, psdiSrchDocumentItems,
            uiSrchDocumentItemsLength, &ulDocumentDataID)) != SRCH_NoError ) {
        return (iError);
    }


    /* Save the document table entry */
    if ( (iError = iSrchDocumentSaveDocumentTableEntry(psiSrchIndex, uiDocumentID, ulDocumentDataID, uiRank, 
            uiTermCount, ulAnsiDate, uiLanguageID)) != SRCH_NoError ) {
        return (iError);
    }


    return (SRCH_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchDocumentGetNewDocumentID()

    Purpose:    Generate and return a new document ID

    Parameters: psiSrchIndex    search index structure
                puiDocumentID   return pointer for the document ID

    Globals:    none

    Returns:    SRCH error code

*/
int iSrchDocumentGetNewDocumentID
(
    struct srchIndex *psiSrchIndex,
    unsigned int *puiDocumentID
)
{

/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iSrchDocumentGetNewDocumentID, uiDocumentCount: %u", psiSrchIndex->uiDocumentCount); */


    /* Check the parameters */
    if ( psiSrchIndex == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'psiSrchIndex' parameter passed to 'iSrchDocumentGetNewDocumentID'."); 
        return (SRCH_InvalidIndex);
    }

    if ( puiDocumentID == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'puiDocumentID' parameter passed to 'iSrchDocumentGetNewDocumentID'."); 
        return (SRCH_ReturnParameterError);
    }


    /* Increment the document counter */
    psiSrchIndex->uiDocumentCount++;

    /* Set the return pointer */
    *puiDocumentID = psiSrchIndex->uiDocumentCount;


    return (SRCH_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchDocumentValidateDocumentID() 

    Purpose:    Checks to see if the passed document ID is valid.

    Parameters: psiSrchIndex    search index structure
                uiDocumentID    document id

    Globals:    none

    Returns:    SRCH error code

*/
int iSrchDocumentValidateDocumentID
(
    struct srchIndex *psiSrchIndex,
    unsigned int uiDocumentID
)
{

    int             iError = SRCH_NoError;
    unsigned long   ulDummy = 0;
    unsigned int    uiDummy = 0;
    time_t          tDummy = 0;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iSrchDocumentValidateDocumentID, uiDocumentID: %u", uiDocumentID); */


    /* Check the parameters */
    if ( psiSrchIndex == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'psiSrchIndex' parameter passed to 'iSrchDocumentValidateDocumentID'."); 
        return (SRCH_InvalidIndex);
    }

    if ( uiDocumentID <= 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiDocumentID' parameter passed to 'iSrchDocumentValidateDocumentID'."); 
        return (SRCH_DocumentInvalidDocumentID);
    }


    /* Get the table entry for this document ID */
    if ( (iError = iUtlTableProcessEntry(psiSrchIndex->pvUtlDocumentTable, uiDocumentID, (int (*)())iSrchDocumentReadDocumentTableEntryCallBack, 
            &ulDummy, &uiDummy, &uiDummy, &uiDummy, &uiDummy, &tDummy, &uiDummy, &uiDummy)) != UTL_NoError ) {

        iUtlLogError(UTL_LOG_CONTEXT, "Failed to get a document table entry, document ID: %lu, index: '%s', utl error: %d.", 
                uiDocumentID, psiSrchIndex->pucIndexName, iError);

        /* Remape the utl error as needed */
        return ((iError == UTL_TableInvalidTableEntryID) ? SRCH_DocumentInvalidDocumentID : SRCH_DocumentCheckDocumentIDFailed);
    }


    return (SRCH_NoError);

}


/*---------------------------------------------------------------------------*/


/* 
** ========================================
** ===  Document Table Entry Management ===
** ========================================
*/


/*

    Function:   iSrchDocumentGetDocumentTableEntry()

    Purpose:    Get the document table entry from the document table

    Parameters: psiSrchIndex        search index structure
                uiDocumentID        document ID to read
                pulDocumentDataID   return pointer for the document data ID (optional)
                puiRank             return pointer for the rank (optional)
                puiTermCount        return pointer for the term count (optional)
                pulAnsiDate         return pointer for the ansi date (optional)
                puiLanguageID       return pointer for the language ID (optional)

    Globals:    none

    Returns:    SRCH error code

*/
static int iSrchDocumentGetDocumentTableEntry
(
    struct srchIndex *psiSrchIndex,
    unsigned int uiDocumentID,
    unsigned long *pulDocumentDataID,
    unsigned int *puiRank,
    unsigned int *puiTermCount,
    unsigned long *pulAnsiDate,
    unsigned int *puiLanguageID
)
{

    int     iError = SRCH_NoError;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iSrchDocumentGetDocumentTableEntry, uiDocumentID: %u", uiDocumentID); */


    ASSERT(psiSrchIndex != NULL);
    ASSERT(uiDocumentID > 0);


    /* Get the table entry for this document ID */
    if ( (iError = iUtlTableProcessEntry(psiSrchIndex->pvUtlDocumentTable, uiDocumentID, (int (*)())iSrchDocumentReadDocumentTableEntryCallBack, 
            pulDocumentDataID, puiRank, puiTermCount, pulAnsiDate, puiLanguageID)) != UTL_NoError ) {

        iUtlLogError(UTL_LOG_CONTEXT, "Failed to get a document table entry, document ID: %lu, index: '%s', utl error: %d.", 
                uiDocumentID, psiSrchIndex->pucIndexName, iError);
        
        /* Remap the error as needed */
        return ((iError == UTL_TableInvalidTableEntryID) ? SRCH_DocumentInvalidDocumentID : SRCH_DocumentGetDocumentTableEntryFailed);
    }


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iSrchDocumentGetDocumentTableEntry, *pulDocumentDataID: %lu, *puiRank: %u, *puiTermCount: %u, *pulAnsiDate: %lu, *puiLanguageID: %u",  */
/*             (pulDocumentDataID != NULL) ? (long)*pulDocumentDataID : 0,  */
/*             (puiRank != NULL) ? *puiRank : (unsigned int)0,  */
/*             (puiTermCount != NULL) ? *puiTermCount : (unsigned int)0,  */
/*             (pulAnsiDate != NULL) ? *pulAnsiDate : (unsigned long)0,  */
/*             (puiLanguageID != NULL) ? *puiLanguageID : (unsigned int)0); */


    return (SRCH_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchDocumentSaveDocumentTableEntry()

    Purpose:    Set the document table entry to the document table

    Parameters: psiSrchIndex        search index structure
                uiDocumentID        the document ID
                ulDocumentDataID    document data ID
                uiRank              rank
                uiTermCount         term count
                ulAnsiDate          ansi date
                uiLanguageID        language ID

    Globals:    none

    Returns:    SRCH error code

*/
static int iSrchDocumentSaveDocumentTableEntry
(
    struct srchIndex *psiSrchIndex,
    unsigned int uiDocumentID,
    unsigned long ulDocumentDataID,
    unsigned int uiRank,
    unsigned int uiTermCount,
    unsigned long ulAnsiDate,
    unsigned int uiLanguageID
)
{

    int             iError = SRCH_NoError;
    unsigned char   pucBuffer[SRCH_DOCUMENT_ENTRY_LENGTH] = {'\0'};
    unsigned char   *pucBufferPtr = pucBuffer;
    unsigned long   ulDocumentTableID = 0;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iSrchDocumentSaveDocumentTableEntry, ulDocumentDataID: %lu, uiRank: %u, uiTermCount: %u, ulAnsiDate: %lu, uiLanguageID: %u",  */
/*                 ulDocumentDataID, uiRank, uiTermCount, ulAnsiDate, uiLanguageID); */


    ASSERT(psiSrchIndex != NULL);
    ASSERT(uiDocumentID > 0);
    ASSERT(ulDocumentDataID >= 0);
    ASSERT(uiRank >= 0);
    ASSERT(uiTermCount >= 0);
    ASSERT(ulAnsiDate >= 0);
    ASSERT(uiLanguageID >= 0);


    /* Set the buffer pointer */
    pucBufferPtr = pucBuffer;

    /* Write the document data ID to the buffer (increments the pointer) */
    UTL_NUM_WRITE_ULONG(ulDocumentDataID, SRCH_DOCUMENT_DATA_ID_SIZE, pucBufferPtr);

    /* Write the document rank to the buffer (increments the pointer) */
    UTL_NUM_WRITE_UINT(uiRank, SRCH_DOCUMENT_RANK_SIZE, pucBufferPtr);

    /* Write the term count to the buffer (increments the pointer) */
    UTL_NUM_WRITE_UINT(uiTermCount, SRCH_DOCUMENT_TERM_COUNT_SIZE, pucBufferPtr);

    /* Write the document ansi date to the buffer (increments the pointer) */
    UTL_NUM_WRITE_ULONG(ulAnsiDate, SRCH_DOCUMENT_DATE_SIZE, pucBufferPtr);

    /* Write the document language ID to the buffer (increments the pointer) */
    UTL_NUM_WRITE_UINT(uiLanguageID, SRCH_DOCUMENT_LANGUAGE_ID_SIZE, pucBufferPtr);


    /* Add the document table entry to the document table */
    if ( (iError = iUtlTableAddEntry(psiSrchIndex->pvUtlDocumentTable, pucBuffer, &ulDocumentTableID)) != UTL_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to add a document table entry, document ID: %lu, index: '%s', utl error: %d.", 
                uiDocumentID, psiSrchIndex->pucIndexName, iError);
        return (SRCH_DocumentSaveDocumentTableEntryFailed);
    }


    /* Check that the document ID, the document table ID and the document are in sync */
    ASSERT(ulDocumentTableID == uiDocumentID);
    ASSERT(ulDocumentTableID == psiSrchIndex->uiDocumentCount);


    return (SRCH_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchDocumentReadDocumentTableEntryCallBack()

    Purpose:    This function is passed to the document table access functions
                to extract data from the buffer.

    Parameters: ulEntryID       entry ID
                pvEntryData     entry data
                uiEntryLength   entry length
                ap              arg list

    Globals:    none

    Returns:    0 to continue processing, non-0 otherwise

*/
static int iSrchDocumentReadDocumentTableEntryCallBack
(
    unsigned long ulEntryID,
    void *pvEntryData,
    unsigned int uiEntryLength,
    va_list ap
)
{

    va_list         ap_;

    /* Get a pointer to the data */
    unsigned char   *pucEntryDataPtr = (unsigned char *)pvEntryData;

    unsigned long   ulDocumentDataID = 0;
    unsigned int    uiRank = 0;
    unsigned int    uiTermCount = 0;
    unsigned long   ulAnsiDate = 0;
    unsigned int    uiLanguageID = 0;


    ASSERT(ulEntryID >= 0);
    ASSERT(pvEntryData != NULL);
    ASSERT(uiEntryLength > 0);


    /* Get all our parameters, note that we make a copy of 'ap' */
    va_copy(ap_, ap);
    unsigned long   *pulDocumentDataID = (unsigned long *)va_arg(ap_, unsigned long *);
    unsigned int    *puiRank = (unsigned int *)va_arg(ap_, unsigned int *);
    unsigned int    *puiTermCount = (unsigned int *)va_arg(ap_, unsigned int *);
    unsigned long   *pulAnsiDate = (unsigned long *)va_arg(ap_, unsigned long *);
    unsigned int    *puiLanguageID = (unsigned int *)va_arg(ap_, unsigned int *);
    va_end(ap_);


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iSrchDocumentReadDocumentTableEntryCallBack [%u][%u][%s]", ulEntryID, uiEntryLength, */
/*             (pvEntryData != NULL) ? "(pvEntryData != NULL)" : "(pvEntryData == NULL)"); */


    ASSERT((pulDocumentDataID != NULL) || (pulDocumentDataID == NULL));
    ASSERT((puiRank != NULL) || (puiRank == NULL));
    ASSERT((puiTermCount != NULL) || (puiTermCount == NULL));
    ASSERT((pulAnsiDate != NULL) || (pulAnsiDate == NULL));
    ASSERT((puiLanguageID != NULL) || (puiLanguageID == NULL));


    /* Decode to a variable and copy to the pointers for optimization */

    /* Read the document data ID from the buffer (increments the pointer) */
    if ( pulDocumentDataID != NULL ) {
        UTL_NUM_READ_ULONG(ulDocumentDataID, SRCH_DOCUMENT_DATA_ID_SIZE, pucEntryDataPtr);
        *pulDocumentDataID = ulDocumentDataID;
    }
    else {
        pucEntryDataPtr += SRCH_DOCUMENT_DATA_ID_SIZE;
    }

    /* Read the document rank from the buffer (increments the pointer) */
    if ( puiRank != NULL ) {
        UTL_NUM_READ_UINT(uiRank, SRCH_DOCUMENT_RANK_SIZE, pucEntryDataPtr);
        *puiRank = uiRank;
    }
    else {
        pucEntryDataPtr += SRCH_DOCUMENT_RANK_SIZE;
    }

    /* Read the term count from the buffer (increments the pointer) */
    if ( puiTermCount != NULL ) {
        UTL_NUM_READ_UINT(uiTermCount, SRCH_DOCUMENT_TERM_COUNT_SIZE, pucEntryDataPtr);
        *puiTermCount = uiTermCount;
    }
    else {
        pucEntryDataPtr += SRCH_DOCUMENT_TERM_COUNT_SIZE;
    }

    /* Read the document date from the buffer (increments the pointer) */
    if ( pulAnsiDate != NULL ) {
        UTL_NUM_READ_ULONG(ulAnsiDate, SRCH_DOCUMENT_DATE_SIZE, pucEntryDataPtr);
        *pulAnsiDate = ulAnsiDate;
    }
    else {
        pucEntryDataPtr += SRCH_DOCUMENT_DATE_SIZE;
    }

    /* Read the document language ID from the buffer (increments the pointer) */
    if ( puiLanguageID != NULL ) {
        UTL_NUM_READ_UINT(uiLanguageID, SRCH_DOCUMENT_LANGUAGE_ID_SIZE, pucEntryDataPtr);
        *puiLanguageID = uiLanguageID;
    }
    else {
        pucEntryDataPtr += SRCH_DOCUMENT_LANGUAGE_ID_SIZE;
    }


    return (0);

}


/*---------------------------------------------------------------------------*/


/* 
** =======================================
** ===  Document Data Entry Management ===
** =======================================
*/


/*

    Function:   iSrchDocumentGetDocumentDataEntry()

    Purpose:    Get the document data entry

    Parameters: psiSrchIndex                    search index structure
                ulDocumentDataID                document data ID
                ppucTitle                       return pointer for the title (optional)
                ppucDocumentKey                 return pointer for the document key (optional)
                ppsdiSrchDocumentItems          return pointer for the search document item structures, passed as an array
                                                of search document item structures (optional)
                puiSrchDocumentItemsLength      return pointer for the number of search document item structures, passed as the 
                                                number of search document item structures entries allocated in the array (optional)
                uiItemID                        Item ID to read into ppsdiSrchDocumentItems (0 to ignore)
                bLoadTypeFilePaths              set to true to load type file paths into ppsdiSrchDocumentItems
                bLoadTypeUrls                   set to true to load type URLs into ppsdiSrchDocumentItems
                bLoadStoredData                 set to true to load stored data into ppsdiSrchDocumentItems

    Globals:    none

    Returns:    SRCH error code

*/
static int iSrchDocumentGetDocumentDataEntry
(
    struct srchIndex *psiSrchIndex,
    unsigned long ulDocumentDataID,
    unsigned char **ppucTitle,
    unsigned char **ppucDocumentKey,
    struct srchDocumentItem **ppsdiSrchDocumentItems,
    unsigned int *puiSrchDocumentItemsLength,
    unsigned int uiItemID,
    boolean bLoadTypeFilePaths,
    boolean bLoadTypeUrls,
    boolean bLoadStoredData
)
{

    int                         iError = SRCH_NoError;
    boolean                     bTitleAllocated = false;
    boolean                     bDocumentKeyAllocated = false;
    unsigned char               *pucBuffer = NULL;
    unsigned char               *pucBufferPtr = NULL;
    unsigned int                uiBufferLength = 0;
    struct srchDocumentItem     *psdiSrchDocumentItems = NULL;
    struct srchDocumentItem     *psdiSrchDocumentItemsPtr = NULL;
    unsigned int                uiSrchDocumentItemsLength = 0;
    unsigned int                uiI = 0;
    unsigned int                uiDataLength = 0;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iSrchDocumentGetDocumentDataEntry, ulDocumentDataID: %lu", ulDocumentDataID); */


    ASSERT(psiSrchIndex != NULL);
    ASSERT(ulDocumentDataID >= 0);
    ASSERT((ppucTitle != NULL) || (ppucTitle == NULL));
    ASSERT((ppucDocumentKey != NULL) || (ppucDocumentKey == NULL));
    ASSERT(((ppsdiSrchDocumentItems != NULL) && (puiSrchDocumentItemsLength != NULL)) || ((ppsdiSrchDocumentItems == NULL) && (puiSrchDocumentItemsLength == NULL)));
    ASSERT(uiItemID >= 0);
    ASSERT((bLoadTypeFilePaths == true) || (bLoadTypeFilePaths == false));
    ASSERT((bLoadTypeUrls == true) || (bLoadTypeUrls == false));
    ASSERT((bLoadStoredData == true) || (bLoadStoredData == false));


    /* Get the document data entry */
    if ( (iError = iUtlDataGetEntry(psiSrchIndex->pvUtlDocumentData, ulDocumentDataID, (void **)&pucBuffer, &uiBufferLength)) != UTL_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to get a document data entry, document data ID: %lu, index: '%s', utl error: %d.", 
                ulDocumentDataID, psiSrchIndex->pucIndexName, iError);
        return (SRCH_DocumentGetDocumentDataEntryFailed);
    }


    /* Set the current buffer end */
    pucBufferPtr = pucBuffer;

    /* Read in the title */
    if ( ppucTitle != NULL ) {
        if ( *ppucTitle != NULL ) {
            /* The pointer is already allocated, so we copy the title (max length of SPI_TITLE_MAXIMUM_LENGTH) */
            s_strnncpy(*ppucTitle, pucBufferPtr, SPI_TITLE_MAXIMUM_LENGTH + 1);
        }
        else {
            /* The pointer was not allocated, but the return pointer was set, so we duplicate the title */
            if ( (*ppucTitle = (unsigned char *)s_strdup(pucBufferPtr)) == NULL ) {
                iError = SRCH_MemError;
                goto bailFromiSrchDocumentGetDocumentDataEntry;
            }
            bTitleAllocated = true;
        }
    }

    /* Set the buffer end */
    pucBufferPtr = (unsigned char *)s_strchr(pucBufferPtr, '\0') + 1;


    /* Read in the document key */
    if ( ppucDocumentKey != NULL ) {
        if ( *ppucDocumentKey != NULL ) {
            /* The pointer is already allocated, so we copy the document key (max len of SPI_DOCUMENT_KEY_MAXIMUM_LENGTH) */
            s_strnncpy(*ppucDocumentKey, pucBufferPtr, SPI_DOCUMENT_KEY_MAXIMUM_LENGTH + 1);
        }
        else {
            /* The pointer was not allocated, but the return pointer was set, so we duplicate the document key */
            if ( (*ppucDocumentKey = (unsigned char *)s_strdup(pucBufferPtr)) == NULL ) {
                iError = SRCH_MemError;
                goto bailFromiSrchDocumentGetDocumentDataEntry;
            }
            bDocumentKeyAllocated = true;
        }
    }

    /* Set the buffer end */
    pucBufferPtr = (unsigned char *)s_strchr(pucBufferPtr, '\0') + 1;


    /* Read the number of document types, also the number of sub-entries */
    UTL_NUM_READ_COMPRESSED_UINT(uiSrchDocumentItemsLength, pucBufferPtr);


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iSrchDocumentGetDocumentDataEntry, *ppucTitle: %s, *ppucDocumentKey: %s, uiSrchDocumentItemsLength: %u", */
/*             (ppucTitle != NULL) && (bUtlStringsIsStringNULL(*ppucTitle) == false) ? *ppucTitle : (unsigned char *)"(nul)",  */
/*             (ppucDocumentKey != NULL) && (bUtlStringsIsStringNULL(*ppucDocumentKey) == false) ? *ppucDocumentKey : (unsigned char *)"(null)",  */
/*             uiSrchDocumentItemsLength); */


    /* Do we want to read the type profile information? */
    if ( (ppsdiSrchDocumentItems != NULL) && (puiSrchDocumentItemsLength != NULL) ) {

        /* Are we looking for a specific item ID? */
        if ( uiItemID > 0 ) {
            
            /* Check that this is a valid item ID */
            if ( uiItemID > uiSrchDocumentItemsLength ) {
                iError = SRCH_DocumentInvalidItemID;
                goto bailFromiSrchDocumentGetDocumentDataEntry;
            }

            /* Allocate the search document items structure */
            if ( (psdiSrchDocumentItems = (struct srchDocumentItem *)s_malloc((size_t)(sizeof(struct srchDocumentItem)))) == NULL ) {
                iError = SRCH_MemError;
                goto bailFromiSrchDocumentGetDocumentDataEntry;
            }
            
            /* Set the return pointers */
            *puiSrchDocumentItemsLength = 1;
            *ppsdiSrchDocumentItems = psdiSrchDocumentItems;
        }
        
        else {
            
            /* Allocate the document items structure */
            if ( (psdiSrchDocumentItems = (struct srchDocumentItem *)s_malloc((size_t)(sizeof(struct srchDocumentItem) * uiSrchDocumentItemsLength))) == NULL ) {
                iError = SRCH_MemError;
                goto bailFromiSrchDocumentGetDocumentDataEntry;
            }

            /* Set the return pointers */
            *puiSrchDocumentItemsLength = uiSrchDocumentItemsLength;
            *ppsdiSrchDocumentItems = psdiSrchDocumentItems;
        }


        /* Loop reading each item */
        for ( uiI = 0, psdiSrchDocumentItemsPtr = psdiSrchDocumentItems; uiI < uiSrchDocumentItemsLength; uiI++ ) {
        
            /* Read the item ID from the buffer (increments the pointer) */
            UTL_NUM_READ_COMPRESSED_UINT(psdiSrchDocumentItemsPtr->uiItemID, pucBufferPtr);
            

            /* Skip this item if we are looking for a specific one and if this is not the one we are looking for */
            if ( (uiItemID > 0) && (psdiSrchDocumentItemsPtr->uiItemID != uiItemID) ) {
            
                /* This is not the item we are looking for, so we skip over it */
            
                /* Skip over the document length in the buffer (increments the pointer) */
                UTL_NUM_SKIP_COMPRESSED_UINT(pucBufferPtr);
                
                /* Skip over the URL in the buffer (incrementing the pointer) */
                pucBufferPtr = (unsigned char *)s_strchr(pucBufferPtr, '\0') + 1;
            
                /* Skip over the file path in the buffer (incrementing the pointer) */
                pucBufferPtr = (unsigned char *)s_strchr(pucBufferPtr, '\0') + 1;
            
                /* Skip over the start index in the buffer (increments the pointer) */
                UTL_NUM_SKIP_COMPRESSED_OFF_T(pucBufferPtr);
            
                /* Skip over the end index in the buffer (increments the pointer) */
                UTL_NUM_SKIP_COMPRESSED_OFF_T(pucBufferPtr);
            
                /* Read the data length in the buffer (increments the pointer) */
                UTL_NUM_READ_COMPRESSED_UINT(uiDataLength, pucBufferPtr);

                /* Increment the pointer to point to the next type entry */    
                pucBufferPtr += uiDataLength;
                
                continue;
            }


            /* Read the item length from the buffer (increments the pointer) */
            UTL_NUM_READ_COMPRESSED_UINT(psdiSrchDocumentItemsPtr->uiItemLength, pucBufferPtr);
        
            /* Read in the URL (incrementing the pointer) */
            s_strnncpy(psdiSrchDocumentItemsPtr->pucUrl, pucBufferPtr, SPI_URL_MAXIMUM_LENGTH + 1);
            pucBufferPtr = (unsigned char *)s_strchr(pucBufferPtr, '\0') + 1;

            /* Read in the file path (incrementing the pointer) */
            s_strnncpy(psdiSrchDocumentItemsPtr->pucFilePath, pucBufferPtr, UTL_FILE_PATH_MAX + 1);
            pucBufferPtr = (unsigned char *)s_strchr(pucBufferPtr, '\0') + 1;

            /* Read the start index from the buffer (increments the pointer) */
            UTL_NUM_READ_COMPRESSED_OFF_T(psdiSrchDocumentItemsPtr->zStartOffset, pucBufferPtr);
        
            /* Read the end index from the buffer (increments the pointer) */
            UTL_NUM_READ_COMPRESSED_OFF_T(psdiSrchDocumentItemsPtr->zEndOffset, pucBufferPtr);
    
            /* Read the data length from the buffer (increments the pointer) */
            UTL_NUM_READ_COMPRESSED_UINT(uiDataLength, pucBufferPtr);

            /* Get the stored data, but only if we have any */
            if ( (bLoadStoredData == true) && (uiDataLength > 0) ) {

                /* This is it, so we load it */
                if ( (psdiSrchDocumentItemsPtr->pvData = (void *)s_malloc((size_t)(uiDataLength + 1))) == NULL ) {
                    iError = SRCH_MemError;
                    goto bailFromiSrchDocumentGetDocumentDataEntry;
                }

                psdiSrchDocumentItemsPtr->uiDataLength = uiDataLength;
                s_memcpy(psdiSrchDocumentItemsPtr->pvData, pucBufferPtr, psdiSrchDocumentItemsPtr->uiDataLength);
                *((unsigned char *)psdiSrchDocumentItemsPtr->pvData + psdiSrchDocumentItemsPtr->uiDataLength) = '\0';
            }
            else {
                psdiSrchDocumentItemsPtr->pvData = NULL;
                psdiSrchDocumentItemsPtr->uiDataLength = 0;
            }
            
            /* Increment the pointer to point to the next document item */    
            pucBufferPtr += uiDataLength;

/*             iUtlLogDebug(UTL_LOG_CONTEXT, "iSrchDocumentGetDocumentDataEntry, uiItemID: %u, pucUrl: %s, pucFilePath: %s, zStartOffset: %ld, zEndOffset: %ld", */
/*                     psdiSrchDocumentItemsPtr->uiItemID, psdiSrchDocumentItemsPtr->pucUrl, psdiSrchDocumentItemsPtr->pucFilePath,  */
/*                     psdiSrchDocumentItemsPtr->zStartOffset, psdiSrchDocumentItemsPtr->zEndOffset); */


            /* Increment the document item pointer */
            psdiSrchDocumentItemsPtr++;
        }
    }



    /* Bail label */
    bailFromiSrchDocumentGetDocumentDataEntry:

    
    /* Handle the error */
    if ( iError != SRCH_NoError ) {
        
        /* Free the document items */
        for ( uiI = 0, psdiSrchDocumentItemsPtr = psdiSrchDocumentItems; uiI < uiSrchDocumentItemsLength; uiI++, psdiSrchDocumentItemsPtr++ ) {
            s_free(psdiSrchDocumentItemsPtr->pvData);
        }
        s_free(psdiSrchDocumentItems);

        if ( bTitleAllocated == true ) {
            s_free(*ppucTitle);
        }

        if ( bDocumentKeyAllocated == true ) {
            s_free(*ppucDocumentKey);
        }
    }


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchDocumentSaveDocumentDataEntry()

    Purpose:    Set the document data entry

    Parameters: psiSrchIndex                search index structure
                pucTitle                    title
                pucDocumentKey              document key
                psdiSrchDocumentItems       array of search document item structures
                uiSrchDocumentItemsLength   number of entries in the search document item structures array
                pulDocumentDataID           return pointer for the document data ID

    Globals:    none

    Returns:    SRCH error code

*/
static int iSrchDocumentSaveDocumentDataEntry
(
    struct srchIndex *psiSrchIndex,
    unsigned char *pucTitle,
    unsigned char *pucDocumentKey,
    struct srchDocumentItem *psdiSrchDocumentItems,
    unsigned int uiSrchDocumentItemsLength,
    unsigned long *pulDocumentDataID
)
{

    int             iError = SRCH_NoError;
    unsigned char   *pucBuffer = NULL;
    unsigned char   *pucBufferPtr = NULL;
    unsigned int    uiBufferLength = 0;
    unsigned int    uiI = 0;
    unsigned int    uiDocumentDataEntryLength = 0;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iSrchDocumentSaveDocumentDataEntry, pucTitle: %s, pucDocumentKey: %s, uiSrchDocumentItemsLength: %u", */
/*             pucUtlStringsGetSafeString(pucTitle), pucDocumentKey, uiSrchDocumentItemsLength); */


    ASSERT(psiSrchIndex != NULL);
#if defined(SRCH_DOCUMENT_REQUIRE_DOCUMENT_TITLE)
    ASSERT(bUtlStringsIsStringNULL(pucTitle) == false);
#endif    /* defined(SRCH_DOCUMENT_REQUIRE_DOCUMENT_TITLE) */
#if defined(SRCH_DOCUMENT_REQUIRE_DOCUMENT_ITEMS)
    ASSERT((uiSrchDocumentItemsLength > 0) && (psdiSrchDocumentItems != NULL));
#endif    /* defined(SRCH_DOCUMENT_REQUIRE_DOCUMENT_ITEMS) */
    ASSERT(pulDocumentDataID != NULL);


    /* Get the length of the document data entry block we are about to put together,
    ** note that we assume the max size for the unsigned int values
    */
    uiDocumentDataEntryLength = ((bUtlStringsIsStringNULL(pucTitle) == false) ? s_strlen(pucTitle) : 0) + 1;
    uiDocumentDataEntryLength += s_strlen(pucDocumentKey) + 1;
    uiDocumentDataEntryLength += UTL_NUM_COMPRESSED_UINT_MAX_SIZE;            /* Document item count */
    for ( uiI = 0; uiI < uiSrchDocumentItemsLength; uiI++ ) {
        uiDocumentDataEntryLength += (UTL_NUM_COMPRESSED_UINT_MAX_SIZE * 3);    /* Item ID, document length, data length */
        uiDocumentDataEntryLength += ((bUtlStringsIsStringNULL((psdiSrchDocumentItems + uiI)->pucUrl) == false) ? s_strlen((psdiSrchDocumentItems + uiI)->pucUrl) : 0) + 1;
        uiDocumentDataEntryLength += ((bUtlStringsIsStringNULL((psdiSrchDocumentItems + uiI)->pucFilePath) == false) ? s_strlen((psdiSrchDocumentItems + uiI)->pucFilePath) : 0) + 1;
        uiDocumentDataEntryLength += (UTL_NUM_COMPRESSED_OFF_T_MAX_SIZE * 2);    /* Start offset, end index */
        uiDocumentDataEntryLength += (psdiSrchDocumentItems + uiI)->uiDataLength;
    }


    /* Re/Allocate a pointer for the document data entry if needed */
    if ( uiDocumentDataEntryLength > psiSrchIndex->psibSrchIndexBuild->uiDocumentDataEntryLength ) {
    
        unsigned char    *pucDocumentDataEntryPtr = NULL;

        if ( (pucDocumentDataEntryPtr = (unsigned char *)s_realloc(psiSrchIndex->psibSrchIndexBuild->pucDocumentDataEntry,
                (size_t)(sizeof(unsigned char) * uiDocumentDataEntryLength))) == NULL ) {
            return (SRCH_MemError);
        }
        
        /* Hand over the pointer and length */
        psiSrchIndex->psibSrchIndexBuild->pucDocumentDataEntry = pucDocumentDataEntryPtr;
        psiSrchIndex->psibSrchIndexBuild->uiDocumentDataEntryLength = uiDocumentDataEntryLength;
    }
    

    /* Set the buffer start and current end */
    pucBuffer = psiSrchIndex->psibSrchIndexBuild->pucDocumentDataEntry;
    pucBufferPtr = pucBuffer;


    /* Write the title out along with the trailing NULL */
    if ( bUtlStringsIsStringNULL(pucTitle) == false ) {
        s_strcpy(pucBufferPtr, pucTitle);
        pucBufferPtr += s_strlen(pucTitle) + 1;
    }
    else {
        *pucBufferPtr = '\0';
        pucBufferPtr++;
    }


    /* Write the document key out along with the trailing NULL (downcase the key) */
    s_strcpy(pucBufferPtr, pucDocumentKey);
    pucBufferPtr += s_strlen(pucDocumentKey) + 1;


    /* Write out the number of documents items */
    UTL_NUM_WRITE_COMPRESSED_UINT(uiSrchDocumentItemsLength, pucBufferPtr);


    /* Loop writing out the document items */
    for ( uiI = 0; uiI < uiSrchDocumentItemsLength; uiI++ ) {

/*         iUtlLogDebug(UTL_LOG_CONTEXT, "iSrchDocumentSaveDocumentDataEntry, uiItemID: %u, pucUrl: %s, pucFilePath: %s, zStartOffset: %ld, zEndOffset: %ld", */
/*                 (psdiSrchDocumentItems + uiI)->uiItemID, (psdiSrchDocumentItems + uiI)->pucUrl, (psdiSrchDocumentItems + uiI)->pucFilePath,  */
/*                 (psdiSrchDocumentItems + uiI)->zStartOffset, (psdiSrchDocumentItems + uiI)->zEndOffset); */

        /* Write the item ID to the buffer (increments the pointer) */
        UTL_NUM_WRITE_COMPRESSED_UINT((psdiSrchDocumentItems + uiI)->uiItemID, pucBufferPtr);

        /* Write the document length to the buffer (increments the pointer) */
        UTL_NUM_WRITE_COMPRESSED_UINT((psdiSrchDocumentItems + uiI)->uiItemLength, pucBufferPtr);

        /* Write the URL out along with the trailing NULL (incrementing the pointer) */
        if ( bUtlStringsIsStringNULL((psdiSrchDocumentItems + uiI)->pucUrl) == false ) {
            s_strcpy(pucBufferPtr, (psdiSrchDocumentItems + uiI)->pucUrl);
            pucBufferPtr += s_strlen((psdiSrchDocumentItems + uiI)->pucUrl) + 1;
        }
        else {
            *pucBufferPtr = '\0';
            pucBufferPtr++;
        }

        /* Write the file path out along with the trailing NULL (incrementing the pointer) */
        if ( bUtlStringsIsStringNULL((psdiSrchDocumentItems + uiI)->pucFilePath) == false ) {
            s_strcpy(pucBufferPtr, (psdiSrchDocumentItems + uiI)->pucFilePath);
            pucBufferPtr += s_strlen((psdiSrchDocumentItems + uiI)->pucFilePath) + 1;
        }
        else {
            *pucBufferPtr = '\0';
            pucBufferPtr++;
        }

        /* Write the start index to the buffer (increments the pointer) */
        UTL_NUM_WRITE_COMPRESSED_OFF_T((psdiSrchDocumentItems + uiI)->zStartOffset, pucBufferPtr);

        /* Write the end index to the buffer (increments the pointer) */
        UTL_NUM_WRITE_COMPRESSED_OFF_T((psdiSrchDocumentItems + uiI)->zEndOffset, pucBufferPtr);

        /* Write the data length to the buffer (increments the pointer) */
        UTL_NUM_WRITE_COMPRESSED_UINT((psdiSrchDocumentItems + uiI)->uiDataLength, pucBufferPtr);

        /* Write out the type data (and increment the pointer) */
        if ( (psdiSrchDocumentItems + uiI)->uiDataLength > 0 ) {
            s_memcpy(pucBufferPtr, (psdiSrchDocumentItems + uiI)->pvData, (psdiSrchDocumentItems + uiI)->uiDataLength);
            pucBufferPtr += (psdiSrchDocumentItems + uiI)->uiDataLength;
        }
    }


    /* How long is the buffer? */
    uiBufferLength = pucBufferPtr - pucBuffer;


    /* Add the document data entry in the document data */
    if  ( (iError = iUtlDataAddEntry(psiSrchIndex->pvUtlDocumentData, (void *)pucBuffer, uiBufferLength, pulDocumentDataID)) != UTL_NoError ) { 
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to add a document data entry, index: '%s', utl error: %d.", psiSrchIndex->pucIndexName, iError);
        return (SRCH_DocumentSaveDocumentDataEntryFailed);
    }


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iSrchDocumentSaveDocumentDataEntry - *pulDocumentDataID: %lu", *pulDocumentDataID); */

    return (SRCH_NoError);

}


/*---------------------------------------------------------------------------*/
