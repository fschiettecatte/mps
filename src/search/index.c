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

    Module:     index.c

    Author:     Francois Schiettecatte (FS Consulting LLC.)

    Created:    15 September 1995

    Purpose:    This module manages the index. It provides various
                functions for opening and closing index, both for searching
                and creating. The functions to interact with are:

                    iSrchIndexOpen()
                    iSrchIndexClose()
                    iSrchIndexAbort()


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
#define UTL_LOG_CONTEXT                     (unsigned char *)"com.fsconsult.mps.src.search.index"


/*---------------------------------------------------------------------------*/

/*
** Feature defines
*/

/*---------------------------------------------------------------------------*/

/*
** Defines
*/

/* Shared lock timeout in microseconds */
#define SRCH_INDEX_SHARED_LOCK_SLEEP                (100)
#define SRCH_INDEX_SHARED_LOCK_TIMEOUT              (500)

/* Exclusive lock timeout in seconds */
#define SRCH_INDEX_EXCLUSIVE_LOCK_SLEEP             (1)
#define SRCH_INDEX_EXCLUSIVE_LOCK_TIMEOUT           (600)


/*---------------------------------------------------------------------------*/


/*
** Private function prototypes
*/

static int iSrchIndexFilesOpen (struct srchIndex *psiSrchIndex);

static int iSrchIndexFilesClose (struct srchIndex *psiSrchIndex);


static int iSrchIndexLock (struct srchIndex *psiSrchIndex);

static int iSrchIndexUnlock (struct srchIndex *psiSrchIndex);


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchIndexOpen()

    Purpose:    Open an index (open all its files), and return an opaque object.

    Parameters: pucIndexDirectoryPath           index directory path
                pucConfigurationDirectoryPath   configuration directory path
                pucIndexName                    index name
                uiIntent                        intent
                ppsiSrchIndex                   return pointer for the index

    Globals:    none

    Returns:    SRCH error code

