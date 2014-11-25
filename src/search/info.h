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

    Module:     info.h

    Author:     Francois Schiettecatte (FS Consulting LLC.)

    Created:    14 September 1995

    Purpose:    This is the header file for info.c.

*/


/*---------------------------------------------------------------------------*/


#if !defined(SRCH_INFO_H)
#define SRCH_INFO_H


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

/* Maximum length of a symbol defined in the information file */
#define SRCH_INFO_SYMBOL_MAXIMUM_LENGTH                     UTL_CONFIG_VALUE_MAXIMUM_LENGTH


/* Stop list types */
#define SRCH_INFO_STOP_LIST_TYPE_INVALID_ID                 (0)
#define SRCH_INFO_STOP_LIST_TYPE_INTERNAL_ID                (1)
#define SRCH_INFO_STOP_LIST_TYPE_FILE_ID                    (2)

#define SRCH_STOP_LIST_TYPE_VALID_ID(n)                     (((n) >= SRCH_INFO_STOP_LIST_TYPE_INTERNAL_ID) && \
                                                                    ((n) <= SRCH_INFO_STOP_LIST_TYPE_FILE_ID))


/* Field options bitmap, access macros and string */
#define    SRCH_INFO_FIELD_OPTION_NONE                      (0)
#define    SRCH_INFO_FIELD_OPTION_STEMMING                  (1 << 0)
#define    SRCH_INFO_FIELD_OPTION_STOP_TERM                 (1 << 1)
#define    SRCH_INFO_FIELD_OPTION_TERM_POSITION             (1 << 2)

#define bSrchInfoFieldOptionStemming(f)                     (((f) & SRCH_INFO_FIELD_OPTION_STEMMING) > 0 ? true : false)
#define bSrchInfoFieldOptionStopTerm(f)                     (((f) & SRCH_INFO_FIELD_OPTION_STOP_TERM) > 0 ? true : false)
#define bSrchInfoFieldOptionTermPosition(f)                 (((f) & SRCH_INFO_FIELD_OPTION_TERM_POSITION) > 0 ? true : false)

#define vSrchInfoFieldOptionSetStemmingOn(f)                ((f) |= SRCH_INFO_FIELD_OPTION_STEMMING)
#define vSrchInfoFieldOptionSetStemmingOff(f)               ((f) &= ~SRCH_INFO_FIELD_OPTION_STEMMING)
#define vSrchInfoFieldOptionSetStopTermOn(f)                ((f) |= SRCH_INFO_FIELD_OPTION_STOP_TERM)
#define vSrchInfoFieldOptionSetStopTermOff(f)               ((f) &= ~SRCH_INFO_FIELD_OPTION_STOP_TERM)
#define vSrchInfoFieldOptionSetTermPositionOn(f)            ((f) |= SRCH_INFO_FIELD_OPTION_TERM_POSITION)
#define vSrchInfoFieldOptionSetTermPositionOff(f)           ((f) &= ~SRCH_INFO_FIELD_OPTION_TERM_POSITION)



/* Field option strings */
#define SRCH_INFO_FIELD_OPTION_DEFAULTS_SYMBOL              (unsigned char *)"Defaults"

#define SRCH_INFO_FIELD_OPTION_STEMMING_ON_STRING           (unsigned char *)"StemmingOn"
#define SRCH_INFO_FIELD_OPTION_STEMMING_OFF_STRING          (unsigned char *)"StemmingOff"

#define SRCH_INFO_FIELD_OPTION_STOP_TERM_ON_STRING          (unsigned char *)"StopTermOn"
#define SRCH_INFO_FIELD_OPTION_STOP_TERM_OFF_STRING         (unsigned char *)"StopTermOff"

#define SRCH_INFO_FIELD_OPTION_TERM_POSITION_ON_STRING      (unsigned char *)"TermPositionOn"
#define SRCH_INFO_FIELD_OPTION_TERM_POSITION_OFF_STRING     (unsigned char *)"TermPositionOff"


/* Field type strings */
#define SRCH_INFO_FIELD_TYPE_NONE_STRING                    (unsigned char *)"NONE"
#define SRCH_INFO_FIELD_TYPE_CHAR_STRING                    (unsigned char *)"CHAR"
#define SRCH_INFO_FIELD_TYPE_INT_STRING                     (unsigned char *)"INT"
#define SRCH_INFO_FIELD_TYPE_LONG_STRING                    (unsigned char *)"LONG"
#define SRCH_INFO_FIELD_TYPE_FLOAT_STRING                   (unsigned char *)"FLOAT"
#define SRCH_INFO_FIELD_TYPE_DOUBLE_STRING                  (unsigned char *)"DOUBLE"

/* Field type IDs */
#define SRCH_INFO_FIELD_TYPE_NONE_ID                        (0)
#define SRCH_INFO_FIELD_TYPE_CHAR_ID                        (1)         /* String */
#define SRCH_INFO_FIELD_TYPE_INT_ID                         (2)         /* Unsigned integer - 32 bit */
#define SRCH_INFO_FIELD_TYPE_LONG_ID                        (3)         /* Unsigned long - 64 bit */
#define SRCH_INFO_FIELD_TYPE_FLOAT_ID                       (4)         /* Float - 32 bit */
#define SRCH_INFO_FIELD_TYPE_DOUBLE_ID                      (5)         /* Double - 64 bit */

