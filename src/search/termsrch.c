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

    Module:     termsrch.c

    Author:     Francois Schiettecatte (FS Consulting LLC.)

    Created:    21 January 2006

    Purpose:    This module provides term search processing for search.c 

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
#define UTL_LOG_CONTEXT                     (unsigned char *)"com.fsconsult.mps.src.search.termsrch"


/*---------------------------------------------------------------------------*/

/*
** Feature Defines
*/

/* Enable document ID range restrictions */
/* #define SRCH_TERMSRCH_ENABLE_DOCUMENT_ID_RANGE_RESTRICTIONS */


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchTermSearchGetSearchPostingsListFromTerm()

    Purpose:    This function searches for a single term in the index and returns a 
                search postings list structure for that term whether the term
                exists or not.
                
                This is a bottleneck when searching.

    Parameters: pssSrchSearch                       search structure
                psiSrchIndex                        index structure
                pucTerm                             term to search for
                fWeight                             term weight to use
                pucFieldIDBitmap                    field id bitmap to search (optional)
                uiFieldIDBitmapLength               field id bitmap length (optional)
                fFrequentTermCoverageThreshold      frequent term coverage threshold (0 indicates no threshold)
                uiStartDocumentID                   start document ID restriction (0 for no restriction)
                uiEndDocumentID                     end document ID restriction (0 for no restriction)
                ppsplSrchPostingsList               return pointer for the search postings list structure (allocated)

    Globals:    none

    Returns:    SRCH error code

*/
int iSrchTermSearchGetSearchPostingsListFromTerm
(
    struct srchSearch *pssSrchSearch,
    struct srchIndex *psiSrchIndex,
    unsigned char *pucTerm,
    float fWeight,
    unsigned char *pucFieldIDBitmap,
    unsigned int uiFieldIDBitmapLength,
    float fFrequentTermCoverageThreshold,
    unsigned int uiStartDocumentID,
    unsigned int uiEndDocumentID,
    struct srchPostingsList **ppsplSrchPostingsList
)
{

    int                         iError = UTL_NoError;

    unsigned long               ulIndexBlockID = 0;
    unsigned char               *pucIndexBlock = NULL;
    unsigned int                uiIndexBlockLength = 0;
    unsigned int                uiIndexBlockHeaderLength = 0;
    unsigned int                uiIndexBlockDataLength = 0;

    float                       fTermWeight = 0;
    
    unsigned int                uiFieldID = 0;

    struct srchPostingsList     *psplSrchPostingsList = NULL;


    /* Check the parameters */
    if ( pssSrchSearch == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pssSrchSearch' parameter passed to 'iSrchTermSearchGetSearchPostingsListFromTerm'."); 
        return (SRCH_TermSearchInvalidSearch);
    }

    if ( psiSrchIndex == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'psiSrchIndex' parameter passed to 'iSrchTermSearchGetSearchPostingsListFromTerm'."); 
        return (SRCH_InvalidIndex);
    }

    if ( bUtlStringsIsStringNULL(pucTerm) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pucTerm' parameter passed to 'iSrchTermSearchGetSearchPostingsListFromTerm'."); 
        return (SRCH_TermSearchInvalidTerm);
    }

    if ( fWeight <= 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'fWeight' parameter passed to 'iSrchTermSearchGetSearchPostingsListFromTerm'."); 
        return (SRCH_TermSearchInvalidWeight);
    }

    if ( ((pucFieldIDBitmap == NULL) && (uiFieldIDBitmapLength > 0)) || ((pucFieldIDBitmap != NULL) && (uiFieldIDBitmapLength <= 0)) ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'pucFieldIDBitmap' & 'uiFieldIDBitmapLength' parameters passed to 'iSrchTermSearchGetSearchPostingsListFromTerm'."); 
        return (SRCH_TermSearchInvalidFieldIDBitMap);
    }

    if ( fFrequentTermCoverageThreshold < 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'fFrequentTermCoverageThreshold' parameter passed to 'iSrchTermSearchGetSearchPostingsListFromTerm'."); 
        return (SRCH_TermSearchInvalidFrequentTermCoverageThreshold);
    }

    if ( uiStartDocumentID < 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiStartDocumentID' parameter passed to 'iSrchTermSearchGetSearchPostingsListFromTerm'."); 
        return (SRCH_TermSearchInvalidDocumentID);
    }

    if ( uiEndDocumentID < 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiEndDocumentID' parameter passed to 'iSrchTermSearchGetSearchPostingsListFromTerm'."); 
        return (SRCH_TermSearchInvalidDocumentID);
    }

    if ( ppsplSrchPostingsList == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'ppsplSrchPostingsList' parameter passed to 'iSrchTermSearchGetSearchPostingsListFromTerm'."); 
        return (SRCH_ReturnParameterError);
    }


/* printf("pucTerm: '%s'\n", pucTerm); */
/* printf("pucTerm: '%s', uiStartDocumentID: %u, uiEndDocumentID: %u\n", pucTerm, uiStartDocumentID, uiEndDocumentID); */

    /* Look up the term in the term dict, process any errors */
    {
        unsigned int    uiTermCount = SPI_TERM_COUNT_UNKNOWN;
        unsigned int    uiDocumentCount = SPI_TERM_DOCUMENT_COUNT_UNKNOWN;
        unsigned int    uiTermType = SPI_TERM_TYPE_UNKNOWN;


        /* Create an empty postings list */
        if ( (iError = iSrchPostingCreateSrchPostingsList(SPI_TERM_TYPE_UNKNOWN, SPI_TERM_COUNT_UNKNOWN, SPI_TERM_DOCUMENT_COUNT_UNKNOWN, 
                false, NULL, 0, &psplSrchPostingsList)) != SRCH_NoError ) {
            goto bailFromiSrchTermSearchGetSearchPostingsListFromTerm;
        }


        /* Look up the term */
        iError = iSrchTermDictLookup(psiSrchIndex, pucTerm, pucFieldIDBitmap, uiFieldIDBitmapLength, 
                &uiTermType, &uiTermCount, &uiDocumentCount, &ulIndexBlockID);

        /* Return an empty postings list if the term does not exist or does not occur
        ** (subject to constraints, like field ID), otherwise return an error if
        ** a real error occured
        */
        if ( (iError == SRCH_TermDictTermNotFound) || (iError == SRCH_TermDictTermDoesNotOccur) ) {

            /* Populate the search postings list structure */
            psplSrchPostingsList->uiTermType = SPI_TERM_TYPE_UNKNOWN;
            psplSrchPostingsList->uiTermCount = SPI_TERM_COUNT_UNKNOWN;
            psplSrchPostingsList->uiDocumentCount = SPI_TERM_DOCUMENT_COUNT_UNKNOWN;

            iError = SRCH_NoError;
            goto bailFromiSrchTermSearchGetSearchPostingsListFromTerm;
        }
        else if ( iError != SRCH_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to look up the term: '%s', in the term dictionary, srch error: %d.", pucTerm, iError); 
            goto bailFromiSrchTermSearchGetSearchPostingsListFromTerm;
        }


        /* Term exists */
        
    
        /* We report stop terms here if this is an unfielded search,
        ** otherwise we search on them in case they are stored in the index
        */
        if ( (uiTermType == SPI_TERM_TYPE_STOP) && (pucFieldIDBitmap == NULL) ) {

            /* Populate the search postings list structure */
            psplSrchPostingsList->uiTermType = uiTermType;
            psplSrchPostingsList->uiTermCount = uiTermCount;
            psplSrchPostingsList->uiDocumentCount = uiDocumentCount;

            iError = SRCH_NoError;
            goto bailFromiSrchTermSearchGetSearchPostingsListFromTerm;
        }
    

        /* See if this term meets the frequent term coverage threshold */
        if ( (fFrequentTermCoverageThreshold > 0) && (uiTermType != SPI_TERM_TYPE_STOP) ) {
            
            float    fTermCoverage = ((float)uiTermCount / psiSrchIndex->uiDocumentCount) * 100;
            
/*             iUtlLogDebug(UTL_LOG_CONTEXT, "pucTerm: '%s', uiTermCount: %u, uiDocumentCount: %u, psiSrchIndex->uiDocumentCount: %u, fTermCoverage: %9.4f%%.",  */
/*                     pucTerm, uiTermCount, uiDocumentCount, psiSrchIndex->uiDocumentCount, fTermCoverage); */
        
            /* Flag this term as frequent if its coverage is greater than the threshold */
            if ( fTermCoverage > fFrequentTermCoverageThreshold ) {

                /* Populate the search postings list structure */
                psplSrchPostingsList->uiTermType = SPI_TERM_TYPE_FREQUENT;
                psplSrchPostingsList->uiTermCount = uiTermCount;
                psplSrchPostingsList->uiDocumentCount = uiDocumentCount;

                iError = SRCH_NoError;
                goto bailFromiSrchTermSearchGetSearchPostingsListFromTerm;
            }
        }


        /* Populate the search postings list */
        psplSrchPostingsList->uiTermType = uiTermType;
        psplSrchPostingsList->uiTermCount = uiTermCount;
        psplSrchPostingsList->uiDocumentCount = uiDocumentCount;
        psplSrchPostingsList->pspSrchPostings = NULL;
        psplSrchPostingsList->uiSrchPostingsLength = 0;
    }



    /* Get the index block for this term, processing any errors */
    {
        unsigned char   *pucIndexBlockPtr = NULL;

        /* Get the index block data */
        if ( (iError = iUtlDataGetEntry(psiSrchIndex->pvUtlIndexData, ulIndexBlockID, (void **)&pucIndexBlock, &uiIndexBlockLength)) != UTL_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to get index data, index object ID: %lu, index: '%s', utl error: %d.", 
                    ulIndexBlockID, psiSrchIndex->pucIndexName, iError);
            iError = SRCH_TermSearchGetObjectFailed;
            goto bailFromiSrchTermSearchGetSearchPostingsListFromTerm;
        }
    
        /* Set the pointer to start reading from */
        pucIndexBlockPtr = pucIndexBlock;
    
        /* Get the size of the index block */
        UTL_NUM_READ_COMPRESSED_UINT(uiIndexBlockDataLength, pucIndexBlockPtr);
        uiIndexBlockHeaderLength = pucIndexBlockPtr - pucIndexBlock;
    
        /* Empty block - probably a stop term */
        if ( uiIndexBlockDataLength == 0 ) {
            
            /* Override the postings pointer and postings length */
            psplSrchPostingsList->pspSrchPostings = NULL;
            psplSrchPostingsList->uiSrchPostingsLength = 0;
    
            iError = SRCH_NoError;
            goto bailFromiSrchTermSearchGetSearchPostingsListFromTerm;
        }
    }



    /* Get the term weight */
    {
        /* IDF term weight */
        fTermWeight = SRCH_SEARCH_IDF_FACTOR(psplSrchPostingsList->uiTermCount, psplSrchPostingsList->uiDocumentCount, psiSrchIndex->uiDocumentCount) * fWeight;

        /* Set the term weight to the default if it went pear-shaped */
        if ( fTermWeight <= 0 ) {
            fTermWeight = SRCH_SEARCH_TERM_WEIGHT_DEFAULT;
        }

/*         iUtlLogDebug(UTL_LOG_CONTEXT, "fTermWeight [%f].", fTermWeight);  */
    }



    /* Set the field ID if we can, this allows us to use an optimization further down, 
    ** mmm, optimizations, we like optimizations
    */
    if ( pucFieldIDBitmap != NULL ) {

        unsigned int    uiI = 0;
        unsigned int    uiBitSetCount = 0;

        ASSERT(uiFieldIDBitmapLength == psiSrchIndex->uiFieldIDMaximum);

        /* Loop over each field ID - field ID 0 is not a field */ 
        for ( uiI = 0, uiBitSetCount = 0; uiI < psiSrchIndex->uiFieldIDMaximum; uiI++ ) {

            /* See what is set, setting the field ID and the field count - field ID 0 is not a field */ 
            if ( UTL_BITMAP_IS_BIT_SET_IN_POINTER(pucFieldIDBitmap, uiI) ) {
                uiFieldID = uiI + 1;
                uiBitSetCount++;
            }
        }
        
        /* Clear the field ID if there is more than one field set, we cant use the optimization, its like... a bummer... */
        if ( uiBitSetCount > 1 ) {
            uiFieldID = 0;
        }

        ASSERT(uiBitSetCount > 0);
    }



    /* Optimize the end document ID, this allows us to remove the
    ** 'uiEndDocumentID != 0' clause from the document ID range check
    */
