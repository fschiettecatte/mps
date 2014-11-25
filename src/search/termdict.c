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

    Module:     termdict.c

    Author:     Francois Schiettecatte (FS Consulting LLC.)

    Created:    14 September 1995

    Purpose:    This module contains all the functions which make up the 
                term dictionary management functionality.


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
#define UTL_LOG_CONTEXT                     (unsigned char *)"com.fsconsult.mps.src.search.termdict"


/*---------------------------------------------------------------------------*/


/*
** Feature defines
*/

/* Enable a term to start with a wildcard */
#define SRCH_TERM_DICT_ENABLE_STARTING_WILDCARD


/* Enable the multi wildcard '*' */
#if defined(SRCH_PARSER_ENABLE_MULTI_WILDCARD)
#define SRCH_TERM_DICT_ENABLE_MULTI_WILDCARD
#endif /* defined(SRCH_PARSER_ENABLE_MULTI_WILDCARD) */

/* Enable the single wildcard '?' */
#if defined(SRCH_PARSER_ENABLE_SINGLE_WILDCARD)
#define SRCH_TERM_DICT_ENABLE_SINGLE_WILDCARD
#endif /* defined(SRCH_PARSER_ENABLE_SINGLE_WILDCARD) */

/* Enable the alpha wildcard '@' */
#if defined(SRCH_PARSER_ENABLE_ALPHA_WILDCARD)
#define SRCH_TERM_DICT_ENABLE_ALPHA_WILDCARD
#endif /* defined(SRCH_PARSER_ENABLE_ALPHA_WILDCARD) */

/* Enable the numeric wildcard '%' */
#if defined(SRCH_PARSER_ENABLE_NUMERIC_WILDCARD)
#define SRCH_TERM_DICT_ENABLE_NUMERIC_WILDCARD
#endif /* defined(SRCH_PARSER_ENABLE_NUMERIC_WILDCARD) */


/*---------------------------------------------------------------------------*/


/*
** Defines
*/

#define SRCH_TERM_DICT_TYPO_COUNT_MAX                       (1)


#define SRCH_TERM_DICT_MATCH_TYPE_INVALID                   (0)
#define SRCH_TERM_DICT_MATCH_TYPE_LITERAL                   (1)
#define SRCH_TERM_DICT_MATCH_TYPE_WILDCARD_MULTI            (2)
#define SRCH_TERM_DICT_MATCH_TYPE_WILDCARD_SINGLE           (3)
#define SRCH_TERM_DICT_MATCH_TYPE_WILDCARD_ALPHA            (4)
#define SRCH_TERM_DICT_MATCH_TYPE_WILDCARD_NUMERIC          (5)
#define SRCH_TERM_DICT_MATCH_TYPE_WILDCARD_ALPHA_RANGE      (6)
#define SRCH_TERM_DICT_MATCH_TYPE_WILDCARD_NUMERIC_RANGE    (7)

#define SRCH_TERM_DICT_MATCH_TYPE_VALID(n)                  (((n) >= SRCH_TERM_DICT_MATCH_TYPE_LITERAL) && \
                                                                    ((n) <= SRCH_TERM_DICT_MATCH_TYPE_WILDCARD_NUMERIC_RANGE))


#define SRCH_TERM_DICT_RANGE_MATCH_TERM_INVALID             (0)
#define SRCH_TERM_DICT_RANGE_MATCH_TERM_NUMERIC             (1)
#define SRCH_TERM_DICT_RANGE_MATCH_TERM_NUMERIC_RANGE       (2)
#define SRCH_TERM_DICT_RANGE_MATCH_TERM_ALPHA               (3)
#define SRCH_TERM_DICT_RANGE_MATCH_TERM_ALPHA_RANGE         (4)

#define SRCH_TERM_DICT_RANGE_MATCH_TERM_VALID(n)            (((n) >= SRCH_TERM_DICT_RANGE_MATCH_TERM_NUMERIC) && \
                                                                    ((n) <= SRCH_TERM_DICT_RANGE_MATCH_TERM_ALPHA_RANGE))


#define SRCH_TERM_DICT_CASE_SCAN_INVALID                    (0)
#define SRCH_TERM_DICT_CASE_SCAN_ALL                        (1)
#define SRCH_TERM_DICT_CASE_SCAN_NUMERIC                    (2)
#define SRCH_TERM_DICT_CASE_SCAN_UPPER                      (3)
#define SRCH_TERM_DICT_CASE_SCAN_LOWER                      (4)
#define SRCH_TERM_DICT_CASE_SCAN_HIGH                       (5)

#define SRCH_TERM_DICT_CASE_SCAN_VALID(n)                   (((n) >= SRCH_TERM_DICT_CASE_SCAN_ALL) && \
                                                                    ((n) <= SRCH_TERM_DICT_CASE_SCAN_HIGH))


#define SRCH_TERM_DICT_KEY_FROM_INVALID                     (0)
#define SRCH_TERM_DICT_KEY_FROM_CHARACTER                   (1)
#define SRCH_TERM_DICT_KEY_FROM_ENCODED_TERM                (2)

#define SRCH_TERM_DICT_KEY_FROM_VALID(n)                    (((n) >= SRCH_TERM_DICT_KEY_FROM_CHARACTER) && \
                                                                    ((n) <= SRCH_TERM_DICT_KEY_FROM_ENCODED_TERM))


#define SRCH_TERM_DICT_TERM_INFO_ALLOCATION                 (100)


/* Default names */
#define SRCH_TERM_DICT_SOUNDEX_ID                           LNG_SOUNDEX_STANDARD_ID
#define SRCH_TERM_DICT_METAPHONE_ID                         LNG_METAPHONE_STANDARD_ID
#define SRCH_TERM_DICT_PHONIX_ID                            LNG_PHONIX_STANDARD_ID
#define SRCH_TERM_DICT_TYPO_ID                              LNG_TYPO_STANDARD_ID


/*---------------------------------------------------------------------------*/


/*
** Structures
*/

/* Search term dict match structure */
struct srchTermDictMatch {
    unsigned int    uiMatchType;
    wchar_t         *pwcMatchString;
    unsigned int    uiMatchStringLength;
    boolean         bCaseSensitive;
};


/*---------------------------------------------------------------------------*/


/*
** Macros
*/

#define    SRCH_TERM_DICT_MATCH_ADD(pstdmMacroSrchTermDictMatch, uiMacroSrchTermDictMatchLength, uiMacroMatchType, pwcMacroMatchString, bMacroCaseSensitive) \
    {   \
        struct srchTermDictMatch    *pstdmMacroSrchTermDictMatchPtr = NULL; \
        if ( (pstdmMacroSrchTermDictMatchPtr = (struct srchTermDictMatch *)s_realloc(pstdmMacroSrchTermDictMatch,   \
                (size_t)(sizeof(struct srchTermDictMatch) * (uiMacroSrchTermDictMatchLength + 1)))) == NULL ) {     \
            return (SRCH_MemError); \
        }   \
        pstdmMacroSrchTermDictMatch = pstdmMacroSrchTermDictMatchPtr; \
        pstdmMacroSrchTermDictMatchPtr = pstdmMacroSrchTermDictMatch + uiMacroSrchTermDictMatchLength; \
        pstdmMacroSrchTermDictMatchPtr->uiMatchType = uiMacroMatchType; \
        if ( (pstdmMacroSrchTermDictMatchPtr->pwcMatchString = s_wcsdup(pwcMacroMatchString)) == NULL ) {   \
            return (SRCH_MemError); \
        }   \
        pstdmMacroSrchTermDictMatchPtr->uiMatchStringLength = s_wcslen(pwcMacroMatchString); \
        pstdmMacroSrchTermDictMatchPtr->bCaseSensitive = bMacroCaseSensitive; \
        uiMacroSrchTermDictMatchLength++; \
    }


#define SRCH_TERM_DICT_MATCH_FREE(pstdmMacroSrchTermDictMatch, uiMacroSrchTermDictMatchLength) \
    {   \
        struct srchTermDictMatch *pstdmMacroSrchTermDictMatchPtr = NULL; \
        unsigned int    uiMacroI = 0; \
        if ( pstdmMacroSrchTermDictMatch != NULL ) {    \
            for ( uiMacroI = 0, pstdmMacroSrchTermDictMatchPtr = pstdmMacroSrchTermDictMatch; uiMacroI < uiMacroSrchTermDictMatchLength; uiMacroI++, pstdmMacroSrchTermDictMatchPtr++ ) {   \
                s_free(pstdmMacroSrchTermDictMatchPtr->pwcMatchString); \
            }   \
            s_free(pstdmMacroSrchTermDictMatch); \
        }   \
    }


/*---------------------------------------------------------------------------*/


/*
** Private function prototypes
*/

/* Term dictionary callback functions */
static int iSrchTermDictLookupCallBack (unsigned char *pucKey, void *pvEntryData,
        unsigned int uiEntryLength, va_list ap);

static int iSrchTermDictLookupListCallBack (unsigned char *pucKey, void *pvEntryData,
        unsigned int uiEntryLength, va_list ap);

static int iSrchTermDictLookupRangeCallBack (unsigned char *pucKey, void *pvEntryData,
        unsigned int uiEntryLength, va_list ap);


/* Match structure functions */
static int iSrchTermDictGetSearchTermDictMatchFromTerm (wchar_t *pwcTerm, boolean bCaseSensitive,
        struct srchTermDictMatch **ppstdmSrchTermDictMatch, unsigned int *puiSrchTermDictMatchLength);

static int iSrchTermDictMatchTermToSearchTermDictMatch (wchar_t *pwcTerm,
        struct srchTermDictMatch *pstdmSrchTermDictMatch, unsigned int uiSrchTermDictMatchLength);

static int iSrchTermDictPrintSearchTermDictMatch (struct srchTermDictMatch *pstdmSrchTermDictMatch, 
        unsigned int uiSrchTermDictMatchLength);


/* Regex structure functions */
#if defined(TRE_REGEX_ENABLE)
static int iSrchTermDictGetRegexFromTerm (wchar_t *pwcTerm, regex_t **pprRegex);
static int iSrchTermDictMatchTermToRegex (wchar_t *pwcTerm, regex_t *prRegex);
#else
static int iSrchTermDictGetRegexFromTerm (unsigned char *pucTerm, regex_t **pprRegex);
static int iSrchTermDictMatchTermToRegex (unsigned char *pucTerm, regex_t *prRegex);
#endif    /* TRE_REGEX_ENABLE */


/*---------------------------------------------------------------------------*/


/* 
** =========================
** ===  Term Dictionary  ===
** =========================
*/


