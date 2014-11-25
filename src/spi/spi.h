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

    Module:     spi.h

    Author:     Francois Schiettecatte (FS Consulting LLC.)

    Created:    4 Feb 1994

    Purpose:    This is the header file for spi.c.

*/


/*---------------------------------------------------------------------------*/


#if !defined(SPI_H)
#define SPI_H


/*---------------------------------------------------------------------------*/


/*
** Includes
*/

#include "utils.h"


/*---------------------------------------------------------------------------*/


/* C++ wrapper */
#if defined(__cplusplus)
extern "C" {
#endif    /* defined(__cplusplus) */


/*---------------------------------------------------------------------------*/


/*
** Defines
*/


/* Maximum/minimum weight/force minimum weight */
#define SPI_NORMALIZED_SORT_KEY_MAXIMUM                 (1000)
#define SPI_NORMALIZED_SORT_KEY_MINIMUM                 (1)


/* Maximum/minimum sort keys */
#define SPI_SORT_KEY_DOUBLE_MAXIMUM                     (DBL_MAX)
#define SPI_SORT_KEY_DOUBLE_MINIMUM                     (0)

#define SPI_SORT_KEY_FLOAT_MAXIMUM                      (FLT_MAX)
#define SPI_SORT_KEY_FLOAT_MINIMUM                      (0)

#define SPI_SORT_KEY_UINT_MAXIMUM                       (UINT_MAX)
#define SPI_SORT_KEY_UINT_MINIMUM                       (0)

#define SPI_SORT_KEY_ULONG_MAXIMUM                      (ULONG_MAX)
#define SPI_SORT_KEY_ULONG_MINIMUM                      (0)


/* Recomended maximum URL length */
#define SPI_URL_MAXIMUM_LENGTH                          (255)

/* Recomended maximum key length */
#define SPI_DOCUMENT_KEY_MAXIMUM_LENGTH                 (255)

/* Recomended maximum item name length */
#define SPI_ITEM_NAME_MAXIMUM_LENGTH                    (40)

/* Recomended maximum mime type length */
#define SPI_MIME_TYPE_MAXIMUM_LENGTH                    (40)

/* Maximum date length, usually represented in eight bytes (YYYYMMDD) */
#define SPI_ANSI_DATE_LENGTH                            (8)

/* Maximum time length, usually represented in six bytes (HHMMSS) */
#define SPI_ANSI_TIME_LENGTH                            (6)

/* Recomended maximum title length */
#define SPI_TITLE_MAXIMUM_LENGTH                        (255)

/* Sort type */
#define SPI_SORT_TYPE_UNKNOWN                           (0)
#define SPI_SORT_TYPE_DOUBLE_ASC                        (1)
#define SPI_SORT_TYPE_DOUBLE_DESC                       (2)
#define SPI_SORT_TYPE_FLOAT_ASC                         (3)
#define SPI_SORT_TYPE_FLOAT_DESC                        (4)
#define SPI_SORT_TYPE_UINT_ASC                          (5)
#define SPI_SORT_TYPE_UINT_DESC                         (6)
#define SPI_SORT_TYPE_ULONG_ASC                         (7)
#define SPI_SORT_TYPE_ULONG_DESC                        (8)
#define SPI_SORT_TYPE_UCHAR_ASC                         (9)
#define SPI_SORT_TYPE_UCHAR_DESC                        (10)
#define SPI_SORT_TYPE_NO_SORT                           (11)

#define SPI_SORT_TYPE_VALID(n)                          (((n) >= SPI_SORT_TYPE_DOUBLE_ASC) && \
                                                                ((n) <= SPI_SORT_TYPE_NO_SORT))


/* Search report */
#define SPI_SEARCH_REPORT_TITLE                         (unsigned char *)"Report for this search"
#define SPI_SEARCH_REPORT_LANGUAGE_NAME                 (unsigned char *)"en"
#define SPI_SEARCH_REPORT_ITEM_NAME                     (unsigned char *)"document"
#define SPI_SEARCH_REPORT_MIME_TYPE                     (unsigned char *)"application/x-mps-search-report"


/* Recomended maximum struct spiServerInfo lengths */
#define SPI_SERVER_NAME_MAXIMUM_LENGTH                  (255)
#define SPI_SERVER_DESCRIPTION_MAXIMUM_LENGTH           (255)
#define SPI_SERVER_ADMIN_NAME_MAXIMUM_LENGTH            (255)
#define SPI_SERVER_ADMIN_EMAIL_MAXIMUM_LENGTH           (255)
#define SPI_SERVER_RANKING_ALGORITHM_MAXIMUM_LENGTH     (255)
#define SPI_SERVER_WEIGHT_MINIMUM                       SPI_SORT_KEY_DOUBLE_MINIMUM
#define SPI_SERVER_WEIGHT_MAXIMUM                       SPI_SORT_KEY_DOUBLE_MAXIMUM

#define SPI_SERVER_INDEX_COUNT_UNKNOWN                  (0)


