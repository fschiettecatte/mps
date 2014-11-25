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

    Module:     functions.h

    Author:     Francois Schiettecatte (FS Consulting LLC.)

    Created:    24 February 1994

    Purpose:    This is the header file for functions.c. This file contains a
                the various functions for indexing different types of documents.

*/


/*---------------------------------------------------------------------------*/


#if !defined(PRS_FUNCTIONS_H)
#define PRS_FUNCTIONS_H


/*---------------------------------------------------------------------------*/


/*
** Includes
*/

#include "utils.h"


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


/* Text format - One document per file, first line is title, all subsequent lines are text */
boolean bPrsTextFieldInformationFunction (unsigned int uiFieldID, wchar_t **ppwcFieldName, 
        unsigned char **ppucFieldType, unsigned char **ppucFieldOptions, wchar_t **ppwcFieldDescription, 
        boolean *pbUnfieldedSearch);
void vPrsTextDocumentLineFunction (wchar_t *pwcLine, unsigned int *puiLanguageID, 
        unsigned int *puiFieldID, boolean *pbIndexLine, boolean *pbParseLine, boolean *pbTermPositions);
void vPrsTextDocumentInformationFunction (unsigned char *pucFilePath, wchar_t *pwcDocumentTitle, 
        wchar_t *pwcDocumentKey, wchar_t *pwcDocumentUrl, unsigned int *puiDocumentRank, 
        unsigned long *pulDocumentAnsiDate);


/* Paragraph format - One document per paragraph, first line is title, all subsequent lines are text */
boolean bPrsParaFieldInformationFunction (unsigned int uiFieldID, wchar_t **ppwcFieldName, 
        unsigned char **ppucFieldType, unsigned char **ppucFieldOptions, wchar_t **ppwcFieldDescription, 
        boolean *pbUnfieldedSearch);
boolean bPrsParaDocumentEndFunction (wchar_t *pwcLine);
void vPrsParaDocumentLineFunction (wchar_t *pwcLine, unsigned int *puiLanguageID, 
        unsigned int *puiFieldID, boolean *pbIndexLine, boolean *pbParseLine, boolean *pbTermPositions);
void vPrsParaDocumentInformationFunction (unsigned char *pucFilePath, wchar_t *pwcDocumentTitle, 
        wchar_t *pwcDocumentKey, wchar_t *pwcDocumentUrl, unsigned int *puiDocumentRank, 
        unsigned long *pulDocumentAnsiDate);


/* Line format - One document per file, first line is title, all subsequent lines are terms */
boolean bPrsLineFieldInformationFunction (unsigned int uiFieldID, wchar_t **ppwcFieldName, 
        unsigned char **ppucFieldType, unsigned char **ppucFieldOptions, wchar_t **ppwcFieldDescription, 
        boolean *pbUnfieldedSearch);
void vPrsLineDocumentLineFunction (wchar_t *pwcLine, unsigned int *puiLanguageID, 
        unsigned int *puiFieldID, boolean *pbIndexLine, boolean *pbParseLine, boolean *pbTermPositions);
void vPrsLineDocumentInformationFunction (unsigned char *pucFilePath, wchar_t *pwcDocumentTitle, 
        wchar_t *pwcDocumentKey, wchar_t *pwcDocumentUrl, unsigned int *puiDocumentRank, 
        unsigned long *pulDocumentAnsiDate);


/* Mbox format - Mbox/mail/mail digest format */
boolean bPrsMboxDocumentStartFunction (wchar_t *pwcLine);
boolean bPrsMboxFieldInformationFunction (unsigned int uiFieldID, wchar_t **ppwcFieldName, 
        unsigned char **ppucFieldType, unsigned char **ppucFieldOptions, wchar_t **ppwcFieldDescription, 
        boolean *pbUnfieldedSearch);
void vPrsMboxDocumentLineFunction (wchar_t *pwcLine, unsigned int *puiLanguageID, 
        unsigned int *puiFieldID, boolean *pbIndexLine, boolean *pbParseLine, boolean *pbTermPositions);
void vPrsMboxDocumentInformationFunction (unsigned char *pucFilePath, wchar_t *pwcDocumentTitle, 
        wchar_t *pwcDocumentKey, wchar_t *pwcDocumentUrl, unsigned int *puiDocumentRank, 
        unsigned long *pulDocumentAnsiDate);


/* Refer format */
boolean bPrsReferDocumentEndFunction (wchar_t *pwcLine);
boolean bPrsReferFieldInformationFunction (unsigned int uiFieldID, wchar_t **ppwcFieldName, 
        unsigned char **ppucFieldType, unsigned char **ppucFieldOptions, wchar_t **ppwcFieldDescription, 
        boolean *pbUnfieldedSearch);
void vPrsReferDocumentLineFunction (wchar_t *pwcLine, unsigned int *puiLanguageID, 
        unsigned int *puiFieldID, boolean *pbIndexLine, boolean *pbParseLine, boolean *pbTermPositions);
void vPrsReferDocumentInformationFunction (unsigned char *pucFilePath, wchar_t *pwcDocumentTitle, 
        wchar_t *pwcDocumentKey, wchar_t *pwcDocumentUrl, unsigned int *puiDocumentRank, 
        unsigned long *pulDocumentAnsiDate);


