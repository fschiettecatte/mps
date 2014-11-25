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

    Module:     language.c

    Author:     Francois Schiettecatte (FS Consulting LLC.)

    Created:    30 April 2004

    Purpose:    This file contains various language support function

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
#define UTL_LOG_CONTEXT                     (unsigned char *)"com.fsconsult.mps.src.language.language"


/*---------------------------------------------------------------------------*/


/*
** Defines
*/

/* Minimum and maximum canonical language IDs */
#define LNG_LANGUAGE_MIN_CANONICAL_ID       LNG_LANGUAGE_AA_ID
#define LNG_LANGUAGE_MAX_CANONICAL_ID       LNG_LANGUAGE_ZU_ID

/* Minimum and maximum regional language IDs */
#define LNG_LANGUAGE_MIN_REGIONAL_ID        (LNG_LANGUAGE_AA_ID * 100)
#define LNG_LANGUAGE_MAX_REGIONAL_ID        (LNG_LANGUAGE_ZU_ID * 100)

/*---------------------------------------------------------------------------*/


/*
** Structures
*/

/* Language dictionary structure */
struct lngDictionary {
    unsigned char   *pucName;               /* Name */
    unsigned int    uiID;                   /* ID */
};


/*---------------------------------------------------------------------------*/


/*
** Private function prototypes
*/

static int iLngGetIDForNameFromDictionary (struct lngDictionary *pldLngDictionary,
        unsigned char *pucName, unsigned int *puiID);

static int iLngGetNameForIDFromDictionary (struct lngDictionary *pldLngDictionary, 
        unsigned int uiID, unsigned char *pucName, unsigned int uiNameLength);


/*---------------------------------------------------------------------------*/


/*
** Globals
*/

/* Character set name/ID dictionary */
static struct lngDictionary pldLngCharacterSetDictionaryListGlobal[] = 
{
    {   LNG_CHARACTER_SET_UTF_8_NAME,
        LNG_CHARACTER_SET_UTF_8_ID,
    },
    {   LNG_CHARACTER_SET_WCHAR_NAME,
        LNG_CHARACTER_SET_WCHAR_ID,
    },
    /* Terminator */
    {   NULL,
        0,
    },
};


