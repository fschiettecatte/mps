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

    Module:     indexer.c

    Author:     Francois Schiettecatte (FS Consulting LLC.)

    Created:    20 February 1994

    Purpose:    This module implements the interface into the indexer.

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
#define UTL_LOG_CONTEXT                     (unsigned char *)"com.fsconsult.mps.src.search.indexer"


/* Enable this to require document title */
/* #define SRCH_INDEX_ENABLE_DOCUMENT_TITLE_REQUIRED */

/* Enable this to require document items */
/* #define SRCH_INDEX_ENABLE_DOCUMENT_ITEMS_REQUIRED */

/* Enable this to error out on invalid lines */
/* #define SRCH_INDEX_ENABLE_ERROR_ON_INVALID_LINES */

/* Enable this to warn on invalid tags, default is to ignore them */
/* #define SRCH_INDEX_ENABLE_WARNING_ON_INVALID_TAGS */


/*---------------------------------------------------------------------------*/


/*
** Defines
*/

/* Mime header tags */
#define SRCH_INDEX_MIME_CONTENT_TYPE_TAG        (unsigned char *)"content-type:"
#define SRCH_INDEX_MIME_CONTENT_LENGTH_TAG      (unsigned char *)"content-length:"


/*---------------------------------------------------------------------------*/


/*
** Structures
*/

/* Search indexer item structure */
struct srchIndexerItem {
    unsigned char   pucItemName[SPI_ITEM_NAME_MAXIMUM_LENGTH + 1];                      /* Item name */
    unsigned char   pucMimeType[SPI_MIME_TYPE_MAXIMUM_LENGTH + 1];                      /* Mime type */
};


/* Search indexer field structure */
struct srchIndexerField {
    unsigned char   pucFieldName[SPI_FIELD_NAME_MAXIMUM_LENGTH + 1];                    /* Field name */
    unsigned char   pucFieldDescription[SPI_FIELD_DESCRIPTION_MAXIMUM_LENGTH + 1];      /* Field description */
    unsigned int    uiFieldType;                                                        /* Field type */
    unsigned int    uiFieldOptions;                                                     /* Field options */
};


/*---------------------------------------------------------------------------*/


/*
** Private function prototypes
*/

static int iSrchIndexerIndexCreate (struct srchIndexer *psiSrchIndexer, 
        struct srchIndex **ppsiSrchIndex);

static int iSrchIndexerIndexClose (struct srchIndexer *psiSrchIndexer, 
        struct srchIndex *psiSrchIndex);

static int iSrchIndexerIndexAbort (struct srchIndexer *psiSrchIndexer, 
        struct srchIndex *psiSrchIndex);


static int iSrchIndexerParseVersionInformation (struct srchIndexer *psiSrchIndexer);

static int iSrchIndexerParseIndexInformation (struct srchIndexer *psiSrchIndexer);

static int iSrchIndexerParseLanguageInformation (struct srchIndexer *psiSrchIndexer);

static int iSrchIndexerParseIndexStream (struct srchIndexer *psiSrchIndexer, 
        struct srchIndex *psiSrchIndex);


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchIndexerCreateIndexFromSearchIndexer()

    Purpose:    This function will index a index reading the structured
                index stream from the file descriptor.

    Parameters: psiSrchIndexer      indexer profile

    Globals:    none

    Returns:    SRCH error code

*/
int iSrchIndexerCreateIndexFromSearchIndexer
(
    struct srchIndexer *psiSrchIndexer
)
{

    int                 iError = SRCH_NoError;
    struct srchIndex    *psiSrchIndex = NULL;

    time_t              tStartTime = (time_t)0;
    time_t              tEndTime = (time_t)0;

    unsigned int        uiTimeTaken = 0;
    unsigned int        uiHours = 0;
    unsigned int        uiMinutes = 0;
    unsigned int        uiSeconds = 0;


    /* Check the parameters */
    if ( psiSrchIndexer == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'psiSrchIndexer' parameter passed to 'iSrchIndexerCreateIndexFromSearchIndexer'."); 
        return (SRCH_IndexerInvalidIndexer);
    }


    /* Start the timer */
    tStartTime = s_time(NULL);


    /* Get and check the version information */
    if ( (iError = iSrchIndexerParseVersionInformation(psiSrchIndexer)) != SRCH_NoError ) {
        return (iError); 
    }


    /* Get and check the index information */
    if ( (iError = iSrchIndexerParseIndexInformation(psiSrchIndexer)) != SRCH_NoError ) {
        return (iError);     
    }


    /* Get and check the language information */
    if ( (iError = iSrchIndexerParseLanguageInformation(psiSrchIndexer)) != SRCH_NoError ) {
        return (iError);     
    }


    /* Create the index */
    if ( (iError = iSrchIndexerIndexCreate(psiSrchIndexer, &psiSrchIndex)) != SRCH_NoError ) {
        return (iError);     
    }


    /* Read the stream and index it */
    if ( (iError = iSrchIndexerParseIndexStream(psiSrchIndexer, psiSrchIndex)) != SRCH_NoError ) {
        
        /* Abort the index */
        iSrchIndexerIndexAbort(psiSrchIndexer, psiSrchIndex);
        return (iError);     
    }


    /* Close the index */
    if ( (iError = iSrchIndexerIndexClose(psiSrchIndexer, psiSrchIndex)) != SRCH_NoError ) {
        return (iError);     
    }



    /* Stop the timer and work out how long the job took */
    tEndTime = s_time(NULL);

    uiTimeTaken = s_difftime(tEndTime, tStartTime);
    uiHours = uiTimeTaken / (60 * 60);
    uiMinutes = (uiTimeTaken - (uiHours * 60 * 60)) / 60;
    uiSeconds = uiTimeTaken - ((uiTimeTaken / 60) * 60);

    if ( uiHours > 0 ) {
        iUtlLogInfo(UTL_LOG_CONTEXT, "Indexing time for this index: %u hour%s, %u minute%s and %u second%s.", 
                uiHours, (uiHours == 1) ? "" : "s", uiMinutes, (uiMinutes == 1) ? "" : "s", uiSeconds, (uiSeconds == 1) ? "" : "s");
    }
    else if ( uiMinutes > 0 ) {
        iUtlLogInfo(UTL_LOG_CONTEXT, "Indexing time for this index: %u minute%s and %u second%s.", 
                uiMinutes, (uiMinutes == 1) ? "" : "s", uiSeconds, (uiSeconds == 1) ? "" : "s");
    }
    else {
        iUtlLogInfo(UTL_LOG_CONTEXT, "Indexing time for this index: %u second%s.", uiSeconds, (uiSeconds == 1) ? "" : "s");
    }


    return (SRCH_NoError);

}


/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchIndexerIndexCreate()

    Purpose:    This function sets up the index for indexing. It will check
                all the parameters and correct them if possible. It will then
                open the index and make it ready for indexing.

                It will also log any pertinent information about the set up 
                process.

    Parameters: psiSrchIndexer      search indexer structure
                ppsiSrchIndex       return pointer for the index structure

    Globals:    none

    Returns:    SRCH error code

