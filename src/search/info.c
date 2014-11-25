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

    Module:     info.c

    Author:     Francois Schiettecatte (FS Consulting LLC.)

    Created:    14 September 1995

    Purpose:    This module contains the functions required to store
                stuff in the information file.

                This 'stuff' includes field name and IDs, element names,
                composite names.

                This module also includes the functions needed to create
                the catalog and source files.

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
#define UTL_LOG_CONTEXT                     (unsigned char *)"com.fsconsult.mps.src.search.info"


/* Enable symbol caching in a hash */
#define SRCH_INFO_ENABLE_SYMBOL_CACHING


/*---------------------------------------------------------------------------*/


/*
** Defines
*/


/* Version information key */
#define SRCH_INFO_VERSION_INFO_KEY                          (unsigned char *)"Version"


/* Language information key */
#define SRCH_INFO_LANGUAGE_INFO_KEY                         (unsigned char *)"Language"

/* Tokenizer information key */
#define SRCH_INFO_TOKENIZER_INFO_KEY                        (unsigned char *)"Tokenizer"


/* Stemmer information key */
#define SRCH_INFO_STEMMER_INFO_KEY                          (unsigned char *)"Stemmer"


/* Stop list information key */
#define SRCH_INFO_STOP_LIST_INFO_KEY                        (unsigned char *)"StopList"

/* Stop list type information key */
#define SRCH_INFO_STOP_LIST_TYPE_INFO_KEY                   (unsigned char *)"StopListType"

/* Stop list type names */
#define SRCH_INFO_STOP_LIST_TYPE_INTERNAL_INFO_VALUE        (unsigned char *)"internal"
#define SRCH_INFO_STOP_LIST_TYPE_FILE_INFO_VALUE            (unsigned char *)"file"


/* Term length maximum information key */
#define SRCH_INFO_TERM_LENGTH_MAXIMUM_INFO_KEY              (unsigned char *)"TermLengthMaximum"

/* Term length minimum information key */
#define SRCH_INFO_TERM_LENGTH_MINIMUM_INFO_KEY              (unsigned char *)"TermLengthMinimum"


/* Field names/description information key */
#define SRCH_INFO_FIELD_ID_INFO_KEY                         (unsigned char *)"FieldID"
#define SRCH_INFO_FIELD_NAME_INFO_KEY                       (unsigned char *)"FieldName"
#define SRCH_INFO_FIELD_DESCRIPTION_INFO_KEY                (unsigned char *)"FieldDescription"
#define SRCH_INFO_FIELD_OPTIONS_INFO_KEY                    (unsigned char *)"FieldOptions"
#define SRCH_INFO_FIELD_TYPE_INFO_KEY                       (unsigned char *)"FieldType"


/* Unfielded search field names */
#define SRCH_INFO_UNFIELDED_SEARCH_FIELD_NAMES_INFO_KEY     (unsigned char *)"UnfieldedSearchFieldNames"


/* Item information key */
#define SRCH_INFO_ITEM_INFO_KEY                             (unsigned char *)"Item"


/* Index description information key */
#define SRCH_INFO_INDEX_DESCRIPTION_INFO_KEY                (unsigned char *)"IndexDescription"


/* Unique term count information key */
#define SRCH_INFO_UNIQUE_TERM_COUNT_INFO_KEY                (unsigned char *)"UniqueTermCount"

/* Total term count information key */
#define SRCH_INFO_TOTAL_TERM_COUNT_INFO_KEY                 (unsigned char *)"TotalTermCount"

/* Unique stop term count information key */
#define SRCH_INFO_UNIQUE_STOP_TERM_COUNT_INFO_KEY           (unsigned char *)"UniqueStopTermCount"

/* Total stop term count information key */
#define SRCH_INFO_TOTAL_STOP_TERM_COUNT_INFO_KEY            (unsigned char *)"TotalStopTermCount"

/* Document count information key */
#define SRCH_INFO_DOCUMENT_COUNT_INFO_KEY                   (unsigned char *)"DocumentCount"

/* Document term count maximum information key */
#define SRCH_INFO_DOCUMENT_TERM_COUNT_MAXIMUM_INFO_KEY      (unsigned char *)"DocumentTermCountMaximum"

/* Document term count minimum information key */
#define SRCH_INFO_DOCUMENT_TERM_COUNT_MINIMUM_INFO_KEY      (unsigned char *)"DocumentTermCountMinimum"

/* Maximum field ID information key */
#define SRCH_INFO_MAXIMUM_FIELD_ID_INFO_KEY                 (unsigned char *)"MaximumFieldID"

/* Last update time information key */
#define SRCH_INFO_LAST_UPDATE_TIME_INFO_KEY                 (unsigned char *)"LastUpdateTime"


/*---------------------------------------------------------------------------*/


/* 
** =====================================
** ===  Version Information Support  ===
** =====================================
*/


