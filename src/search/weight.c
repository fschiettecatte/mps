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

    Module:     weight.c

    Author:     Francois Schiettecatte (FS Consulting LLC.)

    Created:    21 January 2006

    Purpose:    This module provides support functions for search.c for 
                    processing weights 

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
#define UTL_LOG_CONTEXT                     (unsigned char *)"com.fsconsult.mps.src.search.weight"


/* Enable co-occurence reweighting */
/* #define SRCH_WEIGHT_ENABLE_CO_OCCURENCE_REWEIGHTING */


/*---------------------------------------------------------------------------*/


/*
** Defines
*/

/* Weighting increase for term co-occurrence in a document  */
#if defined(SRCH_WEIGHT_ENABLE_CO_OCCURENCE_REWEIGHTING)
#define SRCH_WEIGHT_TERM_WEIGHT_CO_OCCURRENCE                            (1.2)
#endif /* defined(SRCH_WEIGHT_ENABLE_CO_OCCURENCE_REWEIGHTING) */


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchWeightCreate()

    Purpose:    This function creates a search weight structure.

    Parameters: pfWeights               weights (optional)
                uiWeightsLength         weights length
                bMappedAllocationFlag   mapped allocation flag
                ppswSrchWeight          search weights structure return pointer

    Globals:    none

    Returns:    SRCH error code