/* Recomended maximum struct spiIndexInfo/spiServerIndexInfo lengths */
#define SPI_INDEX_NAME_MAXIMUM_LENGTH                   (255)
#define SPI_INDEX_DESCRIPTION_MAXIMUM_LENGTH            (255)
#define SPI_INDEX_LANGUAGE_MAXIMUM_LENGTH               (255)
#define SPI_INDEX_TOKENIZER_MAXIMUM_LENGTH              (255)
#define SPI_INDEX_STEMMER_MAXIMUM_LENGTH                (255)
#define SPI_INDEX_STOP_LIST_MAXIMUM_LENGTH              (255)

#define SPI_INDEX_ACCESS_UNKNOWN                        (0)
#define SPI_INDEX_ACCESS_PUBLIC                         (1)
#define SPI_INDEX_ACCESS_PRIVATE                        (2)

#define SPI_INDEX_ACCESS_VALID(n)                       (((n) >= SPI_INDEX_ACCESS_PUBLIC) && \
                                                                ((n) <= SPI_INDEX_ACCESS_PRIVATE))


#define SPI_INDEX_DOCUMENT_COUNT_UNKNOWN                (0)
#define SPI_INDEX_TOTAL_TERM_COUNT_UNKNOWN              (0)
#define SPI_INDEX_UNIQUE_TERM_COUNT_UNKNOWN             (0)
#define SPI_INDEX_STOP_TERM_COUNT_UNKNOWN               (0)

#define SPI_INDEX_UPDATE_UNKNOWN                        (0)
#define SPI_INDEX_UPDATE_ALWAYS                         (1)
#define SPI_INDEX_UPDATE_HOURLY                         (2)
#define SPI_INDEX_UPDATE_DAILY                          (3)
#define SPI_INDEX_UPDATE_WEEKLY                         (4)
#define SPI_INDEX_UPDATE_MONTHY                         (5)
#define SPI_INDEX_UPDATE_YEARLY                         (6)

#define SPI_INDEX_UPDATE_VALID(n)                       (((n) >= SPI_INDEX_UPDATE_ALWAYS) && \
                                                                ((n) <= SPI_INDEX_UPDATE_YEARLY))


#define SPI_INDEX_CASE_UNKNOWN                          (0)
#define SPI_INDEX_CASE_SENSITIVE                        (1)
#define SPI_INDEX_CASE_INSENSITIVE                      (2)



/* Recomended maximum field name length */
#define SPI_FIELD_NAME_MAXIMUM_LENGTH                   (255)
#define SPI_FIELD_DESCRIPTION_MAXIMUM_LENGTH            (255)

#define SPI_FIELD_TYPE_UNKNOWN                          (0)
#define SPI_FIELD_TYPE_TEXT                             (1)
#define SPI_FIELD_TYPE_NUMERIC                          (2)
#define SPI_FIELD_TYPE_DATE                             (3)

#define SPI_FIELD_TYPE_VALID(n)                         (((n) >= SPI_FIELD_TYPE_TEXT) && \
                                                                ((n) <= SPI_FIELD_TYPE_DATE))



/* Document item type (defined in the mime type) */
#define SPI_ITEM_TYPE_UNKNOWN                           (0)
#define SPI_ITEM_TYPE_SYSTEM                            (1)
#define SPI_ITEM_TYPE_INDEX                             (2)

#define SPI_ITEM_TYPE_VALID(n)                          (((n) >= SPI_ITEM_TYPE_SYSTEM) && \
                                                                ((n) <= SPI_ITEM_TYPE_INDEX))


/* Term match */
#define SPI_TERM_MATCH_UNKNOWN                          (0)
#define SPI_TERM_MATCH_REGULAR                          (1)
#define SPI_TERM_MATCH_STOP                             (2)
#define SPI_TERM_MATCH_WILDCARD                         (3)
#define SPI_TERM_MATCH_SOUNDEX                          (4)
#define SPI_TERM_MATCH_METAPHONE                        (5)
#define SPI_TERM_MATCH_PHONIX                           (6)
#define SPI_TERM_MATCH_TYPO                             (7)
#define SPI_TERM_MATCH_REGEX                            (8)

#define SPI_TERM_MATCH_VALID(n)                         (((n) >= SPI_TERM_MATCH_REGULAR) && \
                                                                ((n) <= SPI_TERM_MATCH_TYPO))


/* Term case */
#define SPI_TERM_CASE_UNKNOWN                           (0)
#define SPI_TERM_CASE_SENSITIVE                         (1)
#define SPI_TERM_CASE_INSENSITIVE                       (2)

#define SPI_TERM_CASE_VALID(n)                          (((n) >= SPI_TERM_CASE_UNKNOWN) && \
                                                                ((n) <= SPI_TERM_CASE_INSENSITIVE))


/* Term type */
#define SPI_TERM_TYPE_UNKNOWN                           (0)
#define SPI_TERM_TYPE_REGULAR                           (1)
#define SPI_TERM_TYPE_STOP                              (2)
#define SPI_TERM_TYPE_FREQUENT                          (3)

#define SPI_TERM_TYPE_VALID(n)                          (((n) >= SPI_TERM_TYPE_UNKNOWN) && \
                                                                ((n) <= SPI_TERM_TYPE_FREQUENT))

/* Term counts */
#define SPI_TERM_COUNT_UNKNOWN                          (0)
#define SPI_TERM_DOCUMENT_COUNT_UNKNOWN                 (0)


