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

    Module:     index.h

    Author:     Francois Schiettecatte (FS Consulting LLC.)

    Created:    15 September 1995

    Purpose:    This is the header file for index.c.

*/


/*---------------------------------------------------------------------------*/


#if !defined(SRCH_INDEX_H)
#define SRCH_INDEX_H


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

/* Index open intent */
#define    SRCH_INDEX_INTENT_INVALID            (0)                 /* Invalid */
#define    SRCH_INDEX_INTENT_CREATE             (1)                 /* For creating an index */
#define    SRCH_INDEX_INTENT_SEARCH             (2)                 /* For searching an index */

#define SRCH_INDEX_INTENT_VALID(n)              (((n) >= SRCH_INDEX_INTENT_CREATE) && \
                                                        ((n) <= SRCH_INDEX_INTENT_SEARCH))


/*---------------------------------------------------------------------------*/


/*
** Structures
*/

/* Index Structures
**
** Two structures are provided, the basic structure 'srchIndex' contains all
** the information needed to search a index, the build structure
** 'srchIndexBuild' is used only for building.
*/
struct srchIndexBuild {

    unsigned char           *pucTemporaryDirectoryPath;     /* Temporary directory path */

    unsigned char           *pucStopListFilePath;           /* Stop list file path */
    unsigned int            uiStopListTypeID;               /* Stop list type ID */
    unsigned int            uiStopListID;                   /* Stop list ID */
    void                    *pvLngStopList;                 /* Stop list structure */

    void                    *pvLngStemmer;                  /* Stemmer structure */

    unsigned char           *pucDocumentDataEntry;          /* Document data entry */
    unsigned int            uiDocumentDataEntryLength;      /* Document data entry length */

    void                    *pvUtlTermTrie;                 /* This trie holds the terms trie */

    size_t                  zMemorySize;                    /* This is used to store the allocated memory size while indexing (bytes) */

    unsigned int            uiLastDocumentID;               /* This is used to determine when it is safe to flush the postings blocks to disk */
    
    unsigned int            uiIndexerMemorySizeMaximum;     /* Suggested maximum memory size to use while indexing (megabytes) */
    
    unsigned int            uiIndexFileNumber;              /* Number of index files created during the indexing process */
    
    unsigned int            uiDocumentKeysFileNumber;       /* Number of documents keys files created during the indexing process */
    
    unsigned int            uiUniqueTermCount;              /* Unique term count in the current index block - excluding stop terms */
    unsigned int            uiTotalTermCount;               /* Total term count in the current index block - excluding stop terms */
    unsigned int            uiUniqueStopTermCount;          /* Unique stop term count in the current index block */
    unsigned int            uiTotalStopTermCount;           /* Total stop term count in the current index block */

    unsigned int            uiDocumentCount;                /* Number of documents in the current postings block */

    unsigned char           *pucIndexBlock;                 /* Temporary buffer for compressing index blocks */
    unsigned int            uiIndexBlockLength;             /* Capacity of the index blocks buffer */

    unsigned char           *pucFieldIDBitmap;              /* Field ID bitmap */
    unsigned int            uiFieldIDBitmapLength;          /* Field ID bitmap length */

    unsigned int            uiDuplicateDocumentKeysCount;   /* Number of duplicate document keys detected */
    
    void                    *pvLngConverterUTF8ToWChar;     /* Character set converter */
    void                    *pvLngConverterWCharToUTF8;     /* Character set converter */
};


struct srchIndex {

    unsigned char           *pucIndexName;                  /* Index name */
    unsigned char           *pucIndexPath;                  /* Index path */

    unsigned int            uiMajorVersion;                 /* Major version */
    unsigned int            uiMinorVersion;                 /* Minor version */
    unsigned int            uiPatchVersion;                 /* Patch version */

    FILE                    *pfLockFile;                    /* Lock file */
    unsigned int            uiIntent;                       /* Current intent */
    
    unsigned int            uiLanguageID;                   /* Language ID */
    unsigned int            uiTokenizerID;                  /* Tokenizer ID */
    unsigned int            uiStemmerID;                    /* Stemmer ID  */

    void                    *pvUtlDocumentTable;            /* Document table */
    void                    *pvUtlDocumentData;             /* Document data */
    void                    *pvUtlIndexData;                /* Index data */
    void                    *pvUtlKeyDictionary;            /* Key dictionary */
    void                    *pvUtlTermDictionary;           /* Term dictionary */
    void                    *pvUtlIndexInformation;         /* Index information */

    /* Scalars */
    unsigned int            uiTermLengthMaximum;            /* Maximum term length in this index */
    unsigned int            uiTermLengthMinimum;            /* Minimum term length in this index */

    unsigned long           ulUniqueTermCount;              /* Unique term count - excluding stop terms */
    unsigned long           ulTotalTermCount;               /* Total term count - excluding stop terms */
    unsigned long           ulUniqueStopTermCount;          /* Unique stop term count */
    unsigned long           ulTotalStopTermCount;           /* Total stop term count */

    unsigned int            uiDocumentCount;                /* Number of documents - limited to 4 billion documents */
    unsigned int            uiDocumentTermCountMaximum;     /* Maximum document term count */
    unsigned int            uiDocumentTermCountMinimum;     /* Minimum document term count */

    unsigned int            uiFieldIDMaximum;               /* Maximum field ID */

     time_t                 tLastUpdateTime;                /* Last update time */
    /* End of scalars */

    struct srchIndexBuild   *psibSrchIndexBuild;            /* For building (Creating) */

};


/*---------------------------------------------------------------------------*/


/*
** Public function prototypes
*/

int iSrchIndexOpen (unsigned char *pucIndexDirectoryPath, 
        unsigned char *pucConfigurationDirectoryPath, 
        unsigned char *pucIndexName, unsigned int uiIntent, 
        struct srchIndex **ppsiSrchIndex);

int iSrchIndexClose (struct srchIndex *psiSrchIndex);

int iSrchIndexAbort (struct srchIndex *psiSrchIndex,
        unsigned char *pucConfigurationDirectoryPath);


/*---------------------------------------------------------------------------*/


/* 
** C++ wrapper
*/

#if defined(__cplusplus)
}
#endif    /* defined(__cplusplus) */


/*---------------------------------------------------------------------------*/


#endif    /* !defined(SRCH_INDEX_H) */


/*---------------------------------------------------------------------------*/
