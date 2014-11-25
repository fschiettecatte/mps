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

    Module:     bitmap.h

    Author:     Francois Schiettecatte (FS Consulting LLC.)

    Created:    21 January 2006

    Purpose:    This module provides support functions for search.c for 
                processing bitmaps 

*/


#if !defined(SRCH_BITMAP_H)
#define SRCH_BITMAP_H


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
** Structures
*/

/* Bitmap structure */
struct srchBitmap {
    unsigned char   *pucBitmap;                     /* Bitmap */
    unsigned int    uiBitmapLength;                 /* Bitmap length (in bits) */
    boolean         bMappedAllocationFlag;          /* Memory mapped allocation flag */
};


/*---------------------------------------------------------------------------*/


/*
** Public function prototypes
*/

int iSrchBitmapCreate (unsigned char *pucBitmap, unsigned int uiBitmapLength,
    boolean bMappedAllocationFlag, struct srchBitmap **ppsbSrchBitmap);

int iSrchBitmapFree (struct srchBitmap *psbSrchBitmap);


int iSrchBitmapMergeXOR (struct srchBitmap *psbSrchBitmap1, 
        struct srchBitmap *psbSrchBitmap2, struct srchBitmap **ppsbSrchBitmap);

int iSrchBitmapMergeOR (struct srchBitmap *psbSrchBitmap1, 
        struct srchBitmap *psbSrchBitmap2, struct srchBitmap **ppsbSrchBitmap);

int iSrchBitmapMergeAND (struct srchBitmap *psbSrchBitmap1, 
        struct srchBitmap *psbSrchBitmap2, struct srchBitmap **ppsbSrchBitmap);

int iSrchBitmapMergeNOT (struct srchBitmap *psbSrchBitmap1, 
        struct srchBitmap *psbSrchBitmap2, struct srchBitmap **ppsbSrchBitmap);


int iSrchBitmapPrint (struct srchBitmap *psbSrchBitmap);


/*---------------------------------------------------------------------------*/


/* 
** C++ wrapper
*/

#if defined(__cplusplus)
}
#endif    /* defined(__cplusplus) */


/*---------------------------------------------------------------------------*/


#endif    /* !defined(SRCH_BITMAP_H) */


/*---------------------------------------------------------------------------*/