#define SRCH_INFO_FIELD_TYPE_VALID(n)                       (((n) >= SRCH_INFO_FIELD_TYPE_NONE_ID) && \
                                                                    ((n) <= SRCH_INFO_FIELD_TYPE_DOUBLE_ID))


/* Field options separators */
#define SRCH_INFO_FIELD_OPTIONS_SEPARATORS                  (unsigned char *)","


/* Unfielded search field names separators */
#define SRCH_INFO_UNFIELDED_SEARCH_FIELD_NAMES_SEPARATORS   (unsigned char *)","


/*---------------------------------------------------------------------------*/


/*
** Public function prototypes
*/

int iSrchInfoGetVersionInfo (struct srchIndex *psiSrchIndex, 
        unsigned int *puiMajorVersion, unsigned int *puiMinorVersion, 
        unsigned int *puiPatchVersion);

int iSrchInfoSetVersionInfo (struct srchIndex *psiSrchIndex);


int iSrchInfoGetLanguageInfo (struct srchIndex *psiSrchIndex, 
        unsigned char *pucLanguageCode, unsigned int uiLanguageCodeLength);

int iSrchInfoSetLanguageInfo (struct srchIndex *psiSrchIndex);


int iSrchInfoGetTokenizerInfo (struct srchIndex *psiSrchIndex, 
        unsigned char *pucTokenizerName, unsigned int uiTokenizerNameLength);

int iSrchInfoSetTokenizerInfo (struct srchIndex *psiSrchIndex);


int iSrchInfoGetStemmerInfo (struct srchIndex *psiSrchIndex, 
        unsigned char *pucStemmerName, unsigned int uiStemmerNameLength);

int iSrchInfoSetStemmerInfo (struct srchIndex *psiSrchIndex);


int iSrchInfoGetStopListInfo (struct srchIndex *psiSrchIndex, 
        unsigned char *pucStopListName, unsigned int uiStopListNameLength,
        unsigned int *puiStopListTypeID);

int iSrchInfoSetStopListInfo (struct srchIndex *psiSrchIndex);


int iSrchInfoGetTermLengthInfo (struct srchIndex *psiSrchIndex, 
        unsigned int *puiTermLengthMaximum, unsigned int *puiTermLengthMinimum);

int iSrchInfoSetTermLengthInfo (struct srchIndex *psiSrchIndex);


int iSrchInfoGetFieldID (struct srchIndex *psiSrchIndex, unsigned char *pucFieldName,
        unsigned int *puiFieldID);

int iSrchInfoGetFieldInfo (struct srchIndex *psiSrchIndex, unsigned int uiFieldID, 
        unsigned char *pucFieldName, unsigned int uiFieldNamesLength, 
        unsigned char *pucFieldDescription, unsigned int uiFieldDescriptionLength,
        unsigned int *puiFieldType, unsigned int *puiFieldOptions);

int iSrchInfoSetFieldInfo (struct srchIndex *psiSrchIndex, unsigned int uiFieldID, 
        unsigned char *pucFieldName, unsigned char *pucFieldDescription,
        unsigned int uiFieldType, unsigned int uiFieldOptions);


int iSrchInfoGetUnfieldedSearchFieldNames (struct srchIndex *psiSrchIndex, 
        unsigned char *pucUnfieldedSearchFieldNames, unsigned int uiFieldNamesLength);

int iSrchInfoSetUnfieldedSearchFieldNames (struct srchIndex *psiSrchIndex, 
        unsigned char *pucUnfieldedSearchFieldNames);


int iSrchInfoGetFieldOptionDefaults (struct srchIndex *psiSrchIndex, 
        unsigned int *puiFieldOptions);


int iSrchInfoGetItemInfo (struct srchIndex *psiSrchIndex, unsigned int uiItemID,
        unsigned char *pucItemName, unsigned int uiItemNameLength, 
        unsigned char *pucMimeType, unsigned int uiMimeTypeLength);

int iSrchInfoSetItemInfo (struct srchIndex *psiSrchIndex, unsigned int uiItemID,
        unsigned char *pucItemName, unsigned char *pucMimeType);


int iSrchInfoGetDescriptionInfo (struct srchIndex *psiSrchIndex, 
        unsigned char *pucIndexDescription, unsigned int uiIndexDescriptionLength);

int iSrchInfoSetDescriptionInfo (struct srchIndex *psiSrchIndex,
        unsigned char *pucIndexDescription);


int iSrchInfoGetScalars (struct srchIndex *psiSrchIndex);

int iSrchInfoSetScalars (struct srchIndex *psiSrchIndex);


/*---------------------------------------------------------------------------*/


/* 
** C++ wrapper
*/

#if defined(__cplusplus)
}
#endif    /* defined(__cplusplus) */


/*---------------------------------------------------------------------------*/


#endif    /* !defined(SRCH_INFO_H) */


/*---------------------------------------------------------------------------*/
