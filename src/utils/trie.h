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

    Module:     trie.h

    Author:     Francois Schiettecatte (FS Consulting LLC.)

    Created:    24 February 1994

    Purpose:    This is the header file for trie.c.

*/


/*---------------------------------------------------------------------------*/


#if !defined(UTL_TRIE_H)
#define UTL_TRIE_H


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
** Public function prototypes
*/

int iUtlTrieCreate (void **ppvUtlTrie);

int iUtlTrieFree (void *pvUtlTrie, boolean bFreeDatum);

int iUtlTrieAdd (void *pvUtlTrie, unsigned char *pucKey, 
        void ***pppvDatum);

int iUtlTrieLookup (void *pvUtlTrie, unsigned char *pucKey, 
        void ***pppvDatum);

int iUtlTrieLoop (void *pvUtlTrie, unsigned char *pucKey, 
        int (*iUtlTrieCallBackFunction)(), ...);

int iUtlTrieGetEntryCount (void *pvUtlTrie, unsigned int *puiEntryCount);

int iUtlTriGetMemorySize (void *pvUtlTrie, size_t *pzMemorySize);

int iUtlTriePrintKeys (void *pvUtlTrie, unsigned char *pucKey, boolean bPrintData);


/*---------------------------------------------------------------------------*/


/* 
** C++ wrapper
*/

#if defined(__cplusplus)
}
#endif    /* defined(__cplusplus) */


/*---------------------------------------------------------------------------*/


#endif    /* !defined(UTL_TRIE_H) */


/*---------------------------------------------------------------------------*/
