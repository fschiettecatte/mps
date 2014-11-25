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

    Module:     table.c

    Author:     Francois Schiettecatte (FS Consulting LLC.)

    Created:    29 Aug 2010

    Purpose:    This implements a simple storage system for fixed length blobs
                of data, a table is created, entries are added to it each 
                returning an ID which can later be used to retrieve the entry.
                
                The fixed length is specified when the table is created and is
                stored in a 4 byte header in the table file. This allows each 
                table entry to be up to 4GB in length.

                Data entries can be accessed as soon as they are added.


                Table creation functions:

                    iUtlTableCreate()
                    iUtlTableAddEntry()
                    iUtlTableClose()


                Table access functions:

                    iUtlTableOpen()
                    iUtlTableGetEntry()
                    iUtlTableProcessEntry()
                    iUtlTableClose()

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
#define UTL_LOG_CONTEXT                     (unsigned char *)"com.fsconsult.mps.src.utils.table2"


/*---------------------------------------------------------------------------*/


/*
** Defines
*/

/* Table entry length size */
#define    UTL_TABLE_ENTRY_LENGTH_SIZE              (4)


/* Table mode */
#define    UTL_TABLE_MODE_INVALID                   (0)
#define    UTL_TABLE_MODE_WRITE                     (1)
#define    UTL_TABLE_MODE_READ                      (2)


/*---------------------------------------------------------------------------*/


/*
** Structures
*/


/* Table structure */
struct utlTable {
    unsigned int    uiMode;                     /* Table mode */
    FILE            *pfFile;                    /* Table file */
    unsigned int    uiEntryLength;              /* Table entry length */
    unsigned long   ulEntryCount;               /* Table entry count */
    void            *pvFile;                    /* Data pointer if the table file if memory mapped */
    size_t          zFileLength;                /* Data length if the table file is memory mapped */
};


/*---------------------------------------------------------------------------*/


/*
** Private function prototypes
*/

static int iUtlTableMapFile (struct utlTable *putUtlTable);


static int iUtlTableUnMapFile (struct utlTable *putUtlTable);


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlTableCreate()

    Purpose:    Create a new table.

    Parameters: pucTableFilePath        table file path
                uiTableEntryLength      table entry length
                ppvUtlTable             return pointer for the table structure

    Globals:    none

    Returns:    UTL error code