/* Language code/ID dictionary */
static struct lngDictionary pldLngLanguageDictionaryListGlobal[] = 
{
    {   LNG_LANGUAGE_ANY_CODE,
        LNG_LANGUAGE_ANY_ID,
    },
    {   LNG_LANGUAGE_AA_CODE,
        LNG_LANGUAGE_AA_ID,
    },
    {   LNG_LANGUAGE_AB_CODE,
        LNG_LANGUAGE_AB_ID,
    },
    {   LNG_LANGUAGE_AE_CODE,
        LNG_LANGUAGE_AE_ID,
    },
    {   LNG_LANGUAGE_AF_CODE,
        LNG_LANGUAGE_AF_ID,
    },
    {   LNG_LANGUAGE_AK_CODE,
        LNG_LANGUAGE_AK_ID,
    },
    {   LNG_LANGUAGE_AM_CODE,
        LNG_LANGUAGE_AM_ID,
    },
    {   LNG_LANGUAGE_AN_CODE,
        LNG_LANGUAGE_AN_ID,
    },
    {   LNG_LANGUAGE_AR_CODE,
        LNG_LANGUAGE_AR_ID,
    },
    {   LNG_LANGUAGE_AS_CODE,
        LNG_LANGUAGE_AS_ID,
    },
    {   LNG_LANGUAGE_AV_CODE,
        LNG_LANGUAGE_AV_ID,
    },
    {   LNG_LANGUAGE_AY_CODE,
        LNG_LANGUAGE_AY_ID,
    },
    {   LNG_LANGUAGE_AZ_CODE,
        LNG_LANGUAGE_AZ_ID,
    },
    {   LNG_LANGUAGE_BA_CODE,
        LNG_LANGUAGE_BA_ID,
    },
    {   LNG_LANGUAGE_BE_CODE,
        LNG_LANGUAGE_BE_ID,
    },
    {   LNG_LANGUAGE_BG_CODE,
        LNG_LANGUAGE_BG_ID,
    },
    {   LNG_LANGUAGE_BG_BG_CODE,
        LNG_LANGUAGE_BG_BG_ID,
    },
    {   LNG_LANGUAGE_BH_CODE,
        LNG_LANGUAGE_BH_ID,
    },
    {   LNG_LANGUAGE_BI_CODE,
        LNG_LANGUAGE_BI_ID,
    },
    {   LNG_LANGUAGE_BM_CODE,
        LNG_LANGUAGE_BM_ID,
    },
    {   LNG_LANGUAGE_BN_CODE,
        LNG_LANGUAGE_BN_ID,
    },
    {   LNG_LANGUAGE_BO_CODE,
        LNG_LANGUAGE_BO_ID,
    },
    {   LNG_LANGUAGE_BR_CODE,
        LNG_LANGUAGE_BR_ID,
    },
    {   LNG_LANGUAGE_BS_CODE,
        LNG_LANGUAGE_BS_ID,
    },
    {   LNG_LANGUAGE_CA_CODE,
        LNG_LANGUAGE_CA_ID,
    },
    {   LNG_LANGUAGE_CA_ES_CODE,
        LNG_LANGUAGE_CA_ES_ID,
    },
    {   LNG_LANGUAGE_CE_CODE,
        LNG_LANGUAGE_CE_ID,
    },
    {   LNG_LANGUAGE_CH_CODE,
        LNG_LANGUAGE_CH_ID,
    },
    {   LNG_LANGUAGE_CO_CODE,
        LNG_LANGUAGE_CO_ID,
    },
    {   LNG_LANGUAGE_CR_CODE,
        LNG_LANGUAGE_CR_ID,
    },
    {   LNG_LANGUAGE_CS_CODE,
        LNG_LANGUAGE_CS_ID,
    },
    {   LNG_LANGUAGE_CS_CZ_CODE,
        LNG_LANGUAGE_CS_CZ_ID,
    },
    {   LNG_LANGUAGE_CU_CODE,
        LNG_LANGUAGE_CU_ID,
    },
    {   LNG_LANGUAGE_CV_CODE,
        LNG_LANGUAGE_CV_ID,
    },
    {   LNG_LANGUAGE_CY_CODE,
        LNG_LANGUAGE_CY_ID,
    },
    {   LNG_LANGUAGE_DA_CODE,
        LNG_LANGUAGE_DA_ID,
    },
    {   LNG_LANGUAGE_DA_DK_CODE,
        LNG_LANGUAGE_DA_DK_ID,
    },
    {   LNG_LANGUAGE_DE_CODE,
        LNG_LANGUAGE_DE_ID,
    },
    {   LNG_LANGUAGE_DE_AT_CODE,
        LNG_LANGUAGE_DE_AT_ID,
    },
    {   LNG_LANGUAGE_DE_GE_CODE,
        LNG_LANGUAGE_DE_GE_ID,
    },
    {   LNG_LANGUAGE_DV_CODE,
        LNG_LANGUAGE_DV_ID,
    },
    {   LNG_LANGUAGE_DZ_CODE,
        LNG_LANGUAGE_DZ_ID,
    },
    {   LNG_LANGUAGE_EE_CODE,
        LNG_LANGUAGE_EE_ID,
    },
    {   LNG_LANGUAGE_EL_CODE,
        LNG_LANGUAGE_EL_ID,
    },
    {   LNG_LANGUAGE_EL_GR_CODE,
        LNG_LANGUAGE_EL_GR_ID,
    },
    {   LNG_LANGUAGE_EN_CODE,
        LNG_LANGUAGE_EN_ID,
    },
    {   LNG_LANGUAGE_EN_AU_CODE,
        LNG_LANGUAGE_EN_AU_ID,
    },
    {   LNG_LANGUAGE_EN_CA_CODE,
        LNG_LANGUAGE_EN_CA_ID,
    },
    {   LNG_LANGUAGE_EN_GB_CODE,
        LNG_LANGUAGE_EN_GB_ID,
    },
    {   LNG_LANGUAGE_EN_IE_CODE,
        LNG_LANGUAGE_EN_IE_ID,
    },
    {   LNG_LANGUAGE_EN_SE_CODE,
        LNG_LANGUAGE_EN_SE_ID,
    },
    {   LNG_LANGUAGE_EN_US_CODE,
        LNG_LANGUAGE_EN_US_ID,
    },
    {   LNG_LANGUAGE_EO_CODE,
        LNG_LANGUAGE_EO_ID,
    },
    {   LNG_LANGUAGE_ES_CODE,
        LNG_LANGUAGE_ES_ID,
    },
    {   LNG_LANGUAGE_ES_AR_CODE,
        LNG_LANGUAGE_ES_AR_ID,
    },
    {   LNG_LANGUAGE_ES_BO_CODE,
        LNG_LANGUAGE_ES_BO_ID,
    },
    {   LNG_LANGUAGE_ES_DO_CODE,
        LNG_LANGUAGE_ES_DO_ID,
    },
    {   LNG_LANGUAGE_ES_ES_CODE,
        LNG_LANGUAGE_ES_ES_ID,
    },
    {   LNG_LANGUAGE_ES_GT_CODE,
        LNG_LANGUAGE_ES_GT_ID,
    },
    {   LNG_LANGUAGE_ES_HN_CODE,
        LNG_LANGUAGE_ES_HN_ID,
    },
    {   LNG_LANGUAGE_ES_MX_CODE,
        LNG_LANGUAGE_ES_MX_ID,
    },
    {   LNG_LANGUAGE_ES_NI_CODE,
        LNG_LANGUAGE_ES_NI_ID,
    },
    {   LNG_LANGUAGE_ES_PA_CODE,
        LNG_LANGUAGE_ES_PA_ID,
    },
    {   LNG_LANGUAGE_ES_PE_CODE,
        LNG_LANGUAGE_ES_PE_ID,
    },
    {   LNG_LANGUAGE_ES_SV_CODE,
        LNG_LANGUAGE_ES_SV_ID,
    },
    {   LNG_LANGUAGE_ET_CODE,
        LNG_LANGUAGE_ET_ID,
    },
    {   LNG_LANGUAGE_ET_EE_CODE,
        LNG_LANGUAGE_ET_EE_ID,
    },
    {   LNG_LANGUAGE_EU_CODE,
        LNG_LANGUAGE_EU_ID,
    },
    {   LNG_LANGUAGE_EU_ES_CODE,
        LNG_LANGUAGE_EU_ES_ID,
    },
    {   LNG_LANGUAGE_FA_CODE,
        LNG_LANGUAGE_FA_ID,
    },
    {   LNG_LANGUAGE_FA_IR_CODE,
        LNG_LANGUAGE_FA_IR_ID,
    },
    {   LNG_LANGUAGE_FF_CODE,
        LNG_LANGUAGE_FF_ID,
    },
    {   LNG_LANGUAGE_FI_CODE,
        LNG_LANGUAGE_FI_ID,
    },
    {   LNG_LANGUAGE_FI_FI_CODE,
        LNG_LANGUAGE_FI_FI_ID,
    },
    {   LNG_LANGUAGE_FJ_CODE,
        LNG_LANGUAGE_FJ_ID,
    },
    {   LNG_LANGUAGE_FO_CODE,
        LNG_LANGUAGE_FO_ID,
    },
    {   LNG_LANGUAGE_FO_FO_CODE,
        LNG_LANGUAGE_FO_FO_ID,
    },
    {   LNG_LANGUAGE_FR_CODE,
        LNG_LANGUAGE_FR_ID,
    },
    {   LNG_LANGUAGE_FR_BE_CODE,
        LNG_LANGUAGE_FR_BE_ID,
    },
    {   LNG_LANGUAGE_FR_CA_CODE,
        LNG_LANGUAGE_FR_CA_ID,
    },
    {   LNG_LANGUAGE_FR_FR_CODE,
        LNG_LANGUAGE_FR_FR_ID,
    },
    {   LNG_LANGUAGE_FY_CODE,
        LNG_LANGUAGE_FY_ID,
    },
    {   LNG_LANGUAGE_GA_CODE,
        LNG_LANGUAGE_GA_ID,
    },
    {   LNG_LANGUAGE_GA_IE_CODE,
        LNG_LANGUAGE_GA_IE_ID,
    },
    {   LNG_LANGUAGE_GD_CODE,
        LNG_LANGUAGE_GD_ID,
    },
    {   LNG_LANGUAGE_GL_CODE,
        LNG_LANGUAGE_GL_ID,
    },
    {   LNG_LANGUAGE_GN_CODE,
        LNG_LANGUAGE_GN_ID,
    },
    {   LNG_LANGUAGE_GU_CODE,
        LNG_LANGUAGE_GU_ID,
    },
    {   LNG_LANGUAGE_GV_CODE,
        LNG_LANGUAGE_GV_ID,
    },
    {   LNG_LANGUAGE_HA_CODE,
        LNG_LANGUAGE_HA_ID,
    },
    {   LNG_LANGUAGE_HE_CODE,
        LNG_LANGUAGE_HE_ID,
    },
    {   LNG_LANGUAGE_HE_IL_CODE,
        LNG_LANGUAGE_HE_IL_ID,
    },
    {   LNG_LANGUAGE_HI_CODE,
        LNG_LANGUAGE_HI_ID,
    },
    {   LNG_LANGUAGE_HO_CODE,
        LNG_LANGUAGE_HO_ID,
    },
    {   LNG_LANGUAGE_HR_CODE,
        LNG_LANGUAGE_HR_ID,
    },
    {   LNG_LANGUAGE_HT_CODE,
        LNG_LANGUAGE_HT_ID,
    },
    {   LNG_LANGUAGE_HU_CODE,
        LNG_LANGUAGE_HU_ID,
    },
    {   LNG_LANGUAGE_HY_CODE,
        LNG_LANGUAGE_HY_ID,
    },
    {   LNG_LANGUAGE_HZ_CODE,
        LNG_LANGUAGE_HZ_ID,
    },
    {   LNG_LANGUAGE_IA_CODE,
        LNG_LANGUAGE_IA_ID,
    },
    {   LNG_LANGUAGE_ID_CODE,
        LNG_LANGUAGE_ID_ID,
    },
    {   LNG_LANGUAGE_ID_ID_CODE,
        LNG_LANGUAGE_ID_ID_ID,
    },
    {   LNG_LANGUAGE_IE_CODE,
        LNG_LANGUAGE_IE_ID,
    },
    {   LNG_LANGUAGE_IG_CODE,
        LNG_LANGUAGE_IG_ID,
    },
    {   LNG_LANGUAGE_II_CODE,
        LNG_LANGUAGE_II_ID,
    },
    {   LNG_LANGUAGE_IK_CODE,
        LNG_LANGUAGE_IK_ID,
    },
    {   LNG_LANGUAGE_IO_CODE,
        LNG_LANGUAGE_IO_ID,
    },
    {   LNG_LANGUAGE_IS_CODE,
        LNG_LANGUAGE_IS_ID,
    },
    {   LNG_LANGUAGE_IS_IS_CODE,
        LNG_LANGUAGE_IS_IS_ID,
    },
    {   LNG_LANGUAGE_IT_CODE,
        LNG_LANGUAGE_IT_ID,
    },
    {   LNG_LANGUAGE_IT_IT_CODE,
        LNG_LANGUAGE_IT_IT_ID,
    },
    {   LNG_LANGUAGE_IU_CODE,
        LNG_LANGUAGE_IU_ID,
    },
    {   LNG_LANGUAGE_JA_CODE,
        LNG_LANGUAGE_JA_ID,
    },
    {   LNG_LANGUAGE_JA_JP_CODE,
        LNG_LANGUAGE_JA_JP_ID,
    },
    {   LNG_LANGUAGE_JV_CODE,
        LNG_LANGUAGE_JV_ID,
    },
    {   LNG_LANGUAGE_KA_CODE,
        LNG_LANGUAGE_KA_ID,
    },
    {   LNG_LANGUAGE_KG_CODE,
        LNG_LANGUAGE_KG_ID,
    },
    {   LNG_LANGUAGE_KI_CODE,
        LNG_LANGUAGE_KI_ID,
    },
    {   LNG_LANGUAGE_KJ_CODE,
        LNG_LANGUAGE_KJ_ID,
    },
    {   LNG_LANGUAGE_KK_CODE,
        LNG_LANGUAGE_KK_ID,
    },
    {   LNG_LANGUAGE_KL_CODE,
        LNG_LANGUAGE_KL_ID,
    },
    {   LNG_LANGUAGE_KL_GL_CODE,
        LNG_LANGUAGE_KL_GL_ID,
    },
    {   LNG_LANGUAGE_KM_CODE,
        LNG_LANGUAGE_KM_ID,
    },
    {   LNG_LANGUAGE_KN_CODE,
        LNG_LANGUAGE_KN_ID,
    },
    {   LNG_LANGUAGE_KO_CODE,
        LNG_LANGUAGE_KO_ID,
    },
    {   LNG_LANGUAGE_KR_CODE,
        LNG_LANGUAGE_KR_ID,
    },
    {   LNG_LANGUAGE_KS_CODE,
        LNG_LANGUAGE_KS_ID,
    },
    {   LNG_LANGUAGE_KU_CODE,
        LNG_LANGUAGE_KU_ID,
    },
    {   LNG_LANGUAGE_KV_CODE,
        LNG_LANGUAGE_KV_ID,
    },
    {   LNG_LANGUAGE_KW_CODE,
        LNG_LANGUAGE_KW_ID,
    },
    {   LNG_LANGUAGE_KY_CODE,
        LNG_LANGUAGE_KY_ID,
    },
    {   LNG_LANGUAGE_LA_CODE,
        LNG_LANGUAGE_LA_ID,
    },
    {   LNG_LANGUAGE_LB_CODE,
        LNG_LANGUAGE_LB_ID,
    },
    {   LNG_LANGUAGE_LG_CODE,
        LNG_LANGUAGE_LG_ID,
    },
    {   LNG_LANGUAGE_LI_CODE,
        LNG_LANGUAGE_LI_ID,
    },
    {   LNG_LANGUAGE_LN_CODE,
        LNG_LANGUAGE_LN_ID,
    },
    {   LNG_LANGUAGE_LO_CODE,
        LNG_LANGUAGE_LO_ID,
    },
    {   LNG_LANGUAGE_LT_CODE,
        LNG_LANGUAGE_LT_ID,
    },
    {   LNG_LANGUAGE_LT_LT_CODE,
        LNG_LANGUAGE_LT_LT_ID,
    },
    {   LNG_LANGUAGE_LU_CODE,
        LNG_LANGUAGE_LU_ID,
    },
    {   LNG_LANGUAGE_LV_CODE,
        LNG_LANGUAGE_LV_ID,
    },
    {   LNG_LANGUAGE_LV_LV_CODE,
        LNG_LANGUAGE_LV_LV_ID,
    },
    {   LNG_LANGUAGE_MG_CODE,
        LNG_LANGUAGE_MG_ID,
    },
    {   LNG_LANGUAGE_MH_CODE,
        LNG_LANGUAGE_MH_ID,
    },
    {   LNG_LANGUAGE_MI_CODE,
        LNG_LANGUAGE_MI_ID,
    },
    {   LNG_LANGUAGE_MK_CODE,
        LNG_LANGUAGE_MK_ID,
    },
    {   LNG_LANGUAGE_ML_CODE,
        LNG_LANGUAGE_ML_ID,
    },
    {   LNG_LANGUAGE_MN_CODE,
        LNG_LANGUAGE_MN_ID,
    },
    {   LNG_LANGUAGE_MO_CODE,
        LNG_LANGUAGE_MO_ID,
    },
    {   LNG_LANGUAGE_MR_CODE,
        LNG_LANGUAGE_MR_ID,
    },
    {   LNG_LANGUAGE_MS_CODE,
        LNG_LANGUAGE_MS_ID,
    },
    {   LNG_LANGUAGE_MT_CODE,
        LNG_LANGUAGE_MT_ID,
    },
    {   LNG_LANGUAGE_MY_CODE,
        LNG_LANGUAGE_MY_ID,
    },
    {   LNG_LANGUAGE_NA_CODE,
        LNG_LANGUAGE_NA_ID,
    },
    {   LNG_LANGUAGE_NB_CODE,
        LNG_LANGUAGE_NB_ID,
    },
    {   LNG_LANGUAGE_ND_CODE,
        LNG_LANGUAGE_ND_ID,
    },
    {   LNG_LANGUAGE_NE_CODE,
        LNG_LANGUAGE_NE_ID,
    },
    {   LNG_LANGUAGE_NG_CODE,
        LNG_LANGUAGE_NG_ID,
    },
    {   LNG_LANGUAGE_NL_CODE,
        LNG_LANGUAGE_NL_ID,
    },
    {   LNG_LANGUAGE_NL_BE_CODE,
        LNG_LANGUAGE_NL_BE_ID,
    },
    {   LNG_LANGUAGE_NL_NL_CODE,
        LNG_LANGUAGE_NL_NL_ID,
    },
    {   LNG_LANGUAGE_NN_CODE,
        LNG_LANGUAGE_NN_ID,
    },
    {   LNG_LANGUAGE_NO_CODE,
        LNG_LANGUAGE_NO_ID,
    },
    {   LNG_LANGUAGE_NO_NO_CODE,
        LNG_LANGUAGE_NO_NO_ID,
    },
    {   LNG_LANGUAGE_NO_NY_CODE,
        LNG_LANGUAGE_NO_NY_ID,
    },
    {   LNG_LANGUAGE_NR_CODE,
        LNG_LANGUAGE_NR_ID,
    },
    {   LNG_LANGUAGE_NV_CODE,
        LNG_LANGUAGE_NV_ID,
    },
    {   LNG_LANGUAGE_NY_CODE,
        LNG_LANGUAGE_NY_ID,
    },
    {   LNG_LANGUAGE_OC_CODE,
        LNG_LANGUAGE_OC_ID,
    },
    {   LNG_LANGUAGE_OJ_CODE,
        LNG_LANGUAGE_OJ_ID,
    },
    {   LNG_LANGUAGE_OM_CODE,
        LNG_LANGUAGE_OM_ID,
    },
    {   LNG_LANGUAGE_OR_CODE,
        LNG_LANGUAGE_OR_ID,
    },
    {   LNG_LANGUAGE_OS_CODE,
        LNG_LANGUAGE_OS_ID,
    },
    {   LNG_LANGUAGE_PA_CODE,
        LNG_LANGUAGE_PA_ID,
    },
    {   LNG_LANGUAGE_PI_CODE,
        LNG_LANGUAGE_PI_ID,
    },
    {   LNG_LANGUAGE_PL_CODE,
        LNG_LANGUAGE_PL_ID,
    },
    {   LNG_LANGUAGE_PL_PL_CODE,
        LNG_LANGUAGE_PL_PL_ID,
    },
    {   LNG_LANGUAGE_PS_CODE,
        LNG_LANGUAGE_PS_ID,
    },
    {   LNG_LANGUAGE_PT_CODE,
        LNG_LANGUAGE_PT_ID,
    },
    {   LNG_LANGUAGE_PT_BR_CODE,
        LNG_LANGUAGE_PT_BR_ID,
    },
    {   LNG_LANGUAGE_PT_PT_CODE,
        LNG_LANGUAGE_PT_PT_ID,
    },
    {   LNG_LANGUAGE_QU_CODE,
        LNG_LANGUAGE_QU_ID,
    },
    {   LNG_LANGUAGE_RM_CODE,
        LNG_LANGUAGE_RM_ID,
    },
    {   LNG_LANGUAGE_RN_CODE,
        LNG_LANGUAGE_RN_ID,
    },
    {   LNG_LANGUAGE_RO_CODE,
        LNG_LANGUAGE_RO_ID,
    },
    {   LNG_LANGUAGE_RO_RO_CODE,
        LNG_LANGUAGE_RO_RO_ID,
    },
    {   LNG_LANGUAGE_RU_CODE,
        LNG_LANGUAGE_RU_ID,
    },
    {   LNG_LANGUAGE_RU_RU_CODE,
        LNG_LANGUAGE_RU_RU_ID,
    },
    {   LNG_LANGUAGE_RW_CODE,
        LNG_LANGUAGE_RW_ID,
    },
    {   LNG_LANGUAGE_SA_CODE,
        LNG_LANGUAGE_SA_ID,
    },
    {   LNG_LANGUAGE_SC_CODE,
        LNG_LANGUAGE_SC_ID,
    },
    {   LNG_LANGUAGE_SD_CODE,
        LNG_LANGUAGE_SD_ID,
    },
    {   LNG_LANGUAGE_SE_CODE,
        LNG_LANGUAGE_SE_ID,
    },
    {   LNG_LANGUAGE_SG_CODE,
        LNG_LANGUAGE_SG_ID,
    },
    {   LNG_LANGUAGE_SI_CODE,
        LNG_LANGUAGE_SI_ID,
    },
    {   LNG_LANGUAGE_SK_CODE,
        LNG_LANGUAGE_SK_ID,
    },
    {   LNG_LANGUAGE_SK_SK_CODE,
        LNG_LANGUAGE_SK_SK_ID,
    },
    {   LNG_LANGUAGE_SL_CODE,
        LNG_LANGUAGE_SL_ID,
    },
    {   LNG_LANGUAGE_SL_SI_CODE,
        LNG_LANGUAGE_SL_SI_ID,
    },
    {   LNG_LANGUAGE_SM_CODE,
        LNG_LANGUAGE_SM_ID,
    },
    {   LNG_LANGUAGE_SN_CODE,
        LNG_LANGUAGE_SN_ID,
    },
    {   LNG_LANGUAGE_SO_CODE,
        LNG_LANGUAGE_SO_ID,
    },
    {   LNG_LANGUAGE_SQ_CODE,
        LNG_LANGUAGE_SQ_ID,
    },
    {   LNG_LANGUAGE_SR_CODE,
        LNG_LANGUAGE_SR_ID,
    },
    {   LNG_LANGUAGE_SR_SR_CODE,
        LNG_LANGUAGE_SR_SR_ID,
    },
    {   LNG_LANGUAGE_SS_CODE,
        LNG_LANGUAGE_SS_ID,
    },
    {   LNG_LANGUAGE_ST_CODE,
        LNG_LANGUAGE_ST_ID,
    },
    {   LNG_LANGUAGE_SU_CODE,
        LNG_LANGUAGE_SU_ID,
    },
    {   LNG_LANGUAGE_SV_CODE,
        LNG_LANGUAGE_SV_ID,
    },
    {   LNG_LANGUAGE_SV_SE_CODE,
        LNG_LANGUAGE_SV_SE_ID,
    },
    {   LNG_LANGUAGE_SW_CODE,
        LNG_LANGUAGE_SW_ID,
    },
    {   LNG_LANGUAGE_TA_CODE,
        LNG_LANGUAGE_TA_ID,
    },
    {   LNG_LANGUAGE_TA_IN_CODE,
        LNG_LANGUAGE_TA_IN_ID,
    },
    {   LNG_LANGUAGE_TE_CODE,
        LNG_LANGUAGE_TE_ID,
    },
    {   LNG_LANGUAGE_TG_CODE,
        LNG_LANGUAGE_TG_ID,
    },
    {   LNG_LANGUAGE_TH_CODE,
        LNG_LANGUAGE_TH_ID,
    },
    {   LNG_LANGUAGE_TI_CODE,
        LNG_LANGUAGE_TI_ID,
    },
    {   LNG_LANGUAGE_TK_CODE,
        LNG_LANGUAGE_TK_ID,
    },
    {   LNG_LANGUAGE_TL_CODE,
        LNG_LANGUAGE_TL_ID,
    },
    {   LNG_LANGUAGE_TN_CODE,
        LNG_LANGUAGE_TN_ID,
    },
    {   LNG_LANGUAGE_TO_CODE,
        LNG_LANGUAGE_TO_ID,
    },
    {   LNG_LANGUAGE_TR_CODE,
        LNG_LANGUAGE_TR_ID,
    },
    {   LNG_LANGUAGE_TR_TR_CODE,
        LNG_LANGUAGE_TR_TR_ID,
    },
    {   LNG_LANGUAGE_TS_CODE,
        LNG_LANGUAGE_TS_ID,
    },
    {   LNG_LANGUAGE_TT_CODE,
        LNG_LANGUAGE_TT_ID,
    },
    {   LNG_LANGUAGE_TW_CODE,
        LNG_LANGUAGE_TW_ID,
    },
    {   LNG_LANGUAGE_TY_CODE,
        LNG_LANGUAGE_TY_ID,
    },
    {   LNG_LANGUAGE_UG_CODE,
        LNG_LANGUAGE_UG_ID,
    },
    {   LNG_LANGUAGE_UK_CODE,
        LNG_LANGUAGE_UK_ID,
    },
    {   LNG_LANGUAGE_UK_UA_CODE,
        LNG_LANGUAGE_UK_UA_ID,
    },
    {   LNG_LANGUAGE_UN_CODE,
        LNG_LANGUAGE_UN_ID,
    },
    {   LNG_LANGUAGE_UR_CODE,
        LNG_LANGUAGE_UR_ID,
    },
    {   LNG_LANGUAGE_UZ_CODE,
        LNG_LANGUAGE_UZ_ID,
    },
    {   LNG_LANGUAGE_VE_CODE,
        LNG_LANGUAGE_VE_ID,
    },
    {   LNG_LANGUAGE_VI_CODE,
        LNG_LANGUAGE_VI_ID,
    },
    {   LNG_LANGUAGE_VI_VN_CODE,
        LNG_LANGUAGE_VI_VN_ID,
    },
    {   LNG_LANGUAGE_VO_CODE,
        LNG_LANGUAGE_VO_ID,
    },
    {   LNG_LANGUAGE_WA_CODE,
        LNG_LANGUAGE_WA_ID,
    },
    {   LNG_LANGUAGE_WO_CODE,
        LNG_LANGUAGE_WO_ID,
    },
    {   LNG_LANGUAGE_XH_CODE,
        LNG_LANGUAGE_XH_ID,
    },
    {   LNG_LANGUAGE_YI_CODE,
        LNG_LANGUAGE_YI_ID,
    },
    {   LNG_LANGUAGE_YO_CODE,
        LNG_LANGUAGE_YO_ID,
    },
    {   LNG_LANGUAGE_ZA_CODE,
        LNG_LANGUAGE_ZA_ID,
    },
    {   LNG_LANGUAGE_ZH_CODE,
        LNG_LANGUAGE_ZH_ID,
    },
    {   LNG_LANGUAGE_ZH_SC_CODE,
        LNG_LANGUAGE_ZH_SC_ID,
    },
    {   LNG_LANGUAGE_ZH_TC_CODE,
        LNG_LANGUAGE_ZH_TC_ID,
    },
    {   LNG_LANGUAGE_ZH_CN_CODE,
        LNG_LANGUAGE_ZH_CN_ID,
    },
    {   LNG_LANGUAGE_ZH_HK_CODE,
        LNG_LANGUAGE_ZH_HK_ID,
    },
    {   LNG_LANGUAGE_ZH_TW_CODE,
        LNG_LANGUAGE_ZH_TW_ID,
    },
    {   LNG_LANGUAGE_ZU_CODE,
        LNG_LANGUAGE_ZU_ID,
    },
    /* Terminator */
    {   NULL,
        0,
    },
};


