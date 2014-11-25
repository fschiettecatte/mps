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

    Module:     shortrslt.c

    Author:     Francois Schiettecatte (FS Consulting LLC.)

    Created:    21 January 2006

    Purpose:    This module provides support functions for search.c for 
                processing short results 

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
#define UTL_LOG_CONTEXT                     (unsigned char *)"com.fsconsult.mps.src.search.shortrslt"


/*---------------------------------------------------------------------------*/

/*
** Feature defines
*/

/* Force specific sort - for debug only */
/* #define SRCH_SHORT_RESULTS_ENABLE_FORCED_QUICK_SORT */
/* #define SRCH_SHORT_RESULTS_ENABLE_FORCED_RADIX_SORT */


/* Check that we did not enable both quick sort and radix sort */
#if defined(SRCH_SHORT_RESULTS_ENABLE_FORCED_QUICK_SORT) && defined(SRCH_SHORT_RESULTS_ENABLE_FORCED_RADIX_SORT)
#error "Cannot enable both quick sort and radix sort"
#endif



/* Correlation method to use for deciding whether to use a quicksort 
** or a radix sort when sorting the search short results, if neither method is 
** selected, we always use quicksort.
*/
/* #define SRCH_SHORT_RESULTS_ENABLE_RAW_SCORE_METHOD */
/* #define SRCH_SHORT_RESULTS_ENABLE_DEVIATION_SCORE_METHOD */
#define SRCH_SHORT_RESULTS_ENABLE_PEARSON_METHOD


/* Check that we enabled one method for working out the correlation coefficient */
#if !defined(SRCH_SHORT_RESULTS_ENABLE_RAW_SCORE_METHOD) && !defined(SRCH_SHORT_RESULTS_ENABLE_DEVIATION_SCORE_METHOD) && !defined(SRCH_SHORT_RESULTS_ENABLE_PEARSON_METHOD)
#error "One method for working out the correlation coefficient needs to be enabled"
#endif


/*---------------------------------------------------------------------------*/


/*
** Defines
*/

/* Sort type */
#define SRCH_SHORT_RESULTS_SORT_TYPE_UNKNOWN                SPI_SORT_TYPE_UNKNOWN
#define SRCH_SHORT_RESULTS_SORT_TYPE_DOUBLE_ASC             SPI_SORT_TYPE_DOUBLE_ASC
#define SRCH_SHORT_RESULTS_SORT_TYPE_DOUBLE_DESC            SPI_SORT_TYPE_DOUBLE_DESC
#define SRCH_SHORT_RESULTS_SORT_TYPE_FLOAT_ASC              SPI_SORT_TYPE_FLOAT_ASC
#define SRCH_SHORT_RESULTS_SORT_TYPE_FLOAT_DESC             SPI_SORT_TYPE_FLOAT_DESC
#define SRCH_SHORT_RESULTS_SORT_TYPE_UINT_ASC               SPI_SORT_TYPE_UINT_ASC
#define SRCH_SHORT_RESULTS_SORT_TYPE_UINT_DESC              SPI_SORT_TYPE_UINT_DESC
#define SRCH_SHORT_RESULTS_SORT_TYPE_ULLONG_ASC             SPI_SORT_TYPE_ULONG_ASC
#define SRCH_SHORT_RESULTS_SORT_TYPE_ULONG_DESC             SPI_SORT_TYPE_ULONG_DESC
#define SRCH_SHORT_RESULTS_SORT_TYPE_UCHAR_ASC              SPI_SORT_TYPE_UCHAR_ASC
#define SRCH_SHORT_RESULTS_SORT_TYPE_UCHAR_DESC             SPI_SORT_TYPE_UCHAR_DESC
#define SRCH_SHORT_RESULTS_SORT_TYPE_NO_SORT                SPI_SORT_TYPE_NO_SORT

#define SRCH_SHORT_RESULTS_SORT_TYPE_VALID(n)               (((n) >= SRCH_SHORT_RESULTS_SORT_TYPE_DOUBLE_ASC) && \
                                                                    ((n) <= SRCH_SHORT_RESULTS_SORT_TYPE_NO_SORT))


/*---------------------------------------------------------------------------*/


/*
** Search result count threshold to decide whether we do a radix sort 
** automatically (set to -1 to disable)
*/
#define SRCH_SHORT_RESULTS_HIT_COUNT_THRESHOLD_FOR_RADIX            (1000000)
// #define SRCH_SHORT_RESULTS_HIT_COUNT_THRESHOLD_FOR_RADIX            (-1)


/*
** Search result count threshold to decide whether we run a correlation
** or not, below that we don't, above that we do (set to -1 to disable)
*/
#define SRCH_SHORT_RESULTS_HIT_COUNT_THRESHOLD_FOR_CORRELATION      (100000)
/* #define SRCH_SHORT_RESULTS_HIT_COUNT_THRESHOLD_FOR_CORRELATION      (-1) */


/*
** Correlation coefficient threshold for deciding whether we use 
** a radix sort versus quicksort, below we use quicksort, above we 
** use radix
*/
#define SRCH_SHORT_RESULTS_CORRELATION_COEFFICIENT_THRESHOLD        (0.5)


/* Sort orders  */
#define SRCH_SHORT_RESULTS_SORT_INVALID_ID                          (0)
#define SRCH_SHORT_RESULTS_SORT_QUICK_ID                            (1)
#define SRCH_SHORT_RESULTS_SORT_RADIX_ID                            (2)

#define SRCH_SHORT_RESULTS_SORT_VALID(n)                            (((n) >= SRCH_SHORT_RESULTS_SORT_QUICK_ID) && \
                                                                            ((n) <= SRCH_SHORT_RESULTS_SORT_RADIX_ID))


/*---------------------------------------------------------------------------*/


/*
** Private function prototypes
*/

static int iSrchSearchGetCorrelationCoefficient (struct srchShortResult *pssrSrchShortResults, 
        unsigned int uiSrchShortResultsLength, unsigned int uiSortType, double *pdCorrelationCoefficient);


static int iSrchShortResultQuickSort (struct srchShortResult *pssrSrchShortResults, 
        unsigned int iSrchShortResultsLeftIndex, unsigned int uiSortType);

static int iSrchShortResultQuickSortDoubleAsc (struct srchShortResult *pssrSrchShortResults, 
        int iSrchShortResultsLeftIndex, int iSrchShortResultsRightIndex);

static int iSrchShortResultQuickSortDoubleDesc (struct srchShortResult *pssrSrchShortResults, 
        int iSrchShortResultsLeftIndex, int iSrchShortResultsRightIndex);

static int iSrchShortResultQuickSortFloatAsc (struct srchShortResult *pssrSrchShortResults, 
        int iSrchShortResultsLeftIndex, int iSrchShortResultsRightIndex);

static int iSrchShortResultQuickSortFloatDesc (struct srchShortResult *pssrSrchShortResults, 
        int iSrchShortResultsLeftIndex, int iSrchShortResultsRightIndex);

static int iSrchShortResultQuickSortUIntAsc (struct srchShortResult *pssrSrchShortResults, 
        int iSrchShortResultsLeftIndex, int iSrchShortResultsRightIndex);

static int iSrchShortResultQuickSortUIntDesc (struct srchShortResult *pssrSrchShortResults, 
        int iSrchShortResultsLeftIndex, int iSrchShortResultsRightIndex);

static int iSrchShortResultQuickSortULongAsc (struct srchShortResult *pssrSrchShortResults, 
        int iSrchShortResultsLeftIndex, int iSrchShortResultsRightIndex);

static int iSrchShortResultQuickSortULongDesc (struct srchShortResult *pssrSrchShortResults, 
        int iSrchShortResultsLeftIndex, int iSrchShortResultsRightIndex);

static int iSrchShortResultQuickSortCharAsc (struct srchShortResult *pssrSrchShortResults, 
        int iSrchShortResultsLeftIndex, int iSrchShortResultsRightIndex);

static int iSrchShortResultQuickSortCharDesc (struct srchShortResult *pssrSrchShortResults, 
        int iSrchShortResultsLeftIndex, int iSrchShortResultsRightIndex);


static int iSrchShortResultRadixSort (struct srchShortResult **ppssrSrchShortResults, 
        unsigned int iSrchShortResultsLeftIndex, unsigned int uiSortType);

static int iSrchShortResultRadixSortDouble (struct srchShortResult **ppssrSrchSearchShortResults, 
        unsigned int uiSrchShortResultsLength, unsigned int uiSortOrder);

static int iSrchShortResultRadixSortFloat (struct srchShortResult **ppssrSrchSearchShortResults, 
        unsigned int uiSrchShortResultsLength, unsigned int uiSortOrder);

static int iSrchShortResultRadixSortUInt (struct srchShortResult **ppssrSrchSearchShortResults, 
        unsigned int uiSrchShortResultsLength, unsigned int uiSortOrder);

static int iSrchShortResultRadixSortULong (struct srchShortResult **ppssrSrchSearchShortResults, 
        unsigned int uiSrchShortResultsLength, unsigned int uiSortOrder);


/*---------------------------------------------------------------------------*/


/*
** Macros
*/

/* Macros for moving short result structure content around */
#define SRCH_SHORT_RESULTS_COPY_SRCH_SHORT_RESULTS_DOUBLE(pssr1, pssr2) \
{ \
    (pssr1)->uiDocumentID = (pssr2)->uiDocumentID; \
    (pssr1)->psiSrchIndexPtr = (pssr2)->psiSrchIndexPtr; \
    (pssr1)->dSortKey = (pssr2)->dSortKey; \
}

#define SRCH_SHORT_RESULTS_COPY_SRCH_SHORT_RESULTS_FLOAT(pssr1, pssr2) \
{ \
    (pssr1)->uiDocumentID = (pssr2)->uiDocumentID; \
    (pssr1)->psiSrchIndexPtr = (pssr2)->psiSrchIndexPtr; \
    (pssr1)->fSortKey = (pssr2)->fSortKey; \
}

#define SRCH_SHORT_RESULTS_COPY_SRCH_SHORT_RESULTS_UINT(pssr1, pssr2)   \
{ \
    (pssr1)->uiDocumentID = (pssr2)->uiDocumentID; \
    (pssr1)->psiSrchIndexPtr = (pssr2)->psiSrchIndexPtr; \
    (pssr1)->uiSortKey = (pssr2)->uiSortKey; \
}

#define SRCH_SHORT_RESULTS_COPY_SRCH_SHORT_RESULTS_ULLONG(pssr1, pssr2) \
{ \
    (pssr1)->uiDocumentID = (pssr2)->uiDocumentID; \
    (pssr1)->psiSrchIndexPtr = (pssr2)->psiSrchIndexPtr; \
    (pssr1)->ulSortKey = (pssr2)->ulSortKey; \
}

#define SRCH_SHORT_RESULTS_COPY_SRCH_SHORT_RESULTS_UCHAR(pssr1, pssr2) \
{ \
    (pssr1)->uiDocumentID = (pssr2)->uiDocumentID; \
    (pssr1)->psiSrchIndexPtr = (pssr2)->psiSrchIndexPtr; \
    (pssr1)->pucSortKey = (pssr2)->pucSortKey; \
}

#define SRCH_SHORT_HIT_COPY_SRCH_SHORT_RESULT(pssr1, pssr2) \
{ \
    (pssr1)->uiDocumentID = (pssr2)->uiDocumentID; \
    (pssr1)->psiSrchIndexPtr = (pssr2)->psiSrchIndexPtr; \
}





#define SRCH_SHORT_RESULTS_SWAP_SRCH_SHORT_RESULTS_DOUBLE(pssr1, pssr2) \
{ \
    struct srchShortResult ssrMacroSrchShortResult; \
    SRCH_SHORT_RESULTS_COPY_SRCH_SHORT_RESULTS_DOUBLE(&ssrMacroSrchShortResult, pssr1); \
    SRCH_SHORT_RESULTS_COPY_SRCH_SHORT_RESULTS_DOUBLE(pssr1, pssr2); \
    SRCH_SHORT_RESULTS_COPY_SRCH_SHORT_RESULTS_DOUBLE(pssr2, &ssrMacroSrchShortResult); \
}

#define SRCH_SHORT_RESULTS_SWAP_SRCH_SHORT_RESULTS_FLOAT(pssr1, pssr2) \
{ \
    struct srchShortResult ssrMacroSrchShortResult; \
    SRCH_SHORT_RESULTS_COPY_SRCH_SHORT_RESULTS_FLOAT(&ssrMacroSrchShortResult, pssr1); \
    SRCH_SHORT_RESULTS_COPY_SRCH_SHORT_RESULTS_FLOAT(pssr1, pssr2); \
    SRCH_SHORT_RESULTS_COPY_SRCH_SHORT_RESULTS_FLOAT(pssr2, &ssrMacroSrchShortResult); \
}

#define SRCH_SHORT_RESULTS_SWAP_SRCH_SHORT_RESULTS_UINT(pssr1, pssr2) \
{ \
    struct srchShortResult ssrMacroSrchShortResult; \
    SRCH_SHORT_RESULTS_COPY_SRCH_SHORT_RESULTS_UINT(&ssrMacroSrchShortResult, pssr1); \
    SRCH_SHORT_RESULTS_COPY_SRCH_SHORT_RESULTS_UINT(pssr1, pssr2); \
    SRCH_SHORT_RESULTS_COPY_SRCH_SHORT_RESULTS_UINT(pssr2, &ssrMacroSrchShortResult); \
}

#define SRCH_SHORT_RESULTS_SWAP_SRCH_SHORT_RESULTS_ULLONG(pssr1, pssr2) \
{ \
    struct srchShortResult ssrMacroSrchShortResult; \
    SRCH_SHORT_RESULTS_COPY_SRCH_SHORT_RESULTS_ULLONG(&ssrMacroSrchShortResult, pssr1); \
    SRCH_SHORT_RESULTS_COPY_SRCH_SHORT_RESULTS_ULLONG(pssr1, pssr2); \
    SRCH_SHORT_RESULTS_COPY_SRCH_SHORT_RESULTS_ULLONG(pssr2, &ssrMacroSrchShortResult); \
}

#define SRCH_SHORT_RESULTS_SWAP_SRCH_SHORT_RESULTS_UCHAR(pssr1, pssr2) \
{ \
    struct srchShortResult ssrMacroSrchShortResult; \
    SRCH_SHORT_RESULTS_COPY_SRCH_SHORT_RESULTS_UCHAR(&ssrMacroSrchShortResult, pssr1); \
    SRCH_SHORT_RESULTS_COPY_SRCH_SHORT_RESULTS_UCHAR(pssr1, pssr2); \
    SRCH_SHORT_RESULTS_COPY_SRCH_SHORT_RESULTS_UCHAR(pssr2, &ssrMacroSrchShortResult); \
}

#define SRCH_SHORT_HIT_SWAP_SRCH_SHORT_HIT(pssr1, pssr2) \
{ \
    struct srchShortResult ssrMacroSrchShortResult; \
    SRCH_SHORT_HIT_COPY_SRCH_SHORT_RESULT(&ssrMacroSrchShortResult, pssr1); \
    SRCH_SHORT_HIT_COPY_SRCH_SHORT_RESULT(pssr1, pssr2); \
    SRCH_SHORT_HIT_COPY_SRCH_SHORT_RESULT(pssr2, &ssrMacroSrchShortResult); \
}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchShortResultSort()

    Purpose:    This function sorts the short results array

    Parameters: pssrSrchShortResults        pointer to an array of short result structures
                uiSrchShortResultsLength    number of entries in the array of short result structures
                uiSortType                  sort type (SPI_SORT_TYPE_*)

    Globals:    none

    Returns:    SRCH error code

