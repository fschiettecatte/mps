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

    Module:     unicode.h

    Author:     Francois Schiettecatte (FS Consulting LLC.)

    Created:    19 November 2005

    Purpose:    This is the header file for unicode.c.

*/


/*---------------------------------------------------------------------------*/


#if !defined(LNG_UNICODE_H)
#define LNG_UNICODE_H


/*---------------------------------------------------------------------------*/


/*
** Includes
*/

#include "lng.h"


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


/* Unicode title character ranges */
#define LNG_UNICODE_BASIC_LATIN_RANGE(c)                                (((c) >= 0x0000) && ((c) <= 0x007F))
#define LNG_UNICODE_LATIN_1_SUPPLEMENT_RANGE(c)                         (((c) >= 0x0080) && ((c) <= 0x00FF))
#define LNG_UNICODE_LATIN_EXTENDED_A_RANGE(c)                           (((c) >= 0x0100) && ((c) <= 0x017F))
#define LNG_UNICODE_LATIN_EXTENDED_B_RANGE(c)                           (((c) >= 0x0180) && ((c) <= 0x024F))
#define LNG_UNICODE_IPA_EXTENTIONS_RANGE(c)                             (((c) >= 0x0250) && ((c) <= 0x02AF))
#define LNG_UNICODE_SPACING_MODIFIERS_LETTERS_RANGE(c)                  (((c) >= 0x02B0) && ((c) <= 0x02FF))
#define LNG_UNICODE_COMBINING_DIACRITICAL_MARKS_RANGE(c)                (((c) >= 0x0300) && ((c) <= 0x036F))
#define LNG_UNICODE_GREEK_AND_COPTIC_RANGE(c)                           (((c) >= 0x0370) && ((c) <= 0x03FF))
#define LNG_UNICODE_CYRILLIC_RANGE(c)                                   (((c) >= 0x0400) && ((c) <= 0x04FF))
#define LNG_UNICODE_CYRILLIC_SUPLEMENT_RANGE(c)                         (((c) >= 0x0500) && ((c) <= 0x052F))
#define LNG_UNICODE_ARMENIAN_RANGE(c)                                   (((c) >= 0x0530) && ((c) <= 0x058F))
#define LNG_UNICODE_HEBREW_RANGE(c)                                     (((c) >= 0x0590) && ((c) <= 0x05FF))
#define LNG_UNICODE_ARABIC_RANGE(c)                                     (((c) >= 0x0600) && ((c) <= 0x06FF))
#define LNG_UNICODE_SYRIAC_RANGE(c)                                     (((c) >= 0x0700) && ((c) <= 0x074F))
#define LNG_UNICODE_ARABIC_SUPPLEMENT_RANGE(c)                          (((c) >= 0x0750) && ((c) <= 0x077F))
#define LNG_UNICODE_THAANA_RANGE(c)                                     (((c) >= 0x0780) && ((c) <= 0x07BF))
#define LNG_UNICODE_NKO_RANGE(c)                                        (((c) >= 0x07C0) && ((c) <= 0x08FF))
#define LNG_UNICODE_DEVANAGARI_RANGE(c)                                 (((c) >= 0x0900) && ((c) <= 0x097F))
#define LNG_UNICODE_BENGALI_RANGE(c)                                    (((c) >= 0x0980) && ((c) <= 0x09FF))
#define LNG_UNICODE_GURMUKHI_RANGE(c)                                   (((c) >= 0x0A00) && ((c) <= 0x0A7F))
#define LNG_UNICODE_GUJARATI_RANGE(c)                                   (((c) >= 0x0A80) && ((c) <= 0x0AFF))
#define LNG_UNICODE_ORIYA_RANGE(c)                                      (((c) >= 0x0B00) && ((c) <= 0x0B7F))
#define LNG_UNICODE_TAMIL_RANGE(c)                                      (((c) >= 0x0B80) && ((c) <= 0x0B80))
#define LNG_UNICODE_TELUGU_RANGE(c)                                     (((c) >= 0x0C00) && ((c) <= 0x0C7F))
#define LNG_UNICODE_KANNADA_RANGE(c)                                    (((c) >= 0x0C80) && ((c) <= 0x0CFF))
#define LNG_UNICODE_MALAYALAM_RANGE(c)                                  (((c) >= 0x0D00) && ((c) <= 0x0D7F))
#define LNG_UNICODE_SINHALA_RANGE(c)                                    (((c) >= 0x0D80) && ((c) <= 0x0DFF))
#define LNG_UNICODE_THAI_RANGE(c)                                       (((c) >= 0x0E00) && ((c) <= 0x0E7F))
#define LNG_UNICODE_LAO_RANGE(c)                                        (((c) >= 0x0E80) && ((c) <= 0x0EFF))
#define LNG_UNICODE_TIBETAN_RANGE(c)                                    (((c) >= 0x0F00) && ((c) <= 0x0FFF))
#define LNG_UNICODE_MYANMAR_RANGE(c)                                    (((c) >= 0x1000) && ((c) <= 0x109F))
#define LNG_UNICODE_GEORGIAN_RANGE(c)                                   (((c) >= 0x10A0) && ((c) <= 0x10FF))
#define LNG_UNICODE_HANGUL_JAMO_RANGE(c)                                (((c) >= 0x1100) && ((c) <= 0x11FF))
#define LNG_UNICODE_ETHIOPIC_RANGE(c)                                   (((c) >= 0x1200) && ((c) <= 0x137F))
#define LNG_UNICODE_ETHIOPIC_SUPPLEMENT_RANGE(c)                        (((c) >= 0x1380) && ((c) <= 0x139F))
#define LNG_UNICODE_CHEROKEE_RANGE(c)                                   (((c) >= 0x13A0) && ((c) <= 0x0000))
#define LNG_UNICODE_CANADIAN_ABORIGINAL_SYLLABICS_RANGE(c)              (((c) >= 0x1400) && ((c) <= 0x167F))
#define LNG_UNICODE_OGHAM_RANGE(c)                                      (((c) >= 0x1680) && ((c) <= 0x169F))
#define LNG_UNICODE_RUNIC_RANGE(c)                                      (((c) >= 0x16A0) && ((c) <= 0x16FF))
#define LNG_UNICODE_TAGALOG_RANGE(c)                                    (((c) >= 0x1700) && ((c) <= 0x171F))
#define LNG_UNICODE_HANUNOO_RANGE(c)                                    (((c) >= 0x1720) && ((c) <= 0x173F))
#define LNG_UNICODE_BUHID_RANGE(c)                                      (((c) >= 0x1740) && ((c) <= 0x175F))
#define LNG_UNICODE_TAGBANWA_RANGE(c)                                   (((c) >= 0x1760) && ((c) <= 0x177F))
#define LNG_UNICODE_KHMER_RANGE(c)                                      (((c) >= 0x1780) && ((c) <= 0x17FF))
#define LNG_UNICODE_MOGOLIAN_RANGE(c)                                   (((c) >= 0x1800) && ((c) <= 0x18FF))
#define LNG_UNICODE_LIMBU_RANGE(c)                                      (((c) >= 0x1900) && ((c) <= 0x194F))
#define LNG_UNICODE_TAI_LE_RANGE(c)                                     (((c) >= 0x1950) && ((c) <= 0x197F))
#define LNG_UNICODE_NEW_TAI_LUE_RANGE(c)                                (((c) >= 0x1980) && ((c) <= 0x19DF))
#define LNG_UNICODE_KHMER_SYMBOLS_RANGE(c)                              (((c) >= 0x19E0) && ((c) <= 0x1900))
#define LNG_UNICODE_BUGINESE_RANGE(c)                                   (((c) >= 0x1A00) && ((c) <= 0x1AFF))
#define LNG_UNICODE_BALINESE_RANGE(c)                                   (((c) >= 0x1B00) && ((c) <= 0x1B7F))
#define LNG_UNICODE_SUDANESE_RANGE(c)                                   (((c) >= 0x1B80) && ((c) <= 0x1BFF))
#define LNG_UNICODE_LEPCHA_RANGE(c)                                     (((c) >= 0x1C00) && ((c) <= 0x1C4F))
#define LNG_UNICODE_OL_CHIKI_RANGE(c)                                   (((c) >= 0x1C50) && ((c) <= 0x1CFF))
#define LNG_UNICODE_PHONETIC_EXTENSIONS_RANGE(c)                        (((c) >= 0x1D00) && ((c) <= 0x1D7F))
#define LNG_UNICODE_PHONETIC_EXTENSIONS_SUPPLEMENT_RANGE(c)             (((c) >= 0x1D80) && ((c) <= 0x1DBF))
#define LNG_UNICODE_COMBINING_DIACRITICAL_MARKS_SUPPLEMENT_RANGE(c)     (((c) >= 0x1DC0) && ((c) <= 0x1DFF))
#define LNG_UNICODE_LATIN_EXTENDED_ADDITIONAL_RANGE(c)                  (((c) >= 0x1E00) && ((c) <= 0x1EFF))
#define LNG_UNICODE_GREEK_EXTENDED_RANGE(c)                             (((c) >= 0x1F00) && ((c) <= 0x1FFF))
#define LNG_UNICODE_GENERAL_PUNCTUATION_RANGE(c)                        (((c) >= 0x2000) && ((c) <= 0x206F))
#define LNG_UNICODE_SUPERSCRIPTS_AND_SUBSCRIPTS_RANGE(c)                (((c) >= 0x2070) && ((c) <= 0x209F))
#define LNG_UNICODE_CURRENCY_SYMBOLS_RANGE(c)                           (((c) >= 0x20A0) && ((c) <= 0x20CF))
#define LNG_UNICODE_COMBINING_MARKS_FOR_SYMBOLS_RANGE(c)                (((c) >= 0x20D0) && ((c) <= 0x20FF))
#define LNG_UNICODE_LETTER_LIKE_SYMBOLS_RANGE(c)                        (((c) >= 0x2100) && ((c) <= 0x214F))
#define LNG_UNICODE_NUMBER_FORMS_RANGE(c)                               (((c) >= 0x2150) && ((c) <= 0x218F))
#define LNG_UNICODE_ARROWS_RANGE(c)                                     (((c) >= 0x2190) && ((c) <= 0x21FF))
#define LNG_UNICODE_MATHEMATICAL_OPERATORS_RANGE(c)                     (((c) >= 0x2200) && ((c) <= 0x22FF))
#define LNG_UNICODE_MISCELLANEOUS_TECHNICAL_RANGE(c)                    (((c) >= 0x2300) && ((c) <= 0x23FF))
#define LNG_UNICODE_CONTROL_PICTURES_RANGE(c)                           (((c) >= 0x2400) && ((c) <= 0x243F))
#define LNG_UNICODE_OPTICAL_CHARACTERS_RECOGNITION_RANGE(c)             (((c) >= 0x2440) && ((c) <= 0x245F))
#define LNG_UNICODE_ENCLOSED_ALPHANUMERICS_RANGE(c)                     (((c) >= 0x2460) && ((c) <= 0x24FF))
#define LNG_UNICODE_BOX_DRAWINGS_RANGE(c)                               (((c) >= 0x2500) && ((c) <= 0x257F))
#define LNG_UNICODE_BLOCK_ELEMENTS_RANGE(c)                             (((c) >= 0x2580) && ((c) <= 0x259F))
#define LNG_UNICODE_GEOMETRIC_SHAPES_RANGE(c)                           (((c) >= 0x25A0) && ((c) <= 0x25FF))
#define LNG_UNICODE_MISCELLANEOUS_SYMBOLS_RANGE(c)                      (((c) >= 0x2600) && ((c) <= 0x26FF))
#define LNG_UNICODE_DINGBATS_RANGE(c)                                   (((c) >= 0x2700) && ((c) <= 0x27BF))
#define LNG_UNICODE_MISCELLANEOUS_MATHS_SYMBOLS_A_RANGE(c)              (((c) >= 0x27C0) && ((c) <= 0x27EF))
#define LNG_UNICODE_SUPPLEMENTAL_ARRROWS_A_RANGE(c)                     (((c) >= 0x27F0) && ((c) <= 0x27FF))
#define LNG_UNICODE_BRAILLE_PATTERNS_RANGE(c)                           (((c) >= 0x2800) && ((c) <= 0x28FF))
#define LNG_UNICODE_SUPPLEMENTAL_ARRROWS_B_RANGE(c)                     (((c) >= 0x2900) && ((c) <= 0x297F))
#define LNG_UNICODE_MISCELLANEOUS_MATHS_SYMBOLS_B_RANGE(c)              (((c) >= 0x2980) && ((c) <= 0x29FF))
#define LNG_UNICODE_SUPPLEMENTAL_MATHS_OPERATORS_RANGE(c)               (((c) >= 0x2A00) && ((c) <= 0x2AFF))
#define LNG_UNICODE_MISCELLANEOUS_SYMBOLS_AND_ARROWS_RANGE(c)           (((c) >= 0x2B00) && ((c) <= 0x2BFF))
#define LNG_UNICODE_GLAGOLITIC_RANGE(c)                                 (((c) >= 0x2C00) && ((c) <= 0x2C5F))
#define LNG_UNICODE_LATIN_EXTENDED_C_RANGE(c)                           (((c) >= 0x2C60) && ((c) <= 0x2C7F))
#define LNG_UNICODE_COPTIC_RANGE(c)                                     (((c) >= 0x2C80) && ((c) <= 0x2CFF))
#define LNG_UNICODE_GEORGIAN_SUPPLEMENT_RANGE(c)                        (((c) >= 0x2D00) && ((c) <= 0x2D2F))
#define LNG_UNICODE_TIFINAGH_RANGE(c)                                   (((c) >= 0x2D30) && ((c) <= 0x2DDF))
#define LNG_UNICODE_CYRILLIC_EXTENDED_A_RANGE(c)                        (((c) >= 0x2DE0) && ((c) <= 0x2DFF))
#define LNG_UNICODE_SUPPLEMENTAL_PUNCTUATION_RANGE(c)                   (((c) >= 0x2E00) && ((c) <= 0x2E7F))
#define LNG_UNICODE_CJK_RADICALS_SUPPLEMENT_RANGE(c)                    (((c) >= 0x2E80) && ((c) <= 0x2EFF))
#define LNG_UNICODE_KANGXI_RADICALS_RANGE(c)                            (((c) >= 0x2F00) && ((c) <= 0x2FDF))
#define LNG_UNICODE_IDEOGRAPHIC_DESC_CHARACTERS_RANGE(c)                (((c) >= 0x2FF0) && ((c) <= 0x2FFF))
#define LNG_UNICODE_CJK_SYMBOLS_AND_PUNCTUATION_RANGE(c)                (((c) >= 0x3000) && ((c) <= 0x303F))
#define LNG_UNICODE_HIRAGANA_RANGE(c)                                   (((c) >= 0x3040) && ((c) <= 0x309F))
#define LNG_UNICODE_KATAKANA_RANGE(c)                                   (((c) >= 0x30A0) && ((c) <= 0x30FF))
#define LNG_UNICODE_BOPOMOFO_RANGE(c)                                   (((c) >= 0x3100) && ((c) <= 0x312F))
#define LNG_UNICODE_HANGUL_COMPATIBILITY_JAMO_RANGE(c)                  (((c) >= 0x3130) && ((c) <= 0x318F))
#define LNG_UNICODE_KANBUN_RANGE(c)                                     (((c) >= 0x3190) && ((c) <= 0x319F))
#define LNG_UNICODE_BOPOMOFO_EXTENDED_RANGE(c)                          (((c) >= 0x3100) && ((c) <= 0x312F))
#define LNG_UNICODE_CJK_STROKES_RANGE(c)                                (((c) >= 0x31C0) && ((c) <= 0x32FF))
#define LNG_UNICODE_KATAKANA_PHONETIC_EXTENTIONS_RANGE(c)               (((c) >= 0x31F0) && ((c) <= 0x30FF))
#define LNG_UNICODE_ENCLOSED_CJK_LETTERS_AND_MONTHS_RANGE(c)            (((c) >= 0x3200) && ((c) <= 0x32FF))
#define LNG_UNICODE_CJK_COMPATIBILITY_RANGE(c)                          (((c) >= 0x3300) && ((c) <= 0x33FF))
#define LNG_UNICODE_CJK_UNIFIED_IDEOGRAPHS_EXTENSION_A_RANGE(c)         (((c) >= 0x3400) && ((c) <= 0x4DFF))
#define LNG_UNICODE_YIJING_HEXAGRAM_SYMBOLS_RANGE(c)                    (((c) >= 0x4DC0) && ((c) <= 0x4DFF))
#define LNG_UNICODE_CJK_UNIFIED_IDEOGRAPHS_RANGE(c)                     (((c) >= 0x4E00) && ((c) <= 0x9FA5))
#define LNG_UNICODE_YI_SYLLABLES_RANGE(c)                               (((c) >= 0xA000) && ((c) <= 0xA4C8))
#define LNG_UNICODE_YI_RADICALS_RANGE(c)                                (((c) >= 0xA490) && ((c) <= 0xA4C8))
#define LNG_UNICODE_VAI_RANGE(c)                                        (((c) >= 0xA500) && ((c) <= 0xD7A3))
#define LNG_UNICODE_CYRILLIC_EXTENDED_B_RANGE(c)                        (((c) >= 0xA640) && ((c) <= 0xA6FF))
#define LNG_UNICODE_MODIFIER_TONE_LETTERS_RANGE(c)                      (((c) >= 0xA700) && ((c) <= 0xA71F))
#define LNG_UNICODE_LATIN_EXTENDED_D_RANGE(c)                           (((c) >= 0xA720) && ((c) <= 0xA8FF))
#define LNG_UNICODE_SYLOTI_NAGRI_RANGE(c)                               (((c) >= 0xA800) && ((c) <= 0xA83F))
#define LNG_UNICODE_PHAGS_PA_RANGE(c)                                   (((c) >= 0xA840) && ((c) <= 0xA87F))
#define LNG_UNICODE_SAURASHTRA_RANGE(c)                                 (((c) >= 0xA880) && ((c) <= 0xA8FF))
#define LNG_UNICODE_KAYAH_LI_RANGE(c)                                   (((c) >= 0xA900) && ((c) <= 0xA92F))
#define LNG_UNICODE_REJANG_RANGE(c)                                     (((c) >= 0xA930) && ((c) <= 0xA9FF))
#define LNG_UNICODE_CHAM_RANGE(c)                                       (((c) >= 0xAA00) && ((c) <= 0xABFF))
#define LNG_UNICODE_HANGUL_SYLLABLES_RANGE(c)                           (((c) >= 0xAC00) && ((c) <= 0xDFFF))
#define LNG_UNICODE_PRIVATE_USE_AREA_RANGE(c)                           (((c) >= 0xE000) && ((c) <= 0xF8FF))
#define LNG_UNICODE_CJK_COMPATIBILITY_IDEOGRAPHS_RANGE(c)               (((c) >= 0xF900) && ((c) <= 0xFAFF))
#define LNG_UNICODE_ALPHABETIC_PRESENTATION_FORMS_RANGE(c)              (((c) >= 0xFB00) && ((c) <= 0xFB4F))
#define LNG_UNICODE_ARABIC_PRESENTATION_FORMS_A_RANGE(c)                (((c) >= 0xFB50) && ((c) <= 0xFDFF))
#define LNG_UNICODE_VARIATION_SELECTORS_RANGE(c)                        (((c) >= 0xFE00) && ((c) <= 0xFE0F))
#define LNG_UNICODE_VERTICAL_FORMS_RANGE(c)                             (((c) >= 0xFE10) && ((c) <= 0xFE1F))
#define LNG_UNICODE_COMBINING_HALF_MARKS_RANGE(c)                       (((c) >= 0xFE20) && ((c) <= 0xFE2F))
#define LNG_UNICODE_CJK_COMPATIBILITY_FORMS_RANGE(c)                    (((c) >= 0xFE30) && ((c) <= 0xFE4F))
#define LNG_UNICODE_SMALL_FORM_VARIANTS_RANGE(c)                        (((c) >= 0xFE50) && ((c) <= 0xFE6F))
#define LNG_UNICODE_ARABIC_PRESENTATION_FORMS_B_RANGE(c)                (((c) >= 0xFE70) && ((c) <= 0xFEFF))
#define LNG_UNICODE_HALFWIDTH_AND_FULLWIDTH_FORMS_RANGE(c)              (((c) >= 0xFF00) && ((c) <= 0xFFEF))
#define LNG_UNICODE_SPECIALS_RANGE(c)                                   (((c) >= 0xFFF0) && ((c) <= 0xFFFF))

