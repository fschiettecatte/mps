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

    Module:     keydict.c

    Author:     Francois Schiettecatte (FS Consulting LLC.)

    Created:    14 September 1995

    Purpose:    This module contains all the functions which make up the 
                document keys dictionary management functionality.


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
#define UTL_LOG_CONTEXT                     (unsigned char *)"com.fsconsult.mps.src.search.keydict"


/*---------------------------------------------------------------------------*/


/*
** Defines
*/

/* These dictionary definitions are used to build the index dictionary blocks */
#define SRCH_KEY_DICT_ENTRY_FLAG_SIZE               (1)
#define SRCH_KEY_DICT_ENTRY_DOCUMENT_ID_SIZE        (4)
#define SRCH_KEY_DICT_ENTRY_KEY_SIZE                (2)

#define SRCH_KEY_DICT_ENTRY_HEADER_LENGTH           (SRCH_KEY_DICT_ENTRY_FLAG_SIZE + \
                                                            SRCH_KEY_DICT_ENTRY_DOCUMENT_ID_SIZE + \
                                                            SRCH_KEY_DICT_ENTRY_KEY_SIZE)

/* Entry flag, used for identification */
#define SRCH_KEY_DICT_ENTRY_FLAG                    (123)


/* How many files we can merge at once */
#define SRCH_KEY_DICT_MERGE_WIDTH                   (FOPEN_MAX - 15)


/*---------------------------------------------------------------------------*/


/*
** Structures
*/

/* Search key dict merge structure */
struct srchKeyDictMerge {
    unsigned char   pucFilePath[UTL_FILE_PATH_MAX + 1];
    unsigned char   pucDocumentKey[SPI_DOCUMENT_KEY_MAXIMUM_LENGTH + 1];
    unsigned int    uiDocumentID;
    FILE            *pfFile;
    boolean         bProcessing;
};


/*---------------------------------------------------------------------------*/


/*
** Private function prototypes
*/

static int iSrchKeyDictGenerateCallBack (unsigned char *pucKey, void *pvEntryData, va_list ap);

static int iSrchKeyDictMerge (struct srchIndex *psiSrchIndex);

static int iSrchKeyDictMergeIndexFiles (struct srchIndex *psiSrchIndex, unsigned int uiStartVersion, 
        unsigned int uiEndVersion, unsigned int uiDocumentKeysMergeLength, boolean bFinalMerge);

static int iSrchKeyDictWriteIndexBlockEntry (FILE *pfFile, unsigned int uiDocumentID,
        unsigned char *pucDocumentKey);

static int iSrchKeyDictAddDocumentKey (struct srchIndex *psiSrchIndex, struct srchKeyDictMerge *pskdmSrchKeyDictMerge, 
        unsigned int uiDocumentKeysMergeLength, unsigned int uiDocumentID, unsigned char *pucDocumentKey,
        FILE *pfOutputFile, boolean bFinalMerge);

static int iSrchKeyDictReadIndexBlockEntry (FILE *pfFile, unsigned int *puiDocumentID,
        unsigned char *pucDocumentKey);

static unsigned char *pucSrchKeyDictPrettyPrintFileNumbers (struct srchIndex *psiSrchIndex,
        unsigned int uiStartVersion, unsigned int uiEndVersion, unsigned char *pucBuffer);


static int iSrchKeyDictLookupCallBack (unsigned char *pucKey, void *pvEntryData,
        unsigned int uiEntryLength, va_list ap);



/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchKeyDictGenerate()

    Purpose:    Create the document key dictionary for this index.

                This function will loop through all the documents in this index
                and build the document key dictionary from the document keys.

    Parameters: psiSrchIndex    search index structure

    Globals:    none

    Returns:    SRCH error code