/* Document chunk types */
#define SPI_CHUNK_TYPE_UNKNOWN                          (0)
#define SPI_CHUNK_TYPE_DOCUMENT                         (1)
#define SPI_CHUNK_TYPE_BYTE                             (2)

#define SPI_CHUNK_TYPE_VALID(n)                         (((n) >= SPI_CHUNK_TYPE_UNKNOWN) && \
                                                                ((n) <= SPI_CHUNK_TYPE_BYTE))


/*---------------------------------------------------------------------------*/


/* Error code definitions for the SPI framework, these codes are understood
** by the SPI framework and will be acted upon accordingly, this includes
** putting together diagnostics for the client.
*/

#define SPI_NoError                                     (0)
#define SPI_MemError                                    (-1)
#define SPI_ParameterError                              (-2)
#define SPI_ReturnParameterError                        (-3)
#define SPI_MiscError                                   (-4)

#define SPI_ExceededLoadMaximum                         (-100)

#define SPI_InvalidSession                              (-200)
#define SPI_InvalidIndexDirectory                       (-201)
#define SPI_InvalidConfigurationDirectory               (-202)
#define SPI_InvalidTemporaryDirectory                   (-203)
#define SPI_InitializeServerFailed                      (-204)
#define SPI_ShutdownServerFailed                        (-205)

#define SPI_InvalidIndexName                            (-301)
#define SPI_InvalidIndex                                (-300)
#define SPI_OpenIndexFailed                             (-302)
#define SPI_CloseIndexFailed                            (-303)

#define SPI_InvalidLanguageCode                         (-400)
#define SPI_InvalidSearchText                           (-401)
#define SPI_InvalidPositiveFeedbackText                 (-402)
#define SPI_InvalidNegativeFeedbackText                 (-403)
#define SPI_InvalidSearchResultsRange                   (-404)
#define SPI_SearchIndexFailed                           (-405)

#define SPI_InvalidDocumentKey                          (-501)
#define SPI_InvalidItemName                             (-502)
#define SPI_InvalidMimeType                             (-503)
#define SPI_InvalidChunkType                            (-504)
#define SPI_InvalidChunkRange                           (-505)
#define SPI_RetrieveDocumentFailed                      (-506)

#define SPI_GetServerInfoFailed                         (-600)

#define SPI_GetServerIndexInfoFailed                    (-610)
#define SPI_ServerHasNoIndices                          (-611)

#define SPI_GetIndexInfoFailed                          (-620)

#define SPI_GetIndexFieldInfoFailed                     (-630)
#define SPI_IndexHasNoSearchFields                      (-631)

#define SPI_InvalidTermMatch                            (-640)
#define SPI_InvalidTermCase                             (-641)
#define SPI_InvalidTerm                                 (-642)
#define SPI_InvalidFieldName                            (-643)
#define SPI_GetIndexTermInfoFailed                      (-644)
#define SPI_IndexHasNoTerms                             (-645)

#define SPI_GetDocumentInfoFailed                       (-650)

#define SPI_GetIndexNameFailed                          (-660)


#define SPI_ERROR_VALID(n)                              (((n) <= SPI_NoError) && \
                                                                ((n) >= SPI_GetIndexNameFailed))


/*---------------------------------------------------------------------------*/


/*
** Structures
*/

/* SPI structure used to track an spi session - all fields are read-only
** except for pvClientPtr in which can be used to store a pointer.
*/
struct spiSession {
    unsigned char            *pucIndexDirectoryPath;
    unsigned char            *pucConfigurationDirectoryPath;
    unsigned char            *pucTemporaryDirectoryPath;
    void                    *pvClientPtr;
};


/*---------------------------------------------------------------------------*/


/* SPI structures used to communicate with the search engine */


/* Document item */
struct spiDocumentItem {
    unsigned char           *pucItemName;                           /* Required */
    unsigned char           *pucMimeType;                           /* Required */
    unsigned int            uiLength;                               /* Required */
    unsigned char           *pucUrl;                                /* Optional */
    void                    *pvData;                                /* Optional */
    unsigned int            uiDataLength;                           /* Required */
};


/* Search result */
struct spiSearchResult {
    unsigned char           *pucIndexName;                          /* Required */
    unsigned char           *pucDocumentKey;                        /* Required */
    unsigned char           *pucTitle;                              /* Required */
    union {                                                         /* Required */
        float               fSortKey;                               /* Float sort key */
        double              dSortKey;                               /* Double sort key */
        unsigned int        uiSortKey;                              /* Unsigned integer sort key - 32 bit */
        unsigned long       ulSortKey;                              /* Unsigned long sort key - 64 bit */
        unsigned char       *pucSortKey;                            /* Character sort key */
    };
    unsigned char           *pucLanguageCode;                       /* Optional */
    unsigned int            uiRank;                                 /* Optional */
    unsigned int            uiTermCount;                            /* Optional */
    unsigned long           ulAnsiDate;                             /* Optional */
    struct spiDocumentItem  *psdiSpiDocumentItems;                  /* Optional */
    unsigned int            uiDocumentItemsLength;                  /* Optional */
};


