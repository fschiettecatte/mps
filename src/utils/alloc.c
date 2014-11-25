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


/*{

    Module:     alloc.c

    Author:     Francois Schiettecatte (FS Consulting LLC.)

    Created:    21 January 1998

    Purpose:    This module implements a fast allocator of memory which is
                good to use at those times when we need to do lots of
                mallocs. The only drawback is that we cant do any reallocs. 

}*/


/*---------------------------------------------------------------------------*/


/*
** Includes
*/
#include "utils.h"


/*---------------------------------------------------------------------------*/


/*
** Feature defines
*/

/* Context for logging */
#undef UTL_LOG_CONTEXT
#define UTL_LOG_CONTEXT                     (unsigned char *)"com.fsconsult.mps.src.utils.allocator"


/*---------------------------------------------------------------------------*/


/*
** Defines
*/

/* Macro to get an aligned allocation size */
#define UTL_ALLOC_GET_ALIGNED_ALLOCATION_SIZE(s)        ((s) + (sizeof(unsigned char *) - ((s) % sizeof(unsigned char *))))


/*---------------------------------------------------------------------------*/


/*
** Structures
*/

/* Allocator */
struct utlAllocator {
    void        *pvFirstPtr;                    /* Pointer to the first extent */
    void        *pvLastPtr;                     /* Pointer to the last extent */
    void        *pvFreePtr;                     /* Pointer to the start of free space in the last extent */
    size_t      zAllocationSize;                /* Allocator allocation size in bytes, aligned */
    size_t      zMemorySize;                    /* Allocator memory size in bytes */
};


/*---------------------------------------------------------------------------*/


/*{

    Function:   iUtlAllocCreate()

    Purpose:    Create a new allocator structure. 

                This function will set up a new allocator and return
                a pointer to it. This pointer needs to be passed
                back as part of every allocator function.

    Called by:    

    Parameters: zAllocationSize     suggested allocation size (0 for default)
                ppvUtlAllocator     return pointer for the allocator

    Globals:    none

    Returns:    UTL error code

}*/

int iUtlAllocCreate
(
    size_t zAllocationSize,
    void **ppvUtlAllocator
)
{

    struct utlAllocator     *puaUtlAllocator = NULL;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iUtlAllocCreate"); */


    /* Check the parameters */
    if ( zAllocationSize < 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'zAllocationSize' parameter passed to 'iUtlAllocCreate'."); 
        return (UTL_AllocInvalidAllocationSize);
    }

    if ( ppvUtlAllocator == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppvUtlAllocator' parameter passed to 'iUtlAllocCreate'."); 
        return (UTL_ReturnParameterError);
    }


    /* Allocate the allocator */
    if ( (puaUtlAllocator = (struct utlAllocator *)s_malloc((size_t)(sizeof(struct utlAllocator)))) == NULL ) {
        return (UTL_MemError);
    }


    /* Initialize the allocation size, use the suggested allocation 
    ** size if provided, otherwise use the default 
    */
    if ( zAllocationSize > 0 ) {
        puaUtlAllocator->zAllocationSize = UTL_ALLOC_GET_ALIGNED_ALLOCATION_SIZE(zAllocationSize);
    }
    else {
        puaUtlAllocator->zAllocationSize = UTL_ALLOC_GET_ALIGNED_ALLOCATION_SIZE(UTL_ALLOC_BYTE_SIZE_DEFAULT);
    }

    /* Initialize the memory size */
    puaUtlAllocator->zMemorySize = sizeof(struct utlAllocator);

    /* Initialize the pointers */
    puaUtlAllocator->pvFirstPtr = NULL;
    puaUtlAllocator->pvLastPtr = NULL;
    puaUtlAllocator->pvFreePtr = NULL;


    /* Set the return pointer */
    *ppvUtlAllocator = (void *)puaUtlAllocator;


    return (UTL_NoError);

}


/*---------------------------------------------------------------------------*/


