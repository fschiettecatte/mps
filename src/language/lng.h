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

    Module:     lng.h

    Author:     Francois Schiettecatte (FS Consulting LLC.)

    Created:    30 April 2004

    Purpose:    This is the header file to include when using the 
                language package. It is also used as the header file
                for all the files in the language package.

*/


/*---------------------------------------------------------------------------*/


#if !defined(LNG_LNG_H)
#define LNG_LNG_H


/*---------------------------------------------------------------------------*/


/*
** Includes
*/

/* Pull in the utilities */
#include "utils.h"


/* Pull in the errors codes */
#include "lng_err.h"


/* Pull in the rest of the thundering herd */
#include "charset.h"
#include "tokenizer.h"
#include "stemmer.h"
#include "soundex.h"
#include "metaphone.h"
#include "phonix.h"
#include "typo.h"
#include "stoplist.h"
#include "language.h"
#include "case.h"
#include "convert.h"
#include "location.h"
#include "unicode.h"


/*---------------------------------------------------------------------------*/


#endif    /* !defined(LNG_LNG_H) */


/*---------------------------------------------------------------------------*/