/* Search response */
struct spiSearchResponse {
    struct spiSearchResult  *pssrSpiSearchResults;                  /* Optional */
    unsigned int            uiSpiSearchResultsLength;               /* Optional */
    unsigned int            uiTotalResults;                         /* Required */
    unsigned int            uiStartIndex;                           /* Required */
    unsigned int            uiEndIndex;                             /* Required */
    unsigned int            uiSortType;                             /* Required */
    double                  dMaxSortKey;                            /* Required */
    double                  dSearchTime;                            /* Optional */
};


/* Server information */
struct spiServerInfo {
    unsigned char           *pucName;                               /* Optional */
    unsigned char           *pucDescription;                        /* Optional */
    unsigned char           *pucAdminName;                          /* Optional */
    unsigned char           *pucAdminEmail;                         /* Optional */
    unsigned int            uiIndexCount;                           /* Optional */
    unsigned char           *pucRankingAlgorithm;                   /* Optional */
    double                  dWeightMinimum;                         /* Required */
    double                  dWeightMaximum;                         /* Required */
};


/* Server index information */
struct spiServerIndexInfo {
    unsigned char           *pucName;                               /* Required */
    unsigned char           *pucDescription;                        /* Optional */
};


/* Index information */
struct spiIndexInfo {
    unsigned char           *pucName;                               /* Required */
    unsigned char           *pucDescription;                        /* Optional */
    unsigned char           *pucLanguageCode;                       /* Optional */
    unsigned char           *pucTokenizerName;                      /* Optional */
    unsigned char           *pucStemmerName;                        /* Optional */
    unsigned char           *pucStopListName;                       /* Optional */
    unsigned int            uiDocumentCount;                        /* Optional */
    unsigned long           ulUniqueTermCount;                      /* Optional */
    unsigned long           ulTotalTermCount;                       /* Optional */
    unsigned long           ulUniqueStopTermCount;                  /* Optional */
    unsigned long           ulTotalStopTermCount;                   /* Optional */
    unsigned int            uiAccessControl;                        /* Optional */
    unsigned int            uiUpdateFrequency;                      /* Optional */
    unsigned long           ulLastUpdateAnsiDate;                   /* Optional */
    unsigned int            uiCaseSensitive;                        /* Optional */
};


/* Field information */
struct spiFieldInfo {
    unsigned char           *pucName;                               /* Required */
    unsigned char           *pucDescription;                        /* Optional */
    unsigned int            uiType;                                 /* Optional */
};


/* Term information */
struct spiTermInfo {
    unsigned char           *pucTerm;                               /* Required */
    unsigned int            uiType;                                 /* Required */
    unsigned int            uiCount;                                /* Optional */
    unsigned int            uiDocumentCount;                        /* Optional */
};


/* Document information */
struct spiDocumentInfo {
    unsigned char            *pucIndexName;                         /* Required */
    unsigned char           *pucDocumentKey;                        /* Required */
    unsigned char           *pucTitle;                              /* Required */
    unsigned char           *pucLanguageCode;                       /* Optional */
    unsigned int            uiRank;                                 /* Optional */
    unsigned int            uiTermCount;                            /* Optional */
    unsigned long           ulAnsiDate;                             /* Optional */
    struct spiDocumentItem  *psdiSpiDocumentItems;                  /* Optional */
    unsigned int            uiDocumentItemsLength;                  /* Required */
};


/*---------------------------------------------------------------------------*/


/* 
** Macros
*/

/* Macros to copy a search result, these does NOT duplicate any data */
#define SPI_COPY_SEARCH_RESULT_FLOAT(pssr1, pssr2) \
{ \
    (pssr1)->pucIndexName = (pssr2)->pucIndexName; \
    (pssr1)->pucDocumentKey = (pssr2)->pucDocumentKey; \
    (pssr1)->pucTitle = (pssr2)->pucTitle; \
    (pssr1)->fSortKey = (pssr2)->fSortKey; \
    (pssr1)->pucLanguageCode = (pssr2)->pucLanguageCode; \
    (pssr1)->uiRank = (pssr2)->uiRank; \
    (pssr1)->uiTermCount = (pssr2)->uiTermCount; \
    (pssr1)->ulAnsiDate = (pssr2)->ulAnsiDate; \
    (pssr1)->psdiSpiDocumentItems = (pssr2)->psdiSpiDocumentItems; \
    (pssr1)->uiDocumentItemsLength = (pssr2)->uiDocumentItemsLength; \
}

#define SPI_COPY_SEARCH_RESULT_DOUBLE(pssr1, pssr2) \
{ \
    (pssr1)->pucIndexName = (pssr2)->pucIndexName; \
    (pssr1)->pucDocumentKey = (pssr2)->pucDocumentKey; \
    (pssr1)->pucTitle = (pssr2)->pucTitle; \
    (pssr1)->dSortKey = (pssr2)->dSortKey; \
    (pssr1)->pucLanguageCode = (pssr2)->pucLanguageCode; \
    (pssr1)->uiRank = (pssr2)->uiRank; \
    (pssr1)->uiTermCount = (pssr2)->uiTermCount; \
    (pssr1)->ulAnsiDate = (pssr2)->ulAnsiDate; \
    (pssr1)->psdiSpiDocumentItems = (pssr2)->psdiSpiDocumentItems; \
    (pssr1)->uiDocumentItemsLength = (pssr2)->uiDocumentItemsLength; \
}

