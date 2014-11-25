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

    Module:     retrieval.h

    Author:     Francois Schiettecatte (FS Consulting LLC.)

    Created:    24 February 1994

    Purpose:    This is the header file for retrieval.c.

*/


/*---------------------------------------------------------------------------*/


#if !defined(SRCH_RETRIEVAL_H)
#define SRCH_RETRIEVAL_H


/*---------------------------------------------------------------------------*/


/*
** Includes
*/

#include "srch.h"


/*---------------------------------------------------------------------------*/


/* C++ wrapper */
#if defined(__cplusplus)
extern "C" {
#endif    /* defined(__cplusplus) */


/*---------------------------------------------------------------------------*/


/*
** Public function prototypes
*/

int iSrchRetrievalRetrieveDocument (struct srchSearch *pssSrchSearch, struct srchIndex *psiSrchIndex, 
        unsigned char *pucDocumentKey, unsigned char *pucItemName, unsigned char *pucMimeType, 
        unsigned int uiChunkType, unsigned int uiChunkStart, unsigned int uiChunkEnd, 
        void **ppvData, unsigned int *puiDataLength);


/*---------------------------------------------------------------------------*/


/* 
** C++ wrapper
*/

#if defined(__cplusplus)
}
#endif    /* defined(__cplusplus) */


/*---------------------------------------------------------------------------*/


#endif    /* !defined(SRCH_RETRIEVAL_H) */


/*---------------------------------------------------------------------------*/
