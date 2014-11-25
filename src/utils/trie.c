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

    Module:     trie.c

    Author:     Francois Schiettecatte (FS Consulting LLC.)

    Created:    15 April 1998

    Purpose:    This module provides a ternary search tree as per:

                J.Bentley & B.Sedgewick, Ternary Search Trees, 
                Dr. Dobbs' Journals, April 1998.

                Adapted and tuned by Francois JM Schiettecatte.
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
#define UTL_LOG_CONTEXT                     (unsigned char *)"com.fsconsult.mps.src.utils.trie"


/*---------------------------------------------------------------------------*/


/*
** Defines
*/

#define UTL_TRIE_NODE_ENTRY_COUNT           (10000)         /* Number of entries in the tree node */
#define UTL_TRIE_NODE_LIST_ENTRY_COUNT      (10000)         /* Number of entries per allocation in the tree node entries list */
#define UTL_TRIE_MAX_KEY_LENGTH             (1024)          /* Maximum size of the key */


/*---------------------------------------------------------------------------*/


/*
** Structures
*/

/* Trie node structure */
struct utlTrieNode {
    struct utlTrieNode      *putnUtlTrieNodeLowKid;         /* Low kid */
    struct utlTrieNode      *putnUtlTrieNodeEqKid;          /* Same kid */
    struct utlTrieNode      *putnUtlTrieNodeHighKid;        /* High kid */
    unsigned char           ucSplitChar;                    /* Character we split on */
};


/* Trie structure */
struct utlTrie {
    struct utlTrieNode      *putnUtlTrieRootNode;           /* Root trie node */
    struct utlTrieNode      **pputnUtlTrieNodesList;        /* List of free nodes arrays */
    unsigned int            uiTrieNodeListLength;           /* Total number of entries in the free nodes list */
    unsigned int            uiTrieNodeListEntryCount;       /* Number of allocated entries in the free nodes list */
    unsigned int            uiTrieNodeFreeEntryCount;       /* Number of free entries in free nodes array */
    unsigned int            uilEntryCount;                  /* Number of entries in the trie */
    size_t                  zMemorySize;                    /* Size of the trie in bytes */
};


/*---------------------------------------------------------------------------*/


/*
** Private function prototypes
*/

static int iUtlTrieAddKeyToTrie (struct utlTrie *putUtlTrie, unsigned char *pucKey, void ***ppvDatum);

static int iUtlTrieLookupKeyInTrie (struct utlTrie *putUtlTrie, unsigned char *pucKey, void ***ppvDatum);

static int iUtlTrieLoopOverKeysInTrieNode (struct utlTrieNode *putnUtlTrieNode, unsigned char *pucKey, 
        unsigned int uiKeyLength, unsigned char *pucKeyBuffer, unsigned char **ppucKeyBufferPtr, 
        int (*iUtlTrieCallBackFunction)(), va_list ap);

static int iUtlTrieLoopOverKeysInTrie (struct utlTrie *putUtlTrie, unsigned char *pucKey, 
        int (*iUtlTrieCallBackFunction)(), va_list ap);

static int iUtlTriePrintCallBackFunction (unsigned char *pucKey, void *pvData, va_list ap);

static int iUtlTrieFreeCallBackFunction (unsigned char *pucKey, void *pvData, va_list ap);


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlTrieCreate()

    Purpose:    Create a new trie structure. 

                This function will set up the new trie structures and return
                a pointer to those structures. This pointer needs to be passed
                back as part of every trie function. This allows one to set up
                lots of tries at once.

    Parameters: ppvUtlTrie      return pointer for the newly
                                created trie structure

    Globals:    none

    Returns:    UTL error code

