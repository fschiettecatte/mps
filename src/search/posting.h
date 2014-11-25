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

    Module:     posting.h

    Author:     Francois Schiettecatte (FS Consulting LLC.)

    Created:    21 January 2006

    Purpose:    This module provides support functions for search.c for 
                processing postings 

*/


#if !defined(SRCH_POSTING_H)
#define SRCH_POSTING_H


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

/* Posting boolean operation ID */
#define SRCH_POSTING_BOOLEAN_OPERATION_INVALID_ID       (0)
#define SRCH_POSTING_BOOLEAN_OPERATION_STRICT_ID        (1)
#define SRCH_POSTING_BOOLEAN_OPERATION_RELAXED_ID       (2)


/*---------------------------------------------------------------------------*/


/*
** Structures
*/

/* Search posting structure */
struct srchPosting {
    unsigned int            uiDocumentID;                   /* Document ID */    
    unsigned int            uiTermPosition;                 /* Term position */
    float                   fWeight;                        /* Document weight */
};    


/* Search postings list structure */
struct srchPostingsList {
    unsigned int            uiTermType;                     /* Term type */
    unsigned int            uiTermCount;                    /* Number of occurrences of this term - same as uiSrchPostingsLength */
    unsigned int            uiDocumentCount;                /* Number of documents in which this term occurs */
    boolean                 bRequired;                      /* Required flag */
    struct srchPosting      *pspSrchPostings;               /* Search postings array */
    unsigned int            uiSrchPostingsLength;           /* Search postings array length - same as uiTermCount */
};


/*---------------------------------------------------------------------------*/


/*
** Public function prototypes
*/

int iSrchPostingCreateSrchPostingsList (unsigned int uiTermType, unsigned int uiTermCount, 
        unsigned int uiDocumentCount, boolean bRequired, struct srchPosting *pspSrchPostings, 
        unsigned int uiSrchPostingsLength, struct srchPostingsList **ppsplSrchPostingsList);

int iSrchPostingFreeSrchPostingsList (struct srchPostingsList *psplSrchPostingsList);

int iSrchPostingCheckSrchPostingsList (struct srchPostingsList *psplSrchPostingsList);

int iSrchPostingSortDocumentIDAsc (struct srchPosting *pspSrchPostings, 
        int iSrchSearchPostingsLeftIndex, int iSrchSearchPostingsRightIndex);



int iSrchPostingMergeSrchPostingsListsOR (struct srchPostingsList *psplSrchPostingsList1, 
        struct srchPostingsList *psplSrchPostingsList2, unsigned int uiSrchPostingBooleanOperationID,
        struct srchPostingsList **ppsplSrchPostingsList);

int iSrchPostingMergeSrchPostingsListsIOR (struct srchPostingsList *psplSrchPostingsList1, 
        struct srchPostingsList *psplSrchPostingsList2, unsigned int uiSrchPostingBooleanOperationID,
        struct srchPostingsList **ppsplSrchPostingsList);

int iSrchPostingMergeSrchPostingsListsXOR (struct srchPostingsList *psplSrchPostingsList1, 
        struct srchPostingsList *psplSrchPostingsList2, unsigned int uiSrchPostingBooleanOperationID,
        struct srchPostingsList **ppsplSrchPostingsList);

int iSrchPostingMergeSrchPostingsListsAND (struct srchPostingsList *psplSrchPostingsList1, 
        struct srchPostingsList *psplSrchPostingsList2, unsigned int uiSrchPostingBooleanOperationID,
        struct srchPostingsList **ppsplSrchPostingsList);

int iSrchPostingMergeSrchPostingsListsNOT (struct srchPostingsList *psplSrchPostingsList1, 
        struct srchPostingsList *psplSrchPostingsList2, unsigned int uiSrchPostingBooleanOperationID,
        struct srchPostingsList **ppsplSrchPostingsList);

int iSrchPostingMergeSrchPostingsListsADJ (struct srchPostingsList *psplSrchPostingsList1, 
        struct srchPostingsList *psplSrchPostingsList2, int iTermDistance, 
        unsigned int uiSrchPostingBooleanOperationID, struct srchPostingsList **ppsplSrchPostingsList);

int iSrchPostingMergeSrchPostingsListsNEAR (struct srchPostingsList *psplSrchPostingsList1, 
        struct srchPostingsList *psplSrchPostingsList2, int iTermDistance, boolean bTermOrderMatters, 
        unsigned int uiSrchPostingBooleanOperationID, struct srchPostingsList **ppsplSrchPostingsList);


/*---------------------------------------------------------------------------*/


/* 
** C++ wrapper
*/

#if defined(__cplusplus)
}
#endif    /* defined(__cplusplus) */


/*---------------------------------------------------------------------------*/


#endif    /* !defined(SRCH_POSTING_H) */


/*---------------------------------------------------------------------------*/
