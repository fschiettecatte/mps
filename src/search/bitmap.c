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

    Module:     bitmap.c

    Author:     Francois Schiettecatte (FS Consulting LLC.)

    Created:    21 January 2006

    Purpose:    This module provides support functions for search.c for 
                processing bitmaps 

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
#define UTL_LOG_CONTEXT                     (unsigned char *)"com.fsconsult.mps.src.search.bitmap"


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchBitmapCreate()

    Purpose:    This function creates a search bitmap structure.

    Parameters: pucBitmap               bitmap (optional)
                uiBitmapLength          bitmap length
                bMappedAllocationFlag   mapped allocation flag
                ppsbSrchBitmap          search bitmap structure return pointer

    Globals:    none

    Returns:    SRCH error code

*/
int iSrchBitmapCreate
(
    unsigned char *pucBitmap,
    unsigned int uiBitmapLength,
    boolean bMappedAllocationFlag,
    struct srchBitmap **ppsbSrchBitmap
)
{

    struct srchBitmap   *psbSrchBitmap = NULL;


    /* Check the parameters */
    if ( ppsbSrchBitmap == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppsbSrchBitmap' parameter passed to 'iSrchBitmapCreate'."); 
        return (SPI_ReturnParameterError);
    }


    /* Allocate the search bitmap structure */
    if ( (psbSrchBitmap = (struct srchBitmap *)s_malloc(sizeof(struct srchBitmap))) == NULL ) {
        return (SRCH_MemError);
    }
    
    /* Different path depending whether an existing bitmap was passed or not */
    if ( pucBitmap != NULL ) {

        /* Existing bitmap, so we just had over the values/pointers */
        psbSrchBitmap->uiBitmapLength = uiBitmapLength;
        psbSrchBitmap->bMappedAllocationFlag = bMappedAllocationFlag;
        psbSrchBitmap->pucBitmap = pucBitmap;
    }
    else {

        /* New bitmap so we allocate a new bitmap */ 
        psbSrchBitmap->uiBitmapLength = uiBitmapLength;
        psbSrchBitmap->bMappedAllocationFlag = false;
        
        if ( (psbSrchBitmap->pucBitmap = (unsigned char *)s_malloc((size_t)UTL_BITMAP_GET_BITMAP_BYTE_LENGTH(psbSrchBitmap->uiBitmapLength))) == NULL ) {
            s_free(psbSrchBitmap);
            return (SRCH_MemError);
        }
    }


    /* Set the return pointer */
    *ppsbSrchBitmap = psbSrchBitmap;


    return (SRCH_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchBitmapFree()

    Purpose:    This function frees the search bitmap structure.

    Parameters: psbSrchBitmap       search bitmap structure

    Globals:    none

    Returns:    SRCH error code

*/
int iSrchBitmapFree
(
    struct srchBitmap *psbSrchBitmap
)
{

    /* Free the search bitmap structure */
    if ( psbSrchBitmap != NULL ) {
    
        if ( psbSrchBitmap->pucBitmap != NULL ) {
    
            if ( psbSrchBitmap->bMappedAllocationFlag == true ) {
                iUtlFileMemoryUnMap(psbSrchBitmap->pucBitmap, UTL_BITMAP_GET_BITMAP_BYTE_LENGTH(psbSrchBitmap->uiBitmapLength));
                psbSrchBitmap->pucBitmap = NULL;
            }
            else {
                s_free(psbSrchBitmap->pucBitmap);
            }
        }

        s_free(psbSrchBitmap);
    }


    return (SRCH_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchBitmapMergeXOR()

    Purpose:    This function XORs psbSrchBitmap1 and psbSrchBitmap2
                and return a pointer to the new search bitmap structure.
                It disposes of the two passed search bitmap structures 
                except if an error occurs.

    Parameters: psbSrchBitmap1        search bitmap structure
                psbSrchBitmap2        search bitmap structure
                ppsbSrchBitmap        return pointer for the search bitmap structure

    Globals:    none

    Returns:    SRCH Error code

*/
int iSrchBitmapMergeXOR
(
    struct srchBitmap *psbSrchBitmap1,
    struct srchBitmap *psbSrchBitmap2,
    struct srchBitmap **ppsbSrchBitmap
)
{
    
    int                 iError = SRCH_NoError;
    struct srchBitmap   *psbSrchBitmap = NULL;

    unsigned char       *pucBitmapPtr = NULL;
    unsigned char       *pucBitmap1Ptr = NULL;
    unsigned char       *pucBitmap2Ptr = NULL;
    unsigned char       *pucBitmapEnd = NULL;


    /* Check the parameters */
    if ( psbSrchBitmap1 == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'psbSrchBitmap1' parameter passed to 'iSrchBitmapMergeXOR'."); 
        return (SRCH_BitmapInvalidBitmap);
    }

    if ( psbSrchBitmap1->pucBitmap == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'psbSrchBitmap1' parameter passed to 'iSrchBitmapMergeXOR'."); 
        return (SRCH_BitmapInvalidBitmap);
    }

    if ( psbSrchBitmap2 == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'psbSrchBitmap2' parameter passed to 'iSrchBitmapMergeXOR'."); 
        return (SRCH_BitmapInvalidBitmap);
    }

    if ( psbSrchBitmap2->pucBitmap == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'psbSrchBitmap2' parameter passed to 'iSrchBitmapMergeXOR'."); 
        return (SRCH_BitmapInvalidBitmap);
    }

    if ( ppsbSrchBitmap == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppsbSrchBitmap' parameter passed to 'iSrchBitmapMergeXOR'."); 
        return (SPI_ReturnParameterError);
    }
    
    
    /* Reuse one or the other search bitmap structure if we can, otherwise we allocate a new one */
    if ( psbSrchBitmap1->bMappedAllocationFlag == false ) {
        psbSrchBitmap = psbSrchBitmap1;
    }
    else if ( psbSrchBitmap2->bMappedAllocationFlag == false ) {
        psbSrchBitmap = psbSrchBitmap2;
    }
    else {

        /* Allocate a new search bitmap structure */
        if ( (iError = iSrchBitmapCreate(NULL, UTL_MACROS_MIN(psbSrchBitmap1->uiBitmapLength, psbSrchBitmap2->uiBitmapLength), false, &psbSrchBitmap)) != SRCH_NoError ) {
            return (iError);
        }
    }


    /* XOR the two search bitmap arrays */
    for ( pucBitmapPtr = psbSrchBitmap->pucBitmap, pucBitmap1Ptr = psbSrchBitmap1->pucBitmap, pucBitmap2Ptr = psbSrchBitmap2->pucBitmap, 
            pucBitmapEnd = psbSrchBitmap->pucBitmap + UTL_BITMAP_GET_BITMAP_BYTE_LENGTH(UTL_MACROS_MIN(psbSrchBitmap1->uiBitmapLength, psbSrchBitmap2->uiBitmapLength)); 
            pucBitmapPtr < pucBitmapEnd; pucBitmapPtr++, pucBitmap1Ptr++, pucBitmap2Ptr++ ) {

        *pucBitmapPtr = *pucBitmap1Ptr ^ *pucBitmap2Ptr;
    }

    
    /* Free the search bitmap structures */
    if ( psbSrchBitmap != psbSrchBitmap1 ) {
        iSrchBitmapFree(psbSrchBitmap1);
        psbSrchBitmap1 = NULL;
    }
    
    if ( psbSrchBitmap != psbSrchBitmap2 ) {
        iSrchBitmapFree(psbSrchBitmap2);
        psbSrchBitmap2 = NULL;
    }
    
    
    /* Set the return pointer */
    *ppsbSrchBitmap = psbSrchBitmap;
    

    return (SRCH_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchBitmapMergeOR()

    Purpose:    This function ORs psbSrchBitmap1 and psbSrchBitmap2
                and return a pointer to the new search bitmap structure.
                It disposes of the two passed search bitmap structures 
                except if an error occurs.

    Parameters: psbSrchBitmap1      search bitmap structure
                psbSrchBitmap2      search bitmap structure
                ppsbSrchBitmap      return pointer for the search bitmap structure

    Globals:    none

    Returns:    SRCH Error code

*/
int iSrchBitmapMergeOR
(
    struct srchBitmap *psbSrchBitmap1,
    struct srchBitmap *psbSrchBitmap2,
    struct srchBitmap **ppsbSrchBitmap
)
{
    
    int                 iError = SRCH_NoError;
    struct srchBitmap   *psbSrchBitmap = NULL;

    unsigned char       *pucBitmapPtr = NULL;
    unsigned char       *pucBitmap1Ptr = NULL;
    unsigned char       *pucBitmap2Ptr = NULL;
    unsigned char       *pucBitmapEnd = NULL;


    /* Check the parameters */
    if ( psbSrchBitmap1 == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'psbSrchBitmap1' parameter passed to 'iSrchBitmapMergeOR'."); 
        return (SRCH_BitmapInvalidBitmap);
    }

    if ( psbSrchBitmap1->pucBitmap == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'psbSrchBitmap1' parameter passed to 'iSrchBitmapMergeOR'."); 
        return (SRCH_BitmapInvalidBitmap);
    }

    if ( psbSrchBitmap2 == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'psbSrchBitmap2' parameter passed to 'iSrchBitmapMergeOR'."); 
        return (SRCH_BitmapInvalidBitmap);
    }

    if ( psbSrchBitmap2->pucBitmap == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'psbSrchBitmap2' parameter passed to 'iSrchBitmapMergeOR'."); 
        return (SRCH_BitmapInvalidBitmap);
    }

    if ( ppsbSrchBitmap == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppsbSrchBitmap' parameter passed to 'iSrchBitmapMergeOR'."); 
        return (SPI_ReturnParameterError);
    }
    
    
    /* Reuse one or the other search bitmap structure if we can, otherwise we allocate a new one */
    if ( psbSrchBitmap1->bMappedAllocationFlag == false ) {
        psbSrchBitmap = psbSrchBitmap1;
    }
    else if ( psbSrchBitmap2->bMappedAllocationFlag == false ) {
        psbSrchBitmap = psbSrchBitmap2;
    }
    else {

        /* Allocate a new search bitmap structure */
        if ( (iError = iSrchBitmapCreate(NULL, UTL_MACROS_MIN(psbSrchBitmap1->uiBitmapLength, psbSrchBitmap2->uiBitmapLength), false, &psbSrchBitmap)) != SRCH_NoError ) {
            return (iError);
        }
    }


    /* OR the two search bitmap arrays */
    for ( pucBitmapPtr = psbSrchBitmap->pucBitmap, pucBitmap1Ptr = psbSrchBitmap1->pucBitmap, pucBitmap2Ptr = psbSrchBitmap2->pucBitmap, 
            pucBitmapEnd = psbSrchBitmap->pucBitmap + UTL_BITMAP_GET_BITMAP_BYTE_LENGTH(UTL_MACROS_MIN(psbSrchBitmap1->uiBitmapLength, psbSrchBitmap2->uiBitmapLength)); 
            pucBitmapPtr < pucBitmapEnd; pucBitmapPtr++, pucBitmap1Ptr++, pucBitmap2Ptr++ ) {

        *pucBitmapPtr = *pucBitmap1Ptr | *pucBitmap2Ptr;
    }

    
    /* Free the search bitmap structures */
    if ( psbSrchBitmap != psbSrchBitmap1 ) {
        iSrchBitmapFree(psbSrchBitmap1);
        psbSrchBitmap1 = NULL;
    }
    
    if ( psbSrchBitmap != psbSrchBitmap2 ) {
        iSrchBitmapFree(psbSrchBitmap2);
        psbSrchBitmap2 = NULL;
    }
    
    
    /* Set the return pointer */
    *ppsbSrchBitmap = psbSrchBitmap;
    

    return (SRCH_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchBitmapMergeAND()

    Purpose:    This function ANDs psbSrchBitmap1 and psbSrchBitmap2
                and return a pointer to the new search bitmap structure.
                It disposes of the two passed search bitmap structures 
                except if an error occurs.

    Parameters: psbSrchBitmap1      search bitmap structure
                psbSrchBitmap2      search bitmap structure
                ppsbSrchBitmap      return pointer for the search bitmap structure

    Globals:    none

    Returns:    SRCH Error code

*/
int iSrchBitmapMergeAND
(
    struct srchBitmap *psbSrchBitmap1,
    struct srchBitmap *psbSrchBitmap2,
    struct srchBitmap **ppsbSrchBitmap
)
{

    int                 iError = SRCH_NoError;
    struct srchBitmap   *psbSrchBitmap = NULL;

    unsigned char       *pucBitmapPtr = NULL;
    unsigned char       *pucBitmap1Ptr = NULL;
    unsigned char       *pucBitmap2Ptr = NULL;
    unsigned char       *pucBitmapEnd = NULL;


    /* Check the parameters */
    if ( psbSrchBitmap1 == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'psbSrchBitmap1' parameter passed to 'iSrchBitmapMergeAND'."); 
        return (SRCH_BitmapInvalidBitmap);
    }

    if ( psbSrchBitmap1->pucBitmap == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'psbSrchBitmap1' parameter passed to 'iSrchBitmapMergeAND'."); 
        return (SRCH_BitmapInvalidBitmap);
    }

    if ( psbSrchBitmap2 == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'psbSrchBitmap2' parameter passed to 'iSrchBitmapMergeAND'."); 
        return (SRCH_BitmapInvalidBitmap);
    }

    if ( psbSrchBitmap2->pucBitmap == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'psbSrchBitmap2' parameter passed to 'iSrchBitmapMergeAND'."); 
        return (SRCH_BitmapInvalidBitmap);
    }

    if ( ppsbSrchBitmap == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppsbSrchBitmap' parameter passed to 'iSrchBitmapMergeAND'."); 
        return (SPI_ReturnParameterError);
    }
    
    
    /* Reuse one or the other search bitmap structure if we can, otherwise we allocate a new one */
    if ( psbSrchBitmap1->bMappedAllocationFlag == false ) {
        psbSrchBitmap = psbSrchBitmap1;
    }
    else if ( psbSrchBitmap2->bMappedAllocationFlag == false ) {
        psbSrchBitmap = psbSrchBitmap2;
    }
    else {

        /* Allocate a new search bitmap structure */
        if ( (iError = iSrchBitmapCreate(NULL, UTL_MACROS_MIN(psbSrchBitmap1->uiBitmapLength, psbSrchBitmap2->uiBitmapLength), false, &psbSrchBitmap)) != SRCH_NoError ) {
            return (iError);
        }
    }


    /* AND the two search bitmap arrays */
    for ( pucBitmapPtr = psbSrchBitmap->pucBitmap, pucBitmap1Ptr = psbSrchBitmap1->pucBitmap, pucBitmap2Ptr = psbSrchBitmap2->pucBitmap, 
            pucBitmapEnd = psbSrchBitmap->pucBitmap + UTL_BITMAP_GET_BITMAP_BYTE_LENGTH(UTL_MACROS_MIN(psbSrchBitmap1->uiBitmapLength, psbSrchBitmap2->uiBitmapLength)); 
            pucBitmapPtr < pucBitmapEnd; pucBitmapPtr++, pucBitmap1Ptr++, pucBitmap2Ptr++ ) {

        *pucBitmapPtr = *pucBitmap1Ptr & *pucBitmap2Ptr;
    }

    
    /* Free the search bitmap structures */
    if ( psbSrchBitmap != psbSrchBitmap1 ) {
        iSrchBitmapFree(psbSrchBitmap1);
        psbSrchBitmap1 = NULL;
    }
    
    if ( psbSrchBitmap != psbSrchBitmap2 ) {
        iSrchBitmapFree(psbSrchBitmap2);
        psbSrchBitmap2 = NULL;
    }
    
    
    /* Set the return pointer */
    *ppsbSrchBitmap = psbSrchBitmap;
    

    return (SRCH_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchBitmapMergeNOT()

    Purpose:    This function NOTs psbSrchBitmap1 and psbSrchBitmap2
                and return a pointer to the new search bitmap structure.
                It disposes of the two passed search bitmap structures 
                except if an error occurs.
                ppsbSrchBitmap        return pointer for the search bitmap structure

    Parameters: psbSrchBitmap1      search bitmap structure
                psbSrchBitmap2      search bitmap structure

    Globals:    none

    Returns:    SRCH Error code

*/
int iSrchBitmapMergeNOT
(
    struct srchBitmap *psbSrchBitmap1,
    struct srchBitmap *psbSrchBitmap2,
    struct srchBitmap **ppsbSrchBitmap
)
{
    
    int                 iError = SRCH_NoError;
    struct srchBitmap   *psbSrchBitmap = NULL;

    unsigned char       *pucBitmapPtr = NULL;
    unsigned char       *pucBitmap1Ptr = NULL;
    unsigned char       *pucBitmap2Ptr = NULL;
    unsigned char       *pucBitmapEnd = NULL;
        

    /* Check the parameters */
    if ( psbSrchBitmap1 == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'psbSrchBitmap1' parameter passed to 'iSrchBitmapMergeNOT'."); 
        return (SRCH_BitmapInvalidBitmap);
    }

    if ( psbSrchBitmap1->pucBitmap == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'psbSrchBitmap1' parameter passed to 'iSrchBitmapMergeNOT'."); 
        return (SRCH_BitmapInvalidBitmap);
    }

    if ( psbSrchBitmap2 == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'psbSrchBitmap2' parameter passed to 'iSrchBitmapMergeNOT'."); 
        return (SRCH_BitmapInvalidBitmap);
    }

    if ( psbSrchBitmap2->pucBitmap == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'psbSrchBitmap2' parameter passed to 'iSrchBitmapMergeNOT'."); 
        return (SRCH_BitmapInvalidBitmap);
    }

    if ( ppsbSrchBitmap == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppsbSrchBitmap' parameter passed to 'iSrchBitmapMergeNOT'."); 
        return (SPI_ReturnParameterError);
    }
    
    
    /* Reuse one or the other search bitmap structure if we can, otherwise we allocate a new one */
    if ( psbSrchBitmap1->bMappedAllocationFlag == false ) {
        psbSrchBitmap = psbSrchBitmap1;
    }
    else if ( psbSrchBitmap2->bMappedAllocationFlag == false ) {
        psbSrchBitmap = psbSrchBitmap2;
    }
    else {

        /* Allocate a new search bitmap structure */
        if ( (iError = iSrchBitmapCreate(NULL, UTL_MACROS_MIN(psbSrchBitmap1->uiBitmapLength, psbSrchBitmap2->uiBitmapLength), false, &psbSrchBitmap)) != SRCH_NoError ) {
            return (iError);
        }
    }


    /* NOT the two search bitmap arrays */
    for ( pucBitmapPtr = psbSrchBitmap->pucBitmap, pucBitmap1Ptr = psbSrchBitmap1->pucBitmap, pucBitmap2Ptr = psbSrchBitmap2->pucBitmap, 
            pucBitmapEnd = psbSrchBitmap->pucBitmap + UTL_BITMAP_GET_BITMAP_BYTE_LENGTH(UTL_MACROS_MIN(psbSrchBitmap1->uiBitmapLength, psbSrchBitmap2->uiBitmapLength)); 
            pucBitmapPtr < pucBitmapEnd; pucBitmapPtr++, pucBitmap1Ptr++, pucBitmap2Ptr++ ) {

        *pucBitmapPtr = *pucBitmap1Ptr | *pucBitmap2Ptr;
    }

    
    /* Free the search bitmap structures */
    if ( psbSrchBitmap != psbSrchBitmap1 ) {
        iSrchBitmapFree(psbSrchBitmap1);
        psbSrchBitmap1 = NULL;
    }
    
    if ( psbSrchBitmap != psbSrchBitmap2 ) {
        iSrchBitmapFree(psbSrchBitmap2);
        psbSrchBitmap2 = NULL;
    }
    
    
    /* Set the return pointer */
    *ppsbSrchBitmap = psbSrchBitmap;
    

    return (SRCH_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iSrchBitmapPrint()

    Purpose:    This function prints out the contents of the search bitmap structure.
                This is used for debugging purposed only.

    Parameters: psbSrchBitmap       search bitmap structure

    Globals:    none

    Returns:    SRCH error code

*/
int iSrchBitmapPrint
(
    struct srchBitmap *psbSrchBitmap
)
{

    /* Check input variables */
    if ( psbSrchBitmap == NULL ) {
        iUtlLogInfo(UTL_LOG_CONTEXT, "iSrchBitmapPrint - psbSrchBitmap pointer is NULL");
    }

    if ( psbSrchBitmap->pucBitmap == NULL ) {
        iUtlLogInfo(UTL_LOG_CONTEXT, "iSrchBitmapPrint - psbSrchBitmap->pucBitmap pointer is NULL");
    }

    if ( psbSrchBitmap->uiBitmapLength == 0 ) {
        iUtlLogInfo(UTL_LOG_CONTEXT, "iSrchBitmapPrint - psbSrchBitmap->uiBitmapLength = %u", psbSrchBitmap->uiBitmapLength);
    }


    /* Print out the search bitmap structure */
    if ( (psbSrchBitmap != NULL) && (psbSrchBitmap->pucBitmap != NULL) ) {
        
        unsigned int    uiI = 0;

        for ( uiI = 0; uiI < psbSrchBitmap->uiBitmapLength; uiI++ ) {

            printf("%8u - %s\n", uiI, (UTL_BITMAP_IS_BIT_SET_IN_POINTER(psbSrchBitmap->pucBitmap, uiI)) ? "true" : "false");

        }
    }


    return (SRCH_NoError);

}


/*---------------------------------------------------------------------------*/