*/
int iSrchIndexOpen
(
    unsigned char *pucIndexDirectoryPath,
    unsigned char *pucConfigurationDirectoryPath,
    unsigned char *pucIndexName,
    unsigned int uiIntent,
    struct srchIndex **ppsiSrchIndex
)
{

    int                 iError = SRCH_NoError;
    unsigned char       pucIndexPath[UTL_FILE_PATH_MAX + 1] = {'\0'};
    struct srchIndex    *psiSrchIndex = NULL;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iSrchIndexOpen - pucIndexDirectoryPath: '%s', pucConfigurationDirectoryPath: '%s', pucIndexName: '%s', uiIntent: %u",  */
/*             pucIndexDirectoryPath, pucConfigurationDirectoryPath, pucIndexName, uiIntent); */


    /* Check the parameters */
    if ( bUtlStringsIsStringNULL(pucIndexDirectoryPath) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pucIndexDirectoryPath' parameter passed to 'iSrchIndexOpen'."); 
        return (SRCH_IndexInvalidIndexDirectoryPath);
    }

    if ( bUtlStringsIsStringNULL(pucConfigurationDirectoryPath) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pucConfigurationDirectoryPath' parameter passed to 'iSrchIndexOpen'."); 
        return (SRCH_IndexInvalidConfigurationDirectoryPath);
    }

    if ( bUtlStringsIsStringNULL(pucIndexName) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pucIndexName' parameter passed to 'iSrchIndexOpen'."); 
        return (SRCH_IndexInvalidIndexName);
    }

    if ( SRCH_INDEX_INTENT_VALID(uiIntent) == false ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiIntent' parameter passed to 'iSrchIndexOpen'."); 
        return (SRCH_IndexInvalidIntent);
    }

    if ( ppsiSrchIndex == NULL) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppsiSrchIndex' parameter passed to 'iSrchIndexOpen'."); 
        return (SRCH_ReturnParameterError);
    }


    /* Check that the index name is indeed a name */
    if ( bUtlFileIsName(pucIndexName) == false ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid index name, index: '%s'.", pucIndexName); 
        iError = SRCH_IndexInvalidIndexName;
        goto bailFromiSrchIndexOpen;
    }


    /* Create the index path */
    if ( (iError = iUtlFileMergePaths(pucIndexDirectoryPath, pucIndexName, pucIndexPath, UTL_FILE_PATH_MAX + 1)) != UTL_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to create the index path, index: '%s', index directory path: '%s', utl error: %d.", 
                pucIndexName, pucIndexDirectoryPath, iError); 
        iError = SRCH_IndexInvalidIndexPath;
        goto bailFromiSrchIndexOpen;
    }


    /* Allocate the index structure */
    if ( (psiSrchIndex = (struct srchIndex *)s_malloc((size_t)sizeof(struct srchIndex))) == NULL ) {
        iError = SRCH_MemError;
        goto bailFromiSrchIndexOpen;
    }

    /* Clear the index structure */
    psiSrchIndex->pucIndexName = NULL;
    psiSrchIndex->pucIndexPath = NULL;
    psiSrchIndex->uiMajorVersion = 0;
    psiSrchIndex->uiMinorVersion = 0;
    psiSrchIndex->uiPatchVersion = 0;
    psiSrchIndex->pfLockFile = NULL;
    psiSrchIndex->uiIntent = 0;
    psiSrchIndex->uiLanguageID = 0;
    psiSrchIndex->uiTokenizerID = 0;
    psiSrchIndex->uiStemmerID = 0;
    psiSrchIndex->pvUtlDocumentTable = NULL;
    psiSrchIndex->pvUtlDocumentData = NULL;
    psiSrchIndex->pvUtlIndexData = NULL;
    psiSrchIndex->pvUtlKeyDictionary = NULL;
    psiSrchIndex->pvUtlTermDictionary = NULL;
    psiSrchIndex->pvUtlIndexInformation = NULL;
    psiSrchIndex->uiTermLengthMaximum = 0;
    psiSrchIndex->uiTermLengthMinimum = 0;
    psiSrchIndex->ulUniqueTermCount = 0;
    psiSrchIndex->ulTotalTermCount = 0;
    psiSrchIndex->ulUniqueStopTermCount = 0;
    psiSrchIndex->ulTotalStopTermCount = 0;
    psiSrchIndex->uiDocumentCount = 0;
    psiSrchIndex->uiDocumentTermCountMaximum = 0;
    psiSrchIndex->uiDocumentTermCountMinimum = 0;
    psiSrchIndex->uiFieldIDMaximum = 0;
    psiSrchIndex->tLastUpdateTime = 0;
    psiSrchIndex->psibSrchIndexBuild = NULL;


    /* Set the index path */
    if ( (psiSrchIndex->pucIndexPath = (unsigned char *)s_strdup(pucIndexPath)) == NULL ) {
        iError = SRCH_MemError;
        goto bailFromiSrchIndexOpen;
    }
    
    /* Set the index name - note that this is just a pointer in the index path */
    if ( (iError = iUtlFileGetPathBase(psiSrchIndex->pucIndexPath, &psiSrchIndex->pucIndexName)) != UTL_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the index name from the index path: '%s', utl error: %d.", psiSrchIndex->pucIndexPath, iError);
        iError = SRCH_IndexInvalidIndexPath;
        goto bailFromiSrchIndexOpen;
    }
    
    /* Set the current intent */
    psiSrchIndex->uiIntent = uiIntent;

    
    /* Create the index directory if it does not exist and we are creating */
    if ( uiIntent == SRCH_INDEX_INTENT_CREATE ) {

        /* Check that the directory exists, if not we create it */
        if ( bUtlFileIsDirectory(psiSrchIndex->pucIndexPath) == false ) {
            if ( s_mkdir(psiSrchIndex->pucIndexPath, S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH) != 0 ) {
                iUtlLogError(UTL_LOG_CONTEXT, "Failed to create index directory, index directory path: '%s', index: '%s'.", 
                        pucIndexDirectoryPath, psiSrchIndex->pucIndexName);
                iError = SRCH_IndexInvalidIndexPath;
                goto bailFromiSrchIndexOpen;
            }
        }
        else {
            /* The index directory already exists, but we check that we can access it */
            if ( !((bUtlFilePathRead(psiSrchIndex->pucIndexPath) == true) && (bUtlFilePathWrite(psiSrchIndex->pucIndexPath) == true) && 
                    (bUtlFilePathExec(psiSrchIndex->pucIndexPath) == true)) ) {
                iUtlLogError(UTL_LOG_CONTEXT, "Insufficient permissions to create directory, index directory path: '%s', index: '%s'.", 
                        pucIndexDirectoryPath, psiSrchIndex->pucIndexName);
                iError = SRCH_IndexInvalidIndexPath;
                goto bailFromiSrchIndexOpen;
            }
        }
    }
    
    /* Check that we can access the index directory */
    else if ( uiIntent == SRCH_INDEX_INTENT_SEARCH ) {
    
        unsigned char   pucFilePath[UTL_FILE_PATH_MAX + 1] = {'\0'};

        /* Check that this is a directory */
        if ( bUtlFileIsDirectory(psiSrchIndex->pucIndexPath) == false ) {
            iError = SRCH_IndexInvalidIndexPath;
            goto bailFromiSrchIndexOpen;
        }

        /* We check that we can access the index directory */
        if ( bUtlFilePathRead(psiSrchIndex->pucIndexPath) == false ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Insufficient permissions to access directory, index directory path: '%s', index: '%s'.", 
                    pucIndexDirectoryPath, psiSrchIndex->pucIndexName);
            iError = SRCH_IndexInvalidIndexPath;
            goto bailFromiSrchIndexOpen;
        }
        
        if ( bUtlFilePathExec(psiSrchIndex->pucIndexPath) == false ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Insufficient permissions to access directory, index directory path: '%s', index: '%s'.", 
                    pucIndexDirectoryPath, psiSrchIndex->pucIndexName);
            iError = SRCH_IndexInvalidIndexPath;
            goto bailFromiSrchIndexOpen;
        }

        /* Create the lock file path */
        iSrchFilePathsGetIndexLockFilePathFromIndexPath(psiSrchIndex->pucIndexPath, pucFilePath, UTL_FILE_PATH_MAX + 1);
        
        if ( bUtlFileIsFile(pucFilePath) != true ) {
            iError = SRCH_IndexInvalidIndexPath;
            goto bailFromiSrchIndexOpen;
        }
    }



    /* Allocate the index building structure if we are creating */
    if ( uiIntent == SRCH_INDEX_INTENT_CREATE ) {

        /* Allocate the index build structure */
        if ( (psiSrchIndex->psibSrchIndexBuild = (struct srchIndexBuild *)s_malloc((size_t)sizeof(struct srchIndexBuild))) == NULL ) {
            iError = SRCH_MemError;
            goto bailFromiSrchIndexOpen;
        }

        /* Set the index build structure */
        psiSrchIndex->psibSrchIndexBuild->pucTemporaryDirectoryPath = NULL;
        psiSrchIndex->psibSrchIndexBuild->pucStopListFilePath = NULL;
        psiSrchIndex->psibSrchIndexBuild->uiStopListTypeID = 0;
        psiSrchIndex->psibSrchIndexBuild->uiStopListID = 0;
        psiSrchIndex->psibSrchIndexBuild->pvLngStopList = NULL;
        psiSrchIndex->psibSrchIndexBuild->pvLngStemmer = NULL;
        psiSrchIndex->psibSrchIndexBuild->pucDocumentDataEntry = NULL;
        psiSrchIndex->psibSrchIndexBuild->uiDocumentDataEntryLength = 0;
        psiSrchIndex->psibSrchIndexBuild->pvUtlTermTrie = NULL;
        psiSrchIndex->psibSrchIndexBuild->zMemorySize = 0;
        psiSrchIndex->psibSrchIndexBuild->uiLastDocumentID = 0;
        psiSrchIndex->psibSrchIndexBuild->uiIndexerMemorySizeMaximum = 0;
        psiSrchIndex->psibSrchIndexBuild->uiIndexFileNumber = 0;
        psiSrchIndex->psibSrchIndexBuild->uiDocumentKeysFileNumber = 0;
        psiSrchIndex->psibSrchIndexBuild->uiUniqueTermCount = 0;
        psiSrchIndex->psibSrchIndexBuild->uiTotalTermCount = 0;
        psiSrchIndex->psibSrchIndexBuild->uiUniqueStopTermCount = 0;
        psiSrchIndex->psibSrchIndexBuild->uiTotalStopTermCount = 0;
        psiSrchIndex->psibSrchIndexBuild->uiDocumentCount = 0;
        psiSrchIndex->psibSrchIndexBuild->pucIndexBlock = NULL;
        psiSrchIndex->psibSrchIndexBuild->uiIndexBlockLength = 0;
        psiSrchIndex->psibSrchIndexBuild->pucFieldIDBitmap = NULL;
        psiSrchIndex->psibSrchIndexBuild->uiFieldIDBitmapLength = 0;
        psiSrchIndex->psibSrchIndexBuild->uiDuplicateDocumentKeysCount = 0;
        psiSrchIndex->psibSrchIndexBuild->pvLngConverterUTF8ToWChar = NULL;
        psiSrchIndex->psibSrchIndexBuild->pvLngConverterWCharToUTF8 = NULL;


        /* Create the character sets converters */
        if ( (iError = iLngConverterCreateByName(LNG_CHARACTER_SET_UTF_8_NAME, LNG_CHARACTER_SET_WCHAR_NAME, &psiSrchIndex->psibSrchIndexBuild->pvLngConverterUTF8ToWChar)) != LNG_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to create a character set converter to convert from utf-8 to wide characters, lng error: %d.", iError);
            iError = SRCH_IndexOpenFailed;
            goto bailFromiSrchIndexOpen;
        }    
        
        if ( (iError = iLngConverterCreateByName(LNG_CHARACTER_SET_WCHAR_NAME, LNG_CHARACTER_SET_UTF_8_NAME, &psiSrchIndex->psibSrchIndexBuild->pvLngConverterWCharToUTF8)) != LNG_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to create a character set converter to convert from wide characters to utf-8, lng error: %d.", iError);
            iError = SRCH_IndexOpenFailed;
            goto bailFromiSrchIndexOpen;
        }    
    }



    /* Set the index lock */
    if ( (iError = iSrchIndexLock(psiSrchIndex)) != SRCH_NoError ) {
        goto bailFromiSrchIndexOpen;
    }


    /* Open the index file */
    if ( (iError = iSrchIndexFilesOpen(psiSrchIndex)) != SRCH_NoError ) {
        goto bailFromiSrchIndexOpen;
    }

    
    
    /* Bail label */
    bailFromiSrchIndexOpen:
    
    
    /* Handle the error */
    if ( iError == SRCH_NoError ) {

        /* Set the return pointer */
        *ppsiSrchIndex = psiSrchIndex;
    }
    else {
    
        /* Abort the index if we got anywhere in opening it */
        if ( psiSrchIndex != NULL ) {
            iSrchIndexAbort(psiSrchIndex, pucConfigurationDirectoryPath);
            psiSrchIndex = NULL;
        }
    }


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iSrchIndexOpen - pucIndexName: '%s', iError: %d.", pucIndexName, iError); */


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchIndexClose()

    Purpose:    Close the index.

    Parameters: psiSrchIndex    search index structure

    Globals:    none

    Returns:    SRCH error code

