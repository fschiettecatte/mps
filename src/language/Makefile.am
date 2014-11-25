
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


if MPS_ENABLE_MECAB
#   mps_mecab_subdirs = mecab
  mps_mecab_subdirs =
else
  mps_mecab_subdirs =
endif

if MPS_ENABLE_ICU
  mps_icu_subdirs = icu
else
  mps_icu_subdirs =
endif

SUBDIRS = $(mps_mecab_subdirs) $(mps_icu_subdirs)



if MPS_ENABLE_MECAB
#   mps_mecab_cppflags = -I./mecab
  mps_mecab_cppflags =
else
  mps_mecab_cppflags =
endif

if MPS_ENABLE_ICU
  mps_icu_cppflags = -I./icu
else
  mps_icu_cppflags =
endif

AM_CPPFLAGS = $(mps_mecab_cppflags) $(mps_icu_cppflags) -I../utils $(MPS_ICU_INCLUDES) $(MPS_MECAB_INCLUDES)



# Language library sources
language_sources = case.c case.h \
    charset.h \
    convert.c convert.h \
    language.c language.h \
    lng_err.h \
    lng.h \
    location.c location.h \
    metaphone.c metaphone.h \
    phonix.c phonix.h \
    soundex.c soundex.h \
    stemmer.c stemmer.h \
    stoplist.c stoplist.h \
    tokenizer.c tokenizer.h \
    typo.c typo.h \
    unicode.c unicode.h


# Libraries
lib_LIBRARIES = liblanguage.a


# Language library
liblanguage_a_SOURCES = $(language_sources)



# Extras to distribute
EXTRA_DIST=Contents



# Clean
clean-local:
    rm -rf $(CLEAN_LOCAL)