/*{

    Function:   iUtlAllocFree()

    Purpose:    Free the allocator.

                This function will release all the memory resources currently
                held by this allocator. After this the allocator will 
                be invalid.

    Called by:    

    Parameters: pvUtlAllocator      allocator

    Globals:    none

    Returns:    UTL error code

}*/
int iUtlAllocFree
(
    void *pvUtlAllocator
)
{

    struct utlAllocator     *puaUtlAllocator = (struct utlAllocator *)pvUtlAllocator;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iUtlAllocFree"); */


    /* Check the parameters */
    if ( pvUtlAllocator == NULL ) {
        iUtlLogDebug(UTL_LOG_CONTEXT, "Null 'pvUtlAllocator' parameter passed to 'iUtlAllocFree'."); 
        return (UTL_AllocInvalidAllocator);
    }


    /* Loop over the linked list and free the allocated blocks */
    while ( puaUtlAllocator->pvFirstPtr != NULL ) {

        void    *pvNextPtr = *(void **)puaUtlAllocator->pvFirstPtr;

        s_free(puaUtlAllocator->pvFirstPtr);

        puaUtlAllocator->pvFirstPtr = pvNextPtr;
    }


    /* Free the allocator */
    s_free(puaUtlAllocator);


    return (UTL_NoError);

}


/*---------------------------------------------------------------------------*/


/*{

    Function:   iUtlAllocAllocate()

    Purpose:    Allocate space from the allocator.

    Called by:    

    Parameters: pvUtlAllocator      allocator
                zSize               size to allocate
                ppvData             return pointer for the space allocated

    Globals:    none

    Returns:    UTL error code

}*/
int iUtlAllocAllocate
(
    void *pvUtlAllocator,
    size_t zSize,
    void **ppvData
)
{

    struct utlAllocator     *puaUtlAllocator = (struct utlAllocator *)pvUtlAllocator;
    size_t                  zAlignedSize = UTL_ALLOC_GET_ALIGNED_ALLOCATION_SIZE(zSize);


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iUtlAllocAllocate"); */


    /* Check the parameters */
    if ( pvUtlAllocator == NULL ) {
        iUtlLogDebug(UTL_LOG_CONTEXT, "Null 'pvUtlAllocator' parameter passed to 'iUtlAllocAllocate'."); 
        return (UTL_AllocInvalidAllocator);
    }

    if ( zSize <= 0 ) {
        iUtlLogDebug(UTL_LOG_CONTEXT, "Invalid 'zSize' parameter passed to 'iUtlAllocAllocate'."); 
        return (UTL_AllocInvalidSize);
    }

    if ( ppvData == NULL ) {
        iUtlLogDebug(UTL_LOG_CONTEXT, "Null 'ppvData' parameter passed to 'iUtlAllocAllocate'."); 
        return (UTL_ReturnParameterError);
    }



    /* Check to see if this is the first time this function is called, if so
    ** we need to allocate the first block, initialize it and set the end
    */
    if ( puaUtlAllocator->pvFirstPtr == NULL ) {

        /* Allocate the first block */
        if ( (puaUtlAllocator->pvFirstPtr = (void *)s_malloc((size_t)(UTL_MACROS_MAX(puaUtlAllocator->zAllocationSize, sizeof(size_t) + zAlignedSize)))) == NULL ) {
            return (UTL_MemError);
        }

        /* Set the end - this is used to point to the next block in the chain */
        *(void **)puaUtlAllocator->pvFirstPtr = NULL;
        puaUtlAllocator->pvLastPtr = puaUtlAllocator->pvFirstPtr;
        puaUtlAllocator->pvFreePtr = (void *)((unsigned char *)puaUtlAllocator->pvFirstPtr + sizeof(size_t));
    }


    /* Check to see if what we have left in the current block will satify the 
    ** memory requirement, if so we just allocate the requirement from the
    ** current block, otherwise we create a new block and chain it to the 
    ** last block
    */
    if ( ((size_t)puaUtlAllocator->pvFreePtr + zAlignedSize) >= (puaUtlAllocator->zAllocationSize + (size_t)puaUtlAllocator->pvLastPtr) ) {

        void    *pvNewPtr = NULL;

        /* Allocate a new block */
        if ( (pvNewPtr = (void *)s_malloc((size_t)(UTL_MACROS_MAX(puaUtlAllocator->zAllocationSize, sizeof(size_t) + zAlignedSize)))) == NULL ) {
            return (UTL_MemError);
        }

        /* Set the end of the chain */
        *(void **)pvNewPtr = NULL; 
        *(void **)puaUtlAllocator->pvLastPtr = pvNewPtr;
        puaUtlAllocator->pvFreePtr = (void *)((unsigned char *)pvNewPtr + sizeof(size_t));
        puaUtlAllocator->pvLastPtr = pvNewPtr;
    }

    /* Get the pointer to the allocated memory */
    *ppvData = (void *)puaUtlAllocator->pvFreePtr;

    /* Update the new free position */
    puaUtlAllocator->pvFreePtr = (void *)((unsigned char *)puaUtlAllocator->pvFreePtr + zAlignedSize);    

    /* Update the allocated memory size */
    puaUtlAllocator->zMemorySize += zAlignedSize;


    return (UTL_NoError);

}