*/
int iSrchWeightCreate
(
    float *pfWeights,
    unsigned int uiWeightsLength,
    boolean bMappedAllocationFlag,
    struct srchWeight **ppswSrchWeight
)
{

    struct srchWeight   *pswSrchWeight = NULL;


    /* Check the parameters */
    if ( ppswSrchWeight == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppswSrchWeight' parameter passed to 'iSrchWeightCreate'."); 
        return (SPI_ReturnParameterError);
    }


    /* Allocate the search weights structure */
    if ( (pswSrchWeight = (struct srchWeight *)s_malloc(sizeof(struct srchWeight))) == NULL ) {
        return (SRCH_MemError);
    }
    
    /* Different path depending whether existing weights were passed or not */
    if ( pfWeights != NULL ) {

        /* Existing weights, so we just had over the values/pointers */
        pswSrchWeight->uiWeightsLength = uiWeightsLength;
        pswSrchWeight->bMappedAllocationFlag = bMappedAllocationFlag;
        pswSrchWeight->pfWeights = pfWeights;
    }
    else {

        /* New weights so we allocate a new bitmap */ 
        pswSrchWeight->uiWeightsLength = uiWeightsLength;
        pswSrchWeight->bMappedAllocationFlag = false;
        
        /* Allocate a new weights array */
        if ( (pswSrchWeight->pfWeights = (float *)s_malloc((size_t)(pswSrchWeight->uiWeightsLength * sizeof(float)))) == NULL ) {
            s_free(pswSrchWeight);
            return (SRCH_MemError);
        }
    }


    /* Set the return pointer */
    *ppswSrchWeight = pswSrchWeight;


    return (SRCH_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchWeightFree()

    Purpose:    This function frees the search weight structure.

    Parameters: pswSrchWeight   search weight structure

    Globals:    none

    Returns:    SRCH error code

*/
int iSrchWeightFree
(
    struct srchWeight *pswSrchWeight
)
{

    /* Free the search weight structure */
    if ( pswSrchWeight != NULL ) {
    
        if ( pswSrchWeight->pfWeights != NULL ) {
    
            if ( pswSrchWeight->bMappedAllocationFlag == true ) {
                iUtlFileMemoryUnMap(pswSrchWeight->pfWeights, pswSrchWeight->uiWeightsLength * sizeof(float));
                pswSrchWeight->pfWeights = NULL;
            }
            else {
                s_free(pswSrchWeight->pfWeights);
            }
        }

        s_free(pswSrchWeight);
    }


    return (SRCH_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchWeightMergeXOR()

    Purpose:    This function XORs pswSrchWeight1 and pswSrchWeight2
                and return a pointer to the new search weight structure.
                It disposes of the two passed search weight structures 
                except if an error occurs.

    Parameters: pswSrchWeight1      search weight structure
                pswSrchWeight2      search weight structure
                ppswSrchWeight      return pointer for the new search weight structure

    Globals:    none

    Returns:    SRCH error code

*/
int iSrchWeightMergeXOR
(
    struct srchWeight *pswSrchWeight1,
    struct srchWeight *pswSrchWeight2,
    struct srchWeight **ppswSrchWeight
)
{
    
    int                 iError = SRCH_NoError;

    struct srchWeight   *pswSrchWeight = NULL;

    float               *pfWeightsPtr = NULL;
    float               *pfWeights1Ptr = NULL;
    float               *pfWeights2Ptr = NULL;
    float               *pfWeightsEnd = NULL;


    /* Check the parameters */
    if ( pswSrchWeight1 == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pswSrchWeight1' parameter passed to 'iSrchWeightMergeXOR'."); 
        return (SRCH_WeightInvalidWeight);
    }

    if ( pswSrchWeight2 == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pswSrchWeight2' parameter passed to 'iSrchWeightMergeXOR'."); 
        return (SRCH_WeightInvalidWeight);
    }


    if ( pswSrchWeight1->pfWeights == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pswSrchWeight1->pfWeights' parameter passed to 'iSrchWeightMergeXOR'."); 
        return (SRCH_WeightInvalidWeight);
    }

    if ( pswSrchWeight2->pfWeights == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pswSrchWeight2->pfWeights' parameter passed to 'iSrchWeightMergeXOR'."); 
        return (SRCH_WeightInvalidWeight);
    }

    if ( pswSrchWeight1->uiWeightsLength != pswSrchWeight2->uiWeightsLength ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Unequal 'pswSrchWeight1->uiWeightsLength' & 'pswSrchWeight2->uiWeightsLength' parameters passed to 'iSrchWeightMergeXOR'."); 
        return (SRCH_WeightInvalidWeight);
    }

    
    /* Reuse one or the other search weight structure if we can, otherwise we allocate a new one */
    if ( pswSrchWeight1->bMappedAllocationFlag == false ) {
        pswSrchWeight = pswSrchWeight1;
    }
    else if ( pswSrchWeight2->bMappedAllocationFlag == false ) {
        pswSrchWeight = pswSrchWeight2;
    }
    else {

        /* Allocate a new search weights structure */
        if ( (iError = iSrchWeightCreate(NULL, pswSrchWeight1->uiWeightsLength, false, &pswSrchWeight)) != SRCH_NoError ) {
            return (iError);
        }
    }


    /* XOR the two search weight arrays */
    for ( pfWeightsPtr = pswSrchWeight->pfWeights, pfWeights1Ptr = pswSrchWeight1->pfWeights, pfWeights2Ptr = pswSrchWeight2->pfWeights, 
            pfWeightsEnd = (pswSrchWeight->pfWeights + pswSrchWeight->uiWeightsLength); pfWeightsPtr < pfWeightsEnd; pfWeightsPtr++, pfWeights1Ptr++, pfWeights2Ptr++ ) {

#if defined(SRCH_WEIGHT_ENABLE_CO_OCCURENCE_REWEIGHTING)

        *pfWeightsPtr = (*pfWeights1Ptr > 0) ? (*pfWeights1Ptr + (*pfWeights2Ptr * SRCH_WEIGHT_TERM_WEIGHT_CO_OCCURRENCE)) : 0;

#else

        *pfWeightsPtr = (*pfWeights1Ptr > 0) ? (*pfWeights1Ptr + *pfWeights2Ptr) : 0;

#endif /* defined(SRCH_WEIGHT_ENABLE_CO_OCCURENCE_REWEIGHTING) */

    }

    
    /* Free the search weight structures */
    if ( pswSrchWeight != pswSrchWeight1 ) {
        iSrchWeightFree(pswSrchWeight1);
        pswSrchWeight1 = NULL;
    }
    
    if ( pswSrchWeight != pswSrchWeight2 ) {
        iSrchWeightFree(pswSrchWeight2);
        pswSrchWeight2 = NULL;
    }
    
    
    /* Set the return pointer */
    *ppswSrchWeight = pswSrchWeight;

    
    return (SRCH_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchWeightMergeOR()

    Purpose:    This function XORs pswSrchWeight1 and pswSrchWeight2
                and return a pointer to the new search weight structure.
                It disposes of the two passed search weight structures 
                except if an error occurs.

    Parameters: pswSrchWeight1      search weight structure
                pswSrchWeight2      search weight structure
                ppswSrchWeight      return pointer for the new search weight structure

    Globals:    none

    Returns:    SRCH error code

*/
int iSrchWeightMergeOR
(
    struct srchWeight *pswSrchWeight1,
    struct srchWeight *pswSrchWeight2,
    struct srchWeight **ppswSrchWeight
)
{

    int                 iError = SRCH_NoError;

    struct srchWeight   *pswSrchWeight = NULL;

    float               *pfWeightsPtr = NULL;
    float               *pfWeights1Ptr = NULL;
    float               *pfWeights2Ptr = NULL;
    float               *pfWeightsEnd = NULL;


    /* Check the parameters */
    if ( pswSrchWeight1 == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pswSrchWeight1' parameter passed to 'iSrchWeightMergeOR'."); 
        return (SRCH_WeightInvalidWeight);
    }

    if ( pswSrchWeight2 == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pswSrchWeight2' parameter passed to 'iSrchWeightMergeOR'."); 
        return (SRCH_WeightInvalidWeight);
    }


    if ( pswSrchWeight1->pfWeights == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pswSrchWeight1->pfWeights' parameter passed to 'iSrchWeightMergeOR'."); 
        return (SRCH_WeightInvalidWeight);
    }

    if ( pswSrchWeight2->pfWeights == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pswSrchWeight2->pfWeights' parameter passed to 'iSrchWeightMergeOR'."); 
        return (SRCH_WeightInvalidWeight);
    }

    if ( pswSrchWeight1->uiWeightsLength != pswSrchWeight2->uiWeightsLength ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Unequal 'pswSrchWeight1->uiWeightsLength' & 'pswSrchWeight2->uiWeightsLength' parameters passed to 'iSrchWeightMergeOR'."); 
        return (SRCH_WeightInvalidWeight);
    }

    
    /* Reuse one or the other search weight structure if we can, otherwise we allocate a new one */
    if ( pswSrchWeight1->bMappedAllocationFlag == false ) {
        pswSrchWeight = pswSrchWeight1;
    }
    else if ( pswSrchWeight2->bMappedAllocationFlag == false ) {
        pswSrchWeight = pswSrchWeight2;
    }
    else {

        /* Allocate a new search weights structure */
        if ( (iError = iSrchWeightCreate(NULL, pswSrchWeight1->uiWeightsLength, false, &pswSrchWeight)) != SRCH_NoError ) {
            return (iError);
        }
    }


    /* OR the two search weight arrays */
    for ( pfWeightsPtr = pswSrchWeight->pfWeights, pfWeights1Ptr = pswSrchWeight1->pfWeights, pfWeights2Ptr = pswSrchWeight2->pfWeights, 
            pfWeightsEnd = (pswSrchWeight->pfWeights + pswSrchWeight->uiWeightsLength); pfWeightsPtr < pfWeightsEnd; pfWeightsPtr++, pfWeights1Ptr++, pfWeights2Ptr++ ) {

#if defined(SRCH_WEIGHT_ENABLE_CO_OCCURENCE_REWEIGHTING)

        *pfWeightsPtr = (*pfWeights1Ptr > 0) ? ((*pfWeights1Ptr + *pfWeights2Ptr) * SRCH_WEIGHT_TERM_WEIGHT_CO_OCCURRENCE) : (*pfWeights1Ptr + *pfWeights2Ptr);

#else

        *pfWeightsPtr = *pfWeights1Ptr + *pfWeights2Ptr;

#endif /* defined(SRCH_WEIGHT_ENABLE_CO_OCCURENCE_REWEIGHTING) */

    }

    
    /* Free the search weight structures */
    if ( pswSrchWeight != pswSrchWeight1 ) {
        iSrchWeightFree(pswSrchWeight1);
        pswSrchWeight1 = NULL;
    }
    
    if ( pswSrchWeight != pswSrchWeight2 ) {
        iSrchWeightFree(pswSrchWeight2);
        pswSrchWeight2 = NULL;
    }
    
    
    /* Set the return pointer */
    *ppswSrchWeight = pswSrchWeight;

    
    return (SRCH_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchWeightMergeAND()

    Purpose:    This function XORs pswSrchWeight1 and pswSrchWeight2
                and return a pointer to the new search weight structure.
                It disposes of the two passed search weight structures 
                except if an error occurs.

    Parameters: pswSrchWeight1      search weight structure
                pswSrchWeight2      search weight structure
                ppswSrchWeight      return pointer for the new search weight structure

    Globals:    none

    Returns:    SRCH error code

*/
int iSrchWeightMergeAND
(
    struct srchWeight *pswSrchWeight1,
    struct srchWeight *pswSrchWeight2,
    struct srchWeight **ppswSrchWeight
)
{
    
    int                 iError = SRCH_NoError;

    struct srchWeight   *pswSrchWeight = NULL;

    float               *pfWeightsPtr = NULL;
    float               *pfWeights1Ptr = NULL;
    float               *pfWeights2Ptr = NULL;
    float               *pfWeightsEnd = NULL;


    /* Check the parameters */
    if ( pswSrchWeight1 == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pswSrchWeight1' parameter passed to 'iSrchWeightMergeAND'."); 
        return (SRCH_WeightInvalidWeight);
    }

    if ( pswSrchWeight2 == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pswSrchWeight2' parameter passed to 'iSrchWeightMergeAND'."); 
        return (SRCH_WeightInvalidWeight);
    }


    if ( pswSrchWeight1->pfWeights == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pswSrchWeight1->pfWeights' parameter passed to 'iSrchWeightMergeAND'."); 
        return (SRCH_WeightInvalidWeight);
    }

    if ( pswSrchWeight2->pfWeights == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pswSrchWeight2->pfWeights' parameter passed to 'iSrchWeightMergeAND'."); 
        return (SRCH_WeightInvalidWeight);
    }

    if ( pswSrchWeight1->uiWeightsLength != pswSrchWeight2->uiWeightsLength ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Unequal 'pswSrchWeight1->uiWeightsLength' & 'pswSrchWeight2->uiWeightsLength' parameters passed to 'iSrchWeightMergeAND'."); 
        return (SRCH_WeightInvalidWeight);
    }

    
    /* Reuse one or the other search weight structure if we can, otherwise we allocate a new one */
    if ( pswSrchWeight1->bMappedAllocationFlag == false ) {
        pswSrchWeight = pswSrchWeight1;
    }
    else if ( pswSrchWeight2->bMappedAllocationFlag == false ) {
        pswSrchWeight = pswSrchWeight2;
    }
    else {

        /* Allocate a new search weights structure */
        if ( (iError = iSrchWeightCreate(NULL, pswSrchWeight1->uiWeightsLength, false, &pswSrchWeight)) != SRCH_NoError ) {
            return (iError);
        }
    }


    /* AND the two search weight arrays */
    for ( pfWeightsPtr = pswSrchWeight->pfWeights, pfWeights1Ptr = pswSrchWeight1->pfWeights, pfWeights2Ptr = pswSrchWeight2->pfWeights, 
            pfWeightsEnd = (pswSrchWeight->pfWeights + pswSrchWeight->uiWeightsLength); pfWeightsPtr < pfWeightsEnd; pfWeightsPtr++, pfWeights1Ptr++, pfWeights2Ptr++ ) {

#if defined(SRCH_WEIGHT_ENABLE_CO_OCCURENCE_REWEIGHTING)

        *pfWeightsPtr = ((*pfWeights1Ptr > 0) && (*pfWeights2Ptr > 0)) ? ((*pfWeights1Ptr + *pfWeights2Ptr) * SRCH_WEIGHT_TERM_WEIGHT_CO_OCCURRENCE) : 0;

#else

        *pfWeightsPtr = ((*pfWeights1Ptr > 0) && (*pfWeights2Ptr > 0)) ? (*pfWeights1Ptr + *pfWeights2Ptr) : 0;

#endif /* defined(SRCH_WEIGHT_ENABLE_CO_OCCURENCE_REWEIGHTING) */

    }

    
    /* Free the search weight structures */
    if ( pswSrchWeight != pswSrchWeight1 ) {
        iSrchWeightFree(pswSrchWeight1);
        pswSrchWeight1 = NULL;
    }
    
    if ( pswSrchWeight != pswSrchWeight2 ) {
        iSrchWeightFree(pswSrchWeight2);
        pswSrchWeight2 = NULL;
    }
    
    
    /* Set the return pointer */
    *ppswSrchWeight = pswSrchWeight;

    
    return (SRCH_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchWeightMergeNOT()

    Purpose:    This function XORs pswSrchWeight1 and pswSrchWeight2
                and return a pointer to the new search weight structure.
                It disposes of the two passed search weight structures 
                except if an error occurs.

    Parameters: pswSrchWeight1      search weight structure
                pswSrchWeight2      search weight structure
                ppswSrchWeight      return pointer for the new search weight structure

    Globals:    none

    Returns:    SRCH error code

*/
int iSrchWeightMergeNOT
(
    struct srchWeight *pswSrchWeight1,
    struct srchWeight *pswSrchWeight2,
    struct srchWeight **ppswSrchWeight
)
{
    
    int                 iError = SRCH_NoError;

    struct srchWeight   *pswSrchWeight = NULL;

    float               *pfWeightsPtr = NULL;
    float               *pfWeights1Ptr = NULL;
    float               *pfWeights2Ptr = NULL;
    float               *pfWeightsEnd = NULL;


    /* Check the parameters */
    if ( pswSrchWeight1 == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pswSrchWeight1' parameter passed to 'iSrchWeightMergeNOT'."); 
        return (SRCH_WeightInvalidWeight);
    }

    if ( pswSrchWeight2 == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pswSrchWeight2' parameter passed to 'iSrchWeightMergeNOT'."); 
        return (SRCH_WeightInvalidWeight);
    }


    if ( pswSrchWeight1->pfWeights == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pswSrchWeight1->pfWeights' parameter passed to 'iSrchWeightMergeNOT'."); 
        return (SRCH_WeightInvalidWeight);
    }

    if ( pswSrchWeight2->pfWeights == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pswSrchWeight2->pfWeights' parameter passed to 'iSrchWeightMergeNOT'."); 
        return (SRCH_WeightInvalidWeight);
    }

    if ( pswSrchWeight1->uiWeightsLength != pswSrchWeight2->uiWeightsLength ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Unequal 'pswSrchWeight1->uiWeightsLength' & 'pswSrchWeight2->uiWeightsLength' parameters passed to 'iSrchWeightMergeNOT'."); 
        return (SRCH_WeightInvalidWeight);
    }

    
    /* Reuse one or the other search weight structure if we can, otherwise we allocate a new one */
    if ( pswSrchWeight1->bMappedAllocationFlag == false ) {
        pswSrchWeight = pswSrchWeight1;
    }
    else if ( pswSrchWeight2->bMappedAllocationFlag == false ) {
        pswSrchWeight = pswSrchWeight2;
    }
    else {

        /* Allocate a new search weights structure */
        if ( (iError = iSrchWeightCreate(NULL, pswSrchWeight1->uiWeightsLength, false, &pswSrchWeight)) != SRCH_NoError ) {
            return (iError);
        }
    }


    /* NOT the two search weight arrays */
    for ( pfWeightsPtr = pswSrchWeight->pfWeights, pfWeights1Ptr = pswSrchWeight1->pfWeights, pfWeights2Ptr = pswSrchWeight2->pfWeights, 
            pfWeightsEnd = (pswSrchWeight->pfWeights + pswSrchWeight->uiWeightsLength); pfWeightsPtr < pfWeightsEnd; pfWeightsPtr++, pfWeights1Ptr++, pfWeights2Ptr++ ) {

        *pfWeightsPtr = (*pfWeights2Ptr > 0) ? 0 : *pfWeights1Ptr;

    }

    
    /* Free the search weight structures */
    if ( pswSrchWeight != pswSrchWeight1 ) {
        iSrchWeightFree(pswSrchWeight1);
        pswSrchWeight1 = NULL;
    }
    
    if ( pswSrchWeight != pswSrchWeight2 ) {
        iSrchWeightFree(pswSrchWeight2);
        pswSrchWeight2 = NULL;
    }
    
    
    /* Set the return pointer */
    *ppswSrchWeight = pswSrchWeight;

    
    return (SRCH_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchWeightPrint()

    Purpose:    This function prints out the contents of the search weight structure.
                This is used for debugging purposed only.

    Parameters: pswSrchWeight   search weight structure

    Globals:    none

    Returns:    SRCH error code

*/
int iSrchWeightPrint
(
    struct srchWeight *pswSrchWeight
)
{

    /* Check input variables */
    if ( pswSrchWeight == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pswSrchWeight' parameter passed to 'iSrchWeightPrint'."); 
    }

    if ( pswSrchWeight->pfWeights == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pswSrchWeight->pfWeights' parameter passed to 'iSrchWeightPrint'."); 
    }

    if ( pswSrchWeight->uiWeightsLength == 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'pswSrchWeight->uiWeightsLength' parameter passed to 'iSrchWeightPrint'."); 
    }


    /* Print out the search weight structure */
    if ( (pswSrchWeight != NULL) && (pswSrchWeight->pfWeights != NULL) ) {
        
        unsigned int    uiI = 0;
        float           *pfWeightsPtr = NULL;

        for ( uiI = 0, pfWeightsPtr = pswSrchWeight->pfWeights; uiI < pswSrchWeight->uiWeightsLength; uiI++, pfWeightsPtr++ ) {
            printf("%8u - %9.4f\n", uiI, *pfWeightsPtr);
        }
    }


    return (SRCH_NoError);

}


/*---------------------------------------------------------------------------*/
