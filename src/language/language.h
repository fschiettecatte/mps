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

    Module:     language.h

    Author:     Francois Schiettecatte (FS Consulting LLC.)

    Created:    30 April 2004

    Purpose:    Header file for language support
                functions located in language.c

*/


/*---------------------------------------------------------------------------*/


#if !defined(LNG_LANGUAGE_H)
#define LNG_LANGUAGE_H


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

#define LNG_LANGUAGE_ANY_CODE                   (unsigned char *)""
#define LNG_LANGUAGE_AA_CODE                    (unsigned char *)"aa"       /* Afar */
#define LNG_LANGUAGE_AB_CODE                    (unsigned char *)"ab"       /* Abkhazian */
#define LNG_LANGUAGE_AE_CODE                    (unsigned char *)"ae"       /* Avestan */
#define LNG_LANGUAGE_AF_CODE                    (unsigned char *)"af"       /* Afrikaans */
#define LNG_LANGUAGE_AK_CODE                    (unsigned char *)"ak"       /* Akan */
#define LNG_LANGUAGE_AM_CODE                    (unsigned char *)"am"       /* Amharic */
#define LNG_LANGUAGE_AN_CODE                    (unsigned char *)"an"       /* Aragonese */
#define LNG_LANGUAGE_AR_CODE                    (unsigned char *)"ar"       /* Arabic */
#define LNG_LANGUAGE_AS_CODE                    (unsigned char *)"as"       /* Assamese */
#define LNG_LANGUAGE_AV_CODE                    (unsigned char *)"av"       /* Avaric */
#define LNG_LANGUAGE_AY_CODE                    (unsigned char *)"ay"       /* Aymara */
#define LNG_LANGUAGE_AZ_CODE                    (unsigned char *)"az"       /* Azerbaijani */
#define LNG_LANGUAGE_BA_CODE                    (unsigned char *)"ba"       /* Bashkir */
#define LNG_LANGUAGE_BE_CODE                    (unsigned char *)"be"       /* Belarusian */
#define LNG_LANGUAGE_BG_CODE                    (unsigned char *)"bg"       /* Bulgarian */
#define LNG_LANGUAGE_BG_BG_CODE                 (unsigned char *)"bg_BG"    /* Bulgarian, as spoken in Bulgaria */
#define LNG_LANGUAGE_BH_CODE                    (unsigned char *)"bh"       /* Bihari */
#define LNG_LANGUAGE_BI_CODE                    (unsigned char *)"bi"       /* Bislama */
#define LNG_LANGUAGE_BM_CODE                    (unsigned char *)"bm"       /* Bambara */
#define LNG_LANGUAGE_BN_CODE                    (unsigned char *)"bn"       /* Bengali */
#define LNG_LANGUAGE_BO_CODE                    (unsigned char *)"bo"       /* Tibetan */
#define LNG_LANGUAGE_BR_CODE                    (unsigned char *)"br"       /* Breton */
#define LNG_LANGUAGE_BS_CODE                    (unsigned char *)"bs"       /* Bosnian */
#define LNG_LANGUAGE_CA_CODE                    (unsigned char *)"ca"       /* Catalan; Valencian */
#define LNG_LANGUAGE_CA_ES_CODE                 (unsigned char *)"ca_ES"    /* Catalan, as spoken in Spain */
#define LNG_LANGUAGE_CE_CODE                    (unsigned char *)"ce"       /* Chechen */
#define LNG_LANGUAGE_CH_CODE                    (unsigned char *)"ch"       /* Chamorro */
#define LNG_LANGUAGE_CO_CODE                    (unsigned char *)"co"       /* Corsican */
#define LNG_LANGUAGE_CR_CODE                    (unsigned char *)"cr"       /* Cree */
#define LNG_LANGUAGE_CS_CODE                    (unsigned char *)"cs"       /* Czech */
#define LNG_LANGUAGE_CS_CZ_CODE                 (unsigned char *)"cs_CZ"    /* Czech, as spoken in Czech Republic */
#define LNG_LANGUAGE_CU_CODE                    (unsigned char *)"cu"       /* Old Bulgarian; Old Slavonic; Church Slavonic; Church Slavic; Old Church Slavonic */
#define LNG_LANGUAGE_CV_CODE                    (unsigned char *)"cv"       /* Chuvash */
#define LNG_LANGUAGE_CY_CODE                    (unsigned char *)"cy"       /* Welsh */
#define LNG_LANGUAGE_DA_CODE                    (unsigned char *)"da"       /* Danish */
#define LNG_LANGUAGE_DA_DK_CODE                 (unsigned char *)"da_DK"    /* Danish, as spoken in Denmark */
#define LNG_LANGUAGE_DE_CODE                    (unsigned char *)"de"       /* German */
#define LNG_LANGUAGE_DE_AT_CODE                 (unsigned char *)"de_AT"    /* German, as spoken in Austria */
#define LNG_LANGUAGE_DE_GE_CODE                 (unsigned char *)"de_GE"    /* German, as spoken in Germany */
#define LNG_LANGUAGE_DV_CODE                    (unsigned char *)"dv"       /* Divehi */
#define LNG_LANGUAGE_DZ_CODE                    (unsigned char *)"dz"       /* Dzongkha */
#define LNG_LANGUAGE_EE_CODE                    (unsigned char *)"ee"       /* Ewe */
#define LNG_LANGUAGE_EL_CODE                    (unsigned char *)"el"       /* Greek, Modern (1453-) */
#define LNG_LANGUAGE_EL_GR_CODE                 (unsigned char *)"el_GR"    /* Greek, Modern (1453-), as spoken in Greece */
#define LNG_LANGUAGE_EN_CODE                    (unsigned char *)"en"       /* English */
#define LNG_LANGUAGE_EN_AU_CODE                 (unsigned char *)"en_AU"    /* English, as spoken in Australia */
#define LNG_LANGUAGE_EN_CA_CODE                 (unsigned char *)"en_CA"    /* English, as spoken in Canada */
#define LNG_LANGUAGE_EN_GB_CODE                 (unsigned char *)"en_GB"    /* English, as spoken in Great Britain */
#define LNG_LANGUAGE_EN_IE_CODE                 (unsigned char *)"en_IE"    /* English, as spoken in Ireland */
#define LNG_LANGUAGE_EN_SE_CODE                 (unsigned char *)"en_SE"    /* English, as spoken in Sweden */
#define LNG_LANGUAGE_EN_US_CODE                 (unsigned char *)"en_US"    /* English, as spoken in the United States */
#define LNG_LANGUAGE_EO_CODE                    (unsigned char *)"eo"       /* Esperanto */
#define LNG_LANGUAGE_ES_CODE                    (unsigned char *)"es"       /* Spanish; Castilian */
#define LNG_LANGUAGE_ES_AR_CODE                 (unsigned char *)"es_AR"    /* Spanish, as spoken in Argentina */
#define LNG_LANGUAGE_ES_BO_CODE                 (unsigned char *)"es_BO"    /* Spanish, as spoken in Bolivia */
#define LNG_LANGUAGE_ES_DO_CODE                 (unsigned char *)"es_DO"    /* Spanish, as spoken in Dominican Republic */
#define LNG_LANGUAGE_ES_ES_CODE                 (unsigned char *)"es_ES"    /* Spanish, as spoken in Spain */
#define LNG_LANGUAGE_ES_GT_CODE                 (unsigned char *)"es_GT"    /* Spanish, as spoken in Guatemala */
#define LNG_LANGUAGE_ES_HN_CODE                 (unsigned char *)"es_HN"    /* Spanish, as spoken in Honduras */
#define LNG_LANGUAGE_ES_MX_CODE                 (unsigned char *)"es_MX"    /* Spanish, as spoken in Mexico */
#define LNG_LANGUAGE_ES_NI_CODE                 (unsigned char *)"es_NI"    /* Spanish, as spoken in Nicaragua */
#define LNG_LANGUAGE_ES_PA_CODE                 (unsigned char *)"es_PA"    /* Spanish, as spoken in Panama */
#define LNG_LANGUAGE_ES_PE_CODE                 (unsigned char *)"es_PE"    /* Spanish, as spoken in Peru */
#define LNG_LANGUAGE_ES_SV_CODE                 (unsigned char *)"es_SV"    /* Spanish, as spoken in El Salvador */
#define LNG_LANGUAGE_ET_CODE                    (unsigned char *)"et"       /* Estonian */
#define LNG_LANGUAGE_ET_EE_CODE                 (unsigned char *)"et_EE"    /* Estonian, as spoken in Estonia */
#define LNG_LANGUAGE_EU_CODE                    (unsigned char *)"eu"       /* Basque */
#define LNG_LANGUAGE_EU_ES_CODE                 (unsigned char *)"eu_ES"    /* Basque, as spoken in Spain */
#define LNG_LANGUAGE_FA_CODE                    (unsigned char *)"fa"       /* Persian */
#define LNG_LANGUAGE_FA_IR_CODE                 (unsigned char *)"fa_IR"    /* Persian, as spoken in Iran */
#define LNG_LANGUAGE_FF_CODE                    (unsigned char *)"ff"       /* Fulah */
#define LNG_LANGUAGE_FI_CODE                    (unsigned char *)"fi"       /* Finnish */
#define LNG_LANGUAGE_FI_FI_CODE                 (unsigned char *)"fi_FI"    /* Finnish, as spoken in Finland */
#define LNG_LANGUAGE_FJ_CODE                    (unsigned char *)"fj"       /* Fijian */
#define LNG_LANGUAGE_FO_CODE                    (unsigned char *)"fo"       /* Faroese */
#define LNG_LANGUAGE_FO_FO_CODE                 (unsigned char *)"fo_FO"    /* Faroese as spoken in the Faroe Islands */
#define LNG_LANGUAGE_FR_CODE                    (unsigned char *)"fr"       /* French */
#define LNG_LANGUAGE_FR_BE_CODE                 (unsigned char *)"fr_BE"    /* French, as spoken in Belgium */
#define LNG_LANGUAGE_FR_CA_CODE                 (unsigned char *)"fr_CA"    /* French, as spoken in Canada */
#define LNG_LANGUAGE_FR_FR_CODE                 (unsigned char *)"fr_FR"    /* French, as spoken in France */
#define LNG_LANGUAGE_FY_CODE                    (unsigned char *)"fy"       /* Frisian */
#define LNG_LANGUAGE_GA_CODE                    (unsigned char *)"ga"       /* Irish */
#define LNG_LANGUAGE_GA_IE_CODE                 (unsigned char *)"ga_IE"    /* Irish, as spoken in Ireland */
#define LNG_LANGUAGE_GD_CODE                    (unsigned char *)"gd"       /* Gaelic; Scottish Gaelic */
#define LNG_LANGUAGE_GL_CODE                    (unsigned char *)"gl"       /* Gallegan */
#define LNG_LANGUAGE_GN_CODE                    (unsigned char *)"gn"       /* Guarani */
#define LNG_LANGUAGE_GU_CODE                    (unsigned char *)"gu"       /* Gujarati */
#define LNG_LANGUAGE_GV_CODE                    (unsigned char *)"gv"       /* Manx */
#define LNG_LANGUAGE_HA_CODE                    (unsigned char *)"ha"       /* Hausa */
#define LNG_LANGUAGE_HE_CODE                    (unsigned char *)"he"       /* Hebrew */
#define LNG_LANGUAGE_HE_IL_CODE                 (unsigned char *)"he_IL"    /* Hebrew, as spoken in Israel */
#define LNG_LANGUAGE_HI_CODE                    (unsigned char *)"hi"       /* Hindi */
#define LNG_LANGUAGE_HO_CODE                    (unsigned char *)"ho"       /* Hiri Motu */
#define LNG_LANGUAGE_HR_CODE                    (unsigned char *)"hr"       /* Croatian */
#define LNG_LANGUAGE_HT_CODE                    (unsigned char *)"ht"       /* Haitian Creole; Haitian */
#define LNG_LANGUAGE_HU_CODE                    (unsigned char *)"hu"       /* Hungarian */
#define LNG_LANGUAGE_HY_CODE                    (unsigned char *)"hy"       /* Armenian */
#define LNG_LANGUAGE_HZ_CODE                    (unsigned char *)"hz"       /* Herero */
#define LNG_LANGUAGE_IA_CODE                    (unsigned char *)"ia"       /* Interlingua (International Auxiliary Language Association) */
#define LNG_LANGUAGE_ID_CODE                    (unsigned char *)"id"       /* Indonesian */
#define LNG_LANGUAGE_ID_ID_CODE                 (unsigned char *)"id_ID"    /* Indonesian, as spoken in Indonesia */
#define LNG_LANGUAGE_IE_CODE                    (unsigned char *)"ie"       /* Interlingue */
#define LNG_LANGUAGE_IG_CODE                    (unsigned char *)"ig"       /* Igbo */
#define LNG_LANGUAGE_II_CODE                    (unsigned char *)"ii"       /* Sichuan Yi */
#define LNG_LANGUAGE_IK_CODE                    (unsigned char *)"ik"       /* Inupiaq */
#define LNG_LANGUAGE_IO_CODE                    (unsigned char *)"io"       /* Ido */
#define LNG_LANGUAGE_IS_CODE                    (unsigned char *)"is"       /* Icelandic */
#define LNG_LANGUAGE_IS_IS_CODE                 (unsigned char *)"is_IS"    /* Icelandic, as spoken in Iceland */
#define LNG_LANGUAGE_IT_CODE                    (unsigned char *)"it"       /* Italian */
#define LNG_LANGUAGE_IT_IT_CODE                 (unsigned char *)"it_IT"    /* Italian, as spoken in Italy */
#define LNG_LANGUAGE_IU_CODE                    (unsigned char *)"iu"       /* Inuktitut */
#define LNG_LANGUAGE_JA_CODE                    (unsigned char *)"ja"       /* Japanese */
#define LNG_LANGUAGE_JA_JP_CODE                 (unsigned char *)"ja_JP"    /* Japanese, as spoken in Japan */
#define LNG_LANGUAGE_JV_CODE                    (unsigned char *)"jv"       /* Javanese */
#define LNG_LANGUAGE_KA_CODE                    (unsigned char *)"ka"       /* Georgian */
#define LNG_LANGUAGE_KG_CODE                    (unsigned char *)"kg"       /* Kongo */
#define LNG_LANGUAGE_KI_CODE                    (unsigned char *)"ki"       /* Gikuyu; Kikuyu */
#define LNG_LANGUAGE_KJ_CODE                    (unsigned char *)"kj"       /* Kuanyama; Kwanyama */
#define LNG_LANGUAGE_KK_CODE                    (unsigned char *)"kk"       /* Kazakh */
#define LNG_LANGUAGE_KL_CODE                    (unsigned char *)"kl"       /* Greenlandic; Kalaallisut */
#define LNG_LANGUAGE_KL_GL_CODE                 (unsigned char *)"kl_GL"    /* Kalaallisut, as spoken in Greenland */
#define LNG_LANGUAGE_KM_CODE                    (unsigned char *)"km"       /* Khmer */
#define LNG_LANGUAGE_KN_CODE                    (unsigned char *)"kn"       /* Kannada */
#define LNG_LANGUAGE_KO_CODE                    (unsigned char *)"ko"       /* Korean */
#define LNG_LANGUAGE_KR_CODE                    (unsigned char *)"kr"       /* Kanuri */
#define LNG_LANGUAGE_KS_CODE                    (unsigned char *)"ks"       /* Kashmiri */
#define LNG_LANGUAGE_KU_CODE                    (unsigned char *)"ku"       /* Kurdish */
#define LNG_LANGUAGE_KV_CODE                    (unsigned char *)"kv"       /* Komi */
#define LNG_LANGUAGE_KW_CODE                    (unsigned char *)"kw"       /* Cornish */
#define LNG_LANGUAGE_KY_CODE                    (unsigned char *)"ky"       /* Kirghiz */
#define LNG_LANGUAGE_LA_CODE                    (unsigned char *)"la"       /* Latin */
#define LNG_LANGUAGE_LB_CODE                    (unsigned char *)"lb"       /* Letzeburgesch; Luxembourgish */
#define LNG_LANGUAGE_LG_CODE                    (unsigned char *)"lg"       /* Ganda */
#define LNG_LANGUAGE_LI_CODE                    (unsigned char *)"li"       /* Limburgan; Limburger; Limburgish */
#define LNG_LANGUAGE_LN_CODE                    (unsigned char *)"ln"       /* Lingala */
#define LNG_LANGUAGE_LO_CODE                    (unsigned char *)"lo"       /* Lao */
#define LNG_LANGUAGE_LT_CODE                    (unsigned char *)"lt"       /* Lithuanian */
#define LNG_LANGUAGE_LT_LT_CODE                 (unsigned char *)"lt_LT"    /* Lithuanian, as spoken in Lithuania */
#define LNG_LANGUAGE_LU_CODE                    (unsigned char *)"lu"       /* Luba-Katanga */
#define LNG_LANGUAGE_LV_CODE                    (unsigned char *)"lv"       /* Latvian */
#define LNG_LANGUAGE_LV_LV_CODE                 (unsigned char *)"lv_LV"    /* Latvian, as spoken in Latvia */
#define LNG_LANGUAGE_MG_CODE                    (unsigned char *)"mg"       /* Malagasy */
#define LNG_LANGUAGE_MH_CODE                    (unsigned char *)"mh"       /* Marshallese */
#define LNG_LANGUAGE_MI_CODE                    (unsigned char *)"mi"       /* Maori */
#define LNG_LANGUAGE_MK_CODE                    (unsigned char *)"mk"       /* Macedonian */
#define LNG_LANGUAGE_ML_CODE                    (unsigned char *)"ml"       /* Malayalam */
#define LNG_LANGUAGE_MN_CODE                    (unsigned char *)"mn"       /* Mongolian */
#define LNG_LANGUAGE_MO_CODE                    (unsigned char *)"mo"       /* Moldavian */
#define LNG_LANGUAGE_MR_CODE                    (unsigned char *)"mr"       /* Marathi */
#define LNG_LANGUAGE_MS_CODE                    (unsigned char *)"ms"       /* Malay */
#define LNG_LANGUAGE_MT_CODE                    (unsigned char *)"mt"       /* Maltese */
#define LNG_LANGUAGE_MY_CODE                    (unsigned char *)"my"       /* Burmese */
#define LNG_LANGUAGE_NA_CODE                    (unsigned char *)"na"       /* Nauru */
#define LNG_LANGUAGE_NB_CODE                    (unsigned char *)"nb"       /* Bokmal, Norwegian */
#define LNG_LANGUAGE_ND_CODE                    (unsigned char *)"nd"       /* North Ndebele */
#define LNG_LANGUAGE_NE_CODE                    (unsigned char *)"ne"       /* Nepali */
#define LNG_LANGUAGE_NG_CODE                    (unsigned char *)"ng"       /* Ndonga */
#define LNG_LANGUAGE_NL_CODE                    (unsigned char *)"nl"       /* Dutch; Flemish */
#define LNG_LANGUAGE_NL_BE_CODE                 (unsigned char *)"nl_BE"    /* Dutch, as spoken in Belgium */
#define LNG_LANGUAGE_NL_NL_CODE                 (unsigned char *)"nl_NL"    /* Dutch, as spoken in Netherlands */
#define LNG_LANGUAGE_NN_CODE                    (unsigned char *)"nn"       /* Norwegian Nynorsk; */
#define LNG_LANGUAGE_NO_CODE                    (unsigned char *)"no"       /* Norwegian */
#define LNG_LANGUAGE_NO_NO_CODE                 (unsigned char *)"no_NO"    /* Norwegian, as spoken in Norway */
#define LNG_LANGUAGE_NO_NY_CODE                 (unsigned char *)"no_NY"    /* Norwegian, as spoken in NY ?? */
#define LNG_LANGUAGE_NR_CODE                    (unsigned char *)"nr"       /* South Ndebele */
#define LNG_LANGUAGE_NV_CODE                    (unsigned char *)"nv"       /* Navajo; Navaho */
#define LNG_LANGUAGE_NY_CODE                    (unsigned char *)"ny"       /* Chewa; Chichewa; Nyanja */
#define LNG_LANGUAGE_OC_CODE                    (unsigned char *)"oc"       /* Provencal; Occitan (post 1500)  */
#define LNG_LANGUAGE_OJ_CODE                    (unsigned char *)"oj"       /* Ojibwa */
#define LNG_LANGUAGE_OM_CODE                    (unsigned char *)"om"       /* Oromo */
#define LNG_LANGUAGE_OR_CODE                    (unsigned char *)"or"       /* Oriya */
#define LNG_LANGUAGE_OS_CODE                    (unsigned char *)"os"       /* Ossetian; Ossetic */
#define LNG_LANGUAGE_PA_CODE                    (unsigned char *)"pa"       /* Panjabi; Punjabi */
#define LNG_LANGUAGE_PI_CODE                    (unsigned char *)"pi"       /* Pali */
#define LNG_LANGUAGE_PL_CODE                    (unsigned char *)"pl"       /* Polish */
#define LNG_LANGUAGE_PL_PL_CODE                 (unsigned char *)"pl_PL"    /* Polish, as spoken in Poland */
#define LNG_LANGUAGE_PS_CODE                    (unsigned char *)"ps"       /* Pushto */
#define LNG_LANGUAGE_PT_CODE                    (unsigned char *)"pt"       /* Portuguese */
#define LNG_LANGUAGE_PT_BR_CODE                 (unsigned char *)"pt_BR"    /* Portuguese, as spoken in Brazil */
#define LNG_LANGUAGE_PT_PT_CODE                 (unsigned char *)"pt_PT"    /* Portuguese, as spoken in Portugal */
#define LNG_LANGUAGE_QU_CODE                    (unsigned char *)"qu"       /* Quechua */
#define LNG_LANGUAGE_RM_CODE                    (unsigned char *)"rm"       /* Raeto-Romance */
#define LNG_LANGUAGE_RN_CODE                    (unsigned char *)"rn"       /* Rundi */
#define LNG_LANGUAGE_RO_CODE                    (unsigned char *)"ro"       /* Romanian */
#define LNG_LANGUAGE_RO_RO_CODE                 (unsigned char *)"ro_RO"    /* Romanian, as spoken in Romania */
#define LNG_LANGUAGE_RU_CODE                    (unsigned char *)"ru"       /* Russian */
#define LNG_LANGUAGE_RU_RU_CODE                 (unsigned char *)"ru_RU"    /* Russian, as spoken in Russia */
#define LNG_LANGUAGE_RW_CODE                    (unsigned char *)"rw"       /* Kinyarwanda */
#define LNG_LANGUAGE_SA_CODE                    (unsigned char *)"sa"       /* Sanskrit */
#define LNG_LANGUAGE_SC_CODE                    (unsigned char *)"sc"       /* Sardinian */
#define LNG_LANGUAGE_SD_CODE                    (unsigned char *)"sd"       /* Sindhi */
#define LNG_LANGUAGE_SE_CODE                    (unsigned char *)"se"       /* Northern Sami */
#define LNG_LANGUAGE_SG_CODE                    (unsigned char *)"sg"       /* Sango */
#define LNG_LANGUAGE_SI_CODE                    (unsigned char *)"si"       /* Sinhalese; Sinhala */
#define LNG_LANGUAGE_SK_CODE                    (unsigned char *)"sk"       /* Slovak */
#define LNG_LANGUAGE_SK_SK_CODE                 (unsigned char *)"sk_SK"    /* Slovak, as spoken in Slovakia */
#define LNG_LANGUAGE_SL_CODE                    (unsigned char *)"sl"       /* Slovenian */
#define LNG_LANGUAGE_SL_SI_CODE                 (unsigned char *)"sl_SI"    /* Slovenian, as spoken in Slovenia */
#define LNG_LANGUAGE_SM_CODE                    (unsigned char *)"sm"       /* Samoan */
#define LNG_LANGUAGE_SN_CODE                    (unsigned char *)"sn"       /* Shona */
#define LNG_LANGUAGE_SO_CODE                    (unsigned char *)"so"       /* Somali */
#define LNG_LANGUAGE_SQ_CODE                    (unsigned char *)"sq"       /* Albanian */
#define LNG_LANGUAGE_SR_CODE                    (unsigned char *)"sr"       /* Serbian */
#define LNG_LANGUAGE_SR_SR_CODE                 (unsigned char *)"sr_SR"    /* Serbian, as spoken in Serbia */
#define LNG_LANGUAGE_SS_CODE                    (unsigned char *)"ss"       /* Swati */
#define LNG_LANGUAGE_ST_CODE                    (unsigned char *)"st"       /* Sotho, Southern */
#define LNG_LANGUAGE_SU_CODE                    (unsigned char *)"su"       /* Sundanese */
#define LNG_LANGUAGE_SV_CODE                    (unsigned char *)"sv"       /* Swedish */
#define LNG_LANGUAGE_SV_SE_CODE                 (unsigned char *)"sv_SE"    /* Swedish, as spoken in Sweden */
#define LNG_LANGUAGE_SW_CODE                    (unsigned char *)"sw"       /* Swahili */
#define LNG_LANGUAGE_TA_CODE                    (unsigned char *)"ta"       /* Tamil */
#define LNG_LANGUAGE_TA_IN_CODE                 (unsigned char *)"ta_IN"    /* Tamil, as spoken in India */
#define LNG_LANGUAGE_TE_CODE                    (unsigned char *)"te"       /* Telugu */
#define LNG_LANGUAGE_TG_CODE                    (unsigned char *)"tg"       /* Tajik */
#define LNG_LANGUAGE_TH_CODE                    (unsigned char *)"th"       /* Thai */
#define LNG_LANGUAGE_TI_CODE                    (unsigned char *)"ti"       /* Tigrinya */
#define LNG_LANGUAGE_TK_CODE                    (unsigned char *)"tk"       /* Turkmen */
#define LNG_LANGUAGE_TL_CODE                    (unsigned char *)"tl"       /* Tagalog */
#define LNG_LANGUAGE_TN_CODE                    (unsigned char *)"tn"       /* Tswana */
#define LNG_LANGUAGE_TO_CODE                    (unsigned char *)"to"       /* Tonga (Tonga Islands) */
#define LNG_LANGUAGE_TR_CODE                    (unsigned char *)"tr"       /* Turkish */
#define LNG_LANGUAGE_TR_TR_CODE                 (unsigned char *)"tr_TR"    /* Turkish, as spoken in Turkey */
#define LNG_LANGUAGE_TS_CODE                    (unsigned char *)"ts"       /* Tsonga */
#define LNG_LANGUAGE_TT_CODE                    (unsigned char *)"tt"       /* Tatar */
#define LNG_LANGUAGE_TW_CODE                    (unsigned char *)"tw"       /* Twi */
#define LNG_LANGUAGE_TY_CODE                    (unsigned char *)"ty"       /* Tahitian */
#define LNG_LANGUAGE_UG_CODE                    (unsigned char *)"ug"       /* Uighur; Uyghur */
#define LNG_LANGUAGE_UK_CODE                    (unsigned char *)"uk"       /* Ukrainian */
#define LNG_LANGUAGE_UK_UA_CODE                 (unsigned char *)"uk_UA"    /* Ukrainian, as spoken in Ukraine */
#define LNG_LANGUAGE_UN_CODE                    (unsigned char *)"un"       /* Unknown */
#define LNG_LANGUAGE_UR_CODE                    (unsigned char *)"ur"       /* Urdu */
#define LNG_LANGUAGE_UZ_CODE                    (unsigned char *)"uz"       /* Uzbek */
#define LNG_LANGUAGE_VE_CODE                    (unsigned char *)"ve"       /* Venda */
#define LNG_LANGUAGE_VI_CODE                    (unsigned char *)"vi"       /* Vietnamese */
#define LNG_LANGUAGE_VI_VN_CODE                 (unsigned char *)"vi_VN"    /* Vietnamese, as spoken in Vietnam */
#define LNG_LANGUAGE_VO_CODE                    (unsigned char *)"vo"       /* Volapuk */
#define LNG_LANGUAGE_WA_CODE                    (unsigned char *)"wa"       /* Walloon */
#define LNG_LANGUAGE_WO_CODE                    (unsigned char *)"wo"       /* Wolof */
#define LNG_LANGUAGE_XH_CODE                    (unsigned char *)"xh"       /* Xhosa */
#define LNG_LANGUAGE_YI_CODE                    (unsigned char *)"yi"       /* Yiddish */
#define LNG_LANGUAGE_YO_CODE                    (unsigned char *)"yo"       /* Yoruba */
#define LNG_LANGUAGE_ZA_CODE                    (unsigned char *)"za"       /* Chuang; Zhuang */
#define LNG_LANGUAGE_ZH_CODE                    (unsigned char *)"zh"       /* Chinese */
#define LNG_LANGUAGE_ZH_SC_CODE                 (unsigned char *)"zh_SC"    /* Chinese; simplified */
#define LNG_LANGUAGE_ZH_TC_CODE                 (unsigned char *)"zh_TC"    /* Chinese; traditional */
#define LNG_LANGUAGE_ZH_CN_CODE                 (unsigned char *)"zh_CN"    /* Chinese, as spoken in China */
#define LNG_LANGUAGE_ZH_HK_CODE                 (unsigned char *)"zh_HK"    /* Chinese, as spoken in Hong Kong */
#define LNG_LANGUAGE_ZH_TW_CODE                 (unsigned char *)"zh_TW"    /* Chinese, as spoken in Taiwan */
#define LNG_LANGUAGE_ZU_CODE                    (unsigned char *)"zu"       /* Zulu */

