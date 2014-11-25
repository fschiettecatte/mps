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

    Module:     dict.c

    Author:     Francois Schiettecatte (FS Consulting LLC.)

    Created:    31 Aug 2010

    Purpose:    This implements a disk based 2-level btree dictionary. This is 
                useful for dictionaries which would too big for trie.c/h.

                The dictionary is created specifying the key size.
                
                The file is laid out in three parts, first the header, then
                the key blocks and finally the super block.
                
                    [dictionary file] => [dictionary header][key blocks, ...][super block]

                    
                The dictionary header stores the key length, the super block ID
                and the super block entry count
                
                    [dictionary header] => [key length][super block ID][super block entry count]

                    key length                          2 bytes
                    super block ID                      8 bytes
                    super block entry count             4 bytes


                Following that there are any number of key blocks, each key block starts
                with a key block length, followed by any number of entries:
                
                    [key block] => [key block length][key blocks entries, ...]
                    
                    key block length                    compressed int

                Key block entries contain the key indent, the key snippet,
                the entry length and the entry data:

                    [key blocks entry]    => [key indent][key snippet][entry length][entry data]

                    key indent                          compressed int
                    key snippet                         NULL terminated string 
                    entry length                        compressed int
                    entry data                          binary data

                The entry data is a chunk of binary data specified by the client app.
                
                
                The super block consists of any number of entries:

                    [super block entry] => [key][key block ID]
                    
                    key                                 NULL terminated string, fixed field of 'key length'
                    key block ID                        8 bytes


                Dictionary creation functions:

                    iUtlDictCreate()
                    iUtlDictAddKey()
                    iUtlDictClose()


                Dictionary access functions:

                    iUtlDictOpen()
                    iUtlDictGetEntry()
                    iUtlDictProcessEntry()
                    iUtlDictProcessEntryList()
                    iUtlDictClose()

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
#define UTL_LOG_CONTEXT                     (unsigned char *)"com.fsconsult.mps.src.utils.dict2"


/*---------------------------------------------------------------------------*/


/*
** Defines
*/


/* Header defines */
#define    UTL_DICT_HEADER_KEY_LENGTH_SIZE                  (2)
#define    UTL_DICT_HEADER_SUPER_BLOCK_ID_SIZE              (8)
#define    UTL_DICT_HEADER_SUPER_BLOCK_ENTRY_COUNT_SIZE     (4)

#define UTL_DICT_HEADER_LENGTH                              (UTL_DICT_HEADER_KEY_LENGTH_SIZE + \
                                                                    UTL_DICT_HEADER_SUPER_BLOCK_ID_SIZE + \
                                                                    UTL_DICT_HEADER_SUPER_BLOCK_ENTRY_COUNT_SIZE)

/* Super block defines */
#define    UTL_DICT_SUPER_BLOCK_KEY_BLOCK_ID_SIZE           (8)



/* The first and last keys in the dictionary, the character representations are for optimization */
#define UTL_DICT_FIRST_KEY_STRING                           (unsigned char *)"\40"              /* Ascii 32 */
#define UTL_DICT_LAST_KEY_STRING                            (unsigned char *)"\377\377"         /* Ascii 255 */
#define UTL_DICT_FIRST_KEY_CHARACTER                        (32)                                /* Ascii 32 */
#define UTL_DICT_LAST_KEY_CHARACTER                         (255)                               /* Ascii 255 */


#define    UTL_DICT_KEY_MAXIMUM_LENGTH                      (1023)                              /* Exclude the terminating NULL */
#define    UTL_DICT_KEY_BLOCK_ENTRY_MAXIMUM_COUNT           (250)


/* Dictionary mode */
#define    UTL_DICT_MODE_INVALID                            (0)
#define    UTL_DICT_MODE_WRITE                              (1)
#define    UTL_DICT_MODE_READ                               (2)


/*---------------------------------------------------------------------------*/


/*
** Structures
*/


struct utlDictWrite {

    unsigned int            uiKeyCount;                                     /* Key count */

    unsigned char           *pucKeyBlock;                                   /* Key block */
    unsigned int            uiKeyBlockLength;                               /* Key block length */
    unsigned int            uiKeyBlockCapacity;                             /* Key block capacity */

    unsigned char           pucSavedKey[UTL_DICT_KEY_MAXIMUM_LENGTH + 1];   /* Saved key */

    unsigned char           pucDictFilePath[UTL_FILE_PATH_MAX + 1];         /* Saved dictionary file path */

};


/* Dict structure */
struct utlDict {

    unsigned int            uiMode;                                         /* Mode */
    FILE                    *pfFile;                                        /* Dictionary file */

    unsigned int            uiKeyLength;                                    /* Key length - excludes the terminating NULL */
    unsigned long           ulSuperBlockID;                                 /* Super block ID */
    unsigned int            uiSuperBlockEntryCount;                         /* Super block entry count */
    
    unsigned char           *pucSuperBlock;                                 /* Super block pointer */
    unsigned int            uiSuperBlockEntryLength;                        /* Super block entry length */

    void                    *pvFile;                                        /* Data pointer if the dictionary file if memory mapped */
    size_t                  zFileLength;                                    /* Data length if the dictionary file if memory mapped */

    struct utlDictWrite     *pudwDictWrite;                                 /* Dictionary write structure - allocated when in write mode */

};


/*---------------------------------------------------------------------------*/


/*
** Private function prototypes
*/

static int iUtlDictMapFile (struct utlDict *pudUtlDict);


static int iUtlDictUnMapFile (struct utlDict *pudUtlDict);


static int iUtlDictGetSuperBlockEntry (struct utlDict *pudUtlDict,
        unsigned char *pucKey, unsigned char **ppucSuperBlockEntry);


static int iUtlDictGetKeyBlockEntryData (struct utlDict *pudUtlDict,
        unsigned long ulKeyBlockID, unsigned char *pucKey,
        void **ppvEntryData, unsigned int *puiEntryLength);


static int iUtlDictProcessKeyBlockEntryList (struct utlDict *pudUtlDict,
        unsigned long ulKeyBlockID, unsigned char *pucKey,
        int (*iUtlDictCallBackFunction)(), va_list ap);


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlDictCreate()

    Purpose:    Create a new dictionary.

    Parameters: pucDictFilePath     dictionary file path
                uiDictKeyLength     dictionary key length
                ppvUtlDict          return pointer for the dictionary structure

    Globals:    none

    Returns:    UTL error code

