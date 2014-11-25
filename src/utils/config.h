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

    Module:     config.h

    Author:     Francois Schiettecatte (FS Consulting LLC.)

    Created:    3 Sept 2010

    Purpose:    This is the include file for config.c.

*/


/*---------------------------------------------------------------------------*/


#if !defined(UTL_CONFIG_H)
#define UTL_CONFIG_H


/*---------------------------------------------------------------------------*/


/*
** Includes
*/

#include "utils.h"


/*---------------------------------------------------------------------------*/


/* C++ wrapper */
#if defined(__cplusplus)
extern "C" {
#endif    /* defined(__cplusplus) */


/*---------------------------------------------------------------------------*/


/*
** Defines
*/


/* Config file flag for iUtlConfigOpen() */
#define UTL_CONFIG_FILE_FLAG_OPTIONAL           (0)
#define UTL_CONFIG_FILE_FLAG_REQUIRED           (1)

#define UTL_CONFIG_FILE_FLAG_VALID(n)           (((n) >= UTL_CONFIG_FILE_FLAG_OPTIONAL) && \
                                                        ((n) <= UTL_CONFIG_FILE_FLAG_REQUIRED))


/* Maximum value length */
#define UTL_CONFIG_VALUE_MAXIMUM_LENGTH         (UTL_FILE_PATH_MAX)


/* Subkey separator - iUtlConfigGetSubKeys() */
#define UTL_CONFIG_SUBKEY_SEPARATOR             (unsigned char *)","


/*---------------------------------------------------------------------------*/


/*
** Public function prototypes
*/


int iUtlConfigCreate (unsigned char *pucConfigFilePath, void **ppvUtlConfig);

int iUtlConfigAddEntry (void *pvUtlConfig, unsigned char *pucConfigKey, 
        unsigned char *pucConfigValue);

int iUtlConfigAddEntry1 (void *pvUtlConfig, unsigned char *pucConfigKey, 
        unsigned char *pucConfigSubKey, unsigned char *pucConfigValue);

int iUtlConfigOpen (unsigned char *pucConfigFilePath, unsigned int uiConfigFileFlag,
        void **ppvUtlConfig);

int iUtlConfigGetValue (void *pvUtlConfig, unsigned char *pucConfigKey, 
        unsigned char *pucConfigValue, unsigned int uiConfigValueLength);

int iUtlConfigGetValue1 (void *pvUtlConfig, unsigned char *pucConfigKey, 
        unsigned char *pucConfigSubKey, unsigned char *pucConfigValue, 
        unsigned int uiConfigValueLength);

int iUtlConfigGetSubKeys (void *pvUtlConfig, unsigned char *pucConfigKey, 
        unsigned char *pucConfigSubKeys, unsigned int uiConfigSubKeysLength);

int iUtlConfigClose (void *pvUtlConfig);


/*---------------------------------------------------------------------------*/


/* 
** C++ wrapper
*/

#if defined(__cplusplus)
}
#endif    /* defined(__cplusplus) */


/*---------------------------------------------------------------------------*/


#endif    /* !defined(UTL_CONFIG_H) */


/*---------------------------------------------------------------------------*/
