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

    Module:     posting.c

    Author:     Francois Schiettecatte (FS Consulting LLC.)

    Created:    21 January 2006

    Purpose:    This module provides support functions for search.c for 
                processing postings 

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
#define UTL_LOG_CONTEXT                     (unsigned char *)"com.fsconsult.mps.src.search.posting"


/*---------------------------------------------------------------------------*/

/*
** Feature Defines
*/

/* Enable proximity reweighting */
#define SRCH_POSTING_ENABLE_PROXIMITY_REWEIGHTING


/*---------------------------------------------------------------------------*/

/*
** Defines
*/

#if defined(SRCH_POSTING_ENABLE_PROXIMITY_REWEIGHTING)
#define SRCH_POSTING_PROXIMITY_REWEIGHTING                  (3)
#endif    /* defined(SRCH_POSTING_ENABLE_PROXIMITY_REWEIGHTING) */


/*---------------------------------------------------------------------------*/


/*
** Macros
*/

/* Macros for moving search posting structure content around */
#define SRCH_POSTING_COPY_SRCH_POSTING(psp1, psp2) \
{   \
    (psp1)->uiDocumentID = (psp2)->uiDocumentID; \
    (psp1)->uiTermPosition = (psp2)->uiTermPosition; \
    (psp1)->fWeight = (psp2)->fWeight; \
}

#define SRCH_POSTING_SWAP_SRCH_POSTING(psp1, psp2) \
{   \
    struct srchPosting spMacroSrchPosting; \
    SRCH_POSTING_COPY_SRCH_POSTING(&spMacroSrchPosting, psp1); \
    SRCH_POSTING_COPY_SRCH_POSTING(psp1, psp2); \
    SRCH_POSTING_COPY_SRCH_POSTING(psp2, &spMacroSrchPosting); \
}


/* Macros for moving search postings list structure content around */
#define SRCH_POSTING_COPY_SRCH_POSTINGS_LIST(psp1, psp2) \
{   \
    (psp1)->uiTermType = (psp2)->uiTermType; \
    (psp1)->uiTermCount = (psp2)->uiTermCount; \
    (psp1)->uiDocumentCount = (psp2)->uiDocumentCount; \
    (psp1)->pspSrchPostings = (psp2)->pspSrchPostings; \
    (psp1)->ulPostingLen = (psp2)->ulPostingLen; \
}

#define SRCH_POSTING_SWAP_SRCH_POSTINGS_LIST(psp1, psp2) \
{   \
    struct srchPostingsList splMacroSrchPostingsList; \
    SRCH_POSTING_COPY_SRCH_POSTINGS_LIST(&splMacroSrchPostingsList, psp1); \
    SRCH_POSTING_COPY_SRCH_POSTINGS_LIST(psp1, psp2); \
    SRCH_POSTING_COPY_SRCH_POSTINGS_LIST(psp2, &splMacroSrchPostingsList); \
}


/*---------------------------------------------------------------------------*/


/*
** Private function prototypes
*/

static int iSrchPostingPrintSrchPostingsList (struct srchPostingsList *psplSrchPostingsList);


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchPostingCreateSrchPostingsList()

    Purpose:    This function creates a new search postings list structure

    Parameters: uiTermType              term type
                uiTermCount             term count
                uiDocumentCount         document count
                bRequired               required
                pspSrchPostings         search postings (optional)
                uiSrchPostingsLength    search postings length
                ppsplSrchPostingsList   return pointer for the search postings list

    Globals:    none

    Returns:    SRCH error code