#define SPI_COPY_SEARCH_RESULT_UINT(pssr1, pssr2) \
{ \
    (pssr1)->pucIndexName = (pssr2)->pucIndexName; \
    (pssr1)->pucDocumentKey = (pssr2)->pucDocumentKey; \
    (pssr1)->pucTitle = (pssr2)->pucTitle; \
    (pssr1)->uiSortKey = (pssr2)->uiSortKey; \
    (pssr1)->pucLanguageCode = (pssr2)->pucLanguageCode; \
    (pssr1)->uiRank = (pssr2)->uiRank; \
    (pssr1)->uiTermCount = (pssr2)->uiTermCount; \
    (pssr1)->ulAnsiDate = (pssr2)->ulAnsiDate; \
    (pssr1)->psdiSpiDocumentItems = (pssr2)->psdiSpiDocumentItems; \
    (pssr1)->uiDocumentItemsLength = (pssr2)->uiDocumentItemsLength; \
}

#define SPI_COPY_SEARCH_RESULT_ULLONG(pssr1, pssr2) \
{ \
    (pssr1)->pucIndexName = (pssr2)->pucIndexName; \
    (pssr1)->pucDocumentKey = (pssr2)->pucDocumentKey; \
    (pssr1)->pucTitle = (pssr2)->pucTitle; \
    (pssr1)->ulSortKey = (pssr2)->ulSortKey; \
    (pssr1)->pucLanguageCode = (pssr2)->pucLanguageCode; \
    (pssr1)->uiRank = (pssr2)->uiRank; \
    (pssr1)->uiTermCount = (pssr2)->uiTermCount; \
    (pssr1)->ulAnsiDate = (pssr2)->ulAnsiDate; \
    (pssr1)->psdiSpiDocumentItems = (pssr2)->psdiSpiDocumentItems; \
    (pssr1)->uiDocumentItemsLength = (pssr2)->uiDocumentItemsLength; \
}

#define SPI_COPY_SEARCH_RESULT_UCHAR(pssr1, pssr2)    \
{ \
    (pssr1)->pucIndexName = (pssr2)->pucIndexName; \
    (pssr1)->pucDocumentKey = (pssr2)->pucDocumentKey; \
    (pssr1)->pucTitle = (pssr2)->pucTitle; \
    (pssr1)->pucSortKey = (pssr2)->pucSortKey; \
    (pssr1)->pucLanguageCode = (pssr2)->pucLanguageCode; \
    (pssr1)->uiRank = (pssr2)->uiRank; \
    (pssr1)->uiTermCount = (pssr2)->uiTermCount; \
    (pssr1)->ulAnsiDate = (pssr2)->ulAnsiDate; \
    (pssr1)->psdiSpiDocumentItems = (pssr2)->psdiSpiDocumentItems; \
    (pssr1)->uiDocumentItemsLength = (pssr2)->uiDocumentItemsLength; \
}

#define SPI_COPY_SEARCH_RESULT(pssr1, pssr2) \
{ \
    (pssr1)->pucIndexName = (pssr2)->pucIndexName; \
    (pssr1)->pucDocumentKey = (pssr2)->pucDocumentKey; \
    (pssr1)->pucTitle = (pssr2)->pucTitle; \
    (pssr1)->pucLanguageCode = (pssr2)->pucLanguageCode; \
    (pssr1)->uiRank = (pssr2)->uiRank; \
    (pssr1)->uiTermCount = (pssr2)->uiTermCount; \
    (pssr1)->ulAnsiDate = (pssr2)->ulAnsiDate; \
    (pssr1)->psdiSpiDocumentItems = (pssr2)->psdiSpiDocumentItems; \
    (pssr1)->uiDocumentItemsLength = (pssr2)->uiDocumentItemsLength; \
}


/* Macros to swap two search results */
#define SPI_SWAP_SEARCH_RESULT_FLOAT(pssr1, pssr2) \
{ \
    struct spiSearchResult psshMacroSpiSearchResult; \
    SPI_COPY_SEARCH_RESULT_FLOAT(&psshMacroSpiSearchResult, pssr1); \
    SPI_COPY_SEARCH_RESULT_FLOAT(pssr1, pssr2); \
    SPI_COPY_SEARCH_RESULT_FLOAT(pssr2, &psshMacroSpiSearchResult); \
}

#define SPI_SWAP_SEARCH_RESULT_DOUBLE(pssr1, pssr2) \
{ \
    struct spiSearchResult psshMacroSpiSearchResult; \
    SPI_COPY_SEARCH_RESULT_DOUBLE(&psshMacroSpiSearchResult, pssr1); \
    SPI_COPY_SEARCH_RESULT_DOUBLE(pssr1, pssr2); \
    SPI_COPY_SEARCH_RESULT_DOUBLE(pssr2, &psshMacroSpiSearchResult); \
}

#define SPI_SWAP_SEARCH_RESULT_UINT(pssr1, pssr2) \
{ \
    struct spiSearchResult psshMacroSpiSearchResult; \
    SPI_COPY_SEARCH_RESULT_UINT(&psshMacroSpiSearchResult, pssr1); \
    SPI_COPY_SEARCH_RESULT_UINT(pssr1, pssr2); \
    SPI_COPY_SEARCH_RESULT_UINT(pssr2, &psshMacroSpiSearchResult); \
}

