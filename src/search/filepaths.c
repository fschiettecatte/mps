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

    Module:     filepaths.c

    Author:     Francois Schiettecatte (FS Consulting LLC.)

    Created:    24 February 1994

    Purpose:    This module contains the functions which return 
                the file paths of the various files which make up
                a index.

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
#define UTL_LOG_CONTEXT                     (unsigned char *)"com.fsconsult.mps.src.search.filepaths"


/* Add process ID to temporary files */
#define SRCH_FILE_PATHS_ADD_PROCESS_ID_TO_TMP_FILES     (1)


/*---------------------------------------------------------------------------*/


/*
** Defines
*/

/* File names */
#define SRCH_FILE_PATHS_TERM_DICTIONARY_FILENAME        (unsigned char *)"term.dct"
#define SRCH_FILE_PATHS_KEY_DICTIONARY_FILENAME         (unsigned char *)"key.dct"
#define SRCH_FILE_PATHS_DOCUMENT_TABLE_FILENAME         (unsigned char *)"document.tab"
#define SRCH_FILE_PATHS_DOCUMENT_DATA_FILENAME          (unsigned char *)"document.dat"
#define SRCH_FILE_PATHS_INDEX_DATA_FILENAME             (unsigned char *)"index.dat"
#define SRCH_FILE_PATHS_INDEX_INFORMATION_FILENAME      (unsigned char *)"index.inf"
#define SRCH_FILE_PATHS_INDEX_LOCK_FILENAME             (unsigned char *)"index.lck"


/* Temporary file name addition */
#define SRCH_FILE_PATHS_TMP_TERM_INDEX_FILE_ADDITION    (unsigned char *)"term"
#define SRCH_FILE_PATHS_TMP_KEY_INDEX_FILE_ADDITION     (unsigned char *)"key"


/*---------------------------------------------------------------------------*/


/*
** Private function prototypes
*/

static int iSrchFilePathsGetFilePathFromIndexPath (unsigned char *pucIndexPath, 
        unsigned char *pucLeafFilePath, unsigned char *pucFilePath, unsigned int uiFilePathLength);


/*---------------------------------------------------------------------------*/


/* 
** ================================
** ===  Index FilePath Support  ===
** ================================
*/