/* MPS text/xml format */
boolean bPrsMpsFieldInformationFunction (unsigned int uiFieldID, wchar_t **ppwcFieldName, 
        unsigned char **ppucFieldType, unsigned char **ppucFieldOptions, wchar_t **ppwcFieldDescription, 
        boolean *pbUnfieldedSearch);
void vPrsMpsDocumentInformationFunction (unsigned char *pucFilePath, wchar_t *pwcDocumentTitle, 
        wchar_t *pwcDocumentKey, wchar_t *pwcDocumentUrl, unsigned int *puiDocumentRank, 
        unsigned long *pulDocumentAnsiDate);

/* MPS text format */
boolean bPrsMpsTextDocumentEndFunction (wchar_t *pwcLine);
void vPrsMpsTextDocumentLineFunction (wchar_t *pwcLine, unsigned int *puiLanguageID, 
        unsigned int *puiFieldID, boolean *pbIndexLine, boolean *pbParseLine, boolean *pbTermPositions);

/* MPS xml format */
boolean bPrsMpsXmlDocumentStartFunction (wchar_t *pwcLine);
boolean bPrsMpsXmlDocumentEndFunction (wchar_t *pwcLine);
void vPrsMpsXmlDocumentLineFunction (wchar_t *pwcLine, unsigned int *puiLanguageID, 
        unsigned int *puiFieldID, boolean *pbIndexLine, boolean *pbParseLine, boolean *pbTermPositions);


/* Poplar format */
boolean bPrsPoplarDocumentEndFunction (wchar_t *pwcLine);
boolean bPrsPoplarFieldInformationFunction (unsigned int uiFieldID, wchar_t **ppwcFieldName, 
        unsigned char **ppucFieldType, unsigned char **ppucFieldOptions, wchar_t **ppwcFieldDescription, 
        boolean *pbUnfieldedSearch);
void vPrsPoplarDocumentLineFunction (wchar_t *pwcLine, unsigned int *puiLanguageID, 
        unsigned int *puiFieldID, boolean *pbIndexLine, boolean *pbParseLine, boolean *pbTermPositions);
void vPrsPoplarDocumentInformationFunction (unsigned char *pucFilePath, wchar_t *pwcDocumentTitle, 
        wchar_t *pwcDocumentKey, wchar_t *pwcDocumentUrl, unsigned int *puiDocumentRank, 
        unsigned long *pulDocumentAnsiDate);


/* Medline format */
boolean bPrsMedlineDocumentEndFunction (wchar_t *pwcLine);
boolean bPrsMedlineFieldInformationFunction (unsigned int uiFieldID, wchar_t **ppwcFieldName, 
        unsigned char **ppucFieldType, unsigned char **ppucFieldOptions, wchar_t **ppwcFieldDescription, 
        boolean *pbUnfieldedSearch);
void vPrsMedlineDocumentLineFunction (wchar_t *pwcLine, unsigned int *puiLanguageID, 
        unsigned int *puiFieldID, boolean *pbIndexLine, boolean *pbParseLine, boolean *pbTermPositions);
void vPrsMedlineDocumentInformationFunction (unsigned char *pucFilePath, wchar_t *pwcDocumentTitle, 
        wchar_t *pwcDocumentKey, wchar_t *pwcDocumentUrl, unsigned int *puiDocumentRank, 
        unsigned long *pulDocumentAnsiDate);


/* OMIM format */
boolean bPrsOmimDocumentStartFunction (wchar_t *pwcLine);
boolean bPrsOmimFieldInformationFunction (unsigned int uiFieldID, wchar_t **ppwcFieldName, 
        unsigned char **ppucFieldType, unsigned char **ppucFieldOptions, wchar_t **ppwcFieldDescription, 
        boolean *pbUnfieldedSearch);
void vPrsOmimDocumentLineFunction (wchar_t *pwcLine, unsigned int *puiLanguageID, 
        unsigned int *puiFieldID, boolean *pbIndexLine, boolean *pbParseLine, boolean *pbTermPositions);
void vPrsOmimDocumentInformationFunction (unsigned char *pucFilePath, wchar_t *pwcDocumentTitle, 
        wchar_t *pwcDocumentKey, wchar_t *pwcDocumentUrl, unsigned int *puiDocumentRank, 
        unsigned long *pulDocumentAnsiDate);


/* TREC format */
boolean bPrsTrecDocumentStartFunction (wchar_t *pwcLine);
boolean bPrsTrecDocumentEndFunction (wchar_t *pwcLine);
void vPrsTrecDocumentLineFunction (wchar_t *pwcLine, unsigned int *puiLanguageID, 
        unsigned int *puiFieldID, boolean *pbIndexLine, boolean *pbParseLine, boolean *pbTermPositions);
void vPrsTrecDocumentInformationFunction (unsigned char *pucFilePath, wchar_t *pwcDocumentTitle, 
        wchar_t *pwcDocumentKey, wchar_t *pwcDocumentUrl, unsigned int *puiDocumentRank, 
        unsigned long *pulDocumentAnsiDate);


/*---------------------------------------------------------------------------*/


/* 
** C++ wrapper
*/

#if defined(__cplusplus)
}
#endif    /* defined(__cplusplus) */


/*---------------------------------------------------------------------------*/


#endif    /* !defined(PRS_FUNCTIONS_H) */


/*---------------------------------------------------------------------------*/