/* Tokenizer name/ID dictionary */
static struct lngDictionary pldLngTokenizerDictionaryListGlobal[] = 
{
    {   LNG_TOKENIZER_FSCLT_1_NAME,
        LNG_TOKENIZER_FSCLT_1_ID,
    },
    {   LNG_TOKENIZER_FSCLT_2_NAME,
        LNG_TOKENIZER_FSCLT_2_ID,
    },
    /* Terminator */
    {   NULL,
        0,
    },
};


/* Stemmer name/ID dictionary */
static struct lngDictionary pldLngStemmerDictionaryListGlobal[] = 
{
    {   LNG_STEMMER_NONE_NAME,
        LNG_STEMMER_NONE_ID,
    },
    {   LNG_STEMMER_PLURAL_NAME,
        LNG_STEMMER_PLURAL_ID,
    },
    {   LNG_STEMMER_PORTER_NAME,
        LNG_STEMMER_PORTER_ID,
    },
    {   LNG_STEMMER_LOVINS_NAME,
        LNG_STEMMER_LOVINS_ID,
    },
    /* Terminator */
    {   NULL,
        0,
    },
};


/* Soundex name/ID dictionary */
static struct lngDictionary pldLngSoundexDictionaryListGlobal[] = 
{
    {   LNG_SOUNDEX_STANDARD_NAME,
        LNG_SOUNDEX_STANDARD_ID,
    },
    {   LNG_SOUNDEX_ALTERNATIVE_NAME,
        LNG_SOUNDEX_ALTERNATIVE_ID,
    },
    /* Terminator */
    {   NULL,
        0,
    },
};