*/
int iSrchKeyDictGenerate
(
    struct srchIndex *psiSrchIndex
)
{

    int             iError = UTL_NoError;
    unsigned int    uiDocumentID = 0;
    unsigned char   pucDocumentKey[SPI_DOCUMENT_KEY_MAXIMUM_LENGTH + 1] = {'\0'};
    unsigned char   *pucDocumentKeyPtr = pucDocumentKey;
    void            *pvUtlDocumentKeyTrie = NULL;
    unsigned int    *puiData = NULL;
    
    size_t          zTrieMemorySize = 0;
    unsigned char   pucFilePath[UTL_FILE_PATH_MAX + 1] = {'\0'};
    FILE            *pfFile = NULL;

    boolean         bDocumentTermCountsSet = false;
    unsigned int    uiDocumentTermCount = 0;
    unsigned int    uiDocumentTermCountMaximum = 0;
    unsigned int    uiDocumentTermCountMinimum = UINT_MAX;

    unsigned char   pucDuplicateDocumentKeysCount[UTL_FILE_PATH_MAX + 1] = {'\0'};
    unsigned char   pucNumberString[UTL_FILE_PATH_MAX + 1] = {'\0'};


    /* Check the parameters */
    if ( psiSrchIndex == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'psiSrchIndex' parameter passed to 'iSrchKeyDictGenerate'."); 
        return (SRCH_InvalidIndex);
    }
    
    
    ASSERT(psiSrchIndex->psibSrchIndexBuild != NULL);


    iUtlLogInfo(UTL_LOG_CONTEXT, "Generating document key dictionary.");


    /* Set the number of document keys files */
    psiSrchIndex->psibSrchIndexBuild->uiDocumentKeysFileNumber = 0;


    /* Create a trie */
    if ( (iError = iUtlTrieCreate(&pvUtlDocumentKeyTrie)) != UTL_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to create a trie for the document keys, utl error: %d.", iError);
        return (SRCH_KeyDictCreateFailed);
    }


    /* Loop through all the documents in the document table */
    for ( uiDocumentID = 1; uiDocumentID <= psiSrchIndex->uiDocumentCount; uiDocumentID++ ) {

        /* Read the document information */
        iError = iSrchDocumentGetDocumentInfo(psiSrchIndex, uiDocumentID, NULL, &pucDocumentKeyPtr, 
                NULL, &uiDocumentTermCount, NULL, NULL, NULL, NULL, 0, false, false, false);

        /* Check the error */
        if ( iError != SRCH_NoError ) {
            /* Could not get document information */
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the document information for document ID: %u, index: '%s', srch error: %d.", 
                    uiDocumentID, psiSrchIndex->pucIndexName, iError);
            iError = SRCH_KeyDictCreateFailed;
            goto bailFromiSrchKeyDictGenerate;
        }
    

        /* Add the document key to the trie, this also recovers the original document ID it this document key has already been added */
        if ( (iError = iUtlTrieAdd(pvUtlDocumentKeyTrie, pucDocumentKeyPtr, (void ***)&puiData)) != UTL_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to add an entry to the document key trie, document key: '%s', utl error: %d.", 
                    pucDocumentKeyPtr, iError);
            iError = SRCH_KeyDictCreateFailed;
            goto bailFromiSrchKeyDictGenerate;
         }

 
 
         /* If the document ID for this document key is greater than 0, then this 
         ** document has already been added to the trie, so keep this document and 
         ** delete the document which was originally added to the trie
         */
         if ( (*puiData) > 0 ) {
 
             iUtlLogWarn(UTL_LOG_CONTEXT, "Duplicate document key: '%s', is assigned to document ID: %u, key was previously assigned to document ID: %u.",
                     pucDocumentKeyPtr, uiDocumentID, *puiData);
             
             psiSrchIndex->psibSrchIndexBuild->uiDuplicateDocumentKeysCount++;
            
            /* Now we delete the old document ID */
/*             if ( (iError = iSrchDocumentDeleteDocument(psiSrchIndex, *puiData)) != SRCH_NoError ) { */
/*                 iUtlLogError(UTL_LOG_CONTEXT, "Failed to delete document: %u, index: '%s', srch error: %d.", *puiData, psiSrchIndex->pucIndexName, iError); */
/*                 goto bailFromiSrchKeyDictGenerate; */
/*             } */
         }


        /* Set the pointer with the document ID */
        (*puiData) = uiDocumentID;



        /* Get the current trie size in bytes */
        iUtlTriGetMemorySize(pvUtlDocumentKeyTrie, &zTrieMemorySize);
        
        /* Convert the trie size to megabytes */
        zTrieMemorySize = (float)zTrieMemorySize / (1024 * 1024);

        /* Flush the trie if we have reached our memory limit or we are processing the last document */
        if ( (zTrieMemorySize >= psiSrchIndex->psibSrchIndexBuild->uiIndexerMemorySizeMaximum) || (uiDocumentID == psiSrchIndex->uiDocumentCount) ) {

            /* Get an intermediate document keys file path */
            if ( (iError = iSrchFilePathsGetTempKeyDictionaryFilePathFromIndex(psiSrchIndex, psiSrchIndex->psibSrchIndexBuild->uiDocumentKeysFileNumber, false, pucFilePath, UTL_FILE_PATH_MAX + 1)) != SRCH_NoError ) {
                iUtlLogError(UTL_LOG_CONTEXT, "Failed to create the keys file path, index: '%s', srch error: %d.", psiSrchIndex->pucIndexName, iError);
                iError = SRCH_KeyDictCreateFailed;
                goto bailFromiSrchKeyDictGenerate;
            } 
            if ( (pfFile = s_fopen(pucFilePath, "w")) == NULL ) {
                iUtlLogError(UTL_LOG_CONTEXT, "Could not create document key index file '%s'.", pucFilePath);
                iError = SRCH_KeyDictCreateFailed;
                goto bailFromiSrchKeyDictGenerate;
            }
            
            /* Loop over the trie entries and flush the document keys to disk */
            if ( (iError = iUtlTrieLoop(pvUtlDocumentKeyTrie, NULL, (int (*)())iSrchKeyDictGenerateCallBack, psiSrchIndex, pfFile)) != UTL_NoError ) {
                iUtlLogError(UTL_LOG_CONTEXT, "Failed to loop over the entries in the document key trie, utl error: %d.", iError);
                iError = SRCH_KeyDictCreateFailed;
                goto bailFromiSrchKeyDictGenerate;
            }
            
            /* Close the document keys file */
            s_fclose(pfFile);


            iUtlLogInfo(UTL_LOG_CONTEXT, "Finished flushing document key index file: %u.", psiSrchIndex->psibSrchIndexBuild->uiDocumentKeysFileNumber);


            /* Increment the document keys file number */
            psiSrchIndex->psibSrchIndexBuild->uiDocumentKeysFileNumber++;


            /* Free document key trie */
            iUtlTrieFree(pvUtlDocumentKeyTrie, false);
            pvUtlDocumentKeyTrie = NULL;

            /* Create a new trie */
            if ( (iError = iUtlTrieCreate(&pvUtlDocumentKeyTrie)) != UTL_NoError ) {
                iUtlLogError(UTL_LOG_CONTEXT, "Failed to create a trie for the document keys, utl error: %d.", iError);
                iError = SRCH_KeyDictCreateFailed;
                goto bailFromiSrchKeyDictGenerate;
            }
        }
        
        
        /* Set the max and min document term counts */
        uiDocumentTermCountMaximum = UTL_MACROS_MAX(uiDocumentTermCountMaximum, uiDocumentTermCount);
        uiDocumentTermCountMinimum = UTL_MACROS_MIN(uiDocumentTermCountMinimum, uiDocumentTermCount);
        
        /* Set the flag telling us that the term counts were set */
        bDocumentTermCountsSet = true;
    }

        
    /* Merge document key from disk */
    iUtlLogInfo(UTL_LOG_CONTEXT, "Merging: %u file%s.", psiSrchIndex->psibSrchIndexBuild->uiDocumentKeysFileNumber, 
            psiSrchIndex->psibSrchIndexBuild->uiDocumentKeysFileNumber == 1 ? "" : "s" );

    if ( (iError = iSrchKeyDictMerge(psiSrchIndex)) != SRCH_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Could not complete merging the document key index files, srch error: %d.", iError);
        goto bailFromiSrchKeyDictGenerate;
    }


    if ( psiSrchIndex->psibSrchIndexBuild->uiDuplicateDocumentKeysCount > 0 ) {
        s_strnncpy(pucDuplicateDocumentKeysCount, pucUtlStringsFormatUnsignedNumber(psiSrchIndex->psibSrchIndexBuild->uiDuplicateDocumentKeysCount, 
                pucNumberString, UTL_FILE_PATH_MAX + 1), UTL_FILE_PATH_MAX + 1);
        iUtlLogInfo(UTL_LOG_CONTEXT, "Duplicate document keys count: %s.", pucDuplicateDocumentKeysCount);
    }


    /* Set the max and min document term counts in the search index structure */
    if ( bDocumentTermCountsSet == true ) {
        psiSrchIndex->uiDocumentTermCountMaximum = uiDocumentTermCountMaximum;
        psiSrchIndex->uiDocumentTermCountMinimum = uiDocumentTermCountMinimum;
    }
    else {
        psiSrchIndex->uiDocumentTermCountMaximum = 0;
        psiSrchIndex->uiDocumentTermCountMinimum = 0;
    }



    /* Bail label */
    bailFromiSrchKeyDictGenerate:


    /* Free the trie */
    iUtlTrieFree(pvUtlDocumentKeyTrie, false);
    pvUtlDocumentKeyTrie = NULL;


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchKeyDictLookup()

    Purpose:    Looks up the document key in the dictionary. 

                This function will look up the document key in the document key 
                dictionary, call the callback function to unpack the buffer and 
                populate the return pointer with the unpacked information.

    Parameters: psiSrchIndex        search index structure
                pucDocumentKey      document key
                puiDocumentID       return pointer for the document ID

    Globals:    none

    Returns:    SRCH error code

