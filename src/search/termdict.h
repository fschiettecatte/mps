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

    Module:     termdict.h

    Author:     Francois Schiettecatte (FS Consulting LLC.)

    Created:    14 September 1995

    Purpose:    This is the header file for termdict.c.

*/


/*---------------------------------------------------------------------------*/


#if !defined(SRCH_TERMDICT_H)
#define SRCH_TERMDICT_H


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

/* Term match */
#define    SRCH_TERMDICT_TERM_MATCH_UNKNOWN             (0)
#define    SRCH_TERMDICT_TERM_MATCH_REGULAR             (1)
#define    SRCH_TERMDICT_TERM_MATCH_STOP                (2)
#define    SRCH_TERMDICT_TERM_MATCH_WILDCARD            (3)
#define    SRCH_TERMDICT_TERM_MATCH_SOUNDEX             (4)
#define    SRCH_TERMDICT_TERM_MATCH_METAPHONE           (5)
#define    SRCH_TERMDICT_TERM_MATCH_PHONIX              (6)
#define    SRCH_TERMDICT_TERM_MATCH_TYPO                (7)
#define    SRCH_TERMDICT_TERM_MATCH_REGEX               (8)
#define    SRCH_TERMDICT_TERM_MATCH_RANGE               (9)         /* Variants of a term, 'INFORMATION', 'Information', 'information' */
#define    SRCH_TERMDICT_TERM_MATCH_TERM_RANGE          (10)        /* Term ranges, 'alpha-beta' */

#define SRCH_TERMDICT_TERM_MATCH_VALID(n)               (((n) >= SRCH_TERMDICT_TERM_MATCH_REGULAR) && \
                                                                ((n) <= SRCH_TERMDICT_TERM_MATCH_TERM_RANGE))


/*---------------------------------------------------------------------------*/


/*
** Structures
*/

/* Term information - returned by list functions */
struct srchTermDictInfo {
    unsigned char   *pucTerm;               /* Required */
    unsigned int    uiTermType;             /* Required */
    unsigned int    uiTermCount;            /* Optional */
    unsigned int    uiDocumentCount;        /* Optional */
};


/*---------------------------------------------------------------------------*/


/*
** Public function prototypes
*/

int iSrchTermDictAddTerm  (struct srchIndex *psiSrchIndex, unsigned char *pucTerm, unsigned int uiTermType,
        unsigned int uiTermCount, unsigned int uiDocumentCount, unsigned long ulIndexBlockID, 
        unsigned char *pucFieldIDBitmap, unsigned int uiFieldIDBitmapLength);


int iSrchTermDictLookup (struct srchIndex *psiSrchIndex, unsigned char *pucTerm, 
        unsigned char *pucFieldIDBitmap, unsigned int uiFieldIDBitmapLength, unsigned int *puiTermType, 
        unsigned int *puiTermCount, unsigned int *puiDocumentCount, unsigned long *pulIndexBlockID);

int iSrchTermDictLookupList (struct srchIndex *psiSrchIndex, unsigned int uiLanguageID, 
        unsigned char *pucTerm, unsigned char *pucFieldIDBitmap, unsigned int uiFieldIDBitmapLength, 
        unsigned int uiTermMatch, unsigned int uiRangeID, 
        struct srchTermDictInfo **ppstdiSrchTermDictInfos, unsigned int *puiSrchTermDictInfosLength);

#define iSrchTermDictLookupRegular(d, fibm, fibml, stdi, stdil)             iSrchTermDictLookupList((d), LNG_LANGUAGE_ANY_ID, (NULL), (fibm), (fibml), SRCH_TERMDICT_TERM_MATCH_REGULAR, SRCH_PARSER_INVALID_ID, (stdi), (stdil))
#define iSrchTermDictLookupStop(d, fibm, fibml, stdi, stdil)                iSrchTermDictLookupList((d), LNG_LANGUAGE_ANY_ID, (NULL),(fibm), (fibml), SRCH_TERMDICT_TERM_MATCH_STOP, SRCH_PARSER_INVALID_ID, (stdi), (stdil))
#define iSrchTermDictLookupWildCard(d, t, fibm, fibml, stdi, stdil)         iSrchTermDictLookupList((d), LNG_LANGUAGE_ANY_ID, (t),(fibm), (fibml), SRCH_TERMDICT_TERM_MATCH_WILDCARD, SRCH_PARSER_INVALID_ID, (stdi), (stdil))
#define iSrchTermDictLookupSoundex(d, li, t, fibm, fibml, stdi, stdil)      iSrchTermDictLookupList((d), (li), (t),(fibm), (fibml), SRCH_TERMDICT_TERM_MATCH_SOUNDEX, SRCH_PARSER_INVALID_ID, (stdi), (stdil))
#define iSrchTermDictLookupPhonix(d, li, t, fibm, fibml, stdi, stdil)       iSrchTermDictLookupList((d), (li), (t),(fibm), (fibml), SRCH_TERMDICT_TERM_MATCH_PHONIX, SRCH_PARSER_INVALID_ID, (stdi), (stdil))
#define iSrchTermDictLookupMetaphone(d, li, t, fibm, fibml, stdi, stdil)    iSrchTermDictLookupList((d), (li), (t),(fibm), (fibml), SRCH_TERMDICT_TERM_MATCH_METAPHONE, SRCH_PARSER_INVALID_ID, (stdi), (stdil))
#define iSrchTermDictLookupTypo(d, li, t, fibm, fibml, stdi, stdil)         iSrchTermDictLookupList((d), (li), (t),(fibm), (fibml), SRCH_TERMDICT_TERM_MATCH_TYPO, SRCH_PARSER_INVALID_ID, (stdi), (stdil))
#define iSrchTermDictLookupRegex(d, t, fibm, fibml, stdi, stdil)            iSrchTermDictLookupList((d), LNG_LANGUAGE_ANY_ID, (t),(fibm), (fibml), SRCH_TERMDICT_TERM_MATCH_REGEX, SRCH_PARSER_INVALID_ID, (stdi), (stdil))
#define iSrchTermDictLookupRange(d, t, fibm, fibml, rt, stdi, stdil)        iSrchTermDictLookupList((d), LNG_LANGUAGE_ANY_ID, (t),(fibm), (fibml), SRCH_TERMDICT_TERM_MATCH_RANGE, (rt), (stdi), (stdil))
#define iSrchTermDictLookupTermRange(d, t, fibm, fibml, rt, stdi, stdil)    iSrchTermDictLookupList((d), LNG_LANGUAGE_ANY_ID, (t),(fibm), (fibml), SRCH_TERMDICT_TERM_MATCH_TERM_RANGE, (rt), (stdi), (stdil))


int iSrchTermDictFreeSearchTermDictInfo (struct srchTermDictInfo *pstdiSrchTermDictInfos, unsigned int uiSrchTermDictInfosLength);


/*---------------------------------------------------------------------------*/


/* 
** C++ wrapper
*/

#if defined(__cplusplus)
}
#endif    /* defined(__cplusplus) */


/*---------------------------------------------------------------------------*/


#endif    /* !defined(SRCH_TERMDICT_H) */


/*---------------------------------------------------------------------------*/
