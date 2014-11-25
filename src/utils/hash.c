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

    Module:     hash.c

    Author:     Francois Schiettecatte (FS Consulting LLC.)

    Created:    28 November 1995

    Purpose:    This module implements a simple hash table. This table is
                really designed to hold small amounts of information, say
                a couple of thousand keys, but is not optimized for more
                than than.

*/


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
#define UTL_LOG_CONTEXT                     (unsigned char *)"com.fsconsult.mps.src.utils.hash"


/*---------------------------------------------------------------------------*/


/*
** Defines
*/

/* Hash size */
#define UTL_HASH_SIZE           (511)                       /* Should be prime number */


/*---------------------------------------------------------------------------*/


/*
** Structures
*/

/* Hash key */
struct utlHashKey {
    unsigned char       *pucKey;                            /* Key field */
    void                *pvDatum;
    struct utlHashKey   *puhkNextUtlHashKey;
};


/* Hash */
struct utlHash {
    struct utlHashKey   *puhkUtlHashKeys[UTL_HASH_SIZE];    /* Array of hash keys */
    unsigned int        uiEntryCount;                       /* Number of entries in the hash */
    size_t              pzMemorySize;                       /* Size of the hash in bytes */
};


/*---------------------------------------------------------------------------*/


/*
** Private function prototypes
*/

static int iUtlHashGetKey (unsigned char *pucKey, unsigned int *puiHashKey);

static int iUtlHashFreeHash (struct utlHash *puhUtlHash, boolean bFreeDatum);

static int iUtlHashLookupKeyInHash (struct utlHash *puhUtlHash, unsigned char *pucKey, 
        void ***pppvDatum);

static int iUtlHashAddKeyToHash (struct utlHash *puhUtlHash, unsigned char *pucKey, 
        void ***pppvDatum);

static int iUtlHashLoopOverKeysInHash (struct utlHash *puhUtlHash, 
        int (*iUtlHashCallBackFunction)(), va_list ap);

static int iUtlHashPrintCallBackFunction (unsigned char *pucKey, 
        void *pvData, va_list ap);


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlHashCreate()

    Purpose:    Create a new hash structure. 

                This function will set up the new hash structure and return
                a pointer to those structure.

    Parameters: ppvUtlHash      return pointer for the newly
                                created hash structure

    Globals:    none

    Returns:    UTL error code