#if defined(SRCH_TERMSRCH_ENABLE_DOCUMENT_ID_RANGE_RESTRICTIONS)
    if ( uiEndDocumentID == 0 ) {
        uiEndDocumentID = UINT_MAX;
    }
#endif /* !defined(SRCH_TERMSRCH_ENABLE_DOCUMENT_ID_RANGE_RESTRICTIONS) */



    /* Loop over the index block, decoding it and populating the postings array */
    {
        struct srchPosting      *pspSrchPostingsPtr = NULL;

        unsigned char           *pucIndexBlockPtr = NULL;
        unsigned char           *pucIndexBlockEndPtr = NULL;

        unsigned int            uiIndexEntryDocumentID = 0;
        unsigned int            uiIndexEntryDeltaDocumentID = 0;
        unsigned int            uiIndexEntryTermPosition = 0;
        unsigned int            uiIndexEntryDeltaTermPosition = 0;
        unsigned int            uiIndexEntryFieldID = 0;
/*         unsigned int            uiIndexEntryTermWeight = 0; */
        
        unsigned int            uiDocumentCount = 0;


        /* Set the postings length, initially need as many as there are terms */
        psplSrchPostingsList->uiSrchPostingsLength = psplSrchPostingsList->uiTermCount;

        /* Allocate the postings array */
        if ( (psplSrchPostingsList->pspSrchPostings = (struct srchPosting *)s_malloc((size_t)(psplSrchPostingsList->uiSrchPostingsLength * sizeof(struct srchPosting)))) == NULL ) {
            iError = SRCH_MemError;
            goto bailFromiSrchTermSearchGetSearchPostingsListFromTerm;
        }


        /* uiFieldID > 0 means that this is a fielded search and that there is only one field to match */
        if ( uiFieldID > 0 ) {

            /* Loop over all the entries in the index block */
            for ( pspSrchPostingsPtr = psplSrchPostingsList->pspSrchPostings, pucIndexBlockPtr = pucIndexBlock + uiIndexBlockHeaderLength,
                    pucIndexBlockEndPtr = (pucIndexBlock + uiIndexBlockDataLength + uiIndexBlockHeaderLength); pucIndexBlockPtr < pucIndexBlockEndPtr; ) {
        
/* Compressed int */
                /* Read the index block */
                UTL_NUM_READ_COMPRESSED_UINT(uiIndexEntryDeltaDocumentID, pucIndexBlockPtr);
                UTL_NUM_READ_COMPRESSED_UINT(uiIndexEntryDeltaTermPosition, pucIndexBlockPtr);
                UTL_NUM_READ_COMPRESSED_UINT(uiIndexEntryFieldID, pucIndexBlockPtr);
/*                 UTL_NUM_READ_COMPRESSED_UINT(uiIndexEntryTermWeight, pucIndexBlockPtr); */
        
/* Varint */
                /* Read the index block */
/*                 UTL_NUM_READ_COMPACT_VARINT_TRIO(uiIndexEntryDeltaDocumentID, uiIndexEntryDeltaTermPosition, uiIndexEntryFieldID, pucIndexBlockPtr); */
/*                 UTL_NUM_READ_COMPACT_VARINT_QUAD(uiIndexEntryDeltaDocumentID, uiIndexEntryDeltaTermPosition, uiIndexEntryFieldID, uiIndexEntryTermWeight, pucIndexBlockPtr); */


                ASSERT(uiIndexEntryDeltaDocumentID >= 0);
                ASSERT(uiIndexEntryDeltaTermPosition >= 0);
                ASSERT(uiIndexEntryFieldID >= 0);
/*                 ASSERT(uiIndexEntryTermWeight >= 0); */

                /* Set the document ID */
                uiIndexEntryDocumentID += uiIndexEntryDeltaDocumentID;

                /* Skip this document if this is not the field we are searching on */
                if ( uiIndexEntryFieldID != uiFieldID ) {
                    continue;
                }

#if defined(SRCH_TERMSRCH_ENABLE_DOCUMENT_ID_RANGE_RESTRICTIONS)
                /* Skip this document if we are not yet in range */
                if ( uiIndexEntryDocumentID < uiStartDocumentID ) {
                    continue;
                }
                
                /* Break out if we are beyond the range */
                if ( uiIndexEntryDocumentID > uiEndDocumentID ) {
                    break;
                }
#endif /* defined(SRCH_TERMSRCH_ENABLE_DOCUMENT_ID_RANGE_RESTRICTIONS) */

                /* Reset the term position if this is a new document */
                if ( uiIndexEntryDeltaDocumentID != 0 ) {
                    uiIndexEntryTermPosition = 0;
                }

                /* Increment the term position */
                uiIndexEntryTermPosition += uiIndexEntryDeltaTermPosition;

                /* Increment the document count */
                if ( uiIndexEntryDeltaDocumentID != 0 ) {
                    uiDocumentCount++;
                }

                /* Set the posting information */
                pspSrchPostingsPtr->uiDocumentID = uiIndexEntryDocumentID;
                pspSrchPostingsPtr->uiTermPosition = uiIndexEntryTermPosition;
                pspSrchPostingsPtr->fWeight = fTermWeight;

/*                 iUtlLogDebug(UTL_LOG_CONTEXT, "pucTerm: [%s], pspSrchPostingsPtr->uiDocumentID: %u, uiIndexEntryFieldID: %u, pspSrchPostingsPtr->uiTermPosition: %u, pspSrchPostingsPtr->fWeight: %f",  */
/*                         pucTerm, pspSrchPostingsPtr->uiDocumentID, uiIndexEntryFieldID, pspSrchPostingsPtr->uiTermPosition, pspSrchPostingsPtr->fWeight); */

                /* Increment the postings pointer */
                pspSrchPostingsPtr++;
            }
        }

        /* pucFieldIDBitmap != NULL means that this is a fielded search and there is more than one field to match */
        else if ( pucFieldIDBitmap != NULL ) {

            /* Loop over all the entries in the index block */
            for ( pspSrchPostingsPtr = psplSrchPostingsList->pspSrchPostings, pucIndexBlockPtr = pucIndexBlock + uiIndexBlockHeaderLength,
                    pucIndexBlockEndPtr = (pucIndexBlock + uiIndexBlockDataLength + uiIndexBlockHeaderLength); pucIndexBlockPtr < pucIndexBlockEndPtr; ) {
        
/* Compressed int */
                /* Read the index block */
                UTL_NUM_READ_COMPRESSED_UINT(uiIndexEntryDeltaDocumentID, pucIndexBlockPtr);
                UTL_NUM_READ_COMPRESSED_UINT(uiIndexEntryDeltaTermPosition, pucIndexBlockPtr);
                UTL_NUM_READ_COMPRESSED_UINT(uiIndexEntryFieldID, pucIndexBlockPtr);
/*                 UTL_NUM_READ_COMPRESSED_UINT(uiIndexEntryTermWeight, pucIndexBlockPtr); */
        
/* Varint */
                /* Read the index block */
/*                 UTL_NUM_READ_COMPACT_VARINT_TRIO(uiIndexEntryDeltaDocumentID, uiIndexEntryDeltaTermPosition, uiIndexEntryFieldID, pucIndexBlockPtr); */
/*                 UTL_NUM_READ_COMPACT_VARINT_QUAD(uiIndexEntryDeltaDocumentID, uiIndexEntryDeltaTermPosition, uiIndexEntryFieldID, uiIndexEntryTermWeight, pucIndexBlockPtr); */


                ASSERT(uiIndexEntryDeltaDocumentID >= 0);
                ASSERT(uiIndexEntryTermPosition >= 0);
                ASSERT((uiIndexEntryFieldID >= 0) && (uiIndexEntryFieldID <= uiFieldIDBitmapLength));

                /* Set the document ID */
                uiIndexEntryDocumentID += uiIndexEntryDeltaDocumentID;
        
                /* Skip this document if this is not the field we are searching on - field ID 0 is not a field */
                if ( !UTL_BITMAP_IS_BIT_SET_IN_POINTER(pucFieldIDBitmap, uiIndexEntryFieldID - 1) ) {
                    continue;
                }

#if defined(SRCH_TERMSRCH_ENABLE_DOCUMENT_ID_RANGE_RESTRICTIONS)
                /* Skip this document if we are not yet in range */
                if ( uiIndexEntryDocumentID < uiStartDocumentID ) {
                    continue;
                }
                
                /* Break out if we are beyond the range */
                if ( uiIndexEntryDocumentID > uiEndDocumentID ) {
                    break;
                }
#endif /* defined(SRCH_TERMSRCH_ENABLE_DOCUMENT_ID_RANGE_RESTRICTIONS) */

                /* Reset the term position if this is a new document */
                if ( uiIndexEntryDeltaDocumentID != 0 ) {
                    uiIndexEntryTermPosition = 0;
                }

                /* Increment the term position */
                uiIndexEntryTermPosition += uiIndexEntryDeltaTermPosition;

                /* Increment the document count */
                if ( uiIndexEntryDeltaDocumentID != 0 ) {
                    uiDocumentCount++;
                }

                /* Set the posting information */
                pspSrchPostingsPtr->uiDocumentID = uiIndexEntryDocumentID;
                pspSrchPostingsPtr->uiTermPosition = uiIndexEntryTermPosition;
                pspSrchPostingsPtr->fWeight = fTermWeight;
                
/*                 iUtlLogDebug(UTL_LOG_CONTEXT, "pucTerm: [%s], pspSrchPostingsPtr->uiDocumentID: %u, uiIndexEntryFieldID: %u, pspSrchPostingsPtr->uiTermPosition: %u, pspSrchPostingsPtr->fWeight: %f",  */
/*                         pucTerm, pspSrchPostingsPtr->uiDocumentID, uiIndexEntryFieldID, pspSrchPostingsPtr->uiTermPosition, pspSrchPostingsPtr->fWeight); */

                /* Increment the postings pointer */
                pspSrchPostingsPtr++;
            }
        }

        /* uiFieldID == 0 && pucFieldIDBitmap == NULL means that this is not a fielded search */
        else {

            /* Loop over all the entries in the index block */
            for ( pspSrchPostingsPtr = psplSrchPostingsList->pspSrchPostings, pucIndexBlockPtr = pucIndexBlock + uiIndexBlockHeaderLength,
                    pucIndexBlockEndPtr = (pucIndexBlock + uiIndexBlockDataLength + uiIndexBlockHeaderLength); pucIndexBlockPtr < pucIndexBlockEndPtr; ) {
        
/* Compressed int */
                /* Read the index block */
                UTL_NUM_READ_COMPRESSED_UINT(uiIndexEntryDeltaDocumentID, pucIndexBlockPtr);
                UTL_NUM_READ_COMPRESSED_UINT(uiIndexEntryDeltaTermPosition, pucIndexBlockPtr);
                UTL_NUM_READ_COMPRESSED_UINT(uiIndexEntryFieldID, pucIndexBlockPtr);
/*                 UTL_NUM_READ_COMPRESSED_UINT(uiIndexEntryTermWeight, pucIndexBlockPtr); */
        
/* Varint */
                /* Read the index block */
/*                 UTL_NUM_READ_COMPACT_VARINT_TRIO(uiIndexEntryDeltaDocumentID, uiIndexEntryDeltaTermPosition, uiIndexEntryFieldID, pucIndexBlockPtr); */
/*                 UTL_NUM_READ_COMPACT_VARINT_QUAD(uiIndexEntryDeltaDocumentID, uiIndexEntryDeltaTermPosition, uiIndexEntryFieldID, uiIndexEntryTermWeight, pucIndexBlockPtr); */


                ASSERT(uiIndexEntryDeltaDocumentID >= 0);
                ASSERT(uiIndexEntryDeltaTermPosition >= 0);
                ASSERT(uiIndexEntryFieldID >= 0);
/*                 ASSERT(uiIndexEntryTermWeight >= 0); */
        
                /* Set the document ID */
                uiIndexEntryDocumentID += uiIndexEntryDeltaDocumentID;

#if defined(SRCH_TERMSRCH_ENABLE_DOCUMENT_ID_RANGE_RESTRICTIONS)
                /* Skip this document if we are not yet in range */
                if ( uiIndexEntryDocumentID < uiStartDocumentID ) {
                    continue;
                }
                
                /* Break out if we are beyond the range */
                if ( uiIndexEntryDocumentID > uiEndDocumentID ) {
                    break;
                }
#endif    /* defined(SRCH_TERMSRCH_ENABLE_DOCUMENT_ID_RANGE_RESTRICTIONS) */
        
                /* Reset the term position if this is a new document */
                if ( uiIndexEntryDeltaDocumentID != 0 ) {
                    uiIndexEntryTermPosition = 0;
                }

                /* Set the term position */
                uiIndexEntryTermPosition += uiIndexEntryDeltaTermPosition;

#if defined(SRCH_TERMSRCH_ENABLE_DOCUMENT_ID_RANGE_RESTRICTIONS)
                /* Increment the document count */
                if ( uiIndexEntryDeltaDocumentID != 0 ) {
                    uiDocumentCount++;
                }
#endif    /* defined(SRCH_TERMSRCH_ENABLE_DOCUMENT_ID_RANGE_RESTRICTIONS) */

                /* Set the posting information */
                pspSrchPostingsPtr->uiDocumentID = uiIndexEntryDocumentID;
                pspSrchPostingsPtr->uiTermPosition = uiIndexEntryTermPosition;
                pspSrchPostingsPtr->fWeight = fTermWeight;

/*                 iUtlLogInfo(UTL_LOG_CONTEXT, "pucTerm: [%s], pspSrchPostingsPtr->uiDocumentID: %u, pspSrchPostingsPtr->uiTermPosition: %u, pspSrchPostingsPtr->fWeight: %f",  */
/*                         pucTerm, pspSrchPostingsPtr->uiDocumentID, pspSrchPostingsPtr->uiTermPosition, pspSrchPostingsPtr->fWeight); */
    
                /* Increment the postings pointer and the actual postings length */
                pspSrchPostingsPtr++;
    
            }
            
#if !defined(SRCH_TERMSRCH_ENABLE_DOCUMENT_ID_RANGE_RESTRICTIONS)
            /* Set the document count */
            uiDocumentCount = psplSrchPostingsList->uiDocumentCount;
#endif    /* defined(SRCH_TERMSRCH_ENABLE_DOCUMENT_ID_RANGE_RESTRICTIONS) */

        }

        /* Set the term and document counts */
        psplSrchPostingsList->uiTermCount = pspSrchPostingsPtr - psplSrchPostingsList->pspSrchPostings;
        psplSrchPostingsList->uiDocumentCount = uiDocumentCount;

    }



    /* Adjust the size of the postings array if needed */
    if ( psplSrchPostingsList->uiTermCount == 0 ) {

        /* Free the search postings */
        s_free(psplSrchPostingsList->pspSrchPostings);
        psplSrchPostingsList->uiSrchPostingsLength = 0;
    }
    else if ( psplSrchPostingsList->uiTermCount < psplSrchPostingsList->uiSrchPostingsLength ) {
        
        struct srchPosting    *pspSrchPostingsPtr = NULL;

        /* Adjust the size of the postings */
        if ( (pspSrchPostingsPtr = (struct srchPosting *)s_realloc(psplSrchPostingsList->pspSrchPostings, (size_t)(psplSrchPostingsList->uiTermCount * sizeof(struct srchPosting)))) == NULL ) {
            iError = SRCH_MemError;
            goto bailFromiSrchTermSearchGetSearchPostingsListFromTerm;
        }

        /* Hand over the postings and set the postings length */
        psplSrchPostingsList->pspSrchPostings = pspSrchPostingsPtr;
        psplSrchPostingsList->uiSrchPostingsLength = psplSrchPostingsList->uiTermCount;
    }
    
