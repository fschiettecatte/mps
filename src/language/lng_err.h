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

    Module:     lng_err.h

    Author:     Francois Schiettecatte (FS Consulting LLC.)

    Created:    2 May 2004

    Purpose:    This header file defines all the error codes which 
                can be returned by the language functions.

                These error codes are organized in such a way that 
                anything other that a 0 is an error.

                Also they have been divided into error ranges as 
                follows:


                generic             -1 through      -99 
                language          -100 through     -199 
                tokenizer         -200 through     -299 
                stemmer           -300 through     -399 
                soundex           -400 through     -499 
                metaphone         -500 through     -599 
                phonix            -600 through     -699 
                typo              -700 through     -799
                stoplist          -800 through     -899
                converter         -900 through     -999
                location         -1000 through    -1099
                unicode          -1100 through    -1199


                Error code -1 is a generic error.

                Error codes -2 through -99 are reserved.

*/


/*---------------------------------------------------------------------------*/


#if !defined(LNG_ERR_H)
#define LNG_ERR_H


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


/* Generic */
#define LNG_NoError                                     (0)
#define LNG_MemError                                    (-1)
#define LNG_ParameterError                              (-2)
#define LNG_ReturnParameterError                        (-3)
#define LNG_MiscError                                   (-4)


/* Language */
#define LNG_LanguageInvalidCharacterSetName             (-100)
#define LNG_LanguageInvalidCharacterSetID               (-101)
#define LNG_LanguageInvalidLanguageCode                 (-102)
#define LNG_LanguageInvalidLanguageID                   (-103)
#define LNG_LanguageInvalidTokenizerName                (-104)
#define LNG_LanguageInvalidTokenizerID                  (-105)
#define LNG_LanguageInvalidStemmerName                  (-106)
#define LNG_LanguageInvalidStemmerID                    (-107)
#define LNG_LanguageInvalidSoundexName                  (-108)
#define LNG_LanguageInvalidSoundexID                    (-109)
#define LNG_LanguageInvalidMetaphoneName                (-110)
#define LNG_LanguageInvalidMetaphoneID                  (-111)
#define LNG_LanguageInvalidPhonixName                   (-112)
#define LNG_LanguageInvalidPhonixID                     (-113)
#define LNG_LanguageInvalidTypoName                     (-114)
#define LNG_LanguageInvalidTypoID                       (-115)
#define LNG_LanguageInvalidStopListName                 (-116)
#define LNG_LanguageInvalidStopListID                   (-117)
#define LNG_LanguageInvalidName                         (-190)
#define LNG_LanguageInvalidID                           (-191)


/* Tokenizer */
#define LNG_TokenizerInvalidConfigurationDirectoryPath  (-200)
#define LNG_TokenizerInvalidTokenizer                   (-201)
#define LNG_TokenizerInvalidString                      (-202)
#define LNG_TokenizerInvalidStringLength                (-203)
#define LNG_TokenizerInvalidToken                       (-204)
#define LNG_TokenizerInvalidTokenLength                 (-205)
#define LNG_TokenizerTokenizationFailed                 (-206)
#define LNG_TokenizerConversionFailed                   (-207)
#define LNG_TokenizerInvalidFileDescriptor              (-208)
#define LNG_TokenizerInvalidLogContext                  (-209)


/* Stemmer */
#define LNG_StemmerUnavailableStemmer                   (-300)
#define LNG_StemmerInvalidStemmer                       (-301)
#define LNG_StemmerInvalidTerm                          (-302)
#define LNG_StemmerStemmingFailed                       (-303)

/* Soundex */
#define LNG_SoundexUnavailableSoundex                   (-400)
#define LNG_SoundexInvalidSoundex                       (-401)
#define LNG_SoundexInvalidTerm                          (-402)
#define LNG_SoundexInvalidCharacterList                 (-403)
#define LNG_SoundexInvalidSoundexKey                    (-404)
#define LNG_SoundexSoundexingFailed                     (-405)

/* Metaphone */
#define LNG_MetaphoneUnavailableMetaphone               (-500)
#define LNG_MetaphoneInvalidMetaphone                   (-501)
#define LNG_MetaphoneInvalidTerm                        (-502)
#define LNG_MetaphoneInvalidCharacterList               (-503)
#define LNG_MetaphoneInvalidMetaphoneKey                (-504)
#define LNG_MetaphoneMetaphoningFailed                  (-505)

/* Phonix */
#define LNG_PhonixUnavailablePhonix                     (-600)
#define LNG_PhonixInvalidPhonix                         (-601)
#define LNG_PhonixInvalidTerm                           (-602)
#define LNG_PhonixInvalidCharacterList                  (-603)
#define LNG_PhonixInvalidPhonixKey                      (-604)
#define LNG_PhonixPhonixingFailed                       (-605)

/* Typo */
#define LNG_TypoUnavailableTypo                         (-700)
#define LNG_TypoInvalidTypo                             (-701)
#define LNG_TypoInvalidCharacterList                    (-702)
#define LNG_TypoInvalidTerm                             (-703)
#define LNG_TypoInvalidCandidateTerm                    (-704)
#define LNG_TypoInvalidMaxCount                         (-705)
#define LNG_TypoFailedMatch                             (-706)

/* Stoplist */
#define LNG_StopListUnavailableStopList                 (-800)
#define LNG_StopListInvalidStopList                     (-801)
#define LNG_StopListInvalidStopListFilePath             (-802)
#define LNG_StopListInvalidStopListFile                 (-803)

/* Converter */
#define LNG_ConverterInvalidCharacterSetCombination     (-900)
#define LNG_ConverterInvalidConverter                   (-901)
#define LNG_ConverterInvalidErrorHandling               (-902)
#define LNG_ConverterInvalidSourceString                (-903)
#define LNG_ConverterInvalidSourceStringLength          (-904)
#define LNG_ConverterInvalidDestinationString           (-905)
#define LNG_ConverterInvalidDestinationStringLength     (-906)
#define LNG_ConverterConversionFailed                   (-907)

/* Location */
#define LNG_LocationInvalidLocale                       (-1000)

/* Unicode */
#define LNG_UnicodeInvalidConfigurationDirectoryPath    (-1100)
#define LNG_UnicodeInvalidNormalizationMode             (-1101)
#define LNG_UnicodeNormalizationUnsupported             (-1102)
#define LNG_UnicodeUnsupportedNormalizationMode         (-1103)
#define LNG_UnicodeInvalidNormalizer                    (-1104)
#define LNG_UnicodeFailedNormalizerCreate               (-1105)
#define LNG_UnicodeInvalidString                        (-1106)
#define LNG_UnicodeInvalidStringLength                  (-1107)
#define LNG_UnicodeInvalidNormalizedString              (-1108)
#define LNG_UnicodeInvalidNormalizedStringLength        (-1109)
#define LNG_UnicodeFailedNormalizeString                (-1110)
#define LNG_UnicodeInvalidUtf8String                    (-1111)


/*---------------------------------------------------------------------------*/


/* 
** C++ wrapper
*/

#if defined(__cplusplus)
}
#endif    /* defined(__cplusplus) */


/*---------------------------------------------------------------------------*/


#endif    /* !defined(LNG_ERR_H) */


/*---------------------------------------------------------------------------*/