#define SPI_SWAP_SEARCH_RESULT_ULLONG(pssr1, pssr2) \
{ \
    struct spiSearchResult psshMacroSpiSearchResult; \
    SPI_COPY_SEARCH_RESULT_ULLONG(&psshMacroSpiSearchResult, pssr1); \
    SPI_COPY_SEARCH_RESULT_ULLONG(pssr1, pssr2); \
    SPI_COPY_SEARCH_RESULT_ULLONG(pssr2, &psshMacroSpiSearchResult); \
}

#define SPI_SWAP_SEARCH_RESULT_UCHAR(pssr1, pssr2) \
{ \
    struct spiSearchResult psshMacroSpiSearchResult; \
    SPI_COPY_SEARCH_RESULT_UCHAR(&psshMacroSpiSearchResult, pssr1); \
    SPI_COPY_SEARCH_RESULT_UCHAR(pssr1, pssr2); \
    SPI_COPY_SEARCH_RESULT_UCHAR(pssr2, &psshMacroSpiSearchResult); \
}

#define SPI_SWAP_SEARCH_RESULT(pssr1, pssr2) \
{ \
    struct spiSearchResult psshMacroSpiSearchResult; \
    SPI_COPY_SEARCH_RESULT(&psshMacroSpiSearchResult, pssr1); \
    SPI_COPY_SEARCH_RESULT(pssr1, pssr2); \
    SPI_COPY_SEARCH_RESULT(pssr2, &psshMacroSpiSearchResult); \
}

/*---------------------------------------------------------------------------*/


/*
** Public function prototypes
*/


/* 
** ==============================
** ===  Server SPI Functions  ===
** ==============================
*/


/*

    Function:   iSpiInitializeServer()

    Purpose:    This is called when the server is initialized.

                NOTE - This function may be called more than once

    Parameters: pssSpiSession   spi session structure

    Globals:    none

    Returns:    SPI Error Code

*/
int iSpiInitializeServer (struct spiSession *pssSpiSession);


/*

    Function:   iSpiShutdownServer()

    Purpose:    This is called when the server is being shut down.

                NOTE - This function may be called more than once

    Parameters: pssSpiSession   spi session structure

    Globals:    none

    Returns:    SPI Error Code

*/
int iSpiShutdownServer (struct spiSession *pssSpiSession);


/*---------------------------------------------------------------------------*/


/* 
** =============================
** ===  Index SPI Functions  ===
** =============================
*/


/*

    Function:   iSpiOpenIndex()

    Purpose:    This function will be called to open a index before any
                operations are performed on it.

                This function must allocate the index structure returned. 
                Note that the index structure is opaque to the SPI framework 
                so you really can  get any size of pointer (even NULL, though 
                this may not be a good idea).

    Parameters: pssSpiSession   spi session structure
                pucIndexName    index name
                ppvIndex        return pointer for the index structure

    Globals:    none

    Returns:    SPI Error Code

*/
int iSpiOpenIndex (struct spiSession *pssSpiSession, 
        unsigned char *pucIndexName, void **ppvIndex);


/*

    Function:   iSpiCloseIndex()

    Purpose:    This function will be called after all operations performed
                on this index are done.

                This function must free the index structure passed, it will
                not be referenced anymore. Note that the index structure 
                is opaque to the SPI framework so you really can 
                get any size of pointer (even NULL, though this may not be a 
                good idea). The pointer gets allocated in iSpiOpenIndex.

    Parameters: pssSpiSession   spi session structure
                pvIndex         index structure

    Globals:    none

    Returns:    SPI Error Code

*/
int iSpiCloseIndex (struct spiSession *pssSpiSession, void *pvIndex);


/*---------------------------------------------------------------------------*/


/* 
** =================================
** ===  Searching SPI Functions  ===
** =================================
*/


/*

    Function:   iSpiSearchIndex()

    Purpose:    This function searches the index. 

    Parameters: pssSpiSession               spi session structure
                ppvIndexList                pointer to a NULL terminated list of index structures
                pucLanguageCode             language code (optional)
                pucSearchText               search text (optional)
                pucPositiveFeedbackText     positive feedback text (optional)
                pucNegativeFeedbackText     negative feedback text (optional)
                uiStartIndex                start index
                uiEndIndex                  end index, 0 if there is no end index
                ppssrSpiSearchResponse      return pointer for the spi search response structure

    Globals:    none

    Returns:    SPI Error Code

*/
int iSpiSearchIndex (struct spiSession *pssSpiSession, void **ppvIndexList,
    unsigned char *pucLanguageCode, unsigned char *pucSearchText, 
    unsigned char *pucPositiveFeedbackText, unsigned char *pucNegativeFeedbackText, 
    unsigned int uiStartIndex, unsigned int uiEndIndex, 
    struct spiSearchResponse **ppssrSpiSearchResponse);


/*---------------------------------------------------------------------------*/


/* 
** =================================
** ===  Retrieval SPI Functions  ===
** =================================
*/


