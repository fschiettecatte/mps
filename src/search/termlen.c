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

    Module:     termlen.c

    Author:     Francois Schiettecatte (FS Consulting LLC.)

    Created:    16 October 1999

    Purpose:    This module manages the maximum and minimum term length

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
#define UTL_LOG_CONTEXT                     (unsigned char *)"com.fsconsult.mps.src.search.termlen"


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchTermLengthInit()

    Purpose:    Initialize the term lengths

    Parameters: psiSrchIndex            The search index structure
                uiTermLengthMaximum     The maximum term length
                uiTermLengthMinimum     The minimum term length

    Globals:    none

    Returns:    SRCH error code

*/
int iSrchTermLengthInit
(
    struct srchIndex *psiSrchIndex,
    unsigned int uiTermLengthMaximum,
    unsigned int uiTermLengthMinimum
)
{

    int     iError = SRCH_NoError;


    /* Check the parameters */
    if ( psiSrchIndex == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'psiSrchIndex' parameter passed to 'iSrchTermLengthInit'."); 
        return (SRCH_InvalidIndex);
    }

    if ( (uiTermLengthMaximum < SRCH_TERM_LENGTH_MINIMUM) || (uiTermLengthMaximum > SRCH_TERM_LENGTH_MAXIMUM) ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiTermLengthMaximum' parameter passed to 'iSrchTermLengthInit'."); 
        return (SRCH_TermLenInvalidMax);
    }

    if ( uiTermLengthMinimum < SRCH_TERM_LENGTH_MINIMUM ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiTermLengthMinimum' parameter passed to 'iSrchTermLengthInit'."); 
        return (SRCH_TermLenInvalidMin);
    }



    /* Set term lengths */
    psiSrchIndex->uiTermLengthMaximum = uiTermLengthMaximum;
    psiSrchIndex->uiTermLengthMinimum = uiTermLengthMinimum;


    return (SRCH_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchTermLengthInitFromInfo()

    Purpose:    Initialize the term lengths from the information file

    Parameters: psiSrchIndex    The search index structure

    Globals:    none

    Returns:    SRCH error code

*/
int iSrchTermLengthInitFromInfo
(
    struct srchIndex *psiSrchIndex
)
{

    int             iError = SRCH_NoError;
    unsigned int    uiTermLengthMaximum = 0;
    unsigned int    uiTermLengthMinimum = 0;


    /* Check the parameters */
    if ( psiSrchIndex == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'psiSrchIndex' parameter passed to 'iSrchTermLengthInitFromInfo'."); 
        return (SRCH_InvalidIndex);
    }



    /* Get the term length information from the information file */
    if ( (iError = iSrchInfoGetTermLengthInfo(psiSrchIndex, &uiTermLengthMaximum, &uiTermLengthMinimum)) != SRCH_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the term length information from the configuration, index: '%s', srch error: %d.", 
                psiSrchIndex->pucIndexName, iError); 
        return (iError);
    }


    /* Set term lengths */
    psiSrchIndex->uiTermLengthMaximum = uiTermLengthMaximum;
    psiSrchIndex->uiTermLengthMinimum = uiTermLengthMinimum;


    return (SRCH_NoError);

}


/*---------------------------------------------------------------------------*/