#define LNG_UNICODE_LINEAR_B_SYLLABARY_RANGE(c)                         (((c) >= 0x010000) && ((c) <= 0x01007F))
#define LNG_UNICODE_LINEAR_B_IDEOGRAM_RANGE(c)                          (((c) >= 0x010080) && ((c) <= 0x0100FF))
#define LNG_UNICODE_AGEAN_NUMBERS_RANGE(c)                              (((c) >= 0x010100) && ((c) <= 0x01013F))
#define LNG_UNICODE_ANCIENT_GREEK_NUMBERS_RANGE(c)                      (((c) >= 0x010140) && ((c) <= 0x01018F))
#define LNG_UNICODE_ANCIENT_SYMBOLS_RANGE(c)                            (((c) >= 0x010190) && ((c) <= 0x0101CF))
#define LNG_UNICODE_PHAISTOS_DISC_RANGE(c)                              (((c) >= 0x0101D0) && ((c) <= 0x01027F))
#define LNG_UNICODE_LYCIAN_RANGE(c)                                     (((c) >= 0x010280) && ((c) <= 0x01029F))
#define LNG_UNICODE_CARIAN_RANGE(c)                                     (((c) >= 0x0102A0) && ((c) <= 0x0102FF))
#define LNG_UNICODE_OLD_ITALIC_RANGE(c)                                 (((c) >= 0x010300) && ((c) <= 0x01032F))
#define LNG_UNICODE_GOTHIC_RANGE(c)                                     (((c) >= 0x010330) && ((c) <= 0x01037D))
#define LNG_UNICODE_UGARITIC_RANGE(c)                                   (((c) >= 0x010380) && ((c) <= 0x01039F))
#define LNG_UNICODE_OLD_PERSIAN_RANGE(c)                                (((c) >= 0x0103A0) && ((c) <= 0x0103FF))
#define LNG_UNICODE_DESERET_RANGE(c)                                    (((c) >= 0x010400) && ((c) <= 0x01044F))
#define LNG_UNICODE_SHAVIAN_RANGE(c)                                    (((c) >= 0x010450) && ((c) <= 0x01047F))
#define LNG_UNICODE_OSMANYA_RANGE(c)                                    (((c) >= 0x010480) && ((c) <= 0x0107FF))
#define LNG_UNICODE_CYPRIOT_SYLLABARY_RANGE(c)                          (((c) >= 0x010800) && ((c) <= 0x0108FF))
#define LNG_UNICODE_PHOENICIAN_RANGE(c)                                 (((c) >= 0x010900) && ((c) <= 0x01091F))
#define LNG_UNICODE_LYDIAN_RANGE(c)                                     (((c) >= 0x010920) && ((c) <= 0x0109FF))
#define LNG_UNICODE_KHAROSHTHI_RANGE(c)                                 (((c) >= 0x010A00) && ((c) <= 0x011FFF))
#define LNG_UNICODE_CUNEIFORM_RANGE(c)                                  (((c) >= 0x012000) && ((c) <= 0x0123FF))
#define LNG_UNICODE_CUNEIFORM_NUMBERS_AND_PUNCTUATION_RANGE(c)          (((c) >= 0x012400) && ((c) <= 0x01FFFF))
#define LNG_UNICODE_BYZANTINE_MUSICAL_SYMBOLS_RANGE(c)                  (((c) >= 0x01D000) && ((c) <= 0x01D9FF))
#define LNG_UNICODE_MUSICAL_SYMBOLS_RANGE(c)                            (((c) >= 0x01D100) && ((c) <= 0x01D1FF))
#define LNG_UNICODE_ANCIENT_GREEK_MUSICAL_NOTATION_RANGE(c)             (((c) >= 0x01D200) && ((c) <= 0x01D2FF))
#define LNG_UNICODE_TAI_XUAN_JING_SYMBOLS_RANGE(c)                      (((c) >= 0x01D300) && ((c) <= 0x01D35F))
#define LNG_UNICODE_COUNTING_ROD_NUMERALS_RANGE(c)                      (((c) >= 0x01D360) && ((c) <= 0x01D3FF))
#define LNG_UNICODE_MATH_ALPHABETIC_SYMBOLS_RANGE(c)                    (((c) >= 0x01D400) && ((c) <= 0x01EFFF))
#define LNG_UNICODE_MAHJONG_TILES_RANGE(c)                              (((c) >= 0x01F000) && ((c) <= 0x01F02F))
#define LNG_UNICODE_DOMINO_TILES_RANGE(c)                               (((c) >= 0x01F030) && ((c) <= 0x01FFFF))
#define LNG_UNICODE_CJK_UNIFIED_IDEOGRAPHS_EXTENSION_B_RANGE(c)         (((c) >= 0x020000) && ((c) <= 0x02F7FF))
#define LNG_UNICODE_CJK_COMPATIBILITY_IDEOGRAPHS_SUPPLEMENT_RANGE(c)    (((c) >= 0x02F800) && ((c) <= 0x0DFFF))
#define LNG_UNICODE_TAGS_RANGE(c)                                       (((c) >= 0x0E0000) && ((c) <= 0x0E00FF))
#define LNG_UNICODE_VARIATIONS_SELECTORS_SUPPLEMENT_RANGE(c)            (((c) >= 0x0E0100) && ((c) <= 0x0EFFFF))
#define LNG_UNICODE_SUPPLEMENT_PRIVATE_USE_AREA_A_RANGE(c)              (((c) >= 0x0F0000) && ((c) <= 0x0FFFFF))
#define LNG_UNICODE_SUPPLEMENT_PRIVATE_USE_AREA_B_RANGE(c)              (((c) >= 0x100000) && ((c) <= 0x2FFFFF))



