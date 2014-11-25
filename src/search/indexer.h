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

    Module:     indexer.h

    Author:     Francois Schiettecatte (FS Consulting LLC.)

    Created:    24 February 1994

    Purpose:    This is the header file for indexer.c. This file contains the
                indexer functions.

*/


/*---------------------------------------------------------------------------*/


#if !defined(SRCH_INDEXER_H)
#define SRCH_INDEXER_H


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

/* The minimum and maximum amount of memory to use (megabytes) */
#define SRCH_INDEXER_MEMORY_MINIMUM         (256)
#define SRCH_INDEXER_MEMORY_MAXIMUM         (2048)


/*---------------------------------------------------------------------------*/


/*
** Structures
*/

/* Search indexer structure */
struct srchIndexer {

    unsigned char   *pucIndexDirectoryPath;             /* Index directory path */
    unsigned char   *pucConfigurationDirectoryPath;     /* Configuration directory path */
    unsigned char   *pucTemporaryDirectoryPath;         /* Temporary directory path */

    unsigned char   *pucIndexName;                      /* Index name */
    unsigned char   *pucIndexDescription;               /* Index description */

    unsigned int    uiTermLengthMinimum;                /* Minimum term length */
    unsigned int    uiTermLengthMaximum;                /* Maximum term length */
    unsigned char   *pucLanguageCode;                   /* Language code (set from index stream) */
    unsigned char   *pucTokenizerName;                  /* Tokenizer set name (set from index stream) */
    unsigned char   *pucStemmerName;                    /* Stemmer name */
    unsigned char   *pucStopListName;                   /* Stop list name */
    unsigned char   *pucStopListFilePath;               /* Stop list file path */

    unsigned int    uiIndexerMemorySizeMaximum;         /* Maximum memory to use (megabytes) */
    boolean         bSuppressMessages;                  /* Suppress messages if set to true */

    FILE            *pfFile;                            /* File descriptor from which we read the index stream */

};


/*---------------------------------------------------------------------------*/


/*
** Public function prototypes
*/

int iSrchIndexerCreateIndexFromSearchIndexer (struct srchIndexer *psiSrchIndexer);    


/*---------------------------------------------------------------------------*/


/* 
** C++ wrapper
*/

#if defined(__cplusplus)
}
#endif    /* defined(__cplusplus) */


/*---------------------------------------------------------------------------*/


#endif    /* !defined(SRCH_INDEXER_H) */


/*---------------------------------------------------------------------------*/
