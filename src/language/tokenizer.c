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

    Module:     tokenizer.c

    Author:     Francois Schiettecatte (FS Consulting LLC.)

    Created:    30 April 2004

    Purpose:    This file contains various tokenizer functions
    
                http://www.icu-project.org/

                http://mecab.sourceforge.net/

*/


/*---------------------------------------------------------------------------*/


/*
** Includes
*/

#include "lng.h"


/*---------------------------------------------------------------------------*/


/*
** Feature defines
*/

/* Context for logging */
#undef UTL_LOG_CONTEXT
#define UTL_LOG_CONTEXT                     (unsigned char *)"com.fsconsult.mps.src.language.tokenizer"



/* Enable this to remove trailing dots from tokens */
/* #define LNG_TOKENIZER_ENABLE_TRAILING_DOTS_REMOVAL_2_UN */


/* Enable MeCab if available */
#if defined(MPS_ENABLE_MECAB)

/* Enable Japanese tokenization */
#define LNG_TOKENIZER_ENABLE_MECAB_JA           (1)

/* Enable ICU transliterator for Katakana to Hiragana transliteration */
#if defined(MPS_ENABLE_ICU)
#define LNG_TOKENIZER_ENABLE_ICU_TRANSLITERATOR
#endif    /* defined(MPS_ENABLE_ICU) */

#endif    /* defined(MPS_ENABLE_MECAB) */



/* Enable ICU if available */
#if defined(MPS_ENABLE_ICU)

/* Enable Japanese tokenization */
/* #define LNG_TOKENIZER_ENABLE_ICU_JA              (1) */

/* Enable Chinese tokenization */
/* #define LNG_TOKENIZER_ENABLE_ICU_ZH              (1) */

/* Enable Korean tokenization */
/* #define LNG_TOKENIZER_ENABLE_ICU_KO              (1) */

/* Enable Thai tokenization */
#define LNG_TOKENIZER_ENABLE_ICU_TH             (1)

/* Enable CJK tokenization */
#define LNG_TOKENIZER_ENABLE_ICU_CJK            (1)

#endif    /* defined(MPS_ENABLE_ICU) */



/* Bigrams */

/* Enable Japanese tokenization using bigrams */
/* #define LNG_TOKENIZER_ENABLE_BIGRAM_JA       (1) */

/* Enable Chinese tokenization using bigrams */
#define LNG_TOKENIZER_ENABLE_BIGRAM_ZH      (1)

/* Enable Korean tokenization using bigrams */
/* #define LNG_TOKENIZER_ENABLE_BIGRAM_KO       (1) */

/* Enable Thai tokenization using bigrams */
/* #define LNG_TOKENIZER_ENABLE_BIGRAM_TH       (1) */

/* Enable CJK tokenization using bigrams */
/* #define LNG_TOKENIZER_ENABLE_BIGRAM_CJK      (1) */



/* Spaces */

/* Enable Korean tokenization using spaces */
#define LNG_TOKENIZER_ENABLE_SPACE_KO       (1)

/* Enable Thai tokenization using spaces */
/* #define LNG_TOKENIZER_ENABLE_SPACE_TH        (1) */

/* Enable CJK tokenization using spaces */
/* #define LNG_TOKENIZER_ENABLE_SPACE_CJK       (1) */




/* Check that we did not enable multiple tokenizers for Japanese (preference order: mecab, icu, bigram) */
#if ((LNG_TOKENIZER_ENABLE_MECAB_JA + LNG_TOKENIZER_ENABLE_ICU_JA + LNG_TOKENIZER_ENABLE_BIGRAM_JA) > 1)
#warning "Multiple tokenizers enabled for Japanese (ja)"

#if defined(LNG_TOKENIZER_ENABLE_MECAB_JA)
#undef LNG_TOKENIZER_ENABLE_ICU_JA
#undef LNG_TOKENIZER_ENABLE_BIGRAM_JA
#warning "Selected MeCab as the tokenizer for Japanese (ja)"

#elif defined(LNG_TOKENIZER_ENABLE_ICU_JA)
#undef LNG_TOKENIZER_ENABLE_MECAB_JA
#undef LNG_TOKENIZER_ENABLE_BIGRAM_JA
#warning "Selected ICU as the tokenizer for Japanese (ja)"

#elif defined(LNG_TOKENIZER_ENABLE_BIGRAM_JA)
#undef LNG_TOKENIZER_ENABLE_MECAB_JA
#undef LNG_TOKENIZER_ENABLE_ICU_JA
#warning "Selected MPS (bigrams) as the tokenizer for Japanese (ja)"
#endif

#endif    /* ((LNG_TOKENIZER_ENABLE_MECAB_JA + LNG_TOKENIZER_ENABLE_ICU_JA + LNG_TOKENIZER_ENABLE_BIGRAM_JA) > 1) */



/* Check that we did not enable multiple tokenizers for Chinese (preference order: bigrams, icu) */
#if ((LNG_TOKENIZER_ENABLE_ICU_ZH + LNG_TOKENIZER_ENABLE_BIGRAM_ZH) > 1)
#warning "Multiple tokenizers enabled for Chinese (zh)"

#if defined(LNG_TOKENIZER_ENABLE_BIGRAM_ZH)
#undef LNG_TOKENIZER_ENABLE_ICU_ZH
#warning "Selected MPS (bigrams) as the tokenizer for Chinese (zh)"

#elif defined(LNG_TOKENIZER_ENABLE_ICU_ZH)
#undef LNG_TOKENIZER_ENABLE_BIGRAM_ZH
#warning "Selected ICU as the tokenizer for Chinese (zh)"
#endif

#endif    /* ((LNG_TOKENIZER_ENABLE_ICU_ZH + LNG_TOKENIZER_ENABLE_BIGRAM_ZH) > 1) */



/* Check that we did not enable multiple tokenizers for Korean (preference order: spaces, icu, bigrams) */
#if ((LNG_TOKENIZER_ENABLE_ICU_KO + LNG_TOKENIZER_ENABLE_BIGRAM_KO + LNG_TOKENIZER_ENABLE_SPACE_KO) > 1)
#warning "Multiple tokenizers enabled for Korean (ko)"

#if defined(LNG_TOKENIZER_ENABLE_SPACE_KO)
#undef LNG_TOKENIZER_ENABLE_ICU_KO
#undef LNG_TOKENIZER_ENABLE_BIGRAM_KO
#warning "Selected MPS (spaces) as the tokenizer for Korean (ko)"

#elif defined(LNG_TOKENIZER_ENABLE_ICU_KO)
#undef LNG_TOKENIZER_ENABLE_BIGRAM_KO
#undef LNG_TOKENIZER_ENABLE_SPACE_KO
#warning "Selected ICU as the tokenizer for Korean (ko)"

#elif defined(LNG_TOKENIZER_ENABLE_BIGRAM_KO)
#undef LNG_TOKENIZER_ENABLE_ICU_KO
#undef LNG_TOKENIZER_ENABLE_SPACE_KO
#warning "Selected MPS (bigrams) as the tokenizer for Korean (ko)"
#endif

#endif    /* ((LNG_TOKENIZER_ENABLE_ICU_KO + LNG_TOKENIZER_ENABLE_BIGRAM_KO + LNG_TOKENIZER_ENABLE_SPACE_KO) > 1) */



/* Check that we did not enable multiple tokenizers for Thai (preference order: icu, spaces, bigrams) */
#if ((LNG_TOKENIZER_ENABLE_ICU_TH + LNG_TOKENIZER_ENABLE_BIGRAM_TH + LNG_TOKENIZER_ENABLE_SPACE_TH) > 1)
#warning "Multiple tokenizers enabled for Thai (th)"

#if defined(LNG_TOKENIZER_ENABLE_ICU_TH)
#undef LNG_TOKENIZER_ENABLE_BIGRAM_TH
#undef LNG_TOKENIZER_ENABLE_SPACE_TH
#warning "Selected ICU as the tokenizer for Thai (th)"

#elif defined(LNG_TOKENIZER_ENABLE_SPACE_TH)
#undef LNG_TOKENIZER_ENABLE_ICU_TH
#undef LNG_TOKENIZER_ENABLE_BIGRAM_TH
#warning "Selected MPS (spaces) as the tokenizer for Thai (th)"

#elif defined(LNG_TOKENIZER_ENABLE_BIGRAM_TH)
#undef LNG_TOKENIZER_ENABLE_ICU_TH
#undef LNG_TOKENIZER_ENABLE_SPACE_TH
#warning "Selected MPS (bigrams) as the tokenizer for Thai (th)"
#endif

#endif    /* ((LNG_TOKENIZER_ENABLE_ICU_TH + LNG_TOKENIZER_ENABLE_BIGRAM_TH + LNG_TOKENIZER_ENABLE_SPACE_TH) > 1) */



/* Check that we did not enable multiple tokenizers for CJK (preference order: icu, bigrams, spaces) */
#if ((LNG_TOKENIZER_ENABLE_ICU_CJK + LNG_TOKENIZER_ENABLE_BIGRAM_CJK + LNG_TOKENIZER_ENABLE_SPACE_CJK) > 1)
#warning "Multiple tokenizers enabled for CJK"

#if defined(LNG_TOKENIZER_ENABLE_ICU_CJK)
#undef LNG_TOKENIZER_ENABLE_BIGRAM_CJK
#undef LNG_TOKENIZER_ENABLE_SPACE_CJK
#warning "Selected ICU as the tokenizer for CJK"

#elif defined(LNG_TOKENIZER_ENABLE_BIGRAM_CJK)
#undef LNG_TOKENIZER_ENABLE_ICU_CJK
#undef LNG_TOKENIZER_ENABLE_SPACE_CJK
#warning "Selected MPS (bigrams) as the tokenizer for CJK"

#elif defined(LNG_TOKENIZER_ENABLE_SPACE_CJK)
#undef LNG_TOKENIZER_ENABLE_ICU_CJK
#undef LNG_TOKENIZER_ENABLE_BIGRAM_CJK
#warning "Selected MPS (spaces) as the tokenizer for CJK"
#endif

#endif    /* ((LNG_TOKENIZER_ENABLE_ICU_CJK + LNG_TOKENIZER_ENABLE_BIGRAM_CJK + LNG_TOKENIZER_ENABLE_SPACE_CJK) > 1) */



/* Check that we have a tokenizer for Japanese */
#if ((LNG_TOKENIZER_ENABLE_MECAB_JA + LNG_TOKENIZER_ENABLE_ICU_JA + LNG_TOKENIZER_ENABLE_BIGRAM_JA) > 1)
#error "Multiple tokenizers enabled for Japanese (ja)"
#elif ((LNG_TOKENIZER_ENABLE_MECAB_JA + LNG_TOKENIZER_ENABLE_ICU_JA + LNG_TOKENIZER_ENABLE_BIGRAM_JA) == 0)
#error "No tokenizers enabled for Japanese (ja)"
#endif    /* ((LNG_TOKENIZER_ENABLE_MECAB_JA + LNG_TOKENIZER_ENABLE_ICU_JA + LNG_TOKENIZER_ENABLE_BIGRAM_JA) > 1) */


/* Check that we have a tokenizer for Chinese */
#if ((LNG_TOKENIZER_ENABLE_ICU_ZH + LNG_TOKENIZER_ENABLE_BIGRAM_ZH) > 1)
#error "Multiple tokenizers enabled for Chinese (zh)"
#elif ((LNG_TOKENIZER_ENABLE_ICU_ZH + LNG_TOKENIZER_ENABLE_BIGRAM_ZH) ==0)
#error "No tokenizers enabled for Chinese (zh)"
#endif    /* ((LNG_TOKENIZER_ENABLE_ICU_ZH + LNG_TOKENIZER_ENABLE_BIGRAM_ZH) > 1) */


/* Check that we have a tokenizer for Korean */
#if ((LNG_TOKENIZER_ENABLE_ICU_KO + LNG_TOKENIZER_ENABLE_BIGRAM_KO + LNG_TOKENIZER_ENABLE_SPACE_KO) > 1)
#error "Multiple tokenizers enabled for Korean (ko)"
#elif ((LNG_TOKENIZER_ENABLE_ICU_KO + LNG_TOKENIZER_ENABLE_BIGRAM_KO + LNG_TOKENIZER_ENABLE_SPACE_KO) == 0)
#error "No tokenizers enabled for Korean (ko)"
#endif    /* ((LNG_TOKENIZER_ENABLE_ICU_KO + LNG_TOKENIZER_ENABLE_BIGRAM_KO + LNG_TOKENIZER_ENABLE_SPACE_KO) > 1) */


/* Check that we have a tokenizer for Thai */
#if ((LNG_TOKENIZER_ENABLE_ICU_TH + LNG_TOKENIZER_ENABLE_BIGRAM_TH + LNG_TOKENIZER_ENABLE_SPACE_TH) > 1)
#error "Multiple tokenizers enabled for Thai (th)"
#elif ((LNG_TOKENIZER_ENABLE_ICU_TH + LNG_TOKENIZER_ENABLE_BIGRAM_TH + LNG_TOKENIZER_ENABLE_SPACE_TH) == 0)
#error "No tokenizers enabled for Thai (th)"
#endif    /* ((LNG_TOKENIZER_ENABLE_ICU_TH + LNG_TOKENIZER_ENABLE_BIGRAM_TH + LNG_TOKENIZER_ENABLE_SPACE_TH) > 1) */


/* Check that we have a tokenizer for CJK */
#if ((LNG_TOKENIZER_ENABLE_ICU_CJK + LNG_TOKENIZER_ENABLE_BIGRAM_CJK + LNG_TOKENIZER_ENABLE_SPACE_CJK) > 1)
#error "Multiple tokenizers enabled for CJK"
#elif ((LNG_TOKENIZER_ENABLE_ICU_CJK + LNG_TOKENIZER_ENABLE_BIGRAM_CJK + LNG_TOKENIZER_ENABLE_SPACE_CJK) == 0)
#error "No tokenizers enabled for CJK"
#endif    /* ((LNG_TOKENIZER_ENABLE_ICU_CJK + LNG_TOKENIZER_ENABLE_BIGRAM_CJK + LNG_TOKENIZER_ENABLE_SPACE_CJK) > 1) */



/* Disable MeCab if we are not using it */
#if defined(MPS_ENABLE_MECAB)
#if !defined(LNG_TOKENIZER_ENABLE_MECAB_JA)
#warning "Disabling MeCab because we are not using it"
#undef MPS_ENABLE_MECAB
#endif    /* !defined(LNG_TOKENIZER_ENABLE_MECAB_JA) */
#endif    /* defined(MPS_ENABLE_MECAB) */


/* Disable ICU tokenizer if we are not using it */
#if defined(MPS_ENABLE_ICU)
#if ((LNG_TOKENIZER_ENABLE_ICU_JA + LNG_TOKENIZER_ENABLE_ICU_ZH + LNG_TOKENIZER_ENABLE_ICU_KO + LNG_TOKENIZER_ENABLE_ICU_TH + LNG_TOKENIZER_ENABLE_ICU_CJK) == 0)
#warning "Disabling ICU tokenizer because we are not using it"
#undef MPS_ENABLE_ICU
#endif    /* ((LNG_TOKENIZER_ENABLE_ICU_JA + LNG_TOKENIZER_ENABLE_ICU_ZH + LNG_TOKENIZER_ENABLE_ICU_KO + LNG_TOKENIZER_ENABLE_ICU_TH + LNG_TOKENIZER_ENABLE_ICU_CJK) == 0) */
#endif    /* defined(MPS_ENABLE_ICU) */


/*---------------------------------------------------------------------------*/


/*
** Specific includes
*/

/* MeCab specific includes */
#if defined(MPS_ENABLE_MECAB)
#include "mecab.h"
#endif    /* defined(MPS_ENABLE_MECAB) */


/* ICU specific includes */
#if defined(MPS_ENABLE_ICU) || defined(LNG_TOKENIZER_ENABLE_ICU_TRANSLITERATOR)
#include "unicode/utypes.h"
#include "unicode/ustring.h"
#include "unicode/ubrk.h"
#include "unicode/utrans.h"
#endif    /* defined(MPS_ENABLE_ICU) */


/*---------------------------------------------------------------------------*/


/*
** Specific defines
*/

/* CJK language ID, has to be out of range of LNG_LANGUAGE_##_ID */
#define LNG_TOKENIZER_CJK_ID                            (UINT_MAX)


/* MeCab specific defines */
#if defined(MPS_ENABLE_MECAB)

/* Enable this to ignore tokenizer errors */
/* #define LNG_TOKENIZER_MECAB_ENABLE_IGNORE_TOKENIZER_ERRORS */


/* Enable this for Katakana to Hiragana transliteration */
#if defined(LNG_TOKENIZER_ENABLE_ICU_TRANSLITERATOR)
#define LNG_TOKENIZER_MECAB_ENABLE_KANA_TRANSLITERATION
#else
#warning "Cannot enable Katakana to Hiragana transliteration because ICU is not enabled"
#endif    /* defined(LNG_TOKENIZER_ENABLE_ICU_TRANSLITERATOR) */


