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

    Module:     document.h

    Author:     Francois Schiettecatte (FS Consulting LLC.)

    Created:    14 September 1995

    Purpose:    This is the header file for document.c.

*/


/*---------------------------------------------------------------------------*/


#if !defined(SRCH_DOCUMENT_H)
#define SRCH_DOCUMENT_H


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
** Defines
*/

/* Size defines - these are used by document.c only, except for SRCH_DOCUMENT_ENTRY_LENGTH
** which is needed by index.c to create the document table
*/ 
#define SRCH_DOCUMENT_DATA_ID_SIZE              (8)
#define SRCH_DOCUMENT_RANK_SIZE                 (4)
#define SRCH_DOCUMENT_TERM_COUNT_SIZE           (4)
#define SRCH_DOCUMENT_DATE_SIZE                 (6)
#define SRCH_DOCUMENT_LANGUAGE_ID_SIZE          (2)

#define SRCH_DOCUMENT_ENTRY_LENGTH              (SRCH_DOCUMENT_DATA_ID_SIZE + \
                                                        SRCH_DOCUMENT_RANK_SIZE + \
                                                        SRCH_DOCUMENT_TERM_COUNT_SIZE + \
                                                        SRCH_DOCUMENT_DATE_SIZE + \
                                                        SRCH_DOCUMENT_LANGUAGE_ID_SIZE)


/*---------------------------------------------------------------------------*/


/*
** Structures
*/

/* Document item structure */
struct srchDocumentItem {
    unsigned int    uiItemID;                                   /* Item ID */
    unsigned int    uiItemLength;                               /* Item length */
    unsigned char   pucUrl[SPI_URL_MAXIMUM_LENGTH + 1];         /* URL (optional) */
    unsigned char   pucFilePath[UTL_FILE_PATH_MAX + 1];         /* File path (optional) */
    off_t           zStartOffset;                               /* Start offset into the filename (optional) */
    off_t           zEndOffset;                                 /* End offset into the filename (optional) */
    void            *pvData;                                    /* Data pointer for this item (optional) */
    unsigned int    uiDataLength;                               /* Length of the above data pointer (optional) */
    int             iDataError;                                 /* SRCH error code (optional) */
};


/*---------------------------------------------------------------------------*/


/*
** Public function prototypes
*/

int iSrchDocumentGetDocumentInfo (struct srchIndex *psiSrchIndex, unsigned int uiDocumentID, 
        unsigned char **ppucTitle, unsigned char **ppucDocumentKey, unsigned int *puiRank, 
        unsigned int *puiTermCount, unsigned long *pulAnsiDate, unsigned int *puiLanguageID,
        struct srchDocumentItem **ppsdiSrchDocumentItems, unsigned int *puiSrchDocumentItemsLength, 
        unsigned int uiItemID, boolean bLoadTypeFilePaths, boolean bLoadTypeURLs, boolean bLoadStoredData);

int iSrchDocumentSaveDocumentInfo (struct srchIndex *psiSrchIndex, unsigned int uiDocumentID, 
        unsigned char *pucTitle, unsigned char *pucDocumentKey, unsigned int uiRank, 
        unsigned int uiTermCount, unsigned long ulAnsiDate, unsigned int uiLanguageID, 
        struct srchDocumentItem *psdiSrchDocumentItems, unsigned int uiSrchDocumentItemsLength);


int iSrchDocumentGetNewDocumentID (struct srchIndex *psiSrchIndex, unsigned int *puiDocumentID);

int iSrchDocumentValidateDocumentID (struct srchIndex *psiSrchIndex, unsigned int uiDocumentID);


/*---------------------------------------------------------------------------*/


/* 
** C++ wrapper
*/

#if defined(__cplusplus)
}
#endif    /* defined(__cplusplus) */


/*---------------------------------------------------------------------------*/


#endif    /* !defined(SRCH_DOCUMENT_H) */


/*---------------------------------------------------------------------------*/
