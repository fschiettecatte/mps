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

    Module:     config.c

    Author:     Francois Schiettecatte (FS Consulting LLC.)

    Created:    3 Sept 2010

    Purpose:    This implements a simple storage system for key/value pairs.

                Config creation functions:

                    iUtlConfigCreate()
                    iUtlConfigAddEntry()
                    iUtlConfigAddEntry1()
                    iUtlConfigClose()


                Config access functions:

                    iUtlConfigOpen()
                    iUtlConfigGetValue()
                    iUtlConfigGetValue1()
                    iUtlConfigGetSubKeys()
                    iUtlConfigClose()

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
#define UTL_LOG_CONTEXT                     (unsigned char *)"com.fsconsult.mps.src.utils.config2"


/*---------------------------------------------------------------------------*/


/*
** Defines
*/


/* Config mode */
#define    UTL_CONFIG_MODE_INVALID                  (0)
#define    UTL_CONFIG_MODE_WRITE                    (1)
#define    UTL_CONFIG_MODE_READ                     (2)
#define    UTL_CONFIG_MODE_NO_OP                    (3)


/* Maximum entry length */
#define UTL_CONFIG_ENTRY_MAXIMUM_LENGTH             (UTL_FILE_PATH_MAX)


/* Separators */
#define UTL_CONFIG_KEY_SUBKEY_SEPARATOR             (unsigned char *)":"
#define UTL_CONFIG_KEY_VALUE_SEPARATOR              (unsigned char *)"="


/*---------------------------------------------------------------------------*/


/*
** Structures
*/


/* Config structure */
struct utlConfig {
    unsigned int    uiMode;                             /* Config mode */
    FILE            *pfFile;                            /* Config file */
};


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlConfigCreate()

    Purpose:    Create a new config.

    Parameters: pucConfigFilePath   config file path
                ppvUtlConfig        return pointer for the config structure

    Globals:    none

    Returns:    UTL error code