#define LNG_LANGUAGE_ANY_ID                     (0)
#define LNG_LANGUAGE_AA_ID                      (101)                       /* Afar */
#define LNG_LANGUAGE_AB_ID                      (102)                       /* Abkhazian */
#define LNG_LANGUAGE_AE_ID                      (103)                       /* Avestan */
#define LNG_LANGUAGE_AF_ID                      (104)                       /* Afrikaans */
#define LNG_LANGUAGE_AK_ID                      (105)                       /* Akan */
#define LNG_LANGUAGE_AM_ID                      (106)                       /* Amharic */
#define LNG_LANGUAGE_AN_ID                      (107)                       /* Aragonese */
#define LNG_LANGUAGE_AR_ID                      (108)                       /* Arabic */
#define LNG_LANGUAGE_AS_ID                      (109)                       /* Assamese */
#define LNG_LANGUAGE_AV_ID                      (110)                       /* Avaric */
#define LNG_LANGUAGE_AY_ID                      (111)                       /* Aymara */
#define LNG_LANGUAGE_AZ_ID                      (112)                       /* Azerbaijani */
#define LNG_LANGUAGE_BA_ID                      (113)                       /* Bashkir */
#define LNG_LANGUAGE_BE_ID                      (114)                       /* Belarusian */
#define LNG_LANGUAGE_BG_ID                      (115)                       /* Bulgarian */
#define LNG_LANGUAGE_BG_BG_ID                   (11500)                     /* Bulgarian, as spoken in Bulgaria */
#define LNG_LANGUAGE_BH_ID                      (116)                       /* Bihari */
#define LNG_LANGUAGE_BI_ID                      (117)                       /* Bislama */
#define LNG_LANGUAGE_BM_ID                      (118)                       /* Bambara */
#define LNG_LANGUAGE_BN_ID                      (119)                       /* Bengali */
#define LNG_LANGUAGE_BO_ID                      (120)                       /* Tibetan */
#define LNG_LANGUAGE_BR_ID                      (121)                       /* Breton */
#define LNG_LANGUAGE_BS_ID                      (122)                       /* Bosnian */
#define LNG_LANGUAGE_CA_ID                      (123)                       /* Catalan; Valencian */
#define LNG_LANGUAGE_CA_ES_ID                   (12300)                     /* Catalan, as spoken in Spain */
#define LNG_LANGUAGE_CE_ID                      (124)                       /* Chechen */
#define LNG_LANGUAGE_CH_ID                      (125)                       /* Chamorro */
#define LNG_LANGUAGE_CO_ID                      (126)                       /* Corsican */
#define LNG_LANGUAGE_CR_ID                      (127)                       /* Cree */
#define LNG_LANGUAGE_CS_ID                      (128)                       /* Czech */
#define LNG_LANGUAGE_CS_CZ_ID                   (12800)                     /* Czech, as spoken in Czech Republic */
#define LNG_LANGUAGE_CU_ID                      (129)                       /* Old Bulgarian; Old Slavonic; Church Slavonic; Church Slavic; Old Church Slavonic */
#define LNG_LANGUAGE_CV_ID                      (130)                       /* Chuvash */
#define LNG_LANGUAGE_CY_ID                      (131)                       /* Welsh */
#define LNG_LANGUAGE_DA_ID                      (132)                       /* Danish */
#define LNG_LANGUAGE_DA_DK_ID                   (13200)                     /* Danish, as spoken in Denmark */
#define LNG_LANGUAGE_DE_ID                      (133)                       /* German */
#define LNG_LANGUAGE_DE_AT_ID                   (13300)                     /* German, as spoken in Austria */
#define LNG_LANGUAGE_DE_GE_ID                   (13301)                     /* German, as spoken in Germany */
#define LNG_LANGUAGE_DV_ID                      (134)                       /* Divehi */
#define LNG_LANGUAGE_DZ_ID                      (135)                       /* Dzongkha */
#define LNG_LANGUAGE_EE_ID                      (136)                       /* Ewe */
#define LNG_LANGUAGE_EL_ID                      (137)                       /* Greek, Modern (1453-) */
#define LNG_LANGUAGE_EL_GR_ID                   (13700)                     /* Greek, Modern (1453-), as spoken in Greece */
#define LNG_LANGUAGE_EN_ID                      (138)                       /* English */
#define LNG_LANGUAGE_EN_AU_ID                   (13800)                     /* English, as spoken in Australia */
#define LNG_LANGUAGE_EN_CA_ID                   (13801)                     /* English, as spoken in Canada */
#define LNG_LANGUAGE_EN_GB_ID                   (13802)                     /* English, as spoken in Great Britain */
#define LNG_LANGUAGE_EN_IE_ID                   (13803)                     /* English, as spoken in Ireland */
#define LNG_LANGUAGE_EN_SE_ID                   (13804)                     /* English, as spoken in Sweden */
#define LNG_LANGUAGE_EN_US_ID                   (13805)                     /* English, as spoken in the United States */
#define LNG_LANGUAGE_EO_ID                      (139)                       /* Esperanto */
#define LNG_LANGUAGE_ES_ID                      (140)                       /* Spanish; Castilian */
#define LNG_LANGUAGE_ES_AR_ID                   (14000)                     /* Spanish, as spoken in Argentina */
#define LNG_LANGUAGE_ES_BO_ID                   (14001)                     /* Spanish, as spoken in Bolivia */
#define LNG_LANGUAGE_ES_DO_ID                   (14002)                     /* Spanish, as spoken in Dominican Republic */
#define LNG_LANGUAGE_ES_ES_ID                   (14003)                     /* Spanish, as spoken in Spain */
#define LNG_LANGUAGE_ES_GT_ID                   (14004)                     /* Spanish, as spoken in Guatemala */
#define LNG_LANGUAGE_ES_HN_ID                   (14005)                     /* Spanish, as spoken in Honduras */
#define LNG_LANGUAGE_ES_MX_ID                   (14006)                     /* Spanish, as spoken in Mexico */
#define LNG_LANGUAGE_ES_NI_ID                   (14007)                     /* Spanish, as spoken in Nicaragua */
#define LNG_LANGUAGE_ES_PA_ID                   (14008)                     /* Spanish, as spoken in Panama */
#define LNG_LANGUAGE_ES_PE_ID                   (14009)                     /* Spanish, as spoken in Peru */
#define LNG_LANGUAGE_ES_SV_ID                   (14010)                     /* Spanish, as spoken in El Salvador */
#define LNG_LANGUAGE_ET_ID                      (141)                       /* Estonian */
#define LNG_LANGUAGE_ET_EE_ID                   (14100)                     /* Estonian, as spoken in Estonia */
#define LNG_LANGUAGE_EU_ID                      (142)                       /* Basque */
#define LNG_LANGUAGE_EU_ES_ID                   (14200)                     /* Basque, as spoken in Spain */
#define LNG_LANGUAGE_FA_ID                      (143)                       /* Persian */
#define LNG_LANGUAGE_FA_IR_ID                   (14300)                     /* Persian, as spoken in Iran */
#define LNG_LANGUAGE_FF_ID                      (144)                       /* Fulah */
#define LNG_LANGUAGE_FI_ID                      (145)                       /* Finnish */
#define LNG_LANGUAGE_FI_FI_ID                   (14500)                     /* Finnish, as spoken in Finland */
#define LNG_LANGUAGE_FJ_ID                      (146)                       /* Fijian */
#define LNG_LANGUAGE_FO_ID                      (147)                       /* Faroese */
#define LNG_LANGUAGE_FO_FO_ID                   (14700)                     /* Faroese, as spoken in the Faroe Islands */
#define LNG_LANGUAGE_FR_ID                      (148)                       /* French */
#define LNG_LANGUAGE_FR_BE_ID                   (14800)                     /* French, as spoken in Belgium */
#define LNG_LANGUAGE_FR_CA_ID                   (14801)                     /* French, as spoken in Canada */
#define LNG_LANGUAGE_FR_FR_ID                   (14802)                     /* French, as spoken in France */
#define LNG_LANGUAGE_FY_ID                      (149)                       /* Frisian */
#define LNG_LANGUAGE_GA_ID                      (150)                       /* Irish */
#define LNG_LANGUAGE_GA_IE_ID                   (15000)                     /* Irish, as spoken in Ireland */
#define LNG_LANGUAGE_GD_ID                      (151)                       /* Gaelic; Scottish Gaelic */
#define LNG_LANGUAGE_GL_ID                      (152)                       /* Gallegan */
#define LNG_LANGUAGE_GN_ID                      (153)                       /* Guarani */
#define LNG_LANGUAGE_GU_ID                      (154)                       /* Gujarati */
#define LNG_LANGUAGE_GV_ID                      (155)                       /* Manx */
#define LNG_LANGUAGE_HA_ID                      (156)                       /* Hausa */
#define LNG_LANGUAGE_HE_ID                      (157)                       /* Hebrew */
#define LNG_LANGUAGE_HE_IL_ID                   (15700)                     /* Hebrew, as spoken in Israel */
#define LNG_LANGUAGE_HI_ID                      (158)                       /* Hindi */
#define LNG_LANGUAGE_HO_ID                      (159)                       /* Hiri Motu */
#define LNG_LANGUAGE_HR_ID                      (160)                       /* Croatian */
#define LNG_LANGUAGE_HT_ID                      (161)                       /* Haitian Creole; Haitian */
#define LNG_LANGUAGE_HU_ID                      (162)                       /* Hungarian */
#define LNG_LANGUAGE_HY_ID                      (163)                       /* Armenian */
#define LNG_LANGUAGE_HZ_ID                      (164)                       /* Herero */
#define LNG_LANGUAGE_IA_ID                      (165)                       /* Interlingua (International Auxiliary Language Association) */
#define LNG_LANGUAGE_ID_ID                      (166)                       /* Indonesian */
#define LNG_LANGUAGE_ID_ID_ID                   (16600)                     /* Indonesian, as spoken in Indonesia */
#define LNG_LANGUAGE_IE_ID                      (167)                       /* Interlingue */
#define LNG_LANGUAGE_IG_ID                      (168)                       /* Igbo */
#define LNG_LANGUAGE_II_ID                      (169)                       /* Sichuan Yi */
#define LNG_LANGUAGE_IK_ID                      (170)                       /* Inupiaq */
#define LNG_LANGUAGE_IO_ID                      (171)                       /* Ido */
#define LNG_LANGUAGE_IS_ID                      (172)                       /* Icelandic */
#define LNG_LANGUAGE_IS_IS_ID                   (17200)                     /* Icelandic, as spoken in Iceland */
#define LNG_LANGUAGE_IT_ID                      (173)                       /* Italian */
#define LNG_LANGUAGE_IT_IT_ID                   (17300)                     /* Italian, as spoken in Italy */
#define LNG_LANGUAGE_IU_ID                      (174)                       /* Inuktitut */
#define LNG_LANGUAGE_JA_ID                      (175)                       /* Japanese */
#define LNG_LANGUAGE_JA_JP_ID                   (17500)                     /* Japanese, as spoken in Japan */
#define LNG_LANGUAGE_JV_ID                      (176)                       /* Javanese */
#define LNG_LANGUAGE_KA_ID                      (177)                       /* Georgian */
#define LNG_LANGUAGE_KG_ID                      (178)                       /* Kongo */
#define LNG_LANGUAGE_KI_ID                      (179)                       /* Gikuyu; Kikuyu */
#define LNG_LANGUAGE_KJ_ID                      (180)                       /* Kuanyama; Kwanyama */
#define LNG_LANGUAGE_KK_ID                      (181)                       /* Kazakh */
#define LNG_LANGUAGE_KL_ID                      (182)                       /* Greenlandic; Kalaallisut */
#define LNG_LANGUAGE_KL_GL_ID                   (18200)                     /* Kalaallisut, as spoken in Greenland */
#define LNG_LANGUAGE_KM_ID                      (183)                       /* Khmer */
#define LNG_LANGUAGE_KN_ID                      (184)                       /* Kannada */
#define LNG_LANGUAGE_KO_ID                      (185)                       /* Korean */
#define LNG_LANGUAGE_KR_ID                      (186)                       /* Kanuri */
#define LNG_LANGUAGE_KS_ID                      (187)                       /* Kashmiri */
#define LNG_LANGUAGE_KU_ID                      (188)                       /* Kurdish */
#define LNG_LANGUAGE_KV_ID                      (189)                       /* Komi */
#define LNG_LANGUAGE_KW_ID                      (190)                       /* Cornish */
#define LNG_LANGUAGE_KY_ID                      (191)                       /* Kirghiz */
#define LNG_LANGUAGE_LA_ID                      (192)                       /* Latin */
#define LNG_LANGUAGE_LB_ID                      (193)                       /* Letzeburgesch; Luxembourgish */
#define LNG_LANGUAGE_LG_ID                      (194)                       /* Ganda */
#define LNG_LANGUAGE_LI_ID                      (195)                       /* Limburgan; Limburger; Limburgish */
#define LNG_LANGUAGE_LN_ID                      (196)                       /* Lingala */
#define LNG_LANGUAGE_LO_ID                      (197)                       /* Lao */
#define LNG_LANGUAGE_LT_ID                      (198)                       /* Lithuanian */
#define LNG_LANGUAGE_LT_LT_ID                   (19800)                     /* Lithuanian, as spoken in Lithuania */
#define LNG_LANGUAGE_LU_ID                      (199)                       /* Luba-Katanga */
#define LNG_LANGUAGE_LV_ID                      (200)                       /* Latvian */
#define LNG_LANGUAGE_LV_LV_ID                   (20000)                     /* Latvian, as spoken in Latvia */
#define LNG_LANGUAGE_MG_ID                      (201)                       /* Malagasy */
#define LNG_LANGUAGE_MH_ID                      (202)                       /* Marshallese */
#define LNG_LANGUAGE_MI_ID                      (203)                       /* Maori */
#define LNG_LANGUAGE_MK_ID                      (204)                       /* Macedonian */
#define LNG_LANGUAGE_ML_ID                      (205)                       /* Malayalam */
#define LNG_LANGUAGE_MN_ID                      (206)                       /* Mongolian */
#define LNG_LANGUAGE_MO_ID                      (207)                       /* Moldavian */
#define LNG_LANGUAGE_MR_ID                      (208)                       /* Marathi */
#define LNG_LANGUAGE_MS_ID                      (209)                       /* Malay */
#define LNG_LANGUAGE_MT_ID                      (210)                       /* Maltese */
#define LNG_LANGUAGE_MY_ID                      (211)                       /* Burmese */
#define LNG_LANGUAGE_NA_ID                      (212)                       /* Nauru */
#define LNG_LANGUAGE_NB_ID                      (213)                       /* Bokmal, Norwegian */
#define LNG_LANGUAGE_ND_ID                      (214)                       /* North Ndebele */
#define LNG_LANGUAGE_NE_ID                      (215)                       /* Nepali */
#define LNG_LANGUAGE_NG_ID                      (216)                       /* Ndonga */
#define LNG_LANGUAGE_NL_ID                      (217)                       /* Dutch; Flemish */
#define LNG_LANGUAGE_NL_BE_ID                   (21700)                     /* Dutch, as spoken in Belgium */
#define LNG_LANGUAGE_NL_NL_ID                   (21701)                     /* Dutch, as spoken in Netherlands */
#define LNG_LANGUAGE_NN_ID                      (218)                       /* Norwegian Nynorsk; */
#define LNG_LANGUAGE_NO_ID                      (219)                       /* Norwegian */
#define LNG_LANGUAGE_NO_NO_ID                   (219)                       /* Norwegian, as spoken in Norway */
#define LNG_LANGUAGE_NO_NY_ID                   (219)                       /* Norwegian, as spoken in NY ? */
#define LNG_LANGUAGE_NR_ID                      (220)                       /* South Ndebele */
#define LNG_LANGUAGE_NV_ID                      (221)                       /* Navajo; Navaho */
#define LNG_LANGUAGE_NY_ID                      (222)                       /* Chewa; Chichewa; Nyanja */
#define LNG_LANGUAGE_OC_ID                      (223)                       /* Provencal; Occitan (post 1500) */
#define LNG_LANGUAGE_OJ_ID                      (224)                       /* Ojibwa */
#define LNG_LANGUAGE_OM_ID                      (225)                       /* Oromo */
#define LNG_LANGUAGE_OR_ID                      (226)                       /* Oriya */
#define LNG_LANGUAGE_OS_ID                      (227)                       /* Ossetian; Ossetic */
#define LNG_LANGUAGE_PA_ID                      (228)                       /* Panjabi; Punjabi */
#define LNG_LANGUAGE_PI_ID                      (229)                       /* Pali */
#define LNG_LANGUAGE_PL_ID                      (230)                       /* Polish */
#define LNG_LANGUAGE_PL_PL_ID                   (23000)                     /* Polish, as spoken in Poland */
#define LNG_LANGUAGE_PS_ID                      (231)                       /* Pushto */
#define LNG_LANGUAGE_PT_ID                      (232)                       /* Portuguese */
#define LNG_LANGUAGE_PT_BR_ID                   (23200)                     /* Portuguese, as spoken in Brazil */
#define LNG_LANGUAGE_PT_PT_ID                   (23201)                     /* Portuguese, as spoken in Portugal */
#define LNG_LANGUAGE_QU_ID                      (233)                       /* Quechua */
#define LNG_LANGUAGE_RM_ID                      (234)                       /* Raeto-Romance */
#define LNG_LANGUAGE_RN_ID                      (235)                       /* Rundi */
#define LNG_LANGUAGE_RO_ID                      (236)                       /* Romanian */
#define LNG_LANGUAGE_RO_RO_ID                   (23600)                     /* Romanian, as spoken in Romania */
#define LNG_LANGUAGE_RU_ID                      (237)                       /* Russian */
#define LNG_LANGUAGE_RU_RU_ID                   (23700)                     /* Russian, as spoken in Russia */
#define LNG_LANGUAGE_RW_ID                      (238)                       /* Kinyarwanda */
#define LNG_LANGUAGE_SA_ID                      (239)                       /* Sanskrit */
#define LNG_LANGUAGE_SC_ID                      (240)                       /* Sardinian */
#define LNG_LANGUAGE_SD_ID                      (241)                       /* Sindhi */
#define LNG_LANGUAGE_SE_ID                      (242)                       /* Northern Sami */
#define LNG_LANGUAGE_SG_ID                      (243)                       /* Sango */
#define LNG_LANGUAGE_SI_ID                      (244)                       /* Sinhalese; Sinhala */
#define LNG_LANGUAGE_SK_ID                      (245)                       /* Slovak */
#define LNG_LANGUAGE_SK_SK_ID                   (24500)                     /* Slovak, as spoken in Slovakia */
#define LNG_LANGUAGE_SL_ID                      (246)                       /* Slovenian */
#define LNG_LANGUAGE_SL_SI_ID                   (24600)                     /* Slovenian, as spoken in Slovenia */
#define LNG_LANGUAGE_SM_ID                      (247)                       /* Samoan */
#define LNG_LANGUAGE_SN_ID                      (248)                       /* Shona */
#define LNG_LANGUAGE_SO_ID                      (249)                       /* Somali */
#define LNG_LANGUAGE_SQ_ID                      (250)                       /* Albanian */
#define LNG_LANGUAGE_SR_ID                      (251)                       /* Serbian */
#define LNG_LANGUAGE_SR_SR_ID                   (25100)                     /* Serbian, as spoken in Serbia */
#define LNG_LANGUAGE_SS_ID                      (252)                       /* Swati */
#define LNG_LANGUAGE_ST_ID                      (253)                       /* Sotho, Southern */
#define LNG_LANGUAGE_SU_ID                      (254)                       /* Sundanese */
#define LNG_LANGUAGE_SV_ID                      (255)                       /* Swedish */
#define LNG_LANGUAGE_SV_SE_ID                   (25500)                     /* Swedish, as spoken in Sweden */
#define LNG_LANGUAGE_SW_ID                      (256)                       /* Swahili */
#define LNG_LANGUAGE_TA_ID                      (257)                       /* Tamil */
#define LNG_LANGUAGE_TA_IN_ID                   (25700)                     /* Tamil, as spoken in India */
#define LNG_LANGUAGE_TE_ID                      (258)                       /* Telugu */
#define LNG_LANGUAGE_TG_ID                      (259)                       /* Tajik */
#define LNG_LANGUAGE_TH_ID                      (260)                       /* Thai */
#define LNG_LANGUAGE_TI_ID                      (261)                       /* Tigrinya */
#define LNG_LANGUAGE_TK_ID                      (262)                       /* Turkmen */
#define LNG_LANGUAGE_TL_ID                      (263)                       /* Tagalog */
#define LNG_LANGUAGE_TN_ID                      (264)                       /* Tswana */
#define LNG_LANGUAGE_TO_ID                      (265)                       /* Tonga (Tonga Islands) */
#define LNG_LANGUAGE_TR_ID                      (266)                       /* Turkish */
#define LNG_LANGUAGE_TR_TR_ID                   (26600)                     /* Turkish, as spoken in Turkey */
#define LNG_LANGUAGE_TS_ID                      (267)                       /* Tsonga */
#define LNG_LANGUAGE_TT_ID                      (268)                       /* Tatar */
#define LNG_LANGUAGE_TW_ID                      (269)                       /* Twi */
#define LNG_LANGUAGE_TY_ID                      (270)                       /* Tahitian */
#define LNG_LANGUAGE_UG_ID                      (271)                       /* Uighur; Uyghur */
#define LNG_LANGUAGE_UK_ID                      (272)                       /* Ukrainian */
#define LNG_LANGUAGE_UK_UA_ID                   (27200)                     /* Ukrainian, as spoken in Ukraine */
#define LNG_LANGUAGE_UN_ID                      (273)                       /* Unknown */
#define LNG_LANGUAGE_UR_ID                      (274)                       /* Urdu */
#define LNG_LANGUAGE_UZ_ID                      (275)                       /* Uzbek */
#define LNG_LANGUAGE_VE_ID                      (276)                       /* Venda */
#define LNG_LANGUAGE_VI_ID                      (277)                       /* Vietnamese */
#define LNG_LANGUAGE_VI_VN_ID                   (27700)                     /* Vietnamese, as spoken in Vietnam */
#define LNG_LANGUAGE_VO_ID                      (278)                       /* Volapuk */
#define LNG_LANGUAGE_WA_ID                      (279)                       /* Walloon */
#define LNG_LANGUAGE_WO_ID                      (280)                       /* Wolof */
#define LNG_LANGUAGE_XH_ID                      (281)                       /* Xhosa */
#define LNG_LANGUAGE_YI_ID                      (282)                       /* Yiddish */
#define LNG_LANGUAGE_YO_ID                      (283)                       /* Yoruba */
#define LNG_LANGUAGE_ZA_ID                      (284)                       /* Chuang; Zhuang */
#define LNG_LANGUAGE_ZH_ID                      (285)                       /* Chinese */
#define LNG_LANGUAGE_ZH_SC_ID                   (28500)                     /* Chinese; simplified */
#define LNG_LANGUAGE_ZH_TC_ID                   (28501)                     /* Chinese; traditional */
#define LNG_LANGUAGE_ZH_CN_ID                   (28502)                     /* Chinese, as spoken in China */
#define LNG_LANGUAGE_ZH_HK_ID                   (28503)                     /* Chinese, as spoken in Hong Kong */
#define LNG_LANGUAGE_ZH_TW_ID                   (28504)                     /* Chinese, as spoken in Taiwan */
#define LNG_LANGUAGE_ZU_ID                      (286)                       /* Zulu */

