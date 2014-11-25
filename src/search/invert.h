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

    Module:     invert.h

    Author:     Francois Schiettecatte (FS Consulting LLC.)

    Created:    24 February 1994

    Purpose:    This is the header file for invert.c.

*/


/*---------------------------------------------------------------------------*/


#if !defined(SRCH_INVERT_H)
#define SRCH_INVERT_H


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

int iSrchInvertInit (struct srchIndex *psiSrchIndex, unsigned char *pucLanguageCode, unsigned char *pucTokenizerName, 
        unsigned char *pucStemmerName, unsigned char *pucStopListName, unsigned char *pucStopListFilePath, 
        unsigned int uiIndexerMemorySizeMaximum, unsigned int uiTermLengthMinimum,  unsigned int uiTermLengthMaximum, 
        unsigned char *pucTemporaryDirectoryPath);

int iSrchInvertSwitchLanguage (struct srchIndex *psiSrchIndex, unsigned char *pucLanguageCode);

int iSrchInvertAddTerm (struct srchIndex *psiSrchIndex, unsigned int uiDocumentID, 
        unsigned char *pucTerm, unsigned int uiTermPosition, unsigned int uiFieldID, 
        unsigned int uiFieldType, unsigned int uiFieldOptions);

int iSrchInvertFinish (struct srchIndex *psiSrchIndex);

int iSrchInvertAbort (struct srchIndex *psiSrchIndex);


/*---------------------------------------------------------------------------*/


/* 
** C++ wrapper
*/

#if defined(__cplusplus)
}
#endif    /* defined(__cplusplus) */


/*---------------------------------------------------------------------------*/


#endif    /* !defined(SRCH_INVERT_H) */


/*---------------------------------------------------------------------------*/
