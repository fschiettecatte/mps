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

    Created:    4 February 1994

    Purpose:    Header file for stoplist.c, also contains the public
                functions in stoplist.c

*/


/*---------------------------------------------------------------------------*/


#if !defined(LNG_STOPLIST_H)
#define LNG_STOPLIST_H


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

/* Language stop list names/IDs/length */
#define    LNG_STOP_LIST_NONE_NAME                  (unsigned char *)"none"
#define    LNG_STOP_LIST_GOOGLE_NAME                (unsigned char *)"google"
#define    LNG_STOP_LIST_GOOGLE_MODIFIED_NAME       (unsigned char *)"google-modified"


#define    LNG_STOP_LIST_ANY_ID                    (0)
#define    LNG_STOP_LIST_NONE_ID                   (1 << 0)
#define    LNG_STOP_LIST_GOOGLE_ID                 (1 << 1)
#define    LNG_STOP_LIST_GOOGLE_MODIFIED_ID        (1 << 2)


#define    LNG_STOP_LIST_NAME_LENGTH               (15)


/*---------------------------------------------------------------------------*/


/*
** Public function prototypes
*/

int iLngStopListCreateByName (unsigned char *pucStopListName, 
        unsigned char *pucLanguageCode, void **ppvLngStopList);

int iLngStopListCreateByID (unsigned int uiStopListID, 
        unsigned int uiLanguageID, void **ppvLngStopList);

int iLngStopListCreateByNameFromFile (unsigned char *pucStopListFilePath, 
        unsigned char *pucLanguageCode, void **ppvLngStopList);

int iLngStopListCreateByIDFromFile (unsigned char *pucStopListFilePath, 
        unsigned int uiLanguageID, void **ppvLngStopList);

int iLngStopListFree (void *pvLngStopList);

int iLngStopListGetTermList (void *pvLngStopList, 
        wchar_t ***pppwcStopListTermList, 
        unsigned int *puiStopListTermListLength);


/*---------------------------------------------------------------------------*/


/* 
** C++ wrapper
*/

#if defined(__cplusplus)
}
#endif    /* defined(__cplusplus) */


/*---------------------------------------------------------------------------*/


#endif    /* !defined(LNG_STOPLIST_H) */


/*---------------------------------------------------------------------------*/
