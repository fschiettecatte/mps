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

    Module:     filepaths.h

    Author:     Francois Schiettecatte (FS Consulting LLC.)

    Created:    15 September 1994

    Purpose:    This is the header file for filepaths.c.

*/


/*---------------------------------------------------------------------------*/


#if !defined(SRCH_FILEPATHS_H)
#define SRCH_FILEPATHS_H


/*---------------------------------------------------------------------------*/


/*
** Includes
*/

#include "srch.h"


/*---------------------------------------------------------------------------*/


/* 
** C++ wrapper
*/

#if defined(__cplusplus)
extern "C" {
#endif    /* defined(__cplusplus) */


/*---------------------------------------------------------------------------*/

/*
** Public function prototypes
*/

int iSrchFilePathsGetTempTermDictionaryFilePathFromIndex (struct srchIndex *psiSrchIndex,
        unsigned int uiVersion, boolean bShadowFile, unsigned char *pucFilePath,
        unsigned int uiFilePathLength);

int iSrchFilePathsGetTempKeyDictionaryFilePathFromIndex (struct srchIndex *psiSrchIndex,
        unsigned int uiVersion, boolean bShadowFile, unsigned char *pucFilePath,
        unsigned int uiFilePathLength);



int iSrchFilePathsGetTermDictionaryFilePathFromIndex (struct srchIndex *psiSrchIndex,
        unsigned char *pucFilePath, unsigned int uiFilePathLength);

int iSrchFilePathsGetKeyDictionaryFilePathFromIndex (struct srchIndex *psiSrchIndex,
        unsigned char *pucFilePath, unsigned int uiFilePathLength);

int iSrchFilePathsGetDocumentTableFilePathFromIndex (struct srchIndex *psiSrchIndex,
        unsigned char *pucFilePath, unsigned int uiFilePathLength);

int iSrchFilePathsGetDocumentDataFilePathFromIndex (struct srchIndex *psiSrchIndex,
        unsigned char *pucFilePath, unsigned int uiFilePathLength);

int iSrchFilePathsGetIndexDataFilePathFromIndex (struct srchIndex *psiSrchIndex,
        unsigned char *pucFilePath, unsigned int uiFilePathLength);

int iSrchFilePathsGetIndexLockFilePathFromIndex (struct srchIndex *psiSrchIndex,
        unsigned char *pucFilePath, unsigned int uiFilePathLength);

int iSrchFilePathsGetIndexInformationFilePathFromIndex (struct srchIndex *psiSrchIndex,
        unsigned char *pucFilePath, unsigned int uiFilePathLength);



int iSrchFilePathsGetTermDictionaryFilePathFromIndexPath (unsigned char *pucIndexPath,
        unsigned char *pucFilePath, unsigned int uiFilePathLength);

int iSrchFilePathsGetKeyDictionaryFilePathFromIndexPath (unsigned char *pucIndexPath,
        unsigned char *pucFilePath, unsigned int uiFilePathLength);

int iSrchFilePathsGetDocumentTableFilePathFromIndexPath (unsigned char *pucIndexPath,
        unsigned char *pucFilePath, unsigned int uiFilePathLength);

int iSrchFilePathsGetDocumentDataFilePathFromIndexPath (unsigned char *pucIndexPath,
        unsigned char *pucFilePath, unsigned int uiFilePathLength);

int iSrchFilePathsGetIndexDataFilePathFromIndexPath (unsigned char *pucIndexPath,
        unsigned char *pucFilePath, unsigned int uiFilePathLength);

int iSrchFilePathsGetIndexLockFilePathFromIndexPath (unsigned char *pucIndexPath,
        unsigned char *pucFilePath, unsigned int uiFilePathLength);

int iSrchFilePathsGetIndexInformationFilePathFromIndexPath (unsigned char *pucIndexPath,
        unsigned char *pucFilePath, unsigned int uiFilePathLength);


/*---------------------------------------------------------------------------*/


/* 
** C++ wrapper
*/

#if defined(__cplusplus)
}
#endif    /* defined(__cplusplus) */


/*---------------------------------------------------------------------------*/


#endif    /* !defined(SRCH_FILEPATHS_H) */


/*---------------------------------------------------------------------------*/