*/
int iUtlTrieCreate
(
    void **ppvUtlTrie
)
{

    struct utlTrie      *putUtlTrie = NULL;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iUtlTrieCreate "); */


    /* Check the parameters */
    if ( ppvUtlTrie == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppvUtlTrie' parameter passed to 'iUtlTrieCreate'."); 
        return (UTL_ReturnParameterError);
    }


    /* Allocate the trie */
    if ( (putUtlTrie = (struct utlTrie *)s_malloc((size_t)(sizeof(struct utlTrie)))) == NULL ) {
        return (UTL_MemError);
    }

    /* Initialize the memory size */
    putUtlTrie->zMemorySize = sizeof(struct utlTrie);


    /* Initialize the free stuff */
    putUtlTrie->pputnUtlTrieNodesList = NULL;
    putUtlTrie->uiTrieNodeListEntryCount = 0;
    putUtlTrie->uiTrieNodeListLength = 0;
    putUtlTrie->uiTrieNodeFreeEntryCount = 0;


    /* Initialize the entry counters */
    putUtlTrie->uilEntryCount = 0;


    /* Set the return pointer */
    *ppvUtlTrie = (void *)putUtlTrie;


    return (UTL_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlTrieFree()

    Purpose:    Free the trie.

                This function will release all the memory resources currently
                held by this trie. After this the trie will be invalid.

    Parameters: pvUtlTrie       trie structure
                bFreeDatum      set to true if datum needs to be freed
                                when vTrieFree() is called

    Globals:    none

    Returns:    UTL error code

*/
int iUtlTrieFree
(
    void *pvUtlTrie,
    boolean bFreeDatum
)
{

    int                 iError = UTL_NoError;
    struct utlTrie      *putUtlTrie = (struct utlTrie *)pvUtlTrie;
    unsigned int        uiI = 0;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iUtlTrieFree"); */


    /* Check the parameters */
    if ( pvUtlTrie == NULL ) {
        iUtlLogDebug(UTL_LOG_CONTEXT, "Null 'pvUtlTrie' parameter passed to 'iUtlTrieFree'."); 
        return (UTL_TrieInvalidTrie);
    }


/*     iUtlTriePrintKeys(pvUtlTrie, false); */


    /* Free the datum if needed */
    if ( bFreeDatum == true ) {
        /* Traverse the trie and free the datum */
        iError = iUtlTrieLoop(putUtlTrie, NULL, (int (*)())iUtlTrieFreeCallBackFunction, NULL);
    }

    /* Free the allocated node list entries */
    if ( putUtlTrie->pputnUtlTrieNodesList != NULL ) {
        for ( uiI = 0; uiI < putUtlTrie->uiTrieNodeListEntryCount; uiI++ ) {
            s_free(putUtlTrie->pputnUtlTrieNodesList[uiI]);
        }
        s_free(putUtlTrie->pputnUtlTrieNodesList);
    }

    /* Free the trie */
    s_free(putUtlTrie);


    return (UTL_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlTrieAdd()

    Purpose:    Add a key to the trie. 

                This function will add the passed key in the trie. If
                the key does not exist, it will be inserted into the trie.
                A pointer to a pointer will be returned which may be used
                to store data about the key.

    Parameters: pvUtlTrie       trie structure
                pucKey          key to add
                pppvDatum       return pointer for the datum (optional)

    Globals:    none

    Returns:    UTL error code

*/
int iUtlTrieAdd
(
    void *pvUtlTrie,
    unsigned char *pucKey,
    void ***pppvDatum
)
{

    int                 iError = UTL_NoError;
    struct utlTrie      *putUtlTrie = (struct utlTrie *)pvUtlTrie;
    void                **ppvDatum = NULL;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "TrieAdd - pucKey [%s]", pucKey); */


    /* Check the parameters */
    if ( putUtlTrie == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvUtlTrie' parameter passed to 'iUtlTrieAdd'."); 
        return (UTL_TrieInvalidTrie);
    }

    if ( bUtlStringsIsStringNULL(pucKey) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pucKey' parameter passed to 'iUtlTrieAdd'."); 
        return (UTL_TrieInvalidKey);
    }


    /* Add the key */
    if ( (iError = iUtlTrieAddKeyToTrie(putUtlTrie, pucKey, &ppvDatum)) != UTL_NoError ) {
        return (iError);
    }


    /* Increment the number of entries if this is a new key */
    if ( *ppvDatum == NULL ) {
        putUtlTrie->uilEntryCount++;
    }


    /* Set the return pointer if it was set */
    if ( pppvDatum != NULL ) {
        *pppvDatum = ppvDatum;
    }


    return (UTL_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlTrieLookup()

    Purpose:    Look up a key in the trie. 

                This function will look for the passed key in the trie.
                If the key exists, a pointer to a pointer will be returned.

    Parameters: pvUtlTrie   trie structure
                pucKey      key to look up
                pppvDatum   return pointer for the datum (optional)

    Globals:    none

    Returns:    UTL error code

*/
int iUtlTrieLookup
(
    void *pvUtlTrie,
    unsigned char *pucKey,
    void ***pppvDatum
)
{

    int                 iError = UTL_NoError;
    struct utlTrie      *putUtlTrie = (struct utlTrie *)pvUtlTrie;
    void                **ppvDatum = NULL;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iUtlTrieLookup - pucKey [%s]", pucKey); */


    /* Check the parameters */
    if ( pvUtlTrie == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvUtlTrie' parameter passed to 'iUtlTrieLookup'."); 
        return (UTL_TrieInvalidTrie);
    }

    if ( bUtlStringsIsStringNULL(pucKey) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pucKey' parameter passed to 'iUtlTrieLookup'."); 
        return (UTL_TrieInvalidKey);
    }


    /* Look up the key */
    if ( (iError = iUtlTrieLookupKeyInTrie(putUtlTrie, pucKey, &ppvDatum)) != UTL_NoError ) {
        return (iError);
    }


    /* Set the return pointer if it was set */
    if ( pppvDatum != NULL ) {
        *pppvDatum = ppvDatum;
    }


    return (UTL_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlTrieLoop()

    Purpose:    Loop through the trie keys.

                This function will loop over each key in the trie and call
                the call back function with each key. All the parameters
                passed after the call back function name will be passed to
                the call back function. In addition the call back function
                will be passed a pointer to the key and a pointer to the 
                data.

                The declaration format for the call back function is:

                    lCallBackFunction(unsigned char *pucKey, void *pvData, va_list ap)

                The call back function should return 0 on success and non-0 on error.

    Parameters: pvUtlTrie                       trie
                pucKey                          key to start processing from (optional)
                (*iUtlTrieCallBackFunction)()   call back function pointer
                ...                             args (optional)

    Globals:    none

    Returns:    UTL error code

*/
int iUtlTrieLoop
(
    void *pvUtlTrie,
    unsigned char *pucKey,
    int (*iUtlTrieCallBackFunction)(),
    ...
)
{

    int                 iError = UTL_NoError;
    struct utlTrie      *putUtlTrie = (struct utlTrie *)pvUtlTrie;
    va_list             ap;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iUtlTrieLoop"); */


    /* Check the parameters */
    if ( pvUtlTrie == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvUtlTrie' parameter passed to 'iUtlTrieLoop'."); 
        return (UTL_TrieInvalidTrie);
    }

    if ( iUtlTrieCallBackFunction == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'iUtlTrieCallBackFunction' parameter passed to 'iUtlTrieLoop'."); 
        return (UTL_TrieInvalidCallBackFunction);
    }


    /* Call the looping function */
    va_start(ap, iUtlTrieCallBackFunction);
    iError = iUtlTrieLoopOverKeysInTrie(putUtlTrie, pucKey, (int (*)())iUtlTrieCallBackFunction, ap);
    va_end(ap);


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlTrieGetEntryCount()

    Purpose:    Return the number of entries in a trie.

                This function will return the number of entries added to the
                trie so far. This includes all entries added more than once.
                In fact it returns the number of times iUtlTrieAdd() has been 
                called.

    Parameters: pvUtlTrie       trie structure
                puiEntryCount   return pointer for the trie entries count

    Globals:    none

    Returns:    UTL error code

*/
int iUtlTrieGetEntryCount
(
    void *pvUtlTrie,
    unsigned int *puiEntryCount
)
{

    struct utlTrie      *putUtlTrie = (struct utlTrie *)pvUtlTrie;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iUtlTrieGetEntryCount"); */


    /* Check the parameters */
    if ( putUtlTrie == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvUtlTrie' parameter passed to 'iUtlTrieGetEntryCount'."); 
        return (UTL_TrieInvalidTrie);
    }

    if ( puiEntryCount == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'puiEntryCount' parameter passed to 'iUtlTrieGetEntryCount'."); 
        return (UTL_ReturnParameterError);
    }


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iUtlTrieGetEntryCount [%lu]", putUtlTrie->uilEntryCount); */


    /* Set the return pointer */
    *puiEntryCount = putUtlTrie->uilEntryCount;


    return (UTL_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlTriGetMemorySize()

    Purpose:    Return the current size of a trie in bytes.

    Parameters: pvUtlTrie       trie structure
                pzMemorySize    return pointer for the trie memory size

    Globals:    none

    Returns:    UTL error code


*/
int iUtlTriGetMemorySize
(
    void *pvUtlTrie,
    size_t *pzMemorySize
)
{

    struct utlTrie      *putUtlTrie = (struct utlTrie *)pvUtlTrie;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iUtlTriGetMemorySize"); */


    /* Check the parameters */
    if ( putUtlTrie == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvUtlTrie' parameter passed to 'iUtlTriGetMemorySize'."); 
        return (UTL_TrieInvalidTrie);
    }

    if ( pzMemorySize == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pzMemorySize' parameter passed to 'iUtlTriGetMemorySize'."); 
        return (UTL_ReturnParameterError);
    }


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iUtlTriGetMemorySize [%lu]", (unsigned long)putUtlTrie->pzMemorySize); */


    /* Set the return pointer */
    *pzMemorySize = putUtlTrie->zMemorySize;


    return (UTL_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlTriePrintKeys()

    Purpose:    Print all the keys in the trie.

                This function will loop over all the keys in a trie and 
                print them out to the log.

    Parameters: pvUtlTrie       trie structure
                bPrintData      set to true to print the data

    Globals:    none

    Returns:    UTL error code

*/
int iUtlTriePrintKeys
(
    void *pvUtlTrie,
    unsigned char *pucKey,
    boolean bPrintData
)
{

    int                 iError = UTL_NoError;
    struct utlTrie      *putUtlTrie = (struct utlTrie *)pvUtlTrie;


    /* Check the handle */
    if ( pvUtlTrie == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvUtlTrie' parameter passed to 'iUtlTriePrintKeys'."); 
        return (UTL_TrieInvalidTrie);
    }


    iError = iUtlTrieLoop(putUtlTrie, pucKey, (int (*)())iUtlTriePrintCallBackFunction, (int)bPrintData);


    return (iError);

}


/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlTrieAddKeyToTrie()

    Purpose:    Add the pucKey to the trie.

    Parameters: putUtlTrie      trie structure
                pucKey          key to add
                pppvDatum       return pointer for the datum

    Globals:    none

    Returns:    a pointer to the datum found, NULL on error

*/
static int iUtlTrieAddKeyToTrie
(
    struct utlTrie *putUtlTrie,
    unsigned char *pucKey,
    void ***pppvDatum
)
{

    struct utlTrieNode      *putnUtlTrieNodePtr = NULL;
    struct utlTrieNode      **pputnUtlTrieNodePtr = NULL;
    struct utlTrieNode      **pputnUtlTrieNodesListPtr = NULL;
    unsigned char           *pucKeyPtr = NULL;


    ASSERT(putUtlTrie != NULL);
    ASSERT(bUtlStringsIsStringNULL(pucKey) == false);
    ASSERT(pppvDatum != NULL);


    /* Set the pointers */
    pputnUtlTrieNodePtr = &putUtlTrie->putnUtlTrieRootNode;
    pucKeyPtr = pucKey;


    /* Navigate down the tree until we either hit the end or we find it */
    while ( (putnUtlTrieNodePtr = *pputnUtlTrieNodePtr) ) {

        /* Get the character - this is the bottleneck when indexing */
        int     iChar = (int)(*pucKeyPtr - putnUtlTrieNodePtr->ucSplitChar);

        /* Check this kid we are interested in */
        if ( iChar == 0 ) {
            if ( *pucKeyPtr == '\0' ) {
                /* We already have this key */
                *pppvDatum = (void **)&(putnUtlTrieNodePtr->putnUtlTrieNodeEqKid);
                return (UTL_NoError);
            }
            pucKeyPtr++;
            pputnUtlTrieNodePtr = &(putnUtlTrieNodePtr->putnUtlTrieNodeEqKid);
        }
        else if ( iChar < 0 ) {
            pputnUtlTrieNodePtr = &(putnUtlTrieNodePtr->putnUtlTrieNodeLowKid);
        }
        else {
            pputnUtlTrieNodePtr = &(putnUtlTrieNodePtr->putnUtlTrieNodeHighKid);
        }
    }


    /* We hit the end of the tree, so we need to add the rest of the key */


    while ( true ) {

        /* Check to see if we have enough space left in the current tree node */
        if ( putUtlTrie->uiTrieNodeFreeEntryCount <= 0 ) {

            /* Skip to the next tree node in the list */
            putUtlTrie->uiTrieNodeListEntryCount++;

            /* Extend the free tree node list if needed, we do it in bounds of UTL_TRIE_NODE_LIST_ENTRY_COUNT entries */
            if ( putUtlTrie->uiTrieNodeListEntryCount >= putUtlTrie->uiTrieNodeListLength ) {

                putUtlTrie->uiTrieNodeListLength += UTL_TRIE_NODE_LIST_ENTRY_COUNT;

                if ( (pputnUtlTrieNodesListPtr = (struct utlTrieNode **)s_realloc(putUtlTrie->pputnUtlTrieNodesList,
                        (size_t)(putUtlTrie->uiTrieNodeListLength * sizeof(struct utlTrieNode *)))) == NULL ) {
                    return (UTL_MemError);
                }

                putUtlTrie->pputnUtlTrieNodesList = pputnUtlTrieNodesListPtr;
                putUtlTrie->zMemorySize += UTL_TRIE_NODE_LIST_ENTRY_COUNT * sizeof(struct utlTrieNode *);
            }

            /* Create a new tree node of UTL_TRIE_NODE_ENTRY_COUNT entries */
            putUtlTrie->uiTrieNodeFreeEntryCount = UTL_TRIE_NODE_ENTRY_COUNT;

            if ( (putnUtlTrieNodePtr = (struct utlTrieNode *)s_malloc((size_t)(putUtlTrie->uiTrieNodeFreeEntryCount * sizeof(struct utlTrieNode))) ) == NULL ) {
                return (UTL_MemError);
            }

            putUtlTrie->pputnUtlTrieNodesList[putUtlTrie->uiTrieNodeListEntryCount - 1] = putnUtlTrieNodePtr;
            putUtlTrie->zMemorySize += putUtlTrie->uiTrieNodeFreeEntryCount * sizeof(struct utlTrieNode);
        }

        putUtlTrie->uiTrieNodeFreeEntryCount--;
        *pputnUtlTrieNodePtr = putUtlTrie->pputnUtlTrieNodesList[putUtlTrie->uiTrieNodeListEntryCount - 1] + putUtlTrie->uiTrieNodeFreeEntryCount;

        /* Dereference the trie node for convenience */
        putnUtlTrieNodePtr = *pputnUtlTrieNodePtr;

        /* Set the field of the new trie node */
        putnUtlTrieNodePtr->ucSplitChar = *pucKeyPtr;
        putnUtlTrieNodePtr->putnUtlTrieNodeLowKid = NULL;
        putnUtlTrieNodePtr->putnUtlTrieNodeEqKid = NULL;
        putnUtlTrieNodePtr->putnUtlTrieNodeHighKid = NULL;

        /* Return a pointer to the datum if this is the end of the string */
        if ( *pucKeyPtr == '\0' ) {
            *pppvDatum = (void **)&(putnUtlTrieNodePtr->putnUtlTrieNodeEqKid);
            return (UTL_NoError);
        }
        pucKeyPtr++;
        pputnUtlTrieNodePtr = &(putnUtlTrieNodePtr->putnUtlTrieNodeEqKid);
    }


    return (UTL_TrieAddKeyFailed);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlTrieLookupKeyInTrie()

    Purpose:    Look up the pucKey in the trie.

    Parameters: putUtlTrie      trie structure
                pucKey          key to lookup
                pppvDatum       return pointer for the datum

    Globals:    none

    Returns:    UTL error code

*/
static int iUtlTrieLookupKeyInTrie
(
    struct utlTrie *putUtlTrie,
    unsigned char *pucKey,
    void ***pppvDatum
)
{

    struct utlTrieNode      *putnUtlTrieNodePtr = NULL;
    unsigned char           *pucKeyPtr = NULL;


    ASSERT(putUtlTrie != NULL);
    ASSERT(bUtlStringsIsStringNULL(pucKey) == false);
    ASSERT(pppvDatum != NULL);


    /* Set the trie node */
    putnUtlTrieNodePtr = putUtlTrie->putnUtlTrieRootNode;

    /* Set the key pointer */
    pucKeyPtr = pucKey;

    while ( putnUtlTrieNodePtr != NULL ) {

        if ( *pucKeyPtr < putnUtlTrieNodePtr->ucSplitChar ) {
            putnUtlTrieNodePtr = putnUtlTrieNodePtr->putnUtlTrieNodeLowKid;
        }
        else if ( *pucKeyPtr == putnUtlTrieNodePtr->ucSplitChar )  {
            if ( *pucKeyPtr == '\0' ) {
                *pppvDatum = (void **)&(putnUtlTrieNodePtr->putnUtlTrieNodeEqKid);
                return (UTL_NoError);
            }
            pucKeyPtr++;
            putnUtlTrieNodePtr = putnUtlTrieNodePtr->putnUtlTrieNodeEqKid;
        }
        else {
            putnUtlTrieNodePtr = putnUtlTrieNodePtr->putnUtlTrieNodeHighKid;
        }
    }


    return (UTL_TrieKeyNotFound);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlTrieLoopOverKeysInTrie()

    Purpose:    Loop through the trie information.

                This function will loop over each key in the trie and call
                the passed call back function for every key. The va_list will
                be passed to the call back function along with a pointer to
                the key and a pointer to the data. 

    Parameters: putUtlTrie                      trie structure
                pucKey                          key to start processing from (optional)
                (*iUtlTrieCallBackFunction)()   call back function pointer
                ap                              arg list

    Globals:    none

    Returns:    UTL error code

*/
static int iUtlTrieLoopOverKeysInTrie
(
    struct utlTrie *putUtlTrie,
    unsigned char *pucKey,
    int (*iUtlTrieCallBackFunction)(),
    va_list ap
)
{

    int                     iError = UTL_NoError;
    struct utlTrieNode      *putnUtlTrieNodePtr = NULL;
    unsigned char           *pucKeyPtr = NULL;
    unsigned int            uiKeyLength = 0;

    unsigned char           pucKeyBuffer[UTL_TRIE_MAX_KEY_LENGTH + 1] = {'\0'};
    unsigned char           *pucKeyBufferPtr = pucKeyBuffer;


    ASSERT(putUtlTrie != NULL);
    ASSERT((bUtlStringsIsStringNULL(pucKey) == false) || (bUtlStringsIsStringNULL(pucKey) == true));
    ASSERT((iUtlTrieCallBackFunction != NULL) || (iUtlTrieCallBackFunction == NULL));


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iUtlTrieLoopOverKeysInTrie"); */


    /* Set the trie node and the saved trie node */
    putnUtlTrieNodePtr = putUtlTrie->putnUtlTrieRootNode;


    /* Find the trie node for the passed key, if one was passed */
    if ( bUtlStringsIsStringNULL(pucKey) == false ) {

        pucKeyPtr = pucKey;
        uiKeyLength = s_strlen(pucKey);
    
        while ( putnUtlTrieNodePtr != NULL ) {
    
            if ( *pucKeyPtr < putnUtlTrieNodePtr->ucSplitChar ) {
                putnUtlTrieNodePtr = putnUtlTrieNodePtr->putnUtlTrieNodeLowKid;
            }
            else if ( *pucKeyPtr == putnUtlTrieNodePtr->ucSplitChar )  {
    
                *pucKeyBufferPtr = *pucKeyPtr;
                *(pucKeyBufferPtr + 1) = '\0';
                
                if ( s_strncmp(pucKey, pucKeyBuffer, uiKeyLength) == 0 ) {
                    break;
                }

                pucKeyPtr++;
                pucKeyBufferPtr++;

                putnUtlTrieNodePtr = putnUtlTrieNodePtr->putnUtlTrieNodeEqKid;
            }
            else {
                putnUtlTrieNodePtr = putnUtlTrieNodePtr->putnUtlTrieNodeHighKid;
            }
        }
    }


    /* Call the looping function */
    iError = iUtlTrieLoopOverKeysInTrieNode(putnUtlTrieNodePtr, pucKey, uiKeyLength, pucKeyBuffer, &pucKeyBufferPtr, (int (*)())iUtlTrieCallBackFunction, ap);


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlTrieLoopOverKeysInTrieNode()

    Purpose:    Loop through the trie information.

                This function will loop over each key in the trie and call
                the passed call back function for every key. The va_list will
                be passed to the call back function along with a pointer to
                the key and a pointer to the data. 

    Parameters: putnUtlTrieNode                 trie node structure
                pucKey                          key to start processing from (optional)
                uiKeyLength                     key length - denormalization (optional)
                pucKeyBuffer                    buffer to build the key in
                ppucKeyBufferPtr                pointer to the current position in the buffer
                (*iUtlTrieCallBackFunction)()   call back function pointer
                ap                              arg list

    Globals:    none

    Returns:    UTL error code

*/
static int iUtlTrieLoopOverKeysInTrieNode
(
    struct utlTrieNode *putnUtlTrieNode,
    unsigned char *pucKey,
    unsigned int uiKeyLength,
    unsigned char *pucKeyBuffer,
    unsigned char **ppucKeyBufferPtr,
    int (*iUtlTrieCallBackFunction)(),
    va_list ap
)
{

    int     iError = UTL_NoError;


    ASSERT((putnUtlTrieNode != NULL) || (putnUtlTrieNode == NULL));
    ASSERT((bUtlStringsIsStringNULL(pucKey) == false) || (bUtlStringsIsStringNULL(pucKey) == true));
    ASSERT(uiKeyLength >= 0);
    ASSERT(pucKeyBuffer != NULL);
    ASSERT(ppucKeyBufferPtr != NULL);
    ASSERT((iUtlTrieCallBackFunction != NULL) || (iUtlTrieCallBackFunction == NULL));


    if ( putnUtlTrieNode == NULL ) {
        return (UTL_NoError);
    }


    if ( bUtlStringsIsStringNULL(pucKey) == false ) {
        if ( s_strncmp(pucKeyBuffer, pucKey, uiKeyLength) > 0 ) {
            return (UTL_NoError);
        }
    }


    if ( (iError = iUtlTrieLoopOverKeysInTrieNode(putnUtlTrieNode->putnUtlTrieNodeLowKid, pucKey, uiKeyLength, pucKeyBuffer, ppucKeyBufferPtr, iUtlTrieCallBackFunction, ap)) != UTL_NoError ) {
        return (iError);
    }


    **ppucKeyBufferPtr = putnUtlTrieNode->ucSplitChar;
    (*ppucKeyBufferPtr)++;
    

    /* Check that we have not burst the max key length */
    if ( (*ppucKeyBufferPtr - pucKeyBuffer) >  UTL_TRIE_MAX_KEY_LENGTH ) {
        return (UTL_TrieInvalidKeyLength);
    }


    if ( putnUtlTrieNode->ucSplitChar != '\0' ) {
    
        **ppucKeyBufferPtr = '\0';
    
        if ( (iError = iUtlTrieLoopOverKeysInTrieNode(putnUtlTrieNode->putnUtlTrieNodeEqKid, pucKey, uiKeyLength, pucKeyBuffer, ppucKeyBufferPtr, iUtlTrieCallBackFunction, ap)) != UTL_NoError ) {
            return (iError);
        }
    }
    else {
        if ( (iUtlTrieCallBackFunction != NULL) && ((pucKey == NULL) || (s_strncmp(pucKey, pucKeyBuffer, uiKeyLength) == 0)) ) {
            if ( (iError = iUtlTrieCallBackFunction(pucKeyBuffer, (void *)putnUtlTrieNode->putnUtlTrieNodeEqKid, ap)) != UTL_NoError ) {
                return (iError);
            }
        }
    }


    (*ppucKeyBufferPtr)--;
    **ppucKeyBufferPtr = '\0';

    if ( (iError = iUtlTrieLoopOverKeysInTrieNode(putnUtlTrieNode->putnUtlTrieNodeHighKid, pucKey, uiKeyLength, pucKeyBuffer, ppucKeyBufferPtr, iUtlTrieCallBackFunction, ap)) != UTL_NoError ) {
        return (iError);
    }


    return (UTL_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlTriePrintCallBackFunction()

    Purpose:    This function is passed as the call back function to iUtlTrieLoop()
                and gets called for every key in the trie. It will get passed
                pointers to the key currently being processed and to the data
                stored for that key. It will also get passed as a va_list, the 
                parameters that were specified in the call to iUtlTrieLoop().

    Parameters: pucKey      key
                pvData      data        
                ap          args (optional)

    Globals:    none

    Returns:    0 if successful, -1 on error
*/
static int iUtlTriePrintCallBackFunction
(
    unsigned char *pucKey,
    void *pvData,
    va_list ap
)
{

    va_list     ap_;


    ASSERT(bUtlStringsIsStringNULL(pucKey) == false);


    /* Get all our parameters, note that we make a copy of 'ap' */
    va_copy(ap_, ap);
    boolean bPrintData = (boolean)va_arg(ap_, int);
    va_end(ap_);

    /* Print the key/data depending on the flag */
    if ( bPrintData == true ) {
        printf("'%s' => '%s'\n", pucKey, (pvData != NULL) ? (unsigned char *)pvData : (unsigned char *)"(no data)");
    }
    else {
        printf("'%s'\n", pucKey);
    }


    return (0);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlTrieFreeCallBackFunction()

    Purpose:    This function is passed as the call back function to iUtlTrieLoop()
                and gets called for every key in the trie. It will get passed
                pointers to the key currently being processed and to the data
                stored for that key. It will also get passed as a va_list, the 
                parameters that were specified in the call to iUtlTrieLoop().

    Parameters: pucKey      key
                pvData      data        
                ap          arg (optional)

    Globals:    none

    Returns:    0 if successful, -1 on error
*/
static int iUtlTrieFreeCallBackFunction
(
    unsigned char *pucKey,
    void *pvData,
    va_list ap
)
{

    ASSERT(bUtlStringsIsStringNULL(pucKey) == false);


    /* Free the data if defined */
    if ( pvData != NULL ) {
        s_free(pvData);
    }


    return (0);
}


/*---------------------------------------------------------------------------*/