/*

    Function:   iSpiRetrieveDocument()

    Purpose:    This function should return the text/data specified by the document
                key from chunk = 'uiChunkStart' to chunk <= 'uiChunkEnd'.

                The chunk start and end are meaningless if a document chunk type is
                requested.

    Parameters: pssSpiSession       spi session structure
                pvIndex             index structure
                pucDocumentKey      document key
                pucItemName         document item name
                pucMimeType         document mime type
                uiChunkType         chunk type
                uiChunkStart        start of chunk
                uiChunkEnd          end of chunk
                ppvData             return pointer of data returned
                puiDataLength       return pointer for length of data returned

    Globals:    none

    Returns:    SPI Error Code

*/
int iSpiRetrieveDocument (struct spiSession *pssSpiSession, void *pvIndex, 
    unsigned char *pucDocumentKey, unsigned char *pucItemName, unsigned char *pucMimeType, 
    unsigned int uiChunkType, unsigned int uiChunkStart, unsigned int uiChunkEnd, 
    void **ppvData, unsigned int *puiDataLength);


/*---------------------------------------------------------------------------*/


/* 
** ==========================================
** ===  Server Information SPI Functions  ===
** ==========================================
*/


/*

    Function:   iSpiGetServerInfo()

    Purpose:    This function should allocate, populate a single spi server info
                structure. If an error is returned, the return pointer
                will be ignored.

    Parameters: pssSpiSession           spi session structure
                ppssiSpiServerInfo      return pointer for the spi server info structure

    Globals:    none

    Returns:    SPI Error Code

*/
int iSpiGetServerInfo (struct spiSession *pssSpiSession, struct spiServerInfo **ppssiSpiServerInfo);


/*

    Function:   iSpiGetServerIndexInfo()

    Purpose:    This function should allocate, populate and return a array of spi 
                server index info structures. The number of entries in the array 
                should be returned in puiSpiServerIndexInfosLength. If an error is returned, 
                the return pointers will be ignored.

    Parameters: pssSpiSession                   spi session structure
                ppssiiSpiServerIndexInfos       return pointer for an array of spi server index info structures
                puiSpiServerIndexInfosLength    return pointer for the number of entries
                                                in the spi server index info structures array

    Globals:    none

    Returns:    SPI Error Code

*/
int iSpiGetServerIndexInfo (struct spiSession *pssSpiSession, 
        struct spiServerIndexInfo **ppssiiSpiServerIndexInfos, 
        unsigned int *puiSpiServerIndexInfosLength);


/*

    Function:   iSpiGetIndexInfo()

    Purpose:    This function should allocate and return a single spi index info structure
                populated with information pertinent to the index contained in the 
                index structure. If an error is  returned, the return pointer will be ignored.

    Parameters: pssSpiSession       spi session structure
                pvIndex             index structure
                ppsiiSpiIndexInfo   return pointer for the spi index info structure

    Globals:    none

    Returns:    SPI Error Code

*/
int iSpiGetIndexInfo (struct spiSession *pssSpiSession, void *pvIndex, 
        struct spiIndexInfo **ppsiiSpiIndexInfo);


/*

    Function:   iSpiGetIndexFieldInfo()

    Purpose:    This function should allocate and return an array of spi field info
                structures, populated with information pertinent to the index contained 
                in the index structure. The number of entries in the array should be returned 
                in puiSpiFieldInfosLength. If an error is returned, the return pointer
                will be ignored.

                Note that returning SPI_IndexHasNoSearchFields is not strictly
                an error, but the field list will also be ignored.

    Parameters: pssSpiSession               spi session structure
                pvIndex                     index structure
                ppsfiSpiFieldInfos          return pointer for an array of spi field info structures
                puiSpiFieldInfosLength      return pointer for the number of entries
                                            in the spi field info structures array

    Globals:    none

    Returns:    SPI Error Code

*/
int iSpiGetIndexFieldInfo (struct spiSession *pssSpiSession, void *pvIndex, 
        struct spiFieldInfo **ppsfiSpiFieldInfos, unsigned int *puiSpiFieldInfosLength);


/*

    Function:   iSpiGetIndexTermInfo()

    Purpose:    This function should allocate and return an array of spi term info
                structures, populated with information pertinent to the index contained 
                in the index structure. The number of entries in the array should be 
                returned in puiSpiTermInfosLength. If an error is returned, the return pointer
                will be ignored.

                The uiTermMatch specifies which term type we want to match
                on, pucTerm specifies a term to match on and pucFieldName 
                specifies the field name for which we get the terms.

                Note that returning SPI_IndexHasNoTerms is not strictly
                an error, but the term list will be ignored.

    Parameters: pssSpiSession           spi session structure
                pvIndex                 index structure
                uiTermMatch             term match type
                uiTermCase              term case to search for
                pucTerm                 term to match on (optional)
                pucFieldName            field name to match on (optional)
                ppstiSpiTermInfos       return pointer for an array of spi term info structures
                puiSpiTermInfosLength   return pointer for the number of entries
                                        in the spi term info structures array

    Globals:    none

    Returns:    SPI Error Code

*/
int iSpiGetIndexTermInfo (struct spiSession *pssSpiSession, void *pvIndex, 
        unsigned int uiTermMatch, unsigned int uiTermCase, unsigned char *pucTerm, 
        unsigned char *pucFieldName, struct spiTermInfo **ppstiSpiTermInfos, 
        unsigned int *puiSpiTermInfosLength);