*/
int iUtlConfigCreate
(
    unsigned char *pucConfigFilePath,
    void **ppvUtlConfig
)
{

    long                iError = UTL_NoError;
    struct utlConfig    *pucUtlConfig = NULL;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iUtlConfigCreate - pucConfigFilePath: [%s]", pucConfigFilePath); */


    /* Check the parameters */
    if ( bUtlStringsIsStringNULL(pucConfigFilePath) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'pucConfigFilePath' parameter passed to 'iUtlConfigCreate'."); 
        return (UTL_ConfigInvalidFilePath);
    }

    if ( ppvUtlConfig == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppvUtlConfig' parameter passed to 'iUtlConfigCreate'."); 
        return (UTL_ReturnParameterError);
    }


    /* Allocate a config structure */
    if ( (pucUtlConfig = (struct utlConfig *)s_malloc((size_t)(sizeof(struct utlConfig)))) == NULL ) {
        return (UTL_MemError);
    }

    /* Initialize all the fields in the config structure */
    pucUtlConfig->uiMode = UTL_CONFIG_MODE_WRITE;
    pucUtlConfig->pfFile = NULL;


    /* Create the config file - 'w+' so we can access it while we are creating it */
    if ( (pucUtlConfig->pfFile = s_fopen(pucConfigFilePath, "w+")) == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to create the config, config file path: '%s'.", pucConfigFilePath);
        iError = UTL_ConfigCreateFailed;
        goto bailFromiUtlConfigCreate;
    }



    /* Bail label */
    bailFromiUtlConfigCreate:
    
    /* Handle errors */
    if ( iError != UTL_NoError ) {
    
        /* Close the config */
        if ( pucUtlConfig != NULL ) {
            iUtlConfigClose((void *)pucUtlConfig);
        }
        
        /* Remove the file */
        s_remove(pucConfigFilePath);

        /* Clear the return pointer */
        *ppvUtlConfig = NULL;
    }
    else {

        /* Set the return pointer */
        *ppvUtlConfig = (void *)pucUtlConfig;
    }


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlConfigAddEntry()

    Purpose:    Add a data entry.

    Parameters: pvUtlConfig         config structure
                pucConfigKey        config key
                pucConfigValue      config value

    Globals:    none

    Returns:    UTL error code 

*/
int iUtlConfigAddEntry
(
    void *pvUtlConfig, 
    unsigned char *pucConfigKey, 
    unsigned char *pucConfigValue
)
{

    struct utlConfig    *pucUtlConfig = (struct utlConfig *)pvUtlConfig;
    unsigned char       pucEntry[UTL_CONFIG_ENTRY_MAXIMUM_LENGTH + 1] = {'\0'};


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iUtlConfigAddEntry - pucConfigKey: [%s], pucConfigValue: [%s]", pucConfigKey, pucConfigValue); */


    /* Check the parameters */
    if ( pvUtlConfig == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvUtlConfig' parameter passed to 'iUtlConfigAddEntry'."); 
        return (UTL_ConfigInvalidConfig);
    }

    if ( bUtlStringsIsStringNULL(pucConfigKey) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'pucConfigKey' parameter passed to 'iUtlConfigAddEntry'."); 
        return (UTL_ConfigInvalidConfigKey);
    }

    if ( bUtlStringsIsStringNULL(pucConfigValue) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'pucConfigValue' parameter passed to 'iUtlConfigAddEntry'."); 
        return (UTL_ConfigInvalidConfigValue);
    }



    /* Check that we are in write mode */
    if ( pucUtlConfig->uiMode != UTL_CONFIG_MODE_WRITE ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to add an entry to the data, invalid data mode: %u.", pucUtlConfig->uiMode); 
        return (UTL_ConfigInvalidMode);
    }


    /* Create the config entry */
    snprintf(pucEntry, UTL_CONFIG_ENTRY_MAXIMUM_LENGTH + 1, "%s%s%s\n", pucConfigKey, UTL_CONFIG_KEY_VALUE_SEPARATOR, pucConfigValue);


    /* Write out the config entry to the file */
    if ( s_fputs(pucEntry, pucUtlConfig->pfFile) == EOF ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to write the config entry to the config file."); 
        return (UTL_ConfigWriteFailed);
    }


    return (UTL_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlConfigAddEntry1()

    Purpose:    Add a data entry.

    Parameters: pvUtlConfig         config structure
                pucConfigKey        config key
                pucConfigSubKey     config subkey
                pucConfigValue      config value

    Globals:    none

    Returns:    UTL error code 

*/
int iUtlConfigAddEntry1
(
    void *pvUtlConfig, 
    unsigned char *pucConfigKey, 
    unsigned char *pucConfigSubKey, 
    unsigned char *pucConfigValue
)
{

    struct utlConfig    *pucUtlConfig = (struct utlConfig *)pvUtlConfig;
    unsigned char       pucKey[UTL_CONFIG_ENTRY_MAXIMUM_LENGTH + 1] = {'\0'};


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iUtlConfigAddEntry1 - pucConfigKey: [%s], pucConfigSubKey: [%s], pucConfigValue: [%s]", pucConfigKey, pucConfigSubKey, pucConfigValue); */


    /* Check the parameters */
    if ( pvUtlConfig == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvUtlConfig' parameter passed to 'iUtlConfigAddEntry1'."); 
        return (UTL_ConfigInvalidConfig);
    }

    if ( bUtlStringsIsStringNULL(pucConfigKey) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'pucConfigKey' parameter passed to 'iUtlConfigAddEntry1'."); 
        return (UTL_ConfigInvalidConfigKey);
    }

    if ( bUtlStringsIsStringNULL(pucConfigSubKey) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'pucConfigSubKey' parameter passed to 'iUtlConfigAddEntry1'."); 
        return (UTL_ConfigInvalidConfigSubKey);
    }

    if ( bUtlStringsIsStringNULL(pucConfigValue) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'pucConfigValue' parameter passed to 'iUtlConfigAddEntry1'."); 
        return (UTL_ConfigInvalidConfigValue);
    }



    /* Check that we are in write mode */
    if ( pucUtlConfig->uiMode != UTL_CONFIG_MODE_WRITE ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to add an entry to the data, invalid data mode: %u.", pucUtlConfig->uiMode); 
        return (UTL_ConfigInvalidMode);
    }


    /* Create the key:
    **
    **  key:subkey
    **
    */
    snprintf(pucKey, UTL_CONFIG_ENTRY_MAXIMUM_LENGTH + 1, "%s%s%s", pucConfigKey, UTL_CONFIG_KEY_SUBKEY_SEPARATOR, pucConfigSubKey);


    /* Pass the call through to our sister method */
    return (iUtlConfigAddEntry(pvUtlConfig, pucKey, pucConfigValue));

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlConfigClose()

    Purpose:    Close the config.

    Parameters: pvUtlConfig     config structure

    Globals:    none

    Returns:    UTL error code 

*/
int iUtlConfigClose
(
    void *pvUtlConfig
)
{

    struct utlConfig    *pucUtlConfig = (struct utlConfig *)pvUtlConfig;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iUtlConfigClose"); */


    /* Check the parameters */
    if ( pvUtlConfig == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvUtlConfig' parameter passed to 'iUtlConfigClose'."); 
        return (UTL_ConfigInvalidConfig);
    }



    /* Close the config file */
    s_fclose(pucUtlConfig->pfFile);

    /* Finally release the config */
    s_free(pucUtlConfig);


    return (UTL_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlConfigOpen()

    Purpose:    Open a data.

    Parameters: pucConfigFilePath   config file path
                uiConfigFileFlag    config file flag
                ppvUtlConfig        return pointer for the config structure

    Globals:    none

    Returns:    UTL error code 

*/
int iUtlConfigOpen
(
    unsigned char *pucConfigFilePath,
    unsigned int uiConfigFileFlag,
    void **ppvUtlConfig
)
{

    long                iError = UTL_NoError;
    struct utlConfig    *pucUtlConfig = NULL;


/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iUtlConfigOpen - pucConfigFilePath: [%s]", pucConfigFilePath); */


    /* Check the parameters */
    if ( UTL_CONFIG_FILE_FLAG_VALID(uiConfigFileFlag) == false ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiConfigFileFlag' parameter passed to 'iUtlConfigOpen'."); 
        return (UTL_ConfigInvalidFileStatus);
    }

    /* The config file path is only required if the config file is required */
    if ( (bUtlStringsIsStringNULL(pucConfigFilePath) == true) && (uiConfigFileFlag == UTL_CONFIG_FILE_FLAG_REQUIRED) ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'pucConfigFilePath' parameter passed to 'iUtlConfigOpen'."); 
        return (UTL_ConfigInvalidFilePath);
    }

    if ( ppvUtlConfig == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppvUtlConfig' parameter passed to 'iUtlConfigOpen'."); 
        return (UTL_ReturnParameterError);
    }



    /* Allocate a config structure */
    if ( (pucUtlConfig = (struct utlConfig *)s_malloc((size_t)(sizeof(struct utlConfig)))) == NULL ) {
        return (UTL_MemError);
    }

    /* Initialize all the fields in the config structure */
    pucUtlConfig->uiMode = UTL_CONFIG_MODE_READ;
    pucUtlConfig->pfFile = NULL;


    /* Set the mode to no-op if the config file is optional and the config file path is NULL */
    if ( (uiConfigFileFlag == UTL_CONFIG_FILE_FLAG_OPTIONAL) && (bUtlStringsIsStringNULL(pucConfigFilePath) == true) ) {
        pucUtlConfig->uiMode = UTL_CONFIG_MODE_NO_OP;
        iError = UTL_NoError;
        goto bailFromiUtlConfigOpen;
    }


    /* Open the config file */
    if ( (pucUtlConfig->pfFile = s_fopen(pucConfigFilePath, "r")) == NULL ) {

        /* Error if the config file is required */
        if ( uiConfigFileFlag == UTL_CONFIG_FILE_FLAG_REQUIRED ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to open the config, config file path: '%s'.", pucConfigFilePath); 
            iError = UTL_ConfigOpenFailed;
        }
        
        /* Set the mode to no-op if the config file is optional */
        else if ( uiConfigFileFlag == UTL_CONFIG_FILE_FLAG_OPTIONAL ) {
            pucUtlConfig->uiMode = UTL_CONFIG_MODE_NO_OP;
            iError = UTL_NoError;
        }

        goto bailFromiUtlConfigOpen;

    }



    /* Bail label */
    bailFromiUtlConfigOpen:
    
    /* Handle errors */
    if ( iError != UTL_NoError ) {
    
        /* Close the data */
        if ( pucUtlConfig != NULL ) {
            iUtlConfigClose((void *)pucUtlConfig);
        }
        
        /* Clear the return pointer */
        *ppvUtlConfig = NULL;
    }
    else {

        /* Set the return pointer */
        *ppvUtlConfig = (void *)pucUtlConfig;
    }


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlConfigGetValue()

    Purpose:    Get a value from the config

    Parameters: pvUtlConfig             config structure
                pucConfigKey            config key
                pucConfigValue          return pointer for the config value
                uiConfigValueLength     length of the config value pointer

    Globals:    none

    Returns:    UTL error code 

*/
int iUtlConfigGetValue
(
    void *pvUtlConfig, 
    unsigned char *pucConfigKey, 
    unsigned char *pucConfigValue, 
    unsigned int uiConfigValueLength
)
{

    struct utlConfig    *pucUtlConfig = (struct utlConfig *)pvUtlConfig;
    unsigned char       pucSearchKey[UTL_CONFIG_ENTRY_MAXIMUM_LENGTH + 1] = {'\0'};
    unsigned int        uiSearchKeyLength = 0;
    unsigned char       pucEntry[UTL_CONFIG_ENTRY_MAXIMUM_LENGTH + 1] = {'\0'};
    

/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iUtlConfigGetValue - pucConfigKey: [%s]", pucConfigKey); */
/* printf("iUtlConfigGetValue - pucConfigKey: [%s]\n", pucConfigKey); */


    /* Check the parameters */
    if ( pvUtlConfig == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvUtlConfig' parameter passed to 'iUtlConfigGetValue'."); 
        return (UTL_ConfigInvalidConfig);
    }

    if ( bUtlStringsIsStringNULL(pucConfigKey) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'pucConfigKey' parameter passed to 'iUtlConfigGetValue'."); 
        return (UTL_ConfigInvalidConfigKey);
    }

    if ( pucConfigValue == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pucConfigValue' parameter passed to 'iUtlConfigGetValue'."); 
        return (UTL_ReturnParameterError);
    }

    if ( uiConfigValueLength <= 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'uiConfigValueLength' parameter passed to 'iUtlConfigGetValue'."); 
        return (UTL_ReturnParameterError);
    }



    /* Return here if the config is in no-op mode */
    if ( pucUtlConfig->uiMode == UTL_CONFIG_MODE_NO_OP ) {
        return (UTL_ConfigSymbolNotFound);
    }
    
    
    /* Flush the config file if the config is in write mode */
    if ( pucUtlConfig->uiMode == UTL_CONFIG_MODE_WRITE ) {
        s_fflush(pucUtlConfig->pfFile);
    }
    


    /* Create the search key:
    **
    **  key=
    **  key:subkey=
    **
    */
    snprintf(pucSearchKey, UTL_CONFIG_ENTRY_MAXIMUM_LENGTH + 1, "%s%s", pucConfigKey, UTL_CONFIG_KEY_VALUE_SEPARATOR);


    /* Seek to the start of the file */
    if ( s_fseek(pucUtlConfig->pfFile, 0, SEEK_SET) != 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to seek to the start of the config file.");
        return (UTL_ConfigSeekFailed);
    }


    /* Get the search key length */
    uiSearchKeyLength = s_strlen(pucSearchKey);

    
    /* Loop over the file until we run out of entries */
    while ( s_fgets(pucEntry, UTL_CONFIG_ENTRY_MAXIMUM_LENGTH, pucUtlConfig->pfFile) != NULL ) {
    
        /* Match up the length of the search key */
        if ( s_strncmp(pucSearchKey, pucEntry, uiSearchKeyLength) == 0 ) {

            /* Get a pointer to the end of the config entry */
            unsigned char *pucPtr = pucEntry + (s_strlen(pucEntry) - 1);
            
            /* And scrub any trailing newline */
            if ( *pucPtr == '\n' ) {
                *pucPtr = '\0';
            }
            
            /* Copy the value part of the entry to the return variable */
            s_strnncpy(pucConfigValue, pucEntry + uiSearchKeyLength, uiConfigValueLength);

            /* Seek to the end of the config file so that further appends go to the end of the file */
            if ( pucUtlConfig->uiMode == UTL_CONFIG_MODE_WRITE ) {
                if ( s_fseek(pucUtlConfig->pfFile, 0, SEEK_END) != 0 ) {
                    iUtlLogError(UTL_LOG_CONTEXT, "Failed to seek to the end of the config file.");
                    return (UTL_ConfigSeekFailed);
                }
            }

/* printf("iUtlConfigGetValue - pucConfigKey: [%s], pucConfigValue: [%s]\n", pucConfigKey, pucConfigValue); */

            /* Value was found, so we return */
            return (UTL_NoError);
        }
    }

    
    /* No need to seek to the end here because we have read the config file to the end */

    return (UTL_ConfigSymbolNotFound);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlConfigGetValue1()

    Purpose:    Get a value from the config

    Parameters: pvUtlConfig             config structure
                pucConfigKey            config key
                pucConfigSubKey         config subkey
                pucConfigValue          return pointer for the config value
                uiConfigValueLength     length of the config value pointer

    Globals:    none

    Returns:    UTL error code 

*/
int iUtlConfigGetValue1
(
    void *pvUtlConfig, 
    unsigned char *pucConfigKey, 
    unsigned char *pucConfigSubKey, 
    unsigned char *pucConfigValue, 
    unsigned int uiConfigValueLength
)
{

    struct utlConfig    *pucUtlConfig = (struct utlConfig *)pvUtlConfig;
    unsigned char       puKey[UTL_CONFIG_ENTRY_MAXIMUM_LENGTH + 1] = {'\0'};
    

/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iUtlConfigGetValue1 - pucConfigKey: [%s], pucConfigSubKey: [%s]", pucConfigKey, pucConfigSubKey); */
/* printf("iUtlConfigGetValue1 - pucConfigKey: [%s], pucConfigSubKey: [%s]\n", pucConfigKey, pucConfigSubKey); */


    /* Check the parameters */
    if ( pvUtlConfig == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvUtlConfig' parameter passed to 'iUtlConfigGetValue1'."); 
        return (UTL_ConfigInvalidConfig);
    }

    if ( bUtlStringsIsStringNULL(pucConfigKey) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'pucConfigKey' parameter passed to 'iUtlConfigGetValue1'."); 
        return (UTL_ConfigInvalidConfigKey);
    }

    if ( bUtlStringsIsStringNULL(pucConfigSubKey) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'pucConfigSubKey' parameter passed to 'iUtlConfigGetValue1'."); 
        return (UTL_ConfigInvalidConfigSubKey);
    }

    if ( pucConfigValue == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pucConfigValue' parameter passed to 'iUtlConfigGetValue1'."); 
        return (UTL_ReturnParameterError);
    }

    if ( uiConfigValueLength <= 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'uiConfigValueLength' parameter passed to 'iUtlConfigGetValue1'."); 
        return (UTL_ReturnParameterError);
    }



    /* Return here if the config is in no-op mode */
    if ( pucUtlConfig->uiMode == UTL_CONFIG_MODE_NO_OP ) {
        return (UTL_ConfigSymbolNotFound);
    }
    
    
    /* Create the key:
    **
    **  key:subkey
    **
    */
    snprintf(puKey, UTL_CONFIG_ENTRY_MAXIMUM_LENGTH + 1, "%s%s%s", pucConfigKey, UTL_CONFIG_KEY_SUBKEY_SEPARATOR, pucConfigSubKey);

    
    /* Pass the call through to our sister function */
    return (iUtlConfigGetValue(pvUtlConfig, puKey, pucConfigValue, uiConfigValueLength));

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iUtlConfigGetSubKeys()

    Purpose:    Get the subkeys from the config

    Parameters: pvUtlConfig             config structure
                pucConfigKey            config key
                pucConfigSubKeys        return pointer for the subkeys
                uiConfigSubKeysLength   length of the config subkeys pointer

    Globals:    none

    Returns:    UTL error code 

*/
int iUtlConfigGetSubKeys
(
    void *pvUtlConfig, 
    unsigned char *pucConfigKey, 
    unsigned char *pucConfigSubKeys, 
    unsigned int uiConfigSubKeysLength
)
{

    struct utlConfig    *pucUtlConfig = (struct utlConfig *)pvUtlConfig;
    unsigned char       pucSearchKey[UTL_CONFIG_ENTRY_MAXIMUM_LENGTH + 1] = {'\0'};
    unsigned int        uiSearchKeyLength = 0;
    unsigned char       pucEntry[UTL_CONFIG_ENTRY_MAXIMUM_LENGTH + 1] = {'\0'};
    

/*     iUtlLogDebug(UTL_LOG_CONTEXT, "iUtlConfigGetSubKeys - pucConfigKey: [%s]", pucConfigKey); */
/* printf("iUtlConfigGetSubKeys - pucSearchKey: [%s]\n", pucSearchKey); */


    /* Check the parameters */
    if ( pvUtlConfig == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvUtlConfig' parameter passed to 'iUtlConfigGetSubKeys'."); 
        return (UTL_ConfigInvalidConfig);
    }

    if ( bUtlStringsIsStringNULL(pucConfigKey) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'pucConfigKey' parameter passed to 'iUtlConfigGetSubKeys'."); 
        return (UTL_ConfigInvalidConfigKey);
    }

    if ( pucConfigSubKeys == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pucConfigSubKeys' parameter passed to 'iUtlConfigGetSubKeys'."); 
        return (UTL_ReturnParameterError);
    }

    if ( uiConfigSubKeysLength <= 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'uiConfigSubKeysLength' parameter passed to 'iUtlConfigGetSubKeys'."); 
        return (UTL_ReturnParameterError);
    }



    /* Return here if the config is in no-op mode */
    if ( pucUtlConfig->uiMode == UTL_CONFIG_MODE_NO_OP ) {
        return (UTL_ConfigSymbolNotFound);
    }
    
    
    /* Flush the config file if the config is in write mode */
    if ( pucUtlConfig->uiMode == UTL_CONFIG_MODE_WRITE ) {
        s_fflush(pucUtlConfig->pfFile);
    }


    /* Create the search key:
    **
    **  key:
    **
    */
    snprintf(pucSearchKey, UTL_CONFIG_ENTRY_MAXIMUM_LENGTH + 1, "%s%s", pucConfigKey, UTL_CONFIG_KEY_SUBKEY_SEPARATOR);


    /* Seek to the start of the file */
    if ( s_fseek(pucUtlConfig->pfFile, 0, SEEK_SET) != 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to seek to the start of the config file.");
        return (UTL_ConfigSeekFailed);
    }


    /* Clear the return variable */
    pucConfigSubKeys[0] = '\0';

    /* Get the search key length */
    uiSearchKeyLength = s_strlen(pucSearchKey);

    /* Loop over the file until we run out of entries */
    while ( s_fgets(pucEntry, UTL_CONFIG_ENTRY_MAXIMUM_LENGTH, pucUtlConfig->pfFile) != NULL ) {
    
        /* Match up the length of the search key */
        if ( (s_strncmp(pucSearchKey, pucEntry, uiSearchKeyLength) == 0) ) {
        
            /* Get a pointer to the key/value separator */
            unsigned char *pucPtr = s_strstr(pucEntry, UTL_CONFIG_KEY_VALUE_SEPARATOR);
            
            /* Null terminate on the key/value separator */
            if ( pucPtr != NULL ) {
                *pucPtr = '\0';
            }
            
            /* Append a separator to the return variable if there are already stuff there */
            if ( bUtlStringsIsStringNULL(pucConfigSubKeys) == false ) {
                s_strnncat(pucConfigSubKeys, UTL_CONFIG_SUBKEY_SEPARATOR, uiConfigSubKeysLength, uiConfigSubKeysLength);
            }
            
            /* Append the appender to the return variable */
            s_strnncat(pucConfigSubKeys, pucEntry + uiSearchKeyLength, uiConfigSubKeysLength, uiConfigSubKeysLength);
        }
    }

/* printf("iUtlConfigGetSubKeys - pucSearchKey: [%s], pucConfigSubKeys: [%s]\n", pucSearchKey, pucConfigSubKeys); */

    /* Return true if subkeys were found */
    return ((bUtlStringsIsStringNULL(pucConfigKey) == false) ? UTL_NoError : UTL_ConfigSymbolNotFound);

}


/*---------------------------------------------------------------------------*/