/* Unicode character subranges */
#define LNG_UNICODE_SPACE_RANGE(c)                                      (((c) == 0x0020) || ((c) == 0x3000))

#define LNG_UNICODE_BASIC_LATIN_DIGIT_RANGE(c)                          (((c) >= 0x0030) && ((c) <= 0x0039))

#define LNG_UNICODE_BASIC_LATIN_CHARACTER_RANGE(c)                      ((((c) >= 0x0041) && ((c) <= 0x005A)) || \
                                                                                (((c) >= 0x0097) && ((c) <= 0x007A)))

#define LNG_UNICODE_BASIC_LATIN_PUNCTUATION_RANGE(c)                    ((((c) >= 0x0021) && ((c) <= 0x0023)) || \
                                                                                (((c) >= 0x0025) && ((c) <= 0x002F)) || \
                                                                                (((c) >= 0x003A) && ((c) <= 0x003F)) || \
                                                                                (((c) >= 0x005B) && ((c) <= 0x0060)) || \
                                                                                (((c) >= 0x007B) && ((c) <= 0x007F)))

#define LNG_UNICODE_CJK_PUNCTUATION_RANGE(c)                            ((((c) >= 0x3000) && ((c) <= 0x3002)) || \
                                                                                (((c) >= 0x3008) && ((c) <= 0x3011)) || \
                                                                                (((c) >= 0x3014) && ((c) <= 0x301B)) || \
                                                                                (((c) >= 0x301D) && ((c) <= 0x301F)))