*/
int iSrchIndexClose
(
    struct srchIndex *psiSrchIndex
)
{

    int     iError = UTL_NoError;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iSrchIndexClose - psiSrchIndex->pucIndexName: '%s'.", psiSrchIndex->pucIndexName); */


    /* Check the parameters */
    if ( psiSrchIndex == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'psiSrchIndex' parameter passed to 'iSrchIndexClose'."); 
        return (SRCH_InvalidIndex);
    }


    /* Close the index files */
    if ( (iError = iSrchIndexFilesClose(psiSrchIndex)) != SRCH_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to close index files, index: '%s', srch error: %d.", psiSrchIndex->pucIndexName, iError);
    }

    /* Make sure that the index is unlocked */
    if ( (iError = iSrchIndexUnlock(psiSrchIndex)) != SRCH_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to unlock the index: '%s', srch error: %d.", psiSrchIndex->pucIndexName, iError);
    }


    /* Finally we release the index memory */
    if ( psiSrchIndex->psibSrchIndexBuild != NULL ) {

        /* Free the stemmer */
        iLngStemmerFree(psiSrchIndex->psibSrchIndexBuild->pvLngStemmer);
        psiSrchIndex->psibSrchIndexBuild->pvLngStemmer = NULL;


        /* Free the trie, this should only be allocated if something
        ** stopped the indexing process before it was finished. 
        ** Note that we free the datum since it was allocated
        ** with malloc(), but we leak the postings blocks.
        */
        if ( psiSrchIndex->psibSrchIndexBuild->pvUtlTermTrie != NULL ) {
            iUtlTrieFree(psiSrchIndex->psibSrchIndexBuild->pvUtlTermTrie, true);
            psiSrchIndex->psibSrchIndexBuild->pvUtlTermTrie = NULL;
        }
        

        /* Free the stop list and the stop list file path */
        iLngStopListFree(psiSrchIndex->psibSrchIndexBuild->pvLngStopList);
        psiSrchIndex->psibSrchIndexBuild->pvLngStopList = NULL;
        s_free(psiSrchIndex->psibSrchIndexBuild->pucStopListFilePath);

        /* Free the various pointers */
        s_free(psiSrchIndex->psibSrchIndexBuild->pucDocumentDataEntry);
        s_free(psiSrchIndex->psibSrchIndexBuild->pucIndexBlock);
        s_free(psiSrchIndex->psibSrchIndexBuild->pucFieldIDBitmap);
        s_free(psiSrchIndex->psibSrchIndexBuild->pucTemporaryDirectoryPath);

        /* Close the character set converters */
        iLngConverterFree(psiSrchIndex->psibSrchIndexBuild->pvLngConverterUTF8ToWChar);
        psiSrchIndex->psibSrchIndexBuild->pvLngConverterUTF8ToWChar = NULL;

        iLngConverterFree(psiSrchIndex->psibSrchIndexBuild->pvLngConverterWCharToUTF8);
        psiSrchIndex->psibSrchIndexBuild->pvLngConverterWCharToUTF8 = NULL;

        s_free(psiSrchIndex->psibSrchIndexBuild);
    }


    /* Free the index path - note that we don't free the index name 
    ** as it is just a pointer inside the index path
    */
    s_free(psiSrchIndex->pucIndexPath);

    /* Free the index */
    s_free(psiSrchIndex);


    return (SRCH_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchIndexAbort()

    Purpose:    Abort indexing.

    Parameters: psiSrchIndex                    search index structure
                pucConfigurationDirectoryPath   configuration directory path

    Globals:    none

    Returns:    SRCH error code

*/
int iSrchIndexAbort
(
    struct srchIndex *psiSrchIndex,
    unsigned char *pucConfigurationDirectoryPath
)
{

    int     iError = UTL_NoError;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iSrchIndexAbort - psiSrchIndex->pucIndexName: '%s', pucConfigurationDirectoryPath: '%s'.",  */
/*             psiSrchIndex->pucIndexName, pucConfigurationDirectoryPath); */


    /* Check the parameters */
    if ( psiSrchIndex == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'psiSrchIndex' parameter passed to 'iSrchIndexClose'."); 
        return (SRCH_InvalidIndex);
    }

    if ( bUtlStringsIsStringNULL(pucConfigurationDirectoryPath) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pucConfigurationDirectoryPath' parameter passed to 'iSrchIndexAbort'."); 
        return (SRCH_IndexInvalidConfigurationDirectoryPath);
    }

    /* Open the information file */
    switch ( psiSrchIndex->uiIntent ) {

        case SRCH_INDEX_INTENT_CREATE:
        
            {
#if defined(SRCH_INDEX_ENABLE_INDEX_DELETION_ON_ABORT)

                unsigned char   pucIndexName[UTL_FILE_PATH_MAX + 1] = {'\0'};
                unsigned char   pucIndexDirectoryPath[UTL_FILE_PATH_MAX + 1] = {'\0'};

                iUtlLogError(UTL_LOG_CONTEXT, "Index aborted, deleting index, index: '%s'.", psiSrchIndex->pucIndexName);

                /* Save the index name and derive the index directory path */
                if ( bUtlStringsIsStringNULL(psiSrchIndex->pucIndexName) == false ) {
                    s_strnncpy(pucIndexName, psiSrchIndex->pucIndexName, UTL_FILE_PATH_MAX + 1);
                }
                if ( bUtlStringsIsStringNULL(psiSrchIndex->pucIndexPath) == false ) {
                    iUtlFileGetPathDirectoryPath(psiSrchIndex->pucIndexPath, pucIndexDirectoryPath, UTL_FILE_PATH_MAX + 1);
                }
        
                /* Close the index */
                iSrchIndexClose(psiSrchIndex);
        
                /* Delete the index */
                if ( bUtlStringsIsStringNULL(pucIndexName) == false ) {
                    iSrchIndexDelete(pucIndexDirectoryPath, pucConfigurationDirectoryPath, pucIndexName);
                }

#else    /* defined(SRCH_INDEX_ENABLE_INDEX_DELETION_ON_ABORT) */

                iUtlLogError(UTL_LOG_CONTEXT, "Index aborted, index: '%s'.", psiSrchIndex->pucIndexName);

                /* Close the index */
                iSrchIndexClose(psiSrchIndex);

#endif    /* defined(SRCH_INDEX_ENABLE_INDEX_DELETION_ON_ABORT) */
            }
            
            break;
    

        case SRCH_INDEX_INTENT_SEARCH:
    
            /* Close the index */
            iError = iSrchIndexClose(psiSrchIndex);

            break;


        default:
            return (SRCH_CloseIndexFailed);

    }


    return (iError);


}


/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchIndexFilesOpen()

    Purpose:    This opens the index files for search/create

    Parameters: psiSrchIndex    search index structure

    Globals:    none

    Returns:    SRCH error code

*/
static int iSrchIndexFilesOpen
(
    struct srchIndex *psiSrchIndex
)
{

    int             iError = SRCH_NoError;
    unsigned char   pucIndexConfigurationFilePath[UTL_FILE_PATH_MAX + 1] = {'\0'};
    unsigned char   pucDocumentTableFilePath[UTL_FILE_PATH_MAX + 1] = {'\0'};
    unsigned char   pucDocumentDataFilePath[UTL_FILE_PATH_MAX + 1] = {'\0'};
    unsigned char   pucIndexDataFilePath[UTL_FILE_PATH_MAX + 1] = {'\0'};
    unsigned char   pucKeyDictionaryFilePath[UTL_FILE_PATH_MAX + 1] = {'\0'};
    unsigned char   pucTermDictionaryFilePath[UTL_FILE_PATH_MAX + 1] = {'\0'};


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iSrchIndexFilesOpen - psiSrchIndex->pucIndexName: '%s'", psiSrchIndex->pucIndexName); */


    ASSERT(psiSrchIndex != NULL);
    ASSERT(SRCH_INDEX_INTENT_VALID(psiSrchIndex->uiIntent) == true);



    /* Create the index information file path */
    if ( (iError = iSrchFilePathsGetIndexInformationFilePathFromIndex(psiSrchIndex, pucIndexConfigurationFilePath, UTL_FILE_PATH_MAX + 1)) != SRCH_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to create the index information file path, index: '%s', srch error: %d.", psiSrchIndex->pucIndexName, iError);
        return (iError);
    }

    /* Create the document table file path */
    if ( (iError = iSrchFilePathsGetDocumentTableFilePathFromIndex(psiSrchIndex, pucDocumentTableFilePath, UTL_FILE_PATH_MAX + 1)) != SRCH_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to create the document table file path, index: '%s', srch error: %d.", psiSrchIndex->pucIndexName, iError);
        return (iError);
    }

    /* Create the document data file path */
    if ( (iError = iSrchFilePathsGetDocumentDataFilePathFromIndex(psiSrchIndex, pucDocumentDataFilePath, UTL_FILE_PATH_MAX + 1)) != SRCH_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to create the document data file path, index: '%s', srch error: %d.", psiSrchIndex->pucIndexName, iError);
        return (iError);
    }

    /* Create the index data file path */
    if ( (iError = iSrchFilePathsGetIndexDataFilePathFromIndex(psiSrchIndex, pucIndexDataFilePath, UTL_FILE_PATH_MAX + 1)) != SRCH_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to create the index data file path, index: '%s', srch error: %d.", psiSrchIndex->pucIndexName, iError);
        return (iError);
    }

    /* Create the key dictionary file path */
    if ( (iError = iSrchFilePathsGetKeyDictionaryFilePathFromIndex(psiSrchIndex, pucKeyDictionaryFilePath, UTL_FILE_PATH_MAX + 1)) != SRCH_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to create the key dictionary file path, index: '%s', srch error: %d.", psiSrchIndex->pucIndexName, iError);
        return (iError);
    }

    /* Create the term dictionary file path */
    if ( (iError = iSrchFilePathsGetTermDictionaryFilePathFromIndex(psiSrchIndex, pucTermDictionaryFilePath, UTL_FILE_PATH_MAX + 1)) != SRCH_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to create the term dictionary file path, index: '%s', srch error: %d.", psiSrchIndex->pucIndexName, iError);
        return (iError);
    }


    /* Open the index files */
    if ( psiSrchIndex->uiIntent == SRCH_INDEX_INTENT_SEARCH ) {

        /* Open the index information */
        if ( (iError = iUtlConfigOpen(pucIndexConfigurationFilePath, UTL_CONFIG_FILE_FLAG_REQUIRED, &psiSrchIndex->pvUtlIndexInformation)) != UTL_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to open the index information, index: '%s', srch error: %d.", psiSrchIndex->pucIndexName, iError); 
            return (SRCH_IndexOpenFailed);
        }
    
    
        /* Initialize index version information from the information file */
        if ( (iError = iSrchVersionInitFromInfo(psiSrchIndex)) != SRCH_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Invalid index version in information file, index: '%s', srch error: %d.", psiSrchIndex->pucIndexName, iError); 
            return (iError);
        }
    
    
        /* Initialize language parser information from the information file */
        if ( (iError = iSrchLanguageInitFromInfo(psiSrchIndex)) != SRCH_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to initialize the language, index: '%s', srch error: %d.", psiSrchIndex->pucIndexName, iError); 
            return (iError);
        }
    
    
        /* Initialize stemmer information from the information file */
        if ( (iError = iSrchStemmerInitFromInfo(psiSrchIndex)) != SRCH_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to initialize the stemmer, index: '%s', srch error: %d.", psiSrchIndex->pucIndexName, iError); 
            return (iError);
        }
    
    
        /* Initialize term lengths from the information file */
        if ( (iError = iSrchTermLengthInitFromInfo(psiSrchIndex)) != SRCH_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to initialize term lengths, index: '%s', srch error: %d.", psiSrchIndex->pucIndexName, iError); 
            return (iError);
        }
    
    
        /* Get the scalars */
        if ( (iError = iSrchInfoGetScalars(psiSrchIndex)) != SRCH_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the scalars from the information file, index: '%s', srch error: %d.", 
                    psiSrchIndex->pucIndexName, iError); 
            return (iError);
        }
    
    
        /* Open the document table */
        if ( (iError = iUtlTableOpen(pucDocumentTableFilePath, &psiSrchIndex->pvUtlDocumentTable)) != UTL_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to open the document table, index: '%s', utl error: %d.", psiSrchIndex->pucIndexName, iError);
            return (SRCH_IndexOpenFailed);
        }
    
    
        /* Open the document data */
        if ( (iError = iUtlDataOpen(pucDocumentDataFilePath, &psiSrchIndex->pvUtlDocumentData)) != UTL_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to open the document data, index: '%s', utl error: %d.", psiSrchIndex->pucIndexName, iError);
            return (SRCH_IndexOpenFailed);
        }
    
    
        /* Open the index data */
        if ( (iError = iUtlDataOpen(pucIndexDataFilePath, &psiSrchIndex->pvUtlIndexData)) != UTL_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to open the index data, index: '%s', utl error: %d.", psiSrchIndex->pucIndexName, iError);
            return (SRCH_IndexOpenFailed);
        }
    
    
        /* Open the key dictionary */
        if ( (iError = iUtlDictOpen(pucKeyDictionaryFilePath, &psiSrchIndex->pvUtlKeyDictionary)) != UTL_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to open the key dictionary, index: '%s', srch error: %d.", psiSrchIndex->pucIndexName, iError); 
            return (SRCH_IndexOpenFailed);
        }
    
    
        /* Open the term dictionary */
        if ( (iError = iUtlDictOpen(pucTermDictionaryFilePath, &psiSrchIndex->pvUtlTermDictionary)) != UTL_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to open the term dictionary, index: '%s', srch error: %d.", psiSrchIndex->pucIndexName, iError); 
            return (SRCH_IndexOpenFailed);
        }

    }
    
    /* Create the index files */
    else if ( psiSrchIndex->uiIntent == SRCH_INDEX_INTENT_CREATE ) {

        /* Create the index information */
        if ( (iError = iUtlConfigCreate(pucIndexConfigurationFilePath, &psiSrchIndex->pvUtlIndexInformation)) != UTL_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to create the index information, index: '%s', srch error: %d.", psiSrchIndex->pucIndexName, iError); 
            return (SRCH_IndexCreateFailed);
        }
    
    
        /* Initialize the version */
        if ( (iError = iSrchVersionInit(psiSrchIndex, UTL_VERSION_MAJOR, UTL_VERSION_MINOR, UTL_VERSION_PATCH)) != SRCH_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Invalid index version in information file, index: '%s', srch error: %d.", psiSrchIndex->pucIndexName, iError); 
            return (iError);
        }
    
    
        /* Set the version info */
        if ( (iError = iSrchInfoSetVersionInfo(psiSrchIndex)) != SRCH_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to set the version in the information file, srch error: %d.", iError);
            return (iError);
        }
        
    
        /* Create the document table */
        if ( (iError = iUtlTableCreate(pucDocumentTableFilePath, SRCH_DOCUMENT_ENTRY_LENGTH, &psiSrchIndex->pvUtlDocumentTable)) != UTL_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to create the document table, index: '%s', srch error: %d.", psiSrchIndex->pucIndexName, iError); 
            return (SRCH_IndexCreateFailed);
        }
    
    
        /* Create the document data */
        if ( (iError = iUtlDataCreate(pucDocumentDataFilePath, &psiSrchIndex->pvUtlDocumentData)) != UTL_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to create the document data, index: '%s', srch error: %d.", psiSrchIndex->pucIndexName, iError); 
            return (SRCH_IndexCreateFailed);
        }
    
    
        /* Create the index data */
        if ( (iError = iUtlDataCreate(pucIndexDataFilePath, &psiSrchIndex->pvUtlIndexData)) != UTL_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to create the index data, index: '%s', srch error: %d.", psiSrchIndex->pucIndexName, iError); 
            return (SRCH_IndexCreateFailed);
        }
    
    
        /* Create the key dictionary */
        if ( (iError = iUtlDictCreate(pucKeyDictionaryFilePath, SPI_DOCUMENT_KEY_MAXIMUM_LENGTH, &psiSrchIndex->pvUtlKeyDictionary)) != UTL_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to create the key dictionary, index: '%s', srch error: %d.", psiSrchIndex->pucIndexName, iError); 
            return (SRCH_IndexCreateFailed);
        }
    
    
        /* Create the term dictionary */
        if ( (iError = iUtlDictCreate(pucTermDictionaryFilePath, SPI_DOCUMENT_KEY_MAXIMUM_LENGTH, &psiSrchIndex->pvUtlTermDictionary)) != UTL_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to create the term dictionary, index: '%s', srch error: %d.", psiSrchIndex->pucIndexName, iError); 
            return (SRCH_IndexCreateFailed);
        }
    
    }


    return (SRCH_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchIndexFilesClose()

    Purpose:    Close the index files.

                This function will close as much as possible, where errors are
                encountered, they will be reported, but the function will just 
                carry on.

    Parameters: psiSrchIndex    search index structure

    Globals:    none

    Returns:    SRCH error code

*/
static int iSrchIndexFilesClose
(
    struct srchIndex *psiSrchIndex
)
{

    int     iError = UTL_NoError;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iSrchIndexFilesClose - psiSrchIndex->pucIndexName: '%s'", psiSrchIndex->pucIndexName); */


    ASSERT(psiSrchIndex != NULL);
    ASSERT(SRCH_INDEX_INTENT_VALID(psiSrchIndex->uiIntent) == true);


    /* Write out the various information items if we are creating this index */
    if ( psiSrchIndex->uiIntent == SRCH_INDEX_INTENT_CREATE ) {

        /* Write out the language info */
        if ( (iError = iSrchInfoSetLanguageInfo(psiSrchIndex)) != SRCH_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to set the language in the information file, srch error: %d.", iError);
            return (SRCH_IndexCloseFailed);
        }

        /* Write out the tokenizer info */
        if ( (iError = iSrchInfoSetTokenizerInfo(psiSrchIndex)) != SRCH_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to set the tokenizer in the information file, srch error: %d.", iError);
            return (SRCH_IndexCloseFailed);
        }

        /* Write out the stemmer info */
        if ( (iError = iSrchInfoSetStemmerInfo(psiSrchIndex)) != SRCH_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to set the stemmer in the information file, srch error: %d.", iError);
            return (SRCH_IndexCloseFailed);
        }

        /* Write out the stop list info */
        if ( (iError = iSrchInfoSetStopListInfo(psiSrchIndex)) != SRCH_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to set the stop list in the information file, srch error: %d.", iError);
            return (SRCH_IndexCloseFailed);
        }

        /* Write out the term length info */
        if ( (iError = iSrchInfoSetTermLengthInfo(psiSrchIndex)) != SRCH_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to set the term length in the information file, srch error: %d.", iError);
            return (SRCH_IndexCloseFailed);
        }
    
        /* Set the last update time, which is now */
        psiSrchIndex->tLastUpdateTime = s_time(NULL);

        /* Write out the scalars */
        if ( (iError = iSrchInfoSetScalars(psiSrchIndex)) != SRCH_NoError ) {
            return (SRCH_IndexCloseFailed);
        }
    }
    

    /* Close the document table */
    if ( (iError = iUtlTableClose(psiSrchIndex->pvUtlDocumentTable)) != UTL_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to close the document table, index: '%s', utl error: %d.", psiSrchIndex->pucIndexName, iError);
        return (SRCH_IndexCloseFailed);
    }
    psiSrchIndex->pvUtlDocumentTable = NULL;


    /* Close the document data */
    if ( (iError = iUtlDataClose(psiSrchIndex->pvUtlDocumentData)) != UTL_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to close the document data, index: '%s', utl error: %d.", psiSrchIndex->pucIndexName, iError);
        return (SRCH_IndexCloseFailed);
    }
    psiSrchIndex->pvUtlDocumentData = NULL;


    /* Close the index data */
    if ( (iError = iUtlDataClose(psiSrchIndex->pvUtlIndexData)) != UTL_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to close the index data, index: '%s', utl error: %d.", psiSrchIndex->pucIndexName, iError);
        return (SRCH_IndexCloseFailed);
    }
    psiSrchIndex->pvUtlIndexData = NULL;


    /* Close the key dictionary */
    if ( (iError = iUtlDictClose(psiSrchIndex->pvUtlKeyDictionary)) != UTL_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to close the key dictionary, index: '%s', utl error: %d.", psiSrchIndex->pucIndexName, iError);
        return (SRCH_IndexCloseFailed);
    }
    psiSrchIndex->pvUtlKeyDictionary = NULL;


    /* Close the term dictionary */
    if ( (iError = iUtlDictClose(psiSrchIndex->pvUtlTermDictionary)) != UTL_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to close the term dictionary, index: '%s', utl error: %d.", psiSrchIndex->pucIndexName, iError);
        return (SRCH_IndexCloseFailed);
    }
    psiSrchIndex->pvUtlTermDictionary = NULL;


    /* Close the index information */
    if ( (iError = iUtlConfigClose(psiSrchIndex->pvUtlIndexInformation)) != UTL_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to close the index information, index: '%s', utl error: %d.", psiSrchIndex->pucIndexName, iError);
        return (SRCH_IndexCloseFailed);
    }
    psiSrchIndex->pvUtlIndexInformation = NULL;


    return (SRCH_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchIndexLock()

    Purpose:    This function will attempt to set a lock on the index according to
                a set of rules (list below). In addition it will allow us to promote/demote a 
                lock from one type to another. If the promotion/demotion fails, the original
                lock remain.

    Parameters: psiSrchIndex        search index structure

    Globals:    none

    Returns:    SRCH error code

*/
static int iSrchIndexLock
(
    struct srchIndex *psiSrchIndex
)
{

    int             iError = SRCH_NoError;
    unsigned int    uiTimeOut = 0;
    struct flock    flFLock;
    unsigned char   pucIndexLockFilePath[UTL_FILE_PATH_MAX + 1] = {'\0'};
    boolean         bMessageFlag = false;
    unsigned char   pucMode[3] = {'\0'};


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iSrchIndexLock - psiSrchIndex->pucIndexName: '%s'", psiSrchIndex->pucIndexName); */


    ASSERT(psiSrchIndex != NULL);
    ASSERT(SRCH_INDEX_INTENT_VALID(psiSrchIndex->uiIntent) == true);


    /* Create the lock file path */
    if ( (iError = iSrchFilePathsGetIndexLockFilePathFromIndex(psiSrchIndex, pucIndexLockFilePath, UTL_FILE_PATH_MAX + 1)) != SRCH_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to create the lock file path, index: '%s', srch error: %d.", psiSrchIndex->pucIndexName, iError);
        return (SRCH_IndexLockFailed);
    }


    /* Set the file mode based on the intent */
    if ( psiSrchIndex->uiIntent == SRCH_INDEX_INTENT_SEARCH ) {
        s_strnncpy(pucMode, "r", 3); 
    }
    else if ( psiSrchIndex->uiIntent == SRCH_INDEX_INTENT_CREATE ) {
        s_strnncpy(pucMode, (bUtlFileIsFile(pucIndexLockFilePath) == true) ? "r+" : "w+", 3);
    }


    /* Close the lock file */
    s_fclose(psiSrchIndex->pfLockFile);


    /* Create/open the lock file */
    if ( (iError = iSrchFilePathsGetIndexLockFilePathFromIndex(psiSrchIndex, pucIndexLockFilePath, UTL_FILE_PATH_MAX + 1)) != SRCH_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to create the lock file path, index: '%s', srch error: %d.", psiSrchIndex->pucIndexName, iError);
        return (SRCH_IndexLockFailed);
    }

    if ( (psiSrchIndex->pfLockFile = s_fopen(pucIndexLockFilePath, pucMode)) == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to create/open the lock file, index: '%s'.", psiSrchIndex->pucIndexName);
        return (SRCH_IndexLockFailed);
    }


    /* Set up the flock structure */
    flFLock.l_type = F_UNLCK;
    flFLock.l_whence = SEEK_SET;
    flFLock.l_start = 0;
    flFLock.l_len = 0;
    flFLock.l_pid = 0;
    

    /* Set the shared lock */
    if ( psiSrchIndex->uiIntent == SRCH_INDEX_INTENT_SEARCH ) {

        /* Set up the flock structure for a read lock */
        flFLock.l_type = F_RDLCK;

        /* Loop while we have not timed out */
        do {

            /* Try to place a shared lock, return here if successful */
            if ( fcntl(fileno(psiSrchIndex->pfLockFile), F_SETLK, &flFLock) != -1 ) {
                return (SRCH_NoError);
            }

            /* Go to sleep and increment the timeout */
            s_usleep(SRCH_INDEX_SHARED_LOCK_SLEEP);
            uiTimeOut += SRCH_INDEX_SHARED_LOCK_SLEEP;

        } while ( uiTimeOut < SRCH_INDEX_SHARED_LOCK_TIMEOUT );

        /* Failed to place a shared lock */
        iUtlLogError(UTL_LOG_CONTEXT, "Timed out waiting to get a shared lock, index: '%s'.", psiSrchIndex->pucIndexName);

    }
    
    /* Set the exclusive lock */
    else if ( psiSrchIndex->uiIntent == SRCH_INDEX_INTENT_CREATE ) {

        /* Set up the flock structure for a write lock */
        flFLock.l_type = F_WRLCK;
    
        /* Loop while we have not timed out */
        do {

            /* Try to place an exclusive lock, return here if successful */
            if ( fcntl(fileno(psiSrchIndex->pfLockFile), F_SETLK, &flFLock) != -1 ) {
                /* Log a new message if we logged a message that we were waiting */
                if ( bMessageFlag == true ) {
                    iUtlLogInfo(UTL_LOG_CONTEXT, "Got an exclusive lock, index: '%s'.", psiSrchIndex->pucIndexName);
                }
                return (SRCH_NoError);
            }


            /* Failed to place an exclusive lock, either someone is searching or updating 
            ** the index, we test this out by trying to place a shared lock, if we can
            ** place a shared lock, then someone else has a shared lock too, if we cant
            ** place a shared lock, then someone else has an exclusive lock
            */
            if ( fcntl(fileno(psiSrchIndex->pfLockFile), F_SETLK, &flFLock) == -1 ) {
                /* Failed to place a shared lock */
                iUtlLogError(UTL_LOG_CONTEXT, "Failed to get an exclusive lock, index: '%s'.", psiSrchIndex->pucIndexName);
                return (SRCH_IndexLockFailed);
            }

            /* Log a message if we havent done so already (we can tell from the flag) */
            if ( bMessageFlag == false ) {
                iUtlLogInfo(UTL_LOG_CONTEXT, "Waiting to get an exclusive lock, index: '%s'.", psiSrchIndex->pucIndexName);
                bMessageFlag = true;
            }

            /* Go to sleep and increment the timeout */
            s_sleep(SRCH_INDEX_EXCLUSIVE_LOCK_SLEEP);
            uiTimeOut += SRCH_INDEX_EXCLUSIVE_LOCK_SLEEP;

        } while ( uiTimeOut < SRCH_INDEX_EXCLUSIVE_LOCK_TIMEOUT );

        /* Failed to place an exclusive lock */
        iUtlLogError(UTL_LOG_CONTEXT, "Timed out waiting to get an exclusive lock, index: '%s'.", psiSrchIndex->pucIndexName);

    }


    return (SRCH_IndexLockFailed);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchIndexUnlock()

    Purpose:    This function will release the current lock being
                held on a index 

    Parameters: psiSrchIndex    search index structure

    Globals:    none

    Returns:    SRCH error code

*/
static int iSrchIndexUnlock
(
    struct srchIndex *psiSrchIndex
)
{

    struct flock    flFLock;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iSrchIndexUnlock - psiSrchIndex->pucIndexName: '%s'u", psiSrchIndex->pucIndexName); */


    ASSERT(psiSrchIndex != NULL);


    /* Unlock and close the lock file */
    if ( psiSrchIndex->pfLockFile != NULL ) {
    
        /* Set up the flock structure */
        flFLock.l_type = F_UNLCK;
        flFLock.l_whence = SEEK_SET;
        flFLock.l_start = 0;
        flFLock.l_len = 0;
        flFLock.l_pid = 0;
        
        /* Release the lock we are currently holding */
        if ( fcntl(fileno(psiSrchIndex->pfLockFile), F_SETLK, &flFLock) == -1 ) {
            return(SRCH_IndexUnlockFailed);
        }
    
        /* Close the lock file */
        s_fclose(psiSrchIndex->pfLockFile);
    }
    

    return (SRCH_NoError);

}


/*---------------------------------------------------------------------------*/