/* Additional defines if Katakana to Hiragana transliteration is enabled */
#if defined(LNG_TOKENIZER_MECAB_ENABLE_KANA_TRANSLITERATION)

/* Define the transliteration capacity, the longuest string we can transliterate */
#define LNG_TOKENIZER_MECAB_TRANSLITERATION_CAPACITY        (50)

/* Transliterator string and length */
#define LNG_TOKENIZER_MECAB_TRANSLITERATOR_STRING            "Katakana-Hiragana"
#define LNG_TOKENIZER_MECAB_TRANSLITERATOR_STRING_LENGTH    s_strlen(LNG_TOKENIZER_MECAB_TRANSLITERATOR_STRING)

/* Enable this to ignore transliteration errors */
/* #define LNG_TOKENIZER_MECAB_ENABLE_IGNORE_TRANSLITERATOR_ERRORS */

#endif    /* defined(LNG_TOKENIZER_MECAB_ENABLE_KANA_TRANSLITERATION) */

#endif    /* defined(MPS_ENABLE_MECAB) */



/* Bigram specific defines */

/* Fallback language code for CJK for bigram */
#if defined(LNG_TOKENIZER_ENABLE_BIGRAM_CJK)
#define LNG_TOKENIZER_BIGRAM_LANGUAGE_CJK_CODE        LNG_LANGUAGE_ZH_CODE
#endif    /* defined(LNG_TOKENIZER_ENABLE_BIGRAM_CJK) */



/* ICU specific defines */
#if defined(MPS_ENABLE_ICU)

/* Enable this to ignore tokenizer errors */
/* #define LNG_TOKENIZER_ICU_ENABLE_IGNORE_TOKENIZER_ERRORS */

/* Fallback language code for CJK for ICU */
#if defined(LNG_TOKENIZER_ENABLE_ICU_CJK)
#define LNG_TOKENIZER_ICU_LANGUAGE_CJK_CODE        LNG_LANGUAGE_ZH_CODE
#endif    /* defined(LNG_TOKENIZER_ENABLE_ICU_CJK) */

#endif    /* defined(MPS_ENABLE_ICU) */


/*---------------------------------------------------------------------------*/


/*
** Defines
*/


/*
** Language flags for CJK
*/

/* Language flag bits */
#define LNG_TOKENIZER_LANGUAGE_FLAG_NONE                        (0)
#define LNG_TOKENIZER_LANGUAGE_FLAG_CJK                         (1 << 0)
#define LNG_TOKENIZER_LANGUAGE_FLAG_CHINESE                     (1 << 1)
#define LNG_TOKENIZER_LANGUAGE_FLAG_JAPANESE                    (1 << 2)
#define LNG_TOKENIZER_LANGUAGE_FLAG_KOREAN                      (1 << 3)

/* Language flag macros to detect whether individual language bits are set */
#define bLngTokenizerLanguageFlagCjk(f)                         ((((f) & LNG_TOKENIZER_LANGUAGE_FLAG_CJK) > 0) ? true : false)
#define bLngTokenizerLanguageFlagChinese(f)                     ((((f) & LNG_TOKENIZER_LANGUAGE_FLAG_CHINESE) > 0) ? true : false)
#define bLngTokenizerLanguageFlagJapanese(f)                    ((((f) & LNG_TOKENIZER_LANGUAGE_FLAG_JAPANESE) > 0) ? true : false)
#define bLngTokenizerLanguageFlagKorean(f)                      ((((f) & LNG_TOKENIZER_LANGUAGE_FLAG_KOREAN) > 0) ? true : false)

/* Language flag macros to detect whether specific language bits are set */
#define bLngTokenizerLanguageFlagCjkOnly(f)                     (((f) == LNG_TOKENIZER_LANGUAGE_FLAG_CJK) ? true : false)
#define bLngTokenizerLanguageFlagChineseOnly(f)                 (((f) == LNG_TOKENIZER_LANGUAGE_FLAG_CHINESE) ? true : false)
#define bLngTokenizerLanguageFlagJapaneseOnly(f)                (((f) == LNG_TOKENIZER_LANGUAGE_FLAG_JAPANESE) ? true : false)
#define bLngTokenizerLanguageFlagKoreanOnly(f)                  (((f) == LNG_TOKENIZER_LANGUAGE_FLAG_KOREAN) ? true : false)


/*---------------------------------------------------------------------------*/


/*
** Structures
*/

/* Tokenizer function structure */
struct lngTokenizerFunction {
    unsigned int        uiTokenizerID;                                      /* Tokenizer ID */
    unsigned int        uiLanguageID;                                       /* Language ID */
    int                 (*iLngTokenizerGetTokenFunction_un)();              /* Tokenizer get token function un pointer */
    int                 (*iLngTokenizerGetComponentFunction_un)();          /* Tokenizer get component function un pointer */
    int                 (*iLngTokenizerStripTrailingsFunction_un)();        /* Tokenizer strip trailings function un pointer */
    wchar_t             *pwcLeadingPunctuationToLeave;                      /* Leading characters to leave */
    wchar_t             *pwcTrailingPunctuationToLeave;                     /* Trailing characters to leave */
};


/* Tokenizer structure */
struct lngTokenizer {

    /* Generic tokenizer fields */
    wchar_t             *pwcStringPtr;                                      /* String pointer */
    wchar_t             *pwcStringEndPtr;                                   /* String end pointer */
    wchar_t             *pwcStringRangeStartPtr;                            /* String range start pointer */
    wchar_t             *pwcStringRangeEndPtr;                              /* String range end pointer */
    wchar_t             *pwcStringCurrentPtr;                               /* String current pointer */

    unsigned int        uiStringLanguageID;                                 /* String language ID */
    unsigned int        uiStringRangeLanguageID;                            /* String range language ID */

    unsigned int        uiTokenizerID;                                      /* Tokenizer ID */
    unsigned int        uiLanguageID;                                       /* Language ID */

    /* Denormalized tokenizer fields */
    int                 (*iLngTokenizerGetTokenFunction_un)();              /* Tokenizer get token function un pointer (denormalization) */
    int                 (*iLngTokenizerGetComponentFunction_un)();          /* Tokenizer get component function un pointer (denormalization) */
    int                 (*iLngTokenizerStripTrailingsFunction_un)();        /* Tokenizer strip trailings function un pointer (denormalization) */
    wchar_t             *pwcLeadingPunctuationToLeave;                      /* Leading characters to leave (denormalization) */
    wchar_t             *pwcTrailingPunctuationToLeave;                     /* Trailing characters to leave (denormalization) */


    /* un 2 tokenizer fields */
    wchar_t             *pwcTokenStartPtr;                                  /* Token start pointer */
    wchar_t             *pwcTokenEndPtr;                                    /* Token end pointer */
    wchar_t             *pwcComponentStartPtr;                              /* Component start pointer */
    wchar_t             *pwcComponentEndPtr;                                /* Component end pointer */
    wchar_t             *pwcTokenCurrentPtr;                                /* Token current pointer */


    /* ICU tokenizer fields */
#if defined(MPS_ENABLE_ICU)
    UBreakIterator      *pUBreakIterator;                                   /* Break iterator */
    int                 iStart;                                             /* Break iterator start */
    UChar               *pUString;                                          /* String pointer */
    unsigned int        uiUStringCapacity;                                  /* String capacity */
#endif    /* defined(MPS_ENABLE_ICU) */


    /* MeCab tokenizer fields */
#if defined(MPS_ENABLE_MECAB)
    mecab_t             *pmtMecabTokenizer;                                 /* Tokenizer pointer */
    mecab_node_t        *pmnMecabNode;                                      /* Node pointer */
    void                *pvLngConverterWCharToUTF8;                         /* wchar_t utf-8 converter */
    void                *pvLngConverterUTF8ToWChar;                         /* utf-8 to wchar_t converter */
    unsigned char       *pucString;                                         /* String */
    unsigned int        uiStringCapacity;                                   /* String capacity */
    wchar_t             *pwcToken;                                          /* Token */
    unsigned int        uiTokenCapacity;                                    /* Token capacity */
    boolean             bNewString;                                         /* Set to true if we are ready for a new string */
#if defined(LNG_TOKENIZER_MECAB_ENABLE_KANA_TRANSLITERATION)
    UTransliterator     *pUTransliterator;                                  /* Transliterator for Katakana-Hiragana transliteration */
#endif    /* defined(LNG_TOKENIZER_MECAB_ENABLE_KANA_TRANSLITERATION) */
#endif    /* defined(MPS_ENABLE_MECAB) */

    boolean             bNormalized;                                        /* Set to true if the current token/component is normalized */

};


/*---------------------------------------------------------------------------*/


/*
** Private function prototypes
*/

static int iLngTokenizerGetToken1_un (struct lngTokenizer *pltLngTokenizer);

static int iLngTokenizerGetComponent1_un (struct lngTokenizer *pltLngTokenizer);

static int iLngTokenizerStripTrailings1_un (struct lngTokenizer *pltLngTokenizer, 
        wchar_t *pwcTokenStart, wchar_t *pwcTokenEnd, wchar_t **ppwcTokenEnd);


static int iLngTokenizerGetToken2_un (struct lngTokenizer *pltLngTokenizer);

static int iLngTokenizerGetComponent2_un (struct lngTokenizer *pltLngTokenizer);

static int iLngTokenizerStripTrailings2_un (struct lngTokenizer *pltLngTokenizer, 
        wchar_t *pwcTokenStart, wchar_t *pwcTokenEnd, wchar_t **ppwcTokenEnd);


#if defined(MPS_ENABLE_ICU)

static int iLngTokenizerGetToken_icu (struct lngTokenizer *pltLngTokenizer, 
        unsigned char *pucLanguageCode);

static int iLngTokenizerGetComponent_icu (struct lngTokenizer *pltLngTokenizer);

#endif    /* defined(MPS_ENABLE_ICU) */


#if defined(MPS_ENABLE_MECAB)

static int iLngTokenizerGetToken_mecab (struct lngTokenizer *pltLngTokenizer);

static int iLngTokenizerGetComponent_mecab (struct lngTokenizer *pltLngTokenizer);

static boolean bIsKatakana (wchar_t *pwcTokenStart, wchar_t *pwcTokenEnd);

#endif    /* defined(MPS_ENABLE_MECAB) */


static int iLngTokenizerGetToken_unigram (struct lngTokenizer *pltLngTokenizer, 
        unsigned char *pucLanguageCode);

static int iLngTokenizerGetComponent_unigram (struct lngTokenizer *pltLngTokenizer);


static int iLngTokenizerGetToken_bigram (struct lngTokenizer *pltLngTokenizer);

static int iLngTokenizerGetComponent_bigram (struct lngTokenizer *pltLngTokenizer);


static int iLngTokenizerPrintWideToken (unsigned char *pucLabel, 
        wchar_t *pwcTokenStart, wchar_t *pwcTokenEnd);

static int iLngTokenizerPrintToken (unsigned char *pucLabel, 
        unsigned char *pucTokenStart, unsigned char *pucTokenEnd);



/*---------------------------------------------------------------------------*/


/*
** Globals
*/


/* Tokenizer function list */
static struct lngTokenizerFunction pltfLngTokenizerFunctionListGlobal[] = 
{
    /* Split along spaces and punctuation */
    {   LNG_TOKENIZER_FSCLT_1_ID,
        LNG_LANGUAGE_ANY_ID,
        iLngTokenizerGetToken1_un,
        iLngTokenizerGetComponent1_un,
        iLngTokenizerStripTrailings1_un,
        NULL,        /* Ignored in this tokenizer */
        NULL,        /* Ignored in this tokenizer */
    },

    /* Split along spaces to generate tokens, trim leading and trailing punctuation except for the 
    ** characters in the two lists (pwcLeadingPunctuationToLeave and pwcTrailingPunctuationToLeave), 
    ** then split along punctuation to generate components 
    */
    {   LNG_TOKENIZER_FSCLT_2_ID,
        LNG_LANGUAGE_ANY_ID,
        iLngTokenizerGetToken2_un,
        iLngTokenizerGetComponent2_un,
        iLngTokenizerStripTrailings2_un,
        L"$£¢¥.",
        L"#+",
    },

    /* Terminator */
    {   LNG_TOKENIZER_ANY_ID,
        LNG_LANGUAGE_ANY_ID,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
    },
};


/*---------------------------------------------------------------------------*/