*/
int iSrchPostingCreateSrchPostingsList
(
    unsigned int uiTermType, 
    unsigned int uiTermCount, 
    unsigned int uiDocumentCount, 
    boolean bRequired, 
    struct srchPosting *pspSrchPostings, 
    unsigned int uiSrchPostingsLength,
    struct srchPostingsList **ppsplSrchPostingsList
)
{

    struct srchPostingsList     *psplSrchPostingsList = NULL;


    if ( ppsplSrchPostingsList == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppsplSrchPostingsList' parameter passed to 'iSrchPostingCreateSrchPostingsList'."); 
        return (SRCH_ReturnParameterError);
    }


    /* Allocate the search postings list structure */
    if ( (psplSrchPostingsList = (struct srchPostingsList *)s_malloc(sizeof(struct srchPostingsList))) == NULL ) {
        return (SRCH_MemError);
    }


    /* Set the fields */
    psplSrchPostingsList->uiTermType = uiTermType;
    psplSrchPostingsList->uiTermCount = uiTermCount;
    psplSrchPostingsList->uiDocumentCount = uiDocumentCount;
    psplSrchPostingsList->bRequired = bRequired;

    
    /* Different path depending whether existing search postings were passed or not */
    if ( pspSrchPostings != NULL ) {

        /* Existing search postings, so we just had over the values/pointers */
        psplSrchPostingsList->uiSrchPostingsLength = uiSrchPostingsLength;
        psplSrchPostingsList->pspSrchPostings = pspSrchPostings;
    }
    else if ( uiSrchPostingsLength > 0 ) {

        /* New search postings so we allocate new search postings */ 
        psplSrchPostingsList->uiSrchPostingsLength = uiSrchPostingsLength;
        
        /* Allocate a new postings list */
        if ( (psplSrchPostingsList->pspSrchPostings = (struct srchPosting *)s_malloc((size_t)((psplSrchPostingsList->uiSrchPostingsLength) * sizeof(struct srchPosting)))) == NULL ) {
            iSrchPostingFreeSrchPostingsList(psplSrchPostingsList);
            psplSrchPostingsList = NULL;
            return (SRCH_MemError);
        }
    }


    /* Set the return pointer */
    *ppsplSrchPostingsList = psplSrchPostingsList;


    return (SRCH_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchPostingFreeSrchPostingsList()

    Purpose:    This function frees the search postings list structure

    Parameters: psplSrchPostingsList    search postings list structure

    Globals:    none

    Returns:    SRCH error code

*/
int iSrchPostingFreeSrchPostingsList
(
    struct srchPostingsList *psplSrchPostingsList
)
{

    /* Free the search postings list structure */
    if ( psplSrchPostingsList != NULL ) {
        s_free(psplSrchPostingsList->pspSrchPostings);
        s_free(psplSrchPostingsList);
    }


    return (SRCH_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchPostingCheckSrchPostingsList()

    Purpose:    This function check out the contents of the search postings list structure.
                This is used for debuging purposed only.

    Parameters: psplSrchPostingsList    search postings list structure

    Globals:    none

    Returns:    SRCH error code

*/
int iSrchPostingCheckSrchPostingsList
(
    struct srchPostingsList *psplSrchPostingsList
)
{

    struct srchPosting      *pspSrchPostingsPtr = NULL;
    struct srchPosting      *pspSrchPostingsEnd = NULL;
    unsigned int            uiCurrentDocumentID = 0;
    unsigned int            uiTermCount = 0;
    int                     uiTermPosition = 0;

/*     iSrchPostingPrintSrchPostingsList(psplSrchPostingsList); */

    /* Check the parameters */
    if ( psplSrchPostingsList == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'psplSrchPostingsList' parameter passed to 'iSrchPostingCheckSrchPostingsList'."); 
        return (SRCH_PostingInvalidPostingsList);
    }

    if ( (psplSrchPostingsList->pspSrchPostings == NULL) && (psplSrchPostingsList->uiSrchPostingsLength > 0) ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'psplSrchPostingsList->pspSrchPostings' & 'psplSrchPostingsList->uiSrchPostingsLength' parameters passed to 'iSrchPostingCheckSrchPostingsList'."); 
        return (SRCH_PostingInvalidPostingsList);
    }

    if ( SPI_TERM_TYPE_VALID(psplSrchPostingsList->uiTermType) == false ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'psplSrchPostingsList->uiTermType' parameter passed to 'iSrchPostingCheckSrchPostingsList'."); 
        return (SRCH_PostingInvalidPostingsList);
    }


    /* Check the sequence information in this  */
    for ( pspSrchPostingsPtr = psplSrchPostingsList->pspSrchPostings, 
            pspSrchPostingsEnd = psplSrchPostingsList->pspSrchPostings + psplSrchPostingsList->uiSrchPostingsLength, 
            uiCurrentDocumentID = 0, uiTermCount= 0; pspSrchPostingsPtr < pspSrchPostingsEnd; pspSrchPostingsPtr++ ) {

        ASSERT(pspSrchPostingsPtr->uiDocumentID >= uiCurrentDocumentID);

        if ( pspSrchPostingsPtr->uiDocumentID > uiCurrentDocumentID ) {

            ASSERT(pspSrchPostingsPtr->uiDocumentID > uiCurrentDocumentID);

            if ( uiTermCount > 1 ) {
                ASSERT((pspSrchPostingsPtr - 2) >= psplSrchPostingsList->pspSrchPostings);
                ASSERT((pspSrchPostingsPtr - 1)->uiTermPosition >= (pspSrchPostingsPtr - 2)->uiTermPosition);
            }

            uiTermPosition = 0;
            uiTermCount = 0;
            uiCurrentDocumentID = pspSrchPostingsPtr->uiDocumentID;
        }
        else {

            ASSERT(pspSrchPostingsPtr->uiDocumentID == uiCurrentDocumentID);

            if ( uiTermCount > 0 ) {
                ASSERT((pspSrchPostingsPtr - 1) >= psplSrchPostingsList->pspSrchPostings);
                ASSERT(pspSrchPostingsPtr->uiTermPosition >= (pspSrchPostingsPtr - 1)->uiTermPosition);
            }

            uiTermPosition = pspSrchPostingsPtr->uiTermPosition;
            uiTermCount++;
        }
    }


    return (SRCH_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchPostingMergeSrchPostingsListsOR()

    Purpose:    This function ORs psplSrchPostingsList2 into psplSrchPostingsList1 
                and returns a pointer to a new search postings list structure.

    Parameters: psplSrchPostingsList1               search postings list structure
                psplSrchPostingsList2               search postings list structure
                uiSrchPostingBooleanOperationID     posting boolean operation ID
                ppsplSrchPostingsList               return pointer for the search postings list structure

    Globals:    none

    Returns:    SRCH error code

*/
int iSrchPostingMergeSrchPostingsListsOR
(
    struct srchPostingsList *psplSrchPostingsList1,
    struct srchPostingsList *psplSrchPostingsList2,
    unsigned int uiSrchPostingBooleanOperationID,
    struct srchPostingsList **ppsplSrchPostingsList
)
{

    int                         iError = SRCH_NoError;
    unsigned int                uiSrchPostingsLength = 0;
    struct srchPostingsList     *psplSrchPostingsList = NULL;
    struct srchPosting          *pspSrchPostingsPtr1 = NULL;
    struct srchPosting          *pspSrchPostingsPtr2 = NULL;
    struct srchPosting          *pspSrchPostingsEnd1 = NULL;
    struct srchPosting          *pspSrchPostingsEnd2 = NULL;
    struct srchPosting          *pspSrchPostingsPtr = NULL;
    struct srchPosting          *pspSrchPostingsEnd = NULL;
    unsigned int                uiCurrentDocumentID = 0;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iSrchPostingMergeSrchPostingsListsOR"); */
/*     iUtlLogDebug(UTL_LOG_CONTEXT, "psplSrchPostingsList1"); */
/*     iSrchPostingPrintSrchPostingsList(psplSrchPostingsList1); */
/*     iUtlLogDebug(UTL_LOG_CONTEXT, "psplSrchPostingsList2"); */
/*     iSrchPostingPrintSrchPostingsList(psplSrchPostingsList2); */


    /* Check the parameters */
    if ( (psplSrchPostingsList1 != NULL) && (iSrchPostingCheckSrchPostingsList(psplSrchPostingsList1) != SRCH_NoError) ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'psplSrchPostingsList1' parameter passed to 'iSrchPostingMergeSrchPostingsListsOR'."); 
        return (SRCH_PostingInvalidPostingsList);
    }

    if ( (psplSrchPostingsList2 != NULL) && (iSrchPostingCheckSrchPostingsList(psplSrchPostingsList2) != SRCH_NoError) ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'psplSrchPostingsList2' parameter passed to 'iSrchPostingMergeSrchPostingsListsOR'."); 
        return (SRCH_PostingInvalidPostingsList);
    }

    if ( !((uiSrchPostingBooleanOperationID == SRCH_POSTING_BOOLEAN_OPERATION_RELAXED_ID) || (uiSrchPostingBooleanOperationID == SRCH_POSTING_BOOLEAN_OPERATION_STRICT_ID)) ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiSrchPostingBooleanOperationID' parameter passed to 'iSrchPostingMergeSrchPostingsListsOR'."); 
        return (SRCH_PostingInvalidSearchBooleanModifier);
    }

    if ( ppsplSrchPostingsList == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppsplSrchPostingsList' parameter passed to 'iSrchPostingMergeSrchPostingsListsOR'."); 
        return (SRCH_ReturnParameterError);
    }


    /* If either of the postings lists are NULL or of zero length we try to return the
    ** other one, if they are both null then we just return an empty search postings list structure
    */
    if ( (psplSrchPostingsList1 == NULL) || (psplSrchPostingsList1->uiSrchPostingsLength == 0) || 
            (psplSrchPostingsList2 == NULL) || (psplSrchPostingsList2->uiSrchPostingsLength == 0) ) {

        /* Return the non-empty set if the other one is empty or contains a non-required term */ 
        if ( (psplSrchPostingsList1 != NULL) && (psplSrchPostingsList1->uiSrchPostingsLength > 0) && 
                ((psplSrchPostingsList2 == NULL) || ((psplSrchPostingsList2 != NULL) && (psplSrchPostingsList2->bRequired == false))) ) {

            iSrchPostingFreeSrchPostingsList(psplSrchPostingsList2);
            psplSrchPostingsList2 = NULL;

            *ppsplSrchPostingsList = psplSrchPostingsList1;
            return (SRCH_NoError);
        }
        else if ( (psplSrchPostingsList2 != NULL) && (psplSrchPostingsList2->uiSrchPostingsLength > 0) && 
                ((psplSrchPostingsList1 == NULL) || ((psplSrchPostingsList1 != NULL) && (psplSrchPostingsList1->bRequired == false))) ) {

            iSrchPostingFreeSrchPostingsList(psplSrchPostingsList1);
            psplSrchPostingsList1 = NULL;

            *ppsplSrchPostingsList = psplSrchPostingsList2;
            return (SRCH_NoError);
        }

    
        /* Return an empty postings list */
        {
            int     uiTermType = SPI_TERM_TYPE_UNKNOWN;
    
            /* Set the term type to stop word is both passed term types were stop words */
            if ( (psplSrchPostingsList1 != NULL) && (psplSrchPostingsList1->uiTermType == SPI_TERM_TYPE_STOP) && 
                    (psplSrchPostingsList2 != NULL) && (psplSrchPostingsList2->uiTermType == SPI_TERM_TYPE_STOP) ) {
                uiTermType = SPI_TERM_TYPE_STOP;
            }
    
            /* Create an empty postings list */
            if ( (iError = iSrchPostingCreateSrchPostingsList(uiTermType, SPI_TERM_COUNT_UNKNOWN, SPI_TERM_DOCUMENT_COUNT_UNKNOWN, 
                    false, NULL, 0, &psplSrchPostingsList)) != SRCH_NoError ) {
                return (iError);
            }
    
            iSrchPostingFreeSrchPostingsList(psplSrchPostingsList1);
            psplSrchPostingsList1 = NULL;
    
            iSrchPostingFreeSrchPostingsList(psplSrchPostingsList2);
            psplSrchPostingsList2 = NULL;
    
            *ppsplSrchPostingsList = psplSrchPostingsList;
            return (SRCH_NoError);
        }
    }


    
    /* Various permutations of handling required terms, we send all those to AND or IOR */
    if ( (psplSrchPostingsList1->bRequired == true) && (psplSrchPostingsList2->bRequired == true) ) {
/*         iUtlLogDebug(UTL_LOG_CONTEXT, "remapped to iSrchPostingMergeSrchPostingsListsAND"); */
        return(iSrchPostingMergeSrchPostingsListsAND(psplSrchPostingsList1, psplSrchPostingsList2, uiSrchPostingBooleanOperationID, ppsplSrchPostingsList));
    }
    else if ( (psplSrchPostingsList1->bRequired == true) && (psplSrchPostingsList2->bRequired == false) ) {
/*         iUtlLogDebug(UTL_LOG_CONTEXT, "remapped to iSrchPostingMergeSrchPostingsListsIOR (i)"); */
        return(iSrchPostingMergeSrchPostingsListsIOR(psplSrchPostingsList1, psplSrchPostingsList2, uiSrchPostingBooleanOperationID, ppsplSrchPostingsList));
    }
    else if ( (psplSrchPostingsList1->bRequired == false) && (psplSrchPostingsList2->bRequired == true) ) {
/*         iUtlLogDebug(UTL_LOG_CONTEXT, "remapped to iSrchPostingMergeSrchPostingsListsIOR (ii)"); */
        return(iSrchPostingMergeSrchPostingsListsIOR(psplSrchPostingsList2, psplSrchPostingsList1, uiSrchPostingBooleanOperationID, ppsplSrchPostingsList));
    }


    ASSERT(psplSrchPostingsList1->uiSrchPostingsLength > 0);
    ASSERT(psplSrchPostingsList2->uiSrchPostingsLength > 0);

    /* Set up our variables */
    pspSrchPostingsPtr1 = psplSrchPostingsList1->pspSrchPostings;
    pspSrchPostingsPtr2 = psplSrchPostingsList2->pspSrchPostings;
    pspSrchPostingsEnd1 = psplSrchPostingsList1->pspSrchPostings + psplSrchPostingsList1->uiSrchPostingsLength;
    pspSrchPostingsEnd2 = psplSrchPostingsList2->pspSrchPostings + psplSrchPostingsList2->uiSrchPostingsLength;


    /* Create a new search postings list structure */
    if ( (iError = iSrchPostingCreateSrchPostingsList(SPI_TERM_TYPE_REGULAR, 0, 0, 
            (((psplSrchPostingsList1->bRequired == true) || (psplSrchPostingsList2->bRequired == true)) ? true : false), 
            NULL, (psplSrchPostingsList1->uiSrchPostingsLength + psplSrchPostingsList2->uiSrchPostingsLength), &psplSrchPostingsList)) != SRCH_NoError ) {
        return (iError);
    }


    /* Loop through the outer postings array */
    for ( pspSrchPostingsPtr = psplSrchPostingsList->pspSrchPostings; pspSrchPostingsPtr1 < pspSrchPostingsEnd1; pspSrchPostingsPtr1++ ) {

/*         iUtlLogDebug(UTL_LOG_CONTEXT, "uiDocumentID1 [%u], uiTermPosition [%d] fWeight [%.4f] (tracking/1)",  */
/*                 pspSrchPostingsPtr1->uiDocumentID, pspSrchPostingsPtr1->uiTermPosition, pspSrchPostingsPtr1->fWeight); */

        /* Loop on the inner postings array */
        for ( ; (pspSrchPostingsPtr2 < pspSrchPostingsEnd2) && (pspSrchPostingsPtr1->uiDocumentID >= pspSrchPostingsPtr2->uiDocumentID); pspSrchPostingsPtr2++ ) {

/*             iUtlLogDebug(UTL_LOG_CONTEXT, "uiDocumentID2 [%u], uiTermPosition [%d] fWeight [%.4f] (tracking/2)",  */
/*                     pspSrchPostingsPtr2->uiDocumentID, pspSrchPostingsPtr2->uiTermPosition, pspSrchPostingsPtr2->fWeight); */

            /* Same document */
            if ( pspSrchPostingsPtr1->uiDocumentID == pspSrchPostingsPtr2->uiDocumentID ) {

                /* The outer term is after the inner term, so we add the inner posting */
                if ( pspSrchPostingsPtr1->uiTermPosition >= pspSrchPostingsPtr2->uiTermPosition ) {

                    SRCH_POSTING_COPY_SRCH_POSTING(pspSrchPostingsPtr, pspSrchPostingsPtr2);

#if defined(SRCH_POSTING_ENABLE_PROXIMITY_REWEIGHTING)
                    if ( (pspSrchPostingsPtr > psplSrchPostingsList->pspSrchPostings) && 
                            (pspSrchPostingsPtr->uiDocumentID == pspSrchPostingsPtr2->uiDocumentID) && 
                            (pspSrchPostingsPtr2->uiTermPosition == ((pspSrchPostingsPtr - 1)->uiTermPosition + 1)) ) {
                        pspSrchPostingsPtr->fWeight *= SRCH_POSTING_PROXIMITY_REWEIGHTING;
                    }
#endif    /* defined(SRCH_POSTING_ENABLE_PROXIMITY_REWEIGHTING) */

                    pspSrchPostingsPtr++;

/*                     iUtlLogDebug(UTL_LOG_CONTEXT, "uiDocumentID2 [%u], uiTermPosition [%d] fWeight [%.4f] (added/1)",  */
/*                             (pspSrchPostingsPtr - 1)->uiDocumentID, (pspSrchPostingsPtr - 1)->uiTermPosition, (pspSrchPostingsPtr - 1)->fWeight); */
                }
                /* Otherwise we break */
                else {
                    break;
                }
            }
            /* Add the inner posting */
            else {

                SRCH_POSTING_COPY_SRCH_POSTING(pspSrchPostingsPtr, pspSrchPostingsPtr2);

#if defined(SRCH_POSTING_ENABLE_PROXIMITY_REWEIGHTING)
                if ( (pspSrchPostingsPtr > psplSrchPostingsList->pspSrchPostings) && 
                        (pspSrchPostingsPtr->uiDocumentID == pspSrchPostingsPtr2->uiDocumentID) && 
                        (pspSrchPostingsPtr2->uiTermPosition == ((pspSrchPostingsPtr - 1)->uiTermPosition + 1)) ) {
                    pspSrchPostingsPtr->fWeight *= SRCH_POSTING_PROXIMITY_REWEIGHTING;
                }
#endif    /* defined(SRCH_POSTING_ENABLE_PROXIMITY_REWEIGHTING) */

                pspSrchPostingsPtr++;

/*                 iUtlLogDebug(UTL_LOG_CONTEXT, "uiDocumentID2 [%u], uiTermPosition [%d] fWeight [%.4f] (added/2)",  */
/*                         (pspSrchPostingsPtr - 1)->uiDocumentID, (pspSrchPostingsPtr - 1)->uiTermPosition, (pspSrchPostingsPtr - 1)->fWeight); */
            }
        }

        /* Add this outer posting to the return posting array */
        SRCH_POSTING_COPY_SRCH_POSTING(pspSrchPostingsPtr, pspSrchPostingsPtr1);

#if defined(SRCH_POSTING_ENABLE_PROXIMITY_REWEIGHTING)
        if ( (pspSrchPostingsPtr > psplSrchPostingsList->pspSrchPostings) && 
                (pspSrchPostingsPtr->uiDocumentID == pspSrchPostingsPtr1->uiDocumentID) && 
                (pspSrchPostingsPtr1->uiTermPosition == ((pspSrchPostingsPtr - 1)->uiTermPosition + 1)) ) {
            pspSrchPostingsPtr->fWeight *= SRCH_POSTING_PROXIMITY_REWEIGHTING;
        }
#endif    /* defined(SRCH_POSTING_ENABLE_PROXIMITY_REWEIGHTING) */

        pspSrchPostingsPtr++;

/*         iUtlLogDebug(UTL_LOG_CONTEXT, "uiDocumentID1 [%u], uiTermPosition [%d] fWeight [%.4f] (added/1)",  */
/*                 (pspSrchPostingsPtr - 1)->uiDocumentID, (pspSrchPostingsPtr - 1)->uiTermPosition, (pspSrchPostingsPtr - 1)->fWeight); */
    }


    /* Merge the rest of the inner postings array */
    for ( ; pspSrchPostingsPtr2 < pspSrchPostingsEnd2; pspSrchPostingsPtr2++ ) {

        SRCH_POSTING_COPY_SRCH_POSTING(pspSrchPostingsPtr, pspSrchPostingsPtr2);

#if defined(SRCH_POSTING_ENABLE_PROXIMITY_REWEIGHTING)
        if ( (pspSrchPostingsPtr > psplSrchPostingsList->pspSrchPostings) && 
                (pspSrchPostingsPtr->uiDocumentID == pspSrchPostingsPtr2->uiDocumentID) && 
                (pspSrchPostingsPtr2->uiTermPosition == ((pspSrchPostingsPtr - 1)->uiTermPosition + 1)) ) {
            pspSrchPostingsPtr->fWeight *= SRCH_POSTING_PROXIMITY_REWEIGHTING;
        }
#endif    /* defined(SRCH_POSTING_ENABLE_PROXIMITY_REWEIGHTING) */

        pspSrchPostingsPtr++;

/*         iUtlLogDebug(UTL_LOG_CONTEXT, "uiDocumentID2 [%u], uiTermPosition [%d] fWeight [%.4f] (added/3)",  */
/*                 (pspSrchPostingsPtr - 1)->uiDocumentID, (pspSrchPostingsPtr - 1)->uiTermPosition, (pspSrchPostingsPtr - 1)->fWeight); */
    }


    /* Get the size of the new postings array */
    uiSrchPostingsLength = pspSrchPostingsPtr - psplSrchPostingsList->pspSrchPostings;
    
    /* Set the term count and postings length */
    psplSrchPostingsList->uiTermCount = uiSrchPostingsLength;
    psplSrchPostingsList->uiSrchPostingsLength = uiSrchPostingsLength;

    /* Adjust the size of the new postings array */
    if ( uiSrchPostingsLength < psplSrchPostingsList->uiSrchPostingsLength ) {

        if ( uiSrchPostingsLength > 0 ) {

            if ( (pspSrchPostingsPtr = (struct srchPosting *)s_realloc(psplSrchPostingsList->pspSrchPostings, (size_t)(uiSrchPostingsLength * sizeof(struct srchPosting)))) == NULL ) {
                iSrchPostingFreeSrchPostingsList(psplSrchPostingsList);
                psplSrchPostingsList = NULL;
                return (SRCH_MemError);
            }

            psplSrchPostingsList->pspSrchPostings = pspSrchPostingsPtr;
        }
        else {
            s_free(psplSrchPostingsList->pspSrchPostings);
        }
    }


    /* Count up the number of documents in this posting */
    for ( pspSrchPostingsPtr = psplSrchPostingsList->pspSrchPostings, pspSrchPostingsEnd = psplSrchPostingsList->pspSrchPostings + psplSrchPostingsList->uiSrchPostingsLength, 
            psplSrchPostingsList->uiDocumentCount = 0, uiCurrentDocumentID = 0; pspSrchPostingsPtr < pspSrchPostingsEnd; pspSrchPostingsPtr++ ) {
        
        if ( pspSrchPostingsPtr->uiDocumentID != uiCurrentDocumentID ) {
            psplSrchPostingsList->uiDocumentCount++;
            uiCurrentDocumentID = pspSrchPostingsPtr->uiDocumentID;
        }
    }


    /* Free the two old postings lists */
    iSrchPostingFreeSrchPostingsList(psplSrchPostingsList1);
    psplSrchPostingsList1 = NULL;

    iSrchPostingFreeSrchPostingsList(psplSrchPostingsList2);
    psplSrchPostingsList2 = NULL;


    ASSERT(iSrchPostingCheckSrchPostingsList(psplSrchPostingsList) == SRCH_NoError);

/*     iUtlLogDebug(UTL_LOG_CONTEXT, "psplSrchPostingsList"); */
/*     iSrchPostingPrintSrchPostingsList(psplSrchPostingsList); */


    /* Set the return pointer */
    *ppsplSrchPostingsList = psplSrchPostingsList;

    /* Return  */
    return (SRCH_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchPostingMergeSrchPostingsListsIOR()

    Purpose:    This function IORs psplSrchPostingsList2 into psplSrchPostingsList1 
                and returns a pointer to a new search postings list structure.

    Parameters: psplSrchPostingsList1               search postings list structure
                psplSrchPostingsList2               search postings list structure
                uiSrchPostingBooleanOperationID     posting boolean operation ID
                ppsplSrchPostingsList               return pointer for the search postings list structure

    Globals:    none

    Returns:    SRCH error code

*/
int iSrchPostingMergeSrchPostingsListsIOR
(
    struct srchPostingsList *psplSrchPostingsList1,
    struct srchPostingsList *psplSrchPostingsList2,
    unsigned int uiSrchPostingBooleanOperationID,
    struct srchPostingsList **ppsplSrchPostingsList
)
{

    int                         iError = SRCH_NoError;
    unsigned int                uiSrchPostingsLength = 0;
    struct srchPostingsList     *psplSrchPostingsList = NULL;
    struct srchPosting          *pspSrchPostingsPtr1 = NULL;
    struct srchPosting          *pspSrchPostingsPtr2 = NULL;
    struct srchPosting          *pspSrchPostingsEnd1 = NULL;
    struct srchPosting          *pspSrchPostingsEnd2 = NULL;
    struct srchPosting          *pspSrchPostingsPtr = NULL;
    struct srchPosting          *pspSrchPostingsEnd = NULL;
    unsigned int                uiCurrentDocumentID = 0;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iSrchPostingMergeSrchPostingsListsIOR"); */
/*     iUtlLogDebug(UTL_LOG_CONTEXT, "psplSrchPostingsList1"); */
/*     iSrchPostingPrintSrchPostingsList(psplSrchPostingsList1); */
/*     iUtlLogDebug(UTL_LOG_CONTEXT, "psplSrchPostingsList2"); */
/*     iSrchPostingPrintSrchPostingsList(psplSrchPostingsList2); */


    /* Check the parameters */
    if ( (psplSrchPostingsList1 != NULL) && (iSrchPostingCheckSrchPostingsList(psplSrchPostingsList1) != SRCH_NoError) ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'psplSrchPostingsList1' parameter passed to 'iSrchPostingMergeSrchPostingsListsIOR'."); 
        return (SRCH_PostingInvalidPostingsList);
    }

    if ( (psplSrchPostingsList2 != NULL) && (iSrchPostingCheckSrchPostingsList(psplSrchPostingsList2) != SRCH_NoError) ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'psplSrchPostingsList2' parameter passed to 'iSrchPostingMergeSrchPostingsListsIOR'."); 
        return (SRCH_PostingInvalidPostingsList);
    }

    if ( !((uiSrchPostingBooleanOperationID == SRCH_POSTING_BOOLEAN_OPERATION_RELAXED_ID) || (uiSrchPostingBooleanOperationID == SRCH_POSTING_BOOLEAN_OPERATION_STRICT_ID)) ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiSrchPostingBooleanOperationID' parameter passed to 'iSrchPostingMergeSrchPostingsListsIOR'."); 
        return (SRCH_PostingInvalidSearchBooleanModifier);
    }

    if ( ppsplSrchPostingsList == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppsplSrchPostingsList' parameter passed to 'iSrchPostingMergeSrchPostingsListsIOR'."); 
        return (SRCH_ReturnParameterError);
    }


    /* If either of the postings lists are NULL or of zero length we try to return the
    ** other one, if they are both null then we just return an empty search postings list structure
    */
    if ( (psplSrchPostingsList1 == NULL) || (psplSrchPostingsList1->uiSrchPostingsLength == 0) || 
            (psplSrchPostingsList2 == NULL) || (psplSrchPostingsList2->uiSrchPostingsLength == 0) ) {

        /* Return the non-empty set if we are using strict booleans and the non-empty one contains a stop term, 
        ** or if we are using relaxed booleans and the non-empty one contains a non-required term
        */ 
        if ( (psplSrchPostingsList1 != NULL) && (psplSrchPostingsList2 != NULL) ) {

            if ( (psplSrchPostingsList1->uiSrchPostingsLength > 0) &&
                    ( ((uiSrchPostingBooleanOperationID == SRCH_POSTING_BOOLEAN_OPERATION_STRICT_ID) && (psplSrchPostingsList2->uiTermType == SPI_TERM_TYPE_STOP)) ||
                    ((uiSrchPostingBooleanOperationID == SRCH_POSTING_BOOLEAN_OPERATION_RELAXED_ID) && (psplSrchPostingsList2->bRequired == false)) ) ) {
                
                iSrchPostingFreeSrchPostingsList(psplSrchPostingsList2);
                psplSrchPostingsList2 = NULL;

                *ppsplSrchPostingsList = psplSrchPostingsList1;
                return (SRCH_NoError);
            }
            else if ( (psplSrchPostingsList2->uiSrchPostingsLength > 0) &&
                    ( ((uiSrchPostingBooleanOperationID == SRCH_POSTING_BOOLEAN_OPERATION_STRICT_ID) && (psplSrchPostingsList1->uiTermType == SPI_TERM_TYPE_STOP)) ||
                    ((uiSrchPostingBooleanOperationID == SRCH_POSTING_BOOLEAN_OPERATION_RELAXED_ID) && (psplSrchPostingsList1->bRequired == false)) ) ) {
    
                iSrchPostingFreeSrchPostingsList(psplSrchPostingsList1);
                psplSrchPostingsList1 = NULL;

                *ppsplSrchPostingsList = psplSrchPostingsList2;
                return (SRCH_NoError);
            }
        }
        /* Return the non-NULL set if we are using relaxed booleans */
        else if ( uiSrchPostingBooleanOperationID == SRCH_POSTING_BOOLEAN_OPERATION_RELAXED_ID ) {
            
            if ( (psplSrchPostingsList1 == NULL) && (psplSrchPostingsList2 != NULL)  ) {
                *ppsplSrchPostingsList = psplSrchPostingsList2;
                return (SRCH_NoError);
            }
            else if ( (psplSrchPostingsList2 == NULL) && (psplSrchPostingsList1 != NULL) ) {
                *ppsplSrchPostingsList = psplSrchPostingsList1;
                return (SRCH_NoError);
            }
        }
    

        /* Return an empty postings list */
        {
            int     uiTermType = SPI_TERM_TYPE_UNKNOWN;
    
            /* Set the term type to stop word is both passed term types were stop words */
            if ( (psplSrchPostingsList1 != NULL) && (psplSrchPostingsList1->uiTermType == SPI_TERM_TYPE_STOP) && 
                    (psplSrchPostingsList2 != NULL) && (psplSrchPostingsList2->uiTermType == SPI_TERM_TYPE_STOP) ) {
                uiTermType = SPI_TERM_TYPE_STOP;
            }
    
            /* Create an empty postings list */
            if ( (iError = iSrchPostingCreateSrchPostingsList(uiTermType, SPI_TERM_COUNT_UNKNOWN, SPI_TERM_DOCUMENT_COUNT_UNKNOWN, 
                    false, NULL, 0, &psplSrchPostingsList)) != SRCH_NoError ) {
                return (iError);
            }
    
            iSrchPostingFreeSrchPostingsList(psplSrchPostingsList1);
            psplSrchPostingsList1 = NULL;
    
            iSrchPostingFreeSrchPostingsList(psplSrchPostingsList2);
            psplSrchPostingsList2 = NULL;
    
            *ppsplSrchPostingsList = psplSrchPostingsList;
            return (SRCH_NoError);
        }
    }


    ASSERT(psplSrchPostingsList1->uiSrchPostingsLength > 0);
    ASSERT(psplSrchPostingsList2->uiSrchPostingsLength > 0);


    /* Set up our variables */
    pspSrchPostingsPtr1 = psplSrchPostingsList1->pspSrchPostings;
    pspSrchPostingsPtr2 = psplSrchPostingsList2->pspSrchPostings;
    pspSrchPostingsEnd1 = psplSrchPostingsList1->pspSrchPostings + psplSrchPostingsList1->uiSrchPostingsLength;
    pspSrchPostingsEnd2 = psplSrchPostingsList2->pspSrchPostings + psplSrchPostingsList2->uiSrchPostingsLength;


    /* Create a new search postings list structure */
    if ( (iError = iSrchPostingCreateSrchPostingsList(SPI_TERM_TYPE_REGULAR, 0, 0, 
            (((psplSrchPostingsList1->bRequired == true) || (psplSrchPostingsList2->bRequired == true)) ? true : false), 
            NULL, (psplSrchPostingsList1->uiSrchPostingsLength + psplSrchPostingsList2->uiSrchPostingsLength), &psplSrchPostingsList)) != SRCH_NoError ) {
        return (iError);
    }


    /* Loop through the outer postings array */
    for ( pspSrchPostingsPtr = psplSrchPostingsList->pspSrchPostings; pspSrchPostingsPtr1 < pspSrchPostingsEnd1; pspSrchPostingsPtr1++ ) {

/*         iUtlLogDebug(UTL_LOG_CONTEXT, "uiDocumentID1 [%u], uiTermPosition [%d] fWeight [%.4f] (tracking/1)",  */
/*                 pspSrchPostingsPtr1->uiDocumentID, pspSrchPostingsPtr1->uiTermPosition, pspSrchPostingsPtr1->fWeight); */

        /* Loop on the inner postings array */
        for ( ; (pspSrchPostingsPtr2 < pspSrchPostingsEnd2) && (pspSrchPostingsPtr1->uiDocumentID >= pspSrchPostingsPtr2->uiDocumentID); pspSrchPostingsPtr2++ ) {

/*             iUtlLogDebug(UTL_LOG_CONTEXT, "uiDocumentID2 [%u], uiTermPosition [%d] fWeight [%.4f] (tracking/2)",  */
/*                     pspSrchPostingsPtr2->uiDocumentID, pspSrchPostingsPtr2->uiTermPosition, pspSrchPostingsPtr2->fWeight); */

            /* Same document */
            if ( pspSrchPostingsPtr1->uiDocumentID == pspSrchPostingsPtr2->uiDocumentID ) {

                /* The outer term is after the inner term, so we add the inner posting */
                if ( pspSrchPostingsPtr1->uiTermPosition >= pspSrchPostingsPtr2->uiTermPosition ) {

                    SRCH_POSTING_COPY_SRCH_POSTING(pspSrchPostingsPtr, pspSrchPostingsPtr2);

#if defined(SRCH_POSTING_ENABLE_PROXIMITY_REWEIGHTING)
                    if ( (pspSrchPostingsPtr > psplSrchPostingsList->pspSrchPostings) && 
                            (pspSrchPostingsPtr->uiDocumentID == pspSrchPostingsPtr2->uiDocumentID) && 
                            (pspSrchPostingsPtr2->uiTermPosition == ((pspSrchPostingsPtr - 1)->uiTermPosition + 1)) ) {
                        pspSrchPostingsPtr->fWeight *= SRCH_POSTING_PROXIMITY_REWEIGHTING;
                    }
#endif    /* defined(SRCH_POSTING_ENABLE_PROXIMITY_REWEIGHTING) */

                    pspSrchPostingsPtr++;

/*                     iUtlLogDebug(UTL_LOG_CONTEXT, "uiDocumentID2 [%u], uiTermPosition [%d] fWeight [%.4f] (added/1)",  */
/*                             (pspSrchPostingsPtr - 1)->uiDocumentID, (pspSrchPostingsPtr - 1)->uiTermPosition, (pspSrchPostingsPtr - 1)->fWeight); */
                }
                /* Otherwise we break */
                else {
                    break;
                }
            }
            /* Add the inner posting */
            else if ( (pspSrchPostingsPtr > psplSrchPostingsList->pspSrchPostings) && (pspSrchPostingsPtr - 1)->uiDocumentID == pspSrchPostingsPtr2->uiDocumentID ) {

                SRCH_POSTING_COPY_SRCH_POSTING(pspSrchPostingsPtr, pspSrchPostingsPtr2);

#if defined(SRCH_POSTING_ENABLE_PROXIMITY_REWEIGHTING)
                    if ( (pspSrchPostingsPtr > psplSrchPostingsList->pspSrchPostings) && 
                            (pspSrchPostingsPtr->uiDocumentID == pspSrchPostingsPtr2->uiDocumentID) && 
                            (pspSrchPostingsPtr2->uiTermPosition == ((pspSrchPostingsPtr - 1)->uiTermPosition + 1)) ) {
                        pspSrchPostingsPtr->fWeight *= SRCH_POSTING_PROXIMITY_REWEIGHTING;
                    }
#endif    /* defined(SRCH_POSTING_ENABLE_PROXIMITY_REWEIGHTING) */

                pspSrchPostingsPtr++;

/*                 iUtlLogDebug(UTL_LOG_CONTEXT, "uiDocumentID2 [%u], uiTermPosition [%d] fWeight [%.4f] (added/2)",  */
/*                         (pspSrchPostingsPtr - 1)->uiDocumentID, (pspSrchPostingsPtr - 1)->uiTermPosition, (pspSrchPostingsPtr - 1)->fWeight); */
            } 
        }

        /* Add this outer posting to the return posting array */
        SRCH_POSTING_COPY_SRCH_POSTING(pspSrchPostingsPtr, pspSrchPostingsPtr1);

#if defined(SRCH_POSTING_ENABLE_PROXIMITY_REWEIGHTING)
        if ( (pspSrchPostingsPtr > psplSrchPostingsList->pspSrchPostings) && 
                (pspSrchPostingsPtr->uiDocumentID == pspSrchPostingsPtr1->uiDocumentID) && 
                (pspSrchPostingsPtr1->uiTermPosition == ((pspSrchPostingsPtr - 1)->uiTermPosition + 1)) ) {
            pspSrchPostingsPtr->fWeight *= SRCH_POSTING_PROXIMITY_REWEIGHTING;
        }
#endif    /* defined(SRCH_POSTING_ENABLE_PROXIMITY_REWEIGHTING) */

        pspSrchPostingsPtr++;

/*         iUtlLogDebug(UTL_LOG_CONTEXT, "uiDocumentID1 [%u], uiTermPosition [%d] fWeight [%.4f] (added/3)",  */
/*                 (pspSrchPostingsPtr - 1)->uiDocumentID, (pspSrchPostingsPtr - 1)->uiTermPosition, (pspSrchPostingsPtr - 1)->fWeight); */
    }


    /* Merge the rest of the inner postings array */
    for ( ; (pspSrchPostingsPtr2 < pspSrchPostingsEnd2) && (pspSrchPostingsPtr > psplSrchPostingsList->pspSrchPostings) && 
            ((pspSrchPostingsPtr - 1)->uiDocumentID == pspSrchPostingsPtr2->uiDocumentID); pspSrchPostingsPtr2++ ) {

        SRCH_POSTING_COPY_SRCH_POSTING(pspSrchPostingsPtr, pspSrchPostingsPtr2);

#if defined(SRCH_POSTING_ENABLE_PROXIMITY_REWEIGHTING)
        if ( (pspSrchPostingsPtr > psplSrchPostingsList->pspSrchPostings) && 
                (pspSrchPostingsPtr->uiDocumentID == pspSrchPostingsPtr2->uiDocumentID) && 
                (pspSrchPostingsPtr2->uiTermPosition == ((pspSrchPostingsPtr - 1)->uiTermPosition + 1)) ) {
            pspSrchPostingsPtr->fWeight *= SRCH_POSTING_PROXIMITY_REWEIGHTING;
        }
#endif    /* defined(SRCH_POSTING_ENABLE_PROXIMITY_REWEIGHTING) */

        pspSrchPostingsPtr++;

/*         iUtlLogDebug(UTL_LOG_CONTEXT, "uiDocumentID2 [%u], uiTermPosition [%d] fWeight [%.4f] (added/4)",  */
/*                 (pspSrchPostingsPtr - 1)->uiDocumentID, (pspSrchPostingsPtr - 1)->uiTermPosition, (pspSrchPostingsPtr - 1)->fWeight); */
    }


    /* Get the size of the new postings array */
    uiSrchPostingsLength = pspSrchPostingsPtr - psplSrchPostingsList->pspSrchPostings;
    
    /* Set the term count and postings length */
    psplSrchPostingsList->uiTermCount = uiSrchPostingsLength;
    psplSrchPostingsList->uiSrchPostingsLength = uiSrchPostingsLength;

    /* Adjust the size of the new postings array */
    if ( uiSrchPostingsLength < psplSrchPostingsList->uiSrchPostingsLength ) {

        if ( uiSrchPostingsLength > 0 ) {

            if ( (pspSrchPostingsPtr = (struct srchPosting *)s_realloc(psplSrchPostingsList->pspSrchPostings, (size_t)(uiSrchPostingsLength * sizeof(struct srchPosting)))) == NULL ) {
                iSrchPostingFreeSrchPostingsList(psplSrchPostingsList);
                psplSrchPostingsList = NULL;
                return (SRCH_MemError);
            }

            psplSrchPostingsList->pspSrchPostings = pspSrchPostingsPtr;
        }
        else {
            s_free(psplSrchPostingsList->pspSrchPostings);
        }
    }


    /* Count up the number of documents in this posting */
    for ( pspSrchPostingsPtr = psplSrchPostingsList->pspSrchPostings, pspSrchPostingsEnd = psplSrchPostingsList->pspSrchPostings + psplSrchPostingsList->uiSrchPostingsLength, 
            psplSrchPostingsList->uiDocumentCount = 0, uiCurrentDocumentID = 0; pspSrchPostingsPtr < pspSrchPostingsEnd; pspSrchPostingsPtr++ ) {

        if ( pspSrchPostingsPtr->uiDocumentID != uiCurrentDocumentID ) {
            psplSrchPostingsList->uiDocumentCount++;
            uiCurrentDocumentID = pspSrchPostingsPtr->uiDocumentID;
        }
    }


    /* Free the two old postings lists */
    iSrchPostingFreeSrchPostingsList(psplSrchPostingsList1);
    psplSrchPostingsList1 = NULL;

    iSrchPostingFreeSrchPostingsList(psplSrchPostingsList2);
    psplSrchPostingsList2 = NULL;


    ASSERT(iSrchPostingCheckSrchPostingsList(psplSrchPostingsList) == SRCH_NoError);

/*     iUtlLogDebug(UTL_LOG_CONTEXT, "psplSrchPostingsList"); */
/*     iSrchPostingPrintSrchPostingsList(psplSrchPostingsList); */


    /* Set the return pointer */
    *ppsplSrchPostingsList = psplSrchPostingsList;

    /* Return  */
    return (SRCH_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchPostingMergeSrchPostingsListsXOR()

    Purpose:    This function XORs psplSrchPostingsList2 into psplSrchPostingsList1 
                and returns a pointer to a new search postings list structure.

    Parameters: psplSrchPostingsList1               search postings list structure
                psplSrchPostingsList2               search postings list structure
                uiSrchPostingBooleanOperationID     posting boolean operation ID
                ppsplSrchPostingsList               return pointer for the search postings list structure

    Globals:    none

    Returns:    SRCH error code

*/
int iSrchPostingMergeSrchPostingsListsXOR
(
    struct srchPostingsList *psplSrchPostingsList1,
    struct srchPostingsList *psplSrchPostingsList2,
    unsigned int uiSrchPostingBooleanOperationID,
    struct srchPostingsList **ppsplSrchPostingsList
)
{

    int                         iError = SRCH_NoError;
    unsigned int                uiSrchPostingsLength = 0;
    struct srchPostingsList     *psplSrchPostingsList = NULL;
    struct srchPosting          *pspSrchPostingsPtr1 = NULL;
    struct srchPosting          *pspSrchPostingsPtr2 = NULL;
    struct srchPosting          *pspSrchPostingsEnd1 = NULL;
    struct srchPosting          *pspSrchPostingsEnd2 = NULL;
    struct srchPosting          *pspSrchPostingsPtr = NULL;
    struct srchPosting          *pspSrchPostingsEnd = NULL;
    unsigned int                uiCurrentDocumentID = 0;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iSrchPostingMergeSrchPostingsListsXOR"); */
/*     iUtlLogDebug(UTL_LOG_CONTEXT, "psplSrchPostingsList1"); */
/*     iSrchPostingPrintSrchPostingsList(psplSrchPostingsList1); */
/*     iUtlLogDebug(UTL_LOG_CONTEXT, "psplSrchPostingsList2"); */
/*     iSrchPostingPrintSrchPostingsList(psplSrchPostingsList2); */


    /* Check the parameters */
    if ( (psplSrchPostingsList1 != NULL) && (iSrchPostingCheckSrchPostingsList(psplSrchPostingsList1) != SRCH_NoError) ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'psplSrchPostingsList1' parameter passed to 'iSrchPostingMergeSrchPostingsListsXOR'."); 
        return (SRCH_PostingInvalidPostingsList);
    }

    if ( (psplSrchPostingsList2 != NULL) && (iSrchPostingCheckSrchPostingsList(psplSrchPostingsList2) != SRCH_NoError) ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'psplSrchPostingsList2' parameter passed to 'iSrchPostingMergeSrchPostingsListsXOR'."); 
        return (SRCH_PostingInvalidPostingsList);
    }

    if ( !((uiSrchPostingBooleanOperationID == SRCH_POSTING_BOOLEAN_OPERATION_RELAXED_ID) || (uiSrchPostingBooleanOperationID == SRCH_POSTING_BOOLEAN_OPERATION_STRICT_ID)) ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiSrchPostingBooleanOperationID' parameter passed to 'iSrchPostingMergeSrchPostingsListsXOR'."); 
        return (SRCH_PostingInvalidSearchBooleanModifier);
    }

    if ( ppsplSrchPostingsList == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppsplSrchPostingsList' parameter passed to 'iSrchPostingMergeSrchPostingsListsXOR'."); 
        return (SRCH_ReturnParameterError);
    }


    /* If either of the postings lists are NULL or of zero length we try to return the
    ** other one, if they are both null then we just return an empty search postings list structure
    */
    if ( (psplSrchPostingsList1 == NULL) || (psplSrchPostingsList1->uiSrchPostingsLength == 0) || 
            (psplSrchPostingsList2 == NULL) || (psplSrchPostingsList2->uiSrchPostingsLength == 0) ) {

        /* Return the non-empty set if we are using strict booleans and the non-empty one contains a stop term, 
        ** or if we are using relaxed booleans and the non-empty one contains a non-required term
        */ 
        if ( (psplSrchPostingsList1 != NULL) && (psplSrchPostingsList2 != NULL) ) {

            if ( (psplSrchPostingsList1->uiSrchPostingsLength > 0) &&
                    ( ((uiSrchPostingBooleanOperationID == SRCH_POSTING_BOOLEAN_OPERATION_STRICT_ID) && (psplSrchPostingsList2->uiTermType == SPI_TERM_TYPE_STOP)) ||
                    ((uiSrchPostingBooleanOperationID == SRCH_POSTING_BOOLEAN_OPERATION_RELAXED_ID) && (psplSrchPostingsList2->bRequired == false)) ) ) {
                
                iSrchPostingFreeSrchPostingsList(psplSrchPostingsList2);
                psplSrchPostingsList2 = NULL;

                *ppsplSrchPostingsList = psplSrchPostingsList1;
                return (SRCH_NoError);
            }
            else if ( (psplSrchPostingsList2->uiSrchPostingsLength > 0) &&
                    ( ((uiSrchPostingBooleanOperationID == SRCH_POSTING_BOOLEAN_OPERATION_STRICT_ID) && (psplSrchPostingsList1->uiTermType == SPI_TERM_TYPE_STOP)) ||
                    ((uiSrchPostingBooleanOperationID == SRCH_POSTING_BOOLEAN_OPERATION_RELAXED_ID) && (psplSrchPostingsList1->bRequired == false)) ) ) {
    
                iSrchPostingFreeSrchPostingsList(psplSrchPostingsList1);
                psplSrchPostingsList1 = NULL;
    
                *ppsplSrchPostingsList = psplSrchPostingsList2;
                return (SRCH_NoError);
            }
        }
        /* Return the non-NULL set if we are using relaxed booleans */
        else if ( uiSrchPostingBooleanOperationID == SRCH_POSTING_BOOLEAN_OPERATION_RELAXED_ID ) {
            
            if ( (psplSrchPostingsList1 == NULL) && (psplSrchPostingsList2 != NULL)  ) {
                *ppsplSrchPostingsList = psplSrchPostingsList2;
                return (SRCH_NoError);
            }
            else if ( (psplSrchPostingsList2 == NULL) && (psplSrchPostingsList1 != NULL) ) {
                *ppsplSrchPostingsList = psplSrchPostingsList1;
                return (SRCH_NoError);
            }
        }
    

        /* Return an empty postings list */
        {
            int     uiTermType = SPI_TERM_TYPE_UNKNOWN;
    
            /* Set the term type to stop word is both passed term types were stop words */
            if ( (psplSrchPostingsList1 != NULL) && (psplSrchPostingsList1->uiTermType == SPI_TERM_TYPE_STOP) && 
                    (psplSrchPostingsList2 != NULL) && (psplSrchPostingsList2->uiTermType == SPI_TERM_TYPE_STOP) ) {
                uiTermType = SPI_TERM_TYPE_STOP;
            }
    
            /* Create an empty postings list */
            if ( (iError = iSrchPostingCreateSrchPostingsList(uiTermType, SPI_TERM_COUNT_UNKNOWN, SPI_TERM_DOCUMENT_COUNT_UNKNOWN, 
                    false, NULL, 0, &psplSrchPostingsList)) != SRCH_NoError ) {
                return (iError);
            }
    
            iSrchPostingFreeSrchPostingsList(psplSrchPostingsList1);
            psplSrchPostingsList1 = NULL;
    
            iSrchPostingFreeSrchPostingsList(psplSrchPostingsList2);
            psplSrchPostingsList2 = NULL;
    
            *ppsplSrchPostingsList = psplSrchPostingsList;
            return (SRCH_NoError);
        }
    }


    ASSERT(psplSrchPostingsList1->uiSrchPostingsLength > 0);
    ASSERT(psplSrchPostingsList2->uiSrchPostingsLength > 0);


    /* Set up our variables */
    pspSrchPostingsPtr1 = psplSrchPostingsList1->pspSrchPostings;
    pspSrchPostingsPtr2 = psplSrchPostingsList2->pspSrchPostings;
    pspSrchPostingsEnd1 = psplSrchPostingsList1->pspSrchPostings + psplSrchPostingsList1->uiSrchPostingsLength;
    pspSrchPostingsEnd2 = psplSrchPostingsList2->pspSrchPostings + psplSrchPostingsList2->uiSrchPostingsLength;


    /* Create a new search postings list structure */
    if ( (iError = iSrchPostingCreateSrchPostingsList(SPI_TERM_TYPE_REGULAR, 0, 0, 
            (((psplSrchPostingsList1->bRequired == true) || (psplSrchPostingsList2->bRequired == true)) ? true : false), 
            NULL, (psplSrchPostingsList1->uiSrchPostingsLength + psplSrchPostingsList2->uiSrchPostingsLength), &psplSrchPostingsList)) != SRCH_NoError ) {
        return (iError);
    }


    /* Loop through the outer postings array */
    for ( pspSrchPostingsPtr = psplSrchPostingsList->pspSrchPostings; pspSrchPostingsPtr1 < pspSrchPostingsEnd1; pspSrchPostingsPtr1++ ) {

/*         iUtlLogDebug(UTL_LOG_CONTEXT, "uiDocumentID1 [%u], uiTermPosition [%d] fWeight [%.4f] (tracking/1)",  */
/*                 pspSrchPostingsPtr1->uiDocumentID, pspSrchPostingsPtr1->uiTermPosition, pspSrchPostingsPtr1->fWeight); */

        /* Loop on the inner postings array */
        for ( ; (pspSrchPostingsPtr2 < pspSrchPostingsEnd2) && (pspSrchPostingsPtr1->uiDocumentID >= pspSrchPostingsPtr2->uiDocumentID); pspSrchPostingsPtr2++ ) {

/*             iUtlLogDebug(UTL_LOG_CONTEXT, "uiDocumentID2 [%u], uiTermPosition [%d] fWeight [%.4f] (tracking/2)",  */
/*                     pspSrchPostingsPtr2->uiDocumentID, pspSrchPostingsPtr2->uiTermPosition, pspSrchPostingsPtr2->fWeight); */

            /* Different document, add inner posting */
            if ( pspSrchPostingsPtr1->uiDocumentID != pspSrchPostingsPtr2->uiDocumentID ) {

                SRCH_POSTING_COPY_SRCH_POSTING(pspSrchPostingsPtr, pspSrchPostingsPtr2);
                pspSrchPostingsPtr++;

/*                 iUtlLogDebug(UTL_LOG_CONTEXT, "uiDocumentID2 [%u], uiTermPosition [%d] fWeight [%.4f] (added/1)",  */
/*                         (pspSrchPostingsPtr - 1)->uiDocumentID, (pspSrchPostingsPtr - 1)->uiTermPosition, (pspSrchPostingsPtr - 1)->fWeight); */
            }
            /* Same document, skip */
/*             else { */
/*                 iUtlLogDebug(UTL_LOG_CONTEXT, "uiDocumentID1 [%u], uiTermPosition [%d] fWeight [%.4f] (skip/1)",  */
/*                         pspSrchPostingsPtr1->uiDocumentID, pspSrchPostingsPtr1->uiTermPosition, pspSrchPostingsPtr1->fWeight); */
/*                 iUtlLogDebug(UTL_LOG_CONTEXT, "uiDocumentID2 [%u], uiTermPosition [%d] fWeight [%.4f] (skip/1)",  */
/*                         pspSrchPostingsPtr2->uiDocumentID, pspSrchPostingsPtr2->uiTermPosition, pspSrchPostingsPtr2->fWeight); */
/*             } */
        }
        
        /* Add this outer posting */
        if ( (pspSrchPostingsPtr2 == psplSrchPostingsList2->pspSrchPostings) || 
                ((pspSrchPostingsPtr2 > psplSrchPostingsList2->pspSrchPostings) && (pspSrchPostingsPtr1->uiDocumentID != (pspSrchPostingsPtr2 - 1)->uiDocumentID)) ) {

            SRCH_POSTING_COPY_SRCH_POSTING(pspSrchPostingsPtr, pspSrchPostingsPtr1);
            pspSrchPostingsPtr++;

/*             iUtlLogDebug(UTL_LOG_CONTEXT, "uiDocumentID1 [%u], uiTermPosition [%d] fWeight [%.4f] (added/2)",  */
/*                     (pspSrchPostingsPtr - 1)->uiDocumentID, (pspSrchPostingsPtr - 1)->uiTermPosition, (pspSrchPostingsPtr - 1)->fWeight); */
        }
    }


    /* Merge the rest of the inner postings array */
    if ( pspSrchPostingsPtr2 < pspSrchPostingsEnd2 ) {

        uiSrchPostingsLength = pspSrchPostingsEnd2 - pspSrchPostingsPtr2;
        s_memcpy((void*)pspSrchPostingsPtr, (void*)pspSrchPostingsPtr2, uiSrchPostingsLength * sizeof(struct srchPosting));
        pspSrchPostingsPtr += uiSrchPostingsLength;

/*         iUtlLogDebug(UTL_LOG_CONTEXT, "uiSrchPostingsLength: %u (added/3)", uiSrchPostingsLength); */
    }


    /* Get the size of the new postings array */
    uiSrchPostingsLength = pspSrchPostingsPtr - psplSrchPostingsList->pspSrchPostings;
    
    /* Set the term count and postings length */
    psplSrchPostingsList->uiTermCount = uiSrchPostingsLength;
    psplSrchPostingsList->uiSrchPostingsLength = uiSrchPostingsLength;

    /* Adjust the size of the new postings array */
    if ( uiSrchPostingsLength < psplSrchPostingsList->uiSrchPostingsLength ) {

        if ( uiSrchPostingsLength > 0 ) {

            if ( (pspSrchPostingsPtr = (struct srchPosting *)s_realloc(psplSrchPostingsList->pspSrchPostings, (size_t)(uiSrchPostingsLength * sizeof(struct srchPosting)))) == NULL ) {
                iSrchPostingFreeSrchPostingsList(psplSrchPostingsList);
                psplSrchPostingsList = NULL;
                return (SRCH_MemError);
            }

            psplSrchPostingsList->pspSrchPostings = pspSrchPostingsPtr;
        }
        else {
            s_free(psplSrchPostingsList->pspSrchPostings);
        }
    }


    /* Count up the number of documents in this posting */
    for ( pspSrchPostingsPtr = psplSrchPostingsList->pspSrchPostings, pspSrchPostingsEnd = psplSrchPostingsList->pspSrchPostings + psplSrchPostingsList->uiSrchPostingsLength, 
            psplSrchPostingsList->uiDocumentCount = 0, uiCurrentDocumentID = 0; pspSrchPostingsPtr < pspSrchPostingsEnd; pspSrchPostingsPtr++ ) {

        if ( pspSrchPostingsPtr->uiDocumentID != uiCurrentDocumentID ) {
            psplSrchPostingsList->uiDocumentCount++;
            uiCurrentDocumentID = pspSrchPostingsPtr->uiDocumentID;
        }
    }


    /* Free the two old postings lists */
    iSrchPostingFreeSrchPostingsList(psplSrchPostingsList1);
    psplSrchPostingsList1 = NULL;

    iSrchPostingFreeSrchPostingsList(psplSrchPostingsList2);
    psplSrchPostingsList2 = NULL;


    ASSERT(iSrchPostingCheckSrchPostingsList(psplSrchPostingsList) == SRCH_NoError);

/*     iUtlLogDebug(UTL_LOG_CONTEXT, "psplSrchPostingsList"); */
/*     iSrchPostingPrintSrchPostingsList(psplSrchPostingsList); */


    /* Set the return pointer */
    *ppsplSrchPostingsList = psplSrchPostingsList;

    /* Return  */
    return (SRCH_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchPostingMergeSrchPostingsListsAND()

    Purpose:    This function ANDs psplSrchPostingsList2 and psplSrchPostingsList1 
                and returns a pointer to a new search postings list structure.

    Parameters: psplSrchPostingsList1               search postings list structure
                psplSrchPostingsList2               search postings list structure
                uiSrchPostingBooleanOperationID     posting boolean operation ID
                ppsplSrchPostingsList               return pointer for the search postings list structure

    Globals:    none

    Returns:    SRCH error code


*/
int iSrchPostingMergeSrchPostingsListsAND
(
    struct srchPostingsList *psplSrchPostingsList1,
    struct srchPostingsList *psplSrchPostingsList2,
    unsigned int uiSrchPostingBooleanOperationID,
    struct srchPostingsList **ppsplSrchPostingsList
)
{

    int                         iError = SRCH_NoError;
    unsigned int                uiSrchPostingsLength = 0;
    struct srchPostingsList     *psplSrchPostingsList = NULL;
    struct srchPosting          *pspSrchPostingsPtr1 = NULL;
    struct srchPosting          *pspSrchPostingsPtr2 = NULL;
    struct srchPosting          *pspSrchPostingsEnd1 = NULL;
    struct srchPosting          *pspSrchPostingsEnd2 = NULL;
    struct srchPosting          *pspSrchPostingsPtr = NULL;
    struct srchPosting          *pspSrchPostingsEnd = NULL;
    struct srchPosting          *pspSrchPostingsCurrentPtr = NULL;
    unsigned int                uiCurrentDocumentID = 0;
#if defined(SRCH_POSTING_ENABLE_PROXIMITY_REWEIGHTING)
    int                         iCurrentTermPosition = -1;
#endif    /* defined(SRCH_POSTING_ENABLE_PROXIMITY_REWEIGHTING) */
    float                       fCurrentWeight = 0;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iSrchPostingMergeSrchPostingsListsAND"); */
/*     iUtlLogDebug(UTL_LOG_CONTEXT, "psplSrchPostingsList1"); */
/*     iSrchPostingPrintSrchPostingsList(psplSrchPostingsList1); */
/*     iUtlLogDebug(UTL_LOG_CONTEXT, "psplSrchPostingsList2"); */
/*     iSrchPostingPrintSrchPostingsList(psplSrchPostingsList2); */


    /* Check the parameters */
    if ( (psplSrchPostingsList1 != NULL) && (iSrchPostingCheckSrchPostingsList(psplSrchPostingsList1) != SRCH_NoError) ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'psplSrchPostingsList1' parameter passed to 'iSrchPostingMergeSrchPostingsListsAND'."); 
        return (SRCH_PostingInvalidPostingsList);
    }

    if ( (psplSrchPostingsList2 != NULL) && (iSrchPostingCheckSrchPostingsList(psplSrchPostingsList2) != SRCH_NoError) ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'psplSrchPostingsList2' parameter passed to 'iSrchPostingMergeSrchPostingsListsAND'."); 
        return (SRCH_PostingInvalidPostingsList);
    }

    if ( !((uiSrchPostingBooleanOperationID == SRCH_POSTING_BOOLEAN_OPERATION_RELAXED_ID) || (uiSrchPostingBooleanOperationID == SRCH_POSTING_BOOLEAN_OPERATION_STRICT_ID)) ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiSrchPostingBooleanOperationID' parameter passed to 'iSrchPostingMergeSrchPostingsListsAND'."); 
        return (SRCH_PostingInvalidSearchBooleanModifier);
    }

    if ( ppsplSrchPostingsList == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppsplSrchPostingsList' parameter passed to 'iSrchPostingMergeSrchPostingsListsAND'."); 
        return (SRCH_ReturnParameterError);
    }


    /* If either of the postings lists are NULL or of zero length we try to return the
    ** other one, if they are both null then we just return an empty search postings list structure
    */
    if ( (psplSrchPostingsList1 == NULL) || (psplSrchPostingsList1->uiSrchPostingsLength == 0) || 
            (psplSrchPostingsList2 == NULL) || (psplSrchPostingsList2->uiSrchPostingsLength == 0) ) {

        /* Return the non-empty set if we are using strict booleans and the non-empty one contains a stop term, 
        ** or if we are using relaxed booleans and the non-empty one contains a non-required term
        */ 
        if ( (psplSrchPostingsList1 != NULL) && (psplSrchPostingsList2 != NULL) ) {
                
            if ( (psplSrchPostingsList1->uiSrchPostingsLength > 0) &&
                    ( ((uiSrchPostingBooleanOperationID == SRCH_POSTING_BOOLEAN_OPERATION_STRICT_ID) && (psplSrchPostingsList2->uiTermType == SPI_TERM_TYPE_STOP)) ||
                    ((uiSrchPostingBooleanOperationID == SRCH_POSTING_BOOLEAN_OPERATION_RELAXED_ID) && (psplSrchPostingsList2->bRequired == false)) ) ) {
            
                iSrchPostingFreeSrchPostingsList(psplSrchPostingsList2);
                psplSrchPostingsList2 = NULL;

                *ppsplSrchPostingsList = psplSrchPostingsList1;
                return (SRCH_NoError);
            }
            else if ( (psplSrchPostingsList2->uiSrchPostingsLength > 0) &&
                    ( ((uiSrchPostingBooleanOperationID == SRCH_POSTING_BOOLEAN_OPERATION_STRICT_ID) && (psplSrchPostingsList1->uiTermType == SPI_TERM_TYPE_STOP)) ||
                    ((uiSrchPostingBooleanOperationID == SRCH_POSTING_BOOLEAN_OPERATION_RELAXED_ID) && (psplSrchPostingsList1->bRequired == false)) ) ) {
    
                iSrchPostingFreeSrchPostingsList(psplSrchPostingsList1);
                psplSrchPostingsList1 = NULL;

                *ppsplSrchPostingsList = psplSrchPostingsList2;
                return (SRCH_NoError);
            }
        }
        /* Return the non-NULL set if we are using relaxed booleans */
        else if ( uiSrchPostingBooleanOperationID == SRCH_POSTING_BOOLEAN_OPERATION_RELAXED_ID ) {
            
            if ( (psplSrchPostingsList1 == NULL) && (psplSrchPostingsList2 != NULL)  ) {
                *ppsplSrchPostingsList = psplSrchPostingsList2;
                return (SRCH_NoError);
            }
            else if ( (psplSrchPostingsList2 == NULL) && (psplSrchPostingsList1 != NULL) ) {
                *ppsplSrchPostingsList = psplSrchPostingsList1;
                return (SRCH_NoError);
            }
        }
    

        /* Return an empty postings list */
        {
            int     uiTermType = SPI_TERM_TYPE_UNKNOWN;
    
            /* Set the term type to stop word is both passed term types were stop words */
            if ( (psplSrchPostingsList1 != NULL) && (psplSrchPostingsList1->uiTermType == SPI_TERM_TYPE_STOP) && 
                    (psplSrchPostingsList2 != NULL) && (psplSrchPostingsList2->uiTermType == SPI_TERM_TYPE_STOP) ) {
                uiTermType = SPI_TERM_TYPE_STOP;
            }
    
            /* Create an empty postings list */
            if ( (iError = iSrchPostingCreateSrchPostingsList(uiTermType, SPI_TERM_COUNT_UNKNOWN, SPI_TERM_DOCUMENT_COUNT_UNKNOWN, 
                    false, NULL, 0, &psplSrchPostingsList)) != SRCH_NoError ) {
                return (iError);
            }
    
            iSrchPostingFreeSrchPostingsList(psplSrchPostingsList1);
            psplSrchPostingsList1 = NULL;
    
            iSrchPostingFreeSrchPostingsList(psplSrchPostingsList2);
            psplSrchPostingsList2 = NULL;
    
            *ppsplSrchPostingsList = psplSrchPostingsList;
            return (SRCH_NoError);
        }
    }


    ASSERT(psplSrchPostingsList1->uiSrchPostingsLength > 0);
    ASSERT(psplSrchPostingsList2->uiSrchPostingsLength > 0);


    /* Set up our variables */
    pspSrchPostingsPtr1 = psplSrchPostingsList1->pspSrchPostings;
    pspSrchPostingsPtr2 = psplSrchPostingsList2->pspSrchPostings;
    pspSrchPostingsEnd1 = psplSrchPostingsList1->pspSrchPostings + psplSrchPostingsList1->uiSrchPostingsLength;
    pspSrchPostingsEnd2 = psplSrchPostingsList2->pspSrchPostings + psplSrchPostingsList2->uiSrchPostingsLength;


    /* Create a new search postings list structure */
    if ( (iError = iSrchPostingCreateSrchPostingsList(SPI_TERM_TYPE_REGULAR, 0, 0, 
            (((psplSrchPostingsList1->bRequired == true) || (psplSrchPostingsList2->bRequired == true)) ? true : false), 
            NULL, UTL_MACROS_MIN(psplSrchPostingsList1->uiSrchPostingsLength, psplSrchPostingsList2->uiSrchPostingsLength), &psplSrchPostingsList)) != SRCH_NoError ) {
        return (iError);
    }


    /* Initialize our working variables */
    pspSrchPostingsCurrentPtr = NULL;
    uiCurrentDocumentID = 0;
#if defined(SRCH_POSTING_ENABLE_PROXIMITY_REWEIGHTING)
    iCurrentTermPosition = -1;
#endif    /* defined(SRCH_POSTING_ENABLE_PROXIMITY_REWEIGHTING) */
    fCurrentWeight = 0;


    /* Loop through the outer postings */
    for ( pspSrchPostingsPtr = psplSrchPostingsList->pspSrchPostings; pspSrchPostingsPtr1 < pspSrchPostingsEnd1; pspSrchPostingsPtr1++ ) {

/*         iUtlLogDebug(UTL_LOG_CONTEXT, "uiDocumentID1 [%u], uiTermPosition [%d] fWeight [%.4f] (tracking/1)",  */
/*                 pspSrchPostingsPtr1->uiDocumentID, pspSrchPostingsPtr1->uiTermPosition, pspSrchPostingsPtr1->fWeight); */

        /* Loop on the inner postings */
        for ( ; (pspSrchPostingsPtr2 < pspSrchPostingsEnd2) && (pspSrchPostingsPtr1->uiDocumentID >= pspSrchPostingsPtr2->uiDocumentID); pspSrchPostingsPtr2++ ) {

/*             iUtlLogDebug(UTL_LOG_CONTEXT, "uiDocumentID2 [%u], uiTermPosition [%d] fWeight [%.4f] (tracking/2)",  */
/*                     pspSrchPostingsPtr2->uiDocumentID, pspSrchPostingsPtr2->uiTermPosition, pspSrchPostingsPtr2->fWeight); */

            /* Add the current posting if there is one and we have run into a new document in the inner postings */
            if ( (pspSrchPostingsCurrentPtr != NULL) && (pspSrchPostingsPtr2->uiDocumentID != pspSrchPostingsCurrentPtr->uiDocumentID) ) {

                SRCH_POSTING_COPY_SRCH_POSTING(pspSrchPostingsPtr, pspSrchPostingsCurrentPtr);
                pspSrchPostingsPtr->fWeight = fCurrentWeight;
                pspSrchPostingsPtr++;

/*                 iUtlLogDebug(UTL_LOG_CONTEXT, "uiDocumentID [%u], uiTermPosition [%d] fWeight [%.4f] (added/1)",  */
/*                         (pspSrchPostingsPtr - 1)->uiDocumentID, (pspSrchPostingsPtr - 1)->uiTermPosition, (pspSrchPostingsPtr - 1)->fWeight); */

                pspSrchPostingsCurrentPtr = NULL;
                uiCurrentDocumentID = 0;
#if defined(SRCH_POSTING_ENABLE_PROXIMITY_REWEIGHTING)
                iCurrentTermPosition = -1;
#endif    /* defined(SRCH_POSTING_ENABLE_PROXIMITY_REWEIGHTING) */
                fCurrentWeight = 0;
            }


            /* Process if the document IDs match */
            if ( pspSrchPostingsPtr1->uiDocumentID == pspSrchPostingsPtr2->uiDocumentID ) {

                /* Set the current document ID */
                uiCurrentDocumentID = pspSrchPostingsPtr1->uiDocumentID;

                /* Increment the weight and set the current posting */
                if ( pspSrchPostingsPtr1->uiTermPosition >= pspSrchPostingsPtr2->uiTermPosition ) {

                    fCurrentWeight += pspSrchPostingsPtr2->fWeight;

#if defined(SRCH_POSTING_ENABLE_PROXIMITY_REWEIGHTING)
                    if ( pspSrchPostingsPtr2->uiTermPosition == (iCurrentTermPosition + 1) ) {
                        fCurrentWeight *= SRCH_POSTING_PROXIMITY_REWEIGHTING;
                    }
                    iCurrentTermPosition = pspSrchPostingsPtr2->uiTermPosition;
#endif    /* defined(SRCH_POSTING_ENABLE_PROXIMITY_REWEIGHTING) */

                    pspSrchPostingsCurrentPtr = pspSrchPostingsPtr2;
/*                     iUtlLogDebug(UTL_LOG_CONTEXT, "uiDocumentID2 [%u], uiTermPosition [%d] fWeight [%.4f] (incremented/1)",  */
/*                             pspSrchPostingsPtr2->uiDocumentID, pspSrchPostingsPtr2->uiTermPosition, pspSrchPostingsPtr2->fWeight); */
                }
                else {

                    /* Add the current posting if there is one and we have run into a new document in the inner postings */
                    if ( (pspSrchPostingsCurrentPtr != NULL) && (pspSrchPostingsPtr2->uiDocumentID != pspSrchPostingsCurrentPtr->uiDocumentID) ) {
    
                        SRCH_POSTING_COPY_SRCH_POSTING(pspSrchPostingsPtr, pspSrchPostingsCurrentPtr);
                        pspSrchPostingsPtr->fWeight = fCurrentWeight;
                        pspSrchPostingsPtr++;

/*                         iUtlLogDebug(UTL_LOG_CONTEXT, "uiDocumentID [%u], uiTermPosition [%d] fWeight [%.4f] (added/2)",  */
/*                                 (pspSrchPostingsPtr - 1)->uiDocumentID, (pspSrchPostingsPtr - 1)->uiTermPosition, (pspSrchPostingsPtr - 1)->fWeight); */
        
                        pspSrchPostingsCurrentPtr = NULL;
                        uiCurrentDocumentID = 0;
#if defined(SRCH_POSTING_ENABLE_PROXIMITY_REWEIGHTING)
                        iCurrentTermPosition = -1;
#endif    /* defined(SRCH_POSTING_ENABLE_PROXIMITY_REWEIGHTING) */
                        fCurrentWeight = 0;
                    }

                    break;
                }
            }
            /* Increment the weight and set the current posting if we are still in the same document */
            else if ( pspSrchPostingsPtr2->uiDocumentID == uiCurrentDocumentID ) {

                fCurrentWeight += pspSrchPostingsPtr2->fWeight;

#if defined(SRCH_POSTING_ENABLE_PROXIMITY_REWEIGHTING)
                if (  pspSrchPostingsPtr2->uiTermPosition == (iCurrentTermPosition + 1) ) {
                    fCurrentWeight *= SRCH_POSTING_PROXIMITY_REWEIGHTING;
                }
                iCurrentTermPosition = pspSrchPostingsPtr2->uiTermPosition;
#endif    /* defined(SRCH_POSTING_ENABLE_PROXIMITY_REWEIGHTING) */

                pspSrchPostingsCurrentPtr = pspSrchPostingsPtr2;

/*                 iUtlLogDebug(UTL_LOG_CONTEXT, "uiDocumentID2 [%u], uiTermPosition [%d] fWeight [%.4f] (incremented/2)",  */
/*                         pspSrchPostingsPtr2->uiDocumentID, pspSrchPostingsPtr2->uiTermPosition, pspSrchPostingsPtr2->fWeight); */
            } 
        }

        /* Increment the weight and set the current posting if we are still in the same document */
        if ( pspSrchPostingsPtr1->uiDocumentID == uiCurrentDocumentID ) {

            fCurrentWeight += pspSrchPostingsPtr1->fWeight;

#if defined(SRCH_POSTING_ENABLE_PROXIMITY_REWEIGHTING)
            iCurrentTermPosition = pspSrchPostingsPtr1->uiTermPosition;
#endif    /* defined(SRCH_POSTING_ENABLE_PROXIMITY_REWEIGHTING) */

            pspSrchPostingsCurrentPtr = pspSrchPostingsPtr1;

/*             iUtlLogDebug(UTL_LOG_CONTEXT, "uiDocumentID1 [%u], uiTermPosition [%d] fWeight [%.4f] (incremented/3)",  */
/*                     pspSrchPostingsPtr1->uiDocumentID, pspSrchPostingsPtr1->uiTermPosition, pspSrchPostingsPtr1->fWeight); */
        }
    }


    /* Increment the weight from any inner postings we may have missed if we had an early break from the outer postings, and set the current posting */
    if ( ((pspSrchPostingsPtr2 < pspSrchPostingsEnd2) && (pspSrchPostingsPtr2->uiDocumentID == uiCurrentDocumentID)) ) {

        for ( ; ((pspSrchPostingsPtr2 < pspSrchPostingsEnd2) && (pspSrchPostingsPtr2->uiDocumentID == uiCurrentDocumentID)); pspSrchPostingsPtr2++ ) {

            fCurrentWeight += pspSrchPostingsPtr2->fWeight;

#if defined(SRCH_POSTING_ENABLE_PROXIMITY_REWEIGHTING)
            if ( pspSrchPostingsPtr2->uiTermPosition == (iCurrentTermPosition + 1) ) {
                fCurrentWeight *= SRCH_POSTING_PROXIMITY_REWEIGHTING;
            }
            iCurrentTermPosition = pspSrchPostingsPtr2->uiTermPosition;
#endif    /* defined(SRCH_POSTING_ENABLE_PROXIMITY_REWEIGHTING) */

            pspSrchPostingsCurrentPtr = pspSrchPostingsPtr2;

/*             iUtlLogDebug(UTL_LOG_CONTEXT, "uiDocumentID2 [%u], uiTermPosition [%d] fWeight [%.4f] (incremented/4)",  */
/*                     pspSrchPostingsPtr2->uiDocumentID, pspSrchPostingsPtr2->uiTermPosition, pspSrchPostingsPtr2->fWeight); */
        }
    }


    /* Add the current posting if there is one */
    if ( pspSrchPostingsCurrentPtr != NULL ) {

        SRCH_POSTING_COPY_SRCH_POSTING(pspSrchPostingsPtr, pspSrchPostingsCurrentPtr);
        pspSrchPostingsPtr->fWeight = fCurrentWeight;
        pspSrchPostingsPtr++;

/*         iUtlLogDebug(UTL_LOG_CONTEXT, "uiDocumentID [%u], uiTermPosition [%d] fWeight [%.4f] (added/3)",  */
/*                 (pspSrchPostingsPtr - 1)->uiDocumentID, (pspSrchPostingsPtr - 1)->uiTermPosition, (pspSrchPostingsPtr - 1)->fWeight); */
    }


    /* Get the size of the new postings array */
    uiSrchPostingsLength = pspSrchPostingsPtr - psplSrchPostingsList->pspSrchPostings;
    
    /* Set the term count and postings length */
    psplSrchPostingsList->uiTermCount = uiSrchPostingsLength;
    psplSrchPostingsList->uiSrchPostingsLength = uiSrchPostingsLength;

    /* Adjust the size of the new postings array */
    if ( uiSrchPostingsLength < psplSrchPostingsList->uiSrchPostingsLength ) {

        if ( uiSrchPostingsLength > 0 ) {

            if ( (pspSrchPostingsPtr = (struct srchPosting *)s_realloc(psplSrchPostingsList->pspSrchPostings, (size_t)(uiSrchPostingsLength * sizeof(struct srchPosting)))) == NULL ) {
                iSrchPostingFreeSrchPostingsList(psplSrchPostingsList);
                psplSrchPostingsList = NULL;
                return (SRCH_MemError);
            }

            psplSrchPostingsList->pspSrchPostings = pspSrchPostingsPtr;
        }
        else {
            s_free(psplSrchPostingsList->pspSrchPostings);
        }
    }


    /* Count up the number of documents in this posting */
    for ( pspSrchPostingsPtr = psplSrchPostingsList->pspSrchPostings, pspSrchPostingsEnd = psplSrchPostingsList->pspSrchPostings + psplSrchPostingsList->uiSrchPostingsLength, 
            psplSrchPostingsList->uiDocumentCount = 0, uiCurrentDocumentID = 0; pspSrchPostingsPtr < pspSrchPostingsEnd; pspSrchPostingsPtr++ ) {

        if ( pspSrchPostingsPtr->uiDocumentID != uiCurrentDocumentID ) {
            psplSrchPostingsList->uiDocumentCount++;
            uiCurrentDocumentID = pspSrchPostingsPtr->uiDocumentID;
        }
    }


    /* Free the two old postings lists */
    iSrchPostingFreeSrchPostingsList(psplSrchPostingsList1);
    psplSrchPostingsList1 = NULL;

    iSrchPostingFreeSrchPostingsList(psplSrchPostingsList2);
    psplSrchPostingsList2 = NULL;


    ASSERT(iSrchPostingCheckSrchPostingsList(psplSrchPostingsList) == SRCH_NoError);

/*     iUtlLogDebug(UTL_LOG_CONTEXT, "psplSrchPostingsList"); */
/*     iSrchPostingPrintSrchPostingsList(psplSrchPostingsList); */


    /* Set the return pointer */
    *ppsplSrchPostingsList = psplSrchPostingsList;

    /* Return  */
    return (SRCH_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchPostingMergeSrchPostingsListsNOT()

    Purpose:    This function NOTs psplSrchPostingsList2 in psplSrchPostingsList1 
                and returns a pointer to a new search postings list structure.

    Parameters: psplSrchPostingsList1               search postings list structure
                psplSrchPostingsList2               search postings list structure
                uiSrchPostingBooleanOperationID     posting boolean operation ID
                ppsplSrchPostingsList               return pointer for the search postings list structure

    Globals:    none

    Returns:    SRCH error code

*/
int iSrchPostingMergeSrchPostingsListsNOT
(
    struct srchPostingsList *psplSrchPostingsList1,
    struct srchPostingsList *psplSrchPostingsList2,
    unsigned int uiSrchPostingBooleanOperationID,
    struct srchPostingsList **ppsplSrchPostingsList
)
{

    int                         iError = SRCH_NoError;
    unsigned int                uiSrchPostingsLength = 0;
    struct srchPostingsList     *psplSrchPostingsList = NULL;
    struct srchPosting          *pspSrchPostingsPtr1 = NULL;
    struct srchPosting          *pspSrchPostingsPtr2 = NULL;
    struct srchPosting          *pspSrchPostingsEnd1 = NULL;
    struct srchPosting          *pspSrchPostingsEnd2 = NULL;
    struct srchPosting          *pspSrchPostingsPtr = NULL;
    struct srchPosting          *pspSrchPostingsEnd = NULL;
    unsigned int                uiCurrentDocumentID = 0;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iSrchPostingMergeSrchPostingsListsNOT"); */
/*      iUtlLogDebug(UTL_LOG_CONTEXT, "psplSrchPostingsList1"); */
/*      iSrchPostingPrintSrchPostingsList(psplSrchPostingsList1); */
/*      iUtlLogDebug(UTL_LOG_CONTEXT, "psplSrchPostingsList2"); */
/*      iSrchPostingPrintSrchPostingsList(psplSrchPostingsList2); */


    /* Check the parameters */
    if ( (psplSrchPostingsList1 != NULL) && (iSrchPostingCheckSrchPostingsList(psplSrchPostingsList1) != SRCH_NoError) ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'psplSrchPostingsList1' parameter passed to 'iSrchPostingMergeSrchPostingsListsNOT'."); 
        return (SRCH_PostingInvalidPostingsList);
    }

    if ( (psplSrchPostingsList2 != NULL) && (iSrchPostingCheckSrchPostingsList(psplSrchPostingsList2) != SRCH_NoError) ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'psplSrchPostingsList2' parameter passed to 'iSrchPostingMergeSrchPostingsListsNOT'."); 
        return (SRCH_PostingInvalidPostingsList);
    }

    if ( !((uiSrchPostingBooleanOperationID == SRCH_POSTING_BOOLEAN_OPERATION_RELAXED_ID) || (uiSrchPostingBooleanOperationID == SRCH_POSTING_BOOLEAN_OPERATION_STRICT_ID)) ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiSrchPostingBooleanOperationID' parameter passed to 'iSrchPostingMergeSrchPostingsListsNOT'."); 
        return (SRCH_PostingInvalidSearchBooleanModifier);
    }

    if ( ppsplSrchPostingsList == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppsplSrchPostingsList' parameter passed to 'iSrchPostingMergeSrchPostingsListsNOT'."); 
        return (SRCH_ReturnParameterError);
    }


    /* If either of the postings lists are NULL or of zero length, we need to check what we can actually return */
    if ( (psplSrchPostingsList1 == NULL) || (psplSrchPostingsList1->uiSrchPostingsLength == 0) || 
            (psplSrchPostingsList2 == NULL) || (psplSrchPostingsList2->uiSrchPostingsLength == 0) ) {

        /* Return the outer postings list if the inner postings list is empty */ 
        if ( (psplSrchPostingsList1 != NULL) && (psplSrchPostingsList1->uiSrchPostingsLength > 0) && 
                (psplSrchPostingsList2 != NULL) && (psplSrchPostingsList2->uiSrchPostingsLength == 0) ) {
            
            iSrchPostingFreeSrchPostingsList(psplSrchPostingsList2);
            psplSrchPostingsList2 = NULL;

            *ppsplSrchPostingsList = psplSrchPostingsList1;
            return (SRCH_NoError);
        }


        /* Return an empty postings list */
        {
            int     uiTermType = SPI_TERM_TYPE_UNKNOWN;
    
            /* Set the term type to stop word is both passed term types were stop words */
            if ( (psplSrchPostingsList1 != NULL) && (psplSrchPostingsList1->uiTermType == SPI_TERM_TYPE_STOP) && 
                    (psplSrchPostingsList2 != NULL) && (psplSrchPostingsList2->uiTermType == SPI_TERM_TYPE_STOP) ) {
                uiTermType = SPI_TERM_TYPE_STOP;
            }
    
            /* Create an empty postings list */
            if ( (iError = iSrchPostingCreateSrchPostingsList(uiTermType, SPI_TERM_COUNT_UNKNOWN, SPI_TERM_DOCUMENT_COUNT_UNKNOWN, 
                    false, NULL, 0, &psplSrchPostingsList)) != SRCH_NoError ) {
                return (iError);
            }
    
            iSrchPostingFreeSrchPostingsList(psplSrchPostingsList1);
            psplSrchPostingsList1 = NULL;
    
            iSrchPostingFreeSrchPostingsList(psplSrchPostingsList2);
            psplSrchPostingsList2 = NULL;
    
            *ppsplSrchPostingsList = psplSrchPostingsList;
            return (SRCH_NoError);
        }
    }


    ASSERT(psplSrchPostingsList1->uiSrchPostingsLength > 0);
    ASSERT(psplSrchPostingsList2->uiSrchPostingsLength > 0);


    /* Set up our variables */
    pspSrchPostingsPtr1 = psplSrchPostingsList1->pspSrchPostings;
    pspSrchPostingsPtr2 = psplSrchPostingsList2->pspSrchPostings;
    pspSrchPostingsEnd1 = psplSrchPostingsList1->pspSrchPostings + psplSrchPostingsList1->uiSrchPostingsLength;
    pspSrchPostingsEnd2 = psplSrchPostingsList2->pspSrchPostings + psplSrchPostingsList2->uiSrchPostingsLength;


    /* Create a new search postings list structure */
    if ( (iError = iSrchPostingCreateSrchPostingsList(SPI_TERM_TYPE_REGULAR, 0, 0, 
            (((psplSrchPostingsList1->bRequired == true) || (psplSrchPostingsList2->bRequired == true)) ? true : false), 
            NULL, psplSrchPostingsList1->uiSrchPostingsLength, &psplSrchPostingsList)) != SRCH_NoError ) {
        return (iError);
    }


    /* Loop through the outer postings array */
    for ( pspSrchPostingsPtr = psplSrchPostingsList->pspSrchPostings; pspSrchPostingsPtr1 < pspSrchPostingsEnd1; pspSrchPostingsPtr1++ ) {

/*         iUtlLogDebug(UTL_LOG_CONTEXT, "uiDocumentID1 [%u], uiTermPosition [%d] fWeight [%.4f] (tracking/1)",  */
/*                 pspSrchPostingsPtr1->uiDocumentID, pspSrchPostingsPtr1->uiTermPosition, pspSrchPostingsPtr1->fWeight); */

        /* Loop on the inner postings array */
        for ( ; (pspSrchPostingsPtr2 < pspSrchPostingsEnd2) && (pspSrchPostingsPtr1->uiDocumentID >= pspSrchPostingsPtr2->uiDocumentID); pspSrchPostingsPtr2++ ) {

/*             iUtlLogDebug(UTL_LOG_CONTEXT, "uiDocumentID2 [%u], uiTermPosition [%d] fWeight [%.4f] (tracking/2)",  */
/*                     pspSrchPostingsPtr2->uiDocumentID, pspSrchPostingsPtr2->uiTermPosition, pspSrchPostingsPtr2->fWeight); */

            /* Save the uiDocumentID to drop from the outer postings array */
            uiCurrentDocumentID = pspSrchPostingsPtr2->uiDocumentID;

/*             iUtlLogDebug(UTL_LOG_CONTEXT, "uiDocumentID2 [%u], uiTermPosition [%d] fWeight [%.4f] (skipped/1)",  */
/*                     pspSrchPostingsPtr2->uiDocumentID, pspSrchPostingsPtr2->uiTermPosition, pspSrchPostingsPtr2->fWeight); */
        } 

        /* Add this outer posting to the return posting array if it is not to be dropped */
        if ( pspSrchPostingsPtr1->uiDocumentID != uiCurrentDocumentID ) {

            SRCH_POSTING_COPY_SRCH_POSTING(pspSrchPostingsPtr, pspSrchPostingsPtr1);
            pspSrchPostingsPtr++;

/*             iUtlLogDebug(UTL_LOG_CONTEXT, "uiDocumentID1 [%u], uiTermPosition [%d] fWeight [%.4f] (added/1)",  */
/*                     (pspSrchPostingsPtr - 1)->uiDocumentID, (pspSrchPostingsPtr - 1)->uiTermPosition); */
        }
    }


    /* Get the size of the new postings array */
    uiSrchPostingsLength = pspSrchPostingsPtr - psplSrchPostingsList->pspSrchPostings;
    
    /* Set the term count and postings length */
    psplSrchPostingsList->uiTermCount = uiSrchPostingsLength;
    psplSrchPostingsList->uiSrchPostingsLength = uiSrchPostingsLength;

    /* Adjust the size of the new postings array */
    if ( uiSrchPostingsLength < psplSrchPostingsList->uiSrchPostingsLength ) {

        if ( uiSrchPostingsLength > 0 ) {

            if ( (pspSrchPostingsPtr = (struct srchPosting *)s_realloc(psplSrchPostingsList->pspSrchPostings, (size_t)(uiSrchPostingsLength * sizeof(struct srchPosting)))) == NULL ) {
                iSrchPostingFreeSrchPostingsList(psplSrchPostingsList);
                psplSrchPostingsList = NULL;
                return (SRCH_MemError);
            }

            psplSrchPostingsList->pspSrchPostings = pspSrchPostingsPtr;
        }
        else {
            s_free(psplSrchPostingsList->pspSrchPostings);
        }
    }


    /* Count up the number of documents in this posting */
    for ( pspSrchPostingsPtr = psplSrchPostingsList->pspSrchPostings, pspSrchPostingsEnd = psplSrchPostingsList->pspSrchPostings + psplSrchPostingsList->uiSrchPostingsLength, 
            psplSrchPostingsList->uiDocumentCount = 0, uiCurrentDocumentID = 0; pspSrchPostingsPtr < pspSrchPostingsEnd; pspSrchPostingsPtr++ ) {

        if ( pspSrchPostingsPtr->uiDocumentID != uiCurrentDocumentID ) {
            psplSrchPostingsList->uiDocumentCount++;
            uiCurrentDocumentID = pspSrchPostingsPtr->uiDocumentID;
        }
    }


    /* Free the two old postings lists */
    iSrchPostingFreeSrchPostingsList(psplSrchPostingsList1);
    psplSrchPostingsList1 = NULL;

    iSrchPostingFreeSrchPostingsList(psplSrchPostingsList2);
    psplSrchPostingsList2 = NULL;


    ASSERT(iSrchPostingCheckSrchPostingsList(psplSrchPostingsList) == SRCH_NoError);

/*     iUtlLogDebug(UTL_LOG_CONTEXT, "psplSrchPostingsList"); */
/*     iSrchPostingPrintSrchPostingsList(psplSrchPostingsList); */


    /* Set the return pointer */
    *ppsplSrchPostingsList = psplSrchPostingsList;

    /* Return  */
    return (SRCH_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchPostingMergeSrchPostingsListsADJ()

    Purpose:    This function ADJs psplSrchPostingsList2 in psplSrchPostingsList1 
                and returns a pointer to a new search postings list structure.

    Parameters: psplSrchPostingsList1               search postings list structure
                psplSrchPostingsList2               search postings list structure
                iTermDistance                       term distance
                uiSrchPostingBooleanOperationID     posting boolean operation ID
                ppsplSrchPostingsList            return pointer for the search postings list structure

    Globals:    none

    Returns:    SRCH error code

*/
int iSrchPostingMergeSrchPostingsListsADJ
(
    struct srchPostingsList *psplSrchPostingsList1,
    struct srchPostingsList *psplSrchPostingsList2,
    int iTermDistance,
    unsigned int uiSrchPostingBooleanOperationID,
    struct srchPostingsList **ppsplSrchPostingsList
)
{

    int                         iError = SRCH_NoError;
    unsigned int                uiSrchPostingsLength = 0;
    struct srchPostingsList     *psplSrchPostingsList = NULL;
    struct srchPosting          *pspSrchPostingsPtr1 = NULL;
    struct srchPosting          *pspSrchPostingsPtr2 = NULL;
    struct srchPosting          *pspSrchPostingsEnd1 = NULL;
    struct srchPosting          *pspSrchPostingsEnd2 = NULL;
    struct srchPosting          *pspSrchPostingsPtr = NULL;
    struct srchPosting          *pspSrchPostingsEnd = NULL;
    unsigned int                uiCurrentDocumentID = 0;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iSrchPostingMergeSrchPostingsListsADJ - iTermDistance: %d", iTermDistance); */
/*     iUtlLogDebug(UTL_LOG_CONTEXT, "psplSrchPostingsList1"); */
/*     iSrchPostingPrintSrchPostingsList(psplSrchPostingsList1); */
/*     iUtlLogDebug(UTL_LOG_CONTEXT, "psplSrchPostingsList2"); */
/*     iSrchPostingPrintSrchPostingsList(psplSrchPostingsList2); */


    /* Check the parameters */
    if ( (psplSrchPostingsList1 != NULL) && (iSrchPostingCheckSrchPostingsList(psplSrchPostingsList1) != SRCH_NoError) ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'psplSrchPostingsList1' parameter passed to 'iSrchPostingMergeSrchPostingsListsADJ'."); 
        return (SRCH_PostingInvalidPostingsList);
    }

    if ( (psplSrchPostingsList2 != NULL) && (iSrchPostingCheckSrchPostingsList(psplSrchPostingsList2) != SRCH_NoError) ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'psplSrchPostingsList2' parameter passed to 'iSrchPostingMergeSrchPostingsListsADJ'."); 
        return (SRCH_PostingInvalidPostingsList);
    }

    if ( iTermDistance <= 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'iTermDistance' parameter passed to 'iSrchPostingMergeSrchPostingsListsADJ'."); 
        return (SRCH_PostingInvalidTermDistance);
    }

    if ( !((uiSrchPostingBooleanOperationID == SRCH_POSTING_BOOLEAN_OPERATION_RELAXED_ID) || (uiSrchPostingBooleanOperationID == SRCH_POSTING_BOOLEAN_OPERATION_STRICT_ID)) ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiSrchPostingBooleanOperationID' parameter passed to 'iSrchPostingMergeSrchPostingsListsADJ'."); 
        return (SRCH_PostingInvalidSearchBooleanModifier);
    }

    if ( ppsplSrchPostingsList == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppsplSrchPostingsList' parameter passed to 'iSrchPostingMergeSrchPostingsListsADJ'."); 
        return (SRCH_ReturnParameterError);
    }


    /* If either of the postings lists are NULL or of zero length we try to return the
    ** other one, if they are both null then we just return an empty search postings list structure
    */
    if ( (psplSrchPostingsList1 == NULL) || (psplSrchPostingsList1->uiSrchPostingsLength == 0) || 
            (psplSrchPostingsList2 == NULL) || (psplSrchPostingsList2->uiSrchPostingsLength == 0) ) {

        /* Return the non-empty set if we are using strict booleans and the non-empty one contains a stop term, 
        ** or if we are using relaxed booleans and the non-empty one contains a non-required term
        */ 
        if ( (psplSrchPostingsList1 != NULL) && (psplSrchPostingsList2 != NULL) ) {
                
            if ( (psplSrchPostingsList1->uiSrchPostingsLength > 0) &&
                    ( ((uiSrchPostingBooleanOperationID == SRCH_POSTING_BOOLEAN_OPERATION_STRICT_ID) && (psplSrchPostingsList2->uiTermType == SPI_TERM_TYPE_STOP)) ||
                    ((uiSrchPostingBooleanOperationID == SRCH_POSTING_BOOLEAN_OPERATION_RELAXED_ID) && (psplSrchPostingsList2->bRequired == false)) ) ) {
                
                iSrchPostingFreeSrchPostingsList(psplSrchPostingsList2);
                psplSrchPostingsList2 = NULL;

                *ppsplSrchPostingsList = psplSrchPostingsList1;
                return (SRCH_NoError);
            }
            else if ( (psplSrchPostingsList2->uiSrchPostingsLength > 0) &&
                    ( ((uiSrchPostingBooleanOperationID == SRCH_POSTING_BOOLEAN_OPERATION_STRICT_ID) && (psplSrchPostingsList1->uiTermType == SPI_TERM_TYPE_STOP)) ||
                    ((uiSrchPostingBooleanOperationID == SRCH_POSTING_BOOLEAN_OPERATION_RELAXED_ID) && (psplSrchPostingsList1->bRequired == false)) ) ) {
    
                iSrchPostingFreeSrchPostingsList(psplSrchPostingsList1);
                psplSrchPostingsList1 = NULL;

                *ppsplSrchPostingsList = psplSrchPostingsList2;
                return (SRCH_NoError);
            }
        }
        /* Return the non-NULL set if we are using relaxed booleans */
        else if ( uiSrchPostingBooleanOperationID == SRCH_POSTING_BOOLEAN_OPERATION_RELAXED_ID ) {
            
            if ( (psplSrchPostingsList1 == NULL) && (psplSrchPostingsList2 != NULL)  ) {
                *ppsplSrchPostingsList = psplSrchPostingsList2;
                return (SRCH_NoError);
            }
            else if ( (psplSrchPostingsList2 == NULL) && (psplSrchPostingsList1 != NULL) ) {
                *ppsplSrchPostingsList = psplSrchPostingsList1;
                return (SRCH_NoError);
            }
        }
    

        /* Return an empty postings list */
        {
            int     uiTermType = SPI_TERM_TYPE_UNKNOWN;
    
            /* Set the term type to stop word is both passed term types were stop words */
            if ( (psplSrchPostingsList1 != NULL) && (psplSrchPostingsList1->uiTermType == SPI_TERM_TYPE_STOP) && 
                    (psplSrchPostingsList2 != NULL) && (psplSrchPostingsList2->uiTermType == SPI_TERM_TYPE_STOP) ) {
                uiTermType = SPI_TERM_TYPE_STOP;
            }
    
            /* Create an empty postings list */
            if ( (iError = iSrchPostingCreateSrchPostingsList(uiTermType, SPI_TERM_COUNT_UNKNOWN, SPI_TERM_DOCUMENT_COUNT_UNKNOWN, 
                    false, NULL, 0, &psplSrchPostingsList)) != SRCH_NoError ) {
                return (iError);
            }
    
            iSrchPostingFreeSrchPostingsList(psplSrchPostingsList1);
            psplSrchPostingsList1 = NULL;
    
            iSrchPostingFreeSrchPostingsList(psplSrchPostingsList2);
            psplSrchPostingsList2 = NULL;
    
            *ppsplSrchPostingsList = psplSrchPostingsList;
            return (SRCH_NoError);
        }
    }


    ASSERT(psplSrchPostingsList1->uiSrchPostingsLength > 0);
    ASSERT(psplSrchPostingsList2->uiSrchPostingsLength > 0);


    /* Set up our variables */
    pspSrchPostingsPtr1 = psplSrchPostingsList1->pspSrchPostings;
    pspSrchPostingsPtr2 = psplSrchPostingsList2->pspSrchPostings;
    pspSrchPostingsEnd1 = psplSrchPostingsList1->pspSrchPostings + psplSrchPostingsList1->uiSrchPostingsLength;
    pspSrchPostingsEnd2 = psplSrchPostingsList2->pspSrchPostings + psplSrchPostingsList2->uiSrchPostingsLength;


    /* Create a new search postings list structure */
    if ( (iError = iSrchPostingCreateSrchPostingsList(SPI_TERM_TYPE_REGULAR, 0, 0, 
            (((psplSrchPostingsList1->bRequired == true) || (psplSrchPostingsList2->bRequired == true)) ? true : false), 
            NULL, UTL_MACROS_MAX(psplSrchPostingsList1->uiSrchPostingsLength, psplSrchPostingsList2->uiSrchPostingsLength), &psplSrchPostingsList)) != SRCH_NoError ) {
        return (iError);
    }


    /* Loop on the inner postings array */
    for ( pspSrchPostingsPtr = psplSrchPostingsList->pspSrchPostings; pspSrchPostingsPtr2 < pspSrchPostingsEnd2; pspSrchPostingsPtr2++ ) {

/*         iUtlLogDebug(UTL_LOG_CONTEXT, "uiDocumentID2 [%u], uiTermPosition [%d] fWeight [%.4f] (tracking/2)",  */
/*                 pspSrchPostingsPtr2->uiDocumentID, pspSrchPostingsPtr2->uiTermPosition, pspSrchPostingsPtr2->fWeight); */

        /* Loop through the outer postings array */
        for ( ; (pspSrchPostingsPtr1 < pspSrchPostingsEnd1) && (pspSrchPostingsPtr2->uiDocumentID >= pspSrchPostingsPtr1->uiDocumentID); pspSrchPostingsPtr1++ ) {

/*             iUtlLogDebug(UTL_LOG_CONTEXT, "uiDocumentID1 [%u], uiTermPosition [%d] fWeight [%.4f] (tracking/1)",  */
/*                     pspSrchPostingsPtr1->uiDocumentID, pspSrchPostingsPtr1->uiTermPosition, pspSrchPostingsPtr1->fWeight); */

            /* Skip terms positions of zero since those are meta-terms with no location */
            if ( pspSrchPostingsPtr1->uiDocumentID == pspSrchPostingsPtr2->uiDocumentID ) {

                /* Work out the term position delta, hoisted here for speed */
                int iTermPositionDelta = (pspSrchPostingsPtr2->uiTermPosition - pspSrchPostingsPtr1->uiTermPosition);

                /* Look for A B adjacency - skip terms positions of zero since those are meta-terms with no location  */
                if ( ((iTermPositionDelta == 0) || (iTermPositionDelta == iTermDistance)) &&
                        (pspSrchPostingsPtr1->uiTermPosition != 0) && (pspSrchPostingsPtr2->uiTermPosition != 0) ) {

                    SRCH_POSTING_COPY_SRCH_POSTING(pspSrchPostingsPtr, pspSrchPostingsPtr2);
                    pspSrchPostingsPtr->fWeight += pspSrchPostingsPtr1->fWeight;

#if defined(SRCH_POSTING_ENABLE_PROXIMITY_REWEIGHTING)
                    pspSrchPostingsPtr->fWeight *= SRCH_POSTING_PROXIMITY_REWEIGHTING;
#endif    /* defined(SRCH_POSTING_ENABLE_PROXIMITY_REWEIGHTING) */

                    pspSrchPostingsPtr++;

/*                     iUtlLogDebug(UTL_LOG_CONTEXT, "uiDocumentID2 [%u], uiTermPosition [%d] fWeight [%.4f] (added/1)",  */
/*                             (pspSrchPostingsPtr - 1)->uiDocumentID, (pspSrchPostingsPtr - 1)->uiTermPosition, (pspSrchPostingsPtr - 1)->fWeight); */
                }
                else if ( iTermPositionDelta < 0 ) {
                    break;
                }
            }
        } 
    }


    /* Get the size of the new postings array */
    uiSrchPostingsLength = pspSrchPostingsPtr - psplSrchPostingsList->pspSrchPostings;
    
    /* Set the term count and postings length */
    psplSrchPostingsList->uiTermCount = uiSrchPostingsLength;
    psplSrchPostingsList->uiSrchPostingsLength = uiSrchPostingsLength;

    /* Adjust the size of the new postings array */
    if ( uiSrchPostingsLength < psplSrchPostingsList->uiSrchPostingsLength ) {

        if ( uiSrchPostingsLength > 0 ) {

            if ( (pspSrchPostingsPtr = (struct srchPosting *)s_realloc(psplSrchPostingsList->pspSrchPostings, (size_t)(uiSrchPostingsLength * sizeof(struct srchPosting)))) == NULL ) {
                iSrchPostingFreeSrchPostingsList(psplSrchPostingsList);
                psplSrchPostingsList = NULL;
                return (SRCH_MemError);
            }

            psplSrchPostingsList->pspSrchPostings = pspSrchPostingsPtr;
        }
        else {
            s_free(psplSrchPostingsList->pspSrchPostings);
        }
    }


    /* Count up the number of documents in this posting */
    for ( pspSrchPostingsPtr = psplSrchPostingsList->pspSrchPostings, pspSrchPostingsEnd = psplSrchPostingsList->pspSrchPostings + psplSrchPostingsList->uiSrchPostingsLength, 
            psplSrchPostingsList->uiDocumentCount = 0, uiCurrentDocumentID = 0; pspSrchPostingsPtr < pspSrchPostingsEnd; pspSrchPostingsPtr++ ) {

        if ( pspSrchPostingsPtr->uiDocumentID != uiCurrentDocumentID ) {
            psplSrchPostingsList->uiDocumentCount++;
            uiCurrentDocumentID = pspSrchPostingsPtr->uiDocumentID;
        }
    }


    /* Free the two old postings lists */
    iSrchPostingFreeSrchPostingsList(psplSrchPostingsList1);
    psplSrchPostingsList1 = NULL;

    iSrchPostingFreeSrchPostingsList(psplSrchPostingsList2);
    psplSrchPostingsList2 = NULL;


    ASSERT(iSrchPostingCheckSrchPostingsList(psplSrchPostingsList) == SRCH_NoError);

/*     iUtlLogDebug(UTL_LOG_CONTEXT, "psplSrchPostingsList"); */
/*     iSrchPostingPrintSrchPostingsList(psplSrchPostingsList); */


    /* Set the return pointer */
    *ppsplSrchPostingsList = psplSrchPostingsList;

    /* Return  */
    return (SRCH_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchPostingMergeSrchPostingsListsNEAR()

    Purpose:    This function NEARs psplSrchPostingsList2 in psplSrchPostingsList1 
                and returns a pointer to a new search postings list structure.

    Parameters: psplSrchPostingsList1               search postings list structure
                psplSrchPostingsList2               search postings list structure
                iTermDistance                       term distance
                bTermOrderMatters                   term order matters
                uiSrchPostingBooleanOperationID     posting boolean operation ID
                ppsplSrchPostingsList               return pointer for the search postings list structure

    Globals:    none

    Returns:    pointer to a merged array, NULL on error

*/
int iSrchPostingMergeSrchPostingsListsNEAR
(
    struct srchPostingsList *psplSrchPostingsList1,
    struct srchPostingsList *psplSrchPostingsList2,
    int iTermDistance,
    boolean bTermOrderMatters,
    unsigned int uiSrchPostingBooleanOperationID,
    struct srchPostingsList **ppsplSrchPostingsList
)
{

    int                         iError = SRCH_NoError;
    unsigned int                uiSrchPostingsLength = 0;
    struct srchPostingsList     *psplSrchPostingsList = NULL;
    struct srchPosting          *pspSrchPostingsPtr1 = NULL;
    struct srchPosting          *pspSrchPostingsPtr2 = NULL;
    struct srchPosting          *pspSrchPostingsEnd1 = NULL;
    struct srchPosting          *pspSrchPostingsEnd2 = NULL;
    struct srchPosting          *pspSrchPostingsPtr = NULL;
    struct srchPosting          *pspSrchPostingsEnd = NULL;
    unsigned int                uiCurrentDocumentID = 0;
    int                         iPositiveTermDistance = abs(iTermDistance);


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iSrchPostingMergeSrchPostingsListsNEAR - iTermDistance: %d, bTermOrderMatters: %s",  */
/*             iTermDistance, (bTermOrderMatters == true) ? "true" : "false"); */
/*     iUtlLogDebug(UTL_LOG_CONTEXT, "psplSrchPostingsList1"); */
/*     iSrchPostingPrintSrchPostingsList(psplSrchPostingsList1); */
/*     iUtlLogDebug(UTL_LOG_CONTEXT, "psplSrchPostingsList2"); */
/*     iSrchPostingPrintSrchPostingsList(psplSrchPostingsList2); */


    /* Check the parameters */
    if ( (psplSrchPostingsList1 != NULL) && (iSrchPostingCheckSrchPostingsList(psplSrchPostingsList1) != SRCH_NoError) ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'psplSrchPostingsList1' parameter passed to 'iSrchPostingMergeSrchPostingsListsNEAR'."); 
        return (SRCH_PostingInvalidPostingsList);
    }

    if ( (psplSrchPostingsList2 != NULL) && (iSrchPostingCheckSrchPostingsList(psplSrchPostingsList2) != SRCH_NoError) ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'psplSrchPostingsList2' parameter passed to 'iSrchPostingMergeSrchPostingsListsNEAR'."); 
        return (SRCH_PostingInvalidPostingsList);
    }

    if ( !((uiSrchPostingBooleanOperationID == SRCH_POSTING_BOOLEAN_OPERATION_RELAXED_ID) || (uiSrchPostingBooleanOperationID == SRCH_POSTING_BOOLEAN_OPERATION_STRICT_ID)) ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiSrchPostingBooleanOperationID' parameter passed to 'iSrchPostingMergeSrchPostingsListsNEAR'."); 
        return (SRCH_PostingInvalidSearchBooleanModifier);
    }

    if ( ppsplSrchPostingsList == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppsplSrchPostingsList' parameter passed to 'iSrchPostingMergeSrchPostingsListsNEAR'."); 
        return (SRCH_ReturnParameterError);
    }


    /* If either of the postings lists are NULL or of zero length we try to return the
    ** other one, if they are both null then we just return an empty search postings list structure
    */
    if ( (psplSrchPostingsList1 == NULL) || (psplSrchPostingsList1->uiSrchPostingsLength == 0) || 
            (psplSrchPostingsList2 == NULL) || (psplSrchPostingsList2->uiSrchPostingsLength == 0) ) {

        /* Return the non-empty set if we are using strict booleans and the non-empty one contains a stop term, 
        ** or if we are using relaxed booleans and the non-empty one contains a non-required term
        */ 
        if ( (psplSrchPostingsList1 != NULL) && (psplSrchPostingsList2 != NULL) ) {

            if ( (psplSrchPostingsList1->uiSrchPostingsLength > 0) &&
                    ( ((uiSrchPostingBooleanOperationID == SRCH_POSTING_BOOLEAN_OPERATION_STRICT_ID) && (psplSrchPostingsList2->uiTermType == SPI_TERM_TYPE_STOP)) ||
                    ((uiSrchPostingBooleanOperationID == SRCH_POSTING_BOOLEAN_OPERATION_RELAXED_ID) && (psplSrchPostingsList2->bRequired == false)) ) ) {
                
                iSrchPostingFreeSrchPostingsList(psplSrchPostingsList2);
                psplSrchPostingsList2 = NULL;

                *ppsplSrchPostingsList = psplSrchPostingsList1;
                return (SRCH_NoError);
            }
            else if ( (psplSrchPostingsList2->uiSrchPostingsLength > 0) &&
                    ( ((uiSrchPostingBooleanOperationID == SRCH_POSTING_BOOLEAN_OPERATION_STRICT_ID) && (psplSrchPostingsList1->uiTermType == SPI_TERM_TYPE_STOP)) ||
                    ((uiSrchPostingBooleanOperationID == SRCH_POSTING_BOOLEAN_OPERATION_RELAXED_ID) && (psplSrchPostingsList1->bRequired == false)) ) ) {
    
                iSrchPostingFreeSrchPostingsList(psplSrchPostingsList1);
                psplSrchPostingsList1 = NULL;

                *ppsplSrchPostingsList = psplSrchPostingsList2;
                return (SRCH_NoError);
            }
        }
        /* Return the non-NULL set if we are using relaxed booleans */
        else if ( uiSrchPostingBooleanOperationID == SRCH_POSTING_BOOLEAN_OPERATION_RELAXED_ID ) {
            
            if ( (psplSrchPostingsList1 == NULL) && (psplSrchPostingsList2 != NULL)  ) {
                *ppsplSrchPostingsList = psplSrchPostingsList2;
                return (SRCH_NoError);
            }
            else if ( (psplSrchPostingsList2 == NULL) && (psplSrchPostingsList1 != NULL) ) {
                *ppsplSrchPostingsList = psplSrchPostingsList1;
                return (SRCH_NoError);
            }
        }


        /* Return an empty postings list */
        {
            int     uiTermType = SPI_TERM_TYPE_UNKNOWN;
    
            /* Set the term type to stop word is both passed term types were stop words */
            if ( (psplSrchPostingsList1 != NULL) && (psplSrchPostingsList1->uiTermType == SPI_TERM_TYPE_STOP) && 
                    (psplSrchPostingsList2 != NULL) && (psplSrchPostingsList2->uiTermType == SPI_TERM_TYPE_STOP) ) {
                uiTermType = SPI_TERM_TYPE_STOP;
            }
    
            /* Create an empty postings list */
            if ( (iError = iSrchPostingCreateSrchPostingsList(uiTermType, SPI_TERM_COUNT_UNKNOWN, SPI_TERM_DOCUMENT_COUNT_UNKNOWN, 
                    false, NULL, 0, &psplSrchPostingsList)) != SRCH_NoError ) {
                return (iError);
            }
    
            iSrchPostingFreeSrchPostingsList(psplSrchPostingsList1);
            psplSrchPostingsList1 = NULL;
    
            iSrchPostingFreeSrchPostingsList(psplSrchPostingsList2);
            psplSrchPostingsList2 = NULL;
    
            *ppsplSrchPostingsList = psplSrchPostingsList;
            return (SRCH_NoError);
        }
    }


    ASSERT(psplSrchPostingsList1->uiSrchPostingsLength > 0);
    ASSERT(psplSrchPostingsList2->uiSrchPostingsLength > 0);


    /* Set up our variables */
    pspSrchPostingsPtr1 = psplSrchPostingsList1->pspSrchPostings;
    pspSrchPostingsPtr2 = psplSrchPostingsList2->pspSrchPostings;
    pspSrchPostingsEnd1 = psplSrchPostingsList1->pspSrchPostings + psplSrchPostingsList1->uiSrchPostingsLength;
    pspSrchPostingsEnd2 = psplSrchPostingsList2->pspSrchPostings + psplSrchPostingsList2->uiSrchPostingsLength;


    /* Create a new search postings list structure */
    if ( (iError = iSrchPostingCreateSrchPostingsList(SPI_TERM_TYPE_REGULAR, 0, 0, 
            (((psplSrchPostingsList1->bRequired == true) || (psplSrchPostingsList2->bRequired == true)) ? true : false), 
            NULL, UTL_MACROS_MAX(psplSrchPostingsList1->uiSrchPostingsLength, psplSrchPostingsList2->uiSrchPostingsLength), &psplSrchPostingsList)) != SRCH_NoError ) {
        return (iError);
    }


    /* Loop on the inner postings array */
    for ( pspSrchPostingsPtr = psplSrchPostingsList->pspSrchPostings; pspSrchPostingsPtr2 < pspSrchPostingsEnd2; pspSrchPostingsPtr2++ ) {

/*         iUtlLogDebug(UTL_LOG_CONTEXT, "uiDocumentID2 [%u], uiTermPosition [%d] fWeight [%.4f] (tracking/2)",  */
/*                 pspSrchPostingsPtr2->uiDocumentID, pspSrchPostingsPtr2->uiTermPosition, pspSrchPostingsPtr2->fWeight); */

        /* Loop through the outer postings array */
        for ( ; (pspSrchPostingsPtr1 < pspSrchPostingsEnd1) && (pspSrchPostingsPtr2->uiDocumentID >= pspSrchPostingsPtr1->uiDocumentID); pspSrchPostingsPtr1++ ) {

/*             iUtlLogDebug(UTL_LOG_CONTEXT, "uiDocumentID1 [%u], uiTermPosition [%d] fWeight [%.4f] (tracking/1)",  */
/*                     pspSrchPostingsPtr1->uiDocumentID, pspSrchPostingsPtr1->uiTermPosition, pspSrchPostingsPtr1->fWeight); */

            /* Skip terms positions of zero since those are meta-terms with no location */
            if ( pspSrchPostingsPtr1->uiDocumentID == pspSrchPostingsPtr2->uiDocumentID ) {

                /* Work out the term position delta, hoisted here for speed */
                int iTermPositionDelta = pspSrchPostingsPtr2->uiTermPosition - pspSrchPostingsPtr1->uiTermPosition;
                int iPositiveTermPositionDelta = abs(iTermPositionDelta);

                /* Look for A B adjacency */
                if ( (iTermPositionDelta >= 0) && (iTermPositionDelta <= iPositiveTermDistance) && 
                        (((bTermOrderMatters == true) && (iTermDistance >= 0)) || (bTermOrderMatters == false)) &&
                        (pspSrchPostingsPtr1->uiTermPosition != 0) && (pspSrchPostingsPtr2->uiTermPosition != 0) ) {

                    SRCH_POSTING_COPY_SRCH_POSTING(pspSrchPostingsPtr, pspSrchPostingsPtr2);
                    pspSrchPostingsPtr->fWeight += pspSrchPostingsPtr1->fWeight;

#if defined(SRCH_POSTING_ENABLE_PROXIMITY_REWEIGHTING)
                    pspSrchPostingsPtr->fWeight *= SRCH_POSTING_PROXIMITY_REWEIGHTING;
#endif    /* defined(SRCH_POSTING_ENABLE_PROXIMITY_REWEIGHTING) */

                    pspSrchPostingsPtr++;

/*                     iUtlLogDebug(UTL_LOG_CONTEXT, "uiDocumentID2 [%u], uiTermPosition [%d] fWeight [%.4f] (added/1)",  */
/*                             (pspSrchPostingsPtr - 1)->uiDocumentID, (pspSrchPostingsPtr - 1)->uiTermPosition, (pspSrchPostingsPtr - 1)->fWeight); */
                }
                /* Look for B A adjacency */
                else if ( (iTermPositionDelta < 0) && (iPositiveTermPositionDelta <= iPositiveTermDistance) && 
                        (((bTermOrderMatters == true) && (iTermDistance <= 0)) || (bTermOrderMatters == false)) &&
                        (pspSrchPostingsPtr1->uiTermPosition != 0) && (pspSrchPostingsPtr2->uiTermPosition != 0) ) {

                    SRCH_POSTING_COPY_SRCH_POSTING(pspSrchPostingsPtr, pspSrchPostingsPtr1);
                    pspSrchPostingsPtr->fWeight += pspSrchPostingsPtr2->fWeight;

#if defined(SRCH_POSTING_ENABLE_PROXIMITY_REWEIGHTING)
                    pspSrchPostingsPtr->fWeight *= SRCH_POSTING_PROXIMITY_REWEIGHTING;
#endif    /* defined(SRCH_POSTING_ENABLE_PROXIMITY_REWEIGHTING) */

                    pspSrchPostingsPtr++;

/*                     iUtlLogDebug(UTL_LOG_CONTEXT, "uiDocumentID1 [%u], uiTermPosition [%d] fWeight [%.4f] (added/2)",  */
/*                             (pspSrchPostingsPtr - 1)->uiDocumentID, (pspSrchPostingsPtr - 1)->uiTermPosition, (pspSrchPostingsPtr - 1)->fWeight); */
                }
                else if ( iTermPositionDelta < 0 ) {
                    break;
                }
            }
        } 
    }


    /* Get the size of the new postings array */
    uiSrchPostingsLength = pspSrchPostingsPtr - psplSrchPostingsList->pspSrchPostings;
    
    /* Set the term count and postings length */
    psplSrchPostingsList->uiTermCount = uiSrchPostingsLength;
    psplSrchPostingsList->uiSrchPostingsLength = uiSrchPostingsLength;

    /* Adjust the size of the new postings array */
    if ( uiSrchPostingsLength < psplSrchPostingsList->uiSrchPostingsLength ) {

        if ( uiSrchPostingsLength > 0 ) {

            if ( (pspSrchPostingsPtr = (struct srchPosting *)s_realloc(psplSrchPostingsList->pspSrchPostings, (size_t)(uiSrchPostingsLength * sizeof(struct srchPosting)))) == NULL ) {
                iSrchPostingFreeSrchPostingsList(psplSrchPostingsList);
                psplSrchPostingsList = NULL;
                return (SRCH_MemError);
            }

            psplSrchPostingsList->pspSrchPostings = pspSrchPostingsPtr;
        }
        else {
            s_free(psplSrchPostingsList->pspSrchPostings);
        }
    }


    /* Count up the number of documents in this posting */
    for ( pspSrchPostingsPtr = psplSrchPostingsList->pspSrchPostings, pspSrchPostingsEnd = psplSrchPostingsList->pspSrchPostings + psplSrchPostingsList->uiSrchPostingsLength, 
            psplSrchPostingsList->uiDocumentCount = 0, uiCurrentDocumentID = 0; pspSrchPostingsPtr < pspSrchPostingsEnd; pspSrchPostingsPtr++ ) {

        if ( pspSrchPostingsPtr->uiDocumentID != uiCurrentDocumentID ) {
            psplSrchPostingsList->uiDocumentCount++;
            uiCurrentDocumentID = pspSrchPostingsPtr->uiDocumentID;
        }
    }


    /* Free the two old postings lists */
    iSrchPostingFreeSrchPostingsList(psplSrchPostingsList1);
    psplSrchPostingsList1 = NULL;

    iSrchPostingFreeSrchPostingsList(psplSrchPostingsList2);
    psplSrchPostingsList2 = NULL;


    ASSERT(iSrchPostingCheckSrchPostingsList(psplSrchPostingsList) == SRCH_NoError);

/*     iUtlLogDebug(UTL_LOG_CONTEXT, "psplSrchPostingsList"); */
/*     iSrchPostingPrintSrchPostingsList(psplSrchPostingsList); */


    /* Set the return pointer */
    *ppsplSrchPostingsList = psplSrchPostingsList;

    /* Return  */
    return (SRCH_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchPostingSortDocumentIDAsc()

    Purpose:    This functions sorts a postings array in ascending
                document ID order, it implements a quicksort algorithm for 
                speed. The reason we use this guy rather than qsort() is
                for sheer speed, we get a 2:1 to 3:1 speed increase because
                the condition is inline.

    Parameters: pspSrchPostings                 pointer to a postings array
                iSrchSearchPostingsLeftIndex    left hand index in the postings structure array to sort 
                iSrchSearchPostingsRightIndex   right hand index in the postings structure array to sort 

    Globals:    none

    Returns:    SRCH error code

*/
int iSrchPostingSortDocumentIDAsc
(
    struct srchPosting *pspSrchPostings,
    int iSrchSearchPostingsLeftIndex,
    int iSrchSearchPostingsRightIndex
)
{

    int                     iSrchSearchPostingsLocalLeftIndex = 0;
    int                     iSrchSearchPostingsLocalRightIndex = 0;
    struct srchPosting      *ppPostingsRightOndexPtr = pspSrchPostings + iSrchSearchPostingsRightIndex;


    /* Check the parameters */
    if ( pspSrchPostings == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pspSrchPostings' parameter passed to 'iSrchPostingSortDocumentIDAsc'."); 
        return (SRCH_PostingInvalidPostings);
    }


    if ( iSrchSearchPostingsRightIndex > iSrchSearchPostingsLeftIndex ) {
    
        iSrchSearchPostingsLocalLeftIndex = iSrchSearchPostingsLeftIndex - 1; 
        iSrchSearchPostingsLocalRightIndex = iSrchSearchPostingsRightIndex;

        while ( true ) {

            while ((++iSrchSearchPostingsLocalLeftIndex <= iSrchSearchPostingsRightIndex) && 
                    (((pspSrchPostings + iSrchSearchPostingsLocalLeftIndex)->uiDocumentID < ppPostingsRightOndexPtr->uiDocumentID) || 
                    (((pspSrchPostings + iSrchSearchPostingsLocalLeftIndex)->uiDocumentID == ppPostingsRightOndexPtr->uiDocumentID) && 
                    ((pspSrchPostings + iSrchSearchPostingsLocalLeftIndex)->uiTermPosition < ppPostingsRightOndexPtr->uiTermPosition))));

            while ((--iSrchSearchPostingsLocalRightIndex > iSrchSearchPostingsLeftIndex) && 
                    (((pspSrchPostings + iSrchSearchPostingsLocalRightIndex)->uiDocumentID > ppPostingsRightOndexPtr->uiDocumentID) ||
                    (((pspSrchPostings + iSrchSearchPostingsLocalRightIndex)->uiDocumentID == ppPostingsRightOndexPtr->uiDocumentID) && 
                    ((pspSrchPostings + iSrchSearchPostingsLocalRightIndex)->uiTermPosition > ppPostingsRightOndexPtr->uiTermPosition))));

            if ( iSrchSearchPostingsLocalLeftIndex >= iSrchSearchPostingsLocalRightIndex ) {
                break;
            }
            
            SRCH_POSTING_SWAP_SRCH_POSTING((pspSrchPostings + iSrchSearchPostingsLocalRightIndex), (pspSrchPostings + iSrchSearchPostingsLocalLeftIndex));
        }

        if ( iSrchSearchPostingsLocalLeftIndex != iSrchSearchPostingsRightIndex ) {
            SRCH_POSTING_SWAP_SRCH_POSTING(ppPostingsRightOndexPtr, (pspSrchPostings + iSrchSearchPostingsLocalLeftIndex));
        }

        iSrchPostingSortDocumentIDAsc(pspSrchPostings, iSrchSearchPostingsLeftIndex, iSrchSearchPostingsLocalLeftIndex - 1);
        iSrchPostingSortDocumentIDAsc(pspSrchPostings, iSrchSearchPostingsLocalLeftIndex + 1, iSrchSearchPostingsRightIndex);
    }


    return (SRCH_NoError);

}


/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchPostingPrintSrchPostingsList()

    Purpose:    This function prints out the contents of the search postings list structure.
                This is used for debuging purposed only.

    Parameters: psplSrchPostingsList    search postings list structure

    Globals:    none

    Returns:    SRCH error code

*/
static int iSrchPostingPrintSrchPostingsList
(
    struct srchPostingsList *psplSrchPostingsList
)
{

    struct srchPosting      *pspSrchPostingsPtr = NULL;
    struct srchPosting      *pspSrchPostingsEnd = NULL;


    /* Check input variables */
    if ( psplSrchPostingsList == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'psplSrchPostingsList' parameter passed to 'iSrchPostingPrintSrchPostingsList'."); 
        return (SRCH_PostingInvalidPostingsList);
    }

    printf("iSrchPostingPrintSrchPostingsList - uiTermCount [%u], uiDocumentCount [%u], uiTermType [%u], uiSrchPostingsLength [%u], bRequired [%s]\n", 
            psplSrchPostingsList->uiTermCount, psplSrchPostingsList->uiDocumentCount, psplSrchPostingsList->uiTermType, psplSrchPostingsList->uiSrchPostingsLength,
            (psplSrchPostingsList->bRequired == true) ? "true" : "false");

    /* Print out the postings array */
    for ( pspSrchPostingsPtr = psplSrchPostingsList->pspSrchPostings, pspSrchPostingsEnd = psplSrchPostingsList->pspSrchPostings + psplSrchPostingsList->uiSrchPostingsLength; 
            pspSrchPostingsPtr < pspSrchPostingsEnd; pspSrchPostingsPtr++ ) {

        printf("  uiDocumentID [%u], uiTermPosition [%d], fWeight [%.4f]\n", 
                pspSrchPostingsPtr->uiDocumentID, pspSrchPostingsPtr->uiTermPosition, pspSrchPostingsPtr->fWeight);
    }


    return (SRCH_NoError);

}


/*---------------------------------------------------------------------------*/

