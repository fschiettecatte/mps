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

    Module:     parser.h

    Author:     Francois Schiettecatte (FS Consulting LLC.)

    Created:    2 October 1994

    Purpose:    This is the header file for parser.c.

*/


/*---------------------------------------------------------------------------*/


#if !defined(PRS_PARSER_H)
#define PRS_PARSER_H


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
** Structures
*/


/* Parser format structure */
struct prsFormat {

    unsigned char           *pucType;                                   /* Document type */
    unsigned char           *pucDescription;                            /* Document description */

    unsigned char           *pucItemName;                               /* Item name */
    unsigned char           *pucMimeType;                               /* Mime type */

    boolean                 (*bPrsDocumentStartFunction)();             /* Document start function function */
    boolean                 (*bPrsDocumentEndFunction)();               /* Document end function function */
    boolean                 (*bPrsFieldInformationFunction)();          /* Field information function */
    void                    (*vPrsDocumentLineFunction)();              /* Document line function */
    void                    (*vPrsDocumentInformationFunction)();       /* Document information function */

};


/* Selector structure */
struct prsSelector {
    
    boolean                 bTraverseDirectories;                       /* Traverse directories */

    unsigned char           **ppucFileNameIncludeList;                  /* List of file name extensions to include */
    unsigned char           **ppucFileNameExcludeList;                  /* List of file name patterns to exclude */

};


/* Parser association structure */
struct prsAssociation {

    unsigned char           pucItemName[UTL_FILE_PATH_MAX + 1];         /* Item name */
    unsigned char           pucMimeType[UTL_FILE_PATH_MAX + 1];         /* Mime type */
    unsigned char           pucExtension[UTL_FILE_PATH_MAX + 1];        /* File name extension */

};


/* Parser structure */
struct prsParser {

    unsigned int            uiLanguageID;                               /* Language ID */

    void                    *pvLngTokenizer;                            /* Tokenizer structure */
    void                    *pvLngConverter;                            /* Converter structure */
    void                    *pvLngUnicodeNormalizer;                    /* Unicode normalizer structure */

    unsigned int            uiTermLengthMinimum;                        /* Minimum term length */
    unsigned int            uiTermLengthMaximum;                        /* Maximum term length */

    unsigned char           *pucItemName;                               /* Item name (default override) */
    unsigned char           *pucMimeType;                               /* Mime type (default override) */

    struct prsAssociation   *ppaPrsAssociation;                         /* Item/mime type to file name associations */
    unsigned int            uiPrsAssociationLength;                     /* Length of the association array */

    unsigned int            uiDocumentKey;                              /* Current document key (0 = no keys produced) */
    unsigned int            uiDocumentCount;                            /* Current number of documents submitted to the indexer */
    unsigned int            uiDocumentCountMax;                         /* Maximum number of documents to submit to the indexer */

    boolean                 bStoreFilePaths;                            /* Store file paths */

    boolean                 bCleanUnicode;                              /* Clean unicode */
    boolean                 bSkipUnicodeErrors;                         /* Skip unicode errors */

    boolean                 bSuppressMessages;                          /* Message suppression */

};



/*---------------------------------------------------------------------------*/


/*
** Public function prototypes
*/

int iPrsParseTextFile (struct prsParser *pppPrsParser, struct prsFormat *ppfPrsFormat,  
        unsigned char *pucFilePath, FILE *pfInputFile, FILE *pfOutputFile, float *pfDataLength);


/*---------------------------------------------------------------------------*/


/* 
** C++ wrapper
*/

#if defined(__cplusplus)
}
#endif    /* defined(__cplusplus) */


/*---------------------------------------------------------------------------*/


#endif    /* !defined(PRS_PARSER_H) */


/*---------------------------------------------------------------------------*/