*/
int iSrchKeyDictLookup
(
    struct srchIndex *psiSrchIndex,
    unsigned char *pucDocumentKey,
    unsigned int *puiDocumentID
)
{

    int     iError = UTL_NoError;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iSrchKeyDictLookup [%s]", pucDocumentKey); */


    /* Check the parameters */
    if ( psiSrchIndex == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'psiSrchIndex' parameter passed to 'iSrchKeyDictLookup'."); 
        return (SRCH_InvalidIndex);
    }

    if ( bUtlStringsIsStringNULL(pucDocumentKey) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pucDocumentKey' parameter passed to 'iSrchKeyDictLookup'."); 
        return (SRCH_KeyDictInvalidDocumentKey);
    }

    if ( puiDocumentID == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'puiDocumentID' parameter passed to 'iSrchKeyDictLookup'."); 
        return (SRCH_ReturnParameterError);
    }


    /* Look up the document key */
    iError = iUtlDictProcessEntry(psiSrchIndex->pvUtlKeyDictionary, pucDocumentKey, (int (*)())iSrchKeyDictLookupCallBack, puiDocumentID);

    /* Handle the error */
    if ( iError == UTL_DictKeyNotFound ) {
        return (SRCH_KeyDictDocumentKeyNotFound);
    }
    else if ( iError != UTL_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to look up a document key in the document key dictionary, document key: '%s', index: '%s', utl error: %d", 
                pucDocumentKey, psiSrchIndex->pucIndexName, iError);
        return (SRCH_KeyDictDocumentKeyLookupFailed);
    }


    return (SRCH_NoError);

}


/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchKeyDictGenerateCallBack()

    Purpose:    Flush the document key to disk.

    Parameters: pucKey          key
                pvEntryData     entry data        
                ap              args (optional)

    Globals:    none

    Returns:    0 if successful, -1 on error