/* if ( psplSrchPostingsList->pspSrchPostings != NULL ) { */
/*     printf("uiDocumentID: %u - %u, term count: %u, document count: %u\n",  */
/*             psplSrchPostingsList->pspSrchPostings->uiDocumentID, (psplSrchPostingsList->pspSrchPostings + (psplSrchPostingsList->uiSrchPostingsLength - 1))->uiDocumentID, */
/*             psplSrchPostingsList->uiTermCount, psplSrchPostingsList->uiDocumentCount); */
/* } */
/* else { */
/*     printf("uiDocumentID: null\n");  */
/* } */
/* printf("\n");  */



    /* Bail label */
    bailFromiSrchTermSearchGetSearchPostingsListFromTerm:


    /* Handle the error */
    if ( iError == SRCH_NoError ) {

        /* Set the return pointer */
        *ppsplSrchPostingsList = psplSrchPostingsList;
    }
    else {

        /* Release the search postings list */
        iSrchPostingFreeSrchPostingsList(psplSrchPostingsList);
        psplSrchPostingsList = NULL;
    }


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchTermSearchGetSearchWeightsFromTerm()

    Purpose:    This function searches for a single term and cumulates the weight into the
                weight array. This function is used for relevance feedback because it is 
                very fast.

    Parameters: pssSrchSearch                       search structure
                psiSrchIndex                        index structure
                pucTerm                             term to search for
                fWeight                             term weight to use
                pucFieldIDBitmap                    field id bitmap to search (optional)
                uiFieldIDBitmapLength               field id bitmap length (optional)
                fFrequentTermCoverageThreshold      frequent term coverage threshold (0 indicates no threshold)
                uiStartDocumentID                   start document ID restriction (0 for no restriction)
                uiEndDocumentID                     end document ID restriction (0 for no restriction)
                ppswSrchWeight                      return pointer for the search weight structure (allocated if null, or unusable)

    Globals:    none

    Returns:    SRCH error code

