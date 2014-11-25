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

    Module:     stemmer.c

    Author:     Francois Schiettecatte (FS Consulting LLC.)

    Created:    22 June 1998

    Purpose:    This module manages the stemmer

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
#define UTL_LOG_CONTEXT                     (unsigned char *)"com.fsconsult.mps.src.search.stemmer"


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchStemmerInit()

    Purpose:    Initialize the stemmer

    Parameters: psiSrchIndex        The search index structure
                pucStemmerName      The stemmer name

    Globals:    none

    Returns:    SRCH error code

*/
int iSrchStemmerInit
(
    struct srchIndex *psiSrchIndex,
    unsigned char *pucStemmerName
)
{

    int             iError = SRCH_NoError;
    unsigned int    uiStemmerID = 0;


    /* Check the parameters */
    if ( psiSrchIndex == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'psiSrchIndex' parameter passed to 'iSrchStemmerInit'."); 
        return (SRCH_InvalidIndex);
    }

    if ( bUtlStringsIsStringNULL(pucStemmerName) == true ) { 
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pucStemmerName' parameter passed to 'iSrchStemmerInit'."); 
        return (SRCH_StemmerInvalidStemmerName);
    }



    /* Get the stemmer ID from the stemmer name */
    if ( (iError = iLngGetStemmerIDFromName(pucStemmerName, &uiStemmerID)) != LNG_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the stemmer ID from the stemmer name: '%s', lng error: %d.", pucStemmerName, iError); 
        return (SRCH_StemmerInvalidStemmerName);
    }


    /* Set the various fields in the index structure */
    psiSrchIndex->uiStemmerID = uiStemmerID;


    return (SRCH_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchStemmerInitFromInfo()

    Purpose:    Initialize the stemmer from the information file

    Parameters: psiSrchIndex    The search index structure

    Globals:    none

    Returns:    SRCH error code

*/
int iSrchStemmerInitFromInfo
(
    struct srchIndex *psiSrchIndex
)
{

    int             iError = SRCH_NoError;
    unsigned char   pucStemmerName[SPI_INDEX_STEMMER_MAXIMUM_LENGTH + 1] = {'\0'};
    unsigned int    uiStemmerID = 0;


    /* Check the parameters */
    if ( psiSrchIndex == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'psiSrchIndex' parameter passed to 'iSrchStemmerInitFromInfo'."); 
        return (SRCH_InvalidIndex);
    }



    /* Get the stemmer info from the information file */
    if ( (iError = iSrchInfoGetStemmerInfo(psiSrchIndex, pucStemmerName, SPI_INDEX_STEMMER_MAXIMUM_LENGTH + 1)) != SRCH_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the stemmer info from the configuration, index: '%s', srch error: %d.", 
                psiSrchIndex->pucIndexName, iError); 
        return (iError);
    }


    /* Get the stemmer ID from the stemmer name */
    if ( (iError = iLngGetStemmerIDFromName(pucStemmerName, &uiStemmerID)) != LNG_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the stemmer ID from the stemmer name: '%s', lng error: %d.", pucStemmerName, iError); 
        return (SRCH_StemmerInvalidStemmerName);
    }


    /* Set the various fields in the index structure */
    psiSrchIndex->uiStemmerID = uiStemmerID;


    return (SRCH_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   bSrchStemmerOn()

    Purpose:    Return true if we are using a stemmer for this index,
                false otherwise

    Parameters: psiSrchIndex    The search index structure

    Globals:    none

    Returns:    true if we are using a stemmer on this index, false
                otherwise

*/
boolean bSrchStemmerOn
(
    struct srchIndex *psiSrchIndex
)
{

    /* Check the parameters */
    if ( psiSrchIndex == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'psiSrchIndex' parameter passed to 'bSrchStemmerOn'."); 
        return (false);
    }


    /* Anything other than LNG_STEMMER_NONE_ID means we have a stemmer defined */
    return ( (psiSrchIndex->uiStemmerID != LNG_STEMMER_NONE_ID) ? true : false );

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchStemmerGetName()

    Purpose:    Return the stemmer name

    Parameters: psiSrchIndex            The search index structure
                pucStemmerName          Return pointer for the stemmer name
                uiStemmerNameLength     Stemmer name length 

    Globals:    none

    Returns:    SRCH error code

*/
int iSrchStemmerGetName
(
    struct srchIndex *psiSrchIndex,
    unsigned char *pucStemmerName,
    unsigned int uiStemmerNameLength
)
{

    int     iError = SRCH_NoError;


    /* Check the parameters */
    if ( psiSrchIndex == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'psiSrchIndex' parameter passed to 'iSrchStemmerGetName'."); 
        return (SRCH_InvalidIndex);
    }

    if ( pucStemmerName == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pucStemmerNameSymbolString' parameter passed to 'iSrchStemmerGetName'."); 
        return (SRCH_ReturnParameterError);
    }

    if ( uiStemmerNameLength <= 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'uiStemmerNameLength' parameter passed to 'iSrchStemmerGetName'."); 
        return (SRCH_ReturnParameterError);
    }


    /* Get the stemmer name from the stemmer ID */
    if ( (iError = iLngGetStemmerNameFromID(psiSrchIndex->uiStemmerID, pucStemmerName, uiStemmerNameLength)) != LNG_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the stemmer name from the stemmer ID: %u, lng error: %d.", psiSrchIndex->uiStemmerID, iError); 
        return (SRCH_StemmerInvalidStemmerID);
    }


    return (SRCH_NoError);

}


/*---------------------------------------------------------------------------*/