/*

    Function:   iSrchTermDictAddTerm()

    Purpose:    This must be called in alphabetical order, and writes the term to
                the dictionary file.

                This function packages up the term information such as the 
                index block ID, the occurrences, the document count and 
                the field ID list into a flat buffer and adds it to the
                work entry.

    Parameters: psiSrchIndex            Search index structure
                pucTerm                 Term
                uiTermType              Term type
                uiTermCount             Number of occurrences of the term
                uiDocumentCount         Number of documents in which this term occurs
                ulIndexBlockID          Index block object ID
                pucFieldIDBitmap        Field ID bitmap (optional)
                uiFieldIDBitmapLength   Field ID bitmap length (optional)

    Globals:    none

    Returns:    SRCH error code

*/
int iSrchTermDictAddTerm
(
    struct srchIndex *psiSrchIndex,
    unsigned char *pucTerm,
    unsigned int uiTermType,
    unsigned int uiTermCount,
    unsigned int uiDocumentCount,
    unsigned long ulIndexBlockID,
    unsigned char *pucFieldIDBitmap,
    unsigned int uiFieldIDBitmapLength
    
)
{

    int             iError = UTL_NoError;
    unsigned char   pucBuffer[SRCH_TERM_LENGTH_MAXIMUM * 2];    /* Should be enough space for info */
    unsigned char   *pucBufferEndPtr = NULL;
    unsigned int    uiBufferLength = 0;


    /* Initialize our string variables - optimization as pucString[n] = {'0'} is very expensive */
    pucBuffer[0] = '\0';


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iSrchTermDictAddTerm, pucTerm: [%s], uiTermType: [%u], uiTermCount: [%u], uiDocumentCount: [%u], ulIndexBlockID: [%lu], ", */
/*             pucTerm, uiTermType, uiTermCount, uiDocumentCount, ulIndexBlockID); */


    /* Check the parameters */
    if ( psiSrchIndex == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'psiSrchIndex' parameter passed to 'iSrchTermDictAddTerm'."); 
        return (SRCH_InvalidIndex);
    }

    if ( bUtlStringsIsStringNULL(pucTerm) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pucTerm' parameter passed to 'iSrchTermDictAddTerm'."); 
        return (SRCH_TermDictInvalidTerm);
    }

    if ( SPI_TERM_TYPE_VALID(uiTermType) == false ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiTermType' parameter passed to 'iSrchTermDictAddTerm'."); 
        return (SRCH_TermDictInvalidTermType);
    }

    if ( uiTermCount < 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiTermCount' parameter passed to 'iSrchTermDictAddTerm'."); 
        return (SRCH_TermDictInvalidTermCount);
    }

    if ( uiDocumentCount < 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiDocumentCount' parameter passed to 'iSrchTermDictAddTerm'."); 
        return (SRCH_TermDictInvalidDocumentCount);
    }

    if ( ulIndexBlockID < 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'ulIndexBlockID' parameter passed to 'iSrchTermDictAddTerm'."); 
        return (SRCH_TermDictInvalidIndexBlockID);
    }

    if ( ((pucFieldIDBitmap == NULL) && (uiFieldIDBitmapLength > 0)) || ((pucFieldIDBitmap != NULL) && (uiFieldIDBitmapLength <= 0)) ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'pucFieldIDBitmap' & 'uiFieldIDBitmapLength' parameters passed to 'iSrchTermDictAddTerm'."); 
        return (SRCH_TermDictInvalidFieldIDBitmap);
    }


    /* First we create a buffer of information which will be passed to the dictionary */
    pucBufferEndPtr = pucBuffer;

    /* Write out the term type (increments the pointer) */
    UTL_NUM_WRITE_COMPRESSED_UINT(uiTermType, pucBufferEndPtr);

    /* Write out the number of occurrences (increments the pointer) */
    UTL_NUM_WRITE_COMPRESSED_UINT(uiTermCount, pucBufferEndPtr);

    /* Write out the number of documents in which this term occurs (increments the pointer) */
    UTL_NUM_WRITE_COMPRESSED_UINT(uiDocumentCount, pucBufferEndPtr);

    /* Write out the index block ID (increments the pointer) */
    UTL_NUM_WRITE_COMPRESSED_ULONG(ulIndexBlockID, pucBufferEndPtr);


    /* Write out the field IDs */
    if ( (pucFieldIDBitmap != NULL) && (uiFieldIDBitmapLength > 0) ) {
        
        unsigned int    uiI = 0;
        
        for ( uiI = 0; uiI < uiFieldIDBitmapLength; uiI++ ) {
            
            /* Check the field ID bitmap - field ID 0 is not a field */
            if ( UTL_BITMAP_IS_BIT_SET_IN_POINTER(pucFieldIDBitmap, uiI) ) {
                UTL_NUM_WRITE_COMPRESSED_UINT(uiI + 1, pucBufferEndPtr);
            }
        }
    }

    /* How long is the buffer? */
    uiBufferLength = pucBufferEndPtr - pucBuffer;


    /* Add the term and the buffer to the dictionary */
    if ( (iError = iUtlDictAddEntry(psiSrchIndex->pvUtlTermDictionary, pucTerm, pucBuffer, uiBufferLength)) != UTL_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to add a term to the term dictionary, term: '%s', index: '%s', utl error: %d.", pucTerm, psiSrchIndex->pucIndexName, iError);
        return (SRCH_TermDictAddFailed);
    }


    return (SRCH_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchTermDictLookup()

    Purpose:    Looks up the term in the term dictionary.

                This function will look up the term in the term dictionary,
                call the callback function to unpack the buffer and 
                populate the return pointers with the unpacked information.

    Parameters: psiSrchIndex            search index structure
                pucTerm                 term to search for
                pucFieldIDBitmap        field ID bitmap to filter against (optional)
                uiFieldIDBitmapLength   field ID bitmap length (optional)
                puiTermType             return pointer for the term type
                puiTermCount            return pointer for the term count
                puiDocumentCount        return pointer for the document count
                pulIndexBlockID         return pointer for the index block ID

    Globals:    none

    Returns:    SRCH error code

*/
int iSrchTermDictLookup
(
    struct srchIndex *psiSrchIndex,
    unsigned char *pucTerm,
    unsigned char *pucFieldIDBitmap,
    unsigned int uiFieldIDBitmapLength,
    unsigned int *puiTermType,
    unsigned int *puiTermCount,
    unsigned int *puiDocumentCount,
    unsigned long *pulIndexBlockID
)
{

    int     iUtlError = UTL_NoError;
    int     iError = SRCH_NoError;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iSrchTermDictLookup [%s]", pucTerm); */


    /* Check the parameters */
    if ( psiSrchIndex == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'psiSrchIndex' parameter passed to 'iSrchTermDictLookup'."); 
        return (SRCH_InvalidIndex);
    }

    if ( bUtlStringsIsStringNULL(pucTerm) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pucTerm' parameter passed to 'iSrchTermDictLookup'."); 
        return (SRCH_TermDictInvalidTerm);
    }

    if ( ((pucFieldIDBitmap == NULL) && (uiFieldIDBitmapLength > 0)) || ((pucFieldIDBitmap != NULL) && (uiFieldIDBitmapLength <= 0)) ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'pucFieldIDBitmap' & 'uiFieldIDBitmapLength' parameters passed to 'iSrchTermDictLookup'."); 
        return (SRCH_TermDictInvalidFieldIDBitmap);
    }

    if ( puiTermType == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'puiTermType' parameter passed to 'iSrchTermDictLookup'."); 
        return (SRCH_ReturnParameterError);
    }

    if ( puiTermCount == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'puiTermCount' parameter passed to 'iSrchTermDictLookup'."); 
        return (SRCH_ReturnParameterError);
    }

    if ( puiDocumentCount == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'puiDocumentCount' parameter passed to 'iSrchTermDictLookup'."); 
        return (SRCH_ReturnParameterError);
    }

    if ( pulIndexBlockID == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pulIndexBlockID' parameter passed to 'iSrchTermDictLookup'."); 
        return (SRCH_ReturnParameterError);
    }


    /* Look up the term, note that we pass iError as a parameter to the call back function */
    iUtlError = iUtlDictProcessEntry(psiSrchIndex->pvUtlTermDictionary, pucTerm, (int (*)())iSrchTermDictLookupCallBack, 
            pucFieldIDBitmap, uiFieldIDBitmapLength, puiTermType, puiTermCount, puiDocumentCount, pulIndexBlockID, &iError);

    /* Handle the error */
    if ( iUtlError == UTL_DictKeyNotFound ) {
        return (SRCH_TermDictTermNotFound);
    }
    else if ( iUtlError != UTL_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to look up a term in the term dictionary, term: '%s', index: '%s', utl error: %d", 
                pucTerm, psiSrchIndex->pucIndexName, iUtlError);
        return (SRCH_TermDictTermLookupFailed);
    }

    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchTermDictLookupList()

    Purpose:    Looks up the term in the dictionary file and return an srchTermDictInfo
                array based on the passed uiTermMatch.

                This function should not be called directly, but via the three
                macros provided in this file's header file.

    Parameters: psiSrchIndex                    search index structure
                uiLanguageID                    search language ID
                pucTerm                         term to search for (optional)
                pucFieldIDBitmap                field ID bitmap to filter against (optional)
                uiFieldIDBitmapLength           field ID bitmap length (optional)
                uiTermMatch                     term match type
                uiRangeID                       range tag for range searches
                ppstdiSrchTermDictInfos         return pointer for the search term dict info structure array
                puiSrchTermDictInfosLength      return pointer for the number of entries
                                                in the array of search term dict info structures

    Globals:    none

    Returns:     SRCH error code

*/
int iSrchTermDictLookupList
(
    struct srchIndex *psiSrchIndex,
    unsigned int uiLanguageID, 
    unsigned char *pucTerm,
    unsigned char *pucFieldIDBitmap,
    unsigned int uiFieldIDBitmapLength,
    unsigned int uiTermMatch,
    unsigned int uiRangeID,
    struct srchTermDictInfo **ppstdiSrchTermDictInfos,
    unsigned int *puiSrchTermDictInfosLength
)
{

    int                         iError = SRCH_NoError;
    int                         iPassedError = SRCH_NoError;
    unsigned char               pucKey[SRCH_TERM_LENGTH_MAXIMUM + 1] = {'\0'};
    wchar_t                     pwcEncodedTerm[SRCH_TERM_LENGTH_MAXIMUM + 1] = {L'\0'};
    unsigned int                uiEncodedTermLength = 0;
    struct srchTermDictMatch    *pstdmSrchTermDictMatch = NULL;
    unsigned int                uiSrchTermDictMatchLength = 0;
    
    regex_t                     *prRegex = NULL;

    struct srchTermDictInfo     *pstdiSrchTermDictInfos = NULL;
    struct srchTermDictInfo     *pstdiSrchTermDictInfosPtr = NULL;
    unsigned int                uiSrchTermDictInfosLength = 0;
    struct srchTermDictInfo     *ptiTermInfoMaster = NULL;
    unsigned int                uiTermInfoMasterLength = 0;

    unsigned int                uiI = 0;

    unsigned int                uiDictCaseScan = SRCH_TERM_DICT_CASE_SCAN_ALL;

    wchar_t                     pwcCharacterListStatic[2] = {L'\0'};
    wchar_t                     *pwcCharacterListAllocated = NULL;
    wchar_t                     *pwcCharacterList = NULL;

    unsigned int                uiDictRangeMatchTerm = SRCH_TERM_DICT_RANGE_MATCH_TERM_ALPHA;
    int                         iTermStartNumber = 0;
    int                         iTermEndNumber = 0;
    wchar_t                     *pwcTermStart = NULL;
    wchar_t                     *pwcTermEnd = NULL;
    
    void                        *pvHandle = NULL;

    wchar_t                     *pwcTerm = NULL;
    wchar_t                     *pwcPtr = NULL;

    /* This always true */
    boolean                     bCaseSensitive = true;

    unsigned int                uiKeyGenerator = SRCH_TERM_DICT_KEY_FROM_INVALID;


    iUtlLogDebug(UTL_LOG_CONTEXT, "iSrchTermDictLookupList [%s][field ID bitmap %sdefined][%u][%u]", 
            pucUtlStringsGetPrintableString(pucTerm), (pucFieldIDBitmap != NULL) ? "" : "not ", uiTermMatch, uiRangeID);


    /* Check the parameters */
    if ( psiSrchIndex == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'psiSrchIndex' parameter passed to 'iSrchTermDictLookupList'."); 
        return (SRCH_InvalidIndex);
    }

    if ( ((pucFieldIDBitmap == NULL) && (uiFieldIDBitmapLength > 0)) || ((pucFieldIDBitmap != NULL) && (uiFieldIDBitmapLength <= 0)) ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'pucFieldIDBitmap' & 'uiFieldIDBitmapLength' parameters passed to 'iSrchTermDictLookupList'."); 
        return (SRCH_TermDictInvalidFieldIDBitmap);
    }

    if ( (SRCH_TERMDICT_TERM_MATCH_VALID(uiTermMatch) == false) && (uiTermMatch != SRCH_TERMDICT_TERM_MATCH_UNKNOWN) ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiTermMatch' parameter passed to 'iSrchTermDictLookupList'."); 
        return (SRCH_TermDictInvalidTermMatch);
    }

    if ( (SRCH_PARSER_RANGE_VALID(uiRangeID) == false) && (uiRangeID != SRCH_PARSER_INVALID_ID) ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiRangeID' parameter passed to 'iSrchTermDictLookupList'."); 
        return (SRCH_TermDictInvalidRangeID);
    }

    if ( ppstdiSrchTermDictInfos == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppstdiSrchTermDictInfos' parameter passed to 'iSrchTermDictLookupList'."); 
        return (SRCH_ReturnParameterError);
    }

    if ( puiSrchTermDictInfosLength == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'puiSrchTermDictInfosLength' parameter passed to 'iSrchTermDictLookupList'."); 
        return (SRCH_ReturnParameterError);
    }

    /* Cant have both a term match and a range match */
    if ( (SRCH_TERMDICT_TERM_MATCH_VALID(uiTermMatch) == true) && (SRCH_PARSER_RANGE_VALID(uiRangeID) == true) ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiTermMatch'/'uiRangeID' parameter combination passed to 'iSrchTermDictLookupList' (i)."); 
        return (SRCH_TermDictTermLookupFailed);
    }

    /* Cant have a term range match without a range match */
    if ( ((uiTermMatch == SRCH_TERMDICT_TERM_MATCH_RANGE) || (uiTermMatch == SRCH_TERMDICT_TERM_MATCH_TERM_RANGE)) && (SRCH_PARSER_RANGE_VALID(uiRangeID) == false) ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiTermMatch'/'uiRangeID' parameter combination passed to 'iSrchTermDictLookupList' (ii)."); 
        return (SRCH_TermDictTermLookupFailed);
    }

    /* Check that there is a term if a term match is requested */
    if ( (SRCH_TERMDICT_TERM_MATCH_VALID(uiTermMatch) == true) && (bUtlStringsIsWideStringNULL(pucTerm) == true) ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pucTerm' parameter passed to 'iSrchTermDictLookupList'."); 
        return (SRCH_TermDictTermLookupFailed);
    }


    /* Convert the term from utf-8 to wide characters - pwcTerm gets allocated */
    if ( bUtlStringsIsStringNULL(pucTerm) == false ) {
        if ( (iError = iLngConvertUtf8ToWideString_d(pucTerm, 0, &pwcTerm)) != LNG_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to convert a term from utf-8 to wide characters, lng error: %d.", iError);
            iError = SRCH_TermDictCharacterSetConvertionFailed;
            goto bailFromiSrchTermDictLookupList;
        }
    }



    /* Create the key and the encoded term
    **
    **  pwcTerm             - the term we are looking for (wchar_t)
    **  pucTerm             - the term we are looking for (utf-8)
    **  pucKey              - what is used to navigate to the first term we need to look at in the dictionary
    **  pwcEncodedTerm      - what is used to store the term code for soundex, phonix and metaphone transforms
    **  pwcCharacterList    - the list of characters we need to scan in the dictionary
    **  bGenerateKey        - flag whether we need to generate the key or not
    */
    switch ( uiTermMatch ) {

        case SRCH_TERMDICT_TERM_MATCH_REGULAR:
        case SRCH_TERMDICT_TERM_MATCH_STOP:
            
            /* Create a dummy character list */
            pwcCharacterListStatic[0] = L' ';
            pwcCharacterListStatic[1] = L'\0';
            pwcCharacterList = pwcCharacterListStatic;

            /* Generate the key from the character */
            uiKeyGenerator = SRCH_TERM_DICT_KEY_FROM_CHARACTER;

            break;


        case SRCH_TERMDICT_TERM_MATCH_WILDCARD:

#if !defined(SRCH_TERM_DICT_ENABLE_STARTING_WILDCARD)

            /* This will never match, but it makes the defines below a little easier to read */        
            if ( pwcTerm[0] == L' ' ) {
                ;
            }
    
#if defined(SRCH_TERM_DICT_ENABLE_MULTI_WILDCARD)
            /* Check for a term which starts with a multi wildcard '*' */
            else if ( pwcTerm[0] == SRCH_PARSER_WILDCARD_MULTI_WCHAR ) {
                iError = SRCH_TermDictTermBadWildCard;
                goto bailFromiSrchTermDictLookupList;
            }
#endif    /* defined(SRCH_TERM_DICT_ENABLE_MULTI_WILDCARD) */
            
#if defined(SRCH_TERM_DICT_ENABLE_SINGLE_WILDCARD)
            /* Check for a term which starts with a single wildcard '?' */
            else if ( pwcTerm[0] == SRCH_PARSER_WILDCARD_SINGLE_WCHAR ) {
                iError = SRCH_TermDictTermBadWildCard;
                goto bailFromiSrchTermDictLookupList;
            }
#endif    /* defined(SRCH_TERM_DICT_ENABLE_SINGLE_WILDCARD) */
            
#if defined(SRCH_TERM_DICT_ENABLE_ALPHA_WILDCARD)
            /* Check for a term which starts with an alpha wildcard '@' */
            else if ( pwcTerm[0] == SRCH_PARSER_WILDCARD_ALPHA_WCHAR ) {
                iError = SRCH_TermDictTermBadWildCard;
                goto bailFromiSrchTermDictLookupList;
            }
#endif    /* defined(SRCH_TERM_DICT_ENABLE_ALPHA_WILDCARD) */
            
#if defined(SRCH_TERM_DICT_ENABLE_NUMERIC_WILDCARD)
            /* Check for a term which starts with a numeric wildcard '%' */
            else if ( pwcTerm[0] == SRCH_PARSER_WILDCARD_NUMERIC_WCHAR ) {
                iError = SRCH_TermDictTermBadWildCard;
                goto bailFromiSrchTermDictLookupList;
            }
#endif    /* defined(SRCH_TERM_DICT_ENABLE_NUMERIC_WILDCARD) */
            
#endif    /* !defined(SRCH_TERM_DICT_ENABLE_STARTING_WILDCARD) */

            /* Parse the term into a match structure */
            if ( (iError = iSrchTermDictGetSearchTermDictMatchFromTerm(pwcTerm, bCaseSensitive, &pstdmSrchTermDictMatch, &uiSrchTermDictMatchLength)) != SRCH_NoError ) {
                goto bailFromiSrchTermDictLookupList;
            }

            /* Create the character list, use a space for the character list if the term starts with a wildcard */
            pwcCharacterListStatic[0] = L' ';


            /* This will never match, but it makes the defines below a little easier to read */        
            if ( pwcTerm[0] == L' ' ) {
                ;
            }

#if defined(SRCH_TERM_DICT_ENABLE_MULTI_WILDCARD)
            else if ( pwcTerm[0] != SRCH_PARSER_WILDCARD_MULTI_WCHAR ) {
                pwcCharacterListStatic[0] = pwcTerm[0];
            }
#endif    /* defined(SRCH_TERM_DICT_ENABLE_MULTI_WILDCARD) */

#if defined(SRCH_TERM_DICT_ENABLE_SINGLE_WILDCARD)
            else if ( pwcTerm[0] != SRCH_PARSER_WILDCARD_SINGLE_WCHAR ) {
                pwcCharacterListStatic[0] = pwcTerm[0];
            }
#endif    /* defined(SRCH_TERM_DICT_ENABLE_SINGLE_WILDCARD) */

#if defined(SRCH_TERM_DICT_ENABLE_ALPHA_WILDCARD)
            else if ( pwcTerm[0] != SRCH_PARSER_WILDCARD_ALPHA_WCHAR ) {
                pwcCharacterListStatic[0] = pwcTerm[0];
            }
#endif    /* defined(SRCH_TERM_DICT_ENABLE_ALPHA_WILDCARD) */

#if defined(SRCH_TERM_DICT_ENABLE_NUMERIC_WILDCARD)
            else if ( pwcTerm[0] != SRCH_PARSER_WILDCARD_NUMERIC_WCHAR ) {
                pwcCharacterListStatic[0] = pwcTerm[0];
            }
#endif    /* defined(SRCH_TERM_DICT_ENABLE_NUMERIC_WILDCARD) */

            pwcCharacterListStatic[1] = L'\0';
            pwcCharacterList = pwcCharacterListStatic;

            /* Create the encoded term from the part of the term that is before the first wildcard
            ** so the term 'comp*ing' will yield 'comp' as the encoded term
            */
            s_wcsnncpy(pwcEncodedTerm, pwcTerm, SRCH_TERM_LENGTH_MAXIMUM + 1);

#if defined(SRCH_TERM_DICT_ENABLE_MULTI_WILDCARD)
            if ( (pwcPtr = s_wcschr(pwcEncodedTerm, SRCH_PARSER_WILDCARD_MULTI_WCHAR)) != NULL ) {
                *pwcPtr = L'\0';
            }
#endif    /* defined(SRCH_TERM_DICT_ENABLE_MULTI_WILDCARD) */

#if defined(SRCH_TERM_DICT_ENABLE_SINGLE_WILDCARD)
            if ( (pwcPtr = s_wcschr(pwcEncodedTerm, SRCH_PARSER_WILDCARD_SINGLE_WCHAR)) != NULL ) {
                *pwcPtr = L'\0';
            }
#endif    /* defined(SRCH_TERM_DICT_ENABLE_SINGLE_WILDCARD) */

#if defined(SRCH_TERM_DICT_ENABLE_ALPHA_WILDCARD)
            if ( (pwcPtr = s_wcschr(pwcEncodedTerm, SRCH_PARSER_WILDCARD_ALPHA_WCHAR)) != NULL ) {
                *pwcPtr = L'\0';
            }
#endif    /* defined(SRCH_TERM_DICT_ENABLE_ALPHA_WILDCARD) */

#if defined(SRCH_TERM_DICT_ENABLE_NUMERIC_WILDCARD)
            if ( (pwcPtr = s_wcschr(pwcEncodedTerm, SRCH_PARSER_WILDCARD_NUMERIC_WCHAR)) != NULL ) {
                *pwcPtr = L'\0';
            }
#endif    /* defined(SRCH_TERM_DICT_ENABLE_NUMERIC_WILDCARD) */

            /* Generate the key from the encoded term */
            uiKeyGenerator = SRCH_TERM_DICT_KEY_FROM_ENCODED_TERM;

/*             iUtlLogDebug(UTL_LOG_CONTEXT, "pwcTerm: '%ls', pwcEncodedTerm: '%ls', pwcCharacterList: '%ls'", pwcTerm, pwcEncodedTerm, pwcCharacterList); */

            break;


        case SRCH_TERMDICT_TERM_MATCH_SOUNDEX:

            /* Create the soundex */
            if ( (iError = iLngSoundexCreateByID(SRCH_TERM_DICT_SOUNDEX_ID, uiLanguageID, &pvHandle)) != LNG_NoError ) {
                iUtlLogError(UTL_LOG_CONTEXT, "Failed to create a soundex, lng error: %d.", iError);
                iError = SRCH_TermDictTermSoundexFailed;
                goto bailFromiSrchTermDictLookupList;
            }
            
            /* Get the soundex key for the term, this what we match on */
            if ( (iError = iLngSoundexGetSoundexKey(pvHandle, pwcTerm, pwcEncodedTerm, SRCH_TERM_LENGTH_MAXIMUM + 1)) != LNG_NoError ) {
                iUtlLogError(UTL_LOG_CONTEXT, "Failed to get a soundex key for a term, lng error: %d.", iError);
                iError = SRCH_TermDictTermSoundexFailed;
                goto bailFromiSrchTermDictLookupList;
            }
            
            /* Get the soundex character list */
            if ( (iError = iLngSoundexGetSoundexCharacterList(pvHandle, pwcTerm, &pwcCharacterListAllocated)) != LNG_NoError ) {
                iUtlLogError(UTL_LOG_CONTEXT, "Failed to get a soundex character list for a term, lng error: %d.", iError);
                iError = SRCH_TermDictTermSoundexFailed;
                goto bailFromiSrchTermDictLookupList;
            }
            pwcCharacterList = pwcCharacterListAllocated;
            
            /* Generate the key from the character */
            uiKeyGenerator = SRCH_TERM_DICT_KEY_FROM_CHARACTER;

/*             iUtlLogDebug(UTL_LOG_CONTEXT, "pwcTerm: '%ls', pwcEncodedTerm: '%ls', pwcCharacterList: '%ls'", pwcTerm, pwcEncodedTerm, pwcCharacterList); */
            
            break;


        case SRCH_TERMDICT_TERM_MATCH_PHONIX:
            
            /* Create the phonix */
            if ( (iError = iLngPhonixCreateByID(SRCH_TERM_DICT_PHONIX_ID, uiLanguageID, &pvHandle)) != LNG_NoError ) {
                iUtlLogError(UTL_LOG_CONTEXT, "Failed to create a phonix, lng error: %d.", iError);
                iError = SRCH_TermDictTermPhonixFailed;
                goto bailFromiSrchTermDictLookupList;
            }

            /* Get the phonix key for the term, this what we match on */
            if ( (iError = iLngPhonixGetPhonixKey(pvHandle, pwcTerm, pwcEncodedTerm, SRCH_TERM_LENGTH_MAXIMUM + 1)) != LNG_NoError ) {
                iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the phonix key for a term, lng error: %d.", iError);
                iError = SRCH_TermDictTermPhonixFailed;
                goto bailFromiSrchTermDictLookupList;
            }
            
            /* Get the phonix character list */
            if ( (iError = iLngPhonixGetPhonixCharacterList(pvHandle, pwcTerm, &pwcCharacterListAllocated)) != LNG_NoError ) {
                iUtlLogError(UTL_LOG_CONTEXT, "Failed to get a phonix character list for a term, lng error: %d.", iError);
                iError = SRCH_TermDictTermPhonixFailed;
                goto bailFromiSrchTermDictLookupList;
            }
            pwcCharacterList = pwcCharacterListAllocated;
            
            /* Generate the key from the character */
            uiKeyGenerator = SRCH_TERM_DICT_KEY_FROM_CHARACTER;
            
/*             iUtlLogDebug(UTL_LOG_CONTEXT, "pwcTerm: '%ls', pwcEncodedTerm: '%ls', pwcCharacterListAllocated: '%ls'", pwcTerm, pwcEncodedTerm, pwcCharacterListAllocated); */

            break;


        case SRCH_TERMDICT_TERM_MATCH_METAPHONE:

            /* Create the metaphone */
            if ( (iError = iLngMetaphoneCreateByID(SRCH_TERM_DICT_METAPHONE_ID, uiLanguageID, &pvHandle)) != LNG_NoError ) {
                iUtlLogError(UTL_LOG_CONTEXT, "Failed to create a metaphone, lng error: %d.", iError);
                iError = SRCH_TermDictTermMetaphoneFailed;
                goto bailFromiSrchTermDictLookupList;
            }
            
            /* Get the metaphone key for the term, this what we match on */
            if ( (iError = iLngMetaphoneGetMetaphoneKey(pvHandle, pwcTerm, pwcEncodedTerm, SRCH_TERM_LENGTH_MAXIMUM + 1)) != LNG_NoError ) {
                iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the metaphone key for a term, lng error: %d.", iError);
                iError = SRCH_TermDictTermMetaphoneFailed;
                goto bailFromiSrchTermDictLookupList;
            }
            
            /* Get the metaphone character list */
            if ( (iError = iLngMetaphoneGetMetaphoneCharacterList(pvHandle, pwcTerm, &pwcCharacterListAllocated)) != LNG_NoError ) {
                iUtlLogError(UTL_LOG_CONTEXT, "Failed to get a metaphone character list for a term, lng error: %d.", iError);
                iError = SRCH_TermDictTermMetaphoneFailed;
                goto bailFromiSrchTermDictLookupList;
            }
            pwcCharacterList = pwcCharacterListAllocated;
            
            /* Generate the key from the character */
            uiKeyGenerator = SRCH_TERM_DICT_KEY_FROM_CHARACTER;
            
/*             iUtlLogDebug(UTL_LOG_CONTEXT, "pwcTerm: '%ls', pwcEncodedTerm: '%ls', pwcCharacterListAllocated: '%ls'", pwcTerm, pwcEncodedTerm, pwcCharacterListAllocated); */

            break;


        case SRCH_TERMDICT_TERM_MATCH_TYPO:

            /* The encoded term is the term itself */
            s_wcsnncpy(pwcEncodedTerm, pwcTerm, SRCH_TERM_LENGTH_MAXIMUM + 1);

            /* Create the typo */
            if ( (iError = iLngTypoCreateByID(SRCH_TERM_DICT_TYPO_ID, uiLanguageID, &pvHandle)) != LNG_NoError ) {
                iUtlLogError(UTL_LOG_CONTEXT, "Failed to create a typo, lng error: %d.", iError);
                iError = SRCH_TermDictTermTypoFailed;
                goto bailFromiSrchTermDictLookupList;
            }

            /* Get the typo character list */
            if ( (iError = iLngTypoGetTypoCharacterList(pvHandle, pwcTerm, &pwcCharacterListAllocated)) != LNG_NoError ) {
                iUtlLogError(UTL_LOG_CONTEXT, "Failed to get a typo character list for a term, lng error: %d.", iError);
                iError = SRCH_TermDictTermTypoFailed;
                goto bailFromiSrchTermDictLookupList;
            }
            pwcCharacterList = pwcCharacterListAllocated;
            
            /* Generate the key from the character */
            uiKeyGenerator = SRCH_TERM_DICT_KEY_FROM_CHARACTER;

/*             iUtlLogDebug(UTL_LOG_CONTEXT, "pwcTerm: '%ls', pwcEncodedTerm: '%ls', pwcCharacterListAllocated: '%ls'", pwcTerm, pwcEncodedTerm, pwcCharacterListAllocated); */

            break;


        case SRCH_TERMDICT_TERM_MATCH_REGEX:

            /* Parse the term into a regex structure */
#if defined(TRE_REGEX_ENABLE)
            if ( (iError = iSrchTermDictGetRegexFromTerm(pwcTerm, &prRegex)) != SRCH_NoError ) {
                iUtlLogError(UTL_LOG_CONTEXT, "Failed to create a regex, srch error: %d.", iError);
                goto bailFromiSrchTermDictLookupList;
            }
#else
            if ( (iError = iSrchTermDictGetRegexFromTerm(pucTerm, &prRegex)) != SRCH_NoError ) {
                iUtlLogError(UTL_LOG_CONTEXT, "Failed to create a regex, srch error: %d.", iError);
                goto bailFromiSrchTermDictLookupList;
            }
#endif    /* TRE_REGEX_ENABLE */

            /* Create the character list */
            pwcCharacterListStatic[0] = pwcTerm[0];
            pwcCharacterListStatic[1] = L'\0';
            pwcCharacterList = pwcCharacterListStatic;

            /* Create the encoded term from the part of the term that is before any 
            ** regex stuff, which we assume is anything other than an alphanumeric
            */
            s_wcsnncpy(pwcEncodedTerm, pwcTerm, SRCH_TERM_LENGTH_MAXIMUM + 1);
            
            for ( pwcPtr = pwcEncodedTerm; *pwcPtr != L'\0'; pwcPtr++ ) {
                if ( iswalnum(*pwcPtr) == 0 ) {
                    *pwcPtr = L'\0';
                    break;
                }
            }

            /* Generate the key from the encoded term */
            uiKeyGenerator = SRCH_TERM_DICT_KEY_FROM_ENCODED_TERM;

/*             iUtlLogDebug(UTL_LOG_CONTEXT, "pwcTerm: '%ls', pwcEncodedTerm: '%ls', pwcCharacterListAllocated: '%ls'", pwcTerm, pwcEncodedTerm, pwcCharacterListAllocated); */

            break;


        case SRCH_TERMDICT_TERM_MATCH_TERM_RANGE:

            /* Extract the range from the term if there is a range,
            ** the range format is     '100-200' or 'aaa-bbb', make sure
            ** we also handle '-100-200'    
            */
            if ( (pwcPtr = s_wcschr(pwcTerm + 1, L'-')) != NULL ) {
                
                /* Extract the start and end terms, making sure we truncate the first term */
                if ( (pwcTermStart = s_wcsdup(pwcTerm)) == NULL ) {
                    iError = SRCH_MemError;
                    goto bailFromiSrchTermDictLookupList;
                }
                pwcTermStart[pwcPtr - pwcTerm] = L'\0';
                if ( (pwcTermEnd = s_wcsdup(pwcPtr + 1)) == NULL ) {
                    iError = SRCH_MemError;
                    goto bailFromiSrchTermDictLookupList;
                }
            }

            ASSERT(((bUtlStringsIsWideStringNULL(pwcTermStart) == true) && (bUtlStringsIsWideStringNULL(pwcTermEnd) == true)) ||
                    ((bUtlStringsIsWideStringNULL(pwcTermStart) == false) && (bUtlStringsIsWideStringNULL(pwcTermEnd) == false)));


            /* See if the term passed is a number, to qualify a number must 
            ** contain digits, dots and dashes (commas fail in s_strtol() )
            */
            for ( pwcPtr = pwcTerm, uiDictRangeMatchTerm = SRCH_TERM_DICT_RANGE_MATCH_TERM_NUMERIC; *pwcPtr != L'\0'; pwcPtr++ ) {
                if ( (iswdigit(*pwcPtr) == 0) && (*pwcPtr != L'.') && (*pwcPtr != L'-') ) {
                    uiDictRangeMatchTerm = SRCH_TERM_DICT_RANGE_MATCH_TERM_ALPHA;
                    break;
                }
            }

/*             iUtlLogDebug(UTL_LOG_CONTEXT, "pwcTerm: '%ls', pwcTermStart: '%ls', pwcTermEnd: '%ls', uiDictRangeMatchTerm: '%s'", pwcTerm, pwcTermStart, pwcTermEnd,  */
/*                     (uiDictRangeMatchTerm == SRCH_TERM_DICT_RANGE_MATCH_TERM_NUMERIC) ? "SRCH_TERM_DICT_RANGE_MATCH_TERM_NUMERIC" : "SRCH_TERM_DICT_RANGE_MATCH_TERM_ALPHA"); */

            /* Fall through - dont forget this below */


        case SRCH_TERMDICT_TERM_MATCH_RANGE:

            /* Force the match type to non-numeric if this is a plain range
            ** as opposed to a term range from which we fall into this from 
            ** above
            */
            if ( uiTermMatch == SRCH_TERMDICT_TERM_MATCH_RANGE ) {
                uiDictRangeMatchTerm = SRCH_TERM_DICT_RANGE_MATCH_TERM_ALPHA;
            }

            /* Now test to see if we extracted a range by seeing if we got a start term 
            ** and an end term, this means that either both start and end terms are set or
            ** neither are set
            */
            if ( uiDictRangeMatchTerm == SRCH_TERM_DICT_RANGE_MATCH_TERM_NUMERIC ) {
                
                /* Extract the start number from the term if neither start or end term are available */
                if ( (bUtlStringsIsWideStringNULL(pwcTermStart) == true) && (bUtlStringsIsWideStringNULL(pwcTermEnd) == true) ) {
                    iTermStartNumber = s_wcstol(pwcTerm, NULL, 10);
                }
                /* Extract the start and end numbers from the start and end term, and promote this to a numeric range */
                else if ( (bUtlStringsIsWideStringNULL(pwcTermStart) == false) && (bUtlStringsIsWideStringNULL(pwcTermEnd) == false) ) {
                    iTermStartNumber = s_wcstol(pwcTermStart, NULL, 10);
                    iTermEndNumber = s_wcstol(pwcTermEnd, NULL, 10);
                    uiDictRangeMatchTerm = SRCH_TERM_DICT_RANGE_MATCH_TERM_NUMERIC_RANGE;
                }
                /* This should not happen */
                else {
                    ASSERT(false);
                }
            }
            /* Non-numeric */
            else if ( uiDictRangeMatchTerm == SRCH_TERM_DICT_RANGE_MATCH_TERM_ALPHA ) {
        
                /* Extract the start term from the term if neither start or end term are available */
                if ( (bUtlStringsIsWideStringNULL(pwcTermStart) == true) && (bUtlStringsIsWideStringNULL(pwcTermEnd) == true) ) {
                    if ( (pwcTermStart = s_wcsdup(pwcTerm)) == NULL ) {
                        iError = SRCH_MemError;
                        goto bailFromiSrchTermDictLookupList;
                    }
                }
                /* We already have the start and end terms, promote this to a non-numeric range */
                else if ( (bUtlStringsIsWideStringNULL(pwcTermStart) == false) && (bUtlStringsIsWideStringNULL(pwcTermEnd) == false) ) {
                    uiDictRangeMatchTerm = SRCH_TERM_DICT_RANGE_MATCH_TERM_ALPHA_RANGE;
                }
                /* This should not happen */
                else {
                    ASSERT(false);
                }
            }

    
            /* Error out if we try a term range range with a range search, ie 'date>range[100-200]' */
            if ( (uiRangeID != SRCH_PARSER_RANGE_EQUAL_ID) && 
                    ((uiDictRangeMatchTerm == SRCH_TERM_DICT_RANGE_MATCH_TERM_NUMERIC_RANGE) || (uiDictRangeMatchTerm == SRCH_TERM_DICT_RANGE_MATCH_TERM_ALPHA_RANGE)) ) {
                iError = SRCH_TermDictTermBadRange;
                goto bailFromiSrchTermDictLookupList;
            }
            
            /* Error out if we try an impossible numeric range, ie 'date=range[300-200]' */
            if ( (uiDictRangeMatchTerm == SRCH_TERM_DICT_RANGE_MATCH_TERM_NUMERIC_RANGE) && (iTermStartNumber > iTermEndNumber) ) {
                iError = SRCH_TermDictTermBadRange;
                goto bailFromiSrchTermDictLookupList;
            }
            
                
            /* Error out if we try an impossible alpha range, ie 'field=range[bbb-aaa]', 'field=range[AAA-bbb]', 'field=range[aaa-BBB]',
            ** 'field=range[Aaa-bbb]', 'field=range[aaa-Bbb]', 
            */
            if ( uiDictRangeMatchTerm == SRCH_TERM_DICT_RANGE_MATCH_TERM_ALPHA_RANGE ) {

                if ( s_wcscmp(pwcTermStart, pwcTermEnd) > 0 ) {
                    iError = SRCH_TermDictTermBadRange;
                    goto bailFromiSrchTermDictLookupList;
                }

                if ( ((bLngCaseIsWideStringAllUpperCase(pwcTermStart) == true) && (bLngCaseIsWideStringAllUpperCase(pwcTermEnd) == false)) ||
                        ((bLngCaseIsWideStringAllUpperCase(pwcTermStart) == false) && (bLngCaseIsWideStringAllUpperCase(pwcTermEnd) == true)) ) {
                    iError = SRCH_TermDictTermBadRange;
                    goto bailFromiSrchTermDictLookupList;
                }
                
                if ( ((bLngCaseIsWideStringAllLowerCase(pwcTermStart) == true) && (bLngCaseIsWideStringAllLowerCase(pwcTermEnd) == false)) ||
                        ((bLngCaseIsWideStringAllLowerCase(pwcTermStart) == false) && (bLngCaseIsWideStringAllLowerCase(pwcTermEnd) == true)) ) {
                    iError = SRCH_TermDictTermBadRange;
                    goto bailFromiSrchTermDictLookupList;
                }
                
                if ( ((bLngCaseDoesWideStringStartWithUpperCase(pwcTermStart) == true) && (bLngCaseDoesWideStringStartWithLowerCase(pwcTermEnd) == true)) ||
                        ((bLngCaseDoesWideStringStartWithUpperCase(pwcTermStart) == true) && (bLngCaseDoesWideStringStartWithLowerCase(pwcTermEnd) == true)) ) {
                    iError = SRCH_TermDictTermBadRange;
                    goto bailFromiSrchTermDictLookupList;
                }
            }
            
    
            /* Alpha */
            if ( (uiDictRangeMatchTerm == SRCH_TERM_DICT_RANGE_MATCH_TERM_ALPHA) || (uiDictRangeMatchTerm == SRCH_TERM_DICT_RANGE_MATCH_TERM_ALPHA_RANGE) ) {
                
                /* Create the character list */
                switch ( uiRangeID ) {
                    
                    case SRCH_PARSER_RANGE_EQUAL_ID:
                        
                        if ( (bUtlStringsIsWideStringNULL(pwcTermStart) == false) && (bUtlStringsIsWideStringNULL(pwcTermEnd) == false) ) {
                            
                            wchar_t     wcCharacter = '\0';

                            /* Allocate space for the character list */
                            if ( (pwcCharacterListAllocated = (wchar_t *)s_malloc((size_t)(((pwcTermEnd[0] - pwcTermStart[0]) + 2) * sizeof(wchar_t)))) == NULL ) {
                                iError = SRCH_MemError;
                                goto bailFromiSrchTermDictLookupList;
                            }
                            pwcCharacterList = pwcCharacterListAllocated;
                            
                            for ( wcCharacter = pwcTermStart[0], uiI = 0; wcCharacter <= pwcTermEnd[0]; wcCharacter++, uiI++ ) {
                                pwcCharacterListAllocated[uiI] = wcCharacter;
                                pwcCharacterListAllocated[uiI + 1] = L'\0';
                            }
                        }
                        else {
                            pwcCharacterListStatic[0] = pwcTermStart[0];
                            pwcCharacterListStatic[1] = L'\0';
                            pwcCharacterList = pwcCharacterListStatic;
                        }
                        break;

                    
                    case SRCH_PARSER_RANGE_GREATER_ID:
                    case SRCH_PARSER_RANGE_GREATER_OR_EQUAL_ID:
                        pwcCharacterListStatic[0] = pwcTermStart[0];
                        pwcCharacterListStatic[1] = L'\0';
                        pwcCharacterList = pwcCharacterListStatic;
                        break;

                    
                    case SRCH_PARSER_RANGE_NOT_EQUAL_ID:
                    case SRCH_PARSER_RANGE_LESS_ID:
                    case SRCH_PARSER_RANGE_LESS_OR_EQUAL_ID:
                        pwcCharacterListStatic[0] = L' ';
                        pwcCharacterListStatic[1] = L'\0';
                        pwcCharacterList = pwcCharacterListStatic;
                        break;
                }
                    
                /* Create the encoded term */
                s_wcsnncpy(pwcEncodedTerm, pwcTermStart, SRCH_TERM_LENGTH_MAXIMUM + 1);

                /* Generate the key from the character */
                uiKeyGenerator = SRCH_TERM_DICT_KEY_FROM_ENCODED_TERM;

/*                 iUtlLogDebug(UTL_LOG_CONTEXT, "alpha - pwcTerm: '%ls', pwcEncodedTerm: '%ls', pwcCharacterListStatic: '%ls'", pwcTerm, pwcEncodedTerm, pwcCharacterListStatic); */
            }
            
            /* Numeric */
            else if ( (uiDictRangeMatchTerm == SRCH_TERM_DICT_RANGE_MATCH_TERM_NUMERIC_RANGE) || (uiDictRangeMatchTerm == SRCH_TERM_DICT_RANGE_MATCH_TERM_NUMERIC) ) {
                pwcCharacterListStatic[0] = L'-';
                pwcCharacterListStatic[1] = L'\0';
                pwcCharacterList = pwcCharacterListStatic;

                /* Generate the key from the character */
                uiKeyGenerator = SRCH_TERM_DICT_KEY_FROM_CHARACTER;

/*                 iUtlLogDebug(UTL_LOG_CONTEXT, "numeric - pwcTerm: '%ls', pwcEncodedTerm: '%ls', pwcCharacterListStatic: '%ls'", pwcTerm, pwcEncodedTerm, pwcCharacterListStatic); */
            }

            break;


        default:

            /* Unknown term match type */
            iError = SRCH_TermDictTermLookupFailed;
            goto bailFromiSrchTermDictLookupList;
    }    



    /* Get the encoded term length */
    uiEncodedTermLength = s_wcslen(pwcEncodedTerm);


    /* Select the dictionary scan method */
    if ( bUtlStringsIsWideStringNULL(pwcTerm) == false ) {
        if ( (*pwcTerm >= L'0') && (*pwcTerm <= L'9') )  {
            uiDictCaseScan = SRCH_TERM_DICT_CASE_SCAN_NUMERIC;
        }
        else if ( (*pwcTerm >= L'A') && (*pwcTerm <= L'Z') )  {
            uiDictCaseScan = SRCH_TERM_DICT_CASE_SCAN_UPPER;
        }
        else if ( (*pwcTerm >= L'a') && (*pwcTerm <= L'z') )  {
            uiDictCaseScan = SRCH_TERM_DICT_CASE_SCAN_LOWER;
        }
        else if ( *pwcTerm > (wchar_t)127 )  {
            uiDictCaseScan = SRCH_TERM_DICT_CASE_SCAN_HIGH;
        }
        else {
            uiDictCaseScan = SRCH_TERM_DICT_CASE_SCAN_ALL;
        }
    }
    else {
        uiDictCaseScan = SRCH_TERM_DICT_CASE_SCAN_ALL;
    }


    /* Set the case of the character list */
    if ( uiDictCaseScan == SRCH_TERM_DICT_CASE_SCAN_UPPER ) {
        pwcLngCaseConvertWideStringToUpperCase(pwcCharacterList);
/*         iUtlLogDebug(UTL_LOG_CONTEXT, "i - pwcCharacterList [%ls]", pwcCharacterList); */
    }
    else if ( uiDictCaseScan == SRCH_TERM_DICT_CASE_SCAN_LOWER ) {
        pwcLngCaseConvertWideStringToLowerCase(pwcCharacterList);
/*         iUtlLogDebug(UTL_LOG_CONTEXT, "ii - pwcCharacterList [%ls]", pwcCharacterList); */
    }
    else if ( uiDictCaseScan == SRCH_TERM_DICT_CASE_SCAN_HIGH ) {
        if ( bLngCaseDoesWideStringStartWithUpperCase(pwcTerm) == true ) {
            pwcLngCaseConvertWideStringToUpperCase(pwcCharacterList);
/*             iUtlLogDebug(UTL_LOG_CONTEXT, "iii - pwcCharacterList [%ls]", pwcCharacterList); */
        }
        else if ( bLngCaseDoesWideStringStartWithLowerCase(pwcTerm) == true ) {
            pwcLngCaseConvertWideStringToLowerCase(pwcCharacterList);
/*             iUtlLogDebug(UTL_LOG_CONTEXT, "iv - pwcCharacterList [%ls]", pwcCharacterList); */
        }
        else {
/*             iUtlLogDebug(UTL_LOG_CONTEXT, "v - pwcCharacterList [%ls]", pwcCharacterList); */
        }
    }
/*     else { */
/*         iUtlLogDebug(UTL_LOG_CONTEXT, "vi - pwcCharacterList [%ls]", pwcCharacterList); */
/*     } */


    /* Set the case of the encoded term if we are generating the key from it */
    if ( uiKeyGenerator == SRCH_TERM_DICT_KEY_FROM_ENCODED_TERM ) {

        if ( uiDictCaseScan == SRCH_TERM_DICT_CASE_SCAN_UPPER ) {
/*             pwcLngCaseConvertWideStringToUpperCase(pwcEncodedTerm); */
/*             iUtlLogDebug(UTL_LOG_CONTEXT, "i - pwcEncodedTerm [%ls]", pwcEncodedTerm); */
        }
        else if ( uiDictCaseScan == SRCH_TERM_DICT_CASE_SCAN_LOWER ) {
            pwcLngCaseConvertWideStringToLowerCase(pwcEncodedTerm);
/*             iUtlLogDebug(UTL_LOG_CONTEXT, "ii - pwcEncodedTerm [%ls]", pwcEncodedTerm); */
        }
        else if ( uiDictCaseScan == SRCH_TERM_DICT_CASE_SCAN_HIGH ) {
            if ( bLngCaseDoesWideStringStartWithUpperCase(pwcTerm) == true ) {
/*                 pwcLngCaseConvertWideStringToUpperCase(pwcEncodedTerm); */
/*                 iUtlLogDebug(UTL_LOG_CONTEXT, "iii - pwcEncodedTerm [%ls]", pwcEncodedTerm); */
            }
            else if ( bLngCaseDoesWideStringStartWithLowerCase(pwcTerm) == true ) {
                pwcLngCaseConvertWideStringToLowerCase(pwcEncodedTerm);
/*                 iUtlLogDebug(UTL_LOG_CONTEXT, "iv - pwcEncodedTerm [%ls]", pwcEncodedTerm); */
            }
/*             else { */
/*                 iUtlLogDebug(UTL_LOG_CONTEXT, "v - pwcEncodedTerm [%ls]", pwcEncodedTerm); */
/*             } */
        }
/*         else { */
/*             iUtlLogDebug(UTL_LOG_CONTEXT, "vi - pwcEncodedTerm [%ls]", pwcEncodedTerm); */
/*         } */
    }


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "uiDictCaseScan: %u, pwcCharacterList: '%ls', pwcEncodedTerm: '%ls'", uiDictCaseScan, pwcCharacterList, pwcEncodedTerm); */

    /* Loop over each character in the list */
    for ( uiI = 0; pwcCharacterList[uiI] != L'\0'; uiI++ ) {

        wchar_t     wcCharacter = pwcCharacterList[uiI];

        
        /* Flip the key generator to character */
        if ( uiI == 1 ) {
            uiKeyGenerator = SRCH_TERM_DICT_KEY_FROM_CHARACTER;
        }


        /* Create the key */
        if ( uiKeyGenerator == SRCH_TERM_DICT_KEY_FROM_CHARACTER ) {

            wchar_t         pwcCharacter[2] = {L'\0'};
            unsigned char   pucCharacter[SRCH_TERM_LENGTH_MAXIMUM + 1] = {'\0'};

            /* Create a one character string from the current character */
            pwcCharacter[0] = wcCharacter;
            pwcCharacter[1] = L'\0';
    
            /* Convert the one character string from wide characters to utf-8 */
            if ( iLngConvertWideStringToUtf8_s(pwcCharacter, 0, pucCharacter, SRCH_TERM_LENGTH_MAXIMUM + 1) != LNG_NoError ) {
                iUtlLogError(UTL_LOG_CONTEXT, "Failed to convert the character from wide characters to utf-8, lng error: %d.", iError);
                iError = SRCH_TermDictCharacterSetConvertionFailed;
                goto bailFromiSrchTermDictLookupList;
            }

            snprintf(pucKey, SRCH_TERM_LENGTH_MAXIMUM + 1, "%c ", pucCharacter[0]);

/*             iUtlLogDebug(UTL_LOG_CONTEXT, "i - pucKey [%s]", pucKey); */
        }
        else if ( uiKeyGenerator == SRCH_TERM_DICT_KEY_FROM_ENCODED_TERM ) {

            if ( iLngConvertWideStringToUtf8_s(pwcEncodedTerm, 0, pucKey, SRCH_TERM_LENGTH_MAXIMUM + 1) != LNG_NoError ) {
                iUtlLogError(UTL_LOG_CONTEXT, "Failed to convert the encoded term from wide characters to utf-8, lng error: %d.", iError);
                iError = SRCH_TermDictCharacterSetConvertionFailed;
                goto bailFromiSrchTermDictLookupList;
            }
            
            s_strnncat(pucKey, " ", SRCH_TERM_LENGTH_MAXIMUM, SRCH_TERM_LENGTH_MAXIMUM + 1);

/*             iUtlLogDebug(UTL_LOG_CONTEXT, "ii - pucKey [%s]", pucKey); */
        }


/*         iUtlLogDebug(UTL_LOG_CONTEXT, "pucKey [%s], wcCharacter [%lc][%d]", pucKey, wcCharacter, (wint_t)wcCharacter); */


        /* Look up the keys list - as a list */
        if ( (uiTermMatch != SRCH_TERMDICT_TERM_MATCH_RANGE) && (uiTermMatch != SRCH_TERMDICT_TERM_MATCH_TERM_RANGE) ) {
            
            if ( (iError = iUtlDictProcessEntryList(psiSrchIndex->pvUtlTermDictionary, pucKey, (int (*)())iSrchTermDictLookupListCallBack, uiTermMatch, 
                    (unsigned int)bCaseSensitive, wcCharacter, pwcEncodedTerm, uiEncodedTermLength, pstdmSrchTermDictMatch, uiSrchTermDictMatchLength,
                    prRegex, pvHandle, pucFieldIDBitmap, uiFieldIDBitmapLength, &pstdiSrchTermDictInfos, &uiSrchTermDictInfosLength, &iPassedError)) != UTL_NoError ) {

                iUtlLogError(UTL_LOG_CONTEXT, "Failed to loop over the term dictionary, index: '%s', utl error: %d.", psiSrchIndex->pucIndexName, iError);

                /* Free the term information structure - the srchTermDictInfo structure is compatible with the spiTermInfo structure */
                struct spiTermInfo *pstiSpiTermInfos = (struct spiTermInfo *)ptiTermInfoMaster;
                iSpiFreeTermInfo(pstiSpiTermInfos, uiTermInfoMasterLength);
                pstiSpiTermInfos = NULL;
                
                iError = SRCH_TermDictTermLookupFailed;
                goto bailFromiSrchTermDictLookupList;
            }
        }
        /* Look up the keys list - as a range */
        else {
            
            if ( (uiDictRangeMatchTerm == SRCH_TERM_DICT_RANGE_MATCH_TERM_NUMERIC) || (uiDictRangeMatchTerm == SRCH_TERM_DICT_RANGE_MATCH_TERM_NUMERIC_RANGE) ) {

                if ( (iError = iUtlDictProcessEntryList(psiSrchIndex->pvUtlTermDictionary, pucKey, (int (*)())iSrchTermDictLookupRangeCallBack, 
                        uiDictRangeMatchTerm, wcCharacter, iTermStartNumber, iTermEndNumber, uiRangeID, uiDictCaseScan, pucFieldIDBitmap, uiFieldIDBitmapLength, 
                        &pstdiSrchTermDictInfos, &uiSrchTermDictInfosLength, &iPassedError)) != UTL_NoError ) {

                    iUtlLogError(UTL_LOG_CONTEXT, "Failed to loop over the term dictionary, index: '%s', utl error: %d.", psiSrchIndex->pucIndexName, iError);

                    /* Free the term information structure - the srchTermDictInfo structure is compatible with the spiTermInfo structure */
                    struct spiTermInfo *pstiSpiTermInfos = (struct spiTermInfo *)ptiTermInfoMaster;
                    iSpiFreeTermInfo(pstiSpiTermInfos, uiTermInfoMasterLength);
                    pstiSpiTermInfos = NULL;

                    iError = SRCH_TermDictTermLookupFailed;
                    goto bailFromiSrchTermDictLookupList;
                }
            }
            else if ( (uiDictRangeMatchTerm == SRCH_TERM_DICT_RANGE_MATCH_TERM_ALPHA) || (uiDictRangeMatchTerm == SRCH_TERM_DICT_RANGE_MATCH_TERM_ALPHA_RANGE) ) {

                if ( (iError = iUtlDictProcessEntryList(psiSrchIndex->pvUtlTermDictionary, pucKey, (int (*)())iSrchTermDictLookupRangeCallBack, 
                        uiDictRangeMatchTerm, wcCharacter, pwcTermStart, pwcTermEnd, uiRangeID, uiDictCaseScan, pucFieldIDBitmap, uiFieldIDBitmapLength, 
                        &pstdiSrchTermDictInfos, &uiSrchTermDictInfosLength, &iPassedError)) != UTL_NoError ) {

                    iUtlLogError(UTL_LOG_CONTEXT, "Failed to loop over the term dictionary, index: '%s', utl error: %d.", psiSrchIndex->pucIndexName, iError);

                    /* Free the term information structure - the srchTermDictInfo structure is compatible with the spiTermInfo structure */
                    struct spiTermInfo *pstiSpiTermInfos = (struct spiTermInfo *)ptiTermInfoMaster;
                    iSpiFreeTermInfo(pstiSpiTermInfos, uiTermInfoMasterLength);
                    pstiSpiTermInfos = NULL;

                    iError = SRCH_TermDictTermLookupFailed;
                    goto bailFromiSrchTermDictLookupList;
                }
            }
        }


        /* Append any results we got back to the end of the master pointer */
        if ( (pstdiSrchTermDictInfos != NULL) && (uiSrchTermDictInfosLength > 0) ) {

            /* Only reallocate the master pointer if we need to otherwise we just hand over the pointers */
            if ( (ptiTermInfoMaster != NULL) && (uiTermInfoMasterLength > 0) ) {

                /* Reallocate */
                if ( (pstdiSrchTermDictInfosPtr = (struct srchTermDictInfo *)s_realloc(ptiTermInfoMaster, (size_t)((uiTermInfoMasterLength + uiSrchTermDictInfosLength) * 
                        sizeof(struct srchTermDictInfo)))) == NULL ) {

                    /* Free the term information structure - the srchTermDictInfo structure is compatible with the spiTermInfo structure */
                    struct spiTermInfo *pstiSpiTermInfos = NULL;
                    
                    pstiSpiTermInfos = (struct spiTermInfo *)ptiTermInfoMaster;
                    iSpiFreeTermInfo(pstiSpiTermInfos, uiTermInfoMasterLength);
                    pstiSpiTermInfos = NULL;
                    
                    pstiSpiTermInfos = (struct spiTermInfo *)pstiSpiTermInfos;
                    iSpiFreeTermInfo(pstiSpiTermInfos, uiSrchTermDictInfosLength);
                    pstiSpiTermInfos = NULL;

                    iError = SRCH_MemError;
                    goto bailFromiSrchTermDictLookupList;
                }

                /* Hand over the new master pointer and transfer the new term info pointer content to it */
                ptiTermInfoMaster = pstdiSrchTermDictInfosPtr;
                pstdiSrchTermDictInfosPtr = (ptiTermInfoMaster + uiTermInfoMasterLength);
                uiTermInfoMasterLength += uiSrchTermDictInfosLength;
                s_memcpy(pstdiSrchTermDictInfosPtr, pstdiSrchTermDictInfos, (sizeof(struct srchTermDictInfo) * uiSrchTermDictInfosLength));
                s_free(pstdiSrchTermDictInfos);
            }
            else {
                /* Hand over the pointers */
                ptiTermInfoMaster = pstdiSrchTermDictInfos;
                uiTermInfoMasterLength = uiSrchTermDictInfosLength;
            }
        }

        /* Clean up these pointers */
        pstdiSrchTermDictInfos = NULL;
        uiSrchTermDictInfosLength = 0;
    }



    
    /* Bail label */
    bailFromiSrchTermDictLookupList:


    s_free(pwcCharacterListAllocated)
    s_free(pwcTerm)

    /* Free the regex structure */
    if ( prRegex != NULL ) {
        s_regfree(prRegex);
        s_free(prRegex);
    }

    /* Free the match structure */
    SRCH_TERM_DICT_MATCH_FREE(pstdmSrchTermDictMatch, uiSrchTermDictMatchLength);

    /* Free the term start and term end */
    s_free(pwcTermStart)
    s_free(pwcTermEnd)

    /* Free the handle */
    switch ( uiTermMatch ) {

        case SRCH_TERMDICT_TERM_MATCH_SOUNDEX:
            iLngSoundexFree(pvHandle);
            pvHandle = NULL;
            break;
            
        case SRCH_TERMDICT_TERM_MATCH_METAPHONE:
            iLngMetaphoneFree(pvHandle);
            pvHandle = NULL;
            break;
            
        case SRCH_TERMDICT_TERM_MATCH_PHONIX:
            iLngPhonixFree(pvHandle);
            pvHandle = NULL;
            break;

        case SRCH_TERMDICT_TERM_MATCH_TYPO:
            iLngTypoFree(pvHandle);
            pvHandle = NULL;
            break;
    }
    

    /* Handle the error */
    if ( iError == SRCH_NoError ) {

        ASSERT(((ptiTermInfoMaster != NULL) && (uiTermInfoMasterLength > 0)) || ((ptiTermInfoMaster == NULL) && (uiTermInfoMasterLength == 0)))

        /* Set the return pointer if we got terms, otherwise we override the error  */
        if ( (ptiTermInfoMaster != NULL) && (uiTermInfoMasterLength > 0) ) {

            /* Set the return pointers */
            *ppstdiSrchTermDictInfos = ptiTermInfoMaster;
            *puiSrchTermDictInfosLength = uiTermInfoMasterLength;
        }
        else {
            
            /* Override the error */
            iError = SRCH_TermDictTermDoesNotOccur;
        }
    }


    /* Return that we have no terms if there are none */
    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchTermDictFreeSearchTermDictInfo()

    Purpose:    Free the dict info structure.

    Parameters: ppstdiSrchTermDictInfos         search term dict info structure array
                puiSrchTermDictInfosLength      number of entries in the array of search term dict info structures

    Globals:    none

    Returns:     SRCH error code

*/
int iSrchTermDictFreeSearchTermDictInfo
(
    struct srchTermDictInfo *pstdiSrchTermDictInfos,
    unsigned int uiSrchTermDictInfosLength
)
{


    struct srchTermDictInfo     *pstdiSrchTermDictInfosPtr = NULL;
    unsigned int                uiI = 0;
    

    /* Free the term dict info structure */
    if ( pstdiSrchTermDictInfos != NULL ) {
        for ( pstdiSrchTermDictInfosPtr = pstdiSrchTermDictInfos, uiI = 0; uiI < uiSrchTermDictInfosLength; pstdiSrchTermDictInfosPtr++, uiI++ ) {
            s_free(pstdiSrchTermDictInfosPtr->pucTerm);
        }
        s_free(pstdiSrchTermDictInfos);
    }


    return (SRCH_NoError);

}


/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchTermDictLookupCallBack()

    Purpose:    This function is passed to the dictionary lookup function to
                perform extra processing on a term.

                If there is a hit, the data will be unpacked and the 
                return pointers will be populated.

    Parameters: pucKey          key (term)
                pvEntryData     entry data
                uiEntryLength   entry length
                ap              args (optional)

    Globals:    none

    Returns:    0 to continue processing, non-0 otherwise

*/
static int iSrchTermDictLookupCallBack
(
    unsigned char *pucKey,
    void *pvEntryData,
    unsigned int uiEntryLength,
    va_list ap
)
{

    va_list         ap_;
    unsigned char   *pucEntryDataPtr = NULL;
    unsigned char   *pucEntryDataEndPtr = NULL;
    unsigned char   *pucFieldIDBitmap = NULL;
    unsigned int    uiFieldIDBitmapLength = 0;
    unsigned int    *puiTermType = NULL;
    unsigned int    *puiTermCount = NULL;
    unsigned int    *puiDocumentCount = NULL;
    unsigned long   *pulIndexBlockID = NULL;
    int             *piError = NULL;
    unsigned int    uiTermType = 0;
    unsigned int    uiTermCount = 0;
    unsigned int    uiDocumentCount = 0;
    unsigned long   ulIndexBlockID = 0;
    unsigned int    uiFieldID = 0;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iSrchTermDictLookupCallBack [%s][%u][%s]", pucKey, uiEntryLength, */
/*             (pvEntryData != NULL) ? "(pvEntryData != NULL)" : "(pvEntryData == NULL)" ); */


    ASSERT(bUtlStringsIsStringNULL(pucKey) == false);
    ASSERT(pvEntryData != NULL);
    ASSERT(uiEntryLength > 0);


    /* Get all our parameters, note that we make a copy of 'ap' */
    va_copy(ap_, ap);
    pucFieldIDBitmap = (unsigned char *)va_arg(ap_, unsigned char *);
    uiFieldIDBitmapLength = (unsigned int)va_arg(ap_, unsigned int);
    puiTermType = (unsigned int *)va_arg(ap_, unsigned int *);
    puiTermCount = (unsigned int *)va_arg(ap_, unsigned int *);
    puiDocumentCount = (unsigned int *)va_arg(ap_, unsigned int *);
    pulIndexBlockID = (unsigned long *)va_arg(ap_, unsigned long *);
    piError = (int *)va_arg(ap_, int *);
    va_end(ap_);


    ASSERT(((pucFieldIDBitmap == NULL) && (uiFieldIDBitmapLength <= 0)) || ((pucFieldIDBitmap != NULL) && (uiFieldIDBitmapLength > 0)));
    ASSERT(puiTermType != NULL);
    ASSERT(puiTermCount != NULL);
    ASSERT(puiDocumentCount != NULL);
    ASSERT(pulIndexBlockID != NULL);
    ASSERT(piError != NULL);


    /* Extract some information from the data pointer, decode to a variable and copy to the pointer for optimization */ 
    pucEntryDataPtr = (unsigned char *)pvEntryData;
    UTL_NUM_READ_COMPRESSED_UINT(uiTermType, pucEntryDataPtr);
    UTL_NUM_READ_COMPRESSED_UINT(uiTermCount, pucEntryDataPtr);
    UTL_NUM_READ_COMPRESSED_UINT(uiDocumentCount, pucEntryDataPtr);
    UTL_NUM_READ_COMPRESSED_ULONG(ulIndexBlockID, pucEntryDataPtr);

    *puiTermType = uiTermType;
    *puiTermCount = uiTermCount;
    *puiDocumentCount = uiDocumentCount;
    *pulIndexBlockID = ulIndexBlockID;


    /* This is not a fielded lookup, so we can just return */
    if ( pucFieldIDBitmap == NULL ) {
        
        /* Set the returned error, the term exists and occurs */
        *piError = SRCH_NoError;
                
/*         iUtlLogDebug(UTL_LOG_CONTEXT, "pucKey [%s], *puiTermType: [%u], puiTermCount: [%u], *puiDocumentCount: [%u], *pulIndexBlockID: [%lu]",  */
/*                 pucKey, *puiTermType, *puiTermCount, *puiDocumentCount, *pulIndexBlockID); */
        
        return (0);
    }


    /* We need to check that this term occurs in the field specified by the passed field ID */
    pucEntryDataEndPtr = (unsigned char *)pvEntryData + uiEntryLength;

    /* Scan the field IDs */
    while ( pucEntryDataPtr < pucEntryDataEndPtr ) {

        /* Decode the field ID */
        UTL_NUM_READ_COMPRESSED_UINT(uiFieldID, pucEntryDataPtr);

        ASSERT(uiFieldID <= uiFieldIDBitmapLength); 

        /* Check for a match - field ID 0 is not a field */
        if ( UTL_BITMAP_IS_BIT_SET_IN_POINTER(pucFieldIDBitmap, uiFieldID - 1) ) {
            
            /* Set the returned error, the term exists and occurs in the specified field */
            *piError = SRCH_NoError;
            
/*             iUtlLogDebug(UTL_LOG_CONTEXT, "pucKey [%s], *puiTermType: [%u], puiTermCount: [%u], *puiDocumentCount: [%u], *pulIndexBlockID: [%lu], uiFieldID: [%u]",  */
/*                     pucKey, *puiTermType, *puiTermCount, *puiDocumentCount, *pulIndexBlockID, uiFieldID); */

            return (0);
        }
    }


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "Dropped pucKey [%s]", pucKey); */


    /* Dropped through from the field check without finding the term in the specific field,
    ** so we set the term count and doc count to 0, but we leave the work type alone 
    */
    *puiTermCount = 0;    
    *puiDocumentCount = 0;    
    *pulIndexBlockID = 0;    
    

    /* Set the returned error, the term exists but does not occur */
    *piError = SRCH_TermDictTermDoesNotOccur;


    return (0);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchTermDictLookupListCallBack()

    Purpose:    This function is passed to the dictionary lookup function to
                perform extra processing on a term.

    Parameters: pucKey          key (term)
                pvEntryData     entry data
                uiEntryLength   entry length
                ap              arg list

    Globals:    none

    Returns:    0 to continue processing, non-0 otherwise

*/
static int iSrchTermDictLookupListCallBack
(
    unsigned char *pucKey,
    void *pvEntryData,
    unsigned int uiEntryLength,
    va_list ap
)
{

    va_list                     ap_;
    int                         iError = SRCH_NoError;
    int                         iStatus = 0;
    boolean                     bFinished = false;

    struct srchTermDictInfo     *pstdiSrchTermDictInfos = NULL;
    unsigned int                uiSrchTermDictInfosLength = 0;
    unsigned int                uiTermMatch = SRCH_TERMDICT_TERM_MATCH_UNKNOWN;
    boolean                     bCaseSensitive = false;
    wchar_t                     wcCharacter = L'\0';
    wchar_t                     *pwcEncodedTerm = NULL;
    unsigned int                uiEncodedTermLength = 0;
    struct srchTermDictMatch    *pstdmSrchTermDictMatch = NULL;
    unsigned int                uiSrchTermDictMatchLength = 0;
    regex_t                     *prRegex = NULL;
    void                        *pvHandle = NULL;
    unsigned char               *pucFieldIDBitmap = NULL;
    unsigned int                uiFieldIDBitmapLength = 0;
    struct srchTermDictInfo     **ppstdiSrchTermDictInfos = NULL;
    unsigned int                *puiSrchTermDictInfosLength = NULL;
    unsigned int                *piError = NULL;
    boolean                     bFieldMatch = false;
    boolean                     bTermMatch = false;

    unsigned char               *pucEntryDataPtr = NULL;
    unsigned char               *pucEntryDataEndPtr = NULL;
    unsigned int                uiTermType = 0;
    unsigned int                uiTermCount = 0;
    unsigned int                uiDocumentCount = 0;
    unsigned long               ulIndexBlockID = 0;
    unsigned int                uiFieldID = 0;

    /* This used as a return parameter for calls to soundex, phonix and metaphone */
    wchar_t                     pwcEncodedKey[SRCH_TERM_LENGTH_MAXIMUM + 1] = {L'\0'};
    wchar_t                     pwcKey[SRCH_TERM_LENGTH_MAXIMUM + 1] = {L'\0'};



/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iSrchTermDictLookupListCallBack [%s][%u][%s]", pucKey, uiEntryLength, */
/*             (pvEntryData != NULL) ? "(pvEntryData != NULL)" : "(pvEntryData == NULL)"); */

    ASSERT(bUtlStringsIsStringNULL(pucKey) == false);
    ASSERT(pvEntryData != NULL);
    ASSERT(uiEntryLength > 0);


    /* Get all our parameters, note that we make a copy of 'ap' */
    va_copy(ap_, ap);
    uiTermMatch = (unsigned int)va_arg(ap_, unsigned int);
    bCaseSensitive = (boolean)va_arg(ap_, unsigned int);
    wcCharacter = (wchar_t)va_arg(ap_, wchar_t);
    pwcEncodedTerm = (wchar_t *)va_arg(ap_, wchar_t *);
    uiEncodedTermLength = (unsigned int)va_arg(ap_, unsigned int);
    pstdmSrchTermDictMatch = (struct srchTermDictMatch *)va_arg(ap_, struct srchTermDictMatch *);
    uiSrchTermDictMatchLength = (unsigned int)va_arg(ap_, unsigned int);
    prRegex = (regex_t *)va_arg(ap_, regex_t *);
    pvHandle = (void *)va_arg(ap_, void *);
    pucFieldIDBitmap = (unsigned char *)va_arg(ap_, unsigned char *);
    uiFieldIDBitmapLength = (unsigned int)va_arg(ap_, unsigned int);
    ppstdiSrchTermDictInfos = (struct srchTermDictInfo **)va_arg(ap_, struct srchTermDictInfo **);
    puiSrchTermDictInfosLength = (unsigned int *)va_arg(ap_, unsigned int *);
    piError = (int *)va_arg(ap_, int *);
    va_end(ap_);


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iSrchTermDictLookupListCallBack [%u][%s][%lc][%ls][%u][%ls][%u][field ID bitmap %sdefined][%u]", */
/*             uiTermMatch, (bCaseSensitive == true) ? "true" : "false", (wint_t)wcCharacter, pwcEncodedTerm, uiEncodedTermLength, pwcUtlStringsGetPrintableWideString(pwcEncodedTerm),  */
/*             uiSrchTermDictMatchLength, (pucFieldIDBitmap != NULL) ? "" : "not ", *puiSrchTermDictInfosLength); */


    ASSERT(SRCH_TERMDICT_TERM_MATCH_VALID(uiTermMatch) == true);
    ASSERT(wcCharacter != L'\0');
    ASSERT((bCaseSensitive == true) || (bCaseSensitive == false));
    ASSERT(((bUtlStringsIsWideStringNULL(pwcEncodedTerm) == false) && (uiEncodedTermLength > 0)) || ((bUtlStringsIsWideStringNULL(pwcEncodedTerm) == true) && (uiEncodedTermLength == 0)));
    ASSERT(((pstdmSrchTermDictMatch != NULL) && (uiSrchTermDictMatchLength > 0)) || ((pstdmSrchTermDictMatch == NULL) && (uiSrchTermDictMatchLength == 0)));
    ASSERT(((pvHandle != NULL) && 
                ((uiTermMatch == SRCH_TERMDICT_TERM_MATCH_SOUNDEX) || (uiTermMatch == SRCH_TERMDICT_TERM_MATCH_PHONIX) || (uiTermMatch == SRCH_TERMDICT_TERM_MATCH_METAPHONE) || (uiTermMatch == SRCH_TERMDICT_TERM_MATCH_TYPO))) ||
                ((pvHandle == NULL) && 
                ((uiTermMatch == SRCH_TERMDICT_TERM_MATCH_REGULAR) || (uiTermMatch == SRCH_TERMDICT_TERM_MATCH_STOP) || (uiTermMatch == SRCH_TERMDICT_TERM_MATCH_WILDCARD) || (uiTermMatch == SRCH_TERMDICT_TERM_MATCH_REGEX))));
    ASSERT(((prRegex != NULL) && (uiTermMatch == SRCH_TERMDICT_TERM_MATCH_REGEX)) || ((prRegex == NULL) && (uiTermMatch != SRCH_TERMDICT_TERM_MATCH_REGEX)));
    ASSERT(((pucFieldIDBitmap == NULL) && (uiFieldIDBitmapLength <= 0)) || ((pucFieldIDBitmap != NULL) && (uiFieldIDBitmapLength > 0)));
    ASSERT(ppstdiSrchTermDictInfos != NULL);
    ASSERT(puiSrchTermDictInfosLength != NULL);


    /* Extract some information from the data pointer */ 
    pucEntryDataPtr = (unsigned char *)pvEntryData;
    UTL_NUM_READ_COMPRESSED_UINT(uiTermType, pucEntryDataPtr);
    UTL_NUM_READ_COMPRESSED_UINT(uiTermCount, pucEntryDataPtr);
    UTL_NUM_READ_COMPRESSED_UINT(uiDocumentCount, pucEntryDataPtr);
    UTL_NUM_READ_COMPRESSED_ULONG(ulIndexBlockID, pucEntryDataPtr);


    /* Assume that we are not going to add this term */
    bFieldMatch = false;

    /* Do we need to prequalify this term against the field ID */
    if ( pucFieldIDBitmap != NULL ) {

        pucEntryDataEndPtr = (unsigned char *)pvEntryData + uiEntryLength;

        /* Scan the field IDs */
        while ( pucEntryDataPtr < pucEntryDataEndPtr ) {
        
            /* Decode */
            UTL_NUM_READ_COMPRESSED_UINT(uiFieldID, pucEntryDataPtr);

            ASSERT(uiFieldID <= uiFieldIDBitmapLength); 

            /* Check for a match - field ID 0 is not a field */
            if ( UTL_BITMAP_IS_BIT_SET_IN_POINTER(pucFieldIDBitmap, uiFieldID - 1) ) {
                /* We have a match so we break */
                bFieldMatch = true;
                break;
            }
        }
    }
    else {
        bFieldMatch = true;
    }

    /* Bail here if there is no field match */ 
    if ( bFieldMatch == false ) {
        goto bailFromiSrchTermDictLookupListCallBack;
    }
    
    

    /* Convert the key from utf-8 to wide characters */
    if ( (iError = iLngConvertUtf8ToWideString_s(pucKey, 0, pwcKey, SRCH_TERM_LENGTH_MAXIMUM + 1)) != LNG_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to convert the key from utf-8 to wide characters, lng error: %d.", iError);
        iStatus = -1;
        goto bailFromiSrchTermDictLookupListCallBack;
    }


    /* Assume that we are not going to add this term */
    bTermMatch = false;

    /* And check it, adding it to the list if it matches */
    switch ( uiTermMatch ) {

        case SRCH_TERMDICT_TERM_MATCH_REGULAR:
            
            /* Include all regular terms */
            if ( uiTermType == SPI_TERM_TYPE_REGULAR ) {
                bTermMatch = true;
            }
            break;


        case SRCH_TERMDICT_TERM_MATCH_STOP:
            
            /* Include all stop terms */
            if ( uiTermType == SPI_TERM_TYPE_STOP ) {
                bTermMatch = true;
            }
            break;


        case SRCH_TERMDICT_TERM_MATCH_WILDCARD:
        
            /* Match all terms if the character is a space, otherwise
            ** match terms where the first letter of the key matches the character,
            ** bail as soon as we exit the character range 
            */
            if ( wcCharacter == L' ' ) {

/*                 iUtlLogDebug(UTL_LOG_CONTEXT, "pwcKey: '%ls'", pwcKey); */

                if ( iSrchTermDictMatchTermToSearchTermDictMatch(pwcKey, pstdmSrchTermDictMatch, uiSrchTermDictMatchLength) == 0 ) {
                    bTermMatch = true;
                }
            }
            else {
                if ( pwcKey[0] > wcCharacter ) {
                    bFinished = true;
                }
                else if ( pwcKey[0] == wcCharacter ) {
                    
                    int iLocalStatus = s_wcsncmp(pwcEncodedTerm, pwcKey, uiEncodedTermLength);
                    
                    if ( iLocalStatus < 0 ) {
                        bFinished = true;
                    }
                    else if ( iLocalStatus == 0 ) {
                        
/*                         iUtlLogDebug(UTL_LOG_CONTEXT, "pwcKey: '%ls'", pwcKey); */

                        if ( iSrchTermDictMatchTermToSearchTermDictMatch(pwcKey, pstdmSrchTermDictMatch, uiSrchTermDictMatchLength) == 0 ) {
                            bTermMatch = true;
                        }
                    }
                }
            }
            break;


        case SRCH_TERMDICT_TERM_MATCH_SOUNDEX:
            
            /* Match terms while the first letter of the key matches the character,
            ** bail as soon as we exit the character range
            */
            if ( pwcKey[0] > wcCharacter  ) {
                bFinished = true;
            }
            else if ( pwcKey[0] == wcCharacter ) {

                /* Soundex the key and compare */
                if ( (iError = iLngSoundexGetSoundexKey(pvHandle, pwcKey, pwcEncodedKey, SRCH_TERM_LENGTH_MAXIMUM + 1)) != LNG_NoError ) {
                    iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the soundex key for a term, lng error: %d.", iError);
                    iStatus = -1;
                    goto bailFromiSrchTermDictLookupListCallBack;
                }

/*                 iUtlLogDebug(UTL_LOG_CONTEXT, "pwcKey: '%ls', pwcEncodedTerm: '%ls', pwcEncodedKey: '%ls'", pwcKey, pwcEncodedTerm, pwcEncodedKey); */

                if ( s_wcscmp(pwcEncodedTerm, pwcEncodedKey) == 0 ) {
                    bTermMatch = true;
                }
            }
            break;


        case SRCH_TERMDICT_TERM_MATCH_PHONIX:

            /* Match terms while the first letter of the key matches the character,
            ** bail as soon as we exit the character range
            */
            if ( pwcKey[0] > wcCharacter  ) {
                bFinished = true;
            }
            else if ( pwcKey[0] == wcCharacter ) {

                /* Phonix the key and compare */
                if ( (iError = iLngPhonixGetPhonixKey(pvHandle, pwcKey, pwcEncodedKey, SRCH_TERM_LENGTH_MAXIMUM + 1)) != LNG_NoError ) {
                    iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the phonix key for a term, lng error: %d.", iError);
                    iStatus = -1;
                    goto bailFromiSrchTermDictLookupListCallBack;
                }

/*                 iUtlLogDebug(UTL_LOG_CONTEXT, "pwcKey: '%ls', pwcEncodedTerm: '%ls', pwcEncodedKey: '%ls'", pwcKey, pwcEncodedTerm, pwcEncodedKey); */

                if ( s_wcscmp(pwcEncodedTerm, pwcEncodedKey) == 0 ) {
                    bTermMatch = true;
                }
            }
            break;


        case SRCH_TERMDICT_TERM_MATCH_METAPHONE:

            /* Match terms while the first letter of the key matches the character,
            ** bail as soon as we exit the character range
            */
            if ( pwcKey[0] > wcCharacter  ) {
                bFinished = true;
            }
            else if ( pwcKey[0] == wcCharacter ) {

                /* Metaphone the key and compare */
                if ( (iError = iLngMetaphoneGetMetaphoneKey(pvHandle, pwcKey, pwcEncodedKey, UTL_FILE_PATH_MAX + 1)) != LNG_NoError ) {
                    iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the metaphone key for a term, lng error: %d.", iError);
                    iStatus = -1;
                    goto bailFromiSrchTermDictLookupListCallBack;
                }

/*                 iUtlLogDebug(UTL_LOG_CONTEXT, "pwcKey: '%ls', pwcEncodedTerm: '%ls', pwcEncodedKey: '%ls'", pwcKey, pwcEncodedTerm, pwcEncodedKey); */

                if ( s_wcscmp(pwcEncodedTerm, pwcEncodedKey) == 0 ) {
                    bTermMatch = true;
                }
            }
            break;


        case SRCH_TERMDICT_TERM_MATCH_TYPO:
            
            /* Match terms while the first letter of the key matches the character,
            ** bail as soon as we exit the character range
            */
            if ( pwcKey[0] > wcCharacter  ) {
                bFinished = true;
            }
            else if ( pwcKey[0] == wcCharacter ) {

/*                 iUtlLogDebug(UTL_LOG_CONTEXT, "pwcKey: '%ls', pwcEncodedTerm: '%ls'", pwcKey, pwcEncodedTerm); */

                /* Check for typo match */
                if ( (iError = iLngTypoGetTypoMatch(pvHandle, pwcEncodedTerm, pwcKey, bCaseSensitive, SRCH_TERM_DICT_TYPO_COUNT_MAX)) == LNG_NoError ) {
                    bTermMatch = true;
                }
            }
            break;


        case SRCH_TERMDICT_TERM_MATCH_REGEX:
            
            /* Match terms while the first letter of the key matches the character,
            ** bail as soon as we exit the character range
            */
            if ( pwcKey[0] > wcCharacter  ) {
                bFinished = true;
            }
            else if ( pwcKey[0] == wcCharacter ) {
                if ( s_wcsncasecmp(pwcEncodedTerm, pwcKey, uiEncodedTermLength) == 0 ) {
#if defined(TRE_REGEX_ENABLE)
                    if ( iSrchTermDictMatchTermToRegex(pwcKey, prRegex) == 0 ) {
                        bTermMatch = true;
                    }
#else
                    if ( iSrchTermDictMatchTermToRegex(pucKey, prRegex) == 0 ) {
                        bTermMatch = true;
                    }
#endif    /* TRE_REGEX_ENABLE */
                }
            }
            break;


        default:
            return (-1);
    }



    /* Bail label */
    bailFromiSrchTermDictLookupListCallBack:


    /* Handle the status - part i */
    if ( iStatus == 0 ) {

        /* Add the term */
        if ( (bFieldMatch == true) && (bTermMatch == true) ) {
    
            struct srchTermDictInfo     *pstdiSrchTermDictInfosPtr = NULL;
                
/*             iUtlLogDebug(UTL_LOG_CONTEXT, "pucKey [%s], uiTermCount: [%u], uiDocumentCount: [%u]",  pucKey, uiTermCount, uiDocumentCount); */
    
            /* Derefence the term list and the term list length */
            pstdiSrchTermDictInfos = *ppstdiSrchTermDictInfos;
            uiSrchTermDictInfosLength = *puiSrchTermDictInfosLength;
    
            /* Allocate in blocks of SRCH_TERM_DICT_TERM_INFO_ALLOCATION */
            if ( (uiSrchTermDictInfosLength % SRCH_TERM_DICT_TERM_INFO_ALLOCATION) == 0 ) {
                
                /* Note that we loop back to the bail label here, this is intentional and taken care of by iStatus */
                if ( (pstdiSrchTermDictInfosPtr = (struct srchTermDictInfo *)s_realloc(pstdiSrchTermDictInfos, 
                        (size_t)((uiSrchTermDictInfosLength + SRCH_TERM_DICT_TERM_INFO_ALLOCATION) * sizeof(struct srchTermDictInfo)))) == NULL ) {
                    iStatus = -1;
                    goto bailFromiSrchTermDictLookupListCallBack;
                }
                pstdiSrchTermDictInfos = pstdiSrchTermDictInfosPtr;
            }
    
            /* Dereference the pointer and add the term */
            pstdiSrchTermDictInfosPtr = pstdiSrchTermDictInfos + uiSrchTermDictInfosLength;
            if ( (pstdiSrchTermDictInfosPtr->pucTerm = (unsigned char *)s_strdup(pucKey)) == NULL ) {
                iStatus = -1;
                goto bailFromiSrchTermDictLookupListCallBack;
            }
            pstdiSrchTermDictInfosPtr->uiTermType = uiTermType;
            pstdiSrchTermDictInfosPtr->uiTermCount = uiTermCount;
            pstdiSrchTermDictInfosPtr->uiDocumentCount = uiDocumentCount;
            uiSrchTermDictInfosLength++;
    
            /* Set the return pointers */
            *ppstdiSrchTermDictInfos = pstdiSrchTermDictInfos;
            *puiSrchTermDictInfosLength = uiSrchTermDictInfosLength;
        }
    }


    /* Handle the status - part ii */
    if ( iStatus != 0 ) {

        /* Free allocations */
        struct spiTermInfo *pstiSpiTermInfos = (struct spiTermInfo *)pstdiSrchTermDictInfos;

        iSpiFreeTermInfo(pstiSpiTermInfos, uiSrchTermDictInfosLength);
        pstiSpiTermInfos = NULL;

        *ppstdiSrchTermDictInfos = NULL;
        *puiSrchTermDictInfosLength = 0;
    }


    return ((iStatus != 0) ? iStatus : ((bFinished == true) ? -1 : 0));

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchTermDictLookupRangeCallBack()

    Purpose:    This function is passed to the dictionary lookup function to
                perform extra processing on a term.

    Parameters: pucKey          key (term)
                pvEntryData     entry data
                uiEntryLength   entry length
                ap              arg list

    Globals:    none

    Returns:    0 to continue processing, non-0 otherwise

*/
static int iSrchTermDictLookupRangeCallBack
(
    unsigned char *pucKey,
    void *pvEntryData,
    unsigned int uiEntryLength,
    va_list ap
)
{

    va_list                     ap_;
    int                         iError = SRCH_NoError;
    int                         iStatus = 0;
    boolean                     bFinished = false;

    struct srchTermDictInfo     *pstdiSrchTermDictInfos = NULL;
    unsigned int                uiSrchTermDictInfosLength = 0;
    unsigned int                uiDictRangeMatchTerm = SRCH_TERM_DICT_RANGE_MATCH_TERM_ALPHA;
    wchar_t                     wcCharacter = L'\0';
    int                         iTermStartNumber = 0;
    int                         iTermEndNumber = 0;
    wchar_t                     *pwcTermStart = NULL;
    wchar_t                     *pwcTermEnd = NULL;
    unsigned int                uiRangeID = 0;
    unsigned int                uiDictCaseScan = 0;
    unsigned char               *pucFieldIDBitmap = NULL;
    unsigned int                uiFieldIDBitmapLength = 0;
    struct srchTermDictInfo     **ppstdiSrchTermDictInfos = NULL;
    unsigned int                *puiSrchTermDictInfosLength = 0;
    unsigned int                *piError = NULL;

    unsigned char               *pucEntryDataPtr = NULL;
    unsigned char               *pucEntryDataEndPtr = NULL;
    unsigned int                uiFieldID = 0;
    unsigned int                uiTermType = 0;
    unsigned int                uiTermCount = 0;
    unsigned int                uiDocumentCount = 0;
    unsigned int                ulIndexBlockID = 0;

    unsigned char               *pucKeyPtr = NULL;

    int                         iDictTermNumber = 0;
    boolean                     bFieldMatch = false;
    boolean                     bTermMatch = false;
    
    wchar_t                     pwcKey[SRCH_TERM_LENGTH_MAXIMUM + 1] = {L'\0'};


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iSrchTermDictLookupRangeCallBack [%s][%u][%s]", pucKey, uiEntryLength, */
/*             (pvEntryData != NULL) ? "(pvEntryData != NULL)" : "(pvEntryData == NULL)"); */


    ASSERT(bUtlStringsIsStringNULL(pucKey) == false);
    ASSERT(pvEntryData != NULL);
    ASSERT(uiEntryLength > 0);


    /* Get all our parameters, note that we make a copy of 'ap' */
    va_copy(ap_, ap);
    uiDictRangeMatchTerm = (unsigned int)va_arg(ap_, unsigned int);
    wcCharacter = (wchar_t)va_arg(ap_, wchar_t);

    if ( (uiDictRangeMatchTerm == SRCH_TERM_DICT_RANGE_MATCH_TERM_NUMERIC) || (uiDictRangeMatchTerm == SRCH_TERM_DICT_RANGE_MATCH_TERM_NUMERIC_RANGE) ) {
        iTermStartNumber = (int)va_arg(ap_, int);
        iTermEndNumber = (int)va_arg(ap_, int);
    }
    else if ( (uiDictRangeMatchTerm == SRCH_TERM_DICT_RANGE_MATCH_TERM_ALPHA) || (uiDictRangeMatchTerm == SRCH_TERM_DICT_RANGE_MATCH_TERM_ALPHA_RANGE) ) {
        pwcTermStart = (wchar_t *)va_arg(ap_, wchar_t *);
        pwcTermEnd = (wchar_t *)va_arg(ap_, wchar_t *);
    }

    uiRangeID = (unsigned int)va_arg(ap_, unsigned int);
    uiDictCaseScan = (unsigned int)va_arg(ap_, unsigned int);
    pucFieldIDBitmap = (unsigned char *)va_arg(ap_, unsigned char *);
    uiFieldIDBitmapLength = (unsigned int)va_arg(ap_, unsigned int);
    ppstdiSrchTermDictInfos = (struct srchTermDictInfo **)va_arg(ap_, struct srchTermDictInfo **);
    puiSrchTermDictInfosLength = (unsigned int *)va_arg(ap_, unsigned int *);
    piError = (int *)va_arg(ap_, int *);
    va_end(ap_);


/*     if ( bUtlLogIsDebug(UTL_LOG_CONTEXT) == true ) { */
/*         if ( (uiDictRangeMatchTerm == SRCH_TERM_DICT_RANGE_MATCH_TERM_NUMERIC) || (uiDictRangeMatchTerm == SRCH_TERM_DICT_RANGE_MATCH_TERM_NUMERIC_RANGE) ) { */
/*             iUtlLogDebug(UTL_LOG_CONTEXT, "iSrchTermDictLookupRangeCallBack [%u][%lc][%d][%d][%u][%u][field ID bitmap %defined][%u]", uiDictRangeMatchTerm, wcCharacter,  */
/*                     iTermStartNumber, iTermEndNumber, uiRangeID, uiDictCaseScan, (pucFieldIDBitmap != NULL) ? "" : "not ", *puiSrchTermDictInfosLength); */
/*         } */
/*         else if ( (uiDictRangeMatchTerm == SRCH_TERM_DICT_RANGE_MATCH_TERM_ALPHA) || (uiDictRangeMatchTerm == SRCH_TERM_DICT_RANGE_MATCH_TERM_ALPHA_RANGE) ) { */
/*             iUtlLogDebug(UTL_LOG_CONTEXT, "iSrchTermDictLookupRangeCallBack [%u][%lc][%ls][%ls][%u][%u][field ID bitmap %defined][%u]", uiDictRangeMatchTerm, wcCharacter,  */
/*                     pwcUtlStringsGetPrintableWideString(pwcTermStart), pwcUtlStringsGetPrintableWideString(pwcTermEnd), uiRangeID, uiDictCaseScan,  */
/*                     (pucFieldIDBitmap != NULL) ? "" : "not ", *puiSrchTermDictInfosLength); */
/*         } */
/*     } */

    ASSERT(SRCH_TERM_DICT_RANGE_MATCH_TERM_VALID(uiDictRangeMatchTerm) == true);
    
    ASSERT( ((uiDictRangeMatchTerm == SRCH_TERM_DICT_RANGE_MATCH_TERM_NUMERIC) || (uiDictRangeMatchTerm == SRCH_TERM_DICT_RANGE_MATCH_TERM_NUMERIC_RANGE)) ||
            (((uiDictRangeMatchTerm == SRCH_TERM_DICT_RANGE_MATCH_TERM_ALPHA) || (uiDictRangeMatchTerm == SRCH_TERM_DICT_RANGE_MATCH_TERM_ALPHA_RANGE)) && 
            (bUtlStringsIsWideStringNULL(pwcTermStart) == false)) );

    ASSERT( ((bUtlStringsIsWideStringNULL(pwcTermStart) == true) && (bUtlStringsIsWideStringNULL(pwcTermEnd) == true)) ||
            ((bUtlStringsIsWideStringNULL(pwcTermStart) == false) && (bUtlStringsIsWideStringNULL(pwcTermEnd) == true)) ||
            ((bUtlStringsIsWideStringNULL(pwcTermStart) == false) && (bUtlStringsIsWideStringNULL(pwcTermEnd) == false)) );

    ASSERT(SRCH_PARSER_RANGE_VALID(uiRangeID) == true);
    
    ASSERT( ((uiRangeID == SRCH_PARSER_RANGE_EQUAL_ID) && ((uiDictRangeMatchTerm == SRCH_TERM_DICT_RANGE_MATCH_TERM_NUMERIC_RANGE) || 
            (uiDictRangeMatchTerm == SRCH_TERM_DICT_RANGE_MATCH_TERM_ALPHA_RANGE))) || 
            (((uiRangeID == SRCH_PARSER_RANGE_EQUAL_ID) || (uiRangeID == SRCH_PARSER_RANGE_NOT_EQUAL_ID) || (uiRangeID == SRCH_PARSER_RANGE_GREATER_ID) || 
            (uiRangeID == SRCH_PARSER_RANGE_LESS_ID) || (uiRangeID == SRCH_PARSER_RANGE_GREATER_OR_EQUAL_ID) || (uiRangeID == SRCH_PARSER_RANGE_LESS_OR_EQUAL_ID)) && 
            ((uiDictRangeMatchTerm == SRCH_TERM_DICT_RANGE_MATCH_TERM_NUMERIC) || (uiDictRangeMatchTerm == SRCH_TERM_DICT_RANGE_MATCH_TERM_ALPHA))) );
    
    ASSERT(SRCH_TERM_DICT_CASE_SCAN_VALID(uiDictCaseScan) == true);
    ASSERT(((pucFieldIDBitmap == NULL) && (uiFieldIDBitmapLength <= 0)) || ((pucFieldIDBitmap != NULL) && (uiFieldIDBitmapLength > 0)));
    ASSERT(ppstdiSrchTermDictInfos != NULL);
    ASSERT(puiSrchTermDictInfosLength != NULL);
    ASSERT(piError != NULL);


    /* Extract some information from the data pointer */ 
    pucEntryDataPtr = (unsigned char *)pvEntryData;
    UTL_NUM_READ_COMPRESSED_UINT(uiTermType, pucEntryDataPtr);
    UTL_NUM_READ_COMPRESSED_UINT(uiTermCount, pucEntryDataPtr);
    UTL_NUM_READ_COMPRESSED_UINT(uiDocumentCount, pucEntryDataPtr);
    UTL_NUM_READ_COMPRESSED_ULONG(ulIndexBlockID, pucEntryDataPtr);


    /* Assume that we are not going to add this term */
    bFieldMatch = false;

    /* Do we need to prequalify this term against the field ID */
    if ( pucFieldIDBitmap != NULL ) {

        pucEntryDataEndPtr = (unsigned char *)pvEntryData + uiEntryLength;

        /* Scan the field IDs */
        while ( pucEntryDataPtr < pucEntryDataEndPtr ) {
        
            /* Decode */
            UTL_NUM_READ_COMPRESSED_UINT(uiFieldID, pucEntryDataPtr);

            ASSERT(uiFieldID <= uiFieldIDBitmapLength); 

            /* Check for a match - field ID 0 is not a field */
            if ( UTL_BITMAP_IS_BIT_SET_IN_POINTER(pucFieldIDBitmap, uiFieldID - 1) ) {
                /* We have a match so we break */
                bFieldMatch = true;
                break;
            }
        }
    }
    else {
        bFieldMatch = true;
    }

    /* Bail here if there is no field match */ 
    if ( bFieldMatch == false ) {
        goto bailFromlLookUpRangeInTermDictCallBack;
    }

    

    /* Assume that we are not going to add this term */
    bTermMatch = false;

    /* Are we looking for numbers or strings ? */
    switch ( uiDictRangeMatchTerm ) {
        
        /* We are looking for numerics */
        case SRCH_TERM_DICT_RANGE_MATCH_TERM_NUMERIC:
        case SRCH_TERM_DICT_RANGE_MATCH_TERM_NUMERIC_RANGE:

            /* Assume that we are not going to add this term */
            bTermMatch = false;

            /* Check that we have not reached the end of the numeric part of the dictionary */
            if ( pucKey[0] > '9' ) {
                bFinished = true;
                goto bailFromlLookUpRangeInTermDictCallBack;
            }

            /* See if the number is numeric (check the first character of this term) */
            if ( ((isdigit(pucKey[0]) == 0) && (pucKey[0] != '-') && (pucKey[0] != '.')) == true ) {
                bTermMatch = false;
                goto bailFromlLookUpRangeInTermDictCallBack;
            }

            /* See if the number is numeric (check all the other characters of this term) */
            for ( bTermMatch = true, pucKeyPtr = pucKey + 1; *pucKeyPtr != '\0'; pucKeyPtr++ ) {
                if ( (isdigit(*pucKeyPtr) == 0) && (*pucKeyPtr != '.') ) {
                    bTermMatch = false;
                    goto bailFromlLookUpRangeInTermDictCallBack;
                }
            }


            /* Assume that we are not going to add this term */
            bTermMatch = false;

            iDictTermNumber = s_strtol(pucKey, NULL, 10);
            
            switch ( uiRangeID ) {
                
                case SRCH_PARSER_RANGE_EQUAL_ID:
                    
                    if ( uiDictRangeMatchTerm == SRCH_TERM_DICT_RANGE_MATCH_TERM_NUMERIC ) {
                        if ( iDictTermNumber == iTermStartNumber ) {
                            bTermMatch = true;
                            bFinished = true;
                        }
                    }
                    else if ( uiDictRangeMatchTerm == SRCH_TERM_DICT_RANGE_MATCH_TERM_NUMERIC_RANGE ) {
                        if ( (iDictTermNumber >= iTermStartNumber) && (iDictTermNumber <= iTermEndNumber) ) {
                            bTermMatch = true;
                        }
                    }
                    break;
                
                
                case SRCH_PARSER_RANGE_NOT_EQUAL_ID:
                    
                    if ( iDictTermNumber != iTermStartNumber ) {
                        bTermMatch = true;
                    }
                    break;
                
                
                case SRCH_PARSER_RANGE_GREATER_ID:
                    
                    if ( iDictTermNumber > iTermStartNumber ) {
                        bTermMatch = true;
                    }
                    break;
                
                
                case SRCH_PARSER_RANGE_LESS_ID:
                    
                    if ( iDictTermNumber < iTermStartNumber ) {
                        bTermMatch = true;
                    }
                    break;
                
                
                case SRCH_PARSER_RANGE_GREATER_OR_EQUAL_ID:
                    
                    if ( iDictTermNumber >= iTermStartNumber ) {
                        bTermMatch = true;
                    }
                    break;
                
                
                case SRCH_PARSER_RANGE_LESS_OR_EQUAL_ID:
                    
                    if ( iDictTermNumber <= iTermStartNumber ) {
                        bTermMatch = true;
                    }
                    break;
                
                
                default:
                    return (-1);
            }
            break;
        
        
        /* We are looking for non-numerics */
        case SRCH_TERM_DICT_RANGE_MATCH_TERM_ALPHA:
        case SRCH_TERM_DICT_RANGE_MATCH_TERM_ALPHA_RANGE:

            /* Assume that we are not going to add this term */
            bTermMatch = false;

            /* Check that we are still in the correct part of the dictionary */
            if ( uiDictCaseScan == SRCH_TERM_DICT_CASE_SCAN_NUMERIC ) {
                if ( (pucKey[0] >= '0') && (pucKey[0] <= '9') ) {
                    bTermMatch = true;
                }
                else if ( pucKey[0] > '9' ) {
                    bFinished = true;
                }
            }
            else if ( uiDictCaseScan == SRCH_TERM_DICT_CASE_SCAN_UPPER ) {
                if ( (pucKey[0] >= 'A') && (pucKey[0] <= 'Z') ) {
                    bTermMatch = true;
                }
                else if ( pucKey[0] > 'Z' ) {
                    bFinished = true;
                }
            }
            else if ( uiDictCaseScan == SRCH_TERM_DICT_CASE_SCAN_LOWER ) {
                if ( (pucKey[0] >= 'a') && (pucKey[0] <= 'z') ) {
                    bTermMatch = true;
                }
                else if ( pucKey[0] > 'z' ) {
                    bFinished = true;
                }
            }
            else if ( uiDictCaseScan == SRCH_TERM_DICT_CASE_SCAN_HIGH ) {
                if ( pucKey[0] > 127 ) {
                    bTermMatch = true;
                }
            }

            /* Check that we can still potentially add this term */
            if ( bTermMatch == false ) {
                goto bailFromlLookUpRangeInTermDictCallBack;
            }


            /* Assume that we are not going to add this term */
            bTermMatch = false;

            /* Convert the key from utf-8 to wide characters */
            if ( (iError = iLngConvertUtf8ToWideString_s(pucKey, 0, pwcKey, SRCH_TERM_LENGTH_MAXIMUM + 1)) != LNG_NoError ) {
                iUtlLogError(UTL_LOG_CONTEXT, "Failed to convert the key from utf-8 to wide characters, lng error: %d.", iError);
                iStatus = -1;
                goto bailFromlLookUpRangeInTermDictCallBack;
            }

            switch ( uiRangeID ) {
                
                case SRCH_PARSER_RANGE_EQUAL_ID:
                    
                    /* Match terms while the first letter of the key matches the character,
                    ** bail as soon as we exit the character range
                    */
                    if ( uiDictRangeMatchTerm == SRCH_TERM_DICT_RANGE_MATCH_TERM_ALPHA ) {
                        if ( pwcKey[0] > wcCharacter ) {
                            bFinished = true;
                        }
                        else if ( pwcKey[0] == wcCharacter ) {
                        
                            int iLocalStatus = s_wcscasecmp(pwcKey, pwcTermStart);

                            if ( iLocalStatus == 0 ) {
                                bTermMatch = true;
                            }
                            else if ( iLocalStatus > 0 ) {
                                if ( s_wcscmp(pwcKey + 1, pwcTermStart + 1) > 0 ) {
                                    bFinished = true;
                                    goto bailFromlLookUpRangeInTermDictCallBack;
                                }
                            }
                        }
                    }
                    else if ( uiDictRangeMatchTerm == SRCH_TERM_DICT_RANGE_MATCH_TERM_ALPHA_RANGE ) {
                        if ( pwcKey[0] > wcCharacter ) {
                            bFinished = true;
                        }
                        else if ( pwcKey[0] == wcCharacter ) {
                        
                            int iLocalStatus = s_wcscasecmp(pwcKey, pwcTermEnd);
                            
                            if ( (s_wcscasecmp(pwcKey, pwcTermStart) >= 0) && (iLocalStatus <= 0) ) {
                                bTermMatch = true;
                            }
                            else if ( iLocalStatus > 0 ) {
                                bFinished = true;
                                goto bailFromlLookUpRangeInTermDictCallBack;
                            }
                        }
                    }
                    break;


                case SRCH_PARSER_RANGE_NOT_EQUAL_ID:

                    if ( s_wcscasecmp(pwcKey, pwcTermStart) != 0 ) {
                        bTermMatch = true;
                    }
                    break;


                case SRCH_PARSER_RANGE_GREATER_ID:

                    /* Match terms while the first letter of the key is greater or equal to the character*/
                    if ( pwcKey[0] >= wcCharacter ) {
                        if ( s_wcscasecmp(pwcKey, pwcTermStart) > 0 ) {
                            bTermMatch = true;
                        }
                    }
                    break;
            
            
                case SRCH_PARSER_RANGE_LESS_ID:
                    
                    /* Match terms while the first letter of the key is less than or equal to the character*/
                    if ( pwcKey[0] > pwcTermStart[0] ) {
                        bFinished = true;
                    }
                    else {
                        if ( s_wcscasecmp(pwcKey, pwcTermStart) < 0 ) {
                            bTermMatch = true;
                        }
                    } 
                    break;


                case SRCH_PARSER_RANGE_GREATER_OR_EQUAL_ID:

                    /* Match terms while the first letter of the key is greater or equal to the character*/
                    if ( pwcKey[0] >= wcCharacter ) {
                        if ( s_wcscasecmp(pwcKey, pwcTermStart) >= 0 ) {
                            bTermMatch = true;
                        }
                    }
                    break;
                
                
                case SRCH_PARSER_RANGE_LESS_OR_EQUAL_ID:

                    /* Match terms while the first letter of the key is less than or equal to the character*/
                    if ( pwcKey[0] > pwcTermStart[0] ) {
                        bFinished = true;
                    }
                    else {
                        if ( s_wcscasecmp(pwcKey, pwcTermStart) <= 0 ) {
                            bTermMatch = true;
                        }
                    } 
                    break;


                default:
                    return (-1);
            }
            break;
        
        default:
            return (-1);
    }



    /* Bail label */
    bailFromlLookUpRangeInTermDictCallBack:

    /* Handle the status - part i */
    if ( iStatus == 0 ) {

        /* Add the term */
        if ( (bFieldMatch == true) && (bTermMatch == true) ) {
    
            struct srchTermDictInfo     *pstdiSrchTermDictInfosPtr = NULL;
    
/*             iUtlLogDebug(UTL_LOG_CONTEXT, "pucKey [%s], uiTermType: [%u], uiTermCount: [%u], uiDocumentCount: [%u]",  pucKey, uiTermType, uiTermCount, uiDocumentCount); */
    
            /* Derefence the term list and the term list length */
            pstdiSrchTermDictInfos = *ppstdiSrchTermDictInfos;
            uiSrchTermDictInfosLength = *puiSrchTermDictInfosLength;
    
            /* Allocate in blocks of SRCH_TERM_DICT_TERM_INFO_ALLOCATION */
            if ( (uiSrchTermDictInfosLength % SRCH_TERM_DICT_TERM_INFO_ALLOCATION) == 0 ) {

                /* Note that we loop back to the bail label here, this is intentional and taken care of by iStatus */
                if ( (pstdiSrchTermDictInfosPtr = (struct srchTermDictInfo *)s_realloc(pstdiSrchTermDictInfos, 
                        (size_t)((uiSrchTermDictInfosLength + SRCH_TERM_DICT_TERM_INFO_ALLOCATION) * sizeof(struct srchTermDictInfo)))) == NULL ) {
                    iStatus = -1;
                    goto bailFromlLookUpRangeInTermDictCallBack;
                }
                pstdiSrchTermDictInfos = pstdiSrchTermDictInfosPtr;
            }
    
            /* Dereference the pointer and add the term */
            pstdiSrchTermDictInfosPtr = pstdiSrchTermDictInfos + uiSrchTermDictInfosLength;
            if ( (pstdiSrchTermDictInfosPtr->pucTerm = (unsigned char *)s_strdup(pucKey)) == NULL ) {
                iStatus = -1;
                goto bailFromlLookUpRangeInTermDictCallBack;
            }
            pstdiSrchTermDictInfosPtr->uiTermType = uiTermType;
            pstdiSrchTermDictInfosPtr->uiTermCount = uiTermCount;
            pstdiSrchTermDictInfosPtr->uiDocumentCount = uiDocumentCount;
            uiSrchTermDictInfosLength++;
    
            /* Set the return pointers */
            *ppstdiSrchTermDictInfos = pstdiSrchTermDictInfos;
            *puiSrchTermDictInfosLength = uiSrchTermDictInfosLength;
        }
    }


    /* Handle the status - part ii */
    if ( iStatus != 0 ) {

        /* Free allocations */
        struct spiTermInfo *pstiSpiTermInfos = (struct spiTermInfo *)pstdiSrchTermDictInfos;

        iSpiFreeTermInfo(pstiSpiTermInfos, uiSrchTermDictInfosLength);
        pstiSpiTermInfos = NULL;

        *ppstdiSrchTermDictInfos = NULL;
        *puiSrchTermDictInfosLength = 0;
    }


    return ((iStatus != 0) ? iStatus : ((bFinished == true) ? -1 : 0));

}


/*---------------------------------------------------------------------------*/


/* 
** =========================
** ===  Match Structure  ===
** =========================
*/


/*

    Function:   iSrchTermDictGetSearchTermDictMatchFromTerm()

    Purpose:    Parse a term into a match structure.

    Parameters: pwcTerm                        term to parse
                bCaseSensitive                  true if case sensitive
                ppstdmSrchTermDictMatch         return pointer for the search term dict structure array
                puiSrchTermDictMatchLength      return pointer for the number of entries 
                                                in the array of match search term dict structures

    Globals:    none

    Returns:     SRCH error code

*/
static int iSrchTermDictGetSearchTermDictMatchFromTerm
(
    wchar_t *pwcTerm,
    boolean bCaseSensitive,
    struct srchTermDictMatch **ppstdmSrchTermDictMatch,
    unsigned int *puiSrchTermDictMatchLength
)
{

    wchar_t                     *pwcTermPtr = NULL;

    wchar_t                     pwcBuffer[SRCH_TERM_LENGTH_MAXIMUM + 1] = {L'\0'};
    wchar_t                     *pwcBufferPtr = NULL;

    struct srchTermDictMatch    *pstdmSrchTermDictMatch = NULL;
    unsigned int                uiSrchTermDictMatchLength = 0;

    unsigned int                uiMatchType = SRCH_TERM_DICT_MATCH_TYPE_LITERAL;


    ASSERT(bUtlStringsIsWideStringNULL(pwcTerm) == false);
    ASSERT((bCaseSensitive == true) || (bCaseSensitive == false));
    ASSERT(ppstdmSrchTermDictMatch != NULL);
    ASSERT(puiSrchTermDictMatchLength != NULL);


    /* Parse the term into a match structure */
    for ( pwcTermPtr = pwcTerm, pwcBufferPtr = pwcBuffer, uiMatchType = SRCH_TERM_DICT_MATCH_TYPE_LITERAL; *pwcTermPtr != L'\0'; pwcTermPtr++ ) {

        /* This will never match, but it makes the defines below a little easier to read */        
        if ( *pwcTermPtr == L' ' ) {
            ;
        }

#if defined(SRCH_TERM_DICT_ENABLE_MULTI_WILDCARD)
        else if ( *pwcTermPtr == SRCH_PARSER_WILDCARD_MULTI_WCHAR ) {

            /* Terminate the current buffer and add it to the match entry list */
            *pwcBufferPtr = L'\0';
            SRCH_TERM_DICT_MATCH_ADD(pstdmSrchTermDictMatch, uiSrchTermDictMatchLength, uiMatchType, pwcBuffer, bCaseSensitive);

            /* Set the next match type and reset the buffer */
            uiMatchType = SRCH_TERM_DICT_MATCH_TYPE_WILDCARD_MULTI;
            pwcBufferPtr = pwcBuffer;
            *pwcBufferPtr = L'\0';

        }
#endif /* defined(SRCH_TERM_DICT_ENABLE_MULTI_WILDCARD) */

#if defined(SRCH_TERM_DICT_ENABLE_SINGLE_WILDCARD)
        else if ( *pwcTermPtr == SRCH_PARSER_WILDCARD_SINGLE_WCHAR ) {

            /* Terminate the current buffer and add it to the match entry list */
            *pwcBufferPtr = L'\0';
            SRCH_TERM_DICT_MATCH_ADD(pstdmSrchTermDictMatch, uiSrchTermDictMatchLength, uiMatchType, pwcBuffer, bCaseSensitive);

            /* Set the next match type and reset the buffer */
            uiMatchType = SRCH_TERM_DICT_MATCH_TYPE_WILDCARD_SINGLE;
            pwcBufferPtr = pwcBuffer;
            *pwcBufferPtr = L'\0';

        }
#endif /* defined(SRCH_TERM_DICT_ENABLE_SINGLE_WILDCARD) */

#if defined(SRCH_TERM_DICT_ENABLE_ALPHA_WILDCARD)
        else if ( *pwcTermPtr == SRCH_PARSER_WILDCARD_ALPHA_WCHAR ) {

            /* Terminate the current buffer and add it to the match entry list */
            *pwcBufferPtr = L'\0';
            SRCH_TERM_DICT_MATCH_ADD(pstdmSrchTermDictMatch, uiSrchTermDictMatchLength, uiMatchType, pwcBuffer, bCaseSensitive);

            /* Set the next match type and reset the buffer */
            uiMatchType = SRCH_TERM_DICT_MATCH_TYPE_WILDCARD_ALPHA;
            pwcBufferPtr = pwcBuffer;
            *pwcBufferPtr = L'\0';

        }
#endif /* defined(SRCH_TERM_DICT_ENABLE_ALPHA_WILDCARD) */

#if defined(SRCH_TERM_DICT_ENABLE_NUMERIC_WILDCARD)
        else if ( *pwcTermPtr == SRCH_PARSER_WILDCARD_NUMERIC_WCHAR ) {

            /* Terminate the current buffer and add it to the match entry list */
            *pwcBufferPtr = L'\0';
            SRCH_TERM_DICT_MATCH_ADD(pstdmSrchTermDictMatch, uiSrchTermDictMatchLength, uiMatchType, pwcBuffer, bCaseSensitive);

            /* Set the next match type and reset the buffer */
            uiMatchType = SRCH_TERM_DICT_MATCH_TYPE_WILDCARD_NUMERIC;
            pwcBufferPtr = pwcBuffer;
            *pwcBufferPtr = L'\0';

        }
#endif /* defined(SRCH_TERM_DICT_ENABLE_NUMERIC_WILDCARD) */

        else {
            /* Cummulate the current letter into the buffer */
            *pwcBufferPtr = *pwcTermPtr;
            pwcBufferPtr++;
        }
    }

    /* And add the final match entry to the match entry list */
    *pwcBufferPtr = L'\0';
    SRCH_TERM_DICT_MATCH_ADD(pstdmSrchTermDictMatch, uiSrchTermDictMatchLength, uiMatchType, pwcBuffer, bCaseSensitive);


    /* Set the return pointers */
    *ppstdmSrchTermDictMatch = pstdmSrchTermDictMatch;
    *puiSrchTermDictMatchLength = uiSrchTermDictMatchLength;


/*     iSrchTermDictPrintSearchTermDictMatch(pstdmSrchTermDictMatch, uiSrchTermDictMatchLength); */


    return (SRCH_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchTermDictMatchTermToSearchTermDictMatch()

    Purpose:    Match a term to a match structure.

    Parameters: pwcTerm                     term to parse
                pstdmSrchTermDictMatch      search term dict structure array
                uiSrchTermDictMatchLength   number of entries in the array of match search term dict structures

    Globals:    none

    Returns:     0 on match, non-0 otherwise

*/
static int iSrchTermDictMatchTermToSearchTermDictMatch
(
    wchar_t *pwcTerm,
    struct srchTermDictMatch *pstdmSrchTermDictMatch,
    unsigned int uiSrchTermDictMatchLength
)
{

    unsigned int                uiI = 0;
    wchar_t                     *pwcPtr = NULL;
    wchar_t                     *pwcTermPtr = NULL;
    struct srchTermDictMatch    *pstdmSrchTermDictMatchPtr = NULL;


    ASSERT(bUtlStringsIsWideStringNULL(pwcTerm) == false);
    ASSERT(pstdmSrchTermDictMatch != NULL);
    ASSERT(uiSrchTermDictMatchLength > 0);


    /* Loop over the match entry list */
    for ( uiI = 0, pwcTermPtr = pwcTerm, pstdmSrchTermDictMatchPtr = pstdmSrchTermDictMatch; (uiI < uiSrchTermDictMatchLength) && (*pwcTermPtr != L'\0'); uiI++, pstdmSrchTermDictMatchPtr++ ) {

        switch ( pstdmSrchTermDictMatchPtr->uiMatchType ) {

            case SRCH_TERM_DICT_MATCH_TYPE_LITERAL:

                if ( pstdmSrchTermDictMatchPtr->bCaseSensitive == true ) {
                    if ( s_wcsncmp(pwcTermPtr, pstdmSrchTermDictMatchPtr->pwcMatchString, pstdmSrchTermDictMatchPtr->uiMatchStringLength) == 0 ) {
                        pwcTermPtr += pstdmSrchTermDictMatchPtr->uiMatchStringLength;
                    }
                    else {
                        return (-1); 
                    }
                }
                else {
                    if ( s_wcsncasecmp(pwcTermPtr, pstdmSrchTermDictMatchPtr->pwcMatchString, pstdmSrchTermDictMatchPtr->uiMatchStringLength) == 0 ) {
                        pwcTermPtr += pstdmSrchTermDictMatchPtr->uiMatchStringLength;
                    }
                    else {
                        return (-1); 
                    }
                }
                break;


#if defined(SRCH_TERM_DICT_ENABLE_MULTI_WILDCARD)
            case SRCH_TERM_DICT_MATCH_TYPE_WILDCARD_MULTI:

                if ( pstdmSrchTermDictMatchPtr->bCaseSensitive == true ) {
                    if ( (pwcPtr = s_wcsstr(pwcTermPtr, pstdmSrchTermDictMatchPtr->pwcMatchString)) != NULL ) {
                        if ( (uiI >= (uiSrchTermDictMatchLength - 1)) && ((bUtlStringsIsWideStringNULL(pstdmSrchTermDictMatchPtr->pwcMatchString) == true)) ) {
                            pwcTermPtr = s_wcschr(pwcTermPtr, L'\0');
                        }
                        else {
                            pwcTermPtr = pwcPtr + pstdmSrchTermDictMatchPtr->uiMatchStringLength;
                        }
                    }
                    else {
                        return (-1); 
                    }
                }
                else {
                    if ( (pwcPtr = s_wcscasestr(pwcTermPtr, pstdmSrchTermDictMatchPtr->pwcMatchString)) != NULL ) {
                        if ( (uiI >= (uiSrchTermDictMatchLength - 1)) && ((bUtlStringsIsWideStringNULL(pstdmSrchTermDictMatchPtr->pwcMatchString) == true)) ) {
                            pwcTermPtr = s_wcschr(pwcTermPtr, L'\0');
                        }
                        else {
                            pwcTermPtr = pwcPtr + pstdmSrchTermDictMatchPtr->uiMatchStringLength;
                        }
                    }
                    else {
                        return (-1); 
                    }
                }
                break;
#endif /* defined(SRCH_TERM_DICT_ENABLE_MULTI_WILDCARD) */


#if defined(SRCH_TERM_DICT_ENABLE_SINGLE_WILDCARD)
            case SRCH_TERM_DICT_MATCH_TYPE_WILDCARD_SINGLE:

                if ( pstdmSrchTermDictMatchPtr->bCaseSensitive == true ) {
                    if ( s_wcsncmp(pwcTermPtr + 1, pstdmSrchTermDictMatchPtr->pwcMatchString, pstdmSrchTermDictMatchPtr->uiMatchStringLength) == 0 ) {
                        pwcTermPtr += (pstdmSrchTermDictMatchPtr->uiMatchStringLength + 1);
                    }
                    else {
                        return (-1); 
                    }
                }
                else {
                    if ( s_wcsncasecmp(pwcTermPtr + 1, pstdmSrchTermDictMatchPtr->pwcMatchString, pstdmSrchTermDictMatchPtr->uiMatchStringLength) == 0 ) {
                        pwcTermPtr += (pstdmSrchTermDictMatchPtr->uiMatchStringLength + 1);
                    }
                    else {
                        return (-1); 
                    }
                }
                break;
#endif /* defined(SRCH_TERM_DICT_ENABLE_SINGLE_WILDCARD) */


#if defined(SRCH_TERM_DICT_ENABLE_ALPHA_WILDCARD)
            case SRCH_TERM_DICT_MATCH_TYPE_WILDCARD_ALPHA:

                if ( pstdmSrchTermDictMatchPtr->bCaseSensitive == true ) {
                    if ( (s_wcsncmp(pwcTermPtr + 1, pstdmSrchTermDictMatchPtr->pwcMatchString, pstdmSrchTermDictMatchPtr->uiMatchStringLength) == 0) &&
                            (iswalpha(*pwcTermPtr) != 0) ) {
                        pwcTermPtr += (pstdmSrchTermDictMatchPtr->uiMatchStringLength + 1);
                    }
                    else {
                        return (-1); 
                    }
                }
                else {
                    if ( (s_wcsncasecmp(pwcTermPtr + 1, pstdmSrchTermDictMatchPtr->pwcMatchString, pstdmSrchTermDictMatchPtr->uiMatchStringLength) == 0) &&
                            (iswalpha(*pwcTermPtr) != 0) ) {
                        pwcTermPtr += (pstdmSrchTermDictMatchPtr->uiMatchStringLength + 1);
                    }
                    else {
                        return (-1); 
                    }
                }
                break;
#endif /* defined(SRCH_TERM_DICT_ENABLE_ALPHA_WILDCARD) */


#if defined(SRCH_TERM_DICT_ENABLE_NUMERIC_WILDCARD)
            case SRCH_TERM_DICT_MATCH_TYPE_WILDCARD_NUMERIC:

                if ( pstdmSrchTermDictMatchPtr->bCaseSensitive == true ) {
                    if ( (s_wcsncasecmp(pwcTermPtr + 1, pstdmSrchTermDictMatchPtr->pwcMatchString, pstdmSrchTermDictMatchPtr->uiMatchStringLength) == 0) &&
                            (iswdigit(*pwcTermPtr) != 0) ) {
                        pwcTermPtr += (pstdmSrchTermDictMatchPtr->uiMatchStringLength + 1);
                    }
                    else {
                        return (-1); 
                    }
                }
                else {
                    if ( (s_wcsncasecmp(pwcTermPtr + 1, pstdmSrchTermDictMatchPtr->pwcMatchString, pstdmSrchTermDictMatchPtr->uiMatchStringLength) == 0) &&
                            (iswdigit(*pwcTermPtr) != 0) ) {
                        pwcTermPtr += (pstdmSrchTermDictMatchPtr->uiMatchStringLength + 1);
                    }
                    else {
                        return (-1); 
                    }
                }
                break;
#endif /* defined(SRCH_TERM_DICT_ENABLE_NUMERIC_WILDCARD) */


            default:
                /* Error - invalid match type entry */
                return (-1); 

        }
    }


    /* Check to see if there is only one more match record and that it is a wildard entry, 
    ** this is to take care of cases such as 'animal*' where the wildcard is at the end of
    ** the term because the loop bails on us at '(*pwcTermPtr != L'\0')' and we dont make 
    ** a match when we should have done so.
    */ 
    if ( (bUtlStringsIsWideStringNULL(pwcTermPtr) == true) && (uiI == (uiSrchTermDictMatchLength - 1)) && 
            ((pstdmSrchTermDictMatch + uiI)->uiMatchType == SRCH_TERM_DICT_MATCH_TYPE_WILDCARD_MULTI) && ((pstdmSrchTermDictMatch + uiI)->uiMatchStringLength == 0) ) {
        uiI = uiSrchTermDictMatchLength;
    }

    /* We failed to make a match if there was anything left in our term pointer of in the match entries */
    if ( (bUtlStringsIsWideStringNULL(pwcTermPtr) == false) || (uiI < uiSrchTermDictMatchLength) ) {
        return (-1);
    }


    /* Return a match */
    return (0);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchTermDictPrintSearchTermDictMatch()

    Purpose:    Print a match structure.

    Parameters: pstdmSrchTermDictMatch      search term dict structure array
                uiSrchTermDictMatchLength   number of entries in the array of match search term dict structures

    Globals:    none

    Returns:     0 on match, non-0 otherwise

*/
static int iSrchTermDictPrintSearchTermDictMatch
(
    struct srchTermDictMatch *pstdmSrchTermDictMatch,
    unsigned int uiSrchTermDictMatchLength
)
{

    unsigned int                uiI = 0;
    struct srchTermDictMatch    *pstdmSrchTermDictMatchPtr = NULL;


    ASSERT(pstdmSrchTermDictMatch != NULL);
    ASSERT(uiSrchTermDictMatchLength > 0);


    /* Loop over the match entry list */
    for ( uiI = 0, pstdmSrchTermDictMatchPtr = pstdmSrchTermDictMatch; uiI < uiSrchTermDictMatchLength; uiI++, pstdmSrchTermDictMatchPtr++ ) {

        printf("uiMatchType: [");
        
        if ( pstdmSrchTermDictMatchPtr->uiMatchType == SRCH_TERM_DICT_MATCH_TYPE_LITERAL ) {
            printf("literal");
        }
        else if ( pstdmSrchTermDictMatchPtr->uiMatchType == SRCH_TERM_DICT_MATCH_TYPE_WILDCARD_MULTI ) {
            printf("wildcard multi (%c)", SRCH_PARSER_WILDCARD_MULTI_CHAR);
        }
        else if ( pstdmSrchTermDictMatchPtr->uiMatchType == SRCH_TERM_DICT_MATCH_TYPE_WILDCARD_SINGLE ) {
            printf("wildcard single (%c)", SRCH_PARSER_WILDCARD_SINGLE_CHAR);
        }
        else if ( pstdmSrchTermDictMatchPtr->uiMatchType == SRCH_TERM_DICT_MATCH_TYPE_WILDCARD_ALPHA ) {
            printf("wildcard alpha (%c)", SRCH_PARSER_WILDCARD_ALPHA_CHAR);
        }
        else if ( pstdmSrchTermDictMatchPtr->uiMatchType == SRCH_TERM_DICT_MATCH_TYPE_WILDCARD_NUMERIC ) {
            printf("wildcard numeric (%c)", SRCH_PARSER_WILDCARD_NUMERIC_CHAR);
        }
        else {
            printf("invalid wildcard");
        }
        
        printf("], pwcMatchString: [%ls], uiMatchStringLength: [%u], bCaseSensitive: [%s]\n", pstdmSrchTermDictMatchPtr->pwcMatchString, 
                pstdmSrchTermDictMatchPtr->uiMatchStringLength, (pstdmSrchTermDictMatchPtr->bCaseSensitive == true) ? "true" : "false");
    }


    return (0);

}


/*---------------------------------------------------------------------------*/


/* 
** =========================
** ===  Regex Structure  ===
** =========================
*/


/*

    Function:   iSrchTermDictGetRegexFromTerm()

    Purpose:    Parse a term into a regex structure.

    Parameters: pucTerm/pwcTerm     term to parse
                pprRegex            return pointer for the regex structure

    Globals:    none

    Returns:     SRCH error code

*/
static int iSrchTermDictGetRegexFromTerm
(
#if defined(TRE_REGEX_ENABLE)
    wchar_t *pwcTerm,
#else
    unsigned char *pucTerm,
#endif    /* TRE_REGEX_ENABLE */
    regex_t **pprRegex
)
{

    int         iError = SRCH_NoError;
    regex_t     *prRegex = NULL;
    int         iStatus = 0;


#if defined(TRE_REGEX_ENABLE)
    ASSERT(bUtlStringsIsWideStringNULL(pwcTerm) == false);
#else
    ASSERT(bUtlStringsIsStringNULL(pucTerm) == false);
#endif    /* TRE_REGEX_ENABLE */
    ASSERT(pprRegex != NULL);


    /* Allocate the buffer for the regex structure */
    if ( (prRegex = (regex_t *)s_malloc((size_t)sizeof(regex_t))) == NULL ) {
        return (SRCH_MemError);
    }

    
    /* Create the regex structure */
#if defined(TRE_REGEX_ENABLE)
    iStatus = s_regwcomp(prRegex, pwcTerm, REG_EXTENDED | REG_NOSUB);
#else
    iStatus = s_regcomp(prRegex, pucTerm, REG_EXTENDED | REG_NOSUB);
#endif    /* TRE_REGEX_ENABLE */


    /* Handle the return status */
    switch ( iStatus ) {
        
        case 0:
            iError = SRCH_NoError;
            break;
        
        case REG_NOMATCH:
            iError = SRCH_ParserRegexNoMatch;
            break;
        
        case REG_BADRPT:
            iError = SRCH_ParserRegexBadRpt;
            break;
        
        case REG_BADBR:
            iError = SRCH_ParserRegexBadBr;
            break;
    
        case REG_BADPAT:
            iError = SRCH_ParserRegexBadPat;
            break;
    
        case REG_EBRACE:
            iError = SRCH_ParserRegexEBrace;
            break;
    
        case REG_EBRACK:
            iError = SRCH_ParserRegexEBrack;
            break;
    
        case REG_ERANGE:
            iError = SRCH_ParserRegexERange;
            break;
    
        case REG_ECTYPE:
            iError = SRCH_ParserRegexECType;
            break;
    
        case REG_ECOLLATE:
            iError = SRCH_ParserRegexECollate;
            break;
    
        case REG_EPAREN:
            iError = SRCH_ParserRegexEParen;
            break;
    
        case REG_ESUBREG:
            iError = SRCH_ParserRegexESubReg;
            break;
    
        case REG_EESCAPE:
            iError = SRCH_ParserRegexEEscape;
            break;
    
        case REG_ESPACE:
            iError = SRCH_ParserRegexESpace;
            break;
    
#if defined(REG_EEND)
        case REG_EEND:
            iError = SRCH_ParserRegexEEnd;
            break;
#endif    /* defined(REG_EEND) */

#if defined(REG_ESIZE)
        case REG_ESIZE:
            iError = SRCH_ParserRegexESize;
            break;
#endif    /* defined(REG_ESIZE) */

#if defined(REG_ENOSYS)
        case REG_ENOSYS:
            iError = SRCH_ParserRegexENoSys;
            break;
#endif    /* defined(REG_ENOSYS) */

        default:
            iError = SRCH_ParserRegexFailed;
    }


    /* Handle the status */
    if ( iStatus == 0 ) {
    
        /* Set the return pointer */
        *pprRegex = prRegex;
    }
    else {
    
        /* Free allocations */
        if ( prRegex != NULL ) {
            s_regfree(prRegex);
            s_free(prRegex);
        }
    }


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchTermDictMatchTermToRegex()

    Purpose:    Match a term to a regex structure.

    Parameters: pucTerm/pwcTerm     term to parse
                prRegex             regex structure

    Globals:    none

    Returns:     0 on match, non-0 otherwise

*/
static int iSrchTermDictMatchTermToRegex
(
#if defined(TRE_REGEX_ENABLE)
    wchar_t *pwcTerm,
#else
    unsigned char *pucTerm,
#endif    /* TRE_REGEX_ENABLE */
    regex_t *prRegex
)
{

    int     iStatus = 0;


#if defined(TRE_REGEX_ENABLE)
    ASSERT(bUtlStringsIsWideStringNULL(pwcTerm) == false);
#else
    ASSERT(bUtlStringsIsStringNULL(pucTerm) == false);
#endif    /* TRE_REGEX_ENABLE */
    ASSERT(prRegex != NULL);


    /* Do the match */
#if defined(TRE_REGEX_ENABLE)
    iStatus = s_regwexec(prRegex, pwcTerm, (size_t)0, NULL, 0);
#else
    iStatus = s_regexec(prRegex, pucTerm, (size_t)0, NULL, 0);
#endif    /* TRE_REGEX_ENABLE */


    return (iStatus);

}


/*---------------------------------------------------------------------------*/