*/
int iUtlTableCreate
(
    unsigned char *pucTableFilePath,
    unsigned int uiTableEntryLength,
    void **ppvUtlTable
)
{

    int                 iError = UTL_NoError;
    unsigned char       pucTableHeader[UTL_TABLE_ENTRY_LENGTH_SIZE];
    unsigned char       *pucTableHeaderPtr = pucTableHeader;
    struct utlTable     *putUtlTable = NULL;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iUtlTableCreate - pucTableFilePath: [%s], uiTableEntryLength: [%u]", pucTableFilePath, uiTableEntryLength); */


    /* Check the parameters */
    if ( bUtlStringsIsStringNULL(pucTableFilePath) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'pucTableFilePath' parameter passed to 'iUtlTableCreate'."); 
        return (UTL_TableInvalidFilePath);
    }

    if ( uiTableEntryLength <= 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiTableEntryLength' parameter passed to 'iUtlTableCreate'."); 
        return (UTL_TableInvalidEntryLength);
    }

    if ( ppvUtlTable == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppvUtlTable' parameter passed to 'iUtlTableCreate'."); 
        return (UTL_ReturnParameterError);
    }


    /* Allocate a table structure */
    if ( (putUtlTable = (struct utlTable *)s_malloc((size_t)(sizeof(struct utlTable)))) == NULL ) {
        return (UTL_MemError);
    }

    /* Initialize all the fields in the table structure */
    putUtlTable->uiMode = UTL_TABLE_MODE_WRITE;
    putUtlTable->pfFile = NULL;
    putUtlTable->uiEntryLength = uiTableEntryLength;
    putUtlTable->ulEntryCount = 0;
    putUtlTable->pvFile = NULL;
    putUtlTable->zFileLength = 0;


    /* Create the table file - 'w+' so we can access it while we are creating it */
    if ( (putUtlTable->pfFile = s_fopen(pucTableFilePath, "w+")) == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to create the table file, table file path: '%s'.", pucTableFilePath);
        iError = UTL_TableCreateFailed;
        goto bailFromiUtlTableCreate;
    }


    /* Encode the table entry length into the table header */
    pucTableHeaderPtr = pucTableHeader;
    UTL_NUM_WRITE_UINT(putUtlTable->uiEntryLength, UTL_TABLE_ENTRY_LENGTH_SIZE, pucTableHeaderPtr);
    
    /* Write out the table header to the table file */
    if ( s_fwrite(pucTableHeader, UTL_TABLE_ENTRY_LENGTH_SIZE, 1, putUtlTable->pfFile) != 1 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to write the table header to the table file."); 
        iError = UTL_TableWriteFailed;
        goto bailFromiUtlTableCreate;
    }


    /* Bail label */
    bailFromiUtlTableCreate:
    
    /* Handle errors */
    if ( iError != UTL_NoError ) {
    
        /* Close the table */
        if ( putUtlTable != NULL ) {
            iUtlTableClose((void *)putUtlTable);
        }
        
        /* Remove the table file */
        s_remove(pucTableFilePath);

        /* Clear the return pointer */
        *ppvUtlTable = NULL;
    }
    else {

        /* Set the return pointer */
        *ppvUtlTable = (void *)putUtlTable;
    }


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlTableAddEntry()

    Purpose:    Add a table entry.

    Parameters: pvUtlTable          table structure
                pvTableEntryData    pointer the entry data
                pulTableEntryID     return pointer for the new table entry ID

    Globals:    none

    Returns:    UTL error code 

*/
int iUtlTableAddEntry
(
    void *pvUtlTable,
    void *pvTableEntryData,
    unsigned long *pulTableEntryID
)
{

    struct utlTable     *putUtlTable = (struct utlTable *)pvUtlTable;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iUtlTableAddEntry"); */


    /* Check the parameters */
    if ( pvUtlTable == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvUtlTable' parameter passed to 'iUtlTableAddEntry'."); 
        return (UTL_TableInvalidTable);
    }

    if ( pvTableEntryData == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvTableEntryData' parameter passed to 'iUtlTableAddEntry'."); 
        return (UTL_TableInvalidData);
    }

    if ( pulTableEntryID == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pulTableEntryID' parameter passed to 'iUtlTableAddEntry'."); 
        return (UTL_ReturnParameterError);
    }



    /* Check that we are in write mode */
    if ( putUtlTable->uiMode != UTL_TABLE_MODE_WRITE ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to add an entry to the table, invalid table mode: %u.", putUtlTable->uiMode); 
        return (UTL_TableInvalidMode);
    }


    /* Unmap the table file, ignore errors */
    iUtlTableUnMapFile(putUtlTable);


    /* Write out the table entry data to the table file */
    if ( s_fwrite(pvTableEntryData, putUtlTable->uiEntryLength, 1, putUtlTable->pfFile) != 1 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to write the table entry data to the table file."); 
        return (UTL_TableWriteFailed);
    }


    /* Increment the table entry count */
    putUtlTable->ulEntryCount++;    

    /* Set the return pointer */
    *pulTableEntryID = putUtlTable->ulEntryCount;


    return (UTL_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlTableClose()

    Purpose:    Close the table.

    Parameters: pvUtlTable      table structure

    Globals:    none

    Returns:    UTL error code 

*/
int iUtlTableClose
(
    void *pvUtlTable
)
{

    struct utlTable     *putUtlTable = (struct utlTable *)pvUtlTable;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iUtlTableClose"); */


    /* Check the parameters */
    if ( pvUtlTable == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvUtlTable' parameter passed to 'iUtlTableClose'."); 
        return (UTL_TableInvalidTable);
    }



    /* Unmap the table file, ignore errors */
    iUtlTableUnMapFile(putUtlTable);

    /* Close the table file */
    s_fclose(putUtlTable->pfFile);

    /* Finally release the table */
    s_free(putUtlTable);


    return (UTL_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlTableOpen()

    Purpose:    Open a table.

    Parameters: pucTableFilePath    table file path
                ppvUtlTable         return pointer for the table structure

    Globals:    none

    Returns:    UTL error code 

*/
int iUtlTableOpen
(
    unsigned char *pucTableFilePath,
    void **ppvUtlTable
)
{

    int                 iError = UTL_NoError;
    unsigned char       pucTableHeader[UTL_TABLE_ENTRY_LENGTH_SIZE];
    unsigned char       *pucTableHeaderPtr = pucTableHeader;
    struct utlTable     *putUtlTable = NULL;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iUtlTableOpen - pucTableFilePath: [%s]", pucTableFilePath); */


    /* Check the parameters */
    if ( bUtlStringsIsStringNULL(pucTableFilePath) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'pucTableFilePath' parameter passed to 'iUtlTableOpen'."); 
        return (UTL_TableInvalidFilePath);
    }

    if ( ppvUtlTable == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppvUtlTable' parameter passed to 'iUtlTableOpen'."); 
        return (UTL_ReturnParameterError);
    }



    /* Allocate a table structure */
    if ( (putUtlTable = (struct utlTable *)s_malloc((size_t)(sizeof(struct utlTable)))) == NULL ) {
        return (UTL_MemError);
    }

    /* Initialize all the fields in the table structure */
    putUtlTable->uiMode = UTL_TABLE_MODE_READ;
    putUtlTable->pfFile = NULL;
    putUtlTable->uiEntryLength = 0;
    putUtlTable->ulEntryCount = 0;
    putUtlTable->pvFile = NULL;
    putUtlTable->zFileLength = 0;


    /* Open the table file */
    if ( (putUtlTable->pfFile = s_fopen(pucTableFilePath, "r")) == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to open the table, table file path: '%s'.", pucTableFilePath); 
        iError = UTL_TableOpenFailed;
        goto bailFromiUtlTableOpen;
    }


    /* Read the table header from the table file */
    if ( s_fread(pucTableHeader, UTL_TABLE_ENTRY_LENGTH_SIZE, 1, putUtlTable->pfFile) != 1 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to read the table header from the table file."); 
        iError = UTL_TableReadFailed;
        goto bailFromiUtlTableOpen;
    }

    /* Decode the table entry length from the table header */
    pucTableHeaderPtr = pucTableHeader;
    UTL_NUM_READ_UINT(putUtlTable->uiEntryLength, UTL_TABLE_ENTRY_LENGTH_SIZE, pucTableHeaderPtr);


    /* Map the table file */
    if ( (iError = iUtlTableMapFile(putUtlTable)) != UTL_NoError ) {
        goto bailFromiUtlTableOpen;
    }


    /* Get the table entry count, file length divided by entry length */
    putUtlTable->ulEntryCount = putUtlTable->zFileLength / putUtlTable->uiEntryLength;



    /* Bail label */
    bailFromiUtlTableOpen:
    
    /* Handle errors */
    if ( iError != UTL_NoError ) {
    
        /* Close the table */
        if ( putUtlTable != NULL ) {
            iUtlTableClose((void *)putUtlTable);
        }
        
        /* Clear the return pointer */
        *ppvUtlTable = NULL;
    }
    else {

        /* Set the return pointer */
        *ppvUtlTable = (void *)putUtlTable;
    }


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlTableGetEntry()

    Purpose:    Get an entry from the table

    Parameters: pvUtlTable          table structure
                ulTableEntryID      table entry ID
                ppvTableEntryData   return pointer for the table entry data

    Globals:    none

    Returns:    UTL error code 

*/
int iUtlTableGetEntry
(
    void *pvUtlTable,
    unsigned long ulTableEntryID, 
    void **ppvTableEntryData
)
{

    int                 iError = UTL_NoError;
    struct utlTable     *putUtlTable = (struct utlTable *)pvUtlTable;
    size_t              zDataOffset = 0;
    unsigned char       *pucDataPtr = NULL;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iUtlTableGetEntry - ulTableEntryID: [%lu]", ulTableEntryID); */


    /* Check the parameters */
    if ( pvUtlTable == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvUtlTable' parameter passed to 'iUtlTableGetEntry'."); 
        return (UTL_TableInvalidTable);
    }

    if ( ulTableEntryID <= 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'ulTableEntryID' parameter passed to 'iUtlTableGetEntry'."); 
        return (UTL_TableInvalidTableEntryID);
    }

    if ( ppvTableEntryData == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppvTableEntryData' parameter passed to 'iUtlTableGetEntry'."); 
        return (UTL_ReturnParameterError);
    }



    /* Map the table file if the table is in write mode and it is not mapped */
    if ( (putUtlTable->uiMode == UTL_TABLE_MODE_WRITE) && (putUtlTable->pvFile == NULL) ) {
        if ( (iError = iUtlTableMapFile(putUtlTable)) != UTL_NoError ) {
            return (iError);
        }
    }


    /* Check that the table entry ID is sensible */
    if ( ulTableEntryID > putUtlTable->ulEntryCount ) {
/*         iUtlLogError(UTL_LOG_CONTEXT, "Failed to get an entry from the table, entry ID: %lu, table entry count: %lu.", ulTableEntryID, putUtlTable->ulEntryCount);  */
        return (UTL_TableInvalidTableEntryID);
    }


    /* Get the data offset, note that table entry ID is one-based while the data is zero-based */
    zDataOffset = (ulTableEntryID - 1) * putUtlTable->uiEntryLength;

    /* Get the data pointer */
    pucDataPtr = (unsigned char *)putUtlTable->pvFile + zDataOffset;
    
    /* Set the return pointer */
    *ppvTableEntryData = (void *)pucDataPtr;


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlTableProcessEntry()

    Purpose:    Get an entry from the table

                The declaration format for the call back function is:

                    iUtlTableCallBackFunction(unsigned long ulEntryID, void *pvEntryData, 
                        unsigned int uiEntryLength, va_list ap)

                The call-back function needs to return 0 to keep processing or
                non-zero to stop processing.

    Parameters: pvUtlTable                  table structure
                ulTableEntryID              table entry ID
                iUtlTableCallBackFunction   get call-back function
                ...                         args (optional)

    Globals:    none

    Returns:    UTL error code 

*/
int iUtlTableProcessEntry
(
    void *pvUtlTable,
    unsigned long ulTableEntryID, 
    int (*iUtlTableCallBackFunction)(),
    ...
)
{

    int                 iError = UTL_NoError;
    struct utlTable     *putUtlTable = (struct utlTable *)pvUtlTable;
    void                *pvTableEntryDataPtr = NULL;
    int                 iStatus = 0;
    va_list             ap;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iUtlTableProcessEntry - ulTableEntryID: [%lu]", ulTableEntryID); */


    /* Check the parameters */
    if ( pvUtlTable == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvUtlTable' parameter passed to 'iUtlTableProcessEntry'."); 
        return (UTL_TableInvalidTable);
    }

    if ( ulTableEntryID <= 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'ulTableEntryID' parameter passed to 'iUtlTableProcessEntry'."); 
        return (UTL_TableInvalidTableEntryID);
    }

    if ( iUtlTableCallBackFunction == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'iUtlTableCallBackFunction' parameter passed to 'iUtlTableProcessEntry'."); 
        return (UTL_TableInvalidCallBackFunction);
    }



    /* Get the table entry */
    if ( (iError = iUtlTableGetEntry((void *)putUtlTable, ulTableEntryID, &pvTableEntryDataPtr)) != UTL_NoError ) {
        return (iError);
    }


    /* Call the call back function */
    va_start(ap, iUtlTableCallBackFunction);
    iStatus = iUtlTableCallBackFunction(ulTableEntryID, pvTableEntryDataPtr, putUtlTable->uiEntryLength, ap);
    va_end(ap);


    return (UTL_NoError);

}


/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlTableMapFile()

    Purpose:    Map the table file

    Parameters: putUtlTable     table structure

    Globals:    none

    Returns:    UTL error code 

*/
static int iUtlTableMapFile
(
    struct utlTable *putUtlTable
)
{

    int     iError = UTL_NoError;
    off_t   zTableFileLength = 0;
    

    ASSERT(putUtlTable != NULL);
    ASSERT(putUtlTable->pfFile != NULL);
    ASSERT(putUtlTable->uiEntryLength > 0);
    ASSERT(putUtlTable->pvFile == NULL);
    ASSERT(putUtlTable->zFileLength == 0);


    /* Flush the table file */
    s_fflush(putUtlTable->pfFile);


    /* Get the table file length */
    if ( (iError = iUtlFileGetFileLength(putUtlTable->pfFile, &zTableFileLength)) != UTL_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the table file length, utl error: %d.", iError); 
        return (UTL_TableMappingFailed);
    }


    /* Get the data length, file length minus the header */
    putUtlTable->zFileLength = zTableFileLength - UTL_TABLE_ENTRY_LENGTH_SIZE;


    /* Mmap the table file, exclude the table header hence the UTL_TABLE_ENTRY_LENGTH_SIZE offset */
    if ( (iError = iUtlFileMemoryMap(fileno(putUtlTable->pfFile), UTL_TABLE_ENTRY_LENGTH_SIZE, putUtlTable->zFileLength, PROT_READ, (void **)&putUtlTable->pvFile)) != UTL_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to map in the table file, utl error: %d.", iError);
        return (UTL_TableMappingFailed);
    }


    return (UTL_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlTableUnMapFile()

    Purpose:    Unmap the table file

    Parameters: putUtlTable     table structure

    Globals:    none

    Returns:    UTL error code 

*/
static int iUtlTableUnMapFile
(
    struct utlTable *putUtlTable
)
{

    ASSERT(putUtlTable != NULL);


    /* Unmap the table file and clear the table structure variables */
    if ( putUtlTable->pvFile != NULL ) {
        iUtlFileMemoryUnMap(putUtlTable->pvFile, putUtlTable->zFileLength);
        putUtlTable->pvFile = NULL;
        putUtlTable->zFileLength = 0;
    }


    return (UTL_NoError);

}


/*---------------------------------------------------------------------------*/