*/
int iSrchShortResultSort
(
    struct srchShortResult **ppssrSrchShortResults,
    unsigned int uiSrchShortResultsLength,
    unsigned int uiSortType
)
{

    int     iError = SRCH_NoError;
    int     uiSortMethod = SRCH_SHORT_RESULTS_SORT_INVALID_ID;


    /* Check the parameters */
    if ( ppssrSrchShortResults == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppssrSrchShortResults' parameter passed to 'iSrchShortResultSplice'."); 
        return (SRCH_ShortResultInvalidShortResults);
    }

    if ( *ppssrSrchShortResults == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppssrSrchShortResults' parameter passed to 'iSrchShortResultSplice'."); 
        return (SRCH_ShortResultInvalidShortResults);
    }

    if ( uiSrchShortResultsLength <= 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'uiSrchShortResultsLength' parameter passed to 'iSrchShortResultSplice'."); 
        return (SRCH_ShortResultInvalidShortResults);
    }


    /* Automatically select radix sorting if we reach a certain number of results, unless we are sorting alphabetically */
    if ( (SRCH_SHORT_RESULTS_HIT_COUNT_THRESHOLD_FOR_RADIX != -1) && (uiSrchShortResultsLength > SRCH_SHORT_RESULTS_HIT_COUNT_THRESHOLD_FOR_RADIX) && 
            (uiSortType != SRCH_SHORT_RESULTS_SORT_TYPE_UCHAR_ASC) &&
            (uiSortType != SRCH_SHORT_RESULTS_SORT_TYPE_UCHAR_DESC) ) {
/*         iUtlLogDebug(UTL_LOG_CONTEXT, "Selecting radix sort, search result count: %u (threshold: %u)\n", uiSrchShortResultsLength, SRCH_SHORT_RESULTS_HIT_COUNT_THRESHOLD_FOR_RADIX); */
        uiSortMethod = SRCH_SHORT_RESULTS_SORT_RADIX_ID;
    }

    /* See if we have hit the threshold for working out the correlation */
    else if ( (SRCH_SHORT_RESULTS_HIT_COUNT_THRESHOLD_FOR_CORRELATION != -1) && (uiSrchShortResultsLength > SRCH_SHORT_RESULTS_HIT_COUNT_THRESHOLD_FOR_CORRELATION) && 
            ((uiSortType == SPI_SORT_TYPE_DOUBLE_ASC) || (uiSortType == SPI_SORT_TYPE_DOUBLE_DESC) || 
            (uiSortType == SPI_SORT_TYPE_FLOAT_ASC) || (uiSortType == SPI_SORT_TYPE_FLOAT_DESC) || 
            (uiSortType == SPI_SORT_TYPE_UINT_ASC) || (uiSortType == SPI_SORT_TYPE_UINT_DESC) || 
            (uiSortType == SPI_SORT_TYPE_ULONG_ASC) || (uiSortType == SPI_SORT_TYPE_ULONG_DESC)) ) {

        double      dCorrelationCoefficient = 0;

        /* Get the correlation coefficient from the short results, default to quicksort if we cant find it */
        if ( iSrchSearchGetCorrelationCoefficient(*ppssrSrchShortResults, uiSrchShortResultsLength, uiSortType, &dCorrelationCoefficient) == SRCH_NoError ) {
        
            /* Select the radix sort method if we reach the correlation coefficient threshold */
            if ( (dCorrelationCoefficient >= SRCH_SHORT_RESULTS_CORRELATION_COEFFICIENT_THRESHOLD) || (dCorrelationCoefficient <= -SRCH_SHORT_RESULTS_CORRELATION_COEFFICIENT_THRESHOLD) ) {
/*                 iUtlLogDebug(UTL_LOG_CONTEXT, "Selecting radix sort, search result count: %u, correlation coefficient: %f", uiSrchShortResultsLength, dCorrelationCoefficient); */
                uiSortMethod = SRCH_SHORT_RESULTS_SORT_RADIX_ID;
            }
            else {
/*                 iUtlLogDebug(UTL_LOG_CONTEXT, "Selecting quicksort, search result count: %u, correlation coefficient: %f", uiSrchShortResultsLength, dCorrelationCoefficient); */
                uiSortMethod = SRCH_SHORT_RESULTS_SORT_QUICK_ID;
            }    
        }
        else {
            /* Failed to get the correlation coefficient, default to quicksort */
/*             iUtlLogDebug(UTL_LOG_CONTEXT, "Selecting quicksort, failed to get the correlation coefficient"); */
            uiSortMethod = SRCH_SHORT_RESULTS_SORT_QUICK_ID;
        }
    }
    
    /* Default to quicksort */
    else {
/*         iUtlLogDebug(UTL_LOG_CONTEXT, "Selecting quicksort, default"); */
        uiSortMethod = SRCH_SHORT_RESULTS_SORT_QUICK_ID;
    }


/* Force quick sort */
#if defined(SRCH_SHORT_RESULTS_ENABLE_FORCED_QUICK_SORT)

    uiSortMethod = SRCH_SHORT_RESULTS_SORT_QUICK_ID;

#endif    /* defined(SRCH_SHORT_RESULTS_ENABLE_FORCED_QUICK_SORT) */

/* Force radix sort */
#if defined(SRCH_SHORT_RESULTS_ENABLE_FORCED_RADIX_SORT)

    uiSortMethod = SRCH_SHORT_RESULTS_SORT_RADIX_ID;

#endif    /* defined(SRCH_SHORT_RESULTS_ENABLE_FORCED_RADIX_SORT) */

/*     iUtlLogDebug(UTL_LOG_CONTEXT, "uiSortMethod: %u (quicksort = 1, radix = 2)", uiSortMethod); */
/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iSrchShortResultSort - uiSrchShortResultsLength: %u", uiSrchShortResultsLength); */


    /* Sort the short results array with the chosen sort method and sort order */
    if ( uiSortMethod == SRCH_SHORT_RESULTS_SORT_QUICK_ID    ) {

        if ( (iError = iSrchShortResultQuickSort(*ppssrSrchShortResults, uiSrchShortResultsLength, uiSortType)) != SRCH_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to sort the short results: srch error: %d.", iError);
            goto bailFromiSrchShortResultSort;
        }
    }
    else if ( uiSortMethod == SRCH_SHORT_RESULTS_SORT_RADIX_ID    ) {

        if ( (iError = iSrchShortResultRadixSort(ppssrSrchShortResults, uiSrchShortResultsLength, uiSortType)) != SRCH_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to sort the short results: srch error: %d.", iError);
            goto bailFromiSrchShortResultSort;
        }
    }
    else {
        ASSERT(false);
    }


    
    /* Bail label */
    bailFromiSrchShortResultSort:


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchShortResultSplice()

    Purpose:    This functions splices out the passed short results array 
                based on the passed start and end offsets. One of the odities of this 
                function is that the short results array and short results length 
                are passed and returned via the same pointers.

    Parameters: ppssrSrchShortResults           pass and return pointer for a short results array
                puiSrchShortResultsLength       pass and return pointer for the short results array length
                uiSrchShortResultsStartIndex    short results start index
                uiSrchShortResultsEndIndex      short results end index
                uiSortType                      sort type (SPI_SORT_TYPE_*)

    Globals:    none

    Returns:    SRCH error code

*/
int iSrchShortResultSplice
(
    struct srchShortResult **ppssrSrchShortResults,
    unsigned int *puiSrchShortResultsLength,
    unsigned int uiSrchShortResultsStartIndex, 
    unsigned int uiSrchShortResultsEndIndex,
    unsigned int uiSortType
)
{

    struct srchShortResult      *pssrSrchShortResults = NULL;
    unsigned int                uiSrchShortResultsLength = 0;


    /* Check the parameters */
    if ( ppssrSrchShortResults == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppssrSrchShortResults' parameter passed to 'iSrchShortResultSplice'."); 
        return (SRCH_ShortResultInvalidShortResults);
    }

    if ( puiSrchShortResultsLength == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'puiSrchShortResultsLength' parameter passed to 'iSrchShortResultSplice'."); 
        return (SRCH_ShortResultInvalidShortResults);
    }

    if ( *puiSrchShortResultsLength < 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'puiSrchShortResultsLength' parameter passed to 'iSrchShortResultSplice'."); 
        return (SRCH_ShortResultInvalidShortResults);
    }

    if ( uiSrchShortResultsStartIndex < 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiSrchShortResultsStartIndex' parameter passed to 'iSrchShortResultSplice'."); 
        return (SRCH_ShortResultInvalidIndices);
    }

    if ( uiSrchShortResultsEndIndex < 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiSrchShortResultsEndIndex' parameter passed to 'iSrchShortResultSplice'."); 
        return (SRCH_ShortResultInvalidIndices);
    }

    if ( uiSrchShortResultsStartIndex > uiSrchShortResultsEndIndex ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiSrchShortResultsStartIndex' & 'uiSrchShortResultsEndIndex' parameters passed to 'iSrchShortResultSplice'."); 
        return (SRCH_ShortResultInvalidIndices);
    }

    if ( SRCH_SHORT_RESULTS_SORT_TYPE_VALID(uiSortType) == false ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiSortOrder' parameter passed to 'iSrchShortResultSplice: %u.", uiSortType); 
        return (SRCH_ShortResultInvalidSortOrder);
    }

    
    /* Set our working pointer */
    pssrSrchShortResults = *ppssrSrchShortResults;
    uiSrchShortResultsLength = *puiSrchShortResultsLength;


    /* Do we need to splice out the short results? */
    if ( (uiSrchShortResultsStartIndex > 0) || ((uiSrchShortResultsEndIndex > 0) && (uiSrchShortResultsLength > (uiSrchShortResultsEndIndex + 1))) ) {
        
        unsigned int                uiSrchShortResultsActualStartIndex = 0;
        unsigned int                uiSrchShortResultsActualEndIndex = 0;
        struct srchShortResult      *pssrSrchShortResultsPtr = NULL;
        struct srchShortResult      *psrCopyToShortResultPtr = NULL;
        struct srchShortResult      *psrCopyFromShortResultPtr = NULL;
        

        /* Do we need to set the actual start index? */
        if ( uiSrchShortResultsStartIndex > 0 ) {
            /* Set the actual start index to the start index, or to include any search reports */
            uiSrchShortResultsActualStartIndex = (uiSrchShortResultsStartIndex > uiSrchShortResultsLength) ? uiSrchShortResultsLength : uiSrchShortResultsStartIndex;
        }



        /* Set the actual end index */
        if ( (uiSrchShortResultsEndIndex > 0) && (uiSrchShortResultsLength > (uiSrchShortResultsEndIndex + 1)) ) {
            uiSrchShortResultsActualEndIndex = uiSrchShortResultsEndIndex;
        }
        else {
            uiSrchShortResultsActualEndIndex = (uiSrchShortResultsLength - 1);
        }
        
        

        /* Check that all went well */
        ASSERT((uiSrchShortResultsActualStartIndex > 0) || (uiSrchShortResultsActualEndIndex > 0));


        /* Free any character sort keys from the short results that get spliced out */
        if ( (uiSortType == SRCH_SHORT_RESULTS_SORT_TYPE_UCHAR_ASC) || (uiSortType == SRCH_SHORT_RESULTS_SORT_TYPE_UCHAR_DESC) ) {
            
            /* Free the sort key in the short results that are before the start of the splice */
            if ( uiSrchShortResultsActualStartIndex > 0 ) {
                for ( pssrSrchShortResultsPtr = pssrSrchShortResults; pssrSrchShortResultsPtr < (pssrSrchShortResults + uiSrchShortResultsActualStartIndex); pssrSrchShortResultsPtr++ ) {
                    s_free(pssrSrchShortResultsPtr->pucSortKey);
                }
            }

            /* Free the sort key in the short results that are beyond the end of the splice */
            if ( uiSrchShortResultsLength > (uiSrchShortResultsActualEndIndex + 1) ) {
                for ( pssrSrchShortResultsPtr = pssrSrchShortResults + (uiSrchShortResultsActualEndIndex + 1); pssrSrchShortResultsPtr < (pssrSrchShortResults + uiSrchShortResultsLength); pssrSrchShortResultsPtr++ ) {
                    s_free(pssrSrchShortResultsPtr->pucSortKey);
                }
            }
        }
        
        /* Calculate the actual number of short results we want to splice out */
        uiSrchShortResultsLength = (uiSrchShortResultsActualEndIndex - uiSrchShortResultsActualStartIndex) + 1;

        /* Splice out the short results, this means that we shuffle them up the array */
        if ( (uiSrchShortResultsLength > 0) && (uiSrchShortResultsActualStartIndex > 0) ) {

            /* Shuffle up */
            for ( psrCopyToShortResultPtr = pssrSrchShortResults, psrCopyFromShortResultPtr = pssrSrchShortResults + uiSrchShortResultsActualStartIndex;
                    psrCopyFromShortResultPtr < pssrSrchShortResults + (uiSrchShortResultsActualStartIndex + uiSrchShortResultsLength);
                    psrCopyToShortResultPtr++, psrCopyFromShortResultPtr++) {

                /* Copy the short result */
                switch ( uiSortType ) {
        
                    case SRCH_SHORT_RESULTS_SORT_TYPE_DOUBLE_ASC:
                    case SRCH_SHORT_RESULTS_SORT_TYPE_DOUBLE_DESC:
                        SRCH_SHORT_RESULTS_COPY_SRCH_SHORT_RESULTS_DOUBLE(psrCopyToShortResultPtr, psrCopyFromShortResultPtr);
                        break;
        
                    case SRCH_SHORT_RESULTS_SORT_TYPE_FLOAT_ASC:
                    case SRCH_SHORT_RESULTS_SORT_TYPE_FLOAT_DESC:
                        SRCH_SHORT_RESULTS_COPY_SRCH_SHORT_RESULTS_FLOAT(psrCopyToShortResultPtr, psrCopyFromShortResultPtr);
                        break;
                    
                    case SRCH_SHORT_RESULTS_SORT_TYPE_UINT_ASC:
                    case SRCH_SHORT_RESULTS_SORT_TYPE_UINT_DESC:
                        SRCH_SHORT_RESULTS_COPY_SRCH_SHORT_RESULTS_UINT(psrCopyToShortResultPtr, psrCopyFromShortResultPtr);
                        break;
                    
                    case SRCH_SHORT_RESULTS_SORT_TYPE_ULLONG_ASC:
                    case SRCH_SHORT_RESULTS_SORT_TYPE_ULONG_DESC:
                        SRCH_SHORT_RESULTS_COPY_SRCH_SHORT_RESULTS_ULLONG(psrCopyToShortResultPtr, psrCopyFromShortResultPtr);
                        break;
                    
                    case SRCH_SHORT_RESULTS_SORT_TYPE_UCHAR_ASC:
                    case SRCH_SHORT_RESULTS_SORT_TYPE_UCHAR_DESC:
                        SRCH_SHORT_RESULTS_COPY_SRCH_SHORT_RESULTS_UCHAR(psrCopyToShortResultPtr, psrCopyFromShortResultPtr);
                        break;
                    
                    case SRCH_SHORT_RESULTS_SORT_TYPE_NO_SORT:
                        SRCH_SHORT_HIT_COPY_SRCH_SHORT_RESULT(psrCopyToShortResultPtr, psrCopyFromShortResultPtr);
                        break;
        
                    case SRCH_SHORT_RESULTS_SORT_TYPE_UNKNOWN:
                        SRCH_SHORT_HIT_COPY_SRCH_SHORT_RESULT(psrCopyToShortResultPtr, psrCopyFromShortResultPtr);
                        break;
                    
                    default:
                        ASSERT(false);
                        break;
                }
            }
        }
    }


    /* Release the short results if the final number was 0 */
    if ( uiSrchShortResultsLength == 0 ) {
        iSrchShortResultFree(pssrSrchShortResults, uiSrchShortResultsLength, uiSortType);
        pssrSrchShortResults = NULL; 
    }


    /* Set the return pointers */
    *ppssrSrchShortResults = pssrSrchShortResults;
    *puiSrchShortResultsLength = uiSrchShortResultsLength;


    /* For debugging */        
/*     iSrchShortResultPrint(pssrSrchShortResults, uiSrchShortResultsLength, uiSortType); */


    return (SRCH_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchShortResultFree()

    Purpose:    This function frees the short results array, note thar the 
                index pointer is not freed.

    Parameters: pssrSrchShortResults        pointer to an array of short result structures
                uiSrchShortResultsLength    number of entries in the array of short result structures
                uiSortType                  sort type (SPI_SORT_TYPE_*)

    Globals:    none

    Returns:    SRCH error code

*/
int iSrchShortResultFree
(
    struct srchShortResult *pssrSrchShortResults,
    unsigned int uiSrchShortResultsLength,
    unsigned int uiSortType
)
{

    /* Free the short results */
    if ( pssrSrchShortResults != NULL ) {

        /* Free the character sort key */
        if ( (uiSrchShortResultsLength > 0) && ((uiSortType == SRCH_SHORT_RESULTS_SORT_TYPE_UCHAR_ASC) || (uiSortType == SRCH_SHORT_RESULTS_SORT_TYPE_UCHAR_DESC)) ) {

            unsigned int                uiI = 0;
            struct srchShortResult      *pssrSrchShortResultsPtr = NULL;

            for ( uiI = 0, pssrSrchShortResultsPtr = pssrSrchShortResults; uiI < uiSrchShortResultsLength; uiI++, pssrSrchShortResultsPtr++ ) {
                s_free(pssrSrchShortResultsPtr->pucSortKey);
            }
        }

        s_free(pssrSrchShortResults);
    }
    
    
    return (SRCH_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchShortResultPrint()

    Purpose:    This function prints out the contents of the short results array.
                This is used for debuging purposes only.

    Parameters: pssrSrchShortResults        pointer to an array of short result structures
                uiSrchShortResultsLength    number of entries in the array of short result structures
                uiSortType                  sort type (SPI_SORT_TYPE_*)

    Globals:    none

    Returns:    SRCH error code

*/
int iSrchShortResultPrint
(
    struct srchShortResult *pssrSrchShortResults,
    unsigned int uiSrchShortResultsLength,
    unsigned int uiSortType
)
{

    unsigned int                uiI = 0;
    struct srchShortResult      *pssrSrchShortResultsPtr = NULL;


    /* Check input variables */
    if ( pssrSrchShortResults == NULL ) {
        iUtlLogInfo(UTL_LOG_CONTEXT, "iSrchShortResultPrint - pssrSrchShortResults pointer is NULL");
    }

    if ( uiSrchShortResultsLength <= 0 ) {
        iUtlLogInfo(UTL_LOG_CONTEXT, "iSrchShortResultPrint - uiSrchShortResultsLength = %u", uiSrchShortResultsLength);
    }

    if ( (pssrSrchShortResults == NULL) || (uiSrchShortResultsLength <= 0) ) {
        return (SRCH_NoError);
    }


    /* Print out the short results */
    for ( uiI = 0, pssrSrchShortResultsPtr = pssrSrchShortResults; uiI < uiSrchShortResultsLength; uiI++, pssrSrchShortResultsPtr++ ) {
    
        unsigned char   pucSortKey[UTL_FILE_PATH_MAX + 1] = {'\0'};
        
        /* Create the sort key string */
        switch ( uiSortType ) {

            case SRCH_SHORT_RESULTS_SORT_TYPE_DOUBLE_ASC:
            case SRCH_SHORT_RESULTS_SORT_TYPE_DOUBLE_DESC:
                snprintf(pucSortKey, UTL_FILE_PATH_MAX + 1, "dSortKey [%9.4f]", pssrSrchShortResultsPtr->dSortKey);
                break;

            case SRCH_SHORT_RESULTS_SORT_TYPE_FLOAT_ASC:
            case SRCH_SHORT_RESULTS_SORT_TYPE_FLOAT_DESC:
                snprintf(pucSortKey, UTL_FILE_PATH_MAX + 1, "fSortKey [%9.4f]", pssrSrchShortResultsPtr->fSortKey);
                break;
            
            case SRCH_SHORT_RESULTS_SORT_TYPE_UINT_ASC:
            case SRCH_SHORT_RESULTS_SORT_TYPE_UINT_DESC:
                snprintf(pucSortKey, UTL_FILE_PATH_MAX + 1, "uiSortKey [%u]", pssrSrchShortResultsPtr->uiSortKey);
                break;
            
            case SRCH_SHORT_RESULTS_SORT_TYPE_ULLONG_ASC:
            case SRCH_SHORT_RESULTS_SORT_TYPE_ULONG_DESC:
                snprintf(pucSortKey, UTL_FILE_PATH_MAX + 1, "ulSortKey [%lu]", pssrSrchShortResultsPtr->ulSortKey);
                break;
            
            case SRCH_SHORT_RESULTS_SORT_TYPE_UCHAR_ASC:
            case SRCH_SHORT_RESULTS_SORT_TYPE_UCHAR_DESC:
                snprintf(pucSortKey, UTL_FILE_PATH_MAX + 1, "pucSortKey [%s]", pucUtlStringsGetPrintableString(pssrSrchShortResultsPtr->pucSortKey));
                break;
            
            case SRCH_SHORT_RESULTS_SORT_TYPE_NO_SORT:
                snprintf(pucSortKey, UTL_FILE_PATH_MAX + 1, "SortKey [none]");
                break;

            case SRCH_SHORT_RESULTS_SORT_TYPE_UNKNOWN:
                snprintf(pucSortKey, UTL_FILE_PATH_MAX + 1, "SortKey [unknown]");
                break;
            
            default:
                snprintf(pucSortKey, UTL_FILE_PATH_MAX + 1, "SortKey [invalid]");
                break;
        }
    
        printf("Hit %u, uiDocumentID %u, %s, pucIndexName: '%s'\n", 
                uiI, pssrSrchShortResultsPtr->uiDocumentID, pucSortKey, pssrSrchShortResultsPtr->psiSrchIndexPtr->pucIndexName);
    }


    return (SRCH_NoError);

}


/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/




/*

    Function:   iSrchSearchGetCorrelationCoefficient()

    Purpose:    This function correlates the document ID and the sort key in the short results.

    Parameters: pssrSrchShortResults        pointer at an allocated array of short results
                uiSrchShortResultsLength    number of entries in the short results
                uiSortType                  sort type
                pdCorrelationCoefficient    return pointer for the correlation coefficient

    Globals:    none

    Returns:    SRCH error code

*/
static int iSrchSearchGetCorrelationCoefficient
(
    struct srchShortResult *pssrSrchShortResults,
    unsigned int uiSrchShortResultsLength,
    unsigned int uiSortType,
    double *pdCorrelationCoefficient
)
{

    struct srchShortResult      *pssrSrchShortResultsPtr = NULL;
    struct srchShortResult      *pssrSrchShortResultEndPtr = NULL;


#if defined(SRCH_SHORT_RESULTS_ENABLE_RAW_SCORE_METHOD)
    double                      dSumX = 0;
    double                      dSumY = 0;

    double                      dSumSquaredX = 0;
    double                      dSumSquaredY = 0;

    double                      dMeanX = 0;
    double                      dMeanY = 0;

    double                      dVarianceX = 0;
    double                      dVarianceY = 0;
        
    double                      dStandardDeviationX = 0;
    double                      dStandardDeviationY = 0;
        
    double                      dSumZxy = 0;
#endif    /* defined(SRCH_SHORT_RESULTS_ENABLE_RAW_SCORE_METHOD) */
    

#if defined(SRCH_SHORT_RESULTS_ENABLE_DEVIATION_SCORE_METHOD)
    double                      dSumX = 0;
    double                      dSumY = 0;

    double                      dSumSquaredX = 0;
    double                      dSumSquaredY = 0;

    double                      dMeanX = 0;
    double                      dMeanY = 0;

    double                      dVarianceX = 0;
    double                      dVarianceY = 0;
        
    double                      dStandardDeviationX = 0;
    double                      dStandardDeviationY = 0;
        
    double                      dSumZxy = 0;
#endif    /* defined(SRCH_SHORT_RESULTS_ENABLE_DEVIATION_SCORE_METHOD) */


#if defined(SRCH_SHORT_RESULTS_ENABLE_PEARSON_METHOD)
    unsigned int                uiI = 0;

    double                      dMeanX = 0;
    double                      dMeanY = 0;

    double                      dSweep = 0;

    double                      dDeltaX = 0;
    double                      dDeltaY = 0;

    double                      dSumSquaredX = 0;
    double                      dSumSquaredY = 0;

    double                      dSumCoproduct = 0;

    double                      dVarianceX = 0;
    double                      dVarianceY = 0;
        
    double                      dStandardDeviationX = 0;
    double                      dStandardDeviationY = 0;
        
    double                      dCovariance = 0;
#endif    /* defined(SRCH_SHORT_RESULTS_ENABLE_PEARSON_METHOD) */


    double                      dCorrelationCoefficient = 0;

    struct timeval              tvCorrelationStartTimeVal;
    struct timeval              tvCorrelationEndTimeVal;
    struct timeval              tvCorrelationDiffTimeVal;
    double                      dCorrelationTime = 0;


    ASSERT(pssrSrchShortResults != NULL);
    ASSERT(uiSrchShortResultsLength >= 0);
    ASSERT(SRCH_SHORT_RESULTS_SORT_TYPE_VALID(uiSortType) == true);
    ASSERT(pdCorrelationCoefficient != NULL);

    

    /* Set the start time */
    s_gettimeofday(&tvCorrelationStartTimeVal, NULL);


    /* We cannot correlate less than 2 data points */
    if ( uiSrchShortResultsLength < 2 ) {

        /* Set the return pointer */
        *pdCorrelationCoefficient = 1;
        return (SRCH_NoError);
    }


/* Raw score method */
#if defined(SRCH_SHORT_RESULTS_ENABLE_RAW_SCORE_METHOD)

/*     iUtlLogInfo(UTL_LOG_CONTEXT, "Raw Score Method"); */

    /* First step is to work out the total counts and total squared counts */
    if ( (uiSortType == SPI_SORT_TYPE_DOUBLE_ASC) || (uiSortType == SPI_SORT_TYPE_DOUBLE_DESC) ) {

        for ( pssrSrchShortResultsPtr = pssrSrchShortResults, pssrSrchShortResultEndPtr = (pssrSrchShortResults + uiSrchShortResultsLength); 
                pssrSrchShortResultsPtr < pssrSrchShortResultEndPtr; pssrSrchShortResultsPtr++ ) {

            dSumX += pssrSrchShortResultsPtr->uiDocumentID;
            dSumSquaredX += (double)pssrSrchShortResultsPtr->uiDocumentID * pssrSrchShortResultsPtr->uiDocumentID;
            
            dSumY += pssrSrchShortResultsPtr->dSortKey;
            dSumSquaredY += (double)pssrSrchShortResultsPtr->dSortKey * pssrSrchShortResultsPtr->dSortKey;
        }
    }
    else if ( (uiSortType == SPI_SORT_TYPE_FLOAT_ASC) || (uiSortType == SPI_SORT_TYPE_FLOAT_DESC) ) {

        for ( pssrSrchShortResultsPtr = pssrSrchShortResults, pssrSrchShortResultEndPtr = (pssrSrchShortResults + uiSrchShortResultsLength); 
                pssrSrchShortResultsPtr < pssrSrchShortResultEndPtr; pssrSrchShortResultsPtr++ ) {

            dSumX += pssrSrchShortResultsPtr->uiDocumentID;
            dSumSquaredX += (double)pssrSrchShortResultsPtr->uiDocumentID * pssrSrchShortResultsPtr->uiDocumentID;
            
            dSumY += pssrSrchShortResultsPtr->fSortKey;
            dSumSquaredY += (double)pssrSrchShortResultsPtr->fSortKey * pssrSrchShortResultsPtr->fSortKey;
        }
    }
    else if ( (uiSortType == SPI_SORT_TYPE_UINT_ASC) || (uiSortType == SPI_SORT_TYPE_UINT_DESC) ) {

        for ( pssrSrchShortResultsPtr = pssrSrchShortResults, pssrSrchShortResultEndPtr = (pssrSrchShortResults + uiSrchShortResultsLength); 
                pssrSrchShortResultsPtr < pssrSrchShortResultEndPtr; pssrSrchShortResultsPtr++ ) {

            dSumX += pssrSrchShortResultsPtr->uiDocumentID;
            dSumSquaredX += (double)pssrSrchShortResultsPtr->uiDocumentID * pssrSrchShortResultsPtr->uiDocumentID;
            
            dSumY += pssrSrchShortResultsPtr->uiSortKey;
            dSumSquaredY += (double)pssrSrchShortResultsPtr->uiSortKey * pssrSrchShortResultsPtr->uiSortKey;
        }
    }
    else if ( (uiSortType == SPI_SORT_TYPE_ULONG_ASC) || (uiSortType == SPI_SORT_TYPE_ULONG_DESC) ) {

        for ( pssrSrchShortResultsPtr = pssrSrchShortResults, pssrSrchShortResultEndPtr = (pssrSrchShortResults + uiSrchShortResultsLength); 
                pssrSrchShortResultsPtr < pssrSrchShortResultEndPtr; pssrSrchShortResultsPtr++ ) {

            dSumX += pssrSrchShortResultsPtr->uiDocumentID;
            dSumSquaredX += (double)pssrSrchShortResultsPtr->uiDocumentID * pssrSrchShortResultsPtr->uiDocumentID;
            
            dSumY += pssrSrchShortResultsPtr->ulSortKey;
            dSumSquaredY += (double)pssrSrchShortResultsPtr->ulSortKey * pssrSrchShortResultsPtr->ulSortKey;
        }
    }
    else {
        ASSERT(false);
    }


    /* Work out the averages */
    dMeanX = dSumX / uiSrchShortResultsLength;
    dMeanY = dSumY / uiSrchShortResultsLength;

    /* Work out the variance */
    dVarianceX = (dSumSquaredX - ((dSumX * dSumX) / uiSrchShortResultsLength)) / uiSrchShortResultsLength;
    dVarianceY = (dSumSquaredY - ((dSumY * dSumY) / uiSrchShortResultsLength)) / uiSrchShortResultsLength;

    /* Work out the standard deviations */
    dStandardDeviationX = sqrt(dVarianceX);
    dStandardDeviationY = sqrt(dVarianceY);


    /* Work out the total Zxy value  */
    if ( (uiSortType == SPI_SORT_TYPE_DOUBLE_ASC) || (uiSortType == SPI_SORT_TYPE_DOUBLE_DESC) ) {
        for ( pssrSrchShortResultsPtr = pssrSrchShortResults, pssrSrchShortResultEndPtr = (pssrSrchShortResults + uiSrchShortResultsLength); 
                pssrSrchShortResultsPtr < pssrSrchShortResultEndPtr; pssrSrchShortResultsPtr++ ) {
            dSumZxy += ((dMeanX - pssrSrchShortResultsPtr->uiDocumentID) / dStandardDeviationX) * ((dMeanY - pssrSrchShortResultsPtr->dSortKey) / dStandardDeviationY);
        }
    }
    else if ( (uiSortType == SPI_SORT_TYPE_FLOAT_ASC) || (uiSortType == SPI_SORT_TYPE_FLOAT_DESC) ) {
        for ( pssrSrchShortResultsPtr = pssrSrchShortResults, pssrSrchShortResultEndPtr = (pssrSrchShortResults + uiSrchShortResultsLength); 
                pssrSrchShortResultsPtr < pssrSrchShortResultEndPtr; pssrSrchShortResultsPtr++ ) {
            dSumZxy += ((dMeanX - pssrSrchShortResultsPtr->uiDocumentID) / dStandardDeviationX) * ((dMeanY - pssrSrchShortResultsPtr->fSortKey) / dStandardDeviationY);
        }
    }
    else if ( (uiSortType == SPI_SORT_TYPE_UINT_ASC) || (uiSortType == SPI_SORT_TYPE_UINT_DESC) ) {
        for ( pssrSrchShortResultsPtr = pssrSrchShortResults, pssrSrchShortResultEndPtr = (pssrSrchShortResults + uiSrchShortResultsLength); 
                pssrSrchShortResultsPtr < pssrSrchShortResultEndPtr; pssrSrchShortResultsPtr++ ) {
            dSumZxy += ((dMeanX - pssrSrchShortResultsPtr->uiDocumentID) / dStandardDeviationX) * ((dMeanY - pssrSrchShortResultsPtr->uiSortKey) / dStandardDeviationY);
        }
    }
    else if ( (uiSortType == SPI_SORT_TYPE_ULONG_ASC) || (uiSortType == SPI_SORT_TYPE_ULONG_DESC) ) {
        for ( pssrSrchShortResultsPtr = pssrSrchShortResults, pssrSrchShortResultEndPtr = (pssrSrchShortResults + uiSrchShortResultsLength); 
                pssrSrchShortResultsPtr < pssrSrchShortResultEndPtr; pssrSrchShortResultsPtr++ ) {
            dSumZxy += ((dMeanX - pssrSrchShortResultsPtr->uiDocumentID) / dStandardDeviationX) * ((dMeanY - pssrSrchShortResultsPtr->ulSortKey) / dStandardDeviationY);
        }
    }
    else {
        ASSERT(false);
    }
    

    /* Work out the correlation coefficient */
    dCorrelationCoefficient = dSumZxy / (uiSrchShortResultsLength - 1);

#endif    /* defined(SRCH_SHORT_RESULTS_ENABLE_RAW_SCORE_METHOD) */



/* Deviation score method */
#if defined(SRCH_SHORT_RESULTS_ENABLE_DEVIATION_SCORE_METHOD)

/*     iUtlLogInfo(UTL_LOG_CONTEXT, "Deviation Score Method"); */
    
    /* First step is to work out the total counts */
    if ( (uiSortType == SPI_SORT_TYPE_DOUBLE_ASC) || (uiSortType == SPI_SORT_TYPE_DOUBLE_DESC) ) {
        for ( pssrSrchShortResultsPtr = pssrSrchShortResults, pssrSrchShortResultEndPtr = (pssrSrchShortResults + uiSrchShortResultsLength); 
                pssrSrchShortResultsPtr < pssrSrchShortResultEndPtr; pssrSrchShortResultsPtr++ ) {
            dSumX += pssrSrchShortResultsPtr->uiDocumentID;
            dSumY += pssrSrchShortResultsPtr->dSortKey;
        }
    }
    else if ( (uiSortType == SPI_SORT_TYPE_FLOAT_ASC) || (uiSortType == SPI_SORT_TYPE_FLOAT_DESC) ) {
        for ( pssrSrchShortResultsPtr = pssrSrchShortResults, pssrSrchShortResultEndPtr = (pssrSrchShortResults + uiSrchShortResultsLength); 
                pssrSrchShortResultsPtr < pssrSrchShortResultEndPtr; pssrSrchShortResultsPtr++ ) {
            dSumX += pssrSrchShortResultsPtr->uiDocumentID;
            dSumY += pssrSrchShortResultsPtr->fSortKey;
        }
    }
    else if ( (uiSortType == SPI_SORT_TYPE_UINT_ASC) || (uiSortType == SPI_SORT_TYPE_UINT_DESC) ) {
        for ( pssrSrchShortResultsPtr = pssrSrchShortResults, pssrSrchShortResultEndPtr = (pssrSrchShortResults + uiSrchShortResultsLength); 
                pssrSrchShortResultsPtr < pssrSrchShortResultEndPtr; pssrSrchShortResultsPtr++ ) {
            dSumX += pssrSrchShortResultsPtr->uiDocumentID;
            dSumY += pssrSrchShortResultsPtr->uiSortKey;
        }
    }
    else if ( (uiSortType == SPI_SORT_TYPE_ULONG_ASC) || (uiSortType == SPI_SORT_TYPE_ULONG_DESC) ) {
        for ( pssrSrchShortResultsPtr = pssrSrchShortResults, pssrSrchShortResultEndPtr = (pssrSrchShortResults + uiSrchShortResultsLength); 
                pssrSrchShortResultsPtr < pssrSrchShortResultEndPtr; pssrSrchShortResultsPtr++ ) {
            dSumX += pssrSrchShortResultsPtr->uiDocumentID;
            dSumY += pssrSrchShortResultsPtr->ulSortKey;
        }
    }
    else {
        ASSERT(false);
    }


    /* Work out the averages */
    dMeanX = dSumX / uiSrchShortResultsLength;
    dMeanY = dSumY / uiSrchShortResultsLength;


    /* Work out the total squared X and Y values  */
    if ( (uiSortType == SPI_SORT_TYPE_DOUBLE_ASC) || (uiSortType == SPI_SORT_TYPE_DOUBLE_DESC) ) {
        for ( pssrSrchShortResultsPtr = pssrSrchShortResults, pssrSrchShortResultEndPtr = (pssrSrchShortResults + uiSrchShortResultsLength); 
                pssrSrchShortResultsPtr < pssrSrchShortResultEndPtr; pssrSrchShortResultsPtr++ ) {
            dSumSquaredX += ((double)pssrSrchShortResultsPtr->uiDocumentID - dMeanX) * ((double)pssrSrchShortResultsPtr->uiDocumentID - dMeanX);
            dSumSquaredY += ((double)pssrSrchShortResultsPtr->dSortKey - dMeanY) * ((double)pssrSrchShortResultsPtr->dSortKey - dMeanY);
        }
    }
    else if ( (uiSortType == SPI_SORT_TYPE_FLOAT_ASC) || (uiSortType == SPI_SORT_TYPE_FLOAT_DESC) ) {
        for ( pssrSrchShortResultsPtr = pssrSrchShortResults, pssrSrchShortResultEndPtr = (pssrSrchShortResults + uiSrchShortResultsLength); 
                pssrSrchShortResultsPtr < pssrSrchShortResultEndPtr; pssrSrchShortResultsPtr++ ) {
            dSumSquaredX += ((double)pssrSrchShortResultsPtr->uiDocumentID - dMeanX) * ((double)pssrSrchShortResultsPtr->uiDocumentID - dMeanX);
            dSumSquaredY += ((double)pssrSrchShortResultsPtr->fSortKey - dMeanY) * ((double)pssrSrchShortResultsPtr->fSortKey - dMeanY);
        }
    }
    else if ( (uiSortType == SPI_SORT_TYPE_UINT_ASC) || (uiSortType == SPI_SORT_TYPE_UINT_DESC) ) {
        for ( pssrSrchShortResultsPtr = pssrSrchShortResults, pssrSrchShortResultEndPtr = (pssrSrchShortResults + uiSrchShortResultsLength); 
                pssrSrchShortResultsPtr < pssrSrchShortResultEndPtr; pssrSrchShortResultsPtr++ ) {
            dSumSquaredX += ((double)pssrSrchShortResultsPtr->uiDocumentID - dMeanX) * ((double)pssrSrchShortResultsPtr->uiDocumentID - dMeanX);
            dSumSquaredY += ((double)pssrSrchShortResultsPtr->uiSortKey - dMeanY) * ((double)pssrSrchShortResultsPtr->uiSortKey - dMeanY);
        }
    }
    else if ( (uiSortType == SPI_SORT_TYPE_ULONG_ASC) || (uiSortType == SPI_SORT_TYPE_ULONG_DESC) ) {
        for ( pssrSrchShortResultsPtr = pssrSrchShortResults, pssrSrchShortResultEndPtr = (pssrSrchShortResults + uiSrchShortResultsLength); 
                pssrSrchShortResultsPtr < pssrSrchShortResultEndPtr; pssrSrchShortResultsPtr++ ) {
            dSumSquaredX += ((double)pssrSrchShortResultsPtr->uiDocumentID - dMeanX) * ((double)pssrSrchShortResultsPtr->uiDocumentID - dMeanX);
            dSumSquaredY += ((double)pssrSrchShortResultsPtr->ulSortKey - dMeanY) * ((double)pssrSrchShortResultsPtr->ulSortKey - dMeanY);
        }
    }
    else {
        ASSERT(false);
    }
    

    /* Work out the variance */
    dVarianceX = dSumSquaredX / uiSrchShortResultsLength;
    dVarianceY = dSumSquaredY / uiSrchShortResultsLength;

    /* Work out the standard deviations */
    dStandardDeviationX = sqrt(dVarianceX);
    dStandardDeviationY = sqrt(dVarianceY);


    /* Work out the total Zxy value  */
    if ( (uiSortType == SPI_SORT_TYPE_DOUBLE_ASC) || (uiSortType == SPI_SORT_TYPE_DOUBLE_DESC) ) {
        for ( pssrSrchShortResultsPtr = pssrSrchShortResults, pssrSrchShortResultEndPtr = (pssrSrchShortResults + uiSrchShortResultsLength); 
                pssrSrchShortResultsPtr < pssrSrchShortResultEndPtr; pssrSrchShortResultsPtr++ ) {
            dSumZxy += ((dMeanX - pssrSrchShortResultsPtr->uiDocumentID) / dStandardDeviationX) * ((dMeanY - pssrSrchShortResultsPtr->dSortKey) / dStandardDeviationY);
        }
    }
    else if ( (uiSortType == SPI_SORT_TYPE_FLOAT_ASC) || (uiSortType == SPI_SORT_TYPE_FLOAT_DESC) ) {
        for ( pssrSrchShortResultsPtr = pssrSrchShortResults, pssrSrchShortResultEndPtr = (pssrSrchShortResults + uiSrchShortResultsLength); 
                pssrSrchShortResultsPtr < pssrSrchShortResultEndPtr; pssrSrchShortResultsPtr++ ) {
            dSumZxy += ((dMeanX - pssrSrchShortResultsPtr->uiDocumentID) / dStandardDeviationX) * ((dMeanY - pssrSrchShortResultsPtr->fSortKey) / dStandardDeviationY);
        }
    }
    else if ( (uiSortType == SPI_SORT_TYPE_UINT_ASC) || (uiSortType == SPI_SORT_TYPE_UINT_DESC) ) {
        for ( pssrSrchShortResultsPtr = pssrSrchShortResults, pssrSrchShortResultEndPtr = (pssrSrchShortResults + uiSrchShortResultsLength); 
                pssrSrchShortResultsPtr < pssrSrchShortResultEndPtr; pssrSrchShortResultsPtr++ ) {
            dSumZxy += ((dMeanX - pssrSrchShortResultsPtr->uiDocumentID) / dStandardDeviationX) * ((dMeanY - pssrSrchShortResultsPtr->uiSortKey) / dStandardDeviationY);
        }
    }
    else if ( (uiSortType == SPI_SORT_TYPE_ULONG_ASC) || (uiSortType == SPI_SORT_TYPE_ULONG_DESC) ) {
        for ( pssrSrchShortResultsPtr = pssrSrchShortResults, pssrSrchShortResultEndPtr = (pssrSrchShortResults + uiSrchShortResultsLength); 
                pssrSrchShortResultsPtr < pssrSrchShortResultEndPtr; pssrSrchShortResultsPtr++ ) {
            dSumZxy += ((dMeanX - pssrSrchShortResultsPtr->uiDocumentID) / dStandardDeviationX) * ((dMeanY - pssrSrchShortResultsPtr->ulSortKey) / dStandardDeviationY);
        }
    }
    else {
        ASSERT(false);
    }
    

    /* Work out the correlation coefficient */
    dCorrelationCoefficient = dSumZxy / (uiSrchShortResultsLength - 1);

#endif    /* defined(SRCH_SHORT_RESULTS_ENABLE_DEVIATION_SCORE_METHOD) */
    
    

/* Pearson method */
#if defined(SRCH_SHORT_RESULTS_ENABLE_PEARSON_METHOD)

/*     iUtlLogInfo(UTL_LOG_CONTEXT, "Pearson Method"); */

    /* First step is to work out the total counts and total squared counts */
    if ( (uiSortType == SPI_SORT_TYPE_DOUBLE_ASC) || (uiSortType == SPI_SORT_TYPE_DOUBLE_DESC) ) {
    
        dMeanX = pssrSrchShortResults->uiDocumentID;
        dMeanY = pssrSrchShortResults->dSortKey;

        for ( pssrSrchShortResultsPtr = pssrSrchShortResults + 1, pssrSrchShortResultEndPtr = (pssrSrchShortResults + uiSrchShortResultsLength), uiI = 2; 
                pssrSrchShortResultsPtr < pssrSrchShortResultEndPtr; pssrSrchShortResultsPtr++, uiI++ ) {

            dSweep = (double)(uiI - 1) / uiI;
            dDeltaX = (double)pssrSrchShortResultsPtr->uiDocumentID - dMeanX;
            dDeltaY = pssrSrchShortResultsPtr->dSortKey - dMeanY;

            dSumSquaredX += dDeltaX * dDeltaX * dSweep;
            dSumSquaredY += dDeltaY * dDeltaY * dSweep;
            
            dSumCoproduct += dDeltaX * dDeltaY * dSweep;

            dMeanX += dDeltaX / uiI;
            dMeanY += dDeltaY / uiI;
        }
    }
    else if ( (uiSortType == SPI_SORT_TYPE_FLOAT_ASC) || (uiSortType == SPI_SORT_TYPE_FLOAT_DESC) ) {

        dMeanX = pssrSrchShortResults->uiDocumentID;
        dMeanY = (double)pssrSrchShortResults->fSortKey;

        for ( pssrSrchShortResultsPtr = pssrSrchShortResults + 1, pssrSrchShortResultEndPtr = (pssrSrchShortResults + uiSrchShortResultsLength), uiI = 2; 
                pssrSrchShortResultsPtr < pssrSrchShortResultEndPtr; pssrSrchShortResultsPtr++, uiI++ ) {

            dSweep = (double)(uiI - 1) / uiI;
            dDeltaX = (double)pssrSrchShortResultsPtr->uiDocumentID - dMeanX;
            dDeltaY = (double)pssrSrchShortResultsPtr->fSortKey - dMeanY;

            dSumSquaredX += dDeltaX * dDeltaX * dSweep;
            dSumSquaredY += dDeltaY * dDeltaY * dSweep;
            
            dSumCoproduct += dDeltaX * dDeltaY * dSweep;

            dMeanX += dDeltaX / uiI;
            dMeanY += dDeltaY / uiI;
        }
    }
    else if ( (uiSortType == SPI_SORT_TYPE_UINT_ASC) || (uiSortType == SPI_SORT_TYPE_UINT_DESC) ) {

        dMeanX = pssrSrchShortResults->uiDocumentID;
        dMeanY = (double)pssrSrchShortResults->uiSortKey;

        for ( pssrSrchShortResultsPtr = pssrSrchShortResults + 1, pssrSrchShortResultEndPtr = (pssrSrchShortResults + uiSrchShortResultsLength), uiI = 2; 
                pssrSrchShortResultsPtr < pssrSrchShortResultEndPtr; pssrSrchShortResultsPtr++, uiI++ ) {

            dSweep = (double)(uiI - 1) / uiI;
            dDeltaX = (double)pssrSrchShortResultsPtr->uiDocumentID - dMeanX;
            dDeltaY = (double)pssrSrchShortResultsPtr->uiSortKey - dMeanY;

            dSumSquaredX += dDeltaX * dDeltaX * dSweep;
            dSumSquaredY += dDeltaY * dDeltaY * dSweep;
            
            dSumCoproduct += dDeltaX * dDeltaY * dSweep;

            dMeanX += dDeltaX / uiI;
            dMeanY += dDeltaY / uiI;
        }
    }
    else if ( (uiSortType == SPI_SORT_TYPE_ULONG_ASC) || (uiSortType == SPI_SORT_TYPE_ULONG_DESC) ) {

        dMeanX = pssrSrchShortResults->uiDocumentID;
        dMeanY = (double)pssrSrchShortResults->ulSortKey;

        for ( pssrSrchShortResultsPtr = pssrSrchShortResults + 1, pssrSrchShortResultEndPtr = (pssrSrchShortResults + uiSrchShortResultsLength), uiI = 2; 
                pssrSrchShortResultsPtr < pssrSrchShortResultEndPtr; pssrSrchShortResultsPtr++, uiI++ ) {

            dSweep = (double)(uiI - 1) / uiI;
            dDeltaX = (double)pssrSrchShortResultsPtr->uiDocumentID - dMeanX;
            dDeltaY = (double)pssrSrchShortResultsPtr->ulSortKey - dMeanY;

            dSumSquaredX += dDeltaX * dDeltaX * dSweep;
            dSumSquaredY += dDeltaY * dDeltaY * dSweep;
            
            dSumCoproduct += dDeltaX * dDeltaY * dSweep;

            dMeanX += dDeltaX / uiI;
            dMeanY += dDeltaY / uiI;
        }
    }
    else {
        ASSERT(false);
    }

    /* Work out the variance */
    dVarianceX = dSumSquaredX / uiSrchShortResultsLength;
    dVarianceY = dSumSquaredY / uiSrchShortResultsLength;

    /* Work out the standard deviations */
    dStandardDeviationX = sqrt(dVarianceX);
    dStandardDeviationY = sqrt(dVarianceY);

    /* Work out the covariance */
    dCovariance = dSumCoproduct / uiSrchShortResultsLength;

    /* Work out the correlation coefficient */
    dCorrelationCoefficient = dCovariance / (dStandardDeviationX * dStandardDeviationY);

#endif    /* defined(SRCH_SHORT_RESULTS_ENABLE_PEARSON_METHOD) */



    /* Set the end time and work out how long this took */
    s_gettimeofday(&tvCorrelationEndTimeVal, NULL);
    UTL_DATE_DIFF_TIMEVAL(tvCorrelationStartTimeVal, tvCorrelationEndTimeVal, tvCorrelationDiffTimeVal);
    UTL_DATE_TIMEVAL_TO_SECONDS(tvCorrelationDiffTimeVal, dCorrelationTime);


/*     iUtlLogInfo(UTL_LOG_CONTEXT, "dCorrelationCoefficient: %f, time: %.2f milliseconds", dCorrelationCoefficient, dCorrelationTime); */


    /* Set the return pointer */
    *pdCorrelationCoefficient = dCorrelationCoefficient;
    
    
    return (SRCH_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchShortResultQuickSort()

    Purpose:    This function sorts the short results array using quicksort.

    Parameters: pssrSrchShortResults        pointer to an array of short result structures
                uiSrchShortResultsLength    number of entries in the array of short result structures
                uiSortType                  sort type

    Globals:    none

    Returns:    SRCH error code

*/
static int iSrchShortResultQuickSort
(
    struct srchShortResult *pssrSrchShortResults,
    unsigned int uiSrchShortResultsLength,
    unsigned int uiSortType
)
{

    int     iError = SRCH_NoError;


    ASSERT(pssrSrchShortResults != NULL);
    ASSERT(uiSrchShortResultsLength >= 0);


    switch ( uiSortType ) {

        case SRCH_SHORT_RESULTS_SORT_TYPE_DOUBLE_ASC:
            iError = iSrchShortResultQuickSortDoubleAsc(pssrSrchShortResults, 0, uiSrchShortResultsLength - 1);
            break;

        case SRCH_SHORT_RESULTS_SORT_TYPE_DOUBLE_DESC:
            iError = iSrchShortResultQuickSortDoubleDesc(pssrSrchShortResults, 0, uiSrchShortResultsLength - 1);
            break;

        case SRCH_SHORT_RESULTS_SORT_TYPE_FLOAT_ASC:
            iError = iSrchShortResultQuickSortFloatAsc(pssrSrchShortResults, 0, uiSrchShortResultsLength - 1);
            break;

        case SRCH_SHORT_RESULTS_SORT_TYPE_FLOAT_DESC:
            iError = iSrchShortResultQuickSortFloatDesc(pssrSrchShortResults, 0, uiSrchShortResultsLength - 1);
            break;
        
        case SRCH_SHORT_RESULTS_SORT_TYPE_UINT_ASC:
            iError = iSrchShortResultQuickSortUIntAsc(pssrSrchShortResults, 0, uiSrchShortResultsLength - 1);
            break;

        case SRCH_SHORT_RESULTS_SORT_TYPE_UINT_DESC:
            iError = iSrchShortResultQuickSortUIntDesc(pssrSrchShortResults, 0, uiSrchShortResultsLength - 1);
            break;
        
        case SRCH_SHORT_RESULTS_SORT_TYPE_ULLONG_ASC:
            iError = iSrchShortResultQuickSortULongAsc(pssrSrchShortResults, 0, uiSrchShortResultsLength - 1);
            break;

        case SRCH_SHORT_RESULTS_SORT_TYPE_ULONG_DESC:
            iError = iSrchShortResultQuickSortULongDesc(pssrSrchShortResults, 0, uiSrchShortResultsLength - 1);
            break;
        
        case SRCH_SHORT_RESULTS_SORT_TYPE_UCHAR_ASC:
            iError = iSrchShortResultQuickSortCharAsc(pssrSrchShortResults, 0, uiSrchShortResultsLength - 1);
            break;

        case SRCH_SHORT_RESULTS_SORT_TYPE_UCHAR_DESC:
            iError = iSrchShortResultQuickSortCharDesc(pssrSrchShortResults, 0, uiSrchShortResultsLength - 1);
            break;
        
        case SRCH_SHORT_RESULTS_SORT_TYPE_NO_SORT:
        case SRCH_SHORT_RESULTS_SORT_TYPE_UNKNOWN:
            break;
        
        default:
            ASSERT(false);
            break;
    }


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchShortResultQuickSortDoubleAsc()

    Purpose:    This functions sorts a short results array in ascending double order, 
                it implements a quicksort algorithm for speed. The reason we use
                this guy rather than qsort() is for sheer speed, we get a 
                2:1 to 3:1 speed increase because the condition is inline.

    Parameters: pssrSrchShortResults        pointer to a short result structure array
                iSrchShortResultsLeftIndex  left hand index in the short result structure array to sort 
                iSrchShortResultsRightIndex     right hand index in the short result structure array to sort 

    Globals:    none

    Returns:    SRCH error code

*/
static int iSrchShortResultQuickSortDoubleAsc
(
    struct srchShortResult *pssrSrchShortResults,
    int iSrchShortResultsLeftIndex,
    int iSrchShortResultsRightIndex
)
{

    int                         iSrchShortResultsLocalLeftIndex = 0;
    int                         iSrchShortResultsLocalRightIndex = 0;
    struct srchShortResult      *psrShortResultsRightIndexPtr = pssrSrchShortResults + iSrchShortResultsRightIndex;


    ASSERT(pssrSrchShortResults != NULL);
    ASSERT(iSrchShortResultsLeftIndex >= 0);
    ASSERT(iSrchShortResultsRightIndex >= -1);


    if ( iSrchShortResultsRightIndex > iSrchShortResultsLeftIndex ) {

        iSrchShortResultsLocalLeftIndex = iSrchShortResultsLeftIndex - 1; 
        iSrchShortResultsLocalRightIndex = iSrchShortResultsRightIndex;
        
        for ( ;; ) {

            while ( (++iSrchShortResultsLocalLeftIndex <= iSrchShortResultsRightIndex) && 
                    ((pssrSrchShortResults + iSrchShortResultsLocalLeftIndex)->dSortKey < psrShortResultsRightIndexPtr->dSortKey) );
            
            while ( (--iSrchShortResultsLocalRightIndex > iSrchShortResultsLeftIndex) && 
                    ((pssrSrchShortResults + iSrchShortResultsLocalRightIndex)->dSortKey > psrShortResultsRightIndexPtr->dSortKey) );

            if ( iSrchShortResultsLocalLeftIndex >= iSrchShortResultsLocalRightIndex ) {
                break;
            }
            
            SRCH_SHORT_RESULTS_SWAP_SRCH_SHORT_RESULTS_DOUBLE((pssrSrchShortResults + iSrchShortResultsLocalRightIndex), (pssrSrchShortResults + iSrchShortResultsLocalLeftIndex));
        }
        
        if ( iSrchShortResultsLocalLeftIndex != iSrchShortResultsRightIndex ) {
            SRCH_SHORT_RESULTS_SWAP_SRCH_SHORT_RESULTS_DOUBLE(psrShortResultsRightIndexPtr, (pssrSrchShortResults + iSrchShortResultsLocalLeftIndex));
        }
        
        iSrchShortResultQuickSortDoubleAsc(pssrSrchShortResults, iSrchShortResultsLeftIndex, iSrchShortResultsLocalLeftIndex - 1);
        iSrchShortResultQuickSortDoubleAsc(pssrSrchShortResults, iSrchShortResultsLocalLeftIndex + 1, iSrchShortResultsRightIndex);
    }
    

    return (SRCH_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchShortResultQuickSortDoubleDesc()

    Purpose:    This functions sorts a short results array in descending double order,
                it implements a quicksort algorithm for speed. The reason we use
                this guy rather than qsort() is for sheer speed, we get a 
                2:1 to 3:1 speed increase because the condition is inline.

    Parameters: pssrSrchShortResults            pointer to a short result structure array
                iSrchShortResultsLeftIndex      left hand index in the short result structure array to sort 
                iSrchShortResultsRightIndex     right hand index in the short result structure array to sort 

    Globals:    none

    Returns:    SRCH error code

*/
static int iSrchShortResultQuickSortDoubleDesc
(
    struct srchShortResult *pssrSrchShortResults,
    int iSrchShortResultsLeftIndex,
    int iSrchShortResultsRightIndex
)
{

    int                         iSrchShortResultsLocalLeftIndex = 0;
    int                         iSrchShortResultsLocalRightIndex = 0;
    struct srchShortResult      *psrShortResultsRightIndexPtr = pssrSrchShortResults + iSrchShortResultsRightIndex;


    ASSERT(pssrSrchShortResults != NULL);
    ASSERT(iSrchShortResultsLeftIndex >= 0);
    ASSERT(iSrchShortResultsRightIndex >= -1);


    if ( iSrchShortResultsRightIndex > iSrchShortResultsLeftIndex ) {

        iSrchShortResultsLocalLeftIndex = iSrchShortResultsLeftIndex - 1; 
        iSrchShortResultsLocalRightIndex = iSrchShortResultsRightIndex;
        
        for ( ;; ) {

            while ( (++iSrchShortResultsLocalLeftIndex <= iSrchShortResultsRightIndex) && 
                    ((pssrSrchShortResults + iSrchShortResultsLocalLeftIndex)->dSortKey > psrShortResultsRightIndexPtr->dSortKey) );

            while ( (--iSrchShortResultsLocalRightIndex > iSrchShortResultsLeftIndex) && 
                    ((pssrSrchShortResults + iSrchShortResultsLocalRightIndex)->dSortKey < psrShortResultsRightIndexPtr->dSortKey) );

            if ( iSrchShortResultsLocalLeftIndex >= iSrchShortResultsLocalRightIndex ) {
                break;
            }
            
            SRCH_SHORT_RESULTS_SWAP_SRCH_SHORT_RESULTS_DOUBLE((pssrSrchShortResults + iSrchShortResultsLocalRightIndex), (pssrSrchShortResults + iSrchShortResultsLocalLeftIndex));
        }

        if ( iSrchShortResultsLocalLeftIndex != iSrchShortResultsRightIndex ) {
            SRCH_SHORT_RESULTS_SWAP_SRCH_SHORT_RESULTS_DOUBLE(psrShortResultsRightIndexPtr, (pssrSrchShortResults + iSrchShortResultsLocalLeftIndex));
        }
        
        iSrchShortResultQuickSortDoubleDesc(pssrSrchShortResults, iSrchShortResultsLeftIndex, iSrchShortResultsLocalLeftIndex - 1);
        iSrchShortResultQuickSortDoubleDesc(pssrSrchShortResults, iSrchShortResultsLocalLeftIndex + 1, iSrchShortResultsRightIndex);
    }
    

    return (SRCH_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchShortResultQuickSortFloatAsc()

    Purpose:    This functions sorts a short results array in ascending float order, 
                it implements a quicksort algorithm for speed. The reason we use
                this guy rather than qsort() is for sheer speed, we get a 
                2:1 to 3:1 speed increase because the condition is inline.

    Parameters: pssrSrchShortResults            pointer to a short result structure array
                iSrchShortResultsLeftIndex      left hand index in the short result structure array to sort 
                iSrchShortResultsRightIndex     right hand index in the short result structure array to sort 

    Globals:    none

    Returns:    SRCH error code

*/
static int iSrchShortResultQuickSortFloatAsc
(
    struct srchShortResult *pssrSrchShortResults,
    int iSrchShortResultsLeftIndex,
    int iSrchShortResultsRightIndex
)
{

    int                         iSrchShortResultsLocalLeftIndex = 0;
    int                         iSrchShortResultsLocalRightIndex = 0;
    struct srchShortResult      *psrShortResultsRightIndexPtr = pssrSrchShortResults + iSrchShortResultsRightIndex;


    ASSERT(pssrSrchShortResults != NULL);
    ASSERT(iSrchShortResultsLeftIndex >= 0);
    ASSERT(iSrchShortResultsRightIndex >= -1);


    if ( iSrchShortResultsRightIndex > iSrchShortResultsLeftIndex ) {

        iSrchShortResultsLocalLeftIndex = iSrchShortResultsLeftIndex - 1; 
        iSrchShortResultsLocalRightIndex = iSrchShortResultsRightIndex;
        
        for ( ;; ) {

            while ( (++iSrchShortResultsLocalLeftIndex <= iSrchShortResultsRightIndex) && 
                    ((pssrSrchShortResults + iSrchShortResultsLocalLeftIndex)->fSortKey < psrShortResultsRightIndexPtr->fSortKey) );
            
            while ( (--iSrchShortResultsLocalRightIndex > iSrchShortResultsLeftIndex) && 
                    ((pssrSrchShortResults + iSrchShortResultsLocalRightIndex)->fSortKey > psrShortResultsRightIndexPtr->fSortKey) );

            if ( iSrchShortResultsLocalLeftIndex >= iSrchShortResultsLocalRightIndex ) {
                break;
            }
            
            SRCH_SHORT_RESULTS_SWAP_SRCH_SHORT_RESULTS_FLOAT((pssrSrchShortResults + iSrchShortResultsLocalRightIndex), (pssrSrchShortResults + iSrchShortResultsLocalLeftIndex));
        }
        
        if ( iSrchShortResultsLocalLeftIndex != iSrchShortResultsRightIndex ) {
            SRCH_SHORT_RESULTS_SWAP_SRCH_SHORT_RESULTS_FLOAT(psrShortResultsRightIndexPtr, (pssrSrchShortResults + iSrchShortResultsLocalLeftIndex));
        }
        
        iSrchShortResultQuickSortFloatAsc(pssrSrchShortResults, iSrchShortResultsLeftIndex, iSrchShortResultsLocalLeftIndex - 1);
        iSrchShortResultQuickSortFloatAsc(pssrSrchShortResults, iSrchShortResultsLocalLeftIndex + 1, iSrchShortResultsRightIndex);
    }

    
    return (SRCH_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchShortResultQuickSortFloatDesc()

    Purpose:    This functions sorts a short results array in descending float order, 
                it implements a quicksort algorithm for speed. The reason we use
                this guy rather than qsort() is for sheer speed, we get a 
                2:1 to 3:1 speed increase because the condition is inline.

    Parameters: pssrSrchShortResults            pointer to a short result structure array
                iSrchShortResultsLeftIndex      left hand index in the short result structure array to sort 
                iSrchShortResultsRightIndex     right hand index in the short result structure array to sort 

    Globals:    none

    Returns:    SRCH error code

*/
static int iSrchShortResultQuickSortFloatDesc
(
    struct srchShortResult *pssrSrchShortResults,
    int iSrchShortResultsLeftIndex,
    int iSrchShortResultsRightIndex
)
{

    int                         iSrchShortResultsLocalLeftIndex = 0;
    int                         iSrchShortResultsLocalRightIndex = 0;
    struct srchShortResult      *psrShortResultsRightIndexPtr = pssrSrchShortResults + iSrchShortResultsRightIndex;


    ASSERT(pssrSrchShortResults != NULL);
    ASSERT(iSrchShortResultsLeftIndex >= 0);
    ASSERT(iSrchShortResultsRightIndex >= -1);


    if ( iSrchShortResultsRightIndex > iSrchShortResultsLeftIndex ) {

        iSrchShortResultsLocalLeftIndex = iSrchShortResultsLeftIndex - 1; 
        iSrchShortResultsLocalRightIndex = iSrchShortResultsRightIndex;
        
        for ( ;; ) {

            while ( (++iSrchShortResultsLocalLeftIndex <= iSrchShortResultsRightIndex) && 
                    ((pssrSrchShortResults + iSrchShortResultsLocalLeftIndex)->fSortKey > psrShortResultsRightIndexPtr->fSortKey) );
            
            while ( (--iSrchShortResultsLocalRightIndex > iSrchShortResultsLeftIndex) && 
                    ((pssrSrchShortResults + iSrchShortResultsLocalRightIndex)->fSortKey < psrShortResultsRightIndexPtr->fSortKey) );

            if ( iSrchShortResultsLocalLeftIndex >= iSrchShortResultsLocalRightIndex ) {
                break;
            }
            
            SRCH_SHORT_RESULTS_SWAP_SRCH_SHORT_RESULTS_FLOAT((pssrSrchShortResults + iSrchShortResultsLocalRightIndex), (pssrSrchShortResults + iSrchShortResultsLocalLeftIndex));
        }

        if ( iSrchShortResultsLocalLeftIndex != iSrchShortResultsRightIndex ) {
            SRCH_SHORT_RESULTS_SWAP_SRCH_SHORT_RESULTS_FLOAT(psrShortResultsRightIndexPtr, (pssrSrchShortResults + iSrchShortResultsLocalLeftIndex));
        }
        
        iSrchShortResultQuickSortFloatDesc(pssrSrchShortResults, iSrchShortResultsLeftIndex, iSrchShortResultsLocalLeftIndex - 1);
        iSrchShortResultQuickSortFloatDesc(pssrSrchShortResults, iSrchShortResultsLocalLeftIndex + 1, iSrchShortResultsRightIndex);
    }

    
    return (SRCH_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchShortResultQuickSortUIntAsc()

    Purpose:    This functions sorts a short results array in ascending unsigned integer order, 
                it implements a quicksort algorithm for speed. The reason we use
                this guy rather than qsort() is for sheer speed, we get a 
                2:1 to 3:1 speed increase because the condition is inline.

    Parameters: pssrSrchShortResults            pointer to a short result structure array
                iSrchShortResultsLeftIndex      left hand index in the short result structure array to sort 
                iSrchShortResultsRightIndex     right hand index in the short result structure array to sort 

    Globals:    none

    Returns:    SRCH error code

*/
static int iSrchShortResultQuickSortUIntAsc
(
    struct srchShortResult *pssrSrchShortResults,
    int iSrchShortResultsLeftIndex,
    int iSrchShortResultsRightIndex
)
{

    int                         iSrchShortResultsLocalLeftIndex = 0;
    int                         iSrchShortResultsLocalRightIndex = 0;
    struct srchShortResult      *psrShortResultsRightIndexPtr = pssrSrchShortResults + iSrchShortResultsRightIndex;


    ASSERT(pssrSrchShortResults != NULL);
    ASSERT(iSrchShortResultsLeftIndex >= 0);
    ASSERT(iSrchShortResultsRightIndex >= -1);


    if ( iSrchShortResultsRightIndex > iSrchShortResultsLeftIndex ) {

        iSrchShortResultsLocalLeftIndex = iSrchShortResultsLeftIndex - 1; 
        iSrchShortResultsLocalRightIndex = iSrchShortResultsRightIndex;
        
        for ( ;; ) {
            
            while ( (++iSrchShortResultsLocalLeftIndex <= iSrchShortResultsRightIndex) && 
                    ((pssrSrchShortResults + iSrchShortResultsLocalLeftIndex)->uiSortKey < psrShortResultsRightIndexPtr->uiSortKey) );
            
            while ( (--iSrchShortResultsLocalRightIndex > iSrchShortResultsLeftIndex) && 
                    ((pssrSrchShortResults + iSrchShortResultsLocalRightIndex)->uiSortKey > psrShortResultsRightIndexPtr->uiSortKey) );

            if ( iSrchShortResultsLocalLeftIndex >= iSrchShortResultsLocalRightIndex ) {
                break;
            }
            
            SRCH_SHORT_RESULTS_SWAP_SRCH_SHORT_RESULTS_UINT((pssrSrchShortResults + iSrchShortResultsLocalRightIndex), (pssrSrchShortResults + iSrchShortResultsLocalLeftIndex));
        }
        
        if ( iSrchShortResultsLocalLeftIndex != iSrchShortResultsRightIndex ) {
            SRCH_SHORT_RESULTS_SWAP_SRCH_SHORT_RESULTS_UINT(psrShortResultsRightIndexPtr, (pssrSrchShortResults + iSrchShortResultsLocalLeftIndex));
        }

        iSrchShortResultQuickSortUIntAsc(pssrSrchShortResults, iSrchShortResultsLeftIndex, iSrchShortResultsLocalLeftIndex - 1);
        iSrchShortResultQuickSortUIntAsc(pssrSrchShortResults, iSrchShortResultsLocalLeftIndex + 1, iSrchShortResultsRightIndex);
    }


    return (SRCH_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchShortResultQuickSortUIntDesc()

    Purpose:    This functions sorts a short results array in descending unsigned integer order, 
                it implements a quicksort algorithm for speed. The reason we use
                this guy rather than qsort() is for sheer speed, we get a 
                2:1 to 3:1 speed increase because the condition is inline.

    Parameters: pssrSrchShortResults            pointer to a short result structure array
                iSrchShortResultsLeftIndex      left hand index in the short result structure array to sort 
                iSrchShortResultsRightIndex     right hand index in the short result structure array to sort 

    Globals:    none

    Returns:    SRCH error code

*/
static int iSrchShortResultQuickSortUIntDesc
(
    struct srchShortResult *pssrSrchShortResults,
    int iSrchShortResultsLeftIndex,
    int iSrchShortResultsRightIndex
)
{

    int                         iSrchShortResultsLocalLeftIndex = 0;
    int                         iSrchShortResultsLocalRightIndex = 0;
    struct srchShortResult      *psrShortResultsRightIndexPtr = pssrSrchShortResults + iSrchShortResultsRightIndex;


    ASSERT(pssrSrchShortResults != NULL);
    ASSERT(iSrchShortResultsLeftIndex >= 0);
    ASSERT(iSrchShortResultsRightIndex >= -1);


    if ( iSrchShortResultsRightIndex > iSrchShortResultsLeftIndex ) {

        iSrchShortResultsLocalLeftIndex = iSrchShortResultsLeftIndex - 1; 
        iSrchShortResultsLocalRightIndex = iSrchShortResultsRightIndex;
        
        for ( ;; ) {

            while ( (++iSrchShortResultsLocalLeftIndex <= iSrchShortResultsRightIndex) && 
                    ((pssrSrchShortResults + iSrchShortResultsLocalLeftIndex)->uiSortKey > psrShortResultsRightIndexPtr->uiSortKey) );
            
            while ( (--iSrchShortResultsLocalRightIndex > iSrchShortResultsLeftIndex) && 
                    ((pssrSrchShortResults + iSrchShortResultsLocalRightIndex)->uiSortKey < psrShortResultsRightIndexPtr->uiSortKey) );

            if ( iSrchShortResultsLocalLeftIndex >= iSrchShortResultsLocalRightIndex ) {
                break;
            }
            
            SRCH_SHORT_RESULTS_SWAP_SRCH_SHORT_RESULTS_UINT((pssrSrchShortResults + iSrchShortResultsLocalRightIndex), (pssrSrchShortResults + iSrchShortResultsLocalLeftIndex));
        }
        
        if ( iSrchShortResultsLocalLeftIndex != iSrchShortResultsRightIndex ) {
            SRCH_SHORT_RESULTS_SWAP_SRCH_SHORT_RESULTS_UINT(psrShortResultsRightIndexPtr, (pssrSrchShortResults + iSrchShortResultsLocalLeftIndex));
        }

        iSrchShortResultQuickSortUIntDesc(pssrSrchShortResults, iSrchShortResultsLeftIndex, iSrchShortResultsLocalLeftIndex - 1);
        iSrchShortResultQuickSortUIntDesc(pssrSrchShortResults, iSrchShortResultsLocalLeftIndex + 1, iSrchShortResultsRightIndex);
    }


    return (SRCH_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchShortResultQuickSortULongAsc()

    Purpose:    This functions sorts a short results array in ascending unsigned long order, 
                it implements a quicksort algorithm for speed. The reason we use
                this guy rather than qsort() is for sheer speed, we get a 
                2:1 to 3:1 speed increase because the condition is inline.

    Parameters: pssrSrchShortResults            pointer to a short result structure array
                iSrchShortResultsLeftIndex      left hand index in the short result structure array to sort 
                iSrchShortResultsRightIndex     right hand index in the short result structure array to sort 

    Globals:    none

    Returns:    SRCH error code

*/
static int iSrchShortResultQuickSortULongAsc
(
    struct srchShortResult *pssrSrchShortResults,
    int iSrchShortResultsLeftIndex,
    int iSrchShortResultsRightIndex
)
{

    int                         iSrchShortResultsLocalLeftIndex = 0;
    int                         iSrchShortResultsLocalRightIndex = 0;
    struct srchShortResult      *psrShortResultsRightIndexPtr = pssrSrchShortResults + iSrchShortResultsRightIndex;


    ASSERT(pssrSrchShortResults != NULL);
    ASSERT(iSrchShortResultsLeftIndex >= 0);
    ASSERT(iSrchShortResultsRightIndex >= -1);


    if ( iSrchShortResultsRightIndex > iSrchShortResultsLeftIndex ) {

        iSrchShortResultsLocalLeftIndex = iSrchShortResultsLeftIndex - 1; 
        iSrchShortResultsLocalRightIndex = iSrchShortResultsRightIndex;
        
        for ( ;; ) {
            
            while ( (++iSrchShortResultsLocalLeftIndex <= iSrchShortResultsRightIndex) && 
                    ((pssrSrchShortResults + iSrchShortResultsLocalLeftIndex)->ulSortKey < psrShortResultsRightIndexPtr->ulSortKey) );
            
            while ( (--iSrchShortResultsLocalRightIndex > iSrchShortResultsLeftIndex) && 
                    ((pssrSrchShortResults + iSrchShortResultsLocalRightIndex)->ulSortKey > psrShortResultsRightIndexPtr->ulSortKey) );

            if ( iSrchShortResultsLocalLeftIndex >= iSrchShortResultsLocalRightIndex ) {
                break;
            }
            
            SRCH_SHORT_RESULTS_SWAP_SRCH_SHORT_RESULTS_ULLONG((pssrSrchShortResults + iSrchShortResultsLocalRightIndex), (pssrSrchShortResults + iSrchShortResultsLocalLeftIndex));
        }
        
        if ( iSrchShortResultsLocalLeftIndex != iSrchShortResultsRightIndex ) {
            SRCH_SHORT_RESULTS_SWAP_SRCH_SHORT_RESULTS_ULLONG(psrShortResultsRightIndexPtr, (pssrSrchShortResults + iSrchShortResultsLocalLeftIndex));
        }

        iSrchShortResultQuickSortULongAsc(pssrSrchShortResults, iSrchShortResultsLeftIndex, iSrchShortResultsLocalLeftIndex - 1);
        iSrchShortResultQuickSortULongAsc(pssrSrchShortResults, iSrchShortResultsLocalLeftIndex + 1, iSrchShortResultsRightIndex);
    }


    return (SRCH_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchShortResultQuickSortULongDesc()

    Purpose:    This functions sorts a short results array in descending unsigned long order, 
                it implements a quicksort algorithm for speed. The reason we use
                this guy rather than qsort() is for sheer speed, we get a 
                2:1 to 3:1 speed increase because the condition is inline.

    Parameters: pssrSrchShortResults            pointer to a short result structure array
                iSrchShortResultsLeftIndex      left hand index in the short result structure array to sort 
                iSrchShortResultsRightIndex     right hand index in the short result structure array to sort 

    Globals:    none

    Returns:    SRCH error code

*/
static int iSrchShortResultQuickSortULongDesc
(
    struct srchShortResult *pssrSrchShortResults,
    int iSrchShortResultsLeftIndex,
    int iSrchShortResultsRightIndex
)
{

    int                         iSrchShortResultsLocalLeftIndex = 0;
    int                         iSrchShortResultsLocalRightIndex = 0;
    struct srchShortResult      *psrShortResultsRightIndexPtr = pssrSrchShortResults + iSrchShortResultsRightIndex;


    ASSERT(pssrSrchShortResults != NULL);
    ASSERT(iSrchShortResultsLeftIndex >= 0);
    ASSERT(iSrchShortResultsRightIndex >= -1);


    if ( iSrchShortResultsRightIndex > iSrchShortResultsLeftIndex ) {

        iSrchShortResultsLocalLeftIndex = iSrchShortResultsLeftIndex - 1; 
        iSrchShortResultsLocalRightIndex = iSrchShortResultsRightIndex;
        
        for ( ;; ) {

            while ( (++iSrchShortResultsLocalLeftIndex <= iSrchShortResultsRightIndex) && 
                    ((pssrSrchShortResults + iSrchShortResultsLocalLeftIndex)->ulSortKey > psrShortResultsRightIndexPtr->ulSortKey) );
            
            while ( (--iSrchShortResultsLocalRightIndex > iSrchShortResultsLeftIndex) && 
                    ((pssrSrchShortResults + iSrchShortResultsLocalRightIndex)->ulSortKey < psrShortResultsRightIndexPtr->ulSortKey) );

            if ( iSrchShortResultsLocalLeftIndex >= iSrchShortResultsLocalRightIndex ) {
                break;
            }
            
            SRCH_SHORT_RESULTS_SWAP_SRCH_SHORT_RESULTS_ULLONG((pssrSrchShortResults + iSrchShortResultsLocalRightIndex), (pssrSrchShortResults + iSrchShortResultsLocalLeftIndex));
        }
        
        if ( iSrchShortResultsLocalLeftIndex != iSrchShortResultsRightIndex ) {
            SRCH_SHORT_RESULTS_SWAP_SRCH_SHORT_RESULTS_ULLONG(psrShortResultsRightIndexPtr, (pssrSrchShortResults + iSrchShortResultsLocalLeftIndex));
        }

        iSrchShortResultQuickSortULongDesc(pssrSrchShortResults, iSrchShortResultsLeftIndex, iSrchShortResultsLocalLeftIndex - 1);
        iSrchShortResultQuickSortULongDesc(pssrSrchShortResults, iSrchShortResultsLocalLeftIndex + 1, iSrchShortResultsRightIndex);
    }


    return (SRCH_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchShortResultQuickSortCharAsc()

    Purpose:    This functions sorts a short results array in ascending character order, 
                it implements a quicksort algorithm for speed. The reason we use
                this guy rather than qsort() is for sheer speed, we get a 
                2:1 to 3:1 speed increase because the condition is inline.

    Parameters: pssrSrchShortResults            pointer to a short result structure array
                iSrchShortResultsLeftIndex      left hand index in the short result structure array to sort 
                iSrchShortResultsRightIndex     right hand index in the short result structure array to sort 

    Globals:    none

    Returns:    SRCH error code

*/
static int iSrchShortResultQuickSortCharAsc
(
    struct srchShortResult *pssrSrchShortResults,
    int iSrchShortResultsLeftIndex,
    int iSrchShortResultsRightIndex
)
{

    int                         iSrchShortResultsLocalLeftIndex = 0;
    int                         iSrchShortResultsLocalRightIndex = 0;
    struct srchShortResult      *psrShortResultsRightIndexPtr = pssrSrchShortResults + iSrchShortResultsRightIndex;


    ASSERT(pssrSrchShortResults != NULL);
    ASSERT(iSrchShortResultsLeftIndex >= 0);
    ASSERT(iSrchShortResultsRightIndex >= -1);


    if ( iSrchShortResultsRightIndex > iSrchShortResultsLeftIndex ) {

        iSrchShortResultsLocalLeftIndex = iSrchShortResultsLeftIndex - 1; 
        iSrchShortResultsLocalRightIndex = iSrchShortResultsRightIndex;
        
        for ( ;; ) {

            while ( (++iSrchShortResultsLocalLeftIndex <= iSrchShortResultsRightIndex) && 
                    (s_strcoll((pssrSrchShortResults + iSrchShortResultsLocalLeftIndex)->pucSortKey, psrShortResultsRightIndexPtr->pucSortKey) < 0) );

            while ( (--iSrchShortResultsLocalRightIndex > iSrchShortResultsLeftIndex) && 
                    (s_strcoll((pssrSrchShortResults + iSrchShortResultsLocalRightIndex)->pucSortKey, psrShortResultsRightIndexPtr->pucSortKey) > 0) );

            if ( iSrchShortResultsLocalLeftIndex >= iSrchShortResultsLocalRightIndex ) {
                break;
            }
            
            SRCH_SHORT_RESULTS_SWAP_SRCH_SHORT_RESULTS_UCHAR((pssrSrchShortResults + iSrchShortResultsLocalRightIndex), (pssrSrchShortResults + iSrchShortResultsLocalLeftIndex));
        }

        if ( iSrchShortResultsLocalLeftIndex != iSrchShortResultsRightIndex ) {
            SRCH_SHORT_RESULTS_SWAP_SRCH_SHORT_RESULTS_UCHAR(psrShortResultsRightIndexPtr, (pssrSrchShortResults + iSrchShortResultsLocalLeftIndex));
        }
        
        iSrchShortResultQuickSortCharAsc(pssrSrchShortResults, iSrchShortResultsLeftIndex, iSrchShortResultsLocalLeftIndex - 1);
        iSrchShortResultQuickSortCharAsc(pssrSrchShortResults, iSrchShortResultsLocalLeftIndex + 1, iSrchShortResultsRightIndex);
    }
    

    return (SRCH_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchShortResultQuickSortCharDesc()

    Purpose:    This functions sorts a short results array in descending character order, 
                it implements a quicksort algorithm for speed. The reason we use
                this guy rather than qsort() is for sheer speed, we get a 
                2:1 to 3:1 speed increase because the condition is inline.

    Parameters: pssrSrchShortResults            pointer to a short result structure array
                iSrchShortResultsLeftIndex      left hand index in the short result structure array to sort 
                iSrchShortResultsRightIndex     right hand index in the short result structure array to sort 

    Globals:    none

    Returns:    SRCH error code

*/
static int iSrchShortResultQuickSortCharDesc
(
    struct srchShortResult *pssrSrchShortResults,
    int iSrchShortResultsLeftIndex,
    int iSrchShortResultsRightIndex
)
{

    int                         iSrchShortResultsLocalLeftIndex = 0;
    int                         iSrchShortResultsLocalRightIndex = 0;
    struct srchShortResult      *psrShortResultsRightIndexPtr = pssrSrchShortResults + iSrchShortResultsRightIndex;


    ASSERT(pssrSrchShortResults != NULL);
    ASSERT(iSrchShortResultsLeftIndex >= 0);
    ASSERT(iSrchShortResultsRightIndex >= -1);


    if ( iSrchShortResultsRightIndex > iSrchShortResultsLeftIndex ) {

        iSrchShortResultsLocalLeftIndex = iSrchShortResultsLeftIndex - 1; 
        iSrchShortResultsLocalRightIndex = iSrchShortResultsRightIndex;
        
        for ( ;; ) {

            while ( (++iSrchShortResultsLocalLeftIndex <= iSrchShortResultsRightIndex) && 
                    (s_strcoll((pssrSrchShortResults + iSrchShortResultsLocalLeftIndex)->pucSortKey, psrShortResultsRightIndexPtr->pucSortKey) > 0) );
            
            while ( (--iSrchShortResultsLocalRightIndex > iSrchShortResultsLeftIndex) && 
                    (s_strcoll((pssrSrchShortResults + iSrchShortResultsLocalRightIndex)->pucSortKey, psrShortResultsRightIndexPtr->pucSortKey) < 0) );

            if ( iSrchShortResultsLocalLeftIndex >= iSrchShortResultsLocalRightIndex ) {
                break;
            }
            
            SRCH_SHORT_RESULTS_SWAP_SRCH_SHORT_RESULTS_UCHAR((pssrSrchShortResults + iSrchShortResultsLocalRightIndex), (pssrSrchShortResults + iSrchShortResultsLocalLeftIndex));
        }

        if ( iSrchShortResultsLocalLeftIndex != iSrchShortResultsRightIndex ) {
            SRCH_SHORT_RESULTS_SWAP_SRCH_SHORT_RESULTS_UCHAR(psrShortResultsRightIndexPtr, (pssrSrchShortResults + iSrchShortResultsLocalLeftIndex));
        }
        
        iSrchShortResultQuickSortCharDesc(pssrSrchShortResults, iSrchShortResultsLeftIndex, iSrchShortResultsLocalLeftIndex - 1);
        iSrchShortResultQuickSortCharDesc(pssrSrchShortResults, iSrchShortResultsLocalLeftIndex + 1, iSrchShortResultsRightIndex);
    }

    
    return (SRCH_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchShortResultRadixSort()

    Purpose:    This function sorts the short results array using radix.

    Parameters: ppssrSrchShortResults       pointer to a short result structure array (return pointer as well)
                uiSrchShortResultsLength    number of entries in the array of short result structures
                uiSortType                  sort type

    Globals:    none

    Returns:    SRCH error code

*/
int iSrchShortResultRadixSort
(
    struct srchShortResult **ppssrSrchShortResults,
    unsigned int uiSrchShortResultsLength,
    unsigned int uiSortType
)
{

    int     iError = SRCH_NoError;


    ASSERT(ppssrSrchShortResults != NULL);
    ASSERT(*ppssrSrchShortResults != NULL);
    ASSERT(uiSrchShortResultsLength >= 0);


    switch ( uiSortType ) {

        case SRCH_SHORT_RESULTS_SORT_TYPE_DOUBLE_ASC:
            iError = iSrchShortResultRadixSortDouble(ppssrSrchShortResults, uiSrchShortResultsLength, SRCH_SHORT_RESULTS_SORT_TYPE_DOUBLE_ASC);
            break;

        case SRCH_SHORT_RESULTS_SORT_TYPE_DOUBLE_DESC:
            iError = iSrchShortResultRadixSortDouble(ppssrSrchShortResults, uiSrchShortResultsLength, SRCH_SHORT_RESULTS_SORT_TYPE_DOUBLE_DESC);
            break;

        case SRCH_SHORT_RESULTS_SORT_TYPE_FLOAT_ASC:
            iError = iSrchShortResultRadixSortFloat(ppssrSrchShortResults, uiSrchShortResultsLength, SRCH_SHORT_RESULTS_SORT_TYPE_FLOAT_ASC);
            break;

        case SRCH_SHORT_RESULTS_SORT_TYPE_FLOAT_DESC:
            iError = iSrchShortResultRadixSortFloat(ppssrSrchShortResults, uiSrchShortResultsLength, SRCH_SHORT_RESULTS_SORT_TYPE_FLOAT_DESC);
            break;
        
        case SRCH_SHORT_RESULTS_SORT_TYPE_UINT_ASC:
            iError = iSrchShortResultRadixSortUInt(ppssrSrchShortResults, uiSrchShortResultsLength, SRCH_SHORT_RESULTS_SORT_TYPE_UINT_ASC);
            break;

        case SRCH_SHORT_RESULTS_SORT_TYPE_UINT_DESC:
            iError = iSrchShortResultRadixSortUInt(ppssrSrchShortResults, uiSrchShortResultsLength, SRCH_SHORT_RESULTS_SORT_TYPE_UINT_DESC);
            break;
        
        case SRCH_SHORT_RESULTS_SORT_TYPE_ULLONG_ASC:
            iError = iSrchShortResultRadixSortULong(ppssrSrchShortResults, uiSrchShortResultsLength, SRCH_SHORT_RESULTS_SORT_TYPE_ULLONG_ASC);
            break;

        case SRCH_SHORT_RESULTS_SORT_TYPE_ULONG_DESC:
            iError = iSrchShortResultRadixSortULong(ppssrSrchShortResults, uiSrchShortResultsLength, SRCH_SHORT_RESULTS_SORT_TYPE_ULONG_DESC);
            break;
        
        case SRCH_SHORT_RESULTS_SORT_TYPE_UCHAR_ASC:
            ASSERT(false);
            break;

        case SRCH_SHORT_RESULTS_SORT_TYPE_UCHAR_DESC:
            ASSERT(false);
            break;
        
        case SRCH_SHORT_RESULTS_SORT_TYPE_NO_SORT:
        case SRCH_SHORT_RESULTS_SORT_TYPE_UNKNOWN:
            break;
        
        default:
            ASSERT(false);
            break;
    }


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchShortResultRadixSortDouble()

    Purpose:    This functions sorts a short results array in double order, 
                it implements a radix algorithm.
                
                NOTE: this does not handle negative values yet.

    Parameters: ppssrSrchShortResults       pointer to a short result structure array (return pointer as well)
                uiSrchShortResultsLength    length of the short result structure array
                uiSortOrder                 sort order

    Globals:    none

    Returns:    SRCH error code

*/
static int iSrchShortResultRadixSortDouble
(
    struct srchShortResult **ppssrSrchShortResults,
    unsigned int uiSrchShortResultsLength,
    unsigned int uiSortOrder
)
{

    struct srchShortResult      *pssrSrchShortResultsDestination = NULL;
    unsigned int                uiRadix = 0;


    ASSERT(ppssrSrchShortResults != NULL);
    ASSERT(uiSrchShortResultsLength > 0);
    ASSERT((uiSortOrder == SRCH_SHORT_RESULTS_SORT_TYPE_DOUBLE_ASC) || (uiSortOrder == SRCH_SHORT_RESULTS_SORT_TYPE_DOUBLE_DESC));

    
    /* Allocate space for the destination, this starts off being the destination, 
    ** and will alternatively become the source and the destination
    */
    if ( (pssrSrchShortResultsDestination = (struct srchShortResult *)s_malloc((size_t)(sizeof(struct srchShortResult) * uiSrchShortResultsLength))) == NULL ) {
        return (SRCH_MemError);
    }

    
    /* Loop over each radix (byte) in the number, starting with the least significant byte */
    for ( uiRadix = 0; uiRadix < sizeof(unsigned long); uiRadix++ ) {

        unsigned int                puiCount[256];
        unsigned int                *puiCountPtr = NULL;
        unsigned int                *puiCountEndPtr = NULL;
        struct srchShortResult      *pssrSrchShortResultsSource = *ppssrSrchShortResults;
        struct srchShortResult      *pssrSrchShortResultsSourcePtr = NULL;
        struct srchShortResult      *pssrSrchShortResultsSourceEndPtr = NULL;
        struct srchShortResult      *pssrSrchShortResultsPtr = NULL;
        unsigned int                uiRadixBits = uiRadix * 8;
        boolean                     bSkipRadix = false;
        unsigned long               ulTotalCount = 0, ulCount = 0;


        /* Clear the count array */
        for ( puiCountPtr = puiCount, puiCountEndPtr = puiCount + 256; puiCountPtr < puiCountEndPtr; puiCountPtr++ ) {
            *puiCountPtr = 0;
        }
    

        /* Count occurences of every byte value */
        for ( pssrSrchShortResultsSourcePtr = pssrSrchShortResultsSource, pssrSrchShortResultsSourceEndPtr = pssrSrchShortResultsSource + uiSrchShortResultsLength; 
                pssrSrchShortResultsSourcePtr < pssrSrchShortResultsSourceEndPtr; pssrSrchShortResultsSourcePtr++ ) {

            /* Get the index into the count array for this byte, ie a byte 
            ** containing a value of 20 will point to the counter array element 20.
            ** Note the cast to a unsigned long, as a double is 8 bytes long on both 32 and 64 bit platforms
            */
            puiCountPtr = puiCount + (((*((unsigned long *)&pssrSrchShortResultsSourcePtr->dSortKey)) >> uiRadixBits) & 0xFF);

            /* Increment the counter in the counter array element */
            ++(*puiCountPtr);
        }


        /* Check to see if we can skip this radix, we can skip it if all the bytes contain the same value */
        for ( puiCountPtr = puiCount, puiCountEndPtr = puiCount + 256, bSkipRadix = false; puiCountPtr < puiCountEndPtr; puiCountPtr++ ) {

            /* All the bytes contain the same value if the value contained in the 
            ** counter is the same as the number of elements to sort 
            */
            if ( *puiCountPtr == uiSrchShortResultsLength ) {
                bSkipRadix = true;
            }
        }
        
        /* Skip if we were flagged to do so */
        if ( bSkipRadix == true ) {
/*            iUtlLogDebug(UTL_LOG_CONTEXT, "Skipping radix: %u", uiRadix); */
            continue;
        }

/*        iUtlLogDebug(UTL_LOG_CONTEXT, "Processing radix: %u", uiRadix); */

        /* Select the transform based on the sort order requested */
        if ( uiSortOrder == SRCH_SHORT_RESULTS_SORT_TYPE_DOUBLE_ASC ) {

            /* Transform count into index by summing elements and storing into same array */
            for ( puiCountPtr = puiCount, puiCountEndPtr = puiCount + 256, ulTotalCount = 0; puiCountPtr < puiCountEndPtr; puiCountPtr++ ) {
                ulCount = *puiCountPtr;
                *puiCountPtr = ulTotalCount;
                ulTotalCount += ulCount;
            }
        }
        else if ( uiSortOrder == SRCH_SHORT_RESULTS_SORT_TYPE_DOUBLE_DESC ) {

            /* Transform count into index by summing elements and storing into same array */
            for ( puiCountPtr = puiCount + 255, ulTotalCount = 0; puiCountPtr >= puiCount; puiCountPtr-- ) {
                ulCount = *puiCountPtr;
                *puiCountPtr = ulTotalCount;
                ulTotalCount += ulCount;
            }
        }


        /* Fill destination with the right values in the right place */
        for ( pssrSrchShortResultsSourcePtr = pssrSrchShortResultsSource, pssrSrchShortResultsSourceEndPtr = pssrSrchShortResultsSource + uiSrchShortResultsLength; 
                pssrSrchShortResultsSourcePtr < pssrSrchShortResultsSourceEndPtr; pssrSrchShortResultsSourcePtr++ ) {
            
            /* Get the index into the count array for this byte, ie a byte 
            ** containing a value of 20 will point to the counter array element 20.
            ** Note the cast to a unsigned long, as a float is 4 bytes long on both 32 and 64 bit platforms
            */
            puiCountPtr = puiCount + (((*((unsigned long *)&pssrSrchShortResultsSourcePtr->dSortKey)) >> uiRadixBits) & 0xFF);
            
            /* Copy the short result, note that the counter array element points to the index in the destination */
            pssrSrchShortResultsPtr = pssrSrchShortResultsDestination + *puiCountPtr;
            SRCH_SHORT_RESULTS_COPY_SRCH_SHORT_RESULTS_DOUBLE(pssrSrchShortResultsPtr, pssrSrchShortResultsSourcePtr);
    
            /* Increment the counter in the counter array element */
            ++(*puiCountPtr);
        }


        /* Swap the source and destination, ie the destination becomes the new source, 
        ** and the source becomes the new destination, note that this sets up the source
        ** and destination for the next loop or the return (we return the source)
        */
        pssrSrchShortResultsPtr = *ppssrSrchShortResults;
        *ppssrSrchShortResults = pssrSrchShortResultsDestination;
        pssrSrchShortResultsDestination = pssrSrchShortResultsPtr;
    }

    
    /* Free the destination */
    s_free(pssrSrchShortResultsDestination);


    return (SRCH_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchShortResultRadixSortFloat()

    Purpose:    This functions sorts a short results array in float order, 
                it implements a radix algorithm.
                
                NOTE: this does not handle negative values yet.

    Parameters: ppssrSrchShortResults       pointer to a short result structure array (return pointer as well)
                uiSrchShortResultsLength    length of the short result structure array
                uiSortOrder                 sort order

    Globals:    none

    Returns:    SRCH error code

*/
static int iSrchShortResultRadixSortFloat
(
    struct srchShortResult **ppssrSrchShortResults,
    unsigned int uiSrchShortResultsLength,
    unsigned int uiSortOrder
)
{

    struct srchShortResult      *pssrSrchShortResultsDestination = NULL;
    unsigned int                uiRadix = 0;


    ASSERT(ppssrSrchShortResults != NULL);
    ASSERT(uiSrchShortResultsLength > 0);
    ASSERT((uiSortOrder == SRCH_SHORT_RESULTS_SORT_TYPE_FLOAT_ASC) || (uiSortOrder == SRCH_SHORT_RESULTS_SORT_TYPE_FLOAT_DESC));

    
    /* Allocate space for the destination, this starts off being the destination, 
    ** and will alternatively become the source and the destination
    */
    if ( (pssrSrchShortResultsDestination = (struct srchShortResult *)s_malloc((size_t)(sizeof(struct srchShortResult) * uiSrchShortResultsLength))) == NULL ) {
        return (SRCH_MemError);
    }

    
    /* Loop over each radix (byte) in the number, starting with the least significant byte */
    for ( uiRadix = 0; uiRadix < sizeof(float); uiRadix++ ) {

        unsigned int                puiCount[256];
        unsigned int                *puiCountPtr = NULL;
        unsigned int                *puiCountEndPtr = NULL;
        struct srchShortResult      *pssrSrchShortResultsSource = *ppssrSrchShortResults;
        struct srchShortResult      *pssrSrchShortResultsSourcePtr = NULL;
        struct srchShortResult      *pssrSrchShortResultsSourceEndPtr = NULL;
        struct srchShortResult      *pssrSrchShortResultsPtr = NULL;
        unsigned int                uiRadixBits = uiRadix * 8;
        boolean                     bSkipRadix = false;
        unsigned long               ulTotalCount = 0, ulCount = 0;


        /* Clear the count array */
        for ( puiCountPtr = puiCount, puiCountEndPtr = puiCount + 256; puiCountPtr < puiCountEndPtr; puiCountPtr++ ) {
            *puiCountPtr = 0;
        }
    

        /* Count occurences of every byte value */
        for ( pssrSrchShortResultsSourcePtr = pssrSrchShortResultsSource, pssrSrchShortResultsSourceEndPtr = pssrSrchShortResultsSource + uiSrchShortResultsLength; 
                pssrSrchShortResultsSourcePtr < pssrSrchShortResultsSourceEndPtr; pssrSrchShortResultsSourcePtr++ ) {

            /* Get the index into the count array for this byte, ie a byte 
            ** containing a value of 20 will point to the counter array element 20.
            ** Note the cast to an int, as a float is 4 bytes long on both 32 and 64 bit platforms
            */
            puiCountPtr = puiCount + (((*((unsigned int *)&pssrSrchShortResultsSourcePtr->fSortKey)) >> uiRadixBits) & 0xFF);

            /* Increment the counter in the counter array element */
            ++(*puiCountPtr);
        }


        /* Check to see if we can skip this radix, we can skip it if all the bytes contain the same value */
        for ( puiCountPtr = puiCount, puiCountEndPtr = puiCount + 256, bSkipRadix = false; puiCountPtr < puiCountEndPtr; puiCountPtr++ ) {

            /* All the bytes contain the same value if the value contained in the 
            ** counter is the same as the number of elements to sort 
            */
            if ( *puiCountPtr == uiSrchShortResultsLength ) {
                bSkipRadix = true;
            }
        }
        
        /* Skip if we were flagged to do so */
        if ( bSkipRadix == true ) {
/*            iUtlLogDebug(UTL_LOG_CONTEXT, "Skipping radix: %u", uiRadix); */
            continue;
        }

/*        iUtlLogDebug(UTL_LOG_CONTEXT, "Processing radix: %u", uiRadix); */

        /* Select the transform based on the sort order requested */
        if ( uiSortOrder == SRCH_SHORT_RESULTS_SORT_TYPE_FLOAT_ASC ) {

            /* Transform count into index by summing elements and storing into same array */
            for ( puiCountPtr = puiCount, puiCountEndPtr = puiCount + 256, ulTotalCount = 0; puiCountPtr < puiCountEndPtr; puiCountPtr++ ) {
                ulCount = *puiCountPtr;
                *puiCountPtr = ulTotalCount;
                ulTotalCount += ulCount;
            }
        }
        else if ( uiSortOrder == SRCH_SHORT_RESULTS_SORT_TYPE_FLOAT_DESC ) {

            /* Transform count into index by summing elements and storing into same array */
            for ( puiCountPtr = puiCount + 255, ulTotalCount = 0; puiCountPtr >= puiCount; puiCountPtr-- ) {
                ulCount = *puiCountPtr;
                *puiCountPtr = ulTotalCount;
                ulTotalCount += ulCount;
            }
        }


        /* Fill destination with the right values in the right place */
        for ( pssrSrchShortResultsSourcePtr = pssrSrchShortResultsSource, pssrSrchShortResultsSourceEndPtr = pssrSrchShortResultsSource + uiSrchShortResultsLength; 
                pssrSrchShortResultsSourcePtr < pssrSrchShortResultsSourceEndPtr; pssrSrchShortResultsSourcePtr++ ) {
            
            /* Get the index into the count array for this byte, ie a byte 
            ** containing a value of 20 will point to the counter array element 20.
            ** Note the cast to an int, as a float is 4 bytes long on both 32 and 64 bit platforms
            */
            puiCountPtr = puiCount + (((*((unsigned int *)&pssrSrchShortResultsSourcePtr->fSortKey)) >> uiRadixBits) & 0xFF);
            
            /* Copy the short result, note that the counter array element points to the index in the destination */
            pssrSrchShortResultsPtr = pssrSrchShortResultsDestination + *puiCountPtr;
            SRCH_SHORT_RESULTS_COPY_SRCH_SHORT_RESULTS_FLOAT(pssrSrchShortResultsPtr, pssrSrchShortResultsSourcePtr);
    
            /* Increment the counter in the counter array element */
            ++(*puiCountPtr);
        }


        /* Swap the source and destination, ie the destination becomes the new source, 
        ** and the source becomes the new destination, note that this sets up the source
        ** and destination for the next loop or the return (we return the source)
        */
        pssrSrchShortResultsPtr = *ppssrSrchShortResults;
        *ppssrSrchShortResults = pssrSrchShortResultsDestination;
        pssrSrchShortResultsDestination = pssrSrchShortResultsPtr;
    }

    
    /* Free the destination */
    s_free(pssrSrchShortResultsDestination);


    return (SRCH_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchShortResultRadixSortUInt()

    Purpose:    This functions sorts a short results array in unsigned integer order, 
                it implements a radix algorithm.

    Parameters: ppssrSrchShortResults       pointer to a short result structure array (return pointer as well)
                uiSrchShortResultsLength    length of the short result structure array
                uiSortOrder                 sort order

    Globals:    none

    Returns:    SRCH error code

*/
static int iSrchShortResultRadixSortUInt
(
    struct srchShortResult **ppssrSrchShortResults,
    unsigned int uiSrchShortResultsLength,
    unsigned int uiSortOrder
)
{

    struct srchShortResult      *pssrSrchShortResultsDestination = NULL;
    unsigned int                uiRadix = 0;


    ASSERT(ppssrSrchShortResults != NULL);
    ASSERT(uiSrchShortResultsLength > 0);
    ASSERT((uiSortOrder == SRCH_SHORT_RESULTS_SORT_TYPE_UINT_ASC) || (uiSortOrder == SRCH_SHORT_RESULTS_SORT_TYPE_UINT_DESC));

    
    /* Allocate space for the destination, this starts off being the destination, 
    ** and will alternatively become the source and the destination
    */
    if ( (pssrSrchShortResultsDestination = (struct srchShortResult *)s_malloc((size_t)(sizeof(struct srchShortResult) * uiSrchShortResultsLength))) == NULL ) {
        return (SRCH_MemError);
    }

    
    /* Loop over each radix (byte) in the number, starting with the least significant byte */
    for ( uiRadix = 0; uiRadix < sizeof(unsigned int); uiRadix++ ) {

        unsigned int                puiCount[256];
        unsigned int                *puiCountPtr = NULL;
        unsigned int                *puiCountEndPtr = NULL;
        struct srchShortResult      *pssrSrchShortResultsSource = *ppssrSrchShortResults;
        struct srchShortResult      *pssrSrchShortResultsSourcePtr = NULL;
        struct srchShortResult      *pssrSrchShortResultsSourceEndPtr = NULL;
        struct srchShortResult      *pssrSrchShortResultsPtr = NULL;
        unsigned int                uiRadixBits = uiRadix * 8;
        boolean                     bSkipRadix = false;
        unsigned long               ulTotalCount = 0, ulCount = 0;


        /* Clear the count array */
        for ( puiCountPtr = puiCount, puiCountEndPtr = puiCount + 256; puiCountPtr < puiCountEndPtr; puiCountPtr++ ) {
            *puiCountPtr = 0;
        }
    

        /* Count occurences of every byte value */
        for ( pssrSrchShortResultsSourcePtr = pssrSrchShortResultsSource, pssrSrchShortResultsSourceEndPtr = pssrSrchShortResultsSource + uiSrchShortResultsLength; 
                pssrSrchShortResultsSourcePtr < pssrSrchShortResultsSourceEndPtr; pssrSrchShortResultsSourcePtr++ ) {

            /* Get the index into the count array for this byte, ie a byte 
            ** containing a value of 20 will point to the counter array element 20
            */
            puiCountPtr = puiCount + ((pssrSrchShortResultsSourcePtr->uiSortKey >> uiRadixBits) & 0xFF);

            /* Increment the counter in the counter array element */
            ++(*puiCountPtr);
        }


        /* Check to see if we can skip this radix, we can skip it if all the bytes contain the same value */
        for ( puiCountPtr = puiCount, puiCountEndPtr = puiCount + 256, bSkipRadix = false; puiCountPtr < puiCountEndPtr; puiCountPtr++ ) {

            /* All the bytes contain the same value if the value contained in the 
            ** counter is the same as the number of elements to sort 
            */
            if ( *puiCountPtr == uiSrchShortResultsLength ) {
                bSkipRadix = true;
            }
        }
        
        /* Skip if we were flagged to do so */
        if ( bSkipRadix == true ) {
/*            iUtlLogDebug(UTL_LOG_CONTEXT, "Skipping radix: %u", uiRadix); */
            continue;
        }

/*         iUtlLogDebug(UTL_LOG_CONTEXT, "Processing radix: %u", uiRadix); */

        /* Select the transform based on the sort order requested */
        if ( uiSortOrder == SRCH_SHORT_RESULTS_SORT_TYPE_UINT_ASC ) {

            /* Transform count into index by summing elements and storing into same array */
            for ( puiCountPtr = puiCount, puiCountEndPtr = puiCount + 256, ulTotalCount = 0; puiCountPtr < puiCountEndPtr; puiCountPtr++ ) {
                ulCount = *puiCountPtr;
                *puiCountPtr = ulTotalCount;
                ulTotalCount += ulCount;
            }
        }
        else if ( uiSortOrder == SRCH_SHORT_RESULTS_SORT_TYPE_UINT_DESC ) {

            /* Transform count into index by summing elements and storing into same array */
            for ( puiCountPtr = puiCount + 255, ulTotalCount = 0; puiCountPtr >= puiCount; puiCountPtr-- ) {
                ulCount = *puiCountPtr;
                *puiCountPtr = ulTotalCount;
                ulTotalCount += ulCount;
            }
        }


        /* Fill destination with the right values in the right place */
        for ( pssrSrchShortResultsSourcePtr = pssrSrchShortResultsSource, pssrSrchShortResultsSourceEndPtr = pssrSrchShortResultsSource + uiSrchShortResultsLength; 
                pssrSrchShortResultsSourcePtr < pssrSrchShortResultsSourceEndPtr; pssrSrchShortResultsSourcePtr++ ) {
            
            /* Get the index into the count array for this byte, ie a byte 
            ** containing a value of 20 will point to the counter array element 20
            */
            puiCountPtr = puiCount + ((pssrSrchShortResultsSourcePtr->uiSortKey >> uiRadixBits) & 0xFF);
            
            /* Copy the short result, note that the counter array element points to the index in the destination */
            pssrSrchShortResultsPtr = pssrSrchShortResultsDestination + *puiCountPtr;
            SRCH_SHORT_RESULTS_COPY_SRCH_SHORT_RESULTS_UINT(pssrSrchShortResultsPtr, pssrSrchShortResultsSourcePtr);
    
            /* Increment the counter in the counter array element */
            ++(*puiCountPtr);
        }


        /* Swap the source and destination, ie the destination becomes the new source, 
        ** and the source becomes the new destination, note that this sets up the source
        ** and destination for the next loop or the return (we return the source)
        */
        pssrSrchShortResultsPtr = *ppssrSrchShortResults;
        *ppssrSrchShortResults = pssrSrchShortResultsDestination;
        pssrSrchShortResultsDestination = pssrSrchShortResultsPtr;
    }

    
    /* Free the destination */
    s_free(pssrSrchShortResultsDestination);


    return (SRCH_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchShortResultRadixSortULong()

    Purpose:    This functions sorts a short results array in unsigned long order, 
                it implements a radix algorithm.

    Parameters: ppssrSrchShortResults       pointer to a short result structure array (return pointer as well)
                uiSrchShortResultsLength    length of the short result structure array
                uiSortOrder                 sort order

    Globals:    none

    Returns:    SRCH error code

*/
static int iSrchShortResultRadixSortULong
(
    struct srchShortResult **ppssrSrchShortResults,
    unsigned int uiSrchShortResultsLength,
    unsigned int uiSortOrder
)
{

    struct srchShortResult      *pssrSrchShortResultsDestination = NULL;
    unsigned int                uiRadix = 0;


    ASSERT(ppssrSrchShortResults != NULL);
    ASSERT(uiSrchShortResultsLength > 0);
    ASSERT((uiSortOrder == SRCH_SHORT_RESULTS_SORT_TYPE_ULLONG_ASC) || (uiSortOrder == SRCH_SHORT_RESULTS_SORT_TYPE_ULONG_DESC));

    
    /* Allocate space for the destination, this starts off being the destination, 
    ** and will alternatively become the source and the destination
    */
    if ( (pssrSrchShortResultsDestination = (struct srchShortResult *)s_malloc((size_t)(sizeof(struct srchShortResult) * uiSrchShortResultsLength))) == NULL ) {
        return (SRCH_MemError);
    }

    
    /* Loop over each radix (byte) in the number, starting with the least significant byte */
    for ( uiRadix = 0; uiRadix < sizeof(unsigned long); uiRadix++ ) {

        unsigned int                puiCount[256];
        unsigned int                *puiCountPtr = NULL;
        unsigned int                *puiCountEndPtr = NULL;
        struct srchShortResult      *pssrSrchShortResultsSource = *ppssrSrchShortResults;
        struct srchShortResult      *pssrSrchShortResultsSourcePtr = NULL;
        struct srchShortResult      *pssrSrchShortResultsSourceEndPtr = NULL;
        struct srchShortResult      *pssrSrchShortResultsPtr = NULL;
        unsigned int                uiRadixBits = uiRadix * 8;
        boolean                     bSkipRadix = false;
        unsigned long               ulTotalCount = 0, ulCount = 0;


        /* Clear the count array */
        for ( puiCountPtr = puiCount, puiCountEndPtr = puiCount + 256; puiCountPtr < puiCountEndPtr; puiCountPtr++ ) {
            *puiCountPtr = 0;
        }
    

        /* Count occurences of every byte value */
        for ( pssrSrchShortResultsSourcePtr = pssrSrchShortResultsSource, pssrSrchShortResultsSourceEndPtr = pssrSrchShortResultsSource + uiSrchShortResultsLength; 
                pssrSrchShortResultsSourcePtr < pssrSrchShortResultsSourceEndPtr; pssrSrchShortResultsSourcePtr++ ) {

            /* Get the index into the count array for this byte, ie a byte 
            ** containing a value of 20 will point to the counter array element 20
            */
            puiCountPtr = puiCount + ((pssrSrchShortResultsSourcePtr->ulSortKey >> uiRadixBits) & 0xFF);

            /* Increment the counter in the counter array element */
            ++(*puiCountPtr);
        }


        /* Check to see if we can skip this radix, we can skip it if all the bytes contain the same value */
        for ( puiCountPtr = puiCount, puiCountEndPtr = puiCount + 256, bSkipRadix = false; puiCountPtr < puiCountEndPtr; puiCountPtr++ ) {

            /* All the bytes contain the same value if the value contained in the 
            ** counter is the same as the number of elements to sort 
            */
            if ( *puiCountPtr == uiSrchShortResultsLength ) {
                bSkipRadix = true;
            }
        }
        
        /* Skip if we were flagged to do so */
        if ( bSkipRadix == true ) {
/*            iUtlLogDebug(UTL_LOG_CONTEXT, "Skipping radix: %u", uiRadix); */
            continue;
        }

/*        UtlLogDebug(UTL_LOG_CONTEXT, "Processing radix: %u", uiRadix); */

        /* Select the transform based on the sort order requested */
        if ( uiSortOrder == SRCH_SHORT_RESULTS_SORT_TYPE_ULLONG_ASC ) {

            /* Transform count into index by summing elements and storing into same array */
            for ( puiCountPtr = puiCount, puiCountEndPtr = puiCount + 256, ulTotalCount = 0; puiCountPtr < puiCountEndPtr; puiCountPtr++ ) {
                ulCount = *puiCountPtr;
                *puiCountPtr = ulTotalCount;
                ulTotalCount += ulCount;
            }
        }
        else if ( uiSortOrder == SRCH_SHORT_RESULTS_SORT_TYPE_ULONG_DESC ) {

            /* Transform count into index by summing elements and storing into same array */
            for ( puiCountPtr = puiCount + 255, ulTotalCount = 0; puiCountPtr >= puiCount; puiCountPtr-- ) {
                ulCount = *puiCountPtr;
                *puiCountPtr = ulTotalCount;
                ulTotalCount += ulCount;
            }
        }


        /* Fill destination with the right values in the right place */
        for ( pssrSrchShortResultsSourcePtr = pssrSrchShortResultsSource, pssrSrchShortResultsSourceEndPtr = pssrSrchShortResultsSource + uiSrchShortResultsLength; 
                pssrSrchShortResultsSourcePtr < pssrSrchShortResultsSourceEndPtr; pssrSrchShortResultsSourcePtr++ ) {
            
            /* Get the index into the count array for this byte, ie a byte 
            ** containing a value of 20 will point to the counter array element 20
            */
            puiCountPtr = puiCount + ((pssrSrchShortResultsSourcePtr->ulSortKey >> uiRadixBits) & 0xFF);
            
            /* Copy the short result, note that the counter array element points to the index in the destination */
            pssrSrchShortResultsPtr = pssrSrchShortResultsDestination + *puiCountPtr;
            SRCH_SHORT_RESULTS_COPY_SRCH_SHORT_RESULTS_ULLONG(pssrSrchShortResultsPtr, pssrSrchShortResultsSourcePtr);
    
            /* Increment the counter in the counter array element */
            ++(*puiCountPtr);
        }


        /* Swap the source and destination, ie the destination becomes the new source, 
        ** and the source becomes the new destination, note that this sets up the source
        ** and destination for the next loop or the return (we return the source)
        */
        pssrSrchShortResultsPtr = *ppssrSrchShortResults;
        *ppssrSrchShortResults = pssrSrchShortResultsDestination;
        pssrSrchShortResultsDestination = pssrSrchShortResultsPtr;
    }

    
    /* Free the destination */
    s_free(pssrSrchShortResultsDestination);


    return (SRCH_NoError);

}


/*---------------------------------------------------------------------------*/
