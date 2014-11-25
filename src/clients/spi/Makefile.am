
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
AM_CPPFLAGS = -I../../search -I../../report -I../../spi -I../../language -I../../utils


# Applications
bin_PROGRAMS = spiscript spisearch spitrec


# MPS libraries
mps_base_libs = ../../search/libsearch.a ../../report/libreport.a ../../spi/libspi.a ../../language/liblanguage.a ../../utils/libutils.a

if MPS_ENABLE_MECAB
  mps_mecab_libs = $(MPS_MECAB_LIBS)
else
  mps_mecab_libs =
endif

if MPS_ENABLE_ICU
  mps_icu_libs = ../../language/icu/libicu.a $(MPS_ICU_LIBS)
else
  mps_icu_libs =
endif

mps_libs = $(mps_base_libs) $(mps_mecab_libs) $(mps_icu_libs)


# Spiscript
spiscript_SOURCES = spiscript.c
spiscript_LDADD = $(mps_libs)


# Spisearch
spisearch_SOURCES = spisearch.c
spisearch_LDADD = $(mps_libs)


# Spitrec
spitrec_SOURCES = spitrec.c
spitrec_LDADD = $(mps_libs)



# Extras to distribute
EXTRA_DIST=Contents



# Clean
clean-local:
    rm -rf $(CLEAN_LOCAL)