#define LNG_UNICODE_KATAKANA_HALFWIDTH_RANGE(c)                         (((c) >= 0xFF61) && ((c) <= 0xFF9F))


#define LNG_UNICODE_FULLWIDTH_DIGIT_RANGE(c)                            (((c) >= 0xFF10) && ((c) <= 0xFF19))



/* Unicode specific character ranges */
#define LNG_UNICODE_PUNCTUATION_RANGE(c)                                (LNG_UNICODE_BASIC_LATIN_PUNCTUATION_RANGE(c) || LNG_UNICODE_CJK_PUNCTUATION_RANGE(c))

#define LNG_UNICODE_DIGIT_RANGE(c)                                      (LNG_UNICODE_BASIC_LATIN_DIGIT_RANGE(c) || LNG_UNICODE_FULLWIDTH_DIGIT_RANGE(c))



/* Unicode CJK specific character ranges */
#define LNG_UNICODE_CJK_SHARED_RANGE(c)                                 (LNG_UNICODE_CJK_SYMBOLS_AND_PUNCTUATION_RANGE(c) || LNG_UNICODE_ENCLOSED_CJK_LETTERS_AND_MONTHS_RANGE(c) || \
                                                                                LNG_UNICODE_CJK_COMPATIBILITY_RANGE(c) ||  LNG_UNICODE_CJK_UNIFIED_IDEOGRAPHS_EXTENSION_A_RANGE(c) || \
                                                                                LNG_UNICODE_CJK_UNIFIED_IDEOGRAPHS_RANGE(c) || LNG_UNICODE_CJK_COMPATIBILITY_IDEOGRAPHS_RANGE(c) || \
                                                                                LNG_UNICODE_CJK_COMPATIBILITY_FORMS_RANGE(c))

