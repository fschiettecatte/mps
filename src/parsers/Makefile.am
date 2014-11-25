
#*****************************************************************************
#       Copyright (C) 1993-2011, FS Consulting LLC. All rights reserved      *
#                                                                            *
#  This notice is intended as a precaution against inadvertent publication   *
#  and does not constitute an admission or acknowledgement that publication  *
#  has occurred or constitute a waiver of confidentiality.                   *
#                                                                            *
#  This software is the proprietary and confidential property                *
#  of FS Consulting LLC.                                                     *
#*****************************************************************************


# Includes
AM_CPPFLAGS = -I../spi -I../language -I../utils


# MPS libraries
mps_base_libs = ../language/liblanguage.a ../utils/libutils.a

if MPS_ENABLE_MECAB
  mps_mecab_libs = $(MPS_MECAB_LIBS)
else
  mps_mecab_libs =
endif

if MPS_ENABLE_ICU
  mps_icu_libs = ../language/icu/libicu.a $(MPS_ICU_LIBS)
else
  mps_icu_libs =
endif

mps_libs = $(mps_base_libs) $(mps_mecab_libs) $(mps_icu_libs)


# Applications
bin_PROGRAMS = mpsparser


# MPS Parser
mpsparser_SOURCES = mpsparser.c \
    parser.c parser.h \
    functions.c functions.h 

mpsparser_LDADD = $(mps_libs)



# Extras to distribute
EXTRA_DIST=Contents



# Clean
clean-local:
    rm -rf $(CLEAN_LOCAL)