*/
int iSrchTermSearchGetSearchWeightsFromTerm
(
    struct srchSearch *pssSrchSearch,
    struct srchIndex *psiSrchIndex,
    unsigned char *pucTerm,
    float fWeight,
    unsigned char *pucFieldIDBitmap,
    unsigned int uiFieldIDBitmapLength,
    float fFrequentTermCoverageThreshold,
    unsigned int uiStartDocumentID,
    unsigned int uiEndDocumentID,
    struct srchWeight **ppswSrchWeight
)
{

    int                 iError = UTL_NoError;

    float               fAdjustedWeight = 0;

    unsigned long       ulIndexBlockID = 0;
    unsigned char       *pucIndexBlock = NULL;
    unsigned int        uiIndexBlockLength = 0;
    unsigned int        uiIndexBlockHeaderLength = 0;
    unsigned int        uiIndexBlockDataLength = 0;
    
    unsigned int        uiFieldID = 0;

    struct srchWeight   *pswSrchWeight = NULL;


    /* Check the parameters */
    if ( pssSrchSearch == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pssSrchSearch' parameter passed to 'iSrchTermSearchGetSearchWeightsFromTerm'."); 
        return (SRCH_TermSearchInvalidSearch);
    }

    if ( psiSrchIndex == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'psiSrchIndex' parameter passed to 'iSrchTermSearchGetSearchWeightsFromTerm'."); 
        return (SRCH_InvalidIndex);
    }

    if ( bUtlStringsIsStringNULL(pucTerm) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pucTerm' parameter passed to 'iSrchTermSearchGetSearchWeightsFromTerm'."); 
        return (SRCH_TermSearchInvalidTerm);
    }

    if ( fWeight <= 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'fWeight' parameter passed to 'iSrchTermSearchGetSearchWeightsFromTerm'."); 
        return (SRCH_TermSearchInvalidWeight);
    }

    if ( ((pucFieldIDBitmap == NULL) && (uiFieldIDBitmapLength > 0)) || ((pucFieldIDBitmap != NULL) && (uiFieldIDBitmapLength <= 0)) ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'pucFieldIDBitmap' & 'uiFieldIDBitmapLength' parameters passed to 'iSrchTermSearchGetSearchWeightsFromTerm'."); 
        return (SRCH_TermSearchInvalidFieldIDBitMap);
    }

    if ( fFrequentTermCoverageThreshold < 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'fFrequentTermCoverageThreshold' parameter passed to 'iSrchTermSearchGetSearchWeightsFromTerm'."); 
        return (SRCH_TermSearchInvalidFrequentTermCoverageThreshold);
    }

    if ( uiStartDocumentID < 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiStartDocumentID' parameter passed to 'iSrchTermSearchGetSearchWeightsFromTerm'."); 
        return (SRCH_TermSearchInvalidDocumentID);
    }

    if ( uiEndDocumentID < 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiEndDocumentID' parameter passed to 'iSrchTermSearchGetSearchWeightsFromTerm'."); 
        return (SRCH_TermSearchInvalidDocumentID);
    }

    if ( ppswSrchWeight == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'ppswSrchWeight' parameter passed to 'iSrchTermSearchGetSearchWeightsFromTerm'."); 
        return (SRCH_ReturnParameterError);
    }


/* printf("pucTerm: '%s'\n", pucTerm); */


    /* Look up the term in the term dict, process any errors */
    {    
        unsigned int    uiTermType = SPI_TERM_TYPE_UNKNOWN;
        unsigned int    uiTermCount = SPI_TERM_COUNT_UNKNOWN;
        unsigned int    uiDocumentCount = SPI_TERM_DOCUMENT_COUNT_UNKNOWN;


        /* Look up the term */
        iError = iSrchTermDictLookup(psiSrchIndex, pucTerm, pucFieldIDBitmap, uiFieldIDBitmapLength, 
                &uiTermType, &uiTermCount, &uiDocumentCount, &ulIndexBlockID);
    
        /* Skip setting the weights if the term does not exist or does not occur
        ** (subject to constraints, like field ID), otherwise return an error if
        ** a real error occured
        */
        if ( (iError == SRCH_TermDictTermNotFound) || (iError == SRCH_TermDictTermDoesNotOccur) ) {
            iError = SRCH_NoError;
            goto bailFromiSrchTermSearchGetSearchWeightsFromTerm;
        }
        else if ( iError != SRCH_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to look up the term: '%s', in the term dictionary, srch error: %d.", pucTerm, iError); 
            goto bailFromiSrchTermSearchGetSearchWeightsFromTerm;
        }


        /* Term is present */
    
        /* Is this a stop term? */
        if ( (uiTermType == SPI_TERM_TYPE_STOP) && (pucFieldIDBitmap == NULL) ) {
            iError = SRCH_NoError;
            goto bailFromiSrchTermSearchGetSearchWeightsFromTerm;
        }
    

        /* See if this term meets the frequent term coverage threshold */
        if ( (fFrequentTermCoverageThreshold > 0) && (uiTermType != SPI_TERM_TYPE_STOP) ) {
            
            float   fTermCoverage = ((float)uiTermCount / psiSrchIndex->uiDocumentCount) * 100;
            
/*             iUtlLogDebug(UTL_LOG_CONTEXT, "pucTerm: '%s', uiTermCount: %u, uiDocumentCount: %u, psiSrchIndex->uiDocumentCount: %u, fTermCoverage: %9.4f%%.",  */
/*                     pucTerm, uiTermCount, uiDocumentCount, psiSrchIndex->uiDocumentCount, fTermCoverage); */
        
            /* Flag this term as frequent if its coverage is greater than the threshold */
            if ( fTermCoverage > fFrequentTermCoverageThreshold ) {
                iError = SRCH_NoError;
                goto bailFromiSrchTermSearchGetSearchWeightsFromTerm;
            }
        }


        /* IDF adjusted weight */
        fAdjustedWeight = SRCH_SEARCH_IDF_FACTOR(uiTermCount, uiDocumentCount, psiSrchIndex->uiDocumentCount) * fWeight;

/*         iUtlLogDebug(UTL_LOG_CONTEXT, "fAdjustedWeight [%f].", fAdjustedWeight);  */
    }



    /* Get the index block for this term, processing any errors */
    {
        unsigned char   *pucIndexBlockPtr = NULL;

        /* Get the index block data */
        if ( (iError = iUtlDataGetEntry(psiSrchIndex->pvUtlIndexData, ulIndexBlockID, (void **)&pucIndexBlock, &uiIndexBlockLength)) != UTL_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to get index data, index object ID: %lu, index: '%s', utl error: %d.", 
                    ulIndexBlockID, psiSrchIndex->pucIndexName, iError);
            iError = SRCH_TermSearchGetObjectFailed;
            goto bailFromiSrchTermSearchGetSearchWeightsFromTerm;
        }
    
        /* Set the pointer to start reading from */
        pucIndexBlockPtr = pucIndexBlock;
    
        /* Get the size of the index block */
        UTL_NUM_READ_COMPRESSED_UINT(uiIndexBlockDataLength, pucIndexBlockPtr);
        uiIndexBlockHeaderLength = pucIndexBlockPtr - pucIndexBlock;

        /* Empty block - probably a stop term */
        if ( uiIndexBlockDataLength == 0 ) {
            iError = SRCH_NoError;
            goto bailFromiSrchTermSearchGetSearchWeightsFromTerm;
        }
    }



    /* Use the search weight structure if we were passed one, otherwise we create a new one */ 
    if ( *ppswSrchWeight != NULL ) {
        
        /* Set the pointer and length */
        pswSrchWeight = *ppswSrchWeight;

        /* Check that it is not mapped */
        if ( pswSrchWeight->bMappedAllocationFlag == true ) {
            return (SRCH_MiscError);
        }
        
        /* And check that it is large enough */
        if ( pswSrchWeight->uiWeightsLength < (psiSrchIndex->uiDocumentCount + 1) ) {
            return (SRCH_MiscError);
        }

        /* Adjust the length */
        pswSrchWeight->uiWeightsLength = psiSrchIndex->uiDocumentCount + 1;
    }
    else {
        
        /* Allocate a new search weights structure */
        if ( (iError = iSrchWeightCreate(NULL, psiSrchIndex->uiDocumentCount + 1, false, &pswSrchWeight)) != SRCH_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to create a new search weight, srch error: %d.", iError);
            goto bailFromiSrchTermSearchGetSearchWeightsFromTerm;
        }
    }



    /* Set the field ID if we can, this allows us to use an optimization further down, 
    ** mmm, optimizations, we like optimizations
    */
    if ( pucFieldIDBitmap != NULL ) {

        unsigned int    uiI = 0;
        unsigned int    uiBitSetCount = 0;

        ASSERT(uiFieldIDBitmapLength == psiSrchIndex->uiFieldIDMaximum);

        /* Loop over each ID, seeing what is set, setting the field ID and the field count - field ID 0 is not a field */ 
        for ( uiI = 0, uiBitSetCount = 0; uiI < psiSrchIndex->uiFieldIDMaximum; uiI++ ) {

            /* See what is set, setting the field ID and the field count - field ID 0 is not a field */ 
            if ( UTL_BITMAP_IS_BIT_SET_IN_POINTER(pucFieldIDBitmap, uiI) ) {
                uiFieldID = uiI + 1;
                uiBitSetCount++;
            }
        }
        
        
        /* Clear the field ID if there is more than one field set, we cant use the optimization, its like... a bummer... */
        if ( uiBitSetCount > 1 ) {
            uiFieldID = 0;
        }
        
        ASSERT(uiBitSetCount > 0);
    }



    /* Optimize the end document ID, this allows us to remove the
    ** 'uiEndDocumentID != 0' clause from the document ID range check
    */
    if ( uiEndDocumentID == 0 ) {
        uiEndDocumentID = UINT_MAX;
    }



    /* Loop over the index block, decoding it and populating the postings array */
    {
        unsigned char   *pucIndexBlockPtr = NULL;
        unsigned char   *pucIndexBlockEndPtr = NULL;

        unsigned int    uiIndexEntryDocumentID = 0;
        unsigned int    uiIndexEntryDeltaDocumentID = 0;
/*         unsigned int    uiIndexEntryDeltaTermPosition = 0; */
        unsigned int    uiIndexEntryFieldID = 0;
/*         unsigned int    uiIndexEntryTermWeight = 0; */
            
        float           *pfWeights = NULL;


        /* Dereference the weights array */
        pfWeights = pswSrchWeight->pfWeights;


        /* uiFieldID > 0 means that this is a fielded search and that there is only one field to match */
        if ( uiFieldID > 0 ) {

            /* Add the entry to the weights array */
            for ( pucIndexBlockPtr = pucIndexBlock + uiIndexBlockHeaderLength, pucIndexBlockEndPtr = pucIndexBlock + uiIndexBlockHeaderLength + uiIndexBlockDataLength; 
                    pucIndexBlockPtr < pucIndexBlockEndPtr; ) {
    
/* Compressed int */
                /* Read the index block, skip the term position */
                UTL_NUM_READ_COMPRESSED_UINT(uiIndexEntryDeltaDocumentID, pucIndexBlockPtr);
                UTL_NUM_SKIP_COMPRESSED_UINT(pucIndexBlockPtr);
                UTL_NUM_READ_COMPRESSED_UINT(uiIndexEntryFieldID, pucIndexBlockPtr);
/*                 UTL_NUM_READ_COMPRESSED_UINT(uiIndexEntryTermWeight, pucIndexBlockPtr); */
        
/* Varint */
                /* Read the index block */
/*                 UTL_NUM_READ_COMPACT_VARINT_TRIO(uiIndexEntryDeltaDocumentID, uiIndexEntryDeltaTermPosition, uiIndexEntryFieldID, pucIndexBlockPtr); */
/*                 UTL_NUM_READ_COMPACT_VARINT_QUAD(uiIndexEntryDeltaDocumentID, uiIndexEntryDeltaTermPosition, uiIndexEntryFieldID, uiIndexEntryTermWeight, pucIndexBlockPtr); */

    
                ASSERT(uiIndexEntryDeltaDocumentID >= 0);
                ASSERT(uiIndexEntryFieldID >= 0);
/*                 ASSERT(uiIndexEntryTermWeight >= 0); */

                /* Set the document ID */
                uiIndexEntryDocumentID += uiIndexEntryDeltaDocumentID;

                /* Check that this is a field we are searching on */
                if ( uiIndexEntryFieldID == uiFieldID ) {
    
#if defined(SRCH_TERMSRCH_ENABLE_DOCUMENT_ID_RANGE_RESTRICTIONS)
                    /* Skip this document if we are not yet in range */
                    if ( uiIndexEntryDocumentID < uiStartDocumentID ) {
                        continue;
                    }
                    
                    /* Break out if we are beyond the range */
                    if ( uiIndexEntryDocumentID > uiEndDocumentID ) {
                        break;
                    }
#endif /* defined(SRCH_TERMSRCH_ENABLE_DOCUMENT_ID_RANGE_RESTRICTIONS) */
    
                    /* Cumulate the weight into the weight array from the passed weigth and the adjustment */
                    pfWeights[uiIndexEntryDocumentID] += fAdjustedWeight;
    
/*                     iUtlLogDebug(UTL_LOG_CONTEXT, "pucTerm: [%s], uiIndexEntryDocumentID: %u, uiIndexEntryFieldID: %u, pfWeights[uiIndexEntryDocumentID]: %f",  */
/*                             pucTerm, uiIndexEntryDocumentID, uiIndexEntryFieldID, pfWeights[uiIndexEntryDocumentID]); */
                }
            }
        }

        /* pucFieldIDBitmap != NULL means that this is a fielded search and there is more than one field to match */
        else if ( pucFieldIDBitmap != NULL ) {

            /* Add the entry to the weights array */
            for ( pucIndexBlockPtr = pucIndexBlock + uiIndexBlockHeaderLength, pucIndexBlockEndPtr = pucIndexBlock + uiIndexBlockHeaderLength + uiIndexBlockDataLength; 
                    pucIndexBlockPtr < pucIndexBlockEndPtr; ) {
    
/* Compressed int */
                /* Read the index block, skip the term position */
                UTL_NUM_READ_COMPRESSED_UINT(uiIndexEntryDeltaDocumentID, pucIndexBlockPtr);
                UTL_NUM_SKIP_COMPRESSED_UINT(pucIndexBlockPtr);
                UTL_NUM_READ_COMPRESSED_UINT(uiIndexEntryFieldID, pucIndexBlockPtr);
/*                 UTL_NUM_READ_COMPRESSED_UINT(uiIndexEntryTermWeight, pucIndexBlockPtr); */
        
/* Varint */
                /* Read the index block */
/*                 UTL_NUM_READ_COMPACT_VARINT_TRIO(uiIndexEntryDeltaDocumentID, uiIndexEntryDeltaTermPosition, uiIndexEntryFieldID, pucIndexBlockPtr); */
/*                 UTL_NUM_READ_COMPACT_VARINT_QUAD(uiIndexEntryDeltaDocumentID, uiIndexEntryDeltaTermPosition, uiIndexEntryFieldID, uiIndexEntryTermWeight, pucIndexBlockPtr); */


                ASSERT(uiIndexEntryDeltaDocumentID >= 0);
                ASSERT((uiIndexEntryFieldID >= 0) && (uiIndexEntryFieldID <= uiFieldIDBitmapLength));
/*                 ASSERT(uiIndexEntryTermWeight >= 0); */

                /* Set the document ID */
                uiIndexEntryDocumentID += uiIndexEntryDeltaDocumentID;

                /* Check that this is a field we are searching on - field ID 0 is not a field */
                if ( UTL_BITMAP_IS_BIT_SET_IN_POINTER(pucFieldIDBitmap, uiIndexEntryFieldID - 1) ) {
    
#if defined(SRCH_TERMSRCH_ENABLE_DOCUMENT_ID_RANGE_RESTRICTIONS)
                    /* Skip this document if we are not yet in range */
                    if ( uiIndexEntryDocumentID < uiStartDocumentID ) {
                        continue;
                    }
                    
                    /* Break out if we are beyond the range */
                    if ( uiIndexEntryDocumentID > uiEndDocumentID ) {
                        break;
                    }
#endif /* defined(SRCH_TERMSRCH_ENABLE_DOCUMENT_ID_RANGE_RESTRICTIONS) */

                    /* Cumulate the weight into the weight array from the passed weigth and the adjustment */
                    pfWeights[uiIndexEntryDocumentID] += fAdjustedWeight;
    
/*                     iUtlLogDebug(UTL_LOG_CONTEXT, "pucTerm: [%s], uiIndexEntryDocumentID: %u, uiIndexEntryFieldID: %u, pfWeights[uiIndexEntryDocumentID]: %f",  */
/*                             pucTerm, uiIndexEntryDocumentID, uiIndexEntryFieldID, pfWeights[uiIndexEntryDocumentID]); */
                }
            }
        }

        /* uiFieldID == 0 && pucFieldIDBitmap == NULL means that this is not a fielded search */
        else {

            /* Add the entry to the weights array */
            for ( pucIndexBlockPtr = pucIndexBlock + uiIndexBlockHeaderLength, pucIndexBlockEndPtr = pucIndexBlock + uiIndexBlockHeaderLength + uiIndexBlockDataLength; 
                    pucIndexBlockPtr < pucIndexBlockEndPtr; ) {
    
/* Compressed int */
                /* Read the index block, skip the term position and the field ID */
                UTL_NUM_READ_COMPRESSED_UINT(uiIndexEntryDeltaDocumentID, pucIndexBlockPtr);
                UTL_NUM_SKIP_COMPRESSED_UINT(pucIndexBlockPtr);
                UTL_NUM_SKIP_COMPRESSED_UINT(pucIndexBlockPtr);
/*                 UTL_NUM_READ_COMPRESSED_UINT(uiIndexEntryTermWeight, pucIndexBlockPtr); */
        
/* Varint */
                /* Read the index block */
/*                 UTL_NUM_READ_COMPACT_VARINT_TRIO(uiIndexEntryDeltaDocumentID, uiIndexEntryDeltaTermPosition, uiIndexEntryFieldID, pucIndexBlockPtr); */
/*                 UTL_NUM_READ_COMPACT_VARINT_QUAD(uiIndexEntryDeltaDocumentID, uiIndexEntryDeltaTermPosition, uiIndexEntryFieldID, uiIndexEntryTermWeight, pucIndexBlockPtr); */

                ASSERT(uiIndexEntryDeltaDocumentID >= 0);
/*                 ASSERT(uiIndexEntryTermWeight >= 0); */


                /* Set the document ID */
                uiIndexEntryDocumentID += uiIndexEntryDeltaDocumentID;

#if defined(SRCH_TERMSRCH_ENABLE_DOCUMENT_ID_RANGE_RESTRICTIONS)
                /* Skip this document if we are not yet in range */
                if ( uiIndexEntryDocumentID < uiStartDocumentID ) {
                    continue;
                }
                
                /* Break out if we are beyond the range */
                if ( uiIndexEntryDocumentID > uiEndDocumentID ) {
                    break;
                }
#endif /* defined(SRCH_TERMSRCH_ENABLE_DOCUMENT_ID_RANGE_RESTRICTIONS) */

                /* Cumulate the weight into the weight array from the passed weigth and the adjustment */
                pfWeights[uiIndexEntryDocumentID] += fAdjustedWeight;
    
/*                 iUtlLogDebug(UTL_LOG_CONTEXT, "pucTerm: [%s], uiIndexEntryDocumentID: %u, pfWeights[uiIndexEntryDocumentID]: %f",  */
/*                         pucTerm, uiIndexEntryDocumentID, pfWeights[uiIndexEntryDocumentID]); */
            }
        }
    }



    /* Bail label */
    bailFromiSrchTermSearchGetSearchWeightsFromTerm:


    /* The search weights was allocated if it was not passed, so we need to do something with it */ 
    if ( *ppswSrchWeight == NULL ) {
        
        /* Handle the error */
        if ( iError == SRCH_NoError ) {

            /* Set the return pointer */
            *ppswSrchWeight = pswSrchWeight;
        }
        else {
        
            /* Free allocations */
            s_free(pswSrchWeight);
        }
    }


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchTermSearchGetSearchBitmapFromTerm()

    Purpose:    This function searches for a single term and sets the bitmap entry
                for that document to 1 if the document exists.

    Parameters: pssSrchSearch                       search structure
                psiSrchIndex                        index structure
                pucTerm                             term to search for
                pucFieldIDBitmap                    field id bitmap to search (optional)
                uiFieldIDBitmapLength               field id bitmap length (optional)
                fFrequentTermCoverageThreshold      frequent term coverage threshold (0 indicates no threshold)
                uiStartDocumentID                   start document ID restriction (0 for no restriction)
                uiEndDocumentID                     end document ID restriction (0 for no restriction)
                ppsbSrchBitmap                      return pointer for the search bitmap structure (allocated if null, or unusable)

    Globals:    none

    Returns:    SRCH error code

