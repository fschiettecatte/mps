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

    Module:     report.h

    Author:     Francois Schiettecatte (FS Consulting LLC.)

    Created:    29 November 1995

    Purpose:    This is the include file for report.c.

*/


/*---------------------------------------------------------------------------*/


#if !defined(SRCH_REPORT)
#define SRCH_REPORT


/*---------------------------------------------------------------------------*/


/*
** Includes
*/

#include "srch.h"


/*---------------------------------------------------------------------------*/


/* 
** C++ wrapper
*/

#if defined(__cplusplus)
extern "C" {
#endif    /* defined(__cplusplus) */


/*---------------------------------------------------------------------------*/


/*
** Public function prototypes
*/

int iSrchReportCreate (struct srchSearch *pssSrchSearch, void **ppvSrchReport);

int iSrchReportClose (void *pvSrchReport);

int iSrchReportCreateReport (void *pvSrchReport);

int iSrchReportCloseReport (void *pvSrchReport);

int iSrchReportAppend (void *pvSrchReport, unsigned char *pucFormat, ...);

int iSrchReportGetReportKey (void *pvSrchReport, unsigned char *pucSearchReportKey, 
        unsigned int uiSearchReportKeyLength);

int iSrchReportGetReportOffset (void *pvSrchReport, off_t *pzSearchReportOffset);

int iSrchReportGetReportSnippet (void *pvSrchReport, off_t zSearchReportStartOffset, 
        off_t zSearchReportEndOffset, unsigned char **ppucSearchReportSnippet);

int iSrchReportGetReportText (void *pvSrchReport, unsigned char *pucSearchReportKey,
        unsigned char **ppucSearchReportText);


/*---------------------------------------------------------------------------*/


/* 
** C++ wrapper
*/

#if defined(__cplusplus)
}
#endif    /* defined(__cplusplus) */


/*---------------------------------------------------------------------------*/


#endif    /* !defined(SRCH_REPORT) */


/*---------------------------------------------------------------------------*/