#define    LNG_LANGUAGE_CODE_LENGTH             (5)


/*---------------------------------------------------------------------------*/


/*
** Public function prototypes
*/

int iLngCheckLanguageID (unsigned int uiLanguageID);
int iLngCheckLanguageCode (unsigned char *pucLanguageCode);
int iLngGetLanguageIDFromCode (unsigned char *pucLanguageCode, unsigned int *puiLanguageID);
int iLngGetLanguageCodeFromID (unsigned int uiLanguageID, unsigned char *pucLanguageCode, 
        unsigned int uiLanguageCodeLength);

int iLngGetCanonicalLanguageID (unsigned int uiLanguageID, unsigned int *puiCanonicalLanguageID);
int iLngIsLanguageIDCanonical (unsigned int uiLanguageID, boolean *pbCanonical);


int iLngCheckCharacterSetID (unsigned int uiCharacterSetID);
int iLngCheckCharacterSetName (unsigned char *pucCharacterSetName);
int iLngGetCharacterSetIDFromName (unsigned char *pucCharacterSetName, unsigned int *puiCharacterSetID);
int iLngGetCharacterSetNameFromID (unsigned int uiCharacterSetID, unsigned char *pucCharacterSetName, 
        unsigned int uiCharacterSetNameLength);


int iLngCheckTokenizerID (unsigned int uiTokenizerID);
int iLngCheckTokenizerName (unsigned char *pucTokenizerName);
int iLngGetTokenizerIDFromName (unsigned char *pucTokenizerName, unsigned int *puiTokenizerID);
int iLngGetTokenizerNameFromID (unsigned int uiTokenizerID, unsigned char *pucTokenizerName, 
        unsigned int uiTokenizerNameLength);


