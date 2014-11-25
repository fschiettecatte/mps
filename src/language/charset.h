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

    Module:     charset.h

    Author:     Francois Schiettecatte (FS Consulting LLC.)

    Created:    18 January 2007

    Purpose:    Character set definitions.

*/


/*---------------------------------------------------------------------------*/


#if !defined(LNG_CHARSET_H)
#define LNG_CHARSET_H


/*---------------------------------------------------------------------------*/


/*
** Includes
*/

/* Pull in the utilities */
#include "utils.h"


/*---------------------------------------------------------------------------*/


/*
** Defines
*/

/* Language character set names/IDs - the IDs are set up as a bitmap, but are not meant to be used as such */
#define LNG_CHARACTER_SET_UTF_8_NAME        (unsigned char *)"UTF-8"            /* Unicode UTF-8 */
#define LNG_CHARACTER_SET_WCHAR_NAME        (unsigned char *)"WCHAR_T"          /* Unicode ISO-10646/UC-4 */

#define LNG_CHARACTER_SET_ANY_ID            (0)
#define LNG_CHARACTER_SET_UTF_8_ID          (1 << 0)
#define LNG_CHARACTER_SET_WCHAR_ID          (1 << 2)

#define LNG_CHARACTER_SET_NAME_LENGTH       (10)


/*---------------------------------------------------------------------------*/


#endif    /* !defined(LNG_CHARSET_H) */


/*---------------------------------------------------------------------------*/
