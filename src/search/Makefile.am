
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
AM_CPPFLAGS = -I../report -I../spi -I../language -I../utils


# Libraries
lib_LIBRARIES = libsearch.a


# Search library
libsearch_a_SOURCES = bitmap.c bitmap.h \
        cache.c cache.h \
        document.c document.h \
        feedback.c feedback.h \
        filepaths.c filepaths.h \
        filter.c filter.h \
        index.c index.h \
        indexer.c indexer.h \
        info.c info.h \
        invert.c invert.h \
        keydict.c keydict.h \
        language.c language.h \
        parser.c parser.h \
        posting.c posting.h \
        report.c report.h \
        retrieval.c retrieval.h \
        search.c search.h \
        shortrslt.c shortrslt.h \
        srch_err.h \
        srch.h \
        srchconf.h \
        stemmer.c stemmer.h \
        stoplist.c stoplist.h \
        termdict.c termdict.h \
        termlen.c termlen.h \
        termsrch.c termsrch.h \
        version.c version.h \
        weight.c weight.h


# Applications
bin_PROGRAMS = mpsindexer mpsterms mpsphrases


# MPS libraries
mps_base_libs = libsearch.a ../spi/libspi.a ../language/liblanguage.a ../utils/libutils.a

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


# MPS Indexer
mpsindexer_SOURCES = mpsindexer.c
mpsindexer_LDADD = $(mps_libs)


# MPS Terms
mpsterms_SOURCES = mpsterms.c
mpsterms_LDADD = $(mps_libs)


# MPS Phrases
mpsphrases_SOURCES = mpsphrases.c
mpsphrases_LDADD = $(mps_libs)



# Extras to distribute
EXTRA_DIST=Contents



# Clean
clean-local:
    rm -rf $(CLEAN_LOCAL)