/*

    Function:   iSpiGetDocumentInfo()

    Purpose:    This function should allocate, populate and return a single spi document info
                structure. If an error is returned, the return pointer
                will be ignored.

    Parameters: pssSpiSession           spi session structure
                pvIndex                 index structure
                pucDocumentKey          document key
                ppsdiSpiDocumentInfo    return pointer for the spi document info structure

    Globals:    none

    Returns:    SPI Error Code

*/
int iSpiGetDocumentInfo (struct spiSession *pssSpiSession, void *pvIndex,
        unsigned char *pucDocumentKey, struct spiDocumentInfo **ppsdiSpiDocumentInfo);


/*

    Function:   iSpiGetIndexName()

    Purpose:    This function should allocate, populate and return the name of the 
                index for a index structure. If an error is returned, the
                return pointer will be ignored.

    Parameters: pssSpiSession   spi session structure
                pvIndex         index structure
                ppucIndexName   return pointer for the index name

    Globals:    none

    Returns:    SPI Error Code

*/
int iSpiGetIndexName (struct spiSession *pssSpiSession, void *pvIndex, 
        unsigned char **ppucIndexName);


/*---------------------------------------------------------------------------*/


/* 
** Public function prototypes
*/

int iSpiDuplicateSession (struct spiSession *pssSpiSession, struct spiSession **ppssSpiSession);

int iSpiDuplicateServerInfo (struct spiServerInfo *pssiSpiServerInfo, struct spiServerInfo **ppssiSpiServerInfo);

int iSpiDuplicateServerIndexInfo (struct spiServerIndexInfo *pssiiSpiServerIndexInfos, 
        unsigned int uiSpiServerIndexInfosLength, struct spiServerIndexInfo **ppssiiSpiServerIndexInfos);

int iSpiDuplicateIndexInfo (struct spiIndexInfo *psiiSpiIndexInfo, struct spiIndexInfo **ppsiiSpiIndexInfo);

int iSpiDuplicateIndexFieldInfo (struct spiFieldInfo *psfiSpiFieldInfos, unsigned int uiSpiFieldInfosLength,
        struct spiFieldInfo **ppsfiSpiFieldInfos);

int iSpiDuplicateTermInfo (struct spiTermInfo *pstiSpiTermInfos, unsigned int uiSpiTermInfosLength,
        struct spiTermInfo **ppstiSpiTermInfos);

int iSpiDuplicateDocumentInfo (struct spiDocumentInfo *psdiSpiDocumentInfo, struct spiDocumentInfo **ppsdiSpiDocumentInfo);


int iSpiFreeSession (struct spiSession *pssSpiSession);

int iSpiFreeServerInfo (struct spiServerInfo *pssiSpiServerInfo);

int iSpiFreeServerIndexInfo (struct spiServerIndexInfo *pssiiSpiServerIndexInfos, unsigned int uiSpiServerIndexInfosLength);

int iSpiFreeIndexInfo (struct spiIndexInfo *psiiSpiIndexInfo);

int iSpiFreeIndexFieldInfo (struct spiFieldInfo *psfiSpiFieldInfos, unsigned int uiSpiFieldInfosLength);

int iSpiFreeTermInfo (struct spiTermInfo *pstiSpiTermInfos, unsigned int uiSpiTermInfosLength);

int iSpiFreeDocumentInfo (struct spiDocumentInfo *psdiSpiDocumentInfo);

int iSpiFreeSearchResultComponents (struct spiSearchResult *pssrSpiSearchResult, unsigned int uiSortType);

int iSpiFreeSearchResult (struct spiSearchResult *pssrSpiSearchResult, unsigned int uiSortType);

int iSpiFreeSearchResults (struct spiSearchResult *pssrSpiSearchResults, unsigned int uiSpiSearchResultsLength, 
        unsigned int uiSortType);

int iSpiFreeSearchResponse (struct spiSearchResponse *pssrSpiSearchResponse);


int iSpiSpliceSearchResults (struct spiSearchResult **ppssrSpiSearchResults, unsigned int *puiSpiSearchResultsLength,
        unsigned int uiSpiSearchResultsStartIndex, unsigned int uiSpiSearchResultsEndIndex, unsigned int uiSortType);

int iSpiSortSearchResults (struct spiSearchResult *pssrSpiSearchResults, unsigned int uiSpiSearchResultsLength, 
        unsigned int uiSortType);

int iSpiGetErrorText (int iError, unsigned char *pucErrorText, unsigned int uiErrorTextLength);

int iSpiPrintSpiSearchResponse (struct spiSearchResponse *pssrSpiSearchResponse);

int iSpiPrintSpiSearchResults (struct spiSearchResult *pssrSpiSearchResults, 
        unsigned int uiSpiSearchResultsLength, unsigned int uiSortType);


/*---------------------------------------------------------------------------*/


/* 
** C++ wrapper
*/

#if defined(__cplusplus)
}
#endif    /* defined(__cplusplus) */


/*---------------------------------------------------------------------------*/


#endif    /* !defined(SPI_H) */


/*---------------------------------------------------------------------------*/
