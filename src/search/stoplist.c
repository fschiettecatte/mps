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

    Module:     stoplist.c

    Author:     Francois Schiettecatte (FS Consulting LLC.)

    Created:    14 May 2004

    Purpose:    This module manages the stoplist

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
#define UTL_LOG_CONTEXT                     (unsigned char *)"com.fsconsult.mps.src.search.stoplist"


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchStopListInit()

    Purpose:    To initialize the stop list parameter from the information file if the 
                stop list name passed is NULL, otherwise we set it from the 
                passed stop list name.

    Parameters: psiSrchIndex            The search index structure
                pucStopListName         The stop list name
                pucStopListFilePath     The stop list file path

    Globals:    none

    Returns:    SRCH error code

*/
int iSrchStopListInit
(
    struct srchIndex *psiSrchIndex,
    unsigned char *pucStopListName,
    unsigned char *pucStopListFilePath
)
{

    int             iError = SRCH_NoError;
    unsigned char   pucSymbol[SRCH_INFO_SYMBOL_MAXIMUM_LENGTH + 1] = {'\0'};
    unsigned char   *pucStopListNamePtr = NULL;
    unsigned char   *pucStopListFilePathPtr = NULL;
    unsigned int    uiStopListID = LNG_STOP_LIST_ANY_ID;
    unsigned int    uiStopListTypeID = SRCH_INFO_STOP_LIST_TYPE_INVALID_ID;
    void            *pvLngStopList = NULL;


    /* Check the parameters */
    if ( psiSrchIndex == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'psiSrchIndex' parameter passed to 'iSrchStopListInit'."); 
        return (SRCH_InvalidIndex);
    }


    if ( psiSrchIndex->psibSrchIndexBuild == NULL ) {
        return (SRCH_InvalidIndex);
    }


    /* No parameters are set, so we find out if the stop list type is internal or set from a file   */
    if ( (bUtlStringsIsStringNULL(pucStopListName) == true) && (bUtlStringsIsStringNULL(pucStopListFilePath) == true) ) { 

        /* Read the stop list information  */
        if ( (iError = iSrchInfoGetStopListInfo(psiSrchIndex, pucSymbol, SRCH_INFO_SYMBOL_MAXIMUM_LENGTH + 1, &uiStopListTypeID)) != SRCH_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the stop list information from the configuration, index: '%s', srch error: %d.", 
                    psiSrchIndex->pucIndexName, iError); 
            return (iError);
        }

        /* Internal stop list */ 
        if ( uiStopListTypeID == SRCH_INFO_STOP_LIST_TYPE_INTERNAL_ID ) {
            /* Set the stop list name pointer from the symbol, this is also a flag */
            pucStopListNamePtr = pucSymbol;
        }
        /* File stop list */
        else if ( uiStopListTypeID == SRCH_INFO_STOP_LIST_TYPE_FILE_ID ) {
            /* Set the stop list name pointer from the symbol, this is also a flag */
            pucStopListFilePathPtr = pucSymbol;
        }
    }

    /* The stop list to use in internal */
    else if ( bUtlStringsIsStringNULL(pucStopListName) == false ) {
        /* Set the stop list name pointer from the stop list name, this is also a flag */
        pucStopListNamePtr = pucStopListName;
    }
    
    /* The stop list to use is a file */ 
    else if ( bUtlStringsIsStringNULL(pucStopListFilePath) == false ) {
        /* Set the stop list file path pointer from the stop list file path, this is also a flag */
        pucStopListFilePathPtr = pucStopListFilePath;
    }


    /* We are using an internal stop list if the stop list name pointer is set */
    if ( pucStopListNamePtr != NULL ) {

        /* Get the stop list ID */
        if ( (iError = iLngGetStopListIDFromName(pucStopListNamePtr, &uiStopListID)) != LNG_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the stop list ID from the stop list name: '%s', lng error: %d.", pucStopListNamePtr, iError); 
            return (SRCH_StopListInvalidStopListName);
        }
    
        /* Create the stop list object */
        if ( (iError = iLngStopListCreateByID(uiStopListID, psiSrchIndex->uiLanguageID, &pvLngStopList)) != LNG_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to create the stop list, lng error: %d.", iError);
            return (SRCH_StopListCreateFailed);
        }

        psiSrchIndex->psibSrchIndexBuild->uiStopListTypeID = SRCH_INFO_STOP_LIST_TYPE_INTERNAL_ID;
    }
    /* We are using a file */
    else if ( pucStopListFilePathPtr != NULL ) {

        /* Get the stop list handle */
        if ( (iError = iLngStopListCreateByIDFromFile(pucStopListFilePathPtr, psiSrchIndex->uiLanguageID, &pvLngStopList)) != LNG_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to create the stop list from the stop list file: '%s', lng error: %d.", pucStopListFilePathPtr, iError); 
            return (SRCH_StopListCreateFailed);
        }

        /* Make a copy of the stop list file path */
        if ( (psiSrchIndex->psibSrchIndexBuild->pucStopListFilePath = s_strdup(pucStopListFilePath)) == NULL ) {
            return (SRCH_MemError);
        }

        psiSrchIndex->psibSrchIndexBuild->uiStopListTypeID = SRCH_INFO_STOP_LIST_TYPE_FILE_ID;
    }
    else {
        return (SRCH_StopListInitFailed);
    }


    /* Set the various fields in the index structure */
    psiSrchIndex->psibSrchIndexBuild->pvLngStopList = pvLngStopList;
    psiSrchIndex->psibSrchIndexBuild->uiStopListID = uiStopListID;


    return (SRCH_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   bSrchStopListOn() << refactor out

    Purpose:    Return true if we are using a stemmer for this index,
                false otherwise

    Parameters: psiSrchIndex    The search index structure

    Globals:    none

    Returns:    true if we are using a stemmer on this index, false
                otherwise

*/
boolean bSrchStopListOn
(
    struct srchIndex *psiSrchIndex
)
{

    int             iError = SRCH_NoError;
    unsigned int    uiStopListTypeID = SRCH_INFO_STOP_LIST_TYPE_INVALID_ID;
    unsigned int    uiStopListID = 0;
    unsigned char   pucStopListName[SPI_INDEX_STOP_LIST_MAXIMUM_LENGTH + 1] = {'\0'};


    /* Check the parameters */
    if ( psiSrchIndex == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'psiSrchIndex' parameter passed to 'bSrchStopListOn'."); 
        return (false);
    }


    /* Pull the information out the index build structure if it is there, otherwise we get it from the info */
    if ( psiSrchIndex->psibSrchIndexBuild != NULL ) {
        
        /* We are using a stop word list if it is a file */
        if ( psiSrchIndex->psibSrchIndexBuild->uiStopListTypeID == SRCH_INFO_STOP_LIST_TYPE_FILE_ID ) {
            return (true);
        }
    
        /* Anything other than LNG_STOP_LIST_NONE_ID means we have a stop list defined */
        return ( (psiSrchIndex->psibSrchIndexBuild->uiStopListID != LNG_STOP_LIST_NONE_ID) ? true : false );
    }
    else {

        /* Get the stop list information */
        if ( (iError = iSrchInfoGetStopListInfo(psiSrchIndex, pucStopListName, SPI_INDEX_STOP_LIST_MAXIMUM_LENGTH + 1, &uiStopListTypeID)) != SRCH_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the stop list name symbol from the configuration, index: '%s', srch error: %d.", 
                    psiSrchIndex->pucIndexName, iError); 
            return (false);
        }
    
        /* We are using a stop word list if it is a file */
        if ( uiStopListTypeID == SRCH_INFO_STOP_LIST_TYPE_FILE_ID ) {
            return (true);
        }
    
        /* Get the stop list ID */
        if ( (iError = iLngGetStopListIDFromName(pucStopListName, &uiStopListID)) != LNG_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the stop list ID from the stop list name: '%s', lng error: %d.", pucStopListName, iError); 
            return (false);
        }
    
        /* Anything other than LNG_STOP_LIST_NONE_ID means we have a stop list defined */
        return ( (uiStopListID != LNG_STOP_LIST_NONE_ID) ? true : false );
    }

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchStopListGetName()

    Purpose:    Return the stop list name

    Parameters: psiSrchIndex            The search index structure
                pucStopListName         Return pointer for the stop list name
                uiStopListNameLength    StopList name length 

    Globals:    none

    Returns:    SRCH error code

*/
int iSrchStopListGetName
(
    struct srchIndex *psiSrchIndex,
    unsigned char *pucStopListName,
    unsigned int uiStopListNameLength
)
{

    int     iError = SRCH_NoError;


    /* Check the parameters */
    if ( psiSrchIndex == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'psiSrchIndex' parameter passed to 'iSrchStopListGetName'."); 
        return (SRCH_InvalidIndex);
    }

    if ( pucStopListName == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pucStopListName' parameter passed to 'iSrchStopListGetName'."); 
        return (SRCH_ReturnParameterError);
    }

    if ( uiStopListNameLength <= 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiStopListNameLength' parameter passed to 'iSrchStopListGetName'."); 
        return (SRCH_ReturnParameterError);
    }


    /* Get the stop list information */
    if ( (iError = iSrchInfoGetStopListInfo(psiSrchIndex, pucStopListName, uiStopListNameLength, NULL)) != SRCH_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the stop list name symbol from the configuration, index: '%s', srch error: %d.", 
                psiSrchIndex->pucIndexName, iError); 
        return (iError);
    }


    return (SRCH_NoError);

}

/*---------------------------------------------------------------------------*/