*/
int iSrchTermSearchGetSearchBitmapFromTerm
(
    struct srchSearch *pssSrchSearch,
    struct srchIndex *psiSrchIndex,
    unsigned char *pucTerm,
    unsigned char *pucFieldIDBitmap,
    unsigned int uiFieldIDBitmapLength,
    float fFrequentTermCoverageThreshold,
    unsigned int uiStartDocumentID,
    unsigned int uiEndDocumentID,
    struct srchBitmap **ppsbSrchBitmap
)
{

    int                 iError = UTL_NoError;

    unsigned long       ulIndexBlockID = 0;
    unsigned char       *pucIndexBlock = NULL;
    unsigned int        uiIndexBlockLength = 0;
    unsigned int        uiIndexBlockHeaderLength = 0;
    unsigned int        uiIndexBlockDataLength = 0;
    
    unsigned int        uiFieldID = 0;

    struct srchBitmap   *psbSrchBitmap = NULL;


    /* Check the parameters */
    if ( pssSrchSearch == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pssSrchSearch' parameter passed to 'iSrchTermSearchGetSearchBitmapFromTerm'."); 
        return (SRCH_TermSearchInvalidSearch);
    }

    if ( psiSrchIndex == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'psiSrchIndex' parameter passed to 'iSrchTermSearchGetSearchBitmapFromTerm'."); 
        return (SRCH_InvalidIndex);
    }

    if ( bUtlStringsIsStringNULL(pucTerm) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pucTerm' parameter passed to 'iSrchTermSearchGetSearchBitmapFromTerm'."); 
        return (SRCH_TermSearchInvalidTerm);
    }

    if ( ((pucFieldIDBitmap == NULL) && (uiFieldIDBitmapLength > 0)) || ((pucFieldIDBitmap != NULL) && (uiFieldIDBitmapLength <= 0)) ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'pucFieldIDBitmap' & 'uiFieldIDBitmapLength' parameters passed to 'iSrchTermSearchGetSearchBitmapFromTerm'."); 
        return (SRCH_TermSearchInvalidFieldIDBitMap);
    }

    if ( fFrequentTermCoverageThreshold < 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'fFrequentTermCoverageThreshold' parameter passed to 'iSrchTermSearchGetSearchBitmapFromTerm'."); 
        return (SRCH_TermSearchInvalidFrequentTermCoverageThreshold);
    }

    if ( uiStartDocumentID < 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiStartDocumentID' parameter passed to 'iSrchTermSearchGetSearchBitmapFromTerm'."); 
        return (SRCH_TermSearchInvalidDocumentID);
    }

    if ( uiEndDocumentID < 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiEndDocumentID' parameter passed to 'iSrchTermSearchGetSearchBitmapFromTerm'."); 
        return (SRCH_TermSearchInvalidDocumentID);
    }

    if ( ppsbSrchBitmap == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'ppsbSrchBitmap' parameter passed to 'iSrchTermSearchGetSearchBitmapFromTerm'."); 
        return (SRCH_ReturnParameterError);
    }


/* printf("pucTerm: '%s'\n", pucTerm); */


    /* Look up the term in the term dict, process any errors */
    {    
        unsigned int    uiTermType = SPI_TERM_TYPE_UNKNOWN;
        unsigned int    uiTermCount = SPI_TERM_COUNT_UNKNOWN;
        unsigned int    uiDocumentCount = SPI_TERM_DOCUMENT_COUNT_UNKNOWN;


        /* Look up the term */
        iError = iSrchTermDictLookup(psiSrchIndex, pucTerm, pucFieldIDBitmap, uiFieldIDBitmapLength, 
                &uiTermType, &uiTermCount, &uiDocumentCount, &ulIndexBlockID);
    
        /* Skip setting the bitmap if the term does not exist or does not occur
        ** (subject to constraints, like field ID), otherwise return an error if
        ** a real error occured
        */
        if ( (iError == SRCH_TermDictTermNotFound) || (iError == SRCH_TermDictTermDoesNotOccur) ) {
            iError = SRCH_NoError;
            goto bailFromiSrchTermSearchGetSearchBitmapFromTerm;
        }
        else if ( iError != SRCH_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to look up the term: '%s', in the term dictionary, srch error: %d.", pucTerm, iError); 
            goto bailFromiSrchTermSearchGetSearchBitmapFromTerm;
        }


        /* Term is present */
    
        /* Is this a stop term? */
        if ( (uiTermType == SPI_TERM_TYPE_STOP) && (pucFieldIDBitmap == NULL) ) {
            iError = SRCH_NoError;
            goto bailFromiSrchTermSearchGetSearchBitmapFromTerm;
        }
        

        /* See if this term meets the frequent term coverage threshold */
        if ( (fFrequentTermCoverageThreshold > 0) && (uiTermType != SPI_TERM_TYPE_STOP) ) {
            
            float   fTermCoverage = ((float)uiTermCount / psiSrchIndex->uiDocumentCount) * 100;
            
/*             iUtlLogDebug(UTL_LOG_CONTEXT, "pucTerm: '%s', uiTermCount: %u, uiDocumentCount: %u, psiSrchIndex->uiDocumentCount: %u, fTermCoverage: %9.4f%%.",  */
/*                     pucTerm, uiTermCount, uiDocumentCount, psiSrchIndex->uiDocumentCount, fTermCoverage); */
        
            /* Skip this term as frequent if its coverage is greater than the threshold */
            if ( fTermCoverage > fFrequentTermCoverageThreshold ) {
                iError = SRCH_NoError;
                goto bailFromiSrchTermSearchGetSearchBitmapFromTerm;
            }
        }
    }



    /* Get the index block for this term, processing any errors */
    {
        unsigned char   *pucIndexBlockPtr = NULL;

        /* Get the index block data */
        if ( (iError = iUtlDataGetEntry(psiSrchIndex->pvUtlIndexData, ulIndexBlockID, (void **)&pucIndexBlock, &uiIndexBlockLength)) != UTL_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to get index data, index object ID: %lu, index: '%s', utl error: %d.", 
                    ulIndexBlockID, psiSrchIndex->pucIndexName, iError);
            iError = SRCH_TermSearchGetObjectFailed;
            goto bailFromiSrchTermSearchGetSearchBitmapFromTerm;
        }
    
        /* Set the pointer to start reading from */
        pucIndexBlockPtr = pucIndexBlock;
    
        /* Get the size of the index block */
        UTL_NUM_READ_COMPRESSED_UINT(uiIndexBlockDataLength, pucIndexBlockPtr);
        uiIndexBlockHeaderLength = pucIndexBlockPtr - pucIndexBlock;

        /* Empty block - probably a stop term */
        if ( uiIndexBlockDataLength == 0 ) {
            iError = SRCH_NoError;
            goto bailFromiSrchTermSearchGetSearchBitmapFromTerm;
        }
    }



    /* Use the search bitmap structure if we were passed one, otherwise we create a new one */ 
    if ( *ppsbSrchBitmap != NULL ) {
        
        /* Set the pointer */
        psbSrchBitmap = *ppsbSrchBitmap;

        /* Check that it is not mapped */
        if ( psbSrchBitmap->bMappedAllocationFlag == true ) {
            return (SRCH_MiscError);
        }
        
        /* And check that it is large enough */
        if ( psbSrchBitmap->uiBitmapLength < (psiSrchIndex->uiDocumentCount + 1) ) {
            return (SRCH_MiscError);
        }

        /* Update the length */
        psbSrchBitmap->uiBitmapLength = psiSrchIndex->uiDocumentCount + 1;
    }
    else {

        /* Allocate a new search bitmap structure */
        if ( (iError = iSrchBitmapCreate(NULL, psiSrchIndex->uiDocumentCount + 1, false, ppsbSrchBitmap)) != SRCH_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to create a new search bitmap, srch error: %d.", iError);
            goto bailFromiSrchTermSearchGetSearchBitmapFromTerm;
        }

        /* Set the pointer */
        psbSrchBitmap = *ppsbSrchBitmap;
    }



    /* Set the field ID if we can, this allows us to use an optimization further down, 
    ** mmm, optimizations, we like optimizations
    */
    if ( pucFieldIDBitmap != NULL ) {

        unsigned int    uiI = 0;
        unsigned int    uiBitSetCount = 0;

        ASSERT(uiFieldIDBitmapLength == psiSrchIndex->uiFieldIDMaximum);

        /* Loop over each field ID - field ID 0 is not a field */ 
        for ( uiI = 0, uiBitSetCount = 0; uiI < psiSrchIndex->uiFieldIDMaximum; uiI++ ) {

            /* See what is set, setting the field ID and the field count - field ID 0 is not a field */ 
            if ( UTL_BITMAP_IS_BIT_SET_IN_POINTER(pucFieldIDBitmap, uiI) ) {
                uiFieldID = uiI + 1;
                uiBitSetCount++;
            }
        }
        
        /* Clear the field ID if there is more than one field set, we cant use the optimization, its like... a bummer... */
        if ( uiBitSetCount > 1 ) {
            uiFieldID = 0;
        }
        
        ASSERT(uiBitSetCount > 0);
    }



    /* Optimize the end document ID, this allows us to remove the
    ** 'uiEndDocumentID != 0' clause from the document ID range check
    */
    if ( uiEndDocumentID == 0 ) {
        uiEndDocumentID = UINT_MAX;
    }



    /* Loop over the index block, decoding it and populating the postings array */
    {
        unsigned char   *pucIndexBlockPtr = NULL;
        unsigned char   *pucIndexBlockEndPtr = NULL;

        unsigned int    uiIndexEntryDocumentID = 0;
        unsigned int    uiIndexEntryDeltaDocumentID = 0;
/*         unsigned int    uiIndexEntryDeltaTermPosition = 0; */
        unsigned int    uiIndexEntryFieldID = 0;
/*         unsigned int     uiIndexEntryTermWeight = 0; */
            
        /* Dereference the bitmap array */
        unsigned char   *pucBitmapPtr = psbSrchBitmap->pucBitmap;


        /* uiFieldID > 0 means that this is a fielded search and that there is only one field to match */
        if ( uiFieldID > 0 ) {

            /* Add the entry to the weights array */
            for ( pucIndexBlockPtr = pucIndexBlock + uiIndexBlockHeaderLength, pucIndexBlockEndPtr = pucIndexBlock + uiIndexBlockHeaderLength + uiIndexBlockDataLength; 
                    pucIndexBlockPtr < pucIndexBlockEndPtr; ) {
    
/* Compressed int */
                /* Read the index block, skip the term position */
                UTL_NUM_READ_COMPRESSED_UINT(uiIndexEntryDeltaDocumentID, pucIndexBlockPtr);
                UTL_NUM_SKIP_COMPRESSED_UINT(pucIndexBlockPtr);
                UTL_NUM_READ_COMPRESSED_UINT(uiIndexEntryFieldID, pucIndexBlockPtr);
/*                 UTL_NUM_READ_COMPRESSED_UINT(uiIndexEntryTermWeight, pucIndexBlockPtr); */
        
/* Varint */
                /* Read the index block */
/*                 UTL_NUM_READ_COMPACT_VARINT_TRIO(uiIndexEntryDeltaDocumentID, uiIndexEntryDeltaTermPosition, uiIndexEntryFieldID, pucIndexBlockPtr); */
/*                 UTL_NUM_READ_COMPACT_VARINT_QUAD(uiIndexEntryDeltaDocumentID, uiIndexEntryDeltaTermPosition, uiIndexEntryFieldID, uiIndexEntryTermWeight, pucIndexBlockPtr); */

    
                ASSERT(uiIndexEntryDeltaDocumentID >= 0);
                ASSERT(uiIndexEntryFieldID >= 0);
/*                 ASSERT(uiIndexEntryTermWeight >= 0); */

                /* Set the document ID */
                uiIndexEntryDocumentID += uiIndexEntryDeltaDocumentID;

                /* Check that this is a field we are searching on */
                if ( uiIndexEntryFieldID == uiFieldID ) {
    
#if defined(SRCH_TERMSRCH_ENABLE_DOCUMENT_ID_RANGE_RESTRICTIONS)
                    /* Skip this document if we are not yet in range */
                    if ( uiIndexEntryDocumentID < uiStartDocumentID ) {
                        continue;
                    }
                    
                    /* Break out if we are beyond the range */
                    if ( uiIndexEntryDocumentID > uiEndDocumentID ) {
                        break;
                    }
#endif /* defined(SRCH_TERMSRCH_ENABLE_DOCUMENT_ID_RANGE_RESTRICTIONS) */

                    /* Set the bit */
                    UTL_BITMAP_SET_BIT_IN_POINTER(pucBitmapPtr, uiIndexEntryDocumentID);
    
/*                     iUtlLogDebug(UTL_LOG_CONTEXT, "pucTerm: [%s], uiIndexEntryDocumentID: %u, uiIndexEntryFieldID: %u",  */
/*                             pucTerm, uiIndexEntryDocumentID, uiIndexEntryFieldID); */
                }
            }
        }

        /* pucFieldIDBitmap != NULL means that this is a fielded search and there is more than one field to match */
        else if ( pucFieldIDBitmap != NULL ) {

            /* Add the entry to the weights array */
            for ( pucIndexBlockPtr = pucIndexBlock + uiIndexBlockHeaderLength, pucIndexBlockEndPtr = pucIndexBlock + uiIndexBlockHeaderLength + uiIndexBlockDataLength; 
                    pucIndexBlockPtr < pucIndexBlockEndPtr; ) {
    
/* Compressed int */
                /* Read the index block, skip the term position */
                UTL_NUM_READ_COMPRESSED_UINT(uiIndexEntryDeltaDocumentID, pucIndexBlockPtr);
                UTL_NUM_SKIP_COMPRESSED_UINT(pucIndexBlockPtr);
                UTL_NUM_READ_COMPRESSED_UINT(uiIndexEntryFieldID, pucIndexBlockPtr);
/*                 UTL_NUM_READ_COMPRESSED_UINT(uiIndexEntryTermWeight, pucIndexBlockPtr); */
        
/* Varint */
                /* Read the index block */
/*                 UTL_NUM_READ_COMPACT_VARINT_TRIO(uiIndexEntryDeltaDocumentID, uiIndexEntryDeltaTermPosition, uiIndexEntryFieldID, pucIndexBlockPtr); */
/*                 UTL_NUM_READ_COMPACT_VARINT_QUAD(uiIndexEntryDeltaDocumentID, uiIndexEntryDeltaTermPosition, uiIndexEntryFieldID, uiIndexEntryTermWeight, pucIndexBlockPtr); */
    
                ASSERT(uiIndexEntryDeltaDocumentID >= 0);
                ASSERT((uiIndexEntryFieldID >= 0) && (uiIndexEntryFieldID <= uiFieldIDBitmapLength));
/*                 ASSERT(uiIndexEntryTermWeight >= 0); */

                /* Set the document ID */
                uiIndexEntryDocumentID += uiIndexEntryDeltaDocumentID;

                /* Check that this is a field we are searching on - field ID 0 is not a field */
                if ( UTL_BITMAP_IS_BIT_SET_IN_POINTER(pucFieldIDBitmap, uiIndexEntryFieldID - 1) ) {
    
#if defined(SRCH_TERMSRCH_ENABLE_DOCUMENT_ID_RANGE_RESTRICTIONS)
                    /* Skip this document if we are not yet in range */
                    if ( uiIndexEntryDocumentID < uiStartDocumentID ) {
                        continue;
                    }
                    
                    /* Break out if we are beyond the range */
                    if ( uiIndexEntryDocumentID > uiEndDocumentID ) {
                        break;
                    }
#endif /* defined(SRCH_TERMSRCH_ENABLE_DOCUMENT_ID_RANGE_RESTRICTIONS) */

                    /* Set the bit */
                    UTL_BITMAP_SET_BIT_IN_POINTER(pucBitmapPtr, uiIndexEntryDocumentID);
    
/*                     iUtlLogDebug(UTL_LOG_CONTEXT, "pucTerm: [%s], uiIndexEntryDocumentID: %u, uiIndexEntryFieldID: %u",  */
/*                             pucTerm, uiIndexEntryDocumentID, uiIndexEntryFieldID); */
                }
            }
        }

        /* uiFieldID == 0 && pucFieldIDBitmap == NULL means that this is not a fielded search */
        else {

            /* Add the entry to the weights array */
            for ( pucIndexBlockPtr = pucIndexBlock + uiIndexBlockHeaderLength, pucIndexBlockEndPtr = pucIndexBlock + uiIndexBlockHeaderLength + uiIndexBlockDataLength; 
                    pucIndexBlockPtr < pucIndexBlockEndPtr; ) {
    
/* Compressed int */
                /* Read the index block, skip the term position and the field ID */
                UTL_NUM_READ_COMPRESSED_UINT(uiIndexEntryDeltaDocumentID, pucIndexBlockPtr);
                UTL_NUM_SKIP_COMPRESSED_UINT(pucIndexBlockPtr);
                UTL_NUM_SKIP_COMPRESSED_UINT(pucIndexBlockPtr);
/*                 UTL_NUM_READ_COMPRESSED_UINT(uiIndexEntryTermWeight, pucIndexBlockPtr); */
        
/* Varint */
                /* Read the index block */
/*                 UTL_NUM_READ_COMPACT_VARINT_TRIO(uiIndexEntryDeltaDocumentID, uiIndexEntryDeltaTermPosition, uiIndexEntryFieldID, pucIndexBlockPtr); */
/*                 UTL_NUM_READ_COMPACT_VARINT_QUAD(uiIndexEntryDeltaDocumentID, uiIndexEntryDeltaTermPosition, uiIndexEntryFieldID, uiIndexEntryTermWeight, pucIndexBlockPtr); */

                ASSERT(uiIndexEntryDeltaDocumentID >= 0);
/*                 ASSERT(uiIndexEntryTermWeight >= 0); */

                /* Set the document ID */
                uiIndexEntryDocumentID += uiIndexEntryDeltaDocumentID;

#if defined(SRCH_TERMSRCH_ENABLE_DOCUMENT_ID_RANGE_RESTRICTIONS)
                /* Skip this document if we are not yet in range */
                if ( uiIndexEntryDocumentID < uiStartDocumentID ) {
                    continue;
                }
                
                /* Break out if we are beyond the range */
                if ( uiIndexEntryDocumentID > uiEndDocumentID ) {
                    break;
                }
#endif /* defined(SRCH_TERMSRCH_ENABLE_DOCUMENT_ID_RANGE_RESTRICTIONS) */

                /* Set the bit */
                UTL_BITMAP_SET_BIT_IN_POINTER(pucBitmapPtr, uiIndexEntryDocumentID);

/* printf("uiIndexEntryDocumentID: %u\n", uiIndexEntryDocumentID); */
    
/*                 iUtlLogDebug(UTL_LOG_CONTEXT, "pucTerm: [%s], uiIndexEntryDocumentID: %u, uiIndexEntryFieldIDn: %u",  */
/*                         pucTerm, uiIndexEntryDocumentID, uiIndexEntryFieldIDn); */
            }
        }
    }


    
    /* Bail label */
    bailFromiSrchTermSearchGetSearchBitmapFromTerm:


    /* The bitmap was allocated if it was not passed, so we need to do something with it */ 
    if ( *ppsbSrchBitmap == NULL ) {

        /* Handle the error */
        if ( iError != SRCH_NoError ) {

            /* Set the return pointer */
            *ppsbSrchBitmap = psbSrchBitmap;
        }
        else {

            /* Free the resources */
            iSrchBitmapFree(psbSrchBitmap);
            psbSrchBitmap = NULL;
        }
    }


    return (iError);

}


/*---------------------------------------------------------------------------*/


