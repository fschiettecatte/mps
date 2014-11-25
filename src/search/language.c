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

    Module:     language.c

    Author:     Francois Schiettecatte (FS Consulting LLC.)

    Created:    1 May 2004

    Purpose:    This module manages the languages

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
#define UTL_LOG_CONTEXT                     (unsigned char *)"com.fsconsult.mps.src.search.language"


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchLanguageInit()

    Purpose:    Initialize the language parser

    Parameters: psiSrchIndex        The search index structure
                pucLanguageCode     Language code
                pucTokenizerName    Tokenizer name

    Globals:    none

    Returns:    SRCH error code

*/
int iSrchLanguageInit
(
    struct srchIndex *psiSrchIndex,
    unsigned char *pucLanguageCode,
    unsigned char *pucTokenizerName
)
{

    int             iError = SRCH_NoError;
    unsigned int    uiLanguageID = 0;
    unsigned int    uiTokenizerID = 0;


    /* Check the parameters */
    if ( psiSrchIndex == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'psiSrchIndex' parameter passed to 'iSrchLanguageInit'."); 
        return (SRCH_InvalidIndex);
    }
    
    if ( bUtlStringsIsStringNULL(pucLanguageCode) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pucLanguageCode' parameter passed to 'iSrchLanguageInit'."); 
        return (SRCH_ParameterError);
    }
    
    if ( bUtlStringsIsStringNULL(pucTokenizerName) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pucTokenizerName' parameter passed to 'iSrchLanguageInit'."); 
        return (SRCH_ParameterError);
    }



    /* Get the language ID */
    if ( (iError = iLngGetLanguageIDFromCode(pucLanguageCode, &uiLanguageID)) != LNG_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the language ID from the language code: '%s', lng error: %d.", pucLanguageCode, iError); 
        return (SRCH_LanguageInvalidLanguageCode);
    }

    /* Get the tokenizer ID */
    if ( (iError = iLngGetTokenizerIDFromName(pucTokenizerName, &uiTokenizerID)) != LNG_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the tokenizer ID from the tokenizer name: '%s', lng error: %d.", pucTokenizerName, iError); 
        return (SRCH_LanguageInvalidTokenizerName);
    }


    /* Set the various fields in the index structure */
    psiSrchIndex->uiLanguageID = uiLanguageID;
    psiSrchIndex->uiTokenizerID = uiTokenizerID;


    return (SRCH_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchLanguageInitFromInfo()

    Purpose:    Initialize the language parser from the information file

    Parameters: psiSrchIndex    The search index structure

    Globals:    none

    Returns:    SRCH error code

*/
int iSrchLanguageInitFromInfo
(
    struct srchIndex *psiSrchIndex
)
{

    int             iError = SRCH_NoError;
    unsigned char   pucLanguageCode[SPI_INDEX_LANGUAGE_MAXIMUM_LENGTH + 1] = {'\0'};
    unsigned char   pucTokenizerName[SPI_INDEX_TOKENIZER_MAXIMUM_LENGTH + 1] = {'\0'};
    unsigned int    uiLanguageID = 0;
    unsigned int    uiTokenizerID = 0;


    /* Check the parameters */
    if ( psiSrchIndex == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'psiSrchIndex' parameter passed to 'iSrchLanguageInit'."); 
        return (SRCH_InvalidIndex);
    }



    /* Get the language code from the information file */
    if ( (iError = iSrchInfoGetLanguageInfo(psiSrchIndex, pucLanguageCode, SPI_INDEX_LANGUAGE_MAXIMUM_LENGTH + 1)) != SRCH_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the language info from the configuration, index: '%s', srch error: %d.", 
                psiSrchIndex->pucIndexName, iError); 
        return (iError);
    }

    /* Get the tokenizer name from the information file*/
    if ( (iError = iSrchInfoGetTokenizerInfo(psiSrchIndex, pucTokenizerName, SPI_INDEX_TOKENIZER_MAXIMUM_LENGTH + 1)) != SRCH_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the tokenizer info from the configuration, index: '%s', srch error: %d.", 
                psiSrchIndex->pucIndexName, iError); 
        return (iError);
    }


    /* Get the language ID */
    if ( (iError = iLngGetLanguageIDFromCode(pucLanguageCode, &uiLanguageID)) != LNG_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the language ID from the language code: '%s', lng error: %d.", pucLanguageCode, iError); 
        return (SRCH_LanguageInvalidLanguageCode);
    }

    /* Get the tokenizer ID */
    if ( (iError = iLngGetTokenizerIDFromName(pucTokenizerName, &uiTokenizerID)) != LNG_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the tokenizer ID from the tokenizer name: '%s', lng error: %d.", pucTokenizerName, iError); 
        return (SRCH_LanguageInvalidTokenizerName);
    }


    /* Set the various fields in the index structure */
    psiSrchIndex->uiLanguageID = uiLanguageID;
    psiSrchIndex->uiTokenizerID = uiTokenizerID;


    return (SRCH_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchLanguageGetLanguageCode()

    Purpose:    Return the language code

    Parameters: psiSrchIndex            The search index structure
                pucLanguageCode         Return pointer for the language code
                uiLanguageCodeLength    Language code length 

    Globals:    none

    Returns:    SRCH error code

*/
int iSrchLanguageGetLanguageCode
(
    struct srchIndex *psiSrchIndex,
    unsigned char *pucLanguageCode,
    unsigned int uiLanguageCodeLength
)
{

    int     iError = SRCH_NoError;


    /* Check the parameters */
    if ( psiSrchIndex == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'psiSrchIndex' parameter passed to 'iSrchLanguageGetLanguageCode'."); 
        return (SRCH_InvalidIndex);
    }

    if ( pucLanguageCode == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pucLanguageCode' parameter passed to 'iSrchLanguageGetLanguageCode'."); 
        return (SRCH_ReturnParameterError);
    }

    if ( uiLanguageCodeLength <= 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiLanguageCodeLength' parameter passed to 'iSrchLanguageGetLanguageCode'."); 
        return (SRCH_ReturnParameterError);
    }


    /* Get the language code from the language ID */
    if ( (iError = iLngGetLanguageCodeFromID(psiSrchIndex->uiLanguageID, pucLanguageCode, uiLanguageCodeLength)) != LNG_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the language code from the language ID: %u, lng error: %d.", psiSrchIndex->uiLanguageID, iError); 
        return (SRCH_LanguageInvalidLanguageID);
    }


    return (SRCH_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchLanguageGetTokenizerName()

    Purpose:    Return the tokenizer name

    Parameters: psiSrchIndex            The search index structure
                pucTokenizerName        Return pointer for the tokenizer name
                uiTokenizerNameLength   Tokenizer name  length 

    Globals:    none

    Returns:    SRCH error code

*/
int iSrchLanguageGetTokenizerName
(
    struct srchIndex *psiSrchIndex,
    unsigned char *pucTokenizerName,
    unsigned int uiTokenizerNameLength
)
{

    int     iError = SRCH_NoError;


    /* Check the parameters */
    if ( psiSrchIndex == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'psiSrchIndex' parameter passed to 'iSrchLanguageGetTokenizerName'."); 
        return (SRCH_InvalidIndex);
    }

    if ( pucTokenizerName == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pucTokenizerName' parameter passed to 'iSrchLanguageGetTokenizerName'."); 
        return (SRCH_ReturnParameterError);
    }

    if ( uiTokenizerNameLength <= 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiTokenizerNameLength' parameter passed to 'iSrchLanguageGetTokenizerName'."); 
        return (SRCH_ReturnParameterError);
    }


    /* Get the tokenizer name from the tokenizer ID */
    if ( (iError = iLngGetTokenizerNameFromID(psiSrchIndex->uiTokenizerID, pucTokenizerName, uiTokenizerNameLength)) != LNG_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the tokenizer name from the tokenizer ID: %u, lng error: %d.", psiSrchIndex->uiTokenizerID, iError); 
        return (SRCH_LanguageInvalidTokenizerID);
    }


    return (SRCH_NoError);

}


/*---------------------------------------------------------------------------*/
