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

    Module:     invert.c

    Author:     Francois Schiettecatte (FS Consulting LLC.)

    Created:    24 February 1994

    Purpose:    This module manages the inverted files

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
#define UTL_LOG_CONTEXT                     (unsigned char *)"com.fsconsult.mps.src.search.invert"


/*---------------------------------------------------------------------------*/

/*
** Defines
*/

/* The space needed to store the index block data length in the intermediate index */
#define SRCH_INVERT_INDEX_BLOCK_DATA_LENGTH_SIZE                UTL_NUM_UINT_MAX_SIZE


/* The space needed to store the compressed index block data length in the final index */
#define SRCH_INVERT_INDEX_BLOCK_DATA_COMPRESSED_LENGTH_SIZE     UTL_NUM_COMPRESSED_UINT_MAX_SIZE


/* Initial length of a index entry in memory, this 
** is used to set the initial size of the index block
*/
#define SRCH_INVERT_INITIAL_INDEX_BLOCK_LENGTH                  (6)


/* New index block length */
#define SRCH_INVERT_NEW_INDEX_BLOCK_LENGTH(s)                   (UTL_MACROS_MAX((s) * 2, SRCH_INVERT_INITIAL_INDEX_BLOCK_LENGTH))


/* These index block definitions are used to build the index blocks */
#define SRCH_INVERT_INDEX_BLOCK_TERM_TYPE_SIZE                  (1)
#define SRCH_INVERT_INDEX_BLOCK_TERM_COUNT_SIZE                 (4)
#define SRCH_INVERT_INDEX_BLOCK_DOCUMENT_COUNT_SIZE             (4)
#define SRCH_INVERT_INDEX_BLOCK_TERM_SIZE                       (2)
#define SRCH_INVERT_INDEX_BLOCK_INCLUDE_IN_COUNTS_SIZE          (1)

#define SRCH_INVERT_INDEX_BLOCK_HEADER_LENGTH                   (SRCH_INVERT_INDEX_BLOCK_TERM_TYPE_SIZE + \
                                                                        SRCH_INVERT_INDEX_BLOCK_TERM_COUNT_SIZE + \
                                                                        SRCH_INVERT_INDEX_BLOCK_DOCUMENT_COUNT_SIZE + \
                                                                        SRCH_INVERT_INDEX_BLOCK_TERM_SIZE + \
                                                                        SRCH_INVERT_INDEX_BLOCK_INCLUDE_IN_COUNTS_SIZE)

/* How many files we can merge at once */
#define SRCH_INVERT_MERGE_WIDTH                                 (FOPEN_MAX - 15)


/*---------------------------------------------------------------------------*/


/*
** Structures
*/

/* Search inverted trie term structure */
struct srchInvertTrieTerm {

    unsigned char   *pucIndexBlock;                         /* Index block */
    unsigned char   *pucIndexBlockEndPtr;                   /* Current end pointer into the index block */
    unsigned int    uiIndexBlockCapacity;                   /* Capacity of the index block */

    unsigned int    uiTermType;                             /* Term type */
    unsigned int    uiTermCount;                            /* Number of occurrences of this term in this block */
    unsigned int    uiDocumentCount;                        /* Number of unique documents is this block */
    unsigned int    uiDocumentID;                           /* Last document ID in this block */

    boolean bIncludeInCounts;                               /* Include this term in the counts */

};


/* Search inverted index merge structure */
struct srchInvertIndexMerge {
    unsigned char   pucFilePath[UTL_FILE_PATH_MAX + 1];     /* File path */
    FILE            *pfFile;                                /* File handle */
    unsigned char   *pucTerm;                               /* Term */
    unsigned int    uiTermType;                             /* Term type */
    unsigned int    uiTermCount;                            /* Term count */
    unsigned int    uiDocumentCount;                        /* Document count */
    unsigned int    uiIndexBlockDataLength;                 /* Index block data length */
    boolean         bProcessing;                            /* Processing flag */
    boolean         bIncludeInCounts;                       /* Include this term in the counts */
};


/*---------------------------------------------------------------------------*/


/*
** Private function prototypes
*/

static int iSrchInvertTrieAddTermInit (struct srchIndex *psiSrchIndex);

static int iSrchInvertTrieAddStopTerms (struct srchIndex *psiSrchIndex,
        wchar_t **ppwcStopListTermList, unsigned int uiStopListTermListLength);

static int iSrchInvertTrieAddTerm (struct srchIndex *psiSrchIndex, unsigned int uiDocumentID, 
        unsigned char *pucTerm, unsigned int uiFieldID, unsigned int uiFieldType, unsigned int uiFieldOptions, 
        unsigned int uiTermPosition, boolean bIncludeInCounts);

static int iSrchInvertTrieAddTermFinish (struct srchIndex *psiSrchIndex);

static int iSrchInvertTrieAddTermFree (struct srchIndex *psiSrchIndex);


static int iSrchInvertTrieGetTrieTerm (struct srchIndex *psiSrchIndex, unsigned char *pucTerm, 
        struct srchInvertTrieTerm **ppsittSrchInvertTrieTerm);


static int iSrchInvertFlushIndexBlocks (struct srchIndex *psiSrchIndex);

static int iSrchInvertFlushIndexBlocksCallBack (unsigned char *pucKey, void *pvData, va_list ap);

static int iSrchInvertFreeIndexBlocksCallBack (unsigned char *pucKey, void *pvData, va_list ap);


static int iSrchInvertIndexBlockDictEntryWrite (FILE *pfFile, unsigned char *pucTerm, unsigned int uiTermType,
        unsigned int uiTermCount, unsigned int uiDocumentCount, boolean bIncludeInCounts);

static int iSrchInvertIndexBlockDictEntryRead (FILE *pfFile, unsigned char **ppucTerm, unsigned int *puiTermType,
        unsigned int *puiTermCount, unsigned int *puiDocumentCount, boolean *pbIncludeInCount);


static int iSrchInvertMerge (struct srchIndex *psiSrchIndex);

static int iSrchInvertMergeIndexFilesSetup (struct srchIndex *psiSrchIndex, unsigned int uiStartVersion, 
        unsigned int uiEndVersion, unsigned int uiSrchInvertIndexMergeLength, boolean bFinalMerge);

static int iSrchInvertMergeIndexFiles (struct srchIndex *psiSrchIndex,
        struct srchInvertIndexMerge *psiimSrchInvertIndexMerge, unsigned int uiSrchInvertIndexMergeLength, 
        FILE *pfOutputFile, boolean bFinalMerge);


static int iSrchInvertStoreTermInIndex (struct srchIndex *psiSrchIndex, unsigned char *pucTerm, 
        struct srchInvertIndexMerge *psiimSrchInvertIndexMerge, unsigned int uiSrchInvertIndexMergeLength, 
        void *pvData, unsigned int uiDataLength, FILE *pfOutputFile, boolean bFinalMerge);

static int iSrchInvertCompressIndexBlock (unsigned char *pucTerm, unsigned char *pucIndexBlock, unsigned int *puiIndexBlockLength, 
        unsigned char *pucFieldIDBitmap, unsigned int uiFieldIDBitmapLength);


static unsigned char *pucSrchInvertPrettyPrintFileNumbers (struct srchIndex *psiSrchIndex,
        unsigned int uiStartVersion, unsigned int uiEndVersion, unsigned char *pucBuffer,
        unsigned int uiBufferLength);


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchInvertInit()

    Purpose:    This function must be called before any calls to iSrchInvertAddTerm.  
                iSrchInvertFinish needs to be called before another
                iSrchInvertInit is called.

                This function basically sets up the index for adding new 
                terms. The index must have been opened with an intent
                to create it or update it.

    Parameters: psiSrchIndex                    search index structure
                pucLanguageCode                 language code
                pucTokenizerName                tokenizer name
                pucStemmerName                  stemmer name
                pucStopListName                 stop list name (optional)
                pucStopListFilePath             stop list file path (optional)
                uiIndexerMemorySizeMaximum      memory to use in megabytes
                uiTermLengthMinimum             min term length
                uiTermLengthMaximum             max term length
                pucTemporaryDirectoryPath       temporary directory path

    Globals:    none

    Returns:    SRCH error code

*/
int iSrchInvertInit
(
    struct srchIndex *psiSrchIndex,
    unsigned char *pucLanguageCode,
    unsigned char *pucTokenizerName,
    unsigned char *pucStemmerName,
    unsigned char *pucStopListName,
    unsigned char *pucStopListFilePath,
    unsigned int uiIndexerMemorySizeMaximum,
    unsigned int uiTermLengthMinimum,
    unsigned int uiTermLengthMaximum,
    unsigned char *pucTemporaryDirectoryPath
)
{

    int     iError = SRCH_NoError;


    /* Check the parameters */
    if ( psiSrchIndex == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'psiSrchIndex' parameter passed to 'iSrchInvertInit'."); 
        return (SRCH_InvalidIndex);
    }

    if ( bUtlStringsIsStringNULL(pucLanguageCode) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pucLanguageCode' parameter passed to 'iSrchInvertInit'."); 
        return (SRCH_InvertInvalidLanguageCode);
    }

    if ( bUtlStringsIsStringNULL(pucTokenizerName) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pucTokenizerName' parameter passed to 'iSrchInvertInit'."); 
        return (SRCH_InvertInvalidTokenizerName);
    }

    if ( bUtlStringsIsStringNULL(pucStemmerName) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pucStemmerName' parameter passed to 'iSrchInvertInit'."); 
        return (SRCH_InvertInvalidStemmerName);
    }

    if ( bUtlStringsIsStringNULL(pucStopListName) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pucStopListName' parameter passed to 'iSrchInvertInit'."); 
        return (SRCH_InvertInvalidStopListName);
    }

    if ( (uiIndexerMemorySizeMaximum < SRCH_INDEXER_MEMORY_MINIMUM) || (uiIndexerMemorySizeMaximum > SRCH_INDEXER_MEMORY_MAXIMUM) ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiIndexerMemorySizeMaximum' parameter passed to 'iSrchInvertInit'."); 
        return (SRCH_InvertInvalidMemorySizeMax);
    }

    if ( uiTermLengthMinimum < SRCH_TERM_LENGTH_MINIMUM ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiTermLengthMinimum' parameter passed to 'iSrchInvertInit'."); 
        return (SRCH_InvertInvalidTermLengthMin);
    }

    if ( uiTermLengthMaximum > SRCH_TERM_LENGTH_MAXIMUM ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiTermLengthMaximum' parameter passed to 'iSrchInvertInit'."); 
        return (SRCH_InvertInvalidTermLengthMax);
    }

    if ( uiTermLengthMinimum >= uiTermLengthMaximum ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiTermLengthMinimum' & 'uiTermLengthMaximum' parameters passed to 'iSrchInvertInit'."); 
        return (SRCH_InvertInvalidTermLengths);
    }


    ASSERT(psiSrchIndex->uiIntent == SRCH_INDEX_INTENT_CREATE);
    ASSERT(psiSrchIndex->psibSrchIndexBuild != NULL);


    /* Duplicate the temporary directory path if set */
    if ( bUtlStringsIsStringNULL(pucTemporaryDirectoryPath) == false ) {
        if ( (psiSrchIndex->psibSrchIndexBuild->pucTemporaryDirectoryPath = s_strdup(pucTemporaryDirectoryPath)) == NULL ) {
            return (SRCH_MemError);
        }
    }


    /* Initialize the language parser, this needs to happen before any other language intialization */
    if ( (iError = iSrchLanguageInit(psiSrchIndex, pucLanguageCode, pucTokenizerName)) != SRCH_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to initialize the language, index: '%s', srch error: %d.", 
                psiSrchIndex->pucIndexName, iError);
        return (iError);
    }

    /* Initialize the stemmer */
    if ( (iError = iSrchStemmerInit(psiSrchIndex, pucStemmerName)) != SRCH_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to initialize the stemmer, index: '%s', srch error: %d.", 
                psiSrchIndex->pucIndexName, iError);
        return (iError);
    }

    /* Initialize the stop list */
    if ( (iError = iSrchStopListInit(psiSrchIndex, pucStopListName, pucStopListFilePath)) != SRCH_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to initialize the stop list, index: '%s', srch error: %d.", 
                psiSrchIndex->pucIndexName, iError);
        return (iError);
    }

    /* Initialize the term lengths */
    if ( (iError = iSrchTermLengthInit(psiSrchIndex, uiTermLengthMaximum, uiTermLengthMinimum)) != SRCH_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to initialize the term length, index: '%s', srch error: %d.", 
                psiSrchIndex->pucIndexName, iError);
        return (iError);
    }


    /* Create the stemmer */
    if ( (iError = iLngStemmerCreateByID(psiSrchIndex->uiStemmerID, psiSrchIndex->uiLanguageID, &psiSrchIndex->psibSrchIndexBuild->pvLngStemmer)) != LNG_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to create a stemmer, lng error: %d.", iError);
        return (SRCH_InvertCreateStemmerFailed);
    }


    /* Set the ammount of memory to use in bytes */
    psiSrchIndex->psibSrchIndexBuild->uiIndexerMemorySizeMaximum =
            (uiIndexerMemorySizeMaximum >= SRCH_INDEXER_MEMORY_MINIMUM) ? uiIndexerMemorySizeMaximum : SRCH_INDEXER_MEMORY_MINIMUM;


    /* Initialize for adding terms */
    if ( (iError = iSrchInvertTrieAddTermInit(psiSrchIndex)) != SRCH_NoError ) {
        goto bailFromiSrchInvertInit;
    }



    /* Bail label */
    bailFromiSrchInvertInit:

    /* Handle the error */    
    if ( iError != SRCH_NoError ) {
        iSrchInvertAbort(psiSrchIndex);
    }


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchInvertSwitchLanguage()

    Purpose:    This function does all the work needed when switching languages.

    Parameters: psiSrchIndex        search index structure
                pucLanguageCode     language code

    Globals:    none

    Returns:    SRCH error code

