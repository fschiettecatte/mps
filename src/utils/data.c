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

    Module:     data.c

    Author:     Francois Schiettecatte (FS Consulting LLC.)

    Created:    29 Aug 2010

    Purpose:    This implements a simple storage system for blobs of data, 
                a 'data' is created, entries are added to it each returning
                an ID which can later be used to retrieve the entry.
            
                Each entry is written with a four byte header storing the
                length of the entry, so each entry can be up to 4G in size.
            
                Data entries can be accessed as soon as they are added.


                Data creation functions:

                    iUtlDataCreate()
                    iUtlDataAddEntry()
                    iUtlDataClose()


                Data access functions:

                    iUtlDataOpen()
                    iUtlDataGetEntry()
                    iUtlDataProcessEntry()
                    iUtlDataClose()

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
#define UTL_LOG_CONTEXT                     (unsigned char *)"com.fsconsult.mps.src.utils.data"


/*---------------------------------------------------------------------------*/


/*
** Defines
*/


/* Data entry length size */
#define UTL_DATA_ENTRY_LENGTH_SIZE                  (4)


/* Data mode */
#define UTL_DATA_MODE_INVALID                       (0)
#define UTL_DATA_MODE_WRITE                         (1)
#define UTL_DATA_MODE_READ                          (2)


/*---------------------------------------------------------------------------*/


/*
** Structures
*/


/* Data structure */
struct utlData {
    unsigned int    uiMode;                         /* Data mode */
    FILE            *pfFile;                        /* Data file */
    void            *pvFile;                        /* Data pointer if the data file if memory mapped */
    size_t          zFileLength;                    /* Data length if the data file if memory mapped */
};


/*---------------------------------------------------------------------------*/


/*
** Private function prototypes
*/

static int iUtlDataMapFile (struct utlData *pudUtlData);


static int iUtlDataUnMapFile (struct utlData *pudUtlData);


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlDataCreate()

    Purpose:    Create a new data.

    Parameters: pucDataFilePath     data file path
                ppvUtlData          return pointer for the data structure

    Globals:    none

    Returns:    UTL error code