/* Metaphone name/ID dictionary */
static struct lngDictionary pldLngMetaphoneDictionaryListGlobal[] = 
{
    {   LNG_METAPHONE_STANDARD_NAME,
        LNG_METAPHONE_STANDARD_ID,
    },
    /* Terminator */
    {   NULL,
        0,
    },
};


/* Phonix name/ID dictionary */
static struct lngDictionary pldLngPhonixDictionaryListGlobal[] = 
{
    {   LNG_PHONIX_STANDARD_NAME,
        LNG_PHONIX_STANDARD_ID,
    },
    /* Terminator */
    {   NULL,
        0,
    },
};


/* Typo name/ID dictionary */
static struct lngDictionary pldLngTypoDictionaryListGlobal[] = 
{
    {   LNG_TYPO_STANDARD_NAME,
        LNG_TYPO_STANDARD_ID,
    },
    /* Terminator */
    {   NULL,
        0,
    },
};


/* Stop list name/ID dictionary */
static struct lngDictionary pldLngStopListDictionaryListGlobal[] = 
{
    {   LNG_STOP_LIST_NONE_NAME,
        LNG_STOP_LIST_NONE_ID,
    },
    {   LNG_STOP_LIST_GOOGLE_NAME,
        LNG_STOP_LIST_GOOGLE_ID,
    },
    {   LNG_STOP_LIST_GOOGLE_MODIFIED_NAME,
        LNG_STOP_LIST_GOOGLE_MODIFIED_ID,
    },
    /* Terminator */
    {   NULL,
        0,
    },
};


/*---------------------------------------------------------------------------*/