*/
static int iSrchIndexerIndexCreate
(
    struct srchIndexer *psiSrchIndexer,
    struct srchIndex **ppsiSrchIndex
)
{

    int     iError = SRCH_NoError;


    ASSERT(psiSrchIndexer != NULL);
    ASSERT(ppsiSrchIndex != NULL);


    /* Create the index */
    iUtlLogInfo(UTL_LOG_CONTEXT, "Starting to build index: '%s'.", psiSrchIndexer->pucIndexName);
    if ( (iError = iSrchIndexOpen(psiSrchIndexer->pucIndexDirectoryPath, psiSrchIndexer->pucConfigurationDirectoryPath, 
            psiSrchIndexer->pucIndexName, SRCH_INDEX_INTENT_CREATE, ppsiSrchIndex)) != SRCH_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to create the index: '%s', srch error: %d.", psiSrchIndexer->pucIndexName, iError);
        return (iError);     
    }


    /* Initialize the index for adding documents */
    if ( (iError = iSrchInvertInit(*ppsiSrchIndex, psiSrchIndexer->pucLanguageCode, psiSrchIndexer->pucTokenizerName, 
                psiSrchIndexer->pucStemmerName, psiSrchIndexer->pucStopListName, psiSrchIndexer->pucStopListFilePath, 
                psiSrchIndexer->uiIndexerMemorySizeMaximum, psiSrchIndexer->uiTermLengthMinimum, psiSrchIndexer->uiTermLengthMaximum, 
                psiSrchIndexer->pucTemporaryDirectoryPath)) != SRCH_NoError ) {
        
        /* Abort the index */
        iSrchIndexAbort(*ppsiSrchIndex, psiSrchIndexer->pucConfigurationDirectoryPath);
        *ppsiSrchIndex = NULL;

        iUtlLogError(UTL_LOG_CONTEXT, "Failed to initialize the inverter, index: '%s', srch error: %d.", psiSrchIndexer->pucIndexName, iError);
        return (iError);     
    }


    /* The index is now ready to index documents */


    return (SRCH_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchIndexerIndexClose()

    Purpose:    This function will make sure that we have finished adding
                documents to the index, close it and rename it if it was
                indexed in the background

                It will also log any pertinent information about the shut down 
                process.

    Parameters: psiSrchIndexer      search indexer structure
                psiSrchIndex        search index structure

    Globals:    none

    Returns:    SRCH error code

*/
static int iSrchIndexerIndexClose
(
    struct srchIndexer *psiSrchIndexer,
    struct srchIndex *psiSrchIndex
)
{

    int     iError = SRCH_NoError;


    ASSERT(psiSrchIndex != NULL);
    ASSERT(psiSrchIndexer != NULL);


    /* Finish inverting */
    if ( (iError = iSrchInvertFinish(psiSrchIndex)) != SRCH_NoError ) {

        iUtlLogError(UTL_LOG_CONTEXT, "Failed to finish inverting, index: '%s', srch error: %d.", psiSrchIndex->pucIndexName, iError);

        /* Abort the index */
        iSrchIndexerIndexAbort(psiSrchIndexer, psiSrchIndex);
        psiSrchIndex = NULL;

        return (iError);     
    }
    
    
    /* Close the index */
    iSrchIndexClose(psiSrchIndex);
    psiSrchIndex = NULL;


    iUtlLogInfo(UTL_LOG_CONTEXT, "Finished indexing.");


    return (SRCH_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchIndexerIndexAbort()

    Purpose:    This function will make sure that we have finished adding
                documents to the index, close it and rename it if it was
                indexed in the background

                It will also log any pertinent information about the shut down 
                process.

    Parameters: psiSrchIndexer      search indexer structure
                psiSrchIndex        search index structure (optional)

    Globals:    none

    Returns:    SRCH error code

*/
static int iSrchIndexerIndexAbort
(
    struct srchIndexer *psiSrchIndexer,
    struct srchIndex *psiSrchIndex
)
{


    ASSERT(psiSrchIndexer != NULL);


    iUtlLogError(UTL_LOG_CONTEXT, "Indexing aborted, index: '%s'.", 
            ((psiSrchIndex != NULL) && (psiSrchIndex->pucIndexName != NULL)) ? psiSrchIndex->pucIndexName : (unsigned char *)"(undefined)");


    /* Abort inverting */
    iSrchInvertAbort(psiSrchIndex);


    /* Abort indexing */
    if ( psiSrchIndex != NULL ) {
        iSrchIndexAbort(psiSrchIndex, psiSrchIndexer->pucConfigurationDirectoryPath);
        psiSrchIndex = NULL;
    }


    return (SRCH_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchIndexerParseVersionInformation()

    Purpose:    Reads one line from the stream specified in the search indexer
                structures, check that is the version information and that 
                this version information is correct

    Parameters: psiSrchIndexer      Search indexer structure

    Globals:    none

    Returns:    SRCH error code

*/
static int iSrchIndexerParseVersionInformation
(
    struct srchIndexer *psiSrchIndexer
)
{

    unsigned char   pucLine[BUFSIZ + 1] = {'\0'};
    unsigned int    uiMajorVersion = 0;
    unsigned int    uiMinorVersion = 0;


    ASSERT(psiSrchIndexer != NULL);


    /* Version Number
    **
    **  V major_version{N} minor_version{N}
    **
    */ 


    /* Read the line, bail on error */
    if ( s_fgets(pucLine, BUFSIZ, psiSrchIndexer->pfFile) == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to read index stream for version version number line.");
        return (SRCH_IndexerReadFailed);
    }

    /* Version Number */
    if ( pucLine[0] != 'V' ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to get a valid version number line, line text: '%s'.", pucLine);
        return (SRCH_IndexerVersionFailed);
    }

    /* Scan for the version numbers, no need for a scan string here are these are just numbers */
    if ( sscanf(pucLine + 2, "%u %u", &uiMajorVersion, &uiMinorVersion) != 2 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Missing number in version number line, line text: '%s'.", pucLine);
        return (SRCH_IndexerInvalidVersion);
    }


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "Major version: %u, minor version: %u.", uiMajorVersion, uiMinorVersion); */

    
    /* Check for == major version and <= minor version */
    if ( (uiMajorVersion == UTL_VERSION_MAJOR) && (uiMinorVersion <= UTL_VERSION_MINOR) ) {
        return (SRCH_NoError);
    }

    /* Check for < major version */
    else if ( uiMajorVersion < UTL_VERSION_MAJOR ) {
        iUtlLogWarn(UTL_LOG_CONTEXT, "Old index stream version, got: %u.%u, and expected: %u.%u, indexing anyway.", uiMajorVersion, uiMinorVersion, UTL_VERSION_MAJOR, UTL_VERSION_MINOR);
        return (SRCH_NoError);
    }
    
    /* Anything else */
    iUtlLogError(UTL_LOG_CONTEXT, "Failed to match version, got: %u.%u, and expected: %u.%u.", uiMajorVersion, uiMinorVersion, UTL_VERSION_MAJOR, UTL_VERSION_MINOR);


    return (SRCH_IndexerInvalidVersion);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchIndexerParseIndexInformation()

    Purpose:    Reads one line from the stream specified in the search indexer
                structures, check that is the name information and that 
                this name information is correct

    Parameters: psiSrchIndexer      Search indexer structure

    Globals:    none

    Returns:    SRCH error code

*/
static int iSrchIndexerParseIndexInformation
(
    struct srchIndexer *psiSrchIndexer
)
{

    unsigned char   pucScanfFormatL[UTL_STRING_SCANF_FORMAT_LENGTH + 1] = {'\0'};
    unsigned char   pucLine[BUFSIZ + 1] = {'\0'};
    unsigned char   pucIndexName[SPI_INDEX_NAME_MAXIMUM_LENGTH + 1] = {'\0'};
    unsigned char   pucIndexDescription[SPI_INDEX_DESCRIPTION_MAXIMUM_LENGTH + 1] = {'\0'};


    ASSERT(psiSrchIndexer != NULL);


    /* Index Name/Description
    **
    **  N index_name{A} index_description{A}
    **
    */ 


    /* Read the line, bail on error */
    if ( s_fgets(pucLine, BUFSIZ, psiSrchIndexer->pfFile) == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to read index stream for index language line.");
        return (SRCH_IndexerReadFailed);
    }
    
    /* Index name */
    if ( pucLine[0] != 'N' ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to get a index name line, line text: '%s'.", pucLine);
        return (SRCH_IndexerIndexNameFailed);
    }


    /* Prepare the scan string */
    snprintf(pucScanfFormatL, UTL_STRING_SCANF_FORMAT_LENGTH + 1, "%%%ds %%%ds", 
            SPI_INDEX_NAME_MAXIMUM_LENGTH, SPI_INDEX_DESCRIPTION_MAXIMUM_LENGTH);

    /* Scan the line for the index name entry */
    if ( sscanf(pucLine + 2, pucScanfFormatL, pucIndexName, pucIndexDescription) < 1 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Missing elements in index name, line text: '%s'.", pucLine);
        return (SRCH_IndexerInvalidIndexName);
    }

/*     iUtlLogDebug(UTL_LOG_CONTEXT, "pucIndexName: '%s', pucIndexDescription: '%s'.", pucIndexName, pucIndexDescription); */


    return (SRCH_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchIndexerParseLanguageInformation()

    Purpose:    Reads one line from the stream specified in the search indexer
                structures, check that is the language information and that 
                this language information is correct

    Parameters: psiSrchIndexer      Search indexer structure

    Globals:    none

    Returns:    SRCH error code

*/
static int iSrchIndexerParseLanguageInformation
(
    struct srchIndexer *psiSrchIndexer
)
{

    int             iError = UTL_NoError;
    unsigned char   pucScanfFormatL[UTL_STRING_SCANF_FORMAT_LENGTH + 1] = {'\0'};
    unsigned char   pucLine[BUFSIZ + 1] = {'\0'};
    unsigned char   pucLanguageCode[SPI_INDEX_LANGUAGE_MAXIMUM_LENGTH + 1] = {'\0'};
    unsigned char   pucTokenizerName[SPI_INDEX_TOKENIZER_MAXIMUM_LENGTH + 1] = {'\0'};


    ASSERT(psiSrchIndexer != NULL);


    /* Language
    **
    **  L language{A} character_set{A} tokenizer{A}
    **
    */ 


    /* Read the line, bail on error */
    if ( s_fgets(pucLine, BUFSIZ, psiSrchIndexer->pfFile) == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to read index stream for index language line.");
        return (SRCH_IndexerReadFailed);
    }
    
    /* Index language */
    if ( pucLine[0] != 'L' ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to get a index language line, line text: '%s'.", pucLine);
        return (SRCH_IndexerLanguageFailed);
    }


    /* Prepare the scan string */
    snprintf(pucScanfFormatL, UTL_STRING_SCANF_FORMAT_LENGTH + 1, "%%%ds %%%ds", 
            SPI_INDEX_LANGUAGE_MAXIMUM_LENGTH, SPI_INDEX_TOKENIZER_MAXIMUM_LENGTH);

    /* Scan the line for the index language entry */
    if ( sscanf(pucLine + 2, pucScanfFormatL, pucLanguageCode, pucTokenizerName) != 2 ) {
        iUtlLogWarn(UTL_LOG_CONTEXT, "Missing elements in index language, line text: '%s'.", pucLine);
        return (SRCH_IndexerInvalidLanguage);
    }


    /* Check the language code */
    if ( (iError = iLngCheckLanguageCode(pucLanguageCode)) != UTL_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid language code: '%s', utl error: %d.", pucLanguageCode, iError);
        return (SRCH_IndexerInvalidLanguageCode);
    }

    /* Check the tokenizer name */
    if ( (iError = iLngCheckTokenizerName(pucTokenizerName)) != UTL_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid tokenizer name: '%s', utl error: %d.", pucTokenizerName, iError);
        return (SRCH_IndexerInvalidTokenizerName);
    }


    /* Hand over the names */
    if ( (psiSrchIndexer->pucLanguageCode = s_strdup(pucLanguageCode)) == NULL ) {
        return (SRCH_MemError);
    }
    
    if ( (psiSrchIndexer->pucTokenizerName = s_strdup(pucTokenizerName)) == NULL ) {
        return (SRCH_MemError);
    }


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "Language Code: '%s', Tokenizer Name: '%s'.", */
/*             pucLanguageCode, pucTokenizerName); */


    return (SRCH_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchIndexerParseIndexStream()

    Purpose:    Indexes the passed structured index stream into the index

    Parameters: psiSrchIndexer      Search indexer structure
                psiSrchIndex        search index structure

    Globals:    none

    Returns:    SRCH error code

*/
static int iSrchIndexerParseIndexStream
(
    struct srchIndexer *psiSrchIndexer,
    struct srchIndex *psiSrchIndex
)
{
    int                         iError = SRCH_NoError;

    unsigned int                uiI = 0;
    unsigned int                uiLineCount = 3;                /* We have already read three lines, the version line, the index name line and the language line */

    unsigned char               pucScanfFormatI[UTL_STRING_SCANF_FORMAT_LENGTH + 1] = {'\0'};

    unsigned char               *pucLine = NULL;
    unsigned char               *pucLinePtr = NULL;
    unsigned char               *pucLineStrtokPtr = NULL;
    unsigned int                uiLineCapacity = 0;
    unsigned int                uiLineLength = 0;

    unsigned char               pucUnfieldedSearchFieldNames[SRCH_INFO_SYMBOL_MAXIMUM_LENGTH + 1] = {'\0'};

    unsigned char               pucDocumentTitle[SPI_TITLE_MAXIMUM_LENGTH + 1] = {'\0'};
    unsigned char               pucDocumentKey[SPI_DOCUMENT_KEY_MAXIMUM_LENGTH + 1] = {'\0'};

    unsigned int                uiDocumentID = 0;
    unsigned int                uiDocumentRank = 0;
    unsigned int                uiDocumentTermCount = 0;
    unsigned long               ulDocumentAnsiDate = 0;

    unsigned int                uiTermCount = 0;
    boolean                     bDocumentTermCount = false;
    unsigned int                uiPreviousTermPosition = 0;

    struct srchIndexerField     *psifSrchIndexerFields = NULL;
    unsigned int                uiSrchIndexerFieldsLength = 0;
    struct srchIndexerField     *psifSrchIndexerFieldsPtr = NULL;


    struct srchIndexerItem      *psiiSrchIndexerItems = NULL;
    unsigned int                uiSrchItemIndexerItemsLength = 0;
    struct srchIndexerItem      *psiiSrchIndexerItemsPtr = NULL;

    struct srchDocumentItem     *psdiSrchDocumentItems = NULL;
    unsigned int                uiSrchDocumentItemsCapacity = 0;
    unsigned int                uiSrchDocumentItemsLength = 0;
    struct srchDocumentItem     *psdiSrchDocumentItemsPtr = NULL;

    unsigned int                uiDocumentLanguageID = LNG_LANGUAGE_ANY_ID;
    unsigned int                uiCurrentLanguageID = LNG_LANGUAGE_ANY_ID;
    unsigned int                uiDefaultLanguageID = LNG_LANGUAGE_ANY_ID;

    boolean                     bReadLine = true;



    ASSERT(psiSrchIndexer != NULL);
    ASSERT(psiSrchIndex != NULL);



    /* Prepare the scan format */
    snprintf(pucScanfFormatI, UTL_STRING_SCANF_FORMAT_LENGTH + 1, "%%%ds %%%ds %%u %%%ds %%ld %%ld %%%ds", 
            SPI_ITEM_NAME_MAXIMUM_LENGTH, SPI_MIME_TYPE_MAXIMUM_LENGTH, UTL_FILE_PATH_MAX, SPI_URL_MAXIMUM_LENGTH);



    /* Set the current language ID from the language code stored in the search indexer structure */
    if ( (iError = iLngGetLanguageIDFromCode(psiSrchIndexer->pucLanguageCode, &uiCurrentLanguageID)) != LNG_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the language ID from the language code: '%s', lng error: %d.", psiSrchIndexer->pucLanguageCode, iError); 
        iError = SRCH_IndexerInvalidLanguageCode;
        goto bailFromiSrchIndexParseIndexStream;
    }
    
    /* And set the default language ID and the document language ID */
    uiDefaultLanguageID = uiCurrentLanguageID;
    uiDocumentLanguageID = uiCurrentLanguageID;



    /* Allocate the initial field name list - this is for field 0 */
    if ( (psifSrchIndexerFields = (struct srchIndexerField *)s_malloc((size_t)(sizeof(struct srchIndexerField) * (uiSrchIndexerFieldsLength + 1)))) == NULL ) {
        iError = SRCH_MemError;
        goto bailFromiSrchIndexParseIndexStream;
    }
    
    /* Increment the field length */
    uiSrchIndexerFieldsLength++;

    /* Set the field option defaults */
    iSrchInfoGetFieldOptionDefaults(psiSrchIndex, &psifSrchIndexerFields->uiFieldOptions);


    
    /* Loop forever */
    while ( true ) {

        /* Reset the line if we are due to read a new one */
        if ( bReadLine == true ) {
            if ( pucLine != NULL ) {
                pucLine[0] = '\0';
            }
            
            uiLineLength = 0;
        }

        /* Read while there is data to be read, ie until we hit a new-line of some sort */
        while ( true ) {

            /* Allocate/reallocate the line if needed */
            if ( (pucLine == NULL) || (uiLineCapacity == 0) || ((uiLineLength + BUFSIZ) > uiLineCapacity) ) {

                /* Reallocate extending by BUFSIZ */
                if ( (pucLinePtr = (unsigned char *)s_realloc(pucLine, (size_t)(sizeof(unsigned char) * (uiLineCapacity + BUFSIZ)))) == NULL ) {
                    iError = SRCH_MemError;
                    goto bailFromiSrchIndexParseIndexStream;
                }

                /* Hand over the pointer */
                pucLine = pucLinePtr;

                /* Clear the new allocation */
                s_memset(pucLine + uiLineCapacity, 0, BUFSIZ);

                /* Set the new line length */
                uiLineCapacity += BUFSIZ;
            }


            /* Read the next line chunk, appending to the current line */
            if ( bReadLine == true ) {
                
                /* Read the line */
                if ( s_fgets(pucLine + uiLineLength, uiLineCapacity - uiLineLength, psiSrchIndexer->pfFile) == NULL ) {

                    /* Check if we got an error */
                    if ( s_ferror(psiSrchIndexer->pfFile) != 0 ) {
                        iUtlLogError(UTL_LOG_CONTEXT, "Failed to read from the parser, line: %u, line text: '%s'.", uiLineCount, pucLine);
                        iError = SRCH_IndexerReadFailed;
                        goto bailFromiSrchIndexParseIndexStream;
                    }
                    
                    /* Check if we have hit the end of the file */
                    else if ( s_feof(psiSrchIndexer->pfFile) != 0 ) {
                        
                        /* No text so force an end of stream, and break */
                        if ( s_strlen(pucLine) == 0 ) {
                            s_strcpy(pucLine, "Z");
                        }

                        break;
                    }
                }
            }
            else {
                /* Make sure we read the next line */
                bReadLine = true;
            }


            /* Get the new line length here for optimization */
            uiLineLength = s_strlen(pucLine);

            /* Erase the trailing new line - be platform sensitive, handle PC first, then Unix and Mac  */
            if ( (uiLineLength >= 2) && (pucLine[uiLineLength - 2] == '\r') ) {
                uiLineLength -= 2;
                pucLinePtr = pucLine + uiLineLength;
            }
            else if ( (uiLineLength >= 1) && ((pucLine[uiLineLength - 1] == '\n') || (pucLine[uiLineLength - 1] == '\r')) ) {
                uiLineLength -= 1;
                pucLinePtr = pucLine + uiLineLength;
            }
            else {
                pucLinePtr = NULL;
            }


            /* See if we found a trailing new line */
            if ( pucLinePtr != NULL ) {
                
                /* Erase it */
                *pucLinePtr = '\0';
        
                /* Increment the line count */
                uiLineCount++;
                
                /* We have read the line, so we break out */
                break;
            }

        }    /* while ( true ) */


/*         iUtlLogDebug(UTL_LOG_CONTEXT, "'%s' %u", pucLine, uiLineLength); */

        /* Check the line */
        if ( (pucLine[0] != 'E') && (pucLine[0] != 'Z') && (pucLine[1] != ' ') ) {
#if defined(SRCH_INDEX_ENABLE_ERROR_ON_INVALID_LINES)
            iUtlLogError(UTL_LOG_CONTEXT, "Invalid tag, line: %u, line text: '%s'.", uiLineCount, pucLine);
            iError = SRCH_IndexerInvalidTag;
            goto bailFromiSrchIndexParseIndexStream;
#else
            iUtlLogWarn(UTL_LOG_CONTEXT, "Invalid tag, line: %u, line text: '%s'.", uiLineCount, pucLine);
            continue;
#endif    /* defined(SRCH_INDEX_ENABLE_ERROR_ON_INVALID_LINES) */
        }


/* fprintf(stderr, "%u - %s\n", uiLineCount, pucLine); */

        switch ( pucLine[0] ) {
            
            /* Field Name
            **
            **  F field_name{A} field_id{N} field_type{A} field_option{A}[,...] [field_description{A}]
            **
            */
            case 'F':
                
                {
                    unsigned char   *pucFieldNamePtr = NULL;
                    unsigned int    uiFieldID = 0;
                    unsigned int    uiFieldType = SRCH_INFO_FIELD_TYPE_NONE_ID;
                    unsigned int    uiFieldOptions = SRCH_INFO_FIELD_OPTION_NONE;
                    unsigned char   *pucFieldDescriptionPtr = NULL;


                    /* Set the field option defaults */
                    iSrchInfoGetFieldOptionDefaults(psiSrchIndex, &uiFieldOptions);
                
    
                    /* Get the field name */
                    if ( (pucLinePtr = s_strtok_r(pucLine + 2, " \t", (char **)&pucLineStrtokPtr)) == NULL ) {
                        iUtlLogError(UTL_LOG_CONTEXT, "Missing field name in field name, line: %u, line text: '%s'.", uiLineCount, pucLine);
                        iError = SRCH_IndexerInvalidFieldNameTag;
                        goto bailFromiSrchIndexParseIndexStream;
                    }
                    else {
                        pucFieldNamePtr = pucLinePtr;
                    }


                    /* Get the field ID */
                    if ( (pucLinePtr = s_strtok_r(NULL, " \t", (char **)&pucLineStrtokPtr)) == NULL ) {
                        iUtlLogError(UTL_LOG_CONTEXT, "Missing field ID in field name, line: %u, line text: '%s'.", uiLineCount, pucLine);
                        iError = SRCH_IndexerInvalidFieldNameTag;
                        goto bailFromiSrchIndexParseIndexStream;
                    }
                    else {
                        uiFieldID = s_strtol(pucLinePtr, NULL, 10);
                    }


                    /* Get the field type */
                    if ( (pucLinePtr = s_strtok_r(NULL, " \t", (char **)&pucLineStrtokPtr)) == NULL ) {
                        iUtlLogError(UTL_LOG_CONTEXT, "Missing field type in field name, line: %u, line text: '%s'.", uiLineCount, pucLine);
                        iError = SRCH_IndexerInvalidFieldNameTag;
                        goto bailFromiSrchIndexParseIndexStream;
                    }
                    else {

                        /* Field type NONE */
                        if ( s_strcasecmp(SRCH_INFO_FIELD_TYPE_NONE_STRING, pucLinePtr) == 0 ) {
                            uiFieldType = SRCH_INFO_FIELD_TYPE_NONE_ID;
                        }
                        
                        /* Field type CHAR */
                        else if ( s_strcasecmp(SRCH_INFO_FIELD_TYPE_CHAR_STRING, pucLinePtr) == 0 ) {
                            uiFieldType = SRCH_INFO_FIELD_TYPE_CHAR_ID;
                        }
                        
                        /* Field type INT */
                        else if ( s_strcasecmp(SRCH_INFO_FIELD_TYPE_INT_STRING, pucLinePtr) == 0 ) {
                            uiFieldType = SRCH_INFO_FIELD_TYPE_INT_ID;
                        }
                        
                        /* Field type LONG */
                        else if ( s_strcasecmp(SRCH_INFO_FIELD_TYPE_LONG_STRING, pucLinePtr) == 0 ) {
                            uiFieldType = SRCH_INFO_FIELD_TYPE_LONG_ID;
                        }
                        
                        /* Field type FLOAT */
                        else if ( s_strcasecmp(SRCH_INFO_FIELD_TYPE_FLOAT_STRING, pucLinePtr) == 0 ) {
                            uiFieldType = SRCH_INFO_FIELD_TYPE_FLOAT_ID;
                        }
                        
                        /* Field type DOUBLE */
                        else if ( s_strcasecmp(SRCH_INFO_FIELD_TYPE_DOUBLE_STRING, pucLinePtr) == 0 ) {
                            uiFieldType = SRCH_INFO_FIELD_TYPE_DOUBLE_ID;
                        }
                        
                        /* Unknown field type */
                        else {
                            iUtlLogError(UTL_LOG_CONTEXT, "Invalid field type in field name, line: %u, line text: '%s'.", uiLineCount, pucLine);
                            iError = SRCH_IndexerInvalidFieldNameTag;
                            goto bailFromiSrchIndexParseIndexStream;
                        }
                    }

                    
                    /* Get the field options */
                    if ( (pucLinePtr = s_strtok_r(NULL, " \t", (char **)&pucLineStrtokPtr)) == NULL ) {
                        iUtlLogError(UTL_LOG_CONTEXT, "Missing field options in field name, line: %u, line text: '%s'.", uiLineCount, pucLine);
                        iError = SRCH_IndexerInvalidFieldNameTag;
                        goto bailFromiSrchIndexParseIndexStream;
                    }
                    else {
    
                        unsigned char   *pucFieldOptions = NULL;
                        unsigned char   *pucFieldOptionPtr = NULL;
                        unsigned char   *pucFieldOptionsStrtokPtr = NULL;
    

                        /* Duplicate the field options as s_strtok_r() destroys it */
                        if ( (pucFieldOptions = s_strdup(pucLinePtr)) == NULL ) {
                            iError = SRCH_MemError;
                            goto bailFromiSrchIndexParseIndexStream;
                        }
    
                        /* Loop parsing the field options */
                        for ( pucFieldOptionPtr = (unsigned char *)s_strtok_r(pucFieldOptions, SRCH_INFO_FIELD_OPTIONS_SEPARATORS, (char **)&pucFieldOptionsStrtokPtr); 
                                pucFieldOptionPtr != NULL; 
                                pucFieldOptionPtr = (unsigned char *)s_strtok_r(NULL, SRCH_INFO_FIELD_OPTIONS_SEPARATORS, (char **)&pucFieldOptionsStrtokPtr) ) {
        
                            /* Defaults */
                            if ( s_strcasecmp(SRCH_INFO_FIELD_OPTION_DEFAULTS_SYMBOL, pucFieldOptionPtr) == 0 ) {
                                ;
                            }
                            
                            /* Switch stemming on */
                            else if ( s_strcasecmp(SRCH_INFO_FIELD_OPTION_STEMMING_ON_STRING, pucFieldOptionPtr) == 0 ) {
                                vSrchInfoFieldOptionSetStemmingOn(uiFieldOptions);
                            }
                            
                            /* Switch stemming off */
                            else if ( s_strcasecmp(SRCH_INFO_FIELD_OPTION_STEMMING_OFF_STRING, pucFieldOptionPtr) == 0 ) {
                                vSrchInfoFieldOptionSetStemmingOff(uiFieldOptions);
                            }
                            
                            /* Switch stop term on */
                            else if ( s_strcasecmp(SRCH_INFO_FIELD_OPTION_STOP_TERM_ON_STRING, pucFieldOptionPtr) == 0 ) {
                                vSrchInfoFieldOptionSetStopTermOn(uiFieldOptions);
                            }
                            
                            /* Switch stop term off */
                            else if ( s_strcasecmp(SRCH_INFO_FIELD_OPTION_STOP_TERM_OFF_STRING, pucFieldOptionPtr) == 0 ) {
                                vSrchInfoFieldOptionSetStopTermOff(uiFieldOptions);
                            }
                        
                            /* Switch term position on */
                            else if ( s_strcasecmp(SRCH_INFO_FIELD_OPTION_TERM_POSITION_ON_STRING, pucFieldOptionPtr) == 0 ) {
                                vSrchInfoFieldOptionSetTermPositionOn(uiFieldOptions);
                            }
                            
                            /* Switch term position off */
                            else if ( s_strcasecmp(SRCH_INFO_FIELD_OPTION_TERM_POSITION_OFF_STRING, pucFieldOptionPtr) == 0 ) {
                                vSrchInfoFieldOptionSetTermPositionOff(uiFieldOptions);
                            }
                        
                            /* Unknown field option, assume that it is the start of the field description */
                            else {
                                iUtlLogError(UTL_LOG_CONTEXT, "Invalid field option in field name, line: %u, line text: '%s'.", uiLineCount, pucLine);
                                iError = SRCH_IndexerInvalidFieldNameTag;
                                goto bailFromiSrchIndexParseIndexStream;
                            }
                        }
        
                        /* Free the field options */
                        s_free(pucFieldOptions);
                    }

                
                    /* Get the field description if there is one */
                    if ( (pucLinePtr = s_strtok_r(NULL, "\n", (char **)&pucLineStrtokPtr)) != NULL ) {
                        pucFieldDescriptionPtr = pucLinePtr;
                    }
    
    
/*                     iUtlLogDebug(UTL_LOG_CONTEXT, "Field [%s] [%u] [%u] [%u] [%s]", pucFieldNamePtr, uiFieldID, uiFieldType, uiFieldOptions, pucUtlStringsGetPrintableString(pucFieldDescription)); */
    
    
                    /* Check the parameters we read */
                    if ( (uiFieldID == 0) || (bUtlStringsIsStringNULL(pucFieldNamePtr) == true) ) {
                        iUtlLogError(UTL_LOG_CONTEXT, "Missing field name and/or field ID in field name, line: %u, line text: '%s'.", uiLineCount, pucLine);
                        iError = SRCH_IndexerInvalidFieldNameTag;
                        goto bailFromiSrchIndexParseIndexStream;
                    }        
    
                    /* Check that we did not skip a field in the sequence */
                    if ( uiFieldID != uiSrchIndexerFieldsLength ) {
                        iUtlLogError(UTL_LOG_CONTEXT, "Field entry has a field ID which is out of sequence, we were expecting [%u], and we got [%u], in field name, line: %u, line text: '%s'.", 
                                (uiSrchIndexerFieldsLength + 1), uiFieldID, uiLineCount, pucLine);
                        iError = SRCH_IndexerInvalidFieldNameTag;
                        goto bailFromiSrchIndexParseIndexStream;
                    }

/*                     iUtlLogDebug(UTL_LOG_CONTEXT, "Adding search field structure, uiFieldID: [%u], pucFieldNamePtr: [%s], pucFieldDescription: [%s], uiFieldType: [%u], uiFieldOptions: [%u]",  */
/*                             uiFieldID, pucFieldNamePtr, pucFieldDescription, uiFieldType, uiFieldOptions); */

                    /* Add to the field name list */
                    if ( (psifSrchIndexerFieldsPtr = (struct srchIndexerField *)s_realloc(psifSrchIndexerFields, (size_t)(sizeof(struct srchIndexerField) * (uiSrchIndexerFieldsLength + 1)))) == NULL ) {
                        iError = SRCH_MemError;
                        goto bailFromiSrchIndexParseIndexStream;
                    }
                    
                    /* Hand over the pointer */
                    psifSrchIndexerFields = psifSrchIndexerFieldsPtr;
                    
                    /* Dereference the new entry */
                    psifSrchIndexerFieldsPtr = psifSrchIndexerFields + uiSrchIndexerFieldsLength;
                    
                    /* Set the data */
                    s_strnncpy(psifSrchIndexerFieldsPtr->pucFieldName, pucFieldNamePtr, SPI_FIELD_NAME_MAXIMUM_LENGTH + 1);
                    s_strnncpy(psifSrchIndexerFieldsPtr->pucFieldDescription, pucFieldDescriptionPtr, SPI_FIELD_DESCRIPTION_MAXIMUM_LENGTH + 1);
                    psifSrchIndexerFieldsPtr->uiFieldType = uiFieldType;
                    psifSrchIndexerFieldsPtr->uiFieldOptions = uiFieldOptions;
    
                    /* Increment the search field structure length */
                    uiSrchIndexerFieldsLength++;
                }

                break;


        
            /* Unfielded search field names
            **
            **  S field_name{A}[,...]
            **
            */
            case 'S':
            
                {
                    /* Get the unfielded search field names */
                    if ( pucLine[2] == '\0' ) { 
                        iUtlLogError(UTL_LOG_CONTEXT, "Missing unfielded search field names, line: %u, line text: '%s'.", uiLineCount, pucLine);
                        iError = SRCH_IndexerInvalidUnfieldedSearchFieldNamesTag;
                        goto bailFromiSrchIndexParseIndexStream;
                    }
                    else {
                        pucLinePtr = pucLine + 2;
                    }
                    
                    /* Make a copy of the search field names - do it now because s_strtok_r() destroys the string */
                    s_strnncpy(pucUnfieldedSearchFieldNames, pucLinePtr, SRCH_INFO_SYMBOL_MAXIMUM_LENGTH + 1);
    
                    /* Loop parsing the unfielded search field names */
                    for ( pucLinePtr = (unsigned char *)s_strtok_r(pucLinePtr, SRCH_INFO_UNFIELDED_SEARCH_FIELD_NAMES_SEPARATORS, (char **)&pucLineStrtokPtr); 
                            pucLinePtr != NULL; 
                            pucLinePtr = (unsigned char *)s_strtok_r(NULL, SRCH_INFO_UNFIELDED_SEARCH_FIELD_NAMES_SEPARATORS, (char **)&pucLineStrtokPtr) ) {
    
                        /* Check to see if the field name is in our list, if it is we check it out */
                        for ( uiI = 1, psifSrchIndexerFieldsPtr = psifSrchIndexerFields + 1; uiI < uiSrchIndexerFieldsLength; uiI++, psifSrchIndexerFieldsPtr++ ) {
                            if ( s_strcmp(psifSrchIndexerFieldsPtr->pucFieldName, pucLinePtr) == 0 ) {
                                break;
                            }
                        }
                        
                        /* Complain if we have not found it */
                        if ( uiI >= uiSrchIndexerFieldsLength ) {
                            iUtlLogError(UTL_LOG_CONTEXT, "Failed to find one of the unfielded search field name in the field names, line: %u, line text: '%s'.", uiLineCount, pucLine);
                            iError = SRCH_IndexerInvalidUnfieldedSearchFieldNamesTag;
                            goto bailFromiSrchIndexParseIndexStream;
                        }
                    }
    
/*                     iUtlLogDebug(UTL_LOG_CONTEXT, "Unfielded search field names [%s]", pucUnfieldedSearchFieldNames); */
                }

                break;


        
            /* Language
            **
            **  L language{A}
            **
            */
            case 'L':

                {
                    unsigned char   *pucLanguageCodePtr = NULL;
                    unsigned int    uiLanguageID = LNG_LANGUAGE_ANY_ID;

                    /* Get the language code */
                    if ( pucLine[1] == '\0' ) { 
                        iUtlLogError(UTL_LOG_CONTEXT, "Missing language code, line: %u, line text: '%s'.", uiLineCount, pucLine);
                        iError = SRCH_IndexerInvalidLanguageCodeTag;
                        goto bailFromiSrchIndexParseIndexStream;
                    }
                    else {
                        pucLanguageCodePtr = pucLine + 2;
                    }
    
    
                    /* Get the language ID */
                    if ( (iError = iLngGetLanguageIDFromCode(pucLanguageCodePtr, &uiLanguageID)) != LNG_NoError ) {
    
                        iUtlLogWarn(UTL_LOG_CONTEXT, "Invalid language code: '%s', line: %u, line text: '%s', utl error: %d.", pucLanguageCodePtr, uiLineCount, pucLine, iError);
    
                        /* We failed to get the language ID, so we set the current language ID from the default language ID */
                        uiCurrentLanguageID = uiDefaultLanguageID;
                        
                        /* Dont change the document language ID */
                    }
                    else {
    
                        /* Switch the language if the language ID and the current language ID don't match, we do this so that we don't keep flipping languages */
                        if ( uiLanguageID != uiCurrentLanguageID ) {
                            
                            /* Switch the language */
                            if ( (iError = iSrchInvertSwitchLanguage(psiSrchIndex, pucLanguageCodePtr)) != SRCH_NoError ) {
                                iUtlLogError(UTL_LOG_CONTEXT, "Failed to switch the language, line: %u, line text: '%s', srch error: %d.", uiLineCount, pucLine, iError);
                                goto bailFromiSrchIndexParseIndexStream;
                            }
            
                            /* Set the current language ID */
                            uiCurrentLanguageID = uiLanguageID;
                        }
    
                        /* Set the document language ID if it has not already been set */
                        if ( (uiDocumentLanguageID == uiDefaultLanguageID) && (uiLanguageID != uiDocumentLanguageID) ) {
                            uiDocumentLanguageID = uiLanguageID;
                        }
                    }

/*                     iUtlLogDebug(UTL_LOG_CONTEXT, "Language code: [%s]", pucLanguageCodePtr); */
                }

                break;



            /* Document Term 
            **
            **  T term [term_position{N}] field_id{N}
            **
            */
            case 'T':

                {
                    unsigned char   *pucTermPtr = NULL;
                    unsigned int    uiFieldID = 0;
                    unsigned int    uiTermPosition = 0;
    
                    unsigned int    uiFieldType = 0;
                    unsigned int    uiFieldOptions = 0;
    
    
                    /* Allocate a document ID if it has not been allocated */
                    if  ( uiDocumentID == 0 ) {
                        if ( (iError = iSrchDocumentGetNewDocumentID(psiSrchIndex, &uiDocumentID)) != SRCH_NoError ) {
                            iUtlLogError(UTL_LOG_CONTEXT, "Failed to get a new document ID, index: '%s', srch error: %d.", psiSrchIndex->pucIndexName, iError);
                            goto bailFromiSrchIndexParseIndexStream;
                        }
                    }


                    /* Look for the field ID, we start at the end of the line and work backwards until we hit a space/tab or the start of the line */
                    for ( pucLinePtr = pucLine + uiLineLength; (pucLinePtr > pucLine) && (*pucLinePtr != ' ') && (*pucLinePtr != '\t'); pucLinePtr-- ) {
                        ;
                    }
                
                    /* Failed to find the field ID */
                    if ( pucLinePtr == pucLine ) {
                        iUtlLogError(UTL_LOG_CONTEXT, "Missing field ID in document term, line: %u, line text: '%s'.", uiLineCount, pucLine);
                        iError = SRCH_IndexerInvalidDocumentTermTag;
                        goto bailFromiSrchIndexParseIndexStream;
                    }

                    /* Get the field ID */
                    uiFieldID = s_strtol(pucLinePtr + 1, NULL, 10);
                
                    /* Check the field ID */
                    if ( uiFieldID > uiSrchIndexerFieldsLength ) {
                        iUtlLogError(UTL_LOG_CONTEXT, "Invalid field ID in document term, line: %u, line text: '%s'.", uiLineCount, pucLine);
                        iError = SRCH_IndexerInvalidDocumentTermTag;
                        goto bailFromiSrchIndexParseIndexStream;
                    }

                    /* Set the field type and options from the search field structure */
                    uiFieldType = (psifSrchIndexerFields + uiFieldID)->uiFieldType;
                    uiFieldOptions = (psifSrchIndexerFields + uiFieldID)->uiFieldOptions;

                    
                    /* Get the term position if needed */
                    if ( bSrchInfoFieldOptionTermPosition(uiFieldOptions) == true ) {

                        /* Look for the term position, we start from where we left off and work backwards until we hit a space/tab or the start of the line */
                        for ( pucLinePtr--; (pucLinePtr > pucLine) && (*pucLinePtr != ' ') && (*pucLinePtr != '\t'); pucLinePtr-- ) {
                            ;
                        }

                        /* Failed to find the term position */
                        if ( pucLinePtr == (pucLine + 1) ) {
                            iUtlLogError(UTL_LOG_CONTEXT, "Missing term position in document term, line: %u, line text: '%s'.", uiLineCount, pucLine);
                            iError = SRCH_IndexerInvalidDocumentTermTag;
                            goto bailFromiSrchIndexParseIndexStream;
                        }

                        /* Get the term position */
                        uiTermPosition = s_strtol(pucLinePtr + 1, NULL, 10);

                        /* Check the term position */
                        if ( uiTermPosition <= 0 ) {
                            iUtlLogError(UTL_LOG_CONTEXT, "Missing or invalid term position in document term, line: %u, line text: '%s'.", uiLineCount, pucLine);
                            iError = SRCH_IndexerInvalidDocumentTermTag;
                            goto bailFromiSrchIndexParseIndexStream;
                        }
                    }


                    /* Null terminate the line where we left off which would be the end of the term */
                    *pucLinePtr = '\0';

                    /* Get the term */
                    if ( (pucLinePtr = s_strtok_r(pucLine + 2, " \t", (char **)&pucLineStrtokPtr)) == NULL ) {
                        iUtlLogError(UTL_LOG_CONTEXT, "Missing term in document term, line: %u, line text: '%s'.", uiLineCount, pucLine);
                        iError = SRCH_IndexerInvalidDocumentTermTag;
                        goto bailFromiSrchIndexParseIndexStream;
                    }
                    else {
                        pucTermPtr = pucLinePtr;
                    }


                    /* Cannot accept a 0 term position once a non-0 term position has been accepted, and we cannot accept a reduction in term position */
                    if ( ((uiTermPosition == 0) && (uiPreviousTermPosition != 0)) || ((uiTermPosition != 0) && (uiTermPosition < uiPreviousTermPosition)) ) {
                        iUtlLogError(UTL_LOG_CONTEXT, "Invalid term position in document term, line: %u, line text: '%s'.", uiLineCount, pucLine);
                        iError = SRCH_IndexerInvalidDocumentTermTag;
                        goto bailFromiSrchIndexParseIndexStream;
                    }


                    /* Process term count and previous term position */
                    if ( (uiTermPosition != 0) && (uiTermPosition > uiPreviousTermPosition) ) {
                        
                        /* Increment the term count */
                        uiTermCount++;
                    
                        /* Set the previous term position */
                        uiPreviousTermPosition = uiTermPosition;
                    }
    
/*                     iUtlLogInfo(UTL_LOG_CONTEXT, "Term '%s' %u %u %u %u", pucTermPtr, uiTermPosition, uiFieldID, uiFieldType, uiFieldOptions); */

                    /* Add it, return any errors that are fatal  */
                    if ( (iError = iSrchInvertAddTerm(psiSrchIndex, uiDocumentID, pucTermPtr, uiTermPosition, uiFieldID, uiFieldType, uiFieldOptions)) != SRCH_NoError ) {
                        iUtlLogError(UTL_LOG_CONTEXT, "Failed to add term to the index, line: %u, line text: '%s', srch error: %d.", uiLineCount, pucLine, iError);
                        goto bailFromiSrchIndexParseIndexStream;
                    }
                }
                    
                break;



            /* Document Date
            **
            **  D date{D} [time{T}]
            **
            */
            case 'D':

                {
                    /* Initialize the date */
                    ulDocumentAnsiDate = 0;

                    /* Get the date */
                    if ( (pucLinePtr = s_strtok_r(pucLine + 2, " \t", (char **)&pucLineStrtokPtr)) == NULL ) {
                        iUtlLogError(UTL_LOG_CONTEXT, "Missing document date, line: %u, line text: '%s'.", uiLineCount, pucLine);
                        iError = SRCH_IndexerInvalidDocumentDateTag;
                        goto bailFromiSrchIndexParseIndexStream;
                    }


                    /* Validate/normalize the date */
                    if ( (iError = iUtlDateValidateAnsiDate(pucLinePtr, &ulDocumentAnsiDate)) != UTL_NoError ) {
                        iUtlLogError(UTL_LOG_CONTEXT, "Invalid document date, line: %u, line text: '%s', srch error: %d.", uiLineCount, pucLine, iError);
                        goto bailFromiSrchIndexParseIndexStream;
                    }

/*                     iUtlLogDebug(UTL_LOG_CONTEXT, "Document date: [%lu]", ulDocumentAnsiDate); */
                }
                
                break;



            /* Document Title 
            **
            **  H title{A}
            **
            */
            case 'H':

                {
                    /* Check that there is a title at all */
                    if ( pucLine[2] == '\0' ) { 

#if defined(SRCH_INDEX_ENABLE_DOCUMENT_TITLE_REQUIRED)
                        iUtlLogError(UTL_LOG_CONTEXT, "Missing title in document title, line: %u, line text: '%s'.", uiLineCount, pucLine);
                        iError = SRCH_IndexerInvalidDocumentTitleTag;
                        goto bailFromiSrchIndexParseIndexStream;
#endif    /* defined(SRCH_INDEX_ENABLE_DOCUMENT_TITLE_REQUIRED) */

                    }
                    else {
                        s_strnncpy(pucDocumentTitle, pucLine + 2, SPI_TITLE_MAXIMUM_LENGTH + 1);
/*                         iUtlLogDebug(UTL_LOG_CONTEXT, "Document title [%s]", pucDocumentTitle); */
                    }
                }
                
                break;



            /* Document Item
            **
            **  I item_name{A} mime_type{A} length{N} [filename{A} start{N} end{N}] [URL{A}] (Document types and source files - optional)
            **
            */ 
            case 'I':

                {
                    unsigned char   pucItemName[SPI_ITEM_NAME_MAXIMUM_LENGTH + 1] = {'\0'};
                    unsigned char   pucMimeType[SPI_MIME_TYPE_MAXIMUM_LENGTH + 1] = {'\0'};
                    unsigned int    uiItemLength = 0;
                    unsigned char   pucUrl[SPI_URL_MAXIMUM_LENGTH + 1] = {'\0'};
                    unsigned char   pucFilePath[UTL_FILE_PATH_MAX + 1] = {'\0'};
                    off_t           zStartOffset = 0;
                    off_t           zEndOffset = 0;
    
                    void            *pvData = NULL;
                    unsigned int    uiDataLength = 0;
                    unsigned int    uiBytesRead = 0;

                    int             iStatus = 0;
                    unsigned int    uiItemID = 0;
                    boolean         bFoundItemName = false;


                    /* Scan the line */
                    if ( (iStatus = sscanf(pucLine + 2, pucScanfFormatI, pucItemName, pucMimeType, &uiItemLength, pucFilePath, 
                            &zStartOffset, &zEndOffset, pucUrl)) < 3 ) {
                        iUtlLogError(UTL_LOG_CONTEXT, "Missing fields in document type, line: %u, line text: '%s'.", uiLineCount, pucLine);
                        iError = SRCH_IndexerInvalidDocumentItemTag;
                        goto bailFromiSrchIndexParseIndexStream;
                    }
    
/*                     iUtlLogDebug(UTL_LOG_CONTEXT, "Document types [%s][%s][%u][%s][%ld][%ld][%s]",  */
/*                             pucItemName, pucMimeType, uiItemLength, pucFilePath, zStartOffset, zEndOffset, pucUrl); */

                    /* Check the variables, need to find 3, 4, 6 or 7 fields */
                    if ( (iStatus != 3) && (iStatus != 4) && (iStatus != 6) && (iStatus != 7) ) {
                        iUtlLogError(UTL_LOG_CONTEXT, "Missing fields in document type, line: %u, line text: '%s'.", uiLineCount, pucLine);
                        iError = SRCH_IndexerInvalidDocumentItemTag;
                        goto bailFromiSrchIndexParseIndexStream;
                    }
    
                    /* Check the mime type, we should have a '/' in it */ 
                    if ( s_strchr(pucMimeType, '/') == NULL ) {
                        iUtlLogError(UTL_LOG_CONTEXT, "Bad mime type in document type, line: %u, line text: '%s'.", uiLineCount, pucLine);
                        iError = SRCH_IndexerInvalidDocumentItemTag;
                        goto bailFromiSrchIndexParseIndexStream;
                    }
    
                    /* If we got 3 fields, we need to check that the item length is ok */ 
                    if ( (iStatus == 3) && (uiItemLength < 0) ) {
                        iUtlLogError(UTL_LOG_CONTEXT, "Found a negative item length in document type, line: %u, line text: '%s'.", uiLineCount, pucLine);
                        iError = SRCH_IndexerInvalidDocumentItemTag;
                        goto bailFromiSrchIndexParseIndexStream;
                    }
    
                    /* If we got 4 or 7 fields, we need to check that we got a URL */ 
                    if ( ((iStatus == 4) && (s_strstr(pucFilePath, "://") == NULL)) || ((iStatus == 7) && (s_strstr(pucUrl, "://") == NULL)) ) {
                        iUtlLogError(UTL_LOG_CONTEXT, "Found a bad URL in document type, line: %u, line text: '%s'.", uiLineCount, pucLine);
                        iError = SRCH_IndexerInvalidDocumentItemTag;
                        goto bailFromiSrchIndexParseIndexStream;
                    }
    
                    /* If we got 6 fields, we need to check that the start and end index make sense */ 
                    if ( (iStatus == 6) && (zEndOffset < zStartOffset) ) {
                        iUtlLogError(UTL_LOG_CONTEXT, "Found a bad file offsets in document type, line: %u, line text: '%s'.", uiLineCount, pucLine);
                        iError = SRCH_IndexerInvalidDocumentItemTag;
                        goto bailFromiSrchIndexParseIndexStream;
                    }
    
                    /* If we got 4 fields, then we have a URL and no file path, so we copy it over */
                    if ( iStatus == 4 ) {
                        s_strnncpy(pucUrl, pucFilePath, SPI_URL_MAXIMUM_LENGTH + 1);
                        pucFilePath[0] = '\0';
                    }
    
    
                    /* See if we already have this item name/mime type in the item list */
                    for ( uiI = 0, psiiSrchIndexerItemsPtr = psiiSrchIndexerItems, bFoundItemName = false; uiI < uiSrchItemIndexerItemsLength; uiI++, psiiSrchIndexerItemsPtr++ ) {
                        if ( (s_strcmp(psiiSrchIndexerItemsPtr->pucItemName, pucItemName) == 0) && (s_strcmp(psiiSrchIndexerItemsPtr->pucMimeType, pucMimeType) == 0) ) {
    
                            /* Set the document item ID, note that they start at 1 */
                            bFoundItemName = true;
                            uiItemID = uiI + 1;
                            break;
                        }
                    }
    
    
                    /* Check that we dont already have this item name/mime type in our list of document items */
                    if ( bFoundItemName == true ) {
                        for ( uiI = 0, psdiSrchDocumentItemsPtr = psdiSrchDocumentItems; uiI < uiSrchDocumentItemsLength; uiI++, psdiSrchDocumentItemsPtr++ ) {
                            if ( psdiSrchDocumentItemsPtr->uiItemID == uiItemID ) {
                                iUtlLogError(UTL_LOG_CONTEXT, "Document item: '%s', mime type: '%s', has already been submitted for this document in document type, line: %u, line text: '%s'.", 
                                        pucItemName, pucMimeType, uiLineCount, pucLine);
                                iError = SRCH_IndexerInvalidDocumentItemTag;
                                goto bailFromiSrchIndexParseIndexStream;
                            }
                        }
                    }
    
    
                    /* Add the new item name into the search item structure if needed */
                    if ( bFoundItemName == false ) {
                        if ( (psiiSrchIndexerItemsPtr = (struct srchIndexerItem *)s_realloc(psiiSrchIndexerItems, (size_t)(sizeof(struct srchIndexerItem) * (uiSrchItemIndexerItemsLength + 1)))) == NULL ) {
                            iError = SRCH_MemError;
                            goto bailFromiSrchIndexParseIndexStream;
                        }
                        psiiSrchIndexerItems = psiiSrchIndexerItemsPtr;
                        s_strnncpy((psiiSrchIndexerItems + uiSrchItemIndexerItemsLength)->pucItemName, pucItemName, SPI_ITEM_NAME_MAXIMUM_LENGTH + 1);
                        s_strnncpy((psiiSrchIndexerItems + uiSrchItemIndexerItemsLength)->pucMimeType, pucMimeType, SPI_MIME_TYPE_MAXIMUM_LENGTH + 1);
                        uiSrchItemIndexerItemsLength++;
                        uiItemID = uiSrchItemIndexerItemsLength;
                    }
    
    
                    /* Increment the number of document type profiles that we have */
                    uiSrchDocumentItemsLength++;
    
                    /* Do we need to expand the document type profile structure? */
                    if ( uiSrchDocumentItemsLength > uiSrchDocumentItemsCapacity ) {
                        
                        if ( (psdiSrchDocumentItemsPtr = (struct srchDocumentItem *)s_realloc(psdiSrchDocumentItems, (size_t)(sizeof(struct srchDocumentItem) * uiSrchDocumentItemsLength))) == NULL ) {
                            iError = SRCH_MemError;
                            goto bailFromiSrchIndexParseIndexStream;
                        }
                        
                        psdiSrchDocumentItems = psdiSrchDocumentItemsPtr;
                        uiSrchDocumentItemsCapacity = uiSrchDocumentItemsLength;
                    }
    
    
                    /* Check to see if this document type is followed by a mime body */
    
                    /* Get the next line, this should be the mime content type tag, or just regular index stream */
                    if ( s_fgets(pucLine, uiLineCapacity, psiSrchIndexer->pfFile) == NULL ) {
                        /* We could not get a line, so we break out and let the outer while()
                        ** loop handle the error when it next tries to read from the file
                        */
                        break;
                    }
    
    
                    /* Increment the line count if needed */
                    uiLineLength = s_strlen(pucLine);
                    if ( ((uiLineLength >= 2) && (pucLine[uiLineLength - 2] == '\r')) || 
                            ((uiLineLength >= 1) && ((pucLine[uiLineLength - 1] == '\n') || (pucLine[uiLineLength - 1] == '\r'))) ) {
                        uiLineCount++;
                    }
    
                    /* Check for the mime content type tag, note that the content type should match
                    ** the mime type passed on the type line, we trust the index stream and dont
                    ** check what we got here with what was declared
                    */
                    if ( s_strncasecmp(pucLine, SRCH_INDEX_MIME_CONTENT_TYPE_TAG, s_strlen(SRCH_INDEX_MIME_CONTENT_TYPE_TAG)) != 0 ) {
                        /* This is not mime content tag, flag to the outer loop that a
                        ** line has already been read and is ready for processing 
                        */
                        bReadLine = false;
                    }
                    else {
    
                        /* Get the next line, this should be the mime length tag */
                        if ( s_fgets(pucLine, uiLineCapacity, psiSrchIndexer->pfFile) == NULL ) {
                            /* We could not get a line, so we break out and let the outer while()
                            ** loop handle the error when it next tries to read from the file
                            */
                            break;
                        }
    
                        /* Increment the line count if needed */
                        uiLineLength = s_strlen(pucLine);
                        if ( ((uiLineLength >= 2) && (pucLine[uiLineLength - 2] == '\r')) || 
                                ((uiLineLength >= 1) && ((pucLine[uiLineLength - 1] == '\n') || (pucLine[uiLineLength - 1] == '\r'))) ) {
                            uiLineCount++;
                        }
    
                        /* Check for the mime length tag */
                        if ( s_strncasecmp(pucLine, SRCH_INDEX_MIME_CONTENT_LENGTH_TAG, s_strlen(SRCH_INDEX_MIME_CONTENT_LENGTH_TAG)) == 0 ) {
    
                            /* Get the content length, note that the content length should match
                            ** the length passed on the type line, we trust the index stream and dont
                            ** check what we got here with what was declared
                            */
                            uiDataLength = s_strtol(pucLine + s_strlen(SRCH_INDEX_MIME_CONTENT_LENGTH_TAG), NULL, 10);
    
                            /* Get the next line, this is the empty line that comes after the mime header tags */
                            if ( s_fgets(pucLine, uiLineCapacity, psiSrchIndexer->pfFile) == NULL ) {
                                /* We could not get a line, so we break out and let the outer while()
                                ** loop handle the error when it next tries to read from the file
                                */
                                break;
                            }
    
    
                            /* Process the mime content if there is any */
                            if ( uiDataLength > 0 ) {
                                /* Allocate a buffer to read the mime content */
                                if ( (pvData = (void *)s_malloc((size_t)(uiDataLength))) == NULL ) {
                                    iError = SRCH_MemError;
                                    goto bailFromiSrchIndexParseIndexStream;
                                }
    
                                /* Read the mime content, we fail totally if we cant read it */
                                if ( (uiBytesRead = s_fread(pvData, 1, uiDataLength, psiSrchIndexer->pfFile)) != uiDataLength ) {
                                    iUtlLogError(UTL_LOG_CONTEXT, "Failed to read content for item: '%s', mime type: '%s', for this document in document type, looking for: %u bytes, read: %u bytes, line: %u, line text: '%s'.", 
                                            pucItemName, pucMimeType, uiDataLength, uiBytesRead, uiLineCount, pucLine);
                                    iError = SRCH_IndexerReadFailed;
                                    goto bailFromiSrchIndexParseIndexStream;
                                }
                                if ( s_feof(psiSrchIndexer->pfFile) != 0 ) {
                                    iUtlLogError(UTL_LOG_CONTEXT, "feof detected");
                                    iError = SRCH_IndexerReadFailed;
                                    goto bailFromiSrchIndexParseIndexStream;
                                }
                            }
    
                            /* Increment the line count if needed */
                            uiLineLength = s_strlen(pucLine);
                            if ( ((uiLineLength >= 2) && (pucLine[uiLineLength - 2] == '\r')) || 
                                    ((uiLineLength >= 1) && ((pucLine[uiLineLength - 1] == '\n') || (pucLine[uiLineLength - 1] == '\r'))) ) {
                                uiLineCount++;
                            }
                        }
                        else {
                            iUtlLogError(UTL_LOG_CONTEXT, "Missing mime content length tag for this document in document type, line: %u, line text: '%s'.", uiLineCount, pucLine);
                            iError = SRCH_IndexerInvalidDocumentItemTag;
                            goto bailFromiSrchIndexParseIndexStream;
                        }
                    }
    
    
                    /* Populate the document item */
                    (psdiSrchDocumentItems + (uiSrchDocumentItemsLength - 1))->uiItemID = uiItemID;
                    (psdiSrchDocumentItems + (uiSrchDocumentItemsLength - 1))->uiItemLength = uiItemLength;
                    s_strnncpy((psdiSrchDocumentItems + (uiSrchDocumentItemsLength - 1))->pucUrl, pucUrl, SPI_URL_MAXIMUM_LENGTH + 1);
                    s_strnncpy((psdiSrchDocumentItems + (uiSrchDocumentItemsLength - 1))->pucFilePath, pucFilePath, UTL_FILE_PATH_MAX + 1);
                    (psdiSrchDocumentItems + (uiSrchDocumentItemsLength - 1))->zStartOffset = zStartOffset;
                    (psdiSrchDocumentItems + (uiSrchDocumentItemsLength - 1))->zEndOffset = zEndOffset;
                    (psdiSrchDocumentItems + (uiSrchDocumentItemsLength - 1))->pvData = pvData;
                    (psdiSrchDocumentItems + (uiSrchDocumentItemsLength - 1))->uiDataLength = uiDataLength;
                }
                
                break;



            /* Document Key
            **
            **  K key{A}
            **
            */
            case 'K':
            
                {
                    /* Check if a document key has already been submitted for this document */
                    if ( bUtlStringsIsStringNULL(pucDocumentKey) == false ) {
                        iUtlLogError(UTL_LOG_CONTEXT, "Document key was already set to: '%s', for this document, line: %u, line text: '%s'.", 
                                pucDocumentKey, uiLineCount, pucLine);
                        iError = SRCH_IndexerInvalidDocumentKeyTag;
                        goto bailFromiSrchIndexParseIndexStream;
                    }
    
                    /* Get the document key */
                    if ( pucLine[2] == '\0' ) { 
                        iUtlLogError(UTL_LOG_CONTEXT, "Missing document key, line: %u, line text: '%s'.", uiLineCount, pucLine);
                        iError = SRCH_IndexerInvalidDocumentKeyTag;
                        goto bailFromiSrchIndexParseIndexStream;
                    }
                    else {
                        s_strnncpy(pucDocumentKey, pucLine + 2, SPI_DOCUMENT_KEY_MAXIMUM_LENGTH + 1);
/*                         iUtlLogDebug(UTL_LOG_CONTEXT, "Document key: [%s]", pucDocumentKey); */
                    }
                }

                break;



            /* Document rank
            **
            **  R rank{N}
            **
            */
            case 'R':

                {
                    /* Initialize the variables */
                    uiDocumentRank = 0;
                    
                    /* Get the document rank */
                    if ( pucLine[2] == '\0' ) { 
                        iUtlLogError(UTL_LOG_CONTEXT, "Missing document rank, line: %u, line text: '%s'.", uiLineCount, pucLine);
                        iError = SRCH_IndexerInvalidDocumentRankTag;
                        goto bailFromiSrchIndexParseIndexStream;
                    }
                    else {
                        uiDocumentRank = s_strtol(pucLine + 2, NULL, 10);
/*                         iUtlLogDebug(UTL_LOG_CONTEXT, "Document Rank [%u]", uiDocumentRank); */
                    }
                }
                
                break;



            /* Document term count
            **
            **  C count{N}
            **
            */
            case 'C':

                {
                    /* Initialize the variables */
                    uiDocumentTermCount = 0;
                    bDocumentTermCount = false;
                
                    /* Get the term count */
                    if ( pucLine[2] == '\0' ) { 
                        iUtlLogError(UTL_LOG_CONTEXT, "Missing term count, line: %u, line text: '%s'.", uiLineCount, pucLine);
                        iError = SRCH_IndexerInvalidDocumentTermCountTag;
                        goto bailFromiSrchIndexParseIndexStream;
                    }
                    else {
                        uiDocumentTermCount = s_strtol(pucLine + 2, NULL, 10);
                        bDocumentTermCount = true;
/*                         iUtlLogDebug(UTL_LOG_CONTEXT, "Document term count: [%u]", uiDocumentTermCount); */
                    }
                }

                break;



            /* Message to print out (if not supppressed)
            **
            **  M message{A}
            **
            */
            case 'M':
            
                if ( psiSrchIndexer->bSuppressMessages == false ) {
                    iUtlLogInfo(UTL_LOG_CONTEXT, "%s", pucLine + 2);
                }
                
                break;



            /* Document End
            **
            **  E
            **
            */
            case 'E':
                
                {

#if defined(SRCH_INDEX_ENABLE_DOCUMENT_TITLE_REQUIRED)
                    /* Check that the title is defined */
                    if ( bUtlStringsIsStringNULL(pucDocumentTitle) == true ) {
                        iUtlLogError(UTL_LOG_CONTEXT, "The title is required to store a document, line: %u, line text: '%s'.", 
                                uiLineCount, pucLine);
                        iError = SRCH_IndexerInvalidDocumentEndTag;
                        goto bailFromiSrchIndexParseIndexStream;
                    }
#endif    /* defined(SRCH_INDEX_ENABLE_DOCUMENT_TITLE_REQUIRED) */


#if defined(SRCH_INDEX_ENABLE_DOCUMENT_ITEMS_REQUIRED)
                    /* Check that there is at least one document item */
                    if ( uiSrchDocumentItemsLength == 0 ) {
                        iUtlLogError(UTL_LOG_CONTEXT, "At least one document item is required to store a document, line: %u, line text: '%s'.", 
                                uiLineCount, pucLine);
                        iError = SRCH_IndexerInvalidDocumentEndTag;
                        goto bailFromiSrchIndexParseIndexStream;
                    }
#endif    /* defined(SRCH_INDEX_ENABLE_DOCUMENT_ITEMS_REQUIRED) */


/*                     if ( bUtlLogIsDebug(UTL_LOG_CONTEXT) == true ) { */
/*                         iUtlLogDebug(UTL_LOG_CONTEXT, "pucDocumentKey: [%s]", pucDocumentKey); */
/*                          */
/*                         for ( uiI = 0, psdiSrchDocumentItemsPtr = psdiSrchDocumentItems; uiI < uiSrchDocumentItemsLength; uiI++, psdiSrchDocumentItemsPtr++ ) { */
/*                             iUtlLogDebug(UTL_LOG_CONTEXT, "Stored document item [%s][%s][%u][%s][%ld][%ld]", (psiiSrchIndexerItems + (psdiSrchDocumentItemsPtr->uiItemID))->pucItemName,  */
/*                                     (psiiSrchIndexerItems + (psdiSrchDocumentItemsPtr->uiItemID))->pucMimeType, psdiSrchDocumentItemsPtr->uiItemLength, psdiSrchDocumentItemsPtr->pucFilePath,  */
/*                                     psdiSrchDocumentItemsPtr->zStartOffset, psdiSrchDocumentItemsPtr->zEndOffset); */
/*                         } */
/*                     } */


                    /* The document key is required */
                    if ( bUtlStringsIsStringNULL(pucDocumentKey) == true ) {
                        iUtlLogError(UTL_LOG_CONTEXT, "The document key is required to store a document, title: '%s', line: %u, line text: '%s'.", 
                            pucDocumentTitle, uiLineCount, pucLine);
                        iError = SRCH_IndexerInvalidDocumentEndTag;
                        goto bailFromiSrchIndexParseIndexStream;
                    }


                    /* Allocate a document ID if it has not been allocated */
                    if  ( uiDocumentID == 0 ) {
                        if ( (iError = iSrchDocumentGetNewDocumentID(psiSrchIndex, &uiDocumentID)) != SRCH_NoError ) {
                            iUtlLogError(UTL_LOG_CONTEXT, "Failed to get a new document ID, index: '%s', srch error: %d.", psiSrchIndex->pucIndexName, iError);
                            goto bailFromiSrchIndexParseIndexStream;
                        }
                    }                
    
                    /* Set the document term count from the term count if the former was not set */
                    if ( bDocumentTermCount == false ) {
                        uiDocumentTermCount = uiTermCount;
                    }


                    /* Store the document information */
                    if ( (iError = iSrchDocumentSaveDocumentInfo(psiSrchIndex, uiDocumentID, pucDocumentTitle, pucDocumentKey, uiDocumentRank, uiDocumentTermCount, 
                            ulDocumentAnsiDate, uiDocumentLanguageID, psdiSrchDocumentItems, uiSrchDocumentItemsLength)) != SRCH_NoError ) {
                        iUtlLogError(UTL_LOG_CONTEXT, "Failed to set the document information for document ID: %u, index: '%s', srch error: %d.", 
                                uiDocumentID, psiSrchIndex->pucIndexName, iError);
                        goto bailFromiSrchIndexParseIndexStream;
                    }


                    /* Clear out the various variables and structures, note that we 
                    ** reset the document language ID to the default language ID
                    */
                    pucDocumentKey[0] = '\0';
                    uiDocumentID = 0;
                    pucDocumentTitle[0] = '\0';
                    uiDocumentRank = 0;
                    uiDocumentTermCount = 0;
                    ulDocumentAnsiDate = 0;
                    uiDocumentLanguageID = uiDefaultLanguageID;
    
                    uiTermCount = 0;
                    bDocumentTermCount = false;
                    uiPreviousTermPosition = 0;
    
                    uiSrchDocumentItemsLength = 0;
    
                    /* Free the data from the document items and clear it */
                    for ( uiI = 0, psdiSrchDocumentItemsPtr = psdiSrchDocumentItems; uiI < uiSrchDocumentItemsCapacity; uiI++, psdiSrchDocumentItemsPtr++ ) {
                        s_free(psdiSrchDocumentItemsPtr->pvData);
                    }
                    s_memset(psdiSrchDocumentItems, 0, (size_t)(sizeof(struct srchDocumentItem) * uiSrchDocumentItemsCapacity));

                }

                break;



            /* Stream End, normal termination
            **
            **  Z
            **
            */
            case 'Z':
                
                {
                    /* Check that the current document was properly ended */
                    if ( uiDocumentID != 0 ) {
                        iUtlLogError(UTL_LOG_CONTEXT, "Index stream was finished before the current document was ended.");
                        iError = SRCH_IndexerInvalidStreamEndTag;
                        goto bailFromiSrchIndexParseIndexStream;
                    }

    
                    /* Write out the index description info */
                    if ( (iError = iSrchInfoSetDescriptionInfo(psiSrchIndex, psiSrchIndexer->pucIndexDescription)) != SRCH_NoError ) {
                        iUtlLogError(UTL_LOG_CONTEXT, "Failed to set the index description in the information file, srch error: %d.", iError);
                        goto bailFromiSrchIndexParseIndexStream;
                    }

                    /* Write out the unfielded search field names info */
                    if ( bUtlStringsIsStringNULL(pucUnfieldedSearchFieldNames) == false ) {
                        if ( (iError = iSrchInfoSetUnfieldedSearchFieldNames(psiSrchIndex, pucUnfieldedSearchFieldNames)) != SRCH_NoError ) {
                            iUtlLogError(UTL_LOG_CONTEXT, "Failed to set the unfielded search field names in the information file, srch error: %d.", iError);
                            goto bailFromiSrchIndexParseIndexStream;
                        }
                    }
    
                    /* Write out the field info */
                    for ( uiI = 1, psifSrchIndexerFieldsPtr = psifSrchIndexerFields + 1; uiI < uiSrchIndexerFieldsLength; uiI++, psifSrchIndexerFieldsPtr++ ) {
                        if ( (iError = iSrchInfoSetFieldInfo(psiSrchIndex, uiI, psifSrchIndexerFieldsPtr->pucFieldName, psifSrchIndexerFieldsPtr->pucFieldDescription,
                                psifSrchIndexerFieldsPtr->uiFieldType, psifSrchIndexerFieldsPtr->uiFieldOptions)) != SRCH_NoError ) {
                            iUtlLogError(UTL_LOG_CONTEXT, "Failed to set the field information in the information file, srch error: %d.", iError);
                            goto bailFromiSrchIndexParseIndexStream;
                        }
                    }
    
                    /* Write out the item info */
                    for ( uiI = 0, psiiSrchIndexerItemsPtr = psiiSrchIndexerItems; uiI < uiSrchItemIndexerItemsLength; uiI++, psiiSrchIndexerItemsPtr++ ) {
                        if ( bUtlStringsIsStringNULL(psiiSrchIndexerItemsPtr->pucItemName) == false ) {
                            if ( (iError = iSrchInfoSetItemInfo(psiSrchIndex, (uiI + 1), psiiSrchIndexerItemsPtr->pucItemName, psiiSrchIndexerItemsPtr->pucMimeType)) != SRCH_NoError ) {
                                iUtlLogError(UTL_LOG_CONTEXT, "Failed to set the item information in the information file, srch error: %d.", iError);
                            }
                        }
                    }

                
                    /* Set the field ID maximum */
                    psiSrchIndex->uiFieldIDMaximum = uiSrchIndexerFieldsLength;


                    /* Exit normally */
                    iError = SRCH_NoError;
                    goto bailFromiSrchIndexParseIndexStream;
                }
            
                
#if defined(SRCH_INDEX_ENABLE_WARNING_ON_INVALID_TAGS)
            default:
                {
                    iUtlLogWarn(UTL_LOG_CONTEXT, "Found an undefined tag in the stream, line: %u, line text: '%s'.", uiLineCount, pucLine);
                }
#endif    /* defined(SRCH_INDEX_ENABLE_WARNING_ON_INVALID_TAGS) */

        }

    }        /* While () */




    /* Bail label */ 
    bailFromiSrchIndexParseIndexStream:


    /* Free up any allocated resources */
    s_free(psifSrchIndexerFields);
    s_free(psdiSrchDocumentItems);
    s_free(psiiSrchIndexerItems);
    s_free(pucLine);


    return (iError);

}


/*---------------------------------------------------------------------------*/