*/
static int iSrchKeyDictGenerateCallBack
(
    unsigned char *pucKey,
    void *pvEntryData,
    va_list ap
)
{

    unsigned int        iError = SRCH_NoError;
    va_list             ap_;
    unsigned int        uiDocumentID = 0;
    struct srchIndex    *psiSrchIndex = NULL;
    FILE                *pfFile = NULL;


    ASSERT(bUtlStringsIsStringNULL(pucKey) == false);
    ASSERT(pvEntryData != NULL);


    /* Get the document ID, and hand it over */
    uiDocumentID = (unsigned long)pvEntryData;

    /* Get all our parameters, note that we make a copy of 'ap' */
    va_copy(ap_, ap);
    psiSrchIndex = (struct srchIndex *)va_arg(ap_, struct srchIndex *);
    pfFile = (FILE *)va_arg(ap_, FILE *);
    va_end(ap_);


    ASSERT(psiSrchIndex != NULL);
    ASSERT(pfFile != NULL);


    /* Write the index block document key entry */
    if ( (iError = iSrchKeyDictWriteIndexBlockEntry(pfFile, uiDocumentID, pucKey)) != SRCH_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to write the index block document key entry, srch error: %d.", iError);
        return (-1);
    }


    return (0);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchKeyDictMerge()

    Purpose:    This merges all the temporary inverted files into a large on 
                and creates a dictionary.

    Parameters: psiSrchIndex    search index structure

    Globals:    none

    Returns:    SRCH error code

*/
static int iSrchKeyDictMerge
(
    struct srchIndex *psiSrchIndex
)
{

    int             iError = SRCH_NoError;
    unsigned char   pucFilePath[UTL_FILE_PATH_MAX + 1] = {'\0'};
    unsigned int    uiDocumentKeysFileNumber = 0;
    unsigned int    uiDocumentKeysMergeLength = 0;
    unsigned int    uiPreviousDocumentKeysFileCount = 0;
    unsigned int    uiStartVersion = 0;
    unsigned int    uiEndVersion = 0;
    unsigned int    uiI = 0;
    off_t           zFileLength = 0;
    boolean         bCompletion = false;


    ASSERT(psiSrchIndex != NULL);


    /* Extract some figures from the index structure for convenience */
    uiDocumentKeysFileNumber = psiSrchIndex->psibSrchIndexBuild->uiDocumentKeysFileNumber;


    /* Start an infinite loop, we control the exit from within */
    while ( true ) {

        /* Set the completion flag */
        bCompletion = false;


        /* Count up the number of files remaining */
        for ( uiI = 0, uiDocumentKeysMergeLength = 0; uiI < uiDocumentKeysFileNumber; uiI++ ) {

            /* Create the file path */
            if ( (iError = iSrchFilePathsGetTempKeyDictionaryFilePathFromIndex(psiSrchIndex, uiI, false, pucFilePath, UTL_FILE_PATH_MAX + 1)) != SRCH_NoError ) {
                iUtlLogError(UTL_LOG_CONTEXT, "Failed to create the keys file path, index: '%s', srch error: %d.", psiSrchIndex->pucIndexName, iError);
                return (SRCH_KeyDictMergeFailed);
            } 

            /* Increment the number of index files */
            uiDocumentKeysMergeLength += (bUtlFileIsFile(pucFilePath) == true) ? 1 : 0;
        }


        /* We set the completion flag and exit the loop if the number of files
        ** is less than SRCH_KEY_DICT_MERGE_WIDTH because we can do the final merge now
        */
        if ( uiDocumentKeysMergeLength < SRCH_KEY_DICT_MERGE_WIDTH ) {
            bCompletion = true;
            break;
        }


        /* See if we reduced the number of files, this is the general idea 
        ** so if we see that this is not happening, then there is a problem
        */
        if ( (uiPreviousDocumentKeysFileCount != 0) && (uiDocumentKeysMergeLength >= uiPreviousDocumentKeysFileCount) ) {
            bCompletion = false;
            break;
        }


        /* Start looping from uiStartVersion adding up the size of the files until
        ** we reach UTL_FILE_LEN_MAX or SRCH_KEY_DICT_MERGE_WIDTH or uiDocumentKeysFileNumber at which point we merge the
        ** files and set the flag, if it was uiDocumentKeysFileNumber that we reached, we reset I
        ** to 0 to start a new iteration
        */
        for ( uiDocumentKeysMergeLength = 0, zFileLength = 0, uiStartVersion = 0; uiEndVersion < uiDocumentKeysFileNumber; uiEndVersion++ ) {

            /* Create the file path */
            if ( (iError = iSrchFilePathsGetTempKeyDictionaryFilePathFromIndex(psiSrchIndex, uiEndVersion, false, pucFilePath, UTL_FILE_PATH_MAX + 1)) != SRCH_NoError ) {
                iUtlLogError(UTL_LOG_CONTEXT, "Failed to create the keys file path, index: '%s', srch error: %d.", psiSrchIndex->pucIndexName, iError);
                return (SRCH_KeyDictMergeFailed);
            } 

            /* Process this file if it exists */
            if ( bUtlFileIsFile(pucFilePath) == true ) {

                off_t   zIntermediateFileLength = 0;

                uiStartVersion = (uiStartVersion == 0) ? uiEndVersion : uiStartVersion;

                /* Get the intermediate file length */
                if ( (iError = iUtlFileGetFilePathLength(pucFilePath, &zIntermediateFileLength)) != UTL_NoError ) {
                    iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the length of the document keys index file: '%s', utl error: %d.", pucFilePath, iError);
                    return (SRCH_KeyDictMergeFailed);
                }
                
                /* Are we at a point where we need to merge? */
                if ( ((zFileLength + zIntermediateFileLength) > UTL_FILE_LEN_MAX) || (uiDocumentKeysMergeLength > SRCH_KEY_DICT_MERGE_WIDTH) || (uiEndVersion == (uiDocumentKeysFileNumber - 1)) ) {

                    if ((zFileLength + zIntermediateFileLength) > UTL_FILE_LEN_MAX) {
                        uiEndVersion--;
/*                         iUtlLogDebug(UTL_LOG_CONTEXT, "triggered by file size [%ld][%ld] - ", zFileLength, zIntermediateFileLength); */
                    }
                    else if ( uiDocumentKeysMergeLength > SRCH_KEY_DICT_MERGE_WIDTH ) {
                        uiEndVersion--;
/*                         iUtlLogDebug(UTL_LOG_CONTEXT, "triggered by SRCH_KEY_DICT_MERGE_WIDTH - "); */
                    }
                    else if ( uiEndVersion == (uiDocumentKeysFileNumber - 1) ) {
/*                         iUtlLogDebug(UTL_LOG_CONTEXT, "triggered by end of file - "); */
                        uiDocumentKeysMergeLength++;
                    }


/*                    iUtlLogDebug(UTL_LOG_CONTEXT, "[%u, %u, %u] - ", uiStartVersion, uiEndVersion, uiDocumentKeysMergeLength); */
                    ASSERT(uiStartVersion <= uiEndVersion);

                    /* Set the completion flag */
                    bCompletion = true;

                    /* Are we merging multiple files? */
                    if ( uiDocumentKeysMergeLength > 1 ) {

                        /* Do the intermediate merge here */
                        iError = iSrchKeyDictMergeIndexFiles(psiSrchIndex, uiStartVersion, uiEndVersion, uiDocumentKeysMergeLength, false);
                        ASSERT(iError == SRCH_NoError);


                    }
                    /* Are we merging a single file? */
                    else if ( uiDocumentKeysMergeLength == 1  ) { 
/*                         iUtlLogDebug(UTL_LOG_CONTEXT, "leaving %u alone", uiStartVersion); */
                    }
                    else {
/*                         iUtlLogDebug(UTL_LOG_CONTEXT, "serious error [%u, %u]", uiStartVersion, uiEndVersion); */
                        break;
                    }

                    /* Reset the end */
                    uiEndVersion = (uiEndVersion == (uiDocumentKeysFileNumber - 1)) ? 0 : uiEndVersion + 1;

                    /* And bounce out of the for() loop since we have just finished this merge,
                    ** we need to check if we can do a final merge after each merge
                    */
                    break;
                }


                /* Increment file count and file size counters */
                zFileLength += zIntermediateFileLength;
                uiDocumentKeysMergeLength++;

            }
        }


        /* If the flag is not set, we cant complete this merge, so we break out */
        if ( bCompletion == false ) {
            break;
        }
    }


    /* If the completion flag is set, we can complete this merge */
    if ( bCompletion == true ) {
        if ( uiDocumentKeysMergeLength > 0 ) {
            iError = iSrchKeyDictMergeIndexFiles(psiSrchIndex, 0, uiDocumentKeysFileNumber - 1, uiDocumentKeysMergeLength, true);
            ASSERT(iError == SRCH_NoError);
        }
    }
    else {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to complete the document key index file merge.");
    }


    return ((bCompletion == true) ? SRCH_NoError: SRCH_KeyDictMergeFailed);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchKeyDictMergeIndexFiles()

    Purpose:    Merges a index file (lFromVersion) into another (lIntoVersion).
                This is done by merging both into version -1 and then, 
                renames it to lIntoVersion, then deletes lFromVersion.

    Parameters: psiSrchIndex        search index structure
                uiStartVersion      index file version to start with
                uiEndVersion        index file version to end with (stop short of that)
                bFinalMerge         true if this is the final merge

    Globals:    none

    Returns:    SRCH error code

*/
static int iSrchKeyDictMergeIndexFiles
(
    struct srchIndex *psiSrchIndex,
    unsigned int uiStartVersion,
    unsigned int uiEndVersion,
    unsigned int uiDocumentKeysMergeLength,
    boolean bFinalMerge
)
{

    int                         iError = SRCH_NoError;
    unsigned char               pucOutputFilePath[UTL_FILE_PATH_MAX + 1] = {'\0'};            /* version -1 */
    unsigned char               pucInputFilePath[UTL_FILE_PATH_MAX + 1] = {'\0'};
    FILE                        *pfOutputFile = NULL;
    unsigned char               pucDocumentKey[SPI_DOCUMENT_KEY_MAXIMUM_LENGTH + 1] = {'\0'};
    struct srchKeyDictMerge     *pskdmSrchKeyDictMerge = NULL;
    struct srchKeyDictMerge     *pdkmDocumentKeyMergePtr = NULL;
    unsigned int                uiI = 0;
    unsigned int                uiDocumentID = 0;
    unsigned char               pucNumberString[UTL_FILE_PATH_MAX + 1] = {'\0'};


    ASSERT(psiSrchIndex != NULL);
    ASSERT(uiStartVersion >= 0);
    ASSERT(uiEndVersion >= uiStartVersion);
    ASSERT(uiDocumentKeysMergeLength > 0);
    ASSERT((bFinalMerge == true) || (bFinalMerge == false));


    /* Allocate the merge array, this structure allows us to track the various document keys files we are merging */
    if ( (pskdmSrchKeyDictMerge = (struct srchKeyDictMerge *)s_malloc((size_t)(sizeof(struct srchKeyDictMerge) * uiDocumentKeysMergeLength))) == NULL ) {
        return (SRCH_MemError);
    }


    /* Start up initialization */
    if ( bFinalMerge == true ) {

        /* Log */
        if ( uiStartVersion == uiEndVersion ) {
            iUtlLogInfo(UTL_LOG_CONTEXT, "Merging document key index file: %u and generating document key dictionary.", uiStartVersion);
        }
        else {
            iUtlLogInfo(UTL_LOG_CONTEXT, "Merging document key index files: %s and generating document key dictionary.",
                    pucSrchKeyDictPrettyPrintFileNumbers(psiSrchIndex, uiStartVersion, uiEndVersion, pucNumberString));
        }
    }
    else {
        /* Log */
        iUtlLogInfo(UTL_LOG_CONTEXT, "Merging document key index files: %s into document key index file: %u", 
                pucSrchKeyDictPrettyPrintFileNumbers(psiSrchIndex, uiStartVersion, uiEndVersion, pucNumberString), uiStartVersion);


        /* Create the output file path, note that it is a shadow file */
        if ( (iError = iSrchFilePathsGetTempKeyDictionaryFilePathFromIndex(psiSrchIndex, uiStartVersion, true, pucOutputFilePath, UTL_FILE_PATH_MAX + 1)) != SRCH_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to create the keys file path, index: '%s', srch error: %d.", psiSrchIndex->pucIndexName, iError);
            return (SRCH_KeyDictMergeFailed);
        } 

        if ( (pfOutputFile = s_fopen(pucOutputFilePath, "w")) == NULL ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to open the output document key index file: '%s'.", pucOutputFilePath);
            return (SRCH_KeyDictMergeFailed);
        }
    }




    /* Open the input document key files */
    for ( uiI = uiStartVersion, pdkmDocumentKeyMergePtr = pskdmSrchKeyDictMerge; uiI <= uiEndVersion; uiI++ ) {

        /* Create an document key file path for this version */
        if ( (iError = iSrchFilePathsGetTempKeyDictionaryFilePathFromIndex(psiSrchIndex, uiI, false, pucInputFilePath, UTL_FILE_PATH_MAX + 1)) != SRCH_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to create the keys file path, index: '%s', srch error: %d.", psiSrchIndex->pucIndexName, iError);
            return (SRCH_KeyDictMergeFailed);
        } 

        if ( bUtlFileIsFile(pucInputFilePath) == true ) {

            s_strnncpy(pdkmDocumentKeyMergePtr->pucFilePath, pucInputFilePath, UTL_FILE_PATH_MAX + 1);

            /* Open the document key file */
            if ( (pdkmDocumentKeyMergePtr->pfFile = s_fopen(pdkmDocumentKeyMergePtr->pucFilePath, "r")) == NULL ) {
                iUtlLogError(UTL_LOG_CONTEXT, "Failed to open the input document key index file: '%s'.", pdkmDocumentKeyMergePtr->pucFilePath);
                return (SRCH_KeyDictMergeFailed);
            }

            /* Increment to the next document keys merge entry */
            pdkmDocumentKeyMergePtr++;
        }
    }



    /* Keep looping forever, we control the breakout timing from within the loop */
    while ( true ) {

        /* Loop over the merge array and read in the document keys from the document key files */
        for ( uiI = 0, pdkmDocumentKeyMergePtr = pskdmSrchKeyDictMerge, pucDocumentKey[0] = '\0'; uiI < uiDocumentKeysMergeLength; uiI++, pdkmDocumentKeyMergePtr++ ) {

            /* Loop over all the open files, closed files indicate that either there is nothing
            ** more to read from the file or that the merge array slot was allocated but not used
            */
            if ( pdkmDocumentKeyMergePtr->pfFile != NULL ) {

                /* Check the document key slot */
                if ( bUtlStringsIsStringNULL(pdkmDocumentKeyMergePtr->pucDocumentKey) == false ) {
                    /* There is still a document key here to handle, so we set the current document key */
                    if ( (bUtlStringsIsStringNULL(pucDocumentKey) == true) || (s_strcmp(pdkmDocumentKeyMergePtr->pucDocumentKey, pucDocumentKey) < 0) ) {
                        s_strnncpy(pucDocumentKey, pdkmDocumentKeyMergePtr->pucDocumentKey, SPI_DOCUMENT_KEY_MAXIMUM_LENGTH + 1);
                    }
                }
                else {
                    /* The slot is clear, we need to read the next document key in this document keys file */

                    /* Read the index block document entry from the index blocks stream */
                    iError = iSrchKeyDictReadIndexBlockEntry(pdkmDocumentKeyMergePtr->pfFile, &pdkmDocumentKeyMergePtr->uiDocumentID, 
                            pdkmDocumentKeyMergePtr->pucDocumentKey);

                    /* Handle the error */
                    if ( iError == SRCH_NoError ) {
                        /* This was a real block which was read, so we set the current document key */
                        if ( (bUtlStringsIsStringNULL(pucDocumentKey) == true) || (s_strcmp(pdkmDocumentKeyMergePtr->pucDocumentKey, pucDocumentKey) < 0) ) {
                            s_strnncpy(pucDocumentKey, pdkmDocumentKeyMergePtr->pucDocumentKey, SPI_DOCUMENT_KEY_MAXIMUM_LENGTH + 1);
                        }
                    }
                    else if ( iError == SRCH_KeyDictIndexBlockReadEOF ) {
                         /* We reached the end of the index blocks for this index file, so  we clear this entry */
                         s_fclose(pdkmDocumentKeyMergePtr->pfFile);
                         pdkmDocumentKeyMergePtr->pfFile = NULL;
                         s_memset(pdkmDocumentKeyMergePtr, 0, sizeof(struct srchKeyDictMerge));
                    }
                    else {
                        /* Another error occured which we could not handle */
                        iUtlLogError(UTL_LOG_CONTEXT, "Failed to read dictionary block in input document key index file: '%s', srch error: %d.", 
                                pdkmDocumentKeyMergePtr->pucFilePath, iError);
                        return (iError);
                    }
                }
            }
        }


         /* No more document keys to compare, we are done, break out */
        if ( bUtlStringsIsStringNULL(pucDocumentKey) == true ) {
            break;
        }


/*         iUtlLogDebug(UTL_LOG_CONTEXT, "pucDocumentKey: [%s].", pucDocumentKey); */


        /* Loop over each entry in the structure setting the processing flag */
        for ( uiI = 0, uiDocumentID = 0, pdkmDocumentKeyMergePtr = pskdmSrchKeyDictMerge; uiI < uiDocumentKeysMergeLength; uiI++, pdkmDocumentKeyMergePtr++ ) {

            /* Is this the document key we are current processing ? */
            if ( s_strcmp(pdkmDocumentKeyMergePtr->pucDocumentKey, pucDocumentKey) == 0 ) {

                /* Set the document key */
                uiDocumentID = pdkmDocumentKeyMergePtr->uiDocumentID;

                /* Set the flag telling that we are currently processing this entry */
                pdkmDocumentKeyMergePtr->bProcessing = true;

            }
            else {
                /* Set the flag telling that we are NOT currently processing this entry */
                pdkmDocumentKeyMergePtr->bProcessing = false;
            }
        }


        /* And store the document key in the index */
        if ( (iError = iSrchKeyDictAddDocumentKey(psiSrchIndex, pskdmSrchKeyDictMerge, uiDocumentKeysMergeLength, uiDocumentID, 
                pucDocumentKey, pfOutputFile, bFinalMerge)) != SRCH_NoError ) {
            if ( bFinalMerge == true ) {
                iUtlLogError(UTL_LOG_CONTEXT, "Failed to store document key '%s' in the document key dictionary, srch error: %d.", 
                        pucDocumentKey, iError);
            }
            else {
                iUtlLogError(UTL_LOG_CONTEXT, "Failed to store document key '%s' in the output document key index file: '%s', srch error: %d.", 
                        pucDocumentKey, pucOutputFilePath, iError);
            }
            return (iError);
        }


        /* Loop over each entry in the structure erasing the document key we just processed */
        for ( uiI = 0, pdkmDocumentKeyMergePtr = pskdmSrchKeyDictMerge; uiI < uiDocumentKeysMergeLength; uiI++, pdkmDocumentKeyMergePtr++ ) {
            if ( pdkmDocumentKeyMergePtr->bProcessing == true ) {
                pdkmDocumentKeyMergePtr->pucDocumentKey[0] = '\0';
            }
        }


    } /* while ( true ) */



    /* Delete the input files */
    for ( uiI = uiStartVersion; uiI <= uiEndVersion; uiI++ ) {

        if ( (iError = iSrchFilePathsGetTempKeyDictionaryFilePathFromIndex(psiSrchIndex, uiI, false, pucInputFilePath, UTL_FILE_PATH_MAX + 1)) != SRCH_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to create the keys file path, index: '%s', srch error: %d.", psiSrchIndex->pucIndexName, iError);
            return (SRCH_KeyDictMergeFailed);
        } 

        if ( bUtlFileIsFile(pucInputFilePath) == true ) {
            s_remove(pucInputFilePath);
        }
    }


    /* Release the merge array */
    s_free(pskdmSrchKeyDictMerge);



    /* Rename the output file to the new version */
    if ( bFinalMerge == false ) {

        /* Close the output index file */
        s_fclose(pfOutputFile);

        if ( (iError = iSrchFilePathsGetTempKeyDictionaryFilePathFromIndex(psiSrchIndex, uiStartVersion, false, pucInputFilePath, UTL_FILE_PATH_MAX + 1)) != SRCH_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to create the keys file path, index: '%s', srch error: %d.", psiSrchIndex->pucIndexName, iError);
            return (SRCH_KeyDictMergeFailed);
        }
    
        if ( s_rename(pucOutputFilePath, pucInputFilePath) != 0 ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to rename file from: '%s' to: '%s'.", pucOutputFilePath, pucInputFilePath);
            return (SRCH_KeyDictMergeFailed);
        }
    }


    return (SRCH_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchKeyDictAddDocumentKey()

    Purpose:    Adds a document key to the index, the path the function will take
                depends on whether this is the final build, and whether this is 
                an update as opposed to a creation.

    Parameters: psiSrchIndex                index structure
                pskdmSrchKeyDictMerge       search key dict merge structure
                uiDocumentKeysMergeLength   number of entries in the document key merge structure
                uiDocumentID                document ID
                pucDocumentKey              current document key being merged
                pfOutputFile                output file descriptor (NULL if bFinalMerge is true)
                bFinalMerge                 set to true if this is the final merge

    Globals:    none

    Returns:    SRCH error code

*/
static int iSrchKeyDictAddDocumentKey
(
    struct srchIndex *psiSrchIndex,
    struct srchKeyDictMerge *pskdmSrchKeyDictMerge,
    unsigned int uiDocumentKeysMergeLength,
    unsigned int uiDocumentID,
    unsigned char *pucDocumentKey,
    FILE *pfOutputFile,
    boolean bFinalMerge
)
{

    int             iError = UTL_NoError;

    unsigned char   pucBuffer[10] = {'\0'};    /* Should be enough space */
    unsigned char   *pucBufferEndPtr = NULL;
    unsigned int    uiBufferLength = 0;


    ASSERT(psiSrchIndex != NULL);
    ASSERT(psiSrchIndex->psibSrchIndexBuild != NULL);
/*     ASSERT(psiSrchIndex->pvUtlKeyDictionary != NULL); */
    ASSERT(pskdmSrchKeyDictMerge != NULL);
    ASSERT(uiDocumentKeysMergeLength > 0);
    ASSERT(bUtlStringsIsStringNULL(pucDocumentKey) == false);
    ASSERT(((bFinalMerge == true) && (pfOutputFile == NULL)) || ((bFinalMerge != true) && (pfOutputFile != NULL)));


    /* Delete old document */


    /* so the right thing */
    if ( bFinalMerge == true ) {

        /* Set the start of the buffer pointer */
        pucBufferEndPtr = pucBuffer;


        /* Write out the document ID (increments the pointer) */
        UTL_NUM_WRITE_COMPRESSED_UINT(uiDocumentID, pucBufferEndPtr);


        /* How long is the buffer? */
        uiBufferLength = pucBufferEndPtr - pucBuffer;

        /* Add the document key and the buffer to the document key dictionary */
        if ( (iError = iUtlDictAddEntry(psiSrchIndex->pvUtlKeyDictionary, pucDocumentKey, pucBuffer, uiBufferLength)) != SRCH_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to add a document key to the document key dictionary, document key: '%s', index: '%s', utl error: %d.", 
                    pucDocumentKey, psiSrchIndex->pucIndexName, iError);
            return (SRCH_KeyDictMergeFailed);
        }

    }
    else {

        /* write the output index block dict entry */
        if ( (iError = iSrchKeyDictWriteIndexBlockEntry(pfOutputFile, uiDocumentID, pucDocumentKey)) != SRCH_NoError ) {
            return (iError);
        }
    }


    return (SRCH_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchKeyDictWriteIndexBlockEntry()

    Purpose:    Writes the index block document key entry

    Parameters: pfFile              index block file descriptor
                uiDocumentID        document ID
                pucDocumentKey      document key

    Globals:    none

    Returns:    SRCH error code

*/
static int iSrchKeyDictWriteIndexBlockEntry
(
    FILE *pfFile,
    unsigned int uiDocumentID,
    unsigned char *pucDocumentKey
)
{

    unsigned int    uiDocumentKeyLength = 0;
    unsigned char   pucBuffer[SRCH_KEY_DICT_ENTRY_HEADER_LENGTH];
    unsigned char   *pucBufferPtr = pucBuffer;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iSrchKeyDictWriteIndexBlockEntry - uiDocumentID: %u, pucDocumentKey: %s", uiDocumentID, pucDocumentKey); */


    ASSERT(pfFile != NULL);
    ASSERT(bUtlStringsIsStringNULL(pucDocumentKey) == false);


    /* Get the key length */
    uiDocumentKeyLength = s_strlen(pucDocumentKey);

    /* Set the start of the buffer pointer */
    pucBufferPtr = pucBuffer;

    /* Write out the start of the document key entry */
    UTL_NUM_WRITE_UINT(SRCH_KEY_DICT_ENTRY_FLAG, SRCH_KEY_DICT_ENTRY_FLAG_SIZE, pucBufferPtr);
    UTL_NUM_WRITE_UINT(uiDocumentID, SRCH_KEY_DICT_ENTRY_DOCUMENT_ID_SIZE, pucBufferPtr);
    UTL_NUM_WRITE_UINT(uiDocumentKeyLength, SRCH_KEY_DICT_ENTRY_KEY_SIZE, pucBufferPtr);

    ASSERT((pucBufferPtr - pucBuffer) == SRCH_KEY_DICT_ENTRY_HEADER_LENGTH);

    /* Write out the buffer */
    if ( s_fwrite(pucBuffer, SRCH_KEY_DICT_ENTRY_HEADER_LENGTH, 1, pfFile) != 1 ) {
        return (SRCH_KeyDictIndexBlockWriteFailed);
    }

    /* Write out the document key, exclude the terminating NULL */
    if ( s_fwrite(pucDocumentKey, uiDocumentKeyLength, 1, pfFile) != 1 ) {
        return (SRCH_KeyDictIndexBlockWriteFailed);
    }


    return (SRCH_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchKeyDictReadIndexBlockEntry()

    Purpose:    This read the index block document key entry from the stream.
                It assumes the stream is positioned at the start of an index
                block document key entry.

    Parameters: pfFile              file descriptor
                puiDocumentID       document ID
                pucDocumentKey      document key

    Globals:    none

    Returns:    SRCH error code

                SRCH_KeyDictIndexBlockReadEOF if it is at the end of a file,
                SRCH_KeyDictIndexBlockReadFailed on error

*/
static int iSrchKeyDictReadIndexBlockEntry
(
    FILE *pfFile,
    unsigned int *puiDocumentID,
    unsigned char *pucDocumentKey
)
{

    unsigned int    uiDocumentKeyLength = 0;
    unsigned int    uiIndexBlockFlag = 0;
    unsigned char   pucBuffer[SRCH_KEY_DICT_ENTRY_HEADER_LENGTH];
    unsigned char   *pucBufferPtr = NULL;


    ASSERT(pfFile != NULL);
    ASSERT(puiDocumentID != NULL);
    ASSERT(pucDocumentKey != NULL);


    /* Clear the return parameters */
    *puiDocumentID = 0;    
    pucDocumentKey[0] = '\0';


    /* Read the buffer */
    if ( s_fread(pucBuffer, SRCH_KEY_DICT_ENTRY_HEADER_LENGTH, 1, pfFile) != 1 ) {
        /* Not an error, it is how we tell that we are done */
        if ( s_feof(pfFile) != 0 ) {
            return (SRCH_KeyDictIndexBlockReadEOF);
        }
        else {
            return (SRCH_KeyDictIndexBlockReadFailed);
        }
    }

    /* Set the start of the buffer pointer */
    pucBufferPtr = pucBuffer;

    /* Read the index block flag */
    UTL_NUM_READ_UINT(uiIndexBlockFlag, SRCH_KEY_DICT_ENTRY_FLAG_SIZE, pucBufferPtr);

    /* Check the flag */
    if ( uiIndexBlockFlag != SRCH_KEY_DICT_ENTRY_FLAG ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Did not find a dictionary entry, flag is: %u at file position: %ld.", uiIndexBlockFlag, (s_ftell(pfFile) - SRCH_KEY_DICT_ENTRY_FLAG_SIZE));
        return (SRCH_KeyDictIndexBlockReadFailed);
    }

    /* Read the rest of the header */
    UTL_NUM_READ_UINT(*puiDocumentID, SRCH_KEY_DICT_ENTRY_DOCUMENT_ID_SIZE, pucBufferPtr);
    UTL_NUM_READ_UINT(uiDocumentKeyLength, SRCH_KEY_DICT_ENTRY_KEY_SIZE, pucBufferPtr);

    ASSERT((pucBufferPtr - pucBuffer) == SRCH_KEY_DICT_ENTRY_HEADER_LENGTH);


    /* Read the document key and NULL terminate it */
    if ( s_fread(pucDocumentKey, uiDocumentKeyLength, 1, pfFile) != 1 ) {
        return (SRCH_KeyDictIndexBlockReadFailed);
    }
    pucDocumentKey[uiDocumentKeyLength] = '\0';


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iSrchKeyDictReadIndexBlockEntry - *puiDocumentID: %u, pucDocumentKey: %s", *puiDocumentID, pucDocumentKey); */


    return (SRCH_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   pucSrchKeyDictPrettyPrintFileNumbers()

    Purpose:    This function formats the merge index file number for printing

    Parameters: psiSrchIndex        search index structure
                uiStartVersion      start level
                uiEndVersion        end level
                pucBuffer           return buffer

    Globals:    none

    Returns:    a pointer to the return buffer

*/
static unsigned char *pucSrchKeyDictPrettyPrintFileNumbers
(
    struct srchIndex *psiSrchIndex,
    unsigned int uiStartVersion,
    unsigned int uiEndVersion,
    unsigned char *pucBuffer
)
{

    int             iError = SRCH_NoError;
    unsigned int    uiI = 0, uiJ = 0;
    boolean         bExtent = false, bComma = false;
    unsigned char   pucFilePath[UTL_FILE_PATH_MAX + 1] = {'\0'};
    unsigned char   pucLocalBuffer[UTL_FILE_PATH_MAX + 1] = {'\0'};


    ASSERT(psiSrchIndex != NULL);
    ASSERT(uiStartVersion >= 0);
    ASSERT(uiEndVersion >= uiStartVersion);
    ASSERT(pucBuffer != NULL);


    /* Create the merge info buffer */
    for ( uiI = uiStartVersion, uiJ = uiStartVersion, pucBuffer[0] = '\0'; uiI <= uiEndVersion; uiI++ ) {

        /* Create the file path */
        if ( (iError = iSrchFilePathsGetTempKeyDictionaryFilePathFromIndex(psiSrchIndex, uiI, false, pucFilePath, UTL_FILE_PATH_MAX + 1)) != SRCH_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to create the keys file path, index: '%s', srch error: %d.", psiSrchIndex->pucIndexName, iError);
            return (NULL);
        }

        if ( bUtlFileIsFile(pucFilePath) == true) {
            if ( (uiI - uiJ) > 1 ) {
                if ( bExtent == true ) {
                    snprintf(pucLocalBuffer, UTL_FILE_PATH_MAX + 1, " - %u", uiJ);
                    s_strnncat(pucBuffer, pucLocalBuffer, UTL_FILE_PATH_MAX, UTL_FILE_PATH_MAX + 1);
                    bExtent = false;
                }
                else {
                    snprintf(pucLocalBuffer, UTL_FILE_PATH_MAX + 1, "%s%u", (bComma == true) ? ", " : "", uiJ);
                    s_strnncat(pucBuffer, pucLocalBuffer, UTL_FILE_PATH_MAX, UTL_FILE_PATH_MAX + 1);
                    bComma = true;
                }
            }
            else if ( (uiI - uiJ) == 1 ) {
                if ( bExtent == false ) {
                    snprintf(pucLocalBuffer, UTL_FILE_PATH_MAX + 1, "%s%u", (bComma == true) ? ", " : "", uiJ);
                    s_strnncat(pucBuffer, pucLocalBuffer, UTL_FILE_PATH_MAX, UTL_FILE_PATH_MAX + 1);
                    bComma = true;
                }
                bExtent = true;
            }
            uiJ = uiI;
        }
    }

    /* Make sure we add the last number */
    if ( uiI != uiJ ) {
        if ( bExtent == true ) {
            snprintf(pucLocalBuffer, UTL_FILE_PATH_MAX + 1, " - %u", uiJ);
            s_strnncat(pucBuffer, pucLocalBuffer, UTL_FILE_PATH_MAX, UTL_FILE_PATH_MAX + 1);
        }
        else {
            snprintf(pucLocalBuffer, UTL_FILE_PATH_MAX + 1, "%s%u", (bComma == true) ? ", " : "", uiJ);
            s_strnncat(pucBuffer, pucLocalBuffer, UTL_FILE_PATH_MAX, UTL_FILE_PATH_MAX + 1);
        }
    }


    return (pucBuffer);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchKeyDictLookupCallBack()

    Purpose:    This function is passed to the dictionary lookup function to
                perform extra processing on a document key.

                If there is a hit, the data will be unpacked and the 
                return pointers will be populated.

    Parameters: pucKey          key
                pvEntryData     entry data
                uiEntryLength   entry length
                ap              args (optional)

    Globals:    none

    Returns:    0 to continue processing, non-0 otherwise

*/
static int iSrchKeyDictLookupCallBack
(
    unsigned char *pucKey,
    void *pvEntryData,
    unsigned int uiEntryLength,
    va_list ap
)
{

    va_list         ap_;
    unsigned char   *pucEntryDataPtr = NULL;
    unsigned int    uiDocumentID = 0;
    unsigned int    *puiDocumentID = NULL;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iSrchKeyDictLookupCallBack [%s][%u][%s]", pucKey, uiEntryLength, */
/*             (pvEntryData != NULL) ? "(pvEntryData != NULL)" : "(pvEntryData == NULL)" ); */


    ASSERT(bUtlStringsIsStringNULL(pucKey) == false);
    ASSERT(pvEntryData != NULL);
    ASSERT(uiEntryLength > 0);


    /* Get all our parameters, note that we make a copy of 'ap' */
    va_copy(ap_, ap);
    puiDocumentID = (unsigned int *)va_arg(ap_, unsigned int *);
    va_end(ap_);


    ASSERT(puiDocumentID != NULL);


    /* Decode the buffer to get the document ID, decode to a variable and copy to the pointer for optimization */
    pucEntryDataPtr = (unsigned char *)pvEntryData;
    UTL_NUM_READ_COMPRESSED_UINT(uiDocumentID, pucEntryDataPtr);
    *puiDocumentID = uiDocumentID;


    return (0);

}


/*---------------------------------------------------------------------------*/