*/
int iUtlDictCreate
(
    unsigned char *pucDictFilePath,
    unsigned int uiDictKeyLength,
    void **ppvUtlDict
)
{

    long                iError = UTL_NoError;
    struct utlDict      *pudUtlDict = NULL;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iUtlDictCreate - pucDictFilePath: [%s]", pucDictFilePath); */


    /* Check the parameters */
    if ( bUtlStringsIsStringNULL(pucDictFilePath) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'pucDictFilePath' parameter passed to 'iUtlDictCreate'."); 
        return (UTL_DictInvalidFilePath);
    }

    if ( (uiDictKeyLength <= 0) || (uiDictKeyLength > UTL_DICT_KEY_MAXIMUM_LENGTH) ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiDictKeyLength' parameter passed to 'iUtlDictCreate'."); 
        return (UTL_DictInvalidKeyLength);
    }

    if ( ppvUtlDict == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppvUtlDict' parameter passed to 'iUtlDictCreate'."); 
        return (UTL_ReturnParameterError);
    }


    /* Allocate a dictionary structure */
    if ( (pudUtlDict = (struct utlDict *)s_malloc((size_t)(sizeof(struct utlDict)))) == NULL ) {
        return (UTL_MemError);
    }

    /* Initialize all the fields in the dictionary structure */
    pudUtlDict->uiMode = UTL_DICT_MODE_WRITE;
    pudUtlDict->pfFile = NULL;
    pudUtlDict->uiKeyLength = uiDictKeyLength + 1;    /* Includes the terminating NULL */
    pudUtlDict->ulSuperBlockID = 0;
    pudUtlDict->uiSuperBlockEntryCount = 0;
    pudUtlDict->pucSuperBlock = NULL;
    pudUtlDict->uiSuperBlockEntryLength = pudUtlDict->uiKeyLength + UTL_DICT_SUPER_BLOCK_KEY_BLOCK_ID_SIZE;


    /* Allocate a dictionary write structure */
    if ( (pudUtlDict->pudwDictWrite = (struct utlDictWrite *)s_malloc((size_t)(sizeof(struct utlDictWrite)))) == NULL ) {
        return (UTL_MemError);
    }

    /* Initialize all the fields in the dictionary write structure */
    pudUtlDict->pudwDictWrite->uiKeyCount = 0;
    pudUtlDict->pudwDictWrite->pucKeyBlock = NULL;
    pudUtlDict->pudwDictWrite->uiKeyBlockLength = 0;
    pudUtlDict->pudwDictWrite->uiKeyBlockCapacity = 0;
    pudUtlDict->pudwDictWrite->pucSavedKey[0] = '\0';
    s_strnncpy(pudUtlDict->pudwDictWrite->pucDictFilePath, pucDictFilePath, UTL_FILE_PATH_MAX + 1);


    /* Create the dictionary file - 'w+' so we can access it while we are creating it,
    ** which we will do when we write the header which contains the super block ID
    ** when we are done creating the dictionary (note that we do not allow access 
    ** via the iUtlDictGetEntry() functions)
    */
    if ( (pudUtlDict->pfFile = s_fopen(pucDictFilePath, "w+")) == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to create the dictionary, dictionary file path: '%s'.", pucDictFilePath);
        iError = UTL_DictCreateFailed;
        goto bailFromiUtlDictCreate;
    }
    

    /* Leave room for the dictionary header at the start of the dictionary file */
    if ( s_fseek(pudUtlDict->pfFile, UTL_DICT_HEADER_LENGTH, SEEK_SET) == -1 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to seek past the dictionary header in the dictionary file."); 
        return (UTL_DictCreateFailed);
    }


    /* Add the first key - no entry data */
    if ( (iError = iUtlDictAddEntry((void *)pudUtlDict, UTL_DICT_FIRST_KEY_STRING, NULL, 0)) != UTL_NoError ) {
        goto bailFromiUtlDictCreate;
    }



    /* Bail label */
    bailFromiUtlDictCreate:
    
    /* Handle errors */
    if ( iError != UTL_NoError ) {
    
        /* Close the dict */
        if ( pudUtlDict != NULL ) {
            iUtlDictClose((void *)pudUtlDict);
        }
        
        /* Remove the file */
        s_remove(pucDictFilePath);

        /* Set the return pointer */
        *ppvUtlDict = NULL;
    }
    else {

        /* Set the return pointer */
        *ppvUtlDict = (void *)pudUtlDict;
    }


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlDictAddEntry()

    Purpose:    Add a dictionary entry.

    Parameters: pvUtlDict           dictionary structure
                pucDictKey          dictionary key
                pvDictEntryData     dictionary entry data (optional)
                uiDictEntryLength   dictionary entry length (optional)

    Globals:    none

    Returns:    UTL error code 

*/
int iUtlDictAddEntry
(
    void *pvUtlDict,
    unsigned char *pucDictKey,
    void *pvDictEntryData,
    unsigned int uiDictEntryLength
)
{

    struct utlDict      *pudUtlDict = (struct utlDict *)pvUtlDict;
    unsigned char       *pucDictKeyDeltaPtr = NULL;
    unsigned char       *pucSavedKeyPtr = NULL;
    unsigned int        uiDictKeyDeltaLength = 0;
    unsigned int        uiDictEntryLengthSize = 0;
    unsigned int        uiDictKeyIndentLength = 0;
    unsigned int        uiKeyIndentLengthSize = 0;
    unsigned int        uiKeyBlockEntryLength = 0;
    unsigned char       *pucKeyBlockEntryPtr = NULL;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iUtlDictAddEntry - pucDictKey: [%s]", pucDictKey); */


    /* Check the parameters */
    if ( pvUtlDict == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvUtlDict' parameter passed to 'iUtlDictAddEntry'."); 
        return (UTL_DictInvalidDict);
    }

    if ( bUtlStringsIsStringNULL(pucDictKey) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pucDictKey' parameter passed to 'iUtlDictAddEntry'."); 
        return (UTL_DictInvalidKey);
    }

    if ( s_strlen(pucDictKey) > (pudUtlDict->uiKeyLength - 1) ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'pucDictKey' parameter passed to 'iUtlDictAddEntry'."); 
        return (UTL_DictInvalidKey);
    }

    if ( s_strcmp(pudUtlDict->pudwDictWrite->pucSavedKey, pucDictKey) > 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'pucDictKey' parameter passed to 'iUtlDictAddEntry'."); 
        return (UTL_DictInvalidKey);
    }

    if ( ((pvDictEntryData == NULL) && (uiDictEntryLength > 0)) || ((pvDictEntryData != NULL) && (uiDictEntryLength == 0)) ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'pvDictEntryData' and 'uiDictEntryLength' parameters passed to 'iUtlDictAddEntry'."); 
        return (UTL_DictInvalidEntry);
    }



    /* Check that we are in write mode */
    if ( pudUtlDict->uiMode != UTL_DICT_MODE_WRITE ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to add an entry to the dictionary, invalid dictionary mode: %u.", pudUtlDict->uiMode); 
        return (UTL_DictInvalidMode);
    }
    


    /* Extend the super block if the key count is 0 */
    if ( pudUtlDict->pudwDictWrite->uiKeyCount == 0 ) {
    
        unsigned int    uiSuperBlockCapacity = 0;
        unsigned char    *pucSuperBlockPtr = NULL;

        /* Get the new super block capacity */
        uiSuperBlockCapacity = pudUtlDict->uiSuperBlockEntryLength * (pudUtlDict->uiSuperBlockEntryCount + 1);

        /* Reallocate the super block */
        if ( (pucSuperBlockPtr = (unsigned char *)s_realloc(pudUtlDict->pucSuperBlock, (size_t)(sizeof(unsigned char) * uiSuperBlockCapacity))) == NULL ) {
            return (UTL_MemError);
        }
        
        /* Hand over the super block pointer */
        pudUtlDict->pucSuperBlock = pucSuperBlockPtr;
        
        /* Increment the super block entry count */
        pudUtlDict->uiSuperBlockEntryCount++;

        /* Set the super block pointer to the start of the new super block entry */
        pucSuperBlockPtr = pudUtlDict->pucSuperBlock + (pudUtlDict->uiSuperBlockEntryLength * (pudUtlDict->uiSuperBlockEntryCount - 1));

        /* Copy the key to the super block entry */
        s_strnncpy(pucSuperBlockPtr, pucDictKey, pudUtlDict->uiKeyLength - 1);
    }
    

    
    /* Generate the dictionary key delta, this is done by comparing it with
    ** the saved key, we only store the difference at the end along with an 
    ** indent length, for example:
    **
    **        computer
    **        computing
    **
    ** Both share the same root, 'comput', so we store 'computing' as 'ing' with
    ** an indent of 6. This encoding tells us that we need to get the first 
    ** 6 characters of the previous key and append the 'ing' to reconstitute
    ** 'computing'.
    */
    pucDictKeyDeltaPtr = pucDictKey;
    pucSavedKeyPtr = pudUtlDict->pudwDictWrite->pucSavedKey;
    uiDictKeyIndentLength = 0;

    /* See how many characters the two keys have in common */
    while ( (*pucSavedKeyPtr != '\0') && (*pucDictKeyDeltaPtr != '\0') && (*pucSavedKeyPtr == *pucDictKeyDeltaPtr) ) {
        pucSavedKeyPtr++;
        pucDictKeyDeltaPtr++;
        uiDictKeyIndentLength++;
    }

    /* Save this dict key, this becomes our saved key next time around */
    s_strnncpy(pudUtlDict->pudwDictWrite->pucSavedKey, pucDictKey, pudUtlDict->uiKeyLength - 1);
/*     s_strcpy(pudUtlDict->pudwDictWrite->pucSavedKey, pucDictKey); */
/*     s_strnncpy(pucSavedKeyPtr, pucDictKeyDeltaPtr, pudUtlDict->uiKeyLength - 1); */
/*     s_strcpy(pucSavedKeyPtr, pucDictKeyDeltaPtr); */

    /* Get the compressed size of the dict key indent length */
    UTL_NUM_GET_COMPRESSED_UINT_SIZE(uiDictKeyIndentLength, uiKeyIndentLengthSize);
    
    /* Get the dict key delta length - include terminating NULL */
    uiDictKeyDeltaLength = s_strlen(pucDictKeyDeltaPtr) + 1;

    /* Get the compressed size of the dict key entry length */
    UTL_NUM_GET_COMPRESSED_UINT_SIZE(uiDictEntryLength, uiDictEntryLengthSize);
    
    /* Work out the key block entry length needed */
    uiKeyBlockEntryLength = uiKeyIndentLengthSize + uiDictKeyDeltaLength + uiDictEntryLengthSize + uiDictEntryLength;


    /* Extend the key block if needed */
    if ( (pudUtlDict->pudwDictWrite->uiKeyBlockLength + uiKeyBlockEntryLength) >= pudUtlDict->pudwDictWrite->uiKeyBlockCapacity ) {
    
        unsigned int     uiKeyBlockCapacity = 0;
        unsigned char    *pucKeyBlockPtr = NULL;

        /* Get the new key block capacity, double the existing capacity and add enough space for the new key block entry */
        uiKeyBlockCapacity = (pudUtlDict->pudwDictWrite->uiKeyBlockCapacity * 2) + uiKeyBlockEntryLength;

        /* Reallocate the key block */
        if ( (pucKeyBlockPtr = (unsigned char *)s_realloc(pudUtlDict->pudwDictWrite->pucKeyBlock, (size_t)(sizeof(unsigned char) * uiKeyBlockCapacity))) == NULL ) {
            return (UTL_MemError);
        }

        /* Hand over the key block pointer */
        pudUtlDict->pudwDictWrite->pucKeyBlock = pucKeyBlockPtr;

        /* Set the key block capacity */
        pudUtlDict->pudwDictWrite->uiKeyBlockCapacity = uiKeyBlockCapacity;
    }


    /* Set the key block entry pointer */
    pucKeyBlockEntryPtr = pudUtlDict->pudwDictWrite->pucKeyBlock + pudUtlDict->pudwDictWrite->uiKeyBlockLength;

    /* Write the dict key indent length */
    UTL_NUM_WRITE_COMPRESSED_UINT(uiDictKeyIndentLength, pucKeyBlockEntryPtr);

    /* Copy the dict key delta */
    s_strcpy(pucKeyBlockEntryPtr, pucDictKeyDeltaPtr);

    /* Move our pointer past the end of the dict key delta - which includes the terminating NULL*/
    pucKeyBlockEntryPtr += uiDictKeyDeltaLength;

    /* Write  the dict entry length */
    UTL_NUM_WRITE_COMPRESSED_UINT(uiDictEntryLength, pucKeyBlockEntryPtr);

    /* Copy the dict entry data and move the key block entry pointer past the end of the data */
    if ( uiDictEntryLength > 0 ) {
        s_memcpy(pucKeyBlockEntryPtr, pvDictEntryData, uiDictEntryLength);
        pucKeyBlockEntryPtr += uiDictEntryLength;
    }

    /* Update the key block length */
    pudUtlDict->pudwDictWrite->uiKeyBlockLength = pucKeyBlockEntryPtr - pudUtlDict->pudwDictWrite->pucKeyBlock;

    /* Increment the key count */
    pudUtlDict->pudwDictWrite->uiKeyCount++;


    /* Save the key block if we have hit the key block maximum entry count
    ** or if this is the last key (which gets added when we done adding keys
    ** to the dictionary)
    */
    if ( (pudUtlDict->pudwDictWrite->uiKeyCount > UTL_DICT_KEY_BLOCK_ENTRY_MAXIMUM_COUNT) || (s_strcmp(pucDictKey, UTL_DICT_LAST_KEY_STRING) == 0) ) {
    
        unsigned long   ulKeyBlockID = 0;
        unsigned char   *pucSuperBlockPtr = NULL;
        unsigned char   pucKeyBlockLengthBuffer[UTL_NUM_UINT_MAX_SIZE];
        unsigned char   *pucKeyBlockLengthBufferPtr = pucKeyBlockLengthBuffer;
        
        /* Set the key block ID from the current dict file position */
        ulKeyBlockID = s_ftell(pudUtlDict->pfFile);

        /* Write the key block length */
        pucKeyBlockLengthBufferPtr = pucKeyBlockLengthBuffer;
        UTL_NUM_WRITE_COMPRESSED_UINT(pudUtlDict->pudwDictWrite->uiKeyBlockLength, pucKeyBlockLengthBufferPtr);
        
        /* Write out the key block length to the dict file */
        if ( s_fwrite(pucKeyBlockLengthBuffer, pucKeyBlockLengthBufferPtr - pucKeyBlockLengthBuffer, 1, pudUtlDict->pfFile) != 1 ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to write the dictionary key block header to the dict file."); 
            return (UTL_DictWriteFailed);
        }
        
        /* Write out the key block to the dict file */
        if ( s_fwrite(pudUtlDict->pudwDictWrite->pucKeyBlock, pudUtlDict->pudwDictWrite->uiKeyBlockLength, 1, pudUtlDict->pfFile) != 1 ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to write the dictionary key block to the dict file."); 
            return (UTL_DictWriteFailed);
        }
        
        /* Set the super block pointer to the end of the last super block entry, where we write the key block ID */
        pucSuperBlockPtr = pudUtlDict->pucSuperBlock + (pudUtlDict->uiSuperBlockEntryLength * (pudUtlDict->uiSuperBlockEntryCount - 1)) + pudUtlDict->uiKeyLength;

        /* Write the key block ID to the super block */
        UTL_NUM_WRITE_ULONG(ulKeyBlockID, UTL_DICT_SUPER_BLOCK_KEY_BLOCK_ID_SIZE, pucSuperBlockPtr);

        /* Reset the key block length */
        pudUtlDict->pudwDictWrite->uiKeyBlockLength = 0;

        /* Reset the key count */
        pudUtlDict->pudwDictWrite->uiKeyCount = 0;

        /* Null the saved key */
        pudUtlDict->pudwDictWrite->pucSavedKey[0] = '\0';
    }


    /* Save the super block and write the super block ID to the header
    ** if this is the last key
    */
    if ( s_strcmp(pucDictKey, UTL_DICT_LAST_KEY_STRING) == 0 ) {

        unsigned long   ulSuperBlockID = 0;
        unsigned long   ulSuperBlockLength = 0;
        unsigned char   pucDictHeader[UTL_DICT_HEADER_LENGTH];
        unsigned char   *pucDictHeaderPtr = pucDictHeader;
        
        /* Get the super block ID from the current dictionary file position */
        ulSuperBlockID = s_ftell(pudUtlDict->pfFile);

        /* Get the super block length */
        ulSuperBlockLength = pudUtlDict->uiSuperBlockEntryLength * pudUtlDict->uiSuperBlockEntryCount;

        /* Write out the super block to the dictionary file */
        if ( s_fwrite(pudUtlDict->pucSuperBlock, ulSuperBlockLength, 1, pudUtlDict->pfFile) != 1 ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to write the dictionary super block to the dictionary file."); 
            return (UTL_DictWriteFailed);
        }

        /* Write the dictionary header */
        pucDictHeaderPtr = pucDictHeader;
        UTL_NUM_WRITE_UINT(pudUtlDict->uiKeyLength, UTL_DICT_HEADER_KEY_LENGTH_SIZE, pucDictHeaderPtr);
        UTL_NUM_WRITE_ULONG(ulSuperBlockID, UTL_DICT_HEADER_SUPER_BLOCK_ID_SIZE, pucDictHeaderPtr);
        UTL_NUM_WRITE_UINT(pudUtlDict->uiSuperBlockEntryCount, UTL_DICT_HEADER_SUPER_BLOCK_ENTRY_COUNT_SIZE, pucDictHeaderPtr);
        
        /* Seek to the start the dictionary file */
        if ( s_fseek(pudUtlDict->pfFile, 0, SEEK_SET) == -1 ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to seek to the dictionary header in the dictionary file."); 
            return (UTL_DictWriteFailed);
        }
        
        /* Write out the dictionary header to the dict file */
        if ( s_fwrite(pucDictHeader, UTL_DICT_HEADER_LENGTH, 1, pudUtlDict->pfFile) != 1 ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to write the dictionary header to the dictionary file."); 
            return (UTL_DictWriteFailed);
        }
    }


    return (UTL_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlDictClose()

    Purpose:    Close the dictionary.

    Parameters: pvUtlDict       dictionary structure

    Globals:    none

    Returns:    UTL error code 

*/
int iUtlDictClose
(
    void *pvUtlDict
)
{

    int                 iError = UTL_NoError;
    struct utlDict      *pudUtlDict = (struct utlDict *)pvUtlDict;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iUtlDictClose"); */


    /* Check the parameters */
    if ( pvUtlDict == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvUtlDict' parameter passed to 'iUtlDictClose'."); 
        return (UTL_DictInvalidDict);
    }



    /* Need to add the last key if we are in write mode - no entry data */
    if ( pudUtlDict->uiMode == UTL_DICT_MODE_WRITE ) {
        if ( (iError = iUtlDictAddEntry((void *)pudUtlDict, UTL_DICT_LAST_KEY_STRING, NULL, 0)) != UTL_NoError ) {
            goto bailFromiUtlDictClose;
        }
    }



    /* Bail label */
    bailFromiUtlDictClose:
    
    /* Handle errors */
    if ( iError != UTL_NoError ) {
    
        /* Remove the dictionary file if we are in write mode and it is available */
        if ( (pudUtlDict->uiMode == UTL_DICT_MODE_WRITE) && (pudUtlDict->pudwDictWrite != NULL) ) {
            s_remove(pudUtlDict->pudwDictWrite->pucDictFilePath);
        }

    }

    /* Unmap the dictionary file, ignore errors */
    iUtlDictUnMapFile(pudUtlDict);

    /* Close the dictionary file */
    s_fclose(pudUtlDict->pfFile);

    /* Finally release the dictionary structure */
    s_free(pudUtlDict->pudwDictWrite);
    s_free(pudUtlDict);


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlDictOpen()

    Purpose:    Open a dictionary.

    Parameters: pucDictFilePath     dictionary file path
                ppvUtlDict          return pointer for the dictionary structure

    Globals:    none

    Returns:    UTL error code 

*/
int iUtlDictOpen
(
    unsigned char *pucDictFilePath,
    void **ppvUtlDict
)
{

    long                iError = UTL_NoError;
    unsigned char       *pucDictHeaderPtr = NULL;
    struct utlDict      *pudUtlDict = NULL;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iUtlDictOpen - pucDictFilePath: [%s]", pucDictFilePath); */


    /* Check the parameters */
    if ( bUtlStringsIsStringNULL(pucDictFilePath) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'pucDictFilePath' parameter passed to 'iUtlDictOpen'."); 
        return (UTL_DictInvalidFilePath);
    }

    if ( ppvUtlDict == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppvUtlDict' parameter passed to 'iUtlDictOpen'."); 
        return (UTL_ReturnParameterError);
    }



    /* Allocate a dictionary structure */
    if ( (pudUtlDict = (struct utlDict *)s_malloc((size_t)(sizeof(struct utlDict)))) == NULL ) {
        return (UTL_MemError);
    }

    /* Set all the fields in the dictionary structure */
    pudUtlDict->uiMode = UTL_DICT_MODE_READ;
    pudUtlDict->pfFile = NULL;
    pudUtlDict->ulSuperBlockID = 0;
    pudUtlDict->uiKeyLength = 0;
    pudUtlDict->uiSuperBlockEntryCount = 0;
    pudUtlDict->pucSuperBlock = NULL;
    pudUtlDict->uiSuperBlockEntryLength = 0;


    /* Open the dict file */
    if ( (pudUtlDict->pfFile = s_fopen(pucDictFilePath, "r")) == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to open the dictionary, dictionary file path: '%s'.", pucDictFilePath); 
        return (UTL_DictOpenFailed);
    }


    /* Map the dictionary file */
    if ( (iError = iUtlDictMapFile(pudUtlDict)) != UTL_NoError ) {
        return (iError);
    }


    /* Read the dictionary header */
    pucDictHeaderPtr = (unsigned char *)pudUtlDict->pvFile;
    UTL_NUM_READ_UINT(pudUtlDict->uiKeyLength, UTL_DICT_HEADER_KEY_LENGTH_SIZE, pucDictHeaderPtr);
    UTL_NUM_READ_ULONG(pudUtlDict->ulSuperBlockID, UTL_DICT_HEADER_SUPER_BLOCK_ID_SIZE, pucDictHeaderPtr);
    UTL_NUM_READ_UINT(pudUtlDict->uiSuperBlockEntryCount, UTL_DICT_HEADER_SUPER_BLOCK_ENTRY_COUNT_SIZE, pucDictHeaderPtr);


    /* Set the super block */
    pudUtlDict->pucSuperBlock = (unsigned char *)pudUtlDict->pvFile + pudUtlDict->ulSuperBlockID;

    /* Set the super block entry length */
    pudUtlDict->uiSuperBlockEntryLength = pudUtlDict->uiKeyLength + UTL_DICT_SUPER_BLOCK_KEY_BLOCK_ID_SIZE;


    /* Set the return pointer */
    *ppvUtlDict = (void *)pudUtlDict;


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlDictGetEntry()

    Purpose:    Get a key from the dictionary

    Parameters: pvUtlDict               dictionary structure
                pucDictKey              dictionary key
                ppvDictEntryData        return pointer for the dictionary entry data
                puiDictEntryLength      return pointer for the dictionary entry length

    Globals:    none

    Returns:    UTL error code 

*/
int iUtlDictGetEntry
(
    void *pvUtlDict,
    unsigned char *pucDictKey, 
    void **ppvDictEntryData, 
    unsigned int *puiDictEntryLength
)
{

    int                 iError = UTL_NoError;
    struct utlDict      *pudUtlDict = (struct utlDict *)pvUtlDict;
    unsigned char       *pucSuperBlockEntry = NULL;
    unsigned char       *pucSuperBlockEntryPtr = NULL;
    unsigned long       ulKeyBlockID = 0;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iUtlDictGetEntry - pucDictKey: [%s]", pucDictKey); */


    /* Check the parameters */
    if ( pvUtlDict == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvUtlDict' parameter passed to 'iUtlDictGetEntry'."); 
        return (UTL_DictInvalidDict);
    }

    if ( bUtlStringsIsStringNULL(pucDictKey) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'pucDictKey' parameter passed to 'iUtlDictGetEntry'."); 
        return (UTL_DictInvalidKey);
    }

    if ( ppvDictEntryData == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppvDictEntryData' parameter passed to 'iUtlDictGetEntry'."); 
        return (UTL_ReturnParameterError);
    }

    if ( puiDictEntryLength == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'puiDictEntryLength' parameter passed to 'iUtlDictGetEntry'."); 
        return (UTL_ReturnParameterError);
    }



    /* Get the super block entry for this key */
    if ( (iError = iUtlDictGetSuperBlockEntry(pudUtlDict, pucDictKey, &pucSuperBlockEntry)) != UTL_NoError ) {
        return (iError);
    }


    /* Get a pointer to the key block ID in the super block entry */
    pucSuperBlockEntryPtr = pucSuperBlockEntry + pudUtlDict->uiKeyLength;

    /* Read the key block ID from the super block entry */
    UTL_NUM_READ_ULONG(ulKeyBlockID, UTL_DICT_SUPER_BLOCK_KEY_BLOCK_ID_SIZE, pucSuperBlockEntryPtr);


    /* Find the key in the key block entry */
    iError = iUtlDictGetKeyBlockEntryData(pudUtlDict, ulKeyBlockID, pucDictKey, ppvDictEntryData, puiDictEntryLength);


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlDictProcessEntry()

    Purpose:    Process a key from the dictionary

                The declaration format for the call back function is:

                    iUtlDictCallBackFunction(unsigned char *pucKey, void *pvEntryData, 
                        unsigned int uiEntryLength, va_list ap)

                The call-back function needs to return 0 to keep processing or
                non-zero to stop processing.

    Parameters: pvUtlDict                   dictionary structure
                pucDictKey                  dictionary key
                iUtlDictCallBackFunction    call-back function
                ...                         args (optional)

    Globals:    none

    Returns:    UTL error code 

*/
int iUtlDictProcessEntry
(
    void *pvUtlDict,
    unsigned char *pucDictKey, 
    int (*iUtlDictCallBackFunction)(),
    ...
)
{

    int                 iError = UTL_NoError;
    int                 iStatus = 0;
    struct utlDict      *pudUtlDict = (struct utlDict *)pvUtlDict;
    void                *pvEntryData = NULL;
    unsigned int        uiEntryLength = 0;
    va_list             ap;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iUtlDictProcessEntry - pucDictKey: [%s]", pucDictKey); */


    /* Check the parameters */
    if ( pvUtlDict == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvUtlDict' parameter passed to 'iUtlDictProcessEntry'."); 
        return (UTL_DictInvalidDict);
    }

    if ( bUtlStringsIsStringNULL(pucDictKey) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'pucDictKey' parameter passed to 'iUtlDictProcessEntry'."); 
        return (UTL_DictInvalidKey);
    }

    if ( iUtlDictCallBackFunction == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'iUtlDictCallBackFunction' parameter passed to 'iUtlDictProcessEntry'."); 
        return (UTL_DictInvalidCallBackFunction);
    }


    
    /* Get the dict entry */
    if ( (iError = iUtlDictGetEntry((void *)pudUtlDict, pucDictKey, &pvEntryData, &uiEntryLength)) != UTL_NoError ) {
        return (iError);
    }


    /* Call the call back function */
    va_start(ap, iUtlDictCallBackFunction);
    iStatus = iUtlDictCallBackFunction(pucDictKey, pvEntryData, uiEntryLength, ap);
    va_end(ap);


    return (UTL_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlDictProcessEntryList()

    Purpose:    Process a list of entries from the dictionary

                The declaration format for the call back function is:

                    iUtlDictCallBackFunction(unsigned char *pucKey, void *pvEntryData, 
                        unsigned int uiEntryLength, va_list ap)

                The call-back function needs to return 0 to keep processing or
                non-zero to stop processing.

    Parameters: pvUtlDict                   dictionary structure
                pucDictKey                  dictionary key
                iUtlDictCallBackFunction    call-back function
                ...                         args (optional)

    Globals:    none

    Returns:    UTL error code 

*/
int iUtlDictProcessEntryList
(
    void *pvUtlDict,
    unsigned char *pucDictKey, 
    int (*iUtlDictCallBackFunction)(),
    ...
)
{

    int                 iError = UTL_NoError;
    struct utlDict      *pudUtlDict = (struct utlDict *)pvUtlDict;
    unsigned char       *pucSuperBlockEntry = NULL;
    unsigned char       *pucSuperBlockEntryPtr = NULL;
    unsigned long       ulKeyBlockID = 0;
    va_list             ap;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iUtlDictProcessEntryList - pucDictKey: [%s]", pucDictKey); */


    /* Check the parameters */
    if ( pvUtlDict == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvUtlDict' parameter passed to 'iUtlDictProcessEntryList'."); 
        return (UTL_DictInvalidDict);
    }

    if ( bUtlStringsIsStringNULL(pucDictKey) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'pucDictKey' parameter passed to 'iUtlDictProcessEntryList'."); 
        return (UTL_DictInvalidKey);
    }

    if ( iUtlDictCallBackFunction == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'iUtlDictCallBackFunction' parameter passed to 'iUtlDictProcessEntryList'."); 
        return (UTL_DictInvalidCallBackFunction);
    }



    /* Get the super block entry for this key */
    if ( (iError = iUtlDictGetSuperBlockEntry(pudUtlDict, pucDictKey, &pucSuperBlockEntry)) != UTL_NoError ) {
        return (iError);
    }


    /* Get a pointer to the key block ID in the super block entry */
    pucSuperBlockEntryPtr = pucSuperBlockEntry + pudUtlDict->uiKeyLength;

    /* Read the key block ID from the super block entry */
    UTL_NUM_READ_ULONG(ulKeyBlockID, UTL_DICT_SUPER_BLOCK_KEY_BLOCK_ID_SIZE, pucSuperBlockEntryPtr);


    /* Process the key block entry list */
    va_start(ap, iUtlDictCallBackFunction);
    iError = iUtlDictProcessKeyBlockEntryList(pvUtlDict, ulKeyBlockID, pucDictKey, (int (*)())iUtlDictCallBackFunction, ap);
    va_end(ap);
    

    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlDictList()

    Purpose:    List a dictionary.

    Parameters: pucDictFilePath     dictionary file path

    Globals:    none

    Returns:    UTL error code 

*/
int iUtlDictList
(
    unsigned char *pucDictFilePath
)
{

    int                 iError = UTL_NoError;
    struct utlDict      *pudUtlDict = NULL;
    unsigned char       *pucSuperBlockPtr = NULL;
    unsigned int        uiI = 0;
    unsigned int        uiKeyCount = 0;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iUtlDictList - pucDictFilePath: [%lu]", pucDictFilePath); */


    /* Check the parameters */
    if ( bUtlStringsIsStringNULL(pucDictFilePath) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'pucDictFilePath' parameter passed to 'iUtlDictList'."); 
        return (UTL_DictInvalidFilePath);
    }



    /* Open the dictionary */
    if ( (iError = iUtlDictOpen(pucDictFilePath, (void **)&pudUtlDict)) != UTL_NoError ) {
        return (iError);
    }
    
    
    /* List the super block */
    printf("Dictionary Super Block:\n");

    printf("Super Block ID: %lu, Key Length: %u, Super Block Entry Count: %u\n", pudUtlDict->ulSuperBlockID, pudUtlDict->uiKeyLength, pudUtlDict->uiSuperBlockEntryCount);

    printf("Key                                          Key Block ID\n");

    /* Loop through all the super block entries */
    for ( uiI = 0, pucSuperBlockPtr = pudUtlDict->pucSuperBlock; uiI < pudUtlDict->uiSuperBlockEntryCount; uiI++ ) {
        
        unsigned long   ulKeyBlockID = 0;

        /* Set the super block key block ID pointer */
        unsigned char *pucSuperBlockKeyBlockIDPtr = pucSuperBlockPtr + pudUtlDict->uiKeyLength;

        /* Read the key block ID */
        UTL_NUM_READ_ULONG(ulKeyBlockID, UTL_DICT_SUPER_BLOCK_KEY_BLOCK_ID_SIZE, pucSuperBlockKeyBlockIDPtr);

        /* Print the key and the key block ID */
        if (  s_strcmp(pucSuperBlockPtr, UTL_DICT_FIRST_KEY_STRING) == 0 ) {
            printf(" %-40s %10lu\n", "UTL_DICT_FIRST_KEY_STRING", ulKeyBlockID);
        }
        else if ( s_strcmp(pucSuperBlockPtr, UTL_DICT_LAST_KEY_STRING) == 0 ) {
            printf(" %-40s %10lu\n", "UTL_DICT_LAST_KEY_STRING", ulKeyBlockID);
        }
        else {
            printf(" %-40s %10lu\n", pucSuperBlockPtr, ulKeyBlockID);
        }

        /* Move to the next super block entry */
        pucSuperBlockPtr += pudUtlDict->uiSuperBlockEntryLength;
    }

    printf("\n");



    /* List the key blocks */
    printf("Dictionary Key Blocks:\n");

    /* Loop through all the super block entries */
    for ( uiI = 0, pucSuperBlockPtr = pudUtlDict->pucSuperBlock; uiI < pudUtlDict->uiSuperBlockEntryCount; uiI++ ) {
        
        unsigned long   ulKeyBlockID = 0;
        unsigned char   *pucKeyBlockPtr = NULL;
        unsigned char   *pucKeyBlockEndPtr = NULL;
        unsigned int    uiKeyBlockLength = 0;
        unsigned char   pucKey[UTL_DICT_KEY_MAXIMUM_LENGTH + 1] = {'\0'};

        /* Set the super block key block ID pointer */
        unsigned char *pucSuperBlockKeyBlockIDPtr = pucSuperBlockPtr + pudUtlDict->uiKeyLength;

        /* Read the key block ID */
        UTL_NUM_READ_ULONG(ulKeyBlockID, UTL_DICT_SUPER_BLOCK_KEY_BLOCK_ID_SIZE, pucSuperBlockKeyBlockIDPtr);

        /* Move to the next super block entry */
        pucSuperBlockPtr += pudUtlDict->uiSuperBlockEntryLength;

        /* Get the key block pointer */
        pucKeyBlockPtr = (unsigned char *)pudUtlDict->pvFile + ulKeyBlockID;

        /* Read the key block length */
        UTL_NUM_READ_COMPRESSED_UINT(uiKeyBlockLength, pucKeyBlockPtr);

        /* Get the key block end pointer */
        pucKeyBlockEndPtr = pucKeyBlockPtr + uiKeyBlockLength;

        /* Nice header */
        printf("Key Block ID: %lu, Key Block Length: %u\n", ulKeyBlockID, uiKeyBlockLength);
        printf("Key                                          Data Length\n"); 

        /* Loop over the pointer until we run out */
        do {
            
            unsigned int    uiKeyIndentLength = 0;
            unsigned int    uiEntryLength = 0;
            void            *pvEntryData = NULL;
            
            /* Read the key indent length */
            UTL_NUM_READ_COMPRESSED_UINT(uiKeyIndentLength, pucKeyBlockPtr);

            /* Copy the key delta */
            s_strnncpy(pucKey + uiKeyIndentLength, pucKeyBlockPtr, (UTL_DICT_KEY_MAXIMUM_LENGTH + 1) - uiKeyIndentLength);

            /* Move our pointer past the end of the key delta - which includes the terminating NULL*/
            pucKeyBlockPtr = (unsigned char *)s_strchr(pucKeyBlockPtr, '\0') + 1;
            
            /* Read the entry length */
            UTL_NUM_READ_COMPRESSED_UINT(uiEntryLength, pucKeyBlockPtr);

            if ( uiEntryLength > 0 ) {
                pvEntryData = (void *)pucKeyBlockPtr;
            }
            else {
                pvEntryData = NULL;
            }

            /* Skip over the entry data */
            pucKeyBlockPtr += uiEntryLength;

            if (  s_strcmp(pucKey, UTL_DICT_FIRST_KEY_STRING) == 0 ) {
                printf(" %-40s %10u\n", "UTL_DICT_FIRST_KEY_STRING", uiEntryLength);
            }
            else if ( s_strcmp(pucKey, UTL_DICT_LAST_KEY_STRING) == 0 ) {
                printf(" %-40s %10u\n", "UTL_DICT_LAST_KEY_STRING", uiEntryLength);
            }
            else {
                printf(" %-40s %10u\n", pucKey, uiEntryLength);
            }
            
            /* Increment the key count */
            uiKeyCount++;

        } while ( pucKeyBlockPtr < pucKeyBlockEndPtr );

        printf("\n");

    }

    printf("Key Count: %u\n", uiKeyCount);
    printf("\n");


    return (iError);

}


/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlDictMapFile()

    Purpose:    Map the dictionary file

    Parameters: pudUtlDict      dictionary structure

    Globals:    none

    Returns:    UTL error code 

*/
static int iUtlDictMapFile
(
    struct utlDict *pudUtlDict
)
{

    int     iError = UTL_NoError;

    
/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iUtlDictMapFile'); */


    ASSERT(pudUtlDict != NULL);
    ASSERT(pudUtlDict->pfFile != NULL);
    ASSERT(pudUtlDict->pvFile == NULL);
    ASSERT(pudUtlDict->zFileLength == 0);


    /* Get the dictionary file length */
    if ( (iError = iUtlFileGetFileLength(pudUtlDict->pfFile, &pudUtlDict->zFileLength)) != UTL_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the dictionary file length, utl error: %d.", iError); 
        return (UTL_DictMappingFailed);
    }


    /* Mmap the dictionary file */
    if ( (iError = iUtlFileMemoryMap(fileno(pudUtlDict->pfFile), 0, pudUtlDict->zFileLength, PROT_READ, (void **)&pudUtlDict->pvFile)) != UTL_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to map in the dictionary file, utl error: %d.", iError);
        return (UTL_DictMappingFailed);
    }


    return (UTL_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlDictUnMapFile()

    Purpose:    Unmap the dictionary file

    Parameters: pudUtlDict      dictionary structure

    Globals:    none

    Returns:    UTL error code 

*/
static int iUtlDictUnMapFile
(
    struct utlDict *pudUtlDict
)
{

/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iUtlDictUnMapFile'); */


    ASSERT(pudUtlDict != NULL);


    /* Unmap the dictionary file and clear the structure variables */
    if ( pudUtlDict->pvFile != NULL ) {
        iUtlFileMemoryUnMap(pudUtlDict->pvFile, pudUtlDict->zFileLength);
        pudUtlDict->pvFile = NULL;
        pudUtlDict->zFileLength = 0;
    }


    return (UTL_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlDictGetSuperBlockEntry()

    Purpose:    Get the super block entry where the key would be located.

    Parameters: pudUtlDict              dictionary structure
                pucKey                  key
                ppucSuperBlockEntry     return pointer for the super block entry

    Globals:    none

    Returns:    UTL error code 

*/
static int iUtlDictGetSuperBlockEntry
(
    struct utlDict *pudUtlDict,
    unsigned char *pucKey,
    unsigned char **ppucSuperBlockEntry
)
{

    unsigned int    uiHigh = 0;
    unsigned int    uiLow = 0;
    unsigned int    uiSuperBlockEntryIndex = 0;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iUtlDictGetSuperBlockEntry - pucKey: [%s]", pucKey); */


    ASSERT(pudUtlDict != NULL);
    ASSERT(bUtlStringsIsStringNULL(pucKey) == false);
    ASSERT(ppucSuperBlockEntry != NULL);


    /* Check to see if the key is lexically before the first key in the super block, 
    ** remember that the super block start with the (NULL terminated) key 
    */
    if ( s_strcmp(pucKey, pudUtlDict->pucSuperBlock) < 0 ) {
/*         iUtlLogDebug(UTL_LOG_CONTEXT, "Key is before the super block - key: [%s], super block key: [%s]", pucKey, pudUtlDict->pucSuperBlock); */
        return (UTL_DictKeyNotFound);
    }

    uiHigh = pudUtlDict->uiSuperBlockEntryCount;
    uiSuperBlockEntryIndex = (uiLow + uiHigh) / 2;

/* iUtlLogDebug(UTL_LOG_CONTEXT, "initial - uiSuperBlockEntryIndex = %u", uiSuperBlockEntryIndex); */

    while ( uiLow != uiHigh ) {
    
        /* Get the super block entry pointer and the key, remember that the super block entry start with the key (NULL terminated) */
        unsigned char   *pucSuperBlockEntryPtr = pudUtlDict->pucSuperBlock + (pudUtlDict->uiSuperBlockEntryLength * uiSuperBlockEntryIndex);
        unsigned char   *pucKeyPtr = pucSuperBlockEntryPtr;
        
        /* Compare the super block key with the key */
        int    iStatus = s_strcmp(pucKeyPtr, pucKey);

/* iUtlLogDebug(UTL_LOG_CONTEXT, "iStatus [%d] - super block key [%s]", iStatus, pucKeyPtr); */

        /* Exact match */
        if ( iStatus == 0 ) {
            *ppucSuperBlockEntry = pudUtlDict->pucSuperBlock + (pudUtlDict->uiSuperBlockEntryLength * uiSuperBlockEntryIndex);
/*             *ppucSuperBlockEntry = pucSuperBlockEntryPtr; */
            return (UTL_NoError);
        }
        
        /* Further back */
        else if ( iStatus > 0 ) {

            if ( uiHigh != uiSuperBlockEntryIndex ) {
                uiHigh = uiSuperBlockEntryIndex;
                uiSuperBlockEntryIndex = (uiLow + uiHigh) / 2;
/*                 uiSuperBlockEntryIndex = uiLow + ((uiHigh - uiLow) / 2); */    /* Use this if 'uiLow + uiHigh' blows the maximum unsigned int and wraps around */
/* iUtlLogDebug(UTL_LOG_CONTEXT, "(iStatus > 0) - uiSuperBlockEntryIndex = %u", uiSuperBlockEntryIndex); */
            } 
            else {
/* iUtlLogDebug(UTL_LOG_CONTEXT, "(iStatus > 0) - match - super block key [%s]", pucKeyPtr); */
                *ppucSuperBlockEntry = pudUtlDict->pucSuperBlock + (pudUtlDict->uiSuperBlockEntryLength * (uiSuperBlockEntryIndex - 1));
/*                 *ppucSuperBlockEntry = pucSuperBlockEntryPtr - pudUtlDict->uiSuperBlockEntryLength; */
                return (UTL_NoError);
            }
        }
        
        /* Further along */
        else if ( iStatus < 0 ) {

            if ( uiLow != uiSuperBlockEntryIndex ) {
                uiLow = uiSuperBlockEntryIndex;
                uiSuperBlockEntryIndex = (0.5 + uiLow + uiHigh) / 2;
/* iUtlLogDebug(UTL_LOG_CONTEXT, "(iStatus < 0) - uiSuperBlockEntryIndex = %u", uiSuperBlockEntryIndex); */
            } 
            else {
/* iUtlLogDebug(UTL_LOG_CONTEXT, "(iStatus < 0) - match - super block key [%s]", pucKeyPtr); */
                *ppucSuperBlockEntry = pudUtlDict->pucSuperBlock + (pudUtlDict->uiSuperBlockEntryLength * uiSuperBlockEntryIndex);
/*                 *ppucSuperBlockEntry = pucSuperBlockEntryPtr; */
                return (UTL_NoError);
            }
        }
    }


    /* Found the block */
    if ( uiSuperBlockEntryIndex != 0 ) {
/* iUtlLogDebug(UTL_LOG_CONTEXT, "(uiSuperBlockEntryIndex != 0) "); */
        *ppucSuperBlockEntry = pudUtlDict->pucSuperBlock + (pudUtlDict->uiSuperBlockEntryLength * (uiSuperBlockEntryIndex - 1));
/*         *ppucSuperBlockEntry = pucSuperBlockEntryPtr - pudUtlDict->uiSuperBlockEntryLength; */
        return (UTL_NoError);
    }

    /* Fall through, this should never happen */            
/* iUtlLogDebug(UTL_LOG_CONTEXT, "(uiSuperBlockEntryIndex == 0)"); */


    return (UTL_DictKeyNotFound);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlDictGetKeyBlockEntryData()

    Purpose:    Get the entry data from the key block for the key.

    Parameters: pvUtlDict           dictionary structure
                ulKeyBlockID        key block ID
                pucKey              key
                ppvEntryData        return pointer for the entry data
                puiEntryLength      return pointer for the entry length

    Globals:    none

    Returns:    UTL error code 

*/
static int iUtlDictGetKeyBlockEntryData
(
    struct utlDict *pudUtlDict,
    unsigned long ulKeyBlockID,
    unsigned char *pucKey,
    void **ppvEntryData,
    unsigned int *puiEntryLength
    
)
{


    unsigned int    uiKeyBlockLength = 0;
    unsigned char   *pucKeyBlockPtr = NULL;
    unsigned char   *pucKeyBlockEndPtr = NULL;
    unsigned int    uiKeyLength = 0;
    unsigned int    uiKeyIndentLength = 0;
    unsigned char   pucLocalKey[UTL_DICT_KEY_MAXIMUM_LENGTH + 1] = {'\0'};
    unsigned char   *pucLocalKeyPtr = NULL;
    unsigned int    uiEntryLength = 0;
    int             iStatus = 0;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iUtlDictGetKeyBlockEntryData - ulKeyBlockID: [%lu], pucKey: [%s]", ulKeyBlockID, pucKey); */


    ASSERT(pudUtlDict != NULL);
    ASSERT(ulKeyBlockID > 0);
    ASSERT(bUtlStringsIsStringNULL(pucKey) == false);
    ASSERT(ppvEntryData != NULL);
    ASSERT(puiEntryLength != NULL);


    /* Get the key length */
    uiKeyLength = s_strlen(pucKey);


    /* Set the key block pointer */
    pucKeyBlockPtr = (unsigned char *)pudUtlDict->pvFile + ulKeyBlockID;
    
    /* Read the key block length */
    UTL_NUM_READ_COMPRESSED_UINT(uiKeyBlockLength, pucKeyBlockPtr);
    
    /* Set the key block end pointer */
    pucKeyBlockEndPtr = pucKeyBlockPtr + uiKeyBlockLength;


    /* Loop while we are in the key block */
    do {

        /* Read the key indent length */
        UTL_NUM_READ_COMPRESSED_UINT(uiKeyIndentLength, pucKeyBlockPtr);
        
        /* Copy the key delta to the local key starting at the indent in the local key,
        ** the copy is done character by character because this is much faster than s_strnncpy()
        */
        for ( pucLocalKeyPtr = pucLocalKey + uiKeyIndentLength; *pucKeyBlockPtr != '\0'; pucLocalKeyPtr++, pucKeyBlockPtr++ ) {
            *pucLocalKeyPtr = *pucKeyBlockPtr;
        }
    
        /* NULL terminate the local key */
        *pucLocalKeyPtr = '\0';

        /* Increment the key block pointer past the end of the key delta */
        pucKeyBlockPtr++;

        /* Read the entry length, after which pucKeyBlockPtr will point to the entry data */
        UTL_NUM_READ_COMPRESSED_UINT(uiEntryLength, pucKeyBlockPtr);


        /* And check the local key against the key we are looking for, 
        ** first check that they are the same length before calling s_strcmp()
        */
        if ( (uiKeyLength == (pucLocalKeyPtr - pucLocalKey)) && (iStatus = s_strcmp(pucKey, pucLocalKey)) == 0 ) {

/*             iUtlLogDebug(UTL_LOG_CONTEXT, "iUtlDictGetKeyBlockEntryData - Key [%s]", pucLocalKey); */

            /* This is the key we are looking for, so we set the return pointers and return */
            *ppvEntryData = pucKeyBlockPtr;
            *puiEntryLength = uiEntryLength;
            
            return (UTL_NoError);
        }

/* iUtlLogDebug(UTL_LOG_CONTEXT, "iUtlDictGetKeyBlockEntryData - Key [%s][%d]", pucLocalKey, s_strcmp(pucLocalKey, UTL_DICT_LAST_KEY_FLAG)); */

        /* We failed to make a match, so we skip over the entry data */
        pucKeyBlockPtr += uiEntryLength;

    } while ( (iStatus >= 0) && (pucKeyBlockPtr < pucKeyBlockEndPtr) );    


    /* We dropped out of the loop, so the key was not found */
    return (UTL_DictKeyNotFound);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlDictProcessKeyBlockEntryList()

    Purpose:    Look for a key in the key-block.

                This function will look for a range of keys in the key blocks.

                For each key found, the call-back function will be called to 
                futher qualify that key. 

                The declaration format for the call back function is:

                    iUtlDictCallBackFunction(unsigned char *pucKey, void *pvEntryData, 
                        unsigned int uiEntryLength, va_list ap)

                The call-back function needs to return 0 to keep processing or
                non-zero to stop processing.
                
    Parameters: pvUtlDict                   dictionary structure
                ulKeyBlockID                key block ID
                pucKey                      key
                iUtlDictCallBackFunction    lookup call back function
                ap                          arg list (optional)

    Globals:    none

    Returns:    UTL error code 

*/
static int iUtlDictProcessKeyBlockEntryList
(
    struct utlDict *pudUtlDict,
    unsigned long ulKeyBlockID,
    unsigned char *pucKey,
    int (*iUtlDictCallBackFunction)(),
    va_list ap
)
{

    unsigned int    uiKeyBlockLength = 0;
    unsigned char   *pucKeyBlockPtr = NULL;
    unsigned char   *pucKeyBlockEndPtr = NULL;
    unsigned int    uiKeyIndentLength = 0;
    unsigned char   pucLocalKey[UTL_DICT_KEY_MAXIMUM_LENGTH + 1] = {'\0'};
    unsigned char   *pucLocalKeyPtr = NULL;
    void            *pvEntryData = 0;
    unsigned int    uiEntryLength = 0;
    int             iStatus = 0;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iUtlDictProcessKeyBlockEntryList - ulKeyBlockID: [%lu], pucKey: [%s]", ulKeyBlockID, pucKey); */


    ASSERT(pudUtlDict != NULL);
    ASSERT(ulKeyBlockID > 0);
    ASSERT(bUtlStringsIsStringNULL(pucKey) == false);
    ASSERT(iUtlDictCallBackFunction != NULL);


    /* Set the key block pointer */
    pucKeyBlockPtr = (unsigned char *)pudUtlDict->pvFile + ulKeyBlockID;
    

    /* Loop while there are keys to be found, the call back function will tell us
    ** when this is the case and returns a non-0 value
    **
    ** Also note that key blocks are sequential without any breaks in between
    */
    while ( true ) {

        /* Read the key block length */
        UTL_NUM_READ_COMPRESSED_UINT(uiKeyBlockLength, pucKeyBlockPtr);
    
        /* Set the key block end pointer */
        pucKeyBlockEndPtr = pucKeyBlockPtr + uiKeyBlockLength;


        /* Loop while we are still in the key block */
        do {

            /* Read the key indent length */
            UTL_NUM_READ_COMPRESSED_UINT(uiKeyIndentLength, pucKeyBlockPtr);
            
            /* Copy the key delta to the local key starting at the indent in the local key,
            ** the copy is done character by character because this is much faster than s_strnncpy()
            */
            for ( pucLocalKeyPtr = pucLocalKey + uiKeyIndentLength; *pucKeyBlockPtr != '\0'; pucLocalKeyPtr++, pucKeyBlockPtr++ ) {
                *pucLocalKeyPtr = *pucKeyBlockPtr;
            }
        
            /* NULL terminate the local key */
            *pucLocalKeyPtr = '\0';
    
            /* Increment the key block pointer past the end of the key delta */
            pucKeyBlockPtr++;
    
            /* Read the entry length */
            UTL_NUM_READ_COMPRESSED_UINT(uiEntryLength, pucKeyBlockPtr);

            /* Set the entry data pointer */
            pvEntryData = (void *)pucKeyBlockPtr;

            /* Skip over the data */
            pucKeyBlockPtr += uiEntryLength;

    
            /* Is this the first key? */
            if (  pucLocalKey[0] == UTL_DICT_FIRST_KEY_CHARACTER ) {
                if ( s_strcmp(pucLocalKey, UTL_DICT_FIRST_KEY_STRING) == 0 ) {
                    continue;
                }
            }

            /* Is this the last key? */
            else if ( pucLocalKey[0] == UTL_DICT_LAST_KEY_CHARACTER ) { 
                if ( s_strcmp(pucLocalKey, UTL_DICT_LAST_KEY_STRING) == 0 ) {
                    return (UTL_NoError);
                }
            }


/*             iUtlLogInfo(UTL_LOG_CONTEXT, "iUtlDictProcessKeyBlockEntryList - Key [%s]", pucLocalKey); */

            /* Call the call back function */
            if ( iUtlDictCallBackFunction(pucLocalKey, pvEntryData, uiEntryLength, ap) != 0 ) {
                /* Did we get a signal to stop? */
                return (UTL_NoError);
            }

        } while ( (iStatus >= 0) && (pucKeyBlockPtr < pucKeyBlockEndPtr) );    

    }


    return (UTL_NoError);

}


/*---------------------------------------------------------------------------*/