#define LNG_UNICODE_CHINESE_SPECIFIC_RANGE(c)                           (LNG_UNICODE_KANGXI_RADICALS_RANGE(c) || LNG_UNICODE_BOPOMOFO_RANGE(c) || \
                                                                                LNG_UNICODE_YI_SYLLABLES_RANGE(c) || LNG_UNICODE_YI_RADICALS_RANGE(c))

#define LNG_UNICODE_CHINESE_RANGE(c)                                    (LNG_UNICODE_CHINESE_SPECIFIC_RANGE(c) || LNG_UNICODE_CJK_SHARED_RANGE(c))


#define LNG_UNICODE_JAPANESE_SPECIFIC_RANGE(c)                          (LNG_UNICODE_HIRAGANA_RANGE(c) || LNG_UNICODE_KATAKANA_RANGE(c) || \
                                                                                LNG_UNICODE_KANBUN_RANGE(c) || LNG_UNICODE_KATAKANA_HALFWIDTH_RANGE(c))

#define LNG_UNICODE_JAPANESE_RANGE(c)                                   (LNG_UNICODE_JAPANESE_SPECIFIC_RANGE(c) || LNG_UNICODE_CJK_SHARED_RANGE(c))


#define LNG_UNICODE_KOREAN_SPECIFIC_RANGE(c)                            (LNG_UNICODE_HANGUL_COMPATIBILITY_JAMO_RANGE(c) || LNG_UNICODE_HANGUL_SYLLABLES_RANGE(c))