int iLngCheckStemmerID (unsigned int uiStemmerID);
int iLngCheckStemmerName (unsigned char *pucStemmerName);
int iLngGetStemmerIDFromName (unsigned char *pucStemmerName, unsigned int *puiStemmerID);
int iLngGetStemmerNameFromID (unsigned int uiStemmerID, unsigned char *pucStemmerName, 
        unsigned int uiStemmerNameLength);


int iLngCheckSoundexID (unsigned int uiSoundexID);
int iLngCheckSoundexName (unsigned char *pucSoundexName);
int iLngGetSoundexIDFromName (unsigned char *pucSoundexName, unsigned int *puiSoundexID);
int iLngGetSoundexNameFromID (unsigned int uiSoundexID, unsigned char *pucSoundexName, 
        unsigned int uiSoundexNameLen);


int iLngCheckMetaphoneID (unsigned int uiMetaphoneID);
int iLngCheckMetaphoneName (unsigned char *pucMetaphoneName);
int iLngGetMetaphoneIDFromName (unsigned char *pucMetaphoneName, unsigned int *puiMetaphoneID);
int iLngGetMetaphoneNameFromID (unsigned int uiMetaphoneID, unsigned char *pucMetaphoneName, 
        unsigned int uiMetaphoneNameLen);


int iLngCheckPhonixID (unsigned int uiPhonixID);
int iLngCheckPhonixName (unsigned char *pucPhonixName);
int iLngGetPhonixIDFromName (unsigned char *pucPhonixName, unsigned int *puiPhonixID);
int iLngGetPhonixNameFromID (unsigned int uiPhonixID, unsigned char *pucPhonixName, 
        unsigned int uiPhonixNameLen);


int iLngCheckTypoID (unsigned int uiTypoID);
int iLngCheckTypoName (unsigned char *pucTypoName);
int iLngGetTypoIDFromName (unsigned char *pucTypoName, unsigned int *puiTypoID);
int iLngGetTypoNameFromID (unsigned int uiTypoID, unsigned char *pucTypoName, 
        unsigned int uiTypoNameLen);


int iLngCheckStopListID (unsigned int uiStopListID);
int iLngCheckStopListName (unsigned char *pucStopListName);
int iLngGetStopListIDFromName (unsigned char *pucStopListName, unsigned int *puiStopListID);
int iLngGetStopListNameFromID (unsigned int uiStopListID, unsigned char *pucStopListName, 
        unsigned int uiStopListNameLength);


/*---------------------------------------------------------------------------*/


/* 
** C++ wrapper
*/

#if defined(__cplusplus)
}
#endif    /* defined(__cplusplus) */


/*---------------------------------------------------------------------------*/


#endif    /* !defined(LNG_LANGUAGE_H) */


/*---------------------------------------------------------------------------*/