/*

    Function:   iLngTokenizerCreateByName()

    Purpose:    This function returns a language tokenizer structure for a particular 
                tokenizer name and language code combination.

    Parameters: pucConfigurationDirectoryPath   Configuration directory path (optional)
                pucTokenizerName                Tokenizer name
                pucLanguageCode                 Language code
                ppvLngTokenizer                 Return pointer for the language tokenizer structure

    Globals:    none

    Returns:    An LNG error code

*/
int iLngTokenizerCreateByName
(
    unsigned char *pucConfigurationDirectoryPath,
    unsigned char *pucTokenizerName,
    unsigned char *pucLanguageCode,
    void **ppvLngTokenizer
)
{

    int             iError = LNG_NoError;
    unsigned int    uiTokenizerID = 0;
    unsigned int    uiLanguageID = 0;


    /* Check the parameters */
/*     if ( bUtlStringsIsStringNULL(pucConfigurationDirectoryPath) == true ) { */
/*         iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pucConfigurationDirectoryPath' parameter passed to 'iLngTokenizerCreateByName'.");  */
/*         return (LNG_TokenizerInvalidConfigurationDirectoryPath); */
/*     } */

    if ( bUtlStringsIsStringNULL(pucTokenizerName) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pucTokenizerName' parameter passed to 'iLngTokenizerCreateByName'."); 
        return (LNG_LanguageInvalidTokenizerName);
    }

    if ( bUtlStringsIsStringNULL(pucLanguageCode) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pucLanguageCode' parameter passed to 'iLngTokenizerCreateByName'."); 
        return (LNG_LanguageInvalidLanguageCode);
    }

    if ( ppvLngTokenizer == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppvLngTokenizer' parameter passed to 'iLngTokenizerCreateByName'."); 
        return (LNG_ReturnParameterError);
    }


    /* Get the tokenizer ID */
    if ( (iError = iLngGetTokenizerIDFromName(pucTokenizerName, &uiTokenizerID)) != LNG_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the tokenizer ID from the tokenizer name: '%s', lng error: %d.", pucTokenizerName, iError); 
        return (iError);
    }

    /* Get the language ID */
    if ( (iError = iLngGetLanguageIDFromCode(pucLanguageCode, &uiLanguageID)) != LNG_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the language ID from the language code: '%s', lng error: %d.", pucLanguageCode, iError); 
        return (iError);
    }


    /* Pass the call onto create by IDs */
    return (iLngTokenizerCreateByID(pucConfigurationDirectoryPath, uiTokenizerID, uiLanguageID, ppvLngTokenizer));

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLngTokenizerCreateByID()

    Purpose:    This function returns a language tokenizer structure for a particular 
                tokenizer ID and language ID combination.

    Parameters: pucConfigurationDirectoryPath   Configuration directory path (optional)
                uiTokenizerID                   Tokenizer ID
                uiLanguageID                    Language ID
                ppvLngTokenizer                 Return pointer for the language tokenizer structure

    Globals:    pltfLngTokenizerFunctionListGlobal

    Returns:    An LNG error code

*/
int iLngTokenizerCreateByID
(
    unsigned char *pucConfigurationDirectoryPath,
    unsigned int uiTokenizerID,
    unsigned int uiLanguageID,
    void **ppvLngTokenizer
)
{

    int                             iError = LNG_NoError;
    struct lngTokenizerFunction     *pltfLngTokenizerFunctionPtr = pltfLngTokenizerFunctionListGlobal;
    struct lngTokenizer             *pltLngTokenizer = NULL;

    
    /* Check the parameters */
/*     if ( bUtlStringsIsStringNULL(pucConfigurationDirectoryPath) == true ) { */
/*         iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pucConfigurationDirectoryPath' parameter passed to 'iLngTokenizerCreateByID'.");  */
/*         return (LNG_TokenizerInvalidConfigurationDirectoryPath); */
/*     } */

    if ( uiTokenizerID < 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiTokenizerID' parameter passed to 'iLngTokenizerCreateByID'."); 
        return (LNG_LanguageInvalidTokenizerID);
    }
    
    if ( uiLanguageID < 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiLanguageID' parameter passed to 'iLngTokenizerCreateByID'."); 
        return (LNG_LanguageInvalidLanguageID);
    }
    
    if ( ppvLngTokenizer == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppvLngTokenizer' parameter passed to 'iLngTokenizerCreateByID'."); 
        return (LNG_ReturnParameterError);
    }


    /* Set the locale first */
    if ( (iError = iLngLocationSetLocale(LC_ALL, LNG_LOCALE_EN_US_UTF_8_NAME)) != LNG_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to set the locale to: '%s', lng error: %d", LNG_LOCALE_EN_US_UTF_8_NAME, iError);
        return (iError);
    }


    /* Canonicalize the language ID */
    if ( (iError = iLngGetCanonicalLanguageID(uiLanguageID, &uiLanguageID)) != LNG_NoError ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the canonical language ID for the language ID: %u, lng error: %d.", uiLanguageID, iError); 
        return (iError);
    }


    /* Loop over the tokenizer function structures to see which one we want */
    for ( pltfLngTokenizerFunctionPtr = pltfLngTokenizerFunctionListGlobal; pltfLngTokenizerFunctionPtr->uiTokenizerID != LNG_TOKENIZER_ANY_ID; pltfLngTokenizerFunctionPtr++ ) {

        /* Select this tokenizer function if there is a match */
        if ( ((pltfLngTokenizerFunctionPtr->uiTokenizerID == uiTokenizerID) || (pltfLngTokenizerFunctionPtr->uiTokenizerID == LNG_TOKENIZER_ANY_ID)) &&
                ((pltfLngTokenizerFunctionPtr->uiLanguageID == uiLanguageID) || (pltfLngTokenizerFunctionPtr->uiLanguageID == LNG_LANGUAGE_ANY_ID)) ) {

            /* Allocate the language tokenizer structure */
            if ( (pltLngTokenizer = (struct lngTokenizer *)s_malloc((size_t)(sizeof(struct lngTokenizer)))) == NULL ) {
                return (LNG_MemError);
            }
            
            /* Set the language tokenizer structure generic fields */
            pltLngTokenizer->pwcStringPtr = NULL;
            pltLngTokenizer->pwcStringEndPtr = NULL;
            pltLngTokenizer->pwcStringRangeStartPtr = NULL;
            pltLngTokenizer->pwcStringRangeEndPtr = NULL;
            pltLngTokenizer->pwcStringCurrentPtr = NULL;

            pltLngTokenizer->uiStringLanguageID = LNG_LANGUAGE_ANY_ID;
            pltLngTokenizer->uiStringRangeLanguageID = LNG_LANGUAGE_ANY_ID;

            pltLngTokenizer->uiTokenizerID = uiTokenizerID;
            pltLngTokenizer->uiLanguageID = uiLanguageID;


            /* Set the language tokenizer structure denormalized fields */
            pltLngTokenizer->iLngTokenizerGetTokenFunction_un = pltfLngTokenizerFunctionPtr->iLngTokenizerGetTokenFunction_un;
            pltLngTokenizer->iLngTokenizerGetComponentFunction_un = pltfLngTokenizerFunctionPtr->iLngTokenizerGetComponentFunction_un;
            pltLngTokenizer->iLngTokenizerStripTrailingsFunction_un = pltfLngTokenizerFunctionPtr->iLngTokenizerStripTrailingsFunction_un;
            pltLngTokenizer->pwcLeadingPunctuationToLeave = pltfLngTokenizerFunctionPtr->pwcLeadingPunctuationToLeave;
            pltLngTokenizer->pwcTrailingPunctuationToLeave = pltfLngTokenizerFunctionPtr->pwcTrailingPunctuationToLeave;


            /* Set the language tokenizer structure un 2 fields */
            pltLngTokenizer->pwcTokenStartPtr = NULL;
            pltLngTokenizer->pwcTokenEndPtr = NULL;
            pltLngTokenizer->pwcComponentStartPtr = NULL;
            pltLngTokenizer->pwcComponentEndPtr = NULL;
            pltLngTokenizer->pwcTokenCurrentPtr = NULL;


            /* Set the language tokenizer structure ICU fields */
#if defined(MPS_ENABLE_ICU)

            /* Initialize all other fields */
            pltLngTokenizer->pUBreakIterator = NULL;
            pltLngTokenizer->iStart = -1;
            pltLngTokenizer->pUString = NULL;
            pltLngTokenizer->uiUStringCapacity = 0;

#endif    /* defined(MPS_ENABLE_ICU) */


            /* Set the language tokenizer structure MeCab fields */
#if defined(MPS_ENABLE_MECAB)

            /* Initialize all fields, the tokenizer and the converter are created lazily when we need them */
            pltLngTokenizer->pmtMecabTokenizer = NULL;
            pltLngTokenizer->pmnMecabNode = NULL;
            pltLngTokenizer->pvLngConverterWCharToUTF8 = NULL;
            pltLngTokenizer->pvLngConverterUTF8ToWChar = NULL;
            pltLngTokenizer->pucString = NULL;
            pltLngTokenizer->uiStringCapacity = 0;
            pltLngTokenizer->pwcToken = NULL;
            pltLngTokenizer->uiTokenCapacity = 0;
            pltLngTokenizer->bNewString = true;
#if defined(LNG_TOKENIZER_MECAB_ENABLE_KANA_TRANSLITERATION)
            pltLngTokenizer->pUTransliterator = NULL;
#endif    /* defined(LNG_TOKENIZER_MECAB_ENABLE_KANA_TRANSLITERATION) */
#endif    /* defined(MPS_ENABLE_MECAB) */


            /* Set the return pointer */
            *ppvLngTokenizer = (void *)pltLngTokenizer;

            return (LNG_NoError);
        }
    }
    

    /* There is no tokenizer function available for this tokenizer name and language code combination */
    return (LNG_TokenizerInvalidTokenizer);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLngTokenizerFree()

    Purpose:    This function frees the language tokenizer structure.

    Parameters: pvLngTokenizer      Language tokenizer structure

    Globals:    none

    Returns:    An LNG error code

*/
int iLngTokenizerFree
(
    void *pvLngTokenizer
)
{

    struct lngTokenizer     *pltLngTokenizer = (struct lngTokenizer *)pvLngTokenizer;


    /* Check the parameters */
    if ( pvLngTokenizer == NULL ) {
        iUtlLogDebug(UTL_LOG_CONTEXT, "Null 'pvLngTokenizer' parameter passed to 'iLngTokenizerFree'."); 
        return (LNG_TokenizerInvalidTokenizer);
    }
    

#if defined(MPS_ENABLE_ICU)

        /* Free allocations */
        if ( pltLngTokenizer->pUBreakIterator != NULL ) {
            ubrk_close(pltLngTokenizer->pUBreakIterator);
            pltLngTokenizer->pUBreakIterator = NULL;
        }
        s_free(pltLngTokenizer->pUString);

#endif    /* defined(MPS_ENABLE_ICU) */


#if defined(MPS_ENABLE_MECAB)

        /* Free allocations */
        if ( pltLngTokenizer->pmtMecabTokenizer != NULL ) {
            mecab_destroy(pltLngTokenizer->pmtMecabTokenizer);
            pltLngTokenizer->pmtMecabTokenizer = NULL;
        }

        iLngConverterFree(pltLngTokenizer->pvLngConverterWCharToUTF8);
        pltLngTokenizer->pvLngConverterWCharToUTF8 = NULL;
        
        iLngConverterFree(pltLngTokenizer->pvLngConverterUTF8ToWChar);
        pltLngTokenizer->pvLngConverterUTF8ToWChar = NULL;

        s_free(pltLngTokenizer->pucString);
        s_free(pltLngTokenizer->pwcToken);

#if defined(LNG_TOKENIZER_MECAB_ENABLE_KANA_TRANSLITERATION)
        if ( pltLngTokenizer->pUTransliterator != NULL ) {
            utrans_close(pltLngTokenizer->pUTransliterator);
            pltLngTokenizer->pUTransliterator = NULL;
        }
#endif    /* defined(LNG_TOKENIZER_MECAB_ENABLE_KANA_TRANSLITERATION) */

#endif    /* defined(MPS_ENABLE_MECAB) */


    /* Free the language tokenizer structure */
    s_free(pltLngTokenizer);


    return (LNG_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLngTokenizerParseString()

    Purpose:    Set up tokenization for a string

    Parameters: pvLngTokenizer      Language tokenizer structure
                uiLanguageID        Language ID of the string (if known)
                pwcString           Pointer to the string being evaluated
                uiStringLength      String length

    Globals:    

    Returns:    An LNG error code

*/
int iLngTokenizerParseString
(
    void *pvLngTokenizer,
    unsigned int uiLanguageID,
    wchar_t *pwcString,
    unsigned int uiStringLength
)
{

    struct lngTokenizer     *pltLngTokenizer = (struct lngTokenizer *)pvLngTokenizer;


    /* Check the parameters */
    if ( pvLngTokenizer == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvLngTokenizer' parameter passed to 'iLngTokenizerParseString'."); 
        return (LNG_TokenizerInvalidTokenizer);
    }

    if ( bUtlStringsIsWideStringNULL(pwcString) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pwcString' parameter passed to 'iLngTokenizerParseString'."); 
        return (LNG_TokenizerInvalidString);
    }

    if ( uiStringLength <= 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiStringLength' parameter passed to 'iLngTokenizerParseString'."); 
        return (LNG_TokenizerInvalidStringLength);
    }


/*     printf("iLngTokenizerParseString - uiLanguageID: %d\n", uiLanguageID); */
/* iLngTokenizerPrintWideToken("iLngTokenizerParseString - pwcString", pwcString, pwcString + uiStringLength); */

    /* Generic tokenizer fields */
    pltLngTokenizer->pwcStringPtr = pwcString;
    pltLngTokenizer->pwcStringEndPtr = pwcString + uiStringLength;
    pltLngTokenizer->pwcStringRangeStartPtr = NULL;
    pltLngTokenizer->pwcStringRangeEndPtr = NULL;
    pltLngTokenizer->pwcStringCurrentPtr = pwcString;

    pltLngTokenizer->uiStringLanguageID = uiLanguageID;
    pltLngTokenizer->uiStringRangeLanguageID = LNG_LANGUAGE_ANY_ID;


    /* un 2 tokenizer fields */
    pltLngTokenizer->pwcTokenStartPtr = NULL;
    pltLngTokenizer->pwcTokenEndPtr = NULL;
    pltLngTokenizer->pwcComponentStartPtr = NULL;
    pltLngTokenizer->pwcComponentEndPtr = NULL;
    pltLngTokenizer->pwcTokenCurrentPtr = NULL;



    /* ICU tokenizer fields */
#if defined(MPS_ENABLE_ICU)

    /* Free allocations */
    if ( pltLngTokenizer->pUBreakIterator != NULL ) {
        ubrk_close(pltLngTokenizer->pUBreakIterator);
        pltLngTokenizer->pUBreakIterator = NULL;
    }
    
    /* Reset all the fields in the tokenizer */
    pltLngTokenizer->iStart = -1;

#endif    /* defined(MPS_ENABLE_ICU) */


    /* MeCab tokenizer fields */
#if defined(MPS_ENABLE_MECAB)

    /* Reset all the fields in the tokenizer */
    pltLngTokenizer->bNewString = true;

#endif    /* defined(MPS_ENABLE_MECAB) */


    return (LNG_NoError);

}

    
/*---------------------------------------------------------------------------*/


/*

    Function:   iLngTokenizerGetToken()

    Purpose:    Get the next token for the current string

    Parameters: pvLngTokenizer      Language tokenizer structure
                ppwcTokenStart      Return pointer for the start of the token in the string
                                    (NULL is returned if there are no more tokens)
                ppwcTokenEnd        Return pointer for the end of the token in the string
                                    (NULL is returned if there are no more tokens)

    Globals:    

    Returns:    An LNG error code

*/
int iLngTokenizerGetToken
(
    void *pvLngTokenizer,
    wchar_t **ppwcTokenStart,
    wchar_t **ppwcTokenEnd
)
{

    int                     iError = LNG_NoError;
    struct lngTokenizer     *pltLngTokenizer = (struct lngTokenizer *)pvLngTokenizer;


    /* Check the parameters */
    if ( pvLngTokenizer == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvLngTokenizer' parameter passed to 'iLngTokenizerGetToken'."); 
        return (LNG_TokenizerInvalidTokenizer);
    }

    if ( ppwcTokenStart == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppwcTokenStart' parameter passed to 'iLngTokenizerGetToken'."); 
        return (LNG_ReturnParameterError);
    }

    if ( ppwcTokenEnd == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppwcTokenEnd' parameter passed to 'iLngTokenizerGetToken'."); 
        return (LNG_ReturnParameterError);
    }


    /* Reset the normalized flag */
    pltLngTokenizer->bNormalized = false;


    /* Loop if the selected tokenizer did not return a token and we have not reached the end of the string, this means that 
    ** there is nothing more in the current range and that we should move onto the next range and select a new tokenizer
    */
    do {

/* printf("pltLngTokenizer->uiStringRangeLanguageID: %u, pltLngTokenizer->pwcStringCurrentPtr: %lu, pltLngTokenizer->pwcStringPtr: %lu, pltLngTokenizer->pwcStringEndPtr: %lu\n",  */
/*         pltLngTokenizer->uiStringRangeLanguageID, (unsigned long)pltLngTokenizer->pwcStringCurrentPtr, (unsigned long)pltLngTokenizer->pwcStringPtr, (unsigned long)pltLngTokenizer->pwcStringEndPtr); */

        /* Return if we have read to the end of the string */
        if ( pltLngTokenizer->pwcStringCurrentPtr >= pltLngTokenizer->pwcStringEndPtr ) {
            *ppwcTokenStart = NULL;
            *ppwcTokenEnd = NULL;
            return (LNG_NoError);
        }
    
    
        /* Select a new string range language if we have reached the end of the current range */
        if ( pltLngTokenizer->pwcStringCurrentPtr >= pltLngTokenizer->pwcStringRangeEndPtr ) {
        
            /* Set the default string range language ID */
            pltLngTokenizer->uiStringRangeLanguageID = LNG_LANGUAGE_ANY_ID;

            /* Set the range start and range end */
            pltLngTokenizer->pwcStringRangeStartPtr = pltLngTokenizer->pwcStringCurrentPtr;
            pltLngTokenizer->pwcStringRangeEndPtr = NULL;
    
            /* CJK range */
            if ( LNG_UNICODE_ENTIRE_CJK_RANGE(*pltLngTokenizer->pwcStringRangeStartPtr) == true ) {

                /* Flags to tell us what languages has been detected in the range */
                unsigned int uiLanguageFlags = LNG_TOKENIZER_LANGUAGE_FLAG_NONE;
                
                /* Loop while we are the line */
                for ( pltLngTokenizer->pwcStringRangeEndPtr = pltLngTokenizer->pwcStringRangeStartPtr; pltLngTokenizer->pwcStringRangeEndPtr < pltLngTokenizer->pwcStringEndPtr; pltLngTokenizer->pwcStringRangeEndPtr++ ) {
                        
                    /* Detect specific languages and CJK range */
                    if ( LNG_UNICODE_JAPANESE_SPECIFIC_RANGE(*pltLngTokenizer->pwcStringRangeEndPtr) == true ) {
                        uiLanguageFlags |= LNG_TOKENIZER_LANGUAGE_FLAG_JAPANESE;
                    }
                    else if ( LNG_UNICODE_CHINESE_SPECIFIC_RANGE(*pltLngTokenizer->pwcStringRangeEndPtr) == true ) {
                        uiLanguageFlags |= LNG_TOKENIZER_LANGUAGE_FLAG_CHINESE;
                    }
                    else if ( LNG_UNICODE_KOREAN_SPECIFIC_RANGE(*pltLngTokenizer->pwcStringRangeEndPtr) == true ) {
                        uiLanguageFlags |= LNG_TOKENIZER_LANGUAGE_FLAG_KOREAN;
                    }
                    else if ( LNG_UNICODE_CJK_SHARED_RANGE(*pltLngTokenizer->pwcStringRangeEndPtr) == true ) {
                        uiLanguageFlags |= LNG_TOKENIZER_LANGUAGE_FLAG_CJK;
                    }
                    

                    /* Break out here if the character is not in the CJK range, punctuation range or space range
                    **
                    ** Punctuation and space ranges because we don't want to 'bounce' across ranges
                    */
                    if ( (LNG_UNICODE_ENTIRE_CJK_RANGE(*pltLngTokenizer->pwcStringRangeEndPtr) == false) && 
                            (LNG_UNICODE_PUNCTUATION_RANGE(*pltLngTokenizer->pwcStringRangeEndPtr) == false) &&
                            (LNG_UNICODE_SPACE_RANGE(*pltLngTokenizer->pwcStringRangeEndPtr) == false) ) {
                        break;
                    }
                }


                /* Set the string range language ID, this section takes the hint into account */

                /* Hint is Japanese and Japanese was identified */
                if ( (pltLngTokenizer->uiStringLanguageID == LNG_LANGUAGE_JA_ID) && 
                        ((bLngTokenizerLanguageFlagJapanese(uiLanguageFlags) == true) || (bLngTokenizerLanguageFlagCjk(uiLanguageFlags) == true)) ) {
                    pltLngTokenizer->uiStringRangeLanguageID = LNG_LANGUAGE_JA_ID;
                }

                /* Hint is Chinese and Chinese was identified */
                else if ( (pltLngTokenizer->uiStringLanguageID == LNG_LANGUAGE_ZH_ID) && 
                        ((bLngTokenizerLanguageFlagChinese(uiLanguageFlags) == true) || (bLngTokenizerLanguageFlagCjk(uiLanguageFlags) == true)) ) {
                    pltLngTokenizer->uiStringRangeLanguageID = LNG_LANGUAGE_ZH_ID;
                }

                /* Hint is Korean and Korean was identified */
                else if ( (pltLngTokenizer->uiStringLanguageID == LNG_LANGUAGE_KO_ID) && 
                        ((bLngTokenizerLanguageFlagKorean(uiLanguageFlags) == true) || (bLngTokenizerLanguageFlagCjk(uiLanguageFlags) == true)) ) {
                    pltLngTokenizer->uiStringRangeLanguageID = LNG_LANGUAGE_KO_ID;
                }

                /* Either there is no hint, or nothing of what we found matches the hint */
                else {

                    /* Language count */
                    unsigned int uiLanguageCount = 0;
                    
                    /* Count up the number of languages we identified */
                    uiLanguageCount += bLngTokenizerLanguageFlagChineseOnly(uiLanguageFlags) ? 1 : 0;
                    uiLanguageCount += bLngTokenizerLanguageFlagJapaneseOnly(uiLanguageFlags) ? 1 : 0;
                    uiLanguageCount += bLngTokenizerLanguageFlagKoreanOnly(uiLanguageFlags) ? 1 : 0;

                    /* More than one language was positively identified */
                    if ( uiLanguageCount > 1 ) {
                        pltLngTokenizer->uiStringRangeLanguageID = LNG_TOKENIZER_CJK_ID;
                    }
                    
                    /* Only Japanese was identified */
                    else if ( bLngTokenizerLanguageFlagJapaneseOnly(uiLanguageFlags) == true ) {
                        pltLngTokenizer->uiStringRangeLanguageID = LNG_LANGUAGE_JA_ID;
                    }

                    /* Only Chinese was identified */
                    else if ( bLngTokenizerLanguageFlagChineseOnly(uiLanguageFlags) == true ) {
                        pltLngTokenizer->uiStringRangeLanguageID = LNG_LANGUAGE_ZH_ID;
                    }

                    /* Only Korean was identified */
                    else if ( bLngTokenizerLanguageFlagKoreanOnly(uiLanguageFlags) == true ) {
                        pltLngTokenizer->uiStringRangeLanguageID = LNG_LANGUAGE_KO_ID;
                    }

                    /* Only CJK was identified */
                    else if ( bLngTokenizerLanguageFlagCjk(uiLanguageFlags) == true ) {
                        pltLngTokenizer->uiStringRangeLanguageID = LNG_TOKENIZER_CJK_ID;
                    }
                    
                    /* We got nothing, so this is a mix of digits, punctuation and spaces */
                    else {
                        pltLngTokenizer->uiStringRangeLanguageID = LNG_LANGUAGE_ANY_ID;
                    }
                }
/* printf("pltLngTokenizer->uiStringRangeLanguageID = %u\n", pltLngTokenizer->uiStringRangeLanguageID); */
            }
            
            /* Thai range */
            else if ( LNG_UNICODE_THAI_RANGE(*pltLngTokenizer->pwcStringRangeStartPtr) == true ) {
                
                /* Loop while we are in the thai range */
                for ( pltLngTokenizer->pwcStringRangeEndPtr = pltLngTokenizer->pwcStringRangeStartPtr + 1; pltLngTokenizer->pwcStringRangeEndPtr < pltLngTokenizer->pwcStringEndPtr; pltLngTokenizer->pwcStringRangeEndPtr++ ) {

                    /* Break out here if the character is not in the Thai range, punctuation or space
                    **
                    ** Punctuation and spaces because we don't want to 'bounce' across ranges
                    */
                    if ( (LNG_UNICODE_THAI_RANGE(*pltLngTokenizer->pwcStringRangeEndPtr) == false) && 
                            (LNG_UNICODE_PUNCTUATION_RANGE(*pltLngTokenizer->pwcStringRangeEndPtr) == false) &&
                            (LNG_UNICODE_SPACE_RANGE(*pltLngTokenizer->pwcStringRangeEndPtr) == false) ) {
                        break;
                    }
                }
        
                /* Set the string range language */
                pltLngTokenizer->uiStringRangeLanguageID = LNG_LANGUAGE_TH_ID;
/* printf("pltLngTokenizer->uiStringRangeLanguageID = LNG_LANGUAGE_TH_ID\n"); */
            }

            /* Everything else range */
            else {
            
                /* Set the range start and range end */
                pltLngTokenizer->pwcStringRangeStartPtr = pltLngTokenizer->pwcStringCurrentPtr;
                pltLngTokenizer->pwcStringRangeEndPtr = NULL;
            
                /* Loop while we are in the everything else range */
                for ( pltLngTokenizer->pwcStringRangeEndPtr = pltLngTokenizer->pwcStringRangeStartPtr + 1; pltLngTokenizer->pwcStringRangeEndPtr < pltLngTokenizer->pwcStringEndPtr; pltLngTokenizer->pwcStringRangeEndPtr++ ) {
                    
                    /* Break out here if the character is in the CJK range or the Thai range */
                    if ( (LNG_UNICODE_ENTIRE_CJK_RANGE(*pltLngTokenizer->pwcStringRangeEndPtr) == true) || 
                            (LNG_UNICODE_THAI_RANGE(*pltLngTokenizer->pwcStringRangeEndPtr) == true) ) {
                        break;
                    }
                }
            }

/* printf("pltLngTokenizer->uiStringRangeLanguageID: %u, pltLngTokenizer->pwcStringRangeStartPtr: %lu, pltLngTokenizer->pwcStringRangeStartPtr: %lu, pltLngTokenizer->pwcStringRangeEndPtr: %lu, length: %lu\n",  */
/*         pltLngTokenizer->uiStringRangeLanguageID, (unsigned long)pltLngTokenizer->pwcStringRangeStartPtr, (unsigned long)pltLngTokenizer->pwcStringRangeStartPtr, (unsigned long)pltLngTokenizer->pwcStringRangeEndPtr,  */
/*         (unsigned long)pltLngTokenizer->pwcStringRangeEndPtr - (unsigned long)pltLngTokenizer->pwcStringRangeStartPtr); */
    

/* Print the range */
/* { */
/*     unsigned char    pucLanguageCode[LNG_LANGUAGE_CODE_LENGTH + 2] = {'\0'}; */
/*     wchar_t            wcStringRangeEnd = L'\0'; */
/*     wcStringRangeEnd = *pltLngTokenizer->pwcStringRangeEndPtr; */
/*     *pltLngTokenizer->pwcStringRangeEndPtr = L'\0'; */
/*     if ( pltLngTokenizer->uiStringRangeLanguageID == LNG_LANGUAGE_ANY_ID ) { s_strncpy(pucLanguageCode, "un", LNG_LANGUAGE_CODE_LENGTH + 2); } */
/*     else if ( pltLngTokenizer->uiStringRangeLanguageID == LNG_TOKENIZER_CJK_ID ) { s_strncpy(pucLanguageCode, "cjk", LNG_LANGUAGE_CODE_LENGTH + 2); } */
/*     else { iLngGetLanguageCodeFromID(pltLngTokenizer->uiStringRangeLanguageID, pucLanguageCode, LNG_LANGUAGE_CODE_LENGTH + 1); } */
/*     printf("range: [%ls][%s]\n", pltLngTokenizer->pwcStringRangeStartPtr, pucLanguageCode); */
/*     *pltLngTokenizer->pwcStringRangeEndPtr = wcStringRangeEnd; */
/* } */

        }


        /* Select the tokenizer function based according to the string range language */
        if ( pltLngTokenizer->uiStringRangeLanguageID == LNG_LANGUAGE_ANY_ID ) {
            iError = pltLngTokenizer->iLngTokenizerGetTokenFunction_un(pltLngTokenizer);
        }
        else if ( pltLngTokenizer->uiStringRangeLanguageID == LNG_TOKENIZER_CJK_ID ) {

#if defined(LNG_TOKENIZER_ENABLE_BIGRAM_CJK)
            iError = iLngTokenizerGetToken_bigram(pltLngTokenizer);
#elif defined(LNG_TOKENIZER_ENABLE_ICU_CJK)
            iError = iLngTokenizerGetToken_icu(pltLngTokenizer, LNG_TOKENIZER_ICU_LANGUAGE_CJK_CODE);
#elif defined(LNG_TOKENIZER_ENABLE_SPACE_CJK)
            iError = pltLngTokenizer->iLngTokenizerGetTokenFunction_un(pltLngTokenizer);
#else
#error "No tokenizers enabled for CJK"
#endif    /* defined(LNG_TOKENIZER_ENABLE_BIGRAM_CJK) */

        }
        else if ( pltLngTokenizer->uiStringRangeLanguageID == LNG_LANGUAGE_JA_ID ) {

#if defined(LNG_TOKENIZER_ENABLE_MECAB_JA)
            iError = iLngTokenizerGetToken_mecab(pltLngTokenizer);
#elif defined(LNG_TOKENIZER_ENABLE_ICU_JA)
            iError = iLngTokenizerGetToken_icu(pltLngTokenizer, LNG_LANGUAGE_JA_CODE);
#elif defined(LNG_TOKENIZER_ENABLE_BIGRAM_JA)
            iError = iLngTokenizerGetToken_bigram(pltLngTokenizer);
#else
#error "No tokenizers enabled for Japanese (ja)"
#endif    /* defined(LNG_TOKENIZER_ENABLE_MECAB_JA) */

        }
        else if ( pltLngTokenizer->uiStringRangeLanguageID == LNG_LANGUAGE_ZH_ID ) {

#if defined(LNG_TOKENIZER_ENABLE_ICU_ZH)
            iError = iLngTokenizerGetToken_icu(pltLngTokenizer, LNG_LANGUAGE_ZH_CODE);
#elif defined(LNG_TOKENIZER_ENABLE_BIGRAM_ZH)
            iError = iLngTokenizerGetToken_bigram(pltLngTokenizer);
#else
#error "No tokenizers enabled for Chinese (zh)"
#endif    /* defined(LNG_TOKENIZER_ENABLE_BIGRAM_ZH) */

        }
        else if ( pltLngTokenizer->uiStringRangeLanguageID == LNG_LANGUAGE_KO_ID ) {

#if defined(LNG_TOKENIZER_ENABLE_ICU_KO)
            iError = iLngTokenizerGetToken_icu(pltLngTokenizer, LNG_LANGUAGE_KO_CODE);
#elif defined(LNG_TOKENIZER_ENABLE_BIGRAM_KO)
            iError = iLngTokenizerGetToken_bigram(pltLngTokenizer);
#elif defined(LNG_TOKENIZER_ENABLE_SPACE_KO)
            iError = pltLngTokenizer->iLngTokenizerGetTokenFunction_un(pltLngTokenizer);
#else
#error "No tokenizers enabled for Korean (ko)"
#endif    /* defined(LNG_TOKENIZER_ENABLE_ICU_KO) */

        }
        else if ( pltLngTokenizer->uiStringRangeLanguageID == LNG_LANGUAGE_TH_ID ) {

#if defined(LNG_TOKENIZER_ENABLE_ICU_TH)
            iError = iLngTokenizerGetToken_icu(pltLngTokenizer, LNG_LANGUAGE_TH_CODE);
#elif defined(LNG_TOKENIZER_ENABLE_SPACE_TH)
            iError = pltLngTokenizer->iLngTokenizerGetTokenFunction_un(pltLngTokenizer);
#elif defined(LNG_TOKENIZER_ENABLE_BIGRAM_TH)
            iError = iLngTokenizerGetToken_bigram(pltLngTokenizer);
#else
#error "No tokenizers enabled for Korean (ko)"
#endif    /* defined(LNG_TOKENIZER_ENABLE_ICU_TH) */

        }


    } while ( (pltLngTokenizer->pwcTokenStartPtr == NULL) && (pltLngTokenizer->pwcStringCurrentPtr < pltLngTokenizer->pwcStringEndPtr) && (iError == LNG_NoError) );
    /* Loop if the selected tokenizer did not return a token and we have not reached the end of the string, this means that 
    ** there is nothing more in the current range and that we should move onto the next range and select a new tokenizer
    */



    /* Set the return pointers if there was no error */
    if ( iError == LNG_NoError ) {
        
        /* Set the token start and end */
        *ppwcTokenStart = pltLngTokenizer->pwcTokenStartPtr;
        *ppwcTokenEnd = pltLngTokenizer->pwcTokenEndPtr;
    }


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLngTokenizerGetComponent()

    Purpose:    Get the next component for the current token in the string being parsed

    Parameters: pvLngTokenizer      Language tokenizer structure
                ppwcTokenStart      Return pointer for the start of the component in the token
                                    (NULL is returned if there are no more components)
                ppwcTokenEnd        Return pointer for the end of the component in the token
                                    (NULL is returned if there are no more components)

    Globals:    

    Returns:    An LNG error code

*/
int iLngTokenizerGetComponent
(
    void *pvLngTokenizer,
    wchar_t **ppwcComponentStart,
    wchar_t **ppwcComponentEnd
)
{

    int                     iError = LNG_NoError;
    struct lngTokenizer     *pltLngTokenizer = (struct lngTokenizer *)pvLngTokenizer;


    /* Check the parameters */
    if ( pvLngTokenizer == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvLngTokenizer' parameter passed to 'iLngTokenizerGetToken'."); 
        return (LNG_TokenizerInvalidTokenizer);
    }

    if ( ppwcComponentStart == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppwcComponentStart' parameter passed to 'iLngTokenizerGetToken'."); 
        return (LNG_ReturnParameterError);
    }

    if ( ppwcComponentEnd == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppwcComponentEnd' parameter passed to 'iLngTokenizerGetToken'."); 
        return (LNG_ReturnParameterError);
    }


    /* Reset the normalized flag */
    pltLngTokenizer->bNormalized = false;


    /* Select the tokenizer function based according to the string range language */
    if ( pltLngTokenizer->uiStringRangeLanguageID == LNG_LANGUAGE_ANY_ID ) {
        iError = pltLngTokenizer->iLngTokenizerGetComponentFunction_un(pltLngTokenizer);
    }
    else if ( pltLngTokenizer->uiStringRangeLanguageID == LNG_TOKENIZER_CJK_ID ) {

#if defined(LNG_TOKENIZER_ENABLE_BIGRAM_CJK)
        iError = iLngTokenizerGetComponent_bigram(pltLngTokenizer);
#elif defined(LNG_TOKENIZER_ENABLE_ICU_CJK)
        iError = iLngTokenizerGetComponent_icu(pltLngTokenizer);
#elif defined(LNG_TOKENIZER_ENABLE_SPACE_CJK)
        iError = pltLngTokenizer->iLngTokenizerGetComponentFunction_un(pltLngTokenizer);
#else
#error "No tokenizers enabled for CJK"
#endif    /* defined(LNG_TOKENIZER_ENABLE_BIGRAM_CJK) */

    }
    else if ( pltLngTokenizer->uiStringRangeLanguageID == LNG_LANGUAGE_JA_ID ) {

#if defined(LNG_TOKENIZER_ENABLE_MECAB_JA)
        iError = iLngTokenizerGetComponent_mecab(pltLngTokenizer);
#elif defined(LNG_TOKENIZER_ENABLE_ICU_JA)
        iError = iLngTokenizerGetComponent_icu(pltLngTokenizer);
#elif defined(LNG_TOKENIZER_ENABLE_BIGRAM_JA)
            iError = iLngTokenizerGetComponent_bigram(pltLngTokenizer);
#else
#error "No tokenizers enabled for Japanese (ja)"
#endif    /* defined(LNG_TOKENIZER_ENABLE_MECAB_JA) */

    }
    else if ( pltLngTokenizer->uiStringRangeLanguageID == LNG_LANGUAGE_ZH_ID ) {

#if defined(LNG_TOKENIZER_ENABLE_ICU_ZH)
        iError = iLngTokenizerGetComponent_icu(pltLngTokenizer);
#elif defined(LNG_TOKENIZER_ENABLE_BIGRAM_ZH)
        iError = iLngTokenizerGetComponent_bigram(pltLngTokenizer);
#else
#error "No tokenizers enabled for Chinese (zh)"
#endif    /* defined(LNG_TOKENIZER_ENABLE_BIGRAM_ZH) */

    }
    else if ( pltLngTokenizer->uiStringRangeLanguageID == LNG_LANGUAGE_KO_ID ) {

#if defined(LNG_TOKENIZER_ENABLE_ICU_KO)
            iError = iLngTokenizerGetComponent_icu(pltLngTokenizer);
#elif defined(LNG_TOKENIZER_ENABLE_BIGRAM_KO)
            iError = iLngTokenizerGetComponent_bigram(pltLngTokenizer);
#elif defined(LNG_TOKENIZER_ENABLE_SPACE_KO)
            iError = pltLngTokenizer->iLngTokenizerGetComponentFunction_un(pltLngTokenizer);
#else
#error "No tokenizers enabled for Korean (ko)"
#endif    /* defined(LNG_TOKENIZER_ENABLE_ICU_KO) */

    }
    else if ( pltLngTokenizer->uiStringRangeLanguageID == LNG_LANGUAGE_TH_ID ) {

#if defined(LNG_TOKENIZER_ENABLE_ICU_TH)
            iError = iLngTokenizerGetComponent_icu(pltLngTokenizer);
#elif defined(LNG_TOKENIZER_ENABLE_SPACE_TH)
            iError = pltLngTokenizer->iLngTokenizerGetComponentFunction_un(pltLngTokenizer);
#elif defined(LNG_TOKENIZER_ENABLE_BIGRAM_TH)
            iError = iLngTokenizerGetComponent_bigram(pltLngTokenizer);
#else
#error "No tokenizers enabled for Thai (th)"
#endif    /* defined(LNG_TOKENIZER_ENABLE_ICU_TH) */

    }


    /* Set the return pointers if there was no error */
    if ( iError == LNG_NoError ) {

        /* Set the component start and end */
        *ppwcComponentStart = pltLngTokenizer->pwcComponentStartPtr;
        *ppwcComponentEnd = pltLngTokenizer->pwcComponentEndPtr;
    }


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLngTokenizerIsTokenNormalized()

    Purpose:    Get the next component for the current token in the string being parsed

    Parameters: pvLngTokenizer      Language tokenizer structure
                pbNormalized        Return pointer to indicate whether the token has been 
                                    normalized or not

    Globals:    

    Returns:    An LNG error code

*/
int iLngTokenizerIsTokenNormalized
(
    void *pvLngTokenizer,
    boolean *pbNormalized
)
{

    struct lngTokenizer     *pltLngTokenizer = (struct lngTokenizer *)pvLngTokenizer;


    /* Check the parameters */
    if ( pvLngTokenizer == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvLngTokenizer' parameter passed to 'iLngTokenizerIsTokenNormalized'."); 
        return (LNG_TokenizerInvalidTokenizer);
    }

    if ( pbNormalized == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pbNormalized' parameter passed to 'iLngTokenizerIsTokenNormalized'."); 
        return (LNG_ReturnParameterError);
    }
    
    
    /* Set the return pointer */
    *pbNormalized = pltLngTokenizer->bNormalized;
    
    
    /* And return */
    return (LNG_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLngTokenizerStripTrailings()

    Purpose:    This function will strip the trailings off of a token 

    Parameters: pvLngTokenizer      Language tokenizer structure
                uiLanguageID        Language ID of the token (if known)
                pwcToken            Pointer to the token being stripped
                uiTokenLength       Token length
                ppwcTokenEnd        Return pointer to the end of the token

    Globals:    none

    Returns:    An LNG error code

*/
int iLngTokenizerStripTrailings
(
    void *pvLngTokenizer,
    unsigned int uiLanguageID,
    wchar_t *pwcToken,
    unsigned int uiTokenLength,
    wchar_t **ppwcTokenEnd
)
{

    struct lngTokenizer     *pltLngTokenizer = (struct lngTokenizer *)pvLngTokenizer;


    /* Check the parameters */
    if ( pvLngTokenizer == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pvLngTokenizer' parameter passed to 'iLngTokenizerStripTrailings'."); 
        return (LNG_TokenizerInvalidTokenizer);
    }

    if ( uiLanguageID < 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiLanguageID' parameter passed to 'iLngTokenizerStripTrailings'."); 
        return (LNG_LanguageInvalidLanguageID);
    }
    
    if ( pwcToken == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pwcToken' parameter passed to 'iLngTokenizerStripTrailings'."); 
        return (LNG_TokenizerInvalidToken);
    }

    if ( uiTokenLength <= 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiTokenLength' parameter passed to 'iLngTokenizerStripTrailings'."); 
        return (LNG_TokenizerInvalidTokenLength);
    }

    if ( ppwcTokenEnd == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'ppwcTokenEnd' parameter passed to 'iLngTokenizerStripTrailings'."); 
        return (LNG_ReturnParameterError);
    }


    /* Set the token start and end */
    pltLngTokenizer->pwcTokenStartPtr = pwcToken;
    pltLngTokenizer->pwcTokenEndPtr = pwcToken + uiTokenLength;


    /* Strip the trailings */
    return (pltLngTokenizer->iLngTokenizerStripTrailingsFunction_un(pltLngTokenizer, pwcToken, pwcToken + uiTokenLength, ppwcTokenEnd));

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLngTokenizerGetFeaturesString()

    Purpose:    This creates the tokenizer features string

    Parameters: pucString           string
                uiStringLength      string length

    Globals:    none

    Returns:    LNG error code

*/
int iLngTokenizerGetFeaturesString
(
    unsigned char *pucString,
    unsigned int uiStringLength
)
{

#if defined(LNG_TOKENIZER_ENABLE_ICU_CJK)
    unsigned char    pucBuffer[LNG_TOKENIZER_FEATURES_STRING_LENGTH + 1] = {'\0'};
#endif    /* defined(LNG_TOKENIZER_ENABLE_ICU_CJK) */


    /* Check the parameters */
    if ( pucString == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pucString' parameter passed to 'iLngTokenizerGetFeaturesString'."); 
        return (LNG_ReturnParameterError);
    }

    if ( uiStringLength == 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiStringLength' parameter passed to 'iLngTokenizerGetFeaturesString'."); 
        return (LNG_ReturnParameterError);
    }



    /* Create the tokenizer features string */
    snprintf(pucString, uiStringLength, "Tokenizers");


#if defined(MPS_ENABLE_MECAB)

    /* Add the tokenizer name */
    s_strnncat(pucString, " - MeCab:", uiStringLength, uiStringLength);

#if defined(LNG_TOKENIZER_ENABLE_MECAB_JA)
    s_strnncat(pucString, " Japanese,", uiStringLength, uiStringLength);
#endif    /* defined(LNG_TOKENIZER_ENABLE_MECAB_JA) */

#if defined(LNG_TOKENIZER_MECAB_ENABLE_KANA_TRANSLITERATION)
    s_strnncat(pucString, " Katakana to Hiragana transliteration,", uiStringLength, uiStringLength);
#endif    /* defined(LNG_TOKENIZER_MECAB_ENABLE_KANA_TRANSLITERATION) */

#if defined(LNG_TOKENIZER_MECAB_ENABLE_IGNORE_TOKENIZER_ERRORS)
    s_strnncat(pucString, " Ignore tokenization errors,", uiStringLength, uiStringLength);
#endif    /* defined(LNG_TOKENIZER_MECAB_ENABLE_IGNORE_TOKENIZER_ERRORS) */

    /* Remove trailing comma */
    {
        unsigned int uiLocalStringLength = s_strlen(pucString);
        if ( pucString[uiLocalStringLength - 1] == ',' ) {
            pucString[uiLocalStringLength - 1] = '\0';
        }
    }

#endif    /* defined(MPS_ENABLE_MECAB) */



#if defined(MPS_ENABLE_ICU)

    /* Add the tokenizer name */
    s_strnncat(pucString, " - ICU:", uiStringLength, uiStringLength);

#if defined(LNG_TOKENIZER_ENABLE_ICU_JA)
    s_strnncat(pucString, " Japanese,", uiStringLength, uiStringLength);
#endif    /* defined(LNG_TOKENIZER_ENABLE_ICU_JA) */

#if defined(LNG_TOKENIZER_ENABLE_ICU_ZH)
    s_strnncat(pucString, " Chinese,", uiStringLength, uiStringLength);
#endif    /* defined(LNG_TOKENIZER_ENABLE_ICU_ZH) */

#if defined(LNG_TOKENIZER_ENABLE_ICU_KO)
    s_strnncat(pucString, " Korean,", uiStringLength, uiStringLength);
#endif    /* defined(LNG_TOKENIZER_ENABLE_ICU_KO) */

#if defined(LNG_TOKENIZER_ENABLE_ICU_TH)
    s_strnncat(pucString, " Thai,", uiStringLength, uiStringLength);
#endif    /* defined(LNG_TOKENIZER_ENABLE_ICU_KO) */

#if defined(LNG_TOKENIZER_ENABLE_ICU_CJK)
    snprintf(pucBuffer, LNG_TOKENIZER_FEATURES_STRING_LENGTH, " CJK (language code: '%s'),", LNG_TOKENIZER_ICU_LANGUAGE_CJK_CODE);
    s_strnncat(pucString, pucBuffer, uiStringLength, uiStringLength);
#endif    /* defined(LNG_TOKENIZER_ENABLE_ICU_CJK) */

    /* Remove trailing comma */
    if ( pucString[s_strlen(pucString) - 1] == ',' ) {
        pucString[s_strlen(pucString) - 1] = '\0';
    }
    
#endif    /* defined(MPS_ENABLE_ICU) */



    /* Add the tokenizer name */
    s_strnncat(pucString, " - MPS:", uiStringLength, uiStringLength);

#if defined(LNG_TOKENIZER_ENABLE_BIGRAM_JA)
    s_strnncat(pucString, " Japanese (bigrams),", uiStringLength, uiStringLength);
#endif    /* defined(LNG_TOKENIZER_ENABLE_BIGRAM_JA) */

#if defined(LNG_TOKENIZER_ENABLE_BIGRAM_ZH)
    s_strnncat(pucString, " Chinese (bigrams),", uiStringLength, uiStringLength);
#endif    /* defined(LNG_TOKENIZER_ENABLE_BIGRAM_ZH) */

#if defined(LNG_TOKENIZER_ENABLE_BIGRAM_KO)
    s_strnncat(pucString, " Korean (bigrams),", uiStringLength, uiStringLength);
#elif defined(LNG_TOKENIZER_ENABLE_SPACE_KO)
    s_strnncat(pucString, " Korean (spaces),", uiStringLength, uiStringLength);
#endif    /* defined(LNG_TOKENIZER_ENABLE_BIGRAM_KO) */

#if defined(LNG_TOKENIZER_ENABLE_BIGRAM_TH)
    s_strnncat(pucString, " Thai (bigrams),", uiStringLength, uiStringLength);
#elif defined(LNG_TOKENIZER_ENABLE_SPACE_TH)
    s_strnncat(pucString, " Thai (spaces),", uiStringLength, uiStringLength);
#endif    /* defined(LNG_TOKENIZER_ENABLE_BIGRAM_TH) */

#if defined(LNG_TOKENIZER_ENABLE_BIGRAM_CJK)
    s_strnncat(pucString, " CJK (bigrams),", uiStringLength, uiStringLength);
#elif defined(LNG_TOKENIZER_ENABLE_SPACE_CJK)
    s_strnncat(pucString, " CJK (spaces),", uiStringLength, uiStringLength);
#endif    /* defined(LNG_TOKENIZER_ENABLE_BIGRAM_CJK) */

    s_strnncat(pucString, " all other languages (spaces)", uiStringLength, uiStringLength);

    /* Remove trailing comma */
    if ( pucString[s_strlen(pucString) - 1] == ',' ) {
        pucString[s_strlen(pucString) - 1] = '\0';
    }

    s_strnncat(pucString, ".", uiStringLength, uiStringLength);

    return (LNG_NoError);

}


/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/


/*

    Function:   iLngTokenizerGetToken1_un()

    Purpose:    This function will tokenize languages with space delimited tokens 

    Parameters: pltLngTokenizer     Language tokenizer structure

    Globals:    none

    Returns:    An LNG error code

*/
static int iLngTokenizerGetToken1_un
(
    struct lngTokenizer *pltLngTokenizer
)
{

    wchar_t     *pwcTokenStartPtr = NULL;
    wchar_t     *pwcTokenEndPtr = NULL;


    ASSERT(pltLngTokenizer != NULL);


/* iLngTokenizerPrintWideToken("iLngTokenizerGetToken1_un - pltLngTokenizer->pwcStringCurrentPtr", pltLngTokenizer->pwcStringCurrentPtr, pltLngTokenizer->pwcStringRangeEndPtr); */

    /* Find the start of the new token */
    for ( pwcTokenStartPtr = pltLngTokenizer->pwcStringCurrentPtr; pwcTokenStartPtr < pltLngTokenizer->pwcStringRangeEndPtr; pwcTokenStartPtr++ ) { 

        /* Break once we hit the start of a new token */
        if ( iswalnum(*pwcTokenStartPtr) != 0 ) {
            break;
        }
    }
/* printf("iLngTokenizerGetToken1_un - pwcTokenStartPtr: '%ls' (%lu)\n", pwcTokenStartPtr, (unsigned long)pwcTokenStartPtr); */


    /* Return if we have read to the end of the string */
    if ( pwcTokenStartPtr >= pltLngTokenizer->pwcStringRangeEndPtr ) {
        
        /* Set the token pointers */
        pltLngTokenizer->pwcTokenStartPtr = NULL;
        pltLngTokenizer->pwcTokenEndPtr = NULL;
        
        /* Set the string current pointer to the string end since we are done */
        pltLngTokenizer->pwcStringCurrentPtr = pltLngTokenizer->pwcStringRangeEndPtr;

        return (LNG_NoError);
    }


    /* Find the end of the new token */
    for ( pwcTokenEndPtr = pwcTokenStartPtr; pwcTokenEndPtr < pltLngTokenizer->pwcStringRangeEndPtr; pwcTokenEndPtr++ ) {

        /* Break once we hit the end of the new token */
        if ( iswalnum(*pwcTokenEndPtr) == 0 ) {
            break;
        }
    }
/* iLngTokenizerPrintWideToken("iLngTokenizerGetToken1_un - pwcTokenStartPtr", pwcTokenStartPtr, pwcTokenEndPtr); */


    /* Save the string current pointer */
    pltLngTokenizer->pwcStringCurrentPtr = pwcTokenEndPtr;


      /* Set the token pointers */
    pltLngTokenizer->pwcTokenStartPtr = pwcTokenStartPtr;
    pltLngTokenizer->pwcTokenEndPtr = pwcTokenEndPtr;


    return (LNG_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLngTokenizerGetComponent1_un()

    Purpose:    This function returns the next component for the current token
                in the current string being parsed

    Parameters: pltLngTokenizer     Language tokenizer structure

    Globals:    none

    Returns:    An LNG error code

*/
static int iLngTokenizerGetComponent1_un
(
    struct lngTokenizer *pltLngTokenizer
)
{

    ASSERT(pltLngTokenizer != NULL);


      /* Set the component pointers */
    pltLngTokenizer->pwcComponentStartPtr = NULL;
    pltLngTokenizer->pwcComponentEndPtr = NULL;


    return (LNG_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLngTokenizerStripTrailings1_un()

    Purpose:    This function will strip the trailings off of a token 

    Parameters: pltLngTokenizer     Language tokenizer structure
                pwcTokenStart       Pointer to the start of the token being stripped
                pwcTokenEnd         Pointer to the end of the token being stripped
                ppwcTokenEnd        Return pointer to the end of the token

    Globals:    none

    Returns:    An LNG error code

*/
static int iLngTokenizerStripTrailings1_un
(
    struct lngTokenizer *pltLngTokenizer,
    wchar_t *pwcTokenStart,
    wchar_t *pwcTokenEnd,
    wchar_t **ppwcTokenEnd
)
{

/*     wchar_t     *pwcTokenStartPtr = pwcTokenStart; */
    wchar_t     *pwcTokenEndPtr = pwcTokenEnd;


    ASSERT(pltLngTokenizer != NULL);
    ASSERT(pwcTokenStart != NULL);
    ASSERT(pwcTokenEnd != NULL);
    ASSERT(ppwcTokenEnd != NULL);


      /* Set the return pointer */
    *ppwcTokenEnd = pwcTokenEndPtr;


/* printf("pwcTokenStart: '%ls' (%lu)\n", pwcTokenStart, (unsigned long)pwcTokenStart); */
/* printf("pwcTokenEnd: '%ls' (%lu)\n", pwcTokenEnd, (unsigned long)pwcTokenEnd); */
/* printf("pwcTokenEndPtr: '%ls' (%lu)\n", pwcTokenEndPtr, (unsigned long)pwcTokenEndPtr); */


    return (LNG_NoError);

}


/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/


/*

    Function:   iLngTokenizerGetToken2_un()

    Purpose:    This function will tokenize languages with space delimited tokens 

    Parameters: pltLngTokenizer     Language tokenizer structure

    Globals:    none

    Returns:    An LNG error code

*/
static int iLngTokenizerGetToken2_un
(
    struct lngTokenizer *pltLngTokenizer
)
{

    wchar_t     *pwcTokenStartPtr = NULL;
    wchar_t     *pwcTokenEndPtr = NULL;


    ASSERT(pltLngTokenizer != NULL);


/* iLngTokenizerPrintWideToken("iLngTokenizerGetToken2_un - pltLngTokenizer->pwcStringCurrentPtr", pltLngTokenizer->pwcStringCurrentPtr, pltLngTokenizer->pwcStringRangeEndPtr); */

    /* Loop forever, we control the loop from within */
    while ( true ) {
        
        boolean         bIsTokenValid = false;
        boolean         bDoesTokenContainPunctuation = false;
        unsigned int    uiPunctuationDetectionState = 0;

    
        /* Find the start of the new token */
        for ( pwcTokenStartPtr = pltLngTokenizer->pwcStringCurrentPtr; pwcTokenStartPtr < pltLngTokenizer->pwcStringRangeEndPtr; pwcTokenStartPtr++ ) { 
    
            /* Break once we hit the start of an alphanumeric or if the character is in the list of leading punctuation characters to leave in */
            if ( (iswalnum(*pwcTokenStartPtr) != 0) || ((pltLngTokenizer->pwcLeadingPunctuationToLeave != NULL) && (s_wcschr(pltLngTokenizer->pwcLeadingPunctuationToLeave, *pwcTokenStartPtr) != NULL)) ) {
                break;
            }
        }
/* printf("iLngTokenizerGetToken2_un - pwcTokenStartPtr: '%ls' (%lu)\n", pwcTokenStartPtr, (unsigned long)pwcTokenStartPtr); */

    
        /* Return if we have read to the end of the string */
        if ( pwcTokenStartPtr >= pltLngTokenizer->pwcStringRangeEndPtr ) {
            
            /* Set the token pointers */
            pltLngTokenizer->pwcTokenStartPtr = NULL;
            pltLngTokenizer->pwcTokenEndPtr = NULL;
            
            /* Set the string current pointer to the string end since we are done */
            pltLngTokenizer->pwcStringCurrentPtr = pltLngTokenizer->pwcStringRangeEndPtr;
    
            return (LNG_NoError);
        }
    
    
        /* Find the end of the new token */
        for ( pwcTokenEndPtr = pwcTokenStartPtr, bIsTokenValid = false, bDoesTokenContainPunctuation = false, uiPunctuationDetectionState = 0; 
                pwcTokenEndPtr < pltLngTokenizer->pwcStringRangeEndPtr; pwcTokenEndPtr++ ) {

            boolean bIsCharacterPunctuation = false;
    
            /* Break once we hit a space */
            if ( iswspace(*pwcTokenEndPtr) != 0 ) {
                break;
            }
            
            /* Find out whether this term is punctuation or not */
            bIsCharacterPunctuation = (iswpunct(*pwcTokenEndPtr) == 0) ? false : true;
            
            /* Valid token if it contains something other than punctuation */
            if ( bIsCharacterPunctuation == false ) {
                bIsTokenValid = true;
            }
            
            /* Fugly little state machine to figure out if there is punctuation embedded in the term,
            ** ie. that is flanked by non-punctuation on either side, terms like 'M&A' or 
            ** 'francois@fsconsult.com', we use this to determine whether we need to extract
            ** components from this token
            */
            if ( bDoesTokenContainPunctuation == false ) {
                if ( (uiPunctuationDetectionState == 0) && (bIsCharacterPunctuation == false) ) {
                    uiPunctuationDetectionState = 1;
                }
                else if ( (uiPunctuationDetectionState == 1) && (bIsCharacterPunctuation == true) ) {
                    uiPunctuationDetectionState = 2;
                }
                else if ( (uiPunctuationDetectionState == 2) && (bIsCharacterPunctuation == false) ) {
                    bDoesTokenContainPunctuation = true;
                }
            }
        }


        /* Set the current string pointer */
        pltLngTokenizer->pwcStringCurrentPtr = pwcTokenEndPtr;    


        /* We can only end a token with a non-punctuation character or a character that is in 
        ** the list of trailing punctuation characters, so we crank back until we meet that condition
        */
        while ( ((pwcTokenEndPtr - 1) >= pwcTokenStartPtr) &&
                ((iswpunct(*(pwcTokenEndPtr - 1)) != 0) && ((pltLngTokenizer->pwcTrailingPunctuationToLeave == NULL) || (s_wcschr(pltLngTokenizer->pwcTrailingPunctuationToLeave, *(pwcTokenEndPtr - 1)) == NULL))) ) {
    
            /* Decrement the token end */
            pwcTokenEndPtr--;
        }
/* printf("iLngTokenizerGetToken2_un - pwcTokenEndPtr: '%ls' (%lu)\n", pwcTokenEndPtr, (unsigned long)pwcTokenEndPtr); */
/* iLngTokenizerPrintWideToken("iLngTokenizerGetToken2_un - pwcTokenStartPtr", pwcTokenStartPtr, pwcTokenEndPtr); */


        /* Break out if this is a valid token */
        if ( bIsTokenValid == true ) {
            
            /* Set the token current pointer to the token start pointer if the token contains punctuation,
            ** this is what tells us whether we need to extract components from this token 
            */
            pltLngTokenizer->pwcTokenCurrentPtr = (bDoesTokenContainPunctuation == true) ? pwcTokenStartPtr : NULL;

            break;
        }

    }
/* iLngTokenizerPrintWideToken("iLngTokenizerGetToken2_un - pwcTokenStartPtr", pwcTokenStartPtr, pwcTokenEndPtr); */


      /* Set the token pointers */
    pltLngTokenizer->pwcTokenStartPtr = pwcTokenStartPtr;
    pltLngTokenizer->pwcTokenEndPtr = pwcTokenEndPtr;


/* printf("pltLngTokenizer->pwcStringRangeStartPtr: '%ls' (%lu)\n", pltLngTokenizer->pwcStringRangeStartPtr, (unsigned long)pltLngTokenizer->pwcStringRangeStartPtr); */
/* printf("pwcTokenStartPtr: '%ls' (%lu)\n", pwcTokenStartPtr, (unsigned long)pwcTokenStartPtr); */
/* printf("pwcTokenEndPtr: '%ls' (%lu)\n", pwcTokenEndPtr, (unsigned long)pwcTokenEndPtr); */
/* printf("\n\n"); */


    return (LNG_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLngTokenizerGetComponent2_un()

    Purpose:    This function returns the next component for the current token
                in the current string being parsed

    Parameters: pltLngTokenizer     Language tokenizer structure

    Globals:    none

    Returns:    An LNG error code

*/
static int iLngTokenizerGetComponent2_un
(
    struct lngTokenizer *pltLngTokenizer
)
{

    wchar_t     *pwcComponentStartPtr = NULL;
    wchar_t     *pwcComponentEndPtr = NULL;


    ASSERT(pltLngTokenizer != NULL);


    /* There are no components for this token */
    if ( pltLngTokenizer->pwcTokenCurrentPtr == NULL ) {

        /* Set the component pointers */
        pltLngTokenizer->pwcComponentStartPtr = NULL;
        pltLngTokenizer->pwcComponentEndPtr = NULL;
        return (LNG_NoError);
    }

/* printf("iLngTokenizerGetComponent2_un - pltLngTokenizer->pwcTokenCurrentPtr: '%ls'\n", pltLngTokenizer->pwcTokenCurrentPtr); */


    /* Find the start of the new component */
    for ( pwcComponentStartPtr = pltLngTokenizer->pwcTokenCurrentPtr; pwcComponentStartPtr < pltLngTokenizer->pwcTokenEndPtr ; pwcComponentStartPtr++ ) { 

        /* Break once we hit good stuff */
        if ( iswpunct(*pwcComponentStartPtr) == 0 ) {
            break;
        }
    }
/* printf("iLngTokenizerGetComponent2_un - pwcComponentStartPtr: '%ls' (%lu) (%lu)\n", pwcComponentStartPtr, (unsigned long)pwcComponentStartPtr, (unsigned long)pltLngTokenizer->pwcTokenEndPtr); */


    /* Return if we have read to the end of the token */
    if ( pwcComponentStartPtr >= pltLngTokenizer->pwcTokenEndPtr ) {
        
        /* Set the component pointers */
        pltLngTokenizer->pwcComponentStartPtr = NULL;
        pltLngTokenizer->pwcComponentEndPtr = NULL;
        return (LNG_NoError);
    }


    /* Find the end of the new component */
    for ( pwcComponentEndPtr = pwcComponentStartPtr; pwcComponentEndPtr < pltLngTokenizer->pwcTokenEndPtr; pwcComponentEndPtr++ ) {

        /* Break once we hit punctuation or a space */
        if ( (iswpunct(*pwcComponentEndPtr) != 0) || (iswspace(*pwcComponentEndPtr) != 0) ) {
            break;
        }
    }
/* printf("iLngTokenizerGetComponent2_un - pwcComponentEndPtr: '%ls' (%lu)\n", pwcComponentEndPtr, (unsigned long)pwcComponentEndPtr); */
/* iLngTokenizerPrintWideToken("iLngTokenizerGetComponent2_un - pwcComponentStartPtr", pwcComponentStartPtr, pwcComponentEndPtr); */


    /* Set the string token pointer to the component end + 1 */
    pltLngTokenizer->pwcTokenCurrentPtr = pwcComponentEndPtr + 1;


    /* Set the component pointers */
    pltLngTokenizer->pwcComponentStartPtr = pwcComponentStartPtr;
    pltLngTokenizer->pwcComponentEndPtr = pwcComponentEndPtr;


/* printf("pwcComponentStartPtr: '%ls' (%lu)\n", pwcComponentStartPtr, (unsigned long)pwcComponentStartPtr); */
/* printf("pwcComponentEndPtr: '%ls' (%lu)\n", pwcComponentEndPtr, (unsigned long)pwcComponentEndPtr); */
/* printf("\n\n"); */


    return (LNG_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLngTokenizerStripTrailings2_un()

    Purpose:    This function will strip the trailings off of a token 

    Parameters: pltLngTokenizer     Language tokenizer structure
                pwcTokenStart       Pointer to the start of the token being stripped
                pwcTokenEnd         Pointer to the end of the token being stripped
                ppwcTokenEnd        Return pointer to the end of the token

    Globals:    none

    Returns:    An LNG error code

*/
static int iLngTokenizerStripTrailings2_un
(
    struct lngTokenizer *pltLngTokenizer,
    wchar_t *pwcTokenStart,
    wchar_t *pwcTokenEnd,
    wchar_t **ppwcTokenEnd
)
{


    wchar_t     *pwcTokenEndPtr = pwcTokenEnd;


    ASSERT(pltLngTokenizer != NULL);
    ASSERT(pwcTokenStart != NULL);
    ASSERT(pwcTokenEnd != NULL);


    /* We can only end a token with a non-punctuation character or a character that is in 
    ** the list of trailing punctuation characters, so we crank back until we meet that condition
    */
    while ( ((pwcTokenEndPtr - 1) >= pwcTokenStart) && 
            ((iswpunct(*(pwcTokenEndPtr - 1)) != 0) && ((pltLngTokenizer->pwcTrailingPunctuationToLeave == NULL) || (s_wcschr(pltLngTokenizer->pwcTrailingPunctuationToLeave, *(pwcTokenEndPtr - 1)) == NULL))) ) {

        /* Decrement the token end */
        pwcTokenEndPtr--;
    }


      /* Set the return pointers */
    *ppwcTokenEnd = pwcTokenEndPtr;


/* printf("pwcTokenEnd: '%ls' (%lu)\n", pwcTokenEnd, (unsigned long)pwcTokenEnd); */
/* printf("pwcTokenEndPtr: '%ls' (%lu)\n", pwcTokenEndPtr, (unsigned long)pwcTokenEndPtr); */


    return (LNG_NoError);

}


/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

#if defined(MPS_ENABLE_ICU)

/*

    Function:   iLngTokenizerGetToken_icu()

    Purpose:    This function interfaces with the ICU tokenizer

    Parameters: pltLngTokenizer     Language tokenizer structure
                pucLanguageCode     Language code

    Globals:    none

    Returns:    An LNG error code

*/
static int iLngTokenizerGetToken_icu
(
    struct lngTokenizer *pltLngTokenizer,
    unsigned char *pucLanguageCode
)
{

    int             iEnd = 0;
    UErrorCode      uErrorCode = U_ZERO_ERROR;


    ASSERT(pltLngTokenizer != NULL);
    ASSERT(bUtlStringsIsStringNULL(pucLanguageCode) == false);

    ASSERT(((pltLngTokenizer->pUBreakIterator == NULL) && (pltLngTokenizer->iStart == -1)) || 
            ((pltLngTokenizer->pUBreakIterator != NULL) && (pltLngTokenizer->iStart != -1)));


/* iLngTokenizerPrintWideToken("iLngTokenizerGetToken_icu - pltLngTokenizer->pwcStringRangeStartPtr", pltLngTokenizer->pwcStringRangeStartPtr, pltLngTokenizer->pwcStringRangeEndPtr); */

    /* Convert the string and create the break iterator if this is a new string */
    if ((pltLngTokenizer->pUBreakIterator == NULL) && (pltLngTokenizer->iStart == -1) ) {
        
        /* String length - include space for a terminating NULL */
        unsigned int uiStringLength = (pltLngTokenizer->pwcStringRangeEndPtr - pltLngTokenizer->pwcStringRangeStartPtr) + 1;
        
        /* Increase the UString capacity if needed */
        if ( uiStringLength > pltLngTokenizer->uiUStringCapacity ) {
            
            /* Pointer for the newly allocated string */
            UChar    *pUStringPtr = NULL;
        
            /* Reallocate the string */
            if ( (pUStringPtr = (UChar *)s_realloc(pltLngTokenizer->pUString, (size_t)(sizeof(UChar) * uiStringLength))) == NULL ) {
                return (LNG_MemError);
            }
            
            /* Hand over the pointer and update the string capacity */
            pltLngTokenizer->pUString = pUStringPtr;
            pltLngTokenizer->uiUStringCapacity = uiStringLength;
        }

        /* Convert the text from wchar_t to UChar */
        u_strFromWCS(pltLngTokenizer->pUString, uiStringLength, NULL, pltLngTokenizer->pwcStringRangeStartPtr, 
                (pltLngTokenizer->pwcStringRangeEndPtr - pltLngTokenizer->pwcStringRangeStartPtr), &uErrorCode);

        /* Check the error code */
        if ( U_FAILURE(uErrorCode) ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to convert text from wide characters to UChar, icu error: %d. (i)", uErrorCode);
            return (LNG_TokenizerConversionFailed);
        }

        /* Create the break iterator - check the error */
        pltLngTokenizer->pUBreakIterator = ubrk_open(UBRK_WORD, pucLanguageCode, pltLngTokenizer->pUString, u_strlen(pltLngTokenizer->pUString), &uErrorCode);

        /* Check the error code */
        if ( U_FAILURE(uErrorCode) ) {

#if defined(LNG_TOKENIZER_ICU_ENABLE_IGNORE_TOKENIZER_ERRORS)

            /* Set the string current pointer to the string range end to skip the range */
            pltLngTokenizer->pwcStringCurrentPtr = pltLngTokenizer->pwcStringRangeEndPtr;

            /* Set the token pointers */
            pltLngTokenizer->pwcTokenStartPtr = NULL;
            pltLngTokenizer->pwcTokenEndPtr = NULL;
            
            return (LNG_NoError);

#else    /* defined(LNG_TOKENIZER_ICU_ENABLE_IGNORE_TOKENIZER_ERRORS) */    

            iUtlLogError(UTL_LOG_CONTEXT, "Failed to create an ICU break iterator, icu error: %d.", uErrorCode);
            return (LNG_TokenizerTokenizationFailed);

#endif    /* defined(LNG_TOKENIZER_ICU_ENABLE_IGNORE_TOKENIZER_ERRORS) */    

        }

        /* Get the first break */
        pltLngTokenizer->iStart = ubrk_first(pltLngTokenizer->pUBreakIterator);

/* printf("iLngTokenizerGetToken_icu - pltLngTokenizer->iStart: %d\n", pltLngTokenizer->iStart); */

    }


    /* Loop until we either reach the end of the string, or we get a valid token, 
    ** ie one which does not start with punctuation or a space 
    */
    do {

        /* Get the next break */
        iEnd = ubrk_next(pltLngTokenizer->pUBreakIterator);
    
/* printf("iLngTokenizerGetToken_icu - pltLngTokenizer->iStart: %d, iEnd: %d\n", pltLngTokenizer->iStart, iEnd); */
        
        /* Wrap up */
        if ( ((pltLngTokenizer->pwcStringRangeStartPtr + iEnd) >= pltLngTokenizer->pwcStringRangeEndPtr) || (iEnd == UBRK_DONE) ) {
        
/* printf("iLngTokenizerGetToken_icu - ((pltLngTokenizer->pwcStringRangeStartPtr + iEnd) >= pltLngTokenizer->pwcStringRangeEndPtr) || (iEnd == UBRK_DONE)\n"); */
    
            /* Set the token pointers */
            if ( (pltLngTokenizer->pwcStringRangeStartPtr + iEnd) >= pltLngTokenizer->pwcStringRangeEndPtr ) {
                
                pltLngTokenizer->pwcTokenStartPtr = pltLngTokenizer->pwcStringRangeStartPtr + pltLngTokenizer->iStart;
                pltLngTokenizer->pwcTokenEndPtr = pltLngTokenizer->pwcStringRangeStartPtr + iEnd;

/* iLngTokenizerPrintWideToken("iLngTokenizerGetToken_icu - pltLngTokenizer->pwcStringRangeStartPtr", pltLngTokenizer->pwcTokenStartPtr, pltLngTokenizer->pwcTokenEndPtr); */

                /* There is a feature (ahem... bug!) in ICU whereby it thinks a trailing space is a valid token */
                if ( ((pltLngTokenizer->pwcTokenEndPtr - pltLngTokenizer->pwcTokenStartPtr) == 1) && (LNG_UNICODE_SPACE_RANGE(*pltLngTokenizer->pwcTokenStartPtr) == true) ) {
                    pltLngTokenizer->pwcTokenStartPtr = NULL;
                    pltLngTokenizer->pwcTokenEndPtr = NULL;
                }

            }
            else if ( iEnd == UBRK_DONE ) {
                pltLngTokenizer->pwcTokenStartPtr = NULL;
                pltLngTokenizer->pwcTokenEndPtr = NULL;
            }
    
            /* Set the string current pointer to the string end since we are done */
            pltLngTokenizer->pwcStringCurrentPtr = pltLngTokenizer->pwcStringRangeEndPtr;
    
            /* Free the break iterator */
            ubrk_close(pltLngTokenizer->pUBreakIterator);
            pltLngTokenizer->pUBreakIterator = NULL;
            
            /* Reset the start */
            pltLngTokenizer->iStart = -1;
        }
        else {
    
            /* Set the token pointers */
            pltLngTokenizer->pwcTokenStartPtr = pltLngTokenizer->pwcStringRangeStartPtr + pltLngTokenizer->iStart;
            pltLngTokenizer->pwcTokenEndPtr = pltLngTokenizer->pwcStringRangeStartPtr + iEnd;
    
            /* Save the string current pointer */
            pltLngTokenizer->pwcStringCurrentPtr = pltLngTokenizer->pwcTokenEndPtr;
    
            /* Set the new start */
            pltLngTokenizer->iStart = iEnd;
        }

    } while ( (pltLngTokenizer->iStart != -1) && ((iswpunct(*pltLngTokenizer->pwcTokenStartPtr) != 0) || (iswspace(*pltLngTokenizer->pwcTokenStartPtr) != 0)) );
    /* Loop until we either reach the end of the string, or we get a valid token, 
    ** i.e. one which does not start with punctuation or a space 
    */


    return (LNG_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLngTokenizerGetComponent_icu()

    Purpose:    This function returns the next component for the current token
                in the current string being parsed

    Parameters: pltLngTokenizer     Language tokenizer structure

    Globals:    none

    Returns:    An LNG error code

*/
static int iLngTokenizerGetComponent_icu
(
    struct lngTokenizer *pltLngTokenizer
)
{

    ASSERT(pltLngTokenizer != NULL);


      /* Set the component pointers */
    pltLngTokenizer->pwcComponentStartPtr = NULL;
    pltLngTokenizer->pwcComponentEndPtr = NULL;


    return (LNG_NoError);

}

#endif    /* defined(MPS_ENABLE_ICU) */

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

#if defined(MPS_ENABLE_MECAB)

/*

    Function:   iLngTokenizerGetToken_mecab()

    Purpose:    This function interfaces with the MeCab tokenizer

    Parameters: pltLngTokenizer     Language tokenizer structure

    Globals:    none

    Returns:    An LNG error code

*/
static int iLngTokenizerGetToken_mecab
(
    struct lngTokenizer *pltLngTokenizer
)
{


    int     iError = LNG_NoError;

    
    ASSERT(pltLngTokenizer != NULL);

    ASSERT(((pltLngTokenizer->pmtMecabTokenizer == NULL) && (pltLngTokenizer->pvLngConverterWCharToUTF8 == NULL) && (pltLngTokenizer->pvLngConverterUTF8ToWChar == NULL)) || 
            ((pltLngTokenizer->pmtMecabTokenizer != NULL) && (pltLngTokenizer->pvLngConverterWCharToUTF8 != NULL) && (pltLngTokenizer->pvLngConverterUTF8ToWChar != NULL)));



    /* Lazy creation of the tokenizer and the converter */
    if ( (pltLngTokenizer->pmtMecabTokenizer == NULL) && (pltLngTokenizer->pvLngConverterWCharToUTF8 == NULL) && (pltLngTokenizer->pvLngConverterUTF8ToWChar == NULL) ) {

        /* Create the MeCab tokenizer, note that we don't pass any parameters */
        if ( (pltLngTokenizer->pmtMecabTokenizer = mecab_new2("")) == NULL ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to create the MeCab tokenizer, mecab error: '%s'.", mecab_strerror(pltLngTokenizer->pmtMecabTokenizer));
            return (LNG_TokenizerInvalidTokenizer);
        }

        /* Create the character set converter */
        if ( (iError = iLngConverterCreateByID(LNG_CHARACTER_SET_WCHAR_ID, LNG_CHARACTER_SET_UTF_8_ID, &pltLngTokenizer->pvLngConverterWCharToUTF8)) != LNG_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to create a character set converter to convert from wide characters to utf-8, lng error: %d.", iError);
            return (LNG_TokenizerInvalidTokenizer);
        }    

        /* Create the character set converter */
        if ( (iError = iLngConverterCreateByID(LNG_CHARACTER_SET_UTF_8_ID, LNG_CHARACTER_SET_WCHAR_ID, &pltLngTokenizer->pvLngConverterUTF8ToWChar)) != LNG_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to create a character set converter to convert from utf-8 to wide characters, lng error: %d.", iError);
            return (LNG_TokenizerInvalidTokenizer);
        }    
    }



    /* Allocate the buffer, convert the string and parse it if this is a new string */
    if ( pltLngTokenizer->bNewString == true ) {
        
        unsigned int    uiStringLength = 0;

/* iLngTokenizerPrintWideToken("iLngTokenizerGetToken_mecab - pltLngTokenizer->pwcStringRangeStartPtr", pltLngTokenizer->pwcStringRangeStartPtr, pltLngTokenizer->pwcStringRangeEndPtr); */

        /* Extend the string space if needed - space for the NULL */
        if ( ((((pltLngTokenizer->pwcStringRangeEndPtr - pltLngTokenizer->pwcStringRangeStartPtr) + 1) * LNG_CONVERTER_WCHAR_TO_UTF_8_MULTIPLIER)) > pltLngTokenizer->uiStringCapacity ) {
            
            unsigned char    *pucStringPtr = NULL;
            
            /* Set the string capacity - space for the NULL */
            pltLngTokenizer->uiStringCapacity = ((pltLngTokenizer->pwcStringRangeEndPtr - pltLngTokenizer->pwcStringRangeStartPtr) + 1) * LNG_CONVERTER_WCHAR_TO_UTF_8_MULTIPLIER;
            
            /* Allocate space for the string */
            if ( (pucStringPtr = (unsigned char *)s_realloc(pltLngTokenizer->pucString, (size_t)(sizeof(unsigned char) * pltLngTokenizer->uiStringCapacity))) == NULL ) {
                return (LNG_MemError);
            }
            
            /* Hand over the pointer */
            pltLngTokenizer->pucString = pucStringPtr;
        }


        /* Extend the token space if needed - space for normalization and the NULL */
        if ( ((pltLngTokenizer->pwcStringRangeEndPtr - pltLngTokenizer->pwcStringRangeStartPtr) + 1) > pltLngTokenizer->uiTokenCapacity ) {
            
            wchar_t     *pwcTokenPtr = NULL;
            
            /* Set the token capacity - space for the NULL */
            pltLngTokenizer->uiTokenCapacity = (pltLngTokenizer->pwcStringRangeEndPtr - pltLngTokenizer->pwcStringRangeStartPtr) + 1;
            
            /* Allocate space for the token */
            if ( (pwcTokenPtr = (wchar_t *)s_realloc(pltLngTokenizer->pwcToken, (size_t)(sizeof(wchar_t) * pltLngTokenizer->uiTokenCapacity))) == NULL ) {
                return (LNG_MemError);
            }
            
            /* Hand over the pointer */
            pltLngTokenizer->pwcToken = pwcTokenPtr;
        }


        /* Use a separate variable for the string length */
        uiStringLength = pltLngTokenizer->uiStringCapacity;
            
        /* Convert the string */
        if ( (iError = iLngConverterConvertString(pltLngTokenizer->pvLngConverterWCharToUTF8, LNG_CONVERTER_RETURN_ON_ERROR, (unsigned char *)pltLngTokenizer->pwcStringRangeStartPtr, 
                (pltLngTokenizer->pwcStringRangeEndPtr - pltLngTokenizer->pwcStringRangeStartPtr) * sizeof(wchar_t), (unsigned char **)&pltLngTokenizer->pucString, &uiStringLength)) != LNG_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to convert text from wide characters to utf-8, lng error: %d.", iError);
            return (LNG_TokenizerConversionFailed);
        }

/* printf("iLngTokenizerGetToken_mecab - pltLngTokenizer->pucString: '%s', uiStringLength %d\n", pltLngTokenizer->pucString, uiStringLength); */
        
        /* Parse the string, this returns the first node which should be a BOS (begining of sentence), we check for that later */
        if ( (pltLngTokenizer->pmnMecabNode = mecab_sparse_tonode2(pltLngTokenizer->pmtMecabTokenizer, pltLngTokenizer->pucString, uiStringLength)) == NULL ) {

#if defined(LNG_TOKENIZER_MECAB_ENABLE_IGNORE_TOKENIZER_ERRORS)

            /* Set the string current pointer to the string range end */
            pltLngTokenizer->pwcStringCurrentPtr = pltLngTokenizer->pwcStringRangeEndPtr;

            /* Reset all the fields in the tokenizer */
            pltLngTokenizer->bNewString = true;

            /* Set the token pointers */
            pltLngTokenizer->pwcTokenStartPtr = NULL;
            pltLngTokenizer->pwcTokenEndPtr = NULL;
            
            return (LNG_NoError);

#else    /* defined(LNG_TOKENIZER_MECAB_ENABLE_IGNORE_TOKENIZER_ERRORS) */    

            iUtlLogError(UTL_LOG_CONTEXT, "Failed to parse a string with the MeCab tokenizer, mecab error: '%s'.", mecab_strerror(pltLngTokenizer->pmtMecabTokenizer));
            return (LNG_TokenizerTokenizationFailed);

#endif    /* defined(LNG_TOKENIZER_MECAB_ENABLE_IGNORE_TOKENIZER_ERRORS) */    

        }
        
        
        /* The first node should be a BOS (begining of sentence) */
        if ( pltLngTokenizer->pmnMecabNode->stat != MECAB_BOS_NODE ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to parse a string with the MeCab tokenizer, expected the first node to be a BOS.");
            return (LNG_TokenizerTokenizationFailed);
        }
        
    }



    /* Get the next token, convert it and return it
    **
    ** MeCab does not skip over punctuation but returns individual punctuation characters
    ** as surfaces so we need to check for those and filter them out ourselves, hence the
    ** while() loop
    */
    while ( true ) {

        unsigned char   *pucTokenStart = NULL;
        unsigned char   *pucTokenEnd = NULL;
        unsigned int    uiTokenLength = 0;
        wchar_t         *pwcTokenStartPtr = NULL;
        wchar_t         *pwcTokenEndPtr = NULL;


        /* Get the next node */
        pltLngTokenizer->pmnMecabNode = pltLngTokenizer->pmnMecabNode->next;
        
        /* Processing if we failed to get the next node */
        if ( pltLngTokenizer->pmnMecabNode == NULL ) {

#if defined(LNG_TOKENIZER_MECAB_ENABLE_IGNORE_TOKENIZER_ERRORS)

            /* Set the string current pointer to the string range end */
            pltLngTokenizer->pwcStringCurrentPtr = pltLngTokenizer->pwcStringRangeEndPtr;

            /* Reset all the fields in the tokenizer */
            pltLngTokenizer->bNewString = true;

            /* Set the token pointers */
            pltLngTokenizer->pwcTokenStartPtr = NULL;
            pltLngTokenizer->pwcTokenEndPtr = NULL;
            
            return (LNG_NoError);

#else    /* defined(LNG_TOKENIZER_MECAB_ENABLE_IGNORE_TOKENIZER_ERRORS) */    

            iUtlLogError(UTL_LOG_CONTEXT, "Failed to get the next node from the MeCab tokenizer, mecab error: '%s'.", mecab_strerror(pltLngTokenizer->pmtMecabTokenizer));
            return (LNG_TokenizerTokenizationFailed);

#endif    /* defined(LNG_TOKENIZER_MECAB_ENABLE_IGNORE_TOKENIZER_ERRORS) */    

        }


        /* Return here if this was the last node, namely an EOS (end of sentence) */
        if ( pltLngTokenizer->pmnMecabNode->stat == MECAB_EOS_NODE ) {

            /* Set the string current pointer to the string range end */
            pltLngTokenizer->pwcStringCurrentPtr = pltLngTokenizer->pwcStringRangeEndPtr;

            /* Reset all the fields in the tokenizer */
            pltLngTokenizer->bNewString = true;

            /* Set the token pointers */
            pltLngTokenizer->pwcTokenStartPtr = NULL;
            pltLngTokenizer->pwcTokenEndPtr = NULL;

            return (LNG_NoError);
        }

    
        /* Set the token start and token end */
        pucTokenStart = (unsigned char *)pltLngTokenizer->pmnMecabNode->surface;
        pucTokenEnd = pucTokenStart + pltLngTokenizer->pmnMecabNode->length;
        
        /* Use a separate variable for the token length */
        uiTokenLength = pltLngTokenizer->uiTokenCapacity * sizeof(wchar_t);
            
        /* Convert the token */
        if ( (iError = iLngConverterConvertString(pltLngTokenizer->pvLngConverterUTF8ToWChar, LNG_CONVERTER_RETURN_ON_ERROR, pucTokenStart, pucTokenEnd - pucTokenStart, 
                (unsigned char **)&pltLngTokenizer->pwcToken, &uiTokenLength)) != LNG_NoError ) {
            iUtlLogError(UTL_LOG_CONTEXT, "Failed to convert text from utf-8 to wide characters, lng error: %d.", iError);
            return (LNG_TokenizerConversionFailed);
        }
        
        /* Set the token pointers
        **
        ** Note that uiTokenLength is set to the number of bytes (as opposed to characters) 
        ** in pltLngTokenizer->pwcToken, so we adjust it by dividing by sizeof(wchar_t)
        */
        pwcTokenStartPtr = pltLngTokenizer->pwcToken;
        pwcTokenEndPtr = pltLngTokenizer->pwcToken + (uiTokenLength / sizeof(wchar_t));
        

        /* MeCab does not filter out punctuation characters, rather it breaks them out as
        ** a token (surface), adjacent punctuation characters are parsed as a single token
        ** so we only need to test the first character.
        **
        ** Loop if the first character of the token is a punctuation character
        */
        if ( LNG_UNICODE_PUNCTUATION_RANGE(*pwcTokenStartPtr) ) {
            continue;
        }
        

/* Katakana-Hiragana transliteration */
#if defined(LNG_TOKENIZER_MECAB_ENABLE_KANA_TRANSLITERATION)

        /* Check if the token is Katakana only */
        if ( bIsKatakana(pwcTokenStartPtr, pwcTokenEndPtr) == true ) {
            
            /* Error code */
            UErrorCode  uErrorCode = U_ZERO_ERROR;
            
            /* Create the transliterator if needed */
            if ( pltLngTokenizer->pUTransliterator == NULL ) {

                /* Create the transliterator ID */
                UChar    pUCharTransliteratorID[LNG_TOKENIZER_MECAB_TRANSLITERATOR_STRING_LENGTH + 1];
                u_strFromUTF8(pUCharTransliteratorID, LNG_TOKENIZER_MECAB_TRANSLITERATOR_STRING_LENGTH, NULL, LNG_TOKENIZER_MECAB_TRANSLITERATOR_STRING, -1, &uErrorCode);

                /* Create the Katakana-Hiragana transliterator */
                pltLngTokenizer->pUTransliterator = utrans_openU(pUCharTransliteratorID, -1, UTRANS_FORWARD, NULL, -1, NULL, &uErrorCode);
                
                /* Check the status */
                if ( U_FAILURE(uErrorCode) ) {
                    iUtlLogError(UTL_LOG_CONTEXT, "Failed to create a transliterator: '%s', icu error: %d.", LNG_TOKENIZER_MECAB_TRANSLITERATOR_STRING, uErrorCode);
                    return (LNG_TokenizerTokenizationFailed);
                }
            }


            /* To convert the wchar_t token to a UString */
            UChar   pUString[LNG_TOKENIZER_MECAB_TRANSLITERATION_CAPACITY];
            int     iUStringCapacity = LNG_TOKENIZER_MECAB_TRANSLITERATION_CAPACITY;
            int     iUStringLength = 0;

            /* Convert the wchar_t token to a UString */
            u_strFromWCS(pUString, iUStringCapacity, &iUStringLength, pwcTokenStartPtr, pwcTokenEndPtr - pwcTokenStartPtr, &uErrorCode);

            /* Check the status */
            if ( U_FAILURE(uErrorCode) ) {
                iUtlLogError(UTL_LOG_CONTEXT, "Failed to convert text from wide characters to UChar, icu error: %d.", uErrorCode);
                return (LNG_TokenizerConversionFailed);
            }


            /* Set the string limit */
            int     iUStringLimit = iUStringLength;

            /* Transliterate */
            utrans_transUChars(pltLngTokenizer->pUTransliterator, pUString, &iUStringLength, iUStringCapacity, 0, &iUStringLimit, &uErrorCode);

            /* Check the status */
            if ( U_FAILURE(uErrorCode) ) {

#if defined(LNG_TOKENIZER_MECAB_ENABLE_IGNORE_TRANSLITERATOR_ERRORS)

                /* Set the token pointers to the orginal token */
                pltLngTokenizer->pwcTokenStartPtr = pwcTokenStartPtr;
                pltLngTokenizer->pwcTokenEndPtr = pwcTokenEndPtr;
                
                return (LNG_NoError);

#else    /* defined(LNG_TOKENIZER_MECAB_ENABLE_IGNORE_TRANSLITERATOR_ERRORS) */    

                iUtlLogError(UTL_LOG_CONTEXT, "Failed to transliterate: '%s', icu error: %d.", LNG_TOKENIZER_MECAB_TRANSLITERATOR_STRING, uErrorCode);
                return (LNG_TokenizerTokenizationFailed);

#endif    /* defined(LNG_TOKENIZER_MECAB_ENABLE_IGNORE_TRANSLITERATOR_ERRORS) */    

            }


            /* To convert the UString to wchar_t */
/*             unsigned int uiTokenLength = 0; */
            uiTokenLength = 0;

            /* Convert the UString to wchar_t - note the use of iUStringLength */
            u_strToWCS(pltLngTokenizer->pwcToken, pltLngTokenizer->uiTokenCapacity * sizeof(wchar_t), &uiTokenLength, pUString, iUStringLength, &uErrorCode);

            /* Check the status */
            if ( U_FAILURE(uErrorCode) ) {
                iUtlLogError(UTL_LOG_CONTEXT, "Failed to convert text from UChar to wide characters, icu error: %d.", uErrorCode);
                return (LNG_TokenizerConversionFailed);
            }

            
            /* Set the token pointers */
            pwcTokenStartPtr = pltLngTokenizer->pwcToken;
            pwcTokenEndPtr = pltLngTokenizer->pwcToken + uiTokenLength;

/* iLngTokenizerPrintWideToken("iLngTokenizerGetToken_mecab - pwcTokenStartPtr", pwcTokenStartPtr, pwcTokenEndPtr); */

            /* Token is normalized (transliterated) */
            pltLngTokenizer->bNormalized = true;

        }

#endif    /* defined(LNG_TOKENIZER_MECAB_ENABLE_KANA_TRANSLITERATION) */
        

        /* Set the token pointers */
        pltLngTokenizer->pwcTokenStartPtr = pwcTokenStartPtr;
        pltLngTokenizer->pwcTokenEndPtr = pwcTokenEndPtr;

/* iLngTokenizerPrintWideToken("iLngTokenizerGetToken_mecab - pltLngTokenizer->pwcTokenStartPtr", pltLngTokenizer->pwcTokenStartPtr, pltLngTokenizer->pwcTokenEndPtr); */

        /* Set the flag telling us that we are still processing the current string */
        pltLngTokenizer->bNewString = false;

        /* Break out of the loop */
        break;

    }    /* while ( true ) */


    return (LNG_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLngTokenizerGetComponent_mecab()

    Purpose:    This function returns the next component for the current token
                in the current string being parsed

    Parameters: pltLngTokenizer     Language tokenizer structure

    Globals:    none

    Returns:    An LNG error code

*/
static int iLngTokenizerGetComponent_mecab
(
    struct lngTokenizer *pltLngTokenizer
)
{

    ASSERT(pltLngTokenizer != NULL);


    /* Set the component pointers */
    pltLngTokenizer->pwcComponentStartPtr = NULL;
    pltLngTokenizer->pwcComponentEndPtr = NULL;


    return (LNG_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   bIsKatakana()

    Purpose:    This function returns true if all the CJK characters in it 
                are Katakana

    Parameters: pwcTokenStart       Token start
                pwcTokenEnd         Token end

    Globals:    none

    Returns:    true if all the CJK characters in it are Katakana,
                false otherwise

*/
static boolean bIsKatakana
(
    wchar_t *pwcTokenStart,
    wchar_t *pwcTokenEnd
)
{

    wchar_t     *pwcTokenPtr = pwcTokenStart;


    /* Check the parameters */
    if ( pwcTokenStart == NULL ) {
        return (false);
    }

    if ( pwcTokenEnd == NULL ) {
        return (false);
    }


    /* Katakana flag */
    boolean     isKatakana = false;

    /* Loop over all the characters in the token */
    for ( pwcTokenPtr = pwcTokenStart; pwcTokenPtr < pwcTokenEnd; pwcTokenPtr++ ) { 
        
        /* Flip the katakana flag if we find katakana in the token */
        if ( LNG_UNICODE_KATAKANA_RANGE(*pwcTokenPtr) ) {
            isKatakana = true;
        }

        /* But return false if we find something else in the CJK range */
        else if ( LNG_UNICODE_ENTIRE_CJK_RANGE(*pwcTokenPtr) ) {
            return (false);
        }
    }


    /* Return the katakana flag */
    return (isKatakana);

}

#endif    /* defined(MPS_ENABLE_MECAB) */

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/


/*

    Function:   iLngTokenizerGetToken_bigram()

    Purpose:    This function gets the next bigram

    Parameters: pltLngTokenizer     Language tokenizer structure

    Globals:    none

    Returns:    An LNG error code

*/
static int iLngTokenizerGetToken_bigram
(
    struct lngTokenizer *pltLngTokenizer
)
{

    wchar_t     *pwcTokenStartPtr = NULL;
    wchar_t     *pwcTokenEndPtr = NULL;


    ASSERT(pltLngTokenizer != NULL);


/* iLngTokenizerPrintWideToken("iLngTokenizerGetToken_bigram - pltLngTokenizer->pwcStringCurrentPtr", pltLngTokenizer->pwcStringCurrentPtr, pltLngTokenizer->pwcStringRangeEndPtr); */

    /* Loop forever, we control the look from within */
    while ( true ) {

    
        /* Find the start of the new token */
        for ( pwcTokenStartPtr = pltLngTokenizer->pwcStringCurrentPtr; pwcTokenStartPtr < pltLngTokenizer->pwcStringRangeEndPtr; pwcTokenStartPtr++ ) { 
    
            /* Break once we hit the start of a new token */
            if ( iswalnum(*pwcTokenStartPtr) != 0 ) {
                break;
            }
        }
/* printf("iLngTokenizerGetToken_bigram - pwcTokenStartPtr: '%ls' (%lu)\n", pwcTokenStartPtr, (unsigned long)pwcTokenStartPtr); */
    
    
        /* Return if we have read to the end of the string */
        if ( pwcTokenStartPtr >= pltLngTokenizer->pwcStringRangeEndPtr ) {
            
            /* Set the token pointers */
            pltLngTokenizer->pwcTokenStartPtr = NULL;
            pltLngTokenizer->pwcTokenEndPtr = NULL;
            
            /* Set the string current pointer to the string end since we are done */
            pltLngTokenizer->pwcStringCurrentPtr = pltLngTokenizer->pwcStringRangeEndPtr;
    
            return (LNG_NoError);
        }
    
    
    
        /* Flags to indicate whether the previous and the next characters are valid */
        boolean isPreviousCharacterValid = false;
        boolean isNextCharacterValid = false;
        
        
        /* Flip the flag if the previous character is valid */
        if ( ((pwcTokenStartPtr - 1) >= pltLngTokenizer->pwcStringRangeStartPtr) && (iswalnum(*(pwcTokenStartPtr - 1)) != 0) ) {
            isPreviousCharacterValid = true;
        }
        
        /* Flip the flag if the next character is valid */
        if ( ((pwcTokenStartPtr + 1) < pltLngTokenizer->pwcStringRangeEndPtr) && (iswalnum(*(pwcTokenStartPtr + 1)) != 0) ) {
            isNextCharacterValid = true;
        }
    
        
        /* Legend:
        **
        **  @   valid character
        **  #   start of bigram
        **  :   invalid character (punctuation, start/end of range)
        */

        /* @#@ or :#@ */
        if ( isNextCharacterValid == true ) {
            pwcTokenEndPtr = pwcTokenStartPtr + 2;
        }

        /* :#: */
        else if ( (isPreviousCharacterValid == false) && (isNextCharacterValid == false) ) {
            pwcTokenEndPtr = pwcTokenStartPtr + 1;
        }
        
        /* @#: */
        else if ( (isPreviousCharacterValid == true) && (isNextCharacterValid == false) ) {
            pltLngTokenizer->pwcStringCurrentPtr++;
            continue;
        }
    
/* iLngTokenizerPrintWideToken("iLngTokenizerGetToken_bigram - pwcTokenStartPtr", pwcTokenStartPtr, pwcTokenEndPtr); */
    
        /* Save the string current pointer */
        pltLngTokenizer->pwcStringCurrentPtr = pwcTokenStartPtr + 1;
    
    
        /* Set the token pointers */
        pltLngTokenizer->pwcTokenStartPtr = pwcTokenStartPtr;
        pltLngTokenizer->pwcTokenEndPtr = pwcTokenEndPtr;
    

        /* Break out of the loop */
        break;

    }    /* while (true) */


    return (LNG_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLngTokenizerGetComponent_bigram()

    Purpose:    This function returns the next component for the current token
                in the current string being parsed

    Parameters: pltLngTokenizer     Language tokenizer structure

    Globals:    none

    Returns:    An LNG error code

*/
static int iLngTokenizerGetComponent_bigram
(
    struct lngTokenizer *pltLngTokenizer
)
{

    ASSERT(pltLngTokenizer != NULL);


    /* Set the component pointers */
    pltLngTokenizer->pwcComponentStartPtr = NULL;
    pltLngTokenizer->pwcComponentEndPtr = NULL;


    return (LNG_NoError);

}


/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/


/*

    Function:   iLngTokenizerGetToken_unigram()

    Purpose:    This function 

    Parameters: pltLngTokenizer     Language tokenizer structure
                pucLanguageCode     Language code

    Globals:    none

    Returns:    An LNG error code

*/
static int iLngTokenizerGetToken_unigram
(
    struct lngTokenizer *pltLngTokenizer,
    unsigned char *pucLanguageCode
)
{

    wchar_t     *pwcTokenStartPtr = NULL;
    wchar_t     *pwcTokenEndPtr = NULL;


    ASSERT(pltLngTokenizer != NULL);


/* iLngTokenizerPrintWideToken("iLngTokenizerGetToken_unigram - pltLngTokenizer->pwcStringCurrentPtr", pltLngTokenizer->pwcStringCurrentPtr, pltLngTokenizer->pwcStringRangeEndPtr); */

    /* Find the start of the new token */
    for ( pwcTokenStartPtr = pltLngTokenizer->pwcStringCurrentPtr; pwcTokenStartPtr < pltLngTokenizer->pwcStringRangeEndPtr; pwcTokenStartPtr++ ) { 

        /* Break once we hit the start of a new token */
        if ( iswalnum(*pwcTokenStartPtr) != 0 ) {
            break;
        }
    }
/* printf("iLngTokenizerGetToken_unigram - pwcTokenStartPtr: '%ls' (%lu)\n", pwcTokenStartPtr, (unsigned long)pwcTokenStartPtr); */


    /* Return if we have read to the end of the string */
    if ( pwcTokenStartPtr >= pltLngTokenizer->pwcStringRangeEndPtr ) {
        
        /* Set the token pointers */
        pltLngTokenizer->pwcTokenStartPtr = NULL;
        pltLngTokenizer->pwcTokenEndPtr = NULL;
        
        /* Set the string current pointer to the string end since we are done */
        pltLngTokenizer->pwcStringCurrentPtr = pltLngTokenizer->pwcStringRangeEndPtr;

        return (LNG_NoError);
    }


    /* Set the end of the new token */
    pwcTokenEndPtr = pwcTokenStartPtr + 1;

/* iLngTokenizerPrintWideToken("iLngTokenizerGetToken_unigram - pwcTokenStartPtr", pwcTokenStartPtr, pwcTokenEndPtr); */


    /* Save the string current pointer */
    pltLngTokenizer->pwcStringCurrentPtr = pwcTokenEndPtr;


    /* Set the token pointers */
    pltLngTokenizer->pwcTokenStartPtr = pwcTokenStartPtr;
    pltLngTokenizer->pwcTokenEndPtr = pwcTokenEndPtr;



    return (LNG_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLngTokenizerGetComponent_unigram()

    Purpose:    This function returns the next component for the current token
                in the current string being parsed

    Parameters: pltLngTokenizer     Language tokenizer structure

    Globals:    none

    Returns:    An LNG error code

*/
static int iLngTokenizerGetComponent_unigram
(
    struct lngTokenizer *pltLngTokenizer
)
{

    ASSERT(pltLngTokenizer != NULL);


    /* Set the component pointers */
    pltLngTokenizer->pwcComponentStartPtr = NULL;
    pltLngTokenizer->pwcComponentEndPtr = NULL;


    return (LNG_NoError);

}


/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/


/*

    Function:   iLngTokenizerPrintWideToken()

    Purpose:    This function will print a wide token 

    Parameters: pucLabel            label
                pwcTokenStart       Pointer to the start of the token
                pwcTokenEnd         Pointer to the end of the token

    Globals:    none

    Returns:    An LNG error code

*/
static int iLngTokenizerPrintWideToken
(
    unsigned char *pucLabel,
    wchar_t *pwcTokenStart,
    wchar_t *pwcTokenEnd
)
{

    ASSERT(pucLabel != NULL);
    ASSERT(pwcTokenStart != NULL);
    ASSERT(pwcTokenEnd != NULL);


    /* Print the token */
    {
        /* Save the last character */
        wchar_t     wcTokenEnd = *pwcTokenEnd;
    
        /* Null terminate the string */
        *pwcTokenEnd = L'\0';

        /* Print the token */
        iUtlLogInfo(UTL_LOG_CONTEXT, "%s: '%ls' (%d)", pucLabel, pwcTokenStart, (int)s_wcslen(pwcTokenStart));

        /* Restore the last character */
        *pwcTokenEnd = wcTokenEnd;
    }


    return (LNG_NoError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLngTokenizerPrintToken()

    Purpose:    This function will print a token 

    Parameters: pucLabel            label
                pucTokenStart       Pointer to the start of the token
                pucTokenEnd         Pointer to the end of the token

    Globals:    none

    Returns:    An LNG error code

*/
static int iLngTokenizerPrintToken
(
    unsigned char *pucLabel,
    unsigned char *pucTokenStart,
    unsigned char *pucTokenEnd
)
{

    ASSERT(pucLabel != NULL);
    ASSERT(pucTokenStart != NULL);
    ASSERT(pucTokenEnd != NULL);


    /* Print the token */
    {
        /* Save the last character */
        unsigned char   ucTokenEnd = *pucTokenEnd;
    
        /* Null terminate the string */
        *pucTokenEnd = L'\0';

        /* Print the token */
        iUtlLogInfo(UTL_LOG_CONTEXT, "%s: '%s' (%d)", pucLabel, pucTokenStart, (int)s_strlen(pucTokenStart));

        /* Restore the last character */
        *pucTokenEnd = ucTokenEnd;
    }


    return (LNG_NoError);

}


/*---------------------------------------------------------------------------*/