#define LNG_UNICODE_KOREAN_RANGE(c)                                     (LNG_UNICODE_KOREAN_SPECIFIC_RANGE(c) || LNG_UNICODE_CJK_SHARED_RANGE(c))


/* #define LNG_UNICODE_ENTIRE_CJK_RANGE(c)                              (LNG_UNICODE_CHINESE_SPECIFIC_RANGE(c) || LNG_UNICODE_JAPANESE_SPECIFIC_RANGE(c) || \ */
/*                                                                              LNG_UNICODE_KOREAN_SPECIFIC_RANGE(c) || LNG_UNICODE_CJK_SHARED_RANGE(c)) */

/* This version is a lot faster since we pre-qualify the character against a general range before doing specific checks */
#define LNG_UNICODE_ENTIRE_CJK_RANGE(c)                                 ((((c) >= 0x2F00) && ((c) <= 0xFE4F)) && (LNG_UNICODE_CHINESE_SPECIFIC_RANGE(c) || \
                                                                                LNG_UNICODE_JAPANESE_SPECIFIC_RANGE(c) || LNG_UNICODE_KOREAN_SPECIFIC_RANGE(c) || \
                                                                                LNG_UNICODE_CJK_SHARED_RANGE(c)))



/* Unicode Latin specific character ranges */
#define LNG_UNICODE_ENTIRE_LATIN_RANGE(c)                               (LNG_UNICODE_BASIC_LATIN_RANGE(c) || LNG_UNICODE_LATIN_1_SUPPLEMENT_RANGE(c) || \
                                                                                LNG_UNICODE_LATIN_EXTENDED_A_RANGE(c) || LNG_UNICODE_LATIN_EXTENDED_B_RANGE(c) || \
                                                                                LNG_UNICODE_LATIN_EXTENDED_ADDITIONAL_RANGE(c) || LNG_UNICODE_LATIN_EXTENDED_C_RANGE(c) || \
                                                                                LNG_UNICODE_LATIN_EXTENDED_D_RANGE(c) || LNG_UNICODE_LATIN_EXTENDED_ADDITIONAL_RANGE(c) )