/*

    Function:   iLngCheckLanguageID()

    Purpose:    This function checks the passed language ID

    Parameters: uiLanguageID        Language ID

    Globals:    none

    Returns:    An LNG error code

*/
int iLngCheckLanguageID
(
    unsigned int uiLanguageID
)
{

    unsigned char   pucLanguageCode[LNG_LANGUAGE_CODE_LENGTH + 1] = {'\0'};


    /* Check the parameters */
    if ( uiLanguageID < 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiLanguageID' parameter passed to 'iLngCheckLanguageID'."); 
        return (LNG_LanguageInvalidLanguageID);
    }


    /* Check the language ID */
    return (iLngGetLanguageCodeFromID(uiLanguageID, pucLanguageCode, LNG_LANGUAGE_CODE_LENGTH + 1));

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLngCheckLanguageCode()

    Purpose:    This function checks the passed language code

    Parameters: pucLanguageCode     Language code

    Globals:    none

    Returns:    An LNG error code

*/
int iLngCheckLanguageCode
(
    unsigned char *pucLanguageCode
)
{

    unsigned int    uiLanguageID = LNG_LANGUAGE_ANY_ID;


    /* Check the parameters */
    if ( bUtlStringsIsStringNULL(pucLanguageCode) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pucLanguageCode' parameter passed to 'iLngCheckLanguageCode'."); 
        return (LNG_LanguageInvalidLanguageCode);
    }


    /* Check the language code */
    return (iLngGetLanguageIDFromCode(pucLanguageCode, &uiLanguageID));

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLngGetLanguageIDFromCode()

    Purpose:    This function gets the language ID for the language code

    Parameters: pucLanguageCode     Language code
                puiLanguageID       Return pointer for the language ID

    Globals:    pldLngLanguageDictionaryListGlobal

    Returns:    An LNG error code

*/
int iLngGetLanguageIDFromCode
(
    unsigned char *pucLanguageCode,
    unsigned int *puiLanguageID
)
{

    int     iError = LNG_NoError;


    /* Check the parameters */
    if ( bUtlStringsIsStringNULL(pucLanguageCode) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pucLanguageCode' parameter passed to 'iLngGetLanguageIDFromCode'."); 
        return (LNG_LanguageInvalidLanguageCode);
    }

    if ( puiLanguageID == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'puiLanguageID' parameter passed to 'iLngGetLanguageIDFromCode'."); 
        return (LNG_ReturnParameterError);
    }


    /* Get the language ID for this language code */
    if ( (iError = iLngGetIDForNameFromDictionary(pldLngLanguageDictionaryListGlobal, pucLanguageCode, puiLanguageID)) != LNG_NoError ) {
        return (LNG_LanguageInvalidLanguageCode);
    }


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLngGetLanguageCodeFromID()

    Purpose:    This function gets the language code for the language ID

    Parameters: uiLanguageID            Language ID
                pucLanguageCode         Return pointer for the language code
                uiLanguageCodeLength    Length of the language code

    Globals:    pldLngLanguageDictionaryListGlobal

    Returns:    An LNG error code

*/
int iLngGetLanguageCodeFromID
(
    unsigned int uiLanguageID,
    unsigned char *pucLanguageCode,
    unsigned int uiLanguageCodeLength
)
{

    int     iError = LNG_NoError;


    /* Check the parameters */
    if ( uiLanguageID < 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiLanguageID' parameter passed to 'iLngGetLanguageCodeFromID'."); 
        return (LNG_LanguageInvalidLanguageID);
    }

    if ( pucLanguageCode == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pucLanguageCode' parameter passed to 'iLngGetLanguageCodeFromID'."); 
        return (LNG_ReturnParameterError);
    }

    if ( uiLanguageCodeLength <= 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiLanguageCodeLength' parameter passed to 'iLngGetLanguageCodeFromID'."); 
        return (LNG_ParameterError);
    }


    /* Get the language code for the language ID */
    if ( (iError = iLngGetNameForIDFromDictionary(pldLngLanguageDictionaryListGlobal, uiLanguageID, pucLanguageCode, uiLanguageCodeLength)) != LNG_NoError ) {
        return (LNG_LanguageInvalidLanguageID);
    }


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLngIsLanguageIDCanonical()

    Purpose:    This function returns true if the language ID passed is canonical

    Parameters: uiLanguageID    Language ID
                pbCanonical     Return pointer for the canonical flag

    Globals:    

    Returns:    An LNG error code

*/
int iLngIsLanguageIDCanonical
(
    unsigned int uiLanguageID,
    boolean *pbCanonical
)
{

    /* Check the parameters */
    if ( uiLanguageID < 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiLanguageID' parameter passed to 'iLngIsLanguageIDCanonical'."); 
        return (LNG_LanguageInvalidLanguageID);
    }

    if ( pbCanonical == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pbCanonical' parameter passed to 'iLngIsLanguageIDCanonical'."); 
        return (LNG_ReturnParameterError);
    }


    /* Check the canonical range */
    if ( (uiLanguageID >= LNG_LANGUAGE_MIN_CANONICAL_ID) && (uiLanguageID <= LNG_LANGUAGE_MAX_CANONICAL_ID) ) {
        *pbCanonical = true;
        return (LNG_NoError);
    }

    /* Check the regional range */
    if ( (uiLanguageID >= LNG_LANGUAGE_MIN_REGIONAL_ID) && (uiLanguageID <= LNG_LANGUAGE_MAX_REGIONAL_ID) ) {
        *pbCanonical = false;
        return (LNG_NoError);
    }


    return (LNG_LanguageInvalidLanguageID);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLngGetCanonicalLanguageID()

    Purpose:    This function gets the canonical language ID for the language ID

    Parameters: uiLanguageID                Language ID
                puiCanonicalLanguageID      Return pointer for the canonical language ID

    Globals:    

    Returns:    An LNG error code

*/
int iLngGetCanonicalLanguageID
(
    unsigned int uiLanguageID,
    unsigned int *puiCanonicalLanguageID
)
{

    /* Check the parameters */
    if ( uiLanguageID < 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiLanguageID' parameter passed to 'iLngGetCanonicalLanguageID'."); 
        return (LNG_LanguageInvalidLanguageID);
    }

    if ( puiCanonicalLanguageID == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'puiCanonicalLanguageID' parameter passed to 'iLngGetCanonicalLanguageID'."); 
        return (LNG_ReturnParameterError);
    }


    /* This is automatically canonical - yes but is it valid?? */
    if ( uiLanguageID == LNG_LANGUAGE_ANY_ID ) {
        *puiCanonicalLanguageID = LNG_LANGUAGE_ANY_ID;
        return (LNG_NoError);
    }

    /* Check the canonical range */
    if ( (uiLanguageID >= LNG_LANGUAGE_MIN_CANONICAL_ID) && (uiLanguageID <= LNG_LANGUAGE_MAX_CANONICAL_ID) ) {
        *puiCanonicalLanguageID = uiLanguageID;
        return (LNG_NoError);
    }

    /* Check the regional range */
    if ( (uiLanguageID >= LNG_LANGUAGE_MIN_REGIONAL_ID) && (uiLanguageID <= LNG_LANGUAGE_MAX_REGIONAL_ID) ) {
        *puiCanonicalLanguageID = (uiLanguageID / 100);
        return (LNG_NoError);
    }


    return (LNG_LanguageInvalidLanguageID);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLngCheckCharacterSetID()

    Purpose:    This function checks the passed character set ID

    Parameters: uiCharacterSetID        Character set ID

    Globals:    none

    Returns:    An LNG error code

*/
int iLngCheckCharacterSetID
(
    unsigned int uiCharacterSetID
)
{

    unsigned char   pucCharacterSetName[LNG_CHARACTER_SET_NAME_LENGTH + 1] = {'\0'};


    /* Check the parameters */
    if ( uiCharacterSetID < 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiCharacterSetID' parameter passed to 'iLngCheckCharacterSetID'."); 
        return (LNG_LanguageInvalidCharacterSetID);
    }


    /* Check the character set ID */
    return (iLngGetCharacterSetNameFromID(uiCharacterSetID, pucCharacterSetName, LNG_CHARACTER_SET_NAME_LENGTH + 1));

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLngCheckCharacterSetName()

    Purpose:    This function checks the passed character set name

    Parameters: pucCharacterSetName     Character set name

    Globals:    none

    Returns:    An LNG error code

*/
int iLngCheckCharacterSetName
(
    unsigned char *pucCharacterSetName
)
{

    unsigned int    uiCharacterSetID = LNG_CHARACTER_SET_ANY_ID;


    /* Check the parameters */
    if ( bUtlStringsIsStringNULL(pucCharacterSetName) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pucCharacterSetName' parameter passed to 'iLngCheckCharacterSetName'."); 
        return (LNG_LanguageInvalidCharacterSetName);
    }


    /* Check the character set name */
    return (iLngGetCharacterSetIDFromName(pucCharacterSetName, &uiCharacterSetID));

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLngGetCharacterSetIDFromName()

    Purpose:    This function gets the character set ID for the character set name

    Parameters: pucCharacterSetName     Character set name
                puiCharacterSetID       Return pointer for the character set ID

    Globals:    pldLngCharacterSetDictionaryListGlobal

    Returns:    An LNG error code

*/
int iLngGetCharacterSetIDFromName
(
    unsigned char *pucCharacterSetName,
    unsigned int *puiCharacterSetID
)
{

    int     iError = LNG_NoError;


    /* Check the parameters */
    if ( bUtlStringsIsStringNULL(pucCharacterSetName) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pucCharacterSetName' parameter passed to 'iLngGetCharacterSetIDFromName'."); 
        return (LNG_LanguageInvalidCharacterSetName);
    }

    if ( puiCharacterSetID == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'puiCharacterSetID' parameter passed to 'iLngGetCharacterSetIDFromName'."); 
        return (LNG_ReturnParameterError);
    }


    /* Get the character set ID for this character set name */
    if ( (iError = iLngGetIDForNameFromDictionary(pldLngCharacterSetDictionaryListGlobal, pucCharacterSetName, puiCharacterSetID)) != LNG_NoError ) {
        return (LNG_LanguageInvalidCharacterSetName);
    }


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLngGetCharacterSetNameFromID()

    Purpose:    This function gets the character set name for the character set ID

    Parameters: uiCharacterSetID            Character set ID
                pucCharacterSetName         Return pointer for the character set name
                uiCharacterSetNameLength    Length of the character set name

    Globals:    pldLngCharacterSetDictionaryListGlobal

    Returns:    An LNG error code

*/
int iLngGetCharacterSetNameFromID
(
    unsigned int uiCharacterSetID,
    unsigned char *pucCharacterSetName,
    unsigned int uiCharacterSetNameLength
)
{

    int     iError = LNG_NoError;


    /* Check the parameters */
    if ( uiCharacterSetID < 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiCharacterSetID' parameter passed to 'iLngGetCharacterSetNameFromID'."); 
        return (LNG_LanguageInvalidCharacterSetID);
    }

    if ( pucCharacterSetName == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pucCharacterSetName' parameter passed to 'iLngGetCharacterSetNameFromID'."); 
        return (LNG_ReturnParameterError);
    }

    if ( uiCharacterSetNameLength <= 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiCharacterSetNameLength' parameter passed to 'iLngGetCharacterSetNameFromID'."); 
        return (LNG_ParameterError);
    }


    /* Get the character set name for the character set ID */
    if ( (iError = iLngGetNameForIDFromDictionary(pldLngCharacterSetDictionaryListGlobal, uiCharacterSetID, pucCharacterSetName, uiCharacterSetNameLength)) != LNG_NoError ) {
        return (LNG_LanguageInvalidCharacterSetID);
    }


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLngCheckTokenizerID()

    Purpose:    This function checks the passed tokenizer ID

    Parameters: uiTokenizerID       Tokenizer ID

    Globals:    none

    Returns:    An LNG error code

*/
int iLngCheckTokenizerID
(
    unsigned int uiTokenizerID
)
{

    unsigned char   pucTokenizerName[LNG_TOKENIZER_NAME_LENGTH + 1] = {'\0'};


    /* Check the parameters */
    if ( uiTokenizerID < 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiTokenizerID' parameter passed to 'iLngCheckTokenizerID'."); 
        return (LNG_LanguageInvalidTokenizerID);
    }


    /* Check the tokenizer ID */
    return (iLngGetTokenizerNameFromID(uiTokenizerID, pucTokenizerName, LNG_TOKENIZER_NAME_LENGTH + 1));

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLngCheckTokenizerName()

    Purpose:    This function checks the passed tokenizer name

    Parameters: pucTokenizerName        Tokenizer name

    Globals:    none

    Returns:    An LNG error code

*/
int iLngCheckTokenizerName
(
    unsigned char *pucTokenizerName
)
{

    unsigned int    uiTokenizerID = LNG_TOKENIZER_ANY_ID;

    
    /* Check the parameters */
    if ( bUtlStringsIsStringNULL(pucTokenizerName) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pucTokenizerName' parameter passed to 'iLngCheckTokenizerName'."); 
        return (LNG_LanguageInvalidTokenizerName);
    }


    /* Validate the tokenizer name */
    return (iLngGetTokenizerIDFromName(pucTokenizerName, &uiTokenizerID));

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLngGetTokenizerIDFromName()

    Purpose:    This function gets the tokenizer ID for the tokenizer name

    Parameters: pucTokenizerName    Tokenizer name
                puiTokenizerID      Return pointer for the tokenizer ID

    Globals:    pldLngTokenizerDictionaryListGlobal

    Returns:    An LNG error code

*/
int iLngGetTokenizerIDFromName
(
    unsigned char *pucTokenizerName,
    unsigned int *puiTokenizerID
)
{

    int     iError = LNG_NoError;


    /* Check the parameters */
    if ( bUtlStringsIsStringNULL(pucTokenizerName) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pucTokenizerName' parameter passed to 'iLngGetTokenizerIDFromName'."); 
        return (LNG_LanguageInvalidTokenizerName);
    }

    if ( puiTokenizerID == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'puiTokenizerID' parameter passed to 'iLngGetTokenizerIDFromName'."); 
        return (LNG_ReturnParameterError);
    }


    /* Get the tokenizer ID for this tokenizer name */
    if ( (iError = iLngGetIDForNameFromDictionary(pldLngTokenizerDictionaryListGlobal, pucTokenizerName, puiTokenizerID)) != LNG_NoError ) {
        return (LNG_LanguageInvalidTokenizerName);
    }

    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLngGetTokenizerNameFromID()

    Purpose:    This function gets the tokenizer name for the tokenizer ID

    Parameters: uiTokenizerID           Tokenizer ID
                pucTokenizerName        Return pointer for the tokenizer name
                uiTokenizerNameLength   Length of the tokenizer name

    Globals:    pldLngTokenizerDictionaryListGlobal

    Returns:    An LNG error code

*/
int iLngGetTokenizerNameFromID
(
    unsigned int uiTokenizerID,
    unsigned char *pucTokenizerName,
    unsigned int uiTokenizerNameLength
)
{

    int     iError = LNG_NoError;


    /* Check the parameters */
    if ( uiTokenizerID < 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiTokenizerID' parameter passed to 'iLngGetTokenizerNameFromID'."); 
        return (LNG_LanguageInvalidTokenizerID);
    }

    if ( pucTokenizerName == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pucTokenizerName' parameter passed to 'iLngGetTokenizerNameFromID'."); 
        return (LNG_ReturnParameterError);
    }

    if ( uiTokenizerNameLength <= 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiTokenizerNameLength' parameter passed to 'iLngGetTokenizerNameFromID'."); 
        return (LNG_ParameterError);
    }


    /* Get the tokenizer name for the tokenizer ID */
    if ( (iError = iLngGetNameForIDFromDictionary(pldLngTokenizerDictionaryListGlobal, uiTokenizerID, pucTokenizerName, uiTokenizerNameLength)) != LNG_NoError ) {
        return (LNG_LanguageInvalidTokenizerID);
    }


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLngCheckStemmerID()

    Purpose:    This function checks the passed stemmer ID

    Parameters: uiStemmerID     Stemmer ID

    Globals:    none

    Returns:    An LNG error code

*/
int iLngCheckStemmerID
(
    unsigned int uiStemmerID
)
{

    unsigned char   pucStemmerName[LNG_STEMMER_NAME_LENGTH + 1] = {'\0'};


    /* Check the parameters */
    if ( uiStemmerID < 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiStemmerID' parameter passed to 'iLngCheckStemmerID'."); 
        return (LNG_LanguageInvalidStemmerID);
    }


    /* Check the stemmer ID */
    return (iLngGetStemmerNameFromID(uiStemmerID, pucStemmerName, LNG_STEMMER_NAME_LENGTH + 1));

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLngCheckStemmerName()

    Purpose:    This function checks the passed stemmer name

    Parameters: pucStemmerName      Stemmer name

    Globals:    none

    Returns:    An LNG error code

*/
int iLngCheckStemmerName
(
    unsigned char *pucStemmerName
)
{

    unsigned int    uiStemmerID = LNG_TOKENIZER_ANY_ID;

    
    /* Check the parameters */
    if ( bUtlStringsIsStringNULL(pucStemmerName) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pucStemmerName' parameter passed to 'iLngCheckStemmerName'."); 
        return (LNG_LanguageInvalidStemmerName);
    }


    /* Check the stemmer name */
    return (iLngGetStemmerIDFromName(pucStemmerName, &uiStemmerID));

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLngGetStemmerIDFromName()

    Purpose:    This function gets the stemmer ID for the stemmer name

    Parameters: pucStemmerName      Stemmer name
                puiStemmerID        Return pointer for the stemmer ID

    Globals:    pldLngStemmerDictionaryListGlobal

    Returns:    An LNG error code

*/
int iLngGetStemmerIDFromName
(
    unsigned char *pucStemmerName,
    unsigned int *puiStemmerID
)
{

    int     iError = LNG_NoError;


    /* Check the parameters */
    if ( bUtlStringsIsStringNULL(pucStemmerName) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pucStemmerName' parameter passed to 'iLngGetStemmerIDFromName'."); 
        return (LNG_LanguageInvalidStemmerName);
    }

    if ( puiStemmerID == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'puiStemmerID' parameter passed to 'iLngGetStemmerIDFromName'."); 
        return (LNG_ReturnParameterError);
    }


    /* Get the stemmer ID for this stemmer name */
    if ( (iError = iLngGetIDForNameFromDictionary(pldLngStemmerDictionaryListGlobal, pucStemmerName, puiStemmerID)) != LNG_NoError ) {
        return (LNG_LanguageInvalidStemmerName);
    }


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLngGetStemmerNameFromID()

    Purpose:    This function gets the stemmer name for the stemmer ID

    Parameters: uiStemmerID             Stemmer ID
                pucStemmerName          Return pointer for the stemmer name
                uiStemmerNameLength     Length of the stemmer name

    Globals:    pldLngStemmerDictionaryListGlobal

    Returns:    An LNG error code

*/
int iLngGetStemmerNameFromID
(
    unsigned int uiStemmerID,
    unsigned char *pucStemmerName,
    unsigned int uiStemmerNameLength
)
{

    int     iError = LNG_NoError;


    /* Check the parameters */
    if ( uiStemmerID < 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiStemmerID' parameter passed to 'iLngGetStemmerNameFromID'."); 
        return (LNG_LanguageInvalidStemmerID);
    }

    if ( pucStemmerName == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pucStemmerName' parameter passed to 'iLngGetStemmerNameFromID'."); 
        return (LNG_ReturnParameterError);
    }

    if ( uiStemmerNameLength <= 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiStemmerNameLength' parameter passed to 'iLngGetStemmerNameFromID'."); 
        return (LNG_ParameterError);
    }


    /* Get the stemmer name for the stemmer ID */
    if ( (iError = iLngGetNameForIDFromDictionary(pldLngStemmerDictionaryListGlobal, uiStemmerID, pucStemmerName, uiStemmerNameLength)) != LNG_NoError ) {
        return (LNG_LanguageInvalidStemmerID);
    }


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLngCheckSoundexID()

    Purpose:    This function checks the passed soundex ID

    Parameters: uiSoundexID     Soundex ID

    Globals:    none

    Returns:    An LNG error code

*/
int iLngCheckSoundexID
(
    unsigned int uiSoundexID
)
{

    unsigned char   pucSoundexName[LNG_SOUNDEX_NAME_LENGTH + 1] = {'\0'};


    /* Check the parameters */
    if ( uiSoundexID < 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiSoundexID' parameter passed to 'iLngCheckSoundexID'."); 
        return (LNG_LanguageInvalidSoundexID);
    }


    /* Check the soundex ID */
    return (iLngGetSoundexNameFromID(uiSoundexID, pucSoundexName, LNG_SOUNDEX_NAME_LENGTH + 1));

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLngCheckSoundexName()

    Purpose:    This function checks the passed soundex name

    Parameters: pucSoundexName      Soundex name

    Globals:    none

    Returns:    An LNG error code

*/
int iLngCheckSoundexName
(
    unsigned char *pucSoundexName
)
{

    unsigned int    uiSoundexID = LNG_SOUNDEX_ANY_ID;


    /* Check the parameters */
    if ( bUtlStringsIsStringNULL(pucSoundexName) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pucSoundexName' parameter passed to 'iLngCheckSoundexName'."); 
        return (LNG_LanguageInvalidSoundexName);
    }


    /* Check the soundex name */
    return (iLngGetSoundexIDFromName(pucSoundexName, &uiSoundexID));

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLngGetSoundexIDFromName()

    Purpose:    This function gets the soundex ID for the soundex name

    Parameters: pucSoundexName      Soundex name
                puiSoundexID        Return pointer for the soundex ID

    Globals:    pldLngSoundexDictionaryListGlobal

    Returns:    An LNG error code

*/
int iLngGetSoundexIDFromName
(
    unsigned char *pucSoundexName,
    unsigned int *puiSoundexID
)
{

    int     iError = LNG_NoError;


    /* Check the parameters */
    if ( bUtlStringsIsStringNULL(pucSoundexName) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pucSoundexName' parameter passed to 'iLngGetSoundexIDFromName'."); 
        return (LNG_LanguageInvalidSoundexName);
    }

    if ( puiSoundexID == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'puiSoundexID' parameter passed to 'iLngGetSoundexIDFromName'."); 
        return (LNG_ReturnParameterError);
    }


    /* Get the soundex ID for this soundex name */
    if ( (iError = iLngGetIDForNameFromDictionary(pldLngSoundexDictionaryListGlobal, pucSoundexName, puiSoundexID)) != LNG_NoError ) {
        return (LNG_LanguageInvalidSoundexName);
    }


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLngGetSoundexNameFromID()

    Purpose:    This function gets the soundex name for the soundex ID

    Parameters: uiSoundexID         Soundex ID
                pucSoundexName      Return pointer for the soundex name
                uiSoundexNameLen    Length of the soundex name

    Globals:    pldLngSoundexDictionaryListGlobal

    Returns:    An LNG error code

*/
int iLngGetSoundexNameFromID
(
    unsigned int uiSoundexID,
    unsigned char *pucSoundexName,
    unsigned int uiSoundexNameLen
)
{

    int     iError = LNG_NoError;


    /* Check the parameters */
    if ( uiSoundexID < 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiSoundexID' parameter passed to 'iLngGetSoundexNameFromID'."); 
        return (LNG_LanguageInvalidSoundexID);
    }

    if ( pucSoundexName == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pucSoundexName' parameter passed to 'iLngGetSoundexNameFromID'."); 
        return (LNG_ReturnParameterError);
    }

    if ( uiSoundexNameLen <= 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiSoundexNameLen' parameter passed to 'iLngGetSoundexNameFromID'."); 
        return (LNG_ParameterError);
    }


    /* Get the soundex name for the soundex ID */
    if ( (iError = iLngGetNameForIDFromDictionary(pldLngSoundexDictionaryListGlobal, uiSoundexID, pucSoundexName, uiSoundexNameLen)) != LNG_NoError ) {
        return (LNG_LanguageInvalidSoundexID);
    }


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLngCheckMetaphoneID()

    Purpose:    This function checks the passed metaphone ID

    Parameters: uiMetaphoneID       Metaphone ID

    Globals:    none

    Returns:    An LNG error code

*/
int iLngCheckMetaphoneID
(
    unsigned int uiMetaphoneID
)
{

    unsigned char   pucMetaphoneName[LNG_METAPHONE_NAME_LENGTH + 1] = {'\0'};

    
    /* Check the parameters */
    if ( uiMetaphoneID < 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiMetaphoneID' parameter passed to 'iLngCheckMetaphoneID'."); 
        return (LNG_LanguageInvalidMetaphoneID);
    }


    /* Check the metaphone ID */
    return (iLngGetMetaphoneNameFromID(uiMetaphoneID, pucMetaphoneName, LNG_METAPHONE_NAME_LENGTH + 1));

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLngCheckMetaphoneName()

    Purpose:    This function checks the passed metaphone name

    Parameters: pucMetaphoneName        Metaphone name

    Globals:    none

    Returns:    An LNG error code

*/
int iLngCheckMetaphoneName
(
    unsigned char *pucMetaphoneName
)
{

    unsigned int    uiMetaphoneID = LNG_METAPHONE_ANY_ID;


    /* Check the parameters */
    if ( bUtlStringsIsStringNULL(pucMetaphoneName) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pucMetaphoneName' parameter passed to 'iLngCheckMetaphoneName'."); 
        return (LNG_LanguageInvalidMetaphoneName);
    }


    /* Check the metaphone name */
    return (iLngGetMetaphoneIDFromName(pucMetaphoneName, &uiMetaphoneID));

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLngGetMetaphoneIDFromName()

    Purpose:    This function gets the metaphone ID for the metaphone name

    Parameters: pucMetaphoneName        Metaphone name
                puiMetaphoneID          Return pointer for the metaphone ID

    Globals:    pldLngMetaphoneDictionaryListGlobal

    Returns:    An LNG error code

*/
int iLngGetMetaphoneIDFromName
(
    unsigned char *pucMetaphoneName,
    unsigned int *puiMetaphoneID
)
{

    int     iError = LNG_NoError;


    /* Check the parameters */
    if ( bUtlStringsIsStringNULL(pucMetaphoneName) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pucMetaphoneName' parameter passed to 'iLngGetMetaphoneIDFromName'."); 
        return (LNG_LanguageInvalidMetaphoneName);
    }

    if ( puiMetaphoneID == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'puiMetaphoneID' parameter passed to 'iLngGetMetaphoneIDFromName'."); 
        return (LNG_ReturnParameterError);
    }


    /* Get the metaphone ID for this metaphone name */
    if ( (iError = iLngGetIDForNameFromDictionary(pldLngMetaphoneDictionaryListGlobal, pucMetaphoneName, puiMetaphoneID)) != LNG_NoError ) {
        return (LNG_LanguageInvalidMetaphoneName);
    }


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLngGetMetaphoneNameFromID()

    Purpose:    This function gets the metaphone name for the metaphone ID

    Parameters: uiMetaphoneID           Metaphone ID
                pucMetaphoneName        Return pointer for the metaphone name
                uiMetaphoneNameLen      Length of the metaphone name

    Globals:    pldLngMetaphoneDictionaryListGlobal

    Returns:    An LNG error code

*/
int iLngGetMetaphoneNameFromID
(
    unsigned int uiMetaphoneID,
    unsigned char *pucMetaphoneName,
    unsigned int uiMetaphoneNameLen
)
{

    int     iError = LNG_NoError;


    /* Check the parameters */
    if ( uiMetaphoneID < 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiMetaphoneID' parameter passed to 'iLngGetMetaphoneNameFromID'."); 
        return (LNG_LanguageInvalidMetaphoneID);
    }

    if ( pucMetaphoneName == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pucMetaphoneName' parameter passed to 'iLngGetMetaphoneNameFromID'."); 
        return (LNG_ReturnParameterError);
    }

    if ( uiMetaphoneNameLen <= 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiMetaphoneNameLen' parameter passed to 'iLngGetMetaphoneNameFromID'."); 
        return (LNG_ParameterError);
    }


    /* Get the metaphone name for the metaphone ID */
    if ( (iError = iLngGetNameForIDFromDictionary(pldLngMetaphoneDictionaryListGlobal, uiMetaphoneID, pucMetaphoneName, uiMetaphoneNameLen)) != LNG_NoError ) {
        return (LNG_LanguageInvalidMetaphoneID);
    }


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLngCheckPhonixID()

    Purpose:    This function checks the passed phonix ID

    Parameters: uiPhonixID      Phonix ID

    Globals:    none

    Returns:    An LNG error code

*/
int iLngCheckPhonixID
(
    unsigned int uiPhonixID
)
{

    unsigned char   pucPhonixName[LNG_PHONIX_NAME_LENGTH + 1] = {'\0'};


    /* Check the parameters */
    if ( uiPhonixID < 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiPhonixID' parameter passed to 'iLngCheckPhonixID'."); 
        return (LNG_LanguageInvalidPhonixID);
    }


    /* Check the phonix ID */
    return (iLngGetPhonixNameFromID(uiPhonixID, pucPhonixName, LNG_PHONIX_NAME_LENGTH + 1));

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLngCheckPhonixName()

    Purpose:    This function checks the passed phonix name

    Parameters: pucPhonixName       Phonix name

    Globals:    none

    Returns:    An LNG error code

*/
int iLngCheckPhonixName
(
    unsigned char *pucPhonixName
)
{

    unsigned int    uiPhonixID = LNG_PHONIX_ANY_ID;

    
    /* Check the parameters */
    if ( bUtlStringsIsStringNULL(pucPhonixName) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pucPhonixName' parameter passed to 'iLngCheckPhonixName'."); 
        return (LNG_LanguageInvalidPhonixName);
    }


    /* Check the phonix name */
    return (iLngGetPhonixIDFromName(pucPhonixName, &uiPhonixID));

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLngGetPhonixIDFromName()

    Purpose:    This function gets the phonix ID for the phonix name

    Parameters: pucPhonixName       Phonix name
                puiPhonixID         Return pointer for the phonix ID

    Globals:    pldLngPhonixDictionaryListGlobal

    Returns:    An LNG error code

*/
int iLngGetPhonixIDFromName
(
    unsigned char *pucPhonixName,
    unsigned int *puiPhonixID
)
{

    int     iError = LNG_NoError;


    /* Check the parameters */
    if ( bUtlStringsIsStringNULL(pucPhonixName) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pucPhonixName' parameter passed to 'iLngGetPhonixIDFromName'."); 
        return (LNG_LanguageInvalidPhonixName);
    }

    if ( puiPhonixID == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'puiPhonixID' parameter passed to 'iLngGetPhonixIDFromName'."); 
        return (LNG_ReturnParameterError);
    }


    /* Get the phonix ID for this phonix name */
    if ( (iError = iLngGetIDForNameFromDictionary(pldLngPhonixDictionaryListGlobal, pucPhonixName, puiPhonixID)) != LNG_NoError ) {
        return (LNG_LanguageInvalidPhonixName);
    }


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLngGetPhonixNameFromID()

    Purpose:    This function gets the phonix name for the phonix ID

    Parameters: uiPhonixID          Phonix ID
                pucPhonixName       Return pointer for the phonix name
                uiPhonixNameLen     Length of the phonix name

    Globals:    pldLngPhonixDictionaryListGlobal

    Returns:    An LNG error code

*/
int iLngGetPhonixNameFromID
(
    unsigned int uiPhonixID,
    unsigned char *pucPhonixName,
    unsigned int uiPhonixNameLen
)
{

    int     iError = LNG_NoError;


    /* Check the parameters */
    if ( uiPhonixID < 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiPhonixID' parameter passed to 'iLngGetPhonixNameFromID'."); 
        return (LNG_LanguageInvalidPhonixID);
    }

    if ( pucPhonixName == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pucPhonixName' parameter passed to 'iLngGetPhonixNameFromID'."); 
        return (LNG_ReturnParameterError);
    }

    if ( uiPhonixNameLen <= 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiPhonixNameLen' parameter passed to 'iLngGetPhonixNameFromID'."); 
        return (LNG_ParameterError);
    }


    /* Get the phonix name for the phonix ID */
    if ( (iError = iLngGetNameForIDFromDictionary(pldLngPhonixDictionaryListGlobal, uiPhonixID, pucPhonixName, uiPhonixNameLen)) != LNG_NoError ) {
        return (LNG_LanguageInvalidPhonixID);
    }


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLngCheckTypoID()

    Purpose:    This function checks the passed typo ID

    Parameters: uiTypoID        Typo ID

    Globals:    none

    Returns:    An LNG error code

*/
int iLngCheckTypoID
(
    unsigned int uiTypoID
)
{

    unsigned char   pucTypoName[LNG_TYPO_NAME_LENGTH + 1] = {'\0'};

    
    /* Check the parameters */
    if ( uiTypoID < 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiTypoID' parameter passed to 'iLngCheckTypoID'."); 
        return (LNG_LanguageInvalidTypoID);
    }


    /* Check the typo ID */
    return (iLngGetTypoNameFromID(uiTypoID, pucTypoName, LNG_TYPO_NAME_LENGTH + 1));

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLngCheckTypoName()

    Purpose:    This function checks the passed typo name

    Parameters: pucTypoName     Typo name

    Globals:    none

    Returns:    An LNG error code

*/
int iLngCheckTypoName
(
    unsigned char *pucTypoName
)
{

    unsigned int    uiTypoID = LNG_TYPO_ANY_ID;


    /* Check the parameters */
    if ( bUtlStringsIsStringNULL(pucTypoName) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pucTypoName' parameter passed to 'iLngCheckTypoName'."); 
        return (LNG_LanguageInvalidTypoName);
    }


    /* Check the typo name */
    return (iLngGetTypoIDFromName(pucTypoName, &uiTypoID));

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLngGetTypoIDFromName()

    Purpose:    This function gets the typo ID for the typo name

    Parameters: pucTypoName     Typo name
                puiTypoID       Return pointer for the typo ID

    Globals:    pldLngTypoDictionaryListGlobal

    Returns:    An LNG error code

*/
int iLngGetTypoIDFromName
(
    unsigned char *pucTypoName,
    unsigned int *puiTypoID
)
{

    int     iError = LNG_NoError;


    /* Check the parameters */
    if ( bUtlStringsIsStringNULL(pucTypoName) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pucTypoName' parameter passed to 'iLngGetTypoIDFromName'."); 
        return (LNG_LanguageInvalidTypoName);
    }

    if ( puiTypoID == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'puiTypoID' parameter passed to 'iLngGetTypoIDFromName'."); 
        return (LNG_ReturnParameterError);
    }


    /* Get the typo ID for this typo name */
    if ( (iError = iLngGetIDForNameFromDictionary(pldLngTypoDictionaryListGlobal, pucTypoName, puiTypoID)) != LNG_NoError ) {
        return (LNG_LanguageInvalidTypoName);
    }


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLngGetTypoNameFromID()

    Purpose:    This function gets the typo name for the typo ID

    Parameters: uiTypoID            Typo ID
                pucTypoName         Return pointer for the typo name
                uiTypoNameLen       Length of the typo name

    Globals:    pldLngTypoDictionaryListGlobal

    Returns:    An LNG error code

*/
int iLngGetTypoNameFromID
(
    unsigned int uiTypoID,
    unsigned char *pucTypoName,
    unsigned int uiTypoNameLen
)
{

    int     iError = LNG_NoError;


    /* Check the parameters */
    if ( uiTypoID < 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiTypoID' parameter passed to 'iLngGetTypoNameFromID'."); 
        return (LNG_LanguageInvalidTypoID);
    }

    if ( pucTypoName == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pucTypoName' parameter passed to 'iLngGetTypoNameFromID'."); 
        return (LNG_ReturnParameterError);
    }

    if ( uiTypoNameLen <= 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiTypoNameLen' parameter passed to 'iLngGetTypoNameFromID'."); 
        return (LNG_ParameterError);
    }


    /* Get the typo name for the typo ID */
    if ( (iError = iLngGetNameForIDFromDictionary(pldLngTypoDictionaryListGlobal, uiTypoID, pucTypoName, uiTypoNameLen)) != LNG_NoError ) {
        return (LNG_LanguageInvalidTypoID);
    }


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLngCheckStopListID()

    Purpose:    This function checks the passed stop list ID

    Parameters: uiStopListID        Stop list ID

    Globals:    none

    Returns:    An LNG error code

*/
int iLngCheckStopListID
(
    unsigned int uiStopListID
)
{

    unsigned char   pucStopListName[LNG_STOP_LIST_NAME_LENGTH + 1] = {'\0'};


    /* Check the parameters */
    if ( uiStopListID < 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiStopListID' parameter passed to 'iLngCheckStopListID'."); 
        return (LNG_LanguageInvalidStopListID);
    }


    /* Check the stop list ID */
    return (iLngGetStopListNameFromID(uiStopListID, pucStopListName, LNG_STOP_LIST_NAME_LENGTH + 1));

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLngCheckStopListName()

    Purpose:    This function checks the passed stop list name

    Parameters: pucStopListName     Stop list name

    Globals:    none

    Returns:    An LNG error code

*/
int iLngCheckStopListName
(
    unsigned char *pucStopListName
)
{

    unsigned int    uiStopListID = LNG_STOP_LIST_ANY_ID;


    /* Check the parameters */
    if ( bUtlStringsIsStringNULL(pucStopListName) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pucStopListName' parameter passed to 'iLngCheckStopListName'."); 
        return (LNG_LanguageInvalidStopListName);
    }


    /* Check the stop list name */
    return (iLngGetStopListIDFromName(pucStopListName, &uiStopListID));

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLngGetStopListIDFromName()

    Purpose:    This function gets the stop list ID for the stop list name

    Parameters: pucStopListName     Stop list name
                puiStopListID       Return pointer for the stop list ID

    Globals:    pldLngStopListDictionaryListGlobal

    Returns:    An LNG error code

*/
int iLngGetStopListIDFromName
(
    unsigned char *pucStopListName,
    unsigned int *puiStopListID
)
{

    int     iError = LNG_NoError;


    /* Check the parameters */
    if ( bUtlStringsIsStringNULL(pucStopListName) == true ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null or empty 'pucStopListName' parameter passed to 'iLngGetStopListIDFromName'."); 
        return (LNG_LanguageInvalidStopListName);
    }

    if ( puiStopListID == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'puiStopListID' parameter passed to 'iLngGetStopListIDFromName'."); 
        return (LNG_ReturnParameterError);
    }


    /* Get the stop list ID for this stop list name */
    if ( (iError = iLngGetIDForNameFromDictionary(pldLngStopListDictionaryListGlobal, pucStopListName, puiStopListID)) != LNG_NoError ) {
        return (LNG_LanguageInvalidStopListName);
    }


    return (iError);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLngGetStopListNameFromID()

    Purpose:    This function gets the stop list name for the stop list ID

    Parameters: uiStopListID                Stop list ID
                pucStopListName             Return pointer for the stop list name
                uiStopListNameLength        Length of the stop list name

    Globals:    pldLngStopListDictionaryListGlobal

    Returns:    An LNG error code

*/
int iLngGetStopListNameFromID
(
    unsigned int uiStopListID,
    unsigned char *pucStopListName,
    unsigned int uiStopListNameLength
)
{

    int     iError = LNG_NoError;


    /* Check the parameters */
    if ( uiStopListID < 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiStopListID' parameter passed to 'iLngGetStopListNameFromID'."); 
        return (LNG_LanguageInvalidStopListID);
    }

    if ( pucStopListName == NULL ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Null 'pucStopListName' parameter passed to 'iLngGetStopListNameFromID'."); 
        return (LNG_ReturnParameterError);
    }

    if ( uiStopListNameLength <= 0 ) {
        iUtlLogError(UTL_LOG_CONTEXT, "Invalid 'uiStopListNameLength' parameter passed to 'iLngGetStopListNameFromID'."); 
        return (LNG_ParameterError);
    }


    /* Get the stop list name for the stop list ID */
    if ( (iError = iLngGetNameForIDFromDictionary(pldLngStopListDictionaryListGlobal, uiStopListID, pucStopListName, uiStopListNameLength)) != LNG_NoError ) {
        return (LNG_LanguageInvalidStopListID);
    }


    return (iError);

}


/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/


/*

    Function:   iLngGetIDForNameFromDictionary()

    Purpose:    This function gets the ID for a name from a dictionary

    Parameters: pldLngDictionary    Dictionary to search in
                pucName             Name to search for 
                puiID               Return pointer for the ID

    Globals:    none

    Returns:    An LNG error code

*/
static int iLngGetIDForNameFromDictionary
(
    struct lngDictionary *pldLngDictionary,
    unsigned char *pucName,
    unsigned int *puiID
)
{

    struct lngDictionary    *pldLngDictionaryPtr = pldLngDictionary;


    ASSERT(pldLngDictionary != NULL);
    ASSERT(bUtlStringsIsStringNULL(pucName) == false);
    ASSERT(puiID != NULL);


    /* Loop over the dictionary looking for the name we want */
    for ( pldLngDictionaryPtr = pldLngDictionary; pldLngDictionaryPtr->pucName != NULL; pldLngDictionaryPtr++) {

        /* Select this ID if there is a match on the name */
        if ( s_strcasecmp(pldLngDictionaryPtr->pucName, pucName) == 0 ) {
            
            /* Set the return pointer, and return */
            *puiID = pldLngDictionaryPtr->uiID;
            
            return (LNG_NoError);
        }
    }
    
    
    /* The name does not exist in this dictionary */
    return (LNG_LanguageInvalidName);

}


/*---------------------------------------------------------------------------*/


/*

    Function:   iLngGetNameForIDFromDictionary()

    Purpose:    This function gets the name for an ID from a dictionary

    Parameters: pldLngDictionary    Dictionary to search in
                uiID                ID to search for
                pucName             Return pointer for the name
                uiNameLength        Return pointer length

    Globals:    none

    Returns:    An LNG error code

*/
static int iLngGetNameForIDFromDictionary
(
    struct lngDictionary *pldLngDictionary,
    unsigned int uiID,
    unsigned char *pucName,
    unsigned int uiNameLength
)
{

    struct lngDictionary    *pldLngDictionaryPtr = pldLngDictionary;


    ASSERT(pldLngDictionary != NULL);
    ASSERT(uiID >= 0);
    ASSERT(pucName != NULL);
    ASSERT(uiNameLength > 0);


    /* Loop over the dictionary looking for the name we want */
    for ( pldLngDictionaryPtr = pldLngDictionary; pldLngDictionaryPtr->pucName != NULL; pldLngDictionaryPtr++) {

        /* Select this name if there is a match on the ID */
        if ( pldLngDictionaryPtr->uiID == uiID ) {
            
            /* Set the return pointer, and return */
            s_strnncpy(pucName, pldLngDictionaryPtr->pucName, uiNameLength);

            return (LNG_NoError);
        }
    }
    
    
    /* The name does not exist in this dictionary */
    return (LNG_LanguageInvalidID);

}


/*---------------------------------------------------------------------------*/