/*

    Function:   iSrchFilePathsGetTempTermDictionaryFilePathFromIndex()

    Purpose:    Return the temp terms dictionary file path

    Parameters: psiSrchIndex        search index structure
                uiVersion           version number
                bShadowFile         shadow file
                pucFilePath         return pointer for the file path
                uiFilePathLength    length of the return pointer for the file path

    Globals:    none

    Returns:    SRCH error name

*/
int iSrchFilePathsGetTempTermDictionaryFilePathFromIndex
(
    struct srchIndex *psiSrchIndex,
    unsigned int uiVersion,
    boolean bShadowFile,
    unsigned char *pucFilePath,
    unsigned int uiFilePathLength
)
{

    int             iError = SRCH_NoError;
    unsigned char   pucFileName[UTL_FILE_PATH_MAX + 1] = {'\0'};


    /* Check the parameters */
    if ( psiSrchIndex == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'psiSrchIndex' parameter passed to 'iSrchFilePathsGetTempTermDictionaryFilePathFromIndex'."); 
        return (SRCH_InvalidIndex);
    }

    if ( uiVersion < 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiVersion' parameter passed to 'iSrchFilePathsGetTempTermDictionaryFilePathFromIndex'."); 
        return (SRCH_FilePathsInvalidVersion);
    }

    if ( pucFilePath == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pucFilePath' parameter passed to 'iSrchFilePathsGetTempTermDictionaryFilePathFromIndex'."); 
        return (SRCH_ReturnParameterError);
    }

    if ( uiFilePathLength <= 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiFilePathLength' parameter passed to 'iSrchFilePathsGetTempTermDictionaryFilePathFromIndex'."); 
        return (SRCH_ReturnParameterError);
    }


    /* Use the temporary directory if it was provided */
    if ( psiSrchIndex->psibSrchIndexBuild->pucTemporaryDirectoryPath != NULL ) {
    
        /* Create the file name, prepend the index base name to the name */
        snprintf(pucFileName, UTL_FILE_PATH_MAX + 1, "%s-%s.%s%03u", psiSrchIndex->pucIndexName, 
                SRCH_FILE_PATHS_TMP_TERM_INDEX_FILE_ADDITION, ((bShadowFile == true) ? "-" : ""), uiVersion);

        /* Create the final file path */
        if ( (iError = iUtlFileMergePaths(psiSrchIndex->psibSrchIndexBuild->pucTemporaryDirectoryPath, pucFileName, pucFilePath, uiFilePathLength)) != UTL_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to create the terms index file path, term index file name: '%s', temporary directory path: '%s', utl error: %d.", 
                    pucFileName, psiSrchIndex->psibSrchIndexBuild->pucTemporaryDirectoryPath, iError); 
            return (SRCH_FilePathsFailed);
        }
    }
    else {

        /* Create the file name */
        snprintf(pucFileName, UTL_FILE_PATH_MAX + 1, "%s.%s%03u", SRCH_FILE_PATHS_TMP_TERM_INDEX_FILE_ADDITION, ((bShadowFile == true) ? "-" : ""), uiVersion);

        /* Create the final file path */
        if ( (iError = iUtlFileMergePaths(psiSrchIndex->pucIndexPath, pucFileName, pucFilePath, uiFilePathLength)) != UTL_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to create the terms index file path, term index file name: '%s', index: '%s', utl error: %d.", 
                    pucFileName, psiSrchIndex->pucIndexName, iError); 
            return (SRCH_FilePathsFailed);
        }
    }


    return (SRCH_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchFilePathsGetTempKeyDictionaryFilePathFromIndex()

    Purpose:    Return the temp keys dictionary file path

    Parameters: psiSrchIndex        search index structure
                uiVersion           version number
                bShadowFile         shadow file
                pucFilePath         return pointer for the file path
                uiFilePathLength    length of the return pointer for the file path

    Globals:    none

    Returns:    SRCH error name

*/
int iSrchFilePathsGetTempKeyDictionaryFilePathFromIndex
(
    struct srchIndex *psiSrchIndex,
    unsigned int uiVersion,
    boolean bShadowFile,
    unsigned char *pucFilePath,
    unsigned int uiFilePathLength
)
{

    int             iError = SRCH_NoError;
    unsigned char   pucFileName[UTL_FILE_PATH_MAX + 1] = {'\0'};


    /* Check the parameters */
    if ( psiSrchIndex == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'psiSrchIndex' parameter passed to 'iSrchFilePathsGetTempKeyDictionaryFilePathFromIndex'."); 
        return (SRCH_InvalidIndex);
    }

    if ( uiVersion < 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiVersion' parameter passed to 'iSrchFilePathsGetTempKeyDictionaryFilePathFromIndex'."); 
        return (SRCH_FilePathsInvalidVersion);
    }

    if ( pucFilePath == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pucFilePath' parameter passed to 'iSrchFilePathsGetTempKeyDictionaryFilePathFromIndex'."); 
        return (SRCH_ReturnParameterError);
    }

    if ( uiFilePathLength <= 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiFilePathLength' parameter passed to 'iSrchFilePathsGetTempKeyDictionaryFilePathFromIndex'."); 
        return (SRCH_ReturnParameterError);
    }


    /* Use the temporary directory if it is provided */
    if ( psiSrchIndex->psibSrchIndexBuild->pucTemporaryDirectoryPath != NULL ) {
    
        /* Create the file name, prepend the index base name to the name */
        snprintf(pucFileName, UTL_FILE_PATH_MAX + 1, "%s-%s.%s%03u", psiSrchIndex->pucIndexName, 
                SRCH_FILE_PATHS_TMP_KEY_INDEX_FILE_ADDITION, ((bShadowFile == true) ? "-" : ""), uiVersion);
        
        /* Create the final file path */
        if ( (iError = iUtlFileMergePaths(psiSrchIndex->psibSrchIndexBuild->pucTemporaryDirectoryPath, pucFileName, pucFilePath, uiFilePathLength)) != UTL_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to create the keys index file path, document keys index file name: '%s', temporary directory path: '%s', utl error: %d.", 
                    pucFileName, psiSrchIndex->psibSrchIndexBuild->pucTemporaryDirectoryPath, iError); 
            return (SRCH_FilePathsFailed);
        }
    }
    else {

        /* Create the file name */
        snprintf(pucFileName, UTL_FILE_PATH_MAX + 1, "%s.%s%03u", SRCH_FILE_PATHS_TMP_KEY_INDEX_FILE_ADDITION, ((bShadowFile == true) ? "-" : ""), uiVersion);

        /* Create the final file path */
        if ( (iError = iUtlFileMergePaths(psiSrchIndex->pucIndexPath, pucFileName, pucFilePath, uiFilePathLength)) != UTL_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to create the keys index file path, term index file name: '%s', index: '%s', utl error: %d.", 
                    pucFileName, psiSrchIndex->pucIndexName, iError); 
            return (SRCH_FilePathsFailed);
        }
    }


    return (SRCH_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchFilePathsGetTermDictionaryFilePathFromIndex()

    Purpose:    Constructs and returns the terms dictionary file path from the index.

    Parameters: psiSrchIndex        search index structure
                pucFilePath         return pointer for the file path
                uiFilePathLength    length of the return pointer for the file path

    Globals:    none

    Returns:    SRCH error name

*/
int iSrchFilePathsGetTermDictionaryFilePathFromIndex
(
    struct srchIndex *psiSrchIndex,
    unsigned char *pucFilePath,
    unsigned int uiFilePathLength
)
{

    /* Check the parameters */
    if ( psiSrchIndex == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'psiSrchIndex' parameter passed to 'iSrchFilePathsGetTermDictionaryFilePathFromIndex'."); 
        return (SRCH_InvalidIndex);
    }

    if ( pucFilePath == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pucFilePath' parameter passed to 'iSrchFilePathsGetTermDictionaryFilePathFromIndex'."); 
        return (SRCH_ReturnParameterError);
    }

    if ( uiFilePathLength <= 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'uiFilePathLength' parameter passed to 'iSrchFilePathsGetTermDictionaryFilePathFromIndex'."); 
        return (SRCH_ReturnParameterError);
    }


    /* Get and return the terms dictionary file path */
    return (iSrchFilePathsGetFilePathFromIndexPath(psiSrchIndex->pucIndexPath, SRCH_FILE_PATHS_TERM_DICTIONARY_FILENAME, pucFilePath, uiFilePathLength));

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchFilePathsGetKeyDictionaryFilePathFromIndex()

    Purpose:    Constructs and returns the keys dictionary file path from the index.

    Parameters: psiSrchIndex        search index structure
                pucFilePath         return pointer for the file path
                uiFilePathLength    length of the return pointer for the file path

    Globals:    none

    Returns:    SRCH error name

*/
int iSrchFilePathsGetKeyDictionaryFilePathFromIndex
(
    struct srchIndex *psiSrchIndex,
    unsigned char *pucFilePath,
    unsigned int uiFilePathLength
)
{

    /* Check the parameters */
    if ( psiSrchIndex == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'psiSrchIndex' parameter passed to 'iSrchFilePathsGetKeyDictionaryFilePathFromIndex'."); 
        return (SRCH_InvalidIndex);
    }

    if ( pucFilePath == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pucFilePath' parameter passed to 'iSrchFilePathsGetKeyDictionaryFilePathFromIndex'."); 
        return (SRCH_ReturnParameterError);
    }

    if ( uiFilePathLength <= 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'uiFilePathLength' parameter passed to 'iSrchFilePathsGetKeyDictionaryFilePathFromIndex'."); 
        return (SRCH_ReturnParameterError);
    }


    /* Get and return the keys dictionary file path */
    return (iSrchFilePathsGetFilePathFromIndexPath(psiSrchIndex->pucIndexPath, SRCH_FILE_PATHS_KEY_DICTIONARY_FILENAME, pucFilePath, uiFilePathLength));

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchFilePathsGetDocumentTableFilePathFromIndex()

    Purpose:    Constructs and returns the documents table file path from the index.

    Parameters: psiSrchIndex        search index structure
                pucFilePath         return pointer for the file path
                uiFilePathLength    length of the return pointer for the file path

    Globals:    none

    Returns:    SRCH error name

*/
int iSrchFilePathsGetDocumentTableFilePathFromIndex
(
    struct srchIndex *psiSrchIndex,
    unsigned char *pucFilePath,
    unsigned int uiFilePathLength
)
{

    /* Check the parameters */
    if ( psiSrchIndex == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'psiSrchIndex' parameter passed to 'iSrchFilePathsGetDocumentTableFilePathFromIndex'."); 
        return (SRCH_InvalidIndex);
    }

    if ( pucFilePath == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pucFilePath' parameter passed to 'iSrchFilePathsGetDocumentTableFilePathFromIndex'."); 
        return (SRCH_ReturnParameterError);
    }

    if ( uiFilePathLength <= 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'uiFilePathLength' parameter passed to 'iSrchFilePathsGetDocumentTableFilePathFromIndex'."); 
        return (SRCH_ReturnParameterError);
    }


    /* Get and return the documents table file path */
    return (iSrchFilePathsGetFilePathFromIndexPath(psiSrchIndex->pucIndexPath, SRCH_FILE_PATHS_DOCUMENT_TABLE_FILENAME, pucFilePath, uiFilePathLength));

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchFilePathsGetDocumentDataFilePathFromIndex()

    Purpose:    Constructs and returns the headlines data file path from the index.

    Parameters: psiSrchIndex        search index structure
                pucFilePath         return pointer for the file path
                uiFilePathLength    length of the return pointer for the file path

    Globals:    none

    Returns:    SRCH error name

*/
int iSrchFilePathsGetDocumentDataFilePathFromIndex
(
    struct srchIndex *psiSrchIndex,
    unsigned char *pucFilePath,
    unsigned int uiFilePathLength
)
{

    /* Check the parameters */
    if ( psiSrchIndex == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'psiSrchIndex' parameter passed to 'iSrchFilePathsGetDocumentDataFilePathFromIndex'."); 
        return (SRCH_InvalidIndex);
    }

    if ( pucFilePath == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pucFilePath' parameter passed to 'iSrchFilePathsGetDocumentDataFilePathFromIndex'."); 
        return (SRCH_ReturnParameterError);
    }

    if ( uiFilePathLength <= 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'uiFilePathLength' parameter passed to 'iSrchFilePathsGetDocumentDataFilePathFromIndex'."); 
        return (SRCH_ReturnParameterError);
    }


    /* Get and return the headlines data file path */
    return (iSrchFilePathsGetFilePathFromIndexPath(psiSrchIndex->pucIndexPath, SRCH_FILE_PATHS_DOCUMENT_DATA_FILENAME, pucFilePath, uiFilePathLength));

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchFilePathsGetIndexDataFilePathFromIndex()

    Purpose:    Constructs and returns the postings data file path from the index.

    Parameters: psiSrchIndex        search index structure
                pucFilePath         return pointer for the file path
                uiFilePathLength    length of the return pointer for the file path

    Globals:    none

    Returns:    SRCH error name

*/
int iSrchFilePathsGetIndexDataFilePathFromIndex
(
    struct srchIndex *psiSrchIndex,
    unsigned char *pucFilePath,
    unsigned int uiFilePathLength
)
{

    /* Check the parameters */
    if ( psiSrchIndex == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'psiSrchIndex' parameter passed to 'iSrchFilePathsGetIndexDataFilePathFromIndex'."); 
        return (SRCH_InvalidIndex);
    }

    if ( pucFilePath == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pucFilePath' parameter passed to 'iSrchFilePathsGetIndexDataFilePathFromIndex'."); 
        return (SRCH_ReturnParameterError);
    }

    if ( uiFilePathLength <= 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'uiFilePathLength' parameter passed to 'iSrchFilePathsGetIndexDataFilePathFromIndex'."); 
        return (SRCH_ReturnParameterError);
    }


    /* Get and return the postings data file path */
    return (iSrchFilePathsGetFilePathFromIndexPath(psiSrchIndex->pucIndexPath, SRCH_FILE_PATHS_INDEX_DATA_FILENAME, pucFilePath, uiFilePathLength));

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchFilePathsGetIndexLockFilePathFromIndex()

    Purpose:    Constructs and returns the index lock file path from the index.

    Parameters: psiSrchIndex        search index structure
                pucFilePath         return pointer for the file path
                uiFilePathLength    length of the return pointer for the file path

    Globals:    none

    Returns:    SRCH error name

*/
int iSrchFilePathsGetIndexLockFilePathFromIndex
(
    struct srchIndex *psiSrchIndex,
    unsigned char *pucFilePath,
    unsigned int uiFilePathLength
)
{

    /* Check the parameters */
    if ( psiSrchIndex == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'psiSrchIndex' parameter passed to 'iSrchFilePathsGetIndexLockFilePathFromIndex'."); 
        return (SRCH_InvalidIndex);
    }

    if ( pucFilePath == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pucFilePath' parameter passed to 'iSrchFilePathsGetIndexLockFilePathFromIndex'."); 
        return (SRCH_ReturnParameterError);
    }

    if ( uiFilePathLength <= 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'uiFilePathLength' parameter passed to 'iSrchFilePathsGetIndexLockFilePathFromIndex'."); 
        return (SRCH_ReturnParameterError);
    }


    /* Get and return the index lock file path */
    return (iSrchFilePathsGetFilePathFromIndexPath(psiSrchIndex->pucIndexPath, SRCH_FILE_PATHS_INDEX_LOCK_FILENAME, pucFilePath, uiFilePathLength));

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchFilePathsGetIndexInformationFilePathFromIndex()

    Purpose:    Constructs and returns the index information file path from the index.

    Parameters: psiSrchIndex        search index structure
                pucFilePath         return pointer for the file path
                uiFilePathLength    length of the return pointer for the file path

    Globals:    none

    Returns:    SRCH error name

*/
int iSrchFilePathsGetIndexInformationFilePathFromIndex
(
    struct srchIndex *psiSrchIndex,
    unsigned char *pucFilePath,
    unsigned int uiFilePathLength
)
{

    /* Check the parameters */
    if ( psiSrchIndex == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'psiSrchIndex' parameter passed to 'iSrchFilePathsGetIndexInformationFilePathFromIndex'."); 
        return (SRCH_InvalidIndex);
    }

    if ( pucFilePath == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pucFilePath' parameter passed to 'iSrchFilePathsGetIndexInformationFilePathFromIndex'."); 
        return (SRCH_ReturnParameterError);
    }

    if ( uiFilePathLength <= 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'uiFilePathLength' parameter passed to 'iSrchFilePathsGetIndexInformationFilePathFromIndex'."); 
        return (SRCH_ReturnParameterError);
    }


    /* Get and return the index information file path */
    return (iSrchFilePathsGetFilePathFromIndexPath(psiSrchIndex->pucIndexPath, SRCH_FILE_PATHS_INDEX_INFORMATION_FILENAME, pucFilePath, uiFilePathLength));

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchFilePathsGetTermDictionaryFilePathFromIndexPath()

    Purpose:    Constructs and returns the terms dictionary path from the index path.

    Parameters: pucIndexPath        index path
                pucFilePath         return pointer for the file path
                uiFilePathLength    length of the return pointer for the file path

    Globals:    none

    Returns:    SRCH error name

*/
int iSrchFilePathsGetTermDictionaryFilePathFromIndexPath
(
    unsigned char *pucIndexPath,
    unsigned char *pucFilePath,
    unsigned int uiFilePathLength
)
{

    /* Check the parameters */
    if ( bUtlStringsIsStringNULL(pucIndexPath) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pucIndexPath' parameter passed to 'iSrchFilePathsGetTermDictionaryFilePathFromIndexPath'."); 
        return (SRCH_InvalidIndex);
    }

    if ( pucFilePath == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pucFilePath' parameter passed to 'iSrchFilePathsGetTermDictionaryFilePathFromIndexPath'."); 
        return (SRCH_ReturnParameterError);
    }

    if ( uiFilePathLength <= 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'uiFilePathLength' parameter passed to 'iSrchFilePathsGetTermDictionaryFilePathFromIndexPath'."); 
        return (SRCH_ReturnParameterError);
    }


    /* Get and return the terms dictionary file path */
    return (iSrchFilePathsGetFilePathFromIndexPath(pucIndexPath, SRCH_FILE_PATHS_TERM_DICTIONARY_FILENAME, pucFilePath, uiFilePathLength));

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchFilePathsGetKeyDictionaryFilePathFromIndexPath()

    Purpose:    Constructs and returns the keys dictionary file path from the index path.

    Parameters: pucIndexPath        index path
                pucFilePath         return pointer for the file path
                uiFilePathLength    length of the return pointer for the file path

    Globals:    none

    Returns:    SRCH error name

*/
int iSrchFilePathsGetKeyDictionaryFilePathFromIndexPath
(
    unsigned char *pucIndexPath,
    unsigned char *pucFilePath,
    unsigned int uiFilePathLength
)
{

    /* Check the parameters */
    if ( bUtlStringsIsStringNULL(pucIndexPath) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pucIndexPath' parameter passed to 'iSrchFilePathsGetKeyDictionaryFilePathFromIndexPath'."); 
        return (SRCH_InvalidIndex);
    }

    if ( pucFilePath == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pucFilePath' parameter passed to 'iSrchFilePathsGetKeyDictionaryFilePathFromIndexPath'."); 
        return (SRCH_ReturnParameterError);
    }

    if ( uiFilePathLength <= 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'uiFilePathLength' parameter passed to 'iSrchFilePathsGetKeyDictionaryFilePathFromIndexPath'."); 
        return (SRCH_ReturnParameterError);
    }


    /* Get and return the keys dictionary file path */
    return (iSrchFilePathsGetFilePathFromIndexPath(pucIndexPath, SRCH_FILE_PATHS_KEY_DICTIONARY_FILENAME, pucFilePath, uiFilePathLength));

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchFilePathsGetDocumentTableFilePathFromIndexPath()

    Purpose:    Constructs and returns the documents table file path from the index path.

    Parameters: pucIndexPath        index path
                pucFilePath         return pointer for the file path
                uiFilePathLength    length of the return pointer for the file path

    Globals:    none

    Returns:    SRCH error name

*/
int iSrchFilePathsGetDocumentTableFilePathFromIndexPath
(
    unsigned char *pucIndexPath,
    unsigned char *pucFilePath,
    unsigned int uiFilePathLength
)
{

    /* Check the parameters */
    if ( bUtlStringsIsStringNULL(pucIndexPath) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pucIndexPath' parameter passed to 'iSrchFilePathsGetDocumentTableFilePathFromIndexPath'."); 
        return (SRCH_InvalidIndex);
    }

    if ( pucFilePath == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pucFilePath' parameter passed to 'iSrchFilePathsGetDocumentTableFilePathFromIndexPath'."); 
        return (SRCH_ReturnParameterError);
    }

    if ( uiFilePathLength <= 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'uiFilePathLength' parameter passed to 'iSrchFilePathsGetDocumentTableFilePathFromIndexPath'."); 
        return (SRCH_ReturnParameterError);
    }


    /* Get and return the documents table file path */
    return (iSrchFilePathsGetFilePathFromIndexPath(pucIndexPath, SRCH_FILE_PATHS_DOCUMENT_TABLE_FILENAME, pucFilePath, uiFilePathLength));

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchFilePathsGetDocumentDataFilePathFromIndexPath()

    Purpose:    Constructs and returns the headlines data file path from the index path.

    Parameters: pucIndexPath        index path
                pucFilePath         return pointer for the file path
                uiFilePathLength    length of the return pointer for the file path

    Globals:    none

    Returns:    SRCH error name

*/
int iSrchFilePathsGetDocumentDataFilePathFromIndexPath
(
    unsigned char *pucIndexPath,
    unsigned char *pucFilePath,
    unsigned int uiFilePathLength
)
{

    /* Check the parameters */
    if ( bUtlStringsIsStringNULL(pucIndexPath) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pucIndexPath' parameter passed to 'iSrchFilePathsGetDocumentDataFilePathFromIndexPath'."); 
        return (SRCH_InvalidIndex);
    }

    if ( pucFilePath == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pucFilePath' parameter passed to 'iSrchFilePathsGetDocumentDataFilePathFromIndexPath'."); 
        return (SRCH_ReturnParameterError);
    }

    if ( uiFilePathLength <= 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'uiFilePathLength' parameter passed to 'iSrchFilePathsGetDocumentDataFilePathFromIndexPath'."); 
        return (SRCH_ReturnParameterError);
    }


    /* Get and return the headlines data file path */
    return (iSrchFilePathsGetFilePathFromIndexPath(pucIndexPath, SRCH_FILE_PATHS_DOCUMENT_DATA_FILENAME, pucFilePath, uiFilePathLength));

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchFilePathsGetIndexDataFilePathFromIndexPath()

    Purpose:    Constructs and returns the postings data file path from the index path.

    Parameters: pucIndexPath        index path
                pucFilePath         return pointer for the file path
                uiFilePathLength    length of the return pointer for the file path

    Globals:    none

    Returns:    SRCH error name

*/
int iSrchFilePathsGetIndexDataFilePathFromIndexPath
(
    unsigned char *pucIndexPath,
    unsigned char *pucFilePath,
    unsigned int uiFilePathLength
)
{

    /* Check the parameters */
    if ( bUtlStringsIsStringNULL(pucIndexPath) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pucIndexPath' parameter passed to 'iSrchFilePathsGetIndexDataFilePathFromIndexPath'."); 
        return (SRCH_InvalidIndex);
    }

    if ( pucFilePath == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pucFilePath' parameter passed to 'iSrchFilePathsGetIndexDataFilePathFromIndexPath'."); 
        return (SRCH_ReturnParameterError);
    }

    if ( uiFilePathLength <= 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'uiFilePathLength' parameter passed to 'iSrchFilePathsGetIndexDataFilePathFromIndexPath'."); 
        return (SRCH_ReturnParameterError);
    }


    /* Get and return the postings data file path */
    return (iSrchFilePathsGetFilePathFromIndexPath(pucIndexPath, SRCH_FILE_PATHS_INDEX_DATA_FILENAME, pucFilePath, uiFilePathLength));

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchFilePathsGetIndexLockFilePathFromIndexPath()

    Purpose:    Constructs and returns the lock file path from the index path.

    Parameters: pucIndexPath        index path
                pucFilePath         return pointer for the file path
                uiFilePathLength    length of the return pointer for the file path

    Globals:    none

    Returns:    SRCH error name

*/
int iSrchFilePathsGetIndexLockFilePathFromIndexPath
(
    unsigned char *pucIndexPath,
    unsigned char *pucFilePath,
    unsigned int uiFilePathLength
)
{

    /* Check the parameters */
    if ( bUtlStringsIsStringNULL(pucIndexPath) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pucIndexPath' parameter passed to 'iSrchFilePathsGetIndexLockFilePathFromIndexPath'."); 
        return (SRCH_InvalidIndex);
    }

    if ( pucFilePath == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pucFilePath' parameter passed to 'iSrchFilePathsGetIndexLockFilePathFromIndexPath'."); 
        return (SRCH_ReturnParameterError);
    }

    if ( uiFilePathLength <= 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'uiFilePathLength' parameter passed to 'iSrchFilePathsGetIndexLockFilePathFromIndexPath'."); 
        return (SRCH_ReturnParameterError);
    }


    /* Get and return the lock file path */
    return (iSrchFilePathsGetFilePathFromIndexPath(pucIndexPath, SRCH_FILE_PATHS_INDEX_LOCK_FILENAME, pucFilePath, uiFilePathLength));

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchFilePathsGetIndexInformationFilePathFromIndexPath()

    Purpose:    Constructs and returns the information file path from the index path.

    Parameters: pucIndexPath        index path
                pucFilePath         return pointer for the file path
                uiFilePathLength    length of the return pointer for the file path

    Globals:    none

    Returns:    SRCH error name

*/
int iSrchFilePathsGetIndexInformationFilePathFromIndexPath
(
    unsigned char *pucIndexPath,
    unsigned char *pucFilePath,
    unsigned int uiFilePathLength
)
{

    /* Check the parameters */
    if ( bUtlStringsIsStringNULL(pucIndexPath) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pucIndexPath' parameter passed to 'iSrchFilePathsGetIndexInformationFilePathFromIndexPath'."); 
        return (SRCH_FilePathsInvalidIndexPath);
    }

    if ( pucFilePath == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pucFilePath' parameter passed to 'iSrchFilePathsGetIndexInformationFilePathFromIndexPath'."); 
        return (SRCH_ReturnParameterError);
    }

    if ( uiFilePathLength <= 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'uiFilePathLength' parameter passed to 'iSrchFilePathsGetIndexInformationFilePathFromIndexPath'."); 
        return (SRCH_ReturnParameterError);
    }


    /* Get and return the information file path */
    return (iSrchFilePathsGetFilePathFromIndexPath(pucIndexPath, SRCH_FILE_PATHS_INDEX_INFORMATION_FILENAME, pucFilePath, uiFilePathLength));

}


/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchFilePathsGetFilePathFromIndexPath()

    Purpose:    Constructs and returns a file path for a index path
                from the file name

    Parameters: pucIndexPath        index path
                pucFileName         file name
                pucFilePath         return pointer for the file path
                uiFilePathLength    length of the return pointer for the file path

    Globals:    none

    Returns:    SRCH error name

*/
static int iSrchFilePathsGetFilePathFromIndexPath
(
    unsigned char *pucIndexPath,
    unsigned char *pucFileName,
    unsigned char *pucFilePath,
    unsigned int uiFilePathLength
)
{

    int     iError = SRCH_NoError;


    ASSERT(bUtlStringsIsStringNULL(pucIndexPath) == false);
    ASSERT(bUtlStringsIsStringNULL(pucFileName) == false);
    ASSERT(pucFilePath != NULL);
    ASSERT(uiFilePathLength > 0);


    /* Create the file path */
    if ( (iError = iUtlFileMergePaths(pucIndexPath, pucFileName, pucFilePath, uiFilePathLength)) != UTL_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to create the file path, file name: '%s', index path: '%s', utl error: %d.", pucFileName, pucIndexPath, iError); 
        return (SRCH_FilePathsFailed);
    }


    return (SRCH_NoError);

}


/*---------------------------------------------------------------------------*/
