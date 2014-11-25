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

    Module:     log.h

    Author:     Francois Schiettecatte (FS Consulting LLC.)

    Created:    24 July 1995

    Purpose:    This is the header file for log.c.

*/


/*---------------------------------------------------------------------------*/


#if !defined(UTL_LOG_H)
#define UTL_LOG_H


/*---------------------------------------------------------------------------*/


/*
** Includes
*/

#include "utils.h"


/*---------------------------------------------------------------------------*/


/* 
** C++ wrapper
*/

#if defined(__cplusplus)
extern "C" {
#endif    /* defined(__cplusplus) */


/*---------------------------------------------------------------------------*/


/*
** Defines
*/

/* Log file names */
#define UTL_LOG_FILE_STDOUT             (unsigned char *)"stdout"
#define UTL_LOG_FILE_STDERR             (unsigned char *)"stderr"

/* Log types */
#define UTL_LOG_TYPE_INVALID            (0)
#define UTL_LOG_TYPE_PROCESS            (1)
#define UTL_LOG_TYPE_THREAD             (2)
#define UTL_LOG_TYPE_FULL               (3)

#define UTL_LOG_TYPE_VALID(n)           (((n) >= UTL_LOG_TYPE_PROCESS) && \
                                                ((n) <= UTL_LOG_TYPE_FULL))


/* Log levels */
#define UTL_LOG_LEVEL_INVALID           (0)
#define UTL_LOG_LEVEL_FATAL             (1)
#define UTL_LOG_LEVEL_ERROR             (2)
#define UTL_LOG_LEVEL_WARN              (3)
#define UTL_LOG_LEVEL_INFO              (4)
#define UTL_LOG_LEVEL_DEBUG             (5)

#define UTL_LOG_LEVEL_VALID(n)          (((n) >= UTL_LOG_LEVEL_FATAL) && \
                                                ((n) <= UTL_LOG_LEVEL_DEBUG))


#define UTL_LOG_LEVEL_MINIMUM           UTL_LOG_LEVEL_FATAL
#define UTL_LOG_LEVEL_MAXIMUM           UTL_LOG_LEVEL_DEBUG


/*---------------------------------------------------------------------------*/


/*
** Macros
*/

#define iUtlLog(l, c, m, ...)           iUtlLogMessage((l), (c), (unsigned char *)__FILE__, __LINE__, (unsigned char *)__func__, false, (m), ##__VA_ARGS__)

#define iUtlLogDebug(c, m, ...)         iUtlLogMessage(UTL_LOG_LEVEL_DEBUG, (c), (unsigned char *)__FILE__, __LINE__, (unsigned char *)__func__, false, (m), ##__VA_ARGS__)
#define iUtlLogInfo(c, m, ...)          iUtlLogMessage(UTL_LOG_LEVEL_INFO,  (c), (unsigned char *)__FILE__, __LINE__, (unsigned char *)__func__, false, (m), ##__VA_ARGS__)
#define iUtlLogWarn(c, m, ...)          iUtlLogMessage(UTL_LOG_LEVEL_WARN,  (c), (unsigned char *)__FILE__, __LINE__, (unsigned char *)__func__, false, (m), ##__VA_ARGS__)
#define iUtlLogError(c, m, ...)         iUtlLogMessage(UTL_LOG_LEVEL_ERROR, (c), (unsigned char *)__FILE__, __LINE__, (unsigned char *)__func__, false, (m), ##__VA_ARGS__)
#define iUtlLogFatal(c, m, ...)         iUtlLogMessage(UTL_LOG_LEVEL_FATAL, (c), (unsigned char *)__FILE__, __LINE__, (unsigned char *)__func__, false, (m), ##__VA_ARGS__)

#define iUtlLogPanic(c, m, ...)         iUtlLogMessage(UTL_LOG_LEVEL_FATAL, (c), (unsigned char *)__FILE__, __LINE__, (unsigned char *)__func__, true,  (m), ##__VA_ARGS__)


/*---------------------------------------------------------------------------*/


/*
** Public function prototypes
*/

int iUtlLogInit (void);


int iUtlLogSetFilePath (unsigned char *pucLogFilePath);
int iUtlLogSetOwnerName (unsigned char *pucLogFileOwnerName);
int iUtlLogSetLevel (unsigned int uiLogLevel);
int iUtlLogSetType (unsigned int uiLogType);
int iUtlLogSetLine (unsigned int uiLogLine);


boolean bUtlLogIsFatal (unsigned char *pucContext);
boolean bUtlLogIsError (unsigned char *pucContext);
boolean bUtlLogIsWarn (unsigned char *pucContext);
boolean bUtlLogIsInfo (unsigned char *pucContext);
boolean bUtlLogIsDebug (unsigned char *pucContext);


int iUtlLogMessage (unsigned int uiLogLevel, unsigned char *pucContext, 
        unsigned char *pucFile, unsigned int uiLine, unsigned char *pucFunction, 
        boolean bExit, unsigned char *pucMessage, ...);


/*---------------------------------------------------------------------------*/


/* 
** C++ wrapper
*/

#if defined(__cplusplus)
}
#endif    /* defined(__cplusplus) */


/*---------------------------------------------------------------------------*/


#endif    /* ndef UTL_LOG_H */


/*---------------------------------------------------------------------------*/