/*

    Function:   iSrchInfoGetVersionInfo()

    Purpose:    Get the version information from the information file

    Parameters: psiSrchIndex        search index structure
                puiMajorVersion     return pointer for the major version
                puiMinorVersion     return pointer for the minor version
                puiPatchVersion     return pointer for the patch version

    Globals:    none

    Returns:    SRCH error code

*/
int iSrchInfoGetVersionInfo
(
    struct srchIndex *psiSrchIndex,
    unsigned int *puiMajorVersion,
    unsigned int *puiMinorVersion,
    unsigned int *puiPatchVersion
)
{

    int             iError = SRCH_NoError;
    unsigned char   pucConfigValue[SRCH_INFO_SYMBOL_MAXIMUM_LENGTH + 1] = {'\0'};
    unsigned int    uiMajorVersion = 0;
    unsigned int    uiMinorVersion = 0;
    unsigned int    uiPatchVersion = 0;
    

    /* Check the parameters */
    if ( psiSrchIndex == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'psiSrchIndex' parameter passed to 'iSrchInfoGetVersionInfo'."); 
        return (SRCH_InvalidIndex);
    }

    if ( puiMajorVersion == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'puiMajorVersion' parameter passed to 'iSrchInfoGetVersionInfo'."); 
        return (SRCH_ReturnParameterError);
    }


    /* Get the version information value */
    if ( (iError = iUtlConfigGetValue(psiSrchIndex->pvUtlIndexInformation, SRCH_INFO_VERSION_INFO_KEY, pucConfigValue, SRCH_INFO_SYMBOL_MAXIMUM_LENGTH + 1)) != UTL_NoError ) {
        return (SRCH_InfoSymbolNotFound);
    }


    /* Parse out the version information value */
    if ( sscanf(pucConfigValue, "%u.%u.%u", &uiMajorVersion, &uiMinorVersion, &uiPatchVersion) != 3 ) {
        return (SRCH_InfoInvalidVersion);
    }


    /* Set the return pointers */
    *puiMajorVersion = uiMajorVersion;
    *puiMinorVersion = uiMinorVersion;
    *puiPatchVersion = uiPatchVersion;


    return (SRCH_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchInfoSetVersionInfo()

    Purpose:    Write the index version information to the information file

    Parameters: psiSrchIndex        search index structure

    Globals:    none

    Returns:    SRCH error code

*/
int iSrchInfoSetVersionInfo
(
    struct srchIndex *psiSrchIndex
)
{

    int             iError = SRCH_NoError;
    unsigned char   pucConfigValue[SRCH_INFO_SYMBOL_MAXIMUM_LENGTH + 1] = {'\0'};


    /* Check the parameters */
    if ( psiSrchIndex == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'psiSrchIndex' parameter passed to 'iSrchInfoSetVersionInfo'."); 
        return (SRCH_InvalidIndex);
    }


    /* Add the version information entry */
    snprintf(pucConfigValue, UTL_CONFIG_VALUE_MAXIMUM_LENGTH + 1, "%u.%u.%u", psiSrchIndex->uiMajorVersion, psiSrchIndex->uiMinorVersion, psiSrchIndex->uiPatchVersion);
    if ( (iError = iUtlConfigAddEntry(psiSrchIndex->pvUtlIndexInformation, SRCH_INFO_VERSION_INFO_KEY, pucConfigValue)) != UTL_NoError ) {
        return (SRCH_InfoSymbolSetFailed);
    }


    return (SRCH_NoError);

}


/*---------------------------------------------------------------------------*/


/* 
** ======================================
** ===  Language Information Support  ===
** ======================================
*/


/*

    Function:   iSrchInfoGetLanguageInfo()

    Purpose:    Get the language information from the information file

    Parameters: psiSrchIndex            search index structure
                pucLanguageCode         return pointer for the language code
                uiLanguageCodeLength    length of the return pointer for the language code

    Globals:    none

    Returns:    SRCH error code

*/
int iSrchInfoGetLanguageInfo
(
    struct srchIndex *psiSrchIndex,
    unsigned char *pucLanguageCode,
    unsigned int uiLanguageCodeLength
)
{

    int     iError = SRCH_NoError;
    

    /* Check the parameters */
    if ( psiSrchIndex == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'psiSrchIndex' parameter passed to 'iSrchInfoGetLanguageInfo'."); 
        return (SRCH_InvalidIndex);
    }

    if ( pucLanguageCode == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pucLanguageCode' parameter passed to 'iSrchInfoGetLanguageInfo'."); 
        return (SRCH_ReturnParameterError);
    }

    if ( uiLanguageCodeLength <= 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiLanguageCodeLength' parameter passed to 'iSrchInfoGetDescriptionInfo'."); 
        return (SRCH_ReturnParameterError);
    }


    /* Get the language information value */
    if ( (iError = iUtlConfigGetValue(psiSrchIndex->pvUtlIndexInformation, SRCH_INFO_LANGUAGE_INFO_KEY, pucLanguageCode, uiLanguageCodeLength)) != UTL_NoError ) {
        return (SRCH_InfoSymbolNotFound);
    }


    return (SRCH_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchInfoSetLanguageInfo()

    Purpose:    Write the index language information to the information file

    Parameters: psiSrchIndex    search index structure

    Globals:    none

    Returns:    SRCH error code

*/
int iSrchInfoSetLanguageInfo
(
    struct srchIndex *psiSrchIndex
)
{

    int             iError = SRCH_NoError;
    unsigned char   pucLanguageCode[SPI_INDEX_LANGUAGE_MAXIMUM_LENGTH + 1] = {'\0'};


    /* Check the parameters */
    if ( psiSrchIndex == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'psiSrchIndex' parameter passed to 'iSrchInfoSetLanguageInfo'."); 
        return (SRCH_InvalidIndex);
    }


    /* Get the language code from the language ID */
    if ( (iError = iLngGetLanguageCodeFromID(psiSrchIndex->uiLanguageID, pucLanguageCode, SPI_INDEX_LANGUAGE_MAXIMUM_LENGTH + 1)) != LNG_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the language code from the language ID: %u, lng error: %d.", psiSrchIndex->uiLanguageID, iError); 
        return (SRCH_InfoInvalidLanguageID);
    }


    /* Add the language code entry */
    if ( (iError = iUtlConfigAddEntry(psiSrchIndex->pvUtlIndexInformation, SRCH_INFO_LANGUAGE_INFO_KEY, pucLanguageCode)) != UTL_NoError ) {
        return (SRCH_InfoSymbolSetFailed);
    }


    return (SRCH_NoError);

}


/*---------------------------------------------------------------------------*/


/* 
** =======================================
** ===  Tokenizer Information Support  ===
** =======================================
*/


/*

    Function:   iSrchInfoGetTokenizerInfo()

    Purpose:    Get the tokenizer information from the information file

    Parameters: psiSrchIndex        search index structure
                pucTokenizerName        return pointer for the tokenizer name
                uiTokenizerNameLength   length of the return pointer for the tokenizer name

    Globals:    none

    Returns:    SRCH error code

*/
int iSrchInfoGetTokenizerInfo
(
    struct srchIndex *psiSrchIndex,
    unsigned char *pucTokenizerName,
    unsigned int uiTokenizerNameLength
)
{

    int     iError = SRCH_NoError;
    

    /* Check the parameters */
    if ( psiSrchIndex == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'psiSrchIndex' parameter passed to 'iSrchInfoGetTokenizerInfo'."); 
        return (SRCH_InvalidIndex);
    }

    if ( pucTokenizerName == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pucTokenizerName' parameter passed to 'iSrchInfoGetTokenizerInfo'."); 
        return (SRCH_ReturnParameterError);
    }

    if ( uiTokenizerNameLength <= 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiTokenizerNameLength' parameter passed to 'iSrchInfoGetTokenizerInfo'."); 
        return (SRCH_ReturnParameterError);
    }


    /* Get the tokenizer information value */
    if ( (iError = iUtlConfigGetValue(psiSrchIndex->pvUtlIndexInformation, SRCH_INFO_TOKENIZER_INFO_KEY, pucTokenizerName, uiTokenizerNameLength)) != UTL_NoError ) {
        return (SRCH_InfoSymbolNotFound);
    }


    return (SRCH_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchInfoSetTokenizerInfo()

    Purpose:    Write the index tokenizer information to the information file

    Parameters: psiSrchIndex    search index structure

    Globals:    none

    Returns:    SRCH error code

*/
int iSrchInfoSetTokenizerInfo
(
    struct srchIndex *psiSrchIndex
)
{

    int             iError = SRCH_NoError;
    unsigned char   pucTokenizerName[SPI_INDEX_TOKENIZER_MAXIMUM_LENGTH + 1] = {'\0'};


    /* Check the parameters */
    if ( psiSrchIndex == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'psiSrchIndex' parameter passed to 'iSrchInfoSetTokenizerInfo'."); 
        return (SRCH_InvalidIndex);
    }


    /* Get the tokenizer name from the tokenizer ID */
    if ( (iError = iLngGetTokenizerNameFromID(psiSrchIndex->uiTokenizerID, pucTokenizerName, SPI_INDEX_TOKENIZER_MAXIMUM_LENGTH + 1)) != LNG_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the tokenizer name from the tokenizer ID: %u, lng error: %d.", psiSrchIndex->uiTokenizerID, iError); 
        return (SRCH_InfoInvalidTokenizerID);
    }


    /* Add the tokenizer name information entry */
    if ( (iError = iUtlConfigAddEntry(psiSrchIndex->pvUtlIndexInformation, SRCH_INFO_TOKENIZER_INFO_KEY, pucTokenizerName)) != UTL_NoError ) {
        return (SRCH_InfoSymbolSetFailed);
    }


    return (SRCH_NoError);

}


/*---------------------------------------------------------------------------*/


/* 
** =====================================
** ===  Stemmer Information Support  ===
** =====================================
*/


/*

    Function:   iSrchInfoGetStemmerInfo()

    Purpose:    Get the stemmer information from the information file

    Parameters: psiSrchIndex            search index structure
                pucStemmerName          return pointer for the stemmer name
                uiStemmerNameLength     length of the return pointer for the stemmer name

    Globals:    none

    Returns:    SRCH error code

*/
int iSrchInfoGetStemmerInfo
(
    struct srchIndex *psiSrchIndex,
    unsigned char *pucStemmerName,
    unsigned int uiStemmerNameLength
)
{

    int     iError = SRCH_NoError;
    

    /* Check the parameters */
    if ( psiSrchIndex == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'psiSrchIndex' parameter passed to 'iSrchInfoGetStemmerInfo'."); 
        return (SRCH_InvalidIndex);
    }

    if ( pucStemmerName == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pucStemmerName' parameter passed to 'iSrchInfoGetStemmerInfo'."); 
        return (SRCH_ReturnParameterError);
    }

    if ( uiStemmerNameLength <= 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiStemmerNameLength' parameter passed to 'iSrchInfoGetStemmerInfo'."); 
        return (SRCH_ReturnParameterError);
    }


    /* Get the stemmer information value */
    if ( (iError = iUtlConfigGetValue(psiSrchIndex->pvUtlIndexInformation, SRCH_INFO_STEMMER_INFO_KEY, pucStemmerName, uiStemmerNameLength)) != UTL_NoError ) {
        return (SRCH_InfoSymbolNotFound);
    }


    return (SRCH_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchInfoSetStemmerInfo()

    Purpose:    Write the index stemmer information to the information file

    Parameters: psiSrchIndex        search index structure

    Globals:    none

    Returns:    SRCH error code

*/
int iSrchInfoSetStemmerInfo
(
    struct srchIndex *psiSrchIndex
)
{

    int             iError = SRCH_NoError;
    unsigned char   pucStemmerName[SPI_INDEX_STEMMER_MAXIMUM_LENGTH + 1] = {'\0'};


    /* Check the parameters */
    if ( psiSrchIndex == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'psiSrchIndex' parameter passed to 'iSrchInfoSetStemmerInfo'."); 
        return (SRCH_InvalidIndex);
    }


    /* Get the stemmer name from the stemmer ID */
    if ( (iError = iLngGetStemmerNameFromID(psiSrchIndex->uiStemmerID, pucStemmerName, SPI_INDEX_STEMMER_MAXIMUM_LENGTH + 1)) != LNG_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the stemmer name from the stemmer ID: %u, lng error: %d.", psiSrchIndex->uiStemmerID, iError); 
        return (SRCH_InfoInvalidStemmerID);
    }


    /* Add the stemmer information entry */
    if ( (iError = iUtlConfigAddEntry(psiSrchIndex->pvUtlIndexInformation, SRCH_INFO_STEMMER_INFO_KEY, pucStemmerName)) != UTL_NoError ) {
        return (SRCH_InfoSymbolSetFailed);
    }


    return (SRCH_NoError);

}


/*---------------------------------------------------------------------------*/


/* 
** =======================================
** ===  Stop List Information Support  ===
** =======================================
*/


/*

    Function:   iSrchInfoGetStopListInfo()

    Purpose:    Get the stop list information from the information file

    Parameters: psiSrchIndex            search index structure
                pucStopListName         return pointer for the stop list name (optional)
                uiStopListNameLength    length of the return pointer for the stop list name (optional)
                puiStopListTypeID       return pointer for the stop list type ID (optional)

    Globals:    none

    Returns:    SRCH error code

*/
int iSrchInfoGetStopListInfo
(
    struct srchIndex *psiSrchIndex,
    unsigned char *pucStopListName,
    unsigned int uiStopListNameLength,
    unsigned int *puiStopListTypeID
)
{

    int     iError = SRCH_NoError;
    

    /* Check the parameters */
    if ( psiSrchIndex == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'psiSrchIndex' parameter passed to 'iSrchInfoGetStopListInfo'."); 
        return (SRCH_InvalidIndex);
    }

    if ( (pucStopListName == NULL) && (puiStopListTypeID == NULL) ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pucStopListName' & 'puiStopListTypeID' parameters passed to 'iSrchInfoGetStopListInfo'.");
        return (SRCH_ReturnParameterError);
    }

    if ( (pucStopListName != NULL) && (uiStopListNameLength <= 0) ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiStopListNameLength' parameter passed to 'iSrchInfoGetStopListInfo'.");
        return (SRCH_ReturnParameterError);
    }


    /* Get the stop list name information value */
    if ( pucStopListName != NULL ) {
        if ( (iError = iUtlConfigGetValue(psiSrchIndex->pvUtlIndexInformation, SRCH_INFO_STOP_LIST_INFO_KEY, pucStopListName, uiStopListNameLength)) != UTL_NoError ) {
            return (SRCH_InfoSymbolNotFound);
        }
    }


    /* Get the stop list type */
    if ( puiStopListTypeID != NULL ) {

        unsigned char   pucSymbolValue[SRCH_INFO_SYMBOL_MAXIMUM_LENGTH + 1] = {'\0'};

        /* Get the stop list type information value */
        if ( (iError = iUtlConfigGetValue(psiSrchIndex->pvUtlIndexInformation, SRCH_INFO_STOP_LIST_TYPE_INFO_KEY, pucSymbolValue, SRCH_INFO_SYMBOL_MAXIMUM_LENGTH + 1)) != UTL_NoError ) {
            return (SRCH_InfoSymbolNotFound);
        }
        
        /* Match the stop list type symbol to the stop list type ID */
        if ( s_strcmp(SRCH_INFO_STOP_LIST_TYPE_INTERNAL_INFO_VALUE, pucSymbolValue) == 0 ) {
            *puiStopListTypeID = SRCH_INFO_STOP_LIST_TYPE_INTERNAL_ID;
        }
        else if ( s_strcmp(SRCH_INFO_STOP_LIST_TYPE_FILE_INFO_VALUE, pucSymbolValue) == 0 ) {
            *puiStopListTypeID = SRCH_INFO_STOP_LIST_TYPE_FILE_ID;
        }
        else {
            /* Failed the match, return an error */
            return (SRCH_InfoInvalidStopListTypeID);
        }
    }


    return (SRCH_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchInfoSetStopListInfo()

    Purpose:    Write the index stop list name information to the information file

    Parameters: psiSrchIndex    search index structure

    Globals:    none

    Returns:    SRCH error code

*/
int iSrchInfoSetStopListInfo
(
    struct srchIndex *psiSrchIndex
)
{

    int             iError = SRCH_NoError;
    unsigned char   pucStopListName[SPI_INDEX_STEMMER_MAXIMUM_LENGTH + 1] = {'\0'};


    /* Check the parameters */
    if ( psiSrchIndex == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'psiSrchIndex' parameter passed to 'iSrchInfoSetStopListInfo'."); 
        return (SRCH_InvalidIndex);
    }


    /* Add the stop list name information entry */
    if ( psiSrchIndex->psibSrchIndexBuild->uiStopListTypeID == SRCH_INFO_STOP_LIST_TYPE_INTERNAL_ID ) { 
        
        /* Get the stop list name from the stop list ID */
        if ( (iError = iLngGetStopListNameFromID(psiSrchIndex->psibSrchIndexBuild->uiStopListID, pucStopListName, SPI_INDEX_STEMMER_MAXIMUM_LENGTH + 1)) != LNG_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the stop list name from the stop list ID: %u, lng error: %d.", psiSrchIndex->psibSrchIndexBuild->uiStopListID, iError); 
            return (SRCH_InfoInvalidStopListID);
        }
    
        /* Add the stop list name information entry */
        if ( (iError = iUtlConfigAddEntry(psiSrchIndex->pvUtlIndexInformation, SRCH_INFO_STOP_LIST_INFO_KEY, pucStopListName)) != UTL_NoError ) {
            return (SRCH_InfoSymbolSetFailed);
        }

    }
    else if ( psiSrchIndex->psibSrchIndexBuild->uiStopListTypeID == SRCH_INFO_STOP_LIST_TYPE_FILE_ID ) { 
    
        /* Add the stop list name information entry */
        if ( (iError = iUtlConfigAddEntry(psiSrchIndex->pvUtlIndexInformation, SRCH_INFO_STOP_LIST_INFO_KEY, psiSrchIndex->psibSrchIndexBuild->pucStopListFilePath)) != UTL_NoError ) {
            return (SRCH_InfoSymbolSetFailed);
        }
    }
    else {
        /* Failed the match, return an error */
        return (SRCH_InfoInvalidStopListTypeID);
    }


    /* Add the stop list type information entry */
    if ( psiSrchIndex->psibSrchIndexBuild->uiStopListTypeID == SRCH_INFO_STOP_LIST_TYPE_INTERNAL_ID ) { 
        if ( (iError = iUtlConfigAddEntry(psiSrchIndex->pvUtlIndexInformation, SRCH_INFO_STOP_LIST_TYPE_INFO_KEY, SRCH_INFO_STOP_LIST_TYPE_INTERNAL_INFO_VALUE)) != UTL_NoError ) {
            return (SRCH_InfoSymbolSetFailed);
        }
    }
    else if ( psiSrchIndex->psibSrchIndexBuild->uiStopListTypeID == SRCH_INFO_STOP_LIST_TYPE_FILE_ID ) { 
        if ( (iError = iUtlConfigAddEntry(psiSrchIndex->pvUtlIndexInformation, SRCH_INFO_STOP_LIST_TYPE_INFO_KEY, SRCH_INFO_STOP_LIST_TYPE_FILE_INFO_VALUE)) != UTL_NoError ) {
            return (SRCH_InfoSymbolSetFailed);
        }
    }
    else {
        /* Failed the match, return an error */
        return (SRCH_InfoInvalidStopListTypeID);
    }


    return (SRCH_NoError);

}


/*---------------------------------------------------------------------------*/


/* 
** =========================================
** ===  Term Length Information Support  ===
** =========================================
*/


/*

    Function:   iSrchInfoGetTermLengthInfo()

    Purpose:    Get the term length information from the information file

    Parameters: psiSrchIndex            search index structure
                puiTermLengthMaximum    return pointer for the maximum term length
                puiTermLengthMinimum    return pointer for the minimum term length

    Globals:    none

    Returns:    SRCH error code

*/
int iSrchInfoGetTermLengthInfo
(
    struct srchIndex *psiSrchIndex,
    unsigned int *puiTermLengthMaximum, 
    unsigned int *puiTermLengthMinimum
)
{

    int             iError = SRCH_NoError;
    unsigned char   pucConfigValue[SRCH_INFO_SYMBOL_MAXIMUM_LENGTH + 1] = {'\0'};
    

    /* Check the parameters */
    if ( psiSrchIndex == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'psiSrchIndex' parameter passed to 'iSrchInfoGetTermLengthInfo'."); 
        return (SRCH_InvalidIndex);
    }

    if ( (puiTermLengthMaximum == NULL) && (puiTermLengthMinimum == NULL) ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'puiTermLengthMaximum' & 'puiTermLengthMinimum' parameters passed to 'iSrchInfoGetTermLengthInfo'.");
        return (SRCH_ReturnParameterError);
    }


    /* Get the term length maximum */
    if ( puiTermLengthMaximum != NULL ) {
        
        /* Get the term length max information value */
        if ( (iError = iUtlConfigGetValue(psiSrchIndex->pvUtlIndexInformation, SRCH_INFO_TERM_LENGTH_MAXIMUM_INFO_KEY, pucConfigValue, SRCH_INFO_SYMBOL_MAXIMUM_LENGTH + 1)) != UTL_NoError ) {
            return (SRCH_InfoSymbolNotFound);
        }
        
        /* Set the term length maximum return pointer */
        *puiTermLengthMaximum = s_strtol(pucConfigValue, NULL, 10);
    }


    /* Get the term length minimum */
    if ( puiTermLengthMinimum != NULL ) {

        /* Get the term length min information value */
        if ( (iError = iUtlConfigGetValue(psiSrchIndex->pvUtlIndexInformation, SRCH_INFO_TERM_LENGTH_MINIMUM_INFO_KEY, pucConfigValue, SRCH_INFO_SYMBOL_MAXIMUM_LENGTH + 1)) != UTL_NoError ) {
            return (SRCH_InfoSymbolNotFound);
        }
        
        /* Set the term length minimum return pointer */
        *puiTermLengthMinimum = s_strtol(pucConfigValue, NULL, 10);
    }


    return (SRCH_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchInfoSetTermLengthInfo()

    Purpose:    Write the index term length information to the information file

    Parameters: psiSrchIndex    search index structure

    Globals:    none

    Returns:    SRCH error code

*/
int iSrchInfoSetTermLengthInfo
(
    struct srchIndex *psiSrchIndex
)
{

    int             iError = SRCH_NoError;
    unsigned char   pucConfigValue[SRCH_INFO_SYMBOL_MAXIMUM_LENGTH + 1] = {'\0'};


    /* Check the parameters */
    if ( psiSrchIndex == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'psiSrchIndex' parameter passed to 'iSrchInfoSetTermLengthInfo'."); 
        return (SRCH_InvalidIndex);
    }


    /* Add the term length maximum information entry */
    snprintf(pucConfigValue, SRCH_INFO_SYMBOL_MAXIMUM_LENGTH + 1, "%u", psiSrchIndex->uiTermLengthMaximum);
    if ( (iError = iUtlConfigAddEntry(psiSrchIndex->pvUtlIndexInformation, SRCH_INFO_TERM_LENGTH_MAXIMUM_INFO_KEY, pucConfigValue)) != UTL_NoError ) {
        return (SRCH_InfoSymbolNotFound);
    }


    /* Add the term length minimum information entry */
    snprintf(pucConfigValue, SRCH_INFO_SYMBOL_MAXIMUM_LENGTH + 1, "%u", psiSrchIndex->uiTermLengthMinimum);
    if ( (iError = iUtlConfigAddEntry(psiSrchIndex->pvUtlIndexInformation, SRCH_INFO_TERM_LENGTH_MINIMUM_INFO_KEY, pucConfigValue)) != UTL_NoError ) {
        return (SRCH_InfoSymbolNotFound);
    }


    return (SRCH_NoError);

}


/*---------------------------------------------------------------------------*/


/* 
** ===================================
** ===  Field Information Support  ===
** ===================================
*/


/*

    Function:   iSrchInfoGetFieldID()

    Purpose:    Get the field ID for a field name from the information file

    Parameters: psiSrchIndex    search index structure
                pucFieldName    field name
                puiFieldID      return pointer for the field ID

    Globals:    none

    Returns:    SRCH error code

*/
int iSrchInfoGetFieldID
(
    struct srchIndex *psiSrchIndex,
    unsigned char *pucFieldName,
    unsigned int *puiFieldID
)
{

    int             iError = SRCH_NoError;
    unsigned char   pucSymbolValue[SRCH_INFO_SYMBOL_MAXIMUM_LENGTH + 1] = {'\0'};


    /* Check the parameters */
    if ( psiSrchIndex == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'psiSrchIndex' parameter passed to 'iSrchInfoGetFieldID'."); 
        return (SRCH_InvalidIndex);
    }

    if ( bUtlStringsIsStringNULL(pucFieldName) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pucFieldName' parameter passed to 'iSrchInfoGetFieldID'."); 
        return (SRCH_InfoInvalidFieldName);
    }

    if ( puiFieldID == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'puiFieldID' parameter passed to 'iSrchInfoGetFieldID'."); 
        return (SRCH_ReturnParameterError);
    }


    /* Get the field ID information value */
    if ( (iError = iUtlConfigGetValue1(psiSrchIndex->pvUtlIndexInformation, SRCH_INFO_FIELD_ID_INFO_KEY, pucFieldName, 
            pucSymbolValue, SRCH_INFO_SYMBOL_MAXIMUM_LENGTH + 1)) != UTL_NoError ) {
        return (SRCH_InfoInvalidFieldName);
    }

    /* Convert the field ID information value to a field ID number and set the return pointer */
    *puiFieldID = s_strtol(pucSymbolValue, NULL, 10);


    return (SRCH_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchInfoGetFieldInfo()

    Purpose:    Get the field name, field description, field type and field option 
                for a field ID from the information file

    Parameters: psiSrchIndex                search index structure
                uiFieldID                   field ID of the field we want to get information for
                pucFieldName                return pointer for the field name (optional)
                uiFieldNameLength           length of the return pointer for the field names (optional)
                pucFieldDescription         return pointer for the field description (optional)
                uiFieldDescriptionLength    length of the return pointer for the field description (optional)
                puiFieldType                return pointer for the field type (optional)
                puiFieldOptions             return pointer for the field options bitmap (optional)

    Globals:    none

    Returns:    SRCH error code

*/
int iSrchInfoGetFieldInfo
(
    struct srchIndex *psiSrchIndex,
    unsigned int uiFieldID,
    unsigned char *pucFieldName,
    unsigned int uiFieldNameLength,
    unsigned char *pucFieldDescription,
    unsigned int uiFieldDescriptionLength,
    unsigned int *puiFieldType,
    unsigned int *puiFieldOptions
)
{

    int             iError = SRCH_NoError;
    unsigned char   pucConfigSubKey[SRCH_INFO_SYMBOL_MAXIMUM_LENGTH + 1] = {'\0'};
    unsigned char   pucConfigValue[SRCH_INFO_SYMBOL_MAXIMUM_LENGTH + 1] = {'\0'};



    /* Check the parameters */
    if ( psiSrchIndex == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'psiSrchIndex' parameter passed to 'iSrchInfoGetFieldID'."); 
        return (SRCH_InvalidIndex);
    }

    if ( (pucFieldName == NULL) && (pucFieldDescription == NULL) && (puiFieldType == NULL) && (puiFieldOptions == NULL) ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pucFieldName' & 'pucFieldDescription' & 'puiFieldType' & 'puiFieldOptions' parameters passed to 'iSrchInfoGetFieldID'.");
        return (SRCH_ReturnParameterError);
    }

    if ( (pucFieldName != NULL) && (uiFieldNameLength <= 0) ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiFieldNameLength' parameter passed to 'iSrchInfoGetFieldID'.");
        return (SRCH_ReturnParameterError);
    }

    if ( (pucFieldDescription != NULL) && (uiFieldDescriptionLength <= 0) ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiFieldDescriptionLength' parameter passed to 'iSrchInfoGetFieldID'.");
        return (SRCH_ReturnParameterError);
    }



    /* Create the information appender */
    snprintf(pucConfigSubKey, SRCH_INFO_SYMBOL_MAXIMUM_LENGTH + 1, "%u", uiFieldID);


    /* Get the field name information value */
    if ( pucFieldName != NULL ) {
        if ( (iError = iUtlConfigGetValue1(psiSrchIndex->pvUtlIndexInformation, SRCH_INFO_FIELD_NAME_INFO_KEY, pucConfigSubKey, 
                pucFieldName, uiFieldNameLength)) != UTL_NoError ) {
            return (SRCH_InfoInvalidFieldID);
        }
    }


    /* Get the field description, note that there may not be a description so we ignore any returned errors */
    if ( pucFieldDescription != NULL ) {
        iUtlConfigGetValue1(psiSrchIndex->pvUtlIndexInformation, SRCH_INFO_FIELD_DESCRIPTION_INFO_KEY, pucConfigSubKey, 
                pucFieldDescription, uiFieldDescriptionLength);
    }


    /* Get the field type */
    if ( puiFieldType != NULL ) {
        
        /* Get the field type information value */
        if ( (iError = iUtlConfigGetValue1(psiSrchIndex->pvUtlIndexInformation, SRCH_INFO_FIELD_TYPE_INFO_KEY, pucConfigSubKey, 
                pucConfigValue, SRCH_INFO_SYMBOL_MAXIMUM_LENGTH + 1)) != UTL_NoError ) {
            return (SRCH_InfoInvalidFieldID);
        }
        
        /* Match the field type symbol to the field type ID */
        if ( s_strcmp(pucConfigValue, SRCH_INFO_FIELD_TYPE_NONE_STRING) == 0 ) {
            *puiFieldType = SRCH_INFO_FIELD_TYPE_NONE_ID;
        }
        else if ( s_strcmp(pucConfigValue, SRCH_INFO_FIELD_TYPE_CHAR_STRING) == 0 ) {
            *puiFieldType = SRCH_INFO_FIELD_TYPE_CHAR_ID;
        }
        else if ( s_strcmp(pucConfigValue, SRCH_INFO_FIELD_TYPE_INT_STRING) == 0 ) {
            *puiFieldType = SRCH_INFO_FIELD_TYPE_INT_ID;
        }
        else if ( s_strcmp(pucConfigValue, SRCH_INFO_FIELD_TYPE_LONG_STRING) == 0 ) {
            *puiFieldType = SRCH_INFO_FIELD_TYPE_LONG_ID;
        }
        else if ( s_strcmp(pucConfigValue, SRCH_INFO_FIELD_TYPE_FLOAT_STRING) == 0 ) {
            *puiFieldType = SRCH_INFO_FIELD_TYPE_FLOAT_ID;
        }
        else if ( s_strcmp(pucConfigValue, SRCH_INFO_FIELD_TYPE_DOUBLE_STRING) == 0 ) {
            *puiFieldType = SRCH_INFO_FIELD_TYPE_DOUBLE_ID;
        }
        else {
            /* Failed the match, return an error */
            return (SRCH_InfoInvalidFieldType);
        }
    }


    /* Get the field options */
    if ( puiFieldOptions != NULL ) {
        
        unsigned char   *pucFieldOptionPtr = NULL;
        unsigned char   *pucFieldOptionStrtokPtr = NULL;

        /* Get the field options information value */
        if ( (iError = iUtlConfigGetValue1(psiSrchIndex->pvUtlIndexInformation, SRCH_INFO_FIELD_OPTIONS_INFO_KEY, pucConfigSubKey, 
                pucConfigValue, SRCH_INFO_SYMBOL_MAXIMUM_LENGTH + 1)) != UTL_NoError ) {
            return (SRCH_InfoInvalidFieldID);
        }

        /* Parse out the options, setting the field options bitmap */
        for ( pucFieldOptionPtr = (unsigned char *)s_strtok_r(pucConfigValue, SRCH_INFO_FIELD_OPTIONS_SEPARATORS, (char **)&pucFieldOptionStrtokPtr); 
                pucFieldOptionPtr != NULL; 
                pucFieldOptionPtr = (unsigned char *)s_strtok_r(NULL, SRCH_INFO_FIELD_OPTIONS_SEPARATORS, (char **)&pucFieldOptionStrtokPtr) ) {

            /* Switch stemming on */
            if ( s_strcmp(SRCH_INFO_FIELD_OPTION_STEMMING_ON_STRING, pucFieldOptionPtr) == 0 ) {
                vSrchInfoFieldOptionSetStemmingOn(*puiFieldOptions);
            }
            
            /* Switch stemming off */
            else if ( s_strcmp(SRCH_INFO_FIELD_OPTION_STEMMING_OFF_STRING, pucFieldOptionPtr) == 0 ) {
                vSrchInfoFieldOptionSetStemmingOff(*puiFieldOptions);
            }
            
            /* Switch stop term on */
            else if ( s_strcmp(SRCH_INFO_FIELD_OPTION_STOP_TERM_ON_STRING, pucFieldOptionPtr) == 0 ) {
                vSrchInfoFieldOptionSetStopTermOn(*puiFieldOptions);
            }
            
            /* Switch stop term off */
            else if ( s_strcmp(SRCH_INFO_FIELD_OPTION_STOP_TERM_OFF_STRING, pucFieldOptionPtr) == 0 ) {
                vSrchInfoFieldOptionSetStopTermOff(*puiFieldOptions);
            }
            
            /* Switch term position on */
            else if ( s_strcmp(SRCH_INFO_FIELD_OPTION_TERM_POSITION_ON_STRING, pucFieldOptionPtr) == 0 ) {
                vSrchInfoFieldOptionSetTermPositionOn(*puiFieldOptions);
            }
            
            /* Switch term position off */
            else if ( s_strcmp(SRCH_INFO_FIELD_OPTION_TERM_POSITION_OFF_STRING, pucFieldOptionPtr) == 0 ) {
                vSrchInfoFieldOptionSetTermPositionOff(*puiFieldOptions);
            }
            
            else {
                /* Failed the match, return an error */
                return (SRCH_InfoInvalidFieldOptions);
            }
        }
    }


    return (SRCH_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchInfoSetFieldInfo()

    Purpose:    Write the field id/name/desc

    Parameters: psiSrchIndex            search index structure
                uiFieldID               field ID
                pucFieldName            field name
                pucFieldDescription     field description (optional)
                uiFieldType             field type
                uiFieldOptions          field option bitmap

    Globals:    none

    Returns:    SRCH error code

*/
int iSrchInfoSetFieldInfo
(
    struct srchIndex *psiSrchIndex,
    unsigned int uiFieldID,
    unsigned char *pucFieldName,
    unsigned char *pucFieldDescription,
    unsigned int uiFieldType,
    unsigned int uiFieldOptions
)
{

    int             iError = SRCH_NoError;
    unsigned char   pucConfigSubKey[SRCH_INFO_SYMBOL_MAXIMUM_LENGTH + 1] = {'\0'};
    unsigned char   pucConfigValue[SRCH_INFO_SYMBOL_MAXIMUM_LENGTH + 1] = {'\0'};


    /* Check the parameters */
    if ( psiSrchIndex == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'psiSrchIndex' parameter passed to 'iSrchInfoSetFieldInfo'."); 
        return (SRCH_InvalidIndex);
    }

    if ( bUtlStringsIsStringNULL(pucFieldName) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null or invalid 'pucFieldName' parameter passed to 'iSrchInfoSetFieldInfo'."); 
        return (SRCH_InfoInvalidFieldName);
    }
    
    if ( SRCH_INFO_FIELD_TYPE_VALID(uiFieldType) == false ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiFieldType' parameter passed to 'iSrchInfoSetFieldInfo'."); 
        return (SRCH_InfoInvalidFieldName);
    }
    
    if ( uiFieldOptions < 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiFieldOptions' parameter passed to 'iSrchInfoSetFieldInfo'."); 
        return (SRCH_InfoInvalidFieldOptions);
    }



    /* Create the information appender */
    snprintf(pucConfigSubKey, SRCH_INFO_SYMBOL_MAXIMUM_LENGTH + 1, "%u", uiFieldID);


    /* Add the field ID information entry */
    snprintf(pucConfigValue, SRCH_INFO_SYMBOL_MAXIMUM_LENGTH + 1, "%u", uiFieldID);
    if ( (iError = iUtlConfigAddEntry1(psiSrchIndex->pvUtlIndexInformation, SRCH_INFO_FIELD_ID_INFO_KEY, pucFieldName, pucConfigValue)) != UTL_NoError ) {
        return (SRCH_InfoSymbolSetFailed);
    }

    /* Add the field name information entry */
    if ( (iError = iUtlConfigAddEntry1(psiSrchIndex->pvUtlIndexInformation, SRCH_INFO_FIELD_NAME_INFO_KEY, pucConfigSubKey, pucFieldName)) != UTL_NoError ) {
        return (SRCH_InfoSymbolSetFailed);
    }

    /* Add the field description information entry */
    if ( bUtlStringsIsStringNULL(pucFieldDescription) == false ) {
        if ( (iError = iUtlConfigAddEntry1(psiSrchIndex->pvUtlIndexInformation, SRCH_INFO_FIELD_DESCRIPTION_INFO_KEY, pucConfigSubKey, pucFieldDescription)) != UTL_NoError ) {
            return (SRCH_InfoSymbolSetFailed);
        }
    }
    

    /* Add the field type information entry */
    if ( uiFieldType == SRCH_INFO_FIELD_TYPE_NONE_ID ) {
        iError = iUtlConfigAddEntry1(psiSrchIndex->pvUtlIndexInformation, SRCH_INFO_FIELD_TYPE_INFO_KEY, pucConfigSubKey, SRCH_INFO_FIELD_TYPE_NONE_STRING);
    }
    else if ( uiFieldType == SRCH_INFO_FIELD_TYPE_CHAR_ID ) {
        iError = iUtlConfigAddEntry1(psiSrchIndex->pvUtlIndexInformation, SRCH_INFO_FIELD_TYPE_INFO_KEY, pucConfigSubKey, SRCH_INFO_FIELD_TYPE_CHAR_STRING);
    }
    else if ( uiFieldType == SRCH_INFO_FIELD_TYPE_INT_ID ) {
        iError = iUtlConfigAddEntry1(psiSrchIndex->pvUtlIndexInformation, SRCH_INFO_FIELD_TYPE_INFO_KEY, pucConfigSubKey, SRCH_INFO_FIELD_TYPE_INT_STRING);
    }
    else if ( uiFieldType == SRCH_INFO_FIELD_TYPE_LONG_ID ) {
        iError = iUtlConfigAddEntry1(psiSrchIndex->pvUtlIndexInformation, SRCH_INFO_FIELD_TYPE_INFO_KEY, pucConfigSubKey, SRCH_INFO_FIELD_TYPE_LONG_STRING);
    }
    else if ( uiFieldType == SRCH_INFO_FIELD_TYPE_FLOAT_ID ) {
        iError = iUtlConfigAddEntry1(psiSrchIndex->pvUtlIndexInformation, SRCH_INFO_FIELD_TYPE_INFO_KEY, pucConfigSubKey, SRCH_INFO_FIELD_TYPE_FLOAT_STRING);
    }
    else if ( uiFieldType == SRCH_INFO_FIELD_TYPE_DOUBLE_ID ) {
        iError = iUtlConfigAddEntry1(psiSrchIndex->pvUtlIndexInformation, SRCH_INFO_FIELD_TYPE_INFO_KEY, pucConfigSubKey, SRCH_INFO_FIELD_TYPE_DOUBLE_STRING);
    }
    else {
        /* Failed the match, return an error */
        return (SRCH_InfoInvalidFieldType);
    }

    /* Check the error */
    if ( iError != UTL_NoError ) {
        return (SRCH_InfoSymbolSetFailed);
    }


    /* Add the field options information entry */
    sprintf(pucConfigValue, "%s,%s,%s", ((bSrchInfoFieldOptionStemming(uiFieldOptions) == true) ? SRCH_INFO_FIELD_OPTION_STEMMING_ON_STRING : SRCH_INFO_FIELD_OPTION_STEMMING_OFF_STRING),
            ((bSrchInfoFieldOptionStopTerm(uiFieldOptions) == true) ? SRCH_INFO_FIELD_OPTION_STOP_TERM_ON_STRING : SRCH_INFO_FIELD_OPTION_STOP_TERM_OFF_STRING),
            ((bSrchInfoFieldOptionTermPosition(uiFieldOptions) == true) ? SRCH_INFO_FIELD_OPTION_TERM_POSITION_ON_STRING : SRCH_INFO_FIELD_OPTION_TERM_POSITION_OFF_STRING));
    if ( (iError = iUtlConfigAddEntry1(psiSrchIndex->pvUtlIndexInformation, SRCH_INFO_FIELD_OPTIONS_INFO_KEY, pucConfigSubKey, pucConfigValue)) != SRCH_NoError ) {
        return (iError);
    }


    return (SRCH_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchInfoGetUnfieldedSearchFieldNames()

    Purpose:    Get the unfielded search field names
                from the information file

    Parameters: psiSrchIndex                        search index structure
                pucUnfieldedSearchFieldNames        return pointer for the unfielded search field names
                uiUnfieldedSearchFieldNamesLength   length of the return pointer for the unfielded search field names

    Globals:    none

    Returns:    SRCH error code

*/
int iSrchInfoGetUnfieldedSearchFieldNames
(
    struct srchIndex *psiSrchIndex,
    unsigned char *pucUnfieldedSearchFieldNames,
    unsigned int uiUnfieldedSearchFieldNamesLength
)
{

    int     iError = SRCH_NoError;


    /* Check the parameters */
    if ( psiSrchIndex == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'psiSrchIndex' parameter passed to 'iSrchInfoGetUnfieldedSearchFieldNames'."); 
        return (SRCH_InvalidIndex);
    }

    if ( pucUnfieldedSearchFieldNames == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pucUnfieldedSearchFieldNames' parameter passed to 'iSrchInfoGetUnfieldedSearchFieldNames'."); 
        return (SRCH_ReturnParameterError);
    }

    if ( uiUnfieldedSearchFieldNamesLength <= 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiUnfieldedSearchFieldNamesLength' parameter passed to 'iSrchInfoGetUnfieldedSearchFieldNames'."); 
        return (SRCH_ReturnParameterError);
    }


    /* Get the unfielded search field names information value */
    if ( (iError = iUtlConfigGetValue(psiSrchIndex->pvUtlIndexInformation, SRCH_INFO_UNFIELDED_SEARCH_FIELD_NAMES_INFO_KEY,
            pucUnfieldedSearchFieldNames, uiUnfieldedSearchFieldNamesLength)) != UTL_NoError ) {
        return (SRCH_InfoSymbolNotFound);
    }


    return (SRCH_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchInfoSetUnfieldedSearchFieldNames()

    Purpose:    Write the unfielded search field names

    Parameters: psiSrchIndex                    search index structure
                pucUnfieldedSearchFieldNames    unfielded search field names

    Globals:    none

    Returns:    SRCH error code

*/
int iSrchInfoSetUnfieldedSearchFieldNames
(
    struct srchIndex *psiSrchIndex,
    unsigned char *pucUnfieldedSearchFieldNames
)
{

    int     iError = SRCH_NoError;


    /* Check the parameters */
    if ( psiSrchIndex == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'psiSrchIndex' parameter passed to 'iSrchInfoSetUnfieldedSearchFieldNames'."); 
        return (SRCH_InvalidIndex);
    }

    if ( bUtlStringsIsStringNULL(pucUnfieldedSearchFieldNames) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pucUnfieldedSearchFieldNames' parameter passed to 'iSrchInfoSetUnfieldedSearchFieldNames'."); 
        return (SRCH_InfoInvalidUnfieldedSearchFieldNames);
    }
    

    /* Add the unfielded search field names information entry */
    if ( (iError = iUtlConfigAddEntry(psiSrchIndex->pvUtlIndexInformation, SRCH_INFO_UNFIELDED_SEARCH_FIELD_NAMES_INFO_KEY, pucUnfieldedSearchFieldNames)) != UTL_NoError ) {
        return (SRCH_InfoSymbolSetFailed);
    }


    return (SRCH_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchInfoGetFieldOptionDefaults()

    Purpose:    Get the default field options for this database

    Parameters: psiSrchIndex        search index structure
                puiFieldOptions     return pointer for the field options bitmap

    Globals:    none

    Returns:    SRCH error code

*/
int iSrchInfoGetFieldOptionDefaults
(
    struct srchIndex *psiSrchIndex,
    unsigned int *puiFieldOptions
)
{

    /* Check the parameters */
    if ( psiSrchIndex == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'psiSrchIndex' parameter passed to 'iSrchInfoGetFieldOptionDefaults'."); 
        return (SRCH_InvalidIndex);
    }

    if ( puiFieldOptions == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'puiFieldOptions' parameter passed to 'iSrchInfoGetFieldOptionDefaults'."); 
        return (SRCH_InvalidIndex);
    }


    /* Clear the field options */
    *puiFieldOptions = SRCH_INFO_FIELD_OPTION_NONE;


    /* Set the stemming field option based the index stemming state */
    if ( bSrchStemmerOn(psiSrchIndex) == true ) {
        vSrchInfoFieldOptionSetStemmingOn(*puiFieldOptions);
    }
    else {
        vSrchInfoFieldOptionSetStemmingOff(*puiFieldOptions);
    }
    
    
    /* Set the stemming field option based the index stemming state */
    if ( bSrchStopListOn(psiSrchIndex) == true ) {
        vSrchInfoFieldOptionSetStopTermOn(*puiFieldOptions);
    }
    else {
        vSrchInfoFieldOptionSetStopTermOff(*puiFieldOptions);
    }


    /* Term position is on by default */
    vSrchInfoFieldOptionSetTermPositionOn(*puiFieldOptions);


    return (SRCH_NoError);

}


/*---------------------------------------------------------------------------*/


/* 
** =======================================
** ===  Item Info Information Support  ===
** =======================================
*/


/*

    Function:   iSrchInfoGetItemInfo()

    Purpose:    Get the item info for an item ID
                from the information file

    Parameters: psiSrchIndex        search index structure
                uiItemID            item ID of the item we want to get information for
                pucItemName         return pointer for the item name (optional)
                uiItemNameLength    length of the return pointer for the item name (optional)
                pucMimeType         return pointer for the mime type (optional)
                uiMimeTypeLength    length of the return pointer for the mime type (optional)

    Globals:    none

    Returns:    SRCH error code

*/
int iSrchInfoGetItemInfo
(
    struct srchIndex *psiSrchIndex,
    unsigned int uiItemID,
    unsigned char *pucItemName,
    unsigned int uiItemNameLength,
    unsigned char *pucMimeType,
    unsigned int uiMimeTypeLength
)
{

    int             iError = SRCH_NoError;
    unsigned char   pucScanfFormat[UTL_STRING_SCANF_FORMAT_LENGTH + 1] = {'0'};
    unsigned char   pucSymbolAppender[SRCH_INFO_SYMBOL_MAXIMUM_LENGTH + 1] = {'0'};
    unsigned char   pucConfigValue[SRCH_INFO_SYMBOL_MAXIMUM_LENGTH + 1] = {'0'};


    /* Check the parameters */
    if ( psiSrchIndex == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'psiSrchIndex' parameter passed to 'iSrchInfoGetItemInfo'."); 
        return (SRCH_InvalidIndex);
    }

    if ( (pucItemName == NULL) && (pucMimeType == NULL) ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pucItemName' & 'pucMimeType' parameters passed to 'iSrchInfoGetItemInfo'."); 
        return (SRCH_ReturnParameterError);
    }

    if ( (pucItemName == NULL) && (uiItemNameLength <= 0) ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiItemNameLength' parameter passed to 'iSrchInfoGetItemInfo'.");
        return (SRCH_ReturnParameterError);
    }

    if ( (pucMimeType == NULL) && (uiMimeTypeLength <= 0) ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiMimeTypeLength' parameter passed to 'iSrchInfoGetItemInfo'.");
        return (SRCH_ReturnParameterError);
    }


    /* Get the item info  information value */
    if ( (pucItemName != NULL) || (pucMimeType != NULL) ) {
        snprintf(pucSymbolAppender, SRCH_INFO_SYMBOL_MAXIMUM_LENGTH + 1, "%u", uiItemID);
        if ( (iError = iUtlConfigGetValue1(psiSrchIndex->pvUtlIndexInformation, SRCH_INFO_ITEM_INFO_KEY, pucSymbolAppender, 
                pucConfigValue, SRCH_INFO_SYMBOL_MAXIMUM_LENGTH + 1)) != UTL_NoError ) {
            return (SRCH_InfoInvalidItemID);
        }
    }

    /* Parse the symbol - ignore errors */
    if ( pucItemName != NULL ) {
        snprintf(pucScanfFormat, UTL_STRING_SCANF_FORMAT_LENGTH + 1, "%%%u[^:]", uiItemNameLength);
        sscanf(pucConfigValue, pucScanfFormat, pucItemName);
    }

    if ( pucMimeType != NULL ) {
        snprintf(pucScanfFormat, UTL_STRING_SCANF_FORMAT_LENGTH + 1, "%%*[^:]:%%%us", uiMimeTypeLength);
        sscanf(pucConfigValue, pucScanfFormat, pucMimeType);
    }


    return (SRCH_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchInfoSetItemInfo()

    Purpose:    Write the item info to the information file

    Parameters: psiSrchIndex    search index structure
                uiItemID        item ID
                pucItemName     item name
                pucMimeType     mime type

    Globals:    none

    Returns:    SRCH error code

*/
int iSrchInfoSetItemInfo
(
    struct srchIndex *psiSrchIndex,
    unsigned int uiItemID,
    unsigned char *pucItemName,
    unsigned char *pucMimeType
)
{

    int             iError = SRCH_NoError;
    unsigned char   pucConfigSubKey[SRCH_INFO_SYMBOL_MAXIMUM_LENGTH + 1] = {'\0'};
    unsigned char   pucConfigValue[SRCH_INFO_SYMBOL_MAXIMUM_LENGTH + 1] = {'\0'};


    /* Check the parameters */
    if ( psiSrchIndex == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'psiSrchIndex' parameter passed to 'iSrchInfoSetItemInfo'."); 
        return (SRCH_InvalidIndex);
    }

    if ( bUtlStringsIsStringNULL(pucItemName) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pucItemName' parameter passed to 'iSrchInfoSetItemInfo'."); 
        return (SRCH_InfoInvalidItemName);
    }

    if ( bUtlStringsIsStringNULL(pucMimeType) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pucMimeType' parameter passed to 'iSrchInfoSetItemInfo'."); 
        return (SRCH_InfoInvalidMimeTypeName);
    }


    /* Add the item info information entry */
    snprintf(pucConfigSubKey, SRCH_INFO_SYMBOL_MAXIMUM_LENGTH + 1, "%u", uiItemID);
    snprintf(pucConfigValue, SRCH_INFO_SYMBOL_MAXIMUM_LENGTH + 1, "%s:%s", pucItemName, pucMimeType);
    if ( (iError = iUtlConfigAddEntry1(psiSrchIndex->pvUtlIndexInformation, SRCH_INFO_ITEM_INFO_KEY, pucConfigSubKey, pucConfigValue)) != UTL_NoError ) {
        return (SRCH_InfoSymbolSetFailed);
    }


    return (SRCH_NoError);

}


/*---------------------------------------------------------------------------*/


/* 
** =========================================
** ===  Description Information Support  ===
** =========================================
*/


/*

    Function:   iSrchInfoGetDescriptionInfo()

    Purpose:    Get the description information from the information file

    Parameters: psiSrchIndex            search index structure
                pucIndexDescription     return pointer for the index description
                ulIndexDescription      length of the return pointer for the index description

    Globals:    none

    Returns:    SRCH error code

*/
int iSrchInfoGetDescriptionInfo
(
    struct srchIndex *psiSrchIndex,
    unsigned char *pucIndexDescription,
    unsigned int uiIndexDescriptionLength
)
{

    int     iError = SRCH_NoError;


    /* Check the parameters */
    if ( psiSrchIndex == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'psiSrchIndex' parameter passed to 'iSrchInfoGetDescriptionInfo'."); 
        return (SRCH_InvalidIndex);
    }

    if ( pucIndexDescription == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pucIndexDescription' parameter passed to 'iSrchInfoGetDescriptionInfo'."); 
        return (SRCH_ReturnParameterError);
    }

    if ( uiIndexDescriptionLength <= 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiIndexDescriptionLength' parameter passed to 'iSrchInfoGetDescriptionInfo'."); 
        return (SRCH_ReturnParameterError);
    }


    /* Get the index description information value */
    if ( (iError = iUtlConfigGetValue(psiSrchIndex->pvUtlIndexInformation, SRCH_INFO_INDEX_DESCRIPTION_INFO_KEY,
            pucIndexDescription, uiIndexDescriptionLength)) != UTL_NoError ) {
        return (SRCH_InfoSymbolNotFound);
    }


    return (SRCH_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchInfoSetDescriptionInfo()

    Purpose:    Write the description information to the information file

    Parameters: psiSrchIndex            search index structure
                pucIndexDescription     index description

    Globals:    none

    Returns:    SRCH error code

*/
int iSrchInfoSetDescriptionInfo
(
    struct srchIndex *psiSrchIndex,
    unsigned char *pucIndexDescription
)
{

    int     iError = SRCH_NoError;


    /* Check the parameters */
    if ( psiSrchIndex == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'psiSrchIndex' parameter passed to 'iSrchInfoSetDescriptionInfo'."); 
        return (SRCH_InvalidIndex);
    }

    if ( bUtlStringsIsStringNULL(pucIndexDescription) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pucIndexDescription' parameter passed to 'iSrchInfoSetDescriptionInfo'."); 
        return (SRCH_InfoInvalidIndexDescription);
    }


    /* Add the index description information entry */
    if ( (iError = iUtlConfigAddEntry(psiSrchIndex->pvUtlIndexInformation, SRCH_INFO_INDEX_DESCRIPTION_INFO_KEY, pucIndexDescription)) != UTL_NoError ) {
        return (SRCH_InfoSymbolSetFailed);
    }


    return (SRCH_NoError);

}


/*---------------------------------------------------------------------------*/


/* 
** =====================================
** ===  Scalars Information Support  ===
** =====================================
*/


/*

    Function:   iSrchInfoGetScalars()

    Purpose:    Get the scalars from the information file

    Parameters: psiSrchIndex    search index structure

    Globals:    none

    Returns:    SRCH error code

*/
int iSrchInfoGetScalars
(
    struct srchIndex *psiSrchIndex
)
{

    int             iError = SRCH_NoError;
    unsigned char   pucConfigValue[SRCH_INFO_SYMBOL_MAXIMUM_LENGTH + 1] = {'\0'};
    

    /* Check the parameters */
    if ( psiSrchIndex == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'psiSrchIndex' parameter passed to 'iSrchInfoGetScalars'."); 
        return (SRCH_InvalidIndex);
    }



    /* Get the unique term count information value */
    if ( (iError = iUtlConfigGetValue(psiSrchIndex->pvUtlIndexInformation, SRCH_INFO_UNIQUE_TERM_COUNT_INFO_KEY, pucConfigValue, SRCH_INFO_SYMBOL_MAXIMUM_LENGTH + 1)) != UTL_NoError ) {
        return (SRCH_InfoSymbolNotFound);
    }
    
    /* Set the unique term count */
    psiSrchIndex->ulUniqueTermCount = s_strtol(pucConfigValue, NULL, 10);



    /* Get the total term count information value */
    if ( (iError = iUtlConfigGetValue(psiSrchIndex->pvUtlIndexInformation, SRCH_INFO_TOTAL_TERM_COUNT_INFO_KEY, pucConfigValue, SRCH_INFO_SYMBOL_MAXIMUM_LENGTH + 1)) != UTL_NoError ) {
        return (SRCH_InfoSymbolNotFound);
    }
    
    /* Set the total term count */
    psiSrchIndex->ulTotalTermCount = s_strtol(pucConfigValue, NULL, 10);



    /* Get the unique stop term count information value */
    if ( (iError = iUtlConfigGetValue(psiSrchIndex->pvUtlIndexInformation, SRCH_INFO_UNIQUE_STOP_TERM_COUNT_INFO_KEY, pucConfigValue, SRCH_INFO_SYMBOL_MAXIMUM_LENGTH + 1)) != UTL_NoError ) {
        return (SRCH_InfoSymbolNotFound);
    }
    
    /* Set the unique stop term count */
    psiSrchIndex->ulUniqueStopTermCount = s_strtol(pucConfigValue, NULL, 10);



    /* Get the total stop term count information value */
    if ( (iError = iUtlConfigGetValue(psiSrchIndex->pvUtlIndexInformation, SRCH_INFO_TOTAL_STOP_TERM_COUNT_INFO_KEY, pucConfigValue, SRCH_INFO_SYMBOL_MAXIMUM_LENGTH + 1)) != UTL_NoError ) {
        return (SRCH_InfoSymbolNotFound);
    }
    
    /* Set the total stop term count */
    psiSrchIndex->ulTotalStopTermCount = s_strtol(pucConfigValue, NULL, 10);



    /* Get the document count information value */
    if ( (iError = iUtlConfigGetValue(psiSrchIndex->pvUtlIndexInformation, SRCH_INFO_DOCUMENT_COUNT_INFO_KEY, pucConfigValue, SRCH_INFO_SYMBOL_MAXIMUM_LENGTH + 1)) != UTL_NoError ) {
        return (SRCH_InfoSymbolNotFound);
    }
    
    /* Set the document count */
    psiSrchIndex->uiDocumentCount = s_strtol(pucConfigValue, NULL, 10);



    /* Get the document term count maximum information value */
    if ( (iError = iUtlConfigGetValue(psiSrchIndex->pvUtlIndexInformation, SRCH_INFO_DOCUMENT_TERM_COUNT_MAXIMUM_INFO_KEY, pucConfigValue, SRCH_INFO_SYMBOL_MAXIMUM_LENGTH + 1)) != UTL_NoError ) {
        return (SRCH_InfoSymbolNotFound);
    }
    
    /* Set the document term count maximum */
    psiSrchIndex->uiDocumentTermCountMaximum = s_strtol(pucConfigValue, NULL, 10);



    /* Get the document term count minimum information value */
    if ( (iError = iUtlConfigGetValue(psiSrchIndex->pvUtlIndexInformation, SRCH_INFO_DOCUMENT_TERM_COUNT_MINIMUM_INFO_KEY, pucConfigValue, SRCH_INFO_SYMBOL_MAXIMUM_LENGTH + 1)) != UTL_NoError ) {
        return (SRCH_InfoSymbolNotFound);
    }
    
    /* Set the document term count minimum */
    psiSrchIndex->uiDocumentTermCountMinimum = s_strtol(pucConfigValue, NULL, 10);



    /* Get the maximum field ID information value */
    if ( (iError = iUtlConfigGetValue(psiSrchIndex->pvUtlIndexInformation, SRCH_INFO_MAXIMUM_FIELD_ID_INFO_KEY, pucConfigValue, SRCH_INFO_SYMBOL_MAXIMUM_LENGTH + 1)) != UTL_NoError ) {
        return (SRCH_InfoSymbolNotFound);
    }
    
    /* Set the maximum field ID minimum */
    psiSrchIndex->uiFieldIDMaximum = s_strtol(pucConfigValue, NULL, 10);



    /* Get the last update time information value */
    if ( (iError = iUtlConfigGetValue(psiSrchIndex->pvUtlIndexInformation, SRCH_INFO_LAST_UPDATE_TIME_INFO_KEY, pucConfigValue, SRCH_INFO_SYMBOL_MAXIMUM_LENGTH + 1)) != UTL_NoError ) {
        return (SRCH_InfoSymbolNotFound);
    }
    
    /* Set the last update time */
    psiSrchIndex->tLastUpdateTime = (time_t)s_strtol(pucConfigValue, NULL, 10);


    return (SRCH_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchInfoSetScalars()

    Purpose:    Write the scalars to the information file

    Parameters: psiSrchIndex    search index structure

    Globals:    none

    Returns:    SRCH error code

*/
int iSrchInfoSetScalars
(
    struct srchIndex *psiSrchIndex
)
{

    int             iError = SRCH_NoError;
    unsigned char   pucConfigValue[SRCH_INFO_SYMBOL_MAXIMUM_LENGTH + 1] = {'\0'};


    /* Check the parameters */
    if ( psiSrchIndex == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'psiSrchIndex' parameter passed to 'iSrchInfoSetTermLengthInfo'."); 
        return (SRCH_InvalidIndex);
    }


    /* Add the unique term count information entry */
    snprintf(pucConfigValue, SRCH_INFO_SYMBOL_MAXIMUM_LENGTH + 1, "%lu", psiSrchIndex->ulUniqueTermCount);
    if ( (iError = iUtlConfigAddEntry(psiSrchIndex->pvUtlIndexInformation, SRCH_INFO_UNIQUE_TERM_COUNT_INFO_KEY, pucConfigValue)) != UTL_NoError ) {
        return (iError);
    }


    /* Add the total term count information entry */
    snprintf(pucConfigValue, SRCH_INFO_SYMBOL_MAXIMUM_LENGTH + 1, "%lu", psiSrchIndex->ulTotalTermCount);
    if ( (iError = iUtlConfigAddEntry(psiSrchIndex->pvUtlIndexInformation, SRCH_INFO_TOTAL_TERM_COUNT_INFO_KEY, pucConfigValue)) != UTL_NoError ) {
        return (iError);
    }


    /* Add the unique stop term count information entry */
    snprintf(pucConfigValue, SRCH_INFO_SYMBOL_MAXIMUM_LENGTH + 1, "%lu", psiSrchIndex->ulUniqueStopTermCount);
    if ( (iError = iUtlConfigAddEntry(psiSrchIndex->pvUtlIndexInformation, SRCH_INFO_UNIQUE_STOP_TERM_COUNT_INFO_KEY, pucConfigValue)) != UTL_NoError ) {
        return (iError);
    }


    /* Add the total stop term count information entry */
    snprintf(pucConfigValue, SRCH_INFO_SYMBOL_MAXIMUM_LENGTH + 1, "%lu", psiSrchIndex->ulTotalStopTermCount);
    if ( (iError = iUtlConfigAddEntry(psiSrchIndex->pvUtlIndexInformation, SRCH_INFO_TOTAL_STOP_TERM_COUNT_INFO_KEY, pucConfigValue)) != UTL_NoError ) {
        return (iError);
    }


    /* Add the document count information entry */
    snprintf(pucConfigValue, SRCH_INFO_SYMBOL_MAXIMUM_LENGTH + 1, "%u", psiSrchIndex->uiDocumentCount);
    if ( (iError = iUtlConfigAddEntry(psiSrchIndex->pvUtlIndexInformation, SRCH_INFO_DOCUMENT_COUNT_INFO_KEY, pucConfigValue)) != UTL_NoError ) {
        return (iError);
    }


    /* Add the document term count maximum information entry */
    snprintf(pucConfigValue, SRCH_INFO_SYMBOL_MAXIMUM_LENGTH + 1, "%u", psiSrchIndex->uiDocumentTermCountMaximum);
    if ( (iError = iUtlConfigAddEntry(psiSrchIndex->pvUtlIndexInformation, SRCH_INFO_DOCUMENT_TERM_COUNT_MAXIMUM_INFO_KEY, pucConfigValue)) != UTL_NoError ) {
        return (iError);
    }


    /* Add the document term count minimum information entry */
    snprintf(pucConfigValue, SRCH_INFO_SYMBOL_MAXIMUM_LENGTH + 1, "%u", psiSrchIndex->uiDocumentTermCountMinimum);
    if ( (iError = iUtlConfigAddEntry(psiSrchIndex->pvUtlIndexInformation, SRCH_INFO_DOCUMENT_TERM_COUNT_MINIMUM_INFO_KEY, pucConfigValue)) != UTL_NoError ) {
        return (iError);
    }


    /* Add the maximum field ID information entry */
    snprintf(pucConfigValue, SRCH_INFO_SYMBOL_MAXIMUM_LENGTH + 1, "%u", psiSrchIndex->uiFieldIDMaximum);
    if ( (iError = iUtlConfigAddEntry(psiSrchIndex->pvUtlIndexInformation, SRCH_INFO_MAXIMUM_FIELD_ID_INFO_KEY, pucConfigValue)) != UTL_NoError ) {
        return (iError);
    }


    /* Add the last update time information entry */
    snprintf(pucConfigValue, SRCH_INFO_SYMBOL_MAXIMUM_LENGTH + 1, "%ld", psiSrchIndex->tLastUpdateTime);
    if ( (iError = iUtlConfigAddEntry(psiSrchIndex->pvUtlIndexInformation, SRCH_INFO_LAST_UPDATE_TIME_INFO_KEY, pucConfigValue)) != UTL_NoError ) {
        return (iError);
    }


    return (SRCH_NoError);

}


/*---------------------------------------------------------------------------*/
