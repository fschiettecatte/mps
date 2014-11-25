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

    Module:     termlen.h

    Author:     Francois Schiettecatte (FS Consulting LLC.)

    Created:    16 October 1999

    Purpose:    This is the header file for termlen.c.

*/


/*---------------------------------------------------------------------------*/


#if !defined(SRCH_TERMLEN_H)
#define SRCH_TERMLEN_H


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
** Defines
*/

/* Term length maximum */
#define SRCH_TERM_LENGTH_MAXIMUM                    (1024)

/* Term length minimum */
#define SRCH_TERM_LENGTH_MINIMUM                    (1)


/* Term length maximum default, hey we can manage 'Antidisestablishmentarianism', cool!! */
#define SRCH_TERM_LENGTH_MAXIMUM_DEFAULT            (255)

/* Term length minimum default */
#define SRCH_TERM_LENGTH_MINIMUM_DEFAULT            (2)


/*---------------------------------------------------------------------------*/


/*
** Public function prototypes
*/

int iSrchTermLengthInit (struct srchIndex *psiSrchIndex, 
        unsigned int uiTermLengthMaximum, unsigned int uiTermLengthMinimum);

int iSrchTermLengthInitFromInfo (struct srchIndex *psiSrchIndex);


/*---------------------------------------------------------------------------*/


/* 
** C++ wrapper
*/

#if defined(__cplusplus)
}
#endif    /* defined(__cplusplus) */


/*---------------------------------------------------------------------------*/


#endif    /* !defined(SRCH_TERMLEN_H) */


/*---------------------------------------------------------------------------*/
