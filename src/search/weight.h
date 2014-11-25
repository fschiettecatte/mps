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

    Module:     weight.h

    Author:     Francois Schiettecatte (FS Consulting LLC.)

    Created:    21 January 2006

    Purpose:    This module provides support functions for search.c for 
                    processing weights 

*/


#if !defined(SRCH_WEIGHT_H)
#define SRCH_WEIGHT_H


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
** Structures
*/

/* Weight structure */
struct srchWeight {
    float           *pfWeights;                 /* Weights array */
    unsigned int    uiWeightsLength;            /* Weights array length (number of entries) */
    boolean         bMappedAllocationFlag;      /* Memory mapped allocation flag */
};


/*---------------------------------------------------------------------------*/


/*
** Public function prototypes
*/

int iSrchWeightCreate (float *pfWeights, unsigned int uiWeightsLength,
    boolean bMappedAllocationFlag, struct srchWeight **ppswSrchWeight);

int iSrchWeightFree (struct srchWeight *pswSrchWeight);


int iSrchWeightMergeXOR (struct srchWeight *pswSrchWeight1, 
        struct srchWeight *pswSrchWeight2, struct srchWeight **ppswSrchWeight);

int iSrchWeightMergeOR (struct srchWeight *pswSrchWeight1,
        struct srchWeight *pswSrchWeight2, struct srchWeight **ppswSrchWeight);

int iSrchWeightMergeAND (struct srchWeight *pswSrchWeight1, 
        struct srchWeight *pswSrchWeight2, struct srchWeight **ppswSrchWeight);

int iSrchWeightMergeNOT (struct srchWeight *pswSrchWeight1, 
        struct srchWeight *pswSrchWeight2, struct srchWeight **ppswSrchWeight);


int iSrchWeightPrint (struct srchWeight *pswSrchWeight);


/*---------------------------------------------------------------------------*/


/* 
** C++ wrapper
*/

#if defined(__cplusplus)
}
#endif    /* defined(__cplusplus) */


/*---------------------------------------------------------------------------*/


#endif    /* !defined(SRCH_WEIGHT_H) */


/*---------------------------------------------------------------------------*/