/*---------------------------------------------------------------------------*/


/*{

    Function:   iUtlAllocReallocate()

    Purpose:    Reallocated space from the allocator.

    Called by:    

    Parameters: pvUtlAllocator      allocator
                pvData              pointer to existing data (optional)
                zDataSize           size of existing data (optional)
                zSize               size to allocate
                ppvData             return pointer for the space allocated

    Globals:    none

    Returns:    UTL error code

}*/
int iUtlAllocReallocate
(
    void *pvUtlAllocator,
    void *pvData, 
    size_t zDataSize,
    size_t zSize,
    void **ppvData
)
{


    unsigned int            iError = UTL_NoError;
    struct utlAllocator     *puaUtlAllocator = (struct utlAllocator *)pvUtlAllocator;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iUtlAllocReallocate"); */


    /* Check the parameters */
    if ( pvUtlAllocator == NULL ) {
        iUtlLogDebug(UTL_LOG_CONTEXT, "Null 'pvUtlAllocator' parameter passed to 'iUtlAllocReallocate'."); 
        return (UTL_AllocInvalidAllocator);
    }

    if ( zSize <= 0 ) {
        iUtlLogDebug(UTL_LOG_CONTEXT, "Invalid 'zSize' parameter passed to 'iUtlAllocReallocate'."); 
        return (UTL_AllocInvalidSize);
    }

    if ( ppvData == NULL ) {
        iUtlLogDebug(UTL_LOG_CONTEXT, "Null 'ppvData' parameter passed to 'iUtlAllocReallocate'."); 
        return (UTL_ReturnParameterError);
    }


    /* Allocate space */
    if ( (iError = iUtlAllocAllocate((void *)puaUtlAllocator, zSize, ppvData)) != UTL_NoError ) {
        return (iError);
    }

    /* Copy the existing data into place */
    if ( (pvData != NULL) && (zDataSize > 0) ) {
        s_memcpy(*ppvData, pvData, zDataSize);
    }


    return (UTL_NoError);

}


/*---------------------------------------------------------------------------*/


/*{

    Function:   iUtlAllocMemorySize()

    Purpose:    Return the current memory size of the allocator in bytes.

    Called by:    

    Parameters: pvUtlAllocator      allocator
                pzMemorySize        return pointer for the memory size

    Globals:    none

    Returns:    UTL error code if negative


}*/
int iUtlAllocMemorySize
(
    void *pvUtlAllocator,
    size_t *pzMemorySize
)
{

    struct utlAllocator     *puaUtlAllocator = (struct utlAllocator *)pvUtlAllocator;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iUtlAllocMemorySize"); */


    /* Check the parameters */
    if ( pvUtlAllocator == NULL ) {
        iUtlLogDebug(UTL_LOG_CONTEXT, "Null 'pvUtlAllocator' parameter passed to 'iUtlAllocMemorySize'."); 
        return (UTL_AllocInvalidAllocator);
    }

    if ( pzMemorySize == NULL ) {
        iUtlLogDebug(UTL_LOG_CONTEXT, "Null 'pzMemorySize' parameter passed to 'iUtlAllocMemorySize'."); 
        return (UTL_ReturnParameterError);
    }


    /* Set the return pointer */
    *pzMemorySize = puaUtlAllocator->zMemorySize;


    return (UTL_NoError);

}


/*---------------------------------------------------------------------------*/
