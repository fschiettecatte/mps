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

    Module:     stoplist.h

    Author:     Francois Schiettecatte (FS Consulting LLC.)

    Created:    14 May 2004

    Purpose:    This is the header file for stoplist.c.

*/


/*---------------------------------------------------------------------------*/


#if !defined(SRCH_STOPLIST_H)
#define SRCH_STOPLIST_H


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

int iSrchStopListInit (struct srchIndex *psiSrchIndex, unsigned char *pucStopListName,
        unsigned char *pucStopListFilePath);

boolean bSrchStopListOn (struct srchIndex *psiSrchIndex);

int iSrchStopListGetName (struct srchIndex *psiSrchIndex, 
        unsigned char *pucStopListName, unsigned int uiStopListNameLength);


/*---------------------------------------------------------------------------*/


/* 
** C++ wrapper
*/

#if defined(__cplusplus)
}
#endif    /* defined(__cplusplus) */


/*---------------------------------------------------------------------------*/


#endif    /* !defined(SRCH_STOPLIST_H) */


/*---------------------------------------------------------------------------*/