*/
int iUtlHashCreate
(
    void **ppvUtlHash
)
{

    struct utlHash      *puhUtlHash = NULL;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iUtlHashCreate "); */


    /* Check the parameters */
    if ( ppvUtlHash == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppvUtlHash' parameter passed to 'iUtlHashCreate'."); 
        return (UTL_ReturnParameterError);
    }


    /* Allocate the hash */
    if ( (puhUtlHash = (struct utlHash *)s_malloc((size_t)(sizeof(struct utlHash)))) == NULL ) {
        return (UTL_MemError);
    }

    /* Initialize the entry counter */
    puhUtlHash->uiEntryCount = 0;

    /* Initialize the hash size */
    puhUtlHash->pzMemorySize = sizeof(struct utlHash);

    /* Set the return pointer */
    *ppvUtlHash = (void *)puhUtlHash;


    return (UTL_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlHashFree()

    Purpose:    Free the hash.

                This function will release all the memory resources currently
                held by this hash. After this the hash will be invalid.

    Parameters: pvUtlHash       hash to release
                bFreeDatum      set to true if datum needs to be freed

    Globals:    none

    Returns:    UTL error code

*/
int iUtlHashFree
(
    void *pvUtlHash,
    boolean bFreeDatum
)
{

    struct utlHash      *puhUtlHash = (struct utlHash *)pvUtlHash;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iUtlHashFree"); */


    /* Check the parameters */
    if ( pvUtlHash == NULL ) {
        iUtlLogDebug(UTL_LOG_CONTEXT, "Null 'pvUtlHash' parameter passed to 'iUtlHashFree'."); 
        return (UTL_HashInvalidHash);
    }


/*     iUtlHashPrintKeys(puhUtlHash, false); */


    /* Free the hash data */
    iUtlHashFreeHash(puhUtlHash, bFreeDatum);

    /* Free the hash */
    s_free(puhUtlHash);


    return (UTL_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlHashAdd()

    Purpose:    Add a key to the hash. 

                This function will add the passed key in the hash. If
                the key does not exist, it will be inserted into the hash.
                A pointer to a pointer will be returned which may be used
                to store data about the key.

    Parameters: pvUtlHash       hash
                pucKey          key to add
                ppvDatum        return pointer for the datum (optional)

    Globals:    none

    Returns:    UTL error code

*/
int iUtlHashAdd
(
    void *pvUtlHash,
    unsigned char *pucKey,
    void ***pppvDatum
)
{

    int                 iError = UTL_NoError;
    struct utlHash      *puhUtlHash = (struct utlHash *)pvUtlHash;
    void                **ppvDatum = NULL;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "ppvHashAdd - pucKey [%s]", pucKey); */


    /* Check the parameters */
    if ( pvUtlHash == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvUtlHash' parameter passed to 'iUtlHashAdd'."); 
        return (UTL_HashInvalidHash);
    }

    if ( bUtlStringsIsStringNULL(pucKey) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pucKey' parameter passed to 'iUtlHashAdd'."); 
        return (UTL_HashInvalidKey);
    }


    /* Look up the key  */
    iError = iUtlHashLookupKeyInHash(puhUtlHash, pucKey, &ppvDatum);
    
    /* Add the key if it was not found in the hash */
    if ( iError == UTL_HashKeyNotFound ) {

        /* Create a new hash entry for this key */
        if ( (iError = iUtlHashAddKeyToHash(puhUtlHash, pucKey, &ppvDatum)) != UTL_NoError ) {
            return (iError);
        }

        /* Increment the number of unique entries */
        puhUtlHash->uiEntryCount++;
    }
    /* Report errors */
    else if ( iError != UTL_NoError ) {
        return (iError);
    }


    /* Set the return pointer if it was passed */
    if ( pppvDatum != NULL ) {
        *pppvDatum = ppvDatum;
    }


    return (UTL_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlHashLookup()

    Purpose:    Look up a key in the hash. 

                This function will look for the passed key in the hash.
                If the key exists, a pointer to a pointer will be returned

    Parameters: pvUtlHash       hash
                pucKey          key to lookup
                pppvDatum       return pointer for the datum (optional)

    Globals:    none

    Returns:    UTL error code

*/
int iUtlHashLookup
(
    void *pvUtlHash,
    unsigned char *pucKey,
    void ***pppvDatum
)
{

    int                 iError = UTL_NoError;
    struct utlHash      *puhUtlHash = (struct utlHash *)pvUtlHash;
    void                **ppvDatum = NULL;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iUtlHashLookup - pucKey [%s]", pucKey); */


    /* Check the parameters */
    if ( pvUtlHash == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvUtlHash' parameter passed to 'iUtlHashLookup'."); 
        return (UTL_HashInvalidHash);
    }

    if ( bUtlStringsIsStringNULL(pucKey) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pucKey' parameter passed to 'iUtlHashLookup'."); 
        return (UTL_HashInvalidKey);
    }


    /* Look up the key */
    if ( (iError = iUtlHashLookupKeyInHash(puhUtlHash, pucKey, &ppvDatum)) != UTL_NoError ) {
        return (iError);
    }


    /* Set the return pointer if it was passed */
    if ( pppvDatum != NULL ) {
        *pppvDatum = ppvDatum;
    }


    return (UTL_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlHashLoopOverKeys()

    Purpose:    Loop through the hash information.

                This function will loop over each key in the hash and call
                the call back function with each key. All the parameters
                passed after the call back function name will be passed to
                the call back function. In addition the call back function
                will be passed a pointer to the key and a pointer to the 
                data.

                The declaration format for the call back function is:

                    lCallBackFunction(unsigned char *pucKey, void *pvData, va_list ap)

                The call back function should return 0 on success and non-0 on error.

    Parameters: pvUtlHash                       hash
                (*iUtlHashCallBackFunction)()   call back function pointer
                ...                             args (optional)

    Globals:    none

    Returns:    UTL error code

*/
int iUtlHashLoopOverKeys
(
    void *pvUtlHash,
    int (*iUtlHashCallBackFunction)(),
    ...
)
{

    struct utlHash      *puhUtlHash = (struct utlHash *)pvUtlHash;
    int                 iError = UTL_NoError;
    va_list             ap;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iUtlHashLoopOverKeys"); */


    /* Check the parameters */
    if ( pvUtlHash == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvUtlHash' parameter passed to 'iUtlHashLoopOverKeys'."); 
        return (UTL_HashInvalidHash);
    }

    if ( iUtlHashCallBackFunction == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'iUtlHashCallBackFunction' parameter passed to 'iUtlHashLoopOverKeys'."); 
        return (UTL_HashInvalidCallBackFunction);
    }


    va_start(ap, iUtlHashCallBackFunction);
    iError = iUtlHashLoopOverKeysInHash(puhUtlHash, (int (*)())iUtlHashCallBackFunction, ap);
    va_end(ap);


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlHashGetEntryCount()

    Purpose:    Return the number of entries in a hash.

                This function will return the number of entries
                in the hash.

    Parameters: pvUtlHash       hash
                puiEntryCount   return pointer for the entry count

    Globals:    none

    Returns:    UTL error code

*/
int iUtlHashGetEntryCount
(
    void *pvUtlHash,
    unsigned int *puiEntryCount
)
{

    struct utlHash      *puhUtlHash = (struct utlHash *)pvUtlHash;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "lHashGetUniqueEntriesCount"); */


    /* Check the parameters */
    if ( pvUtlHash == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvUtlHash' parameter passed to 'iUtlHashGetEntryCount'."); 
        return (UTL_HashInvalidHash);
    }

    if ( puiEntryCount == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'puiEntryCount' parameter passed to 'iUtlHashGetEntryCount'."); 
        return (UTL_ReturnParameterError);
    }


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iUtlHashGetEntryCount [%lu]", puhUtlHash->uiEntryCount); */


    /* Set the return pointer */
    *puiEntryCount = puhUtlHash->uiEntryCount;


    return (UTL_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   lHashSize()

    Purpose:    Return the current memory size of the hash in bytes.

    Parameters: pvUtlHash       hash
                pzMemorySize    return pointer for the hash memory size

    Globals:    none

    Returns:    UTL error code

*/
int iUtlHashGetMemorySize
(
    void *pvUtlHash,
    size_t *pzMemorySize
)
{

    struct utlHash      *puhUtlHash = (struct utlHash *)pvUtlHash;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "lHashSize"); */


    /* Check the parameters */
    if ( puhUtlHash == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvUtlHash' parameter passed to 'iUtlHashGetMemorySize'."); 
        return (UTL_HashInvalidHash);
    }

    if ( pzMemorySize == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pzMemorySize' parameter passed to 'iUtlHashGetMemorySize'."); 
        return (UTL_ReturnParameterError);
    }


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "puhUtlHash->pzMemorySize [%lu]", (unsigned long)puhUtlHash->pzMemorySize); */


    /* Set the return pointer */
    *pzMemorySize = puhUtlHash->pzMemorySize;


    return (UTL_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlHashPrintKeys()

    Purpose:    Print all the keys in the hash.

                This function will loop over all the keys in a hash and 
                print them out to the log.

    Parameters: pvUtlHash       hash
                bPrintData      set to true to print the data

    Globals:    none

    Returns:    UTL error code

*/
int iUtlHashPrintKeys
(
    void *pvUtlHash,
    boolean bPrintData
)
{

    struct utlHash      *puhUtlHash = (struct utlHash *)pvUtlHash;
    int                 iError = UTL_NoError;


    /* Check the hash */
    if ( pvUtlHash == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvUtlHash' parameter passed to 'iUtlHashPrintKeys'."); 
        return (UTL_HashInvalidHash);
    }


    iError = iUtlHashLoopOverKeys(puhUtlHash, (int (*)())iUtlHashPrintCallBackFunction, (int)bPrintData);


    return (iError);

}


/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/


/*
    Function:   iUtlHashGetKey()

    Purpose:    Computes hash value for key string.

    Parameters: pucKey          key for which to work out the hash value
                puiHashKey      return pointer for the hash key

    Globals:      none

    Returns:    UTL error code

*/
static int iUtlHashGetKey
(
    unsigned char *pucKey,
    unsigned int *puiHashKey
)
{

    unsigned char   *pucKeyPtr = NULL;
    unsigned int    uiHashKey = 0;


    ASSERT(bUtlStringsIsStringNULL(pucKey) == false);
    ASSERT(puiHashKey != NULL);


    /* Generate a hash key for this string */
    for ( pucKeyPtr = pucKey; *pucKeyPtr != '\0'; pucKeyPtr++ ) {
        uiHashKey += *pucKeyPtr & 127;
    }
    uiHashKey = uiHashKey % UTL_HASH_SIZE;


    /* Set the return pointer */
    *puiHashKey = uiHashKey;


    return (UTL_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlHashFreeHash()

    Purpose:    Function to free the entries in the hash table

    Parameters: puhUtlHash      hash to release
                bFreeDatum      set to true if datum needs to be freed

    Globals:    none

    Returns:    UTL error code

*/
static int iUtlHashFreeHash
(
    struct utlHash *puhUtlHash,
    boolean bFreeDatum
)
{

    struct utlHashKey   *pheHashEntry = NULL;
    struct utlHashKey   *phkNextHashKeyHashEntry = NULL;
    unsigned int        uiI = 0;


    ASSERT(puhUtlHash != NULL);
    ASSERT((bFreeDatum == true) || (bFreeDatum == false));


    /* Loop through the hash table */
    for ( uiI = 0; uiI < UTL_HASH_SIZE; uiI++ ) {

        pheHashEntry = puhUtlHash->puhkUtlHashKeys[uiI];

        /* Loop while there are entries and free them */
        while ( pheHashEntry != NULL ) {
            
            /* Get the next hash entry */
            phkNextHashKeyHashEntry = pheHashEntry->puhkNextUtlHashKey;

            /* Free the datum if requested */
            if ( bFreeDatum == true ) {
                s_free(pheHashEntry->pvDatum);
            }
            s_free(pheHashEntry->pucKey)
            s_free(pheHashEntry);
            
            /* The next entry is now the current entry */
            pheHashEntry = phkNextHashKeyHashEntry;
        }
    }        


    return (UTL_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlHashLookupKeyInHash()

    Purpose:    Function to lookup a key in the hash table

    Parameters: pvUtlHash   hash
                pucKey      key to lookup
                pppvDatum   return pointer for the datum

    Globals:    none

    Returns:    UTL error code

*/
static int iUtlHashLookupKeyInHash
(
    struct utlHash *puhUtlHash,
    unsigned char *pucKey,
    void ***pppvDatum
)
{

    int                 iError = UTL_NoError;
    struct utlHashKey   *puhkUtlHashKeyPtr = NULL;
    unsigned int        uiHashKey = 0;


    ASSERT(puhUtlHash != NULL);
    ASSERT(bUtlStringsIsStringNULL(pucKey) == false);
    ASSERT(pppvDatum != NULL);


    /* Get the hash key for this key */
    if ( (iError = iUtlHashGetKey(pucKey, &uiHashKey) != UTL_NoError) ) {
        return (iError);
    }


    /* Loop through all the entries in this hash bucket and see if we find the value we are looking for */
    for ( puhkUtlHashKeyPtr = puhUtlHash->puhkUtlHashKeys[uiHashKey]; puhkUtlHashKeyPtr != NULL; puhkUtlHashKeyPtr = puhkUtlHashKeyPtr->puhkNextUtlHashKey ) {
        if ( s_strcmp(puhkUtlHashKeyPtr->pucKey, pucKey) == 0 ) {
            *pppvDatum = &puhkUtlHashKeyPtr->pvDatum;
            return (UTL_NoError);
        }
    }


    return (UTL_HashKeyNotFound);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlHashAddKeyToHash()

    Purpose:    Function to add a key in the hash table

    Parameters: pvUtlHash       hash
                pucKey          key to lookup
                pppvDatum       return pointer for the datum

    Globals:    none

    Returns:    UTL error code

*/
static int iUtlHashAddKeyToHash
(
    struct utlHash *puhUtlHash,
    unsigned char *pucKey,
    void ***pppvDatum
)
{

    int                 iError = UTL_NoError;
    struct utlHashKey   *puhkUtlHashKeyPtr = NULL;
    struct utlHashKey   *puhkUtlHashKeyHeadPtr = NULL;
    unsigned int        uiHashKey = 0;


    ASSERT(puhUtlHash != NULL);
    ASSERT(bUtlStringsIsStringNULL(pucKey) == false);
    ASSERT(pppvDatum != NULL);


    /* Get the hash key for this key */
    if ( (iError = iUtlHashGetKey(pucKey, &uiHashKey) != UTL_NoError) ) {
        return (iError);
    }

    /* Get the current head entry */
    puhkUtlHashKeyHeadPtr = puhUtlHash->puhkUtlHashKeys[uiHashKey];

    /* Create a new entry */
    if ( (puhkUtlHashKeyPtr = (struct utlHashKey *)s_malloc((size_t)(sizeof(struct utlHashKey)))) == NULL ) {
        return (UTL_MemError);
    }


    /* Increment the hash size */
    puhUtlHash->pzMemorySize += sizeof(struct utlHashKey);

    /* Attach it */
    puhkUtlHashKeyPtr->puhkNextUtlHashKey = puhkUtlHashKeyHeadPtr;
    puhUtlHash->puhkUtlHashKeys[uiHashKey] = puhkUtlHashKeyPtr;

    /* Make a copy of the key */
    if ( (puhkUtlHashKeyPtr->pucKey = (unsigned char *)s_strdup(pucKey)) == NULL ) {
        return (UTL_MemError);
    }


    /* Set the return pointer */
    *pppvDatum = &puhkUtlHashKeyPtr->pvDatum;


    return (UTL_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlHashLoopOverKeysInHash()

    Purpose:    Loop through the hash.

                This function will loop over each key in the trie and call
                the passed call back function for every key. The va_list will
                be passed to the call back function along with a pointer to
                the key and a pointer to the data. 

    Parameters: puhUtlHash                      hash
                (*iUtlHashCallBackFunction)()   call back function pointer
                ap                              arg list (optional)

    Globals:    none

    Returns:    UTL error code

*/
static int iUtlHashLoopOverKeysInHash
(
    struct utlHash *puhUtlHash,
    int (*iUtlHashCallBackFunction)(),
    va_list ap
)
{

    int                 iError = UTL_NoError;
    struct utlHashKey   *puhkUtlHashKeyPtr = NULL;
    unsigned int        uiI = 0;


    ASSERT(puhUtlHash != NULL);
    ASSERT(iUtlHashCallBackFunction != NULL);


    /* Loop through the hash table */
    for ( uiI = 0; uiI < UTL_HASH_SIZE; uiI++) {
        for (puhkUtlHashKeyPtr = puhUtlHash->puhkUtlHashKeys[uiI]; puhkUtlHashKeyPtr != NULL; puhkUtlHashKeyPtr = puhkUtlHashKeyPtr->puhkNextUtlHashKey) {
            if ( (iError = iUtlHashCallBackFunction(puhkUtlHashKeyPtr->pucKey, puhkUtlHashKeyPtr->pvDatum, ap)) != 0 ) {
                return (iError);
            }
        }
    }


    return (UTL_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlHashPrintCallBackFunction()

    Purpose:    This function is passed as the call back function to iUtlHashLoopOverKeys()
                and gets called for every key in the hash. It will get passed
                pointers to the key currently being processed and to the data
                stored for that key. It will also get passed as a va_list, the 
                parameters that were specified in the call to iUtlHashLoopOverKeys().

    Parameters: pucKey      key
                pvData      data        
                ap          arg list (optional)

    Globals:    none

    Returns:    0 if successful, non-0 on error
*/
static int iUtlHashPrintCallBackFunction
(
    unsigned char *pucKey,
    void *pvData,
    va_list ap
)
{

    va_list     ap_;
    

    ASSERT(bUtlStringsIsStringNULL(pucKey) == false);
    ASSERT(pvData != NULL);


    /* Get all our parameters, note that we make a copy of 'ap' */
    va_copy(ap_, ap);
    boolean bPrintData = (boolean)va_arg(ap_, int);
    va_end(ap_);


    /* Print the key/data depending on the flag */
    if ( bPrintData == true ) {
        iUtlLogInfo(UTL_LOG_CONTEXT, "'%s' => '%s'", pucKey, (unsigned char *)pvData);
    }
    else {
        iUtlLogInfo(UTL_LOG_CONTEXT, "'%s'", pucKey);
    }


    return (0);

}


/*---------------------------------------------------------------------------*/

