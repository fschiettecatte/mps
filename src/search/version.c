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

    Module:     version.c

    Author:     Francois Schiettecatte (FS Consulting LLC.)

    Created:    24 February 1994

    Purpose:    This is module implements the various version functions.
                What this really does is to enable us to store and 
                search engine version/size in the information file and
                check them before we start searching so that we dont
                version conflicts.

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
#define UTL_LOG_CONTEXT                     (unsigned char *)"com.fsconsult.mps.src.search.version"


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchVersionInit()

    Purpose:    Initialize the index version.

    Parameters: psiSrchIndex        Search index structure
                uiMajorVersion      major version
                uiMinorVersion      minor version
                uiPatchVersion      patch version

    Globals:    none

    Returns:    SRCH error code

*/
int iSrchVersionInit
(
    struct srchIndex *psiSrchIndex,
    unsigned int uiMajorVersion,
    unsigned int uiMinorVersion, 
    unsigned int uiPatchVersion
)
{

    /* Check the parameters */
    if ( psiSrchIndex == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'psiSrchIndex' parameter passed to 'iSrchVersionInit'."); 
        return (SRCH_InvalidIndex);
    }


    
    /* Set the index fields */            
    psiSrchIndex->uiMajorVersion = uiMajorVersion;
    psiSrchIndex->uiMinorVersion = uiMinorVersion;
    psiSrchIndex->uiPatchVersion = uiPatchVersion;
        

    return (SRCH_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchVersionInitFromInfo()

    Purpose:    Initialize the index version from the information file

    Parameters: psiSrchIndex        Search index structure

    Globals:    none

    Returns:    SRCH error code

*/
int iSrchVersionInitFromInfo
(
    struct srchIndex *psiSrchIndex
)
{

    int             iError = SRCH_NoError;
    unsigned int    uiMajorVersion = 0;
    unsigned int    uiMinorVersion = 0;
    unsigned int    uiPatchVersion = 0;


    /* Check the parameters */
    if ( psiSrchIndex == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'psiSrchIndex' parameter passed to 'iSrchVersionInitFromInfo'."); 
        return (SRCH_InvalidIndex);
    }



    /* Get the version information from the information file */
    if ( (iError = iSrchInfoGetVersionInfo(psiSrchIndex, &uiMajorVersion, &uiMinorVersion, &uiPatchVersion)) != SRCH_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the version from the information file, index: '%s', srch error: %d.", 
                psiSrchIndex->pucIndexName, iError); 
        return (SRCH_InvalidIndex);
    }


    /* Check the version numbers we got against the internal version, note that we don't check the patch version */
    if ( (uiMajorVersion != UTL_VERSION_MAJOR) || (uiMinorVersion != UTL_VERSION_MINOR) ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid index version in information file for the index: '%s', got: %u.%u.%u, expected: %u.%u.%u.", 
                psiSrchIndex->pucIndexName, uiMajorVersion, uiMinorVersion, uiPatchVersion, UTL_VERSION_MAJOR, UTL_VERSION_MINOR, UTL_VERSION_PATCH); 
        return (SRCH_InvalidIndex);
    }

        
    /* Set the index fields */            
    psiSrchIndex->uiMajorVersion = uiMajorVersion;
    psiSrchIndex->uiMinorVersion = uiMinorVersion;
    psiSrchIndex->uiPatchVersion = uiPatchVersion;
        

    return (SRCH_NoError);

}


/*---------------------------------------------------------------------------*/