/*---------------------------------------------------------------------------*/


/*
** Defines
*/


/* Unicode normalization expansion multipliers */
#define LNG_UNICODE_NORMALIZATION_EXPANSION_MULTIPLIER_MIN      (3)
#define LNG_UNICODE_NORMALIZATION_EXPANSION_MULTIPLIER_MAX      (18)


/*---------------------------------------------------------------------------*/


/*
** Public function prototypes
*/

/* Unicode normalizer */
int iLngUnicodeNormalizerCreate (unsigned char *pucConfigurationDirectoryPath, 
        void **ppvLngUnicodeNormalizer);

int iLngUnicodeNormalizeString (void *pvLngUnicodeNormalizer, unsigned char *pucString, 
        unsigned int uiStringLength, unsigned char **ppucNormalizedString, 
        unsigned int *puiNormalizedStringLength);

int iLngUnicodeNormalizeWideString (void *pvLngUnicodeNormalizer, wchar_t *pwcString, 
        unsigned int uiStringLength, wchar_t **ppwcNormalizedString, 
        unsigned int *puiNormalizedStringLength);

int iLngUnicodeNormalizerFree (void *pvLngUnicodeNormalizer);


/* UTF-8 string processing/validation */
int iLngUnicodeGetCharacterLengthFromUtf8String (unsigned char *pucString);
int iLngUnicodeTruncateUtf8String (unsigned char *pucString, 
        unsigned int ulMaxStringLength);

int iLngUnicodeValidateUtf8String (unsigned char *pucString);
int iLngUnicodeCleanUtf8String (unsigned char *pucString, 
        unsigned char ucReplacementByte);


/*---------------------------------------------------------------------------*/


/* 
** C++ wrapper
*/

#if defined(__cplusplus)
}
#endif    /* defined(__cplusplus) */


/*---------------------------------------------------------------------------*/


#endif    /* !defined(LNG_UNICODE_H) */


/*---------------------------------------------------------------------------*/