*/
int iUtlDataCreate
(
    unsigned char *pucDataFilePath,
    void **ppvUtlData
)
{

    long                iError = UTL_NoError;
    struct utlData      *pudUtlData = NULL;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iUtlDataCreate - pucDataFilePath: [%s]", pucDataFilePath); */


    /* Check the parameters */
    if ( bUtlStringsIsStringNULL(pucDataFilePath) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'pucDataFilePath' parameter passed to 'iUtlDataCreate'."); 
        return (UTL_DataInvalidFilePath);
    }

    if ( ppvUtlData == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppvUtlData' parameter passed to 'iUtlDataCreate'."); 
        return (UTL_ReturnParameterError);
    }


    /* Allocate a data structure */
    if ( (pudUtlData = (struct utlData *)s_malloc((size_t)(sizeof(struct utlData)))) == NULL ) {
        return (UTL_MemError);
    }

    /* Initialize all the fields in the data structure */
    pudUtlData->uiMode = UTL_DATA_MODE_WRITE;
    pudUtlData->pfFile = NULL;
    pudUtlData->pvFile = NULL;
    pudUtlData->zFileLength = 0;


    /* Create the data file - 'w+' so we can access it while we are creating it */
    if ( (pudUtlData->pfFile = s_fopen(pucDataFilePath, "w+")) == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to create the data, data file path: '%s'.", pucDataFilePath);
        iError = UTL_DataCreateFailed;
        goto bailFromiUtlDataCreate;
    }



    /* Bail label */
    bailFromiUtlDataCreate:
    
    /* Handle errors */
    if ( iError != UTL_NoError ) {
    
        /* Close the data */
        if ( pudUtlData != NULL ) {
            iUtlDataClose((void *)pudUtlData);
        }
        
        /* Remove the file */
        s_remove(pucDataFilePath);

        /* Clear the return pointer */
        *ppvUtlData = NULL;
    }
    else {

        /* Set the return pointer */
        *ppvUtlData = (void *)pudUtlData;
    }


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlDataAddEntry()

    Purpose:    Add a data entry.

    Parameters: pvUtlData           data structure
                pvDataEntryData     pointer the data entry data
                uiDataEntryLength   data entry length
                pulDataEntryID      return pointer for the new data entry ID

    Globals:    none

    Returns:    UTL error code 

*/
int iUtlDataAddEntry
(
    void *pvUtlData,
    void *pvDataEntryData,
    unsigned int uiDataEntryLength,
    unsigned long *pulDataEntryID
)
{

    struct utlData      *pudUtlData = (struct utlData *)pvUtlData;
    unsigned char       pucDataEntryHeader[UTL_DATA_ENTRY_LENGTH_SIZE];
    unsigned char       *pucDataEntryHeaderPtr = pucDataEntryHeader;
    unsigned long       ulDataEntryID = 0;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iUtlDataAddEntry"); */


    /* Check the parameters */
    if ( pvUtlData == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvUtlData' parameter passed to 'iUtlDataAddEntry'."); 
        return (UTL_DataInvalidData);
    }

    if ( pvDataEntryData == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvDataEntryData' parameter passed to 'iUtlDataAddEntry'."); 
        return (UTL_DataInvalidData);
    }

    if ( uiDataEntryLength <= 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiDataEntryLength' parameter passed to 'iUtlDataAddEntry'."); 
        return (UTL_DataInvalidDataLength);
    }

    if ( pulDataEntryID == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pulDataEntryID' parameter passed to 'iUtlDataAddEntry'."); 
        return (UTL_ReturnParameterError);
    }



    /* Check that we are in write mode */
    if ( pudUtlData->uiMode != UTL_DATA_MODE_WRITE ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to add an entry to the data, invalid data mode: %u.", pudUtlData->uiMode); 
        return (UTL_DataInvalidMode);
    }


    /* Unmap the data file, ignore errors */
    iUtlDataUnMapFile(pudUtlData);


    /* Set the data entry ID from the current data file position */
    ulDataEntryID = s_ftell(pudUtlData->pfFile);


    /* Encode the data entry length into the data entry header */
    pucDataEntryHeaderPtr = pucDataEntryHeader;
    UTL_NUM_WRITE_COMPRESSED_UINT(uiDataEntryLength, pucDataEntryHeaderPtr);
    
    /* Write out the data entry header to the data file */
    if ( s_fwrite(pucDataEntryHeader, pucDataEntryHeaderPtr - pucDataEntryHeader, 1, pudUtlData->pfFile) != 1 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to write the data entry header to the data file."); 
        return (UTL_DataWriteFailed);
    }

    /* Write out the data entry data to the data file */
    if ( s_fwrite(pvDataEntryData, uiDataEntryLength, 1, pudUtlData->pfFile) != 1 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to write the data entry to the data file."); 
        return (UTL_DataWriteFailed);
    }


    /* Set the data entry ID return pointer */
    *pulDataEntryID = ulDataEntryID;


    return (UTL_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlDataClose()

    Purpose:    Close the data.

    Parameters: pvUtlData       data structure

    Globals:    none

    Returns:    UTL error code 

*/
int iUtlDataClose
(
    void *pvUtlData
)
{

    struct utlData      *pudUtlData = (struct utlData *)pvUtlData;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iUtlDataClose"); */


    /* Check the parameters */
    if ( pvUtlData == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvUtlData' parameter passed to 'iUtlDataClose'."); 
        return (UTL_DataInvalidData);
    }



    /* Unmap the data file, ignore errors */
    iUtlDataUnMapFile(pudUtlData);

    /* Close the data file */
    s_fclose(pudUtlData->pfFile);

    /* Finally release the data */
    s_free(pudUtlData);


    return (UTL_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlDataOpen()

    Purpose:    Open a data.

    Parameters: pucDataFilePath     data file path
                ppvUtlData          return pointer for the data structure

    Globals:    none

    Returns:    UTL error code 

*/
int iUtlDataOpen
(
    unsigned char *pucDataFilePath,
    void **ppvUtlData
)
{

    long                iError = UTL_NoError;
    struct utlData      *pudUtlData = NULL;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iUtlDataOpen - pucDataFilePath: [%s]", pucDataFilePath); */


    /* Check the parameters */
    if ( bUtlStringsIsStringNULL(pucDataFilePath) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'pucDataFilePath' parameter passed to 'iUtlDataOpen'."); 
        return (UTL_DataInvalidFilePath);
    }

    if ( ppvUtlData == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppvUtlData' parameter passed to 'iUtlDataOpen'."); 
        return (UTL_ReturnParameterError);
    }



    /* Allocate a data structure */
    if ( (pudUtlData = (struct utlData *)s_malloc((size_t)(sizeof(struct utlData)))) == NULL ) {
        return (UTL_MemError);
    }

    /* Initialize all the fields in the data structure */
    pudUtlData->uiMode = UTL_DATA_MODE_READ;
    pudUtlData->pfFile = NULL;
    pudUtlData->pvFile = NULL;
    pudUtlData->zFileLength = 0;


    /* Open the data file */
    if ( (pudUtlData->pfFile = s_fopen(pucDataFilePath, "r")) == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to open the data, data file path: '%s'.", pucDataFilePath); 
        iError = UTL_DataOpenFailed;
        goto bailFromiUtlDataOpen;
    }


    /* Map the data file */
    if ( (iError = iUtlDataMapFile(pudUtlData)) != UTL_NoError ) {
        goto bailFromiUtlDataOpen;
    }



    /* Bail label */
    bailFromiUtlDataOpen:
    
    /* Handle errors */
    if ( iError != UTL_NoError ) {
    
        /* Close the data */
        if ( pudUtlData != NULL ) {
            iUtlDataClose((void *)pudUtlData);
        }
        
        /* Clear the return pointer */
        *ppvUtlData = NULL;
    }
    else {

        /* Set the return pointer */
        *ppvUtlData = (void *)pudUtlData;
    }


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlDataGetEntry()

    Purpose:    Get an entry from the data

    Parameters: pvUtlData               data structure
                ulDataEntryID           data entry ID
                ppvDataEntryData        return pointer for the data entry data
                puiDataEntryLength      return pointer for the data entry length

    Globals:    none

    Returns:    UTL error code 

*/
int iUtlDataGetEntry
(
    void *pvUtlData,
    unsigned long ulDataEntryID, 
    void **ppvDataEntryData, 
    unsigned int *puiDataEntryLength
)
{

    int                 iError = UTL_NoError;
    struct utlData      *pudUtlData = (struct utlData *)pvUtlData;
    unsigned char       *pucDataPtr = NULL;
    unsigned int        uiEntryLength = 0;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iUtlDataGetEntry - ulDataEntryID: [%u]", ulDataEntryID); */


    /* Check the parameters */
    if ( pvUtlData == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvUtlData' parameter passed to 'iUtlDataGetEntry'."); 
        return (UTL_DataInvalidData);
    }

    if ( ulDataEntryID < 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'ulDataEntryID' parameter passed to 'iUtlDataGetEntry'."); 
        return (UTL_DataInvalidDataEntryID);
    }

    if ( ppvDataEntryData == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppvDataEntryData' parameter passed to 'iUtlDataGetEntry'."); 
        return (UTL_ReturnParameterError);
    }

    if ( puiDataEntryLength == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'puiDataEntryLength' parameter passed to 'iUtlDataGetEntry'."); 
        return (UTL_ReturnParameterError);
    }



    /* Map the data file if the data is in write mode and it is not mapped */
    if ( (pudUtlData->uiMode == UTL_DATA_MODE_WRITE) && (pudUtlData->pvFile == NULL) ) {
        if ( (iError = iUtlDataMapFile(pudUtlData)) != UTL_NoError ) {
            return (iError);
        }
    }


    /* Check that the data entry ID is sensible */
    if ( ulDataEntryID > pudUtlData->zFileLength ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to get an entry from the data, entry ID: %lu, data length: %lu.", ulDataEntryID, pudUtlData->zFileLength); 
        return (UTL_DataInvalidDataEntryID);
    }


    /* Get the data pointer */
    pucDataPtr = (unsigned char *)pudUtlData->pvFile + ulDataEntryID;

    /* Read the data entry length, advances pucDataPtr to the entry data */
    UTL_NUM_READ_COMPRESSED_UINT(uiEntryLength, pucDataPtr);

    
    /* Set the return pointers */
    *ppvDataEntryData = (void *)pucDataPtr;
    *puiDataEntryLength = uiEntryLength;


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlDataProcessEntry()

    Purpose:    Get an entry from the data

                The declaration format for the call back function is:

                    iUtlDataCallBackFunction(unsigned long ulEntryID, void *pvEntryData, 
                        unsigned int uiEntryLength, va_list ap)

                The call-back function needs to return 0 to keep processing or
                non-zero to stop processing.

    Parameters: pvUtlData                   data structure
                ulDataEntryID               data entry ID
                iUtlDataCallBackFunction    call-back function
                ...                         args (optional)

    Globals:    none

    Returns:    UTL error code 

*/
int iUtlDataProcessEntry
(
    void *pvUtlData,
    unsigned long ulDataEntryID, 
    int (*iUtlDataCallBackFunction)(),
    ...
)
{

    int                 iError = UTL_NoError;
    struct utlData      *pudUtlData = (struct utlData *)pvUtlData;
    void                *pvEntryData = NULL;
    unsigned int        uiEntryLength = 0;
    int                 iStatus = 0;
    va_list             ap;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iUtlDataProcessEntry - ulDataEntryID: [%u]", ulDataEntryID); */


    /* Check the parameters */
    if ( pvUtlData == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvUtlData' parameter passed to 'iUtlDataProcessEntry'."); 
        return (UTL_DataInvalidData);
    }

    if ( ulDataEntryID < 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'ulDataEntryID' parameter passed to 'iUtlDataProcessEntry'."); 
        return (UTL_DataInvalidDataEntryID);
    }

    if ( iUtlDataCallBackFunction == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'iUtlDataCallBackFunction' parameter passed to 'iUtlDataProcessEntry'."); 
        return (UTL_DataInvalidCallBackFunction);
    }


    
    /* Get the data entry */
    if ( (iError = iUtlDataGetEntry((void *)pudUtlData, ulDataEntryID, &pvEntryData, &uiEntryLength)) != UTL_NoError ) {
        return (iError);
    }


    /* Call the call back function */
    va_start(ap, iUtlDataCallBackFunction);
    iStatus = iUtlDataCallBackFunction(ulDataEntryID, pvEntryData, uiEntryLength, ap);
    va_end(ap);
    

    return (UTL_NoError);

}


/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlDataMapFile()

    Purpose:    Map the data file

    Parameters: pudUtlData      data structure

    Globals:    none

    Returns:    UTL error code 

*/
static int iUtlDataMapFile
(
    struct utlData *pudUtlData
)
{

    int     iError = UTL_NoError;

    
/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iUtlDataMapFile'); */


    ASSERT(pudUtlData != NULL);
    ASSERT(pudUtlData->pfFile != NULL);
    ASSERT(pudUtlData->pvFile == NULL);
    ASSERT(pudUtlData->zFileLength == 0);


    /* Flush the data file */
    s_fflush(pudUtlData->pfFile);


    /* Get the data file length */
    if ( (iError = iUtlFileGetFileLength(pudUtlData->pfFile, &pudUtlData->zFileLength)) != UTL_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the data file length, utl error: %d.", iError); 
        return (UTL_DataMappingFailed);
    }


    /* Mmap the data file */
    if ( (iError = iUtlFileMemoryMap(fileno(pudUtlData->pfFile), 0, pudUtlData->zFileLength, PROT_READ, (void **)&pudUtlData->pvFile)) != UTL_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to map in the data file, utl error: %d.", iError);
        return (UTL_DataMappingFailed);
    }


    return (UTL_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlDataUnMapFile()

    Purpose:    Unmap the data file

    Parameters: pudUtlData      data structure

    Globals:    none

    Returns:    UTL error code 

*/
static int iUtlDataUnMapFile
(
    struct utlData *pudUtlData
)
{

/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iUtlDataUnMapFile'); */


    ASSERT(pudUtlData != NULL);


    /* Unmap the data file and clear the data structure variables */
    if ( pudUtlData->pvFile != NULL ) {
        iUtlFileMemoryUnMap(pudUtlData->pvFile, pudUtlData->zFileLength);
        pudUtlData->pvFile = NULL;
        pudUtlData->zFileLength = 0;
    }


    return (UTL_NoError);

}


/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