*/
int iSrchInvertSwitchLanguage
(
    struct srchIndex *psiSrchIndex,
    unsigned char *pucLanguageCode
)
{

    int             iError = SRCH_NoError;
    unsigned int    uiLanguageID = 0;


    /* Check the parameters */
    if ( psiSrchIndex == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'psiSrchIndex' parameter passed to 'iSrchInvertSwitchLanguage'."); 
        return (SRCH_InvalidIndex);
    }

    if ( bUtlStringsIsStringNULL(pucLanguageCode) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pucLanguageCode' parameter passed to 'iSrchInvertSwitchLanguage'."); 
        return (SRCH_InvertInvalidLanguageCode);
    }


    ASSERT(psiSrchIndex->uiIntent == SRCH_INDEX_INTENT_CREATE);
    ASSERT(psiSrchIndex->psibSrchIndexBuild != NULL);

    
    /* Get the language ID */
    if ( (iError = iLngGetLanguageIDFromCode(pucLanguageCode, &uiLanguageID)) != LNG_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the language ID from the language code: '%s', lng error: %d.", pucLanguageCode, iError); 
        goto bailFromiSrchInvertSwitchLanguage;
    }


    /* Free the stemmer */
    iLngStemmerFree(psiSrchIndex->psibSrchIndexBuild->pvLngStemmer);
    psiSrchIndex->psibSrchIndexBuild->pvLngStemmer = NULL;


    /* Create the stemmer */
    if ( (iError = iLngStemmerCreateByID(psiSrchIndex->uiStemmerID, uiLanguageID, &psiSrchIndex->psibSrchIndexBuild->pvLngStemmer)) != LNG_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to create a stemmer, lng error: %d.", iError);
        goto bailFromiSrchInvertSwitchLanguage;
    }



    /* Bail label */
    bailFromiSrchInvertSwitchLanguage:

    /* Handle the error */    
    if ( iError != SRCH_NoError ) {
        iSrchInvertAbort(psiSrchIndex);
    }


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchInvertAddTerm()

    Purpose:    This function adds a term to the index.

                Note that if an error occurs here, the index resources are
                freed, so indexing has to stop.

    Parameters: psiSrchIndex        search index structure
                uiDocumentID        current document, this will never be 0 
                pucTerm             term to be indexed
                uiTermPosition      the position of the term in the document
                uiFieldID           the field ID of the term in the document
                uiFieldType         the field type for this field
                uiFieldOptions      the field options for this field

    Globals:    none

    Returns:    SRCH error code
*/
int iSrchInvertAddTerm
(
    struct srchIndex *psiSrchIndex,
    unsigned int uiDocumentID,
    unsigned char *pucTerm,
    unsigned int uiTermPosition,
    unsigned int uiFieldID,
    unsigned int uiFieldType,
    unsigned int uiFieldOptions
)
{

    int             iError = SRCH_NoError;
    wchar_t         pwcTerm[SRCH_TERM_LENGTH_MAXIMUM + 1] = {L'\0'};
    wchar_t         *pwcTermPtr = NULL;
    wchar_t         pwcTermLowerCase[SRCH_TERM_LENGTH_MAXIMUM + 1] = {L'\0'};
    wchar_t         *pwcTermLowerCasePtr = NULL;
    unsigned char   pucTrieTerm[SRCH_TERM_LENGTH_MAXIMUM + 1] = {'\0'};
    unsigned char   *pucTrieTermPtr = NULL;
    unsigned int    uiTermLength = 0;
    unsigned int    uiWideTermLength = 0;
    boolean         bUpperCaseTerm = false;
    boolean         bMixedCaseTerm = false;
    boolean         bLowerCaseTerm = false;


    /* Check the parameters */
    if ( psiSrchIndex == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'psiSrchIndex' parameter passed to 'iSrchInvertAddTerm'."); 
        return (SRCH_InvalidIndex);
    }

    if ( uiDocumentID <= 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiDocumentID' parameter passed to 'iSrchInvertAddTerm'."); 
        return (SRCH_InvertInvalidDocumentID);
    }

    if ( bUtlStringsIsStringNULL(pucTerm) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pucTerm' parameter passed to 'iSrchInvertAddTerm'."); 
        return (SRCH_InvertInvalidTerm);
    }

    if ( uiFieldID < 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiFieldID' parameter passed to 'iSrchInvertAddTerm'."); 
        return (SRCH_InvertInvalidFieldID);
    }

    if ( uiFieldType < 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiFieldType' parameter passed to 'iSrchInvertAddTerm'."); 
        return (SRCH_InvertInvalidFieldType);
    }

    if ( uiFieldOptions < 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiFieldOptions' parameter passed to 'iSrchInvertAddTerm'."); 
        return (SRCH_InvertInvalidFieldOptions);
    }

    if ( uiTermPosition < 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiTermPosition' parameter passed to 'iSrchInvertAddTerm'."); 
        return (SRCH_InvertInvalidTermPosition);
    }


    ASSERT(psiSrchIndex->uiIntent == SRCH_INDEX_INTENT_CREATE);
    ASSERT(psiSrchIndex->pvUtlDocumentTable != NULL);
    ASSERT(psiSrchIndex->psibSrchIndexBuild != NULL);
    ASSERT(psiSrchIndex->psibSrchIndexBuild->pvUtlTermTrie != NULL);

    ASSERT(psiSrchIndex->psibSrchIndexBuild->pvLngConverterUTF8ToWChar != NULL);
    ASSERT(psiSrchIndex->psibSrchIndexBuild->pvLngConverterWCharToUTF8 != NULL);


/*     iUtlLogInfo(UTL_LOG_CONTEXT, "pucTerm: '%s' uiDocumentID: %u, uiTermPosition: %u, uiFieldID: %u",  */
/*             pucTerm, uiDocumentID, uiTermPosition, uiFieldID); */

    /* Set the field ID bitmap length */
    if ( uiFieldID > psiSrchIndex->psibSrchIndexBuild->uiFieldIDBitmapLength ) {
        psiSrchIndex->psibSrchIndexBuild->uiFieldIDBitmapLength = uiFieldID;
    }


    /* Check to see if we have reached the memory threshold to use in a cycle, if we have
    ** we need to flush the index blocks to disk, then we need to free the term resources 
    ** and init add terms.
    **
    ** Note that we can only check the memory sizes if are in between documents
    */
    if ( uiDocumentID != psiSrchIndex->psibSrchIndexBuild->uiLastDocumentID ) {

        size_t  zMemorySize = 0;
        size_t  zTotalMemorySize = 0;

        /* Get the trie memory size */
        iUtlTriGetMemorySize(psiSrchIndex->psibSrchIndexBuild->pvUtlTermTrie, &zMemorySize);
        
        /* Convert the trie memory size to megabytes and add to the total memory size */
        zTotalMemorySize += (float)zMemorySize / (1024 * 1024);

        /* Convert the memory size to megabytes and add to the total memory size */
        zTotalMemorySize += (float)psiSrchIndex->psibSrchIndexBuild->zMemorySize / (1024 * 1024);
        
/*         iUtlLogDebug(UTL_LOG_CONTEXT, "zTotalMemorySize [%u], uiIndexerMemorySizeMaximum [%u]",  */
/*                  (unsigned int)zTotalMemorySize, psiSrchIndex->psibSrchIndexBuild->uiIndexerMemorySizeMaximum); */

        if ( zTotalMemorySize >= psiSrchIndex->psibSrchIndexBuild->uiIndexerMemorySizeMaximum ) {

            /* Flush the index blocks to disk */
            if ( (iError = iSrchInvertFlushIndexBlocks(psiSrchIndex)) != SRCH_NoError ) {
                goto bailFromiSrchInvertAddTerm;
            }

            /* Free the resources */
            if ( (iError = iSrchInvertTrieAddTermFree(psiSrchIndex)) != SRCH_NoError ) {
                goto bailFromiSrchInvertAddTerm;
            }

            /* And reallocate them */
            if ( (iError = iSrchInvertTrieAddTermInit(psiSrchIndex)) != SRCH_NoError ) {
                goto bailFromiSrchInvertAddTerm;
            }
        }

        /* Update the last document ID */
        psiSrchIndex->psibSrchIndexBuild->uiLastDocumentID = uiDocumentID;

        /* Increment the number of documents */
        psiSrchIndex->psibSrchIndexBuild->uiDocumentCount++;
    }



    /* Get the term length, this also provides the term length for the converter function */
    uiTermLength = s_strlen(pucTerm);
    
    /* Make sure that the term is no longer than the max, truncate the string on a wide character boundary */
    if ( uiTermLength > psiSrchIndex->uiTermLengthMaximum ) {
        
        /* Truncate the term */
        if ( (iError = iLngUnicodeTruncateUtf8String(pucTerm, psiSrchIndex->uiTermLengthMaximum)) != LNG_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to truncate a term along a utf-8 boundary, lng error: %d.", iError);
            iError = SRCH_InvertTermTruncationFailed;
            goto bailFromiSrchInvertAddTerm;
        }
        
        /* Ajust the term length */
        uiTermLength = s_strlen(pucTerm);
    }



    /* Convert the term from utf-8 to wide characters */
    pwcTermPtr = pwcTerm;
    uiWideTermLength = SRCH_TERM_LENGTH_MAXIMUM * sizeof(wchar_t);
    if ( (iError = iLngConverterConvertString(psiSrchIndex->psibSrchIndexBuild->pvLngConverterUTF8ToWChar, LNG_CONVERTER_RETURN_ON_ERROR, 
            pucTerm, uiTermLength, (unsigned char **)&pwcTermPtr, &uiWideTermLength)) != LNG_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to convert a term from utf-8 to wide characters, lng error: %d.", iError);
        iError = SRCH_InvertCharacterSetConvertionFailed;
        goto bailFromiSrchInvertAddTerm;
    }


    /* Get the term length, this also provides the term length for the stemming function */
    uiWideTermLength = s_wcslen(pwcTermPtr);

    /* Is the term long enough to add */
    if ( uiWideTermLength < psiSrchIndex->uiTermLengthMinimum ) {
        return (SRCH_NoError);
    }


    /* Stemming and case policies:
    ** 
    ** Upper case terms
    **  - no stemming
    **  - stored in original case and lower case
    **
    ** Mixed case terms
    **  - stemmed
    **  - stored in original case and lower case
    **
    ** Lower case terms
    **  - stemmed
    **  - stored in original case
    */

    /* Set the case flags */
    bUpperCaseTerm = bLngCaseIsWideStringAllUpperCase(pwcTerm);
    bMixedCaseTerm = bLngCaseDoesWideStringContainMixedCase(pwcTerm);
    bLowerCaseTerm = ((bUpperCaseTerm == false) && (bMixedCaseTerm == false)) ? true : false;


    /* Convert the term to lower case if it contains upper case and set the term pointer */
    if ( (bUpperCaseTerm == true) || (bMixedCaseTerm == true) ) {

        /* Copy the term */
        s_wcsnncpy(pwcTermLowerCase, pwcTerm, SRCH_TERM_LENGTH_MAXIMUM + 1);
        
        /* And convert the copy to lower case */
        pwcLngCaseConvertWideStringToLowerCase(pwcTermLowerCase);
        
        /* Set the term pointer, it now points to the lower case version of the term */
        pwcTermLowerCasePtr = pwcTermLowerCase;
    }
    else {
        /* Set the term pointer, it now points to the term which was in lower case from the start */
        pwcTermLowerCasePtr = pwcTerm;
    }


    /* Stem if stemming is on */
    if ( bSrchInfoFieldOptionStemming(uiFieldOptions) == true ) {

        /* Stem the original term if this is a mixed case term */
        if ( bMixedCaseTerm == true ) {
            if ( (iError = iLngStemmerStemTerm(psiSrchIndex->psibSrchIndexBuild->pvLngStemmer, pwcTerm, uiWideTermLength)) != LNG_NoError ) {
                iUtlLogError(UTL_LOG_CONTEXT, "Failed to stem a term, lng error: %d.", iError);
                iError = SRCH_InvertStemmingFailed;
                goto bailFromiSrchInvertAddTerm;
            }
        }

        /* Stem the lower case term if this is a mixed case or a lower case term */
        if ( (bMixedCaseTerm == true) || (bLowerCaseTerm == true) ) {
            if ( (iError = iLngStemmerStemTerm(psiSrchIndex->psibSrchIndexBuild->pvLngStemmer, pwcTermLowerCasePtr, uiWideTermLength)) != LNG_NoError ) {
                iUtlLogError(UTL_LOG_CONTEXT, "Failed to stem a term, lng error: %d.", iError);
                iError = SRCH_InvertStemmingFailed;
                goto bailFromiSrchInvertAddTerm;
            }
        }
    }


    /* Check that the lower case term did not get stemmed out of existence */
    if ( bUtlStringsIsWideStringNULL(pwcTermLowerCasePtr) == false ) {

        /* Convert the lower case term from wide characters to utf-8 */
        pucTrieTermPtr = pucTrieTerm;
        uiTermLength = SRCH_TERM_LENGTH_MAXIMUM;
        if ( (iError = iLngConverterConvertString(psiSrchIndex->psibSrchIndexBuild->pvLngConverterWCharToUTF8, LNG_CONVERTER_RETURN_ON_ERROR, 
                (unsigned char *)pwcTermLowerCasePtr, s_wcslen(pwcTermLowerCasePtr) * sizeof(wchar_t), &pucTrieTermPtr, &uiTermLength)) != LNG_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to convert a term from wide characters to utf-8, lng error: %d.", iError);
            iError = SRCH_InvertCharacterSetConvertionFailed;
            goto bailFromiSrchInvertAddTerm;
        }

        /* Add the lower case term to the trie - include in the counts */
        if ( (iError = iSrchInvertTrieAddTerm(psiSrchIndex, uiDocumentID, pucTrieTermPtr, uiFieldID, uiFieldType, uiFieldOptions, uiTermPosition, true)) != SRCH_NoError ) { 
            goto bailFromiSrchInvertAddTerm;
        }
    }


    /* Add the original term to the trie if it is an upper case term or a mixed case term */
    if ( (bUpperCaseTerm == true) || (bMixedCaseTerm == true) ) {
        
        /* Check that the original term did not get stemmed out of existence */
        if ( bUtlStringsIsWideStringNULL(pwcTerm) == false ) {

            /* Convert the term from wide characters to utf-8 */
            pucTrieTermPtr = pucTrieTerm;
            uiTermLength = SRCH_TERM_LENGTH_MAXIMUM;
            if ( iLngConverterConvertString(psiSrchIndex->psibSrchIndexBuild->pvLngConverterWCharToUTF8, LNG_CONVERTER_RETURN_ON_ERROR, 
                    (unsigned char *)pwcTerm, s_wcslen(pwcTerm) * sizeof(wchar_t), &pucTrieTermPtr, &uiTermLength) != LNG_NoError ) {
                iUtlLogError(UTL_LOG_CONTEXT, "Failed to convert a term from wide characters to utf-8, lng error: %d.", iError);
                iError = SRCH_InvertCharacterSetConvertionFailed;
                goto bailFromiSrchInvertAddTerm;
            }
    
            /* Add the term to the trie - exclude from the counts */
            if ( (iError = iSrchInvertTrieAddTerm(psiSrchIndex, uiDocumentID, pucTrieTermPtr, uiFieldID, uiFieldType, uiFieldOptions, uiTermPosition, false)) != SRCH_NoError ) { 
                goto bailFromiSrchInvertAddTerm;
            }
        }
    }



    /* Bail label */
    bailFromiSrchInvertAddTerm:

    /* Handle the error */    
    if ( iError != SRCH_NoError ) {
        iSrchInvertAbort(psiSrchIndex);
    }


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchInvertFinish()

    Purpose:    This function will be called when there are no more terms to 
                add to this index.

    Parameters: psiSrchIndex    search index structure

    Globals:    none

    Returns:    SRCH error code

*/
int iSrchInvertFinish
(
    struct srchIndex *psiSrchIndex
)
{

    int             iError = SRCH_NoError;
    unsigned char   pucUniqueTermCount[UTL_FILE_PATH_MAX + 1] = {'\0'};
    unsigned char   pucTotalTermCount[UTL_FILE_PATH_MAX + 1] = {'\0'};
    unsigned char   pucUniqueStopTermCount[UTL_FILE_PATH_MAX + 1] = {'\0'};
    unsigned char   pucTotalStopTermCount[UTL_FILE_PATH_MAX + 1] = {'\0'};
    unsigned char   pucMaxDocumentTermCount[UTL_FILE_PATH_MAX + 1] = {'\0'};
    unsigned char   pucMinDocumentTermCount[UTL_FILE_PATH_MAX + 1] = {'\0'};
    unsigned char   pucDocumentEntryCount[UTL_FILE_PATH_MAX + 1] = {'\0'};
    unsigned char   pucNumberString[UTL_FILE_PATH_MAX + 1] = {'\0'};


    /* Check the parameters */
    if ( psiSrchIndex == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'psiSrchIndex' parameter passed to 'iSrchInvertFinish'."); 
        return (SRCH_InvalidIndex);
    }


    ASSERT(psiSrchIndex->uiIntent == SRCH_INDEX_INTENT_CREATE);


    /* Finish adding terms */
    if ( (iError = iSrchInvertTrieAddTermFinish(psiSrchIndex)) != SRCH_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to finish adding the terms, srch error: %d.", iError);
        goto bailFromiSrchInvertFinish;
    }


    /* Generate the document key dictionary */
    if ( (iError = iSrchKeyDictGenerate(psiSrchIndex)) != SRCH_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to generate the document key dictionary, srch error: %d.", iError);
        goto bailFromiSrchInvertFinish;
    }


    /* Print up some stats */
    s_strnncpy(pucUniqueTermCount, pucUtlStringsFormatUnsignedNumber(psiSrchIndex->ulUniqueTermCount, pucNumberString, UTL_FILE_PATH_MAX + 1), UTL_FILE_PATH_MAX + 1);
    s_strnncpy(pucTotalTermCount, pucUtlStringsFormatUnsignedNumber(psiSrchIndex->ulTotalTermCount, pucNumberString, UTL_FILE_PATH_MAX + 1), UTL_FILE_PATH_MAX + 1);
    s_strnncpy(pucUniqueStopTermCount, pucUtlStringsFormatUnsignedNumber(psiSrchIndex->ulUniqueStopTermCount, pucNumberString, UTL_FILE_PATH_MAX + 1), UTL_FILE_PATH_MAX + 1);
    s_strnncpy(pucTotalStopTermCount, pucUtlStringsFormatUnsignedNumber(psiSrchIndex->ulTotalStopTermCount, pucNumberString, UTL_FILE_PATH_MAX + 1), UTL_FILE_PATH_MAX + 1);
    iUtlLogInfo(UTL_LOG_CONTEXT, "Terms, unique: %s, total: %s. Stop terms, unique: %s, total: %s.", pucUniqueTermCount, pucTotalTermCount, pucUniqueStopTermCount, pucTotalStopTermCount);



    s_strnncpy(pucMaxDocumentTermCount, pucUtlStringsFormatUnsignedNumber(psiSrchIndex->uiDocumentTermCountMaximum, pucNumberString, UTL_FILE_PATH_MAX + 1), UTL_FILE_PATH_MAX + 1);
    s_strnncpy(pucMinDocumentTermCount, pucUtlStringsFormatUnsignedNumber(psiSrchIndex->uiDocumentTermCountMinimum, pucNumberString, UTL_FILE_PATH_MAX + 1), UTL_FILE_PATH_MAX + 1);
    iUtlLogInfo(UTL_LOG_CONTEXT, "Document term counts, maximum: %s, minimum: %s.", pucMaxDocumentTermCount, pucMinDocumentTermCount);

    s_strnncpy(pucDocumentEntryCount, pucUtlStringsFormatUnsignedNumber(psiSrchIndex->uiDocumentCount, pucNumberString, UTL_FILE_PATH_MAX + 1), UTL_FILE_PATH_MAX + 1);
    iUtlLogInfo(UTL_LOG_CONTEXT, "Document count: %s.", pucDocumentEntryCount);



    /* Bail label */
    bailFromiSrchInvertFinish:

    /* Handle the error */    
    if ( iError != SRCH_NoError ) {
        iSrchInvertAbort(psiSrchIndex);
    }


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchInvertAbort()

    Purpose:    This function will be called when there are no more terms to 
                add to this index.

    Parameters: psiSrchIndex    search index structure

    Globals:    none

    Returns:    SRCH error code

*/
int iSrchInvertAbort
(
    struct srchIndex *psiSrchIndex
)
{

    /* Check the parameters */
    if ( psiSrchIndex == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'psiSrchIndex' parameter passed to 'iSrchInvertFinish'."); 
        return (SRCH_InvalidIndex);
    }


    ASSERT(psiSrchIndex->uiIntent == SRCH_INDEX_INTENT_CREATE);


    iUtlLogError(UTL_LOG_CONTEXT, "Inverting aborted, index: '%s'.", psiSrchIndex->pucIndexName);


    /* Free the resources */
    iSrchInvertTrieAddTermFree(psiSrchIndex);


    return (SRCH_NoError);

}


/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchInvertTrieAddTermInit()

    Purpose:    This function sets up the trie for adding terms

    Parameters: psiSrchIndex    search index structure

    Globals:    none

    Returns:    SRCH error code

*/
static int iSrchInvertTrieAddTermInit
(
    struct srchIndex *psiSrchIndex
)
{

    int             iError = SRCH_NoError;
    wchar_t         **ppwcStopListTermList = NULL;
    unsigned int    uiStopListTermListLength = 0;


    ASSERT(psiSrchIndex != NULL);

    ASSERT(psiSrchIndex->psibSrchIndexBuild != NULL);
    ASSERT(psiSrchIndex->psibSrchIndexBuild->pvUtlTermTrie == NULL);


    /* Initialize the memory size */
    psiSrchIndex->psibSrchIndexBuild->zMemorySize = 0;

    /* Initialize the counts */
    psiSrchIndex->psibSrchIndexBuild->uiUniqueTermCount = 0;
    psiSrchIndex->psibSrchIndexBuild->uiTotalTermCount = 0;
    psiSrchIndex->psibSrchIndexBuild->uiUniqueStopTermCount = 0;
    psiSrchIndex->psibSrchIndexBuild->uiTotalStopTermCount = 0;
    psiSrchIndex->psibSrchIndexBuild->uiDocumentCount = 0;


    /* Create a new trie */
    if ( (iError = iUtlTrieCreate(&psiSrchIndex->psibSrchIndexBuild->pvUtlTermTrie)) != UTL_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to create a trie for the terms, utl error: %d.", iError);
        return (SRCH_InvertAddTermInitFailed); 
    }


    /* Get the stop list term list */
    if ( (iError = iLngStopListGetTermList(psiSrchIndex->psibSrchIndexBuild->pvLngStopList, &ppwcStopListTermList, &uiStopListTermListLength)) != LNG_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the stop list term list, lng error: %d.", iError);
        return (SRCH_InvertAddTermInitFailed); 
    }


    /* Add the stop terms to the trie, is there was a stop term list returned */
    if ( (ppwcStopListTermList != NULL) && (uiStopListTermListLength > 0) ) {
        if ( (iError = iSrchInvertTrieAddStopTerms(psiSrchIndex, ppwcStopListTermList, uiStopListTermListLength)) != SRCH_NoError ) {
            return (SRCH_InvertAddTermInitFailed); 
        }
    }


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchInvertTrieAddStopTerms()

    Purpose:    Add the stop terms to the trie.

    Parameters: psiSrchIndex                search index structure
                ppwcStopListTermList        stop term list
                uiStopListTermListLength    stop term list length

    Globals:    none

    Returns:    SRCH error code

*/
static int iSrchInvertTrieAddStopTerms
(
    struct srchIndex *psiSrchIndex,
    wchar_t **ppwcStopListTermList,
    unsigned int uiStopListTermListLength
)
{

    int                         iError = SRCH_NoError;
    unsigned int                uiMiddle = 0;
    unsigned char               pucStopTerm[SRCH_TERM_LENGTH_MAXIMUM + 1] = {'\0'};
    unsigned char               *pucStopTermPtr = NULL;
    wchar_t                     pwcStopTerm[SRCH_TERM_LENGTH_MAXIMUM + 1] = {L'\0'};
    unsigned int                uiTermLength = SRCH_TERM_LENGTH_MAXIMUM + 1;
    struct srchInvertTrieTerm   *psittSrchInvertTrieTerm = NULL;


     ASSERT(psiSrchIndex != NULL);
     ASSERT(ppwcStopListTermList != NULL);
     ASSERT(uiStopListTermListLength >= 0);

    ASSERT(psiSrchIndex->psibSrchIndexBuild != NULL);
    ASSERT(psiSrchIndex->psibSrchIndexBuild->pvLngConverterWCharToUTF8 != NULL);
    ASSERT(psiSrchIndex->psibSrchIndexBuild->pvLngStemmer != NULL);


    /* Bail if we have hit the end of the list */
    if ( uiStopListTermListLength <= 0 ) {
        return (SRCH_NoError);
    }

    
    /* Add the term in the middle of the list */
    uiMiddle = uiStopListTermListLength / 2;


    /* Convert the stop term from wide characters to utf-8 */
    pucStopTermPtr = pucStopTerm;
    uiTermLength = SRCH_TERM_LENGTH_MAXIMUM;
    if ( (iError = iLngConverterConvertString(psiSrchIndex->psibSrchIndexBuild->pvLngConverterWCharToUTF8, LNG_CONVERTER_RETURN_ON_ERROR, 
            (unsigned char *)ppwcStopListTermList[uiMiddle], s_wcslen(ppwcStopListTermList[uiMiddle]) * sizeof(wchar_t), &pucStopTermPtr, &uiTermLength)) != LNG_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to convert a stop term from wide characters to utf-8, lng error: %d.", iError);
        return (SRCH_InvertCharacterSetConvertionFailed);
    }


    /* Add the stop term */
    if ( (iError = iSrchInvertTrieGetTrieTerm(psiSrchIndex, pucStopTerm, &psittSrchInvertTrieTerm)) != SRCH_NoError ) {
        return (iError);
    }


    /* Set the stop term flag if it is not already set and increment the number of stop terms, 
    ** if the stop term flag is already set, then this stop term has already been added so there
    ** is no need to set it or to increment the number of stop terms
    */
    if ( psittSrchInvertTrieTerm->uiTermType != SPI_TERM_TYPE_STOP ) {
    
        /* Set the stop term flag */
        psittSrchInvertTrieTerm->uiTermType = SPI_TERM_TYPE_STOP;

        /* And increment the number of stop terms */
        psiSrchIndex->psibSrchIndexBuild->uiUniqueStopTermCount++;
    }


    /* Make a copy of the stop term and get its length */
    s_wcscpy(pwcStopTerm, ppwcStopListTermList[uiMiddle]);
    uiTermLength = s_wcslen(pwcStopTerm);


    /* Stem the term */
    if ( (iError = iLngStemmerStemTerm(psiSrchIndex->psibSrchIndexBuild->pvLngStemmer, pwcStopTerm, uiTermLength)) != UTL_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to stem a stop term, lng error: %d.", iError);
        return (SRCH_InvertStemmingFailed);
    }

    
    /* Add the stemmed stop term if it was not stemmed out of existance and if it was actually modified */
    if ( (bUtlStringsIsStringNULL(pwcStopTerm) == false) && (s_wcscmp(pwcStopTerm, ppwcStopListTermList[uiMiddle]) != 0) ) {
        
        /* Convert the stop term from wide characters to utf-8 */
        pucStopTermPtr = pucStopTerm;
        uiTermLength = SRCH_TERM_LENGTH_MAXIMUM;
        if ( (iError = iLngConverterConvertString(psiSrchIndex->psibSrchIndexBuild->pvLngConverterWCharToUTF8, LNG_CONVERTER_RETURN_ON_ERROR, 
                (unsigned char *)pwcStopTerm, s_wcslen(pwcStopTerm) * sizeof(wchar_t), &pucStopTermPtr, &uiTermLength)) != LNG_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to convert a stop term from wide characters to utf-8, lng error: %d.", iError);
            return (SRCH_InvertCharacterSetConvertionFailed);
        }
    
        /* Add the stemmed stop term */
        if ( (iError = iSrchInvertTrieGetTrieTerm(psiSrchIndex, pucStopTerm, &psittSrchInvertTrieTerm)) == SPI_NoError ) {
            return (iError);
        }

        /* Set the stop term flag if it is not already set and increment the number of stop terms, 
        ** if the stop term flag is already set, then this stop term has already been added so there
        ** is no need to set it or to increment the number of stop terms
        */
        if ( psittSrchInvertTrieTerm->uiTermType != SPI_TERM_TYPE_STOP ) {
        
            /* Set the stop term flag */
            psittSrchInvertTrieTerm->uiTermType = SPI_TERM_TYPE_STOP;

            /* And increment the number of stop terms */
            psiSrchIndex->psibSrchIndexBuild->uiUniqueStopTermCount++;
        }
    }


    /* Do the low end of the list */
    if ( (iError = iSrchInvertTrieAddStopTerms(psiSrchIndex, ppwcStopListTermList, uiMiddle)) != SRCH_NoError ) {
        return (iError);
    }


    /* Do the high end of the list */
    if ( (iError = iSrchInvertTrieAddStopTerms(psiSrchIndex, ppwcStopListTermList + uiMiddle + 1, uiStopListTermListLength - uiMiddle - 1)) != SRCH_NoError ) {
        return (iError);
    }


    return (SRCH_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchInvertTrieAddTerm()

    Purpose:    This function adds a term to the trie

    Parameters: psiSrchIndex        search index structure
                uiDocumentID        current document, this will never be 0 
                pucTerm             term to be indexed
                uiFieldID           field ID of the term in the document
                uiFieldType         field type for this field
                uiFieldOptions      field options for this field
                uiTermPosition      position of the term in the document
                bIncludeInCounts    include the term in the counts

    Globals:    none

    Returns:    SRCH error code

*/
static int iSrchInvertTrieAddTerm
(
    struct srchIndex *psiSrchIndex,
    unsigned int uiDocumentID,
    unsigned char *pucTerm,
    unsigned int uiFieldID,
    unsigned int uiFieldType,
    unsigned int uiFieldOptions,
    unsigned int uiTermPosition,
    boolean bIncludeInCounts
)
{

    int                         iError = SRCH_NoError;
    unsigned int                uiByteCount = 0;
    unsigned int                uiEntryByteCount = 0;
    struct srchInvertTrieTerm   *psittSrchInvertTrieTerm = NULL;


    ASSERT(psiSrchIndex != NULL);
    ASSERT(uiDocumentID > 0);
    ASSERT(bUtlStringsIsStringNULL(pucTerm) == false);
    ASSERT(uiFieldID >= 0);
    ASSERT(uiFieldOptions >= 0);
    ASSERT(uiTermPosition >= 0);
    ASSERT((bIncludeInCounts == true) || (bIncludeInCounts == false));


    ASSERT(psiSrchIndex->psibSrchIndexBuild != NULL);

/*     iUtlLogDebug(UTL_LOG_CONTEXT, "pucTerm: '%s' uiDocumentID: %u, uiTermPosition: %u, uiFieldID: %u", pucTerm, uiDocumentID, uiTermPosition, uiFieldID); */

    /* Add the term to the trie */
    if ( (iError = iSrchInvertTrieGetTrieTerm(psiSrchIndex, pucTerm, &psittSrchInvertTrieTerm)) != SRCH_NoError ) {
        return (iError);
    }

    /* If the term type is unknown then we set it to a regular term */
    if ( psittSrchInvertTrieTerm->uiTermType == SPI_TERM_TYPE_UNKNOWN ) {
        psittSrchInvertTrieTerm->uiTermType = SPI_TERM_TYPE_REGULAR;
    }


    /* Set the count inclusion flag */
    psittSrchInvertTrieTerm->bIncludeInCounts = bIncludeInCounts;


    /* Increment various counters */
    if ( psittSrchInvertTrieTerm->bIncludeInCounts == true ) {
            
        /* Regular term */
        if ( psittSrchInvertTrieTerm->uiTermType == SPI_TERM_TYPE_REGULAR ) {
        
            /* Increment the unique number of terms added if this is the first one, this is
            ** detected by the fact that no memory has been allocated for this term as yet,
            ** something we do below
            */
            if ( psittSrchInvertTrieTerm->pucIndexBlock == NULL ) {
                psiSrchIndex->psibSrchIndexBuild->uiUniqueTermCount++;
            }
    
            /* Increment the total number of terms added within this cycle */
            psiSrchIndex->psibSrchIndexBuild->uiTotalTermCount++;
        }

        /* Stop term */
        else if ( psittSrchInvertTrieTerm->uiTermType == SPI_TERM_TYPE_STOP ) {
        
            /* Increment the unique number of stop terms added if this is the first one, this is
            ** detected by the fact that no memory has been allocated for this term as yet,
            ** something we do below
            */
            if ( psittSrchInvertTrieTerm->pucIndexBlock == NULL ) {
                psiSrchIndex->psibSrchIndexBuild->uiUniqueStopTermCount++;
            }
    
            /* Increment the total number of stop terms added within this cycle */
            psiSrchIndex->psibSrchIndexBuild->uiTotalStopTermCount++;
        }
        
        /* Ouch! */
        else {
            ASSERT(false);
        }
    }


    /* Increment the occurrence counter for this term for this cycle */
    psittSrchInvertTrieTerm->uiTermCount++;


    /* If this is this a new document, we need to update some variables */
    if ( uiDocumentID != psittSrchInvertTrieTerm->uiDocumentID ) {

        /* Increment the number of documents */
        psittSrchInvertTrieTerm->uiDocumentCount++;

        /* Update the document ID */
        psittSrchInvertTrieTerm->uiDocumentID = uiDocumentID;
    }


    /* Return if this is a stop term and stop terms are enabled on this field */
    if ( (psittSrchInvertTrieTerm->uiTermType == SPI_TERM_TYPE_STOP) && (bSrchInfoFieldOptionStopTerm(uiFieldOptions) == true) ) {
        return (SRCH_NoError);
    }


    ASSERT(uiDocumentID > 0);
    ASSERT(uiTermPosition >= 0);
    ASSERT(uiFieldID >= 0);

    
/* Compressed int */
    /* Work out the length of the index entry */
    UTL_NUM_GET_COMPRESSED_UINT_SIZE(uiDocumentID, uiByteCount);
    uiEntryByteCount += uiByteCount;

    UTL_NUM_GET_COMPRESSED_UINT_SIZE(uiTermPosition, uiByteCount);
    uiEntryByteCount += uiByteCount;

    UTL_NUM_GET_COMPRESSED_UINT_SIZE(uiFieldID, uiByteCount);
    uiEntryByteCount += uiByteCount;

/*     UTL_NUM_GET_COMPRESSED_UINT_SIZE(uiTermWeight, uiByteCount); */
/*     uiEntryByteCount += uiByteCount; */


/* Varint */
    /* Work out the length of the index entry */
/*     UTL_NUM_GET_VARINT_TRIO_SIZE(uiDocumentID, uiTermPosition, uiFieldID, uiEntryByteCount); */
/*     UTL_NUM_GET_VARINT_QUAD_SIZE(uiDocumentID, uiTermPosition, uiFieldID, uiTermWeight, uiEntryByteCount); */


    /* Check to see if we need more memory */
    if ( (psittSrchInvertTrieTerm->pucIndexBlock == NULL) ||
            ((psittSrchInvertTrieTerm->uiIndexBlockCapacity - (psittSrchInvertTrieTerm->pucIndexBlockEndPtr - psittSrchInvertTrieTerm->pucIndexBlock) < uiEntryByteCount)) ) {

        unsigned char    *pucNewIndexBlockPtr = NULL;
        unsigned int    uiNewIndexBlockCapacity = 0;
        
        /* Work out the new index block capacity */
        if ( psittSrchInvertTrieTerm->uiIndexBlockCapacity == 0 ) {
            uiNewIndexBlockCapacity = UTL_MACROS_MAX(uiEntryByteCount, SRCH_INVERT_INITIAL_INDEX_BLOCK_LENGTH);
        }
        else {
            uiNewIndexBlockCapacity = SRCH_INVERT_NEW_INDEX_BLOCK_LENGTH(psittSrchInvertTrieTerm->uiIndexBlockCapacity + uiEntryByteCount);
        }

        /* Reallocate the index block */
        if ( (pucNewIndexBlockPtr = (unsigned char *)s_realloc((void *)psittSrchInvertTrieTerm->pucIndexBlock, (size_t)uiNewIndexBlockCapacity)) == NULL ) {
            return (SRCH_MemError);
        }

        /* Increment the memory size, we want the delta */
        psiSrchIndex->psibSrchIndexBuild->zMemorySize += uiNewIndexBlockCapacity - psittSrchInvertTrieTerm->uiIndexBlockCapacity;


        /* Set the new current pointer into the index block before we hand over the new index block */
        psittSrchInvertTrieTerm->pucIndexBlockEndPtr = pucNewIndexBlockPtr + (psittSrchInvertTrieTerm->pucIndexBlockEndPtr - psittSrchInvertTrieTerm->pucIndexBlock);

        /* Hand over the new index block */
        psittSrchInvertTrieTerm->pucIndexBlock = pucNewIndexBlockPtr;

        /* Set the index block capacity */
        psittSrchInvertTrieTerm->uiIndexBlockCapacity = uiNewIndexBlockCapacity;
    }


/* Compressed int */
    /* Add the index entry */
    UTL_NUM_WRITE_COMPRESSED_UINT(uiDocumentID, psittSrchInvertTrieTerm->pucIndexBlockEndPtr);
    UTL_NUM_WRITE_COMPRESSED_UINT(uiTermPosition, psittSrchInvertTrieTerm->pucIndexBlockEndPtr);
    UTL_NUM_WRITE_COMPRESSED_UINT(uiFieldID, psittSrchInvertTrieTerm->pucIndexBlockEndPtr);
/*     UTL_NUM_WRITE_COMPRESSED_UINT(uiTermWeight, psittSrchInvertTrieTerm->pucIndexBlockEndPtr); */


/* Varint int */
    /* Add the index entry */
/*     UTL_NUM_WRITE_VARINT_TRIO(uiDocumentID, uiTermPosition, uiFieldID, psittSrchInvertTrieTerm->pucIndexBlockEndPtr); */
/*     UTL_NUM_WRITE_VARINT_QUAD(uiDocumentID, uiTermPosition, uiFieldID, uiTermWeight, psittSrchInvertTrieTerm->pucIndexBlockEndPtr); */


    ASSERT(psittSrchInvertTrieTerm->pucIndexBlockEndPtr <= (psittSrchInvertTrieTerm->pucIndexBlock + psittSrchInvertTrieTerm->uiIndexBlockCapacity));


    return (SRCH_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchInvertTrieAddTermFinish()

    Purpose:    This function will be called when there are no more terms to 
                add to this index.

    Parameters: psiSrchIndex    search index structure

    Globals:    none

    Returns:    SRCH error code

*/
static int iSrchInvertTrieAddTermFinish
(
    struct srchIndex *psiSrchIndex
)
{

    int     iError = SRCH_NoError;


    ASSERT(psiSrchIndex != NULL);
    ASSERT(psiSrchIndex->psibSrchIndexBuild != NULL);


    /* Flush the current set of index blocks to the disk */
    if ( (iError = iSrchInvertFlushIndexBlocks(psiSrchIndex)) != SRCH_NoError ) {
        return (iError);
    }


    /* Free the resources */
    if ( (iError = iSrchInvertTrieAddTermFree(psiSrchIndex)) != SRCH_NoError ) {
        return (iError);
    }


    /* This is the final flush, we proceed to merge all the index blocks files together */
    if ( (iError = iSrchInvertMerge(psiSrchIndex)) != SRCH_NoError ) {
        return (iError);
    }


    return (SRCH_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchInvertTrieAddTermFree()

    Purpose:    This function will be called when we are all finished
                with adding terms, it can also be called to free all
                the resources held if an error occured when indexing 

    Parameters: psiSrchIndex    search index structure

    Globals:    none

    Returns:    SRCH error code

*/
static int iSrchInvertTrieAddTermFree
(
    struct srchIndex *psiSrchIndex
)
{

    int     iError = SRCH_NoError;


    ASSERT(psiSrchIndex != NULL);

    ASSERT(psiSrchIndex->psibSrchIndexBuild != NULL);


    /* Free the trie entries and the trie */
    if ( psiSrchIndex->psibSrchIndexBuild->pvUtlTermTrie != NULL ) {

        /* Loop over the trie entries and free the index blocks */
        if ( (iError = iUtlTrieLoop(psiSrchIndex->psibSrchIndexBuild->pvUtlTermTrie, NULL, 
                (int (*)())iSrchInvertFreeIndexBlocksCallBack, psiSrchIndex)) != UTL_NoError ) {
    
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to loop over the entries in the terms trie, utl error: %d.", iError);
            return (SRCH_InvertIndexBlockFreeFailed);
        }
    
    
        /* Free the trie, note that we free the datum since 
        ** it was allocated using malloc()
        */
        iUtlTrieFree(psiSrchIndex->psibSrchIndexBuild->pvUtlTermTrie, true);
        psiSrchIndex->psibSrchIndexBuild->pvUtlTermTrie = NULL;
    }
    

    /* Reset the memory size */
    psiSrchIndex->psibSrchIndexBuild->zMemorySize = 0;


    return (SRCH_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchInvertTrieGetTrieTerm()

    Purpose:    This function will add the term in the index trie and 
                allocate a trie term structure if needed.

    Parameters: psiSrchIndex                search index structure
                pucTerm                     term to add
                ppsittSrchInvertTrieTerm    return pointer for the trie term structure
    
    Globals:    none

    Returns:    pointer to the trie term entry, NULL on error

*/
static int iSrchInvertTrieGetTrieTerm
(
    struct srchIndex *psiSrchIndex,
    unsigned char *pucTerm,
    struct srchInvertTrieTerm **ppsittSrchInvertTrieTerm
)
{

    int                         iError = UTL_NoError;
    void                        **ppvTrieTerm = NULL;
    struct srchInvertTrieTerm   *psittSrchInvertTrieTerm = NULL;


    ASSERT(psiSrchIndex != NULL);
    ASSERT(bUtlStringsIsStringNULL(pucTerm) == false);
    ASSERT(ppsittSrchInvertTrieTerm != NULL);

    ASSERT(psiSrchIndex->psibSrchIndexBuild != NULL);
    ASSERT(psiSrchIndex->psibSrchIndexBuild->pvUtlTermTrie != NULL);

    /* Look up/store the term - skip it if it cant be looked-up/stored */
    if ( (iError = iUtlTrieAdd(psiSrchIndex->psibSrchIndexBuild->pvUtlTermTrie, pucTerm, &ppvTrieTerm)) != UTL_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to add an entry to the terms trie, term: '%s', utl error: %d.", pucTerm, iError);
        return (SRCH_InvertAddTermFailed);
     }


     /* Dereference the pointer to the trie term structure, allocating it if it not allocated */
     if ( (psittSrchInvertTrieTerm = (struct srchInvertTrieTerm *)*ppvTrieTerm) == NULL ) {

        /* Allocate space for the trie term structure */
        if ( (*ppvTrieTerm = (void *)s_malloc(sizeof(struct srchInvertTrieTerm))) == NULL ) {
            return (SRCH_MemError);
        }

        /* Increment the memory size */
        psiSrchIndex->psibSrchIndexBuild->zMemorySize += sizeof(struct srchInvertTrieTerm);


        /* Dereference the trie term */
        psittSrchInvertTrieTerm = (struct srchInvertTrieTerm *)*ppvTrieTerm;

        /* Initialize the trie term fields */
        psittSrchInvertTrieTerm->uiTermType = SPI_TERM_TYPE_UNKNOWN;
        psittSrchInvertTrieTerm->uiTermCount = 0;
        psittSrchInvertTrieTerm->uiDocumentCount = 0;
        psittSrchInvertTrieTerm->uiDocumentID = 0;
        psittSrchInvertTrieTerm->pucIndexBlock = NULL;
        psittSrchInvertTrieTerm->pucIndexBlockEndPtr = NULL;
        psittSrchInvertTrieTerm->uiIndexBlockCapacity = 0;
    }
    
    
    /* Set the return pointer */
    *ppsittSrchInvertTrieTerm = psittSrchInvertTrieTerm;


    return (SRCH_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchInvertFlushIndexBlocks()

    Purpose:    This function flush all the current index blocks to disk.

    Parameters: psiSrchIndex    search index structure

    Globals:    none

    Returns:    SRCH error code

*/
static int iSrchInvertFlushIndexBlocks
(
    struct srchIndex *psiSrchIndex
)
{

    int             iError = SRCH_NoError;
    unsigned char   pucFilePath[UTL_FILE_PATH_MAX + 1] = {'\0'};
    FILE            *pfFile = NULL;
    boolean         bFlushStopTerm = false;
    unsigned char   pucUniqueTermCount[UTL_FILE_PATH_MAX + 1] = {'\0'};
    unsigned char   pucTotalTermCount[UTL_FILE_PATH_MAX + 1] = {'\0'};
    unsigned char   pucUniqueStopTermCount[UTL_FILE_PATH_MAX + 1] = {'\0'};
    unsigned char   pucTotalStopTermCount[UTL_FILE_PATH_MAX + 1] = {'\0'};
    unsigned char   pucDocumentCount[UTL_FILE_PATH_MAX + 1] = {'\0'};
    unsigned char   pucNumberString[UTL_FILE_PATH_MAX + 1] = {'\0'};


    ASSERT(psiSrchIndex != NULL);

    ASSERT(psiSrchIndex->psibSrchIndexBuild != NULL);
    ASSERT(psiSrchIndex->psibSrchIndexBuild->pvUtlTermTrie != NULL);


    /* Inform the user what is going on
    **
    ** Note that we flush the stop terms into the first index block file (and only if we 
    ** are creating the index) because we dont need to flush them into each file
    */
    if ( (psiSrchIndex->psibSrchIndexBuild->uiIndexFileNumber == 0) && (psiSrchIndex->uiIntent == SRCH_INDEX_INTENT_CREATE) ) {
        s_strnncpy(pucUniqueTermCount, pucUtlStringsFormatUnsignedNumber(psiSrchIndex->psibSrchIndexBuild->uiUniqueTermCount, pucNumberString, UTL_FILE_PATH_MAX + 1), UTL_FILE_PATH_MAX + 1);
        s_strnncpy(pucTotalTermCount, pucUtlStringsFormatUnsignedNumber(psiSrchIndex->psibSrchIndexBuild->uiTotalTermCount, pucNumberString, UTL_FILE_PATH_MAX + 1), UTL_FILE_PATH_MAX + 1);
        s_strnncpy(pucUniqueStopTermCount, pucUtlStringsFormatUnsignedNumber(psiSrchIndex->psibSrchIndexBuild->uiUniqueStopTermCount, pucNumberString, UTL_FILE_PATH_MAX + 1), UTL_FILE_PATH_MAX + 1);
        s_strnncpy(pucTotalStopTermCount, pucUtlStringsFormatUnsignedNumber(psiSrchIndex->psibSrchIndexBuild->uiTotalStopTermCount, pucNumberString, UTL_FILE_PATH_MAX + 1), UTL_FILE_PATH_MAX + 1);
        s_strnncpy(pucDocumentCount, pucUtlStringsFormatUnsignedNumber(psiSrchIndex->psibSrchIndexBuild->uiDocumentCount, pucNumberString, UTL_FILE_PATH_MAX + 1), UTL_FILE_PATH_MAX + 1);
        iUtlLogInfo(UTL_LOG_CONTEXT, "Flushing: terms, unique: %s, total: %s; stop terms, unique: %s, total: %s; documents: %s.", 
                pucUniqueTermCount, pucTotalTermCount, pucUniqueStopTermCount, pucTotalStopTermCount, pucDocumentCount);
        bFlushStopTerm = true;
    }
    else {
        s_strnncpy(pucUniqueTermCount, pucUtlStringsFormatUnsignedNumber(psiSrchIndex->psibSrchIndexBuild->uiUniqueTermCount, pucNumberString, UTL_FILE_PATH_MAX + 1), UTL_FILE_PATH_MAX + 1);
        s_strnncpy(pucTotalTermCount, pucUtlStringsFormatUnsignedNumber(psiSrchIndex->psibSrchIndexBuild->uiTotalTermCount, pucNumberString, UTL_FILE_PATH_MAX + 1), UTL_FILE_PATH_MAX + 1);
        s_strnncpy(pucDocumentCount, pucUtlStringsFormatUnsignedNumber(psiSrchIndex->psibSrchIndexBuild->uiDocumentCount, pucNumberString, UTL_FILE_PATH_MAX + 1), UTL_FILE_PATH_MAX + 1);
        iUtlLogInfo(UTL_LOG_CONTEXT, "Flushing: terms, unique: %s, total: %s; documents: %s.", pucUniqueTermCount, pucTotalTermCount, pucDocumentCount);
        bFlushStopTerm = false;
    }


    /* Open the index block file we are going to flush our terms to, note that we create a file path with version */
    if ( (iError = iSrchFilePathsGetTempTermDictionaryFilePathFromIndex(psiSrchIndex, psiSrchIndex->psibSrchIndexBuild->uiIndexFileNumber, false, pucFilePath, UTL_FILE_PATH_MAX + 1)) != SRCH_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to create the index file path, index: '%s', srch error: %d.", psiSrchIndex->pucIndexName, iError);
        return (SRCH_InvertIndexBlockFlushFailed);
    }
    if ( (pfFile = s_fopen(pucFilePath, "w")) == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to create the index file: '%s'.", pucFilePath);
        return (SRCH_InvertIndexBlockFlushFailed);
    }

    /* Loop over the trie entries and flush the index blocks to disk */
    if ( (iError = iUtlTrieLoop(psiSrchIndex->psibSrchIndexBuild->pvUtlTermTrie, NULL, 
            (int (*)())iSrchInvertFlushIndexBlocksCallBack, psiSrchIndex, pfFile, (int)bFlushStopTerm)) != UTL_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to loop over the entries in the terms trie, utl error: %d.", iError);
        return (SRCH_InvertIndexBlockFlushFailed);
    }

    /* Close the index block file */
    s_fclose(pfFile);


    iUtlLogInfo(UTL_LOG_CONTEXT, "Finished flushing term index file: %u.", psiSrchIndex->psibSrchIndexBuild->uiIndexFileNumber);


    /* Increment the index block file number */
    psiSrchIndex->psibSrchIndexBuild->uiIndexFileNumber++;


    /* Since everything is written out, we can re-initialize the counters
    ** used to track term counts and document count in the building phase
    */
    psiSrchIndex->psibSrchIndexBuild->uiTotalTermCount = 0;
    psiSrchIndex->psibSrchIndexBuild->uiUniqueTermCount = 0;
    psiSrchIndex->psibSrchIndexBuild->uiTotalStopTermCount = 0;
    psiSrchIndex->psibSrchIndexBuild->uiUniqueStopTermCount = 0;
    psiSrchIndex->psibSrchIndexBuild->uiDocumentCount = 0;


    return (SRCH_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchInvertFlushIndexBlocksCallBack()

    Purpose:    Flush the index block to disk

    Parameters: pucKey      key (term)
                pvData      data        
                ap          args (optional)

    Globals:    none

    Returns:    0 if successful, -1 on error

*/
static int iSrchInvertFlushIndexBlocksCallBack
(
    unsigned char *pucKey,
    void *pvData,
    va_list ap
)
{

    va_list                     ap_;
    int                         iError = SRCH_NoError;
    struct srchInvertTrieTerm   *psittSrchInvertTrieTerm = NULL;
    struct srchIndex            *psiSrchIndex = NULL;
    FILE                        *pfFile = NULL;
    boolean                     bFlushStopTerm = false;
    unsigned int                uiIndexBlockDataLength = 0;
    unsigned char               pucBuffer[SRCH_INVERT_INDEX_BLOCK_DATA_LENGTH_SIZE];
    unsigned char               *pucBufferPtr = pucBuffer;


    ASSERT(bUtlStringsIsStringNULL(pucKey) == false);
    ASSERT(pvData != NULL);


    /* Get the invert trie term */
    psittSrchInvertTrieTerm = (struct srchInvertTrieTerm *)pvData;

    /* Get all our parameters, note that we make a copy of 'ap' */
    va_copy(ap_, ap);
    psiSrchIndex = (struct srchIndex *)va_arg(ap_, struct srchIndex *);
    pfFile = (FILE *)va_arg(ap_, FILE *);
    bFlushStopTerm = (boolean)va_arg(ap_, int);
    va_end(ap_);


    ASSERT(psiSrchIndex != NULL);
    ASSERT(pfFile != NULL);
    ASSERT((bFlushStopTerm == true) || (bFlushStopTerm == false));


    /* Get the index block size for this term */
    uiIndexBlockDataLength = (psittSrchInvertTrieTerm->pucIndexBlock != NULL) ? 
            (psittSrchInvertTrieTerm->pucIndexBlockEndPtr - psittSrchInvertTrieTerm->pucIndexBlock) : 0;


    /* Dont flush stop terms if we are not required to do so and if there are no index entries for it */
    if ( (bFlushStopTerm == false) && (psittSrchInvertTrieTerm->uiTermType == SPI_TERM_TYPE_STOP) && 
            (psittSrchInvertTrieTerm->pucIndexBlock == NULL) && ((psittSrchInvertTrieTerm->pucIndexBlockEndPtr - psittSrchInvertTrieTerm->pucIndexBlock) == 0) ) {
        return (0);
    }

    /* Write the dictionary entry in the index file */
    if ( (iError = iSrchInvertIndexBlockDictEntryWrite(pfFile, pucKey, psittSrchInvertTrieTerm->uiTermType, psittSrchInvertTrieTerm->uiTermCount, 
            psittSrchInvertTrieTerm->uiDocumentCount, psittSrchInvertTrieTerm->bIncludeInCounts)) != SRCH_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to write the index file dictionary entry, srch error: %d.", iError);
        return (-1);
    }


    /* Write out the block size in a space of SRCH_INVERT_INDEX_BLOCK_DATA_LENGTH_SIZE */
    pucBufferPtr = pucBuffer;
    UTL_NUM_WRITE_UINT(uiIndexBlockDataLength, SRCH_INVERT_INDEX_BLOCK_DATA_LENGTH_SIZE, pucBufferPtr);
    ASSERT((pucBufferPtr - pucBuffer) == SRCH_INVERT_INDEX_BLOCK_DATA_LENGTH_SIZE);
    if ( s_fwrite(pucBuffer, SRCH_INVERT_INDEX_BLOCK_DATA_LENGTH_SIZE, 1, pfFile) != 1 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to write the index block header.");
        return (-1);
    }


    /* Write the index block in the index file */
    if ( uiIndexBlockDataLength > 0 ) {
        if ( uiIndexBlockDataLength != s_fwrite(psittSrchInvertTrieTerm->pucIndexBlock, 1, uiIndexBlockDataLength, pfFile) ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to write the index block to the index file.");
            return (-1);
        }
    }


    return (0);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchInvertFreeIndexBlocksCallBack()

    Purpose:    Free the index block.

    Parameters: pucKey      key (term)
                pvData      data        
                ap          args (optional)

    Globals:    none

    Returns:    0 if successful, -1 on error

*/
static int iSrchInvertFreeIndexBlocksCallBack
(
    unsigned char *pucKey,
    void *pvData,
    va_list ap
)
{

    va_list                     ap_;
    struct srchInvertTrieTerm   *psittSrchInvertTrieTerm = NULL;
    struct srchIndex            *psiSrchIndex = NULL;


    ASSERT(bUtlStringsIsStringNULL(pucKey) == false);
    ASSERT(pvData != NULL);


    /* Get the invert trie term */
    psittSrchInvertTrieTerm = (struct srchInvertTrieTerm *)pvData;

    /* Get all our parameters, note that we make a copy of 'ap' */
    va_copy(ap_, ap);
    psiSrchIndex = (struct srchIndex *)va_arg(ap_, struct srchIndex *);
    va_end(ap_);


    ASSERT(psiSrchIndex != NULL);
    ASSERT(psiSrchIndex->psibSrchIndexBuild != NULL);


    /* Free the index block */
    s_free(psittSrchInvertTrieTerm->pucIndexBlock);


    /* Clear the index block pointers and capacity */
    psittSrchInvertTrieTerm->pucIndexBlock = NULL;
    psittSrchInvertTrieTerm->pucIndexBlockEndPtr = NULL;
    psittSrchInvertTrieTerm->uiIndexBlockCapacity = 0;


    return (0);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchInvertIndexBlockDictEntryWrite()

    Purpose:    Writes the index block dict entry

    Parameters: pfFile              index block file descriptor
                pucTerm             term
                uiTermType          term type
                uiTermCount         term count
                uiDocumentCount     document count
                bIncludeInCounts    counts inclusion flag

    Globals:    none

    Returns:    SRCH error code

*/
static int iSrchInvertIndexBlockDictEntryWrite
(
    FILE *pfFile,
    unsigned char *pucTerm,
    unsigned int uiTermType,
    unsigned int uiTermCount,
    unsigned int uiDocumentCount,
    boolean bIncludeInCounts
)
{

    unsigned int    uiIncludeInTermCounts = 0;
    unsigned int    uiTermLength = 0;
    unsigned char   pucBuffer[SRCH_INVERT_INDEX_BLOCK_HEADER_LENGTH];
    unsigned char   *pucBufferPtr = pucBuffer;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iSrchInvertIndexBlockDictEntryWrite - pucTerm: %s, uiTermType: %u, uiTermCount: %u, uiDocumentCount: %u",  */
/*             pucTerm, uiTermType, uiTermCount, uiDocumentCount); */


    ASSERT(pfFile != NULL);
    ASSERT(bUtlStringsIsStringNULL(pucTerm) == false);
    ASSERT(uiTermType >= 0);
    ASSERT(uiTermCount >= 0);
    ASSERT(uiDocumentCount >= 0);
    ASSERT((bIncludeInCounts == true) || (bIncludeInCounts == false));


    /* Set the term count flag */
    uiIncludeInTermCounts = (bIncludeInCounts == true) ? 1 : 0;

    /* Get the term length */
    uiTermLength = s_strlen(pucTerm);

    /* Set the start of the buffer pointer */
    pucBufferPtr = pucBuffer;

    /* Write out the start of the index dictionary entry */
    UTL_NUM_WRITE_UINT(uiTermType, SRCH_INVERT_INDEX_BLOCK_TERM_TYPE_SIZE, pucBufferPtr);
    UTL_NUM_WRITE_UINT(uiTermCount, SRCH_INVERT_INDEX_BLOCK_TERM_COUNT_SIZE, pucBufferPtr);
    UTL_NUM_WRITE_UINT(uiDocumentCount, SRCH_INVERT_INDEX_BLOCK_DOCUMENT_COUNT_SIZE, pucBufferPtr);
    UTL_NUM_WRITE_UINT(uiIncludeInTermCounts, SRCH_INVERT_INDEX_BLOCK_INCLUDE_IN_COUNTS_SIZE, pucBufferPtr);
    UTL_NUM_WRITE_UINT(uiTermLength, SRCH_INVERT_INDEX_BLOCK_TERM_SIZE, pucBufferPtr);

    ASSERT((pucBufferPtr - pucBuffer) == SRCH_INVERT_INDEX_BLOCK_HEADER_LENGTH);

    /* Write out the buffer */
    if ( s_fwrite(pucBuffer, SRCH_INVERT_INDEX_BLOCK_HEADER_LENGTH, 1, pfFile) != 1 ) {
        return (SRCH_InvertIndexBlockWriteFailed);
    }

    /* Write out the term, exclude the terminating NULL */
    if ( s_fwrite(pucTerm, uiTermLength, 1, pfFile) != 1 ) {
        return (SRCH_InvertIndexBlockWriteFailed);
    }


    return (SRCH_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchInvertIndexBlockDictEntryRead()

    Purpose:    This read the dictionary index block from the index stream.
                It assumes the stream is positioned at the start of a dictionary
                block, and will return non-0 if it is not.

    Parameters: pfFile              file descriptor
                ppucTerm            term
                puiTermType         term type
                puiTermCount        term count
                puiDocumentCount    document count
                pbIncludeInCount    count inclusion flag

    Globals:    none

    Returns:    SRCH error code

                returns SRCH_InvertIndexBlockReadEOF if it is at the end of a file
                returns SRCH_InvertIndexBlockReadFailed on error

*/
static int iSrchInvertIndexBlockDictEntryRead
(
    FILE *pfFile,
    unsigned char **ppucTerm,
    unsigned int *puiTermType,
    unsigned int *puiTermCount,
    unsigned int *puiDocumentCount,
    boolean *pbIncludeInCount
)
{

    unsigned int    uiTermType = 0;
    unsigned int    uiIncludeInTermCounts = 0;
    unsigned int    uiTermCount = 0;
    unsigned int    uiDocumentCount = 0;
    unsigned int    uiTermLength = 0;
    unsigned char   *pucTerm = NULL;
    unsigned char   pucBuffer[SRCH_INVERT_INDEX_BLOCK_HEADER_LENGTH];
    unsigned char   *pucBufferPtr = NULL;


    ASSERT(pfFile != NULL);
    ASSERT(ppucTerm != NULL);
    ASSERT(puiTermType != NULL);
    ASSERT(puiTermCount != NULL);
    ASSERT(puiDocumentCount != NULL);
    ASSERT(pbIncludeInCount != NULL);


    /* Clear the return parameters */
    *ppucTerm = NULL;
    *puiTermType = 0;    
    *puiTermCount = 0;    
    *puiDocumentCount = 0;    
    *pbIncludeInCount = false;    


    /* Read the buffer */
    if ( s_fread(pucBuffer, SRCH_INVERT_INDEX_BLOCK_HEADER_LENGTH, 1, pfFile) != 1 ) {
        /* Not an error, it is how we tell that we are done */
        if ( s_feof(pfFile) != 0 ) {
            return (SRCH_InvertIndexBlockReadEOF);
        }
        else {
            return (SRCH_InvertIndexBlockReadFailed);
        }
    }

    /* Set the pointer to read from */
    pucBufferPtr = pucBuffer;

    /* Read the rest of the header, decode to a variable and copy to the pointer for optimization */ 
    UTL_NUM_READ_UINT(uiTermType, SRCH_INVERT_INDEX_BLOCK_TERM_TYPE_SIZE, pucBufferPtr);
    UTL_NUM_READ_UINT(uiTermCount, SRCH_INVERT_INDEX_BLOCK_TERM_COUNT_SIZE, pucBufferPtr);
    UTL_NUM_READ_UINT(uiDocumentCount, SRCH_INVERT_INDEX_BLOCK_DOCUMENT_COUNT_SIZE, pucBufferPtr);
    UTL_NUM_READ_UINT(uiIncludeInTermCounts, SRCH_INVERT_INDEX_BLOCK_INCLUDE_IN_COUNTS_SIZE, pucBufferPtr);
    UTL_NUM_READ_UINT(uiTermLength, SRCH_INVERT_INDEX_BLOCK_TERM_SIZE, pucBufferPtr);
    *puiTermType = uiTermType;
    *puiTermCount = uiTermCount;
    *puiDocumentCount = uiDocumentCount;
    *pbIncludeInCount = (uiIncludeInTermCounts == 0) ? false : true;    

    ASSERT((pucBufferPtr - pucBuffer) == SRCH_INVERT_INDEX_BLOCK_HEADER_LENGTH);


    /* Allocate space for the term */
    if ( (pucTerm = (unsigned char *)s_malloc(sizeof(unsigned char) * (uiTermLength + 1))) == NULL ) {
        return (SRCH_MemError);
    }

    /* Read the term and NULL terminate it */
    if ( s_fread(pucTerm, uiTermLength, 1, pfFile) != 1 ) {
        return (SRCH_InvertIndexBlockReadFailed);
    }
    pucTerm[uiTermLength] = '\0';

    /* Set the return pointer */
    *ppucTerm = pucTerm;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iSrchInvertIndexBlockDictEntryRead - *ppucTerm: %s, *puiTermType: %u, *puiTermCount: %u, *puiDocumentCount: %u, *pbIncludeInCount: '%s'",  */
/*             *ppucTerm, *puiTermType, *puiTermCount, *puiDocumentCount, (*pbIncludeInCount == true) ? "true" : "false"); */


    return (SRCH_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchInvertMerge()

    Purpose:    This merges all the temporary inverted files into a large on 
                and creates a dictionary.

    Parameters: psiSrchIndex    search index structure

    Globals:    none

    Returns:    SRCH error code

*/
static int iSrchInvertMerge
(
    struct srchIndex *psiSrchIndex
)
{

    int             iError = SRCH_NoError;
    unsigned char   pucFilePath[UTL_FILE_PATH_MAX + 1] = {'\0'};
    unsigned int    uiIndexFileNumber = 0;
    unsigned int    uiSrchInvertIndexMergeLength = 0;
    unsigned int    uiPreviousIndexFileCount = 0;
    unsigned int    uiStartVersion = 0;
    unsigned int    uiEndVersion = 0;
    unsigned int    uiI = 0;
    off_t           zFileLength = 0;
    boolean         bCompletion = false;
    boolean         bStartVersionSet = false;


    ASSERT(psiSrchIndex != NULL);

    ASSERT(psiSrchIndex->psibSrchIndexBuild != NULL);

    iUtlLogInfo(UTL_LOG_CONTEXT, "Merging: %u file%s.", psiSrchIndex->psibSrchIndexBuild->uiIndexFileNumber, 
            psiSrchIndex->psibSrchIndexBuild->uiIndexFileNumber == 1 ? "" : "s" );


    /* Allocate a field ID bitmap only if there are any fields other than field ID 0 */
    if ( psiSrchIndex->psibSrchIndexBuild->uiFieldIDBitmapLength > 0 ) {

        /* Allocate the field ID bitmap - field ID 0 is not a field */
        if ( (psiSrchIndex->psibSrchIndexBuild->pucFieldIDBitmap = 
                (unsigned char *)s_malloc(sizeof(unsigned char) * UTL_BITMAP_GET_BITMAP_BYTE_LENGTH(psiSrchIndex->psibSrchIndexBuild->uiFieldIDBitmapLength))) == NULL ) {
            return (SRCH_MemError);
        }
    }


    /* Extract some figures from the index structure for convenience */
    uiIndexFileNumber = psiSrchIndex->psibSrchIndexBuild->uiIndexFileNumber;


    /* Start an infinite loop, we control the exit from within */
    while ( true ) {

        /* Set the completion flag */
        bCompletion = false;


        /* Count up the number of files remaining */
        for ( uiI = 0, uiSrchInvertIndexMergeLength = 0; uiI < uiIndexFileNumber; uiI++ ) {

            /* Create the file path */
            if ( (iError = iSrchFilePathsGetTempTermDictionaryFilePathFromIndex(psiSrchIndex, uiI, false, pucFilePath, UTL_FILE_PATH_MAX + 1)) != SRCH_NoError ) {
                iUtlLogError(UTL_LOG_CONTEXT, "Failed to create the index file path, index: '%s', srch error: %d.", psiSrchIndex->pucIndexName, iError);
                return (SRCH_InvertMergeFailed);
            } 

            /* Increment the number of index files */
            uiSrchInvertIndexMergeLength += (bUtlFileIsFile(pucFilePath) == true) ? 1 : 0;
        }


        /* We set the completion flag and exit the loop if the number of files
        ** is less than SRCH_INVERT_MERGE_WIDTH because we can do the final merge now
        */
        if ( uiSrchInvertIndexMergeLength <= SRCH_INVERT_MERGE_WIDTH ) {
            bCompletion = true;
            break;
        }


        /* See if we reduced the number of files, this is the general idea 
        ** so if we see that this is not happening, then there is a problem
        */
        if ( (uiPreviousIndexFileCount != 0) && (uiSrchInvertIndexMergeLength >= uiPreviousIndexFileCount) ) {
            bCompletion = false;
            break;
        }


        /* Start looping from uiStartVersion adding up the size of the files until
        ** we reach UTL_FILE_LEN_MAX or SRCH_INVERT_MERGE_WIDTH or uiIndexFileNumber at which point we merge the
        ** files and set the flag, if it was uiIndexFileNumber that we reached, we reset uiStartVersion
        ** to 0 to start a new iteration
        */
        for ( uiSrchInvertIndexMergeLength = 0, zFileLength = 0, uiStartVersion = 0, bStartVersionSet = false;
                uiEndVersion < uiIndexFileNumber; uiEndVersion++ ) {
        
            /* Create the file path */
            if ( (iError = iSrchFilePathsGetTempTermDictionaryFilePathFromIndex(psiSrchIndex, uiEndVersion, false, pucFilePath, UTL_FILE_PATH_MAX + 1)) != SRCH_NoError ) {
                iUtlLogError(UTL_LOG_CONTEXT, "Failed to create the index file path, index: '%s', srch error: %d.", psiSrchIndex->pucIndexName, iError);
                return (SRCH_InvertMergeFailed);
            }  

            /* Process this file if it exists */
            if ( bUtlFileIsFile(pucFilePath) == true ) {

                off_t   zIntermediateFileLength = 0;
    
                /* Set the start version if it has not been set */
                if ( bStartVersionSet == false ) {
                    uiStartVersion = (uiStartVersion == 0) ? uiEndVersion : uiStartVersion;
                    bStartVersionSet = true;
                }

                /* Get the intermediate file length */
                if ( (iError = iUtlFileGetFilePathLength(pucFilePath, &zIntermediateFileLength)) != UTL_NoError ) {
                    iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the length of the terms index file: '%s', utl error: %d.", pucFilePath, iError);
                    return (SRCH_InvertMergeFailed);
                }
                
                /* Are we at a point where we need to merge? */
                if ( ((zFileLength + zIntermediateFileLength) > UTL_FILE_LEN_MAX) || (uiSrchInvertIndexMergeLength >= SRCH_INVERT_MERGE_WIDTH) || (uiEndVersion == (uiIndexFileNumber - 1)) ) {

                    if ( (zFileLength + zIntermediateFileLength) > UTL_FILE_LEN_MAX ) {
                        uiEndVersion--;
/*                         iUtlLogDebug(UTL_LOG_CONTEXT, "triggered by file size [%ld][%ld] - ", zFileLength, zIntermediateFileLength); */
                    }
                    else if ( uiSrchInvertIndexMergeLength >= SRCH_INVERT_MERGE_WIDTH ) {
                        uiEndVersion--;
/*                         iUtlLogDebug(UTL_LOG_CONTEXT, "triggered by SRCH_INVERT_MERGE_WIDTH - "); */
                    }
                    else if ( uiEndVersion == (uiIndexFileNumber - 1) ) {
/*                         iUtlLogDebug(UTL_LOG_CONTEXT, "triggered by last index file - "); */
                        uiSrchInvertIndexMergeLength++;
                    }


/*                     iUtlLogDebug(UTL_LOG_CONTEXT, "[%u, %u, %u] - ", uiStartVersion, uiEndVersion, uiSrchInvertIndexMergeLength); */

                    ASSERT(uiStartVersion <= uiEndVersion);

                    /* Set the completion flag */
                    bCompletion = true;

                    /* Are we merging multiple files? */
                    if ( uiSrchInvertIndexMergeLength > 1 ) {

                        /* Do the intermediate merge here */
                        if ( (iError = iSrchInvertMergeIndexFilesSetup(psiSrchIndex, uiStartVersion, uiEndVersion, uiSrchInvertIndexMergeLength, false)) != SRCH_NoError ) {
                            return (iError);
                        }
                    }
                    /* Are we merging a single file? */
                    else if ( uiSrchInvertIndexMergeLength == 1  ) { 
/*                         iUtlLogDebug(UTL_LOG_CONTEXT, "leaving %u alone", uiStartVersion); */
                    }
                    else {
/*                         iUtlLogDebug(UTL_LOG_CONTEXT, "serious error [%u, %u]", uiStartVersion, uiEndVersion); */
                        break;
                    }

                    /* Reset the end */
                    uiEndVersion = (uiEndVersion == (uiIndexFileNumber - 1)) ? 0 : uiEndVersion + 1;

                    /* And bounce out of the for() loop since we have just finished this merge,
                    ** we need to check if we can do a final merge after each merge
                    */
                    break;
                }


                /* Increment file count and file size counters */
                zFileLength += zIntermediateFileLength;
                uiSrchInvertIndexMergeLength++;
            }
        }



        /*If the flag is not set, we cant complete this merge, so we break out */
        if ( bCompletion == false ) {
            break;
        }
    }


    /* If the completion flag is set, we can complete this merge */
    if ( bCompletion == true ) {

/*         iUtlLogDebug(UTL_LOG_CONTEXT, "[%u, %u]", uiIndexFileNumber - 1, uiSrchInvertIndexMergeLength); */

        if ( (iError = iSrchInvertMergeIndexFilesSetup(psiSrchIndex, 0, uiIndexFileNumber - 1, uiSrchInvertIndexMergeLength, true)) != SRCH_NoError ) {
            return (iError);
        }
    }
    else {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to complete the term index file merge.");
    }


    return ((bCompletion == true) ? SRCH_NoError: SRCH_InvertMergeFailed);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchInvertMergeIndexFilesSetup()

    Purpose:    Merges a index file (lFromVersion) into another (lIntoVersion).
                This is done by merging both into version -1 and then, 
                renames it to lIntoVersion, then deletes lFromVersion.

    Parameters: psiSrchIndex                    search index structure
                uiStartVersion                  index file version to start with
                uiEndVersion                    index file version to end with (stop short of that)
                uiSrchInvertIndexMergeLength    index merge length
                bFinalMerge                     true if this is the final merge

    Globals:    none

    Returns:    SRCH error code

*/
static int iSrchInvertMergeIndexFilesSetup
(
    struct srchIndex *psiSrchIndex,
    unsigned int uiStartVersion,
    unsigned int uiEndVersion,
    unsigned int uiSrchInvertIndexMergeLength,
    boolean bFinalMerge
)
{

    int                             iError = SRCH_NoError;
    unsigned char                   pucOutputFilePath[UTL_FILE_PATH_MAX + 1] = {'\0'};            /* Version -1 */
    unsigned char                   pucInputFilePath[UTL_FILE_PATH_MAX + 1] = {'\0'};
    FILE                            *pfOutputFile = NULL;
    struct srchInvertIndexMerge     *psiimSrchInvertIndexMerge = NULL;
    struct srchInvertIndexMerge     *psiimSrchInvertIndexMergePtr = NULL;
    unsigned int                    uiI = 0;
    unsigned char                   pucNumberString[UTL_FILE_PATH_MAX + 1] = {'\0'};


    ASSERT(psiSrchIndex != NULL);
    ASSERT(uiStartVersion >= 0);
    ASSERT(uiEndVersion >= 0);
    ASSERT(uiSrchInvertIndexMergeLength > 0);
    ASSERT((bFinalMerge == true) || (bFinalMerge == false));

    ASSERT(psiSrchIndex->psibSrchIndexBuild != NULL);
    ASSERT(uiEndVersion >= uiStartVersion);


    /* Allocate the merge array, this structure allows us to track the various index files we are merging */
    if ( (psiimSrchInvertIndexMerge = (struct srchInvertIndexMerge *)s_malloc((size_t)(sizeof(struct srchInvertIndexMerge) * uiSrchInvertIndexMergeLength))) == NULL ) {
        return (SRCH_MemError);
    }


    /* Log */
    if ( bFinalMerge == true ) {
        iUtlLogInfo(UTL_LOG_CONTEXT, "Merging term index file%s: %s into repository and generating term dictionary.", (uiStartVersion == uiEndVersion) ? "" : "s", 
                pucSrchInvertPrettyPrintFileNumbers(psiSrchIndex, uiStartVersion, uiEndVersion, pucNumberString, UTL_FILE_PATH_MAX + 1));
    }
    else {
        iUtlLogInfo(UTL_LOG_CONTEXT, "Merging term index file%s: %s into term index file: %u", (uiStartVersion == uiEndVersion) ? "" : "s", 
                pucSrchInvertPrettyPrintFileNumbers(psiSrchIndex, uiStartVersion, uiEndVersion, pucNumberString, UTL_FILE_PATH_MAX + 1), uiStartVersion);
    }



    /* Open the input index files */
    for ( uiI = uiStartVersion, psiimSrchInvertIndexMergePtr = psiimSrchInvertIndexMerge; uiI <= uiEndVersion; uiI++ ) {

        /* Create an index file path for this version */
        if ( (iError = iSrchFilePathsGetTempTermDictionaryFilePathFromIndex(psiSrchIndex, uiI, false, pucInputFilePath, UTL_FILE_PATH_MAX + 1)) != SRCH_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to create the index file path, index: '%s', srch error: %d.", psiSrchIndex->pucIndexName, iError);
            return (SRCH_InvertMergeFailed);
        } 

        if ( bUtlFileIsFile(pucInputFilePath) == true ) {

            s_strnncpy(psiimSrchInvertIndexMergePtr->pucFilePath, pucInputFilePath, UTL_FILE_PATH_MAX + 1);

            /* Open the index file */
            if ( (psiimSrchInvertIndexMergePtr->pfFile = s_fopen(psiimSrchInvertIndexMergePtr->pucFilePath, "r")) == NULL ) {
                iUtlLogError(UTL_LOG_CONTEXT, "Failed to open the input index file: '%s'.", psiimSrchInvertIndexMergePtr->pucFilePath);
                return (SRCH_InvertMergeFailed);
            }

            /* Increment to the next index merge entry */
            psiimSrchInvertIndexMergePtr++;
        }
    }



    /* Set up for the merge */
    if ( bFinalMerge == false ) {

        /* Create the output file path, note that it is a shadow file */
        if ( (iError = iSrchFilePathsGetTempTermDictionaryFilePathFromIndex(psiSrchIndex, uiStartVersion, true, pucOutputFilePath, UTL_FILE_PATH_MAX + 1)) != SRCH_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to create the index file path, index: '%s', srch error: %d.", psiSrchIndex->pucIndexName, iError);
            return (SRCH_InvertMergeFailed);
        } 

        /* Open the output file */
        if ( (pfOutputFile = s_fopen(pucOutputFilePath, "w")) == NULL ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to create the output index file: '%s'.", pucOutputFilePath);
            return (SRCH_InvertMergeFailed);
        }
    }


    /* Do the merge */
    if ( (iError = iSrchInvertMergeIndexFiles(psiSrchIndex, psiimSrchInvertIndexMerge, uiSrchInvertIndexMergeLength, pfOutputFile, bFinalMerge)) != SRCH_NoError ) {
        return (iError);
    }



    /* Delete the input files */
    for ( uiI = uiStartVersion; uiI <= uiEndVersion; uiI++ ) {
        
        if ( (iError = iSrchFilePathsGetTempTermDictionaryFilePathFromIndex(psiSrchIndex, uiI, false, pucInputFilePath, UTL_FILE_PATH_MAX + 1)) != SRCH_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to create the index file path, index: '%s', srch error: %d.", psiSrchIndex->pucIndexName, iError);
            return (SRCH_InvertMergeFailed);
        } 

        if ( bUtlFileIsFile(pucInputFilePath) == true ) {
            s_remove(pucInputFilePath);
        }
    }


    /* Release the merge array */
    s_free(psiimSrchInvertIndexMerge);



    /* Rename the output file to the new version */
    if ( bFinalMerge == false ) {

        /* Close the output index file */
        s_fclose(pfOutputFile);

        if ( (iError = iSrchFilePathsGetTempTermDictionaryFilePathFromIndex(psiSrchIndex, uiStartVersion, false, pucInputFilePath, UTL_FILE_PATH_MAX + 1)) != SRCH_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to create the index file path, index: '%s', srch error: %d.", psiSrchIndex->pucIndexName, iError);
            return (SRCH_InvertMergeFailed);
        } 

        if ( s_rename(pucOutputFilePath, pucInputFilePath) != 0 ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to rename file from: '%s' to: '%s'.", pucOutputFilePath, pucInputFilePath);
            return (SRCH_InvertMergeFailed);
        }
    }


    return (SRCH_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchInvertMergeIndexFiles()

    Purpose:    Merges index files from the index merge array.

    Parameters: psiSrchIndex                    search index structure
                psiimSrchInvertIndexMerge       index merge structure
                uiSrchInvertIndexMergeLength    number of entries in the index merge structure
                pfOutputFile                    output file descriptor (NULL if bFinalMerge is true)
                bFinalMerge                     true if this is the final merge

    Globals:    none

    Returns:    SRCH error code

*/
static int iSrchInvertMergeIndexFiles
(
    struct srchIndex *psiSrchIndex,
    struct srchInvertIndexMerge *psiimSrchInvertIndexMerge,
    unsigned int uiSrchInvertIndexMergeLength,
    FILE *pfOutputFile,
    boolean bFinalMerge
)
{

    int                             iError = SRCH_NoError;
    unsigned char                   *pucTermPtr = NULL;
    boolean                         bIsStopTerm = false;
    struct srchInvertIndexMerge     *psiimSrchInvertIndexMergePtr = NULL;
    unsigned int                    uiI = 0;


    ASSERT(psiSrchIndex != NULL);
    ASSERT(psiimSrchInvertIndexMerge != NULL);
    ASSERT(uiSrchInvertIndexMergeLength > 0);
    ASSERT(((pfOutputFile != NULL) && (bFinalMerge == false)) || ((pfOutputFile == NULL) && (bFinalMerge == true)));

    ASSERT(psiSrchIndex->psibSrchIndexBuild != NULL);
    ASSERT(psiSrchIndex->uiIntent == SRCH_INDEX_INTENT_CREATE);



    /* Keep looping forever, we control the breakout timing from within the loop */
    while ( true ) {
    
        /* Set the stop term flag */
        bIsStopTerm = false;

        /* Preset the term pointer */
        pucTermPtr = NULL;

        /* Loop over the merge array and read in the terms from the index files */
        for ( uiI = 0, psiimSrchInvertIndexMergePtr = psiimSrchInvertIndexMerge; uiI < uiSrchInvertIndexMergeLength; uiI++, psiimSrchInvertIndexMergePtr++ ) {

            /* Loop over all the open files, closed files indicate that either there is nothing
            ** more to read from the file or that the merge array slot was allocated but not used
            */
            if ( psiimSrchInvertIndexMergePtr->pfFile != NULL ) {

                /* Check the term slot */
                if ( bUtlStringsIsStringNULL(psiimSrchInvertIndexMergePtr->pucTerm) == false ) {
                    /* There is still a term here to handle, so we set the current term */
                    if ( (bUtlStringsIsStringNULL(pucTermPtr) == true) || (s_strcmp(psiimSrchInvertIndexMergePtr->pucTerm, pucTermPtr) < 0) ) {
                        pucTermPtr = psiimSrchInvertIndexMergePtr->pucTerm;
                    }
                }
                else {
                    /* The slot is clear, we need to read the next term in this index block */

                    /* Read the first dictionary block from the from index blocks stream */
                    iError = iSrchInvertIndexBlockDictEntryRead(psiimSrchInvertIndexMergePtr->pfFile, &psiimSrchInvertIndexMergePtr->pucTerm,
                            &psiimSrchInvertIndexMergePtr->uiTermType, &psiimSrchInvertIndexMergePtr->uiTermCount, 
                            &psiimSrchInvertIndexMergePtr->uiDocumentCount, &psiimSrchInvertIndexMergePtr->bIncludeInCounts);

                    /* Handle the error */
                    if ( iError == SRCH_NoError ) {
                        /* This was a real block which was read, so we set the current term */
                        if ( (bUtlStringsIsStringNULL(pucTermPtr) == true) || (s_strcmp(psiimSrchInvertIndexMergePtr->pucTerm, pucTermPtr) < 0) ) {
                            pucTermPtr = psiimSrchInvertIndexMergePtr->pucTerm;
                        }
                    }
                    else if ( iError == SRCH_InvertIndexBlockReadEOF ) {
                         /* We reached the end of the index blocks for this index file, so  we clear this entry */
                         s_fclose(psiimSrchInvertIndexMergePtr->pfFile);
                         psiimSrchInvertIndexMergePtr->pfFile = NULL;
                         s_memset(psiimSrchInvertIndexMergePtr, 0, sizeof(struct srchInvertIndexMerge));
                    }
                    else {
                        /* Another error occured which we could not handle */
                        iUtlLogError(UTL_LOG_CONTEXT, "Failed to read dictionary block in input index file: '%s', utl error: %d.", 
                                psiimSrchInvertIndexMergePtr->pucFilePath, iError);
                        return (iError);
                    }
                }
            }
        }


        /* Bail now if the term is empty */
        if ( bUtlStringsIsStringNULL(pucTermPtr) == true ) {
            break;
        }


        /* Loop over each entry in the structure setting the processing flag and the stop term flag */
        for ( uiI = 0, psiimSrchInvertIndexMergePtr = psiimSrchInvertIndexMerge; uiI < uiSrchInvertIndexMergeLength; uiI++, psiimSrchInvertIndexMergePtr++ ) {

            /* Is this the term we are current processing ? */
            if ( (bUtlStringsIsStringNULL(psiimSrchInvertIndexMergePtr->pucTerm) == false) && (s_strcmp(psiimSrchInvertIndexMergePtr->pucTerm, pucTermPtr) == 0) ) {

                /* Set the flag telling that we are currently processing this entry */
                psiimSrchInvertIndexMergePtr->bProcessing = true;

                /* Set the stop term flag */
                if ( bIsStopTerm == false ) {
                    if ( psiimSrchInvertIndexMergePtr->uiTermType == SPI_TERM_TYPE_STOP ) {
                        bIsStopTerm = true;
                    }
                }
            }
            else {
                /* Set the flag telling that we are NOT currently processing this entry */
                psiimSrchInvertIndexMergePtr->bProcessing = false;
            }
        }

        
        /* This term may have been turned into a stop term */ 
        if ( bIsStopTerm == true ) {

            /* Loop over each entry in the structure setting the stop term flag */
            for ( uiI = 0, psiimSrchInvertIndexMergePtr = psiimSrchInvertIndexMerge; uiI < uiSrchInvertIndexMergeLength; uiI++, psiimSrchInvertIndexMergePtr++ ) {
                if ( psiimSrchInvertIndexMergePtr->bProcessing == true ) {
                    psiimSrchInvertIndexMergePtr->uiTermType = SPI_TERM_TYPE_STOP;
                }
            }
        }


/*         iUtlLogDebug(UTL_LOG_CONTEXT, "iSrchInvertMergeIndexFiles - adding pucTermPtr: '%s'.", pucTermPtr); */

        if ( (iError = iSrchInvertStoreTermInIndex(psiSrchIndex, pucTermPtr, psiimSrchInvertIndexMerge, uiSrchInvertIndexMergeLength, NULL, 0, pfOutputFile, bFinalMerge)) != SRCH_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to store '%s' in the %s, srch error: %d.", 
                        pucTermPtr, (bFinalMerge == false) ? "output term index file" : "repository", iError);
            return (iError);
        }


        /* Loop over each entry in the structure erasing the term we just processed */
        for ( uiI = 0, psiimSrchInvertIndexMergePtr = psiimSrchInvertIndexMerge; uiI < uiSrchInvertIndexMergeLength; uiI++, psiimSrchInvertIndexMergePtr++ ) {
            if ( psiimSrchInvertIndexMergePtr->bProcessing == true ) {
                s_free(psiimSrchInvertIndexMergePtr->pucTerm);
            }
        }
            

    } /* While ( true ) */


    return (SRCH_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchInvertStoreTermInIndex()

    Purpose:    Adds a term to the index, the path the function will take
                depends on whether this is the final build, and whether this is 
                an update as opposed to a creation.

    Parameters: psiSrchIndex                    index structure
                pucTerm                         current term being merged
                psiimSrchInvertIndexMerge       index merge structure
                uiSrchInvertIndexMergeLength    number of entries in the index merge structure
                pvData                          data
                uiDataLength                    data length
                pfOutputFile                    output file descriptor (NULL if bFinalMerge is true)
                bFinalMerge                     set to true if this is the final merge

    Globals:    none

    Returns:    SRCH error code

*/
static int iSrchInvertStoreTermInIndex
(
    struct srchIndex *psiSrchIndex,
    unsigned char *pucTerm,
    struct srchInvertIndexMerge *psiimSrchInvertIndexMerge,
    unsigned int uiSrchInvertIndexMergeLength,
    void *pvData,
    unsigned int uiDataLength,
    FILE *pfOutputFile,
    boolean bFinalMerge
)
{

    int                             iError = UTL_NoError;
    unsigned int                    uiI = 0;
    unsigned int                    uiTermType = 0;
    boolean                         bIncludeInCounts = false;
    unsigned int                    uiTotalTermCount = 0;
    unsigned int                    uiTotalDocumentCount = 0;
    unsigned long                   ulIndexBlockObjectID = 0;
    unsigned char                   *pucIndexBlockPtr = NULL;
    unsigned char                   *pucIndexBlockDataPtr = NULL;
    unsigned int                    uiIndexBlockLength = 0;
    unsigned int                    uiIndexBlockDataLength = 0;
    unsigned int                    uiIndexBlockDataLengthSize = 0;
    struct srchInvertIndexMerge     *psiimSrchInvertIndexMergePtr = NULL;
    
    unsigned int                    uiVariableIndexBlockDataLengthSize = 0;



    ASSERT(psiSrchIndex != NULL);
    ASSERT(bUtlStringsIsStringNULL(pucTerm) == false);
    ASSERT(psiimSrchInvertIndexMerge != NULL);
    ASSERT(uiSrchInvertIndexMergeLength > 0);
    ASSERT(((pfOutputFile != NULL) && (bFinalMerge == false)) || ((pfOutputFile == NULL) && (bFinalMerge == true)));

    ASSERT(psiSrchIndex->psibSrchIndexBuild != NULL);
    ASSERT(psiSrchIndex->uiIntent == SRCH_INDEX_INTENT_CREATE);

    
    /* Get the variable index block data length size */
    uiVariableIndexBlockDataLengthSize = (bFinalMerge == true) ? SRCH_INVERT_INDEX_BLOCK_DATA_COMPRESSED_LENGTH_SIZE: SRCH_INVERT_INDEX_BLOCK_DATA_LENGTH_SIZE;


    /* Calculate the index block data length and increment some counts  */
    for ( uiI = 0, psiimSrchInvertIndexMergePtr = psiimSrchInvertIndexMerge; uiI < uiSrchInvertIndexMergeLength; uiI++, psiimSrchInvertIndexMergePtr++ ) {

        /* Is this a term we are current processing ? */
        if ( psiimSrchInvertIndexMergePtr->bProcessing == true ) {

            unsigned char   pucBuffer[SRCH_INVERT_INDEX_BLOCK_DATA_LENGTH_SIZE];
            unsigned char   *pucBufferPtr = pucBuffer;

            /* Read the buffer */
            if ( s_fread(pucBufferPtr, SRCH_INVERT_INDEX_BLOCK_DATA_LENGTH_SIZE, 1, psiimSrchInvertIndexMergePtr->pfFile) != 1 ) {
                iUtlLogError(UTL_LOG_CONTEXT, "Failed to read the index block.");
                return (SRCH_InvertIndexBlockReadFailed);
            }

            /* Read the size of the input index block */
            UTL_NUM_READ_UINT(psiimSrchInvertIndexMergePtr->uiIndexBlockDataLength, SRCH_INVERT_INDEX_BLOCK_DATA_LENGTH_SIZE, pucBufferPtr);
            ASSERT((pucBufferPtr - pucBuffer) == SRCH_INVERT_INDEX_BLOCK_DATA_LENGTH_SIZE);

            /* Increment the index block data length */
            uiIndexBlockDataLength += psiimSrchInvertIndexMergePtr->uiIndexBlockDataLength;

            /* Increment the counts */
            uiTotalTermCount += psiimSrchInvertIndexMergePtr->uiTermCount;
            uiTotalDocumentCount += psiimSrchInvertIndexMergePtr->uiDocumentCount;
        }
    }

    /* Set the term type */
    uiTermType = psiimSrchInvertIndexMerge->uiTermType;
    
    /* Set the count inclusion flag */
    bIncludeInCounts = psiimSrchInvertIndexMerge->bIncludeInCounts;


    /* Calculate the index block length */
    uiIndexBlockLength = uiVariableIndexBlockDataLengthSize + uiIndexBlockDataLength;


    /* Allocate space for the index block, this is preserved across calls to make 
    ** the process faster, this index block is released when we close the index
    */
    if ( uiIndexBlockLength > psiSrchIndex->psibSrchIndexBuild->uiIndexBlockLength ) {

        /* Free the current index block buffer */
        s_free(psiSrchIndex->psibSrchIndexBuild->pucIndexBlock);
        
        /* Set the new index block buffer length */
        psiSrchIndex->psibSrchIndexBuild->uiIndexBlockLength = uiIndexBlockLength;

        /* Allocate the new index block buffer */
        if ( (psiSrchIndex->psibSrchIndexBuild->pucIndexBlock = (unsigned char *)s_malloc((size_t)(psiSrchIndex->psibSrchIndexBuild->uiIndexBlockLength * sizeof(unsigned char)))) == NULL ) {
            return (SRCH_MemError);
        }
    }


    /* Set the index block data pointer */
    pucIndexBlockDataPtr = psiSrchIndex->psibSrchIndexBuild->pucIndexBlock + uiVariableIndexBlockDataLengthSize;

    /* Read the index blocks */
    for ( uiI = 0, psiimSrchInvertIndexMergePtr = psiimSrchInvertIndexMerge; uiI < uiSrchInvertIndexMergeLength; uiI++, psiimSrchInvertIndexMergePtr++ ) {

        /* Is this a term we are current processing and do we have to read it ? */
        if ( (psiimSrchInvertIndexMergePtr->bProcessing == true) && (psiimSrchInvertIndexMergePtr->uiIndexBlockDataLength > 0) ) {

            /* Read the index block, read to pucIndexBlockDataPtr */
            if ( psiimSrchInvertIndexMergePtr->uiIndexBlockDataLength != s_fread(pucIndexBlockDataPtr, sizeof(unsigned char), 
                    psiimSrchInvertIndexMergePtr->uiIndexBlockDataLength, psiimSrchInvertIndexMergePtr->pfFile) ) {
                return (SRCH_InvertIndexBlockReadFailed);
            }

            /* Increment the index block data pointer */
            pucIndexBlockDataPtr += psiimSrchInvertIndexMergePtr->uiIndexBlockDataLength;
        }
    }
    

    ASSERT(pucIndexBlockDataPtr <= (psiSrchIndex->psibSrchIndexBuild->pucIndexBlock + uiIndexBlockLength));
    ASSERT(pucIndexBlockDataPtr <= (psiSrchIndex->psibSrchIndexBuild->pucIndexBlock + uiVariableIndexBlockDataLengthSize + uiIndexBlockDataLength));

    
    /* Is this the final merge */
    if ( bFinalMerge == false ) {

        /* Write the index block dict entry */
        if ( (iError = iSrchInvertIndexBlockDictEntryWrite(pfOutputFile, pucTerm, uiTermType, uiTotalTermCount, uiTotalDocumentCount, bIncludeInCounts)) != SRCH_NoError ) {
            return (iError);
        }

        /* Write the size of the index block */
        pucIndexBlockPtr = psiSrchIndex->psibSrchIndexBuild->pucIndexBlock;
        UTL_NUM_WRITE_UINT(uiIndexBlockDataLength, SRCH_INVERT_INDEX_BLOCK_DATA_LENGTH_SIZE, pucIndexBlockPtr);
        ASSERT((pucIndexBlockPtr - psiSrchIndex->psibSrchIndexBuild->pucIndexBlock) == SRCH_INVERT_INDEX_BLOCK_DATA_LENGTH_SIZE);

        /* Write the index block */
        if ( uiIndexBlockLength != s_fwrite(psiSrchIndex->psibSrchIndexBuild->pucIndexBlock, sizeof(unsigned char), uiIndexBlockLength, pfOutputFile) ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to write the index block.");
            return (SRCH_InvertIndexBlockWriteFailed);
        }
    }
    else {
        
        /* Clear out the field ID bitmap */
        if ( psiSrchIndex->psibSrchIndexBuild->pucFieldIDBitmap != NULL ) {
            UTL_BITMAP_CLEAR_POINTER(psiSrchIndex->psibSrchIndexBuild->pucFieldIDBitmap, psiSrchIndex->psibSrchIndexBuild->uiFieldIDBitmapLength);
        }

        /* Get a pointer to the start of the index block data */
        pucIndexBlockDataPtr = psiSrchIndex->psibSrchIndexBuild->pucIndexBlock + SRCH_INVERT_INDEX_BLOCK_DATA_COMPRESSED_LENGTH_SIZE;
    
        /* Compress the index block, and create the field ID bitmap while we are at it */
        if ( (iError = iSrchInvertCompressIndexBlock(pucTerm, pucIndexBlockDataPtr, &uiIndexBlockDataLength, 
                psiSrchIndex->psibSrchIndexBuild->pucFieldIDBitmap, psiSrchIndex->psibSrchIndexBuild->uiFieldIDBitmapLength)) != SRCH_NoError ) {
            return (iError);
        }


        /* Get the size we will need to store the index block data length in compressed form */
        UTL_NUM_GET_COMPRESSED_UINT_SIZE(uiIndexBlockDataLength, uiIndexBlockDataLengthSize);
        ASSERT(uiIndexBlockDataLengthSize <= SRCH_INVERT_INDEX_BLOCK_DATA_COMPRESSED_LENGTH_SIZE);

        /* Get the a pointer to the place where we need to write the compressed index block data 
        ** length, this is also the start of the index block
        */
        pucIndexBlockPtr = psiSrchIndex->psibSrchIndexBuild->pucIndexBlock + (SRCH_INVERT_INDEX_BLOCK_DATA_COMPRESSED_LENGTH_SIZE - uiIndexBlockDataLengthSize);

        /* Write the size of the index block in the space we saved for it */
        UTL_NUM_WRITE_COMPRESSED_UINT(uiIndexBlockDataLength, pucIndexBlockPtr);
        ASSERT((pucIndexBlockPtr - psiSrchIndex->psibSrchIndexBuild->pucIndexBlock) == SRCH_INVERT_INDEX_BLOCK_DATA_COMPRESSED_LENGTH_SIZE);

        /* Get the a pointer to the start of the index block */
        pucIndexBlockPtr = psiSrchIndex->psibSrchIndexBuild->pucIndexBlock + (SRCH_INVERT_INDEX_BLOCK_DATA_COMPRESSED_LENGTH_SIZE - uiIndexBlockDataLengthSize);

        /* Store the block */
        if ( (iError = iUtlDataAddEntry(psiSrchIndex->pvUtlIndexData, (void *)pucIndexBlockPtr, uiIndexBlockDataLength + uiIndexBlockDataLengthSize, &ulIndexBlockObjectID)) != UTL_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to store an index block in the repository, index: '%s', utl error: %d.", psiSrchIndex->pucIndexName, iError);
            return (SRCH_InvertBlockObjectStoreFailed);
        }


        /* The index block must have been allocated now */
/*         ASSERT(ulIndexBlockObjectID > 0); */


        /* We add the term to the dictionary */
        if ( (iError = iSrchTermDictAddTerm(psiSrchIndex, pucTerm, uiTermType, uiTotalTermCount, uiTotalDocumentCount, ulIndexBlockObjectID,
                psiSrchIndex->psibSrchIndexBuild->pucFieldIDBitmap, psiSrchIndex->psibSrchIndexBuild->uiFieldIDBitmapLength)) != SRCH_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to add a term to the term dictionary, term: '%s', index: '%s', srch error: %d.", pucTerm, psiSrchIndex->pucIndexName, iError);
            return (iError);
        }


        /* Increment the term counts */
        switch ( uiTermType ) {

            case SPI_TERM_TYPE_UNKNOWN:
                break;

            case SPI_TERM_TYPE_REGULAR:
                if ( bIncludeInCounts == true ) {
                    psiSrchIndex->ulTotalTermCount += uiTotalTermCount;
                    psiSrchIndex->ulUniqueTermCount++;
                }
                break;
            
            case SPI_TERM_TYPE_STOP:
                psiSrchIndex->ulTotalStopTermCount += uiTotalTermCount;
                psiSrchIndex->ulUniqueStopTermCount++;
                break;
            
            default:
                break;
        }
    }


    return (SRCH_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchInvertCompressIndexBlock()

    Purpose:    Compresses a index block. 

    Parameters: pucIndexBlock           index block
                puiIndexBlockLength     pointer to the index block length, also a return 
                                        pointer for the new length
                pucFieldIDBitmap        field ID bitmap (optional)
                uiFieldIDBitmapLength   field ID bitmap length (optional)

    Globals:    none

    Returns:    SRCH error code

*/
static int iSrchInvertCompressIndexBlock
(
    unsigned char *pucTerm,
    unsigned char *pucIndexBlock,
    unsigned int *puiIndexBlockLength,
    unsigned char *pucFieldIDBitmap,
    unsigned int uiFieldIDBitmapLength
)
{
    
    unsigned char   *pucReadPtr = NULL;
    unsigned char   *pucWritePtr = NULL;

    unsigned int    uiIndexEntryDocumentID = 0;
    unsigned int    uiIndexEntryTermPosition = 0;
    unsigned int    uiIndexEntryFieldID = 0;
/*     unsigned int    uiIndexEntryTermWeight = 0; */

    unsigned int    uiIndexEntryPreviousDocumentID = 0;
    unsigned int    uiIndexEntryDeltaDocumentID = 0;

    unsigned int    uiIndexEntryPreviousTermPosition = 0;
    unsigned int    uiIndexEntryDeltaTermPosition = 0;


    ASSERT(pucIndexBlock != NULL);
    ASSERT(puiIndexBlockLength != NULL);
    ASSERT(((pucFieldIDBitmap == NULL) && (uiFieldIDBitmapLength <= 0)) || ((pucFieldIDBitmap != NULL) && (uiFieldIDBitmapLength > 0)));


    /* We only need to copy blocks which have a length greater than */
    if ( *puiIndexBlockLength == 0 ) {
        return (SRCH_NoError);
    }


    /* Initialize the pointers */
    pucReadPtr = pucIndexBlock;
    pucWritePtr = pucIndexBlock;


    /* Loop while there is stuff to scan */
    while ( pucReadPtr < (pucIndexBlock + (*puiIndexBlockLength)) ) {

/* Compressed int */
        /* Read the index entry */
        UTL_NUM_READ_COMPRESSED_UINT(uiIndexEntryDocumentID, pucReadPtr);
        UTL_NUM_READ_COMPRESSED_UINT(uiIndexEntryTermPosition, pucReadPtr);
        UTL_NUM_READ_COMPRESSED_UINT(uiIndexEntryFieldID, pucReadPtr);
/*         UTL_NUM_READ_COMPRESSED_UINT(uiIndexEntryTermWeight, pucReadPtr); */


/* Varint */
        /* Read the index entry */
/*         UTL_NUM_READ_VARINT_TRIO(uiIndexEntryDocumentID, uiIndexEntryTermPosition, uiIndexEntryFieldID, pucReadPtr); */
/*         UTL_NUM_READ_VARINT_QUAD(uiIndexEntryDocumentID, uiIndexEntryTermPosition, uiIndexEntryFieldID, uiIndexEntryTermWeight, pucReadPtr); */

/* printf("pucTerm: [%s][%u][%u][%u][%u]\n", pucTerm, uiIndexEntryDocumentID, uiIndexEntryTermPosition, uiIndexEntryFieldID); */
/* printf("pucTerm: [%s][%u][%u][%u][%u]\n", pucTerm, uiIndexEntryDocumentID, uiIndexEntryTermPosition, uiIndexEntryFieldID, uiIndexEntryTermWeight); */


        ASSERT(uiIndexEntryDocumentID > 0);
        ASSERT(uiIndexEntryTermPosition >= 0);
        ASSERT((uiIndexEntryFieldID >= 0) && (uiIndexEntryFieldID <= uiFieldIDBitmapLength));


        /* Set the field ID in the field ID bitmap - field ID 0 is not a field */
        if ( (pucFieldIDBitmap != NULL) && (uiIndexEntryFieldID > 0) ) {
            UTL_BITMAP_SET_BIT_IN_POINTER(pucFieldIDBitmap, uiIndexEntryFieldID - 1);
        }

        ASSERT(pucReadPtr > pucWritePtr);
        ASSERT(uiIndexEntryDocumentID >= uiIndexEntryPreviousDocumentID);


        /* We store a delta for the document ID if this is a new document,
        ** existing documents get a delta document ID of 0
        **
        ** We store the term position if this is a new document,
        ** existing documents get a delta term position
        */
        if ( uiIndexEntryDocumentID > uiIndexEntryPreviousDocumentID ) {

            /* Work out the delta document ID and save the document ID */
            uiIndexEntryDeltaDocumentID = uiIndexEntryDocumentID - uiIndexEntryPreviousDocumentID;
            uiIndexEntryPreviousDocumentID = uiIndexEntryDocumentID;

            /* Set the delta term position and save the term position */
            uiIndexEntryDeltaTermPosition = uiIndexEntryTermPosition;
            uiIndexEntryPreviousTermPosition = uiIndexEntryTermPosition;

            ASSERT(uiIndexEntryDeltaDocumentID > 0);
            ASSERT(uiIndexEntryDeltaTermPosition >= 0);
            ASSERT(uiIndexEntryPreviousTermPosition >= 0);
        }
        
        /* Same document */
        else {

            /* Set the document ID delta to 0 */
            uiIndexEntryDeltaDocumentID = 0;

            /* Work out the delta term position and save the term position */
            uiIndexEntryDeltaTermPosition = uiIndexEntryTermPosition - uiIndexEntryPreviousTermPosition;
            uiIndexEntryPreviousTermPosition = uiIndexEntryTermPosition;

            ASSERT(uiIndexEntryDeltaTermPosition >= 0);
            ASSERT(uiIndexEntryDeltaTermPosition <= uiIndexEntryTermPosition);
            ASSERT(uiIndexEntryPreviousTermPosition >= 0);
            ASSERT(uiIndexEntryPreviousTermPosition <= uiIndexEntryTermPosition);
        }


/* Compressed int */
        /* Write out the index block */
        UTL_NUM_WRITE_COMPRESSED_UINT(uiIndexEntryDeltaDocumentID, pucWritePtr);
        UTL_NUM_WRITE_COMPRESSED_UINT(uiIndexEntryDeltaTermPosition, pucWritePtr);
        UTL_NUM_WRITE_COMPRESSED_UINT(uiIndexEntryFieldID, pucWritePtr);
/*         UTL_NUM_WRITE_COMPRESSED_UINT(uiIndexEntryTermWeight, pucWritePtr); */


/* Varint */
        /* Write out the index block */
/*         UTL_NUM_WRITE_COMPACT_VARINT_TRIO(uiIndexEntryDeltaDocumentID, uiIndexEntryDeltaTermPosition, uiIndexEntryFieldID, pucWritePtr); */
/*         UTL_NUM_WRITE_COMPACT_VARINT_QUAD(uiIndexEntryDeltaDocumentID, uiIndexEntryDeltaTermPosition, uiIndexEntryFieldID, uiIndexEntryTermWeight, pucWritePtr); */


        /* Make sure we are not getting ahead of ourselves */
        ASSERT(pucReadPtr >= pucWritePtr);
    }


    /* Set the return pointer */
    *puiIndexBlockLength = pucWritePtr - pucIndexBlock;


    return (SRCH_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   pucSrchInvertPrettyPrintFileNumbers()

    Purpose:    This function formats the merge index file number for printing

    Parameters: psiSrchIndex        search index structure
                uiStartVersion      start level
                uiEndVersion        end level
                pucBuffer           return buffer
                uiBufferLength      return buffer length

    Globals:    none

    Returns:    a pointer to the return buffer

*/
static unsigned char *pucSrchInvertPrettyPrintFileNumbers
(
    struct srchIndex *psiSrchIndex,
    unsigned int uiStartVersion,
    unsigned int uiEndVersion,
    unsigned char *pucBuffer,
    unsigned int uiBufferLength
)
{

    int             iError = SRCH_NoError;
    unsigned int    uiI = 0, uiJ = 0;
    boolean         bExtent = false, bComma = false;
    unsigned char   pucFilePath[UTL_FILE_PATH_MAX + 1] = {'\0'};
    unsigned char   pucLocalBuffer[UTL_FILE_PATH_MAX + 1] = {'\0'};


    ASSERT(psiSrchIndex != NULL);
    ASSERT(uiStartVersion >= 0);
    ASSERT(uiEndVersion >= 0);
    ASSERT(uiEndVersion >= uiStartVersion);
    ASSERT(pucBuffer != NULL);
    ASSERT(uiBufferLength > 0);


    /* Handle the case where there is only one file */
    if ( uiStartVersion == uiEndVersion ) {
        snprintf(pucBuffer, uiBufferLength, "%u", uiStartVersion);
        return (pucBuffer);
    }


    /* Create the merge info buffer */
    for ( uiI = uiStartVersion, uiJ = uiStartVersion, pucBuffer[0] = '\0'; uiI <= uiEndVersion; uiI++ ) {

        /* Create the file path */
        if ( (iError = iSrchFilePathsGetTempTermDictionaryFilePathFromIndex(psiSrchIndex, uiI, false, pucFilePath, UTL_FILE_PATH_MAX + 1)) != SRCH_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to create the index file path, index: '%s', srch error: %d.", psiSrchIndex->pucIndexName, iError);
            return (NULL);
        } 

        if ( bUtlFileIsFile(pucFilePath) == true) {
            if ( (uiI - uiJ) > 1 ) {
                if ( bExtent == true ) {
                    snprintf(pucLocalBuffer, UTL_FILE_PATH_MAX + 1, " - %u", uiJ);
                    s_strnncat(pucBuffer, pucLocalBuffer, uiBufferLength, uiBufferLength + 1);
                    bExtent = false;
                }
                else {
                    snprintf(pucLocalBuffer, UTL_FILE_PATH_MAX + 1, "%s%u", (bComma == true) ? ", " : "", uiJ);
                    s_strnncat(pucBuffer, pucLocalBuffer, uiBufferLength, uiBufferLength + 1);
                    bComma = true;
                }
            }
            else if ( (uiI - uiJ) == 1 ) {
                if ( bExtent == false ) {
                    snprintf(pucLocalBuffer, UTL_FILE_PATH_MAX + 1, "%s%u", (bComma == true) ? ", " : "", uiJ);
                    s_strnncat(pucBuffer, pucLocalBuffer, uiBufferLength, uiBufferLength + 1);
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
            s_strnncat(pucBuffer, pucLocalBuffer, uiBufferLength, uiBufferLength + 1);
        }
        else {
            snprintf(pucLocalBuffer, UTL_FILE_PATH_MAX + 1, "%s%u", (bComma == true) ? ", " : "", uiJ);
            s_strnncat(pucBuffer, pucLocalBuffer, uiBufferLength, uiBufferLength + 1);
        }
    }


    return (pucBuffer);

}


/*---------------------------------------------------------------------------*/


